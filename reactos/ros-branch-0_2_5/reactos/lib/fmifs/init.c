/* $Id: init.c,v 1.3 2004/02/23 11:55:12 ekohl Exp $
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

static BOOL FmIfsInitialized = FALSE;

static BOOL STDCALL
InitializeFmIfsOnce (VOID)
{
	/* TODO: Check how many IFS are installed in the system */
	/* TOSO: and register a descriptor for each one */
	return TRUE;
}


/* FMIFS.8 */
BOOL STDCALL
InitializeFmIfs (PVOID hinstDll,
		 DWORD dwReason,
		 PVOID reserved)
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
