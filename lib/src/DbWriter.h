/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#ifndef __DB_LOGGER_INCS__
#define __DB_LOGGER_INCS__

#include "LogWriter.h"
#include <QFile>

class DbHandler;

class DbWriter: public LogWriter
{
	Q_OBJECT

    int m_maxFileSizeMB;
    bool m_fileSizeExceeded;
	QString m_logfileBaseName;
    DbHandler *m_dbh;

	void writeToDevice();

public:
    DbWriter(const QString &);
    virtual ~DbWriter();
    void setOutputFile(QString filename="log.txt");
    void setLogfileMaxSize(int filesize);
    QString getBaseName() {return m_logfileBaseName;}
};

#endif

