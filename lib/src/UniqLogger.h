/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#ifndef ULOGGER_H
#define ULOGGER_H

#include <QMutex>
#include <QMap>


class RemoteWriter;
class DbWriter;
class FileWriter;
class LogWriter;
class DummyWriter;

class NRThreadPool;

#include "Logger.h"
#include "ConsoleWriter.h"


/*!
  \mainpage

  The UniQLogger library is a Qt based library that allows the logging and monitoring
  functionalities for Qt-based projects. You can create various loggers (with different
  module names and logging times) and associate them to different logwriters so that the
  same messages can be dispatched to various devices (Console, Network or File).

Network writers are clients and (will) support either TCP or UDP connections
Console writers can have different colors
File writers have the ability to use log-rotation

Loggers have different levels of logging and you can be set so that a minimum level has to be achieved
before the actual message gets dispatched to the writers.

It's also possible to define the timestamp format in order to show milliseconds.

Monitoring is another cool feature of the UniQLogger library, it allows to define a map
of variables (identified by a keyword that must be unique per logger) that can be \i turned on
and off by the UniQLogger monitorVar() method.
*/


#ifdef WIN32
 #ifdef ULOG_LIB_EXPORTS
   #define ULOG_LIB_API __declspec(dllexport)
 #else
   #define ULOG_LIB_API __declspec(dllimport)
 #endif
#else
 #define ULOG_LIB_API
#endif

typedef QMap<QString, bool> VarMonitoringMap;
typedef QMap<LogWriter*,int> LogWriterUsageMapType;

class VarMonitorInfo
{
public:
    bool active;
    double value;
};

class ULOG_LIB_API UniqLogger : public QObject
{
    Q_OBJECT

	static QMutex gmuxUniqLoggerInstance;
	static QMap<QString,UniqLogger*> gUniqLoggerInstanceMap;

	QMutex muxDeviceCounter, muxMonitorVarMap;
    QString m_defaultTimeStampFormat;
    QChar m_defaultSpaceChar, m_defaultStartEncasingChar, m_defaultEndEncasingChar;

protected:
        UniqLogger(int nthreads);
        ~UniqLogger();

protected slots:
        void writerFinished(const QList<LogWriter*> aList);

private:
        NRThreadPool *m_pTPool;
        LogWriterUsageMapType m_DevicesMap;
        ConsoleWriter  *m_ConsoleLogger;
        VarMonitoringMap m_VarMonitorMap;

        void registerWriter(LogWriter*);
        void unregisterWriter(LogWriter*);

public:
        static UniqLogger* instance (const QString &ulname="Default UniQLogger", int nthreads = 0);

        Logger* createLogger        ( const QString &logName                                                                      );
        Logger* createConsoleLogger ( const QString &logName, ConsoleColorType c,           const WriterConfig &wc=WriterConfig() );
        Logger* createConsoleLogger ( const QString &logName, bool log2StdConsole=true,     const WriterConfig &wc=WriterConfig() );
        Logger* createFileLogger    ( const QString &logName, const QString &inFileName,    const WriterConfig &wc=WriterConfig() );
        Logger* createDummyLogger   ( const QString &logName,                               const WriterConfig &wc=WriterConfig() );
        Logger* createNetworkLogger ( const QString &logName, const QString &addr,int port, const WriterConfig &wc=WriterConfig() );
        Logger* createDbLogger      ( const QString &logName, const QString &aDbFileName,   const WriterConfig &wc=WriterConfig() );

        LogWriter& getDbWriter        ( const QString &aDbFileName          );
        LogWriter& getNetworkWriter   ( const QString &address, int port    );
        LogWriter& getFileWriter      ( const QString &inFileName           );
        LogWriter& getConsoleWriter   ( ConsoleColorType c=NONE             );
        LogWriter& getStdConsoleWriter();
        LogWriter& getDummyWriter     ();

        void addMonitorVar ( const QString &var, bool initialStatus=false );
        void changeMonitorVarStatus ( const QString &var, bool status );
        void delMonitorVar ( const QString &var );

		int addWriterToLogger       (const Logger*, const LogWriter&);
		int removeWriterFromLogger  (const Logger*, const LogWriter&);

        void setEncasingChars       ( const QChar& aStartChar, const QChar &aEndChar         );
        void setSpacingChar         ( const QChar& aSpaceChar                                );
        void setTimeStampFormat		( const QString&	                                     );
        void setStdConsoleColor     ( ConsoleColorType c        							 );
};

#endif // ULOGGER_H
