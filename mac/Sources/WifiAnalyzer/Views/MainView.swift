import SwiftUI

/// Holds the current scan and reruns it on a timer.
@MainActor
final class ScanModel: ObservableObject {
    // 5 s is about as fast as the driver will produce a fresh sweep; asking more
    // often gets the same cache back and costs airtime, since an active scan makes
    // the radio leave its channel.
    private static let scanInterval: Duration = .seconds(5)

    @Published private(set) var points: [AccessPoint] = []
    @Published private(set) var status = "Scanning..."
    @Published var band: Band = .twoPointFour {
        didSet { updateStatus() }
    }

    private let location = LocationPermission()
    private var loop: Task<Void, Never>?
    private var lastResult = ScanResult()

    /// A scan can outlast the interval, so overlapping runs are suppressed rather
    /// than queued.
    private var scanning = false

    func start() {
        guard loop == nil else { return }

        location.onChange = { [weak self] in
            // Names appear the moment access is granted, so rescan instead of
            // leaving the graph labelled "(hidden)" until the next tick.
            Task { await self?.scanOnce() }
        }

        // The graph itself needs no permission; this is only what stops macOS
        // redacting the network names. See LocationPermission and README.md.
        location.request()

        loop = Task { [weak self] in
            while !Task.isCancelled {
                await self?.scanOnce()
                try? await Task.sleep(for: Self.scanInterval)
            }
        }
    }

    func stop() {
        loop?.cancel()
        loop = nil
    }

    func scanOnce() async {
        if scanning { return }
        scanning = true

        // scanForNetworks blocks for about a second, so it runs off the main actor.
        lastResult = await Task.detached(priority: .userInitiated) { WifiScanner.scan() }.value
        points = lastResult.points

        scanning = false
        updateStatus()
    }

    private static let clock: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm:ss"
        return formatter
    }()

    private func updateStatus() {
        var parts: [String] = []
        if let name = lastResult.interfaceName { parts.append(name) }

        let onBand = points.filter { $0.band == band }.count
        parts.append("\(onBand) of \(points.count) networks on \(band.label)")
        parts.append(Self.clock.string(from: Date()))

        if let error = lastResult.error { parts.append(error) }

        status = parts.joined(separator: "  -  ")
    }
}

struct MainView: View {
    @StateObject private var model = ScanModel()

    var body: some View {
        VStack(spacing: 0) {
            SignalGraph(points: model.points, band: model.band)
                .padding(.horizontal, 8)
                .padding(.top, 8)
                .padding(.bottom, 4)

            Text(model.status)
                .font(.caption)
                .foregroundStyle(.secondary)
                .lineLimit(2)
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding(.horizontal, 12)
                .padding(.bottom, 8)
        }
        .frame(minWidth: 560, minHeight: 380)
        // The window title already says what the app is, so the controls live in
        // the title bar rather than in a header row that repeats it.
        .toolbar {
            // The bands are separate graphs rather than one axis, because they are
            // hundreds of MHz apart and would leave most of a shared axis empty.
            ToolbarItem(placement: .principal) {
                Picker("Band", selection: $model.band) {
                    ForEach(Band.allCases) { band in
                        Text(band.label).tag(band)
                    }
                }
                .pickerStyle(.segmented)
                .labelsHidden()
                .fixedSize()
            }

            ToolbarItem(placement: .primaryAction) {
                Button {
                    Task { await model.scanOnce() }
                } label: {
                    Image(systemName: "arrow.clockwise")
                }
                .help("Rescan now")
            }
        }
        .task { model.start() }
    }
}
