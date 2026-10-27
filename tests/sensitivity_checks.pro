QT = core
CONFIG += console c++17 release
CONFIG -= app_bundle debug debug_and_release
TEMPLATE = app
TARGET = Azec-sensitivity-checks
DESTDIR = $$OUT_PWD/release
SOURCES += sensitivity_checks.cpp core_checks.cpp ../src/core.cpp ../src/engine.cpp
HEADERS += ../src/sensitivity.hpp ../src/profile_store.hpp ../src/profile_transfer.hpp
DEFINES += NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS
QMAKE_CXXFLAGS += /utf-8
LIBS += -luser32

LIBS += -ladvapi32
