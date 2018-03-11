#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>

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

    QSqlQuery query(db_);
    const QString &table = "create table if not exists voca ( word varchar[max], meaning varchar[max]);";
    bool ok = query.prepare(table);
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }
    ok = query.exec();
    if (!ok) {
        qDebug() << query.executedQuery();
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


