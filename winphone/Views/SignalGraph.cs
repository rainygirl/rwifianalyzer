using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Text;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;
using WifiAnalyzer.Models;
using WifiAnalyzer.Services;

namespace WifiAnalyzer.Views
{
    /// <summary>
    /// Draws the channel-vs-strength graph by filling a Canvas with Shapes.
    ///
    /// Deliberately not a custom control and not Win2D: Win2D's UWP package needs a
    /// newer minimum than 10586, and a plain Canvas of Paths is enough for the ~30
    /// access points a phone ever sees. Everything is rebuilt on each scan, which is
    /// cheap at this element count and avoids any diffing logic.
    /// </summary>
    public static class SignalGraph
    {
        // Gutters for the axis labels. The plot area is what is left over.
        private const double LeftGutter = 34;
        private const double BottomGutter = 20;
        private const double TopPad = 18;
        private const double RightPad = 10;

        // dBm range of the vertical axis. -20 is closer than any real reading and
        // -100 is the noise floor, so every curve fits without clipping.
        private const double TopDbm = -20;
        private const double BottomDbm = -100;

        private static readonly Color[] Palette =
        {
            Color.FromArgb(255, 0xE6, 0x19, 0x4B), Color.FromArgb(255, 0x3C, 0xB4, 0x4B),
            Color.FromArgb(255, 0xFF, 0xE1, 0x19), Color.FromArgb(255, 0x43, 0x63, 0xD8),
            Color.FromArgb(255, 0xF5, 0x82, 0x31), Color.FromArgb(255, 0x91, 0x1E, 0xB4),
            Color.FromArgb(255, 0x46, 0xF0, 0xF0), Color.FromArgb(255, 0xF0, 0x32, 0xE6),
            Color.FromArgb(255, 0xBC, 0xF6, 0x0C), Color.FromArgb(255, 0xFA, 0xBE, 0xBE),
            Color.FromArgb(255, 0x2E, 0xC4, 0xB6), Color.FromArgb(255, 0xE6, 0xBE, 0xFF)
        };

        private static readonly SolidColorBrush GridBrush =
            new SolidColorBrush(Color.FromArgb(0x3C, 0xFF, 0xFF, 0xFF));
        private static readonly SolidColorBrush AxisTextBrush =
            new SolidColorBrush(Color.FromArgb(0xB4, 0xFF, 0xFF, 0xFF));

        public static void Render(Canvas canvas, IReadOnlyList<AccessPoint> points, Band band)
        {
            canvas.Children.Clear();

            var width = canvas.ActualWidth;
            var height = canvas.ActualHeight;
            if (width < 60 || height < 60) return; // not laid out yet

            // A Canvas does not clip its children, and a curve near the domain edge
            // runs past the plot area, so clip explicitly.
            canvas.Clip = new RectangleGeometry { Rect = new Rect(0, 0, width, height) };

            var plotLeft = LeftGutter;
            var plotRight = width - RightPad;
            var plotTop = TopPad;
            var plotBottom = height - BottomGutter;

            var visible = new List<AccessPoint>();
            foreach (var ap in points)
            {
                if (ap.Band == band && ap.Channel != 0) visible.Add(ap);
            }

            int minMhz, maxMhz;
            DomainFor(band, visible, out minMhz, out maxMhz);

            DrawDbmAxis(canvas, plotLeft, plotRight, plotTop, plotBottom);
            DrawChannelAxis(canvas, band, minMhz, maxMhz, plotLeft, plotRight, plotTop, plotBottom);

            if (visible.Count == 0)
            {
                AddText(canvas, "No networks on this band",
                        (plotLeft + plotRight) / 2 - 70, (plotTop + plotBottom) / 2 - 8,
                        12, AxisTextBrush.Color, false);
                return;
            }

            // Strongest first, so the tall curves end up behind the short ones and a
            // weak network is not hidden under a wide translucent neighbour.
            visible.Sort((a, b) => b.Rssi.CompareTo(a.Rssi));

            foreach (var ap in visible)
            {
                DrawNetwork(canvas, ap, minMhz, maxMhz, plotLeft, plotRight, plotTop, plotBottom);
            }
        }

        /// <summary>
        /// The 2.4 GHz axis is fixed - it is only 100 MHz wide and every phone shows
        /// the same 1..14. The 5 GHz plan spans 5170-5885, which at phone width would
        /// squeeze each channel into two pixels, so its axis is fitted to the
        /// networks actually present and widened to a 140 MHz minimum.
        /// </summary>
        private static void DomainFor(Band band, List<AccessPoint> visible, out int minMhz, out int maxMhz)
        {
            if (band == Band.TwoPointFour)
            {
                minMhz = 2400;
                maxMhz = 2500;
                return;
            }

            if (visible.Count == 0)
            {
                minMhz = 5170;
                maxMhz = 5330;
                return;
            }

            // Fitted to the edges of the occupied channels, not to their centres, so a
            // wide network at the end of the range is not half off the axis.
            var lo = int.MaxValue;
            var hi = int.MinValue;
            foreach (var ap in visible)
            {
                // Half a channel of margin past each edge, so the outermost curve
                // does not sit exactly on the axis.
                var half = ChannelMap.OccupiedWidth(ap.WidthMhz) / 2 + ChannelMap.ChannelWidthMhz / 2;
                if (ap.CentreMhz - half < lo) lo = ap.CentreMhz - half;
                if (ap.CentreMhz + half > hi) hi = ap.CentreMhz + half;
            }

            var shortfall = 140 - (hi - lo);
            if (shortfall > 0)
            {
                lo -= shortfall / 2;
                hi += shortfall - shortfall / 2;
            }

            minMhz = lo;
            maxMhz = hi;
        }

        private static void DrawDbmAxis(Canvas canvas, double left, double right, double top, double bottom)
        {
            for (var dbm = -30; dbm >= -90; dbm -= 10)
            {
                var y = ToY(dbm, top, bottom);
                AddLine(canvas, left, y, right, y);
                AddText(canvas, dbm.ToString(), 2, y - 8, 10, AxisTextBrush.Color, false);
            }

            // Plot border. The left and bottom edges double as the axes.
            AddLine(canvas, left, top, left, bottom);
            AddLine(canvas, left, bottom, right, bottom);
            AddLine(canvas, right, top, right, bottom);
            AddLine(canvas, left, top, right, top);
        }

        private static void DrawChannelAxis(Canvas canvas, Band band, int minMhz, int maxMhz,
                                            double left, double right, double top, double bottom)
        {
            var lastLabelX = double.NegativeInfinity;
            foreach (var channel in ChannelsIn(band, minMhz, maxMhz))
            {
                var x = ToX(ChannelMap.ToFrequency(channel, band), minMhz, maxMhz, left, right);
                if (x < left || x > right) continue;

                AddLine(canvas, x, top, x, bottom);

                // Drop labels that would collide. The gridline still marks the
                // channel, so the axis stays readable rather than becoming a smear.
                var label = channel.ToString();
                var minGap = 8 + label.Length * 7;
                if (x - lastLabelX < minGap) continue;

                AddText(canvas, label, x - label.Length * 3, bottom + 2, 10, AxisTextBrush.Color, false);
                lastLabelX = x;
            }
        }

        /// <summary>
        /// Valid 20 MHz channel centres. 5 GHz numbering leaves gaps - 36..64 and
        /// 100..144 step by 4 from a multiple of 4, then UNII-3 restarts at 149.
        /// </summary>
        private static IEnumerable<int> ChannelsIn(Band band, int minMhz, int maxMhz)
        {
            if (band == Band.TwoPointFour)
            {
                for (var channel = 1; channel <= 14; channel++) yield return channel;
                yield break;
            }

            for (var channel = 36; channel <= 177; channel++)
            {
                var valid = channel <= 144 ? channel % 4 == 0 : (channel - 149) % 4 == 0;
                if (!valid) continue;

                var mhz = ChannelMap.ToFrequency(channel, Band.Five);
                if (mhz >= minMhz && mhz <= maxMhz) yield return channel;
            }
        }

        private static void DrawNetwork(Canvas canvas, AccessPoint ap, int minMhz, int maxMhz,
                                        double left, double right, double top, double bottom)
        {
            var color = Palette[ColorIndex(ap.Bssid.Length > 0 ? ap.Bssid : ap.Ssid)];

            // The curve is as wide as the channel really is, centred on the centre of
            // the occupied channel rather than on the primary. On this OS that is
            // always 22 MHz on the primary, because nothing reports otherwise - the
            // arithmetic matches the other two ports so the drawing code does not
            // have to differ.
            var half = ChannelMap.OccupiedWidth(ap.WidthMhz) / 2.0;
            var xCentre = ToX(ap.CentreMhz, minMhz, maxMhz, left, right);
            var xLeft = ToX(ap.CentreMhz - half, minMhz, maxMhz, left, right);
            var xRight = ToX(ap.CentreMhz + half, minMhz, maxMhz, left, right);
            var yPeak = ToY(ap.Rssi, top, bottom);

            // Two mirrored cubic segments make the bell: flat at the baseline, flat
            // at the peak, steep in between. The 0.6 is the control-point offset as
            // a fraction of the half-width - lower looks spiky, higher looks square.
            var halfWidth = xCentre - xLeft;
            var figure = new PathFigure
            {
                StartPoint = new Point(xLeft, bottom),
                IsClosed = true,
                IsFilled = true
            };
            figure.Segments.Add(new BezierSegment
            {
                Point1 = new Point(xLeft + halfWidth * 0.6, bottom),
                Point2 = new Point(xCentre - halfWidth * 0.6, yPeak),
                Point3 = new Point(xCentre, yPeak)
            });
            figure.Segments.Add(new BezierSegment
            {
                Point1 = new Point(xCentre + halfWidth * 0.6, yPeak),
                Point2 = new Point(xRight - halfWidth * 0.6, bottom),
                Point3 = new Point(xRight, bottom)
            });

            var geometry = new PathGeometry();
            geometry.Figures.Add(figure);

            canvas.Children.Add(new Path
            {
                Data = geometry,
                Fill = new SolidColorBrush(Color.FromArgb(0x55, color.R, color.G, color.B)),
                Stroke = new SolidColorBrush(color),
                StrokeThickness = 1.5
            });

            AddLabel(canvas, ap.Ssid, xCentre, yPeak - 16, color, left, right);
        }

        private static void AddLabel(Canvas canvas, string text, double xCentre, double y,
                                     Color color, double left, double right)
        {
            var block = AddText(canvas, text, 0, y, 11, color, true);
            block.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));

            var x = xCentre - block.DesiredSize.Width / 2;
            if (x < left) x = left;
            if (x + block.DesiredSize.Width > right) x = right - block.DesiredSize.Width;
            Canvas.SetLeft(block, x);
        }

        private static TextBlock AddText(Canvas canvas, string text, double x, double y,
                                         double size, Color color, bool bold)
        {
            var block = new TextBlock
            {
                Text = text,
                FontSize = size,
                Foreground = new SolidColorBrush(color)
            };
            if (bold) block.FontWeight = FontWeights.SemiBold;

            Canvas.SetLeft(block, x);
            Canvas.SetTop(block, y);
            canvas.Children.Add(block);
            return block;
        }

        private static void AddLine(Canvas canvas, double x1, double y1, double x2, double y2)
        {
            canvas.Children.Add(new Line
            {
                X1 = x1,
                Y1 = y1,
                X2 = x2,
                Y2 = y2,
                Stroke = GridBrush,
                StrokeThickness = 1
            });
        }

        private static double ToX(double mhz, int minMhz, int maxMhz, double left, double right)
        {
            return left + (mhz - minMhz) / (maxMhz - minMhz) * (right - left);
        }

        private static double ToY(double dbm, double top, double bottom)
        {
            if (dbm > TopDbm) dbm = TopDbm;
            if (dbm < BottomDbm) dbm = BottomDbm;
            return top + (TopDbm - dbm) / (TopDbm - BottomDbm) * (bottom - top);
        }

        /// <summary>
        /// FNV-1a rather than string.GetHashCode: the .NET hash is randomised per
        /// process, which would repaint every network a different colour on each
        /// launch. A BSSID must keep its colour.
        /// </summary>
        private static int ColorIndex(string key)
        {
            unchecked
            {
                var hash = 2166136261u;
                foreach (var c in key)
                {
                    // The u suffixes keep the arithmetic in uint; without them C#
                    // widens to long and the assignment back to hash fails to compile.
                    hash = (hash ^ (uint)c) * 16777619u;
                }
                return (int)(hash % (uint)Palette.Length);
            }
        }
    }
}
