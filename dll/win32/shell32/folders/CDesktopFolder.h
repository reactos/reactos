/*
 *    Virtual Desktop Folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2009                Andrew Hill
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

#ifndef _CDESKTOPFOLDER_H_
#define _CDESKTOPFOLDER_H_

class CStubFolderBase : public IShellFolder
{
public:
    CStubFolderBase() { }

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override { return E_NOTIMPL; }
    STDMETHODIMP_(ULONG) AddRef()  override { return 3; }
    STDMETHODIMP_(ULONG) Release() override { return 2; }

    // IShellFolder methods
    STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
                                  DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes) override
    { return E_NOTIMPL; }
    STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList) override
    { return E_NOTIMPL; }
    STDMETHODIMP BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override
    { return E_NOTIMPL; }
    STDMETHODIMP BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override
    { return E_NOTIMPL; }
    STDMETHODIMP CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2) override
    { return E_NOTIMPL; }
    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut) override
    { return E_NOTIMPL; }
    STDMETHODIMP GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut) override
    { return E_NOTIMPL; }
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
                               REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) override
    { return E_NOTIMPL; }
    STDMETHODIMP GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet) override
    { return E_NOTIMPL; }
    STDMETHODIMP SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName,
                           DWORD dwFlags, PITEMID_CHILD *pPidlOut) override
    { return E_NOTIMPL; }
};

class CShellUrlStub : public CStubFolderBase
{
public:
    STDMETHODIMP
    ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten,
                     PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes) override;
};

class CFileUrlStub : public CStubFolderBase
{
public:
    STDMETHODIMP
    ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten,
                     PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes) override;
};

class CIDListUrlStub : public CStubFolderBase
{
public:
    STDMETHODIMP
    ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten,
                     PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes) override;
};

class CHttpUrlStub : public CStubFolderBase
{
public:
    STDMETHODIMP
    ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, DWORD *pchEaten,
                     PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes) override;
};

class CDesktopFolder :
    public CComCoClass<CDesktopFolder, &CLSID_ShellDesktop>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    public IPersistFolder2,
    public IContextMenuCB,
    public IItemNameLimits
{
    private:
        /* both paths are parsible from the desktop */
        CComPtr<IShellFolder2> m_DesktopFSFolder;
        CComPtr<IShellFolder2> m_SharedDesktopFSFolder;
        CComPtr<IShellFolder2> m_regFolder;

        // Stub URL objects
        CShellUrlStub   m_ShellUrlStub;
        CFileUrlStub    m_FileUrlStub;
        CIDListUrlStub  m_IDListUrlStub;
        CHttpUrlStub    m_HttpUrlStub;

        LPWSTR sPathTarget;     /* complete path to target used for enumeration and ChangeNotify */
        LPITEMIDLIST pidlRoot;  /* absolute pidl */

        HRESULT _GetSFFromPidl(LPCITEMIDLIST pidl, IShellFolder2** psf);

        BOOL _TryUrlJunctions(
            LPCWSTR pcszURL,
            IBindCtx *pBindCtx,
            IShellFolder **ppShellFolder,
            LPITEMIDLIST *ppidl);
        BOOL _GetParentForParsing(
            LPCWSTR pszPath,
            IBindCtx *pbc,
            IShellFolder **ppParentFolder,
            LPITEMIDLIST *ppidlParent);
        HRESULT _ChildParseDisplayName(
            IShellFolder *pParentFolder,
            LPCITEMIDLIST pidlParent,
            HWND hwndOwner,
            IBindCtx *pBindCtx,
            LPWSTR lpszDisplayName,
            DWORD *pchEaten,
            LPITEMIDLIST *ppidl,
            DWORD *pdwAttributes);

    public:
        CDesktopFolder();
        ~CDesktopFolder();
        HRESULT WINAPI FinalConstruct();

        // *** IShellFolder methods ***
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

        // *** IShellFolder2 methods ***
        STDMETHOD(GetDefaultSearchGUID)(GUID *pguid) override;
        STDMETHOD(EnumSearches)(IEnumExtraSearch **ppenum) override;
        STDMETHOD(GetDefaultColumn)(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) override;
        STDMETHOD(GetDefaultColumnState)(UINT iColumn, DWORD *pcsFlags) override;
        STDMETHOD(GetDetailsEx)(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv) override;
        STDMETHOD(GetDetailsOf)(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd) override;
        STDMETHOD(MapColumnToSCID)(UINT column, SHCOLUMNID *pscid) override;

        // *** IPersist methods ***
        STDMETHOD(GetClassID)(CLSID *lpClassId) override;

        // *** IPersistFolder methods ***
        STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidl) override;

        // *** IPersistFolder2 methods ***
        STDMETHOD(GetCurFolder)(PIDLIST_ABSOLUTE * pidl) override;

        // IContextMenuCB
        STDMETHOD(CallBack)(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

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

        DECLARE_REGISTRY_RESOURCEID(IDR_SHELLDESKTOP)
        DECLARE_CENTRAL_INSTANCE_NOT_AGGREGATABLE(CDesktopFolder)

        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CDesktopFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IItemNameLimits, IItemNameLimits)
        END_COM_MAP()
};

#endif /* _CDESKTOPFOLDER_H_ */
