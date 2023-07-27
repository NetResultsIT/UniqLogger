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

    void writeUncompressedMessages();
    void writeCompressedMessages();
public:
    AndroidWriter(const WriterConfig &wc);

    virtual ~AndroidWriter();

    void writeToDevice();
};

#endif // ANDROIDWRITER_H
