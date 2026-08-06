#include "SignalGraph.h"

#include <Region.h>
#include <String.h>

#include <algorithm>
#include <stdio.h>

#include "ChannelMap.h"


// Gutters for the axis labels. The plot area is what is left over.
static const float kLeftGutter = 40;
static const float kBottomGutter = 24;
static const float kTopPad = 20;
static const float kRightPad = 12;

// dBm range of the vertical axis. -20 is closer than any real reading and -100
// is the noise floor, so every curve fits without clipping.
static const float kTopDbm = -20;
static const float kBottomDbm = -100;

static const float kAxisFontSize = 10;
static const float kLabelFontSize = 11;

static const rgb_color kBackground = { 0x0a, 0x0a, 0x0a, 255 };

// Opaque on purpose. The grid is drawn in B_OP_COPY, which ignores the alpha
// channel, so a translucent white would come out as solid white.
static const rgb_color kGrid = { 0x3a, 0x3a, 0x3a, 255 };
static const rgb_color kAxisText = { 0xb4, 0xb4, 0xb4, 255 };

static const rgb_color kPalette[] = {
	{ 0xe6, 0x19, 0x4b, 255 }, { 0x3c, 0xb4, 0x4b, 255 },
	{ 0xff, 0xe1, 0x19, 255 }, { 0x43, 0x63, 0xd8, 255 },
	{ 0xf5, 0x82, 0x31, 255 }, { 0x91, 0x1e, 0xb4, 255 },
	{ 0x46, 0xf0, 0xf0, 255 }, { 0xf0, 0x32, 0xe6, 255 },
	{ 0xbc, 0xf6, 0x0c, 255 }, { 0xfa, 0xbe, 0xbe, 255 },
	{ 0x2e, 0xc4, 0xb6, 255 }, { 0xe6, 0xbe, 0xff, 255 }
};
static const int32 kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);


// Points per cubic segment. Twelve is past the point where the facets show at any
// window size this app is used at.
static const int32 kCurveSteps = 12;


static rgb_color
with_alpha(rgb_color color, uint8 alpha)
{
	color.alpha = alpha;
	return color;
}


/*!	Appends `kCurveSteps` points along a cubic Bezier, excluding its start. */
static void
_Bezier(BPoint* points, int32& count, BPoint start, BPoint control1,
	BPoint control2, BPoint end)
{
	for (int32 step = 1; step <= kCurveSteps; step++) {
		float t = (float)step / kCurveSteps;
		float u = 1 - t;

		float a = u * u * u;
		float b = 3 * u * u * t;
		float c = 3 * u * t * t;
		float d = t * t * t;

		points[count++] = BPoint(
			a * start.x + b * control1.x + c * control2.x + d * end.x,
			a * start.y + b * control1.y + c * control2.y + d * end.y);
	}
}


/*!	FNV-1a over the BSSID.

	Any hash would do as long as it is the same one every launch, which rules out
	anything seeded per process: a network changing colour on each start makes the
	graph unreadable across scans.
*/
static int32
color_index(const BString& key)
{
	uint32 hash = 2166136261u;
	for (int32 i = 0; i < key.Length(); i++)
		hash = (hash ^ (uint8)key.ByteAt(i)) * 16777619u;

	return hash % (uint32)kPaletteSize;
}


static bool
stronger_first(const AccessPoint& a, const AccessPoint& b)
{
	return a.rssi > b.rssi;
}


SignalGraph::SignalGraph()
	:
	BView("graph", B_WILL_DRAW | B_FRAME_EVENTS | B_SUBPIXEL_PRECISE),
	fBand(kBand2400)
{
	SetViewColor(kBackground);
	SetLowColor(kBackground);
	SetExplicitMinSize(BSize(480, 260));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
}


void
SignalGraph::SetData(const std::vector<AccessPoint>& points, Band band)
{
	fPoints = points;
	fBand = band;
	Invalidate();
}


void
SignalGraph::Draw(BRect updateRect)
{
	BRect bounds = Bounds();
	if (bounds.Width() < 120 || bounds.Height() < 100)
		return;

	BRect plot(bounds.left + kLeftGutter, bounds.top + kTopPad,
		bounds.right - kRightPad, bounds.bottom - kBottomGutter);

	std::vector<AccessPoint> visible;
	for (size_t i = 0; i < fPoints.size(); i++) {
		if (fPoints[i].band == fBand && fPoints[i].channel != 0)
			visible.push_back(fPoints[i]);
	}

	int32 minMhz, maxMhz;
	_DomainFor(visible, minMhz, maxMhz);

	_DrawDbmAxis(plot);
	_DrawChannelAxis(plot, minMhz, maxMhz);

	if (visible.empty()) {
		SetFontSize(kLabelFontSize);
		SetHighColor(kAxisText);
		const char* message = "No networks on this band";
		DrawString(message, BPoint(plot.left
				+ (plot.Width() - StringWidth(message)) / 2,
			plot.top + plot.Height() / 2));
		return;
	}

	// Strongest first, so the tall wide curves end up behind the short ones and a
	// weak network is not hidden under a translucent neighbour. Labels are placed
	// in the same order, so the strongest network keeps the spot it wants.
	std::sort(visible.begin(), visible.end(), stronger_first);

	std::vector<BRect> placed;
	for (size_t i = 0; i < visible.size(); i++) {
		BPoint anchor = _DrawCurve(visible[i], plot, minMhz, maxMhz);
		_DrawLabel(visible[i], anchor, plot, placed);
	}
}


/*!	The 2.4 GHz axis is fixed - it is only 100 MHz wide and channels 1..14 all fit
	with room to label them. The 5 GHz plan spans 5170-5885, so its axis is fitted
	to the networks actually present and widened to a 200 MHz minimum; at the full
	span a 22 MHz curve would be a sliver. The axis moves when the set of channels
	in range changes, which is rare in practice.
*/
void
SignalGraph::_DomainFor(const std::vector<AccessPoint>& visible, int32& minMhz,
	int32& maxMhz)
{
	if (fBand == kBand2400) {
		minMhz = 2400;
		maxMhz = 2500;
		return;
	}

	if (visible.empty()) {
		minMhz = 5170;
		maxMhz = 5370;
		return;
	}

	// Fitted to the edges of the occupied channels, not to their centres, so an
	// 80 MHz BSS at the end of the range is not half off the axis.
	int32 low = visible[0].centreMhz;
	int32 high = low;
	for (size_t i = 0; i < visible.size(); i++) {
		// Half a channel of margin past each edge, so the outermost curve does not
		// sit exactly on the axis.
		int32 half = ChannelMap::OccupiedWidth(visible[i].widthMhz) / 2
			+ ChannelMap::kChannelWidthMhz / 2;
		low = std::min(low, visible[i].centreMhz - half);
		high = std::max(high, visible[i].centreMhz + half);
	}

	int32 shortfall = 200 - (high - low);
	if (shortfall > 0) {
		low -= shortfall / 2;
		high += shortfall - shortfall / 2;
	}

	minMhz = low;
	maxMhz = high;
}


void
SignalGraph::_DrawDbmAxis(BRect plot)
{
	SetFontSize(kAxisFontSize);
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	for (int32 dbm = -30; dbm >= -90; dbm -= 10) {
		float y = _ToY(dbm, plot);

		SetHighColor(kGrid);
		StrokeLine(BPoint(plot.left, y), BPoint(plot.right, y));

		BString label;
		label << dbm;
		SetHighColor(kAxisText);
		DrawString(label.String(), BPoint(plot.left - 6
				- StringWidth(label.String()),
			y + fontHeight.ascent / 2 - 1));
	}

	// Plot border. The left and bottom edges double as the axes.
	SetHighColor(kGrid);
	StrokeRect(plot);
}


void
SignalGraph::_DrawChannelAxis(BRect plot, int32 minMhz, int32 maxMhz)
{
	SetFontSize(kAxisFontSize);
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	// Valid 20 MHz channel centres. 5 GHz numbering leaves gaps: 36..144 step by
	// 4 from a multiple of 4, then UNII-3 restarts at 149.
	int32 first = fBand == kBand2400 ? 1 : 36;
	int32 last = fBand == kBand2400 ? 14 : 177;
	float lastLabelRight = -1e6;

	for (int32 channel = first; channel <= last; channel++) {
		if (fBand == kBand5000) {
			bool valid = channel <= 144
				? channel % 4 == 0 : (channel - 149) % 4 == 0;
			if (!valid)
				continue;
		}

		int32 mhz = ChannelMap::ToFrequency(channel, fBand);
		if (mhz < minMhz || mhz > maxMhz)
			continue;

		float x = _ToX(mhz, minMhz, maxMhz, plot);
		if (x < plot.left || x > plot.right)
			continue;

		SetHighColor(kGrid);
		StrokeLine(BPoint(x, plot.top), BPoint(x, plot.bottom));

		// Drop labels that would collide. The gridline still marks the channel,
		// so a crowded axis stays readable rather than becoming a smear.
		BString label;
		label << channel;
		float width = StringWidth(label.String());
		if (x - width / 2 - 6 < lastLabelRight)
			continue;

		SetHighColor(kAxisText);
		DrawString(label.String(), BPoint(x - width / 2,
			plot.bottom + 4 + fontHeight.ascent));
		lastLabelRight = x + width / 2;
	}
}


/*!	Draws one bell curve and returns the point its label wants to sit above. */
BPoint
SignalGraph::_DrawCurve(const AccessPoint& ap, BRect plot, int32 minMhz,
	int32 maxMhz)
{
	rgb_color color = kPalette[color_index(ap.bssid.Length() > 0
		? ap.bssid : ap.ssid)];

	// The curve is as wide as the channel really is - 22 MHz for a 20 MHz channel,
	// 42 for a 40 - and sits on the centre of the occupied channel rather than on
	// the primary. That is what makes a 40 MHz BSS visibly cover its neighbour.
	float half = ChannelMap::OccupiedWidth(ap.widthMhz) / 2.0f;
	float xCentre = _ToX(ap.centreMhz, minMhz, maxMhz, plot);
	float xLeft = _ToX(ap.centreMhz - half, minMhz, maxMhz, plot);
	float xRight = _ToX(ap.centreMhz + half, minMhz, maxMhz, plot);
	float yPeak = _ToY(ap.rssi, plot);

	// Two mirrored cubic segments make the bell: flat at the baseline, flat at the
	// peak, steep in between. The 0.6 is the control-point offset as a fraction of
	// the half-width - lower looks spiky, higher looks square.
	//
	// Sampled into a polygon rather than handed to BShape: FillShape draws nothing
	// here, and a polygon is one call with no drawing-mode conditions attached.
	float halfWidth = xCentre - xLeft;

	BPoint points[2 * kCurveSteps + 1];
	int32 count = 0;
	points[count++] = BPoint(xLeft, plot.bottom);

	_Bezier(points, count, BPoint(xLeft, plot.bottom),
		BPoint(xLeft + halfWidth * 0.6f, plot.bottom),
		BPoint(xCentre - halfWidth * 0.6f, yPeak),
		BPoint(xCentre, yPeak));
	_Bezier(points, count, BPoint(xCentre, yPeak),
		BPoint(xCentre + halfWidth * 0.6f, yPeak),
		BPoint(xRight - halfWidth * 0.6f, plot.bottom),
		BPoint(xRight, plot.bottom));

	// A curve at the edge of the domain runs past the plot area, so the fill and
	// stroke are clipped - but the label is not, because it sits above the peak
	// and would be cut off at the top of the plot.
	BRegion clip(plot);
	ConstrainClippingRegion(&clip);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
	SetHighColor(with_alpha(color, 0x55));
	FillPolygon(points, count);

	SetDrawingMode(B_OP_COPY);
	SetHighColor(color);
	SetPenSize(1.5);
	StrokePolygon(points, count, true);
	SetPenSize(1);

	ConstrainClippingRegion(NULL);

	return BPoint(xCentre, yPeak);
}


/*!	Places a label above its peak, nudging it up until it clears the labels
	already placed. A desktop window shows every network in range at once, and a
	dozen names all drawn at their own peak height overlap into a smear.
*/
void
SignalGraph::_DrawLabel(const AccessPoint& ap, BPoint anchor, BRect plot,
	std::vector<BRect>& placed)
{
	SetFont(be_bold_font);
	SetFontSize(kLabelFontSize);
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	// Just the name. The channel width is in the width of the curve, which is where
	// it belongs - a number in the label says nothing about what it overlaps.
	float height = fontHeight.ascent + fontHeight.descent;
	float width = StringWidth(ap.ssid.String());

	float x = anchor.x - width / 2;
	x = std::max(x, plot.left);
	x = std::min(x, plot.right - width);

	float bottom = anchor.y - 4;

	// Six steps is enough to unstack a busy channel; past that the graph is too
	// crowded for labels to help anyway and one is left overlapping.
	for (int32 attempt = 0; attempt < 6; attempt++) {
		BRect rect(x, bottom - height, x + width, bottom);
		bool clear = true;
		for (size_t i = 0; i < placed.size(); i++) {
			if (placed[i].Intersects(rect)) {
				clear = false;
				break;
			}
		}
		if (clear || rect.top < Bounds().top + 2)
			break;
		bottom -= height;
	}

	bottom = std::max(bottom, Bounds().top + height + 2);
	placed.push_back(BRect(x, bottom - height, x + width, bottom));

	SetHighColor(kPalette[color_index(ap.bssid.Length() > 0
		? ap.bssid : ap.ssid)]);
	DrawString(ap.ssid.String(), BPoint(x, bottom - fontHeight.descent));

	SetFont(be_plain_font);
}


float
SignalGraph::_ToX(float mhz, int32 minMhz, int32 maxMhz, BRect plot) const
{
	return plot.left + (mhz - minMhz) / (maxMhz - minMhz) * plot.Width();
}


float
SignalGraph::_ToY(float dbm, BRect plot) const
{
	if (dbm > kTopDbm)
		dbm = kTopDbm;
	if (dbm < kBottomDbm)
		dbm = kBottomDbm;

	return plot.top + (kTopDbm - dbm) / (kTopDbm - kBottomDbm) * plot.Height();
}
