/********************************************************************************
 *   Copyright (C) 2010-2012 by NetResults S.r.l. ( http://www.netresults.it )  *
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

#include "FileWriter.h"
#include "ConsoleWriter.h"
#include "Logger.h"
#include "DummyWriter.h"
 
//Define (and not simply declare) the static members (see C++ FAQ 10.12)
QMutex UniqLogger::gmuxUniqLoggerInstance;
QMap<QString,UniqLogger*> UniqLogger::gUniqLoggerInstanceMap;

extern QMap<UNQL::LogMessagePriorityType,QString> UnqlPriorityLevelNamesMap;
#include <QCoreApplication>
/*!
  \brief this is the class ctor, it is protected since we want just the singleton instance
  */
UniqLogger::UniqLogger()
{
    qDebug() << Q_FUNC_INFO;
    m_defaultTimeStampFormat="hh:mm:ss";
    m_defaultSpaceChar=' ';
    m_defaultStartEncasingChar='[';
    m_defaultEndEncasingChar=']';


    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_FATAL,"FATAL");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_CRITICAL,"CRITICAL");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_WARNING,"WARNING");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_INFO,"INFO");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_DBG,"DEBUG");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_DBG_ALL,"FULL DEBUG");
    UnqlPriorityLevelNamesMap.insert(UNQL::LOG_MONITOR,"MONITOR");

    m_ConsoleLogger = new ConsoleWriter();
    registerWriter(m_ConsoleLogger);
    m_ConsoleLogger->start();
    qDebug() << "Being here with app: " << QCoreApplication::instance();
}
 
UniqLogger::~UniqLogger()
{

}

/*!
  \brief this returns the singleton instance of the UniqLogger library
  */
UniqLogger*
UniqLogger::instance(const QString &ulname)
{
	UniqLogger *ulptr;
    static UniqLogger instance;

	UniqLogger::gmuxUniqLoggerInstance.lock();
	if (gUniqLoggerInstanceMap.contains(ulname))
		ulptr=gUniqLoggerInstanceMap[ulname];
	else {
		if(gUniqLoggerInstanceMap.count()==0) {
			ulptr = &instance;
		}
		else {
			ulptr = new UniqLogger;
		}
		gUniqLoggerInstanceMap.insert(ulname,ulptr);
	}
	UniqLogger::gmuxUniqLoggerInstance.unlock();

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
#ifdef ULOGDBG
    qDebug() << Q_FUNC_INFO << "lw: " << lw;
#endif
    muxDeviceCounter.lock();
    if (m_DevicesMap.contains(lw)) {
        int refcount = m_DevicesMap[lw];
        if (refcount==1) {
            m_DevicesMap.remove(lw);
            delete lw;
        }
        else {
            m_DevicesMap[lw] = refcount-1;
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

#ifdef ULOGDBG
    qDebug() << Q_FUNC_INFO << "received a call to delete writers from finished logger " << sender();
    qDebug() << "about to delete writers: " << aList;
#endif
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

        if (m_defaultTimeStampFormat!="hh:mm:ss")
            l->setTimeStampFormat(m_defaultTimeStampFormat);
        if (m_defaultSpaceChar!=' ')
            l->setSpacingString(m_defaultSpaceChar);
        if (m_defaultStartEncasingChar!='[')
            l->setEncasingChars(m_defaultStartEncasingChar, m_defaultEndEncasingChar);

        //after having set the various strings we set the module name
        l->setModuleName(logname);

        bool b = connect(l,SIGNAL(writersToDelete(const QList<LogWriter*>)),this,SLOT(writerFinished(const QList<LogWriter*>)),Qt::DirectConnection);
        Q_ASSERT(b);
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

/*!
  \brief creates a logger and automatically connects a file writer with default values
  \param _logname the module name for this logger
  \param _filename the filename where this logger will write messages by default
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
Logger*
UniqLogger::createFileLogger(const QString & _logname, const QString &_filename, const WriterConfig &i_wconf)
{
	Logger *l = createLogger(_logname);
    LogWriter &fw = getFileWriter(_filename);
    fw.setWriterConfig(i_wconf);
	this->addWriterToLogger(l,fw);
	return l;
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
	this->addWriterToLogger(l,rlog);
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
UniqLogger::createDbLogger(const QString & _logname, const QString &aDbFileName)
{
	Logger *l = createLogger(_logname);
	LogWriter const &rlog = getDbWriter(aDbFileName);
	this->addWriterToLogger(l,rlog);
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
	this->addWriterToLogger(l,clog);
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
  \param _filename the filename where this logger will write messages by default
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
LogWriter &UniqLogger::getFileWriter(const QString &_filename)
{

    FileWriter *fw;
    LogWriter *lw;

	muxDeviceCounter.lock();
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        fw = dynamic_cast<FileWriter*>(lw);
        if (fw && fw->getBaseName() == _filename) {
#ifdef ULOGDBG
            qDebug() << "Existing Filewriter found! ------------------->><<-------" << fw;
#endif
			muxDeviceCounter.unlock();
			return *fw;
        }
        fw = 0;
    }
    muxDeviceCounter.unlock();
    fw = new FileWriter();
#ifdef ULOGDBG
    qDebug() << "Filewriter for file "<< _filename <<" not found, creating one " << fw;
#endif
    registerWriter(fw);
    fw->setOutputFile(_filename);
    fw->start();

	return *fw;
}

#ifdef ULOG_DBLOGGING
/*!
  \brief returns a file writer that can be added to other loggers
  \param _filename the filename where this logger will write messages by default
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
const LogWriter&
UniqLogger::getDbWriter(const QString &_filename)
{

    DbWriter *fw;
    LogWriter *lw;

	muxDeviceCounter.lock();
    LogWriterUsageMapType::Iterator it;
    for (it = m_DevicesMap.begin(); it != m_DevicesMap.end(); it++) {
        lw = it.key();
        fw = dynamic_cast<DbWriter*>(lw);
        if (fw && fw->getBaseName() == _filename) {
#ifdef ULOGDBG
            qDebug() << "Existing DbWriter found! ------------------->><<-------" << fw;
#endif
			muxDeviceCounter.unlock();
			return *fw;
        }
        fw = 0;
    }
    muxDeviceCounter.unlock();
    fw = new DbWriter(_filename);
#ifdef ULOGDBG
    qDebug() << "Dbwriter for file "<< _filename <<" not found, creating one " << fw;
#endif
    registerWriter(fw);
    //fw->setOutputFile(_filename);
    fw->start();

	return *fw;
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
#ifdef ULOGDBG
			qDebug() << "Existing Networkwriter found! ------------------->><<-------";
#endif
			muxDeviceCounter.unlock();
			return *rw;
        }
    }
	muxDeviceCounter.unlock();

    rw = new RemoteWriter(_ha,_port);
    if (rw)
    {
        registerWriter(rw);
        rw->start();
    }
    return *rw;
}
#endif

/*!
  \brief returns the standard console writer
  \return the pointer to the standard console logger
  */
LogWriter &UniqLogger::getStdConsoleWriter()
{ return *m_ConsoleLogger; }


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
    cw->start();
	return *cw;
}

LogWriter&
UniqLogger::getDummyWriter()
{
    DummyWriter* dw = new DummyWriter();
    registerWriter(dw);
    dw->start();
    return *dw;
}

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

    this->unregisterWriter(const_cast<LogWriter*>(&writer));
	res = _l->removeLogDevice(const_cast<LogWriter*>(&writer));
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
    this->registerWriter(const_cast<LogWriter*>(&writer));
	res = _l->addLogDevice(const_cast<LogWriter*>(&writer));
	return res;
}

/*!
  \brief changes the monitor status of a monitored variable
  \param var is the key that identified a monitored variable
  \param status the new monitor status of the variable
  */
void
UniqLogger::monitorVar(const QString &var, bool status)
{
	muxMonitorVarMap.lock();
	if (m_VarMonitorMap.contains(var)) {
		m_VarMonitorMap[var]=status;
    }
    else {
    #ifdef ULOGDBG
        qDebug() << "The var is not monitored " << var;
	#endif
	}
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
    m_defaultTimeStampFormat = aTimeFormat;
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
    m_defaultStartEncasingChar = aStartChar;
    m_defaultEndEncasingChar = aEndChar;
}
 
/*!
  \brief this method changes the spacing char for new Loggers
  \param aSpaceChar is the new spacing char. The default is ' ';
  All loggers created after a call to this method will log with the new spacing char, previously created ones will be unaffected
  */
void
UniqLogger::setSpacingChar(const QChar &aSpaceChar)
{
    m_defaultSpaceChar = aSpaceChar;
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
    m_ConsoleLogger->setConsoleColor(c);
}
