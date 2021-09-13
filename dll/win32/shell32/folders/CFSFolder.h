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
    public:
        CFSFolder();
        ~CFSFolder();

        // IShellFolder
        virtual HRESULT WINAPI ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes);
        virtual HRESULT WINAPI EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList);
        virtual HRESULT WINAPI BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);
        virtual HRESULT WINAPI BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut);
        virtual HRESULT WINAPI CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2);
        virtual HRESULT WINAPI CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut);
        virtual HRESULT WINAPI GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut);
        virtual HRESULT WINAPI GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
        virtual HRESULT WINAPI GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet);
        virtual HRESULT WINAPI SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut);

        /* ShellFolder2 */
        virtual HRESULT WINAPI GetDefaultSearchGUID(GUID *pguid);
        virtual HRESULT WINAPI EnumSearches(IEnumExtraSearch **ppenum);
        virtual HRESULT WINAPI GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
        virtual HRESULT WINAPI GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags);
        virtual HRESULT WINAPI GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv);
        virtual HRESULT WINAPI GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd);
        virtual HRESULT WINAPI MapColumnToSCID(UINT column, SHCOLUMNID *pscid);

        // IPersist
        virtual HRESULT WINAPI GetClassID(CLSID *lpClassId);

        // IPersistFolder
        virtual HRESULT WINAPI Initialize(PCIDLIST_ABSOLUTE pidl);

        // IPersistFolder2
        virtual HRESULT WINAPI GetCurFolder(PIDLIST_ABSOLUTE * pidl);

        // IPersistFolder3
        virtual HRESULT WINAPI InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfti);
        virtual HRESULT WINAPI GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *ppfti);

        // IContextMenuCB
        virtual HRESULT WINAPI CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // IShellFolderViewCB
        virtual HRESULT WINAPI MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

        /*** IItemNameLimits methods ***/

        STDMETHODIMP
        GetMaxLength(LPCWSTR pszName, int *piMaxNameLen)
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP
        GetValidCharacters(LPWSTR *ppwszValidChars, LPWSTR *ppwszInvalidChars)
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
};

#endif /* _CFSFOLDER_H_ */
