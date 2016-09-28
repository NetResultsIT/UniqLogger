/*
 *   testlogger_zip.cpp
 *   (c) 2016 NetResults Srl
 */

#include "testlogger_zip.h"
#include <QTimer>
#include <QDebug>
#include <QFile>

#ifdef WIN32
    #include "windows.h"
    #define SLEEP(X) Sleep(X/1000)
#else
    #include <unistd.h>
    #define SLEEP(X) usleep(X)
#endif

TestloggerZip::TestloggerZip()
{
    m_unqloggerPtr = UniqLogger::instance();

    QTimer::singleShot( 0, this, SLOT(run_all_tests()) );
}

TestloggerZip::~TestloggerZip()
{

}


void TestloggerZip::writeLog(Logger* io_loggerPtr, int i_MBToWrite)
{
    qint32 byteLogged = 0;
    qint32 logStrSizeByte = 1024;
    qint32 byteToLog  = 1024*1024*i_MBToWrite;
    const QString logstr(logStrSizeByte, QChar('A')); /* 1KB string */
    while ( byteLogged < byteToLog )
    {
        *io_loggerPtr << logstr << UNQL::EOM;
        byteLogged += logstr.size();
        SLEEP(10000);
    }
}


void TestloggerZip::test_SingleFile(const QString& i_fileName, FileRotationPolicyType i_rotationPolicy, int i_compressionLevel)
{
    WriterConfig wconf;
    wconf.maxFileNum  = 1;
    wconf.maxFileSize = 1;  /* 1MB */
    wconf.compressionLevel = i_compressionLevel;
    wconf.rotationPolicy = i_rotationPolicy;
    Logger* flogPtr = m_unqloggerPtr->createFileLogger("ZIPTEST", i_fileName, wconf);

    qDebug() << "starting uniqlogger zip test...";

    /* Write 3MB to log */
    writeLog(flogPtr, 3);

    QString fileNameToCheck = i_fileName;
    if ( i_rotationPolicy == IncrementalNumbers )
    {
        QString filePattern("%1-%2");
        fileNameToCheck = filePattern.arg(i_fileName).arg(4);
    }

    if ( !QFile::exists(fileNameToCheck) )
    {
        qDebug() << "ERROR! Log file does not exists!";
    }else
    {
        qDebug() << "Log file correctly created!";
    }
    flogPtr->deleteLater();

    qDebug() << "uniqlogger zip test terminated...";
}

void TestloggerZip::test_3FilesStrictRotation(const QString& i_fileName, int i_compressionLevel)
{
    int fileCount = 3;

    WriterConfig wconf;
    wconf.maxFileNum  = fileCount;
    wconf.maxFileSize = 1;  /* 1MB */
    wconf.compressionLevel = i_compressionLevel;
    wconf.rotationPolicy = StrictRotation;
    Logger* flogPtr = m_unqloggerPtr->createFileLogger("ZIPTEST", i_fileName, wconf);

    qDebug() << "starting uniqlogger zip test...";

    /* Write 6MB to log */
    writeLog(flogPtr, 6);

    if ( !QFile::exists(i_fileName) )
    {
        qDebug() << "ERROR! Log file does not exists!";
        return;
    }

    QString filePatternToCheck("%1-%2");
    if ( i_compressionLevel > 0 )
    {
        filePatternToCheck.append(".zip");
    }
    for(int i = 1; i < fileCount; i++)
    {
        QString filezip = filePatternToCheck.arg(i_fileName).arg(QString::number(i));
        if ( !QFile::exists(filezip) )
        {
            qDebug() << "ERROR! Log file does not exists!";
            return;
        }
    }
    qDebug() << "Log files correctly created!";

    qDebug() << "uniqlogger zip test terminated...";
    flogPtr->deleteLater();

}


void TestloggerZip::test_3FilesIncrementalRotation(const QString& i_fileName, int i_compressionLevel)
{
    int fileCount = 3;

    WriterConfig wconf;
    wconf.maxFileNum  = fileCount;
    wconf.maxFileSize = 1;  /* 1MB */
    wconf.compressionLevel = i_compressionLevel;
    wconf.rotationPolicy = IncrementalNumbers;
    Logger* flogPtr = m_unqloggerPtr->createFileLogger("ZIPTEST", i_fileName, wconf);

    qDebug() << "starting uniqlogger zip test...";

    /* Write 6MB to log */
    writeLog(flogPtr, 6);

    QString lastFilePattern("%1-%2");
    QString previousFilePattern("%1-%2");
    if ( i_compressionLevel > 0 )
    {
        previousFilePattern.append(".zip");
    }
    int fileIndexStart = 5;
    for(int i = 0; i < (fileCount - 1); i++)
    {
        QString filezip = previousFilePattern.arg(i_fileName).arg(QString::number(i + fileIndexStart));
        if ( !QFile::exists(filezip) )
        {
            qDebug() << "ERROR! Log file does not exists!";
            return;
        }
    }

    if ( !QFile::exists(lastFilePattern.arg(i_fileName).arg(QString::number(fileIndexStart + fileCount - 1))) )
    {
        qDebug() << "ERROR! Log file does not exists!";
        return;
    }
    qDebug() << "Log files correctly created!";

    qDebug() << "uniqlogger zip test terminated...";

    flogPtr->deleteLater();

}


void TestloggerZip::test_compressSingleFileStrictRotation()
{
    test_SingleFile("ziptest_single_file_strictRotation", StrictRotation, 6);
}

void TestloggerZip::test_compressSingleFileIncrementalRotation()
{
    test_SingleFile("ziptest_single_file_IncrementalRotation", IncrementalNumbers, 6);
}

void TestloggerZip::test_noCompressSingleFileStrictRotation()
{
    test_SingleFile("ziptest_single_file_noCompress_strictRotation", StrictRotation, 0);
}

void TestloggerZip::test_noCompressSingleFileIncrementalRotation()
{
    test_SingleFile("ziptest_single_file_noCompress_IncrementalRotation", IncrementalNumbers, 0);
}

void TestloggerZip::test_compress3FilesStrictRotation()
{
    test_3FilesStrictRotation("ziptest_3_files_strictRotation", 6);
}

void TestloggerZip::test_noCompress3FilesStrictRotation()
{
    test_3FilesStrictRotation("ziptest_3_files_noCompress_strictRotation", 0);
}

void TestloggerZip::test_compress3FilesIncrementalRotation()
{
    test_3FilesIncrementalRotation("ziptest_3_files_incrementalRotation", 6);

}

void TestloggerZip::test_noCompress3FilesIncrementalRotation()
{
    test_3FilesIncrementalRotation("ziptest_3_files_noCompress_incrementalRotation", 0);

}

void TestloggerZip::run_all_tests()
{
    test_compressSingleFileStrictRotation();
    test_compressSingleFileIncrementalRotation();
    test_noCompressSingleFileStrictRotation();
    test_noCompressSingleFileIncrementalRotation();
    test_compress3FilesStrictRotation();
    test_noCompress3FilesStrictRotation();
    test_compress3FilesIncrementalRotation();
    test_noCompress3FilesIncrementalRotation();
}


