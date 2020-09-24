/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiProvider.cpp

Abstract:

    This module implements the FxWmiProvider object

Author:



Revision History:


--*/

#include "fxwmipch.hpp"

extern "C" {
#include "FxWmiProvider.tmh"
}

FxWmiProvider::FxWmiProvider(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_WMI_PROVIDER_CONFIG Config,
    __in FxDevice* Device
    ) :
    FxNonPagedObject(FX_TYPE_WMI_PROVIDER,
                     sizeof(FxWmiProvider),
                     FxDriverGlobals),
    m_FunctionControl(FxDriverGlobals)
{
    InitializeListHead(&m_ListEntry);
    InitializeListHead(&m_InstanceListHead);
    m_NumInstances = 0;

    m_Parent = Device->m_PkgWmi;

    m_EventControlEnabled = FALSE;
    m_DataBlockControlEnabled = FALSE;
    m_RemoveGuid = FALSE;

    m_TracingHandle = 0;

    m_Flags = Config->Flags;
    m_MinInstanceBufferSize = Config->MinInstanceBufferSize;
    RtlCopyMemory(&m_Guid, &Config->Guid, sizeof(GUID));

    if (Config->EvtWmiProviderFunctionControl != NULL) {
        m_FunctionControl.m_Method = Config->EvtWmiProviderFunctionControl;
    }

    //
    // Driver cannot call WdfObjectDelete on this handle
    //
    MarkNoDeleteDDI();

    MarkDisposeOverride(ObjectDoNotLock);
}

FxWmiProvider::~FxWmiProvider()
{
    ASSERT(IsListEmpty(&m_ListEntry));
}

BOOLEAN
FxWmiProvider::Dispose(
    VOID
    )
{
    //
    // Object is being deleted, remove this object from the irp handler's list
    // of providers.  If we don't do this, the irp handler will have a list
    // which contains entries which have been freed.
    //
    m_Parent->RemoveProvider(this);

    return __super::Dispose();
}

_Must_inspect_result_
NTSTATUS
FxWmiProvider::_Create(
    __in PFX_DRIVER_GLOBALS CallersGlobals,
    __in WDFDEVICE Device,
    __in_opt PWDF_OBJECT_ATTRIBUTES ProviderAttributes,
    __in PWDF_WMI_PROVIDER_CONFIG WmiProviderConfig,
    __out WDFWMIPROVIDER* WmiProvider,
    __out FxWmiProvider** Provider
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;
    FxWmiProvider* pProvider;
    NTSTATUS status;
    WDFOBJECT hProvider;
    GUID zeroGuid;
    BOOLEAN update;

    FxObjectHandleGetPtrAndGlobals(CallersGlobals,
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID*) &pDevice,
                                   &pFxDriverGlobals);

    *Provider = NULL;
    update = FALSE;

    *WmiProvider = NULL;

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        ProviderAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (WmiProviderConfig->Size != sizeof(WDF_WMI_PROVIDER_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WmiProviderConfig Size 0x%x, expected size 0x%x, %!STATUS!",
            WmiProviderConfig->Size, sizeof(WDF_WMI_PROVIDER_CONFIG),
            status);

        return status;
    }

    if ((WmiProviderConfig->Flags & ~WdfWmiProviderValidFlags) != 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Invalid flag(s) set, Flags 0x%x, valid mask 0x%x, %!STATUS!",
            WmiProviderConfig->Flags, WdfWmiProviderValidFlags,
            status);
        return status;
    }

    if ((WmiProviderConfig->Flags & WdfWmiProviderTracing) &&
        (WmiProviderConfig->Flags & ~WdfWmiProviderTracing)) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WdfWmiProviderTracing must be the only flag set, %!STATUS!",
            status);

        return status;
    }

    //
    // Function control makes sense if it is expensive.  Otherwise you will
    // not be called back on it.
    //
    // The opposite, marking yourself as expensive but providing no callback is
    // OK b/c the provider marks the enabled state and the driver can retrieve
    // it at runtime.
    //
    // Function control also applies to tracing GUIDs since the tracing subsystem
    // will call enable/disable events to start/stop tracing.
    //
    if (WmiProviderConfig->EvtWmiProviderFunctionControl != NULL &&
        ((WmiProviderConfig->Flags & (WdfWmiProviderTracing | WdfWmiProviderExpensive)) == 0)) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtWmiProviderFunctionControl can only be set if Flags 0x%x has "
            "WdfWmiProviderTracing (%d) or WdfWmiProviderExpensive (%d) bit "
            "values set, %!STATUS!",
            WmiProviderConfig->Flags, WdfWmiProviderTracing,
            WdfWmiProviderExpensive, status);

        return status;
    }

    RtlZeroMemory(&zeroGuid, sizeof(zeroGuid));

    if (RtlCompareMemory(&WmiProviderConfig->Guid,
                         &zeroGuid,
                         sizeof(GUID)) == sizeof(GUID)) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WmiProvider Guid filed is all zeros, %!STATUS!",
            status);

        return status;
    }

    pProvider = NULL;

    pProvider = new(pFxDriverGlobals, ProviderAttributes)
        FxWmiProvider(pFxDriverGlobals, WmiProviderConfig, pDevice);

    if (pProvider == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not allocate memory for a WDFWMIPROVIDER, %!STATUS!",
            status);

        return status;
    }

    status = pDevice->m_PkgWmi->AddProvider(pProvider, &update);

    if (NT_SUCCESS(status)) {
        status = pProvider->Commit(ProviderAttributes, &hProvider, pDevice);

        if (!NT_SUCCESS(status)) {
            pDevice->m_PkgWmi->RemoveProvider(pProvider);
        }
        else {
            //
            // NT_SUCCES(status) case
            //
            *WmiProvider = (WDFWMIPROVIDER) hProvider;
        }
    }

    if (NT_SUCCESS(status)) {
        *Provider = pProvider;

        if (update) {
            pDevice->m_PkgWmi->UpdateGuids();
        }
    }
    else {
        //
        // AddProvider incremented update count on success however since we
        // are not going to update registration due to this failure, decrement
        // the count.
        //
        if (update) {
            pDevice->m_PkgWmi->DecrementUpdateCount();
        }

        pProvider->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiProvider::AddInstanceLocked(
    __in  FxWmiInstance* Instance,
    __in  BOOLEAN NoErrorIfPresent,
    __out PBOOLEAN Update,
    __in  AddInstanceAction Action
    )
{
    NTSTATUS status;

    *Update = FALSE;

    if (!IsListEmpty(&Instance->m_ListEntry)) {
        if (NoErrorIfPresent) {
            return STATUS_SUCCESS;
        }
        else {
            //
            // Entry is already on a list, bad caller!
            //
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFWMIINSTANCE %p already added, %!STATUS!",
                Instance->GetHandle(), status);

            return status;
        }
    }

    //
    // Check to see if we are in the process of
    //
    switch (m_Parent->m_RegisteredState) {
    case FxWmiIrpHandler::WmiUnregistered:
        //
        // The GUID will be reported when we do the initial registration
        //
        break;

    case FxWmiIrpHandler::WmiDeregistered:
        //
        // Either the GUID will be reported when we do the re-registration or
        // we will clean it up when we goto the cleanup state.
        //
        break;

    case FxWmiIrpHandler::WmiRegistered:
        //
        // Since we already registered we need to tell WMI the change in the
        // number of instances on this provider.
        //
        *Update = TRUE;
        break;

    case FxWmiIrpHandler::WmiCleanedUp:
        //
        // Device is going away, registration is not allowed for the device
        // anymore.
        //
        status = STATUS_INVALID_DEVICE_STATE;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGDEVICE,
            "WMI is being cleanedup, WDFWMIINSTANCE %p add failing, %!STATUS!",
            Instance->GetHandle(), status);

        return status;

    default:
        ASSERT(FALSE);
        break;
    }

    if (Action == AddInstanceToTail) {
        InsertTailList(&m_InstanceListHead, &Instance->m_ListEntry);
    }
    else {
        InsertHeadList(&m_InstanceListHead, &Instance->m_ListEntry);
    }

    //
    // Since the count is increasing to at least one, we are not going to
    // need to report this GUID as missing.  We could check the
    // m_Parent->m_RegisteredState and only set it when we are registered, but
    // it does us no harm to always clear this value regardless of state.
    //
    m_RemoveGuid = FALSE;

    m_NumInstances++;
    status = STATUS_SUCCESS;

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiProvider::AddInstance(
     __in FxWmiInstance* Instance,
     __in BOOLEAN NoErrorIfPresent
     )
{
    NTSTATUS status;
    KIRQL irql;
    BOOLEAN update;

    if (m_Flags & WdfWmiProviderTracing) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFWMIINSTANCE %p cannot be added to tracing WDFWMIPROVIDER %p, "
            "%!STATUS!", Instance->GetHandle(), GetHandle(), status);

        return status;
    }

    m_Parent->Lock(&irql);
    status = AddInstanceLocked(Instance, NoErrorIfPresent, &update);

    if (update) {
        update = m_Parent->DeferUpdateLocked(irql);
    }
    m_Parent->Unlock(irql);

    if (update) {
        m_Parent->UpdateGuids();
    }

    return status;
}

VOID
FxWmiProvider::RemoveInstance(
    __in FxWmiInstance* Instance
    )
{
    KIRQL irql;
    BOOLEAN update;

    update = FALSE;

    m_Parent->Lock(&irql);

    if (!IsListEmpty(&Instance->m_ListEntry)) {
        //
        // Instance is in the list of instances on this provider.  Remove it.
        //
        RemoveEntryList(&Instance->m_ListEntry);
        InitializeListHead(&Instance->m_ListEntry);
        m_NumInstances--;

        if (m_Parent->m_RegisteredState == FxWmiIrpHandler::WmiRegistered) {
            update = TRUE;

            //
            // When the count goes to zero, inform WMI that the GUID should be
            // removed when we get requeried.  We only need to do this once we have
            // been registered.  In all other states, we ignore this value.
            //
            if (m_NumInstances == 0 &&
                (m_Flags & WdfWmiProviderExpensive) == 0) {
                m_RemoveGuid = TRUE;
            }
        }
    }
    else {
        //
        // The instance was explicitly removed and now the instance is trying
        // to remove itself during dispose.  Nothing to do.
        //
        DO_NOTHING();
    }

    if (update) {
        update = m_Parent->DeferUpdateLocked(irql);
    }

    m_Parent->Unlock(irql);

    if (update) {
        m_Parent->UpdateGuids();
    }
}

ULONG
FxWmiProvider::GetInstanceIndex(
    __in FxWmiInstance* Instance
    )
{
    PLIST_ENTRY ple;
    ULONG index;
    KIRQL irql;

    m_Parent->Lock(&irql);
    for (index = 0, ple = m_InstanceListHead.Flink;
         index < m_NumInstances;
         index++, ple = ple->Flink) {
        if (CONTAINING_RECORD(ple, FxWmiInstance, m_ListEntry) == Instance) {
            break;
        }
    }
    m_Parent->Unlock(irql);

    return index;
}

_Must_inspect_result_
FxWmiInstance*
FxWmiProvider::GetInstanceReferenced(
    __in ULONG Index,
    __in PVOID Tag
    )
{
    FxWmiInstance* pInstance;
    KIRQL irql;

    m_Parent->Lock(&irql);
    pInstance = GetInstanceReferencedLocked(Index, Tag);
    m_Parent->Unlock(irql);

    return pInstance;
}

_Must_inspect_result_
FxWmiInstance*
FxWmiProvider::GetInstanceReferencedLocked(
    __in ULONG Index,
    __in PVOID Tag
    )
{
    FxWmiInstance* pFound;
    PLIST_ENTRY ple;
    ULONG i;

    pFound = NULL;

    for (i = 0, ple = m_InstanceListHead.Flink;
         i < m_NumInstances;
         ple = ple->Flink, i++) {

        if (i == Index) {
            pFound = CONTAINING_RECORD(ple, FxWmiInstance, m_ListEntry);
            pFound->ADDREF(Tag);
            break;
        }
    }

    return pFound;
}

_Must_inspect_result_
NTSTATUS
FxWmiProvider::FunctionControl(
    __in WDF_WMI_PROVIDER_CONTROL Control,
    __in BOOLEAN Enable
    )
{
    return m_FunctionControl.Invoke(m_Parent->GetDevice()->GetHandle(),
                                    GetHandle(),
                                    Control,
                                    Enable);
}

ULONG
FxWmiProvider::GetRegistrationFlagsLocked(
    VOID
    )
{
    ULONG flags;

    if (m_Flags & WdfWmiProviderTracing) {
        flags = WMIREG_FLAG_TRACED_GUID | WMIREG_FLAG_TRACE_CONTROL_GUID;

        //
        // Once tracing GUID is registered, we do not allow it to be unregistered
        //
        ASSERT(m_RemoveGuid == FALSE);
    }
    else {
        flags = WMIREG_FLAG_INSTANCE_PDO;

        if (m_Flags & WdfWmiProviderExpensive) {
            flags |= WMIREG_FLAG_EXPENSIVE;
        }

        if (m_Flags & WdfWmiProviderEventOnly) {
            flags |= WMIREG_FLAG_EVENT_ONLY_GUID;
        }
    }

    if (m_RemoveGuid) {
        //
        // We have gone down to zero instances of this provider, report it as
        // gone to WMI.
        //
        ASSERT(m_NumInstances == 0);
        flags |= WMIREG_FLAG_REMOVE_GUID;

        //
        // Once reported as removed, we do not have not have to report ourselves
        // as removed again.
        //
        m_RemoveGuid = FALSE;
    }

    return flags;
}

