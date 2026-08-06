# R WiFi Analyzer

*[한국어 버전](README.ko.md)*

Wi-Fi channel and signal strength graph. Every access point in range is drawn as a
curve over the channel it occupies, with its height set by signal strength in dBm -
so you can see at a glance which channels are crowded and which are free.

Same app, three ports, one graph:

| OS | Source | Written in | Install |
|----|--------|------------|---------|
| Windows 10 Mobile | [`winphone/`](winphone) | C# + XAML (UWP) | [README](winphone/README.md) - a signed package is in [`winphone/dist`](winphone/dist), no build needed |
| macOS 14 and newer | [`mac/`](mac) | Swift + SwiftUI | [README](mac/README.md) - `cd mac && make` |
| Haiku | [`haiku/`](haiku) | C++ + the Haiku API | [README](haiku/README.md) - `cd haiku && make` |

Each port stands alone: its own README, its own build, and its own `AGENTS.md`
covering the traps of that platform. Nothing is shared at build time.

![The macOS port showing the 2.4 GHz band; the network names are made up](mac/docs/screenshot.png)

## What every port does

- 2.4 GHz and 5 GHz, one graph per band
- Rescans every 5 seconds, or on demand
- Each network keeps the same colour between scans (FNV-1a of the BSSID, because
  the platform string hash is seeded per process)
- Each curve is as wide as its channel really is - 22 MHz for a 20 MHz channel,
  42 for a 40 MHz one - and sits over the spectrum it actually occupies, so a wide
  network visibly covers its neighbours
- Nothing ever connects to anything; the app only listens

## How to read the graph

- **Horizontal axis** - channel number. Overlapping curves are networks that
  interfere with each other.
- **Vertical axis** - signal strength in dBm. Closer to 0 is stronger: -50 is
  excellent, -70 is usable, below -85 is barely there.
- On 2.4 GHz, channels **1, 6 and 11** are the only three that do not overlap.
  Pick whichever of them has the least under it.

## Where the ports differ

The OS decides what a Wi-Fi scan is allowed to tell you, and that is most of the
difference between them.

| | Windows 10 Mobile | macOS | Haiku |
|---|---|---|---|
| Scan API | `Windows.Devices.WiFi` | CoreWLAN | net80211 ioctl ([why](haiku/AGENTS.md)) |
| 6 GHz | no radio exists on the hardware | yes | no driver supports it |
| Channel width | not reported at all - every curve is 22 MHz | from CoreWLAN, placed using the beacon's HT/VHT Operation element | from the same beacon elements |
| Network names need | the Wi-Fi consent prompt | Location access | nothing |
| Security type | WPA/WPA2 | up to WPA3 | encrypted or open |

## The icon

All three ports use the same Wi-Fi glyph, in the same green. The UWP tiles are
committed as drawn; the `.icns` and the Haiku bitmap icons are generated from the
same geometry by [`mac/tools/make-icon.py`](mac/tools/make-icon.py) and
[`haiku/tools/make-icon.py`](haiku/tools/make-icon.py).

## License

MIT - see [LICENSE](LICENSE).

## AI Disclaimer

This application was produced by a human working with Claude.
