#include <QtTest>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QThread>

#include <Logger.h>
#include <LogWriter.h>
#include <UniqLogger.h>
#include <WriterConfig.h>

#ifdef ENABLE_UNQL_NETLOG
#include <QSslSocket>
#endif

class TestRegressionCoverage : public QObject
{
    Q_OBJECT

private:
    QString uniqueName(const QString &prefix) const;
    QString readFile(const QString &path) const;
    void removeIfExists(const QString &path) const;

private slots:
    void test_defaultOverloadPreservesPercentSigns();
    void test_defaultInstanceNameIsUnql();
    void test_instanceTagCanBeEnabledAndDisabled();
    void test_defaultTagOnlyAffectsNewLoggers();
    void test_twoInstancesKeepDistinctTagsOnSameOutput();
    void test_threadPoolThreadNamesUseInstanceTag();
    void test_writerConfigEqualityIncludesCompressionAlgo();
    void test_fileWriterWithoutExtensionDoesNotAppendDot();
#ifdef ENABLE_UNQL_NETLOG
    void test_tlsWritersUseSslSockets();
#endif
};

QString TestRegressionCoverage::uniqueName(const QString &prefix) const
{
    return QString("%1_%2").arg(prefix).arg(QDateTime::currentMSecsSinceEpoch());
}

QString TestRegressionCoverage::readFile(const QString &path) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed opening" << path << ":" << f.errorString();
        return QString();
    }
    return QString::fromUtf8(f.readAll());
}

void TestRegressionCoverage::removeIfExists(const QString &path) const
{
    if (QFile::exists(path)) {
        QVERIFY2(QFile::remove(path), qPrintable(QString("Failed removing %1").arg(path)));
    }
}

void TestRegressionCoverage::test_defaultOverloadPreservesPercentSigns()
{
    const QString fileName = uniqueName("regression_percent") + ".log";
    removeIfExists(fileName);

    WriterConfig wc;
    wc.writerFlushSecs = 60;

    int ok = UNQL::UnqlErrorNoError;
    UniqLogger *ul = UniqLogger::instance(uniqueName("UL_percent"), 1);
    Logger *logger = ul->createFileLogger("PercentLogger", fileName, wc, ok);

    QCOMPARE(ok, UNQL::UnqlErrorNoError);

    logger->log("%s", "80%% complete");
    ul->flushAllWriters();

    const QString contents = readFile(fileName);
    QVERIFY2(contents.contains("80%% complete"), qPrintable(contents));
    QVERIFY(!contents.contains("80% complete"));

    delete logger;
    removeIfExists(fileName);
}

void TestRegressionCoverage::test_writerConfigEqualityIncludesCompressionAlgo()
{
    WriterConfig wc1;
    WriterConfig wc2;

    wc2.compressionAlgo = 1;

    QVERIFY(wc1 != wc2);

    wc1.compressionAlgo = wc2.compressionAlgo;
    QVERIFY(wc1 == wc2);
}

void TestRegressionCoverage::test_defaultInstanceNameIsUnql()
{
    UniqLogger *ul = UniqLogger::instance();
    QCOMPARE(ul->instanceName(), QString(DEF_UNQL_INSTANCE_NAME));
}

void TestRegressionCoverage::test_instanceTagCanBeEnabledAndDisabled()
{
    const QString fileName = uniqueName("regression_tag_toggle") + ".log";
    removeIfExists(fileName);

    WriterConfig wc;
    wc.writerFlushSecs = 60;

    UniqLogger *ul = UniqLogger::instance();
    ul->setDefaultPrintTag(false);

    int ok = UNQL::UnqlErrorNoError;
    Logger *logger = ul->createFileLogger("TaggedLogger", fileName, wc, ok);

    QCOMPARE(ok, UNQL::UnqlErrorNoError);

    logger->log(UNQL::LOG_INFO, "without tag");
    logger->printTag(true);
    logger->log(UNQL::LOG_INFO, "with tag");
    logger->printTag(false);
    logger->log(UNQL::LOG_INFO, "without tag again");
    ul->flushAllWriters();

    const QString contents = readFile(fileName);
    QVERIFY2(contents.contains("[UNQL] ["), qPrintable(contents));
    QVERIFY2(contents.contains("with tag"), qPrintable(contents));
    QVERIFY2(contents.contains("without tag"), qPrintable(contents));
    QVERIFY2(contents.contains("without tag again"), qPrintable(contents));
    QVERIFY2(contents.indexOf("[UNQL] [") < contents.indexOf("with tag"), qPrintable(contents));

    const QStringList lines = contents.split('\n', Qt::SkipEmptyParts);
    QVERIFY(lines.count() >= 3);
    QVERIFY2(!lines.at(lines.count() - 3).startsWith("[UNQL] "), qPrintable(lines.at(lines.count() - 3)));
    QVERIFY2(lines.at(lines.count() - 2).startsWith("[UNQL] "), qPrintable(lines.at(lines.count() - 2)));
    QVERIFY2(!lines.at(lines.count() - 1).startsWith("[UNQL] "), qPrintable(lines.at(lines.count() - 1)));

    delete logger;
    ul->setDefaultPrintTag(false);
    removeIfExists(fileName);
}

void TestRegressionCoverage::test_defaultTagOnlyAffectsNewLoggers()
{
    const QString fileName1 = uniqueName("regression_default_tag_old") + ".log";
    const QString fileName2 = uniqueName("regression_default_tag_new") + ".log";
    removeIfExists(fileName1);
    removeIfExists(fileName2);

    WriterConfig wc;
    wc.writerFlushSecs = 60;

    UniqLogger *ul = UniqLogger::instance(uniqueName("UL_default_tag"), 1);
    ul->setDefaultPrintTag(false);

    int ok1 = UNQL::UnqlErrorNoError;
    Logger *logger1 = ul->createFileLogger("LoggerOne", fileName1, wc, ok1);
    QCOMPARE(ok1, UNQL::UnqlErrorNoError);
    QVERIFY(!logger1->tagPrintingEnabled());

    ul->setDefaultPrintTag(true);

    int ok2 = UNQL::UnqlErrorNoError;
    Logger *logger2 = ul->createFileLogger("LoggerTwo", fileName2, wc, ok2);
    QCOMPARE(ok2, UNQL::UnqlErrorNoError);
    QVERIFY(logger2->tagPrintingEnabled());
    QVERIFY(!logger1->tagPrintingEnabled());

    logger1->log(UNQL::LOG_INFO, "old logger");
    logger2->log(UNQL::LOG_INFO, "new logger");
    ul->flushAllWriters();

    const QString contents1 = readFile(fileName1);
    const QString contents2 = readFile(fileName2);
    QVERIFY2(!contents1.contains("[UL_default_tag"), qPrintable(contents1));
    QVERIFY2(contents2.contains("[UL_default_tag"), qPrintable(contents2));

    delete logger1;
    delete logger2;
    ul->setDefaultPrintTag(false);
    removeIfExists(fileName1);
    removeIfExists(fileName2);
}

void TestRegressionCoverage::test_twoInstancesKeepDistinctTagsOnSameOutput()
{
    const QString fileName = uniqueName("regression_two_instance_tags") + ".log";
    removeIfExists(fileName);

    WriterConfig wc;
    wc.writerFlushSecs = 60;

    const QString instanceName1 = uniqueName("UL_alpha");
    const QString instanceName2 = uniqueName("UL_beta");

    UniqLogger *ul1 = UniqLogger::instance(instanceName1, 1);
    UniqLogger *ul2 = UniqLogger::instance(instanceName2, 1);
    ul1->setDefaultPrintTag(true);
    ul2->setDefaultPrintTag(true);

    int ok1 = UNQL::UnqlErrorNoError;
    int ok2 = UNQL::UnqlErrorNoError;
    Logger *logger1 = ul1->createFileLogger("LoggerAlpha", fileName, wc, ok1);
    Logger *logger2 = ul2->createFileLogger("LoggerBeta", fileName, wc, ok2);

    QCOMPARE(ok1, UNQL::UnqlErrorNoError);
    QCOMPARE(ok2, UNQL::UnqlErrorNoError);
    QVERIFY(logger1->tagPrintingEnabled());
    QVERIFY(logger2->tagPrintingEnabled());

    logger1->log(UNQL::LOG_INFO, "message from alpha");
    logger2->log(UNQL::LOG_INFO, "message from beta");
    ul1->flushAllWriters();
    ul2->flushAllWriters();

    const QString contents = readFile(fileName);
    const QString expectedAlpha = "[" + instanceName1 + "] [";
    const QString expectedBeta = "[" + instanceName2 + "] [";

    QVERIFY2(contents.contains(expectedAlpha), qPrintable(contents));
    QVERIFY2(contents.contains(expectedBeta), qPrintable(contents));
    QVERIFY2(contents.contains("message from alpha"), qPrintable(contents));
    QVERIFY2(contents.contains("message from beta"), qPrintable(contents));

    const QStringList lines = contents.split('\n', Qt::SkipEmptyParts);
    bool foundAlpha = false;
    bool foundBeta = false;
    for (const QString &line : lines) {
        if (line.contains("message from alpha")) {
            QVERIFY2(line.startsWith("[" + instanceName1 + "] "), qPrintable(line));
            foundAlpha = true;
        }
        if (line.contains("message from beta")) {
            QVERIFY2(line.startsWith("[" + instanceName2 + "] "), qPrintable(line));
            foundBeta = true;
        }
    }

    QVERIFY2(foundAlpha, qPrintable(contents));
    QVERIFY2(foundBeta, qPrintable(contents));

    delete logger1;
    delete logger2;
    ul1->setDefaultPrintTag(false);
    ul2->setDefaultPrintTag(false);
    removeIfExists(fileName);
}

void TestRegressionCoverage::test_threadPoolThreadNamesUseInstanceTag()
{
    const QString instanceName = uniqueName("UL_threads");
    UniqLogger *ul = UniqLogger::instance(instanceName, 2);

    const auto threads = ul->findChildren<QThread*>();
    QCOMPARE(threads.count(), 2);

    QSet<QString> names;
    for (QThread *thread : threads) {
        names.insert(thread->objectName());
    }

    const QSet<QString> expectedNames = {
        instanceName + "_LOGW T_0",
        instanceName + "_LOGW T_1"
    };

    QCOMPARE(names, expectedNames);
}

void TestRegressionCoverage::test_fileWriterWithoutExtensionDoesNotAppendDot()
{
    const QString fileName = uniqueName("regression_no_extension");
    const QString dottedFileName = fileName + ".";

    removeIfExists(fileName);
    removeIfExists(dottedFileName);

    WriterConfig wc;
    wc.writerFlushSecs = 60;

    int ok = UNQL::UnqlErrorNoError;
    UniqLogger *ul = UniqLogger::instance(uniqueName("UL_noext"), 1);
    Logger *logger = ul->createFileLogger("NoExtensionLogger", fileName, wc, ok);

    QCOMPARE(ok, UNQL::UnqlErrorNoError);

    logger->log(UNQL::LOG_INFO, "message");
    ul->flushAllWriters();

    QVERIFY2(QFileInfo::exists(fileName), qPrintable(QString("Expected file missing: %1").arg(fileName)));
    QVERIFY2(!QFileInfo::exists(dottedFileName), qPrintable(QString("Unexpected dotted file exists: %1").arg(dottedFileName)));

    delete logger;
    removeIfExists(fileName);
    removeIfExists(dottedFileName);
}

#ifdef ENABLE_UNQL_NETLOG
void TestRegressionCoverage::test_tlsWritersUseSslSockets()
{
    WriterConfig wc;
    wc.netProtocol = UNQL::TLS;
    wc.reconnectionSecs = 60;

    int ok = UNQL::UnqlErrorNoError;
    UniqLogger *ul = UniqLogger::instance(uniqueName("UL_tls"), 1);
    LogWriter &writer = ul->getNetworkWriter("127.0.0.1", 6514, wc, ok);

    QCOMPARE(ok, UNQL::UnqlErrorNoError);

    const auto sslSockets = writer.findChildren<QSslSocket*>();
    QVERIFY2(!sslSockets.isEmpty(), "TLS network writer should own at least one QSslSocket");
}
#endif

QTEST_MAIN(TestRegressionCoverage)

#include "tst_testregressioncoverage.moc"
