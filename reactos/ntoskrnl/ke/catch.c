/*
 *  ReactOS kernel
 *  Copyright (C) 2000  ReactOS Team
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
/* $Id: catch.c,v 1.12 2001/03/17 11:11:11 dwelch Exp $
 *
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/catch.c
 * PURPOSE:              Exception handling
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ldr.h>

#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

VOID 
KiDispatchException(PEXCEPTION_RECORD Er,
		    ULONG Reserved,
		    PKTRAP_FRAME Tf,
		    KPROCESSOR_MODE PreviousMode,
		    BOOLEAN SearchFrames)
{
  CONTEXT Context;

  Context.ContextFlags = CONTEXT_FULL;
  if (PreviousMode == UserMode)
    {
      Context.ContextFlags = Context.ContextFlags | CONTEXT_DEBUGGER;
    }
  
  /* PCR->KeExceptionDispatchCount++; */

  //  KeContextFromKframes(Tf, Reserved, &Context);

  if (Er->ExceptionCode == STATUS_BREAKPOINT) 
    {
      Context.Eip--;
    }

  if (PreviousMode == UserMode)
    {
      if (SearchFrames)
	{
	  PULONG Stack;
	  ULONG CDest;

	  /* FIXME: Give the kernel debugger a chance */

	  /* FIXME: Forward exception to user mode debugger */

	  /* FIXME: Check user mode stack for enough space */

	  
	  /*
	   * Let usermode try and handle the exception
	   */
	  Tf->Esp = Tf->Esp - 
	    (12 + sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT));
	  Stack = (PULONG)Tf->Esp;
	  CDest = 3 + (ROUND_UP(sizeof(EXCEPTION_RECORD), 4) / 4);
	  /* Return address */
	  Stack[0] = 0;    
	  /* Pointer to EXCEPTION_RECORD structure */
	  Stack[1] = (ULONG)&Stack[3];   
	  /* Pointer to CONTEXT structure */
	  Stack[2] = (ULONG)&Stack[CDest];     
	  memcpy(&Stack[3], Er, sizeof(EXCEPTION_RECORD));
	  memcpy(&Stack[CDest], &Context, sizeof(CONTEXT));

	  Tf->Eip = (ULONG)LdrpGetSystemDllExceptionDispatcher();
	  return;
	}
      
      /* FIXME: Forward the exception to the debugger */

      /* FIXME: Forward the exception to the debugger */

      ZwTerminateThread(NtCurrentThread(), Er->ExceptionCode);

      KeBugCheck(KMODE_EXCEPTION_NOT_HANDLED);
    }

}

VOID STDCALL
ExRaiseAccessViolation (VOID)
{
  ExRaiseStatus (STATUS_ACCESS_VIOLATION);
}

VOID STDCALL
ExRaiseDatatypeMisalignment (VOID)
{
  ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
}

VOID STDCALL
ExRaiseStatus (IN	NTSTATUS	Status)
{
  DbgPrint("ExRaiseStatus(%x)\n",Status);
  for(;;);
}



NTSTATUS STDCALL
NtRaiseException (IN	PEXCEPTION_RECORD	ExceptionRecord,
		  IN	PCONTEXT		Context,
		  IN	BOOL			IsDebugger	OPTIONAL)
{
  UNIMPLEMENTED;
}


VOID STDCALL
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
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
