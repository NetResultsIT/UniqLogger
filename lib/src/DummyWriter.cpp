/*
 *  DummyWriter.cpp
 *  (c) 2013 NetResults srl
 */

#include "DummyWriter.h"

DummyWriter::DummyWriter()
    : LogWriter(WriterConfig())
{
#ifdef ULOGDBG
    qDebug() << Q_FUNC_INFO << this;
#endif
}

DummyWriter::~DummyWriter()
{
#ifdef ULOGDBG
   qDebug() << Q_FUNC_INFO << this;
#endif
   this->flush();
}

void DummyWriter::writeToDevice()
{
    if (!m_logIsPaused)
    {
        mutex.lock();
        /*qDebug() << Q_FUNC_INFO << m_logMessageList.size();*/
        m_logMessageList.clear();
        mutex.unlock();
    }
}

