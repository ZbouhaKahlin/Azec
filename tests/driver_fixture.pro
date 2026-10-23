TEMPLATE = lib
CONFIG += dll c++17 release
CONFIG -= qt debug debug_and_release
TARGET = Azec-driver-fixture
DESTDIR = $$OUT_PWD/release
SOURCES += driver_fixture.cpp
