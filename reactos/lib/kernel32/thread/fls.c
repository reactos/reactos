/* $Id: fls.c,v 1.1 2003/05/29 00:36:41 hyperion Exp $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS system libraries
 * FILE:       lib/kernel32/thread/fls.c
 * PURPOSE:    Fiber local storage functions
 * PROGRAMMER: KJK::Hyperion <noog@libero.it>
 *
 * UPDATE HISTORY:
 *             28/05/2003 - created. Stubs only
 *
 */

#include <k32.h>

#include <kernel32/kernel32.h>

DWORD WINAPI FlsAlloc(PFLS_CALLBACK_FUNCTION lpCallback)
{
 (void)lpCallback;

 UNIMPLEMENTED;
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FLS_OUT_OF_INDEXES;
}

BOOL WINAPI FlsFree(DWORD dwFlsIndex)
{
 (void)dwFlsIndex;

 UNIMPLEMENTED;
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}

PVOID WINAPI FlsGetValue(DWORD dwFlsIndex)
{
 (void)dwFlsIndex;

 UNIMPLEMENTED;
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return NULL;
}

BOOL WINAPI FlsSetValue(DWORD dwFlsIndex, PVOID lpFlsData)
{
 (void)dwFlsIndex;
 (void)lpFlsData;

 UNIMPLEMENTED;
 SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
 return FALSE;
}

/* EOF */
