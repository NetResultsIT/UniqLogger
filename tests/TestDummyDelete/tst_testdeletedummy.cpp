#include <QtTest>

// add necessary includes here
#include <UniqLogger.h>
#include <FileWriter.h>
#include <Logger.h>

class TestDeleteDummy : public QObject
{
    Q_OBJECT

public:
    TestDeleteDummy();
    ~TestDeleteDummy();

private slots:
    void test_case1();

};

TestDeleteDummy::TestDeleteDummy()
{

}

TestDeleteDummy::~TestDeleteDummy()
{

}

void TestDeleteDummy::test_case1()
{
    QString filename = "logdummytest.txt";
    if (QFile::exists(filename))
    {
        QFile::remove(filename);
    }

    WriterConfig wc;
    wc.writerFlushSecs = 1;
    wc.compressMessages = true;

    int ok = UNQL::UnqlErrorNoError;

    UniqLogger *ul = UniqLogger::instance("TestDummyDelete", 1);
    Logger *logger = ul->createDummyLogger("DummyLogger", wc);

    QCOMPARE(ok, UNQL::UnqlErrorNoError);
    *logger << UNQL::LOG_INFO << "This is going to be logged" << UNQL::EOM;

    QTest::qSleep(20);
    delete logger;
    QTest::qSleep(1000);
    //QThread::usleep(10000);
    qDebug() << QDateTime::currentDateTime() << "About to exit test" << Q_FUNC_INFO;
}

QTEST_MAIN(TestDeleteDummy)

#include "tst_testdeletedummy.moc"
