/* $Id: luid.c,v 1.5 2002/09/07 15:13:06 chorns Exp $
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

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

static KSPIN_LOCK LuidLock;
static LARGE_INTEGER LuidIncrement;
static LUID Luid;

/* FUNCTIONS *****************************************************************/

VOID
SepInitLuid(VOID)
{
  KeInitializeSpinLock(&LuidLock);
  Luid.QuadPart = 999;   /* SYSTEM_LUID */
  LuidIncrement.QuadPart = 1;
}


NTSTATUS STDCALL
NtAllocateLocallyUniqueId(OUT LUID* LocallyUniqueId)
{
  KIRQL oldIrql;
  LUID ReturnedLuid;

  KeAcquireSpinLock(&LuidLock,
		    &oldIrql);
  ReturnedLuid = Luid;
  Luid = RtlLargeIntegerAdd(Luid,
			    LuidIncrement);
  KeReleaseSpinLock(&LuidLock,
		    oldIrql);
  *LocallyUniqueId = ReturnedLuid;

  return(STATUS_SUCCESS);
}


VOID STDCALL
RtlCopyLuid(IN PLUID LuidDest,
	    IN PLUID LuidSrc)
{
  LuidDest->QuadPart = LuidSrc->QuadPart;
}

#undef RtlEqualLuid

BOOLEAN STDCALL
RtlEqualLuid(IN PLUID Luid1,
	     IN PLUID Luid2)
{
  return((Luid1->QuadPart == Luid2->QuadPart) ? TRUE : FALSE);
}

/* EOF */
