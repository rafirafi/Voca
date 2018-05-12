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


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

DEFINES += SUPPORT_APKG
contains(DEFINES, SUPPORT_APKG) {
    message("Add support for apkg export")
    SOURCES += ankipackage.cpp
    HEADERS += ankipackage.h
    include(/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_KArchive.pri)
    QT += KArchive
}

