/*
 *  CompressionAlgo.cpp
 *  (c) NetResults Srl 2018
 */

#include "CompressionAlgo.h"
#include "FileCompressor.h"

int CompressionAlgoZip::compresionAlgo() const
{
    return FileCompressor::ZIP_FILE;
}

int CompressionAlgoGzip::compresionAlgo() const
{
    return FileCompressor::GZIP_FILE;
}
