#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

#define ACTCTX_FLAGS_ALL (\
 ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID |\
 ACTCTX_FLAG_LANGID_VALID |\
 ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID |\
 ACTCTX_FLAG_RESOURCE_NAME_VALID |\
 ACTCTX_FLAG_SET_PROCESS_DEFAULT |\
 ACTCTX_FLAG_APPLICATION_NAME_VALID |\
 ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF |\
 ACTCTX_FLAG_HMODULE_VALID )

#define ACTCTX_FAKE_HANDLE ((HANDLE) 0xf00baa)
#define ACTCTX_FAKE_COOKIE ((ULONG_PTR) 0xf00bad)

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
    bRetVal = FindActCtxSectionStringW(dwFlags, 
                                        lpExtensionGuid,
                                        ulSectionId, 
                                        lpStringToFindW,
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

    ZeroMemory(&pActCtxW, sizeof(ACTCTXW));
    pActCtxW.cbSize = sizeof(ACTCTXW);
    pActCtxW.dwFlags = pActCtx->dwFlags;
    pActCtxW.wLangId = pActCtx->wLangId;
    pActCtxW.hModule = pActCtx->hModule;
    pActCtxW.wProcessorArchitecture = pActCtx->wProcessorArchitecture;

    pActCtxW.hModule = pActCtx->hModule;

    /* Convert ActCtx Strings */
    if (pActCtx->lpSource)
    {
        BasepAnsiStringToHeapUnicodeString(pActCtx->lpSource,
                                          (LPWSTR*) &pActCtxW.lpSource);
    } 
    if (pActCtx->lpAssemblyDirectory)
    {
        BasepAnsiStringToHeapUnicodeString(pActCtx->lpAssemblyDirectory,
                                          (LPWSTR*) &pActCtxW.lpAssemblyDirectory);
    }
    if (HIWORD(pActCtx->lpResourceName))
    {
        BasepAnsiStringToHeapUnicodeString(pActCtx->lpResourceName,
                                          (LPWSTR*) &pActCtxW.lpResourceName);
    }
    else
    {
        pActCtxW.lpResourceName = (LPWSTR) pActCtx->lpResourceName;
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
    if (HIWORD(pActCtx->lpResourceName))
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) pActCtxW.lpResourceName);
    RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) pActCtxW.lpApplicationName);

    return hRetVal;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *ulCookie
    )
{
    DPRINT("ActivateActCtx(%p %p)\n", hActCtx, ulCookie );
    if (ulCookie)
        *ulCookie = ACTCTX_FAKE_COOKIE;
    return TRUE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
AddRefActCtx(
    HANDLE hActCtx
    )
{
    DPRINT("AddRefActCtx(%p)\n", hActCtx);
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateActCtxW(
    PCACTCTXW pActCtx
    )
{
    DPRINT("CreateActCtxW(%p %08lx)\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

    if (!pActCtx)
        return INVALID_HANDLE_VALUE;
    if (pActCtx->cbSize != sizeof *pActCtx)
        return INVALID_HANDLE_VALUE;
    if (pActCtx->dwFlags & ~ACTCTX_FLAGS_ALL)
        return INVALID_HANDLE_VALUE;
    return ACTCTX_FAKE_HANDLE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    )
{
    DPRINT("DeactivateActCtx(%08lx %08lx)\n", dwFlags, ulCookie);
    if (ulCookie != ACTCTX_FAKE_COOKIE)
        return FALSE;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindActCtxSectionGuid(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    const GUID *lpGuidToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    DPRINT("%s() is UNIMPLEMENTED!\n", __FUNCTION__);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindActCtxSectionStringW(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCWSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    DPRINT("%s() is UNIMPLEMENTED!\n", __FUNCTION__);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetCurrentActCtx(
    HANDLE *phActCtx)
{
    DPRINT("GetCurrentActCtx(%p)\n", phActCtx);
    *phActCtx = ACTCTX_FAKE_HANDLE;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueryActCtxW(
    DWORD dwFlags,
    HANDLE hActCtx,
    PVOID pvSubInstance,
    ULONG ulInfoClass,
    PVOID pvBuffer,
    SIZE_T cbBuffer OPTIONAL,
    SIZE_T *pcbWrittenOrRequired OPTIONAL
    )
{
    DPRINT("%s() is UNIMPLEMENTED!\n", __FUNCTION__);
    /* this makes Adobe Photoshop 7.0 happy */
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
ReleaseActCtx(
    HANDLE hActCtx
    )
{
    DPRINT("ReleaseActCtx(%p)\n", hActCtx);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ZombifyActCtx(
    HANDLE hActCtx
    )
{
    DPRINT("ZombifyActCtx(%p)\n", hActCtx);
    if (hActCtx != ACTCTX_FAKE_HANDLE)
        return FALSE;
    return TRUE;
}
