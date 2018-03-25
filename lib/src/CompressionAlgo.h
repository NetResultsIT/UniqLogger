/*
 *  CompressionAlgo.h
 *  (c) NetResults Srl 2018
 */

#ifndef _UNQL_COMPRESSION_ALGO_H_
#define _UNQL_COMPRESSION_ALGO_H_

class CompressionAlgoIface
{
public:
    virtual int compresionAlgo() const = 0;
    virtual ~CompressionAlgoIface() {}
};


class CompressionAlgoZip : public CompressionAlgoIface
{
public:
    int compresionAlgo() const;
    virtual ~CompressionAlgoZip() {}
};

class CompressionAlgoGzip : public CompressionAlgoIface
{
public:
    int compresionAlgo() const;
    virtual ~CompressionAlgoGzip() {}
};


#endif /* _UNQL_COMPRESSION_ALGO_H_ */
