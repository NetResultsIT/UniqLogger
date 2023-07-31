/*******************************************************************************
 *  Copyright (C) 2010-2019 by NetResults S.r.l. ( http://www.netresults.it )  *
 *  Author(s):                                                                 *
 *              Francesco Lamonica   <f.lamonica@netresults.it>                *
 *******************************************************************************/

#ifndef UNQL_REMOTE_LOGGER_INCS
#define UNQL_REMOTE_LOGGER_INCS

#include "LogWriter.h"

#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>

class RemoteWriter: public LogWriter
{
    Q_OBJECT

protected:
    QTcpSocket *m_pTcpSocket;
    QUdpSocket *m_pUdpSocket;
    QTimer *m_pReconnectionTimer;
    QString m_serverAddress;
    quint16 m_serverPort;

protected:
    virtual QString getMessage();

protected slots:
    void writeToDevice();
    void onConnectionToServer();
    void onDisconnectionFromServer();

    int connectToServer(); // <-- BEWARE, this is also called via invokeMethod, do NOT change its name

public:
    explicit RemoteWriter(const QString &aServerAddress, quint16 aServerPort, const WriterConfig &wconf);
    virtual ~RemoteWriter();

    virtual void run();

    const QString getHost() const   { return m_serverAddress;   }
    int getPort() const             { return m_serverPort;      }
};
#endif

