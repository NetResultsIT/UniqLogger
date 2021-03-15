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
    , m_rotationCurFileNumber(0)
{
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
void FileWriter::setLogfileMaxSize(int filesize)
{ m_Config.maxFileSize = filesize; }
 

/*!
  \brief sets the number of files to be used in log rotation
  \param maxfilenum is the number of files the logger should use before doing a rotation
 */
void FileWriter::setLogfileRotationRange(int maxfilenum)
{ m_Config.maxFileNum = maxfilenum; }
 

/*!
 * \brief FileWriter::overrideCurrentRotationNumber Sets a new value for m_rotationCurFileNumber member
 * \param index the new value for m_rotationCurFileNumber
 * \warning this function should only be used for unit testing, using in production might stop correct uniqlogger behaviour
 */
void FileWriter::overrideCurrentRotationNumber(int index)
{ m_rotationCurFileNumber = index; }


/*!
 * \brief FileWriter::overrideLastWrittenDateTime Sets a new value for lastWrittenDateTime member
 * \param dt the new date time that should be used
 * \warning this function should only be used for unit testing, using in production might stop correct uniqlogger behaviour
 */
void FileWriter::overrideLastWrittenDateTime(QDateTime dt)
{ m_lastWrittenDateTime = dt; }


/*!
 * \brief FileWriter::overrideLastWrittenDateTime Sets a new value to be used as QDateTime::getCurrentDateTime()
 * \param dt the new date time that should be used as current DateTime
 * \warning this function *MUST* only be used for unit testing, using in production WILL stop correct uniqlogger behaviour
 */
void FileWriter::setTestingCurrentDateTime(QDateTime dt)
{ m_currentDateTimeUsedForTest = dt; }


/*!
 * \brief FileWriter::resetLastUsedFilenames clear the Queue containg the last used filenames
 * \warning this is a method to be used only during testing.
 */
void FileWriter::resetLastUsedFilenames()
{ m_lastUsedFilenames.clear(); }


/*!
 * \brief FileWriter::getCurrentDateTime a utility function that wraps QDateTime::currentDateTime and might override it for testing
 * \return current QDateTime
 */
QDateTime FileWriter::getCurrentDateTime() const
{
    if (m_currentDateTimeUsedForTest.isValid()) {
        ULDBG << "WARNING - using a user-set datetime: " << m_currentDateTimeUsedForTest;
        return m_currentDateTimeUsedForTest;
    }

    return QDateTime::currentDateTime();
}


/*!
 * \brief FileWriter::secsPassedSinceTimeRotationWasNeeded
 * \return the number of seconds passed since the last time-rotation was needed (or 0 if such time did not pass)
 */
int FileWriter::secsPassedSinceTimeRotationWasNeeded()
{
    int timePassed = 0;

    //use testing qdatetime if valid or the real now() if not.
    QDateTime now = getCurrentDateTime();

    //TODO - we should compare not the whole lastwrittendatetime but only the day, hour, minute part
    //otherwise if we start at 16:57 we will trigger the hour change at 17:57
    if (
         (m_Config.timeRotationPolicy == UNQL::HourlyRotation
          && m_lastWrittenDateTime.secsTo(now) >= 3600)
       ||
         (m_Config.timeRotationPolicy == UNQL::DailyRotation
          && m_lastWrittenDateTime.addDays(1) >= now)
       ||
         (m_Config.timeRotationPolicy == UNQL::PerMinuteRotation
          && m_lastWrittenDateTime.secsTo(now) >= 60)
       )
    {
        timePassed = m_lastWrittenDateTime.secsTo(now);
    }

    return timePassed;
}


/*!
 * \brief FileWriter::calculateLogFilePattern calculates the pattern (i.e. the logfilename with a placeholder for file numbering) for the logfile
 * \param i_filename the log basefilename that will be analyzed to split into basic info and fill he LogFileInfo
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
    //qDebug() << filename << sl;

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


/*!
 * \brief FileWriter::calculatePreviousLogFileName is simply a wrapper around calculateLogFileNameForIndex()
 * \param index the rotation index of the logfile we want to calculate the name of
 * \return the complete logfile name
 */
QString
FileWriter::calculatePreviousLogFileName(int index)
{
    Q_ASSERT(index>=0);
    ULDBG << "Calculating previous log filename for index " << index;

    QString tempfilename;

    tempfilename = calculateLogFileNameForIndex(index);

    ULDBG << "calculated old log filename: " << tempfilename;
    return tempfilename;
}


/*!
 * \brief FileWriter::calculateLogFileNameForIndex calculate the full name of the log file
 * \param index the rotation index used o calculate the new filename
 * \return a string containing the desired logfilename
 * This method will return the complete logfile name including its rotation index and time suffix
 * i.e.
 */
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
  \brief calculates the log file that is going to be used for the logging the next messages
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


    if (!m_lastUsedFilenames.contains(i_filename)) {
        m_lastUsedFilenames.enqueue(i_filename);
        ULDBG << "appended newest logfile " << i_filename << " to last used filenames that now is " << m_lastUsedFilenames ;
    }
    else {
        ULDBG << m_lastUsedFilenames << " already contains " << i_filename;
    }
}


/*!
 * \brief FileWriter::getCurrentLogFilename
 * \return the name of the file we're currently logging on
 */
QString FileWriter::getCurrentLogFilename() const
{ return m_LogFile.fileName(); }


/*!
  \brief sets the base name that will be used for the log files
  \param i_Filename the basename of the log files
  */
void
FileWriter::setOutputFile(const QString& i_Filename)
{
    m_logfileBaseName = i_Filename;
    m_rotationCurFileNumber = 0;

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
        int writtenbytes = 0;
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

                writtenbytes += m_LogFile.write(s.toLatin1());
            }
        m_LogFile.flush();
        mutex.unlock();

        ULDBG << "wrote " << writtenbytes << " on " << m_LogFile.fileName();

        rotateFilesIfNeeded();
    }
}


/*!
 * \brief FileWriter::isCompressionActive
 * \return a boolean indicating whether we should compress old log files
 */
bool FileWriter::isCompressionActive() const
{
    if ( ( m_Config.compressionAlgo > 0 ) && ( m_Config.maxFileNum > 1) )
        return true;

    return false;
}


/*!
 * \brief FileWriter::removeOldestFile removes the files that should no longer be used for logging
 */
void FileWriter::removeOldestFiles()
{
    ULDBG << "Checking whether there are some old logfiles to be removed";
    if (m_Config.maxFileNum == 0) {
        ULDBG << "There is no rotation, ignoring";
        return;
    }

    ULDBG << "Last used file names is currently: " << m_lastUsedFilenames;
    while (m_lastUsedFilenames.size() > m_Config.maxFileNum) {
        QString lastfile = m_lastUsedFilenames.dequeue();
        if (isCompressionActive()) {
            ULDBG << "Compression is active, adding compressed extension to " << lastfile;
            lastfile = NrFileCompressor::getCompressedFilename(lastfile, static_cast<NrFileCompressor::compressedFileFormatEnum>(m_Config.compressionAlgo));
        }
        if (QFile::exists(lastfile)) {
            ULDBG << "about to remove old logfile: " << lastfile;
            QFile::remove(lastfile);
        } else {
            ULDBG << lastfile << " does not seem to exist, cannot delete it ";
        }
    }

    ULDBG << "Last used file names is NOW: " << m_lastUsedFilenames;
}


/*!
 * \brief FileWriter::renameOldLogFilesForStrictRotation
 * In case of strict rotation this method will rename all involved files accordingly
 * (i.e. log-2 -> log-3, log-1 -> log-2 and log -> log-1)
 * \note this will rename only files that have the same basename, so if we changed basename (i.e. we manualy changed the
 * logfile basename OR time rotation is involved) the previous file will not be renamed (as they should have their correct naming)
 */
void FileWriter::renameOldLogFilesForStrictRotation()
{
    ULDBG << "last used files Q has " << m_lastUsedFilenames.size() << " elements to rename:" << m_lastUsedFilenames;

    //now we shall rename each file
    for (int i = m_rotationCurFileNumber; i > 0; i--)
    {
        //qDebug() << "Processing renaming cycle " << i ;
        QString olderfile = calculatePreviousLogFileName(i);
        QString newerfile = calculatePreviousLogFileName(i-1);

        if ( isCompressionActive() && i != 1) {
            olderfile = NrFileCompressor::getCompressedFilename(olderfile, static_cast<NrFileCompressor::compressedFileFormatEnum>(m_Config.compressionAlgo));
            newerfile = NrFileCompressor::getCompressedFilename(newerfile, static_cast<NrFileCompressor::compressedFileFormatEnum>(m_Config.compressionAlgo));
        }

        ULDBG << "renaming " << newerfile << " into " << olderfile;
        QFile newer(newerfile);
        QFile older(olderfile);
        if (older.exists()) {
            ULDBG << "target file " << olderfile << " exists, removing it before renaming";
            if (!older.remove()) {
                ULDBG << "error renaming file: " << older.errorString();
            }
        }
        bool b = newer.rename(olderfile);
        if (!b) {
            ULDBG << "error renaming file: " << newer.errorString();
        }
        Q_ASSERT(b);
    }
    ULDBG << "finished renaming files";
    ULDBG << "last used files Q has AFTER renaming" << m_lastUsedFilenames.size() << " elements to rename:" << m_lastUsedFilenames;




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

    if ( isCompressionActive() )
    {
        previousFile = compressIfNeeded( previousFile );
    }

    removeOldestFiles();
}

/*!
 * \brief FileWriter::rotateFileForStrictRotation will perform a logrotate-style of rotation
 * Rotate logfiles when we're logging always in the same set of files (i.e. log-1.txt -> log-2.txt and log.txt -> log-1.txt)
 * the file without suffix contains always the newest logs
 */
void FileWriter::rotateFileForStrictRotation()
{
    ULDBG << "Rotating files for STRICT rotation...";
    //Close current logfile
    m_LogFile.flush();
    m_LogFile.close();

    //increment the rotation number and calculate the max rotation filename
    if (m_rotationCurFileNumber < m_Config.maxFileNum - 1) {
        //ULDBG << "Incrementing m_rotationCurFileNumber from " << m_rotationCurFileNumber << " to " << m_rotationCurFileNumber + 1;
        m_rotationCurFileNumber++;
    }

    QString renamedlogfile = calculatePreviousLogFileName(m_rotationCurFileNumber);
    //we insert the name the current file will be renamed to at the beginning of last used file Q to be rotated
    ULDBG << "INSERTING " << renamedlogfile << " to lastUsedFilenames";
    if (!m_lastUsedFilenames.contains(renamedlogfile)) {
        QString previousFile = calculatePreviousLogFileName(m_rotationCurFileNumber - 1);
        //ULDBG << "Calculating the file that contains log before this: " << previousFile;
        int pos = m_lastUsedFilenames.indexOf(previousFile);
        //ULDBG << "found " << m_LogFile.fileName() << " in " << m_lastUsedFilenames << " at pos " << pos;
        m_lastUsedFilenames.insert(pos, renamedlogfile);
    } else {
        ULDBG << "NOT INSERTING since it's already there";
    }
    //ULDBG << "now last Q is: " << m_lastUsedFilenames;

    //check if there are any file to remove
    removeOldestFiles();

    //now move the other files starting from the one b4 last
    renameOldLogFilesForStrictRotation(); /* loop to rename old rotated files */

    if ( isCompressionActive() )
    {
        //we always need to compress the first file (i.e. log-1.txt) after rotation of ex-current mainfile (i.e. log.txt)
        QString previousFile = calculatePreviousLogFileName(1);
        ULDBG << "Compressing " << previousFile << " after strict rotation...";
        compressIfNeeded( previousFile );
    }

    //Set the same logfile to restart logging.
    changeOutputFile( m_LogFile.fileName() );
}


/*!
 * \brief FileWriter::rotateFileForTimePolicy will rotate the log files following the selected time policy
 * If the user selected "log.txt" as log file then we will write to log-yyyy-MM-ddThh:mm:ss.txt log file and rotate
 * (if enabled) by size using the usual policies (HigherNumbersNewer, StrictRotation)
 */
void FileWriter::rotateFileForTimePolicy()
{
    QString newlogfilename;
    int secsPassedSinceLastWrittenLog = secsPassedSinceTimeRotationWasNeeded();

    ULDBG << "Seconds passed since rotation is needed (current time is " << getCurrentDateTime()
             << "): " << secsPassedSinceLastWrittenLog;

    if (secsPassedSinceLastWrittenLog > 0) {
        ULDBG << "File rotation for TIME policy IS needed";
        QString previousFile = m_LogFile.fileName();
        //we need to reset the rotation index before calculating the new logfile name
        m_rotationCurFileNumber = 0;
        ULDBG << "Currently logging to " << previousFile;
        //we need to update the written date time before calculating the new logfile name
        m_lastWrittenDateTime = getCurrentDateTime();
        newlogfilename = calculateNextLogFileName();
        //qDebug() << "time passed and new name should be: " << newlogfilename;
        changeOutputFile(newlogfilename);
        ULDBG << "Current1 last used files: " << m_lastUsedFilenames;

        removeOldestFiles();
    } else {
        ULDBG << "File rotation for TIME policy was NOT needed";
    }
    ULDBG << "Current last used files: " << m_lastUsedFilenames;
}


/*!
 * \brief FileWriter::rotateFilesIfNeeded
 * check whether the current log file needs to be rotated (i.e. size or time) and rotates all log files involved
 */
void
FileWriter::rotateFilesIfNeeded()
{
    ULDBG << "Checking if file " << m_LogFile.fileName() << " needs to be rotated";
    //first we check if we need to change the file due to time rotation
    if (m_Config.timeRotationPolicy != UNQL::NoTimeRotation) {
        ULDBG << "Checking if File rotation is needed due to TIME policy...";
        rotateFileForTimePolicy();
    }
    //check if we need to change file for its size, NOTE: if we rotated the file for time above its size will be 0 at first run
    ULDBG << "Checking if File rotation is needed due to SIZE policy...";
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
        } /* switch( rotation policy ) */
    } else {
        if (m_Config.maxFileSize == 0) {
            ULDBG << "Filesize cheking is disabled... ignoring";
        } else {
            ULDBG << "File rotation due to SIZE NOT needed: accepted size is " << m_Config.maxFileSize << "MB and file "
                  << m_LogFile.fileName() << " is big " << m_LogFile.size() << " bytes ("<<  (m_LogFile.size() / 1E6) << "MB)";
        }
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
        ULDBG << "uncompressed filename: " << i_fileToBeCompressed;
        ULDBG << "compressed   filename:  " << compressedfileName;
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
