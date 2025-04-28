/********************************************************************************
 *   Copyright (C) 2010-2021 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#include "Logger.h"
#include "LogWriter.h"

#include <QStringList>
#include <QDateTime>
#include <QMutexLocker>

#include <iostream>

/*!
  */
Logger::Logger()
{
    m_logDeviceList.clear();
    m_timeStampFormat = DEF_UNQL_TSTAMP_FMT;
    m_moduleName = "Generic Module";
    m_logVerbosityAcceptedLevel = UNQL::LOG_INFO;
    m_logVerbosityDefaultLevel = UNQL::LOG_INFO;
    m_spacingString = ' ';
    m_startEncasingChar = '[';
    m_endEncasingChar = ']';
    m_printToQDebug = false;
    m_printToStdOut = false;
    m_printThreadID = false;
}

Logger::~Logger()
{
    ULDBG << Q_FUNC_INFO << "Deleting logger " << m_moduleName;

    //before emptying the list, notify it to uniqlogger
    emit writersToDelete(m_logDeviceList);
    m_logDeviceList.clear();
}



/*!
 * \brief Logger::getVerbosityLevel
 * \return the current logging priority level of this logger (for the calling thread)
 */
int
Logger::getVerbosityLevel() const
{
    if (m_bufferedStreamMessages.contains(QThread::currentThread())) {
        return static_cast<int>(m_bufferedStreamMessages[QThread::currentThread()].priority());
    }
    return m_logVerbosityDefaultLevel;
}




/*!
  \brief gets the verbosity level of this logger
  */
int
Logger::getVerbosityAcceptedLevel() const
{ return m_logVerbosityAcceptedLevel; }



/*!
  \brief sets the verbosity level of this logger
  \param v the value that must be reached before the actual logging take place
  */
void
Logger::setVerbosityAcceptedLevel(const int &v)
{ m_logVerbosityAcceptedLevel = v; }




/*!
  \brief gets the default verbosity level of this logger
  */
int
Logger::getVerbosityDefaultLevel() const
{ return m_logVerbosityDefaultLevel; }


/*!
  \brief sets the verbosity default level of this logger
  \param v the value that must be reached before the actual logging take place
  The default value takes place whenever during the log() function call no level is specified
  */
void
Logger::setVerbosityDefaultLevel(const int &v)
{ m_logVerbosityDefaultLevel = v; }



/*!
  \brief change the timestamp format for this logger
  \param s is the string reprensenting the timestamp format, the default is HH:mm:ss (hour:minutes:seconds)
  \note for valid format see Qt::QDateTime valid formats. Most common is: yyyy-MM-dd HH:mm:ss.zzz
  */
void
Logger::setTimeStampFormat(const QString &s)
{ m_timeStampFormat = s; }


/*!
  \brief change the starting end ending characters that are used for this logger
  \param startChar is the starting character that encases the module names and priority levels, the default is '['
  \param endChar is the ending character that encases the module names and priority levels, the default is ']'
  */
void
Logger::setEncasingChars(const QChar &startChar, const QChar &endChar)
{
    m_startEncasingChar = startChar;
    m_endEncasingChar = endChar;
}



/*!
  \brief change the spacing character that is used for this logger
  \param spStr is the spacing string that separates the module names and priority levels, the default is ' '
  */
void
Logger::setSpacingString(const QString &spStr)
{ m_spacingString = spStr; }



/*!
  \brief sets the name for this logger, as it will appear in the log messages
  \param s the logger modulename
  */
void
Logger::setModuleName(const QString &s)
{
    log(UNQL::LOG_INFO, "Changing logger name from %s to %s", m_moduleName.toLatin1().constData(), s.toLatin1().constData());
    m_moduleName = s;
}



/*!
  \brief adds a new logwriter to this logger
  \param el the pointer to a new LogWriter
  \return a negative value if an error occurred, 0 otherwise
  this method will add a new LogWriter to which this logger will dispatch its messages
  */
int
Logger::addLogDevice(LogWriter *el)
{
    QMutexLocker lock(&muxDevices);
    if (m_logDeviceList.contains(el))
        return -1;

    m_logDeviceList.append(el);
    return 0;
}


/*!
 * \brief Logger::removeLogDevice removes a logwriter from this logger
 * \param el the pointer to the writer we want to remove
 * \return a negative value if an error occurred, 0 otherwise
  this method will remove a LogWriter from the ones this logger dispatches its messages
 */
int
Logger::removeLogDevice(LogWriter *el)
{
    QMutexLocker lock(&muxDevices);
    if (!m_logDeviceList.contains(el))
        return -1;

    m_logDeviceList.removeAll(el);
    return 0;
}
 

void
Logger::dispatchMessage(const LogMessage &m)
{
    QMutexLocker lock(&muxDevices);
    for (int i=0; i<m_logDeviceList.count(); i++)
    {
        if (m_logDeviceList.at(i) == nullptr)
            qWarning() << "NULL WRITER";
        else
            m_logDeviceList.at(i)->appendMessage(m);
    }
}


/*!
 * \brief Logger::flush forces this Logger to write all its messages to the connected writers
 */
void
Logger::flush()
{
    this->dispatchBufferedMessages();

    QMutexLocker lock(&muxDevices);
    for (int i=0; i<m_logDeviceList.count(); i++)
    {
        m_logDeviceList.at(i)->flush();
    }
}



void
Logger::monitor(const QVariant &d, const QString &key, const QString &desc)
{
    bool status = false;

    muxMonitorVar->lock();
    if (m_varMonitorMap->contains(key)) {
        ULDBG  << key << " is in the map, checking its status...";
        status = (*m_varMonitorMap)[key];
        ULDBG << key << " monitor status is " << status;
    }
    else {
        ULDBG << key << " is not in the map... adding it";
        m_varMonitorMap->insert(key, false);
    }

    if (status) {
        LogMessage lm(m_moduleName, UNQL::LOG_MONITOR, desc + ": " + d.toString(),
                      QDateTime::currentDateTime().toString(m_timeStampFormat));

        QPair<QChar,QChar> encasingPair(m_startEncasingChar, m_endEncasingChar);
        LogMessageFormatting lmf(m_spacingString, encasingPair);
        lm.setFormatting(lmf);
        dispatchMessage(lm);
    }
    else {
        ULDBG << "not monitoring " << key << " is not enabled in the map";
    }
    muxMonitorVar->unlock();
}


QString Logger::priv_addLogTag(const QString &i_msg)
{
    QString msg = i_msg;
    if (Q_UNLIKELY(m_printLogTag)) {
        msg = QString("%1 %2").arg(m_logTag).arg(i_msg);
    };

    return msg;
}


/*!
 * \brief Logger::priv_addThreadPointer adds the thread pointer to the message
 * \param i_msg the message that should be modified to include the thread pointer
 * \return a new string with the thread pointer included
 */
QString Logger::priv_addThreadPointer(const QString &i_msg)
{
    QString msg = i_msg;
    if (Q_UNLIKELY(m_printThreadID)) {
        msg = QString("(Th. 0x%1) %2").arg((quint64)QThread::currentThread(), 0, 16).arg(i_msg);
    };

    return msg;
}


/*!
  \brief log a message with a given priority
  \param i_priority the priority of the message
  \param i_msg the message to be logged
  */
void
Logger::priv_log(int i_priority, const QString &i_msg)
{
    UNQL::LogMessagePriorityType lev = selectCorrectLogLevel(i_priority);


    QString msg = priv_addThreadPointer(i_msg);

    //This should be the last addition since it is printed at beginning of string
    msg = priv_addLogTag(msg);

    QPair<QChar,QChar> encasingPair(m_startEncasingChar, m_endEncasingChar);
    LogMessageFormatting lmf(m_spacingString, encasingPair);
    LogMessage lm(m_moduleName, lev, msg, QDateTime::currentDateTime().toString(m_timeStampFormat));

    lm.setFormatting(lmf);
    dispatchMessage(lm);
}



UNQL::LogMessagePriorityType
Logger::selectCorrectLogLevel(int chosenPriority) const
{
    UNQL::LogMessagePriorityType loglevel = UNQL::LOG_OFF;

    if (chosenPriority >= UNQL::LOG_DBG_ALL)
        loglevel = UNQL::LOG_DBG_ALL;
    else if (chosenPriority >= UNQL::LOG_DBG)
        loglevel = UNQL::LOG_DBG;
    else if (chosenPriority >= UNQL::LOG_INFO)
        loglevel = UNQL::LOG_INFO;
    else if (chosenPriority >= UNQL::LOG_NOTICE)
        loglevel = UNQL::LOG_NOTICE;
    else if (chosenPriority >= UNQL::LOG_WARNING)
        loglevel = UNQL::LOG_WARNING;
    else if (chosenPriority >= UNQL::LOG_ERROR)
        loglevel = UNQL::LOG_ERROR;
    else if (chosenPriority >= UNQL::LOG_CRITICAL)
        loglevel = UNQL::LOG_CRITICAL;
    else if (chosenPriority >= UNQL::LOG_ALARM)
        loglevel = UNQL::LOG_ALARM;
    else if (chosenPriority >= UNQL::LOG_EMERGENCY)
        loglevel = UNQL::LOG_EMERGENCY;
    else if (chosenPriority >= UNQL::LOG_FATAL)
        loglevel = UNQL::LOG_FATAL;

    return loglevel;
}


/*!
 * \brief Logger::printToStdOut will enable or disable the printout of logged message to stdout
 * \param enable the new status of the std::cout printout
 * \note the printing will \em not take log priority into account so \em every message will be printed to std::cout
 * The purpose of this function is to cleanup code during debugging and allow programmers see all the messages that might go into log files
 * directly on console
 */
void Logger::printToStdOut(bool enable)
{ m_printToStdOut = enable; }


/*!
 * \brief Logger::printToQDebug
 * \param enable the new status of the qDebug() printout
 * \note the printing will \em not take log priority into account so \em every message will be printed with qDebug()
 * The purpose of this function is to cleanup code during debugging and allow programmers see all the messages that might go into log files
 * directly on console
 */
void Logger::printToQDebug(bool enable)
{ m_printToQDebug = enable; }


/*!
 * \brief Logger::printThreadID
 * \param enable the new status of the thread ID printing
 * The purpose of this method is to enable the logging of the thread ID that is currently executing the log message
 */
void Logger::printThreadID(bool enable)
{ m_printThreadID = enable; }


/*!
 * \brief Logger::printAlsoToConsoleIfRequired will print out to std::cout and/or with qDebug() (that is std::cerr but with different timing due to buffering)
 * \param mess the QString containing the message to print
 */
void Logger::printAlsoToConsoleIfRequired(const QString &mess)
{
    QString m = priv_addThreadPointer(mess);

    //This should be the last addition since it is printed at beginning of string
    m = priv_addLogTag(m);

    if (m_printToQDebug)
        qDebug() << m;
    if (m_printToStdOut)
        std::cout << m.toStdString() << std::endl;
}

/*!
  \brief this is the main method called to log a message on this logger module
  \param priority is the priority level (and string) that will be logged along with the message
  \param mess is the C string containing the message to be logged
  This function uses variadic argument so that the message can be in the printf() formatting
  */
void
Logger::log(int priority, const char* mess, ...)
{
    printAlsoToConsoleIfRequired(mess);

    if (priority <= m_logVerbosityAcceptedLevel || priority == UNQL::LOG_FORCED)
    {
        char buffer[UNQL_ERRMSG_SIZE];

        va_list args;
        va_start(args, mess);

#if defined(_MSC_VER) && _MSC_VER < 1400
        _vsnprintf(buffer, UNQL_ERRMSG_SIZE-1, mess, args);
#else
        vsnprintf(buffer, UNQL_ERRMSG_SIZE-1, mess, args);
#endif
        buffer[UNQL_ERRMSG_SIZE-1] = '\0';
        QString msg = buffer;
        priv_log(priority, msg);

        va_end(args);
    }
    else {
        ULDBG << "Won't log message " << mess << "because it's priority ("
              << priority << ") was not high enough for this logger (" << m_logVerbosityAcceptedLevel << ")";
    }
}



/*!
  \brief this is an overloaded method to allow logging with the default priority level
  \param mess the message to be logged
  Since the message can be in printf() format we have to parse the variadic args and then use the base log() function passing the already
  reconstructed text message.
  */
void
Logger::log(const char* mess, ...)
{
    char buffer2[UNQL_ERRMSG_SIZE];
    va_list args;

    va_start(args,mess);
#if defined(_MSC_VER) && _MSC_VER < 1400
    _vsnprintf(buffer2, UNQL_ERRMSG_SIZE-1, mess, args);
#else
    vsnprintf(buffer2, UNQL_ERRMSG_SIZE-1, mess, args);
#endif
    va_end(args);
    buffer2[UNQL_ERRMSG_SIZE-1] = '\0';

    this->log(m_logVerbosityDefaultLevel, buffer2);
}



void
Logger::dispatchBufferedMessages()
{
    QString s;
    BufferOfStrings bos;

    muxMessages.lock();
    bos = m_bufferedStreamMessages.take(QThread::currentThread());
    //qDebug() << "buffer of thread" << QThread::currentThread() << "has " << bos.count() << "messages" << bos.list();
    if (bos.count() > 0) {
        s = bos.list().join(m_spacingString);
        printAlsoToConsoleIfRequired(s);
        if ( bos.priority() != UNQL::LOG_OFF
             && bos.priority() <= m_logVerbosityAcceptedLevel
        ) {

            this->priv_log(bos.priority(), s);
        }
    }
    muxMessages.unlock();
}


Logger&
Logger::operator<< ( const UNQL::LogMessagePriorityType& lmt )
{
    muxMessages.lock();
        BufferOfStrings bos = m_bufferedStreamMessages[QThread::currentThread()];
    muxMessages.unlock(); // <-- We need to unlock since dispatchBufferedMessages() will try to acquire the mutex

    if(lmt != bos.priority()) {
        dispatchBufferedMessages();
    }

    bos.setPriority(lmt);

    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()] = bos;
    muxMessages.unlock();

    return *this;
}



Logger&
Logger::operator<< ( const UNQL::LogStreamManipType& lsm )
{
    dispatchBufferedMessages();
    switch(lsm) {
        case UNQL::fls:
            this->flush();
            break;
        case UNQL::eom:
        default:
            //do nothing
        break;
    }

    return *this;
}


Logger&
Logger::operator<< ( const QVariant& v )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(v.toString());
    muxMessages.unlock();
    return *this;
}


Logger&
Logger::operator<< ( const QStringList& sl )
{
    muxMessages.lock();
    foreach (QString s,sl) {
        m_bufferedStreamMessages[QThread::currentThread()].append(s);
    }
    muxMessages.unlock();
    return *this;
}

/* Previous implementation is still here commented since there was bit of a hack for android that need to be tested
Logger&
Logger::operator<< ( double d )
{
#ifdef Q_OS_ANDROID
    QString s = "%1.%2";

    quint64 integral = static_cast<quint64>(d);
    quint16 decimal = static_cast<quint16>((d - integral)*1000);
    s = s.arg(integral).arg(decimal);

    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(s);
    muxMessages.unlock();
#else
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(d));
    muxMessages.unlock();
#endif
    return *this;
}
*/

Logger&
Logger::operator<< ( unsigned long d )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(d));
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< ( signed long d )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(d));
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< ( unsigned int d )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(d));
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< ( signed int d )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(d));
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< (quint64 d )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(d));
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< ( qint64 d )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(d));
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< ( double d )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(d));
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< ( bool b )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(b ? "true" : "false");
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< ( const char* s )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString(s));
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< ( char c )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QChar(c));
    muxMessages.unlock();
    return *this;
}

