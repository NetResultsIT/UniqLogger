/********************************************************************************
 *   Copyright (C) 2010-2018 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#ifndef UNQL_DB_LOGGER_INCS
#define UNQL_DB_LOGGER_INCS

#include "LogWriter.h"
#include <QFile>

class UnqlDbHandler;

class DbWriter: public LogWriter
{
    Q_OBJECT

    int m_maxFileSizeMB;
    bool m_fileSizeExceeded;
    QString m_logfileBaseName;
    UnqlDbHandler *m_pDbHandler = nullptr;

    void writeToDevice();

public:
    DbWriter(const QString &, const WriterConfig &wconf);
    virtual ~DbWriter();
    void setOutputFile(QString filename="log.txt");
    void setLogfileMaxSize(int filesize);
    QString getBaseName() { return m_logfileBaseName; }
};

#endif

