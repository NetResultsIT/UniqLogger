#ifndef TESTSYSLOGMESSAGE_H
#define TESTSYSLOGMESSAGE_H

#include <QObject>

class TestSysLogMessage : public QObject
{
    Q_OBJECT
public:
    TestSysLogMessage();
private slots:
    void test_case1();
};

#endif // TESTSYSLOGMESSAGE_H
