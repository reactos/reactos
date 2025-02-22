/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/cabman.h
 * PURPOSE:     Cabinet manager header
 */

#pragma once

#include "cabinet.h"

#ifndef CAB_READ_ONLY

class CCFDATAStorage
{
public:
    /* Default constructor */
    CCFDATAStorage();
    /* Default destructor */
    virtual ~CCFDATAStorage();
    ULONG Create();
    ULONG Destroy();
    ULONG Truncate();
    ULONG Position();
    ULONG Seek(LONG Position);
    ULONG ReadBlock(PCFDATA Data, void* Buffer, PULONG BytesRead);
    ULONG WriteBlock(PCFDATA Data, void* Buffer, PULONG BytesWritten);
private:
    char FullName[PATH_MAX];
    FILE* FileHandle;
};

#endif /* CAB_READ_ONLY */

