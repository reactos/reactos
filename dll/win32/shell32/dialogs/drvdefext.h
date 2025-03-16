/*
 * Provides default drive shell extension
 *
 * Copyright 2012 Rafal Harabien
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

#ifndef _DRV_DEF_EXT_H_
#define _DRV_DEF_EXT_H_

class CDrvDefExt :
	public CComCoClass<CDrvDefExt, &CLSID_ShellDrvDefExt>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IShellExtInit,
	public IContextMenu,
	public IShellPropSheetExt,
	public IObjectWithSite
{
private:
    VOID PaintStaticControls(HWND hwndDlg, LPDRAWITEMSTRUCT pDrawItem);
    VOID InitGeneralPage(HWND hwndDlg);
    static INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ExtraPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK HardwarePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    WCHAR m_wszDrive[MAX_PATH];
    UINT m_FreeSpacePerc;
    CComPtr<IDataObject> m_Multiple;

    HRESULT AddMainPage(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);

public:
	CDrvDefExt();
	~CDrvDefExt();

	// IShellExtInit
	STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pDataObj, HKEY hkeyProgID) override;

    // IContextMenu
	STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
	STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici) override;
	STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax) override;

	// IShellPropSheetExt
	STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) override;
	STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam) override;

    // IObjectWithSite
	STDMETHOD(SetSite)(IUnknown *punk) override;
	STDMETHOD(GetSite)(REFIID iid, void **ppvSite) override;

DECLARE_REGISTRY_RESOURCEID(IDR_DRVDEFEXT)
DECLARE_NOT_AGGREGATABLE(CDrvDefExt)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDrvDefExt)
	COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
	COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
	COM_INTERFACE_ENTRY_IID(IID_IShellPropSheetExt, IShellPropSheetExt)
	COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
END_COM_MAP()
};

#endif /* _DRV_DEF_EXT_H_ */
