/* $Id: exception.c,v 1.17 2004/04/08 21:14:18 navaraf Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode exception support
 * FILE:              lib/ntdll/rtl/exception.c
 * PROGRAMERS:        David Welch <welch@cwcom.net>
 *                    Skywing <skywing@valhallalegends.com>
 * UPDATES:           Skywing, 09/11/2003: Implemented RtlRaiseException and
 *                    KiUserRaiseExceptionDispatcher.
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <string.h>
#include <napi/teb.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

VOID STDCALL
RtlBaseProcessStart(PTHREAD_START_ROUTINE StartAddress,
  PVOID Parameter);

__declspec(dllexport)
PRTL_BASE_PROCESS_START_ROUTINE RtlBaseProcessStartRoutine = RtlBaseProcessStart;

ULONG
RtlpDispatchException(IN PEXCEPTION_RECORD  ExceptionRecord,
	IN PCONTEXT  Context);

VOID STDCALL
KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord,
			  PCONTEXT Context)
{
  EXCEPTION_RECORD NestedExceptionRecord;
  NTSTATUS Status;

  if (RtlpDispatchException(ExceptionRecord, Context) != ExceptionContinueExecution)
    {
      Status = NtContinue(Context, FALSE);
    }
  else
    {
      Status = NtRaiseException(ExceptionRecord, Context, FALSE);
    }

  NestedExceptionRecord.ExceptionCode = Status;
  NestedExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
  NestedExceptionRecord.ExceptionRecord = ExceptionRecord;
  NestedExceptionRecord.NumberParameters = Status;

  RtlRaiseException(&NestedExceptionRecord);
}

/* implemented in except.s */
VOID
RtlpCaptureContext(PCONTEXT Context);

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

  Status = ZwRaiseException(ExceptionRecord, &Context, TRUE);
  RtlRaiseException(ExceptionRecord);
  RtlRaiseStatus(Status); /* If we get to this point, something is seriously wrong... */
}

/*
 * @implemented
 */
VOID STDCALL
KiRaiseUserExceptionDispatcher(VOID)
{
  EXCEPTION_RECORD ExceptionRecord;

  ExceptionRecord.ExceptionCode = ((PTEB)NtCurrentTeb())->ExceptionCode;
  ExceptionRecord.ExceptionFlags = 0;
  ExceptionRecord.ExceptionRecord = NULL;
  ExceptionRecord.NumberParameters = 0;

  RtlRaiseException(&ExceptionRecord);
}

VOID STDCALL
RtlBaseProcessStart(PTHREAD_START_ROUTINE StartAddress,
  PVOID Parameter)
{
  NTSTATUS ExitStatus = STATUS_SUCCESS;

  ExitStatus = (NTSTATUS) (StartAddress)(Parameter);

  NtTerminateProcess(NtCurrentProcess(), ExitStatus);
}

/* EOF */
