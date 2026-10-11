#include "MainWindow.h"

#include <Alignment.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <RadioButton.h>
#include <StringView.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "SignalGraph.h"


static const uint32 kMsgTick = 'tick';
static const uint32 kMsgRescan = 'rscn';
static const uint32 kMsgBand2400 = 'bn24';
static const uint32 kMsgBand5000 = 'bn50';

// 5 s is about as fast as a radio will produce a fresh sweep; asking more often
// gets the same cache back and costs airtime, since scanning makes the card leave
// its channel.
static const bigtime_t kScanInterval = 5000000;


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 999, 659), "R WiFi Analyzer", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE),
	fRunner(NULL),
	fBand(kBand2400)
{
	fGraph = new SignalGraph();
	fStatus = new BStringView("status", "Scanning...");

	// The window title already says what the app is, so the top row carries only
	// the controls. The two bands are separate graphs rather than one axis,
	// because they are 600 MHz apart and would leave most of a shared axis empty.
	fBand2400 = new BRadioButton("2.4 GHz", new BMessage(kMsgBand2400));
	fBand5000 = new BRadioButton("5 GHz", new BMessage(kMsgBand5000));
	fBand2400->SetValue(B_CONTROL_ON);

	// A radio button is shorter than the button beside it, and the layout would
	// otherwise pin it to the top of the row, leaving it and its label sitting
	// above the centre line.
	BAlignment centred(B_ALIGN_HORIZONTAL_UNSET, B_ALIGN_VERTICAL_CENTER);
	fBand2400->SetExplicitAlignment(centred);
	fBand5000->SetExplicitAlignment(centred);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
			B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fBand2400)
			.Add(fBand5000)
			.AddGlue()
			.Add(new BButton("Rescan", new BMessage(kMsgRescan)))
		.End()
		.Add(fGraph)
		.Add(fStatus)
	.End();

	if (!fScanner.Initialize()) {
		fStatus->SetText(fScanner.LastError());
		return;
	}

	fScanner.TriggerScan();

	BMessage tick(kMsgTick);
	fRunner = new BMessageRunner(BMessenger(this), &tick, kScanInterval);
}


MainWindow::~MainWindow()
{
	delete fRunner;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgTick:
		case kMsgRescan:
			_Rescan();
			break;

		case kMsgBand2400:
			_SelectBand(kBand2400);
			break;

		case kMsgBand5000:
			_SelectBand(kBand5000);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
MainWindow::_Rescan()
{
	if (!fScanner.IsReady())
		return;

	// Read first, then ask for the next sweep: the results on hand are the ones
	// the previous trigger produced. Waiting for a scan to finish would block the
	// window for a second or two every tick.
	fScanner.ReadNetworks(fPoints);
	fGraph->SetData(fPoints, fBand);
	_UpdateStatus();

	fScanner.TriggerScan();
}


void
MainWindow::_SelectBand(Band band)
{
	if (fBand == band)
		return;

	fBand = band;
	fGraph->SetData(fPoints, fBand);
	_UpdateStatus();
}


void
MainWindow::_UpdateStatus()
{
	size_t onBand = 0;
	for (size_t i = 0; i < fPoints.size(); i++) {
		if (fPoints[i].band == fBand)
			onBand++;
	}

	char clock[16];
	time_t now = time(NULL);
	struct tm local;
	localtime_r(&now, &local);
	strftime(clock, sizeof(clock), "%H:%M:%S", &local);

	char text[512];
	snprintf(text, sizeof(text), "%s  -  %zu of %zu networks on %s  -  %s%s%s",
		fScanner.DeviceName(), onBand, fPoints.size(),
		fBand == kBand2400 ? "2.4 GHz" : "5 GHz", clock,
		strlen(fScanner.LastError()) > 0 ? "  -  " : "",
		fScanner.LastError());

	fStatus->SetText(text);
}
