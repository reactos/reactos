/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxFileObjectUm.h

Abstract:

    User Mode implementation of File Object defined in MxFileObject.h

--*/

#pragma once

struct IWudfFile;

typedef IWudfFile * MdFileObject;

#include "MxFileObject.h"

__inline
PLARGE_INTEGER
MxFileObject::GetCurrentByteOffset(
   VOID
   )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return NULL;
}

__inline
ULONG
MxFileObject::GetFlags(
   VOID
   )
{








    return 0;
}

__inline
VOID
MxFileObject::SetFsContext(
    _In_ PVOID Value
    )
{
    UNREFERENCED_PARAMETER(Value);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

__inline
VOID
MxFileObject::SetFsContext2(
    _In_ PVOID Value
    )
{
    UNREFERENCED_PARAMETER(Value);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

__inline
PVOID
MxFileObject::GetFsContext(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return NULL;
}

__inline
PVOID
MxFileObject::GetFsContext2(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
    return NULL;
}

