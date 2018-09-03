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

#include "NrFileCompressor.h"

FileWriter::FileWriter(const WriterConfig &wc)
    : LogWriter(wc)
    , m_fileSizeExceeded(false)
{
    m_rotationCurFileNumber = 1;
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
{   m_Config.maxFileSize = filesize; }
 

/*!
  \brief sets the number of files to be used in log rotation
  \param maxfilenum is the number of files the logger should use before doing a rotation
 */
void
FileWriter::setLogfileRotationRange(int maxfilenum)
{   m_Config.maxFileNum = maxfilenum;   }
 

/*!
  \brief calculate the last log file used.
  \return the name of the last log file used or an empty string if an error occurs
  */
QString
FileWriter::calculateOldLogFileName()
{
    QString tmp="";

    int i = m_rotationCurFileNumber - m_Config.maxFileNum;
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
        if (m_Config.rotationPolicy == StrictRotation) {
            return fullfilename;
        }

        filenum = m_rotationCurFileNumber;
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
    QMutexLocker ml(&mutex);
    if ( !m_logFile.fileName().isEmpty() &&
         m_logFile.fileName() != aFilename &&
         !m_lastUsedFilenames.contains(m_logFile.fileName()))
        m_lastUsedFilenames.append(m_logFile.fileName());

    if (m_logFile.isOpen()) //we were already logging to a file so we close it
    {
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_INFO, "Closing previously opened logfile", LogMessage::getCurrentTstampString());
        m_logMessageList.append(lm);
        m_streamIsOpen = false;
        m_logFile.close();
    }

    m_logFile.setFileName(aFilename);
    m_logFile.open( QIODevice::WriteOnly | QIODevice::Append );
    if (!m_logFile.isOpen()) //we were already logging to a file
    {
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_CRITICAL, "Cannot open logfile " + aFilename + " for writing", LogMessage::getCurrentTstampString());
        m_logMessageList.append(lm);
        m_streamIsOpen = false;
        m_logFile.close();
        //TODO - the fallback case is not very clear, we try to log on the default file...
    }
    else
    {
        m_streamIsOpen = true;
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_INFO, "Opened logfile " + aFilename + " for writing", LogMessage::getCurrentTstampString());
        m_logMessageList.append(lm);
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
                LogMessage lm = m_logMessageList.takeFirst();
                QString m = lm.message();
                QString s = m + terminator;
#ifdef ENABLE_UNQL_DBG
                int dddd = m_logFile.write(s.toLatin1());
                qDebug() << "wrote " << dddd << " on " << m_logFile.fileName();
#else
                m_logFile.write(s.toLatin1());
#endif
            }
        m_logFile.flush();
        mutex.unlock();

        rotateFilesIfNeeded();
    }
}


/*!
 * \brief FileWriter::rotateFileForIncrementalNumbers
 * This will rotate the file with incremental numbering
 */
void FileWriter::rotateFileForIncrementalNumbers()
{
    m_rotationCurFileNumber++;
    QString currFileName = calculateCurrentFileName();
    changeOutputFile( currFileName );
    //check to see if we need to delete some files
    QString oldfile = calculateOldLogFileName();

    if (!oldfile.isEmpty()) {
        if ( isCompressionActive() )
        {
            oldfile = NrFileCompressor::getCompressedFilename(oldfile, static_cast<NrFileCompressor::compressedFileFormatEnum>(m_Config.compressionAlgo));
        }
        QFile::remove(oldfile);
    }

    if ( isCompressionActive() )
    {
        QString previousFile = calculateCurrentFileName(m_rotationCurFileNumber - 1);
        compressIfNeeded( previousFile );
    }
}


bool FileWriter::isCompressionActive() const
{
    if ( ( m_Config.compressionLevel > 0 ) && ( m_Config.maxFileNum > 1) )
        return true;

    return false;
}


void FileWriter::removeOldestFile()
{
//    QString lastfile = calculateCurrentFileName(m_Config.maxFileNum-1);
//    //Q_ASSERT(lastfile == m_lastUsedFilenames.last());

//    // if we are compressing the rotated log files, we need to add the extension to the filename (.zip|.gz)
//    if ( isCompressionActive() )
//    {
//        lastfile = addCompressFileExtension(lastfile);
//    }
    if (m_lastUsedFilenames.size() == 0)
        return;

    QString lastfile = m_lastUsedFilenames.dequeue();
    if (QFile::exists(lastfile)) {
        QFile::remove(lastfile);
    }
    else {
        ULDBG << lastfile << " does not exists, cannot delete";
    }
}


void FileWriter::renameOldLogFiles()
{
    for (int i = m_Config.maxFileNum-2; i>=0; i--) {
        QString olderfile = calculateCurrentFileName(i);
        QString newerfile = calculateCurrentFileName(i+1);
        if ( m_Config.compressionLevel > 0 && i != 0 ) {
            olderfile = addCompressFileExtension(olderfile);
            newerfile = addCompressFileExtension(newerfile);
        }

        if (QFile::exists(olderfile)) {
            if (QFile::exists(newerfile)) {
                if (!QFile::remove(newerfile)) {
                    qCritical() << Q_FUNC_INFO << "Unable to remove older log file " << newerfile;
                    //TODO - log this message to writer
                }
            }
            qDebug() << "renaming " << olderfile << " to " << newerfile;
            if (!QFile::rename(olderfile, newerfile)) {
                qCritical() << Q_FUNC_INFO << "Unable to move log file " << olderfile << " to " << newerfile;
                //TODO - log this message to writer
            }
        }
        else {
            ULDBG << olderfile << " does not exists: cannot rename it into " << newerfile;
        }
    }
}


void FileWriter::rotateFileForStrictRotation()
{
    m_logFile.flush();
    m_logFile.close();

    //remove the last file (if exists)
    //removeOldestFile();

    //now move the other files starting from the one b4 last
    renameOldLogFiles(); /* loop to rename old rotated files */

    QString currFileName = calculateCurrentFileName();
    changeOutputFile( currFileName );

    /* If the rotation file count is 1 we don't need to compress old files */
    if ( isCompressionActive() )
    {
        QString currFileName = calculateCurrentFileName(1);
        compressIfNeeded( currFileName );
    }
}

void
FileWriter::rotateFilesIfNeeded()
{
    //check if we need to change file for its size
    if ( (m_Config.maxFileSize > 0) && ( (m_logFile.size() / 1000000.0) > m_Config.maxFileSize) )
    {
        switch (m_Config.rotationPolicy)
        {
        case IncrementalNumbers:
            rotateFileForIncrementalNumbers();
            break;
        case StrictRotation:
            rotateFileForStrictRotation();
            break;
        default:
            break;
        } /* switch( rotation policy ) */
    }

    //TODO
    //check if we need to change the file due to timestamp
}


const QString
FileWriter::addCompressFileExtension(const QString& i_filename)
{
    return "";
//    return i_filename + QString(".") + FileCompressor::filenameExtension(
//                static_cast<FileCompressor::compressedFileFormatEnum>(m_Config.compressionAlgo) );
}

void
FileWriter::compressIfNeeded( const QString& i_fileToBeCompressed )
{
    if ( m_Config.compressionLevel > 0 )
    {
        QString compressedfileName = addCompressFileExtension(i_fileToBeCompressed);
        qDebug() << "to compress filename: " << i_fileToBeCompressed;
        qDebug() << "compressed filename:  " << compressedfileName;
        mutex.lock();
        int ret = NrFileCompressor::fileCompress(i_fileToBeCompressed,
                                     static_cast<NrFileCompressor::compressedFileFormatEnum>(m_Config.compressionAlgo),
                                     m_Config.compressionLevel);
        if (ret != 0) {
            qCritical() << Q_FUNC_INFO << "Error compressing file: " << i_fileToBeCompressed;
            //TODO - here we should log to file as well the error
        }
        else {
            /* remove original (uncompressed) file */
            QFile toRemove(i_fileToBeCompressed);
            bool b = toRemove.remove();
            if (!b) {
                qCritical() << Q_FUNC_INFO << "Error removing file to compress: " << i_fileToBeCompressed;
                //TODO - here we should log to file as well the error
            }
            else {
                ULDBG << "Removing file to compress (already compressed): " << i_fileToBeCompressed;
            }
        }
        mutex.unlock();
    }
}
