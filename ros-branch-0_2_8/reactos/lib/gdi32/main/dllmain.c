/*
 * dllmain.c
 *
 * $Revision: 1.11 $
 * $Author$
 * $Date$
 *
 */

#include "precomp.h"

/*
 * GDI32.DLL doesn't have an entry point. The initialization is done by a call
 * to GdiDllInitialize(). This call is done from the entry point of USER32.DLL.
 */
BOOL
WINAPI
DllMain (
	HANDLE	hDll,
	DWORD	dwReason,
	LPVOID	lpReserved
	)
{
	return TRUE;
}


VOID
WINAPI
GdiProcessSetup (VOID)
{
	hProcessHeap = GetProcessHeap();

        /* map the gdi handle table to user space */
	GdiHandleTable = NtCurrentTeb()->ProcessEnvironmentBlock->GdiSharedHandleTable;
	CurrentProcessId = NtCurrentTeb()->Cid.UniqueProcess;
}


/*
 * @implemented
 */
BOOL
WINAPI
GdiDllInitialize (
	HANDLE	hDll,
	DWORD	dwReason,
	LPVOID	lpReserved
	)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			GdiProcessSetup ();
			break;

		case DLL_THREAD_ATTACH:
			break;

		default:
			return FALSE;
	}

#if 0
	/* FIXME: working teb handling needed */
	NtCurrentTeb()->GdiTebBatch.Offset = 0;
	NtCurrentTeb()->GdiBatchCount = 0;
#endif

	return TRUE;
}

/* EOF */
