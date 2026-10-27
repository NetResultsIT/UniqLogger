/********************************************************************************
 *   Copyright (C) 2010-2026 by Kalliope S.r.l. ( https://www.kalliope.com )    *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#include "UniqLogger.h"

#ifdef ENABLE_UNQL_NETLOG
 #include "RemoteWriter.h"
 #include "RSyslogWriter.h"
#endif

#ifdef ENABLE_UNQL_DBLOG
 #include "DbWriter.h"
#endif

#ifdef ENABLE_UNQL_ANDROIDLOG
 #include "AndroidWriter.h"
#endif

#include "FileWriter.h"
#include "ConsoleWriter.h"
#include "Logger.h"
#include "DummyWriter.h"
 
#include "nrthreadpool.h"

#include <QCoreApplication>

//Define (and not simply declare) the static members (see C++ FAQ 10.12)
QMutex UniqLogger::gmuxUniqLoggerInstance;
QMap<QString,UniqLogger*> UniqLogger::gUniqLoggerInstanceMap;
int UniqLogger::DEFAULT_OK = UNQL::UnqlErrorNoError;

extern QMap<UNQL::LogMessagePriorityType,QString> UnqlPriorityLevelNamesMap;
/*!
  \brief this is the class ctor, it is protected since we want just the singleton instance
  */
UniqLogger::UniqLogger(int nthreads)
    : m_instanceName(DEF_UNQL_INSTANCE_NAME)
    , m_defaultPrintTag(false)
{
    m_defaultTimeStampFormat = DEF_UNQL_TSTAMP_FMT;
    m_defaultSpaceChar = ' ';
    m_defaultStartEncasingChar = '[';
    m_defaultEndEncasingChar = ']';

    m_pTPool = new NRThreadPool(nthreads, writerThreadNamePrefix(), this);

    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_FATAL,    "FATAL");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_EMERGENCY,"EMERGENCY");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_ALARM,    "ALARM");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_CRITICAL, "CRITICAL");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_ERROR,    "ERROR");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_WARNING,  "WARNING");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_NOTICE,   "NOTICE");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_INFO,     "INFO");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_DBG,      "DEBUG");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_DBG_ALL,  "FULL DEBUG");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_MONITOR,  "MONITOR");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_FORCED,   "FORCED");

    ULDBG << "Being here with app: " << QCoreApplication::instance();
}
 


UniqLogger::~UniqLogger()
{
    ULDBG << Q_FUNC_INFO;
    QMutexLocker ml(&muxDeviceCounter);
    for (auto it = m_DevicesMap.begin(); it != m_DevicesMap.end(); ++it) {
        LogWriter *lw = it.key();
        if (lw && it.value() > 1) {
            lw->deactivateWriter();
        }
    }
    m_DevicesMap.clear();
}


/*!
 * \brief UniqLogger::threadsUsedForLogging
 * \return the number of threads that are available for the logging writers
 */
int
UniqLogger::threadsUsedForLogging() const
{
    int nt = 0;

    if (m_pTPool)
        return m_pTPool->threadsInPool();

    return nt;
}



/*!
  \brief this returns the singleton instance of the UniqLogger library
  */
UniqLogger*
UniqLogger::instance(const QString &ulname, int nthreads)
{
    UniqLogger *ulptr;
    static UniqLogger instance(nthreads);

    UniqLogger::gmuxUniqLoggerInstance.lock();
    if (gUniqLoggerInstanceMap.contains(ulname))
        ulptr = gUniqLoggerInstanceMap[ulname];
    else {
        if(gUniqLoggerInstanceMap.count() == 0) {
            ulptr = &instance;
        }
        else {
            ulptr = new UniqLogger(nthreads);
        }
        ulptr->m_instanceName = ulname;
        ulptr->m_pTPool->setPoolName(ulptr->writerThreadNamePrefix());
        gUniqLoggerInstanceMap.insert(ulname, ulptr);
    }
    UniqLogger::gmuxUniqLoggerInstance.unlock();

    if (nthreads == 0) { //adjust nthreads for the check below
        nthreads = QThread::idealThreadCount();
    }

    if (nthreads != ulptr->threadsUsedForLogging()) {
        qWarning() << "The UniqLogger instance was already configured with a different number of logging threads: "
                   << ulptr->threadsUsedForLogging() << " ignoring new value";
    }

    return ulptr;
}
 

/*!
 * \brief UniqLogger::setDefaultLogLevel sets the default loglevel accepted by newly created loggers
 * \param loglevel the default loge level that will be the minimum accepted by all newly created loggers
 * \note all existing logger will be unaffected
 * \version 1.1.0
 */
void
UniqLogger::setDefaultLogLevel(UNQL::LogMessagePriorityType loglevel)
{ m_defaultLogLevel = loglevel; }


QString
UniqLogger::writerThreadNamePrefix() const
{ return m_instanceName + "_LOGW"; }


void
UniqLogger::setDefaultPrintTag(bool enable)
{ m_defaultPrintTag = enable; }


bool
UniqLogger::defaultTagPrintingEnabled() const
{ return m_defaultPrintTag; }


QString
UniqLogger::instanceName() const
{ return m_instanceName; }


/*!
  \brief register a logwriter in the internal list (caller must hold muxDeviceCounter)
  \note Model B reference counting:
        - When a writer is first created by get*Writer(), it is inserted into m_DevicesMap
          with refcount = 1 (the base reference owned by UniqLogger itself, corresponding
          to 0 attached Loggers / dormant state).
        - Each Logger attached via addWriterToLogger() increments refcount (1 -> 2, 2 -> 3, ...).
          Transitioning from 1 (dormant) to 2 (first active Logger) re-activates the writer.
  */
void
UniqLogger::priv_registerWriter(LogWriter *lw)
{
    if (m_DevicesMap.contains(lw)) {
        int refcount = m_DevicesMap[lw];
        m_DevicesMap[lw] = refcount + 1;
        if (refcount == 1) {
            lw->activateWriter();
        }
    }
    else {
        m_DevicesMap.insert(lw, 1);
    }
}


/*!
  \brief register a logwriter in the internal list
  */
void
UniqLogger::registerWriter(LogWriter *lw)
{
    QMutexLocker ml(&muxDeviceCounter);
    priv_registerWriter(lw);
}



/*!
  \brief unregister a logwriter in the internal list
  \note Point 1, 3 & 4 fix:
        When the last Logger detaches (refcount drops from 2 to 1), we do NOT remove
        the writer from m_DevicesMap and do NOT call deleteLater(). Instead, we keep it
        in m_DevicesMap with refcount = 1 (dormant state) and call deactivateWriter(),
        which synchronously flushes pending messages, stops the timer, and closes the
        underlying file/device handle.
        This prevents:
        1) refcount underflow below 1;
        2) the deleteLater() race condition where a new Logger reopens the same file
           while the old FileWriter is still awaiting asynchronous deletion on NRThreadPool;
        3) loss of FileWriter rotation state (m_rotationCurFileNumber, m_lastUsedFilenames,
           m_lastWrittenDateTime) across temporary Loggers.
  */
void
UniqLogger::unregisterWriter(LogWriter *lw)
{
    ULDBG << Q_FUNC_INFO << "lw: " << lw;

    QMutexLocker ml(&muxDeviceCounter);
    if (m_DevicesMap.contains(lw)) {
        int refcount = m_DevicesMap[lw];
        if (refcount > 2) {
            m_DevicesMap[lw] = refcount - 1;
        }
        else if (refcount == 2) {
            m_DevicesMap[lw] = 1;
            lw->deactivateWriter();
        }
        else {
            Q_ASSERT(refcount >= 1);
            ULDBG << "Writer" << lw << "is already dormant (refcount == 1)";
        }
    }
    else {
        ULDBG << " +++++++++++++++ ERROR  deregistering writer +++++++++++++";
    }
}
 

/*!
  \brief this slot is called from Loggers' dtors in order to allow proper management of shared writers
  \param aList a list of pointers to LogWriter objects
  */
void
UniqLogger::writerFinished(const QList<LogWriter*> aList)
{
    ULDBG << Q_FUNC_INFO << "received a call to delete writers from finished logger " << sender();
    ULDBG << "about to delete writers: " << aList;

     foreach(LogWriter *lw, aList) {
         this->unregisterWriter(lw);
     }
}



bool
UniqLogger::checkMatchingConfigForWriter(LogWriter &w, const WriterConfig &wc)
{
    const WriterConfig &wc2 = w.getWriterConfig();
    if (wc2 != wc) {
        return false;
    }

    return true;
}


/*!
  \brief returns a generic Logger
  \param logname the module name for this logger
  \return a reference to the logger class created
  \note this logger does not have a default writer device, you should manually add one in order to see its output
  */
Logger*
UniqLogger::createLogger(const QString &logname)
{
    Logger *l = new Logger();                   //we need the friendship here
    if (l) {
        l->muxMonitorVar = &muxMonitorVarMap;   //we need the friendship here
        l->m_varMonitorMap = &m_VarMonitorMap;  //we need the friendship here

        if (m_defaultTimeStampFormat != DEF_UNQL_TSTAMP_FMT)
            l->setTimeStampFormat(m_defaultTimeStampFormat);
        if (m_defaultSpaceChar != ' ')
            l->setSpacingString(m_defaultSpaceChar);
        if (m_defaultStartEncasingChar != '[')
            l->setEncasingChars(m_defaultStartEncasingChar, m_defaultEndEncasingChar);

        //after having set the various strings we set the module name
        l->setModuleName(logname);
        l->setInstanceTag(m_instanceName);
        l->printTag(m_defaultPrintTag);

        l->setVerbosityDefaultLevel(m_defaultLogLevel);

        bool b = connect(l, SIGNAL(writersToDelete(const QList<LogWriter*>)), this, SLOT(writerFinished(const QList<LogWriter*>)), Qt::DirectConnection);
        Q_ASSERT(b);
        Q_UNUSED(b)
    }
    return l;
}



/*!
 * \brief creates a dummy logger: it will drop all the data sent to the logger
 *  you could consider as redirecting logs to /dev/null
 * \param logname  the modulename for this logger
 * \param wc the configuration to be used (only some params will be honoured) with this logger
 * \return the pointer to the logger class created or a null pointer if something went wrong
 */
Logger*
UniqLogger::createDummyLogger(const QString& logname, const WriterConfig &)
{
    Logger *l = createLogger(logname);
    LogWriter &dw = getDummyWriter();
    this->addWriterToLogger(l, dw);
    return l;
}


#ifdef ENABLE_UNQL_ANDROIDLOG
/*!
 * \brief creates an Android native logger and automatically connects a file writer with default values
 * \param logname the module name for this logger
 * \param wc the configuration to be used (only some params will be honoured) with this logger
 * \return the pointer to the logger class created or a null pointer if something went wrong
 */
Logger *UniqLogger::createAndroidLogger(const QString& logname, const WriterConfig &wc)
{
    Logger *l = createLogger(logname);
    LogWriter &dw = getAndroidWriter(wc);
    this->addWriterToLogger(l, dw);
    return l;
}
#endif


/*!
  \brief creates a logger and automatically connects a file writer with default values
  \param i_logname the module name for this logger
  \param i_filename the filename where this logger will write messages by default
  \param i_wconf the writerConfig class containing confguration options for this writer
  \param ok an output param containing the status code of this operation
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
Logger*
UniqLogger::createFileLogger(const QString & i_logname, const QString &i_filename, const WriterConfig &i_wconf, int &ok)
{
    if (i_wconf.neededSanitizing()) {
         ok = UNQL::UnqlErrorWriterConfigNotSane;
         return nullptr;
    }

    Logger *l = createLogger(i_logname);
    LogWriter &fw = getFileWriter(i_filename, i_wconf, ok);
    if (ok != UNQL::UnqlErrorNoError) {
        //FIXME - handle all the possible other errors
        WriterConfig wc2 = fw.getWriterConfig();
        QString msg = QString("Logger %3 failed to open file %4 (errcode %5) with this configuration:\n%1\nbut it had been previously opened with this one:\n%2").arg(i_wconf.toString()).arg(wc2.toString()).arg(i_logname).arg(i_filename).arg(ok);
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_WARNING, msg, LogMessage::getCurrentTstampString());
        fw.appendMessage(lm);
    }
    this->addWriterToLogger(l, fw);
    return l;
}
 


#ifdef ENABLE_UNQL_NETLOG
/*!
  \brief creates a logger and automatically connects a network writer with default values
  \param i_logname the module name for this logger
  \param i_ha the address where this logger will try to connect to write messages
  \param i_port the server port where this logger will try to connect to write messages
  \param i_wconf the writerConfig class containing confguration options for this writer
  \param ok an output param containing the status code of this operation
  \return a pointer to the logger class created
  */
Logger*
UniqLogger::createNetworkLogger(const QString & i_logname, const QString &i_ha, int i_port, const WriterConfig &i_wconf, int &ok)
{
    Logger *l = createLogger(i_logname);
    LogWriter &rw = getNetworkWriter(i_ha, i_port, i_wconf, ok);
    if (ok != UNQL::UnqlErrorNoError) {
        //FIXME - handle all the possible other errors
        WriterConfig wc2 = rw.getWriterConfig();
        QString msg = QString("Logger %3 tried to open this writer with this configuration:\n%1\nbut it had been previously opened with this one:\n%2").arg(i_wconf.toString()).arg(wc2.toString()).arg(i_logname);
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_WARNING, msg, LogMessage::getCurrentTstampString());
        rw.appendMessage(lm);
    }
    this->addWriterToLogger(l, rw);
    return l;
}



/*!
  \brief creates a logger and automatically connects a rsyslog writer with default values
  \param i_logname the module name for this logger
  \param i_id the message ID string reported by this logger as specified in RFC 5424
  \param i_facility the facility (0-23) reported by this logger as specified in RFC 5424
  \param i_ha the address where this logger will try to connect to write messages
  \param i_port the server port where this logger will try to connect to write messages
  \param i_wconf the writerConfig class containing confguration options for this writer
  \param ok an output param containing the status code of this operation
  \return a pointer to the logger class created
  */
Logger*
UniqLogger::createRSyslogLogger(const QString & i_logname, const QString &i_id, quint8 i_facility, const QString &i_ha, int i_port, const WriterConfig &i_wconf, int &ok)
{
    Logger *l = createLogger(i_logname);
    LogWriter &rw = getRSyslogWriter(i_id, i_facility, i_ha, i_port, i_wconf, ok);
    if (ok != UNQL::UnqlErrorNoError) {
        //FIXME - handle all the possible other errors
        WriterConfig wc2 = rw.getWriterConfig();
        QString msg = QString("Logger %3 tried to open this writer with this configuration:\n%1\nbut it had been previously opened with this one:\n%2").arg(i_wconf.toString()).arg(wc2.toString()).arg(i_logname);
        LogMessage lm(DEF_UNQL_LOG_STR, UNQL::LOG_WARNING, msg, LogMessage::getCurrentTstampString());
        rw.appendMessage(lm);
    }
    this->addWriterToLogger(l, rw);
    return l;
}
#endif 
 
 
#ifdef ENABLE_UNQL_DBLOG
/*!
  \brief creates a logger and automatically connects a DB writer with default values
  \param _logname the module name for this logger
  \param aDbFileName the name of the file that continas DB data
  \param wc the writerConfig class containing confguration options for this writer
  \param ok an output param containing the status code of this operation
  \return a reference to the logger class created
  */
Logger*
UniqLogger::createDbLogger(const QString & i_logname, const QString &aDbFileName, const WriterConfig &wc, int &ok)
{
    Logger *l = createLogger(i_logname);
    LogWriter &rlog = getDbWriter(aDbFileName, wc, ok);
    this->addWriterToLogger(l, rlog);
    return l;
}
#endif
 


/*!
  \brief creates a logger and automatically connects a console writer with default values
  \param logName the module name for this logger
  \param scheme the color scheme in which this logger will going to write messages with
  \param wc the writerConfig class containing confguration options for this writer
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
Logger*
UniqLogger::createConsoleLogger(const QString &logName, UNQL::ConsoleColorScheme scheme, const WriterConfig &wc)
{
    Logger *l = createLogger(logName);
    LogWriter &clog = getConsoleWriter(scheme, wc);
    this->addWriterToLogger(l, clog);
    return l;
}



/*!
  \brief returns a file writer that can be added to other loggers
        If it fails it throw an exception
  \param i_filename the filename where this logger will write messages by default
  \param wc the writerConfig class containing confguration options for this writer
  \param ok an output param containing the status code of this operation
  \return the reference to the logger class created
  */
LogWriter &UniqLogger::getFileWriter(const QString &i_filename, const WriterConfig &wc, int &ok)
{
    FileWriter *fw = nullptr;
    LogWriter *lw = nullptr;

    QMutexLocker ml(&muxDeviceCounter);

    //Assume we are successful
    ok = UNQL::UnqlErrorNoError;

    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        fw = dynamic_cast<FileWriter*>(lw);
        if (fw && fw->getBaseName() == i_filename) {
            int refcount = it.value();
            if ( !checkMatchingConfigForWriter(*fw, wc) )
            {
                if (refcount == 1) {
                    fw->setWriterConfig(wc);
                    ok = UNQL::UnqlErrorNoError;
                } else {
                    ULDBG << "WriterConfigs are incompatible";
                    ok = UNQL::UnqlErrorWriterConfigIncompatible;
                }
            }
            ULDBG << "Existing Filewriter for " << i_filename << " found! ------------------->><<-------" << fw;
            return *fw;
        }
        fw = nullptr;
    }

    fw = new FileWriter(wc);

    ULDBG << "Filewriter for file " << i_filename << " not found, creating one " << fw;

    // Initialize the output file (and m_logfileBaseName) BEFORE releasing muxDeviceCounter
    // so that a concurrent thread calling getFileWriter() for the same filename cannot
    // observe the default basename ("uniqlogfile.txt") and create a duplicate FileWriter.
    fw->setOutputFile(i_filename);
    priv_registerWriter(fw);
    ml.unlock();

    m_pTPool->runObject(fw);
    fw->run();

    return *fw;
}



#ifdef ENABLE_UNQL_DBLOG
/*!
  \brief returns a file writer that can be added to other loggers
  \param _filename the filename where this logger will write messages by default
  \param wc the writerConfig class containing confguration options for this writer
  \param ok an output param containing the status code of this operation
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
LogWriter&
UniqLogger::getDbWriter(const QString &_filename, const WriterConfig &wc, int &ok)
{
    DbWriter *dw = nullptr;
    LogWriter *lw = nullptr;

    QMutexLocker ml(&muxDeviceCounter);
    ok = UNQL::UnqlErrorNoError;

    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        dw = dynamic_cast<DbWriter*>(lw);
        if (dw && dw->getBaseName() == _filename) {
            ULDBG << "Existing DbWriter found! ------------------->><<-------" << dw;
            int refcount = it.value();
            if ( !checkMatchingConfigForWriter(*dw, wc) )
            {
                if (refcount == 1) {
                    dw->setWriterConfig(wc);
                    ok = UNQL::UnqlErrorNoError;
                } else {
                    ULDBG << "WriterConfigs are incompatible for DB writers";
                    ok = UNQL::UnqlErrorWriterConfigIncompatible;
                }
            }

            return *dw;
        }
        dw = nullptr;
    }

    dw = new DbWriter(_filename, wc);

    ULDBG << "Dbwriter for file "<< _filename <<" not found, creating one " << dw;

    priv_registerWriter(dw);
    ml.unlock();

    m_pTPool->runObject(dw);
    dw->run();

    return *dw;
}
#endif



#ifdef ENABLE_UNQL_NETLOG
/*!
  \brief returns a network writer connected to the specified address and port
  \param i_ha the address where this logger will try to connect to write messages
  \param i_port the server port where this logger will try to connect to write messages
  \param wc the writerConfig class containing confguration options for this writer
  \param ok an output param containing the status code of this operation
  \return a reference to the logwriter class created
  */
LogWriter &UniqLogger::getNetworkWriter(const QString & i_ha, int i_port, const WriterConfig &wc, int &ok)
{
    RemoteWriter *rw = nullptr;
    LogWriter *lw = nullptr;

    QMutexLocker ml(&muxDeviceCounter);
    ok = UNQL::UnqlErrorNoError;

    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        rw = dynamic_cast<RemoteWriter*>(lw);
        if (rw && rw->getHost() == i_ha && rw->getPort() == i_port) {
            ULDBG << "Existing Networkwriter found! ------------------->><<-------";
            int refcount = it.value();
            if ( !checkMatchingConfigForWriter(*rw, wc) )
            {
                if (refcount == 1) {
                    rw->setWriterConfig(wc);
                    ok = UNQL::UnqlErrorNoError;
                } else {
                    ULDBG << "WriterConfigs are incompatible for remote writers";
                    ok = UNQL::UnqlErrorWriterConfigIncompatible;
                }
            }
            return *rw;
        }
    }

    rw = new RemoteWriter(i_ha, static_cast<quint16>(i_port), wc);
    priv_registerWriter(rw);
    ml.unlock();

    m_pTPool->runObject(rw);
    rw->run();

    return *rw;
}



/*!
  \brief returns a network writer connected to the specified address and port
  \param i_id the message ID string reported by this logger as specified in RFC 5424
  \param i_facility the facility (0-23) reported by this logger as specified in RFC 5424
  \param i_ha the address where this logger will try to connect to write messages
  \param i_port the server port where this logger will try to connect to write messages
  \param wc the writerConfig class containing confguration options for this writer
  \param ok an output param containing the status code of this operation
  \return a reference to the logwriter class created
  */
LogWriter &UniqLogger::getRSyslogWriter(const QString &i_id, quint8 i_facility, const QString & i_ha, int i_port, const WriterConfig &wc, int &ok)
{
    RSyslogWriter *rw = nullptr;
    LogWriter *lw = nullptr;

    QMutexLocker ml(&muxDeviceCounter);
    ok = UNQL::UnqlErrorNoError;

    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        rw = dynamic_cast<RSyslogWriter*>(lw);
        if (rw && rw->getHost() == i_ha && rw->getPort() == i_port) {
            ULDBG << "Existing RSyslog found! ------------------->><<-------";
            int refcount = it.value();
            if ( !checkMatchingConfigForWriter(*rw, wc) )
            {
                if (refcount == 1) {
                    rw->setWriterConfig(wc);
                    ok = UNQL::UnqlErrorNoError;
                } else {
                    ULDBG << "WriterConfigs are incompatible for remote writers";
                    ok = UNQL::UnqlErrorWriterConfigIncompatible;
                }
            }
            return *rw;
        }
    }

    rw = new RSyslogWriter(i_id, i_facility, i_ha, static_cast<quint16>(i_port), wc);
    priv_registerWriter(rw);
    ml.unlock();

    m_pTPool->runObject(rw);
    rw->run();

    return *rw;
}
#endif



/*!
  \brief creates a ConsoleWriter with the specified color scheme.
  \note If a console writer with the same color scheme and writer config exists it will be reused.
  \param i_colorScheme The color scheme which this logger will use to write messages.
  \param wc the writerConfig class containing confguration options for this writer
  \param ok an output param containing the status code of this operation
  \return A reference to the LogWriter class.
  */
LogWriter &UniqLogger::getConsoleWriter(UNQL::ConsoleColorScheme i_colorScheme, const WriterConfig &wc, int &ok)
{
    LogWriter *lw;
    ConsoleWriter *cw;

    ok = UNQL::UnqlErrorNoError; //the search for ConsoleWriter cannot fail (we may create a new one if need be)

    QMutexLocker ml(&muxDeviceCounter);
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        cw = dynamic_cast<ConsoleWriter*>(lw);
        if (cw && cw->getColorScheme() == i_colorScheme) {
            int refcount = it.value();
            if ( !checkMatchingConfigForWriter(*cw, wc) )
            {
                if (refcount == 1) {
                    cw->setWriterConfig(wc);
                    ok = UNQL::UnqlErrorNoError;
                    return *cw;
                } else {
                    ULDBG << "WriterConfigs are incompatible for console writers";
                    continue; //there could be another consolewriter with same color but compatible config
                }
            }
            ULDBG << "Existing matching ConsoleWriter found! ------------------->><<-------" << cw;
            return *cw;
        }
        cw = nullptr;
    }

    cw = new ConsoleWriter(wc);
    priv_registerWriter(cw);
    cw->setColorScheme(i_colorScheme);
    ml.unlock();

    m_pTPool->runObject(cw);
    cw->run();

    return *cw;
}



/*!
 * \brief UniqLogger::getDummyWriter
 * \return
 */
LogWriter&
UniqLogger::getDummyWriter()
{
    QMutexLocker ml(&muxDeviceCounter);
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); ++it) {
        DummyWriter *dw = dynamic_cast<DummyWriter*>(it.key());
        if (dw) {
            return *dw;
        }
    }

    DummyWriter* dw = new DummyWriter();
    priv_registerWriter(dw);
    ml.unlock();

    m_pTPool->runObject(dw);
    dw->run();
    return *dw;
}


#ifdef ENABLE_UNQL_ANDROIDLOG
/*!
 * \brief UniqLogger::getAndroidWriter Creates a writer that will log to logcat on Android devices
 * \param wc the configuration for this writer
 * \note: only \em "common" part of WriterConfig will be taken into accoutn
 * \return the reference to the newly created logger
 */
LogWriter &UniqLogger::getAndroidWriter(const WriterConfig &wc)
{
    QMutexLocker ml(&muxDeviceCounter);
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); ++it) {
        AndroidWriter *aw = dynamic_cast<AndroidWriter*>(it.key());
        if (aw) {
            int refcount = it.value();
            if (!checkMatchingConfigForWriter(*aw, wc)) {
                if (refcount == 1) {
                    aw->setWriterConfig(wc);
                } else {
                    ULDBG << "WriterConfigs are incompatible for Android writers";
                }
            }
            return *aw;
        }
    }

    AndroidWriter* aw = new AndroidWriter(wc);
    priv_registerWriter(aw);
    ml.unlock();

    m_pTPool->runObject(aw);
    aw->run();

    return *aw;
}
#endif


/*!
  \brief Removes a LogWriter from a Logger it is connected to
  \param _l the logger to which the writer is connect
  \param writer Is the writer that is going to be removed from the logger
  \return a negative error message if the method fails, 0 otherwise
  \note Point 2 fix:
        1) Check lptr->removeLogDevice(lw) BEFORE calling unregisterWriter(lw): if the
           writer was not attached to _l (res != 0), we must NOT decrement the refcount
           belonging to other active Loggers.
        2) Append the debug message BEFORE unregisterWriter(lw) so that it is queued
           before deactivateWriter() flushes and closes the device (and never after a
           potential deletion).
  */
int
UniqLogger::removeWriterFromLogger(const Logger* _l, const LogWriter& writer)
{
    if (!_l) return -1;
    Logger *lptr = const_cast<Logger*>(_l);
    LogWriter *lw = const_cast<LogWriter*>(&writer);

    int res = lptr->removeLogDevice(lw);
    if (res == 0) {
        LogMessage lm (DEF_UNQL_LOG_STR, UNQL::LOG_DBG, QString("Removing logger %1 to this writer").arg(lptr->getModuleName()), LogMessage::getCurrentTstampString());
        lw->appendMessage(lm);
        this->unregisterWriter(lw);
    }
    return res;
}
 


/*!
  \brief Adds a LogWriter to a Logger
  \param _l the logger to which the writer is going to be connected
  \param writer Is the writer that is going to be added to the logger
  \return a negative error message if the method fails, 0 otherwise
  \note the only reason it might fail is if the writer was already connected
  */
int
UniqLogger::addWriterToLogger(const Logger* _l, const LogWriter &writer)
{
    if (!_l) return -1;
    int res = 0;
    Logger *lptr = const_cast<Logger*>(_l);
    LogWriter *lw = const_cast<LogWriter*>(&writer);

    res = lptr->addLogDevice(lw);
    if (res == 0) {
        this->registerWriter(lw);
        LogMessage lm (DEF_UNQL_LOG_STR, UNQL::LOG_DBG, QString("Adding logger %1 to this writer").arg(lptr->getModuleName()), LogMessage::getCurrentTstampString());
        lw->appendMessage(lm);
    }
    return res;
}


/*!
  \brief adds a new monitored variable
  \param var is the key that identifies a monitored variable
  \param status the initial monitor status of the variable (default is false)
  */
void
UniqLogger::addMonitorVar(const QString &var, bool status)
{
    muxMonitorVarMap.lock();
        m_VarMonitorMap[var] = status;
    muxMonitorVarMap.unlock();
}


/*!
  \brief changes the monitor status of a monitored variable
  \param var is the key that identifies a monitored variable
  \param status the new monitor status of the variable
  */
void
UniqLogger::changeMonitorVarStatus(const QString &var, bool status)
{
    muxMonitorVarMap.lock();
    if (m_VarMonitorMap.contains(var)) {
        m_VarMonitorMap[var] = status;
    }
    else {
        ULDBG << "The var is not monitored " << var;
    }
    muxMonitorVarMap.unlock();
}



/*!
  \brief deletes a monitored variable
  \param var is the key that identifies a monitored variable
  */
void
UniqLogger::delMonitorVar(const QString &var)
{
    muxMonitorVarMap.lock();
        m_VarMonitorMap.remove(var);
    muxMonitorVarMap.unlock();
}


/*!
  \brief this method changes the timestamp format for new Loggers
  \param aTimeFormat is a string containing the new timestamp fomat
  All loggers created after a call to this method will log with the new timestamp format, previously created ones will be unaffected
  \note See Qt::QDateTime for a list of valid timestamp formats
  */
void
UniqLogger::setTimeStampFormat(const QString &aTimeFormat)
{
    UniqLogger::gmuxUniqLoggerInstance.lock();
    {
        m_defaultTimeStampFormat = aTimeFormat;
    }
    UniqLogger::gmuxUniqLoggerInstance.unlock();
}
 


/*!
  \brief this method changes the encasing chars for new Loggers
  \param aStartChar is the starting encasing char. The default is '['
  \param aEndChar is the ending encasing char. The default is ']'
  All loggers created after a call to this method will log with the new encasing chars, previously created ones will be unaffected
  */
void
UniqLogger::setEncasingChars(const QChar &aStartChar, const QChar &aEndChar)
{
    UniqLogger::gmuxUniqLoggerInstance.lock();
    {
        m_defaultStartEncasingChar = aStartChar;
        m_defaultEndEncasingChar = aEndChar;
    }
    UniqLogger::gmuxUniqLoggerInstance.unlock();
}
 


/*!
  \brief this method changes the spacing char for new Loggers
  \param aSpaceChar is the new spacing char. The default is ' ';
  All loggers created after a call to this method will log with the new spacing char, previously created ones will be unaffected
  */
void
UniqLogger::setSpacingChar(const QChar &aSpaceChar)
{
    UniqLogger::gmuxUniqLoggerInstance.lock();
    {
        m_defaultSpaceChar = aSpaceChar;
    }
    UniqLogger::gmuxUniqLoggerInstance.unlock();
}



/*!
 * \brief UniqLogger::flushAllWriters This function will flush all writers in order to make them write whatever they might be
 * holding in their internal memory. It's primary function is to be called by a signal handler in case of catastrophic crashes
 * to dump (if possible) every logging statement gathered so far.
 */
void
UniqLogger::flushAllWriters()
{
    LogWriter *lw;

    muxDeviceCounter.lock();
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        lw->flush();
    }
    muxDeviceCounter.unlock();
}


/*!
 * \brief UniqLogger::purgeUnusedWriters Explicitly removes and schedules destruction of
 *        all dormant writers (refcount == 1, i.e., no attached Loggers) from m_DevicesMap.
 * \note We call deactivateWriter() synchronously (so the underlying file/device is
 *       already flushed and closed before returning) and then use deleteLater() instead
 *       of direct delete because each LogWriter lives on a NRThreadPool worker thread
 *       and owns a child QTimer (m_pLogTimer) that must be destroyed on its owning thread.
 */
void
UniqLogger::purgeUnusedWriters()
{
    QMutexLocker ml(&muxDeviceCounter);
    QList<LogWriter*> toRemove;
    for (auto it = m_DevicesMap.begin(); it != m_DevicesMap.end(); ++it) {
        if (it.value() == 1) {
            toRemove.append(it.key());
        }
    }
    for (LogWriter *lw : toRemove) {
        m_DevicesMap.remove(lw);
        lw->deactivateWriter();
        lw->deleteLater();
    }
}
