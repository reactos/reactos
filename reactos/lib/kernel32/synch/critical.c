/* $Id: critical.c,v 1.5 1999/08/29 06:59:03 ea Exp $
 *
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


VOID
STDCALL
DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
}


VOID
STDCALL
EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
}


VOID
STDCALL
InitializeCriticalSection(LPCRITICAL_SECTION pcritical)
{
   UNIMPLEMENTED;
}


VOID
STDCALL
LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
}


WINBOOL
STDCALL
TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
   return(FALSE);
}


/* EOF */
