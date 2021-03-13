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
    QFile m_LogFile;
    bool m_streamIsOpen;
    LogFileInfo m_LogfileInfo;
    QString m_logfileBaseName;

    QDateTime m_lastWrittenDateTime;

protected:
    int m_rotationCurFileNumber;
    QQueue<QString> m_lastUsedFilenames;
    LogFileInfo calculateLogFilePattern(const QString &filename);
    QString calculateLogFileNameForIndex(int index);
    QString calculateNextLogFileName();
    QString calculatePreviousLogFileName(int index);
    void changeOutputFile(const QString&);
    void writeToDevice();
    void rotateFilesIfNeeded();
    int secsPassedSinceTimeRotationWasNeeded();
    QString compressIfNeeded( const QString& i_toCompressFilename );

    void removeOldestFiles();
    void rotateFileForIncrementalNumbers();
    void rotateFileForStrictRotation();
    void rotateFileForTimePolicy();

    void renameOldLogFiles();
    bool isCompressionActive() const;

public:
    explicit FileWriter(const WriterConfig &);
    virtual ~FileWriter();

    QString getCurrentLogFilename() const;
    void setOutputFile(const QString& filename="log.txt");
    void setLogfileMaxSize(int filesize);
    void setLogfileRotationRange(int maxfilenum);
    void stopLogging(bool erasefile=false);
    QString getBaseName() const { return m_logfileBaseName; }
    UNQL::FileRotationNamingPolicyType getRotationPolicy() const { return m_Config.rotationPolicy; }
};

#endif

