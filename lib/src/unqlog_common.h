#ifndef _UNQL_GLOBAL_
#define _UNQL_GLOBAL_

#ifdef ULOGDBG
    #define ULDBG qDebug()
#else
    #define ULDBG if (true) {} else qDebug()
#endif

namespace UNQL {

enum LogMessagePriorityType
{
    LOG_FATAL       = 0,
    LOG_CRITICAL    = 10,
    LOG_WARNING     = 100,
    LOG_INFO        = 1000,
    LOG_DBG         = 10000,
    LOG_DBG_ALL     = 100000,
    LOG_OFF         = -1,
    LOG_MONITOR     = -10
};


enum LogStreamManipType
{
    eom,
    EOM=eom,
    fls,
    FLS=fls
};

}

#endif
