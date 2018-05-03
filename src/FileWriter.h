/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#ifndef __FILE_LOGGER_INCS__
#define __FILE_LOGGER_INCS__

#include "LogWriter.h"
#include <QFile>

class FileWriter: public LogWriter
{
	Q_OBJECT

private:
	QFile m_logFile;
	double m_maxFileSizeMB;
    int m_rotationMaxFileNumber, m_RotationCurFileNumber;
	bool m_streamIsOpen, m_fileSizeExceeded;
	QString m_logfileBaseName;
    FileRotationPolicyType m_fileRotationPolicy;
    QString lastUsedLogfilePostfix;

    int m_compressionLevel;
    int m_compressionAlgo;


	QString calculateCurrentFileName(int num=0);
	QString calculateOldLogFileName();
	void changeOutputFile(const QString&);
	void writeToDevice();
    void rotateFilesIfNeeded();

    void compressIfNeeded( const QString& i_toCompressFilename );
    const QString addCompressFileExtension(const QString& i_filename);

public:
    explicit FileWriter(FileRotationPolicyType i_rotationPolicy = StrictRotation);
    virtual ~FileWriter();

    virtual void setWriterConfig(const WriterConfig &wconf);

	void setOutputFile(const QString& filename="log.txt");
	void setLogfileMaxSize(int filesize);
	void setLogfileRotationRange(int maxfilenum);
	void stopLogging(bool erasefile=false);
    QString getBaseName() const { return m_logfileBaseName; }
    FileRotationPolicyType getRotationPolicy() const { return m_fileRotationPolicy; }
};

#endif

