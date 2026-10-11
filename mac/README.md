<img src="../icon.png" width="64" align="left" alt="">

# R WiFi Analyzer - macOS

*[한국어 버전](README.ko.md)*

*The macOS port. See the [repository README](../README.md) for the
[Windows Phone](../winphone) and [RenkuOS](../renkuos) ports.*

Wi-Fi channel and signal strength graph for macOS 14 (Sonoma) and newer. Every
access point in range is drawn as a curve over the channel it occupies, with its
height set by signal strength in dBm.

- 2.4 GHz, 5 GHz and 6 GHz, switched from the title bar
- Rescans every 5 seconds, or on demand
- Each network keeps the same colour between scans
- Wide channels are drawn at their real width, over the spectrum they occupy

![The 2.4 GHz graph, with made-up network names](docs/screenshot.png)

## Building it

You need the Xcode command line tools (`xcode-select --install`). There is no
Xcode project: SwiftPM builds the binary and the Makefile assembles the bundle
around it.

```
cd mac
make
open "./R WiFi Analyzer.app"
```

`make install` copies it to `/Applications` as `R WiFi Analyzer.app`. `make ARCHS="arm64 x86_64"` builds a
universal binary. `make run` does both steps at once.

## Why it asks for Location access

Because on macOS that is the switch for network names, and nothing else.

macOS treats a Wi-Fi network's identity as location data - a BSSID can be looked up
on a map - so CoreWLAN withholds `ssid`, `bssid` and `countryCode` from any app
that is not authorized for Location Services. Everything else a scan returns,
including channel and signal strength, is never withheld. So the graph is correct
either way; without the authorization every curve is simply labelled `(hidden)`.

**The app never reads a location.** It calls
`CLLocationManager.requestWhenInUseAuthorization()` on launch and nothing else -
there is no `startUpdatingLocation`, no delegate method that receives coordinates,
and nothing is stored or sent anywhere. Deny it and the app keeps working, minus
the names.

If you dismissed the prompt and want the names, turn it on under **System Settings
→ Privacy & Security → Location Services → R WiFi Analyzer**.

`make` signs the bundle ad-hoc, which means its identity changes on every rebuild
and macOS treats each build as a new app - so it asks again, and old entries pile
up in the Location Services list. Sign with a real identity to make the grant
stick:

```
make SIGN="Developer ID Application: Your Name (TEAMID)"
```

## What is not supported

- **6 GHz channel width.** A 40 or 80 MHz network is drawn at its real width,
  because the beacon says where its centre is. On 6 GHz that information lives in an
  element this app does not read yet, so a 6 GHz network is drawn 22 MHz wide over
  its primary channel - understated, never misplaced.
- **Hidden networks** show as `(hidden)`, and so does every network when Location
  access has been denied - the app cannot tell the two apart, because in both cases
  it is handed no name.
- **No history.** The graph is whatever the last scan saw.

## If the graph is empty

- **"No Wi-Fi interface found"** - Wi-Fi is off, or this Mac has no Wi-Fi.
- **Every label says `(hidden)`** - Location access, see above. The graph is still
  accurate.
- **"Scan refused"** - normal and harmless: the driver rate-limits scans, and the
  app falls back to the last cache. It clears itself on the next tick.

Scanning briefly takes the radio off its channel, so a scan every 5 seconds is a
few percent of your throughput. Close the window when you are done measuring.

If you are changing the code rather than just building it, read
[AGENTS.md](AGENTS.md) first.

## License

MIT - see [LICENSE](../LICENSE).

## AI Disclaimer

This application was produced by a human working with Claude.
