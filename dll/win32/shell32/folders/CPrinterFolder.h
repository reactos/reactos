/*
 *    Virtual Printers Folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2005                Huw Davies
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _SHFLDR_PRINTERS_H_
#define _SHFLDR_PRINTERS_H_

class CPrinterFolder :
    public CComCoClass<CPrinterFolder, &CLSID_Printers>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellFolder2,
    public IPersistFolder2
{
    private:
        CLSID *pclsid;

        LPITEMIDLIST pidlRoot;  /* absolute pidl */

        int dwAttributes;        /* attributes returned by GetAttributesOf FIXME: use it */
    public:
        CPrinterFolder();
        ~CPrinterFolder();

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

        DECLARE_REGISTRY_RESOURCEID(IDR_PRINTERS)
        DECLARE_NOT_AGGREGATABLE(CPrinterFolder)

        DECLARE_PROTECT_FINAL_CONSTRUCT()

        BEGIN_COM_MAP(CPrinterFolder)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder2, IShellFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IShellFolder, IShellFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2, IPersistFolder2)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        END_COM_MAP()
};

#endif /* _SHFLDR_PRINTERS_H_ */
