/* $Id: critical.c,v 1.11 2002/09/07 15:12:28 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/sync/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <kernel32.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <kernel32/kernel32.h>


/* FUNCTIONS *****************************************************************/

VOID STDCALL
InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   NTSTATUS Status;

   Status = RtlInitializeCriticalSection((PRTL_CRITICAL_SECTION)lpCriticalSection);
   if (!NT_SUCCESS(Status))
     {
	RtlRaiseStatus(Status);
     }
}

/* EOF */
