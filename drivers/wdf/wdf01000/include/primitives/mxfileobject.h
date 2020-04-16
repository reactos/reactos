#ifndef _MXFILEOBJECT_H_
#define _MXFILEOBJECT_H_

#include "common/mxgeneral.h"


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

    __inline
    ULONG
    GetFlags(
       VOID
       )
    {
       return m_FileObject->Flags;
    }

    __inline
    PUNICODE_STRING
    GetFileName(
        _Inout_opt_ PUNICODE_STRING Filename
       )
    {
        UNREFERENCED_PARAMETER(Filename);

        return  &m_FileObject->FileName;
    }

    __inline
    PLARGE_INTEGER
    GetCurrentByteOffset(
       VOID
       )
    {
       return &m_FileObject->CurrentByteOffset;
    }

    __inline
    VOID
    SetFsContext(
        _In_ PVOID Value
        )
    {
        m_FileObject->FsContext = Value;
    }

    __inline
    VOID
    SetFsContext2(
        _In_ PVOID Value
        )
    {
        m_FileObject->FsContext2 = Value;
    }

    __inline
    PVOID
    GetFsContext(
        VOID
        )
    {
        return m_FileObject->FsContext;
    }

    __inline
    PVOID
    GetFsContext2(
        VOID
        )
    {
        return m_FileObject->FsContext2;
    }
};

#endif //_MXFILEOBJECT_H_
