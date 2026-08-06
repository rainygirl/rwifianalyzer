/// Channel-number to frequency arithmetic, and which channel numbers exist.
///
/// CWChannel hands us a channel number and a band but no frequency, and the
/// graph is drawn against frequency, so the plan has to be spelled out here. The
/// 2.4 GHz plan is 2412 MHz for channel 1 and 5 MHz per step, with channel 14 an
/// exception at 2484. The 5 GHz plan is a flat 5000 + 5 * channel. The 6 GHz plan
/// is 5950 + 5 * channel, with channel 2 an exception at 5935.
enum ChannelMap {
    /// A 20 MHz channel actually occupies ~22 MHz of spectrum, which is why
    /// adjacent 2.4 GHz channels overlap. Curves are drawn at the occupied width so
    /// the overlap is visible.
    static let channelWidthMhz = 22

    /// Occupied bandwidth of a channel of the given nominal width - the nominal
    /// figure plus the 2 MHz of shoulder that makes 20 MHz channels 22 MHz wide.
    static func occupiedWidth(nominalMhz: Int) -> Int {
        nominalMhz + 2
    }

    static func frequency(channel: Int, band: Band) -> Int {
        switch band {
        case .twoPointFour:
            return channel == 14 ? 2484 : 2407 + channel * 5
        case .five:
            return 5000 + channel * 5
        case .six:
            return channel == 2 ? 5935 : 5950 + channel * 5
        }
    }

    static func band(of channelBand: Int) -> Band? {
        switch channelBand {
        case 1: return .twoPointFour
        case 2: return .five
        case 3: return .six
        default: return nil
        }
    }

    /// Valid 20 MHz channel centres, in order.
    ///
    /// 5 GHz numbering leaves gaps: 36..144 step by 4 from a multiple of 4, then
    /// UNII-3 restarts at 149. 6 GHz is a clean 1, 5, 9 ... 233, plus the odd
    /// channel 2 at the bottom edge, which is left out because it overlaps
    /// channel 1 and would only crowd the axis.
    static func channels(in band: Band) -> [Int] {
        switch band {
        case .twoPointFour:
            return Array(1...14)
        case .five:
            return (36...177).filter { $0 <= 144 ? $0 % 4 == 0 : ($0 - 149) % 4 == 0 }
        case .six:
            return stride(from: 1, through: 233, by: 4).map { $0 }
        }
    }
}
