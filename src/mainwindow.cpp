#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QSaveFile>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>


#ifdef SUPPORT_APKG
#include "ankipackage.h"
#endif

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

    on_zoomGroupAction_triggered(ui->actionZoom_In);

    QObject::connect(completer_, SIGNAL(activated(QString)),
                     this, SLOT(on_pushButton_search_clicked()));

#ifndef SUPPORT_APKG
    ui->actionExport_as_apkg->setVisible(false);
#endif

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
    QString str = "create table if not exists voca (word varchar[max], meaning varchar[max]);";
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }
    ok = query.exec();
    if (!ok) {
        qDebug() << query.executedQuery();
        abort();
    }

    str = "create unique index if not exists idx_voca_word on voca (word);";
    ok = query.prepare(str);
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

    // insert or replace
    QSqlQuery query(db_);
    QString str = QString("replace into voca (word, meaning) values (:word,:meaning)");

    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }

    query.bindValue(":word", word);
    query.bindValue(":meaning", meaning);

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

    QString str = QString("select meaning from voca where word=:word limit 1");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }

    query.bindValue(":word", word);

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

void MainWindow::on_actionExport_to_csv_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),
                               QDir::homePath(),
                               tr("csv files (*.csv)"));
    if (filename.isEmpty()) {
        return;
    }

    QSaveFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox msgBox;
        msgBox.setText(tr("Export to csv failed : permission denied"));
        msgBox.exec();
        return;
    }

    QSqlQuery query(db_);
    QString str = QString("select word, meaning from voca;");
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

    while (query.next()) {
        QString word = query.value("word").toString();
        QString meaning = query.value("meaning").toString();
        word.replace("\t", "   ");
        meaning.replace("\t", "   ");
        word.replace("\n","<br/>");
        meaning.replace("\n","<br/>");
        QByteArray line = word.toLocal8Bit() + '\t' + meaning.toLocal8Bit() + '\n';
        if (file.write(line) == -1) {
            break;
        }
    }

    if (!file.commit()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Export to csv failed : write error"));
        msgBox.exec();
    }
}

void MainWindow::on_zoomGroupAction_triggered(QAction *action)
{
    QFont font = ui->centralWidget->font();
    int pointSize = font.pointSize();
    if (action == ui->actionNormal_Size) {
        int appPointSize = qApp->font().pointSize();
        if (pointSize == appPointSize) {
            return;
        }
        pointSize = appPointSize;
    } else {
        if (action == ui->actionZoom_In) {
            pointSize += 2;
        } else if (action == ui->actionZoom_Out) {
            pointSize -= 2;
        }
    }
    font.setPointSize(pointSize);
    ui->centralWidget->setFont(font);
    completer_->popup()->setFont(font);
}

void MainWindow::on_actionImport_from_tab_separated_csv_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                               QDir::homePath(),
                               tr("csv files (*.csv)"));
    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox msgBox;
        msgBox.setText(tr("Import from csv failed : permission denied"));
        msgBox.exec();
        return;
    }

    QSqlQuery query(db_);
    QString str = QString("replace into voca (word, meaning) values (:word,:meaning)");
    bool ok = query.prepare(str);
    if (!ok) {
        qDebug() << Q_FUNC_INFO << "prepare" << query.executedQuery();
        abort();
    }

    int importCnt = 0, lineCnt = 0;

    char buf[4096];
    while (file.readLine(buf, sizeof(buf)) != -1) {
        lineCnt++;

        QString line(buf);
        line.replace("<br/>", "\n");
        QStringList words = line.split('\t');
        if (words.size() != 2) {
            continue;
        }
        words[0] = words[0].trimmed();
        words[1] = words[1].trimmed();
        if (words[0].isEmpty() || words[1].isEmpty()) {
            continue;
        }

        words[0].toLower();

        query.bindValue(":word", words[0]);
        query.bindValue(":meaning", words[1]);
        ok = query.exec();
        if (!ok) {
            qDebug() << Q_FUNC_INFO << "exec" <<  query.executedQuery();
            abort();
        }

        importCnt++;
    }

    QMessageBox msgBox;
    msgBox.setText(tr("Import from csv : imported %1/%2 data").arg(importCnt).arg(lineCnt));
    msgBox.exec();

    if (importCnt) {
        model_->select(); // update model for completer
    }
}

void MainWindow::on_actionDelete_everything_triggered()
{
    ui->lineEdit_input->clear();
    ui->textEdit_output->clear();

    QSqlQuery query(db_);
    QString str = QString("delete from voca");
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

    model_->select();
}

void MainWindow::on_actionExport_as_apkg_triggered()
{
#ifdef SUPPORT_APKG
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File As .apkg "),
                               QDir::homePath(),
                               tr("apkg files (*.apkg)"));
    if (fileName.endsWith(".apkg")) {
        fileName.remove(-5, 5);
    }
    if (fileName.isEmpty()) {
        return;
    }
    auto offset = fileName.lastIndexOf(QDir::separator());
    QString filePath = fileName.left(offset);
    QString baseName = fileName.mid(offset + 1);
    if (baseName.isEmpty()) {
        return;
    }

    QString deckName;
    QMessageBox msgBox;
    msgBox.setText(QObject::tr("Specifying a deck name ?"));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    if (msgBox.exec() ==  QMessageBox::Ok) {
        deckName = QInputDialog::getText(this, tr("Deck name"), tr("Deck Name :"), QLineEdit::Normal);
    }

    AnkiPackage apkg{deckName};
    QSqlQuery query(db_);
    query.setForwardOnly(true);
    QString str = QString("select word, meaning from voca;");
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

    while (query.next()) {
        QString word = query.value("word").toString();
        QString meaning = query.value("meaning").toString();
        word.replace("\n","<br/>");
        meaning.replace("\n","<br/>");
        apkg.addBasicCard(word, meaning);
    }

    apkg.exportAsApkg(filePath, baseName);
#endif
}

void MainWindow::on_actionAbout_triggered()
{
    QDialog dia;
    dia.setWindowTitle(tr("About ") + qApp->applicationName());

    auto butBox = new QDialogButtonBox(QDialogButtonBox::Close, &dia);
    butBox->button(QDialogButtonBox::Close)->setIcon(QIcon::fromTheme("cancel"));
    connect(butBox, &QDialogButtonBox::clicked, &dia, &QDialog::close);

    auto text = new QLabel(&dia);
    text->setAlignment(Qt::AlignCenter);
    text->setText(tr("Copyright © 2018 rafirafi\n\nLicense AGPL v3\n"));

    dia.setLayout(new QVBoxLayout(&dia));
    dia.layout()->addWidget(text);
    dia.layout()->addWidget(butBox);

    dia.exec();
}
