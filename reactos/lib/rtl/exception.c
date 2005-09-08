/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         User-mode exception support
 * FILE:            lib/rtl/exception.c
 * PROGRAMERS:      David Welch <welch@cwcom.net>
 *                  Skywing <skywing@valhallalegends.com>
 *                  KJK::Hyperion <noog@libero.it>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/* implemented in except.s */
VOID
RtlpCaptureContext(PCONTEXT Context);

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlDispatchException(
	PEXCEPTION_RECORD pExcptRec,
	CONTEXT * pContext
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
VOID
STDCALL
RtlGetCallersAddress(
	OUT PVOID *CallersAddress,
	OUT PVOID *CallersCaller
	)
{
	UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID STDCALL
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
{
  CONTEXT Context;
  NTSTATUS Status;

  RtlpCaptureContext(&Context);

  ExceptionRecord->ExceptionAddress = (PVOID)(*(((PULONG)Context.Ebp)+1));
  Context.ContextFlags = CONTEXT_FULL;

  Status = NtRaiseException(ExceptionRecord, &Context, TRUE);
  RtlRaiseException(ExceptionRecord);
  RtlRaiseStatus(Status);
}


/*
 * @implemented
 */
VOID STDCALL
RtlRaiseStatus(NTSTATUS Status)
{
  EXCEPTION_RECORD ExceptionRecord;

  DPRINT("RtlRaiseStatus(Status 0x%.08x)\n", Status);

  ExceptionRecord.ExceptionCode    = Status;
  ExceptionRecord.ExceptionRecord  = NULL;
  ExceptionRecord.NumberParameters = 0;
  ExceptionRecord.ExceptionFlags   = EXCEPTION_NONCONTINUABLE;
  RtlRaiseException (& ExceptionRecord);
}

/*
* @unimplemented
*/
ULONG
STDCALL
RtlWalkFrameChain (
	OUT PVOID *Callers,
	IN ULONG Count,
	IN ULONG Flags
	)
{
	UNIMPLEMENTED;
	return 0;
}


/* EOF */
