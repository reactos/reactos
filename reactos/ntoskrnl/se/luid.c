/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/luid.c
 * PURPOSE:         Security manager
 * 
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static KSPIN_LOCK LuidLock;
static LARGE_INTEGER LuidIncrement;
static LARGE_INTEGER LuidValue;

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
SepInitLuid(VOID)
{
  LUID DummyLuidValue = SYSTEM_LUID;
  
  KeInitializeSpinLock(&LuidLock);
  LuidValue.u.HighPart = DummyLuidValue.HighPart;
  LuidValue.u.LowPart = DummyLuidValue.LowPart;
  LuidIncrement.QuadPart = 1;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtAllocateLocallyUniqueId(OUT LUID *LocallyUniqueId)
{
  LARGE_INTEGER ReturnedLuid;
  KIRQL Irql;

  KeAcquireSpinLock(&LuidLock,
		    &Irql);
  ReturnedLuid = LuidValue;
  LuidValue = RtlLargeIntegerAdd(LuidValue,
				 LuidIncrement);
  KeReleaseSpinLock(&LuidLock,
		    Irql);

  LocallyUniqueId->LowPart = ReturnedLuid.u.LowPart;
  LocallyUniqueId->HighPart = ReturnedLuid.u.HighPart;

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
VOID STDCALL
RtlCopyLuid(IN PLUID LuidDest,
	    IN PLUID LuidSrc)
{
  LuidDest->LowPart = LuidSrc->LowPart;
  LuidDest->HighPart = LuidSrc->HighPart;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlEqualLuid(IN PLUID Luid1,
	     IN PLUID Luid2)
{
  return (Luid1->LowPart == Luid2->LowPart &&
	  Luid1->HighPart == Luid2->HighPart);
}

/* EOF */
