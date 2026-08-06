#include "WifiScanner.h"

#include <NetworkDevice.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>

#include <stdio.h>
#include <stdlib.h>

#include "ChannelMap.h"
#include "net80211.h"


// The kernel writes one variable-length record per network into this buffer.
// 64 KB is what Haiku's own net_server allocates for the same call.
static const size_t kResultBufferSize = 65535;


/*!	Channel width and the centre of the occupied channel, in MHz, read out of the
	beacon's own elements.

	Nothing in `ieee80211req_scan_result` reports either - see net80211.h - but the
	elements captured from the beacon travel with each result, and the HT and VHT
	Operation elements state both outright. The centre matters as much as the width:
	a 40 MHz BSS occupies its primary channel plus one extension channel to one
	side, so drawing it centred on the primary would put half of it over spectrum
	that is actually free.

	HT Operation's second octet holds the secondary channel offset in bits 0-1
	(1 = the extension is above the primary, 3 = below) and the STA channel width in
	bit 2. Both have to say 40 for it to be 40: a 20 MHz-only BSS on a 40 MHz-capable
	radio still advertises the element.

	VHT Operation names its centre directly, as a channel number in the 5 GHz plan.
*/
static void
_ChannelGeometry(const uint8* elements, int32 length, int32 primaryMhz,
	int32& widthMhz, int32& centreMhz)
{
	widthMhz = 20;
	centreMhz = primaryMhz;

	while (length >= 2) {
		uint8 id = elements[0];
		uint8 elementLength = elements[1];
		if (2 + elementLength > length)
			break;

		const uint8* data = elements + 2;

		if (id == IEEE80211_ELEM_HT_OPERATION && elementLength >= 2) {
			bool wide = (data[1] & 0x04) != 0;
			uint8 secondary = data[1] & 0x03;
			if (wide && secondary == 1) {
				widthMhz = 40;
				centreMhz = primaryMhz + 10;
			} else if (wide && secondary == 3) {
				widthMhz = 40;
				centreMhz = primaryMhz - 10;
			}
		} else if (id == IEEE80211_ELEM_VHT_OPERATION && elementLength >= 2) {
			// Width 0 means "whatever HT said". 2 and 3 are the deprecated
			// spellings of 160 and 80+80, which some APs still send.
			int32 vhtWidth = 0;
			switch (data[0]) {
				case 1:
					vhtWidth = 80;
					break;
				case 2:
				case 3:
					vhtWidth = 160;
					break;
			}

			if (vhtWidth != 0 && data[1] != 0) {
				widthMhz = vhtWidth;
				centreMhz = 5000 + data[1] * 5;
			}
		}

		elements += 2 + elementLength;
		length -= 2 + elementLength;
	}
}


WifiScanner::WifiScanner()
{
}


bool
WifiScanner::Initialize()
{
	uint32 cookie = 0;
	BNetworkInterface interface;
	while (BNetworkRoster::Default().GetNextInterface(&cookie, interface)
			== B_OK) {
		BNetworkDevice device(interface.Name());
		if (device.IsWireless()) {
			fDevice = interface.Name();
			fLastError = "";
			return true;
		}
	}

	fLastError = "No wireless interface found.";
	return false;
}


void
WifiScanner::TriggerScan()
{
	if (!IsReady())
		return;

	// wait = false: the scan takes a second or two and the window has to stay
	// responsive. forceRescan = false leaves the driver's cache in place, so a
	// refused or still-running scan costs nothing.
	BNetworkDevice device(fDevice.String());
	device.Scan(false, false);
}


status_t
WifiScanner::ReadNetworks(std::vector<AccessPoint>& networks)
{
	networks.clear();
	if (!IsReady())
		return B_NO_INIT;

	uint8* buffer = (uint8*)malloc(kResultBufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	int32 length = kResultBufferSize;
	status_t status = get_80211(fDevice.String(), IEEE80211_IOC_SCAN_RESULTS,
		buffer, length);
	if (status != B_OK) {
		free(buffer);
		fLastError.SetToFormat("Could not read scan results: %s",
			strerror(status));
		return status;
	}

	int32 bytesLeft = length;
	uint8* entry = buffer;

	while (bytesLeft > (int32)sizeof(struct ieee80211req_scan_result)) {
		ieee80211req_scan_result* result = (ieee80211req_scan_result*)entry;

		// isr_len is the record's stride and isr_ie_off is the kernel's
		// sizeof(ieee80211req_scan_result). If either disagrees with this build,
		// net80211.h no longer matches the kernel and every field after the
		// mismatch is garbage - stop rather than draw nonsense.
		if (result->isr_len < sizeof(struct ieee80211req_scan_result)
			|| result->isr_ie_off < sizeof(struct ieee80211req_scan_result)) {
			fLastError = "Scan result layout mismatch; see src/net80211.h.";
			break;
		}

		AccessPoint ap;

		if (result->isr_ssid_len > 0) {
			ap.ssid.SetTo((const char*)entry + result->isr_ie_off,
				result->isr_ssid_len);
		} else
			ap.ssid = "(hidden)";

		ap.bssid.SetToFormat("%02x:%02x:%02x:%02x:%02x:%02x",
			result->isr_bssid[0], result->isr_bssid[1], result->isr_bssid[2],
			result->isr_bssid[3], result->isr_bssid[4], result->isr_bssid[5]);

		ap.frequencyMhz = result->isr_freq;
		ap.channel = ChannelMap::ToChannel(result->isr_freq);
		ap.band = ChannelMap::BandOf(result->isr_freq);

		// net80211 reports signal strength in half-dBm above the noise floor.
		ap.rssi = result->isr_noise + result->isr_rssi / 2.0f;

		ap.security = (result->isr_capinfo & IEEE80211_CAPINFO_PRIVACY) != 0
			? "Encrypted" : "Open";

		_ChannelGeometry(entry + result->isr_ie_off + result->isr_ssid_len
				+ result->isr_meshid_len,
			result->isr_ie_len, ap.frequencyMhz, ap.widthMhz, ap.centreMhz);

		// One BSSID can appear twice when the driver kept two probe responses.
		// Keep the stronger reading.
		bool merged = false;
		for (size_t i = 0; i < networks.size(); i++) {
			if (networks[i].bssid != ap.bssid)
				continue;
			if (networks[i].rssi < ap.rssi)
				networks[i] = ap;
			merged = true;
			break;
		}
		if (!merged)
			networks.push_back(ap);

		entry += result->isr_len;
		bytesLeft -= result->isr_len;
	}

	free(buffer);
	fLastError = "";
	return B_OK;
}
