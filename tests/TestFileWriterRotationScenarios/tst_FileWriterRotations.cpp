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
        ////qWarning() << "Could not remove file " << fname << " - " << f.errorString();
    }
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
    resetLastUsedFilenames();
    setTestingCurrentDateTime(QDateTime());
    m_Config = WriterConfig();
}

void FileWriterRotations::cleanup(QStringList filenames) {
    foreach (QString s, filenames) {
        deleteFile(s);
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

    QStringList filenames;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    cleanup(filenames);

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy= UNQL::NoTimeRotation;

    foreach (QString s, filenames) {
        createFile(s);
    }

    for (int i=0; i<filenames.size(); i++)
        m_lastUsedFilenames.enqueue(filenames[i]);

    qDebug() << m_lastUsedFilenames;
    removeOldestFiles();
    qDebug() << m_lastUsedFilenames;

    QVERIFY(!QFile::exists(filenames[0]));
    QVERIFY(QFile::exists(filenames[1]));
    QVERIFY(QFile::exists(filenames[2]));
    QVERIFY(QFile::exists(filenames[3]));


    cleanup(filenames);
}


void
FileWriterRotations::testRemoveLeftovers()
{
    int maxfiles = 4;
    QList<QDateTime> dtlist;

    QString initialdatetime = "2021-03-19T01:57:32";
    UNQL::FileRotationTimePolicyType timerotPolicy = UNQL::HourlyRotation;

    QDateTime dt = QDateTime::fromString(initialdatetime, DEF_UNQL_TIME_ROTATION_FMT);
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

    QDir::current().mkdir("testsubfolder");
    QString path = QDir::currentPath() + "/testsubfolder";

    QStringList filenames;
    //define the filenames each more in the future than previous
    for (int i=0; i<maxfiles; i++) {
        dtlist << dt.addSecs(i * rotation_seconds); //add one minute, hour or day
        QVERIFY(dtlist[i].isValid());
        filenames << path + "/log" + dtlist[i].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
        createFile(filenames[i]);
    }

    //go ahead in the future 3 hours
    setTestingCurrentDateTime(dt.addSecs(3*rotation_seconds));
    //we configured that we want just 3 files so when we set the new output file we should get rid of
    //the first two entries of filenames
    setOutputFile("testsubfolder/log.txt");
    removeLeftoversFromPreviousRun();
    qDebug() << "Looking for file (should not exist) " << filenames[0];
    QVERIFY(!QFile::exists(filenames[0]));
    qDebug() << "Looking for file (should not exist) " << filenames[1];
    QVERIFY(!QFile::exists(filenames[1]));
    qDebug() << "Looking for file " << filenames[2];
    QVERIFY(QFile::exists(filenames[2]));
    qDebug() << "Looking for file " << filenames[3];
    QVERIFY(QFile::exists(filenames[3]));
    cleanup(filenames);
    QDir::current().rmdir("testsubfolder");
}



//TODO - check if there is a way to test renaming without exposing the last used file Q
void
FileWriterRotations::testRenameOldFiles()
{
    //QSKIP("skipping for now");

    QStringList filenames;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    //cleanup possible leftover from previous (failed tests)
    cleanup(filenames);

    m_Config.maxFileNum = 3;
    m_Config.timeRotationPolicy = UNQL::NoTimeRotation;
    m_Config.compressionAlgo = 0; //TODO - test fail if we use compression because we are not considering it when creating test files

    setOutputFile(filenames[0]);
    overrideCurrentRotationNumber(1);
    qDebug() << "manually adding " << filenames[1] << " to last used files";
    m_lastUsedFilenames.append(filenames[1]);

    renameOldLogFilesForStrictRotation();
    QVERIFY(!QFile::exists(filenames[0]));
    QVERIFY(QFile::exists(filenames[1]));
    //end of first test (renaming log into log-1)

    cleanup(filenames);

    //test now rotate 2 files
    setOutputFile(filenames[0]);
    createFile(filenames[1]);
    qDebug() << "manually inserting " << filenames[1] << " to beginning of last used files";
    m_lastUsedFilenames.push_front(filenames[1]);
    qDebug() << "manually inserting " << filenames[2] << " to beginning last used files";
    m_lastUsedFilenames.push_front(filenames[2]);
    overrideCurrentRotationNumber(2);
    renameOldLogFilesForStrictRotation();
    QVERIFY(!QFile::exists(filenames[0]));
    QVERIFY(QFile::exists(filenames[1]));
    QVERIFY(QFile::exists(filenames[2]));
    //end of second test (renaming log-1 into log-2, then log into log-1)

    cleanup(filenames);

    //this test is similar to the above but tests also the removal of an existing file
    setOutputFile(filenames[0]);
    createFile(filenames[1]);
    createFile(filenames[2]);
    qDebug() << "manually inserting " << filenames[1] << " to beginning of last used files";
    m_lastUsedFilenames.push_front(filenames[1]);
    qDebug() << "manually inserting " << filenames[2] << " to beginning last used files";
    m_lastUsedFilenames.push_front(filenames[2]);
    overrideCurrentRotationNumber(2);
    renameOldLogFilesForStrictRotation();

    QVERIFY(!QFile::exists(filenames[0]));
    QVERIFY(QFile::exists(filenames[1]));
    QVERIFY(QFile::exists(filenames[2]));

    cleanup(filenames);
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

    QStringList filenames;
    for (int i=0; i<maxfiles; i++) {
        dtlist << dt.addSecs(i * rotation_seconds); //add one minute, hour or day
        QVERIFY(dtlist[i].isValid());
        filenames << "log" + dtlist[i].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
    }

    cleanup(filenames);

    qDebug() << dtlist;
    qDebug() << filenames;

    overrideLastWrittenDateTime(dtlist[0]);
    setTestingCurrentDateTime(dtlist[0]);
    setOutputFile("log.txt");

    QVERIFY(QFileInfo::exists(filenames[0]));
    QCOMPARE(getCurrentLogFilename(), filenames[0]);


    setTestingCurrentDateTime(dtlist[1]);
    rotateFileForTimePolicy();
    qDebug() << "Examining if " << QDir::currentPath() + "/" + getCompressedFilename(filenames[0], compressionAlg) << "exists...";
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    qDebug() << "Examining if " << filenames[1] << "exists...";
    QVERIFY(QFileInfo::exists(filenames[1]));
    QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
        QVERIFY(!QFileInfo::exists(filenames[0]));
        QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
        QVERIFY(!QFileInfo::exists(filenames[2]));
        QVERIFY(!QFileInfo::exists(filenames[3]));
    }
    QCOMPARE(getCurrentLogFilename(), filenames[1]);

    setTestingCurrentDateTime(dtlist[2]);
    rotateFileForTimePolicy();
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
        QVERIFY(!QFileInfo::exists(filenames[0]));
        QVERIFY(!QFileInfo::exists(filenames[1]));
        QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
        QVERIFY(!QFileInfo::exists(filenames[3]));
    }
    QVERIFY(QFileInfo::exists(filenames[2]));
    QCOMPARE(getCurrentLogFilename(), filenames[2]);


    setTestingCurrentDateTime(dtlist[3]);
    rotateFileForTimePolicy();
    QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    QVERIFY(QFileInfo::exists(filenames[3]));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
        QVERIFY(!QFileInfo::exists(filenames[0]));
        QVERIFY(!QFileInfo::exists(filenames[1]));
        QVERIFY(!QFileInfo::exists(filenames[2]));
        QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
    }
    QCOMPARE(getCurrentLogFilename(), filenames[3]);

    foreach (QString s, filenames) {
        deleteFile(getCompressedFilename(s, compressionAlg));
    }
    cleanup(filenames);
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
    testRotateForTimePolicy(compressionAlg, UNQL::PerMinuteRotation, "2021-03-16T23:00:12");
}


void FileWriterRotations::testRotateWithHourlyRotation(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicy(compressionAlg, UNQL::HourlyRotation, "2021-03-16T23:15:12");
}


void FileWriterRotations::testRotateWithDailyRotation(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicy(compressionAlg, UNQL::DailyRotation, "2021-03-16T21:42:12");
}


void FileWriterRotations::testRotateWithElapsedMinutesRotation(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicy(compressionAlg, UNQL::ElapsedMinutesRotation, "2021-03-16T23:00:12");
}



/***************************************
 *  TESTS FOR TIME ROTATION INCREMENTAL
 *  *************************************/


void FileWriterRotations::testRotateWithPerMinuteRotationAndSizeHigherNewer(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::PerMinuteRotation, "2021-03-16T23:00:12");
}



void FileWriterRotations::testRotateWithHourlyRotationAndSizeHigherNewer(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::HourlyRotation, "2021-03-16T23:15:12");
}


void FileWriterRotations::testRotateWithDailyRotationAndSizeHigherNewer(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::DailyRotation, "2021-03-16T21:42:12");
}


void FileWriterRotations::testRotateWithElapsedMinutesRotationAndSizeHigherNewer(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::ElapsedMinutesRotation, "2021-03-16T23:00:12");
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


    QStringList filenames;
    // we will use 4 files that will be used for incremental size (first 3) and then we trigger the time rotation
    // so for minute-based rotation they would be like:
    //filenames: "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
    //          << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt";

    //first add the first datetime
    dtlist << dt;
    //then add the second datetime (with time increment)
    dtlist << dt.addSecs(1 * rotation_seconds); //add one minute, hour or day
    QVERIFY(dtlist[1].isValid());

    //Add the filenames
    //first the basic one
    filenames << "log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
    //then the two rotated for size
    filenames << "log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-1.txt";
    filenames << "log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-2.txt";
    //finally the one rotated for time
    filenames << "log" + dtlist[1].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";


    cleanup(filenames);

    qDebug() << dtlist;
    qDebug() << filenames;


    overrideLastWrittenDateTime(dt);
    setTestingCurrentDateTime(dt);
    setOutputFile("log.txt");

    QVERIFY(QFileInfo::exists(filenames[0]));

    writeToFile(filenames[0], 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    qDebug() << "verify that exists " << getCompressedFilename(filenames[0], compressionAlg);
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(filenames[1]));
    QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
            QVERIFY(!QFileInfo::exists(filenames[0]));
            QVERIFY(!QFileInfo::exists(filenames[2]));
            QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
            QVERIFY(!QFileInfo::exists(filenames[3]));
    }
    QVERIFY(getCurrentLogFilename() == filenames[1]);


    writeToFile(filenames[1], 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    QVERIFY(QFileInfo::exists(filenames[2]));
    QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
            QVERIFY(!QFileInfo::exists(filenames[0]));
            QVERIFY(!QFileInfo::exists(filenames[1]));
            QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
            QVERIFY(!QFileInfo::exists(filenames[3]));
    }
    QVERIFY(getCurrentLogFilename() == filenames[2]);


    setTestingCurrentDateTime(dtlist[1]);
    rotateFilesIfNeeded(); //or rotateFileForTimePolicy();
    QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    QVERIFY(QFileInfo::exists(filenames[3]));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
            QVERIFY(!QFileInfo::exists(filenames[0]));
            QVERIFY(!QFileInfo::exists(filenames[1]));
            QVERIFY(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
            QVERIFY(!QFileInfo::exists(filenames[2]));
    }
    QVERIFY(getCurrentLogFilename() == filenames[3]);


    foreach (QString s, filenames) {
        deleteFile(getCompressedFilename(s, compressionAlg));
    }
    cleanup(filenames);
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


    QStringList filenames;
    // we will use 5 files that will be used for incremental size (first 3) and then we trigger the time rotation
    // and again the size rotation so for minute-based rotation they would be like:
    /*    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
                  << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt"
                  << "log-2021-03-16T02:01:00-1.txt";*/

    //first add the first datetime
    dtlist << dt;
    //then add the second datetime (with time increment)
    dtlist << dt.addSecs(1 * rotation_seconds); //add one minute, hour or day
    QVERIFY(dtlist[1].isValid());

    //Add the filenames
    //first the basic one
    filenames << "log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
    //then the two rotated for size
    filenames << "log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-1.txt";
    filenames << "log" + dtlist[0].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-2.txt";
    //finally the one rotated for time
    filenames << "log" + dtlist[1].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + ".txt";
    filenames << "log" + dtlist[1].toString(DEF_UNQL_TIME_ROTATION_SUFFIX) + "-1.txt";


    cleanup(filenames);

    qDebug() << dtlist;
    qDebug() << filenames;

    overrideLastWrittenDateTime(dt);
    setOutputFile("log.txt");

    QVERIFY(QFileInfo::exists(filenames[0]));

    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();
    QVERIFY(QFileInfo::exists(filenames[0]));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    QVERIFY(!QFileInfo::exists(filenames[2]));
    QVERIFY(!QFileInfo::exists(filenames[3]));
    QVERIFY(!QFileInfo::exists(filenames[4]));
    QVERIFY(getCurrentLogFilename() == filenames[0]);


    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();
    QVERIFY(QFileInfo::exists(filenames[0]));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    QVERIFY(!QFileInfo::exists(filenames[3]));
    QVERIFY(!QFileInfo::exists(filenames[4]));
    QVERIFY(getCurrentLogFilename() == filenames[0]);


    setTestingCurrentDateTime(dtlist[1]);
    rotateFileForTimePolicy();
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    QVERIFY(!QFileInfo::exists(filenames[2]));
    QVERIFY(QFileInfo::exists(filenames[3]));
    QVERIFY(!QFileInfo::exists(filenames[4]));
    QVERIFY(getCurrentLogFilename() == filenames[3]);


    writeToFile(filenames[3], 2);
    rotateFileForStrictRotation();
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    QVERIFY(QFileInfo::exists(filenames[3]));
    QVERIFY(QFileInfo::exists(getCompressedFilename(filenames[4], compressionAlg)));
    QVERIFY(!QFileInfo::exists(filenames[1]));
    QVERIFY(!QFileInfo::exists(filenames[2]));
    QVERIFY(getCurrentLogFilename() == filenames[3]);

    cleanup(filenames);
    foreach(QString s, filenames) {
        deleteFile(getCompressedFilename(s, compressionAlg));
    }
}

void FileWriterRotations::testRotateWithPerMinuteRotationAndSizeStrict(int compressionAlg)
{
    //QSKIP("Adjusting code");
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::PerMinuteRotation, "2021-03-16T23:00:12");
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
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::ElapsedMinutesRotation, "2021-03-16T23:00:12");
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
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::HourlyRotation, "2021-03-16T23:00:12");
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
    testRotateForTimePolicyAndSizeHigherNewer(compressionAlg, UNQL::DailyRotation, "2021-03-16T23:00:12");
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
    QStringList filenames;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    //cleanup possible leftover from previous (failed tests)
    cleanup(filenames);

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.compressionAlgo = compressionAlgorithm;

    QString compressedExt = getCompressedExten(compressionAlgorithm);

    //create base file
    setOutputFile(filenames[0]);

    //write to log.txt more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForIncrementalNumbers();

    //verify we created the new file
    QVERIFY(QFileInfo::exists(filenames[0] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[1]));
    QVERIFY(!QFileInfo::exists(filenames[2]));
    QVERIFY(!QFileInfo::exists(filenames[3]));
    QVERIFY(getCurrentLogFilename() == filenames[1]);


    //write to log-1 more than allowed size and rotate
    writeToFile(filenames[1], 2);
    rotateFileForIncrementalNumbers();
    QVERIFY(QFileInfo::exists(filenames[0] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[1] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[2]));
    QVERIFY(!QFileInfo::exists(filenames[3]));
    QVERIFY(getCurrentLogFilename() == filenames[2]);


    //write to log-2 more than allowed size and rotate
    writeToFile(filenames[2], 2);
    rotateFileForIncrementalNumbers();

    QVERIFY(!QFileInfo::exists(filenames[0] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[1] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[2] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[3]));
    QVERIFY(getCurrentLogFilename() == filenames[3]);

    foreach (QString s, filenames) {
        deleteFile(s + compressedExt);
    }
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

    QStringList filenames;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";
    cleanup(filenames);

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::StrictRotation;
    m_Config.compressionAlgo = compressionAlgorithm;

    QString compressedExt = getCompressedExten(compressionAlgorithm);

    //create base file
    setOutputFile(filenames[0]);

    //write to log.txt more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();

    //verify we created the new file
    QVERIFY(QFileInfo::exists(filenames[0]));
    QVERIFY(QFileInfo::exists(filenames[1] + compressedExt));
    QVERIFY(getCurrentLogFilename() == filenames[0]);


    //write to log more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();

    QVERIFY(QFileInfo::exists(filenames[0]));
    QVERIFY(QFileInfo::exists(filenames[1] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[2] + compressedExt));
    QVERIFY(!QFileInfo::exists(filenames[3] + compressedExt));
    QVERIFY(getCurrentLogFilename() == filenames[0]);


    //write to log more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();

    QVERIFY(!QFileInfo::exists(filenames[3] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[2] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[1] + compressedExt));
    QVERIFY(QFileInfo::exists(filenames[0]));
    QVERIFY(getCurrentLogFilename() == filenames[0]);

    foreach (QString s, filenames) {
        deleteFile(s + compressedExt);
    }
    cleanup(filenames);
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
