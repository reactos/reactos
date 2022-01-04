/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxSyncRequest.cpp

Abstract:

    This module implements FxSyncRequest object

Author:

Environment:

    Both kernel and user mode

Revision History:



--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
// #include "FxSyncRequest.tmh"
}

FxSyncRequest::FxSyncRequest(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in_opt FxRequestContext* Context,
    __in_opt WDFREQUEST Request
    ) :
    FxRequestBase(FxDriverGlobals,
                  0, // no handle for this object
                  NULL, // No PIRP
                  FxRequestDoesNotOwnIrp,
                  FxRequestConstructorCallerIsFx,
                  FxObjectTypeEmbedded)
/*++

Routine Description:
    Constructs an FxSyncRequest

Arguments:
    Context - Context to associate with this object

    Request - (opt) real Request object.

Return Value:
    None.

  --*/
{
    //
    // If m_CleanContextOnDestroy is TRUE, m_RequestContext is cleared in the
    // destructor so that the base class destructor does not free the context.
    // This is useful if the context is also stack based.
    //
    if (Context != NULL) {
        m_ClearContextOnDestroy = TRUE;
    }

    else {
        m_ClearContextOnDestroy = FALSE;
    }

    m_RequestContext = Context;

    if (Request == NULL) {
        m_TrueRequest = this;
        m_RequestBaseFlags |= FxRequestBaseSyncCleanupContext;
    }
    else {
        FxRequest* pRequest;

        FxObjectHandleGetPtr(FxDriverGlobals,
                             Request,
                             FX_TYPE_REQUEST,
                             (PVOID*) &pRequest);
        m_TrueRequest = pRequest;

        //
        // pRequest could be currently formatted and the caller has not reused the request
        // in between the format and the sync. send.  This will place pRequest into the
        // correct state.
        //
        if (pRequest->m_RequestContext != NULL) {
            pRequest->m_RequestContext->ReleaseAndRestore(pRequest);
        }
        pRequest->SetContext(Context);
        pRequest->m_RequestBaseFlags |= FxRequestBaseSyncCleanupContext;
    }

    //
    // Indicate that there is no object header so when FxObject::Release is
    // called on the final ref count removal, it doesn't try to touch hyperspace.
    //
    SetNoContextHeader();
}

FxSyncRequest::~FxSyncRequest(
    VOID
    )
/*++

Routine Description:
    Destroys an FxSyncRequest.  Releases the initial reference taken during the
    creation of this object.  If there are any outstanding references to the
    object, the destructor will not exit they are released.

Arguments:
    None.

Return Value:
    None.

  --*/
{
    ULONG count;

    //
    // Release the initial reference taken on create.  Use the base release call
    // so that we don't unnecessarily set the event unless we have to.
    //
    count = FxRequestBase::RELEASE(NULL); // __super call

    //
    // For a driver supplied request(m_TrueRequest) the request context is
    // allocated on the stack so clear it.
    //
    if (m_TrueRequest != this && m_ClearContextOnDestroy) {
       m_TrueRequest->m_RequestContext = NULL;
       m_TrueRequest->m_RequestBaseFlags &= ~FxRequestBaseSyncCleanupContext;
    }

    //
    // Clear the context so that it is not automatically deleted.  Useful
    // if the caller's context is also allocated on the stack and does not
    // need to be freed (as does not have to rememeber to clear the context
    // before this object goes out of scope).
    //
    if (m_ClearContextOnDestroy) {
        m_RequestContext = NULL;
    }

    if (count > 0) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Request %p, waiting on event %p",
                            this, m_DestroyedEvent.GetEvent());

        m_DestroyedEvent.EnterCRAndWaitAndLeave();

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Request %p, wait on event %p done",
                            this, m_DestroyedEvent.GetEvent());
    }
}

VOID
FxSyncRequest::SelfDestruct(
    VOID
    )
/*++

Routine Description:
    Override of base class SelfDestruct.  Since this is a stack based object,
    we must delay the stack caused destruction until all outstanding references
    have been released.  SelfDestruct is called when the last reference has been
    removed from the object.

    Since this is a stack based object, do nothing to free our memory (going out
    of scope will do the trick).

Arguments:
    None.

Return Value:
    None.

  --*/
{
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "SyncRequest %p, signaling event %p on SelfDestruct",
                        this, m_DestroyedEvent.GetEvent());

    m_DestroyedEvent.Set();
}
