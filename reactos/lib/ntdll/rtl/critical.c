/* $Id: critical.c,v 1.5 2000/03/09 00:14:10 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/critical.c
 * PURPOSE:         Critical sections
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
RtlDeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   lpCriticalSection->Reserved = -1;
}

VOID
STDCALL
RtlEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
}

VOID
STDCALL
RtlInitializeCriticalSection(LPCRITICAL_SECTION pcritical)
{
   pcritical->LockCount = -1;
   pcritical->RecursionCount = 0;
   pcritical->LockSemaphore = NULL;
   pcritical->OwningThread = (HANDLE)-1;
   pcritical->Reserved = 0;
}

VOID
STDCALL
RtlLeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
}

BOOLEAN
STDCALL
RtlTryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
   UNIMPLEMENTED;
   for(;;);
}

/* EOF */
