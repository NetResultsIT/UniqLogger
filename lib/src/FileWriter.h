/********************************************************************************
 *   Copyright (C) 2010-2021 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#ifndef UNQL_FILE_LOGGER_INCS
#define UNQL_FILE_LOGGER_INCS

#include "LogWriter.h"
#include <QFile>
#include <QDateTime>
#include <QQueue>

class LogFileInfo
{
public:
    QString path;
    QString basename;
    QString extension;
    QString pattern;
};

class FileWriter: public LogWriter
{
    Q_OBJECT

private:
    int m_rotationCurFileNumber;
    QFile m_LogFile;
    bool m_streamIsOpen;
    LogFileInfo m_LogfileInfo;
    QString m_logfileBaseName;

    QDateTime m_lastWrittenDateTime;
    QDateTime m_currentDateTimeUsedForTest;

protected:
    //Testing functions
    QQueue<QString> m_lastUsedFilenames;
    void overrideCurrentRotationNumber(int idx);
    void overrideLastWrittenDateTime(QDateTime dt);
    void setTestingCurrentDateTime(QDateTime dt);
    void resetLastUsedFilenames();
    QDateTime adjustDateTimeForFileSuffix(QDateTime);

    //normal usage functions
private:
    QString calculateLogFileNameForIndex(int index);
    QString calculateNextLogFileName();
    QString calculatePreviousLogFileName(int index);
    QDateTime getCurrentDateTime() const;
    void changeOutputFile(const QString&);
    int secsPassedSinceTimeRotationWasNeeded();
    QString compressIfNeeded( const QString& i_toCompressFilename );
    QString removeLogPath(const QString &);

protected:
    void writeToDevice();
    LogFileInfo calculateLogFilePattern(const QString &filename);
    int rotationSecondsForTimePolicy(UNQL::FileRotationTimePolicyType);
    void removeOldestFiles();
    void removeLeftoversFromPreviousRun();
    void rotateFileForIncrementalNumbers();
    void rotateFileForStrictRotation();
    void rotateFileForTimePolicy();
    void rotateFilesIfNeeded();

    void renameOldLogFilesForStrictRotation();
    bool isCompressionActive() const;

public:
    explicit FileWriter(const WriterConfig &);
    virtual ~FileWriter();

    QString getCurrentLogFilename() const;
    void setOutputFile(const QString& filename);
    void setLogfileMaxSize(int filesize);
    void setLogfileRotationRange(int maxfilenum);
    void stopLogging(bool erasefile=false);
    QString getBaseName() const { return m_logfileBaseName; }
    UNQL::FileRotationNamingPolicyType getRotationPolicy() const { return m_Config.rotationPolicy; }
};

#endif

