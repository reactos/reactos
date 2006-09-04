/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/init.c
 * PURPOSE:         Initialisation
 *
 * PROGRAMMERS:     Emanuele Aliberti
 */

#include "precomp.h"

static BOOLEAN FmIfsInitialized = FALSE;

static BOOLEAN NTAPI
InitializeFmIfsOnce(void)
{
	/* TODO: Check how many IFS are installed in the system */
	/* TOSO: and register a descriptor for each one */
	return TRUE;
}

/* FMIFS.8 */
BOOLEAN NTAPI
InitializeFmIfs(
	IN PVOID hinstDll,
	IN DWORD dwReason,
	IN PVOID reserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			if (FALSE == FmIfsInitialized)
			{
				if (FALSE == InitializeFmIfsOnce())
				{
						return FALSE;
				}

				FmIfsInitialized = TRUE;
			}
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

/* EOF */
