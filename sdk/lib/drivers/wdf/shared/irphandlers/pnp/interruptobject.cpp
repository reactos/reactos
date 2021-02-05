/*++

Copyright (c) Microsoft Corporation

Module Name:

    InterruptObject.cpp

Abstract:

    This module implements a frameworks managed interrupt object

Author:




Environment:

    Both kernel and user mode

Revision History:





--*/

#include "pnppriv.hpp"

// Tracing support
extern "C" {
#if defined(EVENT_TRACING)
#include "InterruptObject.tmh"
#endif
}

//
// We need three parameters for KeSynchronizeExecution so we use this
// structure on the stack since its a synchronous call
//
struct FxInterruptSyncParameters {
    FxInterrupt*                  Interrupt;
    PFN_WDF_INTERRUPT_SYNCHRONIZE Callback;
    WDFCONTEXT                    Context;
};

//
// At this time we are unable to include wdf19.h in the share code, thus for
// now we simply cut and paste the needed structures.
//
typedef struct _WDF_INTERRUPT_CONFIG_V1_9 {
    ULONG              Size;

    //
    // If this interrupt is to be synchronized with other interrupt(s) assigned
    // to the same WDFDEVICE, create a WDFSPINLOCK and assign it to each of the
    // WDFINTERRUPTs config.
    //
    WDFSPINLOCK        SpinLock;

    WDF_TRI_STATE      ShareVector;

    BOOLEAN            FloatingSave;

    //
    // Automatic Serialization of the DpcForIsr
    //
    BOOLEAN            AutomaticSerialization;

    // Event Callbacks
    PFN_WDF_INTERRUPT_ISR         EvtInterruptIsr;

    PFN_WDF_INTERRUPT_DPC         EvtInterruptDpc;

    PFN_WDF_INTERRUPT_ENABLE      EvtInterruptEnable;

    PFN_WDF_INTERRUPT_DISABLE     EvtInterruptDisable;

} WDF_INTERRUPT_CONFIG_V1_9, *PWDF_INTERRUPT_CONFIG_V1_9;

//
// The interrupt config structure has changed post win8-Beta. This is a
// temporary definition to allow beta drivers to load on post-beta builds.
// Note that size of win8-beta and win8-postbeta structure is different only on
// non-x64 platforms, but the fact that size is same on amd64 is harmless because
// the struture gets zero'out by init macro, and the default value of the new
// field is 0 on amd64.
//
typedef struct _WDF_INTERRUPT_CONFIG_V1_11_BETA {
    ULONG              Size;

    //
    // If this interrupt is to be synchronized with other interrupt(s) assigned
    // to the same WDFDEVICE, create a WDFSPINLOCK and assign it to each of the
    // WDFINTERRUPTs config.
    //
    WDFSPINLOCK                     SpinLock;

    WDF_TRI_STATE                   ShareVector;

    BOOLEAN                         FloatingSave;

    //
    // DIRQL handling: automatic serialization of the DpcForIsr/WaitItemForIsr.
    // Passive-level handling: automatic serialization of all callbacks.
    //
    BOOLEAN                         AutomaticSerialization;

    //
    // Event Callbacks
    //
    PFN_WDF_INTERRUPT_ISR           EvtInterruptIsr;
    PFN_WDF_INTERRUPT_DPC           EvtInterruptDpc;
    PFN_WDF_INTERRUPT_ENABLE        EvtInterruptEnable;
    PFN_WDF_INTERRUPT_DISABLE       EvtInterruptDisable;
    PFN_WDF_INTERRUPT_WORKITEM      EvtInterruptWorkItem;

    //
    // These fields are only used when interrupt is created in
    // EvtDevicePrepareHardware callback.
    //
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptRaw;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptTranslated;

    //
    // Optional passive lock for handling interrupts at passive-level.
    //
    WDFWAITLOCK                     WaitLock;

    //
    // TRUE: handle interrupt at passive-level.
    // FALSE: handle interrupt at DIRQL level. This is the default.
    //
    BOOLEAN                         PassiveHandling;

} WDF_INTERRUPT_CONFIG_V1_11_BETA, *PWDF_INTERRUPT_CONFIG_V1_11_BETA;

//
// Public constructors
//
FxInterrupt::FxInterrupt(
    __in PFX_DRIVER_GLOBALS Globals
    ) :
    FxNonPagedObject(FX_TYPE_INTERRUPT, sizeof(FxInterrupt), Globals)
{

    m_Interrupt = NULL;

    m_OldIrql   = PASSIVE_LEVEL;

    m_EvtInterruptEnable = NULL;
    m_EvtInterruptDisable = NULL;

    m_PassiveHandling = FALSE;

    m_EvtInterruptIsr = NULL;
    m_EvtInterruptDpc = NULL;
    m_EvtInterruptWorkItem = NULL;

    m_CallbackLock = NULL;
    m_WaitLock = NULL;
    m_SystemWorkItem = NULL;

    m_DisposeWaitLock = FALSE;

    //
    // We want platform specific behavior for soft disconnect to avoid any
    // compat issues on existing platforms. In later versions (after 1.11) the
    // platform differenciation could be removed.
    //
#if defined(_ARM_)
    m_UseSoftDisconnect = TRUE;
#else
    m_UseSoftDisconnect = FALSE;
#endif

#if FX_IS_KERNEL_MODE
    KeInitializeDpc(&m_Dpc, _InterruptDpcThunk, this);

    m_Active = FALSE;
    m_InterruptCaptured = NULL;

#elif FX_IS_USER_MODE

    m_RdInterruptContext = NULL;
    m_InterruptWaitblock = NULL;
    m_CanQueue = FALSE;
    m_PassiveHandlingByRedirector = FALSE;
#endif

    m_Disconnecting = FALSE;
    m_IsEdgeTriggeredNonMsiInterrupt = FALSE;

    m_ShareVector = WdfUseDefault;

    m_AddedToList = FALSE;
    m_Connected = FALSE;
    m_ForceDisconnected = FALSE;
    m_Enabled = FALSE;
    m_FloatingSave = FALSE;

    m_WakeInterruptMachine = NULL;

    // This field is init later on.
    m_CreatedInPrepareHardware = FALSE;

    WDF_INTERRUPT_INFO_INIT(&m_InterruptInfo);
    m_CmTranslatedResource = NULL;

    Reset();

    // This is set up by Initialize
    m_SpinLock  = NULL;

    //
    // MSI Support
    //
    m_InterruptInfo.MessageNumber = 0;

    //
    // WdfIrqPolicyOneCloseProcessor is a safe policy to use. It ensures that
    // old devices continue to work without any change in their functionality.
    //
    m_Policy    = WdfIrqPolicyOneCloseProcessor;
    m_Priority  = WdfIrqPriorityUndefined;
    RtlZeroMemory(&m_Processors, sizeof(m_Processors));
    m_SetPolicy = FALSE;

    InitializeListHead(&m_PnpList);

    //
    // Driver writer can only create WDFINTERRUPTs, not delete them
    //
    MarkNoDeleteDDI(ObjectDoNotLock);
    MarkPassiveDispose(ObjectDoNotLock);
    MarkDisposeOverride(ObjectDoNotLock);
}

FxInterrupt::~FxInterrupt()
{

    //
    // If this hits, its because someone destroyed the INTERRUPT by
    // removing too many references by mistake without calling WdfObjectDelete
    //
    if( m_Interrupt != NULL ) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Destroy Interrupt destroyed without calling WdfObjectDelete, or "
            "by Framework processing DeviceRemove. Possible reference count problem?");
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }

    if (m_Device != NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Destroy Interrupt destroyed without calling WdfObjectDelete, or "
            "by Framework processing DeviceRemove. Possible reference count problem?");
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }
}

_Must_inspect_result_
NTSTATUS
FxInterrupt::CreateWakeInterruptMachine(
    VOID
    )
/*++

Routine Description:
    This routine is used to create a state machine that
    is used to manage a wake capable interrupt machine

Arguments:

Return Value:
    None

--*/
{
    NTSTATUS status;
    FxWakeInterruptMachine * fxWakeInterruptMachine = NULL;

    ASSERT(NULL == m_WakeInterruptMachine);

    fxWakeInterruptMachine = new (m_Device->m_PkgPnp->GetDriverGlobals())
                            FxWakeInterruptMachine(this);

    if (NULL == fxWakeInterruptMachine) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE 0x%p failed to allocate FxWakeInterruptMachine. %!STATUS!.",
            m_Device, status);
        goto exit;
    }

    status = fxWakeInterruptMachine->Initialize(m_Device->m_PkgPnp->GetDriverGlobals());
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE 0x%p failed to initialize FxWakeInterruptMachine. %!STATUS!.",
            m_Device, status);
        goto exit;
    }

    status = fxWakeInterruptMachine->Init(
                            m_Device->m_PkgPnp,
                            FxWakeInterruptMachine::_ProcessEventInner,
                            fxWakeInterruptMachine
                            );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(),
            TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE 0x%p failed to init FxWakeInterruptMachine. %!STATUS!.",
            m_Device, status);
        goto exit;
    }

    m_WakeInterruptMachine = fxWakeInterruptMachine;

    fxWakeInterruptMachine->m_IsrEvent.Initialize(SynchronizationEvent,FALSE);

    m_Device->m_PkgPnp->WakeInterruptCreated();

    DoTraceLevelMessage(
        GetDriverGlobals(),
        TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "WDFDEVICE 0x%p created wake interrupt",
        m_Device);

exit:
    if (!NT_SUCCESS(status)) {
        if (NULL != fxWakeInterruptMachine) {
            delete fxWakeInterruptMachine;
        }
    }
    return status;
}

VOID
FxInterrupt::InvokeWakeInterruptEvtIsr(
    VOID
/*++

Routine Description:
    This routine is called by the interrupt wake machine to invoke
    the Evt for the ISR

Arguments:

Return Value:
    None

--*/
    )
{
    ASSERT(m_PassiveHandling);
    ASSERT(m_WakeInterruptMachine != NULL);

    //
    // Acquire our internal passive-lock after the kernel acquired its own
    // passive-lock and before invoking the callback.
    //
    AcquireLock();

    m_WakeInterruptMachine->m_Claimed = m_EvtInterruptIsr(
                                            GetHandle(),
                                            m_InterruptInfo.MessageNumber);
    ReleaseLock();
}

BOOLEAN
FxInterrupt::WakeInterruptIsr(
    VOID
    )
/*++

Routine Description:
    This is the ISR for a wake interrupt. This queues an event into the
    state machine. State machine will take care of waking the device if
    the device is in Dx and then invoking the driver callback for the
    interrupt.

Arguments:

Return Value:
    None

--*/
{
    ASSERT(m_WakeInterruptMachine != NULL);

    //
    // Queue an event in the state machine
    //
    m_WakeInterruptMachine->ProcessEvent(WakeInterruptEventIsr);

    GetDriverGlobals()->WaitForSignal(m_WakeInterruptMachine->m_IsrEvent.GetSelfPointer(),
                                      "Wake Interrupt ISR is stuck waiting for the device"
                                      "to power back up and driver calllback to be processed",
                                      GetHandle(),
                                      GetDriverGlobals()->DbgWaitForWakeInterruptIsrTimeoutInSec,
                                      WaitSignalBreakUnderVerifier|WaitSignalBreakUnderDebugger);

    //
    // State machine stores the return value of the callback in the
    // m_Claimed member variable
    //
    return m_WakeInterruptMachine->m_Claimed;
}


_Must_inspect_result_
NTSTATUS
FxInterrupt::_CreateAndInit(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CfxDevice * Device,
    __in_opt FxObject * Parent,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in PWDF_INTERRUPT_CONFIG Configuration,
    __out FxInterrupt ** Interrupt
    )
{
    FxInterrupt * pFxInterrupt;
    NTSTATUS status;

    pFxInterrupt = new (FxDriverGlobals, Attributes)
        FxInterrupt(FxDriverGlobals);

    if (pFxInterrupt == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "not enough memory to allocate WDFINTERRUPT for WDFDEVICE %p, "
            "%!STATUS!", Device, status);

        return status;
    }

    if (NULL == Parent) {
        Parent = Device;
    }

    status = pFxInterrupt->Initialize(Device, Parent, Configuration);

    if (NT_SUCCESS(status)) {
        status = pFxInterrupt->Commit(Attributes, NULL, Parent);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "FxInterrupt Commit failed %!STATUS!", status);
        }
    }

    if (NT_SUCCESS(status)) {
        *Interrupt = pFxInterrupt;
    }
    else {
        pFxInterrupt->DeleteFromFailedCreate();
        return status;
    }

    if (Configuration->CanWakeDevice) {
        status = pFxInterrupt->CreateWakeInterruptMachine();
    }

    if (!NT_SUCCESS(status)) {
        pFxInterrupt->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxInterrupt::Initialize(
    __in CfxDevice* Device,
    __in FxObject*  Parent,
    __in PWDF_INTERRUPT_CONFIG Configuration
    )
{
    NTSTATUS    status;
    FxPkgPnp*   pkgPnp;

    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescRaw;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescTrans;

    //
    // DIRQL handling: FxInterrupt can be parented by, and optionally
    //     serialize its DpcForIsr or WorItemForIsr with an FxDevice or
    //     FxIoQueue.
    //
    // Passive handling: FxInterrupt can be parented by, and optionally
    //     serialize its WorItemForIsr with an FxDevice or FxIoQueue.

    //
    // Add a reference to the device object we are associated with.  We will be
    // notified in Dispose to release this reference.
    //
    Device->ADDREF(this);

    //
    // It is important to store the Device only after taking the reference
    // because Dispose checks for m_Device != NULL to release the reference.  If
    // assign it here and then take the reference later, we can return failure
    // in between the assignment and reference and then release a reference that
    // was not taken in Dispose.
    //
    m_Device = Device;
    pkgPnp = m_Device->m_PkgPnp;

    //
    // NOTE:  Since Dispose always releases this reference, we *must* take this
    //        reference, so that even if we return error, the reference is
    //        still there to be removed on cleanup.
    //
    ADDREF((PVOID)_InterruptThunk);

    //
    // Values always supplied by the caller
    //
    m_ShareVector           = Configuration->ShareVector;
    m_FloatingSave          = Configuration->FloatingSave;

    m_EvtInterruptEnable    = Configuration->EvtInterruptEnable;
    m_EvtInterruptDisable   = Configuration->EvtInterruptDisable;

    //
    // Do further initialization
    //
    status = InitializeWorker(Parent, Configuration);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Update the message number used for MSI support
    //
    m_InterruptInfo.MessageNumber = pkgPnp->m_InterruptObjectCount;

    //
    // This logic is executed when the driver creates interrupts in its
    // EvtDevicePrepareHardware callback.
    //
    if (Configuration->InterruptRaw != NULL) {

        ASSERT(Configuration->InterruptTranslated != NULL);
        ASSERT(m_Device->GetDevicePnpState() != WdfDevStatePnpInit);

        m_CreatedInPrepareHardware = TRUE;

        //
        // Get the real resource descriptors not the copies.
        //
        cmDescRaw = &(CONTAINING_RECORD(Configuration->InterruptRaw,
                                        FxResourceCm,
                                        m_DescriptorClone))->m_Descriptor;

        cmDescTrans = &(CONTAINING_RECORD(Configuration->InterruptTranslated,
                                          FxResourceCm,
                                          m_DescriptorClone))->m_Descriptor;
        //
        // Assign resources to this interrupt.
        //
        FxInterrupt::AssignResources(cmDescRaw, cmDescTrans);
    }

    // Add this interrupt to the list of them being kept in the PnP package.
    pkgPnp->AddInterruptObject(this);

    m_AddedToList = TRUE;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxInterrupt::InitializeWorker(
    __in FxObject*  Parent,
    __in PWDF_INTERRUPT_CONFIG Configuration
    )
{
    FxObject*           tmpObject;
    IFxHasCallbacks*    callbacks;
    NTSTATUS            status;
    CfxDeviceBase*      deviceBase;
    BOOLEAN             passiveCallbacks;

    const PFX_DRIVER_GLOBALS fxDriverGlobals = GetDriverGlobals();
    const WDFTYPE parentType = Parent->GetType();

    //
    // Init interrupt's callbacks.
    //
    m_EvtInterruptIsr       = Configuration->EvtInterruptIsr;
    m_EvtInterruptDpc       = Configuration->EvtInterruptDpc;
    m_EvtInterruptWorkItem  = Configuration->EvtInterruptWorkItem;

    //
    // Init soft disconnect configuration
    //
    switch (Configuration->ReportInactiveOnPowerDown) {
    case WdfTrue:
        m_UseSoftDisconnect = TRUE;
        break;

    case WdfFalse:
        m_UseSoftDisconnect = FALSE;
        break;

    case WdfUseDefault:
    default:
        //
        // Leave the driver's value alone.
        //
        break;
    }

    //
    // TRUE if passive-level interrupt handling is enabled for this interrupt.
    //
    m_PassiveHandling = Configuration->PassiveHandling;

    //
    // If the caller specified a spinlock we use it.
    //
    if (Configuration->SpinLock != NULL) {
        FxSpinLock* pFxSpinLock;

        FxObjectHandleGetPtr(GetDriverGlobals(),
                             Configuration->SpinLock,
                             FX_TYPE_SPIN_LOCK,
                             (PVOID*)&pFxSpinLock);

        pFxSpinLock->SetInterruptSpinLock();

        m_SpinLock = pFxSpinLock->GetLock();
    }
    else if (m_PassiveHandling == FALSE) {
        //
        // If the caller does not specify a spinlock, and this is
        // a DIRQL interrupt we use a built in one.
        // Originally this logic was added to allow Acquire/Release Lock
        // to work on W2K platforms that does not support
        // KeAcquireInterruptSpinLock.
        //
        m_SpinLock = &m_BuiltInSpinLock.Get();
    }

    //
    // Automatic serialization: the DPC or work-item is synchronized with
    // the parent's callback lock.
    //

    //
    // Get the first ancestor that implements callbacks.
    //
    deviceBase = FxDeviceBase::_SearchForDevice(Parent, &callbacks);

    //
    // Validate parent:
    // - the specified device (from API) must be one of the ancestors.
    // - the parent can only be a queue or a device.
    //
    if (m_DeviceBase == NULL || deviceBase != m_DeviceBase ||
        (parentType != FX_TYPE_QUEUE && parentType != FX_TYPE_DEVICE)) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "The specified object 0x%p is not a valid parent for a "
            "WDFINTERRUPT, WDF_INTERRUPT_CONFIG structure 0x%p passed, "
            "%!STATUS!", Parent->GetObjectHandle(), Configuration, status);

        return status;
    }

    //
    // If automatic-serialization is off, m_CallbackLock is NULL.
    // else if parent doesn't use locking, m_CallbackLock is NULL.
    // else if work-item ISR callback set, m_CallbackLock is a passive lock.
    // else m_CallbackLock is a spin-lock.
    //
    // Note: logic retrieves the parent's callback lock when automatic
    // serialization is on even if work-item or DPC callbacks are not set,
    // this is not to break the existing behavior/validation.
    //
    if (Configuration->EvtInterruptWorkItem != NULL) {
        passiveCallbacks = TRUE;
    }
    else if (Configuration->EvtInterruptDpc != NULL) {
        passiveCallbacks = FALSE;
    }
    else if (m_PassiveHandling) {
        passiveCallbacks = TRUE;
    }
    else {
        passiveCallbacks = FALSE;
    }

    status = _GetEffectiveLock( Parent,
                                callbacks,      // IFxHasCallbacks*
                                Configuration->AutomaticSerialization,
                                passiveCallbacks,
                                &m_CallbackLock,
                                &tmpObject      // No reference count is taken.
                                );
    if (!NT_SUCCESS(status)) {
        //
        // We should never incur this error.
        //
        ASSERT(status != STATUS_WDF_INCOMPATIBLE_EXECUTION_LEVEL);
        return status;
    }

    //
    // If the parent is a queue, the queue inherits the deletion constraints of the
    // interrupt object, i.e., driver cannot manually delete the queue.
    //
    if (FX_TYPE_QUEUE == parentType) {
        ((FxIoQueue*)Parent)->SetInterruptQueue();
    }

    //
    // Passive-level handling: init wait-lock.
    //
    if (m_PassiveHandling) {
        ASSERT(NULL == m_SpinLock);

        //
        //If the caller specified a waitlock, we use it.
        //
        if (Configuration->WaitLock != NULL) {
            ASSERT(m_PassiveHandling);
            FxObjectHandleGetPtr(GetDriverGlobals(),
                                 Configuration->WaitLock,
                                 FX_TYPE_WAIT_LOCK,
                                 (PVOID*)&m_WaitLock);
        }

        //
        // Use a default lock if none was specified.
        //
        if (NULL == m_WaitLock) {
            WDFWAITLOCK waitLock = NULL;
            WDF_OBJECT_ATTRIBUTES attributes;

            WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

            status = FxWaitLock::_Create(
                            fxDriverGlobals,
                            &attributes,
                            NULL,
                            FALSE,
                            &waitLock);

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                                    "Could not allocate waitlock, %!STATUS!",
                                    status);
                return status;
            }

            FxObjectHandleGetPtr(fxDriverGlobals,
                                 waitLock,
                                 FX_TYPE_WAIT_LOCK,
                                 (PVOID*)&m_WaitLock);
            //
            // Explicitly dispose this wait-lock object.
            //
            m_DisposeWaitLock = TRUE;
        }
    }

    //
    // If needed, initialize the interrupt's workitem.
    // Alloacte workitem if driver specified EvtInterruptWorkitem.
    // In addition, for Umdf, allocate workitem if driver alternatively
    // specified EvtInterruptDpc. Note that driver is not allwed to specify both.
    //
    if (m_EvtInterruptWorkItem != NULL ||
        (FxLibraryGlobals.IsUserModeFramework && m_EvtInterruptDpc != NULL)) {
        status = FxSystemWorkItem::_Create(
                        fxDriverGlobals,
                        m_Device->GetDeviceObject(),
                        &m_SystemWorkItem);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                                "Could not allocate workitem, %!STATUS!",
                                status);
            return status;
        }
    }

    //
    // Do mode-apecific initialization
    //
    status = InitializeInternal(Parent, Configuration);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
FxInterrupt::Reset(
    VOID
    )
/*++

Routine Description:
    Resets the interrupt info and synchronize irql for the interrupt.  The pnp
    state machine will call this function every time new resources are assigned
    to the device.

Arguments:
    None

Return Value:
    None

  --*/
{
    //
    // Other values in m_InterruptInfo survive a reset, so RtlZeroMemory is not
    // an option.  Manually set the fields that need resetting.
    //
    m_InterruptInfo.TargetProcessorSet      = 0;
    m_InterruptInfo.Group                   = 0;
    m_InterruptInfo.Irql                    = PASSIVE_LEVEL;
    m_InterruptInfo.ShareDisposition        = 0;
    m_InterruptInfo.Mode                    = LevelSensitive;
    m_InterruptInfo.Vector                  = 0;

    m_SynchronizeIrql = PASSIVE_LEVEL;

    //
    // Do mode-specific reset.
    // For KMDF, it's a no-op.
    // For UMDF, a message is sent to reflector to reset the interrupt info.
    //
    ResetInternal();
}

//
// This is an API call from the device driver to delete this INTERRUPT.
//
VOID
FxInterrupt::DeleteObject(
    VOID
    )
{
    if (m_AddedToList) {
        //
        // Pop this off of PnP's list of interrupts.
        //
        m_Device->m_PkgPnp->RemoveInterruptObject(this);
    }

    if (m_CmTranslatedResource != NULL) {
        //
        // This can happen if driver explicitly deletes the interrupt in its
        // release hardware callback.
        //
        RevokeResources();
    }

    if (m_WakeInterruptMachine) {
        delete m_WakeInterruptMachine;
        m_WakeInterruptMachine = NULL;
    }

    //
    // Use the base FxObject's DeleteObject implementation which will Dispose us
    //
    FxNonPagedObject::DeleteObject(); // __super call
}

//
// Called by the PnP package after the driver's release hardware callback.
//
VOID
FxInterrupt::OnPostReleaseHardware(
    VOID
    )
{
    if (m_CreatedInPrepareHardware) {
        // Delete this interrupt.
        DeleteObject();
    }
}

PWDF_INTERRUPT_INFO
FxInterrupt::GetInfo(
    VOID
    )
{
    return &m_InterruptInfo;
}

//
// Called from the parent when the parent is being removed.
//
// Must ensure that any races with Delete are handled properly
//
BOOLEAN
FxInterrupt::Dispose(
    VOID
    )
{
    // MarkPassiveDispose() in Initialize ensures this
    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    FlushAndRundown();

    return TRUE;
}

VOID
FxInterrupt::AssignResources(
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescRaw,
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescTrans
    )
/*++

Routine Description:

    This function allows an interrupt object to know what resources have been
    assigned to it.  It will be called as part of IRP_MN_START_DEVICE.

Arguments:

    CmDescRaw - A CmResourceDescriptor that describes raw interrupt resources

    CmDescTrans - A CmResourceDescriptor that describes translated interrupt
        resources

Return Value:

    VOID

--*/
{
    if (CmDescTrans->u.Interrupt.Group > 0 &&
        FxIsProcessorGroupSupported() == FALSE) {
        //
        // This should never happen.
        //
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }

#if FX_IS_USER_MODE
    //
    // For UMDF, see if this is level-triggered interrupt in which case we need
    // reflector to handle it at passive level. Also, level-triggered
    // support is present only on Win8 and newer. Note that, for KMDF, driver
    // would have provided the choice of handling at passive level so we know
    // that early on for KMDF, however for UMDF, driver can't specify the choice
    // and UMDF figures out whether to handle at passive or not by looking at
    // the interrupt type in resources.
    //
    if (IsLevelTriggered(CmDescTrans->Flags) &&
        FxIsPassiveLevelInterruptSupported()) {
        m_PassiveHandlingByRedirector = TRUE;
    }
#endif

    if (IsPassiveConnect() && _IsMessageInterrupt(CmDescTrans->Flags)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Driver cannot specify PassiveHandling for MSI interrupts.");
        FxVerifierDbgBreakPoint(GetDriverGlobals());
        // IoConnectInterruptEx will fail later on.
    }

    m_InterruptInfo.Group                   = CmDescTrans->u.Interrupt.Group;
    m_InterruptInfo.TargetProcessorSet      = CmDescTrans->u.Interrupt.Affinity;
    m_InterruptInfo.ShareDisposition        = CmDescTrans->ShareDisposition;
    m_InterruptInfo.Mode                    =
        CmDescTrans->Flags & CM_RESOURCE_INTERRUPT_LATCHED ? Latched : LevelSensitive;

    //
    // Interrupt's IRQL.
    //
    m_InterruptInfo.Irql = (KIRQL)CmDescTrans->u.Interrupt.Level;

    if (IsPassiveConnect()) {
        m_InterruptInfo.Irql = PASSIVE_LEVEL;
    }

    //
    //  Note if this is an MSI interrupt
    //
    m_InterruptInfo.MessageSignaled = _IsMessageInterrupt(CmDescTrans->Flags);

    //
    // Edge-triggered interrupts that are ActiveBoth are made stateful by the OS
    // (GPIO buttons driver) to track buttons' press/release state. This is because
    // the driver may not have the ability to read the state directly from the hardware.
    //
    // There is no way to identify ActiveBoth interrupts since KINTERRUPT_POLARITY is
    // not exposed to client drivers, so we decided to apply this logic to all
    // edge-triggered non-MSI interrupts.
    //
    m_IsEdgeTriggeredNonMsiInterrupt = (m_InterruptInfo.Mode == Latched &&
        m_InterruptInfo.MessageSignaled == FALSE);

    if (m_InterruptInfo.MessageSignaled &&
        CmDescRaw->u.MessageInterrupt.Raw.MessageCount > 1) {
        //
        // This is an assignment for a multi-message PCI 2.2-style resource.
        // Thus the vector and message data have to be deduced.
        //
        m_InterruptInfo.Vector = CmDescTrans->u.Interrupt.Vector + m_InterruptInfo.MessageNumber;

        m_Device->SetDeviceTelemetryInfoFlags(DeviceInfoMsi22MultiMessageInterrupt);
    }
    else {
        //
        // This is an assignment for a single interrupt, either line-based or
        // PCI 2.2 single-message MSI, or PCI 3.0 MSI-X-style resource.
        //
        m_InterruptInfo.Vector = CmDescTrans->u.Interrupt.Vector;

        if (m_InterruptInfo.MessageSignaled) {
            m_Device->SetDeviceTelemetryInfoFlags(DeviceInfoMsiXOrSingleMsi22Interrupt);
        }
        else {
            if (IsLevelTriggered(CmDescTrans->Flags)) {
                m_Device->SetDeviceTelemetryInfoFlags(DeviceInfoLineBasedLevelTriggeredInterrupt);
            }
            else {
                m_Device->SetDeviceTelemetryInfoFlags(DeviceInfoLineBasedEdgeTriggeredInterrupt);
            }
        }
    }

    if (IsPassiveConnect()) {
        m_Device->SetDeviceTelemetryInfoFlags(DeviceInfoPassiveLevelInterrupt);
    }

    //
    // Do mode-specific work. For KMDF, it's a no-op.
    // For UMDF, send a sync message to Reflector to assign resources.
    //
    AssignResourcesInternal(CmDescRaw, CmDescTrans, &m_InterruptInfo);

    //
    // Weak ref to the translated resource interrupt descriptor.
    // It is valid from prepare hardware callback to release hardware callback.
    //
    m_CmTranslatedResource = CmDescTrans;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "Is MSI? %d, MSI-ID %d, AffinityPolicy %!WDF_INTERRUPT_POLICY!, "
        "Priority %!WDF_INTERRUPT_PRIORITY!, Group %d, Affinity 0x%I64x, "
        "Irql 0x%x, Vector 0x%x\n",
        m_InterruptInfo.MessageSignaled,
        m_InterruptInfo.MessageNumber,
        m_Policy,
        m_Priority,
        m_InterruptInfo.Group,
        (ULONGLONG)m_InterruptInfo.TargetProcessorSet,
        m_InterruptInfo.Irql,
        m_InterruptInfo.Vector);

}

VOID
FxInterrupt::RevokeResources(
    VOID
    )
/*++

Routine Description:

    This function tells an interrupt object that it no longer owns any resources.

Arguments:

    none

Return Value:

    VOID

--*/
{
    ULONG messageNumber;

    //
    // The message # doesn't change, so we must preserve it.
    //
    messageNumber = m_InterruptInfo.MessageNumber;

    //
    // This will zero out all the fields and init the size (as the structure
    // can be resued again say after a rebalance).
    //
    WDF_INTERRUPT_INFO_INIT(&m_InterruptInfo);

    m_InterruptInfo.MessageNumber = messageNumber;

    //
    // Used by interrupts created during 'EvtDevicePrepareHardware' callback.
    //
    m_CmTranslatedResource = NULL;

    //
    // Do mode-specific work. For KMDF, it's a no-op.
    // For UMDF, send a sync message to Reflector.
    //
    RevokeResourcesInternal();
}

VOID
FxInterrupt::SetPolicy(
    __in WDF_INTERRUPT_POLICY   Policy,
    __in WDF_INTERRUPT_PRIORITY Priority,
    __in PGROUP_AFFINITY        TargetProcessorSet
    )
/*++

Routine Description:

    This function fills in the policy member variables.  These values will be
    used in IRP_MN_FILTER_RESOURCE_REQUIREMENTS.

Arguments:

    Policy  - Strategy for assigning target processors

    Priority - DIRQL preference

    TargetProcessorSet - Processors which should receive this interrupt, if
        the policy is "SpecifyProcessors."

Return Value:

    VOID

--*/
{
    //
    // We cannot apply policy for interrupts created during prepare hardware.
    //
    if (m_CreatedInPrepareHardware) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "You cannot apply policy at this stage for WDFINTERRUPT 0x%p "
            "For interrupts created in EvtDevicePrepareHardware you must use "
            "EvtDeviceFilter APIs or use a pre-process routine to handle the "
            "IRP_MN_FILTER_RESOURCE_REQUIREMENT, %!STATUS!",
            GetHandle(), STATUS_INVALID_DEVICE_REQUEST);
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }

    m_Policy            = Policy;
    m_Priority          = Priority;
    m_Processors        = *TargetProcessorSet;

    //
    // Make sure OS supports processor groups, default to group 0 otherwise.
    //
    if (FxIsProcessorGroupSupported() == FALSE) {
       m_Processors.Group  = 0;
    }

    m_SetPolicy = TRUE;

    //
    // Do mode-specific work. This function does nothing for KMDF.
    // It sends a message to reflector for UMDF.
    //
    SetPolicyInternal(Policy, Priority, TargetProcessorSet);
}

_Must_inspect_result_
NTSTATUS
FxInterrupt::Connect(
    __in ULONG NotifyFlags
    )
/*++

Routine Description:

    This function is the external interface for connecting the interrupt.  It
    calls the PnP manager to connect the interrupt (only if the operation is
    not occurring in a non power pageable state).  Then it calls
    EvtInterruptEnable at DIRQL and EvtInterruptPostEnable at PASSIVE_LEVEL.

Arguments:
    NotifyFlags - combination of values from the enum NotifyResourcesFlags

Return Value:

    NTSTATUS

--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    pFxDriverGlobals = GetDriverGlobals();

    if ((NotifyFlags & NotifyResourcesExplicitPowerup) &&
        IsActiveForWake()) {
        //
        // If an interrupt is marked as wakeable and the device has been set to
        // wake via a driver-owned ISR, leave the interrupt connected.
        //
        SetActiveForWake(FALSE);

        return STATUS_SUCCESS;
    }

    //
    // See if we need to just do soft connect. We do soft connect on explicit
    // power up if driver opted-in for that.
    //
    if (IsSoftDisconnectCapable() &&
        (NotifyFlags & NotifyResourcesExplicitPowerup)){

        ReportActive(TRUE);

        status = STATUS_SUCCESS;
        goto Enable;
    }

    //
    // We should either be disconnected or being asked to connect in the NP path
    //
    ASSERT(m_Connected == FALSE || (NotifyFlags & NotifyResourcesNP));

    if (m_ForceDisconnected) {
        return STATUS_SUCCESS;
    }

    //
    // Check to see if this interrupt object was actually assigned any
    // resources.  If it wasn't, then don't attempt to connect.  A WDFINTERRUPT
    // object won't be assigned any resources if the underlying device wasn't
    // granted any by the PnP manager.
    //
    if (m_InterruptInfo.Vector == 0) {
        return STATUS_SUCCESS;
    }

    //
    // If we are in an NP path, the interrupt remained connected while the device
    // went into Dx so there is no need to reconnect it.
    //
    if ((NotifyFlags & NotifyResourcesNP) == 0) {

        ASSERT(m_Interrupt == NULL);
        ASSERT(m_SynchronizeIrql != PASSIVE_LEVEL || m_PassiveHandling);

        //
        // Call pnp manager to connect the interrupt. For KMDF, we call
        // kernel DDI, whereas for UMDF, we send a sync message to redirector.
        //
        status = ConnectInternal();

        if (!NT_SUCCESS(status)) {
            m_Interrupt = NULL;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "IoConnectInterrupt(Ex) Failed,"
                " SpinLock 0x%p,"
                " Vector 0x%x,"
                " IRQL 0x%x,"
                " Synchronize IRQL 0x%x,"
                " Mode 0x%x,"
                " ShareVector %s,"
                " ProcessorGroup %d,"
                " ProcessorEnableMask 0x%I64x,"
                " FloatingSave %s,"
                " %!STATUS!",
                m_SpinLock,
                m_InterruptInfo.Vector,
                m_InterruptInfo.Irql,
                m_SynchronizeIrql,
                m_InterruptInfo.Mode,
                m_InterruptInfo.ShareDisposition ==
                    CmResourceShareShared ? "True" : "False",
                m_InterruptInfo.Group,
                (ULONGLONG)m_InterruptInfo.TargetProcessorSet,
                m_FloatingSave ? "True" : "False",
                status
                );

            return status;
        }

        m_Connected = TRUE;

#if FX_IS_KERNEL_MODE
        m_Active = TRUE;
#endif

    }
    else {
        ASSERT(m_Connected);
        ASSERT(m_Interrupt != NULL);
    }

Enable:

    //
    // Enable the interrupt at the hardware.
    //
    status = InterruptEnable();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtInterruptEnable WDFDEVICE %p, WDFINTERRUPT %p, PKINTERRUPT %p "
            "returned %!STATUS!", m_Device->GetHandle(), GetHandle(),
            m_Interrupt, status);

        return status;
    }

    m_Enabled = TRUE;

    return status;
}

_Must_inspect_result_
NTSTATUS
FxInterrupt::Disconnect(
    __in ULONG NotifyFlags
    )
/*++

Routine Description:

    This function is the external interface for disconnecting the interrupt.  It
    calls the Io manager to disconnect.  Then it calls EvtInterruptPreDisable at
    PASSIVE_LEVEL and EvtInterruptDisable at DIRQL.

Arguments:

    Surprise - Indicates that we are disconnecting due to a surprise-remove,
               which means that we shouldn't do anything that touches hardware.

Return Value:

    NTSTATUS

--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status, finalStatus;

    finalStatus = STATUS_SUCCESS;
    pFxDriverGlobals = GetDriverGlobals();

    //
    // Check to see if this interrupt object was actually assigned any
    // resources.  If it wasn't, then don't attempt to disconnect.  A
    // WDFINTERRUPT object won't be assigned any resources if the underlying
    // device wasn't granted any by the PnP manager.
    //

    if (m_InterruptInfo.Vector == 0) {
        return STATUS_SUCCESS;
    }

    if (IsWakeCapable() &&
        (NotifyFlags & NotifyResourcesArmedForWake)) {
        //
        // If an interrupt is marked as wakeable and the device has been set to
        // wake via a driver-owned ISR, leave the interrupt connected.
        //
        ASSERT(NotifyFlags & NotifyResourcesExplicitPowerDown);

        SetActiveForWake(TRUE);

        return STATUS_SUCCESS;
    }

    //
    // This takes care of the power-up failure path for interrupt that doesnt
    // support soft disconnect. The interrupt has already been hard
    // disconnected in power-up failure path.
    //
    if ((NotifyFlags & NotifyResourcesDisconnectInactive) &&
        (IsSoftDisconnectCapable() == FALSE) &&
        (IsActiveForWake() == FALSE)) {
        //
        // We got here to disconnect an inactive interrupt. But if
        // this interrupt is not Soft Disconnect capable then it was
        // never made inactive in the first place so nothing to do here.
        // It should already be hard disconnected by now.
        //
        ASSERT(NotifyFlags & NotifyResourcesForceDisconnect);

        return STATUS_SUCCESS;
    }


    if (m_Connected == FALSE) {
        //
        // No way we can be not connect and enabled
        //
        ASSERT(m_Enabled == FALSE);

        //
        // if m_Connected is FALSE because the driver forcefully disconnected
        // the interrupt we still want to disconnect the actual interrupt object
        // if the caller wants to force disconnect (e.g., the device is being
        // removed)
        //
        if (m_Interrupt != NULL &&
            (NotifyFlags & NotifyResourcesForceDisconnect)) {
            //
            // If the driver lets the state machine handle the interrupt state
            // we should never get here so make sure the driver forced the issue.
            //
            ASSERT(m_ForceDisconnected);

            goto Disconnect;
        }

        return STATUS_SUCCESS;
    }

    if (m_Enabled && ((NotifyFlags & NotifyResourcesSurpriseRemoved) == 0)) {

        //
        //
        // For wake capable interrupts it is possible to enter this path
        // with NotifyResourcesDisconnectInactive flag if the device fails
        // to power up after the interrupt was left connected during Dx
        //
        if (IsWakeCapable() == FALSE) {
            ASSERT((NotifyFlags & NotifyResourcesDisconnectInactive) == 0);
        }

        //
        // Disable the interrupt at the hardware.
        //
        status = InterruptDisable();

        m_Enabled = FALSE;

        if (!NT_SUCCESS(status)) {
            //
            // Even upon failure we continue because we don't want to leave
            // the interrupt connected when we tear down the stack.
            //
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "EvtInterruptDisable WDFDEVICE %p, WDFINTERRUPT %p, "
                "PKINTERRUPT %p returned %!STATUS!",
                m_Device->GetHandle(),
                GetHandle(), m_Interrupt, status);

            //
            // Overwrite the previous value.  Not a big deal since both are
            // errors.
            //
            finalStatus = status;
        }
    }

#if FX_IS_KERNEL_MODE
    //
    // Some edge-triggered interrupts may fire before the connection process is
    // finished and m_Interrupt is set. To accomodate them, we save the KINTERRUPT
    // in _InterruptThunk to m_InterruptCaptured which serves as a backup for
    // m_Interrupt. Now we need to NULL m_InterruptCaptured and ensure that
    // _InterruptThunk will not re-capture it.
    //
    if (m_IsEdgeTriggeredNonMsiInterrupt == TRUE) {
        //
        // Synchronize the setting of m_Disconnecting with any running ISRs.
        // No new ISR callbacks will run after _SynchronizeExecution returns,
        // until m_Disconnecting is set to FALSE again.
        //
        if (m_Interrupt != NULL) {
            _SynchronizeExecution(m_Interrupt, _InterruptMarkDisconnecting, this);
        }

        //
        // Because m_Disconnecting was set, we know the ISR
        // will not re-capture the KINTERRUPT again.
        //
        m_InterruptCaptured = NULL;
    }
#endif

    //
    // Now flush queued callbacks so that we know that nobody is still trying to
    // synchronize against this interrupt. For KMDF this will flush DPCs and
    // for UMDF this will send a message to reflector to flush queued DPCs.
    //
    FlushQueuedDpcs();

#if FX_IS_KERNEL_MODE
    //
    // Rundown the workitem if present (passive-level interrupt support or KMDF).
    // Not needed for UMDF since reflector doesn't use workitem for isr.
    //
    FlushQueuedWorkitem();

#endif

    //
    // See if we need to just do soft disconnect. Soft disconnect is done only
    // during explicit power down.
    //
    if (IsSoftDisconnectCapable() &&
        (NotifyFlags & NotifyResourcesExplicitPowerDown)) {
        ReportInactive(TRUE);

        goto Exit;
    }

    //
    // In the NP path we disable the interrupt but do not disconnect the
    // interrupt.  (That is b/c IoDisconnectInterrupt is a pagable function and
    // calling it could cause paging I/O on this device which will be unserviceable
    // because it is in Dx.
    //
    if (NotifyFlags & NotifyResourcesNP) {
        //
        // If we are in the NP path, force disconnect should not be set.  Force
        // disconnect is setup during a query remove/stop power down.
        //
        ASSERT((NotifyFlags & NotifyResourcesForceDisconnect) == 0);

        goto Exit;
    }

Disconnect:
    //
    // Disconnect the interrupt. For KMDF, this calls the kernel DDI, and for
    // UMDF, sends a sync message to reflector.
    //
    DisconnectInternal();

    if (IsActiveForWake()) {
        //
        // Since the interrupt has been disconnected, it not longer active
        // for wake
        //
        SetActiveForWake(FALSE);
    }

    m_Connected = FALSE;

#if FX_IS_KERNEL_MODE
    m_Active = FALSE;
#endif

Exit:
    m_Disconnecting = FALSE;

    return finalStatus;
}


_Must_inspect_result_
NTSTATUS
FxInterrupt::ForceDisconnect(
    VOID
    )
{
    ULONG flags;

    //
    // Since we have no context or coordination of power state when these calls
    // are made, our only recourse is to see if the device is power pagable or
    // not and use that as the basis for our flags.
    //
    if (m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) {
        flags = NotifyResourcesNoFlags;
    }
    else {
        flags = NotifyResourcesNP;
    }

    //
    // Log the event because the driver is not allow the state machine to handle
    // the interrupt's state.
    //
    DoTraceLevelMessage(
         GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
         "Force disconnect called on WDFDEVICE %p, WDFINTERRUPT %p, PKINTERRUPT %p",
         m_Device->GetHandle(), GetHandle(), m_Interrupt);

    m_ForceDisconnected = TRUE;

    return Disconnect(flags);
}

_Must_inspect_result_
NTSTATUS
FxInterrupt::ForceReconnect(
    VOID
    )
{
    ULONG flags;

    //
    // Since we have no context or coordination of power state when these calls
    // are made, our only recourse is to see if the device is power pagable or
    // not and use that as the basis for our flags.
    //
    if (m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) {
        flags = NotifyResourcesNoFlags;
    }
    else {
        flags = NotifyResourcesNP;
    }

    //
    // Log the event because the driver is not allow the state machine to handle
    // the interrupt's state.
    //
    DoTraceLevelMessage(
         GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
         "Force connect called on WDFDEVICE %p, WDFINTERRUPT %p, PKINTERRUPT %p",
         m_Device->GetHandle(), GetHandle(), m_Interrupt);

    m_ForceDisconnected = FALSE;

    return Connect(flags);
}

//
// Called by the system work item to finish the rundown
//
VOID
FxInterrupt::FlushAndRundown()
{
    FxObject* pObject;

    //
    // This called at PASSIVE_LEVEL which is required for
    // IoDisconnectInterrupt and KeFlushQueuedDpcs
    //
    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // If we have the KeFlushQueuedDpcs function call it
    // to ensure the DPC routine is no longer running before
    // we release the final reference to memory and the framework objects
    //
    FlushQueuedDpcs();

    //
    // Do mode-specific work.
    //
    FlushAndRundownInternal();

    //
    // Release the reference taken in FxInterrupt::Initialize
    //
    if (m_Device != NULL) {
        pObject = m_Device;
        m_Device = NULL;

        pObject->RELEASE(this);
    }

    //
    // Release the reference taken in FxInterrupt::Initialize
    //
    RELEASE((PVOID)_InterruptThunk);
}

//
// Enable interrupts
//
NTSTATUS
FxInterrupt::InterruptEnableInvokeCallback(
    VOID
    )
{
    NTSTATUS status;

    if (m_PassiveHandling) {
        //
        // Passive-level interrupt handling: acquire our internal passive-lock
        // after the kernel acquired its own passive-lock and before invoking
        // the callback.
        //
        AcquireLock();
        status = m_EvtInterruptEnable(GetHandle(),
                                      m_Device->GetHandle());
        ReleaseLock();
    }
    else {
        //
        // DIRQL interrupt handling: invoke the callback.
        //
        status = m_EvtInterruptEnable(GetHandle(),
                                      m_Device->GetHandle());
    }

    return status;
}

BOOLEAN
FxInterrupt::_InterruptEnableThunk(
    __in PVOID SyncContext
    )
{
    FxInterruptEnableParameters* p;

    p = (FxInterruptEnableParameters*) SyncContext;

    p->ReturnVal = p->Interrupt->InterruptEnableInvokeCallback();

    return TRUE;
}

NTSTATUS
FxInterrupt::InterruptEnable(
    VOID
    )
{
    FxInterruptEnableParameters params;

    params.Interrupt = this;
    params.ReturnVal = STATUS_SUCCESS;

    if (m_EvtInterruptEnable) {
        _SynchronizeExecution(m_Interrupt, _InterruptEnableThunk, &params);
    }

    return params.ReturnVal;
}

//
// Disable interrupts
//
NTSTATUS
FxInterrupt::InterruptDisableInvokeCallback(
    VOID
    )
{
    NTSTATUS status;

    if (m_PassiveHandling) {
        //
        // Passive-level interrupt handling: acquire our internal passive-lock
        // after the kernel acquired its own passive-lock and before invoking
        // the callback.
        //
        AcquireLock();
        status = m_EvtInterruptDisable(GetHandle(),
                                       m_Device->GetHandle());
        ReleaseLock();
    }
    else {
        //
        // DIRQL interrupt handling: invoke the callback.
        //
        status = m_EvtInterruptDisable(GetHandle(),
                                       m_Device->GetHandle());
    }

    return status;
}


BOOLEAN
FxInterrupt::_InterruptDisableThunk(
    __in PVOID SyncContext
    )
{
    FxInterruptDisableParameters* p;

    p = (FxInterruptDisableParameters*) SyncContext;

    p->ReturnVal = p->Interrupt->InterruptDisableInvokeCallback();

    return TRUE;
}

NTSTATUS
FxInterrupt::InterruptDisable(
    VOID
    )
{
    FxInterruptDisableParameters params;

    params.Interrupt = this;
    params.ReturnVal = STATUS_SUCCESS;

    if (m_EvtInterruptDisable != NULL) {
        _SynchronizeExecution(m_Interrupt, _InterruptDisableThunk, &params);
    }

    return params.ReturnVal;
}

BOOLEAN
FxInterrupt::QueueWorkItemForIsr(
    VOID
    )
{
    BOOLEAN queued;

    //
    // Using this function is optional,
    // but the caller better have registered a handler
    //
#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    ASSERT(m_EvtInterruptWorkItem != NULL);
#else
    ASSERT(m_EvtInterruptWorkItem != NULL || m_EvtInterruptDpc != NULL);
#endif

    if (Mx::MxGetCurrentIrql() > DISPATCH_LEVEL) {
        //
        // Note: if the caller runs at DIRQL, the function returns the result
        // of KeInsertQueueDpc() and not that of WorkItem.TryToQueue().
        // The latter result is lost. Docs should clarify this behavior.
        //
#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
        queued = Mx::MxInsertQueueDpc(&m_Dpc, this, NULL);
#else
        queued = FALSE;
        FX_VERIFY(INTERNAL, TRAPMSG("Not expected"));
#endif
    }
    else {
        queued = m_SystemWorkItem->TryToEnqueue(_InterruptWorkItemCallback, this);
    }

    return queued;
}

VOID
FxInterrupt::_InterruptWorkItemCallback(
    __in PVOID DeferredContext
    )
/*++

Routine Description:
   Thunk used to invoke EvtInterruptWorkItem at passive-level

--*/
{
    ASSERT(DeferredContext != NULL);
    ((FxInterrupt*)DeferredContext)->WorkItemHandler();
}

VOID
FxInterrupt::FlushQueuedWorkitem(
    VOID
    )
{
    if (m_SystemWorkItem != NULL) {
        m_SystemWorkItem->WaitForExit();
    }
}

#pragma prefast(push)
#pragma prefast(disable:__WARNING_UNEXPECTED_IRQL_CHANGE, "Used unannotated pointers previously")

VOID
FxInterrupt::AcquireLock(
    )
{
    if (FALSE == m_PassiveHandling) {
        struct _KINTERRUPT* kinterrupt = GetInterruptPtr();

        //
        // DIRQL interrupt handling.
        //
        ASSERTMSG("Can't synchronize when the interrupt isn't connected: ",
                  kinterrupt != NULL);

        if (NULL != kinterrupt) {
            m_OldIrql = Mx::MxAcquireInterruptSpinLock(kinterrupt);
        }
    }
    else {
        //
        // Passive-level interrupt handling when automatic serialization is off.
        //
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);
        ASSERT(m_WaitLock != NULL);
        m_WaitLock->AcquireLock(GetDriverGlobals(), NULL);
    }
}
#pragma prefast(pop)

BOOLEAN
FxInterrupt::TryToAcquireLock(
    )
{
    LONGLONG    timeout = 0;

    ASSERTMSG("TryToAcquireLock is only available for passive-level "
              "interrupt handling: ", m_PassiveHandling);

    if (FALSE == m_PassiveHandling) {
        return FALSE;
    }

    ASSERT(m_WaitLock != NULL);
    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Passive-level interrupt handling. Automatic serialization is off.
    //
    return FxWaitLockInternal::IsLockAcquired(
                m_WaitLock->AcquireLock(GetDriverGlobals(), &timeout)
                );
}

#pragma prefast(push)
#pragma prefast(disable:__WARNING_UNEXPECTED_IRQL_CHANGE, "Used unannotated pointers previously")

VOID
FxInterrupt::ReleaseLock(
    )
{
    if (FALSE == m_PassiveHandling) {
        struct _KINTERRUPT* kinterrupt = GetInterruptPtr();

        //
        // DIRQL interrupt handling.
        //
        ASSERTMSG("Can't synchronize when the interrupt isn't connected: ",
                  kinterrupt != NULL);

        if (NULL != kinterrupt) {
#pragma prefast(suppress:__WARNING_CALLER_FAILING_TO_HOLD, "Unable to annotate ReleaseLock for this case.");
            Mx::MxReleaseInterruptSpinLock(kinterrupt, m_OldIrql);
        }
    }
    else {
        //
        // Passive-level interrupt handling when automatic serialization is off.
        //
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);
        ASSERT(m_WaitLock != NULL);
#pragma prefast(suppress:__WARNING_CALLER_FAILING_TO_HOLD, "Unable to annotate ReleaseLock for this case.");
        m_WaitLock->ReleaseLock(GetDriverGlobals());
    }
}
#pragma prefast(pop)

BOOLEAN
FxInterrupt::_InterruptSynchronizeThunk(
    __in PVOID SyncContext
    )
{
    FxInterruptSyncParameters* p;
    BOOLEAN result;

    p = (FxInterruptSyncParameters*) SyncContext;

    if (p->Interrupt->m_PassiveHandling) {
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);
        //
        // Passive-level interrupt handling: acquire our internal passive-lock
        // after the kernel acquired its own passive-lock and before invoking
        // the callback.
        //
        p->Interrupt->AcquireLock();
        result = p->Callback(p->Interrupt->GetHandle(), p->Context);
        p->Interrupt->ReleaseLock();
    }
    else {
        result = p->Callback(p->Interrupt->GetHandle(), p->Context);
    }

    return result;
}

BOOLEAN
FxInterrupt::Synchronize(
    __in  PFN_WDF_INTERRUPT_SYNCHRONIZE Callback,
    __in  WDFCONTEXT                    Context
    )
{
    struct _KINTERRUPT* kinterrupt;
    FxInterruptSyncParameters params;

    params.Interrupt = this;
    params.Callback  = Callback;
    params.Context   = Context;

    kinterrupt = GetInterruptPtr();

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    ASSERTMSG("Can't synchronize when the interrupt isn't connected: ",
              kinterrupt != NULL);
#endif

    return _SynchronizeExecution(kinterrupt,
                                 _InterruptSynchronizeThunk,
                                 &params);
}

