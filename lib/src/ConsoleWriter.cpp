/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#include "ConsoleWriter.h"

#include <QStringList>
#include <QTime>

QMutex ConsoleWriter::m_consoleMux;

ConsoleWriter::ConsoleWriter()
: LogWriter()
{
    //qDebug() << this << " Console writer ctor ";
    m_color = NONE; //default value
    m_sleepingMilliSecs = 500; //on console we're more aggressive
    //qDebug() << this << " end of console write ctor " << m_color;
}
 
/*!
  \brief In the class dtor we want to flush whatever we might have got that is not written
  */
ConsoleWriter::~ConsoleWriter()
{
    //on exit, write all we've got
    this->flush();
}

/*!
  \brief writes the messages in the queue on the console with the specified color
  */
void
ConsoleWriter::writeToDevice()
{
    QString s;
    if (!m_logIsPaused)
    {
        mutex.lock();
        int msgcount = m_logMessageList.count();
        for (int i=0; i<msgcount; i++) {
            s = m_logMessageList.takeFirst().message();
            //qDebug() << this <<  " the color code is " << m_color;
            QString colorcode;

            ConsoleWriter::m_consoleMux.lock();
            //windows console does not support color codes
#ifndef WIN32
            if (m_color != NONE) {
                colorcode = "\033[22;" + QString::number((int)m_color) + "m";
                std::cerr << colorcode.toLatin1().constData();
            }
#endif
            std::cerr << s.toLatin1().constData() << std::endl;

            //windows console does not support color codes
#ifndef WIN32
            if (m_color != NONE) {
                colorcode = "\033[0m";
                std::cerr << colorcode.toLatin1().constData();
            }
#endif            
            ConsoleWriter::m_consoleMux.unlock();
        }
        mutex.unlock();
    }
}
 

