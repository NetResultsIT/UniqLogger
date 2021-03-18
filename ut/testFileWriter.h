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

public slots:
    //basic time rotation test methods not to be called directly
    void testRotateForTimePolicy(int compressionAlg, UNQL::FileRotationTimePolicyType timerotPolicy, QString initdatetime);
    void testRotateForTimePolicyAndSizeHigherNewer(int compressionAlg, UNQL::FileRotationTimePolicyType timerotPolicy, QString initdatetime);
    void testRotateForTimePolicyAndSizeStrict(int compressionAlg, UNQL::FileRotationTimePolicyType timerotPolicy, QString initdatetime);

private slots:
    void init(); //called before each test

private slots: //switch to public to ignore some
    //utility tests
    void testRenameOldFiles();
    void testCalculateLogInfo();
    void testRemoveOldestFiles();

    //time rotation (simple) tests
    void testRotateWithPerMinuteRotation(int compressionAlg=0);
    void testRotateWithElapsedMinutesRotation(int compressionAlg=0);
    void testRotateWithHourlyRotation(int compressionAlg=0);
    void testRotateWithDailyRotation(int compressionAlg=0);
    void testRotateWithPerMinuteRotationGzipCompressed();
    void testRotateWithPerMinuteRotationZipCompressed();
    void testRotateWithHourlyRotationGzipCompressed();
    void testRotateWithHourlyRotationZipCompressed();
    void testRotateWithDailyRotationGzipCompressed();
    void testRotateWithDailyRotationZipCompressed();
    void testRotateWithElapsedMinutesRotationGzipCompressed();
    void testRotateWithElapsedMinutesRotationZipCompressed();

    //time rotation with incremental numbers tests
    void testRotateWithPerMinuteRotationAndSizeHigherNewer(int compressionAlg=0);
    void testRotateWithPerMinuteRotationAndSizeHigherNewerGzipCompressed();
    void testRotateWithPerMinuteRotationAndSizeHigherNewerZipCompressed();
    void testRotateWithElapsedMinutesRotationAndSizeHigherNewer(int compressionAlg=0);
    void testRotateWithElapsedMinutesRotationAndSizeHigherNewerGzipCompressed();
    void testRotateWithElapsedMinutesRotationAndSizeHigherNewerZipCompressed();
    void testRotateWithHourlyRotationAndSizeHigherNewer(int compressionAlg=0);
    void testRotateWithHourlyRotationAndSizeHigherNewerGzipCompressed();
    void testRotateWithHourlyRotationAndSizeHigherNewerZipCompressed();
    void testRotateWithDailyRotationAndSizeHigherNewer(int compressionAlg=0);
    void testRotateWithDailyRotationAndSizeHigherNewerGzipCompressed();
    void testRotateWithDailyRotationAndSizeHigherNewerZipCompressed();

    // size-based rotation tests
    void testRotateForIncrementalNumbers(int compressionAlg=0);
    void testRotateForIncrementalNumbersGzipCompressed();
    void testRotateForIncrementalNumbersZipCompressed();
    void testRotateForStrictNumbers(int compressionAlg=0);
    void testRotateForStrictNumbersGzipCompressed();
    void testRotateForStrictNumbersZipCompressed();

    //time rotation with strict numbers tests
    void testRotateWithPerMinuteRotationAndSizeStrict(int compressionAlg=0);
    void testRotateWithPerMinuteRotationAndSizeStrictGzipCompressed();
    void testRotateWithPerMinuteRotationAndSizeStrictZipCompressed();
    void testRotateWithElapsedMinutesRotationAndSizeStrict(int compressionAlg=0);
    void testRotateWithElapsedMinutesRotationAndSizeStrictGzipCompressed();
    void testRotateWithElapsedMinutesRotationAndSizeStrictZipCompressed();
    void testRotateWithHourlyRotationAndSizeStrict(int compressionAlg=0);
    void testRotateWithHourlyRotationAndSizeStrictGzipCompressed();
    void testRotateWithHourlyRotationAndSizeStrictZipCompressed();
    void testRotateWithDailyRotationAndSizeStrict(int compressionAlg=0);
    void testRotateWithDailyRotationAndSizeStrictGzipCompressed();
    void testRotateWithDailyRotationAndSizeStrictZipCompressed();
};

#endif // TESTFILEWRITER_H
