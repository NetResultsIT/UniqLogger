/********************************************************************************
 *   Copyright (C) 2010-2014 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#ifndef __INTERNAL_LOGGER_INCS__
#define __INTERNAL_LOGGER_INCS__

#include <QStringList>
#include <QObject>
#include <QMutex>
#include <QMap>
#include <QVariant>

#include "LogMessage.h"

#define ERRMSG_SIZE 5000

class UniqLogger;
class Logger;
class LogWriter;

#ifdef WIN32
 #ifdef ULOG_LIB_EXPORTS
   #define ULOG_LIB_API __declspec(dllexport)
 #else
   #define ULOG_LIB_API __declspec(dllimport)
 #endif
#else
 #define ULOG_LIB_API
#endif

class ULOG_LIB_API Logger: public QObject
{
	Q_OBJECT

    int m_logVerbosityAcceptableLevel, m_logVerbosityDefaultLevel, m_logVerbosityCurrentLevel;

	char buffer[ERRMSG_SIZE];
    QString m_moduleName, m_errorPrefix, m_timeStampFormat, m_spacingString;
	QStringList m_bufferedStreamMessages;
    QChar m_startEncasingChar, m_endEncasingChar;

    mutable QList<LogWriter*> m_logDeviceList;
	QMap<QString,bool>* m_varMonitorMap;
	QMutex *muxMonitorVar;
    QMutex muxMessages;

    void dispatchMessage(const LogMessage &);
	void dispatchBufferedMessages();
    void priv_log(int loglevel, const QString& message);
    UNQL::LogMessagePriorityType selectCorrectLogLevel(int chosenPriority) const;

	//we define UniqLogger to be friend so it can:
	//* set the map and its mutex
	//* access ctor and addLogDevice() and removeLogDevice()
	friend class UniqLogger;

signals:
	void writersToDelete(const QList<LogWriter*>);

protected:
	Logger();
	int addLogDevice	( LogWriter * ) const;
	int removeLogDevice	( LogWriter * ) const;

public:

    virtual ~Logger();

	void setVerbosityLevel			( const int&		);
	void setVerbosityDefaultLevel	( const int&		);
	void setModuleName				( const QString&	);
	void setTimeStampFormat			( const QString&	);

    void setSpacingString           ( const QString &      );
    void setEncasingChars           ( const QChar&, const QChar& );

	void log	( int, const char*, ... );
	void log	( const char*, ...		);
    void flush();
	void monitor( const QVariant &data, const QString &key, const QString &desc="");

    Logger& operator<< ( const UNQL::LogMessagePriorityType& d		);
    Logger& operator<< ( const UNQL::LogStreamManipType& d          );
    Logger& operator<< ( const double& d				);
    Logger& operator<< ( const QStringList& sl          );
    Logger& operator<< ( const QList<int>& vl           );
    Logger& operator<< ( const QMap<int, QList<int> >&  );
    Logger& operator<< ( const QVariant& v				);
};

#endif

