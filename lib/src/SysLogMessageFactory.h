#ifndef SYSLOGMESSAGEFACTORY_H
#define SYSLOGMESSAGEFACTORY_H

#include <QObject>

class SysLogMessageFactory : public QObject
{
    Q_OBJECT
    int m_priority;

public:
    explicit SysLogMessageFactory(QObject *parent = nullptr);
    QString generateMessage();

signals:

public slots:
};

#endif // SYSLOGMESSAGEFACTORY_H
