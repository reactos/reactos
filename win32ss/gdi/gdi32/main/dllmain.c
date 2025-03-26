/*
 * dllmain.c
 */

#include <precomp.h>

static BOOL gbInitialized = FALSE;
extern HGDIOBJ stock_objects[];
BOOL SetStockObjects = FALSE;
PDEVCAPS GdiDevCaps = NULL;
PGDIHANDLECACHE GdiHandleCache = NULL;
BOOL gbLpk = FALSE;
RTL_CRITICAL_SECTION semLocal;
extern CRITICAL_SECTION gcsClientObjLinks;

/*
 * GDI32.DLL does have an entry point for DisableThreadLibraryCalls().
 * The initialization is done by a call to GdiDllInitialize(). This
 * call is done from the entry point of USER32.DLL.
 */
BOOL
WINAPI
DllMain(
    _In_ HANDLE hDll,
    _In_ ULONG dwReason,
    _In_opt_ PVOID pReserved)
{
    UNREFERENCED_PARAMETER(pReserved);

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
GdiProcessSetup(VOID)
{
    if (!gbInitialized)
    {
        gbInitialized = TRUE;
        hProcessHeap = GetProcessHeap();

        /* map the gdi handle table to user space */
        GdiHandleTable = NtCurrentTeb()->ProcessEnvironmentBlock->GdiSharedHandleTable;
        GdiSharedHandleTable = NtCurrentTeb()->ProcessEnvironmentBlock->GdiSharedHandleTable;
        GdiDevCaps = &GdiSharedHandleTable->DevCaps;
        CurrentProcessId = NtCurrentTeb()->ClientId.UniqueProcess;
        GDI_BatchLimit = (DWORD) NtCurrentTeb()->ProcessEnvironmentBlock->GdiDCAttributeList;
        GdiHandleCache = (PGDIHANDLECACHE)NtCurrentTeb()->ProcessEnvironmentBlock->GdiHandleBuffer;
        RtlInitializeCriticalSection(&semLocal);
        InitializeCriticalSection(&gcsClientObjLinks);
        GdiInitializeLanguagePack(0);
    }
}

VOID
WINAPI
GdiProcessShutdown(VOID)
{
    DeleteCriticalSection(&gcsClientObjLinks);
    RtlDeleteCriticalSection(&semLocal);
}


/*
 * @implemented
 */
BOOL
WINAPI
GdiDllInitialize(
    _In_ HANDLE hDll,
    _In_ ULONG dwReason,
    _In_opt_ PVOID pReserved)
{
    UNREFERENCED_PARAMETER(pReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            /* Don't bother us for each thread */
            // DisableThreadLibraryCalls(hDll);

            /* Initialize the kernel part of GDI first */
            if (!NtGdiInit()) return FALSE;

            /* Now initialize ourselves */
            GdiProcessSetup();
            break;
        }

        case DLL_THREAD_ATTACH:
        {
            NtCurrentTeb()->GdiTebBatch.Offset = 0;
            NtCurrentTeb()->GdiBatchCount = 0;
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            /* Cleanup */
            GdiProcessShutdown();
            return TRUE;
        }

        default:
            return FALSE;
    }

    /* Very simple, the list will fill itself as it is needed */
    if (!SetStockObjects)
    {
        RtlZeroMemory(&stock_objects, NB_STOCK_OBJECTS); // Assume ROS is dirty
        SetStockObjects = TRUE;
    }

    return TRUE;
}

/* EOF */
