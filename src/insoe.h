#ifndef ZEROWRITER_HELPER_H
#define ZEROWRITER_HELPER_H
#include <Arduino.h>
#include "U8g2_for_Adafruit_GFX.h"
extern U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
extern int lineSpacing;
extern int letterSpacing;
extern int typingSpeed;
extern int refreshLimit;
extern int autoSleepIndex;
extern int englishLayoutIndex;
extern int keyboardLayoutIndex;
extern int rightFileIndex;  
extern bool isKoreanMode;
extern bool rtlTextMode;
extern float displayScale;
#include <ESPmDNS.h>
#include <U8g2_for_Adafruit_GFX.h>
extern const uint8_t* font_ptr;
extern const uint8_t* font_latin_ptr;
extern const uint8_t* font_hangul_ptr;
extern const uint8_t* font_jamo_ptr;
extern const uint8_t* font_jp_ptr;
extern const uint8_t* font_greek_cyrillic_ptr;
extern const uint8_t* font_arabic_ptr;
extern const uint8_t* font_indic_ptr;
extern const uint8_t* font_sea_ptr;
extern const uint8_t* font_misc_ptr;
#define Typewriter_16px font_ptr


extern bool needUpdate;

const int MARGIN_X = 35; 
const int MARGIN_Y = 65; 
const int STATUS_Y = 30;
const int RIGHT_EDGE_MARGIN = 35;

uint8_t* fontBuffer = nullptr; 
size_t fontBufferSize = 0;


enum NetworkSubMode { NET_MAIN, NET_USB, NET_BT_SELECT, NET_WIFI };
extern NetworkSubMode currentNetSubMode;



static inline int zwUtf8CharLen(const String& text, int index) {
    int len = text.length();
    if (index < 0 || index >= len) return 0;
    const uint8_t* s = (const uint8_t*)text.c_str();
    uint8_t c = s[index];
    if (c < 0x80) return 1;
    int need = 0;
    if ((c & 0xE0) == 0xC0) {
        if (c < 0xC2) return 1;
        need = 2;
    } else if ((c & 0xF0) == 0xE0) {
        need = 3;
    } else {
        return 1;
    }
    if (index + need > len) return 1;
    for (int i = 1; i < need; i++) {
        if ((s[index + i] & 0xC0) != 0x80) return 1;
    }
    if (need == 3) {
        if (c == 0xE0 && s[index + 1] < 0xA0) return 1;
        if (c == 0xED && s[index + 1] >= 0xA0) return 1;
    }
    return need;
}

static inline uint32_t zwUtf8Codepoint(const char* ptr, int charLen) {
    const uint8_t* s = (const uint8_t*)ptr;
    if (charLen == 1) return s[0];
    if (charLen == 2) return ((uint32_t)(s[0] & 0x1F) << 6) | (s[1] & 0x3F);
    if (charLen == 3) return ((uint32_t)(s[0] & 0x0F) << 12) | ((uint32_t)(s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    return 0xFFFD;
}

static inline bool zwIsDrawableCodepoint(uint32_t cp) {
    if (cp == '\t') return true;
    if (cp >= 0x20 && cp <= 0x7E) return true;
    if (cp >= 0x00A0 && cp <= 0x02FF) return true;
    if (cp >= 0x0370 && cp <= 0x03FF) return true;
    if (cp >= 0x0400 && cp <= 0x052F) return true;
    if (cp >= 0x0530 && cp <= 0x058F) return true;  // Armenian
    if (cp >= 0x0590 && cp <= 0x05FF) return true;  // Hebrew
    if (cp >= 0x0600 && cp <= 0x06FF) return true;  // Arabic
    if (cp >= 0x0750 && cp <= 0x077F) return true;  // Arabic Supplement
    if (cp >= 0x0780 && cp <= 0x07BF) return true;
    if (cp >= 0x0900 && cp <= 0x0D7F) return true;  // Indic (Devanagari~Malayalam)
    if (cp >= 0x0D80 && cp <= 0x0DFF) return true;  // Sinhala
    if (cp >= 0x0E00 && cp <= 0x0E7F) return true;  // Thai
    if (cp >= 0x0E80 && cp <= 0x0EFF) return true;  // Lao
    if (cp >= 0x0F00 && cp <= 0x0FFF) return true;  // Tibetan
    if (cp >= 0x1000 && cp <= 0x109F) return true;  // Myanmar
    if (cp >= 0x10A0 && cp <= 0x10FF) return true;  // Georgian
    if (cp >= 0x1200 && cp <= 0x137F) return true;  // Ethiopic
    if (cp >= 0x1780 && cp <= 0x17FF) return true;  // Khmer
    if (cp >= 0x19E0 && cp <= 0x19FF) return true;
    if (cp >= 0xFB50 && cp <= 0xFDFF) return true;  // Arabic Presentation Forms-A
    if (cp >= 0xFE70 && cp <= 0xFEFF) return true;  // Arabic Presentation Forms-B
    if (cp >= 0x1100 && cp <= 0x11FF) return true;
    if (cp >= 0x13A0 && cp <= 0x13FF) return true;
    if (cp >= 0x1780 && cp <= 0x17FF) return true;
    if (cp >= 0x1E00 && cp <= 0x1EFF) return true;
    if (cp >= 0x1F00 && cp <= 0x1FFF) return true;
    if (cp >= 0x2000 && cp <= 0x206F) return true;
    if (cp >= 0x20A0 && cp <= 0x20CF) return true;
    if (cp >= 0x2100 && cp <= 0x22FF) return true;
    if (cp >= 0x2500 && cp <= 0x257F) return true;
    if (cp >= 0x25A0 && cp <= 0x26FF) return true;
    if (cp >= 0x2C00 && cp <= 0x2C5F) return true;
    if (cp >= 0x2C60 && cp <= 0x2C7F) return true;
    if (cp >= 0x2D00 && cp <= 0x2D2F) return true;
    if (cp >= 0x2D30 && cp <= 0x2D7F) return true;
    if (cp >= 0x3000 && cp <= 0x303F) return true;
    if (cp >= 0x3040 && cp <= 0x30FF) return true;
    if (cp >= 0x31F0 && cp <= 0x31FF) return true;
    if (cp >= 0x3130 && cp <= 0x318F) return true;
    if (cp >= 0xA500 && cp <= 0xA63F) return true;
    if (cp >= 0xA640 && cp <= 0xA69F) return true;
    if (cp >= 0xA720 && cp <= 0xA7FF) return true;
    if (cp >= 0xA960 && cp <= 0xA97F) return true;
    if (cp >= 0xAC00 && cp <= 0xD7A3) return true;
    if (cp >= 0xD7B0 && cp <= 0xD7FF) return true;
    if (cp >= 0xFB00 && cp <= 0xFDFF) return true;
    if (cp >= 0xFE70 && cp <= 0xFEFF) return true;
    if (cp >= 0xFF00 && cp <= 0xFFEF) return true;
    return false;
}

static inline const uint8_t* zwFontForCodepoint(uint32_t cp) {
    if (cp >= 0x20 && cp <= 0x7E) return font_latin_ptr ? font_latin_ptr : font_ptr;
    if (cp >= 0x00A0 && cp <= 0x02FF) return font_latin_ptr ? font_latin_ptr : font_ptr;
    if (cp >= 0x0370 && cp <= 0x03FF) return font_greek_cyrillic_ptr ? font_greek_cyrillic_ptr : font_latin_ptr;
    if (cp >= 0x0400 && cp <= 0x052F) return font_greek_cyrillic_ptr ? font_greek_cyrillic_ptr : font_latin_ptr;
    if (cp >= 0x1F00 && cp <= 0x1FFF) return font_greek_cyrillic_ptr ? font_greek_cyrillic_ptr : font_latin_ptr;
    // 以묐룞
    if (cp >= 0x0530 && cp <= 0x058F) return font_misc_ptr    ? font_misc_ptr    : font_latin_ptr;  // Armenian
    if (cp >= 0x0590 && cp <= 0x05FF) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Hebrew
    if (cp >= 0x0600 && cp <= 0x06FF) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Arabic
    if (cp >= 0x0750 && cp <= 0x077F) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Arabic Supplement
    if (cp >= 0xFB50 && cp <= 0xFDFF) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Arabic Presentation Forms-A
    if (cp >= 0xFE70 && cp <= 0xFEFF) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Arabic Presentation Forms-B
    // ?⑥븘?쒖븘 Indic
    if (cp >= 0x0900 && cp <= 0x0D7F) return font_indic_ptr   ? font_indic_ptr   : font_latin_ptr;
    if (cp >= 0x0D80 && cp <= 0x0DFF) return font_indic_ptr   ? font_indic_ptr   : font_latin_ptr;  // Sinhala
    if (cp >= 0x0E00 && cp <= 0x0E7F) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;
    // ?숇궓?꾩떆??    if (cp >= 0x0E00 && cp <= 0x0E7F) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;  // Thai
    if (cp >= 0x0E80 && cp <= 0x0EFF) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;  // Lao
    if (cp >= 0x1000 && cp <= 0x109F) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;  // Myanmar
    if (cp >= 0x1780 && cp <= 0x17FF) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;  // Khmer
    if (cp >= 0x19E0 && cp <= 0x19FF) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;
    // 湲고?
    if (cp >= 0x0F00 && cp <= 0x0FFF) return font_misc_ptr    ? font_misc_ptr    : font_latin_ptr;  // Tibetan
    if (cp >= 0x10A0 && cp <= 0x10FF) return font_misc_ptr    ? font_misc_ptr    : font_latin_ptr;  // Georgian
    if (cp >= 0x1200 && cp <= 0x137F) return font_misc_ptr    ? font_misc_ptr    : font_latin_ptr;  // Ethiopic
    if (cp >= 0x1E00 && cp <= 0x22FF) return font_latin_ptr ? font_latin_ptr : font_ptr;
    if (cp >= 0x2500 && cp <= 0x26FF) return font_latin_ptr ? font_latin_ptr : font_ptr;
    if (cp >= 0xAC00 && cp <= 0xD7A3) return font_hangul_ptr ? font_hangul_ptr : font_ptr;
    if (cp >= 0x1100 && cp <= 0x11FF) return font_jamo_ptr ? font_jamo_ptr : font_ptr;
    if (cp >= 0x3130 && cp <= 0x318F) return font_jamo_ptr ? font_jamo_ptr : font_ptr;
    if (cp >= 0xA960 && cp <= 0xA97F) return font_jamo_ptr ? font_jamo_ptr : font_ptr;
    if (cp >= 0xD7B0 && cp <= 0xD7FF) return font_jamo_ptr ? font_jamo_ptr : font_ptr;
    if (cp >= 0x3000 && cp <= 0x30FF) return font_jp_ptr ? font_jp_ptr : font_ptr;
    if (cp >= 0x31F0 && cp <= 0x31FF) return font_jp_ptr ? font_jp_ptr : font_ptr;
    if (cp >= 0xFF00 && cp <= 0xFFEF) return font_jp_ptr ? font_jp_ptr : font_ptr;
    if (font_latin_ptr != nullptr) return font_latin_ptr;
    return font_ptr;
}

static inline int zwGlyphAdvance(uint32_t cp, int charLen, bool isMenu) {
    if (cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0xFEFF) return 0;
    if ((cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x0591 && cp <= 0x05BD) || cp == 0x05BF || (cp >= 0x05C1 && cp <= 0x05C2) || (cp >= 0x05C4 && cp <= 0x05C5) || cp == 0x05C7) return 0;
    if ((cp >= 0x064B && cp <= 0x065F) || cp == 0x0670 || (cp >= 0x06D6 && cp <= 0x06ED)) return 0;
    if ((cp >= 0x0900 && cp <= 0x0D7F) && (((cp & 0x7F) <= 0x03) || ((cp & 0x7F) == 0x3C) || ((cp & 0x7F) >= 0x3E && (cp & 0x7F) <= 0x4D) || ((cp & 0x7F) >= 0x55 && (cp & 0x7F) <= 0x57) || ((cp & 0x7F) >= 0x62 && (cp & 0x7F) <= 0x63))) return 0;
    if (cp == 0x0DCA || (cp >= 0x0DCF && cp <= 0x0DDF) || cp == 0x0DF2 || cp == 0x0DF3) return 0;
    if (cp == 0x0E31 || (cp >= 0x0E34 && cp <= 0x0E3A) || (cp >= 0x0E47 && cp <= 0x0E4E)) return 0;
    if (cp == 0x0EB1 || (cp >= 0x0EB4 && cp <= 0x0EBC) || (cp >= 0x0EC8 && cp <= 0x0ECD)) return 0;
    if ((cp >= 0x0F71 && cp <= 0x0F87) || (cp >= 0x0F90 && cp <= 0x0FBC)) return 0;
    if ((cp >= 0x102B && cp <= 0x103E) || (cp >= 0x1056 && cp <= 0x1059) || (cp >= 0x105E && cp <= 0x1060) || (cp >= 0x1062 && cp <= 0x1064) || (cp >= 0x1067 && cp <= 0x106D) || (cp >= 0x1071 && cp <= 0x1074) || (cp >= 0x1082 && cp <= 0x108D) || cp == 0x108F) return 0;
    if ((cp >= 0x17B6 && cp <= 0x17D3) || cp == 0x17DD) return 0;
    int width = (charLen == 1 || cp < 0x3000) ? 8 : 16;
    return width + (isMenu ? 1 : letterSpacing);
}

static inline int zwGlyphDrawXOffset(uint32_t cp) {
    if (zwGlyphAdvance(cp, 3, false) == 0) return -8;
    return 0;
}

static inline bool zwGlyphVisible(uint32_t cp) {
    return !(cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0xFEFF);
}

static inline int zwGlyphDrawYOffset(uint32_t cp) {
    if (cp >= 0xAC00 && cp <= 0xD7A3) return 3;
    if (cp >= 0x1100 && cp <= 0x11FF) return 3;
    if (cp >= 0x3130 && cp <= 0x318F) return 3;
    if (cp >= 0xA960 && cp <= 0xA97F) return 3;
    if (cp >= 0xD7B0 && cp <= 0xD7FF) return 3;
    return 0;
}

static inline int zwMeasureTextWidth(const String& text, bool isMenu = false) {
    int width = 0;
    const char* ptr = text.c_str();
    for (int i = 0; i < text.length(); ) {
        int l = zwUtf8CharLen(text, i);
        if (l <= 0) break;
        uint32_t cp = zwUtf8Codepoint(ptr + i, l);
        width += zwGlyphAdvance(cp, l, isMenu);
        i += l;
    }
    return width;
}

void printCleanText(U8G2_FOR_ADAFRUIT_GFX &u8g2, const String& text, int x, int y, bool isMenu = false, int maxWidth = 800) {
    int cx = x;
    u8g2.setFontMode(0);
    if (font_ptr != nullptr) {
        u8g2.setFont(font_ptr);
    } else {
        return;
    }
    const char* ptr = text.c_str();
    const uint8_t* activeFont = font_ptr;
    for (int i = 0; i < text.length(); ) {
        if (cx > maxWidth) break;
        int l = zwUtf8CharLen(text, i);
        if (l <= 0) break;
        uint32_t cp = zwUtf8Codepoint(ptr + i, l);
        char fallback[2] = {'?', 0};
        char buf[5] = {0};
        const char* glyph = fallback;
        if (zwIsDrawableCodepoint(cp)) {
            for (int m = 0; m < l; m++) buf[m] = ptr[i + m];
            glyph = buf;
        } else {
            l = 1;
        }
        const uint8_t* selectedFont = zwFontForCodepoint(cp);
        if (selectedFont != nullptr && selectedFont != activeFont) {
            u8g2.setFont(selectedFont);
            activeFont = selectedFont;
        }
        int adv = zwGlyphAdvance(cp, l, isMenu);
        // Prevent the last glyph from spilling into the next UI column.
        // This matters most in FILE_MENU_MODE where the left menu and document list sit side by side.
        if (cx + adv > maxWidth) break;
        if (zwGlyphVisible(cp)) u8g2.drawUTF8(cx + zwGlyphDrawXOffset(cp), y + zwGlyphDrawYOffset(cp), glyph);
        cx += adv;
        i += l;
    }
}
void drawNetworkUI(Inkplate &display, U8G2_FOR_ADAFRUIT_GFX &u8g2, float scale, int selectedIdx) {
    
    display.fillRect(0, 0, display.width(), display.height(), WHITE);
    
    printCleanText(u8g2, "=== NETWORK MODE ===", MARGIN_X, MARGIN_Y, true);
    
    String options[] = {"1. USB (SD Reader Mode)", "2. Bluetooth (HID/Send)", "3. Wi-Fi (Email Send)"};
    for(int i=0; i<3; i++) {
        int ty = MARGIN_Y + 40 + (i * 30);
        if (i == selectedIdx) {
            
            display.fillRect((int)((MARGIN_X-4)*scale), (int)((ty-16)*scale), (int)(300*scale), (int)(22*scale), BLACK);
            u8g2.setForegroundColor(WHITE); u8g2.setBackgroundColor(BLACK);
            printCleanText(u8g2, options[i], MARGIN_X, ty, true);
            u8g2.setForegroundColor(BLACK); u8g2.setBackgroundColor(WHITE);
        } else {
            printCleanText(u8g2, options[i], MARGIN_X, ty, true);
        }
    }
    
    display.display();
}

#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);


const char* ssid = "IZEcompose_FileServer";
const char* password = "00009888";


extern bool webServerUpdateOnly;

bool isDocFilename(const String& fn) {
    if (!fn.startsWith("doc_") || !fn.endsWith(".txt")) return false;
    if (fn.indexOf('/') >= 0 || fn.indexOf('\\') >= 0) return false;
    if (fn.length() <= 8) return false;
    for (int i = 4; i < fn.length() - 4; i++) {
        if (!isDigit(fn[i])) return false;
    }
    return true;
}

int docNumberFromName(const String& fn) {
    if (!isDocFilename(fn)) return 0;
    int value = 0;
    for (int i = 4; i < fn.length() - 4; i++) {
        value = value * 10 + (fn[i] - '0');
    }
    return value;
}

String nextDocFilename() {
    SdFile root;
    SdFile file;
    char name[64];
    int maxNum = 0;

    if (!root.open("/", O_RDONLY)) return "doc_1.txt";
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
    return "doc_" + String(maxNum + 1) + ".txt";
}

String htmlEscape(const String& src) {
    String out = src;
    out.replace("&", "&amp;");
    out.replace("<", "&lt;");
    out.replace(">", "&gt;");
    out.replace("\"", "&quot;");
    return out;
}

String urlEncode(const String& src) {
    const char* hex = "0123456789ABCDEF";
    String out = "";
    for (int i = 0; i < src.length(); i++) {
        unsigned char c = (unsigned char)src[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else {
            out += '%';
            out += hex[(c >> 4) & 0x0F];
            out += hex[c & 0x0F];
        }
    }
    return out;
}

void appendDocList(String& html) {
    static const int WEB_DOCS_PER_PAGE = 12;
    static const int WEB_MAX_DOCUMENT_FILES = 256;
    SdFile root;
    SdFile file;
    char name[64];
    // Keep these off the loop/web-handler stack. 256 String objects plus
    // metadata is large enough to cause sporadic stack pressure on ESP32.
    static int docNums[WEB_MAX_DOCUMENT_FILES];
    static uint32_t docSizes[WEB_MAX_DOCUMENT_FILES];
    static String docNames[WEB_MAX_DOCUMENT_FILES];
    int totalDocs = 0;
    int page = server.hasArg("page") ? server.arg("page").toInt() : 1;
    if (page < 1) page = 1;

    if (!root.open("/", O_RDONLY)) {
        html += "<p class=\"empty\">SD open failed.</p>";
        return;
    }

    while (file.openNext(&root, O_RDONLY)) {
        file.getName(name, sizeof(name));
        String fn = String(name);
        if (!file.isDir() && isDocFilename(fn) && totalDocs < WEB_MAX_DOCUMENT_FILES) {
            docNums[totalDocs] = docNumberFromName(fn);
            docSizes[totalDocs] = file.fileSize();
            docNames[totalDocs] = fn;
            totalDocs++;
        }
        file.close();
        yield();
    }
    root.close();

    for (int i = 0; i < totalDocs - 1; i++) {
        for (int j = i + 1; j < totalDocs; j++) {
            if (docNums[i] < docNums[j]) {
                int tn = docNums[i]; docNums[i] = docNums[j]; docNums[j] = tn;
                uint32_t ts = docSizes[i]; docSizes[i] = docSizes[j]; docSizes[j] = ts;
                String tf = docNames[i]; docNames[i] = docNames[j]; docNames[j] = tf;
            }
        }
    }

    int start = (page - 1) * WEB_DOCS_PER_PAGE;
    int end = start + WEB_DOCS_PER_PAGE;
    if (start >= totalDocs && totalDocs > 0) {
        page = ((totalDocs - 1) / WEB_DOCS_PER_PAGE) + 1;
        start = (page - 1) * WEB_DOCS_PER_PAGE;
        end = start + WEB_DOCS_PER_PAGE;
    }
    if (end > totalDocs) end = totalDocs;

    html += "<table><thead><tr><th>File</th><th>Size</th><th>Actions</th></tr></thead><tbody>";
    for (int i = start; i < end; i++) {
        String fn = docNames[i];
        String safeName = htmlEscape(fn);
        String urlName = urlEncode(fn);
        html += "<tr><td>" + safeName + "</td><td>" + String(docSizes[i]) + "</td><td>";
        html += "<a href=\"/read?file=" + urlName + "\">Read</a> ";
        html += "<a href=\"/download?file=" + urlName + "\">Download</a> ";
        html += "<a href=\"/delete?file=" + urlName + "\" onclick=\"return confirm('Delete " + safeName + "?')\">Delete</a>";
        html += "</td></tr>";
    }
    html += "</tbody></table>";
    if (totalDocs == 0) html += "<p class=\"empty\">No documents found.</p>";
    html += "<p class=\"empty\">Page " + String(page) + "</p>";
    if (page > 1) html += "<a href=\"/?page=" + String(page - 1) + "\">Previous</a> ";
    if (end < totalDocs) html += "<a href=\"/?page=" + String(page + 1) + "\">Next</a>";
}

String pageStart(const String& title) {
    String html = F("<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>IZE Compose</title><style>body{font-family:Arial,sans-serif;background:#f4f4f0;color:#151515;margin:0;padding:24px}main{max-width:980px;margin:0 auto}h1{margin:0 0 6px;font-size:28px}h2{font-size:18px;margin:0 0 14px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}.card{background:#fff;border:1px solid #bbb;padding:18px;border-radius:6px}.wide{grid-column:1/-1}label{display:block;margin:12px 0 6px;font-weight:bold}input,select,button{width:100%;box-sizing:border-box;padding:10px;font-size:15px}button{background:#111;color:#fff;border:0;border-radius:4px;margin-top:12px;cursor:pointer}button:disabled{background:#777}table{width:100%;border-collapse:collapse}th,td{text-align:left;border-bottom:1px solid #ddd;padding:8px}a{color:#0645ad;margin-right:10px}.status,.empty{color:#555;font-size:14px;min-height:20px}.saved{color:#b00000;font-weight:bold}.note{color:#555;font-size:13px;line-height:1.45;margin-top:8px}.inline{display:flex;gap:12px;align-items:center}.inline>*{flex:1}.range-row{display:grid;grid-template-columns:1fr auto;gap:12px;align-items:center}.range-value{min-width:90px;text-align:right;font-size:14px;color:#333}.radio-row{display:flex;gap:18px;flex-wrap:wrap;margin-top:10px}.radio-row label{display:flex;align-items:center;gap:8px;margin:0;font-weight:normal}.radio-row input{width:auto}.stack{display:grid;gap:18px}.subgrid{display:grid;grid-template-columns:1fr 1fr;gap:16px}.subgrid .full{grid-column:1/-1}select[size]{min-height:220px}.muted{opacity:.55}@media(max-width:760px){.grid,.subgrid{grid-template-columns:1fr}}</style></head><body><main><h1>");
    html += title;
    html += F("</h1>");
    return html;
}

String webSleepLabelForIndex(int idx) {
    switch (idx) {
        case 0: return "30 sec";
        case 1: return "1 min";
        case 2: return "5 min";
        case 3: return "10 min";
        case 4: return "30 min";
        case 5: return "1 hr";
        default: return "OFF";
    }
}

void appendLanguageOptions(String& html) {
    html += "<option value=\"english\"";
    if (!isKoreanMode) html += " selected";
    html += ">English</option>";
    for (uint8_t i = 2; i < KEYBOARD_LAYOUT_TOTAL; i++) {
        html += "<option value=\"" + String(getKeyboardLayoutIdString(KEYBOARD_LAYOUTS[i].id)) + "\"";
        if (isKoreanMode && keyboardLayoutIndex == i) html += " selected";
        html += ">";
        html += htmlEscape(String(KEYBOARD_LAYOUTS[i].name));
        html += "</option>";
    }
}

bool sendSdFileResponse(const char* path, const char* contentType) {
    SdFile file;
    if (!file.open(path, O_RDONLY)) return false;
    server.setContentLength(file.fileSize());
    server.send(200, contentType, "");
    uint8_t buffer[512];
    while (file.available()) {
        int bytesRead = file.read(buffer, sizeof(buffer));
        if (bytesRead > 0) server.sendContent((char*)buffer, bytesRead);
        yield();
    }
    server.sendContent("");
    file.close();
    return true;
}

void handleDocumentRoot(const String& message = "") {
    String html = pageStart("IZE Compose Documents");
    if (message.length() > 0) html += "<p class=\"saved\">" + htmlEscape(message) + "</p>";
    html += F("<section class=\"grid\"><div class=\"card wide\"><h2>Documents</h2>");
    appendDocList(html);
    html += F("</div><div class=\"card wide\"><h2>Upload Text File</h2><form method=\"POST\" action=\"/uploadText\" enctype=\"multipart/form-data\"><label for=\"textFile\">Text file</label><input type=\"file\" id=\"textFile\" name=\"file\" accept=\".txt,text/plain\" required><button type=\"submit\">Upload Text</button></form><p class=\"empty\">Uploaded text is saved as the next doc_N.txt file.</p></div></section></main></body></html>");
    server.send(200, "text/html", html);
}

void handleUpdateRoot() {
    if (sendSdFileResponse(WEB_PROPERTY_PAGE_PATH, "text/html; charset=utf-8")) return;

    String html = pageStart("IZE Compose Update");
    html += F("<section class=\"grid\">");
    html += F("<div class=\"card wide\"><h2>PIN</h2><label for=\"pin\">4-digit PIN</label><input type=\"password\" id=\"pin\" inputmode=\"numeric\" maxlength=\"4\" placeholder=\"PIN\" required><p class=\"note\">This PIN is required before settings, firmware updates, font uploads, and image uploads are applied.</p></div>");
    html += F("<div class=\"card wide\"><h2>Environment Settings</h2><div class=\"stack\">");

    html += F("<div class=\"subgrid\">");
    html += F("<div><label for=\"sleepTimer\">Sleep Timer</label><div class=\"range-row\"><input type=\"range\" id=\"sleepTimer\" min=\"0\" max=\"6\" step=\"1\" value=\"");
    html += String(autoSleepIndex);
    html += F("\"><span class=\"range-value\" id=\"sleepTimerValue\">");
    html += webSleepLabelForIndex(autoSleepIndex);
    html += F("</span></div></div>");

    html += F("<div><label for=\"fontScale\">Text Size</label><div class=\"range-row\"><input type=\"range\" id=\"fontScale\" min=\"5\" max=\"35\" step=\"1\" value=\"");
    html += String((int)round(displayScale * 10.0f));
    html += F("\"><span class=\"range-value\" id=\"fontScaleValue\">");
    html += String(displayScale, 1);
    html += F("x</span></div></div>");

    html += F("<div><label for=\"lineSpacing\">Line Space</label><div class=\"range-row\"><input type=\"range\" id=\"lineSpacing\" min=\"0\" max=\"30\" step=\"1\" value=\"");
    html += String(lineSpacing);
    html += F("\"><span class=\"range-value\" id=\"lineSpacingValue\">");
    html += String(lineSpacing);
    html += F("</span></div></div>");

    html += F("<div><label for=\"letterSpacing\">Character Space</label><div class=\"range-row\"><input type=\"range\" id=\"letterSpacing\" min=\"-5\" max=\"10\" step=\"1\" value=\"");
    html += String(letterSpacing);
    html += F("\"><span class=\"range-value\" id=\"letterSpacingValue\">");
    html += String(letterSpacing);
    html += F("</span></div></div>");

    html += F("<div><label for=\"typingSpeed\">Speed</label><input type=\"number\" id=\"typingSpeed\" min=\"0\" max=\"2000\" step=\"1\" value=\"");
    html += String(typingSpeed);
    html += F("\"><p class=\"note\">Default: 0. Changing this value can make typing less comfortable.</p></div>");

    html += F("<div><label for=\"refreshLimit\">Refresh</label><input type=\"number\" id=\"refreshLimit\" min=\"0\" max=\"2000\" step=\"1\" value=\"");
    html += String(refreshLimit);
    html += F("\"><p class=\"note\">Default: 2000. Changing this value can make refresh behavior less comfortable.</p></div>");

    html += F("<div class=\"full\"><label>English Keyboard</label><div class=\"radio-row\" id=\"englishLayoutGroup\">");
    html += F("<label><input type=\"radio\" name=\"englishLayout\" value=\"1\"");
    if (englishLayoutIndex != 0) html += F(" checked");
    html += F(">Qwerty</label>");
    html += F("<label><input type=\"radio\" name=\"englishLayout\" value=\"0\"");
    if (englishLayoutIndex == 0) html += F(" checked");
    html += F(">Dvorak</label></div></div>");

    html += F("<div class=\"full\"><label for=\"language\">Language</label><select id=\"language\" size=\"10\">");
    appendLanguageOptions(html);
    html += F("</select></div>");
    html += F("</div>");

    html += F("<button id=\"settingsBtn\" onclick=\"saveSettings()\">Save</button><p class=\"status\" id=\"settingsStatus\">Waiting...</p></div></div>");

    html += F("<div class=\"card\"><h2>Firmware Update</h2><label for=\"fwFile\">Firmware file</label><input type=\"file\" id=\"fwFile\" accept=\".bin\"><button id=\"fwBtn\" onclick=\"uploadFirmware()\">Upload and Update</button><p class=\"status\" id=\"fwStatus\">Waiting...</p></div>");
    html += F("<div class=\"card\"><h2>Font / Image Upload</h2><label for=\"resFile\">File</label><input type=\"file\" id=\"resFile\" accept=\".bin,.png\"><p class=\"note\">The filename decides the target automatically. Examples: initial.png, hwalja_hangul.bin, NanumGothic_hangul.bin, NanumGothic_jamo.bin.</p><button id=\"resBtn\" onclick=\"uploadResource()\">Upload to SD</button><p class=\"status\" id=\"resStatus\">Waiting...</p></div>");
    html += F("</section></main><script>");
    html += F("const sleepLabels=['30 sec','1 min','5 min','10 min','30 min','1 hr','OFF'];");
    html += F("function bindRange(id,render){const el=document.getElementById(id);const out=document.getElementById(id+'Value');const draw=()=>out.textContent=render(el.value);el.addEventListener('input',draw);draw();}");
    html += F("bindRange('sleepTimer',v=>sleepLabels[Number(v)]||'OFF');");
    html += F("bindRange('fontScale',v=>(Number(v)/10).toFixed(1)+'x');");
    html += F("bindRange('lineSpacing',v=>String(v));");
    html += F("bindRange('letterSpacing',v=>String(v));");
    html += F("function syncEnglishState(){const isEnglish=document.getElementById('language').value==='english';const wrap=document.getElementById('englishLayoutGroup');wrap.classList.toggle('muted',!isEnglish);wrap.querySelectorAll('input').forEach(i=>i.disabled=!isEnglish);}document.getElementById('language').addEventListener('change',syncEnglishState);syncEnglishState();");
    html += F("function getPin(){return document.getElementById('pin').value.trim();}");
    html += F("async function saveSettings(){const pin=getPin();const status=document.getElementById('settingsStatus');const btn=document.getElementById('settingsBtn');if(!/^\\d{4}$/.test(pin)){status.textContent='Enter the 4-digit PIN.';return;}const english=document.querySelector('input[name=\"englishLayout\"]:checked').value;const body=new URLSearchParams({pin:pin,sleepTimer:document.getElementById('sleepTimer').value,fontScale:document.getElementById('fontScale').value,lineSpacing:document.getElementById('lineSpacing').value,letterSpacing:document.getElementById('letterSpacing').value,typingSpeed:document.getElementById('typingSpeed').value,refreshLimit:document.getElementById('refreshLimit').value,englishLayout:english,language:document.getElementById('language').value});btn.disabled=true;status.textContent='Saving...';try{const res=await fetch('/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'},body:body.toString()});const text=await res.text();status.textContent=text||'Done.';}catch(e){status.textContent='Error: '+e.message;}finally{btn.disabled=false;}}");
    html += F("async function sendUpload(inputId,statusId,buttonId,targetName){const input=document.getElementById(inputId);const status=document.getElementById(statusId);const btn=document.getElementById(buttonId);if(!input.files.length){status.textContent='Select a file.';return;}const pin=getPin();if(!/^\\d{4}$/.test(pin)){status.textContent='Enter the 4-digit PIN.';return;}const file=input.files[0];btn.disabled=true;status.textContent='Uploading...';const fd=new FormData();fd.append('file',file,targetName||file.name);try{const res=await fetch('/update',{method:'POST',headers:{'X-OTA-PIN':pin},body:fd});const text=await res.text();status.textContent=text||'Done.';}catch(e){status.textContent='Error: '+e.message;}finally{btn.disabled=false;}}");
    html += F("function uploadFirmware(){sendUpload('fwFile','fwStatus','fwBtn','izefirmware.bin');}");
    html += F("function uploadResource(){sendUpload('resFile','resStatus','resBtn','');}");
    html += F("</script></body></html>");
    server.send(200, "text/html", html);
}

void handleRoot() {
    if (webServerUpdateOnly) handleUpdateRoot();
    else handleDocumentRoot();
}

void handleRead() {
    String fn = server.arg("file");
    if (!isDocFilename(fn)) {
        server.send(400, "text/plain", "Invalid file");
        return;
    }

    SdFile file;
    if (!file.open(fn.c_str(), O_RDONLY)) {
        server.send(404, "text/plain", "File Not Found");
        return;
    }

    server.setContentLength(file.fileSize());
    server.send(200, "text/plain; charset=utf-8", "");
    uint8_t buffer[512];
    while (file.available()) {
        int bytesRead = file.read(buffer, sizeof(buffer));
        if (bytesRead > 0) server.sendContent((char*)buffer, bytesRead);
        yield();
    }
    server.sendContent("");
    file.close();
}

void handleDownload() {
    String fn = server.arg("file");
    if (!isDocFilename(fn)) {
        server.send(400, "text/plain", "Invalid file");
        return;
    }

    SdFile file;
    if (!file.open(fn.c_str(), O_RDONLY)) {
        server.send(404, "text/plain", "File Not Found");
        return;
    }

    server.setContentLength(file.fileSize());
    server.sendHeader("Content-Disposition", "attachment; filename=" + fn);
    server.send(200, "application/octet-stream", "");
    uint8_t buffer[512];
    while (file.available()) {
        int bytesRead = file.read(buffer, sizeof(buffer));
        if (bytesRead > 0) server.sendContent((char*)buffer, bytesRead);
        yield();
    }
    server.sendContent("");
    file.close();
}

void handleDelete() {
    String fn = server.arg("file");
    if (!isDocFilename(fn)) {
        server.send(400, "text/plain", "Invalid file");
        return;
    }

    SdFile file;
    if (file.open(fn.c_str(), O_RDWR)) {
        if (file.remove()) {
            server.sendHeader("Location", "/");
            server.send(303);
            return;
        }
    }
    server.send(500, "text/plain", "Delete Failed");
}

SdFile textUploadFile;
String textUploadName = "";
bool textUploadAccepted = false;
int textUploadStatus = 200;
String textUploadMessage = "";

void handleTextUpload() {
    HTTPUpload& upload = server.upload();
    if (webServerUpdateOnly) {
        textUploadAccepted = false;
        textUploadStatus = 403;
        textUploadMessage = "Text upload is not available in update mode.";
        return;
    }

    if (upload.status == UPLOAD_FILE_START) {
        textUploadName = "";
        textUploadAccepted = false;
        textUploadStatus = 400;
        textUploadMessage = "Only .txt files can be uploaded.";
        String filename = upload.filename;
        filename.toLowerCase();
        if (!filename.endsWith(".txt")) return;
        textUploadName = nextDocFilename();
        if (textUploadFile.open(textUploadName.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) {
            textUploadAccepted = true;
            textUploadStatus = 200;
            textUploadMessage = "Saved as " + textUploadName + ". The uploaded title was changed and saved as " + textUploadName + ".";
        } else {
            textUploadStatus = 500;
            textUploadMessage = "Could not create document.";
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (textUploadAccepted && textUploadFile.isOpen()) textUploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (textUploadFile.isOpen()) textUploadFile.close();
    }
}

void handleTextUploadComplete() {
    if (textUploadStatus == 200) {
        handleDocumentRoot(textUploadMessage);
    } else {
        server.send(textUploadStatus, "text/plain", textUploadMessage);
    }
}
String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

#endif
