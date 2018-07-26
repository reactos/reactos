/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * PURPOSE:     Auto Check (autochk) page handler
 * COPYRIGHT:   Copyright 2018 Bișoc George (fraizeraust99 at gmail dot com)
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"
#include "resource.h"
#include "autochkpage.h"

/* VARIABLES *******************************************************************/

HWND hAutoChkPage;
HWND hAutoChkDialog;
WCHAR SubKey[] = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager";
WCHAR ValueName[] = L"BootExecute";
WCHAR EnableValue[] = L"autocheck autochk *\0";

/* FUNCTIONS *******************************************************************/

INT_PTR CALLBACK
AutoChkPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HKEY hKey;
    LONG lResult;
    WCHAR szValueData[255];
    DWORD dwcbData = sizeof(szValueData);
    DWORD dwBufferSize = 0;
    LPWSTR lpszzMultiString = NULL;
    size_t size;
    LPWSTR ptr;
    DWORD dwType;
    PWCHAR pszOldValue;
    DWORD cbData;

    UNREFERENCED_PARAMETER(lParam);
  
    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

            // Open the key so we can query it
            lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   SubKey,
                                   0,
                                   KEY_READ,
                                   &hKey);

            if (lResult != ERROR_SUCCESS)
            {
                // We failed
                return FALSE;
            }

            lResult = RegQueryValueExW(hKey,
                                       ValueName,
                                       NULL,
                                       NULL,
                                       (LPBYTE)szValueData,
                                       &dwcbData);

            RegCloseKey(hKey);

            if (lResult != ERROR_SUCCESS)
            {
                // We failed
                return FALSE;
            }

            // Check the array if it's empty
            if (szValueData[0] == L'\0')
            {
                // If the array is empty, always set the Disable radio button
                SendDlgItemMessage(hDlg, IDC_AUTOCHK_DISABLE, BM_SETCHECK, BST_CHECKED, 0);
            }
            else
            {
                // Otherwise, always set the Enable radio button
                SendDlgItemMessage(hDlg, IDC_AUTOCHK_ENABLE, BM_SETCHECK, BST_CHECKED, 0);
            }

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch ( LOWORD(wParam) )
            {
                case IDC_AUTOCHK_ENABLE:
                {

                    // Open the key so we can enable autochk
                    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                           SubKey,
                                           0,
                                           KEY_SET_VALUE,
                                           &hKey);

                    if (lResult != ERROR_SUCCESS)
                    {
                        // We failed (but return 0 since we're processing the message anyways)
                        return 0;
                    }

                    lResult = RegQueryValueExW(hKey,
                                               ValueName,
                                               NULL,
                                               NULL,
                                               (LPBYTE)szValueData,
                                               &dwcbData);

                    RegCloseKey(hKey);

                    if (lResult != ERROR_SUCCESS)
                    {
                        // We failed (but return 0 since we're processing the message anyways)
                        return 0;
                    }

                    // Check if the array is empty
                    if (szValueData[0] == L'\0')
                    {
                        // If it is, add the data contents directly
                        lResult = RegSetValueEx(hKey,
                                                ValueName,
                                                0,
                                                REG_MULTI_SZ,
                                                (const BYTE *)EnableValue,
                                                sizeof(EnableValue));

                        RegCloseKey(hKey);

                        if (lResult != ERROR_SUCCESS)
                        {
                            // We failed
                            return 1;
                        }
                    }
                    else
                    {
                        // Otherwise add the value to the existing data
                        StringCbCopy(szValueData, sizeof(szValueData), EnableValue);
                        pszOldValue = szValueData + wcslen(EnableValue) + 1;

                        lResult = RegQueryValueExW(hKey,
                                                  ValueName,
                                                  NULL,
                                                  NULL,
                                                  (LPBYTE)pszOldValue,
                                                  &dwcbData);

                        RegCloseKey(hKey);

                        if (lResult != ERROR_SUCCESS)
                        {
                            // Bail out
                            return 1;
                        }

                        cbData = dwcbData + (wcslen(EnableValue) + 1) * sizeof(WCHAR);

                        lResult = RegSetValueEx(hKey,
                                                ValueName,
                                                0,
                                                REG_MULTI_SZ,
                                                (const BYTE *)pszOldValue,
                                                cbData);

                        RegCloseKey(hKey);

                        if (lResult != ERROR_SUCCESS)
                        {
                            // Bail out
                            return 1;
                        }
                    }

                    break;
                }

                case IDC_AUTOCHK_DISABLE:
                {
                    // Open the key so we can query it
                    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                           SubKey,
                                           0,
                                           KEY_ALL_ACCESS,
                                           &hKey);

                    if (lResult != ERROR_SUCCESS)
                    {
                        // We failed (but return 0 since we're processing the message anyways)
                        return 0;
                    }

                    lResult = RegQueryValueExW(hKey,
                                               ValueName,
                                               NULL,
                                               &dwType,
                                               NULL,
                                               &dwcbData);

                    if (lResult != ERROR_SUCCESS || dwType != REG_MULTI_SZ)
                    {
                        // Error, bail out
                        RegCloseKey(hKey);
                        return 1;
                    }

                    // Allocate from the heap for the variable
                    lpszzMultiString = HeapAlloc(GetProcessHeap(), 0, dwcbData);

                    if (!lpszzMultiString)
                    {
                        // Allocation failed, bail out.
                        RegCloseKey(hKey);
                        return 1;
                    }

                    lResult = RegQueryValueExW(hKey,
                                               ValueName,
                                               NULL,
                                               NULL,
                                               (LPBYTE)lpszzMultiString,
                                               &dwcbData);

                    if (lResult != ERROR_SUCCESS)
                    {
                        // An error happened... free memory & close the key & bail out
                        HeapFree(GetProcessHeap(), 0, lpszzMultiString);
                        RegCloseKey(hKey);
                        return 1;
                    }

                    dwBufferSize = dwcbData;
                    ptr = lpszzMultiString;

                    while (*ptr)
                    {
                        size = wcslen(ptr) + 1;
                        if (_wcsnicmp(ptr, L"autocheck ", 10) == 0)
                        {
                            /*
                             * Found a string to remove. To do that, we just
                             * move everything that is after it backwards.
                            */
                            DWORD sizeToCopy;
                            dwBufferSize -= size * sizeof(WCHAR); // Size that remains after the string to be removed.
                            sizeToCopy = dwBufferSize - (ptr - lpszzMultiString) * sizeof(WCHAR);
                            memmove(ptr, ptr + size, sizeToCopy);
                            continue;
                        }
 
                        /* Otherwise, continue with the next string */
                        ptr += size;
                    }

                    // If all went good from previous points, disable autochk
                    lResult = RegSetValueEx(hKey,
                                            ValueName,
                                            0,
                                            REG_MULTI_SZ,
                                            (const BYTE *)lpszzMultiString,
                                            dwBufferSize);

                    RegCloseKey(hKey);

                    if (lResult != ERROR_SUCCESS)
                    {
                        // We failed
                        return 1;
                    }

                    break;

                    default:
                        break;
                }
            }
        }
    }
    return 0;
}
