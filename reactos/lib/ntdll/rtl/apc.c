/* $Id: apc.c,v 1.3 2000/06/29 23:35:29 dwelch Exp $
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
   ApcRoutine(ApcContext,
	      Iosb,
	      Reserved);;
   NtContinue(Context, 1);
}

