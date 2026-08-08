<img src="../icon.png" width="64" align="left" alt="">

# R WiFi Analyzer - Windows 10 Mobile

*[한국어 버전](README.ko.md)*

*The Windows Phone port. See the [repository README](../README.md) for the
[macOS](../mac) and [Haiku](../haiku) ports.*

Wi-Fi channel and signal strength graph for Windows 10 Mobile (Windows Phone 10).
Every access point in range is drawn as a curve over the channel it occupies, with
its height set by signal strength in dBm - so you can see at a glance which
channels are crowded and which are free.

- 2.4 GHz and 5 GHz, switched from the header
- Rescans every 5 seconds, or on demand
- Each network keeps the same colour between scans

Targets build 10586 (Windows 10 Mobile 1511) and newer.

![The 2.4 GHz graph on a Lumia 920; the network names are mosaicked out](docs/screenshot.png)

## Installing on a phone

A prebuilt, signed package is in [`dist/`](dist), so you can skip the build.

You need a phone on **build 10586 or newer** with developer mode enabled:
**Settings → Update & security → For developers → Developer mode**.

1. Connect the phone to a Windows PC over USB. It appears as `Windows phone`.
2. Copy these into the phone's **Downloads** folder:
   - `dist/WifiAnalyzer_1.1.0.0_ARM.cer`
   - `dist/WifiAnalyzer_1.1.0.0_ARM.appx`
   - the three files in `dist/Dependencies/` (only needed if step 5 fails)
3. On the phone, open **File Explorer → Downloads** and tap
   **`WifiAnalyzer_1.1.0.0_ARM.cer`** first. Confirm the certificate install.
   File Explorer hides extensions, so this is the 784-byte one.
4. Tap **`WifiAnalyzer_1.1.0.0_ARM.appx`** (the 420 KB one). It installs and
   appears in the app list as **R WiFi Analyzer**.
5. If the install complains about a missing framework, tap the three
   `Dependencies` packages first, then repeat step 4. Most phones already have
   them.

Step 3 is not optional - the app install fails without the certificate.

Steps 1 and 2 are scripted: `powershell -ExecutionPolicy Bypass -File
tools\copy-to-phone.ps1` on the Windows machine copies both files and confirms they
arrived. Steps 3 and 4 have to be taps on the phone - see AGENTS.md for why.

On first launch the phone asks for permission to use Wi-Fi. Deny it and the app
shows an empty graph and says so in the status line.

### If tapping install does nothing

That is what a phone does when the package requires a newer OS than it has. Check
the phone's build number under **Settings → About → More info**.

### The tools that do not work

`WinAppDeployCmd`, Device Portal over USB, and Visual Studio's F5 deployment all
need the `IpOverUsb` service, which Microsoft no longer ships. Copying the files
to the phone is the way. (Device Portal over Wi-Fi should still work if you enable
it on the phone.)

## Building it yourself

You need a **Windows** machine. Windows 11 works, including ARM64.

**1. Install Visual Studio 2017 Build Tools with the UWP workload.** VS2019 and
later dropped Windows Mobile support, so 2017 is the last version that can build
this. Budget about 17 GB of disk.

```
curl -L -o vs_buildtools.exe https://aka.ms/vs/15/release/vs_buildtools.exe
.\vs_buildtools.exe --quiet --norestart --wait ^
    --add Microsoft.VisualStudio.Workload.UniversalBuildTools --includeRecommended
```

Do not add `--log`: the bootstrapper updates itself to a newer installer engine
that rejects that option and quits with code 87 having done nothing.

**2. Create a signing certificate.** Its subject must match the manifest's
`Publisher` exactly. In PowerShell:

```powershell
$cert = New-SelfSignedCertificate -Type Custom -Subject "CN=WifiAnalyzer" `
  -KeyUsage DigitalSignature -FriendlyName "WifiAnalyzer sideload" `
  -CertStoreLocation "Cert:\CurrentUser\My" `
  -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")
$cert.Thumbprint
```

**3. Build and package.** Sign by *thumbprint*, not by `.pfx` file - MSBuild's key
file import fails with `APPX0105` even when the password is correct.

```powershell
$msbuild = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2017\BuildTools\MSBuild\15.0\Bin\MSBuild.exe"
& $msbuild WifiAnalyzer.csproj /t:Restore /p:Configuration=Release /p:Platform=ARM
& $msbuild WifiAnalyzer.csproj /p:Configuration=Release /p:Platform=ARM `
    /p:PackageCertificateThumbprint=$($cert.Thumbprint)
```

Use `ARM`. Windows 10 Mobile devices are all 32-bit ARM; `x86` only builds for the
phone emulator.

Then follow "Installing on a phone" above.

If you are changing the code rather than just building it, read
[AGENTS.md](AGENTS.md) first.

## How to read the graph

- **Horizontal axis** - channel number. A curve is 22 MHz wide because that is
  what a 20 MHz channel actually occupies; overlapping curves are networks that
  interfere with each other.
- **Vertical axis** - signal strength in dBm. Closer to 0 is stronger: -50 is
  excellent, -70 is usable, below -85 is barely there.
- On 2.4 GHz, channels **1, 6 and 11** are the only three that do not overlap.
  Pick whichever of them has the least under it.

## What is not supported

- **Channel width is not detected.** The Wi-Fi API available on this OS does not
  report it, and gives no access to the beacon data the macOS and Haiku ports read it
  from, so 40 and 80 MHz networks are drawn 22 MHz wide like everything else.
- **Hidden networks** show as `(hidden)` - the beacon carries no name.
- **6 GHz** - no Windows 10 Mobile device has the radio.
- The app never connects to anything; it only listens.

## License

MIT - see [LICENSE](../LICENSE).

## AI Disclaimer

This application was produced by a human working with Claude.
