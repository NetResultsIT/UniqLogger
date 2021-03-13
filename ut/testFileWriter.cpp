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

void
testFileWriter::testRenameOldFiles()
{
    //QSKIP("skipping for now");
    //cleanup
    m_lastUsedFilenames.clear();

    m_Config.maxFileNum = 3;

    QString f("log.txt");
    QString f1("log-1.txt");
    QString f2("log-2.txt");

    createFile(f);
    Q_ASSERT(QFile::exists(f));

    setOutputFile(f);
    m_rotationCurFileNumber = 1;
    m_lastUsedFilenames.append(f1);

    renameOldLogFiles();
    Q_ASSERT(!QFile::exists(f));
    Q_ASSERT(QFile::exists(f1));
    //end of first test (renaming log into log-1)

    m_lastUsedFilenames.clear();
    setOutputFile(f);
    createFile(f);
    createFile(f1);
    m_lastUsedFilenames.append(f2);
    m_lastUsedFilenames.append(f1);
    m_rotationCurFileNumber = 2;
    renameOldLogFiles();
    Q_ASSERT(!QFile::exists(f));
    Q_ASSERT(QFile::exists(f1));
    Q_ASSERT(QFile::exists(f2));
    //end of second test (renaming log-1 into log-2, then log into log-1)

    m_lastUsedFilenames.clear();
    createFile(f);
    createFile(f1);
    createFile(f2);

    Q_ASSERT(QFile::exists(f));
    Q_ASSERT(QFile::exists(f1));
    Q_ASSERT(QFile::exists(f2));

    m_lastUsedFilenames.append(f2);
    m_lastUsedFilenames.append(f1);

    m_rotationCurFileNumber = 2;
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

}


void testFileWriter::testRotateForIncrementalNumbers()
{
    //cleanup
    m_lastUsedFilenames.clear();

    QStringList filenames;

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
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
    m_lastUsedFilenames.clear();
    QStringList filenames;

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::StrictRotation;
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
