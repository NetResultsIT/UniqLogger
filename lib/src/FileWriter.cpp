/********************************************************************************
 *   Copyright (C) 2010-2018 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#include "FileWriter.h"

#include <QStringList>
#include <QTime>
#include <QFileInfo>
#include <QDir>
#include <QQueue>

#include "FileCompressor.h"

FileWriter::FileWriter(FileRotationPolicyType i_rotationPolicy)
: LogWriter()
{
    m_fileRotationPolicy = i_rotationPolicy;
    m_fileSizeExceeded = false;
    m_maxFileSizeMB = 1.0;
    m_RotationCurFileNumber = 1;
    m_rotationMaxFileNumber = 2;
    m_logfileBaseName = "uniqlogfile.txt";
    m_lastWrittenDateTime = QDateTime::currentDateTime();
}


/*!
  \brief In the class dtor we want to flush whatever we might have got that is not written
  */
FileWriter::~FileWriter()
{
    //on exit, write all we've got
    this->flush();
    m_logFile.close();
    ULDBG << Q_FUNC_INFO << "Deleting filewriter on " << m_logFile.fileName();
}


/*!
  \brief sets the maximum size (in Megabytes) allowed for a single log file
  \param filesize the max allowed file size
  */
void
FileWriter::setLogfileMaxSize(int filesize)
{   m_maxFileSizeMB = filesize;	}
 

/*!
  \brief sets the number of files to be used in log rotation
  \param maxfilenum is the number of files the logger should use before doing a rotation
 */
void
FileWriter::setLogfileRotationRange(int maxfilenum)
{   m_rotationMaxFileNumber = maxfilenum;	}
 

/*!
  \brief calculate the last log file used.
  \return the name of the last log file used or an empty string if an error occurs
  */
QString
FileWriter::calculateOldLogFileName()
{
    QString tmp="";

    int i = m_RotationCurFileNumber - m_rotationMaxFileNumber;
    if (i>0)
    {
        tmp = calculateCurrentFileName(i);
    }
    return tmp;
}
 

/*!
 * \brief FileWriter::addNumberAndTimeToFilename
 * \param sl
 * \param filenum
 */
void FileWriter::addNumberAndTimeToFilename(QString &s, int filenum)
{
    s += "-";
    s += QString::number(filenum);

    QDateTime now = QDateTime::currentDateTime();

    if (
        m_timeRotationPolicy == HourlyRotation
        && m_lastWrittenDateTime.secsTo(now) > 3600
       )
    {
        s += "-h" + now.toString("HH");
        m_lastWrittenDateTime = now;
    }
    else if (
             (
                 m_timeRotationPolicy == DayOfWeekRotation
              || m_timeRotationPolicy == DayOfMonthRotation
             )
             && m_lastWrittenDateTime.addDays(1) >= now
            )
    {
        QString dayPartFormat = "ddd"; //By default set as day of week
        if (m_timeRotationPolicy == DayOfMonthRotation)
            dayPartFormat = "dd";

        s += "-" + now.toString(dayPartFormat);
        m_lastWrittenDateTime = now;
    }
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


    if (fullfilename.lastIndexOf("/") >= 0) {
        filepath = fullfilename.left(fullfilename.lastIndexOf("/")) + "/";
    }

    int filenum;

    /*
     *  The file with num == 0 --> no number, will not have any special extension if
     *  compression is enabled, because it is the file we are logging on
     */
    if (num == 0) {
        if (m_fileRotationPolicy == StrictRotation) {
            return fullfilename;
        }

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

#if (0)
    /*
     *  if we are compressing log files, the file will gave the extension .zip or .gz
     */
    if ( m_compressionLevel > 0 )
    {
        filename += FileCompressor::filenameExtension(
                    static_cast<FileCompressor::compressedFileFormatEnum>(m_compressionAlgo) );
    }
#endif

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
        LogMessage lm("UniqLogger", UNQL::LOG_INFO, "Closing previously opened logfile", QDateTime::currentDateTime().toString("hh:mm:ss"));
        m_logMessageList.append(lm);
        m_streamIsOpen = false;
        mutex.unlock();
        m_logFile.close();
        m_lastUsedFilenames.append(m_logFile.fileName());
    }

    m_logFile.setFileName(aFilename);
    m_logFile.open( QIODevice::WriteOnly | QIODevice::Append );
    if (!m_logFile.isOpen()) //we were already logging to a file
    {
        mutex.lock();
        LogMessage lm("UniqLogger", UNQL::LOG_CRITICAL, "Cannot open logfile " + aFilename + " for writing", QDateTime::currentDateTime().toString("hh:mm:ss"));
        m_logMessageList.append(lm);
        m_streamIsOpen = false;
        mutex.unlock();
        m_logFile.close();
    }
    else
    {
        m_streamIsOpen = true;
        mutex.lock();
        LogMessage lm("UniqLogger", UNQL::LOG_INFO, "Opened logfile " + aFilename + " for writing", QDateTime::currentDateTime().toString("hh:mm:ss"));
        m_logMessageList.append(lm);
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
   ULDBG << "Writing to file";

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
        switch (m_fileRotationPolicy)
        {
        case IncrementalNumbers:
        {
            m_RotationCurFileNumber++;
            QString currFileName = calculateCurrentFileName();
            changeOutputFile( currFileName );
            //check to see if we need to delete some files
            QString oldfile = calculateOldLogFileName();
            if (!oldfile.isEmpty()) {
                if ( ( m_compressionLevel > 0 ) && ( m_rotationMaxFileNumber > 1) )
                {
                    oldfile = addCompressFileExtension(oldfile);
                }
                QFile::remove(oldfile);
            }
            if ( ( m_compressionLevel > 0 ) && ( m_rotationMaxFileNumber > 1) )
            {
                QString previousFile = calculateCurrentFileName(m_RotationCurFileNumber - 1);
                compressIfNeeded( previousFile );
            }
        }
            break;
        case StrictRotation:
        {
            m_logFile.flush();
            m_logFile.close();

            //remove the last file (if exists)
            QString lastfile = calculateCurrentFileName(m_rotationMaxFileNumber-1);
            Q_ASSERT(lastfile == m_lastUsedFilenames.last());
            // if we are compressing the rotated log files, we need to add the extension to the filename (.zip|.gz)
            if ( ( m_compressionLevel > 0 ) && ( m_rotationMaxFileNumber > 1) )
            {
                lastfile = addCompressFileExtension(lastfile);
            }
            if (QFile::exists(lastfile)) {
                QFile::remove(lastfile);
            }
            else {
                ULDBG << lastfile << " does not exists, cannot delete";
            }

            //now move the other files starting from the one b4 last
            for (int i = m_rotationMaxFileNumber-2; i>=0; i--) {
                QString olderfile = calculateCurrentFileName(i);
                QString newerfile = calculateCurrentFileName(i+1);
                if ( m_compressionLevel > 0 && i != 0 ) {
                    olderfile = addCompressFileExtension(olderfile);
                    newerfile = addCompressFileExtension(newerfile);
                }
                if (QFile::exists(olderfile)) {
                    QFile::rename(olderfile,newerfile);
                }
                else {
                    ULDBG << olderfile << " does not exists cannot rename into " << newerfile;
                }
            } /* loop to rename old rotated files */

            QString currFileName = calculateCurrentFileName();
            changeOutputFile( currFileName );

            /* If the rotation file count is 1 we don't need to compress old files */
            if ( ( m_compressionLevel > 0 ) && ( m_rotationMaxFileNumber > 1) )
            {
                QString currFileName = calculateCurrentFileName(1);
                compressIfNeeded( currFileName );
            }
        }
            break;
        default:
            break;
        } /* switch( rotation policy ) */
    }
}

const QString
FileWriter::addCompressFileExtension(const QString& i_filename)
{
    return i_filename + QString(".") + FileCompressor::filenameExtension(
                static_cast<FileCompressor::compressedFileFormatEnum>(m_compressionAlgo) );
}

void
FileWriter::compressIfNeeded( const QString& i_toCompressFilename )
{
    if ( m_compressionLevel > 0 )
    {
        QString compressedfileName = addCompressFileExtension(i_toCompressFilename);
        qDebug() << "to compress filename: " << i_toCompressFilename;
        qDebug() << "compressed filename:  " << compressedfileName;
        mutex.lock();
        bool b = FileCompressor::fileCompress(i_toCompressFilename,
                                     compressedfileName,
                                     static_cast<FileCompressor::compressedFileFormatEnum>(m_compressionAlgo),
                                     m_compressionLevel);
        if (!b) {
            qDebug() << "Error compressing file: " << i_toCompressFilename;
        }
        else {
            /* remove original (uncompressed) file */
            QFile toRemove(i_toCompressFilename);
            b = toRemove.remove();
            if (!b) {
                qDebug() << "Error removing file to compress: " << i_toCompressFilename;
            }
            else {
                qDebug() << "Removing file to compress (already compressed): " << i_toCompressFilename;
            }
        }
        mutex.unlock();
    }
}

void
FileWriter::setWriterConfig(const WriterConfig &wconf)
{
    LogWriter::setWriterConfig(wconf);

    m_maxFileSizeMB = wconf.maxFileSize;
    m_rotationMaxFileNumber = wconf.maxFileNum;

    m_compressionLevel = wconf.compressionLevel;
    m_compressionAlgo  = wconf.compressionAlgo;
}
