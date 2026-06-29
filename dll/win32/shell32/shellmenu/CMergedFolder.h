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

interface IAugmentedShellFolder : public IShellFolder
{
    STDMETHOD(AddNameSpace)(LPGUID, IShellFolder *, LPCITEMIDLIST, ULONG) PURE;
    STDMETHOD(GetNameSpaceID)(LPCITEMIDLIST, LPGUID) PURE;
    STDMETHOD(QueryNameSpace)(ULONG, LPGUID, IShellFolder **) PURE;
    STDMETHOD(EnumNameSpace)(ULONG, PULONG) PURE;
};

interface IAugmentedShellFolder2 : public IAugmentedShellFolder
{
    STDMETHOD(UnWrapIDList)(LPCITEMIDLIST, LONG, IShellFolder **, LPITEMIDLIST *, LPITEMIDLIST *, LONG *) PURE;
};

/* No idea what QUERYNAMESPACEINFO struct contains, yet */
struct QUERYNAMESPACEINFO
{
    BYTE unknown[1];
};

interface IAugmentedShellFolder3 : public IAugmentedShellFolder2
{
    STDMETHOD(QueryNameSpace2)(ULONG, QUERYNAMESPACEINFO *) PURE;
};

class CEnumMergedFolder;

class CMergedFolder :
    public CComCoClass<CMergedFolder, &CLSID_MergedFolder>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    public IPersistFolder2,
    public IItemNameLimits,
    public IAugmentedShellFolder3     // -- undocumented
    //public IShellService,              // DEPRECATED IE4 interface: https://learn.microsoft.com/en-us/windows/win32/api/shdeprecated/nn-shdeprecated-ishellservice
    //public ITranslateShellChangeNotify,// -- undocumented
    //public IStorage,
    //public IPersistPropertyBag,
    //public IShellIconOverlay,          // -- undocumented
    //public ICompositeFolder,           // -- undocumented
    //public IItemNameLimits,            // https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-iitemnamelimits
{
private:
    CComPtr<IShellFolder> m_UserLocal;
    CComPtr<IShellFolder> m_AllUsers;
    CComPtr<CEnumMergedFolder> m_EnumSource;

    LPITEMIDLIST m_UserLocalPidl;
    LPITEMIDLIST m_AllUsersPidl;
    LPITEMIDLIST m_shellPidl;

public:
    CMergedFolder();
    virtual ~CMergedFolder();

    HRESULT _SetSources(IShellFolder* userLocal, IShellFolder* allUSers);

    DECLARE_REGISTRY_RESOURCEID(IDR_MERGEDFOLDER)
    DECLARE_NOT_AGGREGATABLE(CMergedFolder)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMergedFolder)
        COM_INTERFACE_ENTRY2_IID(IID_IShellFolder, IShellFolder, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2,   IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersist,        IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder,  IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IItemNameLimits, IItemNameLimits)
        COM_INTERFACE_ENTRY_IID(IID_IAugmentedShellFolder,  IAugmentedShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IAugmentedShellFolder2, IAugmentedShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IAugmentedShellFolder3, IAugmentedShellFolder3)
        //COM_INTERFACE_ENTRY_IID(IID_IStorage,                   IStorage)
        //COM_INTERFACE_ENTRY_IID(IID_IShellService,              IShellService)
        //COM_INTERFACE_ENTRY_IID(IID_ITranslateShellChangeNotify,ITranslateShellChangeNotify)
        //COM_INTERFACE_ENTRY_IID(IID_IPersistPropertyBag,IPersistPropertyBag)
        //COM_INTERFACE_ENTRY_IID(IID_IShellIconOverlay,  IShellIconOverlay)
        //COM_INTERFACE_ENTRY_IID(IID_ICompositeFolder,   ICompositeFolder)
        //COM_INTERFACE_ENTRY_IID(IID_IItemNameLimits,    IItemNameLimits)
    END_COM_MAP()

    // IShellFolder
    STDMETHOD(ParseDisplayName)(
        HWND hwndOwner,
        LPBC pbcReserved,
        LPOLESTR lpszDisplayName,
        ULONG *pchEaten,
        LPITEMIDLIST *ppidl,
        ULONG *pdwAttributes) override;

    STDMETHOD(EnumObjects)(
        HWND hwndOwner,
        SHCONTF grfFlags,
        IEnumIDList **ppenumIDList) override;

    STDMETHOD(BindToObject)(
        LPCITEMIDLIST pidl,
        LPBC pbcReserved,
        REFIID riid,
        void **ppvOut) override;

    STDMETHOD(BindToStorage)(
        LPCITEMIDLIST pidl,
        LPBC pbcReserved,
        REFIID riid,
        void **ppvObj) override;

    STDMETHOD(CompareIDs)(
        LPARAM lParam,
        LPCITEMIDLIST pidl1,
        LPCITEMIDLIST pidl2) override;

    STDMETHOD(CreateViewObject)(
        HWND hwndOwner,
        REFIID riid,
        void **ppvOut) override;

    STDMETHOD(GetAttributesOf)(
        UINT cidl,
        PCUITEMID_CHILD_ARRAY apidl,
        SFGAOF *rgfInOut) override;

    STDMETHOD(GetUIObjectOf)(
        HWND hwndOwner,
        UINT cidl,
        PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid,
        UINT *prgfInOut,
        void **ppvOut) override;

    STDMETHOD(GetDisplayNameOf)(
        LPCITEMIDLIST pidl,
        SHGDNF uFlags,
        STRRET *lpName) override;

    STDMETHOD(SetNameOf)(
        HWND hwnd,
        LPCITEMIDLIST pidl,
        LPCOLESTR lpszName,
        SHGDNF uFlags,
        LPITEMIDLIST *ppidlOut) override;

    // IShellFolder2
    STDMETHOD(GetDefaultSearchGUID)(
        GUID *lpguid) override;

    STDMETHOD(EnumSearches)(
        IEnumExtraSearch **ppenum) override;

    STDMETHOD(GetDefaultColumn)(
        DWORD dwReserved,
        ULONG *pSort,
        ULONG *pDisplay) override;

    STDMETHOD(GetDefaultColumnState)(
        UINT iColumn,
        SHCOLSTATEF *pcsFlags) override;

    STDMETHOD(GetDetailsEx)(
        LPCITEMIDLIST pidl,
        const SHCOLUMNID *pscid,
        VARIANT *pv) override;

    STDMETHOD(GetDetailsOf)(
        LPCITEMIDLIST pidl,
        UINT iColumn,
        SHELLDETAILS *psd) override;

    STDMETHOD(MapColumnToSCID)(
        UINT iColumn,
        SHCOLUMNID *pscid) override;

    // IPersist
    STDMETHOD(GetClassID)(CLSID *lpClassId) override;

    // IPersistFolder
    STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidl) override;

    // IPersistFolder2
    STDMETHOD(GetCurFolder)(PIDLIST_ABSOLUTE * pidl) override;

    /*** IItemNameLimits methods ***/

    STDMETHODIMP GetMaxLength(LPCWSTR pszName, int *piMaxNameLen) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    GetValidCharacters(LPWSTR *ppwszValidChars, LPWSTR *ppwszInvalidChars) override
    {
        if (ppwszValidChars)
        {
            *ppwszValidChars = NULL;
        }
        if (ppwszInvalidChars)
        {
            SHStrDupW(INVALID_FILETITLE_CHARACTERSW, ppwszInvalidChars);
        }
        return S_OK;
    }

    // IAugmentedShellFolder2
    STDMETHOD(AddNameSpace)(LPGUID lpGuid, IShellFolder * psf, LPCITEMIDLIST pcidl, ULONG dwUnknown) override;
    STDMETHOD(GetNameSpaceID)(LPCITEMIDLIST pcidl, LPGUID lpGuid) override;
    STDMETHOD(QueryNameSpace)(ULONG dwUnknown, LPGUID lpGuid, IShellFolder ** ppsf) override;
    STDMETHOD(EnumNameSpace)(ULONG dwUnknown, PULONG lpUnknown) override;
    STDMETHOD(UnWrapIDList)(LPCITEMIDLIST pcidl, LONG lUnknown, IShellFolder ** ppsf, LPITEMIDLIST * ppidl1, LPITEMIDLIST *ppidl2, LONG * lpUnknown) override;

    // IAugmentedShellFolder3
    STDMETHOD(QueryNameSpace2)(ULONG, QUERYNAMESPACEINFO *) override;
};
