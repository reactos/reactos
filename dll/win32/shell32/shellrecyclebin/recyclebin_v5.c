/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/recyclebin_v5.c
 * PURPOSE:     Deals with recycle bins of Windows 2000/XP/2003
 * PROGRAMMERS: Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "recyclebin_private.h"

#include "sddl.h"

static BOOL
IntDeleteRecursive(
    IN LPCWSTR FullName)
{
    DWORD RemovableAttributes = FILE_ATTRIBUTE_READONLY;
    WIN32_FIND_DATAW FindData;
    HANDLE hSearch = INVALID_HANDLE_VALUE;
    LPWSTR FullPath = NULL, pFilePart;
    DWORD FileAttributes;
    SIZE_T dwLength;
    BOOL ret = FALSE;

    FileAttributes = GetFileAttributesW(FullName);
    if (FileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            ret = TRUE;
        goto cleanup;
    }
    if (FileAttributes & RemovableAttributes)
    {
        if (!SetFileAttributesW(FullName, FileAttributes & ~RemovableAttributes))
            goto cleanup;
    }
    if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        /* Prepare file specification */
        dwLength = wcslen(FullName);
        FullPath = HeapAlloc(GetProcessHeap(), 0, (dwLength + 1 + MAX_PATH + 1) * sizeof(WCHAR));
        if (!FullPath)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        wcscpy(FullPath, FullName);
        if (FullPath[dwLength - 1] != '\\')
        {
            FullPath[dwLength] = '\\';
            dwLength++;
        }
        pFilePart = &FullPath[dwLength];
        wcscpy(pFilePart, L"*");

        /* Enumerate contents, and delete it */
        hSearch = FindFirstFileW(FullPath, &FindData);
        if (hSearch == INVALID_HANDLE_VALUE)
            goto cleanup;
        do
        {
            if (!(FindData.cFileName[0] == '.' &&
                (FindData.cFileName[1] == '\0' || (FindData.cFileName[1] == '.' && FindData.cFileName[2] == '\0'))))
            {
                wcscpy(pFilePart, FindData.cFileName);
                if (!IntDeleteRecursive(FullPath))
                {
                    FindClose(hSearch);
                    goto cleanup;
                }
            }
        }
        while (FindNextFileW(hSearch, &FindData));
        FindClose(hSearch);
        if (GetLastError() != ERROR_NO_MORE_FILES)
            goto cleanup;

        /* Remove (now empty) directory */
        if (!RemoveDirectoryW(FullName))
            goto cleanup;
    }
    else
    {
        if (!DeleteFileW(FullName))
            goto cleanup;
    }
    ret = TRUE;

cleanup:
    HeapFree(GetProcessHeap(), 0, FullPath);
    return ret;
}

struct RecycleBin5
{
    ULONG ref;
    IRecycleBin5 recycleBinImpl;
    HANDLE hInfo;
    HANDLE hInfoMapped;

    DWORD EnumeratorCount;

    LPWSTR VolumePath;
    WCHAR Folder[ANY_SIZE]; /* [drive]:\[RECYCLE_BIN_DIRECTORY]\{SID} */
};

static HRESULT STDMETHODCALLTYPE
RecycleBin5_RecycleBin5_QueryInterface(
    IRecycleBin5 *This,
    REFIID riid,
    void **ppvObject)
{
    struct RecycleBin5 *s = CONTAINING_RECORD(This, struct RecycleBin5, recycleBinImpl);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppvObject = &s->recycleBinImpl;
    else if (IsEqualIID(riid, &IID_IRecycleBin))
        *ppvObject = &s->recycleBinImpl;
    else if (IsEqualIID(riid, &IID_IRecycleBin5))
        *ppvObject = &s->recycleBinImpl;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(This);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
RecycleBin5_RecycleBin5_AddRef(
    IRecycleBin5 *This)
{
    struct RecycleBin5 *s = CONTAINING_RECORD(This, struct RecycleBin5, recycleBinImpl);
    ULONG refCount = InterlockedIncrement((PLONG)&s->ref);
    TRACE("(%p)\n", This);
    return refCount;
}

static VOID
RecycleBin5_Destructor(
    struct RecycleBin5 *s)
{
    TRACE("(%p)\n", s);

    if (s->hInfo && s->hInfo != INVALID_HANDLE_VALUE)
        CloseHandle(s->hInfo);
    if (s->hInfoMapped)
        CloseHandle(s->hInfoMapped);
    CoTaskMemFree(s);
}

static ULONG STDMETHODCALLTYPE
RecycleBin5_RecycleBin5_Release(
    IRecycleBin5 *This)
{
    struct RecycleBin5 *s = CONTAINING_RECORD(This, struct RecycleBin5, recycleBinImpl);
    ULONG refCount;

    TRACE("(%p)\n", This);

    refCount = InterlockedDecrement((PLONG)&s->ref);

    if (refCount == 0)
        RecycleBin5_Destructor(s);

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5_RecycleBin5_DeleteFile(
    IN IRecycleBin5 *This,
    IN LPCWSTR szFileName)
{
    struct RecycleBin5 *s = CONTAINING_RECORD(This, struct RecycleBin5, recycleBinImpl);
    LPWSTR szFullName = NULL;
    DWORD dwBufferLength = 0;
    LPWSTR lpFilePart;
    LPCWSTR Extension;
    WCHAR DeletedFileName[MAX_PATH];
    DWORD len;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PINFO2_HEADER pHeader = NULL;
    PDELETED_FILE_RECORD pDeletedFile;
    ULARGE_INTEGER FileSize;
    DWORD dwAttributes, dwEntries;
    SYSTEMTIME SystemTime;
    DWORD ClusterSize, BytesPerSector, SectorsPerCluster;
    HRESULT hr;

    TRACE("(%p, %s)\n", This, debugstr_w(szFileName));

    if (s->EnumeratorCount != 0)
        return HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);

    /* Get full file name */
    while (TRUE)
    {
        len = GetFullPathNameW(szFileName, dwBufferLength, szFullName, &lpFilePart);
        if (len == 0)
        {
            if (szFullName)
                CoTaskMemFree(szFullName);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        else if (len < dwBufferLength)
            break;
        if (szFullName)
            CoTaskMemFree(szFullName);
        dwBufferLength = len;
        szFullName = CoTaskMemAlloc(dwBufferLength * sizeof(WCHAR));
        if (!szFullName)
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    /* Check if file exists */
    dwAttributes = GetFileAttributesW(szFullName);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES)
    {
        CoTaskMemFree(szFullName);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (dwBufferLength < 2 || szFullName[1] != ':')
    {
        /* Not a local file */
        CoTaskMemFree(szFullName);
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }

    hFile = CreateFileW(szFullName, 0, 0, NULL, OPEN_EXISTING, (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_FLAG_BACKUP_SEMANTICS : 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    /* Increase INFO2 file size */
    CloseHandle(s->hInfoMapped);
    SetFilePointer(s->hInfo, sizeof(DELETED_FILE_RECORD), NULL, FILE_END);
    SetEndOfFile(s->hInfo);
    s->hInfoMapped = CreateFileMappingW(s->hInfo, NULL, PAGE_READWRITE | SEC_COMMIT, 0, 0, NULL);
    if (!s->hInfoMapped)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    /* Open INFO2 file */
    pHeader = MapViewOfFile(s->hInfoMapped, FILE_MAP_WRITE, 0, 0, 0);
    if (!pHeader)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    /* Get number of entries */
    FileSize.u.LowPart = GetFileSize(s->hInfo, &FileSize.u.HighPart);
    if (FileSize.u.LowPart < sizeof(INFO2_HEADER))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    dwEntries = (DWORD)((FileSize.QuadPart - sizeof(INFO2_HEADER)) / sizeof(DELETED_FILE_RECORD)) - 1;
    pDeletedFile = ((PDELETED_FILE_RECORD)(pHeader + 1)) + dwEntries;

    /* Get file size */
#if 0
    if (!GetFileSizeEx(hFile, (PLARGE_INTEGER)&FileSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
#else
    FileSize.u.LowPart = GetFileSize(hFile, &FileSize.u.HighPart);
    if (FileSize.u.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
#endif
    /* Check if file size is > 4Gb */
    if (FileSize.u.HighPart != 0)
    {
        /* Yes, this recyclebin can't support this file */
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        goto cleanup;
    }
    pHeader->dwTotalLogicalSize += FileSize.u.LowPart;

    /* Generate new name */
    Extension = wcsrchr(szFullName, '.');
    ZeroMemory(pDeletedFile, sizeof(DELETED_FILE_RECORD));
    if (dwEntries == 0)
        pDeletedFile->dwRecordUniqueId = 0;
    else
    {
        PDELETED_FILE_RECORD pLastDeleted = ((PDELETED_FILE_RECORD)(pHeader + 1)) + dwEntries - 1;
        pDeletedFile->dwRecordUniqueId = pLastDeleted->dwRecordUniqueId + 1;
    }
    pDeletedFile->dwDriveNumber = tolower(szFullName[0]) - 'a';
    _snwprintf(DeletedFileName, MAX_PATH, L"%s\\D%c%lu%s", s->Folder, pDeletedFile->dwDriveNumber + 'a', pDeletedFile->dwRecordUniqueId, Extension);

    /* Get cluster size */
    if (!GetDiskFreeSpaceW(s->VolumePath, &SectorsPerCluster, &BytesPerSector, NULL, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    ClusterSize = BytesPerSector * SectorsPerCluster;

    /* Get current time */
    GetSystemTime(&SystemTime);
    if (!SystemTimeToFileTime(&SystemTime, &pDeletedFile->DeletionTime))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    pDeletedFile->dwPhysicalFileSize = ROUND_UP(FileSize.u.LowPart, ClusterSize);

    /* Set name */
    wcscpy(pDeletedFile->FileNameW, szFullName);
    if (WideCharToMultiByte(CP_ACP, 0, pDeletedFile->FileNameW, -1, pDeletedFile->FileNameA, MAX_PATH, NULL, NULL) == 0)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
        SetLastError(ERROR_INVALID_NAME);
        goto cleanup;
    }

    /* Move file */
    if (MoveFileW(szFullName, DeletedFileName))
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

cleanup:
    if (pHeader)
        UnmapViewOfFile(pHeader);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    CoTaskMemFree(szFullName);
    return hr;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5_RecycleBin5_EmptyRecycleBin(
    IN IRecycleBin5 *This)
{
    IRecycleBinEnumList *prbel;
    IRecycleBinFile *prbf;
    HRESULT hr;

    TRACE("(%p)\n", This);

    while (TRUE)
    {
        hr = IRecycleBin5_EnumObjects(This, &prbel);
        if (!SUCCEEDED(hr))
            return hr;
        hr = IRecycleBinEnumList_Next(prbel, 1, &prbf, NULL);
        IRecycleBinEnumList_Release(prbel);
        if (hr == S_FALSE)
            return S_OK;
        hr = IRecycleBinFile_Delete(prbf);
        IRecycleBinFile_Release(prbf);
        if (!SUCCEEDED(hr))
            return hr;
    }
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5_RecycleBin5_EnumObjects(
    IN IRecycleBin5 *This,
    OUT IRecycleBinEnumList **ppEnumList)
{
    struct RecycleBin5 *s = CONTAINING_RECORD(This, struct RecycleBin5, recycleBinImpl);
    IRecycleBinEnumList *prbel;
    HRESULT hr;
    IUnknown *pUnk;

    TRACE("(%p, %p)\n", This, ppEnumList);

    hr = RecycleBin5Enum_Constructor(This, s->hInfo, s->hInfoMapped, s->Folder, &pUnk);
    if (!SUCCEEDED(hr))
        return hr;

    hr = IUnknown_QueryInterface(pUnk, &IID_IRecycleBinEnumList, (void **)&prbel);
    if (SUCCEEDED(hr))
    {
        s->EnumeratorCount++;
        *ppEnumList = prbel;
    }
    IUnknown_Release(pUnk);
    return hr;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5_RecycleBin5_Delete(
    IN IRecycleBin5 *This,
    IN LPCWSTR pDeletedFileName,
    IN DELETED_FILE_RECORD *pDeletedFile)
{
    struct RecycleBin5 *s = CONTAINING_RECORD(This, struct RecycleBin5, recycleBinImpl);
    ULARGE_INTEGER FileSize;
    PINFO2_HEADER pHeader;
    DELETED_FILE_RECORD *pRecord, *pLast;
    DWORD dwEntries, i;

    TRACE("(%p, %s, %p)\n", This, debugstr_w(pDeletedFileName), pDeletedFile);

    if (s->EnumeratorCount != 0)
        return HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);

    pHeader = MapViewOfFile(s->hInfoMapped, FILE_MAP_WRITE, 0, 0, 0);
    if (!pHeader)
        return HRESULT_FROM_WIN32(GetLastError());

    FileSize.u.LowPart = GetFileSize(s->hInfo, &FileSize.u.HighPart);
    if (FileSize.u.LowPart == 0)
    {
        UnmapViewOfFile(pHeader);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    dwEntries = (DWORD)((FileSize.QuadPart - sizeof(INFO2_HEADER)) / sizeof(DELETED_FILE_RECORD));

    pRecord = (DELETED_FILE_RECORD *)(pHeader + 1);
    for (i = 0; i < dwEntries; i++)
    {
        if (pRecord->dwRecordUniqueId == pDeletedFile->dwRecordUniqueId)
        {
            /* Delete file */
            if (!IntDeleteRecursive(pDeletedFileName))
            {
                UnmapViewOfFile(pHeader);
                return HRESULT_FROM_WIN32(GetLastError());
            }

            /* Clear last entry in the file */
            MoveMemory(pRecord, pRecord + 1, (dwEntries - i - 1) * sizeof(DELETED_FILE_RECORD));
            pLast = pRecord + (dwEntries - i - 1);
            ZeroMemory(pLast, sizeof(DELETED_FILE_RECORD));
            UnmapViewOfFile(pHeader);

            /* Resize file */
            CloseHandle(s->hInfoMapped);
            SetFilePointer(s->hInfo, -(LONG)sizeof(DELETED_FILE_RECORD), NULL, FILE_END);
            SetEndOfFile(s->hInfo);
            s->hInfoMapped = CreateFileMappingW(s->hInfo, NULL, PAGE_READWRITE | SEC_COMMIT, 0, 0, NULL);
            if (!s->hInfoMapped)
                return HRESULT_FROM_WIN32(GetLastError());
            return S_OK;
        }
        pRecord++;
    }
    UnmapViewOfFile(pHeader);
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5_RecycleBin5_Restore(
    IN IRecycleBin5 *This,
    IN LPCWSTR pDeletedFileName,
    IN DELETED_FILE_RECORD *pDeletedFile)
{
    struct RecycleBin5 *s = CONTAINING_RECORD(This, struct RecycleBin5, recycleBinImpl);
    ULARGE_INTEGER FileSize;
    PINFO2_HEADER pHeader;
    DELETED_FILE_RECORD *pRecord, *pLast;
    DWORD dwEntries, i;
    SHFILEOPSTRUCTW op;
    int res;

    TRACE("(%p, %s, %p)\n", This, debugstr_w(pDeletedFileName), pDeletedFile);

    if (s->EnumeratorCount != 0)
        return HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);

    pHeader = MapViewOfFile(s->hInfoMapped, FILE_MAP_WRITE, 0, 0, 0);
    if (!pHeader)
        return HRESULT_FROM_WIN32(GetLastError());

    FileSize.u.LowPart = GetFileSize(s->hInfo, &FileSize.u.HighPart);
    if (FileSize.u.LowPart == 0)
    {
        UnmapViewOfFile(pHeader);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    dwEntries = (DWORD)((FileSize.QuadPart - sizeof(INFO2_HEADER)) / sizeof(DELETED_FILE_RECORD));

    pRecord = (DELETED_FILE_RECORD *)(pHeader + 1);
    for (i = 0; i < dwEntries; i++)
    {
        if (pRecord->dwRecordUniqueId == pDeletedFile->dwRecordUniqueId)
        {
            /* Restore file */
            ZeroMemory(&op, sizeof(op));
            op.wFunc = FO_MOVE;
            op.pFrom = pDeletedFileName;
            op.pTo = pDeletedFile->FileNameW;

            res = SHFileOperationW(&op);
            if (res)
            {
                ERR("SHFileOperationW failed with 0x%x\n", res);
                UnmapViewOfFile(pHeader);
                return E_FAIL;
            }

            /* Clear last entry in the file */
            MoveMemory(pRecord, pRecord + 1, (dwEntries - i - 1) * sizeof(DELETED_FILE_RECORD));
            pLast = pRecord + (dwEntries - i - 1);
            ZeroMemory(pLast, sizeof(DELETED_FILE_RECORD));
            UnmapViewOfFile(pHeader);

            /* Resize file */
            CloseHandle(s->hInfoMapped);
            SetFilePointer(s->hInfo, -(LONG)sizeof(DELETED_FILE_RECORD), NULL, FILE_END);
            SetEndOfFile(s->hInfo);
            s->hInfoMapped = CreateFileMappingW(s->hInfo, NULL, PAGE_READWRITE | SEC_COMMIT, 0, 0, NULL);
            if (!s->hInfoMapped)
                return HRESULT_FROM_WIN32(GetLastError());
            return S_OK;
        }
        pRecord++;
    }

    UnmapViewOfFile(pHeader);
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5_RecycleBin5_OnClosing(
    IN IRecycleBin5 *This,
    IN IRecycleBinEnumList *prbel)
{
    struct RecycleBin5 *s = CONTAINING_RECORD(This, struct RecycleBin5, recycleBinImpl);
    TRACE("(%p, %p)\n", This, prbel);
    s->EnumeratorCount--;
    return S_OK;
}

CONST_VTBL struct IRecycleBin5Vtbl RecycleBin5Vtbl =
{
    RecycleBin5_RecycleBin5_QueryInterface,
    RecycleBin5_RecycleBin5_AddRef,
    RecycleBin5_RecycleBin5_Release,
    RecycleBin5_RecycleBin5_DeleteFile,
    RecycleBin5_RecycleBin5_EmptyRecycleBin,
    RecycleBin5_RecycleBin5_EnumObjects,
    RecycleBin5_RecycleBin5_Delete,
    RecycleBin5_RecycleBin5_Restore,
    RecycleBin5_RecycleBin5_OnClosing,
};

static HRESULT
RecycleBin5_Create(
    IN LPCWSTR Folder,
    IN PSID OwnerSid OPTIONAL)
{
    LPWSTR BufferName = NULL;
    LPWSTR Separator; /* Pointer into BufferName buffer */
    LPWSTR FileName; /* Pointer into BufferName buffer */
    LPCSTR DesktopIniContents = "[.ShellClassInfo]\r\nCLSID={645FF040-5081-101B-9F08-00AA002F954E}\r\n";
    INFO2_HEADER Info2Contents[] = { { 5, 0, 0, 0x320, 0 } };
    DWORD BytesToWrite, BytesWritten, Needed;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HRESULT hr;

    Needed = (wcslen(Folder) + 1 + max(wcslen(RECYCLE_BIN_FILE_NAME), wcslen(L"desktop.ini")) + 1) * sizeof(WCHAR);
    BufferName = HeapAlloc(GetProcessHeap(), 0, Needed);
    if (!BufferName)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    wcscpy(BufferName, Folder);
    Separator = wcsstr(&BufferName[3], L"\\");
    if (Separator)
        *Separator = UNICODE_NULL;
    if (!CreateDirectoryW(BufferName, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    SetFileAttributesW(BufferName, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
    if (Separator)
    {
        *Separator = L'\\';
        if (!CreateDirectoryW(BufferName, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }
    }

    if (OwnerSid)
    {
        //DWORD rc;

        /* Add ACL to allow only user/SYSTEM to open it */
        /* FIXME: rc = SetNamedSecurityInfo(
            BufferName,
            SE_FILE_OBJECT,
            ???,
            OwnerSid,
            NULL,
            ???,
            ???);
        if (rc != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(rc);
            goto cleanup;
        }
        */
    }

    wcscat(BufferName, L"\\");
    FileName = &BufferName[wcslen(BufferName)];

    /* Create desktop.ini */
    wcscpy(FileName, L"desktop.ini");
    hFile = CreateFileW(BufferName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    BytesToWrite = strlen(DesktopIniContents);
    if (!WriteFile(hFile, DesktopIniContents, (DWORD)BytesToWrite, &BytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    if (BytesWritten != BytesToWrite)
    {
        hr = E_FAIL;
        goto cleanup;
    }
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    /* Create empty INFO2 file */
    wcscpy(FileName, RECYCLE_BIN_FILE_NAME);
    hFile = CreateFileW(BufferName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    BytesToWrite = sizeof(Info2Contents);
    if (!WriteFile(hFile, Info2Contents, (DWORD)BytesToWrite, &BytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    if (BytesWritten == BytesToWrite)
        hr = S_OK;
    else
        hr = E_FAIL;

cleanup:
    HeapFree(GetProcessHeap(), 0, BufferName);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    return hr;
}

HRESULT RecycleBin5_Constructor(IN LPCWSTR VolumePath, OUT IUnknown **ppUnknown)
{
    struct RecycleBin5 *s = NULL;
    DWORD FileSystemFlags;
    LPCWSTR RecycleBinDirectory;
    HANDLE tokenHandle = INVALID_HANDLE_VALUE;
    PTOKEN_USER TokenUserInfo = NULL;
    LPWSTR StringSid = NULL, p;
    DWORD Needed, DirectoryLength;
    INT len;
    HRESULT hr;

    if (!ppUnknown)
        return E_POINTER;

    /* Get information about file system */
    if (!GetVolumeInformationW(
        VolumePath,
        NULL,
        0,
        NULL,
        NULL,
        &FileSystemFlags,
        NULL,
        0))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    if (!(FileSystemFlags & FILE_PERSISTENT_ACLS))
        RecycleBinDirectory = RECYCLE_BIN_DIRECTORY_WITHOUT_ACL;
    else
    {
        RecycleBinDirectory = RECYCLE_BIN_DIRECTORY_WITH_ACL;

        /* Get user SID */
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }
        if (GetTokenInformation(tokenHandle, TokenUser, NULL, 0, &Needed))
        {
            hr = E_FAIL;
            goto cleanup;
        }
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }
        TokenUserInfo = HeapAlloc(GetProcessHeap(), 0, Needed);
        if (!TokenUserInfo)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        if (!GetTokenInformation(tokenHandle, TokenUser, TokenUserInfo, (DWORD)Needed, &Needed))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }
        if (!ConvertSidToStringSidW(TokenUserInfo->User.Sid, &StringSid))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }
    }

    DirectoryLength = wcslen(VolumePath) + wcslen(RecycleBinDirectory) + 1;
    if (StringSid)
        DirectoryLength += wcslen(StringSid) + 1;
    DirectoryLength += 1 + wcslen(RECYCLE_BIN_FILE_NAME);
    DirectoryLength += wcslen(VolumePath) + 1;
    Needed = (DirectoryLength + 1) * sizeof(WCHAR);

    s = CoTaskMemAlloc(sizeof(struct RecycleBin5) + Needed);
    if (!s)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    ZeroMemory(s, sizeof(struct RecycleBin5));
    s->recycleBinImpl.lpVtbl = &RecycleBin5Vtbl;
    s->ref = 1;
    if (StringSid)
        len = swprintf(s->Folder, L"%s%s\\%s", VolumePath, RecycleBinDirectory, StringSid);
    else
        len = swprintf(s->Folder, L"%s%s", VolumePath, RecycleBinDirectory);
    p = &s->Folder[len];
    wcscpy(p, L"\\" RECYCLE_BIN_FILE_NAME);
    s->hInfo = CreateFileW(s->Folder, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (s->hInfo == INVALID_HANDLE_VALUE && (GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_FILE_NOT_FOUND))
    {
        *p = UNICODE_NULL;
        hr = RecycleBin5_Create(s->Folder, TokenUserInfo ? TokenUserInfo->User.Sid : NULL);
        *p = L'\\';
        if (!SUCCEEDED(hr))
            goto cleanup;
        s->hInfo = CreateFileW(s->Folder, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    }
    if (s->hInfo == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    s->hInfoMapped = CreateFileMappingW(s->hInfo, NULL, PAGE_READWRITE | SEC_COMMIT, 0, 0, NULL);
    if (!s->hInfoMapped)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    *p = UNICODE_NULL;
    s->VolumePath = p + 1;
    wcscpy(s->VolumePath, VolumePath);

    *ppUnknown = (IUnknown *)&s->recycleBinImpl;

    hr = S_OK;

cleanup:
    if (tokenHandle != INVALID_HANDLE_VALUE)
        CloseHandle(tokenHandle);
    HeapFree(GetProcessHeap(), 0, TokenUserInfo);
    if (StringSid)
        LocalFree(StringSid);
    if (!SUCCEEDED(hr))
    {
        if (s)
            RecycleBin5_Destructor(s);
    }
    return hr;
}
