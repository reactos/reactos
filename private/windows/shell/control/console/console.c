/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    menu.c

Abstract:

        This file implements the system menu management.

Author:

    Therese Stowell (thereses) Jan-24-1992 (swiped from Win3.1)

--*/

#include "precomp.h"
#pragma hdrstop


BOOL InEM_UNDO=FALSE;

BYTE ColorArray[4];
int Index;

BOOL gbSaveToRegistry;
BOOL gbWriteToConsole;
BOOL gbStartedFromLink;
LONG gcxScreen;
LONG gcyScreen;
UINT gnCurrentPage;

PCONSOLE_STATE_INFO gpStateInfo;


/*
 *  Context Help Ids.
 */
CONST DWORD gaConsoleHelpIds[] =
{
    IDD_WINDOWED,               IDH_DOS_SCREEN_USAGE_WINDOW,
    IDD_FULLSCREEN,             IDH_DOS_SCREEN_USAGE_FULL,
    IDD_DISPLAY_GROUPBOX,       -1,
    IDD_QUICKEDIT,              IDH_CONSOLE_OPTIONS_QUICK_EDIT,
    IDD_INSERT,                 IDH_CONSOLE_OPTIONS_INSERT,
    IDD_CURSOR_SMALL,           IDH_CONSOLE_OPTIONS_CURSOR,
    IDD_CURSOR_MEDIUM,          IDH_CONSOLE_OPTIONS_CURSOR,
    IDD_CURSOR_LARGE,           IDH_CONSOLE_OPTIONS_CURSOR,
    IDD_HISTORY_SIZE,           IDH_CONSOLE_OPTIONS_BUFF_SIZE,
    IDD_HISTORY_SIZESCROLL,     IDH_CONSOLE_OPTIONS_BUFF_SIZE,
    IDD_HISTORY_NUM,            IDH_CONSOLE_OPTIONS_BUFF_NUM,
    IDD_HISTORY_NUMSCROLL,      IDH_CONSOLE_OPTIONS_BUFF_NUM,
    IDD_HISTORY_NODUP,          IDH_CONSOLE_OPTIONS_DISCARD_DUPS,
#if defined(FE_SB)
    IDD_LANGUAGELIST,           IDH_CONSOLE_OPTIONS_LANGUAGE,
    IDD_LANGUAGE,               IDH_CONSOLE_OPTIONS_LANGUAGE,
#endif
    IDD_STATIC,                 IDH_CONSOLE_FONT_FONT,
    IDD_FACENAME,               IDH_CONSOLE_FONT_FONT,
    IDD_BOLDFONT,               IDH_CONSOLE_FONT_BOLD_FONTS,
    IDD_PREVIEWLABEL,           IDH_DOS_FONT_WINDOW_PREVIEW,
    IDD_PREVIEWWINDOW,          IDH_DOS_FONT_WINDOW_PREVIEW,
    IDD_GROUP,                  IDH_DOS_FONT_FONT_PREVIEW,
    IDD_STATIC2,                IDH_DOS_FONT_FONT_PREVIEW,
    IDD_STATIC3,                IDH_DOS_FONT_FONT_PREVIEW,
    IDD_STATIC4,                IDH_DOS_FONT_FONT_PREVIEW,
    IDD_FONTWIDTH,              IDH_DOS_FONT_FONT_PREVIEW,
    IDD_FONTHEIGHT,             IDH_DOS_FONT_FONT_PREVIEW,
    IDD_FONTWINDOW,             IDH_DOS_FONT_FONT_PREVIEW,
    IDD_FONTSIZE,               IDH_DOS_FONT_SIZE,
    IDD_POINTSLIST,             IDH_DOS_FONT_SIZE,
    IDD_PIXELSLIST,             IDH_DOS_FONT_SIZE,
    IDD_SCRBUF_WIDTH,           IDH_CONSOLE_SIZE_BUFF_WIDTH,
    IDD_SCRBUF_WIDTHSCROLL,     IDH_CONSOLE_SIZE_BUFF_WIDTH,
    IDD_SCRBUF_HEIGHT,          IDH_CONSOLE_SIZE_BUFF_HEIGHT,
    IDD_SCRBUF_HEIGHTSCROLL,    IDH_CONSOLE_SIZE_BUFF_HEIGHT,
    IDD_WINDOW_WIDTH,           IDH_CONSOLE_SIZE_WIN_WIDTH,
    IDD_WINDOW_WIDTHSCROLL,     IDH_CONSOLE_SIZE_WIN_WIDTH,
    IDD_WINDOW_HEIGHT,          IDH_CONSOLE_SIZE_WIN_HEIGHT,
    IDD_WINDOW_HEIGHTSCROLL,    IDH_CONSOLE_SIZE_WIN_HEIGHT,
    IDD_WINDOW_POSX,            IDH_CONSOLE_SIZE_POS_LEFT,
    IDD_WINDOW_POSXSCROLL,      IDH_CONSOLE_SIZE_POS_LEFT,
    IDD_WINDOW_POSY,            IDH_CONSOLE_SIZE_POS_TOP,
    IDD_WINDOW_POSYSCROLL,      IDH_CONSOLE_SIZE_POS_TOP,
    IDD_AUTO_POSITION,          IDH_CONSOLE_SIZE_LET_SYS,
    IDD_COLOR_SCREEN_TEXT,      IDH_CONSOLE_COLOR_SCR_TEXT,
    IDD_COLOR_SCREEN_BKGND,     IDH_CONSOLE_COLOR_SCR_BACK,
    IDD_COLOR_POPUP_TEXT,       IDH_CONSOLE_COLOR_POPUP_TEXT,
    IDD_COLOR_POPUP_BKGND,      IDH_CONSOLE_COLOR_POPUP_BACK,
    IDD_COLOR_1,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_2,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_3,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_4,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_5,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_6,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_7,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_8,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_9,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_10,               IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_11,               IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_12,               IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_13,               IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_14,               IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_15,               IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_16,               IDH_CONSOLE_COLOR_COLOR_BAR,
    IDD_COLOR_SCREEN_COLORS,    IDH_CONSOLE_COLOR_SCR_COLORS,
    IDD_COLOR_POPUP_COLORS,     IDH_CONSOLE_COLOR_WIN_COLORS,
    IDD_COLOR_RED,              IDH_CONSOLE_COLOR_RED,
    IDD_COLOR_REDSCROLL,        IDH_CONSOLE_COLOR_RED,
    IDD_COLOR_GREEN,            IDH_CONSOLE_COLOR_RED,
    IDD_COLOR_GREENSCROLL,      IDH_CONSOLE_COLOR_RED,
    IDD_COLOR_BLUE,             IDH_CONSOLE_COLOR_RED,
    IDD_COLOR_BLUESCROLL,       IDH_CONSOLE_COLOR_RED,
    0,                          0
};

VOID
UpdateItem(
    HWND hDlg,
    UINT item,
    UINT nNum
    )
{
    SetDlgItemInt(hDlg, item, nNum, TRUE);
    SendDlgItemMessage(hDlg, item, EM_SETSEL, 0, (DWORD)-1);
}


BOOL
CheckNum(
    HWND hDlg,
    UINT Item)
{
    int i;
    TCHAR szNum[5];
    BOOL fSigned;

    if (Item == IDD_WINDOW_POSX || Item == IDD_WINDOW_POSY) {
        fSigned = TRUE;
    } else {
        fSigned = FALSE;
    }

    GetDlgItemText(hDlg, Item, szNum, NELEM(szNum));
    for (i = 0; szNum[i]; i++) {
        if (!iswdigit(szNum[i]) && (!fSigned || i > 0 || szNum[i] != TEXT('-')))
            return FALSE;
    }
    return TRUE;
}


INT_PTR
CommonDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (wMsg) {
    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)gaConsoleHelpIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)gaConsoleHelpIds);
        break;

    default:
        break;
    }
    return FALSE;
}


INT_PTR
WINAPI
SaveQueryDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HFONT hFont;
    HWND  hChild;
    HWND  hCreator;

    switch (wMsg) {
    case WM_INITDIALOG:
        /*
         * Save the handle of the window that created us
         */
        hCreator = (HWND)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

        /*
         * Get the font used in other controls
         */
        hChild = GetWindow(hCreator, GW_CHILD);
        hFont = GetWindowFont(hChild);

        /*
         * Now apply it to our controls
         */
        hChild = GetWindow(hDlg, GW_CHILD);
        while (hChild != NULL) {
            SetWindowFont(hChild, hFont, TRUE);
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }

        CheckRadioButton(hDlg, IDD_APPLY, IDD_SAVE, IDD_APPLY);
        gbSaveToRegistry = FALSE;
        gbWriteToConsole = FALSE;
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if (IsDlgButtonChecked(hDlg, IDD_SAVE)) {
                gbSaveToRegistry = TRUE;
            }
            gbWriteToConsole = TRUE;
            EndDialog(hDlg, PSNRET_NOERROR);
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg, PSNRET_INVALID_NOCHANGEPAGE);
            return TRUE;
        }
        break;

    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        /*
         * Let the window who created us decide what colors to use
         */
        hCreator = (HWND)GetWindowLongPtr(hDlg, GWLP_USERDATA);
        return SendMessage(hCreator, wMsg, wParam, lParam);
    }
    return FALSE;
}


UINT
ConsolePropSheetProc(
    HWND hDlg,
    UINT wMsg,
    LPARAM lParam
    )
{
    DWORD dwExStyle;

    switch (wMsg) {
    case PSCB_INITIALIZED:
        /*
         * If we're connected to the server, tell him we're starting
         */
        if (gpStateInfo->hWnd != NULL) {
            SendMessage(gpStateInfo->hWnd, CM_PROPERTIES_START, (WPARAM)hDlg, 0);
        }
        break;
    default:
        break;
    }

    return 0;
}


VOID
EndDlgPage(
    HWND hDlg
    )
{
    HWND hParent;
    HWND hTabCtrl;
    INT_PTR Result;

    /*
     * If we've already made a decision, we're done
     */
    if (gbWriteToConsole || gbSaveToRegistry) {
        SetDlgMsgResult(hDlg, PSN_APPLY, PSNRET_NOERROR);
        return;
    }

    /*
     * Get the current page number
     */
    hParent = GetParent(hDlg);
    hTabCtrl = PropSheet_GetTabControl(hParent);
    gnCurrentPage = TabCtrl_GetCurSel(hTabCtrl);

    /*
     * If we're not connected to the server, we're done
     */
    if (gpStateInfo->hWnd == NULL) {
        gbSaveToRegistry = TRUE;
        SetDlgMsgResult(hDlg, PSN_APPLY, PSNRET_NOERROR);
        return;
    }

    /*
     * Check to show the Apply/Save dialog box
     */
    if (gbStartedFromLink)
    {
        Result = DialogBoxParam(ghInstance, MAKEINTRESOURCE(DID_SAVE_QUERY_LINK),
                                GetParent(hDlg), SaveQueryDlgProc, (LPARAM)hDlg);

        SetDlgMsgResult(hDlg, PSN_APPLY, Result);
    }
    else
    {
        Result = DialogBoxParam(ghInstance, MAKEINTRESOURCE(DID_SAVE_QUERY),
                                GetParent(hDlg), SaveQueryDlgProc, (LPARAM)hDlg);

        SetDlgMsgResult(hDlg, PSN_APPLY, Result);
    }
    return;
}


LRESULT
ColorControlProc(
    HWND hColor,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Window proc for the color buttons

--*/

{
    PAINTSTRUCT ps;
    int ColorId;
    RECT rColor;
    RECT rTemp;
    HBRUSH hbr;
    HDC hdc;
    HWND hWnd;
    HWND hDlg;
    COLORREF rgbBrush;

    ColorId = GetWindowLong(hColor, GWL_ID);
    hDlg = GetParent(hColor);

    switch (wMsg) {
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTTAB;
        break;
    case WM_SETFOCUS:
        if (ColorArray[Index] != (BYTE)(ColorId - IDD_COLOR_1)) {
            hWnd = GetDlgItem(hDlg, ColorArray[Index]+IDD_COLOR_1);
            SetFocus(hWnd);
        }
        // Fall through
    case WM_KILLFOCUS:
        hdc = GetDC(hDlg);
        hWnd = GetDlgItem(hDlg, IDD_COLOR_1);
        GetWindowRect(hWnd, &rColor);
        hWnd = GetDlgItem(hDlg, IDD_COLOR_16);
        GetWindowRect(hWnd, &rTemp);
        rColor.right = rTemp.right;
        ScreenToClient(hDlg, (LPPOINT)&rColor.left);
        ScreenToClient(hDlg, (LPPOINT)&rColor.right);
        InflateRect(&rColor, 2, 2);
        DrawFocusRect(hdc, &rColor);
        ReleaseDC(hDlg, hdc);
        break;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_UP:
        case VK_LEFT:
            if (ColorId > IDD_COLOR_1) {
                SendMessage(hDlg, CM_SETCOLOR,
                            ColorId - 1 - IDD_COLOR_1, (LPARAM)hColor);
            }
            break;
        case VK_DOWN:
        case VK_RIGHT:
            if (ColorId < IDD_COLOR_16) {
                SendMessage(hDlg, CM_SETCOLOR,
                            ColorId + 1 - IDD_COLOR_1, (LPARAM)hColor);
            }
            break;
        case VK_TAB:
            hWnd = GetDlgItem(hDlg, IDD_COLOR_1);
            hWnd = GetNextDlgTabItem(hDlg, hWnd, GetKeyState(VK_SHIFT) < 0);
            SetFocus(hWnd);
            break;
        default:
            return DefWindowProc(hColor, wMsg, wParam, lParam);
        }
        break;
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
        SendMessage(hDlg, CM_SETCOLOR,
                    ColorId - IDD_COLOR_1, (LPARAM)hColor);
        break;
    case WM_PAINT:
        BeginPaint(hColor, &ps);
        GetClientRect(hColor, &rColor);
        rgbBrush = GetNearestColor(ps.hdc, AttrToRGB(ColorId-IDD_COLOR_1));
        if ((hbr = CreateSolidBrush(rgbBrush)) != NULL) {
            //
            // are we the selected color for the current object?
            //
            if (ColorArray[Index] == (BYTE)(ColorId - IDD_COLOR_1)) {

                //
                // put current values in dialog box
                //
                UpdateItem(hDlg, IDD_COLOR_RED,
                           GetRValue(AttrToRGB(ColorArray[Index])));
                UpdateItem(hDlg, IDD_COLOR_GREEN,
                           GetGValue(AttrToRGB(ColorArray[Index])));
                UpdateItem(hDlg, IDD_COLOR_BLUE,
                           GetBValue(AttrToRGB(ColorArray[Index])));

                //
                // highlight the selected color
                //
                FrameRect(ps.hdc, &rColor, GetStockObject(BLACK_BRUSH));
                InflateRect(&rColor, -1, -1);
                FrameRect(ps.hdc, &rColor, GetStockObject(BLACK_BRUSH));
            }
            InflateRect(&rColor, -1, -1);
            FillRect(ps.hdc, &rColor, hbr);
            DeleteObject(hbr);
        }
        EndPaint(hColor, &ps);
        break;
    default:
        return DefWindowProc(hColor, wMsg, wParam, lParam);
        break;
    }
    return TRUE;
}


INT_PTR
WINAPI
ColorDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the color selection dialog box.

--*/

{
    UINT Value;
    UINT Red;
    UINT Green;
    UINT Blue;
    UINT Item;
    HWND hWnd;
    HWND hWndOld;
    BOOL bOK;

    switch (wMsg) {
    case WM_INITDIALOG:
        ColorArray[IDD_COLOR_SCREEN_TEXT - IDD_COLOR_SCREEN_TEXT] =
                LOBYTE(gpStateInfo->ScreenAttributes) & 0x0F;
        ColorArray[IDD_COLOR_SCREEN_BKGND - IDD_COLOR_SCREEN_TEXT] =
                LOBYTE(gpStateInfo->ScreenAttributes >> 4);
        ColorArray[IDD_COLOR_POPUP_TEXT - IDD_COLOR_SCREEN_TEXT] =
                LOBYTE(gpStateInfo->PopupAttributes) & 0x0F;
        ColorArray[IDD_COLOR_POPUP_BKGND - IDD_COLOR_SCREEN_TEXT] =
                LOBYTE(gpStateInfo->PopupAttributes >> 4);
        CheckRadioButton(hDlg,IDD_COLOR_SCREEN_TEXT,IDD_COLOR_POPUP_BKGND,IDD_COLOR_SCREEN_BKGND);
        Index = IDD_COLOR_SCREEN_BKGND - IDD_COLOR_SCREEN_TEXT;

        // initialize size of edit controls

        SendDlgItemMessage(hDlg, IDD_COLOR_RED, EM_LIMITTEXT, 3, 0L);
        SendDlgItemMessage(hDlg, IDD_COLOR_GREEN, EM_LIMITTEXT, 3, 0L);
        SendDlgItemMessage(hDlg, IDD_COLOR_BLUE, EM_LIMITTEXT, 3, 0L);

        // initialize arrow controls

        SendDlgItemMessage(hDlg, IDD_COLOR_REDSCROLL, UDM_SETRANGE, 0,
                           MAKELONG(255, 0));
        SendDlgItemMessage(hDlg, IDD_COLOR_REDSCROLL, UDM_SETPOS, 0,
                           MAKELONG(GetRValue(AttrToRGB(ColorArray[Index])), 0));
        SendDlgItemMessage(hDlg, IDD_COLOR_GREENSCROLL, UDM_SETRANGE, 0,
                           MAKELONG(255, 0));
        SendDlgItemMessage(hDlg, IDD_COLOR_GREENSCROLL, UDM_SETPOS, 0,
                           MAKELONG(GetGValue(AttrToRGB(ColorArray[Index])), 0));
        SendDlgItemMessage(hDlg, IDD_COLOR_BLUESCROLL, UDM_SETRANGE, 0,
                           MAKELONG(255, 0));
        SendDlgItemMessage(hDlg, IDD_COLOR_BLUESCROLL, UDM_SETPOS, 0,
                           MAKELONG(GetBValue(AttrToRGB(ColorArray[Index])), 0));

        return TRUE;

    case WM_COMMAND:
        Item = LOWORD(wParam);
        switch (Item) {
        case IDD_COLOR_SCREEN_TEXT:
        case IDD_COLOR_SCREEN_BKGND:
        case IDD_COLOR_POPUP_TEXT:
        case IDD_COLOR_POPUP_BKGND:
            hWndOld = GetDlgItem(hDlg, ColorArray[Index]+IDD_COLOR_1);

            Index = Item - IDD_COLOR_SCREEN_TEXT;

            // repaint new color
            hWnd = GetDlgItem(hDlg, ColorArray[Index]+IDD_COLOR_1);
            InvalidateRect(hWnd, NULL, TRUE);

            // repaint old color
            if (hWndOld != hWnd) {
                InvalidateRect(hWndOld, NULL, TRUE);
            }

            return TRUE;

        case IDD_COLOR_RED:
        case IDD_COLOR_GREEN:
        case IDD_COLOR_BLUE:
            switch (HIWORD(wParam)) {
            case EN_UPDATE:
                if (!CheckNum (hDlg, Item)) {
                    if (!InEM_UNDO) {
                        InEM_UNDO = TRUE;
                        SendMessage((HWND)lParam, EM_UNDO, 0, 0L);
                        InEM_UNDO = FALSE;
                    }
                }
                break;
            case EN_KILLFOCUS:
                /*
                 * Update the state info structure
                 */
                Value = GetDlgItemInt(hDlg, Item, &bOK, TRUE);
                if (bOK) {
                    if (Value > 255) {
                        UpdateItem(hDlg, Item, 255);
                        Value = 255;
                    }
                    if (Item == IDD_COLOR_RED) {
                        Red = Value;
                    } else {
                        Red = GetRValue(AttrToRGB(ColorArray[Index]));
                    }
                    if (Item == IDD_COLOR_GREEN) {
                        Green = Value;
                    } else {
                        Green = GetGValue(AttrToRGB(ColorArray[Index]));
                    }
                    if (Item == IDD_COLOR_BLUE) {
                        Blue = Value;
                    } else {
                        Blue = GetBValue(AttrToRGB(ColorArray[Index]));
                    }
                    UpdateStateInfo(hDlg, ColorArray[Index] + IDD_COLOR_1,
                                    RGB(Red, Green, Blue));
                }

                /*
                 * Update the preview windows with the new value
                 */
                hWnd = GetDlgItem(hDlg, IDD_COLOR_SCREEN_COLORS);
                InvalidateRect(hWnd, NULL, FALSE);
                hWnd = GetDlgItem(hDlg, IDD_COLOR_POPUP_COLORS);
                InvalidateRect(hWnd, NULL, FALSE);
                hWnd = GetDlgItem(hDlg, ColorArray[Index]+IDD_COLOR_1);
                InvalidateRect(hWnd, NULL, FALSE);
                break;
            }
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case PSN_APPLY:
            /*
             * Write out the state values and exit.
             */
            EndDlgPage(hDlg);
            return TRUE;

        case PSN_KILLACTIVE:
            /*
             * Fake the dialog proc into thinking the edit control just
             * lost focus so it'll update properly
             */
            if (Item = GetDlgCtrlID(GetFocus())) {
                SendMessage(hDlg, WM_COMMAND, MAKELONG(Item, EN_KILLFOCUS), 0);
            }
            return TRUE;
        }
        break;

    case WM_VSCROLL:
        /*
         * Fake the dialog proc into thinking the edit control just
         * lost focus so it'll update properly
         */
        Item = GetDlgCtrlID((HWND)lParam) - 1;
        SendMessage(hDlg, WM_COMMAND, MAKELONG(Item, EN_KILLFOCUS), 0);
        return TRUE;

    case CM_SETCOLOR:
        UpdateStateInfo(hDlg, Index + IDD_COLOR_SCREEN_TEXT, (UINT)wParam);

        hWndOld = GetDlgItem(hDlg, ColorArray[Index]+IDD_COLOR_1);

        ColorArray[Index] = (BYTE)wParam;

        /* Force the preview window to repaint */

        if (Index < (IDD_COLOR_POPUP_TEXT - IDD_COLOR_SCREEN_TEXT)) {
            hWnd = GetDlgItem(hDlg, IDD_COLOR_SCREEN_COLORS);
        } else {
            hWnd = GetDlgItem(hDlg, IDD_COLOR_POPUP_COLORS);
        }
        InvalidateRect(hWnd, NULL, TRUE);

        // repaint new color
        hWnd = GetDlgItem(hDlg, ColorArray[Index]+IDD_COLOR_1);
        InvalidateRect(hWnd, NULL, TRUE);
        SetFocus(hWnd);

        // repaint old color
        if (hWndOld != hWnd) {
            InvalidateRect(hWndOld, NULL, TRUE);
        }
        return TRUE;

    default:
        break;
    }
    return CommonDlgProc(hDlg, wMsg, wParam, lParam);
}


int
GetStateInfo(
    HWND hDlg,
    UINT Item,
    BOOL *bOK
    )
{
    int Value = 0;

    *bOK = TRUE;
    switch (Item) {
    case IDD_SCRBUF_WIDTH:
        Value = gpStateInfo->ScreenBufferSize.X;
        break;
    case IDD_SCRBUF_HEIGHT:
        Value = gpStateInfo->ScreenBufferSize.Y;
        break;
    case IDD_WINDOW_WIDTH:
        Value = gpStateInfo->WindowSize.X;
        break;
    case IDD_WINDOW_HEIGHT:
        Value = gpStateInfo->WindowSize.Y;
        break;
    case IDD_WINDOW_POSX:
        Value = gpStateInfo->WindowPosX;
        break;
    case IDD_WINDOW_POSY:
        Value = gpStateInfo->WindowPosY;
        break;
    default:
        *bOK = FALSE;
        break;
    }
    return Value;
}


BOOL
UpdateStateInfo(
    HWND hDlg,
    UINT Item,
    int Value
    )
{
    switch (Item) {
    case IDD_SCRBUF_WIDTH:
        gpStateInfo->ScreenBufferSize.X = (SHORT)Value;
        if (gpStateInfo->WindowSize.X > Value) {
            gpStateInfo->WindowSize.X = (SHORT)Value;
            UpdateItem(hDlg, IDD_WINDOW_WIDTH, Value);
        }
        break;
    case IDD_SCRBUF_HEIGHT:
        gpStateInfo->ScreenBufferSize.Y = (SHORT)Value;
        if (gpStateInfo->WindowSize.Y > Value) {
            gpStateInfo->WindowSize.Y = (SHORT)Value;
            UpdateItem(hDlg, IDD_WINDOW_HEIGHT, Value);
        }
        break;
    case IDD_WINDOW_WIDTH:
        gpStateInfo->WindowSize.X = (SHORT)Value;
        if (gpStateInfo->ScreenBufferSize.X < Value) {
            gpStateInfo->ScreenBufferSize.X = (SHORT)Value;
            UpdateItem(hDlg, IDD_SCRBUF_WIDTH, Value);
        }
        break;
    case IDD_WINDOW_HEIGHT:
        gpStateInfo->WindowSize.Y = (SHORT)Value;
        if (gpStateInfo->ScreenBufferSize.Y < Value) {
            gpStateInfo->ScreenBufferSize.Y = (SHORT)Value;
            UpdateItem(hDlg, IDD_SCRBUF_HEIGHT, Value);
        }
        break;
    case IDD_WINDOW_POSX:
        gpStateInfo->WindowPosX = Value;
        break;
    case IDD_WINDOW_POSY:
        gpStateInfo->WindowPosY = Value;
        break;
    case IDD_AUTO_POSITION:
        gpStateInfo->AutoPosition = Value;
        break;
    case IDD_COLOR_SCREEN_TEXT:
        gpStateInfo->ScreenAttributes =
                    (gpStateInfo->ScreenAttributes & 0xF0) |
                    (Value & 0x0F);
        break;
    case IDD_COLOR_SCREEN_BKGND:
        gpStateInfo->ScreenAttributes =
                    (gpStateInfo->ScreenAttributes & 0x0F) |
                    (Value << 4);
        break;
    case IDD_COLOR_POPUP_TEXT:
        gpStateInfo->PopupAttributes =
                    (gpStateInfo->PopupAttributes & 0xF0) |
                    (Value & 0x0F);
        break;
    case IDD_COLOR_POPUP_BKGND:
        gpStateInfo->PopupAttributes =
                    (gpStateInfo->PopupAttributes & 0x0F) |
                    (Value << 4);
        break;
    case IDD_COLOR_1:
    case IDD_COLOR_2:
    case IDD_COLOR_3:
    case IDD_COLOR_4:
    case IDD_COLOR_5:
    case IDD_COLOR_6:
    case IDD_COLOR_7:
    case IDD_COLOR_8:
    case IDD_COLOR_9:
    case IDD_COLOR_10:
    case IDD_COLOR_11:
    case IDD_COLOR_12:
    case IDD_COLOR_13:
    case IDD_COLOR_14:
    case IDD_COLOR_15:
    case IDD_COLOR_16:
        gpStateInfo->ColorTable[Item - IDD_COLOR_1] = Value;
        break;
#ifdef i386
    case IDD_FULLSCREEN:
        gpStateInfo->FullScreen = TRUE;
        break;
    case IDD_WINDOWED:
        gpStateInfo->FullScreen = FALSE;
        break;
#endif
#if defined(FE_SB)
    case IDD_LANGUAGELIST:
        /*
         * Value is a code page
         */
        gpStateInfo->CodePage = Value;
        break;
#endif
    case IDD_QUICKEDIT:
        gpStateInfo->QuickEdit = Value;
        break;
    case IDD_INSERT:
        gpStateInfo->InsertMode = Value;
        break;
    case IDD_HISTORY_SIZE:
        gpStateInfo->HistoryBufferSize = max(Value, 1);
        break;
    case IDD_HISTORY_NUM:
        gpStateInfo->NumberOfHistoryBuffers = max(Value, 1);
        break;
    case IDD_HISTORY_NODUP:
        gpStateInfo->HistoryNoDup = Value;
        break;
    case IDD_CURSOR_SMALL:
        gpStateInfo->CursorSize = 25;
        break;
    case IDD_CURSOR_MEDIUM:
        gpStateInfo->CursorSize = 50;
        break;
    case IDD_CURSOR_LARGE:
        gpStateInfo->CursorSize = 100;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}


VOID
UpdateWarningMessage(
    HWND hDlg,
    BOOL fLoadString
    )
{
    static WCHAR achFormat[256];
    WCHAR achText[NELEM(achFormat)+4];
    HWND hWnd;
    UINT Value;

    /*
     * Load the format string, if requested.
     */
    if (fLoadString) {
        LoadString(ghInstance, IDS_WARNING, achFormat, NELEM(achFormat));
    }

    /*
     * Put up the warning message if we're using more than 1 Meg
     * of memory per console window, otherwise hide it.
     */
    hWnd = GetDlgItem(hDlg, IDD_WARNING);
    Value = (gpStateInfo->ScreenBufferSize.X *
             gpStateInfo->ScreenBufferSize.Y) / 0x080000;
    if (Value) {
        wsprintf(achText, achFormat, Value);
        SetWindowText(hWnd, achText);
        ShowWindow(hWnd, SW_SHOW);
    } else {
        ShowWindow(hWnd, SW_HIDE);
    }
}


INT_PTR
WINAPI
ScreenSizeDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the screen size dialog box.

--*/

{
    UINT Value;
    UINT Item;
    HWND hWnd;
    BOOL bOK;
    LONG xScreen;
    LONG yScreen;
    LONG cxScreen;
    LONG cyScreen;
    LONG cxFrame;
    LONG cyFrame;

    switch (wMsg) {
    case WM_INITDIALOG:
        // initialize size of edit controls

        SendDlgItemMessage(hDlg, IDD_SCRBUF_WIDTH, EM_LIMITTEXT, 4, 0L);
        SendDlgItemMessage(hDlg, IDD_SCRBUF_HEIGHT, EM_LIMITTEXT, 4, 0L);
        SendDlgItemMessage(hDlg, IDD_WINDOW_WIDTH, EM_LIMITTEXT, 4, 0L);
        SendDlgItemMessage(hDlg, IDD_WINDOW_HEIGHT, EM_LIMITTEXT, 4, 0L);
        SendDlgItemMessage(hDlg, IDD_WINDOW_POSX, EM_LIMITTEXT, 4, 0L);
        SendDlgItemMessage(hDlg, IDD_WINDOW_POSY, EM_LIMITTEXT, 4, 0L);

        // Get some system parameters

        xScreen  = GetSystemMetrics(SM_XVIRTUALSCREEN);
        yScreen  = GetSystemMetrics(SM_YVIRTUALSCREEN);
        cxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        cyScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        cxFrame  = GetSystemMetrics(SM_CXFRAME);
        cyFrame  = GetSystemMetrics(SM_CYFRAME);

        // initialize arrow controls

        SendDlgItemMessage(hDlg, IDD_SCRBUF_WIDTHSCROLL, UDM_SETRANGE, 0,
                           MAKELONG(9999, 1));
        SendDlgItemMessage(hDlg, IDD_SCRBUF_WIDTHSCROLL, UDM_SETPOS, 0,
                           MAKELONG(gpStateInfo->ScreenBufferSize.X, 0));
        SendDlgItemMessage(hDlg, IDD_SCRBUF_HEIGHTSCROLL, UDM_SETRANGE, 0,
                           MAKELONG(9999, 1));
        SendDlgItemMessage(hDlg, IDD_SCRBUF_HEIGHTSCROLL, UDM_SETPOS, 0,
                           MAKELONG(gpStateInfo->ScreenBufferSize.Y, 0));
        SendDlgItemMessage(hDlg, IDD_WINDOW_WIDTHSCROLL, UDM_SETRANGE, 0,
                           MAKELONG(9999, 1));
        SendDlgItemMessage(hDlg, IDD_WINDOW_WIDTHSCROLL, UDM_SETPOS, 0,
                           MAKELONG(gpStateInfo->WindowSize.X, 0));
        SendDlgItemMessage(hDlg, IDD_WINDOW_HEIGHTSCROLL, UDM_SETRANGE, 0,
                           MAKELONG(9999, 1));
        SendDlgItemMessage(hDlg, IDD_WINDOW_HEIGHTSCROLL, UDM_SETPOS, 0,
                           MAKELONG(gpStateInfo->WindowSize.Y, 0));
        SendDlgItemMessage(hDlg, IDD_WINDOW_POSXSCROLL, UDM_SETRANGE, 0,
                           MAKELONG(xScreen + cxScreen - cxFrame, xScreen - cxFrame));
        SendDlgItemMessage(hDlg, IDD_WINDOW_POSXSCROLL, UDM_SETPOS, 0,
                           MAKELONG(gpStateInfo->WindowPosX, 0));
        SendDlgItemMessage(hDlg, IDD_WINDOW_POSYSCROLL, UDM_SETRANGE, 0,
                           MAKELONG(yScreen + cyScreen - cyFrame, yScreen - cyFrame));
        SendDlgItemMessage(hDlg, IDD_WINDOW_POSYSCROLL, UDM_SETPOS, 0,
                           MAKELONG(gpStateInfo->WindowPosY, 0));

        //
        // put current values in dialog box
        //

        CheckDlgButton(hDlg, IDD_AUTO_POSITION, gpStateInfo->AutoPosition);
        SendMessage(hDlg, WM_COMMAND, IDD_AUTO_POSITION, 0);

        // update the warning message

        UpdateWarningMessage(hDlg, TRUE);

        return TRUE;

    case WM_VSCROLL:
        /*
         * Fake the dialog proc into thinking the edit control just
         * lost focus so it'll update properly
         */
        Item = GetDlgCtrlID((HWND)lParam) - 1;
        SendMessage(hDlg, WM_COMMAND, MAKELONG(Item, EN_KILLFOCUS), 0);
        return TRUE;

    case WM_COMMAND:
        Item = LOWORD(wParam);
        switch (Item) {
        case IDD_SCRBUF_WIDTH:
        case IDD_SCRBUF_HEIGHT:
        case IDD_WINDOW_WIDTH:
        case IDD_WINDOW_HEIGHT:
        case IDD_WINDOW_POSX:
        case IDD_WINDOW_POSY:
            switch (HIWORD(wParam)) {
            case EN_UPDATE:
                if (!CheckNum (hDlg, Item)) {
                    if (!InEM_UNDO) {
                        InEM_UNDO = TRUE;
                        SendMessage((HWND)lParam, EM_UNDO, 0, 0L);
                        InEM_UNDO = FALSE;
                    }
                }
                break;
            case EN_KILLFOCUS:
                /*
                 * Update the state info structure
                 */
                Value = (UINT)SendDlgItemMessage(hDlg, Item + 1, UDM_GETPOS, 0, 0);
                if (HIWORD(Value) == 0) {
                    UpdateStateInfo(hDlg, Item, (SHORT)LOWORD(Value));
                } else {
                    Value = GetStateInfo(hDlg, Item, &bOK);
                    if (bOK) {
                        UpdateItem(hDlg, Item, Value);
                    }
                }

                /*
                 * Update the warning message
                 */
                 UpdateWarningMessage(hDlg, FALSE);

                /*
                 * Update the preview window with the new value
                 */
                hWnd = GetDlgItem(hDlg, IDD_PREVIEWWINDOW);
                SendMessage(hWnd, CM_PREVIEW_UPDATE, 0, 0);
                break;
            }
            return TRUE;

        case IDD_AUTO_POSITION:
            Value = IsDlgButtonChecked(hDlg, IDD_AUTO_POSITION);
            UpdateStateInfo(hDlg, IDD_AUTO_POSITION, Value);
            for (Item = IDD_WINDOW_POSX; Item < IDD_AUTO_POSITION; Item++) {
                hWnd = GetDlgItem(hDlg, Item);
                EnableWindow(hWnd, (Value == FALSE));
            }
            break;

        default:
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case PSN_APPLY:
            /*
             * Write out the state values and exit.
             */
            EndDlgPage(hDlg);
            return TRUE;

        case PSN_KILLACTIVE:
            /*
             * Fake the dialog proc into thinking the edit control just
             * lost focus so it'll update properly
             */
            if (Item = GetDlgCtrlID(GetFocus())) {
                SendMessage(hDlg, WM_COMMAND, MAKELONG(Item, EN_KILLFOCUS), 0);
            }
            return TRUE;
        }
        break;

    default:
        break;
    }
    return CommonDlgProc(hDlg, wMsg, wParam, lParam);
}


INT_PTR
WINAPI
SettingsDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the settings dialog box.

--*/

{
    UINT Value;
    UINT Item;
    BOOL bOK;
    SYSTEM_INFO SystemInfo;

    switch (wMsg) {
    case WM_INITDIALOG:
        GetSystemInfo(&SystemInfo);
        if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
            if (gpStateInfo->FullScreen) {
                CheckRadioButton(hDlg,IDD_WINDOWED,IDD_FULLSCREEN,IDD_FULLSCREEN);
            } else {
                CheckRadioButton(hDlg,IDD_WINDOWED,IDD_FULLSCREEN,IDD_WINDOWED);
            }
        } else {
            ShowWindow(GetDlgItem(hDlg, IDD_WINDOWED), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDD_FULLSCREEN), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDD_DISPLAY_GROUPBOX), SW_HIDE);
        }

        CheckDlgButton(hDlg, IDD_HISTORY_NODUP, gpStateInfo->HistoryNoDup);
        CheckDlgButton(hDlg, IDD_QUICKEDIT, gpStateInfo->QuickEdit);
        CheckDlgButton(hDlg, IDD_INSERT, gpStateInfo->InsertMode);

        // initialize cursor radio buttons

        if (gpStateInfo->CursorSize <= 25) {
            Item = IDD_CURSOR_SMALL;
        } else if (gpStateInfo->CursorSize <= 50) {
            Item = IDD_CURSOR_MEDIUM;
        } else {
            Item = IDD_CURSOR_LARGE;
        }
        CheckRadioButton(hDlg, IDD_CURSOR_SMALL, IDD_CURSOR_LARGE, Item);

        SetDlgItemInt(hDlg, IDD_HISTORY_SIZE, gpStateInfo->HistoryBufferSize,
                      FALSE);
        SendDlgItemMessage(hDlg, IDD_HISTORY_SIZE, EM_LIMITTEXT, 3, 0L);
        SendDlgItemMessage(hDlg, IDD_HISTORY_SIZESCROLL, UDM_SETRANGE, 0,
                           MAKELONG(999, 1));

        SetDlgItemInt(hDlg, IDD_HISTORY_NUM, gpStateInfo->NumberOfHistoryBuffers,
                      FALSE);
        SendDlgItemMessage(hDlg, IDD_HISTORY_NUM, EM_LIMITTEXT, 3, 0L);
        SendDlgItemMessage(hDlg, IDD_HISTORY_NUM, EM_SETSEL, 0, (DWORD)-1);
        SendDlgItemMessage(hDlg, IDD_HISTORY_NUMSCROLL, UDM_SETRANGE, 0,
                           MAKELONG(999, 1));

        // FE_SB
        // Let users select Default CodePage.
        // Per request from PMs, this feature should be activated only for FE enabled NT.
        if (gfFESystem) {
            if (gpStateInfo->hWnd != NULL) {
                LanguageDisplay(hDlg, gpStateInfo->CodePage);
            }
            else{
                LanguageListCreate(hDlg, gpStateInfo->CodePage);
            }
        }
        else {
            // If the system is not FE enabled, just disable and hide them.
            EnableWindow(GetDlgItem(hDlg,IDD_LANGUAGELIST), FALSE);
            ShowWindow(GetDlgItem(hDlg, IDD_LANGUAGELIST), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDD_LANGUAGE_GROUPBOX), SW_HIDE);
        }
        // end FE_SB
        return TRUE;

    case WM_COMMAND:
        Item = LOWORD(wParam);
        switch (Item) {
#ifdef i386
        case IDD_WINDOWED:
        case IDD_FULLSCREEN:
            UpdateStateInfo(hDlg, Item, 0);
            return TRUE;
#endif
        // FE_SB
        case IDD_LANGUAGELIST:
            switch (HIWORD(wParam)) {
            case CBN_KILLFOCUS: {
                HWND hWndLanguageCombo;
                LONG lListIndex;

                hWndLanguageCombo = GetDlgItem(hDlg, IDD_LANGUAGELIST);
                lListIndex = (LONG)SendMessage(hWndLanguageCombo, CB_GETCURSEL, 0, 0L);
                Value = (UINT)SendMessage(hWndLanguageCombo, CB_GETITEMDATA, lListIndex, 0L);
                if (Value != -1) {
                    fChangeCodePage = (Value != gpStateInfo->CodePage);
                    UpdateStateInfo(hDlg, Item, Value);
                }
                break;
            }

            default:
                DBGFONTS(("unhandled CBN_%x from POINTSLIST\n",HIWORD(wParam)));
                break;
            }
            return TRUE;
        // end FE_SB
        case IDD_CURSOR_SMALL:
        case IDD_CURSOR_MEDIUM:
        case IDD_CURSOR_LARGE:
            UpdateStateInfo(hDlg, Item, 0);
            return TRUE;

        case IDD_HISTORY_NODUP:
        case IDD_QUICKEDIT:
        case IDD_INSERT:
            Value = IsDlgButtonChecked(hDlg, Item);
            UpdateStateInfo(hDlg, Item, Value);
            return TRUE;

        case IDD_HISTORY_SIZE:
        case IDD_HISTORY_NUM:
            switch (HIWORD(wParam)) {
            case EN_UPDATE:
                if (!CheckNum(hDlg, Item)) {
                    if (!InEM_UNDO) {
                        InEM_UNDO = TRUE;
                        SendMessage((HWND)lParam, EM_UNDO, 0, 0L);
                        InEM_UNDO = FALSE;
                    }
                }
                break;

            case EN_KILLFOCUS:
                /*
                 * Update the state info structure
                 */
                Value = GetDlgItemInt(hDlg, Item, &bOK, TRUE);
                if (bOK) {
                    UpdateStateInfo(hDlg, Item, Value);
                }
                break;
            }
            return TRUE;

        default:
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case PSN_APPLY:
            /*
             * Write out the state values and exit.
             */
            EndDlgPage(hDlg);
            return TRUE;

        case PSN_KILLACTIVE:
            /*
             * Fake the dialog proc into thinking the edit control just
             * lost focus so it'll update properly
             */
            if (Item = GetDlgCtrlID(GetFocus())) {
                SendMessage(hDlg, WM_COMMAND, MAKELONG(Item, EN_KILLFOCUS), 0);
            }
            return TRUE;
        }
        break;

    case WM_VSCROLL:
        /*
         * Fake the dialog proc into thinking the edit control just
         * lost focus so it'll update properly
         */
        Item = GetDlgCtrlID((HWND)lParam) - 1;
        SendMessage(hDlg, WM_COMMAND, MAKELONG(Item, EN_KILLFOCUS), 0);
        return TRUE;

    default:
        break;
    }
    return CommonDlgProc(hDlg, wMsg, wParam, lParam);
}


INT_PTR
ConsolePropertySheet(
    IN HWND hWnd
    )

/*++

    Creates the property sheet to change console settings.

--*/

{
    PROPSHEETPAGE psp[4];
    PROPSHEETHEADER psh;
    INT_PTR Result = IDCANCEL;
    WCHAR awchBuffer[MAX_PATH];

    //
    // Initialize the state information
    //

    gpStateInfo = InitStateValues((HANDLE)hWnd);
    if (gpStateInfo == NULL) {
        KdPrint(("CONSOLE: can't get state information\n"));
        return IDCANCEL;
    }

    //
    // Initialize the font cache and current font index
    //

    InitializeFonts();
    CurrentFontIndex = FindCreateFont(gpStateInfo->FontFamily,
                                      gpStateInfo->FaceName,
                                      gpStateInfo->FontSize,
                                      gpStateInfo->FontWeight,
                                      gpStateInfo->CodePage);

    //
    // Get the current page number
    //

    gnCurrentPage = GetRegistryValues(NULL);

    //
    // Initialize the property sheet structures
    //

    RtlZeroMemory(psp, sizeof(psp));

    psp[0].dwSize      = sizeof(PROPSHEETPAGE);
    psp[0].hInstance   = ghInstance;
#if defined(FE_SB) // v-HirShi Nov.20.1996
    if (gpStateInfo->hWnd != NULL) {
        psp[0].pszTemplate = MAKEINTRESOURCE(DID_SETTINGS2);
    }
    else{
        psp[0].pszTemplate = MAKEINTRESOURCE(DID_SETTINGS);
    }
#else
    psp[0].pszTemplate = MAKEINTRESOURCE(DID_SETTINGS);
#endif
    psp[0].pfnDlgProc  = SettingsDlgProc;
    psp[0].lParam      = 0;

    psp[1].dwSize      = sizeof(PROPSHEETPAGE);
    psp[1].hInstance   = ghInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(DID_FONTDLG);
    psp[1].pfnDlgProc  = FontDlgProc;
    psp[1].lParam      = 1;

    psp[2].dwSize      = sizeof(PROPSHEETPAGE);
    psp[2].hInstance   = ghInstance;
    psp[2].pszTemplate = MAKEINTRESOURCE(DID_SCRBUFSIZE);
    psp[2].pfnDlgProc  = ScreenSizeDlgProc;
    psp[2].lParam      = 2;

    psp[3].dwSize      = sizeof(PROPSHEETPAGE);
    psp[3].hInstance   = ghInstance;
    psp[3].pszTemplate = MAKEINTRESOURCE(DID_COLOR);
    psp[3].pfnDlgProc  = ColorDlgProc;
    psp[3].lParam      = 3;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPTITLE | PSH_USEICONID | PSH_PROPSHEETPAGE |
                  PSH_NOAPPLYNOW | PSH_USECALLBACK;
    if (gpStateInfo->hWnd) {
        psh.hwndParent = gpStateInfo->hWnd;
    } else {
        psh.hwndParent = hWnd;
    }
    psh.hInstance = ghInstance;
    psh.pszIcon = MAKEINTRESOURCE(IDI_CONSOLE);
    if (gpStateInfo->ConsoleTitle[0] != TEXT('\0')) {
        wcscpy(awchBuffer, TEXT("\""));
        ExpandEnvironmentStrings(gpStateInfo->ConsoleTitle, &awchBuffer[1], NELEM(awchBuffer) - 2);
        wcscat(awchBuffer, TEXT("\""));
        gbStartedFromLink = WereWeStartedFromALnk();
    } else {
        LoadString(ghInstance, IDS_TITLE, awchBuffer, NELEM(awchBuffer));
        gbStartedFromLink = FALSE;
    }
    psh.pszCaption = awchBuffer;
    psh.nPages = NELEM(psp);
    psh.nStartPage = min(gnCurrentPage, NELEM(psp));
    psh.ppsp = psp;
    psh.pfnCallback = ConsolePropSheetProc;

    //
    // Create the property sheet
    //

    Result = PropertySheet(&psh);

    //
    // Send the state values to the console server
    //

    if (gbWriteToConsole) {
        WriteStateValues(gpStateInfo);
    }

    //
    // Save our changes to the registry
    //

    if (gbSaveToRegistry) {

        //
        // If we're looking at the default font, clear the values
        // before we save them
        //

        if ((gpStateInfo->FontFamily == DefaultFontFamily) &&
            (gpStateInfo->FontSize.X == DefaultFontSize.X) &&
            (gpStateInfo->FontSize.Y == DefaultFontSize.Y) &&
            (gpStateInfo->FontWeight == FW_NORMAL) &&
            (wcscmp(gpStateInfo->FaceName, DefaultFaceName) == 0)) {

            gpStateInfo->FontFamily = 0;
            gpStateInfo->FontSize.X = 0;
            gpStateInfo->FontSize.Y = 0;
            gpStateInfo->FontWeight = 0;
            gpStateInfo->FaceName[0] = TEXT('\0');
        }
        if (gbStartedFromLink) {

            if (!SetLinkValues( gpStateInfo )) {

                WCHAR szMessage[ MAX_PATH + 100 ];
                STARTUPINFOW si;
                HWND hwndTemp;

                // An error occured try to save the link file,
                // display a message box to that effect...

                GetStartupInfoW( &si );
                LoadStringW(ghInstance, IDS_LINKERROR, awchBuffer, NELEM(awchBuffer));
                wsprintfW( szMessage,
                           awchBuffer,
                           si.lpTitle
                          );
                LoadStringW(ghInstance, IDS_LINKERRCAP, awchBuffer, NELEM(awchBuffer));
                if (gpStateInfo->hWnd) {
                    hwndTemp = gpStateInfo->hWnd;
                } else {
                    hwndTemp = hWnd;
                }
                MessageBoxW( hwndTemp,
                             szMessage,
                             awchBuffer,
                             MB_APPLMODAL | MB_OK | MB_ICONSTOP | MB_SETFOREGROUND
                            );

            }

        } else {
            SetRegistryValues(gpStateInfo, gnCurrentPage);
        }
    } else {
        SetRegistryValues(NULL, gnCurrentPage);
    }

    //
    // Tell the console server that we're done
    //

    if (gpStateInfo->hWnd) {
        SendMessage(gpStateInfo->hWnd, CM_PROPERTIES_END, 0, 0);
    }

    //
    // Free the state information
    //

    HeapFree(RtlProcessHeap(), 0, gpStateInfo);

    //
    // Destroy the font cache
    //

    DestroyFonts();

    return Result;
}


BOOL
RegisterClasses(HANDLE hModule)
{
    WNDCLASS wc;

    wc.lpszClassName = TEXT("cpColor");
    wc.hInstance     = hModule;
    wc.lpfnWndProc   = (WNDPROC)ColorControlProc;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = NULL;
    wc.lpszMenuName  = NULL;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    wc.lpszClassName = TEXT("WOAWinPreview");
    wc.lpfnWndProc   = PreviewWndProc;
    wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
    wc.style         = 0L;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    wc.lpszClassName = TEXT("WOAFontPreview");
    wc.lpfnWndProc   = FontPreviewWndProc;
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.style         = 0L;
    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    return TRUE;
}

void
UnregisterClasses(HANDLE hModule)
{
    UnregisterClass(TEXT("cpColor"),        hModule);
    UnregisterClass(TEXT("WOAWinPreview"),  hModule);
    UnregisterClass(TEXT("WOAFontPreview"), hModule);
}


PCONSOLE_STATE_INFO
ReadStateValues(HANDLE hMap)
{
    PCONSOLE_STATE_INFO pConsoleInfo;
    PCONSOLE_STATE_INFO pStateInfo;

    /*
     * Map the shared memory block into our address space.
     */
    pConsoleInfo = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (pConsoleInfo == NULL) {
        return NULL;
    }

    /*
     * Copy the data into a locally allocated buffer.
     */
    pStateInfo = HeapAlloc(RtlProcessHeap(), 0, pConsoleInfo->Length);
    if (pStateInfo) {
        RtlCopyMemory(pStateInfo, pConsoleInfo, pConsoleInfo->Length);
    }

    /*
     * Close any open handles.
     */
    UnmapViewOfFile(pConsoleInfo);
    CloseHandle(hMap);

    return pStateInfo;
}


BOOL
WriteStateValues(PCONSOLE_STATE_INFO pStateInfo)
{
    HANDLE hMap;
    PCONSOLE_STATE_INFO pConsoleInfo;

    /*
     * Make sure we have a console window to notify.
     */
    if (pStateInfo->hWnd == NULL) {
        return FALSE;
    }

    /*
     * Create the shared memory block which will contain the state info.
     */
    hMap = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0,
                             pStateInfo->Length, NULL);
    if (!hMap) {
        KdPrint(("CONSOLE: error %d creating file mapping\n", GetLastError()));
        return FALSE;
    }

    /*
     * Map the shared memory block into our address space and copy the
     * data into it.
     */
    pConsoleInfo = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!pConsoleInfo) {
        KdPrint(("CONSOLE: error %d mapping view of file\n", GetLastError()));
        CloseHandle(hMap);
        return FALSE;
    }

    RtlCopyMemory(pConsoleInfo, pStateInfo, pStateInfo->Length);
    UnmapViewOfFile(pConsoleInfo);

    /*
     * Send a message to the server window telling him to read the data
     * and then close any open handles.
     */
    SendMessage(pStateInfo->hWnd, CM_PROPERTIES_UPDATE, (WPARAM)hMap, 0);

    CloseHandle(hMap);

    return TRUE;
}


PCONSOLE_STATE_INFO
InitStateValues(HANDLE hMap)
{
    PCONSOLE_STATE_INFO pStateInfo;

    /*
     * Try to open the shared memory block and read the state info
     * into our address space.
     */
    pStateInfo = ReadStateValues(hMap);
    if (pStateInfo != NULL) {
        return pStateInfo;
    }

    /*
     * Couldn't read the shared memory block so allocate and fill
     * in default values in structure.
     */
    pStateInfo = InitRegistryValues();
    if (pStateInfo == NULL) {
        return NULL;
    }

    /*
     * Now overwrite default values with values from registry
     */
    GetRegistryValues(pStateInfo);

    return pStateInfo;
}
