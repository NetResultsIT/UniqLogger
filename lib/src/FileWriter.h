/********************************************************************************
 *   Copyright (C) 2010-2018 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#ifndef UNQL_FILE_LOGGER_INCS
#define UNQL_FILE_LOGGER_INCS

#include "LogWriter.h"
#include <QFile>
#include <QDateTime>
#include <QQueue>

class FileWriter: public LogWriter
{
    Q_OBJECT

private:
    QFile m_logFile;
    int m_rotationCurFileNumber;
    bool m_streamIsOpen, m_fileSizeExceeded;
    QString m_logfileBaseName;
    QString m_currentLogfileName;

    QDateTime m_lastWrittenDateTime;
    QString lastUsedLogfilePostfix;
    QQueue<QString> m_lastUsedFilenames;

protected:
    void calculateLogFilePattern(const QString &filename, QString &path, QString &pattern);
    QString calculateNextLogFileName(int offset=0);
    void changeOutputFile(const QString&);
    void writeToDevice();
    void rotateFilesIfNeeded();
    bool addNumberAndTimeToFilename(QString &sl, int filenum);
    QString compressIfNeeded( const QString& i_toCompressFilename );

    void removeOldestFile();
    void rotateFileForIncrementalNumbers();
    void rotateFileForStrictRotation();
    void rotateFileForTimePolicy();

    void renameOldLogFiles();
    bool isCompressionActive() const;

public:
    explicit FileWriter(const WriterConfig &);
    virtual ~FileWriter();

    void setOutputFile(const QString& filename="log.txt");
    void setLogfileMaxSize(int filesize);
    void setLogfileRotationRange(int maxfilenum);
    void stopLogging(bool erasefile=false);
    QString getBaseName() const { return m_logfileBaseName; }
    UNQL::FileRotationNamingPolicyType getRotationPolicy() const { return m_Config.rotationPolicy; }
};

#endif

