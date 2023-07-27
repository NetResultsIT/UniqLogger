#include "RSyslogWriter.h"

#include <QCoreApplication>
#include <QHostInfo>

RSyslogWriter::RSyslogWriter(const QString &i_id, quint8 i_facility, const QString &aServerAddress, quint16 aServerPort, const WriterConfig &wconf)
    : RemoteWriter(aServerAddress, aServerPort, wconf)
    , m_mid(i_id)
    , m_facility(i_facility)
{
    slmf.m_hostname = QHostInfo::localHostName();
    slmf.m_appname = QCoreApplication::applicationName();
    slmf.m_pid = QString::number(QCoreApplication::applicationPid());
    slmf.m_mid = m_mid;
    slmf.m_facility = m_facility;
}


/*!
 * \brief RemoteWriter::getMessage composes and returns the next message string to be printed by this writer
 * This is an internal method that is supposed to be called within the mutex.lock()
 * \return the composed message string
 */
QString
RSyslogWriter::getMessage()
{

    //now get data from logmessage
    LogMessage lm = m_logMessageList.takeFirst();

    slmf.m_severity = convertUnqlLogLevelToSyslog(lm.level());
    slmf.m_timestamp = QDateTime::fromString(lm.tstamp(), Qt::ISODateWithMs);
    slmf.m_msgBody = lm.message();

    QString msg = slmf.generateMessage();

    return msg;
}


quint8
RSyslogWriter::convertUnqlLogLevelToSyslog(UNQL::LogMessagePriorityType logLevel)
{
    //Note all the return code are specified in Syslog RFC 5424 6.2.1 Table 2
    if (logLevel >= UNQL::LOG_DBG)
        return 7;
    if (logLevel == UNQL::LOG_INFO)
        return 6;
    if (logLevel == UNQL::LOG_NOTICE)
        return 5;
    if (logLevel == UNQL::LOG_WARNING)
        return 4;
    if (logLevel == UNQL::LOG_ERROR)
        return 3;
    if (logLevel == UNQL::LOG_CRITICAL)
        return 2;
    if (logLevel == UNQL::LOG_ALARM)
        return 1;
    if (logLevel == UNQL::LOG_FATAL)
        return 0;

    //by default return INFO
    return 6;
}

/*!
  \brief writes the messages in the queue on the socket if logger is not paused
  */
void
RSyslogWriter::writeToDevice()
{
    ULDBG << Q_FUNC_INFO << "executed in thread" << QThread::currentThread();

    QString s;
    mutex.lock();
    if (!m_logIsPaused) {
        if (m_Config.netProtocol == UNQL::UDP) {
            int msgcount = m_logMessageList.count();
            for (int i=0; i<msgcount; i++) {
                s = getMessage();
                int wb = m_pUdpSocket->writeDatagram(s.toLatin1()  +"\r\n", QHostAddress(m_serverAddress), m_serverPort);
            }
        } else if (m_pTcpSocket->state() == QAbstractSocket::ConnectedState) {
            int msgcount = m_logMessageList.count();
            for (int i=0; i<msgcount; i++) {
                s = getMessage();
                m_pTcpSocket->write(s.toLatin1() + "\r\n");
            }
        }
    }
    mutex.unlock();
}
