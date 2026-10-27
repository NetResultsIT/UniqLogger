#if defined(__aarch64__) && __has_include(<arm_acle.h>)
#include <arm_acle.h>
#endif
#include <QtTest>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSemaphore>
#include <QString>
#include <QThread>

#include <iostream>
#include <sstream>

#include <FileWriter.h>
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
    void test_priorityOverloadFormatsConsoleOutput();
    void test_refCountUnderflowOnUnattachedWriter();
    void test_removeUnattachedWriterDoesNotDestroyActiveWriter();
    void test_recreateFileLoggerBeforeDeleteLaterCompletes();
    void test_fileRotationStatePreservedAcrossTemporaryLoggers();
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

void TestRegressionCoverage::test_priorityOverloadFormatsConsoleOutput()
{
    // Logger::log(int priority, const char*, ...) must apply printf-style
    // formatting before echoing the message to stdout, exactly like the
    // Logger::log(const char*, ...) overload already does. Regression test
    // for a bug where the raw, unformatted format string (still containing
    // e.g. "%d") was sent to the console while the file writer correctly
    // received the formatted text.
    const QString fileName = uniqueName("regression_console_format") + ".log";
    removeIfExists(fileName);

    WriterConfig wc;
    wc.writerFlushSecs = 60;

    int ok = UNQL::UnqlErrorNoError;
    UniqLogger *ul = UniqLogger::instance(uniqueName("UL_consolefmt"), 1);
    Logger *logger = ul->createFileLogger("ConsoleFmtLogger", fileName, wc, ok);

    QCOMPARE(ok, UNQL::UnqlErrorNoError);

    logger->setVerbosityAcceptedLevel(UNQL::LOG_DBG);
    logger->printToStdOut(true);

    std::ostringstream captured;
    std::streambuf *oldBuf = std::cout.rdbuf(captured.rdbuf());
    logger->log(UNQL::LOG_INFO, "SIP call on line %d is now in terminated state", 42);
    std::cout.rdbuf(oldBuf);

    const QString consoleOutput = QString::fromStdString(captured.str());
    QVERIFY2(consoleOutput.contains("SIP call on line 42 is now in terminated state"), qPrintable(consoleOutput));
    QVERIFY2(!consoleOutput.contains("%d"), qPrintable(consoleOutput));

    ul->flushAllWriters();
    const QString fileContents = readFile(fileName);
    QVERIFY2(fileContents.contains("SIP call on line 42 is now in terminated state"), qPrintable(fileContents));

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

void TestRegressionCoverage::test_refCountUnderflowOnUnattachedWriter()
{
    const QString fileName = uniqueName("regression_refcount_underflow") + ".log";
    removeIfExists(fileName);

    WriterConfig wc;
    wc.writerFlushSecs = 60;

    int ok = UNQL::UnqlErrorNoError;
    UniqLogger *ul = UniqLogger::instance(uniqueName("UL_underflow"), 1);

    // 1. getFileWriter registers fw in m_DevicesMap with refcount = 0
    LogWriter &fw = ul->getFileWriter(fileName, wc, ok);
    QCOMPARE(ok, UNQL::UnqlErrorNoError);
    QPointer<LogWriter> fwGuard(&fw);

    Logger *unattached = ul->createDummyLogger("Unattached", wc);
    Logger *logger1 = ul->createDummyLogger("Logger1", wc);
    Logger *logger2 = ul->createDummyLogger("Logger2", wc);

    // 2. Removing fw from a logger it was never attached to returns -1,
    //    and must NOT decrement refcount from 0 to -1.
    QCOMPARE(ul->removeWriterFromLogger(unattached, fw), -1);

    // 3. Attach fw to two active loggers (logger1 and logger2).
    //    True refcount must be 2 (not 1 due to earlier underflow to -1).
    QCOMPARE(ul->addWriterToLogger(logger1, fw), 0);
    QCOMPARE(ul->addWriterToLogger(logger2, fw), 0);

    // 4. Delete only logger2; logger1 is STILL attached to fw (refcount == 2).
    delete logger2;
    ul->purgeUnusedWriters(); // Must NOT purge fw because logger1 is still attached (refcount == 2)
    QTest::qWait(100);

    const bool writerSurvivedWhileLogger1Active = !fwGuard.isNull();
    bool logger1StillWritesToFile = false;
    bool writerPurgedWhenDormant = false;
    if (writerSurvivedWhileLogger1Active) {
        logger1->log(UNQL::LOG_INFO, "logger1 still active after logger2 deleted");
        logger1->flush();
        logger1StillWritesToFile = readFile(fileName).contains("logger1 still active after logger2 deleted");
        delete logger1; // fw transitions to dormant (refcount == 1)
        ul->purgeUnusedWriters();
        QTest::qWait(100);
        writerPurgedWhenDormant = fwGuard.isNull();
    }
    delete unattached;
    QTest::qWait(100);
    removeIfExists(fileName);

    QVERIFY2(writerSurvivedWhileLogger1Active,
             "FileWriter was prematurely destroyed while logger1 was still attached because refcount underflowed");
    QVERIFY2(logger1StillWritesToFile,
             "FileWriter stopped logging for active logger1 after logger2 was deleted");
    QVERIFY2(writerPurgedWhenDormant,
             "Dormant FileWriter (refcount == 1) was not destroyed by purgeUnusedWriters()");
}

void TestRegressionCoverage::test_removeUnattachedWriterDoesNotDestroyActiveWriter()
{
    const QString fileName = uniqueName("regression_remove_unattached") + ".log";
    removeIfExists(fileName);

    WriterConfig wc;
    wc.writerFlushSecs = 60;

    int ok = UNQL::UnqlErrorNoError;
    UniqLogger *ul = UniqLogger::instance(uniqueName("UL_rem_unattached"), 1);

    Logger *logger1 = ul->createFileLogger("ActiveOwner", fileName, wc, ok);
    QCOMPARE(ok, UNQL::UnqlErrorNoError);

    LogWriter &fw = ul->getFileWriter(fileName, wc, ok);
    QPointer<LogWriter> fwGuard(&fw);

    Logger *logger2 = ul->createDummyLogger("UnrelatedLogger", wc);

    // fw is NOT attached to logger2: removeWriterFromLogger must fail with -1
    // and must NOT unregister/deactivate fw (which is still owned by logger1 with refcount == 2).
    const int res = ul->removeWriterFromLogger(logger2, fw);
    QCOMPARE(res, -1);

    ul->purgeUnusedWriters(); // If refcount had been decremented to 1, purgeUnusedWriters() would destroy fw!
    QTest::qWait(100);

    const bool writerStillAlive = !fwGuard.isNull();
    bool logger1StillWritesToFile = false;
    if (writerStillAlive) {
        logger1->log(UNQL::LOG_INFO, "still logging safely on logger1");
        logger1->flush();
        logger1StillWritesToFile = readFile(fileName).contains("still logging safely on logger1");
        delete logger1;
    }
    delete logger2;
    QTest::qWait(100);
    removeIfExists(fileName);

    QVERIFY2(writerStillAlive,
             "Failed removeWriterFromLogger(logger2, fw) destroyed FileWriter still in use by logger1");
    QVERIFY2(logger1StillWritesToFile,
             "Failed removeWriterFromLogger(logger2, fw) broke logging for active owner logger1");
}

void TestRegressionCoverage::test_recreateFileLoggerBeforeDeleteLaterCompletes()
{
    const QString fileName = uniqueName("regression_deletelater_race") + ".log";
    removeIfExists(fileName);

    WriterConfig wc;
    wc.writerFlushSecs = 60;

    int ok = UNQL::UnqlErrorNoError;
    UniqLogger *ul = UniqLogger::instance(uniqueName("UL_race"), 1);

    Logger *logger1 = ul->createFileLogger("TempLogger1", fileName, wc, ok);
    QCOMPARE(ok, UNQL::UnqlErrorNoError);
    logger1->log(UNQL::LOG_INFO, "message from TempLogger1");

    LogWriter &fw1 = ul->getFileWriter(fileName, wc, ok);
    QPointer<LogWriter> fw1Guard(&fw1);

    // Deterministically hold the worker thread event loop so that DeferredDelete
    // from deleteLater() is still pending when TempLogger2 opens the same file.
    QSemaphore workerEntered;
    QSemaphore allowWorkerToProceed;
    QMetaObject::invokeMethod(&fw1, [&]() {
        workerEntered.release();
        allowWorkerToProceed.tryAcquire(1, 1000);
    }, Qt::QueuedConnection);

    QVERIFY(workerEntered.tryAcquire(1, 1000));

    // Destroy the last logger on fileName and immediately create a new one on the same file
    delete logger1;
    Logger *logger2 = ul->createFileLogger("TempLogger2", fileName, wc, ok);
    QCOMPARE(ok, UNQL::UnqlErrorNoError);
    logger2->log(UNQL::LOG_INFO, "message from TempLogger2");

    LogWriter &fw2 = ul->getFileWriter(fileName, wc, ok);
    const bool twoDistinctWritersAliveOnSameFile = (!fw1Guard.isNull() && (&fw1 != &fw2));

    allowWorkerToProceed.release();
    QTest::qWait(100);

    logger2->flush();
    delete logger2;
    QTest::qWait(100);
    removeIfExists(fileName);

    QVERIFY2(!twoDistinctWritersAliveOnSameFile,
             "Two distinct FileWriter instances were simultaneously alive on the same file due to deleteLater() race");
}

void TestRegressionCoverage::test_fileRotationStatePreservedAcrossTemporaryLoggers()
{
    const QString baseName = uniqueName("regression_rot_state");
    const QString file0 = baseName + ".log";
    const QString file1 = baseName + "-1.log";
    const QString file2 = baseName + "-2.log";
    removeIfExists(file0);
    removeIfExists(file1);
    removeIfExists(file2);

    WriterConfig wc;
    wc.writerFlushSecs = 60;
    wc.maxFileSize = 1; // 1 MB
    wc.maxFileNum = 2;  // Keep at most 2 files
    wc.rotationPolicy = UNQL::HigherNumbersNewer;

    int ok = UNQL::UnqlErrorNoError;
    UniqLogger *ul = UniqLogger::instance(uniqueName("UL_rot_state"), 1);

    const QByteArray chunk(1000, 'A'); // 1 KB per log line

    // 1. First temporary logger writes > 1 MB and rotates from file0 to file1
    Logger *logger1 = ul->createFileLogger("RTP_Call1", file0, wc, ok);
    QCOMPARE(ok, UNQL::UnqlErrorNoError);
    for (int i = 0; i < 1100; ++i) {
        logger1->log(UNQL::LOG_INFO, "%s", chunk.constData());
    }
    logger1->flush();

    QVERIFY2(QFileInfo::exists(file0), qPrintable(file0));
    QVERIFY2(QFileInfo::exists(file1), qPrintable(file1));

    // Destroy the temporary logger (e.g. end of Call 1)
    delete logger1;
    QTest::qWait(100);

    // 2. Second temporary logger opens the same base file (e.g. Call 2)
    Logger *logger2 = ul->createFileLogger("RTP_Call2", file0, wc, ok);
    QCOMPARE(ok, UNQL::UnqlErrorNoError);
    logger2->log(UNQL::LOG_INFO, "CALL2_MARKER_LINE");
    for (int i = 0; i < 1100; ++i) {
        logger2->log(UNQL::LOG_INFO, "%s", chunk.constData());
    }
    logger2->flush();

    const QString file1Contents = readFile(file1);
    const bool call2WroteToFile1 = file1Contents.contains("CALL2_MARKER_LINE");
    const bool file2Created = QFileInfo::exists(file2);
    const bool oldestFile0Removed = !QFileInfo::exists(file0);

    delete logger2;
    QTest::qWait(100);
    removeIfExists(file0);
    removeIfExists(file1);
    removeIfExists(file2);

    QVERIFY2(call2WroteToFile1,
             "Temporary logger recreation lost m_rotationCurFileNumber and reset output back to file 0 instead of file -1");
    QVERIFY2(file2Created,
             "Second rotation did not advance to file -2");
    QVERIFY2(oldestFile0Removed,
             "Temporary logger recreation lost m_lastUsedFilenames and failed to remove oldest rotated file0");
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
