/* $Id: critical.c,v 1.10 2001/02/17 00:02:13 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/sync/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <windows.h>

#include <kernel32/kernel32.h>


/* FUNCTIONS *****************************************************************/

VOID STDCALL
InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   NTSTATUS Status;

   Status = RtlInitializeCriticalSection(lpCriticalSection);
   if (!NT_SUCCESS(Status))
     {
	RtlRaiseStatus(Status);
     }
}

/* EOF */
