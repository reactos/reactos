/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/pin.c
 * PURPOSE:         Implements cache managers pinning interface
 *
 * PROGRAMMERS:     ?
                    Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern NPAGED_LOOKASIDE_LIST iBcbLookasideList;

/* Counters:
 * - Number of calls to CcMapData that could wait
 * - Number of calls to CcMapData that couldn't wait
 * - Number of calls to CcPinRead that could wait
 * - Number of calls to CcPinRead that couldn't wait
 * - Number of calls to CcPinMappedDataCount
 */
ULONG CcMapDataWait = 0;
ULONG CcMapDataNoWait = 0;
ULONG CcPinReadWait = 0;
ULONG CcPinReadNoWait = 0;
ULONG CcPinMappedDataCount = 0;

/* FUNCTIONS *****************************************************************/

static
PINTERNAL_BCB
NTAPI
CcpFindBcb(
    IN PROS_SHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Pinned)
{
    PINTERNAL_BCB Bcb;
    BOOLEAN Found = FALSE;
    PLIST_ENTRY NextEntry;

    for (NextEntry = SharedCacheMap->BcbList.Flink;
         NextEntry != &SharedCacheMap->BcbList;
         NextEntry = NextEntry->Flink)
    {
        Bcb = CONTAINING_RECORD(NextEntry, INTERNAL_BCB, BcbEntry);

        if (Bcb->PFCB.MappedFileOffset.QuadPart <= FileOffset->QuadPart &&
            (Bcb->PFCB.MappedFileOffset.QuadPart + Bcb->PFCB.MappedLength) >=
            (FileOffset->QuadPart + Length))
        {
            if ((Pinned && Bcb->PinCount > 0) || (!Pinned && Bcb->PinCount == 0))
            {
                Found = TRUE;
                break;
            }
        }
    }

    return (Found ? Bcb : NULL);
}

static
BOOLEAN
NTAPI
CcpMapData(
    IN PROS_SHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG Flags,
    OUT PROS_VACB *pVacb,
    OUT PVOID *pBuffer)
{
    LONGLONG ReadOffset, BaseOffset;
    BOOLEAN Valid;
    PROS_VACB Vacb;
    NTSTATUS Status;
    LONGLONG ROffset;

    ReadOffset = FileOffset->QuadPart;

    DPRINT("SectionSize %I64x, FileSize %I64x\n",
           SharedCacheMap->SectionSize.QuadPart,
           SharedCacheMap->FileSize.QuadPart);

    if (ReadOffset % VACB_MAPPING_GRANULARITY + Length > VACB_MAPPING_GRANULARITY)
    {
        CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> FALSE\n",
            SharedCacheMap->FileObject, FileOffset, Length, Flags);
        return FALSE;
    }

    if (!BooleanFlagOn(Flags, MAP_NO_READ))
    {
        static int Warned = 0;

        SetFlag(Flags, MAP_NO_READ);
        if (!Warned)
        {
            DPRINT1("Mapping/pinning with no read not implemented. Forcing read, might fail if wait not allowed\n");
            Warned++;
        }
    }

    /* Properly round offset and call internal helper for getting a VACB */
    ROffset = ROUND_DOWN(ReadOffset, VACB_MAPPING_GRANULARITY);
    Status = CcRosGetVacb(SharedCacheMap,
                          ROffset,
                          &BaseOffset,
                          pBuffer,
                          &Valid,
                          &Vacb);
    if (!NT_SUCCESS(Status))
    {
        CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> FALSE\n",
            SharedCacheMap->FileObject, FileOffset, Length, Flags);
        ExRaiseStatus(Status);
        return FALSE;
    }

    if (!Valid && BooleanFlagOn(Flags, MAP_NO_READ))
    {
        if (!BooleanFlagOn(Flags, MAP_WAIT))
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
            CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> FALSE\n",
                SharedCacheMap->FileObject, FileOffset, Length, Flags);
            return FALSE;
        }

        Status = CcReadVirtualAddress(Vacb);
        if (!NT_SUCCESS(Status))
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE, FALSE);
            CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> FALSE\n",
                SharedCacheMap->FileObject, FileOffset, Length, Flags);
            ExRaiseStatus(Status);
            return FALSE;
        }
    }

    *pBuffer = (PUCHAR)*pBuffer + ReadOffset % VACB_MAPPING_GRANULARITY;
    *pVacb = Vacb;

    return TRUE;
}

static
VOID
CcpDereferenceBcb(
    IN PROS_SHARED_CACHE_MAP SharedCacheMap,
    IN PINTERNAL_BCB Bcb)
{
    ULONG RefCount;
    KIRQL OldIrql;

    KeAcquireSpinLock(&SharedCacheMap->BcbSpinLock, &OldIrql);
    RefCount = --Bcb->RefCount;
    if (RefCount == 0)
    {
        RemoveEntryList(&Bcb->BcbEntry);
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        ASSERT(Bcb->PinCount == 0);
        /*
         * Don't mark dirty, if it was dirty,
         * the VACB was already marked as such
         * following the call to CcSetDirtyPinnedData
         */
        CcRosReleaseVacb(SharedCacheMap,
                         Bcb->Vacb,
                         TRUE,
                         FALSE,
                         FALSE);

        ExDeleteResourceLite(&Bcb->Lock);
        ExFreeToNPagedLookasideList(&iBcbLookasideList, Bcb);
    }
    else
    {
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);
    }
}

static
PVOID
CcpGetAppropriateBcb(
    IN PROS_SHARED_CACHE_MAP SharedCacheMap,
    IN PROS_VACB Vacb,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG PinFlags,
    IN BOOLEAN ToPin)
{
    KIRQL OldIrql;
    BOOLEAN Result;
    PINTERNAL_BCB iBcb, DupBcb;

    iBcb = ExAllocateFromNPagedLookasideList(&iBcbLookasideList);
    if (iBcb == NULL)
    {
        CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, FALSE);
        return NULL;
    }

    RtlZeroMemory(iBcb, sizeof(*iBcb));
    iBcb->PFCB.NodeTypeCode = 0xDE45; /* Undocumented (CAPTIVE_PUBLIC_BCB_NODETYPECODE) */
    iBcb->PFCB.NodeByteSize = sizeof(PUBLIC_BCB);
    iBcb->PFCB.MappedLength = Length;
    iBcb->PFCB.MappedFileOffset = *FileOffset;
    iBcb->Vacb = Vacb;
    iBcb->PinCount = 0;
    iBcb->RefCount = 1;
    ExInitializeResourceLite(&iBcb->Lock);

    KeAcquireSpinLock(&SharedCacheMap->BcbSpinLock, &OldIrql);

    /* Check if we raced with another BCB creation */
    DupBcb = CcpFindBcb(SharedCacheMap, FileOffset, Length, ToPin);
    /* Yes, and we've lost */
    if (DupBcb != NULL)
    {
        /* We will return that BCB */
        ++DupBcb->RefCount;
        Result = TRUE;
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        if (ToPin)
        {
            if (BooleanFlagOn(PinFlags, PIN_EXCLUSIVE))
            {
                Result = ExAcquireResourceExclusiveLite(&iBcb->Lock, BooleanFlagOn(PinFlags, PIN_WAIT));
            }
            else
            {
                Result = ExAcquireSharedStarveExclusive(&iBcb->Lock, BooleanFlagOn(PinFlags, PIN_WAIT));
            }

            if (Result)
            {
                DupBcb->PinCount++;
            }
            else
            {
                CcpDereferenceBcb(SharedCacheMap, DupBcb);
                DupBcb = NULL;
            }
        }

        if (DupBcb != NULL)
        {
            /* Delete the loser */
            CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, FALSE);
            ExDeleteResourceLite(&iBcb->Lock);
            ExFreeToNPagedLookasideList(&iBcbLookasideList, iBcb);
        }

        /* Return the winner - no need to update buffer address, it's
         * relative to the VACB, which is unchanged.
         */
        iBcb = DupBcb;
    }
    /* Nope, insert ourselves */
    else
    {
        if (ToPin)
        {
            iBcb->PinCount++;

            if (BooleanFlagOn(PinFlags, PIN_EXCLUSIVE))
            {
                Result = ExAcquireResourceExclusiveLite(&iBcb->Lock, BooleanFlagOn(PinFlags, PIN_WAIT));
            }
            else
            {
                Result = ExAcquireSharedStarveExclusive(&iBcb->Lock, BooleanFlagOn(PinFlags, PIN_WAIT));
            }

            ASSERT(Result);
        }

        InsertTailList(&SharedCacheMap->BcbList, &iBcb->BcbEntry);
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);
    }

    return iBcb;
}

static
BOOLEAN
CcpPinData(
    IN PROS_SHARED_CACHE_MAP SharedCacheMap,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG Flags,
    OUT	PVOID * Bcb,
    OUT	PVOID * Buffer)
{
    PINTERNAL_BCB NewBcb;
    BOOLEAN Result;
    PROS_VACB Vacb;
    KIRQL OldIrql;
    ULONG MapFlags;

    KeAcquireSpinLock(&SharedCacheMap->BcbSpinLock, &OldIrql);
    NewBcb = CcpFindBcb(SharedCacheMap, FileOffset, Length, TRUE);

    if (NewBcb != NULL)
    {
        ++NewBcb->RefCount;
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        if (BooleanFlagOn(Flags, PIN_EXCLUSIVE))
        {
            Result = ExAcquireResourceExclusiveLite(&NewBcb->Lock, BooleanFlagOn(Flags, PIN_WAIT));
        }
        else
        {
            Result = ExAcquireSharedStarveExclusive(&NewBcb->Lock, BooleanFlagOn(Flags, PIN_WAIT));
        }

        if (!Result)
        {
            CcpDereferenceBcb(SharedCacheMap, NewBcb);
            NewBcb = NULL;
        }
        else
        {
            NewBcb->PinCount++;
            *Bcb = NewBcb;
            *Buffer = (PUCHAR)NewBcb->Vacb->BaseAddress + FileOffset->QuadPart % VACB_MAPPING_GRANULARITY;
        }

        return Result;
    }
    else
    {
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        if (BooleanFlagOn(Flags, PIN_IF_BCB))
        {
            return FALSE;
        }

        MapFlags = Flags & PIN_WAIT;
        if (BooleanFlagOn(Flags, PIN_NO_READ))
        {
            SetFlag(MapFlags, MAP_NO_READ);
        }

        Result = CcpMapData(SharedCacheMap, FileOffset, Length, MapFlags, &Vacb, Buffer);
        if (Result)
        {
            NewBcb = CcpGetAppropriateBcb(SharedCacheMap, Vacb, FileOffset, Length, Flags, TRUE);
            if (NewBcb == NULL)
            {
                CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, FALSE);
                Result = FALSE;
            }
            else
            {
                *Bcb = NewBcb;
            }
        }
    }

    return Result;
}

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
    BOOLEAN Ret;
    KIRQL OldIrql;
    PINTERNAL_BCB iBcb;
    PROS_VACB Vacb;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    DPRINT("CcMapData(FileObject 0x%p, FileOffset %I64x, Length %lu, Flags 0x%lx,"
           " pBcb 0x%p, pBuffer 0x%p)\n", FileObject, FileOffset->QuadPart,
           Length, Flags, pBcb, pBuffer);

    ASSERT(FileObject);
    ASSERT(FileObject->SectionObjectPointer);
    ASSERT(FileObject->SectionObjectPointer->SharedCacheMap);

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(SharedCacheMap);

    if (Flags & MAP_WAIT)
    {
        ++CcMapDataWait;
    }
    else
    {
        ++CcMapDataNoWait;
    }

    KeAcquireSpinLock(&SharedCacheMap->BcbSpinLock, &OldIrql);
    iBcb = CcpFindBcb(SharedCacheMap, FileOffset, Length, FALSE);

    if (iBcb == NULL)
    {
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        Ret = CcpMapData(SharedCacheMap, FileOffset, Length, Flags, &Vacb, pBuffer);
        if (Ret)
        {
            iBcb = CcpGetAppropriateBcb(SharedCacheMap, Vacb, FileOffset, Length, 0, FALSE);
            if (iBcb == NULL)
            {
                CcRosReleaseVacb(SharedCacheMap, Vacb, TRUE, FALSE, FALSE);
                *pBcb = NULL; // If you ever remove this for compat, make sure to review all callers for using an unititialized value
                Ret = FALSE;
            }
            else
            {
                *pBcb = iBcb;
            }
        }
    }
    else
    {
        ++iBcb->RefCount;
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        *pBcb = iBcb;
        *pBuffer = (PUCHAR)iBcb->Vacb->BaseAddress + FileOffset->QuadPart % VACB_MAPPING_GRANULARITY;
        Ret = TRUE;
    }

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> %d Bcb=%p\n",
        FileObject, FileOffset, Length, Flags, Ret, *pBcb);
    return Ret;
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
    BOOLEAN Result;
    PVOID Buffer;
    PINTERNAL_BCB iBcb;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx\n",
        FileObject, FileOffset, Length, Flags);

    ASSERT(FileObject);
    ASSERT(FileObject->SectionObjectPointer);
    ASSERT(FileObject->SectionObjectPointer->SharedCacheMap);

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(SharedCacheMap);
    if (!SharedCacheMap->PinAccess)
    {
        DPRINT1("FIXME: Pinning a file with no pin access!\n");
        return FALSE;
    }

    iBcb = *Bcb;

    ++CcPinMappedDataCount;

    Result = CcpPinData(SharedCacheMap, FileOffset, Length, Flags, Bcb, &Buffer);
    if (Result)
    {
        CcUnpinData(iBcb);
    }

    return Result;
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
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "FileOffset=%p FileOffset=%p Length=%lu Flags=0x%lx\n",
        FileObject, FileOffset, Length, Flags);

    ASSERT(FileObject);
    ASSERT(FileObject->SectionObjectPointer);
    ASSERT(FileObject->SectionObjectPointer->SharedCacheMap);

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;
    ASSERT(SharedCacheMap);
    if (!SharedCacheMap->PinAccess)
    {
        DPRINT1("FIXME: Pinning a file with no pin access!\n");
        return FALSE;
    }

    if (Flags & PIN_WAIT)
    {
        ++CcPinReadWait;
    }
    else
    {
        ++CcPinReadNoWait;
    }

    return CcpPinData(SharedCacheMap, FileOffset, Length, Flags, Bcb, Buffer);
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
    CCTRACE(CC_API_DEBUG, "FileOffset=%p FileOffset=%p Length=%lu Zero=%d Flags=0x%lx\n",
        FileObject, FileOffset, Length, Zero, Flags);

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

    CCTRACE(CC_API_DEBUG, "Bcb=%p Lsn=%p\n",
        Bcb, Lsn);

    if (!iBcb->Vacb->Dirty)
    {
        CcRosMarkDirtyVacb(iBcb->Vacb);
    }
}


/*
 * @implemented
 */
VOID NTAPI
CcUnpinData (
    IN PVOID Bcb)
{
    CCTRACE(CC_API_DEBUG, "Bcb=%p\n", Bcb);

    CcUnpinDataForThread(Bcb, (ERESOURCE_THREAD)PsGetCurrentThread());
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
    PINTERNAL_BCB iBcb = Bcb;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "Bcb=%p ResourceThreadId=%lu\n", Bcb, ResourceThreadId);

    if (iBcb->PinCount != 0)
    {
        ExReleaseResourceForThreadLite(&iBcb->Lock, ResourceThreadId);
        iBcb->PinCount--;
    }

    SharedCacheMap = iBcb->Vacb->SharedCacheMap;
    CcpDereferenceBcb(SharedCacheMap, iBcb);
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

    CCTRACE(CC_API_DEBUG, "Bcb=%p\n", Bcb);

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
    KIRQL OldIrql;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "Bcb=%p WriteThrough=%d\n", Bcb, WriteThrough);

    SharedCacheMap = iBcb->Vacb->SharedCacheMap;
    IoStatus->Status = STATUS_SUCCESS;

    KeAcquireSpinLock(&SharedCacheMap->BcbSpinLock, &OldIrql);
    if (--iBcb->RefCount == 0)
    {
        RemoveEntryList(&iBcb->BcbEntry);
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        IoStatus->Information = 0;
        if (WriteThrough)
        {
            if (iBcb->Vacb->Dirty)
            {
                IoStatus->Status = CcRosFlushVacb(iBcb->Vacb);
            }
            else
            {
                IoStatus->Status = STATUS_SUCCESS;
            }
        }
        else
        {
            IoStatus->Status = STATUS_SUCCESS;
        }

        if (iBcb->PinCount != 0)
        {
            ExReleaseResourceLite(&iBcb->Lock);
            iBcb->PinCount--;
            ASSERT(iBcb->PinCount == 0);
        }

        /*
         * Don't mark dirty, if it was dirty,
         * the VACB was already marked as such
         * following the call to CcSetDirtyPinnedData
         */
        CcRosReleaseVacb(iBcb->Vacb->SharedCacheMap,
                         iBcb->Vacb,
                         TRUE,
                         FALSE,
                         FALSE);

        ExDeleteResourceLite(&iBcb->Lock);
        ExFreeToNPagedLookasideList(&iBcbLookasideList, iBcb);
    }
    else
    {
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);
    }
}
