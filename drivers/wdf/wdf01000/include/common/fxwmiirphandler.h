#ifndef _FXWMIIRPHANDLER_H_
#define _FXWMIIRPHANDLER_H_

#include "common/fxpackage.h"
//#include "common/fxwmiprovider.h"
//#include "common/fxwmiinstance.h"

class FxWmiInstance;
class FxWmiProvider;
class FxWmiIrpHandler;
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

protected:

    _Must_inspect_result_
    FxWmiProvider*
    FindProviderLocked(
        __in LPGUID Guid
        );

protected:
    enum WmiRegisteredState {
        WmiUnregistered = 0,
        WmiRegistered,
        WmiDeregistered,
        WmiCleanedUp
    };

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
    //FxCREvent m_UpdateEvent;

    PKEVENT m_WorkItemEvent;

    BOOLEAN m_WorkItemQueued;

};

#endif //_FXWMIIRPHANDLER_H_
