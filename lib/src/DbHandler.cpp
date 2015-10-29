#include "DbHandler.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QVariant>
#include <QTime>

#include "unqlog_common.h"

//included for debug statements in ctor
#include <QCoreApplication>
#include <QStringList>

UnqlDbHandler::UnqlDbHandler(QString inDbname) :
	_M_DbName(inDbname)
{

	//check plugins
    //QCoreApplication *capp = QCoreApplication::instance();
	foreach (QString path, QCoreApplication::libraryPaths())
		qDebug() << "library path: " << path;
	foreach (QString plugin, QSqlDatabase::drivers())
		qDebug() << "library path: " << plugin;

    _M_db = QSqlDatabase::addDatabase("QSQLITE",_M_DbName);
    _M_db.setDatabaseName(_M_DbName);

#ifdef ULOGDBG
	qDebug() << "connecting to " << _M_DbHost << " user: " << _M_DbUsername << " ( " << _M_DbPasswd << ")"
			<< " on db " << _M_DbName;
#endif
}

UnqlDbHandler::UnqlDbHandler(QString inDbname, QString inDbuser, QString inDbpwd, QString inDbhost, int inDbport) :
	_M_DbHost(inDbhost),
	_M_DbName(inDbname),
	_M_DbUsername(inDbuser),
    _M_DbPasswd(inDbpwd),
	_M_DbPort(inDbport)
{

	//check plugins
	QCoreApplication *capp = QCoreApplication::instance();
	//capp->addLibraryPath("/usr/lib/qt4/plugins");
	capp->addLibraryPath("/opt/qt/plugins");
	foreach (QString path, QCoreApplication::libraryPaths())
		qDebug() << "library path: " << path;
	foreach (QString plugin, QSqlDatabase::drivers())
		qDebug() << "library path: " << plugin;

	_M_db = QSqlDatabase::addDatabase("QSQLITE",_M_DbName);
	_M_db.setHostName(_M_DbHost);
	_M_db.setDatabaseName(_M_DbName);
	_M_db.setUserName(_M_DbUsername);
	_M_db.setPassword(_M_DbPasswd);

#ifdef ULOGDBG
	qDebug() << "connecting to " << _M_DbHost << " user: " << _M_DbUsername << " ( " << _M_DbPasswd << ")"
			<< " on db " << _M_DbName;
#endif
}

UnqlDbHandler::~UnqlDbHandler()
{
    closeDbConn();
    QSqlDatabase::removeDatabase(_M_DbName);
}

/*!
  \brief opens a connection towards the already configured Database
  \return true if the DB is opened successfully, false otherwise
  */
bool
UnqlDbHandler::openDbConn()
{
    bool ok = false;
    /* qDebug() << "OpenDbConn() -- db is open (" << _M_db.isOpen() <<") -- db is valid ("
			<< _M_db.isValid() <<") -- db is openError ("<< _M_db.isOpenError() <<")"; */

    if ( !_M_db.isOpen() || !_M_db.isValid() )
    {
        if (_M_db.isOpen())
        {
			_M_db.close();
#ifdef ULOGDBG
			qDebug() << "Database was INVALID but OPEN! -> Closing";
#endif
		}
		ok = _M_db.open();
		if (!ok)
        {
#ifdef ULOGDBG
			qDebug() << "------ DB ERROR: " <<  _M_db.lastError();
#endif
            //TODO: what if the connection fails???
        }
	}
	return ok;
}

/*!
  \brief Closes the connection that the QSqlDatabase may have opened
  */
void
UnqlDbHandler::closeDbConn()
{
	/* qDebug() << "closeDbConn() -- db is open (" << _M_db.isOpen() <<") -- db is valid ("
			<< _M_db.isValid() <<") -- db is openError ("<< _M_db.isOpenError() <<")"; */

	if ( !_M_db.isOpen() ) {
			qDebug() << "Database was NOT OPEN! -> Closing anyway";
	}

	_M_db.close();
}

/*!
	\brief this static method creates the DB to hold the events happening in the Q for the Kalliope CallCenter edition
	\param qeventdbname the string containing the full pathname of the queue events DB
	\return a positive integer in case something wrong happened in the creation process 0 otherwise
	This method is called in kctiserver ctor and only if in the .ini file the logQevents flag is set to true.
  */
int
UnqlDbHandler::createLoggerDb(QString qeventdbname)
{
	QCoreApplication *capp = QCoreApplication::instance();
	//capp->addLibraryPath("/usr/lib/qt4/plugins/sqldrivers");
	capp->addLibraryPath("/opt/qt/plugins");
	QSqlDatabase qevdb = QSqlDatabase::addDatabase("QSQLITE","qeventsdb");
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
	if(!ok) {
		qDebug() << "got error: " << q.lastError() << " for query (" << q.lastQuery() << ")";
		return(10);
	}

    q.prepare("\
               CREATE TABLE IF NOT EXISTS  ul_event (\n\
                    id          INTEGER PRIMARY KEY,\n\
                    tstamp      TIMESTAMP,\n\
                    module      VARCHAR(40) NOT NULL,\n\
                    level       INTEGER NOT NULL REFERENCES ul_level(level_value),\n\
                    message		VARCHAR(5000)\
               );");

    ok = q.exec();
    if(!ok) {
        qDebug() << "got error: " << q.lastError() << " for query (" << q.lastQuery() << ")";
        return(10);
    }


    QVariantList vals,names;
    names << "FATAL"
         << "CRITICAL"
         << "WARNING"
         << "INFO"
         << "DEBUG"
         << "FULL DEBUG";

    vals << UNQL::LOG_FATAL
         << UNQL::LOG_CRITICAL
         << UNQL::LOG_WARNING
         << UNQL::LOG_INFO
         << UNQL::LOG_DBG
         << UNQL::LOG_DBG_ALL;

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
    query.bindValue(":lname",module);
	query.bindValue(":tstamp",tstamp);
    query.bindValue(":prio",level);
    query.bindValue(":msg",message);

	ok = query.exec();
	if(!ok) {
		qDebug() << "got error: " << query.lastError() << " for query (" << query.lastQuery() << ")";
	}
    query.clear();
	closeDbConn();
}
