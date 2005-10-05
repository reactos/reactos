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

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>

#include "main.h"
#include "regproc.h"

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
    RECT rt = {0, 0, cx, cy};

    cx = pChildWnd->nSplitPos + SPLIT_WIDTH/2;
    DeferWindowPos(hdwp, pChildWnd->hTreeWnd, 0, rt.left, rt.top, pChildWnd->nSplitPos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
    DeferWindowPos(hdwp, pChildWnd->hListWnd, 0, rt.left+cx  , rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
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
    TCHAR szConfirmTitle[256];
    TCHAR szConfirmText[256];
    WORD wID = LOWORD(wParam);

    switch (wID) {
        /* Parse the menu selections: */
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        /* TODO */
        break;
    case ID_TREE_EXPANDBRANCH:
        TreeView_Expand(pChildWnd->hTreeWnd, TreeView_GetSelection(pChildWnd->hTreeWnd), TVE_EXPAND);
        break;
    case ID_TREE_COLLAPSEBRANCH:
        TreeView_Expand(pChildWnd->hTreeWnd, TreeView_GetSelection(pChildWnd->hTreeWnd), TVE_COLLAPSE);
        break;
    case ID_TREE_RENAME:
        SetFocus(pChildWnd->hTreeWnd);
        TreeView_EditLabel(pChildWnd->hTreeWnd, TreeView_GetSelection(pChildWnd->hTreeWnd));
        break;
    case ID_TREE_DELETE:
        hSelection = TreeView_GetSelection(pChildWnd->hTreeWnd);
        keyPath = GetItemPath(pChildWnd->hTreeWnd, hSelection, &hRootKey);

        LoadString(hInst, IDS_QUERY_DELETE_KEY_CONFIRM, szConfirmTitle, sizeof(szConfirmTitle) / sizeof(szConfirmTitle[0]));
        LoadString(hInst, IDS_QUERY_DELETE_KEY_ONE, szConfirmText, sizeof(szConfirmText) / sizeof(szConfirmText[0]));

        if (MessageBox(pChildWnd->hWnd, szConfirmText, szConfirmTitle, MB_YESNO) == IDYES)
        {
            if (RegDeleteKeyRecursive(hRootKey, keyPath) == ERROR_SUCCESS)
                TreeView_DeleteItem(pChildWnd->hTreeWnd, hSelection);
        }
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
				if (szBuffer[0] != '\0')
				{
					if (RegOpenKey(hRootKey, szBuffer, &hOtherKey) == ERROR_SUCCESS)
					{
						lstrcpyn(pszSuggestions, TEXT("HKCR\\"), iSuggestionsLength);
						i = _tcslen(pszSuggestions);
						pszSuggestions += i;
	    				iSuggestionsLength -= i;

						lstrcpyn(pszSuggestions, szBuffer, iSuggestionsLength);
						i = _tcslen(pszSuggestions) + 1;
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
		while(bFound);

		/* Check CLSID key */
		if (RegOpenKey(hRootKey, pszKeyPath, &hSubKey) == ERROR_SUCCESS)
		{
			if (RegQueryStringValue(hSubKey, TEXT("CLSID"), NULL,
				szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])) == ERROR_SUCCESS)
			{
				lstrcpyn(pszSuggestions, TEXT("HKCR\\CLSID\\"), iSuggestionsLength);
				i = _tcslen(pszSuggestions);
				pszSuggestions += i;
				iSuggestionsLength -= i;

				lstrcpyn(pszSuggestions, szBuffer, iSuggestionsLength);
				i = _tcslen(pszSuggestions) + 1;
				pszSuggestions += i;
				iSuggestionsLength -= i;
			}
			RegCloseKey(hSubKey);
		}
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
    static short last_split;
    BOOL Result;
    ChildWnd* pChildWnd = g_pChildWnd;

    switch (message) {
    case WM_CREATE:
    {
        TCHAR buffer[MAX_PATH];
        /* load "My Computer" string */
        LoadString(hInst, IDS_MY_COMPUTER, buffer, sizeof(buffer)/sizeof(TCHAR));

	g_pChildWnd = pChildWnd = HeapAlloc(GetProcessHeap(), 0, sizeof(ChildWnd));
        if (!pChildWnd) return 0;
        _tcsncpy(pChildWnd->szPath, buffer, MAX_PATH);
        pChildWnd->nSplitPos = 250;
        pChildWnd->hWnd = hWnd;
        pChildWnd->hTreeWnd = CreateTreeView(hWnd, pChildWnd->szPath, TREE_WINDOW);
        pChildWnd->hListWnd = CreateListView(hWnd, LIST_WINDOW/*, pChildWnd->szPath*/);
        SetFocus(pChildWnd->hTreeWnd);
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
			    HeapFree(GetProcessHeap(), 0, fullPath);
			}
		    }
                }
                break;
	    case NM_SETFOCUS:
		pChildWnd->nFocusPanel = 0;
		break;
            case TVN_ENDLABELEDIT:
                {
                  LPCTSTR keyPath;
                  HKEY hRootKey;
                  LPNMTVDISPINFO ptvdi;
                  LONG lResult;

                  ptvdi = (LPNMTVDISPINFO) lParam;
                  if (ptvdi->item.pszText)
                  {
                    keyPath = GetItemPath(pChildWnd->hTreeWnd, ptvdi->item.hItem, &hRootKey);
                    lResult = RegRenameKey(hRootKey, keyPath, ptvdi->item.pszText);
                    return lResult == ERROR_SUCCESS;
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
        TreeView_HitTest(pChildWnd->hTreeWnd, &hti);

        if ((hti.flags & TVHT_ONITEM) != 0)
        {
          hContextMenu = GetSubMenu(hPopupMenus, PM_TREECONTEXT);
          TreeView_SelectItem(pChildWnd->hTreeWnd, hti.hItem);

          memset(&item, 0, sizeof(item));
          item.mask = TVIF_STATE | TVIF_CHILDREN;
          item.hItem = hti.hItem;
          TreeView_GetItem(pChildWnd->hTreeWnd, &item);

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
