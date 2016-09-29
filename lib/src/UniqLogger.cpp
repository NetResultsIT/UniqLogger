/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#include "UniqLogger.h"

#ifdef ULOG_NETLOGGING
 #include "RemoteWriter.h"
#endif

#ifdef ULOG_DBLOGGING
 #include "DbWriter.h"
#endif

#ifdef ULOG_ANDROIDLOGGING
 #include "AndroidWriter.h"
#endif

#include "FileWriter.h"
#include "ConsoleWriter.h"
#include "Logger.h"
#include "DummyWriter.h"
 
#include "nrthreadpool.h"
#include <stdexcept>


//Define (and not simply declare) the static members (see C++ FAQ 10.12)
QMutex UniqLogger::gmuxUniqLoggerInstance;
QMap<QString,UniqLogger*> UniqLogger::gUniqLoggerInstanceMap;

extern QMap<UNQL::LogMessagePriorityType,QString> UnqlPriorityLevelNamesMap;
#include <QCoreApplication>
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

    m_ConsoleLogger = new ConsoleWriter();
    registerWriter(m_ConsoleLogger);
    m_pTPool->runObject(m_ConsoleLogger);
    m_ConsoleLogger->run();

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
        qWarning() << "The UniqLogger instances was already configured with a different number of logging threads: " << ulptr->threadsUsedForLogging();
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
        m_DevicesMap[lw] = refcount+1;
    }
    else {
        m_DevicesMap.insert(lw,0);
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
            m_DevicesMap[lw] = refcount-1;
            Q_ASSERT(refcount >= 0);
        }
    }
    else {
        qDebug() << " +++++++++++++++ ERROR  deregistering writer +++++++++++++";
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


/*!
  \brief returns a generic Logger
  \param logname the module name for this logger
  \return a reference to the logger class created
  \note this logger does not have a default writer device, you should manually add one in order to see its output
  */
Logger*
UniqLogger::createLogger(const QString &logname)
{
	Logger *l = new Logger();					//we need the friendship here
    if (l) {
		l->muxMonitorVar = &muxMonitorVarMap;	//we need the friendship here
		l->m_varMonitorMap = &m_VarMonitorMap;	//we need the friendship here

        if (m_defaultTimeStampFormat != DEF_UNQL_TS_FMT)
            l->setTimeStampFormat(m_defaultTimeStampFormat);
        if (m_defaultSpaceChar != ' ')
            l->setSpacingString(m_defaultSpaceChar);
        if (m_defaultStartEncasingChar != '[')
            l->setEncasingChars(m_defaultStartEncasingChar, m_defaultEndEncasingChar);

        //after having set the various strings we set the module name
        l->setModuleName(logname);

        bool b = connect(l,SIGNAL(writersToDelete(const QList<LogWriter*>)), this, SLOT(writerFinished(const QList<LogWriter*>)), Qt::DirectConnection);
        Q_ASSERT(b);
        Q_UNUSED(b);
    }
	return l;
}



/*!
 * \brief creates a dummy logger: it will drop all the data sent to the logger
 *  you could consider as redirecting logs to /dev/null
 * \param _logname  the modulename for this logger
 * \return the pointer to the logger class created or a null pointer if something went wrong
 */
Logger*
UniqLogger::createDummyLogger( const QString& _logname, const WriterConfig &i_wconf)
{
    Logger *l = createLogger(_logname);
    LogWriter &dw = getDummyWriter();
    dw.setWriterConfig(i_wconf);
    this->addWriterToLogger(l, dw);
    return l;
}

#ifdef ULOG_ANDROIDLOGGING
/*!
 * \brief creates an Android native logger and automatically connects a file writer with default values
 * \param _logname the module name for this logger
 * \return the pointer to the logger class created or a null pointer if something went wrong
 */
Logger *UniqLogger::createAndroidLogger(const QString& _logname)
{
    Logger *l = createLogger(_logname);
    LogWriter &dw = getAndroidWriter();
    this->addWriterToLogger(l, dw);
    return l;
}
#endif


/*!
  \brief creates a logger and automatically connects a file writer with default values
  \param _logname the module name for this logger
  \param _filename the filename where this logger will write messages by default
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
Logger*
UniqLogger::createFileLogger(const QString & _logname, const QString &_filename, const WriterConfig &i_wconf)
{
    try
    {
        Logger *l = createLogger(_logname);
        LogWriter &fw = getFileWriter(_filename, i_wconf.rotationPolicy);
        fw.setWriterConfig(i_wconf);
        this->addWriterToLogger(l, fw);
        return l;
    }catch (std::exception &except)
    {
        ULDBG << except.what();
        return NULL;
    }
}
 


#ifdef ULOG_NETLOGGING
/*!
  \brief creates a logger and automatically connects a network writer with default values
  \param _logname the module name for this logger
  \param _ha the address where this logger will try to connect to write messages
  \param _port the server port where this logger will try to connect to write messages
  \return a reference to the logger class created
  */
Logger*
UniqLogger::createNetworkLogger(const QString & _logname, const QString &_ha, int _port, const WriterConfig &i_wconf)
{
	Logger *l = createLogger(_logname);
    LogWriter &rlog = getNetworkWriter(_ha,_port);
    rlog.setWriterConfig(i_wconf);
    this->addWriterToLogger(l, rlog);
    return l;
}
#endif 
 
 
#ifdef ULOG_DBLOGGING
/*!
  \brief creates a logger and automatically connects a DB writer with default values
  \param _logname the module name for this logger
  \param aDbFileName the name of the file that continas DB data
  \return a reference to the logger class created
  */
Logger*
UniqLogger::createDbLogger(const QString & _logname, const QString &aDbFileName, const WriterConfig &)
{
	Logger *l = createLogger(_logname);
	LogWriter const &rlog = getDbWriter(aDbFileName);
    this->addWriterToLogger(l, rlog);
    return l;
}
#endif
 


/*!
  \brief creates a logger and automatically connects a console writer with default values
  \param _logname the module name for this logger
  \param c the color in which this logger will going to write messages with
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
Logger*
UniqLogger::createConsoleLogger(const QString &_logname, ConsoleColorType c, const WriterConfig &wc)
{
	Logger *l = createLogger(_logname);
    LogWriter &clog = getConsoleWriter(c);
    clog.setWriterConfig(wc);
    this->addWriterToLogger(l, clog);
    return l;
}



/*!
  \brief creates a logger and automatically connects a console writer with default values
  \param _logname the module name for this logger
  \param useStdConsoleLogger chooses whether we should use the existing console logger (with its color and timing) or if
  we should allocate a new thread where we can specify our desires. (NOTE: the default value is to reuse the existing Console)
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
Logger*
UniqLogger::createConsoleLogger(const QString &_logname, bool useStdConsoleLogger, const WriterConfig &wc)
{
    Logger *l = createLogger(_logname);
    if (useStdConsoleLogger) {
        LogWriter &cl = *m_ConsoleLogger;
        cl.setWriterConfig(wc);
        this->addWriterToLogger(l,cl);
    }
    else {
        LogWriter &cl = getConsoleWriter();
        cl.setWriterConfig(wc);
        this->addWriterToLogger(l,cl);
    }
    return l;
}



/*!
  \brief returns a file writer that can be added to other loggers
        If it fails it throw an exception
  \param _filename the filename where this logger will write messages by default
  \param i_rotationPolicy the file rotation policy (by default StrictRotation is used)
  \return the reference to the logger class created
  */
LogWriter &UniqLogger::getFileWriter(const QString &_filename, FileRotationPolicyType i_rotationPolicy)
{

    FileWriter *fw;
    LogWriter *lw;

	muxDeviceCounter.lock();
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        fw = dynamic_cast<FileWriter*>(lw);
        if (fw && fw->getBaseName() == _filename) {
            if ( fw->getRotationPolicy() != i_rotationPolicy )
            {
                muxDeviceCounter.unlock();
                throw std::runtime_error("Requested an existing FileWriter with same filename but different rotation policy!");
            }
            ULDBG << "Existing Filewriter found! ------------------->><<-------" << fw;
			muxDeviceCounter.unlock();
			return *fw;
        }
        fw = 0;
    }
    muxDeviceCounter.unlock();
    fw = new FileWriter(i_rotationPolicy);

    ULDBG << "Filewriter for file "<< _filename <<" not found, creating one " << fw;

    registerWriter(fw);
    fw->setOutputFile(_filename);

    m_pTPool->runObject(fw);
    fw->run();

	return *fw;
}



#ifdef ULOG_DBLOGGING
/*!
  \brief returns a file writer that can be added to other loggers
  \param _filename the filename where this logger will write messages by default
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
LogWriter&
UniqLogger::getDbWriter(const QString &_filename)
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

			muxDeviceCounter.unlock();
            return *dw;
        }
        dw = 0;
    }
    muxDeviceCounter.unlock();
    dw = new DbWriter(_filename);

    ULDBG << "Dbwriter for file "<< _filename <<" not found, creating one " << dw;

    registerWriter(dw);

    m_pTPool->runObject(dw);
    dw->run();

    return *dw;
}
#endif



#ifdef ULOG_NETLOGGING
/*!
  \brief creates a logger and automatically connects a network writer with default values
  \param _ha the address where this logger will try to connect to write messages
  \param _port the server port where this logger will try to connect to write messages
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
LogWriter &UniqLogger::getNetworkWriter(const QString & _ha, int _port)
{
    RemoteWriter *rw = NULL;
    LogWriter *lw = NULL;

	muxDeviceCounter.lock();
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        rw = dynamic_cast<RemoteWriter*>(lw);
        if (rw && rw->getHost() == _ha && rw->getPort() == _port) {

            ULDBG << "Existing Networkwriter found! ------------------->><<-------";

			muxDeviceCounter.unlock();
			return *rw;
        }
    }
	muxDeviceCounter.unlock();

    rw = new RemoteWriter(_ha,_port);
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
  \brief returns the standard console writer
  \return the pointer to the standard console logger
  */
LogWriter &UniqLogger::getStdConsoleWriter()
{
    return *m_ConsoleLogger;

}



/*!
  \brief creates a console writer
  \param c the color in which this logger will going to write messages
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
LogWriter &UniqLogger::getConsoleWriter(ConsoleColorType c)
{
    ConsoleWriter *cw = new ConsoleWriter();
    registerWriter(cw);
    cw->setConsoleColor(c);
    //cw->start();
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
    //dw->start();
    return *dw;
}


#ifdef ULOG_ANDROIDLOGGING
LogWriter &UniqLogger::getAndroidWriter()
{

    AndroidWriter* aw = new AndroidWriter();
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
    \brief this method will change the color the standard console writer will log with
    \param c the color that will be used from now on from the standard console logger
    \note this method will not affect other console logger obtained via getConsoleLogger() specifying a specific color
but will indeed change those that were obtained passing true (default value) to the same getConsoleLogger method;
  */
void
UniqLogger::setStdConsoleColor(ConsoleColorType c)
{
    UniqLogger::gmuxUniqLoggerInstance.lock();
    {
        m_ConsoleLogger->setConsoleColor(c);
    }
    UniqLogger::gmuxUniqLoggerInstance.unlock();
}
