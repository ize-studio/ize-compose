
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include "Inkplate.h"            
#include "U8g2_for_Adafruit_GFX.h"   

#include <esp_sleep.h>
#include <driver/uart.h>
#include <Update.h> 
#include "jado.h"
// Writing fonts are loaded from SD into PSRAM. A small built-in font remains only as fallback.
#include "soc/rtc_cntl_reg.h" 
#include "soc/timer_group_reg.h"
#include <SdFat.h>

#ifndef IZE_ENABLE_DIRECT_GITHUB_SYNC
#define IZE_ENABLE_DIRECT_GITHUB_SYNC 0
#endif

#ifndef IZE_ENABLE_BLE_KEYBOARD
#define IZE_ENABLE_BLE_KEYBOARD 0
#endif

#if IZE_ENABLE_DIRECT_GITHUB_SYNC
#include <WiFiClientSecure.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>
#endif

#ifndef IZE_EXPERIMENTAL_DIRTY_TILE_REFRESH
#define IZE_EXPERIMENTAL_DIRTY_TILE_REFRESH 1
#endif

#ifndef IZE_DIRTY_TILE_REFRESH_DEBUG
#define IZE_DIRTY_TILE_REFRESH_DEBUG 0
#endif

struct GitSyncStateEntry;
struct GitRemoteDocEntry;

const char* APP_ROOT_DIR = "/ize_compose";
const char* FONT_DIR = "/ize_compose/hwalja";
const char* FIRMWARE_DIR = "/ize_compose/upload";
const char* INITIAL_IMAGE_PATH = "/ize_compose/initial.png";
const char* GITHUB_SYNC_STATE_PATH = "/ize_compose/github_sync_state.txt";
const char* LOCAL_DELETED_CSV_PATH = "/ize_compose/deleted.csv";
const char* FIRMWARE_UPDATE_PATH = "/ize_compose/upload/izefirmware.bin";
const char* LATIN_FONT_PATH = "/ize_compose/hwalja/hwalja_latin.bin";
const char* WEB_DOCUMENT_PAGE_PATH = "/ize_compose/ize_compose_1-4-0-test.html";
const char* SETTINGS_BACKUP_PATH = "/ize_compose/settings_backup.json";
// Minimal English fallback used only before SD fonts load or when an asset is missing.
const uint8_t* font_ptr = u8g2_font_5x7_tf;
const uint8_t* font_latin_ptr = u8g2_font_5x7_tf;
const uint8_t* font_hangul_ptr = nullptr;
const uint8_t* font_jamo_ptr = nullptr;
const uint8_t* font_jp_ptr = nullptr;
const uint8_t* font_greek_cyrillic_ptr = nullptr;
const uint8_t* font_arabic_ptr = nullptr;
const uint8_t* font_indic_ptr = nullptr;
const uint8_t* font_sea_ptr = nullptr;
const uint8_t* font_misc_ptr = nullptr;
int currentFontSlot = 1;
#define FIRMWARE_VERSION "v1.4.0-test" // Experimental direct GitHub sync and BLE removal
#define WEB_PAGE_VERSION "1-4-0-test"
const char* OFFICIAL_RELEASE_API = "https://api.github.com/repos/ize-studio/ize-compose/releases/latest";
const char* FIRMWARE_SIGNATURE = "RUPERT_OFFICIAL_KOR";
const gpio_num_t WAKE_BUTTON_PIN = GPIO_NUM_36;
enum AppMode { TYPING_MODE, FILE_MENU_MODE, INITIAL_MODE, SEARCH_MODE, WIFI_SCAN_MODE, WIFI_PASSWORD_MODE, WEB_PASSWORD_MODE };
class InkplateProxy : public Inkplate {
public:
    InkplateProxy(uint8_t mode) : Inkplate(mode), Adafruit_GFX(800, 600) {}
    void resetInternalCounter() { _partialUpdateCounter = 0; } 
};
extern InkplateProxy display;
InkplateProxy display(INKPLATE_1BIT); 

AppMode currentMode = TYPING_MODE;
bool needUpdate = false;
int lastCursorY = -1; // Last cursor Y from previous frame; -1 forces full redraw.
bool isUpdating = false;             
String getKeyboardLayoutIdString(KeyboardLayoutId id);
#include "insoe.h"
void handleDownload(); 
void handleRoot();
void handleDelete();
void handleRead();
void handleTextUpload();
void handleTextUploadComplete();
void handleSettingsJson();
void handleSettingsSave();
void handleWebAuth();
void handleDocumentsList();
void handleGithubSettingsJson();
void handleGithubSettingsSave();
void resetStatusScreenCache();
void drawStatusScreenFrame(const String& title, const String& line1, const String& line2, bool forceFullRefresh = false);
bool appendDeletedTombstone(const String& filename);

TaskHandle_t CalcTaskHandle;
volatile bool needCountUpdate = false; 
volatile bool calcBufferInUse = false;
String calcBuffer = "";               
volatile int sharedWordCount = 0;               
volatile int sharedCharCount = 0;
unsigned long lastCountRequestMs = 0;
const unsigned long COUNT_UPDATE_INTERVAL_MS = 100;
const unsigned long BATTERY_REFRESH_INTERVAL_MS = 5000;
AppMode lastMode = TYPING_MODE; 
NetworkSubMode lastNetSubMode = NET_MAIN;

NetworkSubMode currentNetSubMode = NET_MAIN;

bool isDeletingFile = false;
bool deleteCodePromptActive = false;
String deleteConfirmCode = "";
String deleteConfirmInput = "";
String pendingDeleteFilename = "";
String clipboard = "";
bool isEnglishInputMode = false;

NetworkSubMode tempNetCursor = NET_MAIN; 
float displayScale = 2.0;       

const float UI_SCALE = 2.0f;        
int baseFontSize = 16;     
String currentFileName = "doc0001.txt"; 

struct FileInfo {
    String name;
    String preview;
    float sizeKB;
    uint32_t time;
};

struct WrapMetrics {
    int lineCount;
    int lastLineWidth;
};

#if IZE_EXPERIMENTAL_DIRTY_TILE_REFRESH
struct IzeDirtyTileCandidate {
    bool active = false;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int strideBytes = 0;
    int snapshotXByte = 0;
    int snapshotBytesPerRow = 0;
    uint8_t *snapshot = nullptr;
};

struct IzeDirtyTileStats {
    int candidatePixels = 0;
    int dirtyTiles = 0;
    int dirtyMinX = 0;
    int dirtyMinY = 0;
    int dirtyMaxX = 0;
    int dirtyMaxY = 0;
    bool fallbackUsed = true;
};

IzeDirtyTileCandidate dirtyTailCandidate;
IzeDirtyTileStats lastDirtyTailStats;

void izeReleaseDirtyTileCandidate(IzeDirtyTileCandidate &c) {
    if (c.snapshot) {
        free(c.snapshot);
    }
    c = IzeDirtyTileCandidate();
}

bool izeCaptureDirtyTileCandidate(IzeDirtyTileCandidate &c, int x, int y, int w, int h) {
    izeReleaseDirtyTileCandidate(c);
    if (!display._partial || w <= 0 || h <= 0) return false;

    int displayW = display.width();
    int displayH = display.height();
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > displayW) w = displayW - x;
    if (y + h > displayH) h = displayH - y;
    if (w <= 0 || h <= 0) return false;

    int strideBytes = displayW / 8;
    int xByteStart = x / 8;
    int xByteEnd = (x + w + 7) / 8;
    int bytesPerRow = xByteEnd - xByteStart;
    size_t snapshotLen = (size_t)bytesPerRow * (size_t)h;
    uint8_t *snapshot = (uint8_t *)malloc(snapshotLen);
    if (!snapshot) return false;

    for (int row = 0; row < h; row++) {
        memcpy(snapshot + (size_t)row * bytesPerRow,
               display._partial + (size_t)(y + row) * strideBytes + xByteStart,
               bytesPerRow);
    }

    c.active = true;
    c.x = x;
    c.y = y;
    c.w = w;
    c.h = h;
    c.strideBytes = strideBytes;
    c.snapshotXByte = xByteStart;
    c.snapshotBytesPerRow = bytesPerRow;
    c.snapshot = snapshot;
    return true;
}

static inline bool izeSnapshotBit(const IzeDirtyTileCandidate &c, int px, int py) {
    int relX = px - c.snapshotXByte * 8;
    int relY = py - c.y;
    int byteIndex = relY * c.snapshotBytesPerRow + relX / 8;
    uint8_t mask = 1 << (relX & 7);
    return (c.snapshot[byteIndex] & mask) != 0;
}

static inline bool izeCurrentFramebufferBit(const IzeDirtyTileCandidate &c, int px, int py) {
    int byteIndex = py * c.strideBytes + px / 8;
    uint8_t mask = 1 << (px & 7);
    return (display._partial[byteIndex] & mask) != 0;
}

bool izeComputeDirtyTiles(IzeDirtyTileCandidate &c, IzeDirtyTileStats &stats) {
    stats = IzeDirtyTileStats();
    if (!c.active || !c.snapshot || !display._partial) return false;

    const int tileW = 16;
    const int tileH = 8;
    stats.candidatePixels = c.w * c.h;
    bool anyDirty = false;

    for (int ty = c.y; ty < c.y + c.h; ty += tileH) {
        int tileBottom = min(ty + tileH, c.y + c.h);
        for (int tx = c.x; tx < c.x + c.w; tx += tileW) {
            int tileRight = min(tx + tileW, c.x + c.w);
            bool tileDirty = false;
            for (int py = ty; py < tileBottom && !tileDirty; py++) {
                for (int px = tx; px < tileRight; px++) {
                    if (izeSnapshotBit(c, px, py) != izeCurrentFramebufferBit(c, px, py)) {
                        tileDirty = true;
                        break;
                    }
                }
            }
            if (tileDirty) {
                stats.dirtyTiles++;
                if (!anyDirty) {
                    stats.dirtyMinX = tx;
                    stats.dirtyMinY = ty;
                    stats.dirtyMaxX = tileRight;
                    stats.dirtyMaxY = tileBottom;
                    anyDirty = true;
                } else {
                    stats.dirtyMinX = min(stats.dirtyMinX, tx);
                    stats.dirtyMinY = min(stats.dirtyMinY, ty);
                    stats.dirtyMaxX = max(stats.dirtyMaxX, tileRight);
                    stats.dirtyMaxY = max(stats.dirtyMaxY, tileBottom);
                }
            }
        }
    }
    return true;
}

bool izeTryDirtyTilePanelRefresh(const IzeDirtyTileStats &) {
    // Inkplate's public API in this project exposes display() and full-screen
    // partialUpdate(), but no rectangle/tile partial refresh entry point. Keep
    // this isolated so any future Inkplate-version-sensitive panel call can be
    // added here without touching the renderer.
    return false;
}

void izeDebugDirtyTileStats(const IzeDirtyTileStats &stats) {
#if IZE_DIRTY_TILE_REFRESH_DEBUG
    Serial.print("[dirty-tail] candidate=");
    Serial.print(stats.candidatePixels);
    Serial.print("px tiles=");
    Serial.print(stats.dirtyTiles);
    Serial.print(" fallback=");
    Serial.println(stats.fallbackUsed ? "yes" : "no");
#endif
}
#endif

const int MAX_DOCUMENT_FILES = 256;
FileInfo files[MAX_DOCUMENT_FILES + 1]; 
int fileCount = 0;

uint8_t *imgBuffer = NULL;
uint32_t imgSize = 0;

int refreshLimit = 2000;   
int charCounter = 0;    
int lineSpacing = 2;     
int letterSpacing = 0;   
int typingSpeed = 0;     
int leftMenuOffset = 0;
int countMode = 2;
int charwordcount = 0;
int latinMode = 0;
int englishLayoutIndex = 1;
int keyboardLayoutIndex = 2;
int previewEnglishLayoutIndex = 1;
int previewKeyboardLayoutIndex = 2;
bool isAltPressed = false;
bool rtlTextMode = false;
unsigned long lastTypingTime = 0; 
char lastBaseChar = 0;            
int accentCycleIdx = 0;           
int lastAccentByteLen = 1;        
void loadSystemSettings();
void saveSystemSettings();
bool writeSettingsBackupFile();
bool restoreSettingsFromBackup();
String getKeyboardLayoutIdString(KeyboardLayoutId id);
void preloadInitialImage();
void refreshFileList();
void loadFile();
const uint8_t* loadFontToPSRAM(int slot); 
String searchQuery = ""; 
int searchMatchEnd = -1;  // confirmed match end position; -1 while query is only being edited
#include <Preferences.h> 
Preferences prefs;
String fullText = ""; 
int cursorPos = 0; 
bool isKoreanMode = false; 
bool isShiftPressed = false; 
bool isCtrlPressed = false; 
bool isCapsLockOn = false; 
unsigned long lastKeyPress = 0; 
bool statusBarNeedsUpdate = true; 
volatile int cachedBatteryPct = 0;
volatile bool cachedBatteryValid = false;
volatile bool displayIoBusy = false;
unsigned long lastBatteryReadMs = 0;
int statusBatteryPct = 0;
bool statusBatteryValid = false;
unsigned long lastStatusBatteryFetchMs = 0;
bool forceSafeFullTextRedraw = false;
unsigned long showSavedMessageTime = 0; 
bool savedMessageVisible = false;
bool updateScreenDrawn = false;

int autoSleepIndex = 2; 
unsigned long sleepIntervals[] = {30000, 60000, 300000, 600000, 1800000, 3600000, 0};
String sleepLabels[] = {"30s", "1m", "5m", "10m", "30m", "1h", "OFF"};
unsigned long lastInputTime = 0; 

int startIdx = 0;
int menuFocusSide = 0;   
int leftMenuIndex = 0;   
const int FILE_MENU_ITEMS_PER_PAGE = 12;
int fileScrollOffset = 0; 
bool isEditingValue = false; 
bool inSystemSubMenu = false; 
bool networkExitRequested = false; 
bool webDocumentUnlocked = false;
String savedWifiSsid = "";
String savedWifiPassword = "";
bool wifiConnectForOnlineSync = false;
static const int WIFI_SCAN_MAX = 12;
String wifiScanSsids[WIFI_SCAN_MAX];
int wifiScanRssi[WIFI_SCAN_MAX];
int wifiScanCount = 0;
int wifiScanSelected = 0;
String wifiPasswordBuffer = "";
String wifiStatusMessage = "";
IPAddress wifiStaIp;
String githubOwner = "";
String githubRepo = "";
String githubBranch = "main";
String githubPath = "documents";
String githubToken = "";
String githubSyncStatusMessage = "GitHub not connected";
String lastStatusScreenTitle = "";
String lastStatusScreenLine1 = "";
String lastStatusScreenLine2 = "";
bool statusScreenPrimed = false;

struct GitSyncStateEntry {
    String name;
    String localBlobSha;
    String remoteBlobSha;
};

struct GitRemoteDocEntry {
    String name;
    String remotePath;
    String blobSha;
};

enum GitSyncAction {
    GIT_SYNC_SKIP = 0,
    GIT_SYNC_UPLOAD = 1,
    GIT_SYNC_DOWNLOAD = 2,
    GIT_SYNC_DELETE_REMOTE = 3,
};

struct GitSyncPlanEntry {
    String name;
    String remotePath;
    String localContent;
    String localSha;
    String remoteSha;
    String finalContent;
    String finalSha;
    GitSyncAction action = GIT_SYNC_SKIP;
};

const char* choStrs[] = {"\xE3\x84\xB1","\xE3\x84\xB2","\xE3\x84\xB4","\xE3\x84\xB7","\xE3\x84\xB8","\xE3\x84\xB9","\xE3\x85\x81","\xE3\x85\x82","\xE3\x85\x83","\xE3\x85\x85","\xE3\x85\x86","\xE3\x85\x87","\xE3\x85\x88","\xE3\x85\x89","\xE3\x85\x8A","\xE3\x85\x8B","\xE3\x85\x8C","\xE3\x85\x8D","\xE3\x85\x8E"};
const char* jungStrs[] = {"\xE3\x85\x8F","\xE3\x85\x90","\xE3\x85\x91","\xE3\x85\x92","\xE3\x85\x93","\xE3\x85\x94","\xE3\x85\x95","\xE3\x85\x96","\xE3\x85\x97","\xE3\x85\x98","\xE3\x85\x99","\xE3\x85\x9A","\xE3\x85\x9B","\xE3\x85\x9C","\xE3\x85\x9D","\xE3\x85\x9E","\xE3\x85\x9F","\xE3\x85\xA0","\xE3\x85\xA1","\xE3\x85\xA2","\xE3\x85\xA3"};
int cho = -1, jung = -1, jong = -1; char lastJongChar = 0;
#include "jeong_eum.h"
KeyEngineScript getSelectedKeyEngine();
int rightFileIndex;
int oldCursorCx = 5;
int oldCursorCy = 35;
int lastSy = 0; 
String otaPinCode = "";              
bool isOtaPinVerified = false;       
bool isOtaUpdatePending = false;     
enum UpdateState { UPD_NONE, UPD_PIN_INPUT, UPD_WIFI_WAITING, UPD_SD_RUNNING };
UpdateState updateState = UPD_NONE;
String pinInputBuffer = "";          
String webServerPasswordInput = "";
String webServerPasswordHint = "Numbers only / Tab cancels";
String activeApPassword = "0000000000";
bool apHadClient = false;
bool apPasswordHidden = false;
bool apAuthenticatedClientSeen = false;
unsigned long apNoClientSinceMs = 0;
class ScaledDisplay : public Adafruit_GFX {
public:
  InkplateProxy* tft;
  float* scaleRef; 
  ScaledDisplay(InkplateProxy* displayInstance, int16_t w, int16_t h, float* s) 
    : Adafruit_GFX(w, h), tft(displayInstance), scaleRef(s) {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        int px = (int)(x * (*scaleRef));
        int py = (int)(y * (*scaleRef));
        int ps = (int)ceil(*scaleRef);
        for (int dy = 0; dy < ps; dy++)
            for (int dx = 0; dx < ps; dx++)
                tft->writePixelInternal(px + dx, py + dy, color);
    }
};

ScaledDisplay bigDisplay(&display, 780, 600, &displayScale);
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

int getTrueLength(String text) {
  int count = 0;
  for (int i = 0; i < text.length(); ) {
    int l = 1;
    unsigned char c = (unsigned char)text[i];
    if ((c & 0x80) != 0) {
      if ((c & 0xE0) == 0xC0) l = 2;
      else if ((c & 0xF0) == 0xE0) l = 3;
      else if ((c & 0xF8) == 0xF0) l = 4;
      else l = 1;
    }
    if (i + l > text.length()) l = 1;
    i += l; count++;
  }
  return count;
}


String utf8Truncate(const String& text, int maxChars) {
    String out = "";
    int p = 0;
    int count = 0;
    while (p < text.length() && count < maxChars) {
        unsigned char c = (unsigned char)text[p];
        int len = 1;
        if ((c & 0x80) == 0) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        else break;  // Invalid UTF-8 leading byte: do not copy broken preview bytes.
        if (p + len > text.length()) break;  // Avoid copying a partial multibyte char from a 128-byte preview read.
        bool complete = true;
        for (int i = 1; i < len; i++) {
            if (((unsigned char)text[p + i] & 0xC0) != 0x80) { complete = false; break; }
        }
        if (!complete) break;
        out += text.substring(p, p + len);
        p += len;
        count++;
    }
    return out;
}

int utf8PrevStart(const String& text, int pos) {
    if (pos <= 0) return 0;
    int p = pos - 1;
    while (p > 0 && (((unsigned char)text[p] & 0xC0) == 0x80)) p--;
    return p;
}

int utf8NextStart(const String& text, int pos) {
    if (pos >= text.length()) return text.length();
    int n = pos + 1;
    while (n < text.length() && (((unsigned char)text[n] & 0xC0) == 0x80)) n++;
    return n;
}

int utf8ColumnBetween(const String& text, int start, int end) {
    int col = 0;
    int p = start;
    while (p < end && p < text.length()) {
        p = utf8NextStart(text, p);
        col++;
    }
    return col;
}

int utf8OffsetForColumn(const String& text, int start, int end, int column) {
    int p = start;
    int col = 0;
    while (p < end && col < column) {
        p = utf8NextStart(text, p);
        col++;
    }
    return p;
}

void doBackspace() { 
    bool textChanged = false;
    KeyEngineScript activeEngine = getSelectedKeyEngine();
    if (currentMode == SEARCH_MODE) {
        if (keyEngineHandleBackspace(activeEngine)) {
            searchMatchEnd = -1;
        } else if (searchQuery.length() > 0) {
            int p = searchQuery.length() - 1;
            while (p > 0 && (((unsigned char)searchQuery[p] & 0xC0) == 0x80)) p--;
            searchQuery = searchQuery.substring(0, p);
            searchMatchEnd = -1;
        } else {
            keyEngineClearComposition(activeEngine);
            searchMatchEnd = -1;
            currentMode = TYPING_MODE;
        }
    } else if (keyEngineHandleBackspace(activeEngine)) { 
        forceSafeFullTextRedraw = true;
    } else if (cursorPos > 0) { 
      if (isCtrlPressed) { 
            int p = cursorPos - 1;
            
            
            while (p > 0 && (fullText[p] == ' ' || fullText[p] == '\n')) p--;
            
            
            while (p > 0 && fullText[p-1] != ' ' && fullText[p-1] != '\n') p--;
            
            
            fullText = fullText.substring(0, p) + fullText.substring(cursorPos); 
            cursorPos = p;
            textChanged = true;
        }
        else{
            
            int p = cursorPos - 1;
            while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--; 
            fullText = fullText.substring(0, p) + fullText.substring(cursorPos); 
            cursorPos = p;
            textChanged = true;
        }
    } 
    if (textChanged) {
        keyEngineAfterEdit(activeEngine);
        forceSafeFullTextRedraw = true;
    }
    needUpdate = true; 
    statusBarNeedsUpdate = true; 
}

void insertText(String str) { 
    if (currentMode == SEARCH_MODE) {
        searchQuery += str; 
        searchMatchEnd = -1;
    } else {
        bool simpleTailAppend = (cursorPos == fullText.length() && str.indexOf('\n') < 0 && getTrueLength(str) == 1);
        keyEngineClampCursorToUtf8Boundary();
        fullText = fullText.substring(0, cursorPos) + str + fullText.substring(cursorPos); 
        cursorPos += str.length();
        keyEngineClampCursorToUtf8Boundary();
        if (!simpleTailAppend) forceSafeFullTextRedraw = true;
    }
}

void preloadInitialImage() {
    String path = INITIAL_IMAGE_PATH;
    SdFile file;
    if (file.open(path.c_str(), O_RDONLY)) {
        imgSize = file.fileSize();
        if (imgSize > 500 * 1024) {
            file.close();
            return;
        }
        if (imgBuffer) {
            free(imgBuffer);
            imgBuffer = nullptr;
        }
        if (ESP.getFreePsram() > imgSize) {
            imgBuffer = (uint8_t *)ps_malloc(imgSize);
        } else {
            imgBuffer = (uint8_t *)malloc(imgSize);
        }
        if (imgBuffer) file.read(imgBuffer, imgSize);
        file.close();
    }
}

void hardRefresh() {
    
    display.display();
    display.clearDisplay(); 
    display.display();
    
    display.resetInternalCounter(); 
    needUpdate = true; statusBarNeedsUpdate = true; lastSy = -1;
}

int viewBottomIdx = 0; 

// Fast path only for ordinary typing at the document end with no line reflow.
int lastRenderedTextLen = -1;
int lastRenderedCursorPos = -1;
int lastRenderedTailStart = -1;
int lastRenderedTailLineCount = -1;
int lastRenderedTailLineWidth = 0;
bool lastRenderedTailRtl = false;

WrapMetrics getWrappedMetricsInRange(const String& text, int start, int end, int maxWidth) {
    WrapMetrics metrics = {1, 0};
    if (start >= end) return metrics;
    for (int k = start; k < end; ) {
        int l = zwUtf8CharLen(text, k);
        if (l <= 0) l = 1;
        uint32_t cp = zwUtf8Codepoint(text.c_str() + k, l);
        int charWidth = zwGlyphAdvance(cp, l, false);
        if (metrics.lastLineWidth + charWidth > maxWidth && metrics.lastLineWidth > 0) {
            metrics.lineCount++;
            metrics.lastLineWidth = charWidth;
        } else {
            metrics.lastLineWidth += charWidth;
        }
        k += l;
    }
    return metrics;
}

int countWrappedLinesInRange(const String& text, int start, int end, int maxWidth) {
    return getWrappedMetricsInRange(text, start, end, maxWidth).lineCount;
}

void adjustViewBottom() {
    
    if (fullText.length() == 0) { viewBottomIdx = 0; return; }
    if (viewBottomIdx > fullText.length()) viewBottomIdx = fullText.length();
    if (cursorPos > viewBottomIdx) viewBottomIdx = cursorPos;

    
    int dynamicVisibleLines = ((display.height() / displayScale) - (MARGIN_Y * 1.5)) / (baseFontSize + lineSpacing);
    if (dynamicVisibleLines < 1) dynamicVisibleLines = 1;
    float avgCharWidth = baseFontSize * 0.8;
    int dynamicCharsPerLine = ((display.width() / displayScale) - MARGIN_X - RIGHT_EDGE_MARGIN) / avgCharWidth;
    if (dynamicCharsPerLine < 1) dynamicCharsPerLine = 1;
    
    
    int approxLines = 1;
    int charsInLine = 0;
    for (int i = 0; i < fullText.length(); i++) {
        if (fullText[i] == '\n' || charsInLine >= dynamicCharsPerLine) {
            approxLines++;
            charsInLine = 0;
        }
        charsInLine++;
    }
    if (approxLines <= dynamicVisibleLines) {
        viewBottomIdx = fullText.length();
        return;
    }

    
    int lineCount = 0;
    int tempIdx = viewBottomIdx;
    int chars = 0;
    while (tempIdx > cursorPos) {
        tempIdx--;
        chars++;
        if (fullText[tempIdx] == '\n' || chars >= dynamicCharsPerLine) {
            lineCount++;
            chars = 0;
        }
    }
    if (lineCount >= dynamicVisibleLines - 1) {
        int forwardLines = 0;
        int fwdTemp = cursorPos;
        int fwdChars = 0;
        while (fwdTemp < fullText.length() && forwardLines < dynamicVisibleLines - 1) {
            if (fullText[fwdTemp] == '\n' || fwdChars >= dynamicCharsPerLine) {
                forwardLines++;
                fwdChars = 0;
            }
            fwdTemp++;
            fwdChars++;
        }
        viewBottomIdx = fwdTemp;
    }
}

void printDualFont(String text, int x, int y, bool isMenu = false, int maxWidth = 800) {
    if (font_ptr == nullptr) return;
    u8g2_for_adafruit_gfx.setFontMode(0);
    int cx = x;
    for (int i = 0; i < text.length(); ) {
        if (cx > maxWidth) break;
        int l = zwUtf8CharLen(text, i);
        if (l <= 0) break;
        uint32_t cp = zwUtf8Codepoint(text.c_str() + i, l);
        bool drawable = zwIsDrawableCodepoint(cp);
        String c = drawable ? text.substring(i, i + l) : String("?");
        if (!drawable) l = 1;
        const uint8_t* selectedFont = zwFontForCodepoint(cp);
        if (selectedFont != nullptr) u8g2_for_adafruit_gfx.setFont(selectedFont);
        int adv = zwGlyphAdvance(cp, l, isMenu);
        if (zwGlyphVisible(cp)) u8g2_for_adafruit_gfx.drawUTF8(cx + zwGlyphDrawXOffset(cp), y + zwGlyphDrawYOffset(cp), c.c_str());
        cx += adv;
        i += l;
    }
}

static inline bool rtlDigitCodepoint(uint32_t cp) {
    return (cp >= '0' && cp <= '9') ||
           (cp >= 0x0660 && cp <= 0x0669) ||
           (cp >= 0x06F0 && cp <= 0x06F9);
}

String makeRtlVisualText(const String& logicalText) {
    String shaped = (getSelectedKeyEngine() == KEY_ENGINE_ARABIC)
        ? keyEngineShapeArabicRun(logicalText)
        : logicalText;

    String visual = "";
    String cluster = "";
    String digitRun = "";

    auto prependCluster = [&](const String& item) {
        if (item.length() == 0) return;
        int l = zwUtf8CharLen(item, 0);
        uint32_t cp = (l > 0) ? zwUtf8Codepoint(item.c_str(), l) : 0;
        if (rtlDigitCodepoint(cp)) {
            digitRun += item;
        } else {
            if (digitRun.length() > 0) {
                visual = digitRun + visual;
                digitRun = "";
            }
            visual = item + visual;
        }
    };

    for (int i = 0; i < shaped.length(); ) {
        int l = zwUtf8CharLen(shaped, i);
        if (l <= 0) l = 1;
        uint32_t cp = zwUtf8Codepoint(shaped.c_str() + i, l);
        String ch = shaped.substring(i, i + l);

        if (zwGlyphAdvance(cp, l, false) == 0 && cluster.length() > 0) {
            cluster += ch;
        } else {
            prependCluster(cluster);
            cluster = ch;
        }
        i += l;
    }
    prependCluster(cluster);
    if (digitRun.length() > 0) visual = digitRun + visual;
    return visual;
}

void printStatusText(String text, int x, int y) {
    float prevScale = displayScale;
    displayScale = 2.0f;
    printCleanText(u8g2_for_adafruit_gfx, text, x, y, true);
    displayScale = prevScale;
}

String utf8LimitLabel(String label, int maxChars) {
    int count = 0;
    int end = 0;
    for (int i = 0; i < label.length() && count < maxChars; ) {
        int l = zwUtf8CharLen(label, i);
        if (l <= 0) break;
        end = i + l;
        i += l;
        count++;
    }
    if (end >= label.length()) return label;
    return label.substring(0, end) + "..";
}

String getStatusLanguageLabel() {
    String label;
    if (isKoreanMode) label = getKeyboardModeName();
    else label = getEnglishKeyboardModeName();
    if (!isKoreanMode) {
        if (isCapsLockOn) label.toUpperCase();
        else label.toLowerCase();
    }
    if (getTrueLength(label) > 6) label = utf8LimitLabel(label, 4);
    return "[" + label + "]";
}

String getEnglishKeyboardModeName() {
    if (englishLayoutIndex <= 0) return String(KEYBOARD_LAYOUTS[0].name);
    return String(KEYBOARD_LAYOUTS[1].name);
}

String getKeyboardModeName() {
    if (keyboardLayoutIndex < 2 || keyboardLayoutIndex >= KEYBOARD_LAYOUT_TOTAL) keyboardLayoutIndex = 2;
    return String(KEYBOARD_LAYOUTS[keyboardLayoutIndex].name);
}

String getPreviewEnglishKeyboardModeName() {
    if (previewEnglishLayoutIndex <= 0) return String(KEYBOARD_LAYOUTS[0].name);
    return String(KEYBOARD_LAYOUTS[1].name);
}

String getPreviewKeyboardModeName() {
    if (previewKeyboardLayoutIndex < 2 || previewKeyboardLayoutIndex >= KEYBOARD_LAYOUT_TOTAL) previewKeyboardLayoutIndex = 2;
    return String(KEYBOARD_LAYOUTS[previewKeyboardLayoutIndex].name);
}

String getAutoSleepDisplayLabel() {
    if (autoSleepIndex == 0) return "30 SEC";
    if (autoSleepIndex == 1) return "1 MIN";
    if (autoSleepIndex == 2) return "5 MIN";
    if (autoSleepIndex == 3) return "10 MIN";
    if (autoSleepIndex == 4) return "30 MIN";
    if (autoSleepIndex == 5) return "1 HR";
    return "OFF";
}

String menuFitToWidth(String text, int maxWidth) {
    if (zwMeasureTextWidth(text, true) <= maxWidth) return text;
    const String ellipsis = "..";
    int p = text.length();
    while (p > 0) {
        p = utf8PrevStart(text, p);
        String candidate = text.substring(0, p) + ellipsis;
        if (zwMeasureTextWidth(candidate, true) <= maxWidth) return candidate;
    }
    return ellipsis;
}

void printMenuEntry(String text, int x, int y, bool isSelected, bool isRightSide) {
    const int maxTextWidth = isRightSide ? 520 : 185;
    String displayText = (rtlTextMode && isRightSide) ? makeRtlVisualText(text) : text;
    displayText = menuFitToWidth(displayText, maxTextWidth);
    int drawX = (int)(x * displayScale);
    int drawY = (int)(y * displayScale);
    int boxW = isRightSide ? (int)(540 * displayScale) : (int)(190 * displayScale);
    int boxH = (int)((baseFontSize + lineSpacing + 8) * displayScale);
    int textX = (rtlTextMode && isRightSide) ? (x + 520 - zwMeasureTextWidth(displayText, true)) : x;
    if (isSelected) {
        
        display.fillRect(drawX - (int)(4 * displayScale), drawY - (int)((baseFontSize + 2) * displayScale), boxW, boxH, BLACK);
        u8g2_for_adafruit_gfx.setForegroundColor(WHITE); 
        u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);
        printDualFont(displayText, textX, y, true, x + (isRightSide ? 520 : 185));
        u8g2_for_adafruit_gfx.setForegroundColor(BLACK); 
        u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
    } else {
        u8g2_for_adafruit_gfx.setForegroundColor(BLACK); 
        u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
        printDualFont(displayText, textX, y, true, x + (isRightSide ? 520 : 185));
    }
}

void refreshFileList() {
  fileCount = 0;
  SdFile root;
  if (!root.open("/")) return;
  SdFile file;
  
  while (file.openNext(&root, O_RDONLY)) {
    if (!file.isDir() && !file.isHidden()) {
      char name[64];
      file.getName(name, sizeof(name));
      String fn = String(name);
      String fnLower = fn; 
      fnLower.toLowerCase();
      
      if (fnLower.endsWith(".txt") && !fn.startsWith(".") && !fn.startsWith("._")) {
        if (fileCount >= MAX_DOCUMENT_FILES) {
          file.close();
          break;
        }
        fileCount++;
        files[fileCount].name = fn;
        files[fileCount].sizeKB = file.fileSize() / 1024.0; 
        
        char buf[129];
        int n = file.read(buf, 128);
        if (n > 0) buf[n] = '\0'; else buf[0] = '\0';
        String pv = String(buf);
        pv.replace("\r", " ");
        pv.replace("\n", " ");
        files[fileCount].preview = utf8Truncate(pv, 14);
        
        int docNum = docNumberFromName(fn);
        files[fileCount].time = docNum; 
      }
    }
    file.close();
    yield();
  }
  root.close();
  
  
  for (int i = 1; i < fileCount; i++) {
    yield(); 
    for (int j = i + 1; j <= fileCount; j++) {
      if (files[i].time < files[j].time) {
        FileInfo temp = files[i];
        files[i] = files[j];
        files[j] = temp;
      }
    }
  }
}

void saveFile() { 
    SdFile f; 
    if (f.open(currentFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) { 
        f.write(fullText.c_str(), fullText.length()); 
        f.sync(); f.close(); 
    } 
    showSavedMessageTime = millis(); savedMessageVisible = true; needUpdate = true; statusBarNeedsUpdate = true; refreshFileList(); 
}


void moveCursorToLineStart() {
  
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); 
          
          
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
          flushKorean(); 
          int p = cursorPos - 1;
          
          
          while (p > 0 && (fullText[p] == ' ' || fullText[p] == '\n')) p--;
          
          
          while (p > 0 && fullText[p-1] != ' ' && fullText[p-1] != '\n') p--;
          
          cursorPos = (p < 0) ? 0 : p;
          forceSafeFullTextRedraw = true;
          needUpdate = true;    
}

void moveCursorToLineEnd() {
  
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); 
          
          
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    
      flushKorean(); 
      int n = cursorPos;
      
      
      while (n < fullText.length() && (fullText[n] == ' ' || fullText[n] == '\n')) n++;
      
      
      while (n < fullText.length() && fullText[n] != ' ' && fullText[n] != '\n') n++;
      
      cursorPos = n;
      forceSafeFullTextRedraw = true;
      needUpdate = true;
}

void moveCursorToParagraphStart() {
  
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); 
          
          
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    
    cursorPos = 0;
    forceSafeFullTextRedraw = true;
    
    needUpdate = true;
}

void moveCursorToParagraphEnd() {
  
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); 
          
          
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    
    cursorPos = fullText.length();
    forceSafeFullTextRedraw = true;
    needUpdate = true;
}

void selectLeft() { 
    
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); 
          
          
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
      if (cursorPos > 0) { cursorPos = utf8PrevStart(fullText, cursorPos); forceSafeFullTextRedraw = true; }
      needUpdate = true; 
  }

void selectRight() { 
    
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); 
          
          
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
      if (cursorPos < fullText.length()) { cursorPos = utf8NextStart(fullText, cursorPos); forceSafeFullTextRedraw = true; }
      needUpdate = true; 
  }
void selectUp() { 
    
    moveCursorUp(); 
}
void selectDown() { 
    
    moveCursorDown(); 
}

void moveCursorUp() {
    
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); 
          
          
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    
    int lineStart = cursorPos;
    while (lineStart > 0 && fullText[lineStart - 1] != '\n') {
        lineStart--;
    }
    if (lineStart == 0) return; 

    
    int column = utf8ColumnBetween(fullText, lineStart, cursorPos);

    
    int prevLineStart = lineStart - 1;
    while (prevLineStart > 0 && fullText[prevLineStart - 1] != '\n') {
        prevLineStart--;
    }

    
    int prevLineEnd = lineStart - 1;
    cursorPos = utf8OffsetForColumn(fullText, prevLineStart, prevLineEnd, column);
    forceSafeFullTextRedraw = true;
    needUpdate = true;
}

void moveCursorDown() {
    
      if (cho != -1 || jung != -1) {
          String composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
          
          
          fullText = fullText.substring(0, cursorPos) + composing + fullText.substring(cursorPos);
          cursorPos += composing.length(); 
          
          
          cho = -1; 
          jung = -1; 
          jong = -1;
      }
    
    int lineStart = cursorPos;
    while (lineStart > 0 && fullText[lineStart - 1] != '\n') {
        lineStart--;
    }
    int column = utf8ColumnBetween(fullText, lineStart, cursorPos);

    
    int nextLineStart = cursorPos;
    while (nextLineStart < fullText.length() && fullText[nextLineStart] != '\n') {
        nextLineStart++;
    }
    if (nextLineStart >= fullText.length()) return; 
    nextLineStart++; 

    
    int nextLineEnd = nextLineStart;
    while (nextLineEnd < fullText.length() && fullText[nextLineEnd] != '\n') {
        nextLineEnd++;
    }

    
    cursorPos = utf8OffsetForColumn(fullText, nextLineStart, nextLineEnd, column);
    forceSafeFullTextRedraw = true;
    needUpdate = true;
}

int getWordCount(const String& text) {
    int wordCount = 0;
    bool inWord = false;
    
    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        
        
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            inWord = false;
        } 
        
        else if (!inWord) {
            inWord = true;
            wordCount++;
        }
    }
    return wordCount;
}

void loadFile() {
    SdFile f;
    if (f.open(currentFileName.c_str(), O_RDONLY)) {
        fullText = "";
        
        size_t fSize = f.fileSize();
        if (fSize > 0 && fSize < 1024 * 1024) {   
            uint8_t* buf = (uint8_t*)ps_malloc(fSize + 1);
            if (buf) {
                f.read(buf, fSize);
                buf[fSize] = '\0';
                fullText = String((char*)buf);
                free(buf);
            }
        }
        f.close();
        cursorPos = fullText.length();
        forceSafeFullTextRedraw = true;
        sharedCharCount = getTrueLength(fullText);
        sharedWordCount = getWordCount(fullText);
        charwordcount = (countMode == 1) ? sharedWordCount : sharedCharCount;
        calcBuffer = fullText;
        needCountUpdate = true;
        lastCountRequestMs = millis();
        statusBarNeedsUpdate = true;
    }
}

void createNewDoc() { 
    currentFileName = nextDocFilename(); fullText = ""; cursorPos = 0; forceSafeFullTextRedraw = true; 
    saveFile(); currentMode = TYPING_MODE; needUpdate = true; 
}

void CalculationTask(void * pvParameters) {
    for(;;) {
        unsigned long nowMs = millis();
        if (statusBarNeedsUpdate && (!cachedBatteryValid || nowMs - lastBatteryReadMs >= BATTERY_REFRESH_INTERVAL_MS) &&
            !displayIoBusy && currentNetSubMode == NET_MAIN && updateState == UPD_NONE &&
            (currentMode == TYPING_MODE || currentMode == SEARCH_MODE)) {
            float batV = display.readBattery();
            int pct = constrain((int)((batV - 3.3) / (4.2 - 3.3) * 100), 0, 100);
            cachedBatteryPct = pct;
            cachedBatteryValid = true;
            lastBatteryReadMs = nowMs;
        }


        // Do not read keyboard Serial on Core0.
        // The main loop owns all key bytes; sharing Serial here caused lost Ctrl/Menu events
        // and random input interference in network/BLE modes.

        
        if (needCountUpdate) {
            // Main loop never writes calcBuffer while needCountUpdate is true.
            // Count directly from calcBuffer to avoid an extra heap-allocating String copy on Core0.
            calcBufferInUse = true;

            sharedCharCount = getTrueLength(calcBuffer);
            int words = 0;
            bool inWord = false;
            for (int i = 0; i < calcBuffer.length(); i++) {
                if (isspace((unsigned char)calcBuffer[i])) inWord = false;
                else if (!inWord) { inWord = true; words++; }
            }
            sharedWordCount = words;
            needCountUpdate = false;
            calcBufferInUse = false;
        }
        vTaskDelay(5); 
    }
}


spi_flash_mmap_handle_t font_mmap_handle; 


static bool isPsramPtr(const uint8_t* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return addr >= 0x3F800000 && addr <= 0x3FBFFFFF;
}

static const uint8_t* loadFontFileToPSRAM(const char* path) {
    SdFile f;
    if (!f.open(path, O_RDONLY)) {
        return nullptr;
    }

    size_t fSize = f.fileSize();
    if (fSize == 0) {
        f.close();
        return nullptr;
    }

    if (ESP.getFreePsram() < fSize) {
        f.close();
        return nullptr;
    }

    uint8_t* pBuf = (uint8_t*)ps_malloc(fSize);
    if (!pBuf) {
        f.close();
        return nullptr;
    }

    size_t bytesRead = f.read(pBuf, fSize);
    f.close();
    if (bytesRead != fSize) {
        free(pBuf);
        return nullptr;
    }

    const char* jamoPrefix = "font_jamo";
    const char* hangulPrefix = "font_hangul";
    const char* newJamoPrefix = "hwalja_jamo";
    const char* newHangulPrefix = "hwalja_hangul";
    size_t prefixLen = 0;
    if (strcmp(path, "/ize_compose/hwalja/hwalja_jamo.bin") == 0 && fSize > strlen(jamoPrefix) && memcmp(pBuf, jamoPrefix, strlen(jamoPrefix)) == 0) prefixLen = strlen(jamoPrefix);
    if (strcmp(path, "/ize_compose/hwalja/hwalja_hangul.bin") == 0 && fSize > strlen(hangulPrefix) && memcmp(pBuf, hangulPrefix, strlen(hangulPrefix)) == 0) prefixLen = strlen(hangulPrefix);
    if (strcmp(path, "/ize_compose/hwalja/hwalja_jamo.bin") == 0 && fSize > strlen(newJamoPrefix) && memcmp(pBuf, newJamoPrefix, strlen(newJamoPrefix)) == 0) prefixLen = strlen(newJamoPrefix);
    if (strcmp(path, "/ize_compose/hwalja/hwalja_hangul.bin") == 0 && fSize > strlen(newHangulPrefix) && memcmp(pBuf, newHangulPrefix, strlen(newHangulPrefix)) == 0) prefixLen = strlen(newHangulPrefix);
    if (prefixLen > 0) {
        memmove(pBuf, pBuf + prefixLen, fSize - prefixLen);
    }

    return pBuf;
}

static void replaceFontPtr(const uint8_t*& target, const uint8_t* next) {
    if (next == nullptr) return;
    if (target != nullptr && target != next && target != font_ptr && target != font_latin_ptr && target != font_hangul_ptr && target != font_jamo_ptr && target != font_jp_ptr && target != font_greek_cyrillic_ptr && target != font_arabic_ptr && target != font_indic_ptr && target != font_sea_ptr && target != font_misc_ptr && isPsramPtr(target)) free((void*)target);
    target = next;
}

const uint8_t* loadFontToPSRAM(int slot) {
    (void)slot;
    const uint8_t* nextLatin = loadFontFileToPSRAM(LATIN_FONT_PATH);
    const uint8_t* nextHangul = loadFontFileToPSRAM("/ize_compose/hwalja/hwalja_hangul.bin");
    const uint8_t* nextJamo = loadFontFileToPSRAM("/ize_compose/hwalja/hwalja_jamo.bin");
    const uint8_t* nextJp = loadFontFileToPSRAM("/ize_compose/hwalja/hwalja_jp.bin");
    const uint8_t* nextGreekCyrillic = loadFontFileToPSRAM("/ize_compose/hwalja/hwalja_greek_cyrillic.bin");
    const uint8_t* nextArabic  = loadFontFileToPSRAM("/ize_compose/hwalja/hwalja_arabic.bin");
    const uint8_t* nextIndic   = loadFontFileToPSRAM("/ize_compose/hwalja/hwalja_indic.bin");
    const uint8_t* nextSea     = loadFontFileToPSRAM("/ize_compose/hwalja/hwalja_sea.bin");
    const uint8_t* nextMisc    = loadFontFileToPSRAM("/ize_compose/hwalja/hwalja_misc.bin");

    font_latin_ptr = u8g2_font_5x7_tf;
    if (nextLatin) replaceFontPtr(font_latin_ptr, nextLatin);
    font_ptr = font_latin_ptr;
    if (nextHangul) replaceFontPtr(font_hangul_ptr, nextHangul); else font_hangul_ptr = font_latin_ptr;
    if (nextJamo) replaceFontPtr(font_jamo_ptr, nextJamo); else font_jamo_ptr = font_latin_ptr;
    if (nextJp) replaceFontPtr(font_jp_ptr, nextJp); else font_jp_ptr = font_latin_ptr;
    if (nextGreekCyrillic) replaceFontPtr(font_greek_cyrillic_ptr, nextGreekCyrillic); else font_greek_cyrillic_ptr = font_latin_ptr;
    if (nextArabic) replaceFontPtr(font_arabic_ptr, nextArabic); else font_arabic_ptr = font_latin_ptr;
    if (nextIndic) replaceFontPtr(font_indic_ptr, nextIndic); else font_indic_ptr = font_latin_ptr;
    if (nextSea) replaceFontPtr(font_sea_ptr, nextSea); else font_sea_ptr = font_latin_ptr;
    if (nextMisc) replaceFontPtr(font_misc_ptr, nextMisc); else font_misc_ptr = font_latin_ptr;
    return font_ptr;
}

void setup() {
    
    setCpuFrequencyMhz(240);
    Serial.begin(921600); 


    
    psramInit();
    
    display.begin(); 
    pinMode(WAKE_BUTTON_PIN, INPUT);

    loadSystemSettings();
    display.setRotation(0);
    u8g2_for_adafruit_gfx.begin(bigDisplay);
    
    
    u8g2_for_adafruit_gfx.setForegroundColor(BLACK); 
    u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
    u8g2_for_adafruit_gfx.setFont(font_ptr);
    
    if (display.sdCardInit()) { 
        if (ensureIzeComposeDirs()) migrateLegacyIzeComposeFiles();
        migrateLegacyDocFilenames();
        
        
        font_ptr = loadFontToPSRAM(currentFontSlot);
        if (font_ptr != nullptr) {
            u8g2_for_adafruit_gfx.setFont(font_ptr);
        }
        
        preloadInitialImage(); 
        refreshFileList();
        if (fileCount > 0) {
            if (rightFileIndex < 1 || rightFileIndex > fileCount) rightFileIndex = 1;
            currentFileName = files[rightFileIndex].name;
        }
        loadFile();
    }
    
    display.clearDisplay();
    printCleanText(u8g2_for_adafruit_gfx, "Booting...", MARGIN_X, MARGIN_Y);  
    display.partialUpdate();
    
    delay(1000); 
    while(Serial.available() > 0) Serial.read();  
    while(Serial1.available() > 0) Serial1.read(); 

    
    

    cursorPos = fullText.length(); 
    needUpdate = true;

    xTaskCreatePinnedToCore(
        CalculationTask, "CalcTask", 8192, NULL, 1, &CalcTaskHandle, 0
    );
}

void saveSystemSettings() {
    prefs.begin("rupert", false);
    prefs.putBool("cfgInit", true);
    prefs.putBool("isKor", isKoreanMode);
    prefs.putBool("isCaps", isCapsLockOn);
    prefs.putInt("fIndex", rightFileIndex);
    prefs.putInt("latin", latinMode);
    prefs.putInt("count", countMode);
    prefs.putInt("engKbd", englishLayoutIndex);
    prefs.putInt("kbd", keyboardLayoutIndex);
    prefs.putInt("fSlot", currentFontSlot);  
    prefs.putFloat("scale", displayScale);
    prefs.putInt("lineSp", lineSpacing);
    prefs.putInt("letterSp", letterSpacing);
    prefs.putInt("typeSpd", typingSpeed);
    prefs.putInt("refresh", refreshLimit);
    prefs.putInt("sleep", autoSleepIndex);
    prefs.putString("wifiSsid", savedWifiSsid);
    prefs.putString("wifiPass", savedWifiPassword);
    prefs.putString("ghOwner", githubOwner);
    prefs.putString("ghRepo", githubRepo);
    prefs.putString("ghBranch", githubBranch);
    prefs.putString("ghPath", githubPath);
    prefs.putString("ghToken", githubToken);
    prefs.end();
    writeSettingsBackupFile();
}

void loadSystemSettings() {
    prefs.begin("rupert", true);
    bool configInitialized = prefs.getBool("cfgInit", false);
    isKoreanMode = prefs.getBool("isKor", false);
    isCapsLockOn = prefs.getBool("isCaps", false);
    rightFileIndex = prefs.getInt("fIndex", 0);
    latinMode = prefs.getInt("latin", 0);
    countMode = prefs.getInt("count", 2);
    if (countMode < 0 || countMode > 2) countMode = 2;
    englishLayoutIndex = prefs.getInt("engKbd", 1);
    if (englishLayoutIndex < 0 || englishLayoutIndex > 1) englishLayoutIndex = 1;
    keyboardLayoutIndex = prefs.getInt("kbd", 2);
    if (keyboardLayoutIndex < 2 || keyboardLayoutIndex >= KEYBOARD_LAYOUT_TOTAL) keyboardLayoutIndex = 2;
    previewEnglishLayoutIndex = englishLayoutIndex;
    previewKeyboardLayoutIndex = keyboardLayoutIndex;
    currentFontSlot = prefs.getInt("fSlot", 1);  
    displayScale = prefs.getFloat("scale", 2.0f);
    if (displayScale < 0.5f || displayScale > 3.5f) displayScale = 2.0f;
    lineSpacing = prefs.getInt("lineSp", 2);
    if (lineSpacing < 0 || lineSpacing > 30) lineSpacing = 2;
    letterSpacing = prefs.getInt("letterSp", 0);
    if (letterSpacing < -5 || letterSpacing > 10) letterSpacing = 0;
    typingSpeed = prefs.getInt("typeSpd", 0);
    if (typingSpeed < 0 || typingSpeed > 2000) typingSpeed = 0;
    refreshLimit = prefs.getInt("refresh", 2000);
    if (refreshLimit < 0 || refreshLimit > 2000) refreshLimit = 2000;
    autoSleepIndex = prefs.getInt("sleep", 2);
    if (autoSleepIndex < 0 || autoSleepIndex > 6) autoSleepIndex = 2;
    savedWifiSsid = prefs.getString("wifiSsid", "");
    savedWifiPassword = prefs.getString("wifiPass", "");
    githubOwner = prefs.getString("ghOwner", "");
    githubRepo = prefs.getString("ghRepo", "");
    githubBranch = prefs.getString("ghBranch", "main");
    githubPath = prefs.getString("ghPath", "documents");
    githubToken = prefs.getString("ghToken", "");
    prefs.end();
    if (!configInitialized && restoreSettingsFromBackup()) {
        saveSystemSettings();
    }
}

String jsonEscape(const String& input) {
    String out = "";
    out.reserve(input.length() + 8);
    for (int i = 0; i < input.length(); i++) {
        char c = input[i];
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

bool extractJsonIntValue(const String& json, const char* key, int& outValue) {
    String token = "\"" + String(key) + "\":";
    int pos = json.indexOf(token);
    if (pos < 0) return false;
    pos += token.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) pos++;
    int end = pos;
    if (end < json.length() && json[end] == '-') end++;
    while (end < json.length() && isDigit(json[end])) end++;
    if (end <= pos) return false;
    outValue = json.substring(pos, end).toInt();
    return true;
}

bool extractJsonBoolValue(const String& json, const char* key, bool& outValue) {
    String token = "\"" + String(key) + "\":";
    int pos = json.indexOf(token);
    if (pos < 0) return false;
    pos += token.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) pos++;
    if (json.startsWith("true", pos)) {
        outValue = true;
        return true;
    }
    if (json.startsWith("false", pos)) {
        outValue = false;
        return true;
    }
    return false;
}

bool extractJsonStringValue(const String& json, const char* key, String& outValue) {
    String token = "\"" + String(key) + "\":\"";
    int pos = json.indexOf(token);
    if (pos < 0) return false;
    pos += token.length();
    String result = "";
    while (pos < json.length()) {
        char c = json[pos++];
        if (c == '\\' && pos < json.length()) {
            char esc = json[pos++];
            switch (esc) {
                case '\\': result += '\\'; break;
                case '\"': result += '\"'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += esc; break;
            }
        } else if (c == '\"') {
            outValue = result;
            return true;
        } else {
            result += c;
        }
    }
    return false;
}

bool writeSettingsBackupFile() {
    if (!ensureIzeComposeDirs()) return false;
    SdFile file;
    if (!file.open(SETTINGS_BACKUP_PATH, O_WRONLY | O_CREAT | O_TRUNC)) return false;

    String json = "{\n";
    json += "  \"isKoreanMode\": " + String(isKoreanMode ? "true" : "false") + ",\n";
    json += "  \"englishLayoutIndex\": " + String(englishLayoutIndex) + ",\n";
    json += "  \"keyboardLayoutIndex\": " + String(keyboardLayoutIndex) + ",\n";
    json += "  \"displayScaleTenths\": " + String((int)round(displayScale * 10.0f)) + ",\n";
    json += "  \"lineSpacing\": " + String(lineSpacing) + ",\n";
    json += "  \"letterSpacing\": " + String(letterSpacing) + ",\n";
    json += "  \"typingSpeed\": " + String(typingSpeed) + ",\n";
    json += "  \"refreshLimit\": " + String(refreshLimit) + ",\n";
    json += "  \"autoSleepIndex\": " + String(autoSleepIndex) + ",\n";
    json += "  \"countMode\": " + String(countMode) + ",\n";
    json += "  \"currentFontSlot\": " + String(currentFontSlot) + ",\n";
    json += "  \"githubOwner\": \"" + jsonEscape(githubOwner) + "\",\n";
    json += "  \"githubRepo\": \"" + jsonEscape(githubRepo) + "\",\n";
    json += "  \"githubBranch\": \"" + jsonEscape(githubBranch) + "\",\n";
    json += "  \"githubPath\": \"" + jsonEscape(githubPath) + "\",\n";
    json += "  \"githubToken\": \"" + jsonEscape(githubToken) + "\",\n";
    json += "  \"currentFileName\": \"" + jsonEscape(currentFileName) + "\"\n";
    json += "}\n";

    bool ok = file.print(json) > 0;
    file.close();
    return ok;
}

bool restoreSettingsFromBackup() {
    SdFile file;
    if (!file.open(SETTINGS_BACKUP_PATH, O_RDONLY)) return false;

    String json = "";
    while (file.available()) {
        char chunk[256];
        int readLen = file.read(chunk, sizeof(chunk));
        if (readLen <= 0) break;
        for (int i = 0; i < readLen; i++) json += chunk[i];
        yield();
    }
    file.close();
    if (json.length() == 0) return false;

    int intValue = 0;
    bool boolValue = false;
    String stringValue = "";

    if (extractJsonBoolValue(json, "isKoreanMode", boolValue)) isKoreanMode = boolValue;
    if (extractJsonIntValue(json, "englishLayoutIndex", intValue) && intValue >= 0 && intValue <= 1) englishLayoutIndex = intValue;
    if (extractJsonIntValue(json, "keyboardLayoutIndex", intValue) && intValue >= 2 && intValue < KEYBOARD_LAYOUT_TOTAL) keyboardLayoutIndex = intValue;
    if (extractJsonIntValue(json, "displayScaleTenths", intValue) && intValue >= 5 && intValue <= 35) displayScale = intValue / 10.0f;
    if (extractJsonIntValue(json, "lineSpacing", intValue) && intValue >= 0 && intValue <= 30) lineSpacing = intValue;
    if (extractJsonIntValue(json, "letterSpacing", intValue) && intValue >= -5 && intValue <= 10) letterSpacing = intValue;
    if (extractJsonIntValue(json, "typingSpeed", intValue) && intValue >= 0 && intValue <= 2000) typingSpeed = intValue;
    if (extractJsonIntValue(json, "refreshLimit", intValue) && intValue >= 0 && intValue <= 2000) refreshLimit = intValue;
    if (extractJsonIntValue(json, "autoSleepIndex", intValue) && intValue >= 0 && intValue <= 6) autoSleepIndex = intValue;
    if (extractJsonIntValue(json, "countMode", intValue) && intValue >= 0 && intValue <= 2) countMode = intValue;
    if (extractJsonIntValue(json, "currentFontSlot", intValue) && intValue >= 0) currentFontSlot = intValue;
    if (extractJsonStringValue(json, "githubOwner", stringValue)) githubOwner = stringValue;
    if (extractJsonStringValue(json, "githubRepo", stringValue)) githubRepo = stringValue;
    if (extractJsonStringValue(json, "githubBranch", stringValue) && stringValue.length() > 0) githubBranch = stringValue;
    if (extractJsonStringValue(json, "githubPath", stringValue)) githubPath = stringValue;
    if (extractJsonStringValue(json, "githubToken", stringValue) && stringValue.length() > 0) githubToken = stringValue;
    if (extractJsonStringValue(json, "currentFileName", stringValue) && stringValue.length() > 0) currentFileName = stringValue;

    previewEnglishLayoutIndex = englishLayoutIndex;
    previewKeyboardLayoutIndex = keyboardLayoutIndex;
    return true;
}

String getCurrentLanguageMenuLabel() {
    if (!isKoreanMode) return "English";
    return getKeyboardModeName();
}

String getKeyboardLayoutIdString(KeyboardLayoutId id) {
    for (uint8_t i = 0; i < KEYBOARD_LAYOUT_TOTAL; i++) {
        if (KEYBOARD_LAYOUTS[i].id == id) return String(i);
    }
    return "0";
}

bool parseIntArgBounded(const String& value, int minValue, int maxValue, int& outValue) {
    if (value.length() == 0) return false;
    for (int i = 0; i < value.length(); i++) {
        if (!isDigit(value[i]) && !(i == 0 && value[i] == '-')) return false;
    }
    int parsed = value.toInt();
    if (parsed < minValue || parsed > maxValue) return false;
    outValue = parsed;
    return true;
}

String buildSettingsJson() {
    String json = "{";
    json += "\"sleepTimer\":" + String(autoSleepIndex);
    json += ",\"fontScale\":" + String((int)round(displayScale * 10.0f));
    json += ",\"lineSpacing\":" + String(lineSpacing);
    json += ",\"letterSpacing\":" + String(letterSpacing);
    json += ",\"typingSpeed\":" + String(typingSpeed);
    json += ",\"refreshLimit\":" + String(refreshLimit);
    json += ",\"englishLayout\":" + String(englishLayoutIndex);
    json += ",\"language\":\"";
    json += isKoreanMode ? getKeyboardLayoutIdString(KEYBOARD_LAYOUTS[keyboardLayoutIndex].id) : "english";
    json += "\"";
    json += ",\"languages\":[";
    json += "{\"value\":\"english\",\"label\":\"English\"}";
    for (uint8_t i = 2; i < KEYBOARD_LAYOUT_TOTAL; i++) {
        json += ",{\"value\":\"";
        json += getKeyboardLayoutIdString(KEYBOARD_LAYOUTS[i].id);
        json += "\",\"label\":\"";
        json += jsonEscape(String(KEYBOARD_LAYOUTS[i].name));
        json += "\"}";
    }
    json += "]}";
    return json;
}

void handleSettingsJson() {
    if (!documentAccessAllowed()) return;
    server.send(200, "application/json; charset=utf-8", buildSettingsJson());
}

void handleSettingsSave() {
    String pin = server.arg("pin");
    if (pin.length() != 4 || pin != otaPinCode) {
        server.send(403, "text/plain", "Invalid PIN.");
        return;
    }

    int newSleepIndex = autoSleepIndex;
    int newFontScale = (int)(displayScale * 10.0f);
    int newLineSpacing = lineSpacing;
    int newLetterSpacing = letterSpacing;
    int newTypingSpeed = typingSpeed;
    int newRefreshLimit = refreshLimit;
    int newEnglishLayoutIndex = englishLayoutIndex;
    int newKeyboardLayoutIndex = keyboardLayoutIndex;
    bool newIsKoreanMode = isKoreanMode;

    if (!parseIntArgBounded(server.arg("sleepTimer"), 0, 6, newSleepIndex) ||
        !parseIntArgBounded(server.arg("fontScale"), 5, 35, newFontScale) ||
        !parseIntArgBounded(server.arg("lineSpacing"), 0, 30, newLineSpacing) ||
        !parseIntArgBounded(server.arg("letterSpacing"), -5, 10, newLetterSpacing) ||
        !parseIntArgBounded(server.arg("typingSpeed"), 0, 2000, newTypingSpeed) ||
        !parseIntArgBounded(server.arg("refreshLimit"), 0, 2000, newRefreshLimit) ||
        !parseIntArgBounded(server.arg("englishLayout"), 0, 1, newEnglishLayoutIndex)) {
        server.send(400, "text/plain", "Invalid settings value.");
        return;
    }

    String language = server.arg("language");
    if (language == "english") {
        newIsKoreanMode = false;
    } else {
        newIsKoreanMode = true;
        bool foundLanguage = false;
        for (int i = 2; i < KEYBOARD_LAYOUT_TOTAL; i++) {
            if (getKeyboardLayoutIdString(KEYBOARD_LAYOUTS[i].id) == language) {
                newKeyboardLayoutIndex = i;
                foundLanguage = true;
                break;
            }
        }
        if (!foundLanguage) {
            server.send(400, "text/plain", "Invalid language selection.");
            return;
        }
    }

    bool changed =
        newSleepIndex != autoSleepIndex ||
        newFontScale != (int)(displayScale * 10.0f) ||
        newLineSpacing != lineSpacing ||
        newLetterSpacing != letterSpacing ||
        newTypingSpeed != typingSpeed ||
        newRefreshLimit != refreshLimit ||
        newEnglishLayoutIndex != englishLayoutIndex ||
        newKeyboardLayoutIndex != keyboardLayoutIndex ||
        newIsKoreanMode != isKoreanMode;

    if (!changed) {
        server.send(200, "text/plain", "No changes detected.");
        return;
    }

    autoSleepIndex = newSleepIndex;
    displayScale = newFontScale / 10.0f;
    lineSpacing = newLineSpacing;
    letterSpacing = newLetterSpacing;
    typingSpeed = newTypingSpeed;
    refreshLimit = newRefreshLimit;
    englishLayoutIndex = newEnglishLayoutIndex;
    previewEnglishLayoutIndex = englishLayoutIndex;
    keyboardLayoutIndex = newKeyboardLayoutIndex;
    previewKeyboardLayoutIndex = keyboardLayoutIndex;
    isKoreanMode = newIsKoreanMode;
    isCapsLockOn = false;
    saveSystemSettings();
    forceSafeFullTextRedraw = true;
    needUpdate = true;
    statusBarNeedsUpdate = true;

    server.send(200, "text/plain", "Settings saved.");
}

String buildGithubSettingsJson() {
    String json = "{";
    json += "\"available\":" + String(currentNetSubMode == NET_WIFI_STA ? "true" : "false");
    json += ",\"configured\":" + String(githubConfigComplete() ? "true" : "false");
    json += ",\"tokenSaved\":" + String(githubToken.length() > 0 ? "true" : "false");
    json += ",\"owner\":\"" + jsonEscape(githubOwner) + "\"";
    json += ",\"repo\":\"" + jsonEscape(githubRepo) + "\"";
    json += ",\"branch\":\"" + jsonEscape(githubBranchName()) + "\"";
    json += ",\"path\":\"" + jsonEscape(githubCleanPath(githubPath)) + "\"";
    json += ",\"syncStatus\":\"" + jsonEscape(githubSyncStatusMessage) + "\"";
    json += "}";
    return json;
}

void handleGithubSettingsJson() {
    if (currentNetSubMode != NET_WIFI_STA || !documentAccessAllowed()) {
        if (!server.client().connected()) return;
        if (currentNetSubMode != NET_WIFI_STA) server.send(403, "application/json", "{\"available\":false}");
        return;
    }
    server.send(200, "application/json; charset=utf-8", buildGithubSettingsJson());
}

void handleGithubSettingsSave() {
    if (currentNetSubMode != NET_WIFI_STA || !documentAccessAllowed()) {
        if (!server.client().connected()) return;
        if (currentNetSubMode != NET_WIFI_STA) server.send(403, "text/plain", "GitHub settings are available only in Wi-Fi mode.");
        return;
    }

    String newOwner = server.arg("owner"); newOwner.trim();
    String newRepo = server.arg("repo"); newRepo.trim();
    String newBranch = server.arg("branch"); newBranch.trim();
    String newPath = server.arg("path"); newPath = githubCleanPath(newPath);
    String newToken = server.arg("token"); newToken.trim();

    if (newOwner.length() == 0 || newRepo.length() == 0) {
        server.send(400, "text/plain", "Owner and repository are required.");
        return;
    }
    if (newBranch.length() == 0) newBranch = "main";
    if (githubToken.length() == 0 && newToken.length() == 0) {
        server.send(400, "text/plain", "Token is required.");
        return;
    }

    bool changed = newOwner != githubOwner || newRepo != githubRepo || newBranch != githubBranch || newPath != githubCleanPath(githubPath) || newToken.length() > 0;
    if (!changed) {
        server.send(200, "text/plain", "No changes detected.");
        return;
    }

    githubOwner = newOwner;
    githubRepo = newRepo;
    githubBranch = newBranch;
    githubPath = newPath;
    if (newToken.length() > 0) githubToken = newToken;
    githubSyncStatusMessage = "GitHub settings saved";
    saveSystemSettings();
    server.send(200, "text/plain", "GitHub settings saved.");
}
SdFile sdBackupFile;
String uploadTargetPath = "";
bool uploadAccepted = false;
bool uploadIsFirmware = false;
bool uploadIsInitialImage = false;
bool pendingSdUpdate = false;
int uploadHttpStatus = 200;
String uploadHttpMessage = "OK";
uint8_t uploadHeader[8];
int uploadHeaderLen = 0;

bool sdPathExists(const char* path) {
    SdFile f;
    if (!f.open(path, O_RDONLY)) return false;
    f.close();
    return true;
}

bool removeSdFile(const char* path) {
    SdFile f;
    if (!f.open(path, O_RDONLY)) return true;
    bool ok = f.remove();
    if (f.isOpen()) f.close();
    return ok;
}

bool ensureDirExists(const char* path) {
    SdFile existing;
    if (existing.open(path, O_RDONLY)) {
        bool ok = existing.isDir();
        existing.close();
        return ok;
    }

    SdFile root;
    SdFile created;
    if (!root.open("/", O_RDONLY) || !root.isDir()) {
        if (root.isOpen()) root.close();
        return false;
    }
    bool ok = created.mkdir(&root, path, true);
    if (created.isOpen()) created.close();
    root.close();
    return ok;
}

bool ensureIzeComposeDirs() {
    if (!ensureDirExists(APP_ROOT_DIR)) return false;
    if (!ensureDirExists(FONT_DIR)) return false;
    if (!ensureDirExists(FIRMWARE_DIR)) return false;
    return true;
}

bool copySdFileIfMissing(const char* fromPath, const char* toPath) {
    if (sdPathExists(toPath) || !sdPathExists(fromPath)) return true;
    SdFile src;
    SdFile dst;
    if (!src.open(fromPath, O_RDONLY)) return false;
    if (!dst.open(toPath, O_WRONLY | O_CREAT | O_TRUNC)) {
        src.close();
        return false;
    }
    uint8_t buf[512];
    int n = 0;
    bool ok = true;
    while ((n = src.read(buf, sizeof(buf))) > 0) {
        if (dst.write(buf, n) != n) {
            ok = false;
            break;
        }
        yield();
    }
    src.close();
    dst.close();
    if (!ok) removeSdFile(toPath);
    return ok;
}

bool moveSdFileWithFallback(const String& fromPath, const String& toPath) {
    SdFile src;
    if (!src.open(fromPath.c_str(), O_RDWR)) return false;
    if (src.rename(toPath.c_str())) {
        src.close();
        return true;
    }
    src.close();

    SdFile inFile;
    SdFile outFile;
    if (!inFile.open(fromPath.c_str(), O_RDONLY)) return false;
    if (!outFile.open(toPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) {
        inFile.close();
        return false;
    }

    uint8_t buf[512];
    bool ok = true;
    while (inFile.available()) {
        int n = inFile.read(buf, sizeof(buf));
        if (n <= 0) break;
        if (outFile.write(buf, n) != n) {
            ok = false;
            break;
        }
        yield();
    }
    outFile.sync();
    inFile.close();
    outFile.close();
    if (!ok) {
        removeSdFile(toPath.c_str());
        return false;
    }

    SdFile oldFile;
    if (!oldFile.open(fromPath.c_str(), O_RDWR)) {
        removeSdFile(toPath.c_str());
        return false;
    }
    bool removed = oldFile.remove();
    oldFile.close();
    if (!removed) {
        removeSdFile(toPath.c_str());
        return false;
    }
    return true;
}

void migrateLegacyIzeComposeFiles() {
    copySdFileIfMissing("/backup/initial.png", INITIAL_IMAGE_PATH);
    copySdFileIfMissing("/backup/font_latin.bin", LATIN_FONT_PATH);
    copySdFileIfMissing("/backup/hwalja_latin.bin", LATIN_FONT_PATH);
    copySdFileIfMissing("/backup/font_hangul.bin", "/ize_compose/hwalja/hwalja_hangul.bin");
    copySdFileIfMissing("/backup/font_jamo.bin", "/ize_compose/hwalja/hwalja_jamo.bin");
    copySdFileIfMissing("/backup/font_jp.bin", "/ize_compose/hwalja/hwalja_jp.bin");
    copySdFileIfMissing("/backup/font_greek_cyrillic.bin", "/ize_compose/hwalja/hwalja_greek_cyrillic.bin");
    copySdFileIfMissing("/backup/font_arabic.bin", "/ize_compose/hwalja/hwalja_arabic.bin");
    copySdFileIfMissing("/backup/font_indic.bin", "/ize_compose/hwalja/hwalja_indic.bin");
    copySdFileIfMissing("/backup/font_sea.bin", "/ize_compose/hwalja/hwalja_sea.bin");
    copySdFileIfMissing("/backup/font_misc.bin", "/ize_compose/hwalja/hwalja_misc.bin");
}

int highestDocNumberOnSd() {
    SdFile root;
    SdFile file;
    char name[64];
    int maxNum = 0;
    if (!root.open("/", O_RDONLY)) return 0;
    while (file.openNext(&root, O_RDONLY)) {
        if (!file.isDir()) {
            file.getName(name, sizeof(name));
            int n = docNumberFromName(String(name));
            if (n > maxNum) maxNum = n;
        }
        file.close();
        yield();
    }
    root.close();
    return maxNum;
}

void migrateGitSyncStateDocNames() {
    SdFile file;
    if (!file.open(GITHUB_SYNC_STATE_PATH, O_RDONLY)) return;

    String lines[80];
    int count = 0;
    String line = "";
    while (file.available() && count < 80) {
        char c = (char)file.read();
        if (c == '\r') continue;
        if (c != '\n') {
            line += c;
            continue;
        }
        if (line.length() > 0) lines[count++] = line;
        line = "";
    }
    if (line.length() > 0 && count < 80) lines[count++] = line;
    file.close();

    if (!file.open(GITHUB_SYNC_STATE_PATH, O_WRONLY | O_CREAT | O_TRUNC)) return;
    for (int i = 0; i < count; i++) {
        String row = lines[i];
        int tab1 = row.indexOf('\t');
        int tab2 = tab1 >= 0 ? row.indexOf('\t', tab1 + 1) : -1;
        if (tab1 <= 0 || tab2 <= tab1) continue;
        String name = row.substring(0, tab1);
        if (isDocFilename(name)) name = canonicalDocFilename(name);
        String out = name + row.substring(tab1) + "\n";
        file.print(out);
    }
    file.close();
}

void migrateLegacyDocFilenames() {
    SdFile root;
    SdFile file;
    char name[64];
    String legacyNames[80];
    int legacyCount = 0;
    int maxNum = highestDocNumberOnSd();
    if (!root.open("/", O_RDONLY)) return;

    while (file.openNext(&root, O_RDONLY) && legacyCount < 80) {
        if (!file.isDir()) {
            file.getName(name, sizeof(name));
            String fn = String(name);
            if (isLegacyDocFilename(fn)) legacyNames[legacyCount++] = fn;
        }
        file.close();
        yield();
    }
    root.close();

    bool migratedAny = false;
    for (int i = 0; i < legacyCount; i++) {
        String fromName = legacyNames[i];
        String toName = canonicalDocFilename(fromName);
        if (toName == fromName) continue;
        if (sdPathExists(toName.c_str())) {
            do {
                maxNum++;
                toName = formatDocFilename(maxNum);
            } while (sdPathExists(toName.c_str()));
        }

        if (!moveSdFileWithFallback(fromName, toName)) continue;
        migratedAny = true;
        if (currentFileName == fromName) currentFileName = toName;
    }

    if (isLegacyDocFilename(currentFileName)) currentFileName = canonicalDocFilename(currentFileName);
    if (migratedAny) {
        migrateGitSyncStateDocNames();
        writeSettingsBackupFile();
    }
}

void handleWebServerUpdate() {
    server.sendHeader("Connection", "close");
    server.send(uploadHttpStatus, "text/plain", uploadHttpMessage);
    if (pendingSdUpdate) {
        delay(200);
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        currentNetSubMode = NET_MAIN;
        isUpdating = false;
        updateScreenDrawn = false;
        updateState = UPD_SD_RUNNING;
        pendingSdUpdate = false;
        needUpdate = true;
    }
}

bool normalizeFontUploadTarget(String filename, String& normalizedName) {
    filename.toLowerCase();
    struct FontAlias { const char* suffix; const char* target; };
    const FontAlias aliases[] = {
        {"latin.bin", "hwalja_latin.bin"},
        {"hangul.bin", "hwalja_hangul.bin"},
        {"jamo.bin", "hwalja_jamo.bin"},
        {"jp.bin", "hwalja_jp.bin"},
        {"greek_cyrillic.bin", "hwalja_greek_cyrillic.bin"},
        {"arabic.bin", "hwalja_arabic.bin"},
        {"indic.bin", "hwalja_indic.bin"},
        {"sea.bin", "hwalja_sea.bin"},
        {"misc.bin", "hwalja_misc.bin"},
    };

    for (const auto& alias : aliases) {
        String target = String(alias.target);
        String underscoredSuffix = String("_") + alias.suffix;
        if (filename == target || filename.endsWith(underscoredSuffix)) {
            normalizedName = target;
            return true;
        }
    }
    return false;
}

bool isUploadedPng() {
    const uint8_t pngSig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (uploadHeaderLen < 8) return false;
    for (int i = 0; i < 8; i++) {
        if (uploadHeader[i] != pngSig[i]) return false;
    }
    return true;
}

void handleWebServerUpload() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
String clientPin = server.header("X-OTA-PIN");
        if (otaPinCode != "" && clientPin != otaPinCode) {
            uploadAccepted = false;
            uploadHttpStatus = 403;
            uploadHttpMessage = "Invalid PIN";
            return;
        }

        String filename = upload.filename;
        String lowerFilename = filename;
        lowerFilename.toLowerCase();
        String normalizedFontName = "";
        uploadTargetPath = "";
        uploadAccepted = false;
        uploadIsFirmware = false;
        uploadIsInitialImage = false;
        pendingSdUpdate = false;
        uploadHttpStatus = 400;
        uploadHttpMessage = "Unsupported file";
        uploadHeaderLen = 0;
        isUpdating = true;
        if (CalcTaskHandle != NULL) vTaskSuspend(CalcTaskHandle);

        if (lowerFilename == "izefirmware.bin") {
            if (!ensureIzeComposeDirs()) {
                uploadHttpStatus = 500;
                uploadHttpMessage = "Could not create ize_compose folders";
            } else {
                uploadTargetPath = FIRMWARE_UPDATE_PATH;
                uploadAccepted = true;
                uploadIsFirmware = true;
                uploadHttpStatus = 200;
                uploadHttpMessage = "Firmware saved. Updating...";
            }
        } else if (lowerFilename.endsWith(".png") || normalizeFontUploadTarget(lowerFilename, normalizedFontName)) {
            if (!ensureIzeComposeDirs()) {
                uploadHttpStatus = 500;
                uploadHttpMessage = "Could not create ize_compose folders";
            } else {
                uploadTargetPath = lowerFilename.endsWith(".png")
                    ? String(INITIAL_IMAGE_PATH)
                    : (String(FONT_DIR) + "/" + normalizedFontName);
                uploadAccepted = true;
                uploadIsInitialImage = lowerFilename.endsWith(".png");
                uploadHttpStatus = 200;
                uploadHttpMessage = "File saved to SD";
            }
        }

        if (uploadAccepted) {
            if (!sdBackupFile.open(uploadTargetPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) {
                uploadAccepted = false;
                uploadIsFirmware = false;
                uploadIsInitialImage = false;
                pendingSdUpdate = false;
                uploadHttpStatus = 500;
                uploadHttpMessage = "Could not create target file";
                isUpdating = false;
                updateScreenDrawn = false;
                if (CalcTaskHandle != NULL) vTaskResume(CalcTaskHandle);
            }
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (!uploadAccepted) return;
        if (uploadIsInitialImage && uploadHeaderLen < 8) {
            int copyLen = min((int)upload.currentSize, 8 - uploadHeaderLen);
            for (int i = 0; i < copyLen; i++) uploadHeader[uploadHeaderLen + i] = upload.buf[i];
            uploadHeaderLen += copyLen;
        }
        if (sdBackupFile.isOpen()) sdBackupFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!uploadAccepted) {
            isUpdating = false;
            updateScreenDrawn = false;
            if (CalcTaskHandle != NULL) vTaskResume(CalcTaskHandle);
            return;
        }

        if (sdBackupFile.isOpen()) sdBackupFile.close();

        if (uploadIsInitialImage && !isUploadedPng()) {
            removeSdFile(uploadTargetPath.c_str());
            isUpdating = false;
            updateScreenDrawn = false;
            uploadHttpStatus = 400;
            uploadHttpMessage = "initial.png is not a PNG file";
            if (CalcTaskHandle != NULL) vTaskResume(CalcTaskHandle);
            return;
        }

        if (uploadIsFirmware) {
            pendingSdUpdate = true;
        } else {
            if (uploadIsInitialImage) {
                preloadInitialImage();
            } else {
                loadFontToPSRAM(currentFontSlot);
                if (font_ptr) u8g2_for_adafruit_gfx.setFont(font_ptr);
            }
            isUpdating = false;
            updateScreenDrawn = false;
        }

        if (CalcTaskHandle != NULL) vTaskResume(CalcTaskHandle);
    }
}

void generateWebPin() {
    uint32_t n = esp_random() % 10000;
    otaPinCode = String(n);
    while (otaPinCode.length() < 4) otaPinCode = "0" + otaPinCode;
    webDocumentUnlocked = false;
    apHadClient = false;
    apPasswordHidden = false;
    apAuthenticatedClientSeen = false;
    apNoClientSinceMs = 0;
}

void stopNetworkServices() {
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    resetStatusScreenCache();
    currentNetSubMode = NET_MAIN;
    tempNetCursor = NET_MAIN;
    updateState = UPD_NONE;
    webDocumentUnlocked = false;
    apHadClient = false;
    apPasswordHidden = false;
    apAuthenticatedClientSeen = false;
    apNoClientSinceMs = 0;
}




bool parseHttpsUrl(const String& url, String& host, String& path) {
    String prefix = "https://";
    if (!url.startsWith(prefix)) return false;
    int hostStart = prefix.length();
    int slash = url.indexOf('/', hostStart);
    if (slash < 0) {
        host = url.substring(hostStart);
        path = "/";
    } else {
        host = url.substring(hostStart, slash);
        path = url.substring(slash);
    }
    return host.length() > 0;
}

bool httpsRequestUrl(const String& method, const String& url, const String& body, bool useGithubToken, const String& accept, int& httpCode, String& response) {
    String host, path;
    if (!parseHttpsUrl(url, host, path)) {
        httpCode = -1;
        response = "Invalid HTTPS URL";
        return false;
    }
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(30000);
    if (!client.connect(host.c_str(), 443)) {
        httpCode = -1;
        response = "HTTPS connect failed";
        return false;
    }
    client.print(method);
    client.print(" ");
    client.print(path);
    client.print(" HTTP/1.1\r\nHost: ");
    client.print(host);
    client.print("\r\nUser-Agent: Ize-Compose\r\nAccept: ");
    client.print(accept.length() ? accept : "*/*");
    client.print("\r\nConnection: close\r\n");
    if (useGithubToken && githubToken.length() > 0) {
        client.print("Authorization: Bearer ");
        client.print(githubToken);
        client.print("\r\n");
    }
    if (method == "POST" || method == "PATCH") {
        client.print("Content-Type: application/json\r\nContent-Length: ");
        client.print(body.length());
        client.print("\r\n");
    }
    client.print("\r\n");
    if (method == "POST" || method == "PATCH") client.print(body);

    unsigned long startMs = millis();
    while (!client.available() && client.connected() && millis() - startMs < 30000) { delay(10); yield(); }
    String statusLine = client.readStringUntil('\n');
    statusLine.trim();
    int firstSpace = statusLine.indexOf(' ');
    httpCode = (firstSpace >= 0) ? statusLine.substring(firstSpace + 1, firstSpace + 4).toInt() : -1;
    bool chunked = false;
    while (client.connected() || client.available()) {
        String header = client.readStringUntil('\n');
        header.trim();
        if (header.length() == 0) break;
        String lower = header; lower.toLowerCase();
        if (lower.indexOf("transfer-encoding:") == 0 && lower.indexOf("chunked") >= 0) chunked = true;
        yield();
    }
    response = "";
    if (chunked) {
        while (client.connected() || client.available()) {
            String lenLine = client.readStringUntil('\n');
            lenLine.trim();
            if (lenLine.length() == 0) continue;
            int chunkLen = (int)strtol(lenLine.c_str(), nullptr, 16);
            if (chunkLen <= 0) break;
            while (chunkLen-- > 0 && (client.connected() || client.available())) {
                while (!client.available() && client.connected()) { delay(1); yield(); }
                if (client.available()) response += (char)client.read();
            }
            if (client.available()) client.read();
            if (client.available()) client.read();
            yield();
        }
    } else {
        while (client.connected() || client.available()) {
            while (client.available()) response += (char)client.read();
            yield();
        }
    }
    client.stop();
    return httpCode >= 200 && httpCode < 300;
}

bool httpsDownloadToSd(const String& url, const String& targetPath, String& errorMessage, bool useGithubToken = false, int redirectDepth = 0) {
    String host, path;
    if (!parseHttpsUrl(url, host, path)) {
        errorMessage = "Invalid download URL";
        return false;
    }
    removeSdFile(targetPath.c_str());
    SdFile out;
    if (!out.open(targetPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) {
        errorMessage = "Cannot create " + targetPath;
        return false;
    }
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(30000);
    if (!client.connect(host.c_str(), 443)) {
        out.close();
        removeSdFile(targetPath.c_str());
        errorMessage = "Download connect failed";
        return false;
    }
    client.print("GET ");
    client.print(path);
    client.print(" HTTP/1.1\r\nHost: ");
    client.print(host);
    client.print("\r\nUser-Agent: Ize-Compose\r\nAccept: application/octet-stream\r\n");
    if (useGithubToken && githubToken.length() > 0) { client.print("Authorization: Bearer "); client.print(githubToken); client.print("\r\n"); }
    client.print("Connection: close\r\n\r\n");
    unsigned long startMs = millis();
    while (!client.available() && client.connected() && millis() - startMs < 30000) { delay(10); yield(); }
    String statusLine = client.readStringUntil('\n');
    statusLine.trim();
    int firstSpace = statusLine.indexOf(' ');
    int httpCode = (firstSpace >= 0) ? statusLine.substring(firstSpace + 1, firstSpace + 4).toInt() : -1;
    bool chunked = false;
    while (client.connected() || client.available()) {
        String header = client.readStringUntil('\n');
        header.trim();
        if (header.length() == 0) break;
        String lower = header; lower.toLowerCase();
        if (lower.indexOf("transfer-encoding:") == 0 && lower.indexOf("chunked") >= 0) chunked = true;
        yield();
    }
    if (httpCode < 200 || httpCode >= 300) {
        out.close();
        removeSdFile(targetPath.c_str());
        errorMessage = "Download HTTP " + String(httpCode);
        return false;
    }
    uint8_t buffer[512];
    bool ok = true;
    if (chunked) {
        while (client.connected() || client.available()) {
            String lenLine = client.readStringUntil('\n');
            lenLine.trim();
            if (lenLine.length() == 0) continue;
            int chunkLen = (int)strtol(lenLine.c_str(), nullptr, 16);
            if (chunkLen <= 0) break;
            while (chunkLen > 0) {
                int want = min(chunkLen, (int)sizeof(buffer));
                int got = 0;
                while (got < want && (client.connected() || client.available())) {
                    if (client.available()) got += client.read(buffer + got, want - got);
                    else { delay(1); yield(); }
                }
                if (got <= 0 || out.write(buffer, got) != got) { ok = false; break; }
                chunkLen -= got;
                yield();
            }
            if (!ok) break;
            if (client.available()) client.read();
            if (client.available()) client.read();
        }
    } else {
        while (client.connected() || client.available()) {
            int n = client.read(buffer, sizeof(buffer));
            if (n > 0 && out.write(buffer, n) != n) { ok = false; break; }
            yield();
        }
    }
    out.close();
    client.stop();
    if (!ok) {
        removeSdFile(targetPath.c_str());
        errorMessage = "Download write failed";
        return false;
    }
    return true;
}


bool latestReleaseInfo(String& latestTag, String& firmwareUrl, String& webUrl, String& webName, String& errorMessage) {
    int code = 0;
    String response;
    if (!httpsRequestUrl("GET", OFFICIAL_RELEASE_API, "", false, "application/vnd.github+json", code, response)) {
        errorMessage = "Release check failed " + String(code);
        return false;
    }
    if (!extractJsonStringValue(response, "tag_name", latestTag)) latestTag = "";
    firmwareUrl = "";
    webUrl = "";
    webName = "";
    int pos = 0;
    while (true) {
        int namePos = response.indexOf("\"name\":\"", pos);
        if (namePos < 0) break;
        namePos += 8;
        int nameEnd = response.indexOf('"', namePos);
        if (nameEnd < 0) break;
        String name = response.substring(namePos, nameEnd);
        int urlPos = response.indexOf("\"browser_download_url\":\"", nameEnd);
        if (urlPos < 0) break;
        urlPos += 24;
        int urlEnd = response.indexOf('"', urlPos);
        if (urlEnd < 0) break;
        String url = response.substring(urlPos, urlEnd);
        if ((name == "izefirmware.bin" || name.indexOf("izefirmware") >= 0) && firmwareUrl.length() == 0) firmwareUrl = url;
        if (name.startsWith("ize_compose_") && name.endsWith(".html")) { webUrl = url; webName = name; }
        pos = urlEnd + 1;
    }
    return latestTag.length() > 0;
}

void handleReleaseStatus() {
    if (currentNetSubMode != NET_WIFI_STA || !documentAccessAllowed()) {
        server.send(403, "application/json", "{\"error\":\"WiFi mode and PIN required\"}");
        return;
    }
    String latest, fwUrl, webUrl, webName, err;
    if (!latestReleaseInfo(latest, fwUrl, webUrl, webName, err)) {
        server.send(500, "application/json", "{\"error\":\"" + jsonEscape(err) + "\"}");
        return;
    }
    bool firmwareAvailable = latest.length() > 0 && latest != String(FIRMWARE_VERSION);
    bool webAvailable = webName.length() > 0 && webName != String("ize_compose_") + String(WEB_PAGE_VERSION) + ".html";
    String json = "{\"current\":\"" + String(FIRMWARE_VERSION) + "\",\"latest\":\"" + jsonEscape(latest) + "\",\"available\":" + String((firmwareAvailable || webAvailable) ? "true" : "false") + ",\"webAsset\":\"" + jsonEscape(webName) + "\",\"webUpdate\":" + String(webAvailable ? "true" : "false") + "}";
    server.send(200, "application/json; charset=utf-8", json);
}

void handleReleaseUpdate() {
    if (currentNetSubMode != NET_WIFI_STA || !documentAccessAllowed()) {
        server.send(403, "text/plain", "WiFi mode and PIN required.");
        return;
    }
    String latest, fwUrl, webUrl, webName, err;
    if (!latestReleaseInfo(latest, fwUrl, webUrl, webName, err)) {
        server.send(500, "text/plain", err);
        return;
    }
    bool firmwareAvailable = latest.length() > 0 && latest != String(FIRMWARE_VERSION);
    bool webAvailable = webName.length() > 0 && webName != String("ize_compose_") + String(WEB_PAGE_VERSION) + ".html";
    if (!firmwareAvailable && !webAvailable) {
        server.send(200, "text/plain", "Current firmware and SD web page are latest.");
        return;
    }
    isUpdating = true;
    drawStatusScreenFrame("Update", "Downloading update files.", "Do not close browser or power off.", true);
    updateScreenDrawn = true;
    if (CalcTaskHandle != NULL) vTaskSuspend(CalcTaskHandle);
    if (firmwareAvailable) {
        if (fwUrl.length() == 0) { isUpdating = false; if (CalcTaskHandle != NULL) vTaskResume(CalcTaskHandle); server.send(500, "text/plain", "Firmware asset missing in release."); return; }
        if (!ensureIzeComposeDirs() || !httpsDownloadToSd(fwUrl, FIRMWARE_UPDATE_PATH, err)) {
            isUpdating = false; if (CalcTaskHandle != NULL) vTaskResume(CalcTaskHandle); server.send(500, "text/plain", err); return;
        }
    }
    if (webAvailable) {
        if (webUrl.length() == 0) { isUpdating = false; if (CalcTaskHandle != NULL) vTaskResume(CalcTaskHandle); server.send(500, "text/plain", "Web page asset missing in release."); return; }
        String target = String(APP_ROOT_DIR) + "/" + webName;
        if (!ensureIzeComposeDirs() || !httpsDownloadToSd(webUrl, target, err)) {
            isUpdating = false; if (CalcTaskHandle != NULL) vTaskResume(CalcTaskHandle); server.send(500, "text/plain", err); return;
        }
    }
    server.send(200, "text/plain", "Downloaded release assets. Updating...");
    if (firmwareAvailable) {
        delay(200);
        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_OFF);
        currentNetSubMode = NET_MAIN;
        updateState = UPD_SD_RUNNING;
        pendingSdUpdate = false;
        needUpdate = true;
    } else {
        isUpdating = false;
        if (CalcTaskHandle != NULL) vTaskResume(CalcTaskHandle);
    }
}
void registerWebRoutes() {
    const char* otaHeaderKeys[] = {"X-OTA-PIN"};
    server.collectHeaders(otaHeaderKeys, 1);

    server.on("/", handleRoot);
    server.on("/auth", HTTP_POST, handleWebAuth);
    server.on("/documents", HTTP_GET, handleDocumentsList);
    server.on("/read", handleRead);
    server.on("/download", handleDownload);
    server.on("/delete", handleDelete);
    server.on("/uploadText", HTTP_POST, handleTextUploadComplete, handleTextUpload);
    server.on("/github/settings.json", HTTP_GET, handleGithubSettingsJson);
    server.on("/github/settings", HTTP_POST, handleGithubSettingsSave);
    server.on("/release/status", HTTP_GET, handleReleaseStatus);
    server.on("/release/update", HTTP_POST, handleReleaseUpdate);

    server.on("/settings.json", HTTP_GET, handleSettingsJson);
    server.on("/settings", HTTP_POST, handleSettingsSave);
    server.on("/update", HTTP_POST, handleWebServerUpdate, handleWebServerUpload);
}

void setupWiFi() {
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP); 
    delay(200);

    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, gateway, subnet);

    WiFi.softAP(ssid, activeApPassword.c_str());

    if (MDNS.begin("izecompose")) {
        MDNS.addService("http", "tcp", 80); 
    }
    WiFi.setTxPower(WIFI_POWER_15dBm);
    if (otaPinCode == "") generateWebPin();
    apHadClient = false;
    apPasswordHidden = false;
    apAuthenticatedClientSeen = false;
    apNoClientSinceMs = 0;
    registerWebRoutes();
    server.begin();
    
} 

void drawOnlineSyncScreen(const String& title, const String& line1, const String& line2 = "");
void finishOnlineSyncToMenu(const String& title, const String& detail);
bool runGithubDocumentSync(String& resultMessage);
bool connectSelectedWifi(const String& password);

void startWifiScanMode(bool forOnlineSync = false) {
    stopNetworkServices();
    wifiConnectForOnlineSync = forOnlineSync;
    currentMode = WIFI_SCAN_MODE;
    wifiStatusMessage = "Scanning Wi-Fi...";
    wifiScanCount = 0;
    wifiScanSelected = 0;
    needUpdate = true;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    delay(200);
    int found = WiFi.scanNetworks(false, true);
    wifiScanCount = 0;
    for (int i = 0; i < found && wifiScanCount < WIFI_SCAN_MAX; i++) {
        String ssidName = WiFi.SSID(i);
        if (ssidName.length() == 0) continue;
        bool duplicate = false;
        for (int j = 0; j < wifiScanCount; j++) {
            if (wifiScanSsids[j] == ssidName) { duplicate = true; break; }
        }
        if (duplicate) continue;
        wifiScanSsids[wifiScanCount] = ssidName;
        wifiScanRssi[wifiScanCount] = WiFi.RSSI(i);
        wifiScanCount++;
    }
    WiFi.scanDelete();
    wifiStatusMessage = (wifiScanCount > 0) ? "Select Wi-Fi. Enter: connect. Tab/Menu: cancel." : "No Wi-Fi networks found. Menu: cancel.";

    if (wifiConnectForOnlineSync) {
        if (savedWifiSsid.length() == 0 || savedWifiPassword.length() == 0) {
            wifiConnectForOnlineSync = false;
            wifiStatusMessage = "No saved Wi-Fi. Select Wi-Fi.";
        } else {
            int savedIndex = -1;
            for (int i = 0; i < wifiScanCount; i++) {
                if (wifiScanSsids[i] == savedWifiSsid) {
                    savedIndex = i;
                    break;
                }
            }
            if (savedIndex >= 0) {
                wifiScanSelected = savedIndex;
                connectSelectedWifi(savedWifiPassword);
                return;
            }
            wifiConnectForOnlineSync = false;
            wifiStatusMessage = "Saved Wi-Fi not found. Select Wi-Fi.";
        }
    }
    needUpdate = true;
}

bool connectSelectedWifi(const String& password) {
    if (wifiScanSelected < 0 || wifiScanSelected >= wifiScanCount) return false;
    String targetSsid = wifiScanSsids[wifiScanSelected];
    wifiStatusMessage = "Connecting to " + targetSsid + "...";
    needUpdate = true;
    drawStatusScreenFrame("WiFi", wifiStatusMessage, "", true);

    WiFi.mode(WIFI_STA);
    WiFi.begin(targetSsid.c_str(), password.c_str());
    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < 5000) {
        delay(250);
        yield();
    }
    if (WiFi.status() == WL_CONNECTED) {
        savedWifiSsid = targetSsid;
        savedWifiPassword = password;
        saveSystemSettings();
        wifiStaIp = WiFi.localIP();
        if (wifiConnectForOnlineSync) {
            String message;
            wifiConnectForOnlineSync = false;
            currentNetSubMode = NET_WIFI_STA;
            githubSyncStatusMessage = "GitHub syncing";
            drawOnlineSyncScreen("GitHub Sync", "Syncing documents...", wifiStaIp.toString());
            bool ok = runGithubDocumentSync(message);
            githubSyncStatusMessage = ok ? "GitHub sync complete" : "GitHub sync failed";
            finishOnlineSyncToMenu(ok ? "Online Sync Complete" : "Online Sync Failed", message);
            return ok;
        }
        generateWebPin();
        currentNetSubMode = NET_WIFI_STA;
        currentMode = TYPING_MODE;
        registerWebRoutes();
        server.begin();
        wifiStatusMessage = "Connected.";
        needUpdate = true;
        statusBarNeedsUpdate = false;
        return true;
    }
    WiFi.disconnect(false, false);
    if (wifiConnectForOnlineSync) {
        wifiConnectForOnlineSync = false;
        wifiPasswordBuffer = "";
        currentMode = WIFI_SCAN_MODE;
        wifiStatusMessage = "Saved Wi-Fi failed. Select Wi-Fi.";
        needUpdate = true;
        return false;
    }
    wifiPasswordBuffer = "";
    currentMode = WIFI_PASSWORD_MODE;
    wifiStatusMessage = "Connection failed. Enter password.";
    needUpdate = true;
    return false;
}

bool githubConfigComplete() {
    String owner = githubOwner; owner.trim();
    String repo = githubRepo; repo.trim();
    String branch = githubBranch; branch.trim();
    String token = githubToken; token.trim();
    return owner.length() > 0 && repo.length() > 0 && branch.length() > 0 && token.length() > 0;
}

String githubCleanPath(String path) {
    path.trim();
    path.replace("\\", "/");
    while (path.startsWith("/")) path.remove(0, 1);
    while (path.endsWith("/")) path.remove(path.length() - 1);
    return path;
}

String githubBranchName() {
    String branch = githubBranch;
    branch.trim();
    if (branch.startsWith("refs/heads/")) branch.remove(0, 11);
    if (branch.length() == 0) branch = "main";
    return branch;
}

String githubPathEncode(const String& value, bool keepSlash) {
    const char* hex = "0123456789ABCDEF";
    String out = "";
    for (int i = 0; i < value.length(); i++) {
        uint8_t c = (uint8_t)value[i];
        bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~' || (keepSlash && c == '/');
        if (safe) out += (char)c;
        else {
            out += '%';
            out += hex[(c >> 4) & 0x0F];
            out += hex[c & 0x0F];
        }
    }
    return out;
}

String githubApiBase() {
    return "https://api.github.com/repos/" + githubPathEncode(githubOwner, false) + "/" + githubPathEncode(githubRepo, false);
}

#if IZE_ENABLE_DIRECT_GITHUB_SYNC
String hexFromBytes(const uint8_t* data, size_t len) {
    const char* hex = "0123456789abcdef";
    String out = "";
    out.reserve(len * 2);
    for (size_t i = 0; i < len; i++) {
        out += hex[(data[i] >> 4) & 0x0F];
        out += hex[data[i] & 0x0F];
    }
    return out;
}

String githubBlobShaForContent(const String& content) {
    mbedtls_sha1_context ctx;
    mbedtls_sha1_init(&ctx);
    mbedtls_sha1_starts_ret(&ctx);
    String header = "blob " + String(content.length()) + "\0";
    mbedtls_sha1_update_ret(&ctx, (const unsigned char*)header.c_str(), header.length());
    mbedtls_sha1_update_ret(&ctx, (const unsigned char*)content.c_str(), content.length());
    uint8_t digest[20];
    mbedtls_sha1_finish_ret(&ctx, digest);
    mbedtls_sha1_free(&ctx);
    return hexFromBytes(digest, sizeof(digest));
}

uint64_t fatDateTimeToStamp(uint16_t date, uint16_t time) {
    if (date == 0) return 0;
    uint16_t year = 1980 + ((date >> 9) & 0x7F);
    uint8_t month = (date >> 5) & 0x0F;
    uint8_t day = date & 0x1F;
    uint8_t hour = (time >> 11) & 0x1F;
    uint8_t minute = (time >> 5) & 0x3F;
    uint8_t second = (time & 0x1F) * 2;
    if (month == 0 || day == 0) return 0;
    return ((uint64_t)year * 10000000000ULL) +
           ((uint64_t)month * 100000000ULL) +
           ((uint64_t)day * 1000000ULL) +
           ((uint64_t)hour * 10000ULL) +
           ((uint64_t)minute * 100ULL) +
           (uint64_t)second;
}

bool parseIsoDateStamp(const String& iso, uint64_t& outStamp) {
    if (iso.length() < 19) return false;
    for (int idx : {0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18}) {
        if (!isDigit(iso[idx])) return false;
    }
    int year = iso.substring(0, 4).toInt();
    int month = iso.substring(5, 7).toInt();
    int day = iso.substring(8, 10).toInt();
    int hour = iso.substring(11, 13).toInt();
    int minute = iso.substring(14, 16).toInt();
    int second = iso.substring(17, 19).toInt();
    outStamp = ((uint64_t)year * 10000000000ULL) +
               ((uint64_t)month * 100000000ULL) +
               ((uint64_t)day * 1000000ULL) +
               ((uint64_t)hour * 10000ULL) +
               ((uint64_t)minute * 100ULL) +
               (uint64_t)second;
    return true;
}

bool getLocalDocModifyStamp(const String& filename, uint64_t& outStamp) {
    SdFile file;
    uint16_t date = 0;
    uint16_t time = 0;
    if (!file.open(filename.c_str(), O_RDONLY)) return false;
    bool ok = file.getModifyDateTime(&date, &time);
    file.close();
    outStamp = ok ? fatDateTimeToStamp(date, time) : 0;
    return ok && outStamp > 0;
}

bool writeDocTextToSd(const String& filename, const String& content, String& errorMessage) {
    SdFile file;
    if (!file.open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) {
        errorMessage = "Could not write " + filename;
        return false;
    }
    bool ok = file.write(content.c_str(), content.length()) == (int)content.length();
    file.sync();
    file.close();
    if (!ok) {
        errorMessage = "Write failed for " + filename;
        return false;
    }
    if (filename == currentFileName) {
        fullText = content;
        cursorPos = min(cursorPos, (int)fullText.length());
        forceSafeFullTextRedraw = true;
    }
    return true;
}

bool readSdTextFile(const char* path, String& content) {
    SdFile file;
    if (!file.open(path, O_RDONLY)) return false;
    content = "";
    char buffer[256];
    while (file.available()) {
        int n = file.read(buffer, sizeof(buffer));
        if (n <= 0) break;
        for (int i = 0; i < n; i++) content += buffer[i];
        yield();
    }
    file.close();
    return true;
}

bool writeSdTextFile(const char* path, const String& content) {
    if (!ensureIzeComposeDirs()) return false;
    SdFile file;
    if (!file.open(path, O_WRONLY | O_CREAT | O_TRUNC)) return false;
    bool ok = file.write(content.c_str(), content.length()) == (int)content.length();
    file.sync();
    file.close();
    return ok;
}

bool csvHasDeletedEntry(const String& csv, const String& filename) {
    int start = 0;
    while (start < csv.length()) {
        int end = csv.indexOf('\n', start);
        if (end < 0) end = csv.length();
        String line = csv.substring(start, end);
        line.trim();
        if (line.length() > 0) {
            int comma = line.indexOf(',');
            String name = (comma >= 0) ? line.substring(0, comma) : line;
            name.trim();
            if (name == filename) return true;
        }
        start = end + 1;
    }
    return false;
}

bool appendDeletedTombstone(const String& filename) {
    if (!isDocFilename(filename)) return false;
    String csv = "";
    readSdTextFile(LOCAL_DELETED_CSV_PATH, csv);
    if (csvHasDeletedEntry(csv, filename)) return true;
    String line = filename + "," + String(millis()) + "\n";
    csv += line;
    return writeSdTextFile(LOCAL_DELETED_CSV_PATH, csv);
}

String generateDeleteConfirmCode() {
    char code[7];
    snprintf(code, sizeof(code), "%06lu", (unsigned long)(esp_random() % 1000000UL));
    return String(code);
}

void cancelDeleteCodePrompt() {
    deleteCodePromptActive = false;
    deleteConfirmCode = "";
    deleteConfirmInput = "";
    pendingDeleteFilename = "";
}

void startDeleteCodePrompt() {
    if (rightFileIndex < 1 || rightFileIndex > fileCount) return;
    pendingDeleteFilename = files[rightFileIndex].name;
    deleteConfirmCode = generateDeleteConfirmCode();
    deleteConfirmInput = "";
    deleteCodePromptActive = true;
}

bool deletePendingFileFromMenu() {
    if (!isDocFilename(pendingDeleteFilename)) return false;
    SdFile root;
    bool removed = false;
    if (root.open("/", O_RDONLY)) {
        SdFile f;
        if (f.open(&root, pendingDeleteFilename.c_str(), O_WRONLY)) {
            removed = f.remove();
            if (removed) appendDeletedTombstone(pendingDeleteFilename);
        }
        root.close();
    }
    if (!removed) return false;
    refreshFileList();
    if (fileCount == 0) createNewDoc();
    if (rightFileIndex < 1) rightFileIndex = 1;
    if (rightFileIndex > fileCount) rightFileIndex = fileCount;
    int maxFileOffsetAfterDelete = fileCount - FILE_MENU_ITEMS_PER_PAGE;
    if (maxFileOffsetAfterDelete < 0) maxFileOffsetAfterDelete = 0;
    if (fileScrollOffset > maxFileOffsetAfterDelete) fileScrollOffset = maxFileOffsetAfterDelete;
    if (fileScrollOffset < 0) fileScrollOffset = 0;
    return true;
}

void drawDeleteCodePopup() {
    if (!deleteCodePromptActive) return;
    const int boxX = 220;
    const int boxY = 165;
    const int boxW = 360;
    const int boxH = 185;
    display.fillRect((int)(boxX * displayScale), (int)(boxY * displayScale), (int)(boxW * displayScale), (int)(boxH * displayScale), WHITE);
    display.drawRect((int)(boxX * displayScale), (int)(boxY * displayScale), (int)(boxW * displayScale), (int)(boxH * displayScale), BLACK);
    display.drawRect((int)((boxX + 3) * displayScale), (int)((boxY + 3) * displayScale), (int)((boxW - 6) * displayScale), (int)((boxH - 6) * displayScale), BLACK);
    u8g2_for_adafruit_gfx.setForegroundColor(BLACK);
    u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
    printCleanText(u8g2_for_adafruit_gfx, "DELETE FILE", boxX + 18, boxY + 30, true);
    printCleanText(u8g2_for_adafruit_gfx, utf8Truncate(pendingDeleteFilename, 28), boxX + 18, boxY + 62, true);
    printCleanText(u8g2_for_adafruit_gfx, "Code: " + deleteConfirmCode, boxX + 18, boxY + 96, true);
    printCleanText(u8g2_for_adafruit_gfx, "Input: " + deleteConfirmInput + "_", boxX + 18, boxY + 126, true);
    printCleanText(u8g2_for_adafruit_gfx, "Enter: delete / Tab: cancel", boxX + 18, boxY + 158, true);
}

void applyDeletedCsvToLocal(const String& csv) {
    int start = 0;
    while (start < csv.length()) {
        int end = csv.indexOf('\n', start);
        if (end < 0) end = csv.length();
        String line = csv.substring(start, end);
        line.trim();
        if (line.length() > 0) {
            int comma = line.indexOf(',');
            String name = (comma >= 0) ? line.substring(0, comma) : line;
            name.trim();
            if (isDocFilename(name)) {
                removeSdFile(name.c_str());
                if (currentFileName == name) currentFileName = nextDocFilename();
            }
        }
        start = end + 1;
    }
}

int findGitSyncStateEntry(GitSyncStateEntry* entries, int count, const String& name) {
    for (int i = 0; i < count; i++) {
        if (entries[i].name == name) return i;
    }
    return -1;
}

int loadGitSyncState(GitSyncStateEntry* entries, int maxEntries) {
    SdFile file;
    if (!file.open(GITHUB_SYNC_STATE_PATH, O_RDONLY)) return 0;
    String line = "";
    int count = 0;
    while (file.available() && count < maxEntries) {
        char c = (char)file.read();
        if (c == '\r') continue;
        if (c != '\n') {
            line += c;
            continue;
        }
        if (line.length() > 0) {
            int tab1 = line.indexOf('\t');
            int tab2 = tab1 >= 0 ? line.indexOf('\t', tab1 + 1) : -1;
            if (tab1 > 0 && tab2 > tab1) {
                entries[count].name = line.substring(0, tab1);
                if (isDocFilename(entries[count].name)) entries[count].name = canonicalDocFilename(entries[count].name);
                entries[count].localBlobSha = line.substring(tab1 + 1, tab2);
                entries[count].remoteBlobSha = line.substring(tab2 + 1);
                count++;
            }
        }
        line = "";
    }
    if (line.length() > 0 && count < maxEntries) {
        int tab1 = line.indexOf('\t');
        int tab2 = tab1 >= 0 ? line.indexOf('\t', tab1 + 1) : -1;
        if (tab1 > 0 && tab2 > tab1) {
            entries[count].name = line.substring(0, tab1);
            if (isDocFilename(entries[count].name)) entries[count].name = canonicalDocFilename(entries[count].name);
            entries[count].localBlobSha = line.substring(tab1 + 1, tab2);
            entries[count].remoteBlobSha = line.substring(tab2 + 1);
            count++;
        }
    }
    file.close();
    return count;
}

bool saveGitSyncState(GitSyncStateEntry* entries, int count) {
    if (!ensureIzeComposeDirs()) return false;
    SdFile file;
    if (!file.open(GITHUB_SYNC_STATE_PATH, O_WRONLY | O_CREAT | O_TRUNC)) return false;
    for (int i = 0; i < count; i++) {
        String line = entries[i].name + "\t" + entries[i].localBlobSha + "\t" + entries[i].remoteBlobSha + "\n";
        if (file.print(line) <= 0) {
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}

void upsertGitSyncState(GitSyncStateEntry* entries, int& count, int maxEntries, const String& name, const String& localBlobSha, const String& remoteBlobSha) {
    int idx = findGitSyncStateEntry(entries, count, name);
    if (idx < 0) {
        if (count >= maxEntries) return;
        idx = count++;
        entries[idx].name = name;
    }
    entries[idx].localBlobSha = localBlobSha;
    entries[idx].remoteBlobSha = remoteBlobSha;
}

bool githubHttpRequest(const String& method, const String& url, const String& body, int& httpCode, String& response) {
    const char* host = "api.github.com";
    String prefix = "https://api.github.com";
    if (!url.startsWith(prefix)) {
        httpCode = -1;
        response = "Invalid GitHub URL";
        return false;
    }
    String path = url.substring(prefix.length());
    if (path.length() == 0) path = "/";

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(25000);
    if (!client.connect(host, 443)) {
        httpCode = -1;
        response = "GitHub connect failed";
        return false;
    }

    client.print(method);
    client.print(" ");
    client.print(path);
    client.print(" HTTP/1.1\r\nHost: api.github.com\r\nUser-Agent: Ize-Compose\r\nAccept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\nAuthorization: Bearer ");
    client.print(githubToken);
    client.print("\r\nConnection: close\r\n");
    if (method == "POST" || method == "PATCH") {
        client.print("Content-Type: application/json\r\nContent-Length: ");
        client.print(body.length());
        client.print("\r\n");
    }
    client.print("\r\n");
    if (method == "POST" || method == "PATCH") client.print(body);

    unsigned long startMs = millis();
    while (!client.available() && client.connected() && millis() - startMs < 25000) {
        delay(10);
        yield();
    }
    String statusLine = client.readStringUntil('\n');
    statusLine.trim();
    int firstSpace = statusLine.indexOf(' ');
    httpCode = (firstSpace >= 0) ? statusLine.substring(firstSpace + 1, firstSpace + 4).toInt() : -1;

    bool chunked = false;
    String location = "";
    while (client.connected() || client.available()) {
        String header = client.readStringUntil('\n');
        header.trim();
        if (header.length() == 0) break;
        String lower = header;
        lower.toLowerCase();
        if (lower.indexOf("transfer-encoding:") == 0 && lower.indexOf("chunked") >= 0) chunked = true;
        if (lower.indexOf("location:") == 0) { location = header.substring(9); location.trim(); }
        yield();
    }

    response = "";
    if (chunked) {
        while (client.connected() || client.available()) {
            String lenLine = client.readStringUntil('\n');
            lenLine.trim();
            if (lenLine.length() == 0) continue;
            int chunkLen = (int)strtol(lenLine.c_str(), nullptr, 16);
            if (chunkLen <= 0) break;
            while (chunkLen-- > 0 && (client.connected() || client.available())) {
                while (!client.available() && client.connected()) { delay(1); yield(); }
                if (client.available()) response += (char)client.read();
            }
            if (client.available()) client.read();
            if (client.available()) client.read();
            yield();
        }
    } else {
        while (client.connected() || client.available()) {
            while (client.available()) response += (char)client.read();
            yield();
        }
    }
    client.stop();
    return httpCode >= 200 && httpCode < 300;
}
bool extractJsonStringValueAfter(const String& json, const String& marker, const char* key, String& outValue) {
    int start = json.indexOf(marker);
    if (start < 0) return false;
    String tail = json.substring(start);
    return extractJsonStringValue(tail, key, outValue);
}

bool githubReadDocText(const String& filename, String& content, String& errorMessage) {
    SdFile file;
    if (!file.open(filename.c_str(), O_RDONLY)) {
        errorMessage = "Could not open " + filename;
        return false;
    }
    uint32_t size = file.fileSize();
    if (!content.reserve(size + 16)) {
        file.close();
        errorMessage = "Not enough memory for " + filename;
        return false;
    }
    content = "";
    char buffer[256];
    while (file.available()) {
        int n = file.read(buffer, sizeof(buffer));
        if (n <= 0) break;
        for (int i = 0; i < n; i++) content += buffer[i];
        yield();
    }
    file.close();
    return true;
}

bool githubCreateBlobForDoc(const String& filename, String& blobSha, String& errorMessage) {
    String content;
    if (!githubReadDocText(filename, content, errorMessage)) return false;
    String escaped = jsonEscape(content);
    String body = "{\"content\":\"" + escaped + "\",\"encoding\":\"utf-8\"}";
    int code = 0;
    String response;
    if (!githubHttpRequest("POST", githubApiBase() + "/git/blobs", body, code, response)) {
        errorMessage = "GitHub blob failed " + String(code) + " for " + filename;
        return false;
    }
    if (!extractJsonStringValue(response, "sha", blobSha) || blobSha.length() == 0) {
        errorMessage = "GitHub blob SHA missing";
        return false;
    }
    return true;
}

bool githubCreateBlobForContent(const String& content, String& blobSha, String& errorMessage) {
    String escaped = jsonEscape(content);
    String body = "{\"content\":\"" + escaped + "\",\"encoding\":\"utf-8\"}";
    int code = 0;
    String response;
    if (!githubHttpRequest("POST", githubApiBase() + "/git/blobs", body, code, response)) {
        errorMessage = "GitHub blob failed " + String(code);
        return false;
    }
    if (!extractJsonStringValue(response, "sha", blobSha) || blobSha.length() == 0) {
        errorMessage = "GitHub blob SHA missing";
        return false;
    }
    return true;
}

bool githubDocNameFromRemotePath(const String& remotePath, String& docName) {
    String base = githubCleanPath(githubPath);
    if (base.length() > 0) {
        String prefix = base + "/";
        if (!remotePath.startsWith(prefix)) return false;
        docName = remotePath.substring(prefix.length());
    } else {
        docName = remotePath;
    }
    if (docName.indexOf('/') >= 0) return false;
    if (!isDocFilename(docName)) return false;
    docName = canonicalDocFilename(docName);
    return true;
}

int parseGithubTreeDocEntries(const String& json, GitRemoteDocEntry* entries, int maxEntries) {
    int count = 0;
    int pos = 0;
    while (count < maxEntries) {
        int pathPos = json.indexOf("\"path\":\"", pos);
        if (pathPos < 0) break;
        pathPos += 8;
        int pathEnd = json.indexOf('\"', pathPos);
        if (pathEnd < 0) break;
        String remotePath = json.substring(pathPos, pathEnd);

        int typePos = json.indexOf("\"type\":\"", pathEnd);
        if (typePos < 0) break;
        typePos += 8;
        int typeEnd = json.indexOf('\"', typePos);
        if (typeEnd < 0) break;
        String type = json.substring(typePos, typeEnd);

        int shaPos = json.indexOf("\"sha\":\"", typeEnd);
        if (shaPos < 0) break;
        shaPos += 7;
        int shaEnd = json.indexOf('\"', shaPos);
        if (shaEnd < 0) break;
        String sha = json.substring(shaPos, shaEnd);

        String name = "";
        if (type == "blob" && githubDocNameFromRemotePath(remotePath, name)) {
            entries[count].name = name;
            entries[count].remotePath = remotePath;
            entries[count].blobSha = sha;
            count++;
        }
        pos = shaEnd + 1;
    }
    return count;
}

int findRemoteDocEntry(GitRemoteDocEntry* entries, int count, const String& name) {
    for (int i = 0; i < count; i++) {
        if (entries[i].name == name) return i;
    }
    return -1;
}

bool githubFetchRemoteCommitStamp(const String& remotePath, uint64_t& stamp, String& errorMessage) {
    int code = 0;
    String response;
    String url = githubApiBase() + "/commits?sha=" + githubPathEncode(githubBranchName(), false) +
                 "&path=" + githubPathEncode(remotePath, true) + "&per_page=1";
    if (!githubHttpRequest("GET", url, "", code, response)) {
        errorMessage = "GitHub commit read failed " + String(code);
        String detail = githubErrorDetail(response);
        if (detail.length() > 0) errorMessage += ": " + detail;
        return false;
    }
    String dateValue;
    if (!extractJsonStringValue(response, "date", dateValue) || !parseIsoDateStamp(dateValue, stamp)) {
        errorMessage = "GitHub commit date missing for " + remotePath;
        return false;
    }
    return true;
}

bool githubFetchRemoteFileContent(const String& remotePath, String& content, String& remoteBlobSha, String& errorMessage) {
    int code = 0;
    String response;
    String url = githubApiBase() + "/contents/" + githubPathEncode(remotePath, true) + "?ref=" + githubPathEncode(githubBranchName(), false);
    if (!githubHttpRequest("GET", url, "", code, response)) {
        errorMessage = "GitHub file read failed " + String(code) + " for " + remotePath;
        String detail = githubErrorDetail(response);
        if (detail.length() > 0) errorMessage += ": " + detail;
        return false;
    }
    String encoded;
    if (!extractJsonStringValue(response, "sha", remoteBlobSha) || !extractJsonStringValue(response, "content", encoded)) {
        errorMessage = "GitHub file payload missing for " + remotePath;
        return false;
    }
    encoded.replace("\n", "");
    encoded.replace("\r", "");
    size_t outLen = 0;
    int rc = mbedtls_base64_decode(nullptr, 0, &outLen, (const unsigned char*)encoded.c_str(), encoded.length());
    if (rc != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL && rc != 0) {
        errorMessage = "Base64 size decode failed for " + remotePath;
        return false;
    }
    unsigned char* buffer = (unsigned char*)malloc(outLen + 1);
    if (!buffer) {
        errorMessage = "Not enough memory for " + remotePath;
        return false;
    }
    rc = mbedtls_base64_decode(buffer, outLen, &outLen, (const unsigned char*)encoded.c_str(), encoded.length());
    if (rc != 0) {
        free(buffer);
        errorMessage = "Base64 decode failed for " + remotePath;
        return false;
    }
    buffer[outLen] = '\0';
    content = String((const char*)buffer);
    free(buffer);
    return true;
}

int collectSyncDocs(String* docNames, int maxDocs) {
    SdFile root;
    SdFile file;
    char name[64];
    static int docNums[80];
    int total = 0;
    if (!root.open("/", O_RDONLY)) return 0;
    while (file.openNext(&root, O_RDONLY)) {
        file.getName(name, sizeof(name));
        String fn = String(name);
        if (!file.isDir() && isDocFilename(fn) && total < maxDocs) {
            docNames[total] = fn;
            docNums[total] = docNumberFromName(fn);
            total++;
        }
        file.close();
        yield();
    }
    root.close();
    for (int i = 0; i < total - 1; i++) {
        for (int j = i + 1; j < total; j++) {
            if (docNums[i] > docNums[j]) {
                int tn = docNums[i]; docNums[i] = docNums[j]; docNums[j] = tn;
                String tf = docNames[i]; docNames[i] = docNames[j]; docNames[j] = tf;
            }
        }
    }
    return total;
}

String githubRemoteDocPath(const String& filename) {
    String base = githubCleanPath(githubPath);
    if (base.length() == 0) return filename;
    return base + "/" + filename;
}

bool runGithubDocumentSync(String& resultMessage) {
    if (!githubConfigComplete()) {
        resultMessage = "GitHub settings missing";
        return false;
    }
    if (WiFi.status() != WL_CONNECTED) {
        resultMessage = "Wi-Fi not connected";
        return false;
    }

    static const int MAX_SYNC_DOCS = 65;
    static String docNames[MAX_SYNC_DOCS];
    static String finalDocNames[MAX_SYNC_DOCS];
    static String docPreviews[MAX_SYNC_DOCS];
    static int docChars[MAX_SYNC_DOCS];
    static String uploadDocNames[MAX_SYNC_DOCS];
    static String uploadBlobShas[MAX_SYNC_DOCS];
    static String deleteRemotePaths[MAX_SYNC_DOCS];
    static GitRemoteDocEntry remoteDocs[MAX_SYNC_DOCS + 1];
    static GitSyncStateEntry syncState[MAX_SYNC_DOCS + 4];
    static GitSyncPlanEntry syncPlan[MAX_SYNC_DOCS];
    int docCount = collectSyncDocs(docNames, MAX_SYNC_DOCS);

    String branch = githubBranchName();
    int code = 0;
    String response;
    String headCommitSha;
    String baseTreeSha;
    String newTreeSha;
    String newCommitSha;

    String getRefUrl = githubApiBase() + "/git/ref/heads/" + githubPathEncode(branch, true);
    String updateRefUrl = githubApiBase() + "/git/refs/heads/" + githubPathEncode(branch, true);
    if (!githubHttpRequest("GET", getRefUrl, "", code, response) || !extractJsonStringValue(response, "sha", headCommitSha)) {
        resultMessage = "GitHub branch read failed " + String(code);
        String detail = githubErrorDetail(response);
        if (detail.length() > 0) resultMessage += ": " + detail;
        return false;
    }

    if (!githubHttpRequest("GET", githubApiBase() + "/git/commits/" + headCommitSha, "", code, response) || !extractJsonStringValueAfter(response, "\"tree\"", "sha", baseTreeSha)) {
        resultMessage = "GitHub tree read failed " + String(code);
        String detail = githubErrorDetail(response);
        if (detail.length() > 0) resultMessage += ": " + detail;
        return false;
    }

    String treeResponse;
    if (!githubHttpRequest("GET", githubApiBase() + "/git/trees/" + baseTreeSha + "?recursive=1", "", code, treeResponse)) {
        resultMessage = "GitHub recursive tree failed " + String(code);
        String detail = githubErrorDetail(treeResponse);
        if (detail.length() > 0) resultMessage += ": " + detail;
        return false;
    }
    int remoteDocCount = parseGithubTreeDocEntries(treeResponse, remoteDocs, MAX_SYNC_DOCS + 1);
    int syncStateCount = loadGitSyncState(syncState, MAX_SYNC_DOCS + 4);

    if (remoteDocCount == 0) {
        if (docCount <= 0) {
            resultMessage = "No documents to sync";
            return true;
        }

        int finalCount = 0;
        for (int i = 0; i < docCount; i++) {
            drawOnlineSyncScreen("GitHub Sync", "Uploading " + String(i + 1) + "/" + String(docCount), docNames[i]);
            if (!githubCreateBlobForDoc(docNames[i], uploadBlobShas[i], resultMessage)) return false;
            uploadDocNames[i] = docNames[i];

            String content;
            if (!githubReadDocText(docNames[i], content, resultMessage)) return false;
            finalDocNames[finalCount] = docNames[i];
            docPreviews[finalCount] = utf8Truncate(content, 240);
            docChars[finalCount] = getTrueLength(content);
            finalCount++;
            upsertGitSyncState(syncState, syncStateCount, MAX_SYNC_DOCS + 4, docNames[i], uploadBlobShas[i], uploadBlobShas[i]);
            yield();
        }

        String treeBody = "{\"base_tree\":\"" + baseTreeSha + "\",\"tree\":[";
        for (int i = 0; i < docCount; i++) {
            if (i > 0) treeBody += ",";
            treeBody += "{\"path\":\"" + jsonEscape(githubRemoteDocPath(uploadDocNames[i])) + "\",\"mode\":\"100644\",\"type\":\"blob\",\"sha\":\"" + uploadBlobShas[i] + "\"}";
        }
        treeBody += "]}";
        if (!githubHttpRequest("POST", githubApiBase() + "/git/trees", treeBody, code, response) || !extractJsonStringValue(response, "sha", newTreeSha)) {
            resultMessage = "GitHub tree create failed " + String(code);
            String detail = githubErrorDetail(response);
            if (detail.length() > 0) resultMessage += ": " + detail;
            return false;
        }

        String commitBody = "{\"message\":\"Sync Ize Compose documents\",\"tree\":\"" + newTreeSha + "\",\"parents\":[\"" + headCommitSha + "\"]}";
        if (!githubHttpRequest("POST", githubApiBase() + "/git/commits", commitBody, code, response) || !extractJsonStringValue(response, "sha", newCommitSha)) {
            resultMessage = "GitHub commit failed " + String(code);
            String detail = githubErrorDetail(response);
            if (detail.length() > 0) resultMessage += ": " + detail;
            return false;
        }

        String refBody = "{\"sha\":\"" + newCommitSha + "\",\"force\":false}";
        if (!githubHttpRequest("PATCH", updateRefUrl, refBody, code, response)) {
            resultMessage = "GitHub ref update failed " + String(code);
            String detail = githubErrorDetail(response);
            if (detail.length() > 0) resultMessage += ": " + detail;
            return false;
        }

        if (!saveGitSyncState(syncState, syncStateCount)) {
            resultMessage = "Sync state save failed";
            return false;
        }
        tempNetCursor = NET_MAIN;
        resultMessage = "Uploaded " + String(docCount) + ", downloaded 0";
        return true;
    }

    int unionCount = 0;
    for (int i = 0; i < docCount && unionCount < MAX_SYNC_DOCS; i++) finalDocNames[unionCount++] = docNames[i];
    for (int i = 0; i < remoteDocCount && unionCount < MAX_SYNC_DOCS; i++) {
        bool exists = false;
        for (int j = 0; j < unionCount; j++) {
            if (finalDocNames[j] == remoteDocs[i].name) {
                exists = true;
                break;
            }
        }
        if (!exists) finalDocNames[unionCount++] = remoteDocs[i].name;
    }

    if (unionCount <= 0) {
        resultMessage = "No documents to sync";
        return true;
    }

    int uploadCount = 0;
    int downloadCount = 0;
    int deleteRemoteCount = 0;
    int finalCount = 0;
    for (int i = 0; i < unionCount; i++) {
        String docName = finalDocNames[i];
        syncPlan[i] = GitSyncPlanEntry();
        syncPlan[i].name = docName;
        int remoteIdx = findRemoteDocEntry(remoteDocs, remoteDocCount, docName);
        bool remoteExists = remoteIdx >= 0;
        String remotePath = remoteExists ? remoteDocs[remoteIdx].remotePath : githubRemoteDocPath(docName);
        String remoteSha = remoteExists ? remoteDocs[remoteIdx].blobSha : "";
        syncPlan[i].remotePath = remotePath;
        syncPlan[i].remoteSha = remoteSha;

        bool localExists = false;
        String localContent = "";
        String localSha = "";
        for (int localIdx = 0; localIdx < docCount; localIdx++) {
            if (docNames[localIdx] == docName) {
                localExists = true;
                break;
            }
        }
        if (localExists) {
            if (!githubReadDocText(docName, localContent, resultMessage)) return false;
            localSha = githubBlobShaForContent(localContent);
        }
        syncPlan[i].localContent = localContent;
        syncPlan[i].localSha = localSha;

        int stateIdx = findGitSyncStateEntry(syncState, syncStateCount, docName);
        String lastLocalSha = stateIdx >= 0 ? syncState[stateIdx].localBlobSha : "";
        String lastRemoteSha = stateIdx >= 0 ? syncState[stateIdx].remoteBlobSha : "";

        String finalContent = localContent;
        String finalSha = localSha;

        if (!localExists && remoteExists) {
            syncPlan[i].action = GIT_SYNC_DELETE_REMOTE;
            deleteRemoteCount++;
        } else if (localExists && !remoteExists) {
            syncPlan[i].action = GIT_SYNC_UPLOAD;
            uploadCount++;
        } else if (localExists && remoteExists) {
            if (localSha == remoteSha) {
                finalContent = localContent;
                finalSha = localSha;
            } else {
                bool localChanged = (stateIdx < 0) ? true : (localSha != lastLocalSha);
                bool remoteChanged = (stateIdx < 0) ? true : (remoteSha != lastRemoteSha);
                if (!localChanged && remoteChanged) {
                    syncPlan[i].action = GIT_SYNC_DOWNLOAD;
                    downloadCount++;
                } else if (localChanged && !remoteChanged) {
                    syncPlan[i].action = GIT_SYNC_UPLOAD;
                    uploadCount++;
                } else {
                    uint64_t localStamp = 0;
                    uint64_t remoteStamp = 0;
                    bool haveLocalStamp = getLocalDocModifyStamp(docName, localStamp);
                    bool haveRemoteStamp = githubFetchRemoteCommitStamp(remotePath, remoteStamp, resultMessage);
                    if (!haveRemoteStamp) return false;
                    if (!haveLocalStamp) {
                        resultMessage = "Conflict for " + docName + ": local modified time unavailable";
                        return false;
                    }
                    if (remoteStamp > localStamp) {
                        syncPlan[i].action = GIT_SYNC_DOWNLOAD;
                        downloadCount++;
                    } else {
                        syncPlan[i].action = GIT_SYNC_UPLOAD;
                        uploadCount++;
                    }
                }
            }
        }

        yield();
    }

    drawOnlineSyncScreen("GitHub Sync", "Compare done", "Upload " + String(uploadCount) + " / Download " + String(downloadCount) + " / Delete " + String(deleteRemoteCount));

    int uploadProgress = 0;
    int downloadProgress = 0;
    int deleteRemoteProgress = 0;
    for (int i = 0; i < unionCount; i++) {
        String docName = syncPlan[i].name;
        String finalContent = syncPlan[i].localContent;
        String finalSha = syncPlan[i].localSha;

        if (syncPlan[i].action == GIT_SYNC_DELETE_REMOTE) {
            deleteRemoteProgress++;
            drawOnlineSyncScreen("GitHub Sync", "Deleting " + String(deleteRemoteProgress) + "/" + String(deleteRemoteCount), docName);
            deleteRemotePaths[deleteRemoteProgress - 1] = syncPlan[i].remotePath;
            yield();
            continue;
        } else if (syncPlan[i].action == GIT_SYNC_DOWNLOAD) {
            downloadProgress++;
            drawOnlineSyncScreen("GitHub Sync", "Downloading " + String(downloadProgress) + "/" + String(downloadCount), docName);
            if (!githubFetchRemoteFileContent(syncPlan[i].remotePath, finalContent, finalSha, resultMessage)) return false;
            if (!writeDocTextToSd(docName, finalContent, resultMessage)) return false;
        } else if (syncPlan[i].action == GIT_SYNC_UPLOAD) {
            uploadProgress++;
            drawOnlineSyncScreen("GitHub Sync", "Uploading " + String(uploadProgress) + "/" + String(uploadCount), docName);
            if (!githubCreateBlobForContent(syncPlan[i].localContent, uploadBlobShas[uploadProgress - 1], resultMessage)) return false;
            uploadDocNames[uploadProgress - 1] = docName;
            finalContent = syncPlan[i].localContent;
            finalSha = uploadBlobShas[uploadProgress - 1];
        }

        if (finalContent.length() > 0 || syncPlan[i].action != GIT_SYNC_SKIP) {
            docPreviews[finalCount] = utf8Truncate(finalContent, 240);
            docChars[finalCount] = getTrueLength(finalContent);
            finalDocNames[finalCount] = docName;
            finalCount++;
            upsertGitSyncState(syncState, syncStateCount, MAX_SYNC_DOCS + 4, docName, finalSha, finalSha);
        }
    }

    if (downloadCount > 0) refreshFileList();
    
    bool anyDocChanged = (uploadCount > 0 || downloadCount > 0 || deleteRemoteCount > 0);

    if (!anyDocChanged) {
        if (!saveGitSyncState(syncState, syncStateCount)) {
            resultMessage = "Sync state save failed";
            return false;
        }
        tempNetCursor = NET_MAIN;
        resultMessage = "Already up to date";
        return true;
    }

    if (uploadCount == 0 && deleteRemoteCount == 0) {
        if (!saveGitSyncState(syncState, syncStateCount)) {
            resultMessage = "Sync state save failed";
            return false;
        }
        tempNetCursor = NET_MAIN;
        resultMessage = "Uploaded 0, downloaded " + String(downloadCount) + ", deleted 0";
        return true;
    }

    String treeBody = "{\"base_tree\":\"" + baseTreeSha + "\",\"tree\":[";
    int treeEntryCount = 0;
    for (int i = 0; i < uploadCount; i++) {
        if (treeEntryCount > 0) treeBody += ",";
        treeBody += "{\"path\":\"" + jsonEscape(githubRemoteDocPath(uploadDocNames[i])) + "\",\"mode\":\"100644\",\"type\":\"blob\",\"sha\":\"" + uploadBlobShas[i] + "\"}";
        treeEntryCount++;
    }
    for (int i = 0; i < deleteRemoteCount; i++) {
        if (treeEntryCount > 0) treeBody += ",";
        treeBody += "{\"path\":\"" + jsonEscape(deleteRemotePaths[i]) + "\",\"sha\":null}";
        treeEntryCount++;
    }
    treeBody += "]}";
    if (!githubHttpRequest("POST", githubApiBase() + "/git/trees", treeBody, code, response) || !extractJsonStringValue(response, "sha", newTreeSha)) {
        resultMessage = "GitHub tree create failed " + String(code);
        String detail = githubErrorDetail(response);
        if (detail.length() > 0) resultMessage += ": " + detail;
        return false;
    }

    String commitBody = "{\"message\":\"Sync Ize Compose documents\",\"tree\":\"" + newTreeSha + "\",\"parents\":[\"" + headCommitSha + "\"]}";
    if (!githubHttpRequest("POST", githubApiBase() + "/git/commits", commitBody, code, response) || !extractJsonStringValue(response, "sha", newCommitSha)) {
        resultMessage = "GitHub commit failed " + String(code);
        String detail = githubErrorDetail(response);
        if (detail.length() > 0) resultMessage += ": " + detail;
        return false;
    }

    String refBody = "{\"sha\":\"" + newCommitSha + "\",\"force\":false}";
    if (!githubHttpRequest("PATCH", updateRefUrl, refBody, code, response)) {
        resultMessage = "GitHub ref update failed " + String(code);
        String detail = githubErrorDetail(response);
        if (detail.length() > 0) resultMessage += ": " + detail;
        return false;
    }

    for (int i = 0; i < uploadCount; i++) {
        upsertGitSyncState(syncState, syncStateCount, MAX_SYNC_DOCS + 4, uploadDocNames[i], uploadBlobShas[i], uploadBlobShas[i]);
    }
    if (!saveGitSyncState(syncState, syncStateCount)) {
        resultMessage = "Sync state save failed";
        return false;
    }
    tempNetCursor = NET_MAIN;
    resultMessage = "Uploaded " + String(uploadCount) + ", downloaded " + String(downloadCount) + ", deleted " + String(deleteRemoteCount);
    return true;
}
#else
bool runGithubDocumentSync(String& resultMessage) {
    resultMessage = "Direct GitHub HTTPS is not built.";
    return false;
}
#endif

String githubErrorDetail(const String& response) {
    String message;
    if (extractJsonStringValue(response, "message", message) && message.length() > 0) {
        return message;
    }
    String compact = response;
    compact.replace('\r', ' ');
    compact.replace('\n', ' ');
    compact.trim();
    if (compact.length() > 72) compact = compact.substring(0, 72) + "...";
    return compact;
}

void resetStatusScreenCache() {
    lastStatusScreenTitle = "";
    lastStatusScreenLine1 = "";
    lastStatusScreenLine2 = "";
    statusScreenPrimed = false;
}

void drawStatusScreenFrame(const String& title, const String& line1, const String& line2, bool forceFullRefresh) {
    bool sameAsLast = statusScreenPrimed &&
                      title == lastStatusScreenTitle &&
                      line1 == lastStatusScreenLine1 &&
                      line2 == lastStatusScreenLine2;
    if (sameAsLast) return;

    display.fillRect(0, 0, display.width(), display.height(), WHITE);
    u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
    printDualFont(title, MARGIN_X, MARGIN_Y + 10, true);
    printDualFont(line1, MARGIN_X, MARGIN_Y + 55, true);
    if (line2.length() > 0) printDualFont(line2, MARGIN_X, MARGIN_Y + 95, true);

    if (forceFullRefresh || !statusScreenPrimed) display.display();
    else display.partialUpdate(false, true);

    lastStatusScreenTitle = title;
    lastStatusScreenLine1 = line1;
    lastStatusScreenLine2 = line2;
    statusScreenPrimed = true;
}

void drawOnlineSyncScreen(const String& title, const String& line1, const String& line2) {
    drawStatusScreenFrame(title, line1, line2);
}

void finishOnlineSyncToMenu(const String& title, const String& detail) {
    drawOnlineSyncScreen(title, detail, "");
    delay(2200);
    stopNetworkServices();
    currentMode = FILE_MENU_MODE;
    menuFocusSide = 0;
    leftMenuIndex = 0;
    isEditingValue = false;
    needUpdate = true;
    statusBarNeedsUpdate = true;
}
void openWifiModeAfterOnlineSyncError(const String& title, const String& detail) {
    drawOnlineSyncScreen(title, detail, "Opening Wi-Fi mode...");
    delay(2400);
    startWifiScanMode();
}

void runOnlineSyncFlow() {
    flushKorean();
    saveFile();
    if (!githubConfigComplete()) {
        githubSyncStatusMessage = "GitHub info missing";
        openWifiModeAfterOnlineSyncError("Online Sync", "GitHub repository info missing");
        return;
    }
    githubSyncStatusMessage = "GitHub syncing";
    drawOnlineSyncScreen("Online Sync", "Connecting saved Wi-Fi...");
    startWifiScanMode(true);
}
String getNetworkModeLabel(NetworkSubMode mode) {
    if (mode == NET_WIFI_STA) return "WiFi";
    if (mode == NET_WIFI) return "WebServer";
    return "Off";
}
String getAccentChar(char base, int mode, int cycle) {
    int idx = cycle - 1;
    if (mode == 1) { 
        if (base == 'a') { String arr[] = {"\xC3\xA1", "\xC3\xA0", "\xC3\xA2", "\xC3\xA4", "\xC3\xA3", "\xC3\xA6", "a"}; return arr[idx % 7]; }
        if (base == 'A') { String arr[] = {"\xC3\x81", "\xC3\x80", "\xC3\x82", "\xC3\x84", "\xC3\x83", "\xC3\x86", "A"}; return arr[idx % 7]; }
        if (base == 'e') { String arr[] = {"\xC3\xA9", "\xC3\xA8", "\xC3\xAA", "\xC3\xAB", "e"}; return arr[idx % 5]; }
        if (base == 'E') { String arr[] = {"\xC3\x89", "\xC3\x88", "\xC3\x8A", "\xC3\x8B", "E"}; return arr[idx % 5]; }
        if (base == 'i') { String arr[] = {"\xC3\xAD", "\xC3\xAC", "\xC3\xAE", "\xC3\xAF", "\xC4\xB1", "i"}; return arr[idx % 6]; } 
        if (base == 'I') { String arr[] = {"\xC3\x8D", "\xC3\x8C", "\xC3\x8E", "\xC3\x8F", "\xC4\xB0", "I"}; return arr[idx % 6]; } 
        if (base == 'o') { String arr[] = {"\xC3\xB3", "\xC3\xB2", "\xC3\xB4", "\xC3\xB6", "\xC3\xB5", "\xC5\x93", "o"}; return arr[idx % 7]; }
        if (base == 'O') { String arr[] = {"\xC3\x93", "\xC3\x92", "\xC3\x94", "\xC3\x96", "\xC3\x95", "\xC5\x92", "O"}; return arr[idx % 7]; }
        if (base == 'u') { String arr[] = {"\xC3\xBA", "\xC3\xB9", "\xC3\xBB", "\xC3\xBC", "u"}; return arr[idx % 5]; }
        if (base == 'U') { String arr[] = {"\xC3\x9A", "\xC3\x99", "\xC3\x9B", "\xC3\x9C", "U"}; return arr[idx % 5]; }
        if (base == 'c') { String arr[] = {"\xC3\xA7", "\xC4\x87", "\xC4\x8D", "c"}; return arr[idx % 4]; } 
        if (base == 'C') { String arr[] = {"\xC3\x87", "\xC4\x86", "\xC4\x8C", "C"}; return arr[idx % 4]; } 
        if (base == 'n') { String arr[] = {"\xC3\xB1", "n"}; return arr[idx % 2]; }
        if (base == 'N') { String arr[] = {"\xC3\x91", "N"}; return arr[idx % 2]; }
        if (base == 's') { String arr[] = {"\xC3\x9F", "\xC5\xA1", "\xC5\x9B", "\xC5\x9F", "s"}; return arr[idx % 5]; } 
        if (base == 'S') { String arr[] = {"\xC5\xA0", "\xC5\x9A", "\xC5\x9E", "S"}; return arr[idx % 4]; } 
        if (base == 'd') { String arr[] = {"\xC4\x91", "d"}; return arr[idx % 2]; } 
        if (base == 'D') { String arr[] = {"\xC4\x90", "D"}; return arr[idx % 2]; } 
        if (base == 'z') { String arr[] = {"\xC5\xBE", "\xC5\xBA", "z"}; return arr[idx % 3]; } 
        if (base == 'Z') { String arr[] = {"\xC5\xBD", "\xC5\xB9", "Z"}; return arr[idx % 3]; } 
        if (base == 'g') { String arr[] = {"\xC4\x9F", "g"}; return arr[idx % 2]; } 
        if (base == 'G') { String arr[] = {"\xC4\x9E", "G"}; return arr[idx % 2]; } 
    }
    return "";
}

String getSdFirmwarePath() {
    if (sdPathExists(FIRMWARE_UPDATE_PATH)) return FIRMWARE_UPDATE_PATH;
    return "";
}

void failSdOta(const char* reason) {
    removeSdFile(FIRMWARE_UPDATE_PATH);   // Prevent endless retry of a failed staged firmware.
    isUpdating = false;
    updateScreenDrawn = false;
    updateState = UPD_NONE;
    currentNetSubMode = NET_MAIN;
    needUpdate = true;

    display.fillRect(0, 0, display.width(), display.height(), WHITE);
    int updateCenterY = (int)((display.height() / displayScale) / 2);
    printCleanText(u8g2_for_adafruit_gfx, "Update failed.", MARGIN_X, updateCenterY - 20);
    printCleanText(u8g2_for_adafruit_gfx, reason, MARGIN_X, updateCenterY + 10);
    printCleanText(u8g2_for_adafruit_gfx, "Return to writing mode.", MARGIN_X, updateCenterY + 40);
    display.display();
    delay(2500);
}

void performSdOta() {
    String path = getSdFirmwarePath();
    if (path == "") {
        failSdOta("Firmware file is missing.");
        return;
    }
    SdFile f;
    if (!f.open(path.c_str(), O_RDONLY)) {
        failSdOta("Cannot open firmware file.");
        return;
    }

    size_t fSize = f.fileSize();
    isUpdating = true;
    needUpdate = true;

    if (!Update.begin(fSize)) {
        Update.printError(Serial);
        f.close();
        failSdOta("Firmware does not fit OTA slot.");
        return;
    }

    uint8_t buf[512];
    int n;
    while ((n = f.read(buf, sizeof(buf))) > 0) {
        if (Update.write(buf, n) != n) {
            Update.printError(Serial);
            f.close();
            failSdOta("Writing firmware failed.");
            return;
        }
        yield();
    }
    f.close();
    if (Update.end(true)) {
        removeSdFile(path.c_str());
        delay(500);
        ESP.restart();
    } else {
        Update.printError(Serial);
        failSdOta("Finalizing firmware failed.");
    }
}



KeyboardLayoutId getSelectedKeyboardLayoutId() {
    if (!isKoreanMode) return englishLayoutIndex <= 0 ? KB_DVORAK : KB_QWERTY;
    if (keyboardLayoutIndex < 2 || keyboardLayoutIndex >= KEYBOARD_LAYOUT_TOTAL) keyboardLayoutIndex = 2;
    return KEYBOARD_LAYOUTS[keyboardLayoutIndex].id;
}

KeyEngineScript getSelectedKeyEngine() {
    KeyboardLayoutId id = getSelectedKeyboardLayoutId();
    if (id == KB_KOREAN) return KEY_ENGINE_KOREAN;
    if (id == KB_ARABIC || id == KB_KURDISH_ARABIC || id == KB_PASHTO || id == KB_PERSIAN || id == KB_URDU) return KEY_ENGINE_ARABIC;
    if (id == KB_HEBREW) return KEY_ENGINE_HEBREW;
    if (id == KB_BENGALI || id == KB_DEVANAGARI || id == KB_GUJARATI || id == KB_KANNADA || id == KB_MALAYALAM || id == KB_NEPALI || id == KB_PUNJABI || id == KB_TAMIL || id == KB_TELUGU) return KEY_ENGINE_INDIC;
    if (id == KB_THAI) return KEY_ENGINE_THAI;
    if (id == KB_JAPAN) return KEY_ENGINE_JAPANESE;
    if (id == KB_MYANMAR) return KEY_ENGINE_MYANMAR;
    if (id == KB_KHMER) return KEY_ENGINE_KHMER;
    if (id == KB_LAO) return KEY_ENGINE_LAO;
    if (id == KB_TIBETAN) return KEY_ENGINE_TIBETAN;
    if (id == KB_SINHALA) return KEY_ENGINE_SINHALA;
    if (id == KB_ETHIOPIC) return KEY_ENGINE_ETHIOPIC;
    return KEY_ENGINE_NONE;
}

String getKeyboardMappedInput(byte k, bool shift, bool alt, bool caps, const char* engMap, const char* shiftMap, size_t mapSize, char fallback) {
    if (k >= mapSize) return fallback == 0 ? String("") : String(fallback);
    char base = engMap[k];
    if (base == 0 || base == '\b' || base == '\t' || base == '\n' || base == ' ') return fallback == 0 ? String("") : String(fallback);
    char keyBuf[2] = {base, 0};
    uint8_t total = 0;
    KeyboardLayoutId layoutId = getSelectedKeyboardLayoutId();
    const KeyboardKeyMap* map = keyboardGetMap(layoutId, total);
    for (uint8_t i = 0; i < total; i++) {
        if (strcmp(map[i].key, keyBuf) == 0) {
            bool useShift = shift;
            if (caps && !alt && map[i].normal != nullptr && map[i].shift != nullptr &&
                strlen(map[i].normal) == 1 && strlen(map[i].shift) == 1 &&
                map[i].normal[0] >= 'a' && map[i].normal[0] <= 'z' &&
                map[i].shift[0] == (char)(map[i].normal[0] - 'a' + 'A')) {
                useShift = !shift;
            }
            const char* value = alt ? (useShift ? map[i].altShift : map[i].alt) : (useShift ? map[i].shift : map[i].normal);
            if (value == nullptr && useShift && !alt) value = map[i].normal;
            if (value == nullptr && useShift && alt) value = map[i].alt;
            if (value == nullptr && layoutId == KB_ETHIOPIC && !shift && !alt && (base == 'e' || base == 'u' || base == 'i' || base == 'o' || base == 'a')) return String(base);
            if (value == nullptr && !alt && fallback != 0) return String(fallback);
            if (value == nullptr) return String("");
            return String(value);
        }
    }
    return fallback == 0 ? String("") : String(fallback);
}

char getAccentBaseFromInserted(const String& inserted) {
    if (inserted.length() != 1) return 0;
    char c = inserted[0];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return c;
    return 0;
}

void showInitialImage() { 
    flushKorean(); 
    display.clearDisplay(); 

    if (imgBuffer) {
        int16_t x = (display.width() - 800) / 2;
        int16_t y = (display.height() - 600) / 2;
        if (x < 0) x = 0;
        if (y < 0) y = 0;

        display.image.drawPngFromBuffer(imgBuffer, imgSize, x, y, true, false);
    }

    display.display(); 
    delay(1000); 

    
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_enable_ext0_wakeup(WAKE_BUTTON_PIN, 0);
    esp_light_sleep_start(); 

    
    display.clearDisplay(); 
    display.fillRect(0, 0, display.width(), display.height(), WHITE); 
    while (Serial.available() > 0) Serial.read(); 
    isCtrlPressed = false;
    isShiftPressed = false;
    isAltPressed = false;
    lastKeyPress = millis();
    currentMode = TYPING_MODE; 
    needUpdate = true; 
    statusBarNeedsUpdate = true; 
}

void loop() {
if (savedMessageVisible && millis() - showSavedMessageTime >= 2000) {
    savedMessageVisible = false;
    needUpdate = true;
    statusBarNeedsUpdate = true;
}

if (currentNetSubMode == NET_WIFI || currentNetSubMode == NET_WIFI_STA || updateState == UPD_WIFI_WAITING) {
    server.handleClient(); 
    if (currentNetSubMode == NET_WIFI && !isUpdating && updateState == UPD_NONE) {
        int stations = WiFi.softAPgetStationNum();
        if (stations > 0) {
            apHadClient = true;
            if (!apPasswordHidden) {
                apPasswordHidden = true;
                needUpdate = true;
                statusBarNeedsUpdate = true;
            }
            apNoClientSinceMs = 0;
        } else if (apHadClient) {
            if (apNoClientSinceMs == 0) apNoClientSinceMs = millis();
            if (millis() - apNoClientSinceMs > 2000) {
                stopNetworkServices();
                needUpdate = true;
                statusBarNeedsUpdate = true;
            }
        }
    }
    }
    
    if (updateState == UPD_WIFI_WAITING) {
        // server.handleClient() is already called above for this state.
        // Avoid double servicing the web server in the same loop tick.
        while (Serial.available() > 0) {
            byte k = Serial.read();
            if (k == 246) { 
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_OFF);
                updateState = UPD_NONE;
                currentNetSubMode = NET_MAIN;
                pinInputBuffer = "";
                otaPinCode = "";
                isCtrlPressed = false;
                isShiftPressed = false;
                isAltPressed = false;
                needUpdate = true;
                break;
            }
        }
        return;
    }

if (__atomic_load_n(&networkExitRequested, __ATOMIC_SEQ_CST)) { 
    __atomic_store_n(&networkExitRequested, false, __ATOMIC_SEQ_CST);  
        stopNetworkServices();
        isCtrlPressed = false;
        isShiftPressed = false;
        isAltPressed = false;
        needUpdate = true;
        statusBarNeedsUpdate = true;
    }

    if (isUpdating) {

    
        if (!updateScreenDrawn) {
            drawStatusScreenFrame("Update", "Updating. Do not close browser.", "Do not disconnect power.", true);
            updateScreenDrawn = true;
        }
        if (currentNetSubMode == NET_WIFI || currentNetSubMode == NET_WIFI_STA) server.handleClient();
        yield(); 
        return;
    }

    if (updateState == UPD_SD_RUNNING) {

            
            drawStatusScreenFrame("Update", "Updating. Do not close browser.", "Do not disconnect power.", true);
            performSdOta();
            return;
        }

    if (currentMode == WEB_PASSWORD_MODE) {
        if (needUpdate) {
            if (webServerPasswordInput.length() == 0) {
                drawStatusScreenFrame("WebServer Password", "Type 10 digits", webServerPasswordHint, !statusScreenPrimed);
            } else {
                String visiblePassword = webServerPasswordInput;
                for (int i = webServerPasswordInput.length(); i < 10; i++) visiblePassword += "_";
                drawStatusScreenFrame("WebServer Password", "Password: " + visiblePassword, webServerPasswordHint, !statusScreenPrimed);
            }
            needUpdate = false;
        }
        while (Serial.available() > 0) {
            byte k = Serial.read();
            const char engMap[] = {'`','1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\\',0,'a','s','d','f','g','h','j','k','l',';','\'','\n',0,'z','x','c','v','b','n','m',',','.','/',0,0,0,0,0,' ',0,0,0};
            char c = (k < sizeof(engMap)) ? engMap[k] : 0;
            if (c >= '0' && c <= '9') {
                if (webServerPasswordInput.length() < 10) {
                    webServerPasswordInput += c;
                    webServerPasswordHint = (webServerPasswordInput.length() == 10) ? "Enter: start / Tab cancels" : "Numbers only / Tab cancels";
                } else {
                    webServerPasswordHint = "Press Enter to start";
                }
                needUpdate = true;
            } else if (k == 13 && webServerPasswordInput.length() > 0) {
                webServerPasswordInput.remove(webServerPasswordInput.length() - 1);
                webServerPasswordHint = "Numbers only / Tab cancels";
                needUpdate = true;
            } else if (k == 40) {
                if (webServerPasswordInput.length() == 10) {
                    activeApPassword = webServerPasswordInput;
                    generateWebPin();
                    currentNetSubMode = NET_WIFI;
                    currentMode = TYPING_MODE;
                    setupWiFi();
                    needUpdate = true;
                    break;
                }
                webServerPasswordHint = "Need 10 digits";
                needUpdate = true;
            } else if (k == 246 || c == '\t') {
                webServerPasswordInput = "";
                webServerPasswordHint = "Numbers only / Tab cancels";
                currentMode = FILE_MENU_MODE;
                needUpdate = true;
                break;
            } else if (c != 0) {
                webServerPasswordHint = "Numbers only";
                needUpdate = true;
            }
        }
        return;
    }
  bool forceImmediateRender = false;
  while (Serial.available() > 0) {

    byte k = Serial.read(); 
    char real = 0;
    lastKeyPress = millis(); 
    if (currentNetSubMode == NET_MAIN) {
        needUpdate = true; 
    } else {
        needUpdate = false;
        statusBarNeedsUpdate = false;
    }
    
    if (k == 240) { isShiftPressed = true; needUpdate = false; statusBarNeedsUpdate = false; continue; } 
    if (k == 241) { isShiftPressed = false; needUpdate = false; statusBarNeedsUpdate = false; continue; } 
    if (k == 242) { isCtrlPressed = true; needUpdate = false; statusBarNeedsUpdate = false; continue; } 
    if (k == 243) { isCtrlPressed = false; needUpdate = false; statusBarNeedsUpdate = false; continue; }

    if (k == 244) { 
        isAltPressed = true;
        bool accentApplied = false;
        
        KeyboardLayoutId activeLayout = getSelectedKeyboardLayoutId();
        if ((activeLayout == KB_QWERTY || activeLayout == KB_DVORAK) && (millis() - lastTypingTime <= 3000)) {
            accentCycleIdx++;
            String newChar = getAccentChar(lastBaseChar, 1, accentCycleIdx);
            if (newChar != "") {
                doBackspace(); 
                insertText(newChar); 
                lastTypingTime = millis(); 
                needUpdate = true;
                accentApplied = true;
            }
        }
        // Alt no-op: do not redraw unless an accent replacement actually happened.
        if (!accentApplied) { needUpdate = false; statusBarNeedsUpdate = false; }
        continue; 
    } 
    if (k == 245) { isAltPressed = false; needUpdate = false; statusBarNeedsUpdate = false; continue; }

    if (k == 28) {
        isCapsLockOn = !isCapsLockOn;
        needUpdate = true;
        statusBarNeedsUpdate = true;
        continue; 
    }
    const char engMap[] = { '`','1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\\',0,'a','s','d','f','g','h','j','k','l',';','\'','\n',0,'z','x','c','v','b','n','m',',','.','/',0,0,0,0,0,' ',0,0,0 }; 
    const char shiftMap[] = { '~','!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','|',0,'A','S','D','F','G','H','J','K','L',':','\"','\n',0,'Z','X','C','V','B','N','M','<','>','?',0,0,0,0,0,' ',0,0,0 };
    
    
    if (k < sizeof(engMap)) {
        char base = engMap[k];
        
        if (base >= 'a' && base <= 'z') {
            
            real = (isShiftPressed != isCapsLockOn) ? shiftMap[k] : engMap[k];
        } else {
            
            real = isShiftPressed ? shiftMap[k] : engMap[k];
        }
    }

        if (k == 56) real = ' '; 
        
    bool isNavigationOrMenuKey = (k == 57 || k == 58 || k == 59 || k == 60 || k == 246);
    if (real == 0 && k != 56 && !isNavigationOrMenuKey) {
        // Unknown/non-printing scan code: do not trigger a redraw or typing delay.
        needUpdate = false;
        statusBarNeedsUpdate = false;
        continue;
    }
    
    if (currentMode == WIFI_SCAN_MODE) {
        if (k == 58 && wifiScanSelected > 0) wifiScanSelected--;
        else if (k == 59 && wifiScanSelected < wifiScanCount - 1) wifiScanSelected++;
        else if (real == '\t' || k == 246) {
            stopNetworkServices();
            currentMode = TYPING_MODE;
        } else if ((real == '\n' || k == 40) && wifiScanCount > 0) {
            if (savedWifiPassword.length() > 0 && connectSelectedWifi(savedWifiPassword)) {
                // Connected with the recently successful password. GitHub sync runs without web PIN in a later phase.
            } else {
                currentMode = WIFI_PASSWORD_MODE;
                wifiPasswordBuffer = "";
                wifiStatusMessage = "Enter password. Enter: connect. Tab/Menu: cancel.";
            }
        }
        needUpdate = true;
        statusBarNeedsUpdate = false;
        continue;
    }

    if (currentMode == WIFI_PASSWORD_MODE) {
        if (real == '\t' || k == 246) {
            stopNetworkServices();
            currentMode = TYPING_MODE;
            needUpdate = true;
            statusBarNeedsUpdate = true;
            continue;
        }
        if (real == '\b' || k == 13) {
            if (wifiPasswordBuffer.length() > 0) wifiPasswordBuffer.remove(wifiPasswordBuffer.length() - 1);
            needUpdate = true;
            continue;
        }
        if (real == '\n' || k == 40) {
            connectSelectedWifi(wifiPasswordBuffer);
            needUpdate = true;
            continue;
        }
        if (real >= 32 && real <= 126 && wifiPasswordBuffer.length() < 63) {
            wifiPasswordBuffer += real;
            needUpdate = true;
            continue;
        }
        needUpdate = false;
        statusBarNeedsUpdate = false;
        continue;
    }
    if (isCtrlPressed) {
      if (currentNetSubMode != NET_MAIN && k == 246) {
          stopNetworkServices();
          needUpdate = true;
          statusBarNeedsUpdate = true;
          continue;
      }
      if (real == 'l' || real == 'L') { flushKorean(); isCtrlPressed = false; showInitialImage(); continue; }
      if (real == 'c' || real == 'C') { flushKorean(); clipboard = fullText; needUpdate = false; statusBarNeedsUpdate = false; continue; } 
      if (real == 'v' || real == 'V') { flushKorean(); insertText(clipboard); continue; } 
      
      if (real == 's' || real == 'S') { flushKorean(); saveFile(); continue; }
      if (real == 'n' || real == 'N') { flushKorean(); createNewDoc(); continue; }
      if (real == 'r' || real == 'R') { flushKorean(); hardRefresh(); continue; }
      if (real == 'f' || real == 'F') { 
          flushKorean(); 
          if (currentMode == SEARCH_MODE) { currentMode = TYPING_MODE; searchMatchEnd = -1; }
          else { currentMode = SEARCH_MODE; searchQuery = ""; searchMatchEnd = -1; }
          needUpdate = true; continue; 
      }
      if (k == 57) { moveCursorToLineStart(); continue; } 
      if (k == 60) { moveCursorToLineEnd(); continue; }   
      if (k == 58) { moveCursorToParagraphStart(); continue; } 
      if (k == 59) { moveCursorToParagraphEnd(); continue; }   
      // Do not let unhandled Ctrl+letter combinations fall through as normal typing.
      // Ctrl+Space and Ctrl+Backspace intentionally continue to the typing path below.
      if (real != ' ' && real != '\b') {
          needUpdate = false;
          statusBarNeedsUpdate = false;
          continue;
      }
    }
    if (k == 246) { 
      if (currentNetSubMode != NET_MAIN) {
        needUpdate = false;
        statusBarNeedsUpdate = false;
        continue;
      }
      if (currentMode == TYPING_MODE) { 
        flushKorean(); 
        currentMode = FILE_MENU_MODE; 
        inSystemSubMenu = false;
        leftMenuIndex = 0;
        leftMenuOffset = 0;
        menuFocusSide = 0;
        isEditingValue = false;
        previewEnglishLayoutIndex = englishLayoutIndex;
        previewKeyboardLayoutIndex = keyboardLayoutIndex;
        tempNetCursor = currentNetSubMode;
        refreshFileList(); 
        rightFileIndex = 1;
        fileScrollOffset = 0;  // Always show the first 12 documents when reopening the file menu.
        isDeletingFile = false; cancelDeleteCodePrompt(); } 
      else {
        currentMode = TYPING_MODE;
        isEditingValue = false;
        isDeletingFile = false;
        cancelDeleteCodePrompt();
        inSystemSubMenu = false;
        leftMenuOffset = 0;
      }
      needUpdate = true;
      statusBarNeedsUpdate = true;
      forceImmediateRender = true;
      break; 
    }
    if (currentNetSubMode != NET_MAIN) {
        needUpdate = false;
        statusBarNeedsUpdate = false;
        continue;
    } 
    
    else if (currentMode == FILE_MENU_MODE) {
      
      if (deleteCodePromptActive) {
        if (real >= '0' && real <= '9' && deleteConfirmInput.length() < 6) {
          deleteConfirmInput += real;
        } else if (real == '\b' || k == 13) {
          if (deleteConfirmInput.length() > 0) deleteConfirmInput.remove(deleteConfirmInput.length() - 1);
        } else if (real == '\n') {
          if (deleteConfirmInput == deleteConfirmCode) {
            deletePendingFileFromMenu();
            isDeletingFile = false;
            cancelDeleteCodePrompt();
          } else {
            deleteConfirmInput = "";
          }
        } else if (real == '\t' || k == 246 || k == 58 || k == 59 || k == 57 || k == 60) {
          isDeletingFile = false;
          cancelDeleteCodePrompt();
        }
        needUpdate = true;
        statusBarNeedsUpdate = true;
        continue;
      }
      if (isDeletingFile) { 
        if (real == '\n') { 
          startDeleteCodePrompt();
        } else if (real == '\b' || k == 58 || k == 59 || k == 57 || k == 60 || real == '\t' || k == 246) {
          isDeletingFile = false;
          cancelDeleteCodePrompt();
        }
        needUpdate = true;
        statusBarNeedsUpdate = true;
        continue; 
      }
      if (isEditingValue) {
        {
            switch (leftMenuIndex) {
                case 5:
                    if (k == 57 || k == 60) {
                        if (tempNetCursor == NET_MAIN) tempNetCursor = NET_WIFI_STA;
                        else if (tempNetCursor == NET_WIFI_STA) tempNetCursor = NET_WIFI;
                        else tempNetCursor = NET_MAIN;
                    }
                    if (real == '\n') {
                        isEditingValue = false;
                        currentNetSubMode = tempNetCursor;
                        if (currentNetSubMode == NET_WIFI) { webServerPasswordInput = ""; webServerPasswordHint = "Numbers only / Tab cancels"; currentMode = WEB_PASSWORD_MODE; resetStatusScreenCache(); }
                        else if (currentNetSubMode == NET_WIFI_STA) { startWifiScanMode(); }
                        else { stopNetworkServices(); currentMode = TYPING_MODE; }
                    }
                    break;

            }
        }
        
        needUpdate = true; 
        continue; 
    }
    if (k == 57) { menuFocusSide = 0; needUpdate = true; } 
    if (k == 60) { menuFocusSide = 1; needUpdate = true; }

    if (menuFocusSide == 0) {
            int menuStep = baseFontSize + lineSpacing + 8;
            int menuTopY = 100;
            int maxVisibleMenu = ((display.height() / displayScale) - menuTopY - 10) / menuStep;
            if (maxVisibleMenu < 1) maxVisibleMenu = 1;
            int menuCount = 6;
            if (leftMenuIndex >= menuCount) leftMenuIndex = menuCount - 1;
            int maxOffset = menuCount - maxVisibleMenu;
            if (maxOffset < 0) maxOffset = 0;
            if (leftMenuOffset > maxOffset) leftMenuOffset = maxOffset;

            if (k == 58 && leftMenuIndex > 0) { 
                leftMenuIndex--; 
                if (leftMenuIndex < leftMenuOffset) leftMenuOffset = leftMenuIndex;
            } 
            if (k == 59 && leftMenuIndex < menuCount - 1) { 
                leftMenuIndex++; 
                if (leftMenuIndex >= leftMenuOffset + maxVisibleMenu) {
                    leftMenuOffset = leftMenuIndex - maxVisibleMenu + 1;
                }
            }
            needUpdate = true;
          if (real == '\n') { 
            if (leftMenuIndex == 0) { currentMode = TYPING_MODE; runOnlineSyncFlow(); }
            else if (leftMenuIndex == 1) createNewDoc();
            else if (leftMenuIndex == 2) { saveFile(); currentMode = TYPING_MODE; }
            else if (leftMenuIndex == 3) { countMode = (countMode + 1) % 3; saveSystemSettings(); needUpdate = true; }
            else if (leftMenuIndex == 4) { currentMode = TYPING_MODE; showInitialImage(); ESP.restart(); }
            else if (leftMenuIndex == 5) { isEditingValue = true; }
            needUpdate = true;
            statusBarNeedsUpdate = true;
            continue;
          }
        } else {
          
          const int maxVisibleItems = FILE_MENU_ITEMS_PER_PAGE;
          
          if (k == 58 && rightFileIndex > 1) { 
              rightFileIndex--;
              
              if (rightFileIndex <= fileScrollOffset) fileScrollOffset = rightFileIndex - 1;
          }
          if (k == 59 && rightFileIndex < fileCount) { 
              rightFileIndex++;
              
              if (rightFileIndex > fileScrollOffset + maxVisibleItems) {
                  fileScrollOffset = rightFileIndex - maxVisibleItems;
              }
          }
          if (fileScrollOffset < 0) fileScrollOffset = 0;
          
          if (real == '\n' || k == 40) {
              if (fileCount > 0 && rightFileIndex <= fileCount) {
                  currentFileName = files[rightFileIndex].name;
                  loadFile();
                  saveSystemSettings();
                  currentMode = TYPING_MODE;
                  needUpdate = true;
                  continue;
              }
          }
          if (real == '\b') {
              isDeletingFile = true;
              needUpdate = true;
          }
        } continue;
    }
     else {
      
      if (k == 57 || k == 58 || k == 59 || k == 60) {
          flushKorean();
      }
      if (k == 58) { int ls = fullText.lastIndexOf('\n', cursorPos - 1); if (ls != -1) { int lineStart = fullText.lastIndexOf('\n', ls - 1) + 1; int col = utf8ColumnBetween(fullText, ls + 1, cursorPos); cursorPos = utf8OffsetForColumn(fullText, lineStart, ls, col); needUpdate = true; } continue; } 
      if (k == 59) { 
        int ns = fullText.indexOf('\n', cursorPos); 
        if (ns != -1) { 
          int ne = fullText.indexOf('\n', ns + 1); 
          if (ne == -1) ne = fullText.length(); 
          int ls = fullText.lastIndexOf('\n', cursorPos - 1); 
          int col = utf8ColumnBetween(fullText, ls + 1, cursorPos); 
          cursorPos = utf8OffsetForColumn(fullText, ns + 1, ne, col); 
          } 
          else if (fullText.indexOf('\n', cursorPos) == -1) {
              cursorPos = fullText.length();
          }

          needUpdate = true;
          continue; 
          }
      if (k == 57) {
        if (rtlTextMode) {
          if (cursorPos < fullText.length()) { cursorPos = utf8NextStart(fullText, cursorPos); needUpdate = true; }
        } else if (cursorPos > 0) { cursorPos = utf8PrevStart(fullText, cursorPos); needUpdate = true; }
        continue;
      }
      if (k == 60) {
        if (rtlTextMode) {
          if (cursorPos > 0) { cursorPos = utf8PrevStart(fullText, cursorPos); needUpdate = true; }
        } else if (cursorPos < fullText.length()) { cursorPos = utf8NextStart(fullText, cursorPos); needUpdate = true; }
        continue;
      }
      if (real == '\b') doBackspace();
      else if (real != 0 || k == 56) { 
        String insertedForAccent = "";
        if (isCtrlPressed && real == ' ') { 
            flushKorean(); 
            isKoreanMode = !isKoreanMode; 
            isCapsLockOn = false;   
            isShiftPressed = false; 
            statusBarNeedsUpdate = true;
            continue; 
        } 
        if (real == ' ' || real == '\n' || real == '\t') { 
            flushKorean();
            
            if (currentMode == SEARCH_MODE) {
                if (real == '\t') { 
                    currentMode = TYPING_MODE; 
                    needUpdate = true; 
                    continue; 
                }
                if (real == '\n') { 
                    int found = (searchQuery.length() > 0) ? fullText.indexOf(searchQuery, cursorPos) : -1;
                    if (found == -1 && searchQuery.length() > 0) found = fullText.indexOf(searchQuery, 0); 
                    if (found != -1) {
                        cursorPos = found + searchQuery.length();
                        searchMatchEnd = cursorPos;
                    } else {
                        searchMatchEnd = -1;
                    }
                    needUpdate = true; 
                    continue;
                }
            }
            insertedForAccent = (real == '\t') ? "" : String(real);
            insertText((real == '\t') ? "    " : String(real));
        } 
        else if (isKoreanMode && getSelectedKeyboardLayoutId() == KB_KOREAN) {
          processKoreanInput(k, real, isShiftPressed, engMap, shiftMap, sizeof(engMap));
        } else {
            
            String insertStr = getKeyboardMappedInput(k, isShiftPressed, isAltPressed, isCapsLockOn && !isKoreanMode, engMap, shiftMap, sizeof(engMap), real);
            if (insertStr != "") {
                String processedStr = (currentMode == SEARCH_MODE) ? insertStr : keyEngineProcessMappedText(getSelectedKeyEngine(), insertStr);
                insertedForAccent = processedStr;
                insertText(processedStr);
                if (currentMode != SEARCH_MODE) keyEngineAfterEdit(getSelectedKeyEngine());
            }
        }

        
        
        KeyboardLayoutId activeLayout = getSelectedKeyboardLayoutId();
        char accentBase = getAccentBaseFromInserted(insertedForAccent);
        if ((activeLayout == KB_QWERTY || activeLayout == KB_DVORAK) && accentBase != 0) {
            lastTypingTime = millis();
            lastBaseChar = accentBase;
            accentCycleIdx = 0;
        } else if (real != 0 && real != ' ' && real != '\n' && real != '\t') {
            lastBaseChar = 0;
        }
      }
      
      charCounter++;
      yield();
      if (forceImmediateRender && needUpdate) break;
    }
  } 

  if (needUpdate) {
    displayIoBusy = true;
#if IZE_EXPERIMENTAL_DIRTY_TILE_REFRESH
    bool dirtyTailRefreshCandidate = false;
#endif
    rtlTextMode = isKoreanMode && keyboardLayoutIsRightToLeft(getSelectedKeyboardLayoutId());
    bool modeChanged = (currentMode != lastMode || currentNetSubMode != lastNetSubMode);
    unsigned long countNowMs = millis();
    bool inputQueueIdle = (Serial.available() == 0);
    bool countDisplayActive = (countMode != 2);
    if (statusBarNeedsUpdate && countDisplayActive && !needCountUpdate && !calcBufferInUse && (modeChanged || ((countNowMs - lastCountRequestMs >= COUNT_UPDATE_INTERVAL_MS) && inputQueueIdle))) {
        // Snapshot only while the keyboard queue is idle and count display is active,
        // so long documents do not pause burst typing when Count is OFF.
        calcBuffer = fullText;
        needCountUpdate = true;
        lastCountRequestMs = countNowMs;
    }
    
    
   
    int menuVisibleStep = baseFontSize + lineSpacing + 8;
    int renderMenuTopY = 100;
    int maxVisibleMenu = ((display.height() / displayScale) - renderMenuTopY - 10) / menuVisibleStep; 
    if (maxVisibleMenu < 1) maxVisibleMenu = 1;
    bool doFullRefresh = false;
    if (currentMode == TYPING_MODE && refreshLimit > 0 && charCounter >= refreshLimit) { doFullRefresh = true; charCounter = 0; }
    bool networkOverlayMode =
        currentMode == WIFI_SCAN_MODE ||
        currentMode == WIFI_PASSWORD_MODE ||
        currentNetSubMode != NET_MAIN ||
        updateState == UPD_PIN_INPUT ||
        updateState == UPD_SD_RUNNING ||
        updateState == UPD_WIFI_WAITING ||
        isUpdating;

    if (modeChanged && !networkOverlayMode) {
        display.fillRect(0, 0, display.width(), display.height(), WHITE);
        lastSy = -1;
        lastCursorY = -1;
    }

    if (currentMode == WIFI_SCAN_MODE) {
        display.clearDisplay();
        u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
        printCleanText(u8g2_for_adafruit_gfx, "=== WIFI SCAN ===", MARGIN_X, MARGIN_Y, true);
        printCleanText(u8g2_for_adafruit_gfx, wifiStatusMessage, MARGIN_X, MARGIN_Y + 30);
        int y = MARGIN_Y + 70;
        for (int i = 0; i < wifiScanCount; i++) {
            String row = String(i + 1) + ". " + wifiScanSsids[i] + " (" + String(wifiScanRssi[i]) + "dBm)";
            printMenuEntry(row, MARGIN_X, y + i * 26, wifiScanSelected == i, false);
        }
        printCleanText(u8g2_for_adafruit_gfx, "Enter: connect / Tab or Menu: cancel", MARGIN_X, (display.height() / displayScale) - 35);
    }
    else if (currentMode == WIFI_PASSWORD_MODE) {
        display.clearDisplay();
        u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
        printCleanText(u8g2_for_adafruit_gfx, "=== WIFI PASSWORD ===", MARGIN_X, MARGIN_Y, true);
        String selected = (wifiScanSelected >= 0 && wifiScanSelected < wifiScanCount) ? wifiScanSsids[wifiScanSelected] : "";
        printCleanText(u8g2_for_adafruit_gfx, "SSID: " + selected, MARGIN_X, MARGIN_Y + 35);
        printCleanText(u8g2_for_adafruit_gfx, wifiStatusMessage, MARGIN_X, MARGIN_Y + 65);
        String masked = "";
        for (int i = 0; i < wifiPasswordBuffer.length(); i++) masked += "*";
        printCleanText(u8g2_for_adafruit_gfx, "Password: " + masked + "_", MARGIN_X, MARGIN_Y + 105);
        printCleanText(u8g2_for_adafruit_gfx, "Enter: connect / Tab or Menu: cancel", MARGIN_X, (display.height() / displayScale) - 35);
    }
    else if (currentNetSubMode != NET_MAIN) {
        int statusBarBottom = (int)(45 * displayScale);
        display.fillRect(0, statusBarBottom, display.width(), display.height() - statusBarBottom, WHITE);

        u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
        int infoY = statusBarBottom + 40;
        if (currentNetSubMode == NET_WIFI_STA) {
            printCleanText(u8g2_for_adafruit_gfx, "Wi-Fi Web Server", MARGIN_X, infoY);
            printCleanText(u8g2_for_adafruit_gfx, "Open: http://" + WiFi.localIP().toString() + "/", MARGIN_X, infoY + 25);
            printCleanText(u8g2_for_adafruit_gfx, "PIN: " + otaPinCode, MARGIN_X, infoY + 50);
            printCleanText(u8g2_for_adafruit_gfx, "GitHub not connected", MARGIN_X, infoY + 75);
            printCleanText(u8g2_for_adafruit_gfx, "EXIT: Ctrl + Menu", MARGIN_X, infoY + 100);
        } else {
            printCleanText(u8g2_for_adafruit_gfx, "Web Server", MARGIN_X, infoY);
            printCleanText(u8g2_for_adafruit_gfx, "EXIT: Ctrl + Menu", MARGIN_X, infoY + 25);
            if (!apPasswordHidden) printCleanText(u8g2_for_adafruit_gfx, "IZEcompose " + activeApPassword + ", 192.168.4.1", MARGIN_X, infoY + 50);
            else printCleanText(u8g2_for_adafruit_gfx, "IZEcompose connected, 192.168.4.1", MARGIN_X, infoY + 50);
            printCleanText(u8g2_for_adafruit_gfx, "PIN: " + otaPinCode, MARGIN_X, infoY + 75);
        }
    }
    else if (currentMode == FILE_MENU_MODE) {
        display.clearDisplay(); lastSy = -1; 
        u8g2_for_adafruit_gfx.setForegroundColor(BLACK); 
        u8g2_for_adafruit_gfx.setBackgroundColor(WHITE); 
        printDualFont("Ize Compose", 50, 28, true);
        printDualFont(getCurrentLanguageMenuLabel(), 50, 58, true);
       
        String m_main = "Sync,New,Save,Count,Sleep,Network";
        int menuCount = 6;
        if (leftMenuIndex >= menuCount) leftMenuIndex = menuCount - 1;
        int maxOffset = menuCount - maxVisibleMenu;
        if (maxOffset < 0) maxOffset = 0;
        if (leftMenuOffset > maxOffset) leftMenuOffset = maxOffset;
        String targetM = m_main;

        int menuStep = baseFontSize + lineSpacing + 8;
        int menuY = 100;
        for (int i = leftMenuOffset; i < menuCount; i++) {
            if (i >= leftMenuOffset + maxVisibleMenu) break;

            String lbl = getValue(targetM, ',', i);
            if (lbl == "") continue;

            if (i == 3) {
                if (countMode == 0) lbl += " [Chars]";
                else if (countMode == 1) lbl += " [Words]";
                else lbl += " [OFF]";
            }
            if (i == 5) {
                String netStr = getNetworkModeLabel(tempNetCursor);
                if (isEditingValue && leftMenuIndex == i) lbl += " < " + netStr + " >";
                else lbl += "   " + netStr + "   ";
            }

            printMenuEntry(lbl, 25, menuY, (menuFocusSide == 0 && leftMenuIndex == i), false);
            menuY += menuStep;
        }

        u8g2_for_adafruit_gfx.setForegroundColor(BLACK);
        u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
        u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
        printCleanText(u8g2_for_adafruit_gfx, String(FIRMWARE_VERSION), 25, (display.height() / displayScale) - 20);


        printDualFont("=== DOCUMENTS ===", 245, 30, true);

        const int maxVisibleItems = FILE_MENU_ITEMS_PER_PAGE;
        for (int f=1; f<=fileCount; f++) {
            if (f > fileScrollOffset && f <= fileScrollOffset + maxVisibleItems) {
                String docLabel = "";
                if (isDeletingFile && menuFocusSide == 1 && rightFileIndex == f) {
                    docLabel = deleteCodePromptActive ? "Delete code required" : "Delete? Enter: code / Tab: cancel";
                } else {
                    docLabel = String(f) + ". " + files[f].name + " [" + String(files[f].sizeKB, 1) + "KB] | " + utf8Truncate(files[f].preview, 12);
                }
                printMenuEntry(docLabel, 225, 60 + ((f-fileScrollOffset-1)*(baseFontSize + lineSpacing + 6)), (menuFocusSide == 1 && rightFileIndex == f), true);
            }
        }
        drawDeleteCodePopup();
    }
    else {
        int statusBarBottom = (int)(45 * displayScale);
        int contentRight = (int)((display.width() / displayScale) - RIGHT_EDGE_MARGIN);
        int maxWidth = contentRight - MARGIN_X;
        bool canReuseTailWrap = false;
        int tailStart = fullText.lastIndexOf('\n');
        tailStart = (tailStart < 0) ? 0 : tailStart + 1;

        if (currentMode == TYPING_MODE &&
            cursorPos == fullText.length() &&
            lastRenderedCursorPos == lastRenderedTextLen &&
            lastRenderedTextLen >= 0 &&
            fullText.length() > lastRenderedTextLen &&
            tailStart == lastRenderedTailStart &&
            rtlTextMode == lastRenderedTailRtl &&
            !forceSafeFullTextRedraw &&
            !doFullRefresh && !modeChanged) {
            bool appendedHasNewline = false;
            for (int p = lastRenderedTextLen; p < fullText.length(); p++) {
                if (fullText[p] == '\n') { appendedHasNewline = true; break; }
            }
            if (!appendedHasNewline) {
                WrapMetrics appendedMetrics = getWrappedMetricsInRange(fullText, lastRenderedTextLen, fullText.length(), maxWidth);
                canReuseTailWrap = (appendedMetrics.lineCount == 1 &&
                                    lastRenderedTailLineWidth + appendedMetrics.lastLineWidth <= maxWidth);
            }
        }
        String d = fullText;
        String composing = "";
        if (currentMode == TYPING_MODE && (cho != -1 || jung != -1)) {
            composing = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
        }
        if (!canReuseTailWrap) adjustViewBottom();    
        
        int targetIdx = cursorPos + composing.length();
        d = d.substring(0, cursorPos) + composing + d.substring(cursorPos);

        
        int renderEnd = viewBottomIdx;
        if (currentMode == TYPING_MODE && cursorPos <= viewBottomIdx) renderEnd += composing.length();
        if (renderEnd > d.length()) renderEnd = d.length();
        // Preserve the remainder of a visual line when the viewport boundary
        // lands in the middle of it during cursor navigation.
        if (renderEnd < d.length() && d[renderEnd] != '\n') {
            int paraStart = renderEnd;
            while (paraStart > 0 && d[paraStart - 1] != '\n') paraStart--;

            int lineStart = paraStart;
            int lineWidth = 0;
            for (int k = paraStart; k < d.length() && d[k] != '\n'; ) {
                int l = zwUtf8CharLen(d, k);
                if (l <= 0) l = 1;
                uint32_t cp = zwUtf8Codepoint(d.c_str() + k, l);
                int charWidth = zwGlyphAdvance(cp, l, false);

                if (lineWidth + charWidth > maxWidth && k > lineStart) {
                    if (renderEnd < k) {
                        renderEnd = k;
                        break;
                    }
                    lineStart = k;
                    lineWidth = 0;
                }
                lineWidth += charWidth;
                k += l;
                if (k >= d.length() || d[k] == '\n') {
                    renderEnd = k;
                    break;
                }
            }
        }

        if (!canReuseTailWrap) {
            tailStart = d.lastIndexOf('\n');
            tailStart = (tailStart < 0) ? 0 : tailStart + 1;
        }
        int tailLineCount = canReuseTailWrap ? lastRenderedTailLineCount : countWrappedLinesInRange(d, tailStart, d.length(), maxWidth);
        bool fastTailRender = (currentMode == TYPING_MODE &&
                               composing.length() == 0 &&
                               cursorPos == fullText.length() &&
                               lastRenderedCursorPos == lastRenderedTextLen &&
                               lastRenderedTextLen >= 0 &&
                               fullText.length() > lastRenderedTextLen &&
                               canReuseTailWrap &&
                               tailStart == lastRenderedTailStart &&
                               tailLineCount == lastRenderedTailLineCount &&
                               rtlTextMode == lastRenderedTailRtl &&
                               !forceSafeFullTextRedraw &&
                               !doFullRefresh && !modeChanged);

        int currentY = (int)((display.height() / displayScale) - 25);
        if (fastTailRender) {
            // Bottom anchored display: with no wrap, only the lowest visible line changed.
            int clearTop = (int)((currentY - baseFontSize - lineSpacing + 2) * displayScale);
            if (clearTop < statusBarBottom) clearTop = statusBarBottom;
            int clearBottom = (int)((currentY + 6) * displayScale);
            if (clearBottom > display.height()) clearBottom = display.height();
            int clearHeight = clearBottom - clearTop;
            int minTailClearHeight = (int)(12 * displayScale);
            if (clearHeight < minTailClearHeight) clearHeight = minTailClearHeight;
            if (clearTop + clearHeight > display.height()) clearHeight = display.height() - clearTop;
#if IZE_EXPERIMENTAL_DIRTY_TILE_REFRESH
            dirtyTailRefreshCandidate = izeCaptureDirtyTileCandidate(
                dirtyTailCandidate,
                0,
                clearTop,
                display.width(),
                clearHeight
            );
#endif
            display.fillRect(0, clearTop, display.width(), clearHeight, WHITE);
        } else {
#if IZE_EXPERIMENTAL_DIRTY_TILE_REFRESH
            izeReleaseDirtyTileCandidate(dirtyTailCandidate);
#endif
            // Enter or wrapping shifts lines above the cursor, so redraw safely.
            display.fillRect(0, statusBarBottom, display.width(), display.height() - statusBarBottom, WHITE);
        }

        bool fastTailDone = false;
        int lastLineEnd = renderEnd;
        for (int i = renderEnd; i >= 0; i--) {
            if (i == 0 || d[i-1] == '\n') {
                String para = d.substring(i, lastLineEnd);
                String lines[40]; 
                int lineStartIdx[40]; 
                int lineCount = 0;
                int currentLineWidth = 0; 
                int currentLineStartK = i; 

                u8g2_for_adafruit_gfx.setFont(Typewriter_16px);
                
                if (i >= lastLineEnd) { 
                    lines[0] = "";
                    lineStartIdx[0] = 0;
                    lineCount = 1;
                } else {
                    
                    for(int k = i; k < lastLineEnd; ) {
                        int l = zwUtf8CharLen(d, k);
                        if (l <= 0) l = 1;
                        uint32_t cp = zwUtf8Codepoint(d.c_str() + k, l);
                        int charWidth = zwGlyphAdvance(cp, l, false);

                        if (currentLineWidth + charWidth > maxWidth) {
                            if (lineCount < 40) {
                                
                                lines[lineCount] = d.substring(currentLineStartK, k);
                                lineStartIdx[lineCount] = currentLineStartK - i;
                                lineCount++;
                            } else {
                                for (int si = 0; si < 39; si++) {
                                    lines[si] = lines[si + 1];
                                    lineStartIdx[si] = lineStartIdx[si + 1];
                                }
                                lines[39] = d.substring(currentLineStartK, k);
                                lineStartIdx[39] = currentLineStartK - i;
                            }
                            currentLineStartK = k; 
                            currentLineWidth = charWidth; 
                        } else {
                            currentLineWidth += charWidth;
                        }
                        k += l;
                    }
                    
                    if (currentLineStartK < lastLineEnd && lineCount < 40) {
                        lines[lineCount] = d.substring(currentLineStartK, lastLineEnd);
                        lineStartIdx[lineCount] = currentLineStartK - i;
                        lineCount++;
                    } else if (currentLineStartK < lastLineEnd) {
                        for (int si = 0; si < 39; si++) {
                            lines[si] = lines[si + 1];
                            lineStartIdx[si] = lineStartIdx[si + 1];
                        }
                        lines[39] = d.substring(currentLineStartK, lastLineEnd);
                        lineStartIdx[39] = currentLineStartK - i;
                    }
                }
                for (int j = lineCount - 1; j >= 0; j--) {
                    if (currentY > STATUS_Y + 20) {
                        String displayLine = rtlTextMode ? makeRtlVisualText(lines[j]) : lines[j];
                        int lineDrawX = MARGIN_X;
                        if (rtlTextMode) {
                            lineDrawX = contentRight - zwMeasureTextWidth(displayLine, false);
                            if (lineDrawX < MARGIN_X) lineDrawX = MARGIN_X;
                        }
                        printCleanText(u8g2_for_adafruit_gfx, displayLine, lineDrawX, currentY, false, contentRight);
                        
                        int absLineStart = i + lineStartIdx[j];
                        int absLineEnd = (j < lineCount - 1) ? (i + lineStartIdx[j+1]) : lastLineEnd;
                        
                        if (targetIdx >= absLineStart && targetIdx <= absLineEnd) {
                            if (targetIdx < absLineEnd || j == lineCount - 1) {
                                int cursorStrLen = targetIdx - absLineStart;
                                String beforeCursor = para.substring(lineStartIdx[j], lineStartIdx[j] + cursorStrLen);
                                
                                String displayBeforeCursor = rtlTextMode ? makeRtlVisualText(beforeCursor) : beforeCursor;
                                int cursorXOffset = zwMeasureTextWidth(displayBeforeCursor, false);
                                
                                if (currentMode == TYPING_MODE) {
                                    int cursorDrawX = rtlTextMode ? (contentRight - cursorXOffset - 12) : (lineDrawX + cursorXOffset);
                                    if (cursorDrawX < MARGIN_X) cursorDrawX = MARGIN_X;
                                    if (cursorDrawX > contentRight - 12) cursorDrawX = contentRight - 12;
                                    bigDisplay.fillRect(cursorDrawX, currentY + 2, 12, 4, BLACK);
                                    lastCursorY = currentY; // Used by the next fast render frame.
                                }
                                else if (currentMode == SEARCH_MODE && searchQuery.length() > 0 && searchMatchEnd >= 0) {
                                    int searchStart = searchMatchEnd - searchQuery.length();
                                    
                                    
                                    if (searchStart >= absLineStart && searchMatchEnd <= absLineEnd && searchStart >= 0 &&
                                        fullText.substring(searchStart, searchMatchEnd) == searchQuery) {
                                        
                                        
                                        int startStrLen = searchStart - absLineStart;
                                        String beforeSearch = para.substring(lineStartIdx[j], lineStartIdx[j] + startStrLen);
                                        String displayBeforeSearch = rtlTextMode ? makeRtlVisualText(beforeSearch) : beforeSearch;
                                        int searchXOffset = zwMeasureTextWidth(displayBeforeSearch, false);

                                        
                                        String displayQuery = rtlTextMode ? makeRtlVisualText(searchQuery) : searchQuery;
                                        int queryWidth = zwMeasureTextWidth(displayQuery, false);

                                        
                                        int searchDrawX = rtlTextMode ? (contentRight - searchXOffset - queryWidth) : (lineDrawX + searchXOffset);
                                        if (searchDrawX < MARGIN_X) searchDrawX = MARGIN_X;
                                        if (searchDrawX + queryWidth > contentRight) searchDrawX = contentRight - queryWidth;
                                        if (searchDrawX < MARGIN_X) searchDrawX = MARGIN_X;
                                        bigDisplay.fillRect(searchDrawX, currentY - 14, queryWidth, 18, BLACK);

                                        
                                        u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
                                        u8g2_for_adafruit_gfx.setBackgroundColor(BLACK);
                                        
                                        printDualFont(displayQuery, searchDrawX, currentY);

                                        
                                        u8g2_for_adafruit_gfx.setForegroundColor(BLACK);
                                        u8g2_for_adafruit_gfx.setBackgroundColor(WHITE);
                                    }
                                }
                            }
                        }
                    }
                    currentY -= (baseFontSize + lineSpacing);
                    if (fastTailRender) {
                        fastTailDone = true;
                        break;
                    }
                    if (currentY < STATUS_Y + 10) break;
                }

                lastLineEnd = i - 1;
                if (fastTailDone || currentY < STATUS_Y + 10) break; 
            }
        }
        lastSy = 0;

        if (currentMode == TYPING_MODE) {
            lastRenderedTextLen = fullText.length();
            lastRenderedCursorPos = cursorPos;
            lastRenderedTailStart = fullText.lastIndexOf('\n');
            lastRenderedTailStart = (lastRenderedTailStart < 0) ? 0 : lastRenderedTailStart + 1;
            WrapMetrics tailMetrics = getWrappedMetricsInRange(fullText, lastRenderedTailStart, fullText.length(), maxWidth);
            lastRenderedTailLineCount = tailMetrics.lineCount;
            lastRenderedTailLineWidth = tailMetrics.lastLineWidth;
            lastRenderedTailRtl = rtlTextMode;
            forceSafeFullTextRedraw = false;
        } else {
            lastRenderedTextLen = -1;
            lastRenderedCursorPos = -1;
            lastRenderedTailStart = -1;
            lastRenderedTailLineCount = -1;
            lastRenderedTailLineWidth = 0;
            forceSafeFullTextRedraw = false;
        }

        if (currentMode == SEARCH_MODE) {
            int boxW = 360; int boxH = 44;
            int boxX = (display.width() / UI_SCALE - boxW) / 2;
            int boxY = (display.height() / UI_SCALE - boxH) / 2; 
            display.fillRect((int)(boxX * UI_SCALE), (int)(boxY * UI_SCALE), (int)(boxW * UI_SCALE), (int)(boxH * UI_SCALE), BLACK);
            display.fillRect((int)((boxX + 3) * UI_SCALE), (int)((boxY + 3) * UI_SCALE), (int)((boxW - 6) * UI_SCALE), (int)((boxH - 6) * UI_SCALE), WHITE);

            String dispSearch = rtlTextMode ? makeRtlVisualText(searchQuery) + " :Search" : "Search: " + searchQuery;
            if (cho != -1 || jung != -1) {
                String composingSearch = ((cho != -1 && jung != -1) ? makeKorStr(cho, jung, jong) : (cho != -1 ? String(choStrs[cho]) : String(jungStrs[jung])));
                if (rtlTextMode) dispSearch = makeRtlVisualText(composingSearch) + dispSearch;
                else dispSearch += composingSearch;
            }
            dispSearch += "_";
            printCleanText(u8g2_for_adafruit_gfx, dispSearch, boxX + 12, boxY + 30, false, boxX + boxW - 12);

        }
    } 
    
    
    
    
    
    
    
    

    
    bool drawStatusBar = (currentMode == TYPING_MODE || currentMode == SEARCH_MODE) &&
                         (statusBarNeedsUpdate || modeChanged || currentMode == SEARCH_MODE || savedMessageVisible);
#if IZE_EXPERIMENTAL_DIRTY_TILE_REFRESH
    if (drawStatusBar) dirtyTailRefreshCandidate = false;
#endif
    if (drawStatusBar) {
        int hY = 30;
        
        display.fillRect(0, 0, display.width(), (int)(40 * UI_SCALE), WHITE);
        
        
        String modeStr = getStatusLanguageLabel();

        
        String countStr = "";
        if (countMode == 0) { 
            charwordcount = sharedCharCount; 
            countStr = String(charwordcount);
        } else if (countMode == 1) { 
            charwordcount = sharedWordCount; 
            countStr = String(charwordcount);
        }

        int logicalWidth = (int)(display.width() / UI_SCALE);
        int countX = 110;
        int batteryX = logicalWidth - 55;
        int savedX = batteryX - 78;

        printStatusText(modeStr + "  ", 15, hY);

        if (countMode != 2) {
            String countLabel = "";
            if (countMode == 0) countLabel = "Count: ";
            else countLabel = "Words: ";
            printStatusText(countLabel + countStr, countX, hY);
        }

        String titleText = currentFileName;
        int titleMaxChars = 14;
        if (titleMaxChars < 4) titleMaxChars = 4;
        if (titleText.length() > titleMaxChars) titleText = titleText.substring(0, titleMaxChars - 2) + "..";
        int titleWidth = getTrueLength(titleText) * 8;
        int titleX = (logicalWidth - titleWidth) / 2;
        int minTitleX = countX + 95;
        int maxTitleX = savedX - titleWidth - 8;
        if (titleX < minTitleX) titleX = minTitleX;
        if (titleX > maxTitleX) titleX = maxTitleX;
        printStatusText(titleText, titleX, hY);

        
        if (savedMessageVisible) {
            u8g2_for_adafruit_gfx.setForegroundColor(BLACK);
            printStatusText("[Saved!]", savedX, hY);
        }

        
        unsigned long nowStatusMs = millis();
        if (!statusBatteryValid || nowStatusMs - lastStatusBatteryFetchMs >= BATTERY_REFRESH_INTERVAL_MS) {
            statusBatteryPct = cachedBatteryPct;
            statusBatteryValid = cachedBatteryValid;
            lastStatusBatteryFetchMs = nowStatusMs;
        }
        printStatusText(statusBatteryValid ? (String(statusBatteryPct) + "%   ") : String("--%   "), batteryX, hY);
        
        display.drawFastHLine(0, (int)(39 * UI_SCALE), display.width(), BLACK);
    }
    
    
    if (CalcTaskHandle) vTaskSuspend(CalcTaskHandle);
    if (doFullRefresh) display.display();
    else {
#if IZE_EXPERIMENTAL_DIRTY_TILE_REFRESH
        bool dirtyRefreshHandled = false;
        if (dirtyTailRefreshCandidate) {
#if IZE_DIRTY_TILE_REFRESH_DEBUG
            if (izeComputeDirtyTiles(dirtyTailCandidate, lastDirtyTailStats)) {
                lastDirtyTailStats.fallbackUsed = false;
                izeDebugDirtyTileStats(lastDirtyTailStats);
            }
#endif
            // Inkplate exposes only full-screen partial update here. Leaving the
            // panel powered between consecutive tail updates saves some latency.
            display.partialUpdate(false, true);
            dirtyRefreshHandled = true;
        } else if (izeComputeDirtyTiles(dirtyTailCandidate, lastDirtyTailStats)) {
            dirtyRefreshHandled = izeTryDirtyTilePanelRefresh(lastDirtyTailStats);
            lastDirtyTailStats.fallbackUsed = !dirtyRefreshHandled;
            izeDebugDirtyTileStats(lastDirtyTailStats);
        }
        izeReleaseDirtyTileCandidate(dirtyTailCandidate);
        if (!dirtyRefreshHandled) display.partialUpdate(false);
#else
        display.partialUpdate(false);
#endif
    }
    if (CalcTaskHandle) vTaskResume(CalcTaskHandle);
    display.resetInternalCounter();
    displayIoBusy = false;

    needUpdate = false;
    statusBarNeedsUpdate = false;
    lastMode = currentMode;
    lastNetSubMode = currentNetSubMode;
  } 
  
  if (currentMode == TYPING_MODE && currentNetSubMode == NET_MAIN && updateState == UPD_NONE && autoSleepIndex != 6) {
    if (!isUpdating && (millis() - lastKeyPress > sleepIntervals[autoSleepIndex])) {
        showInitialImage(); 
        }
    }

    
} 
