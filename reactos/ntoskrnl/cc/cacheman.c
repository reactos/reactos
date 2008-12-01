/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/cacheman.c
 * PURPOSE:         Cache manager
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

PFSN_PREFETCHER_GLOBALS CcPfGlobals;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CcPfInitializePrefetcher(VOID)
{
    /* Notify debugger */
    DbgPrintEx(DPFLTR_PREFETCHER_ID,
               DPFLTR_TRACE_LEVEL,
               "CCPF: InitializePrefetecher()\n");

    /* Setup the Prefetcher Data */
    InitializeListHead(&CcPfGlobals.ActiveTraces);
    InitializeListHead(&CcPfGlobals.CompletedTraces);
    ExInitializeFastMutex(&CcPfGlobals.CompletedTracesLock);

    /* FIXME: Setup the rest of the prefetecher */
}

BOOLEAN
NTAPI
CcInitializeCacheManager(VOID)
{
    CcInitView();
    return TRUE;
}

/*
 * @unimplemented
 */
LARGE_INTEGER
NTAPI
CcGetFlushedValidData (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN BcbListHeld
    )
{
	LARGE_INTEGER i;

	UNIMPLEMENTED;

	i.QuadPart = 0;
	return i;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
CcRemapBcb (
    IN PVOID Bcb
    )
{
	UNIMPLEMENTED;

    return 0;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcScheduleReadAhead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcSetAdditionalCacheAttributes (
	IN	PFILE_OBJECT	FileObject,
	IN	BOOLEAN		DisableReadAhead,
	IN	BOOLEAN		DisableWriteBehind
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcSetBcbOwnerPointer (
	IN	PVOID	Bcb,
	IN	PVOID	Owner
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcSetDirtyPageThreshold (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		DirtyPageThreshold
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
CcSetReadAheadGranularity (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		Granularity
	)
{
	UNIMPLEMENTED;
}
