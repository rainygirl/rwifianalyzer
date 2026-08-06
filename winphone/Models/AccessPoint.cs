namespace WifiAnalyzer.Models
{
    /// <summary>
    /// One BSS from a scan. A record, not a live object: every scan produces a
    /// fresh list, because WiFiAvailableNetwork instances belong to the report
    /// they came from and are replaced wholesale on the next ScanAsync.
    /// </summary>
    public sealed class AccessPoint
    {
        /// <summary>Network name, or "(hidden)" when the beacon carries none.</summary>
        public string Ssid { get; set; }

        /// <summary>MAC of the radio. Distinguishes the two bands of one router.</summary>
        public string Bssid { get; set; }

        /// <summary>Centre of the primary 20 MHz channel.</summary>
        public int FrequencyMhz { get; set; }

        /// <summary>
        /// Centre of the whole occupied channel. Always the same as FrequencyMhz
        /// here: this OS never says a network is wider than 20 MHz. See WifiScanner.
        /// </summary>
        public int CentreMhz { get; set; }

        /// <summary>
        /// Channel width in MHz - the width the curve is drawn at. Always 20 on this
        /// OS, for the same reason.
        /// </summary>
        public int WidthMhz { get; set; }

        /// <summary>0 when the frequency falls outside any band this app knows.</summary>
        public int Channel { get; set; }

        /// <summary>Signal strength in dBm, typically -30 (very close) to -95.</summary>
        public double Rssi { get; set; }

        public Band Band { get; set; }

        public string Security { get; set; }
    }

    public enum Band
    {
        TwoPointFour,
        Five
    }
}
