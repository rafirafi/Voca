#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QCompleter>

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

private:
    Ui::MainWindow *ui;

    QSqlDatabase db_;
    QSqlTableModel *model_ = nullptr;
    QCompleter *completer_ = nullptr;
    void dbOpen();
    void dbClose();
};

#endif // MAINWINDOW_H
