using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using WifiAnalyzer.Models;
using WifiAnalyzer.Services;

namespace WifiAnalyzer.Views
{
    public sealed partial class MainPage : Page
    {
        // 5 s is about as fast as a phone radio will produce a fresh scan; asking
        // more often just gets the same report back and costs battery.
        private static readonly TimeSpan ScanInterval = TimeSpan.FromSeconds(5);

        private readonly WifiScanner _scanner = new WifiScanner();
        private readonly DispatcherTimer _timer = new DispatcherTimer();

        private IReadOnlyList<AccessPoint> _points = new List<AccessPoint>();
        private Band _band = Band.TwoPointFour;

        // A scan can outlast the timer interval, so overlapping runs are suppressed
        // rather than queued.
        private bool _scanning;

        public MainPage()
        {
            InitializeComponent();

            _timer.Interval = ScanInterval;
            _timer.Tick += OnTimerTick;

            UpdateBandButtons();
            Loaded += OnLoaded;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            _timer.Stop();
            base.OnNavigatedFrom(e);
        }

        private async void OnLoaded(object sender, RoutedEventArgs e)
        {
            StatusText.Text = "Requesting Wi-Fi access...";

            // Must happen on the UI thread: the first call raises a consent prompt.
            if (!await _scanner.InitializeAsync())
            {
                StatusText.Text = _scanner.LastError;
                return;
            }

            await ScanAsync();
            _timer.Start();
        }

        private async void OnTimerTick(object sender, object e)
        {
            await ScanAsync();
        }

        private async void OnRefreshClick(object sender, RoutedEventArgs e)
        {
            await ScanAsync();
        }

        private async Task ScanAsync()
        {
            if (_scanning || !_scanner.IsReady) return;
            _scanning = true;
            try
            {
                _points = await _scanner.ScanAsync();
                Render();
                UpdateStatus();
            }
            catch (Exception error)
            {
                StatusText.Text = "Scan failed: " + error.Message;
                App.Log("ScanAsync failed: " + error);
            }
            finally
            {
                _scanning = false;
            }
        }

        private void OnGraphSizeChanged(object sender, SizeChangedEventArgs e)
        {
            // The Canvas has no size until the first layout pass, so the initial
            // render has to happen from here rather than from the constructor.
            Render();
        }

        private void Render()
        {
            SignalGraph.Render(GraphCanvas, _points, _band);
        }

        private void OnBand24Click(object sender, RoutedEventArgs e)
        {
            SelectBand(Band.TwoPointFour);
        }

        private void OnBand5Click(object sender, RoutedEventArgs e)
        {
            SelectBand(Band.Five);
        }

        private void SelectBand(Band band)
        {
            if (_band == band) return;
            _band = band;
            UpdateBandButtons();
            Render();
            UpdateStatus();
        }

        private void UpdateBandButtons()
        {
            // Dimming stands in for a toggle style: a phone-sized header has no room
            // for buttons with visible chrome.
            Band24Button.Opacity = _band == Band.TwoPointFour ? 1.0 : 0.4;
            Band5Button.Opacity = _band == Band.Five ? 1.0 : 0.4;
        }

        private void UpdateStatus()
        {
            var onBand = 0;
            foreach (var ap in _points)
            {
                if (ap.Band == _band) onBand++;
            }

            var text = onBand + " of " + _points.Count + " networks on " +
                       (_band == Band.TwoPointFour ? "2.4 GHz" : "5 GHz") +
                       "  -  " + DateTime.Now.ToString("HH:mm:ss");

            if (!string.IsNullOrEmpty(_scanner.LastError))
            {
                text += "  -  " + _scanner.LastError;
            }
            StatusText.Text = text;
        }
    }
}
