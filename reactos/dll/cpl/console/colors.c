/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/colors.c
 * PURPOSE:         displays colors dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>

static BOOL
PaintStaticControls(HWND hwndDlg,
                    PCONSOLE_PROPS pConInfo,
                    LPDRAWITEMSTRUCT drawItem)
{
    HBRUSH hBrush;
    DWORD index;

    index = min(drawItem->CtlID - IDC_STATIC_COLOR1,
                sizeof(pConInfo->ci.Colors) / sizeof(pConInfo->ci.Colors[0]) - 1);
    hBrush = CreateSolidBrush(pConInfo->ci.Colors[index]);
    if (!hBrush)
    {
        return FALSE;
    }

    FillRect(drawItem->hDC, &drawItem->rcItem, hBrush);
    DeleteObject((HGDIOBJ)hBrush);
    if (pConInfo->ActiveStaticControl == index)
    {
        DrawFocusRect(drawItem->hDC, &drawItem->rcItem);
    }

    return TRUE;
}

INT_PTR CALLBACK
ColorsProc(HWND hwndDlg,
           UINT uMsg,
           WPARAM wParam,
           LPARAM lParam)
{
    PCONSOLE_PROPS pConInfo;
    LPDRAWITEMSTRUCT drawItem;
    DWORD colorIndex;
    COLORREF color;

    pConInfo = (PCONSOLE_PROPS)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pConInfo = (PCONSOLE_PROPS)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pConInfo);

            /* Set the valid range of the colour indicators */
            SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_COLOR_RED), UDM_SETRANGE, 0, (LPARAM)MAKELONG(255, 0));
            SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_COLOR_GREEN), UDM_SETRANGE, 0, (LPARAM)MAKELONG(255, 0));
            SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_COLOR_BLUE), UDM_SETRANGE, 0, (LPARAM)MAKELONG(255, 0));

            /* Select by default the screen background option */
            CheckRadioButton(hwndDlg, IDC_RADIO_SCREEN_TEXT, IDC_RADIO_POPUP_BACKGROUND, IDC_RADIO_SCREEN_BACKGROUND);
            SendMessage(hwndDlg, WM_COMMAND, IDC_RADIO_SCREEN_BACKGROUND, 0);

            return TRUE;
        }

        case WM_DRAWITEM:
        {
            drawItem = (LPDRAWITEMSTRUCT)lParam;
            if (drawItem->CtlID >= IDC_STATIC_COLOR1 && drawItem->CtlID <= IDC_STATIC_COLOR16)
            {
                return PaintStaticControls(hwndDlg, pConInfo, drawItem);
            }
            else if (drawItem->CtlID == IDC_STATIC_SCREEN_COLOR || drawItem->CtlID == IDC_STATIC_POPUP_COLOR)
            {
                PaintText(drawItem, pConInfo);
                return TRUE;
            }
            break;
        }

        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                {
                    // LPPSHNOTIFY lppsn;
                    if (!pConInfo->AppliedConfig)
                    {
                        return ApplyConsoleInfo(hwndDlg, pConInfo);
                    }
                    else
                    {
                        /* Options have already been applied */
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                        return TRUE;
                    }
                    break;
                }

                case UDN_DELTAPOS:
                {
                    LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;

                    /* Get the current color */
                    colorIndex = pConInfo->ActiveStaticControl;
                    color = pConInfo->ci.Colors[colorIndex];

                    if (lpnmud->hdr.idFrom == IDC_UPDOWN_COLOR_RED)
                    {
                        if (lpnmud->iPos < 0) lpnmud->iPos = 0;
                        else if (lpnmud->iPos > 255) lpnmud->iPos = 255;

                        color = RGB(lpnmud->iPos, GetGValue(color), GetBValue(color));
                    }
                    else if (lpnmud->hdr.idFrom == IDC_UPDOWN_COLOR_GREEN)
                    {
                        if (lpnmud->iPos < 0) lpnmud->iPos = 0;
                        else if (lpnmud->iPos > 255) lpnmud->iPos = 255;

                        color = RGB(GetRValue(color), lpnmud->iPos, GetBValue(color));
                    }
                    else if (lpnmud->hdr.idFrom == IDC_UPDOWN_COLOR_BLUE)
                    {
                        if (lpnmud->iPos < 0) lpnmud->iPos = 0;
                        else if (lpnmud->iPos > 255) lpnmud->iPos = 255;

                        color = RGB(GetRValue(color), GetGValue(color), lpnmud->iPos);
                    }
                    else
                    {
                        break;
                    }

                    pConInfo->ci.Colors[colorIndex] = color;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + colorIndex), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);

                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
                }
            }

            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_RADIO_SCREEN_TEXT:
                {
                    /* Get the color of the screen foreground */
                    colorIndex = TextAttribFromAttrib(pConInfo->ci.ScreenAttrib);
                    color = pConInfo->ci.Colors[colorIndex];

                    /* Set the values of the colour indicators */
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(color), FALSE);

                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    pConInfo->ActiveStaticControl = colorIndex;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);
                    break;
                }

                case IDC_RADIO_SCREEN_BACKGROUND:
                {
                    /* Get the color of the screen background */
                    colorIndex = BkgdAttribFromAttrib(pConInfo->ci.ScreenAttrib);
                    color = pConInfo->ci.Colors[colorIndex];

                    /* Set the values of the colour indicators */
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(color), FALSE);

                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    pConInfo->ActiveStaticControl = colorIndex;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);
                    break;
                }

                case IDC_RADIO_POPUP_TEXT:
                {
                    /* Get the color of the popup foreground */
                    colorIndex = TextAttribFromAttrib(pConInfo->ci.PopupAttrib);
                    color = pConInfo->ci.Colors[colorIndex];

                    /* Set the values of the colour indicators */
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(color), FALSE);

                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    pConInfo->ActiveStaticControl = colorIndex;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);
                    break;
                }

                case IDC_RADIO_POPUP_BACKGROUND:
                {
                    /* Get the color of the popup background */
                    colorIndex = BkgdAttribFromAttrib(pConInfo->ci.PopupAttrib);
                    color = pConInfo->ci.Colors[colorIndex];

                    /* Set the values of the colour indicators */
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(color), FALSE);

                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    pConInfo->ActiveStaticControl = colorIndex;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);
                    break;
                }
            }

            if ( HIWORD(wParam) == STN_CLICKED &&
                 IDC_STATIC_COLOR1 <= LOWORD(wParam) && LOWORD(wParam) <= IDC_STATIC_COLOR16 )
            {
                colorIndex = LOWORD(wParam) - IDC_STATIC_COLOR1;

                if (colorIndex == pConInfo->ActiveStaticControl)
                {
                    /* Same static control was re-clicked */
                    break;
                }

                color = pConInfo->ci.Colors[colorIndex];

                SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(color), FALSE);
                SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(color), FALSE);

                /* Update global struct */
                if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_SCREEN_TEXT))
                {
                    pConInfo->ci.ScreenAttrib = MakeAttrib(colorIndex, BkgdAttribFromAttrib(pConInfo->ci.ScreenAttrib));
                }
                else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_SCREEN_BACKGROUND))
                {
                    pConInfo->ci.ScreenAttrib = MakeAttrib(TextAttribFromAttrib(pConInfo->ci.ScreenAttrib), colorIndex);
                }
                else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_POPUP_TEXT))
                {
                    pConInfo->ci.PopupAttrib = MakeAttrib(colorIndex, BkgdAttribFromAttrib(pConInfo->ci.PopupAttrib));
                }
                else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_POPUP_BACKGROUND))
                {
                    pConInfo->ci.PopupAttrib = MakeAttrib(TextAttribFromAttrib(pConInfo->ci.PopupAttrib), colorIndex);
                }

                InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                pConInfo->ActiveStaticControl = colorIndex;
                InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);

                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
        }

        default:
            break;
    }

    return FALSE;
}
