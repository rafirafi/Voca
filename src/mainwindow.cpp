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

    model_ = new QSqlTableModel(this, db_);
    model_->setTable("voca");
    model_->select();

    completer_ = new QCompleter(model_, this);
    ui->lineEdit_input->setCompleter(completer_);
    completer_->setCompletionColumn(0); // "word"
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
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
    const QString &table = "create table if not exists voca (word varchar[max], meaning varchar[max]);";
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

void MainWindow::on_pushButton_update_clicked()
{
    QString word = ui->lineEdit_input->text();
    word = word.trimmed();
    if (word.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "empty word";
        return;
    }
    word = word.toLower();

    QString meaning = ui->textEdit_output->toPlainText();
    meaning = meaning.trimmed();
    if (meaning.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "empty meaning";
        return;
    }

    // if doesn't exist, insert in database
    QSqlQuery query(db_);
    QString str = QString("insert into voca (word, meaning) values ('%1', '%2')").arg(word).arg(meaning);
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }

    // update model for completer
    model_->select();
}

void MainWindow::on_pushButton_search_clicked()
{
    QString word = ui->lineEdit_input->text();
    word = word.trimmed();
    if (word.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "empty word";
        return;
    }
    word = word.toLower();

    // check if exists
    QSqlQuery query(db_);
    QString str = QString("select meaning from voca where word='%1' limit 1").arg(word);
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }
    ok = query.exec();
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
        abort();
    }

    // display meaning if any
    ui->textEdit_output->clear();
    if (query.next()) {
        QString meaning = query.value("meaning").toString();
        qDebug() << Q_FUNC_INFO << "result" << meaning;
        ui->textEdit_output->setText(meaning);
    } else {
        qDebug() << Q_FUNC_INFO << "no result";
    }
}
