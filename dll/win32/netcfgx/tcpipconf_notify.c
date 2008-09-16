#include "precomp.h"

typedef struct
{
    const INetCfgComponentPropertyUi * lpVtbl;
    const INetCfgComponentControl * lpVtblCompControl;
    LONG  ref;
    IUnknown * pUnknown;
    INetCfg * pNCfg;
    INetCfgComponent * pNComp;
}TcpipConfNotifyImpl, *LPTcpipConfNotifyImpl;

static LPTcpipConfNotifyImpl __inline impl_from_INetCfgComponentControl(INetCfgComponentControl *iface)
{
    return (TcpipConfNotifyImpl*)((char *)iface - FIELD_OFFSET(TcpipConfNotifyImpl, lpVtblCompControl));
}


HPROPSHEETPAGE
InitializePropertySheetPage(LPWSTR resname, DLGPROC dlgproc, LPARAM lParam, LPWSTR szTitle)
{
    PROPSHEETPAGEW ppage;

    memset(&ppage, 0x0, sizeof(PROPSHEETPAGEW));
    ppage.dwSize = sizeof(PROPSHEETPAGEW);
    ppage.dwFlags = PSP_DEFAULT;
    ppage.u.pszTemplate = resname;
    ppage.pfnDlgProc = dlgproc;
    ppage.lParam = lParam;
    ppage.hInstance = netcfgx_hInstance;
    if (szTitle)
    {
        ppage.dwFlags |= PSP_USETITLE;
        ppage.pszTitle = szTitle;
    }
    return CreatePropertySheetPageW(&ppage);
}

INT_PTR
CALLBACK
TcpipBasicDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

/***************************************************************
 * INetCfgComponentPropertyUi interface
 */

HRESULT
STDCALL
INetCfgComponentPropertyUi_fnQueryInterface(
    INetCfgComponentPropertyUi * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    //LPOLESTR pStr;
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;

    *ppvObj = NULL;


    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_INetCfgComponentPropertyUi))
    {
        *ppvObj = This;
        INetCfgComponentPropertyUi_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualIID(iid, &IID_INetCfgComponentControl))
    {
        *ppvObj = (LPVOID*)&This->lpVtblCompControl;
        INetCfgComponentPropertyUi_AddRef(iface);
        return S_OK;
    }

    //StringFromCLSID(iid, &pStr);
    //MessageBoxW(NULL, pStr, L"INetConnectionPropertyUi_fnQueryInterface", MB_OK);
    //CoTaskMemFree(pStr);

    return E_NOINTERFACE;
}


ULONG
STDCALL
INetCfgComponentPropertyUi_fnAddRef(
    INetCfgComponentPropertyUi * iface)
{
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

ULONG
STDCALL
INetCfgComponentPropertyUi_fnRelease(
    INetCfgComponentPropertyUi * iface)
{
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount)
    {
       CoTaskMemFree(This);
    }
    return refCount;
}

HRESULT
STDCALL
INetCfgComponentPropertyUi_fnQueryPropertyUi(
    INetCfgComponentPropertyUi * iface,
    IUnknown *pUnkReserved)
{
    INetLanConnectionUiInfo * pLanInfo;
    HRESULT hr;

    hr = IUnknown_QueryInterface(pUnkReserved, &IID_INetLanConnectionUiInfo, (LPVOID*)&pLanInfo);
    if (FAILED(hr))
        return hr;

    //FIXME
    // check if tcpip is enabled on that binding */
    IUnknown_Release(pUnkReserved);
    return S_OK;
}

HRESULT
STDCALL
INetCfgComponentPropertyUi_fnSetContext(
    INetCfgComponentPropertyUi * iface,
    IUnknown *pUnkReserved)
{
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;

    if (!iface || !pUnkReserved)
        return E_POINTER;

    This->pUnknown = pUnkReserved;
    return S_OK;
}

HRESULT
STDCALL
INetCfgComponentPropertyUi_fnMergePropPages( 
    INetCfgComponentPropertyUi * iface,
    DWORD *pdwDefPages,
    BYTE **pahpspPrivate,
    UINT *pcPages,
    HWND hwndParent,
    LPCWSTR *pszStartPage)
{
    HPROPSHEETPAGE * hppages;
    TcpipConfNotifyImpl * This = (TcpipConfNotifyImpl*)iface;

    hppages = (HPROPSHEETPAGE*) CoTaskMemAlloc(sizeof(HPROPSHEETPAGE));
    if (!hppages)
        return E_FAIL;

    hppages[0] = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_TCPIP_BASIC_DLG), TcpipBasicDlg, (LPARAM)This, NULL);
    if (!hppages[0])
    {
        CoTaskMemFree(hppages);
        return E_FAIL;
    }

    *pahpspPrivate = (BYTE*)hppages;
    *pcPages = 1;

    return S_OK;
}

HRESULT
STDCALL
INetCfgComponentPropertyUi_fnValidateProperties(
    INetCfgComponentPropertyUi * iface,
    HWND hwndSheet)
{
    return E_NOTIMPL;
}

HRESULT
STDCALL
INetCfgComponentPropertyUi_fnApplyProperties(
    INetCfgComponentPropertyUi * iface)
{
    return E_NOTIMPL;
}


HRESULT
STDCALL
INetCfgComponentPropertyUi_fnCancelProperties(
    INetCfgComponentPropertyUi * iface)
{
    return E_NOTIMPL;
}

static const INetCfgComponentPropertyUiVtbl vt_NetCfgComponentPropertyUi =
{
    INetCfgComponentPropertyUi_fnQueryInterface,
    INetCfgComponentPropertyUi_fnAddRef,
    INetCfgComponentPropertyUi_fnRelease,
    INetCfgComponentPropertyUi_fnQueryPropertyUi,
    INetCfgComponentPropertyUi_fnSetContext,
    INetCfgComponentPropertyUi_fnMergePropPages,
    INetCfgComponentPropertyUi_fnValidateProperties,
    INetCfgComponentPropertyUi_fnApplyProperties,
    INetCfgComponentPropertyUi_fnCancelProperties
};

/***************************************************************
 * INetCfgComponentControl interface
 */

HRESULT
STDCALL
INetCfgComponentControl_fnQueryInterface(
    INetCfgComponentControl * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    TcpipConfNotifyImpl * This = impl_from_INetCfgComponentControl(iface);
    return INetCfgComponentPropertyUi_QueryInterface((INetCfgComponentPropertyUi*)This, iid, ppvObj);
}

ULONG
STDCALL
INetCfgComponentControl_fnAddRef(
    INetCfgComponentControl * iface)
{
    TcpipConfNotifyImpl * This = impl_from_INetCfgComponentControl(iface);
    return INetCfgComponentPropertyUi_AddRef((INetCfgComponentPropertyUi*)This);
}

ULONG
STDCALL
INetCfgComponentControl_fnRelease(
    INetCfgComponentControl * iface)
{
    TcpipConfNotifyImpl * This = impl_from_INetCfgComponentControl(iface);
    return INetCfgComponentPropertyUi_Release((INetCfgComponentPropertyUi*)This);
}

HRESULT
STDCALL
INetCfgComponentControl_fnInitialize( 
    INetCfgComponentControl * iface,
    INetCfgComponent *pIComp,
    INetCfg *pINetCfg,
    BOOL fInstalling)
{
    TcpipConfNotifyImpl * This = impl_from_INetCfgComponentControl(iface);

    This->pNCfg = pINetCfg;
    This->pNComp = pIComp;

    return S_OK;
}

HRESULT
STDCALL
INetCfgComponentControl_fnApplyRegistryChanges(
    INetCfgComponentControl * iface)
{
    return S_OK;
}

HRESULT
STDCALL
INetCfgComponentControl_fnApplyPnpChanges(
    INetCfgComponentControl * iface,
    INetCfgPnpReconfigCallback *pICallback)
{
    return S_OK;
}

HRESULT
STDCALL
INetCfgComponentControl_fnCancelChanges(
    INetCfgComponentControl * iface)
{
    return S_OK;
}

static const INetCfgComponentControlVtbl vt_NetCfgComponentControl =
{
    INetCfgComponentControl_fnQueryInterface,
    INetCfgComponentControl_fnAddRef,
    INetCfgComponentControl_fnRelease,
    INetCfgComponentControl_fnInitialize,
    INetCfgComponentControl_fnApplyRegistryChanges,
    INetCfgComponentControl_fnApplyPnpChanges,
    INetCfgComponentControl_fnCancelChanges
};

HRESULT
STDCALL
TcpipConfigNotify_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    TcpipConfNotifyImpl *This;

    if (!ppv)
        return E_POINTER;

    This = (TcpipConfNotifyImpl *) CoTaskMemAlloc(sizeof (TcpipConfNotifyImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (const INetCfgComponentPropertyUi*)&vt_NetCfgComponentPropertyUi;
    This->lpVtblCompControl = (const INetCfgComponentControl*)&vt_NetCfgComponentControl;
    This->pNCfg = NULL;
    This->pUnknown = NULL;
    This->pNComp = NULL;

    if (!SUCCEEDED (INetCfgComponentPropertyUi_QueryInterface ((INetCfgComponentPropertyUi*)This, riid, ppv)))
    {
        INetCfgComponentPropertyUi_Release((INetCfgComponentPropertyUi*)This);
        return E_NOINTERFACE;
    }

    INetCfgComponentPropertyUi_Release((INetCfgComponentPropertyUi*)This);
    return S_OK;
}
