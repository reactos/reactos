/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/gdidbg.c
 * PURPOSE:         Special debugging functions for gdi
 * PROGRAMMERS:     Timo Kreuzer
 */

/** INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>


ULONG gulDebugChannels = 0;

#ifdef GDI_DEBUG

ULONG_PTR GDIHandleAllocator[GDI_HANDLE_COUNT][GDI_STACK_LEVELS+1];
ULONG_PTR GDIHandleLocker[GDI_HANDLE_COUNT][GDI_STACK_LEVELS+1];
ULONG_PTR GDIHandleShareLocker[GDI_HANDLE_COUNT][GDI_STACK_LEVELS+1];
ULONG_PTR GDIHandleDeleter[GDI_HANDLE_COUNT][GDI_STACK_LEVELS+1];
struct DbgOpenGDIHandle
{
    ULONG idx;
    int count;
};
#define MAX_BACKTRACES 1024
static struct DbgOpenGDIHandle AllocatorTable[MAX_BACKTRACES];

static
BOOL
CompareBacktraces(ULONG idx1, ULONG idx2)
{
    ULONG iLevel;

    /* Loop all stack levels */
    for (iLevel = 0; iLevel < GDI_STACK_LEVELS; iLevel++)
    {
        if (GDIHandleAllocator[idx1][iLevel]
                != GDIHandleAllocator[idx2][iLevel])
//        if (GDIHandleShareLocker[idx1][iLevel]
//                != GDIHandleShareLocker[idx2][iLevel])
        {
            return FALSE;
        }
    }

    return TRUE;
}

void IntDumpHandleTable(PGDI_HANDLE_TABLE HandleTable)
{
    static int leak_reported = 0;
    int i, j, idx, nTraces = 0;
    KIRQL OldIrql;

    if (leak_reported)
    {
        DPRINT1("gdi handle abusers already reported!\n");
        return;
    }

    leak_reported = 1;
    DPRINT1("reporting gdi handle abusers:\n");

    /* We've got serious business to do */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /* Step through GDI handle table and find out who our culprit is... */
    for (idx = RESERVE_ENTRIES_COUNT; idx < GDI_HANDLE_COUNT; idx++)
    {
        /* If the handle is free, continue */
        if (!IS_HANDLE_VALID(idx)) continue;

        /* Step through all previous backtraces */
        for (j = 0; j < nTraces; j++)
        {
            /* Check if the backtrace matches */
            if (CompareBacktraces(idx, AllocatorTable[j].idx))
            {
                /* It matches, increment count and break out */
                AllocatorTable[j].count++;
                break;
            }
        }

        /* Did we find a new backtrace? */
        if (j == nTraces)
        {
            /* Break out, if we reached the maximum */
            if (nTraces == MAX_BACKTRACES) break;

            /* Initialize this entry */
            AllocatorTable[j].idx = idx;
            AllocatorTable[j].count = 1;
            nTraces++;
        }
    }

    /* bubble sort time! weeeeee!! */
    for (i = 0; i < nTraces-1; i++)
    {
        if (AllocatorTable[i].count < AllocatorTable[i+1].count)
        {
            struct DbgOpenGDIHandle temp;

            temp = AllocatorTable[i+1];
            AllocatorTable[i+1] = AllocatorTable[i];
            j = i;
            while (j > 0 && AllocatorTable[j-1].count < temp.count)
                j--;
            AllocatorTable[j] = temp;
        }
    }

    /* Print the worst offenders... */
    DbgPrint("Worst GDI Handle leak offenders (out of %i unique locations):\n", nTraces);
    for (i = 0; i < nTraces && AllocatorTable[i].count > 1; i++)
    {
        /* Print out the allocation count */
        DbgPrint(" %i allocs, type = 0x%lx:\n",
                 AllocatorTable[i].count,
                 GdiHandleTable->Entries[AllocatorTable[i].idx].Type);

        /* Dump the frames */
        KeRosDumpStackFrames(GDIHandleAllocator[AllocatorTable[i].idx], GDI_STACK_LEVELS);
        //KeRosDumpStackFrames(GDIHandleShareLocker[AllocatorTable[i].idx], GDI_STACK_LEVELS);

        /* Print new line for better readability */
        DbgPrint("\n");
    }

    if (i < nTraces)
        DbgPrint("(list terminated - the remaining entries have 1 allocation only)\n");

    KeLowerIrql(OldIrql);

    ASSERT(FALSE);
}

ULONG
CaptureStackBackTace(PVOID* pFrames, ULONG nFramesToCapture)
{
    ULONG nFrameCount;

    memset(pFrames, 0x00, (nFramesToCapture + 1) * sizeof(PVOID));

    nFrameCount = RtlWalkFrameChain(pFrames, nFramesToCapture, 0);

    if (nFrameCount < nFramesToCapture)
    {
        nFrameCount += RtlWalkFrameChain(pFrames + nFrameCount,
                                         nFramesToCapture - nFrameCount,
                                         1);
    }

    return nFrameCount;
}

BOOL
GdiDbgHTIntegrityCheck()
{
	ULONG i, nDeleted = 0, nFree = 0, nUsed = 0;
	PGDI_TABLE_ENTRY pEntry;
	BOOL r = 1;

	KeEnterCriticalRegion();

	/* FIXME: check reserved entries */

	/* Now go through the deleted objects */
	i = GdiHandleTable->FirstFree & 0xffff;
	while (i)
	{
		pEntry = &GdiHandleTable->Entries[i];
		if (i > GDI_HANDLE_COUNT)
		{
		    DPRINT1("nDeleted=%ld\n", nDeleted);
		    ASSERT(FALSE);
		}

        nDeleted++;

        /* Check the entry */
        if ((pEntry->Type & GDI_ENTRY_BASETYPE_MASK) != 0)
        {
            r = 0;
            DPRINT1("Deleted Entry has a type != 0\n");
        }
        if ((ULONG_PTR)pEntry->KernelData >= GDI_HANDLE_COUNT)
        {
            r = 0;
            DPRINT1("Deleted entries KernelPointer too big\n");
        }
        if (pEntry->UserData != NULL)
        {
            r = 0;
            DPRINT1("Deleted entry has UserData != 0\n");
        }
        if (pEntry->ProcessId != 0)
        {
            r = 0;
            DPRINT1("Deleted entry has ProcessId != 0\n");
        }

        i = (ULONG_PTR)pEntry->KernelData & 0xffff;
	};

	for (i = GdiHandleTable->FirstUnused;
	     i < GDI_HANDLE_COUNT;
	     i++)
	{
		pEntry = &GdiHandleTable->Entries[i];

		if ((pEntry->Type) != 0)
		{
			r = 0;
			DPRINT1("Free Entry has a type != 0\n");
		}
		if ((ULONG_PTR)pEntry->KernelData != 0)
		{
			r = 0;
			DPRINT1("Free entries KernelPointer != 0\n");
		}
		if (pEntry->UserData != NULL)
		{
			r = 0;
			DPRINT1("Free entry has UserData != 0\n");
		}
		if (pEntry->ProcessId != 0)
		{
			r = 0;
			DPRINT1("Free entry has ProcessId != 0\n");
		}
		nFree++;
	}

	for (i = RESERVE_ENTRIES_COUNT; i < GDI_HANDLE_COUNT; i++)
	{
		HGDIOBJ Handle;
		ULONG Type;

		pEntry = &GdiHandleTable->Entries[i];
		Type = pEntry->Type;
		Handle = (HGDIOBJ)((Type << GDI_ENTRY_UPPER_SHIFT) + i);

		if (Type & GDI_ENTRY_BASETYPE_MASK)
		{
			if (pEntry->KernelData == NULL)
			{
				r = 0;
				DPRINT1("Used entry has KernelData == 0\n");
			}
			if (pEntry->KernelData <= MmHighestUserAddress)
			{
				r = 0;
				DPRINT1("Used entry invalid KernelData\n");
			}
			if (((POBJ)(pEntry->KernelData))->hHmgr != Handle)
			{
				r = 0;
				DPRINT1("Used entry %ld, has invalid hHmg %p (expected: %p)\n",
				        i, ((POBJ)(pEntry->KernelData))->hHmgr, Handle);
			}
			nUsed++;
		}
	}

	if (RESERVE_ENTRIES_COUNT + nDeleted + nFree + nUsed != GDI_HANDLE_COUNT)
	{
		r = 0;
		DPRINT1("Number of all entries incorrect: RESERVE_ENTRIES_COUNT = %ld, nDeleted = %ld, nFree = %ld, nUsed = %ld\n",
		        RESERVE_ENTRIES_COUNT, nDeleted, nFree, nUsed);
	}

	KeLeaveCriticalRegion();

	return r;
}

ULONG
FASTCALL
GDIOBJ_IncrementShareCount(POBJ Object)
{
    INT cLocks = InterlockedIncrement((PLONG)&Object->ulShareCount);
    GDIDBG_CAPTURESHARELOCKER(Object->hHmgr);
    ASSERT(cLocks >= 1);
    return cLocks;
}

#endif /* GDI_DEBUG */

void
NTAPI
DbgPreServiceHook(ULONG ulSyscallId, PULONG_PTR pulArguments)
{
    PTHREADINFO pti = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
    if (pti && pti->cExclusiveLocks != 0)
    {
        DbgPrint("FATAL: Win32DbgPreServiceHook(%ld): There are %ld exclusive locks!\n",
                 ulSyscallId, pti->cExclusiveLocks);
        ASSERT(FALSE);
    }

}

ULONG_PTR
NTAPI
DbgPostServiceHook(ULONG ulSyscallId, ULONG_PTR ulResult)
{
    PTHREADINFO pti = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
    if (pti && pti->cExclusiveLocks != 0)
    {
        DbgPrint("FATAL: Win32DbgPostServiceHook(%ld): There are %ld exclusive locks!\n",
                 ulSyscallId, pti->cExclusiveLocks);
        ASSERT(FALSE);
    }
    return ulResult;
}

