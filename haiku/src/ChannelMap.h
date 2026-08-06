#ifndef CHANNEL_MAP_H
#define CHANNEL_MAP_H


#include <SupportDefs.h>

#include "AccessPoint.h"


/*!	Frequency to channel-number arithmetic, in both directions.

	The scan result reports only a centre frequency; the channel number a user
	recognises has to be derived. The 2.4 GHz plan is 2412 MHz for channel 1 and
	5 MHz per step, with channel 14 an exception at 2484. The 5 GHz plan is a flat
	5000 + 5 * channel, which is why 5 GHz channel numbers jump in fours: only
	every fourth 5 MHz slot is a valid 20 MHz channel centre.
*/
namespace ChannelMap {

/*!	A 20 MHz channel actually occupies ~22 MHz of spectrum, which is why adjacent
	2.4 GHz channels overlap. Curves are drawn at the occupied width so the overlap
	is visible, matching what the Android analyzers show.
*/
static const int32 kChannelWidthMhz = 22;

/*!	Occupied bandwidth of a channel of the given nominal width - the nominal figure
	plus the 2 MHz of shoulder that makes 20 MHz channels 22 MHz wide.
*/
int32	OccupiedWidth(int32 nominalMhz);

int32	ToChannel(int32 mhz);
int32	ToFrequency(int32 channel, Band band);
Band	BandOf(int32 mhz);

}	// namespace ChannelMap


#endif	// CHANNEL_MAP_H
