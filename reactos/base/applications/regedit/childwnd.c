/*
 * Regedit child window
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "regedit.h"

ChildWnd* g_pChildWnd;
static int last_split;
HBITMAP SizingPattern = 0;
HBRUSH  SizingBrush = 0;
static WCHAR Suggestions[256];

extern LPCWSTR get_root_key_name(HKEY hRootKey)
{
    if (hRootKey == HKEY_CLASSES_ROOT) return L"HKEY_CLASSES_ROOT";
    if (hRootKey == HKEY_CURRENT_USER) return L"HKEY_CURRENT_USER";
    if (hRootKey == HKEY_LOCAL_MACHINE) return L"HKEY_LOCAL_MACHINE";
    if (hRootKey == HKEY_USERS) return L"HKEY_USERS";
    if (hRootKey == HKEY_CURRENT_CONFIG) return L"HKEY_CURRENT_CONFIG";
    if (hRootKey == HKEY_DYN_DATA) return L"HKEY_DYN_DATA";

    return L"UNKNOWN HKEY, PLEASE REPORT";
}

extern void ResizeWnd(int cx, int cy)
{
    HDWP hdwp = BeginDeferWindowPos(3);
    RECT rt, rs, rb;
    const int tHeight = 22;
    SetRect(&rt, 0, 0, cx, cy);
    cy = 0;
    if (hStatusBar != NULL)
    {
        GetWindowRect(hStatusBar, &rs);
        cy = rs.bottom - rs.top;
    }
    GetWindowRect(g_pChildWnd->hAddressBtnWnd, &rb);
    cx = g_pChildWnd->nSplitPos + SPLIT_WIDTH/2;
    DeferWindowPos(hdwp, g_pChildWnd->hAddressBarWnd, 0, rt.left, rt.top, rt.right-rt.left - 2*tHeight, tHeight, SWP_NOZORDER|SWP_NOACTIVATE);
    DeferWindowPos(hdwp, g_pChildWnd->hAddressBtnWnd, 0, rt.right - 2*tHeight, rt.top, 2*tHeight, tHeight, SWP_NOZORDER|SWP_NOACTIVATE);
    DeferWindowPos(hdwp, g_pChildWnd->hTreeWnd, 0, rt.left, rt.top + tHeight+2, g_pChildWnd->nSplitPos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top-cy, SWP_NOZORDER|SWP_NOACTIVATE);
    DeferWindowPos(hdwp, g_pChildWnd->hListWnd, 0, rt.left+cx, rt.top + tHeight+2, rt.right-cx, rt.bottom-rt.top-cy, SWP_NOZORDER|SWP_NOACTIVATE);
    EndDeferWindowPos(hdwp);
}

/*******************************************************************************
 * Local module support methods
 */

static void draw_splitbar(HWND hWnd, int x)
{
    RECT rt;
    HGDIOBJ OldObj;
    HDC hdc = GetDC(hWnd);

    if(!SizingPattern)
    {
        const DWORD Pattern[4] = {0x5555AAAA, 0x5555AAAA, 0x5555AAAA, 0x5555AAAA};
        SizingPattern = CreateBitmap(8, 8, 1, 1, Pattern);
    }
    if(!SizingBrush)
    {
        SizingBrush = CreatePatternBrush(SizingPattern);
    }
    GetClientRect(hWnd, &rt);
    rt.left = x - SPLIT_WIDTH/2;
    rt.right = x + SPLIT_WIDTH/2+1;
    OldObj = SelectObject(hdc, SizingBrush);
    PatBlt(hdc, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
    SelectObject(hdc, OldObj);
    ReleaseDC(hWnd, hdc);
}

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    RECT rt;

    GetClientRect(hWnd, &rt);
    BeginPaint(hWnd, &ps);
    FillRect(ps.hdc, &rt, GetSysColorBrush(COLOR_BTNFACE));
    EndPaint(hWnd, &ps);
}

/*******************************************************************************
 * finish_splitbar [internal]
 *
 * make the splitbar invisible and resize the windows
 * (helper for ChildWndProc)
 */
static void finish_splitbar(HWND hWnd, int x)
{
    RECT rt;

    draw_splitbar(hWnd, last_split);
    last_split = -1;
    GetClientRect(hWnd, &rt);
    g_pChildWnd->nSplitPos = x;
    ResizeWnd(rt.right, rt.bottom);
    ReleaseCapture();
}

/*******************************************************************************
 *
 *  FUNCTION: _CmdWndProc(HWND, unsigned, WORD, LONG)
 *
 *  PURPOSE:  Processes WM_COMMAND messages for the main frame window.
 *
 */

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HTREEITEM hSelection;
    HKEY hRootKey;
    LPCWSTR keyPath, s;
    WORD wID = LOWORD(wParam);

    UNREFERENCED_PARAMETER(message);

    switch (wID)
    {
        /* Parse the menu selections: */
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        /* TODO */
        break;
    case ID_TREE_EXPANDBRANCH:
        (void)TreeView_Expand(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd), TVE_EXPAND);
        break;
    case ID_TREE_COLLAPSEBRANCH:
        (void)TreeView_Expand(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd), TVE_COLLAPSE);
        break;
    case ID_TREE_RENAME:
        SetFocus(g_pChildWnd->hTreeWnd);
        (void)TreeView_EditLabel(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd));
        break;
    case ID_TREE_DELETE:
        hSelection = TreeView_GetSelection(g_pChildWnd->hTreeWnd);
        keyPath = GetItemPath(g_pChildWnd->hTreeWnd, hSelection, &hRootKey);

        if (keyPath == 0 || *keyPath == 0)
        {
            MessageBeep(MB_ICONHAND);
        }
        else if (DeleteKey(hWnd, hRootKey, keyPath))
            DeleteNode(g_pChildWnd->hTreeWnd, 0);
        break;
    case ID_TREE_EXPORT:
        ExportRegistryFile(g_pChildWnd->hTreeWnd);
        break;
    case ID_EDIT_FIND:
        FindDialog(hWnd);
        break;
    case ID_EDIT_COPYKEYNAME:
        hSelection = TreeView_GetSelection(g_pChildWnd->hTreeWnd);
        keyPath = GetItemPath(g_pChildWnd->hTreeWnd, hSelection, &hRootKey);
        CopyKeyName(hWnd, hRootKey, keyPath);
        break;
    case ID_EDIT_NEW_KEY:
        CreateNewKey(g_pChildWnd->hTreeWnd, TreeView_GetSelection(g_pChildWnd->hTreeWnd));
        break;
    case ID_EDIT_NEW_STRINGVALUE:
    case ID_EDIT_NEW_BINARYVALUE:
    case ID_EDIT_NEW_DWORDVALUE:
        SendMessageW(hFrameWnd, WM_COMMAND, wParam, lParam);
        break;
    case ID_SWITCH_PANELS:
        g_pChildWnd->nFocusPanel = !g_pChildWnd->nFocusPanel;
        SetFocus(g_pChildWnd->nFocusPanel? g_pChildWnd->hListWnd: g_pChildWnd->hTreeWnd);
        break;
    default:
        if ((wID >= ID_TREE_SUGGESTION_MIN) && (wID <= ID_TREE_SUGGESTION_MAX))
        {
            s = Suggestions;
            while(wID > ID_TREE_SUGGESTION_MIN)
            {
                if (*s)
                    s += wcslen(s) + 1;
                wID--;
            }
            SelectNode(g_pChildWnd->hTreeWnd, s);
            break;
        }
        return FALSE;
    }
    return TRUE;
}

/*******************************************************************************
 *
 *  Key suggestion
 */

#define MIN(a,b)    ((a < b) ? (a) : (b))

static void SuggestKeys(HKEY hRootKey, LPCWSTR pszKeyPath, LPWSTR pszSuggestions,
                        size_t iSuggestionsLength)
{
    WCHAR szBuffer[256];
    WCHAR szLastFound[256];
    size_t i;
    HKEY hOtherKey, hSubKey;
    BOOL bFound;

    memset(pszSuggestions, 0, iSuggestionsLength * sizeof(*pszSuggestions));
    iSuggestionsLength--;

    /* Are we a root key in HKEY_CLASSES_ROOT? */
    if ((hRootKey == HKEY_CLASSES_ROOT) && pszKeyPath[0] && !wcschr(pszKeyPath, L'\\'))
    {
        do
        {
            bFound = FALSE;

            /* Check default key */
            if (QueryStringValue(hRootKey, pszKeyPath, NULL,
                                 szBuffer, COUNT_OF(szBuffer)) == ERROR_SUCCESS)
            {
                /* Sanity check this key; it cannot be empty, nor can it be a
                 * loop back */
                if ((szBuffer[0] != L'\0') && _wcsicmp(szBuffer, pszKeyPath))
                {
                    if (RegOpenKeyW(hRootKey, szBuffer, &hOtherKey) == ERROR_SUCCESS)
                    {
                        lstrcpynW(pszSuggestions, L"HKCR\\", (int) iSuggestionsLength);
                        i = wcslen(pszSuggestions);
                        pszSuggestions += i;
                        iSuggestionsLength -= i;

                        lstrcpynW(pszSuggestions, szBuffer, (int) iSuggestionsLength);
                        i = MIN(wcslen(pszSuggestions) + 1, iSuggestionsLength);
                        pszSuggestions += i;
                        iSuggestionsLength -= i;
                        RegCloseKey(hOtherKey);

                        bFound = TRUE;
                        wcscpy(szLastFound, szBuffer);
                        pszKeyPath = szLastFound;
                    }
                }
            }
        }
        while(bFound && (iSuggestionsLength > 0));

        /* Check CLSID key */
        if (RegOpenKeyW(hRootKey, pszKeyPath, &hSubKey) == ERROR_SUCCESS)
        {
            if (QueryStringValue(hSubKey, L"CLSID", NULL, szBuffer,
                                 COUNT_OF(szBuffer)) == ERROR_SUCCESS)
            {
                lstrcpynW(pszSuggestions, L"HKCR\\CLSID\\", (int)iSuggestionsLength);
                i = wcslen(pszSuggestions);
                pszSuggestions += i;
                iSuggestionsLength -= i;

                lstrcpynW(pszSuggestions, szBuffer, (int)iSuggestionsLength);
                i = MIN(wcslen(pszSuggestions) + 1, iSuggestionsLength);
                pszSuggestions += i;
                iSuggestionsLength -= i;
            }
            RegCloseKey(hSubKey);
        }
    }
}


LRESULT CALLBACK AddressBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldwndproc;
    static WCHAR s_szNode[256];
    oldwndproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_KEYUP:
        if (wParam == VK_RETURN)
        {
            GetWindowTextW(hwnd, s_szNode, COUNT_OF(s_szNode));
            SelectNode(g_pChildWnd->hTreeWnd, s_szNode);
        }
        break;
    default:
        break;
    }
    return CallWindowProcW(oldwndproc, hwnd, uMsg, wParam, lParam);
}

static VOID
UpdateAddress(HTREEITEM hItem, HKEY hRootKey, LPCWSTR pszPath)
{
    LPCWSTR keyPath, rootName;
    LPWSTR fullPath;

    if (pszPath == NULL)
        keyPath = GetItemPath(g_pChildWnd->hTreeWnd, hItem, &hRootKey);
    else
        keyPath = pszPath;

    if (keyPath)
    {
        RefreshListView(g_pChildWnd->hListWnd, hRootKey, keyPath);
        rootName = get_root_key_name(hRootKey);
        fullPath = HeapAlloc(GetProcessHeap(), 0, (wcslen(rootName) + 1 + wcslen(keyPath) + 1) * sizeof(WCHAR));
        if (fullPath)
        {
            /* set (correct) the address bar text */
            if (keyPath[0] != L'\0')
                swprintf(fullPath, L"%s\\%s", rootName, keyPath);
            else
                fullPath = wcscpy(fullPath, rootName);
            SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)fullPath);
            SendMessageW(g_pChildWnd->hAddressBarWnd, WM_SETTEXT, 0, (LPARAM)fullPath);
            HeapFree(GetProcessHeap(), 0, fullPath);
            /* disable hive manipulation items temporarily (enable only if necessary) */
            EnableMenuItem(GetSubMenu(hMenuFrame,0), ID_REGISTRY_LOADHIVE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(GetSubMenu(hMenuFrame,0), ID_REGISTRY_UNLOADHIVE, MF_BYCOMMAND | MF_GRAYED);
            /* compare the strings to see if we should enable/disable the "Load Hive" menus accordingly */
            if (!(_wcsicmp(rootName, L"HKEY_LOCAL_MACHINE") &&
                  _wcsicmp(rootName, L"HKEY_USERS")))
            {
                /*
                 * enable the unload menu item if at the root, otherwise
                 * enable the load menu item if there is no slash in
                 * keyPath (ie. immediate child selected)
                 */
                if(keyPath[0] == L'\0')
                    EnableMenuItem(GetSubMenu(hMenuFrame,0), ID_REGISTRY_LOADHIVE, MF_BYCOMMAND | MF_ENABLED);
                else if(!wcschr(keyPath, L'\\'))
                    EnableMenuItem(GetSubMenu(hMenuFrame,0), ID_REGISTRY_UNLOADHIVE, MF_BYCOMMAND | MF_ENABLED);
            }
        }
    }
    else
    {
        (void)ListView_DeleteAllItems(g_pChildWnd->hListWnd);
        SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)NULL);
        SendMessageW(g_pChildWnd->hAddressBarWnd, WM_SETTEXT, 0, (LPARAM)NULL);
    }
}

/*******************************************************************************
 *
 *  FUNCTION: ChildWndProc(HWND, unsigned, WORD, LONG)
 *
 *  PURPOSE:  Processes messages for the child windows.
 *
 *  WM_COMMAND  - process the application menu
 *  WM_PAINT    - Paint the main window
 *  WM_DESTROY  - post a quit message and return
 *
 */
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL Result;

    switch (message)
    {
    case WM_CREATE:
    {
        WNDPROC oldproc;
        HFONT hFont;
        WCHAR buffer[MAX_PATH];

        /* Load "My Computer" string */
        LoadStringW(hInst, IDS_MY_COMPUTER, buffer, COUNT_OF(buffer));

        g_pChildWnd = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ChildWnd));
        if (!g_pChildWnd) return 0;

        wcsncpy(g_pChildWnd->szPath, buffer, MAX_PATH);
        g_pChildWnd->nSplitPos = 250;
        g_pChildWnd->hWnd = hWnd;
        g_pChildWnd->hAddressBarWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", NULL, WS_CHILD | WS_VISIBLE | WS_CHILDWINDOW | WS_TABSTOP,
                                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                                      hWnd, (HMENU)0, hInst, 0);
        g_pChildWnd->hAddressBtnWnd = CreateWindowExW(0, L"Button", L"\x00BB", WS_CHILD | WS_VISIBLE | WS_CHILDWINDOW | WS_TABSTOP | BS_TEXT | BS_CENTER | BS_VCENTER | BS_FLAT | BS_DEFPUSHBUTTON,
                                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                                      hWnd, (HMENU)0, hInst, 0);
        g_pChildWnd->hTreeWnd = CreateTreeView(hWnd, g_pChildWnd->szPath, (HMENU) TREE_WINDOW);
        g_pChildWnd->hListWnd = CreateListView(hWnd, (HMENU) LIST_WINDOW/*, g_pChildWnd->szPath*/);
        SetFocus(g_pChildWnd->hTreeWnd);

        /* set the address bar and button font */
        if ((g_pChildWnd->hAddressBarWnd) && (g_pChildWnd->hAddressBtnWnd))
        {
            hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SendMessageW(g_pChildWnd->hAddressBarWnd,
                         WM_SETFONT,
                         (WPARAM)hFont,
                         0);
            SendMessageW(g_pChildWnd->hAddressBtnWnd,
                         WM_SETFONT,
                         (WPARAM)hFont,
                         0);
        }
        /* Subclass the AddressBar */
        oldproc = (WNDPROC)GetWindowLongPtr(g_pChildWnd->hAddressBarWnd, GWLP_WNDPROC);
        SetWindowLongPtr(g_pChildWnd->hAddressBarWnd, GWLP_USERDATA, (DWORD_PTR)oldproc);
        SetWindowLongPtr(g_pChildWnd->hAddressBarWnd, GWLP_WNDPROC, (DWORD_PTR)AddressBarProc);
        break;
    }
    case WM_COMMAND:
        if(HIWORD(wParam) == BN_CLICKED)
        {
            PostMessageW(g_pChildWnd->hAddressBarWnd, WM_KEYUP, VK_RETURN, 0);
        }

        if (!_CmdWndProc(hWnd, message, wParam, lParam))
        {
            goto def;
        }
        break;
    case WM_PAINT:
        OnPaint(hWnd);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT)
        {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            if (pt.x>=g_pChildWnd->nSplitPos-SPLIT_WIDTH/2 && pt.x<g_pChildWnd->nSplitPos+SPLIT_WIDTH/2+1)
            {
                SetCursor(LoadCursorW(0, IDC_SIZEWE));
                return TRUE;
            }
        }
        goto def;
    case WM_DESTROY:
        DestroyListView(g_pChildWnd->hListWnd);
        DestroyTreeView(g_pChildWnd->hTreeWnd);
        DestroyMainMenu();
        HeapFree(GetProcessHeap(), 0, g_pChildWnd);
        g_pChildWnd = NULL;
        PostQuitMessage(0);
        break;
    case WM_LBUTTONDOWN:
    {
        RECT rt;
        int x = (short)LOWORD(lParam);
        GetClientRect(hWnd, &rt);
        if (x>=g_pChildWnd->nSplitPos-SPLIT_WIDTH/2 && x<g_pChildWnd->nSplitPos+SPLIT_WIDTH/2+1)
        {
            last_split = g_pChildWnd->nSplitPos;
            draw_splitbar(hWnd, last_split);
            SetCapture(hWnd);
        }
        break;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
        if (GetCapture() == hWnd)
        {
            finish_splitbar(hWnd, LOWORD(lParam));
        }
        break;

    case WM_CAPTURECHANGED:
        if (GetCapture()==hWnd && last_split>=0)
            draw_splitbar(hWnd, last_split);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            if (GetCapture() == hWnd)
            {
                RECT rt;
                draw_splitbar(hWnd, last_split);
                GetClientRect(hWnd, &rt);
                ResizeWnd(rt.right, rt.bottom);
                last_split = -1;
                ReleaseCapture();
                SetCursor(LoadCursorW(0, IDC_ARROW));
            }
        break;

    case WM_MOUSEMOVE:
        if (GetCapture() == hWnd)
        {
            HDC hdc;
            RECT rt;
            HGDIOBJ OldObj;
            int x = LOWORD(lParam);
            if(!SizingPattern)
            {
                const DWORD Pattern[4] = {0x5555AAAA, 0x5555AAAA, 0x5555AAAA, 0x5555AAAA};
                SizingPattern = CreateBitmap(8, 8, 1, 1, Pattern);
            }
            if(!SizingBrush)
            {
                SizingBrush = CreatePatternBrush(SizingPattern);
            }

            GetClientRect(hWnd, &rt);
            x = (SHORT) min(max(x, SPLIT_MIN), rt.right - SPLIT_MIN);
            if(last_split != x)
            {
                rt.left = last_split-SPLIT_WIDTH/2;
                rt.right = last_split+SPLIT_WIDTH/2+1;
                hdc = GetDC(hWnd);
                OldObj = SelectObject(hdc, SizingBrush);
                PatBlt(hdc, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
                last_split = x;
                rt.left = x-SPLIT_WIDTH/2;
                rt.right = x+SPLIT_WIDTH/2+1;
                PatBlt(hdc, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
                SelectObject(hdc, OldObj);
                ReleaseDC(hWnd, hdc);
            }
        }
        break;

    case WM_SETFOCUS:
        if (g_pChildWnd != NULL)
        {
            SetFocus(g_pChildWnd->nFocusPanel? g_pChildWnd->hListWnd: g_pChildWnd->hTreeWnd);
        }
        break;

    case WM_TIMER:
        break;

    case WM_NOTIFY:
        if ((int)wParam == TREE_WINDOW && g_pChildWnd != NULL)
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case TVN_ITEMEXPANDING:
                return !OnTreeExpanding(g_pChildWnd->hTreeWnd, (NMTREEVIEW*)lParam);
            case TVN_SELCHANGED:
            {
                NMTREEVIEW* pnmtv = (NMTREEVIEW*)lParam;
                /* Get the parent of the current item */
                HTREEITEM hParentItem = TreeView_GetParent(g_pChildWnd->hTreeWnd, pnmtv->itemNew.hItem);

                UpdateAddress(pnmtv->itemNew.hItem, NULL, NULL);

                /*
                 * Disable Delete/Rename menu options for 'My Computer' (first item so doesn't have any parent)
                 * and HKEY_* keys (their parent is 'My Computer' and the previous remark applies).
                 */
                if (!hParentItem || !TreeView_GetParent(g_pChildWnd->hTreeWnd, hParentItem))
                {
                    EnableMenuItem(hMenuFrame , ID_EDIT_DELETE, MF_BYCOMMAND | MF_GRAYED);
                    EnableMenuItem(hMenuFrame , ID_EDIT_RENAME, MF_BYCOMMAND | MF_GRAYED);
                    EnableMenuItem(hPopupMenus, ID_TREE_DELETE, MF_BYCOMMAND | MF_GRAYED);
                    EnableMenuItem(hPopupMenus, ID_TREE_RENAME, MF_BYCOMMAND | MF_GRAYED); 
                }
                else
                {
                    EnableMenuItem(hMenuFrame , ID_EDIT_DELETE, MF_BYCOMMAND | MF_ENABLED);
                    EnableMenuItem(hMenuFrame , ID_EDIT_RENAME, MF_BYCOMMAND | MF_ENABLED);
                    EnableMenuItem(hPopupMenus, ID_TREE_DELETE, MF_BYCOMMAND | MF_ENABLED);
                    EnableMenuItem(hPopupMenus, ID_TREE_RENAME, MF_BYCOMMAND | MF_ENABLED);
                }

                break;
            }
            case NM_SETFOCUS:
                g_pChildWnd->nFocusPanel = 0;
                break;
            case TVN_BEGINLABELEDIT:
            {
                LPNMTVDISPINFO ptvdi;
                /* cancel label edit for rootkeys  */
                ptvdi = (LPNMTVDISPINFO) lParam;
                if (!TreeView_GetParent(g_pChildWnd->hTreeWnd, ptvdi->item.hItem) ||
                    !TreeView_GetParent(g_pChildWnd->hTreeWnd, TreeView_GetParent(g_pChildWnd->hTreeWnd, ptvdi->item.hItem)))
                    return TRUE;
                break;
            }
            case TVN_ENDLABELEDIT:
            {
                LPCWSTR keyPath;
                HKEY hRootKey;
                HKEY hKey = NULL;
                LPNMTVDISPINFO ptvdi;
                LONG lResult = TRUE;
                WCHAR szBuffer[MAX_PATH];

                ptvdi = (LPNMTVDISPINFO) lParam;
                if (ptvdi->item.pszText)
                {
                    keyPath = GetItemPath(g_pChildWnd->hTreeWnd, TreeView_GetParent(g_pChildWnd->hTreeWnd, ptvdi->item.hItem), &hRootKey);
                    _snwprintf(szBuffer, COUNT_OF(szBuffer), L"%s\\%s", keyPath, ptvdi->item.pszText);
                    keyPath = GetItemPath(g_pChildWnd->hTreeWnd, ptvdi->item.hItem, &hRootKey);
                    if (RegOpenKeyExW(hRootKey, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                    {
                        lResult = FALSE;
                        RegCloseKey(hKey);
                        (void)TreeView_EditLabel(g_pChildWnd->hTreeWnd, ptvdi->item.hItem);
                    }
                    else
                    {
                        if (RenameKey(hRootKey, keyPath, ptvdi->item.pszText) != ERROR_SUCCESS)
                            lResult = FALSE;
                        else
                            UpdateAddress(ptvdi->item.hItem, hRootKey, szBuffer);
                    }
                    return lResult;
                }
            }
            default:
                return 0;
            }
        }
        else
        {
            if ((int)wParam == LIST_WINDOW && g_pChildWnd != NULL)
            {
                switch (((LPNMHDR)lParam)->code)
                {
                case NM_SETFOCUS:
                    g_pChildWnd->nFocusPanel = 1;
                    break;
                default:
                    if(!ListWndNotifyProc(g_pChildWnd->hListWnd, wParam, lParam, &Result))
                    {
                        goto def;
                    }
                    return Result;
                    break;
                }
            }
        }
        break;

    case WM_CONTEXTMENU:
    {
        POINT pt;
        if((HWND)wParam == g_pChildWnd->hListWnd)
        {
            int i, cnt;
            BOOL IsDefault;
            pt.x = (short) LOWORD(lParam);
            pt.y = (short) HIWORD(lParam);
            cnt = ListView_GetSelectedCount(g_pChildWnd->hListWnd);
            i = ListView_GetNextItem(g_pChildWnd->hListWnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
            if (pt.x == -1 && pt.y == -1)
            {
                RECT rc;
                if (i != -1)
                {
                    rc.left = LVIR_BOUNDS;
                    SendMessageW(g_pChildWnd->hListWnd, LVM_GETITEMRECT, i, (LPARAM) &rc);
                    pt.x = rc.left + 8;
                    pt.y = rc.top + 8;
                }
                else
                    pt.x = pt.y = 0;
                ClientToScreen(g_pChildWnd->hListWnd, &pt);
            }
            if(i == -1)
            {
                TrackPopupMenu(GetSubMenu(hPopupMenus, PM_NEW), TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
            }
            else
            {
                HMENU mnu = GetSubMenu(hPopupMenus, PM_MODIFYVALUE);
                SetMenuDefaultItem(mnu, ID_EDIT_MODIFY, MF_BYCOMMAND);
                IsDefault = IsDefaultValue(g_pChildWnd->hListWnd, i);
                if(cnt == 1)
                    EnableMenuItem(mnu, ID_EDIT_RENAME, MF_BYCOMMAND | (IsDefault ? MF_DISABLED | MF_GRAYED : MF_ENABLED));
                else
                    EnableMenuItem(mnu, ID_EDIT_RENAME, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                EnableMenuItem(mnu, ID_EDIT_MODIFY, MF_BYCOMMAND | (cnt == 1 ? MF_ENABLED : MF_DISABLED | MF_GRAYED));
                EnableMenuItem(mnu, ID_EDIT_MODIFY_BIN, MF_BYCOMMAND | (cnt == 1 ? MF_ENABLED : MF_DISABLED | MF_GRAYED));

                TrackPopupMenu(mnu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
            }
        }
        else if ((HWND)wParam == g_pChildWnd->hTreeWnd)
        {
            TVHITTESTINFO hti;
            HMENU hContextMenu;
            TVITEMW item;
            MENUITEMINFOW mii;
            WCHAR resource[256];
            WCHAR buffer[256];
            LPWSTR s;
            LPCWSTR keyPath;
            HKEY hRootKey;
            int iLastPos;
            WORD wID;

            pt.x = (short) LOWORD(lParam);
            pt.y = (short) HIWORD(lParam);

            if (pt.x == -1 && pt.y == -1)
            {
                RECT rc;
                hti.hItem = TreeView_GetSelection(g_pChildWnd->hTreeWnd);
                if (hti.hItem != NULL)
                {
                    TreeView_GetItemRect(g_pChildWnd->hTreeWnd, hti.hItem, &rc, TRUE);
                    pt.x = rc.left + 8;
                    pt.y = rc.top + 8;
                    ClientToScreen(g_pChildWnd->hTreeWnd, &pt);
                    hti.flags = TVHT_ONITEM;
                }
                else
                    hti.flags = 0;
            }
            else
            {
                hti.pt.x = pt.x;
                hti.pt.y = pt.y;
                ScreenToClient(g_pChildWnd->hTreeWnd, &hti.pt);
                (void)TreeView_HitTest(g_pChildWnd->hTreeWnd, &hti);
            }

            if (hti.flags & TVHT_ONITEM)
            {
                hContextMenu = GetSubMenu(hPopupMenus, PM_TREECONTEXT);
                (void)TreeView_SelectItem(g_pChildWnd->hTreeWnd, hti.hItem);

                memset(&item, 0, sizeof(item));
                item.mask = TVIF_STATE | TVIF_CHILDREN;
                item.hItem = hti.hItem;
                (void)TreeView_GetItem(g_pChildWnd->hTreeWnd, &item);

                /* Set the Expand/Collapse menu item appropriately */
                LoadStringW(hInst, (item.state & TVIS_EXPANDED) ? IDS_COLLAPSE : IDS_EXPAND, buffer, COUNT_OF(buffer));
                memset(&mii, 0, sizeof(mii));
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_STRING | MIIM_STATE | MIIM_ID;
                mii.fState = (item.cChildren > 0) ? MFS_DEFAULT : MFS_GRAYED;
                mii.wID = (item.state & TVIS_EXPANDED) ? ID_TREE_COLLAPSEBRANCH : ID_TREE_EXPANDBRANCH;
                mii.dwTypeData = (LPWSTR) buffer;
                SetMenuItemInfo(hContextMenu, 0, TRUE, &mii);

                /* Remove any existing suggestions */
                memset(&mii, 0, sizeof(mii));
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_ID;
                GetMenuItemInfo(hContextMenu, GetMenuItemCount(hContextMenu) - 1, TRUE, &mii);
                if ((mii.wID >= ID_TREE_SUGGESTION_MIN) && (mii.wID <= ID_TREE_SUGGESTION_MAX))
                {
                    do
                    {
                        iLastPos = GetMenuItemCount(hContextMenu) - 1;
                        GetMenuItemInfo(hContextMenu, iLastPos, TRUE, &mii);
                        RemoveMenu(hContextMenu, iLastPos, MF_BYPOSITION);
                    }
                    while((mii.wID >= ID_TREE_SUGGESTION_MIN) && (mii.wID <= ID_TREE_SUGGESTION_MAX));
                }

                /* Come up with suggestions */
                keyPath = GetItemPath(g_pChildWnd->hTreeWnd, NULL, &hRootKey);
                SuggestKeys(hRootKey, keyPath, Suggestions, COUNT_OF(Suggestions));
                if (Suggestions[0])
                {
                    AppendMenu(hContextMenu, MF_SEPARATOR, 0, NULL);

                    LoadStringW(hInst, IDS_GOTO_SUGGESTED_KEY, resource, COUNT_OF(resource));

                    s = Suggestions;
                    wID = ID_TREE_SUGGESTION_MIN;
                    while(*s && (wID <= ID_TREE_SUGGESTION_MAX))
                    {
                        _snwprintf(buffer, COUNT_OF(buffer), resource, s);

                        memset(&mii, 0, sizeof(mii));
                        mii.cbSize = sizeof(mii);
                        mii.fMask = MIIM_STRING | MIIM_ID;
                        mii.wID = wID++;
                        mii.dwTypeData = buffer;
                        InsertMenuItem(hContextMenu, GetMenuItemCount(hContextMenu), TRUE, &mii);

                        s += wcslen(s) + 1;
                    }
                }
                TrackPopupMenu(hContextMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_pChildWnd->hWnd, NULL);
            }
        }
        break;
    }

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && g_pChildWnd != NULL)
        {
            ResizeWnd(LOWORD(lParam), HIWORD(lParam));
        }
        /* fall through */
    default:
def:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}
