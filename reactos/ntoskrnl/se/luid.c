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

static LARGE_INTEGER LuidIncrement;
static LARGE_INTEGER LuidValue;

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
SepInitLuid(VOID)
{
  LUID DummyLuidValue = SYSTEM_LUID;
  
  LuidValue.u.HighPart = DummyLuidValue.HighPart;
  LuidValue.u.LowPart = DummyLuidValue.LowPart;
  LuidIncrement.QuadPart = 1;
}


NTSTATUS
ExpAllocateLocallyUniqueId(OUT LUID *LocallyUniqueId)
{
  LARGE_INTEGER NewLuid, PrevLuid;
  
  /* atomically increment the luid */
  do
  {
    PrevLuid = (volatile LARGE_INTEGER)LuidValue;
    NewLuid = RtlLargeIntegerAdd(PrevLuid,
                                 LuidIncrement);
  } while(ExfInterlockedCompareExchange64(&LuidValue.QuadPart,
                                          &NewLuid.QuadPart,
                                          &PrevLuid.QuadPart) != PrevLuid.QuadPart);

  LocallyUniqueId->LowPart = NewLuid.u.LowPart;
  LocallyUniqueId->HighPart = NewLuid.u.HighPart;
  
  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtAllocateLocallyUniqueId(OUT LUID *LocallyUniqueId)
{
  LUID NewLuid;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;
  
  PreviousMode = ExGetPreviousMode();
  
  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeForWrite(LocallyUniqueId,
                    sizeof(LUID),
                    sizeof(ULONG));
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    
    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

  Status = ExpAllocateLocallyUniqueId(&NewLuid);

  _SEH_TRY
  {
    *LocallyUniqueId = NewLuid;
  }
  _SEH_HANDLE
  {
    Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  return Status;
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
