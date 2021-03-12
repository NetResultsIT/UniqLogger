/********************************************************************************
 *   Copyright (C) 2010-2021 by NetResults S.r.l. ( http://www.netresults.it )  *
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
{
    m_rotationCurFileNumber = 0;
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
    m_LogFile.close();
    ULDBG << Q_FUNC_INFO << "Deleting filewriter on " << m_LogFile.fileName();
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

    //TODO - we should compare not the whole lastwrittendatetime but only the day, hour, minute part
    //otherwise if we start at 16:57 we will trigger the hour change at 17:57
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
 * \brief FileWriter::calculateLogFilePattern calculates the pattern (i.e. the logfilename with a placeholder for file numbering) for the logfile
 * \param i_filename the log basefilename
 * \return LogFileInfo class containing data about the current log file
 */
LogFileInfo FileWriter::calculateLogFilePattern(const QString &i_filename)
{
    QString fullfilename = QDir::fromNativeSeparators(i_filename);
    QString filename = fullfilename.split("/").takeLast();

    LogFileInfo lfi;

    if (fullfilename.lastIndexOf("/") >= 0) {
        lfi.path = fullfilename.left(fullfilename.lastIndexOf("/")) + "/";
    }

    //find how many dots are there within logfile name
    QStringList sl = filename.split(".");
    qDebug() << filename << sl;

    if (sl.count() == 1) //no dots e.g. Mylog
    {
        lfi.basename = filename;
        filename.append("-%1");
        lfi.pattern = filename;
    }
    else if (sl.count() == 2) //one dot e.g: log.txt
    {
        filename = sl[0];
        lfi.basename = filename;
        filename.append("-%1");
        lfi.extension = sl[1];
        filename += ".";
        filename += sl[1];
        lfi.pattern = filename;
    }
    else //more than one dot e.g. a.long.name.with.dots.log
        //eveything before last dot is considered filename the last part the extension
    {
        lfi.extension = sl.takeLast();
        lfi.basename = sl.join(".");
        filename = lfi.basename + "-%1";
        lfi.pattern = filename;
    }

    //override
    lfi.pattern = "%1";
    if (m_Config.timeRotationPolicy != UNQL::NoTimeRotation)
        lfi.pattern.prepend("%2");

    return lfi;
}


QString
FileWriter::calculatePreviousLogFileName(int index)
{
    Q_ASSERT(index>=0);
    qDebug() << "Calculating previous log filename for index " << index;
    //if (index==0)
    //    return m_LogFile.fileName();

    QString tempfilename;
    QString extension = m_LogfileInfo.extension;

    tempfilename = calculateLogFileNameForIndex(index);
    /*if (index < 2) {
        qDebug() << "using next log filename";
        tempfilename = m_LogFile.fileName().left(m_LogFile.fileName().length() - (extension.length() + 1))
                         + "-" + QString::number(index) + "." + extension;
        tempfilename = calculateNextLogFileName();
    } else {
        qDebug() << "using replace method";
        tempfilename = tempfilename.replace("-"+QString::number(index), "-"+QString::number(index+1));
    }*/
    qDebug() << "calculated old log filename: " << tempfilename;
    return tempfilename;
}

QString
FileWriter::calculateLogFileNameForIndex(int index)
{
    QString patt;
    if (index == 0) {
        if (m_Config.timeRotationPolicy != UNQL::NoTimeRotation)
            patt = m_LogfileInfo.pattern.arg("").arg(m_lastWrittenDateTime.toString("-yyyy-MM-ddThh:mm:ss"));
    }
    else {
        if (m_Config.timeRotationPolicy != UNQL::NoTimeRotation)
            patt = m_LogfileInfo.pattern.arg("-" + QString::number(index)).arg(m_lastWrittenDateTime.toString("-yyyy-MM-ddThh:mm:ss"));
        else
            patt = m_LogfileInfo.pattern.arg("-" + QString::number(index));
    }


    return m_LogfileInfo.path + m_LogfileInfo.basename + patt + "." + m_LogfileInfo.extension;
}


/*!
  \brief calculates the log file that is going to be used for the logging
  \param i_fileOffset the offset from which we should start calculating (default = 0)
  */
QString
FileWriter::calculateNextLogFileName()
{
    return calculateLogFileNameForIndex(m_rotationCurFileNumber);
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
    ULDBG << "Setting new log file: " << i_filename;
    if (m_LogFile.isOpen()) //we were already logging to a file so we close it
    {
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_INFO, "Closing previously opened logfile", LogMessage::getCurrentTstampString());
        m_logMessageList.append(lm);
        m_streamIsOpen = false;
        m_LogFile.close();
    }

    m_LogFile.setFileName(i_filename);
    m_LogFile.open( QIODevice::WriteOnly | QIODevice::Append );
    if (!m_LogFile.isOpen()) //we were already logging to a file
    {
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_CRITICAL, "Cannot open logfile " + i_filename + " for writing", LogMessage::getCurrentTstampString());
        m_logMessageList.append(lm);
        m_streamIsOpen = false;
        m_LogFile.close();
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

    m_LogfileInfo = calculateLogFilePattern(m_logfileBaseName);
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
            if (m_LogFile.isOpen()) //we could be in the middle of changing logfile
            {
                QString terminator = "\n";

#ifdef WIN32
                terminator.prepend("\r");
#endif
                LogMessage lm = m_logMessageList.takeFirst();
                QString m = lm.message();
                QString s = m + terminator;
#ifdef ENABLE_UNQL_DBG
                int dddd = m_LogFile.write(s.toLatin1());
                qDebug() << "wrote " << dddd << " on " << m_LogFile.fileName();
#else
                m_LogFile.write(s.toLatin1());
#endif
            }
        m_LogFile.flush();
        mutex.unlock();

        rotateFilesIfNeeded();
    }
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
            ULDBG << "removing old logfile: " << lastfile;
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
    qDebug() << "last used files Q has " << m_lastUsedFilenames.size() << " elements to rename:" << m_lastUsedFilenames;
    /*if (m_lastUsedFilenames.size() < m_Config.maxFileNum) {
        qDebug() << "queue of old files is not yet full: "  << "manually renaming current files...";
        for (int i=m_lastUsedFilenames.size()-1; i>=0; i--) {
            QFile f(m_lastUsedFilenames.at(i));
            QString olderlogsfile = calculatePreviousLogFileName(i+1);
            qDebug() << "renaming " << m_lastUsedFilenames.at(i) << " into " << olderlogsfile;
            f.rename(olderlogsfile);
        }
        return;
    }*/

    //if we have more than m_Config.maxfiles files we get the last and delete it
    if (m_Config.maxFileNum > 0 && m_Config.maxFileNum <= m_lastUsedFilenames.size() + 1) {
        QString lastfile = m_lastUsedFilenames.last();
        if (QFile::exists(lastfile)) {
            qDebug() << "removing old logfile: " << lastfile;
            QFile::remove(lastfile);
        }
        else {
            ULDBG << lastfile << " does not exists, cannot delete";
        }
    }

    //now we shall rename each file
    /*for (int i=m_lastUsedFilenames.size()-2; i >= 0; i--) {
        qDebug() << "renaming " << m_lastUsedFilenames.at(i) << " into "
                 << m_lastUsedFilenames.at(i+1);
        QFile newer(m_lastUsedFilenames.at(i));
        newer.rename(m_lastUsedFilenames.at(i+1));
    }*/
    for (int i = m_rotationCurFileNumber; i > 0; i--)
    {
        qDebug() << "Processing renaming cycle " << i ;
        QString olderfile = calculatePreviousLogFileName(i);
        QString newerfile = calculatePreviousLogFileName(i-1);
        qDebug() << "renaming " << newerfile << " into "
                 << olderfile;
        QFile newer(newerfile);
        bool b = newer.rename(olderfile);
        Q_ASSERT(b);
    }
    qDebug() << "finished renaming files";

    /*
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
    */
}


/*!
 * \brief FileWriter::rotateFileForIncrementalNumbers
 * This will log into a file with a higher number, previous will be zipped (if enabled) and the oldest eventually deleted
 * i.e. log.txt will be deleted, log-1.txt will be zipped (if needed) and new messages will go into log-2.txt (newly created)
 */
void FileWriter::rotateFileForIncrementalNumbers()
{
    QString previousFile = m_LogFile.fileName();
    m_rotationCurFileNumber++;
    QString currFileName = calculateNextLogFileName();
    changeOutputFile( currFileName );

    //QString previousFile = calculateNextLogFileName(m_rotationCurFileNumber - 1);
    //Q_ASSERT(actuallog == previousFile);
    if ( isCompressionActive() )
    {
        previousFile = compressIfNeeded( previousFile );
    }
    m_lastUsedFilenames.append(previousFile);

    removeOldestFile();
}

/*!
 * \brief FileWriter::rotateFileForStrictRotation
 * rotate logfiles when we're logging always in the same set of files (i.e. log-1.txt -> log-2.txt and log.txt -> log-1.txt)
 */
void FileWriter::rotateFileForStrictRotation()
{
    //Close current logfile
    m_LogFile.flush();
    m_LogFile.close();

    //increment the rotation number and calculate the max rotation filename
    m_rotationCurFileNumber++;
    QString renamedlogfile = calculatePreviousLogFileName(m_rotationCurFileNumber);
    //append the last used file to be rotated
    qDebug() << "appending " << renamedlogfile << " to lastUsedFilenames";
    m_lastUsedFilenames.append(renamedlogfile);

    //now move the other files starting from the one b4 last
    renameOldLogFiles(); /* loop to rename old rotated files */

    //Set the new file to log to.
    //QString currFileName = calculateNextLogFileName();
    changeOutputFile( m_LogFile.fileName() );
}


void FileWriter::rotateFileForTimePolicy()
{
    QString newlogfilename;
    int secsPassedSinceLastWrittenLog = secsPassedSinceTimeRotationWasNeeded();

    qDebug() << "Seconds passed since rotation is needed (current time is " << QDateTime::currentDateTime()
             << "): " << secsPassedSinceLastWrittenLog;

    if (secsPassedSinceLastWrittenLog > 0) {
        ULDBG << "File rotation for TIME policy IS needed";
        QString previousFile = m_LogFile.fileName();
        //we need to reset the rotation index before calculating the new logfile name
        m_rotationCurFileNumber = 0;
        qDebug() << "Currently logging to " << previousFile;
        //we need to update the written date time before calculating the new logfile name
        m_lastWrittenDateTime = QDateTime::currentDateTime();
        newlogfilename = calculateNextLogFileName();
        qDebug() << "time passed and new name should be: " << newlogfilename;
        changeOutputFile(newlogfilename);
        m_lastUsedFilenames.append(previousFile);

        removeOldestFile();
    } else {
        ULDBG << "File rotation for TIME policy was NOT needed";
    }
    qDebug() << "Currently used files: " << m_lastUsedFilenames;
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
        ULDBG << "Checking if File rotation is needed due to TIME policy...";
        rotateFileForTimePolicy();
    }
    //check if we need to change file for its size, NOTE: if we rotated the file for time above its size will be 0 at first run
    if ( (m_Config.maxFileSize > 0) && ( (m_LogFile.size() / 1E6) > m_Config.maxFileSize) )
    {
        ULDBG << "File rotation needed due to SIZE policy";
        switch (m_Config.rotationPolicy)
        {
        case UNQL::HigherNumbersNewer:
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
