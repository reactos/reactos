/*
 *  ReactOS
 *  Copyright (C) 2004, 2007 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * PROJECT:         ReactOS Main Control Panel
 * FILE:            dll/cpl/main/keyboard.c
 * PURPOSE:         Keyboard Control Panel
 * PROGRAMMER:      Eric Kohl
 */

#include "main.h"

#define ID_BLINK_TIMER 345

typedef struct _SPEED_DATA
{
    INT nKeyboardDelay;
    INT nOrigKeyboardDelay;
    DWORD dwKeyboardSpeed;
    DWORD dwOrigKeyboardSpeed;
    UINT uCaretBlinkTime;
    UINT uOrigCaretBlinkTime;
    BOOL fShowCursor;
    RECT rcCursor;
} SPEED_DATA, *PSPEED_DATA;

static VOID
UpdateCaretBlinkTimeReg(
    _In_ UINT uCaretBlinkTime)
{
    HKEY hKey;
    WCHAR szBuffer[12];

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Control Panel\\Desktop",
                      0, KEY_SET_VALUE,
                      &hKey) != ERROR_SUCCESS)
    {
         return;
    }

    wsprintf(szBuffer, L"%d", uCaretBlinkTime);

    RegSetValueExW(hKey, L"CursorBlinkRate",
                   0, REG_SZ,
                   (CONST BYTE*)szBuffer,
                   (wcslen(szBuffer) + 1) * sizeof(WCHAR));

    RegCloseKey(hKey);
}

/* Property page dialog callback */
static INT_PTR CALLBACK
KeyboardSpeedProc(IN HWND hwndDlg,
                  IN UINT uMsg,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    PSPEED_DATA pSpeedData;

    pSpeedData = (PSPEED_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pSpeedData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SPEED_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSpeedData);

            /* Get current keyboard delay */
            if (!SystemParametersInfo(SPI_GETKEYBOARDDELAY,
                                      sizeof(INT),
                                      &pSpeedData->nKeyboardDelay,
                                      0))
            {
                pSpeedData->nKeyboardDelay = 2;
            }

            pSpeedData->nOrigKeyboardDelay = pSpeedData->nKeyboardDelay;

            /* Get current keyboard delay */
            if (!SystemParametersInfo(SPI_GETKEYBOARDSPEED,
                                      sizeof(DWORD),
                                      &pSpeedData->dwKeyboardSpeed,
                                      0))
            {
                pSpeedData->dwKeyboardSpeed = 31;
            }

            pSpeedData->dwOrigKeyboardSpeed = pSpeedData->dwKeyboardSpeed;

            pSpeedData->fShowCursor = TRUE;
            GetWindowRect(GetDlgItem(hwndDlg, IDC_TEXT_CURSOR_BLINK), &pSpeedData->rcCursor);
            ScreenToClient(hwndDlg, (LPPOINT)&pSpeedData->rcCursor.left);
            ScreenToClient(hwndDlg, (LPPOINT)&pSpeedData->rcCursor.right);

            /* Get the caret blink time and save its original value */
            pSpeedData->uOrigCaretBlinkTime = GetCaretBlinkTime();
            pSpeedData->uCaretBlinkTime = pSpeedData->uOrigCaretBlinkTime;

            SendDlgItemMessage(hwndDlg, IDC_SLIDER_REPEAT_DELAY, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 3));
            SendDlgItemMessage(hwndDlg, IDC_SLIDER_REPEAT_DELAY, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(3 - pSpeedData->nKeyboardDelay));

            SendDlgItemMessage(hwndDlg, IDC_SLIDER_REPEAT_RATE, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 31));
            SendDlgItemMessage(hwndDlg, IDC_SLIDER_REPEAT_RATE, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pSpeedData->dwKeyboardSpeed);

            SendDlgItemMessage(hwndDlg, IDC_SLIDER_CURSOR_BLINK, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 10));
            SendDlgItemMessage(hwndDlg, IDC_SLIDER_CURSOR_BLINK, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(12 - (pSpeedData->uCaretBlinkTime / 100)));

            /* Start the blink timer */
            pSpeedData->uCaretBlinkTime = pSpeedData->uCaretBlinkTime >= 1200 ? -1 : pSpeedData->uCaretBlinkTime;
            if ((INT)pSpeedData->uCaretBlinkTime > 0)
            {
                SetTimer(hwndDlg, ID_BLINK_TIMER, pSpeedData->uCaretBlinkTime, NULL);
            }
            else
            {
                PostMessage(hwndDlg, WM_TIMER, ID_BLINK_TIMER, 0);
            }

            break;

        case WM_HSCROLL:
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SLIDER_REPEAT_DELAY))
            {
                switch (LOWORD(wParam))
                {
                        case TB_LINEUP:
                        case TB_LINEDOWN:
                        case TB_PAGEUP:
                        case TB_PAGEDOWN:
                        case TB_TOP:
                        case TB_BOTTOM:
                        case TB_ENDTRACK:
                            pSpeedData->nKeyboardDelay = 3 - (INT)SendDlgItemMessage(hwndDlg, IDC_SLIDER_REPEAT_DELAY, TBM_GETPOS, 0, 0);
                            SystemParametersInfo(SPI_SETKEYBOARDDELAY,
                                                 pSpeedData->nKeyboardDelay,
                                                 0,
                                                 0);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;

                        case TB_THUMBTRACK:
                            pSpeedData->nKeyboardDelay = 3 - (INT)HIWORD(wParam);
                            SystemParametersInfo(SPI_SETKEYBOARDDELAY,
                                                 pSpeedData->nKeyboardDelay,
                                                 0,
                                                 0);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;
                }
            }
            else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SLIDER_REPEAT_RATE))
            {
                switch (LOWORD(wParam))
                {
                        case TB_LINEUP:
                        case TB_LINEDOWN:
                        case TB_PAGEUP:
                        case TB_PAGEDOWN:
                        case TB_TOP:
                        case TB_BOTTOM:
                        case TB_ENDTRACK:
                            pSpeedData->dwKeyboardSpeed = (DWORD)SendDlgItemMessage(hwndDlg, IDC_SLIDER_REPEAT_RATE, TBM_GETPOS, 0, 0);
                            SystemParametersInfo(SPI_SETKEYBOARDSPEED,
                                                 pSpeedData->dwKeyboardSpeed,
                                                 0,
                                                 0);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;

                        case TB_THUMBTRACK:
                            pSpeedData->dwKeyboardSpeed = (DWORD)HIWORD(wParam);
                            SystemParametersInfo(SPI_SETKEYBOARDSPEED,
                                                 pSpeedData->dwKeyboardSpeed,
                                                 0,
                                                 0);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;
                }
            }
            else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SLIDER_CURSOR_BLINK))
            {
                switch (LOWORD(wParam))
                {
                        case TB_LINEUP:
                        case TB_LINEDOWN:
                        case TB_PAGEUP:
                        case TB_PAGEDOWN:
                        case TB_TOP:
                        case TB_BOTTOM:
                        case TB_ENDTRACK:
                            pSpeedData->uCaretBlinkTime = (12 - (UINT)SendDlgItemMessage(hwndDlg, IDC_SLIDER_CURSOR_BLINK, TBM_GETPOS, 0, 0)) * 100;
                            KillTimer(hwndDlg, ID_BLINK_TIMER);
                            pSpeedData->uCaretBlinkTime = pSpeedData->uCaretBlinkTime >= 1200 ? -1 : pSpeedData->uCaretBlinkTime;
                            if ((INT)pSpeedData->uCaretBlinkTime > 0)
                            {
                                SetTimer(hwndDlg, ID_BLINK_TIMER, pSpeedData->uCaretBlinkTime, NULL);
                            }
                            else if (pSpeedData->fShowCursor)
                            {
                                SendMessage(hwndDlg, WM_TIMER, ID_BLINK_TIMER, 0);
                            }
                            SetCaretBlinkTime(pSpeedData->uCaretBlinkTime);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;

                        case TB_THUMBTRACK:
                            pSpeedData->uCaretBlinkTime = (12 - (UINT)HIWORD(wParam)) * 100;
                            KillTimer(hwndDlg, ID_BLINK_TIMER);
                            pSpeedData->uCaretBlinkTime = pSpeedData->uCaretBlinkTime >= 1200 ? -1 : pSpeedData->uCaretBlinkTime;
                            if ((INT)pSpeedData->uCaretBlinkTime > 0)
                            {
                                SetTimer(hwndDlg, ID_BLINK_TIMER, pSpeedData->uCaretBlinkTime, NULL);
                            }
                            else if (pSpeedData->fShowCursor)
                            {
                                SendMessage(hwndDlg, WM_TIMER, ID_BLINK_TIMER, 0);
                            }
                            SetCaretBlinkTime(pSpeedData->uCaretBlinkTime);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;
                }
            }
            break;

        case WM_TIMER:
            if (wParam == ID_BLINK_TIMER)
            {
                if (pSpeedData->fShowCursor)
                {
                    HDC hDC = GetDC(hwndDlg);
                    HBRUSH hBrush = GetSysColorBrush(COLOR_BTNTEXT);
                    FillRect(hDC, &pSpeedData->rcCursor, hBrush);
                    ReleaseDC(hwndDlg, hDC);
                }
                else
                {
                    InvalidateRect(hwndDlg, &pSpeedData->rcCursor, TRUE);
                }

                pSpeedData->fShowCursor = !pSpeedData->fShowCursor;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch(lpnm->code)
            {
                case PSN_APPLY:
                    /* Set the new keyboard settings */
                    if (pSpeedData->uOrigCaretBlinkTime != pSpeedData->uCaretBlinkTime)
                    {
                        UpdateCaretBlinkTimeReg(pSpeedData->uCaretBlinkTime);
                    }

                    SystemParametersInfo(SPI_SETKEYBOARDDELAY,
                                         pSpeedData->nKeyboardDelay,
                                         0,
                                         0);
                    SystemParametersInfo(SPI_SETKEYBOARDSPEED,
                                         pSpeedData->dwKeyboardSpeed,
                                         0,
                                         SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                    return TRUE;

                case PSN_RESET:
                    /* Restore the original settings */
                    SetCaretBlinkTime(pSpeedData->uOrigCaretBlinkTime);
                    SystemParametersInfo(SPI_SETKEYBOARDDELAY,
                                         pSpeedData->nOrigKeyboardDelay,
                                         0,
                                         0);
                    SystemParametersInfo(SPI_SETKEYBOARDSPEED,
                                         pSpeedData->dwOrigKeyboardSpeed,
                                         0,
                                         0);
                    break;

                default:
                    break;
            }
        }
        break;

        case WM_DESTROY:
            KillTimer(hwndDlg, ID_BLINK_TIMER);
            HeapFree(GetProcessHeap(), 0, pSpeedData);
            break;
    }

    return FALSE;
}


/* Property page dialog callback */
static INT_PTR CALLBACK
KeybHardwareProc(IN HWND hwndDlg,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    GUID Guids[1];
    Guids[0] = GUID_DEVCLASS_KEYBOARD;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            /* Create the hardware page */
            DeviceCreateHardwarePageEx(hwndDlg,
                                       Guids,
                                       sizeof(Guids) / sizeof(Guids[0]),
                                       HWPD_STANDARDLIST);
            break;
    }

    return FALSE;
}

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDC_CPLICON_2));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

LONG APIENTRY
KeyboardApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    HPROPSHEETPAGE hpsp[MAX_CPL_PAGES];
    PROPSHEETHEADER psh;
    HPSXA hpsxa;
    INT nPage = 0;
    LONG ret;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(hwnd);

    if (uMsg == CPL_STARTWPARMSW && lParam != 0)
        nPage = _wtoi((PWSTR)lParam);

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPTITLE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwnd;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCE(IDC_CPLICON_2);
    psh.pszCaption = MAKEINTRESOURCE(IDS_CPLNAME_2);
    psh.nStartPage = 0;
    psh.phpage = hpsp;
    psh.pfnCallback = PropSheetProc;

    /* Load additional pages provided by shell extensions */
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Keyboard"), MAX_CPL_PAGES - psh.nPages);

    /* NOTE: The speed page (CPLPAGE_KEYBOARD_SPEED) cannot be replaced by
             shell extensions since Win2k! */
    InitPropSheetPage(&psh, IDD_KEYBSPEED, KeyboardSpeedProc);
    InitPropSheetPage(&psh, IDD_HARDWARE, KeybHardwareProc);

    if (hpsxa != NULL)
        SHAddFromPropSheetExtArray(hpsxa, PropSheetAddPage, (LPARAM)&psh);

    if (nPage != 0 && nPage <= psh.nPages)
        psh.nStartPage = nPage;

    ret = (LONG)(PropertySheet(&psh) != -1);

    if (hpsxa != NULL)
        SHDestroyPropSheetExtArray(hpsxa);

    return ret;
}

/* EOF */
