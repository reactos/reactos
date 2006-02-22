#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/*
 * @implemented
 */
BOOL
STDCALL
FindActCtxSectionStringA(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    BOOL bRetVal;
    LPWSTR lpStringToFindW;
    
    /* Convert lpStringToFind */
    if (lpStringToFind)
    {
        BasepAnsiStringToHeapUnicodeString(lpStringToFind,
                                            (LPWSTR*) &lpStringToFindW);
    }

    /* Call the Unicode function */
    bRetVal = FindActCtxSectionStringA(dwFlags, 
                                        lpExtensionGuid,
                                        ulSectionId, 
                                        lpStringToFind,
                                        ReturnedData);

    /* Clean up */
    RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) lpStringToFindW);

    return bRetVal;
}


/*
 * @implemented
 */
HANDLE
STDCALL
CreateActCtxA(
    PCACTCTXA pActCtx
    )
{
    ACTCTXW pActCtxW;
    HANDLE hRetVal;

    ZeroMemory(&pActCtxW, sizeof(pActCtxW));
    pActCtxW.cbSize = sizeof(pActCtxW);
    pActCtxW.dwFlags = pActCtx->dwFlags;
    pActCtxW.wProcessorArchitecture = pActCtx->wProcessorArchitecture;
    pActCtxW.dwFlags = pActCtx->wProcessorArchitecture;

    pActCtxW.hModule = pActCtx->hModule;

    /* Convert ActCtx Strings */
    if (pActCtx->lpAssemblyDirectory)
    {
        BasepAnsiStringToHeapUnicodeString(pActCtx->lpSource,
                                            (LPWSTR*) &pActCtxW.lpSource);
    }

    if (pActCtx->lpAssemblyDirectory)
    {
        BasepAnsiStringToHeapUnicodeString(pActCtx->lpAssemblyDirectory,
                                            (LPWSTR*) &pActCtxW.lpAssemblyDirectory);
    }
    if (pActCtx->lpResourceName)
    {
        BasepAnsiStringToHeapUnicodeString(pActCtx->lpResourceName,
                                            (LPWSTR*) &pActCtxW.lpResourceName);
    }
    if (pActCtx->lpApplicationName)
    {
        BasepAnsiStringToHeapUnicodeString(pActCtx->lpApplicationName,
                                            (LPWSTR*) &pActCtxW.lpApplicationName);
    }

    /* Call the Unicode function */
    hRetVal = CreateActCtxW(&pActCtxW);

    /* Clean up */
    RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) pActCtxW.lpSource);
    RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) pActCtxW.lpAssemblyDirectory);
    RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) pActCtxW.lpResourceName);
    RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) pActCtxW.lpApplicationName);

    return hRetVal;
}
