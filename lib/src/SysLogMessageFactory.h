#ifndef SYSLOGMESSAGEFACTORY_H
#define SYSLOGMESSAGEFACTORY_H

#include <QObject>
#include <QDateTime>

class SysLogMessageFactory : public QObject
{
    Q_OBJECT
    int m_priority;

    QString calculatePriority();
    QString generateHeader();

public:
    quint8 m_facility;
    quint8 m_severity;
    QDateTime m_timestamp;
    QString m_hostname;
    QString m_appname;
    QString m_pid;
    QString m_mid;
    QString m_msgBody;

public:
    explicit SysLogMessageFactory(QObject *parent = nullptr);
    QString generateMessage();

signals:

public slots:
};

#endif // SYSLOGMESSAGEFACTORY_H
