// swift-tools-version:5.9
//
// Swift 5 language mode on purpose: CWNetwork and CWInterface are not Sendable,
// and the scan has to cross a thread boundary. See AGENTS.md.

import PackageDescription

let package = Package(
    name: "WifiAnalyzer",
    platforms: [.macOS(.v14)],
    targets: [
        .executableTarget(
            name: "WifiAnalyzer",
            path: "Sources/WifiAnalyzer"
        )
    ]
)
