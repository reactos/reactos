/* $Id: mp.c,v 1.4 2000/04/05 15:49:52 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/mp.c
 * PURPOSE:         Multiprocessor stubs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

ULONG KeGetCurrentProcessorNumber(VOID)
/*
 * FUNCTION: Returns the system assigned number of the current processor
 */
{
   return(0);
}

VOID
STDCALL
HalInitializeProcessor (
	ULONG	ProcessorNumber
	)
{
	return;
}

BOOLEAN
STDCALL
HalAllProcessorsStarted (
	VOID
	)
{
	return TRUE;
}

BOOLEAN
STDCALL
HalStartNextProcessor (
	ULONG	Unknown1,
	ULONG	Unknown2
	)
{
	return FALSE;
}

/* EOF */
