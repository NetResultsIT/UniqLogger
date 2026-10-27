/********************************************************************************
 *   Copyright (C) 2010-2026 by Kalliope S.r.l. ( https://www.kalliope.com )    *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#ifndef CONSOLE_LOGGER_INCS_H
#define CONSOLE_LOGGER_INCS_H

#include "LogWriter.h"
#include "ConsoleColorScheme.h"


class ConsoleWriter: public LogWriter
{
    Q_OBJECT

    UNQL::ConsoleColorScheme m_colorScheme;
    static QMutex m_consoleMux;

    QString getColorCode(const UNQL::LogMessagePriorityType &i_level);
    void write(const LogMessage &log);

protected slots:
    void writeToDevice();

public:
    ConsoleWriter(const WriterConfig &wc);
    virtual ~ConsoleWriter();

    /*!
        \brief sets a new color for this console writer
        \param c the new color to be used
    */
    void setColorScheme(UNQL::ConsoleColorScheme c) { m_colorScheme = c; }
    UNQL::ConsoleColorScheme getColorScheme() const { return m_colorScheme; }
};

#endif

