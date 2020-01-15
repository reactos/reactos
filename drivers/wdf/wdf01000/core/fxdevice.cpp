#include "common/fxdevice.h"
#include "common/fxpkgio.h"
#include "common/fxirppreprocessinfo.h"


_Must_inspect_result_
__inline
BOOLEAN
IsPreprocessIrp(
    __in MdIrp       Irp,
    __in FxIrpPreprocessInfo*  Info
    )
{
    UCHAR       major, minor;
    BOOLEAN     preprocess;
    FxIrp irp(Irp);

    major = irp.GetMajorFunction();
    minor = irp.GetMinorFunction();

    preprocess = FALSE;

    if (Info->Dispatch[major].EvtDevicePreprocess != NULL)
    {
        if (Info->Dispatch[major].NumMinorFunctions == 0)
        {
            //
            // If the driver is not interested in particular minor codes,
            // just give the irp to it.
            //
            preprocess = TRUE;
        }
        else
        {
            ULONG i;

            //
            // Try to match up to a minor code.
            //
            for (i = 0; i < Info->Dispatch[major].NumMinorFunctions; i++)
            {
                if (Info->Dispatch[major].MinorFunctions[i] == minor)
                {
                    preprocess = TRUE;
                    break;
                }
            }
        }
    }

    return preprocess;
}

_Must_inspect_result_
__inline
NTSTATUS
PreprocessIrp(
    __in FxDevice*  Device,
    __in MdIrp       Irp,
    __in FxIrpPreprocessInfo*  Info,
    __in PVOID      DispatchContext
    )
{
    NTSTATUS        status;
    MdDeviceObject  devObj;
    UCHAR           major, minor;
    FxIrp irp(Irp);

    major = irp.GetMajorFunction();
    minor = irp.GetMinorFunction();

    //
    // If this is a pnp remove irp, this object could be deleted by the time
    // EvtDevicePreprocess returns.  To not touch freed pool, capture all
    // values we will need before preprocessing.
    //
    devObj = Device->GetDeviceObject();

    //if (Info->ClassExtension == FALSE)
    {
        status = Info->Dispatch[major].EvtDevicePreprocess( Device->GetHandle(),
                                                            Irp);
    }
    /*else
    {
        status = Info->Dispatch[major].EvtCxDevicePreprocess(
                                                            Device->GetHandle(),
                                                            Irp,
                                                            DispatchContext);
    }*/

    //
    // If we got this far, we handed the irp off to EvtDevicePreprocess, so we
    // must now do our remlock maintainance if necessary.
    //
    if (FxDevice::_RequiresRemLock(major, minor) == FxDeviceRemLockRequired)
    {
        //
        // Keep the remove lock active until after we call into the driver.
        // If the driver redispatches the irp to the framework, we will
        // reacquire the remove lock at that point in time.
        //
        // Touching pDevObj after sending the pnp remove irp to the framework
        // is OK b/c we have acquired the remlock previously and that will
        // prevent this irp's processing racing with the pnp remove irp
        // processing.
        //
        Mx::MxReleaseRemoveLock(Device->GetRemoveLock(),
                                Irp);
    }

    return status;
}

_Must_inspect_result_
__inline
NTSTATUS
DispatchWorker(
    __in FxDevice*  Device,
    __in MdIrp       Irp,
    __in WDFCONTEXT DispatchContext
    )
{
    PLIST_ENTRY next;
    FxIrp irp(Irp);

    next = (PLIST_ENTRY)DispatchContext;

    ASSERT(NULL != DispatchContext &&
           ((UCHAR)DispatchContext & FX_IN_DISPATCH_CALLBACK) == 0);

    //
    // Check for any driver/class-extensions' preprocess requirements.
    //
    while (next != &Device->m_PreprocessInfoListHead)
    {
        FxIrpPreprocessInfo* info;

        info = CONTAINING_RECORD(next, FxIrpPreprocessInfo, ListEntry);

        //
        // Advance to next node.
        //
        next = next->Flink;

        if (IsPreprocessIrp(Irp, info))
        {
            return PreprocessIrp(Device, Irp, info, next);
        }
    }

    //
    // No preprocess requirements, directly dispatch the IRP.
    //
    return Device->GetDispatchPackage(
        irp.GetMajorFunction()
        )->Dispatch(Irp);
}

_Must_inspect_result_
NTSTATUS
FxDevice::Dispatch(
    __in MdDeviceObject DeviceObject,
    __in MdIrp       Irp
    )
{
    FxDevice* device = FxDevice::GetFxDevice(DeviceObject);
    return DispatchWorker(device,
                          Irp,
                          device->m_PreprocessInfoListHead.Flink);
}

_Must_inspect_result_
NTSTATUS
FxDevice::DispatchWithLock(
    __in MdDeviceObject DeviceObject,
    __in MdIrp Irp
    )
{
    NTSTATUS status;
    FxIrp irp(Irp);

    switch (_RequiresRemLock(irp.GetMajorFunction(),
                             irp.GetMinorFunction())) {

    case FxDeviceRemLockRequired:
        status = Mx::MxAcquireRemoveLock(
            &_GetFxWdmExtension(DeviceObject)->IoRemoveLock,
            Irp
            );

        if (!NT_SUCCESS(status)) {
            irp.SetStatus(status);
            irp.CompleteRequest(IO_NO_INCREMENT);

            return status;
        }

        break;

    case FxDeviceRemLockOptIn:
        status = _AcquireOptinRemoveLock(
            DeviceObject,
            Irp
            );

        if (!NT_SUCCESS(status)) {
            irp.SetStatus(status);
            irp.CompleteRequest(IO_NO_INCREMENT);

            return status;
        }

        break;

    case FxDeviceRemLockTestValid:
        //
        // Try to Acquire and Release the RemLock.  If acquiring the lock
        // fails then it is not safe to process the IRP and the IRP should
        // be completed immediately.
        //
        status = Mx::MxAcquireRemoveLock(
            &_GetFxWdmExtension(DeviceObject)->IoRemoveLock,
            Irp
            );

        if (!NT_SUCCESS(status)) {
            irp.SetStatus(status);
            irp.CompleteRequest(IO_NO_INCREMENT);

            return status;
        }

        Mx::MxReleaseRemoveLock(
            &_GetFxWdmExtension(DeviceObject)->IoRemoveLock,
            Irp
            );
        break;
    }

    return Dispatch(DeviceObject, Irp);
}

_Must_inspect_result_
NTSTATUS
FxDevice::_AcquireOptinRemoveLock(
    __in MdDeviceObject DeviceObject,
    __in MdIrp Irp
    )
{
    NTSTATUS status;
    FxIrp irp(Irp);

    //
    // NOTE: This value contained in WDF 1.15
    //
    //#define WDF_REMOVE_LOCK_OPTION_ACQUIRE_FOR_IO   0x00000001
    //
    // Current WDF version 1.9 and method always return SUCCESS
    //
    /*
    FxWdmDeviceExtension * wdmDeviceExtension =
        FxDevice::_GetFxWdmExtension(DeviceObject);

    if (wdmDeviceExtension->RemoveLockOptionFlags &
            WDF_REMOVE_LOCK_OPTION_ACQUIRE_FOR_IO) {

        status = Mx::MxAcquireRemoveLock(&(wdmDeviceExtension->IoRemoveLock), Irp);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        irp.CopyCurrentIrpStackLocationToNext();

        irp.SetCompletionRoutineEx(
                DeviceObject,
                _CompletionRoutineForRemlockMaintenance,
                DeviceObject,
                TRUE,
                TRUE,
                TRUE
                );

        irp.SetNextIrpStackLocation();
    }*/

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxDevice::DeleteDeviceFromFailedCreate(
    __in NTSTATUS FailedStatus,
    __in BOOLEAN UseStateMachine
    )
{
    NTSTATUS status;

    status = DeleteDeviceFromFailedCreateNoDelete(FailedStatus, UseStateMachine);

    //
    // Delete the Fx object now
    //
    DeleteObject();

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::DeleteDeviceFromFailedCreateNoDelete(
    __in NTSTATUS FailedStatus,
    __in BOOLEAN UseStateMachine
    )
{
    //
    // Cleanup the device, the driver may have allocated resources
    // associated with the WDFDEVICE
    //
    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
        "WDFDEVICE %p !devobj %p created, but EvtDriverDeviceAdd returned "
        "status %!STATUS! or failure in creation",
        GetObjectHandleUnchecked(), GetDeviceObject(), FailedStatus);

    //
    //
#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    //
    // We do not let filters affect the building of the rest of the stack.
    // If they return error, we convert it to STATUS_SUCCESS, remove the
    // attached device from the stack, and cleanup.
    //
    if (IsFilter())
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE %p, !devobj %p is a filter, converting %!STATUS! to"
            " STATUS_SUCCESS", GetObjectHandleUnchecked(), GetDeviceObject(),
            FailedStatus);
        FailedStatus = STATUS_SUCCESS;
    }
#endif

    if (UseStateMachine)
    {
        MxEvent waitEvent;

        //
        // See comments for m_CleanupFromFailedCreate in class definition file
        // for use of this statement.
        //
        SetCleanupFromFailedCreate(TRUE);
        waitEvent.Initialize(SynchronizationEvent, FALSE);
        m_PkgPnp->CleanupDeviceFromFailedCreate(waitEvent.GetSelfPointer());
    }
    else
    {
        //
        // Upon certain types of failure, like STATUS_OBJECT_NAME_COLLISION, we
        // could keep the pDevice around and the caller retry after changing
        // a property, but the simpler route for now is to just recreate
        // everything from scratch on the retry.
        //
        // Usually the pnp state machine will do this and the FxDevice destructor
        // relies on it running b/c it does some cleanup.
        //
        EarlyDispose();
        DestroyChildren();

        //
        // Wait for all children to drain out and cleanup.
        //
        if (m_DisposeList != NULL)
        {
            m_DisposeList->WaitForEmpty();
        }

        //
        // We keep a reference on m_PkgPnp which is released in the destructor
        // so  we can safely touch m_PkgPnp after destroying all of the child
        // objects.
        //
        if (m_PkgPnp != NULL)
        {
            m_PkgPnp->CleanupStateMachines(TRUE);
        }
    }

    //
    // This will detach and delete the device object
    //
    Destroy();

    return FailedStatus;
}

VOID
FxDevice::Destroy(
    VOID
    )
{
    //
    // We must be at passive for IoDeleteDevice
    //
    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    if (m_DeviceObject.GetObject() != NULL)
    {
        //
        // The device object may not go away right away if there are pending
        // references on it.  But we can't look up our FxDevice anymore, so
        // lets clear the DeviceExtension pointer.
        //
        m_DeviceObject.SetDeviceExtension(NULL);
    }

    //
    // Since this can be called in the context of the destructor when the ref
    // count is zero, use GetObjectHandleUnchecked() to get the handle value.
    //
    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGDEVICE,
        "Deleting !devobj %p, WDFDEVICE %p, attached to !devobj %p",
        m_DeviceObject.GetObject(), GetObjectHandleUnchecked(), m_AttachedDevice.GetObject());

    DetachDevice();

    if (m_DeviceObject.GetObject() != NULL)
    {
        DeleteSymbolicLink();

        if (m_DeviceObjectDeleted)
        {
            //
            // The device already deleted previously, release the reference we
            // took at the time of delete.
            //
            Mx::MxDereferenceObject(m_DeviceObject.GetObject());
        }
        else
        {
            Mx::MxDeleteDevice(m_DeviceObject.GetObject());
        }

        m_DeviceObject.SetObject(NULL);
    }

    //
    // Clean up any referenced objects
    //
    if (m_DeviceName.Buffer != NULL)
    {
        FxPoolFree(m_DeviceName.Buffer);
        RtlZeroMemory(&m_DeviceName, sizeof(m_DeviceName));
    }

    if (m_MofResourceName.Buffer != NULL)
    {
        FxPoolFree(m_MofResourceName.Buffer);
        RtlZeroMemory(&m_MofResourceName, sizeof(m_DeviceName));
    }
}

VOID
FxDevice::FinishInitializing(
    VOID
    )

/*++

Routine Description:

    This routine is called when the device is completely initialized.

Arguments:

    none.

Returns:

    none.

--*/

{
    m_DeviceObject.SetFlags( m_DeviceObject.GetFlags() & ~DO_DEVICE_INITIALIZING);
}
