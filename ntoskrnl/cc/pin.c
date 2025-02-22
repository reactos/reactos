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
        CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE);
        return NULL;
    }

    RtlZeroMemory(iBcb, sizeof(*iBcb));
    iBcb->PFCB.NodeTypeCode = 0x2FD; /* As per KMTests */
    iBcb->PFCB.NodeByteSize = 0;
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
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE);
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
    KIRQL OldIrql;
    ULONG VacbOffset;
    NTSTATUS Status;
    _SEH2_VOLATILE BOOLEAN Result;

    VacbOffset = (ULONG)(FileOffset->QuadPart % VACB_MAPPING_GRANULARITY);

    if ((VacbOffset + Length) > VACB_MAPPING_GRANULARITY)
    {
        /* Complain loudly, we shoud pin the whole range */
        DPRINT1("TRUNCATING DATA PIN FROM %lu to %lu!\n", Length, VACB_MAPPING_GRANULARITY - VacbOffset);
        Length = VACB_MAPPING_GRANULARITY - VacbOffset;
    }

    KeAcquireSpinLock(&SharedCacheMap->BcbSpinLock, &OldIrql);
    NewBcb = CcpFindBcb(SharedCacheMap, FileOffset, Length, TRUE);

    if (NewBcb != NULL)
    {
        BOOLEAN Result;

        ++NewBcb->RefCount;
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        if (BooleanFlagOn(Flags, PIN_EXCLUSIVE))
            Result = ExAcquireResourceExclusiveLite(&NewBcb->Lock, BooleanFlagOn(Flags, PIN_WAIT));
        else
            Result = ExAcquireSharedStarveExclusive(&NewBcb->Lock, BooleanFlagOn(Flags, PIN_WAIT));

        if (!Result)
        {
            CcpDereferenceBcb(SharedCacheMap, NewBcb);
            return FALSE;
        }

        NewBcb->PinCount++;
    }
    else
    {
        LONGLONG ROffset;
        PROS_VACB Vacb;

        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        if (BooleanFlagOn(Flags, PIN_IF_BCB))
        {
            return FALSE;
        }

        /* Properly round offset and call internal helper for getting a VACB */
        ROffset = ROUND_DOWN(FileOffset->QuadPart, VACB_MAPPING_GRANULARITY);
        Status = CcRosGetVacb(SharedCacheMap, ROffset, &Vacb);
        if (!NT_SUCCESS(Status))
        {
            CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> FALSE\n",
                SharedCacheMap->FileObject, FileOffset, Length, Flags);
            ExRaiseStatus(Status);
            return FALSE;
        }

        NewBcb = CcpGetAppropriateBcb(SharedCacheMap, Vacb, FileOffset, Length, Flags, TRUE);
        if (NewBcb == NULL)
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE);
            return FALSE;
        }
    }

    Result = FALSE;
    _SEH2_TRY
    {
        /* Ensure the pages are resident */
        Result = CcRosEnsureVacbResident(NewBcb->Vacb,
                BooleanFlagOn(Flags, PIN_WAIT),
                BooleanFlagOn(Flags, PIN_NO_READ),
                VacbOffset, Length);
    }
    _SEH2_FINALLY
    {
        if (!Result)
        {
            CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> FALSE\n",
                            SharedCacheMap->FileObject, FileOffset, Length, Flags);
            CcUnpinData(&NewBcb->PFCB);
            *Bcb = NULL;
            *Buffer = NULL;
        }
    }
    _SEH2_END;

    if (Result)
    {
        *Bcb = &NewBcb->PFCB;
        *Buffer = (PVOID)((ULONG_PTR)NewBcb->Vacb->BaseAddress + VacbOffset);
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
    KIRQL OldIrql;
    PINTERNAL_BCB iBcb;
    PROS_VACB Vacb;
    PROS_SHARED_CACHE_MAP SharedCacheMap;
    ULONG VacbOffset;
    NTSTATUS Status;
    _SEH2_VOLATILE BOOLEAN Result;

    CCTRACE(CC_API_DEBUG, "CcMapData(FileObject 0x%p, FileOffset 0x%I64x, Length %lu, Flags 0x%lx,"
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

    VacbOffset = (ULONG)(FileOffset->QuadPart % VACB_MAPPING_GRANULARITY);
    /* KMTests seem to show that it is allowed to call accross mapping granularity */
    if ((VacbOffset + Length) > VACB_MAPPING_GRANULARITY)
    {
        DPRINT1("TRUNCATING DATA MAP FROM %lu to %lu!\n", Length, VACB_MAPPING_GRANULARITY - VacbOffset);
        Length = VACB_MAPPING_GRANULARITY - VacbOffset;
    }

    KeAcquireSpinLock(&SharedCacheMap->BcbSpinLock, &OldIrql);
    iBcb = CcpFindBcb(SharedCacheMap, FileOffset, Length, FALSE);

    if (iBcb == NULL)
    {
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

        /* Call internal helper for getting a VACB */
        Status = CcRosGetVacb(SharedCacheMap, FileOffset->QuadPart, &Vacb);
        if (!NT_SUCCESS(Status))
        {
            CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> FALSE\n",
                SharedCacheMap->FileObject, FileOffset, Length, Flags);
            ExRaiseStatus(Status);
        }

        iBcb = CcpGetAppropriateBcb(SharedCacheMap, Vacb, FileOffset, Length, 0, FALSE);
        if (iBcb == NULL)
        {
            CcRosReleaseVacb(SharedCacheMap, Vacb, FALSE, FALSE);
            CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> FALSE\n",
                SharedCacheMap->FileObject, FileOffset, Length, Flags);
            *pBcb = NULL; // If you ever remove this for compat, make sure to review all callers for using an unititialized value
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
    }
    else
    {
        ++iBcb->RefCount;
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);
    }

    _SEH2_TRY
    {
        Result = FALSE;
        /* Ensure the pages are resident */
        Result = CcRosEnsureVacbResident(iBcb->Vacb, BooleanFlagOn(Flags, MAP_WAIT),
                BooleanFlagOn(Flags, MAP_NO_READ), VacbOffset, Length);
    }
    _SEH2_FINALLY
    {
        if (!Result)
        {
            CcpDereferenceBcb(SharedCacheMap, iBcb);
            *pBcb = NULL;
            *pBuffer = NULL;
        }
    }
    _SEH2_END;

    if (Result)
    {
        *pBcb = &iBcb->PFCB;
        *pBuffer = (PVOID)((ULONG_PTR)iBcb->Vacb->BaseAddress + VacbOffset);
    }

    CCTRACE(CC_API_DEBUG, "FileObject=%p FileOffset=%p Length=%lu Flags=0x%lx -> TRUE Bcb=%p, Buffer %p\n",
        FileObject, FileOffset, Length, Flags, *pBcb, *pBuffer);
    return Result;
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

    iBcb = *Bcb ? CONTAINING_RECORD(*Bcb, INTERNAL_BCB, PFCB) : NULL;

    ++CcPinMappedDataCount;

    Result = CcpPinData(SharedCacheMap, FileOffset, Length, Flags, Bcb, &Buffer);
    if (Result)
    {
        CcUnpinData(&iBcb->PFCB);
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
    PINTERNAL_BCB iBcb = CONTAINING_RECORD(Bcb, INTERNAL_BCB, PFCB);
    PROS_VACB Vacb = iBcb->Vacb;

    CCTRACE(CC_API_DEBUG, "Bcb=%p Lsn=%p\n", Bcb, Lsn);

    /* Tell Mm */
    MmMakeSegmentDirty(Vacb->SharedCacheMap->FileObject->SectionObjectPointer,
                       iBcb->PFCB.MappedFileOffset.QuadPart,
                       iBcb->PFCB.MappedLength);

    if (!Vacb->Dirty)
    {
        CcRosMarkDirtyVacb(Vacb);
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
    PINTERNAL_BCB iBcb = CONTAINING_RECORD(Bcb, INTERNAL_BCB, PFCB);

    CCTRACE(CC_API_DEBUG, "Bcb=%p ResourceThreadId=%lu\n", Bcb, ResourceThreadId);

    if (iBcb->PinCount != 0)
    {
        ExReleaseResourceForThreadLite(&iBcb->Lock, ResourceThreadId);
        iBcb->PinCount--;
    }

    CcpDereferenceBcb(iBcb->Vacb->SharedCacheMap, iBcb);
}

/*
 * @implemented
 */
VOID
NTAPI
CcRepinBcb (
    IN	PVOID Bcb)
{
    PINTERNAL_BCB iBcb = CONTAINING_RECORD(Bcb, INTERNAL_BCB, PFCB);

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
    PINTERNAL_BCB iBcb = CONTAINING_RECORD(Bcb, INTERNAL_BCB, PFCB);
    KIRQL OldIrql;
    PROS_SHARED_CACHE_MAP SharedCacheMap;

    CCTRACE(CC_API_DEBUG, "Bcb=%p WriteThrough=%d\n", Bcb, WriteThrough);

    SharedCacheMap = iBcb->Vacb->SharedCacheMap;
    IoStatus->Status = STATUS_SUCCESS;

    if (WriteThrough)
    {
        CcFlushCache(iBcb->Vacb->SharedCacheMap->FileObject->SectionObjectPointer,
                     &iBcb->PFCB.MappedFileOffset,
                     iBcb->PFCB.MappedLength,
                     IoStatus);
    }
    else
    {
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = 0;
    }

    KeAcquireSpinLock(&SharedCacheMap->BcbSpinLock, &OldIrql);
    if (--iBcb->RefCount == 0)
    {
        RemoveEntryList(&iBcb->BcbEntry);
        KeReleaseSpinLock(&SharedCacheMap->BcbSpinLock, OldIrql);

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
