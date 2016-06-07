/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#include "Logger.h"
#include "LogWriter.h"

#include <QStringList>
#include <QDateTime>
#include <QMutexLocker>

/*!
  */
Logger::Logger()
{
    m_logDeviceList.clear();
    m_timeStampFormat = DEF_UNQL_TS_FMT;
    m_moduleName = "Generic Module";
    m_logVerbosityAcceptedLevel = UNQL::LOG_INFO;
    m_logVerbosityDefaultLevel = UNQL::LOG_INFO;
    m_spacingString = ' ';
    m_startEncasingChar = '[';
    m_endEncasingChar = ']';
}

Logger::~Logger()
{

#ifdef ULOGDBG
    qDebug() << Q_FUNC_INFO << "Deleting logger " << m_moduleName;
#endif
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
        return (int) m_bufferedStreamMessages[QThread::currentThread()].priority();
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
  \param s is the string reprensenting the timestamp format, the default is hh:mm:ss (hour:minutes:seconds)
  \note for valid format see Qt::QDateTime valid formats. Most common is: yyyy-MM-dd hh:mm:ss.zzz
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
  \param spChar is the spacing character that separates the module names and priority levels, the default is ' '
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
{ m_moduleName = s; }



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
        if (m_logDeviceList.at(i) == 0)
            qWarning() << "NULL WRITER";
        else
            m_logDeviceList.at(i)->appendMessage(m);
    }
}



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
#ifdef ULOGDBG
       qDebug()  << key << " is in the map, checking its status...";
#endif
	   status = (*m_varMonitorMap)[key];
#ifdef ULOGDBG
       qDebug() << key << " monitor status is " << status;
#endif
    }
    else {
#ifdef ULOGDBG
        qDebug() << key << " is not in the map... adding it";
#endif
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
#ifdef ULOGDBG
        qDebug() << "not monitoring " << key << " is not enabled in the map";
#endif
    }
	muxMonitorVar->unlock();
}



void
Logger::priv_log(int priority, const QString &msg)
{
    UNQL::LogMessagePriorityType lev = selectCorrectLogLevel(priority);

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
    else if (chosenPriority >= UNQL::LOG_WARNING)
        loglevel = UNQL::LOG_WARNING;
    else if (chosenPriority >= UNQL::LOG_CRITICAL)
        loglevel = UNQL::LOG_CRITICAL;
    else if (chosenPriority >= UNQL::LOG_FATAL)
        loglevel = UNQL::LOG_FATAL;

    return loglevel;
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
    if (priority <= m_logVerbosityAcceptedLevel)
    {
        va_list args;
        va_start(args,mess);

#if defined(_MSC_VER) && _MSC_VER < 1400
        _vsnprintf(buffer,UNQL_ERRMSG_SIZE,mess,args);
#else
        vsnprintf(buffer,UNQL_ERRMSG_SIZE,mess,args);
#endif

        QString msg = buffer;
        priv_log(priority, msg);

        va_end(args);
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
    _vsnprintf(buffer2,UNQL_ERRMSG_SIZE,mess,args);
#else
    vsnprintf(buffer2,UNQL_ERRMSG_SIZE,mess,args);
#endif
	va_end(args);

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
    if ( bos.priority() != UNQL::LOG_OFF
         && bos.priority() <= m_logVerbosityAcceptedLevel
         && bos.count() > 0 )
    {
        s = bos.list().join(m_spacingString);
        this->priv_log(bos.priority(), s);
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

Logger&
Logger::operator<< ( const QList<int>& vl )
{
    muxMessages.lock();
    foreach (int v,vl) {
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(v));
    }
    muxMessages.unlock();
    return *this;
}

Logger&
Logger::operator<< ( const QMap<int, QList<int> >& amap )
{
    foreach (int mk, amap.keys()) {
        QString liststr = "(";
        foreach (int v, amap.value(mk)) {
            liststr.append(QString::number(v));
        }
        liststr += ")";
        muxMessages.lock();
            m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(mk) + " -> " + liststr);
        muxMessages.unlock();
    }
    return *this;
}

Logger&
Logger::operator<< ( const double& d )
{
    muxMessages.lock();
        m_bufferedStreamMessages[QThread::currentThread()].append(QString::number(d));
    muxMessages.unlock();
    return *this;
}
