/* $Id: session.c,v 1.1 2001/12/07 11:19:49 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/session.c
 * PURPOSE:         Win32 session (TS) functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *     2001-12-07 created
 */
#include <ddk/ntddk.h>

BOOL STDCALL ProcessIdToSessionId (
  DWORD dwProcessId,
  DWORD* pSessionId
  )
{
	if (NULL != pSessionId)
	{
		/* TODO: implement TS */
		*pSessionId = 0; /* no TS */
		return TRUE;
	}
	return FALSE;
}


/* EOF */
