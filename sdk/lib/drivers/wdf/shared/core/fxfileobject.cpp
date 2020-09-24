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

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
#include "FxFileObject.tmh"
}

//
// Public constructors
//


FxFileObject::FxFileObject(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in MdFileObject       pWdmFileObject,
    __in FxDevice*   pDevice
    ) :
    FxNonPagedObject(FX_TYPE_FILEOBJECT, sizeof(FxFileObject), FxDriverGlobals)
{
    m_FileObject.SetFileObject(pWdmFileObject);
    m_PkgContext = NULL;
    m_Device = pDevice;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    RtlInitUnicodeString(&m_FileName, NULL);
    m_RelatedFileObject = NULL;
#endif

    //
    // Cannot be deleted by the device driver
    //
    MarkNoDeleteDDI();
}

FxFileObject::~FxFileObject()
{
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    SAFE_RELEASE(m_RelatedFileObject);
#endif
}

//
// Create the WDFFILEOBJECT and associate it with the WDM PFILE_OBJECT.
//
_Must_inspect_result_
NTSTATUS
FxFileObject::_CreateFileObject(
    __in FxDevice*                   pDevice,
    __in MdIrp                        Irp,
    __in WDF_FILEOBJECT_CLASS        FileObjectClass,
    __in_opt PWDF_OBJECT_ATTRIBUTES  pObjectAttributes,
    __in_opt MdFileObject            pWdmFileObject,
    __deref_out_opt FxFileObject**   pFileObject
    )
{
    NTSTATUS Status;
    FxFileObject* pfo;
    KIRQL irql;
    WDF_FILEOBJECT_CLASS normalizedFileClass;

    PFX_DRIVER_GLOBALS FxDriverGlobals = pDevice->GetDriverGlobals();

    //
    // Create does require a WDM file obj based on the normalized file obj
    // class value.
    //
    normalizedFileClass = FxFileObjectClassNormalize(FileObjectClass);

    //
    // No FileObject support
    //
    if( normalizedFileClass == WdfFileObjectNotRequired ) {
        if( pFileObject != NULL ) *pFileObject = NULL;
        return STATUS_SUCCESS;
    }

    //
    // If fileobject support was specified, and a NULL
    // WDM PFILE_OBJECT is supplied, then it's an error
    //
    if( pWdmFileObject == NULL ) {

        //
        // Seems like some filter driver above us sending a create without fileobject.
        // We support this only if the FileObjectClass class is set to
        // WdfFileObjectWdfCannotUseFsContexts and device is created to be
        // exclusive.
        //
        if ( normalizedFileClass == WdfFileObjectWdfCannotUseFsContexts  &&
             pDevice->IsExclusive() ) {

            DO_NOTHING();

        } else {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
              "WdfFileObjectWdfCanUseFsContexts is specified, but the Create "
              "IRP %p doesn't have a fileObject\n",
              Irp);
            FxVerifierDbgBreakPoint(pDevice->GetDriverGlobals());
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

    }

    // Allocate a new FxFileObject with context
    pfo = new(pDevice->GetDriverGlobals(), pObjectAttributes)
             FxFileObject(pDevice->GetDriverGlobals(), pWdmFileObject, pDevice);

    if( pfo == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pfo->Initialize(Irp);

    // Assign FxDevice as the parent for the file object
    Status = pfo->Commit(pObjectAttributes, NULL, pDevice);
    if( !NT_SUCCESS(Status) ) {
        pfo->DeleteFromFailedCreate();
        return Status;
    }

    //
    // Place it on the list of FxFileObject's for this FxDevice
    //
    pDevice->Lock(&irql);
    InsertHeadList(&pDevice->m_FileObjectListHead, &pfo->m_Link);
    pDevice->Unlock(irql);

    //
    // Set file object context in mode-specific manner
    //
    pfo->SetFileObjectContext(pWdmFileObject,
                              normalizedFileClass,
                              Irp,
                              pDevice);

    // FxFileObject* to caller
    if( pFileObject != NULL ) {
        *pFileObject = pfo;
    }

    return STATUS_SUCCESS;
}

//
// Destroy (dereference) the WDFFILEOBJECT related to the
// WDM PFILE_OBJECT according to its FileObjectClass.
//
VOID
FxFileObject::_DestroyFileObject(
    __in FxDevice*                   pDevice,
    __in WDF_FILEOBJECT_CLASS        FileObjectClass,
    __in_opt MdFileObject            pWdmFileObject
    )
{
    FxFileObject* pfo = NULL;
    PFX_DRIVER_GLOBALS FxDriverGlobals = pDevice->GetDriverGlobals();
    WDF_FILEOBJECT_CLASS normalizedFileClass;

    //
    // Close does require a WDM file obj based on the normalized file obj
    // class value.
    //
    normalizedFileClass = FxFileObjectClassNormalize(FileObjectClass);

    if( normalizedFileClass == WdfFileObjectNotRequired ) {
        return;
    }

    //
    // Driver has specified file object support, and we
    // allocated one at Create, so they must pass one
    // to close, otherwise it's an error and we will leak
    // the file object.
    //
    MxFileObject wdmFileObject(pWdmFileObject);
    if( pWdmFileObject == NULL &&
        normalizedFileClass != WdfFileObjectWdfCannotUseFsContexts) {

        //
        // It's likely that IRP_MJ_CREATE had NULL for the Wdm FileObject as well.
        //
        // If a driver passes != NULL for Wdm FileObject to create, and NULL to
        // this routine, a WDF FxFileObject object leak will occur, which will
        // be reported at driver unload.
        //
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
           "PFILE_OBJECT in close IRP is NULL, *Possible Leak of FxFileObject*\n");

        FxVerifierDbgBreakPoint(pDevice->GetDriverGlobals());

        return;
    }
    else if( normalizedFileClass == WdfFileObjectWdfCanUseFsContext ) {
        pfo = (FxFileObject*)wdmFileObject.GetFsContext();
        wdmFileObject.SetFsContext(NULL);

    }
    else if( normalizedFileClass == WdfFileObjectWdfCanUseFsContext2 ) {
        pfo = (FxFileObject*)wdmFileObject.GetFsContext2();
        wdmFileObject.SetFsContext2(NULL);
    }
    else {
        NTSTATUS status;
        //
        // We must find the associated FxFileObject from the list
        // on the device
        //
        status = FxFileObject::_GetFileObjectFromWdm(
                          pDevice,
                          WdfFileObjectWdfCannotUseFsContexts,
                          pWdmFileObject,
                          &pfo
                          );

        //
        // We should find it, unless a different one was passed to IRP_MJ_CLOSE
        // than to IRP_MJ_CREATE, which is an error.
        //
        if (NT_SUCCESS(status) == FALSE || pfo == NULL) {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not find WDFFILEOBJECT for PFILE_OBJECT 0x%p",
                                pWdmFileObject);

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Did a different PFILE_OBJECT get passed to "
                            "IRP_MJ_CLOSE than did to IRP_MJ_CREATE?");
            FxVerifierDbgBreakPoint(pDevice->GetDriverGlobals());
        }
    }

    if( pfo != NULL ) {
        KIRQL irql;

        //
        // Remove it from the list of FxFileObjects on the FxDevice
        //
        pDevice->Lock(&irql);
        RemoveEntryList(&pfo->m_Link);
        pDevice->Unlock(irql);

        // Delete the file object
        pfo->DeleteObject();
    }

    return;
}

//
// Return the FxFileObject* for the given WDM PFILE_OBJECT
//
_Must_inspect_result_
NTSTATUS
FxFileObject::_GetFileObjectFromWdm(
    __in  FxDevice*                   pDevice,
    __in  WDF_FILEOBJECT_CLASS        FileObjectClass,
    __in_opt  MdFileObject            pWdmFileObject,
    __deref_out_opt FxFileObject**    ppFxFileObject
    )
{
    FxFileObject* pfo = NULL;
    PFX_DRIVER_GLOBALS FxDriverGlobals = pDevice->GetDriverGlobals();
    WDF_FILEOBJECT_CLASS normalizedFileClass;

    //
    // Normalize file object class value.
    //
    normalizedFileClass = FxFileObjectClassNormalize(FileObjectClass);

    //
    // No FileObject support
    //
    if( normalizedFileClass == WdfFileObjectNotRequired ) {
        *ppFxFileObject = NULL;
        return STATUS_SUCCESS;
    }

    if( pWdmFileObject == NULL ) {

        //
        // Warn if an I/O request has NULL for the WDM PFILE_OBJECT
        //
        if ( pDevice->IsExclusive() &&
             normalizedFileClass == WdfFileObjectWdfCannotUseFsContexts ) {
            //
            // We allow a NULL file object iff the device is exclusive and
            // we have to look up the WDFFILEOBJECT by PFILE_OBJECT value
            //
            DO_NOTHING();
        }
        else if ( FxIsFileObjectOptional(FileObjectClass) ) {
            //
            // Driver told us that it's able to handle this case.
            //
            *ppFxFileObject = NULL;
            return STATUS_SUCCESS;
        }
        else {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                    "NULL passed for PFILE_OBJECT when FileObject "
                                    "support is requested in an I/O request");
            FxVerifierDbgBreakPoint(pDevice->GetDriverGlobals());

            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Depending on the drivers configuration, we can quickly
    // get the FxFileObject* from FxContext, or FxContext2.
    //
    // Some drivers can not touch either of the FxContext(s), and
    // in that case we must resort to a list or hashtable.
    //
    MxFileObject wdmFileObject(pWdmFileObject);
    if( normalizedFileClass == WdfFileObjectWdfCanUseFsContext ) {
        pfo = (FxFileObject*)wdmFileObject.GetFsContext();
    }
    else if( normalizedFileClass == WdfFileObjectWdfCanUseFsContext2 ) {
        pfo = (FxFileObject*)wdmFileObject.GetFsContext2();
    }
    else {
        PLIST_ENTRY next;
        FxFileObject* f;
        KIRQL irql;

        //
        // Must look it up from the FxDevice->m_FileObjectListHead.
        //
        pfo = NULL;

        pDevice->Lock(&irql);

        next = pDevice->m_FileObjectListHead.Flink;

        if(pWdmFileObject == NULL) {
            //
            // If the pWdmFileObject is NULL then we will pass the first entry
            // in the list because the device must be exclusive and there
            // can be only one fileobject in the list.
            //
            ASSERT(IsListEmpty(&pDevice->m_FileObjectListHead) == FALSE);

            f = CONTAINING_RECORD(next, FxFileObject, m_Link);
            pfo = f;

        } else {

            while( next != &pDevice->m_FileObjectListHead ) {
                f = CONTAINING_RECORD(next, FxFileObject, m_Link);

                if( f->m_FileObject.GetFileObject()== pWdmFileObject ) {
                    pfo = f;
                    break;
                }

                next = next->Flink;
            }
        }










        if(pfo == NULL
             && pDevice->IsExclusive()
             && pDevice->GetMxDeviceObject()->GetDeviceType() == FILE_DEVICE_SERIAL_PORT
             && !IsListEmpty(&pDevice->m_FileObjectListHead)) {

            f = CONTAINING_RECORD(pDevice->m_FileObjectListHead.Flink,
                                    FxFileObject, m_Link);
            pfo = f;

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGDEVICE,
                            "The PFILE_OBJECT 0x%p in this request (cleanup/close) "
                            "is different from the one specified in "
                            "create request 0x%p.This is bad!", pWdmFileObject,
                            ((f != NULL) ? f->m_FileObject.GetFileObject(): NULL));
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGDEVICE,
                            "Since this is a serial port device, framework is "
                            "using a workaround to allow this");
        }

        pDevice->Unlock(irql);
    }

    //
    // This can happen if a different PFILE_OBJECT is passed to an I/O
    // request than was presented to IRP_MJ_CREATE
    //
    if (pfo == NULL && FxIsFileObjectOptional(FileObjectClass) == FALSE) {

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not locate WDFFILEOBJECT for "
                            "PFILE_OBJECT 0x%p",pWdmFileObject);

        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Did a different PFILE_OBJECT get passed to the "
                            "request than was to IRP_MJ_CREATE?");










        if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,9)) {
            FxVerifierDbgBreakPoint(pDevice->GetDriverGlobals());
        }
    }

    //
    // We don't do an extra reference count since the file objects
    // lifetime is controlled by the IoMgr. When the IRP_MJ_CLOSE
    // occurs, the reference is released after the optional
    // driver event callback.
    //

    *ppFxFileObject = pfo;

    return STATUS_SUCCESS;
}

VOID
FxFileObject::DeleteFileObjectFromFailedCreate(
    VOID
    )
{
    KIRQL irql;

    //
    // Remove it from the list of FxFileObjects on the FxDevice
    //
    m_Device->Lock(&irql);
    RemoveEntryList(&m_Link);
    m_Device->Unlock(irql);

    // Delete the file object
    DeleteFromFailedCreate();
}

_Must_inspect_result_
NTSTATUS
FxFileObject::QueryInterface(
    __in FxQueryInterfaceParams* Params
    )
/*++

Routine Description:
    This routine is used to return the pointer to the Object itself.

Arguments:
    Params - query interface parameters.

Return Value:
    NTSTATUS

--*/

{
    switch (Params->Type) {
    case FX_TYPE_FILEOBJECT:
        *Params->Object = (FxFileObject*) this;
        break;

    case FX_TYPE_IHASCALLBACKS:
        *Params->Object = (IFxHasCallbacks*) this;
        break;

    default:
        return __super::QueryInterface(Params);
    }

    return STATUS_SUCCESS;
}

VOID
FxFileObject::GetConstraints(
    __in WDF_EXECUTION_LEVEL*       ExecutionLevel,
    __in WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
    )
/*++

Routine Description:
    This routine implements the  GetConstraints IfxCallback. The caller gets the
    ExecutionLevel and the SynchronizationScope of the FileObject. The FileObject's
    SynchronizationScope and ExecutionLevel is strored in the FxPkgGeneral object.

Arguments:
    ExecutionLevel - (opt) receives the execution level.

    SynchronizationScope - (opt) receives the synchronization scope.

Return Value:
    None

--*/

{
    //
    // Get it from FxPkgGeneral which has the File attributes passed to it at device Creation time.
    //
    return GetDevice()->m_PkgGeneral->GetConstraintsHelper(ExecutionLevel, SynchronizationScope);
}

_Must_inspect_result_
FxCallbackLock*
FxFileObject::GetCallbackLockPtr(
    __deref_out_opt FxObject** LockObject
    )
/*++

Routine Description:
    This routine implements the GetCallbackLockPtr IfxCallback. The caller gets the
    LockObject to be used before calling the driver callbacks.

Arguments:
    LockObject - receives the lock object.

Return Value:
    FxCallbackLock *

--*/

{
    //
    // Get it from FxPkgGeneral which has the File attributes passed to it at device Creation time.
    //
    return GetDevice()->m_PkgGeneral->GetCallbackLockPtrHelper(LockObject);
}


