# Notes for agents and contributors - Windows Phone port

Engineering notes for changing this code. The README is install-only; everything
about *why the code looks like this* lives here. The decisions this port shares
with the [macOS](../mac) and [Haiku](../haiku) ports - the graph geometry, the
colour hash, what all three do with a refused scan - are in the
[repository AGENTS.md](../AGENTS.md).

Written in English to match the source comments.

## What this is

A Windows 10 Mobile (UWP) Wi-Fi analyzer in C#/XAML: it scans for access points
and draws them as bell curves over a channel axis, one graph per band. Same idea
as the Android Wi-Fi analyzers, sized for a phone screen.

The build/deploy environment is the one worked out in the sibling
`rworldradio-winphone` repository, and its AGENTS.md is still the reference for
the toolchain traps. The ones that apply here are repeated below so this repo
stands alone; the ones about background audio do not apply - this app has no
background component and no second project.

## Module map

```
App.xaml(.cs)                 entry point + startup crash logging
Views/MainPage.xaml(.cs)      the whole UI: header, band switch, canvas, status
Views/SignalGraph.cs          all the drawing - axes, grid, curves, labels
Services/WifiScanner.cs       Windows.Devices.WiFi -> List<AccessPoint>
Services/ChannelMap.cs        frequency <-> channel number arithmetic
Models/AccessPoint.cs         one BSS from a scan
Properties/Default.rd.xml     Runtime Directives for the .NET Native compiler
Package.appxmanifest          identity, phone identity, wiFiControl capability
```

Single project, no Windows Runtime Component. Nothing here is activated by the OS
across a process boundary, so none of the `.winmd` / `ActivatableClass` machinery
that the radio app needs applies.

## The Wi-Fi API

`Windows.Devices.WiFi` is the only way a UWP app can see other people's networks.
`Windows.Networking.Connectivity` shows you the connection you are *on*, not the
ones around you, so it is not usable for this.

Three things are load-bearing:

1. **`<DeviceCapability Name="wiFiControl" />` in the manifest.** Without it the
   app builds, installs, launches and simply finds no adapter -
   `FindAllAdaptersAsync` returns an empty list. There is no error to catch. If
   the app says "No Wi-Fi adapter found" on a phone whose Wi-Fi obviously works,
   check the manifest first.
2. **`RequestAccessAsync` must run on the UI thread.** It raises a consent prompt
   on first launch. Called from a background thread it throws. This is why
   `MainPage.OnLoaded` awaits it directly rather than pushing it onto a worker.
3. **`ScanAsync` failing is not fatal.** Phone radios rate-limit scans, so a scan
   requested 5 s after the last one is sometimes refused. `WifiScanner.ScanAsync`
   catches that and reads `NetworkReport` regardless - the OS keeps scanning on
   its own schedule, so the report is at worst a few seconds stale. Treating a
   refused scan as an error would blank the graph every other refresh.

Every API used is from the original UWP release (10240): `WiFiAdapter`,
`WiFiAvailableNetwork.ChannelCenterFrequencyInKilohertz`,
`NetworkRssiInDecibelMilliwatts`, `WiFiNetworkReport`. That is deliberate - see
the availability trap below.

`WiFiAvailableNetwork` does **not** report channel width, and this is the one place
where this port is behind the other two. Every curve is drawn 22 MHz wide, which is
right for 20 MHz channels and understates 40/80 MHz ones.

There is no way around it here. `PhyKind` says `Ht` or `Vht`, which tells you the
network *could* be wide but not whether it is; nothing on the class exposes the
beacon's information elements, which is where the macOS and Haiku ports read the
HT/VHT Operation element to get both the width and the centre of a wide channel. No
10586-era API - no later one either - hands a UWP app raw beacon data.

`AccessPoint` carries `WidthMhz` and `CentreMhz` anyway, filled with 20 and the
primary frequency, and `SignalGraph` computes the curve from them exactly as the
other ports do. The arithmetic is then identical across all three, and the day
something does report a width there is one assignment to change rather than a
drawing routine to rewrite.

## The README screenshot

`docs/screenshot.png` is a real screenshot off a Lumia 920 - real scan, real phone
chrome - with the network-name labels mosaicked over in 16 px blocks. The
`(hidden)` labels are left legible: they say something about the band, not about a
person. 8 px blocks were tried first and the bold glyph shapes were still readable
at that size, so if you redo it, check the result zoomed in.

The other two ports anonymise at the source instead - a temporary patch that
replaces the SSID before it is drawn - which is cleaner, but nothing on the phone
can be patched at screenshot time without rebuilding and reinstalling the package.

## The build/deploy traps

Carried over from the radio app, where each of these cost real time.

### 1. `Package.appxmanifest`'s `MinVersion` is not the source of truth

MSBuild **overwrites** it from `<TargetPlatformMinVersion>` in the .csproj.
Editing the manifest alone produces a package that still declares the old
minimum. Change the .csproj, and re-run `/t:Restore` afterwards - NuGet needs to
resolve the new `UAP,Version=vX` framework or the build fails with
`Your project does not reference "UAP,Version=v10.0.10586" framework`.

### 2. A phone silently refuses a package whose minimum is too high

Tapping install does *nothing*: no error, no dialog, no log entry. That is what a
14393-minimum package does on a 10586 phone. If install appears to do nothing,
check the minimum first - it is almost never anything else.

### 3. The compiler does not enforce API availability

Reference assemblies come from the 19041 SDK, so anything in that SDK compiles
regardless of `TargetPlatformMinVersion`. A 1607-era API compiles clean here and
then throws on the device, where there is no debugger to tell you which line.

**When adding any WinRT call, check its "Introduced in" version by hand.** The
build will not.

### 4. `Properties/Default.rd.xml` must be a `Content` item

`Microsoft.NetNative.targets` finds rd.xml by filtering `AppxPackagePayload` for
the `.rd.xml` extension. `None` and `RdXmlFile` items are both ignored and you get
warning ILT0027 - after which the .NET Native optimizer may strip a type the XAML
parser looks up by name, and the failure appears at runtime on the device.

### 5. Do not pass `--log` to the VS2017 bootstrapper

It auto-updates itself to the modern installer engine, which rejects that option
and exits with code **87** having done nothing. Logs land in `%TEMP%\dd_*` anyway.

### 6. `Windows10SDK.15063` is no longer installable

Only `Windows10SDK.19041` remains in the VS installer catalog. Compiling against
19041 is fine - trap 3 is the catch.

### 7. Signing: use a thumbprint, not a .pfx path

`/p:PackageCertificateKeyFile=x.pfx` fails with `APPX0105: Cannot import the key
file` even for a password the build knows. Sign from the certificate store
instead, with a certificate whose subject equals the manifest's `Publisher`
exactly (`CN=WifiAnalyzer`).

### 8. Every deployment tool is broken; MTP copy is what works

`WinAppDeployCmd`, Device Portal over USB and VS2017 F5 all need the `IpOverUsb`
service, whose payload Microsoft no longer ships. Copy the `.cer` and `.appx` to
`Phone\Downloads` over MTP and install from the phone, **`.cer` first** - the app
install fails silently otherwise.

`tools/copy-to-phone.ps1` does the copy and the confirmation. It has to run on the
Windows machine, in an interactive desktop session.

MTP scripting caveats, all four of which bit here:

- The shell's MTP namespace is only visible from an interactive desktop session,
  so an SSH session finds no device and reports no error. Run the copy as a
  scheduled task with `-LogonType Interactive`.
- **Register that task by SID, not by name.** `New-ScheduledTaskPrincipal -UserId
  "$env:USERDOMAIN\$env:USERNAME"` fails with *"No mapping between account names
  and security IDs was done"* from a remote session: on a machine that is not
  domain-joined `USERDOMAIN` is `WORKGROUP`, not the computer name.
  `([System.Security.Principal.WindowsIdentity]::GetCurrent()).User.Value` is what
  works.
- `FolderItem.InvokeVerb('delete')` **hangs** waiting for a confirmation dialog on
  the device. Copy under a new filename instead of deleting the old one.
- **MTP reports file names without their extension.** `Folder.CopyHere` is
  asynchronous and returns nothing, so the copy has to be confirmed by polling the
  destination - and `$_.Name -eq "WifiAnalyzer_1.1.0.0_ARM.appx"` never matches,
  because the item is called `WifiAnalyzer_1.1.0.0_ARM`. A poll written that way
  reports a timeout on a copy that in fact succeeded. Match on the base name, or
  check `System.Size`.

## Drawing decisions

- **Canvas of Shapes, not Win2D.** Win2D's UWP package requires a newer minimum
  than 10586. A phone sees a few dozen access points, so ~10 XAML elements per
  network is well within budget, and the whole canvas is rebuilt on each scan
  rather than diffed.
- **The 2.4 GHz axis is fixed at 2400-2500 MHz.** It is the same everywhere and
  narrow enough that channels 1-14 all fit with room to label them.
- **The 5 GHz axis is fitted to what is present.** The 5 GHz plan spans 5170-5885
  MHz; at 320 logical pixels that is under half a pixel per MHz and the curves
  become invisible slivers. `SignalGraph.DomainFor` therefore takes the min and
  max frequency actually detected, pads by one channel width, and widens to a
  140 MHz minimum. The axis moves when the set of channels in range changes, which
  is rare in practice.
- **Channel labels are thinned, gridlines are not.** A label is skipped when it
  would collide with the previous one; the gridline still marks the channel. This
  is what keeps a crowded 5 GHz axis readable.
- **Curves are painted strongest-first.** Painter's order, so a weak narrow curve
  lands on top of a strong wide one instead of disappearing under it.
- **Colours come from an FNV-1a hash of the BSSID**, not `string.GetHashCode`.
  The .NET string hash is randomised per process, so the same network would change
  colour on every launch.

## Deliberate omissions

- **No connect / disconnect.** `WiFiAdapter.ConnectAsync` exists, but this is a
  measurement tool; joining networks is what Settings is for.
- **No history or logging to disk.** Only `crash.log`.
- **No channel-width detection**, for the API reason above.
- **No 6 GHz.** No Windows 10 Mobile device has a Wi-Fi 6E radio.

## Startup failures are invisible - log them

A sideloaded phone app has no console and no debugger. If it throws during startup
the splash screen simply disappears. `App.xaml.cs` writes an `UnhandledException`
handler and a `NavigationFailed` handler to `LocalState\crash.log`, and
`WifiScanner` logs there too. Keep this. It is the only diagnostic channel on the
device.

## Build environment

Windows 11 (including ARM64), VS2017 Build Tools 15.9 with the UWP workload, SDK
10.0.19041. VS2019+ dropped Windows Mobile support. Budget ~17 GB of disk.

## Verification status

- `Release|ARM` (the .NET Native AOT toolchain) compiles with no errors and no
  warnings, on Windows 11 ARM64 build 26200 with VS2017 Build Tools and SDK 19041.
- The signed package is 420 KB. Its embedded `AppxManifest.xml` was read back out
  of the `.appx` and shows `ProcessorArchitecture=arm`, `MinVersion=10.0.10586.0`,
  `DeviceCapability wiFiControl` - the three things that decide whether the phone
  will install it and whether it will see anything once it does - plus
  `DisplayName=R WiFi Analyzer` and version `1.1.0.0`. `dist/` holds that package.
- `Get-AuthenticodeSignature` on it reports `UnknownError` with subject
  `CN=WifiAnalyzer`. That is the expected answer for a self-signed certificate the
  build machine does not trust, not a signing failure - trusting it is exactly what
  installing the `.cer` on the phone does.
- Copied to a Lumia 920's `Downloads` over MTP; both files arrived at the expected
  byte counts.
- The graph geometry was checked by replicating `SignalGraph`'s arithmetic against
  a synthetic access-point list at 360x640, which is where the layout constants
  (gutters, the 0.6 Bezier control offset, the label-collision gap) come from.
- **Not verified: anything that only happens on the device.** Whether the app
  launches, whether `RequestAccessAsync` prompts and returns `Allowed` on 10586,
  whether a 10586 driver honours a 5 s scan cadence, and how the graph actually
  looks on the phone are all still open.
