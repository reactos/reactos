/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/wine/actctx.c
 * PURPOSE:         Activation contexts
 * PROGRAMMERS:     Jacek Caban for CodeWeavers
 *                  Eric Pouech
 *                  Jon Griffiths
 *                  Dmitry Chapyshev (dmitry@reactos.org)
 *                  Samuel Serapión
 */

/* Partly synched with Wine 1.7.17 */

#include <k32.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(actctx);

#define ACTCTX_FAKE_HANDLE ((HANDLE) 0xf00baa)

/***********************************************************************
 * CreateActCtxA (KERNEL32.@)
 *
 * Create an activation context.
 */
HANDLE WINAPI CreateActCtxA(PCACTCTXA pActCtx)
{
    ACTCTXW     actw;
    SIZE_T      len;
    HANDLE      ret = INVALID_HANDLE_VALUE;
    LPWSTR      src = NULL, assdir = NULL, resname = NULL, appname = NULL;

    TRACE("%p %08x\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

    if (!pActCtx || pActCtx->cbSize != sizeof(*pActCtx))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    actw.cbSize = sizeof(actw);
    actw.dwFlags = pActCtx->dwFlags;
    if (pActCtx->lpSource)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pActCtx->lpSource, -1, NULL, 0);
        src = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!src) return INVALID_HANDLE_VALUE;
        MultiByteToWideChar(CP_ACP, 0, pActCtx->lpSource, -1, src, len);
    }
    actw.lpSource = src;

    if (actw.dwFlags & ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID)
        actw.wProcessorArchitecture = pActCtx->wProcessorArchitecture;
    if (actw.dwFlags & ACTCTX_FLAG_LANGID_VALID)
        actw.wLangId = pActCtx->wLangId;
    if (actw.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pActCtx->lpAssemblyDirectory, -1, NULL, 0);
        assdir = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!assdir) goto done;
        MultiByteToWideChar(CP_ACP, 0, pActCtx->lpAssemblyDirectory, -1, assdir, len);
        actw.lpAssemblyDirectory = assdir;
    }
    if (actw.dwFlags & ACTCTX_FLAG_RESOURCE_NAME_VALID)
    {
        if ((ULONG_PTR)pActCtx->lpResourceName >> 16)
        {
            len = MultiByteToWideChar(CP_ACP, 0, pActCtx->lpResourceName, -1, NULL, 0);
            resname = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            if (!resname) goto done;
            MultiByteToWideChar(CP_ACP, 0, pActCtx->lpResourceName, -1, resname, len);
            actw.lpResourceName = resname;
        }
        else actw.lpResourceName = (LPCWSTR)pActCtx->lpResourceName;
    }
    if (actw.dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pActCtx->lpApplicationName, -1, NULL, 0);
        appname = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!appname) goto done;
        MultiByteToWideChar(CP_ACP, 0, pActCtx->lpApplicationName, -1, appname, len);
        actw.lpApplicationName = appname;
    }
    if (actw.dwFlags & ACTCTX_FLAG_HMODULE_VALID)
        actw.hModule = pActCtx->hModule;

    ret = CreateActCtxW(&actw);

done:
    HeapFree(GetProcessHeap(), 0, src);
    HeapFree(GetProcessHeap(), 0, assdir);
    HeapFree(GetProcessHeap(), 0, resname);
    HeapFree(GetProcessHeap(), 0, appname);
    return ret;
}

/***********************************************************************
 * CreateActCtxW (KERNEL32.@)
 *
 * Create an activation context.
 */
HANDLE WINAPI CreateActCtxW(PCACTCTXW pActCtx)
{
    NTSTATUS    status;
    HANDLE      hActCtx;

    TRACE("%p %08x\n", pActCtx, pActCtx ? pActCtx->dwFlags : 0);

    if ((status = RtlCreateActivationContext(0, (PVOID)pActCtx, 0, NULL, NULL, &hActCtx)))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return INVALID_HANDLE_VALUE;
    }
    return hActCtx;
}

/***********************************************************************
 * FindActCtxSectionStringA (KERNEL32.@)
 *
 * Find information about a string in an activation context.
 */
BOOL WINAPI FindActCtxSectionStringA(DWORD dwFlags, const GUID* lpExtGuid,
                                    ULONG ulId, LPCSTR lpSearchStr,
                                    PACTCTX_SECTION_KEYED_DATA pInfo)
{
    LPWSTR  search_str;
    DWORD   len;
    BOOL    ret;

    TRACE("%08x %s %u %s %p\n", dwFlags, debugstr_guid(lpExtGuid),
          ulId, debugstr_a(lpSearchStr), pInfo);

    if (!lpSearchStr || !pInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, lpSearchStr, -1, NULL, 0);
    search_str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, lpSearchStr, -1, search_str, len);

    ret = FindActCtxSectionStringW(dwFlags, lpExtGuid, ulId, search_str, pInfo);

    HeapFree(GetProcessHeap(), 0, search_str);
    return ret;
}

/***********************************************************************
 * FindActCtxSectionStringW (KERNEL32.@)
 *
 * Find information about a string in an activation context.
 */
BOOL WINAPI FindActCtxSectionStringW(DWORD dwFlags, const GUID* lpExtGuid,
                                    ULONG ulId, LPCWSTR lpSearchStr,
                                    PACTCTX_SECTION_KEYED_DATA pInfo)
{
    UNICODE_STRING us;
    NTSTATUS status;

    if (!pInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlInitUnicodeString(&us, lpSearchStr);
    if ((status = RtlFindActivationContextSectionString(dwFlags, lpExtGuid, ulId, &us, pInfo)))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 * FindActCtxSectionGuid (KERNEL32.@)
 *
 * Find information about a GUID in an activation context.
 */
BOOL WINAPI FindActCtxSectionGuid(DWORD dwFlags, const GUID* lpExtGuid,
                                  ULONG ulId, const GUID* lpSearchGuid,
                                  PACTCTX_SECTION_KEYED_DATA pInfo)
{
    NTSTATUS status;

    if ((status = RtlFindActivationContextSectionGuid(dwFlags, lpExtGuid, ulId, lpSearchGuid, pInfo)))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    return TRUE;
}

/* EOF */
