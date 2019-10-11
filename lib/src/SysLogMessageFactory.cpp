#include "SysLogMessageFactory.h"

SysLogMessageFactory::SysLogMessageFactory(QObject *parent) : QObject(parent)
{

}


QString
SysLogMessageFactory::generateMessage()
{
    QString s;

    s = "<165>1 2003-08-24T05:14:15.000003-07:00 192.0.2.1 myproc 8710 - - %% It's time to make the do-nuts.";

    return s;
}
