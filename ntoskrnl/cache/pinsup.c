/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/pinsup.c
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

/* The following is a test mode that only works with modified filesystems.
 * it maps the cache sections read only until they're pinned writable, and then
 * turns them readonly again when they're unpinned.
 * This helped me determine that a certain bug was not a memory overwrite. */

//#define PIN_WRITE_ONLY

/*

Pinsup implements the core of NewCC.

A couple of things about this code:

I wrote this code over the course of about 2 years, often referring to Rajeev
Nagar's Filesystem Internals, book, the msdn pages on the Cc interface, and
a few NT filesystems that are open sourced.  I went to fairly great lengths to
achieve a couple of goals.

1) To make a strictly layered facility that relies entirely on Mm to provide
maps.  There were many ways in which data segments in the legacy Mm were unable
to provide what I needed; page maps were only 4 gig, and all offsets were in
ULONG, so no mapping at an offset greater than 4 gig was possible.  Worse than
that, due to a convoluted set of dependencies, it would have been impossible to
support any two mappings farther apart than 4 gig, even if the above was
corrected.  Along with that, the cache system's ownership of some pages was
integral to the operation of legacy Mm.  All of the above problems, along with
an ambiguity about when the size of a file for mapping purposes is acquired,
and its inability to allow a file to be resized when any mappings were active
led me to rewrite data sections (and all other kinds of sections in the
original version), and use that layer to implement the Cc API without regard
to any internal, undocumented parts.

2) To write the simplest possible code that implements the Cc interface as
documented.  Again this is without regard to any information that might be
gained through reverse engineering the real Cc.  All conclusions about workings
of Cc here are mine, any failures are mine, any differences to the documented
interface were introduced by me due to misreading, misunderstanding or mis
remembering while implementing the code.  I also implemented some obvious, but
not actually specified behaviors of Cc, for example that each cache stripe is
represented by a distinct BCB that the user can make decisions about as an
opaque pointer.

3) To make real filesystems work properly.

So about how it works:

CcCacheSections is the collection of cache sections that are currently mapped.
The cache ranges which are allocated and contain pages is larger, due to the
addition of sections containing rmaps and page references, but this array
determines the actual mapped pages on behalf of all mapped files for Cc's use.
All BCB pointers yielded to a driver are a pointer to one of these cache stripe
structures.  The data structure is specified as opaque and so it contains
information convenient to NEWCC's implementation here.  Free entries are
summarized in CcpBitmapBuffer, for which bits are set when the entry may be
safely evicted and redirected for use by another client.  Note that the
reference count for an evictable cache section will generally be 1, since
we'll keep a reference to wait for any subsequent mapping of the same stripe.
We use CcCacheClockHand as a hint to start checking free bits at a point that
walks around the cache stripe list, so that we might evict a different stripe
every time even if all are awaiting reuse.  This is a way to avoid thrashing.

CcpBitmapBuffer is the RTL_BITMAP that allows us to quickly decide what buffer
to allocate from the mapped buffer set.

CcDeleteEvent is an event used to wait for a cache stripe reference count to
go to 1, thus making the stripe eligible for eviction.  It's used by CcpMapData
to wait for a free map when we can't fail.

All in all, use of Mm by Cc makes this code into a simple manager that wields
sections on behalf of filesystems.  As such, its code is fairly high level and
no architecture specific changes should be necessary.

*/

/* GLOBALS ********************************************************************/

#define TAG_MAP_SEC    TAG('C', 'c', 'S', 'x')
#define TAG_MAP_READ   TAG('M', 'c', 'p', 'y')
#define TAG_MAP_BCB    TAG('B', 'c', 'b', ' ')

NOCC_BCB CcCacheSections[CACHE_NUM_SECTIONS];
CHAR CcpBitmapBuffer[sizeof(RTL_BITMAP) + ROUND_UP((CACHE_NUM_SECTIONS), 32) / 8];
PRTL_BITMAP CcCacheBitmap = (PRTL_BITMAP)&CcpBitmapBuffer;
FAST_MUTEX CcMutex;
KEVENT CcDeleteEvent;
KEVENT CcFinalizeEvent;
ULONG CcCacheClockHand;
LONG CcOutstandingDeletes;

/* FUNCTIONS ******************************************************************/

PETHREAD LastThread;

VOID
_CcpLock(const char *file,
         int line)
{
    //DPRINT("<<<---<<< CC In Mutex(%s:%d %x)!\n", file, line, PsGetCurrentThread());
    ExAcquireFastMutex(&CcMutex);
}

VOID
_CcpUnlock(const char *file,
           int line)
{
    ExReleaseFastMutex(&CcMutex);
    //DPRINT(">>>--->>> CC Exit Mutex!\n", file, line);
}

PDEVICE_OBJECT
NTAPI
MmGetDeviceObjectForFile(IN PFILE_OBJECT FileObject);

/*

Allocate an almost ordinary section object for use by the cache system.
The special internal SEC_CACHE flag is used to indicate that the section
should not count when determining whether the file can be resized.

*/

NTSTATUS
CcpAllocateSection(PFILE_OBJECT FileObject,
                   ULONG Length,
                   ULONG Protect,
                   PROS_SECTION_OBJECT *Result)
{
    NTSTATUS Status;
    LARGE_INTEGER MaxSize;

    MaxSize.QuadPart = Length;

    DPRINT("Making Section for File %x\n", FileObject);
    DPRINT("File name %wZ\n", &FileObject->FileName);

    Status = MmCreateSection((PVOID*)Result,
                             STANDARD_RIGHTS_REQUIRED,
                             NULL,
                             &MaxSize,
                             Protect,
                             SEC_RESERVE | SEC_CACHE,
                             NULL,
                             FileObject);

    return Status;
}

typedef struct _WORK_QUEUE_WITH_CONTEXT
{
    WORK_QUEUE_ITEM WorkItem;
    PVOID ToUnmap;
    LARGE_INTEGER FileOffset;
    LARGE_INTEGER MapSize;
    PROS_SECTION_OBJECT ToDeref;
    PACQUIRE_FOR_LAZY_WRITE AcquireForLazyWrite;
    PRELEASE_FROM_LAZY_WRITE ReleaseFromLazyWrite;
    PVOID LazyContext;
    BOOLEAN Dirty;
} WORK_QUEUE_WITH_CONTEXT, *PWORK_QUEUE_WITH_CONTEXT;

/*

Unmap a cache stripe.  Note that cache stripes aren't unmapped when their
last reference disappears.  We enter this code only if cache for the file
is uninitialized in the last file object, or a cache stripe is evicted.

*/

VOID
CcpUnmapCache(PVOID Context)
{
    PWORK_QUEUE_WITH_CONTEXT WorkItem = (PWORK_QUEUE_WITH_CONTEXT)Context;
    DPRINT("Unmapping (finally) %x\n", WorkItem->ToUnmap);
    MmUnmapCacheViewInSystemSpace(WorkItem->ToUnmap);
    ObDereferenceObject(WorkItem->ToDeref);
    ExFreePool(WorkItem);
    DPRINT("Done\n");
}

/*

Somewhat deceptively named function which removes the last reference to a
cache stripe and completely removes it using CcUnmapCache.  This may be
done either inline (if the Immediate BOOLEAN is set), or using a work item
at a later time.  Whether this is called to unmap immeidately is mainly
determined by whether the caller is calling from a place in filesystem code
where a deadlock may occur if immediate flushing is required.

It's always safe to reuse the Bcb at CcCacheSections[Start] after calling
this.

 */

/* Must have acquired the mutex */
VOID
CcpDereferenceCache(ULONG Start,
                    BOOLEAN Immediate)
{
    PVOID ToUnmap;
    PNOCC_BCB Bcb;
    BOOLEAN Dirty;
    LARGE_INTEGER MappedSize;
    LARGE_INTEGER BaseOffset;
    PWORK_QUEUE_WITH_CONTEXT WorkItem;

    DPRINT("CcpDereferenceCache(#%x)\n", Start);

    Bcb = &CcCacheSections[Start];

    Dirty = Bcb->Dirty;
    ToUnmap = Bcb->BaseAddress;
    BaseOffset = Bcb->FileOffset;
    MappedSize = Bcb->Map->FileSizes.ValidDataLength;

    DPRINT("Dereference #%x (count %d)\n", Start, Bcb->RefCount);
    ASSERT(Bcb->SectionObject);
    ASSERT(Bcb->RefCount == 1);

    DPRINT("Firing work item for %x\n", Bcb->BaseAddress);

    if (Dirty) {
        CcpUnlock();
        Bcb->RefCount++;
        MiFlushMappedSection(ToUnmap, &BaseOffset, &MappedSize, Dirty);
        Bcb->RefCount--;
        CcpLock();
    }

    if (Immediate)
    {
        PROS_SECTION_OBJECT ToDeref = Bcb->SectionObject;
        Bcb->Map = NULL;
        Bcb->SectionObject = NULL;
        Bcb->BaseAddress = NULL;
        Bcb->FileOffset.QuadPart = 0;
        Bcb->Length = 0;
        Bcb->RefCount = 0;
        Bcb->Dirty = FALSE;
        RemoveEntryList(&Bcb->ThisFileList);

        CcpUnlock();
        MmUnmapCacheViewInSystemSpace(ToUnmap);
        ObDereferenceObject(ToDeref);
        CcpLock();
    }
    else
    {
        WorkItem = ExAllocatePool(NonPagedPool, sizeof(*WorkItem));
        if (!WorkItem) KeBugCheck(0);
        WorkItem->ToUnmap = Bcb->BaseAddress;
        WorkItem->FileOffset = Bcb->FileOffset;
        WorkItem->Dirty = Bcb->Dirty;
        WorkItem->MapSize = MappedSize;
        WorkItem->ToDeref = Bcb->SectionObject;
        WorkItem->AcquireForLazyWrite = Bcb->Map->Callbacks.AcquireForLazyWrite;
        WorkItem->ReleaseFromLazyWrite = Bcb->Map->Callbacks.ReleaseFromLazyWrite;
        WorkItem->LazyContext = Bcb->Map->LazyContext;

        ExInitializeWorkItem(((PWORK_QUEUE_ITEM)WorkItem),
                             (PWORKER_THREAD_ROUTINE)CcpUnmapCache,
                             WorkItem);

        Bcb->Map = NULL;
        Bcb->SectionObject = NULL;
        Bcb->BaseAddress = NULL;
        Bcb->FileOffset.QuadPart = 0;
        Bcb->Length = 0;
        Bcb->RefCount = 0;
        Bcb->Dirty = FALSE;
        RemoveEntryList(&Bcb->ThisFileList);

        CcpUnlock();
        ExQueueWorkItem((PWORK_QUEUE_ITEM)WorkItem, DelayedWorkQueue);
        CcpLock();
    }
    DPRINT("Done\n");
}

/*

CcpAllocateCacheSections is called by CcpMapData to obtain a cache stripe,
possibly evicting an old stripe by calling CcpDereferenceCache in order to
obtain an empty Bcb.

This function was named plural due to a question I had at the beginning of
this endeavor about whether a map may span a 256k stripe boundary.  It can't
so this function can only return the index of one Bcb.  Returns INVALID_CACHE
on failure.

 */
/* Needs mutex */
ULONG
CcpAllocateCacheSections(PFILE_OBJECT FileObject,
                         PROS_SECTION_OBJECT SectionObject)
{
    ULONG i = INVALID_CACHE;
    PNOCC_CACHE_MAP Map;
    PNOCC_BCB Bcb;

    DPRINT("AllocateCacheSections: FileObject %x\n", FileObject);

    if (!FileObject->SectionObjectPointer)
        return INVALID_CACHE;

    Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;

    if (!Map)
        return INVALID_CACHE;

    DPRINT("Allocating Cache Section\n");

    i = RtlFindClearBitsAndSet(CcCacheBitmap, 1, CcCacheClockHand);
    CcCacheClockHand = (i + 1) % CACHE_NUM_SECTIONS;

    if (i != INVALID_CACHE)
    {
        DPRINT("Setting up Bcb #%x\n", i);

        Bcb = &CcCacheSections[i];

        ASSERT(Bcb->RefCount < 2);

        if (Bcb->RefCount > 0)
        {
            CcpDereferenceCache(i, FALSE);
        }

        ASSERT(!Bcb->RefCount);
        Bcb->RefCount = 1;

        DPRINT("Bcb #%x RefCount %d\n", Bcb - CcCacheSections, Bcb->RefCount);

        if (!RtlTestBit(CcCacheBitmap, i))
        {
            DPRINT1("Somebody stoeled BCB #%x\n", i);
        }
        ASSERT(RtlTestBit(CcCacheBitmap, i));

        DPRINT("Allocated #%x\n", i);
        ASSERT(CcCacheSections[i].RefCount);
    }
    else
    {
        DPRINT1("Failed to allocate cache segment\n");
    }
    return i;
}

/* Must have acquired the mutex */
VOID
CcpReferenceCache(ULONG Start)
{
    PNOCC_BCB Bcb;
    Bcb = &CcCacheSections[Start];
    ASSERT(Bcb->SectionObject);
    Bcb->RefCount++;
    RtlSetBit(CcCacheBitmap, Start);

}

VOID
CcpMarkForExclusive(ULONG Start)
{
    PNOCC_BCB Bcb;
    Bcb = &CcCacheSections[Start];
    Bcb->ExclusiveWaiter++;
}

/*

Cache stripes have an idea of exclusive access, which would be hard to support
properly in the previous code.  In our case, it's fairly easy, since we have
an event that indicates that the previous exclusive waiter has returned in each
Bcb.

*/
/* Must not have the mutex */
VOID
CcpReferenceCacheExclusive(ULONG Start)
{
    PNOCC_BCB Bcb = &CcCacheSections[Start];

    KeWaitForSingleObject(&Bcb->ExclusiveWait,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    CcpLock();
    ASSERT(Bcb->ExclusiveWaiter);
    ASSERT(Bcb->SectionObject);
    Bcb->Exclusive = TRUE;
    Bcb->ExclusiveWaiter--;
    RtlSetBit(CcCacheBitmap, Start);
    CcpUnlock();
}

/*

Find a map that encompasses the target range.  This function does not check
whether the desired range is partly outside the stripe.  This could be
implemented with a generic table, but we generally aren't carring around a lot
of segments at once for a particular file.

When this returns a map for a given file address, then that address is by
definition already mapped and can be operated on.

Returns a valid index or INVALID_CACHE.

*/
/* Must have the mutex */
ULONG
CcpFindMatchingMap(PLIST_ENTRY Head,
                   PLARGE_INTEGER FileOffset,
                   ULONG Length)
{
    PLIST_ENTRY Entry;
    //DPRINT("Find Matching Map: (%x) %x:%x\n", FileOffset->LowPart, Length);
    for (Entry = Head->Flink; Entry != Head; Entry = Entry->Flink)
    {
        //DPRINT("Link @%x\n", Entry);
        PNOCC_BCB Bcb = CONTAINING_RECORD(Entry, NOCC_BCB, ThisFileList);
        //DPRINT("Selected BCB %x #%x\n", Bcb, Bcb - CcCacheSections);
        //DPRINT("This File: %x:%x\n", Bcb->FileOffset.LowPart, Bcb->Length);
        if (FileOffset->QuadPart >= Bcb->FileOffset.QuadPart &&
            FileOffset->QuadPart < Bcb->FileOffset.QuadPart + CACHE_STRIPE)
        {
            //DPRINT("Found match at #%x\n", Bcb - CcCacheSections);
            return Bcb - CcCacheSections;
        }
    }

    //DPRINT("This region isn't mapped\n");

    return INVALID_CACHE;
}

/*

Internal function that's used by all pinning functions.
It causes a mapped region to exist and prefaults the pages in it if possible,
possibly evicting another stripe in order to get our stripe.

*/

BOOLEAN
NTAPI
CcpMapData(IN PFILE_OBJECT FileObject,
           IN PLARGE_INTEGER FileOffset,
           IN ULONG Length,
           IN ULONG Flags,
           OUT PVOID *BcbResult,
           OUT PVOID *Buffer)
{
    BOOLEAN Success = FALSE, FaultIn = FALSE;
    /* Note: windows 2000 drivers treat this as a bool */
    //BOOLEAN Wait = (Flags & MAP_WAIT) || (Flags == TRUE);
    LARGE_INTEGER Target, EndInterval;
    ULONG BcbHead, SectionSize, ViewSize;
    PNOCC_BCB Bcb = NULL;
    PROS_SECTION_OBJECT SectionObject = NULL;
    NTSTATUS Status;
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;
    ViewSize = CACHE_STRIPE;

    if (!Map)
    {
        DPRINT1("File object was not mapped\n");
        return FALSE;
    }

    DPRINT("CcMapData(F->%x, %I64x:%d)\n",
           FileObject,
           FileOffset->QuadPart,
           Length);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    Target.HighPart = FileOffset->HighPart;
    Target.LowPart = CACHE_ROUND_DOWN(FileOffset->LowPart);

    CcpLock();

    /* Find out if any range is a superset of what we want */
    /* Find an accomodating section */
    BcbHead = CcpFindMatchingMap(&Map->AssociatedBcb, FileOffset, Length);

    if (BcbHead != INVALID_CACHE)
    {
        Bcb = &CcCacheSections[BcbHead];
        Success = TRUE;
        *BcbResult = Bcb;
        *Buffer = ((PCHAR)Bcb->BaseAddress) + (int)(FileOffset->QuadPart - Bcb->FileOffset.QuadPart);

        DPRINT("Bcb #%x Buffer maps (%I64x) At %x Length %x (Getting %p:%x) %wZ\n",
               Bcb - CcCacheSections,
               Bcb->FileOffset.QuadPart,
               Bcb->BaseAddress,
               Bcb->Length,
               *Buffer,
               Length,
               &FileObject->FileName);

        DPRINT("w1n\n");
        goto cleanup;
    }

    DPRINT("File size %I64x\n",
           Map->FileSizes.ValidDataLength.QuadPart);

    /* Not all files have length, in fact filesystems often use stream file
       objects for various internal purposes and are loose about the file
       length, since the filesystem promises itself to write the right number
       of bytes to the internal stream.  In these cases, we just allow the file
       to have the full stripe worth of space. */
    if (Map->FileSizes.ValidDataLength.QuadPart)
    {
        SectionSize = min(CACHE_STRIPE,
                          Map->FileSizes.ValidDataLength.QuadPart - Target.QuadPart);
    }
    else
    {
        SectionSize = CACHE_STRIPE;
    }

    DPRINT("Allocating a cache stripe at %x:%d\n",
           Target.LowPart, SectionSize);

    //ASSERT(SectionSize <= CACHE_STRIPE);

    CcpUnlock();
    /* CcpAllocateSection doesn't need the lock, so we'll give other action
       a chance in here. */
    Status = CcpAllocateSection(FileObject,
                                SectionSize,
#ifdef PIN_WRITE_ONLY
                                PAGE_READONLY,
#else
                                PAGE_READWRITE,
#endif
                                &SectionObject);
    CcpLock();

    if (!NT_SUCCESS(Status))
    {
        *BcbResult = NULL;
        *Buffer = NULL;
        DPRINT1("End %08x\n", Status);
        goto cleanup;
    }

retry:
    /* Returns a reference */
    DPRINT("Allocating cache sections: %wZ\n", &FileObject->FileName);
    BcbHead = CcpAllocateCacheSections(FileObject, SectionObject);
    /* XXX todo: we should handle the immediate fail case here, but don't */
    if (BcbHead == INVALID_CACHE)
    {
        ULONG i;
        DbgPrint("Cache Map:");
        for (i = 0; i < CACHE_NUM_SECTIONS; i++)
        {
            if (!(i % 64)) DbgPrint("\n");
            DbgPrint("%c",
                     CcCacheSections[i].RefCount + (RtlTestBit(CcCacheBitmap, i) ? '@' : '`'));
        }
        DbgPrint("\n");

        KeWaitForSingleObject(&CcDeleteEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        goto retry;
    }

    DPRINT("BcbHead #%x (final)\n", BcbHead);

    if (BcbHead == INVALID_CACHE)
    {
        *BcbResult = NULL;
        *Buffer = NULL;
        DPRINT1("End\n");
        goto cleanup;
    }

    DPRINT("Selected BCB #%x\n", BcbHead);
    ViewSize = CACHE_STRIPE;

    Bcb = &CcCacheSections[BcbHead];
    /* MmMapCacheViewInSystemSpaceAtOffset is one of three methods of Mm
       that are specific to NewCC.  In this case, it's implementation
       exactly mirrors MmMapViewInSystemSpace, but allows an offset to
       be specified. */
    Status = MmMapCacheViewInSystemSpaceAtOffset(SectionObject->Segment,
                                                 &Bcb->BaseAddress,
                                                 &Target,
                                                 &ViewSize);

    /* Summary: Failure.  Dereference our section and tell the user we failed */
    if (!NT_SUCCESS(Status))
    {
        *BcbResult = NULL;
        *Buffer = NULL;
        ObDereferenceObject(SectionObject);
        RemoveEntryList(&Bcb->ThisFileList);
        RtlZeroMemory(Bcb, sizeof(*Bcb));
        RtlClearBit(CcCacheBitmap, BcbHead);
        DPRINT1("Failed to map\n");
        goto cleanup;
    }

    /* Summary: Success.  Put together a valid Bcb and link it with the others
     * in the NOCC_CACHE_MAP.
     */
    Success = TRUE;

    Bcb->Length = MIN(Map->FileSizes.ValidDataLength.QuadPart - Target.QuadPart,
                      CACHE_STRIPE);

    Bcb->SectionObject = SectionObject;
    Bcb->Map = Map;
    Bcb->FileOffset = Target;
    InsertTailList(&Map->AssociatedBcb, &Bcb->ThisFileList);

    *BcbResult = &CcCacheSections[BcbHead];
    *Buffer = ((PCHAR)Bcb->BaseAddress) + (int)(FileOffset->QuadPart - Bcb->FileOffset.QuadPart);
    FaultIn = TRUE;

    DPRINT("Bcb #%x Buffer maps (%I64x) At %x Length %x (Getting %p:%lx) %wZ\n",
           Bcb - CcCacheSections,
           Bcb->FileOffset.QuadPart,
           Bcb->BaseAddress,
           Bcb->Length,
           *Buffer,
           Length,
           &FileObject->FileName);

    EndInterval.QuadPart = Bcb->FileOffset.QuadPart + Bcb->Length - 1;
    ASSERT((EndInterval.QuadPart & ~(CACHE_STRIPE - 1)) ==
           (Bcb->FileOffset.QuadPart & ~(CACHE_STRIPE - 1)));

cleanup:
    CcpUnlock();
    if (Success)
    {
        if (FaultIn)
        {
            /* Fault in the pages.  This forces reads to happen now. */
            ULONG i;
            PCHAR FaultIn = Bcb->BaseAddress;

            DPRINT("Faulting in pages at this point: file %wZ %I64x:%x\n",
                   &FileObject->FileName,
                   Bcb->FileOffset.QuadPart,
                   Bcb->Length);

            for (i = 0; i < Bcb->Length; i += PAGE_SIZE)
            {
                FaultIn[i] ^= 0;
            }
        }
        ASSERT(Bcb >= CcCacheSections &&
               Bcb < (CcCacheSections + CACHE_NUM_SECTIONS));
    }
    else
    {
        ASSERT(FALSE);
    }

    return Success;
}

BOOLEAN
NTAPI
CcMapData(IN PFILE_OBJECT FileObject,
          IN PLARGE_INTEGER FileOffset,
          IN ULONG Length,
          IN ULONG Flags,
          OUT PVOID *BcbResult,
          OUT PVOID *Buffer)
{
    BOOLEAN Result;

    Result = CcpMapData(FileObject,
                        FileOffset,
                        Length,
                        Flags,
                        BcbResult,
                        Buffer);

    if (Result)
    {
        PNOCC_BCB Bcb = (PNOCC_BCB)*BcbResult;

        ASSERT(Bcb >= CcCacheSections &&
               Bcb < CcCacheSections + CACHE_NUM_SECTIONS);

        ASSERT(Bcb->BaseAddress);
        CcpLock();
        CcpReferenceCache(Bcb - CcCacheSections);
        CcpUnlock();
    }

    return Result;
}

/* Used by functions that repin data, CcpPinMappedData does not alter the map,
   but finds the appropriate stripe and update the accounting. */
BOOLEAN
NTAPI
CcpPinMappedData(IN PNOCC_CACHE_MAP Map,
                 IN PLARGE_INTEGER FileOffset,
                 IN ULONG Length,
                 IN ULONG Flags,
                 IN OUT PVOID *Bcb)
{
    BOOLEAN Exclusive = Flags & PIN_EXCLUSIVE;
    ULONG BcbHead;
    PNOCC_BCB TheBcb;

    CcpLock();

    ASSERT(Map->AssociatedBcb.Flink == &Map->AssociatedBcb || (CONTAINING_RECORD(Map->AssociatedBcb.Flink, NOCC_BCB, ThisFileList) >= CcCacheSections && CONTAINING_RECORD(Map->AssociatedBcb.Flink, NOCC_BCB, ThisFileList) < CcCacheSections + CACHE_NUM_SECTIONS));
    BcbHead = CcpFindMatchingMap(&Map->AssociatedBcb, FileOffset, Length);
    if (BcbHead == INVALID_CACHE)
    {
        CcpUnlock();
        return FALSE;
    }

    TheBcb = &CcCacheSections[BcbHead];

    if (Exclusive)
    {
        DPRINT("Requesting #%x Exclusive\n", BcbHead);
        CcpMarkForExclusive(BcbHead);
    }
    else
    {
        DPRINT("Reference #%x\n", BcbHead);
        CcpReferenceCache(BcbHead);
    }

    if (Exclusive)
        CcpReferenceCacheExclusive(BcbHead);

    CcpUnlock();

    *Bcb = TheBcb;
    return TRUE;
}

BOOLEAN
NTAPI
CcPinMappedData(IN PFILE_OBJECT FileObject,
                IN PLARGE_INTEGER FileOffset,
                IN ULONG Length,
                IN ULONG Flags,
                IN OUT PVOID *Bcb)
{
    PVOID Buffer;
    PNOCC_CACHE_MAP Map = (PNOCC_CACHE_MAP)FileObject->SectionObjectPointer->SharedCacheMap;

    if (!Map)
    {
        DPRINT1("Not cached\n");
        return FALSE;
    }

    if (CcpMapData(FileObject, FileOffset, Length, Flags, Bcb, &Buffer))
    {
        return CcpPinMappedData(Map, FileOffset, Length, Flags, Bcb);
    }
    else
    {
        DPRINT1("could not map\n");
        return FALSE;
    }
}

BOOLEAN
NTAPI
CcPinRead(IN PFILE_OBJECT FileObject,
          IN PLARGE_INTEGER FileOffset,
          IN ULONG Length,
          IN ULONG Flags,
          OUT PVOID *Bcb,
          OUT PVOID *Buffer)
{
    PNOCC_BCB RealBcb;
    BOOLEAN Result;

    Result = CcPinMappedData(FileObject, FileOffset, Length, Flags, Bcb);

    if (Result)
    {
        CcpLock();
        RealBcb = *Bcb;
        *Buffer = ((PCHAR)RealBcb->BaseAddress) + (int)(FileOffset->QuadPart - RealBcb->FileOffset.QuadPart);
        CcpUnlock();
    }

    return Result;
}

BOOLEAN
NTAPI
CcPreparePinWrite(IN PFILE_OBJECT FileObject,
                  IN PLARGE_INTEGER FileOffset,
                  IN ULONG Length,
                  IN BOOLEAN Zero,
                  IN ULONG Flags,
                  OUT PVOID *Bcb,
                  OUT PVOID *Buffer)
{
    BOOLEAN Result;
    PNOCC_BCB RealBcb;
#ifdef PIN_WRITE_ONLY
    PVOID BaseAddress;
    SIZE_T NumberOfBytes;
    ULONG OldProtect;
#endif

    DPRINT("CcPreparePinWrite(%x:%x)\n", Buffer, Length);

    Result = CcPinRead(FileObject, FileOffset, Length, Flags, Bcb, Buffer);

    if (Result)
    {
        CcpLock();
        RealBcb = *Bcb;

#ifdef PIN_WRITE_ONLY
        BaseAddress = RealBcb->BaseAddress;
        NumberOfBytes = RealBcb->Length;

        MiProtectVirtualMemory(NULL,
                               &BaseAddress,
                               &NumberOfBytes,
                               PAGE_READWRITE,
                               &OldProtect);
#endif

        CcpUnlock();
        RealBcb->Dirty = TRUE;

        if (Zero)
        {
            DPRINT("Zero fill #%x %I64x:%x Buffer %x %wZ\n",
                   RealBcb - CcCacheSections,
                   FileOffset->QuadPart,
                   Length,
                   *Buffer,
                   &FileObject->FileName);

            DPRINT1("RtlZeroMemory(%p, %lx)\n", *Buffer, Length);
            RtlZeroMemory(*Buffer, Length);
        }
    }

    return Result;
}

/*

CcpUnpinData is the internal function that generally handles unpinning data.
It may be a little confusing, because of the way reference counts are handled.

A reference count of 2 or greater means that the stripe is still fully pinned
and can't be removed.  If the owner had taken an exclusive reference, then
give one up.  Note that it's an error to take more than one exclusive reference
or to take a non-exclusive reference after an exclusive reference, so detecting
or handling that case is not considered.

ReleaseBit is unset if we want to detect when a cache stripe would become
evictable without actually giving up our reference.  We might want to do that
if we were going to flush before formally releasing the cache stripe, although
that facility is not used meaningfully at this time.

A reference count of exactly 1 means that the stripe could potentially be
reused, but could also be evicted for another mapping.  In general, most
stripes should be in that state most of the time.

A reference count of zero means that the Bcb is completely unused.  That's the
start state and the state of a Bcb formerly owned by a file that is
uninitialized.

*/

BOOLEAN
NTAPI
CcpUnpinData(IN PNOCC_BCB RealBcb, BOOLEAN ReleaseBit)
{
    if (RealBcb->RefCount <= 2)
    {
        RealBcb->Exclusive = FALSE;
        if (RealBcb->ExclusiveWaiter)
        {
            DPRINT("Triggering exclusive waiter\n");
            KeSetEvent(&RealBcb->ExclusiveWait, IO_NO_INCREMENT, FALSE);
            return TRUE;
        }
    }
    if (RealBcb->RefCount == 2 && !ReleaseBit)
        return FALSE;
    if (RealBcb->RefCount > 1)
    {
        DPRINT("Removing one reference #%x\n", RealBcb - CcCacheSections);
        RealBcb->RefCount--;
        KeSetEvent(&CcDeleteEvent, IO_DISK_INCREMENT, FALSE);
    }
    if (RealBcb->RefCount == 1)
    {
        DPRINT("Clearing allocation bit #%x\n", RealBcb - CcCacheSections);

        RtlClearBit(CcCacheBitmap, RealBcb - CcCacheSections);

#ifdef PIN_WRITE_ONLY
        PVOID BaseAddress = RealBcb->BaseAddress;
        SIZE_T NumberOfBytes = RealBcb->Length;
        ULONG OldProtect;

        MiProtectVirtualMemory(NULL,
                               &BaseAddress,
                               &NumberOfBytes,
                               PAGE_READONLY,
                               &OldProtect);
#endif
    }

    return TRUE;
}

VOID
NTAPI
CcUnpinData(IN PVOID Bcb)
{
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
    ULONG Selected = RealBcb - CcCacheSections;
    BOOLEAN Released;

    ASSERT(RealBcb >= CcCacheSections &&
           RealBcb - CcCacheSections < CACHE_NUM_SECTIONS);

    DPRINT("CcUnpinData Bcb #%x (RefCount %d)\n", Selected, RealBcb->RefCount);

    CcpLock();
    Released = CcpUnpinData(RealBcb, FALSE);
    CcpUnlock();

    if (!Released) {
        CcpLock();
        CcpUnpinData(RealBcb, TRUE);
        CcpUnlock();
    }
}

VOID
NTAPI
CcSetBcbOwnerPointer(IN PVOID Bcb,
                     IN PVOID OwnerPointer)
{
    PNOCC_BCB RealBcb = (PNOCC_BCB)Bcb;
    CcpLock();
    CcpReferenceCache(RealBcb - CcCacheSections);
    RealBcb->OwnerPointer = OwnerPointer;
    CcpUnlock();
}

VOID
NTAPI
CcUnpinDataForThread(IN PVOID Bcb,
                     IN ERESOURCE_THREAD ResourceThreadId)
{
    CcUnpinData(Bcb);
}

/* EOF */
