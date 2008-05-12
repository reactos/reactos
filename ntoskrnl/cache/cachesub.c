/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/cachesup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
CcSetReadAheadGranularity(IN PFILE_OBJECT FileObject,
                          IN ULONG Granularity)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcScheduleReadAhead(IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN ULONG Length)
{
    UNIMPLEMENTED;
    while (TRUE);  
}

VOID
NTAPI
CcSetDirtyPinnedData(IN PVOID BcbVoid,
                     IN OPTIONAL PLARGE_INTEGER Lsn)
{
    BOOLEAN Result;
    PNOCC_BCB InternalBcb = CONTAINING_RECORD(BcbVoid, NOCC_BCB, Bcb);

    /* Write out the data immediately */
    /* FIXME: This break transactional semantics, such as NTFS Logging */
    Result = CcCopyWrite(InternalBcb->FileObject,
                         &InternalBcb->Bcb.MappedFileOffset,
                         InternalBcb->Bcb.MappedLength,
                         TRUE,
                         InternalBcb->CacheBuffer);
    ASSERT(Result == TRUE);
}

LARGE_INTEGER
NTAPI
CcGetFlushedValidData(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                      IN BOOLEAN CcInternalCaller)
{
    LARGE_INTEGER Result = {{0}};
    UNIMPLEMENTED;
    while (TRUE);
    return Result;
}

VOID
NTAPI
CcFlushCache(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
             IN OPTIONAL PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             OUT OPTIONAL PIO_STATUS_BLOCK IoStatus)
{
    /* Nothing to flush */
    if (IoStatus)
    {
        /* Lie to the caller that we did some work */
        IoStatus->Information = Length;
        IoStatus->Status = STATUS_SUCCESS;
    }
}

PVOID
NTAPI
CcRemapBcb(IN PVOID Bcb)
{
    UNIMPLEMENTED;
    while (TRUE);
    return NULL;
}


VOID
NTAPI
CcRepinBcb(IN PVOID Bcb)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcUnpinRepinnedBcb(IN PVOID Bcb,
                   IN BOOLEAN WriteThrough,
                   OUT PIO_STATUS_BLOCK IoStatus)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/* EOF */
