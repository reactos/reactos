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

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cderr.h>
#include <stdlib.h>
#include <stdio.h>
#include <shellapi.h>
#include <ctype.h>

#include "main.h"
#include "regproc.h"
#include "resource.h"


typedef enum _EDIT_MODE
{
  EDIT_MODE_DEC,
  EDIT_MODE_HEX
} EDIT_MODE;


static const TCHAR* editValueName;
static TCHAR* stringValueData;
static DWORD dwordValueData;
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
        lstrcpy(title, "Error");

    if (!LoadString(hInstance, resId, errfmt, COUNT_OF(errfmt)))
        lstrcpy(errfmt, "Unknown error string!");

    va_start(ap, resId);
    _vsntprintf(errstr, COUNT_OF(errstr), errfmt, ap);
    va_end(ap);

    MessageBox(hwnd, errstr, title, MB_OK | MB_ICONERROR);
}

INT_PTR CALLBACK modify_string_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR* valueData;
    HWND hwndValue;
    int len;

    switch(uMsg) {
    case WM_INITDIALOG:
        if(editValueName && strcmp(editValueName, _T("")))
        {
          SetDlgItemText(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
          SetDlgItemText(hwndDlg, IDC_VALUE_NAME, _T("(Default)"));
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

    oldwndproc = (WNDPROC)GetWindowLong(hwnd, GWL_USERDATA);

    switch (uMsg)
    {
    case WM_CHAR:
        if (dwordEditMode == EDIT_MODE_DEC)
        {
            if (isdigit(wParam & 0xff))
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
            if (isxdigit(wParam & 0xff))
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
    TCHAR* valueData;
    HWND hwndValue;
    int len;
    TCHAR ValueString[32];
    LPTSTR Remainder;
    DWORD Base;
    DWORD Value;

    switch(uMsg) {
    case WM_INITDIALOG:
        dwordEditMode = EDIT_MODE_HEX;

        /* subclass the edit control */
        hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA);
        oldproc = (WNDPROC)GetWindowLong(hwndValue, GWL_WNDPROC);
        SetWindowLong(hwndValue, GWL_USERDATA, (LONG)oldproc);
        SetWindowLong(hwndValue, GWL_WNDPROC, (LONG)DwordEditSubclassProc);

        if(editValueName && strcmp(editValueName, _T("")))
        {
            SetDlgItemText(hwndDlg, IDC_VALUE_NAME, editValueName);
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_VALUE_NAME, _T("(Default)"));
        }
        CheckRadioButton (hwndDlg, IDC_FORMAT_HEX, IDC_FORMAT_DEC, IDC_FORMAT_HEX);
        sprintf (ValueString, "%lx", dwordValueData);
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
                        if (!GetWindowText(hwndValue, ValueString, 32))
                        {
                            Value = 0;
                        }
                        else
                        {
                            Value = strtoul (ValueString, &Remainder, 10);
                        }
                    }
                    else
                    {
                        Value = 0;
                    }
                }
                sprintf (ValueString, "%lx", Value);
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
                        if (!GetWindowText(hwndValue, ValueString, 32))
                        {
                            Value = 0;
                        }
                        else
                        {
                            Value = strtoul (ValueString, &Remainder, 16);
                        }
                    }
                    else
                    {
                        Value = 0;
                    }
                }
                sprintf (ValueString, "%lu", Value);
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
                    dwordValueData = strtoul (ValueString, &Remainder, Base);
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

BOOL ModifyValue(HWND hwnd, HKEY hKey, LPCTSTR valueName)
{
    DWORD valueDataLen;
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
    }

    if (lRet != ERROR_SUCCESS)
    {
        error(hwnd, IDS_BAD_VALUE, valueName);
        goto done;
    }

    if ((type == REG_SZ) || (type == REG_EXPAND_SZ))
    {
        if (valueDataLen > 0)
        {
            if (!(stringValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen)))
            {
                error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
                goto done;
            }
            lRet = RegQueryValueEx(hKey, valueName, 0, 0, stringValueData, &valueDataLen);
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
                lRet = RegSetValueEx(hKey, valueName, 0, type, stringValueData, lstrlen(stringValueData) + 1);
            }
            else
            {
                lRet = RegSetValueEx(hKey, valueName, 0, type, NULL, 0);
            }
            if (lRet == ERROR_SUCCESS)
                result = TRUE;
        }
    }
    else if (type == REG_DWORD)
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
