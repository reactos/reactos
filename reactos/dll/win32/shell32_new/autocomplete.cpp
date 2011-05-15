/*
 *	AutoComplete interfaces implementation.
 *
 *	Copyright 2004	Maxime Bellengé <maxime.bellenge@laposte.net>
 *	Copyright 2009  Andrew Hill
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
  Implemented:
  - ACO_AUTOAPPEND style
  - ACO_AUTOSUGGEST style
  - ACO_UPDOWNKEYDROPSLIST style

  - Handle pwzsRegKeyPath and pwszQuickComplete in Init

  TODO:
  - implement ACO_SEARCH style
  - implement ACO_FILTERPREFIXES style
  - implement ACO_USETAB style
  - implement ACO_RTLREADING style

 */

#include <precomp.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include "autocomplete.h"


WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
 *  IAutoComplete_Constructor
 */
CAutoComplete::CAutoComplete()
{
	enabled = TRUE;
	options = ACO_AUTOAPPEND;
	wpOrigEditProc = NULL;
	hwndListBox = NULL;
	txtbackup = NULL;
	quickComplete = NULL;
}

/**************************************************************************
 *  IAutoComplete_Destructor
 */
CAutoComplete::~CAutoComplete()
{
	TRACE(" destroying IAutoComplete(%p)\n", this);
	HeapFree(GetProcessHeap(), 0, quickComplete);
	HeapFree(GetProcessHeap(), 0, txtbackup);
	if (hwndListBox)
		DestroyWindow(hwndListBox);
}

/******************************************************************************
 * IAutoComplete_fnEnable
 */
HRESULT WINAPI CAutoComplete::Enable(BOOL fEnable)
{
    HRESULT hr = S_OK;

    TRACE("(%p)->(%s)\n", this, (fEnable) ? "true" : "false");

    enabled = fEnable;

    return hr;
}

/******************************************************************************
 * IAutoComplete_fnInit
 */
HRESULT WINAPI CAutoComplete::Init(HWND hwndEdit, IUnknown *punkACL, LPCOLESTR pwzsRegKeyPath, LPCOLESTR pwszQuickComplete)
{
    static const WCHAR lbName[] = {'L','i','s','t','B','o','x',0};

    TRACE("(%p)->(0x%08lx, %p, %s, %s)\n",
	  this, hwndEdit, punkACL, debugstr_w(pwzsRegKeyPath), debugstr_w(pwszQuickComplete));

    if (options & ACO_AUTOSUGGEST) TRACE(" ACO_AUTOSUGGEST\n");
    if (options & ACO_AUTOAPPEND) TRACE(" ACO_AUTOAPPEND\n");
    if (options & ACO_SEARCH) FIXME(" ACO_SEARCH not supported\n");
    if (options & ACO_FILTERPREFIXES) FIXME(" ACO_FILTERPREFIXES not supported\n");
    if (options & ACO_USETAB) FIXME(" ACO_USETAB not supported\n");
    if (options & ACO_UPDOWNKEYDROPSLIST) TRACE(" ACO_UPDOWNKEYDROPSLIST\n");
    if (options & ACO_RTLREADING) FIXME(" ACO_RTLREADING not supported\n");

	hwndEdit = hwndEdit;

	if (!SUCCEEDED (punkACL->QueryInterface(IID_IEnumString, (LPVOID *)&enumstr))) {
	TRACE("No IEnumString interface\n");
	return  E_NOINTERFACE;
    }

    wpOrigEditProc = (WNDPROC)SetWindowLongPtrW(hwndEdit, GWLP_WNDPROC, (LONG_PTR) ACEditSubclassProc);
    SetWindowLongPtrW(hwndEdit, GWLP_USERDATA, (LONG_PTR)this);

    if (options & ACO_AUTOSUGGEST) {
	HWND hwndParent;

	hwndParent = GetParent(hwndEdit);

	/* FIXME : The listbox should be resizable with the mouse. WS_THICKFRAME looks ugly */
	hwndListBox = CreateWindowExW(0, lbName, NULL,
					    WS_BORDER | WS_CHILD | WS_VSCROLL | LBS_HASSTRINGS | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
					    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					    hwndParent, NULL,
					    (HINSTANCE)GetWindowLongPtrW(hwndParent, GWLP_HINSTANCE), NULL);

 if (hwndListBox) {
	    wpOrigLBoxProc = (WNDPROC)SetWindowLongPtrW(hwndListBox, GWLP_WNDPROC, (LONG_PTR)ACLBoxSubclassProc);
	    SetWindowLongPtrW(hwndListBox, GWLP_USERDATA, (LONG_PTR)this);
	}
	}

    if (pwzsRegKeyPath) {
	WCHAR *key;
	WCHAR result[MAX_PATH];
	WCHAR *value;
	HKEY hKey = 0;
	LONG res;
	LONG len;

	/* pwszRegKeyPath contains the key as well as the value, so we split */
	key = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (wcslen(pwzsRegKeyPath) + 1) * sizeof(WCHAR));
	wcscpy(key, pwzsRegKeyPath);
	value = const_cast<WCHAR *>(strrchrW(key, '\\'));
	*value = 0;
	value++;
	/* Now value contains the value and buffer the key */
	res = RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey);
	if (res != ERROR_SUCCESS) {
	    /* if the key is not found, MSDN states we must seek in HKEY_LOCAL_MACHINE */
	    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey);
	}
	if (res == ERROR_SUCCESS) {
	    res = RegQueryValueW(hKey, value, result, &len);
	    if (res == ERROR_SUCCESS) {
		quickComplete = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
		wcscpy(quickComplete, result);
	    }
	    RegCloseKey(hKey);
	}
	HeapFree(GetProcessHeap(), 0, key);
    }

    if ((pwszQuickComplete) && (!quickComplete)) {
	quickComplete = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (wcslen(pwszQuickComplete) + 1) * sizeof(WCHAR));
	wcscpy(quickComplete, pwszQuickComplete);
    }

    return S_OK;
}

/**************************************************************************
 *  IAutoComplete_fnGetOptions
 */
HRESULT WINAPI CAutoComplete::GetOptions(DWORD *pdwFlag)
{
    HRESULT hr = S_OK;

    TRACE("(%p) -> (%p)\n", this, pdwFlag);

    *pdwFlag = options;

    return hr;
}

/**************************************************************************
 *  IAutoComplete_fnSetOptions
 */
HRESULT WINAPI CAutoComplete::SetOptions(DWORD dwFlag)
{
    HRESULT hr = S_OK;

    TRACE("(%p) -> (0x%x)\n", this, dwFlag);

    options = (AUTOCOMPLETEOPTIONS)dwFlag;

    return hr;
}

/*
  Window procedure for autocompletion
 */
LRESULT APIENTRY CAutoComplete::ACEditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAutoComplete			*pThis = (CAutoComplete *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    LPOLESTR strs;
    HRESULT hr;
    WCHAR hwndText[255];
    WCHAR *hwndQCText;
    RECT r;
    BOOL control, filled, displayall = FALSE;
    int cpt, height, sel;

    if (!pThis->enabled) return CallWindowProcW(pThis->wpOrigEditProc, hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
	case CB_SHOWDROPDOWN:
	    ShowWindow(pThis->hwndListBox, SW_HIDE);
	    break;
	case WM_KILLFOCUS:
	    if ((pThis->options & ACO_AUTOSUGGEST) &&
		((HWND)wParam != pThis->hwndListBox))
	    {
		ShowWindow(pThis->hwndListBox, SW_HIDE);
	    }
	    break;
	case WM_KEYUP:

	    GetWindowTextW(hwnd, (LPWSTR)hwndText, 255);

	    switch(wParam) {
		case VK_RETURN:
		    /* If quickComplete is set and control is pressed, replace the string */
		    control = GetKeyState(VK_CONTROL) & 0x8000;
		    if (control && pThis->quickComplete) {
			hwndQCText = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
						       (wcslen(pThis->quickComplete)+wcslen(hwndText))*sizeof(WCHAR));
			sel = swprintf(hwndQCText, pThis->quickComplete, hwndText);
			SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)hwndQCText);
			SendMessageW(hwnd, EM_SETSEL, 0, sel);
			HeapFree(GetProcessHeap(), 0, hwndQCText);
		    }

		    ShowWindow(pThis->hwndListBox, SW_HIDE);
		    return 0;
		case VK_LEFT:
		case VK_RIGHT:
		    return 0;
		case VK_UP:
		case VK_DOWN:
		    /* Two cases here :
		       - if the listbox is not visible, displays it
		       with all the entries if the style ACO_UPDOWNKEYDROPSLIST
		       is present but does not select anything.
		       - if the listbox is visible, change the selection
		    */
		    if ( (pThis->options & (ACO_AUTOSUGGEST | ACO_UPDOWNKEYDROPSLIST))
			 && (!IsWindowVisible(pThis->hwndListBox) && (! *hwndText)) )
		    {
			 /* We must display all the entries */
			 displayall = TRUE;
		    } else {
			if (IsWindowVisible(pThis->hwndListBox)) {
			    int count;

			    count = SendMessageW(pThis->hwndListBox, LB_GETCOUNT, 0, 0);
			    /* Change the selection */
			    sel = SendMessageW(pThis->hwndListBox, LB_GETCURSEL, 0, 0);
			    if (wParam == VK_UP)
			    sel = ((sel-1)<0)?count-1:sel-1;
			    else
				sel = ((sel+1)>= count)?-1:sel+1;
			    SendMessageW(pThis->hwndListBox, LB_SETCURSEL, sel, 0);
			    if (sel != -1) {
				WCHAR *msg;
				int len;

				len = SendMessageW(pThis->hwndListBox, LB_GETTEXTLEN, sel, (LPARAM)NULL);
				msg = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (len + 1) * sizeof(WCHAR));
				SendMessageW(pThis->hwndListBox, LB_GETTEXT, sel, (LPARAM)msg);
				SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)msg);
				SendMessageW(hwnd, EM_SETSEL, wcslen(msg), wcslen(msg));
				HeapFree(GetProcessHeap(), 0, msg);
			    } else {
				SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)pThis->txtbackup);
				SendMessageW(hwnd, EM_SETSEL, wcslen(pThis->txtbackup), wcslen(pThis->txtbackup));
			    }
			}
			return 0;
		    }
		    break;
		case VK_BACK:
		case VK_DELETE:
		    if ((! *hwndText) && (pThis->options & ACO_AUTOSUGGEST)) {
			ShowWindow(pThis->hwndListBox, SW_HIDE);
			return CallWindowProcW(pThis->wpOrigEditProc, hwnd, uMsg, wParam, lParam);
		    }
		    if (pThis->options & ACO_AUTOAPPEND) {
			DWORD b;
			SendMessageW(hwnd, EM_GETSEL, (WPARAM)&b, (LPARAM)NULL);
			if (b>1) {
			    hwndText[b-1] = '\0';
			} else {
			    hwndText[0] = '\0';
			    SetWindowTextW(hwnd, hwndText);
			}
		    }
		    break;
		default:
		    ;
	    }

	    SendMessageW(pThis->hwndListBox, LB_RESETCONTENT, 0, 0);

	    HeapFree(GetProcessHeap(), 0, pThis->txtbackup);
	    pThis->txtbackup = (WCHAR *)HeapAlloc(GetProcessHeap(),
						 HEAP_ZERO_MEMORY, (wcslen(hwndText)+1)*sizeof(WCHAR));
	    wcscpy(pThis->txtbackup, hwndText);

	    /* Returns if there is no text to search and we doesn't want to display all the entries */
	    if ((!displayall) && (! *hwndText) )
		break;

	    pThis->enumstr->Reset();
	    filled = FALSE;
	    for(cpt = 0;;) {
		hr = pThis->enumstr->Next(1, &strs, NULL);
		if (hr != S_OK)
		    break;

		if ((LPWSTR)strstrW(strs, hwndText) == strs) {

		    if (pThis->options & ACO_AUTOAPPEND) {
			SetWindowTextW(hwnd, strs);
			SendMessageW(hwnd, EM_SETSEL, wcslen(hwndText), wcslen(strs));
			break;
		    }

		    if (pThis->options & ACO_AUTOSUGGEST) {
			SendMessageW(pThis->hwndListBox, LB_ADDSTRING, 0, (LPARAM)strs);
			filled = TRUE;
			cpt++;
		    }
		}
	    }

	    if (pThis->options & ACO_AUTOSUGGEST) {
		if (filled) {
		    height = SendMessageW(pThis->hwndListBox, LB_GETITEMHEIGHT, 0, 0);
		    SendMessageW(pThis->hwndListBox, LB_CARETOFF, 0, 0);
		    GetWindowRect(hwnd, &r);
		    SetParent(pThis->hwndListBox, HWND_DESKTOP);
		    /* It seems that Windows XP displays 7 lines at most
		       and otherwise displays a vertical scroll bar */
		    SetWindowPos(pThis->hwndListBox, HWND_TOP,
				 r.left, r.bottom + 1, r.right - r.left, min(height * 7, height * (cpt + 1)),
				 SWP_SHOWWINDOW );
		} else {
		    ShowWindow(pThis->hwndListBox, SW_HIDE);
		}
	    }

	    break;
	default:
	    return CallWindowProcW(pThis->wpOrigEditProc, hwnd, uMsg, wParam, lParam);

	}

	return 0;
}

LRESULT APIENTRY CAutoComplete::ACLBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAutoComplete *pThis = (CAutoComplete *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    WCHAR *msg;
    int sel, len;

    switch (uMsg) {
	case WM_MOUSEMOVE:
	    sel = SendMessageW(hwnd, LB_ITEMFROMPOINT, 0, lParam);
	    SendMessageW(hwnd, LB_SETCURSEL, (WPARAM)sel, (LPARAM)0);
	    break;
	case WM_LBUTTONDOWN:
	    sel = SendMessageW(hwnd, LB_GETCURSEL, 0, 0);
	    if (sel < 0)
		break;
	    len = SendMessageW(pThis->hwndListBox, LB_GETTEXTLEN, sel, 0);
	    msg = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (len + 1) * sizeof(WCHAR));
	    SendMessageW(hwnd, LB_GETTEXT, sel, (LPARAM)msg);
	    SendMessageW(pThis->hwndEdit, WM_SETTEXT, 0, (LPARAM)msg);
	    SendMessageW(pThis->hwndEdit, EM_SETSEL, 0, wcslen(msg));
	    ShowWindow(hwnd, SW_HIDE);
	    HeapFree(GetProcessHeap(), 0, msg);
	    break;
	default:
	    return CallWindowProcW(pThis->wpOrigLBoxProc, hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
