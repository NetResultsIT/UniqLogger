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

namespace UNQL {

enum FileRotationNamingPolicyType {
    StrictRotation,     /*!< New logs are inserted in file with lower numbers and all old logs are moved / renamed to files with a higher number (similar to logrotate) */
    HigherNumbersNewer  /*!< New logs are inserted in files with a higher number, no renaming / moving of files */
};

enum FileRotationTimePolicyType {
    NoTimeRotation,     /*!< Do not perform any time-based rotation */
    DailyRotation,      /*!< Rotates to a new log file every day at midnight with suffix like log-2021-03-16T00:00:00 */
    HourlyRotation,     /*!< Rotates to a new log file at the beginning of each hour with suffix like log-2021-03-16T02:00:00 */
    PerMinuteRotation,  /*!< Rotates to a new log file at the beginning of each minute with suffix like log-2021-03-16T02:12:00 */
    ElapsedMinutesRotation,  /*!< Rotates to a new log file when the time specified in WriterConfig::maxMinutes elapses */
};

enum NetworkProtocolType {
    TCP,
    UDP,
    TLS
};

}

/*!
 * \brief The WriterConfig class provides configuration settings for all LogWriter subclasses
 */
class ULOG_LIB_API WriterConfig
{
public:
    explicit WriterConfig();
    QString toString() const;
    bool neededSanitizing() const;
    bool operator ==(const WriterConfig &rhs) const;
    bool operator !=(const WriterConfig &rhs) const;

    //Common
    int maxMessageNum;      /*!< Maximum number of still unwritten messages that the writer will hold before discarding, 0 means infinite */
    int writerFlushSecs;    /*!< The number of seconds between a write operation from the LogWriter */
    bool writeIdleMark;     /*!< Whether the LogWriter should add a MARK string to show it's alive in case nothing is being logged */
    bool compressMessages;  /*!< If true, compress all identical and consecutive messages on a single line */

    //Makes sense just for FileWriter
    int maxFileSize;        /*!< The maximum file size (in MB) of a single file written by the FileWriter */
    int maxFileNum;         /*!< The number of files that FileWriter should use during rotation, 0 disables it */
    UNQL::FileRotationNamingPolicyType rotationPolicy;   /*!< The naming convention to distinguish log files */
    UNQL::FileRotationTimePolicyType timeRotationPolicy; /*!< Indicate if files should be rotated on a time policy i.e.: hourly, daily */
    /* file log compression specific vars */
    int compressionLevel;   /*!< The compression level used to compress the rotated log files ranges from 0 to 9
                                0: simple storage
                                1: min compression level (faster)
                                9: max compression level (slower)
                                default: 0 - do not compress */
    int compressionAlgo;    /*!< Algorithm used to compress (gzip or zip):
                                0: No compression (default)
                                1: Creates a gzip archive
                                2: Creates a zip archive */
    int maxMinutes; /*!< The maximum number of minutes that can be included in a logfile, this value is considered \em only when ElapsedMinutesRotation is set */


    //Makes sense just for RemoteWriter
    int reconnectionSecs;   /*!< The number of seconds a RemoteWriter will wait before trying to reconnect to its server */
    UNQL::NetworkProtocolType netProtocol; /*!< The protocol used to connect to remote log server (defaults to TCP) */
};

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

