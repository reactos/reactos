/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fontdlg.dlg

Abstract:

    This module contains the code for console font dialog

Author:

    Therese Stowell (thereses) Feb-3-1992 (swiped from Win3.1)

Revision History:

--*/

#include "shellprv.h"
#pragma hdrstop

#include "lnkcon.h"

HBITMAP g_hbmTT = NULL; // handle of TT logo bitmap
BITMAP  g_bmTT;          // attributes of TT source bitmap
int g_dyFacelistItem = 0;


/* ----- Prototypes ----- */

int FontListCreate(
    LPCONSOLEPROP_DATA pcpd,
    HWND hDlg,
    LPTSTR ptszTTFace,
    BOOL bNewFaceList
    );

BOOL ConsolePreviewUpdate(
    LPCONSOLEPROP_DATA pcpd,
    HWND hDlg,
    BOOL bLB
    );

int SelectCurrentSize(
    LPCONSOLEPROP_DATA pcpd,
    HWND hDlg,
    BOOL bLB,
    int FontIndex);

BOOL ConsolePreviewInit(
    LPCONSOLEPROP_DATA pcpd,
    HWND hDlg);

VOID ConsoleDrawItemFontList(
    LPCONSOLEPROP_DATA pcpd,
    const LPDRAWITEMSTRUCT lpdis);

/* ----- Globals ----- */

const TCHAR g_szPreviewText[] = \
    TEXT("C:\\WINDOWS> dir                       \n") \
    TEXT("SYSTEM       <DIR>     10-01-99   5:00a\n") \
    TEXT("SYSTEM32     <DIR>     10-01-99   5:00a\n") \
    TEXT("README   TXT     26926 10-01-99   5:00a\n") \
    TEXT("WINDOWS  BMP     46080 10-01-99   5:00a\n") \
    TEXT("NOTEPAD  EXE    337232 10-01-99   5:00a\n") \
    TEXT("CLOCK    AVI     39594 10-01-99   5:00p\n") \
    TEXT("WIN      INI      7005 10-01-99   5:00a\n");

// Context-sensitive help ids

const static DWORD rgdwHelpFont[] = {
    IDC_CNSL_PREVIEWLABEL,  IDH_DOS_FONT_WINDOW_PREVIEW,
    IDC_CNSL_PREVIEWWINDOW, IDH_DOS_FONT_WINDOW_PREVIEW,
    IDC_CNSL_STATIC,        IDH_CONSOLE_FONT_FONT,
    IDC_CNSL_FACENAME,      IDH_CONSOLE_FONT_FONT,
    IDC_CNSL_FONTSIZE,      IDH_DOS_FONT_SIZE,
    IDC_CNSL_PIXELSLIST,    IDH_DOS_FONT_SIZE,
    IDC_CNSL_POINTSLIST,    IDH_DOS_FONT_SIZE,
    IDC_CNSL_BOLDFONT,      IDH_CONSOLE_FONT_BOLD_FONTS,
    IDC_CNSL_GROUP,         IDH_DOS_FONT_FONT_PREVIEW,
    IDC_CNSL_STATIC2,       IDH_DOS_FONT_FONT_PREVIEW,
    IDC_CNSL_STATIC3,       IDH_DOS_FONT_FONT_PREVIEW,
    IDC_CNSL_STATIC4,       IDH_DOS_FONT_FONT_PREVIEW,
    IDC_CNSL_FONTWIDTH,     IDH_DOS_FONT_FONT_PREVIEW,
    IDC_CNSL_FONTHEIGHT,    IDH_DOS_FONT_FONT_PREVIEW,
    IDC_CNSL_FONTWINDOW,    IDH_DOS_FONT_FONT_PREVIEW,
    0, 0
};

// selelct font based on the current code page
BOOL
SelectCurrentFont(
    LPCONSOLEPROP_DATA pcpd,
    HWND hDlg,
    int FontIndex
    );

// Globals strings loaded from resource
TCHAR tszSelectedFont[CCH_SELECTEDFONT+1];
TCHAR tszRasterFonts[CCH_RASTERFONTS+1];


BOOL
CALLBACK
_FontDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

    Dialog proc for the font selection dialog box.
    Returns the near offset into the far table of LOGFONT structures.

--*/

{
    HWND hWndFocus;
    HWND hWndList;
    int FontIndex;
    BOOL bLB;
    TEXTMETRIC tm;
    HDC hDC;
    LPLINKDATA pld = (LPLINKDATA)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {
    case WM_INITDIALOG:
        pld = (LPLINKDATA)((PROPSHEETPAGE *)lParam)->lParam;

        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pld);
        pld->cpd.bFontInit = FALSE;

        SendDlgItemMessage(hDlg, IDC_CNSL_PREVIEWWINDOW, CM_PREVIEW_INIT, 0, (LPARAM)&pld->cpd );
        SendDlgItemMessage(hDlg, IDC_CNSL_PREVIEWWINDOW, CM_PREVIEW_UPDATE, 0, 0 );

        /*
         * Load the font description strings
         */
        LoadString(HINST_THISDLL, IDS_CNSL_RASTERFONT,
                   tszRasterFonts, NELEM(tszRasterFonts));
        ASSERT(lstrlen(tszRasterFonts) < CCH_RASTERFONTS);

        LoadString(g_hinst, IDS_CNSL_SELECTEDFONT,
                   tszSelectedFont, NELEM(tszSelectedFont));
        ASSERT(lstrlen(tszSelectedFont) < CCH_SELECTEDFONT);

        /* Save current font size as dialog window's user data */
        if (IsFarEastCP(pld->cpd.uOEMCP)) 
        {
            // Assigning different value when we run on FarEast codepage
            pld->cpd.FontLong =
                      MAKELONG(pld->cpd.FontInfo[pld->cpd.CurrentFontIndex].tmCharSet,
                               pld->cpd.FontInfo[pld->cpd.CurrentFontIndex].Size.Y);
        }
        else
        {
            pld->cpd.FontLong =
                      MAKELONG(pld->cpd.FontInfo[pld->cpd.CurrentFontIndex].Size.X,
                               pld->cpd.FontInfo[pld->cpd.CurrentFontIndex].Size.Y);
        }

        /* Create the list of suitable fonts */
        pld->cpd.gbEnumerateFaces = TRUE;
        bLB = !TM_IS_TT_FONT(pld->cpd.lpConsole->uFontFamily);
        pld->cpd.gbBold = IS_BOLD(pld->cpd.lpConsole->uFontWeight);
        CheckDlgButton(hDlg, IDC_CNSL_BOLDFONT, pld->cpd.gbBold);
        FontListCreate(&pld->cpd, hDlg, bLB ? NULL : pld->cpd.lpFaceName, TRUE);

        /* Initialize the preview window - selects current face & size too */
        bLB = ConsolePreviewInit(&pld->cpd, hDlg);
        ConsolePreviewUpdate(&pld->cpd, hDlg, bLB);

        /* Make sure the list box has the focus */
        hWndList = GetDlgItem(hDlg, bLB ? IDC_CNSL_PIXELSLIST : IDC_CNSL_POINTSLIST);
        SetFocus(hWndList);
        pld->cpd.bFontInit = TRUE;
        break;

    case WM_FONTCHANGE:
        pld->cpd.gbEnumerateFaces = TRUE;
        bLB = !TM_IS_TT_FONT(pld->cpd.lpConsole->uFontFamily);
        FontListCreate(&pld->cpd, hDlg, NULL, TRUE);
        FontIndex = FindCreateFont(&pld->cpd,
                                   pld->cpd.lpConsole->uFontFamily,
                                   pld->cpd.lpFaceName,
                                   pld->cpd.lpConsole->dwFontSize,
                                   pld->cpd.lpConsole->uFontWeight);
        SelectCurrentSize(&pld->cpd, hDlg, bLB, FontIndex);
        return TRUE;
        
    case WM_PAINT:
        // fChangeCodePage can be TRUE only on FE codepage
        if (pld->cpd.fChangeCodePage)
        {
            pld->cpd.fChangeCodePage = FALSE;

            /* Create the list of suitable fonts */
            bLB = !TM_IS_TT_FONT(pld->cpd.lpConsole->uFontFamily);
            FontIndex = FontListCreate(&pld->cpd, hDlg, !bLB ? NULL : pld->cpd.lpFaceName, TRUE);
            FontIndex = FontListCreate(&pld->cpd, hDlg, bLB ? NULL : pld->cpd.lpFaceName, TRUE);
            pld->cpd.CurrentFontIndex = FontIndex;

            FontIndex = SelectCurrentSize(&pld->cpd, hDlg, bLB, FontIndex);
            SelectCurrentFont(&pld->cpd, hDlg, FontIndex);

            ConsolePreviewUpdate(&pld->cpd, hDlg, bLB);
        }
        break;

    case WM_HELP:               /* F1 or title-bar help button */
        WinHelp( (HWND) ((LPHELPINFO) lParam)->hItemHandle,
                 NULL,
                 HELP_WM_HELP,
                 (ULONG_PTR) (LPVOID) &rgdwHelpFont[0]
                );
        break;

    case WM_CONTEXTMENU:        /* right mouse click */
        WinHelp( (HWND) wParam,
                 NULL,
                 HELP_CONTEXTMENU,
                 (ULONG_PTR) (LPTSTR) &rgdwHelpFont[0]
                );
        break;


    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CNSL_BOLDFONT:
            pld->cpd.gbBold = IsDlgButtonChecked(hDlg, IDC_CNSL_BOLDFONT);
            pld->cpd.bConDirty = TRUE;
            goto RedoFontListAndPreview;

        case IDC_CNSL_FACENAME:
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
RedoFontListAndPreview:
                if (pld->cpd.bFontInit)
                    PropSheet_Changed( GetParent( hDlg ), hDlg );
                {
                    TCHAR atchNewFace[LF_FACESIZE];
                    LRESULT l;

                    l = SendDlgItemMessage(hDlg, IDC_CNSL_FACENAME, LB_GETCURSEL, 0, 0L);
                    bLB = (BOOL) SendDlgItemMessage(hDlg, IDC_CNSL_FACENAME, LB_GETITEMDATA, l, 0L);
                    if (!bLB) {
                        SendDlgItemMessage(hDlg, IDC_CNSL_FACENAME, LB_GETTEXT, l, (LPARAM)atchNewFace);
                    }
                    FontIndex = FontListCreate(&pld->cpd, hDlg, bLB ? NULL : atchNewFace, FALSE);
                    FontIndex = SelectCurrentSize(&pld->cpd, hDlg, bLB, FontIndex);
                    ConsolePreviewUpdate(&pld->cpd, hDlg, bLB);
                    pld->cpd.bConDirty = TRUE;
                    return TRUE;
                }
            }
            break;

        case IDC_CNSL_POINTSLIST:
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
                if (pld->cpd.bFontInit)
                    PropSheet_Changed( GetParent( hDlg ), hDlg );
                ConsolePreviewUpdate(&pld->cpd, hDlg, FALSE);
                pld->cpd.bConDirty = TRUE;
                return TRUE;

            case CBN_KILLFOCUS:
                if (!pld->cpd.gbPointSizeError) {
                    hWndFocus = GetFocus();
                    if (hWndFocus != NULL && IsChild(hDlg, hWndFocus) &&
                        hWndFocus != GetDlgItem(hDlg, IDCANCEL)) {
                        ConsolePreviewUpdate(&pld->cpd, hDlg, FALSE);
                    }
                }
                return TRUE;

            default:
                break;
            }
            break;

        case IDC_CNSL_PIXELSLIST:
            switch (HIWORD(wParam)) {
            case LBN_SELCHANGE:
                if (pld->cpd.bFontInit)
                    PropSheet_Changed( GetParent( hDlg ), hDlg );
                ConsolePreviewUpdate(&pld->cpd, hDlg, TRUE);
                pld->cpd.bConDirty = TRUE;
                return TRUE;

            default:
                break;
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
            if (FAILED(_SaveLink(pld)))
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            break;

        case PSN_KILLACTIVE:
            //
            // If the TT combo box is visible, update selection
            //
            hWndList = GetDlgItem(hDlg, IDC_CNSL_POINTSLIST);
            if (hWndList != NULL && IsWindowVisible(hWndList)) {
                if (!ConsolePreviewUpdate(&pld->cpd, hDlg, FALSE)) {
                    SetDlgMsgResult(hDlg, PSN_KILLACTIVE, TRUE);
                    return TRUE;
                }
                SetDlgMsgResult(hDlg, PSN_KILLACTIVE, FALSE);
            }

            FontIndex = pld->cpd.CurrentFontIndex;

            if (pld->cpd.FontInfo[FontIndex].SizeWant.Y == 0) {
                // Raster Font, so save actual size
                pld->cpd.lpConsole->dwFontSize = pld->cpd.FontInfo[FontIndex].Size;
            } else {
                // TT Font, so save desired size
                pld->cpd.lpConsole->dwFontSize = pld->cpd.FontInfo[FontIndex].SizeWant;
            }

            pld->cpd.lpConsole->uFontWeight = pld->cpd.FontInfo[FontIndex].Weight;
            pld->cpd.lpConsole->uFontFamily = pld->cpd.FontInfo[FontIndex].Family;
            lstrcpy(pld->cpd.lpFaceName, pld->cpd.FontInfo[FontIndex].FaceName);

            return TRUE;

        }
        break;

    /*
     *  For WM_MEASUREITEM and WM_DRAWITEM, since there is only one
     *  owner-draw item (combobox) in the entire dialog box, we don't have
     *  to do a GetDlgItem to figure out who he is.
     */
    case WM_MEASUREITEM:
        /*
         * Load the TrueType logo bitmap
         */
        if (g_hbmTT == NULL) {
            g_hbmTT = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_TRUETYPE));
            GetObject(g_hbmTT, sizeof(BITMAP), &g_bmTT);
        }

        /*
         * Compute the height of face name listbox entries
         */
        if (g_dyFacelistItem == 0) {
            hDC = GetDC(NULL);
            GetTextMetrics(hDC, &tm);
            ReleaseDC(NULL, hDC);
            g_dyFacelistItem = max(tm.tmHeight, g_bmTT.bmHeight);
        }
        ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = g_dyFacelistItem;
        return TRUE;

    case WM_DRAWITEM:
        ConsoleDrawItemFontList(&pld->cpd, (LPDRAWITEMSTRUCT)lParam);
        return TRUE;

    case WM_DESTROY:

        /*
         * Delete the TrueType logo bitmap
         */
        if (g_hbmTT != NULL) {
            DeleteObject(g_hbmTT);
            g_hbmTT = NULL;
        }
        return TRUE;

    default:
        break;
    }
    return FALSE;
}


int
FontListCreate(
    LPCONSOLEPROP_DATA pcpd,
    HWND hDlg,
    LPTSTR ptszTTFace,
    BOOL bNewFaceList
    )

/*++

    Initializes the font list by enumerating all fonts and picking the
    proper ones for our list.

    Returns
        FontIndex of selected font (LB_ERR if none)
--*/

{
    TCHAR tszText[80];
    LONG lListIndex;
    ULONG i;
    HWND hWndShow;      // List or Combo box
    HWND hWndHide;    // Combo or List box
    HWND hWndFaceCombo;
    BOOL bLB;
    int LastShowX = 0;
    int LastShowY = 0;
    int nSameSize = 0;
    UINT CodePage = pcpd->lpFEConsole->uCodePage;
    BOOL fDbcsCharSet = IS_ANY_DBCS_CHARSET( CodePageToCharSet( CodePage ) );
    BOOL fFESystem = IsFarEastCP(pcpd->uOEMCP);
    BOOL fFindTTFont = FALSE;
    LPTSTR ptszAltTTFace = NULL;
    DWORD dwExStyle = 0L;
    bLB = ((ptszTTFace == NULL) || (ptszTTFace[0] == TEXT('\0')));
    if (! bLB) {
        if (IsAvailableTTFont(pcpd, ptszTTFace)) {
            ptszAltTTFace = GetAltFaceName(pcpd, ptszTTFace);
        }
        else {
            ptszAltTTFace = ptszTTFace;
        }
    }

    /*
     * This only enumerates face names if necessary, and
     * it only enumerates font sizes if necessary
     */
    EnumerateFonts(pcpd, bLB ? EF_OEMFONT : EF_TTFONT);

    /* init the TTFaceNames */


    if (bNewFaceList) {
        PFACENODE panFace;
        hWndFaceCombo = GetDlgItem(hDlg, IDC_CNSL_FACENAME);

        SendMessage(hWndFaceCombo, LB_RESETCONTENT, 0, 0);

        lListIndex = (LONG) SendMessage(hWndFaceCombo, LB_ADDSTRING, 0, (LPARAM)tszRasterFonts);
        SendMessage(hWndFaceCombo, LB_SETITEMDATA, lListIndex, TRUE);
        for (panFace = pcpd->gpFaceNames; panFace; panFace = panFace->pNext) {
            if ((panFace->dwFlag & (EF_TTFONT|EF_NEW)) != (EF_TTFONT|EF_NEW)) {
                continue;
            }
            if (!fDbcsCharSet && (panFace->dwFlag & EF_DBCSFONT)) {
                continue;
            }

            if ( ( fDbcsCharSet && IsAvailableTTFontCP(pcpd, panFace->atch, CodePage)) ||
                 ( !fDbcsCharSet && IsAvailableTTFontCP(pcpd, panFace->atch, 0))) 
            {

                if ( !bLB &&
                     (lstrcmp(ptszTTFace, panFace->atch) == 0 ||
                      lstrcmp(ptszAltTTFace, panFace->atch) == 0)
                   )
                    fFindTTFont = TRUE;
                
                lListIndex = (LONG) SendMessage(hWndFaceCombo, LB_ADDSTRING, 0,
                                        (LPARAM)panFace->atch);
                SendMessage(hWndFaceCombo, LB_SETITEMDATA, lListIndex, FALSE);
            }
        }

        if (! bLB && ! fFindTTFont)
        {
            for (panFace = pcpd->gpFaceNames; panFace; panFace = panFace->pNext) {
                if ((panFace->dwFlag & (EF_TTFONT|EF_NEW)) != (EF_TTFONT|EF_NEW)) {
                    continue;
                }
                if ( !fDbcsCharSet && (panFace->dwFlag & EF_DBCSFONT)) {
                    continue;
                }

                if ( (  fDbcsCharSet && IsAvailableTTFontCP(pcpd, panFace->atch, CodePage)) ||
                     (! fDbcsCharSet && IsAvailableTTFontCP(pcpd, panFace->atch, 0))
                   ) 
                {

                    if (lstrcmp(ptszTTFace, panFace->atch) != 0)
                    {
                        lstrcpy(ptszTTFace, panFace->atch);
                        break;
                    }
                }
            }
        }
    } // bNewFaceList == TRUE
        
    hWndShow = GetDlgItem(hDlg, IDC_CNSL_BOLDFONT);
    
    // Disable bold font if that will be GDI simulated 
    if ( fDbcsCharSet && IsDisableBoldTTFont(pcpd, ptszTTFace) )
    {
        EnableWindow(hWndShow, FALSE);
        pcpd->gbBold = FALSE;
        CheckDlgButton(hDlg, IDC_CNSL_BOLDFONT, FALSE);
    }
    else
    {
        CheckDlgButton(hDlg, IDC_CNSL_BOLDFONT, (bLB || !pcpd->gbBold) ? FALSE : TRUE);
        EnableWindow(hWndShow, bLB ? FALSE : TRUE);
    }

    hWndHide = GetDlgItem(hDlg, bLB ? IDC_CNSL_POINTSLIST : IDC_CNSL_PIXELSLIST);
    ShowWindow(hWndHide, SW_HIDE);
    EnableWindow(hWndHide, FALSE);

    hWndShow = GetDlgItem(hDlg, bLB ? IDC_CNSL_PIXELSLIST : IDC_CNSL_POINTSLIST);
//    hStockFont = GetStockObject(SYSTEM_FIXED_FONT);
//    SendMessage(hWndShow, WM_SETFONT, (DWORD)hStockFont, FALSE);
    ShowWindow(hWndShow, SW_SHOW);
    EnableWindow(hWndShow, TRUE);

    if (bNewFaceList)
    {
        lcbRESETCONTENT(hWndShow, bLB);
    }
    dwExStyle = GetWindowLong(hWndShow, GWL_EXSTYLE);
    if(dwExStyle & RTL_MIRRORED_WINDOW)
    {
        // if mirrored RTL Reading means LTR !!
        SetWindowBits(hWndShow, GWL_EXSTYLE, WS_EX_RTLREADING, WS_EX_RTLREADING);
    }
    /* Initialize hWndShow list/combo box */

    for (i=0;i<pcpd->NumberOfFonts;i++) {
        int ShowX, ShowY;

        if (!bLB == !TM_IS_TT_FONT(pcpd->FontInfo[i].Family)) {
            continue;
        }
        
        if (fDbcsCharSet) {
            if (! IS_ANY_DBCS_CHARSET(pcpd->FontInfo[i].tmCharSet)) {
                continue;
            }
        }
        else {
            if (IS_ANY_DBCS_CHARSET(pcpd->FontInfo[i].tmCharSet)) {
                continue;
            }
        }

        if (!bLB) {
            if (lstrcmp(pcpd->FontInfo[i].FaceName, ptszTTFace) != 0 &&
                lstrcmp(pcpd->FontInfo[i].FaceName, ptszAltTTFace) != 0) {
                /*
                 * A TrueType font, but not the one we're interested in,
                 * so don't add it to the list of point sizes.
                 */
                continue;
            }
            if (pcpd->gbBold != IS_BOLD(pcpd->FontInfo[i].Weight)) {
                continue;
            }
        }

        if (pcpd->FontInfo[i].SizeWant.X > 0) {
            ShowX = pcpd->FontInfo[i].SizeWant.X;
        } else {
            ShowX = pcpd->FontInfo[i].Size.X;
        }
        if (pcpd->FontInfo[i].SizeWant.Y > 0) {
            ShowY = pcpd->FontInfo[i].SizeWant.Y;
        } else {
            ShowY = pcpd->FontInfo[i].Size.Y;
        }
        /*
         * Add the size description string to the end of the right list
         */
        if (TM_IS_TT_FONT(pcpd->FontInfo[i].Family)) {
            // point size
            wsprintf(tszText, TEXT("%2d"), pcpd->FontInfo[i].SizeWant.Y);
        } else {
            // pixel size
            if ((LastShowX == ShowX) && (LastShowY == ShowY)) {
                nSameSize++;
            } else {
                LastShowX = ShowX;
                LastShowY = ShowY;
                nSameSize = 0;
            }

            /*
             * The number nSameSize is appended to the string to distinguish
             * between Raster fonts of the same size.  It is not intended to
             * be visible and exists off the edge of the list
             */

            if(((dwExStyle & WS_EX_RIGHT) && !(dwExStyle & RTL_MIRRORED_WINDOW)) 
                || (!(dwExStyle & WS_EX_RIGHT) && (dwExStyle & RTL_MIRRORED_WINDOW))) {
                // flip  it so that the hidden part be at the far left
                wsprintf(tszText, TEXT("#%d                %2d x %2d"),
                         nSameSize, ShowX, ShowY);                            
            } else {
                wsprintf(tszText, TEXT("%2d x %2d                #%d"),
                         ShowX, ShowY, nSameSize);            
            }
        }
        lListIndex = (LONG) lcbFINDSTRINGEXACT(hWndShow, bLB, tszText);
        if (lListIndex == LB_ERR) {
            lListIndex = (LONG) lcbADDSTRING(hWndShow, bLB, tszText);
        }
        lcbSETITEMDATA(hWndShow, bLB, (DWORD)lListIndex, i);
    }

    /*
     * Get the FontIndex from the currently selected item.
     * (i will be LB_ERR if no currently selected item).
     */
    lListIndex = (LONG) lcbGETCURSEL(hWndShow, bLB);
    i = (int) lcbGETITEMDATA(hWndShow, bLB, lListIndex);

    return i;
}


/** ConsoleDrawItemFontList
 *
 *  Answer the WM_DRAWITEM message sent from the font list box or
 *  facename list box.
 *
 *  Entry:
 *      lpdis     -> DRAWITEMSTRUCT describing object to be drawn
 *
 *  Returns:
 *      None.
 *
 *      The object is drawn.
 */
VOID WINAPI
ConsoleDrawItemFontList(LPCONSOLEPROP_DATA pcpd, const LPDRAWITEMSTRUCT lpdis)
{
    HDC     hDC, hdcMem;
    DWORD   rgbBack, rgbText, rgbFill;
    TCHAR   tszFace[LF_FACESIZE];
    HBITMAP hOld;
    int     dy;
    HBRUSH  hbrFill;
    HWND    hWndItem;
    BOOL    bLB;
    int     dxttbmp;

    if ((int)lpdis->itemID < 0)
        return;

    hDC = lpdis->hDC;

    if (lpdis->itemAction & ODA_FOCUS) {
        if (lpdis->itemState & ODS_SELECTED) {
            DrawFocusRect(hDC, &lpdis->rcItem);
        }
    } else {
        if (lpdis->itemState & ODS_SELECTED) {
            rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            rgbBack = SetBkColor(hDC, rgbFill = GetSysColor(COLOR_HIGHLIGHT));
        } else {
            rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
            rgbBack = SetBkColor(hDC, rgbFill = GetSysColor(COLOR_WINDOW));
        }
        // draw selection background
        hbrFill = CreateSolidBrush(rgbFill);
        if (hbrFill) {
            FillRect(hDC, &lpdis->rcItem, hbrFill);
            DeleteObject(hbrFill);
        }

        // get the string
        if (IsWindow(hWndItem = lpdis->hwndItem) == FALSE) {
            return;
        }
        SendMessage(hWndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)tszFace);
        bLB = (BOOL) SendMessage(hWndItem, LB_GETITEMDATA, lpdis->itemID, 0L);
        dxttbmp = bLB ? 0 : g_bmTT.bmWidth;


        // draw the text
        TabbedTextOut(hDC, lpdis->rcItem.left + dxttbmp,
                      lpdis->rcItem.top, tszFace,
                      lstrlen(tszFace), 0, NULL, dxttbmp);

        // and the TT bitmap if needed
        if (!bLB) {
            hdcMem = CreateCompatibleDC(hDC);
            if (hdcMem) {
                hOld = SelectObject(hdcMem, g_hbmTT);

                dy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - g_bmTT.bmHeight) / 2;

                BitBlt(hDC, lpdis->rcItem.left, lpdis->rcItem.top + dy,
                       dxttbmp, g_dyFacelistItem, hdcMem,
                       0, 0, SRCINVERT);

                if (hOld)
                    SelectObject(hdcMem, hOld);
                DeleteDC(hdcMem);
            }
        }

        SetTextColor(hDC, rgbText);
        SetBkColor(hDC, rgbBack);

        if (lpdis->itemState & ODS_FOCUS) {
            DrawFocusRect(hDC, &lpdis->rcItem);
        }
    }
}


UINT
GetPointSizeInRange(
   HWND hDlg,
   INT Min,
   INT Max)
/*++

Routine Description:

   Get a size from the Point Size ComboBox edit field

Return Value:

   Point Size - of the edit field limited by Min/Max size
   0 - if the field is empty or invalid

--*/

{
    TCHAR szBuf[90];
    int nTmp = 0;
    BOOL bOK;

    if (GetDlgItemText(hDlg, IDC_CNSL_POINTSLIST, szBuf, NELEM(szBuf))) {
        nTmp = GetDlgItemInt(hDlg, IDC_CNSL_POINTSLIST, &bOK, TRUE);
        if (bOK && nTmp >= Min && nTmp <= Max) {
            return nTmp;
        }
    }

    return 0;
}


/* ----- Preview routines ----- */

LRESULT
_FontPreviewWndProc(
    HWND hWnd,
    UINT wMessage,
    WPARAM wParam,
    LPARAM lParam
    )

/*  FontPreviewWndProc
 *      Handles the font preview window
 */

{
    PAINTSTRUCT ps;
    RECT rect;
    HBRUSH hbrClient;
    HBRUSH hbrOld;
    COLORREF rgbText;
    COLORREF rgbBk;

    LPCONSOLEPROP_DATA pcpd = (LPCONSOLEPROP_DATA)GetWindowLongPtr( hWnd, GWLP_USERDATA );

    switch (wMessage) {
    case WM_CREATE:
        pcpd = (LPCONSOLEPROP_DATA)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr( hWnd, GWLP_USERDATA, (LPARAM)pcpd );
        break;

    case WM_PAINT:
        BeginPaint(hWnd, &ps);

        /* Draw the font sample */
        rgbText = GetNearestColor(ps.hdc, ScreenTextColor(pcpd));
        rgbBk = GetNearestColor(ps.hdc, ScreenBkColor(pcpd));
        SelectObject(ps.hdc, pcpd->FontInfo[pcpd->CurrentFontIndex].hFont);
        SetTextColor(ps.hdc, rgbText);
        SetBkColor(ps.hdc, rgbBk);
        GetClientRect(hWnd, &rect);
        InflateRect(&rect, -2, -2);
        hbrClient = CreateSolidBrush(rgbBk);
        hbrOld = SelectObject(ps.hdc, hbrClient);
        PatBlt(ps.hdc, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                PATCOPY);
        DrawText(ps.hdc, g_szPreviewText, -1, &rect, 0);
        SelectObject(ps.hdc, hbrOld);
        DeleteObject(hbrClient);

        EndPaint(hWnd, &ps);
        break;

    default:
        return DefWindowProc(hWnd, wMessage, wParam, lParam);
    }
    return 0L;
}



/*
 * SelectCurrentSize - Select the right line of the Size listbox/combobox.
 *   bLB       : Size controls is a listbox (TRUE for RasterFonts)
 *   FontIndex : Index into FontInfo[] cache
 *               If < 0 then choose a good font.
 * Returns
 *   FontIndex : Index into FontInfo[] cache
 */
int
SelectCurrentSize(LPCONSOLEPROP_DATA pcpd, HWND hDlg, BOOL bLB, int FontIndex)
{
    int iCB;
    HWND hWndList;


    hWndList = GetDlgItem(hDlg, bLB ? IDC_CNSL_PIXELSLIST : IDC_CNSL_POINTSLIST);
    iCB = (int) lcbGETCOUNT(hWndList, bLB);

    if (FontIndex >= 0) {
        /*
         * look for FontIndex
         */
        while (iCB > 0) {
            iCB--;
            if (lcbGETITEMDATA(hWndList, bLB, iCB) == FontIndex) {
                lcbSETCURSEL(hWndList, bLB, iCB);
                break;
            }
        }
    } else {
        /*
         * look for a reasonable default size: looking backwards, find
         * the first one same height or smaller.
         */
        DWORD Size;
        Size = pcpd->FontLong;
        if (IsFarEastCP(pcpd->uOEMCP) & bLB
            && (pcpd->FontInfo[pcpd->CurrentFontIndex].tmCharSet != LOBYTE(LOWORD(Size)))
           )
        {
            TCHAR AltFaceName[LF_FACESIZE];
            COORD AltFontSize;
            BYTE  AltFontFamily;
            ULONG AltFontIndex = 0;

            MakeAltRasterFont(pcpd, pcpd->lpFEConsole->uCodePage, &AltFontSize, &AltFontFamily, &AltFontIndex, AltFaceName);

            while(iCB > 0) {
                iCB--;
                if (lcbGETITEMDATA(hWndList, bLB, iCB) == (int)AltFontIndex) {
                    lcbSETCURSEL(hWndList, bLB, iCB);
                    break;
                }
            }
        }
        else
        {
            while (iCB > 0) {
                iCB--;
                FontIndex = (ULONG) lcbGETITEMDATA(hWndList, bLB, iCB);
                if (pcpd->FontInfo[FontIndex].Size.Y <= HIWORD(Size)) {
                    lcbSETCURSEL(hWndList, bLB, iCB);
                    break;
                }
            }
        }
    }
    return FontIndex;
}


BOOL
SelectCurrentFont(LPCONSOLEPROP_DATA pcpd, HWND hDlg, int FontIndex)
{
    BOOL bLB;


    bLB = !TM_IS_TT_FONT(pcpd->FontInfo[FontIndex].Family);

    SendDlgItemMessage(hDlg, IDC_CNSL_FACENAME, LB_SELECTSTRING, (DWORD)-1,
            bLB ? (LPARAM)tszRasterFonts : (LPARAM)(pcpd->FontInfo[FontIndex].FaceName));

    SelectCurrentSize(pcpd, hDlg, bLB, FontIndex);
    return bLB;
}


BOOL
ConsolePreviewInit(
    LPCONSOLEPROP_DATA pcpd,
    HWND hDlg
    )

/*  PreviewInit
 *      Prepares the preview code, sizing the window and the dialog to
 *      make an attractive preview.
 *  Returns TRUE if Raster Fonts, FALSE if TT Font
 */

{
    HDC hDC;
    TEXTMETRIC tm;
    RECT rectLabel;
    RECT rectGroup;
    int nFont;
    SHORT xChar;
    SHORT yChar;


    /* Get the system char size */
    hDC = GetDC(hDlg);
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hDlg, hDC);
    xChar = (SHORT) (tm.tmAveCharWidth);
    yChar = (SHORT) (tm.tmHeight + tm.tmExternalLeading);

    /* Compute the size of the font preview */
    GetWindowRect(GetDlgItem(hDlg, IDC_CNSL_GROUP), &rectGroup);
    MapWindowRect(HWND_DESKTOP, hDlg, &rectGroup);
    rectGroup.bottom -= rectGroup.top;    
    GetWindowRect(GetDlgItem(hDlg, IDC_CNSL_STATIC2), &rectLabel);
    MapWindowRect(HWND_DESKTOP, hDlg, &rectLabel);


    /* Create the font preview */
    CreateWindowEx(0L, TEXT("WOACnslFontPreview"), NULL,
        WS_CHILD | WS_VISIBLE,
        rectGroup.left + xChar, rectGroup.top + 3 * yChar / 2,
        rectLabel.left - rectGroup.left - 2 * xChar,
        rectGroup.bottom -  2 * yChar,
        hDlg, (HMENU)IDC_CNSL_FONTWINDOW, g_hinst, (LPVOID)pcpd);

    /*
     * Set the current font
     */
    nFont = FindCreateFont(pcpd,
                           pcpd->lpConsole->uFontFamily,
                           pcpd->lpFaceName,
                           pcpd->lpConsole->dwFontSize,
                           pcpd->lpConsole->uFontWeight);

    pcpd->CurrentFontIndex = nFont;

    return SelectCurrentFont(pcpd, hDlg, nFont);
}


BOOL
ConsolePreviewUpdate(
    LPCONSOLEPROP_DATA pcpd,
    HWND hDlg,
    BOOL bLB
    )

/*++

    Does the preview of the selected font.

--*/

{
    PFONT_INFO lpFont;
    int FontIndex;
    LONG lIndex;
    HWND hWnd;
    TCHAR tszText[60];
    TCHAR tszFace[LF_FACESIZE + CCH_SELECTEDFONT];
    HWND hWndList;


    hWndList = GetDlgItem(hDlg, bLB ? IDC_CNSL_PIXELSLIST : IDC_CNSL_POINTSLIST);

    /* When we select a font, we do the font preview by setting it into
     *  the appropriate list box
     */
    lIndex = (LONG) lcbGETCURSEL(hWndList, bLB);
    if ((lIndex < 0) && !bLB) {
        COORD NewSize;

        lIndex = (LONG) SendDlgItemMessage(hDlg, IDC_CNSL_FACENAME, LB_GETCURSEL, 0, 0L);
        SendDlgItemMessage(hDlg, IDC_CNSL_FACENAME, LB_GETTEXT, lIndex, (LPARAM)tszFace);
        NewSize.X = 0;
        NewSize.Y = (SHORT) GetPointSizeInRange(hDlg, MIN_PIXEL_HEIGHT, MAX_PIXEL_HEIGHT);

        if (NewSize.Y == 0) {
            TCHAR tszBuf[60];
            /*
             * Use tszText, tszBuf to put up an error msg for bad point size
             */
            pcpd->gbPointSizeError = TRUE;
            GetWindowText(hDlg, tszBuf, NELEM(tszBuf));
            ShellMessageBox(HINST_THISDLL, hDlg, MAKEINTRESOURCE(IDS_CNSL_FONTSIZE),
                                tszBuf, MB_OK|MB_ICONINFORMATION,
                                MIN_PIXEL_HEIGHT, MAX_PIXEL_HEIGHT);
            SetFocus(hWndList);
            pcpd->gbPointSizeError = FALSE;
            return FALSE;
        }
        FontIndex = FindCreateFont(pcpd,
                                   FF_MODERN|TMPF_VECTOR|TMPF_TRUETYPE,
                                   tszFace, NewSize, 0);
    } else {
        FontIndex = (int) lcbGETITEMDATA(hWndList, bLB, lIndex);
    }

    if (FontIndex < 0) {
        FontIndex = pcpd->DefaultFontIndex;
    }

    /*
     * If we've selected a new font, tell the property sheet we've changed
     */
    if (pcpd->CurrentFontIndex != (ULONG)FontIndex) {
        pcpd->CurrentFontIndex = FontIndex;
    }

    lpFont = &pcpd->FontInfo[FontIndex];

    /* Display the new font */

    lstrcpy(tszFace, tszSelectedFont);
    lstrcat(tszFace, lpFont->FaceName);
    SetDlgItemText(hDlg, IDC_CNSL_GROUP, tszFace);

    /* Put the font size in the static boxes */
    wsprintf(tszText, TEXT("%u"), lpFont->Size.X);
    hWnd = GetDlgItem(hDlg, IDC_CNSL_FONTWIDTH);
    SetWindowText(hWnd, tszText);
    InvalidateRect(hWnd, NULL, TRUE);
    wsprintf(tszText, TEXT("%u"), lpFont->Size.Y);
    hWnd = GetDlgItem(hDlg, IDC_CNSL_FONTHEIGHT);
    SetWindowText(hWnd, tszText);
    InvalidateRect(hWnd, NULL, TRUE);

    /* Force the preview windows to repaint */
    hWnd = GetDlgItem(hDlg, IDC_CNSL_PREVIEWWINDOW);
    SendMessage(hWnd, CM_PREVIEW_UPDATE, 0, 0);
    hWnd = GetDlgItem(hDlg, IDC_CNSL_FONTWINDOW);
    InvalidateRect(hWnd, NULL, TRUE);

    return TRUE;
}
