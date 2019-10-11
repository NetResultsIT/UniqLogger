#include <QtTest>
#include <QCoreApplication>

// add necessary includes here
#include "LogWriter.h"

class TestWriterConfig : public QObject
{
    Q_OBJECT

public:
    TestWriterConfig();
    ~TestWriterConfig();

private slots:
    void test_case1();
    void test_case2();
};

TestWriterConfig::TestWriterConfig()
{

}

TestWriterConfig::~TestWriterConfig()
{

}

void TestWriterConfig::test_case1()
{
    WriterConfig wc1, wc2;
    QVERIFY(wc1 == wc2);
    //wc2.reconnectionSecs=200;
    wc2.maxFileNum = 3;
    wc2.maxFileSize = 10;
    QVERIFY(wc1 != wc2);
    //wc1.reconnectionSecs=200;
    wc1.maxFileNum = 3;
    wc1.maxFileSize = 10;
    QVERIFY(wc1 == wc2);
}

#include "Logger.h"
#include "UniqLogger.h"
void TestWriterConfig::test_case2()
{
    WriterConfig wc, wc2, wc3;
    wc2.reconnectionSecs = 111;
    UniqLogger *unql = UniqLogger::instance();
    Logger *l = unql->createFileLogger("LOG1", "log.txt", wc);
    Logger *l2 = unql->createFileLogger("LOG2", "log.txt", wc2);
    Logger *l3 = unql->createFileLogger("LOG3", "log.txt", wc3);
    *l << UNQL::LOG_INFO << "Log1" << UNQL::FLS;
    *l2 << UNQL::LOG_INFO << "Log2" << UNQL::FLS;
    *l3 << UNQL::LOG_INFO << "Log3" << UNQL::FLS;
}

QTEST_MAIN(TestWriterConfig)

#include "tst_writerconfig.moc"
