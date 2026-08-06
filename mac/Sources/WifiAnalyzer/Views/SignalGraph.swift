import SwiftUI

/// Draws the channel-vs-strength graph into a SwiftUI Canvas.
///
/// Everything is redrawn from the current list on every scan; at the few dozen
/// networks a machine can see there is nothing to gain from diffing. Canvas
/// rather than a stack of Shapes because the text has to be measured to place
/// labels, and `GraphicsContext.resolve` is the only cheap way to do that.
struct SignalGraph: View {
    let points: [AccessPoint]
    let band: Band

    @Environment(\.colorScheme) private var colorScheme

    // Gutters for the axis labels. The plot area is what is left over.
    private let leftGutter: CGFloat = 40
    private let bottomGutter: CGFloat = 24
    private let topPad: CGFloat = 20
    private let rightPad: CGFloat = 12

    // dBm range of the vertical axis. -20 is closer than any real reading and
    // -100 is the noise floor, so every curve fits without clipping.
    private let topDbm: Double = -20
    private let bottomDbm: Double = -100

    var body: some View {
        Canvas { context, size in
            draw(&context, size: size)
        }
    }

    // MARK: - Colours

    private static let palette: [Color] = [
        Color(red: 0.90, green: 0.10, blue: 0.29), Color(red: 0.24, green: 0.71, blue: 0.29),
        Color(red: 1.00, green: 0.88, blue: 0.10), Color(red: 0.26, green: 0.39, blue: 0.85),
        Color(red: 0.96, green: 0.51, blue: 0.19), Color(red: 0.57, green: 0.12, blue: 0.71),
        Color(red: 0.27, green: 0.94, blue: 0.94), Color(red: 0.94, green: 0.20, blue: 0.90),
        Color(red: 0.74, green: 0.96, blue: 0.05), Color(red: 0.98, green: 0.75, blue: 0.75),
        Color(red: 0.18, green: 0.77, blue: 0.71), Color(red: 0.90, green: 0.75, blue: 1.00)
    ]

    private var plotBackground: Color {
        colorScheme == .dark ? Color(white: 0.04) : Color(white: 0.99)
    }

    private var gridColor: Color {
        colorScheme == .dark ? Color(white: 1, opacity: 0.24) : Color(white: 0, opacity: 0.14)
    }

    private var axisTextColor: Color {
        colorScheme == .dark ? Color(white: 1, opacity: 0.7) : Color(white: 0, opacity: 0.6)
    }

    // MARK: - Drawing

    private func draw(_ context: inout GraphicsContext, size: CGSize) {
        context.fill(Path(CGRect(origin: .zero, size: size)), with: .color(plotBackground))

        guard size.width > 120, size.height > 100 else { return } // not laid out yet

        let plot = CGRect(x: leftGutter,
                          y: topPad,
                          width: size.width - leftGutter - rightPad,
                          height: size.height - topPad - bottomGutter)

        let visible = points.filter { $0.band == band && $0.channel > 0 }
        let (minMhz, maxMhz) = domain(for: visible)

        drawDbmAxis(&context, plot: plot)
        drawChannelAxis(&context, plot: plot, minMhz: minMhz, maxMhz: maxMhz)

        if visible.isEmpty {
            context.draw(text("No networks on this band", size: 12, color: axisTextColor),
                         at: CGPoint(x: plot.midX, y: plot.midY))
            return
        }

        // Strongest first, so the tall wide curves end up behind the short ones and
        // a weak network is not hidden under a translucent neighbour. Labels are
        // placed in the same order, so the strongest network keeps the spot it
        // wants and weaker ones stack above it.
        var placed: [CGRect] = []
        for ap in visible.sorted(by: { $0.rssi > $1.rssi }) {
            let anchor = drawCurve(&context, ap: ap, plot: plot, minMhz: minMhz, maxMhz: maxMhz)
            drawLabel(&context, ap: ap, anchor: anchor, plot: plot, placed: &placed)
        }
    }

    /// The 2.4 GHz axis is fixed - it is only 100 MHz wide and channels 1..14 all
    /// fit with room to label them. The 5 and 6 GHz plans span 700 MHz and 1.2 GHz,
    /// where a 22 MHz curve would be a sliver, so those axes are fitted to the
    /// networks actually present and widened to a 200 MHz minimum. The axis moves
    /// when the set of channels in range changes, which is rare in practice.
    private func domain(for visible: [AccessPoint]) -> (Int, Int) {
        if band == .twoPointFour { return (2400, 2500) }

        guard !visible.isEmpty else {
            return band == .five ? (5170, 5370) : (5945, 6145)
        }

        // Fitted to the edges of the occupied channels rather than to their centres,
        // so an 80 MHz BSS at the end of the range is not half off the axis, plus half
        // a channel of margin so the outermost curve does not sit on the axis itself.
        func edge(_ ap: AccessPoint) -> Int {
            ChannelMap.occupiedWidth(nominalMhz: ap.widthMhz) / 2 + ChannelMap.channelWidthMhz / 2
        }
        var lo = visible.map { $0.centreMhz - edge($0) }.min()!
        var hi = visible.map { $0.centreMhz + edge($0) }.max()!

        let shortfall = 200 - (hi - lo)
        if shortfall > 0 {
            lo -= shortfall / 2
            hi += shortfall - shortfall / 2
        }
        return (lo, hi)
    }

    private func drawDbmAxis(_ context: inout GraphicsContext, plot: CGRect) {
        for dbm in stride(from: -30, through: -90, by: -10) {
            let y = toY(Double(dbm), plot: plot)
            line(&context, from: CGPoint(x: plot.minX, y: y), to: CGPoint(x: plot.maxX, y: y))
            context.draw(text("\(dbm)", size: 10, color: axisTextColor),
                         at: CGPoint(x: plot.minX - 6, y: y), anchor: .trailing)
        }

        // Plot border. The left and bottom edges double as the axes.
        context.stroke(Path(plot), with: .color(gridColor), lineWidth: 1)
    }

    private func drawChannelAxis(_ context: inout GraphicsContext, plot: CGRect,
                                 minMhz: Int, maxMhz: Int) {
        var lastLabelMaxX = -CGFloat.greatestFiniteMagnitude

        for channel in ChannelMap.channels(in: band) {
            let mhz = ChannelMap.frequency(channel: channel, band: band)
            guard mhz >= minMhz, mhz <= maxMhz else { continue }

            let x = toX(Double(mhz), minMhz: minMhz, maxMhz: maxMhz, plot: plot)
            guard x >= plot.minX, x <= plot.maxX else { continue }

            line(&context, from: CGPoint(x: x, y: plot.minY), to: CGPoint(x: x, y: plot.maxY))

            // Drop labels that would collide. The gridline still marks the channel,
            // so a crowded axis stays readable rather than becoming a smear.
            let label = text("\(channel)", size: 10, color: axisTextColor)
            let labelWidth = context.resolve(label).measure(in: plot.size).width
            guard x - labelWidth / 2 - 6 > lastLabelMaxX else { continue }

            context.draw(label, at: CGPoint(x: x, y: plot.maxY + 4), anchor: .top)
            lastLabelMaxX = x + labelWidth / 2
        }
    }

    /// Draws one bell curve and returns the point its label wants to sit above.
    private func drawCurve(_ context: inout GraphicsContext, ap: AccessPoint,
                           plot: CGRect, minMhz: Int, maxMhz: Int) -> CGPoint {
        let color = Self.palette[colorIndex(ap.id)]

        // The curve is as wide as the channel really is - 22 MHz for a 20 MHz
        // channel, 42 for a 40 - and sits on the centre of the occupied channel
        // rather than on the primary. That is what makes a wide BSS visibly cover
        // its neighbours.
        let half = Double(ChannelMap.occupiedWidth(nominalMhz: ap.widthMhz)) / 2
        let centre = Double(ap.centreMhz)
        let xCentre = toX(centre, minMhz: minMhz, maxMhz: maxMhz, plot: plot)
        let xLeft = toX(centre - half, minMhz: minMhz, maxMhz: maxMhz, plot: plot)
        let xRight = toX(centre + half, minMhz: minMhz, maxMhz: maxMhz, plot: plot)
        let yPeak = toY(ap.rssi, plot: plot)

        // Two mirrored cubic segments make the bell: flat at the baseline, flat at
        // the peak, steep in between. The 0.6 is the control-point offset as a
        // fraction of the half-width - lower looks spiky, higher looks square.
        let halfWidth = xCentre - xLeft
        var curve = Path()
        curve.move(to: CGPoint(x: xLeft, y: plot.maxY))
        curve.addCurve(to: CGPoint(x: xCentre, y: yPeak),
                       control1: CGPoint(x: xLeft + halfWidth * 0.6, y: plot.maxY),
                       control2: CGPoint(x: xCentre - halfWidth * 0.6, y: yPeak))
        curve.addCurve(to: CGPoint(x: xRight, y: plot.maxY),
                       control1: CGPoint(x: xCentre + halfWidth * 0.6, y: yPeak),
                       control2: CGPoint(x: xRight - halfWidth * 0.6, y: plot.maxY))
        curve.closeSubpath()

        // A curve at the edge of the domain runs past the plot area, so the fill
        // and stroke are clipped - but the label is not, because it sits above the
        // peak and would be cut off at the top of the plot.
        var clipped = context
        clipped.clip(to: Path(plot))
        clipped.fill(curve, with: .color(color.opacity(0.33)))
        clipped.stroke(curve, with: .color(color), lineWidth: 1.5)

        return CGPoint(x: xCentre, y: yPeak)
    }

    /// Places a label above its peak, nudging it up until it clears the labels
    /// already placed. A desktop window shows every network in range at once, and
    /// a dozen names all drawn at their own peak height overlap into a smear.
    private func drawLabel(_ context: inout GraphicsContext, ap: AccessPoint,
                           anchor: CGPoint, plot: CGRect, placed: inout [CGRect]) {
        // Just the name. The channel width is in the width of the curve, which is
        // where it belongs - a number in the label says nothing about what it
        // overlaps.
        let label = text(ap.ssid, size: 11, color: Self.palette[colorIndex(ap.id)],
                         bold: true)
        let size = context.resolve(label).measure(in: plot.size)

        let x = min(max(anchor.x, plot.minX + size.width / 2), plot.maxX - size.width / 2)
        var bottom = anchor.y - 4

        // Six steps is enough to unstack a busy channel; past that the graph is
        // too crowded for labels to help anyway and one is left overlapping.
        for _ in 0..<6 {
            let rect = CGRect(x: x - size.width / 2, y: bottom - size.height,
                              width: size.width, height: size.height)
            if !placed.contains(where: { $0.intersects(rect) }) || rect.minY < 2 { break }
            bottom -= size.height
        }

        bottom = max(bottom, size.height + 2)
        placed.append(CGRect(x: x - size.width / 2, y: bottom - size.height,
                             width: size.width, height: size.height))
        context.draw(label, at: CGPoint(x: x, y: bottom), anchor: .bottom)
    }

    // MARK: - Primitives

    private func line(_ context: inout GraphicsContext, from: CGPoint, to: CGPoint) {
        var path = Path()
        path.move(to: from)
        path.addLine(to: to)
        context.stroke(path, with: .color(gridColor), lineWidth: 1)
    }

    private func text(_ string: String, size: CGFloat, color: Color, bold: Bool = false) -> Text {
        Text(string)
            .font(.system(size: size, weight: bold ? .semibold : .regular))
            .foregroundStyle(color)
    }

    private func toX(_ mhz: Double, minMhz: Int, maxMhz: Int, plot: CGRect) -> CGFloat {
        plot.minX + (mhz - Double(minMhz)) / Double(maxMhz - minMhz) * plot.width
    }

    private func toY(_ dbm: Double, plot: CGRect) -> CGFloat {
        let clamped = min(max(dbm, bottomDbm), topDbm)
        return plot.minY + (topDbm - clamped) / (topDbm - bottomDbm) * plot.height
    }

    /// FNV-1a rather than Swift's Hasher: Hasher is seeded per process, which would
    /// repaint every network a different colour on each launch. A network must keep
    /// its colour.
    private func colorIndex(_ key: String) -> Int {
        var hash: UInt32 = 2166136261
        for byte in key.utf8 {
            hash = (hash ^ UInt32(byte)) &* 16777619
        }
        return Int(hash % UInt32(Self.palette.count))
    }
}
