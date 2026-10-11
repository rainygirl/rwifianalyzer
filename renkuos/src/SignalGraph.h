#ifndef SIGNAL_GRAPH_H
#define SIGNAL_GRAPH_H


#include <View.h>

#include <vector>

#include "AccessPoint.h"


/*!	Draws the channel-vs-strength graph.

	A plain BView that redraws from the current list on every scan. At the few
	dozen networks a machine can see there is nothing to gain from keeping
	per-network objects around, and BShape gives us the bell curves directly.
*/
class SignalGraph : public BView {
public:
								SignalGraph();

			void				SetData(const std::vector<AccessPoint>& points,
									Band band);

	virtual	void				Draw(BRect updateRect);

private:
			void				_DrawDbmAxis(BRect plot);
			void				_DrawChannelAxis(BRect plot, int32 minMhz,
									int32 maxMhz);
			BPoint				_DrawCurve(const AccessPoint& ap, BRect plot,
									int32 minMhz, int32 maxMhz);
			void				_DrawLabel(const AccessPoint& ap, BPoint anchor,
									BRect plot, std::vector<BRect>& placed);
			void				_DomainFor(
									const std::vector<AccessPoint>& visible,
									int32& minMhz, int32& maxMhz);

			float				_ToX(float mhz, int32 minMhz, int32 maxMhz,
									BRect plot) const;
			float				_ToY(float dbm, BRect plot) const;

private:
			std::vector<AccessPoint>	fPoints;
			Band						fBand;
};


#endif	// SIGNAL_GRAPH_H
