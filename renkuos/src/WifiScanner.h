#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H


#include <String.h>
#include <SupportDefs.h>

#include <vector>

#include "AccessPoint.h"


/*!	Finds a wireless interface, asks it to scan, and reads the results.

	Deliberately split in two: TriggerScan() only asks, ReadNetworks() only reads.
	A blocking scan would freeze the window for the second or two the radio spends
	sweeping channels, so nothing here waits - each tick starts a scan and reads
	whatever the previous one left behind. See AGENTS.md.
*/
class WifiScanner {
public:
								WifiScanner();

	/*! Picks the first wireless interface. False when there is none. */
			bool				Initialize();

			bool				IsReady() const { return fDevice.Length() > 0; }

	/*! Asks the driver for a fresh sweep and returns immediately. */
			void				TriggerScan();

			status_t			ReadNetworks(std::vector<AccessPoint>& networks);

	/*! Human-readable reason the last call failed, or an empty string. */
			const char*			LastError() const { return fLastError.String(); }

			const char*			DeviceName() const { return fDevice.String(); }

private:
			BString				fDevice;
			BString				fLastError;
};


#endif	// WIFI_SCANNER_H
