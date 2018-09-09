#include "AndroidWriter.h"


AndroidWriter::AndroidWriter(const WriterConfig &wc)
    : LogWriter(wc)
{
    androidPriorityMap.insert(UNQL::LOG_FATAL, ANDROID_LOG_FATAL);
    androidPriorityMap.insert(UNQL::LOG_CRITICAL, ANDROID_LOG_ERROR);
    androidPriorityMap.insert(UNQL::LOG_WARNING, ANDROID_LOG_WARN);
    androidPriorityMap.insert(UNQL::LOG_INFO, ANDROID_LOG_INFO);
    androidPriorityMap.insert(UNQL::LOG_DBG, ANDROID_LOG_DEBUG);
    androidPriorityMap.insert(UNQL::LOG_DBG_ALL, ANDROID_LOG_VERBOSE);
    androidPriorityMap.insert(UNQL::LOG_OFF, ANDROID_LOG_UNKNOWN);
    androidPriorityMap.insert(UNQL::LOG_MONITOR, ANDROID_LOG_UNKNOWN);

}


AndroidWriter::~AndroidWriter()
{
}


void AndroidWriter::writeToDevice()
{
    mutex.lock();
    if (!m_logIsPaused) {
        int count = m_logMessageList.count();
        for (int i = 0; i < count; i++) {
            LogMessage message = m_logMessageList.takeFirst();
            android_LogPriority priority = androidPriorityMap.value(message.level(), ANDROID_LOG_UNKNOWN);

            if (priority != ANDROID_LOG_UNKNOWN) {
                __android_log_write(priority, message.loggerName().toUtf8(), message.message().toUtf8());
            }
        }
    }
    mutex.unlock();
}

