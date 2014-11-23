/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/pin.c
 * PURPOSE:         Implements cache managers pinning interface
 *
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern NPAGED_LOOKASIDE_LIST iBcbLookasideList;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
CcMapData (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG Flags,
    OUT PVOID *pBcb,
    OUT PVOID *pBuffer)
{
    ULONG ReadOffset;
    BOOLEAN Valid;
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    PROS_VACB Vacb;
    NTSTATUS Status;
    PINTERNAL_BCB iBcb;
    ULONG ROffset;

    DPRINT("CcMapData(FileObject 0x%p, FileOffset %I64x, Length %lu, Flags 0x%lx,"
           " pBcb 0x%p, pBuffer 0x%p)\n", FileObject, FileOffset->QuadPart,
           Length, Flags, pBcb, pBuffer);

    ReadOffset = (ULONG)FileOffset->QuadPart;

    ASSERT(FileObject);
    ASSERT(FileObject->SectionObjectPointer);
    ASSERT(FileObject->SectionObjectPointer->SharedCacheMap);

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(SharedCacheMap);

    DPRINT("SectionSize %I64x, FileSize %I64x\n",
           SharedCacheMap->SectionSize.QuadPart,
           SharedCacheMap->FileSize.QuadPart);

    if (ReadOffset % VACB_MAPPING_GRANULARITY + Length > VACB_MAPPING_GRANULARITY)
    {
        return FALSE;
    }

    ROffset = ROUND_DOWN(ReadOffset, VACB_MAPPING_GRANULARITY);
    Status = CcRosRequestVacb(SharedCacheMap,
                              ROffset,
                              pBuffer,
                              &Valid,
                              &Vacb);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    if (!Valid)
    {
        if (!(Flags & MAP_WAIT))
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
            return FALSE;
        }

        if (!NT_SUCCESS(CcReadVirtualAddress(Vacb)))
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
            return FALSE;
        }
    }

    *pBuffer = (PVOID)((ULONG_PTR)(*pBuffer) + ReadOffset % VACB_MAPPING_GRANULARITY);
    iBcb = ExAllocateFromNPagedLookasideList(&iBcbLookasideList);
    if (iBcb == NULL)
    {
        CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, FALSE);
        return FALSE;
    }

    RtlZeroMemory(iBcb, sizeof(*iBcb));
    iBcb->PFCB.NodeTypeCode = 0xDE45; /* Undocumented (CAPTIVE_PUBLIC_BCB_NODETYPECODE) */
    iBcb->PFCB.NodeByteSize = sizeof(PUBLIC_BCB);
    iBcb->PFCB.MappedLength = Length;
    iBcb->PFCB.MappedFileOffset = *FileOffset;
    iBcb->Vacb = Vacb;
    iBcb->Dirty = FALSE;
    iBcb->RefCount = 1;
    *pBcb = (PVOID)iBcb;

    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
CcPinMappedData (
    IN	PFILE_OBJECT FileObject,
    IN	PLARGE_INTEGER FileOffset,
    IN	ULONG Length,
    IN	ULONG Flags,
    OUT	PVOID * Bcb)
{
    /* no-op for current implementation. */
    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
CcPinRead (
    IN	PFILE_OBJECT FileObject,
    IN	PLARGE_INTEGER FileOffset,
    IN	ULONG Length,
    IN	ULONG Flags,
    OUT	PVOID * Bcb,
    OUT	PVOID * Buffer)
{
    if (CcMapData(FileObject, FileOffset, Length, Flags, Bcb, Buffer))
    {
        if (CcPinMappedData(FileObject, FileOffset, Length, Flags, Bcb))
            return TRUE;
        else
            CcUnpinData(*Bcb);
    }
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
CcPreparePinWrite (
    IN	PFILE_OBJECT FileObject,
    IN	PLARGE_INTEGER FileOffset,
    IN	ULONG Length,
    IN	BOOLEAN Zero,
    IN	ULONG Flags,
    OUT	PVOID * Bcb,
    OUT	PVOID * Buffer)
{
    /*
     * FIXME: This is function is similar to CcPinRead, but doesn't
     * read the data if they're not present. Instead it should just
     * prepare the VACBs and zero them out if Zero != FALSE.
     *
     * For now calling CcPinRead is better than returning error or
     * just having UNIMPLEMENTED here.
     */
    return CcPinRead(FileObject, FileOffset, Length, Flags, Bcb, Buffer);
}

/*
 * @implemented
 */
VOID NTAPI
CcSetDirtyPinnedData (
    IN PVOID Bcb,
    IN PLARGE_INTEGER Lsn)
{
    PINTERNAL_BCB iBcb = Bcb;
    iBcb->Dirty = TRUE;
}


/*
 * @implemented
 */
VOID NTAPI
CcUnpinData (
    IN PVOID Bcb)
{
    PINTERNAL_BCB iBcb = Bcb;

    CcRosReleaseVacb(iBcb->Vacb->SharedCacheMap,
                     iBcb->Vacb,
                     TRUE,
                     iBcb->Dirty,
                     FALSE);
    if (--iBcb->RefCount == 0)
    {
        ExFreeToNPagedLookasideList(&iBcbLookasideList, iBcb);
    }
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcUnpinDataForThread (
    IN	PVOID Bcb,
    IN	ERESOURCE_THREAD ResourceThreadId)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
CcRepinBcb (
    IN	PVOID Bcb)
{
    PINTERNAL_BCB iBcb = Bcb;
    iBcb->RefCount++;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcUnpinRepinnedBcb (
    IN	PVOID Bcb,
    IN	BOOLEAN WriteThrough,
    IN	PIO_STATUS_BLOCK IoStatus)
{
    PINTERNAL_BCB iBcb = Bcb;

    IoStatus->Status = STATUS_SUCCESS;
    if (--iBcb->RefCount == 0)
    {
        IoStatus->Information = 0;
        if (WriteThrough)
        {
            KeWaitForSingleObject(&iBcb->Vacb->Mutex,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            if (iBcb->Vacb->Dirty)
            {
                IoStatus->Status = CcRosFlushVacb(iBcb->Vacb);
            }
            else
            {
                IoStatus->Status = STATUS_SUCCESS;
            }
            KeReleaseMutex(&iBcb->Vacb->Mutex, FALSE);
        }
        else
        {
            IoStatus->Status = STATUS_SUCCESS;
        }

        ExFreeToNPagedLookasideList(&iBcbLookasideList, iBcb);
    }
}
