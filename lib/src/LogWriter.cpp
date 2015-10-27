/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#include "LogWriter.h"

#include <QStringList>
#include <QDateTime>

/******************
 *                *
 *  WriterConfig  *
 *                *
 ******************/

WriterConfig::WriterConfig()
{
        maxMessageNum   = 0;    // unlimited
        maxFileSize     = 0;    // unlimited MB size of logfile
        maxFileNum      = 1;    // log on just one file by default
        writerFlushSecs = 5;    // each writer will flush data every 5 seconds
        writeIdleMark   = false; // If no messages are to be written, write a MARK string to show writer is alive
        reconnectionSecs= 5;    // If RemoteWrite connection drops, try to reconnect every X secs
}

/***************
 *             *
 *  LogWriter  *
 *             *
 ***************/


LogWriter::LogWriter()
{
    m_logIsPaused       = false;
    m_sleepingMilliSecs = 5000;
    m_maxMessages       = -1;
    m_writeIdleMark     = false;
    LogMessage lm("UniqLogger", UNQL::LOG_INFO, "a Logger Started", QDateTime::currentDateTime().toString("hh:mm:ss"));
    m_logMessageList.append(lm);
    m_logTimer = new QTimer(this);
}
 


/*!
  \brief In the class dtor we need to wait that underlying thread has finished to avoid crashes
  */
LogWriter::~LogWriter()
{
    ULDBG << Q_FUNC_INFO;
}



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

    if (m_writeIdleMark) {
        LogMessage lm("UniqLogger", UNQL::LOG_INFO, " -- MARK -- ", QDateTime::currentDateTime().toString("hh:mm:ss"));
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
    m_logTimer->start(m_sleepingMilliSecs);
}


/*!
 * \brief LogWriter::setWriterConfig will set a new set of configuration parameters on the specified LogWriter
 * \param wconf the configuration params we're going to set
 */
void
LogWriter::setWriterConfig(const WriterConfig &wconf)
{
    m_sleepingMilliSecs = wconf.writerFlushSecs * 1000;
    m_writeIdleMark     = wconf.writeIdleMark;
    m_maxMessages       = wconf.maxMessageNum;
}



/*!
  \brief this method creates a timer and enters the thread event loop
  */
void
LogWriter::run()
{
    m_logTimer->setInterval(m_sleepingMilliSecs);
    //qDebug() << this << "setting timer interval to "<< m_sleepingMilliSecs;
    connect(m_logTimer, SIGNAL(timeout()), this, SLOT(priv_writeToDevice()));

    ULDBG << Q_FUNC_INFO << this << "logtimer thread" << m_logTimer->thread() << "current thread" << QThread::currentThread();

    //We need to start the timer from the thread this object has been moved not from the one calling this method
    QMetaObject::invokeMethod(this, "priv_startLogTimer");
}
 


void
LogWriter::setMaximumAllowedMessages(int i_maxmsg)
{
    m_maxMessages = i_maxmsg;
}



/*!
  \brief sets the milliseconds that will occur between each write on the device
  \param msec the milliseconds of the timeout
  */
void
LogWriter::setSleepingMilliSecs(int msec)
{	
    m_sleepingMilliSecs = msec;
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
            if (m_logMessageList.count() >= m_maxMessages && m_maxMessages>0) {
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
 
