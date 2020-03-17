#ifndef RSYSLOGWRITER_H
#define RSYSLOGWRITER_H

#include <RemoteWriter.h>
#include <SysLogMessageFactory.h>
#include <unqlog_common.h>

class RSyslogWriter: public RemoteWriter
{
    QString m_mid;
    quint8 m_facility;
    SysLogMessageFactory slmf;

    QString getMessage() override;

public:
    RSyslogWriter(const QString &i_id, quint8 i_facility, const QString &aServerAddress, quint16 aServerPort, const WriterConfig &wconf);
    QString getMessageId() const { return m_mid; }
    int getFacility() const { return m_facility; }

    static quint8 convertUnqlLogLevelToSyslog(UNQL::LogMessagePriorityType logLevel);
};

#endif // RSYSLOGWRITER_H
