#include <QtTest>

#include <FileWriter.h>
#include <Logger.h>
#include <UniqLogger.h>

class TestMessagesCompression : public QObject
{
    Q_OBJECT

public:
    TestMessagesCompression();
    ~TestMessagesCompression();

private slots:
    void test_FileWriterMessagesCompression();

};

TestMessagesCompression::TestMessagesCompression()
{

}

TestMessagesCompression::~TestMessagesCompression()
{

}

void TestMessagesCompression::test_FileWriterMessagesCompression()
{
    QString filename = "FileWriterMsgCompression.txt";
    QString logName = "TestMessagesCompressionFileWrite";

    if (QFile::exists(filename))
    {
        QFile::remove(filename);
    }

    WriterConfig wc;

    wc.writerFlushSecs = 80;
    wc.compressMessages = true;

    int ok;

    UniqLogger *ul = UniqLogger::instance("FileWriterMessagesCompression", 1);
    Logger *logger = ul->createFileLogger(logName, filename, wc, ok);
    logger->setVerbosityAcceptedLevel(UNQL::LOG_INFO);

    QCOMPARE(ok, UNQL::UnqlErrorNoError);

    qDebug() <<"Elaborating...";

    QString initDateTime;
    QString endDateTime;
    QString body = "Test messaggi uguali";


    int n = 10;

    for (int i = 0; i < n; ++i)
    {
        if (i == 0)
            initDateTime = QDateTime::currentDateTime().toString(DEF_UNQL_TSTAMP_FMT);
        if (i == n-1)
            endDateTime = QDateTime::currentDateTime().toString(DEF_UNQL_TSTAMP_FMT);

        *logger << UNQL::LOG_INFO << body << UNQL::EOM;
        QTest::qSleep(1000);
    }

    ul->flushAllWriters();


    QFile logFile(filename);
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));

    QTextStream in(&logFile);
    QString line;

    QString msgToCheck = "[" + initDateTime + " - " + endDateTime + "] [" + logName + "] [INFO] "
            + body + " [repeated " + QString::number(n) + " times]";

    qDebug() << msgToCheck;

    bool found = false;
    while (!(line = in.readLine()).isNull())
    {
        if (line == msgToCheck)
            found = true;
    }

    QVERIFY(found);

    if (logger)
    {
        delete logger;
    }
}

QTEST_APPLESS_MAIN(TestMessagesCompression)

#include "tst_testmessagescompression.moc"
