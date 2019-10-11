#include "TestSysLogMessage.h"

#include <QtTest>
#include <QCoreApplication>

// add necessary includes here
#include "SysLogMessageFactory.h"

TestSysLogMessage::TestSysLogMessage()
{

}

void TestSysLogMessage::test_case1()
{

    SysLogMessageFactory slmf;
    QString s = "<165>1 2003-08-24T05:14:15.000003-07:00 192.0.2.1 myproc 8710 - - %% It's time to make the do-nuts.";
    QString s2 = slmf.generateMessage();
    QVERIFY(s == s2);
}


QTEST_MAIN(TestSysLogMessage)
