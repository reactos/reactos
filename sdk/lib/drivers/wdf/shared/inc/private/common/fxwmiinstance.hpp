/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiInstance.hpp

Abstract:

    This module implements the WMI instance object

Author:



Environment:

    Both kernel and user mode

Revision History:



--*/

#ifndef _FXWMIINSTANCE_H_
#define _FXWMIINSTANCE_H_


class FxWmiInstance : public FxNonPagedObject {

    friend FxWmiProvider;
    friend FxWmiIrpHandler;

public:
    FxWmiInstance(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in FxWmiProvider* Provider
        );

    ~FxWmiInstance();

    CfxDevice*
    GetDevice(
        VOID
        )
    {
        return m_Provider->GetDevice();
    }

    FxWmiProvider*
    GetProvider(
        VOID
        )
    {
        return m_Provider;
    }

    _Must_inspect_result_
    NTSTATUS
    FireEvent(
        __in_bcount_opt(EventBufferSize) PVOID EventBuffer,
        __inout ULONG EventBufferSize
        );

    BOOLEAN
    IsEnabled(
        __in WDF_WMI_PROVIDER_CONTROL Control
        )
    {
        return m_Provider->IsEnabled(Control);
    }

    WDFWMIINSTANCE
    GetHandle(
        VOID
        )
    {
        return (WDFWMIINSTANCE) GetObjectHandle();
    }

    virtual
    BOOLEAN
    IsQueryInstanceSupported(
        VOID
        ) =0;

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    QueryInstance(
        __in ULONG OutBufferSize,
        __out_bcount_part(OutBufferSize, *BufferUsed) PVOID OutBuffer,
        __out PULONG BufferUsed
        ) =0;

    virtual
    BOOLEAN
    IsSetInstanceSupported(
        VOID
        ) =0;

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    SetInstance(
        __in ULONG InBufferSize,
        __in_bcount(InBufferSize) PVOID InBuffer
        ) =0;

    virtual
    BOOLEAN
    IsSetItemSupported(
        VOID
        ) =0;

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    SetItem(
        __in  ULONG DataItemId,
        __in  ULONG InBufferSize,
        __in_bcount(InBufferSize) PVOID InBuffer
        ) =0;

    virtual
    BOOLEAN
    IsExecuteMethodSupported(
        VOID
        ) =0;

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    ExecuteMethod(
        __in ULONG MethodId,
        __in ULONG InBufferSize,
        __inout ULONG OutBufferSize,
        __drv_when(InBufferSize >= OutBufferSize, __inout_bcount(InBufferSize))
        __drv_when(InBufferSize < OutBufferSize, __inout_bcount(OutBufferSize))
            PVOID Buffer,
        __out PULONG BufferUsed
        ) =0;

    // begin FxObject overrides
    virtual
    BOOLEAN
    Dispose(
        VOID
        );
    // end FxObject overrides

protected:
    //
    // List entry used by FxWmiProvider to hold the list of WMI instances
    //
    LIST_ENTRY m_ListEntry;

    //
    // Pointer to the parent provider
    //
    FxWmiProvider* m_Provider;
};

struct FxWmiInstanceQueryInstanceCallback : public FxCallback {

    PFN_WDF_WMI_INSTANCE_QUERY_INSTANCE m_Method;

    FxWmiInstanceQueryInstanceCallback(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    ~FxWmiInstanceQueryInstanceCallback()
    {
    }

    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in WDFWMIINSTANCE WmiInstance,
        __inout ULONG OutBufferSize,
        __out_bcount(OutBufferSize) PVOID OutBuffer,
        __out PULONG BufferUsed
        )
    {
        NTSTATUS status;

        UNREFERENCED_PARAMETER(Device);

        if (m_Method != NULL) {
            CallbackStart();
            status = m_Method(WmiInstance,
                              OutBufferSize,
                              OutBuffer,
                              BufferUsed);
            CallbackEnd();
        }
        else {
            status = STATUS_UNSUCCESSFUL;
        }

        return status;
    }
};

struct FxWmiInstanceSetInstanceCallback : public FxCallback {

    PFN_WDF_WMI_INSTANCE_SET_INSTANCE m_Method;

    FxWmiInstanceSetInstanceCallback(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    ~FxWmiInstanceSetInstanceCallback()
    {
    }

    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in WDFWMIINSTANCE WmiInstance,
        __in ULONG InBufferSize,
        __in_bcount(InBufferSize) PVOID InBuffer
        )
    {
        NTSTATUS status;

        UNREFERENCED_PARAMETER(Device);

        if (m_Method != NULL) {
            CallbackStart();
            status = m_Method(WmiInstance, InBufferSize, InBuffer);
            CallbackEnd();
        }
        else {
            status = STATUS_WMI_READ_ONLY;
        }

        return status;
    }
};

struct FxWmiInstanceSetItemCallback : public FxCallback {

    PFN_WDF_WMI_INSTANCE_SET_ITEM  m_Method;

    FxWmiInstanceSetItemCallback(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    ~FxWmiInstanceSetItemCallback()
    {
    }

    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in WDFWMIINSTANCE WmiInstance,
        __in ULONG DataItemId,
        __in ULONG InBufferSize,
        __in_bcount(InBufferSize) PVOID InBuffer
        )
    {
        NTSTATUS status;

        UNREFERENCED_PARAMETER(Device);

        if (m_Method != NULL) {
            CallbackStart();
            status = m_Method(WmiInstance,
                              DataItemId,
                              InBufferSize,
                              InBuffer);
            CallbackEnd();

        }
        else {
            status = STATUS_WMI_READ_ONLY;
        }

        return status;
    }
};

struct FxWmiInstanceExecuteMethodCallback : public FxCallback {

    PFN_WDF_WMI_INSTANCE_EXECUTE_METHOD m_Method;

    FxWmiInstanceExecuteMethodCallback(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    ~FxWmiInstanceExecuteMethodCallback()
    {
    }

    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in WDFWMIINSTANCE WmiInstance,
        __in ULONG MethodId,
        __in ULONG InBufferSize,
        __inout ULONG OutBufferSize,
        __drv_when(InBufferSize >= OutBufferSize, __inout_bcount(InBufferSize))
        __drv_when(InBufferSize < OutBufferSize, __inout_bcount(OutBufferSize))
            PVOID Buffer,
        __out PULONG BufferUsed
        )
    {
        NTSTATUS status;

        UNREFERENCED_PARAMETER(Device);

        if (m_Method != NULL) {
            CallbackStart();
            status = m_Method(WmiInstance,
                              MethodId,
                              InBufferSize,
                              OutBufferSize,
                              Buffer,
                              BufferUsed);
            CallbackEnd();
        }
        else {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }

        return status;
    }
};

class FxWmiInstanceExternal : public FxWmiInstance {
public:
    FxWmiInstanceExternal(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_WMI_INSTANCE_CONFIG Config,
        __in FxWmiProvider* Provider
        );

    VOID
    SetContextForQueryLength(
        __in ULONG ContextSize
        )
    {
        m_ContextLength = ContextSize;
    }

    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxWmiProvider* Provider,
        __in PWDF_WMI_INSTANCE_CONFIG WmiInstanceConfig,
        __in_opt PWDF_OBJECT_ATTRIBUTES InstanceAttributes,
        __out WDFWMIINSTANCE* WmiInstance,
        __out FxWmiInstanceExternal** Instance
        );

protected:
    virtual
    BOOLEAN
    IsQueryInstanceSupported(
        VOID
        );

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    QueryInstance(
        __inout ULONG OutBufferSize,
        __out_xcount(OutBuffer->size) PVOID OutBuffer,
        __out PULONG BufferUsed
        );

    virtual
    BOOLEAN
    IsSetInstanceSupported(
        VOID
        );

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    SetInstance(
        __in ULONG InBufferSize,
        __in_bcount(InBufferSize) PVOID InBuffer
        );

    virtual
    BOOLEAN
    IsSetItemSupported(
        VOID
        );

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    SetItem(
        __in ULONG DataItemId,
        __in ULONG InBufferSize,
        __in_bcount(InBufferSize) PVOID InBuffer
        );

    virtual
    BOOLEAN
    IsExecuteMethodSupported(
        VOID
        );

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    ExecuteMethod(
        __in ULONG MethodId,
        __in ULONG InBufferSize,
        __inout ULONG OutBufferSize,
        __drv_when(InBufferSize >= OutBufferSize, __inout_bcount(InBufferSize))
        __drv_when(InBufferSize < OutBufferSize, __inout_bcount(OutBufferSize))
            PVOID Buffer,
        __out PULONG BufferUsed
        );

protected:
    //
    // Attributes associated with this instance
    //
    FxWmiInstanceQueryInstanceCallback m_QueryInstanceCallback;

    FxWmiInstanceSetInstanceCallback m_SetInstanceCallback;

    FxWmiInstanceSetItemCallback m_SetItemCallback;

    FxWmiInstanceExecuteMethodCallback m_ExecuteMethodCallback;

    ULONG m_ContextLength;

    BOOLEAN m_UseContextForQuery;
};

typedef
_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
(*PFN_FX_WMI_INSTANCE_QUERY_INSTANCE)(
    __in  CfxDevice* Device,
    __in  FxWmiInstanceInternal* Instance,
    __inout ULONG OutBufferSize,
    __out_bcount(OutBufferSize) PVOID OutBuffer,
    __out PULONG BufferUsed
    );

typedef
_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
(*PFN_FX_WMI_INSTANCE_SET_INSTANCE)(
    __in CfxDevice* Device,
    __in FxWmiInstanceInternal* Instance,
    __in ULONG InBufferSize,
    __in_bcount(InBufferSize) PVOID InBuffer
    );

typedef
_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
(*PFN_FX_WMI_INSTANCE_SET_ITEM)(
    __in CfxDevice* Device,
    __in FxWmiInstanceInternal* Instance,
    __in ULONG DataItemId,
    __in ULONG InBufferSize,
    __in_bcount(InBufferSize) PVOID InBuffer
    );

typedef
_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
(*PFN_FX_WMI_INSTANCE_EXECUTE_METHOD)(
    __in CfxDevice* Device,
    __in FxWmiInstanceInternal* Instance,
    __in ULONG MethodId,
    __in ULONG InBufferSize,
    __inout ULONG OutBufferSize,
    __drv_when(InBufferSize >= OutBufferSize, __inout_bcount(InBufferSize))
    __drv_when(InBufferSize < OutBufferSize, __inout_bcount(OutBufferSize))
        PVOID Buffer,
    __out PULONG BufferUsed
    );

struct FxWmiInstanceInternalCallbacks {
    FxWmiInstanceInternalCallbacks(
        VOID
        )
    {
        RtlZeroMemory(this, sizeof(*this));
    }

    //
    // Callback when caller wants to query the entire data item's buffer.
    //
    PFN_FX_WMI_INSTANCE_QUERY_INSTANCE QueryInstance;

    //
    // Callback when caller wants to set the entire data item's buffer.
    //
    PFN_FX_WMI_INSTANCE_SET_INSTANCE SetInstance;

    //
    // Callback when caller wants to set a single field in the data item's buffer
    //
    PFN_FX_WMI_INSTANCE_SET_ITEM SetItem;

    //
    // Callback when caller wants to execute a method on the data item.
    //
    PFN_FX_WMI_INSTANCE_EXECUTE_METHOD ExecuteMethod;
};

class FxWmiInstanceInternal : public FxWmiInstance {

public:
    FxWmiInstanceInternal(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxWmiInstanceInternalCallbacks* Callbacks,
        __in FxWmiProvider* m_Provider
        );

protected:
    virtual
    BOOLEAN
    IsQueryInstanceSupported(
        VOID
        );

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    QueryInstance(
        __inout ULONG OutBufferSize,
        __out_bcount(OutBufferSize) PVOID OutBuffer,
        __out PULONG BufferUsed
        );

    virtual
    BOOLEAN
    IsSetInstanceSupported(
        VOID
        );

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    SetInstance(
        __in ULONG InBufferSize,
        __in_bcount(InBufferSize) PVOID InBuffer
        );

    virtual
    BOOLEAN
    IsSetItemSupported(
        VOID
        );

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    SetItem(
        __in ULONG DataItemId,
        __in ULONG InBufferSize,
        __in_bcount(InBufferSize) PVOID InBuffer
        );

    virtual
    BOOLEAN
    IsExecuteMethodSupported(
        VOID
        );

    virtual
    _Must_inspect_result_
    __drv_sameIRQL
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    ExecuteMethod(
        __in ULONG MethodId,
        __in ULONG InBufferSize,
        __inout ULONG OutBufferSize,
        __drv_when(InBufferSize >= OutBufferSize, __inout_bcount(InBufferSize))
        __drv_when(InBufferSize < OutBufferSize, __inout_bcount(OutBufferSize))
            PVOID Buffer,
        __out PULONG BufferUsed
        );

protected:
    PFN_FX_WMI_INSTANCE_QUERY_INSTANCE m_QueryInstance;
    PFN_FX_WMI_INSTANCE_SET_INSTANCE m_SetInstance;
    PFN_FX_WMI_INSTANCE_SET_ITEM m_SetItem;
    PFN_FX_WMI_INSTANCE_EXECUTE_METHOD m_ExecuteMethod;
};

#endif // _FXWMIINSTANCE_H_
