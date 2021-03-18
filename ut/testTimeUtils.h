#ifndef TESTTIMEUTILS_H
#define TESTTIMEUTILS_H

#include <QTest>
#include <QObject>
class testTimeUtils : public QObject
{
    Q_OBJECT
public:
    testTimeUtils();
private slots:
    void testDayTicked();
    void testHourTicked();
    void testMinuteTicked();
};

#endif // TESTTIMEUTILS_H
