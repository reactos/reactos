/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/ntobjshex/regfolder.h
 * PURPOSE:     NT Object Namespace shell extension
 * PROGRAMMERS: David Quintana <gigaherz@gmail.com>
 */
#pragma once

extern const GUID CLSID_RegistryFolder;

class CRegistryFolder :
    public CComCoClass<CRegistryFolder, &CLSID_RegistryFolder>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    public IPersistFolder2,
    public IShellFolderViewCB
{
    HKEY m_hRoot;
    WCHAR m_NtPath[MAX_PATH];

    LPITEMIDLIST m_shellPidl;

public:

    CRegistryFolder();
    virtual ~CRegistryFolder();

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

    // IShellFolderViewCB
    virtual HRESULT STDMETHODCALLTYPE MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Internal
    HRESULT STDMETHODCALLTYPE Initialize(LPCITEMIDLIST pidl, PCWSTR ntPath, HKEY hRoot);

    static HRESULT CALLBACK DefCtxMenuCallback(IShellFolder *, HWND, IDataObject *, UINT, WPARAM, LPARAM);

    DECLARE_REGISTRY_RESOURCEID(IDR_REGISTRYFOLDER)
    DECLARE_NOT_AGGREGATABLE(CRegistryFolder)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CRegistryFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
    END_COM_MAP()

};