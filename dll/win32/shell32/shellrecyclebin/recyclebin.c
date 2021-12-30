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
    IN HANDLE hDeletedFile)
{
    IRecycleBinFile *rbf = (IRecycleBinFile *)hDeletedFile;
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
DeleteFileHandleToRecycleBin(
    IN HANDLE hDeletedFile)
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
        if (!pFnCallback(Context, (HANDLE)prbf))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
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

BOOL WINAPI
GetDeletedFileTypeNameW(
    IN HANDLE hDeletedFile,
    OUT LPWSTR pTypeName,
    IN DWORD BufferSize,
    OUT LPDWORD RequiredSize OPTIONAL)
{
    IRecycleBinFile *prbf = (IRecycleBinFile *)hDeletedFile;
    SIZE_T FinalSize;

    HRESULT hr = IRecycleBinFile_GetTypeName(prbf, BufferSize, pTypeName, &FinalSize);

    if (SUCCEEDED(hr))
    {
        if (RequiredSize)
            *RequiredSize = (DWORD)FinalSize;

        return TRUE;
    }
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        SetLastError(HRESULT_CODE(hr));
    else
        SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
}

BOOL WINAPI
GetDeletedFileDetailsA(
    IN HANDLE hDeletedFile,
    IN DWORD BufferSize,
    IN OUT PDELETED_FILE_DETAILS_A FileDetails OPTIONAL,
    OUT LPDWORD RequiredSize OPTIONAL)
{
    PDELETED_FILE_DETAILS_W FileDetailsW = NULL;
    DWORD BufferSizeW = 0;
    BOOL ret = FALSE;

    TRACE("(%p, %lu, %p, %p)\n", hDeletedFile, BufferSize, FileDetails, RequiredSize);

    if (BufferSize >= FIELD_OFFSET(DELETED_FILE_DETAILS_A, FileName))
    {
        BufferSizeW = FIELD_OFFSET(DELETED_FILE_DETAILS_W, FileName)
            + (BufferSize - FIELD_OFFSET(DELETED_FILE_DETAILS_A, FileName)) * sizeof(WCHAR);
    }
    if (FileDetails && BufferSizeW)
    {
        FileDetailsW = HeapAlloc(GetProcessHeap(), 0, BufferSizeW);
        if (!FileDetailsW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }

    ret = GetDeletedFileDetailsW(hDeletedFile, BufferSizeW, FileDetailsW, RequiredSize);
    if (!ret)
        goto cleanup;

    if (FileDetails)
    {
        CopyMemory(FileDetails, FileDetailsW, FIELD_OFFSET(DELETED_FILE_DETAILS_A, FileName));
        if (0 == WideCharToMultiByte(CP_ACP, 0, FileDetailsW->FileName, -1, FileDetails->FileName, BufferSize - FIELD_OFFSET(DELETED_FILE_DETAILS_A, FileName), NULL, NULL))
            goto cleanup;
    }
    ret = TRUE;

cleanup:
    HeapFree(GetProcessHeap(), 0, FileDetailsW);
    return ret;
}

BOOL WINAPI
GetDeletedFileDetailsW(
    IN HANDLE hDeletedFile,
    IN DWORD BufferSize,
    IN OUT PDELETED_FILE_DETAILS_W FileDetails OPTIONAL,
    OUT LPDWORD RequiredSize OPTIONAL)
{
    IRecycleBinFile *rbf = (IRecycleBinFile *)hDeletedFile;
    HRESULT hr;
    SIZE_T NameSize, Needed;

    TRACE("(%p, %lu, %p, %p)\n", hDeletedFile, BufferSize, FileDetails, RequiredSize);

    hr = IRecycleBinFile_GetFileName(rbf, 0, NULL, &NameSize);
    if (!SUCCEEDED(hr))
        goto cleanup;
    Needed = FIELD_OFFSET(DELETED_FILE_DETAILS_W, FileName) + NameSize;
    if (RequiredSize)
        *RequiredSize = (DWORD)Needed;
    if (Needed > BufferSize)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto cleanup;
    }
    hr = IRecycleBinFile_GetFileName(rbf, NameSize, FileDetails->FileName, NULL);
    if (!SUCCEEDED(hr))
        goto cleanup;
    hr = IRecycleBinFile_GetLastModificationTime(rbf, &FileDetails->LastModification);
    if (!SUCCEEDED(hr))
        goto cleanup;
    hr = IRecycleBinFile_GetDeletionTime(rbf, &FileDetails->DeletionTime);
    if (!SUCCEEDED(hr))
        goto cleanup;
    hr = IRecycleBinFile_GetFileSize(rbf, &FileDetails->FileSize);
    if (!SUCCEEDED(hr))
        goto cleanup;
    hr = IRecycleBinFile_GetPhysicalFileSize(rbf, &FileDetails->PhysicalFileSize);
    if (!SUCCEEDED(hr))
        goto cleanup;
    hr = IRecycleBinFile_GetAttributes(rbf, &FileDetails->Attributes);
    if (!SUCCEEDED(hr))
        goto cleanup;

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
GetRecycleBinDetails(
    IN LPCWSTR pszVolume OPTIONAL,
    OUT ULARGE_INTEGER *pulTotalItems,
    OUT ULARGE_INTEGER *pulTotalSize)
{
    pulTotalItems->QuadPart = 0;
    pulTotalSize->QuadPart = 0;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI
RestoreFile(
    IN HANDLE hDeletedFile)
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

HRESULT WINAPI
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
