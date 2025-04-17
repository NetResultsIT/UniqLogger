/********************************************************************************
 *   Copyright (C) 2010-2018 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
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
class ConsoleWriter;
class WriterConfig;

class NRThreadPool;

#include "Logger.h"
#include "LogWriter.h"
#include "ConsoleColorScheme.h"


/*!
  \mainpage

    The UniQLogger library is a Qt based library that allows the logging and monitoring
    functionalities for Qt-based projects. You can create various loggers (with different
    module names and logging times) and associate them to different logwriters so that the
    same messages can be dispatched to various devices (Console, Network or File).

    Network writers are clients over TCP connections
    Console writers can have different colors
    File writers have the ability to use log-rotation

    Loggers have different levels of logging and you can be set so that a minimum level has to be achieved
    before the actual message gets dispatched to the writers.

    It's also possible to define the timestamp format in order to show milliseconds.

    Monitoring is another cool feature of the UniQLogger library, it allows to define a map
    of variables (identified by a keyword that must be unique per logger) that can be \em turned on
    and off by the UniQLogger monitorVar() method.
*/


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
    static int DEFAULT_OK;

    QMutex muxDeviceCounter, muxMonitorVarMap;
    QString m_defaultTimeStampFormat;
    static constexpr QString DefaultLogTag = 1;
    QChar m_defaultSpaceChar, m_defaultStartEncasingChar, m_defaultEndEncasingChar;
    UNQL::LogMessagePriorityType m_defaultLogLevel;

protected:
    UniqLogger(int nthreads);
    ~UniqLogger();

protected slots:
    void writerFinished(const QList<LogWriter*> aList);

private:
        NRThreadPool *m_pTPool;
        LogWriterUsageMapType m_DevicesMap;
        VarMonitoringMap m_VarMonitorMap;

        void registerWriter(LogWriter*);
        void unregisterWriter(LogWriter*);

        Logger* createLogger        (const QString &logName);
        bool checkMatchingConfigForWriter(LogWriter &w, const WriterConfig & wc);

public:
        static UniqLogger* instance (const QString &ulname="UNQL", int nthreads = 1);

        Logger* createDummyLogger   ( const QString &logName,                                   const WriterConfig &wc=WriterConfig() );
        Logger* createConsoleLogger ( const QString &logName, UNQL::ConsoleColorScheme scheme=UNQL::ConsoleColorScheme(), const WriterConfig &wc=WriterConfig() );
        Logger* createFileLogger    ( const QString &logName, const QString &fileName,          const WriterConfig &wc=WriterConfig(), int &ok=DEFAULT_OK );
        Logger* createNetworkLogger ( const QString &logName, const QString &address, int port, const WriterConfig &wc=WriterConfig(), int &ok=DEFAULT_OK );
        Logger* createRSyslogLogger ( const QString &logName, const QString &i_id, quint8 i_facility, const QString &address, int port, const WriterConfig &wc=WriterConfig(), int &ok=DEFAULT_OK );
        Logger* createDbLogger      ( const QString &logName, const QString &aDbFileName,       const WriterConfig &wc=WriterConfig(), int &ok=DEFAULT_OK  );
        Logger* createAndroidLogger ( const QString &logName, const WriterConfig &wc=WriterConfig()                                                        );

        LogWriter& getDbWriter        ( const QString &aDbFileName,             const WriterConfig &wc=WriterConfig(), int &ok=DEFAULT_OK );
        LogWriter& getNetworkWriter   ( const QString &address, int port,       const WriterConfig &wc=WriterConfig(), int &ok=DEFAULT_OK );
        LogWriter& getRSyslogWriter   ( const QString &i_id, quint8 i_facility, const QString &address, int port,  const WriterConfig &wc=WriterConfig(), int &ok=DEFAULT_OK );
        LogWriter& getFileWriter      ( const QString &fileName,                const WriterConfig &wc=WriterConfig(), int &ok=DEFAULT_OK );
        LogWriter& getConsoleWriter   ( UNQL::ConsoleColorScheme i_colorScheme, const WriterConfig &wc=WriterConfig(), int &ok=DEFAULT_OK );
        LogWriter& getDummyWriter     ();
        LogWriter& getAndroidWriter   (const WriterConfig &wc=WriterConfig());

        void addMonitorVar ( const QString &var, bool initialStatus=false );
        void changeMonitorVarStatus ( const QString &var, bool status );
        void delMonitorVar ( const QString &var );

        int addWriterToLogger       (const Logger*, const LogWriter &);
        int removeWriterFromLogger  (const Logger*, const LogWriter&);

        int threadsUsedForLogging() const;

        void setEncasingChars       ( const QChar& aStartChar, const QChar &aEndChar        );
        void setSpacingChar         ( const QChar& aSpaceChar                               );
        void setTimeStampFormat     ( const QString&                                        );
        void setDefaultLogLevel     (UNQL::LogMessagePriorityType loglevel                  );

        void flushAllWriters();
};

#endif // ULOGGER_H
