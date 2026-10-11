#include "ChannelMap.h"


namespace ChannelMap {


int32
OccupiedWidth(int32 nominalMhz)
{
	return nominalMhz + 2;
}


int32
ToChannel(int32 mhz)
{
	if (mhz == 2484)
		return 14;
	if (mhz >= 2412 && mhz <= 2472)
		return (mhz - 2407) / 5;
	if (mhz >= 5160 && mhz <= 5885)
		return (mhz - 5000) / 5;

	return 0;
}


int32
ToFrequency(int32 channel, Band band)
{
	if (band == kBand2400)
		return channel == 14 ? 2484 : 2407 + channel * 5;

	return 5000 + channel * 5;
}


/*!	Anything below 2500 MHz is treated as 2.4 GHz. The 4.9 GHz public-safety band
	exists but no consumer AP uses it, so it is folded into 5 GHz rather than given
	a band of its own.
*/
Band
BandOf(int32 mhz)
{
	return mhz < 2500 ? kBand2400 : kBand5000;
}


}	// namespace ChannelMap
