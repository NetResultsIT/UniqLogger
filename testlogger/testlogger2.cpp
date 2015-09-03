#include "testlogger2.h"

#include <QDebug>
#include <QTimer>

#define TEST_FILE_ROTATION 0
#define TEST_CONSOLE_COLOR 0
#define TEST_FORMATTING 0
#define TEST_NET 0
#define TEST_NET_MULTISRC 0
#define TEST_DB 0
#define TEST_MONITOR 1

testlogger_cli::testlogger_cli(QObject *parent)
    : QObject(parent)
{
    loggerF1 = NULL; //This will be a file logger
    loggerF2 = NULL; //This will be a file logger
    loggerN1 = NULL; //This will be a net logger to localhost:1674
    loggerN2 = NULL; //This will be a net logger to localhost:1675
    loggerCr = NULL; //This will be red console
    loggerCy = NULL; //This will be yellow console
    loggerCg = NULL; //This will be green console
    loggerCm = NULL; //This will be magenta console
    loggerD  = NULL; //This will be db logger

    QTimer *timer = new QTimer();
    int millis = 2000;
    connect(timer, SIGNAL(timeout()), this, SLOT(timedLog()));


    UniqLogger *ul = UniqLogger::instance("TESTER", 2);

#if(TEST_FORMATTING)
    ul->setEncasingChars( '(' , ')' );
    ul->setSpacingChar( '.' );
    ul->setTimeStampFormat("hh.mm.ss.zzz");
#endif


#if(TEST_FILE_ROTATION)
    millis = 20;
    WriterConfig wc2;
    wc2.maxFileNum      = 3; //we're going to use 3 files
    wc2.maxFileSize     = 1; //up to 1MB each
    wc2.writerFlushSecs = 1; //flush contents to disk every second

    loggerF1 = ul->createFileLogger("test", "log.txt", wc2);
    loggerF1->setModuleName("FILE1");
    loggerF2 = ul->createFileLogger("test", "log.txt", wc2);
    loggerF2->setModuleName("FILE2");
#endif


#if(TEST_CONSOLE_COLOR)
    loggerCr = ul->createConsoleLogger("CONSOLE", red);
    loggerCy = ul->createConsoleLogger("CONSOLE", yellow);
    loggerCg = ul->createConsoleLogger("CONSOLE", green);

    loggerCr->setModuleName("REDCONSOLE");
    loggerCy->setModuleName("YELLOWCONSOLE");
    loggerCg->setModuleName("GREENCONSOLE");
#endif


#if(TEST_NET)
    WriterConfig wconf;
    wconf.maxMessageNum = 10;

    qDebug() << "writing to network...";
    loggerN1 = ul->createNetworkLogger("netlog to localhost:1674", "127.0.0.1", 1674, wconf);
    loggerN2 = ul->createNetworkLogger("netlog to localhost:1675", "127.0.0.1", 1675);

    //testThreadedNetLogger("127.0.0.1", 1674);
#endif


#if(TEST_DB)
    loggerD = ul->createDbLogger("DbLogger","log.db");
#endif



#if(TEST_MONITOR)
    loggerCm = ul->createConsoleLogger("MagentaC", magenta);
#endif

    //Start timed logging
    timer->start(millis);
}


void
testlogger_cli::testThreadedNetLogger(const QString &ip, int port)
{
    Q_UNUSED(ip);
    Q_UNUSED(port);

    TestThreadObject2 *tobj1 = new TestThreadObject2(2000);
    tobj1->l = loggerN1;
    TestThreadObject2 *tobj2 = new TestThreadObject2(3000);
    tobj2->l = loggerN1;

    QThread *t1, *t2;
    t1 = new QThread(this);
    t2 = new QThread(this);
    tobj1->moveToThread(t1);
    tobj2->moveToThread(t2);

    t1->start();
    t2->start();
}


void
testlogger_cli::timedLog()
{
    static int i=0;

    i++;

#if(TEST_FILE_ROTATION)
    qDebug() << "written " << i * 2 << "KB to file...";
    loggerF1->log(UNQL::LOG_INFO, ( QString("file text iteration ") + QString::number(i) + " " +QString().fill('a', 1500) ).toLatin1().constData() );
    loggerF2->log(UNQL::LOG_INFO, ( QString("file2 text iteration") + QString::number(i) + " " + QString().fill('b', 500) ).toLatin1().constData() );
#endif


#if(TEST_CONSOLE_COLOR)
    loggerCr->log(UNQL::LOG_CRITICAL, ( QString("red console text - ") + QString::number(i) ).toLatin1().constData());
    *loggerCy << UNQL::LOG_WARNING << QString("yellow console text 1st part - ");
    *loggerCy << QString("yellow console text 2nd part - ") + QString::number(i) << UNQL::EOM;
    *loggerCg << UNQL::LOG_INFO << ( QString("green console text - ") + QString::number(i) ).toLatin1().constData() << UNQL::EOM;
#endif


#if(TEST_NET)
    qDebug() << "writing to net... iteration" << i;
    loggerN2->log(UNQL::LOG_CRITICAL, ( QString("net critical") + QString::number(i) ).toLatin1().constData());
    *loggerN1 << UNQL::LOG_INFO << (QString("net info ")+QString::number(i)) << UNQL::EOM;

#if(TEST_NET_MULTISRC)
    if (i == 10) {
        UniqLogger *ul = UniqLogger::instance();
        const LogWriter &nlw = ul->getNetworkWriter("127.0.0.1", 1674);
        qDebug() << "adding writer returned: " << ul->addWriterToLogger(loggerN2, nlw);
    }
    if (i == 15) {
        UniqLogger *ul = UniqLogger::instance();
        const LogWriter &nlw = ul->getNetworkWriter("127.0.0.1", 1674);
        qDebug() << "removing writer returned: " << ul->removeWriterFromLogger(loggerN2, nlw);
    }
#endif

#endif


#if(TEST_DB)
        loggerD->log( UNQL::LOG_INFO, QString("Testing db log" + QString::number(i)).toLatin1().constData() );
        *loggerD <<   UNQL::LOG_CRITICAL << "This is a critical db error!" << QString::number(i) << UNQL::EOM;
        qDebug() << "Writing to db...iteration" <<i ;
#endif


#if(TEST_MONITOR)
    qDebug() << "Testing monitoring - iteration" << i;
    double dd = qrand();

    QString key = "groupKey";
    loggerCm->setModuleName("MagentaMonitor");
    loggerCm->monitor(dd, key, "test variable dd");
    loggerCm->monitor(i, key, "iteration number");

    //after 5 iteration start monitoring
    if (i==5) {
        loggerCm->setModuleName("MagentaMonitor active");
        UniqLogger *ul = UniqLogger::instance();
        ul->changeMonitorVarStatus(key, true);
    }

    //now at 10th iteration stop monitoring
    if (i==15) {
        loggerCm->setModuleName("MagentaMonitor");
        UniqLogger *ul = UniqLogger::instance();
        ul->changeMonitorVarStatus(key, false);
    }
#endif

}


