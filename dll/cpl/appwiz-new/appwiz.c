/*
 *
 * PROJECT:                 ReactOS Software Control Panel
 * FILE:                    dll/cpl/appwiz/appwiz.c
 * PURPOSE:                 ReactOS Software Control Panel
 * PROGRAMMERS:             Dmitry Chapyshev (dmitry@reactos.org)
 * UPDATE HISTORY:
 *    02-09-2008  Created
 */

#include "appwiz.h"

WCHAR*     DescriptionHeadline = L"";
WCHAR*     DescriptionText = L"";
HBITMAP    hUnderline;
WCHAR      Strings[2][256];
HICON      hSearchIcon;
HTREEITEM  hRootItem;         // First item in actions list
HFONT      hMainFont;
HIMAGELIST hImageAppList;     // Image list for programs list
BOOL       bAscending = TRUE; // Sorting programs list

HDC BackbufferHdc = NULL;
HBITMAP BackbufferBmp = NULL;


VOID
ShowMessage(WCHAR* title, WCHAR* message)
{
    DescriptionHeadline = title;
    DescriptionText = message;
    InvalidateRect(hMainWnd,NULL,TRUE);
    UpdateWindow(hMainWnd);
}

static VOID
DrawBitmap(HDC hdc, int x, int y, HBITMAP hBmp)
{
    BITMAP bm;
    HDC hdcMem = CreateCompatibleDC(hdc);

    SelectObject(hdcMem, hBmp);
    GetObject(hBmp, sizeof(bm), &bm);
    TransparentBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, 0xFFFFFF);

    DeleteDC(hdcMem);
}

static VOID
DrawDescription(HDC hdc, RECT DescriptionRect)
{
    int i;
    HFONT Font;
    RECT Rect = {DescriptionRect.left+5, DescriptionRect.top+5, DescriptionRect.right-2, DescriptionRect.top+22};

    // Backgroud
    Rectangle(hdc, DescriptionRect.left, DescriptionRect.top, DescriptionRect.right, DescriptionRect.bottom);

    // Underline
    for (i=DescriptionRect.left+1;i<DescriptionRect.right-1;i++)
        DrawBitmap(hdc, i, DescriptionRect.top+22, hUnderline); // less code then stretching ;)

    // Headline
    Font = CreateFont(-14, 0, 0, 0, FW_EXTRABOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"Arial");
    SelectObject(hdc, Font);
    DrawText(hdc, DescriptionHeadline, lstrlen(DescriptionHeadline), &Rect, DT_SINGLELINE|DT_NOPREFIX);
    DeleteObject(Font);

    // Description
    Font = CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"Arial");
    SelectObject(hdc, Font);
    Rect.top += 35;
    Rect.bottom = DescriptionRect.bottom-2;
    DrawText(hdc, DescriptionText, lstrlen(DescriptionText), &Rect, DT_WORDBREAK|DT_NOPREFIX); // ToDo: Call TabbedTextOut to draw a nice table
    DeleteObject(Font);
}

static VOID
ResizeControl(HWND hwnd, int x1, int y1, int x2, int y2)
{
    MoveWindow(hwnd, x1, y1, x2-x1, y2-y1, TRUE);
}

/*
    AddListColumn - adding column items to Application list
*/
static VOID
AddListColumn(VOID)
{
    LV_COLUMN column;
    RECT rect;
    WCHAR szBuf[MAX_PATH];

    GetClientRect(hMainWnd, &rect);
    ZeroMemory(&column, sizeof(LV_COLUMN));
    column.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.fmt          = LVCFMT_LEFT;
    column.iSubItem     = 0;
    LoadString(hApplet, IDS_LIST_TITLE, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    column.pszText      = szBuf;
    column.cx           = 320;
    (void)ListView_InsertColumn(hAppList, 0, &column);

    column.cx           = 75;
    column.iSubItem     = 1;
    LoadString(hApplet, IDS_LAST_USED, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    column.pszText      = szBuf;
    (void)ListView_InsertColumn(hAppList,1,&column);

    column.cx           = 70;
    column.iSubItem     = 2;
    column.fmt          = LVCFMT_RIGHT;
    LoadString(hApplet, IDS_SIZE_TITLE, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    column.pszText      = szBuf;
    (void)ListView_InsertColumn(hAppList,2,&column);
}

static VOID
AddTreeViewItems(VOID)
{
    HIMAGELIST hImageList;
    WCHAR szBuf[1024];
    int Index[2];
    TV_INSERTSTRUCTW Insert;

    hImageList = ImageList_Create(16, 16, ILC_COLORDDB, 1, 1);
    SendMessageW(hActList, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)(HIMAGELIST)hImageList);

    Index[0] = ImageList_Add(hImageList, LoadBitmap(hApplet, MAKEINTRESOURCE(IDB_SELECT)), NULL);
    Index[1] = ImageList_Add(hImageList, LoadBitmap(hApplet, MAKEINTRESOURCE(IDB_ICON)), NULL);

    // Insert items to Actions List
    ZeroMemory(&Insert, sizeof(TV_INSERTSTRUCT));
    Insert.item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
    Insert.hInsertAfter = TVI_LAST;
    Insert.hParent = TVI_ROOT;
    Insert.item.iSelectedImage = Index[0];

    Insert.item.lParam = 0;
    LoadString(hApplet, IDS_PROGANDUPDATES, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    Insert.item.pszText = szBuf;
    Insert.item.iImage = Index[1];
    hRootItem = TreeView_InsertItem(hActList, &Insert);

    Insert.item.lParam = 1;
    LoadString(hApplet, IDS_PROGRAMS_ONLY, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    Insert.item.pszText = szBuf;
    Insert.item.iImage = Index[1];
    (VOID) TreeView_InsertItem(hActList, &Insert);

    Insert.item.lParam = 2;
    LoadString(hApplet, IDS_UPDATES_ONLY, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    Insert.item.pszText = szBuf;
    Insert.item.iImage = Index[1];
    (VOID) TreeView_InsertItem(hActList, &Insert);
    // Select first item
    (VOID) TreeView_SelectItem(hActList, hRootItem);
}

/*
    InitControls - function for init all controls on main window
*/
static VOID
InitControls(VOID)
{
    WCHAR szBuf[1024];

    hMainFont = CreateFont(-11 , 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"Arial");

    hActList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
                              WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS,
                              0, 0, 0, 0, hMainWnd, NULL, hApplet, NULL);

    hAppList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                              WS_CHILD|WS_VISIBLE|WS_BORDER|LVS_SORTASCENDING|LVS_REPORT,
                              0, 0, 0, 0, hMainWnd, NULL, hApplet, NULL);

    (VOID) ListView_SetExtendedListViewStyle(hAppList, LVS_EX_FULLROWSELECT);

    LoadString(hApplet, IDS_SEARCH_TEXT, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    hSearch = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", szBuf, WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
                             0, 0, 0, 0, hMainWnd, NULL, hApplet, NULL);
    SendMessage(hSearch, WM_SETFONT, (WPARAM)hMainFont, 0);
    // Remove button
    LoadString(hApplet, IDS_REMOVE_BTN, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    hRemoveBtn = CreateWindowEx(0, L"BUTTON", szBuf, WS_CHILD|WS_VISIBLE|WS_DISABLED|BS_TEXT,
                             0, 0, 0, 0, hMainWnd, NULL, hApplet, NULL);
    SendMessage(hRemoveBtn, WM_SETFONT, (WPARAM)hMainFont, 0);
    // Modify button
    LoadString(hApplet, IDS_MODIFY_BTN, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    hModifyBtn = CreateWindowEx(0, L"BUTTON", szBuf, WS_CHILD|WS_VISIBLE|WS_DISABLED|BS_TEXT,
                             0, 0, 0, 0, hMainWnd, NULL, hApplet, NULL);
    SendMessage(hModifyBtn, WM_SETFONT, (WPARAM)hMainFont, 0);

    hUnderline = LoadBitmap(hApplet, MAKEINTRESOURCE(IDB_UNDERLINE));
    hSearchIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_SEARCH));

    AddListColumn();
    AddTreeViewItems();
}

/*
 GetARPInfo         - Getting information from ARP cache
 Input:  szName     - Application Name
 Output: szPath     - Path to image file
         szSize     - Application size
         szLastUsed - Last used time
*/
static VOID
GetARPInfo(LPCWSTR szName, LPWSTR szPath, LPWSTR szSize, LPWSTR szLastUsed)
{
    APPARPINFO aai = {0};
    DWORD dwSize = sizeof(aai), dwType = REG_BINARY;
    SYSTEMTIME systime, localtime;
    WCHAR szBuf[MAX_PATH];
    HKEY hKey;

    swprintf(szBuf, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Management\\ARPCache\\%s", szName);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf,
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        ZeroMemory(&aai, sizeof(APPARPINFO));
        if ((RegQueryValueEx(hKey, L"SlowInfoCache", NULL, &dwType, (LPBYTE)&aai, &dwSize) == ERROR_SUCCESS) &&
             (aai.Size == sizeof(APPARPINFO)))
        {
            // Getting path to image
            wcscpy(szPath, aai.ImagePath);

            // Getting application size
            if (aai.AppSize < (ULONGLONG) 2000000000)
            {
                swprintf(szSize, L"%.2f", (float)aai.AppSize/(1024*1024));
            }
            else wcscpy(szSize,L"---");

            // Getting last used
            if (FileTimeToSystemTime(&aai.LastUsed, &systime))
            {
                if (SystemTimeToTzSpecificLocalTime(NULL, &systime, &localtime))
                {
                    if (((int)localtime.wYear > 1900) && ((int)localtime.wYear < 3000))
                    {
                        GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &localtime,
                                      NULL, szLastUsed, 256);
                    }
                    else wcscpy(szLastUsed,L"---");
                }
                else wcscpy(szLastUsed,L"---");
            }
            else wcscpy(szLastUsed,L"---");
        }
        else
        {
            wcscpy(szPath,L"---");
            wcscpy(szSize,L"---");
            wcscpy(szLastUsed,L"---");
        }
    }
    RegCloseKey(hKey);
}

/*
    AddItemToList - create application list

    hSubKey       - handle to the sub key for adding to LPARAM
    szDisplayName - display name
    ItemIndex     - item index
    AppName       - application name (for getting ARPCache info)
*/
static VOID
AddItemToList(LPARAM hSubKey, LPWSTR szDisplayName, INT ItemIndex, LPWSTR AppName)
{
    int index;
    HICON hIcon = NULL;
    LV_ITEM listItem;
    WCHAR IconPath[MAX_PATH], AppSize[256], LastUsed[256];
    int iIndex;

    GetARPInfo(AppName, IconPath, AppSize, LastUsed);

    if (GetFileAttributes(IconPath) != 0xFFFFFFFF)
    {
        // FIXME: This function not getting 32-bits icon
        ExtractIconEx(IconPath, 0, NULL, &hIcon, 1);
    }

    if (hIcon == NULL)
    {
        hIcon = LoadImage(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM), IMAGE_ICON, 16, 16, LR_CREATEDIBSECTION);
    }
    index = ImageList_AddIcon(hImageAppList, hIcon);
    DestroyIcon(hIcon);

    ZeroMemory(&listItem, sizeof(LV_ITEM));
    listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    listItem.pszText    = (LPTSTR)szDisplayName;
    listItem.lParam     = (LPARAM)hSubKey;
    listItem.iItem      = (int)ItemIndex;
    listItem.iImage     = index;
    iIndex = ListView_InsertItem(hAppList, &listItem);
    ListView_SetItemText(hAppList, iIndex, 1, LastUsed);
    ListView_SetItemText(hAppList, iIndex, 2, AppSize);
}

/*
    ShowMode:
        if ShowMode = 0 - programs and updates
        if ShowMode = 1 - show programs only
        if ShowMode = 2 - show updates only
*/
static VOID
FillSoftwareList(INT ShowMode)
{
    WCHAR pszName[MAX_PATH];
    WCHAR pszDisplayName[MAX_PATH];
    WCHAR pszParentKeyName[MAX_PATH];
    FILETIME FileTime;
    HKEY hKey;
    HKEY hSubKey;
    DWORD dwType;
    DWORD dwSize = MAX_PATH;
    DWORD dwValue = 0;
    BOOL bIsUpdate = FALSE;
    BOOL bIsSystemComponent = FALSE;
    INT ItemIndex = 0;
    DEVMODE pDevMode;
    int ColorDepth;

    (VOID) ImageList_Destroy(hImageAppList);
    (VOID) ListView_DeleteAllItems(hAppList);
  
    pDevMode.dmSize = sizeof(DEVMODE);
    pDevMode.dmDriverExtra = 0;
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &pDevMode);
    switch (pDevMode.dmBitsPerPel)
    {
        case 32: ColorDepth = ILC_COLOR32; break;
        case 24: ColorDepth = ILC_COLOR24; break;
        case 16: ColorDepth = ILC_COLOR16; break;
        case  8: ColorDepth = ILC_COLOR8;  break;
        case  4: ColorDepth = ILC_COLOR4;  break;
        default: ColorDepth = ILC_COLOR;   break;
    }
    
    hImageAppList = ImageList_Create(16, 16, ColorDepth | ILC_MASK, 0, 1);
    SendMessage(hAppList, WM_SETREDRAW, FALSE, 0);

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                   &hKey) != ERROR_SUCCESS)
    {
        WCHAR Buf[256];

        LoadString(hApplet, IDS_UNABLEOPEN_UNINSTKEY, Buf, sizeof(Buf) / sizeof(WCHAR));
        MessageBox(hMainWnd, Buf, NULL, MB_ICONWARNING);
        return;
    }

    ItemIndex = 0;
    dwSize = MAX_PATH;
    while (RegEnumKeyEx(hKey, ItemIndex, pszName, &dwSize, NULL, NULL, NULL, &FileTime) == ERROR_SUCCESS)
    {
        if (RegOpenKey(hKey,pszName,&hSubKey) == ERROR_SUCCESS)
        {
            dwType = REG_DWORD;
            dwSize = sizeof(DWORD);
            if (RegQueryValueEx(hSubKey, L"SystemComponent",
                                NULL, &dwType,
                                (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
            {
                bIsSystemComponent = (dwValue == 0x1);
            }
            else
            {
                bIsSystemComponent = FALSE;
            }

            dwType = REG_SZ;
            dwSize = MAX_PATH;
            bIsUpdate = (RegQueryValueEx(hSubKey, L"ParentKeyName",
                                         NULL, &dwType,
                                         (LPBYTE)pszParentKeyName,
                                         &dwSize) == ERROR_SUCCESS);
            dwSize = MAX_PATH;
            if (RegQueryValueEx(hSubKey, L"DisplayName",
                                NULL, &dwType,
                                (LPBYTE)pszDisplayName,
                                &dwSize) == ERROR_SUCCESS)
            {
                if ((ShowMode < 0)||(ShowMode > 2)) ShowMode = 0;
                if (!bIsSystemComponent)
                {
                    if (ShowMode == 0)
                    {
                        AddItemToList((LPARAM)hSubKey, (LPWSTR)pszDisplayName, ItemIndex, pszName);
                    }
                    if ((ShowMode == 1)&&(!bIsUpdate))
                    {
                        AddItemToList((LPARAM)hSubKey, (LPWSTR)pszDisplayName, ItemIndex, pszName);
                    }
                    if ((ShowMode == 2)&&(bIsUpdate))
                    {
                        AddItemToList((LPARAM)hSubKey, (LPWSTR)pszDisplayName, ItemIndex, pszName);
                    }
                }
            }
        }

        dwSize = MAX_PATH;
        ItemIndex++;
    }

    (VOID) ListView_SetImageList(hAppList, hImageAppList, LVSIL_SMALL);
    SendMessage(hAppList, WM_SETREDRAW, TRUE, 0);
    RegCloseKey(hSubKey);
    RegCloseKey(hKey);
}

static BOOL
GetAppString(LPCWSTR lpKeyName, LPWSTR lpString)
{
    HKEY hKey;
    INT nIndex;
    DWORD dwSize;
    DWORD dwType = REG_SZ;

    nIndex = (INT)SendMessage(hAppList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
    if (nIndex != -1)
    {
        LVITEM item;

        ZeroMemory(&item, sizeof(LVITEM));
        item.mask = LVIF_PARAM;
        item.iItem = nIndex;
        (VOID) ListView_GetItem(hAppList,&item);
        hKey = (HKEY)item.lParam;

        if (RegQueryValueEx(hKey, lpKeyName, NULL, &dwType,
                            (LPBYTE)lpString, &dwSize) == ERROR_SUCCESS)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static VOID
CallUninstall(VOID)
{
    INT nIndex;
    HKEY hKey;
    DWORD dwType, dwRet;
    WCHAR pszUninstallString[MAX_PATH];
    DWORD dwSize;
    MSG msg;

    nIndex = (INT)SendMessage(hAppList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
    if (nIndex != -1)
    {
        LVITEM item;

        ZeroMemory(&item, sizeof(LVITEM));
        item.mask = LVIF_PARAM;
        item.iItem = nIndex;
        (VOID) ListView_GetItem(hAppList,&item);
        hKey = (HKEY)item.lParam;

        dwType = REG_SZ;
        dwSize = MAX_PATH;
        if (RegQueryValueEx(hKey, L"UninstallString", NULL, &dwType,
                            (LPBYTE)pszUninstallString, &dwSize) == ERROR_SUCCESS)
        {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.wShowWindow = SW_SHOW;
            if (CreateProcess(NULL,pszUninstallString,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
            {
                CloseHandle(pi.hThread);
                EnableWindow(hMainWnd, FALSE);

                for (;;)
                {
                    dwRet = MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLEVENTS);
                    if (dwRet == WAIT_OBJECT_0 + 1)
                    {
                        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                        {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    }
                    else if (dwRet == WAIT_OBJECT_0 || dwRet == WAIT_FAILED)
                        break;
                }
                CloseHandle(pi.hProcess);

                EnableWindow(hMainWnd, TRUE);
                // Disable all buttons
                EnableWindow(hRemoveBtn,FALSE);
                EnableWindow(hModifyBtn,FALSE);
                // Update software list
                FillSoftwareList(0);
                SetActiveWindow(hMainWnd);
            }
        }
        else
        {
            WCHAR szBuf[256];

            LoadString(hApplet, IDS_UNABLEREAD_UNINSTSTR, szBuf, sizeof(szBuf) / sizeof(WCHAR));
            MessageBox(hMainWnd, szBuf, NULL, MB_ICONWARNING);
        }
    }
}

static VOID
ShowPopupMenu(HWND hwndDlg, INT xPos, INT yPos)
{
    INT nIndex;
    nIndex = (INT)SendMessage(hAppList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
    if ( nIndex != -1)
    {
        POINT pt;
        RECT lvRect;
        HMENU hMenu;

        GetCursorPos(&pt);

        GetWindowRect(hAppList, &lvRect);
        if (PtInRect(&lvRect, pt))
        {
            hMenu = GetSubMenu(LoadMenu(hApplet, MAKEINTRESOURCE(IDR_POPUP_APP)),0);
            TrackPopupMenuEx(hMenu, TPM_RIGHTBUTTON, xPos, yPos, hwndDlg, NULL);
            DestroyMenu(hMenu);
        }
    }
}

static VOID
GetAppInfo(LPWSTR lpInfo)
{
    WCHAR szBuf[1024], szDesc[1024];
    INT iIndex;
    HKEY hKey;

    iIndex = SendMessage(hAppList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
    if (iIndex != -1)
    {
        LVITEM item;
        DWORD dwSize = 2048;
        
        ZeroMemory(&item, sizeof(LVITEM));
        item.mask = LVIF_PARAM;
        item.iItem = iIndex;
        (VOID) ListView_GetItem(hAppList,&item);
        hKey = (HKEY)item.lParam;
        
        wcscpy(lpInfo,L"\0");
        if (RegQueryValueEx(hKey, L"Publisher", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_PUBLISHER, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            //wcscat(lpInfo, szDesc);
            swprintf(lpInfo, L"%s%s\n", szDesc, szBuf);
        }
        if (RegQueryValueEx(hKey, L"RegOwner", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_REG_OWNER, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscat(lpInfo, szDesc);
            swprintf(lpInfo,L"%s%s\n", lpInfo, szBuf);
        }
        if (RegQueryValueEx(hKey, L"ProductID", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_PRODUCT_ID, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscat(lpInfo, szDesc);
            swprintf(lpInfo,L"%s%s\n", lpInfo, szBuf);
        }
        if (RegQueryValueEx(hKey, L"DisplayVersion", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_VERSION, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscat(lpInfo, szDesc);
            swprintf(lpInfo,L"%s%s\n", lpInfo, szBuf);
        }
        if (RegQueryValueEx(hKey, L"Contact", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_CONTACT, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscat(lpInfo, szDesc);
            swprintf(lpInfo,L"%s%s\n", lpInfo, szBuf);
        }
        if (RegQueryValueEx(hKey, L"HelpLink", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_SUP_INFO, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscat(lpInfo, szDesc);
            swprintf(lpInfo,L"%s%s\n", lpInfo, szBuf);
        }
        if (RegQueryValueEx(hKey, L"HelpTelephone", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_SUP_PHONE, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscat(lpInfo, szDesc);
            swprintf(lpInfo,L"%s%s\n", lpInfo, szBuf);
        }
        if (RegQueryValueEx(hKey, L"URLUpdateInfo", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_PRODUCT_UPD, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscat(lpInfo, szDesc);
            swprintf(lpInfo,L"%s%s\n", lpInfo, szBuf);
        }
        if (RegQueryValueEx(hKey, L"Readme", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_README, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscat(lpInfo, szDesc);
            swprintf(lpInfo,L"%s%s\n", lpInfo, szBuf);
        }
        if (RegQueryValueEx(hKey, L"Comments", NULL, NULL, (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            LoadString(hApplet, IDS_INF_COMMENTS, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscat(lpInfo, szDesc);
            swprintf(lpInfo,L"%s%s\n", lpInfo, szBuf);
        }
        if (wcslen(lpInfo) < 10)
        {
            LoadString(hApplet, IDS_NO_INFORMATION, szDesc, sizeof(szDesc) / sizeof(WCHAR));
            wcscpy(lpInfo, szDesc);
        }
    }
}

static VOID
ShowAppInfo(VOID)
{
    WCHAR Info[2048], szBuf[1024];

    if (-1 != (INT) SendMessage(hAppList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED))
    {
        GetAppInfo(Info);
        ListView_GetItemText(hAppList, SendMessage(hAppList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED), 0, szBuf, sizeof(szBuf));
        ShowMessage(szBuf, Info);
    }
}

static VOID
GetListItemText(LPARAM lParam1, LPARAM lParam2, INT iSubItem, LPWSTR Item1, LPWSTR Item2)
{
    LVFINDINFO find;
    INT iIndex;

    find.flags = LVFI_PARAM;

    find.lParam = lParam1;
    iIndex = ListView_FindItem(hAppList, -1, &find);
    ListView_GetItemText(hAppList, iIndex, iSubItem, Item1, sizeof(Item1));

    find.lParam = lParam2;
    iIndex = ListView_FindItem(hAppList, -1, &find);
    ListView_GetItemText(hAppList, iIndex, iSubItem, Item2, sizeof(Item2));
}

static INT CALLBACK
CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    WCHAR szItem1[MAX_PATH], szItem2[MAX_PATH];

    switch ((INT)lParamSort)
    {
        case 0: // Name
        {
            GetListItemText(lParam1, lParam2, 0, szItem1, szItem2);
            if (bAscending == TRUE)
                return wcscmp(szItem2, szItem1);
            else
                return wcscmp(szItem1, szItem2);
        }
        case 1: // Date
            GetListItemText(lParam1, lParam2, 1, szItem1, szItem2);
            if (bAscending == TRUE)
                return wcscmp(szItem2, szItem1);
            else
                return wcscmp(szItem1, szItem2);
        case 2: // Size
        {
            // FIXME: No correct sorting by application size
            INT Size1, Size2;

            GetListItemText(lParam1, lParam2, 2, szItem1, szItem2);
            if (wcscmp(szItem1, L"---") == 0) wcscpy(szItem1, L"0");
            if (wcscmp(szItem2, L"---") == 0) wcscpy(szItem2, L"0");
            Size1 = _wtoi(szItem1);
            Size2 = _wtoi(szItem2);
            if (Size1 < Size2)
            {
                return (bAscending ? -1 : 1);
            }
            else if (Size1 == Size2)
            {
                return 0;
            }
            else if (Size1 > Size2)
            {
                return (bAscending ? 1 : -1);
            }
        }
    }
    
    return 0;
}

static BOOL
LoadSettings(VOID)
{
    HKEY hKey;
    DWORD dwSize;
    BOOL Ret;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\ReactOS\\AppWiz", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(APPWIZSETTINGS);
        if (RegQueryValueEx(hKey, L"Settings", NULL, NULL, (LPBYTE)&AppWizSettings, &dwSize) == ERROR_SUCCESS)
        {
            Ret = (AppWizSettings.Size == sizeof(APPWIZSETTINGS));
        }
        else Ret = FALSE;
    }
    else Ret = FALSE;

    RegCloseKey(hKey);
    return Ret;
}

static BOOL
SaveSettings(VOID)
{
    HKEY hKey;
    BOOL Ret;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\ReactOS\\AppWiz", 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        AppWizSettings.Size = sizeof(APPWIZSETTINGS);
        Ret = (RegSetValueEx(hKey, L"Settings", 0, REG_BINARY, (LPBYTE)&AppWizSettings, sizeof(APPWIZSETTINGS)) == ERROR_SUCCESS);
    }
    else Ret = FALSE;

    RegCloseKey(hKey);
    return Ret;
}

static void UpdateBitmap(HWND hWnd, RECT WndRect, RECT DescriptionRect)
{
    HDC hdc = GetDC(hWnd);

    if (!BackbufferHdc)
        BackbufferHdc = CreateCompatibleDC(hdc);

    if (BackbufferBmp)
        DeleteObject(BackbufferBmp);

    BackbufferBmp = CreateCompatibleBitmap(hdc, WndRect.right, WndRect.bottom);

    SelectObject(BackbufferHdc, BackbufferBmp);
    FillRect(BackbufferHdc, &WndRect, (HBRUSH)COLOR_APPWORKSPACE);
    DrawIconEx(BackbufferHdc, 153, 1, hSearchIcon, 24, 24, 0, NULL, DI_NORMAL|DI_COMPAT);
    DrawDescription(BackbufferHdc, DescriptionRect);
	
    ReleaseDC(hWnd, hdc);
}

static LRESULT CALLBACK
WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static RECT DescriptionRect;
    static RECT AppRect;
    WCHAR szBuf[1024];

    switch (Message)
    {
        case WM_CREATE:
        {
            hMainWnd = hwnd;
            MoveWindow(hMainWnd, AppWizSettings.Left, AppWizSettings.Top,
                       AppWizSettings.Right - AppWizSettings.Left,
                       AppWizSettings.Bottom - AppWizSettings.Top, TRUE);
            if (AppWizSettings.Maximized) ShowWindow(hMainWnd, SW_MAXIMIZE);
            ShowMessage(Strings[0],Strings[1]); // Welcome message
            InitControls();
        }
        break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case ID_APP_REMOVE:
                    CallUninstall();
                break;
            }
            if(HIWORD(wParam) == BN_CLICKED)
            {
                if (lParam == (LPARAM)hRemoveBtn)
                {
                    CallUninstall();
                }
            }
            if (lParam == (LPARAM)hSearch)
            {
                switch (HIWORD(wParam))
                {
                    case EN_SETFOCUS:
                    {
                        WCHAR Tmp[1024];

                        LoadString(hApplet, IDS_SEARCH_TEXT, szBuf, sizeof(szBuf) / sizeof(WCHAR));
                        GetWindowText(hSearch, Tmp, 1024);
                        if (wcscmp(szBuf, Tmp) == 0) SetWindowText(hSearch, L"");
                    }
                    break;
                    case EN_KILLFOCUS:
                    {
                        GetWindowText(hSearch, szBuf, 1024);
                        if (wcslen(szBuf) < 1)
                        {
                            LoadString(hApplet, IDS_SEARCH_TEXT, szBuf, sizeof(szBuf) / sizeof(WCHAR));
                            SetWindowText(hSearch, szBuf);
                        }
                    }
                    break;
                    case EN_CHANGE:
                    break;
                }
            }
        }
        break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            BitBlt(hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, BackbufferHdc, 0, 0, SRCCOPY);
            EndPaint(hwnd, &ps);
        }
        break;
        case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;
            
            switch (data->code)
            {
                case TVN_SELCHANGED:
                    if(data->hwndFrom == hActList)
                    {
                        // Add items to programs list
                        FillSoftwareList(((LPNMTREEVIEW)lParam)->itemNew.lParam);
                        // Set default titile and message
                        ShowMessage(Strings[0], Strings[1]);
                        // Disable all buttons
                        EnableWindow(hRemoveBtn, FALSE);
                        EnableWindow(hModifyBtn, FALSE);
                    }
                break;
                case NM_CLICK:
                    if(data->hwndFrom == hAppList)
                    {
                        if (-1 != (INT) SendMessage(hAppList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED))
                        {
                            EnableWindow(hRemoveBtn, TRUE);
                            if (GetAppString(L"ModifyPath", NULL))
                                EnableWindow(hModifyBtn, TRUE);
                        }
                        ShowAppInfo();
                        UpdateBitmap(hwnd, AppRect, DescriptionRect);
                        InvalidateRect(hwnd, &DescriptionRect, FALSE);
                    }
                break;
                case NM_DBLCLK:
                    if(data->hwndFrom == hAppList)
                    {
                        if (-1 != (INT) SendMessage(hAppList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED))
                        {
                            CallUninstall();
                        }
                    }
                break;
                case LVN_COLUMNCLICK:
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
                    (VOID) ListView_SortItems(hAppList, CompareFunc, pnmv->iSubItem);
                    bAscending = !bAscending;
                }
                break;
            }
        }
        break;
        case WM_SIZING:
        {
            LPRECT pRect = (LPRECT)lParam;
            if (pRect->right-pRect->left < 520)
                pRect->right = pRect->left + 520;

            if (pRect->bottom-pRect->top < 400)
                pRect->bottom = pRect->top + 400;
        }
        break;
        case WM_SIZE:
        {    
            RECT Rect = {1, HIWORD(lParam)-178, LOWORD(lParam)-1, HIWORD(lParam)-1};
            SetRect(&AppRect, 0, 0, LOWORD(lParam)-1, HIWORD(lParam)-1);
            DescriptionRect = Rect;

            // Actions list
            ResizeControl(hActList, 0, 1, 150, HIWORD(lParam)-180);

            // Applications list
            ResizeControl(hAppList, 152, 25, LOWORD(lParam), HIWORD(lParam)-180);

            // Search Edit
            ResizeControl(hSearch, 180, 1, LOWORD(lParam), 25);

            // Buttons
            MoveWindow(hRemoveBtn, LOWORD(lParam)-105, HIWORD(lParam)-30, 100, 25, TRUE); // Remove button
            MoveWindow(hModifyBtn, LOWORD(lParam)-208, HIWORD(lParam)-30, 100, 25, TRUE); // Modify button
            
            (VOID) ListView_SetColumnWidth(hAppList, 0, LOWORD(lParam)-330);
            
            // Update title and info
            ShowAppInfo();
            UpdateBitmap(hwnd, AppRect, DescriptionRect);
            InvalidateRect(hwnd, &DescriptionRect, FALSE);
        }
        break;
        case WM_ACTIVATEAPP:
            ShowAppInfo();
        break;
        case WM_CONTEXTMENU:
        {
            // Show popup menu for programs list
            ShowPopupMenu(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
        case WM_DESTROY:
        {
            WINDOWPLACEMENT wp;

            // Save settings
            ShowWindow(hMainWnd, SW_HIDE);
            wp.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(hMainWnd, &wp);
            AppWizSettings.Left   = wp.rcNormalPosition.left;
            AppWizSettings.Top    = wp.rcNormalPosition.top;
            AppWizSettings.Right  = wp.rcNormalPosition.right;
            AppWizSettings.Bottom = wp.rcNormalPosition.bottom;
            AppWizSettings.Maximized = (IsZoomed(hMainWnd) || (wp.flags & WPF_RESTORETOMAXIMIZED));
            SaveSettings();
            // Destroy all and quit
            if (BackbufferHdc)
                DeleteDC(BackbufferHdc);
            if (BackbufferBmp)
                DeleteObject(BackbufferBmp);
            DeleteObject(hMainFont);
            DeleteObject(hSearchIcon);
            PostQuitMessage(0);
        }
        break;
    }
    return DefWindowProc(hwnd, Message, wParam, lParam);
}

static VOID
InitSettings(VOID)
{
    if (!LoadSettings())
    {
        AppWizSettings.Maximized = FALSE;
        AppWizSettings.Left      = 0;
        AppWizSettings.Top       = 0;
        AppWizSettings.Right     = 520;
        AppWizSettings.Bottom    = 400;
    }
}

static INT
MainWindowCreate(VOID)
{
    WNDCLASS WndClass = {0};
    MSG msg;
    WCHAR szBuf[256];

    // Load welcome strings
    LoadString(hApplet, IDS_WELCOME_TITLE, Strings[0], sizeof(szBuf) / sizeof(WCHAR));
    LoadString(hApplet, IDS_WELCOME_MSG, Strings[1], sizeof(szBuf) / sizeof(WCHAR));

    InitSettings();

    // Create the window
    WndClass.lpszClassName  = L"rosappwiz";
    WndClass.lpfnWndProc    = (WNDPROC)WndProc;
    WndClass.hInstance      = hApplet;
    WndClass.style          = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon          = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
    WndClass.hCursor        = LoadCursor(hApplet, IDC_ARROW);
    WndClass.hbrBackground  = (HBRUSH)COLOR_BTNFACE + 1;

    if (!RegisterClass(&WndClass)) return 0;

    LoadString(hApplet, IDS_CPLSYSTEMNAME, szBuf, 256);
    hMainWnd = CreateWindow(L"rosappwiz", szBuf,
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE | WS_CAPTION,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            0, 0, NULL, NULL, hApplet, NULL); 

    // Show it
    ShowWindow(hMainWnd, SW_SHOW);
    UpdateWindow(hMainWnd);

    // Message Loop
    while(GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    CPLINFO *CPlInfo;
    DWORD i;

    UNREFERENCED_PARAMETER(hwndCPl);

    i = (DWORD)lParam1;
    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
            CPlInfo = (CPLINFO*)lParam2;
            CPlInfo->lData = 0;
            CPlInfo->idIcon = IDI_CPLSYSTEM;
            CPlInfo->idName = IDS_CPLSYSTEMNAME;
            CPlInfo->idInfo = IDS_CPLSYSTEMDESCRIPTION;
            break;

        case CPL_DBLCLK:
            MainWindowCreate();
            break;
    }

    return FALSE;
}


BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            CoInitialize(NULL);
            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}
