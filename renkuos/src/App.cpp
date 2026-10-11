#include <Application.h>

#include "MainWindow.h"


static const char* kSignature = "application/x-vnd.RWiFiAnalyzer";


class WifiAnalyzerApp : public BApplication {
public:
	WifiAnalyzerApp()
		:
		BApplication(kSignature)
	{
	}

	virtual void ReadyToRun()
	{
		(new MainWindow())->Show();
	}
};


int
main()
{
	WifiAnalyzerApp app;
	app.Run();
	return 0;
}
