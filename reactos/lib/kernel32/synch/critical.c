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

#include <kernel32/kernel32.h>

/* FUNCTIONS *****************************************************************/

VOID DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
}

VOID EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
}

VOID InitializeCriticalSection(LPCRITICAL_SECTION pcritical)
{
   UNIMPLEMENTED;
}

VOID LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
}

WINBOOL TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
   return(FALSE);
}

