/********************************************************************************
 *   Copyright (C) 2010-2018 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#include "ConsoleWriter.h"

#include <QStringList>
#include <QTime>
#include <iostream>

QMutex ConsoleWriter::m_consoleMux;

ConsoleWriter::ConsoleWriter(const WriterConfig &wc)
    : LogWriter(wc)
{}
 

/*!
  \brief In the class dtor we want to flush whatever we might have got that is not written
  */
ConsoleWriter::~ConsoleWriter()
{
    //on exit, write all we've got
    this->flush();
}


/*!
 * \brief Get the color code for this message, according to m_colorScheme.
 *
 * The returned string will be empty if no color was selected for this message level. <br/>
 * On Windows platforms this functions does nothing and returns an empty string.
 *
 * \param i_level The message level to be colored.
 * \return The color code (eg. '\033[0;37m') to write to the console.
 */
QString ConsoleWriter::getColorCode(const UNQL::LogMessagePriorityType &i_level)
{
    UNQL::ConsoleColorType color = m_colorScheme.getColorForLevel(i_level);
    if (color == UNQL::NO_COLOR)
    {
        return QString();
    }

    return QString("\033[22;%1m").arg((int) color);
}


/*!
  \brief writes the messages in the queue on the console with the specified color
  */
void
ConsoleWriter::writeToDevice()
{
    mutex.lock();
        int msgcount = m_logMessageList.count();
        for (int i=0; i<msgcount; i++) {
            LogMessage log = m_logMessageList.takeFirst();
            write(log);
        }
    mutex.unlock();
}
 
/*!
 * \internal
 * \brief write to console
 * \param log message to write
 */
void
ConsoleWriter::write(const LogMessage &log)
{
    ConsoleWriter::m_consoleMux.lock();

#if !defined(WIN32) && !defined(Q_OS_IOS)
    std::cerr << getColorCode(log).toLatin1().constData();
#endif
    std::cerr << log.message().toLatin1().constData();

    //windows console does not support color codes
#if !defined(WIN32) && !defined(Q_OS_IOS)
    std::cerr << "\033[0m";
#endif
    std::cerr << std::endl;

    ConsoleWriter::m_consoleMux.unlock();
}
