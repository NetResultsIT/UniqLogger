#include "testlogger2.h"

#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QHash>

#define TEST_FILE_ROTATION 1
#define TEST_CONSOLE_COLOR 0
#define TEST_FORMATTING 0
#define TEST_NET 0
#define TEST_NET_MULTISRC 0
#define TEST_DB 0
#define TEST_MONITOR 0
#define TEST_THREADSAFETY 1
#define TEST_BENCHMARK 0
#define TEST_CRASH 0
#define TEST_SIGSEGV 0
#define TEST_DUMMY 0


testlogger_cli::testlogger_cli(QObject *parent)
    : QObject(parent)
{
    loggerF1 = nullptr; //This will be a file logger
    loggerF2 = nullptr; //This will be a file logger
    loggerN1 = nullptr; //This will be a net logger to localhost:1674
    loggerN2 = nullptr; //This will be a net logger to localhost:1675
    loggerRS1 = nullptr; //This will be rsyslogr to 192.168.1.16:5144 (TCP)
    loggerRS2 = nullptr; //This will be a net logger to nas.netresults.network:5144 (UDP)
    loggerCr = nullptr; //This will be red console
    loggerCy = nullptr; //This will be yellow console
    loggerCg = nullptr; //This will be green console
    loggerCm = nullptr; //This will be magenta console
    loggerD  = nullptr; //This will be db logger
    dummy    = nullptr; //This will be a dummy logger;

    QTimer *timer = new QTimer();
    m_eltimer = new QElapsedTimer();
    int millis = 2000;
    connect(timer, SIGNAL(timeout()), this, SLOT(timedLog()));


    UniqLogger *ul = UniqLogger::instance("TESTER", 1);
    ul->setDefaultLogLevel(UNQL::LOG_DBG);

#if(TEST_FORMATTING)
    ul->setEncasingChars( '(' , ')' );
    ul->setSpacingChar( '.' );
    ul->setTimeStampFormat("hh.mm.ss.zzz");
#endif


#if(TEST_DUMMY)
    dummy = ul->createDummyLogger("DUMMY");
#endif


#if(TEST_FILE_ROTATION)
    millis = 20;
    WriterConfig wc2, wc4;
    wc2.maxFileNum      = 7; //we're going to use 3 files
    wc2.maxFileSize     = 0; //up to 1MB each
    wc2.writerFlushSecs = 1; //flush contents to disk every second
    //wc2.compressionAlgo = 1; //Use Gzip
    //wc2.compressionLevel = 1;
    wc2.rotationPolicy = UNQL::StrictRotation;
    wc2.timeRotationPolicy = UNQL::PerMinuteRotation;
    wc2.maxMinutes = 1;
    wc2.timeRotationPolicy = UNQL::ElapsedMinutesRotation;

    loggerF1 = ul->createFileLogger("test", "log.txt", wc2);
    loggerF1->setModuleName("FILE1");
    loggerF1->printThreadID(true);
    //loggerF2 = ul->createFileLogger("test", "log2.txt", wc4);
    //loggerF2->setModuleName("FILE2");
#endif


#if(TEST_CONSOLE_COLOR || TEST_THREADSAFETY)
    UNQL::ConsoleColorScheme cs_red, cs_green, cs_yellow;

    cs_red.setDefaultColor(UNQL::red);
    cs_green.setDefaultColor(UNQL::green);
    cs_yellow.setDefaultColor(UNQL::yellow);

    loggerCr = ul->createConsoleLogger("RCONSOLE", cs_red);
    loggerCy = ul->createConsoleLogger("YCONSOLE", cs_yellow);
    loggerCg = ul->createConsoleLogger("GONSOLE", cs_green);

    loggerCr->setModuleName("REDCONSOLE");
    loggerCy->setModuleName("YELLOWCONSOLE");
    loggerCg->setModuleName("GREENCONSOLE");

    //enable thread id log
    loggerCr->printThreadID(true);
#endif


#if(TEST_NET)
    WriterConfig wconf, wconf2;
    wconf.maxMessageNum = 10;
    wconf2.netProtocol = UNQL::UDP;

    qDebug() << "writing to network...";
    loggerN1 = ul->createNetworkLogger("netlog to localhost:1674", "127.0.0.1", 1674, wconf);
    loggerN2 = ul->createNetworkLogger("netlog to localhost:1675", "127.0.0.1", 1675);

    loggerRS1 = ul->createRSyslogLogger("syslog to synology 5144", "TCPMSG", 0, "192.168.1.16", 5144);
    loggerRS2 = ul->createRSyslogLogger("syslog to synology 5144", "UDPMSG", 7, "nas.netresults.network", 5144, wconf2);

    //testThreadedNetLogger("127.0.0.1", 1674);
#endif


#if(TEST_DB)
    loggerD = ul->createDbLogger("DbLogger","log.db");
#endif



#if(TEST_MONITOR)
    loggerCm = ul->createConsoleLogger("MagentaC", magenta);
#endif


#if(TEST_THREADSAFETY)
    testThreadedConsoleLogger();
#endif


#if(TEST_BENCHMARK)
    testBenchmark();
#endif


#if(TEST_CRASH)
    ul->enableCrashHandler();
#endif

    //Start timed logging
    timer->start(millis);
}


void
testlogger_cli::testThreadedNetLogger(const QString &ip, int port)
{
    Q_UNUSED(ip)
    Q_UNUSED(port)

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
testlogger_cli::testThreadedConsoleLogger()
{
    int sched_ms1 = 200;
    TestThreadObject2 *tobj1 = new TestThreadObject2(sched_ms1, UNQL::LOG_WARNING, "You should see this...");
    tobj1->l = loggerCr;
    int sched_ms2 = 100;
    TestThreadObject2 *tobj2 = new TestThreadObject2(sched_ms2, UNQL::LOG_DBG, "But *definitely* NOT this...");
    tobj2->l = loggerCr;
    int sched_ms3 = 150;
    TestThreadObject2 *tobj3 = new TestThreadObject2(sched_ms3, UNQL::LOG_INFO, "You *might* see this...");
    tobj3->l = loggerCy;

    QThread *t1, *t2, *t3;
    t1 = new QThread(this);
    t2 = new QThread(this);
    t3 = new QThread(this);
    tobj1->moveToThread(t1);
    tobj2->moveToThread(t2);
    tobj3->moveToThread(t3);

    t1->start();
    t2->start();
    t3->start();
}



void
testlogger_cli::testBenchmark()
{
    int iterations = 1000000;
    UniqLogger *ul = UniqLogger::instance("TESTER", 4);
    Logger *l = ul->createFileLogger("BENCHMARK", "testlog");

    QElapsedTimer t;
    t.start();
    for (int i = 0; i < iterations; i++) {
        *l << UNQL::LOG_INFO << "A test message" << UNQL::EOM;
    }
    qDebug() << "logged" << iterations << "in" << t.elapsed() << "msecs";
}



void
testlogger_cli::timedLog()
{
    static int i=0;

    i++;

#if(TEST_CRASH)
    qDebug() << "About to perform a crashing op...";
    char arr[10];
    arr[1998] = 'aaaaaaaaaaaa';
    qDebug() << "arr" << &(arr)+2000;
    /**/
    short b;
    b += 1234;
    printf("%sd",b);/**/
    qDebug() << "done";
#endif


#if(TEST_DUMMY)
    *dummy << UNQL::LOG_CRITICAL << "Dummy" << UNQL::EOM;
#endif


#if(TEST_FILE_ROTATION)
    static int small = 100;
    static unsigned long long big = 1000000;
    static int smallN = -100;
    static long long bigN = -1000000;
    static quint64 bigq = 1E7;
    double dnum = big * 3.1459163 *1E6;
    static unsigned long int ulnum = 12345678890;
    static long int ulnumN = -12345678890;
    static bool bb = true;
    static QList<int> intlist;
    char c = 'c';
    static QMap<int, QList<int> > intlistmap;
    static QVector<bool> boolvec;
    static QSet<double> doubleset;
    static QHash<QString, bool> stringhash;

    bb = !bb;
    intlist.append(small);
    intlistmap.insert(small, intlist);
    int listsize = intlist.size();
    int mapsize = intlistmap.size();
    boolvec.append(bb);
    doubleset.insert(dnum);
    stringhash.insert(QString::number(small), bb);

    loggerF1->printToQDebug(true);
/**/
    *loggerF1 << "Writing a small number: " << small++ << UNQL::EOM;
    *loggerF1 << "Writing a big number:" << big++ << UNQL::EOM;
    *loggerF1 << "Writing a small negative number: " << smallN-- << UNQL::EOM;
    *loggerF1 << "Writing a big negative number:" << bigN-- << UNQL::EOM;
    *loggerF1 << "Writing a qbig number:" << bigq++ << UNQL::EOM;
    *loggerF1 << "Writing a ulbig number:" << ulnum++ << UNQL::EOM;
    *loggerF1 << "Writing a ulbig negative number:" << ulnumN-- << UNQL::EOM;
    *loggerF1 << "Writing a qbig double:" << dnum << UNQL::EOM;
    *loggerF1 << "Writing a boolean:" << bb << UNQL::EOM;
    *loggerF1 << "Writing a char:" << c << UNQL::EOM;
/*
    m_eltimer->restart();
    *loggerF1 << "Writing a " << listsize << " elem int list :" << intlist << UNQL::EOM;
    qDebug() << "time to write a " << listsize << " elem intlist:" << m_eltimer->nsecsElapsed() << "ns";
    m_eltimer->restart();
    *loggerF1 << "Writing a " << mapsize << " elem qmap of int list:" << intlistmap << UNQL::EOM;
    qDebug() << "time to write a " << mapsize << " elem intlistmap:" << m_eltimer->nsecsElapsed() << "ns";
*/

    *loggerF1 << "Writing a " << boolvec.size() << " elem bool vector:" << boolvec << UNQL::EOM;
    *loggerF1 << "Writing a " << doubleset.size() << " elem double set:" << doubleset << UNQL::EOM;
    *loggerF1 << "Writing a " << stringhash.size() << " elem string hash:" << stringhash << UNQL::EOM;
/**/

    //qDebug() << "written " << i * 2 << "KB to file...";
    //qDebug() << "is loggerF1 printing to qdebug()? " << loggerF1->printToQDebug();
    loggerF1->printToQDebug(false);
    //qDebug() << "is loggerF2 printing to qdebug()? " << loggerF2->printToStdOut();
    //loggerF2->printToStdOut(true);
    loggerF1->log(UNQL::LOG_INFO, ( QString("file text iteration ") + QString::number(i) + " " + QString().fill('a', 1500) ).toLatin1().constData() );
    //loggerF2->log(UNQL::LOG_INFO, ( QString("file2 text iteration") + QString::number(i) + " " + QString().fill('b', 500) ).toLatin1().constData() );
    //*loggerF2 << UNQL::LOG_WARNING << "Do you see me?" << UNQL::EOM;
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
    *loggerN1 << UNQL::LOG_INFO << ( QString("net info ") + QString::number(i) ) << UNQL::EOM;
    *loggerRS1 << UNQL::LOG_ALARM << ( QString("syslog alarm ") + QString::number(i) ) << UNQL::EOM;
    *loggerRS2 << UNQL::LOG_ERROR << ( QString("syslog udp error ") + QString::number(i) ) << UNQL::EOM;

#if(TEST_NET_MULTISRC)
    if (i%3 == 0 && i<40) {
        UniqLogger *ul = UniqLogger::instance();
        const LogWriter &nlw = ul->getNetworkWriter("127.0.0.1", 1674);
        qDebug() << "adding writer returned: " << ul->addWriterToLogger(loggerN2, nlw);
    }
    if (i%13 == 0 && i<40) {
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


