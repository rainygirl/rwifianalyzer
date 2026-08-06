#ifndef NET80211_H
#define NET80211_H


/*!	The smallest possible copy of Haiku's private net80211 ioctl interface.

	BNetworkDevice::GetNetworks() would give us names, signal strength and
	security in three lines - but it throws the channel away. `wireless_network`
	has no frequency field, while the `ieee80211req_scan_result` it is built from
	does. A channel graph cannot be drawn without it, so the scan results are read
	straight from the ioctl instead.

	The definitions below are copied from Haiku's
	src/libs/compat/freebsd_wlan/net80211/ieee80211_ioctl.h, which lives in the
	source tree rather than in the installed development headers and so cannot be
	included. They are FreeBSD's, and stable: Haiku's kernel side fills exactly
	this layout.

	Two things are load-bearing:

	1. `IFNAMSIZ` in the kernel's copy of `ieee80211req` resolves to Haiku's
	   `IF_NAMESIZE`, which is 32, not FreeBSD's 16. `<net/if.h>` below is the same
	   header the kernel sees, so i_name has the same size on both sides.
	2. `SIOCG80211` is `_IOWR('i', 235, ...)`, and Haiku's FreeBSD compatibility
	   layer defines `_IOWR(g, n, t)` as `SIOCEND + n`. Hence SIOCEND + 235.
*/


#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <SupportDefs.h>


#define IEEE80211_ADDR_LEN			6
#define IEEE80211_RATE_MAXSIZE		15

#define IEEE80211_IOC_SCAN_RESULTS	76

#define SIOCG80211					(SIOCEND + 235)

/*! Bit 4 of the capability information field: the BSS uses encryption. */
#define IEEE80211_CAPINFO_PRIVACY	0x10

/*!	Information element IDs, from 802.11 itself rather than from net80211.

	`isr_flags` cannot answer the channel-width question: it is a `uint16` copy of
	net80211's `ic_flags`, and every HT flag (`IEEE80211_CHAN_HT40U` and friends)
	sits above bit 16, so they are all truncated away. The beacon's own elements
	come along with each scan result, though, and they say it exactly.
*/
#define IEEE80211_ELEM_HT_OPERATION		61
#define IEEE80211_ELEM_VHT_OPERATION	192


struct ieee80211req {
	char		i_name[IF_NAMESIZE];
	uint16		i_type;
	int16		i_val;
	uint16		i_len;
	void*		i_data;
};


struct ieee80211req_scan_result {
	uint16		isr_len;		/* total length, a multiple of 4 */
	uint16		isr_ie_off;		/* offset to SSID + mesh id + IE data */
	uint16		isr_ie_len;
	uint16		isr_freq;		/* MHz - the reason this file exists */
	uint16		isr_flags;		/* channel flags */
	int8		isr_noise;		/* dBm */
	int8		isr_rssi;		/* .5 dBm units above isr_noise */
	uint16		isr_intval;		/* beacon interval */
	uint8		isr_capinfo;
	uint8		isr_erp;
	uint8		isr_bssid[IEEE80211_ADDR_LEN];
	uint8		isr_nrates;
	uint8		isr_rates[IEEE80211_RATE_MAXSIZE];
	uint8		isr_ssid_len;
	uint8		isr_meshid_len;
	/* variable-length SSID, mesh id and IE data follow, at isr_ie_off */
};


/*!	Reads one net80211 property. `length` is the buffer size going in and the
	number of bytes written coming out.
*/
static inline status_t
get_80211(const char* device, int32 type, void* data, int32& length)
{
	int socketHandle = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketHandle < 0)
		return errno;

	struct ieee80211req request;
	memset(&request, 0, sizeof(request));
	strlcpy(request.i_name, device, IF_NAMESIZE);
	request.i_type = type;
	request.i_len = length;
	request.i_data = data;

	status_t status = B_OK;
	if (ioctl(socketHandle, SIOCG80211, &request, sizeof(request)) < 0)
		status = errno;
	else
		length = request.i_len;

	close(socketHandle);
	return status;
}


#endif	// NET80211_H
