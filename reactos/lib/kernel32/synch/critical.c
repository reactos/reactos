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

/* FUNCTIONS *****************************************************************/

VOID DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   lpCriticalSection->Reserved = -1;
}

VOID EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
}

VOID InitializeCriticalSection(LPCRITICAL_SECTION pcritical)
{
   pcritical->LockCount = -1;
   pcritical->RecursionCount = 0;
   pcritical->LockSemaphore = NULL;
   pcritical->OwningThread = (HANDLE)-1;
   pcritical->Reserved = 0;
}

VOID LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
}

BOOL TryEntryCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
}
