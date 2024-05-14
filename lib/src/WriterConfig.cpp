#include "WriterConfig.h"

#include <QStringList>

#include "NrFileCompressor.h"

/******************
 *                *
 *  WriterConfig  *
 *                *
 ******************/

WriterConfig::WriterConfig()
    : maxMessageNum      ( 0 )                        // unlimited
    , writerFlushSecs    ( 5 )                        // each writer will flush data every 5 seconds
    , writeIdleMark      ( false )                    // If no messages are to be written, write a MARK string to show writer is alive
    , compressMessages   ( false )                    // No compression of messages
    , maxFileSize        ( 0 )                        // unlimited MB size of logfile
    , maxFileNum         ( 1 )                        // log on just one file by default
    , rotationPolicy     ( UNQL::StrictRotation )     // We use the higher-numbers-are-older rotation policy by default
    , timeRotationPolicy ( UNQL::NoTimeRotation )     // Do not rotate on a time-based policy by default
    , compressionLevel   ( 6 )                        // Use default compression level
    , compressionAlgo    ( NrFileCompressor::NO_COMPRESSION )
    , maxMinutes         ( 0 )                        // Do not use rotation based on minutes elapsed
    , reconnectionSecs   ( 5 )                        // If RemoteWriter connection drops, try to reconnect every X secs
    , netProtocol        ( UNQL::TCP )                // Use TCP as transport protocol for remote messages
{
    /* empty ctor */
}


/*!
 * \brief WriterConfig::neededSanitizing Checks whether the WriterConfig instance was mis-configured
 * \return true if the WriterConfig instance holds some unacceptable values
 */
bool
WriterConfig::neededSanitizing() const
{
    //check that compressionLevel and compressionAlgo were populated with good values
    if (compressionAlgo < 0 || compressionAlgo > 2) {
        return true;
    }

    if (compressionLevel < 0 || compressionLevel > 9) {
        return true;
    }

    return false;
}


QString
WriterConfig::toString() const
{
    QString s;
    QStringList sl;
    sl << "COMMON PARAMS"
       << "\nMax queued messages: " << ((maxMessageNum==0) ? "unlimited" : "0") << "\nFlushing seconds: " << QString::number(writerFlushSecs)
       << "\nLog idle mark: " << (writeIdleMark ? "true" : "false")
       << "\nCompress messages: " << (compressMessages ? "true" : "false")
       << "\nFILE ONLY PARAMS"
       << "\nMax file size (MB): " << QString::number(maxFileSize) << "\nMax number of files: " << QString::number(maxFileNum)
       << "\nRotationNamingPolicy: " << QString::number(rotationPolicy) << "\nCompression level " << QString::number(compressionLevel)
       << "\nTimeRotationPolicy: " << QString::number(timeRotationPolicy) << "\nMax minutes allowed " << QString::number(maxMinutes)
       << "\nNETWORK PARAMS"
       << "\nReconnection seconds: " << QString::number(reconnectionSecs)
       << "\nTransport Protocol: " << QString::number(netProtocol)
       << "\n----------";

    return s = sl.join("");
}


bool
WriterConfig::operator ==(const WriterConfig& rhs) const
{
    if ( maxMessageNum      == rhs.maxMessageNum      &&
        writerFlushSecs    == rhs.writerFlushSecs    &&
        writeIdleMark      == rhs.writeIdleMark      &&
        maxFileNum         == rhs.maxFileNum         &&
        maxFileSize        == rhs.maxFileSize        &&
        maxMinutes         == rhs.maxMinutes         &&
        rotationPolicy     == rhs.rotationPolicy     &&
        timeRotationPolicy == rhs.timeRotationPolicy &&
        compressionLevel   == rhs.compressionLevel   &&
        reconnectionSecs   == rhs.reconnectionSecs   &&
        netProtocol        == rhs.netProtocol        &&
        compressMessages   == rhs.compressMessages
        )
    {
        return true;
    }

    return false;
}


bool WriterConfig::operator !=(const WriterConfig &rhs) const
{ return !(*this == rhs); }
