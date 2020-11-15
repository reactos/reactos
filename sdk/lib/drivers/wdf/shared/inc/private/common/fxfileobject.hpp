/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxFileObject.hpp

Abstract:

    This module implements a frameworks managed FileObject

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXFILEOBJECT_H_
#define _FXFILEOBJECT_H_

class FxFileObject : public FxNonPagedObject, public IFxHasCallbacks {

private:

    // Pointer to WDM FileObject
    MxFileObject        m_FileObject;
    PVOID               m_PkgContext;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    // KMDF retrieves the name unicode string directly from wdm file object,
    // and returns it to caller. UMDF doesn't provide a unicode string and
    // therefore we allocate it in this object and return that to caller after
    // populating it with name string retrieved from host.
    //
    UNICODE_STRING m_FileName;

    //
    // Framework related file object
    //
    FxFileObject* m_RelatedFileObject;

#endif

public:

    // ListEntry for linking FileObjects off of the device
    LIST_ENTRY         m_Link;

private:

    VOID
    SetFileObjectContext(
        _In_ MdFileObject WdmFileObject,
        _In_ WDF_FILEOBJECT_CLASS NormalizedFileClass,
        _In_ MdIrp Irp,
        _In_ FxDevice* Device
        );

public:
    FxFileObject(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in MdFileObject       pWdmFileObject,
        __in FxDevice*          Device
        );

    virtual
    ~FxFileObject(
       );

    __inline
    WDFFILEOBJECT
    GetHandle(
        VOID
        )
    {
        return (WDFFILEOBJECT) GetObjectHandle();
    }

    __inline
    MdFileObject
    GetWdmFileObject(
        VOID
        )
    {
        return m_FileObject.GetFileObject();
    }

    __inline
    CfxDevice*
    GetDevice(
        VOID
        )
    {
        return m_Device;
    }

    __inline
    PUNICODE_STRING
    GetFileName(
        VOID
        )
    {
    #if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        return m_FileObject.GetFileName(NULL);
    #else
        return m_FileObject.GetFileName(&m_FileName);
    #endif
    }

    __inline
    PLARGE_INTEGER
    GetCurrentByteOffset(
        VOID
        )
    {
        return m_FileObject.GetCurrentByteOffset();
    }

    __inline
    ULONG
    GetFlags(
        VOID
        )
    {
        return m_FileObject.GetFlags();
    }

    __inline
    VOID
    SetPkgCleanupCloseContext(
        PVOID Context
        )
    {
        m_PkgContext = Context;
    }

    __inline
    PVOID
    GetPkgCleanupCloseContext(
        VOID
        )
    {
        return m_PkgContext;
    }

    //
    // Create a WDFFILEOBJECT from the WDM PFILE_OBJECT
    // and associate it with the WDM PFILE_OBJECT according
    // to the FileObjectClass.
    //
    _Must_inspect_result_
    static
    NTSTATUS
    _CreateFileObject(
        __in FxDevice*                   pDevice,
        __in MdIrp                        Irp,
        __in WDF_FILEOBJECT_CLASS        FileObjectClass,
        __in_opt PWDF_OBJECT_ATTRIBUTES  pObjectAttributes,
        __in_opt MdFileObject            pWdmFileObject,
        __deref_out_opt FxFileObject**   ppFxFileObject
        );

    VOID
    Initialize(
        _In_ MdIrp CreateIrp
        );

    //
    // Destroy (dereference) the WDFFILEOBJECT related to the
    // WDM PFILE_OBJECT according to its FileObjectClass.
    //
    static
    VOID
    _DestroyFileObject(
        __in FxDevice*                   pDevice,
        __in WDF_FILEOBJECT_CLASS        FileObjectClass,
        __in_opt MdFileObject            pWdmFileObject
        );

    //
    // Return the FxFileObject* for the given WDM PFILE_OBJECT
    // based on the FileObjectClass.
    //
    _Must_inspect_result_
    static
    NTSTATUS
    _GetFileObjectFromWdm(
        __in  FxDevice*                   pDevice,
        __in  WDF_FILEOBJECT_CLASS        FileObjectClass,
        __in_opt  MdFileObject            pWdmFileObject,
        __deref_out_opt FxFileObject**    ppFxFileObject
        );

    VOID
    DeleteFileObjectFromFailedCreate(
        VOID
        );

    // begin FxObject overrides
    _Must_inspect_result_
    NTSTATUS
    QueryInterface(
        __in FxQueryInterfaceParams* Params
        );
    // end FxObject overrides

    // begin IFxHasCallbacks overrides
    VOID
    GetConstraints(
        __in WDF_EXECUTION_LEVEL*       ExecutionLevel,
        __in WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
        ) ;

    _Must_inspect_result_
    FxCallbackLock*
    GetCallbackLockPtr(
        __deref_out_opt FxObject** LockObject
        );
    // end IFxHasCallbacks overrides

    //
    // Update the process keep-alive count
    // TRUE: increment, FALSE: decrement
    //
    _Must_inspect_result_
    NTSTATUS
    UpdateProcessKeepAliveCount(
        _In_ BOOLEAN Increment
        );

#if (FX_CORE_MODE == FX_CORE_USER_MODE)

    FxFileObject*
    GetRelatedFileObject(
        VOID
        )
    {
        return m_RelatedFileObject;
    }

#endif

};

#endif // _FXFILEOBJECT_H_

