/* $Id: exception.c,v 1.11 2002/10/26 00:32:18 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode exception support
 * FILE:              lib/ntdll/rtl/exception.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

ULONG
RtlpDispatchException(IN PEXCEPTION_RECORD  ExceptionRecord,
	IN PCONTEXT  Context);

VOID STDCALL
KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord,
			  PCONTEXT Context)
{
  EXCEPTION_RECORD NestedExceptionRecord;
  NTSTATUS Status;

  DPRINT("KiUserExceptionDispatcher()\n");

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

VOID STDCALL
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
{
	DbgPrint("RtlRaiseException()");
}

/* EOF */
