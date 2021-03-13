#include "testFileWriter.h"

#include <FileWriter.h>

#include <QtTest/QtTest>

testFileWriter::testFileWriter(WriterConfig wc)
    : FileWriter(wc)
{

}

#include <iostream>
void
testFileWriter::testCalculateLogInfo()
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
    QVERIFY(lfi.path == "");

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
    f.remove();
}


void testFileWriter::cleanup(QStringList filenames) {
    resetLastUsedFilenames();
    foreach (QString s, filenames) {
        deleteFile(s);
    }
    setTestingCurrentDateTime(QDateTime());
}

void
testFileWriter::testRemoveOldestFiles()
{
    //QSKIP("Adjusting code...");

    QStringList filenames;
    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy= UNQL::NoTimeRotation;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    cleanup(filenames);
    foreach (QString s, filenames) {
        createFile(s);
    }

    for (int i=0; i<filenames.size(); i++)
        m_lastUsedFilenames.enqueue(filenames[i]);

    qDebug() << m_lastUsedFilenames;
    removeOldestFiles();
    qDebug() << m_lastUsedFilenames;

    Q_ASSERT(!QFile::exists(filenames[0]));
    Q_ASSERT(QFile::exists(filenames[1]));
    Q_ASSERT(QFile::exists(filenames[2]));
    Q_ASSERT(QFile::exists(filenames[3]));


    cleanup(filenames);
}


//TODO - check if there is a way to test renaming without exposing the last used file Q
void
testFileWriter::testRenameOldFiles()
{
    //QSKIP("skipping for now");

    QStringList filenames;
    m_Config.maxFileNum = 3;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    //cleanup possible leftover from previous (failed tests)
    cleanup(filenames);

    setOutputFile(filenames[0]);
    overrideCurrentRotationNumber(1);
    qDebug() << "manually adding " << filenames[1] << " to last used files";
    m_lastUsedFilenames.append(filenames[1]);

    renameOldLogFilesForStrictRotation();
    Q_ASSERT(!QFile::exists(filenames[0]));
    Q_ASSERT(QFile::exists(filenames[1]));
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
    Q_ASSERT(!QFile::exists(filenames[0]));
    Q_ASSERT(QFile::exists(filenames[1]));
    Q_ASSERT(QFile::exists(filenames[2]));
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

    Q_ASSERT(!QFile::exists(filenames[0]));
    Q_ASSERT(QFile::exists(filenames[1]));
    Q_ASSERT(QFile::exists(filenames[2]));

    cleanup(filenames);
}


void testFileWriter::testRotateForTimePolicy()
{
    //QSKIP("adjusting code");

    QStringList filenames;

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 0;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy= UNQL::PerMinuteRotation;

    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:01:00.txt"
              << "log-2021-03-16T02:02:00.txt" << "log-2021-03-16T02:03:00.txt";

    cleanup(filenames);

    QDateTime dt = QDateTime::fromString("2021-03-16T02:00:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt.isValid());
    QDateTime dt1 = QDateTime::fromString("2021-03-16T02:01:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt1.isValid());
    QDateTime dt2 = QDateTime::fromString("2021-03-16T02:02:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt2.isValid());
    QDateTime dt3 = QDateTime::fromString("2021-03-16T02:03:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt3.isValid());

    overrideLastWrittenDateTime(dt);
    setOutputFile("log.txt");

    Q_ASSERT(QFileInfo::exists(filenames[0]));


    setTestingCurrentDateTime(dt1);
    rotateFileForTimePolicy();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[1]);

    setTestingCurrentDateTime(dt2);
    rotateFileForTimePolicy();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[2]);


    setTestingCurrentDateTime(dt3);
    rotateFileForTimePolicy();
    Q_ASSERT(!QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);

    foreach (QString s, filenames) {
        deleteFile(s);
    }
}


void testFileWriter::testRotateForTimePolicyAndSizeHigherNewer()
{
    //QSKIP("adjusting code");

    QStringList filenames;
    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy= UNQL::PerMinuteRotation;

    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
              << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt";

    cleanup(filenames);

    QDateTime dt = QDateTime::fromString("2021-03-16T02:00:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt.isValid());
    QDateTime dt1 = QDateTime::fromString("2021-03-16T02:01:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt1.isValid());

    overrideLastWrittenDateTime(dt);
    setOutputFile("log.txt");

    Q_ASSERT(QFileInfo::exists(filenames[0]));

    writeToFile(filenames[0], 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[1]);


    writeToFile(filenames[1], 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[2]);


    setTestingCurrentDateTime(dt1);
    rotateFilesIfNeeded(); //or rotateFileForTimePolicy();
    Q_ASSERT(!QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);


    cleanup(filenames);
}


void testFileWriter::testRotateForTimePolicyAndSizeStrict()
{
    //QSKIP("adjusting code");

    QStringList filenames;
    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::StrictRotation;
    m_Config.timeRotationPolicy= UNQL::PerMinuteRotation;

    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
              << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt"
              << "log-2021-03-16T02:01:00-1.txt";

    cleanup(filenames);

    QDateTime dt = QDateTime::fromString("2021-03-16T02:00:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt.isValid());
    QDateTime dt1 = QDateTime::fromString("2021-03-16T02:01:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt1.isValid());

    overrideLastWrittenDateTime(dt);
    setOutputFile("log.txt");

    Q_ASSERT(QFileInfo::exists(filenames[0]));

    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(!QFileInfo::exists(filenames[4]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(!QFileInfo::exists(filenames[4]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    setTestingCurrentDateTime(dt1);
    rotateFileForTimePolicy();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(!QFileInfo::exists(filenames[4]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);


    writeToFile(filenames[3], 2);
    rotateFileForStrictRotation();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(QFileInfo::exists(filenames[4]));
    Q_ASSERT(!QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);

    cleanup(filenames);
}


void testFileWriter::testRotateForIncrementalNumbers()
{
    //QSKIP("adjusting code");

    //init
    QStringList filenames;
    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy= UNQL::NoTimeRotation;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    //cleanup possible leftover from previous (failed tests)
    cleanup(filenames);

    //create base file
    setOutputFile(filenames[0]);

    //write to log.txt more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForIncrementalNumbers();

    //verify we created the new file
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[1]);


    //write to log-1 more than allowed size and rotate
    writeToFile(filenames[1], 2);
    rotateFileForIncrementalNumbers();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[2]);


    //write to log-2 more than allowed size and rotate
    writeToFile(filenames[2], 2);
    rotateFileForIncrementalNumbers();

    Q_ASSERT(!QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);

    foreach (QString s, filenames) {
        deleteFile(s);
    }
}


void testFileWriter::testRotateForStrictNumbers()
{
    //QSKIP("adjusting code");

    QStringList filenames;
    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::StrictRotation;
    m_Config.timeRotationPolicy= UNQL::NoTimeRotation;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    //cleanup possible leftover from previous (failed tests)
    resetLastUsedFilenames();
    foreach (QString s, filenames) {
        deleteFile(s);
    }

    //create base file
    setOutputFile(filenames[0]);

    //write to log.txt more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();

    //verify we created the new file
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    //write to log more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();

    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    //write to log more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();

    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);

    foreach (QString s, filenames) {
        deleteFile(s);
    }
}
