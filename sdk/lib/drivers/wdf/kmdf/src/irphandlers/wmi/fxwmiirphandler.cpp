/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiIrpHandler.cpp

Abstract:

    This module implements the wmi irp handler for the driver frameworks.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "fxwmipch.hpp"

extern "C" {
// #include "FxWmiIrpHandler.tmh"
}

#ifndef WppDebug
#define WppDebug(a, b)
#endif

const FxWmiMinorEntry FxWmiIrpHandler::m_WmiDispatchTable[] =
{
    // IRP_MN_QUERY_ALL_DATA
    { _QueryAllData, FALSE },

    // IRP_MN_QUERY_SINGLE_INSTANCE
    { _QuerySingleInstance, TRUE },

    // IRP_MN_CHANGE_SINGLE_INSTANCE
    { _ChangeSingleInstance, TRUE },

    // IRP_MN_CHANGE_SINGLE_ITEM
    { _ChangeSingleItem, TRUE },

    // IRP_MN_ENABLE_EVENTS
    { _EnableDisableEventsAndCollection, FALSE },

    // IRP_MN_DISABLE_EVENTS
    { _EnableDisableEventsAndCollection, FALSE },

    // IRP_MN_ENABLE_COLLECTION
    { _EnableDisableEventsAndCollection, FALSE },

    // IRP_MN_DISABLE_COLLECTION
    { _EnableDisableEventsAndCollection, FALSE },

    // IRP_MN_REGINFO
    { _RegInfo, FALSE },

    // IRP_MN_EXECUTE_METHOD
    { _ExecuteMethod, TRUE },

    // 0xA is reseverved
    { NULL, FALSE },

    // IRP_MN_REGINFO_EX
    { _RegInfo, FALSE },
};

VOID
FxWmiIrpHandler::CheckAssumptions(
    VOID
    )
{
    WDFCASSERT(sizeof(m_WmiDispatchTable)/sizeof(m_WmiDispatchTable[0]) ==
               IRP_MN_REGINFO_EX + 1);
}

FxWmiIrpHandler::FxWmiIrpHandler(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxDevice *Device,
    __in WDFTYPE Type
    ) :
    FxPackage(FxDriverGlobals, Device, Type),
    m_NumProviders(0), m_RegisteredState(WmiUnregistered),
    m_WorkItem(NULL), m_WorkItemEvent(NULL), m_WorkItemQueued(FALSE),
    m_UpdateCount(1) // bias m_UpdateCount to 1, Deregister routine will
                     // decrement this.
{
    InitializeListHead(&m_ProvidersListHead);
}

FxWmiIrpHandler::~FxWmiIrpHandler()
{
    //
    // If the device could not get past AddDevice or failed the initial start
    // device, we will be unregistered.  Otherwise we should be cleaned up.
    //
    ASSERT(m_RegisteredState != WmiRegistered);

    ASSERT(IsListEmpty(&m_ProvidersListHead));

    if (m_WorkItem != NULL) {
        IoFreeWorkItem(m_WorkItem);
    }
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::PostCreateDeviceInitialize(
    VOID
    )
{
    m_WorkItem = IoAllocateWorkItem(GetDevice()->GetDeviceObject());
    if (m_WorkItem == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::Register(
    VOID
    )
{
    NTSTATUS status;
    KIRQL irql;

    //
    // We rely on the PnP state machine to manage our state transitions properly
    // so that we don't have to do any state checking here.
    //
    Lock(&irql);
    ASSERT(m_RegisteredState == WmiUnregistered ||
              m_RegisteredState == WmiDeregistered);
    m_RegisteredState = WmiRegistered;
    Unlock(irql);

    status = IoWMIRegistrationControl(GetDevice()->GetDeviceObject(),
                                      WMIREG_ACTION_REGISTER);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "could not register WMI with OS, %!STATUS!", status);

        Lock(&irql);
        m_RegisteredState = WmiUnregistered;
        Unlock(irql);
    }

    return status;
}

VOID
FxWmiIrpHandler::Deregister(
    VOID
    )
{
    FxCREvent event;
    KIRQL irql;
    BOOLEAN call;

    call = FALSE;

    Lock(&irql);
    if (m_RegisteredState == WmiRegistered) {
        m_RegisteredState = WmiDeregistered;

        if (m_WorkItemQueued) {





            m_WorkItemEvent = (PKEVENT)event.GetEvent();
        }

        call = TRUE;
    }
    Unlock(irql);

    if (m_WorkItemEvent != NULL) {
        event.EnterCRAndWaitAndLeave();
    }

    if (call) {
        NTSTATUS status;

        //
        // Per WMI rules, there should not be any call to update WMI
        // registration after we have deregistered because that can lead to
        // deadlock in Pnp, so before we go ahead to deregister, let's ensure
        // that. This will wait for any pending updates to happen.
        //
        DecrementUpdateCountAndWait();

        status = IoWMIRegistrationControl(GetDevice()->GetDeviceObject(),
                                          WMIREG_ACTION_DEREGISTER);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "failure deregistering WMI with OS, %!STATUS!", status);
        }
    }
}

VOID
FxWmiIrpHandler::Cleanup(
    VOID
    )
{
    KIRQL irql;

    Lock(&irql);
    m_RegisteredState = WmiCleanedUp;
    Unlock(irql);
}

VOID
FxWmiIrpHandler::ResetStateForPdoRestart(
    VOID
    )
{
    KIRQL irql;

    Lock(&irql);

    //
    // We can reach this state in 2 ways:
    // 1. PDO went through Init->Started->Removed->Started transition
    // 2. PDO went through Init-><SomeFailedState>->Started state (e.g.
    // Init->Ejected->EjectFailed->Started) transition.
    //
    // So, WMI registration state would either be Deregistered due to #1
    // or, Unregistered due to #2. Also, update count would be 0 in case of #1
    // and 1 in case of #2.
    //
    ASSERT(m_RegisteredState == WmiUnregistered ||
              m_RegisteredState == WmiDeregistered);
    ASSERT(m_UpdateCount == 0 || m_UpdateCount == 1);

    //
    // Update count is biased to 1 when created so do the same here.
    //
    m_UpdateCount = 1;
    m_UpdateEvent.Initialize();
    Unlock(irql);
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::AddProviderLocked(
    __in      FxWmiProvider* Provider,
    __in      KIRQL OldIrql,
    __out_opt PBOOLEAN Update
    )
{
    BOOLEAN update;

    update = FALSE;

    switch (m_RegisteredState) {
    case WmiRegistered:
        if (Provider->m_Flags & WdfWmiProviderTracing) {
            update = TRUE;
        }
    case WmiUnregistered:
        break;

    case WmiDeregistered:
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // Didn't find it in the list, add it
    //
    m_NumProviders++;
    InsertTailList(&m_ProvidersListHead, &Provider->m_ListEntry);

    if (update) {
        update = DeferUpdateLocked(OldIrql);

        if (Update != NULL) {
            *Update = update;
        }
    }

    return STATUS_SUCCESS;
}

BOOLEAN
FxWmiIrpHandler::DeferUpdateLocked(
    __in KIRQL OldIrql
    )
{
    BOOLEAN checkQueue;

    checkQueue = FALSE;

    //
    // Check to see if the caller is going to return to something > PASSIVE_LEVEL.
    // If so, then always defer to the workitem.
    //
    if (OldIrql > PASSIVE_LEVEL) {
        checkQueue = TRUE;
    }
    else {
        //
        // At passive level and updates are allowed, indicate to the caller to
        // update. The caller will do registration update outside of lock but
        // update count needs to be incremented under lock, so this is done here.
        //
        IncrementUpdateCount();
        return TRUE;
    }

    if (checkQueue && m_WorkItemQueued == FALSE) {
        //
        // we are going to queue a workitem which will do registration update,
        // so increment the update count. Note that one work item may correspond
        // to multiple requests to update (since if the workitem is already
        // queued, it's not queued again).
        //
        IncrementUpdateCount();

        m_WorkItemQueued = TRUE;
        IoQueueWorkItem(m_WorkItem,
                        _UpdateGuids,
                        DelayedWorkQueue,
                        this);
    }

    return FALSE;
}

VOID
FxWmiIrpHandler::_UpdateGuids(
    __in PDEVICE_OBJECT DeviceObject,
    __in PVOID Context
    )
{
    FxWmiIrpHandler* pThis;
    KIRQL irql;

    UNREFERENCED_PARAMETER(DeviceObject);

    pThis = (FxWmiIrpHandler*)Context;

    pThis->UpdateGuids();

    pThis->Lock(&irql);
    pThis->m_WorkItemQueued = FALSE;

    if (pThis->m_WorkItemEvent != NULL) {
        KeSetEvent(pThis->m_WorkItemEvent, FALSE, IO_NO_INCREMENT);
    }
    pThis->Unlock(irql);
}

VOID
FxWmiIrpHandler::UpdateGuids(
    VOID
    )
{
    NTSTATUS status;

    status = IoWMIRegistrationControl(GetDevice()->GetDeviceObject(),
                                      WMIREG_ACTION_UPDATE_GUIDS);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
            "IoWMIRegistrationControl DevObj %p, for UpdateGuids failed, %!STATUS!",
            GetDevice()->GetDeviceObject(), status);

        //
        // Just drop the error
        //
    }

    //
    // The caller incremented the update count when it decided to do the update.
    // Decrement the count now that we have completed registration update.
    //
    DecrementUpdateCount();
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::AddProvider(
    __in      FxWmiProvider* Provider,
    __out_opt PBOOLEAN Update
    )
{
    NTSTATUS status;
    KIRQL irql;

    Lock(&irql);

    if (IsListEmpty(&Provider->m_ListEntry) == FALSE ||
        FindProviderLocked(Provider->GetGUID()) != NULL) {
        status = STATUS_OBJECT_NAME_EXISTS;
    }
    else {
        status = AddProviderLocked(Provider, irql, Update);
    }
    Unlock(irql);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::AddPowerPolicyProviderAndInstance(
    __in    PWDF_WMI_PROVIDER_CONFIG ProviderConfig,
    __in    FxWmiInstanceInternalCallbacks* InstanceCallbacks,
    __inout FxWmiInstanceInternal** Instance
    )
{
    FxWmiProvider* pProvider;
    FxWmiInstanceInternal* pInstance;
    NTSTATUS status;
    KIRQL irql;
    BOOLEAN providerAllocated, providerAdded, update;

    providerAllocated = FALSE;
    providerAdded = FALSE;
    update = FALSE;

    pInstance = NULL;

    status = STATUS_SUCCESS;

    Lock(&irql);

    pProvider = FindProviderLocked(&ProviderConfig->Guid);

    if (pProvider == NULL) {
        pProvider = new (GetDriverGlobals(), WDF_NO_OBJECT_ATTRIBUTES)
            FxWmiProvider(GetDriverGlobals(), ProviderConfig, GetDevice());

        if (pProvider == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        providerAllocated = TRUE;

        status = AddProviderLocked(pProvider, irql);
        if (!NT_SUCCESS(status)) {
            goto Done;
        }

        providerAdded = TRUE;

        status = pProvider->AssignParentObject(GetDevice());
        if (!NT_SUCCESS(status)) {
            goto Done;
        }
    }
    else if (pProvider->m_NumInstances > 0 &&
             (FxIsEqualGuid(pProvider->GetGUID(), &GUID_POWER_DEVICE_ENABLE) ||
              FxIsEqualGuid(pProvider->GetGUID(), &GUID_POWER_DEVICE_WAKE_ENABLE))) {

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIO,
                            "WMI Guid already registered by client driver");
        status = STATUS_WMI_GUID_DISCONNECTED;
    }

    if (NT_SUCCESS(status)) {
        pInstance = new (GetDriverGlobals(), WDF_NO_OBJECT_ATTRIBUTES)
            FxWmiInstanceInternal(GetDriverGlobals(), InstanceCallbacks, pProvider);

        if (pInstance != NULL) {
            status = pInstance->AssignParentObject(pProvider);
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(status) &&
            InterlockedCompareExchangePointer((PVOID*) Instance,
                                              pInstance,
                                              NULL) != NULL) {
            //
            // Some other thread got here first and created an instance, just
            // bail out.  the caller will handle this specific return value
            // gracefully.
            //
            status = STATUS_OBJECT_NAME_COLLISION;
        }
        else {
            //
            // We are the first one to set the value, good for us.
            //
            DO_NOTHING();
        }
    }

    //
    // Add the instance only if we are successful to this point
    //
    if (NT_SUCCESS(status)) {
        //
        // Passing FALSE indicates that this instance cannot be in the list
        // when added.
        //
        status = pProvider->AddInstanceLocked(
            pInstance,
            FALSE,
            &update,
            FxWmiProvider::AddInstanceToHead
            );
    }

Done:
    if (NT_SUCCESS(status)) {
        if (update) {
            update = DeferUpdateLocked(irql);
        }
    }
    else if (providerAdded) {
        RemoveProviderLocked(pProvider);
    }

    Unlock(irql);

    if (NT_SUCCESS(status)) {
        if (update) {
            UpdateGuids();
        }
    }
    else {
        if (pInstance != NULL) {
            pInstance->DeleteObject();
        }

        if (providerAllocated) {
            pProvider->DeleteObject();
        }
    }

    return status;
}

VOID
FxWmiIrpHandler::RemoveProvider(
    __in FxWmiProvider* Provider
    )
{
    KIRQL irql;

    //
    // No need to update via IoWMIRegistrationControl because this is only
    // called in the error path of creating a provider.
    //
    Lock(&irql);
    RemoveProviderLocked(Provider);
    Unlock(irql);
}

VOID
FxWmiIrpHandler::RemoveProviderLocked(
    __in FxWmiProvider* Provider
    )
{
    m_NumProviders--;
    RemoveEntryList(&Provider->m_ListEntry);
    InitializeListHead(&Provider->m_ListEntry);
}

_Must_inspect_result_
FxWmiProvider*
FxWmiIrpHandler::FindProviderLocked(
    __in LPGUID Guid
    )
{
    FxWmiProvider* pFound;
    PLIST_ENTRY ple;

    pFound = NULL;

    for (ple = m_ProvidersListHead.Flink;
         ple != &m_ProvidersListHead;
         ple = ple->Flink) {

        FxWmiProvider* pProvider;

        pProvider = CONTAINING_RECORD(ple, FxWmiProvider, m_ListEntry);

        if (RtlCompareMemory(&pProvider->m_Guid,
                             Guid,
                             sizeof(GUID)) == sizeof(GUID)) {
            pFound = pProvider;
            break;
        }
    }

    return pFound;
}

_Must_inspect_result_
FxWmiProvider*
FxWmiIrpHandler::FindProviderReferenced(
    __in LPGUID Guid,
    __in PVOID Tag
    )
{
    FxWmiProvider* pProvider;
    KIRQL irql;

    Lock(&irql);
    pProvider = FindProviderLocked(Guid);
    if (pProvider != NULL) {
        pProvider->ADDREF(Tag);
    }
    Unlock(irql);

    return pProvider;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::Dispatch(
    __in PIRP Irp
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxWmiProvider* pProvider;
    FxWmiInstance* pInstance;
    PIO_STACK_LOCATION stack;
    PDEVICE_OBJECT pAttached;
    NTSTATUS status;
    PVOID pTag;
    ULONG instanceIndex;
    KIRQL irql;
    BOOLEAN handled, completeNow;
    UCHAR minor;

    pFxDriverGlobals = GetDriverGlobals();

    FX_TRACK_DRIVER(pFxDriverGlobals);

    stack = IoGetCurrentIrpStackLocation(Irp);
    minor = stack->MinorFunction;
    pTag = UlongToPtr(minor);
    status = Irp->IoStatus.Status;

    pProvider = NULL;
    pInstance = NULL;

    handled = FALSE;
    completeNow = FALSE;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
        "WDFDEVICE 0x%p !devobj 0x%p IRP_MJ_SYSTEM_CONTROL, %!sysctrl! IRP 0x%p",
        m_Device->GetHandle(), m_Device->GetDeviceObject(), minor, Irp);

    //
    // Verify the minor code is within range, there is hole in the table at 0xA.
    // This check works around the hole in the table.
    //
    if (minor > IRP_MN_EXECUTE_METHOD && minor != IRP_MN_REGINFO_EX) {
        goto Done;
    }

    //
    // If the irp is not targetted at this device, send it down the stack
    //
    if (stack->Parameters.WMI.ProviderId != (UINT_PTR) m_Device->GetDeviceObject()) {
        goto Done;
    }

    if (minor == IRP_MN_REGINFO || minor == IRP_MN_REGINFO_EX) {
        status = STATUS_SUCCESS;
    }
    else {
        Lock(&irql);

        pProvider = FindProviderLocked((LPGUID)stack->Parameters.WMI.DataPath);

        if (pProvider != NULL) {
            status = STATUS_SUCCESS;
        }
        else {
            //
            // check for WMI tracing (no pProvider)
            //
            status = STATUS_WMI_GUID_NOT_FOUND;
        }

        if (NT_SUCCESS(status) && m_WmiDispatchTable[minor].CheckInstance) {
            PWNODE_SINGLE_INSTANCE pSingle;

            pSingle = (PWNODE_SINGLE_INSTANCE) stack->Parameters.WMI.Buffer;

            instanceIndex = pSingle->InstanceIndex;

            //
            // Also possible bits set in Flags related to instance names
            // WNODE_FLAG_PDO_INSTANCE_NAMES
            //
            if (pSingle->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) {
                //
                // Try to get the instance
                //
                pInstance = pProvider->GetInstanceReferencedLocked(
                    instanceIndex, pTag);

                if (pInstance == NULL) {
                    status = STATUS_WMI_INSTANCE_NOT_FOUND;
                }
            }
            else {






                status = STATUS_WMI_INSTANCE_NOT_FOUND;
            }
        }

        if (NT_SUCCESS(status)) {
            pProvider->ADDREF(pTag);
        }
        else {
            //
            // NULL out the provider so we don't deref it later.  We could not
            // use if (pProvider != NULL && NT_SUCCESS(status) in the Done: block
            // because we could have success here, but the dispatch function
            // returns error.
            //
            pProvider = NULL;
        }

        Unlock(irql);

        if (!NT_SUCCESS(status)) {
            Irp->IoStatus.Status = status;
            completeNow = TRUE;
        }
    }

    if (NT_SUCCESS(status) && m_WmiDispatchTable[minor].Handler != NULL) {
        status = m_WmiDispatchTable[minor].Handler(this,
                                                   Irp,
                                                   pProvider,
                                                   pInstance);
        handled = TRUE;
    }

Done:
    if (pInstance != NULL) {
        pInstance->RELEASE(pTag);
        pInstance = NULL;
    }

    if (pProvider != NULL) {
        pProvider->RELEASE(pTag);
        pProvider = NULL;
    }

    if (handled == FALSE) {
        pAttached = m_Device->GetAttachedDevice();
        if (completeNow || pAttached == NULL) {
            //
            // Sent to a PDO, error in the FDO handling, or controller devobj
            // style devobj, complete it here
            //
            status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        else {
            //
            // Request sent to PNP device object that is not a PDO, send down
            // the stack
            //
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(pAttached, Irp);
        }
    }

    //
    // Only release the remove lock *after* we have removed the thread entry
    // from the list because the list lifetime is tied to the FxDevice lifetime
    // and the remlock controls the lifetime of FxDevice.
    //
    // Since we never pend the wmi request, we can release the remove lock in
    // this Dispatch routine.  If we ever pended the IRPs, this would have to
    // have some more logic involved.
    //
    IoReleaseRemoveLock(
        &FxDevice::_GetFxWdmExtension(m_Device->GetDeviceObject())->IoRemoveLock,
        Irp
        );

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::_QueryAllData(
    __in FxWmiIrpHandler* This,
    __in PIRP Irp,
    __in FxWmiProvider* Provider,
    __in_opt FxWmiInstance* Instance
    )
/*++

Routine Description:
    Handles querying all instances for data.  In the case where there is not
    enough space in the buffer, we query each instance for the required amount
    of buffer space and report that.

Arguments:
    This - device instance

    Irp - WMI request for query all data

    Providre - the provider being queried

    Instance - ignored, NULL

Return Value:
    STATUS_BUFFER_TOO_SMALL in case the buffer is not large enough
    STATUS_SUCCESS on a successful query
    or other values depending on the client driver

  --*/
{
    POFFSETINSTANCEDATAANDLENGTH pOffsets;
    PWNODE_ALL_DATA pNodeData;
    PIO_STACK_LOCATION stack;
    PUCHAR pData;
    NTSTATUS status, addStatus;
    ULONG instanceCount, dataBlockOffset, lengthArraySize, i, sizeRemaining,
          lastAdjustment, bufferUsed, tempOffset;
    KIRQL irql;
    BOOLEAN tooSmall;

    UNREFERENCED_PARAMETER(Instance);

    status = STATUS_SUCCESS;

    stack = IoGetCurrentIrpStackLocation(Irp);
    pNodeData = (PWNODE_ALL_DATA) stack->Parameters.WMI.Buffer;

    lastAdjustment = 0;
    pData = NULL;
    dataBlockOffset = 0;

    //
    // This either the amount of buffer used on successful queries or the
    // amount of buffer required if the buffer is not big enough.
    //
    bufferUsed = 0;

    tooSmall = FALSE;

    if (stack->Parameters.WMI.BufferSize < sizeof(WNODE_ALL_DATA)) {
        status = STATUS_UNSUCCESSFUL;
        goto Done;
    }







    //
    // All of the provider's access is guarded by the irp handler's lock
    //
    This->Lock(&irql);
    instanceCount = Provider->m_NumInstances;
    This->Unlock(irql);











    if (instanceCount == 0) {
        status = STATUS_WMI_INSTANCE_NOT_FOUND;
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
            "Failing QueryAllData since no instances found for "
            "WDFWMIPROVIDER %p, %!STATUS!",
            Provider->GetHandle(), status);
        bufferUsed = 0;
        goto Done;
    }

    DoTraceLevelMessage(
        This->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "WDFWMIPROVIDER %p QueryAllData, num instances %d",
        Provider->GetHandle(), instanceCount);

    pNodeData->InstanceCount = instanceCount;






    pNodeData->WnodeHeader.Flags &= ~WNODE_FLAG_FIXED_INSTANCE_SIZE;

    //
    // Since value of instanceCount is not limited, do overflow-safe addition
    // and multiplication
    //
    // lengthArraySize = instanceCount * sizeof(OFFSETINSTANCEDATAANDLENGTH);
    status = RtlULongMult(instanceCount,
                          sizeof(OFFSETINSTANCEDATAANDLENGTH),
                          &lengthArraySize);

    if (NT_SUCCESS(status)) {
        // dataBlockOffset =
        // FIELD_OFFSET(WNODE_ALL_DATA, OffsetInstanceDataAndLength) + lengthArraySize;
        status = RtlULongAdd(
            FIELD_OFFSET(WNODE_ALL_DATA, OffsetInstanceDataAndLength),
            lengthArraySize,
            &tempOffset);

        if (NT_SUCCESS(status)) {
            dataBlockOffset = (ULONG) WDF_ALIGN_SIZE_UP(
                tempOffset, MEMORY_ALLOCATION_ALIGNMENT);

            if (dataBlockOffset < tempOffset) {
                status = STATUS_INTEGER_OVERFLOW;
            }
        }
    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Failing QueryAllData since integer overflow occured using"
            " provider instance count %d for WDFWMIPROVIDER %p, %!STATUS!",
            instanceCount, Provider->GetHandle(), status);
        bufferUsed = 0;
        goto Done;
    }

    pNodeData->DataBlockOffset = dataBlockOffset;

    if (dataBlockOffset <= stack->Parameters.WMI.BufferSize) {
        pOffsets = &pNodeData->OffsetInstanceDataAndLength[0];
        sizeRemaining = stack->Parameters.WMI.BufferSize - dataBlockOffset;
        pData = (PUCHAR) WDF_PTR_ADD_OFFSET(pNodeData, dataBlockOffset);
    }
    else {
        //
        // There is not enough room in the WNODE to complete the query, but
        // we still want to query all the instances to see how big a buffer the
        // caller should resend.
        //
        pOffsets = NULL;
        pData = NULL;
        sizeRemaining = 0;

        //
        // We are in the too small case
        //
        tooSmall = TRUE;
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (instanceCount > 0 && Provider->GetMinInstanceBufferSize() != 0) {
        ULONG size, minSizeAdjusted;

        size = 0;

        minSizeAdjusted = (ULONG) WDF_ALIGN_SIZE_UP(
            Provider->GetMinInstanceBufferSize(),
            MEMORY_ALLOCATION_ALIGNMENT
            );

        //
        // Quick size check to make sure there is enough buffer for all of the
        // instances.  We round up all but the last instance's minimum size so
        // that we can make sure we have enough room for an aligned bufer for
        // each instance.
        //
        // All but the last instance are using the adjusted size.  In the end,
        // we will have:
        //
        // size = minSizeAdjusted * (instanceCount-1) +
        //        Provider->GetMinInstanceBufferSize();
        //
        // NOTE:  we do not care about instances which do not support query
        //        data.  We don't care b/c by computing the total size of all
        //        instances we compute the maximum sized buffer needed, and if
        //        a few instances  do no support query data, then the buffer
        //        will just be too large, never too small.
        //
        status = RtlULongMult(minSizeAdjusted, instanceCount - 1, &size);
        if (!NT_SUCCESS(status)) {
            goto Done;
        }

        status = RtlULongAdd(size, Provider->GetMinInstanceBufferSize(), &size);
        if (!NT_SUCCESS(status)) {
            goto Done;
        }

        //
        // Since the client indicated that that each block as a minimum instance
        // size (which in reality is the fixed size, variable sized instnaces
        // should report a minimum instance size of zero), we can compute the
        // required buffer size w/out querying each instance for the required
        // size.
        //
        if (sizeRemaining < size) {
            //
            // bufferUsed in the buffer too small case indicates how many bytes
            // we want the caller to allocate.
            //
            bufferUsed = size;
            status = STATUS_BUFFER_TOO_SMALL;
            goto Done;
        }
    }

    for (i = 0; i < instanceCount; i++) {
        FxWmiInstance* pInstance;

        //
        // In the case where we have a minimum instance size, we should have
        // verified correctly above to have enough buffer.
        //
        ASSERT(sizeRemaining >= Provider->GetMinInstanceBufferSize());

        pInstance = Provider->GetInstanceReferenced(i, Irp);

        if (pInstance == NULL) {
            break;
        }

        if (pInstance->IsQueryInstanceSupported()) {
            ULONG tmpSize;

            tmpSize = 0;

            status = pInstance->QueryInstance(sizeRemaining, pData, &tmpSize);

            if (NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL) {
                ULONG adjustedSize;

                //
                // We calculate the size in both cases.  In the NT_SUCCESS case,
                // it is the amount of buffer used. In the too small case, it is
                // the amount of buffer required for a subsequent query.
                //
                adjustedSize = (ULONG) WDF_ALIGN_SIZE_UP(
                    tmpSize, MEMORY_ALLOCATION_ALIGNMENT
                    );

                if (adjustedSize < tmpSize) {
                    //
                    // Overflow, adjustedSize will be >= tmpSize in the normal
                    // case.
                    //
                    status = STATUS_INTEGER_OVERFLOW;
                    DoTraceLevelMessage(
                        This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "WDFWMIINSTNACE %p queried, returned a buffer size of %d,"
                        "but it could not be rounded up, %!STATUS!",
                        pInstance->GetHandle(), tmpSize, status);
                    goto QueryError;
                }

                //
                // Keep track of how much we adjusted up the size on the last
                // instance so that when compute the total buffer used, we
                // do not include the final size adjustment.
                //
                lastAdjustment = adjustedSize - tmpSize;

                //
                // We only write the offset and data if we have not yet
                // encountered an instance where the buffer size was too small.
                //
                if (NT_SUCCESS(status) && tooSmall == FALSE) {
                    //
                    // dataBlockOffset is where we are currently at in the buffer
                    //
                    pOffsets[i].LengthInstanceData = tmpSize;
                    pOffsets[i].OffsetInstanceData = dataBlockOffset;

                    pData += adjustedSize;
                }
                else {
                    tooSmall = TRUE;
                }

                //
                // Compute how much buffer we have left and our offset into it.
                //
                if (adjustedSize <= sizeRemaining) {
                    sizeRemaining -= adjustedSize;

                    //
                    // dataBlockOffset += adjustedSize;
                    //
                    addStatus = RtlULongAdd(dataBlockOffset,
                                            adjustedSize,
                                            &dataBlockOffset);
                }
                else {
                    //
                    // dataBlockOffset += sizeRemaining;
                    //
                    addStatus = RtlULongAdd(dataBlockOffset,
                                            sizeRemaining,
                                            &dataBlockOffset);
                    sizeRemaining = 0;
                }

                if (!NT_SUCCESS(addStatus)) {
                    status = addStatus;
                    DoTraceLevelMessage(
                        This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "WDFWMIPROVIDER %p, arithmetic overflow in computing "
                        "block offset, %!STATUS!", Provider->GetHandle(),
                        status);
                    goto QueryError;
                }

                //
                // Compute how much buffer space we have used or require the
                // caller to provide.  This is just the count of bytes for the
                // data queried, it does not yet include the array size required
                // to report offset & lengths.
                //
                // bufferUsed += adjustedSize;
                //
                addStatus = RtlULongAdd(bufferUsed, adjustedSize, &bufferUsed);

                if (!NT_SUCCESS(addStatus)) {
                    status = addStatus;
                    DoTraceLevelMessage(
                        This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "WDFWMIPROVIDER %p, arithmetic overflow in computing "
                        "buffer consumed(%d+%d), %!STATUS!",
                        Provider->GetHandle(), bufferUsed, adjustedSize, status);
                    goto QueryError;
                }
            }
        }
        else if (pOffsets != NULL) {
            //
            // Indicate no buffer and the current offset.  If there any
            // other instances after this one, they will share the same
            // offset.
            //
            pOffsets[i].LengthInstanceData = 0;
            pOffsets[i].OffsetInstanceData = dataBlockOffset;
        }

QueryError:
        pInstance->RELEASE(Irp);

        //
        // We continue on STATUS_BUFFER_TOO_SMALL so that we can ask each
        // instance the amount of buffer it needs.
        //
        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_TOO_SMALL) {
            break;
        }
    }

    //
    // We can have STATUS_BUFFER_TOO_SMALL if the last instance could not fit
    // its data into the buffer or have success if the last instance could write
    // its data, but a previous instance could not.
    //
    if (status == STATUS_BUFFER_TOO_SMALL || (NT_SUCCESS(status) && tooSmall)) {
        //
        // Since we align up the size of each block to MEMORY_ALLOCATION_ALIGNMENT,
        // the total buffer size is possibly adjusted up too high.  Subtract the
        // last adjustment made for alignment when computing the instance size
        // so that our total size is not rounded up.
        //
        // CompleteWmiQueryAllDataRequest will take care of adding enough space
        // to the requested size to account for the array of instance offsets
        // and lengths.
        //
        bufferUsed -=lastAdjustment;

        status = STATUS_BUFFER_TOO_SMALL;

        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFWMIPROVIDER %p QueryAllData returning %!STATUS!, requesting "
            "buffer size of 0x%x", Provider->GetHandle(), status, bufferUsed);
    }
    else if (NT_SUCCESS(status)) {
        //
        // Compute the total amount of buffer used.  dataBlockOffset is the
        // offset of the next instance, which is one past the end, so just
        // compute the distance from the start.
        //
        // Since align up the size of each block to MEMORY_ALLOCATION_ALIGNMENT,
        // the total buffer size is possibly adjusted up too high.  Subtract the
        // last adjustment made for alignment when computing the instance size
        // so that our total size is not rounded up.
        //
        ASSERT(dataBlockOffset >= pNodeData->DataBlockOffset);
        ASSERT(dataBlockOffset - pNodeData->DataBlockOffset >= lastAdjustment);
        bufferUsed = dataBlockOffset - pNodeData->DataBlockOffset - lastAdjustment;
    }
    else {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFWMIPROVIDER %p QueryAllData returning %!STATUS!",
            Provider->GetHandle(), status);

        bufferUsed = 0;
    }

    DoTraceLevelMessage(
        This->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "WDFWMIPROVIDER %p QueryAllData returning %!STATUS!, buffer used 0x%x",
        Provider->GetHandle(), status, bufferUsed);

Done:
    status = This->CompleteWmiRequest(Irp, status, bufferUsed);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::_QuerySingleInstance(
    __in FxWmiIrpHandler* This,
    __in PIRP Irp,
    __in FxWmiProvider* Provider,
    __in FxWmiInstance* Instance
    )
{
    PWNODE_SINGLE_INSTANCE pSingle;
    PIO_STACK_LOCATION stack;
    NTSTATUS status;
    ULONG size, bufferSize;

    size = 0;

    stack = IoGetCurrentIrpStackLocation(Irp);
    pSingle = (PWNODE_SINGLE_INSTANCE) stack->Parameters.WMI.Buffer;
    bufferSize = stack->Parameters.WMI.BufferSize - pSingle->DataBlockOffset;

    if (Instance->IsQueryInstanceSupported() == FALSE) {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else if (bufferSize < Provider->GetMinInstanceBufferSize()) {
        size = Provider->GetMinInstanceBufferSize();
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        status = Instance->QueryInstance(
            bufferSize,
            WDF_PTR_ADD_OFFSET(pSingle, pSingle->DataBlockOffset),
            &size
            );

        pSingle->SizeDataBlock = size;
    }

    status = This->CompleteWmiRequest(Irp, status, size);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::_ChangeSingleInstance(
    __in FxWmiIrpHandler* This,
    __in PIRP Irp,
    __in FxWmiProvider* Provider,
    __in FxWmiInstance* Instance
    )
{
    PWNODE_SINGLE_INSTANCE pSingle;
    PIO_STACK_LOCATION stack;
    NTSTATUS status;
    ULONG size;

    size = 0;

    stack = IoGetCurrentIrpStackLocation(Irp);
    pSingle = (PWNODE_SINGLE_INSTANCE) stack->Parameters.WMI.Buffer;

    if (Instance->IsSetInstanceSupported() == FALSE) {
        status = STATUS_WMI_READ_ONLY;
    }
    else if (pSingle->SizeDataBlock < Provider->m_MinInstanceBufferSize) {
        size = Provider->m_MinInstanceBufferSize;
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        size = pSingle->SizeDataBlock;

        status = Instance->SetInstance(
            pSingle->SizeDataBlock,
            size == 0 ?
                NULL : WDF_PTR_ADD_OFFSET(pSingle, pSingle->DataBlockOffset)
            );

        ASSERT(status != STATUS_PENDING);
        if (status == STATUS_PENDING) {
            status = STATUS_UNSUCCESSFUL;
            size = 0;
        }
    }

    status = This->CompleteWmiRequest(Irp, status, size);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::_ChangeSingleItem(
    __in FxWmiIrpHandler* This,
    __in PIRP Irp,
    __in FxWmiProvider* Provider,
    __in FxWmiInstance* Instance
    )
{
    PWNODE_SINGLE_ITEM pSingle;
    NTSTATUS status;
    ULONG size;

    UNREFERENCED_PARAMETER(Provider);

    size = 0;

    pSingle = (PWNODE_SINGLE_ITEM) IoGetCurrentIrpStackLocation(Irp)->
        Parameters.WMI.Buffer;

    if (Instance->IsSetItemSupported() == FALSE) {
        status = STATUS_WMI_READ_ONLY;
    }
    else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        status = Instance->SetItem(
            pSingle->ItemId,
            pSingle->SizeDataItem,
            pSingle->SizeDataItem == 0 ?
                NULL : WDF_PTR_ADD_OFFSET(pSingle, pSingle->DataBlockOffset)
            );

        ASSERT(status != STATUS_PENDING);
        if (status == STATUS_PENDING) {
            status = STATUS_UNSUCCESSFUL;
            size = 0;
        }
    }

    status = This->CompleteWmiRequest(Irp, status, size);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::_EnableDisableEventsAndCollection(
    __in FxWmiIrpHandler* This,
    __in PIRP Irp,
    __in FxWmiProvider* Provider,
    __in FxWmiInstance* Instance
    )
{
    NTSTATUS status;
    BOOLEAN enable;
    WDF_WMI_PROVIDER_CONTROL control;
    PIO_STACK_LOCATION stack;

    UNREFERENCED_PARAMETER(This);
    UNREFERENCED_PARAMETER(Instance);

    Irp->IoStatus.Information = 0;

    stack = IoGetCurrentIrpStackLocation(Irp);

    if (stack->Parameters.WMI.BufferSize < sizeof(WNODE_HEADER)) {
        status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_ENABLE_EVENTS:
        enable = TRUE;
        control =  WdfWmiEventControl;
        break;

    case IRP_MN_DISABLE_EVENTS:
        enable = FALSE;
        control = WdfWmiEventControl;
        break;

    case IRP_MN_ENABLE_COLLECTION:
        enable = TRUE;
        control = WdfWmiInstanceControl;
        break;

    case IRP_MN_DISABLE_COLLECTION:
        enable = FALSE;
        control = WdfWmiInstanceControl;
        break;

    default:
        status = Irp->IoStatus.Status;
        goto Done;
    }

    if (control == WdfWmiEventControl) {
        Provider->m_EventControlEnabled = enable;

        //
        // Capture the tracing information before making the callback
        //
        if (Provider->m_Flags & WdfWmiProviderTracing) {
            Provider->SetTracingHandle(
                ((PWNODE_HEADER) stack->Parameters.WMI.Buffer)->HistoricalContext
                );
        }
    }
    else {
        Provider->m_DataBlockControlEnabled = enable;
    }

    if (Provider->IsFunctionControlSupported()) {
        status = Provider->FunctionControl(control, enable);
    }
    else {
        status = STATUS_SUCCESS;
    }

    ASSERT(status != STATUS_PENDING);
    if (status == STATUS_PENDING) {
        status = STATUS_UNSUCCESSFUL;
    }

    //
    // Undo the previous capture on error
    //
    if (!NT_SUCCESS(status)) {
        if (control == WdfWmiEventControl) {
            Provider->m_EventControlEnabled = FALSE;

            if (Provider->m_Flags & WdfWmiProviderTracing) {
                Provider->SetTracingHandle(NULL);
            }
        }
        else {
            Provider->m_DataBlockControlEnabled = FALSE;
        }

    }

Done:
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::_ExecuteMethod(
    __in FxWmiIrpHandler* This,
    __in PIRP Irp,
    __in FxWmiProvider* Provider,
    __in FxWmiInstance* Instance
    )
{
    PWNODE_METHOD_ITEM pMethod;
    PIO_STACK_LOCATION stack;
    NTSTATUS status;
    ULONG size, inBufferSize, outBufferSize;

    UNREFERENCED_PARAMETER(This);
    UNREFERENCED_PARAMETER(Provider);

    size = 0;

    stack = IoGetCurrentIrpStackLocation(Irp);
    pMethod = (PWNODE_METHOD_ITEM) stack->Parameters.WMI.Buffer;
    inBufferSize = pMethod->SizeDataBlock;
    outBufferSize = stack->Parameters.WMI.BufferSize - pMethod->DataBlockOffset;











    if (Instance->IsExecuteMethodSupported() == FALSE) {
        //
        // WmiLib returns this value when there is no execute method function
        // pointer specified to it.
        //
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        status = Instance->ExecuteMethod(
            pMethod->MethodId,
            inBufferSize,
            outBufferSize,
            (inBufferSize == 0 && outBufferSize == 0) ?
                NULL : WDF_PTR_ADD_OFFSET(pMethod, pMethod->DataBlockOffset),
            &size
            );

        ASSERT(status != STATUS_PENDING);
        if (status == STATUS_PENDING) {
            status = STATUS_UNSUCCESSFUL;
            size = 0;
        }
    }

    status = This->CompleteWmiRequest(Irp, status, size);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::_RegInfo(
    __in FxWmiIrpHandler* This,
    __in PIRP Irp,
    __in_opt FxWmiProvider* Provider,
    __in_opt FxWmiInstance* Instance
    )
{
    PIO_STACK_LOCATION stack;
    PUNICODE_STRING pRegPath;
    PWMIREGINFO pWmiRegInfo;
    PUCHAR pBuffer;
    PUNICODE_STRING pMofString;
    FxDevice* pDevice;
    NTSTATUS status;
    ULONG registryPathOffset, mofResourceOffset, bufferNeeded, information,
          bufferSize;
    KIRQL irql;

    UNREFERENCED_PARAMETER(Provider);
    UNREFERENCED_PARAMETER(Instance);

    information = 0;

    stack = IoGetCurrentIrpStackLocation(Irp);
    pBuffer = (PUCHAR) stack->Parameters.WMI.Buffer;
    bufferSize = stack->Parameters.WMI.BufferSize;

    pDevice = This->m_Device;

    This->Lock(&irql);

    mofResourceOffset = sizeof(WMIREGINFO) +
                        This->m_NumProviders * sizeof(WMIREGGUIDW);

    pMofString = NULL;

    if (pDevice->m_MofResourceName.Buffer != NULL) {
        pMofString = &pDevice->m_MofResourceName;
    }
    else {
        //
        // Start with the parent and iterate up until we hit a device without
        // a parent.
        //
        pDevice = pDevice->m_ParentDevice;

        while (pDevice != NULL) {
            if (pDevice->m_MofResourceName.Buffer != NULL) {
                pMofString = &pDevice->m_MofResourceName;
                break;
            }

            //
            // Advance to the next ancestor
            //
            pDevice = pDevice->m_ParentDevice;
        }

        //
        // Restore pDevice back to this device
        //
        pDevice = This->m_Device;
    }

    pRegPath = pDevice->GetDriver()->GetRegistryPathUnicodeString();

    //
    // if there is a mof string, add its length.  We always need to at least
    // add the USHORT to indicate a string of size 0.
    //
    registryPathOffset = mofResourceOffset + sizeof(USHORT);
    if (pMofString != NULL) {
        registryPathOffset += pMofString->Length;
    }

    //
    // eventually bufferNeeded = registryPathOffset + pRegPath->Length +
    //                           sizeof(USHORT)
    //
    status = RtlULongAdd(registryPathOffset, pRegPath->Length, &bufferNeeded);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    status = RtlULongAdd(bufferNeeded, sizeof(USHORT), &bufferNeeded);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    if (bufferNeeded <= bufferSize) {
        PLIST_ENTRY ple;
        ULONG i;
        BOOLEAN addref;

        information = bufferNeeded;

        pWmiRegInfo = (PWMIREGINFO) pBuffer;
        pWmiRegInfo->BufferSize = bufferNeeded;
        pWmiRegInfo->NextWmiRegInfo = 0;
        pWmiRegInfo->MofResourceName = mofResourceOffset;
        pWmiRegInfo->RegistryPath = registryPathOffset;
        pWmiRegInfo->GuidCount = This->m_NumProviders;

        addref = IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_REGINFO_EX
            ? TRUE : FALSE;

        for (ple = This->m_ProvidersListHead.Flink, i = 0;
             i < This->m_NumProviders;
             ple = ple->Flink, i++) {

            PWMIREGGUIDW pWmiRegGuid;
            PDEVICE_OBJECT pdo;
            FxWmiProvider* pProvider;

            pProvider = CONTAINING_RECORD(ple, FxWmiProvider, m_ListEntry);

            pWmiRegGuid = &pWmiRegInfo->WmiRegGuid[i];

            RtlCopyMemory(&pWmiRegGuid->Guid,
                          &pProvider->m_Guid,
                          sizeof(GUID));

            pWmiRegGuid->InstanceCount = pProvider->m_NumInstances;

            pWmiRegGuid->Flags = pProvider->GetRegistrationFlagsLocked();

            pdo = pDevice->GetPhysicalDevice();
            pWmiRegGuid->Pdo = (ULONG_PTR) pdo;

            if (addref) {
                ObReferenceObject(pdo);
            }
        }
    }
    else {
        *((PULONG) pBuffer) = bufferNeeded;
        information = sizeof(ULONG);
    }

    This->Unlock(irql);

    //
    // Must copy the strings outside of any lock since they are in paged pool
    //
    if (bufferNeeded <= bufferSize) {
        PUSHORT pLength;

        pLength = WDF_PTR_ADD_OFFSET_TYPE(pBuffer, mofResourceOffset, PUSHORT);

        if (pMofString != NULL) {
            *pLength = pMofString->Length;

            RtlCopyMemory(pLength + 1,
                          pMofString->Buffer,
                          pMofString->Length);
        }
        else {
            *pLength = 0;
        }

        pLength = WDF_PTR_ADD_OFFSET_TYPE(pBuffer, registryPathOffset, PUSHORT);

        *pLength = pRegPath->Length;
        RtlCopyMemory(pLength + 1,
                      pRegPath->Buffer,
                      pRegPath->Length);
    }

    status = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(status)) {
        This->Unlock(irql);
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = information;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

VOID
FxWmiIrpHandler::CompleteWmiQueryAllDataRequest(
    __in PIRP Irp,
    __in NTSTATUS Status,
    __in ULONG BufferUsed
    )
{
    PWNODE_ALL_DATA pNodeAllData;
    ULONG bufferNeeded, information;
    PIO_STACK_LOCATION stack;

    stack = IoGetCurrentIrpStackLocation(Irp);

    pNodeAllData = (PWNODE_ALL_DATA) stack->Parameters.WMI.Buffer;
    bufferNeeded = pNodeAllData->DataBlockOffset + BufferUsed;

    if (NT_SUCCESS(Status) && bufferNeeded > stack->Parameters.WMI.BufferSize) {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(Status)) {
        KeQuerySystemTime(&pNodeAllData->WnodeHeader.TimeStamp);

        pNodeAllData->WnodeHeader.BufferSize = bufferNeeded;
        information = bufferNeeded;
    }
    else if (Status == STATUS_BUFFER_TOO_SMALL) {
        PWNODE_TOO_SMALL pNodeTooSmall;

        pNodeTooSmall = (PWNODE_TOO_SMALL) stack->Parameters.WMI.Buffer;

        pNodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
        pNodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
        pNodeTooSmall->SizeNeeded = bufferNeeded;

        information = sizeof(WNODE_TOO_SMALL);
        Status = STATUS_SUCCESS;
    }
    else {
        information = 0;
    }

    Irp->IoStatus.Information = information;
    Irp->IoStatus.Status = Status;
}

VOID
FxWmiIrpHandler::CompleteWmiQuerySingleInstanceRequest(
    __in PIRP Irp,
    __in NTSTATUS Status,
    __in ULONG BufferUsed
    )
{
    PWNODE_SINGLE_INSTANCE pNodeSingle;
    ULONG bufferNeeded, information;

    pNodeSingle = (PWNODE_SINGLE_INSTANCE)
        IoGetCurrentIrpStackLocation(Irp)->Parameters.WMI.Buffer;

    bufferNeeded = pNodeSingle->DataBlockOffset + BufferUsed;

    if (NT_SUCCESS(Status)) {
        pNodeSingle->WnodeHeader.BufferSize = bufferNeeded;
        KeQuerySystemTime(&pNodeSingle->WnodeHeader.TimeStamp);

        ASSERT(pNodeSingle->SizeDataBlock <= BufferUsed);
        information = bufferNeeded;
    }
    else if (Status == STATUS_BUFFER_TOO_SMALL) {
        PWNODE_TOO_SMALL pNodeTooSmall;

        pNodeTooSmall = (PWNODE_TOO_SMALL) pNodeSingle;

        pNodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
        pNodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
        pNodeTooSmall->SizeNeeded = bufferNeeded;

        information = sizeof(WNODE_TOO_SMALL);
        Status = STATUS_SUCCESS;
    }
    else {
        information = 0;
    }

    Irp->IoStatus.Information = information;
    Irp->IoStatus.Status = Status;
}

VOID
FxWmiIrpHandler::CompleteWmiExecuteMethodRequest(
    __in PIRP Irp,
    __in NTSTATUS Status,
    __in ULONG BufferUsed
    )
{
    PWNODE_METHOD_ITEM pNodeMethod;
    ULONG bufferNeeded, information;

    pNodeMethod = (PWNODE_METHOD_ITEM)
        IoGetCurrentIrpStackLocation(Irp)->Parameters.WMI.Buffer;

    bufferNeeded = pNodeMethod->DataBlockOffset + BufferUsed;

    if (NT_SUCCESS(Status)) {
        pNodeMethod->WnodeHeader.BufferSize = bufferNeeded;
        pNodeMethod->SizeDataBlock = BufferUsed;
        KeQuerySystemTime(&pNodeMethod->WnodeHeader.TimeStamp);

        information = bufferNeeded;
    }
    else if (Status == STATUS_BUFFER_TOO_SMALL) {
        PWNODE_TOO_SMALL pNodeTooSmall;

        pNodeTooSmall = (PWNODE_TOO_SMALL) pNodeMethod;

        pNodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
        pNodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
        pNodeTooSmall->SizeNeeded = bufferNeeded;

        information = sizeof(WNODE_TOO_SMALL);
        Status = STATUS_SUCCESS;
    }
    else {
        information = 0;
    }

    Irp->IoStatus.Information = information;
    Irp->IoStatus.Status = Status;
}

_Must_inspect_result_
NTSTATUS
FxWmiIrpHandler::CompleteWmiRequest(
    __in PIRP Irp,
    __in NTSTATUS Status,
    __in ULONG BufferUsed
    )
/*++

Routine Description:
    This routine will do the work of completing a WMI irp. Depending upon the
    the WMI request this routine will fixup the returned WNODE appropriately.

    This may be called at DPC level

Arguments:

    Irp - Supplies the Irp making the request.

    Status has the return status code for the IRP

    BufferUsed has the number of bytes needed by the device to return the
       data requested in any query. In the case that the buffer passed to
       the device is too small this has the number of bytes needed for the
       return data. If the buffer passed is large enough then this has the
       number of bytes actually used by the device.

Return Value:

    status

--*/
{
    switch (IoGetCurrentIrpStackLocation(Irp)->MinorFunction) {
    case IRP_MN_QUERY_ALL_DATA:
        CompleteWmiQueryAllDataRequest(Irp, Status, BufferUsed);
        break;

    case IRP_MN_QUERY_SINGLE_INSTANCE:
        CompleteWmiQuerySingleInstanceRequest(Irp, Status, BufferUsed);
        break;

    case IRP_MN_EXECUTE_METHOD:
        CompleteWmiExecuteMethodRequest(Irp, Status, BufferUsed);
        break;

    case IRP_MN_CHANGE_SINGLE_INSTANCE:
    case IRP_MN_CHANGE_SINGLE_ITEM:
        //
        // Lot's of drivers return STATUS_BUFFER_TOO_SMALL for an invalid buffer
        // size.  WMI expects STATUS_WMI_SET_FAILURE in this case.  Change the
        // Status to the expected value.  No way to return any size in
        // Information, the buffer is input only.
        //
        if (Status == STATUS_BUFFER_TOO_SMALL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "Converting %!STATUS! to %!STATUS!",
                Status, STATUS_WMI_SET_FAILURE);
            Status = STATUS_WMI_SET_FAILURE;
        }
        //  ||   ||  Fall through  ||   ||
        //  \/   \/                \/   \/

    default:
        //
        // All other requests don't return any data
        //
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        break;
    }

    //
    // One of the complete functions may have morphed the status value
    //
    Status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

