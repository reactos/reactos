/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/pinsup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define TAG_MAP_READ   TAG('M', 'c', 'p', 'y')
#define TAG_MAP_BCB    TAG('B', 'c', 'b', ' ')

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
CcMapData(IN PFILE_OBJECT FileObject,
          IN PLARGE_INTEGER FileOffset,
          IN ULONG Length,
          IN ULONG Flags,
          OUT PVOID *Bcb,
          OUT PVOID *Buffer)
{
    PVOID CacheBuffer;
    IO_STATUS_BLOCK IoStatusBlock;
    PNOCC_BCB InternalBcb;
    BOOLEAN Result;
    DPRINT("CcMapData(FileObject 0x%p, FileOffset %I64x, Length %lx, Flags %d,"
           " Bcb 0x%p, Buffer 0x%p)\n", FileObject, FileOffset->QuadPart,
           Length, Flags, Bcb, Buffer);

    /* Allocate a buffer for the caller */
    CacheBuffer = ExAllocatePoolWithTag(NonPagedPool, Length, TAG_MAP_READ);

    /* Copy the data the caller requested */
    /* FIXME: Ignore flags */
    Result = CcCopyRead(FileObject,
                        FileOffset,
                        Length,
                        TRUE,
                        CacheBuffer,
                        &IoStatusBlock);
    ASSERT(Result == 1);

    /* Build a BCB so we'll know about this block later */
    InternalBcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(NOCC_BCB), TAG_MAP_BCB);
    InternalBcb->Bcb.NodeTypeCode = 0xDE45;
    InternalBcb->Bcb.NodeByteSize = sizeof(PUBLIC_BCB);
    InternalBcb->Bcb.MappedLength = Length;
    InternalBcb->Bcb.MappedFileOffset = *FileOffset;   
    InternalBcb->CacheBuffer = CacheBuffer;
    InternalBcb->RealLength = IoStatusBlock.Information;
    InternalBcb->FileObject = FileObject;

    /* Return buffer and BCB */
    *Bcb = InternalBcb;
    *Buffer = CacheBuffer;
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
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
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
    /* Just treat this as a map */
    return CcMapData(FileObject, FileOffset, Length, Flags, Bcb, Buffer);
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
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

VOID
NTAPI
CcUnpinData(IN PVOID Bcb)
{
    PNOCC_BCB InternalBcb = CONTAINING_RECORD(Bcb, NOCC_BCB, Bcb);

    /* Our data is never dirty -- we don't lazy write, so just free the buffer */
    ExFreePool(InternalBcb->CacheBuffer);
    ExFreePool(InternalBcb);
}

VOID
NTAPI
CcSetBcbOwnerPointer(IN PVOID Bcb,
                     IN PVOID OwnerPointer)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcUnpinDataForThread(IN PVOID Bcb,
                     IN ERESOURCE_THREAD ResourceThreadId)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/* EOF */
