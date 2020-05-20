/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Request object implementation
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */



#include "common/fxrequestbase.h"
#include "common/fxglobals.h"
#include "common/fxiotarget.h"
#include "common/fxmdl.h"


FxRequestBase::FxRequestBase(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize,
    __in_opt MdIrp Irp,
    __in FxRequestIrpOwnership Ownership,
    __in FxRequestConstructorCaller Caller,
    __in FxObjectType ObjectType
    ) : FxNonPagedObject(FX_TYPE_REQUEST, ObjectSize, FxDriverGlobals, ObjectType),
        m_Irp(Irp)
{
    //
    // By default requests cannot be completed except for request associated with
    // IRP allocated from I/O (upper drivers).
    //
    m_CanComplete = FALSE;

    //
    // After is all said and done with assigning to m_IrpAllocation value, if
    // m_Irp().GetIrp  == NULL, then m_IrpAllocation can be overridden in
    // ValidateTarget.
    //
    if (Caller == FxRequestConstructorCallerIsDriver)
    {
        if (Ownership == FxRequestOwnsIrp)
        {
            //
            // Driver writer gave the irp to the framework but still owns it
            // or there is no irp passed in when the FxRequest was created.
            //
            m_IrpAllocation = REQUEST_ALLOCATED_INTERNAL;
        }
        else
        {
            //
            // Driver writer gave the irp to the framework but still owns it
            //
            m_IrpAllocation = REQUEST_ALLOCATED_DRIVER;
        }

        //
        // Cleanup request's context in Dispose. Please see Dispose below
        // for specific scenario we are trying to fix.  Enable Dispose only if 
        // driver is v1.11 or above b/c it may be possible that older driver 
        // may have used invalid/bad scenarios similar to this one:
        // - create target object.
        // - create one or more driver created requests parented to the target.
        // - send one or more of these requests to the target (lower stack).
        // - delete the target object while requests are pending.
        // Deleting a target while it has a pending request is a valid 
        // operation, what is not valid is for these request to be also 
        // parented to the target at the same time. 
        // In this scenario if we turn on Dispose, the Dispose callback will
        // be called before the request is completed.
        // Note that if the driver had specified any Cleanup callbacks on 
        // these requests, these also are called before the requests are 
        // completed.
        //
        if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1, 11)) {
            MarkDisposeOverride();
        }
    }
    else if (Ownership == FxRequestOwnsIrp)
    {
        //
        // The request will own the irp and free it when the request is freed
        //
        m_IrpAllocation = REQUEST_ALLOCATED_INTERNAL;
    }
    else
    {
        //
        // Request is owned by the io queue
        //
        m_IrpAllocation = REQUEST_ALLOCATED_FROM_IO;
        m_CanComplete = TRUE;
    }
    
    m_Target = NULL;
    m_TargetFlags = 0;

    m_TargetCompletionContext = NULL;

    m_Completed = m_Irp.GetIrp() ? FALSE : TRUE;
    m_Canceled = FALSE;

    m_PriorityBoost = 0;

    m_RequestContext = NULL;
    m_Timer = NULL;

    InitializeListHead(&m_ListEntry);
    m_DrainSingleEntry.Next = NULL;

    m_IrpReferenceCount = 0;

    m_IrpQueue = NULL;

    m_SystemBufferOffset = 0;
    m_OutputBufferOffset = 0;
    m_IrpCompletionReferenceCount = 0;

    m_AllocatedMdl = NULL;

    m_VerifierFlags = 0;
    m_RequestBaseFlags = 0;
    m_RequestBaseStaticFlags = 0x0;
    m_CompletionState = FxRequestCompletionStateNone;
}

FxRequestBase::~FxRequestBase(
    VOID
    )
{
    MdIrp irp;

    //
    // Since m_ListEntry is a union with the CSQ context and the irp have just
    // come off of a CSQ, we cannot be guaranteed that the m_ListEntry is
    // initialized to point to itself.
    //
    // ASSERT(IsListEmpty(&m_ListEntry));


#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    //
    // If an MDL is associated with the request, free it
    //
    if (m_AllocatedMdl != NULL)
    {
        FxMdlFree(GetDriverGlobals(), m_AllocatedMdl);
    }
#endif

    irp = m_Irp.GetIrp();

    //
    // If the request was created through WdfRequestCreate, formatted, and not
    // reused, we can still have a request context.
    //
    if (m_RequestContext != NULL)
    {
        if (irp != NULL)
        {
            m_RequestContext->ReleaseAndRestore(this);
        }

        delete m_RequestContext;
    }

    if (irp != NULL && m_IrpAllocation == REQUEST_ALLOCATED_INTERNAL)
    {
        m_Irp.FreeIrp();
    }

    if (m_Timer != NULL)
    {
        delete m_Timer;
    }
}

VOID
FxRequestBase::CompleteSubmitted(
    VOID
    )
/*++

Routine Description:
    Routine that handles the setting up of the request packet for being passed
    back to the completion routine.  This includes copying over parameters from
    the PIRP and any dereferences necessary.

Arguments:
    None.

Return Value:
    None.

  --*/
{
    FxIoTarget* pTarget;

    pTarget = m_Target;
    
    FX_TRACK_DRIVER(GetDriverGlobals());

    if (GetDriverGlobals()->FxVerifierOn)
    {
        //
        // Zero out any values previous driver may have set; when completing the irp
        // through FxRequest::CompleteInternal, we check to see what the lastest
        // package was (stored off in the DriverContext).  Since the request was
        // sent off to another devobj, don't assume any valid values in the
        // DriverContext anymore.
        //
        ZeroOutDriverContext();

        //
        // ClearFormatted also checks for WdfVefiefierOn, but that's OK
        //
        VerifierClearFormatted();
    }

    if (m_RequestContext != NULL)
    {
        //
        // Always do the copy because the driver can retrieve the parameters
        // later even if there is no completion routine set.
        //
        GetSubmitFxIrp()->CopyStatus(
                    &m_RequestContext->m_CompletionParams.IoStatus
                    );
        
        m_RequestContext->CopyParameters(this);

        //
        // Call the completion routine if present.  Once we call the completion
        // routine we can't touch any fields anymore since the request may be resent
        // down the stack.
        //
        if (m_CompletionRoutine.m_Completion != NULL)
        {
            ClearCompletionRoutine()(GetHandle(),
                                     pTarget->GetHandle(),
                                     &m_RequestContext->m_CompletionParams,
                                     ClearCompletionContext());
        }
    }
    else if (m_CompletionRoutine.m_Completion != NULL)
    {
        //
        // Only put a completion parameters struct on the stack if we have to.
        // By putting it into a separate function, we can control stack usage
        // in this way.
        //
        CompleteSubmittedNoContext();
    }

    //
    // Release the tag that was acquired when the request was submitted or
    // pended.
    //
    RELEASE(pTarget);
}

_Must_inspect_result_
BOOLEAN
FxRequestBase::CancelTimer(
    VOID
    )
/*++

Routine Description:
    Cancel a previously queued timer based on this request if one was set.

Arguments:
    None

Assumes:
    Caller is providing synchronization around the call of this function with
    regard to m_TargetFlags.

Return Value:
    TRUE if the timer was cancelled successfully or if there was no timer set,
    otherwise FALSE if the timer was not cancelled and has fired.

  --*/
{
    if (m_TargetFlags & FX_REQUEST_TIMER_SET)
    {
        //
        // If we can successfully cancel the timer, release the reference
        // taken in StartTimer and mark the timer as not queued.
        //

        if (m_Timer->Timer.Stop() == FALSE)
        {

            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Request %p, did not cancel timer", this);

            //
            // Leave FX_REQUEST_TIMER_SET set.  The timer DPC will clear the it
            //

            return FALSE;
        }

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Request %p, canceled timer successfully", this);

        m_TargetFlags &= ~FX_REQUEST_TIMER_SET;
    }

    return TRUE;
}

VOID
FxRequestBase::CompleteSubmittedNoContext(
    VOID
    )
/*++

Routine Description:
    Invokes the completion routine and uses a completion params that is on the
    stack. This is in a separate function so that we only consume the stack
    space if we need to.

Arguments:
    None

Return Value:
    None

  --*/
{
    WDF_REQUEST_COMPLETION_PARAMS params;

    params.Type = WdfRequestTypeNoFormat;

    GetSubmitFxIrp()->CopyStatus(&params.IoStatus);

    RtlZeroMemory(&params.Parameters, sizeof(params.Parameters));


    //
    // Once we call the completion routine we can't touch any fields anymore
    // since the request may be resent down the stack.
    //
    ClearCompletionRoutine()(GetHandle(),
                             m_Target->GetHandle(),
                             &params,
                             ClearCompletionContext());
}
