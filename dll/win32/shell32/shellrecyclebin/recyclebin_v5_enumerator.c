/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/recyclebin_v5_enumerator.c
 * PURPOSE:     Enumerates contents of a MS Windows 2000/XP/2003 recyclebin
 * PROGRAMMERS: Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "recyclebin_private.h"

struct RecycleBin5File
{
    ULONG ref;
    IRecycleBin5 *recycleBin;
    DELETED_FILE_RECORD deletedFile;
    IRecycleBinFile recycleBinFileImpl;
    WCHAR FullName[ANY_SIZE];
};

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_QueryInterface(
    IN IRecycleBinFile *This,
    IN REFIID riid,
    OUT void **ppvObject)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppvObject = &s->recycleBinFileImpl;
    else if (IsEqualIID(riid, &IID_IRecycleBinFile))
        *ppvObject = &s->recycleBinFileImpl;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(This);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_AddRef(
    IN IRecycleBinFile *This)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    ULONG refCount = InterlockedIncrement((PLONG)&s->ref);
    TRACE("(%p)\n", This);
    return refCount;
}

static VOID
RecycleBin5File_Destructor(
    struct RecycleBin5File *s)
{
    TRACE("(%p)\n", s);

    IRecycleBin5_Release(s->recycleBin);
    CoTaskMemFree(s);
}

static ULONG STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_Release(
    IN IRecycleBinFile *This)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    ULONG refCount;

    TRACE("(%p)\n", This);

    refCount = InterlockedDecrement((PLONG)&s->ref);

    if (refCount == 0)
        RecycleBin5File_Destructor(s);

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_GetLastModificationTime(
    IN IRecycleBinFile *This,
    OUT FILETIME *pLastModificationTime)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    HRESULT hr;
    DWORD dwAttributes;
    HANDLE hFile;

    TRACE("(%p, %p)\n", This, pLastModificationTime);

    dwAttributes = GetFileAttributesW(s->FullName);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
        hFile = CreateFileW(s->FullName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    else
        hFile = CreateFileW(s->FullName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    if (GetFileTime(hFile, NULL, NULL, pLastModificationTime))
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(hFile);
    return hr;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_GetDeletionTime(
    IN IRecycleBinFile *This,
    OUT FILETIME *pDeletionTime)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    TRACE("(%p, %p)\n", This, pDeletionTime);
    *pDeletionTime = s->deletedFile.DeletionTime;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_GetFileSize(
    IN IRecycleBinFile *This,
    OUT ULARGE_INTEGER *pFileSize)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    HRESULT hr;
    DWORD dwAttributes;
    HANDLE hFile;

    TRACE("(%p, %p)\n", This, pFileSize);

    dwAttributes = GetFileAttributesW(s->FullName);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        pFileSize->QuadPart = 0;
        return S_OK;
    }

    hFile = CreateFileW(s->FullName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());
    pFileSize->u.LowPart = GetFileSize(hFile, &pFileSize->u.HighPart);
    if (pFileSize->u.LowPart != INVALID_FILE_SIZE)
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(hFile);
    return hr;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_GetPhysicalFileSize(
    IN IRecycleBinFile *This,
    OUT ULARGE_INTEGER *pPhysicalFileSize)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    TRACE("(%p, %p)\n", This, pPhysicalFileSize);
    pPhysicalFileSize->u.HighPart = 0;
    pPhysicalFileSize->u.LowPart = s->deletedFile.dwPhysicalFileSize;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_GetAttributes(
    IN IRecycleBinFile *This,
    OUT DWORD *pAttributes)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    DWORD dwAttributes;

    TRACE("(%p, %p)\n", This, pAttributes);

    dwAttributes = GetFileAttributesW(s->FullName);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());

    *pAttributes = dwAttributes;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_GetFileName(
    IN IRecycleBinFile *This,
    IN SIZE_T BufferSize,
    IN OUT LPWSTR Buffer,
    OUT SIZE_T *RequiredSize)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    DWORD dwRequired;

    TRACE("(%p, %u, %p, %p)\n", This, BufferSize, Buffer, RequiredSize);

    dwRequired = (DWORD)(wcslen(s->deletedFile.FileNameW) + 1) * sizeof(WCHAR);
    if (RequiredSize)
        *RequiredSize = dwRequired;

    if (BufferSize == 0 && !Buffer)
        return S_OK;

    if (BufferSize < dwRequired)
        return E_OUTOFMEMORY;
    CopyMemory(Buffer, s->deletedFile.FileNameW, dwRequired);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_GetTypeName(
    IN IRecycleBinFile *This,
    IN SIZE_T BufferSize,
    IN OUT LPWSTR Buffer,
    OUT SIZE_T *RequiredSize)
{
    HRESULT hr;

    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    DWORD dwRequired;
    DWORD dwAttributes;
    SHFILEINFOW shFileInfo;

    TRACE("(%p, %u, %p, %p)\n", This, BufferSize, Buffer, RequiredSize);

    hr = RecycleBin5File_RecycleBinFile_GetAttributes(This, &dwAttributes);
    if (!SUCCEEDED(hr))
        return hr;

    hr = SHGetFileInfoW(s->FullName, dwAttributes, &shFileInfo, sizeof(shFileInfo), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
    if (!SUCCEEDED(hr))
        return hr;

    dwRequired = (DWORD)(wcslen(shFileInfo.szTypeName) + 1) * sizeof(WCHAR);
    if (RequiredSize)
        *RequiredSize = dwRequired;

    if (BufferSize == 0 && !Buffer)
        return S_OK;

    if (BufferSize < dwRequired)
        return E_OUTOFMEMORY;
    CopyMemory(Buffer, shFileInfo.szTypeName, dwRequired);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_Delete(
    IN IRecycleBinFile *This)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    TRACE("(%p)\n", This);
    return IRecycleBin5_Delete(s->recycleBin, s->FullName, &s->deletedFile);
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5File_RecycleBinFile_Restore(
    IN IRecycleBinFile *This)
{
    struct RecycleBin5File *s = CONTAINING_RECORD(This, struct RecycleBin5File, recycleBinFileImpl);
    TRACE("(%p)\n", This);
    return IRecycleBin5_Restore(s->recycleBin, s->FullName, &s->deletedFile);
}

CONST_VTBL struct IRecycleBinFileVtbl RecycleBin5FileVtbl =
{
    RecycleBin5File_RecycleBinFile_QueryInterface,
    RecycleBin5File_RecycleBinFile_AddRef,
    RecycleBin5File_RecycleBinFile_Release,
    RecycleBin5File_RecycleBinFile_GetLastModificationTime,
    RecycleBin5File_RecycleBinFile_GetDeletionTime,
    RecycleBin5File_RecycleBinFile_GetFileSize,
    RecycleBin5File_RecycleBinFile_GetPhysicalFileSize,
    RecycleBin5File_RecycleBinFile_GetAttributes,
    RecycleBin5File_RecycleBinFile_GetFileName,
    RecycleBin5File_RecycleBinFile_GetTypeName,
    RecycleBin5File_RecycleBinFile_Delete,
    RecycleBin5File_RecycleBinFile_Restore,
};

static HRESULT
RecycleBin5File_Constructor(
    IN IRecycleBin5 *prb,
    IN LPCWSTR Folder,
    IN PDELETED_FILE_RECORD pDeletedFile,
    OUT IRecycleBinFile **ppFile)
{
    struct RecycleBin5File *s = NULL;
    LPCWSTR Extension;
    SIZE_T Needed;

    if (!ppFile)
        return E_POINTER;

    Extension = wcsrchr(pDeletedFile->FileNameW, '.');
    if (Extension < wcsrchr(pDeletedFile->FileNameW, '\\'))
        Extension = NULL;
    Needed = wcslen(Folder) + 13;
    if (Extension)
        Needed += wcslen(Extension);
    Needed *= sizeof(WCHAR);

    s = CoTaskMemAlloc(sizeof(struct RecycleBin5File) + Needed);
    if (!s)
        return E_OUTOFMEMORY;
    ZeroMemory(s, sizeof(struct RecycleBin5File) + Needed);
    s->recycleBinFileImpl.lpVtbl = &RecycleBin5FileVtbl;
    s->ref = 1;
    s->deletedFile = *pDeletedFile;
    s->recycleBin = prb;
    IRecycleBin5_AddRef(s->recycleBin);
    *ppFile = &s->recycleBinFileImpl;
    wsprintfW(s->FullName, L"%s\\D%c%lu%s", Folder, pDeletedFile->dwDriveNumber + 'a', pDeletedFile->dwRecordUniqueId, Extension);
    if (GetFileAttributesW(s->FullName) == INVALID_FILE_ATTRIBUTES)
    {
        RecycleBin5File_Destructor(s);
        return E_FAIL;
    }

    return S_OK;
}

struct RecycleBin5Enum
{
    ULONG ref;
    IRecycleBin5 *recycleBin;
    HANDLE hInfo;
    INFO2_HEADER *pInfo;
    DWORD dwCurrent;
    IRecycleBinEnumList recycleBinEnumImpl;
    WCHAR szPrefix[ANY_SIZE];
};

static HRESULT STDMETHODCALLTYPE
RecycleBin5Enum_RecycleBinEnumList_QueryInterface(
    IN IRecycleBinEnumList *This,
    IN REFIID riid,
    OUT void **ppvObject)
{
    struct RecycleBin5Enum *s = CONTAINING_RECORD(This, struct RecycleBin5Enum, recycleBinEnumImpl);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppvObject = &s->recycleBinEnumImpl;
    else if (IsEqualIID(riid, &IID_IRecycleBinEnumList))
        *ppvObject = &s->recycleBinEnumImpl;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(This);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
RecycleBin5Enum_RecycleBinEnumList_AddRef(
    IN IRecycleBinEnumList *This)
{
    struct RecycleBin5Enum *s = CONTAINING_RECORD(This, struct RecycleBin5Enum, recycleBinEnumImpl);
    ULONG refCount = InterlockedIncrement((PLONG)&s->ref);
    TRACE("(%p)\n", This);
    return refCount;
}

static VOID
RecycleBin5Enum_Destructor(
    struct RecycleBin5Enum *s)
{
    TRACE("(%p)\n", s);

    IRecycleBin5_OnClosing(s->recycleBin, &s->recycleBinEnumImpl);
    UnmapViewOfFile(s->pInfo);
    IRecycleBin5_Release(s->recycleBin);
    CoTaskMemFree(s);
}

static ULONG STDMETHODCALLTYPE
RecycleBin5Enum_RecycleBinEnumList_Release(
    IN IRecycleBinEnumList *This)
{
    struct RecycleBin5Enum *s = CONTAINING_RECORD(This, struct RecycleBin5Enum, recycleBinEnumImpl);
    ULONG refCount;

    TRACE("(%p)\n", This);

    refCount = InterlockedDecrement((PLONG)&s->ref);

    if (refCount == 0)
        RecycleBin5Enum_Destructor(s);

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5Enum_RecycleBinEnumList_Next(
    IRecycleBinEnumList *This,
    IN DWORD celt,
    IN OUT IRecycleBinFile **rgelt,
    OUT DWORD *pceltFetched)
{
    struct RecycleBin5Enum *s = CONTAINING_RECORD(This, struct RecycleBin5Enum, recycleBinEnumImpl);
    ULARGE_INTEGER FileSize;
    INFO2_HEADER *pHeader = s->pInfo;
    DELETED_FILE_RECORD *pDeletedFile;
    DWORD fetched = 0, i;
    DWORD dwEntries;
    HRESULT hr;

    TRACE("(%p, %u, %p, %p)\n", This, celt, rgelt, pceltFetched);

    if (!rgelt)
        return E_POINTER;
    if (!pceltFetched && celt > 1)
        return E_INVALIDARG;

    FileSize.u.LowPart = GetFileSize(s->hInfo, &FileSize.u.HighPart);
    if (FileSize.u.LowPart == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    dwEntries = (DWORD)((FileSize.QuadPart - sizeof(INFO2_HEADER)) / sizeof(DELETED_FILE_RECORD));

    i = s->dwCurrent;
    pDeletedFile = (DELETED_FILE_RECORD *)(pHeader + 1) + i;
    for (; i < dwEntries && fetched < celt; i++)
    {
        hr = RecycleBin5File_Constructor(s->recycleBin, s->szPrefix, pDeletedFile, &rgelt[fetched]);
        if (SUCCEEDED(hr))
            fetched++;
        pDeletedFile++;
    }

    s->dwCurrent = i;
    if (pceltFetched)
        *pceltFetched = fetched;
    if (fetched == celt)
        return S_OK;
    else
        return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5Enum_RecycleBinEnumList_Skip(
    IN IRecycleBinEnumList *This,
    IN DWORD celt)
{
    struct RecycleBin5Enum *s = CONTAINING_RECORD(This, struct RecycleBin5Enum, recycleBinEnumImpl);
    TRACE("(%p, %u)\n", This, celt);
    s->dwCurrent += celt;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
RecycleBin5Enum_RecycleBinEnumList_Reset(
    IN IRecycleBinEnumList *This)
{
    struct RecycleBin5Enum *s = CONTAINING_RECORD(This, struct RecycleBin5Enum, recycleBinEnumImpl);
    TRACE("(%p)\n", This);
    s->dwCurrent = 0;
    return S_OK;
}

CONST_VTBL struct IRecycleBinEnumListVtbl RecycleBin5EnumVtbl =
{
    RecycleBin5Enum_RecycleBinEnumList_QueryInterface,
    RecycleBin5Enum_RecycleBinEnumList_AddRef,
    RecycleBin5Enum_RecycleBinEnumList_Release,
    RecycleBin5Enum_RecycleBinEnumList_Next,
    RecycleBin5Enum_RecycleBinEnumList_Skip,
    RecycleBin5Enum_RecycleBinEnumList_Reset,
};

HRESULT
RecycleBin5Enum_Constructor(
    IN IRecycleBin5 *prb,
    IN HANDLE hInfo,
    IN HANDLE hInfoMapped,
    IN LPCWSTR szPrefix,
    OUT IUnknown **ppUnknown)
{
    struct RecycleBin5Enum *s = NULL;
    SIZE_T Needed;

    if (!ppUnknown)
        return E_POINTER;

    Needed = (wcslen(szPrefix) + 1) * sizeof(WCHAR);

    s = CoTaskMemAlloc(sizeof(struct RecycleBin5Enum) + Needed);
    if (!s)
        return E_OUTOFMEMORY;
    ZeroMemory(s, sizeof(struct RecycleBin5Enum) + Needed);
    s->recycleBinEnumImpl.lpVtbl = &RecycleBin5EnumVtbl;
    s->ref = 1;
    s->recycleBin = prb;
    wcscpy(s->szPrefix, szPrefix);
    s->hInfo = hInfo;
    s->pInfo = MapViewOfFile(hInfoMapped, FILE_MAP_READ, 0, 0, 0);
    if (!s->pInfo)
    {
        CoTaskMemFree(s);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (s->pInfo->dwVersion != 5 || s->pInfo->dwRecordSize != sizeof(DELETED_FILE_RECORD))
    {
        UnmapViewOfFile(s->pInfo);
        CoTaskMemFree(s);
        return E_FAIL;
    }
    IRecycleBin5_AddRef(s->recycleBin);
    *ppUnknown = (IUnknown *)&s->recycleBinEnumImpl;

    return S_OK;
}
