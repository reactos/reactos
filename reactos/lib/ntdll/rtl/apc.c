/* $Id: apc.c,v 1.4 2000/10/11 20:50:32 dwelch Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode APC support
 * FILE:              lib/ntdll/rtl/apc.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>

/* FUNCTIONS ***************************************************************/

VOID STDCALL KiUserApcDispatcher(PIO_APC_ROUTINE ApcRoutine,
				 PVOID ApcContext,
				 PIO_STATUS_BLOCK Iosb,
				 ULONG Reserved,
				 PCONTEXT Context)
{
   DbgPrint("KiUserApcDispatcher(ApcRoutine %x, ApcContext %x, Context %x)\n",
	    ApcRoutine, ApcContext, Context);
   ApcRoutine(ApcContext,
	      Iosb,
	      Reserved);
   DbgPrint("Done ApcRoutine, Context %x, Context->Eip %x\n",
	    Context, Context->Eip);
   NtContinue(Context, 1);
   DbgPrint("Returned from NtContinue, aargh\n");
}

