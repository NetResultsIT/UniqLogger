#include "testlogger2.h"

#include <QDebug>
#include <QTimer>

#define TEST_FILE_ROTATION 0
#define TEST_CONSOLE_COLOR 0
#define TEST_FORMATTING 0
#define TEST_NET 1
#define TEST_DB 0
#define TEST_MONITOR 0

testlogger_cli::testlogger_cli(QObject *parent)
    : QObject(parent)
{
    loggerF = NULL; //This will be a file logger
    loggerN2 = NULL; //This will be red console
    loggerN1 = NULL;
    loggerCr = NULL;
    loggerCy = NULL;
    loggerCg = NULL;
    loggerD = NULL;

    QTimer *timer = new QTimer();

    connect(timer, SIGNAL(timeout()), this, SLOT(timedLog()));

    WriterConfig wc2;
    wc2.maxFileNum = 3;
    wc2.maxFileSize = 2;

    UniqLogger *ul = UniqLogger::instance();

#if(TEST_FORMATTING)
    ul->setEncasingChars( '(' , ')' );
    ul->setSpacingChar( '.' );
    ul->setTimeStampFormat("hh.mm.ss.zzz");
#endif

    loggerF = ul->createFileLogger("test", "log.txt", wc2);
    loggerCr = ul->createConsoleLogger("CONSOLE", red);
    loggerCy = ul->createConsoleLogger("CONSOLE", yellow);
    loggerCg = ul->createConsoleLogger("CONSOLE", green);

#if(TEST_NET)
    WriterConfig wconf;
    wconf.maxMessageNum = 10;

    qDebug() << "writing to network...";
    loggerN1 = ul->createNetworkLogger("net test",  "127.0.0.1", 1674, wconf);
    loggerN2 = ul->createNetworkLogger("net test2", "127.0.0.1", 1675);
    const LogWriter &nlw = ul->getNetworkWriter("127.0.0.1",1674);
    //ul->addWriterToLogger(logger,nlw);

    //testThreadedNetLogger("127.0.0.1", 1674);
#endif

#if(TEST_DB)
    loggerD = ul->createDbLogger("DbLogger","log.db");
#endif


    loggerF->setModuleName("FILE");
    loggerCr->setModuleName("REDCONSOLE");
    loggerCy->setModuleName("YELLOWCONSOLE");
    loggerCg->setModuleName("GREENCONSOLE");

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

#if(TEST_FILE_ROTATION)
    qDebug() << "written " << ++i * 100 << "KB to file...";
    loggerF->log(UNQL::LOG_INFO, ( QString("file text ") + QString::number(i) + QString().fill('a', 100000) ).toLatin1().constData() );
#endif


#if(TEST_CONSOLE_COLOR)
    loggerCr->log(UNQL::LOG_CRITICAL, ( QString("red console text - ") + QString::number(i) ).toLatin1().constData());
    *loggerCy << UNQL::LOG_WARNING << QString("yellow console text 1st part - ");
    *loggerCy << QString("yellow console text 2nd part - ") + QString::number(i) << UNQL::EOM;
    *loggerCg << UNQL::LOG_INFO << ( QString("green console text - ") + QString::number(i++) ).toLatin1().constData() << UNQL::EOM;
#endif


#if(TEST_NET)
    loggerN2->log(UNQL::LOG_CRITICAL, (QString("net critical")+QString::number(i)).toLatin1().constData());
    *loggerN1 << UNQL::LOG_INFO << (QString("net info ")+QString::number(i++)) << UNQL::EOM;
#endif


#if(TEST_DB)
        loggerD->log( UNQL::LOG_INFO, QString("Testing db log" + QString::number(i)).toLatin1().constData() );
        *loggerD <<   UNQL::LOG_CRITICAL << "This is a critical db error!" << QString::number(i++) << UNQL::EOM;
        qDebug() << "Writing to db...";
#endif


#if(TEST_MONITOR)
    double dd = qrand();

    QString key = "groupKey";
    loggerCr->setModuleName("Red Monitor");
    loggerCr->monitor(dd, key, "test variable dd");
    loggerCr->monitor(i, key, "iteration number");
    i++;

    //after 5 iteration start monitoring
    if (i==5) {
        loggerCr->setModuleName("Red Monitor active");
        UniqLogger *ul = UniqLogger::instance();
        ul->monitorVar(key, true);
    }

    //now at 10th iteration stop monitoring
    if (i==15) {
        loggerCr->setModuleName("Red Monitor");
        UniqLogger *ul = UniqLogger::instance();
        ul->monitorVar(key, false);
    }
#endif

}


