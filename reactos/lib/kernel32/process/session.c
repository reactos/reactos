/* $Id: session.c,v 1.2 2002/09/07 15:12:28 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/session.c
 * PURPOSE:         Win32 session (TS) functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *     2001-12-07 created
 */
#define NTOS_USER_MODE
#include <ntos.h>

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
