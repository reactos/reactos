/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/po/poshtdwn.c
 * PURPOSE:         Power Manager Shutdown Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#ifdef NEWCC
#include <cache/newcc.h>
#endif
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG PopShutdownPowerOffPolicy;
KEVENT PopShutdownEvent;
PPOP_SHUTDOWN_WAIT_ENTRY PopShutdownThreadList;
LIST_ENTRY PopShutdownQueue;
KGUARDED_MUTEX PopShutdownListMutex;
BOOLEAN PopShutdownListAvailable;


/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
PopInitShutdownList(VOID)
{
    PAGED_CODE();

    /* Initialize the global shutdown event */
    KeInitializeEvent(&PopShutdownEvent, NotificationEvent, FALSE);

    /* Initialize the shutdown lists */
    PopShutdownThreadList = NULL;
    InitializeListHead(&PopShutdownQueue);

    /* Initialize the shutdown list lock */
    KeInitializeGuardedMutex(&PopShutdownListMutex);

    /* The list is available now */
    PopShutdownListAvailable = TRUE;
}

NTSTATUS
NTAPI
PoRequestShutdownWait(
    _In_ PETHREAD Thread)
{
    PPOP_SHUTDOWN_WAIT_ENTRY ShutDownWaitEntry;
    NTSTATUS Status;
    PAGED_CODE();

    /* Allocate a new shutdown wait entry */
    ShutDownWaitEntry = ExAllocatePoolWithTag(PagedPool, 8u, 'LSoP');
    if (ShutDownWaitEntry == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    /* Reference the thread and save it in the wait entry */
    ObReferenceObject(Thread);
    ShutDownWaitEntry->Thread = Thread;

    /* Acquire the shutdown list lock */
    KeAcquireGuardedMutex(&PopShutdownListMutex);

    /* Check if the list is still available */
    if (PopShutdownListAvailable)
    {
        /* Insert the item in the list */
        ShutDownWaitEntry->NextEntry = PopShutdownThreadList;
        PopShutdownThreadList = ShutDownWaitEntry;

        /* We are successful */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* We cannot proceed, cleanup and return failure */
        ObDereferenceObject(Thread);
        ExFreePoolWithTag(ShutDownWaitEntry, 0);
        Status = STATUS_UNSUCCESSFUL;
    }

    /* Release the list lock */
    KeReleaseGuardedMutex(&PopShutdownListMutex);

    /* Return the status */
    return Status;
}

VOID
NTAPI
PopProcessShutDownLists(VOID)
{
    PPOP_SHUTDOWN_WAIT_ENTRY ShutDownWaitEntry;
    PWORK_QUEUE_ITEM WorkItem;
    PLIST_ENTRY ListEntry;

    /* First signal the shutdown event */
    KeSetEvent(&PopShutdownEvent, IO_NO_INCREMENT, FALSE);

    /* Acquire the shutdown list lock */
    KeAcquireGuardedMutex(&PopShutdownListMutex);

    /* Block any further attempts to register a shutdown event */
    PopShutdownListAvailable = FALSE;

    /* Release the list lock, since we are exclusively using the lists now */
    KeReleaseGuardedMutex(&PopShutdownListMutex);

    /* Process the shutdown queue */
    while (!IsListEmpty(&PopShutdownQueue))
    {
        /* Get the head entry */
        ListEntry = RemoveHeadList(&PopShutdownQueue);
        WorkItem = CONTAINING_RECORD(ListEntry, WORK_QUEUE_ITEM, List);

        /* Call the shutdown worker routine */
        WorkItem->WorkerRoutine(WorkItem->Parameter);
    }

    /* Now process the shutdown thread list */
    while (PopShutdownThreadList != NULL)
    {
        /* Get the top entry and remove it from the list */
        ShutDownWaitEntry = PopShutdownThreadList;
        PopShutdownThreadList = PopShutdownThreadList->NextEntry;

        /* Wait for the thread to finish and dereference it */
        KeWaitForSingleObject(ShutDownWaitEntry->Thread, 0, 0, 0, 0);
        ObfDereferenceObject(ShutDownWaitEntry->Thread);

        /* Finally free the entry */
        ExFreePoolWithTag(ShutDownWaitEntry, 0);
    }
}

VOID
NTAPI
PopShutdownHandler(VOID)
{
    PUCHAR Logo1, Logo2;
    ULONG i;

    /* Stop all interrupts */
    KeRaiseIrqlToDpcLevel();
    _disable();

    /* Do we have boot video */
    if (InbvIsBootDriverInstalled())
    {
        /* Yes we do, cleanup for shutdown screen */
        if (!InbvCheckDisplayOwnership()) InbvAcquireDisplayOwnership();
        InbvResetDisplay();
        InbvSolidColorFill(0, 0, 639, 479, 0);
        InbvEnableDisplayString(TRUE);
        InbvSetScrollRegion(0, 0, 639, 479);

        /* Display shutdown logo and message */
        Logo1 = InbvGetResourceAddress(IDB_SHUTDOWN_LOGO);
        Logo2 = InbvGetResourceAddress(IDB_LOGO);
        if ((Logo1) && (Logo2))
        {
            InbvBitBlt(Logo1, 215, 352);
            InbvBitBlt(Logo2, 217, 111);
        }
    }
    else
    {
        /* Do it in text-mode */
        for (i = 0; i < 25; i++) InbvDisplayString("\r\n");
        InbvDisplayString("                       ");
        InbvDisplayString("The system may be powered off now.\r\n");
    }

    /* Hang the system */
    for (;;) HalHaltSystem();
}

VOID
NTAPI
PopShutdownSystem(IN POWER_ACTION SystemAction)
{
    /* Note should notify caller of NtPowerInformation(PowerShutdownNotification) */

    /* Unload symbols */
    DPRINT("It's the final countdown...%lx\n", SystemAction);
    DbgUnLoadImageSymbols(NULL, (PVOID)-1, 0);

    /* Run the thread on the boot processor */
    KeSetSystemAffinityThread(1);

    /* Now check what the caller wants */
    switch (SystemAction)
    {
        /* Reset */
        case PowerActionShutdownReset:

            /* Try platform driver first, then legacy */
            //PopInvokeSystemStateHandler(PowerStateShutdownReset, NULL);
            PopSetSystemPowerState(PowerSystemShutdown, SystemAction);
            HalReturnToFirmware(HalRebootRoutine);
            break;

        case PowerActionShutdown:

            /* Check for group policy that says to use "it is now safe" screen */
            if (PopShutdownPowerOffPolicy)
            {
                /* FIXFIX: Switch to legacy shutdown handler */
                //PopPowerStateHandlers[PowerStateShutdownOff].Handler = PopShutdownHandler;
            }

        case PowerActionShutdownOff:

            /* Call shutdown handler */
            //PopInvokeSystemStateHandler(PowerStateShutdownOff, NULL);

            /* ReactOS Hack */
            PopSetSystemPowerState(PowerSystemShutdown, SystemAction);
            PopShutdownHandler();

            /* If that didn't work, call the HAL */
            HalReturnToFirmware(HalPowerDownRoutine);
            break;

        default:
            break;
    }

    /* Anything else should not happen */
    KeBugCheckEx(INTERNAL_POWER_ERROR, 5, 0, 0, 0);
}

VOID
NTAPI
PopGracefulShutdown(IN PVOID Context)
{
    PEPROCESS Process = NULL;

    /* Process the registered waits and work items */
    PopProcessShutDownLists();

    /* Loop every process */
    Process = PsGetNextProcess(Process);
    while (Process)
    {
        /* Make sure this isn't the idle or initial process */
        if ((Process != PsInitialSystemProcess) && (Process != PsIdleProcess))
        {
            /* Print it */
            DPRINT1("%15s is still RUNNING (%p)\n", Process->ImageFileName, Process->UniqueProcessId);
        }

        /* Get the next process */
        Process = PsGetNextProcess(Process);
    }

    /* First, the HAL handles any "end of boot" special functionality */
    DPRINT("HAL shutting down\n");
    HalEndOfBoot();

    /* In this step, the I/O manager does first-chance shutdown notification */
    DPRINT("I/O manager shutting down in phase 0\n");
    IoShutdownSystem(0);

    /* In this step, all workers are killed and hives are flushed */
    DPRINT("Configuration Manager shutting down\n");
    CmShutdownSystem();

    /* Note that modified pages should be written here (MiShutdownSystem) */
#ifdef NEWCC
	/* Flush all user files before we start shutting down IO */
	/* This is where modified pages are written back by the IO manager */
	CcShutdownSystem();
#endif

    /* In this step, the I/O manager does last-chance shutdown notification */
    DPRINT("I/O manager shutting down in phase 1\n");
    IoShutdownSystem(1);
    CcWaitForCurrentLazyWriterActivity();

    /* Note that here, we should broadcast the power IRP to devices */

    /* In this step, the HAL disables any wake timers */
    DPRINT("Disabling wake timers\n");
    HalSetWakeEnable(FALSE);

    /* And finally the power request is sent */
    DPRINT("Taking the system down\n");
    PopShutdownSystem(PopAction.Action);
}

VOID
NTAPI
PopReadShutdownPolicy(VOID)
{
    UNICODE_STRING KeyString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE KeyHandle;
    ULONG Length;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION Info = (PVOID)Buffer;

    /* Setup object attributes */
    RtlInitUnicodeString(&KeyString,
                         L"\\Registry\\Machine\\Software\\Policies\\Microsoft\\Windows NT");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Open the key */
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Open the policy value and query it */
        RtlInitUnicodeString(&KeyString, L"DontPowerOffAfterShutdown");
        Status = ZwQueryValueKey(KeyHandle,
                                 &KeyString,
                                 KeyValuePartialInformation,
                                 &Info,
                                 sizeof(Info),
                                 &Length);
        if ((NT_SUCCESS(Status)) && (Info->Type == REG_DWORD))
        {
            /* Read the policy */
            PopShutdownPowerOffPolicy = *Info->Data == 1;
        }

        /* Close the key */
        ZwClose(KeyHandle);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
PoQueueShutdownWorkItem(
    _In_ PWORK_QUEUE_ITEM WorkItem)
{
    NTSTATUS Status;

    /* Acquire the shutdown list lock */
    KeAcquireGuardedMutex(&PopShutdownListMutex);

    /* Check if the list is (already/still) available */
    if (PopShutdownListAvailable)
    {
        /* Insert the item into the list */
        InsertTailList(&PopShutdownQueue, &WorkItem->List);
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* We are already in shutdown */
        Status = STATUS_SYSTEM_SHUTDOWN;
    }

    /* Release the list lock */
    KeReleaseGuardedMutex(&PopShutdownListMutex);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PoRequestShutdownEvent(OUT PVOID *Event)
{
    NTSTATUS Status;
    PAGED_CODE();

    /* Initialize to NULL */
    if (Event) *Event = NULL;

    /* Request a shutdown wait */
    Status = PoRequestShutdownWait(PsGetCurrentThread());
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Return the global shutdown event */
    if (Event) *Event = &PopShutdownEvent;
    return STATUS_SUCCESS;
}

