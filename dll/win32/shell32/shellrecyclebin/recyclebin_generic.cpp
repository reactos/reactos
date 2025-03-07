/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Deals with a system-wide recycle bin
 * COPYRIGHT:   Copyright 2007 Hervé Poussineau (hpoussin@reactos.org)
 *              Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "recyclebin_private.h"

class RecycleBinGeneric : public IRecycleBin
{
public:
    RecycleBinGeneric();
    virtual ~RecycleBinGeneric();

    /* IUnknown methods */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    /* IRecycleBin methods */
    STDMETHODIMP DeleteFile(LPCWSTR szFileName) override;
    STDMETHODIMP EmptyRecycleBin() override;
    STDMETHODIMP EnumObjects(IRecycleBinEnumList **ppEnumList) override;
    STDMETHODIMP GetDirectory(LPWSTR szPath) override
    {
        return E_UNEXPECTED;
    }

protected:
    LONG m_ref;
};

STDMETHODIMP RecycleBinGeneric::QueryInterface(REFIID riid, void **ppvObject)
{
    TRACE("(%p, %s, %p)\n", this, debugstr_guid(&riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IRecycleBin))
        *ppvObject = static_cast<IRecycleBin *>(this);
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) RecycleBinGeneric::AddRef()
{
    ULONG refCount = InterlockedIncrement(&m_ref);
    TRACE("(%p)\n", this);
    return refCount;
}

RecycleBinGeneric::~RecycleBinGeneric()
{
    TRACE("(%p)\n", this);
}

STDMETHODIMP_(ULONG) RecycleBinGeneric::Release()
{
    TRACE("(%p)\n", this);

    ULONG refCount = InterlockedDecrement(&m_ref);
    if (refCount == 0)
        delete this;
    return refCount;
}

STDMETHODIMP RecycleBinGeneric::DeleteFile(LPCWSTR szFileName)
{
    TRACE("(%p, %s)\n", this, debugstr_w(szFileName));

    /* Get full file name */
    LPWSTR szFullName = NULL;
    DWORD dwBufferLength = 0;
    while (TRUE)
    {
        DWORD len = GetFullPathNameW(szFileName, dwBufferLength, szFullName, NULL);
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

    /* Get associated volume path */
    WCHAR szVolume[MAX_PATH];
#ifndef __REACTOS__
    if (!GetVolumePathNameW(szFullName, szVolume, _countof(szVolume)))
    {
        CoTaskMemFree(szFullName);
        return HRESULT_FROM_WIN32(GetLastError());
    }
#else
    swprintf(szVolume, L"%c:\\", szFullName[0]);
#endif

    /* Skip namespace (if any): "\\.\" or "\\?\" */
    if (szVolume[0] == '\\' &&
        szVolume[1] == '\\' &&
        (szVolume[2] == '.' || szVolume[2] == '?') &&
        szVolume[3] == '\\')
    {
        MoveMemory(szVolume, &szVolume[4], (_countof(szVolume) - 4) * sizeof(WCHAR));
    }

    IRecycleBin *prb;
    HRESULT hr = GetDefaultRecycleBin(szVolume, &prb);
    if (!SUCCEEDED(hr))
    {
        CoTaskMemFree(szFullName);
        return hr;
    }

    hr = prb->DeleteFile(szFullName);
    CoTaskMemFree(szFullName);
    prb->Release();
    return hr;
}

STDMETHODIMP RecycleBinGeneric::EmptyRecycleBin()
{
    TRACE("(%p)\n", this);

    DWORD dwLogicalDrives = GetLogicalDrives();
    if (dwLogicalDrives == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    for (DWORD i = 0; i < 'Z' - 'A' + 1; i++)
    {
        if (!(dwLogicalDrives & (1 << i)))
            continue;

        WCHAR szVolumeName[MAX_PATH];
        swprintf(szVolumeName, L"%c:\\", L'A' + i);
        if (GetDriveTypeW(szVolumeName) != DRIVE_FIXED)
            continue;

        IRecycleBin *prb;
        HRESULT hr = GetDefaultRecycleBin(szVolumeName, &prb);
        if (!SUCCEEDED(hr))
            return hr;

        hr = prb->EmptyRecycleBin();
        prb->Release();
    }

    return S_OK;
}

STDMETHODIMP RecycleBinGeneric::EnumObjects(IRecycleBinEnumList **ppEnumList)
{
    TRACE("(%p, %p)\n", this, ppEnumList);
    return RecycleBinGenericEnum_Constructor(ppEnumList);
}

RecycleBinGeneric::RecycleBinGeneric()
    : m_ref(1)
{
}

EXTERN_C
HRESULT RecycleBinGeneric_Constructor(OUT IUnknown **ppUnknown)
{
    /* This RecycleBin implementation was introduced to be able to manage all
     * drives at once, and instanciate the 'real' implementations when needed */
    RecycleBinGeneric *pThis = new RecycleBinGeneric();
    if (!pThis)
        return E_OUTOFMEMORY;

    *ppUnknown = static_cast<IRecycleBin *>(pThis);
    return S_OK;
}

EXTERN_C
BOOL RecycleBinGeneric_IsEqualFileIdentity(const RECYCLEBINFILEIDENTITY *p1, const RECYCLEBINFILEIDENTITY *p2)
{
    return p1->DeletionTime.dwLowDateTime == p2->DeletionTime.dwLowDateTime &&
           p1->DeletionTime.dwHighDateTime == p2->DeletionTime.dwHighDateTime &&
           _wcsicmp(p1->RecycledFullPath, p2->RecycledFullPath) == 0;
}
