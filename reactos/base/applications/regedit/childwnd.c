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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <regedit.h>

ChildWnd* g_pChildWnd;
HBITMAP SizingPattern = 0;
HBRUSH  SizingBrush = 0;
static TCHAR Suggestions[256];

/*******************************************************************************
 * Local module support methods
 */

static LPCTSTR get_root_key_name(HKEY hRootKey)
{
    if (hRootKey == HKEY_CLASSES_ROOT) return _T("HKEY_CLASSES_ROOT");
    if (hRootKey == HKEY_CURRENT_USER) return _T("HKEY_CURRENT_USER");
    if (hRootKey == HKEY_LOCAL_MACHINE) return _T("HKEY_LOCAL_MACHINE");
    if (hRootKey == HKEY_USERS) return _T("HKEY_USERS");
    if (hRootKey == HKEY_CURRENT_CONFIG) return _T("HKEY_CURRENT_CONFIG");
    if (hRootKey == HKEY_DYN_DATA) return _T("HKEY_DYN_DATA");
    return _T("UKNOWN HKEY, PLEASE REPORT");
}

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

static void ResizeWnd(ChildWnd* pChildWnd, int cx, int cy)
{
    HDWP hdwp = BeginDeferWindowPos(2);
    RECT rt, rs;

    SetRect(&rt, 0, 0, cx, cy);
    cy = 0;
    if (hStatusBar != NULL) {
        GetWindowRect(hStatusBar, &rs);
        cy = rs.bottom - rs.top + 8;
    }
    cx = pChildWnd->nSplitPos + SPLIT_WIDTH/2;
	DeferWindowPos(hdwp, pChildWnd->hAddressBarWnd, 0, rt.left, rt.top, rt.right-rt.left, 23, SWP_NOZORDER|SWP_NOACTIVATE);
    DeferWindowPos(hdwp, pChildWnd->hTreeWnd, 0, rt.left, rt.top + 25, pChildWnd->nSplitPos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top-cy, SWP_NOZORDER|SWP_NOACTIVATE);
    DeferWindowPos(hdwp, pChildWnd->hListWnd, 0, rt.left+cx  , rt.top + 25, rt.right-cx, rt.bottom-rt.top-cy, SWP_NOZORDER|SWP_NOACTIVATE);
    EndDeferWindowPos(hdwp);
}

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    RECT rt;
    HDC hdc;

    GetClientRect(hWnd, &rt);
    hdc = BeginPaint(hWnd, &ps);
    FillRect(ps.hdc, &rt, GetSysColorBrush(COLOR_BTNFACE));
    EndPaint(hWnd, &ps);
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
    ChildWnd* pChildWnd = g_pChildWnd;
    HTREEITEM hSelection;
    HKEY hRootKey;
    LPCTSTR keyPath, s;
    WORD wID = LOWORD(wParam);

    UNREFERENCED_PARAMETER(message);

    switch (wID) {
        /* Parse the menu selections: */
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        /* TODO */
        break;
    case ID_TREE_EXPANDBRANCH:
        (void)TreeView_Expand(pChildWnd->hTreeWnd, TreeView_GetSelection(pChildWnd->hTreeWnd), TVE_EXPAND);
        break;
    case ID_TREE_COLLAPSEBRANCH:
        (void)TreeView_Expand(pChildWnd->hTreeWnd, TreeView_GetSelection(pChildWnd->hTreeWnd), TVE_COLLAPSE);
        break;
    case ID_TREE_RENAME:
        SetFocus(pChildWnd->hTreeWnd);
        (void)TreeView_EditLabel(pChildWnd->hTreeWnd, TreeView_GetSelection(pChildWnd->hTreeWnd));
        break;
    case ID_TREE_DELETE:
        hSelection = TreeView_GetSelection(pChildWnd->hTreeWnd);
        keyPath = GetItemPath(pChildWnd->hTreeWnd, hSelection, &hRootKey);

        if (keyPath == 0 || *keyPath == 0)
        {
           MessageBeep(MB_ICONHAND); 
        } else 
        if (DeleteKey(hWnd, hRootKey, keyPath))
          DeleteNode(g_pChildWnd->hTreeWnd, 0);
        break;
	case ID_TREE_EXPORT:
        ExportRegistryFile(pChildWnd->hTreeWnd);
        break;
	case ID_EDIT_FIND:
        FindDialog(hWnd);
        break;
    case ID_EDIT_COPYKEYNAME:
        hSelection = TreeView_GetSelection(pChildWnd->hTreeWnd);
        keyPath = GetItemPath(pChildWnd->hTreeWnd, hSelection, &hRootKey);
        CopyKeyName(hWnd, hRootKey, keyPath);
        break;
    case ID_EDIT_NEW_KEY:
        CreateNewKey(pChildWnd->hTreeWnd, TreeView_GetSelection(pChildWnd->hTreeWnd));
        break;
    case ID_EDIT_NEW_STRINGVALUE:
    case ID_EDIT_NEW_BINARYVALUE:
    case ID_EDIT_NEW_DWORDVALUE:
        SendMessage(hFrameWnd, WM_COMMAND, wParam, lParam);
        break;
    case ID_SWITCH_PANELS:
        pChildWnd->nFocusPanel = !pChildWnd->nFocusPanel;
        SetFocus(pChildWnd->nFocusPanel? pChildWnd->hListWnd: pChildWnd->hTreeWnd);
        break;
    default:
        if ((wID >= ID_TREE_SUGGESTION_MIN) && (wID <= ID_TREE_SUGGESTION_MAX))
		{
            s = Suggestions;
            while(wID > ID_TREE_SUGGESTION_MIN)
            {
                if (*s)
                    s += _tcslen(s) + 1;
				wID--;
            }
            SelectNode(pChildWnd->hTreeWnd, s);
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

#define MIN(a,b)	((a < b) ? (a) : (b))

static void SuggestKeys(HKEY hRootKey, LPCTSTR pszKeyPath, LPTSTR pszSuggestions,
	size_t iSuggestionsLength)
{
	TCHAR szBuffer[256];
	TCHAR szLastFound[256];
	size_t i;
	HKEY hOtherKey, hSubKey;
	BOOL bFound;

	memset(pszSuggestions, 0, iSuggestionsLength * sizeof(*pszSuggestions));
	iSuggestionsLength--;

	/* Are we a root key in HKEY_CLASSES_ROOT? */
	if ((hRootKey == HKEY_CLASSES_ROOT) && pszKeyPath[0] && !_tcschr(pszKeyPath, '\\'))
	{
		do
		{
			bFound = FALSE;

			/* Check default key */
			if (RegQueryStringValue(hRootKey, pszKeyPath, NULL,
				szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])) == ERROR_SUCCESS)
			{
				/* Sanity check this key; it cannot be empty, nor can it be a
				 * loop back */
				if ((szBuffer[0] != '\0') && _tcsicmp(szBuffer, pszKeyPath))
				{
					if (RegOpenKey(hRootKey, szBuffer, &hOtherKey) == ERROR_SUCCESS)
					{
						lstrcpyn(pszSuggestions, TEXT("HKCR\\"), (int) iSuggestionsLength);
						i = _tcslen(pszSuggestions);
						pszSuggestions += i;
	    				iSuggestionsLength -= i;

						lstrcpyn(pszSuggestions, szBuffer, (int) iSuggestionsLength);
						i = MIN(_tcslen(pszSuggestions) + 1, iSuggestionsLength);
						pszSuggestions += i;
						iSuggestionsLength -= i;
						RegCloseKey(hOtherKey);

						bFound = TRUE;
						_tcscpy(szLastFound, szBuffer);
						pszKeyPath = szLastFound;
					}
				}
			}
		}
		while(bFound && (iSuggestionsLength > 0));

		/* Check CLSID key */
		if (RegOpenKey(hRootKey, pszKeyPath, &hSubKey) == ERROR_SUCCESS)
		{
			if (RegQueryStringValue(hSubKey, TEXT("CLSID"), NULL,
				szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])) == ERROR_SUCCESS)
			{
				lstrcpyn(pszSuggestions, TEXT("HKCR\\CLSID\\"), (int) iSuggestionsLength);
				i = _tcslen(pszSuggestions);
				pszSuggestions += i;
				iSuggestionsLength -= i;

				lstrcpyn(pszSuggestions, szBuffer, (int) iSuggestionsLength);
				i = MIN(_tcslen(pszSuggestions) + 1, iSuggestionsLength);
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
	static TCHAR s_szNode[256];
    oldwndproc = (WNDPROC)(LONG_PTR)GetWindowLongPtr(hwnd, GWL_USERDATA);

	switch (uMsg)
    {
		case WM_KEYUP:
			if (wParam == VK_RETURN)
			{
				GetWindowText(hwnd, s_szNode, sizeof(s_szNode) / sizeof(s_szNode[0]));
				SelectNode(g_pChildWnd->hTreeWnd, s_szNode);
			}
			break;
		default:
			break;
	}
	return CallWindowProc(oldwndproc, hwnd, uMsg, wParam, lParam);
}

/* fix coords to top-left when SHIFT-F10 is pressed */
void FixPointIfContext(POINTS *pt, HWND hWnd)
{
    if (pt->x == -1 && pt->y == -1) {
        POINT p = { 0, 0 };
        ClientToScreen(hWnd, &p);
        pt->x = (WORD)(p.x);
        pt->y = (WORD)(p.y);
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
    static int last_split;
    BOOL Result;
    ChildWnd* pChildWnd = g_pChildWnd;

    switch (message) {
    case WM_CREATE:
    {
		WNDPROC oldproc;
        HFONT hFont;
        TCHAR buffer[MAX_PATH];
        /* load "My Computer" string */
        LoadString(hInst, IDS_MY_COMPUTER, buffer, sizeof(buffer)/sizeof(TCHAR));

	    g_pChildWnd = pChildWnd = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ChildWnd));
		
        if (!pChildWnd) return 0;
        _tcsncpy(pChildWnd->szPath, buffer, MAX_PATH);
        pChildWnd->nSplitPos = 250;
        pChildWnd->hWnd = hWnd;
		pChildWnd->hAddressBarWnd = CreateWindowEx(WS_EX_CLIENTEDGE, _T("Edit"), NULL, WS_CHILD | WS_VISIBLE | WS_CHILDWINDOW | WS_TABSTOP, 
										CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
										hWnd, (HMENU)0, hInst, 0);
		pChildWnd->hTreeWnd = CreateTreeView(hWnd, pChildWnd->szPath, (HMENU) TREE_WINDOW);
        pChildWnd->hListWnd = CreateListView(hWnd, (HMENU) LIST_WINDOW/*, pChildWnd->szPath*/);
        SetFocus(pChildWnd->hTreeWnd);

        /* set the address bar font */
        if (pChildWnd->hAddressBarWnd)
        {
            hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SendMessage(pChildWnd->hAddressBarWnd,
                        WM_SETFONT,
                        (WPARAM)hFont,
                        0);
        }
		
		/* Subclass the AddressBar */
		oldproc = (WNDPROC)(LONG_PTR)GetWindowLongPtr(pChildWnd->hAddressBarWnd, GWL_WNDPROC);
        SetWindowLongPtr(pChildWnd->hAddressBarWnd, GWL_USERDATA, (DWORD_PTR)oldproc);
        SetWindowLongPtr(pChildWnd->hAddressBarWnd, GWL_WNDPROC, (DWORD_PTR)AddressBarProc);
		break;
    }
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
            goto def;
        }
        break;
    case WM_PAINT:
        OnPaint(hWnd);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            if (pt.x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && pt.x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
                SetCursor(LoadCursor(0, IDC_SIZEWE));
                return TRUE;
            }
        }
        goto def;
    case WM_DESTROY:
		DestroyTreeView();
		DestroyListView(pChildWnd->hListWnd);
		DestroyMainMenu();
        HeapFree(GetProcessHeap(), 0, pChildWnd);
        pChildWnd = NULL;
        PostQuitMessage(0);
        break;
    case WM_LBUTTONDOWN: {
            RECT rt;
            POINTS pt;
            pt = MAKEPOINTS(lParam);
            GetClientRect(hWnd, &rt);
            if (pt.x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && pt.x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
                last_split = pChildWnd->nSplitPos;
                draw_splitbar(hWnd, last_split);
                SetCapture(hWnd);
            }
            break;
        }

    case WM_LBUTTONUP:
        if (GetCapture() == hWnd) {
            RECT rt;
            POINTS pt;
            pt = MAKEPOINTS(lParam);
            GetClientRect(hWnd, &rt);
            pt.x = (SHORT) min(max(pt.x, SPLIT_MIN), rt.right - SPLIT_MIN);
            draw_splitbar(hWnd, last_split);
            last_split = -1;
            pChildWnd->nSplitPos = pt.x;
            ResizeWnd(pChildWnd, rt.right, rt.bottom);
            ReleaseCapture();
        }
        break;

    case WM_CAPTURECHANGED:
        if (GetCapture()==hWnd && last_split>=0)
            draw_splitbar(hWnd, last_split);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            if (GetCapture() == hWnd) {
                RECT rt;
                draw_splitbar(hWnd, last_split);
                GetClientRect(hWnd, &rt);
                ResizeWnd(pChildWnd, rt.right, rt.bottom);
                last_split = -1;
                ReleaseCapture();
                SetCursor(LoadCursor(0, IDC_ARROW));
            }
        break;

    case WM_MOUSEMOVE:
        if (GetCapture() == hWnd) {
            HDC hdc;
            RECT rt;
            HGDIOBJ OldObj;
            POINTS pt;
            if(!SizingPattern)
            {
              const DWORD Pattern[4] = {0x5555AAAA, 0x5555AAAA, 0x5555AAAA, 0x5555AAAA};
              SizingPattern = CreateBitmap(8, 8, 1, 1, Pattern);
            }
            if(!SizingBrush)
            {
              SizingBrush = CreatePatternBrush(SizingPattern);
            }

            pt = MAKEPOINTS(lParam);
            GetClientRect(hWnd, &rt);
            pt.x = (SHORT) min(max(pt.x, SPLIT_MIN), rt.right - SPLIT_MIN);
            if(last_split != pt.x)
            {
              rt.left = last_split-SPLIT_WIDTH/2;
              rt.right = last_split+SPLIT_WIDTH/2+1;
              hdc = GetDC(hWnd);
              OldObj = SelectObject(hdc, SizingBrush);
              PatBlt(hdc, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
              last_split = pt.x;
              rt.left = pt.x-SPLIT_WIDTH/2;
              rt.right = pt.x+SPLIT_WIDTH/2+1;
              PatBlt(hdc, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
              SelectObject(hdc, OldObj);
              ReleaseDC(hWnd, hdc);
            }
        }
        break;

    case WM_SETFOCUS:
        if (pChildWnd != NULL) {
            SetFocus(pChildWnd->nFocusPanel? pChildWnd->hListWnd: pChildWnd->hTreeWnd);
        }
        break;

    case WM_TIMER:
        break;

    case WM_NOTIFY:
        if ((int)wParam == TREE_WINDOW) {
            switch (((LPNMHDR)lParam)->code) {
            case TVN_ITEMEXPANDING:
                return !OnTreeExpanding(pChildWnd->hTreeWnd, (NMTREEVIEW*)lParam);
            case TVN_SELCHANGED: {
                    LPCTSTR keyPath, rootName;
		    LPTSTR fullPath;
                    HKEY hRootKey;

		    keyPath = GetItemPath(pChildWnd->hTreeWnd, ((NMTREEVIEW*)lParam)->itemNew.hItem, &hRootKey);
		    if (keyPath) {
		        RefreshListView(pChildWnd->hListWnd, hRootKey, keyPath);
			rootName = get_root_key_name(hRootKey);
			fullPath = HeapAlloc(GetProcessHeap(), 0, (_tcslen(rootName) + 1 + _tcslen(keyPath) + 1) * sizeof(TCHAR));
			if (fullPath) {
			    _stprintf(fullPath, _T("%s\\%s"), rootName, keyPath);
			    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)fullPath);
				SendMessage(pChildWnd->hAddressBarWnd, WM_SETTEXT, 0, (LPARAM)fullPath);
			    HeapFree(GetProcessHeap(), 0, fullPath);

			    {
			        HKEY hKey;
			        TCHAR szBuffer[MAX_PATH];			
			    	_sntprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), _T("My Computer\\%s\\%s"), rootName, keyPath);

			    	if (RegCreateKey(HKEY_CURRENT_USER,
			        	g_szGeneralRegKey,
			    	    &hKey) == ERROR_SUCCESS)
			    	{
			    	    RegSetValueEx(hKey, _T("LastKey"), 0, REG_SZ, (LPBYTE) szBuffer, (DWORD) _tcslen(szBuffer) * sizeof(szBuffer[0]));
			    	    RegCloseKey(hKey);
			    	}
			    }
			}
		    }
                }
                break;
	    case NM_SETFOCUS:
		pChildWnd->nFocusPanel = 0;
		break;
            case TVN_BEGINLABELEDIT:
            {
				LPNMTVDISPINFO ptvdi;
				/* cancel label edit for rootkeys  */
				ptvdi = (LPNMTVDISPINFO) lParam;
                if (!TreeView_GetParent(pChildWnd->hTreeWnd, ptvdi->item.hItem) ||
					!TreeView_GetParent(pChildWnd->hTreeWnd, TreeView_GetParent(pChildWnd->hTreeWnd, ptvdi->item.hItem)))
                  return TRUE;
				break;
			}
            case TVN_ENDLABELEDIT:
                {
                  LPCTSTR keyPath;
                  HKEY hRootKey;
                  HKEY hKey = NULL;
                  LPNMTVDISPINFO ptvdi;
                  LONG lResult = ERROR_SUCCESS;
                  TCHAR szBuffer[MAX_PATH];

                  ptvdi = (LPNMTVDISPINFO) lParam;
                  if (ptvdi->item.pszText)
                  {
                    keyPath = GetItemPath(pChildWnd->hTreeWnd, TreeView_GetParent(pChildWnd->hTreeWnd, ptvdi->item.hItem), &hRootKey);
                    _sntprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), _T("%s\\%s"), keyPath, ptvdi->item.pszText);
                    keyPath = GetItemPath(pChildWnd->hTreeWnd, ptvdi->item.hItem, &hRootKey);
                    if (RegOpenKeyEx(hRootKey, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                    {
                      lResult = REG_OPENED_EXISTING_KEY;
                      RegCloseKey(hKey);
                      (void)TreeView_EditLabel(pChildWnd->hTreeWnd, ptvdi->item.hItem);
                    }
                    else
                    {
                      lResult = RegRenameKey(hRootKey, keyPath, ptvdi->item.pszText);
                    }
                    return lResult;
                  }
                }
            default:
                return 0;
            }
        } else
        {
            if ((int)wParam == LIST_WINDOW)
            {
		switch (((LPNMHDR)lParam)->code) {
		  case NM_SETFOCUS:
		  	pChildWnd->nFocusPanel = 1;
		  	break;
		  default:
                	if(!ListWndNotifyProc(pChildWnd->hListWnd, wParam, lParam, &Result))
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
      POINTS pt;
      if((HWND)wParam == pChildWnd->hListWnd)
      {
        int i, cnt;
        BOOL IsDefault;
        pt = MAKEPOINTS(lParam);
        cnt = ListView_GetSelectedCount(pChildWnd->hListWnd);
        i = ListView_GetNextItem(pChildWnd->hListWnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
        FixPointIfContext(&pt, pChildWnd->hListWnd);
        if(i == -1)
        {
          TrackPopupMenu(GetSubMenu(hPopupMenus, PM_NEW), TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
        }
        else
        {
          HMENU mnu = GetSubMenu(hPopupMenus, PM_MODIFYVALUE);
          SetMenuDefaultItem(mnu, ID_EDIT_MODIFY, MF_BYCOMMAND);
          IsDefault = IsDefaultValue(pChildWnd->hListWnd, i);
          if(cnt == 1)
            EnableMenuItem(mnu, ID_EDIT_RENAME, MF_BYCOMMAND | (IsDefault ? MF_DISABLED | MF_GRAYED : MF_ENABLED));
          else
            EnableMenuItem(mnu, ID_EDIT_RENAME, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
          EnableMenuItem(mnu, ID_EDIT_MODIFY, MF_BYCOMMAND | (cnt == 1 ? MF_ENABLED : MF_DISABLED | MF_GRAYED));
          EnableMenuItem(mnu, ID_EDIT_MODIFY_BIN, MF_BYCOMMAND | (cnt == 1 ? MF_ENABLED : MF_DISABLED | MF_GRAYED));

          TrackPopupMenu(mnu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
        }
      }
      else if ((HWND)wParam == pChildWnd->hTreeWnd)
      {
        TVHITTESTINFO hti;
        HMENU hContextMenu;
        TVITEM item;
        MENUITEMINFO mii;
        TCHAR resource[256];
        TCHAR buffer[256];
        LPTSTR s;
        LPCTSTR keyPath;
        HKEY hRootKey;
        int iLastPos;
        WORD wID;

        pt = MAKEPOINTS(lParam);
        hti.pt.x = pt.x;
        hti.pt.y = pt.y;
        ScreenToClient(pChildWnd->hTreeWnd, &hti.pt);
        (void)TreeView_HitTest(pChildWnd->hTreeWnd, &hti);

        if ((hti.flags & TVHT_ONITEM) != 0 || (pt.x == -1 && pt.y == -1))
        {
          hContextMenu = GetSubMenu(hPopupMenus, PM_TREECONTEXT);
          (void)TreeView_SelectItem(pChildWnd->hTreeWnd, hti.hItem);

          memset(&item, 0, sizeof(item));
          item.mask = TVIF_STATE | TVIF_CHILDREN;
          item.hItem = hti.hItem;
          (void)TreeView_GetItem(pChildWnd->hTreeWnd, &item);

          /* Set the Expand/Collapse menu item appropriately */
          LoadString(hInst, (item.state & TVIS_EXPANDED) ? IDS_COLLAPSE : IDS_EXPAND, buffer, sizeof(buffer) / sizeof(buffer[0]));
          memset(&mii, 0, sizeof(mii));
          mii.cbSize = sizeof(mii);
          mii.fMask = MIIM_STRING | MIIM_STATE | MIIM_ID;
          mii.fState = (item.cChildren > 0) ? MFS_DEFAULT : MFS_GRAYED;
          mii.wID = (item.state & TVIS_EXPANDED) ? ID_TREE_COLLAPSEBRANCH : ID_TREE_EXPANDBRANCH;
          mii.dwTypeData = (LPTSTR) buffer;
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
          keyPath = GetItemPath(pChildWnd->hTreeWnd, NULL, &hRootKey);
          SuggestKeys(hRootKey, keyPath, Suggestions, sizeof(Suggestions) / sizeof(Suggestions[0]));
          if (Suggestions[0])
		  {
            AppendMenu(hContextMenu, MF_SEPARATOR, 0, NULL);

            LoadString(hInst, IDS_GOTO_SUGGESTED_KEY, resource, sizeof(resource) / sizeof(resource[0]));

            s = Suggestions;
            wID = ID_TREE_SUGGESTION_MIN;
            while(*s && (wID <= ID_TREE_SUGGESTION_MAX))
			{
              _sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), resource, s);

              memset(&mii, 0, sizeof(mii));
              mii.cbSize = sizeof(mii);
              mii.fMask = MIIM_STRING | MIIM_ID;
              mii.wID = wID++;
              mii.dwTypeData = buffer;
              InsertMenuItem(hContextMenu, GetMenuItemCount(hContextMenu), TRUE, &mii);

              s += _tcslen(s) + 1;
			}
		  }
          FixPointIfContext(&pt, pChildWnd->hTreeWnd);
          TrackPopupMenu(hContextMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, pChildWnd->hWnd, NULL);
        }
      }
      break;
    }

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && pChildWnd != NULL) {
            ResizeWnd(pChildWnd, LOWORD(lParam), HIWORD(lParam));
        }
        /* fall through */
default: def:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
