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
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "NrFileCompressor.h"
#include <TimeUtils.h>

FileWriter::FileWriter(const WriterConfig &wc)
    : LogWriter(wc)
    , m_rotationCurFileNumber(0)
{
    m_logfileBaseName = "uniqlogfile.txt";
    m_lastWrittenDateTime = QDateTime::currentDateTime();

    //sanity check for time rotation
    if (wc.timeRotationPolicy == UNQL::ElapsedMinutesRotation && wc.maxMinutes <= 0) {
        QString msg = "This FileWriter was configured for time-based rotation based on elapsed minutes but maxMinutes values was set to %1: rotation will NOT happen ";
        msg = msg.arg(m_Config.maxMinutes);
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_WARNING, msg, LogMessage::getCurrentTstampString());
        m_logMessageList.append(lm);
    }
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

    if (
         (m_Config.timeRotationPolicy == UNQL::HourlyRotation
          && TimeUtils::hourTicked(m_lastWrittenDateTime, now))
       ||
         (m_Config.timeRotationPolicy == UNQL::DailyRotation
          && TimeUtils::dayTicked(m_lastWrittenDateTime, now))
       ||
         (m_Config.timeRotationPolicy == UNQL::PerMinuteRotation
          && TimeUtils::minuteTicked(m_lastWrittenDateTime, now))
       ||
         (m_Config.timeRotationPolicy == UNQL::ElapsedMinutesRotation
          && m_Config.maxMinutes > 0
          && m_lastWrittenDateTime.secsTo(now) >= 60 * m_Config.maxMinutes)
       )
    {
        timePassed = m_lastWrittenDateTime.secsTo(now);
    }

    return timePassed;
}


/*!
 * \brief FileWriter::calculateLogFilePattern calculates the pattern (i.e. the logfilename with a placeholder for file numbering) for the logfile
 * \param i_filename the log basefilename that will be analyzed to split into basic info and fill the LogFileInfo
 * \return LogFileInfo class containing data about the current log file
 */
LogFileInfo FileWriter::calculateLogFilePattern(const QString &i_filename)
{
    QString fullfilename = QDir::fromNativeSeparators(i_filename);
    QString filename = fullfilename.split("/").takeLast();

    LogFileInfo lfi;

    if (fullfilename.lastIndexOf("/") >= 0) {
        lfi.path = fullfilename.left(fullfilename.lastIndexOf("/")) + "/";
    } else {
        lfi.path = QDir::currentPath() + "/";
    }

    //find how many dots are there within logfile name
    QStringList sl = filename.split(".");
    //qDebug() << filename << sl;

    if (sl.count() == 1) //no dots e.g. Mylog
    {
        lfi.basename = filename;
    }
    else // one or more than one dot e.g. log.txt or a.long.name.with.dots.log
         // eveything before last dot is considered filename the last part the extension
    {
        lfi.extension = sl.takeLast();
        lfi.basename = sl.join(".");
    }

    // Calculate the logfile pattern using %1 for the file index and a *prepended* %2 in case
    // we're using a time-based rotation
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


QDateTime
FileWriter::adjustDateTimeForFileSuffix(QDateTime dt)
{
    QTime t = dt.time();
    switch (m_Config.timeRotationPolicy) {
    case UNQL::DailyRotation:
        t.setHMS(0, 0, 0);
        break;
    case UNQL::HourlyRotation:
        t.setHMS(t.hour(), 0, 0);
        break;
    case UNQL::PerMinuteRotation:
        t.setHMS(t.hour(), t.minute(), 0);
        break;
    default:
        //we do nothing for NoRotation or ElapsedMinutesRotation
        break;
    }

    if (m_Config.timeRotationPolicy != UNQL::ElapsedMinutesRotation) {
        dt.setTime(t);
    }
    return dt;
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
        if (m_Config.timeRotationPolicy != UNQL::NoTimeRotation) {
            // If we have a time rotation we have a pattern like %2%1 so the first argument
            // will be placed after the second one. See calculateLogFilePattern()
            patt = m_LogfileInfo.pattern.arg("").arg(m_lastWrittenDateTime.toString(DEF_UNQL_TIME_ROTATION_SUFFIX));
        }
    }
    else {
        if (m_Config.timeRotationPolicy != UNQL::NoTimeRotation) {
            // If we have a time rotation we have a pattern like %2%1 so the first argument
            // will be placed after the second one. See calculateLogFilePattern()
            patt = m_LogfileInfo.pattern.arg("-" + QString::number(index)).arg(m_lastWrittenDateTime.toString(DEF_UNQL_TIME_ROTATION_SUFFIX));
        } else {
            patt = m_LogfileInfo.pattern.arg("-" + QString::number(index));
        }
    }


    return m_LogfileInfo.basename + patt + "." + m_LogfileInfo.extension;
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
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_INFO, QString("Closing previously opened logfile ").append(getCurrentLogFilename()) , LogMessage::getCurrentTstampString());
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

    //look for possible leftovers
    removeLeftoversFromPreviousRun();
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
 * \brief FileWriter::rotationSecondsForTimePolicy utility function that return the amount of seconds that surely trigger a time rotation for the specified policy
 * \param timerotPolicy
 * \return the amount of seconds that should have elapsed to certainly trigger a time-rotation
 */
int FileWriter::rotationSecondsForTimePolicy(UNQL::FileRotationTimePolicyType timerotPolicy)
{
    int rotation_seconds = 60;
    if (timerotPolicy == UNQL::HourlyRotation)
        rotation_seconds = 3600;
    if (timerotPolicy == UNQL::DailyRotation)
        rotation_seconds = 3600 * 24;
    if (timerotPolicy == UNQL::NoTimeRotation)
        rotation_seconds = 0;
    return rotation_seconds;
}


/*!
 * \brief FileWriter::removeLeftoversFromPreviousRun
 * If the application using UniqLogger stops / crashes for any reason and then is restarted there might
 * be leftover logfiles from previous runs if we're using time-based rotation i.e. log-SOMEDAY_IN_THE_PAST.txt
 * So we look for those files (comparing against current configuration) and see if enough time has passed to remove
 * them
 * \todo this removes just old files, we do not take into account the possible numbering index i.e. log-DATE-N.txt
 */
void FileWriter::removeLeftoversFromPreviousRun()
{
    //the removal of leftovers makes sense only for time rotation
    if (m_Config.timeRotationPolicy == UNQL::NoTimeRotation)
        return;

    ULDBG << "Removing leftovers from a possible previous run...";
    LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_INFO, "Removing leftovers from a possible previous run...", LogMessage::getCurrentTstampString());
    m_logMessageList.append(lm);

    QStringList filelist = QDir(m_LogfileInfo.path).entryList(QDir::Files);
    ULDBG << "Files found in logpath: " << filelist;

    //remove current logfile
    filelist.removeAll(m_LogFile.fileName());

    QString re_string = "-(\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2})(-\\d+){0,1}\\.";
    QRegularExpression re(m_LogfileInfo.basename + re_string + m_LogfileInfo.extension);
    //qDebug() << "RE string: " << re_string << re.pattern();
    //qDebug() << "RE isvalid: " << re.isValid();
    QString remove_msg = "Removing leftover logfile %1 since %2 secs passed and timerotation policy is %3 meaning we should rotate every %4 seconds.";
    QString leave_msg = "leftover file %1 is deemed too recent to be removed";
    QString leftovermsg;
    foreach (QString f, filelist) {
        QRegularExpressionMatch match = re.match(f);
        if (match.hasMatch()) {
            //qDebug() << f << " matched our RE";
            //qDebug() << match.captured(1);
            QDateTime dt = QDateTime::fromString(match.captured(1), DEF_UNQL_TIME_ROTATION_FMT);
            int seconds_between_dtimes = qAbs(dt.secsTo(getCurrentDateTime()));
            int should_rotate_every_n_secs = rotationSecondsForTimePolicy(m_Config.timeRotationPolicy);
            if (seconds_between_dtimes > should_rotate_every_n_secs) {
                QString filefullpath = m_LogfileInfo.path + QDir::separator() + f;
                leftovermsg = remove_msg.arg(filefullpath).arg(seconds_between_dtimes).arg(m_Config.timeRotationPolicy).arg(should_rotate_every_n_secs);
                bool b = QFile::remove(filefullpath);
                if (!b) {
                    ULDBG << "ERROR ERROR - Could not delete file " << f;
                    LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_WARNING, "Could not remove leftover file " + filefullpath, LogMessage::getCurrentTstampString());
                    m_logMessageList.append(lm);
                }
            } else {
                leftovermsg = leave_msg.arg(f);
            }
            ULDBG << leftovermsg;
            LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_WARNING, leftovermsg, LogMessage::getCurrentTstampString());
            m_logMessageList.append(lm);
        } else {
            //qDebug() << f << " DID NOT match our RE";
        }
    }
}


/*!
 * \brief FileWriter::removeOldestFile removes the files that should no longer be used for logging
 * This will scan the last used files queue and remove oldest files that exceed the maximum defined queue length
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

        // We do not calculate the compressed filenames when i==1 because i=0 is the base logfilename and we already compressed
        // from (i.e.) log.txt to log-1.txt.gz in rotateFileForStrictRotation()
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
        ULDBG << "Compressing " << previousFile << " for incrementalnumbers rotation";
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
 * If the user selected "log.txt" as log file then we will write to log-yyyy-MM-ddTHH:mm:ss.txt log file and rotate
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

        if ( isCompressionActive() )
        {
            ULDBG << "Compressing " << previousFile << " for time rotation";
            previousFile = compressIfNeeded( previousFile );
        }

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
            ULDBG << "Filesize checking is disabled... ignoring";
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
        ULDBG << "compressed   filename: " << compressedfileName;
        mutex.lock();
        int ret = NrFileCompressor::fileCompress(i_fileToBeCompressed,
                                                 m_LogfileInfo.path,
                                                 m_LogfileInfo.path,
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
