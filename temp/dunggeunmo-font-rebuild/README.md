# Ize Compose 둥근모 폰트 변환 재현 메모

이 문서는 2026-05-24 전후에 둥근모(`DungGeunMo.ttf`)로 만든 Ize Compose용 폰트 리소스를 다른 컴퓨터에서 다른 TTF로 다시 만들기 위한 기록입니다. 단순한 변환 프로그램 사용법이 아니라, 당시 실제 산출물과 코드에 남아 있는 설정값을 기준으로 정리했습니다.

## 확인된 원본 위치

당시 작업 디렉터리:

```text
C:\zerowriter\20260528_IzeComposeV1.1.0
```

남아 있는 핵심 파일:

```text
others\DungGeunMo.ttf
tools\u8g2\bdfconv.exe
build\fontbuild_dunggeunmo\font_hangul_16.bdf
build\fontbuild_dunggeunmo\font_jamo_16.bdf
build\fontbuild_dunggeunmo\font_hangul.c
build\fontbuild_dunggeunmo\font_jamo.c
src\hwalja_hangul.bin
src\hwalja_jamo.bin
```

최종 산출물 크기:

```text
src\hwalja_hangul.bin  355674 bytes
src\hwalja_jamo.bin      6624 bytes
```

## 둥근모 변환값

둥근모 산출물은 Noto 계열용 `tools\make_fonts.py`의 `8x16`, `ASCENT=13`, `DESCENT=3` 설정과 다릅니다. 둥근모 한글/자모는 원본을 `16x16` 고정폭 셀로 썼습니다.

`font_hangul_16.bdf`와 `font_jamo_16.bdf`의 헤더는 다음 값이었습니다.

```bdf
SIZE 16 75 75
FONTBOUNDINGBOX 16 16 0 0
FONT_ASCENT 16
FONT_DESCENT 0
DWIDTH 16 0
BBX 16 16 0 0
```

즉 둥근모 쪽은 baseline 아래 여백을 `3px` 주는 방식이 아니라, BDF 좌표 자체를 `0` 기준으로 둔 `16x16` 풀셀 방식입니다.

중요한 구분:

```text
둥근모 한글/자모:
  CELL 16x16
  FONTBOUNDINGBOX 16 16 0 0
  FONT_ASCENT 16
  FONT_DESCENT 0
  BBX 16 16 0 0

Noto 추가 스크립트 계열:
  CELL_W 8
  CELL_H 16
  ASCENT 13
  DESCENT 3
  FONTBOUNDINGBOX 8 16 0 -3
  BBX 8 16 0 -3
```

사용자가 기억하신 “몇 픽셀 아래로 내림” 보정은 Noto 계열 추가 폰트 스크립트에 남아 있습니다. `tools\make_fonts.py`에서 `DESCENT=3`, `FONTBOUNDINGBOX ... 0 -3`, `BBX ... 0 -3`로 처리했고, 렌더링 코드도 `paste_y = ASCENT - new_h`로 glyph bottom을 baseline에 맞췄습니다. 둥근모 최종 산출물 자체는 `BBX ... 0 0`입니다.

## 포함한 글자 범위

둥근모 한글 완성형:

```text
U+AC00 - U+D7A3
총 11172자
```

`font_hangul.c` 헤더:

```text
Glyphs: 11172/11172
const uint8_t font_hangul[355674]
```

둥근모 자모:

```text
총 464자
```

실제 범위는 다음 조합으로 보면 됩니다.

```text
U+1100 - U+11FF   Hangul Jamo
U+3130 - U+318F   Hangul Compatibility Jamo
U+A960 - U+A97F   Hangul Jamo Extended-A
U+D7B0 - U+D7FF   Hangul Jamo Extended-B
```

`font_jamo.c` 헤더:

```text
Glyphs: 464/464
const uint8_t font_jamo[6624]
```

## bdfconv 변환 설정

둥근모 BDF를 u8g2 C 배열로 만들 때 배열 이름은 파일명과 다릅니다.

```text
font_hangul.c -> const uint8_t font_hangul[355674]
font_jamo.c   -> const uint8_t font_jamo[6624]
```

재현 명령은 다음 형태로 맞추면 됩니다.

```powershell
tools\u8g2\bdfconv.exe -f 1 -b 0 -m "44032-55203" -n font_hangul -o build\fontbuild_dunggeunmo\font_hangul.c build\fontbuild_dunggeunmo\font_hangul_16.bdf
```

```powershell
tools\u8g2\bdfconv.exe -f 1 -b 0 -m "4352-4607,12592-12687,43360-43391,55216-55295" -n font_jamo -o build\fontbuild_dunggeunmo\font_jamo.c build\fontbuild_dunggeunmo\font_jamo_16.bdf
```

옵션 의미:

```text
-f 1      u8g2 font format 1
-b 0      BBX build mode 0
-m        포함할 Unicode 범위
-n        C 배열 이름
-o        출력 C 파일
```

`font_hangul.c`에는 `#ifdef U8G2_USE_LARGE_FONTS`가 붙어 있었습니다. 빌드 코드에서 C 파일을 직접 쓰는 것이 아니라 bin으로 변환해 SD/PSRAM에서 읽는 구조라면 이 매크로는 최종 bin 사용에는 직접 영향이 없습니다.

## C 배열을 bin으로 만들 때의 주의점

Ize Compose는 최종적으로 `.c` 파일이 아니라 다음 bin을 읽습니다.

```text
/ize_compose/hwalja/hwalja_hangul.bin
/ize_compose/hwalja/hwalja_jamo.bin
```

중요한 점은 C 소스 전체를 바이너리로 저장하면 안 된다는 것입니다. C 배열의 문자열 리터럴 안에 있는 u8g2 바이트만 추출해야 합니다.

당시 `tools\make_fonts.py`의 `c_to_bin()`이 이 작업을 합니다. 핵심은 정규식으로 `= "...";` 부분만 잡고, C escape를 실제 바이트로 복원하는 것입니다.

반드시 지켜야 할 것:

```text
잘못된 방식:
  font_hangul.c 파일 내용을 그대로 .bin으로 저장
  배열 이름 문자열까지 bin 앞에 들어감

올바른 방식:
  C 문자열 리터럴 내부의 바이트만 추출
  결과 파일명은 hwalja_hangul.bin, hwalja_jamo.bin
```

## 문자열 접두어 문제와 펌웨어 보정

사용자께서 기억하신 “문자열이 잘못 들어가서 수정한 것”은 이 부분입니다.

펌웨어 `src\IZEcompose.ino`에는 폰트 bin 앞에 잘못 붙은 접두어를 제거하는 보정 코드가 남아 있습니다.

```cpp
const char* jamoPrefix = "font_jamo";
const char* hangulPrefix = "font_hangul";
const char* newJamoPrefix = "hwalja_jamo";
const char* newHangulPrefix = "hwalja_hangul";
```

그리고 파일 경로에 따라 다음 접두어가 bin 앞에 있으면 `memmove()`로 제거합니다.

```text
hwalja_jamo.bin   앞의 "font_jamo" 또는 "hwalja_jamo"
hwalja_hangul.bin 앞의 "font_hangul" 또는 "hwalja_hangul"
```

재작업할 때는 이 보정에 의존하지 말고, 애초에 접두어 없는 순수 u8g2 바이트 bin을 만들어야 합니다. 그래도 다른 컴퓨터에서 예전 방식으로 잘못 만든 bin이 들어가면 Ize Compose 펌웨어가 위 네 문자열 접두어는 제거할 수 있습니다.

## 다른 TTF로 다시 만들 때 적용할 값

둥근모와 같은 16x16 한글 폰트를 목표로 하면 BDF 헤더를 아래와 같이 맞춥니다.

```bdf
FONTBOUNDINGBOX 16 16 0 0
FONT_ASCENT 16
FONT_DESCENT 0
DWIDTH 16 0
BBX 16 16 0 0
```

만약 다른 TTF가 위로 뜨거나 아래가 잘리면 두 갈래 중 하나를 선택합니다.

1. 둥근모 방식 유지
   - glyph bitmap을 16x16 안에서 직접 아래/위로 이동합니다.
   - BDF 헤더는 `0 0` 그대로 둡니다.
   - 최종 Ize Compose 호환성은 가장 예측 가능합니다.

2. Noto 계열 방식 사용
   - `ASCENT=13`, `DESCENT=3`으로 둡니다.
   - BDF 헤더는 `FONTBOUNDINGBOX 8 16 0 -3`, `BBX 8 16 0 -3`처럼 baseline 아래 3픽셀을 엽니다.
   - 이 방식은 8픽셀 폭 추가 문자군용으로 남아 있던 설정입니다. 둥근모 한글 16x16과 혼용하면 폭 계산을 반드시 확인해야 합니다.

한글 본문용으로는 1번, 즉 둥근모와 같은 16x16 방식을 우선 사용하십시오.

## 재현 절차

1. 새 TTF를 준비합니다.

```text
others\NewFont.ttf
```

2. 완성형 한글 BDF를 만듭니다.

필수 출력 조건:

```text
font_hangul_16.bdf
CHARS 11172
ENCODING 44032부터 55203까지
DWIDTH 16 0
BBX 16 16 0 0
```

3. 자모 BDF를 만듭니다.

필수 출력 조건:

```text
font_jamo_16.bdf
CHARS 464
DWIDTH 16 0
BBX 16 16 0 0
```

4. `bdfconv.exe`로 C 배열을 만듭니다.

```powershell
tools\u8g2\bdfconv.exe -f 1 -b 0 -m "44032-55203" -n font_hangul -o build\fontbuild_dunggeunmo\font_hangul.c build\fontbuild_dunggeunmo\font_hangul_16.bdf
```

```powershell
tools\u8g2\bdfconv.exe -f 1 -b 0 -m "4352-4607,12592-12687,43360-43391,55216-55295" -n font_jamo -o build\fontbuild_dunggeunmo\font_jamo.c build\fontbuild_dunggeunmo\font_jamo_16.bdf
```

5. C 배열 문자열만 추출해서 bin으로 저장합니다.

출력 파일명:

```text
src\hwalja_hangul.bin
src\hwalja_jamo.bin
```

6. Ize Compose SD 카드 또는 업데이트 화면에 넣을 때 파일명은 반드시 다음 이름을 씁니다.

```text
hwalja_hangul.bin
hwalja_jamo.bin
```

7. 펌웨어 쪽 경로는 다음과 맞아야 합니다.

```text
/ize_compose/hwalja/hwalja_hangul.bin
/ize_compose/hwalja/hwalja_jamo.bin
```

## 검증 기준

완성형 한글:

```text
Glyphs: 11172/11172
결과 bin 약 355KB 전후
```

자모:

```text
Glyphs: 464/464
결과 bin 약 6.5KB 전후
```

새 폰트라면 압축 결과 크기는 달라질 수 있지만, glyph 수는 위와 같아야 합니다. glyph 수가 줄면 BDF 생성 범위나 폰트 지원 범위가 잘못된 것입니다.

화면 검증 문자열:

```text
가각간갇갈감갑값갓갔강갖같갚갛
한글 둥근모 테스트 123 ABC
ㄱㄲㄳㄴㄵㄶㄷㄸㄹㄺㄻㄼㄽㄾㄿㅀ
ㅏㅐㅑㅒㅓㅔㅕㅖㅗㅘㅙㅚㅛㅜㅝㅞㅟㅠㅡㅢㅣ
```

확인할 것:

```text
받침이 아래에서 잘리지 않는지
초성이 위로 붙거나 떠 보이지 않는지
한글 폭이 16픽셀 고정으로 유지되는지
영문/숫자와 섞어도 줄바꿈 계산이 크게 어긋나지 않는지
```

## 요약

둥근모 최종 한글/자모는 `16x16`, `ASCENT 16`, `DESCENT 0`, `BBX 16 16 0 0`입니다. Noto 추가 폰트 스크립트에는 `DESCENT=3` 보정이 남아 있지만, 둥근모 최종 산출물에는 적용되지 않았습니다.

잘못 들어갔던 문자열 문제는 bin 앞에 `font_hangul`, `font_jamo`, `hwalja_hangul`, `hwalja_jamo` 같은 접두어가 붙는 문제였고, 펌웨어에서 이를 제거하는 보정이 들어갔습니다. 새로 만들 때는 C 배열 이름이 아니라 C 문자열 리터럴의 바이트만 추출해서 `hwalja_hangul.bin`, `hwalja_jamo.bin`으로 저장해야 합니다.

