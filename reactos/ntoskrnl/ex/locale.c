/* $Id: locale.c,v 1.3 2000/10/06 22:54:41 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/locale.c
 * PURPOSE:         Locale support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtQueryDefaultLocale(IN BOOLEAN UserProfile,
		     OUT PLCID DefaultLocaleId)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL
NtSetDefaultLocale(IN BOOLEAN UserProfile,
		   IN LCID DefaultLocaleId)
{
   UNIMPLEMENTED;
}

/* EOF */
