#include "TimeUtils.h"

#include <QDebug>

TimeUtils::TimeUtils()
{
    // Empty CTOR
}


bool TimeUtils::minuteTicked(const QDateTime&past, const QDateTime &now)
{
    //qDebug() << "Comparing " << past << " to " << now;
    bool b = false;
    if (
         hourTicked(past, now) //an hour passed
         || (past.time().minute() < now.time().minute())
    ) {
        b = true;
    }

    return b;
}

bool TimeUtils::hourTicked(const QDateTime&past, const QDateTime &now)
{
    //qDebug() << "Comparing " << past << " to " << now;
    bool b = false;
    if (
         (past.date() == now.date() //date is the same but hour is different
          && past.time().hour() < now.time().hour()) ||
         (past.date() < now.date())
    ) {
        b = true;
    }

    return b;
}

bool TimeUtils::dayTicked(const QDateTime&past, const QDateTime &now)
{
    //qDebug() << "Comparing " << past << " to " << now;
    bool b = false;
    if (
         past.date().year() < now.date().year() //date is the same but hour is different
         || (past.date().year() == now.date().year() && past.date().month() < now.date().month())
         || (past.date().year() == now.date().year()
             && past.date().month() == now.date().month()
             && past.date().day() < now.date().day())
    ) {
        b = true;
    }

    return b;
}
