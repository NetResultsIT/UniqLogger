/********************************************************************************
 *   Copyright (C) 2010-2014 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#include "LogMessage.h"


QMap<UNQL::LogMessagePriorityType,QString> UnqlPriorityLevelNamesMap;

LogMessage::LogMessage(const QString &aLoggerName, UNQL::LogMessagePriorityType level,
                       const QString &aMsg, const QString &aTstamp)
    : m_msg(aMsg),
      m_loggerName(aLoggerName),
      m_level(level),
      m_tstamp(aTstamp)
{
}

QString
LogMessage::message() const
{
    QString msg,spc;
    QChar sc,ec;

    sc = m_formatting.startEncasingChar();
    ec = m_formatting.endEncasingChar();
    spc = m_formatting.spacingString();

    QString priolev="UNKNOWN";
    if (UnqlPriorityLevelNamesMap.contains(m_level))
        priolev = UnqlPriorityLevelNamesMap[m_level];

    msg = sc + m_tstamp + ec + spc + sc + m_loggerName + ec + spc + sc + priolev + ec + spc + m_msg;

    return msg;
}
