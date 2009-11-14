/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/largemcb.c
 * PURPOSE:         Large Mapped Control Block (MCB) support for File System Drivers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Pierre Schweitzer (heis_spiter@hotmail.com) 
 *                  Art Yerkes (art.yerkes@gmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define GET_LIST_HEAD(x) ((PLIST_ENTRY)(&((PRTL_GENERIC_TABLE)x)[1]))

PAGED_LOOKASIDE_LIST FsRtlFirstMappingLookasideList;
NPAGED_LOOKASIDE_LIST FsRtlFastMutexLookasideList;

typedef struct _LARGE_MCB_MAPPING_ENTRY
{
	LARGE_INTEGER RunStartVbn;
	LARGE_INTEGER SectorCount;
	LARGE_INTEGER StartingLbn;
	LIST_ENTRY Sequence;
} LARGE_MCB_MAPPING_ENTRY, *PLARGE_MCB_MAPPING_ENTRY;

static PVOID NTAPI McbMappingAllocate(PRTL_GENERIC_TABLE Table, CLONG Bytes)
{
	PVOID Result;
	PBASE_MCB Mcb = (PBASE_MCB)Table->TableContext;
	Result = ExAllocatePoolWithTag(Mcb->PoolType, Bytes, 'LMCB');
	DPRINT("McbMappingAllocate(%d) => %p\n", Bytes, Result);
	return Result;
}

static VOID NTAPI McbMappingFree(PRTL_GENERIC_TABLE Table, PVOID Buffer)
{
	DPRINT("McbMappingFree(%p)\n", Buffer);
	ExFreePoolWithTag(Buffer, 'LMCB');
}

static RTL_GENERIC_COMPARE_RESULTS NTAPI McbMappingCompare
(RTL_GENERIC_TABLE Table, PVOID PtrA, PVOID PtrB)
{
	PLARGE_MCB_MAPPING_ENTRY A = PtrA, B = PtrB;
	return 
		(A->RunStartVbn.QuadPart + A->SectorCount.QuadPart <
		 B->RunStartVbn.QuadPart) ? GenericLessThan :
		(A->RunStartVbn.QuadPart > 
		 B->RunStartVbn.QuadPart + B->SectorCount.QuadPart) ? 
		GenericGreaterThan : GenericEqual;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlAddBaseMcbEntry(IN PBASE_MCB Mcb,
                     IN LONGLONG Vbn,
                     IN LONGLONG Lbn,
                     IN LONGLONG SectorCount)
{
	LARGE_MCB_MAPPING_ENTRY Node;
	PLARGE_MCB_MAPPING_ENTRY Existing = NULL;
	BOOLEAN NewElement = FALSE;

	Node.RunStartVbn.QuadPart = Vbn;
	Node.StartingLbn.QuadPart = Lbn;
	Node.SectorCount.QuadPart = SectorCount;

	while (!NewElement)
	{
		Existing = RtlInsertElementGenericTable
			(Mcb->Mapping, &Node, sizeof(Node), &NewElement);
		if (!Existing) break;

		if (!NewElement)
		{
			// We merge the existing runs
			LARGE_INTEGER StartVbn, FinalVbn;
			if (Existing->RunStartVbn.QuadPart < Node.RunStartVbn.QuadPart)
			{
				StartVbn = Existing->RunStartVbn;
				Node.StartingLbn = Existing->StartingLbn;
			}
			else
			{
				StartVbn = Node.RunStartVbn;
			}
			if (Existing->RunStartVbn.QuadPart + Existing->SectorCount.QuadPart >
				Node.RunStartVbn.QuadPart + Node.SectorCount.QuadPart)
			{
				FinalVbn.QuadPart = 
					Existing->RunStartVbn.QuadPart + Existing->SectorCount.QuadPart;
			}
			else
			{
				FinalVbn.QuadPart =
					Node.RunStartVbn.QuadPart + Node.SectorCount.QuadPart;
			}
			Node.RunStartVbn.QuadPart = StartVbn.QuadPart;
			Node.SectorCount.QuadPart = FinalVbn.QuadPart - StartVbn.QuadPart;
			RemoveHeadList(&Existing->Sequence);
			RtlDeleteElementGenericTable(Mcb->Mapping, Existing);
			Mcb->PairCount--;
		}
		else
		{
			Mcb->MaximumPairCount++;
			Mcb->PairCount++;
			InsertHeadList(GET_LIST_HEAD(Mcb->Mapping), &Existing->Sequence);
		}
	}

	return !!Existing;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlAddLargeMcbEntry(IN PLARGE_MCB Mcb,
                      IN LONGLONG Vbn,
                      IN LONGLONG Lbn,
                      IN LONGLONG SectorCount)
{
    BOOLEAN Result;

	DPRINT1("Mcb %x Vbn %x Lbn %x SectorCount %x\n", Mcb, Vbn, Lbn, SectorCount);

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlAddBaseMcbEntry(&(Mcb->BaseMcb),
                                  Vbn,
                                  Lbn,
                                  SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlGetNextBaseMcbEntry(IN PBASE_MCB Mcb,
                         IN ULONG RunIndex,
                         OUT PLONGLONG Vbn,
                         OUT PLONGLONG Lbn,
                         OUT PLONGLONG SectorCount)
{
	ULONG i = 0;
	BOOLEAN Result = FALSE;
	PLARGE_MCB_MAPPING_ENTRY Entry;
	for (Entry = (PLARGE_MCB_MAPPING_ENTRY)
			 RtlEnumerateGenericTable(Mcb->Mapping, TRUE);
		 Entry && i < RunIndex;
		 Entry = (PLARGE_MCB_MAPPING_ENTRY)
			 RtlEnumerateGenericTable(Mcb->Mapping, FALSE), i++);
	if (Entry)
	{
		Result = TRUE;
		if (Vbn)
			*Vbn = Entry->RunStartVbn.QuadPart;
		if (Lbn)
			*Lbn = Entry->StartingLbn.QuadPart;
		if (SectorCount)
			*SectorCount = Entry->SectorCount.QuadPart;
	}

	return Result;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlGetNextLargeMcbEntry(IN PLARGE_MCB Mcb,
                          IN ULONG RunIndex,
                          OUT PLONGLONG Vbn,
                          OUT PLONGLONG Lbn,
                          OUT PLONGLONG SectorCount)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlGetNextBaseMcbEntry(&(Mcb->BaseMcb),
                                      RunIndex,
                                      Vbn,
                                      Lbn,
                                      SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlInitializeBaseMcb(IN PBASE_MCB Mcb,
                       IN POOL_TYPE PoolType)
{
    Mcb->PairCount = 0;

    if (PoolType == PagedPool)
    {
        Mcb->Mapping = ExAllocateFromPagedLookasideList(&FsRtlFirstMappingLookasideList);
    }
    else
    {
        Mcb->Mapping = ExAllocatePoolWithTag(PoolType | POOL_RAISE_IF_ALLOCATION_FAILURE,
											 sizeof(RTL_GENERIC_TABLE) + sizeof(LIST_ENTRY),
                                             'FSBC');
    }

    Mcb->PoolType = PoolType;
    Mcb->MaximumPairCount = MAXIMUM_PAIR_COUNT;
	RtlInitializeGenericTable
		(Mcb->Mapping,
		 (PRTL_GENERIC_COMPARE_ROUTINE)McbMappingCompare,
		 McbMappingAllocate,
		 McbMappingFree,
		 Mcb);
	InitializeListHead(GET_LIST_HEAD(Mcb->Mapping));
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlInitializeLargeMcb(IN PLARGE_MCB Mcb,
                        IN POOL_TYPE PoolType)
{
    Mcb->GuardedMutex = ExAllocateFromNPagedLookasideList(&FsRtlFastMutexLookasideList);

    KeInitializeGuardedMutex(Mcb->GuardedMutex);

    _SEH2_TRY
    {
        FsRtlInitializeBaseMcb(&(Mcb->BaseMcb), PoolType);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreeToNPagedLookasideList(&FsRtlFastMutexLookasideList,
                                    Mcb->GuardedMutex);
        Mcb->GuardedMutex = NULL;
    }
    _SEH2_END;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlInitializeLargeMcbs(VOID)
{
    /* Initialize the list for the MCB */
    ExInitializePagedLookasideList(&FsRtlFirstMappingLookasideList,
                                   NULL,
                                   NULL,
                                   POOL_RAISE_IF_ALLOCATION_FAILURE,
								   sizeof(RTL_GENERIC_TABLE) + sizeof(LIST_ENTRY),
                                   IFS_POOL_TAG,
                                   0); /* FIXME: Should be 4 */

    /* Initialize the list for the guarded mutex */
    ExInitializeNPagedLookasideList(&FsRtlFastMutexLookasideList,
                                    NULL,
                                    NULL,
                                    POOL_RAISE_IF_ALLOCATION_FAILURE,
                                    sizeof(KGUARDED_MUTEX),
                                    IFS_POOL_TAG,
                                    0); /* FIXME: Should be 32 */
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupBaseMcbEntry(IN PBASE_MCB Mcb,
                        IN LONGLONG Vbn,
                        OUT PLONGLONG Lbn OPTIONAL,
                        OUT PLONGLONG SectorCountFromLbn OPTIONAL,
                        OUT PLONGLONG StartingLbn OPTIONAL,
                        OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
                        OUT PULONG Index OPTIONAL)
{
	BOOLEAN Result = FALSE;
	LARGE_MCB_MAPPING_ENTRY ToLookup;
	PLARGE_MCB_MAPPING_ENTRY Entry;

	ToLookup.RunStartVbn.QuadPart = Vbn;
	ToLookup.SectorCount.QuadPart = 1;

	Entry = RtlLookupElementGenericTable(Mcb->Mapping, &ToLookup);
	if (!Entry)
	{
		// Find out if we have a following entry.  The spec says we should return
		// found with Lbn == -1 when we're beneath the largest map.
		ToLookup.SectorCount.QuadPart = (1ull<<62) - ToLookup.RunStartVbn.QuadPart;
		Entry = RtlLookupElementGenericTable(Mcb->Mapping, &ToLookup);
		if (Entry)
		{
			Result = TRUE;
			if (Lbn) *Lbn = ~0ull;
		}
		else
		{
			Result = FALSE;
		}
	}
	else
	{
		LARGE_INTEGER Offset;
		Offset.QuadPart = Vbn - Entry->RunStartVbn.QuadPart;
		Result = TRUE;
		if (Lbn) *Lbn = Entry->StartingLbn.QuadPart + Offset.QuadPart;
		if (SectorCountFromLbn) *SectorCountFromLbn = Entry->SectorCount.QuadPart - Offset.QuadPart;
		if (StartingLbn) *StartingLbn = Entry->StartingLbn.QuadPart;
		if (SectorCountFromStartingLbn) *SectorCountFromStartingLbn = Entry->SectorCount.QuadPart;
	}
	
    return Result;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlLookupLargeMcbEntry(IN PLARGE_MCB Mcb,
                         IN LONGLONG Vbn,
                         OUT PLONGLONG Lbn OPTIONAL,
                         OUT PLONGLONG SectorCountFromLbn OPTIONAL,
                         OUT PLONGLONG StartingLbn OPTIONAL,
                         OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
                         OUT PULONG Index OPTIONAL)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlLookupBaseMcbEntry(&(Mcb->BaseMcb),
                                     Vbn,
                                     Lbn,
                                     SectorCountFromLbn,
                                     StartingLbn,
                                     SectorCountFromStartingLbn,
                                     Index);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntryAndIndex(IN PBASE_MCB OpaqueMcb,
                                    IN OUT PLONGLONG LargeVbn,
                                    IN OUT PLONGLONG LargeLbn,
                                    IN OUT PULONG Index)
{
	ULONG i = 0;
	BOOLEAN Result = FALSE;
	PLIST_ENTRY ListEntry;
	PLARGE_MCB_MAPPING_ENTRY Entry;
	PLARGE_MCB_MAPPING_ENTRY CountEntry;

	ListEntry = GET_LIST_HEAD(OpaqueMcb->Mapping);
	if (!IsListEmpty(ListEntry))
	{
		Entry = CONTAINING_RECORD(ListEntry->Flink, LARGE_MCB_MAPPING_ENTRY, Sequence);
		Result = TRUE;
		*LargeVbn = Entry->RunStartVbn.QuadPart;
		*LargeLbn = Entry->StartingLbn.QuadPart;

		for (i = 0, CountEntry = RtlEnumerateGenericTable(OpaqueMcb->Mapping, TRUE);
			 CountEntry != Entry;
			 CountEntry = RtlEnumerateGenericTable(OpaqueMcb->Mapping, FALSE));

		*Index = i;
	}

	return Result;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntryAndIndex(IN PLARGE_MCB OpaqueMcb,
                                     OUT PLONGLONG LargeVbn,
                                     OUT PLONGLONG LargeLbn,
                                     OUT PULONG Index)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(OpaqueMcb->GuardedMutex);
    Result = FsRtlLookupLastBaseMcbEntryAndIndex(&(OpaqueMcb->BaseMcb),
                                                 LargeVbn,
                                                 LargeLbn,
                                                 Index);
    KeReleaseGuardedMutex(OpaqueMcb->GuardedMutex);

    return Result;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntry(IN PBASE_MCB Mcb,
                            OUT PLONGLONG Vbn,
                            OUT PLONGLONG Lbn)
{
	BOOLEAN Result = FALSE;
	PLIST_ENTRY ListEntry;
	PLARGE_MCB_MAPPING_ENTRY Entry;

	ListEntry = GET_LIST_HEAD(Mcb->Mapping);
	if (!IsListEmpty(ListEntry))
	{
		Entry = CONTAINING_RECORD(ListEntry->Flink, LARGE_MCB_MAPPING_ENTRY, Sequence);
		Result = TRUE;
		*Vbn = Entry->RunStartVbn.QuadPart;
		*Lbn = Entry->StartingLbn.QuadPart;
	}
	
	return Result;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntry(IN PLARGE_MCB Mcb,
                             OUT PLONGLONG Vbn,
                             OUT PLONGLONG Lbn)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlLookupLastBaseMcbEntry(&(Mcb->BaseMcb),
                                         Vbn,
                                         Lbn);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @implemented
 */
ULONG
NTAPI
FsRtlNumberOfRunsInBaseMcb(IN PBASE_MCB Mcb)
{
    /* Return the count */
    return Mcb->PairCount;
}

/*
 * @implemented
 */
ULONG
NTAPI
FsRtlNumberOfRunsInLargeMcb(IN PLARGE_MCB Mcb)
{
    ULONG NumberOfRuns;

    /* Read the number of runs while holding the MCB lock */
    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    NumberOfRuns = Mcb->BaseMcb.PairCount;
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    /* Return the count */
    return NumberOfRuns;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlRemoveBaseMcbEntry(IN PBASE_MCB Mcb,
                        IN LONGLONG Vbn,
                        IN LONGLONG SectorCount)
{
	LARGE_MCB_MAPPING_ENTRY Node;
	PLARGE_MCB_MAPPING_ENTRY Element;

	Node.RunStartVbn.QuadPart = Vbn;
	Node.SectorCount.QuadPart = SectorCount;

	while ((Element = RtlLookupElementGenericTable(Mcb->Mapping, &Node)))
	{
		// Must split
		if (Element->RunStartVbn.QuadPart < Node.RunStartVbn.QuadPart &&
			Element->SectorCount.QuadPart > Node.SectorCount.QuadPart)
		{
			LARGE_MCB_MAPPING_ENTRY Upper, Reinsert;
			PLARGE_MCB_MAPPING_ENTRY Reinserted, Inserted;
			LARGE_INTEGER StartHole = Node.RunStartVbn;
			LARGE_INTEGER EndHole;
			EndHole.QuadPart = Node.RunStartVbn.QuadPart + Node.SectorCount.QuadPart;
			Upper.RunStartVbn.QuadPart = EndHole.QuadPart;
			Upper.StartingLbn.QuadPart = 
				Element->StartingLbn.QuadPart + 
				EndHole.QuadPart - 
				Element->RunStartVbn.QuadPart;
			Upper.SectorCount.QuadPart = 
				Element->SectorCount.QuadPart -
				(EndHole.QuadPart - Element->RunStartVbn.QuadPart);
			Reinsert = *Element;
			Reinsert.SectorCount.QuadPart = 
				Element->RunStartVbn.QuadPart - StartHole.QuadPart;
			RemoveEntryList(&Element->Sequence);
			RtlDeleteElementGenericTable(Mcb->Mapping, Element);
			Mcb->PairCount--;

			Reinserted = RtlInsertElementGenericTable
				(Mcb->Mapping, &Reinsert, sizeof(Reinsert), NULL);
			InsertHeadList(GET_LIST_HEAD(Mcb->Mapping), &Reinserted->Sequence);
			Mcb->PairCount++;
			
			Inserted = RtlInsertElementGenericTable
				(Mcb->Mapping, &Upper, sizeof(Upper), NULL);
			InsertHeadList(GET_LIST_HEAD(Mcb->Mapping), &Inserted->Sequence);
			Mcb->PairCount++;
		}
		else if (Element->RunStartVbn.QuadPart < Node.RunStartVbn.QuadPart)
		{
			LARGE_MCB_MAPPING_ENTRY NewElement;
			PLARGE_MCB_MAPPING_ENTRY Reinserted;
			LARGE_INTEGER StartHole = Node.RunStartVbn;
			NewElement.RunStartVbn = Element->RunStartVbn;
			NewElement.StartingLbn = Element->StartingLbn;
			NewElement.SectorCount.QuadPart = StartHole.QuadPart - Element->StartingLbn.QuadPart;

			RemoveEntryList(&Element->Sequence);
			RtlDeleteElementGenericTable(Mcb->Mapping, Element);
			Mcb->PairCount--;
			
			Reinserted = RtlInsertElementGenericTable
				(Mcb->Mapping, &NewElement, sizeof(NewElement), NULL);
			InsertHeadList(GET_LIST_HEAD(Mcb->Mapping), &Reinserted->Sequence);
			Mcb->PairCount++;			
		}
		else
		{
			LARGE_MCB_MAPPING_ENTRY NewElement;
			PLARGE_MCB_MAPPING_ENTRY Reinserted;
			LARGE_INTEGER EndHole = Element->RunStartVbn;
			LARGE_INTEGER EndRun;
			EndRun.QuadPart = Element->RunStartVbn.QuadPart + Element->SectorCount.QuadPart;
			NewElement.RunStartVbn = EndHole;
			NewElement.StartingLbn.QuadPart = Element->StartingLbn.QuadPart + 
				(EndHole.QuadPart - Element->RunStartVbn.QuadPart);
			NewElement.SectorCount.QuadPart = EndRun.QuadPart - EndHole.QuadPart;

			RemoveEntryList(&Element->Sequence);
			RtlDeleteElementGenericTable(Mcb->Mapping, Element);
			Mcb->PairCount--;
			
			Reinserted = RtlInsertElementGenericTable
				(Mcb->Mapping, &NewElement, sizeof(NewElement), NULL);
			InsertHeadList(GET_LIST_HEAD(Mcb->Mapping), &Reinserted->Sequence);
			Mcb->PairCount++;			
		}
	}

	return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlRemoveLargeMcbEntry(IN PLARGE_MCB Mcb,
                         IN LONGLONG Vbn,
                         IN LONGLONG SectorCount)
{
    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    FsRtlRemoveBaseMcbEntry(&(Mcb->BaseMcb),
                            Vbn,
                            SectorCount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlResetBaseMcb(IN PBASE_MCB Mcb)
{
	PLARGE_MCB_MAPPING_ENTRY Element;

	while (RtlNumberGenericTableElements(Mcb->Mapping) &&
		   (Element = (PLARGE_MCB_MAPPING_ENTRY)RtlGetElementGenericTable(Mcb->Mapping, 0)))
	{
		RtlDeleteElementGenericTable(Mcb->Mapping, Element);
	}

	Mcb->PairCount = 0;
	Mcb->MaximumPairCount = 0;
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlResetLargeMcb(IN PLARGE_MCB Mcb,
                   IN BOOLEAN SelfSynchronized)
{
    if (!SelfSynchronized)
    {
        KeAcquireGuardedMutex(Mcb->GuardedMutex);
	}

	FsRtlResetBaseMcb(&Mcb->BaseMcb);


    if (!SelfSynchronized)
    {
		KeReleaseGuardedMutex(Mcb->GuardedMutex);
    }
}

#define MCB_BUMP_NO_MORE 0
#define MCB_BUMP_AGAIN   1

static ULONG NTAPI McbBump(PBASE_MCB Mcb, PLARGE_MCB_MAPPING_ENTRY FixedPart)
{
	LARGE_MCB_MAPPING_ENTRY Reimagined;
	PLARGE_MCB_MAPPING_ENTRY Found = NULL;

	Reimagined = *FixedPart;
	while ((Found = RtlLookupElementGenericTable(Mcb->Mapping, &Reimagined)))
	{
		Reimagined = *Found;
		Reimagined.RunStartVbn.QuadPart = 
			FixedPart->RunStartVbn.QuadPart + FixedPart->SectorCount.QuadPart;
	}

	if (!Found) return MCB_BUMP_NO_MORE;
	DPRINT1
		("Moving %x-%x to %x because %x-%x overlaps\n",
		 Found->RunStartVbn.LowPart, 
		 Found->RunStartVbn.LowPart + Found->SectorCount.QuadPart,
		 Reimagined.RunStartVbn.LowPart + Reimagined.SectorCount.LowPart,
		 Reimagined.RunStartVbn.LowPart,
		 Reimagined.RunStartVbn.LowPart + Reimagined.SectorCount.LowPart);
	Found->RunStartVbn.QuadPart = Reimagined.RunStartVbn.QuadPart + Reimagined.SectorCount.QuadPart;
	Found->StartingLbn.QuadPart = Reimagined.StartingLbn.QuadPart + Reimagined.SectorCount.QuadPart;
	return MCB_BUMP_AGAIN;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
FsRtlSplitBaseMcb(IN PBASE_MCB Mcb,
                  IN LONGLONG Vbn,
                  IN LONGLONG Amount)
{
	ULONG Result;
	LARGE_MCB_MAPPING_ENTRY Node;
	PLARGE_MCB_MAPPING_ENTRY Existing = NULL;

	Node.RunStartVbn.QuadPart = Vbn;
	Node.SectorCount.QuadPart = 0;
	
	Existing = RtlLookupElementGenericTable(Mcb->Mapping, &Node);

	if (Existing)
	{
		// We're in the middle of a run
		LARGE_MCB_MAPPING_ENTRY UpperPart;
		LARGE_MCB_MAPPING_ENTRY LowerPart;
		PLARGE_MCB_MAPPING_ENTRY InsertedUpper;

		UpperPart.RunStartVbn.QuadPart = Node.RunStartVbn.QuadPart + Amount;
		UpperPart.SectorCount.QuadPart = Existing->RunStartVbn.QuadPart + 
			(Existing->SectorCount.QuadPart - Node.RunStartVbn.QuadPart);
		UpperPart.StartingLbn.QuadPart = Existing->StartingLbn.QuadPart + 
			(Node.RunStartVbn.QuadPart - Existing->RunStartVbn.QuadPart);
		LowerPart.RunStartVbn.QuadPart = Existing->RunStartVbn.QuadPart;
		LowerPart.SectorCount.QuadPart = Node.RunStartVbn.QuadPart - Existing->RunStartVbn.QuadPart;
		LowerPart.StartingLbn.QuadPart = Existing->StartingLbn.QuadPart;
		
		Node = UpperPart;

		while ((Result = McbBump(Mcb, &Node)) == MCB_BUMP_AGAIN);

		if (Result == MCB_BUMP_NO_MORE)
		{
			Node = *Existing;
			RemoveHeadList(&Existing->Sequence);
			RtlDeleteElementGenericTable(Mcb->Mapping, Existing);
			Mcb->PairCount--;
			
			// Adjust the element we found.
			Existing->SectorCount = LowerPart.SectorCount;

			InsertedUpper = RtlInsertElementGenericTable
				(Mcb->Mapping, &UpperPart, sizeof(UpperPart), NULL);
			if (!InsertedUpper)
			{
				// Just make it like it was
				Existing->SectorCount = Node.SectorCount;
				return FALSE;
			}
			InsertHeadList(GET_LIST_HEAD(Mcb->Mapping), &InsertedUpper->Sequence);
			Mcb->PairCount++;
		}
		else
		{
			Node.RunStartVbn.QuadPart = Vbn;
			Node.SectorCount.QuadPart = Amount;
			while ((Result = McbBump(Mcb, &Node)) == MCB_BUMP_AGAIN);
			return Result == MCB_BUMP_NO_MORE;
		}
	}
	
	return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
FsRtlSplitLargeMcb(IN PLARGE_MCB Mcb,
                   IN LONGLONG Vbn,
                   IN LONGLONG Amount)
{
    BOOLEAN Result;

    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    Result = FsRtlSplitBaseMcb(&(Mcb->BaseMcb),
                               Vbn,
                               Amount);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);

    return Result;
}

/*
 * @unimplemented
 */
VOID
NTAPI
FsRtlTruncateBaseMcb(IN PBASE_MCB Mcb,
                     IN LONGLONG Vbn)
{
	if (!Vbn)
	{
		FsRtlResetBaseMcb(Mcb);
	}
	else
	{
		LARGE_MCB_MAPPING_ENTRY Truncate;
		PLARGE_MCB_MAPPING_ENTRY Found;
		Truncate.RunStartVbn.QuadPart = Vbn;
		Truncate.SectorCount.QuadPart = (1ull<<62) - Truncate.RunStartVbn.QuadPart;
		while ((Found = RtlLookupElementGenericTable(Mcb->Mapping, &Truncate)))
		{
			RemoveEntryList(&Found->Sequence);
			RtlDeleteElementGenericTable(Mcb->Mapping, Found);
			Mcb->PairCount--;
		}
	}
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlTruncateLargeMcb(IN PLARGE_MCB Mcb,
                      IN LONGLONG Vbn)
{
    KeAcquireGuardedMutex(Mcb->GuardedMutex);
    FsRtlTruncateBaseMcb(&(Mcb->BaseMcb),
                         Vbn);
    KeReleaseGuardedMutex(Mcb->GuardedMutex);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlUninitializeBaseMcb(IN PBASE_MCB Mcb)
{
	FsRtlResetBaseMcb(Mcb);

    if ((Mcb->PoolType == PagedPool) && (Mcb->MaximumPairCount == MAXIMUM_PAIR_COUNT))
    {
        ExFreeToPagedLookasideList(&FsRtlFirstMappingLookasideList,
                                   Mcb->Mapping);
    }
    else
    {
        ExFreePoolWithTag(Mcb->Mapping, 'FSBC');
    }
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlUninitializeLargeMcb(IN PLARGE_MCB Mcb)
{
    if (Mcb->GuardedMutex)
    {
        ExFreeToNPagedLookasideList(&FsRtlFastMutexLookasideList,
                                    Mcb->GuardedMutex);
        FsRtlUninitializeBaseMcb(&(Mcb->BaseMcb));
    }
}

