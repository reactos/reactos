/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/layout.c
 * PURPOSE:         Layout dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>

const WCHAR szPreviewText[] =
    L"C:\\ReactOS> dir                       \n" \
    L"SYSTEM       <DIR>      13-04-15  5:00a\n" \
    L"SYSTEM32     <DIR>      13-04-15  5:00a\n" \
    L"readme   txt       1739 13-04-15  5:00a\n" \
    L"explorer exe    3329536 13-04-15  5:00a\n" \
    L"vgafonts cab      18736 13-04-15  5:00a\n" \
    L"setuplog txt        313 13-04-15  5:00a\n" \
    L"win      ini       7005 13-04-15  5:00a\n" ;


VOID
PaintConsole(LPDRAWITEMSTRUCT drawItem,
             PCONSOLE_PROPS pConInfo)
{
    PGUI_CONSOLE_INFO GuiInfo = pConInfo->TerminalInfo.TermInfo;
    HBRUSH hBrush;
    RECT cRect, fRect;
    DWORD startx, starty;
    DWORD endx, endy;
    DWORD sizex, sizey;

    FillRect(drawItem->hDC, &drawItem->rcItem, GetSysColorBrush(COLOR_BACKGROUND));

    // FIXME: Use: SM_CXSIZE, SM_CYSIZE, SM_CXVSCROLL, SM_CYHSCROLL, SM_CXMIN, SM_CYMIN, SM_CXFRAME, SM_CYFRAME
    /* Use it for scaling */
    sizex = drawItem->rcItem.right  - drawItem->rcItem.left;
    sizey = drawItem->rcItem.bottom - drawItem->rcItem.top ;

    if ( GuiInfo->WindowOrigin.x == MAXDWORD &&
         GuiInfo->WindowOrigin.y == MAXDWORD )
    {
        startx = sizex / 3;
        starty = sizey / 3;
    }
    else
    {
        // TODO:
        // Calculate pos correctly when console centered
        startx = GuiInfo->WindowOrigin.x;
        starty = GuiInfo->WindowOrigin.y;
    }

    // TODO:
    // Stretch console when bold fonts are selected
    endx = startx + pConInfo->ci.ConsoleSize.X; // drawItem->rcItem.right - startx + 15;
    endy = starty + pConInfo->ci.ConsoleSize.Y; // starty + sizey / 3;

    /* Draw console size */
    SetRect(&cRect, startx, starty, endx, endy);
    FillRect(drawItem->hDC, &cRect, GetSysColorBrush(COLOR_WINDOWFRAME));

    /* Draw console border */
    SetRect(&fRect, startx + 1, starty + 1, cRect.right - 1, cRect.bottom - 1);
    FrameRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVEBORDER));

    /* Draw left box */
    SetRect(&fRect, startx + 3, starty + 3, startx + 5, starty + 5);
    FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVEBORDER));

    /* Draw window title */
    SetRect(&fRect, startx + 7, starty + 3, cRect.right - 9, starty + 5);
    FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVECAPTION));

    /* Draw first right box */
    SetRect(&fRect, fRect.right + 1, starty + 3, fRect.right + 3, starty + 5);
    FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVEBORDER));

    /* Draw second right box */
    SetRect(&fRect, fRect.right + 1, starty + 3, fRect.right + 3, starty + 5);
    FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVEBORDER));

    /* Draw scrollbar */
    SetRect(&fRect, cRect.right - 5, fRect.bottom + 1, cRect.right - 3, cRect.bottom - 3);
    FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_SCROLLBAR));

    /* Draw console background */
    hBrush = CreateSolidBrush(pConInfo->ci.Colors[BkgdAttribFromAttrib(pConInfo->ci.ScreenAttrib)]);
    SetRect(&fRect, startx + 3, starty + 6, cRect.right - 6, cRect.bottom - 3);
    FillRect(drawItem->hDC, &fRect, hBrush);
    DeleteObject((HGDIOBJ)hBrush);
}

BOOL
PaintText(LPDRAWITEMSTRUCT drawItem,
          PCONSOLE_PROPS pConInfo,
          TEXT_TYPE TextMode)
{
    PGUI_CONSOLE_INFO GuiInfo = pConInfo->TerminalInfo.TermInfo;
    USHORT CurrentAttrib;
    COLORREF pbkColor, ptColor;
    COLORREF nbkColor, ntColor;
    HBRUSH hBrush;
    HFONT Font, OldFont;

    COORD FontSize = GuiInfo->FontSize;

    if (TextMode == Screen)
        CurrentAttrib = pConInfo->ci.ScreenAttrib;
    else if (TextMode == Popup)
        CurrentAttrib = pConInfo->ci.PopupAttrib;
    else
        return FALSE;

    nbkColor = pConInfo->ci.Colors[BkgdAttribFromAttrib(CurrentAttrib)];
    ntColor  = pConInfo->ci.Colors[TextAttribFromAttrib(CurrentAttrib)];

    hBrush = CreateSolidBrush(nbkColor);
    if (!hBrush) return FALSE;

    FontSize.Y = FontSize.Y > 0 ? -MulDiv(FontSize.Y, GetDeviceCaps(drawItem->hDC, LOGPIXELSY), 72)
                                : FontSize.Y;

    Font = CreateFontW(FontSize.Y,
                       FontSize.X,
                       0,
                       TA_BASELINE,
                       GuiInfo->FontWeight,
                       FALSE,
                       FALSE,
                       FALSE,
                       OEM_CHARSET,
                       OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY,
                       FIXED_PITCH | GuiInfo->FontFamily,
                       GuiInfo->FaceName);
    if (Font == NULL)
    {
        DPRINT1("PaintText: CreateFont failed\n");
        return FALSE;
    }

    OldFont = SelectObject(drawItem->hDC, Font);
    if (OldFont == NULL)
    {
        DeleteObject(Font);
        return FALSE;
    }

    FillRect(drawItem->hDC, &drawItem->rcItem, hBrush);

    ptColor = SetTextColor(drawItem->hDC, ntColor);
    pbkColor = SetBkColor(drawItem->hDC, nbkColor);
    DrawTextW(drawItem->hDC, szPreviewText, wcslen(szPreviewText), &drawItem->rcItem, 0);
    SetTextColor(drawItem->hDC, ptColor);
    SetBkColor(drawItem->hDC, pbkColor);
    DeleteObject(hBrush);

    SelectObject(drawItem->hDC, OldFont);
    DeleteObject(Font);

    return TRUE;
}

INT_PTR
CALLBACK
LayoutProc(HWND hwndDlg,
           UINT uMsg,
           WPARAM wParam,
           LPARAM lParam)
{
    PCONSOLE_PROPS pConInfo = (PCONSOLE_PROPS)GetWindowLongPtr(hwndDlg, DWLP_USER);
    PGUI_CONSOLE_INFO GuiInfo = (pConInfo ? pConInfo->TerminalInfo.TermInfo : NULL);

    UNREFERENCED_PARAMETER(hwndDlg);
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Multi-monitor support */
            LONG  xVirtScr,  yVirtScr; // Coordinates of the top-left virtual screen
            LONG cxVirtScr, cyVirtScr; // Width and Height of the virtual screen
            LONG cxFrame  , cyFrame  ; // Thickness of the window frame

            pConInfo = (PCONSOLE_PROPS)((LPPROPSHEETPAGE)lParam)->lParam;
            GuiInfo  = pConInfo->TerminalInfo.TermInfo;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pConInfo);

            /* Multi-monitor support */
            xVirtScr  = GetSystemMetrics(SM_XVIRTUALSCREEN);
            yVirtScr  = GetSystemMetrics(SM_YVIRTUALSCREEN);
            cxVirtScr = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            cyVirtScr = GetSystemMetrics(SM_CYVIRTUALSCREEN);
            cxFrame   = GetSystemMetrics(SM_CXFRAME);
            cyFrame   = GetSystemMetrics(SM_CYFRAME);

            SendDlgItemMessageW(hwndDlg, IDC_UPDOWN_SCREEN_BUFFER_HEIGHT, UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));
            SendDlgItemMessageW(hwndDlg, IDC_UPDOWN_SCREEN_BUFFER_WIDTH , UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));
            SendDlgItemMessageW(hwndDlg, IDC_UPDOWN_WINDOW_SIZE_HEIGHT, UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));
            SendDlgItemMessageW(hwndDlg, IDC_UPDOWN_WINDOW_SIZE_WIDTH , UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));

            SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, pConInfo->ci.ScreenBufferSize.Y, FALSE);
            SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH , pConInfo->ci.ScreenBufferSize.X, FALSE);
            SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, pConInfo->ci.ConsoleSize.Y, FALSE);
            SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_WIDTH , pConInfo->ci.ConsoleSize.X, FALSE);

            SendDlgItemMessageW(hwndDlg, IDC_UPDOWN_WINDOW_POS_LEFT, UDM_SETRANGE, 0,
                                (LPARAM)MAKELONG(xVirtScr + cxVirtScr - cxFrame, xVirtScr - cxFrame));
            SendDlgItemMessageW(hwndDlg, IDC_UPDOWN_WINDOW_POS_TOP , UDM_SETRANGE, 0,
                                (LPARAM)MAKELONG(yVirtScr + cyVirtScr - cyFrame, yVirtScr - cyFrame));

            SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, GuiInfo->WindowOrigin.x, TRUE);
            SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_TOP , GuiInfo->WindowOrigin.y, TRUE);

            if (GuiInfo->AutoPosition)
            {
                EnableDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, FALSE);
                EnableDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_TOP , FALSE);
                EnableDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_LEFT, FALSE);
                EnableDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_TOP , FALSE);
            }
            CheckDlgButton(hwndDlg, IDC_CHECK_SYSTEM_POS_WINDOW,
                           GuiInfo->AutoPosition ? BST_CHECKED : BST_UNCHECKED);

            return TRUE;
        }

        case WM_DRAWITEM:
        {
            PaintConsole((LPDRAWITEMSTRUCT)lParam, pConInfo);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMUPDOWN  lpnmud =  (LPNMUPDOWN)lParam;
            LPPSHNOTIFY lppsn  = (LPPSHNOTIFY)lParam;

            if (lppsn->hdr.code == UDN_DELTAPOS)
            {
                DWORD wheight, wwidth;
                DWORD sheight, swidth;
                DWORD left, top;

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_WIDTH)
                {
                    wwidth = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    wwidth = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_WIDTH, NULL, FALSE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_HEIGHT)
                {
                    wheight = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    wheight = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, NULL, FALSE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_WIDTH)
                {
                    swidth = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    swidth = GetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, NULL, FALSE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_HEIGHT)
                {
                    sheight = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    sheight = GetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, NULL, FALSE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_POS_LEFT)
                {
                    left = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    left = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, NULL, TRUE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_POS_TOP)
                {
                    top = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    top = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_TOP, NULL, TRUE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_WIDTH || lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_HEIGHT)
                {
                    /* Automatically adjust screen buffer size when window size enlarges */
                    if (wwidth >= swidth)
                    {
                        SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, wwidth, TRUE);
                        swidth = wwidth;
                    }
                    if (wheight >= sheight)
                    {
                        SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, wheight, TRUE);
                        sheight = wheight;
                    }
                }

                /* Be sure that the (new) screen buffer sizes are in the correct range */
                swidth  = min(max(swidth , 1), 0xFFFF);
                sheight = min(max(sheight, 1), 0xFFFF);

                if (lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_WIDTH || lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_HEIGHT)
                {
                    /* Automatically adjust window size when screen buffer decreases */
                    if (wwidth > swidth)
                    {
                        SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_WIDTH, swidth, TRUE);
                        wwidth = swidth;
                    }
                    if (wheight > sheight)
                    {
                        SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, sheight, TRUE);
                        wheight = sheight;
                    }
                }

                pConInfo->ci.ScreenBufferSize.X = (SHORT)swidth;
                pConInfo->ci.ScreenBufferSize.Y = (SHORT)sheight;
                pConInfo->ci.ConsoleSize.X = (SHORT)wwidth;
                pConInfo->ci.ConsoleSize.Y = (SHORT)wheight;
                GuiInfo->WindowOrigin.x = left;
                GuiInfo->WindowOrigin.y = top;
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_EDIT_SCREEN_BUFFER_WIDTH:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD swidth, wwidth;

                        swidth = GetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, NULL, FALSE);
                        wwidth = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_WIDTH  , NULL, FALSE);

                        /* Be sure that the (new) screen buffer width is in the correct range */
                        swidth = min(max(swidth, 1), 0xFFFF);

                        /* Automatically adjust window size when screen buffer decreases */
                        if (wwidth > swidth)
                        {
                            wwidth = swidth;
                            SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_WIDTH, wwidth, TRUE);
                        }

                        pConInfo->ci.ScreenBufferSize.X = (SHORT)swidth;
                        pConInfo->ci.ConsoleSize.X      = (SHORT)wwidth;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }

                case IDC_EDIT_WINDOW_SIZE_WIDTH:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD swidth, wwidth;

                        swidth = GetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, NULL, FALSE);
                        wwidth = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_WIDTH  , NULL, FALSE);

                        /* Automatically adjust screen buffer size when window size enlarges */
                        if (wwidth >= swidth)
                        {
                            swidth = wwidth;

                            /* Be sure that the (new) screen buffer width is in the correct range */
                            swidth = min(max(swidth, 1), 0xFFFF);

                            SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, swidth, TRUE);
                        }

                        pConInfo->ci.ScreenBufferSize.X = (SHORT)swidth;
                        pConInfo->ci.ConsoleSize.X      = (SHORT)wwidth;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }

                case IDC_EDIT_SCREEN_BUFFER_HEIGHT:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD sheight, wheight;

                        sheight = GetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, NULL, FALSE);
                        wheight = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT  , NULL, FALSE);

                        /* Be sure that the (new) screen buffer width is in the correct range */
                        sheight = min(max(sheight, 1), 0xFFFF);

                        /* Automatically adjust window size when screen buffer decreases */
                        if (wheight > sheight)
                        {
                            wheight = sheight;
                            SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, wheight, TRUE);
                        }

                        pConInfo->ci.ScreenBufferSize.Y = (SHORT)sheight;
                        pConInfo->ci.ConsoleSize.Y      = (SHORT)wheight;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }

                case IDC_EDIT_WINDOW_SIZE_HEIGHT:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD sheight, wheight;

                        sheight = GetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, NULL, FALSE);
                        wheight = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT  , NULL, FALSE);

                        /* Automatically adjust screen buffer size when window size enlarges */
                        if (wheight >= sheight)
                        {
                            sheight = wheight;

                            /* Be sure that the (new) screen buffer width is in the correct range */
                            sheight = min(max(sheight, 1), 0xFFFF);

                            SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, sheight, TRUE);
                        }

                        pConInfo->ci.ScreenBufferSize.Y = (SHORT)sheight;
                        pConInfo->ci.ConsoleSize.Y      = (SHORT)wheight;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }

                case IDC_EDIT_WINDOW_POS_LEFT:
                case IDC_EDIT_WINDOW_POS_TOP:
                {
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        DWORD left, top;

                        left = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, NULL, TRUE);
                        top  = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_TOP , NULL, TRUE);

                        GuiInfo->WindowOrigin.x = left;
                        GuiInfo->WindowOrigin.y = top;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                }

                case IDC_CHECK_SYSTEM_POS_WINDOW:
                {
                    LONG res = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                    if (res == BST_CHECKED)
                    {
                        ULONG left, top;

                        left = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, NULL, TRUE);
                        top  = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_TOP , NULL, TRUE);

                        GuiInfo->AutoPosition   = FALSE;
                        GuiInfo->WindowOrigin.x = left;
                        GuiInfo->WindowOrigin.y = top;
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
                        EnableDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, TRUE);
                        EnableDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_TOP , TRUE);
                        EnableDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_LEFT, TRUE);
                        EnableDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_TOP , TRUE);
                    }
                    else if (res == BST_UNCHECKED)
                    {
                        GuiInfo->AutoPosition = TRUE;
                        // Do not touch GuiInfo->WindowOrigin !!
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
                        EnableDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, FALSE);
                        EnableDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_TOP , FALSE);
                        EnableDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_LEFT, FALSE);
                        EnableDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_TOP , FALSE);
                    }
                }
            }
        }

        default:
            break;
    }

    return FALSE;
}
