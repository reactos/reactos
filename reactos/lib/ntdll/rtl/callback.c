/* $Id: callback.c,v 1.5 2002/06/18 21:51:09 dwelch Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode callback support
 * FILE:              lib/ntdll/rtl/callback.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>
#include <napi/teb.h>


/* TYPES *********************************************************************/

typedef NTSTATUS STDCALL (*CALLBACK_FUNCTION)(PVOID Argument, 
					      ULONG ArgumentLength);

/* FUNCTIONS *****************************************************************/

VOID STDCALL
KiUserCallbackDispatcher(ULONG RoutineIndex,
			 PVOID Argument,
			 ULONG ArgumentLength)
{
   PPEB Peb;
   NTSTATUS Status;
   CALLBACK_FUNCTION Callback;
   
   Peb = NtCurrentPeb();
   Callback = (CALLBACK_FUNCTION)Peb->KernelCallbackTable[RoutineIndex];
   DbgPrint("KiUserCallbackDispatcher(%d, %x, %d)\n", RoutineIndex,
	    Argument, ArgumentLength);
   Status = Callback(Argument, ArgumentLength);
   DbgPrint("KiUserCallbackDispatcher() finished.\n");
   ZwCallbackReturn(NULL, 0, Status);
}
