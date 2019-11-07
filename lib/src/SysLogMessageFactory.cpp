#include "SysLogMessageFactory.h"

SysLogMessageFactory::SysLogMessageFactory(QObject *parent)
    : QObject(parent)
    , m_facility(0)
    , m_severity(0)
    , m_hostname("-")
    , m_appname("-")
    , m_pid("-")
    , m_mid("-")
{

}


QString
SysLogMessageFactory::calculatePriority()
{
    QString p = "<";
    quint8 pval = (m_facility * 8) + m_severity;
    p.append(QString::number(pval));
    p.append(">");
    return p;
}


QString
SysLogMessageFactory::generateHeader()
{
    QString h;
    const QString VERSION = "1";
    const QString SP = " ";
    h += calculatePriority() + VERSION + SP + m_timestamp.toString(Qt::ISODateWithMs) + SP + m_hostname + SP + m_appname + SP + m_pid + SP + m_mid;
    return h;
}


QString
SysLogMessageFactory::generateMessage()
{
    QString s;
    QString structured_data = "-";
    //s = "<165>1 2003-08-24T05:14:15.000003-07:00 192.0.2.1 myproc 8710 - - %% It's time to make the do-nuts.";
    s = generateHeader() + " " + structured_data + " " + m_msgBody;

    return s;
}
