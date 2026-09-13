/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CEnumZipContents
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

static inline bool IncludeInEnumIDList(SHCONTF contf, SFGAOF att) // Borrowed from cabview
{
    const SHCONTF both = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
    const SFGAOF superbits = SFGAO_HIDDEN | SFGAO_READONLY | SFGAO_SYSTEM;
    const bool isfile = (att & (SFGAO_STREAM | SFGAO_FOLDER)) != SFGAO_FOLDER;
    if ((contf & both) != both && !(contf & SHCONTF_STORAGE))
    {
        if (isfile && (contf & SHCONTF_FOLDERS))
            return false;
        if ((att & SFGAO_FOLDER) && (contf & SHCONTF_NONFOLDERS))
            return false;
    }
    if ((att & SFGAO_HIDDEN) && !(contf & (SHCONTF_INCLUDEHIDDEN | SHCONTF_STORAGE)))
        return false;
    if ((att & superbits) > SFGAO_HIDDEN && !(contf & (SHCONTF_INCLUDESUPERHIDDEN | SHCONTF_STORAGE)))
        return false;
    return true;
}

static inline bool IncludeInEnumIDList(SHCONTF contf, bool dir, unz_file_info64 &zfi)
{
    SFGAOF att = dir ? SFGAO_FOLDER : SFGAO_STREAM;
    // TODO: SFGAO_HIDDEN,READONLY,SYSTEM from zfi.external_fa? (Use MapFSToSFAttributes)
    return IncludeInEnumIDList(contf, att);
}

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
        if (mEnumerator.Initialize(zip))
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
            if (mEnumerator.NextUnique(m_Prefix, name, dir, info))
            {
                if (!IncludeInEnumIDList(dwFlags, dir, info))
                    continue;

                item = _ILCreateZipItem(dir ? ZIP_PIDL_DIRECTORY : ZIP_PIDL_FILE, name, info);
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
            if (!mEnumerator.NextUnique(m_Prefix, name, dir, info))
                return E_FAIL;
            ;
        }
        return S_OK;
    }
    STDMETHODIMP Reset()
    {
        if (mEnumerator.Reset())
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

