<img src="../icon.png" width="64" align="left" alt="">

# R WiFi Analyzer - Haiku OS

*[English version](README.md)*

*Haiku OS 포트입니다. [Windows Phone](../winphone)과 [macOS](../mac) 포트는
[저장소 README](../README.ko.md)를 보세요.*

Haiku OS용 Wi-Fi 채널/신호 세기 그래프입니다. 주변의 모든 AP를 자기가 쓰는 채널 위에 곡선으로
그리고, 높이는 dBm 신호 세기입니다.

- 2.4GHz와 5GHz. 그래프 위의 라디오 버튼에서 전환
- 5초마다 자동 재스캔, 수동 갱신도 가능
- 같은 네트워크는 스캔이 바뀌어도 같은 색 유지
- 40MHz 네트워크는 실제로 점유하는 위치에 42MHz 폭으로 그립니다

![Haiku에서의 2.4GHz 그래프. 실제 스캔이고 네트워크 이름만 바꿨습니다](docs/screenshot.png)

## 빌드하기

Haiku OS R1/beta4 이상. 개발 패키지는 기본 설치에 포함되어 있고, 그 외에 필요한 것은
없습니다.

```
cd haiku
make
make run
```

**32비트(`x86`) 설치에서는 `setarch x86 make`로 빌드하세요.** 거기서는 그냥 `gcc`가
BeOS 호환용으로 남아있는 2.95.3이라서 이 코드를 빌드할 수 없고, `setarch x86`이 gcc 13을
선택해줍니다. `x86_64`에서는 그냥 `make`가 맞습니다.

실행 파일은 `objects.<arch>-...-release/RWiFiAnalyzer`에 생기고 `make run`은 거기서
실행할 뿐입니다. 데이터 파일이 없어서 아무 데로나 복사해도 됩니다. 직접 빌드한 앱은 보통
`~/config/non-packaged/apps/`에 둡니다. **`R WiFi Analyzer`라는 이름으로 복사하세요.**
Haiku에서는 파일 이름이 Tracker/Deskbar/바탕화면에 보이는 이름이고, 빌드 산출물 이름에는
공백을 넣을 수 없습니다(makefile-engine이 `$(NAME)`을 셸 명령에 그대로 넣습니다).
`~/config/settings/deskbar/menu/Applications/`에 심볼릭 링크를 만들면 Deskbar 메뉴에,
`~/Desktop`에 만들면 바탕화면에 나타납니다:

```
APPS=~/config/non-packaged/apps
cp objects.x86-cc13-release/RWiFiAnalyzer "$APPS/R WiFi Analyzer"
mimeset -f "$APPS/R WiFi Analyzer"
ln -s "$APPS/R WiFi Analyzer" ~/config/settings/deskbar/menu/Applications/
ln -s "$APPS/R WiFi Analyzer" ~/Desktop/
```

## 뭔가 보이려면 필요한 것

Haiku에 드라이버가 있는 무선 장치. **환경설정 → Network**에 Wi-Fi 장치가 보이는지, 또는
`ifconfig`로 확인하세요. 앱은 자기가 고른 인터페이스 이름을 상태 줄에 표시합니다.
*"No wireless interface found"* 라고 나오면 읽을 대상이 없다는 뜻입니다.

권한 프롬프트도, 설정도 없습니다. Haiku는 어떤 앱이든 스캔 캐시를 읽게 해줍니다.

## 지원하지 않는 것

- **6GHz.** Haiku가 제공하는 드라이버 중 Wi-Fi 6E 무선을 지원하는 게 없어서 그릴 것이
  없습니다.
- **보안은 암호화 여부만** 나옵니다. 스캔 결과에는 capability 비트 하나만 들어 있어서,
  WPA2와 WPA3를 구분하려면 비콘의 information element를 파싱해야 합니다.
- **숨겨진 네트워크**는 `(hidden)`으로 표시됩니다. 비콘에 이름이 없습니다.
- 이 앱은 어디에도 접속하지 않고 듣기만 합니다.

## 그래프가 비어 있으면

- **"No wireless interface found"** - 지원되는 Wi-Fi 하드웨어가 없거나 드라이버가 붙지
  않았습니다. `listdev`와 `ifconfig`로 확인하세요.
- **"Scan result layout mismatch"** - 이 앱은 커널의 비공개 인터페이스로 스캔 결과를
  읽는데, 그 구조체 레이아웃을 미리 알고 있어야 합니다. 이 Haiku의 것과 어긋났다는
  뜻입니다. [AGENTS.md](AGENTS.md)를 보세요. `src/net80211.h` 몇 줄만 고치면 됩니다.
- **인터페이스 이름은 나오는데 그래프가 계속 비어 있음** - 각 주기는 직전 주기가 요청한
  스캔 결과를 읽으므로, 처음 채워지기까지 두 주기(10초)가 걸릴 수 있습니다. 그래도 계속
  비어 있으면 접속 중에는 드라이버가 스캔을 거부하는 경우입니다. 연결을 끊고 다시 해보면
  구분됩니다.

빌드만 하는 게 아니라 코드를 수정하실 거라면 [AGENTS.md](AGENTS.md)를 먼저 읽어보세요.

## 라이선스

MIT - [LICENSE](../LICENSE) 참고.

## AI 기여 고지

이 프로그램은 Claude와 함께 작업해 만들었습니다.
