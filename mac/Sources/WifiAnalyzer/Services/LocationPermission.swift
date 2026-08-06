import CoreLocation

/// Asks macOS for location authorization, which is the only thing that makes
/// network names visible - and is all this type does.
///
/// macOS treats a Wi-Fi network's identity as location data, since a BSSID can be
/// looked up on a map, so CoreWLAN redacts `ssid`, `bssid` and `countryCode`
/// unless the app is authorized for location. `wlanChannel` and `rssiValue` are
/// never redacted, so the graph is complete either way and only the labels read
/// "(hidden)".
///
/// **No location is ever read.** There is no `startUpdatingLocation`, no delegate
/// method that receives coordinates, and no use of CoreLocation beyond the
/// authorization request and its result. The reasoning is in README.md, not in the
/// UI: the system prompt is the only thing the user should have to read.
///
/// The CLLocationManager has to stay alive for the authorization callback to
/// arrive, which is why this is an object and not a free function.
final class LocationPermission: NSObject, CLLocationManagerDelegate {
    private let manager = CLLocationManager()

    /// Called on the main thread when authorization changes.
    var onChange: (() -> Void)?

    override init() {
        super.init()
        manager.delegate = self
    }

    /// Raises the system prompt on first run, and does nothing on every run after
    /// that - macOS only asks once.
    func request() {
        guard manager.authorizationStatus == .notDetermined else { return }
        manager.requestWhenInUseAuthorization()
    }

    func locationManagerDidChangeAuthorization(_ manager: CLLocationManager) {
        onChange?()
    }
}
