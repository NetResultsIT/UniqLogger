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

//TODO - check if there is a way to test renaming withou exposid the last used file Q
void
testFileWriter::testRenameOldFiles()
{
    //QSKIP("skipping for now");
    //cleanup
    resetLastUsedFilenames();

    m_Config.maxFileNum = 3;

    QString f("log.txt");
    QString f1("log-1.txt");
    QString f2("log-2.txt");

    createFile(f);
    Q_ASSERT(QFile::exists(f));

    setOutputFile(f);
    overrideCurrentRotationNumber(1);
    m_lastUsedFilenames.append(f1);

    renameOldLogFiles();
    Q_ASSERT(!QFile::exists(f));
    Q_ASSERT(QFile::exists(f1));
    //end of first test (renaming log into log-1)

    resetLastUsedFilenames();
    setOutputFile(f);
    createFile(f);
    createFile(f1);
    m_lastUsedFilenames.append(f2);
    m_lastUsedFilenames.append(f1);
    overrideCurrentRotationNumber(2);
    renameOldLogFiles();
    Q_ASSERT(!QFile::exists(f));
    Q_ASSERT(QFile::exists(f1));
    Q_ASSERT(QFile::exists(f2));
    //end of second test (renaming log-1 into log-2, then log into log-1)

    resetLastUsedFilenames();
    createFile(f);
    createFile(f1);
    createFile(f2);

    Q_ASSERT(QFile::exists(f));
    Q_ASSERT(QFile::exists(f1));
    Q_ASSERT(QFile::exists(f2));

    m_lastUsedFilenames.append(f2);
    m_lastUsedFilenames.append(f1);

    overrideCurrentRotationNumber(2);
    renameOldLogFiles();

    Q_ASSERT(!QFile::exists(f));
    Q_ASSERT(QFile::exists(f1));
    Q_ASSERT(QFile::exists(f2));

    //remove files for next test
    deleteFile(f1);
    deleteFile(f2);
}



void testFileWriter::testRotateForTimePolicy()
{
    //cleanup
    resetLastUsedFilenames();

    QStringList filenames;

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 0;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy= UNQL::PerMinuteRotation;

    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:01:00.txt"
              << "log-2021-03-16T02:02:00.txt" << "log-2021-03-16T02:03:00.txt";

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
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(getCurrentLogFilename() == filenames[1]);

    setTestingCurrentDateTime(dt2);
    rotateFileForTimePolicy();
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(getCurrentLogFilename() == filenames[2]);


    setTestingCurrentDateTime(dt3);
    rotateFileForTimePolicy();
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);
    Q_ASSERT(!QFileInfo::exists(filenames[0]));

    foreach (QString s, filenames) {
        deleteFile(s);
    }
}


void testFileWriter::testRotateForTimePolicyAndSizeHigherNewer()
{
    //cleanup
    resetLastUsedFilenames();
    setTestingCurrentDateTime(QDateTime());

    QStringList filenames;

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy= UNQL::PerMinuteRotation;

    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
              << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt";

    QDateTime dt = QDateTime::fromString("2021-03-16T02:00:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt.isValid());
    QDateTime dt1 = QDateTime::fromString("2021-03-16T02:01:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt1.isValid());

    overrideLastWrittenDateTime(dt);
    setOutputFile("log.txt");

    Q_ASSERT(QFileInfo::exists(filenames[0]));

    writeToFile(filenames[0], 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(getCurrentLogFilename() == filenames[1]);


    writeToFile(filenames[1], 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(getCurrentLogFilename() == filenames[2]);


    setTestingCurrentDateTime(dt1);
    rotateFilesIfNeeded(); //or rotateFileForTimePolicy();
    Q_ASSERT(!QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);


    foreach (QString s, filenames) {
        deleteFile(s);
    }
}


void testFileWriter::testRotateForTimePolicyAndSizeStrict()
{
    //cleanup
    resetLastUsedFilenames();
    setTestingCurrentDateTime(QDateTime());

    QStringList filenames;

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::StrictRotation;
    m_Config.timeRotationPolicy= UNQL::PerMinuteRotation;

    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
              << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt";

    QDateTime dt = QDateTime::fromString("2021-03-16T02:00:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt.isValid());
    QDateTime dt1 = QDateTime::fromString("2021-03-16T02:01:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt1.isValid());

    overrideLastWrittenDateTime(dt);
    setOutputFile("log.txt");

    Q_ASSERT(QFileInfo::exists(filenames[0]));

    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    setTestingCurrentDateTime(dt1);
    rotateFileForTimePolicy();
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);


    foreach (QString s, filenames) {
        deleteFile(s);
    }
}


void testFileWriter::testRotateForIncrementalNumbers()
{
    //cleanup
    resetLastUsedFilenames();

    QStringList filenames;

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy= UNQL::NoTimeRotation;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    //create base file
    createFile(filenames[0]);
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    setOutputFile(filenames[0]);

    //write to log.txt more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForIncrementalNumbers();

    //verify we created the new file
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(getCurrentLogFilename() == filenames[1]);


    //write to log-1 more than allowed size and rotate
    writeToFile(filenames[1], 2);
    rotateFileForIncrementalNumbers();

    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(getCurrentLogFilename() == filenames[2]);


    //write to log-2 more than allowed size and rotate
    writeToFile(filenames[2], 2);
    rotateFileForIncrementalNumbers();

    Q_ASSERT(!QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);

    foreach (QString s, filenames) {
        deleteFile(s);
    }
}


void testFileWriter::testRotateForStrictNumbers()
{
    resetLastUsedFilenames();
    QStringList filenames;

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::StrictRotation;
    m_Config.timeRotationPolicy= UNQL::NoTimeRotation;
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    //create base file
    createFile(filenames[0]);
    Q_ASSERT(QFileInfo::exists(filenames[0]));
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
