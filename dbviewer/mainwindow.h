#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlRelationalTableModel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QSqlDatabase db;
    QSqlRelationalTableModel *m_model;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private:
    Ui::MainWindow *ui;
    void loadFile(const QString &filename);
    QString buildFilterString();

protected slots:
    void applyFilter();
    void chooseDbFile();

};

#endif // MAINWINDOW_H
