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
    virtual HRESULT STDMETHODCALLTYPE AddNameSpace(LPGUID, IShellFolder *, LPCITEMIDLIST, ULONG) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetNameSpaceID(LPCITEMIDLIST, LPGUID) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryNameSpace(ULONG, LPGUID, IShellFolder **) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumNameSpace(ULONG, PULONG) = 0;
};

interface IAugmentedShellFolder2 : public IAugmentedShellFolder
{
    virtual HRESULT STDMETHODCALLTYPE UnWrapIDList(LPCITEMIDLIST, LONG, IShellFolder **, LPITEMIDLIST *, LPITEMIDLIST *, LONG *) = 0;
};

/* No idea what QUERYNAMESPACEINFO struct contains, yet */
struct QUERYNAMESPACEINFO
{
    BYTE unknown[1];
};

interface IAugmentedShellFolder3 : public IAugmentedShellFolder2
{
    virtual HRESULT STDMETHODCALLTYPE QueryNameSpace2(ULONG, QUERYNAMESPACEINFO *) = 0;
};

class CEnumMergedFolder;

class CMergedFolder :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    public IPersistFolder2,
    public IAugmentedShellFolder3     // -- undocumented
    //public IShellService,              // DEPRECATED IE4 interface: https://msdn.microsoft.com/en-us/library/windows/desktop/bb774870%28v=vs.85%29.aspx
    //public ITranslateShellChangeNotify,// -- undocumented
    //public IStorage,
    //public IPersistPropertyBag,
    //public IShellIconOverlay,          // -- undocumented
    //public ICompositeFolder,           // -- undocumented
    //public IItemNameLimits,            // https://msdn.microsoft.com/en-us/library/windows/desktop/bb761776%28v=vs.85%29.aspx
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

    DECLARE_NOT_AGGREGATABLE(CMergedFolder)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMergedFolder)
        COM_INTERFACE_ENTRY2_IID(IID_IShellFolder, IShellFolder, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2,   IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersist,        IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder,  IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
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
        PCUITEMID_CHILD_ARRAY apidl,
        SFGAOF *rgfInOut);

    virtual HRESULT STDMETHODCALLTYPE GetUIObjectOf(
        HWND hwndOwner,
        UINT cidl,
        PCUITEMID_CHILD_ARRAY apidl,
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

    // IPersist
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *lpClassId);

    // IPersistFolder
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    virtual HRESULT STDMETHODCALLTYPE GetCurFolder(LPITEMIDLIST * pidl);

    // IAugmentedShellFolder2
    virtual HRESULT STDMETHODCALLTYPE AddNameSpace(LPGUID lpGuid, IShellFolder * psf, LPCITEMIDLIST pcidl, ULONG dwUnknown);
    virtual HRESULT STDMETHODCALLTYPE GetNameSpaceID(LPCITEMIDLIST pcidl, LPGUID lpGuid);
    virtual HRESULT STDMETHODCALLTYPE QueryNameSpace(ULONG dwUnknown, LPGUID lpGuid, IShellFolder ** ppsf);
    virtual HRESULT STDMETHODCALLTYPE EnumNameSpace(ULONG dwUnknown, PULONG lpUnknown);
    virtual HRESULT STDMETHODCALLTYPE UnWrapIDList(LPCITEMIDLIST pcidl, LONG lUnknown, IShellFolder ** ppsf, LPITEMIDLIST * ppidl1, LPITEMIDLIST *ppidl2, LONG * lpUnknown);

    // IAugmentedShellFolder3
    virtual HRESULT STDMETHODCALLTYPE QueryNameSpace2(ULONG, QUERYNAMESPACEINFO *);
};
