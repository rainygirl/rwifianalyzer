# Notes for agents and contributors

One app, three ports, one folder each. This file covers what is true across all of
them; the per-port `AGENTS.md` covers the platform.

- [`winphone/AGENTS.md`](winphone/AGENTS.md) - UWP, VS2017, sideloading over MTP
- [`mac/AGENTS.md`](mac/AGENTS.md) - CoreWLAN, Location Services, bundling by Makefile
- [`renkuos/AGENTS.md`](renkuos/AGENTS.md) - the private net80211 ioctl, BView drawing

Written in English to match the source comments.

## The ports do not share code

Three languages, three toolkits, no build-time dependency between them. What is
shared is a set of decisions, repeated deliberately in each port so that a reader
of one file does not have to look at the other two:

- **The graph.** dBm axis from -20 to -100 with gridlines every 10. Two mirrored
  cubic segments per bell, control points at 0.6 of the half-width. Painted
  strongest-first, so a weak narrow curve lands on top of a strong wide one.
- **A curve is as wide as its channel, and sits where the channel sits.** Occupied
  width is the nominal width plus 2 MHz, so a 20 MHz channel is 22 MHz - which is
  why adjacent 2.4 GHz channels overlap. Wide channels are drawn on the centre of
  the *occupied* channel, not the primary, since a 40 MHz BSS puts its extension
  channel to one side. Where the OS will not say which side (or says nothing at
  all), the curve falls back to the primary 20 MHz rather than guessing: that much
  is certainly occupied. The width never goes in the label - the width of the curve
  is the point.
- **The 2.4 GHz axis is fixed at 2400-2500 MHz.** It is the same everywhere and
  narrow enough that channels 1-14 all fit with room to label them. The 5 GHz axis
  is fitted to the networks actually present and widened to a minimum span, because
  the 5 GHz plan is 700 MHz wide and would render every curve as a sliver.
- **Channel labels are thinned, gridlines are not.** A label is skipped when it
  would collide with the previous one; the gridline still marks the channel.
- **The same 12 colours**, indexed by an FNV-1a hash of the BSSID. Never the
  platform's own string hash - .NET, Swift and RenkuOS all seed theirs per process,
  which would repaint every network on each launch.
- **2.4 GHz is what the app opens on**, on all three.
- **The window title is the app's name; the UI does not repeat it.** The Windows
  Phone port is the exception, because a full-screen phone page has no title bar of
  its own.
- **A refused scan is not an error.** Every one of these radios rate-limits scans.
  All three ports fall back to the last results the OS kept rather than blanking the
  graph, and say so in the status line.

## Adding a port

The order that worked twice: `AccessPoint` and `ChannelMap` first (they are
arithmetic, and portable), then the scanner, then the graph, then the window. The
scanner is where the platform surprises live - budget most of the time there, and
write down what the OS refuses to tell you, because that is what shapes the rest.

## The icon

One Wi-Fi glyph, one green (`#4CAF50`), three formats. The UWP tiles in
`winphone/Assets` were drawn first and are committed as drawn; the geometry
constants in the two generators are measured off `Square150x150Logo.png`.

```
cd mac   && python3 tools/make-icon.py   # Resources/AppIcon.icns, needs iconutil
cd renkuos && python3 tools/make-icon.py   # icons/*.raw, B_CMAP8, runs anywhere
```

Both need Pillow. They duplicate the ~30 lines that draw the glyph rather than
share a module, so that each platform folder stands on its own - if you change the
glyph, change it in both, and the constants are named the same in each.
