/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/sync/critical.c
 * PURPOSE:         Critical regions
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ntdll/rtl.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

VOID RtlDeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   lpCriticalSection->Reserved = -1;
}

VOID RtlEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
}

VOID RtlInitializeCriticalSection(LPCRITICAL_SECTION pcritical)
{
   pcritical->LockCount = -1;
   pcritical->RecursionCount = 0;
   pcritical->LockSemaphore = NULL;
   pcritical->OwningThread = (HANDLE)-1;
   pcritical->Reserved = 0;
}

VOID RtlLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
}

WINBOOL RtlTryEntryCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
   for(;;);
}

