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
    QString s2 = "<165>1 2003-08-24T05:14:15.000-07:00 192.0.2.1 myproc 8710 - - %% It's time to make the do-nuts.";

    slmf.m_hostname = "192.0.2.1";
    slmf.m_appname = "myproc";
    slmf.m_pid = "8710";
    slmf.m_facility = 20;
    slmf.m_severity = 5;
    slmf.m_timestamp = QDateTime::fromString("2003-08-24T05:14:15.000003-07:00", Qt::ISODateWithMs);
    //qDebug() << slmf.m_timestamp.isValid();
    slmf.m_msgBody = "%% It's time to make the do-nuts.";

    QString sysm = slmf.generateMessage();
    QVERIFY(s != sysm);
    QVERIFY(s2 == sysm);
}

void TestSysLogMessage::test_case2()
{
    SysLogMessageFactory slmf;
    QString s = "<34>1 2003-10-11T22:14:15.003Z mymachine.example.com su - ID47 - 'su root' failed for lonvick on /dev/pts/8";

    slmf.m_hostname = "mymachine.example.com";
    slmf.m_appname = "su";
    slmf.m_mid = "ID47";
    slmf.m_facility = 4;
    slmf.m_severity = 2;
    slmf.m_timestamp = QDateTime::fromString("2003-10-11T22:14:15.003Z", Qt::ISODateWithMs);
    //qDebug() << slmf.m_timestamp.isValid();
    slmf.m_msgBody = "'su root' failed for lonvick on /dev/pts/8";

    QString sysm = slmf.generateMessage();
    QVERIFY(s == sysm);
}


QTEST_MAIN(TestSysLogMessage)
