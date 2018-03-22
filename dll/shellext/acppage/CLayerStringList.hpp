/*
 * PROJECT:     ReactOS Compatibility Layer Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CLayerStringList implementation
 * COPYRIGHT:   Copyright 2015-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

/* TODO: Use HSDB instead of PDB */
class CLayerStringList :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString
{
public:
    CLayerStringList()
        :m_root(TAGID_NULL), m_layer(TAGID_NULL)
    {
        WCHAR buf[MAX_PATH];
        SdbGetAppPatchDir(NULL, buf, MAX_PATH);
        StringCchCatW(buf, _countof(buf), L"\\sysmain.sdb");
        m_db = SdbOpenDatabase(buf, DOS_PATH);
        Reset();
    }

    ~CLayerStringList()
    {
        SdbCloseDatabase(m_db);
    }

    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
    {
        if (pceltFetched)
            *pceltFetched = 0;

        while (celt && m_layer)
        {
            TAGID nameid = SdbFindFirstTag(m_db, m_layer, TAG_NAME);
            if (nameid)
            {
                LPWSTR name = SdbGetStringTagPtr(m_db, nameid);
                if (name && !IsBuiltinLayer(name))
                {
                    ULONG Size = wcslen(name) + 1;

                    *rgelt = (LPOLESTR)::CoTaskMemAlloc(Size * sizeof(WCHAR));
                    StringCchCopyW(*rgelt, Size, name);

                    if (pceltFetched)
                        (*pceltFetched)++;

                    celt--;
                    rgelt++;
                }
            }
            m_layer = SdbFindNextTag(m_db, m_root, m_layer);
        }
        return celt ? S_FALSE : S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt)
    {
        while (m_layer && celt)
        {
            m_layer = SdbFindNextTag(m_db, m_root, m_layer);
            --celt;
        }
        return celt ? S_FALSE : S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Reset()
    {
        m_root = m_layer = TAGID_NULL;
        if (m_db)
        {
            m_root = SdbFindFirstTag(m_db, TAGID_ROOT, TAG_DATABASE);
            if (m_root != TAGID_NULL)
            {
                m_layer = SdbFindFirstTag(m_db, m_root, TAG_LAYER);
                return S_OK;
            }
        }
        return E_FAIL;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum)
    {
        return E_NOTIMPL;
    }

protected:
    PDB m_db;
    TAGID m_root;
    TAGID m_layer;

public:
    BEGIN_COM_MAP(CLayerStringList)
        COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
    END_COM_MAP()
};

