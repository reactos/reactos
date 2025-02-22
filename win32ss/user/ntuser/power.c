/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Power management of the Win32 kernel-mode subsystem
 * COPYRIGHT:       Copyright 2024 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserPowerManager);

/* GLOBALS *******************************************************************/

LIST_ENTRY gPowerCalloutsQueueList;
PFAST_MUTEX gpPowerCalloutMutexLock;
PKEVENT gpPowerRequestCalloutEvent;
PKTHREAD gpPowerCalloutMutexOwnerThread;

/* PRIVATE FUNCTIONS *********************************************************/

/**
 * @brief
 * Handles a power event as a result from an incoming power
 * callout from the kernel power manager.
 *
 * @param[in] pParameters
 * A pointer to the power event parameters containing the
 * power event type and sub-code serving as additional datum
 * for that power event type.
 */
static
VOID
IntHandlePowerEventWorker(
    _In_ PWIN32_POWEREVENT_PARAMETERS pParameters)
{
    PSPOWEREVENTTYPE PwrEventType;
    ULONG Code;

    /* Cache the power event parameters and handle the power callout */
    PwrEventType = pParameters->EventNumber;
    Code = pParameters->Code;
    switch (PwrEventType)
    {
        case PsW32SystemTime:
        {
            /*
             * The power manager of the kernel notified us of an impending
             * time change, broadcast this notification to all present windows.
             */
            UserSendNotifyMessage(HWND_BROADCAST,
                                  WM_TIMECHANGE,
                                  0,
                                  0);
            break;
        }

        default:
        {
            TRACE("Power event of type %d is currently UNIMPLEMENTED (code %lu)\n", PwrEventType, Code);
            break;
        }
    }
}

/**
 * @brief
 * Validates the power event parameters that come from
 * a power callout from the kernel power manager.
 *
 * @param[in] pParameters
 * A pointer to the power event parameters containing the
 * power event type of which is to be validated against
 * valid power events.
 *
 * @return
 * Returns STATUS_INVALID_PARAMETER if the captured power
 * event type is unknown, otherwise STATUS_SUCCESS is returned.
 */
static
NTSTATUS
IntValidateWin32PowerParams(
    _In_ PWIN32_POWEREVENT_PARAMETERS pParameters)
{
    PSPOWEREVENTTYPE PwrEventType;

    /* Capture the event number and check if it is within bounds */
    PwrEventType = pParameters->EventNumber;
    if (PwrEventType < PsW32FullWake || PwrEventType > PsW32MonitorOff)
    {
        TRACE("Unknown event number found -> %d\n", PwrEventType);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Gets the next pending power callout from the global queue
 * list and returns it to the caller. Note that the returned
 * power callout is delisted from the list.
 *
 * @param[in] pPowerCallout
 * A pointer to a power callout entry that was previously returned
 * by the same function. If this parameter is set to NULL the function
 * will return the first callout entry from the list. Otherwise the
 * function will return the next callout entry of the current power
 * callout.
 *
 * @return
 * Returns a pointer to a power callout to the caller. If the list is
 * empty then it will return NULL.
 *
 * @remarks
 * The caller ASSUMES responsibility to lock down the power callout list
 * before it begins enumerating the global list!
 */
static
PWIN32POWERCALLOUT
IntGetNextPowerCallout(
    _In_ PWIN32POWERCALLOUT pPowerCallout)
{
    PLIST_ENTRY Entry;
    PWIN32POWERCALLOUT pPowerCalloutEntry = NULL;

    /* Ensure the current calling thread owns the power callout lock */
    ASSERT_POWER_CALLOUT_LOCK_ACQUIRED();

    /* This list is empty, acknowledge the caller */
    if (IsListEmpty(&gPowerCalloutsQueueList))
    {
        return NULL;
    }

    /* The caller supplied a NULL argument, give them the first entry */
    if (!pPowerCallout)
    {
        Entry = gPowerCalloutsQueueList.Flink;
    }
    else
    {
        /* Otherwise give the caller the next power callout entry from the list */
        Entry = pPowerCallout->Link.Flink;
    }

    /* Delist the power callout entry from the list and give it to caller */
    pPowerCalloutEntry = CONTAINING_RECORD(Entry, WIN32POWERCALLOUT, Link);
    RemoveEntryList(&pPowerCalloutEntry->Link);
    return pPowerCalloutEntry;
}

/**
 * @brief
 * Deploys all pending power callouts to appropriate power
 * callout workers.
 */
static
VOID
IntDeployPowerCallout(VOID)
{
    PWIN32POWERCALLOUT pWin32PwrCallout;

    /* Lock the entire USER subsystem down as we do particular stuff */
    UserEnterExclusive();

    /*
     * FIXME: While we did indeed lock the USER subsystem, there is a risk
     * of the current calling thread might die or hang, so we should probably
     * lock the thread while we deploy our power callout. The thread info
     * provides a thread lock field for this purpose (see the ptl member from
     * the _THREADINFO structure) but ReactOS lacks implementation to handle
     * this. Suppose a thread happens to get into this fate, the power callout
     * would never get signaled...
     */

    /* Deploy all the pending power callouts to the appropriate callout workers */
    IntAcquirePowerCalloutLock();
    for (pWin32PwrCallout = IntGetNextPowerCallout(NULL);
         pWin32PwrCallout != NULL;
         pWin32PwrCallout = IntGetNextPowerCallout(pWin32PwrCallout))
    {
        if (pWin32PwrCallout->Type == POWER_CALLOUT_EVENT)
        {
            IntHandlePowerEventWorker(&pWin32PwrCallout->Params);
        }
        else // POWER_CALLOUT_STATE
        {
            ERR("Power state callout management is currently not implemented!\n");
        }

        /* We are done with this power callout */
        ExFreePoolWithTag(pWin32PwrCallout, USERTAG_POWER);
    }

    /* Release what we locked */
    IntReleasePowerCalloutLock();
    UserLeave();
}

/**
 * @brief
 * Enlists a newly allocated power callout into the queue list
 * for later processing.
 *
 * @param[in] pPowerCallout
 * A pointer to a power callout that is to be inserted into the
 * queue list.
 */
static
VOID
IntEnlistPowerCallout(
    _In_ PWIN32POWERCALLOUT pPowerCallout)
{
    PETHREAD CurrentThread;

    /* Enlist it to the queue already */
    IntAcquirePowerCalloutLock();
    InsertTailList(&gPowerCalloutsQueueList, &pPowerCallout->Link);
    IntReleasePowerCalloutLock();

    /*
     * We have to let CSRSS process this power callout if one of the
     * following conditions is TRUE for the current calling thread:
     *
     * - The process of the calling thread is a system process;
     * - The process of the calling thread is attached;
     * - The current calling thread is not a Win32 thread.
     *
     * For the second point, we cannot process the power callout ourselves
     * as we must lock down USER exclusively for our own purpose, which requires
     * us to be in a critical section. So we do not want to fiddle with a process
     * that is attached with others.
     */
    CurrentThread = PsGetCurrentThread();
    if (PsIsSystemThread(CurrentThread) ||
        KeIsAttachedProcess() ||
        !IntIsThreadWin32Thread(CurrentThread))
    {
        /* Alert CSRSS of the presence of an enqueued power callout */
        KeSetEvent(gpPowerRequestCalloutEvent, EVENT_INCREMENT, FALSE);
        return;
    }

    /* Handle this power callout ourselves */
    IntDeployPowerCallout();
}

/* PUBLIC FUNCTIONS **********************************************************/

/**
 * @brief
 * Initializes the power management side of Win32 kernel-mode
 * subsystem component. This enables communication between
 * the power manager of the NT kernel and Win32k.
 *
 * @param[in] hPowerRequestEvent
 * A handle to the global power request event, provided by the
 * Winsrv module. This allows CSRSS to be notified of power callouts
 * that cannot be handled by Win32k.
 *
 * @return
 * Returns STATUS_INSUFFICIENT_RESOURCES if pool allocation for the
 * power callout lock has failed due to lack of necessary memory.
 * A failure NTSTATUS code is returned if the power request event
 * could not be referenced. Otherwise STATUS_SUCCESS is returned to
 * indicate the power management has initialized successfully.
 */
NTSTATUS
NTAPI
IntInitWin32PowerManagement(
    _In_ HANDLE hPowerRequestEvent)
{
    NTSTATUS Status;

    /* Allocate memory pool for the power callout mutex */
    gpPowerCalloutMutexLock = ExAllocatePoolZero(NonPagedPool,
                                                 sizeof(FAST_MUTEX),
                                                 USERTAG_POWER);
    if (gpPowerCalloutMutexLock == NULL)
    {
        ERR("Failed to allocate pool of memory for the power callout mutex\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize the mutex and owner thread */
    ExInitializeFastMutex(gpPowerCalloutMutexLock);
    gpPowerCalloutMutexOwnerThread = NULL;

    /* Initialize the global queue list and the power callout (aka request) event object */
    InitializeListHead(&gPowerCalloutsQueueList);
    Status = ObReferenceObjectByHandle(hPowerRequestEvent,
                                       EVENT_ALL_ACCESS,
                                       *ExEventObjectType,
                                       KernelMode,
                                       (PVOID *)&gpPowerRequestCalloutEvent,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to reference the power callout event handle (Status 0x%08lx)\n", Status);
        ExFreePoolWithTag(gpPowerCalloutMutexLock, USERTAG_POWER);
        return Status;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Cleanup procedure that frees all the allocated resources by
 * the power manager. It is triggered during Win32k subsystem unloading.
 */
NTSTATUS
NTAPI
IntWin32PowerManagementCleanup(VOID)
{
    PWIN32POWERCALLOUT pWin32PwrCallout;

    /* Dereference the power request event */
    ObDereferenceObject(gpPowerRequestCalloutEvent);
    gpPowerRequestCalloutEvent = NULL;

    /*
     * Enumerate all pending power callouts and free them. We do not
     * need to do this with the lock held as the CSR process is tore
     * apart during Win32k cleanup, so future power callouts would not
     * be allowed anyway, therefore we are safe.
     */
    for (pWin32PwrCallout = IntGetNextPowerCallout(NULL);
         pWin32PwrCallout != NULL;
         pWin32PwrCallout = IntGetNextPowerCallout(pWin32PwrCallout))
    {
        ExFreePoolWithTag(pWin32PwrCallout, USERTAG_POWER);
    }

    /* Tear apart the power callout lock mutex */
    ExFreePoolWithTag(gpPowerCalloutMutexLock, USERTAG_POWER);
    gpPowerCalloutMutexLock = NULL;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Handles an incoming power event callout from the NT power
 * manager.
 *
 * @param[in] pWin32PwrEventParams
 * A pointer to power event parameters that is given by the
 * NT power manager of the kernel.
 *
 * @return
 * Returns STATUS_UNSUCCESSFUL if the Client/Server subsystem is
 * not running, of which power callouts cannot be handled.
 * Returns STATUS_INVALID_PARAMETER if the provided power event
 * parameters are not valid. Returns STATUS_INSUFFICIENT_RESOURCES
 * if there is a lack of memory to allocate for the power callout.
 * Otherwise it returns STATUS_SUCCESS to indicate the power callout
 * was handled successfully.
 */
NTSTATUS
NTAPI
IntHandlePowerEvent(
    _In_ PWIN32_POWEREVENT_PARAMETERS pWin32PwrEventParams)
{
    PWIN32POWERCALLOUT pWin32PwrCallout;
    NTSTATUS Status;

    /*
     * CSRSS is not running. As a consequence, the USER subsystem is neither
     * up and running as the Client/Server subsystem is responsible to fire
     * up other subsystems. Another case is that the system undergoes shutdown
     * and Win32k cleanup is currently in effect. Either way, just quit.
     */
    if (!gpepCSRSS)
    {
        TRACE("CSRSS is not running, bailing out\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Validate the power event parameters, just to be sure we have not gotten anything else */
    Status = IntValidateWin32PowerParams(pWin32PwrEventParams);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not deploy power callout, invalid Win32 power parameters!\n");
        return Status;
    }

    /* Allocate pool of memory for this power callout */
    pWin32PwrCallout = ExAllocatePoolZero(NonPagedPool,
                                          sizeof(WIN32POWERCALLOUT),
                                          USERTAG_POWER);
    if (pWin32PwrCallout == NULL)
    {
        ERR("Allocating memory for Win32 power callout failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Fill the necessary power datum */
    pWin32PwrCallout->Type = POWER_CALLOUT_EVENT;
    pWin32PwrCallout->Params.EventNumber = pWin32PwrEventParams->EventNumber;
    pWin32PwrCallout->Params.Code = pWin32PwrEventParams->Code;

    /* Enqueue this power request for later processing */
    IntEnlistPowerCallout(pWin32PwrCallout);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Handles an incoming power state callout from the NT power
 * manager.
 *
 * @param[in] pWin32PwrStateParams
 * A pointer to power state parameters that is given by the
 * NT power manager of the kernel.
 */
NTSTATUS
NTAPI
IntHandlePowerState(
    _In_ PWIN32_POWERSTATE_PARAMETERS pWin32PwrStateParams)
{
    /* FIXME */
    ERR("IntHandlePowerState is UNIMPLEMENTED\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
