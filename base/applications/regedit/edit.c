/*
 * Registry editing UI functions.
 *
 * Copyright (C) 2003 Dimitrie O. Paun
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

typedef enum _EDIT_MODE
{
  EDIT_MODE_DEC,
  EDIT_MODE_HEX
} EDIT_MODE;


static const TCHAR* editValueName;
static TCHAR* stringValueData;
static PVOID binValueData;
static DWORD dwordValueData;
static DWORD valueDataLen;
static EDIT_MODE dwordEditMode = EDIT_MODE_HEX;

void error(HWND hwnd, INT resId, ...)
{
    va_list ap;
    TCHAR title[256];
    TCHAR errfmt[1024];
    TCHAR errstr[1024];
    HINSTANCE hInstance;

    hInstance = GetModuleHandle(0);

    if (!LoadString(hInstance, IDS_ERROR, title, COUNT_OF(title)))
        _tcscpy(title, _T("Error"));

    if (!LoadString(hInstance, resId, errfmt, COUNT_OF(errfmt)))
        _tcscpy(errfmt, _T("Unknown error string!"));

    va_start(ap, resId);
    _vsntprintf(errstr, COUNT_OF(errstr), errfmt, ap);
    va_end(ap);

    MessageBox(hwnd, errstr, title, MB_OK | MB_ICONERROR);
}

static void error_code_messagebox(HWND hwnd, DWORD error_code)
{
    LPTSTR lpMsgBuf;
    DWORD status;
    TCHAR title[256];
    static const TCHAR fallback[] = TEXT("Error displaying error message.\n");
    if (!LoadString(hInst, IDS_ERROR, title, COUNT_OF(title)))
        lstrcpy(title, TEXT("Error"));
    status = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL, error_code, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (!status)
        lpMsgBuf = (LPTSTR)fallback;
    MessageBox(hwnd, lpMsgBuf, title, MB_OK | MB_ICONERROR);
    if (lpMsgBuf != fallback)
        LocalFree(lpMsgBuf);
}

void warning(HWND hwnd, INT resId, ...)
{
    va_list ap;
    TCHAR title[256];
    TCHAR errfmt[1024];
    TCHAR errstr[1024];
    HINSTANCE hInstance;

    hInstance = GetModuleHandle(0);

    if (!LoadString(hInstance, IDS_WARNING, title, COUNT_OF(title)))
        _tcscpy(title, _T("Warning"));

    if (!LoadString(hInstance, resId, errfmt, COUNT_OF(errfmt)))
        _tcscpy(errfmt, _T("Unknown error string!"));

    va_start(ap, resId);
    _vsntprintf(errstr, COUNT_OF(errstr), errfmt, ap);
    va_end(ap);

    MessageBox(hwnd, errstr, title, MB_OK | MB_ICONSTOP);
}

INT_PTR CALLBACK modify_string_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR* valueData;
    HWND hwndValue;
    int len;

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg) {
    case WM_INITDIALOG:
        if(editValueName && _tcscmp(editValueName, _T("")))
        {
          SetDlgItemText(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
          TCHAR buffer[255];
	  LoadString(hInst, IDS_DEFAULT_VALUE_NAME, buffer, sizeof(buffer)/sizeof(TCHAR));
	  SetDlgItemText(hwndDlg, IDC_VALUE_NAME, buffer);
        }
        SetDlgItemText(hwndDlg, IDC_VALUE_DATA, stringValueData);
        SetFocus(GetDlgItem(hwndDlg, IDC_VALUE_DATA));
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
            {
                if ((len = GetWindowTextLength(hwndValue)))
                {
                    if (stringValueData)
                    {
                        if ((valueData = HeapReAlloc(GetProcessHeap(), 0, stringValueData, (len + 1) * sizeof(TCHAR))))
                        {
                            stringValueData = valueData;
                            if (!GetWindowText(hwndValue, stringValueData, len + 1))
                                *stringValueData = 0;
                        }
                    }
                    else
                    {
                        if ((valueData = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(TCHAR))))
                        {
                            stringValueData = valueData;
                            if (!GetWindowText(hwndValue, stringValueData, len + 1))
                                *stringValueData = 0;
                        }
                    }
                }
                else
                {
                  if (stringValueData)
                    *stringValueData = 0;
                }
            }
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}


INT_PTR CALLBACK modify_multi_string_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR* valueData;
    HWND hwndValue;
    int len;

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg) {
    case WM_INITDIALOG:
        if(editValueName && _tcscmp(editValueName, _T("")))
        {
          SetDlgItemText(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
          TCHAR buffer[255];
	  LoadString(hInst, IDS_DEFAULT_VALUE_NAME, buffer, sizeof(buffer)/sizeof(TCHAR));
	  SetDlgItemText(hwndDlg, IDC_VALUE_NAME, buffer);
        }
        SetDlgItemText(hwndDlg, IDC_VALUE_DATA, stringValueData);
        SetFocus(GetDlgItem(hwndDlg, IDC_VALUE_DATA));
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
            {
                if ((len = GetWindowTextLength(hwndValue)))
                {
                    if (stringValueData)
                    {
                        if ((valueData = HeapReAlloc(GetProcessHeap(), 0, stringValueData, (len + 1) * sizeof(TCHAR))))
                        {
                            stringValueData = valueData;
                            if (!GetWindowText(hwndValue, stringValueData, len + 1))
                                *stringValueData = 0;
                        }
                    }
                    else
                    {
                        if ((valueData = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(TCHAR))))
                        {
                            stringValueData = valueData;
                            if (!GetWindowText(hwndValue, stringValueData, len + 1))
                                *stringValueData = 0;
                        }
                    }
                }
                else
                {
                  if (stringValueData)
                    *stringValueData = 0;
                }
            }
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}


LRESULT CALLBACK DwordEditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldwndproc;

    oldwndproc = (WNDPROC)(LONG_PTR)GetWindowLongPtr(hwnd, GWL_USERDATA);

    switch (uMsg)
    {
    case WM_CHAR:
        if (dwordEditMode == EDIT_MODE_DEC)
        {
            if (isdigit((int) wParam & 0xff) || iscntrl((int) wParam & 0xff))
            {
                break;
            }
            else
            {
                return 0;
            }
        }
        else if (dwordEditMode == EDIT_MODE_HEX)
        {
            if (isxdigit((int) wParam & 0xff) || iscntrl((int) wParam & 0xff))
            {
                break;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            break;
        }
    }

    return CallWindowProc(oldwndproc, hwnd, uMsg, wParam, lParam);
}


INT_PTR CALLBACK modify_dword_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc;
    HWND hwndValue;
    int len;
    TCHAR ValueString[32];
    LPTSTR Remainder;
    DWORD Base;
    DWORD Value = 0;

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg) {
    case WM_INITDIALOG:
        dwordEditMode = EDIT_MODE_HEX;

        /* subclass the edit control */
        hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA);
        oldproc = (WNDPROC)(LONG_PTR)GetWindowLongPtr(hwndValue, GWL_WNDPROC);
        SetWindowLongPtr(hwndValue, GWL_USERDATA, (DWORD_PTR)oldproc);
        SetWindowLongPtr(hwndValue, GWL_WNDPROC, (DWORD_PTR)DwordEditSubclassProc);

        if(editValueName && _tcscmp(editValueName, _T("")))
        {
            SetDlgItemText(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
	    TCHAR buffer[255];
	    LoadString(hInst, IDS_DEFAULT_VALUE_NAME, buffer, sizeof(buffer)/sizeof(TCHAR));
	    SetDlgItemText(hwndDlg, IDC_VALUE_NAME, buffer);
        }
        CheckRadioButton (hwndDlg, IDC_FORMAT_HEX, IDC_FORMAT_DEC, IDC_FORMAT_HEX);
        _stprintf (ValueString, _T("%lx"), dwordValueData);
        SetDlgItemText(hwndDlg, IDC_VALUE_DATA, ValueString);
        SetFocus(GetDlgItem(hwndDlg, IDC_VALUE_DATA));
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_FORMAT_HEX:
            if (HIWORD(wParam) == BN_CLICKED && dwordEditMode == EDIT_MODE_DEC)
            {
                dwordEditMode = EDIT_MODE_HEX;
                if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
                {
                    if ((len = GetWindowTextLength(hwndValue)))
                    {
                        if (GetWindowText(hwndValue, ValueString, 32))
                        {
                            Value = _tcstoul (ValueString, &Remainder, 10);
                        }
                    }
                }
                _stprintf (ValueString, _T("%lx"), Value);
                SetDlgItemText(hwndDlg, IDC_VALUE_DATA, ValueString);
                return TRUE;
            }
            break;

        case IDC_FORMAT_DEC:
            if (HIWORD(wParam) == BN_CLICKED && dwordEditMode == EDIT_MODE_HEX)
            {
                dwordEditMode = EDIT_MODE_DEC;
                if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
                {
                    if ((len = GetWindowTextLength(hwndValue)))
                    {
                        if (GetWindowText(hwndValue, ValueString, 32))
                        {
                            Value = _tcstoul (ValueString, &Remainder, 16);
                        }
                    }
                }
                _stprintf (ValueString, _T("%lu"), Value);
                SetDlgItemText(hwndDlg, IDC_VALUE_DATA, ValueString);
                return TRUE;
            }
            break;

        case IDOK:
            if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
            {
                if ((len = GetWindowTextLength(hwndValue)))
                {
                    if (!GetWindowText(hwndValue, ValueString, 32))
                    {
                        EndDialog(hwndDlg, IDCANCEL);
                        return TRUE;
                    }

                    Base = (dwordEditMode == EDIT_MODE_HEX) ? 16 : 10;
                    dwordValueData = _tcstoul (ValueString, &Remainder, Base);
                }
                else
                {
                  EndDialog(hwndDlg, IDCANCEL);
                  return TRUE;
                }
            }
            EndDialog(hwndDlg, IDOK);
            return TRUE;

        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}


INT_PTR CALLBACK modify_binary_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndValue;
    UINT len;

    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg) {
    case WM_INITDIALOG:
        if(editValueName && _tcscmp(editValueName, _T("")))
        {
          SetDlgItemText(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
          TCHAR buffer[255];
	  LoadString(hInst, IDS_DEFAULT_VALUE_NAME, buffer, sizeof(buffer)/sizeof(TCHAR));
	  SetDlgItemText(hwndDlg, IDC_VALUE_NAME, buffer);
        }
        hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA);
        HexEdit_LoadBuffer(hwndValue, binValueData, valueDataLen);
        /* reset the hex edit control's font */
        SendMessage(hwndValue, WM_SETFONT, 0, 0);
        SetFocus(hwndValue);
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA)))
            {
                len = (UINT) HexEdit_GetBufferSize(hwndValue);
                if (len != valueDataLen && len > 0)
                {
                    binValueData = HeapReAlloc(GetProcessHeap(), 0, binValueData, len);
                }
                if (len > 0)
                {
                  HexEdit_CopyBuffer(hwndValue, binValueData, len);
                }
                valueDataLen = len;
            }
            EndDialog(hwndDlg, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}


BOOL ModifyValue(HWND hwnd, HKEY hKey, LPCTSTR valueName, BOOL EditBin)
{
    DWORD type;
    LONG lRet;
    BOOL result = FALSE;

    if (!hKey)
        return FALSE;

    editValueName = valueName;

    lRet = RegQueryValueEx(hKey, valueName, 0, &type, 0, &valueDataLen);
    if (lRet != ERROR_SUCCESS && (!_tcscmp(valueName, _T("")) || valueName == NULL))
    {
      lRet = ERROR_SUCCESS; /* Allow editing of (Default) values which don't exist */
      type = REG_SZ;
      valueDataLen = 0;
      stringValueData = NULL;
      binValueData = NULL;
    }

    if (lRet != ERROR_SUCCESS)
    {
        error(hwnd, IDS_BAD_VALUE, valueName);
        goto done;
    }

    if (EditBin == FALSE && ((type == REG_SZ) || (type == REG_EXPAND_SZ)))
    {
        if (valueDataLen > 0)
        {
            if (!(stringValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen)))
            {
                error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
                goto done;
            }
            lRet = RegQueryValueEx(hKey, valueName, 0, 0, (LPBYTE)stringValueData, &valueDataLen);
            if (lRet != ERROR_SUCCESS)
            {
                error(hwnd, IDS_BAD_VALUE, valueName);
                goto done;
            }
        }
        else
        {
            stringValueData = NULL;
        }

        if (DialogBox(0, MAKEINTRESOURCE(IDD_EDIT_STRING), hwnd, modify_string_dlgproc) == IDOK)
        {
            if (stringValueData)
            {
                lRet = RegSetValueEx(hKey, valueName, 0, type, (LPBYTE)stringValueData, (DWORD) (_tcslen(stringValueData) + 1) * sizeof(TCHAR));
            }
            else
            {
                lRet = RegSetValueEx(hKey, valueName, 0, type, NULL, 0);
            }
            if (lRet == ERROR_SUCCESS)
                result = TRUE;
        }
    }
    else if (EditBin == FALSE && type == REG_MULTI_SZ)
    {
        if (valueDataLen > 0)
        {
            size_t llen, listlen, nl_len;
            LPTSTR src, lines = NULL;

	    if (!(stringValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen)))
            {
                error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
                goto done;
            }
            lRet = RegQueryValueEx(hKey, valueName, 0, 0, (LPBYTE)stringValueData, &valueDataLen);
            if (lRet != ERROR_SUCCESS)
            {
                error(hwnd, IDS_BAD_VALUE, valueName);
                goto done;
            }

	    /* convert \0 to \r\n */
            src = stringValueData;
            nl_len = _tcslen(_T("\r\n")) * sizeof(TCHAR);
            listlen = sizeof(TCHAR);
            lines = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, listlen + sizeof(TCHAR));
            while(*src != _T('\0'))
            {
                llen = _tcslen(src);
                if(llen == 0)
                  break;
                listlen += (llen * sizeof(TCHAR)) + nl_len;
		lines = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lines, listlen);
		_tcscat(lines, src);
		_tcscat(lines, _T("\r\n"));
		src += llen + 1;
            }
            HeapFree(GetProcessHeap(), 0, stringValueData);
            stringValueData = lines;
        }
        else
        {
            stringValueData = NULL;
        }

        if (DialogBox(0, MAKEINTRESOURCE(IDD_EDIT_MULTI_STRING), hwnd, modify_multi_string_dlgproc) == IDOK)
        {
            if (stringValueData)
            {
                /* convert \r\n to \0 */
                BOOL EmptyLines = FALSE;
                LPTSTR src, lines, nl;
                size_t linechars, buflen, c_nl, dest;

                src = stringValueData;
                buflen = sizeof(TCHAR);
                lines = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buflen + sizeof(TCHAR));
                c_nl = _tcslen(_T("\r\n"));
                dest = 0;
                while(*src != _T('\0'))
                {
                    if((nl = _tcsstr(src, _T("\r\n"))))
                    {
                        linechars = nl - src;
                        if(nl == src)
                        {
                            EmptyLines = TRUE;
			    src = nl + c_nl;
                            continue;
                        }
                    }
                    else
                    {
                        linechars = _tcslen(src);
                    }
                    if(linechars > 0)
                    {
			buflen += ((linechars + 1) * sizeof(TCHAR));
                        lines = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lines, buflen);
                        memcpy((lines + dest), src, linechars * sizeof(TCHAR));
                        dest += linechars;
                        lines[dest++] = _T('\0');
                    }
                    else
                    {
                        EmptyLines = TRUE;
                    }
                    src += linechars + (nl != NULL ? c_nl : 0);
                }
                lines[++dest] = _T('\0');

                if(EmptyLines)
                {
                    warning(hwnd, IDS_MULTI_SZ_EMPTY_STRING);
                }

		lRet = RegSetValueEx(hKey, valueName, 0, type, (LPBYTE)lines, (DWORD) buflen);
		HeapFree(GetProcessHeap(), 0, lines);
            }
            else
            {
                lRet = RegSetValueEx(hKey, valueName, 0, type, NULL, 0);
            }
            if (lRet == ERROR_SUCCESS)
                result = TRUE;
        }
    }
    else if (EditBin == FALSE && type == REG_DWORD)
    {
        lRet = RegQueryValueEx(hKey, valueName, 0, 0, (LPBYTE)&dwordValueData, &valueDataLen);
        if (lRet != ERROR_SUCCESS)
        {
            error(hwnd, IDS_BAD_VALUE, valueName);
            goto done;
        }

        if (DialogBox(0, MAKEINTRESOURCE(IDD_EDIT_DWORD), hwnd, modify_dword_dlgproc) == IDOK)
        {
            lRet = RegSetValueEx(hKey, valueName, 0, type, (LPBYTE)&dwordValueData, sizeof(DWORD));
            if (lRet == ERROR_SUCCESS)
                result = TRUE;
        }
    }
    else if (EditBin == TRUE || type == REG_NONE || type == REG_BINARY)
    {
        #ifndef UNICODE
        LPWSTR u_valuename;
        int len_vname = lstrlen(valueName);

	if(len_vname > 0)
        {
          if(!(u_valuename = HeapAlloc(GetProcessHeap(), 0, (len_vname + 1) * sizeof(WCHAR))))
          {
            error(hwnd, IDS_TOO_BIG_VALUE, len_vname);
            goto done;
          }
	  /* convert the ansi value name to an unicode string */
          MultiByteToWideChar(CP_ACP, 0, valueName, -1, u_valuename, len_vname + 1);
          valueDataLen *= sizeof(WCHAR);
        }
        else
          u_valuename = L"";
        #endif
	if(valueDataLen > 0)
        {
	    if(!(binValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen)))
            {
              error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
              goto done;
            }

	    /* force to use the unicode version, so editing strings in binary mode is correct */
	    lRet = RegQueryValueExW(hKey,
	                            #ifndef UNICODE
	                            u_valuename,
	                            #else
	                            valueName,
	                            #endif
				    0, 0, (LPBYTE)binValueData, &valueDataLen);
            if (lRet != ERROR_SUCCESS)
            {
                HeapFree(GetProcessHeap(), 0, binValueData);
                #ifndef UNICODE
                if(len_vname > 0)
                  HeapFree(GetProcessHeap(), 0, u_valuename);
                #endif
	        error(hwnd, IDS_BAD_VALUE, valueName);
                goto done;
            }
        }
        else
        {
            binValueData = NULL;
        }

        if (DialogBox(0, MAKEINTRESOURCE(IDD_EDIT_BIN_DATA), hwnd, modify_binary_dlgproc) == IDOK)
        {
	    /* force to use the unicode version, so editing strings in binary mode is correct */
	    lRet = RegSetValueExW(hKey,
	                          #ifndef UNICODE
	                          u_valuename,
	                          #else
	                          valueName,
	                          #endif
				  0, type, (LPBYTE)binValueData, valueDataLen);
            if (lRet == ERROR_SUCCESS)
                result = TRUE;
        }
        if(binValueData != NULL)
	  HeapFree(GetProcessHeap(), 0, binValueData);
        #ifndef UNICODE
        if(len_vname > 0)
          HeapFree(GetProcessHeap(), 0, u_valuename);
        #endif
    }
    else
    {
        error(hwnd, IDS_UNSUPPORTED_TYPE, type);
    }

done:
    if (stringValueData)
        HeapFree(GetProcessHeap(), 0, stringValueData);
    stringValueData = NULL;

    return result;
}

BOOL DeleteKey(HWND hwnd, HKEY hKeyRoot, LPCTSTR keyPath)
{
    TCHAR msg[128], caption[128];
    BOOL result = FALSE;
    LONG lRet;
    HKEY hKey;

    lRet = RegOpenKeyEx(hKeyRoot, keyPath, 0, KEY_READ|KEY_SET_VALUE, &hKey);
    if (lRet != ERROR_SUCCESS) {
	error_code_messagebox(hwnd, lRet);
	return FALSE;
    }

    LoadString(hInst, IDS_QUERY_DELETE_KEY_CONFIRM, caption, sizeof(caption)/sizeof(TCHAR));
    LoadString(hInst, IDS_QUERY_DELETE_KEY_ONE, msg, sizeof(msg)/sizeof(TCHAR));

    if (MessageBox(g_pChildWnd->hWnd, msg, caption, MB_ICONQUESTION | MB_YESNO) != IDYES)
        goto done;

    lRet = SHDeleteKey(hKeyRoot, keyPath);
    if (lRet != ERROR_SUCCESS) {
	error(hwnd, IDS_BAD_KEY, keyPath);
	goto done;
    }
    result = TRUE;

done:
    RegCloseKey(hKey);
    return result;
}
