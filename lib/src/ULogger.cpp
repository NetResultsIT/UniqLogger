/********************************************************************************
 *   Copyright (C) 2009-2010 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):									*
 *		Francesco Lamonica	<f.lamonica@netresults.it>		*
 ********************************************************************************/

#include "ULogger.h"
 
UniqLogger::UniqLogger()
{
    gLoggerCounter=0;
    m_ConsoleLogger = new ConsoleWriter();
    registerWriter(m_ConsoleLogger);
    m_ConsoleLogger->start();
}
 
UniqLogger::~UniqLogger()
{

}
 
/*!
  \brief this returns the singleton instance of the Unilog library
  */
UniqLogger*
UniqLogger::instance()
{
    static UniqLogger instance;

    return &instance;
}
 
/*!
  \brief register a logger in the internal list
  */
void
UniqLogger::registerLogger(Logger*)
{

}

/*!
  \brief register a logwriter in the internal list
  */
void
UniqLogger::registerWriter(LogWriter *lw)
{
    gMuxDeviceCounter.lock();
    m_DevicesMap.insert(gLoggerCounter,lw);
    gLoggerCounter++;
    gMuxDeviceCounter.unlock();
}

/*!
  \brief returns a generic Logger
  \param logname the module name for this logger
  \return the pointer to the logger class created or a null pointer if something went wrong
  */
Logger*
UniqLogger::createLogger(QString logname)
{
    Logger *l = new Logger();
    if (l) {
        l->setModuleName(logname);
    }
    return l;
}
 
/*!
  \brief creates a logger and automatically connects a file writer with default values
  \param _logname the module name for this logger
  \param _filename the filename where this logger will write messages by default
  \return the pointer to the logger class created or a null pointer if somethinf went wrong
  */
Logger*
UniqLogger::createFileLogger(QString _logname, QString _filename)
{
    Logger *l = createLogger(_logname);
    LogWriter *fw = getFileWriter(_filename);
    l->addLogDevice(fw);
    fw->start();
    return l;
}
 
/*!
  \brief creates a logger and automatically connects a network writer with default values
  \param _logname the module name for this logger
  \param _ha the address where this logger will try to connect to write messages
  \param _port the server port where this logger will try to connect to write messages
  \return the pointer to the logger class created or a null pointer if somethinf went wrong
  */
Logger*
UniqLogger::createNetworkLogger(QString _logname, QString _ha, int _port)
{
    Logger *l = createLogger(_logname);
    LogWriter *rlog = getNetworkWriter(_ha,_port);

    if (rlog)
            addWriterToLogger(l,rlog);
    else {
            delete l;
            l=0;
    }

    return l;
}
 
/*!
  */
Logger*
UniqLogger::createConsoleLogger(QString _logname, ConsoleColorType c)
{
    Logger *l = createLogger(_logname);
    LogWriter *clog = getConsoleWriter(c);

    if (clog)
            addWriterToLogger(l,clog);
    else {
            delete l;
            l=0;
    }

    return l;
}

/*!
  \brief returns a file writer that can be added to other loggers
  \param _filename the filename where this logger will write messages by default
  \return the pointer to the logger class created or a null pointer if somethinf went wrong
  */
LogWriter*
UniqLogger::getFileWriter(QString _filename)
{
    FileWriter *fw = new FileWriter();
    fw->setOutputFile(_filename);
    fw->start();
    return fw;
}

/*!
  \brief creates a logger and automatically connects a network writer with default values
  \param _ha the address where this logger will try to connect to write messages
  \param _port the server port where this logger will try to connect to write messages
  \return the pointer to the logger class created or a null pointer if somethinf went wrong
  */
LogWriter*
UniqLogger::getNetworkWriter(QString _ha, int _port)
{
    RemoteWriter *rlog = new RemoteWriter();
    QHostAddress ha(_ha);
    int result = rlog->connectToServer(ha,_port);
    if (result<0) {
        delete rlog;
        rlog = 0;
    }
    else {
        rlog->start();
    }
    return rlog;
}

/*!
  */
LogWriter*
UniqLogger::getConsoleWriter(ConsoleColorType c)
{
    ConsoleWriter *cw = new ConsoleWriter();
    registerWriter(cw);
    cw->setConsoleColor(c);
    cw->start();
    return cw;
}


/*!
  */
int
UniqLogger::removeWriterFromLogger(Logger* _l, LogWriter *writer)
{
    int res = 0;
    res = _l->removeLogDevice(writer);
    return res;
}
 
/*!
  */
int
UniqLogger::addWriterToLogger(Logger* _l, LogWriter *writer)
{
    int res = 0;
    res = _l->addLogDevice(writer);
    return res;
}
 

