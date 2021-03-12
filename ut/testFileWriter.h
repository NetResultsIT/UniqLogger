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
    void testRotateOldFiles();
    void testCalculateLogInfo();
};

#endif // TESTFILEWRITER_H
