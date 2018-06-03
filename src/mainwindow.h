// Copyright : 2018 rafirafi
// License : GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QCompleter>
#include <QString>

#include "collection.h"
#include "preferences.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_update_clicked();
    void on_pushButton_search_clicked();
    void on_actionExport_to_csv_triggered();
    void on_zoomGroupAction_triggered(QAction *action);
    void on_actionImport_from_tab_separated_csv_triggered();
    void on_actionDelete_current_deck_triggered();
    void on_actionExport_as_apkg_triggered();
    void on_actionAbout_triggered();
    void on_actionRename_current_deck_triggered();
    void on_actionCreate_current_deck_triggered();
    void on_actionChoose_current_deck_triggered();
    void on_actionShow_Deck_Name_toggled(bool isChecked);
    void on_actionShow_Current_Word_toggled(bool isChecked);

private:
    Ui::MainWindow *ui;

    Collection col_;
    QSqlTableModel *model_ = nullptr;
    QCompleter *completer_ = nullptr;

    int currentDeckId_ = -1;

    void setCurrentDeck(const QString &deckName, bool create = true);
    void renameDeck(const QString &deckOldName, const QString &deckNewName);
    bool setFontSize(int pointSize);
    void setCurrentDeckId(int deckId);
    void clearCurrentWord();

    Preferences prefs_;
};

#endif // MAINWINDOW_H
