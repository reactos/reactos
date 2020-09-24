
/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSyncRequest.hpp

Abstract:

    FxSyncRequest is meant to be a completely stack based structure.  This
    allows synchronous functions to not have to allocate an FxRequest for
    something that only lives for the lifetime of the function call.
    Additionally, this object can substitute a WDFREQUEST for itself when making
    synchronous calls.  This allows the driver writer to pass a WDFREQUEST to a
    synchronous DDI and be able to cancel it on another thread later.

    To overcome the initial reference count that is associated upon request, the
    destructor releases the initial reference and SelfDestruct does nothing
    because there is no memory to free.

    FxSyncRequest derives from FxRequestBase as protected so that it cannot be
    used as a FxRequestBase directly.  Instead, m_TrueRequest should be used.
    m_TrueRequest is either this object itself or the WDFREQUEST, as set by
    SetRequestHandle.

Author:



Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXSYNCREQUEST_H_
#define _FXSYNCREQUEST_H_

class DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) FxSyncRequest : protected FxRequestBase {

public:
    // Create a sync request that allocates its own PIRP
    FxSyncRequest(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in_opt FxRequestContext* Context,
        __in_opt WDFREQUEST Request = NULL
        );

    ~FxSyncRequest();

    //
    // FxObject overrides
    //
    VOID
    SelfDestruct(
        VOID
        );

protected:
     PVOID
     operator new(
         __in size_t Size
         )
     {
         UNREFERENCED_PARAMETER(Size);

         ASSERTMSG("FxSyncRequest::operator new called, should only be"
                      " declared on the stack\n", FALSE);

         return NULL;
     }

public:

    NTSTATUS
    Initialize(
        VOID
        )
    {
        NTSTATUS status;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
        //
        // FxCrEvent initialization can fail in UMDF so check for status.
        //
        status = m_DestroyedEvent.Initialize();
        if (!NT_SUCCESS(status)) {
            return status;
        }
#else
        UNREFERENCED_PARAMETER(status);
        DO_NOTHING();
#endif
        return STATUS_SUCCESS;
    }

    //
    // Since this object can be sitting on a list which is access by another
    // thread and that thread will expect lifetime semantics from AddRef and
    // Release, we need to hold up destruction of the object until all
    // references are released.  This event will be set when the last reference
    // is dropped.
    //
    FxCREvent m_DestroyedEvent;

    //
    // By default, this will point to this object.  If AssignRequestHandle is
    // called, it will point to the underlying object for that handle.  Since
    // this object derives from FxRequestBase as protected, this field is how
    // the object is used as an FxRequestBase* pointer.
    //
    FxRequestBase* m_TrueRequest;

    BOOLEAN m_ClearContextOnDestroy;
};

#endif _FXSYNCREQUEST_H_
