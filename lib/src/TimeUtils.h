#ifndef UNQL_TIMEUTILS_H
#define UNQL_TIMEUTILS_H

#include <QDateTime>

class TimeUtils
{
public:
    TimeUtils();
    static bool minuteTicked(const QDateTime&, const QDateTime&);
    static bool hourTicked(const QDateTime&, const QDateTime&);
    static bool dayTicked(const QDateTime&, const QDateTime&);
};

#endif // TIMEUTILS_H
