/* $Id: callback.c,v 1.6 2002/09/07 15:12:40 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode callback support
 * FILE:              lib/ntdll/rtl/callback.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#define NTOS_USER_MODE
#include <ntos.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

/* TYPES *********************************************************************/

typedef NTSTATUS STDCALL (*USER_CALLBACK_FUNCTION)(PVOID Argument, 
					      ULONG ArgumentLength);

/* FUNCTIONS *****************************************************************/

VOID STDCALL
KiUserCallbackDispatcher(ULONG RoutineIndex,
			 PVOID Argument,
			 ULONG ArgumentLength)
{
   PPEB Peb;
   NTSTATUS Status;
   USER_CALLBACK_FUNCTION Callback;
   
   Peb = NtCurrentPeb();
   Callback = (USER_CALLBACK_FUNCTION)Peb->KernelCallbackTable[RoutineIndex];
   DbgPrint("KiUserCallbackDispatcher(%d, %x, %d)\n", RoutineIndex,
	    Argument, ArgumentLength);
   Status = Callback(Argument, ArgumentLength);
   DbgPrint("KiUserCallbackDispatcher() finished.\n");
   ZwCallbackReturn(NULL, 0, Status);
}
