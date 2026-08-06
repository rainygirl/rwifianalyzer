import Foundation

/// One BSS from a scan. A value type, not a live object: every scan produces a
/// fresh list, because CWNetwork instances belong to the scan record they came
/// from and are replaced wholesale on the next scan.
struct AccessPoint: Identifiable, Sendable {
    /// Network name, or "(hidden)". CoreWLAN also returns nil here for *every*
    /// network when the app is not authorized for location - see AGENTS.md.
    let ssid: String

    /// MAC of the radio. nil unless location access is granted, which is why it
    /// cannot be the identity of the record.
    let bssid: String?

    /// Centre of the primary 20 MHz channel.
    let frequencyMhz: Int

    /// Centre of the whole occupied channel, which is not `frequencyMhz` when the
    /// BSS is wider than 20 MHz.
    let centreMhz: Int

    /// Channel width in MHz - the width the curve is drawn at. 20 unless the width
    /// *and* the centre are both known; see WifiScanner.
    let widthMhz: Int

    let channel: Int

    /// Signal strength in dBm, typically -30 (very close) to -95.
    let rssi: Double

    let band: Band

    let security: String

    /// Stable per-network key: the BSSID when we are allowed to see it, otherwise
    /// name plus channel, which is the most specific thing left. Used both to
    /// deduplicate a scan and to keep a network's colour between scans.
    var id: String { bssid ?? "\(ssid)@\(channel)" }
}

enum Band: Int, CaseIterable, Identifiable, Sendable {
    case twoPointFour
    case five
    case six

    var id: Int { rawValue }

    var label: String {
        switch self {
        case .twoPointFour: return "2.4 GHz"
        case .five: return "5 GHz"
        case .six: return "6 GHz"
        }
    }
}
