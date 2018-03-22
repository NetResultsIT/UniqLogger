/********************************************************************************
 *   Copyright (C) 2010-2015 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):																	*
 *				Francesco Lamonica		<f.lamonica@netresults.it>				*
 ********************************************************************************/

#ifndef __CONSOLE_LOGGER_INCS__
#define __CONSOLE_LOGGER_INCS__

#include "LogWriter.h"

using namespace std;

/*!
  \enum ConsoleColorType
  \brief this enumeration defines the color that can be used on the console writer
  */
enum ConsoleColorType {
                        NONE        =   -999,
                        black       =   30,
                        red         =   31,
                        green       =   32,
                        yellow       =   33,
                        blue        =   34,
                        magenta     =   35,
                        cyan        =   36,
						gray        =   37,
						grey        =   37,
						white       =   37/*,
                        darkgray    =   30,
                        lightred    =   31,
                        lightgreen  =   32,
                        yellow      =   33,
                        lightblue   =   34,
                        lightmagenta=   35,
                        lightcyan   =   36,
                        white       =   37*/
                      };

class ConsoleWriter: public LogWriter
{
	Q_OBJECT

    ConsoleColorType m_color;
    static QMutex m_consoleMux;

protected slots:
    void writeToDevice();

public:
    ConsoleWriter();
    virtual ~ConsoleWriter();
    /*!
        \brief sets a new color for this console writer
        \param c the new color to be used
    */
    void setConsoleColor(ConsoleColorType c) { m_color = c; }
};

#endif

