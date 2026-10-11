#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H


#include <String.h>


enum Band {
	kBand2400,
	kBand5000
};


/*!	One BSS from a scan.

	A record, not a live object: every scan produces a fresh vector, because the
	scan results the driver hands back are replaced wholesale by the next sweep.
*/
struct AccessPoint {
	/*! Network name, or "(hidden)" when the beacon carries none. */
	BString	ssid;

	/*! MAC of the radio, formatted xx:xx:xx:xx:xx:xx. Distinguishes the two
	    bands of one router. */
	BString	bssid;

	/*! Centre of the primary 20 MHz channel. */
	int32	frequencyMhz;

	/*! Centre of the whole occupied channel, which is not `frequencyMhz` when the
	    BSS is wider than 20 MHz. From the beacon's HT/VHT Operation element. */
	int32	centreMhz;

	/*! Channel width in MHz, from the same elements. 20 unless they say otherwise;
	    this is the width the curve is drawn at. */
	int32	widthMhz;

	/*! 0 when the frequency falls outside any band this app knows. */
	int32	channel;

	/*! Signal strength in dBm, typically -30 (very close) to -95. */
	float	rssi;

	Band	band;

	/*! "Open" or "Encrypted" - all the scan result carries. See AGENTS.md. */
	BString	security;
};


#endif	// ACCESS_POINT_H
