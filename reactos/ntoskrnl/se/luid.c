/* $Id: luid.c,v 1.3 1999/12/29 01:36:06 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              ntoskrnl/se/luid.c
 * PROGRAMER:         ?
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK LuidLock;
static LARGE_INTEGER LuidIncrement;
static LUID Luid;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtAllocateLocallyUniqueId(OUT LUID* LocallyUniqueId)
{
   KIRQL oldIrql;
   LUID ReturnedLuid;
   
   KeAcquireSpinLock(&LuidLock, &oldIrql);
   ReturnedLuid = Luid;
   Luid = RtlLargeIntegerAdd(Luid, LuidIncrement);
   KeReleaseSpinLock(&LuidLock, oldIrql);
   *LocallyUniqueId = ReturnedLuid;
   return(STATUS_SUCCESS);
}

VOID
STDCALL
RtlCopyLuid (
	PLUID LuidDest,
	PLUID LuidSrc
	)
{
	LuidDest->QuadPart = LuidSrc->QuadPart;
}

BOOLEAN
STDCALL
RtlEqualLuid (
	PLUID	Luid1,
	PLUID	Luid2
	)
{
	return ((Luid1->QuadPart == Luid2->QuadPart) ? TRUE : FALSE);
}

/* EOF */
