#ifndef TESTLOGGER_H
#define TESTLOGGER_H

#include <QMainWindow>
#include "ui_testlogger.h"

#include "UniqLogger.h"

class TestThreadObject : public QObject
{
    Q_OBJECT
    QTimer *m_timer;
    int m_counter;
    int m_timeout_msecs;

public:
    Logger *l;

public:
    explicit TestThreadObject(int msecs, QObject *parent=0)
        :QObject(parent),
          m_timeout_msecs(msecs)
    {
        l=NULL;
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
            *l << UNQL::LOG_INFO << s << Q_FUNC_INFO << UNQL::EOM;
        else
            qCritical() << QThread::currentThread() << Q_FUNC_INFO << " -- Cannot log on a NULL logger";
    }
};

class testlogger : public QMainWindow
{
	Q_OBJECT
	Logger  *logger, *logger2, *logger3, *logger4, *logger5;
    Logger* m_dummyLoggerPtr;

public:
    testlogger(QWidget *parent = 0);
	~testlogger();
    void testThreadedNetLogger(const QString &ip, int port);

private:
	Ui::testloggerClass ui;

    protected slots:
        void timedLog();
};

#endif // TESTLOGGER_H
