/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/winmain.c
 * PURPOSE:         Main program
 * PROGRAMMERS:     Dmitry Chapyshev           (dmitry@reactos.org)
 *                  Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 */

#include <shellapi.h>
#include "rapps.h"

#define SEARCH_TIMER_ID 'SR'

HWND hMainWnd;
HINSTANCE hInst;
HIMAGELIST hImageTreeView = NULL;
INT SelectedEnumType = ENUM_ALL_COMPONENTS;
SETTINGS_INFO SettingsInfo;

WCHAR szSearchPattern[MAX_STR_LEN] = L"";
BOOL SearchEnabled = TRUE;

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
EnumInstalledAppProc(INT ItemIndex, LPWSTR lpName, PINSTALLED_INFO Info)
{
    PINSTALLED_INFO ItemInfo;
    WCHAR szText[MAX_PATH];
    INT Index;

    if (!SearchPatternMatch(lpName, szSearchPattern))
        return TRUE;

    ItemInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(INSTALLED_INFO));
    if (!ItemInfo) return FALSE;

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

    LoadStringW(hInst, IDS_APPS_COUNT, szBuffer2, _countof(szBuffer2));
    StringCbPrintfW(szBuffer1, sizeof(szBuffer1),
                    szBuffer2,
                    ListView_GetItemCount(hListView));
    SetStatusBarText(szBuffer1);

    SetWelcomeText();

    /* set automatic column width for program names if the list is not empty */
    if (ListView_GetItemCount(hListView) > 0)
        ListView_SetColumnWidth(hListView, 0, LVSCW_AUTOSIZE);
}

VOID
InitApplicationsList(VOID)
{
    WCHAR szText[MAX_STR_LEN];

    /* Add columns to ListView */
    LoadStringW(hInst, IDS_APP_NAME, szText, _countof(szText));
    ListViewAddColumn(0, szText, 200, LVCFMT_LEFT);

    LoadStringW(hInst, IDS_APP_INST_VERSION, szText, _countof(szText));
    ListViewAddColumn(1, szText, 90, LVCFMT_RIGHT);

    LoadStringW(hInst, IDS_APP_DESCRIPTION, szText, _countof(szText));
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

    LoadStringW(hInst, TextIndex, szText, _countof(szText));

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

    if (CreateStatusBar(hwnd) &&
        CreateToolBar(hwnd)   &&
        CreateListView(hwnd)  &&
        CreateTreeView(hwnd)  &&
        CreateRichEdit(hwnd)  &&
        CreateVSplitBar(hwnd) &&
        CreateHSplitBar(hwnd))
    {
        WCHAR szBuffer1[MAX_STR_LEN], szBuffer2[MAX_STR_LEN];

        InitApplicationsList();

        InitCategoriesList();

        LoadStringW(hInst, IDS_APPS_COUNT, szBuffer2, _countof(szBuffer2));
        StringCbPrintfW(szBuffer1, sizeof(szBuffer1),
                        szBuffer2,
                        ListView_GetItemCount(hListView));
        SetStatusBarText(szBuffer1);
        return TRUE;
    }

    return FALSE;
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

    if(wParam != SIZE_MINIMIZED)
    {
        while (NewPos < SPLIT_WIDTH + GetWindowHeight(hToolBar))
        {
            RichPos--;
            NewPos = HIWORD(lParam) - (RichPos +
                     SPLIT_WIDTH + GetWindowHeight(hStatusBar));
        }
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

LRESULT CALLBACK
MainWindowProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{

    switch (Msg)
    {
        case WM_CREATE:
            if (!InitControls(hwnd))
                PostMessage(hwnd, WM_CLOSE, 0, 0);
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

                    /* Disable/enable items based on treeview selection */
                    if (IsSelectedNodeInstalled())
                    {
                        EnableMenuItem(GetMenu(hwnd), ID_REGREMOVE, MF_ENABLED);
                        EnableMenuItem(GetMenu(hwnd), ID_INSTALL, MF_GRAYED);
                        EnableMenuItem(GetMenu(hwnd), ID_UNINSTALL, MF_ENABLED);
                        EnableMenuItem(GetMenu(hwnd), ID_MODIFY, MF_ENABLED);

                        EnableMenuItem(GetMenu(hListView), ID_REGREMOVE, MF_ENABLED);
                        EnableMenuItem(GetMenu(hListView), ID_INSTALL, MF_GRAYED);
                        EnableMenuItem(GetMenu(hListView), ID_UNINSTALL, MF_ENABLED);
                        EnableMenuItem(GetMenu(hListView), ID_MODIFY, MF_ENABLED);

                        SendMessage(hToolBar, TB_ENABLEBUTTON, ID_REGREMOVE, TRUE);
                        SendMessage(hToolBar, TB_ENABLEBUTTON, ID_INSTALL, FALSE);
                        SendMessage(hToolBar, TB_ENABLEBUTTON, ID_UNINSTALL, TRUE);
                        SendMessage(hToolBar, TB_ENABLEBUTTON, ID_MODIFY, TRUE);
                    }
                    else
                    {
                        EnableMenuItem(GetMenu(hwnd), ID_REGREMOVE, MF_GRAYED);
                        EnableMenuItem(GetMenu(hwnd), ID_INSTALL, MF_ENABLED);
                        EnableMenuItem(GetMenu(hwnd), ID_UNINSTALL, MF_GRAYED);
                        EnableMenuItem(GetMenu(hwnd), ID_MODIFY, MF_GRAYED);

                        EnableMenuItem(GetMenu(hListView), ID_REGREMOVE, MF_GRAYED);
                        EnableMenuItem(GetMenu(hListView), ID_INSTALL, MF_ENABLED);
                        EnableMenuItem(GetMenu(hListView), ID_UNINSTALL, MF_GRAYED);
                        EnableMenuItem(GetMenu(hListView), ID_MODIFY, MF_GRAYED);

                        SendMessage(hToolBar, TB_ENABLEBUTTON, ID_REGREMOVE, FALSE);
                        SendMessage(hToolBar, TB_ENABLEBUTTON, ID_INSTALL, TRUE);
                        SendMessage(hToolBar, TB_ENABLEBUTTON, ID_UNINSTALL, FALSE);
                        SendMessage(hToolBar, TB_ENABLEBUTTON, ID_MODIFY, FALSE);
                    }
                }
                break;

                case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW pnic = (LPNMLISTVIEW) lParam;

                    if (pnic->hdr.hwndFrom == hListView)
                    {
                        /* Check if this is a valid item
                         * (technically, it can be also an unselect) */
                        INT ItemIndex = pnic->iItem;
                        if (ItemIndex == -1 ||
                            ItemIndex >= ListView_GetItemCount(pnic->hdr.hwndFrom))
                        {
                            break;
                        }

                        /* Check if the focus has been moved to another item */
                        if ((pnic->uChanged & LVIF_STATE) &&
                            (pnic->uNewState & LVIS_FOCUSED) &&
                            !(pnic->uOldState & LVIS_FOCUSED))
                        {
                            if (IS_INSTALLED_ENUM(SelectedEnumType))
                                ShowInstalledAppInfo(ItemIndex);
                            if (IS_AVAILABLE_ENUM(SelectedEnumType))
                                ShowAvailableAppInfo(ItemIndex);
                        }
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
                {
                    if (data->hwndFrom == hListView && ((LPNMLISTVIEW)lParam)->iItem != -1)
                    {
                        if (IS_INSTALLED_ENUM(SelectedEnumType))
                            ShowInstalledAppInfo(-1);
                        if (IS_AVAILABLE_ENUM(SelectedEnumType))
                            ShowAvailableAppInfo(-1);
                    }
                }
                break;

                case NM_DBLCLK:
                {
                    if (data->hwndFrom == hListView && ((LPNMLISTVIEW)lParam)->iItem != -1)
                    {
                        /* this won't do anything if the program is already installed */
                        SendMessage(hwnd, WM_COMMAND, ID_INSTALL, 0);
                    }
                }
                break;

                case NM_RCLICK:
                {
                    if (data->hwndFrom == hListView && ((LPNMLISTVIEW)lParam)->iItem != -1)
                    {
                        ShowPopupMenu(hListView, 0, ID_INSTALL);
                    }
                }
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
    HANDLE hMutex = NULL;
    HACCEL KeyBrd;
    MSG Msg;

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

    /* Create the window */
    WndClass.cbSize        = sizeof(WNDCLASSEXW);
    WndClass.lpszClassName = szWindowClass;
    WndClass.lpfnWndProc   = MainWindowProc;
    WndClass.hInstance     = hInstance;
    WndClass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    WndClass.lpszMenuName  = MAKEINTRESOURCEW(IDR_MAINMENU);

    if (RegisterClassExW(&WndClass) == (ATOM)0) goto Exit;

    LoadStringW(hInst, IDS_APPTITLE, szWindowName, _countof(szWindowName));

    hMainWnd = CreateWindowExW(WS_EX_WINDOWEDGE,
                               szWindowClass,
                               szWindowName,
                               WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                               (SettingsInfo.bSaveWndPos ? SettingsInfo.Left   : CW_USEDEFAULT),
                               (SettingsInfo.bSaveWndPos ? SettingsInfo.Top    : CW_USEDEFAULT),
                               (SettingsInfo.bSaveWndPos ? SettingsInfo.Width  : 680),
                               (SettingsInfo.bSaveWndPos ? SettingsInfo.Height : 450),
                               NULL,
                               NULL,
                               hInstance,
                               NULL);

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

    return 0;
}
