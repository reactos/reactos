/* $Id$
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
#include "../include/debug.h"


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
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

/*
 * @implemented
 */
BOOL
STDCALL
InitializeCriticalSectionAndSpinCount(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
    )
{
    NTSTATUS Status;
    
    Status = RtlInitializeCriticalSectionAndSpinCount((PRTL_CRITICAL_SECTION)lpCriticalSection, dwSpinCount);
    if (Status)
      {
         RtlRaiseStatus(Status);
      }
    return NT_SUCCESS(Status);
}

/* EOF */
