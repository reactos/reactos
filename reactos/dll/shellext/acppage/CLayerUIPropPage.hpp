/*
 * PROJECT:     ReactOS Compatibility Layer Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CLayerUIPropPage definition
 * COPYRIGHT:   Copyright 2015-2017 Mark Jansen (mark.jansen@reactos.org)
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
