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

        LPWSTR sPathTarget;     /* complete path to target used for enumeration and ChangeNotify */
        LPITEMIDLIST pidlRoot;  /* absolute pidl */

        HRESULT _GetSFFromPidl(LPCITEMIDLIST pidl, IShellFolder2** psf);

    public:
        CDesktopFolder();
        ~CDesktopFolder();
        HRESULT WINAPI FinalConstruct();

        // *** IShellFolder methods ***
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

        // *** IShellFolder2 methods ***
        virtual HRESULT WINAPI GetDefaultSearchGUID(GUID *pguid);
        virtual HRESULT WINAPI EnumSearches(IEnumExtraSearch **ppenum);
        virtual HRESULT WINAPI GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay);
        virtual HRESULT WINAPI GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags);
        virtual HRESULT WINAPI GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv);
        virtual HRESULT WINAPI GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd);
        virtual HRESULT WINAPI MapColumnToSCID(UINT column, SHCOLUMNID *pscid);

        // *** IPersist methods ***
        virtual HRESULT WINAPI GetClassID(CLSID *lpClassId);

        // *** IPersistFolder methods ***
        virtual HRESULT WINAPI Initialize(PCIDLIST_ABSOLUTE pidl);

        // *** IPersistFolder2 methods ***
        virtual HRESULT WINAPI GetCurFolder(PIDLIST_ABSOLUTE * pidl);

        // IContextMenuCB
        virtual HRESULT WINAPI CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
