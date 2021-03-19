#include "testTimeUtils.h"

#include "TimeUtils.h"

testTimeUtils::testTimeUtils()
{

}

void testTimeUtils::testDayTicked()
{
    QDateTime past = QDateTime::fromString("1974-03-16", "yyyy-MM-dd");
    QDateTime now = QDateTime::currentDateTime();
    QVERIFY(past.isValid());

    QVERIFY(TimeUtils::dayTicked(past, now));
    QVERIFY(!TimeUtils::dayTicked(now, past));

    //add some time (more than 24 hrs)
    QVERIFY(TimeUtils::dayTicked(past, past.addDays(1)));
    QVERIFY(TimeUtils::dayTicked(past, past.addDays(29))); //switch month
    QVERIFY(TimeUtils::dayTicked(past, past.addMonths(1)));
    QVERIFY(TimeUtils::dayTicked(past, past.addMonths(13))); //switch year
    QVERIFY(TimeUtils::dayTicked(past, past.addYears(1)));

    //add some time (less than 24 hrs)
    QVERIFY(!TimeUtils::dayTicked(past, past.addSecs(23 * 3600)));
    QVERIFY(!TimeUtils::dayTicked(past, past.addSecs((59 * 60) + (23 * 3600))));
}

void testTimeUtils::testHourTicked()
{
    QDateTime past = QDateTime::fromString("1974-03-16T02:00:00", "yyyy-MM-ddTHH:mm:ss");
    QDateTime now = QDateTime::currentDateTime();
    QVERIFY(past.isValid());

    QVERIFY(TimeUtils::hourTicked(past, now));
    QVERIFY(!TimeUtils::hourTicked(now, past));


    //add some time (more than 24 hrs)
    QVERIFY(TimeUtils::hourTicked(past, past.addDays(1)));
    QVERIFY(TimeUtils::hourTicked(past, past.addDays(29))); //switch month
    QVERIFY(TimeUtils::hourTicked(past, past.addMonths(1)));
    QVERIFY(TimeUtils::hourTicked(past, past.addMonths(13))); //switch year
    QVERIFY(TimeUtils::hourTicked(past, past.addYears(1)));

    //add some time (more than 1 hr)
    QVERIFY(TimeUtils::hourTicked(past, past.addSecs(23 * 3600)));
    QVERIFY(TimeUtils::hourTicked(past, past.addSecs(1 * 3600)));

    //add some time (less than 1 hr)
    QVERIFY(!TimeUtils::hourTicked(past, past.addSecs(59 * 60)));
    QVERIFY(!TimeUtils::hourTicked(past, past.addSecs(10 * 60)));
    QVERIFY(!TimeUtils::hourTicked(past, past.addSecs(1 * 60)));
}

void testTimeUtils::testMinuteTicked()
{
    QDateTime past = QDateTime::fromString("1974-03-16T02:00:00", "yyyy-MM-ddTHH:mm:ss");
    QDateTime now = QDateTime::currentDateTime();
    QVERIFY(past.isValid());

    QVERIFY(TimeUtils::minuteTicked(past, now));
    QVERIFY(!TimeUtils::minuteTicked(now, past));

    //add some time (more than 24 hrs)
    QVERIFY(TimeUtils::minuteTicked(past, past.addDays(1)));
    QVERIFY(TimeUtils::minuteTicked(past, past.addDays(29))); //switch month
    QVERIFY(TimeUtils::minuteTicked(past, past.addMonths(1)));
    QVERIFY(TimeUtils::minuteTicked(past, past.addMonths(13))); //switch year
    QVERIFY(TimeUtils::minuteTicked(past, past.addYears(1)));

    //add some time (more than 1 hr)
    QVERIFY(TimeUtils::minuteTicked(past, past.addSecs(23 * 3600)));
    QVERIFY(TimeUtils::minuteTicked(past, past.addSecs(1 * 3600)));

    //add some time (more than 1 min)
    QVERIFY(TimeUtils::minuteTicked(past, past.addSecs(59 * 60)));
    QVERIFY(TimeUtils::minuteTicked(past, past.addSecs(10 * 60)));
    QVERIFY(TimeUtils::minuteTicked(past, past.addSecs(1 * 60)));

    //add some time (less than 1 min)
    QVERIFY(!TimeUtils::minuteTicked(past, past.addSecs(59)));
    QVERIFY(!TimeUtils::minuteTicked(past, past.addSecs(10)));
    QVERIFY(!TimeUtils::minuteTicked(past, past.addSecs(1)));
}
