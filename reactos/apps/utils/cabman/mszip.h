/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/mszip.h
 * PURPOSE:     CAB codec for MSZIP compressed data
 */
#ifndef __MSZIP_H
#define __MSZIP_H

#include "cabinet.h"
#include "zlib/zlib.h"

#define MSZIP_MAGIC 0x4B43


/* Classes */

class CMSZipCodec : public CCABCodec {
public:
    /* Default constructor */
    CMSZipCodec();
    /* Default destructor */
    virtual ~CMSZipCodec();
    /* Compresses a data block */
    virtual ULONG Compress(PVOID OutputBuffer,
                           PVOID InputBuffer,
                           DWORD InputLength,
                           PDWORD OutputLength);
    /* Uncompresses a data block */
    virtual ULONG Uncompress(PVOID OutputBuffer,
                             PVOID InputBuffer,
                             DWORD InputLength,
                             PDWORD OutputLength);
private:
    INT Status;
    z_stream ZStream; /* Zlib stream */
};

#endif /* __MSZIP_H */

/* EOF */
