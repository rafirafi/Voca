// Copyright : 2018 rafirafi
// License : GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator appTranslator;
    appTranslator.load(":/ts/ts.qm");
    a.installTranslator(&appTranslator);

    MainWindow w;
    w.setWindowTitle(a.applicationName());
    w.show();

    return a.exec();
}
