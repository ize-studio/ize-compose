// Browser-side port of the Hangul composition path from src/jeong_eum.h.
window.IZE_KEY_ENGINE = (() => {
  const choStrs = ["ㄱ", "ㄲ", "ㄴ", "ㄷ", "ㄸ", "ㄹ", "ㅁ", "ㅂ", "ㅃ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅉ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ"];
  const jungStrs = ["ㅏ", "ㅐ", "ㅑ", "ㅒ", "ㅓ", "ㅔ", "ㅕ", "ㅖ", "ㅗ", "ㅘ", "ㅙ", "ㅚ", "ㅛ", "ㅜ", "ㅝ", "ㅞ", "ㅟ", "ㅠ", "ㅡ", "ㅢ", "ㅣ"];

  function makeKorStr(c, ju, jo) {
    if (c < 0 || ju < 0) return "";
    return String.fromCharCode(0xac00 + c * 21 * 28 + ju * 28 + (jo < 0 ? 0 : jo));
  }

  function getCho(c) {
    return ({ r: 0, R: 1, s: 2, e: 3, E: 4, f: 5, a: 6, q: 7, Q: 8, t: 9, T: 10, d: 11, w: 12, W: 13, c: 14, z: 15, x: 16, v: 17, g: 18 })[c] ?? -1;
  }

  function getJung(c) {
    return ({ k: 0, o: 1, i: 2, O: 3, j: 4, p: 5, u: 6, P: 7, h: 8, y: 12, n: 13, b: 17, m: 18, l: 20 })[c] ?? -1;
  }

  function getJong(c) {
    return ({ r: 1, R: 2, s: 4, e: 7, f: 8, a: 16, q: 17, t: 19, T: 20, d: 21, w: 22, c: 23, z: 24, x: 25, v: 26, g: 27 })[c] ?? 0;
  }

  function combineJong(j1, j2) {
    if (j1 === 1 && j2 === 19) return 3;
    if (j1 === 4 && j2 === 22) return 5;
    if (j1 === 4 && j2 === 27) return 6;
    if (j1 === 8 && j2 === 1) return 9;
    if (j1 === 8 && j2 === 16) return 10;
    if (j1 === 8 && j2 === 17) return 11;
    if (j1 === 8 && j2 === 19) return 12;
    if (j1 === 8 && j2 === 25) return 13;
    if (j1 === 8 && j2 === 26) return 14;
    if (j1 === 8 && j2 === 27) return 15;
    if (j1 === 17 && j2 === 19) return 18;
    return -1;
  }

  function combineJung(j1, j2) {
    if (j1 === 8 && j2 === 0) return 9;
    if (j1 === 8 && j2 === 1) return 10;
    if (j1 === 8 && j2 === 20) return 11;
    if (j1 === 13 && j2 === 4) return 14;
    if (j1 === 13 && j2 === 5) return 15;
    if (j1 === 13 && j2 === 20) return 16;
    if (j1 === 18 && j2 === 20) return 19;
    return -1;
  }

  function createHangulEngine({ insertText }) {
    const state = { cho: -1, jung: -1, jong: -1, lastJongChar: "" };

    function composingText() {
      if (state.cho !== -1 && state.jung !== -1) return makeKorStr(state.cho, state.jung, state.jong);
      if (state.cho !== -1) return choStrs[state.cho] || "";
      if (state.jung !== -1) return jungStrs[state.jung] || "";
      return "";
    }

    function clear() {
      state.cho = -1;
      state.jung = -1;
      state.jong = -1;
      state.lastJongChar = "";
    }

    function flush() {
      const text = composingText();
      clear();
      if (text) insertText(text);
    }

    function backspace() {
      if (state.cho === -1 && state.jung === -1) return false;
      clear();
      return true;
    }

    function process(real) {
      if (!real || real.length !== 1) {
        flush();
        if (real) insertText(real);
        return;
      }

      const ci = getCho(real);
      const ji = getJung(real);
      const joi = getJong(real);

      if (ci !== -1) {
        if (state.cho === -1 && state.jung === -1) state.cho = ci;
        else if (state.cho !== -1 && state.jung === -1) {
          flush();
          state.cho = ci;
        } else if (state.cho !== -1 && state.jung !== -1 && state.jong === -1) {
          if (joi > 0) {
            state.jong = joi;
            state.lastJongChar = real;
          } else {
            flush();
            state.cho = ci;
          }
        } else if (state.cho !== -1 && state.jung !== -1 && state.jong !== -1) {
          const comb = combineJong(state.jong, joi);
          if (comb !== -1) {
            state.jong = comb;
            state.lastJongChar = real;
          } else {
            flush();
            state.cho = ci;
          }
        } else if (state.cho === -1 && state.jung !== -1) {
          flush();
          state.cho = ci;
        }
      } else if (ji !== -1) {
        if (state.cho !== -1 && state.jung === -1) state.jung = ji;
        else if (state.cho !== -1 && state.jung !== -1 && state.jong === -1) {
          const comb = combineJung(state.jung, ji);
          if (comb !== -1) state.jung = comb;
          else {
            flush();
            state.jung = ji;
          }
        } else if (state.cho !== -1 && state.jung !== -1 && state.jong !== -1) {
          const prev = state.lastJongChar;
          if (state.jong === 3) state.jong = 1;
          else if (state.jong === 5 || state.jong === 6) state.jong = 4;
          else if (state.jong === 18) state.jong = 17;
          else if (state.jong >= 9 && state.jong <= 15) state.jong = 8;
          else state.jong = -1;
          flush();
          state.cho = getCho(prev);
          state.jung = ji;
        } else if (state.cho === -1) {
          if (state.jung === -1) state.jung = ji;
          else {
            const comb = combineJung(state.jung, ji);
            if (comb !== -1) state.jung = comb;
            else {
              flush();
              state.jung = ji;
            }
          }
        }
      } else {
        flush();
        insertText(real);
      }
    }

    return { process, flush, backspace, composingText, clear };
  }

  const japaneseSingle = new Map([
    [0xff61, 0x3002], [0xff62, 0x300c], [0xff63, 0x300d], [0xff64, 0x3001], [0xff65, 0x30fb],
    [0xff66, 0x30f2], [0xff67, 0x30a1], [0xff68, 0x30a3], [0xff69, 0x30a5], [0xff6a, 0x30a7],
    [0xff6b, 0x30a9], [0xff6c, 0x30e3], [0xff6d, 0x30e5], [0xff6e, 0x30e7], [0xff6f, 0x30c3],
    [0xff70, 0x30fc], [0xff71, 0x30a2], [0xff72, 0x30a4], [0xff73, 0x30a6], [0xff74, 0x30a8],
    [0xff75, 0x30aa], [0xff76, 0x30ab], [0xff77, 0x30ad], [0xff78, 0x30af], [0xff79, 0x30b1],
    [0xff7a, 0x30b3], [0xff7b, 0x30b5], [0xff7c, 0x30b7], [0xff7d, 0x30b9], [0xff7e, 0x30bb],
    [0xff7f, 0x30bd], [0xff80, 0x30bf], [0xff81, 0x30c1], [0xff82, 0x30c4], [0xff83, 0x30c6],
    [0xff84, 0x30c8], [0xff85, 0x30ca], [0xff86, 0x30cb], [0xff87, 0x30cc], [0xff88, 0x30cd],
    [0xff89, 0x30ce], [0xff8a, 0x30cf], [0xff8b, 0x30d2], [0xff8c, 0x30d5], [0xff8d, 0x30d8],
    [0xff8e, 0x30db], [0xff8f, 0x30de], [0xff90, 0x30df], [0xff91, 0x30e0], [0xff92, 0x30e1],
    [0xff93, 0x30e2], [0xff94, 0x30e4], [0xff95, 0x30e6], [0xff96, 0x30e8], [0xff97, 0x30e9],
    [0xff98, 0x30ea], [0xff99, 0x30eb], [0xff9a, 0x30ec], [0xff9b, 0x30ed], [0xff9c, 0x30ef],
    [0xff9d, 0x30f3]
  ]);

  function japanesePair(base, mark) {
    const pairs = {
      "65382,65438": 0x30fa, "65395,65438": 0x30f4,
      "65398,65438": 0x30ac, "65399,65438": 0x30ae, "65400,65438": 0x30b0, "65401,65438": 0x30b2, "65402,65438": 0x30b4,
      "65403,65438": 0x30b6, "65404,65438": 0x30b8, "65405,65438": 0x30ba, "65406,65438": 0x30bc, "65407,65438": 0x30be,
      "65408,65438": 0x30c0, "65409,65438": 0x30c2, "65410,65438": 0x30c5, "65411,65438": 0x30c7, "65412,65438": 0x30c9,
      "65418,65438": 0x30d0, "65418,65439": 0x30d1, "65419,65438": 0x30d3, "65419,65439": 0x30d4,
      "65420,65438": 0x30d6, "65420,65439": 0x30d7, "65421,65438": 0x30d9, "65421,65439": 0x30da,
      "65422,65438": 0x30dc, "65422,65439": 0x30dd, "65436,65438": 0x30f7
    };
    return pairs[`${base},${mark}`] || 0;
  }

  function normalizeJapanese(text) {
    const cps = Array.from(text, (ch) => ch.codePointAt(0));
    let out = "";
    for (let i = 0; i < cps.length; i++) {
      const pair = i + 1 < cps.length ? japanesePair(cps[i], cps[i + 1]) : 0;
      if (pair) {
        out += String.fromCodePoint(pair);
        i++;
      } else {
        out += String.fromCodePoint(japaneseSingle.get(cps[i]) || cps[i]);
      }
    }
    return out;
  }

  function isIndicBlock(cp) {
    return (cp >= 0x0900 && cp <= 0x097f) || (cp >= 0x0980 && cp <= 0x09ff) ||
      (cp >= 0x0a00 && cp <= 0x0a7f) || (cp >= 0x0a80 && cp <= 0x0aff) ||
      (cp >= 0x0b80 && cp <= 0x0bff) || (cp >= 0x0c00 && cp <= 0x0c7f) ||
      (cp >= 0x0c80 && cp <= 0x0cff) || (cp >= 0x0d00 && cp <= 0x0d7f);
  }

  function isIndicDependent(cp) {
    const low = cp & 0x7f;
    return isIndicBlock(cp) && (low <= 0x03 || low === 0x3c || (low >= 0x3e && low <= 0x4d) ||
      (low >= 0x55 && low <= 0x57) || (low >= 0x62 && low <= 0x63));
  }

  function isIndicLetter(cp) {
    const low = cp & 0x7f;
    return isIndicBlock(cp) && ((low >= 0x04 && low <= 0x39) || (low >= 0x58 && low <= 0x61));
  }

  function isComplexBlock(cp) {
    return (cp >= 0x0d80 && cp <= 0x0dff) || (cp >= 0x0e00 && cp <= 0x0e7f) ||
      (cp >= 0x0e80 && cp <= 0x0eff) || (cp >= 0x0f00 && cp <= 0x0fff) ||
      (cp >= 0x1000 && cp <= 0x109f) || (cp >= 0x1780 && cp <= 0x17ff);
  }

  function isComplexBase(cp) {
    return (cp >= 0x0d85 && cp <= 0x0dc6) || (cp >= 0x0e01 && cp <= 0x0e30) ||
      (cp >= 0x0e81 && cp <= 0x0eb0) || (cp >= 0x0f40 && cp <= 0x0f6c) ||
      (cp >= 0x1000 && cp <= 0x102a) || (cp >= 0x1780 && cp <= 0x17a2) || cp === 0x25cc;
  }

  function isComplexDependent(cp) {
    if (cp === 0x0dca || (cp >= 0x0dcf && cp <= 0x0ddf) || cp === 0x0df2 || cp === 0x0df3) return true;
    if (cp === 0x0e31 || (cp >= 0x0e34 && cp <= 0x0e3a) || (cp >= 0x0e47 && cp <= 0x0e4e)) return true;
    if (cp === 0x0eb1 || (cp >= 0x0eb4 && cp <= 0x0ebc) || (cp >= 0x0ec8 && cp <= 0x0ecd)) return true;
    if ((cp >= 0x0f71 && cp <= 0x0f87) || (cp >= 0x0f90 && cp <= 0x0fbc)) return true;
    if ((cp >= 0x102b && cp <= 0x103e) || (cp >= 0x1056 && cp <= 0x1059) || (cp >= 0x105e && cp <= 0x1060) ||
      (cp >= 0x1062 && cp <= 0x1064) || (cp >= 0x1067 && cp <= 0x106d) || (cp >= 0x1071 && cp <= 0x1074) ||
      (cp >= 0x1082 && cp <= 0x108d) || cp === 0x108f) return true;
    return (cp >= 0x17b6 && cp <= 0x17d3) || cp === 0x17dd;
  }

  function isHebrewMark(cp) {
    return (cp >= 0x0591 && cp <= 0x05bd) || cp === 0x05bf || (cp >= 0x05c1 && cp <= 0x05c2) ||
      (cp >= 0x05c4 && cp <= 0x05c5) || cp === 0x05c7;
  }

  function isHebrewBase(cp) {
    return (cp >= 0x05d0 && cp <= 0x05ea) || (cp >= 0x05f0 && cp <= 0x05f2) || cp === 0x25cc;
  }

  function lastCodePoint(text) {
    const chars = Array.from(text);
    if (!chars.length) return 0;
    return chars[chars.length - 1].codePointAt(0);
  }

  function needsDotted(layoutId, text, mappedText) {
    if (!mappedText) return false;
    const cp = Array.from(mappedText)[0]?.codePointAt(0) || 0;
    const prev = lastCodePoint(text);
    if (["KB_BENGALI", "KB_DEVANAGARI", "KB_GUJARATI", "KB_KANNADA", "KB_MALAYALAM", "KB_NEPALI", "KB_PUNJABI", "KB_TAMIL", "KB_TELUGU"].includes(layoutId)) {
      return isIndicDependent(cp) && !(isIndicLetter(prev) || prev === 0x25cc);
    }
    if (["KB_THAI", "KB_MYANMAR", "KB_KHMER", "KB_LAO", "KB_TIBETAN", "KB_SINHALA"].includes(layoutId)) {
      return isComplexDependent(cp) && !(isComplexBase(prev));
    }
    if (layoutId === "KB_HEBREW") {
      return isHebrewMark(cp) && !(isHebrewBase(prev));
    }
    return false;
  }

  function ethiopicOrder(c) {
    if (c === "u") return 1;
    if (c === "i") return 2;
    if (c === "a") return 3;
    if (c === "e") return 4;
    if (c === "o") return 6;
    return -1;
  }

  function applyEthiopicVowel(text, c) {
    const order = ethiopicOrder(c);
    if (order < 0) return null;
    const chars = Array.from(text);
    if (!chars.length) return null;
    const cp = chars[chars.length - 1].codePointAt(0);
    if (cp < 0x1200 || cp > 0x135a) return null;
    const next = (cp - (cp & 0x07)) + order;
    if (next < 0x1200 || next > 0x135a) return null;
    chars[chars.length - 1] = String.fromCodePoint(next);
    return chars.join("");
  }

  function processMappedText(layoutId, text) {
    if (layoutId === "KB_JAPAN") return normalizeJapanese(text);
    return text;
  }

  return { createHangulEngine, processMappedText, needsDotted, applyEthiopicVowel };
})();
