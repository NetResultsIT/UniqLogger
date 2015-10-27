/********************************************************************************
 *   Copyright (C) 2014-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#include "bufferofstrings.h"


void BufferOfStrings::append(const QString &s)
{ return m_sl.append(s); }


QStringList BufferOfStrings::list() const
{ return m_sl; }


UNQL::LogMessagePriorityType BufferOfStrings::priority() const
{ return m_priority; }


int BufferOfStrings::count() const
{ return m_sl.count(); }


void BufferOfStrings::setPriority(UNQL::LogMessagePriorityType p)
{ m_priority = p; }
