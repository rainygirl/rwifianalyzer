<img src="icon.png" width="64" align="left" alt="">

# R WiFi Analyzer

*[English version](README.md)*

Wi-Fi 채널/신호 세기 그래프입니다. 주변의 모든 AP를 자기가 쓰는 채널 위에 곡선으로
그리고, 높이는 dBm 신호 세기입니다. 어느 채널이 붐비고 어느 채널이 비어 있는지 한눈에
보입니다.

같은 앱을 세 OS로 이식했고, 그래프는 하나입니다:

| OS | 소스 | 언어 | 설치 |
|----|------|------|------|
| Windows 10 Mobile | [`winphone/`](winphone) | C# + XAML (UWP) | [README](winphone/README.ko.md) - 서명된 패키지가 [`winphone/dist`](winphone/dist)에 있어 빌드 없이 설치 가능 |
| macOS 14 이상 | [`mac/`](mac) | Swift + SwiftUI | [README](mac/README.ko.md) - `cd mac && make` |
| RenkuOS | [`renkuos/`](renkuos) | C++ + Haiku API | [README](renkuos/README.ko.md) - `cd renkuos && make` |

각 포트는 독립적입니다. README, 빌드, 그리고 그 플랫폼의 함정을 정리한 `AGENTS.md`를
각자 가지고 있습니다. 빌드 시점에 공유하는 것은 없습니다.

![macOS 포트의 2.4GHz 화면. 네트워크 이름은 가상의 것입니다](mac/docs/screenshot.png)

## 세 포트가 공통으로 하는 일

- 2.4GHz와 5GHz, 밴드별로 그래프 하나
- 5초마다 자동 재스캔, 수동 갱신도 가능
- 같은 네트워크는 스캔이 바뀌어도 같은 색 유지 (BSSID의 FNV-1a 해시. 플랫폼 기본
  문자열 해시는 프로세스마다 시드가 달라서 못 씁니다)
- 곡선 폭은 그 채널의 실제 폭입니다. 20MHz 채널은 22MHz, 40MHz 채널은 42MHz로 그리고,
  실제로 점유하는 위치에 놓이므로 넓은 채널이 옆 채널을 덮는 게 눈에 보입니다
- 어디에도 접속하지 않고 듣기만 합니다

## 그래프 읽는 법

- **가로축** - 채널 번호. 겹치는 곡선끼리는 서로 간섭합니다.
- **세로축** - dBm 신호 세기. 0에 가까울수록 강합니다. -50은 아주 좋고, -70은 쓸 만하고,
  -85 아래는 거의 안 잡힙니다.
- 2.4GHz에서 서로 겹치지 않는 채널은 **1, 6, 11** 셋뿐입니다. 이 중 아래에 깔린 게 가장
  적은 채널을 고르십시오.

## 포트마다 다른 점

Wi-Fi 스캔이 무엇까지 알려주는지는 OS가 정합니다. 세 포트의 차이는 대부분 거기서
나옵니다.

| | Windows 10 Mobile | macOS | RenkuOS |
|---|---|---|---|
| 스캔 API | `Windows.Devices.WiFi` | CoreWLAN | net80211 ioctl ([이유](renkuos/AGENTS.md)) |
| 6GHz | 해당 무선을 가진 기기가 없음 | 지원 | 지원하는 드라이버가 없음 |
| 채널 폭 | 전혀 알려주지 않음 - 모든 곡선이 22MHz | CoreWLAN이 알려주고, 위치는 비콘의 HT/VHT Operation 요소로 결정 | 같은 비콘 요소에서 읽음 |
| 네트워크 이름을 보려면 | Wi-Fi 사용 동의 | 위치 접근 권한 | 아무것도 필요 없음 |
| 보안 방식 | WPA/WPA2 | WPA3까지 | 암호화 여부만 |

## 아이콘

세 포트가 같은 Wi-Fi 글리프, 같은 초록색을 씁니다. UWP 타일은 그려진 상태로 커밋되어
있고, `.icns`와 RenkuOS 비트맵 아이콘은 같은 도형에서
[`mac/tools/make-icon.py`](mac/tools/make-icon.py)와
[`renkuos/tools/make-icon.py`](renkuos/tools/make-icon.py)로 생성합니다. 루트의
`icon.png`도 같은 그림이고, README에 쓰입니다.

## 라이선스

MIT - [LICENSE](LICENSE) 참고.

## AI 기여 고지

이 프로그램은 Claude와 함께 작업해 만들었습니다.
