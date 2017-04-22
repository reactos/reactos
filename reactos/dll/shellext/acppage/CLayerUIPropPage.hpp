/*
 * Copyright 2015-2017 Mark Jansen
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


class CLayerUIPropPage :
    public CComCoClass<CLayerUIPropPage, &CLSID_CLayerUIPropPage>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellExtInit,
    public IShellPropSheetExt
{
public:
    CLayerUIPropPage();
    ~CLayerUIPropPage();

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pdtobj, HKEY hkeyProgID);


    // IShellPropSheetExt
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    STDMETHODIMP ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM)
    {
        return E_NOTIMPL;
    }

    HRESULT InitFile(PCWSTR Filename);
    INT_PTR InitDialog(HWND hWnd);
    INT_PTR OnCommand(HWND hWnd, WORD id);
    void UpdateControls(HWND hWnd);
    INT_PTR DisableControls(HWND hWnd);
    BOOL HasChanges() const;

    void OnRefresh(HWND hWnd);
    void OnApply(HWND hWnd);

    static INT_PTR CALLBACK PropDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK EditModesProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


protected:
    CString m_Filename;
    BOOL m_IsSfcProtected;
    BOOL m_AllowPermLayer;
    DWORD m_LayerQueryFlags;
    DWORD m_RegistryOSMode, m_OSMode;
    DWORD m_RegistryEnabledLayers, m_EnabledLayers;
    CSimpleArray<CString> m_RegistryCustomLayers, m_CustomLayers;

public:
    DECLARE_REGISTRY_RESOURCEID(IDR_ACPPAGE)
    DECLARE_NOT_AGGREGATABLE(CLayerUIPropPage)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CLayerUIPropPage)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
        COM_INTERFACE_ENTRY_IID(IID_IShellPropSheetExt, IShellPropSheetExt)
    END_COM_MAP()
};
