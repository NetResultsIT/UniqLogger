/********************************************************************************
 *   Copyright (C) 2010-2014 by NetResults S.r.l. ( http://www.netresults.it )  *
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

public:
    enum FileRotationPolicyType { StrictRotation, IncrementalNumbers };

private:
	QFile m_logFile;
	double m_maxFileSizeMB;
    int m_rotationMaxFileNumber, m_RotationCurFileNumber;
	bool m_streamIsOpen, m_fileSizeExceeded;
	QString m_logfileBaseName;
    FileRotationPolicyType m_fileRotationPolicy;

	QString calculateCurrentFileName(int num=0);
	QString calculateOldLogFileName();
	void changeOutputFile(const QString&);
	void writeToDevice();
    void rotateFilesIfNeeded();

public:
    explicit FileWriter();
    virtual ~FileWriter();

    virtual void setWriterConfig(const WriterConfig &wconf);

	void setOutputFile(const QString& filename="log.txt");
	void setLogfileMaxSize(int filesize);
	void setLogfileRotationRange(int maxfilenum);
	void stopLogging(bool erasefile=false);
    QString getBaseName() const { return m_logfileBaseName; }
};

#endif

