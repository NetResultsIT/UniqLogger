/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#include "RemoteWriter.h"

#include <QTime>

RemoteWriter::RemoteWriter(const QString &aServerAddress, int aServerPort, const WriterConfig &wconf)
    : LogWriter(wconf)
{
    m_serverAddress = aServerAddress;
    m_serverPort = aServerPort;

    m_Socket = new QTcpSocket(this);
    m_reconnectionTimer = new QTimer(this);
}
 
/*!
  \brief In the class dtor we want to flush whatever we might have got that is not written
  */
RemoteWriter::~RemoteWriter()
{
    //on exit, write all we've got
    this->flush();
}

/*!
  \brief writes the messages in the queue on the socket
  */
void
RemoteWriter::writeToDevice()
{
    ULDBG << Q_FUNC_INFO << "executed in thread" << QThread::currentThread();

    QString s;
    mutex.lock();
    if (!m_logIsPaused && m_Socket->state() == QAbstractSocket::ConnectedState)
    {
        int msgcount = m_logMessageList.count();
        for (int i=0; i<msgcount; i++) {
            s = m_logMessageList.takeFirst().message();
            m_Socket->write(s.toLatin1()+"\r\n");
        }
    }
    mutex.unlock();
}
 
/*!
  \brief connects the logwriter to the specified server
  \return a negative code if the connection fails, 0 otherwise
  This call is blocking (for this thread) and waits 10 secs to see if the connection can be established or not
  */
int
RemoteWriter::connectToServer()
{
    ULDBG << Q_FUNC_INFO << QDateTime::currentDateTime().toString("hh.mm.ss.zzz") << "executed in thread" << QThread::currentThread();
    m_Socket->connectToHost(m_serverAddress, m_serverPort);
    bool b = m_Socket->waitForConnected(10000);
    if (b)
        return 0;

    LogMessage msg("Remote Logwriter", UNQL::LOG_WARNING, "Connection fail to server " + m_serverAddress+":" + QString::number(m_serverPort),
                   LogMessage::getCurrentTstampString());
    appendMessage(msg);

    //now we restart the connection timer
    m_reconnectionTimer->start(m_Config.reconnectionSecs * 1000);

    return -1;
}

/*!
 \brief this slot is invoked whenever the RemoteWriter connects to the logging server
 Upon connection it stops the reconnection timer
 */
void
RemoteWriter::onConnectionToServer()
{
    ULDBG << Q_FUNC_INFO << "executed in thread" << QThread::currentThread();
    ULDBG << "Connected to server " << m_serverAddress << " : " << m_serverPort;

    LogMessage msg("Remote Logwriter", UNQL::LOG_INFO, "Connected to server " + m_serverAddress + ":" + QString::number(m_serverPort),
                   LogMessage::getCurrentTstampString());
    appendMessage(msg);
    m_reconnectionTimer->stop();
}

/*!
 \brief this slot is invoked whenever the RemoteWriter loses connection to the logging server
 It starts a timer that will keep trying to reconnect to the server
 */
void
RemoteWriter::onDisconnectionFromServer()
{
    ULDBG << Q_FUNC_INFO << "executed in thread" << QThread::currentThread();
    ULDBG << "Disconnected from server " << m_serverAddress << " : " << m_serverPort;

    LogMessage msg("Remote Logwriter", UNQL::LOG_WARNING, "Disconnected from server " + m_serverAddress + ":" + QString::number(m_serverPort),
                   LogMessage::getCurrentTstampString());
    appendMessage(msg);
    m_reconnectionTimer->start(m_Config.reconnectionSecs * 1000);
}
 

//FIXME - protect from multiple calls
void
RemoteWriter::run()
{
    ULDBG << Q_FUNC_INFO;
    LogWriter::run();

    connect (m_reconnectionTimer, SIGNAL(timeout()), this, SLOT(connectToServer()));
    connect (m_Socket, SIGNAL(disconnected()), this, SLOT(onDisconnectionFromServer()));
    connect (m_Socket, SIGNAL(connected()), this, SLOT(onConnectionToServer()));

    QMetaObject::invokeMethod(this, "connectToServer");
}
