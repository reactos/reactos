/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Verifier.cpp

Abstract:

    This file has implementation of verifier support routines

Author:



Environment:

    Shared (Kernel and user)

Revision History:



--*/


#include "vfpriv.hpp"

extern "C"
{

extern WDFVERSION WdfVersion;

// Tracing support
#if defined(EVENT_TRACING)
#include "verifier.tmh"
#endif

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(FX_ENHANCED_VERIFIER_SECTION_NAME,  \
    AddEventHooksWdfDeviceCreate,                      \
    AddEventHooksWdfIoQueueCreate,                     \
    VfAddContextToHandle,                              \
    VfAllocateContext,                                 \
    VfWdfObjectGetTypedContext                        \
    )
#endif

_Must_inspect_result_
NTSTATUS
AddEventHooksWdfDeviceCreate(
    __inout PVF_HOOK_PROCESS_INFO HookProcessInfo,
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in PWDFDEVICE_INIT* DeviceInit,
    __in PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
    __out WDFDEVICE* Device
    )
/*++

Routine Description:

    This routine is called by main hook for WdfDeviceCreate.
    The routine swaps the event callbacks provided by client.

Arguments:

Return value:

--*/
{
    NTSTATUS status;
    PWDFDEVICE_INIT deviceInit = *DeviceInit;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerEvtsOriginal;
    WDF_PNPPOWER_EVENT_CALLBACKS *pnpPowerEvts;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    PVOID contextHeader = NULL;
    WDF_OBJECT_ATTRIBUTES  attributes;

    PAGED_CODE_LOCKED();

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DeviceInit);
    FxPointerNotNull(pFxDriverGlobals, *DeviceInit);
    FxPointerNotNull(pFxDriverGlobals, Device);

    //
    // Check if there are any callbacks set by the client driver. If not, we
    // don't need any callback hooking.
    //
    if (deviceInit->PnpPower.PnpPowerEventCallbacks.Size !=
        sizeof(WDF_PNPPOWER_EVENT_CALLBACKS)) {
        //
        // no hooking required.
        //
        status = STATUS_SUCCESS;
        HookProcessInfo->DonotCallKmdfLib = FALSE;
        return status;
    }

    //
    // Hooking can be done only if we are able to allocate context memory.
    // Try to allocate a context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
                                            &attributes,
                                            VF_WDFDEVICECREATE_CONTEXT
                                            );

    status = VfAllocateContext(DriverGlobals, &attributes, &contextHeader);

    if (!NT_SUCCESS(status)) {
        //
        // couldn't allocate context. hooking not possible
        //
        HookProcessInfo->DonotCallKmdfLib = FALSE;
        return status;
    }

    //
    // store original driver callbacks to local variable
    //
    RtlCopyMemory(&pnpPowerEvtsOriginal,
        &deviceInit->PnpPower.PnpPowerEventCallbacks,
        sizeof(WDF_PNPPOWER_EVENT_CALLBACKS)
        );

    //
    // Set callback hooks
    //
    // Normally override the hooks on local copy of stucture (not the original
    // structure itself) and then pass the local struture to DDI call and
    // copy back the original poniter when returning back to caller. In this case
    // device_init is null'ed out by fx when returning to caller so we can
    // directly override the original
    //
    pnpPowerEvts = &deviceInit->PnpPower.PnpPowerEventCallbacks;

    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceD0Entry);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceD0EntryPostInterruptsEnabled);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceD0Exit);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceD0ExitPreInterruptsDisabled);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDevicePrepareHardware);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceReleaseHardware);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceSelfManagedIoCleanup);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceSelfManagedIoFlush);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceSelfManagedIoInit);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceSelfManagedIoSuspend);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceSelfManagedIoRestart);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceSurpriseRemoval);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceQueryRemove);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceQueryStop);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceUsageNotification);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceUsageNotificationEx);
    SET_HOOK_IF_CALLBACK_PRESENT(pnpPowerEvts, pnpPowerEvts, EvtDeviceRelationsQuery);

    //
    // Call the DDI on behalf of driver.
    //
    status = ((PFN_WDFDEVICECREATE)WdfVersion.Functions.pfnWdfDeviceCreate)(
                DriverGlobals,
                DeviceInit,
                DeviceAttributes,
                Device
                );
    //
    // Tell main hook routine not to call the DDI in kmdf lib since we
    // already called it
    //
    HookProcessInfo->DonotCallKmdfLib = TRUE;
    HookProcessInfo->DdiCallStatus = status;

    //
    // if DDI Succeeds, add context
    //
    if (NT_SUCCESS(status)) {
        PVF_WDFDEVICECREATE_CONTEXT context = NULL;

        //
        // add the already allocated context to the object.
        //
        status = VfAddContextToHandle(contextHeader,
                                      &attributes,
                                      *Device,
                                      (PVOID *)&context);

        if (NT_SUCCESS(status)) {
            //
            // store the DriverGlobals pointer used to associate the driver
            // with its client driver callback later in the callback hooks
            //
            context->CommonHeader.DriverGlobals = DriverGlobals;

            //
            // store original client driver callbacks in context
            //
            RtlCopyMemory(
                &context->PnpPowerEventCallbacksOriginal,
                &pnpPowerEvtsOriginal,
                sizeof(WDF_PNPPOWER_EVENT_CALLBACKS)
                );
        }
        else {
            //
            // For some reason adding context to handle failed. This should be
            // rare failure, because context allocation was already successful,
            // only adding to handle failed.
            //
            // Hooking failed if adding context is unsuccessful. This means
            // kmdf has callbacks hooks but verifier cannot assiociate client
            // driver callbacks with callback hooks. This would be a fatal error.
            //
            ASSERTMSG("KMDF Enhanced Verifier failed to add context to object "
                "handle\n", FALSE);

            if (contextHeader != NULL) {
                FxPoolFree(contextHeader);
            }
        }
    }
    else {
        //
        // KMDF DDI call failed. In case of failure, DeviceInit is not NULL'ed
        // so the driver could potentially call WdfDeviceCreate again with this
        // DeviceInit that contains callback hooks, and result in infinite
        // callback loop. So put original callbacks back
        //
        if ((*DeviceInit) != NULL) {
            //
            // we overwrote only the pnppower callbacks. Put the original back.
            //
            deviceInit = *DeviceInit;
            RtlCopyMemory(&deviceInit->PnpPower.PnpPowerEventCallbacks,
                &pnpPowerEvtsOriginal,
                sizeof(WDF_PNPPOWER_EVENT_CALLBACKS)
                );
        }

        if (contextHeader != NULL) {
            FxPoolFree(contextHeader);
        }
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
AddEventHooksWdfIoQueueCreate(
    __inout PVF_HOOK_PROCESS_INFO HookProcessInfo,
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in WDFDEVICE Device,
    __in PWDF_IO_QUEUE_CONFIG Config,
    __in PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    __out WDFQUEUE* Queue
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG  configNew;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDFQUEUE *pQueue;
    WDFQUEUE queue;
    PVOID contextHeader = NULL;
    WDF_OBJECT_ATTRIBUTES  attributes;

    PAGED_CODE_LOCKED();

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Config);

    //
    // Check if there are any callbacks set by the client driver. If not, we
    // don't need any callback hooking.
    //
    if (Config->Size != sizeof(WDF_IO_QUEUE_CONFIG)) {
        //
        // no hooking required.
        //
        status = STATUS_SUCCESS;
        HookProcessInfo->DonotCallKmdfLib = FALSE;
        return status;
    }

    //
    // Hooking can be done only if we are able to allocate context memory.
    // Try to allocate a context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                            VF_WDFIOQUEUECREATE_CONTEXT);

    status = VfAllocateContext(DriverGlobals, &attributes, &contextHeader);

    if (!NT_SUCCESS(status)) {
        //
        // couldn't allocate context. hooking not possible
        //
        HookProcessInfo->DonotCallKmdfLib = FALSE;
        return status;
    }

    //
    // create a local copy of config
    //
    RtlCopyMemory(&configNew,
        Config,
        sizeof(configNew)
        );

    //
    // Override local copy with event callback hooks.
    //
    SET_HOOK_IF_CALLBACK_PRESENT(Config, &configNew, EvtIoDefault);
    SET_HOOK_IF_CALLBACK_PRESENT(Config, &configNew, EvtIoRead);
    SET_HOOK_IF_CALLBACK_PRESENT(Config, &configNew, EvtIoWrite);
    SET_HOOK_IF_CALLBACK_PRESENT(Config, &configNew, EvtIoDeviceControl);
    SET_HOOK_IF_CALLBACK_PRESENT(Config, &configNew, EvtIoInternalDeviceControl);
    SET_HOOK_IF_CALLBACK_PRESENT(Config, &configNew, EvtIoStop);
    SET_HOOK_IF_CALLBACK_PRESENT(Config, &configNew, EvtIoResume);
    SET_HOOK_IF_CALLBACK_PRESENT(Config, &configNew, EvtIoCanceledOnQueue);

    //
    // Queue handle is an optional parameter
    //
    if (Queue == NULL) {
        pQueue = &queue;
    }
    else {
        pQueue = Queue;
    }

    //
    // call the DDI
    //
    status = WdfVersion.Functions.pfnWdfIoQueueCreate(
                DriverGlobals,
                Device,
                &configNew,
                QueueAttributes,
                pQueue
                );

    //
    // Tell main hook routine not to call the DDI in kmdf lib since we
    // already called it
    //
    HookProcessInfo->DonotCallKmdfLib = TRUE;
    HookProcessInfo->DdiCallStatus = status;

    //
    // if DDI Succeeds, add context
    //
    if (NT_SUCCESS(status)) {
        PVF_WDFIOQUEUECREATE_CONTEXT  context = NULL;

        //
        // add the already allocated context to the object.
        //
        status = VfAddContextToHandle(contextHeader,
                                      &attributes,
                                      *pQueue,
                                      (PVOID *)&context);

        if (NT_SUCCESS(status)) {
            //
            // store the DriverGlobals pointer used to associate the driver
            // with its client driver callback later in the callback hooks
            //
            context->CommonHeader.DriverGlobals = DriverGlobals;

            //
            // add stored original callbacks to context
            //
            RtlCopyMemory(
                &context->IoQueueConfigOriginal,
                Config,
                sizeof(WDF_IO_QUEUE_CONFIG)
                );
        }
        else {
            //
            // For some reason adding context to handle failed. This should be
            // rare failure, because context allocation was already successful,
            // only adding to handle failed.
            //
            // Hooking failed if adding context is unsuccessful. This means
            // kmdf has callbacks hooks but verifier cannot assiociate client
            // driver callbacks with callback hooks. This would be a fatal error.
            //
            ASSERTMSG("KMDF Enhanced Verifier failed to add context to object "
                "handle\n", FALSE);

            if (contextHeader != NULL) {
                FxPoolFree(contextHeader);
            }
        }
    }
    else {
        //
        // DDI call to KMDF failed. Nothing to do by verifier. Just return
        // kmdf's status after freeing context header memory.
        //
        if (contextHeader != NULL) {
            FxPoolFree(contextHeader);
        }
    }

    return status;
}

_Must_inspect_result_
PVOID
FASTCALL
VfWdfObjectGetTypedContext(
    __in
    WDFOBJECT Handle,
    __in
    PCWDF_OBJECT_CONTEXT_TYPE_INFO TypeInfo
    )
/*++

Routine Description:
    Retrieves the requested type from a handle

Arguments:
    Handle - the handle to retrieve the context from
    TypeInfo - global constant pointer which describes the type.  Since the pointer
        value is unique in all of kernel space, we will perform a pointer compare
        instead of a deep structure compare

Return Value:
    A valid context pointere or NULL.  NULL is not a failure, querying for a type
    not associated with the handle is a legitimate operation.

  --*/
{
    FxContextHeader* pHeader;
    FxObject* pObject;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDFOBJECT_OFFSET offset;

    PAGED_CODE_LOCKED();

    //
    // Do not call FxObjectHandleGetPtr( , , FX_TYPE_OBJECT) because this is a
    // hot spot / workhorse function that should be as efficient as possible.
    //
    // A call to FxObjectHandleGetPtr would :
    // 1)  invoke a virtual call to QueryInterface
    //
    // 2)  ASSERT that the ref count of the object is > zero.  Since this is one
    //     of the few functions that can be called in EvtObjectDestroy where the
    //     ref count is zero, that is not a good side affect.
    //
    offset = 0;
    pObject = FxObject::_GetObjectFromHandle(Handle, &offset);

    //
    // Use the object's globals, not the caller's
    //
    pFxDriverGlobals = pObject->GetDriverGlobals();

    FxPointerNotNull(pFxDriverGlobals, Handle);
    FxPointerNotNull(pFxDriverGlobals, TypeInfo);

    pHeader = pObject->GetContextHeader();

    for ( ; pHeader != NULL; pHeader = pHeader->NextHeader) {
        if (pHeader->ContextTypeInfo == TypeInfo) {
            return &pHeader->Context[0];
        }
    }

    PCHAR pGivenName;

    if (TypeInfo->ContextName != NULL) {
        pGivenName = TypeInfo->ContextName;
    }
    else {
        pGivenName = "<no typename given>";
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGDEVICE,
                        "Attempting to get context type %s from WDFOBJECT 0x%p",
                        pGivenName, Handle);

    return NULL;
}

_Must_inspect_result_
NTSTATUS
VfAllocateContext(
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __out PVOID* ContextHeader
    )
/*++

Routine Description:
    Allocates an additional context on the handle if the object is in the
    correct state

Arguments:
    Handle - handle on which to add a context
    Attributes - attributes which describe the type and size of the new context
    Context - optional pointer which will recieve the new context

Return Value:
    STATUS_SUCCESS upon success, STATUS_OBJECT_NAME_EXISTS if the context type
    is already attached to the handle, and !NT_SUCCESS on failure

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxContextHeader *pHeader;
    size_t size;

    PAGED_CODE_LOCKED();

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    status = FxValidateObjectAttributes(
        pFxDriverGlobals, Attributes, FX_VALIDATE_OPTION_ATTRIBUTES_REQUIRED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Must have a context type!
    //
    if (Attributes->ContextTypeInfo == NULL) {
        status = STATUS_OBJECT_NAME_INVALID;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGHANDLE,
            "Attributes %p ContextTypeInfo is NULL, %!STATUS!",
            Attributes, status);
        return status;
    }

    //
    // By passing 0's for raw object size and extra size, we can compute the
    // size of only the header and its contents.
    //
    status = FxCalculateObjectTotalSize(pFxDriverGlobals, 0, 0, Attributes, &size);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pHeader = (FxContextHeader*)
         FxPoolAllocate(pFxDriverGlobals, NonPagedPool, size);

    if (pHeader != NULL) {
        *ContextHeader = pHeader;
        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
VfAddContextToHandle(
    __in PVOID ContextHeader,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in WDFOBJECT Handle,
    __out_opt PVOID* Context
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status = STATUS_SUCCESS;
    FxObject* pObject;
    FxContextHeader *pHeader;
    WDFOBJECT_OFFSET offset;

    PAGED_CODE_LOCKED();

    pHeader = (FxContextHeader *)ContextHeader;

    if (pHeader == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // No need to call FxObjectHandleGetPtr( , , FX_TYPE_OBJECT) because
    // we assume that the object handle will point back to an FxObject.  (The
    // call to FxObjectHandleGetPtr will just make needless virtual call into
    // FxObject anyways).
    //
    offset = 0;
    pObject = FxObject::_GetObjectFromHandle(Handle, &offset);

    pFxDriverGlobals = pObject->GetDriverGlobals();

    if (offset != 0) {
        //
        // for lack of a better error code
        //
        status = STATUS_OBJECT_PATH_INVALID;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGHANDLE,
            "WDFHANDLE %p cannot have contexts added to it, %!STATUS!",
            Handle, status);

        goto cleanup;
    }

    pObject->ADDREF(&status);

    FxContextHeaderInit(pHeader, pObject, Attributes);

    status = pObject->AddContext(pHeader, Context, Attributes);

    //
    // STATUS_OBJECT_NAME_EXISTS will not fail NT_SUCCESS, so check
    // explicitly for STATUS_SUCCESS.
    //
    if (status != STATUS_SUCCESS) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGHANDLE,
            "WDFHANDLE %p failed to add context, %!STATUS!",
            Handle, status);
    }

    pObject->RELEASE(&status);

cleanup:

    if (status != STATUS_SUCCESS && pHeader != NULL) {
        FxPoolFree(pHeader);
    }

    return status;
}

} // extern "C"
