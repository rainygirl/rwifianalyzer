# SwiftPM builds the binary; this builds the .app bundle around it. There is no
# Xcode project, because the only thing Xcode would add here is the bundle.
#
#   make                       build "R WiFi Analyzer.app" for this Mac
#   make ARCHS="arm64 x86_64"  build a universal binary
#   make run                   build and launch it
#   make install               copy it to /Applications
#   make SIGN="Developer ID Application: ..."   sign with a real identity

# The bundle name is what Finder and the Dock show, spaces and all. Every rule
# quotes it for that reason.
APP     := R WiFi Analyzer.app
BINARY  := WifiAnalyzer
CONFIG  := release

# Ad-hoc by default. An ad-hoc signature changes on every rebuild, so macOS treats
# each build as a new app and asks for Location access again; a Developer ID
# identity is what makes that grant stick.
SIGN    ?= -

ARCHS       ?=
SWIFT_FLAGS := $(foreach arch,$(ARCHS),--arch $(arch))
BIN_PATH    := $(shell swift build -c $(CONFIG) $(SWIFT_FLAGS) --show-bin-path)

.PHONY: all bundle run install clean

all: bundle

bundle:
	swift build -c $(CONFIG) $(SWIFT_FLAGS)
	rm -rf "$(APP)"
	mkdir -p "$(APP)/Contents/MacOS" "$(APP)/Contents/Resources"
	cp $(BIN_PATH)/$(BINARY) "$(APP)/Contents/MacOS/$(BINARY)"
	cp Resources/Info.plist "$(APP)/Contents/Info.plist"
	cp Resources/AppIcon.icns "$(APP)/Contents/Resources/AppIcon.icns"
	codesign --force --sign "$(SIGN)" "$(APP)"

# Launched with `open` rather than by running the binary directly: location
# authorization is granted to a bundle, not to a loose executable.
run: bundle
	open "./$(APP)"

install: bundle
	rm -rf "/Applications/$(APP)"
	cp -R "$(APP)" /Applications/
	@echo "installed /Applications/$(APP)"

clean:
	rm -rf .build "$(APP)"
