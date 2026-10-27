QT += core gui widgets svg network
CONFIG += c++17 release
CONFIG -= debug debug_and_release
TEMPLATE = app
TARGET = Azec
DESTDIR = $$OUT_PWD/release
SOURCES += src/app.cpp src/phone_bridge.cpp src/profile_transfer.cpp src/engine.cpp src/core.cpp src/window_backdrop.cpp third_party/qrcodegen/qrcodegen.cpp
HEADERS += src/mono_controls.hpp src/engine.hpp src/core.hpp src/ui_widgets.hpp src/profile_store.hpp src/phone_server.hpp src/main_window.hpp src/app_style.hpp src/theme.hpp src/startup.hpp src/diagnostics.hpp src/pairing_qr.hpp src/hotkeys.hpp src/game_focus.hpp src/sound.hpp
HEADERS += src/version.hpp src/update_check.hpp src/strings.hpp src/flags.hpp src/lang_en.hpp src/lang_pt.hpp src/lang_fr.hpp src/lang_es.hpp src/lang_de.hpp src/lang_ar.hpp src/lang_he.hpp src/lang_zh.hpp src/lang_ja.hpp src/profile_transfer.hpp third_party/qrcodegen/qrcodegen.hpp
contains(CONFIG, verification) {
    TARGET = Azec-checks
    DEFINES += AZEC_UI_TESTS
    SOURCES += tests/ui_checks.cpp
}
RESOURCES += assets/operators.qrc
RC_ICONS = assets/app.ico
DEFINES += NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS
HEADERS += src/lan_address.hpp
LIBS += -luser32 -lgdi32 -ldwmapi -liphlpapi -lws2_32 -lwinmm -lshell32 -ladvapi32 -lwindowsapp -lCoreMessaging
QMAKE_CXXFLAGS += /utf-8
QMAKE_LFLAGS += /MANIFEST:EMBED

HEADERS += src/ui_preferences.hpp src/desktop_layout.hpp src/app_paths.hpp
HEADERS += src/sensitivity.hpp src/driver_input.hpp
HEADERS += src/driver_setup.hpp
HEADERS += src/driver_trust.hpp
HEADERS += src/driver_download.hpp
HEADERS += src/profile_sets.hpp
HEADERS += src/interaction_icons.hpp
HEADERS += src/startup_view.hpp
HEADERS += src/startup_update.hpp
HEADERS += src/window_backdrop.hpp

HEADERS += src/physical_mouse.hpp src/rapid_fire.hpp
