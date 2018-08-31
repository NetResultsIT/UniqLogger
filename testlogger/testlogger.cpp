#include "testlogger.h"

#include <QDebug>
#include <QTimer>

//#define TESTNET 1
//#define TESTDB 1

using namespace UNQL;

testlogger_gui::testlogger_gui(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    loggerFile = nullptr;
    loggerNet2 = nullptr;
    loggerConsole = nullptr;
    loggerNet1 = nullptr;
    logger5 = nullptr;
    m_dummyLoggerPtr = nullptr;

    timer = new QTimer();

    connect(timer, SIGNAL(timeout()), this, SLOT(timedLog()));

    WriterConfig wc2;
    wc2.maxFileNum = 3;
    wc2.maxFileSize = 2;

    UniqLogger *ul = UniqLogger::instance();
    ul->setEncasingChars('(',')');
    ul->setSpacingChar('.');
    loggerFile = ul->createFileLogger("test", "log.txt", wc2);
    ConsoleColorScheme cs;
    cs.setDefaultColor(red);
    loggerConsole = ul->createConsoleLogger("CONSOLE", cs);

#ifdef TESTNET
    WriterConfig wconf;
    wconf.maxMessageNum = 10;
    loggerNet2 = ul->createNetworkLogger("test", "127.0.0.1",1675);
    loggerNet1 = ul->createNetworkLogger("testadsa", "127.0.0.1",1674, wconf);
    //testThreadedNetLogger("127.0.0.1", 1674);
#endif

#ifdef TESTDB
    logger5 = ul->createDbLogger("bdtest","log.db");
#endif

    m_dummyLoggerPtr = ul->createDummyLogger("DUMMY_LOGGER_TEST");
    *m_dummyLoggerPtr << UNQL::LOG_CRITICAL << "this is a dummy logger test" << UNQL::EOM;

    //test_strangeString(ul);

    /* SET LOGGER NAMES */
    if (!loggerNet2) {
        qDebug() << "Error creating logger net2";
    }
    else
        loggerNet2->setModuleName("NET2");

    if (!loggerNet1) {
        qDebug() << "Error creating logger net1";
    }
    else
        loggerNet1->setModuleName("NET1");

    loggerFile->setModuleName("FILE");
    loggerConsole->setModuleName("REDCONSOLE");


    //test_dummylog(ul);
}

testlogger_gui::~testlogger_gui()
{

}



void
testlogger_gui::onStartLog()
{
    timer->start(1000);
}



void
testlogger_gui::onStopLog()
{
    timer->stop();
}



void
testlogger_gui::testThreadedNetLogger(const QString &ip, int port)
{
    Q_UNUSED(ip);
    Q_UNUSED(port);

    TestThreadObject *tobj1 = new TestThreadObject(2000);
    tobj1->l = loggerNet1;
    TestThreadObject *tobj2 = new TestThreadObject(3000);
    tobj2->l = loggerNet1;

    QThread *t1, *t2;
    t1 = new QThread(this);
    t2 = new QThread(this);
    tobj1->moveToThread(t1);
    tobj2->moveToThread(t2);

    t1->start();
    t2->start();
}


void
testlogger_gui::timedLog()
{
    qDebug() << Q_FUNC_INFO;
    static int i=0;
    loggerFile->log(UNQL::LOG_INFO, (QString("file text ") + QString::number(i) + QString().fill('a',1000)).toLatin1().constData() );
    loggerConsole->log(UNQL::LOG_INFO, (QString("console text ")+QString::number(i)).toLatin1().constData());

#ifdef TESTNET
    loggerNet2->log(UNQL::LOG_CRITICAL, (QString("nt") + QString::number(i)).toLatin1().constData());
#endif

#ifdef TESTDB
	logger5->log(UNQL::LOG_INFO,"Testing db log");
#endif

    double dd = qrand();
    loggerConsole->monitor(dd, "nt", "test variable dd");
    i++;
    if (i==5) {
        UniqLogger *ul = UniqLogger::instance();
        ul->changeMonitorVarStatus("nt", true);
    }

    if (i==15) {
        UniqLogger *ul = UniqLogger::instance();
        ul->changeMonitorVarStatus("nt", false);
    }

    *m_dummyLoggerPtr << UNQL::LOG_CRITICAL << "this is a dummy logger test" << UNQL::EOM;
}


void testlogger_gui::test_strangeString(UniqLogger *ul)
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
    ConsoleColorScheme cs;
    cs.setDefaultColor(white);
    LogWriter &lw = ul->getConsoleWriter(cs);

    ul->addWriterToLogger(loggerFile,lw);
    *loggerFile << s << UNQL::LOG_CRITICAL << s1 << UNQL::LOG_FATAL << 1.0 << "hello" << UNQL::eom;
    *loggerFile << s2 << UNQL::eom;
    *loggerFile << s3 << UNQL::eom;
}




void testlogger_gui::test_dummylog(UniqLogger *ul)
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
