/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiIrpHandler.hpp

Abstract:

    This module implements the wmi IRP handler for the driver frameworks.

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXWMIIRPHANDLER_H_
#define _FXWMIIRPHANDLER_H_

typedef
_Must_inspect_result_
NTSTATUS
(*PFN_WMI_HANDLER_MINOR_DISPATCH)(
    __in FxWmiIrpHandler* This,
    __in PIRP Irp,
    __in FxWmiProvider* Provider,
    __in FxWmiInstance* Instance
    );

struct FxWmiMinorEntry {
    __in PFN_WMI_HANDLER_MINOR_DISPATCH Handler;
    __in BOOLEAN CheckInstance;
};

class FxWmiIrpHandler : public FxPackage {
    friend FxWmiProvider;

public:

    FxWmiIrpHandler(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice *Device,
        __in WDFTYPE Type = FX_TYPE_WMI_IRP_HANDLER
        );

    ~FxWmiIrpHandler();

    _Must_inspect_result_
    NTSTATUS
    PostCreateDeviceInitialize(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    HandleWmiTraceRequest(
        __in PIRP Irp,
        __in FxTraceInfo* Info
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    Dispatch(
        __in PIRP Irp
        );

    _Must_inspect_result_
    NTSTATUS
    Register(
        VOID
        );

    VOID
    Deregister(
        VOID
        );

    VOID
    Cleanup(
        VOID
        );

    VOID
    ResetStateForPdoRestart(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    AddProvider(
        __in FxWmiProvider* Provider,
        __out_opt PBOOLEAN Update = NULL
        );

    _Must_inspect_result_
    NTSTATUS
    AddPowerPolicyProviderAndInstance(
        __in    PWDF_WMI_PROVIDER_CONFIG ProviderConfig,
        __in    FxWmiInstanceInternalCallbacks* Callbacks,
        __inout FxWmiInstanceInternal** Instance
        );

protected:
    static
    VOID
    CheckAssumptions(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    AddProviderLocked(
        __in FxWmiProvider* Provider,
        __in KIRQL Irql,
        __out_opt PBOOLEAN Update = NULL
        );

    VOID
    RemoveProvider(
        __in FxWmiProvider* Provider
        );

    VOID
    RemoveProviderLocked(
        __in FxWmiProvider* Provider
        );

    _Must_inspect_result_
    FxWmiProvider*
    FindProviderLocked(
        __in LPGUID Guid
        );

    _Must_inspect_result_
    FxWmiProvider*
    FindProviderReferenced(
        __in LPGUID Guid,
        __in PVOID Tag
        );

private:
    static
    _Must_inspect_result_
    NTSTATUS
    _QueryAllData(
        __in FxWmiIrpHandler* This,
        __in PIRP Irp,
        __in FxWmiProvider* Provider,
        __in_opt FxWmiInstance* Instance
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _QuerySingleInstance(
        __in FxWmiIrpHandler* This,
        __in PIRP Irp,
        __in FxWmiProvider* Provider,
        __in FxWmiInstance* Instance
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _ChangeSingleInstance(
        __in FxWmiIrpHandler* This,
        __in PIRP Irp,
        __in FxWmiProvider* Provider,
        __in FxWmiInstance* Instance
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _ChangeSingleItem(
        __in FxWmiIrpHandler* This,
        __in PIRP Irp,
        __in FxWmiProvider* Provider,
        __in FxWmiInstance* Instance
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _EnableDisableEventsAndCollection(
        __in FxWmiIrpHandler* This,
        __in PIRP Irp,
        __in FxWmiProvider* Provider,
        __in FxWmiInstance* Instance
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _RegInfo(
        __in FxWmiIrpHandler* This,
        __in PIRP Irp,
        __in_opt FxWmiProvider* Provider,
        __in_opt FxWmiInstance* Instance
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _ExecuteMethod(
        __in FxWmiIrpHandler* This,
        __in PIRP Irp,
        __in FxWmiProvider* Provider,
        __in FxWmiInstance* Instance
        );

    VOID
    CompleteWmiQueryAllDataRequest(
        __in PIRP Irp,
        __in NTSTATUS Status,
        __in ULONG BufferUsed
        );

    VOID
    CompleteWmiQuerySingleInstanceRequest(
        __in PIRP Irp,
        __in NTSTATUS Status,
        __in ULONG BufferUsed
        );

    VOID
    CompleteWmiExecuteMethodRequest(
        __in PIRP Irp,
        __in NTSTATUS Status,
        __in ULONG BufferUsed
        );

    _Must_inspect_result_
    NTSTATUS
    CompleteWmiRequest(
        __in PIRP Irp,
        __in NTSTATUS Status,
        __in ULONG BufferUsed
        );

    BOOLEAN
    DeferUpdateLocked(
        __in KIRQL OldIrql
        );

    static
    MX_WORKITEM_ROUTINE _UpdateGuids;

    VOID
    UpdateGuids(
        VOID
        );

    VOID
    IncrementUpdateCount() {
        LONG count;

        count = InterlockedIncrement(&m_UpdateCount);
        ASSERT(count > 1);
        UNREFERENCED_PARAMETER(count);
    }

    VOID
    DecrementUpdateCount() {
        LONG count;

        count = InterlockedDecrement(&m_UpdateCount);
        ASSERT(count >= 0);
        if (count == 0) {
            m_UpdateEvent.Set();
        }
    }

    VOID
    DecrementUpdateCountAndWait() {

        DecrementUpdateCount();

        m_UpdateEvent.EnterCRAndWaitAndLeave();
    }

protected:
    enum WmiRegisteredState {
        WmiUnregistered = 0,
        WmiRegistered,
        WmiDeregistered,
        WmiCleanedUp
    };

protected:
    static const FxWmiMinorEntry m_WmiDispatchTable[];

    LIST_ENTRY m_ProvidersListHead;

    ULONG m_NumProviders;

    WmiRegisteredState m_RegisteredState;

    PIO_WORKITEM m_WorkItem;

    //
    // count of references taken every time an update is needed
    //
    LONG m_UpdateCount;

    //
    // WMI unregister waits on this event to ensure no upadtes are allowed
    // after unregister.
    //
    FxCREvent m_UpdateEvent;

    PKEVENT m_WorkItemEvent;

    BOOLEAN m_WorkItemQueued;


};

#endif // _FXWMIIRPHANDLER_H_
