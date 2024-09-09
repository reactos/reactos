/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Enumerates contents of a MS Windows 2000/XP/2003 recyclebin
 * COPYRIGHT:   Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 *              Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "recyclebin_private.h"
#include <atlstr.h>
#include <shlwapi.h>
#include <strsafe.h>

class RecycleBin5File : public IRecycleBinFile
{
public:
    RecycleBin5File();
    virtual ~RecycleBin5File();

    HRESULT Init(
        _In_ IRecycleBin5 *prb,
        _In_ LPCWSTR Folder,
        _In_ PDELETED_FILE_RECORD pDeletedFile);

    /* IUnknown methods */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    /* IRecycleBinFile methods */
    STDMETHODIMP GetLastModificationTime(FILETIME *pLastModificationTime) override;
    STDMETHODIMP GetDeletionTime(FILETIME *pDeletionTime) override;
    STDMETHODIMP GetFileSize(ULARGE_INTEGER *pFileSize) override;
    STDMETHODIMP GetPhysicalFileSize(ULARGE_INTEGER *pPhysicalFileSize) override;
    STDMETHODIMP GetAttributes(DWORD *pAttributes) override;
    STDMETHODIMP GetFileName(SIZE_T BufferSize, LPWSTR Buffer, SIZE_T *RequiredSize) override;
    STDMETHODIMP GetTypeName(SIZE_T BufferSize, LPWSTR Buffer, SIZE_T *RequiredSize) override;
    STDMETHODIMP Delete() override;
    STDMETHODIMP Restore() override;

protected:
    LONG m_ref;
    IRecycleBin5 *m_recycleBin;
    DELETED_FILE_RECORD m_deletedFile;
    LPWSTR m_FullName;
};

STDMETHODIMP RecycleBin5File::QueryInterface(REFIID riid, void **ppvObject)
{
    TRACE("(%p, %s, %p)\n", this, debugstr_guid(&riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IRecycleBinFile))
        *ppvObject = static_cast<IRecycleBinFile *>(this);
    else if (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW))
    {
        DWORD dwAttributes;
        if (GetAttributes(&dwAttributes) == S_OK)
            return SHCreateFileExtractIconW(m_FullName, dwAttributes, riid, ppvObject);
        else
            return S_FALSE;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) RecycleBin5File::AddRef()
{
    TRACE("(%p)\n", this);
    return InterlockedIncrement(&m_ref);
}

RecycleBin5File::~RecycleBin5File()
{
    TRACE("(%p)\n", this);
    m_recycleBin->Release();
    SHFree(m_FullName);
}

STDMETHODIMP_(ULONG) RecycleBin5File::Release()
{
    TRACE("(%p)\n", this);
    ULONG refCount = InterlockedDecrement(&m_ref);
    if (refCount == 0)
        delete this;
    return refCount;
}

STDMETHODIMP RecycleBin5File::GetLastModificationTime(FILETIME *pLastModificationTime)
{
    TRACE("(%p, %p)\n", this, pLastModificationTime);

    DWORD dwAttributes = ::GetFileAttributesW(m_FullName);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());

    HANDLE hFile;
    hFile = CreateFileW(m_FullName,
                        GENERIC_READ,
                        FILE_SHARE_READ |
                            ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) ? (FILE_SHARE_WRITE | FILE_SHARE_DELETE) : 0),
                        NULL,
                        OPEN_EXISTING,
                        (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_FLAG_BACKUP_SEMANTICS : 0,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    HRESULT hr;
    if (GetFileTime(hFile, NULL, NULL, pLastModificationTime))
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(hFile);
    return hr;
}

STDMETHODIMP RecycleBin5File::GetDeletionTime(FILETIME *pDeletionTime)
{
    TRACE("(%p, %p)\n", this, pDeletionTime);
    *pDeletionTime = m_deletedFile.DeletionTime;
    return S_OK;
}

STDMETHODIMP RecycleBin5File::GetFileSize(ULARGE_INTEGER *pFileSize)
{
    TRACE("(%p, %p)\n", this, pFileSize);

    DWORD dwAttributes = GetFileAttributesW(m_FullName);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        pFileSize->QuadPart = 0;
        return S_OK;
    }

    HANDLE hFile = CreateFileW(m_FullName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());
    pFileSize->u.LowPart = ::GetFileSize(hFile, &pFileSize->u.HighPart);

    HRESULT hr;
    if (pFileSize->u.LowPart != INVALID_FILE_SIZE)
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    CloseHandle(hFile);
    return hr;
}

STDMETHODIMP RecycleBin5File::GetPhysicalFileSize(ULARGE_INTEGER *pPhysicalFileSize)
{
    TRACE("(%p, %p)\n", this, pPhysicalFileSize);
    pPhysicalFileSize->u.HighPart = 0;
    pPhysicalFileSize->u.LowPart = m_deletedFile.dwPhysicalFileSize;
    return S_OK;
}

STDMETHODIMP RecycleBin5File::GetAttributes(DWORD *pAttributes)
{
    DWORD dwAttributes;

    TRACE("(%p, %p)\n", this, pAttributes);

    dwAttributes = GetFileAttributesW(m_FullName);
    if (dwAttributes == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());

    *pAttributes = dwAttributes;
    return S_OK;
}

STDMETHODIMP RecycleBin5File::GetFileName(SIZE_T BufferSize, LPWSTR Buffer, SIZE_T *RequiredSize)
{
    DWORD dwRequired;

    TRACE("(%p, %u, %p, %p)\n", this, BufferSize, Buffer, RequiredSize);

    dwRequired = (DWORD)(wcslen(m_deletedFile.FileNameW) + 1) * sizeof(WCHAR);
    if (RequiredSize)
        *RequiredSize = dwRequired;

    if (BufferSize == 0 && !Buffer)
        return S_OK;

    if (BufferSize < dwRequired)
        return E_OUTOFMEMORY;
    CopyMemory(Buffer, m_deletedFile.FileNameW, dwRequired);
    return S_OK;
}

STDMETHODIMP RecycleBin5File::GetTypeName(SIZE_T BufferSize, LPWSTR Buffer, SIZE_T *RequiredSize)
{
    HRESULT hr;
    DWORD dwRequired;
    DWORD dwAttributes;
    SHFILEINFOW shFileInfo;

    TRACE("(%p, %u, %p, %p)\n", this, BufferSize, Buffer, RequiredSize);

    hr = GetAttributes(&dwAttributes);
    if (!SUCCEEDED(hr))
        return hr;

    hr = SHGetFileInfoW(m_FullName, dwAttributes, &shFileInfo, sizeof(shFileInfo), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
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

STDMETHODIMP RecycleBin5File::Delete()
{
    TRACE("(%p)\n", this);
    return m_recycleBin->Delete(m_FullName, &m_deletedFile);
}

STDMETHODIMP RecycleBin5File::Restore()
{
    TRACE("(%p)\n", this);
    return m_recycleBin->Restore(m_FullName, &m_deletedFile);
}

RecycleBin5File::RecycleBin5File()
    : m_ref(1)
    , m_recycleBin(NULL)
    , m_FullName(NULL)
{
    ZeroMemory(&m_deletedFile, sizeof(m_deletedFile));
}

HRESULT
RecycleBin5File::Init(
    _In_ IRecycleBin5 *prb,
    _In_ LPCWSTR Folder,
    _In_ PDELETED_FILE_RECORD pDeletedFile)
{
    m_recycleBin = prb;
    m_recycleBin->AddRef();
    m_deletedFile = *pDeletedFile;

    WCHAR szUniqueId[32];
    StringCchPrintfW(szUniqueId, _countof(szUniqueId), L"%lu", pDeletedFile->dwRecordUniqueId);

    CStringW strFullName(Folder);
    strFullName += L"\\D";
    strFullName += (WCHAR)(L'a' + pDeletedFile->dwDriveNumber);
    strFullName += szUniqueId;
    strFullName += PathFindExtensionW(pDeletedFile->FileNameW);
    if (GetFileAttributesW(strFullName) == INVALID_FILE_ATTRIBUTES)
        return E_FAIL;

    return SHStrDup(strFullName, &m_FullName);
}

static HRESULT
RecycleBin5File_Constructor(
    _In_ IRecycleBin5 *prb,
    _In_ LPCWSTR Folder,
    _In_ PDELETED_FILE_RECORD pDeletedFile,
    _Out_ IRecycleBinFile **ppFile)
{
    if (!ppFile)
        return E_POINTER;

    *ppFile = NULL;

    RecycleBin5File *pThis = new RecycleBin5File();
    if (!pThis)
        return E_OUTOFMEMORY;

    HRESULT hr = pThis->Init(prb, Folder, pDeletedFile);
    if (FAILED(hr))
    {
        delete pThis;
        return hr;
    }

    *ppFile = static_cast<IRecycleBinFile *>(pThis);
    return S_OK;
}

class RecycleBin5Enum : public IRecycleBinEnumList
{
public:
    RecycleBin5Enum();
    virtual ~RecycleBin5Enum();

    HRESULT Init(
        _In_ IRecycleBin5 *prb,
        _In_ HANDLE hInfo,
        _In_ HANDLE hInfoMapped,
        _In_ LPCWSTR pszPrefix);

    /* IUnknown methods */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    /* IRecycleBinEnumList methods */
    STDMETHODIMP Next(DWORD celt, IRecycleBinFile **rgelt, DWORD *pceltFetched) override;
    STDMETHODIMP Skip(DWORD celt) override;
    STDMETHODIMP Reset() override;

protected:
    LONG m_ref;
    IRecycleBin5 *m_recycleBin;
    HANDLE m_hInfo;
    INFO2_HEADER *m_pInfo;
    DWORD m_dwCurrent;
    LPWSTR m_pszPrefix;
};

STDMETHODIMP RecycleBin5Enum::QueryInterface(REFIID riid, void **ppvObject)
{
    TRACE("(%p, %s, %p)\n", this, debugstr_guid(&riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IRecycleBinEnumList))
        *ppvObject = static_cast<IRecycleBinEnumList *>(this);
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) RecycleBin5Enum::AddRef()
{
    TRACE("(%p)\n", this);
    return InterlockedIncrement(&m_ref);
}

RecycleBin5Enum::~RecycleBin5Enum()
{
    TRACE("(%p)\n", this);

    m_recycleBin->OnClosing(this);
    UnmapViewOfFile(m_pInfo);
    m_recycleBin->Release();
    SHFree(m_pszPrefix);
}

STDMETHODIMP_(ULONG) RecycleBin5Enum::Release()
{
    TRACE("(%p)\n", this);

    ULONG refCount = InterlockedDecrement(&m_ref);
    if (refCount == 0)
        delete this;
    return refCount;
}

STDMETHODIMP RecycleBin5Enum::Next(DWORD celt, IRecycleBinFile **rgelt, DWORD *pceltFetched)
{
    HRESULT hr;

    TRACE("(%p, %u, %p, %p)\n", this, celt, rgelt, pceltFetched);

    if (!rgelt)
        return E_POINTER;
    if (!pceltFetched && celt > 1)
        return E_INVALIDARG;

    ULARGE_INTEGER FileSize;
    FileSize.u.LowPart = GetFileSize(m_hInfo, &FileSize.u.HighPart);
    if (FileSize.u.LowPart == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwEntries =
        (DWORD)((FileSize.QuadPart - sizeof(INFO2_HEADER)) / sizeof(DELETED_FILE_RECORD));

    DWORD iEntry = m_dwCurrent, fetched = 0;
    PDELETED_FILE_RECORD pDeletedFile = (PDELETED_FILE_RECORD)(m_pInfo + 1) + iEntry;
    for (; iEntry < dwEntries && fetched < celt; ++iEntry)
    {
        hr = RecycleBin5File_Constructor(m_recycleBin, m_pszPrefix, pDeletedFile, &rgelt[fetched]);
        if (SUCCEEDED(hr))
            fetched++;
        pDeletedFile++;
    }

    m_dwCurrent = iEntry;
    if (pceltFetched)
        *pceltFetched = fetched;
    if (fetched == celt)
        return S_OK;
    else
        return S_FALSE;
}

STDMETHODIMP RecycleBin5Enum::Skip(DWORD celt)
{
    TRACE("(%p, %u)\n", this, celt);
    m_dwCurrent += celt;
    return S_OK;
}

STDMETHODIMP RecycleBin5Enum::Reset()
{
    TRACE("(%p)\n", this);
    m_dwCurrent = 0;
    return S_OK;
}

RecycleBin5Enum::RecycleBin5Enum()
    : m_ref(1)
    , m_recycleBin(NULL)
    , m_hInfo(NULL)
    , m_pInfo(NULL)
    , m_dwCurrent(0)
    , m_pszPrefix(NULL)
{
}

HRESULT
RecycleBin5Enum::Init(
    _In_ IRecycleBin5 *prb,
    _In_ HANDLE hInfo,
    _In_ HANDLE hInfoMapped,
    _In_ LPCWSTR pszPrefix)
{
    m_recycleBin = prb;
    m_recycleBin->AddRef();

    HRESULT hr = SHStrDup(pszPrefix, &m_pszPrefix);
    if (FAILED(hr))
        return hr;

    m_hInfo = hInfo;
    m_pInfo = (PINFO2_HEADER)MapViewOfFile(hInfoMapped, FILE_MAP_READ, 0, 0, 0);
    if (!m_pInfo)
        return HRESULT_FROM_WIN32(GetLastError());

    if (m_pInfo->dwVersion != 5 || m_pInfo->dwRecordSize != sizeof(DELETED_FILE_RECORD))
        return E_FAIL;

    return S_OK;
}

EXTERN_C
HRESULT
RecycleBin5Enum_Constructor(
    _In_ IRecycleBin5 *prb,
    _In_ HANDLE hInfo,
    _In_ HANDLE hInfoMapped,
    _In_ LPCWSTR szPrefix,
    _Out_ IUnknown **ppUnknown)
{
    if (!ppUnknown)
        return E_POINTER;

    *ppUnknown = NULL;

    RecycleBin5Enum *pThis = new RecycleBin5Enum();
    if (!pThis)
        return E_OUTOFMEMORY;

    HRESULT hr = pThis->Init(prb, hInfo, hInfoMapped, szPrefix);
    if (FAILED(hr))
    {
        delete pThis;
        return hr;
    }

    *ppUnknown = static_cast<IRecycleBinEnumList *>(pThis);
    return S_OK;
}
