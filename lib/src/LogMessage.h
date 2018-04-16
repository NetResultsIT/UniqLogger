/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H

#include <QPair>
#include <QMap>
#include <QString>

#include "unqlog_common.h"

class LogMessageFormatting
{
    QString m_spacingString;
    QPair<QChar,QChar> m_encasingChars;
public:
    LogMessageFormatting(const QString &aSpace=" ", const QPair<QChar,QChar> aEncasingCharPair = qMakePair(QChar('['),QChar(']'))) :
        m_spacingString(aSpace),
        m_encasingChars(aEncasingCharPair)
    {}
    QChar startEncasingChar() const { return m_encasingChars.first;   }
    QChar endEncasingChar() const   { return m_encasingChars.second;  }
    QString spacingString() const   { return m_spacingString;         }
};

class LogMessage
{
    LogMessageFormatting m_formatting;
    QString m_msg, m_loggerName;
    UNQL::LogMessagePriorityType m_level;
    QString m_tstamp;

public:
    LogMessage(const QString &aLoggerName, UNQL::LogMessagePriorityType level, const QString &aMsg, const QString &aTstamp);

    static QString getCurrentTstampString();

    QString loggerName() const  { return m_loggerName;  }
    QString message() const;
    QString rawMessage() const  { return m_msg;         }
    QString tstamp() const      { return m_tstamp;      }
    UNQL::LogMessagePriorityType level() const
                                { return m_level;       }

    void setFormatting(const LogMessageFormatting& aMessageFormat) { m_formatting = aMessageFormat; }
};

#endif // LOGMESSAGE_H
