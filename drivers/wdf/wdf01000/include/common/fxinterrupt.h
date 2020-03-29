#ifndef _FXINTERRUPT_H_
#define _FXINTERRUPT_H_

#include "common/fxnonpagedobject.h"
#include "common/fxwakeinterruptstatemachine.h"
#include "common/fxwaitlock.h"
#include "common/fxsystemworkitem.h"


class FxPkgPnp;
class FxInterrupt;
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
    // Interrupt policy
    //
    BOOLEAN                         m_SetPolicy;
    WDF_INTERRUPT_POLICY            m_Policy;
    WDF_INTERRUPT_PRIORITY          m_Priority;
    GROUP_AFFINITY                  m_Processors;

    
    //
    // PnP data about the interrupt.
    //
    WDF_INTERRUPT_INFO              m_InterruptInfo;

    //
    // Set if this is an Edge-Triggered non-MSI interrupt. These interrupts are
    // stateful and it is important not to drop any around the connection window.
    //
    BOOLEAN m_IsEdgeTriggeredNonMsiInterrupt;

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
    // Callbacks
    //
    PFN_WDF_INTERRUPT_ENABLE        m_EvtInterruptEnable;
    PFN_WDF_INTERRUPT_DISABLE       m_EvtInterruptDisable;

    PFN_WDF_INTERRUPT_ISR           m_EvtInterruptIsr;
    PFN_WDF_INTERRUPT_DPC           m_EvtInterruptDpc;
    //PFN_WDF_INTERRUPT_WORKITEM      m_EvtInterruptWorkItem; //WDF 1.11

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    //
    // Callback used to set m_Disconnecting, synchronized to running ISRs.
    // Only runs if m_IsEdgeTriggeredNonMsiInterrupt is TRUE.
    //
    //static
    //MdInterruptSynchronizeRoutineType _InterruptMarkDisconnecting;

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
    // Weak ref to the translated resource interrupt descriptor.
    // It is valid from prepare hardware callback to release hardware callback.
    // 
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  m_CmTranslatedResource;

public:
    FxInterrupt(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    ~FxInterrupt(
        VOID
        );

    VOID
    FilterResourceRequirements(
        __inout PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor
        );

    static
    BOOLEAN
    _IsMessageInterrupt(
        __in USHORT ResourceFlags
        )
    {
        if (ResourceFlags & CM_RESOURCE_INTERRUPT_MESSAGE)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    VOID
    AssignResources(
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescRaw,
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescTrans
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
            (m_WakeInterruptMachine->m_ActiveForWake))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))

    //
    // NOTE: Start from Windows 8
    //
    //VOID
    //ReportActive(
    //    _In_ BOOLEAN Internal = FALSE
    //    );

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
            //FxLibraryGlobals.IoReportInterruptInactive != NULL &&
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

    __inline
    BOOLEAN
    IsWakeCapable(
        VOID
        )
    {
        return ((m_WakeInterruptMachine != NULL) ? TRUE : FALSE);
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

    __inline
    struct _KINTERRUPT*
    GetInterruptPtr(
        VOID
        )
    {
        struct _KINTERRUPT* interrupt = m_Interrupt;

        if (interrupt == NULL)
        {
            interrupt = m_InterruptCaptured;
        }

        return interrupt;
    }

    VOID
    DpcHandler(
        __in_opt PVOID SystemArgument1,
        __in_opt PVOID SystemArgument2
        );

    VOID
    FlushQueuedDpcs(
        VOID
        );

protected:

    LIST_ENTRY  m_PnpList;

private:
    VOID
    Reset(
        VOID
        );

    VOID
    ResetInternal(
        VOID
        )
    {

    }

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

    static
    MdInterruptServiceRoutineType _InterruptThunk;

    static
    EVT_SYSTEMWORKITEM _InterruptWorkItemCallback;

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    static
    MdDeferredRoutineType _InterruptDpcThunk;

#elif ((FX_CORE_MODE)==(FX_CORE_USER_MODE))

#endif
        
};

__inline
BOOLEAN 
_SynchronizeExecution(
    __in MdInterrupt  Interrupt,
    __in MdInterruptSynchronizeRoutine  SynchronizeRoutine,
    __in PVOID  SynchronizeContext
    )
{
    return KeSynchronizeExecution(Interrupt,
                                  SynchronizeRoutine,
                                  SynchronizeContext);
}

#endif // _FXINTERRUPT_H_