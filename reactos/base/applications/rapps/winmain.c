/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/winmain.c
 * PURPOSE:         Main program
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

HWND hMainWnd;
HINSTANCE hInst;
HIMAGELIST hImageTreeView = NULL;
INT SelectedEnumType = ENUM_ALL_COMPONENTS;
SETTINGS_INFO SettingsInfo;


VOID
FillDafaultSettings(PSETTINGS_INFO pSettingsInfo)
{
    pSettingsInfo->bSaveWndPos = TRUE;
    pSettingsInfo->bUpdateAtStart = FALSE;
    pSettingsInfo->bLogEnabled = TRUE;
    wcscpy(pSettingsInfo->szDownloadDir, L"C:\\Downloads");
    pSettingsInfo->bDelInstaller = FALSE;

    pSettingsInfo->Maximized = FALSE;
    pSettingsInfo->Left = 0;
    pSettingsInfo->Top = 0;
    pSettingsInfo->Right = 680;
    pSettingsInfo->Bottom = 450;
}

static BOOL
LoadSettings(VOID)
{
    HKEY hKey;
    DWORD dwSize;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\ReactOS\\rapps", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
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
        SettingsInfo.Right  = wp.rcNormalPosition.right;
        SettingsInfo.Bottom = wp.rcNormalPosition.bottom;
        SettingsInfo.Maximized = (IsZoomed(hwnd) || (wp.flags & WPF_RESTORETOMAXIMIZED));
    }

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\ReactOS\\rapps", 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, L"Settings", 0, REG_BINARY, (LPBYTE)&SettingsInfo, sizeof(SETTINGS_INFO));
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
        Info = ListViewGetlParam(Count);
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
EnumInstalledAppProc(INT ItemIndex, LPWSTR lpName, INSTALLED_INFO Info)
{
    PINSTALLED_INFO ItemInfo;
    WCHAR szText[MAX_PATH];
    INT Index;

    ItemInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTALLED_INFO));
    if (!ItemInfo) return FALSE;

    *ItemInfo = Info;

    Index = ListViewAddItem(ItemIndex, 0, lpName, (LPARAM)ItemInfo);

    /* Get version info */
    GetApplicationString((HKEY)ItemInfo->hSubKey, L"DisplayVersion", szText);
    ListView_SetItemText(hListView, Index, 1, szText);
    /* Get comments */
    GetApplicationString((HKEY)ItemInfo->hSubKey, L"Comments", szText);
    ListView_SetItemText(hListView, Index, 2, szText);

    return TRUE;
}

VOID
FreeAvailableAppList(VOID)
{
    INT Count = ListView_GetItemCount(hListView) - 1;
    PVOID Info;

    while (Count >= 0)
    {
        Info = ListViewGetlParam(Count);
        if (Info)
            HeapFree(GetProcessHeap(), 0, Info);
        Count--;
    }
}

BOOL
CALLBACK
EnumAvailableAppProc(APPLICATION_INFO Info)
{
    PAPPLICATION_INFO ItemInfo;
    INT Index;

    /* Only add a ListView entry if...
         - no RegName was supplied (so we cannot determine whether the application is installed or not) or
         - a RegName was supplied and the application is not installed
    */
    if (!*Info.szRegName || (!IsInstalledApplication(Info.szRegName, FALSE) && !IsInstalledApplication(Info.szRegName, TRUE)))
    {
        ItemInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(APPLICATION_INFO));
        if (!ItemInfo) return FALSE;

        *ItemInfo = Info;

        Index = ListViewAddItem(Info.Category, 0, Info.szName, (LPARAM)ItemInfo);
        ListView_SetItemText(hListView, Index, 1, Info.szVersion);
        ListView_SetItemText(hListView, Index, 2, Info.szDesc);
    }

    return TRUE;
}

VOID
UpdateApplicationsList(INT EnumType)
{
    WCHAR szBuffer1[MAX_STR_LEN], szBuffer2[MAX_STR_LEN];
    HICON hIcon;
    HIMAGELIST hImageListView;

    (VOID) ListView_DeleteAllItems(hListView);

    /* Create image list */
    hImageListView = ImageList_Create(LISTVIEW_ICON_SIZE,
                                      LISTVIEW_ICON_SIZE,
                                      GetSystemColorDepth() | ILC_MASK,
                                      0, 1);

    hIcon = LoadImage(hInst,
                      MAKEINTRESOURCE(IDI_MAIN),
                      IMAGE_ICON,
                      LISTVIEW_ICON_SIZE,
                      LISTVIEW_ICON_SIZE,
                      LR_CREATEDIBSECTION);

    ImageList_AddIcon(hImageListView, hIcon);
    DestroyIcon(hIcon);

    if (EnumType == -1) EnumType = SelectedEnumType;

    if (IS_INSTALLED_ENUM(SelectedEnumType))
        FreeInstalledAppList();
    else if (IS_AVAILABLE_ENUM(SelectedEnumType))
        FreeAvailableAppList();

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

    LoadStringW(hInst, IDS_APPS_COUNT, szBuffer2, sizeof(szBuffer2) / sizeof(WCHAR));
    swprintf(szBuffer1, szBuffer2, ListView_GetItemCount(hListView));
    SetStatusBarText(szBuffer1);

    SetWelcomeText();
}

VOID
InitApplicationsList(VOID)
{
    WCHAR szText[MAX_STR_LEN];

    /* Add columns to ListView */
    LoadStringW(hInst, IDS_APP_NAME, szText, sizeof(szText) / sizeof(WCHAR));
    ListViewAddColumn(0, szText, 200, LVCFMT_LEFT);

    LoadStringW(hInst, IDS_APP_INST_VERSION, szText, sizeof(szText) / sizeof(WCHAR));
    ListViewAddColumn(1, szText, 90, LVCFMT_RIGHT);

    LoadStringW(hInst, IDS_APP_DESCRIPTION, szText, sizeof(szText) / sizeof(WCHAR));
    ListViewAddColumn(3, szText, 250, LVCFMT_LEFT);

    UpdateApplicationsList(ENUM_ALL_COMPONENTS);
}

HTREEITEM
AddCategory(HTREEITEM hRootItem, UINT TextIndex, UINT IconIndex)
{
    WCHAR szText[MAX_STR_LEN];
    INT Index;
    HICON hIcon;

    hIcon = LoadImage(hInst,
                      MAKEINTRESOURCE(IconIndex),
                      IMAGE_ICON,
                      TREEVIEW_ICON_SIZE,
                      TREEVIEW_ICON_SIZE,
                      LR_CREATEDIBSECTION);

    Index = ImageList_AddIcon(hImageTreeView, hIcon);
    DestroyIcon(hIcon);

    LoadStringW(hInst, TextIndex, szText, sizeof(szText) / sizeof(TCHAR));

    return TreeViewAddItem(hRootItem, szText, Index, Index, TextIndex);
}

VOID
InitCategoriesList(VOID)
{
    HTREEITEM hRootItem1, hRootItem2;

    /* Create image list */
    hImageTreeView = ImageList_Create(TREEVIEW_ICON_SIZE,
                                      TREEVIEW_ICON_SIZE,
                                      GetSystemColorDepth() | ILC_MASK,
                                      0, 1);

    hRootItem1 = AddCategory(TVI_ROOT, IDS_INSTALLED, IDI_CATEGORY);
    AddCategory(hRootItem1, IDS_APPLICATIONS, IDI_APPS);
    AddCategory(hRootItem1, IDS_UPDATES, IDI_APPUPD);

    hRootItem2 = AddCategory(TVI_ROOT, IDS_AVAILABLEFORINST, IDI_CATEGORY);
    AddCategory(hRootItem2, IDS_CAT_AUDIO, IDI_CAT_AUDIO);
    AddCategory(hRootItem2, IDS_CAT_VIDEO, IDI_CAT_VIDEO);
    AddCategory(hRootItem2, IDS_CAT_GRAPHICS, IDI_CAT_GRAPHICS);
    AddCategory(hRootItem2, IDS_CAT_GAMES, IDI_CAT_GAMES);
    AddCategory(hRootItem2, IDS_CAT_INTERNET, IDI_CAT_INTERNET);
    AddCategory(hRootItem2, IDS_CAT_OFFICE, IDI_CAT_OFFICE);
    AddCategory(hRootItem2, IDS_CAT_DEVEL, IDI_CAT_DEVEL);
    AddCategory(hRootItem2, IDS_CAT_EDU, IDI_CAT_EDU);
    AddCategory(hRootItem2, IDS_CAT_ENGINEER, IDI_CAT_ENGINEER);
    AddCategory(hRootItem2, IDS_CAT_FINANCE, IDI_CAT_FINANCE);
    AddCategory(hRootItem2, IDS_CAT_SCIENCE, IDI_CAT_SCIENCE);
    AddCategory(hRootItem2, IDS_CAT_TOOLS, IDI_CAT_TOOLS);
    AddCategory(hRootItem2, IDS_CAT_DRIVERS, IDI_CAT_DRIVERS);
    AddCategory(hRootItem2, IDS_CAT_LIBS, IDI_CAT_LIBS);
    AddCategory(hRootItem2, IDS_CAT_OTHER, IDI_CAT_OTHER);

    (VOID) TreeView_SetImageList(hTreeView, hImageTreeView, TVSIL_NORMAL);

    (VOID) TreeView_Expand(hTreeView, hRootItem2, TVE_EXPAND);
    (VOID) TreeView_Expand(hTreeView, hRootItem1, TVE_EXPAND);

    (VOID) TreeView_SelectItem(hTreeView, hRootItem1);
}

BOOL
InitControls(HWND hwnd)
{
    if (SettingsInfo.bSaveWndPos)
    {
        MoveWindow(hwnd, SettingsInfo.Left, SettingsInfo.Top,
                   SettingsInfo.Right - SettingsInfo.Left,
                   SettingsInfo.Bottom - SettingsInfo.Top, TRUE);

        if (SettingsInfo.Maximized) ShowWindow(hwnd, SW_MAXIMIZE);
    }

    if (CreateStatusBar(hwnd) &&
        CreateToolBar(hwnd) &&
        CreateListView(hwnd) &&
        CreateTreeView(hwnd) &&
        CreateRichEdit(hwnd) &&
        CreateVSplitBar(hwnd) &&
        CreateHSplitBar(hwnd))
    {
        WCHAR szBuffer1[MAX_STR_LEN], szBuffer2[MAX_STR_LEN];

        InitApplicationsList();

        InitCategoriesList();

        LoadStringW(hInst, IDS_APPS_COUNT, szBuffer2, sizeof(szBuffer2) / sizeof(WCHAR));
        swprintf(szBuffer1, szBuffer2, ListView_GetItemCount(hListView));
        SetStatusBarText(szBuffer1);
        return TRUE;
    }

    return FALSE;
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

                LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, sizeof(szBuf) / sizeof(WCHAR));
                GetWindowTextW(hSearchBar, szWndText, MAX_STR_LEN);
                if (wcscmp(szBuf, szWndText) == 0) SetWindowTextW(hSearchBar, L"");
            }
            break;

            case EN_KILLFOCUS:
            {
                GetWindowTextW(hSearchBar, szBuf, MAX_STR_LEN);
                if (wcslen(szBuf) < 1)
                {
                    LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, sizeof(szBuf) / sizeof(WCHAR));
                    SetWindowTextW(hSearchBar, szBuf);
                }
            }
            break;

            case EN_CHANGE:
                /* TODO: Implement search */
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

        case ID_HELP:
            MessageBoxW(hwnd, L"Help not implemented yet", NULL, MB_OK);
            break;

        case ID_ABOUT:
            ShowAboutDialog();
            break;
    }
}

VOID
MainWndOnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDWP hdwp = BeginDeferWindowPos(5);
    INT SearchBarWidth = GetWindowWidth(hSearchBar);
    INT RichPos = GetWindowHeight(hRichEdit);
    INT NewPos = HIWORD(lParam) - (RichPos + SPLIT_WIDTH + GetWindowHeight(hStatusBar));
    INT VSplitterPos;

    /* Size status bar */
    SendMessage(hStatusBar, WM_SIZE, 0, 0);

    /* Size tool bar */
    SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);

    /* Size SearchBar */
    MoveWindow(hSearchBar, LOWORD(lParam) - SearchBarWidth - 4, 5, SearchBarWidth, 22, TRUE);

    /*
     * HIWORD(lParam) - Height of main window
     * LOWORD(lParam) - Width of main window
     */

    /* Size vertical splitter bar */
    DeferWindowPos(hdwp,
                   hVSplitter,
                   0,
                   (VSplitterPos = GetWindowWidth(hTreeView)),
                   GetWindowHeight(hToolBar),
                   SPLIT_WIDTH,
                   HIWORD(lParam) - GetWindowHeight(hToolBar) - GetWindowHeight(hStatusBar),
                   SWP_NOZORDER|SWP_NOACTIVATE);

    /* Size TreeView */
    DeferWindowPos(hdwp,
                   hTreeView,
                   0,
                   0,
                   GetWindowHeight(hToolBar),
                   VSplitterPos,
                   HIWORD(lParam) - GetWindowHeight(hToolBar) - GetWindowHeight(hStatusBar),
                   SWP_NOZORDER|SWP_NOACTIVATE);

    while (NewPos < SPLIT_WIDTH + GetWindowHeight(hToolBar))
    {
        RichPos--;
        NewPos = HIWORD(lParam) - (RichPos +
                 SPLIT_WIDTH + GetWindowHeight(hStatusBar));
    }
    SetHSplitterPos(NewPos);

    /* Size ListView */
    DeferWindowPos(hdwp,
                   hListView,
                   0,
                   VSplitterPos + SPLIT_WIDTH,
                   GetWindowHeight(hToolBar),
                   LOWORD(lParam) - (VSplitterPos + SPLIT_WIDTH),
                   GetHSplitterPos() - GetWindowHeight(hToolBar),
                   SWP_NOZORDER|SWP_NOACTIVATE);

    /* Size RichEdit */
    DeferWindowPos(hdwp,
                   hRichEdit,
                   0,
                   VSplitterPos + SPLIT_WIDTH,
                   GetHSplitterPos() + SPLIT_WIDTH,
                   LOWORD(lParam) - (VSplitterPos + SPLIT_WIDTH),
                   RichPos,
                   SWP_NOZORDER|SWP_NOACTIVATE);

    /* Size horizontal splitter bar */
    DeferWindowPos(hdwp,
                   hHSplitter,
                   0,
                   VSplitterPos + SPLIT_WIDTH,
                   GetHSplitterPos(),
                   LOWORD(lParam) - (VSplitterPos + SPLIT_WIDTH),
                   SPLIT_WIDTH,
                   SWP_NOZORDER|SWP_NOACTIVATE);

    EndDeferWindowPos(hdwp);
}

LRESULT CALLBACK
MainWindowProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_CREATE:
            if (!InitControls(hwnd))
                PostMessage(hwnd, WM_CLOSE, 0, 0);

            if (SettingsInfo.bUpdateAtStart)
                UpdateAppsDB();
            break;

        case WM_COMMAND:
            MainWndOnCommand(hwnd, wParam, lParam);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            switch (data->code)
            {
                case TVN_SELCHANGED:
                {
                    if (data->hwndFrom == hTreeView)
                    {
                        switch (((LPNMTREEVIEW)lParam)->itemNew.lParam)
                        {
                            case IDS_INSTALLED:
                                UpdateApplicationsList(ENUM_ALL_COMPONENTS);
                                break;

                            case IDS_APPLICATIONS:
                                UpdateApplicationsList(ENUM_APPLICATIONS);
                                break;

                            case IDS_UPDATES:
                                UpdateApplicationsList(ENUM_UPDATES);
                                break;

                            case IDS_AVAILABLEFORINST:
                                UpdateApplicationsList(ENUM_ALL_AVAILABLE);
                                break;

                            case IDS_CAT_AUDIO:
                                UpdateApplicationsList(ENUM_CAT_AUDIO);
                                break;

                            case IDS_CAT_DEVEL:
                                UpdateApplicationsList(ENUM_CAT_DEVEL);
                                break;

                            case IDS_CAT_DRIVERS:
                                UpdateApplicationsList(ENUM_CAT_DRIVERS);
                                break;

                            case IDS_CAT_EDU:
                                UpdateApplicationsList(ENUM_CAT_EDU);
                                break;

                            case IDS_CAT_ENGINEER:
                                UpdateApplicationsList(ENUM_CAT_ENGINEER);
                                break;

                            case IDS_CAT_FINANCE:
                                UpdateApplicationsList(ENUM_CAT_FINANCE);
                                break;

                            case IDS_CAT_GAMES:
                                UpdateApplicationsList(ENUM_CAT_GAMES);
                                break;

                            case IDS_CAT_GRAPHICS:
                                UpdateApplicationsList(ENUM_CAT_GRAPHICS);
                                break;

                            case IDS_CAT_INTERNET:
                                UpdateApplicationsList(ENUM_CAT_INTERNET);
                                break;

                            case IDS_CAT_LIBS:
                                UpdateApplicationsList(ENUM_CAT_LIBS);
                                break;

                            case IDS_CAT_OFFICE:
                                UpdateApplicationsList(ENUM_CAT_OFFICE);
                                break;

                            case IDS_CAT_OTHER:
                                UpdateApplicationsList(ENUM_CAT_OTHER);
                                break;

                            case IDS_CAT_SCIENCE:
                                UpdateApplicationsList(ENUM_CAT_SCIENCE);
                                break;

                            case IDS_CAT_TOOLS:
                                UpdateApplicationsList(ENUM_CAT_TOOLS);
                                break;

                            case IDS_CAT_VIDEO:
                                UpdateApplicationsList(ENUM_CAT_VIDEO);
                                break;
                        }
                    }
                }
                break;

                case LVN_KEYDOWN:
                {
                    LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN) lParam;

                    if (pnkd->hdr.hwndFrom == hListView)
                    {
                        INT ItemIndex = (INT) SendMessage(hListView, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

                        if (pnkd->wVKey == VK_UP) ItemIndex -= 1;
                        if (pnkd->wVKey == VK_DOWN) ItemIndex += 1;

                        if (IS_INSTALLED_ENUM(SelectedEnumType))
                            ShowInstalledAppInfo(ItemIndex);
                        if (IS_AVAILABLE_ENUM(SelectedEnumType))
                            ShowAvailableAppInfo(ItemIndex);
                    }
                }
                break;

                case LVN_COLUMNCLICK:
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                    (VOID) ListView_SortItems(hListView, ListViewCompareFunc, pnmv->iSubItem);
                    bAscending = !bAscending;
                }
                break;

                case NM_CLICK:
                    if (data->hwndFrom == hListView)
                    {
                        if (IS_INSTALLED_ENUM(SelectedEnumType))
                            ShowInstalledAppInfo(-1);
                        if (IS_AVAILABLE_ENUM(SelectedEnumType))
                            ShowAvailableAppInfo(-1);
                    }
                    break;

                case NM_RCLICK:
                    if (data->hwndFrom == hListView)
                        ShowPopupMenu(hListView, IDR_APPLICATIONMENU);
                    break;

                case EN_LINK:
                    RichEditOnLink(hwnd, (ENLINK*)lParam);
                    break;

                case TTN_GETDISPINFO:
                    ToolBarOnGetDispInfo((LPTOOLTIPTEXT)lParam);
                    break;
            }
        }
        break;

        case WM_PAINT:
        break;

        case WM_SIZE:
        {
            if ((GetClientWindowHeight(hMainWnd) - GetWindowHeight(hStatusBar) - SPLIT_WIDTH) < GetHSplitterPos())
            {
                INT NewSplitPos = GetClientWindowHeight(hwnd) - 100 - GetWindowHeight(hStatusBar) - SPLIT_WIDTH;
                if (NewSplitPos > GetWindowHeight(hToolBar) + SPLIT_WIDTH)
                    SetHSplitterPos(NewSplitPos);
            }

            MainWndOnSize(hwnd, wParam, lParam);
        }
        break;

        case WM_SIZING:
        {
            int RichEditHeight = GetWindowHeight(hRichEdit);
            LPRECT pRect = (LPRECT)lParam;

            while (RichEditHeight <= 100)
            {
                if (GetHSplitterPos() - 1 < GetWindowHeight(hToolBar) + GetWindowHeight(hListView) + SPLIT_WIDTH)
                    break;
                SetHSplitterPos(GetHSplitterPos() - 1);
                RichEditHeight++;
            }

            if (pRect->right-pRect->left < 565)
                pRect->right = pRect->left + 565;

            if (pRect->bottom-pRect->top < 300)
                pRect->bottom = pRect->top + 300;
            return TRUE;
        }

        case WM_SYSCOLORCHANGE:
        {
            /* Forward WM_SYSCOLORCHANGE to common controls */
            SendMessage(hListView, WM_SYSCOLORCHANGE, 0, 0);
            SendMessage(hTreeView, WM_SYSCOLORCHANGE, 0, 0);
            SendMessage(hToolBar, WM_SYSCOLORCHANGE, 0, 0);
            SendMessageW(hRichEdit, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
        }
        break;

        case WM_DESTROY:
        {
            ShowWindow(hwnd, SW_HIDE);
            SaveSettings(hwnd);

            FreeLogs();

            if (IS_AVAILABLE_ENUM(SelectedEnumType))
                FreeAvailableAppList();
            if (IS_INSTALLED_ENUM(SelectedEnumType))
                FreeInstalledAppList();
            if (hImageTreeView) ImageList_Destroy(hImageTreeView);

            PostQuitMessage(0);
            return 0;
        }
        break;
    }

    return DefWindowProc(hwnd, Msg, wParam, lParam);
}

int WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    WNDCLASSEXW WndClass = {0};
    WCHAR szWindowClass[] = L"ROSAPPMGR";
    WCHAR szWindowName[MAX_STR_LEN];
    WCHAR szErrorText[MAX_STR_LEN];
    HANDLE hMutex = NULL;
    MSG Msg;

    hInst = hInstance;

    if (!IsUserAnAdmin())
    {
        LoadStringW(hInst, IDS_USER_NOT_ADMIN, szErrorText, sizeof(szErrorText) / sizeof(WCHAR));
        MessageBox(0, szErrorText, NULL, MB_OK | MB_ICONWARNING);
        return 1;
    }

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
        FillDafaultSettings(&SettingsInfo);
    }

    InitLogs();

    InitCommonControls();

    /* Create the window */
    WndClass.cbSize        = sizeof(WNDCLASSEXW);
    WndClass.lpszClassName = szWindowClass;
    WndClass.lpfnWndProc   = MainWindowProc;
    WndClass.hInstance     = hInstance;
    WndClass.style         = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    WndClass.lpszMenuName  = MAKEINTRESOURCEW(IDR_MAINMENU);

    if (RegisterClassExW(&WndClass) == (ATOM)0) goto Exit;

    LoadStringW(hInst, IDS_APPTITLE, szWindowName, sizeof(szWindowName) / sizeof(WCHAR));

    hMainWnd = CreateWindowExW(WS_EX_WINDOWEDGE,
                               szWindowClass,
                               szWindowName,
                               WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               680,
                               450,
                               NULL,
                               NULL,
                               hInstance,
                               NULL);

    if (!hMainWnd) goto Exit;

    /* Show it */
    ShowWindow(hMainWnd, SW_SHOW);
    UpdateWindow(hMainWnd);

    /* Message Loop */
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

Exit:
    if (hMutex) CloseHandle(hMutex);

    return 0;
}
