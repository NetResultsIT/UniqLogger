/********************************************************************************
 *   Copyright (C) 2008-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#ifndef __EXTERNAL_LOGGER_INCS__
#define __EXTERNAL_LOGGER_INCS__

#include <QThread>
#include <QStringList>
#include <QMutex>
#include <QTimer>
#include <QDebug>

#include "LogMessage.h"
#include "FileCompressor.h"

#ifdef WIN32
 #ifdef ULOG_LIB_EXPORTS
   #define ULOG_LIB_API __declspec(dllexport)
 #else
   #define ULOG_LIB_API __declspec(dllimport)
 #endif
#else
 #define ULOG_LIB_API
#endif



enum FileRotationPolicyType {
    StrictRotation,     // Rotate over single file
    IncrementalNumbers, // New logs are inserted in file with lower numbers and all old logs are moved / renamed to files with a higher number (similar to logrotate)
    HigherNumbersNewer  // New logs are inserted in files with a higher number, no renaming / moving of files
};

enum TimeRotationPolicyType {
    NoTimeRotation,     /*!< Do not perform any time-based rotation */
    HourlyRotation,     /*!< Rotates over day of the month with suffixes like: h01, h12, h23, etc.*/
    DayOfWeekRotation,  /*!< Rotates over day of the week with suffixes like: Mon, Tue, etc. */
    DayOfMonthRotation  /*!< Rotates over day of the month with suffixes like: d01, d12, d23, etc. */
};

class ULOG_LIB_API WriterConfig
{
public:
    explicit WriterConfig();

    /*!
     * \brief WriterConfig
     * Usefull constructor to initialize inline a WriterConfig for a FileWriter
     *
     * \param i_maxFileSize
     * \param i_maxFileNum
     * \param i_rotationPolicy
     * \param compressionLevel
     * \param compressionAlgo
     */
    explicit WriterConfig(int i_maxFileSize,
                          int i_maxFileNum,
                          FileRotationPolicyType i_rotationPolicy = StrictRotation,
                          int i_compressionLevel = 0,
                          int i_compressionAlgo = FileCompressor::ZIP_FILE);

    //Common
    int maxMessageNum;      /// Maximum number of still unwritten messages that the writer will hold before discarding, 0 means infinite
    int writerFlushSecs;    /// The number of seconds between a write operation from the LogWriter
    bool writeIdleMark;     /// Whether the LogWriter should add a MARK string to show it's alive in case nothing is being logged

    //Makes sense just for FileWriter
    int maxFileSize;        /// The maximum file size (in MB) of a file written by the FileWriter
    int maxFileNum;         /// The number of files that FileWriter should use during rotation, 0 disables it
    FileRotationPolicyType rotationPolicy;     /// The rotation policy enum (StrictRotation or IncrementalNumbers)
    /* file log compression specific vars */
    int compressionLevel;   /// The compression level used to compress the rotated log files
                            /// 0: do not compress
                            /// 1: min compression level (faster)
                            /// 9: max compression level (slower)
                            /// default: 0 - do not compress
    int compressionAlgo;    /// Algorithm used to compress (gz or zip): default value is zip


    //Makes sense just for RemoteWriter
    int reconnectionSecs;   /// The number of seconds a RemoteWriter will wait before trying to reconnect to its server
};

class LogWriter: public QObject
{
    Q_OBJECT

    QString m_lastMessage;
    volatile bool m_stillClosing;
    int m_maxMessages;
    bool m_writeIdleMark;

protected:
    QTimer *m_logTimer;
    QList<LogMessage> m_logMessageList;
    int m_sleepingMilliSecs;
    bool m_logIsPaused;
    QMutex mutex;

protected slots:
    virtual void writeToDevice()=0;
    void priv_writeToDevice();
    void priv_startLogTimer(); // <-- BEWARE this method is called with invokeMethod do NOT change its name

public:
    LogWriter();
    virtual ~LogWriter();

    void setSleepingMilliSecs(int);
    void setMaximumAllowedMessages(int);
    void appendMessage(const LogMessage&);
    virtual void run();
    void pauseLogging(bool);
    void flush();
    virtual void setWriterConfig( const WriterConfig &wconf);
};

#endif

