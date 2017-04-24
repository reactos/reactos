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


/*
 * A function that locates the insertion point (index) for a given value 'Value'
 * in a list 'List' to maintain its sorted order by increasing values.
 *
 * - When 'BisectRightOrLeft' == TRUE, the bisection is performed to the right,
 *   i.e. the returned insertion point comes after (to the right of) any existing
 *   entries of 'Value' in 'List'.
 *   The returned insertion point 'i' partitions the list 'List' into two halves
 *   such that:
 *       all(val <= Value for val in List[start:i[) for the left side, and
 *       all(val >  Value for val in List[i:end+1[) for the right side.
 *
 * - When 'BisectRightOrLeft' == FALSE, the bisection is performed to the left,
 *   i.e. the returned insertion point comes before (to the left of) any existing
 *   entries of 'Value' in 'List'.
 *   The returned insertion point 'i' partitions the list 'List' into two halves
 *   such that:
 *       all(val <  Value for val in List[start:i[) for the left side, and
 *       all(val >= Value for val in List[i:end+1[) for the right side.
 *
 * The exact value of List[i] may, or may not, be equal to Value, depending on
 * whether or not 'Value' is actually present on the list.
 */
static UINT
BisectListSortedByValueEx(
    IN HWND hWndList,
    IN ULONG_PTR Value,
    IN UINT itemStart,
    IN UINT itemEnd,
    OUT PUINT pValueItem OPTIONAL,
    IN BOOL BisectRightOrLeft)
{
    UINT iItemStart, iItemEnd, iItem;
    ULONG_PTR itemData;

    /* Sanity checks */
    if (itemStart > itemEnd)
        return CB_ERR; // Fail

    /* Initialize */
    iItemStart = itemStart;
    iItemEnd = itemEnd;
    iItem = iItemStart;

    if (pValueItem)
        *pValueItem = CB_ERR;

    while (iItemStart <= iItemEnd)
    {
        /*
         * Bisect. Note the following:
         * - if iItemEnd == iItemStart + 1, then iItem == iItemStart;
         * - if iItemStart == iItemEnd, then iItemStart == iItem == iItemEnd.
         * In all but the last case, iItemStart <= iItem < iItemEnd.
         */
        iItem = (iItemStart + iItemEnd) / 2;

        itemData = (ULONG_PTR)SendMessageW(hWndList, CB_GETITEMDATA, (WPARAM)iItem, 0);
        if (itemData == CB_ERR)
            return CB_ERR; // Fail

        if (Value == itemData)
        {
            /* Found a candidate */
            if (pValueItem)
                *pValueItem = iItem;

            /*
             * Try to find the last element (if BisectRightOrLeft == TRUE)
             * or the first element (if BisectRightOrLeft == FALSE).
             */
            if (BisectRightOrLeft)
            {
                iItemStart = iItem + 1; // iItemStart may be > iItemEnd
            }
            else
            {
                if (iItem <= itemStart) break;
                iItemEnd = iItem - 1;   // iItemEnd may be < iItemStart, i.e. iItemStart may be > iItemEnd
            }
        }
        else if (Value < itemData)
        {
            if (iItem <= itemStart) break;
            /* The value should be before iItem */
            iItemEnd = iItem - 1;   // iItemEnd may be < iItemStart, i.e. iItemStart may be > iItemEnd, if iItem == iItemStart.
        }
        else // if (itemData < Value)
        {
            /* The value should be after iItem */
            iItemStart = iItem + 1; // iItemStart may be > iItemEnd, if iItem == iItemEnd.
        }

        /* Here, iItemStart may be == iItemEnd */
    }

    return iItemStart;
}

static UINT
BisectListSortedByValue(
    IN HWND hWndList,
    IN ULONG_PTR Value,
    OUT PUINT pValueItem OPTIONAL,
    IN BOOL BisectRightOrLeft)
{
    INT iItemEnd = (INT)SendMessageW(hWndList, CB_GETCOUNT, 0, 0);
    if (iItemEnd == CB_ERR || iItemEnd <= 0)
        return CB_ERR; // Fail

    return BisectListSortedByValueEx(hWndList, Value,
                                     0, (UINT)(iItemEnd - 1),
                                     pValueItem,
                                     BisectRightOrLeft);
}


static VOID
AddCodePage(
    IN HWND hWndList,
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
        _snwprintf(CPInfo.CodePageName, ARRAYSIZE(CPInfo.CodePageName), L"%lu", CodePage);
    }

    /* Add the code page into the list, sorted by code page value. Avoid any duplicates. */
    iDupItem = CB_ERR;
    iItem = BisectListSortedByValue(hWndList, CodePage, &iDupItem, TRUE);
    if (iItem == CB_ERR)
        iItem = 0;
    if (iDupItem != CB_ERR)
        return;
    iItem = (UINT)SendMessageW(hWndList, CB_INSERTSTRING, iItem, (LPARAM)CPInfo.CodePageName);
    if (iItem != CB_ERR && iItem != CB_ERRSPACE)
        iItem = SendMessageW(hWndList, CB_SETITEMDATA, iItem, CodePage);
}

static VOID
BuildCodePageList(IN HWND hDlg)
{
    HWND hWndList;
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

    hWndList = GetDlgItem(hDlg, IDL_CODEPAGE);

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
        AddCodePage(hWndList, CodePage);
    }

    RegCloseKey(hKey);

    /* Add the special UTF-7 (CP_UTF7 65000) and UTF-8 (CP_UTF8 65001) code pages */
    AddCodePage(hWndList, CP_UTF7);
    AddCodePage(hWndList, CP_UTF8);

    /* Find and select the current code page in the sorted list */
    if (BisectListSortedByValue(hWndList, ConInfo->CodePage, &CodePage, FALSE) == CB_ERR ||
        CodePage == CB_ERR)
    {
        /* Not found, select the first element */
        CodePage = 0;
    }
    SendMessageW(hWndList, CB_SETCURSEL, (WPARAM)CodePage, 0);
}

static VOID
UpdateDialogElements(HWND hwndDlg, PCONSOLE_STATE_INFO pConInfo)
{
    HWND hDlgCtrl;

    /* Update cursor size */
    if (pConInfo->CursorSize <= 25)
    {
        /* Small cursor */
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }
    else if (pConInfo->CursorSize <= 50)
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }
    else /* if (pConInfo->CursorSize <= 100) */
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_LARGE_CURSOR);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_SMALL_CURSOR);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_MEDIUM_CURSOR);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }

    /* Update num buffers */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_NUM_BUFFER);
    SendMessageW(hDlgCtrl, UDM_SETRANGE, 0, MAKELONG(999, 1));
    SetDlgItemInt(hwndDlg, IDC_EDIT_NUM_BUFFER, pConInfo->NumberOfHistoryBuffers, FALSE);

    /* Update buffer size */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_BUFFER_SIZE);
    SendMessageW(hDlgCtrl, UDM_SETRANGE, 0, MAKELONG(999, 1));
    SetDlgItemInt(hwndDlg, IDC_EDIT_BUFFER_SIZE, pConInfo->HistoryBufferSize, FALSE);

    /* Update discard duplicates */
    CheckDlgButton(hwndDlg, IDC_CHECK_DISCARD_DUPLICATES,
                   pConInfo->HistoryNoDup ? BST_CHECKED : BST_UNCHECKED);

    /* Update full/window screen */
    if (pConInfo->FullScreen)
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_FULL);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_WINDOW);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }
    else
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_WINDOW);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_DISPLAY_FULL);
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
    }

    /* Update quick edit */
    CheckDlgButton(hwndDlg, IDC_CHECK_QUICK_EDIT,
                   pConInfo->QuickEdit ? BST_CHECKED : BST_UNCHECKED);

    /* Update insert mode */
    CheckDlgButton(hwndDlg, IDC_CHECK_INSERT_MODE,
                   pConInfo->InsertMode ? BST_CHECKED : BST_UNCHECKED);
}

INT_PTR
CALLBACK
OptionsProc(HWND hwndDlg,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            BuildCodePageList(hwndDlg);
            UpdateDialogElements(hwndDlg, ConInfo);
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
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                else if (lppsn->hdr.idFrom == IDC_UPDOWN_NUM_BUFFER)
                {
                    lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 1), 999);
                    ConInfo->NumberOfHistoryBuffers = lpnmud->iPos;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
            }
            else if (lppsn->hdr.code == PSN_APPLY)
            {
                ApplyConsoleInfo(hwndDlg);
                return TRUE;
            }
            break;
        }

        case WM_COMMAND:
        {
            LRESULT lResult;

            switch (LOWORD(wParam))
            {
                case IDC_RADIO_SMALL_CURSOR:
                {
                    ConInfo->CursorSize = 25;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_MEDIUM_CURSOR:
                {
                    ConInfo->CursorSize = 50;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_LARGE_CURSOR:
                {
                    ConInfo->CursorSize = 100;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_DISPLAY_WINDOW:
                {
                    ConInfo->FullScreen = FALSE;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_RADIO_DISPLAY_FULL:
                {
                    ConInfo->FullScreen = TRUE;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_CHECK_QUICK_EDIT:
                {
                    lResult = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0);
                    if (lResult == BST_CHECKED)
                    {
                        ConInfo->QuickEdit = FALSE;
                        SendMessageW((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        ConInfo->QuickEdit = TRUE;
                        SendMessageW((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                    }
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_CHECK_INSERT_MODE:
                {
                    lResult = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0);
                    if (lResult == BST_CHECKED)
                    {
                        ConInfo->InsertMode = FALSE;
                        SendMessageW((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        ConInfo->InsertMode = TRUE;
                        SendMessageW((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                    }
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_CHECK_DISCARD_DUPLICATES:
                {
                   lResult = SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0);
                    if (lResult == BST_CHECKED)
                    {
                        ConInfo->HistoryNoDup = FALSE;
                        SendMessageW((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        ConInfo->HistoryNoDup = TRUE;
                        SendMessageW((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                    }
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
                case IDC_EDIT_BUFFER_SIZE:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD sizeBuff;

                        sizeBuff = GetDlgItemInt(hwndDlg, IDC_EDIT_BUFFER_SIZE, NULL, FALSE);
                        sizeBuff = min(max(sizeBuff, 1), 999);

                        ConInfo->HistoryBufferSize = sizeBuff;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }
                case IDC_EDIT_NUM_BUFFER:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD numBuff;

                        numBuff = GetDlgItemInt(hwndDlg, IDC_EDIT_NUM_BUFFER, NULL, FALSE);
                        numBuff = min(max(numBuff, 1), 999);

                        ConInfo->NumberOfHistoryBuffers = numBuff;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }
                case IDL_CODEPAGE:
                {
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        INT iItem;
                        UINT CodePage;

                        iItem = (INT)SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);
                        if (iItem != CB_ERR)
                        {
                            CodePage = (UINT)SendMessageW((HWND)lParam, CB_GETITEMDATA, iItem, 0);
                            if (CodePage != CB_ERR)
                            {
                                ConInfo->CodePage = CodePage;
                                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return FALSE;
}
