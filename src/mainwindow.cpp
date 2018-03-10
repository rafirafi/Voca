#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    dbOpen();
}

MainWindow::~MainWindow()
{
    dbClose();

    delete ui;
}

void MainWindow::dbOpen()
{
    db_ = QSqlDatabase::addDatabase("QSQLITE", "vocadb");
    db_.setDatabaseName("vocadb.sqlite");
    if (!db_.open()) {
        qDebug() << Q_FUNC_INFO << "db open failed";
        abort();
    }
}

void MainWindow::dbClose()
{
    if (db_.isOpen()) {
        db_.close();
    } else {
        qDebug() << Q_FUNC_INFO << "db was not open";
        abort();
    }
}

