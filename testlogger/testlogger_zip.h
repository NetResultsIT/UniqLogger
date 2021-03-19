/*
 *   testlogger_zip.h
 *   (c) 2016 NetResults Srl
 */

#ifndef _TESTLOGGER_ZIP_H_
#define _TESTLOGGER_ZIP_H_

#include <QObject>

#include "UniqLogger.h"

class TestloggerZip : public QObject
{
    Q_OBJECT

    UniqLogger*  m_unqloggerPtr;

    void writeLog(Logger *io_loggerPtr, int i_MBToWrite);

    void test_SingleFile(const QString &i_fileName, UNQL::FileRotationNamingPolicyType i_rotationPolicy, int i_compressionLevel);
    void test_compress3FilesIncrementalRotation();
    void test_noCompressSingleFileStrictRotation();
    void test_noCompressSingleFileIncrementalRotation();
    void test_3FilesStrictRotation(const QString &i_fileName, int i_compressionLevel);
    void test_noCompress3FilesStrictRotation();
    void test_3FilesIncrementalRotation(const QString &i_fileName, int i_compressionLevel);
    void test_noCompress3FilesIncrementalRotation();
protected slots:
    void run_all_tests();
    void test_compressSingleFileStrictRotation();
    void test_compress3FilesStrictRotation();
    void test_compressSingleFileIncrementalRotation();


public:
    TestloggerZip();
    ~TestloggerZip();

};


#endif /* _TESTLOGGER_ZIP_H_ */


