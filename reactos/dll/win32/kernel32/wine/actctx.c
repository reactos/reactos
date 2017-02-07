/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/misc/actctx.c
 * PURPOSE:         Activation contexts
 * PROGRAMMERS:     Jacek Caban for CodeWeavers
 *                  Eric Pouech
 *                  Jon Griffiths
 *                  Dmitry Chapyshev (dmitry@reactos.org)
 *                  Samuel Serapión 
 */

/* synched with wine 1.1.26 */

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

    if ((status = RtlCreateActivationContext(&hActCtx, (PVOID*)pActCtx)))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return INVALID_HANDLE_VALUE;
    }
    return hActCtx;
}

/***********************************************************************
 * ActivateActCtx (KERNEL32.@)
 *
 * Activate an activation context.
 */
BOOL WINAPI ActivateActCtx(HANDLE hActCtx, ULONG_PTR *ulCookie)
{
    NTSTATUS status;

    if ((status = RtlActivateActivationContext( 0, hActCtx, ulCookie )))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 * DeactivateActCtx (KERNEL32.@)
 *
 * Deactivate an activation context.
 */
BOOL WINAPI DeactivateActCtx(DWORD dwFlags, ULONG_PTR ulCookie)
{
    RtlDeactivateActivationContext( dwFlags, ulCookie );
    return TRUE;
}

/***********************************************************************
 * GetCurrentActCtx (KERNEL32.@)
 *
 * Get the current activation context.
 */
BOOL WINAPI GetCurrentActCtx(HANDLE* phActCtx)
{
    NTSTATUS status;

    if ((status = RtlGetActiveActivationContext(phActCtx)))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 * AddRefActCtx (KERNEL32.@)
 *
 * Add a reference to an activation context.
 */
void WINAPI AddRefActCtx(HANDLE hActCtx)
{
    RtlAddRefActivationContext(hActCtx);
}

/***********************************************************************
 * ReleaseActCtx (KERNEL32.@)
 *
 * Release a reference to an activation context.
 */
void WINAPI ReleaseActCtx(HANDLE hActCtx)
{
    RtlReleaseActivationContext(hActCtx);
}

/***********************************************************************
 * ZombifyActCtx (KERNEL32.@)
 *
 * Release a reference to an activation context.
 */
BOOL WINAPI ZombifyActCtx(HANDLE hActCtx)
{
  FIXME("%p\n", hActCtx);
  if (hActCtx != ACTCTX_FAKE_HANDLE)
    return FALSE;
  return TRUE;
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

    if (!lpSearchStr)
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
  FIXME("%08x %s %u %s %p\n", dwFlags, debugstr_guid(lpExtGuid),
       ulId, debugstr_guid(lpSearchGuid), pInfo);
  SetLastError( ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 * QueryActCtxW (KERNEL32.@)
 *
 * Get information about an activation context.
 */
BOOL WINAPI QueryActCtxW(DWORD dwFlags, HANDLE hActCtx, PVOID pvSubInst,
                         ULONG ulClass, PVOID pvBuff, SIZE_T cbBuff,
                         SIZE_T *pcbLen)
{
    NTSTATUS status;

    if ((status = RtlQueryInformationActivationContext( dwFlags, hActCtx, pvSubInst, ulClass,
                                                        pvBuff, cbBuff, pcbLen )))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }
    return TRUE;
}

/* REACTOS PRIVATE ************************************************************/

VOID
NTAPI
BasepFreeActivationContextActivationBlock(IN PBASEP_ACTCTX_BLOCK ActivationBlock)
{
    /* Exit if there was nothing passed in */
    if (!ActivationBlock) return;
    
    /* Do we have a context? */
    if (ActivationBlock->ActivationContext)
    {
        /* Release and clear it */
        RtlReleaseActivationContext(ActivationBlock->ActivationContext);
        ActivationBlock->ActivationContext = NULL;
    }
    
    /* Free the block */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ActivationBlock);
}

NTSTATUS
NTAPI
BasepAllocateActivationContextActivationBlock(IN DWORD Flags,
                                              IN PVOID CompletionRoutine,
                                              IN PVOID CompletionContext,
                                              OUT PBASEP_ACTCTX_BLOCK *ActivationBlock)
{
    NTSTATUS Status;
    ACTIVATION_CONTEXT_BASIC_INFORMATION ContextInfo;

    /* Clear the info structure */
    ContextInfo.dwFlags = 0;
    ContextInfo.hActCtx = NULL;

    /* Assume failure */
    if (ActivationBlock) *ActivationBlock = NULL;

    /* Only support valid flags */
    if (Flags & ~(1 | 2)) // FIXME: What are they? 2 looks like BASEP_ACTCTX_FORCE_BLOCK
    {
        /* Fail if unknown flags are passed in */
        Status = STATUS_INVALID_PARAMETER_1;
        goto Quickie;
    }

    /* Caller should have passed in an activation block */
    if (!ActivationBlock)
    {
        /* Fail otherwise */
        Status = STATUS_INVALID_PARAMETER_4;
        goto Quickie;
    }

    /* Query RTL for information on the current activation context */
    Status = RtlQueryInformationActivationContext(1,
                                                  NULL,
                                                  0,
                                                  ActivationContextBasicInformation,
                                                  &ContextInfo,
                                                  sizeof(ContextInfo),
                                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Failed -- bail out */
        DPRINT1("SXS: %s - Failure getting active activation context; ntstatus %08lx\n",
                __FUNCTION__, Status);
        goto Quickie;
    }

    /* Check if the current one should be freed */
    if (ContextInfo.dwFlags & 1)
    {
        /* Release and clear it */
        RtlReleaseActivationContext(ContextInfo.hActCtx);
        ContextInfo.hActCtx = NULL;
    }

    /* Check if there's an active context, or if the caller is forcing one */
    if (!(Flags & 2) || (ContextInfo.hActCtx))
    {
        /* Allocate the block */
        *ActivationBlock = RtlAllocateHeap(RtlGetProcessHeap(),
                                           0,
                                           sizeof(BASEP_ACTCTX_BLOCK));
        if (!(*ActivationBlock))
        {
            /* Ran out of memory, fail */
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Fill it out */
        (*ActivationBlock)->ActivationContext = ContextInfo.hActCtx;
        (*ActivationBlock)->Flags = 0;
        if (Flags & 1) (*ActivationBlock)->Flags |= 1; // Not sure about this flag
        (*ActivationBlock)->CompletionRoutine = CompletionRoutine;
        (*ActivationBlock)->CompletionContext = CompletionContext;

        /* Tell Quickie below not to free anything, since this is success */
        ContextInfo.hActCtx = NULL;
    }

    /* Set success status */
    Status = STATUS_SUCCESS;

Quickie:
    /* Failure or success path, return to caller and free on failure */
    if (ContextInfo.hActCtx) RtlReleaseActivationContext(ContextInfo.hActCtx);
    return Status;
}

/* EOF */
