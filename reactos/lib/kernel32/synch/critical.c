/* $Id: critical.c,v 1.12 2002/09/08 10:22:45 chorns Exp $
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
