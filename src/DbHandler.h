#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <QSqlDatabase>

class UnqlDbHandler
{
    QString _M_DbHost, _M_DbName, _M_DbUsername, _M_DbPasswd;
	QSqlDatabase _M_db;
	int _M_DbPort;

	bool openDbConn();
	void closeDbConn();

public:
    explicit UnqlDbHandler(QString inDbName);
    explicit UnqlDbHandler(QString inDbName, QString inDbUser, QString inDbPwd, QString inDbHost, int inDbPort=3306);
    ~UnqlDbHandler();

    void logEvent(const QString &module, const QString &tstamp, const QString &level, const QString &message);
	static int createLoggerDb(QString qeventdbname);

};

#endif // DBHANDLER_H
