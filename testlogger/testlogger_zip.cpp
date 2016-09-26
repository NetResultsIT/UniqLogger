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

    WriterConfig wconf;
    wconf.maxFileNum  = 5;
    wconf.maxFileSize = 1;  /* 1MB */
    wconf.compressionLevel = 9;
    m_flogPtr = m_unqloggerPtr->createFileLogger("ZIPTEST", "ziptest", wconf);

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
    qint32 byteToLog  = 1024*1024*4;    /* 4MB */
    const QString logstr(logStrSizeByte, QChar('A')); /* 1KB string */
    while ( byteLogged < byteToLog )
    {
        *m_flogPtr << logstr << UNQL::EOM;
        //byteLogged += logstr.size();
        usleep(1000);
    }

    qDebug() << "uniqlogger zip test terminated...";
}

void TestloggerZip::run_all_tests()
{
    test_compressSingleFile();
    test_uniqloggerCompressRotation();
}


