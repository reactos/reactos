/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CEnumZipContents
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

class CEnumZipContents :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
private:
    CZipEnumerator mEnumerator;
    DWORD dwFlags;
    CStringW m_Prefix;
public:
    CEnumZipContents()
        :dwFlags(0)
    {
    }

    STDMETHODIMP Initialize(IZip* zip, DWORD flags, PCWSTR prefix)
    {
        dwFlags = flags;
        m_Prefix = prefix;
        if (mEnumerator.initialize(zip))
            return S_OK;
        return E_FAIL;
    }

    // *** IEnumIDList methods ***
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
    {
        if (!rgelt || (!pceltFetched && celt != 1))
            return E_POINTER;

        HRESULT hr = S_OK;
        ULONG fetched = 0;
        LPITEMIDLIST item;
        CStringW name;
        bool dir;
        unz_file_info64 info;

        while (fetched < celt)
        {
            if (mEnumerator.next_unique(m_Prefix, name, dir, info))
            {
                item = _ILCreate(dir ? ZIP_PIDL_DIRECTORY : ZIP_PIDL_FILE, name, info);
                if (!item)
                {
                    hr = fetched ? S_FALSE : E_OUTOFMEMORY;
                    break;
                }
                rgelt[fetched++] = item;
            }
            else
            {
                hr = S_FALSE;
                break;
            }
        }

        if (pceltFetched)
            *pceltFetched = fetched;
        return hr;
    }
    STDMETHODIMP Skip(ULONG celt)
    {
        CStringW name;
        bool dir;
        unz_file_info64 info;
        while (celt--)
        {
            if (!mEnumerator.next_unique(m_Prefix, name, dir, info))
                return E_FAIL;
            ;
        }
        return S_OK;
    }
    STDMETHODIMP Reset()
    {
        if (mEnumerator.reset())
            return S_OK;
        return E_FAIL;
    }
    STDMETHODIMP Clone(IEnumIDList **ppenum)
    {
        return E_NOTIMPL;
    }


public:
    DECLARE_NOT_AGGREGATABLE(CEnumZipContents)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CEnumZipContents)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()
};


HRESULT _CEnumZipContents_CreateInstance(IZip* zip, DWORD flags, PCWSTR prefix, REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreatorInit<CEnumZipContents>(zip, flags, prefix, riid, ppvOut);
}

