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

    void clearFilterFields();
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private:
    Ui::MainWindow *ui;
    void loadFile(const QString &filename);
    QString buildFilterString();
    QStringList getLogModuleNames(QSqlDatabase &db);
    QPair<QString, QString> getLogMinMaxDates(QSqlDatabase &db);

protected slots:
    void applyFilter();
    void resetFilter();
    void chooseDbFile();

};

#endif // MAINWINDOW_H
