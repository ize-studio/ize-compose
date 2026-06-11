#!/usr/bin/env python3
"""
Noto font downloader and u8g2 binary converter for ZeroWriter
Usage: python make_fonts.py [arabic|indic|sea|misc|all]

파이프라인: TTF(64px 렌더링) -> 8x16 고정셀 BDF -> bdfconv.exe -> C -> bin
"""

import urllib.request
import subprocess
import re
import os
import sys
import shutil
from pathlib import Path
from PIL import Image, ImageFont, ImageDraw

SCRIPT_DIR = Path(__file__).parent
PROJECT_DIR = SCRIPT_DIR.parent
BDFCONV     = str(PROJECT_DIR / "tools" / "u8g2" / "bdfconv.exe")
SRC_DIR     = PROJECT_DIR / "src"
BUILD_DIR   = PROJECT_DIR / "build" / "fontbuild_new"
CACHE_DIR   = PROJECT_DIR / "build" / "noto_fonts"

BUILD_DIR.mkdir(parents=True, exist_ok=True)
CACHE_DIR.mkdir(parents=True, exist_ok=True)

# Fixed glyph cell dimensions (matching existing font_greek_cyrillic_16.bdf)
CELL_W   = 8
CELL_H   = 16
ASCENT   = 13   # rows above baseline
DESCENT  = 3    # rows below baseline (= CELL_H - ASCENT)

# Render at high resolution then scale down for quality
RENDER_SIZE = 64

# ---------------------------------------------------------------------------
# Download URLs
# ---------------------------------------------------------------------------
BASE_GH  = "https://github.com/googlefonts/noto-fonts/raw/main/hinted/ttf"
BASE_ORG = "https://github.com/notofonts"

FONT_URLS = {
    "NotoNaskhArabic":   [f"{BASE_GH}/NotoNaskhArabic/NotoNaskhArabic-Regular.ttf",
                          f"{BASE_ORG}/arabic/raw/main/fonts/NotoNaskhArabic/unhinted/ttf/NotoNaskhArabic-Regular.ttf"],
    "NotoSansHebrew":    [f"{BASE_GH}/NotoSansHebrew/NotoSansHebrew-Regular.ttf",
                          f"{BASE_ORG}/hebrew/raw/main/fonts/NotoSansHebrew/unhinted/ttf/NotoSansHebrew-Regular.ttf"],
    "NotoSansDevanagari":[f"{BASE_GH}/NotoSansDevanagari/NotoSansDevanagari-Regular.ttf",
                          f"{BASE_ORG}/devanagari/raw/main/fonts/NotoSansDevanagari/unhinted/ttf/NotoSansDevanagari-Regular.ttf"],
    "NotoSansBengali":   [f"{BASE_GH}/NotoSansBengali/NotoSansBengali-Regular.ttf",
                          f"{BASE_ORG}/bengali/raw/main/fonts/NotoSansBengali/unhinted/ttf/NotoSansBengali-Regular.ttf"],
    "NotoSansGurmukhi":  [f"{BASE_GH}/NotoSansGurmukhi/NotoSansGurmukhi-Regular.ttf",
                          f"{BASE_ORG}/gurmukhi/raw/main/fonts/NotoSansGurmukhi/unhinted/ttf/NotoSansGurmukhi-Regular.ttf"],
    "NotoSansGujarati":  [f"{BASE_GH}/NotoSansGujarati/NotoSansGujarati-Regular.ttf",
                          f"{BASE_ORG}/gujarati/raw/main/fonts/NotoSansGujarati/unhinted/ttf/NotoSansGujarati-Regular.ttf"],
    "NotoSansTamil":     [f"{BASE_GH}/NotoSansTamil/NotoSansTamil-Regular.ttf",
                          f"{BASE_ORG}/tamil/raw/main/fonts/NotoSansTamil/unhinted/ttf/NotoSansTamil-Regular.ttf"],
    "NotoSansTelugu":    [f"{BASE_GH}/NotoSansTelugu/NotoSansTelugu-Regular.ttf",
                          f"{BASE_ORG}/telugu/raw/main/fonts/NotoSansTelugu/unhinted/ttf/NotoSansTelugu-Regular.ttf"],
    "NotoSansKannada":   [f"{BASE_GH}/NotoSansKannada/NotoSansKannada-Regular.ttf",
                          f"{BASE_ORG}/kannada/raw/main/fonts/NotoSansKannada/unhinted/ttf/NotoSansKannada-Regular.ttf"],
    "NotoSansMalayalam": [f"{BASE_GH}/NotoSansMalayalam/NotoSansMalayalam-Regular.ttf",
                          f"{BASE_ORG}/malayalam/raw/main/fonts/NotoSansMalayalam/unhinted/ttf/NotoSansMalayalam-Regular.ttf"],
    "NotoSansSinhala":   [f"{BASE_GH}/NotoSansSinhala/NotoSansSinhala-Regular.ttf",
                          f"{BASE_ORG}/sinhala/raw/main/fonts/NotoSansSinhala/unhinted/ttf/NotoSansSinhala-Regular.ttf"],
    "NotoSansThai":      [f"{BASE_GH}/NotoSansThai/NotoSansThai-Regular.ttf",
                          f"{BASE_ORG}/thai/raw/main/fonts/NotoSansThai/unhinted/ttf/NotoSansThai-Regular.ttf"],
    "NotoSansLao":       [f"{BASE_GH}/NotoSansLao/NotoSansLao-Regular.ttf",
                          f"{BASE_ORG}/lao/raw/main/fonts/NotoSansLao/unhinted/ttf/NotoSansLao-Regular.ttf"],
    "NotoSansMyanmar":   [f"{BASE_GH}/NotoSansMyanmar/NotoSansMyanmar-Regular.ttf",
                          f"{BASE_ORG}/myanmar/raw/main/fonts/NotoSansMyanmar/unhinted/ttf/NotoSansMyanmar-Regular.ttf"],
    "NotoSansKhmer":     [f"{BASE_GH}/NotoSansKhmer/NotoSansKhmer-Regular.ttf",
                          f"{BASE_ORG}/khmer/raw/main/fonts/NotoSansKhmer/unhinted/ttf/NotoSansKhmer-Regular.ttf"],
    "NotoSansArmenian":  [f"{BASE_GH}/NotoSansArmenian/NotoSansArmenian-Regular.ttf",
                          f"{BASE_ORG}/armenian/raw/main/fonts/NotoSansArmenian/unhinted/ttf/NotoSansArmenian-Regular.ttf"],
    "NotoSansGeorgian":  [f"{BASE_GH}/NotoSansGeorgian/NotoSansGeorgian-Regular.ttf",
                          f"{BASE_ORG}/georgian/raw/main/fonts/NotoSansGeorgian/unhinted/ttf/NotoSansGeorgian-Regular.ttf"],
    "NotoSerifTibetan":  [f"{BASE_GH}/NotoSerifTibetan/NotoSerifTibetan-Regular.ttf",
                          f"{BASE_ORG}/tibetan/raw/main/fonts/NotoSerifTibetan/unhinted/ttf/NotoSerifTibetan-Regular.ttf"],
    "NotoSansEthiopic":  [f"{BASE_GH}/NotoSansEthiopic/NotoSansEthiopic-Regular.ttf",
                          f"{BASE_ORG}/ethiopic/raw/main/fonts/NotoSansEthiopic/unhinted/ttf/NotoSansEthiopic-Regular.ttf"],
}

# ---------------------------------------------------------------------------
# Font groups
# ---------------------------------------------------------------------------
FONT_GROUPS = {
    "arabic": {
        "out_name": "font_arabic",
        "desc": "중동 (Arabic, Hebrew)",
        "fonts": {
            "NotoNaskhArabic": [
                (0x0600, 0x06FF),  # Arabic
                (0x0750, 0x077F),  # Arabic Supplement
                (0xFB50, 0xFDFF),  # Arabic Presentation Forms-A
                (0xFE70, 0xFEFF),  # Arabic Presentation Forms-B
            ],
            "NotoSansHebrew": [
                (0x0590, 0x05FF),  # Hebrew
            ],
        }
    },
    "indic": {
        "out_name": "font_indic",
        "desc": "남아시아 Indic 계열",
        "fonts": {
            "NotoSansDevanagari": [(0x0900, 0x097F)],
            "NotoSansBengali":    [(0x0980, 0x09FF)],
            "NotoSansGurmukhi":   [(0x0A00, 0x0A7F)],
            "NotoSansGujarati":   [(0x0A80, 0x0AFF)],
            "NotoSansTamil":      [(0x0B80, 0x0BFF)],
            "NotoSansTelugu":     [(0x0C00, 0x0C7F)],
            "NotoSansKannada":    [(0x0C80, 0x0CFF)],
            "NotoSansMalayalam":  [(0x0D00, 0x0D7F)],
            "NotoSansSinhala":    [(0x0D80, 0x0DFF)],
        }
    },
    "sea": {
        "out_name": "font_sea",
        "desc": "동남아시아 (Thai, Lao, Myanmar, Khmer)",
        "fonts": {
            "NotoSansThai":    [(0x0E00, 0x0E7F)],
            "NotoSansLao":     [(0x0E80, 0x0EFF)],
            "NotoSansMyanmar": [(0x1000, 0x109F)],
            "NotoSansKhmer":   [(0x1780, 0x17FF)],
        }
    },
    "misc": {
        "out_name": "font_misc",
        "desc": "기타 (Armenian, Georgian, Tibetan, Ethiopic)",
        "fonts": {
            "NotoSansArmenian": [(0x0530, 0x058F)],
            "NotoSansGeorgian": [(0x10A0, 0x10FF)],
            "NotoSerifTibetan": [(0x0F00, 0x0FFF)],
            "NotoSansEthiopic": [(0x1200, 0x137F)],
        }
    },
}

# ---------------------------------------------------------------------------
# Download
# ---------------------------------------------------------------------------
def download_font(font_name):
    dest = CACHE_DIR / f"{font_name}.ttf"
    if dest.exists() and dest.stat().st_size > 10000:
        print(f"  [캐시] {font_name}.ttf")
        return dest

    for url in FONT_URLS.get(font_name, []):
        print(f"  [다운로드] {font_name} <- {url}")
        try:
            req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req, timeout=60) as resp, open(dest, 'wb') as f:
                f.write(resp.read())
            if dest.stat().st_size > 10000:
                print(f"  [완료] {font_name}.ttf ({dest.stat().st_size // 1024}KB)")
                return dest
            else:
                print(f"  [오류] 파일 크기 이상: {dest.stat().st_size}B")
                dest.unlink()
        except Exception as e:
            print(f"  [실패] {url}: {e}")

    print(f"  [오류] {font_name} 다운로드 실패")
    return None

# ---------------------------------------------------------------------------
# Glyph rendering - 8x16 fixed cell
# ---------------------------------------------------------------------------
def render_glyph_8x16(font, cp, fill_width=False):
    """
    Render codepoint cp into fixed 8x16 cell.
    Renders at RENDER_SIZE px for quality, then scales to fit CELL_W x ASCENT.
    Returns list of CELL_H bytes, or None if glyph is empty.
    """
    ch = chr(cp)

    # Large canvas render
    canvas = Image.new('L', (RENDER_SIZE * 2, RENDER_SIZE * 2), 0)
    draw = ImageDraw.Draw(canvas)
    try:
        draw.text((RENDER_SIZE // 4, RENDER_SIZE // 4), ch, font=font, fill=255)
    except Exception:
        return None

    # Find non-zero bounding box
    arr = list(canvas.getdata())
    W, H = canvas.size
    THRESH = 30
    rows_with_ink = [r for r in range(H) if any(arr[r*W+c] > THRESH for c in range(W))]
    cols_with_ink = [c for c in range(W) if any(arr[r*W+c] > THRESH for r in range(H))]

    if not rows_with_ink or not cols_with_ink:
        return None

    r0, r1 = min(rows_with_ink), max(rows_with_ink) + 1
    c0, c1 = min(cols_with_ink), max(cols_with_ink) + 1

    glyph_h = r1 - r0
    glyph_w = c1 - c0
    if glyph_h <= 0 or glyph_w <= 0:
        return None

    # Crop to tight bbox
    cropped = canvas.crop((c0, r0, c1, r1))

    # Scale to fit CELL_W wide and ASCENT tall. Arabic presentation forms need
    # their join strokes to reach the cell edge, so they use the full width.
    scale_x = CELL_W / glyph_w
    scale_y = ASCENT / glyph_h
    scale   = min(scale_x, scale_y)

    new_w = CELL_W if fill_width else max(1, round(glyph_w * scale))
    new_h = max(1, round(glyph_h * scale))
    new_w = min(new_w, CELL_W)
    new_h = min(new_h, ASCENT)

    scaled = cropped.resize((new_w, new_h), Image.LANCZOS)

    # Build 8x16 cell: Arabic is flush to both sides; other scripts stay centered.
    cell = Image.new('L', (CELL_W, CELL_H), 0)
    paste_x = 0 if fill_width else (CELL_W - new_w) // 2
    paste_y = ASCENT - new_h  # align glyph bottom to baseline row
    paste_y = max(0, paste_y)
    cell.paste(scaled, (paste_x, paste_y))

    # Threshold and encode as bytes
    result = []
    for r in range(CELL_H):
        byte_val = 0
        for c in range(CELL_W):
            if cell.getpixel((c, r)) > 100:
                byte_val |= (0x80 >> c)
        result.append(byte_val)

    # Skip if all zero
    if all(b == 0 for b in result):
        return None

    return result

# ---------------------------------------------------------------------------
# BDF generation
# ---------------------------------------------------------------------------
def make_bdf(font_path, codepoint_ranges, out_bdf_path):
    print(f"  [BDF] {font_path.name} -> {out_bdf_path.name}")
    try:
        font = ImageFont.truetype(str(font_path), RENDER_SIZE)
    except Exception as e:
        print(f"  [오류] 폰트 로드 실패: {e}")
        return 0

    chars = []
    for start, end in codepoint_ranges:
        for cp in range(start, end + 1):
            try:
                rows = render_glyph_8x16(font, cp, font_path.stem.startswith("NotoNaskhArabic"))
                if rows is not None:
                    chars.append((cp, rows))
            except Exception:
                pass

    print(f"    -> {len(chars)}개 글리프")
    if not chars:
        return 0

    with open(out_bdf_path, 'w', encoding='ascii', errors='replace') as f:
        f.write("STARTFONT 2.1\n")
        f.write(f"FONT -rupert-Noto-Regular-R-Normal--16-160-75-75-C-80-ISO10646-1\n")
        f.write(f"SIZE 16 75 75\n")
        f.write(f"FONTBOUNDINGBOX {CELL_W} {CELL_H} 0 -{DESCENT}\n")
        f.write("STARTPROPERTIES 6\n")
        f.write(f"FONT_ASCENT {ASCENT}\n")
        f.write(f"FONT_DESCENT {DESCENT}\n")
        f.write(f"PIXEL_SIZE {CELL_H}\n")
        f.write(f"POINT_SIZE 160\n")
        f.write('CHARSET_REGISTRY "ISO10646"\n')
        f.write('CHARSET_ENCODING "1"\n')
        f.write("ENDPROPERTIES\n")
        f.write(f"CHARS {len(chars)}\n")

        for cp, rows in chars:
            f.write(f"STARTCHAR uni{cp:04X}\n")
            f.write(f"ENCODING {cp}\n")
            f.write(f"SWIDTH 500 0\n")
            f.write(f"DWIDTH {CELL_W} 0\n")
            f.write(f"BBX {CELL_W} {CELL_H} 0 -{DESCENT}\n")
            f.write("BITMAP\n")
            for byte_val in rows:
                f.write(f"{byte_val:02X}\n")
            f.write("ENDCHAR\n")

        f.write("ENDFONT\n")

    return len(chars)

# ---------------------------------------------------------------------------
# BDF merge
# ---------------------------------------------------------------------------
def merge_bdfs(bdf_paths, out_bdf_path):
    print(f"  [병합] {len(bdf_paths)}개 BDF -> {out_bdf_path.name}")
    seen = set()
    all_chars = []

    for bdf_path in bdf_paths:
        if not bdf_path.exists():
            continue
        with open(bdf_path, 'r', encoding='ascii', errors='replace') as f:
            content = f.read()
        blocks = re.findall(r'(STARTCHAR.*?ENDCHAR)', content, re.DOTALL)
        for block in blocks:
            m = re.search(r'^ENCODING (\d+)', block, re.MULTILINE)
            if m:
                enc = int(m.group(1))
                if enc not in seen:
                    seen.add(enc)
                    all_chars.append(block)

    with open(out_bdf_path, 'w', encoding='ascii', errors='replace') as f:
        f.write("STARTFONT 2.1\n")
        f.write(f"FONT -rupert-Noto-Regular-R-Normal--16-160-75-75-C-80-ISO10646-1\n")
        f.write(f"SIZE 16 75 75\n")
        f.write(f"FONTBOUNDINGBOX {CELL_W} {CELL_H} 0 -{DESCENT}\n")
        f.write("STARTPROPERTIES 6\n")
        f.write(f"FONT_ASCENT {ASCENT}\n")
        f.write(f"FONT_DESCENT {DESCENT}\n")
        f.write(f"PIXEL_SIZE {CELL_H}\n")
        f.write(f"POINT_SIZE 160\n")
        f.write('CHARSET_REGISTRY "ISO10646"\n')
        f.write('CHARSET_ENCODING "1"\n')
        f.write("ENDPROPERTIES\n")
        f.write(f"CHARS {len(all_chars)}\n")
        for block in all_chars:
            f.write(block + '\n')
        f.write("ENDFONT\n")

    print(f"    -> 총 {len(all_chars)}개 글리프")
    return len(all_chars)

# ---------------------------------------------------------------------------
# BDF -> u8g2 C
# ---------------------------------------------------------------------------
def build_map_arg(codepoint_ranges):
    """bdfconv -m 인수 생성: ASCII 32-127 + 지정된 Unicode 범위 (decimal)"""
    parts = ["32-127"]
    for start, end in codepoint_ranges:
        if end < 32 or start > 127:  # ASCII 외 범위만 추가
            parts.append(f"{start}-{end}")
    return ",".join(parts)


def bdf_to_c(bdf_path, c_path, font_name, codepoint_ranges=None):
    print(f"  [bdfconv] {bdf_path.name} -> {c_path.name}")
    cmd = [BDFCONV, "-f", "1", "-b", "0", "-n", font_name,
           "-o", str(c_path)]
    if codepoint_ranges:
        map_arg = build_map_arg(codepoint_ranges)
        cmd += ["-m", map_arg]
    cmd.append(str(bdf_path))

    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0 or not c_path.exists():
        print(f"  [오류] bdfconv: {r.stderr[:300]}")
        return False
    # Glyphs 카운트 확인
    with open(c_path, 'r') as f:
        hdr = f.read(300)
    m = re.search(r'Glyphs: (\d+)/(\d+)', hdr)
    if m:
        ok, total = m.group(1), m.group(2)
        print(f"    -> Glyphs {ok}/{total}")
        if int(ok) == 0:
            print(f"  [경고] 글리프가 C파일에 하나도 포함되지 않음!")
            return False
    return True

# ---------------------------------------------------------------------------
# C -> binary
# ---------------------------------------------------------------------------
def c_to_bin(c_path, bin_path):
    with open(c_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    match = re.search(
        r'=\s*\n?\s*("(?:[^"\\]|\\.)*"(?:\s*\n?\s*"(?:[^"\\]|\\.)*")*)\s*;',
        content, re.DOTALL
    )
    if not match:
        print(f"  [오류] C 파일에서 데이터 추출 실패")
        return 0

    raw = match.group(1)
    data = bytearray()
    i = 0
    in_str = False
    while i < len(raw):
        c = raw[i]
        if c == '"':
            in_str = not in_str
            i += 1
        elif in_str:
            if c == '\\' and i + 1 < len(raw):
                nx = raw[i+1]
                if nx == 'x' and i + 3 < len(raw):
                    data.append(int(raw[i+2:i+4], 16))
                    i += 4
                elif nx.isdigit():
                    j = i + 1
                    while j < len(raw) and j < i + 4 and raw[j].isdigit():
                        j += 1
                    data.append(int(raw[i+1:j], 8) & 0xFF)
                    i = j
                else:
                    esc = {'n':10,'t':9,'r':13,'\\':92,'"':34,"'":39,
                           '0':0,'a':7,'b':8,'f':12,'v':11}
                    data.append(esc.get(nx, ord(nx) & 0xFF))
                    i += 2
            else:
                data.append(ord(c) & 0xFF)
                i += 1
        else:
            i += 1

    with open(bin_path, 'wb') as f:
        f.write(bytes(data))

    print(f"  [완료] {bin_path.name}: {len(data) // 1024}KB ({len(data)}바이트)")
    return len(data)

# ---------------------------------------------------------------------------
# Build one group
# ---------------------------------------------------------------------------
def build_group(group_name):
    group = FONT_GROUPS[group_name]
    out_name  = group["out_name"]
    desc      = group["desc"]
    fonts_cfg = group["fonts"]

    print(f"\n=== [{group_name.upper()}] {desc} ===")

    per_font_bdfs = []
    for font_name, ranges in fonts_cfg.items():
        font_path = download_font(font_name)
        if font_path is None:
            continue
        bdf_path = BUILD_DIR / f"{out_name}_{font_name}.bdf"
        count = make_bdf(font_path, ranges, bdf_path)
        if count > 0:
            per_font_bdfs.append(bdf_path)

    if not per_font_bdfs:
        print(f"  [실패] 사용 가능한 BDF 없음")
        return False

    merged_bdf = BUILD_DIR / f"{out_name}_16.bdf"
    if len(per_font_bdfs) == 1:
        shutil.copy(per_font_bdfs[0], merged_bdf)
    else:
        merge_bdfs(per_font_bdfs, merged_bdf)

    c_path  = BUILD_DIR / f"{out_name}.c"
    bin_path = SRC_DIR / f"hwalja_{group_name}.bin"

    # 전체 그룹에서 사용된 모든 Unicode 범위 수집
    all_ranges = []
    for ranges in fonts_cfg.values():
        all_ranges.extend(ranges)

    if not bdf_to_c(merged_bdf, c_path, out_name, all_ranges):
        return False

    size = c_to_bin(c_path, bin_path)
    if size == 0:
        return False

    # Clean per-font temp BDFs
    for p in per_font_bdfs:
        if p.exists() and p != merged_bdf:
            p.unlink()

    return True

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    targets = sys.argv[1:] if len(sys.argv) > 1 else ["all"]
    if "all" in targets:
        targets = list(FONT_GROUPS.keys())

    results = {}
    for group in targets:
        if group not in FONT_GROUPS:
            print(f"[알 수 없는 그룹] {group}")
            continue
        results[group] = build_group(group)

    print("\n=== 빌드 결과 ===")
    for g, ok in results.items():
        status = "성공" if ok else "실패"
        print(f"  {g}: {status}")

    print("\n=== src/ 폴더 bin 파일 ===")
    for group_name, group in FONT_GROUPS.items():
        p = SRC_DIR / f"hwalja_{group_name}.bin"
        if p.exists():
            print(f"  {p.name}: {p.stat().st_size // 1024}KB")

if __name__ == "__main__":
    main()
