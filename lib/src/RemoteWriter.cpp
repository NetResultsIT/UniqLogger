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

    m_Socket = new QTcpSocket(this);
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
  \brief writes the messages in the queue on the socket if logger is not paused
  */
void
RemoteWriter::writeToDevice()
{
    ULDBG << Q_FUNC_INFO << "executed in thread" << QThread::currentThread();

    mutex.lock();
    if (!m_logIsPaused) {
        if (m_Config.netProtocol == UNQL::UDP) {
            writeMessages();
        } else if (m_Socket->state() == QAbstractSocket::ConnectedState) {
            writeMessages();
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
    m_Socket->connectToHost(m_serverAddress, m_serverPort);
    bool b = m_Socket->waitForConnected(10000);
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
    connect (m_Socket, SIGNAL(disconnected()), this, SLOT(onDisconnectionFromServer()));
    connect (m_Socket, SIGNAL(connected()), this, SLOT(onConnectionToServer()));

    if (m_Config.netProtocol != UNQL::UDP) {
        QMetaObject::invokeMethod(this, "connectToServer");
    } else {
        qDebug() << "NOT CONNECTING since we're using UDP";
    }
}

/*!
 * \brief write a message based on protocol
 * \param i_msg message to write
 */
void RemoteWriter::write(const QString &i_msg)
{
    if (m_Config.netProtocol == UNQL::UDP) {
        m_pUdpSocket->writeDatagram(i_msg.toLatin1()  +"\r\n", QHostAddress(m_serverAddress), m_serverPort)
    } else {
        m_Socket->write(i_msg.toLatin1() + "\r\n");
    }
}

/*!
 * \internal
 * \brief write messages
 */
void RemoteWriter::writeMessages()
{
    if (m_Config.compressMessages) {
        writeCompressedMessages();
    } else {
        writeUncompressedMessages();
    }
}

/*!
 * \internal
 * \brief Write messages uncompressed, each message on a single line.
 */
void RemoteWriter::writeUncompressedMessages()
{
    QString s;
    int msgcount = m_logMessageList.count();
    for (int i=0; i<msgcount; i++) {
        s = m_logMessageList.takeFirst().message();
        write(s);
    }
}

/*!
 * \internal
 * \brief Write messages in compressed way, if there are multiple
 *        messages with the same body, write all in a unique line.
 */
void RemoteWriter::writeCompressedMessages()
{
    int nummsg = m_logMessageList.count();
    int i = 0;
    int j = 1;

    if (nummsg == 1)
    {
        //just one element, write it
        QString s = m_logMessageList.takeFirst().message();
        write(s);
    }
    else
    {
        QString endTstamp = "";
        int counter = 1; //number of susbsequent messages
        while (j < nummsg)
        {
            if ((m_logMessageList.at(i).rawMessage() == m_logMessageList.at(j).rawMessage()) &&
                (m_logMessageList.at(i).level() == m_logMessageList.at(j).level()))
            {
                //subsequent messages, save end timestamp and look at next element
                endTstamp = m_logMessageList.at(j).tstamp();
                ++j;
                ++counter;
            }
            else
            {
                QString s;
                if (endTstamp.isEmpty())
                {
                    //No subsequent messages, just write the message
                    s = m_logMessageList.at(i).message();
                }
                else
                {
                    //End of subsequent messages, write all as unique string
                    s = m_logMessageList.at(i).message(m_logMessageList.at(i).tstamp(), endTstamp, counter);
                }

                write(s);

                i = j;
                ++j;
                counter = 1;
                endTstamp = "";
            }

            if (j == nummsg)
            {
                //Don't want to skip last element
                QString s;
                if (endTstamp.isEmpty())
                {
                    s = m_logMessageList.at(i).message();
                }
                else
                {
                    s = m_logMessageList.at(i).message(m_logMessageList.at(i).tstamp(), endTstamp, counter);
                }

                write(s);
            }
        }
    }

    m_logMessageList.clear();
}

