/*
 * dllmain.c
 *
 * $Revision: 1.9 $
 * $Author: navaraf $
 * $Date: 2004/04/09 20:03:11 $
 *
 */

#include <windows.h>
#include <win32k/win32k.h>
#include <internal/heap.h>

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
