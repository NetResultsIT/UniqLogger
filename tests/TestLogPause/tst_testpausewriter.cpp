#include <QtTest>

// add necessary includes here
#include <UniqLogger.h>
#include <FileWriter.h>
#include <Logger.h>

class TestPauseWriter : public QObject
{
    Q_OBJECT

public:
    TestPauseWriter();
    ~TestPauseWriter();

private slots:
    void test_case1();

};

TestPauseWriter::TestPauseWriter()
{

}

TestPauseWriter::~TestPauseWriter()
{

}

void TestPauseWriter::test_case1()
{
    QString filename = "logpausetest.txt";
    if (QFile::exists(filename))
    {
        QFile::remove(filename);
    }

    WriterConfig wc;
    wc.writerFlushSecs = 1;
    wc.compressMessages = true;

    int ok;

    UniqLogger *ul = UniqLogger::instance("TestPause", 1);
    Logger *logger = ul->createFileLogger("PausingLogger", filename, wc, ok);

    QCOMPARE(ok, UNQL::UnqlErrorNoError);
    *logger << UNQL::LOG_INFO << "This is going to be logged" << UNQL::FLS;
    LogWriter &pFW = ul->getFileWriter(filename);
    pFW.pauseLogging(true);
    *logger << UNQL::LOG_CRITICAL << "This will NOT be logged" << UNQL::FLS;
    pFW.pauseLogging(false);
    *logger << UNQL::LOG_INFO << "Also this will be logged" << UNQL::FLS;
    pFW.flush();

    QFile f(filename);
    bool b = f.open(QIODevice::ReadOnly);
    Q_ASSERT(b);
    QString readstring = f.readAll();
    f.close();
    QVERIFY(readstring.contains("This is going to be logged"));
    QVERIFY(!readstring.contains("This will NOT be logged"));
    QVERIFY(readstring.contains("Also this will be logged"));
}

QTEST_APPLESS_MAIN(TestPauseWriter)

#include "tst_testpausewriter.moc"
