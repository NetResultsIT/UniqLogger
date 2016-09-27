/*
 *   testlogger_zip.cpp
 *   (c) 2016 NetResults Srl
 */

#include "testlogger_zip.h"
#include <QTimer>
#include <QDebug>
#include <unistd.h>
#include <QFile>

TestloggerZip::TestloggerZip()
{
    m_unqloggerPtr = UniqLogger::instance();

    m_fileName = "ziptest";
    m_fileCount = 5;

    WriterConfig wconf;
    wconf.maxFileNum  = m_fileCount;
    wconf.maxFileSize = 1;  /* 1MB */
    wconf.compressionLevel = 9;
    wconf.rotationPolicy = IncrementalNumbers;
    m_flogPtr = m_unqloggerPtr->createFileLogger("ZIPTEST", m_fileName, wconf);



    QTimer::singleShot( 0, this, SLOT(run_all_tests()) );
}

TestloggerZip::~TestloggerZip()
{
    m_flogPtr->deleteLater();
}

void TestloggerZip::test_compressSingleFile() {}

void TestloggerZip::test_uniqloggerCompressRotation()
{
    qDebug() << "starting uniqlogger zip test...";

    qint32 byteLogged = 0;
    qint32 logStrSizeByte = 1024;
    qint32 byteToLog  = 1024*1024*6;    /* 6MB */
    const QString logstr(logStrSizeByte, QChar('A')); /* 1KB string */
    while ( byteLogged < byteToLog )
    {
        *m_flogPtr << logstr << UNQL::EOM;
        byteLogged += logstr.size();
        usleep(10000);
    }

    qDebug() << "uniqlogger zip test terminated...";

}

void TestloggerZip::test_uniqloggerFilesGenerated()
{
    QString fileZipPattern("%1-%2.zip");
    int zipFileStartIndex = 3;
    for(int i = 0; i < (m_fileCount - 1); i++)
    {
        QString filezip = fileZipPattern.arg(m_fileName).arg(QString::number(i + zipFileStartIndex));
        if ( !QFile::exists(filezip) )
        {
            qDebug() << "ERROR! Log file does not exists!";
            return;
        }
    }
    qDebug() << "Log files correctly created!";
}

void TestloggerZip::run_all_tests()
{
    test_compressSingleFile();
    test_uniqloggerCompressRotation();
    test_uniqloggerFilesGenerated();
}


