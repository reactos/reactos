/* $Id: init.c,v 1.2 2002/03/07 00:24:24 ea Exp $
 *
 * COPYING:	See the top level directory
 * PROJECT:	ReactOS 
 * FILE:	reactos/lib/fmifs/init.c
 * DESCRIPTION:	File management IFS utility functions
 * PROGRAMMER:	Emanuele Aliberti
 * UPDATED
 * 	1999-02-16 (Emanuele Aliberti)
 * 		Entry points added.
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <fmifs.h>

/* FMIFS.8 */
static BOOL FmIfsInitialized = FALSE;

static BOOL STDCALL
InitializeFmIfsOnce (VOID)
{
	/* TODO: Check how many IFS are installed in the system */
	/* TOSO: and register a descriptor for each one */
	return TRUE;
}

BOOL STDCALL
InitializeFmIfs(VOID)
{
	if (FALSE == FmIfsInitialized)
	{
		if (FALSE == InitializeFmIfsOnce())
		{
			return FALSE;
		}
		FmIfsInitialized = TRUE;
	}
	return TRUE;
}

/* EOF */
