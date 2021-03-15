#ifndef TESTFILEWRITER_H
#define TESTFILEWRITER_H

#include <QObject>
#include <FileWriter.h>
class testFileWriter : public FileWriter
{
    Q_OBJECT
public:
    explicit testFileWriter(WriterConfig wc=WriterConfig());
    void cleanup(QStringList);

signals:

private slots:
    void testRenameOldFiles();
    void testCalculateLogInfo();
    void testRotateForTimePolicy();
    void testRotateForTimePolicyAndSizeHigherNewer();
    void testRotateForTimePolicyAndSizeStrict();
    void testRotateForIncrementalNumbers(int compressionAlg=0);
    void testRotateForIncrementalNumbersGzipCompressed();
    void testRotateForIncrementalNumbersZipCompressed();
    void testRotateForStrictNumbers(int compressionAlg=0);
    void testRotateForStrictNumbersGzipCompressed();
    void testRotateForStrictNumbersZipCompressed();
    void testRemoveOldestFiles();
};

#endif // TESTFILEWRITER_H
