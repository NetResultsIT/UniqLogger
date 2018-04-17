#ifndef _UNQL_GLOBAL_
#define _UNQL_GLOBAL_

#ifdef ENABLE_ULOG_DBG
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


enum ErrorType
{
    UnqlErrorNoError                    =  0,
    UnqlErrorFileNotWritable            = -1,
    UnqlErrorWriterConfigIncompatible   = -2,
    UnqlErrorImpossibleToCreateLogger   = -3,
    UnqlErrorImpossibleToCreateWriter   = -4
};


enum RotationPolicy
{
    WeekDaysRotaion,
    MonthDayRotation
};

#define DEF_UNQL_TS_FMT "yyyy-MM-dd HH:mm:ss"
#define DEF_UNQL_LOG_STR "UniqLogger"

}

#endif
