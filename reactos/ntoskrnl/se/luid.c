/* $Id: luid.c,v 1.8 2003/07/11 01:23:16 royce Exp $
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
static LARGE_INTEGER LuidValue;

#define SYSTEM_LUID   0x3E7;

/* FUNCTIONS *****************************************************************/

VOID
SepInitLuid(VOID)
{
  KeInitializeSpinLock(&LuidLock);
  LuidValue.QuadPart = SYSTEM_LUID;
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
