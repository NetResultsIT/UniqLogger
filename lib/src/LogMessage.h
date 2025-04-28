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
    QPair<QChar, QChar> m_encasingChars;
    qsizetype m_spacingSize;
public:
    LogMessageFormatting(const QString &aSpace=" ", const QPair<QChar,QChar> aEncasingCharPair = qMakePair(QChar('['),QChar(']'))) :
        m_spacingString(aSpace),
        m_encasingChars(aEncasingCharPair)
    {
        m_spacingSize = aSpace.size();
    }
    QChar startEncasingChar() const { return m_encasingChars.first;   }
    QChar endEncasingChar() const   { return m_encasingChars.second;  }
    QString spacingString() const   { return m_spacingString;         }
    qsizetype spacingSize() const   { return m_spacingSize;           }
};

class LogMessage
{
    LogMessageFormatting m_formatting;
    QString m_msg, m_loggerName, m_logTag;
    UNQL::LogMessagePriorityType m_level;
    QString m_initTstamp, m_endTstamp;
    unsigned int m_repetitions;

    QString repeatedMessage() const;
    QString singleMessage() const;
public:
    LogMessage(const QString &aLoggerName, UNQL::LogMessagePriorityType level,
               const QString &aMsg, const QString &aInitTstamp, const QString &aEndTstamp = "", unsigned int repetitions = 1);

    static QString getCurrentTstampString();

    QString message() const;
    QString loggerName() const { return m_loggerName;  }
    QString rawMessage() const { return m_msg;         }
    QString initTstamp() const { return m_initTstamp;  }
    QString endTstamp()  const { return m_endTstamp;   }
    unsigned int repetitions() const { return m_repetitions; }
    UNQL::LogMessagePriorityType level() const { return m_level; }

    void setFormatting(const LogMessageFormatting& aMessageFormat) { m_formatting = aMessageFormat; }
};

#endif // LOGMESSAGE_H
