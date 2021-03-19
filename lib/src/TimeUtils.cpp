#include "TimeUtils.h"

#include <QDebug>

TimeUtils::TimeUtils()
{
    // Empty CTOR
}


/*!
 * \brief TimeUtils::minuteTicked
 * \param past older datetime
 * \param now newer datetime
 * \return true if at least a minute passed since past and now
 */
bool TimeUtils::minuteTicked(const QDateTime &past, const QDateTime &now)
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


/*!
 * \brief TimeUtils::hourTicked
 * \param past older datetime
 * \param now newer datetime
 * \return true if at least 1 hour passed since past and now
 */
bool TimeUtils::hourTicked(const QDateTime &past, const QDateTime &now)
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


/*!
 * \brief TimeUtils::dayTicked
 * \param past older datetime
 * \param now newer datetime
 * \return true if at least a day passed since past and now
 */
bool TimeUtils::dayTicked(const QDateTime &past, const QDateTime &now)
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
