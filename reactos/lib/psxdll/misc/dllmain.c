/* $Id: dllmain.c,v 1.2 1999/10/12 21:17:05 ea Exp $
 * 
 * reactos/subsys/psxdll/dllmain.c
 * 
 * ReactOS Operating System
 * - POSIX+ client side DLL (ReactOS POSIX+ Subsystem)
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <reactos/config.h>


/* --- Data --- */

PVOID
__PdxHeap = NULL;

HANDLE
__PdxApiPort = INVALID_HANDLE_VALUE;

HINSTANCE
__PdxModuleHandle;

/* --- Internal functions --- */


BOOLEAN
__PdxConnectToPsxSs (VOID)
{
	/* FIXME: call NtConnectPort(); */
	return TRUE;
}


/* --- Library entry point --- */


BOOL
WINAPI
DllMain (
	HINSTANCE	hInstDll, 
	DWORD		fdwReason, 
	LPVOID		fImpLoad
	)
{
	if (DLL_PROCESS_ATTACH == fdwReason)
	{
		/*
		 * Save the module handle.
		 */
		__PdxModuleHandle = hInstDll;
		/*
		 * Create a heap for the client process.
		 * Initially its size is the same as the 
		 * memory page size; it is growable and
		 * grows 512 bytes a time.
		 */
		__PdxHeap = RtlCreateHeap (
				3,	/* Flags */
				NULL,	/* Base address */
				CONFIG_MEMORY_PAGE_SIZE, /* Initial size */
				512,	/* Maximum size/grow by */
				0,	/* unknown */
				NULL	/* PHEAP_DEFINITION */
				);
		/* 
		 * Connect to the POSIX+ subsystem
		 * process (psxss.exe).
		 */
		if (FALSE == __PdxConnectToPsxSs())
		{
			return FALSE;
		}
	}
	return TRUE;
}


/* EOF */
