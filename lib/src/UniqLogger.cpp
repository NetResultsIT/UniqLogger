/********************************************************************************
 *   Copyright (C) 2010-2018 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#include "UniqLogger.h"

#ifdef ENABLE_UNQL_NETLOG
 #include "RemoteWriter.h"
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
{
    m_defaultTimeStampFormat = DEF_UNQL_TS_FMT;
    m_defaultSpaceChar = ' ';
    m_defaultStartEncasingChar = '[';
    m_defaultEndEncasingChar = ']';

    m_pTPool = new NRThreadPool(nthreads, "UNQL_WPool", this);

    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_FATAL,   "FATAL");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_CRITICAL,"CRITICAL");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_WARNING, "WARNING");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_INFO,    "INFO");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_DBG,     "DEBUG");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_DBG_ALL, "FULL DEBUG");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_MONITOR, "MONITOR");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_FORCED,  "FORCED");

    ULDBG << "Being here with app: " << QCoreApplication::instance();
}
 


UniqLogger::~UniqLogger()
{
    ULDBG << Q_FUNC_INFO;
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
  \brief register a logwriter in the internal list
  */
void
UniqLogger::registerWriter(LogWriter *lw)
{
    muxDeviceCounter.lock();
    if (m_DevicesMap.contains(lw)) {
        int refcount = m_DevicesMap[lw];
        m_DevicesMap[lw] = refcount + 1;
    }
    else {
        m_DevicesMap.insert(lw, 0);
    }
    muxDeviceCounter.unlock();
}



/*!
  \brief unregister a logwriter in the internal list
  */
void
UniqLogger::unregisterWriter(LogWriter *lw)
{
    ULDBG << Q_FUNC_INFO << "lw: " << lw;

    muxDeviceCounter.lock();
    if (m_DevicesMap.contains(lw)) {
        int refcount = m_DevicesMap[lw];
        if (refcount == 1) {
            m_DevicesMap.remove(lw);
            lw->deleteLater();
        }
        else {
            m_DevicesMap[lw] = refcount - 1;
            Q_ASSERT(refcount >= 0);
        }
    }
    else {
        ULDBG << " +++++++++++++++ ERROR  deregistering writer +++++++++++++";
    }
    muxDeviceCounter.unlock();
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

        if (m_defaultTimeStampFormat != DEF_UNQL_TS_FMT)
            l->setTimeStampFormat(m_defaultTimeStampFormat);
        if (m_defaultSpaceChar != ' ')
            l->setSpacingString(m_defaultSpaceChar);
        if (m_defaultStartEncasingChar != '[')
            l->setEncasingChars(m_defaultStartEncasingChar, m_defaultEndEncasingChar);

        //after having set the various strings we set the module name
        l->setModuleName(logname);

        bool b = connect(l, SIGNAL(writersToDelete(const QList<LogWriter*>)), this, SLOT(writerFinished(const QList<LogWriter*>)), Qt::DirectConnection);
        Q_ASSERT(b);
        Q_UNUSED(b);
    }
    return l;
}



/*!
 * \brief creates a dummy logger: it will drop all the data sent to the logger
 *  you could consider as redirecting logs to /dev/null
 * \param logname  the modulename for this logger
 * \return the pointer to the logger class created or a null pointer if something went wrong
 */
Logger*
UniqLogger::createDummyLogger( const QString& logname, const WriterConfig &)
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
        QString msg = QString("Logger %3 tried to open this writer with this configuration:\n%1\nbut it had been previously opened with this one:\n%2").arg(i_wconf.toString()).arg(wc2.toString()).arg(i_logname);
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
#endif 
 
 
#ifdef ENABLE_UNQL_DBLOG
/*!
  \brief creates a logger and automatically connects a DB writer with default values
  \param _logname the module name for this logger
  \param aDbFileName the name of the file that continas DB data
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
  \param i_rotationPolicy the file rotation policy (by default StrictRotation is used)
  \return the reference to the logger class created
  */
LogWriter &UniqLogger::getFileWriter(const QString &i_filename, const WriterConfig &wc, int &ok)
{
    FileWriter *fw;
    LogWriter *lw;

    muxDeviceCounter.lock();
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        fw = dynamic_cast<FileWriter*>(lw);
        if (fw && fw->getBaseName() == i_filename) {
            if ( !checkMatchingConfigForWriter(*fw, wc) )
            {
                ULDBG << "WriterConfigs are incompatible";
                ok = UNQL::UnqlErrorWriterConfigIncompatible;
            }
            ULDBG << "Existing Filewriter found! ------------------->><<-------" << fw;
            muxDeviceCounter.unlock();
            return *fw;
        }
        fw = nullptr;
    }
    muxDeviceCounter.unlock();
    fw = new FileWriter(wc);

    ULDBG << "Filewriter for file " << i_filename << " not found, creating one " << fw;

    registerWriter(fw);
    fw->setOutputFile(i_filename);

    m_pTPool->runObject(fw);
    fw->run();

    return *fw;
}



#ifdef ENABLE_UNQL_DBLOG
/*!
  \brief returns a file writer that can be added to other loggers
  \param _filename the filename where this logger will write messages by default
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
LogWriter&
UniqLogger::getDbWriter(const QString &_filename, const WriterConfig &wc, int &ok)
{
    DbWriter *dw;
    LogWriter *lw;

    muxDeviceCounter.lock();
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        dw = dynamic_cast<DbWriter*>(lw);
        if (dw && dw->getBaseName() == _filename) {
            ULDBG << "Existing DbWriter found! ------------------->><<-------" << dw;
            if ( !checkMatchingConfigForWriter(*dw, wc) )
            {
                ULDBG << "WriterConfigs are incompatible for DB writers";
                ok = UNQL::UnqlErrorWriterConfigIncompatible;
            }

            muxDeviceCounter.unlock();
            return *dw;
        }
        dw = 0;
    }
    muxDeviceCounter.unlock();
    dw = new DbWriter(_filename, wc);

    ULDBG << "Dbwriter for file "<< _filename <<" not found, creating one " << dw;

    registerWriter(dw);

    m_pTPool->runObject(dw);
    dw->run();

    return *dw;
}
#endif



#ifdef ENABLE_UNQL_NETLOG
/*!
  \brief creates a logger and automatically connects a network writer with default values
  \param _ha the address where this logger will try to connect to write messages
  \param _port the server port where this logger will try to connect to write messages
  \return a reference to the logwriter class created
  */
LogWriter &UniqLogger::getNetworkWriter(const QString & _ha, int _port, const WriterConfig &wc, int &ok)
{
    RemoteWriter *rw = nullptr;
    LogWriter *lw = nullptr;

    muxDeviceCounter.lock();
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        rw = dynamic_cast<RemoteWriter*>(lw);
        if (rw && rw->getHost() == _ha && rw->getPort() == _port) {
            ULDBG << "Existing Networkwriter found! ------------------->><<-------";
            if ( !checkMatchingConfigForWriter(*rw, wc) )
            {
                ULDBG << "WriterConfigs are incompatible for remote writers";
                ok = UNQL::UnqlErrorWriterConfigIncompatible;
            }
            muxDeviceCounter.unlock();
            return *rw;
        }
    }
    muxDeviceCounter.unlock();

    rw = new RemoteWriter(_ha, _port, wc);
    if (rw)
    {
        registerWriter(rw);
        m_pTPool->runObject(rw);
        rw->run();
    }
    return *rw;
}
#endif



/*!
  \brief creates a ConsoleWriter with the specified color scheme.

  If a console writer with the same color scheme and writer config exists it will be reused.

  \param i_colorScheme The color scheme which this logger will use to write messages.
  \return A reference to the LogWriter class.
  */
LogWriter &UniqLogger::getConsoleWriter(UNQL::ConsoleColorScheme i_colorScheme, const WriterConfig &wc, int &ok)
{
    LogWriter *lw;
    ConsoleWriter *cw;

    ok = UNQL::UnqlErrorNoError; //the search for ConsoleWriter cannot fail (we may create a new one if need be)

    muxDeviceCounter.lock();
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        cw = dynamic_cast<ConsoleWriter*>(lw);
        if (cw && cw->getColorScheme() == i_colorScheme) {
            if ( !checkMatchingConfigForWriter(*cw, wc) )
            {
                ULDBG << "WriterConfigs are incompatible for console writers";
                continue; //there could be another consolewriter with same color but compatible config
            }
            ULDBG << "Existing matching ConsoleWriter found! ------------------->><<-------" << cw;
            muxDeviceCounter.unlock();
            return *cw;
        }
        cw = nullptr;
    }
    muxDeviceCounter.unlock();

    cw = new ConsoleWriter(wc);
    registerWriter(cw);
    cw->setColorScheme(i_colorScheme);
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
    DummyWriter* dw = new DummyWriter();
    registerWriter(dw);
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
    AndroidWriter* aw = new AndroidWriter(wc);
    registerWriter(aw);
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
  */
int
UniqLogger::removeWriterFromLogger(const Logger* _l, const LogWriter& writer)
{
    int res = 0;
    Logger *lptr = const_cast<Logger*>(_l);

    this->unregisterWriter(const_cast<LogWriter*>(&writer));
    res = lptr->removeLogDevice(const_cast<LogWriter*>(&writer));

    LogWriter* lw = const_cast<LogWriter*>(&writer);
    LogMessage lm (DEF_UNQL_LOG_STR, UNQL::LOG_DBG, QString("Removing logger %1 to this writer").arg(lptr->getModuleName()), LogMessage::getCurrentTstampString());
    lw->appendMessage(lm);
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
    int res = 0;
    Logger *lptr = const_cast<Logger*>(_l);

    res = lptr->addLogDevice(const_cast<LogWriter*>(&writer));
    if (res == 0) {
        this->registerWriter(const_cast<LogWriter*>(&writer));
    }

    LogWriter* lw = const_cast<LogWriter*>(&writer);
    LogMessage lm (DEF_UNQL_LOG_STR, UNQL::LOG_DBG, QString("Adding logger %1 to this writer").arg(lptr->getModuleName()), LogMessage::getCurrentTstampString());
    lw->appendMessage(lm);
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
