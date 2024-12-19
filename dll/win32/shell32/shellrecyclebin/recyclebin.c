/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/recyclebin.c
 * PURPOSE:     Public interface
 * PROGRAMMERS: Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "recyclebin_private.h"

BOOL WINAPI
CloseRecycleBinHandle(
    IN HDELFILE hDeletedFile)
{
    IRecycleBinFile *rbf = IRecycleBinFileFromHDELFILE(hDeletedFile);
    HRESULT hr;

    TRACE("(%p)\n", hDeletedFile);

    hr = IRecycleBinFile_Release(rbf);
    if (SUCCEEDED(hr))
        return TRUE;
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        SetLastError(HRESULT_CODE(hr));
    else
        SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
}

BOOL WINAPI
DeleteFileToRecycleBinA(
    IN LPCSTR FileName)
{
    int len;
    LPWSTR FileNameW = NULL;
    BOOL ret = FALSE;

    TRACE("(%s)\n", debugstr_a(FileName));

    /* Check parameters */
    if (FileName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    len = MultiByteToWideChar(CP_ACP, 0, FileName, -1, NULL, 0);
    if (len == 0)
        goto cleanup;
    FileNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!FileNameW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    if (MultiByteToWideChar(CP_ACP, 0, FileName, -1, FileNameW, len) == 0)
        goto cleanup;

    ret = DeleteFileToRecycleBinW(FileNameW);

cleanup:
    HeapFree(GetProcessHeap(), 0, FileNameW);
    return ret;
}

BOOL WINAPI
DeleteFileToRecycleBinW(
    IN LPCWSTR FileName)
{
    IRecycleBin *prb;
    HRESULT hr;

    TRACE("(%s)\n", debugstr_w(FileName));

    hr = GetDefaultRecycleBin(NULL, &prb);
    if (!SUCCEEDED(hr))
        goto cleanup;

    hr = IRecycleBin_DeleteFile(prb, FileName);
    IRecycleBin_Release(prb);

cleanup:
    if (SUCCEEDED(hr))
        return TRUE;
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        SetLastError(HRESULT_CODE(hr));
    else
        SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
}

BOOL WINAPI
DeleteFileInRecycleBin(
    IN HDELFILE hDeletedFile)
{
    IRecycleBinFile *rbf = (IRecycleBinFile *)hDeletedFile;
    HRESULT hr;

    TRACE("(%p)\n", hDeletedFile);

    hr = IRecycleBinFile_Delete(rbf);

    if (SUCCEEDED(hr))
        return TRUE;
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        SetLastError(HRESULT_CODE(hr));
    else
        SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
}

BOOL WINAPI
EmptyRecycleBinA(
    IN LPCSTR pszRoot OPTIONAL)
{
    int len;
    LPWSTR szRootW = NULL;
    BOOL ret = FALSE;

    TRACE("(%s)\n", debugstr_a(pszRoot));

    if (pszRoot)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pszRoot, -1, NULL, 0);
        if (len == 0)
            goto cleanup;
        szRootW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!szRootW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        if (MultiByteToWideChar(CP_ACP, 0, pszRoot, -1, szRootW, len) == 0)
            goto cleanup;
    }

    ret = EmptyRecycleBinW(szRootW);

cleanup:
    HeapFree(GetProcessHeap(), 0, szRootW);
    return ret;
}

BOOL WINAPI
EmptyRecycleBinW(
    IN LPCWSTR pszRoot OPTIONAL)
{
    IRecycleBin *prb;
    HRESULT hr;

    TRACE("(%s)\n", debugstr_w(pszRoot));

    hr = GetDefaultRecycleBin(pszRoot, &prb);
    if (!SUCCEEDED(hr))
        goto cleanup;

    hr = IRecycleBin_EmptyRecycleBin(prb);
    IRecycleBin_Release(prb);

cleanup:
    if (SUCCEEDED(hr))
        return TRUE;
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        SetLastError(HRESULT_CODE(hr));
    else
        SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
}

BOOL WINAPI
EnumerateRecycleBinA(
    IN LPCSTR pszRoot OPTIONAL,
    IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
    IN PVOID Context OPTIONAL)
{
    int len;
    LPWSTR szRootW = NULL;
    BOOL ret = FALSE;

    TRACE("(%s, %p, %p)\n", debugstr_a(pszRoot), pFnCallback, Context);

    if (pszRoot)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pszRoot, -1, NULL, 0);
        if (len == 0)
            goto cleanup;
        szRootW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!szRootW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        if (MultiByteToWideChar(CP_ACP, 0, pszRoot, -1, szRootW, len) == 0)
            goto cleanup;
    }

    ret = EnumerateRecycleBinW(szRootW, pFnCallback, Context);

cleanup:
    HeapFree(GetProcessHeap(), 0, szRootW);
    return ret;
}

BOOL WINAPI
EnumerateRecycleBinW(
    IN LPCWSTR pszRoot OPTIONAL,
    IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
    IN PVOID Context OPTIONAL)
{
    IRecycleBin *prb = NULL;
    IRecycleBinEnumList *prbel = NULL;
    IRecycleBinFile *prbf;
    HRESULT hr;

    TRACE("(%s, %p, %p)\n", debugstr_w(pszRoot), pFnCallback, Context);

    hr = GetDefaultRecycleBin(NULL, &prb);
    if (!SUCCEEDED(hr))
        goto cleanup;

    hr = IRecycleBin_EnumObjects(prb, &prbel);
    if (!SUCCEEDED(hr))
        goto cleanup;
    while (TRUE)
    {
        hr = IRecycleBinEnumList_Next(prbel, 1, &prbf, NULL);
        if (hr == S_FALSE)
        {
            hr = S_OK;
            goto cleanup;
        }
        else if (!SUCCEEDED(hr))
            goto cleanup;
        if (!pFnCallback(Context, (HDELFILE)prbf))
        {
            UINT error = GetLastError();
            hr = HRESULT_FROM_WIN32(error);
            goto cleanup;
        }
    }

cleanup:
    if (prb)
        IRecycleBin_Release(prb);
    if (prbel)
        IRecycleBinEnumList_Release(prbel);
    if (SUCCEEDED(hr))
        return TRUE;
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        SetLastError(HRESULT_CODE(hr));
    else
        SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
}

typedef struct _BBENUMFILECONTEXT
{
    const RECYCLEBINFILEIDENTITY *pFI;
    HDELFILE hDelFile;
} BBENUMFILECONTEXT;

static BOOL CALLBACK
GetRecycleBinFileHandleCallback(IN PVOID Context, IN HDELFILE hDeletedFile)
{
    BBENUMFILECONTEXT *pCtx = (BBENUMFILECONTEXT*)Context;
    IRecycleBinFile *pRBF = IRecycleBinFileFromHDELFILE(hDeletedFile);
    if (IRecycleBinFile_IsEqualIdentity(pRBF, pCtx->pFI) == S_OK)
    {
        pCtx->hDelFile = hDeletedFile;
        return FALSE;
    }
    CloseRecycleBinHandle(hDeletedFile);
    return TRUE;
}

EXTERN_C HDELFILE
GetRecycleBinFileHandle(
    IN LPCWSTR pszRoot OPTIONAL,
    IN const RECYCLEBINFILEIDENTITY *pFI)
{
    BBENUMFILECONTEXT context = { pFI, NULL };
    EnumerateRecycleBinW(pszRoot, GetRecycleBinFileHandleCallback, &context);
    return context.hDelFile;
}

BOOL WINAPI
RestoreFileFromRecycleBin(
    IN HDELFILE hDeletedFile)
{
    IRecycleBinFile *rbf = (IRecycleBinFile *)hDeletedFile;
    HRESULT hr;

    TRACE("(%p)\n", hDeletedFile);

    hr = IRecycleBinFile_Restore(rbf);
    if (SUCCEEDED(hr))
        return TRUE;
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        SetLastError(HRESULT_CODE(hr));
    else
        SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
}

EXTERN_C HRESULT
GetDefaultRecycleBin(
    IN LPCWSTR pszVolume OPTIONAL,
    OUT IRecycleBin **pprb)
{
    IUnknown *pUnk;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_w(pszVolume), pprb);

    if (!pprb)
        return E_POINTER;

    if (!pszVolume)
        hr = RecycleBinGeneric_Constructor(&pUnk);
    else
    {
        /* FIXME: do a better validation! */
        if (wcslen(pszVolume) != 3 || pszVolume[1] != ':' || pszVolume[2] != '\\')
            return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);

        /* For now, only support this type of recycle bins... */
        hr = RecycleBin5_Constructor(pszVolume, &pUnk);
    }
    if (!SUCCEEDED(hr))
        return hr;
    hr = IUnknown_QueryInterface(pUnk, &IID_IRecycleBin, (void **)pprb);
    IUnknown_Release(pUnk);
    return hr;
}

EXTERN_C HRESULT
GetRecycleBinPathFromDriveNumber(UINT Drive, LPWSTR Path)
{
    const WCHAR volume[] = { LOWORD('A' + Drive), ':', '\\', '\0' };
    IRecycleBin *pRB;
    HRESULT hr = GetDefaultRecycleBin(volume, &pRB);
    if (SUCCEEDED(hr))
    {
        hr = IRecycleBin_GetDirectory(pRB, Path);
        IRecycleBin_Release(pRB);
    }
    return hr;
}
