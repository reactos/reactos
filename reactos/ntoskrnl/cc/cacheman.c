/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/cacheman.c
 * PURPOSE:         Cache manager
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
CcInit(VOID)
{
  CcInitView();
}

/*
 * @unimplemented
 */
LARGE_INTEGER
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
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
STDCALL
CcSetReadAheadGranularity (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		Granularity
	)
{
	UNIMPLEMENTED;
}
