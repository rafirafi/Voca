#-------------------------------------------------
#
# Project created by QtCreator 2018-03-06T21:09:38
#
#-------------------------------------------------

QT       += core gui

QT       += sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ../vocabulary
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    preferences.cpp \
    collection.cpp

HEADERS  += mainwindow.h \
    preferences.h \
    collection.h

FORMS    += mainwindow.ui

linux {
    DEFINES += SUPPORT_APKG
}

#Add support for apkg export
contains(DEFINES, SUPPORT_APKG) {
    SOURCES += ankipackage.cpp
    HEADERS += ankipackage.h
# shouldn't be necessary with a relatively recent distribution
    include(/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_KArchive.pri)
    QT += KArchive
}

TRANSLATIONS += languages/fr_FR.ts

!exists($$PWD/languages/fr_FR.qm){
    error("run lrelease manually before building")
}

DISTFILES += LICENSE \
    languages/fr_FR.ts \
    ../README.md

RESOURCES += resources.qrc

