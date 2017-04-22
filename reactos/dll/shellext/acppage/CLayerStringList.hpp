/*
 * Copyright 2015-2017 Mark Jansen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


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
            if (!nameid)
                return S_FALSE;

            LPWSTR name = SdbGetStringTagPtr(m_db, nameid);
            if (!name)
                return S_FALSE;

            ULONG Size = wcslen(name) + 1;

            *rgelt = (LPOLESTR)::CoTaskMemAlloc(Size * sizeof(WCHAR));
            StringCchCopyW(*rgelt, Size, name);

            if (pceltFetched)
                (*pceltFetched)++;

            m_layer = SdbFindNextTag(m_db, m_root, m_layer);
            celt--;
            rgelt++;
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

