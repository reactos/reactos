/*
 * Regedit child window
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
 * Copyright (C) 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 * LICENSE: LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 */

#include "regedit.h"
#include <shldisp.h>
#include <shlguid.h>

ChildWnd* g_pChildWnd;
static int last_split;
HBITMAP SizingPattern;
HBRUSH  SizingBrush;
WCHAR Suggestions[256];

static HRESULT WINAPI DummyEnumStringsQI(LPVOID This, REFIID riid, void**ppv)
{
    if (ppv)
        *ppv = NULL;
    if (IsEqualIID(riid, &IID_IEnumString) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = This;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI DummyEnumStringsAddRefRelease(LPVOID This)
{
    return 1;
}

static HRESULT WINAPI DummyEnumStringsNext(LPVOID This, ULONG celt, LPWSTR *parr, ULONG *pceltFetched)
{
    if (pceltFetched)
        *pceltFetched = 0;
    return S_FALSE;
}

static HRESULT WINAPI DummyEnumStringsSkip(LPVOID This, ULONG celt)
{
    return S_OK;
}

static HRESULT WINAPI DummyEnumStringsReset(LPVOID This)
{
    return S_OK;
}

static HRESULT WINAPI DummyEnumStringsClone(LPVOID This, void**ppv)
{
    return E_NOTIMPL;
}

struct DummyEnumStringsVtbl {
    LPVOID QI, AddRef, Release, Next, Skip, Reset, Clone;
} g_DummyEnumStringsVtbl = {
    &DummyEnumStringsQI,
    &DummyEnumStringsAddRefRelease,
    &DummyEnumStringsAddRefRelease,
    &DummyEnumStringsNext,
    &DummyEnumStringsSkip,
    &DummyEnumStringsReset,
    &DummyEnumStringsClone
};

struct DummyEnumStrings {
    struct DummyEnumStringsVtbl *lpVtbl;
} g_DummyEnumStrings = {
    &g_DummyEnumStringsVtbl
};

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

static INT ClampSplitBarX(HWND hWnd, INT x)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    return min(max(x, SPLIT_MIN), rc.right - SPLIT_MIN);
}

extern void ResizeWnd(int cx, int cy)
{
    HDWP hdwp = BeginDeferWindowPos(4);
    RECT rt, rs, rb;
    const int nButtonWidth = 44;
    const int nButtonHeight = 22;
    int cyEdge = GetSystemMetrics(SM_CYEDGE);
    const UINT uFlags = SWP_NOZORDER | SWP_NOACTIVATE;
    SetRect(&rt, 0, 0, cx, cy);
    cy = 0;
    if (hStatusBar != NULL)
    {
        GetWindowRect(hStatusBar, &rs);
        cy = rs.bottom - rs.top;
    }
    GetWindowRect(g_pChildWnd->hAddressBtnWnd, &rb);

    g_pChildWnd->nSplitPos = ClampSplitBarX(g_pChildWnd->hWnd, g_pChildWnd->nSplitPos);

    cx = g_pChildWnd->nSplitPos + SPLIT_WIDTH / 2;
    if (hdwp)
        hdwp = DeferWindowPos(hdwp, g_pChildWnd->hAddressBarWnd, NULL,
                              rt.left, rt.top,
                              rt.right - rt.left - nButtonWidth, nButtonHeight,
                              uFlags);
    if (hdwp)
        hdwp = DeferWindowPos(hdwp, g_pChildWnd->hAddressBtnWnd, NULL,
                              rt.right - nButtonWidth, rt.top,
                              nButtonWidth, nButtonHeight,
                              uFlags);
    if (hdwp)
        hdwp = DeferWindowPos(hdwp, g_pChildWnd->hTreeWnd, NULL,
                              rt.left,
                              rt.top + nButtonHeight + cyEdge,
                              g_pChildWnd->nSplitPos - SPLIT_WIDTH/2 - rt.left,
                              rt.bottom - rt.top - cy - 2 * cyEdge,
                              uFlags);
    if (hdwp)
        hdwp = DeferWindowPos(hdwp, g_pChildWnd->hListWnd, NULL,
                              rt.left + cx,
                              rt.top + nButtonHeight + cyEdge,
                              rt.right - cx,
                              rt.bottom - rt.top - cy - 2 * cyEdge,
                              uFlags);
    if (hdwp)
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

/**
 * make the splitbar invisible and resize the windows (helper for ChildWndProc)
 */
static void finish_splitbar(HWND hWnd, int x)
{
    RECT rt;

    draw_splitbar(hWnd, last_split);
    last_split = -1;
    GetClientRect(hWnd, &rt);
    g_pChildWnd->nSplitPos = x;
    ResizeWnd(rt.right, rt.bottom);
    InvalidateRect(hWnd, &rt, FALSE); // HACK: See CORE-19576
    ReleaseCapture();
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
    const REGSAM regsam = KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;

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
                                 szBuffer, ARRAY_SIZE(szBuffer)) == ERROR_SUCCESS)
            {
                /* Sanity check this key; it cannot be empty, nor can it be a
                 * loop back */
                if ((szBuffer[0] != L'\0') && _wcsicmp(szBuffer, pszKeyPath))
                {
                    if (RegOpenKeyExW(hRootKey, szBuffer, 0, regsam, &hOtherKey) == ERROR_SUCCESS)
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
                        StringCbCopyW(szLastFound, sizeof(szLastFound), szBuffer);
                        pszKeyPath = szLastFound;
                    }
                }
            }
        }
        while(bFound && (iSuggestionsLength > 0));

        /* Check CLSID key */
        if (RegOpenKeyExW(hRootKey, pszKeyPath, 0, regsam, &hSubKey) == ERROR_SUCCESS)
        {
            if (QueryStringValue(hSubKey, L"CLSID", NULL, szBuffer,
                                 ARRAY_SIZE(szBuffer)) == ERROR_SUCCESS)
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
    else if ((hRootKey == HKEY_CURRENT_USER || hRootKey == HKEY_LOCAL_MACHINE) && *pszKeyPath)
    {
        LPCWSTR rootstr = hRootKey == HKEY_CURRENT_USER ? L"HKLM" : L"HKCU";
        hOtherKey = hRootKey == HKEY_CURRENT_USER ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        if (RegOpenKeyExW(hOtherKey, pszKeyPath, 0, regsam, &hSubKey) == ERROR_SUCCESS)
        {
            int cch;
            RegCloseKey(hSubKey);
            cch = _snwprintf(pszSuggestions, iSuggestionsLength, L"%s\\%s", rootstr, pszKeyPath);
            if (cch <= 0 || cch > iSuggestionsLength)
                pszSuggestions[0] = UNICODE_NULL;
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
            GetWindowTextW(hwnd, s_szNode, ARRAY_SIZE(s_szNode));
            SelectNode(g_pChildWnd->hTreeWnd, s_szNode);
        }
        break;
    default:
        break;
    }
    return CallWindowProcW(oldwndproc, hwnd, uMsg, wParam, lParam);
}

VOID
UpdateAddress(HTREEITEM hItem, HKEY hRootKey, LPCWSTR pszPath, BOOL bSelectNone)
{
    LPCWSTR keyPath, rootName;
    LPWSTR fullPath;
    DWORD cbFullPath;

    /* Wipe the listview, the status bar and the address bar if the root key was selected */
    if (TreeView_GetParent(g_pChildWnd->hTreeWnd, hItem) == NULL)
    {
        ListView_DeleteAllItems(g_pChildWnd->hListWnd);
        SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)NULL);
        SendMessageW(g_pChildWnd->hAddressBarWnd, WM_SETTEXT, 0, (LPARAM)NULL);
        return;
    }

    if (pszPath == NULL)
        keyPath = GetItemPath(g_pChildWnd->hTreeWnd, hItem, &hRootKey);
    else
        keyPath = pszPath;

    if (keyPath)
    {
        RefreshListView(g_pChildWnd->hListWnd, hRootKey, keyPath, bSelectNone);
        rootName = get_root_key_name(hRootKey);
        cbFullPath = (wcslen(rootName) + 1 + wcslen(keyPath) + 1) * sizeof(WCHAR);
        fullPath = malloc(cbFullPath);
        if (fullPath)
        {
            /* set (correct) the address bar text */
            if (keyPath[0] != UNICODE_NULL)
                StringCbPrintfW(fullPath, cbFullPath, L"%s%s%s", rootName,
                                ((keyPath[0] == L'\\') ? L"" : L"\\"), keyPath);
            else
                StringCbCopyW(fullPath, cbFullPath, rootName);

            SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)fullPath);
            SendMessageW(g_pChildWnd->hAddressBarWnd, WM_SETTEXT, 0, (LPARAM)fullPath);
            free(fullPath);

            /* disable hive manipulation items temporarily (enable only if necessary) */
            EnableMenuItem(hMenuFrame, ID_REGISTRY_LOADHIVE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenuFrame, ID_REGISTRY_UNLOADHIVE, MF_BYCOMMAND | MF_GRAYED);
            /* compare the strings to see if we should enable/disable the "Load Hive" menus accordingly */
            if (_wcsicmp(rootName, L"HKEY_LOCAL_MACHINE") == 0 ||
                _wcsicmp(rootName, L"HKEY_USERS") == 0)
            {
                /*
                 * enable the unload menu item if at the root, otherwise
                 * enable the load menu item if there is no slash in
                 * keyPath (ie. immediate child selected)
                 */
                if (keyPath[0] == UNICODE_NULL)
                    EnableMenuItem(hMenuFrame, ID_REGISTRY_LOADHIVE, MF_BYCOMMAND | MF_ENABLED);
                else if (!wcschr(keyPath, L'\\'))
                    EnableMenuItem(hMenuFrame, ID_REGISTRY_UNLOADHIVE, MF_BYCOMMAND | MF_ENABLED);
            }
        }
    }
}

/**
 * PURPOSE: Processes messages for the child windows.
 *
 * WM_COMMAND - process the application menu
 * WM_DESTROY - post a quit message and return
 */
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL Result;
    RECT rc;

    switch (message)
    {
    case WM_CREATE:
    {
        WNDPROC oldproc;
        HFONT hFont;
        WCHAR buffer[MAX_PATH];
        DWORD style;
        IAutoComplete *pAutoComplete;

        /* Load "My Computer" string */
        LoadStringW(hInst, IDS_MY_COMPUTER, buffer, ARRAY_SIZE(buffer));

        g_pChildWnd = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ChildWnd));
        if (!g_pChildWnd) return 0;

        wcsncpy(g_pChildWnd->szPath, buffer, MAX_PATH);
        g_pChildWnd->nSplitPos = 190;
        g_pChildWnd->hWnd = hWnd;

        /* ES_AUTOHSCROLL style enables horizontal scrolling and shrinking */
        style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
        g_pChildWnd->hAddressBarWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", NULL, style,
                                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                                      hWnd, (HMENU)0, hInst, 0);

        style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_ICON | BS_CENTER |
                BS_VCENTER | BS_FLAT | BS_DEFPUSHBUTTON;
        g_pChildWnd->hAddressBtnWnd = CreateWindowExW(0, L"Button", L"\x00BB", style,
                                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                                      hWnd, (HMENU)0, hInst, 0);
        g_pChildWnd->hArrowIcon = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_ARROW),
                                                    IMAGE_ICON, 12, 12, 0);
        SendMessageW(g_pChildWnd->hAddressBtnWnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_pChildWnd->hArrowIcon);

        if (SUCCEEDED(CoCreateInstance(&CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, &IID_IAutoComplete, (void**)&pAutoComplete)))
        {
            IAutoComplete_Init(pAutoComplete, g_pChildWnd->hAddressBarWnd, (IUnknown*)&g_DummyEnumStrings, NULL, NULL);
            IAutoComplete_Release(pAutoComplete);
        }

        GetClientRect(hWnd, &rc);
        g_pChildWnd->hTreeWnd = CreateTreeView(hWnd, g_pChildWnd->szPath, (HMENU) TREE_WINDOW);
        g_pChildWnd->hListWnd = CreateListView(hWnd, (HMENU) LIST_WINDOW, rc.right - g_pChildWnd->nSplitPos);
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
        break; //goto def;
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
        DestroyIcon(g_pChildWnd->hArrowIcon);
        HeapFree(GetProcessHeap(), 0, g_pChildWnd);
        g_pChildWnd = NULL;
        PostQuitMessage(0);
        break;

    case WM_LBUTTONDOWN:
    {
        INT x = (SHORT)LOWORD(lParam);
        if (x >= g_pChildWnd->nSplitPos - SPLIT_WIDTH / 2 &&
            x <  g_pChildWnd->nSplitPos + SPLIT_WIDTH / 2 + 1)
        {
            x = ClampSplitBarX(hWnd, x);
            draw_splitbar(hWnd, x);
            last_split = x;
            SetCapture(hWnd);
        }
        break;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
        if (GetCapture() == hWnd)
        {
            INT x = (SHORT)LOWORD(lParam);
            x = ClampSplitBarX(hWnd, x);
            finish_splitbar(hWnd, x);
        }
        break;

    case WM_CAPTURECHANGED:
        if (GetCapture() == hWnd && last_split >= 0)
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
            INT x = (SHORT)LOWORD(lParam);
            x = ClampSplitBarX(hWnd, x);
            if (last_split != x)
            {
                draw_splitbar(hWnd, last_split);
                last_split = x;
                draw_splitbar(hWnd, last_split);
            }
        }
        break;

    case WM_SETFOCUS:
        if (g_pChildWnd != NULL)
        {
            SetFocus(g_pChildWnd->nFocusPanel? g_pChildWnd->hListWnd: g_pChildWnd->hTreeWnd);
        }
        break;

    case WM_NOTIFY:
        if (g_pChildWnd == NULL) break;

        if (((LPNMHDR)lParam)->idFrom == TREE_WINDOW)
        {
            if (!TreeWndNotifyProc(g_pChildWnd->hListWnd, wParam, lParam, &Result))
            {
                goto def;
            }

            return Result;
        }
        else
        {
            if (((LPNMHDR)lParam)->idFrom == LIST_WINDOW)
            {
                if (!ListWndNotifyProc(g_pChildWnd->hListWnd, wParam, lParam, &Result))
                {
                    goto def;
                }

                return Result;
            }
            else
            {
                goto def;
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
            BOOL isRoot;

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
                TreeView_HitTest(g_pChildWnd->hTreeWnd, &hti);
            }

            if (hti.flags & TVHT_ONITEM)
            {
                TreeView_SelectItem(g_pChildWnd->hTreeWnd, hti.hItem);

                isRoot = (TreeView_GetParent(g_pChildWnd->hTreeWnd, hti.hItem) == NULL);
                hContextMenu = GetSubMenu(hPopupMenus, isRoot ?  PM_ROOTITEM : PM_TREECONTEXT);

                memset(&item, 0, sizeof(item));
                item.mask = TVIF_STATE | TVIF_CHILDREN;
                item.hItem = hti.hItem;
                TreeView_GetItem(g_pChildWnd->hTreeWnd, &item);

                /* Set the Expand/Collapse menu item appropriately */
                LoadStringW(hInst, (item.state & TVIS_EXPANDED) ? IDS_COLLAPSE : IDS_EXPAND, buffer, ARRAY_SIZE(buffer));
                memset(&mii, 0, sizeof(mii));
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_STRING | MIIM_STATE | MIIM_ID;
                mii.fState = (item.cChildren > 0) ? MFS_DEFAULT : MFS_GRAYED;
                mii.wID = (item.state & TVIS_EXPANDED) ? ID_TREE_COLLAPSEBRANCH : ID_TREE_EXPANDBRANCH;
                mii.dwTypeData = (LPWSTR) buffer;
                SetMenuItemInfo(hContextMenu, 0, TRUE, &mii);

                if (isRoot == FALSE)
                {
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
                    SuggestKeys(hRootKey, keyPath, Suggestions, ARRAY_SIZE(Suggestions));
                    if (Suggestions[0])
                    {
                        AppendMenu(hContextMenu, MF_SEPARATOR, 0, NULL);

                        LoadStringW(hInst, IDS_GOTO_SUGGESTED_KEY, resource, ARRAY_SIZE(resource));

                        s = Suggestions;
                        wID = ID_TREE_SUGGESTION_MIN;
                        while(*s && (wID <= ID_TREE_SUGGESTION_MAX))
                        {
                            WCHAR *path = s, buf[MAX_PATH];
                            if (hRootKey == HKEY_CURRENT_USER || hRootKey == HKEY_LOCAL_MACHINE)
                            {
                                // Windows 10 only displays the root name
                                LPCWSTR next = PathFindNextComponentW(s);
                                if (next > s)
                                    lstrcpynW(path = buf, s, min(next - s, _countof(buf)));
                            }
                            _snwprintf(buffer, ARRAY_SIZE(buffer), resource, path);

                            memset(&mii, 0, sizeof(mii));
                            mii.cbSize = sizeof(mii);
                            mii.fMask = MIIM_STRING | MIIM_ID;
                            mii.wID = wID++;
                            mii.dwTypeData = buffer;
                            InsertMenuItem(hContextMenu, GetMenuItemCount(hContextMenu), TRUE, &mii);

                            s += wcslen(s) + 1;
                        }
                    }
                }
                TrackPopupMenu(hContextMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
            }
        }
        break;
    }

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && g_pChildWnd != NULL)
        {
            ResizeWnd(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    default:
def:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}
