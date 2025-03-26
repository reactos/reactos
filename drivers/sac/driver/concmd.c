/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/concmd.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

#include <ndk/exfuncs.h>

/* GLOBALS ********************************************************************/

PVOID GlobalBuffer;
ULONG GlobalBufferSize;

/* FUNCTIONS ******************************************************************/

NTSTATUS
DoChannelListCommand(
    VOID
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DoChannelCloseByNameCommand(
    IN PCHAR Count
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DoChannelCloseByIndexCommand(
    IN ULONG ChannelIndex
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DoChannelSwitchByNameCommand(
    IN PCHAR Count
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DoChannelSwitchByIndexCommand(
    IN ULONG ChannelIndex
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

typedef struct _SAC_SYSTEM_INFORMATION
{
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_TIMEOFDAY_INFORMATION TimeInfo;
    SYSTEM_FILECACHE_INFORMATION CacheInfo;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    ULONG RemainingSize;
    ULONG ProcessDataOffset;
    // SYSTEM_PAGEFILE_INFORMATION PageFileInfo;
    // SYSTEM_PROCESS_INFORMATION ProcessInfo;
} SAC_SYSTEM_INFORMATION, *PSAC_SYSTEM_INFORMATION;

NTSTATUS
NTAPI
GetTListInfo(IN PSAC_SYSTEM_INFORMATION SacInfo,
             IN ULONG InputSize,
             OUT PULONG TotalSize)
{
    NTSTATUS Status;
    ULONG BufferLength, ReturnLength, RemainingSize;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    ULONG_PTR P;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering.\n");

    /* Assume failure */
    *TotalSize = 0;

    /* Bail out if the buffer is way too small */
    if (InputSize < 4)
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, no memory.\n");
        return STATUS_NO_MEMORY;
    }

    /* Make sure it's at least big enough to hold the static structure */
    BufferLength = InputSize - sizeof(SAC_SYSTEM_INFORMATION);
    if (InputSize < sizeof(SAC_SYSTEM_INFORMATION))
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, no memory (2).\n");
        return STATUS_NO_MEMORY;
    }

    /* Query the time */
    Status = ZwQuerySystemInformation(SystemTimeOfDayInformation,
                                      &SacInfo->TimeInfo,
                                      sizeof(SacInfo->TimeInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, error.\n");
        return Status;
    }

    /* Query basic information */
    Status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &SacInfo->BasicInfo,
                                      sizeof(SacInfo->BasicInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, error (2).\n");
        return Status;
    }

    /* Now query the pagefile information, which comes right after */
    P = (ULONG_PTR)(SacInfo + 1);
    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)P;
    Status = ZwQuerySystemInformation(SystemPageFileInformation,
                                      PageFileInfo,
                                      BufferLength,
                                      &ReturnLength);
    if (!NT_SUCCESS(Status) || !(ReturnLength))
    {
        /* We failed -- is it because our buffer was too small? */
        if (BufferLength < ReturnLength)
        {
            /* Bail out */
            SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, no memory(5).\n");
            return STATUS_NO_MEMORY;
        }

        /* Some other reason, assume the buffer is now full */
        SacInfo->RemainingSize = 0;
    }
    else
    {
        /* This is the leftover data */
        SacInfo->RemainingSize = InputSize - BufferLength;

        /* This much has now been consumed, and where we are now */
        BufferLength -= ReturnLength;
        P += ReturnLength;

        /* Are we out of memory? */
        if ((LONG)BufferLength < 0)
        {
            /* Bail out */
            SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, no memory(3).\n");
            return STATUS_NO_MEMORY;
        }

        /* All good, loop the pagefile data now */
        while (TRUE)
        {
            /* Is the pagefile name too big to fit? */
            if (PageFileInfo->PageFileName.Length > (LONG)BufferLength)
            {
                /* Bail out */
                SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, error(3).\n");
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            /* Copy the name into our own buffer */
            RtlCopyMemory((PVOID)P,
                          PageFileInfo->PageFileName.Buffer,
                          PageFileInfo->PageFileName.Length);
            PageFileInfo->PageFileName.Buffer = (PWCHAR)P;

            /* Update buffer lengths and offset */
            BufferLength -= PageFileInfo->PageFileName.Length;
            P += PageFileInfo->PageFileName.Length;

            /* Are we out of memory? */
            if ((LONG)BufferLength < 0)
            {
                /* Bail out */
                SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, no memory(4).\n");
                return STATUS_NO_MEMORY;
            }

            /* If this was the only pagefile, break out */
            if (!PageFileInfo->NextEntryOffset) break;

            /* Otherwise, move to the next one */
            PageFileInfo = (PVOID)((ULONG_PTR)PageFileInfo +
                                    PageFileInfo->NextEntryOffset);
        }
    }

    /* Next, query the file cache information */
    Status = ZwQuerySystemInformation(SystemFileCacheInformation,
                                      &SacInfo->CacheInfo,
                                      sizeof(SacInfo->CacheInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, error (4).\n");
        return Status;
    }

    /* And then the performance information */
    Status = ZwQuerySystemInformation(SystemPerformanceInformation,
                                      &SacInfo->PerfInfo,
                                      sizeof(SacInfo->PerfInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, error(5).\n");
        return Status;
    }

    /* Finally, align the buffer to query process and thread information */
    P = ALIGN_UP(P, SYSTEM_PROCESS_INFORMATION);
    RemainingSize = (ULONG_PTR)SacInfo + InputSize - P;

    /* Are we out of memory? */
    if ((LONG)RemainingSize < 0)
    {
        /* Bail out */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, no memory (6).\n");
        return STATUS_NO_MEMORY;
    }

    /* Now query the processes and threads */
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)P;
    Status = ZwQuerySystemInformation(SystemProcessInformation,
                                      ProcessInfo,
                                      RemainingSize,
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, error(6).\n");
        return Status;
    }

    /* The first process name will be right after this buffer */
    P += ReturnLength;

    /* The caller should look for process info over here */
    SacInfo->ProcessDataOffset = InputSize - RemainingSize;

    /* This is how much buffer data we have left -- are we out? */
    BufferLength = RemainingSize - ReturnLength;
    if ((LONG)BufferLength < 0)
    {
        /* Bail out */
        SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, no memory(7).\n");
        return STATUS_NO_MEMORY;
    }

    /* All good and ready to parse the process and thread list */
    while (TRUE)
    {
        /* Does the process have a name? */
        if (ProcessInfo->ImageName.Buffer)
        {
            /* Is the process name too big to fit? */
            if ((LONG)BufferLength < ProcessInfo->ImageName.Length)
            {
                /* Bail out */
                SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, error(7).\n");
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            /* Copy the name into our own buffer */
            RtlCopyMemory((PVOID)P,
                          ProcessInfo->ImageName.Buffer,
                          ProcessInfo->ImageName.Length);
            ProcessInfo->ImageName.Buffer = (PWCHAR)P;

            /* Update buffer lengths and offset */
            BufferLength -= ProcessInfo->ImageName.Length;
            P += ProcessInfo->ImageName.Length;

            /* Are we out of memory? */
            if ((LONG)BufferLength < 0)
            {
                /* Bail out */
                SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting, no memory(8).\n");
                return STATUS_NO_MEMORY;
            }
        }

        /* If this was the only process, break out */
        if (!ProcessInfo->NextEntryOffset) break;

        /* Otherwise, move to the next one */
        ProcessInfo = (PVOID)((ULONG_PTR)ProcessInfo +
                               ProcessInfo->NextEntryOffset);
    }

    /* All done! */
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting.\n");
    *TotalSize = InputSize - BufferLength;
    return STATUS_SUCCESS;
}

VOID
NTAPI
PrintTListInfo(IN PSAC_SYSTEM_INFORMATION SacInfo)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Testing: %d %d %I64d\n",
            SacInfo->BasicInfo.NumberOfPhysicalPages,
            SacInfo->PerfInfo.AvailablePages,
            SacInfo->TimeInfo.BootTime);
}

VOID
NTAPI
PutMore(OUT PBOOLEAN ScreenFull)
{
    *ScreenFull = FALSE;
}

BOOLEAN
RetrieveIpAddressFromString(
    IN PWCHAR IpString,
    OUT PULONG IpAddress
    )
{
    return FALSE;
}

NTSTATUS
CallQueryIPIOCTL(
    IN HANDLE DriverHandle,
    IN PVOID DriverObject,
    IN HANDLE WaitEvent,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    IN BOOLEAN PrintMessage,
    OUT PBOOLEAN MessagePrinted
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
DoRebootCommand(IN BOOLEAN Reboot)
{
    LARGE_INTEGER Timeout, TickCount;
    NTSTATUS Status;
    KEVENT Event;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoRebootCommand: Entering.\n");

    /* Get the current time now, and setup a timeout in 1 second */
    KeQueryTickCount(&TickCount);
    Timeout.QuadPart = TickCount.QuadPart / (10000000 / KeQueryTimeIncrement());

    /* Check if the timeout is small enough */
    if (Timeout.QuadPart < 60 )
    {
        /* Show the prompt */
        ConMgrSimpleEventMessage(Reboot ?
                                 SAC_RESTART_PROMPT : SAC_SHUTDOWN_PROMPT,
                                 TRUE);

        /* Do the wait */
        KeInitializeEvent(&Event, SynchronizationEvent, 0);
        Timeout.QuadPart = -10000000 * (60 - Timeout.LowPart);
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &Timeout);
    }

    /* Do a shutdown or a reboot, based on the request */
    Status = NtShutdownSystem(Reboot ? ShutdownReboot : ShutdownPowerOff);

    /* Check if anyone in the command channel already allocated this */
    if (!GlobalBuffer)
    {
        /* Allocate it */
        GlobalBuffer = SacAllocatePool(PAGE_SIZE, GLOBAL_BLOCK_TAG);
        if (!GlobalBuffer)
        {
            /* We need the global buffer, bail out without it*/
            SacPutSimpleMessage(SAC_OUT_OF_MEMORY_PROMPT);
            SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoRebootCommand: Exiting (1).\n");
            return;
        }

        /* Set the size of the buffer */
        GlobalBufferSize = PAGE_SIZE;
    }

    /* We came back from a reboot, this doesn't make sense, tell the user */
    SacPutSimpleMessage(Reboot ? SAC_RESTART_FAIL_PROMPT : SAC_SHUTDOWN_FAIL_PROMPT);
    swprintf(GlobalBuffer, GetMessage(SAC_FAIL_PROMPT), Status);
    SacPutString(GlobalBuffer);
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoRebootCommand: Exiting.\n");
}

VOID
NTAPI
DoFullInfoCommand(VOID)
{
    /* Flip the flag */
    GlobalDoThreads = !GlobalDoThreads;

    /* Print out the new state */
    SacPutSimpleMessage(GlobalDoThreads ? 8 : 7);
}

VOID
NTAPI
DoPagingCommand(VOID)
{
    /* Flip the flag */
    GlobalPagingNeeded = !GlobalPagingNeeded;

    /* Print out the new state */
    SacPutSimpleMessage(GlobalPagingNeeded ? 10 : 9);
}

VOID
NTAPI
DoSetTimeCommand(IN PCHAR InputTime)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoKillCommand(IN PCHAR KillString)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoLowerPriorityCommand(IN PCHAR PrioString)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoRaisePriorityCommand(IN PCHAR PrioString)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoLimitMemoryCommand(IN PCHAR LimitString)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoCrashCommand(VOID)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoCrashCommand: Entering.\n");

    /* Crash the machine */
    KeBugCheckEx(MANUALLY_INITIATED_CRASH, 0, 0, 0, 0);
    __debugbreak();
}

VOID
NTAPI
DoMachineInformationCommand(VOID)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoChannelCommand(IN PCHAR ChannelString)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoCmdCommand(IN PCHAR InputString)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoLockCommand(VOID)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

FORCEINLINE
BOOLEAN
PrintHelpMessage(IN ULONG MessageId,
                 IN OUT PULONG Count)
{
    BOOLEAN ScreenFull;
    ULONG NewCount;

    /* Get the amount of lines this message will take */
    NewCount = GetMessageLineCount(MessageId);
    if ((NewCount + *Count) > SAC_VTUTF8_ROW_HEIGHT)
    {
        /* We are going to overflow the screen, wait for input */
        PutMore(&ScreenFull);
        if (ScreenFull) return FALSE;
        *Count = 0;
    }

    /* Print out the message and update the amount of lines printed */
    SacPutSimpleMessage(MessageId);
    *Count += NewCount;
    return TRUE;
}

VOID
NTAPI
DoHelpCommand(VOID)
{
    ULONG Count = 0;

    /* Print out all the help messages */
    if (!PrintHelpMessage(112, &Count)) return;
    if (!PrintHelpMessage(12, &Count)) return;
    if (!PrintHelpMessage(13, &Count)) return;
    if (!PrintHelpMessage(14, &Count)) return;
    if (!PrintHelpMessage(15, &Count)) return;
    if (!PrintHelpMessage(16, &Count)) return;
    if (!PrintHelpMessage(31, &Count)) return;
    if (!PrintHelpMessage(18, &Count)) return;
    if (!PrintHelpMessage(19, &Count)) return;
    if (!PrintHelpMessage(32, &Count)) return;
    if (!PrintHelpMessage(20, &Count)) return;
    if (!PrintHelpMessage(21, &Count)) return;
    if (!PrintHelpMessage(22, &Count)) return;
    if (!PrintHelpMessage(23, &Count)) return;
    if (!PrintHelpMessage(24, &Count)) return;
    if (!PrintHelpMessage(25, &Count)) return;
    if (!PrintHelpMessage(27, &Count)) return;
    if (!PrintHelpMessage(28, &Count)) return;
    if (!PrintHelpMessage(29, &Count)) return;
}

VOID
NTAPI
DoGetNetInfo(IN BOOLEAN DoPrint)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoSetIpAddressCommand(IN PCHAR IpString)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering\n");
}

VOID
NTAPI
DoTlistCommand(VOID)
{
    NTSTATUS Status;
    PVOID NewGlobalBuffer;
    ULONG Size;
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoTlistCommand: Entering.\n");

    /* Check if a global buffer already exists */
    if (!GlobalBuffer)
    {
        /* It doesn't, allocate one */
        GlobalBuffer = SacAllocatePool(4096, GLOBAL_BLOCK_TAG);
        if (GlobalBuffer)
        {
            /* Remember its current size */
            GlobalBufferSize = 4096;
        }
        else
        {
            /* Out of memory, bail out */
            SacPutSimpleMessage(11);
            SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoTlistCommand: Exiting.\n");
            return;
        }
    }

    /* Loop as long as the buffer is too small */
    while (TRUE)
    {
        /* Get the process list */
        Status = GetTListInfo(GlobalBuffer, GlobalBufferSize, &Size);
        if ((Status != STATUS_NO_MEMORY) &&
            (Status != STATUS_INFO_LENGTH_MISMATCH))
        {
            /* It fits! Bail out */
            break;
        }

        /* We need a new bigger buffer */
        NewGlobalBuffer = SacAllocatePool(GlobalBufferSize + 4096,
                                          GLOBAL_BLOCK_TAG);
        if (!NewGlobalBuffer)
        {
            /* Out of memory, bail out */
            SacPutSimpleMessage(11);
            SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoTlistCommand: Exiting.\n");
            return;
        }

        /* Free the old one, update state */
        SacFreePool(GlobalBuffer);
        GlobalBufferSize += 4096;
        GlobalBuffer = NewGlobalBuffer;
    }

    /* Did we get here because we have the whole list? */
    if (!NT_SUCCESS(Status))
    {
        /* Nope, print out a failure message */
        SacPutSimpleMessage(68);
        swprintf(GlobalBuffer, GetMessage(48), Status);
        SacPutString(GlobalBuffer);
    }
    else
    {
        /* Yep, print out the list */
        PrintTListInfo(GlobalBuffer);
    }

    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoTlistCommand: Exiting.\n");
}
