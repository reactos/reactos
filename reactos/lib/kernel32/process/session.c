/* $Id: session.c,v 1.4 2003/01/15 21:24:35 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/session.c
 * PURPOSE:         Win32 session (TS) functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *     2001-12-07 created
 */
#include <k32.h>

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
