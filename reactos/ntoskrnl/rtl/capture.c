/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/capture.c
 * PURPOSE:         Helper routines for system calls.
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                02/09/01: Created
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
RtlCaptureUnicodeString(OUT PUNICODE_STRING Dest,
	                IN KPROCESSOR_MODE CurrentMode,
	                IN POOL_TYPE PoolType,
	                IN BOOLEAN CaptureIfKernel,
			IN PUNICODE_STRING UnsafeSrc)
{
  UNICODE_STRING Src;
  NTSTATUS Status = STATUS_SUCCESS;
  
  ASSERT(Dest != NULL);
  
  /*
   * Copy the source string structure to kernel space.
   */
  
  if(CurrentMode == UserMode)
  {
    _SEH_TRY
    {
      ProbeForRead(UnsafeSrc,
                   sizeof(UNICODE_STRING),
                   sizeof(ULONG));
      Src = *UnsafeSrc;
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
  else if(!CaptureIfKernel)
  {
    /* just copy the UNICODE_STRING structure, the pointers are considered valid */
    *Dest = *UnsafeSrc;
    return STATUS_SUCCESS;
  }
  else
  {
    /* capture the string even though it is considered to be valid */
    Src = *UnsafeSrc;
  }
  
  /*
   * Initialize the destination string.
   */
  Dest->Length = Src.Length;
  Dest->MaximumLength = Src.Length + sizeof(WCHAR);
  Dest->Buffer = ExAllocatePool(PoolType, Dest->MaximumLength);
  if (Dest->Buffer == NULL)
  {
    Dest->Length = Dest->MaximumLength = 0;
    Dest->Buffer = NULL;
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  /*
   * Copy the source string to kernel space.
   */
  if(Src.Length > 0)
  {
    _SEH_TRY
    {
      RtlCopyMemory(Dest->Buffer, Src.Buffer, Src.Length);
      Dest->Buffer[Src.Length / sizeof(WCHAR)] = L'\0';
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
  }
  
  return Status;
}

VOID
RtlRelaseCapturedUnicodeString(IN PUNICODE_STRING CapturedString,
	                       IN KPROCESSOR_MODE CurrentMode,
	                       IN BOOLEAN CaptureIfKernel)
{
  if(CurrentMode != KernelMode || CaptureIfKernel )
  {
    RtlFreeUnicodeString(CapturedString);
  }
}

NTSTATUS
RtlCaptureAnsiString(PANSI_STRING Dest,
		     PANSI_STRING UnsafeSrc)
{
  PANSI_STRING Src; 
  NTSTATUS Status;
  
  /*
   * Copy the source string structure to kernel space.
   */
  Status = MmCopyFromCaller(&Src, UnsafeSrc, sizeof(ANSI_STRING));
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  /*
   * Initialize the destination string.
   */
  Dest->Length = Src->Length;
  Dest->MaximumLength = Src->MaximumLength;
  Dest->Buffer = ExAllocatePool(NonPagedPool, Dest->MaximumLength);
  if (Dest->Buffer == NULL)
    {
      return(Status);
    }

  /*
   * Copy the source string to kernel space.
   */
  Status = MmCopyFromCaller(Dest->Buffer, Src->Buffer, Dest->Length);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(Dest->Buffer);
      return(Status);
    }

  return(STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
VOID
STDCALL
RtlCaptureContext (
	OUT PCONTEXT ContextRecord
	)
{
	UNIMPLEMENTED;
}

/*
* @unimplemented
*/
USHORT
STDCALL
RtlCaptureStackBackTrace (
	IN ULONG FramesToSkip,
	IN ULONG FramesToCapture,
	OUT PVOID *BackTrace,
	OUT PULONG BackTraceHash OPTIONAL
	)
{
	UNIMPLEMENTED;
	return 0;
}

/* EOF */

