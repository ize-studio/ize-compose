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
bool appendDeletedTombstone(const String& filename);
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


enum NetworkSubMode { NET_MAIN, NET_WIFI_STA, NET_WIFI };
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
    // 繞벿살탮??
    if (cp >= 0x0530 && cp <= 0x058F) return font_misc_ptr    ? font_misc_ptr    : font_latin_ptr;  // Armenian
    if (cp >= 0x0590 && cp <= 0x05FF) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Hebrew
    if (cp >= 0x0600 && cp <= 0x06FF) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Arabic
    if (cp >= 0x0750 && cp <= 0x077F) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Arabic Supplement
    if (cp >= 0xFB50 && cp <= 0xFDFF) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Arabic Presentation Forms-A
    if (cp >= 0xFE70 && cp <= 0xFEFF) return font_arabic_ptr  ? font_arabic_ptr  : font_latin_ptr;  // Arabic Presentation Forms-B
    // ??貫???戮?닡 Indic
    if (cp >= 0x0900 && cp <= 0x0D7F) return font_indic_ptr   ? font_indic_ptr   : font_latin_ptr;
    if (cp >= 0x0D80 && cp <= 0x0DFF) return font_indic_ptr   ? font_indic_ptr   : font_latin_ptr;  // Sinhala
    if (cp >= 0x0E00 && cp <= 0x0E7F) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;
    // ???뺥뀣?熬곣뫖六??    if (cp >= 0x0E00 && cp <= 0x0E7F) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;  // Thai
    if (cp >= 0x0E80 && cp <= 0x0EFF) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;  // Lao
    if (cp >= 0x1000 && cp <= 0x109F) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;  // Myanmar
    if (cp >= 0x1780 && cp <= 0x17FF) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;  // Khmer
    if (cp >= 0x19E0 && cp <= 0x19FF) return font_sea_ptr     ? font_sea_ptr     : font_latin_ptr;
    // ?リ옇??
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
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);


const char* ssid = "IZEcompose_FileServer";
extern String activeApPassword;
extern bool apHadClient;
extern bool apPasswordHidden;
extern bool apAuthenticatedClientSeen;


extern bool webDocumentUnlocked;
extern String otaPinCode;
extern const char* WEB_DOCUMENT_PAGE_PATH;
extern String githubSyncStatusMessage;

bool parseDocNumberFromName(const String& fn, int& outValue) {
    if (!fn.endsWith(".txt")) return false;
    if (fn.indexOf('/') >= 0 || fn.indexOf('\\') >= 0) return false;
    int digitStart = -1;
    if (fn.startsWith("doc_")) digitStart = 4;
    else if (fn.startsWith("doc")) digitStart = 3;
    else return false;
    int digitEnd = fn.length() - 4;
    if (digitEnd <= digitStart) return false;
    int value = 0;
    for (int i = digitStart; i < digitEnd; i++) {
        if (!isDigit(fn[i])) return false;
        value = value * 10 + (fn[i] - '0');
    }
    if (value <= 0) return false;
    outValue = value;
    return true;
}

bool isLegacyDocFilename(const String& fn) {
    if (!fn.startsWith("doc_")) return false;
    int value = 0;
    return parseDocNumberFromName(fn, value);
}

bool isDocFilename(const String& fn) {
    int value = 0;
    return parseDocNumberFromName(fn, value);
}

int docNumberFromName(const String& fn) {
    int value = 0;
    return parseDocNumberFromName(fn, value) ? value : 0;
}

String formatDocFilename(int docNumber) {
    if (docNumber < 1) docNumber = 1;
    char buf[24];
    snprintf(buf, sizeof(buf), "doc%04d.txt", docNumber);
    return String(buf);
}

String canonicalDocFilename(const String& fn) {
    return formatDocFilename(docNumberFromName(fn));
}

String nextDocFilename() {
    SdFile root;
    SdFile file;
    char name[64];
    int maxNum = 0;

    if (!root.open("/", O_RDONLY)) return formatDocFilename(1);
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
    return formatDocFilename(maxNum + 1);
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

String webUtf8Truncate(const String& text, int maxChars) {
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
        else break;
        if (p + len > text.length()) break;
        bool complete = true;
        for (int i = 1; i < len; i++) {
            if (((unsigned char)text[p + i] & 0xC0) != 0x80) {
                complete = false;
                break;
            }
        }
        if (!complete) break;
        out += text.substring(p, p + len);
        p += len;
        count++;
    }
    return out;
}

String readDocPreview(const String& filename) {
    SdFile doc;
    if (!doc.open(filename.c_str(), O_RDONLY)) return "";
    char buffer[129];
    int bytesRead = doc.read(buffer, 128);
    doc.close();
    if (bytesRead <= 0) return "";
    buffer[bytesRead] = '\0';
    String preview = String(buffer);
    preview.replace("\r", " ");
    preview.replace("\n", " ");
    preview.trim();
    return webUtf8Truncate(preview, 32);
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
        String preview = readDocPreview(fn);
        String safePreview = htmlEscape(preview);
        String urlName = urlEncode(fn);
        html += "<tr><td><span class=\"doc-name\">" + safeName + "</span>";
        if (safePreview.length() > 0) html += " <span class=\"doc-preview\">" + safePreview + "</span>";
        html += "</td><td>" + String(docSizes[i]) + "</td><td>";
        html += "<a href=\"/read?file=" + urlName + "\">Read</a> ";
        html += "<a href=\"/download?file=" + urlName + "\">Download</a> ";
        html += "<a href=\"/delete?file=" + urlName + "\">Delete</a>";
        html += "</td></tr>";
    }
    html += "</tbody></table>";
    if (totalDocs == 0) html += "<p class=\"empty\">No documents found.</p>";
    html += "<p class=\"empty\">Page " + String(page) + "</p>";
    if (page > 1) html += "<a href=\"/?page=" + String(page - 1) + "\">Previous</a> ";
    if (end < totalDocs) html += "<a href=\"/?page=" + String(page + 1) + "\">Next</a>";
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


bool documentAccessAllowed() {
    if (webDocumentUnlocked) return true;
    server.send(403, "text/plain", "PIN required.");
    return false;
}

void handleWebAuth() {
    String pin = server.arg("pin");
    if (pin.length() == 4 && pin == otaPinCode) {
        webDocumentUnlocked = true;
        apHadClient = true;
        apPasswordHidden = true;
        apAuthenticatedClientSeen = true;
        needUpdate = true;
        server.send(200, "text/plain", "OK");
        return;
    }
    server.send(403, "text/plain", "Invalid PIN.");
}
void handleDocumentsList() {
    if (!documentAccessAllowed()) return;
    String html = "";
    appendDocList(html);
    server.send(200, "text/html; charset=utf-8", html);
}
void handleDocumentRoot(const String& message = "") {
    if (sendSdFileResponse(WEB_DOCUMENT_PAGE_PATH, "text/html; charset=utf-8")) return;
    server.send(500, "text/plain", String("Missing ") + WEB_DOCUMENT_PAGE_PATH + " on SD card.");
}

void handleRoot() { handleDocumentRoot(); }
void handleRead() {
    if (!documentAccessAllowed()) return;
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
    if (!documentAccessAllowed()) return;
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

void sendDeleteConfirmPage(const String& fn, const String& message = "") {
    String code = server.hasArg("code") ? server.arg("code") : "";
    if (code.length() != 6) {
        char generated[7];
        snprintf(generated, sizeof(generated), "%06lu", (unsigned long)(esp_random() % 1000000UL));
        code = String(generated);
    }
    String safeName = htmlEscape(fn);
    String safeCode = htmlEscape(code);
    String html = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
    html += "<title>Delete document</title><style>body{font-family:system-ui,sans-serif;max-width:520px;margin:32px auto;padding:0 16px}input,button{font-size:18px;padding:10px;margin:6px 0;width:100%;box-sizing:border-box}.code{font-size:32px;font-weight:700;letter-spacing:4px}.err{color:#b00020}</style></head><body>";
    html += "<h1>Delete document</h1>";
    if (message.length() > 0) html += "<p class=\"err\">" + htmlEscape(message) + "</p>";
    html += "<p>Type the 6-digit code to delete <b>" + safeName + "</b>.</p>";
    html += "<p class=\"code\">" + safeCode + "</p>";
    html += "<form method=\"get\" action=\"/delete\">";
    html += "<input type=\"hidden\" name=\"file\" value=\"" + safeName + "\">";
    html += "<input type=\"hidden\" name=\"code\" value=\"" + safeCode + "\">";
    html += "<input name=\"confirm\" inputmode=\"numeric\" pattern=\"[0-9]{6}\" maxlength=\"6\" autofocus>";
    html += "<button type=\"submit\">Delete</button></form><p><a href=\"/\">Cancel</a></p></body></html>";
    server.send(200, "text/html", html);
}

void handleDelete() {
    if (!documentAccessAllowed()) return;
    String fn = server.arg("file");
    if (!isDocFilename(fn)) {
        server.send(400, "text/plain", "Invalid file");
        return;
    }
    if (!server.hasArg("confirm")) {
        sendDeleteConfirmPage(fn);
        return;
    }
    String code = server.arg("code");
    String confirm = server.arg("confirm");
    if (code.length() != 6 || confirm != code) {
        sendDeleteConfirmPage(fn, "Code did not match.");
        return;
    }

    SdFile file;
    if (file.open(fn.c_str(), O_RDWR)) {
        if (file.remove()) {
            appendDeletedTombstone(fn);
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
    if (!webDocumentUnlocked) {
        textUploadAccepted = false;
        textUploadStatus = 403;
        textUploadMessage = "PIN required.";
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
