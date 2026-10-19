QT += core network
CONFIG += console c++17
CONFIG -= app_bundle debug debug_and_release
TEMPLATE = app
TARGET = Azec-update-checks
DESTDIR = $$OUT_PWD/release
SOURCES += update_checks.cpp
HEADERS += ../src/update_check.hpp
