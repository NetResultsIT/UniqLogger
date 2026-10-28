/********************************************************************************
 *   Copyright (C) 2010-2026 by Kalliope S.r.l. ( https://www.kalliope.com )    *
 *   Author(s):                                                                 *
 *              Francesco Lamonica      <f.lamonica@netresults.it>              *
 ********************************************************************************/

#ifndef UNQLDBHANDLER_H
#define UNQLDBHANDLER_H

#include <nrdbhandler.h>

class UnqlDbHandler : NrBaseDbHandler
{

public:
    explicit UnqlDbHandler(const DbhConfig cfg);
    virtual ~UnqlDbHandler();

    void logEvent(const QString &module, const QString &tstamp, const QString &level, const QString &message);
    static int createLoggerDb(QString qeventdbname);
};

#endif // DBHANDLER_H
