using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.Devices.WiFi;
using Windows.Networking.Connectivity;
using WifiAnalyzer.Models;

namespace WifiAnalyzer.Services
{
    /// <summary>
    /// Wraps Windows.Devices.WiFi into "give me a list of access points".
    ///
    /// Everything used here shipped in the first UWP release (10240), which matters:
    /// the app is built against the 19041 SDK but must run on 10586, and the
    /// compiler does not check API availability. See AGENTS.md.
    /// </summary>
    public sealed class WifiScanner
    {
        private WiFiAdapter _adapter;

        /// <summary>Human-readable reason the last Initialize or Scan failed.</summary>
        public string LastError { get; private set; }

        public bool IsReady { get { return _adapter != null; } }

        /// <summary>
        /// Asks for Wi-Fi access and picks an adapter. Must be called from the UI
        /// thread: RequestAccessAsync shows a consent prompt on first run and
        /// throws from a background thread.
        /// </summary>
        public async Task<bool> InitializeAsync()
        {
            try
            {
                var access = await WiFiAdapter.RequestAccessAsync();
                if (access != WiFiAccessStatus.Allowed)
                {
                    LastError = "Wi-Fi access denied (" + access + "). " +
                                "Settings > Privacy > Other devices.";
                    return false;
                }

                var adapters = await WiFiAdapter.FindAllAdaptersAsync();
                if (adapters.Count == 0)
                {
                    LastError = "No Wi-Fi adapter found.";
                    return false;
                }

                _adapter = adapters[0];
                LastError = null;
                return true;
            }
            catch (Exception error)
            {
                LastError = "Wi-Fi init failed: " + error.Message;
                App.Log("InitializeAsync failed: " + error);
                return false;
            }
        }

        /// <summary>
        /// Triggers a scan and returns what the adapter saw. A failing ScanAsync is
        /// not fatal - the OS scans on its own schedule and NetworkReport still
        /// holds those results, so the report is read either way.
        /// </summary>
        public async Task<IReadOnlyList<AccessPoint>> ScanAsync()
        {
            if (_adapter == null) return new List<AccessPoint>();

            try
            {
                await _adapter.ScanAsync();
                LastError = null;
            }
            catch (Exception error)
            {
                // Scans are rate-limited by the driver; a refused scan just means
                // the report below is a few seconds stale.
                LastError = "Scan refused: " + error.Message;
            }

            var byBssid = new Dictionary<string, AccessPoint>();
            try
            {
                var report = _adapter.NetworkReport;
                foreach (var network in report.AvailableNetworks)
                {
                    var mhz = (int)(network.ChannelCenterFrequencyInKilohertz / 1000);
                    var bssid = network.Bssid ?? string.Empty;

                    // WiFiAvailableNetwork reports no channel width and no
                    // information elements to derive one from, so every network is
                    // recorded as a 20 MHz channel centred on its primary. The other
                    // two ports read the width out of the beacon's HT/VHT Operation
                    // elements and draw the curve that wide; there is nothing here to
                    // read. PhyKind says Ht or Vht but not at what width.
                    var ap = new AccessPoint
                    {
                        Ssid = string.IsNullOrEmpty(network.Ssid) ? "(hidden)" : network.Ssid,
                        Bssid = bssid,
                        FrequencyMhz = mhz,
                        CentreMhz = mhz,
                        WidthMhz = 20,
                        Channel = ChannelMap.ToChannel(mhz),
                        Rssi = network.NetworkRssiInDecibelMilliwatts,
                        Band = ChannelMap.BandOf(mhz),
                        Security = Describe(network.SecuritySettings)
                    };

                    // One BSSID can appear twice in a report when the driver merges
                    // two probe responses. Keep the stronger reading.
                    AccessPoint existing;
                    if (byBssid.TryGetValue(bssid, out existing) && existing.Rssi >= ap.Rssi) continue;
                    byBssid[bssid] = ap;
                }
            }
            catch (Exception error)
            {
                LastError = "Could not read scan results: " + error.Message;
                App.Log("NetworkReport failed: " + error);
            }

            return new List<AccessPoint>(byBssid.Values);
        }

        private static string Describe(NetworkSecuritySettings settings)
        {
            if (settings == null) return string.Empty;
            switch (settings.NetworkAuthenticationType)
            {
                case NetworkAuthenticationType.Open80211: return "Open";
                case NetworkAuthenticationType.SharedKey80211: return "WEP";
                case NetworkAuthenticationType.Wpa:
                case NetworkAuthenticationType.WpaPsk:
                case NetworkAuthenticationType.WpaNone: return "WPA";
                case NetworkAuthenticationType.Rsna:
                case NetworkAuthenticationType.RsnaPsk: return "WPA2";
                default: return settings.NetworkAuthenticationType.ToString();
            }
        }
    }
}
