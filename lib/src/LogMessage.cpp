/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#include "LogMessage.h"

#include <QDateTime>

QMap<UNQL::LogMessagePriorityType,QString> UnqlPriorityLevelNamesMap;

LogMessage::LogMessage(const QString &aLoggerName, UNQL::LogMessagePriorityType level,
                       const QString &aMsg, const QString &aInitTstamp, const QString &aEndTstamp, unsigned int repetitions)
    : m_msg(aMsg),
      m_loggerName(aLoggerName),
      m_level(level),
      m_initTstamp(aInitTstamp),
      m_endTstamp(aEndTstamp),
    m_logTag("UNQL2")
{
    (repetitions < 1) ? (m_repetitions = 1) : (m_repetitions = repetitions);
}

/*!
 * \internal
 * \brief Get the message string
 * \return The message string; this method builds
 *         a repeated message string or a single message
 *         string based on m_repetitions value
 */
QString
LogMessage::message() const
{
    if (m_repetitions <= 1)
        return singleMessage();
    else
        return repeatedMessage();
}


/*!
 * \brief LogMessage::getTstampString is a static function that will return a string with current timestamp using the default timeformat
 * \return a string containing current timestamp
 */
QString LogMessage::getCurrentTstampString()
{
    return QDateTime::currentDateTime().toString(DEF_UNQL_TSTAMP_FMT);
}

/*!
 * \internal
 * \brief Get the message string in case of multiple subsequential messages
 * \return The message string as a single line
 */
QString LogMessage::repeatedMessage() const
{
    QString msg,spc,n;
    QChar sc,ec;

    sc = m_formatting.startEncasingChar();
    ec = m_formatting.endEncasingChar();
    spc = m_formatting.spacingString();
    n = "repeated " + QString::number(m_repetitions) + " times";

    QString priolev = "UNKNOWN";
    if (UnqlPriorityLevelNamesMap.contains(m_level))
        priolev = UnqlPriorityLevelNamesMap[m_level];

    //We have 4 start encasing char and 4 end encasing char (3*2) and 7 space (m_formatting.spacingSize() * 7)
    msg.reserve(4*2 + (m_formatting.spacingSize() * 7) + m_initTstamp.size() + m_endTstamp.size()
                + m_loggerName.size() + m_msg.size() + priolev.size() + n.size());

    msg = sc + m_initTstamp + spc + "-" + spc + m_endTstamp + ec + spc
          + sc + m_loggerName + ec + spc + sc + priolev + ec + spc + m_msg + spc + sc + n + ec;

    return msg;
}

/*!
 * \internal
 * \brief Get the message string in case of no subsequential message
 * \return The message string
 */
QString LogMessage::singleMessage() const
{
    QString msg,spc;
    QChar sc,ec;

    sc = m_formatting.startEncasingChar();
    ec = m_formatting.endEncasingChar();
    spc = m_formatting.spacingString();


    QString priolev = "UNKNOWN";
    if (UnqlPriorityLevelNamesMap.contains(m_level))
        priolev = UnqlPriorityLevelNamesMap[m_level];

    //We have 3 start encasing char and 3 end encasing char (3*2) and 4 space (m_formatting.spacingSize() * 4)
    msg.reserve(3*2 + (m_formatting.spacingSize() * 4) + m_initTstamp.size() + m_loggerName.size() + m_msg.size() + priolev.size());

    msg = sc + m_initTstamp + ec + spc + sc + m_loggerName + ec + spc + sc + priolev + ec + spc + m_msg;

    return msg;
}






