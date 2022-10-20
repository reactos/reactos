/*
 * PROJECT:     ReactOS Display Control Panel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Desktop customization property page
 * COPYRIGHT:   Copyright 2018-2022 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "desk.h"

#include <shlwapi.h>
#include <shellapi.h>

/* From shresdef.h */
#define FCIDM_DESKBROWSER_REFRESH    0xA220

#define IDS_TITLE_MYCOMP  30386
#define IDS_TITLE_MYNET   30387
#define IDS_TITLE_BIN_1   30388
#define IDS_TITLE_BIN_0   30389

/* Workaround:
 * There's no special fallback icon title string
 * for My Documents in shell32.dll, so use IDS_PERSONAL.
 *
 * Windows does this in some different way.
 */
#define IDS_PERSONAL  9227

static const TCHAR szHideDesktopIcons[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\");
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

static const TCHAR szUserClass[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\");
static const TCHAR szSysClass[] = TEXT("CLSID\\");
static const TCHAR szDefaultIcon[] = TEXT("\\DefaultIcon");
static const TCHAR szFallbackIcon[] = TEXT("%SystemRoot%\\system32\\shell32.dll,0");

struct
{
    LPCTSTR CLSID;
    UINT TitleId;
    LPCTSTR IconName;
} IconChange[NUM_CHANGE_ICONS] = {
    {TEXT("{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), IDS_TITLE_MYCOMP, NULL},         /* My Computer */
    {TEXT("{450D8FBA-AD25-11D0-98A8-0800361B1103}"), IDS_PERSONAL, NULL},             /* My Documents */
    {TEXT("{208D2C60-3AEA-1069-A2D7-08002B30309D}"), IDS_TITLE_MYNET, NULL},          /* My Network Places */
    {TEXT("{645FF040-5081-101B-9F08-00AA002F954E}"), IDS_TITLE_BIN_1, TEXT("Full")},  /* Recycle Bin (full) */
    {TEXT("{645FF040-5081-101B-9F08-00AA002F954E}"), IDS_TITLE_BIN_0, TEXT("Empty")}, /* Recycle Bin (empty) */
};

VOID
InitDesktopSettings(PDESKTOP_DATA pData)
{
    UINT i;
    TCHAR regPath[MAX_PATH];

    /* Load desktop icon settings from the registry */
    StringCchCopy(regPath, _countof(regPath), szHideDesktopIcons);
    StringCchCat(regPath, _countof(regPath), szClassicStartMenu);

    for (i = 0; i < _countof(pData->optIcons); i++)
    {
        pData->optIcons[i].bHideClassic = SHRegGetBoolUSValue(regPath, DesktopIcons[i].CLSID, FALSE, FALSE);
    }

    StringCchCopy(regPath, _countof(regPath), szHideDesktopIcons);
    StringCchCat(regPath, _countof(regPath), szNewStartPanel);

    for (i = 0; i < _countof(pData->optIcons); i++)
    {
        pData->optIcons[i].bHideNewStart = SHRegGetBoolUSValue(regPath, DesktopIcons[i].CLSID, FALSE, TRUE);
    }

    for (i = 0; i < _countof(IconChange); i++)
    {
        DWORD cbData, dwType;
        TCHAR szData[MAX_PATH];

        /* Current icons */
        StringCchCopy(regPath, _countof(regPath), szUserClass);
        StringCchCat(regPath, _countof(regPath), IconChange[i].CLSID);
        StringCchCat(regPath, _countof(regPath), szDefaultIcon);
        cbData = sizeof(szData);

        if (SHGetValue(HKEY_CURRENT_USER, regPath, IconChange[i].IconName, &dwType,
                       &szData, &cbData) == ERROR_SUCCESS &&
            (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
        {
            StringCchCopy(pData->Icon[i].szPath, _countof(pData->Icon[i].szPath), szData);
        }

        /* Default icons */
        /* FIXME: Get default icons from theme data, fallback to CLSID data on error. */
        StringCchCopy(regPath, _countof(regPath), szSysClass);
        StringCchCat(regPath, _countof(regPath), IconChange[i].CLSID);
        StringCchCat(regPath, _countof(regPath), szDefaultIcon);
        cbData = sizeof(szData);

        if (SHGetValue(HKEY_CLASSES_ROOT, regPath, IconChange[i].IconName, &dwType,
                       &szData, &cbData) == ERROR_SUCCESS &&
            (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
        {
            StringCchCopy(pData->DefIcon[i].szPath, _countof(pData->DefIcon[i].szPath), szData);
        }

        /* Emergency fallback */
        if (lstrlen(pData->DefIcon[i].szPath) == 0)
            StringCchCopy(pData->DefIcon[i].szPath, _countof(pData->DefIcon[i].szPath), szFallbackIcon);

        if (lstrlen(pData->Icon[i].szPath) == 0)
            StringCchCopy(pData->Icon[i].szPath, _countof(pData->Icon[i].szPath), pData->DefIcon[i].szPath);
    }
}

BOOL
SaveDesktopSettings(PDESKTOP_DATA pData)
{
    UINT i;

    if (!pData->bLocalSettingsChanged)
        return FALSE;

    for (i = 0; i < _countof(DesktopIcons); i++)
    {
        if (!pData->bLocalHideChanged[i])
            continue;

        pData->optIcons[i].bHideClassic =
        pData->optIcons[i].bHideNewStart = pData->bLocalHideIcon[i];
        pData->bHideChanged[i] = TRUE;
    }

    for (i = 0; i < _countof(IconChange); i++)
    {
        if (!pData->bLocalIconChanged[i])
            continue;

        StringCchCopy(pData->Icon[i].szPath, _countof(pData->Icon[i].szPath), pData->LocalIcon[i].szPath);
        pData->bIconChanged[i] = TRUE;
    }

    pData->bSettingsChanged = TRUE;
    return TRUE;
}

static BOOL
GetCurrentValue(UINT i, BOOL bNewStart)
{
    TCHAR regPath[MAX_PATH];

    StringCchCopy(regPath, _countof(regPath), szHideDesktopIcons);
    StringCchCat(regPath, _countof(regPath), bNewStart ? szNewStartPanel : szClassicStartMenu);

    return SHRegGetBoolUSValue(regPath, DesktopIcons[i].CLSID, FALSE, bNewStart);
}

static VOID
SetCurrentValue(UINT i, BOOL bNewStart, BOOL bValue)
{
    TCHAR regPath[MAX_PATH];

    StringCchCopy(regPath, _countof(regPath), szHideDesktopIcons);
    StringCchCat(regPath, _countof(regPath), bNewStart ? szNewStartPanel : szClassicStartMenu);

    SHSetValue(HKEY_CURRENT_USER, regPath, DesktopIcons[i].CLSID, REG_DWORD,
               (LPBYTE)&bValue, sizeof(bValue));
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

    for (i = 0; i < _countof(IconChange); i++)
    {
        TCHAR iconPath[MAX_PATH];
        DWORD dwType = (pData->Icon[i].szPath[0] == TEXT('%') ? REG_EXPAND_SZ : REG_SZ);

        if (!pData->bIconChanged[i])
            continue;

        StringCchCopy(iconPath, _countof(iconPath), szUserClass);
        StringCchCat(iconPath, _countof(iconPath), IconChange[i].CLSID);
        StringCchCat(iconPath, _countof(iconPath), szDefaultIcon);

        SHSetValue(HKEY_CURRENT_USER, iconPath, IconChange[i].IconName, dwType,
                   pData->Icon[i].szPath, sizeof(pData->Icon[i].szPath));
        if (IconChange[i].TitleId == IDS_TITLE_BIN_0)
        {
            /* Also apply to the root value */
            SHSetValue(HKEY_CURRENT_USER, iconPath, NULL, dwType,
                       pData->Icon[i].szPath, sizeof(pData->Icon[i].szPath));
        }
        pData->bIconChanged[i] = FALSE;
    }

    pData->bSettingsChanged = FALSE;

    /* Refresh the desktop */
    PostMessage(GetShellWindow(), WM_COMMAND, FCIDM_DESKBROWSER_REFRESH, 0);
}

static HICON
GetIconFromLocation(LPTSTR szIconPath)
{
    INT iIndex;
    TCHAR szPath[MAX_PATH];

    ExpandEnvironmentStrings(szIconPath, szPath, _countof(szPath));
    iIndex = PathParseIconLocation(szPath);
    return ExtractIcon(hApplet, szPath, iIndex);
}

static VOID
DesktopOnInitDialog(IN HWND hwndDlg, IN PDESKTOP_DATA pData)
{
    UINT i;
    SHELLSTATE ss = {0};
    HWND hwndList;

    SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);

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

    pData->iLocalCurIcon = 0;
    hwndList = GetDlgItem(hwndDlg, IDC_ICONS_LISTVIEW);
    pData->hLocalImageList = ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), ILC_COLOR32 | ILC_MASK, 1, 1);
    ListView_SetImageList(hwndList, pData->hLocalImageList, LVSIL_NORMAL);

    for (i = 0; i < _countof(IconChange); i++)
    {
        TCHAR szClassPath[MAX_PATH];
        DWORD dwType, cbData;
        LVITEM lvitem = {0};
        HICON hIcon;

        StringCchCopy(pData->LocalIcon[i].szPath, _countof(pData->LocalIcon[i].szPath), pData->Icon[i].szPath);
        pData->bLocalIconChanged[i] = FALSE;

        /* Try loading user-defined desktop icon title */
        StringCchCopy(szClassPath, _countof(szClassPath), szUserClass);
        StringCchCat(szClassPath, _countof(szClassPath), IconChange[i].CLSID);
        cbData = sizeof(pData->LocalIcon[i].szTitle);

        if (SHGetValue(HKEY_CURRENT_USER, szClassPath, IconChange[i].IconName, &dwType,
                       pData->LocalIcon[i].szTitle, &cbData) != ERROR_SUCCESS || dwType != REG_SZ)
        {
            /* Try loading system-defined class icon title */
            StringCchCopy(szClassPath, _countof(szClassPath), szSysClass);
            StringCchCat(szClassPath, _countof(szClassPath), IconChange[i].CLSID);
            cbData = sizeof(pData->LocalIcon[i].szTitle);

            if (SHGetValue(HKEY_CLASSES_ROOT, szClassPath, IconChange[i].IconName, &dwType,
                           pData->LocalIcon[i].szTitle, &cbData) != ERROR_SUCCESS || dwType != REG_SZ)
            {
                /* Fallback to predefined strings */
                LoadString(GetModuleHandle(TEXT("shell32.dll")),
                           IconChange[i].TitleId,
                           pData->LocalIcon[i].szTitle,
                           _countof(pData->LocalIcon[i].szTitle));
            }
        }

        hIcon = GetIconFromLocation(pData->LocalIcon[i].szPath);

        lvitem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        lvitem.iItem = i;
        lvitem.iSubItem = 0;
        lvitem.pszText = pData->LocalIcon[i].szTitle;
        lvitem.lParam = (LPARAM)i;

        if (hIcon)
        {
            if (pData->hLocalImageList)
                lvitem.iImage = ImageList_AddIcon(pData->hLocalImageList, hIcon);
            DestroyIcon(hIcon);
        }

        if (ListView_InsertItem(hwndList, &lvitem) < 0)
            continue;

        if (i > 0)
            continue;

        lvitem.state = LVIS_FOCUSED | LVIS_SELECTED;
        lvitem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        SendMessage(hwndList, LVM_SETITEMSTATE, 0, (LPARAM)&lvitem);
    }

    pData->bLocalSettingsChanged = FALSE;
}

static VOID
DesktopOnDestroyDialog(IN HWND hwndDlg, IN PDESKTOP_DATA pData)
{
    if (pData->hLocalImageList)
    {
        ListView_SetImageList(GetDlgItem(hwndDlg, IDC_ICONS_LISTVIEW), NULL, LVSIL_NORMAL);
        ImageList_Destroy(pData->hLocalImageList);
    }

    SetWindowLongPtr(hwndDlg, DWLP_USER, 0);
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

        case WM_DESTROY:
        {
            DesktopOnDestroyDialog(hwndDlg, pData);
            break;
        }

        case WM_COMMAND:
        {
            DWORD controlId = LOWORD(wParam);
            DWORD command   = HIWORD(wParam);

            if (command == BN_CLICKED)
            {
                UINT i;
                BOOL bUpdateIcon = FALSE;

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

                if (controlId == IDC_ICONS_CHANGEICON)
                {
                    TCHAR szPath[MAX_PATH];
                    INT iIndex;
                    i = pData->iLocalCurIcon;

                    ExpandEnvironmentStrings(pData->LocalIcon[i].szPath, szPath, _countof(szPath));
                    iIndex = PathParseIconLocation(szPath);

                    if (PickIconDlg(hwndDlg, szPath, _countof(szPath), &iIndex))
                    {
                        StringCchCopy(pData->LocalIcon[i].szPath, _countof(pData->LocalIcon[i].szPath), szPath);
                        PathUnExpandEnvStrings(pData->LocalIcon[i].szPath, szPath, _countof(szPath));

                        StringCchPrintf(pData->LocalIcon[i].szPath, _countof(pData->LocalIcon[i].szPath), TEXT("%s,%d"), szPath, iIndex);
                        bUpdateIcon = TRUE;
                    }
                }
                else if (controlId == IDC_ICONS_SETDEFAULT)
                {
                    i = pData->iLocalCurIcon;

                    StringCchCopy(pData->LocalIcon[i].szPath, _countof(pData->LocalIcon[i].szPath), pData->DefIcon[i].szPath);
                    bUpdateIcon = TRUE;
                }

                if (bUpdateIcon)
                {
                    HWND hwndList = GetDlgItem(hwndDlg, IDC_ICONS_LISTVIEW);
                    HICON hIcon;

                    hIcon = GetIconFromLocation(pData->LocalIcon[i].szPath);

                    if (hIcon)
                    {
                        if (pData->hLocalImageList)
                            ImageList_ReplaceIcon(pData->hLocalImageList, i, hIcon);
                        DestroyIcon(hIcon);
                    }

                    pData->bLocalSettingsChanged =
                    pData->bLocalIconChanged[i] = TRUE;

                    InvalidateRect(hwndList, NULL, TRUE);
                    SetFocus(hwndList);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMLISTVIEW nm = (LPNMLISTVIEW)lParam;

            switch (nm->hdr.code)
            {
                case LVN_ITEMCHANGED:
                {
                    if ((nm->uNewState & LVIS_SELECTED) == 0)
                        return FALSE;

                    pData->iLocalCurIcon = nm->iItem;
                    break;
                }
            }
            break;
        }
    }

    return FALSE;
}
