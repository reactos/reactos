/* $Id: critical.c,v 1.13 2003/01/15 21:24:36 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/sync/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
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
