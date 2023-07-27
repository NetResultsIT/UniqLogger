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
{ /* empty dtor */ }


/*!
 * \brief AndroidWriter::writeToDevice this will write to logcat
 */
void AndroidWriter::writeToDevice()
{
    mutex.lock();
    if (!m_logIsPaused) {
        if (m_Config.compressMessages) {
            writeCompressedMessages();
        } else {
            writeUncompressedMessages();
        }
    }
    mutex.unlock();
}

/*!
 * \internal
 * \brief Write messages uncompressed, each message on a single line.
 */
void AndroidWriter::writeUncompressedMessages()
{
    int count = m_logMessageList.count();
    for (int i = 0; i < count; i++) {
        LogMessage message = m_logMessageList.takeFirst();
        android_LogPriority priority = androidPriorityMap.value(message.level(), ANDROID_LOG_UNKNOWN);

        if (priority != ANDROID_LOG_UNKNOWN) {
            __android_log_write(priority, message.loggerName().toUtf8(), message.message().toUtf8());
        }
    }
}

/*!
 * \internal
 * \brief Write messages in compressed way, if there are multiple
 *        messages with the same body, write all in a unique line.
 */
void AndroidWriter::writeCompressedMessages()
{
    int nummsg = m_logMessageList.count();
    int i = 0;
    int j = 1;

    if (nummsg == 1)
    {
        //just one element, write it
        LogMessage message = m_logMessageList.takeFirst();
        android_LogPriority priority = androidPriorityMap.value(message.level(), ANDROID_LOG_UNKNOWN);

        if (priority != ANDROID_LOG_UNKNOWN) {
            __android_log_write(priority, message.loggerName().toUtf8(), message.message().toUtf8());
        }
    }
    else
    {
        QString endTstamp = "";
        int counter = 1; //number of susbsequent messages
        while (j < nummsg)
        {
            if ((m_logMessageList.at(i).rawMessage() == m_logMessageList.at(j).rawMessage()) &&
                (m_logMessageList.at(i).level() == m_logMessageList.at(j).level()))
            {
                //subsequent messages, save end timestamp and look at next element
                endTstamp = m_logMessageList.at(j).tstamp();
                ++j;
                ++counter;
            }
            else
            {
                QString m;
                if (counter <= 1)
                {
                    //No subsequent messages, just write the message
                    m = m_logMessageList.at(i).message();
                }
                else
                {
                    //End of subsequent messages, write all as unique string
                    m = m_logMessageList.at(i).message(m_logMessageList.at(i).tstamp(), endTstamp, counter);
                }

                android_LogPriority priority = androidPriorityMap.value(m_logMessageList.at(i).level(), ANDROID_LOG_UNKNOWN);

                if (priority != ANDROID_LOG_UNKNOWN) {
                    __android_log_write(priority, m_logMessageList.at(i).loggerName().toUtf8(), m.toUtf8());
                }

                i = j;
                ++j;
                counter = 1;
                endTstamp = "";
            }

            if (j == nummsg)
            {
                //Don't want to skip last element
                QString m;
                if (endTstamp.isEmpty())
                {
                    m = m_logMessageList.at(i).message();
                }
                else
                {
                    m = m_logMessageList.at(i).message(m_logMessageList.at(i).tstamp(), endTstamp, counter);
                }

                android_LogPriority priority = androidPriorityMap.value(m_logMessageList.at(i).level(), ANDROID_LOG_UNKNOWN);

                if (priority != ANDROID_LOG_UNKNOWN) {
                    __android_log_write(priority, m_logMessageList.at(i).loggerName().toUtf8(), m.toUtf8());
                }
            }
        }
    }

    m_logMessageList.clear();
}
