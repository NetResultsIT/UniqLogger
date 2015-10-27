#include <QApplication>
#include "testlogger.h"
#include "testlogger2.h"

#ifdef WIN32
#define sleep(x) Sleep(1000*x)
#include <Windows.h>
#else
#include <unistd.h>
#endif

static void test_logger_leak()
{
    UniqLogger *ul = UniqLogger::instance("LOGGER_LEAK_TEST");
    ul->setTimeStampFormat("yyyy.MM.dd-hh:mm:ss:zzz");

    QSharedPointer<Logger> m_clog;
    QSharedPointer<Logger> m_flog;

    Logger* m_clogPtr = NULL;
    Logger* m_clogPtr2 = NULL;
    Logger* m_flogPtr = NULL;
    Logger* m_flogPtr2 = NULL;

    Logger* m_dummyLoggerPtr = NULL;

    while (1)
    {
        qDebug() << "creating loggers";
        m_clogPtr = ul->createConsoleLogger("LEAK_TEST", blue);
        m_clogPtr2 = ul->createConsoleLogger("LEAK_TEST1", green);
        m_flogPtr = ul->createFileLogger("FLEAK_TEST", "lt.txt");
        m_flogPtr2 = ul->createFileLogger("FLEAK_TEST1", "lt.txt");

        m_dummyLoggerPtr = ul->createDummyLogger("DUMMY_LEAK_TEST");

        qDebug() << "loggers created";

        *m_clogPtr << "hello" << UNQL::EOM;
        *m_clogPtr2 << "hello" << UNQL::EOM;
        
        *m_dummyLoggerPtr << UNQL::LOG_CRITICAL << "dummy log test" << UNQL::EOM;

        delete m_clogPtr;
        delete m_clogPtr2;
        delete m_flogPtr;
        delete m_flogPtr2;

        qDebug() << "loggers destroyed";
        QCoreApplication::processEvents();
        sleep(1);
    }
}


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

    qDebug() << "Ideal thread count reported: " << QThread::idealThreadCount();

#if (0)
    test_logger_leak();
#endif

#if (0)
    testlogger_gui w;
    w.show();
#endif

#if (1)
    testlogger_cli w;
#endif


	return a.exec();
}

