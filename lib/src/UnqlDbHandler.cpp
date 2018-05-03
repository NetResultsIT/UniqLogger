#include "UnqlDbHandler.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QVariant>
#include <QTime>

#include "unqlog_common.h"


UnqlDbHandler::UnqlDbHandler(const DbhConfig dbconf)
    : NrBaseDbHandler(dbconf)
{
    ULDBG << "connecting to " << _M_DbHost << " user: " << _M_DbUsername << " ( " << _M_DbPasswd << ")"
             << " on db " << _M_DbName;
}


UnqlDbHandler::~UnqlDbHandler()
{ /* empty dtor */ }


/*!
	\brief this static method creates the DB to hold the events happening in the Q for the Kalliope CallCenter edition
	\param qeventdbname the string containing the full pathname of the queue events DB
	\return a positive integer in case something wrong happened in the creation process 0 otherwise
	This method is called in kctiserver ctor and only if in the .ini file the logQevents flag is set to true.
  */
int
UnqlDbHandler::createLoggerDb(QString qeventdbname)
{
    QSqlDatabase qevdb = QSqlDatabase::addDatabase("QSQLITE", "qeventsdb");
    qevdb.setDatabaseName(qeventdbname);

    bool ok = qevdb.open();
    if (!ok) {
        qDebug() << "------ QEVDB ERROR: " <<  qevdb.lastError();
        return(1);
    }

    //first create the db
    QSqlQuery q(qevdb);

    q.prepare("\
                CREATE TABLE IF NOT EXISTS  ul_level (\n\
                    id          INTEGER PRIMARY KEY,\n\
                    level_name  VARCHAR(40),\n\
                    level_value INTEGER UNIQUE\
             );");
    ok = q.exec();
    if (!ok) {
        qDebug() << "got error: " << q.lastError() << " for query (" << q.lastQuery() << ")";
        return(10);
    }

    q.prepare("\
               CREATE TABLE IF NOT EXISTS  ul_event (\n\
                    id          INTEGER PRIMARY KEY,\n\
                    tstamp      TIMESTAMP,\n\
                    module      VARCHAR(40) NOT NULL,\n\
                    level       INTEGER NOT NULL REFERENCES ul_level(level_value),\n\
                    message     VARCHAR(5000)\
               );");

    ok = q.exec();
    if(!ok) {
        qDebug() << "got error: " << q.lastError() << " for query (" << q.lastQuery() << ")";
        return(10);
    }


    QVariantList vals,names;
    names   << "FATAL"
            << "CRITICAL"
            << "WARNING"
            << "INFO"
            << "DEBUG"
            << "FULL DEBUG"
            << "FORCED";

    vals << UNQL::LOG_FATAL
         << UNQL::LOG_CRITICAL
         << UNQL::LOG_WARNING
         << UNQL::LOG_INFO
         << UNQL::LOG_DBG
         << UNQL::LOG_DBG_ALL
         << UNQL::LOG_FORCED;

    q.prepare("INSERT INTO ul_level (level_name, level_value) VALUES (?,?)");
    q.addBindValue(names);
    q.addBindValue(vals);
    if (!q.execBatch()) {
        qDebug() << "got error: " << q.lastError() << " for query (" << q.lastQuery() << ")";
        return 10;
    }
    return 0;
}


void
UnqlDbHandler::logEvent(const QString &module, const QString &tstamp, const QString &level, const QString &message)
{
    openDbConn();

    QSqlQuery query(_M_db);
    bool ok;
    QString querystring = "INSERT INTO ul_event (module,tstamp,level,message) VALUES (:lname,:tstamp,:prio,:msg)";

    query.prepare(querystring);
    query.bindValue(":lname", module);
    query.bindValue(":tstamp", tstamp);
    query.bindValue(":prio", level);
    query.bindValue(":msg", message);

    ok = query.exec();
    if(!ok) {
        qDebug() << "got error: " << query.lastError() << " for query (" << query.lastQuery() << ")";
    }
    query.clear();
    closeDbConn();
}
