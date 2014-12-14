/********************************************************************************
 *   Copyright (C) 2010-2014 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#include "FileWriter.h"

#include <QStringList>
#include <QTime>
#include <QFileInfo>
#include <qdir.h>

FileWriter::FileWriter()
: LogWriter()
{
    m_fileRotationPolicy = StrictRotation;
    m_fileSizeExceeded=false;
	m_maxFileSizeMB=1.0;
    m_RotationCurFileNumber=1;
	m_rotationMaxFileNumber=2;
    m_logfileBaseName="logfile.txt";
}


/*!
  \brief In the class dtor we want to flush whatever we might have got that is not written
  */
FileWriter::~FileWriter()
{
    //on exit, write all we've got
    this->flush();
    m_logFile.close();
#ifdef ULOGDBG
    qDebug() << Q_FUNC_INFO << "Deleting filewriter on " << m_logFile.fileName();
#endif
}


/*!
  \brief sets the maximum size (in Megabytes) allowed for a single log file
  \param filesize the max allowed file size
  */
void
FileWriter::setLogfileMaxSize(int filesize)
{   m_maxFileSizeMB=filesize;	}
 

/*!
  \brief sets the number of files to be used in log rotation
  \param maxfilenum is the number of files the logger should use before doing a rotation
 */
void
FileWriter::setLogfileRotationRange(int maxfilenum)
{   m_rotationMaxFileNumber=maxfilenum;	}
 

/*!
  \brief calculate the last log file used.
  \return the name of the last log file used or an empty string if an error occurs
  */
QString
FileWriter::calculateOldLogFileName()
{
    QString tmp="";

	int i = m_RotationCurFileNumber-m_rotationMaxFileNumber;
    if (i>0)
    {
        tmp = calculateCurrentFileName(i);
    }
    return tmp;
}
 

/*!
  \brief calculates the log file that is going to be used for the logging
  \param num the number of the current file name
  */
QString
FileWriter::calculateCurrentFileName(int num)
{
    QString fullfilename = QDir::fromNativeSeparators(m_logfileBaseName);
    QString filename = fullfilename.split("/").takeLast();
    QString filepath;
    if (fullfilename.lastIndexOf("/") >= 0)
        filepath = fullfilename.left(fullfilename.lastIndexOf("/")) + "/";
    int filenum;


    if (num == 0) {
        if (m_fileRotationPolicy == StrictRotation)
            return fullfilename;

        filenum = m_RotationCurFileNumber;
    }
    else {
        filenum = num;
    }

    //find how many dots are there
    QStringList sl = filename.split(".");

    if (sl.count() == 1) //no dots
    {
            filename.append("-");
            filename += QString::number(filenum);
    }
    else if (sl.count() == 2) //one dot
    {
            filename = sl[0];
            filename.append("-");
            filename += QString::number(filenum);
            filename += ".";
            filename += sl[1];
    }
    else //more than one dot
    {
            sl[0] += QString::number(filenum);
            filename = sl.join(".");
    }

    return filepath + filename;
}
 

/*!
  \brief this method changes the logfile that is used for the log file
  \param _filename the new file to be used
  This method is used during log rotation
  */
void
FileWriter::changeOutputFile(const QString &aFilename)
{
    if (m_logFile.isOpen()) //we were already logging to a file
    {
        mutex.lock();
        LogMessage lm("UniqLogger",UNQL::LOG_INFO,"Closing previously opened logfile",QDateTime::currentDateTime().toString("hh:mm:ss"));
        //m_logMessageList.append("["+QTime::currentTime().toString("hh:mm:ss")+"] [LOGGER] Closing previously opened logfile");
        m_logMessageList.append(lm);
        m_streamIsOpen=false;
        mutex.unlock();
        m_logFile.close();
    }

    m_logFile.setFileName(aFilename);
    m_logFile.open( QIODevice::WriteOnly | QIODevice::Append );
    if (!m_logFile.isOpen()) //we were already logging to a file
    {
        mutex.lock();
        LogMessage lm("UniqLogger",UNQL::LOG_CRITICAL,"Cannot open logfile "+aFilename+" for writing",QDateTime::currentDateTime().toString("hh:mm:ss"));
        m_logMessageList.append(lm);
        //m_logMessageList.append("["+QTime::currentTime().toString("hh:mm:ss")+"] [LOGGER] [CRITICAL] Cannot open logfile "+filename+" for writing!");
        m_streamIsOpen=false;
        mutex.unlock();
        m_logFile.close();
    }
    else
    {
        m_streamIsOpen=true;
        mutex.lock();
        LogMessage lm("UniqLogger",UNQL::LOG_INFO,"Opened logfile "+aFilename+" for writing",QDateTime::currentDateTime().toString("hh:mm:ss"));
        m_logMessageList.append(lm);
        //m_logMessageList.append("["+QTime::currentTime().toString("hh:mm:ss")+"] [LOGGER] Opened logfile "+filename+" for writing");
        mutex.unlock();
    }
}
 

/*!
  \brief sets the base name that will be used for the log files
  \param aFilename the basename of the log files
  */
void
FileWriter::setOutputFile(const QString& aFilename)
{
    m_logfileBaseName = aFilename;

    QString fname = calculateCurrentFileName();

    changeOutputFile(fname);
}
 

/*!
  \brief writes the messages in the queue on the current log file
  */
void
FileWriter::writeToDevice()
{
#ifdef ULOGDBG
   qDebug() << "Writing to file";
#endif
   if (!m_streamIsOpen)
       setOutputFile();
   if (!m_logIsPaused)
   {
       mutex.lock();
       int nummsg = m_logMessageList.count();
       for (int i=0; i<nummsg; i++)
           if (m_logFile.isOpen()) //we could be in the middle of changing logfile
           {
               QString terminator = "\n";

#ifdef WIN32
               terminator.prepend("\r");
#endif

               QString s = m_logMessageList.takeFirst().message() + terminator;
#ifdef ULOGDBG
               int dddd = m_logFile.write(s.toLatin1());
               qDebug() << "wrote " << dddd << " on " << m_logFile.fileName();
#else
	           m_logFile.write(s.toLatin1());
#endif
           }
       mutex.unlock();
       m_logFile.flush();

       rotateFilesIfNeeded();
   }
}


void
FileWriter::rotateFilesIfNeeded()
{
    //check if we need to change file
    if ( (m_maxFileSizeMB > 0) && ( (m_logFile.size() / 1000000.0) > m_maxFileSizeMB) )
    {
        if (m_fileRotationPolicy == IncrementalNumbers) {
            m_RotationCurFileNumber++;
            changeOutputFile(calculateCurrentFileName());
            //check to see if we need to delete some files
            QString oldfile = calculateOldLogFileName();
            if (!oldfile.isEmpty())
                    QFile::remove(oldfile);
        }
        else {
            m_logFile.flush();
            m_logFile.close();

            //remove the last file (if exists)
            QString lastfile = calculateCurrentFileName(m_rotationMaxFileNumber-1);
            if (QFile::exists(lastfile))
                QFile::remove(lastfile);
            else {
                qDebug() << lastfile << " does not exists, cannot delete";
            }

            //now move the other files starting from the one b4 last
            for (int i=m_rotationMaxFileNumber-2; i>=0; i--) {
                QString olderfile = calculateCurrentFileName(i);
                QString newerfile = calculateCurrentFileName(i+1);
                if (QFile::exists(olderfile)) {
                    QFile::rename(olderfile,newerfile);
                }
                else {
                    qDebug() << olderfile << " does not exists cannot rename into " << newerfile;
                }
            }

            changeOutputFile(calculateCurrentFileName());
        }
    }
}
 

void
FileWriter::setWriterConfig(const WriterConfig &wconf)
{
    LogWriter::setWriterConfig(wconf);

    m_maxFileSizeMB = wconf.maxFileSize;
    m_rotationMaxFileNumber = wconf.maxFileNum;
}
