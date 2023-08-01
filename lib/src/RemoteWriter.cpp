/*******************************************************************************
 *  Copyright (C) 2010-2019 by NetResults S.r.l. ( http://www.netresults.it )  *
 *  Author(s):                                                                 *
 *              Francesco Lamonica   <f.lamonica@netresults.it>                *
 *******************************************************************************/

#include "RemoteWriter.h"

#include <QTime>
#include <QHostAddress>
#include <QHostInfo>

RemoteWriter::RemoteWriter(const QString &aServerAddress, quint16 aServerPort, const WriterConfig &wconf)
    : LogWriter(wconf)
{
    //ensure we're using an IP address instead of an alphanumeric one
    m_serverAddress = aServerAddress;
    if (QHostAddress(m_serverAddress).isNull()) {
        QHostInfo info = QHostInfo::fromName(m_serverAddress);
        if (!info.addresses().isEmpty()) {
            m_serverAddress = info.addresses().first().toString();
            // use the first IP address
        } else {
            LogMessage msg("Remote Logwriter", UNQL::LOG_ERROR,
                           "DNS failed to give an IP address for " + m_serverAddress,
                           LogMessage::getCurrentTstampString());
            appendMessage(msg);
        }
    }

    m_serverPort = aServerPort;

    m_pTcpSocket = new QTcpSocket(this);
    m_pUdpSocket = new QUdpSocket(this);
    m_pReconnectionTimer = new QTimer(this);
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
 * \brief RemoteWriter::getMessage composes and returns the next message string to be printed by this writer
 * This is an internal method that is supposed to be called within the mutex.lock()
 * \return the composed message string
 */
QString
RemoteWriter::getMessage()
{
    return m_logMessageList.takeFirst().message();
}

/*!
  \brief writes the messages in the queue on the socket if logger is not paused
  */
void
RemoteWriter::writeToDevice()
{
    ULDBG << Q_FUNC_INFO << "executed in thread" << QThread::currentThread();

    mutex.lock();

    if (m_Config.netProtocol == UNQL::UDP) {
        int msgcount = m_logMessageList.count();
        for (int i=0; i<msgcount; i++) {
            QString s = this->getMessage();
            int wb = m_pUdpSocket->writeDatagram(s.toLatin1()  +"\r\n", QHostAddress(m_serverAddress), m_serverPort);
        }
    } else if (m_pTcpSocket->state() == QAbstractSocket::ConnectedState) {
        int msgcount = m_logMessageList.count();
        for (int i=0; i<msgcount; i++) {
            QString s = this->getMessage();
            m_pTcpSocket->write(s.toLatin1() + "\r\n");
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
    ULDBG << Q_FUNC_INFO << QDateTime::currentDateTime().toString("hh.mm.ss.zzz")
          << "executed in thread" << QThread::currentThread();
    m_pTcpSocket->connectToHost(m_serverAddress, m_serverPort);
    bool b = m_pTcpSocket->waitForConnected(10000);
    if (b)
        return 0;

    LogMessage msg("Remote Logwriter", UNQL::LOG_WARNING,
                   "Connection fail to server " + m_serverAddress + ":" + QString::number(m_serverPort),
                   LogMessage::getCurrentTstampString());
    appendMessage(msg);

    //now we restart the connection timer
    m_pReconnectionTimer->start(m_Config.reconnectionSecs * 1000);

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

    LogMessage msg("Remote Logwriter", UNQL::LOG_INFO,
                   "Connected to server " + m_serverAddress + ":" + QString::number(m_serverPort),
                   LogMessage::getCurrentTstampString());
    appendMessage(msg);
    m_pReconnectionTimer->stop();
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

    LogMessage msg("Remote Logwriter", UNQL::LOG_WARNING,
                   "Disconnected from server " + m_serverAddress + ":" + QString::number(m_serverPort),
                   LogMessage::getCurrentTstampString());
    appendMessage(msg);
    m_pReconnectionTimer->start(m_Config.reconnectionSecs * 1000);
}


//FIXME - protect from multiple calls
void
RemoteWriter::run()
{
    ULDBG << Q_FUNC_INFO;
    LogWriter::run();

    connect (m_pReconnectionTimer, SIGNAL(timeout()), this, SLOT(connectToServer()));
    connect (m_pTcpSocket, SIGNAL(disconnected()), this, SLOT(onDisconnectionFromServer()));
    connect (m_pTcpSocket, SIGNAL(connected()), this, SLOT(onConnectionToServer()));

    if (m_Config.netProtocol != UNQL::UDP) {
        QMetaObject::invokeMethod(this, "connectToServer");
    } else {
        qDebug() << "NOT CONNECTING since we're using UDP";
    }
}

