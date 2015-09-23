#ifndef _CLI_TESTLOGGER_H_
#define _CLI_TESTLOGGER_H_

#include <QObject>

#include "UniqLogger.h"


class TestThreadObject2 : public QObject
{
    Q_OBJECT

    QTimer *m_timer;
    int m_counter;
    int m_timeout_msecs;
    UNQL::LogMessagePriorityType m_logPriority;
    QString m_msg;

public:
    Logger *l;

public:
    explicit TestThreadObject2(int msecs, UNQL::LogMessagePriorityType prio=UNQL::LOG_INFO, const QString msg="", QObject *parent=0)
        : QObject(parent),
          m_timeout_msecs(msecs),
          m_logPriority(prio),
          m_msg(msg)
    {
        l = NULL;
        m_timer = new QTimer(this);
        m_counter = 0;
        connect(m_timer, SIGNAL(timeout()), this, SLOT(logwthread()));
        m_timer->start(m_timeout_msecs);
    }

public slots:
    void logwthread() {
        QString s;
        s.sprintf("%p", QThread::currentThread());
        s += " -- " + QString::number(m_counter++);
        if (l)
            *l << m_logPriority << s << m_msg << Q_FUNC_INFO << UNQL::EOM;
        else
            qCritical() << QThread::currentThread() << Q_FUNC_INFO << " -- Cannot log on a NULL logger";
    }
};

class testlogger_cli : public QObject
{
    Q_OBJECT
    Logger  *loggerF1, *loggerF2, *loggerN2, *loggerN1, *loggerD;
    Logger  *loggerCy, *loggerCg, *loggerCr, *loggerCm;

public:
    testlogger_cli(QObject *parent = 0);
    ~testlogger_cli() {}

    void testThreadedNetLogger(const QString &ip, int port);
    void testThreadedConsoleLogger();
    void testBenchmark();
    //void test_strangeString(UniqLogger *ul);

protected slots:
    void timedLog();
};

#endif // TESTLOGGER_H
