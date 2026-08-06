import SwiftUI

/// A plain windowed app. It has to be a real GUI app with a run loop that keeps
/// running: CoreWLAN hands back empty SSIDs to a command-line tool that only
/// spins its run loop while waiting for location authorization. See AGENTS.md.
@main
struct WifiAnalyzerApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) private var delegate

    var body: some Scene {
        Window("R WiFi Analyzer", id: "main") {
            MainView()
        }
        .defaultSize(width: 900, height: 560)
    }
}

final class AppDelegate: NSObject, NSApplicationDelegate {
    func applicationDidFinishLaunching(_ notification: Notification) {
        // The bundle is assembled by the Makefile rather than Xcode, so nothing has
        // told AppKit this is a foreground app.
        NSApp.setActivationPolicy(.regular)
        NSApp.activate(ignoringOtherApps: true)
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ app: NSApplication) -> Bool {
        true
    }
}
