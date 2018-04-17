/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#ifndef __CONSOLE_LOGGER_INCS__
#define __CONSOLE_LOGGER_INCS__

#include "LogWriter.h"


class ConsoleWriter: public LogWriter
{
    Q_OBJECT

    UNQL::ConsoleColorType m_color;
    static QMutex m_consoleMux;

protected slots:
    void writeToDevice();

public:
    ConsoleWriter(const WriterConfig &wc);
    virtual ~ConsoleWriter();
    /*!
        \brief sets a new color for this console writer
        \param c the new color to be used
    */
    void setConsoleColor(UNQL::ConsoleColorType c) { m_color = c; }
    UNQL::ConsoleColorType getColor() const { return m_color; }
};

#endif

