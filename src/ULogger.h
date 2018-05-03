/********************************************************************************
 *   Copyright (C) 2009-2010 by NetResults S.r.l. ( http://www.netresults.it )  *
 *   Author(s):									*
 *                  Francesco Lamonica      <f.lamonica@netresults.it>          *
 ********************************************************************************/

#ifndef ULOGGER_H
#define ULOGGER_H

#include <QHostAddress>
#include <QMutex>
#include <QMap>

#include "RemoteWriter.h"
#include "FileWriter.h"
#include "ConsoleWriter.h"
#include "Logger.h"


QMutex gMuxDeviceCounter;
int gLoggerCounter, gWriterCounter;

class ULOG_LIB_API UniqLogger
{
protected:
        UniqLogger();
        ~UniqLogger();

private:
	QMap<int, LogWriter*> m_DevicesMap;
        ConsoleWriter  *m_ConsoleLogger;

        void registerWriter(LogWriter*);
        void registerLogger(Logger*);

        int removeWriterFromLogger  (Logger*, LogWriter*);
        int addWriterToLogger       (Logger*, LogWriter*);

public:
        static UniqLogger* instance();
        Logger* createLogger        ( QString inLogName                     );
        Logger* createConsoleLogger ( QString inLogName, ConsoleColorType c );
        Logger* createFileLogger    ( QString inLogName, QString inFileName );
        Logger* createNetworkLogger ( QString inLogName, QString address,
                                      int port                              );
        LogWriter* getFileWriter    ( QString inFileName                    );
        LogWriter* getNetworkWriter ( QString address, int port             );
        LogWriter* getConsoleWriter ( ConsoleColorType c                    );
};

#endif // ULOGGER_H
