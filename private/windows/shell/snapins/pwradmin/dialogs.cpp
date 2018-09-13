#include "main.h"

BOOL bSystemDlgIsUp = FALSE;
HWND hSystemDlg;
BOOL bMonitorDlgIsUp = FALSE;
HWND hMonitorDlg;

#define POWER_CFG_KEY "Control Panel\\PowerCfg"
#define DISPLAY_VALUE "AdminMaxVideoTimeout"
#define SLEEP_VALUE   "AdminMaxSleep"

void CSnapIn::DisplayPropertyPage(long ItemId)
{
    if (ItemId == ITEM_SYSTEM)
    {
        if (!bSystemDlgIsUp)
        {
            CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_SYSTEM),
                              m_pcd->m_hwndFrame, SystemDlgProc,
                              (LPARAM) m_pcd->m_pGPTInformation);
        }
        else
        {
            BringWindowToTop(hSystemDlg);
        }
    }
    else if (ItemId == ITEM_MONITOR)
    {
        if (!bMonitorDlgIsUp)
        {
        CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_MONITOR),
                          m_pcd->m_hwndFrame, MonitorDlgProc,
                          (LPARAM) m_pcd->m_pGPTInformation);
        }
        else
        {
            BringWindowToTop(hMonitorDlg);
        }
    }
}

VOID
SetPowerConfig(
    HWND hWnd,
    LPTSTR ValueName,
    ULONG dwValue
    )
{
    HKEY PowerCfgKey;
    LONG Rc;
    ULONG dwDisposition;
    HKEY hGptKey;
    HRESULT hr = E_NOINTERFACE;
    LPGPEINFORMATION pGptInformation = (LPGPEINFORMATION)GetWindowLongPtr (hWnd, DWLP_USER);

    hr = pGptInformation->GetRegistryKey(GPO_SECTION_USER, &hGptKey);
    if (FAILED(hr))     {
        return;
    }

    Rc = RegCreateKeyEx(hGptKey,
                        TEXT(POWER_CFG_KEY),
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &PowerCfgKey,
                        &dwDisposition);


    if (Rc == ERROR_SUCCESS) {
        Rc = RegSetValueEx(PowerCfgKey,
                           ValueName,
                           0,
                           REG_DWORD,
                           (CONST BYTE *)&dwValue,
                           sizeof(ULONG));

        CloseHandle(PowerCfgKey);
    }

    if (Rc == ERROR_SUCCESS) {
        GUID guidRegistryExt = REGISTRY_EXTENSION_GUID;
        GUID guidSnapin = CLSID_SnapIn;

        pGptInformation->PolicyChanged(FALSE, TRUE, &guidRegistryExt, &guidSnapin);

    } else {
        TCHAR szName[256];
        TCHAR szMsg[256];
        LoadString(g_hInstance, IDS_DESCRIPTION, szName, sizeof(szName));
        LoadString(g_hInstance, IDS_ERR_REG, szMsg, sizeof(szName));

        MessageBox(hWnd, szMsg, szName, MB_OK | MB_ICONEXCLAMATION);
    }

    CloseHandle(hGptKey);
}

BOOL
GetPowerConfig(
    HWND hWnd,
    LPTSTR ValueName,
    PULONG pValue
    )
{
    HKEY PowerCfgKey;
    BOOL bSuccess = FALSE;
    ULONG dwType;
    ULONG dwSize;
    HKEY hGptKey;
    HRESULT hr = E_NOINTERFACE;
    LPGPEINFORMATION pGptInformation = (LPGPEINFORMATION)GetWindowLongPtr (hWnd, DWLP_USER);

    hr = pGptInformation->GetRegistryKey(GPO_SECTION_USER, &hGptKey);
    if (FAILED(hr))     {
        return FALSE;
    }


    if (ERROR_SUCCESS == RegOpenKeyEx(hGptKey,
                                     TEXT(POWER_CFG_KEY),
                                     0,
                                     KEY_QUERY_VALUE,
                                     &PowerCfgKey)) {

        dwSize = sizeof(ULONG);
        if (ERROR_SUCCESS == RegQueryValueEx(PowerCfgKey,
                                             ValueName,
                                             NULL,
                                             &dwType,
                                             (LPBYTE)pValue,
                                             &dwSize)) {
            bSuccess = TRUE;
        }

        CloseHandle(PowerCfgKey);
    }

    CloseHandle(hGptKey);
    return bSuccess;
}

INT_PTR CALLBACK CSnapIn::SystemDlgProc
(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    ULONG dwValue;
    BOOL bSuccess;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            bSystemDlgIsUp = TRUE;
            hSystemDlg = hWnd;

            SetWindowLongPtr (hWnd, DWLP_USER, (LONG_PTR) lParam);
            if (GetPowerConfig(hWnd, TEXT(SLEEP_VALUE), &dwValue)) {
                CheckDlgButton(hWnd, IDC_PREVENTPOWEROFF,
                               dwValue ? BST_CHECKED : BST_UNCHECKED);
            }
            return TRUE;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    dwValue = (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_PREVENTPOWEROFF)) ? 1 : 0;
                    SetPowerConfig(hWnd, TEXT(SLEEP_VALUE), dwValue);
                    DestroyWindow(hWnd);
                    break;

                case IDCANCEL:
                    DestroyWindow(hWnd);
                    break;
            }
            break;

        case WM_DESTROY:
            bSystemDlgIsUp = FALSE;
            break;
    }
    return FALSE;
}


INT_PTR CALLBACK CSnapIn::MonitorDlgProc
(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    ULONG dwValue;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            bMonitorDlgIsUp = TRUE;
            hMonitorDlg = hWnd;
            SetWindowLongPtr (hWnd, DWLP_USER, (LONG_PTR) lParam);

            SendDlgItemMessage(hWnd,IDC_SHUTOFF_ARROWS,UDM_SETRANGE,(DWORD)0,(LPARAM)MAKELONG((short) 300, (short) 1));

            SetDlgItemInt(hWnd, IDC_SHUTOFF_TIME, 15, FALSE);
            if (GetPowerConfig(hWnd, TEXT(DISPLAY_VALUE), &dwValue)) {
                if (dwValue != (ULONG) -1) {
                    CheckDlgButton(hWnd, IDC_SHUTOFF, BST_CHECKED);
                    SetDlgItemInt(hWnd, IDC_SHUTOFF_TIME, dwValue, FALSE);
                }
            }

            return TRUE;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:

                    dwValue = (ULONG) -1;
                    if (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_SHUTOFF)) {
                        BOOL bSuccess;
                        dwValue = GetDlgItemInt(hWnd, IDC_SHUTOFF_TIME, &bSuccess, FALSE);
                        if (!bSuccess) {
                            dwValue = (ULONG) -1;
                        }
                    }

                    SetPowerConfig(hWnd, TEXT(DISPLAY_VALUE), dwValue);
                    DestroyWindow(hWnd);
                    break;

                case IDCANCEL:
                    DestroyWindow(hWnd);
                    break;
            }
            break;

        case WM_DESTROY:
            bMonitorDlgIsUp = FALSE;
            break;
    }
    return FALSE;
}
