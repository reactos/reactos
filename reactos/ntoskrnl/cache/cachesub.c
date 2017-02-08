/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/cachesub.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Art Yerkes
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include "newcc.h"
#include "section/newmm.h"
#define NDEBUG
#include <debug.h>

/* STRUCTURES *****************************************************************/

typedef struct _WORK_QUEUE_WITH_READ_AHEAD
{
    WORK_QUEUE_ITEM WorkItem;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER FileOffset;
    ULONG Length;
} WORK_QUEUE_WITH_READ_AHEAD, *PWORK_QUEUE_WITH_READ_AHEAD;

/* FUNCTIONS ******************************************************************/

PDEVICE_OBJECT
NTAPI
MmGetDeviceObjectForFile(IN PFILE_OBJECT FileObject);

VOID
NTAPI
CcSetReadAheadGranularity(IN PFILE_OBJECT FileObject,
                          IN ULONG Granularity)
{
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
    if (Map)
    {
        Map->ReadAheadGranularity = Granularity;
    }
}

VOID
NTAPI
CcpReadAhead(PVOID Context)
{
    LARGE_INTEGER Offset;
    PWORK_QUEUE_WITH_READ_AHEAD WorkItem = (PWORK_QUEUE_WITH_READ_AHEAD)Context;
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)WorkItem->FileObject->SectionObjectPointer->SharedCacheMap;

    DPRINT1("Reading ahead %08x%08x:%x %wZ\n",
            WorkItem->FileOffset.HighPart,
            WorkItem->FileOffset.LowPart,
            WorkItem->Length,
            &WorkItem->FileObject->FileName);

    Offset.HighPart = WorkItem->FileOffset.HighPart;
    Offset.LowPart = PAGE_ROUND_DOWN(WorkItem->FileOffset.LowPart);
    if (Map)
    {
        PLIST_ENTRY ListEntry;
        volatile char *chptr;
        PNOCC_BCB Bcb;
        for (ListEntry = Map->AssociatedBcb.Flink;
             ListEntry != &Map->AssociatedBcb;
             ListEntry = ListEntry->Flink)
        {
            Bcb = CONTAINING_RECORD(ListEntry, NOCC_BCB, ThisFileList);
            if ((Offset.QuadPart + WorkItem->Length < Bcb->FileOffset.QuadPart) ||
                (Bcb->FileOffset.QuadPart + Bcb->Length < Offset.QuadPart))
                continue;
            for (chptr = Bcb->BaseAddress, Offset = Bcb->FileOffset;
                 chptr < ((PCHAR)Bcb->BaseAddress) + Bcb->Length &&
                     Offset.QuadPart <
                     WorkItem->FileOffset.QuadPart + WorkItem->Length;
                 chptr += PAGE_SIZE, Offset.QuadPart += PAGE_SIZE)
            {
                *chptr ^= 0;
            }
        }
    }
    ObDereferenceObject(WorkItem->FileObject);
    ExFreePool(WorkItem);
    DPRINT("Done\n");
}

VOID
NTAPI
CcScheduleReadAhead(IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN ULONG Length)
{
    PWORK_QUEUE_WITH_READ_AHEAD WorkItem;

    DPRINT("Schedule read ahead %08x%08x:%x %wZ\n",
           FileOffset->HighPart,
           FileOffset->LowPart,
           Length,
           &FileObject->FileName);

    WorkItem = ExAllocatePool(NonPagedPool, sizeof(*WorkItem));
    if (!WorkItem) KeBugCheck(0);
    ObReferenceObject(FileObject);
    WorkItem->FileObject = FileObject;
    WorkItem->FileOffset = *FileOffset;
    WorkItem->Length = Length;

    ExInitializeWorkItem(((PWORK_QUEUE_ITEM)WorkItem),
                         (PWORKER_THREAD_ROUTINE)CcpReadAhead,
                         WorkItem);

    ExQueueWorkItem((PWORK_QUEUE_ITEM)WorkItem, DelayedWorkQueue);
    DPRINT("Done\n");
}

VOID
NTAPI
CcSetDirtyPinnedData(IN PVOID BcbVoid,
                     IN OPTIONAL PLARGE_INTEGER Lsn)
{
    PNOCC_BCB Bcb = (PNOCC_BCB)BcbVoid;
    Bcb->Dirty = TRUE;
}

LARGE_INTEGER
NTAPI
CcGetFlushedValidData(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                      IN BOOLEAN CcInternalCaller)
{
    LARGE_INTEGER Result = {{0}};
    UNIMPLEMENTED_DBGBREAK();
    return Result;
}



VOID
NTAPI
_CcpFlushCache(IN PNOCC_CACHE_MAP Map,
               IN OPTIONAL PLARGE_INTEGER FileOffset,
               IN ULONG Length,
               OUT OPTIONAL PIO_STATUS_BLOCK IoStatus,
               BOOLEAN Delete,
               const char *File,
               int Line)
{
    PNOCC_BCB Bcb = NULL;
    LARGE_INTEGER LowerBound, UpperBound;
    PLIST_ENTRY ListEntry;
    IO_STATUS_BLOCK IOSB;

    RtlZeroMemory(&IOSB, sizeof(IO_STATUS_BLOCK));

    DPRINT("CcFlushCache (while file) (%s:%d)\n", File, Line);

    if (FileOffset && Length)
    {
        LowerBound.QuadPart = FileOffset->QuadPart;
        UpperBound.QuadPart = LowerBound.QuadPart + Length;
    }
    else
    {
        LowerBound.QuadPart = 0;
        UpperBound.QuadPart = 0x7fffffffffffffffull;
    }

    CcpLock();
    ListEntry = Map->AssociatedBcb.Flink;

    while (ListEntry != &Map->AssociatedBcb)
    {
        Bcb = CONTAINING_RECORD(ListEntry, NOCC_BCB, ThisFileList);
        CcpReferenceCache((ULONG)(Bcb - CcCacheSections));

        if (Bcb->FileOffset.QuadPart + Bcb->Length >= LowerBound.QuadPart &&
            Bcb->FileOffset.QuadPart < UpperBound.QuadPart)
        {
            DPRINT("Bcb #%x (@%08x%08x)\n",
                   Bcb - CcCacheSections,
                   Bcb->FileOffset.u.HighPart, Bcb->FileOffset.u.LowPart);

            Bcb->RefCount++;
            CcpUnlock();
            MiFlushMappedSection(Bcb->BaseAddress,
                                 &Bcb->FileOffset,
                                 &Map->FileSizes.FileSize,
                                 Bcb->Dirty);
            CcpLock();
            Bcb->RefCount--;

            Bcb->Dirty = FALSE;

            ListEntry = ListEntry->Flink;
            if (Delete && Bcb->RefCount < 2)
            {
                Bcb->RefCount = 1;
                CcpDereferenceCache((ULONG)(Bcb - CcCacheSections), FALSE);
            }
            else
            {
                CcpUnpinData(Bcb, TRUE);
            }
        }
        else
        {
            ListEntry = ListEntry->Flink;
            CcpUnpinData(Bcb, TRUE);
        }

        DPRINT("End loop\n");
    }
    CcpUnlock();

    if (IoStatus) *IoStatus = IOSB;
}

VOID
NTAPI
CcFlushCache(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
             IN OPTIONAL PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             OUT OPTIONAL PIO_STATUS_BLOCK IoStatus)
{
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap;

    /* Not cached */
    if (!Map)
    {
        if (IoStatus)
        {
            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = 0;
        }
        return;
    }

    CcpFlushCache(Map, FileOffset, Length, IoStatus, TRUE);
}

BOOLEAN
NTAPI
CcFlushImageSection(PSECTION_OBJECT_POINTERS SectionObjectPointer,
                    MMFLUSH_TYPE FlushType)
{
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)SectionObjectPointer->SharedCacheMap;
    PNOCC_BCB Bcb;
    PLIST_ENTRY Entry;
    IO_STATUS_BLOCK IOSB;
    BOOLEAN Result = TRUE;

    if (!Map) return TRUE;

    for (Entry = Map->AssociatedBcb.Flink;
         Entry != &Map->AssociatedBcb;
         Entry = Entry->Flink)
    {
        Bcb = CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList);

        if (!Bcb->Dirty) continue;

        switch (FlushType)
        {
            case MmFlushForDelete:
                CcPurgeCacheSection(SectionObjectPointer,
                                    &Bcb->FileOffset,
                                    Bcb->Length,
                                    FALSE);
                break;
            case MmFlushForWrite:
                CcFlushCache(SectionObjectPointer,
                             &Bcb->FileOffset,
                             Bcb->Length,
                             &IOSB);
                break;
        }
    }

    return Result;
}

/* Always succeeds for us */
PVOID
NTAPI
CcRemapBcb(IN PVOID Bcb)
{
    ULONG Number = (ULONG)(((PNOCC_BCB)Bcb) - CcCacheSections);
    CcpLock();
    ASSERT(RtlTestBit(CcCacheBitmap, Number));
    CcpReferenceCache(Number);
    CcpUnlock();
    return Bcb;
}

VOID
NTAPI
CcShutdownSystem(VOID)
{
    ULONG i, Result;
    NTSTATUS Status;

    DPRINT1("CC: Shutdown\n");

    for (i = 0; i < CACHE_NUM_SECTIONS; i++)
    {
        PNOCC_BCB Bcb = &CcCacheSections[i];
        if (Bcb->SectionObject)
        {
            DPRINT1("Evicting #%02x %08x%08x %wZ\n",
                    i,
                    Bcb->FileOffset.u.HighPart,
                    Bcb->FileOffset.u.LowPart,
                    &MmGetFileObjectForSection((PROS_SECTION_OBJECT)Bcb->SectionObject)->FileName);

            CcpFlushCache(Bcb->Map, NULL, 0, NULL, TRUE);
            Bcb->Dirty = FALSE;
        }
    }

    /* Evict all section pages */
    Status = MiRosTrimCache(~0, 0, &Result);

    DPRINT1("Done (Evicted %d, Status %x)\n", Result, Status);
}


VOID
NTAPI
CcRepinBcb(IN PVOID Bcb)
{
    ULONG Number = (ULONG)(((PNOCC_BCB)Bcb) - CcCacheSections);
    CcpLock();
    ASSERT(RtlTestBit(CcCacheBitmap, Number));
    DPRINT("CcRepinBcb(#%x)\n", Number);
    CcpReferenceCache(Number);
    CcpUnlock();
}

VOID
NTAPI
CcUnpinRepinnedBcb(IN PVOID Bcb,
                   IN BOOLEAN WriteThrough,
                   OUT PIO_STATUS_BLOCK IoStatus)
{
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;

    if (WriteThrough)
    {
        DPRINT("BCB #%x\n", RealBcb - CcCacheSections);

        CcpFlushCache(RealBcb->Map,
                      &RealBcb->FileOffset,
                      RealBcb->Length,
                      IoStatus,
                      RealBcb->Dirty);
    }

    CcUnpinData(Bcb);
}

/* EOF */
