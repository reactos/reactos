/* $Id: exception.c,v 1.4 2001/03/17 11:11:11 dwelch Exp $
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

/* FUNCTIONS ***************************************************************/

VOID STDCALL
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
{
}

ULONG
RtlDispatchException(PEXCEPTION_RECORD ExceptionRecord,
		     PCONTEXT Context)
{
  return(0);
}

VOID STDCALL
KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord,
			  PCONTEXT Context)
{
  EXCEPTION_RECORD NestedExceptionRecord;
  NTSTATUS Status;

  if (RtlDispatchException(ExceptionRecord, Context) == 1)
    {
      Status = ZwContinue(Context, FALSE);
    }
  else
    {
      Status = ZwRaiseException(ExceptionRecord, Context, FALSE);
    }

  NestedExceptionRecord.ExceptionCode = Status;
  NestedExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
  NestedExceptionRecord.ExceptionRecord = ExceptionRecord;

  RtlRaiseException(&NestedExceptionRecord);
}

VOID STDCALL
RtlRaiseStatus(NTSTATUS Status)
{

}


VOID STDCALL
RtlUnwind(ULONG Unknown1,
	  ULONG Unknown2,
	  ULONG Unknown3,
	  ULONG Unknown4)
{

}

/* EOF */
