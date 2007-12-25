/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmapi.c
 * PURPOSE:         Configuration Manager - Internal Registry APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

KTIMER CmpLazyFlushTimer;
KDPC CmpLazyFlushDpc;
WORK_QUEUE_ITEM CmpLazyWorkItem;
KTIMER CmpEnableLazyFlushTimer;
KDPC CmpEnableLazyFlushDpc;
BOOLEAN CmpLazyFlushPending;
BOOLEAN CmpForceForceFlush;
BOOLEAN CmpHoldLazyFlush = TRUE;
ULONG CmpLazyFlushIntervalInSeconds = 5;
ULONG CmpLazyFlushHiveCount = 7;
ULONG CmpLazyFlushCount = 1;
LONG CmpFlushStarveWriters;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
CmpDoFlushNextHive(IN BOOLEAN ForceFlush,
                   OUT PBOOLEAN Error,
                   OUT PULONG DirtyCount)
{
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PCMHIVE CmHive;
    BOOLEAN Result;    
    ULONG HiveCount = CmpLazyFlushHiveCount;

    /* Set Defaults */
    *Error = FALSE;
    *DirtyCount = 0;
    
    /* Don't do anything if we're not supposed to */
    if (CmpNoWrite) return TRUE;

    /* Make sure we have to flush at least one hive */
    if (!HiveCount) HiveCount = 1;

    /* Don't force flush */
    CmpForceForceFlush = FALSE;

    /* Acquire the list lock and loop */
    ExAcquirePushLockShared(&CmpHiveListHeadLock);
    NextEntry = CmpHiveListHead.Flink;
    while (NextEntry != &CmpHiveListHead)
    {
        /* Get the hive and check if we should flush it */
        CmHive = CONTAINING_RECORD(NextEntry, CMHIVE, HiveList);
        if (!(CmHive->Hive.HiveFlags & HIVE_NOLAZYFLUSH) &&
            (CmHive->FlushCount != CmpLazyFlushCount))
        {
            /* Great sucess! */
            Result = TRUE;
            
            /* Ignore clean or volatile hves */
            if (!(CmHive->Hive.DirtyCount) ||
                (CmHive->Hive.HiveFlags & HIVE_VOLATILE))
            {
                /* Don't do anything but do update the count */
                CmHive->FlushCount = CmpLazyFlushCount;
            }
            else
            {
                /* Do the sync */
                DPRINT1("Flushing: %wZ\n", CmHive->FileFullPath);
                DPRINT1("Handle: %lx\n", CmHive->FileHandles[HFILE_TYPE_PRIMARY]);
                Status = HvSyncHive(&CmHive->Hive);
                if(!NT_SUCCESS(Status))
                {
                    /* Let them know we failed */
                    *Error = TRUE;
                    Result = FALSE;
                }
            }
        }
        else if ((CmHive->Hive.DirtyCount) &&
                 (!(CmHive->Hive.HiveFlags & HIVE_VOLATILE)) &&
                 (!(CmHive->Hive.HiveFlags & HIVE_NOLAZYFLUSH)))
        {
            /* Use another lazy flusher for this hive */
            ASSERT(CmHive->FlushCount == CmpLazyFlushCount);
            *DirtyCount += CmHive->Hive.DirtyCount;
        }

        /* Try the next one */
        NextEntry = NextEntry->Flink;
    }
    
    /* Check if we've flushed everything */
    if (NextEntry == &CmpHiveListHead)
    {
        /* We have, tell the caller we're done */
        Result = FALSE;
    }
    else
    {
        /* We need to be called again */
        Result = TRUE;
    }
    
    /* Unlock the list and return the result */
    ExReleasePushLock(&CmpHiveListHeadLock);
    return Result;
}

VOID
NTAPI
CmpEnableLazyFlushDpcRoutine(IN PKDPC Dpc,
                             IN PVOID DeferredContext,
                             IN PVOID SystemArgument1,
                             IN PVOID SystemArgument2)
{
    /* Don't stop lazy flushing from happening anymore */
    CmpHoldLazyFlush = FALSE;
}

VOID
NTAPI
CmpLazyFlushDpcRoutine(IN PKDPC Dpc,
                       IN PVOID DeferredContext,
                       IN PVOID SystemArgument1,
                       IN PVOID SystemArgument2)
{
    /* Check if we should queue the lazy flush worker */
    if ((!CmpLazyFlushPending) && (!CmpHoldLazyFlush))
    {
        CmpLazyFlushPending = TRUE;
        ExQueueWorkItem(&CmpLazyWorkItem, DelayedWorkQueue);
    }
}

VOID
NTAPI
CmpLazyFlush(VOID)
{
    LARGE_INTEGER DueTime;
    PAGED_CODE();
    
    /* Check if we should set the lazy flush timer */
    if ((!CmpNoWrite) && (!CmpHoldLazyFlush))
    {
        /* Do it */
        DueTime.QuadPart = Int32x32To64(CmpLazyFlushIntervalInSeconds,
                                        -10 * 1000 * 1000);
        KeSetTimer(&CmpLazyFlushTimer, DueTime, &CmpLazyFlushDpc);
    }
}

VOID
NTAPI
CmpLazyFlushWorker(IN PVOID Parameter)
{
    BOOLEAN ForceFlush, Result, MoreWork = FALSE;
    ULONG DirtyCount = 0;
    PAGED_CODE();

    /* Don't do anything if lazy flushing isn't enabled yet */
    if (CmpHoldLazyFlush) return;
    
    /* Check if we are forcing a flush */
    ForceFlush = CmpForceForceFlush;
    if (ForceFlush)
    {
        /* Lock the registry exclusively */
        CmpLockRegistryExclusive();
    }
    else
    {
        /* Do a normal lock */
        CmpLockRegistry();
        InterlockedIncrement(&CmpFlushStarveWriters);
    }
    
    /* Flush the next hive */
    MoreWork = CmpDoFlushNextHive(ForceFlush, &Result, &DirtyCount);
    if (!MoreWork)
    {
        /* We're done */
        InterlockedIncrement((PLONG)&CmpLazyFlushCount);
    }

    /* Check if we have starved writers */
    if (!ForceFlush) InterlockedDecrement(&CmpFlushStarveWriters);

    /* Not pending anymore, release the registry lock */
    CmpLazyFlushPending = FALSE;
    CmpUnlockRegistry();
    
    /* Check if we need to flush another hive */
    if ((MoreWork) || (DirtyCount)) CmpLazyFlush();
}

VOID
NTAPI
CmpCmdInit(IN BOOLEAN SetupBoot)
{
    LARGE_INTEGER DueTime;  
    PAGED_CODE();
    
    /* Setup the lazy DPC */
    KeInitializeDpc(&CmpLazyFlushDpc, CmpLazyFlushDpcRoutine, NULL);
    
    /* Setup the lazy timer */
    KeInitializeTimer(&CmpLazyFlushTimer);
    
    /* Setup the lazy worker */
    ExInitializeWorkItem(&CmpLazyWorkItem, CmpLazyFlushWorker, NULL);

    /* Setup the forced-lazy DPC and timer */
    KeInitializeDpc(&CmpEnableLazyFlushDpc,
                    CmpEnableLazyFlushDpcRoutine,
                    NULL);
    KeInitializeTimer(&CmpEnableLazyFlushTimer);
    
    /* Enable lazy flushing after 10 minutes */
    DueTime.QuadPart = Int32x32To64(600, -10 * 1000 * 1000);
    KeSetTimer(&CmpEnableLazyFlushTimer, DueTime, &CmpEnableLazyFlushDpc);

    /* Setup flush variables */
    CmpNoWrite = CmpMiniNTBoot;
    CmpWasSetupBoot = SetupBoot;
    
    /* Testing: Force Lazy Flushing */
    CmpHoldLazyFlush = FALSE;
    
    /* Setup the hive list */
    CmpInitializeHiveList(SetupBoot);
}

NTSTATUS
NTAPI
CmpCmdHiveOpen(IN POBJECT_ATTRIBUTES FileAttributes,
               IN PSECURITY_CLIENT_CONTEXT ImpersonationContext,
               IN OUT PBOOLEAN Allocate,
               OUT PCMHIVE *NewHive,
               IN ULONG CheckFlags)
{
    PUNICODE_STRING FileName;
    NTSTATUS Status;
    PAGED_CODE();

    /* Open the file in the current security context */
    FileName = FileAttributes->ObjectName;
    Status = CmpInitHiveFromFile(FileName,
                                 0,
                                 NewHive,
                                 Allocate,
                                 CheckFlags);
    if (((Status == STATUS_ACCESS_DENIED) ||
         (Status == STATUS_NO_SUCH_USER) ||
         (Status == STATUS_WRONG_PASSWORD) ||
         (Status == STATUS_ACCOUNT_EXPIRED) ||
         (Status == STATUS_ACCOUNT_DISABLED) ||
         (Status == STATUS_ACCOUNT_RESTRICTION)) &&
        (ImpersonationContext))
    {
        /* We failed due to an account/security error, impersonate SYSTEM */
        Status = SeImpersonateClientEx(ImpersonationContext, NULL);
        if (NT_SUCCESS(Status))
        {
            /* Now try again */
            Status = CmpInitHiveFromFile(FileName,
                                         0,
                                         NewHive,
                                         Allocate,
                                         CheckFlags);

            /* Restore impersonation token */
            PsRevertToSelf();
        }
    }

    /* Return status of open attempt */
    return Status;
}

VOID
NTAPI
CmpShutdownWorkers(VOID)
{
    /* Stop lazy flushing */
    PAGED_CODE();
    KeCancelTimer(&CmpLazyFlushTimer);
}

/* EOF */
