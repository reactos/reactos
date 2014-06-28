/*
 * Shell Menu Site
 *
 * Copyright 2014 David Quintana
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
#pragma once

class CEnumMergedFolder;

class CMergedFolder :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2
{
private:
    CComPtr<IShellFolder> m_UserLocal;
    CComPtr<IShellFolder> m_AllUSers;
    CComPtr<CEnumMergedFolder> m_EnumSource;

public:
    CMergedFolder() {}
    virtual ~CMergedFolder() {}

    HRESULT _SetSources(IShellFolder* userLocal, IShellFolder* allUSers);

    DECLARE_NOT_AGGREGATABLE(CMergedFolder)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMergedFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
    END_COM_MAP()

    // IShellFolder
    virtual HRESULT STDMETHODCALLTYPE ParseDisplayName(
        HWND hwndOwner,
        LPBC pbcReserved,
        LPOLESTR lpszDisplayName,
        ULONG *pchEaten,
        LPITEMIDLIST *ppidl,
        ULONG *pdwAttributes);

    virtual HRESULT STDMETHODCALLTYPE EnumObjects(
        HWND hwndOwner,
        SHCONTF grfFlags,
        IEnumIDList **ppenumIDList);

    virtual HRESULT STDMETHODCALLTYPE BindToObject(
        LPCITEMIDLIST pidl,
        LPBC pbcReserved,
        REFIID riid,
        void **ppvOut);

    virtual HRESULT STDMETHODCALLTYPE BindToStorage(
        LPCITEMIDLIST pidl,
        LPBC pbcReserved,
        REFIID riid,
        void **ppvObj);

    virtual HRESULT STDMETHODCALLTYPE CompareIDs(
        LPARAM lParam,
        LPCITEMIDLIST pidl1,
        LPCITEMIDLIST pidl2);

    virtual HRESULT STDMETHODCALLTYPE CreateViewObject(
        HWND hwndOwner,
        REFIID riid,
        void **ppvOut);

    virtual HRESULT STDMETHODCALLTYPE GetAttributesOf(
        UINT cidl,
        LPCITEMIDLIST *apidl,
        SFGAOF *rgfInOut);

    virtual HRESULT STDMETHODCALLTYPE GetUIObjectOf(
        HWND hwndOwner,
        UINT cidl,
        LPCITEMIDLIST *apidl,
        REFIID riid,
        UINT *prgfInOut,
        void **ppvOut);

    virtual HRESULT STDMETHODCALLTYPE GetDisplayNameOf(
        LPCITEMIDLIST pidl,
        SHGDNF uFlags,
        STRRET *lpName);

    virtual HRESULT STDMETHODCALLTYPE SetNameOf(
        HWND hwnd,
        LPCITEMIDLIST pidl,
        LPCOLESTR lpszName,
        SHGDNF uFlags,
        LPITEMIDLIST *ppidlOut);

    // IShellFolder2
    virtual HRESULT STDMETHODCALLTYPE GetDefaultSearchGUID(
        GUID *lpguid);

    virtual HRESULT STDMETHODCALLTYPE EnumSearches(
        IEnumExtraSearch **ppenum);

    virtual HRESULT STDMETHODCALLTYPE GetDefaultColumn(
        DWORD dwReserved,
        ULONG *pSort,
        ULONG *pDisplay);

    virtual HRESULT STDMETHODCALLTYPE GetDefaultColumnState(
        UINT iColumn,
        SHCOLSTATEF *pcsFlags);

    virtual HRESULT STDMETHODCALLTYPE GetDetailsEx(
        LPCITEMIDLIST pidl,
        const SHCOLUMNID *pscid,
        VARIANT *pv);

    virtual HRESULT STDMETHODCALLTYPE GetDetailsOf(
        LPCITEMIDLIST pidl,
        UINT iColumn,
        SHELLDETAILS *psd);

    virtual HRESULT STDMETHODCALLTYPE MapColumnToSCID(
        UINT iColumn,
        SHCOLUMNID *pscid);

};
