// Copyright : 2018 rafirafi
// License : GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cassert>

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QSaveFile>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>

#ifdef SUPPORT_APKG
#include "ankipackage.h"
#endif

#include <QtDebug>

static inline const QString defaultDeckName()
{
    return MainWindow::tr("Default");
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    prefs_.init();

    model_ = new QSqlTableModel(this, *col_.db());
    model_->setTable("voca");
    model_->select();

    completer_ = new QCompleter(model_, this);
    ui->lineEdit_input->setCompleter(completer_);
    completer_->setCompletionColumn(0); // "word"
    completer_->setCaseSensitivity(Qt::CaseInsensitive);

    QObject::connect(completer_, SIGNAL(activated(QString)),
                     this, SLOT(on_pushButton_search_clicked()));

    setFontSize(prefs_.getProperty("last-font-size").toInt());

#ifndef SUPPORT_APKG
    ui->actionExport_as_apkg->setVisible(false);
#endif

    // start on last deck or default
    QString deckName = prefs_.getProperty("last-deck-name").toString();
    if (!deckName.isEmpty()) {
        setCurrentDeck(deckName, false);
    }
    if (currentDeckId_ == -1) {
        setCurrentDeck(defaultDeckName());
    }

    ui->actionShow_Deck_Name->setChecked(prefs_.getProperty("show-current-deck-name").toBool());
    ui->actionShow_Current_Word->setChecked(prefs_.getProperty("show-last-word-found").toBool());
}

MainWindow::~MainWindow()
{
    delete model_; // silence QSqlDatabasePrivate::removeDatabase: connection is still in use
    delete ui;
}

void MainWindow::setCurrentDeck(const QString &deckName, bool create)
{
    int id = col_.getDeckId(deckName);
    if (id == -1) {
        if (!create) {
            return;
        }
        col_.addDeck(deckName);
        id = col_.getDeckId(deckName);
    }
    assert(id != -1);

    setCurrentDeckId(id);

    // set then apply filter
    assert(model_);
    model_->setFilter(QString("deckid='%1'").arg(currentDeckId_));
    model_->select();

    ui->label_current_deck_name->setText(QString(tr("Deck : %1")).arg(deckName));
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

    col_.upsertWord(currentDeckId_, word, meaning);

    // update model for completer
    model_->select();

    // update ui
    ui->label_current_word_name->setText(tr("Word : %1").arg(word));
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

    // display meaning if any
    ui->textEdit_output->clear();
    ui->label_current_word_name->setText(tr("Word : %1").arg(""));

    QString meaning = col_.getMeaning(currentDeckId_, word);
    if (!meaning.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "result" << meaning;
        ui->textEdit_output->setText(meaning);
        ui->label_current_word_name->setText(tr("Word : %1").arg(word));
    } else {
        qDebug() << Q_FUNC_INFO << "no result";
    }
}

// export current deck only
void MainWindow::on_actionExport_to_csv_triggered()
{
    QString fileName = QDir::homePath() + "/" + col_.getDeckName(currentDeckId_) + ".csv";
    fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                               fileName,
                               tr("csv files (*.csv)"));
    if (fileName.isEmpty()) {
        return;
    }

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox msgBox(this);
        msgBox.setText(tr("Export to csv failed : permission denied"));
        msgBox.exec();
        return;
    }

    DeckContent content = col_.getDeckContent(currentDeckId_);
    QString word, meaning;
    while (!content.isEmpty()) {
        content.getNext(word, meaning);
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
        QMessageBox msgBox(this);
        msgBox.setText(tr("Export to csv failed : write error"));
        msgBox.exec();
    }
}

void MainWindow::on_zoomGroupAction_triggered(QAction *action)
{
    int pointSize = ui->centralWidget->font().pointSize();
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
    prefs_.setProperty("last-font-size", pointSize);
    setFontSize(pointSize);
}

void MainWindow::on_actionImport_from_tab_separated_csv_triggered()
{    
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                               QDir::homePath(),
                               tr("csv files (*.csv)"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox msgBox(this);
        msgBox.setText(tr("Import from csv failed : permission denied"));
        msgBox.exec();
        return;
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
        words[0] = words[0].toLower();

        col_.upsertWord(currentDeckId_, words[0], words[1]);

        importCnt++;
    }

    QMessageBox msgBox(this);
    msgBox.setText(tr("Import from csv : imported %1/%2 data").arg(importCnt).arg(lineCnt));
    msgBox.exec();

    if (importCnt) {
        model_->select(); // update model for completer
    }
}

void MainWindow::on_actionDelete_current_deck_triggered()
{
    col_.deleteDeck(currentDeckId_);

    clearCurrentWord();

    //  go to 'Default' deck
    setCurrentDeck(defaultDeckName());
}

void MainWindow::on_actionExport_as_apkg_triggered()
{
#ifdef SUPPORT_APKG
    QString fileName = QDir::homePath() + "/" + col_.getDeckName(currentDeckId_) + ".apkg";
    fileName = QFileDialog::getSaveFileName(this, tr("Save File As .apkg "),
                               fileName,
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

    QString deckName = col_.getDeckName(currentDeckId_);
    QMessageBox msgBox(this);
    msgBox.setText(tr("Changing deck name, current : %1 ?").arg(deckName));
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    if (msgBox.exec() ==  QMessageBox::Yes) {
        deckName = QInputDialog::getText(this, tr("Deck name"), tr("Deck Name :"), QLineEdit::Normal);
    }

    AnkiPackage apkg{deckName};

    DeckContent content = col_.getDeckContent(currentDeckId_);
    QString word, meaning;
    while (!content.isEmpty()) {
        content.getNext(word, meaning);
        word.replace("\n","<br/>");
        meaning.replace("\n","<br/>");
        apkg.addBasicCard(word, meaning);
    }

    apkg.exportAsApkg(filePath, baseName);
#endif
}

void MainWindow::on_actionAbout_triggered()
{
    QDialog dia(this);
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

// deckOldName deck must exists and deckNewName can't be empty
void MainWindow::renameDeck(const QString &deckOldName, const QString &deckNewName)
{
    qDebug() << __func__ << "deckNewName" << deckNewName << "deckOldName" << deckOldName;
    // check params validity
    assert(!deckNewName.isEmpty());
    int deckId = col_.getDeckId(deckOldName);
    assert(deckId != -1);

    // nothing to do
    if (deckNewName == deckOldName) {
        return;
    }

    // check case new name already exists
    int newDeckId = col_.getDeckId(deckNewName);
    if (newDeckId != -1) {
        // ask user if ok before deleting deck
        QMessageBox msgBox(this);
        msgBox.setText(tr("Deck %1 already exists, ok for deleting it ?").arg(deckNewName));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if (msgBox.exec() ==  QMessageBox::Cancel) {
            qDebug() << __func__ << "no to overwrite";
            return;
        }
        col_.deleteDeck(newDeckId);
        if (newDeckId == currentDeckId_) {
            setCurrentDeckId(-1);
        }
    }

    // do the renaming
    col_.renameDeck(deckId, deckNewName);

    // if old name was default, recreate it
    if (deckOldName == defaultDeckName()) {
        col_.addDeck(defaultDeckName());
    }

    // make sure to get a current deck
    if (currentDeckId_ == -1) {
        ui->label_current_deck_name->setText(QString(tr("Deck : %1")).arg(defaultDeckName()));
    } else if (currentDeckId_ == deckId) { // if current deck was renamed
        ui->label_current_deck_name->setText(QString(tr("Deck : %1")).arg(deckNewName));
    }

    // to record new name in preferences
    setCurrentDeckId(currentDeckId_);
}

bool MainWindow::setFontSize(int pointSize)
{
    if (pointSize <= 0) {
        return false;
    }
    QFont font = ui->centralWidget->font();
    font.setPointSize(pointSize);
    ui->centralWidget->setFont(font);
    completer_->popup()->setFont(font);
    return true;
}

void MainWindow::setCurrentDeckId(int deckId)
{
    currentDeckId_ = deckId;
    prefs_.setProperty("last-deck-name", col_.getDeckName(deckId));
}

void MainWindow::clearCurrentWord()
{
    ui->lineEdit_input->clear();
    ui->textEdit_output->clear();
    ui->label_current_word_name->setText(tr("Word : %1").arg(""));
}

void MainWindow::on_actionRename_current_deck_triggered()
{
    QString deckOldName = col_.getDeckName(currentDeckId_);
    assert(!deckOldName.isEmpty());
    QString deckNewName = QInputDialog::getText(this,
                                                tr("Rename Deck"),
                                                tr("Deck name : %1").arg(deckOldName),
                                                QLineEdit::Normal);
    if (deckNewName.isEmpty()) {
        return;
    }
    renameDeck(deckOldName, deckNewName);
}

void MainWindow::on_actionCreate_current_deck_triggered()
{
    QString deckNewName = QInputDialog::getText(this,
                                                tr("Create Deck"),
                                                tr("Deck name :"),
                                                QLineEdit::Normal);
    qDebug() << "name" << deckNewName;
    if (deckNewName.isEmpty()) {
        return;
    }
    int deckId = col_.getDeckId(deckNewName);
    qDebug() << "id" << deckId;
    if (deckId != -1) {
        QMessageBox msgBox(this);
        msgBox.setText(tr("Deck %1 already exists, ok for overwriting it ?").arg(deckNewName));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (msgBox.exec() ==  QMessageBox::No) {
            return on_actionCreate_current_deck_triggered();
        }
        col_.deleteDeck(deckId);
    }

    clearCurrentWord();

    setCurrentDeck(deckNewName);
}

void MainWindow::on_actionChoose_current_deck_triggered()
{
    QDialog dia(this);
    dia.setWindowTitle(tr("Choose current deck"));
    dia.setLayout(new QVBoxLayout(&dia));

    QSqlTableModel *model = new QSqlTableModel(&dia, *col_.db());
    model->setTable("decks");
    model->select();

    QListView *view = new QListView(&dia);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setModel(model);
    view->setModelColumn(1); // deck name column

    dia.layout()->addWidget(view);
    auto butBox = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Ok, &dia);
    butBox->button(QDialogButtonBox::Close)->setIcon(QIcon::fromTheme("cancel"));
    connect(butBox, &QDialogButtonBox::rejected , &dia, &QDialog::close);
    connect(butBox, &QDialogButtonBox::accepted , &dia, &QDialog::accept);
    dia.layout()->addWidget(butBox);
    if (dia.exec() == QDialog::Rejected) {
        return;
    }
    auto modelIndexList = view->selectionModel()->selectedIndexes();
    if (modelIndexList.isEmpty()) {
        return;
    }
    assert(modelIndexList.size() == 1);
    QString deckName = modelIndexList[0].data(Qt::DisplayRole).toString();
    int deckId = col_.getDeckId(deckName);
    assert(deckId != -1);
    if (deckId == currentDeckId_) {
        return;
    }

    clearCurrentWord();

    setCurrentDeck(deckName);
}

void MainWindow::on_actionShow_Deck_Name_toggled(bool isChecked)
{
    prefs_.setProperty("show-current-deck-name", isChecked);
}

void MainWindow::on_actionShow_Current_Word_toggled(bool isChecked)
{
    prefs_.setProperty("show-last-word-found", isChecked);
}
