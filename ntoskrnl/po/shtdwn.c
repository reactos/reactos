/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power shutdown mechanism routines
 * COPYRIGHT:   Copyright ReactOS Portable Systems Group
 *              Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include "inbv/logo.h"

/* GLOBALS ********************************************************************/

KEVENT PopShutdownEvent;
PPOP_SHUTDOWN_WAIT_ENTRY PopShutdownThreadList;
LIST_ENTRY PopShutdownQueue;
WORK_QUEUE_ITEM PopShutdownWorkItem;
KGUARDED_MUTEX PopShutdownListMutex;
BOOLEAN PopShutdownListAvailable;
BOOLEAN PopShutdownCleanly = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static
NTSTATUS
PopRequestShutdownEvent(
    _In_ PETHREAD Thread)
{
    PPOP_SHUTDOWN_WAIT_ENTRY ShutDownWaitEntry;

    PAGED_CODE();

    /* Allocate memory pool for the shutdown wait entry */
    ShutDownWaitEntry = PopAllocatePool(sizeof(*ShutDownWaitEntry),
                                        TRUE,
                                        TAG_PO_SHUTDOWN_EVENT);
    if (ShutDownWaitEntry == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    /*
     * Put a reference on this thread so that it does not simply die in
     * our arms when we are going to process shutdown events.
     */
    ObReferenceObject(Thread);
    ShutDownWaitEntry->Thread = Thread;

    /*
     * If there is already a shutdown action in progress then we simply
     * cannot put this entry for later processing anymore.
     */
    PopAcquireShutdownLock();
    if (!PopShutdownListAvailable)
    {
        ObDereferenceObject(Thread);
        PopFreePool(ShutDownWaitEntry, TAG_PO_SHUTDOWN_EVENT);
        PopReleaseShutdownLock();
        return STATUS_UNSUCCESSFUL;
    }

    /* It is still available, put the entry in the list */
    ShutDownWaitEntry->NextEntry = PopShutdownThreadList;
    PopShutdownThreadList = ShutDownWaitEntry;
    PopReleaseShutdownLock();
    return STATUS_SUCCESS;
}

static
VOID
PopProcessShutDownLists(VOID)
{
    PLIST_ENTRY Entry;
    PWORK_QUEUE_ITEM WorkItem;
    PPOP_SHUTDOWN_WAIT_ENTRY ShutDownWaitEntry;

    /* Signal the shutdown evern so that everybody knows the system is shutting down */
    KeSetEvent(&PopShutdownEvent, IO_NO_INCREMENT, FALSE);

    /*
     * Block any further shutdown event requests on an impeding shutdown procedure
     * of the system. It is not problem if we release the lock that early because
     * up to that point we have total control of the queued shutdown events in the list.
     */
    PopAcquireShutdownLock();
    PopShutdownListAvailable = FALSE;
    PopReleaseShutdownLock();

    /* Process any queued shutdown work items */
    while (!IsListEmpty(&PopShutdownQueue))
    {
        Entry = RemoveHeadList(&PopShutdownQueue);
        WorkItem = CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);
        WorkItem->WorkerRoutine(WorkItem->Parameter);
    }

    /* Process any inserted threads into the shutdown list */
    while (PopShutdownThreadList != NULL)
    {
        ShutDownWaitEntry = PopShutdownThreadList;
        PopShutdownThreadList = PopShutdownThreadList->NextEntry;

        /* Wait for the thread to finish its operations before dereferencing it */
        KeWaitForSingleObject(ShutDownWaitEntry->Thread, 0, 0, 0, 0);
        ObDereferenceObject(ShutDownWaitEntry->Thread);
        PopFreePool(ShutDownWaitEntry, TAG_PO_SHUTDOWN_EVENT);
    }
}

static
ULONG
PopEnumLiveProcesses(VOID)
{
    ULONG ProcessCount = 0;
    PEPROCESS Process = NULL;

    /* Loop every active process that could not be killed */
    Process = PsGetNextProcess(Process);
    while (Process)
    {
        /* Make sure this isn't the idle or initial process */
        if ((Process != PsInitialSystemProcess) && (Process != PsIdleProcess))
        {
            /* We still have an active process, count it */
            DPRINT1("%15s is still RUNNING (%p)\n", Process->ImageFileName, Process->UniqueProcessId);
            ProcessCount++;
        }

        /* Get the next process */
        Process = PsGetNextProcess(Process);
    }

    return ProcessCount;
}

static
VOID
PopShutdownSystem(
    _In_ POWER_ACTION SystemAction)
{
    NTSTATUS Status;

    /* Unload the debug symbols before shutdown */
    DbgUnLoadImageSymbols(NULL, (PVOID)-1, 0);

    /* Shutdown the system depending on what the caller asked */
    switch (SystemAction)
    {
        case PowerActionShutdownReset:
        {
            /* It asked for a reboot, handle it to the dedicated state handler */
            PopInvokeSystemStateHandler(PowerStateShutdownReset, NULL);

            /* That did not work, we must reboot the system with the help of HAL instead */
            HalReturnToFirmware(HalRebootRoutine);
            break;
        }

        case PowerActionShutdown:
        {
            /*
             * The caller wants to shutdown the system but not powering it OFF.
             * In this case we must forcibly register the default state handler to
             * ours, thereby ignoring the one registered by HAL.
             */
            if (PopShutdownPowerOffPolicy)
            {
                if (PopDefaultPowerStateHandlers[PowerStateShutdownOff].Handler != PopShutdownHandler)
                {
                    Status = PopRegisterSystemStateHandler(PowerStateShutdownOff,
                                                           FALSE,
                                                           PopShutdownHandler,
                                                           NULL);
                    NT_ASSERT(NT_SUCCESS(Status));
                }
            }

            /*
             * Invoke the state handler. This should not fail supposedly we registered
             * the system state with our handler. But in case we fail the request we
             * must power down...
             */
            PopInvokeSystemStateHandler(PowerStateShutdownOff, NULL);
            HalReturnToFirmware(HalPowerDownRoutine);
            break;
        }

        case PowerActionShutdownOff:
        {
            /* The caller wants to shutdown the system completely */
            PopInvokeSystemStateHandler(PowerStateShutdownOff, NULL);

            /* That did not work, we must shutdown the system with the help of HAL instead */
            HalReturnToFirmware(HalPowerDownRoutine);
            break;
        }

        default:
        {
            /* I do not know what the caller asked, so better reboot the machine */
            HalReturnToFirmware(HalRebootRoutine);
            break;
        }
    }

    /* Anything else should not happen */
    KeBugCheckEx(INTERNAL_POWER_ERROR, 5, 0, 0, 0);
}

/* PUBLIC FUNCTIONS ***********************************************************/

_Use_decl_annotations_
VOID
NTAPI
PopGracefulShutdown(
    _In_ PVOID Parameter)
{
    ULONG ProcessCount;

    /*
     * If somebody issued a bugcheck within the power actions, at this
     * point we must crash the system now!
     */
    if (PopAction.ShutdownBugCode)
    {
        KeBugCheckEx(PopAction.ShutdownBugCode->Code,
                     PopAction.ShutdownBugCode->Parameter1,
                     PopAction.ShutdownBugCode->Parameter2,
                     PopAction.ShutdownBugCode->Parameter3,
                     PopAction.ShutdownBugCode->Parameter4);
    }

    /*
     * If a clean shutdown procedure was requested we must shutdown every
     * subsystem of the Executive layer of the kernel.
     */
    if (PopShutdownCleanly)
    {
        /* Process the registered shutdown events and queued work items */
        PopProcessShutDownLists();

        /*
         * Invoke the Process Manager to terminate any non-critical user mode
         * processes that are still present in the process space, and shutdown
         * the Process Manager.
         *
         * Normally any user mode app is given a shutdown notification and respond to it
         * with no haste, but if ever a naughty application.
         */
        DPRINT1("Process manager shutting down\n");
        PsShutdownSystem();

        /* Make sure that no other processes are alive other than the system ones */
        ProcessCount = PopEnumLiveProcesses();
        ASSERT(ProcessCount == 0);
    }

    /* Invoke the "End Of Boot" HAL routine as the system is shutting down */
    DPRINT1("Executing HAL End of Boot procedure\n");
    HalEndOfBoot();

    /* Shutdown the Shim cache manager if enabled */
    DPRINT1("Shim cache manager shutting down\n");
    ApphelpCacheShutdown();

    /*
     * Shutdown the I/O manager in Phase 0. At this phase every driver is notified
     * with a shutdown IRP to let them know that shutdown is imminent so that they
     * can prepare packing up stuff.
     */
    DPRINT1("I/O manager shutting down in phase 0\n");
    IoShutdownSystem(0);

    /* Flush the registry hives and shutdown the Configuration Manager */
    DPRINT1("Configuration Manager shutting down\n");
    CmShutdownSystem();

    /* Punt the hard-error related stuff initialized by the Executive and shutdown it */
    DPRINT1("Executive shutting down\n");
    ExShutdownSystem();

    /*
     * Shutdown the Memory Manager in Phase 0. At this phase all the paging files are
     * closed and their corresponding page file names freed.
     */
    DPRINT1("Memory manager shutting down in phase 0\n");
    MmShutdownSystem(0);

    /* Wait on the Cache Controller to process the last batch of lazy writer activity */
    DPRINT1("Waiting on Cc to finish the current lazy writer activity\n");
    CcWaitForCurrentLazyWriterActivity();

    /* Flush all user files and shutdown the Cache Controller */
    DPRINT1("Cache Controller shutting down\n");
    CcShutdownSystem();

    /* Process the last I/O operations and shutdown that manager completely */
    DPRINT1("I/O manager shutting down in phase 1\n");
    IoShutdownSystem(1);

    /* FIXME: Must broadcast the power IRP to all devices here */

    /* Shutdown the Object Manager */
    if (PopShutdownCleanly)
    {
        DPRINT1("Object Manager shutting down\n");
        ObShutdownSystem();
    }

    /* Disable any wake timer alarms */
    DPRINT1("Disabling HAL wake alarms\n");
    HalSetWakeEnable(FALSE);

    /* Perform the final shutdown of the Memory Manager */
    DPRINT1("Memory manager shutting down in phase 2\n");
    MmShutdownSystem(2);

    /* Handle the power action to shutdown the system */
    DPRINT1("System shutdown imminent... (action %lx)\n", PopAction.Action);
    PopShutdownSystem(PopAction.Action);
}

NTSTATUS
NTAPI
PopShutdownHandler(
    _In_opt_ PVOID Context,
    _In_opt_ PENTER_STATE_SYSTEM_HANDLER SystemHandler,
    _In_opt_ PVOID SystemContext,
    _In_ LONG NumberProcessors,
    _In_opt_ LONG volatile *Number)
{
    /* Disable interrupts and make sure the shutdown screen is enacted on the first processor */
    _disable();
    if (KeGetCurrentPrcb()->Number == 0)
    {
        /* Display the shutdown screen only if boot video is installed */
        if (InbvIsBootDriverInstalled())
        {
            if (!InbvCheckDisplayOwnership())
            {
                InbvAcquireDisplayOwnership();
            }

            InbvResetDisplay();
            DisplayShutdownBitmap();
        }
        else
        {
            /* No boot video is installed, display the shutdown screen in text-mode */
            DisplayShutdownText();
        }
    }

    /* Hang the system */
    for (;;)
    {
        HalHaltSystem();
    }
}

NTSTATUS
NTAPI
PoQueueShutdownWorkItem(
    _In_ PWORK_QUEUE_ITEM WorkItem)
{
    PAGED_CODE();

    /*
     * Do not enqueue a shutdown work item if a shutdown action
     * is already in progress.
     */
    PopAcquireShutdownLock();
    if (!PopShutdownListAvailable)
    {
        DPRINT1("Cannot enqueue new shutdown work items, shutdown is in progress\n");
        PopReleaseShutdownLock();
        return STATUS_SYSTEM_SHUTDOWN;
    }

    InsertTailList(&PopShutdownQueue, &WorkItem->List);
    PopReleaseShutdownLock();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PoRequestShutdownEvent(
    _Out_ PVOID *Event)
{
    NTSTATUS Status;

    PAGED_CODE();

    /* Initialize the pointer to NULL */
    if (Event)
    {
        *Event = NULL;
    }

    /* Invoke the private helper to do the job */
    Status = PopRequestShutdownEvent(PsGetCurrentThread());
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to request a shutdown event (Status 0x%lx)\n", Status);
        return Status;
    }

    /* Give to the caller the global shutdown event */
    if (Event)
    {
        *Event = &PopShutdownEvent;
    }

    return STATUS_SUCCESS;
}

/* EOF */
