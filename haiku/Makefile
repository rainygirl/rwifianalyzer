## Haiku Makefile, driven by the system's makefile-engine.
##
##   make          build objects.*/RWiFiAnalyzer
##   make run      build and launch it
##   make clean
##
## Only the knobs the engine documents are set here; it supplies everything else.
##
## The build product has no space in its name, because makefile-engine puts $(NAME)
## straight into shell commands. Install it *as* "R WiFi Analyzer": on Haiku the file
## name is the name Tracker and Deskbar show. See README.md.

NAME := RWiFiAnalyzer
TYPE := APP

APP_MIME_SIG := application/x-vnd.RWiFiAnalyzer

SRCS := \
	src/App.cpp \
	src/MainWindow.cpp \
	src/SignalGraph.cpp \
	src/WifiScanner.cpp \
	src/ChannelMap.cpp

RDEFS := RWiFiAnalyzer.rdef

# be:      BApplication, BWindow, BView
# bnetapi: BNetworkRoster, BNetworkInterface, BNetworkDevice
# network: the socket the scan ioctl is issued on
#
# Recursively expanded, not `:=`: makefile-engine defines STDCPPLIBS below this
# line, so an immediate assignment would drop libstdc++ and the link would fail on
# std::vector's out-of-line throw helpers.
LIBS = be bnetapi network $(STDCPPLIBS)

WARNINGS := ALL

DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine

run: default
	$(TARGET) &
