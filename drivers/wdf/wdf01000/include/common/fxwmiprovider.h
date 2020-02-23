#ifndef _FXWMIPROVIDER_H_
#define _FXWMIPROVIDER_H_

#include "common/fxnonpagedobject.h"
#include "common/fxwmiirphandler.h"
#include "common/fxcallback.h"


class FxWmiInstance;
class FxWmiIrpHandler;

struct FxWmiProviderFunctionControlCallback : public FxCallback {

    PFN_WDF_WMI_PROVIDER_FUNCTION_CONTROL m_Method;

    FxWmiProviderFunctionControlCallback(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    ~FxWmiProviderFunctionControlCallback()
    {
    }

};

class FxWmiProvider : public FxNonPagedObject {

    friend FxWmiIrpHandler;

public:

    FxWmiProvider(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_WMI_PROVIDER_CONFIG Config,
        __in CfxDevice* Device
        );

    ~FxWmiProvider();

    _Must_inspect_result_
    FxWmiInstance*
    GetInstanceReferencedLocked(
        __in ULONG Index,
        __in PVOID Tag
        );

    CfxDevice*
    GetDevice(
        VOID
        )
    {
        return m_Parent->GetDevice();
    }

    GUID*
    GetGUID(
        VOID
        )
    {
        return &m_Guid;
    }

    ULONG
    GetFlags(
        VOID
        )
    {
        return m_Flags;
    }

    ULONG
    GetMinInstanceBufferSize(
        VOID
        )
    {
        return m_MinInstanceBufferSize;
    }

    BOOLEAN
    IsEventOnly(
        VOID
        )
    {
        return FLAG_TO_BOOL(m_Flags, WdfWmiProviderEventOnly);
    }

    BOOLEAN
    IsEnabled(
        __in WDF_WMI_PROVIDER_CONTROL Control
        )
    {
        switch (Control) {
        case WdfWmiEventControl:    return m_EventControlEnabled;
        case WdfWmiInstanceControl: return m_DataBlockControlEnabled;
        default:                        ASSERT(FALSE); return FALSE;
        }
    }

    ULONGLONG
    GetTracingHandle(
        VOID
        )
    {
        return m_TracingHandle;
    }

    VOID
    SetTracingHandle(
        __in ULONGLONG TracingHandle
        )
    {
        m_TracingHandle = TracingHandle;
    }

    WDFWMIPROVIDER
    GetHandle(
        VOID
        )
    {
        return (WDFWMIPROVIDER) GetObjectHandle();
    }

    BOOLEAN
    IsFunctionControlSupported(
        VOID
        )
    {
        return m_FunctionControl.m_Method != NULL;
    }

protected:
    //
    // List entry used by FxWmiIrpHandler to hold the list of WMI providers
    //
    LIST_ENTRY m_ListEntry;

    LIST_ENTRY m_InstanceListHead;

    ULONG m_NumInstances;

    FxWmiIrpHandler* m_Parent;

    GUID m_Guid;

    ULONGLONG m_TracingHandle;

    ULONG m_MinInstanceBufferSize;

    union {
        //
        // Set with values from WDF_WMI_PROVIDER_FLAGS
        //
        ULONG m_Flags;

        //
        // Not used by the code, but by the debug extension
        //
        struct {
            ULONG EventOnly : 1;
            ULONG Expensive : 1;
            ULONG Tracing : 1;
        } m_FlagsByName;
    };

    FxWmiProviderFunctionControlCallback m_FunctionControl;

    BOOLEAN m_EventControlEnabled;

    BOOLEAN m_DataBlockControlEnabled;

    BOOLEAN m_RemoveGuid;

};

#endif //_FXWMIPROVIDER_H_
