/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxFileObject.h

Abstract:

    Mode agnostic definition of File Object

    See MxFileObjectKm.h and MxFileObjectUm.h/cpp for mode
    specific implementations

--*/

#pragma once

class MxFileObject
{
private:
    MdFileObject m_FileObject;

public:
    __inline
    MxFileObject(
        _In_ MdFileObject FileObject
        ) :
        m_FileObject(FileObject)
    {
    }

    __inline
    VOID
    SetFileObject(
        _In_ MdFileObject FileObject
        )
    {
        m_FileObject = FileObject;
    }

    __inline
    MdFileObject
    GetFileObject(
        VOID
        )
    {
        return m_FileObject;
    }

    __inline
    MxFileObject(
        VOID
        ) :
        m_FileObject(NULL)
    {
    }

    PUNICODE_STRING
    GetFileName(
        _Inout_opt_ PUNICODE_STRING Filename
        );

    PLARGE_INTEGER
    GetCurrentByteOffset(
        VOID
        );

    ULONG
    GetFlags(
        VOID
        );

    VOID
    SetFsContext(
        _In_ PVOID Value
        );

    VOID
    SetFsContext2(
        _In_ PVOID Value
        );

    PVOID
    GetFsContext(
        VOID
        );

    PVOID
    GetFsContext2(
        VOID
        );

};


