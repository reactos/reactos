/* $Id: mp.c,v 1.5 2000/07/02 10:49:04 ekohl Exp $
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
