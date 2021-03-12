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

void
testFileWriter::testRotateOldFiles()
{
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
    m_lastUsedFilenames.append(f1);
    m_lastUsedFilenames.append(f2);
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

    m_lastUsedFilenames.append(f1);
    m_lastUsedFilenames.append(f2);

    m_rotationCurFileNumber = 2;
    renameOldLogFiles();

    Q_ASSERT(!QFile::exists(f));
    Q_ASSERT(QFile::exists(f1));
    Q_ASSERT(QFile::exists(f2));

    //remove files for next test
    QFile del1(f1);
    QFile del2(f2);
    Q_ASSERT(del1.remove());
    Q_ASSERT(del2.remove());
}
