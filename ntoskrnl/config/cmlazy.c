/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL - See COPYING in the top level directory
 * PURPOSE:     Configuration Manager - Internal Registry APIs
 * PROGRAMMERS: Alex Ionescu <alex.ionescu@reactos.org>
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
CmpDoFlushNextHive(_In_  BOOLEAN ForceFlush,
                   _Out_ PBOOLEAN Error,
                   _Out_ PULONG DirtyCount)
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

    /* Acquire the list lock and loop */
    ExAcquirePushLockShared(&CmpHiveListHeadLock);
    NextEntry = CmpHiveListHead.Flink;
    while ((NextEntry != &CmpHiveListHead) && HiveCount)
    {
        /* Get the hive and check if we should flush it */
        CmHive = CONTAINING_RECORD(NextEntry, CMHIVE, HiveList);
        if (!(CmHive->Hive.HiveFlags & HIVE_NOLAZYFLUSH) &&
            (CmHive->FlushCount != CmpLazyFlushCount))
        {
            /* Great success! */
            Result = TRUE;

            /* One less to flush */
            HiveCount--;

            /* Ignore clean or volatile hives */
            if ((!CmHive->Hive.DirtyCount && !ForceFlush) ||
                (CmHive->Hive.HiveFlags & HIVE_VOLATILE))
            {
                /* Don't do anything but do update the count */
                CmHive->FlushCount = CmpLazyFlushCount;
                DPRINT("Hive %wZ is clean.\n", &CmHive->FileFullPath);
            }
            else
            {
                /* Do the sync */
                DPRINT("Flushing: %wZ\n", &CmHive->FileFullPath);
                DPRINT("Handle: %p\n", CmHive->FileHandles[HFILE_TYPE_PRIMARY]);
                Status = HvSyncHive(&CmHive->Hive);
                if (!NT_SUCCESS(Status))
                {
                    /* Let them know we failed */
                    DPRINT1("Failed to flush %wZ on handle %p (status 0x%08lx)\n",
                        &CmHive->FileFullPath,  CmHive->FileHandles[HFILE_TYPE_PRIMARY], Status);
                    *Error = TRUE;
                    Result = FALSE;
                    break;
                }
                CmHive->FlushCount = CmpLazyFlushCount;
            }
        }
        else if (CmHive->Hive.DirtyCount &&
                 !(CmHive->Hive.HiveFlags & HIVE_VOLATILE) &&
                 !(CmHive->Hive.HiveFlags & HIVE_NOLAZYFLUSH))
        {
            /* Use another lazy flusher for this hive */
            ASSERT(CmHive->FlushCount == CmpLazyFlushCount);
            *DirtyCount += CmHive->Hive.DirtyCount;
            DPRINT("CmHive %wZ already uptodate.\n", &CmHive->FileFullPath);
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

_Function_class_(KDEFERRED_ROUTINE)
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

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
CmpLazyFlushDpcRoutine(IN PKDPC Dpc,
                       IN PVOID DeferredContext,
                       IN PVOID SystemArgument1,
                       IN PVOID SystemArgument2)
{
    /* Check if we should queue the lazy flush worker */
    DPRINT("Flush pending: %s, Holding lazy flush: %s.\n", CmpLazyFlushPending ? "yes" : "no", CmpHoldLazyFlush ? "yes" : "no");
    if (!CmpLazyFlushPending && !CmpHoldLazyFlush)
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
    if (!CmpNoWrite && !CmpHoldLazyFlush)
    {
        /* Do it */
        DueTime.QuadPart = Int32x32To64(CmpLazyFlushIntervalInSeconds,
                                        -10 * 1000 * 1000);
        KeSetTimer(&CmpLazyFlushTimer, DueTime, &CmpLazyFlushDpc);
    }
}

_Function_class_(WORKER_THREAD_ROUTINE)
VOID
NTAPI
CmpLazyFlushWorker(IN PVOID Parameter)
{
    BOOLEAN ForceFlush, Result, MoreWork = FALSE;
    ULONG DirtyCount = 0;
    PAGED_CODE();

    /* Don't do anything if lazy flushing isn't enabled yet */
    if (CmpHoldLazyFlush)
    {
        DPRINT1("Lazy flush held. Bye bye.\n");
        CmpLazyFlushPending = FALSE;
        return;
    }

    /* Check if we are forcing a flush */
    ForceFlush = CmpForceForceFlush;
    if (ForceFlush)
    {
        DPRINT("Forcing flush.\n");
        /* Lock the registry exclusively */
        CmpLockRegistryExclusive();
    }
    else
    {
        DPRINT("Not forcing flush.\n");
        /* Starve writers before locking */
        InterlockedIncrement(&CmpFlushStarveWriters);
        CmpLockRegistry();
    }

    /* Flush the next hive */
    MoreWork = CmpDoFlushNextHive(ForceFlush, &Result, &DirtyCount);
    if (!MoreWork)
    {
        /* We're done */
        InterlockedIncrement((PLONG)&CmpLazyFlushCount);
    }

    /* Check if we have starved writers */
    if (!ForceFlush)
        InterlockedDecrement(&CmpFlushStarveWriters);

    /* Not pending anymore, release the registry lock */
    CmpLazyFlushPending = FALSE;
    CmpUnlockRegistry();

    DPRINT("Lazy flush done. More work to be done: %s. Entries still dirty: %u.\n",
        MoreWork ? "Yes" : "No", DirtyCount);

    if (MoreWork)
    {
        /* Relaunch the flush timer, so the remaining hives get flushed */
        CmpLazyFlush();
    }
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

    /* Setup the system hives list if this is not a Setup boot */
    if (!SetupBoot)
        CmpInitializeHiveList();

    /* Now that the system hives are loaded, if we are in PE mode,
     * all other hives will be loaded with full access */
    if (CmpMiniNTBoot)
        CmpShareSystemHives = FALSE;
}

NTSTATUS
NTAPI
CmpCmdHiveOpen(IN POBJECT_ATTRIBUTES FileAttributes,
               IN PSECURITY_CLIENT_CONTEXT ImpersonationContext,
               IN OUT PBOOLEAN Allocate,
               OUT PCMHIVE *NewHive,
               IN ULONG CheckFlags)
{
    NTSTATUS Status;
    UNICODE_STRING FileName;
    PWCHAR FilePath;
    ULONG Length;
    OBJECT_NAME_INFORMATION DummyNameInfo;
    POBJECT_NAME_INFORMATION FileNameInfo;

    PAGED_CODE();

    if (FileAttributes->RootDirectory)
    {
        /*
         * Validity check: The ObjectName is relative to RootDirectory,
         * therefore it must not start with a path separator.
         */
        if (FileAttributes->ObjectName && FileAttributes->ObjectName->Buffer &&
            FileAttributes->ObjectName->Length >= sizeof(WCHAR) &&
            *FileAttributes->ObjectName->Buffer == OBJ_NAME_PATH_SEPARATOR)
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }

        /* Determine the right buffer size and allocate */
        Status = ZwQueryObject(FileAttributes->RootDirectory,
                               ObjectNameInformation,
                               &DummyNameInfo,
                               sizeof(DummyNameInfo),
                               &Length);
        if (Status != STATUS_BUFFER_OVERFLOW)
        {
            DPRINT1("CmpCmdHiveOpen(): Root directory handle object name size query failed, Status = 0x%08lx\n", Status);
            return Status;
        }

        FileNameInfo = ExAllocatePoolWithTag(PagedPool,
                                             Length + sizeof(UNICODE_NULL),
                                             TAG_CM);
        if (FileNameInfo == NULL)
        {
            DPRINT1("CmpCmdHiveOpen(): Unable to allocate memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Try to get the value */
        Status = ZwQueryObject(FileAttributes->RootDirectory,
                               ObjectNameInformation,
                               FileNameInfo,
                               Length,
                               &Length);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            DPRINT1("CmpCmdHiveOpen(): Root directory handle object name query failed, Status = 0x%08lx\n", Status);
            ExFreePoolWithTag(FileNameInfo, TAG_CM);
            return Status;
        }

        /* Null-terminate and add the length of the terminator */
        Length -= sizeof(OBJECT_NAME_INFORMATION);
        FilePath = FileNameInfo->Name.Buffer;
        FilePath[Length / sizeof(WCHAR)] = UNICODE_NULL;
        Length += sizeof(UNICODE_NULL);

        /* Compute the size of the full path; Length already counts the terminating NULL */
        Length = Length + sizeof(WCHAR) + FileAttributes->ObjectName->Length;
        if (Length > MAXUSHORT)
        {
            /* Name size too long, bail out */
            ExFreePoolWithTag(FileNameInfo, TAG_CM);
            return STATUS_OBJECT_PATH_INVALID;
        }

        /* Build the full path */
        RtlInitEmptyUnicodeString(&FileName, NULL, 0);
        FileName.Buffer = ExAllocatePoolWithTag(PagedPool, Length, TAG_CM);
        if (!FileName.Buffer)
        {
            /* Fail */
            DPRINT1("CmpCmdHiveOpen(): Unable to allocate memory\n");
            ExFreePoolWithTag(FileNameInfo, TAG_CM);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        FileName.MaximumLength = Length;
        RtlCopyUnicodeString(&FileName, &FileNameInfo->Name);
        ExFreePoolWithTag(FileNameInfo, TAG_CM);

        /*
         * Append a path terminator if needed (we have already accounted
         * for a possible extra one when allocating the buffer).
         */
        if (/* FileAttributes->ObjectName->Buffer[0] != OBJ_NAME_PATH_SEPARATOR && */ // We excluded ObjectName starting with a path separator above.
            FileName.Length > 0 && FileName.Buffer[FileName.Length / sizeof(WCHAR) - 1] != OBJ_NAME_PATH_SEPARATOR)
        {
            /* ObjectName does not start with '\' and PathBuffer does not end with '\' */
            FileName.Buffer[FileName.Length / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
            FileName.Length += sizeof(WCHAR);
            FileName.Buffer[FileName.Length / sizeof(WCHAR)] = UNICODE_NULL;
        }

        /* Append the object name */
        Status = RtlAppendUnicodeStringToString(&FileName, FileAttributes->ObjectName);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            DPRINT1("CmpCmdHiveOpen(): RtlAppendUnicodeStringToString() failed, Status = 0x%08lx\n", Status);
            ExFreePoolWithTag(FileName.Buffer, TAG_CM);
            return Status;
        }
    }
    else
    {
        FileName = *FileAttributes->ObjectName;
    }

    /* Open the file in the current security context */
    Status = CmpInitHiveFromFile(&FileName,
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
        ImpersonationContext)
    {
        /* We failed due to an account/security error, impersonate SYSTEM */
        Status = SeImpersonateClientEx(ImpersonationContext, NULL);
        if (NT_SUCCESS(Status))
        {
            /* Now try again */
            Status = CmpInitHiveFromFile(&FileName,
                                         0,
                                         NewHive,
                                         Allocate,
                                         CheckFlags);

            /* Restore impersonation token */
            PsRevertToSelf();
        }
    }

    if (FileAttributes->RootDirectory)
    {
        ExFreePoolWithTag(FileName.Buffer, TAG_CM);
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

VOID
NTAPI
CmSetLazyFlushState(IN BOOLEAN Enable)
{
    /* Set state for lazy flusher */
    CmpHoldLazyFlush = !Enable;
}

/* EOF */
