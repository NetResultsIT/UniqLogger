/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#ifndef __REMOTE_LOGGER_INCS__
#define __REMOTE_LOGGER_INCS__

#include "LogWriter.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>

class RemoteWriter: public LogWriter
{
    Q_OBJECT

    QTcpSocket *m_Socket;
    QTimer *m_reconnectionTimer;
    QString m_serverAddress;
    int m_serverPort;

protected slots:
    void writeToDevice();
    void onConnectionToServer();
    void onDisconnectionFromServer();

    int connectToServer(); // <-- BEWARE, this is also called via invokeMethod, do NOT change its name

public:
    explicit RemoteWriter(const QString &aServerAddress, int aServerPort, const WriterConfig &wconf);
    virtual ~RemoteWriter();

    virtual void run();

    const QString getHost() const   { return m_serverAddress;   }
    int getPort() const             { return m_serverPort;      }
};

#endif

