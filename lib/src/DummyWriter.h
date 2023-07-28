/*
 *  DummyWriter.h
 *  (c) 2013 NetResults srl
 */

#ifndef UNQL_DUMMY_WRITER_H
#define UNQL_DUMMY_WRITER_H

#include "LogWriter.h"

class DummyWriter : public LogWriter
{
    Q_OBJECT

protected slots:
    void writeToDevice();

public:
    DummyWriter();
    virtual ~DummyWriter();
};

#endif /* _DUMMY_WRITER_H_ */
