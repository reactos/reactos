/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/fs.c
 * PURPOSE:         Implements cache managers functions useful for File Systems
 *
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#ifndef VACB_MAPPING_GRANULARITY
#define VACB_MAPPING_GRANULARITY (256 * 1024)
#endif

/* GLOBALS   *****************************************************************/

extern KGUARDED_MUTEX ViewLock;
extern ULONG DirtyPageCount;

NTSTATUS CcRosInternalFreeVacb(PROS_VACB Vacb);

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
LARGE_INTEGER
NTAPI
CcGetDirtyPages (
    IN PVOID LogHandle,
    IN PDIRTY_PAGE_ROUTINE DirtyPageRoutine,
    IN PVOID Context1,
    IN PVOID Context2)
{
    LARGE_INTEGER i;

    CCTRACE(CC_API_DEBUG, "LogHandle=%p DirtyPageRoutine=%p Context1=%p Context2=%p\n",
        LogHandle, DirtyPageRoutine, Context1, Context2);

    UNIMPLEMENTED;
    i.QuadPart = 0;
    return i;
}

/*
 * @implemented
 */
PFILE_OBJECT
NTAPI
CcGetFileObjectFromBcb (
    IN PVOID Bcb)
{
    PINTERNAL_BCB iBcb = (PINTERNAL_BCB)Bcb;

    CCTRACE(CC_API_DEBUG, "Bcb=%p\n", Bcb);

    return iBcb->Vacb->SharedCacheMap->FileObject;
}

/*
 * @unimplemented
 */
LARGE_INTEGER
NTAPI
CcGetLsnForFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PLARGE_INTEGER OldestLsn OPTIONAL)
{
    LARGE_INTEGER i;

    CCTRACE(CC_API_DEBUG, "FileObject=%p\n", FileObject);

    UNIMPLEMENTED;
    i.QuadPart = 0;
    return i;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcInitializeCacheMap (
    IN PFILE_OBJECT FileObject,
    IN PCC_FILE_SIZES FileSizes,
    IN BOOLEAN PinAccess,
    IN PCACHE_MANAGER_CALLBACKS CallBacks,
    IN PVOID LazyWriterContext)
{
    ASSERT(FileObject);
    ASSERT(FileSizes);

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileSizes=%p PinAccess=%d CallBacks=%p LazyWriterContext=%p\n",
        FileObject, FileSizes, PinAccess, CallBacks, LazyWriterContext);

    /* Call old ROS cache init function */
    CcRosInitializeFileCache(FileObject,
                             FileSizes,
                             CallBacks,
                             LazyWriterContext);
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
CcIsThereDirtyData (
    IN PVPB Vpb)
{
    CCTRACE(CC_API_DEBUG, "Vpb=%p\n", Vpb);

    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
CcPurgeCacheSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN PLARGE_INTEGER FileOffset OPTIONAL,
    IN ULONG Length,
    IN BOOLEAN UninitializeCacheMaps)
{
    CCTRACE(CC_API_DEBUG, "SectionObjectPointer=%p\n FileOffset=%p Length=%lu UninitializeCacheMaps=%d",
        SectionObjectPointer, FileOffset, Length, UninitializeCacheMaps);

    //UNIMPLEMENTED;
    return FALSE;
}


/*
 * @implemented
 */
VOID NTAPI
CcSetFileSizes (
    IN PFILE_OBJECT FileObject,
    IN PCC_FILE_SIZES FileSizes)
{
    KIRQL oldirql;
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    PLIST_ENTRY current_entry;
    PROS_VACB current;
    LIST_ENTRY FreeListHead;
    NTSTATUS Status;

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileSizes=%p\n",
        FileObject, FileSizes);

    DPRINT("CcSetFileSizes(FileObject 0x%p, FileSizes 0x%p)\n",
           FileObject, FileSizes);
    DPRINT("AllocationSize %I64d, FileSize %I64d, ValidDataLength %I64d\n",
           FileSizes->AllocationSize.QuadPart,
           FileSizes->FileSize.QuadPart,
           FileSizes->ValidDataLength.QuadPart);

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    /*
     * It is valid to call this function on file objects that weren't
     * initialized for caching. In this case it's simple no-op.
     */
    if (SharedCacheMap == NULL)
        return;

    if (FileSizes->AllocationSize.QuadPart < SharedCacheMap->SectionSize.QuadPart)
    {
        InitializeListHead(&FreeListHead);
        KeAcquireGuardedMutex(&ViewLock);
        KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldirql);

        current_entry = SharedCacheMap->CacheMapVacbListHead.Flink;
        while (current_entry != &SharedCacheMap->CacheMapVacbListHead)
        {
            current = CONTAINING_RECORD(current_entry,
                                        ROS_VACB,
                                        CacheMapVacbListEntry);
            current_entry = current_entry->Flink;
            if (current->FileOffset.QuadPart >= FileSizes->AllocationSize.QuadPart)
            {
                if ((current->ReferenceCount == 0) || ((current->ReferenceCount == 1) && current->Dirty))
                {
                    RemoveEntryList(&current->CacheMapVacbListEntry);
                    RemoveEntryList(&current->VacbLruListEntry);
                    if (current->Dirty)
                    {
                        RemoveEntryList(&current->DirtyVacbListEntry);
                        DirtyPageCount -= VACB_MAPPING_GRANULARITY / PAGE_SIZE;
                    }
                    InsertHeadList(&FreeListHead, &current->CacheMapVacbListEntry);
                }
                else
                {
                    DPRINT1("Someone has referenced a VACB behind the new size.\n");
                    KeBugCheck(CACHE_MANAGER);
                }
            }
        }

        SharedCacheMap->SectionSize = FileSizes->AllocationSize;
        SharedCacheMap->FileSize = FileSizes->FileSize;
        KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldirql);
        KeReleaseGuardedMutex(&ViewLock);

        current_entry = FreeListHead.Flink;
        while(current_entry != &FreeListHead)
        {
            current = CONTAINING_RECORD(current_entry, ROS_VACB, CacheMapVacbListEntry);
            current_entry = current_entry->Flink;
            Status = CcRosInternalFreeVacb(current);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CcRosInternalFreeVacb failed, status = %x\n", Status);
                KeBugCheck(CACHE_MANAGER);
            }
        }
    }
    else
    {
        KeAcquireSpinLock(&SharedCacheMap->CacheMapLock, &oldirql);
        SharedCacheMap->SectionSize = FileSizes->AllocationSize;
        SharedCacheMap->FileSize = FileSizes->FileSize;
        KeReleaseSpinLock(&SharedCacheMap->CacheMapLock, oldirql);
    }
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcSetLogHandleForFile (
    IN PFILE_OBJECT FileObject,
    IN PVOID LogHandle,
    IN PFLUSH_TO_LSN FlushToLsnRoutine)
{
    CCTRACE(CC_API_DEBUG, "FileObject=%p LogHandle=%p FlushToLsnRoutine=%p\n",
        FileObject, LogHandle, FlushToLsnRoutine);

    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
CcUninitializeCacheMap (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER TruncateSize OPTIONAL,
    IN PCACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent OPTIONAL)
{
    NTSTATUS Status;

    CCTRACE(CC_API_DEBUG, "FileObject=%p TruncateSize=%p UninitializeCompleteEvent=%p\n",
        FileObject, TruncateSize, UninitializeCompleteEvent);

    Status = CcRosReleaseFileCache(FileObject);
    if (UninitializeCompleteEvent)
        KeSetEvent(&UninitializeCompleteEvent->Event, IO_NO_INCREMENT, FALSE);
    return NT_SUCCESS(Status);
}

BOOLEAN
NTAPI
CcGetFileSizes (
    IN PFILE_OBJECT FileObject,
    IN PCC_FILE_SIZES FileSizes)
{
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    if (!SharedCacheMap)
        return FALSE;

    FileSizes->AllocationSize = SharedCacheMap->SectionSize;
    FileSizes->FileSize = FileSizes->ValidDataLength = SharedCacheMap->FileSize;
    return TRUE;
}
