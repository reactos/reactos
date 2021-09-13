/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CEnumZipContents
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

class CEnumZipContents :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
private:
    CZipEnumerator mEnumerator;
    DWORD dwFlags;
    CStringA m_Prefix;
public:
    CEnumZipContents()
        :dwFlags(0)
    {
    }

    STDMETHODIMP Initialize(IZip* zip, DWORD flags, const char* prefix)
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
        if (!pceltFetched || !rgelt)
            return E_POINTER;

        *pceltFetched = 0;

        if (celt != 1)
            return E_FAIL;

        CStringA name;
        bool dir;
        unz_file_info64 info;
        if (mEnumerator.next_unique(m_Prefix, name, dir, info))
        {
            *pceltFetched = 1;
            *rgelt = _ILCreate(dir ? ZIP_PIDL_DIRECTORY : ZIP_PIDL_FILE, name, info);
            return S_OK;
        }

        return S_FALSE;
    }
    STDMETHODIMP Skip(ULONG celt)
    {
        CStringA name;
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


HRESULT _CEnumZipContents_CreateInstance(IZip* zip, DWORD flags, const char* prefix, REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreatorInit<CEnumZipContents>(zip, flags, prefix, riid, ppvOut);
}

