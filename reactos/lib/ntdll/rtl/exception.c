/* $Id: exception.c,v 1.3 2000/12/29 13:48:30 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode exception support
 * FILE:              lib/ntdll/rtl/exception.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>

/* FUNCTIONS ***************************************************************/

VOID STDCALL
KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord,
			  PCONTEXT Context)
{
   
}


VOID STDCALL
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
{

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
