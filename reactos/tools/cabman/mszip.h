/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/mszip.h
 * PURPOSE:     CAB codec for MSZIP compressed data
 */

#pragma once

#include "cabinet.h"
#include <zlib.h>

#define MSZIP_MAGIC 0x4B43


/* Classes */

class CMSZipCodec : public CCABCodec
{
public:
    /* Default constructor */
    CMSZipCodec();
    /* Default destructor */
    virtual ~CMSZipCodec();
    /* Compresses a data block */
    virtual ULONG Compress(void* OutputBuffer,
                           void* InputBuffer,
                           ULONG InputLength,
                           PULONG OutputLength);
    /* Uncompresses a data block */
    virtual ULONG Uncompress(void* OutputBuffer,
                             void* InputBuffer,
                             ULONG InputLength,
                             PULONG OutputLength);
private:
    int Status;
    z_stream ZStream; /* Zlib stream */
};

/* EOF */
