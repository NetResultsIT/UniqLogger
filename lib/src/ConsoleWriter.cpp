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

    if (m_Config.compressMessages)
    {
        writeCompressedMessages();
    }
    else
    {
        writeUncompressedMessages();
    }

    mutex.unlock();
}
 
/*!
 * \internal
 * \brief Write messages uncompressed, each message on a single line.
 */
void
ConsoleWriter::writeUncompressedMessages()
{
    int msgcount = m_logMessageList.count();
    for (int i=0; i<msgcount; i++) {
        LogMessage log = m_logMessageList.takeFirst();
        write(log.message(), log.level());
    }
}

/*!
 * \internal
 * \brief Write messages in compressed way, if there are multiple
 *        messages with the same body, write all in a unique line.
 */
void
ConsoleWriter::writeCompressedMessages()
{
    int nummsg = m_logMessageList.count();
    int i = 0;
    int j = 1;

    if (nummsg == 1)
    {
        //just one element, write it
        LogMessage log = m_logMessageList.takeFirst();
        write(log.message(), log.level());
    }
    else
    {
        QString endTstamp = "";
        int counter = 1; //number of susbsequent messages
        while (j < nummsg)
        {
            if ((m_logMessageList.at(i).rawMessage() == m_logMessageList.at(j).rawMessage()) &&
                (m_logMessageList.at(i).level() == m_logMessageList.at(j).level()))
            {
                //subsequent messages, save end timestamp and look at next element
                endTstamp = m_logMessageList.at(j).tstamp();
                ++j;
                ++counter;
            }
            else
            {
                QString m;
                if (counter <= 1)
                {
                    //No subsequent messages, just write the message
                    m = m_logMessageList.at(i).message();
                }
                else
                {
                    //End of subsequent messages, write all as unique string
                    m = m_logMessageList.at(i).message(m_logMessageList.at(i).tstamp(), endTstamp, counter);
                }

                write(m, m_logMessageList.at(i).level());
                i = j;
                ++j;
                counter = 1;
                endTstamp = "";
            }

            if (j == nummsg)
            {
                //Don't want to skip last element
                QString m;
                if (counter <= 1)
                {
                    m = m_logMessageList.at(i).message();
                }
                else
                {
                    m = m_logMessageList.at(i).message(m_logMessageList.at(i).tstamp(), endTstamp, counter);
                }

                write(m, m_logMessageList.at(i).level());
            }
        }
    }

    m_logMessageList.clear();
}

/*!
 * \internal
 * \brief write to console
 * \param i_msg message to write
 * \param i_level priority level of the message
 */
void
ConsoleWriter::write(const QString& i_msg,
                     const UNQL::LogMessagePriorityType &i_level)
{
    ConsoleWriter::m_consoleMux.lock();
#if !defined(WIN32) && !defined(Q_OS_IOS)
    std::cerr << getColorCode( i_level).toLatin1().constData();
#endif
    std::cerr << i_msg.toLatin1().constData();

    //windows console does not support color codes
#if !defined(WIN32) && !defined(Q_OS_IOS)
    std::cerr << "\033[0m";
#endif
    std::cerr << std::endl;
    ConsoleWriter::m_consoleMux.unlock();
}
