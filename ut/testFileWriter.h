#ifndef TESTFILEWRITER_H
#define TESTFILEWRITER_H

#include <QObject>
#include <FileWriter.h>
class testFileWriter : public FileWriter
{
    Q_OBJECT
public:
    explicit testFileWriter(WriterConfig wc=WriterConfig());

signals:

private slots:
    void testRenameOldFiles();
    void testCalculateLogInfo();
    void testRotateForTimePolicy();
    void testRotateForTimePolicyAndSizeHigherNewer();
    void testRotateForIncrementalNumbers();
    void testRotateForStrictNumbers();
};

#endif // TESTFILEWRITER_H
