/********************************************************************************
 *   Copyright (C) 2010-2018 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#include "DbWriter.h"

#include <QStringList>
#include <QTime>
#include <QFileInfo>
#include "UnqlDbHandler.h"

DbWriter::DbWriter(const QString &dbFileName, const WriterConfig &wconf)
    : LogWriter(wconf)
{
    m_fileSizeExceeded = false;
    m_maxFileSizeMB = 1;
    m_logfileBaseName = dbFileName;
    this->setOutputFile(dbFileName); //this will create the DbHandler Object
}


/*!
  \brief In the class dtor we want to flush whatever we might have got that is not written
  */
DbWriter::~DbWriter()
{
    //on exit, write all we've got
    this->flush();
    delete m_dbh;
    ULDBG << Q_FUNC_INFO << "Deleting dbwriter on " << m_logfileBaseName;
}


/*!
  \brief sets the maximum size (in Megabytes) allowed for a single log file
  \param filesize the max allowed file size
  */
void
DbWriter::setLogfileMaxSize(int filesize)
{   m_maxFileSizeMB = filesize;   }
 

/*!
  \brief sets the base name that will be used for the log files
  \param i_filename the basename of the log files
  */
void
DbWriter::setOutputFile(QString i_filename)
{
    m_logfileBaseName = i_filename;

    if (QFile::exists(i_filename)) {
        //TODO - choose if use the old file or delete it
    }
    else {
        UnqlDbHandler::createLoggerDb(i_filename);
    }

    DbhConfig conf("", "", "127.0.0.1", i_filename);
    conf.dbType = "QSQLITE";

    m_dbh = new UnqlDbHandler(conf);
}


/*!
  \brief writes the messages in the queue on the current log file
  */
void
DbWriter::writeToDevice()
{
    ULDBG << "Writing to db";

    if (m_logIsPaused)
        return;

    mutex.lock();
    int nummsg = m_logMessageList.count();
    for (int i=0; i<nummsg; i++) {
        LogMessage mess = m_logMessageList.takeFirst();
        QString s = mess.rawMessage();
        QString ts = mess.tstamp();
        int level = mess.level();
        QString ln = mess.loggerName();
        QString lev = QString::number(level);

        if (!s.isEmpty()) //do not write an empty entry on db
            m_dbh->logEvent(ln, ts, lev, s);
    }
    mutex.unlock();

    //check if we need to change file
    //QString curfile = calculateCurrentFileName();
    QFileInfo fi(m_logfileBaseName);
    if (m_maxFileSizeMB>0 && (fi.size()/1000000)>m_maxFileSizeMB)
    {
        //FIXME - implement rotation
    }
}


