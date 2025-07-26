/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power request object implementation base support routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

GENERIC_MAPPING PopPowerRequestGenericMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_ALL
};

POBJECT_TYPE PoPowerRequestObjectType = NULL;
LIST_ENTRY PopPowerRequestsList;
ULONG PopTotalKernelPowerRequestsCount;
ULONG PopTotalUserPowerRequestsCount;
KSPIN_LOCK PopPowerRequestLock;
PKTHREAD PopPowerRequestOwnerLockThread;
KDPC PopScanActivePowerRequestsDpc;
KDPC PopReapTerminatePowerRequestsDpc;
KTIMER PopReapTerminatePowerRequestsTimer;
KTIMER PopScanActivePowerRequestsTiimer;
ULONG PopScanActivePowerRequestsIntervalInSeconds = 60;
ULONG PopReapTerminatePowerRequestsIntervalInSeconds = 300;
BOOLEAN PopReaperTerminateActivated = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static
VOID
PopStartPowerRequestsReaper(VOID)
{
    LARGE_INTEGER PowerRequestReaperDueTime;

    /* Fire up the debug power request monitoring */
    PowerRequestReaperDueTime.QuadPart = Int32x32To64(PopReapTerminatePowerRequestsIntervalInSeconds,
                                                      -10 * 1000 * 1000);
    KeSetTimerEx(&PopReapTerminatePowerRequestsTimer,
                 PowerRequestReaperDueTime,
                 0,
                 &PopReapTerminatePowerRequestsDpc);
}

static
VOID
PopChangeSystemStateProperties(
    _In_ EXECUTION_STATE EsFlags,
    _Inout_ PPOP_POWER_REQUEST *StateHandle)
{
    ULONG UseCount;
    BOOLEAN LegacyRequest;
    EXECUTION_STATE CurrentFlags;
    PPOP_POWER_REQUEST PowerRequest = *StateHandle;

    PAGED_CODE();

    /* The caller should have already held the power request lock */
    POP_ASSERT_POWER_REQUEST_LOCK_THREAD_OWNERSHIP();

    /*
     * We must ensure this is a legacy power request before we
     * actually mess anything with it.
     */
    LegacyRequest = PowerRequest->Legacy;
    ASSERT(LegacyRequest == TRUE);

    /*
     * We are changing the state properties of a registered
     * system state. It is supposed to be used by someone already
     * and if not then this is a bug.
     */
    UseCount = PowerRequest->UseCount;
    ASSERT(UseCount != 0);

    /* Increase the counter of times this power request is used */
    InterlockedIncrementUL(&PowerRequest->UseCount);

    /*
     * Copy the current legacy flags of this power request so we
     * can determine which power settings this request had before.
     */
    CurrentFlags = PowerRequest->LegacyStateFlags | ES_CONTINUOUS;

    /*
     * Check that the caller wants to clear all the state flags,
     * in this case we gotta reset all the counters and clear the
     * current executing flags of this power request.
     */
    if ((EsFlags & ES_CONTINUOUS) &&
        (EsFlags & ES_SYSTEM_REQUIRED) == 0 &&
        (EsFlags & ES_DISPLAY_REQUIRED) == 0 &&
        (EsFlags & ES_USER_PRESENT) == 0)
    {
        InterlockedExchange((PLONG)&PowerRequest->ActiveRequests.ActiveSystemRequiredRequests, 0);
        InterlockedExchange((PLONG)&PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests, 0);
        InterlockedExchange((PLONG)&PowerRequest->LegacyStateFlags, 0);
        return;
    }

    /* Overwrite the current state flags with the newer ones */
    InterlockedExchange((PLONG)&PowerRequest->LegacyStateFlags, EsFlags);

    /*
     * The caller still wants to keep the system busy, does this
     * power request had such a state request before?
     */
    if (EsFlags & ES_SYSTEM_REQUIRED)
    {
        if (CurrentFlags & ES_SYSTEM_REQUIRED)
        {
            /*
             * This power request still preserves this setting, increase
             * the number of times this request keeps the setting.
             */
            InterlockedIncrementUL(&PowerRequest->ActiveRequests.ActiveSystemRequiredRequests);
        }
        else
        {
            /* Decrease the number of times this power request keeps the setting */
            InterlockedDecrementUL(&PowerRequest->ActiveRequests.ActiveSystemRequiredRequests);
        }
    }

    /*
     * The caller still wants to keep the display turned ON, does this
     * power request had such a state request before?
     */
    if (EsFlags & ES_DISPLAY_REQUIRED)
    {
        if (CurrentFlags & ES_DISPLAY_REQUIRED)
        {
            /*
             * This power request still preserves this setting, increase
             * the number of times this request keeps the setting.
             */
            InterlockedIncrementUL(&PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests);
        }
         else
        {
            /* Decrease the number of times this power request keeps the setting */
            InterlockedDecrementUL(&PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests);
        }
    }

    /* The caller still indicates that a user is physically present */
    if (EsFlags & ES_USER_PRESENT)
    {
        if (CurrentFlags & ES_USER_PRESENT)
        {
            /*
             * This power request still preserves this setting, increase
             * the number of times this request keeps the setting.
             */
            InterlockedIncrementUL(&PowerRequest->ActiveRequests.ActiveUserPresentRequests);
        }
         else
        {
            /* Decrease the number of times this power request keeps the setting */
            InterlockedDecrementUL(&PowerRequest->ActiveRequests.ActiveUserPresentRequests);
        }
    }
}

static
NTSTATUS
PopCreatePowerRequestAsLegacy(
    _In_ EXECUTION_STATE EsFlags,
    _In_ BOOLEAN NewRegister,
    _Inout_ PPOP_POWER_REQUEST *Handle)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    KLOCK_QUEUE_HANDLE LockHandle;
    PPOP_POWER_REQUEST PowerRequestObject;

    PAGED_CODE();

    /*
     * The caller instructed us this request is NOT a newly registered
     * power request. Therefore we have to update its power properties.
     */
    if (!NewRegister)
    {
        PopAcquirePowerRequestLock(&LockHandle);
        PopChangeSystemStateProperties(EsFlags, Handle);
        PopReleasePowerRequestLock(&LockHandle);
        return STATUS_SUCCESS;
    }

    /*
     * Before we are going to do anything else, we must
     * ensure the caller has passed ES_CONTINUOUS in the flags
     * as per the documentation. The power settings are persistent
     * with legacy power requests therefore it makes sense.
     */
    ASSERT((EsFlags & ES_CONTINUOUS));

    /* Setup the object attributes for the power request (default settings) */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Register a power request object with Object Manager */
    Status = ObCreateObject(KernelMode,
                            PoPowerRequestObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(POP_POWER_REQUEST),
                            0,
                            0,
                            (PVOID *)&PowerRequestObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the power request object (Status 0x%lx)\n");
        return Status;
    }

    /* Count this power request to the global counter */
    PopTotalKernelPowerRequestsCount++;

    /* Acquire the lock so that we can fill in power properties and initialize it */
    PopAcquirePowerRequestLock(&LockHandle);
    RtlZeroMemory(PowerRequestObject, sizeof(POP_POWER_REQUEST));

    /* Insert this request into the global list */
    InitializeListHead(&PowerRequestObject->Link);
    InsertTailList(&PopPowerRequestsList, &PowerRequestObject->Link);

    /* Initialize the basic power data to defaults */
    PowerRequestObject->DeviceRequestor = NULL;
    PowerRequestObject->UseCount = 1;
    PowerRequestObject->Legacy = TRUE;
    PowerRequestObject->Terminate = FALSE;
    PowerRequestObject->RequestorMode = KernelMode;
    PowerRequestObject->LegacyStateFlags = EsFlags;
    PowerRequestObject->Context = NULL;

    /* Copy the image filename that created this power request */
    RtlCopyMemory(PowerRequestObject->ImageFileName,
                  PsGetCurrentProcess()->ImageFileName,
                  min(sizeof(PowerRequestObject->ImageFileName), sizeof(PsGetCurrentProcess()->ImageFileName)));

    /* Default the active request counters */
    PowerRequestObject->ActiveRequests.ActiveSystemRequiredRequests = 0;
    PowerRequestObject->ActiveRequests.ActiveDisplayRequiredRequests = 0;
    PowerRequestObject->ActiveRequests.ActiveAwayModeRequiredRequests = 0;
    PowerRequestObject->ActiveRequests.ActiveExecutionRequests = 0;

    /*
     * Now check how does the system should behave based upon
     * the desired state executions by the caller. Keep the
     * system busy.
     */
    if (EsFlags & ES_SYSTEM_REQUIRED)
    {
        PowerRequestObject->ActiveRequests.ActiveSystemRequiredRequests = 1;
        PopIndicateSystemStateActivity(ES_SYSTEM_REQUIRED);
    }

    /* Do not dim the display if the caller requests to */
    if (EsFlags & ES_DISPLAY_REQUIRED)
    {
        PowerRequestObject->ActiveRequests.ActiveDisplayRequiredRequests = 1;
        PopIndicateSystemStateActivity(ES_DISPLAY_REQUIRED);
    }

    /* A user is physically present */
    if (EsFlags & ES_USER_PRESENT)
    {
        PowerRequestObject->ActiveRequests.ActiveUserPresentRequests = 1;
        PopIndicateSystemStateActivity(ES_USER_PRESENT);
    }

    /* Give the registered power request object to caller */
    *Handle = PowerRequestObject;
    PopReleasePowerRequestLock(&LockHandle);
    return Status;
}

static
BOOLEAN
PopIsResourceFileNameProvided(
    _In_ PUNICODE_STRING ResourceFileName)
{
    LONG Equal;
    UNICODE_STRING EmptyString;

    /* The resource filename is provided if the buffer is not NULL or empty string */
    RtlInitUnicodeString(&EmptyString, L"");
    if (ResourceFileName->Buffer != NULL)
    {
        Equal = RtlCompareUnicodeString(ResourceFileName, &EmptyString, TRUE);
        if (Equal != 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static
NTSTATUS
PopValidateAndCaptureReasonContext(
    _In_ PCOUNTED_REASON_CONTEXT Context,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ PCOUNTED_REASON_CONTEXT *CapturedContext)
{
    NTSTATUS Status;
    ULONG StringIndex, CapturedStringsIndex;
    PCOUNTED_REASON_CONTEXT ReasonContext;
    COUNTED_REASON_CONTEXT LocalReasonContext;
    UNICODE_STRING CapturedSimpleString, CapturedResFileNameString, CapturedDetailedString;

    /* We MUST have a reason context to validate for */
    ASSERT(Context != NULL);

    /* Is the request coming from user mode? */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /*
             * The request is coming from user mode, we must probe the upcoming
             * context pointer that it can be read and the strings as well.
             */
            ProbeForRead(Context,
                         sizeof(COUNTED_REASON_CONTEXT),
                         sizeof(ULONG));

            /* Probe the strings depending on what the caller provided */
            if (Context->Flags == DIAGNOSTIC_REASON_SIMPLE_STRING)
            {
                /* This is a simple string, just simply probe that */
                ProbeForRead(&Context->SimpleString,
                             sizeof(UNICODE_STRING),
                             sizeof(ULONG));
            }
            else if (Context->Flags == DIAGNOSTIC_REASON_DETAILED_STRING)
            {
                /* We got an array of detailed strings, probe the resource file name if provided */
                if (PopIsResourceFileNameProvided(&Context->ResourceFileName))
                {
                    ProbeForRead(&Context->ResourceFileName,
                                 sizeof(UNICODE_STRING),
                                 sizeof(ULONG));
                }

                /* Probe the array of strings pointers */
                ProbeForRead(Context->ReasonStrings,
                             sizeof(UNICODE_STRING),
                             sizeof(ULONG));
            }
            else
            {
                /*
                 * The caller didn't pass any of the valid reason context flags.
                 * Since we have no idea what does the caller want just bail out.
                 * The code will bail the execution down below.
                 */
                NOTHING;
            }

            /* Cache the reason context locally for later probing */
            LocalReasonContext = *Context;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* The request is coming from kernel-mode, just cache the reason context outright */
        LocalReasonContext = *Context;
    }

    /* Bail out if the context is of unsupported version */
    if (LocalReasonContext.Version != DIAGNOSTIC_REASON_VERSION)
    {
        DPRINT1("Unsupported reason context version (Version %lu)\n", LocalReasonContext.Version);
        return STATUS_INVALID_PARAMETER;
    }

    /* The caller didn't provide any of the valid reason context flags, bail out */
    if (LocalReasonContext.Flags != DIAGNOSTIC_REASON_SIMPLE_STRING &&
        LocalReasonContext.Flags != DIAGNOSTIC_REASON_DETAILED_STRING)
    {
        DPRINT1("WARNING!!! INVALID REASON CONTEXT FLAG PROVIDED (Flag %lx)\n", Context->Flags);
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate the base header space for the reason context */
    ReasonContext = PopAllocatePool(sizeof(*ReasonContext),
                                    TRUE,
                                    TAG_PO_REASON_CONTEXT);
    if (ReasonContext == NULL)
    {
        DPRINT1("Failed to allocate base reason context, not enough memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the context version and flags right away */
    ReasonContext->Version = LocalReasonContext.Version;
    ReasonContext->Flags = LocalReasonContext.Flags;

    /* The caller provided a simple diagnostic string, capture that */
    if (LocalReasonContext.Flags == DIAGNOSTIC_REASON_SIMPLE_STRING)
    {
        /*
         * We have only probed if the string pointer can be read but
         * we still aren't totally sure if the string coming from user
         * mode is safe. Do more probing and capture a copy for us.
         * If the string is from kernel mode we'll be given a copy of it
         * right away.
         */
        Status = ProbeAndCaptureUnicodeString(&CapturedSimpleString,
                                              PreviousMode,
                                              &LocalReasonContext.SimpleString);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to capture the simple string (Status 0x%lx)\n", Status);
            PopFreePool(ReasonContext, TAG_PO_REASON_CONTEXT);
            return Status;
        }

        /* Give the captured simple string to the allocated captured reason context */
        ReasonContext->SimpleString = CapturedSimpleString;

        /* And give the captured context to the caller, we're done here */
        *CapturedContext = ReasonContext;
        return Status;
    }

    /*
     * The caller provided an array of detailed diagnostic strings.
     * Make sure the given context flag matches what the caller desires.
     */
    ASSERT(LocalReasonContext.Flags == DIAGNOSTIC_REASON_DETAILED_STRING);

    /* Capture the resource filename if provided */
    if (PopIsResourceFileNameProvided(&LocalReasonContext.ResourceFileName))
    {
        Status = ProbeAndCaptureUnicodeString(&CapturedResFileNameString,
                                              PreviousMode,
                                              &LocalReasonContext.ResourceFileName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to capture the resource filename string (Status 0x%lx)\n", Status);
            PopFreePool(ReasonContext, TAG_PO_REASON_CONTEXT);
            return Status;
        }

        /* Assign the captured resource filename string and resource ID */
        ReasonContext->ResourceFileName = CapturedResFileNameString;
        ReasonContext->ResourceReasonId = LocalReasonContext.ResourceReasonId;
    }

    /* Copy the count of detailed strings the caller gave to us */
    ReasonContext->StringCount = LocalReasonContext.StringCount;

    /* Now capture every single string from the array we have gotten */
    for (StringIndex = 0; StringIndex < LocalReasonContext.StringCount; StringIndex++)
    {
        Status = ProbeAndCaptureUnicodeString(&CapturedDetailedString,
                                              PreviousMode,
                                              &LocalReasonContext.ReasonStrings[StringIndex]);
        if (!NT_SUCCESS(Status))
        {
            /*
             * Probing and capture failed. Now the burden on us is to figure out
             * how many strings did we probe and capture successfully as we
             * must release them all.
             */
            for (CapturedStringsIndex = StringIndex; CapturedStringsIndex > 0; CapturedStringsIndex--)
            {
                ReleaseCapturedUnicodeString(&ReasonContext->ReasonStrings[CapturedStringsIndex],
                                             PreviousMode);
            }

            /* If we captured the resource filename, release it */
            if (PopIsResourceFileNameProvided(&ReasonContext->ResourceFileName))
            {
                ReleaseCapturedUnicodeString(&ReasonContext->ResourceFileName,
                                             PreviousMode);
            }

            /* And release the allocated reason context */
            DPRINT1("Failed to probe and capture detailed string at index %lu (Status 0x%lx)\n", StringIndex, Status);
            PopFreePool(ReasonContext, TAG_PO_REASON_CONTEXT);
            return Status;
        }

        /* Assign the captured detailed string to the context */
        ReasonContext->ReasonStrings[StringIndex] = CapturedDetailedString;
    }

    /* Give the captured context to the caller we're done here */
    *CapturedContext = ReasonContext;
    return Status;
}

static
VOID
PopReleaseReasonContext(
    _In_ _Post_invalid_ PCOUNTED_REASON_CONTEXT Context,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    ULONG StringIndex;

    /* Release whatever diagnostic string we have captured */
    if (Context->Flags == DIAGNOSTIC_REASON_SIMPLE_STRING)
    {
        ReleaseCapturedUnicodeString(&Context->SimpleString,
                                     PreviousMode);
    }
    else
    {
        for (StringIndex = Context->StringCount; StringIndex > 0; StringIndex--)
        {
            ReleaseCapturedUnicodeString(&Context->ReasonStrings[StringIndex],
                                         PreviousMode);
        }

        if (PopIsResourceFileNameProvided(&Context->ResourceFileName))
        {
            ReleaseCapturedUnicodeString(&Context->ResourceFileName,
                                         PreviousMode);
        }
    }

    /* Free the allocated reason context */
    PopFreePool(Context, TAG_PO_REASON_CONTEXT);
}

static
NTSTATUS
PopCreatePowerRequestObject(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PCOUNTED_REASON_CONTEXT Context,
    _Out_ PPOP_POWER_REQUEST *PowerRequestObject)
{
    NTSTATUS Status;
    ULONG ObjFlags;
    KLOCK_QUEUE_HANDLE LockHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PPOP_POWER_REQUEST PowerRequest;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PCOUNTED_REASON_CONTEXT ReasonContext = NULL;

    PAGED_CODE();

    /* The passed DO must not be NULL, that's illegal */
    ASSERT(DeviceObject != NULL);

    /*
     * We have gotten a diagnostic reason context from the caller,
     * validate it and capture a safe copy of it for the kernel.
     */
    if (Context != NULL)
    {
        Status = PopValidateAndCaptureReasonContext(Context, PreviousMode, &ReasonContext);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("The passed COUNTED_REASON_CONTEXT by the caller is NOT VALID or failed to capture it (Status 0x%lx)\n", Status);
            return Status;
        }
    }

    /* The object must be created in user or kernel mode space accordingly */
    if (PreviousMode == KernelMode)
    {
        ObjFlags = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;
    }
    else
    {
        ObjFlags = OBJ_CASE_INSENSITIVE;
    }

    /* Setup the object attributes for the power request (default settings) */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               ObjFlags,
                               NULL,
                               NULL);

    /* Register a power request object with Object Manager */
    Status = ObCreateObject(PreviousMode,
                            PoPowerRequestObjectType,
                            &ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(POP_POWER_REQUEST),
                            0,
                            0,
                            (PVOID *)&PowerRequest);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the power request object (Status 0x%lx)\n");
        if (ReasonContext != NULL)
        {
            PopReleaseReasonContext(ReasonContext, PreviousMode);
        }

        return Status;
    }

    /* Count this request accordingly from where is this request coming from */
    if (PreviousMode == KernelMode)
    {
        PopTotalKernelPowerRequestsCount++;
    }
    else
    {
        PopTotalUserPowerRequestsCount++;
    }

    /*
     * If this is the first time a power request of this kind is being created
     * then we have to activate the reaper terminate mechanism of which it
     * terminates all of the active power requests but ONLY if the system
     * is AoAc (aka modern standby) and it's being powered by batteries.
     */
    if (IsListEmpty(&PopPowerRequestsList))
    {
        if (PopAoAcPresent && PopDefaultPowerPolicy == &PopDcPowerPolicy)
        {
            PopStartPowerRequestsReaper();
            PopReaperTerminateActivated = TRUE;
        }
    }

    /* Acquire the lock so that we can fill in power properties and initialize it */
    PopAcquirePowerRequestLock(&LockHandle);
    RtlZeroMemory(PowerRequest, sizeof(POP_POWER_REQUEST));

    /* Insert this request into the global list */
    InitializeListHead(&PowerRequest->Link);
    InsertTailList(&PopPowerRequestsList, &PowerRequest->Link);

    /*
     * Initialize the basic power data to defaults.
     *
     * NOTE: We do not set the use count here because the object is not yet
     * used for usage by the caller now. PoRegisterSystemState which is the
     * legacy API variant immediately uses the object while we don't. Only
     * when the caller invokes PoSetPowerRequest the use count gets incremented
     * alongside with the specific request type. Same rule applies for the
     * request type active counts too.
     */
    PowerRequest->DeviceRequestor = DeviceObject;
    PowerRequest->UseCount = 0;
    PowerRequest->Legacy = FALSE;
    PowerRequest->Terminate = FALSE;
    PowerRequest->RequestorMode = PreviousMode;
    PowerRequest->LegacyStateFlags = 0;
    PowerRequest->Context = ReasonContext;

    /* Copy the image filename that created this power request */
    RtlCopyMemory(PowerRequest->ImageFileName,
                  PsGetCurrentProcess()->ImageFileName,
                  min(sizeof(PowerRequest->ImageFileName), sizeof(PsGetCurrentProcess()->ImageFileName)));

    /* Default the active request counters */
    PowerRequest->ActiveRequests.ActiveSystemRequiredRequests = 0;
    PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests = 0;
    PowerRequest->ActiveRequests.ActiveAwayModeRequiredRequests = 0;
    PowerRequest->ActiveRequests.ActiveExecutionRequests = 0;

    /* Give the registered power request object to caller */
    *PowerRequestObject = PowerRequest;
    PopReleasePowerRequestLock(&LockHandle);

    /* Report the newly created power request object to the debugger */
#if DBG
    PopReportPowerRequest(PowerRequest);
#endif

    return Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopReapTerminatePowerRequestsDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    /* Invoke the reaper to reap each active power request and terminate them */
    PopTerminatePowerRequests();
}

VOID
NTAPI
PopTerminatePowerRequests(VOID)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY ListHead, NextEntry;
    PPOP_POWER_REQUEST PowerRequest;

    /* Don't bother looking for power requests to terminate if there aren't any */
    if (IsListEmpty(&PopPowerRequestsList))
    {
        return;
    }

    /* Lock down the list and start iterating over the active power requests */
    PopAcquirePowerRequestLock(&LockHandle);
    ListHead = &PopPowerRequestsList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /*
         * Is this a modern power request object created by one of the
         * calls like PoCreatePowerRequest?
         */
        PowerRequest = CONTAINING_RECORD(NextEntry, POP_POWER_REQUEST, Link);
        if (PowerRequest->Legacy == FALSE)
        {
            /*
             * Assign the terminate marker so that the Power Manager understands
             * this object is currently terminated and none of the requests set
             * in it should be considered.
             */
            PowerRequest->Terminate = TRUE;
        }
    }

    /* We're done here */
    PopReleasePowerRequestLock(&LockHandle);
}

NTSTATUS
NTAPI
PopChangePowerRequestProperties(
    _In_ PPOP_POWER_REQUEST PowerRequest,
    _In_ POWER_REQUEST_TYPE RequestType,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ BOOLEAN ClearRequest)
{
    /* We must not be above the permitted IRQL */
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* The lock must be held while changing the properties of this request */
    POP_ASSERT_POWER_REQUEST_LOCK_THREAD_OWNERSHIP();

    /*
     * Power requests coming from kernel mode drivers are only permitted
     * to set the system request, any other requests are permitted from
     * the user mode. Bail out if this rule isn't respected.
     */
    if (RequestType != PowerRequestSystemRequired && PreviousMode != UserMode)
    {
        DPRINT1("Power request type not supported in kernel mode (Type %d)\n", RequestType);
        return STATUS_NOT_SUPPORTED;
    }

    /* Increment or decrement the use count of this object depending on the caller's actions */
    if (!ClearRequest)
    {
        InterlockedIncrementUL(&PowerRequest->UseCount);
    }
    else
    {
        InterlockedDecrementUL(&PowerRequest->UseCount);
    }

    /* Clear or set the power request type accordingly */
    switch (RequestType)
    {
        /* Do not dim the display */
        case PowerRequestDisplayRequired:
        {
            if (!ClearRequest)
            {
                InterlockedIncrementUL(&PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests);
            }
            else
            {
                InterlockedDecrementUL(&PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests);
            }

            break;
        }

        /* Make the system busy */
        case PowerRequestSystemRequired:
        {
            if (!ClearRequest)
            {
                InterlockedIncrementUL(&PowerRequest->ActiveRequests.ActiveSystemRequiredRequests);
            }
            else
            {
                InterlockedDecrementUL(&PowerRequest->ActiveRequests.ActiveSystemRequiredRequests);
            }

            break;
        }

        /* Turn OFF the audio and display but not the computer */
        case PowerRequestAwayModeRequired:
        {
            if (!ClearRequest)
            {
                InterlockedIncrementUL(&PowerRequest->ActiveRequests.ActiveAwayModeRequiredRequests);
            }
            else
            {
                InterlockedDecrementUL(&PowerRequest->ActiveRequests.ActiveAwayModeRequiredRequests);
            }

            break;
        }

        /* Ensure the current calling process who made this request is not terminated */
        case PowerRequestExecutionRequired:
        {
            if (!ClearRequest)
            {
                InterlockedIncrementUL(&PowerRequest->ActiveRequests.ActiveExecutionRequests);
            }
            else
            {
                InterlockedDecrementUL(&PowerRequest->ActiveRequests.ActiveExecutionRequests);
            }

            break;
        }

        default:
        {
            DPRINT1("Unknown power request type detected (Type %d)\n", RequestType);
            return STATUS_NOT_SUPPORTED;
        }
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
PopClosePowerRequestObject(
    _In_opt_ PEPROCESS Process,
    _In_ PVOID PowerRequestObject,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG ProcessHandleCount,
    _In_ ULONG SystemHandleCount)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PPOP_POWER_REQUEST PowerRequest = (PPOP_POWER_REQUEST)PowerRequestObject;

    /*
     * Newer power requests MUST NOT have any referenced use counts while
     * we are within the close procedure. The object is about to go away soon.
     * We do not care if this is a legacy request because the use count is
     * served for statistical purposes.
     */
    if (!PowerRequest->Legacy)
    {
        ASSERT(PowerRequest->UseCount == 0);
        ASSERT(PowerRequest->ActiveRequests.ActiveSystemRequiredRequests == 0);
        ASSERT(PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests == 0);
        ASSERT(PowerRequest->ActiveRequests.ActiveAwayModeRequiredRequests == 0);
        ASSERT(PowerRequest->ActiveRequests.ActiveExecutionRequests == 0);
        ASSERT(PowerRequest->ActiveRequests.ActiveUserPresentRequests == 0);
    }

    /*
     * Decrement the global power request count accordingly to whom
     * this object was created by.
     */
    if (PowerRequest->RequestorMode == KernelMode)
    {
        PopTotalKernelPowerRequestsCount--;
    }
    else
    {
        PopTotalUserPowerRequestsCount--;
    }

    /* Delist the power request from the global list */
    PopAcquirePowerRequestLock(&LockHandle);
    RemoveEntryList(&PowerRequest->Link);
    PopReleasePowerRequestLock(&LockHandle);

    /*
     * Deactivate the reaper termiante timer if this was the last power
     * request so that the reaper doesn't need to be ran needlessly.
     */
    if (IsListEmpty(&PopPowerRequestsList) && PopReaperTerminateActivated)
    {
        KeCancelTimer(&PopReapTerminatePowerRequestsTimer);
        PopReaperTerminateActivated = FALSE;
    }

    /* If this object comes with a reason context then free it */
    if (PowerRequest->Context != NULL)
    {
        PopReleaseReasonContext(PowerRequest->Context, PowerRequest->RequestorMode);
        PowerRequest->Context = NULL;
    }
}

NTSTATUS
NTAPI
PopRegisterPowerRequest(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ POP_POWER_REQUEST_INQUIRE_TYPE Request,
    _In_ BOOLEAN NewRegister,
    _In_ EXECUTION_STATE EsFlags,
    _In_opt_ PCOUNTED_REASON_CONTEXT Context,
    _Inout_opt_ PPOP_POWER_REQUEST *PowerRequestHandle)
{
    NTSTATUS Status;

    PAGED_CODE();

    /* The caller should submit a valid request otherwise  */
    ASSERT((Request >= RegisterLegacyRequest) && (Request <= RegisterALaVistaRequest));

    /* Forward the request to the appropriate helper */
    switch (Request)
    {
        case RegisterLegacyRequest:
        {
            /*
             * The caller invoked a legacy request. This typically means a call of
             * PoRegisterSystemState was invoked, therefore we must relay the management
             * of this request to the legacy helper.
             */
            Status = PopCreatePowerRequestAsLegacy(EsFlags,
                                                   NewRegister,
                                                   PowerRequestHandle);
            break;
        }

        case RegisterALaVistaRequest:
        {
            /*
             * This is a Vista+ request. This typically means a call of
             * PoCreatePowerRequest was invoked.
             */
            Status = PopCreatePowerRequestObject(DeviceObject,
                                                 Context,
                                                 PowerRequestHandle);
            break;
        }

        /* The kernel SHOULD NOT BE REACHING HERE */
        DEFAULT_UNREACHABLE;
    }

    return Status;
}

VOID
NTAPI
PoRundownPowerRequestThread(
    _In_ PETHREAD Thread)
{
    PPOP_POWER_REQUEST PowerRequestObject;

    PAGED_CODE();

    /* This thread never had a power request, our job is done here */
    if (Thread->LegacyPowerObject == NULL)
    {
        return;
    }

    /* Cache the power request and make sure it's a legacy one */
    PowerRequestObject = (PPOP_POWER_REQUEST)Thread->LegacyPowerObject;
    ASSERT(PowerRequestObject->Legacy == TRUE);

    /* Invoke the Object Manager to delete the power request object */
    ObDereferenceObject(PowerRequestObject);
    Thread->LegacyPowerObject = NULL;
}

/* EOF */
