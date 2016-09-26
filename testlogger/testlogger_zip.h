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
    Logger*      m_flogPtr;

protected slots:
    void run_all_tests();
    void test_compressSingleFile();
    void test_uniqloggerCompressRotation();

public:
    TestloggerZip();
    ~TestloggerZip();

};


#endif /* _TESTLOGGER_ZIP_H_ */


