/* $Id: apc.c,v 1.2 2000/06/27 19:20:43 dwelch Exp $
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

#include <internal/debug.h>

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

