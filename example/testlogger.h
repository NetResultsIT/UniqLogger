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
        l = nullptr;
        m_timer = new QTimer(this);
        m_counter = 0;
        connect(m_timer, SIGNAL(timeout()), this, SLOT(logwthread()));
        m_timer->start(m_timeout_msecs);
    }

public slots:
    void logwthread() {
        QString s;
        s.asprintf("%p", QThread::currentThread());
        s += " -- " + QString::number(m_counter++);
        if (l)
            *l << UNQL::LOG_INFO << s << Q_FUNC_INFO << UNQL::EOM;
        else
            qCritical() << QThread::currentThread() << Q_FUNC_INFO << " -- Cannot log on a NULL logger";
    }
};

class testlogger_gui : public QMainWindow
{
	Q_OBJECT
	Logger  *loggerFile, *loggerNet2, *loggerConsole, *loggerNet1, *logger5;
    Logger* m_dummyLoggerPtr;
    QTimer *timer;

public:
    testlogger_gui(QWidget *parent = 0);
    ~testlogger_gui();
    void testThreadedNetLogger(const QString &ip, int port);

    void test_strangeString(UniqLogger *ul);
    void test_dummylog(UniqLogger *ul);
private:
	Ui::testloggerClass ui;

    protected slots:
        void timedLog();
        void onStartLog();
        void onStopLog();
};

#endif // TESTLOGGER_H
