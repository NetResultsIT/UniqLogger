#include "testlogger.h"

#include <QDebug>
#include <QTimer>

#define TESTNET 1
//#define TESTDB 1



testlogger::testlogger(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    logger = 0;
    logger2 = 0;
    logger3 = 0;
    logger4 = 0;
    logger5 = 0;
    m_dummyLoggerPtr = 0;

    QTimer *timer = new QTimer();
    timer->start(1000);

    connect(timer, SIGNAL(timeout()), this, SLOT(timedLog()));

    WriterConfig wc2;
    wc2.maxFileNum = 3;
    wc2.maxFileSize = 2;

    UniqLogger *ul = UniqLogger::instance();
    ul->setEncasingChars('(',')');
    ul->setSpacingChar('.');
    logger = ul->createFileLogger("test", "log.txt", wc2);
    logger3 = ul->createConsoleLogger("CONSOLE",red);

#ifdef TESTNET
    WriterConfig wconf;
    wconf.maxMessageNum = 10;
    logger2 = ul->createNetworkLogger("test", "127.0.0.1",1675);
    logger4 = ul->createNetworkLogger("testadsa", "127.0.0.1",1674, wconf);
    const LogWriter &nlw = ul->getNetworkWriter("127.0.0.1",1674);
    //ul->addWriterToLogger(logger,nlw);

    //testThreadedNetLogger("127.0.0.1", 1674);
#endif

#ifdef TESTDB
    logger5 = ul->createDbLogger("bdtest","log.db");
#endif
    //logger4 = ul->createFileLogger("CONSOLE","log.txt");
    //logger4 = ul->createConsoleLogger("CONSOLE",magenta);

    m_dummyLoggerPtr = ul->createDummyLogger("DUMMY_LOGGER_TEST");
    *m_dummyLoggerPtr << UNQL::LOG_CRITICAL << "this is a dummy logger test" << UNQL::EOM;

    //test_strangeString(ul);

    /* SET LOGGER NAMES */
	if (!logger2) {
		qDebug() << "Error creating logger module2";
	}
	else
		logger2->setModuleName("NET");

    if (!logger4) {
		qDebug() << "Error creating logger module4";
	}
	else
        logger4->setModuleName("NETCONSOLE");

	logger->setModuleName("FILE");
	logger3->setModuleName("REDCONSOLE");


    //test_dummylog(ul);
}

testlogger::~testlogger()
{

}


void
testlogger::testThreadedNetLogger(const QString &ip, int port)
{
    Q_UNUSED(ip);
    Q_UNUSED(port);

    TestThreadObject *tobj1 = new TestThreadObject(2000);
    tobj1->l = logger4;
    TestThreadObject *tobj2 = new TestThreadObject(3000);
    tobj2->l = logger4;

    QThread *t1, *t2;
    t1 = new QThread(this);
    t2 = new QThread(this);
    tobj1->moveToThread(t1);
    tobj2->moveToThread(t2);

    t1->start();
    t2->start();
}


void
testlogger::timedLog()
{
    qDebug() << Q_FUNC_INFO;
    static int i=0;
    logger->log(UNQL::LOG_INFO, (QString("file text ") + QString::number(i) + QString().fill('a',1000)).toLatin1().constData() );
    logger3->log(UNQL::LOG_INFO, (QString("console text ")+QString::number(i)).toLatin1().constData());

#ifdef TESTNET
    logger2->log(UNQL::LOG_CRITICAL,(QString("nt")+QString::number(i)).toLatin1().constData());
    //logger4->log(UNQL::LOG_INFO, (QString("net text ")+QString::number(i)).toLatin1().constData());
#endif

#ifdef TESTDB
	logger5->log(UNQL::LOG_INFO,"Testing db log");
#endif

    double dd = qrand();
	logger3->monitor(dd,"nt","test variable dd");
    i++;
    if (i==5) {
        UniqLogger *ul = UniqLogger::instance();
        ul->changeMonitorVarStatus("nt",true);
    }

    if (i==15) {
        UniqLogger *ul = UniqLogger::instance();
        ul->changeMonitorVarStatus("nt",false);
    }

    *m_dummyLoggerPtr << UNQL::LOG_CRITICAL << "this is a dummy logger test" << UNQL::EOM;
}


void testlogger::test_strangeString(UniqLogger *ul)
{
    QString s = "cioaaaa";
    QString s1 = "pipo";
    QString s2 = "http://r6---sn-o5hxm-4jve.googlevideo.com/videoplayback?fexp=935611%2C924614%2C932242%2C929429%2C916617%2C909717%2C924616%2C932295%2C936912%2C936910%2C923305%2C936913%2C907231%2C907240%2C921090&itag=18&key=yt5&ip=77.72.27.3&sver=3&ms=au&source=youtube&mt=1388396952&ratebypass=yes&id=22207cdc28fc9d99&expire=1388419131&upn=sqbm2oECPhc&ipbits=0&mv=u&sparams=id%2Cip%2Cipbits%2Citag%2Cratebypass%2Csource%2Cupn%2Cexpire&signature=3248A19220B9CA2457A9F3A3959CD94782E2F96D.BF776680D6BAF9946EA3C46911068156D587C448";
    QString s3 = s2.toUtf8().toPercentEncoding();

    QVariant v2 = s2;
    QVariant v3 = s3;
    qDebug() << s2 << "\n---\n" << s3;
    qDebug() << v2 << "\n---\n" << v3;
    qDebug() << v2.toString() << "\n---\n" << v3.toString();
    const LogWriter &lw = ul->getConsoleWriter(white);

    ul->addWriterToLogger(logger,lw);
    *logger << s << UNQL::LOG_CRITICAL << s1 << UNQL::LOG_FATAL << 1.0 << "hello" << UNQL::eom;
    *logger << s2 << UNQL::eom;
    *logger << s3 << UNQL::eom;
}




void testlogger::test_dummylog(UniqLogger *ul)
{
    qDebug() << "Testing dummy logger 50 times";
    Logger *m_dummyLoggerPtr2;
    for(int i=0; i < 50; i++) {

        m_dummyLoggerPtr2 = ul->createDummyLogger(QString("DUMMY_LOGGER_TEST")+QString::number(i));
        for(int j=0; j< 100; j++) {
            *m_dummyLoggerPtr2 << UNQL::LOG_CRITICAL << "this is a dummy logger test" << UNQL::EOM;
        }
        delete m_dummyLoggerPtr2;
    }


    qDebug() << "finished testing dummy create/delete";
}
