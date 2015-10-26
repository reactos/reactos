/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/freeldrpage.c
 * PURPOSE:     Freeloader configuration page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *                        2011      Gregor Schneider <Gregor.Schneider@reactos.org>
 *              Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include <share.h>

#include <wingdi.h>
#include <WindowsX.h>

#include "utils.h"

static HWND hFreeLdrPage;

LPCWSTR lpszFreeLdrIni = L"%SystemDrive%\\freeldr.ini";
LPCWSTR lpszBootIni    = L"%SystemDrive%\\boot.ini";

typedef struct _FREELDR_SETTINGS
{
    ULONG TimeOut;
    WCHAR szDefaultOS[512];
    ULONG szDefaultPos;
    ULONG OSConfigurationCount;
    BOOL  UseBootIni;
} FREELDR_SETTINGS;

static FREELDR_SETTINGS Settings = { 0, {0}, 0, 0, FALSE };

static BOOL
LoadIniFile(HWND hDlg,
            LPCWSTR lpszIniFile)
{
    DWORD  dwNumOfChars;
    LPWSTR lpszFileName;
    FILE*  file;

    WCHAR szBuffer[512];
    HWND hDlgCtrl;
    SIZE_T length;
    LRESULT pos;

    SIZE size;
    LONG horzExt;

    HDC hDC;
    HFONT hFont, hOldFont;

    /*
     * Open for read + write (without file creation if it didn't already exist)
     * of a read-only text stream.
     */
    dwNumOfChars = ExpandEnvironmentStringsW(lpszIniFile, NULL, 0);
    lpszFileName = (LPWSTR)MemAlloc(0, dwNumOfChars * sizeof(WCHAR));
    ExpandEnvironmentStringsW(lpszIniFile, lpszFileName, dwNumOfChars);

    file = _wfsopen(lpszFileName, L"rt", _SH_DENYWR); // r+t <-- read write text ; rt <-- read text
    MemFree(lpszFileName);

    if (!file) return FALSE;

    hDlgCtrl = GetDlgItem(hDlg, IDC_LIST_BOX);

    hDC      = GetDC(hDlgCtrl);
    hFont    = (HFONT)SendMessageW(hDlgCtrl, WM_GETFONT, 0, 0);
    hOldFont = (HFONT)SelectObject(hDC, hFont);

    while (!feof(file))
    {
        if (fgetws(szBuffer, ARRAYSIZE(szBuffer), file))
        {
            length = wcslen(szBuffer);
            if (length > 1)
            {
                /* Remove \r\n */
                szBuffer[length-1] = szBuffer[length] = L'\0';

                pos = ListBox_AddString(hDlgCtrl, szBuffer);

                GetTextExtentPoint32W(hDC, szBuffer, (int)wcslen(szBuffer), &size);
                horzExt = max((LONG)ListBox_GetHorizontalExtent(hDlgCtrl), size.cx + 5); // 5 to have a little room between the text and the end of the list box.
                ListBox_SetHorizontalExtent(hDlgCtrl, horzExt);

                if (szBuffer[0] == L'[')
                    continue;

                if (!_wcsnicmp(szBuffer, L"timeout=", 8))
                {
                    Settings.TimeOut = _wtoi(&szBuffer[8]);
                    continue;
                }

                if (!_wcsnicmp(szBuffer, L"default=", 8))
                {
                    wcscpy(Settings.szDefaultOS, &szBuffer[8]);
                    continue;
                }
                if (pos != LB_ERR)
                    ListBox_SetItemData(hDlgCtrl, pos, 1); // indicate that this item is a boot entry

                Settings.OSConfigurationCount++;
            }
        }
    }

    SelectObject(hDC, hOldFont);
    ReleaseDC(hDlgCtrl, hDC);

    fclose(file);
    Settings.UseBootIni = TRUE;

    /*
     * Start to search for the string at the "operating systems" section
     * (after the "boot loader" section, which takes 3 lines in the .INI file).
     */
    pos = ListBox_FindString(hDlgCtrl, 3, Settings.szDefaultOS);
    if (pos != LB_ERR)
    {
        Settings.szDefaultPos = (ULONG)pos;
        ListBox_SetCurSel(hDlgCtrl, pos);
        // SendMessageW(hDlgCtrl, WM_VSCROLL, SB_LINEDOWN, 0); // Or use SetScroll...()
    }

    SetDlgItemInt(hDlg, IDC_TXT_BOOT_TIMEOUT, Settings.TimeOut, FALSE);
    if (Settings.OSConfigurationCount < 2)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SET_DEFAULT_BOOT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_MOVE_UP_BOOT_OPTION), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_MOVE_DOWN_BOOT_OPTION), FALSE);
    }

    return TRUE;
}

INT_PTR CALLBACK
FreeLdrPageWndProc(HWND   hDlg,
                   UINT   message,
                   WPARAM wParam,
                   LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW psp = (LPPROPSHEETPAGEW)lParam;

            hFreeLdrPage = hDlg;
            LoadIniFile(hDlg, (LPWSTR)(psp->lParam));
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BTN_ADVANCED_OPTIONS:
                    // DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_FREELDR_ADVANCED_DIALOG), hDlg /* hMainWnd */, NULL /*FileExtractDialogWndProc*/);
                    break;

                // default:
                //     return FALSE;
            }

            switch (HIWORD(wParam))
            {
                case LBN_SELCHANGE:
                {
                    HWND hWnd = (HWND)lParam;
                    LRESULT pos;

                    pos = ListBox_GetCurSel(hWnd);
                    if (pos != LB_ERR)
                    {
                        if (!ListBox_GetItemData(hWnd, pos)) // Line is not a default one
                            ListBox_SetCurSel(hWnd, Settings.szDefaultPos);
                        else
                            Settings.szDefaultPos = (ULONG)pos;

                        // SendMessageW((HWND)lParam, WM_VSCROLL, SB_LINEDOWN, 0); // Or use SetScroll...()
                    }

                    return TRUE;
                }
            }
        }

        default:
            return FALSE;
    }

    // return FALSE;
}
