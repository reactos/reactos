/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiDataBlock.hpp

Abstract:

    This module implements the WMI data block object

Author:




Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXWMIPROVIDER_H_
#define _FXWMIPROVIDER_H_


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

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFDEVICE Device,
        __in WDFWMIPROVIDER WmiProvider,
        __in WDF_WMI_PROVIDER_CONTROL Control,
        __in BOOLEAN Enable
        )
    {
        NTSTATUS status;

        UNREFERENCED_PARAMETER(Device);

        if (m_Method != NULL) {
            CallbackStart();
            status = m_Method(WmiProvider, Control, Enable);
            CallbackEnd();
        }
        else {
            status = STATUS_SUCCESS;
        }

        return status;
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

    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS CallersGlobals,
        __in WDFDEVICE Device,
        __in_opt PWDF_OBJECT_ATTRIBUTES ProviderAttributes,
        __in PWDF_WMI_PROVIDER_CONFIG WmiProviderConfig,
        __out WDFWMIPROVIDER* WmiProvider,
        __out FxWmiProvider** Provider
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

    ULONG
    GetRegistrationFlagsLocked(
        VOID
        );

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

    _Must_inspect_result_
    NTSTATUS
    FunctionControl(
        __in WDF_WMI_PROVIDER_CONTROL Control,
        __in BOOLEAN Enable
        );

    _Must_inspect_result_
    NTSTATUS
    AddInstance(
         __in FxWmiInstance* Instance,
         __in BOOLEAN NoErrorIfPresent = FALSE
         );

    VOID
    RemoveInstance(
        __in FxWmiInstance* Instance
        );

    ULONG
    GetInstanceIndex(
        __in FxWmiInstance* Instance
        );

     _Must_inspect_result_
    FxWmiInstance*
    GetInstanceReferenced(
        __in ULONG Index,
        __in PVOID Tag
        );

    _Must_inspect_result_
    FxWmiInstance*
    GetInstanceReferencedLocked(
        __in ULONG Index,
        __in PVOID Tag
        );

    // begin FxObject overrides
    virtual
    BOOLEAN
    Dispose(
        VOID
        );
    // end FxObject overrides

protected:
    enum AddInstanceAction {
        AddInstanceToTail,
        AddInstanceToHead
    };

    _Must_inspect_result_
    NTSTATUS
    AddInstanceLocked(
        __in  FxWmiInstance* Instance,
        __in  BOOLEAN NoErrorIfPresent,
        __out PBOOLEAN Update,
        __in  AddInstanceAction Action = AddInstanceToTail
        );

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

#endif // _FXWMIPROVIDER_H_
