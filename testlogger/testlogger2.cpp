#include "testlogger2.h"

#include <QDebug>
#include <QTimer>

#define TEST_FILE_ROTATION 0
#define TEST_CONSOLE_COLOR 0
#define TEST_FORMATTING 0
#define TEST_NET 1
#define TEST_NET_MULTISRC 1
#define TEST_DB 0
#define TEST_MONITOR 0

testlogger_cli::testlogger_cli(QObject *parent)
    : QObject(parent)
{
    loggerF  = NULL; //This will be a file logger
    loggerN1 = NULL; //This will be a net logger to localhost:1674
    loggerN2 = NULL; //This will be a net logger to localhost:1675
    loggerCr = NULL; //This will be red console
    loggerCy = NULL; //This will be yellow console
    loggerCg = NULL; //This will be green console
    loggerCm = NULL; //This will be magenta console
    loggerD  = NULL; //This will be db logger

    QTimer *timer = new QTimer();

    connect(timer, SIGNAL(timeout()), this, SLOT(timedLog()));


    UniqLogger *ul = UniqLogger::instance();

#if(TEST_FORMATTING)
    ul->setEncasingChars( '(' , ')' );
    ul->setSpacingChar( '.' );
    ul->setTimeStampFormat("hh.mm.ss.zzz");
#endif


#if(TEST_FILE_ROTATION)
    WriterConfig wc2;
    wc2.maxFileNum = 2;  //we're going to use 2 files
    wc2.maxFileSize = 1; //up to 500KB max

    loggerF = ul->createFileLogger("test", "log.txt", wc2);
    loggerF->setModuleName("FILE");
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
    //qDebug() << Q_FUNC_INFO << "current Thread" << QThread::currentThread();
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
    timer->start(2000);
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
    qDebug() << "written " << i * 100 << "KB to file...";
    loggerF->log(UNQL::LOG_INFO, ( QString("file text ") + QString::number(i) + QString().fill('a', 100000) ).toLatin1().constData() );
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
        qDebug() << "adding writer returned: " << ul->addWriterToLogger(loggerN1, nlw);
    }
    if (i == 15) {
        UniqLogger *ul = UniqLogger::instance();
        const LogWriter &nlw = ul->getNetworkWriter("127.0.0.1", 1674);
        qDebug() << "removing writer returned: " << ul->removeWriterFromLogger(loggerN1, nlw);
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
        ul->monitorVar(key, true);
    }

    //now at 10th iteration stop monitoring
    if (i==15) {
        loggerCm->setModuleName("MagentaMonitor");
        UniqLogger *ul = UniqLogger::instance();
        ul->monitorVar(key, false);
    }
#endif

}


