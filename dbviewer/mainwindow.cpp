#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QSqlError>
#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMessageBox>

#define APP_DISPLAY_NAME "UniqLogger DbViewer"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_model(nullptr),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->btnFilter, SIGNAL(clicked(bool)), this, SLOT(applyFilter()));
    connect(ui->btnReset, SIGNAL(clicked(bool)), this, SLOT(resetFilter()));
    connect(ui->actionChoose_Db_file, SIGNAL(triggered(bool)), this, SLOT(chooseDbFile()));

    //Connect these if we want to re-filter each time we change something
    connect(ui->txtMessage, SIGNAL(returnPressed()), this, SLOT(applyFilter()));
    connect(ui->txtModule, SIGNAL(returnPressed()), this, SLOT(applyFilter()));
    connect(ui->cmbSeverity, SIGNAL(currentIndexChanged(int)), this, SLOT(applyFilter()));
}

MainWindow::~MainWindow()
{
    delete ui;
}



void
MainWindow::chooseDbFile()
{
    QString filename = QFileDialog::getOpenFileName(nullptr, "DbViewer");
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
        QMessageBox::critical(this, "DB Error", QString("Could not load the DB file: ") + db.lastError().text());
        return;
    }

    QStringList modules;
    modules << "--" << "INFO" << "WARNING" << "CRITICAL" << "FATAL" << "DEBUG" << "FULL DEBUG";
    ui->cmbSeverity->addItems(modules);
    QPair<QString, QString> dates = this->getLogMinMaxDates(db);
    QDateTime dt(QDateTime::fromString(dates.first, "yyyy.MM.dd-hh:mm:ss.zzz"));
    if (!dt.isValid())
        qDebug() << " ==================== ERR" << dates.first;
    ui->dteStop->setDateTime(QDateTime::fromString(dates.second, "yyyy.MM.dd-hh:mm:ss.zzz"));

    this->setWindowTitle(QString(APP_DISPLAY_NAME) + " - " + filename);

    m_model = new QSqlRelationalTableModel(this, db);
    m_model->setTable("ul_event");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->setRelation(3, QSqlRelation("ul_level", "level_value", "level_name"));

    ok = m_model->select();
    qDebug() << "selecting from table:" << ok;
    if (!ok) {
        qDebug() << m_model->lastError();
        QMessageBox::critical(this, "DB Error", QString("Could not access data in the DB file: ") + db.lastError().text());
    }

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
    if (m_model)
        m_model->setFilter(filter);
}



void
MainWindow::resetFilter()
{
    if (m_model)
        m_model->setFilter("");
    clearFilterFields();
}


void
MainWindow::clearFilterFields()
{
    ui->dteStart->setDateTime(QDateTime());
    ui->dteStop->setDateTime(QDateTime());
    ui->txtMessage->clear();
    ui->txtModule->clear();
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


    if (ui->dteStart->isEnabled()) {
        qDebug() << "adding Filtering for start time part " << ui->dteStart->dateTime();
        tokens << "tstamp >= '" + ui->dteStart->dateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") + "'";
    }

    if (ui->dteStop->isEnabled()) {
        qDebug() << "adding Filtering for stop time part " << ui->dteStop->dateTime();
        tokens << "tstamp <= '" + ui->dteStop->dateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") + "'";
    }


    if (ui->cmbSeverity->currentText().trimmed() != "--") {
        qDebug() << "adding Filtering for severity part " << ui->cmbSeverity->currentText();
        tokens << "level_name = '" + ui->cmbSeverity->currentText() + "'";
    }

    filter = tokens.join(" and ");
    return filter;
}


QStringList
MainWindow::getLogModuleNames(QSqlDatabase &db)
{
    QStringList sl;
    QSqlQuery q(db);

    //q.prepare();

    q.exec("select distinct module from ul_event;");

    while (q.next()) {
        sl.append(q.record().value("module").toString());
    }

    return sl;
}




QPair<QString, QString>
MainWindow::getLogMinMaxDates(QSqlDatabase &db)
{
    QPair<QString, QString> p;
    QSqlQuery q(db);
    q.prepare("select min(tstamp) as min_date, max(tstamp) as max_date from ul_event;");
    q.exec();

    if (q.next()) {
        p.first = q.record().value("min_date").toString();
        p.second = q.record().value("max_date").toString();
    }

    return p;
}
