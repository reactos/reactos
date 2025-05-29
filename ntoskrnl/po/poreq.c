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

POBJECT_TYPE PopPowerRequestObjectType = NULL;
LIST_ENTRY PopPowerRequestsList;
ULONG PopTotalKernelPowerRequestsCount;
ULONG PopTotalUserPowerRequestsCount;
KSPIN_LOCK PopPowerRequestLock;
PKTHREAD PopPowerRequestOwnerLockThread;

/* PRIVATE FUNCTIONS **********************************************************/

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
                            PopPowerRequestObjectType,
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
    PowerRequestObject->ComingFromUserMode = FALSE;
    PowerRequestObject->LegacyStateFlags = EsFlags;
    PowerRequestObject->Context = NULL;

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
PopValidateReasonContext(
    _In_ PCOUNTED_REASON_CONTEXT Context,
    _In_ BOOLEAN TrustedCaller)
{
    /* We MUST have a reason context to validate for */
    ASSERT(Context != NULL);

    /*
     * TODO: MUST IMPLEMENT VALIDATION FOR USER MODE PART
     *
     * For user mode we must probe the buffers that are readable
     * and are within proper alignments.
     */
    UNREFERENCED_PARAMETER(TrustedCaller);

    /* Verify the version of the context is what we expect */
    if (Context->Version != DIAGNOSTIC_REASON_VERSION)
    {
        DPRINT1("Invalid reason context version passed, Version = %lu\n", Context->Version);
        return FALSE;
    }

    /*
     * The caller cannot fool us by providing a reason context but
     * with two of the diagnostic flags. ONLY ONE flag can be specified.
     */
    if ((Context->Flags & DIAGNOSTIC_REASON_SIMPLE_STRING) &&
        (Context->Flags & DIAGNOSTIC_REASON_DETAILED_STRING))
    {
        DPRINT1("DIAGNOSTIC_REASON_SIMPLE_STRING and DIAGNOSTIC_REASON_DETAILED_STRING specified, that's not VALID\n");
        return FALSE;
    }

    /* Validate the contents of the reason context depending on what the caller passed */
    if (Context->Flags & DIAGNOSTIC_REASON_SIMPLE_STRING)
    {
        /*
         * The caller provided a simple string for the diagnostic reason
         * context. Is the string empty?
         */
        if (Context->SimpleString.Length == 0)
        {
            /* Empty string for a reason context? That's lame! */
            DPRINT1("Caller passed a reason context with empty simple string\n");
            return FALSE;
        }
    }
    else if (Context->Flags & DIAGNOSTIC_REASON_DETAILED_STRING)
    {
        /*
         * The caller provided an array of diagnostic strings. In this scenario
         * we must check the actual strings count, we don't care if the caller did
         * not pass a resource file name.
         */
        if (Context->StringCount == 0)
        {
            /* Empty string for a reason context? That's lame! */
            DPRINT1("Caller passed a reason context with empty simple string\n");
            return FALSE;
        }
    }
    else
    {
        /* I don't understand any other flags the caller passed, bail out */
        DPRINT1("Invalid diagnostic context reason flags passed (Flags 0x%lx)", Context->Flags);
        return FALSE;
    }

    return TRUE;
}

static
ULONG
PopCalculateReasonContextSize(
    _In_ PCOUNTED_REASON_CONTEXT Context)
{
    ULONG StringIndex, Size, TotalStringsSize = 0;

    /* We MUST have a reason context here */
    ASSERT(Context != NULL);

    /*
     * The caller passed a simple diagnostic string. Simply calculate
     * the size taking into account the length of the provided string.
     */
    if (Context->Flags & DIAGNOSTIC_REASON_SIMPLE_STRING)
    {
        /* The buffer length must not be 0 at this point */
        ASSERT(Context->SimpleString.Length != 0);

        Size = sizeof(COUNTED_REASON_CONTEXT) +
               (Context->SimpleString.Length * sizeof(UNICODE_STRING));
        return Size;
    }

    /* Better be safe and ensure ourselves the caller did indeed pass a detailed string */
    ASSERT(Context->Flags & DIAGNOSTIC_REASON_DETAILED_STRING);

    /*
     * The caller passed a detailed diagnostic reason with an array of strings.
     * Iterate over the array and add up the lengths of each string to make up
     * the total size necessary for the buffer.
     */
    for (StringIndex = 0;
         StringIndex < Context->StringCount;
         StringIndex++)
    {
        TotalStringsSize += Context->ReasonStrings[StringIndex].Length;
    }

    /*
     * We should expect at this point that a detailed diagnostic string
     * was provided. Supposedly we reach this path then the kernel literally
     * let slip the caller on providing an useless array with empty strings.
     */
    ASSERT(TotalStringsSize != 0);

    /* If the caller provided a resource filename then count that as well */
    if (Context->ResourceFileName.Length != 0)
    {
        TotalStringsSize += Context->ResourceFileName.Length;
    }

    /* Now return the calculated total size */
    Size = sizeof(COUNTED_REASON_CONTEXT) +
                 (TotalStringsSize * sizeof(UNICODE_STRING));
    return Size;
}

static
NTSTATUS
PopCreatePowerRequestObject(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN UserRequest,
    _In_opt_ PCOUNTED_REASON_CONTEXT Context,
    _Out_ PPOP_POWER_REQUEST *PowerRequestObject)
{
    NTSTATUS Status;
    BOOLEAN IsValid;
    ULONG ReasonContextSize;
    KLOCK_QUEUE_HANDLE LockHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PPOP_POWER_REQUEST PowerRequest;
    PCOUNTED_REASON_CONTEXT ReasonContext = NULL;
    KPROCESSOR_MODE ModeRequest = UserRequest ? UserMode : KernelMode;
    ULONG ObjFlags = UserRequest ? OBJ_CASE_INSENSITIVE : OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;

    PAGED_CODE();

    /* The passed DO must not be NULL, that's illegal */
    ASSERT(DeviceObject != NULL);

    /*
     * DO NOT ALLOCATE any pool memory for the reason context yet.
     * We have to ensure the passed reason context by the caller is
     * VALID at all.
     */
    if (Context != NULL)
    {
        IsValid = PopValidateReasonContext(Context, UserRequest);
        if (!IsValid)
        {
            DPRINT1("The passed COUNTED_REASON_CONTEXT by the caller is NOT VALID\n");
            return STATUS_INVALID_PARAMETER;
        }

        /*
         * Determine the exact size of the reason context before
         * allocating any memory pool for it.
         */
        ReasonContextSize = PopCalculateReasonContextSize(Context);
        ReasonContext = PopAllocatePool(ReasonContextSize,
                                        TRUE,
                                        TAG_PO_REASON_CONTEXT);
        if (ReasonContext == NULL)
        {
            DPRINT1("Failed to allocate memory pool for the diagnostic counted reason context\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Copy the passed reason context into the allocated buffer */
        RtlCopyMemory(ReasonContext,
                      Context,
                      ReasonContextSize);
    }

    /* Setup the object attributes for the power request (default settings) */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               ObjFlags,
                               NULL,
                               NULL);

    /* Register a power request object with Object Manager */
    Status = ObCreateObject(ModeRequest,
                            PopPowerRequestObjectType,
                            &ObjectAttributes,
                            ModeRequest,
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
            PopFreePool(ReasonContext, TAG_PO_REASON_CONTEXT);
        }

        return Status;
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
    PowerRequest->ComingFromUserMode = UserRequest;
    PowerRequest->LegacyStateFlags = 0;
    PowerRequest->Context = ReasonContext;

    /* Default the active request counters */
    PowerRequest->ActiveRequests.ActiveSystemRequiredRequests = 0;
    PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests = 0;
    PowerRequest->ActiveRequests.ActiveAwayModeRequiredRequests = 0;
    PowerRequest->ActiveRequests.ActiveExecutionRequests = 0;

    /* Give the registered power request object to caller */
    *PowerRequestObject = PowerRequest;
    PopReleasePowerRequestLock(&LockHandle);
    return Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PopDeletePowerRequestObject(
    _In_ PVOID PowerRequestObject)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PPOP_POWER_REQUEST PowerRequest = (PPOP_POWER_REQUEST)PowerRequestObject;

    /* Acquire the lock and delist the power request from the list */
    PopAcquirePowerRequestLock(&LockHandle);
    RemoveEntryList(&PowerRequest->Link);
    PopReleasePowerRequestLock(&LockHandle);

    /* TODO: De-allocate the counted reason context if it has one */
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
    ULONG UseCount;
    PPOP_POWER_REQUEST PowerRequest;

    /* We shouldn't be calling the close procedure on a zero use count */
    PowerRequest = (PPOP_POWER_REQUEST)PowerRequestObject;
    ASSERT(PowerRequest->UseCount != 0);

    /*
     * Simply decrement the use count of this object. If it reaches
     * 0 then call the delete procedure. Or if this is a legacy request
     * then the use count is treated like a statistical value of which
     * we don't care about it, unless it's a modern power request because
     * in this case the caller is responsible to clear all of power request
     * invocations with PoClearPowerRequest.
     */
    UseCount = InterlockedDecrementUL(&PowerRequest->UseCount);
    if (UseCount == 0 || PowerRequest->Legacy)
    {
        PopDeletePowerRequestObject(PowerRequest);
    }
}

NTSTATUS
NTAPI
PopRegisterPowerRequest(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ POP_POWER_REQUEST_INQUIRE_TYPE Request,
    _In_ BOOLEAN ComingFromuserMode,
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
                                                 ComingFromuserMode,
                                                 Context,
                                                 PowerRequestHandle);
            break;
        }

        /* The kernel SHOULD NOT BE REACHING HERE */
        DEFAULT_UNREACHABLE;
    }

    return Status;
}

/* EOF */
