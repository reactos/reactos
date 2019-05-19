/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/console/options.c
 * PURPOSE:         Options dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>

#define MAX_VALUE_NAME 16383


static INT
List_GetCount(IN PLIST_CTL ListCtl)
{
    return (INT)SendMessageW(ListCtl->hWndList, CB_GETCOUNT, 0, 0);
}

static ULONG_PTR
List_GetData(IN PLIST_CTL ListCtl, IN INT Index)
{
    return (ULONG_PTR)SendMessageW(ListCtl->hWndList, CB_GETITEMDATA, (WPARAM)Index, 0);
}

static VOID
AddCodePage(
    IN PLIST_CTL ListCtl,
    IN UINT CodePage)
{
    UINT iItem, iDupItem;
    CPINFOEXW CPInfo;

    /*
     * Add only valid code pages, that is:
     * - If the CodePage is one of the reserved (alias) values:
     *   CP_ACP == 0 ; CP_OEMCP == 1 ; CP_MACCP == 2 ; CP_THREAD_ACP == 3 ;
     *   or the deprecated CP_SYMBOL == 42 (see http://archives.miloush.net/michkap/archive/2005/11/08/490495.html)
     *   it is considered invalid.
     * - If IsValidCodePage() fails because the code page is listed but
     *   not installed on the system, it is also considered invalid.
     */
    if (CodePage == CP_ACP || CodePage == CP_OEMCP || CodePage == CP_MACCP ||
        CodePage == CP_THREAD_ACP || CodePage == CP_SYMBOL || !IsValidCodePage(CodePage))
    {
        return;
    }

    /* Retrieve the code page display name */
    if (!GetCPInfoExW(CodePage, 0, &CPInfo))
    {
        /* We failed, just use the code page value as its name */
        // _ultow(CodePage, CPInfo.CodePageName, 10);
        StringCchPrintfW(CPInfo.CodePageName, ARRAYSIZE(CPInfo.CodePageName), L"%lu", CodePage);
    }

    /* Add the code page into the list, sorted by code page value. Avoid any duplicates. */
    iDupItem = CB_ERR;
    iItem = BisectListSortedByValue(ListCtl, CodePage, &iDupItem, TRUE);
    if (iItem == CB_ERR)
        iItem = 0;
    if (iDupItem != CB_ERR)
        return;
    iItem = (UINT)SendMessageW(ListCtl->hWndList, CB_INSERTSTRING, iItem, (LPARAM)CPInfo.CodePageName);
    if (iItem != CB_ERR && iItem != CB_ERRSPACE)
        iItem = SendMessageW(ListCtl->hWndList, CB_SETITEMDATA, iItem, CodePage);
}

static VOID
BuildCodePageList(
    IN HWND hDlg,
    IN UINT CurrentCodePage)
{
    LIST_CTL ListCtl;
    HKEY hKey;
    DWORD dwIndex, dwSize, dwType;
    UINT CodePage;
    WCHAR szValueName[MAX_VALUE_NAME];

    // #define REGSTR_PATH_CODEPAGE    TEXT("System\\CurrentControlSet\\Control\\Nls\\CodePage")
    /* Open the Nls\CodePage key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"System\\CurrentControlSet\\Control\\Nls\\CodePage",
                      0,
                      KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                      &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    ListCtl.hWndList = GetDlgItem(hDlg, IDL_CODEPAGE);
    ListCtl.GetCount = List_GetCount;
    ListCtl.GetData  = List_GetData;

    /* Enumerate all the available code pages on the system */
    dwSize  = ARRAYSIZE(szValueName);
    dwIndex = 0;
    while (RegEnumValueW(hKey, dwIndex, szValueName, &dwSize,
                         NULL, &dwType, NULL, NULL) == ERROR_SUCCESS) // != ERROR_NO_MORE_ITEMS
    {
        /* Ignore these parameters, prepare for next iteration */
        dwSize = ARRAYSIZE(szValueName);
        ++dwIndex;

        /* Check the value type validity */
        if (dwType != REG_SZ)
            continue;

        /*
         * Add the code page into the list.
         * If _wtol fails and returns 0, the code page is considered invalid
         * (and indeed this value corresponds to the CP_ACP alias too).
         */
        CodePage = (UINT)_wtol(szValueName);
        if (CodePage == 0) continue;
        AddCodePage(&ListCtl, CodePage);
    }

    RegCloseKey(hKey);

    /* Add the special UTF-7 (CP_UTF7 65000) and UTF-8 (CP_UTF8 65001) code pages */
    AddCodePage(&ListCtl, CP_UTF7);
    AddCodePage(&ListCtl, CP_UTF8);

    /* Find and select the current code page in the sorted list */
    if (BisectListSortedByValue(&ListCtl, CurrentCodePage, &CodePage, FALSE) == CB_ERR ||
        CodePage == CB_ERR)
    {
        /* Not found, select the first element */
        CodePage = 0;
    }
    SendMessageW(ListCtl.hWndList, CB_SETCURSEL, (WPARAM)CodePage, 0);
}

static VOID
UpdateDialogElements(
    IN HWND hDlg,
    IN PCONSOLE_STATE_INFO pConInfo)
{
    /* Update the cursor size */
    if (pConInfo->CursorSize <= 25)
    {
        /* Small cursor */
        CheckRadioButton(hDlg, IDC_RADIO_SMALL_CURSOR, IDC_RADIO_LARGE_CURSOR, IDC_RADIO_SMALL_CURSOR);
        // CheckDlgButton(hDlg, IDC_RADIO_SMALL_CURSOR , BST_CHECKED);
        // CheckDlgButton(hDlg, IDC_RADIO_MEDIUM_CURSOR, BST_UNCHECKED);
        // CheckDlgButton(hDlg, IDC_RADIO_LARGE_CURSOR , BST_UNCHECKED);
    }
    else if (pConInfo->CursorSize <= 50)
    {
        /* Medium cursor */
        CheckRadioButton(hDlg, IDC_RADIO_SMALL_CURSOR, IDC_RADIO_LARGE_CURSOR, IDC_RADIO_MEDIUM_CURSOR);
        // CheckDlgButton(hDlg, IDC_RADIO_SMALL_CURSOR , BST_UNCHECKED);
        // CheckDlgButton(hDlg, IDC_RADIO_MEDIUM_CURSOR, BST_CHECKED);
        // CheckDlgButton(hDlg, IDC_RADIO_LARGE_CURSOR , BST_UNCHECKED);
    }
    else /* if (pConInfo->CursorSize <= 100) */
    {
        /* Large cursor */
        CheckRadioButton(hDlg, IDC_RADIO_SMALL_CURSOR, IDC_RADIO_LARGE_CURSOR, IDC_RADIO_LARGE_CURSOR);
        // CheckDlgButton(hDlg, IDC_RADIO_SMALL_CURSOR , BST_UNCHECKED);
        // CheckDlgButton(hDlg, IDC_RADIO_MEDIUM_CURSOR, BST_UNCHECKED);
        // CheckDlgButton(hDlg, IDC_RADIO_LARGE_CURSOR , BST_CHECKED);
    }

    /* Update the number of history buffers */
    SendDlgItemMessageW(hDlg, IDC_UPDOWN_NUM_BUFFER, UDM_SETRANGE, 0, MAKELONG(999, 1));
    SetDlgItemInt(hDlg, IDC_EDIT_NUM_BUFFER, pConInfo->NumberOfHistoryBuffers, FALSE);

    /* Update the history buffer size */
    SendDlgItemMessageW(hDlg, IDC_UPDOWN_BUFFER_SIZE, UDM_SETRANGE, 0, MAKELONG(999, 1));
    SetDlgItemInt(hDlg, IDC_EDIT_BUFFER_SIZE, pConInfo->HistoryBufferSize, FALSE);

    /* Update discard duplicates */
    CheckDlgButton(hDlg, IDC_CHECK_DISCARD_DUPLICATES,
                   pConInfo->HistoryNoDup ? BST_CHECKED : BST_UNCHECKED);

    /* Update full/window screen state */
    if (pConInfo->FullScreen)
    {
        CheckRadioButton(hDlg, IDC_RADIO_DISPLAY_WINDOW, IDC_RADIO_DISPLAY_FULL, IDC_RADIO_DISPLAY_FULL);
        // CheckDlgButton(hDlg, IDC_RADIO_DISPLAY_WINDOW, BST_UNCHECKED);
        // CheckDlgButton(hDlg, IDC_RADIO_DISPLAY_FULL  , BST_CHECKED);
    }
    else
    {
        CheckRadioButton(hDlg, IDC_RADIO_DISPLAY_WINDOW, IDC_RADIO_DISPLAY_FULL, IDC_RADIO_DISPLAY_WINDOW);
        // CheckDlgButton(hDlg, IDC_RADIO_DISPLAY_WINDOW, BST_CHECKED);
        // CheckDlgButton(hDlg, IDC_RADIO_DISPLAY_FULL  , BST_UNCHECKED);
    }

    /* Update "Quick-edit" state */
    CheckDlgButton(hDlg, IDC_CHECK_QUICK_EDIT,
                   pConInfo->QuickEdit ? BST_CHECKED : BST_UNCHECKED);

    /* Update "Insert mode" state */
    CheckDlgButton(hDlg, IDC_CHECK_INSERT_MODE,
                   pConInfo->InsertMode ? BST_CHECKED : BST_UNCHECKED);
}

INT_PTR
CALLBACK
OptionsProc(HWND hDlg,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            BuildCodePageList(hDlg, ConInfo->CodePage);
            UpdateDialogElements(hDlg, ConInfo);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPPSHNOTIFY lppsn = (LPPSHNOTIFY)lParam;

            if (lppsn->hdr.code == UDN_DELTAPOS)
            {
                LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;

                if (lppsn->hdr.idFrom == IDC_UPDOWN_BUFFER_SIZE)
                {
                    lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 1), 999);
                    ConInfo->HistoryBufferSize = lpnmud->iPos;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }
                else if (lppsn->hdr.idFrom == IDC_UPDOWN_NUM_BUFFER)
                {
                    lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 1), 999);
                    ConInfo->NumberOfHistoryBuffers = lpnmud->iPos;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                ApplyConsoleInfo(hDlg);
                return TRUE;
            }
            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                case IDC_RADIO_SMALL_CURSOR:
                {
                    ConInfo->CursorSize = 25;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case IDC_RADIO_MEDIUM_CURSOR:
                {
                    ConInfo->CursorSize = 50;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case IDC_RADIO_LARGE_CURSOR:
                {
                    ConInfo->CursorSize = 100;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case IDC_RADIO_DISPLAY_WINDOW:
                {
                    ConInfo->FullScreen = FALSE;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case IDC_RADIO_DISPLAY_FULL:
                {
                    ConInfo->FullScreen = TRUE;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case IDC_CHECK_QUICK_EDIT:
                {
                    ConInfo->QuickEdit = (IsDlgButtonChecked(hDlg, IDC_CHECK_QUICK_EDIT) == BST_CHECKED); // BST_UNCHECKED or BST_INDETERMINATE => FALSE
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case IDC_CHECK_INSERT_MODE:
                {
                    ConInfo->InsertMode = (IsDlgButtonChecked(hDlg, IDC_CHECK_INSERT_MODE) == BST_CHECKED); // BST_UNCHECKED or BST_INDETERMINATE => FALSE
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case IDC_CHECK_DISCARD_DUPLICATES:
                {
                    ConInfo->HistoryNoDup = (IsDlgButtonChecked(hDlg, IDC_CHECK_DISCARD_DUPLICATES) == BST_CHECKED); // BST_UNCHECKED or BST_INDETERMINATE => FALSE
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                }
            }
            else
            if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                switch (LOWORD(wParam))
                {
                case IDC_EDIT_BUFFER_SIZE:
                {
                    DWORD sizeBuff;

                    sizeBuff = GetDlgItemInt(hDlg, IDC_EDIT_BUFFER_SIZE, NULL, FALSE);
                    sizeBuff = min(max(sizeBuff, 1), 999);

                    ConInfo->HistoryBufferSize = sizeBuff;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                case IDC_EDIT_NUM_BUFFER:
                {
                    DWORD numBuff;

                    numBuff = GetDlgItemInt(hDlg, IDC_EDIT_NUM_BUFFER, NULL, FALSE);
                    numBuff = min(max(numBuff, 1), 999);

                    ConInfo->NumberOfHistoryBuffers = numBuff;
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                }
            }
            else
            // (HIWORD(wParam) == CBN_KILLFOCUS)
            if ((HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_SELENDOK) &&
                (LOWORD(wParam) == IDL_CODEPAGE))
            {
                HWND hWndList = GetDlgItem(hDlg, IDL_CODEPAGE);
                INT iItem;
                UINT CodePage;

                iItem = (INT)SendMessageW(hWndList, CB_GETCURSEL, 0, 0);
                if (iItem == CB_ERR)
                    break;

                CodePage = (UINT)SendMessageW(hWndList, CB_GETITEMDATA, iItem, 0);
                if (CodePage == CB_ERR)
                    break;

                /* If the user validated a different code page... */
                if ((HIWORD(wParam) == CBN_SELENDOK) && (CodePage != ConInfo->CodePage))
                {
                    /* ... update the code page, notify the siblings and change the property sheet state */
                    ConInfo->CodePage = CodePage;
                    // PropSheet_QuerySiblings(GetParent(hDlg), IDL_CODEPAGE, 0);
                    ResetFontPreview(&FontPreview);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }
            }

            break;
        }

        default:
            break;
    }

    return FALSE;
}
