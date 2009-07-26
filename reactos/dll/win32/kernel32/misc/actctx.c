/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/misc/actctx.c
 * PURPOSE:         Comm functions
 * PROGRAMMERS:     Jacek Caban for CodeWeavers
 *                  Eric Pouech
 *                  Jon Griffiths
 *                  Dmitry Chapyshev (dmitry@reactos.org)
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

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
WINAPI
FindActCtxSectionStringA(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    BOOL bRetVal;
    LPWSTR lpStringToFindW = NULL;

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
    if (lpStringToFindW)
        RtlFreeHeap(GetProcessHeap(), 0, (LPWSTR*) lpStringToFindW);

    return bRetVal;
}


/*
 * @implemented
 */
HANDLE
WINAPI
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
WINAPI
ActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *ulCookie
    )
{
    NTSTATUS Status;

    DPRINT("ActivateActCtx(%p %p)\n", hActCtx, ulCookie );

    Status = RtlActivateActivationContext(0, hActCtx, ulCookie);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

/*
 * @unimplemented
 */
VOID
WINAPI
AddRefActCtx(
    HANDLE hActCtx
    )
{
    DPRINT("AddRefActCtx(%p)\n", hActCtx);
    RtlAddRefActivationContext(hActCtx);
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
CreateActCtxW(
    PCACTCTXW pActCtx
    )
{
    NTSTATUS    Status;
    HANDLE      hActCtx;

    DPRINT("CreateActCtxW(%p %08lx)\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

    Status = RtlCreateActivationContext(&hActCtx, (PVOID*)&pActCtx);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return INVALID_HANDLE_VALUE;
    }
    return hActCtx;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    )
{
    NTSTATUS Status;

    DPRINT("DeactivateActCtx(%08lx %08lx)\n", dwFlags, ulCookie);
    Status = RtlDeactivateActivationContext(dwFlags, ulCookie);

    if (!NT_SUCCESS(Status)) return FALSE;

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
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
WINAPI
FindActCtxSectionStringW(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCWSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    UNICODE_STRING us;
    NTSTATUS Status;

    RtlInitUnicodeString(&us, lpStringToFind);
    Status = RtlFindActivationContextSectionString(dwFlags, lpExtensionGuid, ulSectionId, &us, ReturnedData);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetCurrentActCtx(
    HANDLE *phActCtx)
{
    NTSTATUS Status;

    DPRINT("GetCurrentActCtx(%p)\n", phActCtx);
    Status = RtlGetActiveActivationContext(phActCtx);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
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
WINAPI
ReleaseActCtx(
    HANDLE hActCtx
    )
{
    DPRINT("ReleaseActCtx(%p)\n", hActCtx);
    RtlReleaseActivationContext(hActCtx);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
ZombifyActCtx(
    HANDLE hActCtx
    )
{
    NTSTATUS Status;
    DPRINT("ZombifyActCtx(%p)\n", hActCtx);

    Status = RtlZombifyActivationContext(hActCtx);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}
