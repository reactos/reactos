/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/lazyrite.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
//#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KEVENT CcpLazyWriteEvent;

/* FUNCTIONS ******************************************************************/

VOID STDCALL
CcpLazyWriteThread(PVOID Unused)
{
    ULONG i;
    BOOLEAN GotLock, WasAlloc;
    PNOCC_BCB RealBcb;
    LARGE_INTEGER Interval;
    IO_STATUS_BLOCK IoStatus;

    while (TRUE)
    {
	for (i = 0; i < CACHE_NUM_SECTIONS; i++)
	{
	    RealBcb = &CcCacheSections[i];
	    CcpLock(&CcMutex);
	    GotLock = RealBcb->RefCount == 1;
	    WasAlloc = RtlTestBit(CcCacheBitmap, i);
	    if (GotLock) 
	    {
		RtlSetBit(CcCacheBitmap, i);
		RealBcb->RefCount++;
	    }
	    CcpUnlock(&CcMutex);
	    // Pinned (temporarily)
	    if (GotLock)
	    {
		DPRINT("CcpLazyWrite: Flush #%x\n", i);
		CcFlushCache
		    (RealBcb->FileObject->SectionObjectPointer,
		     &RealBcb->FileOffset,
		     RealBcb->Length,
		     &IoStatus);

		DPRINT
		    ("CcpLazyWriteThread UnmapIoStatus.Status %08x IoStatus.Information %08x\n",
		     IoStatus.Status,
		     IoStatus.Information);

		CcpLock(&CcMutex);
		RealBcb->RefCount--;
		if (!WasAlloc) RtlClearBit(CcCacheBitmap, i);
		CcpUnlock(&CcMutex);
		DPRINT("CcpLazyWrite: done #%x\n", i);
	    }
	}
	KeSetEvent(&CcpLazyWriteEvent, IO_DISK_INCREMENT, TRUE);
	Interval.QuadPart = -100000000L;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }
}

NTSTATUS
NTAPI
CcWaitForCurrentLazyWriterActivity(VOID)
{
    KeWaitForSingleObject(&CcpLazyWriteEvent, Executive, KernelMode, FALSE, NULL);
    return STATUS_SUCCESS;
}

/* EOF */
