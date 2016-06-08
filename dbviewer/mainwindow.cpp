#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QSqlError>
#include <QFileDialog>

#define APP_DISPLAY_NAME "UniqLogger DbViewer"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->btnFilter, SIGNAL(clicked(bool)), this, SLOT(applyFilter()));
    connect(ui->actionChoose_Db_file, SIGNAL(triggered(bool)), this, SLOT(chooseDbFile()));
}

MainWindow::~MainWindow()
{
    delete ui;
}



void
MainWindow::chooseDbFile()
{
    QString filename = QFileDialog::getOpenFileName(0, "DbViewer");
    loadFile(filename);
}

void
MainWindow::loadFile(const QString &filename)
{
    qDebug() << "opening db file" << filename;

    db = QSqlDatabase::addDatabase("QSQLITE", filename);
    db.setDatabaseName(filename);

    bool ok = db.open();
    if (!ok) {
        qDebug() << "------ DB ERROR: " <<  db.lastError();
        return;
    }

    this->setWindowTitle(QString(APP_DISPLAY_NAME) + " - " + filename);

    m_model = new QSqlRelationalTableModel(this, db);
    m_model->setTable("ul_event");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->setRelation(3, QSqlRelation("ul_level", "level_value", "level_name"));

    ok = m_model->select();
    qDebug() << "selecting from table:" << ok;
    if (!ok)
        qDebug() << m_model->lastError();

    ui->tableView->setModel(m_model);
    ui->tableView->hideColumn(0);
    ui->tableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    ui->statusBar->setToolTip("Total Rows:" + QString::number(m_model->rowCount()));
}


void
MainWindow::applyFilter()
{
    QString filter = buildFilterString();
    qDebug() << "Applying filter: " << filter;
    m_model->setFilter(filter);
}


QString
MainWindow::buildFilterString()
{
    QString filter;
    QStringList tokens;

    if (!ui->txtModule->text().isEmpty()) {
        qDebug() << "adding Filtering for module " << ui->txtModule->text();
        tokens << "module like '%" + ui->txtModule->text() + "%'";
    }


    if (!ui->txtMessage->text().isEmpty()) {
        qDebug() << "adding Filtering for message part " << ui->txtMessage->text();
        tokens << "message like '%" + ui->txtMessage->text() + "%'";
    }


    if (!ui->dteStart->dateTime().isNull()) {
        qDebug() << "adding Filtering for start time part " << ui->dteStart->dateTime();
        tokens << "tstamp >= '" + ui->dteStart->dateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") + "'";
    }

    if (!ui->dteStop->dateTime().isNull()) {
        qDebug() << "adding Filtering for stop time part " << ui->dteStop->dateTime();
        tokens << "tstamp <= '" + ui->dteStop->dateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") + "'";
    }

    filter = tokens.join(" and ");
    return filter;
}
