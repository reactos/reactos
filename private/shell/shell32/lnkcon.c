//
// lnkcon.c       link console property pages implementation
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma hdrstop
#include "lnkcon.h"


LRESULT PreviewWndProc( HWND hWnd, UINT wMessage, WPARAM wParam, LPARAM lParam );

BOOL_PTR CALLBACK _FontDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam );
LRESULT _FontPreviewWndProc( HWND hWnd, UINT wMessage, WPARAM wParam, LPARAM lParam );

extern TCHAR g_szPreviewText[];

// Context-sensitive help ids

const static DWORD rgdwHelpColor[] = {
    IDC_CNSL_COLOR_SCREEN_TEXT,       IDH_CONSOLE_COLOR_SCR_TEXT,
    IDC_CNSL_COLOR_SCREEN_BKGND,      IDH_CONSOLE_COLOR_SCR_BACK,
    IDC_CNSL_COLOR_POPUP_TEXT,        IDH_CONSOLE_COLOR_POPUP_TEXT,
    IDC_CNSL_COLOR_POPUP_BKGND,       IDH_CONSOLE_COLOR_POPUP_BACK,
    IDC_CNSL_COLOR_RED_LBL,           IDH_CONSOLE_COLOR_RED,
    IDC_CNSL_COLOR_RED,               IDH_CONSOLE_COLOR_RED,
    IDC_CNSL_COLOR_GREEN_LBL,         IDH_CONSOLE_COLOR_RED,
    IDC_CNSL_COLOR_GREEN,             IDH_CONSOLE_COLOR_RED,
    IDC_CNSL_COLOR_BLUE_LBL,          IDH_CONSOLE_COLOR_RED,
    IDC_CNSL_COLOR_BLUE,              IDH_CONSOLE_COLOR_RED,
    IDC_CNSL_COLOR_SCREEN_COLORS,     IDH_CONSOLE_COLOR_SCR_COLORS,
    IDC_CNSL_COLOR_SCREEN_COLORS_LBL, IDH_CONSOLE_COLOR_SCR_COLORS,
    IDC_CNSL_COLOR_POPUP_COLORS,      IDH_CONSOLE_COLOR_WIN_COLORS,
    IDC_CNSL_COLOR_POPUP_COLORS_LBL,  IDH_CONSOLE_COLOR_WIN_COLORS,
    IDC_CNSL_COLOR_1,                 IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_2,                 IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_3,                 IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_4,                 IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_5,                 IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_6,                 IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_7,                 IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_8,                 IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_9,                 IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_10,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_11,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_12,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_13,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_14,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_15,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_COLOR_16,                IDH_CONSOLE_COLOR_COLOR_BAR,
    IDC_CNSL_GROUP0,                  -1,
    IDC_CNSL_GROUP1,                  -1,
    IDC_CNSL_GROUP2,                  -1,
    0, 0
};


const static DWORD rgdwHelpSettings[] = {
    IDC_CNSL_HISTORY_NUM_LBL,  IDH_CONSOLE_OPTIONS_BUFF_NUM,
    IDC_CNSL_HISTORY_NUM,      IDH_CONSOLE_OPTIONS_BUFF_NUM,
    IDC_CNSL_HISTORY_SIZE_LBL, IDH_CONSOLE_OPTIONS_BUFF_SIZE,
    IDC_CNSL_HISTORY_SIZE,     IDH_CONSOLE_OPTIONS_BUFF_SIZE,
    IDC_CNSL_CURSOR_SMALL,     IDH_CONSOLE_OPTIONS_CURSOR,
    IDC_CNSL_CURSOR_LARGE,     IDH_CONSOLE_OPTIONS_CURSOR,
    IDC_CNSL_CURSOR_MEDIUM,    IDH_CONSOLE_OPTIONS_CURSOR,
    IDC_CNSL_HISTORY_NODUP,    IDH_CONSOLE_OPTIONS_DISCARD_DUPS,
    IDC_CNSL_INSERT,           IDH_CONSOLE_OPTIONS_INSERT,
    IDC_CNSL_QUICKEDIT,        IDH_CONSOLE_OPTIONS_QUICK_EDIT,
    IDC_CNSL_LANGUAGELIST,     IDH_CONSOLE_OPTIONS_LANGUAGE,
    IDC_CNSL_FULLSCREEN,       IDH_DOS_SCREEN_USAGE_FULL,
    IDC_CNSL_WINDOWED,         IDH_DOS_SCREEN_USAGE_WINDOW,
    IDC_CNSL_GROUP0,           -1,
    IDC_CNSL_GROUP1,           -1,
    IDC_CNSL_GROUP2,           -1,
    0, 0
};

const static DWORD rgdwHelpSize[] = {
    IDC_CNSL_SCRBUF_WIDTH_LBL,   IDH_CONSOLE_SIZE_BUFF_WIDTH,
    IDC_CNSL_SCRBUF_WIDTH,       IDH_CONSOLE_SIZE_BUFF_WIDTH,
    IDC_CNSL_SCRBUF_HEIGHT_LBL,  IDH_CONSOLE_SIZE_BUFF_HEIGHT,
    IDC_CNSL_SCRBUF_HEIGHT,      IDH_CONSOLE_SIZE_BUFF_HEIGHT,
    IDC_CNSL_WINDOW_WIDTH_LBL,   IDH_CONSOLE_SIZE_WIN_WIDTH,
    IDC_CNSL_WINDOW_WIDTH,       IDH_CONSOLE_SIZE_WIN_WIDTH,
    IDC_CNSL_WINDOW_HEIGHT_LBL,  IDH_CONSOLE_SIZE_WIN_HEIGHT,
    IDC_CNSL_WINDOW_HEIGHT,      IDH_CONSOLE_SIZE_WIN_HEIGHT,
    IDC_CNSL_WINDOW_POSX_LBL,    IDH_CONSOLE_SIZE_POS_LEFT,
    IDC_CNSL_WINDOW_POSX,        IDH_CONSOLE_SIZE_POS_LEFT,
    IDC_CNSL_WINDOW_POSY_LBL,    IDH_CONSOLE_SIZE_POS_TOP,
    IDC_CNSL_WINDOW_POSY,        IDH_CONSOLE_SIZE_POS_TOP,
    IDC_CNSL_AUTO_POSITION,      IDH_CONSOLE_SIZE_LET_SYS,
    IDC_CNSL_PREVIEWLABEL,       IDH_DOS_FONT_WINDOW_PREVIEW,
    IDC_CNSL_PREVIEWWINDOW,      IDH_DOS_FONT_WINDOW_PREVIEW,
    IDC_CNSL_GROUP0,             -1,
    IDC_CNSL_GROUP1,             -1,
    IDC_CNSL_GROUP2,             -1,
    0, 0
};





#ifdef ADVANCED_PAGE
VOID
_AddEnvVariable(
    HWND hwndLV,
    INT iItem,
    LPTSTR pszVar
    )
{
    TCHAR szTemp[ 1024 ];
    LV_ITEM lvi;
    LPTSTR p;

    for( p = szTemp; *pszVar!=TEXT('='); *p++ = *pszVar++ );
    *p = TEXT('\0');
    pszVar++;

    lvi.mask = LVIF_TEXT;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.pszText = szTemp;
    SendMessage( hwndLV, LVM_SETITEM, 0, (LPARAM)&lvi );

    for( p=szTemp; *pszVar!=TEXT('\0'); *p++ = *pszVar++ );
    *p = TEXT('\0');
    lvi.mask = LVIF_TEXT;
    lvi.iItem = iItem;
    lvi.iSubItem = 1;
    lvi.pszText = szTemp;
    SendMessage( hwndLV, LVM_SETITEM, 0, (LPARAM)&lvi );

}


BOOL_PTR
CALLBACK
_AdvancedDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the settings dialog box.

--*/

{
    UINT Item;
    HWND hWnd;
    RECT r;
    LV_COLUMN lvc;
    HWND hwndLV;
    LPTSTR pszEnv, pszSave;
    LV_ITEM lvi;
    INT i;

    LPLINKDATA pld = (LPLINKDATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {
    case WM_INITDIALOG:
        pld = (LPLINKDATA)((PROPSHEETPAGE *)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pld);

        // Initialize Columns in listview
        hwndLV = GetDlgItem( hDlg, IDC_CNSL_ADVANCED_LISTVIEW );
        GetClientRect( hwndLV, &r );
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = (((r.right - r.left) - GetSystemMetrics(SM_CXVSCROLL)) * 7) / 20;
        lvc.pszText = TEXT("Variable Name");
        ListView_InsertColumn( hwndLV, 0, &lvc );
        lvc.cx = (((r.right - r.left) - GetSystemMetrics(SM_CXVSCROLL)) * 13) / 20;
        lvc.pszText = TEXT("Value");
        ListView_InsertColumn( hwndLV, 1, &lvc );


        ZeroMemory( &lvi, sizeof(lvi) );
        pszSave = pszEnv = GetEnvironmentStrings();
        while (pszEnv && *pszEnv)
        {

            i = SendMessage( hwndLV, LVM_INSERTITEM, 0, (LPARAM)&lvi );

            _AddEnvVariable( hwndLV, i, pszEnv );

            for( ; *pszEnv; pszEnv++ );
            pszEnv++;


        }

        FreeEnvironmentStrings( pszSave );


        return TRUE;

    case WM_DESTROY:
        EndDialog( hDlg, TRUE );
        break;

    case WM_COMMAND:
        Item = LOWORD(wParam);

        switch (Item)
        {


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
            if (FAILED(_SaveLink(pld)))
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            break;

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
    return FALSE;
}
#endif // ADVANCED_PAGE

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

    LPCONSOLEPROP_DATA pcpd = (LPCONSOLEPROP_DATA)GetWindowLongPtr( hColor, 0 );

    ColorId = GetWindowLong(hColor, GWL_ID);
    hDlg = GetParent(hColor);

    switch (wMsg) {

    case CM_COLOR_INIT:
        SetWindowLongPtr( hColor, 0, lParam );
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTTAB;
        break;

    case WM_SETFOCUS:
        if (pcpd->ColorArray[pcpd->Index] != (BYTE)(ColorId - IDC_CNSL_COLOR_1)) {
            hWnd = GetDlgItem(hDlg, pcpd->ColorArray[pcpd->Index]+IDC_CNSL_COLOR_1);
            SetFocus(hWnd);
        }
        // Fall through
    case WM_KILLFOCUS:
        hdc = GetDC(hDlg);
        hWnd = GetDlgItem(hDlg, IDC_CNSL_COLOR_1);
        GetWindowRect(hWnd, &rColor);
        hWnd = GetDlgItem(hDlg, IDC_CNSL_COLOR_16);
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
            if (ColorId > IDC_CNSL_COLOR_1) {
                SendMessage(hDlg, CM_SETCOLOR,
                            ColorId - 1 - IDC_CNSL_COLOR_1, (LPARAM)hColor);
            }
            break;
        case VK_DOWN:
        case VK_RIGHT:
            if (ColorId < IDC_CNSL_COLOR_16) {
                SendMessage(hDlg, CM_SETCOLOR,
                            ColorId + 1 - IDC_CNSL_COLOR_1, (LPARAM)hColor);
            }
            break;
        case VK_TAB:
            hWnd = GetDlgItem(hDlg, IDC_CNSL_COLOR_1);
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
                    ColorId - IDC_CNSL_COLOR_1, (LPARAM)hColor);
        break;

    case WM_PAINT:
        BeginPaint(hColor, &ps);
        GetClientRect(hColor, &rColor);
        rgbBrush = GetNearestColor(ps.hdc, pcpd->lpConsole->ColorTable[ColorId-IDC_CNSL_COLOR_1]);
        if ((hbr = CreateSolidBrush(rgbBrush)) != NULL) {
            //
            // are we the selected color for the current object?
            //
            if (pcpd->ColorArray[pcpd->Index] == (BYTE)(ColorId - IDC_CNSL_COLOR_1)) {

                //
                // put current values in dialog box
                //
                SendDlgItemMessage(hDlg, IDC_CNSL_COLOR_REDSCROLL,   UDM_SETPOS, 0, MAKELONG( GetRValue(AttrToRGB(pcpd->ColorArray[pcpd->Index])),0));
                SendDlgItemMessage(hDlg, IDC_CNSL_COLOR_GREENSCROLL, UDM_SETPOS, 0, MAKELONG( GetGValue(AttrToRGB(pcpd->ColorArray[pcpd->Index])),0));
                SendDlgItemMessage(hDlg, IDC_CNSL_COLOR_BLUESCROLL,  UDM_SETPOS, 0, MAKELONG( GetBValue(AttrToRGB(pcpd->ColorArray[pcpd->Index])),0));

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

LRESULT
ColorTextProc(
    HWND hWnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Window proc for the color preview windows

--*/

{
    PAINTSTRUCT ps;
    int ColorId;
    RECT rect;
    HBRUSH hbr;
    HFONT hfT;

    LPCONSOLEPROP_DATA pcpd = (LPCONSOLEPROP_DATA)GetWindowLongPtr( hWnd, 0 );

    ColorId = GetWindowLong(hWnd, GWL_ID);
    switch (wMsg) {
    case CM_COLOR_INIT:
        SetWindowLongPtr( hWnd, 0, lParam );
        break;
    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rect);
        InflateRect(&rect, -2, -2);

        if (ColorId == IDC_CNSL_COLOR_SCREEN_COLORS) {
            SetTextColor(ps.hdc, ScreenTextColor(pcpd));
            SetBkColor(ps.hdc, ScreenBkColor(pcpd));
            hbr = CreateSolidBrush( ScreenBkColor(pcpd) );
        } else {
            SetTextColor(ps.hdc, PopupTextColor(pcpd));
            SetBkColor(ps.hdc, PopupBkColor(pcpd));
            hbr = CreateSolidBrush( PopupBkColor(pcpd) );
        }

        /* Draw the text sample */

        FillRect( ps.hdc, &rect, hbr );
        DeleteObject( hbr );

        hfT = SelectObject(ps.hdc, pcpd->FontInfo[pcpd->CurrentFontIndex].hFont);
        DrawText(ps.hdc, g_szPreviewText, -1, &rect, 0);
        SelectObject(ps.hdc, hfT);

        EndPaint(hWnd, &ps);
        break;
    default:
        return DefWindowProc(hWnd, wMsg, wParam, lParam);
        break;
    }
    return TRUE;
}

BOOL_PTR
WINAPI
_ColorDlgProc(
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

    LPLINKDATA pld = (LPLINKDATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {
    case WM_INITDIALOG:
        pld = (LPLINKDATA)((PROPSHEETPAGE *)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pld);
        pld->cpd.bColorInit = FALSE;
        pld->cpd.ColorArray[IDC_CNSL_COLOR_SCREEN_TEXT - IDC_CNSL_COLOR_SCREEN_TEXT] =
                LOBYTE(pld->cpd.lpConsole->wFillAttribute) & 0x0F;
        pld->cpd.ColorArray[IDC_CNSL_COLOR_SCREEN_BKGND - IDC_CNSL_COLOR_SCREEN_TEXT] =
                LOBYTE(pld->cpd.lpConsole->wFillAttribute >> 4);
        pld->cpd.ColorArray[IDC_CNSL_COLOR_POPUP_TEXT - IDC_CNSL_COLOR_SCREEN_TEXT] =
                LOBYTE(pld->cpd.lpConsole->wPopupFillAttribute) & 0x0F;
        pld->cpd.ColorArray[IDC_CNSL_COLOR_POPUP_BKGND - IDC_CNSL_COLOR_SCREEN_TEXT] =
                LOBYTE(pld->cpd.lpConsole->wPopupFillAttribute >> 4);
        CheckRadioButton(hDlg,IDC_CNSL_COLOR_SCREEN_TEXT,IDC_CNSL_COLOR_POPUP_BKGND,IDC_CNSL_COLOR_SCREEN_BKGND);
        pld->cpd.Index = IDC_CNSL_COLOR_SCREEN_BKGND - IDC_CNSL_COLOR_SCREEN_TEXT;

        // initialize color controls
        for (Item=IDC_CNSL_COLOR_1; Item<=IDC_CNSL_COLOR_16; Item++)
            SendDlgItemMessage(hDlg, Item,  CM_COLOR_INIT, 0, (LPARAM)&pld->cpd );

        // initialize text preview controls
        SendDlgItemMessage(hDlg, IDC_CNSL_COLOR_SCREEN_COLORS, CM_COLOR_INIT, 0, (LPARAM)&pld->cpd );
        SendDlgItemMessage(hDlg, IDC_CNSL_COLOR_POPUP_COLORS,  CM_COLOR_INIT, 0, (LPARAM)&pld->cpd );

        // Set ranges & position for updown controls
        SendDlgItemMessage( hDlg, IDC_CNSL_COLOR_REDSCROLL,   UDM_SETRANGE, 0, (LPARAM)MAKELONG( 255, 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_COLOR_GREENSCROLL, UDM_SETRANGE, 0, (LPARAM)MAKELONG( 255, 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_COLOR_BLUESCROLL,  UDM_SETRANGE, 0, (LPARAM)MAKELONG( 255, 0 ) );
#define pcpd (&pld->cpd)
        SendDlgItemMessage( hDlg, IDC_CNSL_COLOR_REDSCROLL,   UDM_SETPOS, 0,
            (LPARAM)MAKELONG(GetRValue(AttrToRGB(pld->cpd.ColorArray[pld->cpd.Index])), 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_COLOR_GREENSCROLL, UDM_SETPOS, 0,
            (LPARAM)MAKELONG(GetGValue(AttrToRGB(pld->cpd.ColorArray[pld->cpd.Index])), 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_COLOR_BLUESCROLL,  UDM_SETPOS, 0,
            (LPARAM)MAKELONG(GetBValue(AttrToRGB(pld->cpd.ColorArray[pld->cpd.Index])), 0 ) );
#undef pcpd
        pld->cpd.bColorInit = TRUE;
        return TRUE;

    //
    // handle help messages
    //


    case WM_HELP:               /* F1 or title-bar help button */
        WinHelp( (HWND) ((LPHELPINFO) lParam)->hItemHandle,
                 NULL,
                 HELP_WM_HELP,
                 (ULONG_PTR) (LPVOID) &rgdwHelpColor[0]
                );
        break;

    case WM_CONTEXTMENU:        /* right mouse click */
        WinHelp( (HWND) wParam,
                 NULL,
                 HELP_CONTEXTMENU,
                 (ULONG_PTR) (LPVOID) &rgdwHelpColor[0]
                );
        break;


    case WM_COMMAND:
        Item = LOWORD(wParam);

        switch (Item)
        {

        case IDC_CNSL_COLOR_SCREEN_TEXT:
        case IDC_CNSL_COLOR_SCREEN_BKGND:
        case IDC_CNSL_COLOR_POPUP_TEXT:
        case IDC_CNSL_COLOR_POPUP_BKGND:
            hWndOld = GetDlgItem(hDlg, pld->cpd.ColorArray[pld->cpd.Index]+IDC_CNSL_COLOR_1);

            pld->cpd.Index = Item - IDC_CNSL_COLOR_SCREEN_TEXT;
            CheckRadioButton(hDlg,IDC_CNSL_COLOR_SCREEN_TEXT,IDC_CNSL_COLOR_POPUP_BKGND,Item);

            // repaint new color
            hWnd = GetDlgItem(hDlg, pld->cpd.ColorArray[pld->cpd.Index]+IDC_CNSL_COLOR_1);
            InvalidateRect(hWnd, NULL, TRUE);

            // repaint old color
            if (hWndOld != hWnd)
            {
                InvalidateRect(hWndOld, NULL, TRUE);
            }

            return TRUE;

        case IDC_CNSL_COLOR_RED:
        case IDC_CNSL_COLOR_GREEN:
        case IDC_CNSL_COLOR_BLUE:
            switch (HIWORD(wParam))
            {

            case EN_UPDATE:
                Value = GetDlgItemInt(hDlg, Item, &bOK, TRUE);
                if (bOK)
                {
                    if (Value > 255) {
                        Value = 255;
                        SetDlgItemInt( hDlg, Item, Value, TRUE );
                    }
                    if (Value < 0) {
                        Value = 0;
                        SetDlgItemInt( hDlg, Item, Value, TRUE );
                    }

                }
                if (pld)
                    pld->cpd.bConDirty = TRUE;
                PropSheet_Changed( GetParent( hDlg ), hDlg );
                break;

            case EN_KILLFOCUS:

                if (!pld)
                    return FALSE;

                //
                // Update the state info structure
                //

#define pcpd (&pld->cpd)
                Value = GetDlgItemInt(hDlg, Item, &bOK, TRUE);
                if (bOK)
                {
                    if (Value > 255) {
                        Value = 255;
                        SetDlgItemInt( hDlg, Item, Value, TRUE );
                    }
                    if (Value < 0) {
                        Value = 0;
                        SetDlgItemInt( hDlg, Item, Value, TRUE );
                    }
                    if (Item == IDC_CNSL_COLOR_RED) {
                        Red = Value;
                    } else {
                        Red = GetRValue(AttrToRGB(pld->cpd.ColorArray[pld->cpd.Index]));
                    }
                    if (Item == IDC_CNSL_COLOR_GREEN) {
                        Green = Value;
                    } else {
                        Green = GetGValue(AttrToRGB(pld->cpd.ColorArray[pld->cpd.Index]));
                    }
                    if (Item == IDC_CNSL_COLOR_BLUE) {
                        Blue = Value;
                    } else {
                        Blue = GetBValue(AttrToRGB(pld->cpd.ColorArray[pld->cpd.Index]));
                    }
                    pld->cpd.lpConsole->ColorTable[pld->cpd.ColorArray[pld->cpd.Index]] =
                                    RGB(Red, Green, Blue);
                }
#undef pcpd

                //
                // Update the preview windows with the new value
                //

                hWnd = GetDlgItem(hDlg, IDC_CNSL_COLOR_SCREEN_COLORS);
                InvalidateRect(hWnd, NULL, FALSE);
                hWnd = GetDlgItem(hDlg, IDC_CNSL_COLOR_POPUP_COLORS);
                InvalidateRect(hWnd, NULL, FALSE);
                hWnd = GetDlgItem(hDlg, pld->cpd.ColorArray[pld->cpd.Index]+IDC_CNSL_COLOR_1);
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
            if (FAILED(_SaveLink(pld)))
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            break;

        }
        break;

    case WM_VSCROLL:
        // minus 1 because edit control precedes the updown control in the .rc file
        Item = GetDlgCtrlID( (HWND)lParam ) - 1;
        switch (LOWORD(wParam)) {
        case SB_ENDSCROLL:
            SendDlgItemMessage(hDlg, Item, EM_SETSEL, 0, (DWORD)-1);
            break;
        default:
            /*
             * Get the new value for the control
             */
            Value = GetDlgItemInt(hDlg, Item, &bOK, TRUE);
            SendDlgItemMessage(hDlg, Item, EM_SETSEL, 0, (DWORD)-1);
            hWnd = GetDlgItem(hDlg, Item);
            SetFocus(hWnd);

            /*
             * Fake the dialog proc into thinking the edit control just
             * lost focus so it'll update properly
             */
            SendMessage(hDlg, WM_COMMAND, MAKELONG(Item, EN_KILLFOCUS), 0);
            pld->cpd.bConDirty = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );

            break;
        }
        return TRUE;

    case CM_SETCOLOR:
        switch( pld->cpd.Index + IDC_CNSL_COLOR_SCREEN_TEXT )
        {
        case IDC_CNSL_COLOR_SCREEN_TEXT:
            pld->cpd.lpConsole->wFillAttribute = (WORD)
                        ((pld->cpd.lpConsole->wFillAttribute & 0xF0) |
                        (wParam & 0x0F));
            break;
        case IDC_CNSL_COLOR_SCREEN_BKGND:
            pld->cpd.lpConsole->wFillAttribute = (WORD)
                        ((pld->cpd.lpConsole->wFillAttribute & 0x0F) |
                        (wParam << 4));
            break;
        case IDC_CNSL_COLOR_POPUP_TEXT:
            pld->cpd.lpConsole->wPopupFillAttribute = (WORD)
                        ((pld->cpd.lpConsole->wPopupFillAttribute & 0xF0) |
                        (wParam & 0x0F));
            break;
        case IDC_CNSL_COLOR_POPUP_BKGND:
            pld->cpd.lpConsole->wPopupFillAttribute = (WORD)
                        ((pld->cpd.lpConsole->wPopupFillAttribute & 0x0F) |
                        (wParam << 4));
            break;
        }

        hWndOld = GetDlgItem(hDlg, pld->cpd.ColorArray[pld->cpd.Index]+IDC_CNSL_COLOR_1);

        pld->cpd.ColorArray[pld->cpd.Index] = (BYTE)wParam;
        pld->cpd.bConDirty = TRUE;
        PropSheet_Changed( GetParent( hDlg ), hDlg );

        // Force the preview window to repaint

        if (pld->cpd.Index < (IDC_CNSL_COLOR_POPUP_TEXT - IDC_CNSL_COLOR_SCREEN_TEXT)) {
            hWnd = GetDlgItem(hDlg, IDC_CNSL_COLOR_SCREEN_COLORS);
        } else {
            hWnd = GetDlgItem(hDlg, IDC_CNSL_COLOR_POPUP_COLORS);
        }
        InvalidateRect(hWnd, NULL, TRUE);

        // repaint new color
        hWnd = GetDlgItem(hDlg, pld->cpd.ColorArray[pld->cpd.Index]+IDC_CNSL_COLOR_1);
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
    return FALSE;
}



BOOL_PTR
CALLBACK
_ConsoleSettingsDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the settings dialog box.

--*/

{
    UINT Item;
    SYSTEM_INFO SystemInfo;

    LPLINKDATA pld = (LPLINKDATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {
    case WM_INITDIALOG:
        pld = (LPLINKDATA)((PROPSHEETPAGE *)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pld);
        GetSystemInfo(&SystemInfo);
        if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
            if (pld->cpd.lpConsole->bFullScreen)
                CheckRadioButton(hDlg,IDC_CNSL_WINDOWED,IDC_CNSL_FULLSCREEN,IDC_CNSL_FULLSCREEN);
            else
                CheckRadioButton(hDlg,IDC_CNSL_WINDOWED,IDC_CNSL_FULLSCREEN,IDC_CNSL_WINDOWED);
        } else {
            ShowWindow(GetDlgItem(hDlg, IDC_CNSL_WINDOWED), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_CNSL_FULLSCREEN), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_CNSL_GROUP2), SW_HIDE);
        }

        CheckDlgButton(hDlg, IDC_CNSL_HISTORY_NODUP, pld->cpd.lpConsole->bHistoryNoDup);
        CheckDlgButton(hDlg, IDC_CNSL_QUICKEDIT, pld->cpd.lpConsole->bQuickEdit);
        CheckDlgButton(hDlg, IDC_CNSL_INSERT, pld->cpd.lpConsole->bInsertMode);

        // initialize cursor radio buttons

        if (pld->cpd.lpConsole->uCursorSize <= 25)
        {
            Item = IDC_CNSL_CURSOR_SMALL;
        }
        else if (pld->cpd.lpConsole->uCursorSize <= 50)
        {
            Item = IDC_CNSL_CURSOR_MEDIUM;
        }
        else
        {
            Item = IDC_CNSL_CURSOR_LARGE;
        }
        CheckRadioButton(hDlg, IDC_CNSL_CURSOR_SMALL, IDC_CNSL_CURSOR_LARGE, Item);

        // initialize updown controls

        SendDlgItemMessage( hDlg, IDC_CNSL_HISTORY_SIZESCROLL,  UDM_SETRANGE, 0, (LPARAM)MAKELONG( 999, 1 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_HISTORY_NUMSCROLL,   UDM_SETRANGE, 0, (LPARAM)MAKELONG( 999, 1 ) );

        //
        // put current values in dialog box
        //
        SendDlgItemMessage( hDlg, IDC_CNSL_HISTORY_SIZESCROLL,  UDM_SETPOS, 0, (LPARAM)MAKELONG( pld->cpd.lpConsole->uHistoryBufferSize, 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_HISTORY_NUMSCROLL,   UDM_SETPOS, 0, (LPARAM)MAKELONG( pld->cpd.lpConsole->uNumberOfHistoryBuffers, 0 ) );

        if (IsFarEastCP(pld->cpd.uOEMCP))
            LanguageListCreate(hDlg, pld->cpd.lpFEConsole->uCodePage);
        else
        {
            ShowWindow(GetDlgItem(hDlg, IDC_CNSL_GROUP3), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_CNSL_LANGUAGELIST), SW_HIDE);
        }
        return TRUE;

    case WM_DESTROY:
        EndDialog( hDlg, TRUE );
        break;

    //
    // handle help messages
    //
    case WM_HELP:               /* F1 or title-bar help button */
        WinHelp( (HWND) ((LPHELPINFO) lParam)->hItemHandle,
                 NULL,
                 HELP_WM_HELP,
                 (ULONG_PTR) (LPVOID) &rgdwHelpSettings[0]
                );
        break;

    case WM_CONTEXTMENU:        /* right mouse click */
        WinHelp( (HWND) wParam,
                 NULL,
                 HELP_CONTEXTMENU,
                 (ULONG_PTR) (LPVOID) &rgdwHelpSettings[0]
                );
        break;


    case WM_COMMAND:
        Item = LOWORD(wParam);

        switch (Item)
        {

        case IDC_CNSL_HISTORY_SIZE:
            if (pld && (HIWORD(wParam)==EN_UPDATE))
            {
                pld->cpd.lpConsole->uHistoryBufferSize =
                    GetDlgItemInt( hDlg, Item, NULL, FALSE );
                pld->cpd.bConDirty = TRUE;
                PropSheet_Changed( GetParent( hDlg ), hDlg );
            }
            break;

        case IDC_CNSL_HISTORY_NUM:
            if (pld && (HIWORD(wParam)==EN_UPDATE))
            {
                pld->cpd.lpConsole->uNumberOfHistoryBuffers =
                    GetDlgItemInt( hDlg, Item, NULL, FALSE );
                pld->cpd.bConDirty = TRUE;
                PropSheet_Changed( GetParent( hDlg ), hDlg );
            }
            break;

#ifdef i386
        case IDC_CNSL_WINDOWED:
        case IDC_CNSL_FULLSCREEN:
            CheckRadioButton(hDlg, IDC_CNSL_WINDOWED, IDC_CNSL_FULLSCREEN, Item);
            pld->cpd.lpConsole->bFullScreen = (Item == IDC_CNSL_FULLSCREEN);
            pld->cpd.bConDirty = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );
            return TRUE;
#endif
        case IDC_CNSL_LANGUAGELIST:
            switch (HIWORD(wParam)) {
            case CBN_KILLFOCUS: {
                HWND hWndLanguageCombo;
                LONG lListIndex;
                UINT  Value;

                hWndLanguageCombo = GetDlgItem(hDlg, IDC_CNSL_LANGUAGELIST);
                lListIndex = (LONG) SendMessage(hWndLanguageCombo, CB_GETCURSEL, 0, 0L);
                Value = (UINT) SendMessage(hWndLanguageCombo, CB_GETITEMDATA, lListIndex, 0L);
                if (Value != (UINT)-1) {
                    pld->cpd.fChangeCodePage = (Value != pld->cpd.lpFEConsole->uCodePage);
                    pld->cpd.lpFEConsole->uCodePage = Value;
                    PropSheet_Changed( GetParent( hDlg ), hDlg );
                }
                break;
            }

            default:
                break;
            }
            return TRUE;

        case IDC_CNSL_CURSOR_SMALL:
            pld->cpd.lpConsole->uCursorSize = 25;
            goto SetCursorSize;
        case IDC_CNSL_CURSOR_MEDIUM:
            pld->cpd.lpConsole->uCursorSize = 50;
            goto SetCursorSize;
        case IDC_CNSL_CURSOR_LARGE:
            pld->cpd.lpConsole->uCursorSize = 100;
SetCursorSize:
            pld->cpd.bConDirty = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );
            CheckRadioButton(hDlg, IDC_CNSL_CURSOR_SMALL, IDC_CNSL_CURSOR_LARGE, Item);
            return TRUE;

        case IDC_CNSL_HISTORY_NODUP:
            pld->cpd.lpConsole->bHistoryNoDup = IsDlgButtonChecked( hDlg, Item );
            pld->cpd.bConDirty = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );
            return TRUE;

        case IDC_CNSL_QUICKEDIT:
            pld->cpd.lpConsole->bQuickEdit = IsDlgButtonChecked( hDlg, Item );
            pld->cpd.bConDirty = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );
            return TRUE;

        case IDC_CNSL_INSERT:
            pld->cpd.lpConsole->bInsertMode = IsDlgButtonChecked( hDlg, Item );
            pld->cpd.bConDirty = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );
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
            if (FAILED(_SaveLink(pld)))
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            break;

        case PSN_KILLACTIVE:
            /*
             * Fake the dialog proc into thinking the edit control just
             * lost focus so it'll update properly
             */
            if (0 != (Item = GetDlgCtrlID(GetFocus()))) {
                SendMessage(hDlg, WM_COMMAND, MAKELONG(Item, EN_KILLFOCUS), 0);
            }
            return TRUE;
        }
        break;


    default:
        break;
    }
    return FALSE;
}

BOOL
CheckBufferSize(
    HWND hDlg,
    UINT Item,
    LPLINKDATA pld,
    INT i
    )
/*++

    Checks to make sure the buffer size is not smaller than the window size

    Returns: TRUE if preview window should be updated
             FALSE if not

--*/
{
    BOOL fRet = FALSE;

    if (!pld)
     return fRet;

    switch (Item)
    {

    case IDC_CNSL_SCRBUF_WIDTHSCROLL:
    case IDC_CNSL_SCRBUF_WIDTH:
        if (i >= 1)
        {
            pld->cpd.lpConsole->dwScreenBufferSize.X = (SHORT) i;
            if (pld->cpd.lpConsole->dwWindowSize.X > i)
            {
                pld->cpd.lpConsole->dwWindowSize.X = (SHORT) i;
                SetDlgItemInt( hDlg,
                               IDC_CNSL_WINDOW_WIDTH,
                               pld->cpd.lpConsole->dwWindowSize.X,
                               TRUE
                              );

                fRet = TRUE;
            }

        }
        break;

    case IDC_CNSL_SCRBUF_HEIGHTSCROLL:
    case IDC_CNSL_SCRBUF_HEIGHT:
        if (i >= 1)
        {
            pld->cpd.lpConsole->dwScreenBufferSize.Y = (SHORT) i;
            if (pld->cpd.lpConsole->dwWindowSize.Y > i)
            {
                pld->cpd.lpConsole->dwWindowSize.Y = (SHORT) i;
                SetDlgItemInt( hDlg,
                               IDC_CNSL_WINDOW_HEIGHT,
                               pld->cpd.lpConsole->dwWindowSize.Y,
                               TRUE
                              );
                fRet = TRUE;
            }
        }
        break;

    case IDC_CNSL_WINDOW_WIDTHSCROLL:
    case IDC_CNSL_WINDOW_WIDTH:
        if (i >= 1)
        {
            pld->cpd.lpConsole->dwWindowSize.X = (SHORT) i;
            if (pld->cpd.lpConsole->dwScreenBufferSize.X < i)
            {
                pld->cpd.lpConsole->dwScreenBufferSize.X = (SHORT) i;
                SetDlgItemInt( hDlg,
                               IDC_CNSL_SCRBUF_WIDTH,
                               pld->cpd.lpConsole->dwScreenBufferSize.X,
                               TRUE
                              );
                fRet = TRUE;
            }

        }
        break;

    case IDC_CNSL_WINDOW_HEIGHTSCROLL:
    case IDC_CNSL_WINDOW_HEIGHT:
        if (i >= 1)
        {
            pld->cpd.lpConsole->dwWindowSize.Y = (SHORT) i;
            if (pld->cpd.lpConsole->dwScreenBufferSize.Y < i)
            {
                pld->cpd.lpConsole->dwScreenBufferSize.Y = (SHORT) i;
                SetDlgItemInt( hDlg,
                               IDC_CNSL_SCRBUF_HEIGHT,
                               pld->cpd.lpConsole->dwScreenBufferSize.Y,
                               TRUE
                              );
                fRet = TRUE;
            }
        }

    }

    return fRet;

}

BOOL
IsValidSetting(
    HWND hDlg,
    UINT Item,
    LPLINKDATA pld,
    INT i
    )
/*++

    Checks to make sure the proposed new value is valid for the console

    Returns: TRUE if it is valid
             FALSE if not

--*/
{

    BOOL fRet = TRUE;

    if (!pld)
        return FALSE;

    if (i>9999)
        i = -1;

    switch (Item)
    {

    case IDC_CNSL_WINDOW_HEIGHT:
        if (i <= 0)
        {
            SetDlgItemInt( hDlg,
                           Item,
                           pld->cpd.lpConsole->dwWindowSize.Y,
                           TRUE
                          );
            fRet = FALSE;
        }
        else
        {
            pld->cpd.lpConsole->dwWindowSize.Y = (SHORT) i;
        }
        break;

    case IDC_CNSL_WINDOW_WIDTH:
        if (i <= 0)
        {
            SetDlgItemInt( hDlg,
                           Item,
                           pld->cpd.lpConsole->dwWindowSize.X,
                           TRUE
                          );
            fRet = FALSE;
        }
        else
        {
            pld->cpd.lpConsole->dwWindowSize.X = (SHORT) i;
        }
        break;

    case IDC_CNSL_SCRBUF_WIDTH:
        if (i <= 0)
        {
            SetDlgItemInt( hDlg,
                           Item,
                           pld->cpd.lpConsole->dwScreenBufferSize.X,
                           TRUE
                          );
            fRet = FALSE;
        }
        else
        {
            pld->cpd.lpConsole->dwScreenBufferSize.X = (SHORT) i;
        }
        break;

    case IDC_CNSL_SCRBUF_HEIGHT:
        if (i <= 0)
        {
            SetDlgItemInt( hDlg,
                           Item,
                           pld->cpd.lpConsole->dwScreenBufferSize.Y,
                           TRUE
                          );
            fRet = FALSE;
        }
        else
        {
            pld->cpd.lpConsole->dwScreenBufferSize.Y = (SHORT) i;
        }
        break;
    }

    if (i <= 0)
    {
        SendDlgItemMessage( hDlg,
                            Item,
                            EM_SETSEL,
                            (WPARAM)(INT)4,
                            (WPARAM)(INT)4
                           );
    }

    return fRet;
}

BOOL_PTR
CALLBACK
_ScreenSizeDlgProc(
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
    LONG xScreen;
    LONG yScreen;
    LONG cxScreen;
    LONG cyScreen;
    LONG cxFrame;
    LONG cyFrame;

    LPLINKDATA pld = (LPLINKDATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {

    case WM_INITDIALOG:
        pld = (LPLINKDATA)((PROPSHEETPAGE *)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pld);
        SendDlgItemMessage(hDlg, IDC_CNSL_PREVIEWWINDOW, CM_PREVIEW_INIT, 0, (LPARAM)&pld->cpd );
        SendDlgItemMessage(hDlg, IDC_CNSL_PREVIEWWINDOW, CM_PREVIEW_UPDATE, 0, 0 );

        // Get some system parameters

        xScreen  = GetSystemMetrics(SM_XVIRTUALSCREEN);
        yScreen  = GetSystemMetrics(SM_YVIRTUALSCREEN);
        cxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        cyScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        cxFrame  = GetSystemMetrics(SM_CXFRAME);
        cyFrame  = GetSystemMetrics(SM_CYFRAME);

        // initialize updown controls

        SendDlgItemMessage( hDlg, IDC_CNSL_SCRBUF_WIDTHSCROLL,  UDM_SETRANGE, 0, (LPARAM)MAKELONG( MAX_SCRBUF_WIDTH, 1 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_SCRBUF_HEIGHTSCROLL, UDM_SETRANGE, 0, (LPARAM)MAKELONG( MAX_SCRBUF_HEIGHT, 1 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_WINDOW_WIDTHSCROLL,  UDM_SETRANGE, 0, (LPARAM)MAKELONG( 9999, 1 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_WINDOW_HEIGHTSCROLL, UDM_SETRANGE, 0, (LPARAM)MAKELONG( 9999, 1 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_WINDOW_POSXSCROLL,   UDM_SETRANGE, 0, (LPARAM)MAKELONG( xScreen + cxScreen - cxFrame, xScreen - cxFrame ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_WINDOW_POSYSCROLL,   UDM_SETRANGE, 0, (LPARAM)MAKELONG( yScreen + cyScreen - cyFrame, yScreen - cyFrame ) );

        //
        // put current values in dialog box
        //

        SendDlgItemMessage( hDlg, IDC_CNSL_SCRBUF_WIDTHSCROLL,  UDM_SETPOS, 0, (LPARAM)MAKELONG( pld->cpd.lpConsole->dwScreenBufferSize.X, 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_SCRBUF_HEIGHTSCROLL, UDM_SETPOS, 0, (LPARAM)MAKELONG( pld->cpd.lpConsole->dwScreenBufferSize.Y, 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_WINDOW_WIDTHSCROLL,  UDM_SETPOS, 0, (LPARAM)MAKELONG( pld->cpd.lpConsole->dwWindowSize.X, 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_WINDOW_HEIGHTSCROLL, UDM_SETPOS, 0, (LPARAM)MAKELONG( pld->cpd.lpConsole->dwWindowSize.Y, 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_WINDOW_POSXSCROLL,   UDM_SETPOS, 0, (LPARAM)MAKELONG( pld->cpd.lpConsole->dwWindowOrigin.X, 0 ) );
        SendDlgItemMessage( hDlg, IDC_CNSL_WINDOW_POSYSCROLL,   UDM_SETPOS, 0, (LPARAM)MAKELONG( pld->cpd.lpConsole->dwWindowOrigin.Y, 0 ) );

        CheckDlgButton(hDlg, IDC_CNSL_AUTO_POSITION, pld->cpd.lpConsole->bAutoPosition);
        SendMessage(hDlg, WM_COMMAND, IDC_CNSL_AUTO_POSITION, 0);

        return TRUE;

    case WM_DESTROY:
        EndDialog( hDlg, TRUE );
        break;

    //
    // handle help messages
    //
    case WM_HELP:               /* F1 or title-bar help button */
        WinHelp( (HWND) ((LPHELPINFO) lParam)->hItemHandle,
                 NULL,
                 HELP_WM_HELP,
                 (ULONG_PTR) (LPVOID) &rgdwHelpSize[0]
                );
        break;

    case WM_CONTEXTMENU:        /* right mouse click */
        WinHelp( (HWND) wParam,
                 NULL,
                 HELP_CONTEXTMENU,
                 (ULONG_PTR) (LPVOID) &rgdwHelpSize[0]
                );
        break;


    case WM_COMMAND:
        Item = LOWORD(wParam);

        if (Item==IDC_CNSL_AUTO_POSITION)
        {
            pld->cpd.lpConsole->bAutoPosition = IsDlgButtonChecked( hDlg, Item );
            pld->cpd.bConDirty = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );
            Value = IsDlgButtonChecked(hDlg, IDC_CNSL_AUTO_POSITION);
            for (Item = IDC_CNSL_WINDOW_POSX; Item < IDC_CNSL_AUTO_POSITION; Item++) {
                hWnd = GetDlgItem(hDlg, Item);
                EnableWindow(hWnd, (Value == FALSE));
            }
        }

        //
        // Make sure that we don't have a buffer size smaller than a window size
        //
        if (pld && (HIWORD(wParam)==EN_KILLFOCUS))
        {
            INT i;

            i = GetDlgItemInt( hDlg, Item, NULL, FALSE );
            if (CheckBufferSize( hDlg, Item, pld, i ))
                goto UpdatePrevWindow;

        }

        //
        // Verify that what is typed is a valid quantity...
        //
        if (pld && (HIWORD(wParam)==EN_UPDATE))
        {
            INT i;

            i = GetDlgItemInt( hDlg, Item, NULL, FALSE );
            IsValidSetting( hDlg, Item, pld, i );

            switch( Item )
            {

            case IDC_CNSL_WINDOW_POSX:
                pld->cpd.lpConsole->dwWindowOrigin.X = (SHORT)
                    GetDlgItemInt( hDlg, Item, NULL, TRUE );
                break;

            case IDC_CNSL_WINDOW_POSY:
                pld->cpd.lpConsole->dwWindowOrigin.Y = (SHORT)
                    GetDlgItemInt( hDlg, Item, NULL, TRUE );
                break;

            }

UpdatePrevWindow:

            pld->cpd.bConDirty = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );
            SendDlgItemMessage(hDlg, IDC_CNSL_PREVIEWWINDOW, CM_PREVIEW_UPDATE, 0, 0 );

        }

        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case UDN_DELTAPOS:
        {
            NM_UPDOWN * lpud = (NM_UPDOWN *)lParam;
            INT i;

            i = lpud->iPos + lpud->iDelta;

            // Check for bad ranges
            if ((i > 9999) || (i < 1))
                return TRUE;

            // check restrictions and alter values accordingly.  (Buffer size
            // can never be smaller than window size!)
            CheckBufferSize( hDlg, (UINT)wParam, pld, i);

            // highlight the changed entry
            SendDlgItemMessage( hDlg,
                                (UINT)wParam,
                                EM_SETSEL,
                                (WPARAM)(INT)4,
                                (WPARAM)(INT)4
                               );

            // Update the preview window
            pld->cpd.bConDirty = TRUE;
            PropSheet_Changed( GetParent( hDlg ), hDlg );
            SendDlgItemMessage(hDlg, IDC_CNSL_PREVIEWWINDOW, CM_PREVIEW_UPDATE, 0, 0 );
        }
        break;

        case PSN_APPLY:
            /*
             * Write out the state values and exit.
             */
            if (FAILED(_SaveLink(pld)))
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            break;

//        case PSN_HELP:
//            //WinHelp(hDlg, szHelpFileName, HELP_CONTEXT, DID_SCRBUFSIZE);
//            return TRUE;

        case PSN_KILLACTIVE:
            /*
             * Fake the dialog proc into thinking the edit control just
             * lost focus so it'll update properly
             */
            if (0 != (Item = GetDlgCtrlID(GetFocus()))) {
                SendMessage(hDlg, WM_COMMAND, MAKELONG(Item, EN_KILLFOCUS), 0);
            }
            return TRUE;
        }
        break;

    default:
        break;
    }
    return FALSE;
}

void *_CopyDataBlock(IShellLink *psl, DWORD dwSig)
{
    void *pv = NULL;
    IShellLinkDataList *psld;
    if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IShellLinkDataList, (void **)&psld)))
    {
        psld->lpVtbl->CopyDataBlock(psld, dwSig, &pv);
        psld->lpVtbl->Release(psld);
    }
    return (void *)pv;
}

void _RemoveDataBlock(IShellLink *psl, DWORD dwSig)
{
    IShellLinkDataList *psld;
    if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IShellLinkDataList, (void **)&psld)))
    {
        psld->lpVtbl->RemoveDataBlock(psld, dwSig);
        psld->lpVtbl->Release(psld);
    }
}

void _AddDataBlock(IShellLink *psl, void *pv)
{
    IShellLinkDataList *psld;
    if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IShellLinkDataList, (void **)&psld)))
    {
        psld->lpVtbl->AddDataBlock(psld, pv);
        psld->lpVtbl->Release(psld);
    }
}


VOID LinkConsolePagesSave( LPLINKDATA pld )
{
    // First, remove the console settings section if it exists
    _RemoveDataBlock(pld->cpd.psl, NT_CONSOLE_PROPS_SIG);
    _RemoveDataBlock(pld->cpd.psl, NT_FE_CONSOLE_PROPS_SIG);

#ifndef UNICODE
    // if we're the ANSI shell, we need to convert FACENAME
    // over to UNICODE before saving...
    {
        WCHAR wszFaceName[LF_FACESIZE];

        MultiByteToWideChar( CP_ACP, 0,
                         pld->cpd.lpFaceName, LF_FACESIZE,
                         wszFaceName, LF_FACESIZE
                        );
        hmemcpy(pld->cpd.lpConsole->FaceName, wszFaceName, LF_FACESIZE*SIZEOF(WCHAR));
    }

#endif
    //
    // Now, add back the new console settings
    _AddDataBlock(pld->cpd.psl, pld->cpd.lpConsole);

    //
    // Now, update registry settings for this title...
    SetRegistryValues( &pld->cpd );

    if (IsFarEastCP(pld->cpd.uOEMCP))
    {
        // Same for FarEast console settings
        //
        _AddDataBlock(pld->cpd.psl, pld->cpd.lpFEConsole);

        SetFERegistryValues( &pld->cpd );
    }
    // And, mark the console data as current
    pld->cpd.bConDirty = FALSE;
}

#define PEMAGIC         ((WORD)'P'+((WORD)'E'<<8))
void AddLinkConsolePages( LPLINKDATA pld, IShellLink * psl, LPCTSTR pszFile, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hpage;
    PROPSHEETPAGE psp;
    HRESULT hres;
    TCHAR szTarget[ MAX_PATH ];
    TCHAR szBuffer[ MAX_PATH ];
    WNDCLASS wc;
    IPersistFile *ppf;


    // do this here so we don't slow down the loading
    // of other pages

    if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf)))
    {
        WCHAR wszPath[ MAX_PATH ];

        SHTCharToUnicode(pszFile, wszPath, ARRAYSIZE(wszPath));
        hres = ppf->lpVtbl->Load(ppf, wszPath, 0);
        ppf->lpVtbl->Release(ppf);
    }

    // Get the target of the link
    hres = psl->lpVtbl->GetPath(psl, szBuffer, ARRAYSIZE(szBuffer), NULL, 0);
    pld->cpd.psl = psl;

    if (!SUCCEEDED(hres) || GetScode(hres) == S_FALSE)
        goto Exit;

    // Remove args first, to:
    // (1) shorten our string, avoiding ExpandEnvironmentStrings overflowing the buffer
    // (2) increase liklihood that PathRemoveArgs won't get confused by spaces in "Program Files" etc
    PathRemoveArgs(szTarget);

    // expand the target
    if (!SHExpandEnvironmentStrings(szBuffer, szTarget, ARRAYSIZE(szTarget)))
        goto Exit;

    // Check what kind of app target is LOWORD==PEMAGIC, HIWORD==0
    if (GetExeType( szTarget )!=PEMAGIC)
        goto Exit;

    // It's a WIN32 console mode app, let's put up our property sheet

    wc.lpszClassName = TEXT("WOACnslWinPreview");
    wc.hInstance     = HINST_THISDLL;
    wc.lpfnWndProc   = PreviewWndProc;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = NULL;
    wc.lpszMenuName  = NULL;
    wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
    wc.style         = 0L;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 2 * sizeof(PVOID);   // (two pointers)
    if (!RegisterClass(&wc))
        if (GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
            goto Exit;


    wc.lpszClassName = TEXT("WOACnslFontPreview");
    wc.hInstance     = HINST_THISDLL;
    wc.lpfnWndProc   = _FontPreviewWndProc;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = NULL;
    wc.lpszMenuName  = NULL;
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.style         = 0L;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(PVOID);       // (one pointer)
    if (!RegisterClass(&wc))
        if (GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
            goto Exit;

    wc.lpszClassName = TEXT("cpColor");
    wc.hInstance     = HINST_THISDLL;
    wc.lpfnWndProc   = ColorControlProc;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = NULL;
    wc.lpszMenuName  = NULL;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.style         = 0L;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(PVOID);       // (one pointer)
    if (!RegisterClass(&wc))
        if (GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
            goto Exit;

    wc.lpszClassName = TEXT("cpShowColor");
    wc.hInstance     = HINST_THISDLL;
    wc.lpfnWndProc   = ColorTextProc;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = NULL;
    wc.lpszMenuName  = NULL;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.style         = 0L;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(PVOID);       // (one pointer)
    if (!RegisterClass(&wc))
        if (GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)
            goto Exit;

    // Needs TTFontList for all platform.
    if (!NT_SUCCESS( InitializeDbcsMisc(&pld->cpd) ))
        goto Exit;

    GetTitleFromLinkName( (LPTSTR)pszFile, (LPTSTR)pld->cpd.ConsoleTitle );

    // Check if Far East settings exist...
    
    if ((pld->cpd.lpFEConsole = (LPNT_FE_CONSOLE_PROPS)_CopyDataBlock(psl, NT_FE_CONSOLE_PROPS_SIG))==NULL)
    {
        pld->cpd.lpFEConsole = (LPNT_FE_CONSOLE_PROPS)LocalAlloc( LPTR, SIZEOF(NT_FE_CONSOLE_PROPS) );
        if (pld->cpd.lpFEConsole) 
        {
            // Initialize Far East Console settings
            pld->cpd.lpFEConsole->cbSize = SIZEOF( NT_FE_CONSOLE_PROPS );
            pld->cpd.lpFEConsole->dwSignature = NT_FE_CONSOLE_PROPS_SIG;
            if (IsFarEastCP(pld->cpd.uOEMCP))
            {
                InitFERegistryValues( &pld->cpd );
                GetFERegistryValues( &pld->cpd );
            }
        }
    }

    if (!pld->cpd.lpFEConsole)
        goto Exit;
    
    // Get standard settings from link if they exist...
    if ((pld->cpd.lpConsole = (LPNT_CONSOLE_PROPS)_CopyDataBlock(psl, NT_CONSOLE_PROPS_SIG ))==NULL)
    {
        pld->cpd.lpConsole = (LPNT_CONSOLE_PROPS)LocalAlloc(LPTR, SIZEOF(NT_CONSOLE_PROPS) );
        if (pld->cpd.lpConsole)
        {
            // Initialize console settings
            pld->cpd.lpConsole->cbSize = SIZEOF( NT_CONSOLE_PROPS );
            pld->cpd.lpConsole->dwSignature = NT_CONSOLE_PROPS_SIG;
            InitRegistryValues( &pld->cpd );
            GetRegistryValues( &pld->cpd );
        }
        else
        {
            // if the above alloc failes, we fault dereferencing lpConsole...
            ASSERT(FALSE);
        }

    }
#ifndef UNICODE
    else
    {
        // we read the properties off of disk -- so need to convert the
        // UNICODE string to ANSI

        WCHAR wszFaceName[LF_FACESIZE];

        hmemcpy(wszFaceName,pld->cpd.lpConsole->FaceName,LF_FACESIZE*SIZEOF(WCHAR));

        WideCharToMultiByte( CP_ACP, 0,
                             wszFaceName, LF_FACESIZE,
                             pld->cpd.szFaceName, LF_FACESIZE,
                             NULL, NULL
                            );
    }
#endif

    if (!pld->cpd.lpConsole)
        goto Exit;

    // set facename pointer to correct place
#ifdef UNICODE
    pld->cpd.lpFaceName = (LPTSTR)pld->cpd.lpConsole->FaceName;
#else
    pld->cpd.lpFaceName = (LPTSTR)pld->cpd.szFaceName;
#endif

    //
    // Initialize the font cache and current font index
    //

    InitializeFonts( &pld->cpd );
    pld->cpd.CurrentFontIndex = FindCreateFont( &pld->cpd,
                                                pld->cpd.lpConsole->uFontFamily,
                                                pld->cpd.lpFaceName,
                                                pld->cpd.lpConsole->dwFontSize,
                                                pld->cpd.lpConsole->uFontWeight);

    // Mark the console data as current
    pld->cpd.bConDirty = FALSE;

    // add console settings property sheet
    psp.dwSize      = SIZEOF( psp );
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = HINST_THISDLL;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CONSOLE_SETTINGS);
    psp.pfnDlgProc  = _ConsoleSettingsDlgProc;
    psp.lParam      = (LPARAM)pld;

    hpage = CreatePropertySheetPage( &psp );
    if (hpage)
    {
        if (!pfnAddPage(hpage, lParam))
        {
            DestroyPropertySheetPage(hpage);
            goto Exit;
        }
    }
    else
    {
        if (pld->cpd.lpConsole)
        {
            LocalFree( pld->cpd.lpConsole );
            pld->cpd.lpConsole = NULL;
        }
        if (pld->cpd.lpFEConsole)
        {
            LocalFree( pld->cpd.lpFEConsole );
            pld->cpd.lpFEConsole = NULL;
        }
        goto Exit;
    }

    // add font selection property sheet
    psp.dwSize      = SIZEOF( psp );
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = HINST_THISDLL;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CONSOLE_FONTDLG);
    psp.pfnDlgProc  = _FontDlgProc;
    psp.lParam      = (LPARAM)pld;

    hpage = CreatePropertySheetPage( &psp );
    if (hpage)
    {
        if (!pfnAddPage(hpage, lParam))
        {
            DestroyPropertySheetPage(hpage);
            goto Exit;
        }
    }
    else
    {
        if (pld->cpd.lpConsole)
        {
            LocalFree( pld->cpd.lpConsole );
            pld->cpd.lpConsole = NULL;
        }
        if (pld->cpd.lpFEConsole)
        {
            LocalFree( pld->cpd.lpFEConsole );
            pld->cpd.lpFEConsole = NULL;
        }
        goto Exit;
    }

    // add console size propery sheet
    psp.dwSize      = SIZEOF( psp );
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = HINST_THISDLL;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CONSOLE_SCRBUFSIZE);
    psp.pfnDlgProc  = _ScreenSizeDlgProc;
    psp.lParam      = (LPARAM)pld;

    hpage = CreatePropertySheetPage( &psp );
    if (hpage)
    {
        if (!pfnAddPage(hpage, lParam))
        {
            DestroyPropertySheetPage(hpage);
            goto Exit;
        }
    }
    else
    {
        if (pld->cpd.lpConsole)
        {
            LocalFree( pld->cpd.lpConsole );
            pld->cpd.lpConsole = NULL;
        }
        goto Exit;
    }

    // add console color propery sheet
    psp.dwSize      = SIZEOF( psp );
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = HINST_THISDLL;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CONSOLE_COLOR);
    psp.pfnDlgProc  = _ColorDlgProc;
    psp.lParam      = (LPARAM)pld;

    hpage = CreatePropertySheetPage( &psp );
    if (hpage)
    {
        if (!pfnAddPage(hpage, lParam))
        {
            DestroyPropertySheetPage(hpage);
            goto Exit;
        }
    }
    else
    {
        if (pld->cpd.lpConsole)
        {
            LocalFree( pld->cpd.lpConsole );
            pld->cpd.lpConsole = NULL;
        }
        if (pld->cpd.lpFEConsole)
        {
            LocalFree( pld->cpd.lpFEConsole );
            pld->cpd.lpFEConsole = NULL;
        }
        goto Exit;
    }

#ifdef ADVANCED_PAGE
    // add "advanced" settings propery sheet
    psp.dwSize      = SIZEOF( psp );
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = HINST_THISDLL;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CONSOLE_ADVANCED);
    psp.pfnDlgProc  = _AdvancedDlgProc;
    psp.lParam      = (LPARAM)pld;

    hpage = CreatePropertySheetPage( &psp );
    if (hpage)
    {
        if (!pfnAddPage(hpage, lParam))
        {
            DestroyPropertySheetPage(hpage);
            goto Exit;
        }
    }
    else
    {
        if (pld->cpd.lpConsole)
        {
            LocalFree( pld->cpd.lpConsole );
            pld->cpd.lpConsole = NULL;
        }
        if (pld->cpd.lpFEConsole)
        {
            LocalFree( pld->cpd.lpFEConsole );
            pld->cpd.lpFEConsole = NULL;
        }
        goto Exit;
    }
#endif


Exit:
    ;

}
