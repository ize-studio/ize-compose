# Ize Compose Emulator

Ize Compose Emulator는 Ize Compose/Zerowriter 계열 장치의 화면, 키보드, 문서 목록, 네트워크 메뉴, 웹 관리 화면을 브라우저에서 확인하기 위한 HTML 기반 에뮬레이터입니다. 실제 펌웨어를 완전히 대체하는 프로그램이 아니라, 펌웨어의 주요 사용 경험을 웹 브라우저 안에서 재현하고 테스트하기 위한 시뮬레이션입니다.

이 문서는 펌웨어 `1.4.1` 기준의 에뮬레이터 동작을 설명합니다.

현재 저장소 구조는 다음과 같습니다.

```text
codes/
  index.html              # 에뮬레이터 본체
  key-engine.js           # 한글 조합 및 키 처리 보조 엔진
  keyboard-layouts.js     # 지원 키보드 레이아웃 데이터
others/
  initial.png             # 전원/잠자기 화면에 쓰는 렌더링 이미지
ize-compose-emulator.md   # 이 문서
```

## 실행 방법

브라우저에서 `codes/index.html`을 열면 바로 실행됩니다. 별도의 빌드 과정은 없습니다.

GitHub에서 에뮬레이터 파일을 받을 때는 파일을 하나씩 미리보기로 열어서 저장하지 않습니다. 반드시 압축파일로 한 번에 받은 뒤 압축을 풀어야 합니다.

GitHub에서 받는 순서:

1. GitHub 저장소 페이지를 엽니다.
2. `Code` 버튼을 누릅니다.
3. `Download ZIP`을 선택합니다.
4. 컴퓨터에서 받을 폴더를 정합니다.
5. ZIP 파일을 그 폴더에 저장합니다.
6. 저장된 ZIP 파일을 압축 해제합니다.
7. 압축 해제된 폴더 안에서 `codes/`, `others/`, `ize-compose-emulator.md`가 같은 상위 폴더에 있는지 확인합니다.
8. `codes/index.html`을 브라우저에서 엽니다.

`codes/index.html`, `codes/key-engine.js`, `codes/keyboard-layouts.js`, `others/initial.png`는 서로 상대경로로 연결되어 있습니다. 이 파일들을 따로따로 다른 폴더에 저장하면 이미지나 키보드 레이아웃이 깨질 수 있습니다. 항상 압축을 푼 폴더 구조를 유지합니다.

권장 사용 순서:

1. `codes/index.html`을 브라우저에서 엽니다.
2. 전원 스위치로 에뮬레이터를 처음 켭니다.
3. 이때 SD 루트/저장소 폴더 권한 안내가 뜰 수 있습니다.
4. 문서를 실제 폴더에 저장하려면 `Select Folder`를 눌러 폴더 권한을 허가합니다.
5. 브라우저 안에만 저장해도 된다면 닫아도 됩니다.
6. 전자잉크 화면 영역을 클릭해 포커스를 줍니다.
7. 실제 키보드로 입력하거나 화면의 가상 키보드를 클릭합니다.

브라우저 보안 정책 때문에 소리는 첫 사용자 입력 이후부터 정상적으로 재생됩니다. 키보드 입력음과 전원 스위치음은 외부 오디오 파일 없이 Web Audio로 합성됩니다.

## 기본 화면 구성

에뮬레이터는 크게 세 영역으로 나뉩니다.

- 전자잉크 화면: 실제 장치 화면처럼 텍스트, 메뉴, 네트워크 상태, 잠자기 화면을 렌더링합니다.
- 가상 키보드: 실제 키 배열을 흉내 낸 클릭 가능한 키보드입니다.
- 사이드 패널: 브라우저용 보조 컨트롤입니다. 메뉴, 저장, 잠자기, 새 문서, 검색, 웹 화면 열기, 글자 크기, 줄 간격, 언어 선택, 문서 목록, 텍스트 가져오기를 제공합니다.

검은 키는 입력 신호가 들어오는 동안 보라색으로, 흰 키는 노란색으로 표시됩니다. 물리 키보드 입력과 화면 키보드 입력 모두 같은 입력 경로를 사용합니다.

## 글쓰기 화면

전자잉크 화면을 클릭한 뒤 타이핑하면 현재 문서에 글자가 입력됩니다. 입력된 내용은 현재 문서 객체에 반영되고 브라우저 저장소에 보존됩니다.

주요 동작:

| 동작 | 설명 |
| --- | --- |
| 일반 문자 입력 | 현재 커서 위치에 문자 입력 |
| `Enter` | 줄바꿈 입력 |
| `Backspace` | 커서 앞 문자 삭제 |
| `Tab` | 공백 4칸 입력 |
| 방향키 좌/우 | 커서 한 글자 이동 |
| `Home` / `End` | 문서 처음/끝으로 이동 |
| `Ctrl + Backspace` | 커서 앞 단어 삭제 |
| `Ctrl + ←` / `Ctrl + →` | 단어 단위 커서 이동 |
| `Ctrl + ↑` / `Ctrl + ↓` | 문서 처음/끝으로 이동 |
| `Ctrl + S` | 현재 문서 저장 |
| `Ctrl + F` | 검색 모드 |
| `Ctrl + L` | 잠자기 |
| `Ctrl + Space` | 영어/한국어 입력 전환 |
| `Esc` 또는 `Menu` | 메뉴 열기 |

입력 큐로 들어간 글자는 화면에 바로 한꺼번에 나오지 않고 지연 표시됩니다. 현재 기본 지연은 글자당 0.55초입니다.

상태바에는 입력 언어, 글자/단어 카운트, 현재 파일명, 저장 표시, 배터리 표시가 렌더링됩니다. 배터리 표시는 시뮬레이션 값입니다.

## 언어와 키보드 레이아웃

지원 언어 목록은 `codes/keyboard-layouts.js`에 들어 있습니다. 한국어 레이아웃은 `KB_KOREAN`이며 메뉴와 웹 설정에는 `한국어`로 표시됩니다.

언어 설정은 두 가지 경로에서 바꿀 수 있습니다.

- 사이드 패널의 Settings > Language
- 웹 관리 화면의 Settings & Update > Environment Settings > Language

한국어 입력은 키보드 이벤트를 QWERTY 기준으로 해석한 뒤 한글 조합 엔진을 거칩니다. 한글 조합 중 `Backspace`는 조합 중인 글자를 먼저 처리합니다.

## 메뉴 사용법

`Esc`, `Menu` 키 또는 사이드 패널의 `Menu` 버튼으로 메뉴를 엽니다. 메뉴는 왼쪽 명령 목록과 오른쪽 문서 목록으로 구성됩니다.

기본 메뉴 항목:

| 항목 | 설명 |
| --- | --- |
| `Sync` | GitHub 동기화 상태 화면으로 이동 |
| `New` | 새 `docNNNN.txt` 문서 생성 |
| `Save` | 현재 문서 저장 |
| `Count` | 카운트 표시 모드 전환 |
| `Sleep` | 잠자기 모드 진입 |
| `Network` | 네트워크 모드 선택 |

메뉴 조작:

| 키 | 동작 |
| --- | --- |
| `↑` / `↓` | 왼쪽 메뉴 또는 오른쪽 문서 목록에서 이동 |
| `←` / `→` | 왼쪽 메뉴와 오른쪽 문서 목록 사이 포커스 이동 |
| `Enter` | 선택 항목 실행 또는 문서 열기 |
| `Tab` / `Esc` | 메뉴 닫기 또는 현재 편집/삭제 흐름 취소 |
| `Backspace` / `Delete` | 문서 목록 포커스에서 삭제 요청 시작 |

문서 삭제는 바로 실행되지 않습니다.

1. 문서 목록에 포커스를 둡니다.
2. `Backspace` 또는 `Delete`를 누릅니다.
3. `Delete? Enter: code / Tab: cancel` 안내가 표시됩니다.
4. `Enter`를 누르면 6자리 삭제 코드가 표시됩니다.
5. 같은 6자리 숫자를 입력하고 `Enter`를 누르면 삭제됩니다.
6. `Tab`, `Esc`, 다시 `Backspace`, 방향키 등은 삭제 흐름을 취소합니다.

마지막 문서를 삭제하면 빈 새 문서가 자동으로 생성됩니다.

## 검색 모드

`Ctrl + F` 또는 사이드 패널의 `Search` 버튼으로 검색 모드에 들어갑니다.

검색 모드 조작:

| 키 | 동작 |
| --- | --- |
| 일반 문자 | 검색어 입력 |
| `Backspace` | 검색어 한 글자 삭제 |
| `Enter` | 다음 일치 항목 검색 |
| `Esc` | 검색 모드 종료 |

검색 결과가 있으면 커서가 일치 위치로 이동하고 화면에 강조 표시됩니다.

## 잠자기와 전원

잠자기는 다음 경로로 실행할 수 있습니다.

- 사이드 패널의 `Sleep`
- 메뉴의 `Sleep`
- `Ctrl + L`
- 전원 스위치

잠자기 진입 시 현재 문서를 저장한 뒤, 전자잉크 화면 클리어를 흉내 내는 시퀀스를 실행합니다.

```text
1.5초 대기
0.2초 검정 전체 화면
0.2초 흰색 전체 화면
잠자기/초기 이미지 표시
```

복귀할 때도 같은 클리어 시퀀스가 실행된 뒤 글쓰기 화면으로 돌아옵니다. 전원 스위치는 켜짐/꺼짐에 따라 다른 짧은 소리를 냅니다.

## 문서 저장 방식

기본 저장은 브라우저의 `localStorage`에 이루어집니다. 문서 목록, 현재 문서명, 글 내용, 설정값 등이 저장됩니다.

로컬 SD 루트 폴더 권한 안내는 파일을 연 직후가 아니라, 에뮬레이터 전원을 처음 켤 때 나타납니다. 이때 `Select Folder`를 눌러 폴더 권한을 허가하면 브라우저의 File System Access API를 이용해 해당 폴더를 실제 SD 루트처럼 사용하려고 시도합니다. 브라우저가 이 API를 지원하지 않거나 권한을 허가하지 않으면 브라우저 내부 저장만 사용합니다.

문서 이름은 기본적으로 `doc0001.txt`, `doc0002.txt` 형식입니다. 새 문서는 가장 큰 번호 다음 번호로 생성됩니다.

## 웹 관리 화면 사용법

웹 관리 화면은 사이드 패널의 `Web` 버튼, 메뉴의 네트워크 흐름, 또는 Sync 화면의 키 입력으로 열 수 있습니다. 실제 장치의 웹 인터페이스를 브라우저 팝업/모달로 흉내 냅니다.

웹 화면을 열면 먼저 4자리 PIN을 요구합니다. 이 PIN은 장치 화면에 표시되는 `state.webPin` 값입니다. 다른 값을 입력하면 열리지 않습니다.

웹 화면 탭:

- `Documents`
- `Settings & Update`

### Documents 탭

문서 탭에서는 다음 작업을 할 수 있습니다.

| 기능 | 설명 |
| --- | --- |
| Upload Text | 입력한 텍스트를 다음 `docNNNN.txt` 문서로 추가 |
| Read | 문서를 웹 화면 안에서 읽기 전용으로 열기 |
| Back | 읽기 화면에서 문서 목록으로 돌아가기 |
| Download | 문서를 `.txt` 파일로 다운로드 |
| Delete | 6자리 확인 코드를 입력한 뒤 문서 삭제 |

웹 삭제도 메뉴 삭제와 마찬가지로 6자리 숫자 확인을 요구합니다. 틀린 코드를 입력하면 삭제하지 않고 다시 입력하게 합니다.

### Settings & Update 탭

환경설정 카드에서는 다음 설정을 저장할 수 있습니다.

| 설정 | 설명 |
| --- | --- |
| Text Size | 화면 글자 크기 |
| Line Space | 줄 간격 |
| Character Space | 글자 간격 |
| English Keyboard | Qwerty/Dvorak 선택 |
| Language | 입력 언어 선택 |

현재 코드 기준으로 `Sleep Timer`, `Speed`, `Refresh` 입력은 화면에 있지만 실제 동작 전체에 연결되어 있지는 않습니다. 글자 입력 큐 지연은 코드 기본값으로 관리됩니다.

Firmware Update, Font/Image Upload, Online Firmware Update 영역은 실제 펌웨어 업데이트를 수행하지 않습니다. 현재는 시뮬레이션/자리 표시 성격입니다.

### GitHub Sync

GitHub Sync 영역은 GitHub REST API를 사용해 에뮬레이터의 `.txt` 문서와 GitHub 저장소의 `documents/` 폴더를 맞추는 기능입니다. 실제 펌웨어의 네트워크 동기화를 브라우저에서 시험하기 위한 기능이므로, GitHub 계정, 저장소, 브랜치, 토큰이 필요합니다.

GitHub 화면 이름은 시간이 지나면 조금 바뀔 수 있습니다. 이 문서는 2026년 6월 기준 GitHub 웹 UI 흐름을 기준으로 적었습니다. 공식 문서는 [Creating a new repository](https://docs.github.com/en/repositories/creating-and-managing-repositories/creating-a-new-repository)와 [Managing your personal access tokens](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens)를 확인하면 됩니다.

#### 전체 흐름 요약

처음 GitHub 연동을 준비하는 흐름은 다음 순서입니다.

1. GitHub에서 비공개 저장소를 만듭니다.
2. 저장소 안에 `documents/` 폴더를 만듭니다.
3. GitHub personal access token을 발급합니다.
4. 발급 직후 토큰 문자열을 복사합니다.
5. 에뮬레이터 웹 화면을 열고 PIN을 입력합니다.
6. GitHub Sync 설정에 Owner, Repository, Branch, Token을 붙여넣습니다.
7. `Save GitHub Settings`를 누릅니다.
8. `Sync Now`를 눌러 동기화합니다.

#### 1. 비공개 GitHub 저장소 만들기

1. 브라우저에서 `https://github.com`에 로그인합니다.
2. 오른쪽 위의 `+` 버튼을 누릅니다.
3. `New repository`를 선택합니다.
4. `Owner`에서 저장소를 만들 계정 또는 조직을 선택합니다.
5. `Repository name`에 저장소 이름을 입력합니다.
   - 예: `ize-compose-documents`
   - 에뮬레이터 웹 설정의 `Repository` 칸에는 이 이름을 그대로 넣습니다.
6. 공개 여부에서 `Private`를 선택합니다.
7. 필요하면 `Add a README file`을 켭니다. 켜도 되고 꺼도 됩니다.
8. `.gitignore`, License는 문서 저장용 저장소라면 꼭 필요하지 않습니다.
9. `Create repository`를 누릅니다.

저장소를 만든 뒤 에뮬레이터에 입력할 값은 다음처럼 정리해 둡니다.

| GitHub 화면 | 에뮬레이터 입력칸 |
| --- | --- |
| 저장소 소유자 이름 | `Owner` |
| 저장소 이름 | `Repository` |
| 기본 브랜치 이름 | `Branch` |

예를 들어 저장소 주소가 `https://github.com/ize-studio/ize-compose-documents`라면 다음처럼 입력합니다.

```text
Owner: ize-studio
Repository: ize-compose-documents
Branch: main
```

#### 2. `documents/` 폴더 만들기

에뮬레이터는 현재 GitHub 저장소의 `documents/` 폴더를 문서 위치로 사용합니다. GitHub는 빈 폴더만 따로 저장하지 않으므로, 폴더 안에 파일 하나를 만들어야 합니다.

방법 A: `.gitkeep`으로 폴더만 준비하기

1. 새로 만든 저장소 페이지로 들어갑니다.
2. `Add file`을 누릅니다.
3. `Create new file`을 선택합니다.
4. 파일 이름 칸에 `documents/.gitkeep`을 입력합니다.
5. 내용은 비워 둬도 됩니다.
6. 아래쪽 `Commit changes`를 누릅니다.

방법 B: 첫 문서를 직접 만들기

1. 저장소 페이지에서 `Add file`을 누릅니다.
2. `Create new file`을 선택합니다.
3. 파일 이름 칸에 `documents/doc0001.txt`를 입력합니다.
4. 본문에 테스트 문장을 조금 입력합니다.
5. `Commit changes`를 누릅니다.

에뮬레이터 동기화 대상은 `.txt` 파일입니다. `.gitkeep`은 폴더를 만들기 위한 파일일 뿐이고, 문서 동기화 대상으로 쓰지 않습니다.

#### 3. Fine-grained personal access token 만들기

권장 방식은 fine-grained personal access token입니다. 특정 저장소 하나와 필요한 권한만 줄 수 있기 때문입니다.

1. GitHub 오른쪽 위 프로필 사진을 누릅니다.
2. `Settings`를 누릅니다.
3. 왼쪽 메뉴 맨 아래쪽의 `Developer settings`를 누릅니다.
4. `Personal access tokens`를 엽니다.
5. `Fine-grained tokens`를 선택합니다.
6. `Generate new token`을 누릅니다.
7. 토큰 이름을 입력합니다.
   - 예: `Ize Compose Emulator Sync`
8. `Expiration`을 선택합니다.
   - 처음 테스트라면 짧게 잡는 편이 안전합니다.
9. `Resource owner`에서 저장소 소유자 계정 또는 조직을 선택합니다.
10. `Repository access`에서 `Only select repositories`를 선택합니다.
11. 방금 만든 비공개 저장소를 선택합니다.
12. `Repository permissions`에서 `Contents`를 찾습니다.
13. `Contents` 권한을 `Read and write`로 설정합니다.
14. `Metadata`는 보통 자동으로 `Read-only`가 들어갑니다.
15. 다른 권한은 특별히 필요하지 않습니다.
16. 아래쪽 `Generate token`을 누릅니다.
17. GitHub가 토큰을 보여주면 즉시 복사합니다.

여기서 가장 중요한 부분은 저장소 권한 허가입니다. `Repository access`에서 방금 만든 비공개 저장소를 반드시 선택해야 합니다. 저장소를 선택하지 않거나 `Contents` 권한을 `Read and write`로 주지 않으면 에뮬레이터가 파일 목록을 읽거나 문서를 업로드할 수 없습니다.

조직 저장소를 쓰는 경우 조직 정책에 따라 fine-grained token 승인이 추가로 필요할 수 있습니다. 승인이 필요한 조직이라면 GitHub 안내에 따라 토큰 접근 요청을 승인받은 뒤 사용합니다.

토큰은 이 화면을 벗어나면 다시 전체 문자열을 볼 수 없습니다. 복사하지 못했다면 토큰을 새로 만들어야 합니다.

#### 4. 토큰 복사하기

토큰 생성 완료 화면에서 다음 중 하나로 복사합니다.

1. 토큰 문자열 오른쪽의 복사 버튼을 누릅니다.
2. 또는 토큰 문자열을 드래그해서 선택합니다.
3. `Ctrl + C`를 누릅니다.
4. 메모장이나 채팅창 같은 곳에 붙여넣어 보관하지 않습니다. 토큰은 비밀번호처럼 다룹니다.

복사가 제대로 됐는지 확인하려면 에뮬레이터의 Token 칸에 붙여넣을 때 긴 문자열이 들어가는지만 보면 됩니다. 토큰은 보안상 화면에 그대로 보이지 않을 수 있습니다.

#### 5. 에뮬레이터에 GitHub 정보 붙여넣기

1. `codes/index.html`을 브라우저에서 엽니다.
2. 에뮬레이터 화면에서 `Web` 버튼을 누릅니다.
3. 장치 화면에 표시된 4자리 PIN을 확인합니다.
4. 웹 화면의 PIN 입력칸에 4자리 PIN을 입력합니다.
5. `Open`을 누릅니다.
6. `Settings & Update` 탭을 누릅니다.
7. `GitHub Sync` 카드로 이동합니다.
8. `Owner`에 저장소 소유자 이름을 입력합니다.
9. `Repository`에 저장소 이름을 입력합니다.
10. `Branch`에 브랜치 이름을 입력합니다.
    - 보통 `main`입니다.
11. `Token` 칸을 클릭합니다.
12. `Ctrl + V`로 방금 복사한 토큰을 붙여넣습니다.
13. 이 브라우저에 토큰을 저장하려면 `Save token on this computer only`를 체크합니다.
14. 공용 컴퓨터라면 체크하지 않습니다.
15. `Save GitHub Settings`를 누릅니다.
16. 상태 메시지가 저장됐다고 바뀌는지 확인합니다.
17. `Sync Now`를 누릅니다.

동기화가 시작되면 아래 로그 영역에 업로드, 다운로드, 삭제 계획과 결과가 표시됩니다.

#### 준비물

| 항목 | 설명 |
| --- | --- |
| GitHub Owner | 사용자명 또는 조직명 |
| Repository | 문서를 저장할 저장소 이름 |
| Branch | 동기화할 브랜치. 기본값은 `main` |
| Document path | 현재 UI에서는 `documents`로 고정 |
| Token | 저장소 contents를 읽고 쓸 수 있는 GitHub 토큰 |

토큰은 GitHub API에서 파일 목록 읽기, 파일 내용 읽기, 파일 생성/수정, 파일 삭제를 할 수 있어야 합니다. Fine-grained token을 쓰는 경우 대상 저장소에 대해 Contents 읽기/쓰기 권한이 필요합니다. Classic token을 쓰는 경우 private 저장소라면 저장소 접근 권한이 포함되어야 합니다.

토큰은 비밀번호처럼 취급해야 합니다. 공용 컴퓨터에서는 `Save token on this computer only`를 켜지 않는 것이 안전합니다.

#### 저장소 준비

GitHub 저장소에는 문서가 들어갈 `documents/` 폴더가 필요합니다. 폴더가 비어 있으면 Git은 빈 폴더만 따로 저장하지 않으므로, 필요하면 `.gitkeep` 같은 파일을 먼저 넣어둘 수 있습니다. 에뮬레이터는 `.txt` 문서만 동기화 대상으로 봅니다.

권장 구조:

```text
repository-root/
  documents/
    doc0001.txt
    doc0002.txt
```

원격에 문서가 하나도 없고 로컬 에뮬레이터에 문서가 있으면, 첫 동기화 때 로컬 문서를 업로드하려고 합니다.

#### 웹 화면에서 연결하는 순서

1. 에뮬레이터에서 `Web` 버튼을 누르거나, 메뉴의 `Network`를 통해 웹 화면을 엽니다.
2. 장치 화면에 표시된 4자리 PIN을 웹 화면의 PIN 입력칸에 입력합니다.
3. `Open`을 눌러 웹 관리 화면을 엽니다.
4. `Settings & Update` 탭으로 이동합니다.
5. `GitHub Sync` 카드에서 `Owner`, `Repository`, `Branch`, `Token`을 입력합니다.
6. 토큰을 브라우저에 저장하려면 `Save token on this computer only`를 체크합니다.
7. `Save GitHub Settings`를 누릅니다.
8. `Sync Now`를 누릅니다.

`Save GitHub Settings`는 설정을 저장하는 단계이고, 실제 문서 동기화는 `Sync Now`에서 실행됩니다.

#### 토큰 저장 방식

| 방식 | 설명 |
| --- | --- |
| 저장 토큰 | `Save token on this computer only`를 체크하면 브라우저 저장소에 토큰을 저장합니다. 다음 실행에서도 남을 수 있습니다. |
| 세션 토큰 | 체크하지 않으면 현재 브라우저 세션에서만 사용합니다. 새로고침이나 브라우저 상태에 따라 다시 입력해야 할 수 있습니다. |
| Delete token | 저장 토큰과 세션 토큰을 모두 지웁니다. |

토큰 입력칸을 비워 둔 채 저장하면 기존 저장 토큰을 유지하려고 합니다. 저장 토큰을 세션 토큰으로 바꾸거나 삭제하는 동작은 `Save token on this computer only` 체크 상태와 `Delete token` 버튼에 따라 달라집니다.

#### 동기화 규칙

동기화는 로컬 문서 목록과 GitHub 원격 문서 목록을 비교해 계획을 만듭니다.

| 상황 | 동작 |
| --- | --- |
| 로컬에만 있는 `.txt` 문서 | GitHub `documents/` 폴더로 업로드 |
| 원격에만 있는 `.txt` 문서 | 동기화 상태에 따라 원격 삭제 대상으로 계획될 수 있음 |
| 로컬과 원격 둘 다 있고 내용이 같음 | 아무 작업 안 함 |
| 로컬과 원격 둘 다 있고 내용이 다름 | 마지막 동기화 상태를 참고해 업로드 또는 다운로드 |

에뮬레이터는 내부적으로 각 문서의 blob SHA를 비교합니다. 마지막 동기화 상태는 브라우저 저장 상태 또는 선택한 로컬 SD 루트의 `ize_compose/github_sync_state.txt`에 저장될 수 있습니다.

현재 구현은 충돌 해결 UI를 따로 제공하지 않습니다. 로컬과 원격이 동시에 바뀐 경우, 코드의 계획 규칙에 따라 한쪽으로 덮어쓸 수 있습니다. 중요한 문서는 동기화 전에 별도로 백업하는 것이 안전합니다.

#### Sync 화면에서 쓰는 키

장치 화면의 `Sync` 메뉴에 들어가면 다음 키를 사용할 수 있습니다.

| 키 | 동작 |
| --- | --- |
| `Enter` | GitHub 동기화 실행 |
| `G` | 웹 설정 화면 열기 |
| `W` | Wi-Fi 웹 화면 열기 |
| `Esc` | Sync 화면 닫기 |

Sync 화면에는 네트워크 상태, 저장소 정보, 브랜치, 경로, 업로드/다운로드/삭제 로그가 표시됩니다.

#### 자주 나오는 실패 원인

| 메시지/상황 | 의미와 확인할 것 |
| --- | --- |
| `Browser network offline` | 브라우저가 오프라인으로 판단했습니다. 컴퓨터 인터넷 연결을 확인합니다. |
| `GitHub repository info missing` | Owner, Repository, Branch, Token 중 필요한 값이 비어 있습니다. |
| `Token is required.` | 토큰이 없거나 저장되지 않았습니다. |
| `GitHub sync failed` | GitHub API 호출이 실패했습니다. 토큰 권한, 저장소 이름, 브랜치 이름, 네트워크 상태를 확인합니다. |
| 404 계열 실패 | 저장소 이름/Owner/브랜치가 틀렸거나 토큰이 해당 저장소를 볼 권한이 없습니다. |
| 401/403 계열 실패 | 토큰이 없거나 권한이 부족하거나 만료됐을 가능성이 큽니다. |

#### 보안 주의

- GitHub 토큰은 절대 문서 파일이나 공개 저장소에 적지 않습니다.
- 저장 토큰은 현재 브라우저에 남습니다.
- 공용 컴퓨터나 공유 브라우저에서는 세션 토큰으로만 쓰고, 작업 후 `Delete token`을 누르는 것이 안전합니다.
- private 저장소를 쓰는 경우 토큰 권한을 필요한 저장소 하나로 제한하는 것이 좋습니다.

## 네트워크 시뮬레이션

메뉴의 `Network` 항목에서 네트워크 모드를 선택할 수 있습니다.

| 모드 | 설명 |
| --- | --- |
| Off | 네트워크 꺼짐 |
| WiFi | 가상 Wi-Fi 목록 표시 후 연결 |
| WebServer | 10자리 숫자 비밀번호 입력 후 웹 서버 모드 |

Wi-Fi 모드에서는 가상의 SSID 목록이 표시됩니다. 비밀번호는 아무 값이나 입력하고 `Enter`를 누르면 연결된 것으로 처리됩니다.

WebServer 모드는 10자리 숫자를 요구합니다. 10자리를 입력하고 `Enter`를 누르면 웹 화면이 열립니다.

네트워크 상태 화면에서는 PIN과 연결 상태를 표시합니다. 실제 네트워크 장치를 제어하지는 않습니다.

## 사이드 패널 기능

사이드 패널은 실제 장치에는 없는 브라우저용 보조 UI입니다.

| 버튼/항목 | 설명 |
| --- | --- |
| Menu | 메뉴 열기/닫기 |
| Save | 현재 문서 저장 |
| Sleep | 잠자기 |
| New | 새 문서 생성 |
| Search | 검색 모드 |
| Reset | 브라우저 저장 상태를 기본값으로 초기화 |
| Web | 웹 관리 화면 열기 |
| Text size | 글자 크기 조절 |
| Line space | 줄 간격 조절 |
| Language | 입력 언어 선택 |
| Documents | 문서 목록 표시 |
| Import Text > Load | 입력 상자의 내용을 현재 문서로 교체 |
| Import Text > Append | 입력 상자의 내용을 입력 큐로 추가 |

## 실제 펌웨어 1.4.1과 다른 점

이 에뮬레이터는 실제 펌웨어와 최대한 비슷한 화면/흐름을 목표로 하지만, 다음 차이가 있습니다.

| 영역 | 에뮬레이터 | 실제 펌웨어 |
| --- | --- | --- |
| 화면 | HTML Canvas에 렌더링 | 실제 전자잉크 패널 |
| 저장소 | `localStorage`, IndexedDB, 선택적 로컬 폴더 | SD 카드/장치 파일 시스템 |
| 키보드 | 브라우저 키 이벤트와 클릭 가능한 HTML 키 | 실제 키보드 스캔/하드웨어 입력 |
| 소리 | Web Audio 합성음 | 실제 장치 하드웨어 구성에 따름 |
| Wi-Fi | 가상 SSID와 가상 연결 | 실제 Wi-Fi 스캔/접속 |
| WebServer | 브라우저 모달로 구현 | 실제 HTTP 서버 |
| PIN | 화면에 표시된 4자리 `state.webPin` 확인 | 펌웨어 구현에 따름 |
| 펌웨어 업데이트 | 현재 시뮬레이션 | 실제 바이너리 업데이트 |
| 배터리 | 고정/시뮬레이션 표시 | 실제 배터리 측정 |
| 전자잉크 잔상/클리어 | 시간과 색상 플래시로 모사 | 실제 패널 refresh 동작 |
| GitHub Sync | 브라우저 fetch로 GitHub API 호출 | 펌웨어 네트워크 스택 구현에 따름 |

확인된 차이점은 문서에 명시하는 것을 원칙으로 합니다. 펌웨어에 있는 모든 내부 타이밍, 전원 상태, 네트워크 실패 조건, 파일 권한 실패 조건을 1:1로 보장하지는 않습니다.

## 알려진 제한

- 실제 모바일 브라우저, 데스크톱 브라우저, 브라우저 엔진마다 오디오 시작 정책과 파일 접근 권한 UI가 다를 수 있습니다.
- File System Access API는 모든 브라우저에서 지원되지 않습니다.
- GitHub Sync는 실제 토큰과 네트워크 권한이 필요합니다.
- 웹의 Firmware Update, Font/Image Upload, Online Firmware Update는 실제 업데이트를 수행하지 않습니다.
- 문서 경로는 현재 웹 UI에서 `documents`로 고정되어 있습니다.
- 전자잉크 화면의 글꼴과 글자 폭은 Canvas 측정값을 사용하므로 실제 패널/펌웨어 글꼴과 완전히 같지 않을 수 있습니다.

## 개발자가 확인할 주요 위치

| 파일 | 내용 |
| --- | --- |
| `codes/index.html` | 화면, 상태, 메뉴, 웹 UI, 문서 관리, 네트워크 시뮬레이션 |
| `codes/key-engine.js` | 한글 조합 엔진 |
| `codes/keyboard-layouts.js` | 키보드 레이아웃 데이터 |
| `others/initial.png` | 초기/잠자기 화면 이미지 |

문서에 적힌 동작은 `codes/index.html` 기준으로 정리했습니다. 기능을 바꾸면 이 문서도 함께 갱신해야 합니다.
