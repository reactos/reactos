/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDevice.cpp

Abstract:

    This is the class implementation for the base Device class.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "coreprivshared.hpp"

extern "C" {
// #include "FxDevice.tmh"
}

//
// This table contains the mapping between device type and the
// default priority boost used by the the framework when
// an I/O request is completed. The DeviceObject->DeviceType
// is used as an index into this table.
//
const CHAR FxDevice::m_PriorityBoosts[] = {
    IO_NO_INCREMENT,        // FILE_DEVICE_UNDEFINED           0x00000000
    IO_NO_INCREMENT,        // FILE_DEVICE_BEEP                0x00000001
    IO_CD_ROM_INCREMENT,    // FILE_DEVICE_CD_ROM              0x00000002
    IO_CD_ROM_INCREMENT,    // FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
    IO_NO_INCREMENT,        // FILE_DEVICE_CONTROLLER          0x00000004
    IO_NO_INCREMENT,        // FILE_DEVICE_DATALINK            0x00000005
    IO_NO_INCREMENT,        // FILE_DEVICE_DFS                 0x00000006
    IO_DISK_INCREMENT,      // FILE_DEVICE_DISK                0x00000007
    IO_DISK_INCREMENT,      // FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
    IO_NO_INCREMENT,        // FILE_DEVICE_FILE_SYSTEM         0x00000009
    IO_NO_INCREMENT,        // FILE_DEVICE_INPORT_PORT         0x0000000a
    IO_KEYBOARD_INCREMENT,  // FILE_DEVICE_KEYBOARD            0x0000000b
    IO_MAILSLOT_INCREMENT,  // FILE_DEVICE_MAILSLOT            0x0000000c
    IO_SOUND_INCREMENT,     // FILE_DEVICE_MIDI_IN             0x0000000d
    IO_SOUND_INCREMENT,     // FILE_DEVICE_MIDI_OUT            0x0000000e
    IO_MOUSE_INCREMENT,     // FILE_DEVICE_MOUSE               0x0000000f
    IO_NO_INCREMENT,        // FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
    IO_NAMED_PIPE_INCREMENT,// FILE_DEVICE_NAMED_PIPE          0x00000011
    IO_NETWORK_INCREMENT,   // FILE_DEVICE_NETWORK             0x00000012
    IO_NETWORK_INCREMENT,   // FILE_DEVICE_NETWORK_BROWSER     0x00000013
    IO_NETWORK_INCREMENT,   // FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
    IO_NO_INCREMENT,        // FILE_DEVICE_NULL                0x00000015
    IO_PARALLEL_INCREMENT,  // FILE_DEVICE_PARALLEL_PORT       0x00000016
    IO_NETWORK_INCREMENT,   // FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
    IO_NO_INCREMENT,        // FILE_DEVICE_PRINTER             0x00000018
    IO_NO_INCREMENT,        // FILE_DEVICE_SCANNER             0x00000019
    IO_SERIAL_INCREMENT,    // FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
    IO_SERIAL_INCREMENT,    // FILE_DEVICE_SERIAL_PORT         0x0000001b
    IO_VIDEO_INCREMENT,     // FILE_DEVICE_SCREEN              0x0000001c
    IO_SOUND_INCREMENT,     // FILE_DEVICE_SOUND               0x0000001d
    IO_SOUND_INCREMENT,     // FILE_DEVICE_STREAMS             0x0000001e
    IO_NO_INCREMENT,        // FILE_DEVICE_TAPE                0x0000001f
    IO_NO_INCREMENT,        // FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
    IO_NO_INCREMENT,        // FILE_DEVICE_TRANSPORT           0x00000021
    IO_NO_INCREMENT,        // FILE_DEVICE_UNKNOWN             0x00000022
    IO_VIDEO_INCREMENT,     // FILE_DEVICE_VIDEO               0x00000023
    IO_DISK_INCREMENT,      // FILE_DEVICE_VIRTUAL_DISK        0x00000024
    IO_SOUND_INCREMENT,     // FILE_DEVICE_WAVE_IN             0x00000025
    IO_SOUND_INCREMENT,     // FILE_DEVICE_WAVE_OUT            0x00000026
    IO_KEYBOARD_INCREMENT,  // FILE_DEVICE_8042_PORT           0x00000027
    IO_NETWORK_INCREMENT,   // FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028
    IO_NO_INCREMENT,        // FILE_DEVICE_BATTERY             0x00000029
    IO_NO_INCREMENT,        // FILE_DEVICE_BUS_EXTENDER        0x0000002a
    IO_SERIAL_INCREMENT,    // FILE_DEVICE_MODEM               0x0000002b
    IO_NO_INCREMENT,        // FILE_DEVICE_VDM                 0x0000002c
    IO_DISK_INCREMENT,      // FILE_DEVICE_MASS_STORAGE        0x0000002d
    IO_NETWORK_INCREMENT,   // FILE_DEVICE_SMB                 0x0000002e
    IO_SOUND_INCREMENT,     // FILE_DEVICE_KS                  0x0000002f
    IO_NO_INCREMENT,        // FILE_DEVICE_CHANGER             0x00000030
    IO_NO_INCREMENT,        // FILE_DEVICE_SMARTCARD           0x00000031
    IO_NO_INCREMENT,        // FILE_DEVICE_ACPI                0x00000032
    IO_NO_INCREMENT,        // FILE_DEVICE_DVD                 0x00000033
    IO_VIDEO_INCREMENT,     // FILE_DEVICE_FULLSCREEN_VIDEO    0x00000034
    IO_NO_INCREMENT,        // FILE_DEVICE_DFS_FILE_SYSTEM     0x00000035
    IO_NO_INCREMENT,        // FILE_DEVICE_DFS_VOLUME          0x00000036
    IO_SERIAL_INCREMENT,    // FILE_DEVICE_SERENUM             0x00000037
    IO_NO_INCREMENT,        // FILE_DEVICE_TERMSRV             0x00000038
    IO_NO_INCREMENT,        // FILE_DEVICE_KSEC                0x00000039
    IO_NO_INCREMENT,        // FILE_DEVICE_FIPS                0x0000003A
    IO_NO_INCREMENT,        // FILE_DEVICE_INFINIBAND          0x0000003B
};

NTSTATUS
FxDevice::_CompletionRoutineForRemlockMaintenance(
    __in MdDeviceObject   DeviceObject,
    __in MdIrp             Irp,
    __in PVOID            Context
    )
/*++

Routine Description:

    A completion routine for the IRPs for which we acquired opt-in remove lock.

Arguments:
    DeviceObject - Pointer to deviceobject
    Irp          - Pointer to the Irp for which we acquired opt-in remove lock.
    Context      - NULL
Return Value:

    NT Status is returned.

--*/

{
    FxIrp irp(Irp);

    UNREFERENCED_PARAMETER(Context);

    //
    // Let the irp continue on its way.
    //
    irp.PropagatePendingReturned();

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    Mx::MxReleaseRemoveLock(&((FxDevice::_GetFxWdmExtension(
                            DeviceObject))->IoRemoveLock), Irp);
#else
    UNREFERENCED_PARAMETER(DeviceObject);
#endif

    return STATUS_CONTINUE_COMPLETION;
}


FxDevice::FxDevice(
    __in FxDriver *ArgDriver
    ) :
    FxDeviceBase(ArgDriver->GetDriverGlobals(), ArgDriver, FX_TYPE_DEVICE, sizeof(FxDevice)),
    m_ParentDevice(NULL)
{
    SetInitialState();
}

VOID
FxDevice::SetInitialState(
    VOID
    )
{
    //
    // Set the initial device state
    //
    m_CurrentPnpState           = WdfDevStatePnpObjectCreated;
    m_CurrentPowerState         = WdfDevStatePowerObjectCreated;
    m_CurrentPowerPolicyState   = WdfDevStatePwrPolObjectCreated;

    //
    // Set the default IO type to "buffered"
    //
    m_ReadWriteIoType = WdfDeviceIoBuffered;

    RtlZeroMemory(&m_DeviceName, sizeof(m_DeviceName));
    RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
    RtlZeroMemory(&m_MofResourceName, sizeof(m_MofResourceName));

    m_Filter = FALSE;
    m_Exclusive = FALSE;
    m_PowerPageableCapable = FALSE;
    m_ParentWaitingOnChild = FALSE;
    m_Legacy = FALSE;
    m_DeviceObjectDeleted = FALSE;
    m_PdoKnown = FALSE;
    m_Legacy = FALSE;
    m_AutoForwardCleanupClose = FALSE;
    m_SelfIoTargetNeeded = FALSE;
    m_DeviceTelemetryInfoFlags = 0;

    //
    // Clear all packages by default
    //

    m_PkgIo      = NULL;
    m_PkgPnp     = NULL;
    m_PkgGeneral = NULL;
    m_PkgWmi     = NULL;
    m_PkgDefault  = NULL;

    InitializeListHead(&m_PreprocessInfoListHead);
    InitializeListHead(&m_CxDeviceInfoListHead);

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    m_FileObjectClass = WdfFileObjectNotRequired;
#else // UMDF
    //
    // In UMDF file object is always required. So indicate that now.
    //
    m_FileObjectClass = WdfFileObjectWdfCannotUseFsContexts;
#endif

    m_DefaultPriorityBoost = IO_NO_INCREMENT;

    InitializeListHead(&m_FileObjectListHead);

    m_RequestLookasideListElementSize = 0;
    RtlZeroMemory(&m_RequestLookasideList, sizeof(m_RequestLookasideList));
    RtlZeroMemory(&m_RequestAttributes, sizeof(m_RequestAttributes));

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    // Init UMDF specific members
    //
    m_CleanupFromFailedCreate = FALSE;
    m_Dispatcher = NULL;
    m_DevStack = NULL;
    m_PdoDevKey = NULL;
    m_DeviceKeyPath = NULL;
    m_KernelDeviceName = NULL;
    m_DeviceInstanceId = NULL;

    m_RetrievalMode = UMINT::WdfDeviceIoBufferRetrievalDeferred;
    m_IoctlIoType = WdfDeviceIoBuffered;
    m_DirectTransferThreshold = 0;

    m_DirectHardwareAccess = FX_DIRECT_HARDWARE_ACCESS_DEFAULT;
    m_RegisterAccessMode = FX_REGISTER_ACCESS_MODE_DEFAULT;
    m_FileObjectPolicy = FX_FILE_OBJECT_POLICY_DEFAULT;
    m_FsContextUsePolicy = FX_FS_CONTEXT_USE_POLICY_DEFAULT;
    m_InteruptThreadpool = NULL;
#endif
}

FxDevice::~FxDevice()
{
    PLIST_ENTRY next;

    // Make it always present right now even on free builds
    if (IsDisposed() == FALSE) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_FATAL, TRACINGDEVICE,
            "FxDevice 0x%p not disposed: this maybe a driver reference count "
            "problem with WDFDEVICE %p", this, GetObjectHandleUnchecked());

        FxVerifierBugCheck(GetDriverGlobals(),
                           WDF_OBJECT_ERROR,
                           (ULONG_PTR) GetObjectHandleUnchecked(),
                           (ULONG_PTR) this);
    }

    //
    // Execute mode-specific destructor. Noop for KMDF, but does
    // does detach and delete of UM device object for UMDF. Therefore
    // can be done before other cleanup.
    //
    DestructorInternal();

    //
    // If the device has been initialized but hasn't yet been
    // destroyed, destroy it now.
    //







    ASSERT(m_DeviceObject.GetObject() == NULL);

    ASSERT(m_DeviceName.Buffer == NULL);

#if FX_CORE_MODE == FX_CORE_KERNEL_MODE
    //
    // Assert only applicable to KM because FxDevice can get destroyed in UMDF
    // without going through normal pnp remove path, for example, when an
    // AddDevice failure is done in reflector, after all um drivers have
    // succeeded AddDevice. KMDF and host use fake remove irp to handle
    // AddDevice failure in KMDF and host respectively, but reflector does not
    // do that for AddDevice failure that happens in reflector.
    // Note that symbolicName buffer will anyway be deleted in this destructor
    // later on so the symbolic link buffer doesn't leak out.
    //
    ASSERT(m_SymbolicLinkName.Buffer == NULL);
#endif

    ASSERT(m_MofResourceName.Buffer == NULL);

    if (m_PkgIo != NULL) {
        m_PkgIo->RELEASE(NULL);
        m_PkgIo = NULL;
    }

    if (m_PkgPnp != NULL) {
        m_PkgPnp->RELEASE(NULL);
        m_PkgPnp = NULL;
    }

    if (m_PkgGeneral != NULL) {
        m_PkgGeneral->RELEASE(NULL);
        m_PkgGeneral = NULL;
    }

    if (m_PkgWmi != NULL) {
        m_PkgWmi->RELEASE(NULL);
        m_PkgWmi = NULL;
    }

    if (m_PkgDefault != NULL) {
        m_PkgDefault->RELEASE(NULL);
        m_PkgDefault = NULL;
    }

    while (!IsListEmpty(&m_PreprocessInfoListHead)) {
        next = RemoveHeadList(&m_PreprocessInfoListHead);
        FxIrpPreprocessInfo* info;
        info = CONTAINING_RECORD(next, FxIrpPreprocessInfo, ListEntry);
        InitializeListHead(next);
        delete info;
    }

    while (!IsListEmpty(&m_CxDeviceInfoListHead)) {
        next = RemoveHeadList(&m_CxDeviceInfoListHead);
        FxCxDeviceInfo* info;
        info = CONTAINING_RECORD(next, FxCxDeviceInfo, ListEntry);
        InitializeListHead(next);
        delete info;
    }

    //
    // Clean up any referenced objects
    //
    if (m_DeviceName.Buffer != NULL) {
        FxPoolFree(m_DeviceName.Buffer);
        RtlZeroMemory(&m_DeviceName, sizeof(m_DeviceName));
    }

    DeleteSymbolicLink();

    if (m_MofResourceName.Buffer != NULL) {
        FxPoolFree(m_MofResourceName.Buffer);
        RtlZeroMemory(&m_MofResourceName, sizeof(m_DeviceName));
    }

    //
    // m_RequestLookasideListElementSize will be set to non zero if we have
    // initialized the request lookaside list.
    //
    if (m_RequestLookasideListElementSize != 0) {
        Mx::MxDeleteNPagedLookasideList(&m_RequestLookasideList);
        m_RequestLookasideListElementSize = 0;
    }

    if (m_ParentDevice != NULL) {
        m_ParentDevice->RELEASE(this);
    }
}

_Must_inspect_result_
NTSTATUS
FxDevice::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDFDEVICE_INIT* DeviceInit,
    __in_opt PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
    __out FxDevice** Device
    )
{
    PWDFDEVICE_INIT     pInit;
    FxDevice*           pDevice;
    NTSTATUS            status;
    WDFOBJECT           object;
    PLIST_ENTRY         pNext;
    PWDFCXDEVICE_INIT   pCxInit;
    FxWdmDeviceExtension* wdmDeviceExtension;

    *Device = NULL;
    pInit = *DeviceInit;

    pDevice = new (FxDriverGlobals, DeviceAttributes)
        FxDevice(pInit->Driver);

    if (pDevice == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    status = pDevice->Initialize(pInit, DeviceAttributes);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    switch (pInit->InitType) {
    case FxDeviceInitTypeFdo:
        status = pDevice->FdoInitialize(pInit);
        break;

    case FxDeviceInitTypePdo:
        status = pDevice->PdoInitialize(pInit);
        break;

    case FxDeviceInitTypeControlDevice:
        status = pDevice->ControlDeviceInitialize(pInit);
        break;

    default:
        //
        // Should not drop here
        //
        ASSERT(FALSE);
        break;
    }
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Ok, we have created the device.  Now lets create a handle for it.
    //
    status = pDevice->PostInitialize();
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Can't use the PDO's FxDevice m_Parent as the object hierarchy parent
    // because the Fx object hierarchy lifetime rules do not match the
    // rules for a pnp PDO lifetime vs its FDO.
    //
    status = pDevice->Commit(DeviceAttributes,
                             &object,
                             pDevice->GetDriver());
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // NOTE: ---> DO NOT FAIL FROM HERE FORWARD <---
    //

    //
    // Up until now we have not reassigned any of the allocations in pInit
    // and assigned them to the underlying objects.  We are now at the point
    // of "no return", ie we cannot fail.  If we reassigned the allocations
    // before this point and the driver retried to create the device (let's
    // say with a different name), we would have freed those allocations
    // and the driver writer would have thought that particular settings
    // we valid, but were not b/c we freed them on error.  So, to avoid a
    // huge tracking mess, we only grab the allocations once we know for
    // *sure* we are going to return success.
    //
    if (pInit->DeviceName != NULL) {
        pInit->DeviceName->ReleaseString(&pDevice->m_DeviceName);
    }

    //
    // Check for driver preprocess requirements.
    //
    if (pInit->PreprocessInfo != NULL) {
        ASSERT( pInit->PreprocessInfo->ClassExtension == FALSE);
        ASSERT(IsListEmpty(&pDevice->m_PreprocessInfoListHead));
        InsertTailList(&pDevice->m_PreprocessInfoListHead,
                        &pInit->PreprocessInfo->ListEntry);
        pInit->PreprocessInfo = NULL;

        //
        // If the driver is preprocessing requests on this device, they need
        // their own stack location so that they can set their own completion
        // routine.
        //
        pDevice->SetStackSize(pDevice->GetStackSize()+1);
    }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    wdmDeviceExtension = _GetFxWdmExtension(pDevice->GetDeviceObject());
    if (wdmDeviceExtension->RemoveLockOptionFlags &
            WDF_REMOVE_LOCK_OPTION_ACQUIRE_FOR_IO) {
        //
        // We will use a completion routine for remlock maintenance
        //
        pDevice->SetStackSize(pDevice->GetStackSize()+1);
    }

    //
    // Note: In case of UMDF StackSize is incremented prior to attaching
    // the device to stack. See the comment in FxDeviceUm.cpp
    //
    if (pDevice->m_SelfIoTargetNeeded) {
        pDevice->SetStackSize(pDevice->GetStackSize()+1);
    }

#else
    UNREFERENCED_PARAMETER(wdmDeviceExtension);
#endif

    //
    // Check for any class-extensions' preprocess requirements.
    //
    for (pNext = pInit->CxDeviceInitListHead.Flink;
         pNext != &pInit->CxDeviceInitListHead;
         pNext = pNext->Flink) {

        pCxInit = CONTAINING_RECORD(pNext, WDFCXDEVICE_INIT, ListEntry);

        if (pCxInit->PreprocessInfo != NULL) {
            ASSERT(pCxInit->PreprocessInfo->ClassExtension);
            InsertTailList(&pDevice->m_PreprocessInfoListHead,
                           &pCxInit->PreprocessInfo->ListEntry);
            pCxInit->PreprocessInfo = NULL;

            //
            // If the class extension is preprocessing requests on this
            // device, it needs its own stack location so that it can
            // set its own completion routine.
            //
            pDevice->SetStackSize(pDevice->GetStackSize()+1);
        }
    }

    if (pDevice->IsPnp()) {
        //
        // Take all of the allocations out of pInit related to pnp.  This
        // will also transition the pnp state machine into the added state.
        //
        pDevice->m_PkgPnp->FinishInitialize(pInit);
    }

    pInit->CreatedDevice = pDevice;

    //
    // Clear out the pointer, we freed it on behalf of the caller
    //
    *DeviceInit = NULL;

    if (pInit->CreatedOnStack == FALSE) {
        delete pInit;
    }

Done:
    if (!NT_SUCCESS(status) && pDevice != NULL) {
        //
        // We want to propagate the original error code
        //
        (void) pDevice->DeleteDeviceFromFailedCreate(status, FALSE);
        pDevice = NULL;
    }

    *Device = pDevice;

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
    // We do not let filters affect the building of the rest of the stack.
    // If they return error, we convert it to STATUS_SUCCESS, remove the
    // attached device from the stack, and cleanup.
    //
    if (IsFilter()) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE %p, !devobj %p is a filter, converting %!STATUS! to"
            " STATUS_SUCCESS", GetObjectHandleUnchecked(), GetDeviceObject(),
            FailedStatus);
        FailedStatus = STATUS_SUCCESS;
    }

    if (UseStateMachine) {
        MxEvent waitEvent;

        //
        // See comments for m_CleanupFromFailedCreate in class definition file
        // for use of this statement.
        //
        SetCleanupFromFailedCreate(TRUE);





        waitEvent.Initialize(SynchronizationEvent, FALSE);
        m_PkgPnp->CleanupDeviceFromFailedCreate(waitEvent.GetSelfPointer());
    }
    else {
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
        if (m_DisposeList != NULL) {
            m_DisposeList->WaitForEmpty();
        }

        //
        // We keep a reference on m_PkgPnp which is released in the destructor
        // so  we can safely touch m_PkgPnp after destroying all of the child
        // objects.
        //
        if (m_PkgPnp != NULL) {
            m_PkgPnp->CleanupStateMachines(TRUE);
        }
    }

    //
    // This will detach and delete the device object
    //
    Destroy();

    return FailedStatus;
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
FxDevice::Initialize(
    __in PWDFDEVICE_INIT DeviceInit,
    __in_opt PWDF_OBJECT_ATTRIBUTES DeviceAttributes
    )
/*++

Routine Description:
    Generic initialization for an FxDevice regardless of role (pdo, fdo, control).

Arguments:


Return Value:


  --*/

{
    PFX_DRIVER_GLOBALS  pGlobals;
    PLIST_ENTRY         next;
    NTSTATUS            status;
    size_t              reqCtxSize;
    PWDFCXDEVICE_INIT   cxInit;
    CCHAR               cxIndex;
    FxCxDeviceInfo*     cxDeviceInfo;

    pGlobals    = GetDriverGlobals();
    m_Exclusive = DeviceInit->Exclusive;
    cxIndex     = 0;

    MarkDisposeOverride(ObjectDoNotLock);

    //
    // Configure device constraints.
    //
    status = ConfigureConstraints(DeviceAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Generic catch all
    //
    m_PkgDefault = new (pGlobals) FxDefaultIrpHandler(pGlobals, (CfxDevice*)this);
    if (m_PkgDefault == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InstallPackage(m_PkgDefault);

    if (DeviceInit->InitType == FxDeviceInitTypeControlDevice) {
        m_Legacy = TRUE;
    }

    //
    // Size will be set to a non zero if the driver wants request attributes
    // associated with each created request.
    //
    if (DeviceInit->RequestAttributes.Size  != 0) {
        ASSERT(DeviceInit->RequestAttributes.Size == sizeof(WDF_OBJECT_ATTRIBUTES));
        RtlCopyMemory(&m_RequestAttributes,
                      &DeviceInit->RequestAttributes,
                      sizeof(DeviceInit->RequestAttributes));
    }

    reqCtxSize = FxGetContextSize(&m_RequestAttributes);

    //
    // If present, setup a I/O class extensions info chain.
    //
    for (next = DeviceInit->CxDeviceInitListHead.Flink;
         next != &DeviceInit->CxDeviceInitListHead;
         next = next->Flink) {

        cxInit = CONTAINING_RECORD(next, WDFCXDEVICE_INIT, ListEntry);

        cxDeviceInfo = new(pGlobals) FxCxDeviceInfo(pGlobals);
        if (NULL == cxDeviceInfo) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        cxDeviceInfo->Index = ++cxIndex; // 1-based.
        cxDeviceInfo->Driver = cxInit->CxDriverGlobals->Driver;
        cxDeviceInfo->IoInCallerContextCallback.m_Method =
                        cxInit->IoInCallerContextCallback;
        cxDeviceInfo->RequestAttributes = cxInit->RequestAttributes;

        InsertTailList(&m_CxDeviceInfoListHead, &cxDeviceInfo->ListEntry);

        //
        // Set weak ref to this run-time cx struct to help file-object logic later on.
        //
        cxInit->CxDeviceInfo = cxDeviceInfo;

        //
        // Find the max size for the request context. Used below.
        //
        ASSERT(cxInit->RequestAttributes.Size == 0 ||
               cxInit->RequestAttributes.Size == sizeof(WDF_OBJECT_ATTRIBUTES));

        reqCtxSize = MAX(FxGetContextSize(&cxInit->RequestAttributes),
                         reqCtxSize);
    }

    //
    // Memory layout for memory backing FxRequest which is allocated from the
    // lookaside list:
    //
    // If we are tracking memory, the allocation layout is
    // 0x0                                                 - FX_POOL_TRACKER
    // 0x0 + sizeof(FX_POOL_TRACKER)                       - FX_POOL_HEADER
    // 0x0 + sizeof(FX_POOL_TRACKER) + FX_POOL_HEADER_SIZE - start of FxRequest
    //
    // if no tracking is occuring, the allocation layout is
    // 0x0                                                 - FX_POOL_HEADER
    // 0x0 + FX_POOL_HEADER_SIZE                           - start of FxRequest
    //
    // NOTE:  If the computation of m_RequestLookasideListElementSize changes,
    //        FxDevice::AllocateRequestMemory and FxDevice::FreeRequestMemory will also
    //        need to be updated to reflect the changes made.
    //
    status = FxCalculateObjectTotalSize2(pGlobals,
                                         sizeof(FxRequest),
                                         0,
                                         reqCtxSize,
                                         &m_RequestLookasideListElementSize);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxPoolAddHeaderSize(pGlobals,
                                 m_RequestLookasideListElementSize,
                                 &m_RequestLookasideListElementSize);

    if (!NT_SUCCESS(status)) {
        //
        // FxPoolAddHeaderSize will log to the IFR on error
        //
        return status;
    }

    Mx::MxInitializeNPagedLookasideList(&m_RequestLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    m_RequestLookasideListElementSize,
                                    pGlobals->Tag,
                                    0);
    //
    // Init device's auto_forward_cleanup_close.
    //
    ConfigureAutoForwardCleanupClose(DeviceInit);

    //
    // Create, close, cleanup, shutdown
    //
    m_PkgGeneral = new(pGlobals) FxPkgGeneral(pGlobals, this);
    if (m_PkgGeneral == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InstallPackage(m_PkgGeneral);

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)






    // m_PkgWmi = new(pGlobals) FxWmiIrpHandler(pGlobals, this); __REACTOS__
    // if (m_PkgWmi == NULL) {
    //     return STATUS_INSUFFICIENT_RESOURCES;
    // }
    // InstallPackage(m_PkgWmi);
#endif

    //
    // IO package handles reads, writes, internal and external IOCTLs
    //
    m_PkgIo = new(pGlobals) FxPkgIo(pGlobals, (CfxDevice*) this);

    if (m_PkgIo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InstallPackage(m_PkgIo);

    //
    // Configure I/O package.
    //
    m_PkgIo->SetIoInCallerContextCallback(DeviceInit->IoInCallerContextCallback);

    if (DeviceInit->RequiresSelfIoTarget) {
        m_SelfIoTargetNeeded = TRUE;
    }

    return STATUS_SUCCESS;
}

VOID
FxDevice::ConfigureAutoForwardCleanupClose(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    WDF_TRI_STATE   autoForwardCleanupClose;
    PLIST_ENTRY     next;
    BOOLEAN         checkClientDriver;

    autoForwardCleanupClose = WdfUseDefault;
    checkClientDriver = TRUE;

    //
    // Device-wide configuration for auto forwarding cleanup and close requests:
    //  . Use WdfFalse if one of the devices in the chain use this setting with a create
    //    callback (this means it will complete all create IRPs).
    //  . Else use lowest driver's setting in the chain (order of cx chain: lower to higher).
    //  . If no settings are present, use default.
    //
    for (next = DeviceInit->CxDeviceInitListHead.Blink;
         next != &DeviceInit->CxDeviceInitListHead;
         next = next->Blink) {

        PWDFCXDEVICE_INIT cxInit;

        cxInit = CONTAINING_RECORD(next, WDFCXDEVICE_INIT, ListEntry);

        if (cxInit->FileObject.Set) {
            autoForwardCleanupClose = cxInit->FileObject.AutoForwardCleanupClose;

            if (autoForwardCleanupClose == WdfFalse &&
                cxInit->FileObject.Callbacks.EvtCxDeviceFileCreate != NULL) {

                checkClientDriver = FALSE;
                break;
            }
        }
    }

    if (checkClientDriver && DeviceInit->FileObject.Set) {
        autoForwardCleanupClose = DeviceInit->FileObject.AutoForwardCleanupClose;
    }

    switch (autoForwardCleanupClose) {
    case WdfTrue:

        m_AutoForwardCleanupClose = TRUE;
        //
        // If the device is legacy then set it to false because you can't forward
        // requests.
        //
        if(m_Legacy) {
            m_AutoForwardCleanupClose = FALSE;
        }
        break;

    case WdfFalse:
        m_AutoForwardCleanupClose = FALSE;
        break;

    case WdfUseDefault:
        //
        // For filters (which must be FDOs), we default to TRUE.  All other
        // device roles (FDO, PDO, control) default to FALSE.  We cannot check
        // m_Filter yet because it is set in FdoInitialize which occurs later.
        //
        if (DeviceInit->IsFdoInit() && DeviceInit->Fdo.Filter) {
            m_AutoForwardCleanupClose = TRUE;
        }
        else {
            m_AutoForwardCleanupClose = FALSE;
        }
    }
}

_Must_inspect_result_
NTSTATUS
FxDevice::PostInitialize(
    VOID
    )
{
    NTSTATUS status;

    status = FxDisposeList::_Create(GetDriverGlobals(),
                                   m_DeviceObject.GetObject(),
                                   &m_DisposeList);

    return status;
}








#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)

_Must_inspect_result_
NTSTATUS
FxDevice::CreateDevice(
    __in PWDFDEVICE_INIT DeviceInit
    )
{
    MdDeviceObject  pNewDeviceObject;
    ULONG           characteristics;
    NTSTATUS        status;
    DEVICE_TYPE     devType;

    status = m_PkgGeneral->Initialize(DeviceInit);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    devType = DeviceInit->DeviceType;
    if (devType < ARRAY_SIZE(m_PriorityBoosts)) {
        m_DefaultPriorityBoost= m_PriorityBoosts[devType];
    }

    characteristics = DeviceInit->Characteristics;

    //
    // You can only create secure device objects which have a name.  All other
    // device objects rely on the PDO's security
    //
    if (DeviceInit->ShouldCreateSecure()) {
        PUNICODE_STRING pName, pSddl;
        LPGUID pGuid;

        if (DeviceInit->DeviceName != NULL) {
            pName = DeviceInit->DeviceName->GetUnicodeString();
        }
        else {
            pName = NULL;
        }

        if (DeviceInit->Security.DeviceClassSet) {
            pGuid = &DeviceInit->Security.DeviceClass;
        }
        else {
            pGuid = NULL;
        }

        if (DeviceInit->Security.Sddl != NULL) {
            pSddl = DeviceInit->Security.Sddl->GetUnicodeString();
        }
        else {
            //
            // Always provide an SDDL if one is not supplied.
            //
            // SDDL_DEVOBJ_SYS_ALL_ADM_ALL = "D:P(A;;GA;;;SY)(A;;GA;;;BA)"
            //
            // SDDL_DEVOBJ_SYS_ALL_ADM_ALL allows the kernel, system, and
            // administrator complete control over the device. No other users
            // may access the device.
            //
            // pSddl = (PUNICODE_STRING) &SDDL_DEVOBJ_SYS_ALL_ADM_ALL;
            pSddl = NULL; // __REACTOS__ : wdmsec.lib is not supported
        }

        status = Mx::MxCreateDeviceSecure(
            m_Driver->m_DriverObject.GetObject(),
            sizeof(FxWdmDeviceExtension),
            pName,
            devType,
            characteristics,
            m_Exclusive,
            pSddl,
            pGuid,
            &pNewDeviceObject);
    }
    else {
        status = Mx::MxCreateDevice(
            m_Driver->m_DriverObject.GetObject(),
            sizeof(FxWdmDeviceExtension),
            NULL,
            devType,
            characteristics,
            m_Exclusive,
            &pNewDeviceObject);
    }

    if (NT_SUCCESS(status)) {
        FxWdmDeviceExtension* pWdmExt;

        pWdmExt = _GetFxWdmExtension(pNewDeviceObject);

        //
        // We reassign DeviceExtension below and then use the knowledge that
        // we can always retrieve DeviceExtension by adding sizeof(DEVICE_OBJECT)
        // to pNewDeviceObject.  ASSERT that this assumption is correct.
        //
        MxDeviceObject newDeviceObject(pNewDeviceObject);
        ASSERT(pWdmExt == newDeviceObject.GetDeviceExtension());

        Mx::MxInitializeRemoveLock(&pWdmExt->IoRemoveLock,
                               GetDriverGlobals()->Tag,
                               0,          // max min
                               0           // highwater mark
                               );

        //
        // Option for remove lock is stored in device extension
        // since this option may be examined after FxDevice is destroyed
        // (if an Irp is sent after removal of device).
        // We combine the flags from DeviceInit with what's set through registry
        //
        pWdmExt->RemoveLockOptionFlags = DeviceInit->RemoveLockOptionFlags |
                                            GetDriverGlobals()->RemoveLockOptionFlags;

        //
        // We assign the first context assigned to this object as the
        // DeviceExtension for compatibility reasons.  This allows existing
        // WDM extensions to work as well as any stack which exports a known
        // structure for the extension (ie the FDO knows the extension of its
        // PDO and casts it and accesses it directly).
        //
        newDeviceObject.SetDeviceExtension(&GetContextHeader()->Context[0]);
        m_DeviceObject.SetObject(pNewDeviceObject);

        //
        // Set some device object flags based on properties of DeviceInit.
        //
        // If we are a filter, we will set these flags later
        // (in FxDevice::FdoInitialize) based on the device we are attached to.
        //
        if (m_Filter == FALSE) {
            if (DeviceInit->ReadWriteIoType == WdfDeviceIoBuffered) {
                m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() | DO_BUFFERED_IO);
            }
            else if (DeviceInit->ReadWriteIoType == WdfDeviceIoDirect) {
                m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() | DO_DIRECT_IO);
            }

            m_ReadWriteIoType = DeviceInit->ReadWriteIoType;
            m_PowerPageableCapable = DeviceInit->PowerPageable;
        }
    }

    return status;
}

#endif  // (FX_CORE_MODE == FX_CORE_KERNEL_MODE)

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

VOID
FxDevice::DeleteObject(
    VOID
    )
/*++

Routine Description:
    Virtual override of an FxObject::DeleteObject.  For PDOs which are created
    statically and then deleted before being reported to WDF, we must simulate
    a pnp remove event to trigger cleanup.

Arguments:
    None

Return Value:
    None

  --*/
{
    if (IsPnp() && IsPdo()) {
        FxPkgPdo* pPkgPdo;
        KIRQL irql;
        BOOLEAN remove;

        remove = FALSE;

        pPkgPdo = GetPdoPkg();

        pPkgPdo->Lock(&irql);

        if (pPkgPdo->m_Static && pPkgPdo->m_AddedToStaticList == FALSE) {
            //
            // Since no pnp action has been taken since the child was created, we
            // should be in the initial state.
            //
            if (m_CurrentPnpState == WdfDevStatePnpInit) {
                //
                // A PDO in this state should be deletable
                //
                ASSERT(IsNoDeleteDDI() == FALSE);

                remove = TRUE;
            }
            else {
                //
                // If we are not in the init state, we should be in the created
                // state. This means we are failing from FxDevice::CreateDevice.
                //
                ASSERT(m_CurrentPnpState == WdfDevStatePnpObjectCreated);
            }
        }

        pPkgPdo->Unlock(irql);

        if (remove) {
            //
            // Cleanup the device and then let the super class delete the object.
            //
            (void) DeleteDeviceFromFailedCreateNoDelete(
                STATUS_UNSUCCESSFUL, TRUE);
        }
    }
    else if (IsLegacy() && m_PkgGeneral != NULL && m_DeviceObject.GetObject() != NULL) {
        //
        // We allow tracing devices to go through a normal DeleteObject() path
        // where we do not prematurely delete the device object.
        //
        (void) FxVerifierCheckIrqlLevel(GetDriverGlobals(), PASSIVE_LEVEL);

        m_DeviceObjectDeleted = TRUE;

        //
        // This reference will be released in Destroy().
        //
        Mx::MxReferenceObject(m_DeviceObject.GetObject());

        if (m_PkgWmi != NULL) {
            //
            // Since a legacy NT4 driver does not have an explicit WMI
            // deregistration DDI, we do it for them on deletion.
            //
            // This is done in DeleteObject because we need to deregister before
            // we delete the device object, otherwise we can bugcheck when
            // running under driver verifier.
            //
            // m_PkgWmi->Deregister(); __REACTOS__
        }

        //
        // By deleting the device object now, we prevent any new creates from
        // being sent to the device (the io manager enforces this).
        //
        Mx::MxDeleteDevice(m_DeviceObject.GetObject());

        if (m_PkgGeneral->CanDestroyControlDevice() == FALSE) {
            //
            // Delay the actual destruction of the device until the last open
            // handle has been closed.  ControlDeviceDelete() will perform the
            // destruction later.
            //
            return;
        }
    }

    FxDeviceBase::DeleteObject(); // __super call
}

BOOLEAN
FxDevice::Dispose(
    VOID
    )
{
    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    if (m_Legacy) {
        if (m_PkgWmi != NULL) {
            //
            // We deregister in Dispose() (as well as DeleteObject()) for
            // control devices which are implicitly destroyed when the driver
            // unloads and FxDriver is being deleted.
            //
            // Since a legacy NT4 driver does not have an explicit WMI
            // deregistration DDI, we do it for them on destruction.
            //
            // This is done in Dispose because we are guaranteed to be at
            // passive level here.  Even though m_PkgWmi was already
            // Dispose()'ed (because it is a child of this object), it is still
            // valid to reference the pointer because there is an explicit
            // reference on the object that was taken when we created this object.
            //
            // m_PkgWmi->Deregister(); __REACTOS__
        }

        //
        // Important that the cleanup routine be called while the PDEVICE_OBJECT
        // is valid!
        //
        CallCleanup();

        //
        // Manually destroy the children now so that by the time we wait on the
        // dispose empty out, all of the children will have been added to it.
        //
        DestroyChildren();

        if (m_DisposeList != NULL) {
            m_DisposeList->WaitForEmpty();
        }

        //
        // Now delete the device object
        //
        Destroy();

        return FALSE;
    }

    return FxDeviceBase::Dispose(); // __super call
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
    }

    return STATUS_SUCCESS;
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

    if (Info->Dispatch[major].EvtDevicePreprocess != NULL) {
        if (Info->Dispatch[major].NumMinorFunctions == 0) {
            //
            // If the driver is not interested in particular minor codes,
            // just give the irp to it.
            //
            preprocess = TRUE;
        }
        else {
            ULONG i;

            //
            // Try to match up to a minor code.
            //
            for (i = 0; i < Info->Dispatch[major].NumMinorFunctions; i++) {
                if (Info->Dispatch[major].MinorFunctions[i] == minor) {
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
    UCHAR           major, minor;
    FxIrp irp(Irp);

    major = irp.GetMajorFunction();
    minor = irp.GetMinorFunction();

    //
    // If this is a pnp remove irp, this object could be deleted by the time
    // EvtDevicePreprocess returns.  To not touch freed pool, capture all
    // values we will need before preprocessing.
    //

    if (Info->ClassExtension == FALSE) {
        status = Info->Dispatch[major].EvtDevicePreprocess( Device->GetHandle(),
                                                            Irp);
    }
    else {
        status = Info->Dispatch[major].EvtCxDevicePreprocess(
                                                            Device->GetHandle(),
                                                            Irp,
                                                            DispatchContext);
    }

    //
    // If we got this far, we handed the irp off to EvtDevicePreprocess, so we
    // must now do our remlock maintainance if necessary.
    //
    if (FxDevice::_RequiresRemLock(major, minor) == FxDeviceRemLockRequired) {
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
           ((UCHAR)(ULONG_PTR)DispatchContext & FX_IN_DISPATCH_CALLBACK) == 0);

    //
    // Check for any driver/class-extensions' preprocess requirements.
    //
    while (next != &Device->m_PreprocessInfoListHead) {
        FxIrpPreprocessInfo* info;

        info = CONTAINING_RECORD(next, FxIrpPreprocessInfo, ListEntry);

        //
        // Advance to next node.
        //
        next = next->Flink;

        if (IsPreprocessIrp(Irp, info)) {
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
FxDevice::DispatchPreprocessedIrp(
    __in MdIrp       Irp,
    __in WDFCONTEXT DispatchContext
    )
{
    NTSTATUS    status;
    UCHAR       major, minor;
    FxIrp irp(Irp);

    //
    // The contract for this DDI is just like IoCallDriver.  The caller sets up
    // their stack location and then the DDI advances to the next stack location.
    // This means that the caller either has to call IoSkipCurrentIrpStackLocation
    // or IoCopyCurrentIrpStackLocationToNext before calling this DDI.
    //
    irp.SetNextIrpStackLocation();

    major = irp.GetMajorFunction();
    minor = irp.GetMinorFunction();

    //
    // FxPkgPnp and FxWmiIrpHandler expect that there will be a remove lock
    // acquired for all power irps.  We release the remlock when we called
    // Evt(Ext)DevicePreprocessIrp.
    //
    if (_RequiresRemLock(major, minor) == FxDeviceRemLockRequired) {
        status = Mx::MxAcquireRemoveLock(
            GetRemoveLock(),
            Irp
            );

        if (!NT_SUCCESS(status)) {
            goto Done;
        }
    }

    return DispatchWorker(this, Irp, DispatchContext);

Done:
    irp.SetStatus(status);
    irp.SetInformation(0);
    irp.CompleteRequest(IO_NO_INCREMENT);

    return status;
}

VOID
FxDevice::InstallPackage(
    __inout FxPackage *Package
    )

{
    //
    // Add this package as an association on FxDevice
    // so its children get Dispose notifications.
    //
    // Note: This assumes a transfer of the controlling reference
    //       count which it will dereference on FxDevice teardown.
    //       We need to add an extra one here since packages have
    //       an existing reference count model.
    //
    Package->AddRef();
    Package->AssignParentObject(this);
}

PVOID
FxDevice::AllocateRequestMemory(
    __in_opt PWDF_OBJECT_ATTRIBUTES RequestAttributes
    )
/*++

Routine Description:
    Allocates enough memory for an FxRequest* plus any additonal memory required
    for the device's specific context memory.

    If we are tracking memory, the allocation layout is
    0x0                                                 - FX_POOL_TRACKER
    0x0 + sizeof(FX_POOL_TRACKER)                       - FX_POOL_HEADER
    0x0 + sizeof(FX_POOL_TRACKER) + FX_POOL_HEADER_SIZE - start of FxRequest

    if no tracking is occuring, the allocation layout is
    0x0                                                 - FX_POOL_HEADER
    0x0 + FX_POOL_HEADER_SIZE                           - start of FxRequest

    the total size is precomputed in m_RequestLookasideListElementSize during
    FxDevice::Initialize

Arguments:
    RequestAttributes - Attributes setting for the request.

Return Value:
    valid ptr or NULL

  --*/

{
    PFX_DRIVER_GLOBALS pGlobals;
    PFX_POOL_TRACKER pTracker;
    PFX_POOL_HEADER  pHeader;
    PVOID ptr, pTrueBase;

    pGlobals = GetDriverGlobals();
    ptr = NULL;

    if (IsPdo() && GetPdoPkg()->IsForwardRequestToParentEnabled()) {
        pTrueBase = FxAllocateFromNPagedLookasideListNoTracking(&m_RequestLookasideList);
    }
    else {
        pTrueBase = FxAllocateFromNPagedLookasideList(&m_RequestLookasideList,
                                                      m_RequestLookasideListElementSize);
    }

    if (pTrueBase != NULL) {
        if (pGlobals->IsPoolTrackingOn()) {
            pTracker = (PFX_POOL_TRACKER) pTrueBase;
            pHeader  = WDF_PTR_ADD_OFFSET_TYPE(pTrueBase,
                                               sizeof(FX_POOL_TRACKER),
                                               PFX_POOL_HEADER);

            //
            // Format and insert the Tracker in the NonPagedHeader list.
            //
            FxPoolInsertNonPagedAllocateTracker(&pGlobals->FxPoolFrameworks,
                                                pTracker,
                                                m_RequestLookasideListElementSize,
                                                pGlobals->Tag,
                                                _ReturnAddress());
        }
        else {
            pHeader = (PFX_POOL_HEADER) pTrueBase;
        }

        //
        // Common init
        //
        pHeader->Base = pTrueBase;
        pHeader->FxDriverGlobals = pGlobals;

        ptr = &pHeader->AllocationStart[0];

        if (RequestAttributes == NULL) {
            RequestAttributes = &m_RequestAttributes;
        }

        ptr = FxObjectAndHandleHeaderInit(
            pGlobals,
            ptr,
            COMPUTE_OBJECT_SIZE(sizeof(FxRequest), 0),
            RequestAttributes,
            FxObjectTypeExternal
            );

#if FX_VERBOSE_TRACE
        DoTraceLevelMessage(pGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                            "Allocating FxRequest* %p, WDFREQUEST %p",
                            ptr, _ToHandle((FxObject*) ptr));
#endif
        return ptr;
    }

    return NULL;
}

VOID
FxDevice::FreeRequestMemory(
    __in FxRequest* Request
    )
{
    PFX_POOL_HEADER pHeader;

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Free FxRequest* %p memory", Request);
#endif

    //
    // Remove the request from the list of outstanding requests against this
    // driver.
    //
    pHeader = FxObject::_CleanupPointer(GetDriverGlobals(), Request);
    if (IsPdo() && GetPdoPkg()->IsForwardRequestToParentEnabled()) {
        FxFreeToNPagedLookasideListNoTracking(&m_RequestLookasideList, pHeader->Base);
    }
    else {
        FxFreeToNPagedLookasideList(&m_RequestLookasideList, pHeader->Base);
    }
}

_Must_inspect_result_
NTSTATUS
FxDevice::QueryInterface(
    __inout FxQueryInterfaceParams* Params
    )
{
    switch (Params->Type) {
    case FX_TYPE_DEVICE:
        *Params->Object = (FxDevice*) this;
        break;

    default:
        return FxDeviceBase::QueryInterface(Params); // __super call
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxDevice::AddIoTarget(
    __inout FxIoTarget* IoTarget
    )
{
    NTSTATUS status;

    status = m_IoTargetsList.Add(GetDriverGlobals(),
                                 &IoTarget->m_TransactionedEntry);

    if (NT_SUCCESS(status)) {
        IoTarget->m_AddedToDeviceList = TRUE;
        IoTarget->ADDREF(this);
    }

    return status;
}

VOID
FxDevice::RemoveIoTarget(
    __inout FxIoTarget* IoTarget
    )
{
    m_IoTargetsList.Remove(GetDriverGlobals(),
                           &IoTarget->m_TransactionedEntry);

    //
    // Assumes that the caller has its own reference on the IoTarget
    //
    IoTarget->RELEASE(this);
}

_Must_inspect_result_
NTSTATUS
FxDevice::AllocateEnumInfo(
    VOID
    )
{
    if (IsPnp()) {
        return m_PkgPnp->AllocateEnumInfo();
    }
    else {
        return STATUS_SUCCESS;
    }
}

FxIoTarget*
FxDevice::GetDefaultIoTarget(
    VOID
    )
{
    if (IsPnp() && IsFdo()) {
        return GetFdoPkg()->m_DefaultTarget;
    }
    else {
        return NULL;
    }
}

FxIoTargetSelf*
FxDevice::GetSelfIoTarget(
    VOID
    )
/*++
Routine Description:
    Returns the Self IO target for this FxDevice.
    Currently Self IO Target is supported only for a Pnp FDO.
    If the Self IO Target has not been established, it returns NULL.
--*/
{
    if (IsPnp() && IsFdo()) {
        return GetFdoPkg()->m_SelfTarget;
    }
    else {
        return NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxDevice::SetFilter(
    __in BOOLEAN Value
    )
{
    NTSTATUS status;

    ASSERT(IsFdo());

    status = m_PkgIo->SetFilter(Value);

    if (NT_SUCCESS(status) && m_PkgPnp != NULL) {
        status = GetFdoPkg()->SetFilter(Value);
    }

    if (NT_SUCCESS(status)) {
        m_Filter = Value;
    }

    return status;
}

VOID
FxDevice::SetFilterIoType(
    VOID
    )
{
    FxIoTarget * ioTarget;
    FxTransactionedEntry * targetsList = NULL;

    ASSERT(IsFilter());

    m_DeviceObject.SetFlags( m_DeviceObject.GetFlags() & ~(DO_BUFFERED_IO | DO_DIRECT_IO));

    //
    // m_AttachedDevice can be NULL for UMDF, so check for NULL
    //
    if (m_AttachedDevice.GetObject() != NULL) {
        m_DeviceObject.SetFlags(m_DeviceObject.GetFlags() |
            (m_AttachedDevice.GetFlags() & (DO_BUFFERED_IO | DO_DIRECT_IO)));
    }

    if (m_DeviceObject.GetFlags() & DO_BUFFERED_IO) {
        m_ReadWriteIoType = WdfDeviceIoBuffered;
    }
    else if (m_DeviceObject.GetFlags() & DO_DIRECT_IO) {
        m_ReadWriteIoType = WdfDeviceIoDirect;
    }
    else {
        m_ReadWriteIoType = WdfDeviceIoNeither;
    }

    //
    // We also need to propagate these settings to any io targets that
    // have already been created
    //

    m_IoTargetsList.LockForEnum(GetDriverGlobals());

    targetsList = m_IoTargetsList.GetNextEntry(targetsList);

    while (targetsList != NULL) {

        ioTarget = (FxIoTarget *) targetsList->GetTransactionedObject();

        if (ioTarget->GetTargetPDO() == GetPhysicalDevice()) {
            ioTarget->UpdateTargetIoType();
        }

        targetsList = m_IoTargetsList.GetNextEntry(targetsList);
    }

    m_IoTargetsList.UnlockFromEnum(GetDriverGlobals());
}

BOOLEAN
FxDevice::IsInterfaceRegistered(
    _In_ const GUID* InterfaceClassGUID,
    _In_opt_ PCUNICODE_STRING RefString
    )
{
    PSINGLE_LIST_ENTRY ple;
    BOOLEAN found = FALSE;

    m_PkgPnp->m_DeviceInterfaceLock.AcquireLock(GetDriverGlobals());

    //
    // Iterate over the interfaces and see if we have a match
    //
    for (ple = m_PkgPnp->m_DeviceInterfaceHead.Next; ple != NULL; ple = ple->Next) {
        FxDeviceInterface *pDI;

        pDI = FxDeviceInterface::_FromEntry(ple);

        if (FxIsEqualGuid(&pDI->m_InterfaceClassGUID, InterfaceClassGUID)) {
            if (RefString != NULL) {
                if ((RefString->Length == pDI->m_ReferenceString.Length)
                    &&
                    (RtlCompareMemory(RefString->Buffer,
                                      pDI->m_ReferenceString.Buffer,
                                      RefString->Length) == RefString->Length)) {
                    //
                    // They match, carry on
                    //
                    DO_NOTHING();
                }
                else {
                    //
                    // The ref strings do not match, continue on in the search
                    // of the collection.
                    //
                    continue;
                }
            }
            else if (pDI->m_ReferenceString.Length > 0) {
                //
                // Caller didn't specify a ref string but this interface has
                // one, continue on in the search through the collection.
                //
                continue;
            }

            //
            // Set the state and break out of the loop because we found our
            // interface.
            //
            found = TRUE;
            break;
        }
    }

    m_PkgPnp->m_DeviceInterfaceLock.ReleaseLock(GetDriverGlobals());

    return found;
}

_Must_inspect_result_
NTSTATUS
FxDevice::_AllocAndQueryProperty(
    _In_ PFX_DRIVER_GLOBALS Globals,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _In_opt_ FxDevice* Device,
    _In_opt_ MdDeviceObject RemotePdo,
    _In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_ POOL_TYPE PoolType,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_ WDFMEMORY* PropertyMemory
    )
{
    FxMemoryObject* pMemory;
    NTSTATUS status;
    ULONG length = 0;

    status = FxDevice::_QueryProperty(Globals,
                                      DeviceInit,
                                      Device,
                                      RemotePdo,
                                      DeviceProperty,
                                      0,
                                      NULL,
                                      &length);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        DoTraceLevelMessage(Globals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not retrieve property %d length, %!STATUS!",
                            DeviceProperty, status);
        _Analysis_assume_(!NT_SUCCESS(status));
        return status;
    }

    status = FxMemoryObject::_Create(Globals,
                                     PropertyMemoryAttributes,
                                     PoolType,
                                     Globals->Tag,
                                     length,
                                     &pMemory);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(Globals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not allocate WDFMEMORY, %!STATUS!", status);
        return status;
    }

    status = FxDevice::_QueryProperty(Globals,
                                      DeviceInit,
                                      Device,
                                      RemotePdo,
                                      DeviceProperty,
                                      length,
                                      pMemory->GetBuffer(),
                                      &length);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(Globals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not query for full buffer, size %d, for "
                            "property %d, %!STATUS!",
                            length, DeviceProperty, status);
        pMemory->DeleteObject();
        return status;
    }

    status = pMemory->Commit(PropertyMemoryAttributes,
                             (WDFOBJECT*)PropertyMemory);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(Globals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not commit memory object, %!STATUS!",
                            status);
        pMemory->DeleteObject();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::_AllocAndQueryPropertyEx(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _In_opt_ FxDevice* Device,
    _In_ PVOID PropertyData,
    _In_ FxPropertyType FxPropertyType,
    _In_ POOL_TYPE PoolType,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_ WDFMEMORY*  PropertyMemory,
    _Out_ PDEVPROPTYPE PropertyType
    )
{
    FxMemoryObject* pMemory;
    NTSTATUS status;
    ULONG length = 0;
    DEVPROPTYPE propType;
    ULONG requiredLength;

    status = FxDevice::_QueryPropertyEx(DriverGlobals,
                                        DeviceInit,
                                        Device,
                                        PropertyData,
                                        FxPropertyType,
                                        0,
                                        NULL,
                                        &requiredLength,
                                        &propType);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Could not retrieve property, %!STATUS!",
                            status);
        _Analysis_assume_(!NT_SUCCESS(status));
        return status;
    }

    *PropertyMemory = NULL;
    *PropertyType = 0;

    length = requiredLength;
    status = FxMemoryObject::_Create(DriverGlobals,
                                     PropertyMemoryAttributes,
                                     PoolType,
                                     DriverGlobals->Tag,
                                     length,
                                     &pMemory);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Could not allocate WDFMEMORY, %!STATUS!", status);
        return status;
    }

    status = FxDevice::_QueryPropertyEx(DriverGlobals,
                                        DeviceInit,
                                        Device,
                                        PropertyData,
                                        FxPropertyType,
                                        length,
                                        pMemory->GetBuffer(),
                                        &requiredLength,
                                        &propType);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Could not query for full buffer, size %d, for "
                            "property, %!STATUS!",
                            length, status);
        pMemory->DeleteObject();
        return status;
    }

    status = pMemory->Commit(PropertyMemoryAttributes,
                             (WDFOBJECT*)PropertyMemory);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Could not commit memory object, %!STATUS!",
                            status);
        pMemory->DeleteObject();
    }
    else {
        *PropertyMemory = pMemory->GetHandle();
        *PropertyType = propType;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDevice::_ValidateOpenKeyParams(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _In_opt_ FxDevice* Device
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // This function should be called with exactly one valid WDFDEVICE_INIT
    // or one valid FxDevice object. Supplying neither or both is an error.
    //
    if ((DeviceInit == NULL && Device == NULL) ||
        (DeviceInit != NULL && Device != NULL)) {

        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Device OpenKey/QueryProperty was called with invalid "
            "DeviceInit and Device parameters, %!STATUS!", status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }

    return status;
}

