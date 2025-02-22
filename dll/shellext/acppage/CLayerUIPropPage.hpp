/*
 * PROJECT:     ReactOS Compatibility Layer Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CLayerUIPropPage definition
 * COPYRIGHT:   Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

class CLayerUIPropPage :
    public CPropertyPageImpl<CLayerUIPropPage>,
    public CComCoClass<CLayerUIPropPage, &CLSID_CLayerUIPropPage>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellExtInit,
    public IShellPropSheetExt
{
public:
    CSimpleArray<CString> m_CustomLayers;

    CLayerUIPropPage();
    ~CLayerUIPropPage();

    // IShellExtInit
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, LPDATAOBJECT pdtobj, HKEY hkeyProgID);


    // IShellPropSheetExt
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
    {
        HPROPSHEETPAGE hPage = Create();
        if (hPage && !pfnAddPage(hPage, lParam))
            DestroyPropertySheetPage(hPage);

        return S_OK;
    }

    STDMETHODIMP ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM)
    {
        return E_NOTIMPL;
    }

    VOID OnPageAddRef()
    {
        InterlockedIncrement(&g_ModuleRefCnt);
    }

    VOID OnPageRelease()
    {
        InterlockedDecrement(&g_ModuleRefCnt);
    }

    HRESULT InitFile(PCWSTR Filename);
    void UpdateControls();
    INT_PTR DisableControls();
    BOOL HasChanges() const;

    int OnSetActive();
    int OnApply();
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCtrlCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnEditModes(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnClickNotify(INT uCode, LPNMHDR hdr, BOOL& bHandled);

protected:
    CString m_Filename;
    BOOL m_IsSfcProtected;
    BOOL m_AllowPermLayer;
    DWORD m_LayerQueryFlags;
    DWORD m_RegistryOSMode, m_OSMode;
    DWORD m_RegistryEnabledLayers, m_EnabledLayers;
    CSimpleArray<CString> m_RegistryCustomLayers;

public:
    enum { IDD = IDD_ACPPAGESHEET };

    BEGIN_MSG_MAP(CLayerUIPropPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_RANGE_HANDLER(IDC_CHKRUNCOMPATIBILITY, IDC_CHKDISABLEVISUALTHEMES, OnCtrlCommand)
        COMMAND_ID_HANDLER(IDC_EDITCOMPATIBILITYMODES, OnEditModes)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnClickNotify)
        NOTIFY_CODE_HANDLER(NM_RETURN, OnClickNotify)
        CHAIN_MSG_MAP(CPropertyPageImpl<CLayerUIPropPage>)
    END_MSG_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_ACPPAGE)
    DECLARE_NOT_AGGREGATABLE(CLayerUIPropPage)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CLayerUIPropPage)
        COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
        COM_INTERFACE_ENTRY_IID(IID_IShellPropSheetExt, IShellPropSheetExt)
    END_COM_MAP()
};
