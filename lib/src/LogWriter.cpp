/********************************************************************************
 *   Copyright (C) 2010-2018 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#include "LogWriter.h"
#include "NrFileCompressor.h"

#include <QStringList>
#include <QDateTime>


/******************
 *                *
 *  WriterConfig  *
 *                *
 ******************/

WriterConfig::WriterConfig()
    : maxMessageNum      ( 0 )                        // unlimited
    , writerFlushSecs    ( 5 )                        // each writer will flush data every 5 seconds
    , writeIdleMark      ( false )                    // If no messages are to be written, write a MARK string to show writer is alive
    , maxFileSize        ( 0 )                        // unlimited MB size of logfile
    , maxFileNum         ( 1 )                        // log on just one file by default
    , rotationPolicy     ( UNQL::StrictRotation )     // We use the higher-numbers-are-older rotation policy by default
    , timeRotationPolicy ( UNQL::NoTimeRotation )     // Do not rotate on a time-based policy by default
    , compressionLevel   ( 6 )                        // Use default compression level
    , compressionAlgo    ( NrFileCompressor::NO_COMPRESSION )
    , maxMinutes         ( 0 )                        // Do not use rotation based on minutes elapsed
    , reconnectionSecs   ( 5 )                        // If RemoteWriter connection drops, try to reconnect every X secs
    , netProtocol        ( UNQL::TCP )                // Use TCP as transport protocol for remote messages
{
/* empty ctor */
}


/*!
 * \brief WriterConfig::neededSanitizing Checks whether the WriterConfig instance was mis-configured
 * \return true if the WriterConfig instance holds some unacceptable values
 */
bool
WriterConfig::neededSanitizing() const
{
    //check that compressionLevel and compressionAlgo were populated with good values
    if (compressionAlgo < 0 || compressionAlgo > 2) {
        return true;
    }
    
    if (compressionLevel < 0 || compressionLevel > 9) {
        return true;
    }

    return false;
}


QString
WriterConfig::toString() const
{
    QString s;
    QStringList sl;
    sl << "COMMON PARAMS"
       << "\nMax queued messages: " << ((maxMessageNum==0) ? "unlimited" : "0") << "\nFlushing seconds: " << QString::number(writerFlushSecs)
       << "\nLog idle mark: " << (writeIdleMark ? "true" : "false")
       << "\nFILE ONLY PARAMS"
       << "\nMax file size (MB): " << QString::number(maxFileSize) << "\nMax number of files: " << QString::number(maxFileNum)
       << "\nRotationNamingPolicy: " << QString::number(rotationPolicy) << "\nCompression level " << QString::number(compressionLevel)
       << "\nTimeRotationPolicy: " << QString::number(timeRotationPolicy) << "\nMax minutes allowed " << QString::number(maxMinutes)
       << "\nNETWORK PARAMS"
       << "\nReconnection seconds: " << QString::number(reconnectionSecs)
       << "\nTransport Protocol: " << QString::number(netProtocol)
       << "\n----------";

    return s = sl.join("");
}


bool
WriterConfig::operator ==(const WriterConfig& rhs) const
{
    if ( maxMessageNum      == rhs.maxMessageNum      &&
         writerFlushSecs    == rhs.writerFlushSecs    &&
         writeIdleMark      == rhs.writeIdleMark      &&
         maxFileNum         == rhs.maxFileNum         &&
         maxFileSize        == rhs.maxFileSize        &&
         maxMinutes         == rhs.maxMinutes         &&
         rotationPolicy     == rhs.rotationPolicy     &&
         timeRotationPolicy == rhs.timeRotationPolicy &&
         compressionLevel   == rhs.compressionLevel   &&
         reconnectionSecs   == rhs.reconnectionSecs   &&
         netProtocol        == rhs.netProtocol
        )
    {
        return true;
    }

    return false;
}


bool WriterConfig::operator !=(const WriterConfig &rhs) const
{ return !(*this == rhs); }

/***************
 *             *
 *  LogWriter  *
 *             *
 ***************/


LogWriter::LogWriter(const WriterConfig &wc)
    : m_Config(wc)
    , m_logIsPaused(false)
{
    LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_INFO, "LogWriter Started", LogMessage::getCurrentTstampString());
    LogMessage lm2(DEF_UNQL_LOG_STR, UNQL::LOG_DBG, "Opened with WriterConfig:\n" + wc.toString(), LogMessage::getCurrentTstampString());
    m_logMessageList.append(lm);
    m_logMessageList.append(lm2);
    m_logTimer = new QTimer(this);
}
 


/*!
  \brief In the class dtor we need to wait that underlying thread has finished to avoid crashes
  */
LogWriter::~LogWriter()
{
    m_logMessageList.clear();
    m_logTimer->stop();
    disconnect(m_logTimer);
    ULDBG << Q_FUNC_INFO;
}


/*!
 * \brief LogWriter::getWriterConfig
 * \return a const reference to the actual writer configuration
 */
const WriterConfig&
LogWriter::getWriterConfig() const
{ return m_Config; }


/*!
 * \brief LogWriter::isLoggingPaused
 * \return whether or not the logging is puased on this writer
 */
bool
LogWriter::isLoggingPaused() const
{ return m_logIsPaused; }


/*!
  \brief this method forces the logwriter to write the messages it has stored on the underlying device before its timer
  This function is useful to call before exiting the program using UniqLogger.
  */
void
LogWriter::flush()
{
    this->writeToDevice();
}



/*!
 * \brief LogWriter::priv_writeToDevice this method is called from the internal timer in order to check if we have to add the
 * MARK string to the writer to show it's still alive and kicking. It will call the pure virtual writeToDevice() function.
 */
void
LogWriter::priv_writeToDevice()
{
    ULDBG << Q_FUNC_INFO << this << " writing on thread " << QThread::currentThread();

    if (m_Config.writeIdleMark) {
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_INFO, " -- MARK -- ", LogMessage::getCurrentTstampString());
        mutex.lock();
        if ( m_logMessageList.empty() )
            m_logMessageList.append(lm);
        mutex.unlock();
    }

    writeToDevice();
}


void
LogWriter::priv_startLogTimer()
{
    ULDBG << Q_FUNC_INFO << "executed in thread" << QThread::currentThread();
    m_logTimer->start(m_Config.writerFlushSecs * 1000);
}


/*!
 * \brief LogWriter::setWriterConfig will set a new set of configuration parameters on the specified LogWriter
 * \param wconf the configuration params we're going to set
 */
void
LogWriter::setWriterConfig(const WriterConfig &wconf)
{
    m_Config = wconf;
}



/*!
  \brief this method creates a timer and enters the thread event loop
  */
void
LogWriter::run()
{
    connect(m_logTimer, SIGNAL(timeout()), this, SLOT(priv_writeToDevice()));

    ULDBG << Q_FUNC_INFO << this << "logtimer thread" << m_logTimer->thread() << "current thread" << QThread::currentThread();

    //We need to start the timer from the thread this object has been moved not from the one calling this method
    QMetaObject::invokeMethod(this, "priv_startLogTimer");
}
 


void
LogWriter::setMaximumAllowedMessages(int i_maxmsg)
{
    m_Config.maxMessageNum = i_maxmsg;
}



/*!
  \brief sets the milliseconds that will occur between each write on the device
  \param msec the milliseconds of the timeout
  */
void
LogWriter::setSleepingMilliSecs(int msec)
{
    m_Config.writerFlushSecs = msec / 1000;
    m_logTimer->setInterval(msec);
} 
 

/*!
  \brief adds a message to the internal list for later writing
  \param s the message to be logged
  */
void
LogWriter::appendMessage(const LogMessage &s)
{
    mutex.lock();
        if (!m_logIsPaused) {
            if (m_logMessageList.count() >= m_Config.maxMessageNum && m_Config.maxMessageNum > 0) {
                ULDBG << "Message List was full, discarding previous messages..." << this;
                m_logMessageList.removeFirst();
            }
            m_logMessageList.append(s);
        }
    mutex.unlock();
}
 


/*!
  \brief pause the logging of this LogWriter
  \param status the new pause status for this writer
  */
void
LogWriter::pauseLogging(bool status)
{
    m_logIsPaused = status;
}
 
