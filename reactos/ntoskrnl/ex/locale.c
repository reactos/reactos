/* $Id: locale.c,v 1.4 2000/12/23 02:37:39 dwelch Exp $
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
NtQueryDefaultLocale(IN BOOLEAN ThreadOrSystem,
		     OUT PLCID DefaultLocaleId)
/*
 * Returns the default locale.
 * 
 * THREADORSYSTEM = If TRUE then the locale for this thread is returned,
 * otherwise the locale for the system is returned.
 * 
 * DEFAUTLOCALEID = Points to a variable that receives the locale id.
 * 
 * Returns:
 * 
 * Status.
 */
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL
NtSetDefaultLocale(IN BOOLEAN ThreadOrSystem,
		   IN LCID DefaultLocaleId)
/*
 * Sets the default locale.
 * 
 * THREADORSYSTEM = If TRUE then the thread's locale is set, otherwise the
 * sytem locale is set.
 * 
 * DEFAUTLOCALEID = The locale id to be set
 * 
 * Returns:
 * 
 * Status
 */
{
   UNIMPLEMENTED;
}

/* EOF */
