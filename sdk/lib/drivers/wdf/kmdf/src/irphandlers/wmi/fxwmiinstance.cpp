/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWmiInstance.cpp

Abstract:

    This module implements the FxWmiInstance object and its derivations

Author:



Revision History:


--*/

#include "fxwmipch.hpp"

extern "C" {
// #include "FxWmiInstance.tmh"
}

FxWmiInstance::FxWmiInstance(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize,
    __in FxWmiProvider* Provider
    ) :
    FxNonPagedObject(FX_TYPE_WMI_INSTANCE, ObjectSize, FxDriverGlobals)
{
    InitializeListHead(&m_ListEntry);
    m_Provider = Provider;
    m_Provider->ADDREF(this);
    MarkDisposeOverride(ObjectDoNotLock);
}

FxWmiInstance::~FxWmiInstance()
{
    ASSERT(IsListEmpty(&m_ListEntry));
}

BOOLEAN
FxWmiInstance::Dispose(
    VOID
    )
{
    m_Provider->RemoveInstance(this);
    m_Provider->RELEASE(this);

    //
    // Object is being deleted, remove this object from the provider's list
    // of instances.  If we don't do this, the provider will have a list which
    // contains entries which have been freed.
    //
    return FxNonPagedObject::Dispose(); // __super call
}

_Must_inspect_result_
NTSTATUS
FxWmiInstance::FireEvent(
    __in_bcount_opt(EventBufferSize) PVOID EventBuffer,
    __inout ULONG EventBufferSize
    )
{
    ULONG sizeNeeded;
    PWNODE_SINGLE_INSTANCE pNode;
    NTSTATUS status;

    if (EventBuffer == NULL) {
        EventBufferSize = 0;
    }

    sizeNeeded = sizeof(WNODE_SINGLE_INSTANCE) + EventBufferSize;

    //
    // IoWMIWriteEvent will free the memory by calling ExFreePool. This means
    // we cannot use a framework allocate function.
    //
    pNode = (PWNODE_SINGLE_INSTANCE)
        ExAllocatePoolWithTag(NonPagedPool, sizeNeeded, GetDriverGlobals()->Tag);

    if (pNode != NULL) {
        RtlCopyMemory(&pNode->WnodeHeader.Guid,
                      m_Provider->GetGUID(),
                      sizeof(GUID));

        pNode->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(
            GetDevice()->GetDeviceObject());
        pNode->WnodeHeader.BufferSize = sizeNeeded;
        pNode->WnodeHeader.Flags =  WNODE_FLAG_SINGLE_INSTANCE |
                                    WNODE_FLAG_EVENT_ITEM |
                                    WNODE_FLAG_STATIC_INSTANCE_NAMES;
        KeQuerySystemTime(&pNode->WnodeHeader.TimeStamp);

        pNode->InstanceIndex = m_Provider->GetInstanceIndex(this);
        pNode->SizeDataBlock = EventBufferSize;
        pNode->DataBlockOffset = sizeof(WNODE_SINGLE_INSTANCE);

        if (EventBuffer != NULL) {
            RtlCopyMemory(&pNode->VariableData, EventBuffer, EventBufferSize);
        }

        //
        // Upon success, IoWMIWriteEvent will free pNode.
        //
        status = IoWMIWriteEvent(pNode);

        if (!NT_SUCCESS(status)) {
            ExFreePool(pNode);
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFWMIINSTANCE %p insufficient resources to fire event,%!STATUS!",
            GetHandle(), status);
    }

    return status;
}

FxWmiInstanceExternal::FxWmiInstanceExternal(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_WMI_INSTANCE_CONFIG Config,
    __in FxWmiProvider* Provider
    ) :
    FxWmiInstance(FxDriverGlobals, sizeof(FxWmiInstanceExternal), Provider),
    m_QueryInstanceCallback(FxDriverGlobals),
    m_SetInstanceCallback(FxDriverGlobals),
    m_SetItemCallback(FxDriverGlobals),
    m_ExecuteMethodCallback(FxDriverGlobals)
{
    m_ContextLength = 0;
    m_UseContextForQuery = Config->UseContextForQuery;

    if (m_UseContextForQuery == FALSE) {
        m_QueryInstanceCallback.m_Method = Config->EvtWmiInstanceQueryInstance;
    }
    m_SetInstanceCallback.m_Method = Config->EvtWmiInstanceSetInstance;
    m_SetItemCallback.m_Method = Config->EvtWmiInstanceSetItem;

    m_ExecuteMethodCallback.m_Method = Config->EvtWmiInstanceExecuteMethod;
}

_Must_inspect_result_
NTSTATUS
FxWmiInstanceExternal::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxWmiProvider* Provider,
    __in PWDF_WMI_INSTANCE_CONFIG WmiInstanceConfig,
    __in_opt PWDF_OBJECT_ATTRIBUTES InstanceAttributes,
    __out WDFWMIINSTANCE* WmiInstance,
    __out FxWmiInstanceExternal** Instance
    )
{
    FxWmiInstanceExternal* pInstance;
    WDFWMIINSTANCE hInstance;
    NTSTATUS status;
    size_t contextSize;

    contextSize = 0;
    *Instance = 0;

    *WmiInstance = NULL;

    //
    // For event only providers, you cannot specify any callbacks or context
    // usage.
    //
    if (Provider->IsEventOnly() &&
        (WmiInstanceConfig->UseContextForQuery ||
         WmiInstanceConfig->EvtWmiInstanceQueryInstance != NULL ||
         WmiInstanceConfig->EvtWmiInstanceSetInstance != NULL ||
         WmiInstanceConfig->EvtWmiInstanceSetItem != NULL ||
         WmiInstanceConfig->EvtWmiInstanceExecuteMethod != NULL)) {

        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFWMIPROVIDER %p is event only and UseContextForQuery (%d) is TRUE,"
            " or a callback (query instance %p, set instance %p, set item %p, "
            "executue method %p) is not NULL, %!STATUS!",
            Provider->GetHandle(), WmiInstanceConfig->UseContextForQuery,
            WmiInstanceConfig->EvtWmiInstanceQueryInstance,
            WmiInstanceConfig->EvtWmiInstanceSetInstance,
            WmiInstanceConfig->EvtWmiInstanceSetItem,
            WmiInstanceConfig->EvtWmiInstanceExecuteMethod, status);

        return status;
    }

    status = FxValidateObjectAttributes(FxDriverGlobals,
                                        InstanceAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (WmiInstanceConfig->UseContextForQuery) {
        //
        // UseContextForQuery only supported for read only instances.
        // ExecuteMethod has undefined side affects, so we allow it.
        //
        if (WmiInstanceConfig->EvtWmiInstanceSetInstance != NULL ||
            WmiInstanceConfig->EvtWmiInstanceSetItem != NULL) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "UseContextForQuery set, i.e. a read only instance, but "
                "EvtWmiInstanceSetInstance %p or EvtWmiInstanceSetItem %p is "
                "set, %!STATUS!",
                WmiInstanceConfig->EvtWmiInstanceSetInstance,
                WmiInstanceConfig->EvtWmiInstanceSetItem, status);

            return status;
        }

        //
        // We must have a context to use for the query
        //
        if (InstanceAttributes == NULL ||
            InstanceAttributes->ContextTypeInfo == NULL) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "UseContextForQuery set, but InstanceAttributes %p is null or "
                "there is no associated type, %!STATUS!",
                InstanceAttributes, status);

            return status;
        }

        contextSize = InstanceAttributes->ContextTypeInfo->ContextSize;

        if (InstanceAttributes->ContextSizeOverride != 0) {
            status = RtlSizeTAdd(contextSize,
                                 InstanceAttributes->ContextSizeOverride,
                                 &contextSize);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Overlfow adding contextSize %I64d with size override %I64d, "
                    "%!STATUS!", contextSize,
                    InstanceAttributes->ContextSizeOverride, status);

                return status;
            }
        }

        if (contextSize > ULONG_MAX) {
            //
            // Since we are casting to a ULONG below, detect loss of data here
            // (only really applicable on 64 bit machines where sizeof(size_t) !=
            // sizeof(ULONG)
            //
            status = STATUS_INTEGER_OVERFLOW;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "context size %I64d can be %d large, %!STATUS!",
                contextSize, ULONG_MAX, status);

            return status;
        }

        //
        // Make sure the context is the minimum the buffer size.
        //
        if (contextSize < Provider->GetMinInstanceBufferSize()) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "context size %I64d is less then the WDFWMIPROVIDER %p min size "
                "of %d, %!STATUS!",
                contextSize, Provider->GetHandle(),
                Provider->GetMinInstanceBufferSize(), status);

            return status;
        }
    }

    pInstance = new(FxDriverGlobals, InstanceAttributes)
        FxWmiInstanceExternal(FxDriverGlobals, WmiInstanceConfig, Provider);

    if (pInstance == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "could not allocate memory for WDFWMIINSTANCE, %!STATUS!",
            status);

        return status;
    }

    if (contextSize > 0) {
        pInstance->SetContextForQueryLength((ULONG) contextSize);
    }

    if (NT_SUCCESS(status)) {
        status = pInstance->Commit(
            InstanceAttributes, (PWDFOBJECT) &hInstance, Provider);

        if (NT_SUCCESS(status)) {
            //
            // Assign the handle back to the caller.
            //
            *WmiInstance = hInstance;
        }
        else {
            //
            // On failure, DeleteFromFailedCreate will delete the object and
            // the Dispose callback will remove the instance from the provider's
            // list.
            //
            DO_NOTHING();
        }
    }

    if (NT_SUCCESS(status)) {
        *Instance = pInstance;
    }
    else {
        pInstance->DeleteFromFailedCreate();
    }

    return status;
}

BOOLEAN
FxWmiInstanceExternal::IsQueryInstanceSupported(
    VOID
    )
{
    //
    // If we have a function pointer to call or we are using the context
    // as the buffer, query instance is supported.
    //
    // Also, if neither of the first 2 are true, we need to support query
    // instance if the device has an execute method callback b/c WMI will
    // send a query instance to this instance which much succeed for the
    // execute method irp to be sent.
    //
    return (m_UseContextForQuery ||
            m_QueryInstanceCallback.m_Method != NULL ||
            m_ExecuteMethodCallback.m_Method != NULL) ? TRUE
                                                      : FALSE;
}

_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxWmiInstanceExternal::QueryInstance(
    __inout ULONG OutBufferSize,
    __out_bcount(OutBufferSize) PVOID OutBuffer,
    __out PULONG BufferUsed
    )
{
    NTSTATUS status;

    if (m_UseContextForQuery) {
        //
        // No matter what, we are reporting the length of the context.  If the
        // buffer is too small, it is used to report the desired buffer length.
        // Otherwise, it is the amount of data we copied to the query buffer.
        //
        *BufferUsed = m_ContextLength;

        if (OutBufferSize < m_ContextLength) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFWMIINSTANCE %p query instance using context for query, "
                "query buffer length %d, context length %d, %!STATUS!",
                GetHandle(), OutBufferSize, m_ContextLength, status);
        }
        else {
            status = STATUS_SUCCESS;

            RtlCopyMemory(OutBuffer,
                          &GetContextHeader()->Context[0],
                          m_ContextLength);
        }
    }
    else if (m_QueryInstanceCallback.m_Method != NULL) {
        BYTE dummy;

        if (OutBufferSize == 0) {
            ASSERT(m_Provider->GetMinInstanceBufferSize() == 0);
            OutBuffer = (PVOID) &dummy;
            OutBufferSize = sizeof(dummy);
        }

        status = m_QueryInstanceCallback.Invoke(
            GetDevice()->GetHandle(),
            GetHandle(),
            OutBufferSize,
            OutBuffer,
            BufferUsed
            );

        if (status == STATUS_PENDING) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFWMIINSTANCE %p was queried and returned %!STATUS!, which is "
                "not an allowed return value", GetHandle(), status);

            FxVerifierDbgBreakPoint(GetDriverGlobals());

            status = STATUS_UNSUCCESSFUL;
            *BufferUsed = 0;
        }
        else if (NT_SUCCESS(status)) {
            if (*BufferUsed > OutBufferSize) {
                //
                // Caller error, they returned more bytes in *BufferUsed then
                // was passed in via OutBufferSize, yet returned NT_SUCCESS
                //
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "WDFWMIINSTANCE %p was queried with buffer size %d, "
                    " but returned %d bytes and %!STATUS!, should return "
                    "!NT_SUCCESS in this case",
                    GetHandle(), OutBufferSize, *BufferUsed, status);

                FxVerifierDbgBreakPoint(GetDriverGlobals());

                status = STATUS_UNSUCCESSFUL;
                *BufferUsed = 0;
            }
            else if (OutBuffer == &dummy && *BufferUsed > 0) {
                //
                // Convert success back to an error where we can report the
                // required size back to the caller.
                //
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else if (status == STATUS_BUFFER_TOO_SMALL) {
            if (m_Provider->GetMinInstanceBufferSize()) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "WDFWMIINSTANCE %p returned %!STATUS!, but it specified "
                    "a minimum instance size %d in its WDFWMIPROVIDER %p",
                    GetHandle(), status, m_Provider->GetMinInstanceBufferSize(),
                    m_Provider->GetHandle());
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                    "This is a break in the contract.  Minimum instance size "
                    "should only be used for fixed sized instances");

                FxVerifierDbgBreakPoint(GetDriverGlobals());
            }
        }
    }
    else {
        ASSERT(m_ExecuteMethodCallback.m_Method != NULL);

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFWMIINSTANCE %p was queried with no query callback and supports "
            "execute method (%p), zero bytes returned", GetHandle(),
            m_ExecuteMethodCallback.m_Method);

        status = STATUS_SUCCESS;
        *BufferUsed = 0;
    }

    return status;
}

BOOLEAN
FxWmiInstanceExternal::IsSetInstanceSupported(
    VOID
    )
{
    return m_SetInstanceCallback.m_Method != NULL ? TRUE : FALSE;
}

_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxWmiInstanceExternal::SetInstance(
    __in ULONG InBufferSize,
    __in_bcount(InBufferSize) PVOID InBuffer
    )
{
    return m_SetInstanceCallback.Invoke(
        GetDevice()->GetHandle(),
        GetHandle(),
        InBufferSize,
        InBuffer
        );
}

BOOLEAN
FxWmiInstanceExternal::IsSetItemSupported(
    VOID
    )
{
    return m_SetItemCallback.m_Method != NULL ? TRUE : FALSE;
}

_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxWmiInstanceExternal::SetItem(
    __in ULONG DataItemId,
    __in ULONG InBufferSize,
    __in_bcount(InBufferSize) PVOID InBuffer
    )
{
    return m_SetItemCallback.Invoke(
        GetDevice()->GetHandle(),
        GetHandle(),
        DataItemId,
        InBufferSize,
        InBuffer
        );
}

BOOLEAN
FxWmiInstanceExternal::IsExecuteMethodSupported(
    VOID
    )
{
    return m_ExecuteMethodCallback.m_Method != NULL ? TRUE : FALSE;
}

_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxWmiInstanceExternal::ExecuteMethod(
    __in ULONG MethodId,
    __in ULONG InBufferSize,
    __inout ULONG OutBufferSize,
    __drv_when(InBufferSize >= OutBufferSize, __inout_bcount(InBufferSize))
    __drv_when(InBufferSize < OutBufferSize, __inout_bcount(OutBufferSize))
        PVOID Buffer,
    __out PULONG BufferUsed
    )
{
    return m_ExecuteMethodCallback.Invoke(
        GetDevice()->GetHandle(),
        GetHandle(),
        MethodId,
        InBufferSize,
        OutBufferSize,
        Buffer,
        BufferUsed
        );
}

FxWmiInstanceInternal::FxWmiInstanceInternal(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxWmiInstanceInternalCallbacks* Callbacks,
    __in FxWmiProvider* Provider
    ) : FxWmiInstance(FxDriverGlobals, sizeof(FxWmiInstanceInternal), Provider)
{
    m_QueryInstance = Callbacks->QueryInstance;
    m_SetInstance = Callbacks->SetInstance;
    m_SetItem = Callbacks->SetItem;
    m_ExecuteMethod = Callbacks->ExecuteMethod;
}

BOOLEAN
FxWmiInstanceInternal::IsQueryInstanceSupported(
    VOID
    )
{
    return m_QueryInstance != NULL ? TRUE : FALSE;
}

_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxWmiInstanceInternal::QueryInstance(
    __inout ULONG OutBufferSize,
    __out_bcount(OutBufferSize) PVOID OutBuffer,
    __out PULONG BufferUsed
    )
{
    return m_QueryInstance(GetDevice(),
                           this,
                           OutBufferSize,
                           OutBuffer,
                           BufferUsed);
}

BOOLEAN
FxWmiInstanceInternal::IsSetInstanceSupported(
    VOID
    )
{
    return m_SetInstance != NULL ? TRUE : FALSE;
}

_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxWmiInstanceInternal::SetInstance(
    __in ULONG InBufferSize,
    __in_bcount(InBufferSize) PVOID InBuffer
    )
{
    return m_SetInstance(GetDevice(),
                         this,
                         InBufferSize,
                         InBuffer);
}

BOOLEAN
FxWmiInstanceInternal::IsSetItemSupported(
    VOID
    )
{
    return m_SetItem != NULL ? TRUE : FALSE;
}

_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxWmiInstanceInternal::SetItem(
    __in ULONG DataItemId,
    __in ULONG InBufferSize,
    __in_bcount(InBufferSize) PVOID InBuffer
    )
{
    return m_SetItem(GetDevice(),
                     this,
                     DataItemId,
                     InBufferSize,
                     InBuffer);
}

BOOLEAN
FxWmiInstanceInternal::IsExecuteMethodSupported(
    VOID
    )

{
    return m_ExecuteMethod != NULL ? TRUE : FALSE;
}

_Must_inspect_result_
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxWmiInstanceInternal::ExecuteMethod(
    __in ULONG MethodId,
    __in ULONG InBufferSize,
    __inout ULONG OutBufferSize,
    __drv_when(InBufferSize >= OutBufferSize, __inout_bcount(InBufferSize))
    __drv_when(InBufferSize < OutBufferSize, __inout_bcount(OutBufferSize))
        PVOID Buffer,
    __out PULONG BufferUsed
    )
{
    return m_ExecuteMethod(GetDevice(),
                           this,
                           MethodId,
                           InBufferSize,
                           OutBufferSize,
                           Buffer,
                           BufferUsed);
}
