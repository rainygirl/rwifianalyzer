using WifiAnalyzer.Models;

namespace WifiAnalyzer.Services
{
    /// <summary>
    /// Frequency to channel-number arithmetic, in both directions.
    ///
    /// WiFiAvailableNetwork reports only a centre frequency; the channel number a
    /// user recognises has to be derived. The 2.4 GHz plan is 2412 MHz for channel
    /// 1 and 5 MHz per step, with channel 14 an exception at 2484. The 5 GHz plan
    /// is a flat 5000 + 5 * channel, which is why 5 GHz channel numbers jump in
    /// fours: only every fourth 5 MHz slot is a valid 20 MHz channel centre.
    /// </summary>
    public static class ChannelMap
    {
        /// <summary>
        /// A 20 MHz channel actually occupies ~22 MHz of spectrum, which is why
        /// adjacent 2.4 GHz channels overlap. Curves are drawn at the occupied width
        /// so the overlap is visible, matching what the Android analyzers show.
        /// </summary>
        public const int ChannelWidthMhz = 22;

        /// <summary>
        /// Occupied bandwidth of a channel of the given nominal width - the nominal
        /// figure plus the 2 MHz of shoulder that makes 20 MHz channels 22 MHz wide.
        /// </summary>
        public static int OccupiedWidth(int nominalMhz)
        {
            return nominalMhz + 2;
        }

        public static int ToChannel(int mhz)
        {
            if (mhz == 2484) return 14;
            if (mhz >= 2412 && mhz <= 2472) return (mhz - 2407) / 5;
            if (mhz >= 5160 && mhz <= 5885) return (mhz - 5000) / 5;
            return 0;
        }

        public static int ToFrequency(int channel, Band band)
        {
            if (band == Band.TwoPointFour)
            {
                return channel == 14 ? 2484 : 2407 + channel * 5;
            }
            return 5000 + channel * 5;
        }

        /// <summary>
        /// Anything below 2500 MHz is treated as 2.4 GHz. The 4.9 GHz public-safety
        /// band exists but no consumer AP uses it, so it is folded into 5 GHz rather
        /// than given a band of its own.
        /// </summary>
        public static Band BandOf(int mhz)
        {
            return mhz < 2500 ? Band.TwoPointFour : Band.Five;
        }
    }
}
