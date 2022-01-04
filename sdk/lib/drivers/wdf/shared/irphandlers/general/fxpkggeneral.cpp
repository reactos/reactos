/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgGeneral.cpp

Abstract:

    This module implements the wmi package for the driver frameworks.

Author:




Environment:

    Both kernel and user mode

Revision History:

--*/

#if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
#define FX_IS_USER_MODE (TRUE)
#define FX_IS_KERNEL_MODE (FALSE)
#elif ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
#define FX_IS_USER_MODE (FALSE)
#define FX_IS_KERNEL_MODE (TRUE)
#endif

extern "C" {
#include "mx.h"
}
#include "fxmin.hpp"

extern "C" {
// #include "FxPkgGeneral.tmh"
}


FxPkgGeneral::FxPkgGeneral(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CfxDevice *Device
    ) :
    FxPackage(FxDriverGlobals, Device, FX_TYPE_PACKAGE_GENERAL)
{
    //
    // The count is biased to one and not zero for control device objects.  When
    // a control devobj is deleted, we will decrement this bias out so that we
    // know when the last handle has been closed and it is now safe to free the
    // FxDevice.
    //
    m_OpenHandleCount = 1;

    //
    // List of file object info.
    //
    InitializeListHead(&m_FileObjectInfoHeadList);

    m_Flags = 0;
    m_ExecutionLevel = WdfExecutionLevelInheritFromParent;
    m_SynchronizationScope = WdfSynchronizationScopeInheritFromParent;
    m_CallbackLockPtr      = NULL;
    m_CallbackLockObjectPtr = NULL;
    m_DriverCreatedQueue = NULL;
    m_DefaultQueueForCreates = NULL;
}

FxPkgGeneral::~FxPkgGeneral()
{
    PLIST_ENTRY next;

    ASSERT(m_OpenHandleCount <= 1);

    //
    // Delete the file object info list if present.
    //
    while (!IsListEmpty(&m_FileObjectInfoHeadList)) {
        next = RemoveHeadList(&m_FileObjectInfoHeadList);
        FxFileObjectInfo* info;
        info = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);
        InitializeListHead(next);
        delete info;
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::Initialize(
    __in PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:
    Initiliazes how the driver will handle fileobjects and their associated
    event callbacks (create, close, cleanup).

    Assumes that FileObjectAttributes has been validated by the caller.

Arguments:
    DeviceInit - Chain of device_init and cx_device_init with file object config.

Return Value:
    STATUS_SUCCESS or other NTSTATUS values.

  --*/
{
    NTSTATUS                    status;
    PLIST_ENTRY                 next;
    FxFileObjectInfo*           fileObjInfo;
    PFX_DRIVER_GLOBALS          fxDriverGlobals;

    fxDriverGlobals = GetDriverGlobals();

    //
    // Init file object info.
    //
    if (DeviceInit->FileObject.Set) {
        fileObjInfo = new(fxDriverGlobals) FxFileObjectInfo();
        if (fileObjInfo == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(fxDriverGlobals,
                                TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Couldn't create object FileObjectInfo, "
                                "%!STATUS!", status);
            goto Done;
        }

        fileObjInfo->ClassExtension = FALSE;
        fileObjInfo->FileObjectClass = DeviceInit->FileObject.Class;
        fileObjInfo->Attributes = DeviceInit->FileObject.Attributes;

        fileObjInfo->AutoForwardCleanupClose =
            DeviceInit->FileObject.AutoForwardCleanupClose;

        fileObjInfo->EvtFileCreate.Method =
            DeviceInit->FileObject.Callbacks.EvtDeviceFileCreate;

        fileObjInfo->EvtFileCleanup.Method =
            DeviceInit->FileObject.Callbacks.EvtFileCleanup;

        fileObjInfo->EvtFileClose.Method =
            DeviceInit->FileObject.Callbacks.EvtFileClose;

        InsertTailList(&m_FileObjectInfoHeadList, &fileObjInfo->ListEntry);

        m_Flags |= FX_PKG_GENERAL_FLAG_CLIENT_INFO;

        if (fileObjInfo->EvtFileCreate.Method != NULL) {
            m_Flags |= FX_PKG_GENERAL_FLAG_CLIENT_CREATE;
        }
    }

    //
    // Build file object info chain for any class extension.
    //
    for (next = DeviceInit->CxDeviceInitListHead.Flink;
         next != &DeviceInit->CxDeviceInitListHead;
         next = next->Flink) {

        PWDFCXDEVICE_INIT cxInit;

        cxInit = CONTAINING_RECORD(next, WDFCXDEVICE_INIT, ListEntry);

        if (cxInit->FileObject.Set == FALSE) {
            continue;
        }

        fileObjInfo = new(fxDriverGlobals) FxFileObjectInfo();
        if (fileObjInfo == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(fxDriverGlobals,
                                TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Couldn't create object FileObjectInfo, "
                                "%!STATUS!", status);
            goto Done;
        }

        fileObjInfo->ClassExtension = TRUE;
        fileObjInfo->FileObjectClass = cxInit->FileObject.Class;
        fileObjInfo->Attributes = cxInit->FileObject.Attributes;

        fileObjInfo->AutoForwardCleanupClose =
            cxInit->FileObject.AutoForwardCleanupClose;

        fileObjInfo->EvtCxFileCreate.Method =
            cxInit->FileObject.Callbacks.EvtCxDeviceFileCreate;

        fileObjInfo->EvtFileCleanup.Method =
            cxInit->FileObject.Callbacks.EvtFileCleanup;

        fileObjInfo->EvtFileClose.Method =
            cxInit->FileObject.Callbacks.EvtFileClose;

        fileObjInfo->CxDeviceInfo = cxInit->CxDeviceInfo;

        InsertTailList(&m_FileObjectInfoHeadList, &fileObjInfo->ListEntry);

        m_Flags |= FX_PKG_GENERAL_FLAG_CX_INFO;

        if (fileObjInfo->EvtCxFileCreate.Method != NULL) {
            m_Flags |= FX_PKG_GENERAL_FLAG_CX_CREATE;
        }
    }

    //
    // Nothing to do if list if empty.
    //
    if (IsListEmpty(&m_FileObjectInfoHeadList)) {
        status = STATUS_SUCCESS;
        goto Done;
    }

    //
    // We will enable this once the unlocking model is figured out.
    // It's not okay to sent request downstack with the presentation lock held.
    //
    status = ConfigureConstraints(&m_FileObjectInfoHeadList);
    if(!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Configure file object class.
    //
    status = ConfigureFileObjectClass(&m_FileObjectInfoHeadList);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    status = STATUS_SUCCESS;

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::ConfigureConstraints(
    __in PLIST_ENTRY FileObjInfoList
    )
/*++

Routine Description:

  Configure the callback synchronization according to the configuration supplied by the
  client and class extension device driver.
  It is a requirement for this driver chain (CXs and client driver) to use the same settings.

Arguments:
    FileObjInfoList - List of FxFileObjectInfo structs.

Returns:
    NTSTATUS

--*/
{
    WDF_EXECUTION_LEVEL         execLevel, parentExecLevel;
    WDF_SYNCHRONIZATION_SCOPE   synchScope, parentSynchScope;
    BOOLEAN                     automaticLockingRequired;
    PLIST_ENTRY                 next;
    FxFileObjectInfo           *fileObjInfo;
    NTSTATUS                    status;
    PFX_DRIVER_GLOBALS          fxDriverGlobals;

    automaticLockingRequired = FALSE;
    fxDriverGlobals = GetDriverGlobals();

    ASSERT(!IsListEmpty(FileObjInfoList));

    //
    // Get parent values.
    //
    m_Device->GetConstraints(&parentExecLevel, &parentSynchScope);

    //
    // Default constraints settings when driver uses WDF_NO_OBJECT_ATTRIBUTES:
    //
    // v1.9 and below:
    //     WdfExecutionLevelDispatch and WdfSynchronizationScopeNone
    //
    // v1.11 and above:
    //     WdfExecutionLevelPassive and WdfSynchronizationScopeNone
    //
    // In v1.9 and below if driver used WDF_NO_OBJECT_ATTRIBUTES for
    // the file object's attributes, the synchronization scope and execution
    // level were left uninitialized (i.e., zero), which means that WDF
    // defaulted to WdfSynchronizationScopeInvalid and WdfExecutionLevelInvalid,
    // WDF interpreted these values as no_passive and no_synchronization.
    //
    // This default execution level is used when disposing the device's
    // general package object and file object object.
    // Independently of these settings WDF guarantees that Create,
    // Cleanup and Close callbacks are always called at passive level.
    //
    m_ExecutionLevel = fxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11) ?
                        WdfExecutionLevelPassive :
                        WdfExecutionLevelDispatch;

    m_SynchronizationScope = WdfSynchronizationScopeNone;

    //
    // Make sure file object info chain follow these constrains:
    //  Cx's synch scope: none
    //
    for (next = FileObjInfoList->Blink;
         next != FileObjInfoList;
         next = next->Blink) {

        fileObjInfo = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);

        //
        // Size is zero if driver didn't specify any attributes.
        //
        if (0 == fileObjInfo->Attributes.Size) {
            continue;
        }

        //
        // Execution level checks.
        //
        execLevel = fileObjInfo->Attributes.ExecutionLevel;
        if (WdfExecutionLevelInheritFromParent == execLevel) {
            execLevel = parentExecLevel;
        }

        //
        // Passive level wins over DPC level.
        //
        if (WdfExecutionLevelPassive == execLevel) {
            m_ExecutionLevel = WdfExecutionLevelPassive;
        }

        //
        // Synchronization scope checks.
        //
        synchScope = fileObjInfo->Attributes.SynchronizationScope;
        if (WdfSynchronizationScopeInheritFromParent == synchScope) {
            synchScope = parentSynchScope;
        }

        //
        // Make sure the Cx's synch scope is none.
        //
        if (fileObjInfo->ClassExtension) {
            if (synchScope != WdfSynchronizationScopeNone) {
                status = STATUS_INVALID_DEVICE_REQUEST;
                DoTraceLevelMessage(
                    fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Driver 0x%p - Device 0x%p - synchronization scope: "
                    "%!WDF_SYNCHRONIZATION_SCOPE! should be"
                    "WdfSynchronizationScopeNone, %!STATUS!",
                    m_Device->GetCxDriver(fileObjInfo->CxDeviceInfo)->GetHandle(),
                    m_Device->GetHandle(),
                    synchScope,
                    status
                    );

                FxVerifierDbgBreakPoint(fxDriverGlobals);
                goto Done;
            }
        }
        else {
            //
            // Always use client's synch scope for file object.
            //
            m_SynchronizationScope = synchScope;
        }
    }

    if(m_SynchronizationScope == WdfSynchronizationScopeQueue) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WdfSynchronizationScopeQueue is not allowed on "
            "FileObject, %!STATUS!",
            status);
        goto Done;
    }

    if (m_ExecutionLevel == WdfExecutionLevelPassive) {

        //
        // Mark FxObject as passive level to ensure that Dispose and Destroy
        // callbacks are passive to the driver
        //
        MarkPassiveCallbacks(ObjectDoNotLock);
        //
        // We aren't going to use a workitem to  defer the invocation of fileevents
        // to passive-level if the caller is at DISPATCH_LEVEL because we wouldn't
        // be able to guarantee the caller's context for fileevents. It's up to the
        // driver writer to ensure that the layer above doesn't send create requests
        // at dispatch-level.
        //
    }

    if(m_SynchronizationScope == WdfSynchronizationScopeNone) {
        status = STATUS_SUCCESS;
        goto Done;
    }

    if(m_SynchronizationScope == WdfSynchronizationScopeDevice) {
        //
        // Since FileEvents can be invoked only at passive-level, we check the
        // parent executionlevel to see if it's set to passive. If not, we return an error
        // because we can't use the presentation lock of the device.
        //
        if(parentExecLevel != WdfExecutionLevelPassive) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WdfSynchronizationScopeDevice or "
                "WdfSynchronizationScopeInheritFromParent "
                "allowed only if the parent WDFDEVICE 0x%p, "
                "ExecutionLevel is passive, %!STATUS!",
                m_Device->GetHandle(), status);
            goto Done;
        }

        m_CallbackLockPtr = m_Device->GetCallbackLockPtr(&m_CallbackLockObjectPtr);
        automaticLockingRequired = TRUE;
    }

    //
    // Set lock constraint in client's callbacks object only.
    //
    if (automaticLockingRequired) {
        if (!IsListEmpty(FileObjInfoList)) {
            fileObjInfo = CONTAINING_RECORD(FileObjInfoList->Flink,
                                            FxFileObjectInfo,
                                            ListEntry);

            if (FALSE == fileObjInfo->ClassExtension) {
                fileObjInfo->EvtFileCreate.SetCallbackLockPtr(m_CallbackLockPtr);
                fileObjInfo->EvtFileCleanup.SetCallbackLockPtr(m_CallbackLockPtr);
                fileObjInfo->EvtFileClose.SetCallbackLockPtr(m_CallbackLockPtr);
            }
        }
    }

    status = STATUS_SUCCESS;

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::ConfigureFileObjectClass(
    __in PLIST_ENTRY FileObjInfoList
    )
/*++

Routine Description:

  Configure the file object class for this device.

  These are the possible class settings:

    WdfFileObjectNotRequired
    WdfFileObjectWdfCanUseFsContext*
    WdfFileObjectWdfCanUseFsContext2*,
    WdfFileObjectWdfCannotUseFsContexts*

    * these can also be combined with WdfFileObjectCanBeOptional flag.

  Logic:

    - default: not_required.
    - if cx/driver selects not_required, skip it.
    - if cx/driver selects !not_required, than
        . if everyone agrees on the setting, use that setting.
        . else use cannot_use_fs_contexts.

Arguments:
    FileObjInfoList - List of FxFileObjectInfo structs.

Returns:
    NTSTATUS

--*/
{
    PLIST_ENTRY                 next;
    FxFileObjectInfo*           fileObjInfo;
    NTSTATUS                    status;
    PFX_DRIVER_GLOBALS          fxDriverGlobals;
    WDF_FILEOBJECT_CLASS        fileObjClass;
    FxCxDeviceInfo*             previousCxInfo;

    fxDriverGlobals = GetDriverGlobals();
    fileObjClass = WdfFileObjectNotRequired;
    previousCxInfo = NULL;

    ASSERT(!IsListEmpty(FileObjInfoList));

    //
    // Compute the execution level and synchronization scope for all the chain.
    //
    for (next = FileObjInfoList->Blink;
         next != FileObjInfoList;
         next = next->Blink) {

        fileObjInfo = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);

        //
        // If not required, skip it.
        //
        if (WdfFileObjectNotRequired == fileObjInfo->FileObjectClass) {
            continue;
        }

        //
        // If the same, skip it.
        //
        if (fileObjClass == fileObjInfo->FileObjectClass) {
            continue;
        }

        //
        // If not set yet, use new value.
        //
        if (WdfFileObjectNotRequired == fileObjClass) {
            fileObjClass = fileObjInfo->FileObjectClass;
            previousCxInfo = fileObjInfo->CxDeviceInfo;
            continue;
        }

        //
        // Make sure optional flag is compatible.
        //
        if (FxIsFileObjectOptional(fileObjClass) !=
            FxIsFileObjectOptional(fileObjInfo->FileObjectClass)) {

            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                fxDriverGlobals,
                TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Device 0x%p - "
                "Driver 0x%p - WdfFileObjectCanBeOptional (%d) is not "
                "compatible with wdf extension "
                "Driver 0x%p - WdfFileObjectCanBeOptional (%d), %!STATUS!",
                m_Device->GetHandle(),
                m_Device->GetCxDriver(fileObjInfo->CxDeviceInfo)->GetHandle(),
                FxIsFileObjectOptional(fileObjInfo->FileObjectClass) ? 1:0,
                previousCxInfo->Driver->GetHandle(),
                FxIsFileObjectOptional(fileObjClass) ? 1:0,
                status
                );

            FxVerifierDbgBreakPoint(fxDriverGlobals);
            goto Done;
        }

        //
        // Drivers do not agree on the location, set cannot use fx contexts.
        //
        fileObjClass = WdfFileObjectWdfCannotUseFsContexts;
        if (FxIsFileObjectOptional(fileObjInfo->FileObjectClass)) {
            fileObjClass = (WDF_FILEOBJECT_CLASS)
                ((ULONG)fileObjClass |  WdfFileObjectCanBeOptional);
        }

        DoTraceLevelMessage(
            fxDriverGlobals,
            TRACE_LEVEL_INFORMATION, TRACINGDEVICE,
            "Converting file object class for Driver 0x%p - Device 0x%p, "
            "from 0x%x to 0x%x",
            m_Device->GetCxDriver(fileObjInfo->CxDeviceInfo)->GetHandle(),
            m_Device->GetHandle(),
            fileObjInfo->FileObjectClass,
            fileObjClass
            );
    }

    //
    // Set the file object support level on the FxDevice
    //
    m_Device->SetFileObjectClass(fileObjClass);

    status = STATUS_SUCCESS;

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::PostCreateDeviceInitialize(
    __in PWDFDEVICE_INIT Init
    )
/*++

Routine Description:
    Optionally registers a shutdown and last chance shutdown notification on
    behalf of the device.

Arguments:
    Init - Initialization structure which will indicate if registration is required

Return Value:
    NTSTATUS

  --*/
{
    MdDeviceObject pDevice;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES attributes;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();
    status = STATUS_SUCCESS;




#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    if (Init->Control.Flags != 0) {
        pDevice = m_Device->GetDeviceObject();

        if (Init->Control.Flags & WdfDeviceShutdown) {
            status = IoRegisterShutdownNotification(pDevice);
        }

        if (NT_SUCCESS(status) &&
            (Init->Control.Flags & WdfDeviceLastChanceShutdown)) {
            status = IoRegisterLastChanceShutdownNotification(pDevice);

        }

        if (NT_SUCCESS(status)) {
            //
            // IoDeleteDevice will automatically unregister the shutdown
            // notifications if the device is deleted before the machine is
            // shutdown, so we don't need to track registration beyond this point.
            //
            m_EvtDeviceShutdown.m_Method = Init->Control.ShutdownNotification;
        }
        else {
            //
            // This unregisters both the normal and last chance notifications
            //
            IoUnregisterShutdownNotification(pDevice);
        }
    }
#else
    UNREFERENCED_PARAMETER(Init);
    UNREFERENCED_PARAMETER(pDevice);
#endif

    if (NT_SUCCESS(status) && (m_Flags & FX_PKG_GENERAL_FLAG_CREATE)) {
        //
        // Create an internal queue to track create requests presented to the driver.
        // This special queue is used so that we can invoke the events in the context
        // of the caller.
        //
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

        //
        // non power managed queue because we don't anticipate drivers touching
        // hardware in the fileevent callbacks. If they do, then they should make sure
        // to power up the device.
        //
        queueConfig.PowerManaged = WdfFalse;

        //
        // Queue inherits the sync & exec level of fileobject.
        //
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ExecutionLevel = m_ExecutionLevel;
        attributes.SynchronizationScope = m_SynchronizationScope;

        status = m_Device->m_PkgIo->CreateQueue(&queueConfig,
                                                &attributes,
                                                NULL,
                                                &m_DefaultQueueForCreates);

        if (!NT_SUCCESS(status)) {
           DoTraceLevelMessage(
               pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
               "Unable to create an internal queue for creates for WDFDEVICE "
               "0x%p, %!STATUS!", m_Device->GetHandle(), status);
           return status;
        }
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::ConfigureForwarding(
    __in FxIoQueue* TargetQueue
    )
/*++

Routine Description:

    Used to register driver specified for dispatching create requests.

Arguments:

Return Value:

    NTSTATUS

  --*/
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    KIRQL irql;

    status = STATUS_SUCCESS;
    pFxDriverGlobals = GetDriverGlobals();

    if(TargetQueue->IsIoEventHandlerRegistered(WdfRequestTypeCreate) == FALSE){
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "Must have EvtIoDefault registered to receive "
                "WdfRequestTypeCreate requests for WDFQUEUE 0x%p, "
                "%!STATUS!", TargetQueue->GetObjectHandle(), status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    Lock(&irql);

    if (m_DriverCreatedQueue) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "Another WDFQUEUE 0x%p is already configured for auto dispatching "
            "create request, %!STATUS!",
            m_DriverCreatedQueue->GetObjectHandle(), status);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
    else {
        m_DriverCreatedQueue = TargetQueue;
    }

    Unlock(irql);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::Dispatch(
    __inout MdIrp Irp
    )
/*++

Routine Description:

    Dispatch routine for handling create, cleanup, close, and shutdown requests.

Arguments:


Return Value:

    NTSTATUS

  --*/
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    FxIrp fxIrp(Irp);

    FX_TRACK_DRIVER(pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "WDFDEVICE 0x%p !devobj 0x%p %!IRPMJ! IRP 0x%p",
                        m_Device->GetHandle(), m_Device->GetDeviceObject(),
                        fxIrp.GetMajorFunction(), Irp);

    switch (fxIrp.GetMajorFunction()) {
    case IRP_MJ_CREATE:
        status = OnCreate(&fxIrp);
        break;

    case IRP_MJ_CLOSE:
        status = OnClose(&fxIrp);
        break;

    case IRP_MJ_CLEANUP:
        status = OnCleanup(&fxIrp);
        break;

    case IRP_MJ_SHUTDOWN:
        status = OnShutdown(&fxIrp);
        break;

    default:
        ASSERT(FALSE);
        status = STATUS_NOT_SUPPORTED;
        fxIrp.SetStatus(status);
        fxIrp.CompleteRequest(IO_NO_INCREMENT);
        break;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::OnCreate(
    __inout FxIrp* FxIrp
    )
/*++

Routine Description:

    1) Allow only one handle to be open for exclusive device.
    2) Create a WDFFILEOBJECT to represent the WDM fileobject. This
        fileobject is created only if the driver registers for EvtFile events and
        doesn't specify WdfFileObjectNotRequired. If the file-events are not
        registered, the default FileobjectClass is WdfFileObjectNotRequired - set
        during DeviceInit.
    3) If the EvtFileCreate event is *not* set or any driver queue is not configured,
        then complete or forward the request to the lower
        driver depending on AutoForwardCleanupClose. AutoForwardCleanupClose
        is set to TRUE by default for filter drivers.
    4) Create a FxRequest.
    5) First try to dispatch it to a driver specified queue.
    6) If there is no driver specified queue, then check to see if the driver has
         registered EvtDeviceFileCreate event.
    7) If EvtFileCreate is set then dispatch the request to to an internal
        manual queue, retrieve the request by fileobject and present the request to
        the driver in the EvtFileCreate event.  This allow the driver to forward the request
        to another queue to mark the request cancelable and complete it later.

Arguments:

Return Value:

    NTSTATUS

 --*/
{
    NTSTATUS                    status;
    FxFileObject*               pFxFO ;
    WDFFILEOBJECT               hwdfFO;
    FxRequest *                 pRequest;
    PFX_DRIVER_GLOBALS          pFxDriverGlobals;
    MdFileObject                fileObject;
    LONG                        count;
    BOOLEAN                     inCriticalRegion;
    BOOLEAN                     inDefaultQueue;
    FxFileObjectInfo*           fileObjInfo;
    WDF_OBJECT_ATTRIBUTES       attributes;
    PLIST_ENTRY                 next;

    pFxFO = NULL;
    hwdfFO = NULL;
    pRequest = NULL;
    inCriticalRegion = FALSE;
    inDefaultQueue = FALSE;
    pFxDriverGlobals = GetDriverGlobals();
    fileObject = FxIrp->GetFileObject();
    fileObjInfo = NULL;

    //
    // Check for exclusivity.
    //
    count = InterlockedIncrement(&m_OpenHandleCount);

    //
    // The count is biased by one to help track when to delete the control
    // device, so we need to check for 2, not 1.
    //
    if (m_Device->IsExclusive() && count > 2) {
       DoTraceLevelMessage(
           pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
           "Exclusive WDFDEVICE 0x%p, only one open handle is allowed",
           m_Device->GetHandle());
       status =  STATUS_ACCESS_DENIED;
       goto Error;
    }

    // ------------------------------------------------------------------------
    //
    // Create WDFFILEOBJECT. By default we allocate the root driver's file obj
    // context; then we attach the other file obj contexts.
    //

    //
    // Init the file obj's attributes. Use default if not present (legacy behavior).
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    if (!IsListEmpty(&m_FileObjectInfoHeadList)) {
        //
        // file obj info is set, use the top most driver's info, i.e., cx if present.
        //
        fileObjInfo = CONTAINING_RECORD(m_FileObjectInfoHeadList.Blink,
                                        FxFileObjectInfo,
                                        ListEntry);

        //
        // Use this layer's attributes if present.
        // Size is zero if driver didn't specify file object's attributes.
        //
        if (0 != fileObjInfo->Attributes.Size) {
            ASSERT(fileObjInfo->Attributes.Size == sizeof(WDF_OBJECT_ATTRIBUTES));
            attributes = fileObjInfo->Attributes;
        }

        //
        // Use computed constraint settings.
        //
        attributes.ExecutionLevel = m_ExecutionLevel;
        attributes.SynchronizationScope = m_SynchronizationScope;
    }

    //
    // Create the file object.
    //
    status = FxFileObject::_CreateFileObject(
        m_Device,
        FxIrp->GetIrp(),
        m_Device->GetFileObjectClass(),
        &attributes,
        fileObject,
        &pFxFO
        );

    if (!NT_SUCCESS(status) ) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "Could not create WDFFILEOBJECT for WDFDEVICE 0x%p, failing "
            "IRP_MJ_CREATE %!STATUS!", m_Device->GetHandle(), status);
        goto Error;
    }

    if (pFxFO != NULL) {
        hwdfFO = pFxFO->GetHandle();

        //
        // If any, attach the file obj's contexts of the other drivers in this chain.
        //
        for (next = m_FileObjectInfoHeadList.Blink->Blink; // skip one.
             next != &m_FileObjectInfoHeadList;
             next = next->Blink) {

            fileObjInfo = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);

            attributes = fileObjInfo->Attributes;

            //
            // Size is zero if driver didn't specify file object's attributes.
            //
            if (0 == attributes.Size) {
                continue;
            }

            //
            // Don't need these settings for extra contexts.
            //
            attributes.ExecutionLevel = WdfExecutionLevelInheritFromParent;
            attributes.SynchronizationScope = WdfSynchronizationScopeInheritFromParent;
            attributes.ParentObject = NULL;

            status = FxObjectAllocateContext(pFxFO,
                                             &attributes,
                                             TRUE,
                                             NULL);
            if(!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                    "Couldn't allocate file object context 0x%p for "
                    "device 0x%p - driver 0x%p, %!STATUS!",
                    &fileObjInfo->Attributes,
                    m_Device->GetHandle(),
                    m_Device->GetCxDriver(fileObjInfo->CxDeviceInfo)->GetHandle(),
                    status
                    );

                goto Error;
            }
        }
    }

    // ------------------------------------------------------------------------
    //
    // If there is no driver configured queue or m_EvtFileCreate is not registered,
    // complete the request with status-success. The reason for making this
    // check after creating the fileobject is to  allow WdfRequestGetFileObject even
    // if the driver hasn't registered any file-event callbacks.
    //
    if (m_DriverCreatedQueue == NULL &&
        (m_Flags & FX_PKG_GENERAL_FLAG_CREATE) == 0) {

        //
        // Check to see if the driver has opted to autoforward cleanup and close.
        // If so, we should forward create requests also. Else, we should
        // complete the request with STATUS_SUCCESS. Note, if the driver is
        // a filter driver, the default value of m_AutoForwardCleanupClose is TRUE.
        //
        //
        if (m_Device->m_AutoForwardCleanupClose) {
            status = ForwardCreateRequest(FxIrp, _CreateCompletionRoutine, this);
            //
            // _CreateCompletionRoutine will do the cleanup when the request is
            // completed with error status by the lower driver.
            //
        }
        else {
            status = STATUS_SUCCESS;
            FxIrp->SetStatus(status);
            FxIrp->SetInformation(0);
            FxIrp->CompleteRequest(IO_NO_INCREMENT);
        }

       goto RequestIsGone;
    }

    // ------------------------------------------------------------------------
    //
    // Create a FxRequest for this IRP. By default we allocate the top most driver's request
    // context; then we attach the other request contexts.
    //

    //
    // Init the request's attributes.
    //
    if (!IsListEmpty(&m_FileObjectInfoHeadList)) {
        //
        // file obj info is set, use the top most driver's info, i.e., cx if present.
        //
        fileObjInfo = CONTAINING_RECORD(m_FileObjectInfoHeadList.Blink,
                                        FxFileObjectInfo,
                                        ListEntry);
        if (fileObjInfo->ClassExtension) {
            attributes = fileObjInfo->CxDeviceInfo->RequestAttributes;
        }
        else {
            attributes = *m_Device->GetRequestAttributes();
        }
    }
    else {
        attributes = *m_Device->GetRequestAttributes();
    }

    if (m_Device->IsCxInIoPath()) {
        //
        // Apply cx's constrains for create requests:
        //
        attributes.ExecutionLevel = WdfExecutionLevelDispatch;
        attributes.SynchronizationScope = WdfSynchronizationScopeNone;
        attributes.ParentObject = NULL;
    }

    //
    // Create the request.
    //
    status = FxRequest::_CreateForPackage(m_Device,
                                          &attributes,
                                          FxIrp->GetIrp(),
                                          &pRequest);
    if(!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Could not create request for WDFDEVICE 0x%p, %!STATUS!",
                            m_Device->GetHandle(), status);
        goto Error;
    }

    //
    // If any, attach the request's contexts of the other drivers in this chain.
    //
    for (next = m_FileObjectInfoHeadList.Blink->Blink; // skip one.
         next != &m_FileObjectInfoHeadList;
         next = next->Blink) {

        fileObjInfo = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);

        if (fileObjInfo->ClassExtension) {
            attributes = fileObjInfo->CxDeviceInfo->RequestAttributes;
        }
        else {
            attributes = *m_Device->GetRequestAttributes();
        }

        //
        // Size is zero if driver didn't specify request's attributes.
        //
        if (0 == attributes.Size) {
            continue;
        }

        //
        // Don't need these settings for extra contexts.
        //
        attributes.ExecutionLevel = WdfExecutionLevelInheritFromParent;
        attributes.SynchronizationScope = WdfSynchronizationScopeInheritFromParent;
        attributes.ParentObject = NULL;

        status = FxObjectAllocateContext(
                            pRequest,
                            &attributes,
                            TRUE,
                            NULL);

        if(!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "Couldn't allocate request context for "
                "device 0x%p - driver 0x%p, %!STATUS!",
                m_Device->GetHandle(),
                m_Device->GetCxDriver(fileObjInfo->CxDeviceInfo)->GetHandle(),
                status
                );

            goto Error;
        }
    }

    //
    // Disable thread suspension beyond this point by entering critical region.
    //
    if (Mx::MxGetCurrentIrql() <= APC_LEVEL)
    {
        Mx::MxEnterCriticalRegion();
        inCriticalRegion = TRUE;
    }

    // ------------------------------------------------------------------------
    //
    // Queue request in default queue before invoking cx or client's create callbacks.
    //
    if ((m_Flags & FX_PKG_GENERAL_FLAG_CX_CREATE) ||
        m_DriverCreatedQueue == NULL) {

        FxRequest* outputRequest;

        ASSERT(m_Flags & FX_PKG_GENERAL_FLAG_CREATE);

        //
        // Make sure we are calling FileEvents at the right IRQL level.
        //
        if (m_ExecutionLevel ==  WdfExecutionLevelPassive &&
            Mx::MxGetCurrentIrql() >= DISPATCH_LEVEL) {

            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "WDFDEVICE 0x%p cannot handle create request at or above "
                "dispatch-level, fail the Irp: 0x%p, %!STATUS!",
                m_Device->GetObjectHandle(), FxIrp->GetIrp(), status);

            goto Error;
        }

        //
        // Now queue and immediately retrieve the request by FileObject.
        // QueueRequest will return an error if the queue is not accepting request
        // or the request is already cancelled. Either way, the request is completed by
        // the FxIoQueue.
        //
        status = m_DefaultQueueForCreates->QueueRequest(pRequest);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                    "Couldn't forward request to the WDFQUEUE 0x%p, %!STATUS!",
                    m_DefaultQueueForCreates->GetObjectHandle(), status);
            goto RequestIsGone;
        }

        status = m_DefaultQueueForCreates->GetRequest(fileObject,
                                                      NULL,
                                                      &outputRequest);
        if (!NT_SUCCESS(status)) {
            //
            // Oops, request got cancelled and completed by another thread.
            //
            ASSERT(status == STATUS_NO_MORE_ENTRIES);
            status = STATUS_PENDING; // IRP was already marked pending.
            goto RequestIsGone;
        }

        ASSERT(outputRequest == pRequest);

        inDefaultQueue = TRUE;
    }

    // ------------------------------------------------------------------------
    //
    // Invoke Cx's create callbacks.  Here we add the cx's file obj and request contexts.
    //
    if (m_Flags & FX_PKG_GENERAL_FLAG_CX_CREATE) {

        //
        // Loop through all cx's file obj info.
        //
        for (next = m_FileObjectInfoHeadList.Blink;
             next != &m_FileObjectInfoHeadList;
             next = next->Blink) {

            //
            // Get ready to invoke next layer cx's create.
            //
            fileObjInfo = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);

            //
            // Do not invoke the client driver's create callback (if any). For compatibility
            // we need to check first the driver 'create' queue.
            //
            if (FALSE == fileObjInfo->ClassExtension) {
                break;
            }

            ASSERT(fileObjInfo->EvtFileCreate.Method == NULL);
            ASSERT(fileObjInfo->EvtCxFileCreate.Method != NULL);

            //
            // Keep track where we stopped (end, not inclusive node).
            // Note that we cannot do this after the Invoke b/c File Object
            // may be gone by the time callback returns.
            //
            if (pFxFO) {
                pFxFO->SetPkgCleanupCloseContext(next->Blink);
            }

            //
            // Invoke knows how to handle NULL callbacks.
            //
            if (fileObjInfo->EvtCxFileCreate.Invoke(
                                m_Device->GetHandle(),
                                (WDFREQUEST)pRequest->GetObjectHandle(),
                                hwdfFO)) {
                //
                // Callback claimed the request.
                //
                status = STATUS_PENDING; // IRP was already marked pending.
                goto RequestIsGone;
            }
        }
    }

    //-------------------------------------------------------------------------
    //
    // First check for driver configured queue. If there is one, dispatch the request
    // to that queue.
    //
    if (m_DriverCreatedQueue != NULL) {
        if (inDefaultQueue) {
            ASSERT(m_Flags & FX_PKG_GENERAL_FLAG_CX_INFO);

            status = m_DefaultQueueForCreates->ForwardRequest(
                                m_DriverCreatedQueue,
                                pRequest);

            if(!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                    "Couldn't forward request to the WDFQUEUE 0x%p, %!STATUS!",
                    m_DriverCreatedQueue->GetObjectHandle(), status);

                pRequest->Complete(status);
            }

            status = STATUS_PENDING; // IRP was already marked pending.
            goto RequestIsGone;
        }
        else {
            ASSERT(pRequest->GetRefCnt() == 1);
            //
            // QueueRequest will return an error if the queue is not accepting request
            // or the request is already cancelled. Either way, the request is completed by
            // the FxIoQueue.
            //
            status = m_DriverCreatedQueue->QueueRequest(pRequest);
            if(!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                    "Couldn't forward request to the WDFQUEUE 0x%p, %!STATUS!",
                    m_DriverCreatedQueue->GetObjectHandle(), status);
            }

            goto RequestIsGone;
        }
    }

    //-------------------------------------------------------------------------
    //
    // At this point invoke the client driver callback if present.
    //
    if (m_Flags & FX_PKG_GENERAL_FLAG_CLIENT_CREATE) {
        ASSERT(m_Flags & FX_PKG_GENERAL_FLAG_CLIENT_INFO);
        ASSERT(TRUE == inDefaultQueue);
        ASSERT(fileObjInfo->EvtFileCreate.Method != NULL);
        ASSERT(fileObjInfo->EvtCxFileCreate.Method == NULL);

        //
        // Invoke the client driver create requests.
        //
        fileObjInfo->EvtFileCreate.Invoke(
                            m_Device->GetHandle(),
                            (WDFREQUEST)pRequest->GetObjectHandle(),
                            hwdfFO);
        //
        // QueueRequest has already marked the request pending.
        //
        status = STATUS_PENDING;
        goto RequestIsGone;
    }

    //
    // We should be here only if CX's create returned 'continue' but client didn't have
    // a create callback.
    //
    ASSERT(m_Flags & FX_PKG_GENERAL_FLAG_CX_INFO);

    //
    // Check to see if the driver has opted to autoforward cleanup and close.
    // If so, we should forward create requests to lower drivers. Else, we should
    // complete the request with STATUS_SUCCESS. Note, if the driver is
    // a filter driver, the default value of m_AutoForwardCleanupClose is TRUE.
    //
    if (m_Device->m_AutoForwardCleanupClose) {
        (void)ForwardCreateRequest(FxIrp, _CreateCompletionRoutine2, pRequest);
        //
        // _CreateCompletionRoutine2 will complete the WDF request.
        //
    }
    else {
        pRequest->Complete(STATUS_SUCCESS);
    }

    //
    // Done processing this request.
    //
    status = STATUS_PENDING; // IRP was already marked pending.
    goto RequestIsGone;

Error:
    if (pRequest != NULL) {
        pRequest->DeleteFromFailedCreate();
        pRequest = NULL;
    }

    ASSERT(!NT_SUCCESS(status));

    if (pFxFO != NULL) {
        pFxFO->DeleteFileObjectFromFailedCreate();
        pFxFO = NULL;
    }

    //
    // NOTE:  after this call, this object may have been deleted!
    //
    DecrementOpenHandleCount();

    FxIrp->SetStatus(status);
    FxIrp->SetInformation(0);
    FxIrp->CompleteRequest(IO_NO_INCREMENT);

    // fallthrough

RequestIsGone:

    //
    // We have lost the ownership of the request. We have either successfully
    // presented the request to the driver or the queue function we called to
    // present the request returned error but completed the FxRequest on its own.
    // Either way we don't need to worry about cleaning up the resources
    // (fileobject, handle-count, etc) because the FxRequest:Completion routine
    // will call FxPkgGeneral::CreateCompleted to post process IRP upon completion.
    //
    if (inCriticalRegion) {
        Mx::MxLeaveCriticalRegion();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::ForwardCreateRequest(
    __in FxIrp* Irp,
    __in MdCompletionRoutine CompletionRoutine,
    __in PVOID  Context
    )
{
    NTSTATUS status;

    Irp->CopyCurrentIrpStackLocationToNext();
    Irp->SetCompletionRoutineEx(m_Device->GetDeviceObject(),
                                 CompletionRoutine,
                                 Context);
    status = Irp->CallDriver(m_Device->GetAttachedDevice());

    return status;
}

VOID
FxPkgGeneral::CreateCompleted(
    __in FxIrp *Irp
    )
/*++

Routine Description:

    This method is called when the WDFREQUEST for Irp
    is completed either by the driver or framework.

    Here, we check the completion status of the IRP and if
    it's not success, we destroy the fileobject and decrement
    the openhandle count on the device.

Arguments:

Return Value:

    VOID

 --*/
{
    NTSTATUS status = Irp->GetStatus();

    //
    // If the create is completed with error status,
    // we destroy the WDFFILEOBJECT since the IoMgr will destroy
    // the PFILE_OBJECT if the IRP_MJ_CREATE fails and wouldn't send
    // Cleanup or Close IRP.
    //
    if (!NT_SUCCESS(status)) {

        // Now destroy the WDFFILEOBJECT
        FxFileObject::_DestroyFileObject(
            m_Device,
            m_Device->GetFileObjectClass(),
            Irp->GetFileObject()
            );

        DecrementOpenHandleCount();
    }
}

_Must_inspect_result_
NTSTATUS
STDCALL
FxPkgGeneral::_CreateCompletionRoutine(
    __in MdDeviceObject DeviceObject,
    __in MdIrp           OriginalIrp,
    __in_opt PVOID      Context
    )
/*++

Routine Description:

    This completion routine is set only when the IRP is forwarded
    directly by the framework to the lower driver. Framework forwards
    the IRP request if:
    1) The driver happens to be a filter and hasn't registered any
       callbacks to handle create request.
    2) The driver is not a filter but has explicitly requested the
       framework to autoforward create/cleanup/close requests down.

    We need to intercept the create in the completion path to find
    out whether the lower driver has succeeded or failed the request.
    If the request is failed, we should inform the package so that
    it can cleanup the state because I/O manager wouldn't send
    cleanup & close requests if the create is failed.

Arguments:

Return Value:

    NTSTATUS

 --*/
{
    FxPkgGeneral* pFxPkgGeneral;
    FxIrp irp(OriginalIrp);

    UNREFERENCED_PARAMETER(DeviceObject);

    pFxPkgGeneral = (FxPkgGeneral*) Context;

    ASSERT(pFxPkgGeneral != NULL);

    //
    // Let the package know that create is completed
    // so that it can cleanup the state.
    //
    pFxPkgGeneral->CreateCompleted(&irp);

    //
    // Let the irp continue on its way.
    //
    irp.PropagatePendingReturned();

    return irp.GetStatus();
}

_Must_inspect_result_
NTSTATUS
STDCALL
FxPkgGeneral::_CreateCompletionRoutine2(
    __in MdDeviceObject DeviceObject,
    __in MdIrp           OriginalIrp,
    __in_opt PVOID      Context
    )
/*++

Routine Description:

    Routine Description:

        This completion routine is set only when the create request is forwarded by the
        framework to the lower driver. Framework forwards the create request using this
        completion routine if:

        1) Class extension's create callback is set by did not claim the request.
        2) Client driver did not register for a create callback or a create queue.

Arguments:

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

 --*/
{
    FxRequest*  request;
    FxIrp       irp(OriginalIrp);

    UNREFERENCED_PARAMETER(DeviceObject);

    request = (FxRequest*) Context;

    ASSERT(request != NULL);

    irp.PropagatePendingReturned();

    request->Complete(irp.GetStatus());

    return STATUS_MORE_PROCESSING_REQUIRED;
}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::OnCleanup(
    __inout FxIrp* FxIrp
    )
/*++

Routine Description:

    Called in response to IRP_MJ_CLEANUP. This means an handle to
    the device is closed. After invoking the driver registered callback
    event, flush all the queues to cancel requests that belong to the
    file handle being closed. There is however a possibility for
    new requests to come in with the same fileobject.





Arguments:

Return Value:

    NTSTATUS

 --*/
{
    NTSTATUS                status;
    FxFileObject*           pFxFO = NULL;
    WDFFILEOBJECT           hwdfFO = NULL;
    PLIST_ENTRY             next;
    FxFileObjectInfo*       fileObjInfo;
    MxFileObject            fileObject;






    //
    // Check to see if the fileobject represents a stream fileobject
    // created using IoCreateStreamFileObjectLite.
    //
    fileObject.SetFileObject(FxIrp->GetFileObject());
    if (FxIrp->GetFileObject() &&
        (fileObject.GetFlags() & FO_STREAM_FILE)){
        status = STATUS_SUCCESS;
        goto Passthru;
    }

    status = FxFileObject::_GetFileObjectFromWdm(
                 m_Device,
                 m_Device->GetFileObjectClass(),
                 FxIrp->GetFileObject(),
                 &pFxFO
                 );

    ASSERT(status == STATUS_SUCCESS);

    if (pFxFO != NULL && NT_SUCCESS(status)) {
        hwdfFO = pFxFO->GetHandle();
    }

    //
    // Invoke cleanup callbacks.
    //
    if (NULL == pFxFO) {
        //
        // Invoke cleanup callbacks of next layer (cx or client driver) based on the
        // autoforward setting of previous layer. (top to bottom).
        // AutoforwardCleanupClose set to FALSE with a not null create callback
        // means that create request was never forwarded to lower layer.
        //
        for (next = m_FileObjectInfoHeadList.Blink;
             next != &m_FileObjectInfoHeadList;
             next = next->Blink) {

             fileObjInfo = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);

             if (WdfFalse == fileObjInfo->AutoForwardCleanupClose &&
                 fileObjInfo->EvtCxFileCreate.Method != NULL) {
                 next = next->Blink; // one before the real start entry.
                 break;
             }
         }
     }
     else {
         //
         // 'OnCreate' sets this package context.
         //
         next = (PLIST_ENTRY) pFxFO->GetPkgCleanupCloseContext();
         if (NULL == next) {
             next = &m_FileObjectInfoHeadList;
         }
     }

    //
    // Invoke cleanup callbacks only if this layer (cx or client driver) had the
    // opprtunity to see the create request.
    //
    for (next = next->Flink;
         next != &m_FileObjectInfoHeadList;
         next = next->Flink) {

         fileObjInfo = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);
         fileObjInfo->EvtFileCleanup.Invoke(hwdfFO);
    }

    //
    // hwdfFO could be NULL depending on the FileObjectClass
    //

    //
    // Scan all the I/O queues associated with this device
    // and cancel the requests that matches with the handle
    // being closed. We will be able to cancel only requests that
    // are waiting to be dispatched. If the requests are already
    // presented to the driver then it's the responsibility of the
    // driver to complete them when the cleanup callback is invoked.
    //
    if(FxIrp->GetFileObject() != NULL ) {
        FxPkgIo*   pPkgIo;

        pPkgIo = (FxPkgIo*)m_Device->m_PkgIo;
        pPkgIo->FlushAllQueuesByFileObject(FxIrp->GetFileObject());
    }

Passthru:
    if (m_Device->m_AutoForwardCleanupClose) {
        FxIrp->SkipCurrentIrpStackLocation();
        status = FxIrp->CallDriver(m_Device->GetAttachedDevice());
    } else {
        FxIrp->SetStatus(status);
        FxIrp->SetInformation(0);
        FxIrp->CompleteRequest(IO_NO_INCREMENT);
    }

    return status;

}

_Must_inspect_result_
NTSTATUS
FxPkgGeneral::OnClose(
    __inout FxIrp* FxIrp
    )
/*++

Routine Description:

    Called in response to IRP_MJ_CLOSE.  Invoke EvtFileClose event
    if registered and destroy the WDFFILEOBJECT.

Arguments:

Return Value:

    NTSTATUS

 --*/
{
    NTSTATUS                status;
    FxFileObject*           pFxFO = NULL;
    WDFFILEOBJECT           hwdfFO = NULL;
    BOOLEAN                 isStreamFileObject = FALSE;
    BOOLEAN                 acquiredRemLock = FALSE;
    PLIST_ENTRY             next;
    FxFileObjectInfo*       fileObjInfo;
    MxFileObject            fileObject;
    MdIrp                   irp;

    //
    // FxIrp.CompleteRequest NULLs the m_Irp so store m_Irp separately
    // for use in ReleaseRemoveLock.
    //
    irp = FxIrp->GetIrp();

    //
    // Check to see if the fileobject represents a stream fileobject
    // created using IoCreateStreamFileObjectLite. If so, this is a
    // is spurious close sent by the I/O manager when it invalidates
    // the volumes (IopInvalidateVolumesForDevice).
    //
    fileObject.SetFileObject(FxIrp->GetFileObject());
    if (FxIrp->GetFileObject() &&
        (fileObject.GetFlags() & FO_STREAM_FILE)){
        isStreamFileObject = TRUE;
        status = STATUS_SUCCESS;
        goto Passthru;
    }

    status = FxFileObject::_GetFileObjectFromWdm(
        m_Device,
        m_Device->GetFileObjectClass(),
        FxIrp->GetFileObject(),
        &pFxFO
        );

    ASSERT(status == STATUS_SUCCESS);

    if (pFxFO != NULL && NT_SUCCESS(status)) {
        hwdfFO = pFxFO->GetHandle();
    }

    //
    // Invoke close callbacks.
    //
    if (NULL == pFxFO) {
        //
        // Invoke close callbacks of next layer (cx or client driver)  based on the autoforward
        // setting of previous layer. (top to bottom).
        // AutoforwardCleanupClose set to FALSE with a not null create callback
        // means that create request was never forwarded to lower layer.
        //
        for (next = m_FileObjectInfoHeadList.Blink;
             next != &m_FileObjectInfoHeadList;
             next = next->Blink) {

            fileObjInfo = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);

            if (WdfFalse == fileObjInfo->AutoForwardCleanupClose &&
                fileObjInfo->EvtCxFileCreate.Method != NULL) {
                next = next->Blink; // one before the real start entry.
                break;
            }
        }
    }
    else {
        //
        // 'OnCreate' sets this package context.
        //
        next = (PLIST_ENTRY) pFxFO->GetPkgCleanupCloseContext();
        if (NULL == next) {
            next = &m_FileObjectInfoHeadList;
        }
    }

    //
    // Invoke close callbacks only if this layer (cx or client driver) had the opprtunity
    // to see the create request.
    //
    for (next = next->Flink;
         next != &m_FileObjectInfoHeadList;
         next = next->Flink) {

         fileObjInfo = CONTAINING_RECORD(next, FxFileObjectInfo, ListEntry);
         fileObjInfo->EvtFileClose.Invoke(hwdfFO);
    }

    //
    // Destroy the WDFFILEOBJECT. This will result in
    // fileobject EvtCleanup and EvtDestroy event.
    //
    FxFileObject::_DestroyFileObject(
        m_Device,
        m_Device->GetFileObjectClass(),
        FxIrp->GetFileObject()
        );

Passthru:

    if (m_Device->m_AutoForwardCleanupClose) {
        FxIrp->SkipCurrentIrpStackLocation();
        status = FxIrp->CallDriver(m_Device->GetAttachedDevice());
    } else {
        //
        // We're about to complete the request, but we need to decrement the
        // open handle count after we complete the request. However, completing
        // the request immediately opens the gate for the remove IRP to arrive
        // and run down the device. Hence we'll acquire the remove lock in order
        // to ensure that the device is not removed before we've decremented the
        // open handle count.
        //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        acquiredRemLock = AcquireRemoveLockForClose(FxIrp);
#endif
        FxIrp->SetStatus(status);
        FxIrp->SetInformation(0);
        FxIrp->CompleteRequest(IO_NO_INCREMENT);
    }

    if (isStreamFileObject == FALSE) {
        //
        // Note that after this call returns, this object may have been deleted!








        DecrementOpenHandleCount();
    }

    if (acquiredRemLock) {
        Mx::MxReleaseRemoveLock(m_Device->GetRemoveLock(),
                                irp);
    }

    return status;

}

BOOLEAN
FxPkgGeneral::AcquireRemoveLockForClose(
    __in  FxIrp* FxIrp
    )
/*++

Routine Description:

    For PNP devices, this routine acquires the remove lock when handling the
    close IRP.

Arguments:
    Irp - Pointer to the close IRP

Return Value:

    A BOOLEAN value that indicates whether or not the function actually acquired
        the remove lock

 --*/
{
    NTSTATUS status;
    BOOLEAN lockAcquired;
    FxWdmDeviceExtension * wdmExtension = FxDevice::_GetFxWdmExtension(
            m_Device->GetDeviceObject());

    //
    // Initialization
    //
    lockAcquired = FALSE;

    //
    // We attempt to acquire the remove lock only for PNP device objects
    //
    if (m_Device->IsPnp() == FALSE) {
        goto Done;
    }

    //
    // If driver has opted in for remove lock for I/O operations we have
    // already acquired remove lock for Close so no need to do it again.
    //
    if (wdmExtension->RemoveLockOptionFlags &
            WDF_REMOVE_LOCK_OPTION_ACQUIRE_FOR_IO) {
        goto Done;
    }

    status = Mx::MxAcquireRemoveLock(
                m_Device->GetRemoveLock(),
                FxIrp->GetIrp());
    if (NT_SUCCESS(status)) {
        //
        // Successfully acquired the remove lock
        //
        lockAcquired = TRUE;
    } else {
        //
        // This is likely to have failed because we got the remove IRP and
        // called IoReleaseRemoveLockAndWait on another thread. This would
        // happen if there's a bug in the driver above us in the stack that
        // caused it to forward us the remove IRP before we completed the close
        // IRP.
        //
        // There's not much we can do now, since we're already racing against
        // the remove IRP.
        //
        PFX_DRIVER_GLOBALS pFxDriverGlobals;

        pFxDriverGlobals = GetDriverGlobals();

        DoTraceLevelMessage(
                    pFxDriverGlobals,
                    TRACE_LEVEL_ERROR,
                    TRACINGIO,
                    "Unable to acquire remove lock while handling the close IRP"
                    " 0x%p, %!STATUS!",
                    FxIrp->GetIrp(), status);

        if (pFxDriverGlobals->IsVerificationEnabled(1,9, OkForDownLevel)) {
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
        }
    }

Done:
    return lockAcquired;
}


_Must_inspect_result_
NTSTATUS
FxPkgGeneral::OnShutdown(
    __inout FxIrp* FxIrp
    )
/*++

Routine Description:

    Called in response to IRP_MJ_SHUTDOWN.

Arguments:

Return Value:

    NTSTATUS

 --*/
{
    NTSTATUS status;

    m_EvtDeviceShutdown.Invoke(m_Device->GetHandle());

    if(m_Device->IsFilter()) {
        FxIrp->SkipCurrentIrpStackLocation();
        status = FxIrp->CallDriver(m_Device->GetAttachedDevice());
    }
    else {
        status = STATUS_SUCCESS;
        FxIrp->SetStatus(status);
        FxIrp->SetInformation(0);
        FxIrp->CompleteRequest(IO_NO_INCREMENT);
    }

    return status;

}

VOID
FxPkgGeneral::DecrementOpenHandleCount(
    VOID
    )
{
    if (InterlockedDecrement(&m_OpenHandleCount) == 0 && m_Device->IsLegacy()) {
        m_Device->ControlDeviceDelete();
    }
}

BOOLEAN
FxPkgGeneral::CanDestroyControlDevice(
    VOID
    )
{
    //
    // Remove the bias of one that we use to track if a control device should be
    // deleted.
    //
    if (InterlockedDecrement(&m_OpenHandleCount) == 0) {
        return TRUE;
    }
    else {
        return FALSE;
    }

}

VOID
FxPkgGeneral::GetConstraintsHelper(
    __out_opt WDF_EXECUTION_LEVEL*       ExecutionLevel,
    __out_opt WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
    )
/*++

Routine Description:
    This routine is the helper routine and is used by FxFileObject to get the
    ExecutionLevel and SynchronizationScope.

Arguments:

Return Value:
    VOID

--*/

{
    if (ExecutionLevel != NULL) {
        *ExecutionLevel = m_ExecutionLevel;
    }

    if (SynchronizationScope != NULL) {
        *SynchronizationScope = m_SynchronizationScope;
    }
}

FxCallbackLock*
FxPkgGeneral::GetCallbackLockPtrHelper(
    __deref_out_opt FxObject** LockObject
    )
/*++

Routine Description:
    This routine is the helper routine and is used by FxFileObject to get the
    LockObject.


Arguments:

Return Value:
    FxCallbackLock *

--*/

{
    if (LockObject != NULL) {
        *LockObject = m_CallbackLockObjectPtr;
    }

    return m_CallbackLockPtr;
}


