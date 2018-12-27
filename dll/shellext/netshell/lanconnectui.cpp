/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     CNetConnectionPropertyUi: Network connection configuration dialog
 * COPYRIGHT:   Copyright 2008 Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

CNetConnectionPropertyUi::CNetConnectionPropertyUi() :
    m_pProperties(NULL)
{
}

CNetConnectionPropertyUi::~CNetConnectionPropertyUi()
{
    if (m_pNCfg)
        m_pNCfg->Uninitialize();

    if (m_pProperties)
        NcFreeNetconProperties(m_pProperties);
}

VOID
AddItemToListView(HWND hDlgCtrl, PNET_ITEM pItem, LPWSTR szName, BOOL bChecked)
{
    LVITEMW lvItem;

    ZeroMemory(&lvItem, sizeof(lvItem));
    lvItem.mask  = LVIF_TEXT | LVIF_PARAM;
    lvItem.pszText = szName;
    lvItem.lParam = (LPARAM)pItem;
    lvItem.iItem = ListView_GetItemCount(hDlgCtrl);
    lvItem.iItem = SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);
    ListView_SetCheckState(hDlgCtrl, lvItem.iItem, bChecked);
}

BOOL
CNetConnectionPropertyUi::GetINetCfgComponent(INetCfg *pNCfg, INetCfgComponent ** pOut)
{
    LPWSTR pName;
    HRESULT hr;
    ULONG Fetched;
    CComPtr<IEnumNetCfgComponent> pEnumCfg;

    hr = pNCfg->EnumComponents(&GUID_DEVCLASS_NET, &pEnumCfg);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    while (TRUE)
    {
        CComPtr<INetCfgComponent> pNCg;
        hr = pEnumCfg->Next(1, &pNCg, &Fetched);
        if (hr != S_OK)
            break;

        hr = pNCg->GetDisplayName(&pName);
        if (SUCCEEDED(hr))
        {
            if (!_wcsicmp(pName, m_pProperties->pszwDeviceName))
            {
                *pOut = pNCg.Detach();
                return TRUE;
            }
            CoTaskMemFree(pName);
        }
    }
    return FALSE;
}

VOID
CNetConnectionPropertyUi::EnumComponents(HWND hDlgCtrl, INetCfg *pNCfg, const GUID *CompGuid, UINT Type)
{
    HRESULT hr;
    CComPtr<IEnumNetCfgComponent> pENetCfg;
    ULONG Num;
    DWORD dwCharacteristics;
    LPOLESTR pDisplayName, pHelpText;
    PNET_ITEM pItem;
    BOOL bChecked;

    hr = pNCfg->EnumComponents(CompGuid, &pENetCfg);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    while (TRUE)
    {
        CComPtr<INetCfgComponent> pNCfgComp;
        CComPtr<INetCfgComponentBindings> pCompBind;
        CComPtr<INetCfgComponent> pAdapterCfgComp;

        hr = pENetCfg->Next(1, &pNCfgComp, &Num);
        if (hr != S_OK)
            break;

        hr = pNCfgComp->GetCharacteristics(&dwCharacteristics);
        if (SUCCEEDED(hr) && (dwCharacteristics & NCF_HIDDEN))
            continue;

        pDisplayName = NULL;
        pHelpText = NULL;
        hr = pNCfgComp->GetDisplayName(&pDisplayName);
        hr = pNCfgComp->GetHelpText(&pHelpText);
        bChecked = TRUE; //ReactOS hack
        hr = pNCfgComp->QueryInterface(IID_PPV_ARG(INetCfgComponentBindings, &pCompBind));
        if (SUCCEEDED(hr))
        {
            if (GetINetCfgComponent(pNCfg, &pAdapterCfgComp))
            {
                hr = pCompBind->IsBoundTo(pAdapterCfgComp);
                if (hr == S_OK)
                    bChecked = TRUE;
                else
                    bChecked = FALSE;
            }
        }

        pItem = static_cast<NET_ITEM*>(CoTaskMemAlloc(sizeof(NET_ITEM)));
        if (!pItem)
            continue;

        pItem->dwCharacteristics = dwCharacteristics;
        pItem->szHelp = pHelpText;
        pItem->Type = (NET_TYPE)Type;
        pItem->pNCfgComp = pNCfgComp.Detach();
        pItem->NumPropDialogOpen = 0;

        AddItemToListView(hDlgCtrl, pItem, pDisplayName, bChecked);
        CoTaskMemFree(pDisplayName);
    }
}

VOID
CNetConnectionPropertyUi::InitializeLANPropertiesUIDlg(HWND hwndDlg)
{
    HRESULT hr;
    CComPtr<INetCfg> pNCfg;
    CComPtr<INetCfgLock> pNCfgLock;
    HWND hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMPONENTSLIST);
    LVCOLUMNW lc;
    RECT rc;
    DWORD dwStyle;
    LPWSTR pDisplayName;
    LVITEMW li;

    SendDlgItemMessageW(hwndDlg, IDC_NETCARDNAME, WM_SETTEXT, 0, (LPARAM)m_pProperties->pszwDeviceName);
    if (m_pProperties->dwCharacter & NCCF_SHOW_ICON)
    {
        /* check show item on taskbar*/
        SendDlgItemMessageW(hwndDlg, IDC_SHOWTASKBAR, BM_SETCHECK, BST_CHECKED, 0);
    }
    if (m_pProperties->dwCharacter & NCCF_NOTIFY_DISCONNECTED)
    {
        /* check notify item */
        SendDlgItemMessageW(hwndDlg, IDC_NOTIFYNOCONNECTION, BM_SETCHECK, BST_CHECKED, 0);
    }

    memset(&lc, 0, sizeof(LV_COLUMN));
    lc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    lc.fmt = LVCFMT_FIXED_WIDTH;
    if (GetClientRect(hDlgCtrl, &rc))
    {
        lc.mask |= LVCF_WIDTH;
        lc.cx = rc.right - rc.left;
    }
    lc.pszText = (LPWSTR)L"";
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 0, (LPARAM)&lc);
    dwStyle = (DWORD) SendMessage(hDlgCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    dwStyle = dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
    SendMessage(hDlgCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);

    hr = CoCreateInstance(CLSID_CNetCfg, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(INetCfg, &pNCfg));
    if (FAILED(hr))
        return;

    hr = pNCfg->QueryInterface(IID_PPV_ARG(INetCfgLock, &pNCfgLock));
    hr = pNCfgLock->AcquireWriteLock(100, L"", &pDisplayName);
    if (hr == S_FALSE)
    {
        CoTaskMemFree(pDisplayName);
        return;
    }

    hr = pNCfg->Initialize(NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    m_pNCfg = pNCfg;
    m_NCfgLock = pNCfgLock;

    EnumComponents(hDlgCtrl, pNCfg, &GUID_DEVCLASS_NETCLIENT, NET_TYPE_CLIENT);
    EnumComponents(hDlgCtrl, pNCfg, &GUID_DEVCLASS_NETSERVICE, NET_TYPE_SERVICE);
    EnumComponents(hDlgCtrl, pNCfg, &GUID_DEVCLASS_NETTRANS, NET_TYPE_PROTOCOL);

    ZeroMemory(&li, sizeof(li));
    li.mask = LVIF_STATE;
    li.stateMask = (UINT)-1;
    li.state = LVIS_FOCUSED|LVIS_SELECTED;
    (void)SendMessageW(hDlgCtrl, LVM_SETITEMW, 0, (LPARAM)&li);
}

VOID
CNetConnectionPropertyUi::ShowNetworkComponentProperties(HWND hwndDlg)
{
    LVITEMW lvItem;
    HWND hDlgCtrl;
    UINT Index, Count;
    HRESULT hr;
    INetCfgComponent *pNCfgComp;
    PNET_ITEM pItem;

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMPONENTSLIST);
    Count = ListView_GetItemCount(hDlgCtrl);
    if (!Count)
        return;

    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_PARAM | LVIF_STATE;
    lvItem.stateMask = (UINT)-1;
    for (Index = 0; Index < Count; Index++)
    {
        lvItem.iItem = Index;
        if (SendMessageW(hDlgCtrl, LVM_GETITEMW, 0, (LPARAM)&lvItem))
        {
            if (lvItem.state & LVIS_SELECTED)
                break;
        }
    }

    if (!(lvItem.state & LVIS_SELECTED))
    {
        return;
    }

    pItem = (PNET_ITEM)lvItem.lParam;
    pNCfgComp = (INetCfgComponent*)pItem->pNCfgComp;
    hr = pNCfgComp->RaisePropertyUi(GetParent(hwndDlg), NCRP_QUERY_PROPERTY_UI, (INetConnectionConnectUi*)this);
    if (SUCCEEDED(hr))
    {
        hr = pNCfgComp->RaisePropertyUi(GetParent(hwndDlg), NCRP_SHOW_PROPERTY_UI, (INetConnectionConnectUi*)this);
    }
}

INT_PTR
CALLBACK
CNetConnectionPropertyUi::LANPropertiesUIDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PROPSHEETPAGE *page;
    LPNMLISTVIEW lppl;
    LVITEMW li;
    PNET_ITEM pItem;
    CNetConnectionPropertyUi * This;
    LPPSHNOTIFY lppsn;
    DWORD dwShowIcon, dwNotifyDisconnect;
    HRESULT hr;
    WCHAR szKey[200];
    LPOLESTR pStr;
    HKEY hKey;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            page = (PROPSHEETPAGE*)lParam;
            This = (CNetConnectionPropertyUi*)page->lParam;
            This->InitializeLANPropertiesUIDlg(hwndDlg);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)This);
            return TRUE;
        case WM_NOTIFY:
            lppl = (LPNMLISTVIEW) lParam;
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                This = (CNetConnectionPropertyUi*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (This->m_pNCfg)
                {
                    hr = This->m_pNCfg->Apply();
                    if (FAILED(hr))
                        return PSNRET_INVALID;
                }

                if (SendDlgItemMessageW(hwndDlg, IDC_SHOWTASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    dwShowIcon = 1;
                else
                    dwShowIcon = 0;

                if (SendDlgItemMessageW(hwndDlg, IDC_NOTIFYNOCONNECTION, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    dwNotifyDisconnect = 1;
                else
                    dwNotifyDisconnect = 0;

                //NOTE: Windows write these setting with the undocumented INetLanConnection::SetInfo
                if (StringFromCLSID((CLSID)This->m_pProperties->guidId, &pStr) == ERROR_SUCCESS)
                {
                    swprintf(szKey, L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%s\\Connection", pStr);
                    CoTaskMemFree(pStr);
                    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
                    {
                        RegSetValueExW(hKey, L"ShowIcon", 0, REG_DWORD, (LPBYTE)&dwShowIcon, sizeof(DWORD));
                        RegSetValueExW(hKey, L"IpCheckingEnabled", 0, REG_DWORD, (LPBYTE)&dwNotifyDisconnect, sizeof(DWORD));
                        RegCloseKey(hKey);
                    }
                }

                return PSNRET_NOERROR;
            }
#if 0
            else if (lppsn->hdr.code == PSN_CANCEL)
            {
                This = (CNetConnectionPropertyUi*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                if (This->m_pNCfg)
                {
                    hr = This->m_pNCfg->Cancel();
                    if (SUCCEEDED(hr))
                        return PSNRET_NOERROR;
                    else
                        return PSNRET_INVALID;
                }
                return PSNRET_NOERROR;
            }
#endif
            if (lppl->hdr.code == LVN_ITEMCHANGING)
            {
                ZeroMemory(&li, sizeof(li));
                li.mask = LVIF_PARAM;
                li.iItem = lppl->iItem;
                if (!SendMessageW(lppl->hdr.hwndFrom, LVM_GETITEMW, 0, (LPARAM)&li))
                    return TRUE;

                pItem = (PNET_ITEM)li.lParam;
                if (!pItem)
                    return TRUE;

                if (!(lppl->uOldState & LVIS_FOCUSED) && (lppl->uNewState & LVIS_FOCUSED))
                {
                    /* new focused item */
                    if (pItem->dwCharacteristics & NCF_NOT_USER_REMOVABLE)
                        EnableWindow(GetDlgItem(hwndDlg, IDC_UNINSTALL), FALSE);
                    else
                        EnableWindow(GetDlgItem(hwndDlg, IDC_UNINSTALL), TRUE);

                    if (pItem->dwCharacteristics & NCF_HAS_UI)
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTIES), TRUE);
                    else
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTIES), FALSE);

                    SendDlgItemMessageW(hwndDlg, IDC_DESCRIPTION, WM_SETTEXT, 0, (LPARAM)pItem->szHelp);
                }
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_PROPERTIES)
            {
                This = (CNetConnectionPropertyUi*) GetWindowLongPtr(hwndDlg, DWLP_USER);
                This->ShowNetworkComponentProperties(hwndDlg);
                return FALSE;
            }
            else if (LOWORD(wParam) == IDC_CONFIGURE)
            {
                LPOLESTR DeviceInstanceID;
                This = (CNetConnectionPropertyUi*)GetWindowLongPtr(hwndDlg, DWLP_USER);

                if (This->GetDeviceInstanceID(&DeviceInstanceID))
                {
                    WCHAR wszCmd[2*MAX_PATH];
                    StringCbPrintfW(wszCmd, sizeof(wszCmd), L"rundll32.exe devmgr.dll,DeviceProperties_RunDLL /DeviceID %s", DeviceInstanceID);
                    CoTaskMemFree(DeviceInstanceID);

                    STARTUPINFOW si;
                    PROCESS_INFORMATION pi;
                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);
                    if (!CreateProcessW(NULL, wszCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                        break;

                   CloseHandle(pi.hProcess);
                   CloseHandle(pi.hThread);
                }
            }
            break;
    }
    return FALSE;
}

BOOL
CNetConnectionPropertyUi::GetDeviceInstanceID(
    OUT LPOLESTR *DeviceInstanceID)
{
    LPOLESTR pStr, pResult;
    HKEY hKey;
    DWORD dwInstanceID;
    WCHAR szKeyName[2*MAX_PATH];
    WCHAR szInstanceID[2*MAX_PATH];

    if (StringFromCLSID(m_pProperties->guidId, &pStr) != ERROR_SUCCESS)
    {
        // failed to convert guid to string
        return FALSE;
    }

    StringCbPrintfW(szKeyName, sizeof(szKeyName), L"SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%s\\Connection", pStr);
    CoTaskMemFree(pStr);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_READ, &hKey) != ERROR_SUCCESS) 
    {
        // failed to open key
        return FALSE;
    }
    
    dwInstanceID = sizeof(szInstanceID);
    if (RegGetValueW(hKey, NULL, L"PnpInstanceId", RRF_RT_REG_SZ, NULL, (PVOID)szInstanceID, &dwInstanceID) == ERROR_SUCCESS)
    {
        szInstanceID[MAX_PATH-1] = L'\0';
        pResult = static_cast<LPOLESTR>(CoTaskMemAlloc((wcslen(szInstanceID) + 1) * sizeof(WCHAR)));
        if (pResult != 0)
        {
            wcscpy(pResult, szInstanceID);
            *DeviceInstanceID = pResult;
            RegCloseKey(hKey);
            return TRUE;
        }
    }
    RegCloseKey(hKey);
    return FALSE;
}

HRESULT
WINAPI
CNetConnectionPropertyUi::AddPages(
    HWND hwndParent,
    LPFNADDPROPSHEETPAGE pfnAddPage,
    LPARAM lParam)
{
    HPROPSHEETPAGE hProp;
    BOOL ret;
    HRESULT hr = E_FAIL;
    INITCOMMONCONTROLSEX initEx;

    initEx.dwSize = sizeof(initEx);
    initEx.dwICC = ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&initEx))
        return E_FAIL;

    hr = m_pCon->GetProperties(&m_pProperties);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hProp = InitializePropertySheetPage(MAKEINTRESOURCEW(IDD_NETPROPERTIES), LANPropertiesUIDlg, reinterpret_cast<LPARAM>(this), NULL);
    if (hProp)
    {
        ret = (*pfnAddPage)(hProp, lParam);
        if (ret)
        {
            hr = NOERROR;
        }
        else
        {
            DestroyPropertySheetPage(hProp);
        }
    }
    return hr;
}

HRESULT
WINAPI
CNetConnectionPropertyUi::GetIcon(
    DWORD dwSize,
    HICON *phIcon)
{
    return E_NOTIMPL;
}

HRESULT
WINAPI
CNetConnectionPropertyUi::GetDeviceGuid(GUID *pGuid)
{
    CopyMemory(pGuid, &m_pProperties->guidId, sizeof(GUID));
    return S_OK;
}

HRESULT
WINAPI
CNetConnectionPropertyUi::SetConnection(INetConnection* pCon)
{
    if (!pCon)
        return E_POINTER;

    m_pCon = pCon;
    return S_OK;
}

HRESULT
WINAPI
CNetConnectionPropertyUi::Connect(
    HWND hwndParent,
    DWORD dwFlags)
{
    if (!m_pCon)
        return E_POINTER; //FIXME

    if (dwFlags & NCUC_NO_UI)
        return m_pCon->Connect();

    return E_FAIL;
}

HRESULT
WINAPI
CNetConnectionPropertyUi::Disconnect(
    HWND hwndParent,
    DWORD dwFlags)
{
    WCHAR szBuffer[100];
    swprintf(szBuffer, L"INetConnectionConnectUi_fnDisconnect flags %x\n", dwFlags);
    MessageBoxW(NULL, szBuffer, NULL, MB_OK);

    return S_OK;
}
