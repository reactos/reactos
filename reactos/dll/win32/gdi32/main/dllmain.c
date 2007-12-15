/*
 * dllmain.c
 *
 * $Revision: 1.11 $
 * $Author$
 * $Date$
 *
 */

#include "precomp.h"

extern HGDIOBJ stock_objects[];
BOOL SetStockObjects = FALSE;
PDEVCAPS GdiDevCaps = NULL;

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
	GdiSharedHandleTable = NtCurrentTeb()->ProcessEnvironmentBlock->GdiSharedHandleTable;
	GdiDevCaps = &GdiSharedHandleTable->DevCaps;
	CurrentProcessId = NtCurrentTeb()->Cid.UniqueProcess;
	GDI_BatchLimit = (DWORD) NtCurrentTeb()->ProcessEnvironmentBlock->GdiDCAttributeList;
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
                        NtCurrentTeb()->GdiTebBatch.Offset = 0;
                        NtCurrentTeb()->GdiBatchCount = 0;
			break;

		default:
			return FALSE;
	}

  // Very simple, the list will fill itself as it is needed.
        if(!SetStockObjects)
        {
          RtlZeroMemory( &stock_objects, NB_STOCK_OBJECTS); //Assume Ros is dirty.
          SetStockObjects = TRUE;
        }

	return TRUE;
}

/* EOF */
