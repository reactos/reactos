/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/winmain.c
 * PURPOSE:         Main program
 * PROGRAMMERS:     Dmitry Chapyshev           (dmitry@reactos.org)
 *                  Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 */

#include "rapps.h"

#include <atlbase.h>
#include <atlcom.h>
#include <shellapi.h>

#define SEARCH_TIMER_ID 'SR'

HWND hMainWnd;
HINSTANCE hInst;
INT SelectedEnumType = ENUM_ALL_COMPONENTS;
SETTINGS_INFO SettingsInfo;

WCHAR szSearchPattern[MAX_STR_LEN] = L"";
BOOL SearchEnabled = TRUE;

class CRAppsModule : public CComModule
{
public:
};

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

CRAppsModule                             gModule;
CAtlWinModule                               gWinModule;

void *operator new (size_t, void *buf)
{
    return buf;
}

static VOID InitializeAtlModule(HINSTANCE hInstance, BOOL bInitialize)
{
    if (bInitialize)
    {
        /* HACK - the global constructors don't run, so I placement new them here */
        new (&gModule) CRAppsModule;
        new (&gWinModule) CAtlWinModule;
        new (&_AtlBaseModule) CAtlBaseModule;
        new (&_AtlComModule) CAtlComModule;

        gModule.Init(ObjectMap, hInstance, NULL);
    }
    else
    {
        gModule.Term();
    }
}

BOOL
SearchPatternMatch(PCWSTR szHaystack, PCWSTR szNeedle)
{
    if (!*szNeedle)
        return TRUE;
    /* TODO: Improve pattern search beyond a simple case-insensitive substring search. */
    return StrStrIW(szHaystack, szNeedle) != NULL;
}

VOID
FillDefaultSettings(PSETTINGS_INFO pSettingsInfo)
{
    pSettingsInfo->bSaveWndPos = TRUE;
    pSettingsInfo->bUpdateAtStart = FALSE;
    pSettingsInfo->bLogEnabled = TRUE;
    StringCbCopyW(pSettingsInfo->szDownloadDir,
                  sizeof(pSettingsInfo->szDownloadDir),
                  L"C:\\Downloads");
    pSettingsInfo->bDelInstaller = FALSE;

    pSettingsInfo->Maximized = FALSE;
    pSettingsInfo->Left = CW_USEDEFAULT;
    pSettingsInfo->Top = CW_USEDEFAULT;
    pSettingsInfo->Width = 680;
    pSettingsInfo->Height = 450;

    pSettingsInfo->Proxy = 0;
    StringCbCopyW(pSettingsInfo->szProxyServer, sizeof(pSettingsInfo->szProxyServer), L"");
    StringCbCopyW(pSettingsInfo->szNoProxyFor,  sizeof(pSettingsInfo->szNoProxyFor),  L"");
}

static BOOL
LoadSettings(VOID)
{
    HKEY hKey;
    DWORD dwSize;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\ReactOS\\rapps", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(SETTINGS_INFO);
        if (RegQueryValueExW(hKey, L"Settings", NULL, NULL, (LPBYTE)&SettingsInfo, &dwSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }

        RegCloseKey(hKey);
    }

    return FALSE;
}

VOID
SaveSettings(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    HKEY hKey;

    if (SettingsInfo.bSaveWndPos)
    {
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &wp);

        SettingsInfo.Left = wp.rcNormalPosition.left;
        SettingsInfo.Top  = wp.rcNormalPosition.top;
        SettingsInfo.Width  = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
        SettingsInfo.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        SettingsInfo.Maximized = (wp.showCmd == SW_MAXIMIZE || (wp.showCmd == SW_SHOWMINIMIZED && (wp.flags & WPF_RESTORETOMAXIMIZED)));
    }

    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\ReactOS\\rapps", 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, L"Settings", 0, REG_BINARY, (LPBYTE)&SettingsInfo, sizeof(SETTINGS_INFO));
        RegCloseKey(hKey);
    }
}

VOID
FreeInstalledAppList(VOID)
{
    INT Count = ListView_GetItemCount(hListView) - 1;
    PINSTALLED_INFO Info;

    while (Count >= 0)
    {
        Info = (PINSTALLED_INFO)ListViewGetlParam(Count);
        if (Info)
        {
            RegCloseKey(Info->hSubKey);
            HeapFree(GetProcessHeap(), 0, Info);
        }
        Count--;
    }
}

BOOL
CALLBACK
EnumInstalledAppProc(INT ItemIndex, LPWSTR lpName, PINSTALLED_INFO Info)
{
    PINSTALLED_INFO ItemInfo;
    WCHAR szText[MAX_PATH];
    INT Index;

    if (!SearchPatternMatch(lpName, szSearchPattern))
    {
        RegCloseKey(Info->hSubKey);
        return TRUE;
    }

    ItemInfo = (PINSTALLED_INFO) HeapAlloc(GetProcessHeap(), 0, sizeof(INSTALLED_INFO));
    if (!ItemInfo)
    {
        RegCloseKey(Info->hSubKey);
        return FALSE;
    }

    RtlCopyMemory(ItemInfo, Info, sizeof(INSTALLED_INFO));

    Index = ListViewAddItem(ItemIndex, 0, lpName, (LPARAM)ItemInfo);

    /* Get version info */
    GetApplicationString(ItemInfo->hSubKey, L"DisplayVersion", szText);
    ListView_SetItemText(hListView, Index, 1, szText);

    /* Get comments */
    GetApplicationString(ItemInfo->hSubKey, L"Comments", szText);
    ListView_SetItemText(hListView, Index, 2, szText);

    return TRUE;
}

BOOL
CALLBACK
EnumAvailableAppProc(PAPPLICATION_INFO Info)
{
    INT Index;

    if (!SearchPatternMatch(Info->szName, szSearchPattern) &&
        !SearchPatternMatch(Info->szDesc, szSearchPattern))
    {
        return TRUE;
    }

    /* Only add a ListView entry if...
         - no RegName was supplied (so we cannot determine whether the application is installed or not) or
         -  a RegName was supplied and the application is not installed
    */
    if (!*Info->szRegName || (!IsInstalledApplication(Info->szRegName, FALSE) && !IsInstalledApplication(Info->szRegName, TRUE)))
    {
        Index = ListViewAddItem(Info->Category, 0, Info->szName, (LPARAM)Info);

        ListView_SetItemText(hListView, Index, 1, Info->szVersion);
        ListView_SetItemText(hListView, Index, 2, Info->szDesc);
    }

    return TRUE;
}

VOID
UpdateApplicationsList(INT EnumType)
{
    WCHAR szBuffer1[MAX_STR_LEN], szBuffer2[MAX_STR_LEN];
    HICON hIcon;
    HIMAGELIST hImageListView;

    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);

    if (EnumType == -1) EnumType = SelectedEnumType;

    if (IS_INSTALLED_ENUM(SelectedEnumType))
        FreeInstalledAppList();

    (VOID) ListView_DeleteAllItems(hListView);

    /* Create image list */
    hImageListView = ImageList_Create(LISTVIEW_ICON_SIZE,
                                      LISTVIEW_ICON_SIZE,
                                      GetSystemColorDepth() | ILC_MASK,
                                      0, 1);

    hIcon = (HICON)LoadImage(hInst,
                      MAKEINTRESOURCE(IDI_MAIN),
                      IMAGE_ICON,
                      LISTVIEW_ICON_SIZE,
                      LISTVIEW_ICON_SIZE,
                      LR_CREATEDIBSECTION);

    ImageList_AddIcon(hImageListView, hIcon);
    DestroyIcon(hIcon);

    if (IS_INSTALLED_ENUM(EnumType))
    {
        /* Enum installed applications and updates */
        EnumInstalledApplications(EnumType, TRUE, EnumInstalledAppProc);
        EnumInstalledApplications(EnumType, FALSE, EnumInstalledAppProc);
    }
    else if (IS_AVAILABLE_ENUM(EnumType))
    {
        /* Enum availabled applications */
        EnumAvailableApplications(EnumType, EnumAvailableAppProc);
    }

    /* Set image list for ListView */
    hImageListView = ListView_SetImageList(hListView, hImageListView, LVSIL_SMALL);

    /* Destroy old image list */
    if (hImageListView)
        ImageList_Destroy(hImageListView);

    SelectedEnumType = EnumType;

    LoadStringW(hInst, IDS_APPS_COUNT, szBuffer2, _countof(szBuffer2));
    StringCbPrintfW(szBuffer1, sizeof(szBuffer1),
                    szBuffer2,
                    ListView_GetItemCount(hListView));
    SetStatusBarText(szBuffer1);

    SetWelcomeText();

    /* set automatic column width for program names if the list is not empty */
    if (ListView_GetItemCount(hListView) > 0)
        ListView_SetColumnWidth(hListView, 0, LVSCW_AUTOSIZE);

    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
}

VOID CALLBACK
SearchTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    KillTimer(hwnd, SEARCH_TIMER_ID);
    UpdateApplicationsList(-1);
}

VOID
MainWndOnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    WORD wCommand = LOWORD(wParam);

    if (lParam == (LPARAM)hSearchBar)
    {
        WCHAR szBuf[MAX_STR_LEN];

        switch (HIWORD(wParam))
        {
            case EN_SETFOCUS:
            {
                WCHAR szWndText[MAX_STR_LEN];

                LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, _countof(szBuf));
                GetWindowTextW(hSearchBar, szWndText, MAX_STR_LEN);
                if (wcscmp(szBuf, szWndText) == 0)
                {
                    SearchEnabled = FALSE;
                    SetWindowTextW(hSearchBar, L"");
                }
            }
            break;

            case EN_KILLFOCUS:
            {
                GetWindowTextW(hSearchBar, szBuf, MAX_STR_LEN);
                if (wcslen(szBuf) < 1)
                {
                    LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, _countof(szBuf));
                    SearchEnabled = FALSE;
                    SetWindowTextW(hSearchBar, szBuf);
                }
            }
            break;

            case EN_CHANGE:
            {
                WCHAR szWndText[MAX_STR_LEN];

                if (!SearchEnabled)
                {
                    SearchEnabled = TRUE;
                    break;
                }

                LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, _countof(szBuf));
                GetWindowTextW(hSearchBar, szWndText, MAX_STR_LEN);
                if (wcscmp(szBuf, szWndText) != 0)
                {
                    StringCbCopy(szSearchPattern, sizeof(szSearchPattern),
                                 szWndText);
                }
                else
                {
                    szSearchPattern[0] = UNICODE_NULL;
                }

                SetTimer(hwnd, SEARCH_TIMER_ID, 250, SearchTimerProc);
            }
            break;
        }

        return;
    }

    switch (wCommand)
    {
        case ID_OPEN_LINK:
            ShellExecuteW(hwnd, L"open", pLink, NULL, NULL, SW_SHOWNOACTIVATE);
            HeapFree(GetProcessHeap(), 0, pLink);
            break;

        case ID_COPY_LINK:
            CopyTextToClipboard(pLink);
            HeapFree(GetProcessHeap(), 0, pLink);
            break;

        case ID_SETTINGS:
            CreateSettingsDlg(hwnd);
            break;

        case ID_EXIT:
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
            break;

        case ID_INSTALL:
            if (DownloadApplication(-1))
            /* TODO: Implement install dialog
             *   if (InstallApplication(-1))
             */
                UpdateApplicationsList(-1);
            break;

        case ID_UNINSTALL:
            if (UninstallApplication(-1, FALSE))
                UpdateApplicationsList(-1);
            break;

        case ID_MODIFY:
            if (UninstallApplication(-1, TRUE))
                UpdateApplicationsList(-1);
            break;

        case ID_REGREMOVE:
            RemoveAppFromRegistry(-1);
            break;

        case ID_REFRESH:
            UpdateApplicationsList(-1);
            break;

        case ID_RESETDB:
            UpdateAppsDB();
            UpdateApplicationsList(-1);
            break;

        case ID_HELP:
            MessageBoxW(hwnd, L"Help not implemented yet", NULL, MB_OK);
            break;

        case ID_ABOUT:
            ShowAboutDialog();
            break;
    }
}

BOOL IsSelectedNodeInstalled(void)
{
    HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView);
    TV_ITEM tItem;

    tItem.mask = TVIF_PARAM | TVIF_HANDLE;
    tItem.hItem = hSelectedItem;
    TreeView_GetItem(hTreeView, &tItem);
    switch (tItem.lParam)
    {
        case IDS_INSTALLED:
        case IDS_APPLICATIONS:
        case IDS_UPDATES:
            return TRUE;
        default:
            return FALSE;
    }
}

int WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    WCHAR szWindowClass[] = L"ROSAPPMGR";
    HANDLE hMutex = NULL;
    HACCEL KeyBrd;
    MSG Msg;

    InitializeAtlModule(hInstance, TRUE);

    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
            SetProcessDefaultLayout(LAYOUT_RTL);
            break;

        default:
            break;
    }

    hInst = hInstance;

    hMutex = CreateMutexW(NULL, FALSE, szWindowClass);
    if ((!hMutex) || (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        /* If already started, it is found its window */
        HWND hWindow = FindWindowW(szWindowClass, NULL);

        /* Activate window */
        ShowWindow(hWindow, SW_SHOWNORMAL);
        SetForegroundWindow(hWindow);
        return 1;
    }

    if (!LoadSettings())
    {
        FillDefaultSettings(&SettingsInfo);
    }

    InitLogs();

    InitCommonControls();

    hMainWnd = CreateMainWindow();
    if (!hMainWnd) goto Exit;

    /* Maximize it if we must */
    ShowWindow(hMainWnd, (SettingsInfo.bSaveWndPos && SettingsInfo.Maximized ? SW_MAXIMIZE : nShowCmd));
    UpdateWindow(hMainWnd);

    if (SettingsInfo.bUpdateAtStart)
        UpdateAppsDB();

    /* Load the menu hotkeys */
    KeyBrd = LoadAccelerators(NULL, MAKEINTRESOURCE(HOTKEYS));

    /* Message Loop */
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(hMainWnd, KeyBrd, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

Exit:
    if (hMutex)
        CloseHandle(hMutex);

    InitializeAtlModule(hInstance, FALSE);

    return 0;
}
