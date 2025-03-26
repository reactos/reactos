/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CEnumFonts implementation
 * COPYRIGHT:   Copyright 2019 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(fontext);



class CEnumFonts :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
private:
    DWORD m_dwFlags;
    ULONG m_Index;

public:
    CEnumFonts()
        :m_dwFlags(0)
        ,m_Index(0)
    {
    }

    STDMETHODIMP Initialize(CFontExt* folder, DWORD flags)
    {
        m_dwFlags = flags;
        m_Index = 0;
        return S_OK;
    }

    // *** IEnumIDList methods ***
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
    {
        if (!rgelt || (!pceltFetched && celt != 1))
            return E_POINTER;

        HRESULT hr = S_OK;
        ULONG Fetched = 0;

        while (celt)
        {
            celt--;

            if (m_Index < g_FontCache->Size())
            {
                CStringW Name = g_FontCache->Name(m_Index);
                LPITEMIDLIST item = _ILCreate(Name, m_Index);
                if (!item)
                {
                    hr = Fetched ? S_FALSE : E_OUTOFMEMORY;
                    break;
                }
                rgelt[Fetched] = item;
                m_Index++;
                Fetched++;
            }
            else
            {
                hr = S_FALSE;
            }
        }

        if (pceltFetched)
            *pceltFetched = Fetched;
        return hr;
    }
    STDMETHODIMP Skip(ULONG celt)
    {
        m_Index += celt;
        return S_OK;
    }
    STDMETHODIMP Reset()
    {
        m_Index = 0;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumIDList **ppenum)
    {
        return E_NOTIMPL;
    }


public:
    DECLARE_NOT_AGGREGATABLE(CEnumFonts)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CEnumFonts)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()
};


HRESULT _CEnumFonts_CreateInstance(CFontExt* zip, DWORD flags, REFIID riid, LPVOID * ppvOut)
{
    return ShellObjectCreatorInit<CEnumFonts>(zip, flags, riid, ppvOut);
}

