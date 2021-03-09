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
{ m_Config.maxFileSize = filesize; }
 

/*!
  \brief sets the number of files to be used in log rotation
  \param maxfilenum is the number of files the logger should use before doing a rotation
 */
void
FileWriter::setLogfileRotationRange(int maxfilenum)
{ m_Config.maxFileNum = maxfilenum; }
 

int FileWriter::secsPassedSinceTimeRotationWasNeeded()
{
    int timePassed = 0;

    QDateTime now = QDateTime::currentDateTime();

    if (
         (m_Config.timeRotationPolicy == UNQL::HourlyRotation
          && m_lastWrittenDateTime.secsTo(now) > 3600)
       ||
         (m_Config.timeRotationPolicy == UNQL::DailyRotation
          && m_lastWrittenDateTime.addDays(1) >= now)
       ||
         (m_Config.timeRotationPolicy == UNQL::PerMinuteRotation
          && m_lastWrittenDateTime.secsTo(now) > 60)
       )
    {
        timePassed = m_lastWrittenDateTime.secsTo(now);
    }

    return timePassed;
}

/*!
 * \brief FileWriter::addNumberAndTimeToFilename
 * \param sl
 * \param filenum
 */
bool FileWriter::addNumberAndTimeToFilename(QString &s, int filenum, int secsToAdd)
{
    s += "-";
    s += QString::number(filenum);

    QDateTime now = m_lastWrittenDateTime.addSecs(secsToAdd);
    if (
        m_Config.timeRotationPolicy == UNQL::HourlyRotation
       )
    {
        s += "-h" + now.toString("HH");
    }
    else if (
             m_Config.timeRotationPolicy == UNQL::DailyRotation
            )
    {
        QString dayPartFormat = "ddd"; //By default set as day of week
        //if (m_timeRotationPolicy == UNQL::DayOfMonthRotation)
        //    dayPartFormat = "dd";

        s += "-" + now.toString(dayPartFormat);
        m_lastWrittenDateTime = now;
    }

    return true;
}


/*!
 * \brief FileWriter::calculateLogFilePattern calculates the pattern (i.e. the logfilename with a placeholder for file numbering) for the logfile
 * \param i_filename the log basefilename
 * \param o_rPath the output variable containing the path where the new logfile will be
 * \param o_rPattern the pattern (i.e. containing the placeholder that will be replaced with log file number) of the logfile
 */
void FileWriter::calculateLogFilePattern(const QString &i_filename, QString &o_rPath, QString &o_rPattern)
{
    QString fullfilename = QDir::fromNativeSeparators(i_filename);
    QString filename = fullfilename.split("/").takeLast();
    QString filepath;


    if (fullfilename.lastIndexOf("/") >= 0) {
        filepath = fullfilename.left(fullfilename.lastIndexOf("/")) + "/";
    }

    //find how many dots are there within logfile name
    QStringList sl = filename.split(".");

    if (sl.count() == 1) //no dots
    {
        filename.append("-%1");
    }
    else if (sl.count() == 2) //one dot
    {
        filename = sl[0];
        filename.append("-%1");
        filename += ".";
        filename += sl[1];
    }
    else //more than one dot
    {
        sl[0] += "%1";
        filename = sl.join(".");
    }

    o_rPath = filepath;
    o_rPattern = filename;
}


/*!
  \brief calculates the log file that is going to be used for the logging
  \param i_fileOffset the offset from which we should start calculating (default = 0)
  */
QString
FileWriter::calculateNextLogFileName(int i_fileOffset)
{
    QString filepath, filepattern;
    calculateLogFilePattern(m_logfileBaseName, filepath, filepattern);
    Q_ASSERT(filepath == m_logfilePath);
    Q_ASSERT(filepattern == m_logfilePattern);
    int filenum;

    /*
     *  The file with num == 0 --> no number, will not have any special extension if
     *  compression is enabled, because it is the file we are logging on
     */
    if (i_fileOffset == 0) {
        if (m_Config.rotationPolicy == UNQL::StrictRotation) {
            return m_logfileBaseName;
        }

        filenum = m_rotationCurFileNumber;
    }
    else {
        filenum = i_fileOffset;
    }

    QString filename = filepattern.arg(filenum);

    return filepath + filename;
}



/*!
  \brief this method changes the logfile that is used for the log file
  \param i_filename the new file to be used
  This method is used during log rotation and the filename cannot change extension or path
  */
void
FileWriter::changeOutputFile(const QString &i_filename)
{
    QMutexLocker ml(&mutex);

    if (m_logFile.isOpen()) //we were already logging to a file so we close it
    {
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_INFO, "Closing previously opened logfile", LogMessage::getCurrentTstampString());
        m_logMessageList.append(lm);
        m_streamIsOpen = false;
        m_logFile.close();
    }

    m_logFile.setFileName(i_filename);
    m_logFile.open( QIODevice::WriteOnly | QIODevice::Append );
    if (!m_logFile.isOpen()) //we were already logging to a file
    {
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_CRITICAL, "Cannot open logfile " + i_filename + " for writing", LogMessage::getCurrentTstampString());
        m_logMessageList.append(lm);
        m_streamIsOpen = false;
        m_logFile.close();
        //TODO - the fallback case is not very clear, we try to log on the default file...
    }
    else
    {
        m_streamIsOpen = true;
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_INFO, "Opened logfile " + i_filename + " for writing", LogMessage::getCurrentTstampString());
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

    calculateLogFilePattern(m_logfileBaseName, m_logfilePath, m_logfilePattern);
    QString fname = calculateNextLogFileName();
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
 * i.e. log-1.txt will be deleted, log.txt -> log-1.txt and a new log.txt will be created
 */
void FileWriter::rotateFileForIncrementalNumbers()
{
    m_rotationCurFileNumber++;
    QString currFileName = calculateNextLogFileName();
    changeOutputFile( currFileName );

    QString previousFile = calculateNextLogFileName(m_rotationCurFileNumber - 1);
    if ( isCompressionActive() )
    {
        previousFile = compressIfNeeded( previousFile );
    }
    m_lastUsedFilenames.append(previousFile);

    removeOldestFile();
}


bool FileWriter::isCompressionActive() const
{
    if ( ( m_Config.compressionAlgo > 0 ) && ( m_Config.maxFileNum > 1) )
        return true;

    return false;
}


/*!
 * \brief FileWriter::removeOldestFile removes the files that should no longer be used for logging
 */
void FileWriter::removeOldestFile()
{
    if (m_Config.maxFileNum == 0)
        return;

    while (m_lastUsedFilenames.size() > m_Config.maxFileNum) {
        QString lastfile = m_lastUsedFilenames.dequeue();
        if (QFile::exists(lastfile)) {
            QFile::remove(lastfile);
        }
        else {
            ULDBG << lastfile << " does not exists, cannot delete";
        }
    }
}


/*!
 * \brief FileWriter::renameOldLogFiles
 * In case of strict rotation this method will rename all involved files accordingly (i.e. log-2 -> log-3, log-1 -> log-2 and log -> log-1)
 */
void FileWriter::renameOldLogFiles()
{
    for (int i = m_Config.maxFileNum-2; i>=0; i--) {
        QString olderfile = calculateNextLogFileName(i);
        QString newerfile = calculateNextLogFileName(i+1);

        if ( isCompressionActive() && i != 0) {
            olderfile = NrFileCompressor::getCompressedFilename(olderfile, static_cast<NrFileCompressor::compressedFileFormatEnum>(m_Config.compressionAlgo));
            newerfile = NrFileCompressor::getCompressedFilename(newerfile, static_cast<NrFileCompressor::compressedFileFormatEnum>(m_Config.compressionAlgo));
        }


        if (QFile::exists(olderfile)) {
            if (QFile::exists(newerfile)) {
                if (!QFile::remove(newerfile)) {
                    qCritical() << Q_FUNC_INFO << "Unable to remove older log file " << newerfile;
                    //TODO - log this message to writer
                }
            }
            ULDBG << "renaming " << olderfile << " to " << newerfile;
            if (!QFile::rename(olderfile, newerfile)) {
                qCritical() << Q_FUNC_INFO << "Unable to move log file " << olderfile << " to " << newerfile;
                //TODO - log this message to writer
            } else {
                newerfile = compressIfNeeded(newerfile);
            }
            //Add the file to last used
            if (!m_lastUsedFilenames.contains(newerfile))
                m_lastUsedFilenames.append(newerfile);
        }
        else {//TODO - log this message to writer
            ULDBG << olderfile << " does not exists: cannot rename it into " << newerfile;
        }
    }
}


/*!
 * \brief FileWriter::rotateFileForStrictRotation
 * rotate logfiles when we're logging always in the same set of files (i.e. log-1.txt -> log-2.txt and log.txt -> log-1.txt)
 */
void FileWriter::rotateFileForStrictRotation()
{
    //Close current logfile
    m_logFile.flush();
    m_logFile.close();

    //now move the other files starting from the one b4 last
    renameOldLogFiles(); /* loop to rename old rotated files */

    //Set the new file to log to.
    QString currFileName = calculateNextLogFileName();
    changeOutputFile( currFileName );
}


void FileWriter::rotateFileForTimePolicy()
{
    QString basename = m_logfileBaseName;
    int secsPassedSinceLastWrittenLog = secsPassedSinceTimeRotationWasNeeded();

    if (secsPassedSinceLastWrittenLog > 0) {
        bool b = addNumberAndTimeToFilename(basename, m_rotationCurFileNumber, secsPassedSinceLastWrittenLog);
        qDebug() << "time passed and new name should be: " << basename;
        changeOutputFile(basename);
    }
}


/*!
 * \brief FileWriter::rotateFilesIfNeeded
 * check whether the current log file needs to be rotated (i.e. size or time) and rotates alllog files involved
 */
void
FileWriter::rotateFilesIfNeeded()
{
    //first we check if we need to change thefle due to time rotation
    if (m_Config.timeRotationPolicy != UNQL::NoTimeRotation) {
        rotateFileForTimePolicy();
    }
    //check if we need to change file for its size
    if ( (m_Config.maxFileSize > 0) && ( (m_logFile.size() / 1000000.0) > m_Config.maxFileSize) )
    {
        switch (m_Config.rotationPolicy)
        {
        case UNQL::HigherNumbersOlder:
            rotateFileForIncrementalNumbers();
            break;
        case UNQL::StrictRotation:
            rotateFileForStrictRotation();
            break;
        default:
            break;
        } /* switch( rotation policy ) */
    }

}


/*!
 * \brief FileWriter::compressIfNeeded if the passed file is not already compressed (checks the filename extension) and compression algorithm is set then compresses the file
 * \param i_fileToBeCompressed the file that possibly will be compressed
 * \return name of compressed file
 */
QString
FileWriter::compressIfNeeded( const QString& i_fileToBeCompressed )
{
    if ( m_Config.compressionAlgo > 0 &&
         !i_fileToBeCompressed.endsWith(".gz") &&
         !i_fileToBeCompressed.endsWith(".zip") )
    {
        QString compressedfileName = NrFileCompressor::getCompressedFilename(i_fileToBeCompressed, static_cast<NrFileCompressor::compressedFileFormatEnum>(m_Config.compressionAlgo));
        ULDBG << "uncompress filename: " << i_fileToBeCompressed;
        ULDBG << "compressed filename:  " << compressedfileName;
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
        return compressedfileName;
    }

    //return unmodified filename
    return i_fileToBeCompressed;
}
