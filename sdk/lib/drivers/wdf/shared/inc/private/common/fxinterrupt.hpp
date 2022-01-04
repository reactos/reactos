/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxInterrupt.hpp

Abstract:

    This module implements a frameworks managed interrupt object

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXINTERRUPT_H_
#define _FXINTERRUPT_H_

#include "fxwakeinterruptstatemachine.hpp"

//
// We need two parameters for KeSynchronizeExecution when enabling
// and disabling interrupts, so we use this structure on the stack since its
// a synchronous call.
//
struct FxInterruptEnableParameters {
    FxInterrupt*        Interrupt;
    NTSTATUS            ReturnVal;
};

typedef FxInterruptEnableParameters FxInterruptDisableParameters;


class FxInterrupt : public FxNonPagedObject {

    friend FxPkgPnp;

private:

    //
    // User supplied configuration
    //
    WDF_TRI_STATE                   m_ShareVector;

    //
    // Kernel Interupt object
    //
    struct _KINTERRUPT*             m_Interrupt;

    //
    // Kernel spinlock for Interrupt
    //
    MdLock*                         m_SpinLock;

    KIRQL                           m_OldIrql;
    volatile KIRQL                  m_SynchronizeIrql;

    //
    // Built in SpinLock/PassiveLock
    //
    MxLock                          m_BuiltInSpinLock;

    //
    // Passive-level interrupt handling.
    //
    FxWaitLock*                     m_WaitLock;

    //
    // DpcForIsr and WorkItemForIsr support. Note that a DPC is still
    // needed even if the driver opts to use WorkItemForIsr when
    // driver handles interrupts at DIRQL.
    //
#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    KDPC                            m_Dpc;
#endif
    FxSystemWorkItem*               m_SystemWorkItem;

    //
    // Automatic serialization: this is the callback lock for the object the DPC or
    //       work-item will synchronize with.
    //
    FxCallbackLock*                 m_CallbackLock;

    //
    // Set to TRUE when WDF is responsible for disposing the wait-lock.
    //
    BOOLEAN                         m_DisposeWaitLock;

    //
    // Value provided by driver. When TRUE we use IoReportActive/Inactive to
    // do soft connect/disconnect on explicit power transitions.
    //
    BOOLEAN                         m_UseSoftDisconnect;

    //
    // Set to TRUE for passive-level interrupt handling.
    //
    BOOLEAN                         m_PassiveHandling;

    // set to TRUE once the interrupt has been added to the pnp package's
    // interrupt list
    BOOLEAN                         m_AddedToList;

    //
    // Indicates whether the driver has forced a disconnect.  If so, then
    // we should stop automatically managing the connected state.
    //
    BOOLEAN                         m_Connected;
    BOOLEAN                         m_ForceDisconnected;

    //
    // Indicates whether the m_EvtInterruptPostEnable succeeded or not.
    //
    BOOLEAN                         m_Enabled;

    //
    // Save floating point when the ISR runs
    //
    BOOLEAN                         m_FloatingSave;

    //
    // Set to TRUE if interrupt is created in the prepare hardware callback.
    //
    BOOLEAN                         m_CreatedInPrepareHardware;

    //
    // State machine to manage a wake capable interrupt
    //
    FxWakeInterruptMachine*         m_WakeInterruptMachine;


#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    //
    // Set to true on successful connect or when driver reports active.
    // (this field is mainly for aid in debugging)
    //
    BOOLEAN                         m_Active;
#endif

    //
    // Interrupt policy
    //
    BOOLEAN                         m_SetPolicy;
    WDF_INTERRUPT_POLICY            m_Policy;
    WDF_INTERRUPT_PRIORITY          m_Priority;
    GROUP_AFFINITY                  m_Processors;

    //
    // Callbacks
    //
    PFN_WDF_INTERRUPT_ENABLE        m_EvtInterruptEnable;
    PFN_WDF_INTERRUPT_DISABLE       m_EvtInterruptDisable;

    PFN_WDF_INTERRUPT_ISR           m_EvtInterruptIsr;
    PFN_WDF_INTERRUPT_DPC           m_EvtInterruptDpc;
    PFN_WDF_INTERRUPT_WORKITEM      m_EvtInterruptWorkItem;

#if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
    //
    // Rd interrupt object
    //
    RD_INTERRUPT_CONTEXT            m_RdInterruptContext;

    //
    // Each interrupt object has this structure which comprises an event and a
    // wait structure. The wait struture  is associted with interrupt's callback
    // and the event, and is queued to threadpool. The callback is invoked when
    // the event is set.
    //
    FxInterruptWaitblock* m_InterruptWaitblock;

    //
    // True if the interrupt callback can queue another interrupt wait.
    // Set to true when interrupt is connected and false when interrupts
    // callbacks and waits are flushed.
    //
    BOOLEAN m_CanQueue;

    //
    // UMDF's handling of interrupt is split in two parts:
    // 1. framwork code- runs at passive always and therefore uses mode-agnostic
    //    code meant for passive-level handling, tracked through m_PassiveLevel
    //    field of interrupt object.
    // 2. redirector code- does passive handling of all of level-triggered
    //    interrupt and DIRQL handing of all others (edge and msi). Driver
    //    doesn't have any choice in that. The PassiveHandling field in the
    //    interrupt config is always set for passive for UMDF (through UMDF's
    //    init function).
    //
    // This field stores the type of handling done by redirector as opposed to
    // m_PassiveHandling which stores user's choice.
    //
    BOOLEAN m_PassiveHandlingByRedirector;
#endif

    //
    // PnP data about the interrupt.
    //
    WDF_INTERRUPT_INFO              m_InterruptInfo;

    //
    // Weak ref to the translated resource interrupt descriptor.
    // It is valid from prepare hardware callback to release hardware callback.
    //
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  m_CmTranslatedResource;

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    //
    // Callback used to set m_Disconnecting, synchronized to running ISRs.
    // Only runs if m_IsEdgeTriggeredNonMsiInterrupt is TRUE.
    //
    static
    MdInterruptSynchronizeRoutineType _InterruptMarkDisconnecting;

    //
    // Backup KINTERRUPT pointer, captured from the KMDF ISR thunk. We need it
    // because valid interrupts may arrive before IoConnectInterruptEx sets
    // FxInterrupt.m_Interrupt. Non-NULL only if m_IsEdgeTriggeredNonMsiInterrupt is TRUE.
    //
    struct _KINTERRUPT* m_InterruptCaptured;
#endif

    //
    // Used to mark the interrupt disconnect window, and to discard interrupts
    // that arrive within this window. Only set if m_IsEdgeTriggeredNonMsiInterrupt is TRUE.
    //
    BOOLEAN m_Disconnecting;

    //
    // Set if this is an Edge-Triggered non-MSI interrupt. These interrupts are
    // stateful and it is important not to drop any around the connection window.
    //
    BOOLEAN m_IsEdgeTriggeredNonMsiInterrupt;

protected:

    LIST_ENTRY  m_PnpList;

public:
    FxInterrupt(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    ~FxInterrupt(
        VOID
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _CreateAndInit(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice * Device,
        __in_opt FxObject * Parent,
        __in PWDF_OBJECT_ATTRIBUTES Attributes,
        __in PWDF_INTERRUPT_CONFIG Configuration,
        __out FxInterrupt ** Interrupt
        );

    _Must_inspect_result_
    NTSTATUS
    CreateWakeInterruptMachine(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in CfxDevice* Device,
        __in FxObject*  Parent,
        __in PWDF_INTERRUPT_CONFIG Configuration
        );

    _Must_inspect_result_
    NTSTATUS
    InitializeWorker(
        __in FxObject*  Parent,
        __in PWDF_INTERRUPT_CONFIG Configuration
        );

    _Must_inspect_result_
    NTSTATUS
    InitializeInternal(
        __in FxObject*  Parent,
        __in PWDF_INTERRUPT_CONFIG Configuration
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    virtual
    VOID
    DeleteObject(
        VOID
        );

    VOID
    OnPostReleaseHardware(
        VOID
        );

    VOID
    DpcHandler(
        __in_opt PVOID SystemArgument1,
        __in_opt PVOID SystemArgument2
        );

    BOOLEAN
    QueueDpcForIsr(
        VOID
        );

    BOOLEAN
    Synchronize(
        __in  PFN_WDF_INTERRUPT_SYNCHRONIZE Callback,
        __in  WDFCONTEXT                    Context
        );

    struct _KINTERRUPT*
    GetInterruptPtr(
        VOID
        );

    __inline
    BOOLEAN
    IsWakeCapable(
        VOID
        )
    {
        return ((m_WakeInterruptMachine != NULL) ? TRUE:FALSE);
    }

    VOID
    SetActiveForWake(
        __in BOOLEAN ActiveForWake
        )
    {
        m_WakeInterruptMachine->m_ActiveForWake = ActiveForWake;
    }

    BOOLEAN
    IsActiveForWake(
        VOID
        )
    {
        if ((m_WakeInterruptMachine != NULL) &&
            (m_WakeInterruptMachine->m_ActiveForWake)) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    VOID
    ProcessWakeInterruptEvent(
        __in FxWakeInterruptEvents Event
        )
    {
        m_WakeInterruptMachine->ProcessEvent(Event);
    }


#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))

    VOID
    ReportActive(
        _In_ BOOLEAN Internal = FALSE
        );

    VOID
    ReportInactive(
        _In_ BOOLEAN Internal = FALSE
        );

    BOOLEAN
    IsSoftDisconnectCapable(
        VOID
        )
    {
        if (m_UseSoftDisconnect &&
            FxLibraryGlobals.IoReportInterruptInactive != NULL &&
            m_Interrupt != NULL &&
            m_Connected) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

#elif ((FX_CORE_MODE)==(FX_CORE_USER_MODE))

    BOOLEAN
    IsSoftDisconnectCapable(
        VOID
        )
    {
        //
        // Not implemented for UMDF
        //
        return FALSE;
    }

    VOID
    ReportActive(
        _In_ BOOLEAN Internal = FALSE
        )
    {
        UNREFERENCED_PARAMETER(Internal);
        //
        // Not implemented for UMDF
        //
    }

    VOID
    ReportInactive(
        _In_ BOOLEAN Internal = FALSE
        )
    {
        UNREFERENCED_PARAMETER(Internal);
        //
        // Not implemented for UMDF
        //
    }

#endif

    VOID
    WorkItemHandler(
        VOID
        );

    BOOLEAN
    QueueWorkItemForIsr(
        VOID
        );

    __inline
    BOOLEAN
    IsPassiveHandling(
        VOID
        )
    {
        return m_PassiveHandling;
    }

    __inline
    BOOLEAN
    IsPassiveConnect(
        VOID
        )
    {
        //
        // UMDF's handling of interrupt is split in two parts:
        // 1. framework code that runs at passive always in host process and
        //    therefore uses mode-agnostic code meant for passive-level handling,
        //    tracked through m_PassiveHandling member.
        //    field of interrupt object.
        // 2. redirector code that does passive handling of all of level-triggered
        //    interrupt and DIRQL handing of all others (edge and msi). Driver
        //    doesn't have any choice in that. The m_PassiveHandling field in the
        //    interrupt config is always set for passive for UMDF (through UMDF's
        //    init function). m_PasiveHandlingByRedirector member is present to
        //    this part of code.
        // In summary, m_PassiveHandling and m_PasiveHandlingByRedirector
        // effectively maintain how the interrupt is connected (passive or DIRQL),
        // for KMDF and UMDF respectively. This routine tells how the
        // interrupt is connnected by looking at these members.
        //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        return IsPassiveHandling();
#else
        return m_PassiveHandlingByRedirector;
#endif
    }

    __inline
    BOOLEAN
    IsAutomaticSerialization(
        VOID
        )
    {
        return m_CallbackLock != NULL ? TRUE : FALSE;
    }

    VOID
    AcquireLock(
        VOID
        );

    BOOLEAN
    TryToAcquireLock(
        VOID
        );

    VOID
    ReleaseLock(
        VOID
        );

    CfxDevice*
    GetDevice(
        VOID
        )
    {
        return m_Device;
    }

    PWDF_INTERRUPT_INFO
    GetInfo(
        VOID
        );

    WDFINTERRUPT
    GetHandle(
        VOID
        )
    {
        return (WDFINTERRUPT) GetObjectHandle();
    }

    BOOLEAN
    IsSharedSpinLock(
        VOID
        )
    {
        return m_SpinLock != &m_BuiltInSpinLock.Get() ? TRUE : FALSE;
    }

    BOOLEAN
    IsSyncIrqlSet(
        VOID
        )
    {
        return m_SynchronizeIrql != PASSIVE_LEVEL ? TRUE : FALSE;
    }

    KIRQL
    GetSyncIrql(
        VOID
        )
    {
        return m_SynchronizeIrql;
    }

    KIRQL
    GetResourceIrql(
        VOID
        )
    {
        return m_InterruptInfo.Irql;
    }

    BOOLEAN
    SharesLock(
        FxInterrupt* Interrupt
        )
    {
        return m_SpinLock == Interrupt->m_SpinLock ? TRUE : FALSE;
    }

private:
    VOID
    Reset(
        VOID
        );

    VOID
    ResetInternal(
        VOID
        );

    VOID
    SetSyncIrql(
        KIRQL SyncIrql
        )
    {
        m_SynchronizeIrql = SyncIrql;
    }

    //
    // Called from workitem to perform final flushing of any
    // outstanding DPC's and dereferencing of objects.
    //
    VOID
    FlushAndRundown(
        VOID
        );

    VOID
    FlushAndRundownInternal(
        VOID
        );

    static
    MdInterruptServiceRoutineType _InterruptThunk;

    static
    EVT_SYSTEMWORKITEM _InterruptWorkItemCallback;

    static
    MdInterruptSynchronizeRoutineType _InterruptSynchronizeThunk;

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))

    static
    MdDeferredRoutineType _InterruptDpcThunk;

#elif ((FX_CORE_MODE)==(FX_CORE_USER_MODE))

    static
    MX_WORKITEM_ROUTINE _InterruptWorkItemThunk;

    VOID
    ThreadpoolWaitCallback(
        VOID
        );

    VOID
    QueueSingleWaitOnInterruptEvent(
        VOID
        );

    VOID
    StartThreadpoolWaitQueue(
        VOID
        );

    VOID
    StopAndFlushThreadpoolWaitQueue(
        VOID
        );

#endif

    //
    // Helper functions to enable an interrupt.
    // Sequence:
    //  (1) InterruptEnable
    //  (2) _InterruptEnableThunk
    //  (3) InterruptEnableInvokeCallback
    //
    NTSTATUS
    InterruptEnable(
        VOID
        );

    static
    MdInterruptSynchronizeRoutineType _InterruptEnableThunk;


    NTSTATUS
    InterruptEnableInvokeCallback(
        VOID
        );

    //
    // Helper functions to disable an interrupt.
    // Sequence:
    //  (1) InterruptDisable
    //  (2) _InterruptDisableThunk
    //  (3) InterruptDisableInvokeCallback
    //
    NTSTATUS
    InterruptDisable(
        VOID
        );

    static
    MdInterruptSynchronizeRoutineType _InterruptDisableThunk;


    NTSTATUS
    InterruptDisableInvokeCallback(
        VOID
        );
public:
    static
    BOOLEAN
    _IsMessageInterrupt(
        __in USHORT ResourceFlags
        )
    {
        if (ResourceFlags & CM_RESOURCE_INTERRUPT_MESSAGE) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    static
    BOOLEAN
    _IsWakeHintedInterrupt(
        __in USHORT ResourceFlags
        )
    {
        if (ResourceFlags & CM_RESOURCE_INTERRUPT_WAKE_HINT) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    _Must_inspect_result_
    NTSTATUS
    Connect(
        __in ULONG NotifyFlags
        );

    _Must_inspect_result_
    NTSTATUS
    ConnectInternal(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Disconnect(
        __in ULONG NotifyFlags
        );

    VOID
    DisconnectInternal(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    ForceDisconnect(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    ForceReconnect(
        VOID
        );

    VOID
    FilterResourceRequirements(
        __inout PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor
        );

    VOID
    AssignResources(
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescRaw,
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescTrans
        );

    PCM_PARTIAL_RESOURCE_DESCRIPTOR
    GetResources(
        VOID
        )
    {
        // Weak ref to the translated resource interrupt descriptor.
        // It is valid from prepare hardware callback to release hardware callback.
        return m_CmTranslatedResource;
    }

    VOID
    AssignResourcesInternal(
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescRaw,
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescTrans,
        __in PWDF_INTERRUPT_INFO InterruptConfig
        );

    VOID
    RevokeResources(
        VOID
        );

    VOID
    RevokeResourcesInternal(
        VOID
        );

    VOID
    SetPolicy(
        __in WDF_INTERRUPT_POLICY   Policy,
        __in WDF_INTERRUPT_PRIORITY Priority,
        __in PGROUP_AFFINITY        TargetProcessorSet
        );

    VOID
    SetPolicyInternal(
        __in WDF_INTERRUPT_POLICY   Policy,
        __in WDF_INTERRUPT_PRIORITY Priority,
        __in PGROUP_AFFINITY        TargetProcessorSet
        );

    VOID
    FlushQueuedDpcs(
        VOID
        );

    VOID
    FlushQueuedWorkitem(
        VOID
        );

    VOID
    InvokeWakeInterruptEvtIsr(
        VOID
        );

    BOOLEAN
    WakeInterruptIsr(
        VOID
        );

    BOOLEAN
    IsLevelTriggered(
        __in ULONG Flags
        )
    {
        return ((Flags & CM_RESOURCE_INTERRUPT_LEVEL_LATCHED_BITS)
            == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE);
    }

    __inline
    BOOLEAN
    QueueDeferredRoutineForIsr(
        VOID
        )
    {
    //
    // Queue DPC for KMDF and workitem for UMDF. Note that driver can either
    // specify EvtInterruptDpc or EvtInterruptWorkItem, and therefore it can
    // either call WdfInterruptQueueDpcForisr or WdfInterruptQueueWorkitemForIsr.
    //




    //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        return QueueDpcForIsr();
#else
        return QueueWorkItemForIsr();
#endif
     }

};

BOOLEAN
_SynchronizeExecution(
    __in MdInterrupt  Interrupt,
    __in MdInterruptSynchronizeRoutine  SynchronizeRoutine,
    __in PVOID  SynchronizeContext
    );

#endif // _FXINTERRUPT_H_
