# Copies the signed package and its certificate to the phone's Downloads folder
# over MTP. Run it *on the Windows machine*, from an interactive desktop session:
#
#   powershell -ExecutionPolicy Bypass -File tools\copy-to-phone.ps1
#   powershell -ExecutionPolicy Bypass -File tools\copy-to-phone.ps1 -Source C:\path\to\AppPackages\..._Test
#
# Then tap the .cer on the phone, then the .appx. Nothing here can do that part -
# every tool that could install remotely needs the IpOverUsb service, which
# Microsoft no longer ships. See AGENTS.md.
#
# From a remote session the shell's MTP namespace is invisible and this exits with
# "no phone", reporting no error of its own. Run it as a scheduled task instead:
#
#   $sid = ([System.Security.Principal.WindowsIdentity]::GetCurrent()).User.Value
#   $action = New-ScheduledTaskAction -Execute 'powershell.exe' `
#       -Argument '-NoProfile -ExecutionPolicy Bypass -File C:\...\copy-to-phone.ps1'
#   $principal = New-ScheduledTaskPrincipal -UserId $sid -LogonType Interactive `
#       -RunLevel Highest
#   Register-ScheduledTask -TaskName RWifiMtpCopy -Action $action -Principal $principal -Force
#   Start-ScheduledTask -TaskName RWifiMtpCopy
#
# Use the SID, not "$env:USERDOMAIN\$env:USERNAME": in a remote session USERDOMAIN
# is WORKGROUP on a machine that is not domain-joined, and Register-ScheduledTask
# fails with "No mapping between account names and security IDs was done."

param(
    # Defaults to the newest AppPackages\*_Test directory next to this script's project.
    [string]$Source,
    [string]$LogPath = "$env:TEMP\copy-to-phone.log"
)

$ErrorActionPreference = 'Continue'

"start $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" | Out-File $LogPath -Encoding utf8
function Log($message) {
    $line = "$(Get-Date -Format 'HH:mm:ss') $message"
    $line | Out-File -FilePath $LogPath -Append -Encoding utf8
    Write-Host $line
}

if (-not $Source) {
    $project = Split-Path -Parent $PSScriptRoot
    $Source = Get-ChildItem (Join-Path $project 'AppPackages') -Directory -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $Source -or -not (Test-Path $Source)) { Log 'FAIL no AppPackages directory'; exit 1 }
Log "source = $Source"

$appx = Get-ChildItem $Source -Filter *.appx | Select-Object -First 1
$cer = Get-ChildItem $Source -Filter *.cer | Select-Object -First 1
if (-not $appx -or -not $cer) { Log 'FAIL no .appx or .cer in source'; exit 1 }

# Certificate first. The app install fails silently without it.
$files = @($cer, $appx)

$shell = New-Object -ComObject Shell.Application
$pc = $shell.NameSpace(17)          # This PC
if ($null -eq $pc) { Log 'FAIL no This PC namespace'; exit 1 }

$phone = $null
foreach ($item in $pc.Items()) { if ($item.Name -eq 'Windows phone') { $phone = $item } }
if ($null -eq $phone) {
    $names = @(); foreach ($item in $pc.Items()) { $names += $item.Name }
    Log ('FAIL no phone - is it connected, and is this an interactive session? items=' +
        ($names -join '|'))
    exit 1
}
Log ("phone = " + $phone.Name)

# First child of the phone is its internal storage ("Phone" on a Lumia).
$storage = $null
foreach ($item in $phone.GetFolder.Items()) { if ($null -eq $storage) { $storage = $item } }
if ($null -eq $storage) { Log 'FAIL phone exposes no storage'; exit 1 }

$downloads = $null
foreach ($item in $storage.GetFolder.Items()) { if ($item.Name -eq 'Downloads') { $downloads = $item } }
if ($null -eq $downloads) {
    $names = @(); foreach ($item in $storage.GetFolder.Items()) { $names += $item.Name }
    Log ('FAIL no Downloads folder; items=' + ($names -join '|')); exit 1
}
$destination = $downloads.GetFolder
Log ("destination = " + $storage.Name + "\Downloads")

foreach ($file in $files) {
    Log ("copy " + $file.Name + " (" + $file.Length + " bytes)")
    # 16 = answer Yes to All. Existing files are never deleted first:
    # FolderItem.InvokeVerb('delete') hangs waiting for a prompt on the device.
    $destination.CopyHere($file.FullName, 16)
}

# CopyHere is asynchronous and returns nothing, so the copy has to be confirmed by
# polling - and MTP reports names *without* their extension, so match the base name.
$want = @(); foreach ($file in $files) { $want += [IO.Path]::GetFileNameWithoutExtension($file.Name) }
$deadline = (Get-Date).AddSeconds(240)
$missing = $want
while ($missing.Count -gt 0 -and (Get-Date) -lt $deadline) {
    Start-Sleep -Seconds 4
    $names = @(); foreach ($item in $destination.Items()) { $names += $item.Name }
    $still = @()
    foreach ($base in $want) {
        $found = $false
        foreach ($name in $names) {
            if ($name -eq $base -or $name -eq "$base.cer" -or $name -eq "$base.appx") { $found = $true }
        }
        if (-not $found) { $still += $base }
    }
    $missing = $still
}

foreach ($item in $destination.Items()) {
    if ($item.Name -like ([IO.Path]::GetFileNameWithoutExtension($appx.Name).Split('_')[0] + '*')) {
        Log ('on phone: ' + $item.Name + '  size=' + $item.ExtendedProperty('System.Size'))
    }
}

if ($missing.Count -gt 0) {
    Log ('FAIL not confirmed on phone: ' + ($missing -join ','))
    exit 1
}
Log 'OK - now tap the .cer on the phone, then the .appx'
