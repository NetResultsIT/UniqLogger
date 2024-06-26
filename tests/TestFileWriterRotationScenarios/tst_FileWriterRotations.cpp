#include "tst_FileWriterRotations.h"

#include <FileWriter.h>

//needed for compression name function
#include "ext/filecompressor/src/NrFileCompressor.h"

#include <QtTest/QtTest>

FileWriterRotations::FileWriterRotations(WriterConfig wc)
    : FileWriter(wc)
{

}

// UTILITY FUNX

void createFile(QString fname)
{
    QFile f(fname);
    f.open(QIODevice::WriteOnly);
    f.close();
}


void writeToFile(QString fname, int megabytes)
{
    QFile f(fname);
    QString s;
    s.fill('x', megabytes * 1E6);
    f.open(QIODevice::WriteOnly);
    f.write(s.toUtf8());
    f.close();
}


void deleteFile(QString fname)
{
    QFile f(fname);
    qDebug() << "about to remove" << fname;
    bool b = f.remove();
    if (!b) {
        qWarning() << "Could not remove file " << fname << " - " << f.errorString();
    }
}


QString getFileWithFullPath(const QString filename, QString path=(QDir::currentPath() + QDir::separator()))
{
    QChar sep = '/';
    path = QDir::fromNativeSeparators(path);
    if (!path.endsWith(sep) && !filename.startsWith(sep))
        path.append(sep);
    qDebug() << path << filename;
    return path + filename;
}


QString getCompressedExten(int alg) {
    QString compressedExt;
    switch (alg) {
        case 1:
            qDebug() << "Using Gzip compression";
            compressedExt = ".gz";
            break;
        case 2:
            qDebug() << "Using ZIP compression";
            compressedExt = ".zip";
            break;
        default:
            qDebug() << "NOT using compression";
    }
    return compressedExt;
}


QString getCompressedFilename(QString fname, int compressionAlg)
{
    QString s = NrFileCompressor::getCompressedFilename(fname, static_cast<NrFileCompressor::compressedFileFormatEnum>(compressionAlg));
    return s;
}

//END OF UTILITY FUNX

void FileWriterRotations::init()
{
    qDebug() << Q_FUNC_INFO;
    QString testName = QTest::currentTestFunction();
    m_currentSubFolder = testName;
    m_filenames.clear();

    if (!QDir(m_currentSubFolder).exists()) {
        QDir::current().mkdir(m_currentSubFolder);
    }

    resetLastUsedFilenames();
    setTestingCurrentDateTime(QDateTime());
    m_Config = WriterConfig();
}

void FileWriterRotations::cleanup()
{
    cleanupFiles(m_filenames);
    bool b = QDir::current().rmdir(m_currentSubFolder);
    QVERIFY(b);
}

void FileWriterRotations::cleanupFiles(QStringList filenames) {
    foreach (QString s, filenames) {
        deleteFile(m_currentSubFolder + "/" + s);
        QVERIFY(!QFile::exists(m_currentSubFolder + "/" + s));
    }
}


void
FileWriterRotations::testCalculateLogInfo()
{
    LogFileInfo lfi;
    //this->setOutputFile("log.txt");

    lfi = this->calculateLogFilePattern("log.txt");
    qDebug() << "basename: " << lfi.basename;
    qDebug() << "path: " << lfi.path;
    qDebug() << "ext: " << lfi.extension;
    qDebug() << "pattern: " << lfi.pattern;
    QVERIFY(lfi.basename == "log");
    QVERIFY(lfi.basename != "XXX");
    QVERIFY(lfi.extension == "txt");
    QVERIFY(lfi.path == QDir::currentPath() + "/");

    lfi = this->calculateLogFilePattern("/Users/lamonica/testlog.txt");
    qDebug() << "basename: " << lfi.basename;
    qDebug() << "path: " << lfi.path;
    qDebug() << "ext: " << lfi.extension;
    qDebug() << "pattern: " << lfi.pattern;
    QVERIFY(lfi.basename == "testlog");
    QVERIFY(lfi.basename != "XXX");
    QVERIFY(lfi.extension == "txt");
    QVERIFY(lfi.path == "/Users/lamonica/");


    lfi = this->calculateLogFilePattern("/Users/lamonica/noextlog");
    qDebug() << "basename: " << lfi.basename;
    qDebug() << "path: " << lfi.path;
    qDebug() << "ext: " << lfi.extension;
    qDebug() << "pattern: " << lfi.pattern;
    QVERIFY(lfi.basename == "noextlog");
    QVERIFY(lfi.basename != "XXX");
    QVERIFY(lfi.extension == "");
    QVERIFY(lfi.path == "/Users/lamonica/");


    lfi = this->calculateLogFilePattern("/Users/f.lamonica/a-log-with.more-than.one.dot.log");
    qDebug() << "basename: " << lfi.basename;
    qDebug() << "path: " << lfi.path;
    qDebug() << "ext: " << lfi.extension;
    qDebug() << "pattern: " << lfi.pattern;
    QVERIFY(lfi.basename == "a-log-with.more-than.one.dot");
    QVERIFY(lfi.basename != "XXX");
    QVERIFY(lfi.extension == "log");
    QVERIFY(lfi.path == "/Users/f.lamonica/");
}


void
FileWriterRotations::testRemoveOldestFiles()
{
    //QSKIP("Adjusting code...");

    QString path = QDir::currentPath() + QDir::separator() + m_currentSubFolder + "/";
    m_filenames << "log.txt"
                << "log-1.txt"
                << "log-2.txt"
                << "log-3.txt";

    cleanupFiles(m_filenames);

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy= UNQL::NoTimeRotation;

    foreach (QString s, m_filenames) {
        createFile(path + s);
    }

    for (int i=0; i<m_filenames.size(); i++)
        m_lastUsedFilenames.enqueue(path + m_filenames[i]);

    qDebug() << m_lastUsedFilenames;
    removeOldestFiles();
    qDebug() << m_lastUsedFilenames;

    QVERIFY(!QFile::exists(path + m_filenames[0]));
    QVERIFY(QFile::exists(path + m_filenames[1]));
    QVERIFY(QFile::exists(path + m_filenames[2]));
    QVERIFY(QFile::exists(path + m_filenames[3]));
}


void
FileWriterRotations::testRemoveLeftovers()
{
    int maxfiles = 4;
    QList<QDateTime> dtlist;

    QString initialdatetime = "2021-03-19T01:57:32";
    UNQL::FileRotationTimePolicyType timerotPolicy = UNQL::HourlyRotation;

    QDateTime dt = QDateTime::fromString(initialdatetime, Qt::ISODate);
    QVERIFY(dt.isValid());

    //config
    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 0;
    //m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy = timerotPolicy;
    //m_Config.compressionAlgo = compressionAlg;
    m_Config.maxMinutes = 1;

    dt = adjustDateTimeForFileSuffix(dt);

    int rotation_seconds = 60;
    if (timerotPolicy == UNQL::HourlyRotation)
        rotation_seconds = 3600;
    if (timerotPolicy == UNQL::DailyRotation)
        rotation_seconds = 3600 * 24;

    QString path = QDir::currentPath() + QDir::separator() + m_currentSubFolder + "/";
    //define the filenames each more in the future than previous
    for (int i=0; i<maxfiles; i++) {
        dtlist << dt.addSecs(i * rotation_seconds); //add one minute, hour or day
        QVERIFY(dtlist[i].isValid());
        m_filenames << "log" + dtlist[i].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
        createFile(path + m_filenames[i]);
    }

    //go ahead in the future 3 hours
    setTestingCurrentDateTime(dt.addSecs(3*rotation_seconds));
    //we configured that we want just 3 files so when we set the new output file we should get rid of
    //the first two entries of filenames

    setOutputFile(path + "log.txt");
    removeLeftoversFromPreviousRun();
    qDebug() << "Looking for file (should not exist) " << path + m_filenames[0];
    QVERIFY(!QFile::exists(path + m_filenames[0]));
    qDebug() << "Looking for file (should not exist) " << path + m_filenames[1];
    QVERIFY(!QFile::exists(path + m_filenames[1]));
    qDebug() << "Looking for file " << path + m_filenames[2];
    QVERIFY(QFile::exists(path + m_filenames[2]));
    qDebug() << "Looking for file " << path + m_filenames[3];
    QVERIFY(QFile::exists(path + m_filenames[3]));
    QString currentLogFilename = QFileInfo(getCurrentLogFilename()).fileName();
    stopLogging();
    m_filenames << currentLogFilename; //add it to filename to remove in cleanup
}



//TODO - check if there is a way to test renaming without exposing the last used file Q
void
FileWriterRotations::testRenameOldFiles()
{
    //QSKIP("skipping for now");

    QString path = QDir::currentPath() + QDir::separator() + m_currentSubFolder + "/";
    m_filenames << "log.txt"
                << "log-1.txt"
                << "log-2.txt"
                << "log-3.txt";

    //cleanup possible leftover from previous (failed tests)
    cleanupFiles(m_filenames);

    m_Config.maxFileNum = 3;
    m_Config.timeRotationPolicy = UNQL::NoTimeRotation;
    m_Config.compressionAlgo = 0; //TODO - test fail if we use compression because we are not considering it when creating test files

    setOutputFile(path + m_filenames[0]);
    overrideCurrentRotationNumber(1);
    qDebug() << "manually adding " << path + m_filenames[1] << " to last used files";
    m_lastUsedFilenames.append(path + m_filenames[1]);

    stopLogging();// we need to close current logging file cause in this test we call renameOldLogFilesForStrictRotation directly
    renameOldLogFilesForStrictRotation();
    QVERIFY(!QFile::exists(path + m_filenames[0]));
    QVERIFY(QFile::exists(path + m_filenames[1]));
    //end of first test (renaming log into log-1)

    cleanupFiles(m_filenames);

    //test now rotate 2 files
    setOutputFile(path + m_filenames[0]);
    createFile(path + m_filenames[1]);
    qDebug() << "manually inserting " << path + m_filenames[1] << " to beginning of last used files";
    m_lastUsedFilenames.push_front(path + m_filenames[1]);
    qDebug() << "manually inserting " << path + m_filenames[2] << " to beginning last used files";
    m_lastUsedFilenames.push_front(path + m_filenames[2]);
    overrideCurrentRotationNumber(2);
    stopLogging();// we need to close current logging file cause in this test we call renameOldLogFilesForStrictRotation directly
    renameOldLogFilesForStrictRotation();
    QVERIFY(!QFile::exists(path + m_filenames[0]));
    QVERIFY(QFile::exists(path + m_filenames[1]));
    QVERIFY(QFile::exists(path + m_filenames[2]));
    //end of second test (renaming log-1 into log-2, then log into log-1)

    cleanupFiles(m_filenames);

    //this test is similar to the above but tests also the removal of an existing file
    setOutputFile(path + m_filenames[0]);
    createFile(path + m_filenames[1]);
    createFile(path + m_filenames[2]);
    qDebug() << "manually inserting " << path + m_filenames[1] << " to beginning of last used files";
    m_lastUsedFilenames.push_front(path + m_filenames[1]);
    qDebug() << "manually inserting " << path + m_filenames[2] << " to beginning last used files";
    m_lastUsedFilenames.push_front(path + m_filenames[2]);
    overrideCurrentRotationNumber(2);
    stopLogging();// we need to close current logging file cause in this test we call renameOldLogFilesForStrictRotation directly
    renameOldLogFilesForStrictRotation();

    QVERIFY(!QFile::exists(path + m_filenames[0]));
    QVERIFY(QFile::exists(path + m_filenames[1]));
    QVERIFY(QFile::exists(path + m_filenames[2]));
}


/***********************************
 *  TESTS FOR  TIME ROTATION SIMPLE
 *  *********************************/

void FileWriterRotations::testRotateForTimePolicy(int compressionAlg, UNQL::FileRotationTimePolicyType timerotPolicy, QString initialdatetime)
{
    //QSKIP("adjusting code");
    int maxfiles = 4;
    QList<QDateTime> dtlist;

    QDateTime dt = QDateTime::fromString(initialdatetime, DEF_UNQL_TIME_ROTATION_FMT);
    QVERIFY(dt.isValid());

    //config
    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 0;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy = timerotPolicy;
    m_Config.compressionAlgo = compressionAlg;
    m_Config.maxMinutes = 1;

    dt = adjustDateTimeForFileSuffix(dt);

    int rotation_seconds = 60;
    if (timerotPolicy == UNQL::HourlyRotation)
        rotation_seconds = 3600;
    if (timerotPolicy == UNQL::DailyRotation)
        rotation_seconds = 3600 * 24;

    QString path = QDir::currentPath() + QDir::separator() + m_currentSubFolder + "/";
    for (int i=0; i<maxfiles; i++) {
        dtlist << dt.addSecs(i * rotation_seconds); //add one minute, hour or day
        QVERIFY(dtlist[i].isValid());
        m_filenames << "log" + dtlist[i].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
    }

    cleanupFiles(m_filenames);

    qDebug() << dtlist;
    qDebug() << m_filenames;

    overrideLastWrittenDateTime(dtlist[0]);
    setTestingCurrentDateTime(dtlist[0]);
    setOutputFile(path + "log.txt");

    qDebug() << "checking existence of " << path + m_filenames[0];
    QVERIFY(QFileInfo::exists(path + m_filenames[0]));
    qDebug() << "Currlogfilename: " + getCurrentLogFilename()
             << " FileWithFullPath: " + getFileWithFullPath(m_filenames[0], path);
    QCOMPARE(getCurrentLogFilename(), getFileWithFullPath(m_filenames[0], path));


    setTestingCurrentDateTime(dtlist[1]);
    rotateFileForTimePolicy();
    qDebug() << "Examining if compressed file (with algorithm " << compressionAlg << ")"
             << path + getCompressedFilename(m_filenames[0], compressionAlg) << "exists...";
    QVERIFY(QFileInfo::exists(path + getCompressedFilename(m_filenames[0], compressionAlg)));
    qDebug() << "Examining if " << path + m_filenames[1] << "exists...";
    QVERIFY(QFileInfo::exists(path + m_filenames[1]));

    qDebug() << "Verifying if " << getFileWithFullPath(getCompressedFilename(m_filenames[2], compressionAlg), path) << "does not exists...";
    Q_ASSERT(!QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[2], compressionAlg), path)));
    QVERIFY(!QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[3], compressionAlg), path)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
        QVERIFY(!QFileInfo::exists(path + m_filenames[0]));
        qDebug() << "Examining that file " << getFileWithFullPath(getCompressedFilename(m_filenames[1], compressionAlg), path) << "DOES NOT exist";
        Q_ASSERT(!QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[1], compressionAlg), path)));
        QVERIFY(!QFileInfo::exists(path + m_filenames[2]));
        QVERIFY(!QFileInfo::exists(path + m_filenames[3]));
    }
    QCOMPARE(getCurrentLogFilename(), getFileWithFullPath(m_filenames[1], path));

    setTestingCurrentDateTime(dtlist[2]);
    rotateFileForTimePolicy();
    QVERIFY(QFileInfo::exists(path + getCompressedFilename(m_filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(path + getCompressedFilename(m_filenames[1], compressionAlg)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
        QVERIFY(!QFileInfo::exists(path + m_filenames[0]));
        QVERIFY(!QFileInfo::exists(path + m_filenames[1]));
        QVERIFY(!QFileInfo::exists(path + getCompressedFilename(m_filenames[2], compressionAlg)));
        QVERIFY(!QFileInfo::exists(path + m_filenames[3]));
    }
    QVERIFY(QFileInfo::exists(path + m_filenames[2]));
    QCOMPARE(getCurrentLogFilename(), getFileWithFullPath(m_filenames[2], path));


    setTestingCurrentDateTime(dtlist[3]);
    rotateFileForTimePolicy();
    QVERIFY(!QFileInfo::exists(path + getCompressedFilename(m_filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(path + getCompressedFilename(m_filenames[1], compressionAlg)));
    QVERIFY(QFileInfo::exists(path + getCompressedFilename(m_filenames[2], compressionAlg)));
    QVERIFY(QFileInfo::exists(path + m_filenames[3]));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
        QVERIFY(!QFileInfo::exists(path + m_filenames[0]));
        QVERIFY(!QFileInfo::exists(path + m_filenames[1]));
        QVERIFY(!QFileInfo::exists(path + m_filenames[2]));
        QVERIFY(!QFileInfo::exists(path + getCompressedFilename(m_filenames[3], compressionAlg)));
    }
    QCOMPARE(getCurrentLogFilename(), getFileWithFullPath(m_filenames[3], path));

    foreach (QString s, m_filenames) {
        deleteFile(path + getCompressedFilename(s, compressionAlg));
    }
    stopLogging();
}


void FileWriterRotations::testRotateWithPerMinuteRotationGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithPerMinuteRotation(1);
}


void FileWriterRotations::testRotateWithPerMinuteRotationZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithPerMinuteRotation(2);
}


void FileWriterRotations::testRotateWithElapsedMinutesRotationGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithElapsedMinutesRotation(1);
}


void FileWriterRotations::testRotateWithElapsedMinutesRotationZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithElapsedMinutesRotation(2);
}


void FileWriterRotations::testRotateWithHourlyRotationGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithHourlyRotation(1);
}


void FileWriterRotations::testRotateWithHourlyRotationZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithHourlyRotation(2);
}

void FileWriterRotations::testRotateWithDailyRotationGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithDailyRotation(1);
}


void FileWriterRotations::testRotateWithDailyRotationZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithDailyRotation(2);
}


void FileWriterRotations::testRotateWithPerMinuteRotation(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicy(compressionAlg, UNQL::PerMinuteRotation, QDateTime::fromString("2021-03-16T23:00:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}


void FileWriterRotations::testRotateWithHourlyRotation(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicy(compressionAlg, UNQL::HourlyRotation, QDateTime::fromString("2021-03-16T23:15:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}


void FileWriterRotations::testRotateWithDailyRotation(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicy(compressionAlg, UNQL::DailyRotation, QDateTime::fromString("2021-03-16T21:42:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}


void FileWriterRotations::testRotateWithElapsedMinutesRotation(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicy(compressionAlg, UNQL::ElapsedMinutesRotation, QDateTime::fromString("2021-03-16T23:00:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}



/***************************************
 *  TESTS FOR TIME ROTATION INCREMENTAL
 *  *************************************/


void FileWriterRotations::testRotateWithPerMinuteRotationAndSizeHigherNewer(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::PerMinuteRotation, QDateTime::fromString("2021-03-16T23:00:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}



void FileWriterRotations::testRotateWithHourlyRotationAndSizeHigherNewer(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::HourlyRotation, QDateTime::fromString("2021-03-16T23:15:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}


void FileWriterRotations::testRotateWithDailyRotationAndSizeHigherNewer(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::DailyRotation, QDateTime::fromString("2021-03-16T21:42:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}


void FileWriterRotations::testRotateWithElapsedMinutesRotationAndSizeHigherNewer(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::ElapsedMinutesRotation, QDateTime::fromString("2021-03-16T23:00:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}


void FileWriterRotations::testRotateWithElapsedMinutesRotationAndSizeHigherNewerGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithElapsedMinutesRotationAndSizeHigherNewer(1);
}


void FileWriterRotations::testRotateWithElapsedMinutesRotationAndSizeHigherNewerZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithElapsedMinutesRotationAndSizeHigherNewer(2);
}


void FileWriterRotations::testRotateWithHourlyRotationAndSizeHigherNewerGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithHourlyRotationAndSizeHigherNewer(1);
}


void FileWriterRotations::testRotateWithHourlyRotationAndSizeHigherNewerZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithHourlyRotationAndSizeHigherNewer(2);
}

void FileWriterRotations::testRotateWithDailyRotationAndSizeHigherNewerGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithDailyRotationAndSizeHigherNewer(1);
}


void FileWriterRotations::testRotateWithDailyRotationAndSizeHigherNewerZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithDailyRotationAndSizeHigherNewer(2);
}


void FileWriterRotations::testRotateWithPerMinuteRotationAndSizeHigherNewerGzipCompressed()
{
    //QSKIP("");
    testRotateWithPerMinuteRotationAndSizeHigherNewer(1);
}


void FileWriterRotations::testRotateWithPerMinuteRotationAndSizeHigherNewerZipCompressed()
{
    //QSKIP("");
    testRotateWithPerMinuteRotationAndSizeHigherNewer(2);
}

void FileWriterRotations::testRotateForTimePolicyAndSizeHigherNewer(int compressionAlg, UNQL::FileRotationTimePolicyType timerotPolicy, QString initialdatetime)
{
    //QSKIP("adjusting code");

    QList<QDateTime> dtlist;

    QDateTime dt = QDateTime::fromString(initialdatetime, DEF_UNQL_TIME_ROTATION_FMT);
    QVERIFY(dt.isValid());

    //config
    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy = timerotPolicy;
    m_Config.compressionAlgo = compressionAlg;
    m_Config.maxMinutes = 1;

    dt = adjustDateTimeForFileSuffix(dt);

    int rotation_seconds = 60;
    if (timerotPolicy == UNQL::HourlyRotation)
        rotation_seconds = 3600;
    if (timerotPolicy == UNQL::DailyRotation)
        rotation_seconds = 3600 * 24;


    // we will use 4 files that will be used for incremental size (first 3) and then we trigger the time rotation
    // so for minute-based rotation they would be like:
    //m_filenames: "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
    //          << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt";

    //first add the first datetime
    dtlist << dt;
    //then add the second datetime (with time increment)
    dtlist << dt.addSecs(1 * rotation_seconds); //add one minute, hour or day
    QVERIFY(dtlist[1].isValid());

    //Add the filenames
    //first the basic one
    m_filenames << "log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
    //then the two rotated for size
    m_filenames << "log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-1.txt";
    m_filenames << "log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-2.txt";
    //finally the one rotated for time
    m_filenames << "log" + dtlist[1].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";


    cleanupFiles(m_filenames);

    qDebug() << dtlist;
    qDebug() << m_filenames;


    overrideLastWrittenDateTime(dt);
    setTestingCurrentDateTime(dt);



    qDebug() << "setting output file to " << getFileWithFullPath("log.txt", m_currentSubFolder);
    setOutputFile(getFileWithFullPath("log.txt", m_currentSubFolder));
    qDebug() << "checking existenxe of " << getFileWithFullPath(m_filenames[0], m_currentSubFolder);
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[0], m_currentSubFolder)));

    writeToFile(getFileWithFullPath(m_filenames[0], m_currentSubFolder), 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    qDebug() << "verify that exists " << getCompressedFilename(m_filenames[0], compressionAlg);
    QVERIFY(QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[0], compressionAlg), m_currentSubFolder)));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[1], m_currentSubFolder)));
    QVERIFY(!QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[2], compressionAlg), m_currentSubFolder)));
    QVERIFY(!QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[3], compressionAlg), m_currentSubFolder)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
            QVERIFY(!QFileInfo::exists(m_currentSubFolder+m_filenames[0]));
            QVERIFY(!QFileInfo::exists(m_currentSubFolder+m_filenames[2]));
            QVERIFY(!QFileInfo::exists(m_currentSubFolder+getCompressedFilename(m_filenames[1], compressionAlg)));
            QVERIFY(!QFileInfo::exists(m_currentSubFolder+m_filenames[3]));
    }
    QVERIFY(getCurrentLogFilename() == getFileWithFullPath(m_filenames[1], m_currentSubFolder));


    writeToFile(getFileWithFullPath(m_filenames[1], m_currentSubFolder), 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    QVERIFY(QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[0], compressionAlg), m_currentSubFolder)));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[1], compressionAlg), m_currentSubFolder)));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[2], m_currentSubFolder)));
    QVERIFY(!QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[3], compressionAlg), m_currentSubFolder)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
            QVERIFY(!QFileInfo::exists(getFileWithFullPath(m_filenames[0], m_currentSubFolder)));
            QVERIFY(!QFileInfo::exists(getFileWithFullPath(m_filenames[1], m_currentSubFolder)));
            QVERIFY(!QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[2], compressionAlg), m_currentSubFolder)));
            QVERIFY(!QFileInfo::exists(getFileWithFullPath(m_filenames[3], m_currentSubFolder)));
    }
    QVERIFY(getCurrentLogFilename() == getFileWithFullPath(m_filenames[2], m_currentSubFolder));


    setTestingCurrentDateTime(dtlist[1]);
    rotateFilesIfNeeded(); //or rotateFileForTimePolicy();
    QVERIFY(!QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[0], compressionAlg), m_currentSubFolder)));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[1], compressionAlg), m_currentSubFolder)));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[2], compressionAlg), m_currentSubFolder)));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[3], m_currentSubFolder)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
            QVERIFY(!QFileInfo::exists(getFileWithFullPath(m_filenames[0], m_currentSubFolder)));
            QVERIFY(!QFileInfo::exists(getFileWithFullPath(m_filenames[1], m_currentSubFolder)));
            QVERIFY(!QFileInfo::exists(getFileWithFullPath(getCompressedFilename(m_filenames[3], compressionAlg), m_currentSubFolder)));
            QVERIFY(!QFileInfo::exists(getFileWithFullPath(m_filenames[2], m_currentSubFolder)));
    }
    QVERIFY(getCurrentLogFilename() == getFileWithFullPath(m_filenames[3], m_currentSubFolder));

    //on windows we need to explicit stop logging to avoid failing to delete the last open file
    stopLogging();

    foreach (QString s, m_filenames) {
        deleteFile(getFileWithFullPath(getCompressedFilename(s, compressionAlg), m_currentSubFolder));

    }
}



/*********************************
 *  TEST TIME ROTATION STRICT
 *  ******************************/



void FileWriterRotations::testRotateForTimePolicyAndSizeStrict(int compressionAlg, UNQL::FileRotationTimePolicyType timerotPolicy, QString initialdatetime)
{
    //QSKIP("adjusting code");

    QList<QDateTime> dtlist;

    QDateTime dt = QDateTime::fromString(initialdatetime, DEF_UNQL_TIME_ROTATION_FMT);
    QVERIFY(dt.isValid());

    //config
    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::StrictRotation;
    m_Config.timeRotationPolicy = timerotPolicy;
    m_Config.compressionAlgo = compressionAlg;
    m_Config.maxMinutes = 1;

    dt = adjustDateTimeForFileSuffix(dt);

    int rotation_seconds = 60;
    if (timerotPolicy == UNQL::HourlyRotation)
        rotation_seconds = 3600;
    if (timerotPolicy == UNQL::DailyRotation)
        rotation_seconds = 3600 * 24;


    // we will use 5 files that will be used for incremental size (first 3) and then we trigger the time rotation
    // and again the size rotation so for minute-based rotation they would be like:
    /*    m_filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
                  << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt"
                  << "log-2021-03-16T02:01:00-1.txt";*/

    //first add the first datetime
    dtlist << dt;
    //then add the second datetime (with time increment)
    dtlist << dt.addSecs(1 * rotation_seconds); //add one minute, hour or day
    QVERIFY(dtlist[1].isValid());

    //Add the filenames
    //first the basic one
    m_filenames << m_currentSubFolder + "/log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
    //then the two rotated for size
    m_filenames << m_currentSubFolder + "/log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-1.txt";
    m_filenames << m_currentSubFolder + "/log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-2.txt";
    //finally the one rotated for time
    m_filenames << m_currentSubFolder + "/log" + dtlist[1].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
    m_filenames << m_currentSubFolder + "/log" + dtlist[1].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-1.txt";


    cleanupFiles(m_filenames);

    qDebug() << dtlist;
    qDebug() << m_filenames;

    overrideLastWrittenDateTime(dt);
    setOutputFile(m_currentSubFolder + "/log.txt");

    QVERIFY(QFileInfo::exists(m_filenames[0]));

    writeToFile(m_filenames[0], 2);
    rotateFileForStrictRotation();
    QVERIFY(QFileInfo::exists(m_filenames[0]));
    QVERIFY(QFileInfo::exists(getCompressedFilename(m_filenames[1], compressionAlg)));
    QVERIFY(!QFileInfo::exists(m_filenames[2]));
    QVERIFY(!QFileInfo::exists(m_filenames[3]));
    QVERIFY(!QFileInfo::exists(m_filenames[4]));
    QVERIFY(getCurrentLogFilename() == m_filenames[0]);


    writeToFile(m_filenames[0], 2);
    rotateFileForStrictRotation();
    QVERIFY(QFileInfo::exists(m_filenames[0]));
    QVERIFY(QFileInfo::exists(getCompressedFilename(m_filenames[1], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(m_filenames[2], compressionAlg)));
    QVERIFY(!QFileInfo::exists(m_filenames[3]));
    QVERIFY(!QFileInfo::exists(m_filenames[4]));
    QVERIFY(getCurrentLogFilename() == m_filenames[0]);


    setTestingCurrentDateTime(dtlist[1]);
    rotateFileForTimePolicy();
    QVERIFY(QFileInfo::exists(getCompressedFilename(m_filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(m_filenames[1], compressionAlg)));
    QVERIFY(!QFileInfo::exists(m_filenames[2]));
    QVERIFY(QFileInfo::exists(m_filenames[3]));
    QVERIFY(!QFileInfo::exists(m_filenames[4]));
    QVERIFY(getCurrentLogFilename() == m_filenames[3]);


    writeToFile(m_filenames[3], 2);
    rotateFileForStrictRotation();
    QVERIFY(QFileInfo::exists(getCompressedFilename(m_filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(m_filenames[3]));
    QVERIFY(QFileInfo::exists(getCompressedFilename(m_filenames[4], compressionAlg)));
    QVERIFY(!QFileInfo::exists(m_filenames[1]));
    QVERIFY(!QFileInfo::exists(m_filenames[2]));
    QVERIFY(getCurrentLogFilename() == m_filenames[3]);

    cleanupFiles(m_filenames);
    foreach(QString s, m_filenames) {
        deleteFile(getCompressedFilename(s, compressionAlg));
    }
}

void FileWriterRotations::testRotateWithPerMinuteRotationAndSizeStrict(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::PerMinuteRotation, QDateTime::fromString("2021-03-16T23:00:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}

void FileWriterRotations::testRotateWithPerMinuteRotationAndSizeStrictGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithPerMinuteRotationAndSizeStrict(1);
}


void FileWriterRotations::testRotateWithPerMinuteRotationAndSizeStrictZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithPerMinuteRotationAndSizeStrict(2);
}


void FileWriterRotations::testRotateWithElapsedMinutesRotationAndSizeStrict(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::ElapsedMinutesRotation, QDateTime::fromString("2021-03-16T23:00:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}

void FileWriterRotations::testRotateWithElapsedMinutesRotationAndSizeStrictGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithElapsedMinutesRotationAndSizeStrict(1);
}


void FileWriterRotations::testRotateWithElapsedMinutesRotationAndSizeStrictZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithElapsedMinutesRotationAndSizeStrict(2);
}


void FileWriterRotations::testRotateWithHourlyRotationAndSizeStrict(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::HourlyRotation, QDateTime::fromString("2021-03-16T23:00:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}

void FileWriterRotations::testRotateWithHourlyRotationAndSizeStrictGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithHourlyRotationAndSizeStrict(1);
}


void FileWriterRotations::testRotateWithHourlyRotationAndSizeStrictZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithHourlyRotationAndSizeStrict(2);
}


void FileWriterRotations::testRotateWithDailyRotationAndSizeStrict(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::DailyRotation, QDateTime::fromString("2021-03-16T23:00:12", Qt::ISODate).toString(DEF_UNQL_TIME_ROTATION_FMT));
}

void FileWriterRotations::testRotateWithDailyRotationAndSizeStrictGzipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithDailyRotationAndSizeStrict(1);
}


void FileWriterRotations::testRotateWithDailyRotationAndSizeStrictZipCompressed()
{
    //QSKIP("Adjusting code");
    testRotateWithDailyRotationAndSizeStrict(2);
}

/************************************
 * TESTS FOR SIZE BASED ROTATION
 * *********************************/

void FileWriterRotations::testRotateForIncrementalNumbers(int compressionAlgorithm)
{
    //QSKIP("adjusting code");

    //init
    QString path = QDir::currentPath() + QDir::separator() + m_currentSubFolder + "/";
    m_filenames << "log.txt"
                << "log-1.txt"
                << "log-2.txt"
                << "log-3.txt";

    //cleanup possible leftover from previous (failed tests)
    cleanupFiles(m_filenames);

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.compressionAlgo = compressionAlgorithm;

    QString compressedExt = getCompressedExten(compressionAlgorithm);

    //create base file
    setOutputFile(path + m_filenames[0]);

    //write to log.txt more than allowed size and rotate
    writeToFile(path + m_filenames[0], 2);
    rotateFileForIncrementalNumbers();

    //verify we created the new file
    QVERIFY(QFileInfo::exists(path + m_filenames[0] + compressedExt));
    QVERIFY(QFileInfo::exists(path + m_filenames[1]));
    QVERIFY(!QFileInfo::exists(path + m_filenames[2]));
    QVERIFY(!QFileInfo::exists(path + m_filenames[3]));
    QVERIFY(getCurrentLogFilename() == getFileWithFullPath(m_filenames[1], path));


    //write to log-1 more than allowed size and rotate
    writeToFile(path + m_filenames[1], 2);
    rotateFileForIncrementalNumbers();
    QVERIFY(QFileInfo::exists(path + m_filenames[0] + compressedExt));
    QVERIFY(QFileInfo::exists(path + m_filenames[1] + compressedExt));
    QVERIFY(QFileInfo::exists(path + m_filenames[2]));
    QVERIFY(!QFileInfo::exists(path + m_filenames[3]));
    QVERIFY(getCurrentLogFilename() == getFileWithFullPath(m_filenames[2], path));


    //write to log-2 more than allowed size and rotate
    writeToFile(path + m_filenames[2], 2);
    rotateFileForIncrementalNumbers();

    QVERIFY(!QFileInfo::exists(path + m_filenames[0] + compressedExt));
    QVERIFY(QFileInfo::exists(path + m_filenames[1] + compressedExt));
    QVERIFY(QFileInfo::exists(path + m_filenames[2] + compressedExt));
    QVERIFY(QFileInfo::exists(path + m_filenames[3]));
    QVERIFY(getCurrentLogFilename() == getFileWithFullPath(m_filenames[3], path));

    if (!compressedExt.isEmpty()) {
        QStringList compressedFilenames;
        foreach (QString s, m_filenames) {
            compressedFilenames << (s + compressedExt);
        }
        cleanupFiles(compressedFilenames);
    }

    stopLogging();
}

void FileWriterRotations::testRotateForIncrementalNumbersGzipCompressed()
{
    testRotateForIncrementalNumbers(1);
}


void FileWriterRotations::testRotateForIncrementalNumbersZipCompressed()
{
    //QSKIP("");
    testRotateForIncrementalNumbers(2);
}

void FileWriterRotations::testRotateForStrictNumbers(int compressionAlgorithm)
{
    //QSKIP("adjusting code");

    m_filenames << "log.txt"
                << "log-1.txt"
                << "log-2.txt"
                << "log-3.txt";

    cleanupFiles(m_filenames);

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::StrictRotation;
    m_Config.compressionAlgo = compressionAlgorithm;

    QString compressedExt = getCompressedExten(compressionAlgorithm);
    //in this test we use a subfolder

    //create base file
    setOutputFile(getFileWithFullPath(m_filenames[0], m_currentSubFolder));

    //write to log.txt more than allowed size and rotate
    writeToFile(getFileWithFullPath(m_filenames[0], m_currentSubFolder), 2);
    rotateFileForStrictRotation();

    //verify we created the new file
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[0], m_currentSubFolder)));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[1], m_currentSubFolder) + compressedExt));
    QVERIFY(getCurrentLogFilename() == getFileWithFullPath(m_filenames[0], m_currentSubFolder));


    //write to log more than allowed size and rotate
    writeToFile(getFileWithFullPath(m_filenames[0], m_currentSubFolder), 2);
    rotateFileForStrictRotation();

    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[0], m_currentSubFolder)));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[1], m_currentSubFolder) + compressedExt));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[2], m_currentSubFolder) + compressedExt));
    QVERIFY(!QFileInfo::exists(getFileWithFullPath(m_filenames[3], m_currentSubFolder) + compressedExt));
    QVERIFY(getCurrentLogFilename() == getFileWithFullPath(m_filenames[0], m_currentSubFolder));


    //write to log more than allowed size and rotate
    writeToFile(getFileWithFullPath(m_filenames[0], m_currentSubFolder), 2);
    rotateFileForStrictRotation();

    QVERIFY(!QFileInfo::exists(getFileWithFullPath(m_filenames[3], m_currentSubFolder) + compressedExt));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[2], m_currentSubFolder) + compressedExt));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[1], m_currentSubFolder) + compressedExt));
    QVERIFY(QFileInfo::exists(getFileWithFullPath(m_filenames[0], m_currentSubFolder)));
    QVERIFY(getCurrentLogFilename() == getFileWithFullPath(m_filenames[0], m_currentSubFolder));

    if (!compressedExt.isEmpty()){
        QStringList filenamesCompressed;
        foreach (QString s, m_filenames) {
            filenamesCompressed << (s + compressedExt);
        }
        cleanupFiles(filenamesCompressed);
    }

    stopLogging();
}



void FileWriterRotations::testRotateForStrictNumbersGzipCompressed()
{
    testRotateForStrictNumbers(1);
}


void FileWriterRotations::testRotateForStrictNumbersZipCompressed()
{
    //QSKIP("");
    testRotateForStrictNumbers(2);
}




QTEST_GUILESS_MAIN(FileWriterRotations)

// the pattern "moc_*" is for when Q_OBJECT macro in a .h file (and included by Makefile)
// if we have just the .cpp we would have a .moc file to include (FL)
//#include "tst_FileWriterRotations.moc"
