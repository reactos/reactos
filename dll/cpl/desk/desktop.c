/*
 * PROJECT:     ReactOS Display Control Panel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Desktop customization property page
 * COPYRIGHT:   Copyright 2018-2022 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "desk.h"

#define FCIDM_DESKBROWSER_REFRESH    0xA220  /* From shresdef.h */

static const TCHAR szHideDesktopIcons[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons");
static const TCHAR szClassicStartMenu[] = TEXT("ClassicStartMenu");
static const TCHAR szNewStartPanel[] = TEXT("NewStartPanel");

struct
{
    LPCTSTR CLSID;
    UINT Checkbox;
} DesktopIcons[NUM_DESKTOP_ICONS] = {
    {TEXT("{450D8FBA-AD25-11D0-98A8-0800361B1103}"), IDC_ICONS_MYDOCS},   /* My Documents */
    {TEXT("{208D2C60-3AEA-1069-A2D7-08002B30309D}"), IDC_ICONS_MYNET},    /* My Network Places */
    {TEXT("{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), IDC_ICONS_MYCOMP},   /* My Computer */
    {TEXT("{871C5380-42A0-1069-A2EA-08002B30309D}"), IDC_ICONS_INTERNET}, /* Internet Browser */
};

VOID
InitDesktopSettings(PDESKTOP_DATA pData)
{
    HKEY regKey, iconKey1, iconKey2;
    UINT i;

    /* Default values */
    for (i = 0; i < _countof(pData->optIcons); i++)
    {
        // pData->optIcons[i].bHideClassic is FALSE by default
        pData->optIcons[i].bHideNewStart = TRUE;
    }

    /* Load desktop icon settings from the registry */
    if (RegOpenKeyEx(HKEY_CURRENT_USER, szHideDesktopIcons,
                     0, KEY_QUERY_VALUE, &regKey) != ERROR_SUCCESS)
    {
        return;
    }

    if (RegOpenKeyEx(regKey, szClassicStartMenu, 0, KEY_QUERY_VALUE, &iconKey1) != ERROR_SUCCESS)
        iconKey1 = NULL;

    if (RegOpenKeyEx(regKey, szNewStartPanel, 0, KEY_QUERY_VALUE, &iconKey2) != ERROR_SUCCESS)
        iconKey2 = NULL;

    for (i = 0; i < _countof(pData->optIcons); i++)
    {
        LSTATUS res;
        DWORD dwType, dwData, cbData;

        if (iconKey1)
        {
            cbData = sizeof(dwData);
            res = RegQueryValueEx(iconKey1, DesktopIcons[i].CLSID, NULL, &dwType, (LPBYTE)&dwData, &cbData);

            if (res == ERROR_SUCCESS && dwType == REG_DWORD && cbData == sizeof(dwData))
                pData->optIcons[i].bHideClassic = !!dwData;
        }

        if (iconKey2)
        {
            cbData = sizeof(dwData);
            res = RegQueryValueEx(iconKey2, DesktopIcons[i].CLSID, NULL, &dwType, (LPBYTE)&dwData, &cbData);

            if (res == ERROR_SUCCESS && dwType == REG_DWORD && cbData == sizeof(dwData))
                pData->optIcons[i].bHideNewStart = !!dwData;
        }
    }

    if (iconKey1)
        RegCloseKey(iconKey1);

    if (iconKey2)
        RegCloseKey(iconKey2);

    RegCloseKey(regKey);
}

BOOL
SaveDesktopSettings(PDESKTOP_DATA pData)
{
    UINT i;

    if (!pData->bLocalSettingsChanged)
        return FALSE;

    for (i = 0; i < _countof(DesktopIcons); i++)
    {
        if (pData->bLocalHideChanged[i])
            pData->bHideChanged[i] = TRUE;
        else
            continue;

        pData->optIcons[i].bHideClassic =
        pData->optIcons[i].bHideNewStart = pData->bLocalHideIcon[i];
    }

    pData->bSettingsChanged = TRUE;
    return TRUE;
}

static BOOL
GetCurrentValue(UINT i, BOOL bNewStart)
{
    HKEY regKey, iconKey;
    LSTATUS res;
    DWORD dwType, cbData;
    BOOL bRet;

    /* Set default value */
    bRet = bNewStart;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szHideDesktopIcons,
                     0, KEY_QUERY_VALUE, &regKey) != ERROR_SUCCESS)
    {
        return bRet;
    }

    if (RegOpenKeyEx(regKey, (bNewStart ? szNewStartPanel : szClassicStartMenu),
                     0, KEY_QUERY_VALUE, &iconKey) != ERROR_SUCCESS)
    {
        RegCloseKey(regKey);
        return bRet;
    }

    cbData = sizeof(bRet);
    res = RegQueryValueEx(iconKey, DesktopIcons[i].CLSID, NULL, &dwType, (LPBYTE)&bRet, &cbData);

    if (res != ERROR_SUCCESS || dwType != REG_DWORD || cbData != sizeof(bRet))
        bRet = bNewStart;

    RegCloseKey(iconKey);
    RegCloseKey(regKey);

    return bRet;
}

static VOID
SetCurrentValue(UINT i, BOOL bNewStart, BOOL bValue)
{
    HKEY regKey, iconKey;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, szHideDesktopIcons,
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,
                       NULL, &regKey, NULL) != ERROR_SUCCESS)
    {
        return;
    }

    if (RegCreateKeyEx(regKey, (bNewStart ? szNewStartPanel : szClassicStartMenu),
                       0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,
                       NULL, &iconKey, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(regKey);
        return;
    }

    RegSetValueEx(iconKey, DesktopIcons[i].CLSID, 0, REG_DWORD, (LPBYTE)&bValue, sizeof(bValue));

    RegCloseKey(iconKey);
    RegCloseKey(regKey);
}

VOID
SetDesktopSettings(PDESKTOP_DATA pData)
{
    UINT i;

    for (i = 0; i < _countof(DesktopIcons); i++)
    {
        if (!pData->bHideChanged[i])
            continue;

        if (GetCurrentValue(i, FALSE) != pData->optIcons[i].bHideClassic)
            SetCurrentValue(i, FALSE, pData->optIcons[i].bHideClassic);

        if (GetCurrentValue(i, TRUE) != pData->optIcons[i].bHideNewStart)
            SetCurrentValue(i, TRUE, pData->optIcons[i].bHideNewStart);

        pData->bHideChanged[i] = FALSE;
    }

    pData->bSettingsChanged = FALSE;

    /* Refresh the desktop */
    PostMessage(GetShellWindow(), WM_COMMAND, FCIDM_DESKBROWSER_REFRESH, 0);
}

static VOID
DesktopOnInitDialog(IN HWND hwndDlg, IN PDESKTOP_DATA pData)
{
    UINT i;
    SHELLSTATE ss = {0};

    SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);

    /* Disable unimplemented features */
    EnableWindow(GetDlgItem(hwndDlg, IDC_ICONS_CHANGEICON), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_ICONS_SETDEFAULT), FALSE);

    for (i = 0; i < _countof(pData->optIcons); i++)
    {
        BOOL bHide;

        if (ss.fStartPanelOn)
            bHide = pData->optIcons[i].bHideNewStart;
        else
            bHide = pData->optIcons[i].bHideClassic;

        CheckDlgButton(hwndDlg,
                       DesktopIcons[i].Checkbox,
                       bHide ? BST_UNCHECKED : BST_CHECKED);

        pData->bLocalHideIcon[i] = bHide;
        pData->bLocalHideChanged[i] = FALSE;
    }

    pData->bLocalSettingsChanged = FALSE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
DesktopPageProc(IN HWND hwndDlg, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam)
{
    PDESKTOP_DATA pData;

    pData = (PDESKTOP_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;
            pData = (PDESKTOP_DATA)ppsp->lParam;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pData);
            DesktopOnInitDialog(hwndDlg, pData);
            break;
        }

        case WM_COMMAND:
        {
            DWORD controlId = LOWORD(wParam);
            DWORD command   = HIWORD(wParam);

            if (command == BN_CLICKED)
            {
                UINT i;

                for (i = 0; i < _countof(DesktopIcons); i++)
                {
                    if (DesktopIcons[i].Checkbox == controlId)
                    {
                        pData->bLocalHideIcon[i] =
                            (IsDlgButtonChecked(hwndDlg, DesktopIcons[i].Checkbox) == BST_UNCHECKED);

                        pData->bLocalSettingsChanged =
                        pData->bLocalHideChanged[i] = TRUE;

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;
                    }
                }
            }
            break;
        }
    }

    return FALSE;
}
