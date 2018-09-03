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

    TimeRotationPolicyType m_timeRotationPolicy;
    QDateTime m_lastWrittenDateTime;
    QString lastUsedLogfilePostfix;
    QStringList m_lastUsedFilenames2;
    QQueue<QString> m_lastUsedFilenames;


    QString calculateCurrentFileName(int num=0);
    QString calculateOldLogFileName();
    void changeOutputFile(const QString&);
    void writeToDevice();
    void rotateFilesIfNeeded();
    void addNumberAndTimeToFilename(QString &sl, int filenum);
    void compressIfNeeded( const QString& i_toCompressFilename );
    const QString addCompressFileExtension(const QString& i_filename);

    void removeOldestFile();
    void rotateFileForIncrementalNumbers();
    void rotateFileForStrictRotation();

    void renameOldLogFiles();

public:
    explicit FileWriter(const WriterConfig &);
    virtual ~FileWriter();

    void setOutputFile(const QString& filename="log.txt");
    void setLogfileMaxSize(int filesize);
    void setLogfileRotationRange(int maxfilenum);
    void stopLogging(bool erasefile=false);
    QString getBaseName() const { return m_logfileBaseName; }
    FileRotationPolicyType getRotationPolicy() const { return m_Config.rotationPolicy; }
    bool isCompressionActive() const;
};

#endif

