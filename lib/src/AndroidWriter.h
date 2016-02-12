#ifndef ANDROIDWRITER_H
#define ANDROIDWRITER_H

#include "LogWriter.h"
#include "android/log.h"
#include <QObject>
#include <QMap>

class AndroidWriter : public LogWriter
{
    Q_OBJECT

    QMap<UNQL::LogMessagePriorityType, android_LogPriority> androidPriorityMap;

public:
    AndroidWriter();

    virtual ~AndroidWriter();

    void writeToDevice();
};

#endif // ANDROIDWRITER_H
