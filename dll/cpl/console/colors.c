/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/colors.c
 * PURPOSE:         Colors dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
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
    if (!hBrush) return FALSE;

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
            SendDlgItemMessageW(hwndDlg, IDC_UPDOWN_COLOR_RED  , UDM_SETRANGE, 0, (LPARAM)MAKELONG(255, 0));
            SendDlgItemMessageW(hwndDlg, IDC_UPDOWN_COLOR_GREEN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(255, 0));
            SendDlgItemMessageW(hwndDlg, IDC_UPDOWN_COLOR_BLUE , UDM_SETRANGE, 0, (LPARAM)MAKELONG(255, 0));

            /* Select by default the screen background option */
            CheckRadioButton(hwndDlg, IDC_RADIO_SCREEN_TEXT, IDC_RADIO_POPUP_BACKGROUND, IDC_RADIO_SCREEN_BACKGROUND);
            SendMessage(hwndDlg, WM_COMMAND, IDC_RADIO_SCREEN_BACKGROUND, 0);

            return TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;

            if (drawItem->CtlID >= IDC_STATIC_COLOR1 && drawItem->CtlID <= IDC_STATIC_COLOR16)
                return PaintStaticControls(hwndDlg, pConInfo, drawItem);
            else if (drawItem->CtlID == IDC_STATIC_SCREEN_COLOR)
                return PaintText(drawItem, pConInfo, Screen);
            else if (drawItem->CtlID == IDC_STATIC_POPUP_COLOR)
                return PaintText(drawItem, pConInfo, Popup);

            break;
        }

        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                {
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
                        lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 0), 255);
                        color = RGB(lpnmud->iPos, GetGValue(color), GetBValue(color));
                    }
                    else if (lpnmud->hdr.idFrom == IDC_UPDOWN_COLOR_GREEN)
                    {
                        lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 0), 255);
                        color = RGB(GetRValue(color), lpnmud->iPos, GetBValue(color));
                    }
                    else if (lpnmud->hdr.idFrom == IDC_UPDOWN_COLOR_BLUE)
                    {
                        lpnmud->iPos = min(max(lpnmud->iPos + lpnmud->iDelta, 0), 255);
                        color = RGB(GetRValue(color), GetGValue(color), lpnmud->iPos);
                    }
                    else
                    {
                        break;
                    }

                    pConInfo->ci.Colors[colorIndex] = color;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + colorIndex), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR) , NULL, TRUE);

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
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED  , GetRValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE , GetBValue(color), FALSE);

                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    pConInfo->ActiveStaticControl = colorIndex;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR) , NULL, TRUE);
                    break;
                }

                case IDC_RADIO_SCREEN_BACKGROUND:
                {
                    /* Get the color of the screen background */
                    colorIndex = BkgdAttribFromAttrib(pConInfo->ci.ScreenAttrib);
                    color = pConInfo->ci.Colors[colorIndex];

                    /* Set the values of the colour indicators */
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED  , GetRValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE , GetBValue(color), FALSE);

                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    pConInfo->ActiveStaticControl = colorIndex;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR) , NULL, TRUE);
                    break;
                }

                case IDC_RADIO_POPUP_TEXT:
                {
                    /* Get the color of the popup foreground */
                    colorIndex = TextAttribFromAttrib(pConInfo->ci.PopupAttrib);
                    color = pConInfo->ci.Colors[colorIndex];

                    /* Set the values of the colour indicators */
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED  , GetRValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE , GetBValue(color), FALSE);

                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    pConInfo->ActiveStaticControl = colorIndex;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR) , NULL, TRUE);
                    break;
                }

                case IDC_RADIO_POPUP_BACKGROUND:
                {
                    /* Get the color of the popup background */
                    colorIndex = BkgdAttribFromAttrib(pConInfo->ci.PopupAttrib);
                    color = pConInfo->ci.Colors[colorIndex];

                    /* Set the values of the colour indicators */
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED  , GetRValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                    SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE , GetBValue(color), FALSE);

                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    pConInfo->ActiveStaticControl = colorIndex;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR) , NULL, TRUE);
                    break;
                }

                case IDC_EDIT_COLOR_RED:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD red;

                        /* Get the current color */
                        colorIndex = pConInfo->ActiveStaticControl;
                        color = pConInfo->ci.Colors[colorIndex];

                        red = GetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, NULL, FALSE);
                        red = min(max(red, 0), 255);

                        color = RGB(red, GetGValue(color), GetBValue(color));

                        pConInfo->ci.Colors[colorIndex] = color;
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + colorIndex), NULL, TRUE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR) , NULL, TRUE);

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }

                case IDC_EDIT_COLOR_GREEN:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD green;

                        /* Get the current color */
                        colorIndex = pConInfo->ActiveStaticControl;
                        color = pConInfo->ci.Colors[colorIndex];

                        green = GetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, NULL, FALSE);
                        green = min(max(green, 0), 255);

                        color = RGB(GetRValue(color), green, GetBValue(color));

                        pConInfo->ci.Colors[colorIndex] = color;
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + colorIndex), NULL, TRUE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR) , NULL, TRUE);

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }

                case IDC_EDIT_COLOR_BLUE:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD blue;

                        /* Get the current color */
                        colorIndex = pConInfo->ActiveStaticControl;
                        color = pConInfo->ci.Colors[colorIndex];

                        blue = GetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, NULL, FALSE);
                        blue = min(max(blue, 0), 255);

                        color = RGB(GetRValue(color), GetGValue(color), blue);

                        pConInfo->ci.Colors[colorIndex] = color;
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + colorIndex), NULL, TRUE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR) , NULL, TRUE);

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
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

                SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED  , GetRValue(color), FALSE);
                SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(color), FALSE);
                SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE , GetBValue(color), FALSE);

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
                InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR) , NULL, TRUE);

                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
        }

        default:
            break;
    }

    return FALSE;
}
