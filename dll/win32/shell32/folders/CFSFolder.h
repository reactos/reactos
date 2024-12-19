/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     file system folder
 * COPYRIGHT:   Copyright 1997 Marcus Meissner
 *              Copyright 1998, 1999, 2002 Juergen Schmied
 *              Copyright 2009 Andrew Hill
 *              Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef _CFSFOLDER_H_
#define _CFSFOLDER_H_

class CFSFolder :
    public CComCoClass<CFSFolder, &CLSID_ShellFSFolder>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    public IPersistFolder3,
    public IContextMenuCB,
    public IShellFolderViewCB,
    public IItemNameLimits
{
    private:
        const CLSID *m_pclsid;

        /* both paths are parsible from the desktop */
        LPWSTR m_sPathTarget;     /* complete path to target used for enumeration and ChangeNotify */

        LPITEMIDLIST m_pidlRoot; /* absolute pidl */

        DWORD m_bGroupPolicyActive;
        HRESULT _CreateShellExtInstance(const CLSID *pclsid, LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppvOut);
        HRESULT _CreateExtensionUIObject(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppvOut);
        HRESULT _GetDropTarget(LPCITEMIDLIST pidl, LPVOID *ppvOut);
        HRESULT _GetIconHandler(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppvOut);

        HRESULT _ParseSimple(
            _In_ LPOLESTR lpszDisplayName,
            _Inout_ WIN32_FIND_DATAW *pFind,
            _Out_ LPITEMIDLIST *ppidl);
        BOOL _GetFindDataFromName(_In_ LPCWSTR pszName, _Out_ WIN32_FIND_DATAW *pFind);
        HRESULT _CreateIDListFromName(LPCWSTR pszName, DWORD attrs, IBindCtx *pbc, LPITEMIDLIST *ppidl);

    public:
        CFSFolder();
        ~CFSFolder();

        // IShellFolder
        STDMETHOD(ParseDisplayName)(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes) override;
        STDMETHOD(EnumObjects)(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList) override;
        STDMETHOD(BindToObject)(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override;
        STDMETHOD(BindToStorage)(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override;
        STDMETHOD(CompareIDs)(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2) override;
        STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, LPVOID *ppvOut) override;
        STDMETHOD(GetAttributesOf)(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut) override;
        STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) override;
        STDMETHOD(GetDisplayNameOf)(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet) override;
        STDMETHOD(SetNameOf)(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut) override;

        /* ShellFolder2 */
        STDMETHOD(GetDefaultSearchGUID)(GUID *pguid) override;
        STDMETHOD(EnumSearches)(IEnumExtraSearch **ppenum) override;
        STDMETHOD(GetDefaultColumn)(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) override;
        STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pcsFlags) override;
        STDMETHOD(GetDetailsEx)(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv) override;
        STDMETHOD(GetDetailsOf)(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd) override;
        STDMETHOD(MapColumnToSCID)(UINT column, SHCOLUMNID *pscid) override;

        // IPersist
        STDMETHOD(GetClassID)(CLSID *lpClassId) override;

        // IPersistFolder
        STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidl) override;

        // IPersistFolder2
        STDMETHOD(GetCurFolder)(PIDLIST_ABSOLUTE * pidl) override;

        // IPersistFolder3
        STDMETHOD(InitializeEx)(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfti) override;
        STDMETHOD(GetFolderTargetInfo)(PERSIST_FOLDER_TARGET_INFO *ppfti) override;

        // IContextMenuCB
        STDMETHOD(CallBack)(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

        // IShellFolderViewCB
        STDMETHOD(MessageSFVCB)(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

        /*** IItemNameLimits methods ***/

        STDMETHODIMP
        GetMaxLength(LPCWSTR pszName, int *piMaxNameLen) override
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

    DECLARE_REGISTRY_RESOURCEID(IDR_SHELLFSFOLDER)
    DECLARE_NOT_AGGREGATABLE(CFSFolder)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFSFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder3, IPersistFolder3)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
        COM_INTERFACE_ENTRY_IID(IID_IItemNameLimits, IItemNameLimits)
    END_COM_MAP()

    protected:
        HRESULT WINAPI GetCustomViewInfo(ULONG unknown, SFVM_CUSTOMVIEWINFO_DATA *data);

    public:
        // Helper functions shared with CDesktopFolder
        static HRESULT GetFSColumnDetails(UINT iColumn, SHELLDETAILS &sd);
        static HRESULT GetDefaultFSColumnState(UINT iColumn, SHCOLSTATEF &csFlags);
        static HRESULT FormatDateTime(const FILETIME &ft, LPWSTR Buf, UINT cchBuf);
        static HRESULT FormatSize(UINT64 size, LPWSTR Buf, UINT cchBuf);
        static HRESULT CompareSortFoldersFirst(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
        static inline int CompareUiStrings(LPCWSTR a, LPCWSTR b)
        {
            return StrCmpLogicalW(a, b);
        }
};

#endif /* _CFSFOLDER_H_ */
