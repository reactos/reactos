/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxFileObjectKm.h

Abstract:

    Kernel Mode implementation of File Object defined in MxFileObject.h

--*/

#pragma once

#include "mxfileobject.h"

__inline
PUNICODE_STRING
MxFileObject::GetFileName(
    _Inout_opt_ PUNICODE_STRING Filename
   )
{
    UNREFERENCED_PARAMETER(Filename);

    return  &m_FileObject->FileName;
}

__inline
PLARGE_INTEGER
MxFileObject::GetCurrentByteOffset(
   VOID
   )
{
   return &m_FileObject->CurrentByteOffset;
}

__inline
ULONG
MxFileObject::GetFlags(
   VOID
   )
{
   return m_FileObject->Flags;
}

__inline
VOID
MxFileObject::SetFsContext(
    _In_ PVOID Value
    )
{
    m_FileObject->FsContext = Value;
}

__inline
VOID
MxFileObject::SetFsContext2(
    _In_ PVOID Value
    )
{
    m_FileObject->FsContext2 = Value;
}

__inline
PVOID
MxFileObject::GetFsContext(
    VOID
    )
{
    return m_FileObject->FsContext;
}

__inline
PVOID
MxFileObject::GetFsContext2(
    VOID
    )
{
    return m_FileObject->FsContext2;
}
