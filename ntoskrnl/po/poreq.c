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
        InterlockedExchange(&PowerRequest->ActiveRequests.ActiveSystemRequiredRequests, 0);
        InterlockedExchange(&PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests, 0);
        InterlockedExchange(&PowerRequest->LegacyStateFlags, 0);
        return;
    }

    /* Overwrite the current state flags with the newer ones */
    InterlockedExchange(&PowerRequest->LegacyStateFlags, EsFlags);

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
            break;
        }

        /* The kernel SHOULD NOT BE REACHING HERE */
        DEFAULT_UNREACHABLE;
    }

    return Status;
}

/* EOF */
