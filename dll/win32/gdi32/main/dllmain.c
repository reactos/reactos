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

PGDI_TABLE_ENTRY GdiHandleTable = NULL;
HANDLE CurrentProcessId = NULL;

/* Note match vista */
PDEVCAPS pGdiDevCaps = NULL;
PGDIHANDLECACHE GdiHandleCache = NULL;
PGDI_SHARED_HANDLE_TABLE pGdiSharedHandleTable = NULL;
PGDI_SHARED_HANDLE_TABLE pGdiSharedMemory = NULL;
DWORD GdiBatchLimit = 20;



/*
 * GDI32.DLL does have an entry point for disable threadlibrarycall,. The initialization is done by a call
 * to GdiDllInitialize(). This call is done from the entry point of USER32.DLL.
 */
BOOL
WINAPI
DllMain (
    HANDLE  hDll,
    DWORD   dwReason,
    LPVOID  lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH :
                DisableThreadLibraryCalls(hDll);
                break;

        default:
                break;
    }
    return TRUE;
}

VOID
WINAPI
GdiProcessSetup (VOID)
{

    hProcessHeap = GetProcessHeap();
    CurrentProcessId = NtCurrentTeb()->Cid.UniqueProcess;

    /* map the gdi handle table to user space */
    pGdiSharedMemory = NtCurrentTeb()->ProcessEnvironmentBlock->GdiSharedHandleTable;
    pGdiSharedHandleTable = NtCurrentTeb()->ProcessEnvironmentBlock->GdiSharedHandleTable;
    GdiHandleTable = NtCurrentTeb()->ProcessEnvironmentBlock->GdiSharedHandleTable;
    pGdiDevCaps = &pGdiSharedMemory->DevCaps;
    GdiHandleCache = (PGDIHANDLECACHE)NtCurrentTeb()->ProcessEnvironmentBlock->GdiHandleBuffer;

    /* why does vista send down 0x400 to GetAppCompatFlags2 no doc in msdn found 
     * 0x200 GdiBatchLimit is ON if this is not set in AppCompatFlags2 no GdiBatchLimit is active */

    if ((GetAppCompatFlags2( (HTASK) 0x400) & 0x200) == 0x200)
    {
        GdiBatchLimit = (DWORD) NtCurrentTeb()->ProcessEnvironmentBlock->GdiDCAttributeList;
    }
    else
    {
        /* GdiBatchLimit set to 0 */
        GdiBatchLimit = 0;
    }
}


/*
 * @implemented
 */
BOOL
WINAPI
GdiDllInitialize (
    HANDLE  hDll,
    DWORD   dwReason,
    LPVOID  lpReserved)
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
