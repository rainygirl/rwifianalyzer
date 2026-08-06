import CoreWLAN
import Foundation

/// What one scan produced. Returned whole rather than left on the scanner as
/// mutable state, so the scan can run off the main thread without anything
/// needing to be Sendable but this.
struct ScanResult: Sendable {
    var points: [AccessPoint] = []
    var interfaceName: String?
    /// Human-readable reason the scan was degraded or empty. Not fatal on its own.
    var error: String?
}

/// Wraps CoreWLAN into "give me a list of access points".
///
/// Stateless on purpose - see ScanResult.
enum WifiScanner {
    /// Blocking, and slow: a full scan sweeps every channel and takes on the order
    /// of a second. Must not be called on the main thread.
    static func scan() -> ScanResult {
        guard let interface = CWWiFiClient.shared().interface() else {
            return ScanResult(error: "No Wi-Fi interface found.")
        }

        var result = ScanResult(interfaceName: interface.interfaceName)

        let networks: Set<CWNetwork>
        do {
            networks = try interface.scanForNetworks(withSSID: nil, includeHidden: true)
        } catch {
            // The driver rate-limits scans and refuses one issued too soon after
            // the last. The cache still holds the previous sweep, which is at worst
            // a few seconds stale, so a refused scan is not an error - treating it
            // as one would blank the graph every other refresh.
            result.error = "Scan refused: \(error.localizedDescription)"
            networks = interface.cachedScanResults() ?? []
        }

        var byKey: [String: AccessPoint] = [:]
        for network in networks {
            guard let channel = network.wlanChannel,
                  let band = ChannelMap.band(of: channel.channelBand.rawValue),
                  channel.channelNumber > 0
            else { continue }

            let primaryMhz = ChannelMap.frequency(channel: channel.channelNumber,
                                                  band: band)
            let geometry = self.geometry(of: network,
                                         primaryMhz: primaryMhz,
                                         reportedWidth: nominalWidth(channel.channelWidth))

            let ap = AccessPoint(
                ssid: network.ssid ?? "(hidden)",
                bssid: network.bssid,
                frequencyMhz: primaryMhz,
                centreMhz: geometry.centreMhz,
                widthMhz: geometry.widthMhz,
                channel: channel.channelNumber,
                rssi: Double(network.rssiValue),
                band: band,
                security: describe(network)
            )

            // One radio can appear twice in a scan when it answered on two
            // sweeps. Keep the stronger reading.
            if let existing = byKey[ap.id], existing.rssi >= ap.rssi { continue }
            byKey[ap.id] = ap
        }

        result.points = Array(byKey.values)
        return result
    }

    /// Where the occupied channel actually sits, and how wide it is.
    ///
    /// CWChannel reports the width but only the *primary* channel number, and a
    /// 40 MHz BSS puts its extension channel on one side of the primary. Drawing a
    /// 42 MHz curve centred on the primary would cover 20 MHz of spectrum that is
    /// in fact free, so the centre is read from the beacon's own HT and VHT
    /// Operation elements, which state it outright.
    ///
    /// `informationElementData` needs no permission, unlike the SSID.
    ///
    /// When the width is over 20 MHz and the centre cannot be established - a
    /// 6 GHz BSS, whose centre lives in the HE Operation element this does not
    /// parse, or an AP that sent no operation element - the curve falls back to the
    /// primary 20 MHz. That much is certainly occupied; the rest would be a guess.
    private static func geometry(of network: CWNetwork, primaryMhz: Int,
                                 reportedWidth: Int) -> (centreMhz: Int, widthMhz: Int) {
        guard reportedWidth > 20, let elements = network.informationElementData else {
            return (primaryMhz, 20)
        }

        let bytes = [UInt8](elements)
        var centreMhz: Int?
        var index = 0

        while index + 1 < bytes.count {
            let id = bytes[index]
            let length = Int(bytes[index + 1])
            let body = index + 2
            guard body + length <= bytes.count else { break }

            switch id {
            case 61 where length >= 2:
                // HT Operation. Bit 2 of the second octet is the STA channel
                // width, bits 0-1 the secondary channel offset: 1 above, 3 below.
                let info = bytes[body + 1]
                if info & 0x04 != 0 {
                    switch info & 0x03 {
                    case 1: centreMhz = primaryMhz + 10
                    case 3: centreMhz = primaryMhz - 10
                    default: break
                    }
                }
            case 192 where length >= 2:
                // VHT Operation names its centre as a channel number, and only
                // ever in the 5 GHz plan.
                let segment0 = Int(bytes[body + 1])
                if bytes[body] != 0, segment0 != 0 {
                    centreMhz = 5000 + segment0 * 5
                }
            default:
                break
            }

            index = body + length
        }

        guard let centre = centreMhz else { return (primaryMhz, 20) }
        return (centre, reportedWidth)
    }

    private static func nominalWidth(_ width: CWChannelWidth) -> Int {
        switch width {
        case .width20MHz: return 20
        case .width40MHz: return 40
        case .width80MHz: return 80
        case .width160MHz: return 160
        default: return 20
        }
    }

    /// Strongest scheme the network advertises. CWNetwork answers one security
    /// type at a time rather than handing over a list, so this walks down from the
    /// top - a WPA3 transition network also reports WPA2.
    private static func describe(_ network: CWNetwork) -> String {
        if network.supportsSecurity(.wpa3Personal) || network.supportsSecurity(.wpa3Enterprise)
            || network.supportsSecurity(.wpa3Transition) {
            return "WPA3"
        }
        if network.supportsSecurity(.wpa2Personal) || network.supportsSecurity(.wpa2Enterprise) {
            return "WPA2"
        }
        if network.supportsSecurity(.wpaPersonal) || network.supportsSecurity(.wpaEnterprise) {
            return "WPA"
        }
        if network.supportsSecurity(.WEP) { return "WEP" }
        if network.supportsSecurity(.none) { return "Open" }
        return "Unknown"
    }
}
