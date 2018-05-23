// Copyright : 2018 rafirafi
// License : GNU AGPL, version 3 or later; http://www.gnu.org/licenses/agpl.html

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QCompleter>
#include <QString>

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

private:
    Ui::MainWindow *ui;

    QSqlDatabase db_;
    QSqlTableModel *model_ = nullptr;
    QCompleter *completer_ = nullptr;
    int currentDeckId_ = -1;
    void dbOpen();
    void dbClose();
    void addDeck(const QString &deckName);
    void setCurrentDeck(const QString &deckName);    
    void deleteDeck(int deckId);
    int getDeckId(const QString &deckName);
    void renameDeck(const QString &deckOldName, const QString &deckNewName);
};

#endif // MAINWINDOW_H
