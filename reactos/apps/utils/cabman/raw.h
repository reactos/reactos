/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/raw.h
 * PURPOSE:     CAB codec for uncompressed data
 */
#ifndef __RAW_H
#define __RAW_H

#include "cabinet.h"


/* Classes */

class CRawCodec : public CCABCodec {
public:
    /* Default constructor */
    CRawCodec();
    /* Default destructor */
    virtual ~CRawCodec();
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
};

#endif /* __RAW_H */

/* EOF */
