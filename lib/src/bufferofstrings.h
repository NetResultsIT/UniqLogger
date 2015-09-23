/********************************************************************************
 *   Copyright (C) 2014-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#ifndef BUFFEROFSTRINGS_H
#define BUFFEROFSTRINGS_H

#include <QStringList>

#include "LogMessage.h"


class BufferOfStrings
{
    QStringList m_sl;
    UNQL::LogMessagePriorityType m_priority;

public:
    explicit BufferOfStrings(): m_priority(UNQL::LOG_INFO) {}
    void append(const QString &s) { return m_sl.append(s); }
    QStringList list() const { return m_sl; }
    UNQL::LogMessagePriorityType priority() const { return m_priority; }
    int count() const { return m_sl.count(); }
    void setPriority(UNQL::LogMessagePriorityType p) { m_priority = p; }
};

#endif // BUFFEROFSTRINGS_H
