#include "testFileWriter.h"

#include <FileWriter.h>

//needed for compression name function
#include "ext/filecompressor/src/NrFileCompressor.h"

#include <QtTest/QtTest>

testFileWriter::testFileWriter(WriterConfig wc)
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
    if (!b ) {
        qWarning() << "Could not remove file " << fname << " - " << f.errorString();
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


void testFileWriter::cleanup(QStringList filenames) {
    resetLastUsedFilenames();
    foreach (QString s, filenames) {
        deleteFile(s);
    }
    setTestingCurrentDateTime(QDateTime());
    m_Config = WriterConfig();
}


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


void
testFileWriter::testRemoveOldestFiles()
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
    filenames << "log.txt" << "log-1.txt" << "log-2.txt" << "log-3.txt";

    //cleanup possible leftover from previous (failed tests)
    cleanup(filenames);

    m_Config.maxFileNum = 3;
    m_Config.timeRotationPolicy = UNQL::NoTimeRotation;
    m_Config.compressionAlgo = 1;

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


void testFileWriter::testRotateForTimePolicy(int compressionAlg)
{
    //QSKIP("adjusting code");

    QStringList filenames;
    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:01:00.txt"
              << "log-2021-03-16T02:02:00.txt" << "log-2021-03-16T02:03:00.txt";

    cleanup(filenames);

    QString compressedExt = getCompressedExten(compressionAlg);

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 0;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy = UNQL::PerMinuteRotation;
    m_Config.compressionAlgo = compressionAlg;


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

    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
        Q_ASSERT(!QFileInfo::exists(filenames[0]));
        Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
        Q_ASSERT(!QFileInfo::exists(filenames[2]));
        Q_ASSERT(!QFileInfo::exists(filenames[3]));
    }
    Q_ASSERT(getCurrentLogFilename() == filenames[1]);

    setTestingCurrentDateTime(dt2);
    rotateFileForTimePolicy();
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
        Q_ASSERT(!QFileInfo::exists(filenames[0]));
        Q_ASSERT(!QFileInfo::exists(filenames[1]));
        Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
        Q_ASSERT(!QFileInfo::exists(filenames[3]));
    }
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(getCurrentLogFilename() == filenames[2]);


    setTestingCurrentDateTime(dt3);
    rotateFileForTimePolicy();
    Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
        Q_ASSERT(!QFileInfo::exists(filenames[0]));
        Q_ASSERT(!QFileInfo::exists(filenames[1]));
        Q_ASSERT(!QFileInfo::exists(filenames[2]));
        Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
    }
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);

    foreach (QString s, filenames) {
        deleteFile(getCompressedFilename(s, compressionAlg));
    }
    cleanup(filenames);
}


void testFileWriter::testRotateForTimePolicyGzipCompressed()
{
    testRotateForTimePolicy(1);
}


void testFileWriter::testRotateForTimePolicyZipCompressed()
{
    testRotateForTimePolicy(2);
}

void testFileWriter::testRotateForTimePolicyAndSizeHigherNewer(int compressionAlg)
{
    //QSKIP("adjusting code");

    QStringList filenames;
    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
              << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt";

    cleanup(filenames);

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::HigherNumbersNewer;
    m_Config.timeRotationPolicy = UNQL::PerMinuteRotation;
    m_Config.compressionAlgo = compressionAlg;

    QString compressedExt = getCompressedExten(compressionAlg);

    QDateTime dt = QDateTime::fromString("2021-03-16T02:00:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt.isValid());
    QDateTime dt1 = QDateTime::fromString("2021-03-16T02:01:00", "yyyy-MM-ddThh:mm:ss");
    Q_ASSERT(dt1.isValid());

    overrideLastWrittenDateTime(dt);
    setOutputFile("log.txt");

    Q_ASSERT(QFileInfo::exists(filenames[0]));

    writeToFile(filenames[0], 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
            Q_ASSERT(!QFileInfo::exists(filenames[0]));
            Q_ASSERT(!QFileInfo::exists(filenames[2]));
            Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
            Q_ASSERT(!QFileInfo::exists(filenames[3]));
    }
    Q_ASSERT(getCurrentLogFilename() == filenames[1]);


    writeToFile(filenames[1], 2);
    rotateFilesIfNeeded(); //or rotateFileForIncrementalNumbers();
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
            Q_ASSERT(!QFileInfo::exists(filenames[0]));
            Q_ASSERT(!QFileInfo::exists(filenames[1]));
            Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
            Q_ASSERT(!QFileInfo::exists(filenames[3]));
    }
    Q_ASSERT(getCurrentLogFilename() == filenames[2]);


    setTestingCurrentDateTime(dt1);
    rotateFilesIfNeeded(); //or rotateFileForTimePolicy();
    Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    if (compressionAlg != 0) { //verify that uncompressed files are not there
            Q_ASSERT(!QFileInfo::exists(filenames[0]));
            Q_ASSERT(!QFileInfo::exists(filenames[1]));
            Q_ASSERT(!QFileInfo::exists(getCompressedFilename(filenames[3], compressionAlg)));
            Q_ASSERT(!QFileInfo::exists(filenames[2]));
    }
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);


    foreach (QString s, filenames) {
        deleteFile(getCompressedFilename(s, compressionAlg));
    }
    cleanup(filenames);
}


void testFileWriter::testRotateForTimePolicyAndSizeHigherNewerGzipCompressed()
{
    testRotateForTimePolicyAndSizeHigherNewer(1);
}


void testFileWriter::testRotateForTimePolicyAndSizeHigherNewerZipCompressed()
{
    testRotateForTimePolicyAndSizeHigherNewer(2);
}

void testFileWriter::testRotateForTimePolicyAndSizeStrict(int compressionAlg)
{
    //QSKIP("adjusting code");

    QStringList filenames;

    filenames << "log-2021-03-16T02:00:00.txt" << "log-2021-03-16T02:00:00-1.txt"
              << "log-2021-03-16T02:00:00-2.txt" << "log-2021-03-16T02:01:00.txt"
              << "log-2021-03-16T02:01:00-1.txt";

    cleanup(filenames);

    m_Config.maxFileNum = 3;
    m_Config.maxFileSize = 1;
    m_Config.rotationPolicy = UNQL::StrictRotation;
    m_Config.timeRotationPolicy = UNQL::PerMinuteRotation;
    m_Config.compressionAlgo = compressionAlg;

    QString compressedExt = getCompressedExten(compressionAlg);

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
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(!QFileInfo::exists(filenames[4]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[2], compressionAlg)));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(!QFileInfo::exists(filenames[4]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    setTestingCurrentDateTime(dt1);
    rotateFileForTimePolicy();
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[1], compressionAlg)));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(!QFileInfo::exists(filenames[4]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);


    writeToFile(filenames[3], 2);
    rotateFileForStrictRotation();
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[0], compressionAlg)));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(QFileInfo::exists(getCompressedFilename(filenames[4], compressionAlg)));
    Q_ASSERT(!QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);

    cleanup(filenames);
    foreach(QString s, filenames) {
        deleteFile(getCompressedFilename(s, compressionAlg));
    }
}


void testFileWriter::testRotateForTimePolicyAndSizeStrictGzipCompressed()
{
    //QSKIP("");
    testRotateForTimePolicyAndSizeStrict(1);
}


void testFileWriter::testRotateForTimePolicyAndSizeStrictZipCompressed()
{
    //QSKIP("");
    testRotateForTimePolicyAndSizeStrict(2);
}


void testFileWriter::testRotateForIncrementalNumbers(int compressionAlgorithm)
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
    Q_ASSERT(QFileInfo::exists(filenames[0] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[1]));
    Q_ASSERT(!QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[1]);


    //write to log-1 more than allowed size and rotate
    writeToFile(filenames[1], 2);
    rotateFileForIncrementalNumbers();
    Q_ASSERT(QFileInfo::exists(filenames[0] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[1] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[2]));
    Q_ASSERT(!QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[2]);


    //write to log-2 more than allowed size and rotate
    writeToFile(filenames[2], 2);
    rotateFileForIncrementalNumbers();

    Q_ASSERT(!QFileInfo::exists(filenames[0] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[1] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[2] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[3]));
    Q_ASSERT(getCurrentLogFilename() == filenames[3]);

    foreach (QString s, filenames) {
        deleteFile(s + compressedExt);
    }
}

void testFileWriter::testRotateForIncrementalNumbersGzipCompressed()
{
    testRotateForIncrementalNumbers(1);
}


void testFileWriter::testRotateForIncrementalNumbersZipCompressed()
{
    testRotateForIncrementalNumbers(2);
}

void testFileWriter::testRotateForStrictNumbers(int compressionAlgorithm)
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
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1] + compressedExt));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    //write to log more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();

    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(QFileInfo::exists(filenames[1] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[2] + compressedExt));
    Q_ASSERT(!QFileInfo::exists(filenames[3] + compressedExt));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);


    //write to log more than allowed size and rotate
    writeToFile(filenames[0], 2);
    rotateFileForStrictRotation();

    Q_ASSERT(!QFileInfo::exists(filenames[3] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[2] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[1] + compressedExt));
    Q_ASSERT(QFileInfo::exists(filenames[0]));
    Q_ASSERT(getCurrentLogFilename() == filenames[0]);

    foreach (QString s, filenames) {
        deleteFile(s + compressedExt);
    }
    cleanup(filenames);
}



void testFileWriter::testRotateForStrictNumbersGzipCompressed()
{
    testRotateForStrictNumbers(1);
}


void testFileWriter::testRotateForStrictNumbersZipCompressed()
{
    testRotateForStrictNumbers(2);
}
