/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDpc.hpp

Abstract:

    This module implements a frameworks managed DPC that
    can synchrononize with driver frameworks object locks.

Author:



Environment:

    Kernel mode only

Revision History:


--*/

#include "fxcorepch.hpp"

#include "fxdpc.hpp"

// Tracing support
extern "C" {
// #include "FxDpc.tmh"
}

//
// Public constructors
//

FxDpc::FxDpc(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_DPC, sizeof(FxDpc), FxDriverGlobals)
{
   m_Object = NULL;
   m_CallbackLock = NULL;
   m_CallbackLockObject = NULL;
   m_Callback = NULL;
   m_RunningDown = FALSE;

   //
   // Mark the object has having passive level dispose so that KeFlushQueuedDpcs
   // can be called in Dispose().
   //
   MarkPassiveDispose(ObjectDoNotLock);

   MarkDisposeOverride(ObjectDoNotLock);
}


FxDpc::~FxDpc()
{
    //
    // If this hits, its because someone destroyed the DPC by
    // removing too many references by mistake without calling WdfObjectDelete
    //
    if (m_Object != NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Destroy WDFDPC %p destroyed without calling WdfObjectDelete, or by"
            " Framework processing DeviceRemove.  Possible reference count "
            "problem?", GetObjectHandleUnchecked());
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }
}

_Must_inspect_result_
NTSTATUS
FxDpc::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_DPC_CONFIG Config,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in FxObject* ParentObject,
    __out WDFDPC* Dpc
    )
/*++

Routine Description:

    Create an FxDpc factory method

Arguments:

    All arguments have been valided by the FxDpcApi stub.

Returns:

    NTSTATUS

--*/
{
    FxDpc* pFxDpc;
    NTSTATUS status;

    pFxDpc = new(FxDriverGlobals, Attributes) FxDpc(FxDriverGlobals);

    if (pFxDpc == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pFxDpc->Initialize(
        Attributes,
        Config,
        ParentObject,
        Dpc
        );

    if (!NT_SUCCESS(status)) {
        pFxDpc->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDpc::Initialize(
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in PWDF_DPC_CONFIG Config,
    __in FxObject* ParentObject,
    __out WDFDPC* Dpc
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IFxHasCallbacks* pCallbacks;
    NTSTATUS status;
    KDPC* pDpc;

    pFxDriverGlobals = GetDriverGlobals();
    pDpc = NULL;
    pCallbacks = NULL;

    pDpc = &m_Dpc;

    //
    // Set user's callback function
    //
    m_Callback = Config->EvtDpcFunc;

    //
    // Initialize the DPC to point to our thunk
    //
    KeInitializeDpc(
        pDpc,         // Dpc
        FxDpcThunk,   // DeferredRoutine
        this          // DeferredContext
        );

    //
    // As long as we are associated, the parent object holds a reference
    // count on the DPC.
    //
    // We keep an extra reference count since on Dispose, we wait until
    // all outstanding DPC's complete before allowing finalization.
    //
    // This reference must be taken early before we return any failure,
    // since Dispose() expects this extra reference, and Dispose() will
    // be called even if we return a failure status right now.
    //
    ADDREF(this);

    //
    // DPC's can be parented by, and optionally serialize with an FxDevice or
    // an FxQueue.
    //
    m_DeviceBase = FxDeviceBase::_SearchForDevice(ParentObject, &pCallbacks);

    if (m_DeviceBase == NULL) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Configure Serialization for the DPC and callbacks on the supplied object
    //
    status = _GetEffectiveLock(
        ParentObject,
        pCallbacks,
        Config->AutomaticSerialization,
        FALSE,
        &m_CallbackLock,
        &m_CallbackLockObject
        );

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_WDF_INCOMPATIBLE_EXECUTION_LEVEL) {






            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "ParentObject %p can not automatically synchronize callbacks "
                "with a DPC since it is configured for passive level callback "
                "constraints. Set AutomaticSerialization to FALSE. %!STATUS!",
                Attributes->ParentObject, status);
        }

        return status;
    }

    //
    // We automatically synchronize with and reference count
    // the lifetime of the framework object to prevent any DPC races
    // that can access the object while it is going away.
    //

    //
    // The caller supplied object is the object the caller wants the
    // DPC to be associated with, and the framework must ensure this
    // object remains live until the DPC object is destroyed. Otherwise,
    // it could access either object context memory, or an object API
    // on a freed object.
    //
    // Due to the locking model of the framework, the lock may actually
    // be owned by a higher level object as well. This is the lockObject
    // returned. As long was we are a child of this object, the lockObject
    // does not need to be dereferenced since it will notify us of Cleanup
    // before it goes away.
    //

    //
    // Associate the FxDpc with the object. When this object gets deleted or
    // disposed, it will notify our Dispose function as well.
    //

    //
    // Add a reference to the parent object we are associated with.
    // We will be notified of Cleanup to release this reference.
    //
    ParentObject->ADDREF(this);

    // Save the ptr to the object the DPC is associated with
    m_Object = ParentObject;

    //
    // Attributes->ParentObject is the same as ParentObject.  Since we already
    // converted it to an object, use that.
    //
    status = Commit(Attributes, (WDFOBJECT*)Dpc, ParentObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return status;
}

BOOLEAN
FxDpc::Cancel(
    __in BOOLEAN Wait
    )
{
    BOOLEAN result;

    result = KeRemoveQueueDpc(GetDpcPtr());

    //
    // If result == FALSE, then the DPC could already be running.
    //
    // If the caller supplies Wait == TRUE, they want to wait and
    // ensure on return the DPC has finished running.
    //
    // The trick here is to implement this without adding execessive
    // overhead to the "normal" path, such as tracking reference counts,
    // locking, signaling events to waiting threads, etc.
    //
    // So we take the expensive approach for the Cancel call in the
    // case the caller wants to wait, and misses the DPC window. In
    // this case we will just do the system wide FlushQueuedDpc's to
    // ensure the DPC has finished running before return.
    //
    if( Wait && !result ) {

        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

        KeFlushQueuedDpcs();
    }

    return result;
}

VOID
FxDpc::DpcHandler(
    __in PKDPC Dpc,
    __in PVOID SystemArgument1,
    __in PVOID SystemArgument2
    )
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    FX_TRACK_DRIVER(GetDriverGlobals());

    if (m_Callback != NULL) {

        FxPerfTraceDpc(&m_Callback);

        if (m_CallbackLock != NULL) {
            KIRQL irql = 0;

            m_CallbackLock->Lock(&irql);
            m_Callback((WDFDPC)(this->GetObjectHandle()));
            m_CallbackLock->Unlock(irql);
        }
        else {
           m_Callback((WDFDPC)(this->GetObjectHandle()));
        }
    }
}

VOID
FxDpc::FxDpcThunk(
    __in PKDPC Dpc,
    __in PVOID DeferredContext,
    __in PVOID SystemArgument1,
    __in PVOID SystemArgument2
    )

/*++

Routine Description:

    This is the C routine called by the kernels DPC handler

Arguments:

    Dpc             -   our DPC object associated with our Timer
    DeferredContext -   Context for the DPC that we setup in DriverEntry
    SystemArgument1 -
    SystemArgument2 -

Return Value:

    Nothing.

--*/

{
    FxDpc* pDpc = (FxDpc*)DeferredContext;

    pDpc->DpcHandler(
        Dpc,
        SystemArgument1,
        SystemArgument2
        );

    return;
}

//
// Called when DeleteObject is called, or when the parent
// is being deleted or Disposed.
//
// Also invoked directly by the cleanup list at our request after
// a Dispose occurs and must be deferred if not at passive level.
//
BOOLEAN
FxDpc::Dispose()
{
    // MarkPassiveDispose() in Initialize ensures this
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    m_RunningDown = TRUE;

    FlushAndRundown();

    return TRUE;
}

//
// Called by the system work item to finish the rundown
//
VOID
FxDpc::FlushAndRundown()
{
    FxObject* pObject;

    //
    // If we have the KeFlushQueuedDpcs function call it
    // to ensure the DPC routine is no longer running before
    // we release the final reference and memory to the framework objects
    //
    KeFlushQueuedDpcs();

    //
    // Release our reference count to the associated parent object if present
    //
    if (m_Object != NULL) {
        pObject = m_Object;
        m_Object = NULL;

        pObject->RELEASE(this);
    }

    //
    // Perform our final release to ourselves, destroying the FxDpc
    //
    RELEASE(this);
}
