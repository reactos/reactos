/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/recyclebin_generic_enumerator.c
 * PURPOSE:     Enumerates contents of all recycle bins
 * PROGRAMMERS: Copyright 2007 Hervé Poussineau (hpoussin@reactos.org)
 *              Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "recyclebin_private.h"

struct RecycleBinGenericEnum
{
    LONG m_ref;
    IRecycleBinEnumList *m_current;
    DWORD m_dwLogicalDrives;
    SIZE_T m_skip;
};

class CRecycleBinGenericEnum
    : public IRecycleBinEnumList
    , public RecycleBinGenericEnum
{
public:
    CRecycleBinGenericEnum();
    virtual ~CRecycleBinGenericEnum();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IRecycleBinEnumList methods
    STDMETHODIMP Next(DWORD celt, IRecycleBinFile **rgelt, DWORD *pceltFetched) override;
    STDMETHODIMP Skip(DWORD celt) override;
    STDMETHODIMP Reset() override;
};

STDMETHODIMP
CRecycleBinGenericEnum::QueryInterface(
    IN REFIID riid,
    OUT void **ppvObject)
{
    TRACE("(%p, %s, %p)\n", this, debugstr_guid(&riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = static_cast<IRecycleBinEnumList*>(this);
    else if (IsEqualIID(riid, IID_IRecycleBinEnumList))
        *ppvObject = static_cast<IRecycleBinEnumList*>(this);
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CRecycleBinGenericEnum::AddRef()
{
    ULONG refCount = InterlockedIncrement(&m_ref);
    TRACE("(%p)\n", this);
    return refCount;
}

CRecycleBinGenericEnum::~CRecycleBinGenericEnum()
{
    TRACE("(%p)\n", this);

    if (m_current)
        m_current->Release();
}

STDMETHODIMP_(ULONG)
CRecycleBinGenericEnum::Release()
{
    ULONG refCount;

    TRACE("(%p)\n", this);

    refCount = InterlockedDecrement(&m_ref);

    if (refCount == 0)
        delete this;

    return refCount;
}

STDMETHODIMP
CRecycleBinGenericEnum::Next(
    IN DWORD celt,
    IN OUT IRecycleBinFile **rgelt,
    OUT DWORD *pceltFetched)
{
    IRecycleBin *prb;
    DWORD i;
    DWORD fetched = 0, newFetched;
    HRESULT hr;

    TRACE("(%p, %u, %p, %p)\n", this, celt, rgelt, pceltFetched);

    if (!rgelt)
        return E_POINTER;
    if (!pceltFetched && celt > 1)
        return E_INVALIDARG;

    while (TRUE)
    {
        /* Get enumerator implementation */
        if (!m_current && m_dwLogicalDrives)
        {
            for (i = 0; i < 26; i++)
                if (m_dwLogicalDrives & (1 << i))
                {
                    WCHAR szVolumeName[4];
                    szVolumeName[0] = (WCHAR)('A' + i);
                    szVolumeName[1] = ':';
                    szVolumeName[2] = '\\';
                    szVolumeName[3] = UNICODE_NULL;
                    if (GetDriveTypeW(szVolumeName) != DRIVE_FIXED)
                    {
                        m_dwLogicalDrives &= ~(1 << i);
                        continue;
                    }
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
                hr = rbf->Release();
            else if (hr == S_FALSE)
                break;
            else if (!SUCCEEDED(hr))
                return hr;
        }
        if (m_skip > 0)
            continue;

        /* Fill area */
        hr = m_current->Next(celt - fetched, &rgelt[fetched], &newFetched);
        if (SUCCEEDED(hr))
            fetched += newFetched;
        if (hr == S_FALSE || newFetched == 0)
        {
            hr = m_current->Release();
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
}

STDMETHODIMP
CRecycleBinGenericEnum::Skip(
    IN DWORD celt)
{
    TRACE("(%p, %u)\n", this, celt);
    m_skip += celt;
    return S_OK;
}

STDMETHODIMP
CRecycleBinGenericEnum::Reset()
{
    TRACE("(%p)\n", this);

    if (m_current)
    {
        m_current->Release();
        m_current = NULL;
        m_skip = 0;
    }
    m_dwLogicalDrives = GetLogicalDrives();
    return S_OK;
}

CRecycleBinGenericEnum::CRecycleBinGenericEnum()
{
    ZeroMemory(&m_ref, sizeof(RecycleBinGenericEnum));
    m_ref = 1;
}

HRESULT
RecycleBinGenericEnum_Constructor(
    OUT IRecycleBinEnumList **pprbel)
{
    CRecycleBinGenericEnum *s = new CRecycleBinGenericEnum();
    *pprbel = static_cast<IRecycleBinEnumList*>(s);
    return s->Reset();
}
