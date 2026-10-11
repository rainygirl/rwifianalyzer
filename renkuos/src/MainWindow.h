#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include <Window.h>

#include <vector>

#include "AccessPoint.h"
#include "WifiScanner.h"

class BButton;
class BMessageRunner;
class BRadioButton;
class BStringView;
class SignalGraph;


class MainWindow : public BWindow {
public:
								MainWindow();
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_Rescan();
			void				_SelectBand(Band band);
			void				_UpdateStatus();

private:
			SignalGraph*		fGraph;
			BStringView*		fStatus;
			BRadioButton*		fBand2400;
			BRadioButton*		fBand5000;
			BMessageRunner*		fRunner;

			WifiScanner			fScanner;
			std::vector<AccessPoint>	fPoints;
			Band				fBand;
};


#endif	// MAIN_WINDOW_H
