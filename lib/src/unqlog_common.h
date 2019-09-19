#ifndef _UNQL_GLOBAL_
#define _UNQL_GLOBAL_

#ifdef ENABLE_UNQL_DBG
    #define ULDBG qDebug()
#else
    #define ULDBG if (true) {} else qDebug()
#endif

#ifdef WIN32
 #ifdef ULOG_LIB_EXPORTS
   #define ULOG_LIB_API __declspec(dllexport)
 #else
   #define ULOG_LIB_API __declspec(dllimport)
 #endif
#else
 #define ULOG_LIB_API
#endif

namespace UNQL {

enum LogMessagePriorityType
{
    LOG_FATAL       = 0,
    LOG_EMERGENCY   = 5,
    LOG_ALARM       = 8,
    LOG_CRITICAL    = 10,
    LOG_ERROR       = 50,
    LOG_WARNING     = 100,
    LOG_NOTICE      = 500,
    LOG_INFO        = 1000,
    LOG_DBG         = 10000,
    LOG_DBG_ALL     = 100000,
    LOG_OFF         = -1,
    LOG_MONITOR     = -10,
    LOG_FORCED      = -100
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
    UnqlErrorWriterConfigIncompatible   = -10,
    UnqlErrorWriterConfigNotSane        = -11,
    UnqlErrorImpossibleToCreateLogger   = -20,
    UnqlErrorImpossibleToCreateWriter   = -21
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
