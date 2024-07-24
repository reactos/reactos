/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Deals with recycle bins of Windows 2000/XP/2003
 * COPYRIGHT:   Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 *              Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
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
        FullPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (dwLength + 1 + MAX_PATH + 1) * sizeof(WCHAR));
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

class RecycleBin5 : public IRecycleBin, public IRecycleBin5
{
public:
    RecycleBin5(
        LPCWSTR VolumePath,
        LPCWSTR RecycleBinDirectory,
        LPWSTR StringSid,
        PTOKEN_USER TokenUserInfo,
        LPWSTR pszFolder);
    virtual ~RecycleBin5();

    /* IUnknown interface */
    STDMETHODIMP QueryInterface(IN REFIID riid, OUT void **ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    /* IRecycleBin interface */
    STDMETHODIMP DeleteFile(IN LPCWSTR szFileName) override;
    STDMETHODIMP EmptyRecycleBin() override;
    STDMETHODIMP EnumObjects(OUT IRecycleBinEnumList **ppEnumList) override;

    /* IRecycleBin5 interface */
    STDMETHODIMP Delete(
        IN LPCWSTR pDeletedFileName,
        IN DELETED_FILE_RECORD *pDeletedFile) override;
    STDMETHODIMP Restore(
        IN LPCWSTR pDeletedFileName,
        IN DELETED_FILE_RECORD *pDeletedFile) override;
    STDMETHODIMP OnClosing(IN IRecycleBinEnumList *prbel) override;

protected:
    LONG m_ref;
    HANDLE m_hInfo;
    HANDLE m_hInfoMapped;
    DWORD m_EnumeratorCount;
    LPWSTR m_VolumePath;
    LPWSTR m_Folder; /* [drive]:\[RECYCLE_BIN_DIRECTORY]\{SID} */

public:
    HRESULT m_hr;
};

STDMETHODIMP RecycleBin5::QueryInterface(IN REFIID riid, OUT void **ppvObject)
{
    TRACE("(%p, %s, %p)\n", this, debugstr_guid(&riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IRecycleBin))
        *ppvObject = static_cast<IRecycleBin *>(this);
    else if (IsEqualIID(riid, IID_IRecycleBin5))
        *ppvObject = static_cast<IRecycleBin5 *>(this);
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) RecycleBin5::AddRef()
{
    TRACE("(%p)\n", this);
    return InterlockedIncrement(&m_ref);
}

RecycleBin5::~RecycleBin5()
{
    TRACE("(%p)\n", this);

    if (m_hInfo && m_hInfo != INVALID_HANDLE_VALUE)
        CloseHandle(m_hInfo);
    if (m_hInfoMapped)
        CloseHandle(m_hInfoMapped);
    SHFree(m_Folder);
}

STDMETHODIMP_(ULONG) RecycleBin5::Release()
{
    TRACE("(%p)\n", this);

    ULONG refCount = InterlockedDecrement(&m_ref);
    if (refCount == 0)
        delete this;
    return refCount;
}

STDMETHODIMP RecycleBin5::DeleteFile(IN LPCWSTR szFileName)
{
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

    TRACE("(%p, %s)\n", this, debugstr_w(szFileName));

    if (m_EnumeratorCount != 0)
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
        szFullName = (LPWSTR)CoTaskMemAlloc(dwBufferLength * sizeof(WCHAR));
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
    CloseHandle(m_hInfoMapped);
    SetFilePointer(m_hInfo, sizeof(DELETED_FILE_RECORD), NULL, FILE_END);
    SetEndOfFile(m_hInfo);
    m_hInfoMapped = CreateFileMappingW(m_hInfo, NULL, PAGE_READWRITE | SEC_COMMIT, 0, 0, NULL);
    if (!m_hInfoMapped)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    /* Open INFO2 file */
    pHeader = (PINFO2_HEADER)MapViewOfFile(m_hInfoMapped, FILE_MAP_WRITE, 0, 0, 0);
    if (!pHeader)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    /* Get number of entries */
    FileSize.u.LowPart = GetFileSize(m_hInfo, &FileSize.u.HighPart);
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
    _snwprintf(DeletedFileName, MAX_PATH, L"%s\\D%c%lu%s", m_Folder, pDeletedFile->dwDriveNumber + 'a', pDeletedFile->dwRecordUniqueId, Extension);

    /* Get cluster size */
    if (!GetDiskFreeSpaceW(m_VolumePath, &SectorsPerCluster, &BytesPerSector, NULL, NULL))
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

STDMETHODIMP RecycleBin5::EmptyRecycleBin()
{
    TRACE("(%p)\n", this);

    while (TRUE)
    {
        IRecycleBinEnumList *prbel;
        HRESULT hr = EnumObjects(&prbel);
        if (!SUCCEEDED(hr))
            return hr;

        IRecycleBinFile *prbf;
        hr = prbel->Next(1, &prbf, NULL);
        prbel->Release();
        if (hr == S_FALSE)
            return S_OK;
        hr = prbf->Delete();
        prbf->Release();
        if (!SUCCEEDED(hr))
            return hr;
    }
}

STDMETHODIMP RecycleBin5::EnumObjects(OUT IRecycleBinEnumList **ppEnumList)
{
    TRACE("(%p, %p)\n", this, ppEnumList);

    IUnknown *pUnk;
    HRESULT hr = RecycleBin5Enum_Constructor(this, m_hInfo, m_hInfoMapped, m_Folder, &pUnk);
    if (!SUCCEEDED(hr))
        return hr;

    IRecycleBinEnumList *prbel;
    hr = pUnk->QueryInterface(IID_IRecycleBinEnumList, (void **)&prbel);
    if (SUCCEEDED(hr))
    {
        m_EnumeratorCount++;
        *ppEnumList = prbel;
    }

    pUnk->Release();
    return hr;
}

STDMETHODIMP RecycleBin5::Delete(
    IN LPCWSTR pDeletedFileName,
    IN DELETED_FILE_RECORD *pDeletedFile)
{
    ULARGE_INTEGER FileSize;
    PINFO2_HEADER pHeader;
    DELETED_FILE_RECORD *pRecord, *pLast;
    DWORD dwEntries, i;

    TRACE("(%p, %s, %p)\n", this, debugstr_w(pDeletedFileName), pDeletedFile);

    if (m_EnumeratorCount != 0)
        return HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);

    pHeader = (PINFO2_HEADER)MapViewOfFile(m_hInfoMapped, FILE_MAP_WRITE, 0, 0, 0);
    if (!pHeader)
        return HRESULT_FROM_WIN32(GetLastError());

    FileSize.u.LowPart = GetFileSize(m_hInfo, &FileSize.u.HighPart);
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
            CloseHandle(m_hInfoMapped);
            SetFilePointer(m_hInfo, -(LONG)sizeof(DELETED_FILE_RECORD), NULL, FILE_END);
            SetEndOfFile(m_hInfo);
            m_hInfoMapped = CreateFileMappingW(m_hInfo, NULL, PAGE_READWRITE | SEC_COMMIT, 0, 0, NULL);
            if (!m_hInfoMapped)
                return HRESULT_FROM_WIN32(GetLastError());
            return S_OK;
        }
        pRecord++;
    }
    UnmapViewOfFile(pHeader);
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

STDMETHODIMP RecycleBin5::Restore(
    IN LPCWSTR pDeletedFileName,
    IN DELETED_FILE_RECORD *pDeletedFile)
{
    ULARGE_INTEGER FileSize;
    PINFO2_HEADER pHeader;
    DELETED_FILE_RECORD *pRecord, *pLast;
    DWORD dwEntries, i;
    SHFILEOPSTRUCTW op;
    int res;

    TRACE("(%p, %s, %p)\n", this, debugstr_w(pDeletedFileName), pDeletedFile);

    if (m_EnumeratorCount != 0)
        return HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);

    pHeader = (PINFO2_HEADER)MapViewOfFile(m_hInfoMapped, FILE_MAP_WRITE, 0, 0, 0);
    if (!pHeader)
        return HRESULT_FROM_WIN32(GetLastError());

    FileSize.u.LowPart = GetFileSize(m_hInfo, &FileSize.u.HighPart);
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
            CloseHandle(m_hInfoMapped);
            SetFilePointer(m_hInfo, -(LONG)sizeof(DELETED_FILE_RECORD), NULL, FILE_END);
            SetEndOfFile(m_hInfo);
            m_hInfoMapped = CreateFileMappingW(m_hInfo, NULL, PAGE_READWRITE | SEC_COMMIT, 0, 0, NULL);
            if (!m_hInfoMapped)
                return HRESULT_FROM_WIN32(GetLastError());
            return S_OK;
        }
        pRecord++;
    }

    UnmapViewOfFile(pHeader);
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

STDMETHODIMP RecycleBin5::OnClosing(IN IRecycleBinEnumList *prbel)
{
    TRACE("(%p, %p)\n", this, prbel);
    m_EnumeratorCount--;
    return S_OK;
}

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
    BufferName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, Needed);
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

RecycleBin5::RecycleBin5(
    LPCWSTR VolumePath,
    LPCWSTR RecycleBinDirectory,
    LPWSTR StringSid,
    PTOKEN_USER TokenUserInfo,
    LPWSTR pszFolder)
    : m_ref(1)
    , m_hInfo(NULL)
    , m_hInfoMapped(NULL)
    , m_EnumeratorCount(0)
    , m_VolumePath(NULL)
    , m_Folder(pszFolder)
    , m_hr(S_OK)
{
    INT len;
    LPWSTR p;
    HRESULT hr;

    if (StringSid)
        len = swprintf(m_Folder, L"%s%s\\%s", VolumePath, RecycleBinDirectory, StringSid);
    else
        len = swprintf(m_Folder, L"%s%s", VolumePath, RecycleBinDirectory);

    p = &m_Folder[len];
    wcscpy(p, L"\\" RECYCLE_BIN_FILE_NAME);

    m_hInfo = CreateFileW(m_Folder, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (m_hInfo == INVALID_HANDLE_VALUE &&
        (GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_FILE_NOT_FOUND))
    {
        *p = UNICODE_NULL;
        hr = RecycleBin5_Create(m_Folder, TokenUserInfo ? TokenUserInfo->User.Sid : NULL);
        *p = L'\\';
        if (!SUCCEEDED(hr))
        {
            m_hr = hr;
            return;
        }
        m_hInfo = CreateFileW(m_Folder, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    }
    if (m_hInfo == INVALID_HANDLE_VALUE)
    {
        m_hr = HRESULT_FROM_WIN32(GetLastError());
        return;
    }
    m_hInfoMapped = CreateFileMappingW(m_hInfo, NULL, PAGE_READWRITE | SEC_COMMIT, 0, 0, NULL);
    if (!m_hInfoMapped)
    {
        m_hr = HRESULT_FROM_WIN32(GetLastError());
        return;
    }
    *p = UNICODE_NULL;
    m_VolumePath = p + 1;
    wcscpy(m_VolumePath, VolumePath);
}

EXTERN_C
HRESULT RecycleBin5_Constructor(IN LPCWSTR VolumePath, OUT IUnknown **ppUnknown)
{
    RecycleBin5 *pThis;
    DWORD FileSystemFlags;
    LPCWSTR RecycleBinDirectory;
    HANDLE tokenHandle = INVALID_HANDLE_VALUE;
    PTOKEN_USER TokenUserInfo = NULL;
    LPWSTR StringSid = NULL;
    DWORD Needed, DirectoryLength;
    HRESULT hr;
    LPWSTR pszFolder;

    if (!ppUnknown)
        return E_POINTER;

    /* Get information about file system */
    if (!GetVolumeInformationW(VolumePath, NULL, 0, NULL, NULL, &FileSystemFlags, NULL, 0))
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
        TokenUserInfo = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, Needed);
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

    pszFolder = (LPWSTR)SHAlloc(Needed);
    if (!pszFolder)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pThis = new RecycleBin5(VolumePath, RecycleBinDirectory, StringSid, TokenUserInfo, pszFolder);
    if (!pThis)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    if (FAILED(pThis->m_hr))
    {
        hr = pThis->m_hr;
        goto cleanup;
    }

    *ppUnknown = static_cast<IRecycleBin5 *>(pThis);
    hr = S_OK;

cleanup:
    if (tokenHandle != INVALID_HANDLE_VALUE)
        CloseHandle(tokenHandle);
    HeapFree(GetProcessHeap(), 0, TokenUserInfo);
    if (StringSid)
        LocalFree(StringSid);
    if (!SUCCEEDED(hr))
    {
        if (pThis)
            delete pThis;
    }
    return hr;
}
