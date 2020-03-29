#include "common/fxinterrupt.h"
#include "common/fxtelemetry.h"
#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"


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
    //m_EvtInterruptWorkItem = NULL;

    m_CallbackLock = NULL;
    m_WaitLock = NULL;
    m_SystemWorkItem = NULL;

    m_DisposeWaitLock = FALSE;

    //
    // We want platform specific behavior for soft disconnect to avoid any
    // compat issues on existing platforms. In later versions (after 1.11) the
    // platform differenciation could be removed.
    //
#if defined(_ARM_) || defined(_ARM64_)
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
    if ( m_Interrupt != NULL )
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Destroy Interrupt destroyed without calling WdfObjectDelete, or "
            "by Framework processing DeviceRemove. Possible reference count problem?");
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }

    if (m_Device != NULL)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Destroy Interrupt destroyed without calling WdfObjectDelete, or "
            "by Framework processing DeviceRemove. Possible reference count problem?");
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }
}

VOID
FxInterrupt::FilterResourceRequirements(
    __inout PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor
    )
/*++

Routine Description:

    This function allows an interrupt object to change the
    IoResourceRequirementsList that the PnP Manager sends during
    IRP_MN_FILTER_RESOURCE_REQUIREMENTS.  This function takes a single
    IoResourceDescriptor and applies default or user specified policy.

Arguments:

    IoResourceDescriptor - Pointer to descriptor that matches this interrupt object

Return Value:

    VOID

--*/
{
    //
    // Set sharing policy.
    //
    switch (m_ShareVector) {
    case WdfTrue:
        //
        // Override the bus driver's value, explicitly sharing this interrupt.
        //
        IoResourceDescriptor->ShareDisposition = CmResourceShareShared;
        break;

    case WdfFalse:
        //
        // Override the bus driver's value, explicitly claiming this interrupt
        // as non-shared.
        //
        IoResourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        break;

    case WdfUseDefault:
    default:
        //
        // Leave the bus driver's value alone.
        //
        break;
    }

    //
    // Apply policy.  Only do this if we are running on an OS which supports
    // the notion of Interrupt Policy and if the policy is not already included
    // by the bus driver based on registry settings.
    //
    /*if (FxLibraryGlobals.IoConnectInterruptEx != NULL &&
        m_SetPolicy &&
        (IoResourceDescriptor->Flags & CM_RESOURCE_INTERRUPT_POLICY_INCLUDED) == 0x0)
    {
        IoResourceDescriptor->Flags |= CM_RESOURCE_INTERRUPT_POLICY_INCLUDED;
        IoResourceDescriptor->u.Interrupt.AffinityPolicy      = (IRQ_DEVICE_POLICY)m_Policy;
        IoResourceDescriptor->u.Interrupt.PriorityPolicy      = (IRQ_PRIORITY)m_Priority;
        IoResourceDescriptor->u.Interrupt.TargetedProcessors  = m_Processors.Mask;
        IoResourceDescriptor->u.Interrupt.Group               = m_Processors.Group;
    }*/
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
    //
    // This code not from original WDF
    //

    if (CmDescTrans->u.MessageInterrupt.Raw.MessageCount &&
         FxLibraryGlobals.ProcessorGroupSupport == FALSE)
    {
        if (GetDriverGlobals()->FxVerifierDbgBreakOnError)
        {
            __debugbreak();
        }
    }

    m_InterruptInfo.Group = CmDescTrans->u.MessageInterrupt.Raw.MessageCount;
    m_InterruptInfo.TargetProcessorSet = CmDescTrans->u.Generic.Length;
    m_InterruptInfo.Irql = CmDescTrans->u.Generic.Start.QuadPart;
    m_InterruptInfo.ShareDisposition = CmDescTrans->ShareDisposition;
    m_InterruptInfo.Mode                    =
        CmDescTrans->Flags & CM_RESOURCE_INTERRUPT_LATCHED ? Latched : LevelSensitive;
    
    //
    //  Note if this is an MSI interrupt
    //
    m_InterruptInfo.MessageSignaled = _IsMessageInterrupt(CmDescTrans->Flags);

    if (m_InterruptInfo.MessageSignaled &&
        CmDescRaw->u.MessageInterrupt.Raw.MessageCount > 1)
    {
        //
        // This is an assignment for a multi-message PCI 2.2-style resource.
        // Thus the vector and message data have to be deduced.
        //
        m_InterruptInfo.Vector = CmDescTrans->u.Interrupt.Vector + m_InterruptInfo.MessageNumber;
    }
    else
    {
        //
        // This is an assignment for a single interrupt, either line-based or
        // PCI 2.2 single-message MSI, or PCI 3.0 MSI-X-style resource.
        //
        m_InterruptInfo.Vector = CmDescTrans->u.Interrupt.Vector;
    }

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
        IsActiveForWake())
    {
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
        (NotifyFlags & NotifyResourcesExplicitPowerup))
    {
        //
        // NOTE: Start from Windows 8
        //
        //ReportActive(TRUE);

        status = STATUS_SUCCESS;
        goto Enable;
    }

    //
    // We should either be disconnected or being asked to connect in the NP path
    //
    ASSERT(m_Connected == FALSE || (NotifyFlags & NotifyResourcesNP));

    if (m_ForceDisconnected)
    {
        return STATUS_SUCCESS;
    }

    //
    // Check to see if this interrupt object was actually assigned any
    // resources.  If it wasn't, then don't attempt to connect.  A WDFINTERRUPT
    // object won't be assigned any resources if the underlying device wasn't
    // granted any by the PnP manager.
    //
    if (m_InterruptInfo.Vector == 0)
    {
        return STATUS_SUCCESS;
    }

    //
    // If we are in an NP path, the interrupt remained connected while the device
    // went into Dx so there is no need to reconnect it.
    //
    if ((NotifyFlags & NotifyResourcesNP) == 0)
    {
        ASSERT(m_Interrupt == NULL);
        ASSERT(m_SynchronizeIrql != PASSIVE_LEVEL || m_PassiveHandling);

        //
        // Call pnp manager to connect the interrupt. For KMDF, we call
        // kernel DDI, whereas for UMDF, we send a sync message to redirector.
        //
        status = ConnectInternal();

        if (!NT_SUCCESS(status))
        {
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
    else
    {
        ASSERT(m_Connected);
        ASSERT(m_Interrupt != NULL);
    }

Enable:

    //
    // Enable the interrupt at the hardware.
    //
    status = InterruptEnable();
    if (!NT_SUCCESS(status))
    {
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
FxInterrupt::ConnectInternal(
    VOID
    )
{
    IO_CONNECT_INTERRUPT_PARAMETERS connectParams;
    FxPkgPnp* fxPkgPnp;
    NTSTATUS status;

    fxPkgPnp = m_Device->m_PkgPnp;

    //
    // Tell the PnP Manager to connect the interrupt.
    //    
    //ASSERT(fxPkgPnp->m_IoConnectInterruptEx != NULL);
    
    //
    // NOTE: Adaptation for WDF 1.9
    //
    if (fxPkgPnp->m_IoConnectInterruptEx == NULL)
    {
        status = IoConnectInterrupt(
           &m_Interrupt,
           FxInterrupt::_InterruptThunk,
           this,
           m_SpinLock,
           m_InterruptInfo.Vector,
           m_InterruptInfo.Irql,
           m_SynchronizeIrql,
           m_InterruptInfo.Mode,
           m_InterruptInfo.ShareDisposition == CmResourceShareShared,
           m_InterruptInfo.TargetProcessorSet,
           m_FloatingSave);

           return status;
    }
    
    //
    // We're running on Longhorn or later (or somebody backported the new
    // interrupt code,) so tell the PnP manager everything we can about this
    // device.
    //
    RtlZeroMemory(&connectParams, sizeof(connectParams));
    
    if (FxIsProcessorGroupSupported())
    {
        connectParams.Version = CONNECT_FULLY_SPECIFIED_GROUP;
    }
    else
    {
        connectParams.Version = CONNECT_FULLY_SPECIFIED;
    }

    connectParams.FullySpecified.PhysicalDeviceObject = m_Device->GetPhysicalDevice();
    connectParams.FullySpecified.InterruptObject      = &m_Interrupt;
    connectParams.FullySpecified.ServiceRoutine       = _InterruptThunk;
    connectParams.FullySpecified.ServiceContext       = this;
    connectParams.FullySpecified.SpinLock             = m_SpinLock;
    connectParams.FullySpecified.FloatingSave         = m_FloatingSave;
    connectParams.FullySpecified.Vector               = m_InterruptInfo.Vector;
    connectParams.FullySpecified.Irql                 = m_InterruptInfo.Irql;
    connectParams.FullySpecified.ProcessorEnableMask  = m_InterruptInfo.TargetProcessorSet;
    connectParams.FullySpecified.Group                = m_InterruptInfo.Group;
    connectParams.FullySpecified.InterruptMode        = m_InterruptInfo.Mode;
    connectParams.FullySpecified.ShareVector          =
        m_InterruptInfo.ShareDisposition == CmResourceShareShared ? TRUE : FALSE;
    connectParams.FullySpecified.SynchronizeIrql      = m_SynchronizeIrql;

    status = fxPkgPnp->m_IoConnectInterruptEx(&connectParams);

    return status;
}

NTSTATUS
FxInterrupt::InterruptEnable(
    VOID
    )
{
    FxInterruptEnableParameters params;

    params.Interrupt = this;
    params.ReturnVal = STATUS_SUCCESS;

    if (m_EvtInterruptEnable)
    {
        _SynchronizeExecution(m_Interrupt, _InterruptEnableThunk, &params);
    }

    return params.ReturnVal;
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

BOOLEAN
FxInterrupt::_InterruptThunk(
    __in struct _KINTERRUPT *Interrupt,
    __in PVOID              ServiceContext
    )

/*++

Routine Description:

    This is the C routine called by the kernels INTERRUPT handler

Arguments:

Return Value:

--*/

{
    FxInterrupt*    interrupt;
    BOOLEAN         result;

    UNREFERENCED_PARAMETER(Interrupt);

    interrupt = (FxInterrupt*)ServiceContext;
    ASSERT( interrupt->m_EvtInterruptIsr != NULL );

    if (interrupt->m_IsEdgeTriggeredNonMsiInterrupt == TRUE)
    {
        //
        // If KMDF is in the process of disconnecting this interrupt, discard it.
        //
        if (interrupt->m_Disconnecting == TRUE)
        {
            return FALSE;
        }

        //
        // Captures the interrupt object as a backup in case interrupts start
        // to arrive before IoConnectInterruptEx sets FxInterrupt.m_Interrupt.
        //
        interrupt->m_InterruptCaptured = Interrupt;
    }
    //
    // If the interrupt is not connected, treat this as spurious interrupt.
    //
    else if (NULL == interrupt->m_Interrupt)
    {
        return FALSE;
    }
    
    if (interrupt->IsWakeCapable())
    {
        //
        // if it is a wake capable interrupt, we will hand it off
        // to the state machine so that it can power up the device
        // if required and then invoke the ISR callback
        //
        ASSERT(interrupt->m_PassiveHandling);
        //
        // TODO: Implement function
        //
        //FxPerfTracePassiveInterrupt(&interrupt->m_EvtInterruptIsr);

        //
        // NOTE: not in WDF 1.9
        //
        __debugbreak();
        //return interrupt->WakeInterruptIsr();  
    }

    if (interrupt->m_PassiveHandling)
    {
        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);  
        
        //
        // Acquire our internal passive-lock after the kernel acquired its own 
        // passive-lock and before invoking the callback.
        //
        interrupt->AcquireLock();

        //
        // TODO: Implement function
        //
        //FxPerfTracePassiveInterrupt(&interrupt->m_EvtInterruptIsr);

        result = interrupt->m_EvtInterruptIsr(
                                interrupt->GetHandle(), 
                                interrupt->m_InterruptInfo.MessageNumber);
        interrupt->ReleaseLock();
    }
    else
    {
        //
        // TODO: Implement function
        //
        //FxPerfTraceInterrupt(&interrupt->m_EvtInterruptIsr);

        result = interrupt->m_EvtInterruptIsr(
                                interrupt->GetHandle(), 
                                interrupt->m_InterruptInfo.MessageNumber);
    }

    return result;
}

#pragma prefast(push)
#pragma prefast(disable:__WARNING_UNEXPECTED_IRQL_CHANGE, "Used unannotated pointers previously")

VOID
FxInterrupt::AcquireLock(
    )
{
    if (FALSE == m_PassiveHandling)
    {
        struct _KINTERRUPT* kinterrupt = GetInterruptPtr();
        
        //
        // DIRQL interrupt handling.
        //
        ASSERTMSG("Can't synchronize when the interrupt isn't connected: ",
                  kinterrupt != NULL);

        if (NULL != kinterrupt)
        {
            m_OldIrql = Mx::MxAcquireInterruptSpinLock(kinterrupt);
        }
    }
    else
    {
        //
        // Passive-level interrupt handling when automatic serialization is off.
        //
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);
        ASSERT(m_WaitLock != NULL);
        m_WaitLock->AcquireLock(GetDriverGlobals(), NULL);
    }
}
#pragma prefast(pop)

#pragma prefast(push)
#pragma prefast(disable:__WARNING_UNEXPECTED_IRQL_CHANGE, "Used unannotated pointers previously")

VOID
FxInterrupt::ReleaseLock(
    )
{
    if (FALSE == m_PassiveHandling)
    {
        struct _KINTERRUPT* kinterrupt = GetInterruptPtr();
        
        //
        // DIRQL interrupt handling.
        //
        ASSERTMSG("Can't synchronize when the interrupt isn't connected: ",
                  kinterrupt != NULL);

        if (NULL != kinterrupt)
        {
#pragma prefast(suppress:__WARNING_CALLER_FAILING_TO_HOLD, "Unable to annotate ReleaseLock for this case.");
            Mx::MxReleaseInterruptSpinLock(kinterrupt, m_OldIrql);
        }
    }
    else
    {
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

//
// Enable interrupts
//
NTSTATUS
FxInterrupt::InterruptEnableInvokeCallback(
    VOID
    )
{
    NTSTATUS status;

    if (m_PassiveHandling)
    {
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
    else
    {
        //
        // DIRQL interrupt handling: invoke the callback.
        //
        status = m_EvtInterruptEnable(GetHandle(),
                                      m_Device->GetHandle());
    }

    return status;
}

VOID
FxInterrupt::_InterruptDpcThunk(
    __in PKDPC Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
/*++

Routine Description:

   Thunk used to invoke EvtInterruptDpc at DPC-level, or to queue a work-item 
   for invoking EvtInterruptWorkItem at passive-level.

--*/
{
    FxInterrupt* interrupt;

    UNREFERENCED_PARAMETER(Dpc);

    ASSERT(DeferredContext != NULL);
    interrupt = (FxInterrupt*) DeferredContext;

    if (interrupt->m_SystemWorkItem == NULL)
    {
        //
        // TODO: Implement function
        //
        //FxPerfTraceDpc(&interrupt->m_EvtInterruptDpc);

        __debugbreak();
        //interrupt->DpcHandler(SystemArgument1, SystemArgument2);
    }
    else
    {
        interrupt->m_SystemWorkItem->TryToEnqueue(_InterruptWorkItemCallback, 
                                                  interrupt);
    }
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
    __debugbreak();
    //ASSERT(DeferredContext != NULL);
    //((FxInterrupt*)DeferredContext)->WorkItemHandler();
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
    if (m_Device != NULL)
    {
        pObject = m_Device;
        m_Device = NULL;

        pObject->RELEASE(this);
    }

    //
    // Release the reference taken in FxInterrupt::Initialize
    //
    RELEASE(_InterruptThunk);
}

VOID
FxInterrupt::FlushQueuedDpcs(
    VOID
    )
{
    KeFlushQueuedDpcs();
}

VOID
FxInterrupt::FlushAndRundownInternal(
    VOID
    )
{
    //
    // Rundown the workitem.
    //
    if (m_SystemWorkItem != NULL)
    {
        m_SystemWorkItem->DeleteObject();
        m_SystemWorkItem = NULL;
    }

    //
    // If present, delete the default passive-lock.
    //
    if (m_DisposeWaitLock)
    {
        ASSERT(m_WaitLock != NULL);
        m_WaitLock->DeleteObject();
        m_WaitLock = NULL;
        m_DisposeWaitLock = FALSE;
    }
}
