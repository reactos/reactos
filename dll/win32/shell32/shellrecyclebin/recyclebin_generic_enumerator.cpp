/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Enumerates contents of all recycle bins
 * COPYRIGHT:   Copyright 2007 Hervé Poussineau (hpoussin@reactos.org)
 *              Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "recyclebin_private.h"

class RecycleBinGenericEnum : public IRecycleBinEnumList
{
public:
    RecycleBinGenericEnum();
    virtual ~RecycleBinGenericEnum();

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
    IRecycleBinEnumList *m_current;
    DWORD m_dwLogicalDrives;
    SIZE_T m_skip;
};

STDMETHODIMP
RecycleBinGenericEnum::QueryInterface(REFIID riid, void **ppvObject)
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

STDMETHODIMP_(ULONG)
RecycleBinGenericEnum::AddRef()
{
    ULONG refCount = InterlockedIncrement(&m_ref);
    TRACE("(%p)\n", this);
    return refCount;
}

RecycleBinGenericEnum::~RecycleBinGenericEnum()
{
    TRACE("(%p)\n", this);

    if (m_current)
        m_current->Release();
}

STDMETHODIMP_(ULONG)
RecycleBinGenericEnum::Release()
{
    TRACE("(%p)\n", this);

    ULONG refCount = InterlockedDecrement(&m_ref);
    if (refCount == 0)
        delete this;
    return refCount;
}

STDMETHODIMP
RecycleBinGenericEnum::Next(DWORD celt, IRecycleBinFile **rgelt, DWORD *pceltFetched)
{
    TRACE("(%p, %u, %p, %p)\n", this, celt, rgelt, pceltFetched);

    if (!rgelt)
        return E_POINTER;
    if (!pceltFetched && celt > 1)
        return E_INVALIDARG;

    HRESULT hr;
    DWORD fetched = 0;
    while (TRUE)
    {
        /* Get enumerator implementation */
        if (!m_current && m_dwLogicalDrives)
        {
            for (DWORD i = 0; i < L'Z' - L'A' + 1; ++i)
            {
                if (m_dwLogicalDrives & (1 << i))
                {
                    WCHAR szVolumeName[4];
                    szVolumeName[0] = (WCHAR)(L'A' + i);
                    szVolumeName[1] = L':';
                    szVolumeName[2] = L'\\';
                    szVolumeName[3] = UNICODE_NULL;
                    if (GetDriveTypeW(szVolumeName) != DRIVE_FIXED)
                    {
                        m_dwLogicalDrives &= ~(1 << i);
                        continue;
                    }

                    IRecycleBin *prb;
                    hr = GetDefaultRecycleBin(szVolumeName, &prb);
                    if (!SUCCEEDED(hr))
                        return hr;
                    hr = prb->EnumObjects(&m_current);
                    prb->Release();

                    if (!SUCCEEDED(hr))
                        return hr;

                    m_dwLogicalDrives &= ~(1 << i);
                    break;
                }
            }
        }

        if (!m_current)
        {
            /* Nothing more to enumerate */
            if (pceltFetched)
                *pceltFetched = fetched;
            return S_FALSE;
        }

        /* Skip some elements */
        while (m_skip > 0)
        {
            IRecycleBinFile *rbf;
            hr = m_current->Next(1, &rbf, NULL);
            if (hr == S_OK)
                rbf->Release();
            else if (hr == S_FALSE)
                break;
            else if (!SUCCEEDED(hr))
                return hr;
        }

        if (m_skip > 0)
            continue;

        /* Fill area */
        DWORD newFetched;
        hr = m_current->Next(celt - fetched, &rgelt[fetched], &newFetched);
        if (SUCCEEDED(hr))
            fetched += newFetched;

        if (hr == S_FALSE || newFetched == 0)
        {
            m_current->Release();
            m_current = NULL;
        }
        else if (!SUCCEEDED(hr))
            return hr;

        if (fetched == celt)
        {
            if (pceltFetched)
                *pceltFetched = fetched;
            return S_OK;
        }
    }

    /* Never go here */
    UNREACHABLE;
}

STDMETHODIMP RecycleBinGenericEnum::Skip(DWORD celt)
{
    TRACE("(%p, %u)\n", this, celt);
    m_skip += celt;
    return S_OK;
}

STDMETHODIMP RecycleBinGenericEnum::Reset()
{
    TRACE("(%p)\n", this);

    if (m_current)
    {
        m_current->Release();
        m_current = NULL;
    }
    m_skip = 0;
    m_dwLogicalDrives = ::GetLogicalDrives();
    return S_OK;
}

RecycleBinGenericEnum::RecycleBinGenericEnum()
    : m_ref(1)
    , m_current(NULL)
    , m_dwLogicalDrives(0)
    , m_skip(0)
{
}

EXTERN_C
HRESULT
RecycleBinGenericEnum_Constructor(
    OUT IRecycleBinEnumList **pprbel)
{
    RecycleBinGenericEnum *pThis = new RecycleBinGenericEnum();
    if (!pThis)
        return E_OUTOFMEMORY;

    *pprbel = static_cast<IRecycleBinEnumList *>(pThis);
    return (*pprbel)->Reset();
}
