/********************************************************************************
 *   Copyright (C) 2008-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#ifndef UNQL_EXTERNAL_LOGGER_INCS
#define UNQL_EXTERNAL_LOGGER_INCS

#include <QThread>
#include <QStringList>
#include <QMutex>
#include <QTimer>
#include <QDebug>

#include "LogMessage.h"
#include "WriterConfig.h"

class LogWriter: public QObject
{
    Q_OBJECT

    QString m_lastMessage;

protected:
    QTimer *m_pLogTimer;
    QList<LogMessage> m_logMessageList;
    WriterConfig m_Config;
    bool m_logIsPaused;
    QMutex mutex;

    void compressMessages();

protected slots:
    virtual void writeToDevice()=0;
    void priv_writeToDevice();
    void priv_startLogTimer(); // <-- BEWARE this method is called with invokeMethod do NOT change its name

public:
    LogWriter(const WriterConfig &);
    virtual ~LogWriter();

    const WriterConfig& getWriterConfig() const;
    bool isAlreadyConfigured() const;
    bool isLoggingPaused() const;

    /* SETTERs */
    void setSleepingMilliSecs(int);
    void setMaximumAllowedMessages(int);
    void appendMessage(const LogMessage&);
    virtual void run();
    void pauseLogging(bool);
    void flush();
    virtual void setWriterConfig( const WriterConfig &wconf);
};

#endif

