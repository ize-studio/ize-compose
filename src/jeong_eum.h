#pragma once

#include <Arduino.h>

extern int cho;
extern int jung;
extern int jong;
extern char lastJongChar;
extern String fullText;
extern int cursorPos;
extern bool forceSafeFullTextRedraw;

void insertText(String str);

static constexpr int KEY_ENGINE_MAX_RESHAPE_BYTES = 384;

inline int keyEngineUtf8LenAt(const String& text, int index) {
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
        if ((((const uint8_t*)text.c_str())[index + i] & 0xC0) != 0x80) return 1;
    }
    if (need == 3) {
        if (c == 0xE0 && s[index + 1] < 0xA0) return 1;
        if (c == 0xED && s[index + 1] >= 0xA0) return 1;
    }
    return need;
}

inline uint32_t keyEngineCodepointAt(const String& text, int index, int& charLen) {
    charLen = keyEngineUtf8LenAt(text, index);
    const uint8_t* s = (const uint8_t*)text.c_str() + index;
    if (charLen == 1) return s[0];
    if (charLen == 2) return ((uint32_t)(s[0] & 0x1F) << 6) | (s[1] & 0x3F);
    if (charLen == 3) return ((uint32_t)(s[0] & 0x0F) << 12) | ((uint32_t)(s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    return s[0];
}

inline void keyEngineClampCursorToUtf8Boundary() {
    if (cursorPos < 0) cursorPos = 0;
    if (cursorPos > fullText.length()) cursorPos = fullText.length();
    while (cursorPos > 0 && cursorPos < fullText.length() && (((unsigned char)fullText[cursorPos] & 0xC0) == 0x80)) cursorPos--;
}

inline void keyEngineAppendUtf8(String& out, uint32_t cp) {
    if (cp > 0xFFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0xFFFD;
    char buf[5];
    if (cp <= 0x7F) {
        buf[0] = (char)cp; buf[1] = 0;
    } else if (cp <= 0x7FF) {
        buf[0] = (char)(0xC0 | (cp >> 6));
        buf[1] = (char)(0x80 | (cp & 0x3F));
        buf[2] = 0;
    } else {
        buf[0] = (char)(0xE0 | ((cp >> 12) & 0x0F));
        buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (cp & 0x3F));
        buf[3] = 0;
    }
    out += buf;
}

inline String makeKorStr(int c, int ju, int jo) {
    if (c == -1 || ju == -1) return "";
    int uni = 0xAC00 + (c * 21 * 28) + (ju * 28) + (jo == -1 ? 0 : jo);
    char utf8[4];
    utf8[0] = 0xE0 | ((uni >> 12) & 0x0F);
    utf8[1] = 0x80 | ((uni >> 6) & 0x3F);
    utf8[2] = 0x80 | (uni & 0x3F);
    utf8[3] = '\0';
    return String(utf8);
}

inline int keyEngineGetCho(char c) {
    switch(c) {
        case 'r': return 0; case 'R': return 1; case 's': return 2; case 'e': return 3; case 'E': return 4; case 'f': return 5; case 'a': return 6; case 'q': return 7; case 'Q': return 8; case 't': return 9; case 'T': return 10; case 'd': return 11; case 'w': return 12; case 'W': return 13; case 'c': return 14; case 'z': return 15; case 'x': return 16; case 'v': return 17; case 'g': return 18; default: return -1;
    }
}

inline int keyEngineGetJung(char c) {
    switch(c) {
        case 'k': return 0; case 'o': return 1; case 'i': return 2; case 'O': return 3; case 'j': return 4; case 'p': return 5; case 'u': return 6; case 'P': return 7; case 'h': return 8; case 'y': return 12; case 'n': return 13; case 'b': return 17; case 'm': return 18; case 'l': return 20; default: return -1;
    }
}

inline int keyEngineGetJong(char c) {
    switch(c) {
        case 'r': return 1; case 'R': return 2; case 's': return 4; case 'e': return 7; case 'f': return 8; case 'a': return 16; case 'q': return 17; case 't': return 19; case 'T': return 20; case 'd': return 21; case 'w': return 22; case 'c': return 23; case 'z': return 24; case 'x': return 25; case 'v': return 26; case 'g': return 27; default: return 0;
    }
}

inline int keyEngineCombineJong(int j1, int j2) {
    if (j1 == 1 && j2 == 19) return 3;
    if (j1 == 4 && j2 == 22) return 5;
    if (j1 == 4 && j2 == 27) return 6;
    if (j1 == 8 && j2 == 1) return 9;
    if (j1 == 8 && j2 == 16) return 10;
    if (j1 == 8 && j2 == 17) return 11;
    if (j1 == 8 && j2 == 19) return 12;
    if (j1 == 8 && j2 == 25) return 13;
    if (j1 == 8 && j2 == 26) return 14;
    if (j1 == 8 && j2 == 27) return 15;
    if (j1 == 17 && j2 == 19) return 18;
    return -1;
}

inline int keyEngineCombineJung(int j1, int j2) {
    if (j1 == 8 && j2 == 0) return 9;
    if (j1 == 8 && j2 == 1) return 10;
    if (j1 == 8 && j2 == 20) return 11;
    if (j1 == 13 && j2 == 4) return 14;
    if (j1 == 13 && j2 == 5) return 15;
    if (j1 == 13 && j2 == 20) return 16;
    if (j1 == 18 && j2 == 20) return 19;
    return -1;
}

inline int keyEngineSplitCombinedJong(int j) {
    if (j == 3) return 1;
    if (j == 5 || j == 6) return 4;
    if (j >= 9 && j <= 15) return 8;
    if (j == 18) return 17;
    return -1;
}

inline char keyEngineJongKey(int j) {
    switch(j) {
        case 1: return 'r';
        case 2: return 'R';
        case 4: return 's';
        case 7: return 'e';
        case 8: return 'f';
        case 16: return 'a';
        case 17: return 'q';
        case 19: return 't';
        case 20: return 'T';
        case 21: return 'd';
        case 22: return 'w';
        case 23: return 'c';
        case 24: return 'z';
        case 25: return 'x';
        case 26: return 'v';
        case 27: return 'g';
        default: return 0;
    }
}

inline const char* keyEngineChoStr(int c) {
    static const char* values[] = {"\xE3\x84\xB1", "\xE3\x84\xB2", "\xE3\x84\xB4", "\xE3\x84\xB7", "\xE3\x84\xB8", "\xE3\x84\xB9", "\xE3\x85\x81", "\xE3\x85\x82", "\xE3\x85\x83", "\xE3\x85\x85", "\xE3\x85\x86", "\xE3\x85\x87", "\xE3\x85\x88", "\xE3\x85\x89", "\xE3\x85\x8A", "\xE3\x85\x8B", "\xE3\x85\x8C", "\xE3\x85\x8D", "\xE3\x85\x8E"};
    if (c < 0 || c >= 19) return "";
    return values[c];
}

inline const char* keyEngineJungStr(int j) {
    static const char* values[] = {"\xE3\x85\x8F", "\xE3\x85\x90", "\xE3\x85\x91", "\xE3\x85\x92", "\xE3\x85\x93", "\xE3\x85\x94", "\xE3\x85\x95", "\xE3\x85\x96", "\xE3\x85\x97", "\xE3\x85\x98", "\xE3\x85\x99", "\xE3\x85\x9A", "\xE3\x85\x9B", "\xE3\x85\x9C", "\xE3\x85\x9D", "\xE3\x85\x9E", "\xE3\x85\x9F", "\xE3\x85\xA0", "\xE3\x85\xA1", "\xE3\x85\xA2", "\xE3\x85\xA3"};
    if (j < 0 || j >= 21) return "";
    return values[j];
}

inline void flushKorean() {
    String s = "";
    if (cho != -1 && jung != -1) s = makeKorStr(cho, jung, jong);
    else if (cho != -1) s = String(keyEngineChoStr(cho));
    else if (jung != -1) s = String(keyEngineJungStr(jung));
    if (s != "") insertText(s);
    cho = -1;
    jung = -1;
    jong = -1;
    lastJongChar = 0;
}

inline void processKoreanInput(byte k, char real, bool isShiftPressed, const char* engMap, const char* shiftMap, size_t mapSize) {
    char korInput = real;
    if (k < mapSize) korInput = isShiftPressed ? shiftMap[k] : engMap[k];
    if (isShiftPressed && real >= 'A' && real <= 'Z') {
        if (real != 'Q' && real != 'W' && real != 'E' && real != 'R' && real != 'T' && real != 'O' && real != 'P') korInput = real + 32;
    }
    int ci = keyEngineGetCho(korInput);
    int ji = keyEngineGetJung(korInput);
    int joi = keyEngineGetJong(korInput);
    if (ci != -1) {
        if (cho == -1 && jung == -1) cho = ci;
        else if (cho != -1 && jung == -1) { flushKorean(); cho = ci; }
        else if (cho != -1 && jung != -1 && jong == -1) { if (joi > 0) { jong = joi; lastJongChar = korInput; } else { flushKorean(); cho = ci; } }
        else if (cho != -1 && jung != -1 && jong != -1) { int comb = keyEngineCombineJong(jong, joi); if (comb != -1) { jong = comb; lastJongChar = korInput; } else { flushKorean(); cho = ci; } }
        else if (cho == -1 && jung != -1) { flushKorean(); cho = ci; }
    } else if (ji != -1) {
        if (cho != -1 && jung == -1) jung = ji;
        else if (cho != -1 && jung != -1 && jong == -1) { int comb = keyEngineCombineJung(jung, ji); if (comb != -1) jung = comb; else { flushKorean(); jung = ji; } }
        else if (cho != -1 && jung != -1 && jong != -1) {
            int prev = lastJongChar;
            if ((jong == 3 || jong == 5 || jong == 6 || (jong >= 9 && jong <= 15) || jong == 18)) {
                switch(jong) { case 3: jong = 1; break; case 5: case 6: jong = 4; break; case 18: jong = 17; break; default: jong = 8; break; }
            } else jong = -1;
            flushKorean();
            cho = keyEngineGetCho(prev);
            jung = ji;
        } else if (cho == -1) {
            if (jung == -1) jung = ji;
            else { int comb = keyEngineCombineJung(jung, ji); if (comb != -1) jung = comb; else { flushKorean(); jung = ji; } }
        }
    } else {
        flushKorean();
        insertText(String(real));
    }
}

enum KeyEngineScript : uint8_t {
    KEY_ENGINE_NONE = 0,
    KEY_ENGINE_KOREAN,
    KEY_ENGINE_ARABIC,
    KEY_ENGINE_INDIC,
    KEY_ENGINE_THAI,
    KEY_ENGINE_MYANMAR,
    KEY_ENGINE_KHMER,
    KEY_ENGINE_LAO,
    KEY_ENGINE_TIBETAN,
    KEY_ENGINE_SINHALA,
    KEY_ENGINE_ETHIOPIC,
    KEY_ENGINE_JAPANESE,
    KEY_ENGINE_HEBREW
};

inline bool keyEngineNeedsComposition(KeyEngineScript engine) {
    return engine != KEY_ENGINE_NONE && engine != KEY_ENGINE_KOREAN;
}

inline void keyEngineClearComposition(KeyEngineScript engine) {
    if (engine == KEY_ENGINE_KOREAN) {
        cho = -1;
        jung = -1;
        jong = -1;
        lastJongChar = 0;
    }
}

inline void keyEngineReset(KeyEngineScript engine) {
    if (engine == KEY_ENGINE_KOREAN) flushKorean();
}

inline bool keyEngineHandleBackspace(KeyEngineScript engine) {
    if (engine != KEY_ENGINE_KOREAN) return false;
    if (cho == -1 && jung == -1) return false;

    // Ize Compose writing rule: a visible composing Hangul syllable is one editing unit.
    // Backspace must remove the whole composing syllable, not peel jong/jung/cho one by one.
    keyEngineClearComposition(engine);
    return true;
}

struct KeyEngineArabicShape {
    uint32_t base;
    uint32_t isolated;
    uint32_t finalForm;
    uint32_t initialForm;
    uint32_t medialForm;
};

static const KeyEngineArabicShape KEY_ENGINE_ARABIC_SHAPES[] = {
    {0x0621, 0xFE80, 0, 0, 0},
    {0x0622, 0xFE81, 0xFE82, 0, 0},
    {0x0623, 0xFE83, 0xFE84, 0, 0},
    {0x0624, 0xFE85, 0xFE86, 0, 0},
    {0x0625, 0xFE87, 0xFE88, 0, 0},
    {0x0626, 0xFE89, 0xFE8A, 0xFE8B, 0xFE8C},
    {0x0627, 0xFE8D, 0xFE8E, 0, 0},
    {0x0628, 0xFE8F, 0xFE90, 0xFE91, 0xFE92},
    {0x0629, 0xFE93, 0xFE94, 0, 0},
    {0x062A, 0xFE95, 0xFE96, 0xFE97, 0xFE98},
    {0x062B, 0xFE99, 0xFE9A, 0xFE9B, 0xFE9C},
    {0x062C, 0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0},
    {0x062D, 0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4},
    {0x062E, 0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8},
    {0x062F, 0xFEA9, 0xFEAA, 0, 0},
    {0x0630, 0xFEAB, 0xFEAC, 0, 0},
    {0x0631, 0xFEAD, 0xFEAE, 0, 0},
    {0x0632, 0xFEAF, 0xFEB0, 0, 0},
    {0x0633, 0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4},
    {0x0634, 0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8},
    {0x0635, 0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC},
    {0x0636, 0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0},
    {0x0637, 0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4},
    {0x0638, 0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8},
    {0x0639, 0xFEC9, 0xFECA, 0xFECB, 0xFECC},
    {0x063A, 0xFECD, 0xFECE, 0xFECF, 0xFED0},
    {0x0640, 0x0640, 0x0640, 0x0640, 0x0640},
    {0x0641, 0xFED1, 0xFED2, 0xFED3, 0xFED4},
    {0x0642, 0xFED5, 0xFED6, 0xFED7, 0xFED8},
    {0x0643, 0xFED9, 0xFEDA, 0xFEDB, 0xFEDC},
    {0x0644, 0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0},
    {0x0645, 0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4},
    {0x0646, 0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8},
    {0x0647, 0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC},
    {0x0648, 0xFEED, 0xFEEE, 0, 0},
    {0x0649, 0xFEEF, 0xFEF0, 0, 0},
    {0x064A, 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4},
    {0x0671, 0xFB50, 0xFB51, 0, 0},
    {0x0679, 0xFB66, 0xFB67, 0xFB68, 0xFB69},
    {0x067E, 0xFB56, 0xFB57, 0xFB58, 0xFB59},
    {0x0686, 0xFB7A, 0xFB7B, 0xFB7C, 0xFB7D},
    {0x0688, 0xFB88, 0xFB89, 0, 0},
    {0x0691, 0xFB8C, 0xFB8D, 0, 0},
    {0x0698, 0xFB8A, 0xFB8B, 0, 0},
    {0x06A4, 0xFB6A, 0xFB6B, 0xFB6C, 0xFB6D},
    {0x06A9, 0xFB8E, 0xFB8F, 0xFB90, 0xFB91},
    {0x06AF, 0xFB92, 0xFB93, 0xFB94, 0xFB95},
    {0x06BA, 0xFB9E, 0xFB9F, 0, 0},
    {0x06BB, 0xFBA0, 0xFBA1, 0xFBA2, 0xFBA3},
    {0x06BE, 0xFBAA, 0xFBAB, 0xFBAC, 0xFBAD},
    {0x06C0, 0xFBA4, 0xFBA5, 0, 0},
    {0x06C1, 0xFBA6, 0xFBA7, 0xFBA8, 0xFBA9},
    {0x06C6, 0xFBD9, 0xFBDA, 0, 0},
    {0x06C7, 0xFBD7, 0xFBD8, 0, 0},
    {0x06C8, 0xFBDB, 0xFBDC, 0, 0},
    {0x06C9, 0xFBE2, 0xFBE3, 0, 0},
    {0x06CB, 0xFBDE, 0xFBDF, 0, 0},
    {0x06CC, 0xFBFC, 0xFBFD, 0xFBFE, 0xFBFF},
    {0x06D0, 0xFBE4, 0xFBE5, 0xFBE6, 0xFBE7},
    {0x06D2, 0xFBAE, 0xFBAF, 0, 0},
    {0x06D3, 0xFBB0, 0xFBB1, 0, 0}
};

inline const KeyEngineArabicShape* keyEngineArabicShapeFor(uint32_t cp) {
    if (cp == 0) return nullptr;
    for (size_t i = 0; i < sizeof(KEY_ENGINE_ARABIC_SHAPES) / sizeof(KEY_ENGINE_ARABIC_SHAPES[0]); i++) {
        const KeyEngineArabicShape* s = &KEY_ENGINE_ARABIC_SHAPES[i];
        if (cp == s->base || cp == s->isolated || (s->finalForm != 0 && cp == s->finalForm) || (s->initialForm != 0 && cp == s->initialForm) || (s->medialForm != 0 && cp == s->medialForm)) return s;
    }
    return nullptr;
}

inline uint32_t keyEngineArabicBase(uint32_t cp) {
    const KeyEngineArabicShape* s = keyEngineArabicShapeFor(cp);
    return s ? s->base : cp;
}

inline bool keyEngineArabicMark(uint32_t cp) {
    return (cp >= 0x064B && cp <= 0x065F) || cp == 0x0670 || (cp >= 0x06D6 && cp <= 0x06ED);
}

inline bool keyEngineArabicRunChar(uint32_t cp) {
    cp = keyEngineArabicBase(cp);
    return keyEngineArabicShapeFor(cp) != nullptr || keyEngineArabicMark(cp);
}

inline bool keyEngineArabicCanJoinPrev(uint32_t cp) {
    const KeyEngineArabicShape* s = keyEngineArabicShapeFor(keyEngineArabicBase(cp));
    return s && s->finalForm != 0;
}

inline bool keyEngineArabicCanJoinNext(uint32_t cp) {
    const KeyEngineArabicShape* s = keyEngineArabicShapeFor(keyEngineArabicBase(cp));
    return s && s->initialForm != 0;
}

inline int keyEngineCharCountBetween(const String& text, int start, int end) {
    int count = 0;
    for (int i = start; i < end && i < text.length(); ) {
        int l = keyEngineUtf8LenAt(text, i);
        if (l <= 0) break;
        i += l;
        count++;
    }
    return count;
}

inline int keyEngineByteOffsetAfterChars(const String& text, int charCount) {
    int i = 0;
    int count = 0;
    while (i < text.length() && count < charCount) {
        int l = keyEngineUtf8LenAt(text, i);
        if (l <= 0) break;
        i += l;
        count++;
    }
    return i;
}

inline uint32_t keyEngineArabicNeighbor(const String& text, int index, bool previous) {
    if (previous) {
        int p = index - 1;
        while (p >= 0) {
            while (p > 0 && ((text[p] & 0xC0) == 0x80)) p--;
            int l = 0;
            uint32_t cp = keyEngineArabicBase(keyEngineCodepointAt(text, p, l));
            if (!keyEngineArabicMark(cp)) return cp;
            p--;
        }
    } else {
        int p = index;
        while (p < text.length()) {
            int l = 0;
            uint32_t cp = keyEngineArabicBase(keyEngineCodepointAt(text, p, l));
            if (!keyEngineArabicMark(cp)) return cp;
            p += l;
        }
    }
    return 0;
}

inline String keyEngineShapeArabicRun(const String& raw) {
    String out = "";
    out.reserve(raw.length() + 8);
    for (int i = 0; i < raw.length(); ) {
        int l = 0;
        uint32_t original = keyEngineCodepointAt(raw, i, l);
        uint32_t base = keyEngineArabicBase(original);
        if (keyEngineArabicMark(base)) {
            keyEngineAppendUtf8(out, base);
            i += l;
            continue;
        }
        const KeyEngineArabicShape* s = keyEngineArabicShapeFor(base);
        if (!s) {
            keyEngineAppendUtf8(out, original);
            i += l;
            continue;
        }
        uint32_t prev = keyEngineArabicNeighbor(raw, i, true);
        uint32_t next = keyEngineArabicNeighbor(raw, i + l, false);
        bool joinPrev = prev != 0 && keyEngineArabicCanJoinNext(prev) && keyEngineArabicCanJoinPrev(base);
        bool joinNext = next != 0 && keyEngineArabicCanJoinNext(base) && keyEngineArabicCanJoinPrev(next);
        uint32_t shaped = s->isolated;
        if (joinPrev && joinNext && s->medialForm != 0) shaped = s->medialForm;
        else if (joinPrev && s->finalForm != 0) shaped = s->finalForm;
        else if (joinNext && s->initialForm != 0) shaped = s->initialForm;
        keyEngineAppendUtf8(out, shaped);
        i += l;
    }
    return out;
}

inline void keyEngineReshapeArabicAroundCursor() {
    int len = fullText.length();
    if (cursorPos < 0) cursorPos = 0;
    if (cursorPos > len) cursorPos = len;
    int start = cursorPos;
    while (start > 0 && cursorPos - start < KEY_ENGINE_MAX_RESHAPE_BYTES) {
        int p = start - 1;
        while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--;
        int l = 0;
        uint32_t cp = keyEngineArabicBase(keyEngineCodepointAt(fullText, p, l));
        if (!keyEngineArabicRunChar(cp)) break;
        start = p;
    }
    int end = cursorPos;
    while (end < len && end - cursorPos < KEY_ENGINE_MAX_RESHAPE_BYTES) {
        int l = 0;
        uint32_t cp = keyEngineArabicBase(keyEngineCodepointAt(fullText, end, l));
        if (!keyEngineArabicRunChar(cp)) break;
        end += l;
    }
    if (start == end) return;
    int cursorChars = keyEngineCharCountBetween(fullText, start, cursorPos);
    String raw = fullText.substring(start, end);
    String shaped = keyEngineShapeArabicRun(raw);
    int newCursor = start + keyEngineByteOffsetAfterChars(shaped, cursorChars);
    fullText = fullText.substring(0, start) + shaped + fullText.substring(end);
    cursorPos = newCursor;
}

inline bool keyEngineIndicBlock(uint32_t cp) {
    return (cp >= 0x0900 && cp <= 0x097F) || (cp >= 0x0980 && cp <= 0x09FF) || (cp >= 0x0A00 && cp <= 0x0A7F) || (cp >= 0x0A80 && cp <= 0x0AFF) || (cp >= 0x0B80 && cp <= 0x0BFF) || (cp >= 0x0C00 && cp <= 0x0C7F) || (cp >= 0x0C80 && cp <= 0x0CFF) || (cp >= 0x0D00 && cp <= 0x0D7F);
}

inline bool keyEngineIndicDependent(uint32_t cp) {
    uint32_t low = cp & 0x7F;
    if (!keyEngineIndicBlock(cp)) return false;
    if (low <= 0x03) return true;
    if (low == 0x3C) return true;
    if (low >= 0x3E && low <= 0x4D) return true;
    if (low >= 0x55 && low <= 0x57) return true;
    if (low >= 0x62 && low <= 0x63) return true;
    return false;
}

inline bool keyEngineIndicLetter(uint32_t cp) {
    uint32_t low = cp & 0x7F;
    if (!keyEngineIndicBlock(cp)) return false;
    if (low >= 0x04 && low <= 0x39) return true;
    if (low >= 0x58 && low <= 0x61) return true;
    return false;
}

inline bool keyEngineIndicHasBaseBefore(int index) {
    int p = index - 1;
    while (p >= 0) {
        while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--;
        int l = 0;
        uint32_t cp = keyEngineCodepointAt(fullText, p, l);
        if (!keyEngineIndicBlock(cp)) return false;
        if (keyEngineIndicLetter(cp) || cp == 0x25CC) return true;
        p--;
    }
    return false;
}

inline void keyEngineInsertDottedBefore(int index) {
    String dotted = "\xE2\x97\x8C";
    fullText = fullText.substring(0, index) + dotted + fullText.substring(index);
    if (index <= cursorPos) cursorPos += dotted.length();
}

inline void keyEngineStabilizeIndicAt(int index) {
    if (index < 0 || index >= fullText.length()) return;
    int l = 0;
    uint32_t cp = keyEngineCodepointAt(fullText, index, l);
    if (!keyEngineIndicDependent(cp)) return;
    if (keyEngineIndicHasBaseBefore(index)) return;
    keyEngineInsertDottedBefore(index);
}

inline void keyEngineStabilizeIndicAroundCursor() {
    if (cursorPos > 0 && cursorPos <= fullText.length()) {
        int p = cursorPos - 1;
        while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--;
        keyEngineStabilizeIndicAt(p);
    }
    if (cursorPos < fullText.length()) keyEngineStabilizeIndicAt(cursorPos);
}

inline bool keyEngineJapaneseRunChar(uint32_t cp) {
    return (cp >= 0xFF61 && cp <= 0xFF9F) || (cp >= 0x30A0 && cp <= 0x30FF) || (cp >= 0x3000 && cp <= 0x303F);
}

inline uint32_t keyEngineJapaneseSingle(uint32_t cp) {
    switch (cp) {
        case 0xFF61: return 0x3002;
        case 0xFF62: return 0x300C;
        case 0xFF63: return 0x300D;
        case 0xFF64: return 0x3001;
        case 0xFF65: return 0x30FB;
        case 0xFF66: return 0x30F2;
        case 0xFF67: return 0x30A1;
        case 0xFF68: return 0x30A3;
        case 0xFF69: return 0x30A5;
        case 0xFF6A: return 0x30A7;
        case 0xFF6B: return 0x30A9;
        case 0xFF6C: return 0x30E3;
        case 0xFF6D: return 0x30E5;
        case 0xFF6E: return 0x30E7;
        case 0xFF6F: return 0x30C3;
        case 0xFF70: return 0x30FC;
        case 0xFF71: return 0x30A2;
        case 0xFF72: return 0x30A4;
        case 0xFF73: return 0x30A6;
        case 0xFF74: return 0x30A8;
        case 0xFF75: return 0x30AA;
        case 0xFF76: return 0x30AB;
        case 0xFF77: return 0x30AD;
        case 0xFF78: return 0x30AF;
        case 0xFF79: return 0x30B1;
        case 0xFF7A: return 0x30B3;
        case 0xFF7B: return 0x30B5;
        case 0xFF7C: return 0x30B7;
        case 0xFF7D: return 0x30B9;
        case 0xFF7E: return 0x30BB;
        case 0xFF7F: return 0x30BD;
        case 0xFF80: return 0x30BF;
        case 0xFF81: return 0x30C1;
        case 0xFF82: return 0x30C4;
        case 0xFF83: return 0x30C6;
        case 0xFF84: return 0x30C8;
        case 0xFF85: return 0x30CA;
        case 0xFF86: return 0x30CB;
        case 0xFF87: return 0x30CC;
        case 0xFF88: return 0x30CD;
        case 0xFF89: return 0x30CE;
        case 0xFF8A: return 0x30CF;
        case 0xFF8B: return 0x30D2;
        case 0xFF8C: return 0x30D5;
        case 0xFF8D: return 0x30D8;
        case 0xFF8E: return 0x30DB;
        case 0xFF8F: return 0x30DE;
        case 0xFF90: return 0x30DF;
        case 0xFF91: return 0x30E0;
        case 0xFF92: return 0x30E1;
        case 0xFF93: return 0x30E2;
        case 0xFF94: return 0x30E4;
        case 0xFF95: return 0x30E6;
        case 0xFF96: return 0x30E8;
        case 0xFF97: return 0x30E9;
        case 0xFF98: return 0x30EA;
        case 0xFF99: return 0x30EB;
        case 0xFF9A: return 0x30EC;
        case 0xFF9B: return 0x30ED;
        case 0xFF9C: return 0x30EF;
        case 0xFF9D: return 0x30F3;
        default: return cp;
    }
}

inline uint32_t keyEngineJapanesePair(uint32_t base, uint32_t mark) {
    if (base == 0xFF66 && mark == 0xFF9E) return 0x30FA;
    if (base == 0xFF73 && mark == 0xFF9E) return 0x30F4;
    if (base == 0xFF76 && mark == 0xFF9E) return 0x30AC;
    if (base == 0xFF77 && mark == 0xFF9E) return 0x30AE;
    if (base == 0xFF78 && mark == 0xFF9E) return 0x30B0;
    if (base == 0xFF79 && mark == 0xFF9E) return 0x30B2;
    if (base == 0xFF7A && mark == 0xFF9E) return 0x30B4;
    if (base == 0xFF7B && mark == 0xFF9E) return 0x30B6;
    if (base == 0xFF7C && mark == 0xFF9E) return 0x30B8;
    if (base == 0xFF7D && mark == 0xFF9E) return 0x30BA;
    if (base == 0xFF7E && mark == 0xFF9E) return 0x30BC;
    if (base == 0xFF7F && mark == 0xFF9E) return 0x30BE;
    if (base == 0xFF80 && mark == 0xFF9E) return 0x30C0;
    if (base == 0xFF81 && mark == 0xFF9E) return 0x30C2;
    if (base == 0xFF82 && mark == 0xFF9E) return 0x30C5;
    if (base == 0xFF83 && mark == 0xFF9E) return 0x30C7;
    if (base == 0xFF84 && mark == 0xFF9E) return 0x30C9;
    if (base == 0xFF8A && mark == 0xFF9E) return 0x30D0;
    if (base == 0xFF8A && mark == 0xFF9F) return 0x30D1;
    if (base == 0xFF8B && mark == 0xFF9E) return 0x30D3;
    if (base == 0xFF8B && mark == 0xFF9F) return 0x30D4;
    if (base == 0xFF8C && mark == 0xFF9E) return 0x30D6;
    if (base == 0xFF8C && mark == 0xFF9F) return 0x30D7;
    if (base == 0xFF8D && mark == 0xFF9E) return 0x30D9;
    if (base == 0xFF8D && mark == 0xFF9F) return 0x30DA;
    if (base == 0xFF8E && mark == 0xFF9E) return 0x30DC;
    if (base == 0xFF8E && mark == 0xFF9F) return 0x30DD;
    if (base == 0xFF9C && mark == 0xFF9E) return 0x30F7;
    return 0;
}

inline String keyEngineNormalizeJapaneseRun(const String& raw, int cursorChars, int& newCursorChars) {
    String out = "";
    out.reserve(raw.length());
    newCursorChars = 0;
    int inChars = 0;
    for (int i = 0; i < raw.length(); ) {
        int l = 0;
        uint32_t cp = keyEngineCodepointAt(raw, i, l);
        int nextLen = 0;
        uint32_t next = (i + l < raw.length()) ? keyEngineCodepointAt(raw, i + l, nextLen) : 0;
        uint32_t pair = keyEngineJapanesePair(cp, next);
        if (pair != 0) {
            keyEngineAppendUtf8(out, pair);
            if (inChars < cursorChars) newCursorChars++;
            if (inChars + 1 < cursorChars) {
                int shapedChars = keyEngineCharCountBetween(out, 0, out.length());
                if (shapedChars > newCursorChars) newCursorChars = shapedChars;
            }
            i += l + nextLen;
            inChars += 2;
        } else {
            keyEngineAppendUtf8(out, keyEngineJapaneseSingle(cp));
            i += l;
            inChars++;
            if (inChars <= cursorChars) newCursorChars++;
        }
    }
    return out;
}

inline void keyEngineNormalizeJapaneseAroundCursor() {
    int len = fullText.length();
    int start = cursorPos;
    while (start > 0 && cursorPos - start < KEY_ENGINE_MAX_RESHAPE_BYTES) {
        int p = start - 1;
        while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--;
        int l = 0;
        uint32_t cp = keyEngineCodepointAt(fullText, p, l);
        if (!keyEngineJapaneseRunChar(cp)) break;
        start = p;
    }
    int end = cursorPos;
    while (end < len && end - cursorPos < KEY_ENGINE_MAX_RESHAPE_BYTES) {
        int l = 0;
        uint32_t cp = keyEngineCodepointAt(fullText, end, l);
        if (!keyEngineJapaneseRunChar(cp)) break;
        end += l;
    }
    if (start == end) return;
    int cursorChars = keyEngineCharCountBetween(fullText, start, cursorPos);
    int newCursorChars = cursorChars;
    String normalized = keyEngineNormalizeJapaneseRun(fullText.substring(start, end), cursorChars, newCursorChars);
    fullText = fullText.substring(0, start) + normalized + fullText.substring(end);
    cursorPos = start + keyEngineByteOffsetAfterChars(normalized, newCursorChars);
}

inline bool keyEngineComplexBlock(uint32_t cp) {
    return (cp >= 0x0D80 && cp <= 0x0DFF) || (cp >= 0x0E00 && cp <= 0x0E7F) || (cp >= 0x0E80 && cp <= 0x0EFF) || (cp >= 0x0F00 && cp <= 0x0FFF) || (cp >= 0x1000 && cp <= 0x109F) || (cp >= 0x1780 && cp <= 0x17FF);
}

inline bool keyEngineComplexBase(uint32_t cp) {
    if (cp >= 0x0D85 && cp <= 0x0DC6) return true;
    if (cp >= 0x0E01 && cp <= 0x0E30) return true;
    if (cp >= 0x0E81 && cp <= 0x0EB0) return true;
    if (cp >= 0x0F40 && cp <= 0x0F6C) return true;
    if (cp >= 0x1000 && cp <= 0x102A) return true;
    if (cp >= 0x1780 && cp <= 0x17A2) return true;
    if (cp == 0x25CC) return true;
    return false;
}

inline bool keyEngineComplexDependent(uint32_t cp) {
    if (cp == 0x0DCA || (cp >= 0x0DCF && cp <= 0x0DDF) || cp == 0x0DF2 || cp == 0x0DF3) return true;
    if (cp == 0x0E31 || (cp >= 0x0E34 && cp <= 0x0E3A) || (cp >= 0x0E47 && cp <= 0x0E4E)) return true;
    if (cp == 0x0EB1 || (cp >= 0x0EB4 && cp <= 0x0EBC) || (cp >= 0x0EC8 && cp <= 0x0ECD)) return true;
    if ((cp >= 0x0F71 && cp <= 0x0F87) || (cp >= 0x0F90 && cp <= 0x0FBC)) return true;
    if ((cp >= 0x102B && cp <= 0x103E) || (cp >= 0x1056 && cp <= 0x1059) || (cp >= 0x105E && cp <= 0x1060) || (cp >= 0x1062 && cp <= 0x1064) || (cp >= 0x1067 && cp <= 0x106D) || (cp >= 0x1071 && cp <= 0x1074) || (cp >= 0x1082 && cp <= 0x108D) || cp == 0x108F) return true;
    if ((cp >= 0x17B6 && cp <= 0x17D3) || cp == 0x17DD) return true;
    return false;
}

inline bool keyEngineComplexHasBaseBefore(int index) {
    int p = index - 1;
    while (p >= 0) {
        while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--;
        int l = 0;
        uint32_t cp = keyEngineCodepointAt(fullText, p, l);
        if (!keyEngineComplexBlock(cp) && cp != 0x25CC) return false;
        if (keyEngineComplexBase(cp)) return true;
        p--;
    }
    return false;
}

inline void keyEngineStabilizeComplexAt(int index) {
    if (index < 0 || index >= fullText.length()) return;
    int l = 0;
    uint32_t cp = keyEngineCodepointAt(fullText, index, l);
    if (!keyEngineComplexDependent(cp)) return;
    if (keyEngineComplexHasBaseBefore(index)) return;
    keyEngineInsertDottedBefore(index);
}

inline void keyEngineStabilizeComplexAroundCursor() {
    if (cursorPos > 0 && cursorPos <= fullText.length()) {
        int p = cursorPos - 1;
        while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--;
        keyEngineStabilizeComplexAt(p);
    }
    if (cursorPos < fullText.length()) keyEngineStabilizeComplexAt(cursorPos);
}

inline bool keyEngineHebrewMark(uint32_t cp) {
    return (cp >= 0x0591 && cp <= 0x05BD) || cp == 0x05BF || (cp >= 0x05C1 && cp <= 0x05C2) || (cp >= 0x05C4 && cp <= 0x05C5) || cp == 0x05C7;
}

inline bool keyEngineHebrewBase(uint32_t cp) {
    return (cp >= 0x05D0 && cp <= 0x05EA) || (cp >= 0x05F0 && cp <= 0x05F2) || cp == 0x25CC;
}

inline bool keyEngineHebrewHasBaseBefore(int index) {
    int p = index - 1;
    while (p >= 0) {
        while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--;
        int l = 0;
        uint32_t cp = keyEngineCodepointAt(fullText, p, l);
        if (cp < 0x0590 || cp > 0x05FF) return false;
        if (keyEngineHebrewBase(cp)) return true;
        p--;
    }
    return false;
}

inline void keyEngineStabilizeHebrewAt(int index) {
    if (index < 0 || index >= fullText.length()) return;
    int l = 0;
    uint32_t cp = keyEngineCodepointAt(fullText, index, l);
    if (!keyEngineHebrewMark(cp)) return;
    if (keyEngineHebrewHasBaseBefore(index)) return;
    keyEngineInsertDottedBefore(index);
}

inline void keyEngineStabilizeHebrewAroundCursor() {
    if (cursorPos > 0 && cursorPos <= fullText.length()) {
        int p = cursorPos - 1;
        while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--;
        keyEngineStabilizeHebrewAt(p);
    }
    if (cursorPos < fullText.length()) keyEngineStabilizeHebrewAt(cursorPos);
}

inline int keyEngineEthiopicOrder(char c) {
    if (c == 'u') return 1;
    if (c == 'i') return 2;
    if (c == 'a') return 3;
    if (c == 'e') return 4;
    if (c == 'o') return 6;
    return -1;
}

inline bool keyEngineEthiopicSyllable(uint32_t cp) {
    return cp >= 0x1200 && cp <= 0x135A;
}

inline bool keyEngineApplyEthiopicVowel(char c) {
    int order = keyEngineEthiopicOrder(c);
    if (order < 0 || cursorPos <= 0) return false;
    int p = cursorPos - 1;
    while (p > 0 && (((unsigned char)fullText[p] & 0xC0) == 0x80)) p--;
    int l = 0;
    uint32_t cp = keyEngineCodepointAt(fullText, p, l);
    if (!keyEngineEthiopicSyllable(cp)) return false;
    uint32_t base = cp - (cp & 0x07);
    uint32_t next = base + order;
    if (!keyEngineEthiopicSyllable(next)) return false;
    String repl = "";
    keyEngineAppendUtf8(repl, next);
    fullText = fullText.substring(0, p) + repl + fullText.substring(p + l);
    cursorPos = p + repl.length();
    // Ethiopic vowel conversion edits an existing syllable in-place.
    // Force a safe redraw because text length may not change, so tail-render
    // heuristics must not treat it as a no-op.
    forceSafeFullTextRedraw = true;
    return true;
}

inline void keyEngineAfterEdit(KeyEngineScript engine) {
    keyEngineClampCursorToUtf8Boundary();
    // Arabic-script text stays in logical/base Unicode order in fullText.
    // Contextual glyph shaping is applied only when an RTL line is drawn.
    if (engine == KEY_ENGINE_ARABIC) { }
    else if (engine == KEY_ENGINE_INDIC) keyEngineStabilizeIndicAroundCursor();
    else if (engine == KEY_ENGINE_JAPANESE) keyEngineNormalizeJapaneseAroundCursor();
    else if (engine == KEY_ENGINE_THAI || engine == KEY_ENGINE_MYANMAR || engine == KEY_ENGINE_KHMER || engine == KEY_ENGINE_LAO || engine == KEY_ENGINE_TIBETAN || engine == KEY_ENGINE_SINHALA) keyEngineStabilizeComplexAroundCursor();
    else if (engine == KEY_ENGINE_HEBREW) keyEngineStabilizeHebrewAroundCursor();
    keyEngineClampCursorToUtf8Boundary();
}

inline String keyEngineProcessMappedText(KeyEngineScript engine, const String& mappedText) {
    if (engine == KEY_ENGINE_NONE || engine == KEY_ENGINE_KOREAN) return mappedText;
    if (engine == KEY_ENGINE_ETHIOPIC && mappedText.length() == 1) {
        char c = mappedText[0];
        if (keyEngineEthiopicOrder(c) >= 0) {
            if (keyEngineApplyEthiopicVowel(c)) return "";
        }
    }
    return mappedText;
}
