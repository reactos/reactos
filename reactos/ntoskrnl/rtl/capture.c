/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/capture.c
 * PURPOSE:         Helper routines for system calls.
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
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

  if(CurrentMode != KernelMode)
  {
    RtlZeroMemory(&Src, sizeof(Src));

    _SEH_TRY
    {
      ProbeForRead(UnsafeSrc,
                   sizeof(UNICODE_STRING),
                   sizeof(ULONG));
      Src = *UnsafeSrc;
      if(Src.Length > 0)
      {
        ProbeForRead(Src.Buffer,
                     Src.Length,
                     sizeof(WCHAR));
      }
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
  if(Src.Length > 0)
  {
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

    if(!NT_SUCCESS(Status))
    {
      ExFreePool(Dest->Buffer);
      Dest->Buffer = NULL;
      Dest->Length = Dest->MaximumLength = 0;
    }
  }
  else
  {
    Dest->MaximumLength = 0;
    Dest->Buffer = NULL;
  }

  return Status;
}

VOID
RtlReleaseCapturedUnicodeString(IN PUNICODE_STRING CapturedString,
	                        IN KPROCESSOR_MODE CurrentMode,
	                        IN BOOLEAN CaptureIfKernel)
{
  if(CurrentMode != KernelMode || CaptureIfKernel )
  {
    ExFreePool(CapturedString->Buffer);
  }
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

