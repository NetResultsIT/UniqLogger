/********************************************************************************
 *   Copyright (C) 2010-2014 by NetResults S.r.l. ( http://www.netresults.it )  *
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

	QTcpSocket m_Socket;
    QTimer m_reconnectionTimer;
    QString m_serverAddress;
    int m_serverPort;
    int m_reconnectionTimeout;

protected slots:
	void writeToDevice();
    void onConnectionToServer();
    void onDisconnectionFromServer();

    int connectToServer();

public:
    explicit RemoteWriter(const QString &aServerAddress, int aServerPort);
    virtual ~RemoteWriter();

    virtual void run();
    virtual void setWriterConfig(const WriterConfig &wconf);

    const QString getHost() const { return m_serverAddress; }
    int getPort() const { return m_serverPort; }

    QString getRemoteHost() const {
		if (m_Socket.isOpen())
			return m_Socket.peerAddress().toString();
		return "";
    }

    int getRemotePort() const {
		if (m_Socket.isOpen())
			return m_Socket.peerPort();
		return -1;
    }
};

#endif

