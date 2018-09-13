/** FILE: color.c ********** Module Header ********************************
 *
 *    Control panel applet for Color configuration.  This file holds
 *  definitions and other common include file items that deal with custom
 *  colors and color schemes.  The main Color Dialog procedure is in this
 *    file.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Dave Snipp   [davesn]
 *        Took base code from Win 3.1 source
 *  15:40 on Wed   12 Jun 1991  -by-  Steve Cathcart   [stevecat]
 *        Further cleanup for NT control panel
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *
 *  Copyright (C) 1990-1992 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <string.h>
#include <stdlib.h>

// Windows SDK
#define OEMRESOURCE
#define NOMINMAX

// Application specific
#include "main.h"

//==========================================================================
//                            Local Definitions
//==========================================================================
#define  UPARROW          1
#define  DOWNARROW        0

#define MAX_SCHEMESIZE  190


#define NUM_X_BOXES       8

#define BOX_X_MARGIN      5
#define BOX_Y_MARGIN      5


//==========================================================================
//                            External Declarations
//==========================================================================
/* Functions */

/* Data */

//==========================================================================
//                            Data Declarations
//==========================================================================
BOOL    bTuning;
HRGN    hIconRgn = NULL;
HWND    hBox1;
HWND    hCustom1;
HWND    hSave;
DWORD   nCurDsp;
DWORD   nCurMix;
DWORD   nCurBox;
RECT    rColorBox[COLORBOXES];
BOOL    bMouseCapture;
RECT    rSamples;
RECT    rSamplesCapture;
DWORD   nElementIndex;
DWORD   nBoxHeight;
DWORD   nBoxWidth;
DWORD   nDriverColors;              // Does WIN32 Display Drivers have colors ?
HWND    hRainbowDlg;
DWORD   rainbowRGB;
DWORD   rgbBoxColor[COLORBOXES];
DWORD   lCurColors[COLORDEFS];      // current test colors
DWORD   currentRGB;
DWORD   dwContext;

extern HWND hwndColor;

WORD ElementLBItems[COLORDEFS] = {
  BACKGROUND,
  MDIWINDOW,
  CLIENT,
  CLIENTTEXT,
  MENUBAR,
  MENUTEXT,
  MYCAPTION,
  CAPTION2,
  CAPTIONTEXT,
  CAPTION2TEXT,
  BORDER,
  BORDER2,
  WINDOWFRAME,
  SCROLLBARS,
  BUTTONFACE,
  BUTTONSHADOW,
  BUTTONTEXT,
  BUTTONHIGHLIGHT,
  GRAYTEXT,
  HIGHLIGHT,
  HIGHLIGHTTEXT,
} ;

int     Xlat[COLORDEFS] = {  /* translates from combobox itemdata to system */
                         COLOR_BACKGROUND,
                         COLOR_APPWORKSPACE,
                         COLOR_WINDOW,
                         COLOR_WINDOWTEXT,
                         COLOR_MENU,
                         COLOR_MENUTEXT,
                         COLOR_ACTIVECAPTION,
                         COLOR_INACTIVECAPTION,
                         COLOR_CAPTIONTEXT,
                         COLOR_ACTIVEBORDER,
                         COLOR_INACTIVEBORDER,
                         COLOR_WINDOWFRAME,
                         COLOR_SCROLLBAR,
                         COLOR_BTNFACE,
                         COLOR_BTNSHADOW,
                         COLOR_BTNTEXT,
                         COLOR_GRAYTEXT,
                         COLOR_HIGHLIGHT,
                         COLOR_HIGHLIGHTTEXT,
                         COLOR_INACTIVECAPTIONTEXT,
                         COLOR_BTNHIGHLIGHT
                        };

TCHAR    szCurrent[]      = TEXT("current");
TCHAR    szColorSchemes[] = TEXT("color schemes");
TCHAR    szColorA[]       = TEXT("ColorA");
TCHAR    szCustomColors[] = TEXT("Custom Colors");
TCHAR    szEqual[]        = TEXT("=");
TCHAR    szDisplay[]      = TEXT("DISPLAY");
TCHAR    szOEMBIN[]       = TEXT("OEMBIN");    // Resource from display driver
TCHAR    szSchemeName[MAX_SCHEMESIZE];
BYTE    fChanged;
RECT    Orig;
RECT    rSamples, rSamplesCapture;
RECT    rBorderLeft, rBorderTop, rBorderRight, rBorderBottom;
RECT    rBorderLeftFrame, rBorderTopFrame;
RECT    rBorderRightFrame, rBorderBottomFrame;
RECT    rBorderOutline, rBorderInterior;
RECT    rBorderOutline2, rBorderInterior2;
RECT    rBorderLeft2, rBorderTop2;
RECT    rBorderRight2, rBorderBottom2;
RECT    rBorderTopFrame2, rBorderRightFrame2;
RECT    rBorderLeftFrame2, rBorderBottomFrame2;
RECT    rCaptionLeft, rCaptionText, rCaptionRight;
RECT    rCaptionLeft2, rCaptionText2, rCaptionRight2;
RECT    rMenuBar, rMenuFrame, rMenuBar2, rMenuFrame2, rMenuText;
RECT    rUpArrow, rScroll, rDownArrow;
RECT    rScrollFrame;
RECT    rMDIWindow2;
RECT    rMDIWindow, rClient, rClientFrame, rClientText;
RECT    rButton;
RECT    rPullDown,rPullInside,rGrayText,rHighlight;
int     cyCaption, cyBorder, cyIcon, cyMenu, cyVScroll, cyVThumb;
int     cxVScroll, cxBorder, cxSize;
TCHAR    szActive[40], szInactive[40], szMenu[40];
TCHAR    szWindow[40],szGrayText[40], szHighlightText[40];
DWORD   CharHeight, CharWidth;
DWORD   CharExternalLeading, CharDescent;
POINT   ptMenuText, ptTitleText, ptTitleText2;
WNDPROC lpprocStatic;
DWORD   lPrevColors[COLORDEFS];         /* system colors, RGB, on entry */
DWORD   rgbBoxColor[COLORBOXES];
DWORD   rgbBoxColorDefault[COLORBOXES] = {
 0x8080FF, 0x80FFFF, 0x80FF80, 0x80FF00, 0xFFFF80, 0xFF8000, 0xC080FF, 0xFF80FF,
 0x0000FF, 0x00FFFF, 0x00FF80, 0x40FF00, 0xFFFF00, 0xC08000, 0xC08080, 0xFF00FF,
 0x404080, 0x4080FF, 0x00FF00, 0x808000, 0x804000, 0xFF8080, 0x400080, 0x8000FF,
 0x000080, 0x0080FF, 0x008000, 0x408000, 0xFF0000, 0xA00000, 0x800080, 0xFF0080,
 0x000040, 0x004080, 0x004000, 0x404000, 0x800000, 0x400000, 0x400040, 0x800040,
 0x000000, 0x008080, 0x408080, 0x808080, 0x808040, 0xC0C0C0, 0x400040, 0xFFFFFF,
 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF
 };
HBITMAP hUpArrow, hDownArrow;
HDC     hDCBits;

TCHAR   *pszWinStrings[COLORDEFS] = {TEXT("Background"),
                                    TEXT("AppWorkspace"),
                                    TEXT("Window"),
                                    TEXT("WindowText"),
                                    TEXT("Menu"),
                                    TEXT("MenuText"),
                                    TEXT("ActiveTitle"),
                                    TEXT("InactiveTitle"),
                                    TEXT("TitleText"),
                                    TEXT("ActiveBorder"),
                                    TEXT("InactiveBorder"),
                                    TEXT("WindowFrame"),
                                    TEXT("Scrollbar"),
                                    TEXT("ButtonFace"),
                                    TEXT("ButtonShadow"),
                                    TEXT("ButtonText"),
                                    TEXT("GrayText"),
                                    TEXT("Hilight"),
                                    TEXT("HilightText"),
                                    TEXT("InactiveTitleText"),
                                    TEXT("ButtonHilight")
                                    };

short   H,L,S;                         // Hue, Lightness, Saturation
WORD    currentHue;
WORD    currentSat;
WORD    currentLum;
WORD    nHuePos, nSatPos, nLumPos;
WORD    nHueWidth, nSatHeight, nLumHeight;
RECT    rLumPaint;
RECT    rColorSamples;
RECT    rLumScroll;
RECT    rLumCapture;
RECT    rLumPaint;
RECT    rLumScroll;
RECT    rRainbow;
RECT    rRainbowCapture;
HBITMAP hRainbowBitmap;
HWND    hHSLRGB[6];
RECT    rCurrentColor;
RECT    rNearestPure;


//==========================================================================
//                            Local Function Prototypes
//==========================================================================

//==========================================================================
//                                Functions
//==========================================================================

#ifdef JAPAN // InitColorStringTable()

VOID
InitColorStringTable
(
    VOID
)
{
    INT ii;

    for( ii = 0 ; ii < COLORDEFS ; ii++ )
    {
        LoadString( hModule , COLOR_DESCRIPTION + ii , szWinStrings[ii] , 50 );
        pszWinStrings[ii] = szWinStrings[ii];
    }
}

#endif // JAPAN

BOOL APIENTRY ColorDlg (HWND hWnd, UINT message, DWORD wParam, LONG lParam)
{
    BOOL   bTemp;
    DWORD  temp;
    TCHAR  szTemp[81];
    TCHAR  szNew[MAX_SCHEMESIZE];
    LPTSTR pszScan;
    PAINTSTRUCT ps;
    HDC    hDC;
    TEXTMETRIC TM;
    RECT   Rect;
    DWORD  id;
    DWORD  nNewIndex;
    BOOL   bUpdateExample = FALSE;
    HWND   hPointWnd;
    DWORD  dwSave;
    POINT  Point;
    HWND   thWnd;

    switch (message)
    {

    case WM_INITDIALOG:

        HourGlass (TRUE);      // change cursor to hourglass

#ifdef JAPAN // Load Color string table
        InitColorStringTable();
#endif // JAPAN

        bTemp = InitColor (hWnd);
        bTuning = FALSE;

        HourGlass (FALSE);     // change cursor back to arrow

        if (!bTemp)
            return (FALSE);

        break;

    case WM_MOVE:

        DeleteObject (hIconRgn);
        hIconRgn = NULL;
        SetupScreenDiagram (hWnd);
        break;

    case CP_SETFOCUS:    // Setup calls us with these messages ???

        if ((HWND)lParam == hBox1)

            temp = nCurDsp;

        else if ((HWND)lParam == hCustom1)

            temp = nCurMix;

        else

            return (FALSE);

        hDC = GetDC (hWnd);

        CopyRect (&Rect, rColorBox + temp);
        InflateRect (&Rect, 3, 3);
        DrawFocusRect (hDC, &Rect);

        HiLiteBox (hDC, temp, (nCurBox == temp) ? 3 : 2);
        ReleaseDC (hWnd, hDC);

        break;

    case CP_KILLFOCUS:

        if ((HWND)lParam == hBox1)

            temp = nCurDsp;

        else if ((HWND)lParam == hCustom1)

            temp = nCurMix;

        else

            return (FALSE);

        hDC = GetDC (hWnd);

        CopyRect (&Rect, rColorBox + temp);
        InflateRect (&Rect, 3, 3);
        DrawFocusRect (hDC, &Rect);

        HiLiteBox (hDC, temp, (nCurBox == temp) ? 1 : 0);
        ReleaseDC (hWnd, hDC);
        break;

    case WM_RBUTTONDOWN:
        break;

    // Dialog Boxes don't receive MOUSEMOVE unless mouse is captured
    case WM_MOUSEMOVE:

        if (!bMouseCapture)    // if mouse isn't captured, break

            break;

    case WM_LBUTTONDOWN:

    /* The double click case added by C. Stevens Sept 90 to allow
         button and menu items to toggle correctly */

    case WM_LBUTTONDBLCLK:

        Point.x = (int) LOWORD (lParam);
        Point.y = (int) HIWORD (lParam);

        if (PtInRect (&rSamples, Point))
        {
            if (!bTuning)
            {
                return (FALSE);
            }

            nNewIndex = ElementFromPt (Point);

            if (nNewIndex != nElementIndex)
            {
                nElementIndex = nNewIndex;

                for (nNewIndex = 0; nNewIndex < COLORDEFS-1; ++nNewIndex)
                    if (ElementLBItems[nNewIndex] == (WORD)nElementIndex)
                        break;

                SendDlgItemMessage (hWnd, COLOR_ELEMENT, CB_SETCURSEL,
                                                                nNewIndex, 0L);
            }

            if (message == WM_LBUTTONDOWN)
            {
                RetractComboBox (hWnd);
            }
            SetCapture (hWnd);
            ClipCursor ( &rSamplesCapture);
            bMouseCapture = TRUE;
        }
        else
        {
            if (LOWORD (lParam) < (WORD) rColorBox[0].left)
                return (TRUE);

            hPointWnd = ChildWindowFromPoint (hWnd, Point);

            if (hPointWnd == hBox1)
            {
                Rect.top = rColorBox[0].top;
                Rect.left = rColorBox[0].left;
                Rect.right = rColorBox[COLOR_CUSTOM1 - COLOR_BOX1 - 1].right
                     + BOX_X_MARGIN;
                Rect.bottom = rColorBox[COLOR_CUSTOM1 - COLOR_BOX1 - 1].bottom
                     + BOX_Y_MARGIN;
                temp = (COLOR_CUSTOM1 - COLOR_BOX1) / NUM_X_BOXES;
                id = 0;
            }
            else if (hPointWnd == hCustom1)
            {
                Rect.top = rColorBox[COLOR_CUSTOM1 - COLOR_BOX1].top;
                Rect.left = rColorBox[COLOR_CUSTOM1 - COLOR_BOX1].left;
                Rect.right = rColorBox[COLORBOXES - 1].right + BOX_X_MARGIN;
                Rect.bottom = rColorBox[COLORBOXES - 1].bottom + BOX_Y_MARGIN;
                temp = (COLOR_CUSTOM16 - COLOR_CUSTOM1 + 1) / NUM_X_BOXES;
                id = COLOR_CUSTOM1 - COLOR_BOX1;
            }
            else
                return (FALSE);

            if (hPointWnd != GetFocus ())
                SetFocus (hPointWnd);

            // We make this one extra check because the window is larger than the set of
            // available colors, so we just make sure the point is in the expected set.

            if (!PtInRect ( &Rect, Point))
                return (TRUE);

            if (((DWORD)(Point.x - Rect.left) % nBoxWidth) >= (nBoxWidth - BOX_X_MARGIN))
                break;

            if (((DWORD)(Point.y - Rect.top) % nBoxHeight) >= (nBoxHeight - BOX_Y_MARGIN))
                break;

            id += ((Point.y - Rect.top) * temp / (Rect.bottom - Rect.top)) *
                NUM_X_BOXES;

            id += (Point.x - Rect.left) * NUM_X_BOXES / (Rect.right - Rect.left);

            if ((id < nDriverColors) || (id >= COLOR_CUSTOM1 - COLOR_BOX1))
            {
                if (hRainbowDlg && (wParam & MK_CONTROL))
                {
                    ChangeColorSettings (hRainbowDlg, rainbowRGB = rgbBoxColor[id]);
                    SetHLSEdit (0);
                    SetRGBEdit (0);
                    bUpdateExample = FALSE;
                }
                else if (wParam & MK_SHIFT)
                {
                    ChangeBoxFocus (hWnd, id);
                    bUpdateExample = TRUE;
                }
                else
                {
                    ChangeBoxSelection (hWnd, id);
                    nCurBox = id;
                    ChangeBoxFocus (hWnd, id);
                    if (id >= COLOR_CUSTOM1 - COLOR_BOX1)
                        nCurMix = nCurBox;
                    else
                        nCurDsp = nCurBox;
                    currentRGB = rgbBoxColor[nCurBox];
                    bUpdateExample = TRUE;
                }

                hDC = GetDC (hWnd);
                PaintBox (hDC, nCurDsp);
                PaintBox (hDC, nCurMix);
                ReleaseDC (hWnd, hDC);
            }
        }
        break;

    case WM_LBUTTONUP:

        if (bMouseCapture)
        {
            bMouseCapture = FALSE;
            ReleaseCapture ();
            ClipCursor (NULL);

            Point.x = (int) LOWORD (lParam);
            Point.y = (int) HIWORD (lParam);

            if (PtInRect (&rSamples, Point))
            {
                ChangeColorBox (hWnd, currentRGB = lCurColors[nElementIndex]);
            }
        }

        break;

    case WM_CHAR:

        if (wParam == VK_SPACE)
        {
            if (GetFocus () == hBox1)

                temp = nCurDsp;

            else if (GetFocus () == hCustom1)

                temp = nCurMix;

            else

                return (FALSE);

            if (hRainbowDlg && (GetKeyState (VK_CONTROL) & 0x80))
            {
                ChangeColorSettings (hRainbowDlg, rainbowRGB = rgbBoxColor[temp]);
                bUpdateExample = FALSE;
            }
            else
            {
                ChangeBoxSelection (hWnd, temp);
                nCurBox = temp;
                bUpdateExample = TRUE;
            }
        }

        break;

    case WM_KEYDOWN:

        if (ColorKeyDown (wParam, &temp))
        {
            ChangeBoxFocus (hWnd, temp);
        }

        break;

    case WM_SETFONT:

        break;

    case WM_MEASUREITEM:

        hDC = GetDC (hWnd);
        GetTextMetrics (hDC, (LPTEXTMETRIC) &TM);
        ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = (WORD) TM.tmHeight;
        ReleaseDC (hWnd, hDC);

        break;

    case WM_DRAWITEM:

        if ((((LPDRAWITEMSTRUCT)lParam)->CtlID >= COLOR_BOX1) &&
            (((LPDRAWITEMSTRUCT)lParam)->CtlID - COLOR_BOX1 < COLORBOXES))

            return (BoxDrawItem ((LPDRAWITEMSTRUCT)lParam));

        else

            return (ComboDrawItem ((LPDRAWITEMSTRUCT)lParam));

        break;

    case WM_USER + 0x400:

        rgbBoxColor[nCurMix] = lParam;
        InvalidateRect (hWnd,  rColorBox + nCurMix, FALSE);

        // Increment the nCurBox VERTICALLY!  Foolish extra code for vertical
        // instead of horizontal increment

        if (nCurMix >= COLORBOXES - 1)

            nNewIndex = COLOR_CUSTOM1 - COLOR_BOX1;

        else if (nCurMix > (COLOR_CUSTOM8 - COLOR_BOX1))

            nNewIndex = nCurMix - 7;

        else

            nNewIndex = nCurMix + 8;

//    ChangeBoxFocus (hWnd, nNewIndex);

        nCurMix = nNewIndex;
        break;

    case WM_GETDLGCODE:

        return (DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_HASSETSEL);

        break;

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {

        case IDD_HELP:

            goto DoHelp;

        case IDOK:

            HourGlass (TRUE);

            hDC = GetDC (hWnd);
            lCurColors[MENUBAR] = GetNearestColor (hDC, lCurColors[MENUBAR]);
            lCurColors[MENUTEXT] = GetNearestColor (hDC, lCurColors[MENUTEXT]);
            lCurColors[CAPTIONTEXT] = GetNearestColor (hDC, lCurColors[CAPTIONTEXT]);
            lCurColors[CLIENT] = GetNearestColor (hDC, lCurColors[CLIENT]);
            lCurColors[CLIENTTEXT] = GetNearestColor (hDC, lCurColors[CLIENTTEXT]);
            lCurColors[WINDOWFRAME] = GetNearestColor (hDC, lCurColors[WINDOWFRAME]);
            lCurColors[GRAYTEXT] = GetNearestColor (hDC, lCurColors[GRAYTEXT]);
            lCurColors[HIGHLIGHT] = GetNearestColor (hDC, lCurColors[HIGHLIGHT]);
            lCurColors[HIGHLIGHTTEXT] = GetNearestColor (hDC, lCurColors[HIGHLIGHTTEXT]);
            lCurColors[BUTTONFACE] = GetNearestColor (hDC, lCurColors[BUTTONFACE]);
            lCurColors[BUTTONTEXT] = GetNearestColor (hDC, lCurColors[BUTTONTEXT]);
            lCurColors[CAPTION2TEXT] = GetNearestColor (hDC, lCurColors[CAPTION2TEXT]);

            SetSysColors (COLORDEFS, (LPINT)Xlat, (long far * )lCurColors);

            StoreToWin (hWnd); // store changes to win.ini; broadcast changes

            /* Note current Scheme, search through all schemes? */

            temp = SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETCURSEL, 0, 0L);

            if ((temp == CB_ERR) || (SendDlgItemMessage (hWnd, COLOR_SCHEMES,
                            CB_GETLBTEXT, temp, (LONG)szNew) == CB_ERR))
                szNew[0] = TEXT('\0');

            bTemp = WritePrivateProfileString (szCurrent, szColorSchemes,
                            ColorSchemeMatch (hDC, szNew) ? szNew : szNull,
                            szCtlIni);
            ReleaseDC (hWnd, hDC);

            if (fChanged & 0x01)
            {
                lstrcpy (szTemp, szColorA);
                for (id = COLOR_CUSTOM1 - COLOR_BOX1; id < COLORBOXES; id++)
                {
                    szTemp[5] = (TCHAR)TEXT('A') + (TCHAR)(id - COLOR_CUSTOM1 + COLOR_BOX1);
                    wsprintf (szNew, TEXT("%lX"), rgbBoxColor[id]);
                    bTemp = bTemp && WritePrivateProfileString (szCustomColors,
                        szTemp, szNew,
                        szCtlIni);
                }
            }

            HourGlass (FALSE); // change cursor back to arrow
        // fall through

        case IDCANCEL:

            if (bMouseCapture)
            {
                bMouseCapture = FALSE;
                ReleaseCapture ();
                ClipCursor (NULL);
            }

            if (bTuning)
            {
                thWnd = GetFocus ();
                if ((thWnd == hBox1) || (thWnd == hCustom1))
                    SendMessage (thWnd, WM_KILLFOCUS, 0, 0L);
            }

            if (hDCBits)
            {
                DeleteDC (hDCBits);
                hDCBits = NULL;
            }

            if (hUpArrow)
            {
                DeleteObject (hUpArrow);
                hUpArrow = NULL;
            }

            if (hDownArrow)
            {
                DeleteObject (hDownArrow);
                hDownArrow = NULL;
            }

            DeleteObject (hIconRgn);
            hIconRgn = NULL;

            if (hRainbowDlg)
                SendMessage (hRainbowDlg, WM_COMMAND, IDOK, 0L);

            if (GetParent(hWnd))
                EnableWindow(GetParent(hWnd), TRUE);

            EndDialog (hWnd, TRUE);
            hwndColor = NULL;
            break;

        case COLOR_REMOVE:

            id = SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETCURSEL, 0, 0L);
            SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETLBTEXT, (LONG)id, (LONG)szNew);
            pszScan = szNew;
            while (*(pszScan = CharNext (pszScan)) != TEXT('='))
                ;
            *pszScan = TEXT('\0');
            if (!id)
            {

                TCHAR    szFormat[81];

                if (!LoadString (hModule, COLOR + COLORSCHEMES + 1, szFormat, CharSizeOf(szFormat)))
                    ErrMemDlg (hWnd);
                else
                {
                    wsprintf (szTemp, szFormat, (LPTSTR)szNew);
                    MessageBox (hWnd, szTemp, szCtlPanel, MB_OK | MB_ICONINFORMATION);
                }

                break;

            }
            else if (RemoveMsgBox (hWnd, szNew, REMOVEMSG_COLOR))
            {

                GetPrivateProfileString (szCurrent, szColorSchemes, szNull, szTemp,
                                         CharSizeOf(szTemp),  szCtlIni);
                if (!lstrcmpi (szNew, szTemp))
                    WritePrivateProfileString (szCurrent, szColorSchemes, szNull,
                                               szCtlIni);

                WritePrivateProfileString (szColorSchemes, szNew, szNull, szCtlIni);

                // CB_DELETESTRING returns count of entries in Combo Box
                // The test here should not be necessary.  Since id is returned by
                // CB_GETCURSEL, it should be valid since an item is always selected.
                // For now, always select base entry if there's a problem.

                if ((temp = (short)SendDlgItemMessage (hWnd, COLOR_SCHEMES,
                    CB_DELETESTRING, (LONG)id, 0L))
                     == CB_ERR)
                    id = 0;

                else if (temp <= id)
                    --id;

                SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_SETCURSEL, (LONG)id, 0L);
                SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETLBTEXT, (LONG)id,
                    (LONG)szNew);
                SchemeSelection (hWnd, szNew);
            }

            break;

        case COLOR_SAVE:
            pszScan = szSchemeName;
            if ((temp = SendDlgItemMessage (hWnd, COLOR_SCHEMES,
                                             CB_GETCURSEL, 0, 0L)) != CB_ERR)
            {
                SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETLBTEXT, temp,
                    (LONG)szSchemeName);
                while (*(pszScan = CharNext (pszScan)) != TEXT('='))
                    ;
            }
            *pszScan = TEXT('\0');
            dwSave = dwContext;
            dwContext = IDH_DLG_COLORSAVE;

            if (DialogBox (hModule, (LPTSTR) MAKEINTRESOURCE(DLG_COLORSAVE), hWnd,
                 (DLGPROC) SaveScheme))
            {
                // Add the '=' to be certain we get the right one
                lstrcpy (szNew, szSchemeName);
                lstrcat (szNew, szEqual);
                if ((id = SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_FINDSTRING,
                                    (WPARAM) (LONG)-1, (LONG)szNew)) != CB_ERR)
                {
                    SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_DELETESTRING,
                        (LONG)id, 0L);
                }

                for (temp = 0, pszScan = szNew; temp < COLORDEFS; temp++)
                {

                    wsprintf (pszScan, TEXT("%lX,"), lCurColors[temp]);
                    pszScan += lstrlen ( pszScan);
                }

                szNew[lstrlen ( szNew) - 1] = TEXT('\0'); // chop last comma

                WritePrivateProfileString (szColorSchemes, szSchemeName, szNew,
                    szCtlIni);
                lstrcat (szSchemeName,  szEqual);
                lstrcat (szSchemeName,  szNew);
                temp = SendDlgItemMessage (hWnd,
                    COLOR_SCHEMES,
                    (id == CB_ERR) ?
                    CB_ADDSTRING
                     : CB_INSERTSTRING,
                    (LONG)id,
                    (LONG)szSchemeName);
                SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_SETCURSEL,
                    (LONG)temp, 0L);
                EnableWindow (GetDlgItem (hWnd, COLOR_REMOVE), TRUE);
            }

            break;

        case COLOR_SCHEMES:

            if (HIWORD (wParam) == CBN_SELCHANGE)
            {
                temp = SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETCURSEL, 0, 0L);
                if (temp != CB_ERR)
                {
                    SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETLBTEXT, temp,
                                                        (LONG)szNew);
                    SchemeSelection (hWnd, szNew);

                    /* We need to invalidate this Window so that it will
                     * get repainted when the combobox pops up again
                     * Actually, we only need to repaint it if its status
                     * changes, but that's too much work for me.
                     */
                    EnableWindow (hPointWnd=GetDlgItem(hWnd, COLOR_REMOVE), temp);
                    InvalidateRect (hPointWnd, NULL, TRUE);

                    ChangeColorBox (hWnd, currentRGB = lCurColors[nElementIndex]);
                }
            }
            break;

        case COLOR_ELEMENT:

            if (HIWORD (wParam) == CBN_SELCHANGE)
            {
                nElementIndex = ElementLBItems[(short)SendDlgItemMessage (hWnd,
                                        COLOR_ELEMENT, CB_GETCURSEL, 0, 0L)];

                ChangeColorBox (hWnd, currentRGB = lCurColors[nElementIndex]);
            }

            break;

        case COLOR_TUNE:            // fold out the right half of this dialog

            HourGlass (TRUE);       // change cursor to hourglass

            bTuning = TRUE;
            EnableWindow (GetDlgItem (hWnd, COLOR_TUNE), FALSE);

            InitTuning (hWnd);
#if 0
            if (InitTuning (hWnd))
            {
                InvalidateRect (hWnd, NULL, FALSE);
            }
            else
            {
                GetWindowRect (GetDlgItem (hWnd, COLOR_ELEMENT), &Rect);
                Rect.left -= 1;
                Rect.right = Orig.right;
                Rect.top = Orig.top;
                Rect.bottom = Orig.bottom;
                ScreenToClient (hWnd, (LPPOINT) &Rect.left);
                ScreenToClient (hWnd, (LPPOINT) &Rect.right);
                InvalidateRect (hWnd, &Rect, FALSE);
            }

            // UpdateWindow (hWnd);
#endif

            HourGlass (FALSE); // change cursor back to arrow
            break;

        case COLOR_MIX:

            HourGlass (TRUE);  // change cursor to hourglass

            EnableWindow (GetDlgItem (hWnd, COLOR_MIX), FALSE);
            hRainbowDlg = CreateDialog (hModule, (LPTSTR) MAKEINTRESOURCE(DLG_RAINBOW),
                                            hWnd, (DLGPROC) RainbowDlg);

            HourGlass (FALSE); // change cursor back to arrow

            if (!hRainbowDlg || (hRainbowDlg == (HWND) -1))
            {
                hRainbowDlg = 0;
                ErrMemDlg (hWnd);
                EnableWindow (GetDlgItem (hWnd, COLOR_MIX), TRUE);
            }
            else
            {
                SetFocus (hRainbowDlg);
            }
            break;

        }
        break;

    case WM_PAINT:
        BeginPaint (hWnd, (LPPAINTSTRUCT) & ps);
        ColorPaint (hWnd,  ps.hdc, &ps.rcPaint);
        EndPaint (hWnd, (LPPAINTSTRUCT) & ps);
        break;

    case WM_DESTROY:
        break;

    case WM_ACTIVATE:
        if (LOWORD (wParam))
        {
            if (LOWORD (wParam) == 2)
                UpdateWindow (hWnd);
            thWnd = hSave;
            hSave = 0;
            return (FALSE);
        }
        else
        {
            hSave = thWnd = hWnd;
        }

        if ((thWnd == hBox1) || (thWnd == hCustom1))
        {
            if (thWnd == hBox1)
                CopyRect (&Rect, rColorBox + nCurDsp);
            else
                CopyRect (&Rect, rColorBox + nCurMix);

            InflateRect (&Rect, 3, 3);
            hDC = GetDC (hWnd);
            DrawFocusRect (hDC, &Rect);
            ReleaseDC (hWnd, hDC);
            ValidateRect (thWnd, NULL);

        }

        return FALSE;

    default:

        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp (hWnd);
            return TRUE;
        }
        else
            return FALSE;

        break;
    }

    if (bUpdateExample && bTuning)
    {
        lCurColors[nElementIndex] = currentRGB;
        hDC = GetDC (hWnd);
        PaintElement (hWnd, hDC, nElementIndex);
        ReleaseDC (hWnd, hDC);
    }
    return TRUE;
}


BOOL RemoveMsgBox (HWND  hWnd, LPTSTR lpStr1, WORD wString)
{
    TCHAR    lpS[PATHMAX];
    TCHAR    removemsg[PATHMAX];

    LoadString (hModule, wString, removemsg, CharSizeOf(removemsg));
    wsprintf (lpS, removemsg, lpStr1);
    return (MessageBox (hWnd, lpS, szCtlPanel, MB_YESNO | MB_ICONEXCLAMATION)
                         == IDYES);
}

/* hexatol converts an asciz string representing a hexidecimal number
   into an unsigned long.  This routine assumes that the string is NULL
   terminated immediately after the number.  Overflow is not checked,
   and naturally it will only return values of 0xFFFFFFFF or less.
NOTE:  The characters in the string other than 0-9 and A-F will be altered
*/

DWORD hexatol (LPTSTR psz)
{
    DWORD dwRetval = 0;
#ifdef UNICODE
    TCHAR addval;
#else
    unsigned char addval;
#endif

    while (*psz)
    {
        addval = 0xF0;

        if ((*psz >= TEXT('0')) && (*psz <= TEXT('9')))
        {
            addval = *psz - TEXT('0');
        }
        else if (_totupper(*psz) >= TEXT('A'))
        {
            // To uppercase

            if (*psz <= TEXT('F'))
                addval = *psz - TEXT('A') + (TCHAR) 10;
        }

        if (addval & 0xF0) // Non hex character encountered
            break;

        dwRetval <<= 4;  // dwRetval *= 16

        dwRetval |= addval;
        psz++;
    }
    return (dwRetval);
}


void HiLiteBox (HDC hDC, DWORD nBox, DWORD fStyle)
{
    RECT Rect;
    HBRUSH hBrush;

    CopyRect (&Rect, rColorBox + nBox);
    Rect.left--, Rect.top--, Rect.right++, Rect.bottom++;
    if (hBrush = CreateSolidBrush ((fStyle & 1) ? 0L : GetSysColor (COLOR_WINDOW)))
    {
        FrameRect (hDC, &Rect, hBrush);
        DeleteObject (hBrush);
    }

    return;
}


void ChangeBoxSelection (HWND  hWnd, DWORD nNewBox)
{
    HDC hDC;

    hDC = GetDC (hWnd);
    HiLiteBox (hDC, nCurBox, 0);
    HiLiteBox (hDC, nNewBox, 1);
    ReleaseDC (hWnd, hDC);
    currentRGB = rgbBoxColor[nNewBox];

    return;
}


void ChangeBoxFocus (HWND  hWnd, DWORD nNewBox)
{
    HDC hDC;
    RECT Rect;
    DWORD *nCur = (nNewBox < (COLOR_CUSTOM1 - COLOR_BOX1)) ? &nCurDsp
                                                           : &nCurMix;
    hDC = GetDC (hWnd);
    CopyRect (&Rect, rColorBox + *nCur);
    InflateRect (&Rect, 3, 3);
    DrawFocusRect (hDC, &Rect);
    CopyRect (&Rect, rColorBox + (*nCur = nNewBox));
    InflateRect (&Rect, 3, 3);
    DrawFocusRect (hDC, &Rect);
    ReleaseDC (hWnd, hDC);

    return;
}


void ChangeColorBox (HWND  hWnd, DWORD dwRGBcolor)
{
    DWORD nBox;

    for (nBox = 0; nBox < COLORBOXES; nBox++)
    {
        if (rgbBoxColor[nBox] == dwRGBcolor)
            break;
    }

    if (nBox >= COLORBOXES)
    {
        /* Color Not Found.  Now What Should We Do? */
    }
    else
    {
        ChangeBoxSelection (hWnd, nBox);
        nCurBox = nBox;
    }

    return;
}


void RetractComboBox (HWND  hWnd)
{
    HWND hFocus;

    hFocus = GetFocus ();

    if ((hFocus == GetDlgItem (hWnd, COLOR_ELEMENT)) ||
        (hFocus == GetDlgItem (hWnd, COLOR_SCHEMES)))
    {
        SendMessage (hFocus, CB_SHOWDROPDOWN, FALSE, 0L);
    }
    return;
}


BOOL ColorSchemeMatch (HDC hDC, LPTSTR pszScheme)
{
    short    i;
    DWORD lColor;

    if (!*pszScheme)
        return (FALSE);

    while (*pszScheme != TEXT('='))
        pszScheme = CharNext (pszScheme);

    *pszScheme++ = TEXT('\0');                  /* NULL terminate scheme name */

    for (i = 0; i < COLORDEFS; i++)
    {
        lColor = hexatol (pszScheme);

        switch (i)
        {
        case MENUBAR:
        case MENUTEXT:
        case CAPTIONTEXT:
        case CLIENT:
        case CLIENTTEXT:
        case WINDOWFRAME:
        case GRAYTEXT:
        case HIGHLIGHT:
        case HIGHLIGHTTEXT:
        case BUTTONFACE:
        case BUTTONTEXT:
        case CAPTION2TEXT:
            lColor = GetNearestColor (hDC, lColor);
            break;
        }

        if (lCurColors[i] != lColor)
            return (FALSE);

        while (*pszScheme && (*pszScheme != TEXT(',')))
            pszScheme = CharNext (pszScheme);

        pszScheme = CharNext (pszScheme);
    }
    return (TRUE);
}


short SchemeSelection (HWND  hWnd, LPTSTR pszScheme)
{
    short    i, j;
//    HDC hDC;

    while (*pszScheme && *pszScheme != TEXT('=') )
        pszScheme = CharNext (pszScheme);

    pszScheme = CharNext (pszScheme);

    for (i = 0; *pszScheme && i < COLORDEFS; i++)
    {
        lCurColors[i] = hexatol (pszScheme);

        while (*pszScheme && (*pszScheme != TEXT(',')))
            pszScheme = CharNext (pszScheme);

        pszScheme = CharNext (pszScheme);
    }

    /* The following added by C. Stevens, Sept 90.  This code checks to
     see if the schemes were created before version 3.1 */

    if (i != COLORDEFS)
    {
        for (j = i; j < COLORDEFS; j++)
            lCurColors[i] = GetSysColor (Xlat[i]);

        UpdateScheme (hWnd);
    }

    if (bTuning)
    {
        currentRGB = lCurColors[nElementIndex];
        ChangeColorBox (hWnd, currentRGB);
    }

//    hDC = GetDC (hWnd);
//    PaintElement (hWnd, hDC, ALLELEMENTS);
//    ReleaseDC (hWnd, hDC);

    InvalidateRect(hWnd, &rSamples, FALSE);

    return (1);
}


/* The following function updates a colour scheme to include all the
   user configurable colour settings.  This is used to convert a colour
   scheme from the 3.0 format to the 3.1 format.  C. Stevens, Sept 90 */

void UpdateScheme (HWND  hWnd)
{
    LPTSTR pszScan;
    TCHAR    szNew[MAX_SCHEMESIZE];
    DWORD temp;
    DWORD id;

    /* Get scheme name */

    pszScan = szSchemeName;

    temp = SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETCURSEL, 0, 0L);
    if (temp != CB_ERR) {
        SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETLBTEXT, temp,
                (LONG)szSchemeName);

        while (*(pszScan = CharNext (pszScan)) != TEXT('='));
        *pszScan = TEXT('\0');
    }

    lstrcpy (szNew,  szSchemeName);
    lstrcat (szNew,  szEqual); // Add the '=' to be certain we get the right one

    /* Get string id in list box and delete it */

    if ((id = SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_FINDSTRING,
                                  (WPARAM)(LONG) -1,(LONG)szNew)) != CB_ERR)
        SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_DELETESTRING, id, 0L);

    /* Generate new scheme string for 3.1 format */

    for (temp = 0, pszScan = szNew; temp < COLORDEFS; temp++)
    {
        wsprintf ( pszScan, TEXT("%lX,"), lCurColors[temp]);
        pszScan += lstrlen ( pszScan);
    }

    szNew[lstrlen (szNew) - 1] = TEXT('\0'); // chop last comma

    // Write to CONTROL.INI
    WritePrivateProfileString (szColorSchemes, szSchemeName, szNew, szCtlIni);

    /* Store string in list box */
    lstrcat ( szSchemeName, szEqual);
    lstrcat ( szSchemeName, szNew);
    temp = SendDlgItemMessage (hWnd, COLOR_SCHEMES,
                (id == CB_ERR) ? CB_ADDSTRING : CB_INSERTSTRING,
                (LONG)id,
                (LONG)szSchemeName);

    SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_SETCURSEL, (LONG)temp, 0L);
}


BOOL SaveScheme (HWND  hWnd, UINT message, DWORD wParam, LONG lParam)
{
    TCHAR    szRHS[MAX_SCHEMESIZE];
    TCHAR    szTemp[MAX_SCHEMESIZE];
    WORD     i;
    LPTSTR   psz;

    switch (message)
    {
    case WM_INITDIALOG:

        HourGlass (TRUE);
        SetDlgItemText (hWnd, COLOR_SAVE,  szSchemeName);
        SendDlgItemMessage (hWnd, COLOR_SAVE, EM_SETSEL, 0, 32767);
        SendDlgItemMessage (hWnd, COLOR_SAVE, EM_LIMITTEXT, 32, 0L);
        EnableWindow (GetDlgItem (hWnd, IDOK), szSchemeName[0] != TEXT('\0'));
        HourGlass (FALSE);
        return (TRUE);

        break;

    case WM_COMMAND:

        switch (LOWORD (wParam))
        {
        case COLOR_SAVE:
            if (HIWORD (wParam) == EN_CHANGE)
            {
                EnableWindow (GetDlgItem (hWnd, IDOK),
                GetDlgItemText (hWnd, COLOR_SAVE, szRHS, CharSizeOf(szRHS)));
            }
            break;

        case IDD_HELP:
            goto DoHelp;

        case IDOK:

            GetDlgItemText (hWnd, COLOR_SAVE,  szRHS, CharSizeOf(szRHS));
            StripBlanks (szRHS);

            if (*(psz = strscan (szRHS, TEXT("="))))
            {
                i = SCHEMEERR + 2;
            }
            else if ((*(psz = strscan (szRHS, TEXT("[")))) ||
                (*(psz = strscan (szRHS, TEXT("]")))))
            {
                i = SCHEMEERR + 3;
            }
            else if (!(*szRHS))
            {
                i = SCHEMEERR + 4;
            }
            else
            {
                LoadString (hModule, COLOR + COLORSCHEMES, szTemp, CharSizeOf(szTemp));

                if (!lstrcmpi (szRHS, szTemp))
                    i = SCHEMEERR + 1;
                else
                    i = 0;
            }

            if (i)
            {
                if (!LoadString (hModule, (WORD)(COLOR + i), szRHS, CharSizeOf(szRHS)))
                    ErrMemDlg (hWnd);
                else
                    MessageBox (hWnd, szRHS, szCtlPanel, MB_OK | MB_ICONINFORMATION);

                break;
            }

            lstrcpy (szSchemeName,  szRHS);

        // fall through...

        case IDCANCEL:

            EndDialog (hWnd, LOWORD (wParam) == IDOK);
            return (TRUE);
        }

    default:

        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp (hWnd);
            return TRUE;
        }
        else
            return FALSE;
    }
    return (FALSE);  // Didn't process a message
}


DWORD ColorStringFunc (LPTSTR pszScheme)
{
    LPTSTR  psz = pszScheme;

    while (*psz != TEXT('=') && *psz)
        psz = CharNext (psz);

    *psz = TEXT('\0');

    return (psz - pszScheme);
}


BOOL BoxDrawItem (LPDRAWITEMSTRUCT lpDIS)
{
    RECT Rect;
    HBRUSH hBrush, hOldBrush;

    GetClientRect(lpDIS->hwndItem, &Rect);
    if (hBrush=CreateSolidBrush(rgbBoxColor[lpDIS->CtlID - COLOR_BOX1]))
    {
        hOldBrush = SelectObject(lpDIS->hDC, hBrush);
        Rectangle(lpDIS->hDC, Rect.left, Rect.top, Rect.right, Rect.bottom);
        if (hOldBrush)
            SelectObject(lpDIS->hDC, hOldBrush);
        DeleteObject(hBrush);
    }
    return(TRUE);
}


BOOL ComboDrawItem (LPDRAWITEMSTRUCT lpDIS)
{
    TCHAR    szEntry[MAX_SCHEMESIZE];
    DWORD nLen;
    DWORD dwRGBBkGnd;
    DWORD dwRGBText;

    if ((!(lpDIS->itemAction & (ODA_SELECT | ODA_FOCUS | ODA_DRAWENTIRE))) ||
        (lpDIS->itemID == (DWORD)-1))
        return FALSE;

    SendMessage (lpDIS->hwndItem, CB_GETLBTEXT, lpDIS->itemID, (LONG)szEntry);
    nLen = ColorStringFunc (szEntry);

    if (lpDIS->itemAction & (ODA_DRAWENTIRE | ODA_SELECT))
    {
        if (lpDIS->itemState & ODS_SELECTED)
        {
            dwRGBBkGnd = SetBkColor (lpDIS->hDC, GetSysColor (COLOR_HIGHLIGHT));
            dwRGBText = SetTextColor (lpDIS->hDC, GetSysColor (COLOR_HIGHLIGHTTEXT));
        }

        ExtTextOut (lpDIS->hDC, lpDIS->rcItem.left, lpDIS->rcItem.top,
                ETO_CLIPPED | ETO_OPAQUE, &lpDIS->rcItem, szEntry, nLen, 0L);

        if (lpDIS->itemState & ODS_SELECTED)
        {
            SetBkColor (lpDIS->hDC, dwRGBBkGnd);
            SetTextColor (lpDIS->hDC, dwRGBText);
        }

        if (lpDIS->itemState & ODS_FOCUS)
            lpDIS->itemAction |= ODA_FOCUS;
    }

    if (lpDIS->itemAction & ODA_FOCUS)
    {
        InflateRect (&lpDIS->rcItem, 1, 1);
        DrawFocusRect (lpDIS->hDC, &lpDIS->rcItem);
    }

    return TRUE;
}


BOOL ColorKeyDown (DWORD wParam, DWORD *id)
{
    DWORD temp;

    temp = GetWindowLong (GetFocus (), GWL_ID);

    if (temp == COLOR_BOX1)
        temp = nCurDsp;
    else if (temp == COLOR_CUSTOM1)
        temp = nCurMix;
    else
        return (FALSE);

    switch (wParam)
    {

    case VK_UP:

        if (temp >= (COLOR_CUSTOM1 - COLOR_BOX1 + NUM_X_BOXES))
            temp -= NUM_X_BOXES;

        else if ((temp < COLOR_CUSTOM1 - COLOR_BOX1) && (temp >= NUM_X_BOXES))
            temp -= NUM_X_BOXES;

        break;

    case VK_HOME:

        if (temp == nCurDsp)
            temp = 0;
        else
            temp = COLOR_CUSTOM1 - COLOR_BOX1;

        break;

    case VK_END:

        if (temp == nCurDsp)
            temp = nDriverColors - 1;
        else
            temp = COLORBOXES - 1;

        break;

    case VK_DOWN:

        if (temp < (COLOR_CUSTOM1 - COLOR_BOX1 - NUM_X_BOXES))
            temp += NUM_X_BOXES;
        else if ((temp >= (COLOR_CUSTOM1 - COLOR_BOX1)) &&
            (temp < (COLORBOXES - NUM_X_BOXES)))
            temp += NUM_X_BOXES;

        break;

    case VK_LEFT:

        if (temp % NUM_X_BOXES)
            temp--;

        break;

    case VK_RIGHT:

        if (!(++temp % NUM_X_BOXES))
            --temp;

        break;

    }

    // if we've received colors from the driver, make certain the arrow would
    // not take us to an undefined color

    if ((temp >= nDriverColors) && (temp < COLOR_CUSTOM1 - COLOR_BOX1))
        temp = nCurDsp;

    *id = temp;

    return ((temp != nCurDsp) && (temp != nCurMix));
}


DWORD ElementFromPt (POINT pt)
{
    DWORD nElement;
    static int    dButtonToggle = 0;
    static int    dHighlightToggle = 0;

    if (PtInRect (&rMDIWindow, pt) || PtInRect (&rMDIWindow2, pt))
    {
        if (PtInRect (&rClient, pt))
        {
            if (PtInRect (&rClientText, pt))
            {
                nElement = CLIENTTEXT;

                // This next code added by C. Stevens, Sept 90.  Check for
                  // hit in sample pushbutton, elements of pulldown menu.
            }
            else if (PtInRect (&rButton, pt))
            {
                // Toggle between Button Face, Button Shadow, Button
                // Text, and Button Highlight selection

                nElement = BUTTONFACE + dButtonToggle;
                dButtonToggle++;

                // Kluge added so we don't have to put BUTTONHIGHLIGHT next to
                // the other button elements.  This is so people with older
                // versions of 3.1 can be compatible

                if (dButtonToggle == BUTTONTEXT - BUTTONFACE + 1)
                    dButtonToggle = BUTTONHIGHLIGHT - BUTTONFACE;
                else if (dButtonToggle == BUTTONHIGHLIGHT - BUTTONFACE + 1)
                    dButtonToggle = 0;
            }
            else
            {
                nElement =  CLIENT;
            }
        }
        else if (PtInRect (&rPullDown, pt))
        {
            /* Determine if hit is in Gray Text or Highlighted Text area */

            if (PtInRect (&rGrayText, pt))
            {
                nElement = GRAYTEXT;
            }
            else
            {
                /* Toggle between Highlight and Highlighted Text selection */

                nElement = HIGHLIGHT + dHighlightToggle;
                dHighlightToggle = (dHighlightToggle + 1) % 2;
            }
        }
        else
        {
            nElement =  MDIWINDOW;
        }
    }
    else if (PtInRect ( &rMenuBar, pt) || PtInRect ( &rMenuBar2, pt))
    {
        if (PtInRect ( &rMenuText, pt))
            nElement =  MENUTEXT;
        else
            nElement =  MENUBAR;
    }
    else if (PtInRect ( &rCaptionText, pt))
    {
        nElement =  CAPTIONTEXT;
    }
    else if (PtInRect ( &rCaptionText2, pt))
    {
        nElement = CAPTION2TEXT;
    }
    else if (PtInRect ( &rCaptionLeft, pt) || PtInRect ( &rCaptionRight, pt))
    {
        nElement =  MYCAPTION;
    }
    else if (PtInRect ( &rCaptionLeft2, pt) || PtInRect ( &rCaptionRight2, pt)
         || PtInRect ( &rCaptionText2, pt))
    {
        nElement =  CAPTION2;
    }
    else if (PtInRect ( &rUpArrow, pt) || PtInRect ( &rDownArrow, pt))
    {
        nElement =  nElementIndex;   // No element change
    }
    else if (PtInRect ( &rScroll, pt))
    {
        nElement =  SCROLLBARS;
    }
    else if (PtInRect ( &rBorderTop, pt) || PtInRect ( &rBorderLeft, pt) ||
        PtInRect ( &rBorderRight, pt) || PtInRect ( &rBorderBottom, pt))
    {
        nElement =  BORDER;
    }
    else if (PtInRect ( &rBorderTop2, pt) || PtInRect ( &rBorderLeft2, pt) ||
        PtInRect ( &rBorderRight2, pt) || PtInRect ( &rBorderBottom2, pt))
    {
        nElement =  BORDER2;
    }
    else if (PtInRegion (hIconRgn, pt.x, pt.y))
    {
        nElement = BACKGROUND;
    }
    else
    {
        nElement = WINDOWFRAME;
    }

    return (nElement);
}


void PaintBox (HDC   hDC, DWORD i)
{
    HBRUSH hBrush, hOldBrush;

    if ((i < COLOR_CUSTOM1 - COLOR_BOX1) && (i >= nDriverColors))
        return;

    if (hBrush=CreateSolidBrush(rgbBoxColor[i]))
    {
        hOldBrush = SelectObject(hDC, hBrush);
        Rectangle(hDC, rColorBox[i].left, rColorBox[i].top,
          rColorBox[i].right, rColorBox[i].bottom);

        /* Dont forget to outline the rectangle if it is the currently selected
            one */

        if (i == nCurBox)
            HiLiteBox (hDC,nCurBox,1);

        if (hOldBrush)
            SelectObject(hDC, hOldBrush);
        DeleteObject(hBrush);
    }

    return;
}


BOOL SetupScreenDiagram (HWND hWnd)
{
    DWORD lenActive, lenInactive, lenClientText, lenMenuText;
    HDC hDC;
    DWORD cyTop, cyTemp;
    DWORD cySimCaption, cySimMenu;
    HRGN hTempRgn, hTempRgn2, hTempRgn3;
    BOOL bError;
    TEXTMETRIC tm;
    SIZE Size;

    hDC = GetDC (hWnd);
    GetTextMetrics (hDC, &tm);
    cySimMenu = cySimCaption = tm.tmHeight + 2 * cyBorder;

    GetTextExtentPoint (hDC, szActive, lstrlen (szActive), &Size);
    lenActive = Size.cx;

    GetTextExtentPoint (hDC, szInactive, lstrlen (szInactive), &Size);
    lenInactive = Size.cx;

    GetTextExtentPoint (hDC, szMenu, lstrlen (szMenu), &Size);
    lenMenuText = Size.cx;

    GetTextExtentPoint (hDC, szWindow, lstrlen (szWindow), &Size);
    lenClientText = Size.cx;

    ReleaseDC (hWnd, hDC);

    GetWindowRect (GetDlgItem (hWnd, COLOR_SAMPLES), &rSamples);
    CopyRect (&rSamplesCapture, &rSamples);
    ScreenToClient (hWnd, (LPPOINT) & rSamples.left);
    ScreenToClient (hWnd, (LPPOINT) & rSamples.right);

    // Set up the edges

    rBorderOutline2.left =
        rBorderLeftFrame2.left = rSamples.left + cxSize / 2;

    rBorderOutline2.top =
        rBorderTopFrame2.top = rSamples.top + cySimCaption / 2;

    rBorderRightFrame.right =
        rBorderOutline.right =
        rBorderBottom.right = rSamples.right - cxSize / 2;

    // From the top

    rBorderTop2.top =
        rBorderRight2.top =
        rBorderLeft2.top = rBorderOutline2.top + 1;

    rBorderInterior2.top =
        rBorderTop2.bottom = rBorderTop2.top + 5 * cyBorder;

    rCaptionLeft2.top =
        rCaptionText2.top =
        rCaptionRight2.top =
        rBorderTopFrame2.bottom = rBorderInterior2.top + 1;

    rBorderLeftFrame2.top =
        rBorderRightFrame2.top = rCaptionLeft2.top + cySimCaption;

    rCaptionLeft2.bottom =
        rCaptionText2.bottom =
        rMenuFrame2.top =
        rCaptionRight2.bottom =
        rBorderTopFrame.top =
        rBorderOutline.top = rCaptionLeft2.top + cySimCaption;

    rMenuBar2.top = rMenuFrame2.top + cyBorder;

    rBorderTop.top =
        rBorderRight.top =
        rBorderLeft.top = rBorderOutline.top + 1;

    rMenuBar2.bottom = rMenuBar2.top + cySimMenu;

    rMDIWindow2.top =
        rMenuFrame2.bottom = rMenuBar2.bottom + cyBorder;

    rBorderTop.bottom =
        rBorderInterior.top = rBorderTop.top + 5 * cyBorder;

    rCaptionLeft.top =
        rCaptionText.top =
        rCaptionRight.top =
        rBorderTopFrame.bottom = rBorderInterior.top + 1;

    rCaptionLeft.bottom =
        rCaptionText.bottom =
        rCaptionRight.bottom =
        rMenuFrame.top = rCaptionLeft.top + cySimCaption;

    rMenuText.top =
        rMenuBar.top = rMenuFrame.top + cyBorder;

    rBorderLeftFrame.top =
        rBorderRightFrame.top = rCaptionLeft.top + cySimCaption;

    rMenuText.bottom =
        rMenuBar.bottom =  rMenuBar.top + cySimMenu;

    rUpArrow.top = rMenuBar.bottom;

    rMenuFrame.bottom =
        rMDIWindow.top = rMenuBar.bottom + cyBorder;

    rScrollFrame.top = rMenuFrame.bottom - cyBorder;

    rUpArrow.bottom = rUpArrow.top + cyVScroll;

    rScroll.top = rUpArrow.bottom;

    // From the bottom

    rBorderBottomFrame.bottom =
        rBorderOutline.bottom = rSamples.bottom - (cyIcon >> 2) + 1;

    rBorderLeft.bottom =
        rBorderBottom.bottom =
        rBorderRight.bottom = rBorderOutline.bottom - 1;

    rScrollFrame.bottom =
        rDownArrow.bottom =
        rBorderBottom.top =
        rBorderInterior.bottom = rBorderBottom.bottom - 5 * cyBorder;

    rMDIWindow.bottom = rBorderInterior.bottom - 1;

    rBorderBottomFrame.top = rBorderInterior.bottom - 1;

    rBorderOutline2.bottom = rBorderOutline.bottom - cySimCaption / 2;

    rBorderLeft2.bottom =
        rBorderRight2.bottom =
        rBorderBottom2.bottom = rBorderOutline2.bottom - 1;

    rBorderBottom2.top =
        rBorderInterior2.bottom = rBorderBottom2.bottom - 5 * cyBorder;

    rMDIWindow2.bottom = rBorderInterior2.bottom - 1;

    rBorderLeftFrame2.bottom =
        rBorderRightFrame2.bottom = rMDIWindow2.bottom - cySimCaption;

    rBorderLeftFrame.bottom =
        rBorderRightFrame.bottom = rMDIWindow.bottom - cySimCaption;

    rDownArrow.top = rDownArrow.bottom - cyVScroll;

    rScroll.bottom = rDownArrow.top;

    // From the left

    rBorderLeft2.left =
        rBorderBottom2.left =
        rBorderTop2.left = rBorderOutline2.left + 1;

    rBorderLeft2.right =
        rBorderInterior2.left =
        rMenuFrame2.left = rBorderOutline2.left + 5 * cxBorder;

    rBorderLeftFrame2.right =
        rCaptionLeft2.left =
        rMenuBar2.left =
        rMDIWindow2.left = rBorderInterior2.left + 1;

    rBorderTopFrame2.left =
        rBorderBottomFrame2.left = rBorderLeft2.right + cxSize;

    rMDIWindow2.right =
        rMenuBar2.right =
        rBorderOutline.left =
        rBorderLeftFrame.left = rBorderLeft2.right + cxSize / 4;

    rMenuFrame2.right =
        rBorderLeft.left =
        rBorderBottom.left =
        rBorderTop.left = rBorderOutline.left + 1;

    rBorderLeft.right =
        rBorderInterior.left =
        rMenuFrame.left = rBorderOutline.left + 5 * cxBorder;

    rBorderLeftFrame.right =
        rCaptionLeft.left =
        rMenuBar.left =
        rMenuText.left =
        rMDIWindow.left = rBorderInterior.left + 1;

    rMenuText.right = rMenuText.left + lenMenuText;

    rBorderTopFrame.left =
        rBorderBottomFrame.left = rBorderLeft.right + cxSize;

    // From the right

    rBorderRight.right =
        rBorderTop.right = rBorderOutline.right - 1;

    rBorderRight.left =
        rBorderInterior.right =
        rMenuFrame.right =
        rScrollFrame.right = rBorderRight.right - 5 * cxBorder;

    rUpArrow.right =
        rDownArrow.right = rScrollFrame.right;

    rScroll.right = rScrollFrame.right - 1;

    rCaptionRight.right =
        rMenuBar.right =
        rBorderRightFrame.left = rBorderInterior.right - 1;

    rScrollFrame.left =
        rMDIWindow.right =
        rUpArrow.left =
        rDownArrow.left = rUpArrow.right - cxVScroll;

    rScroll.left = rScrollFrame.left + 1;

    rBorderRightFrame2.right =
        rBorderOutline2.right = rBorderOutline.right - cxSize / 2;

    rBorderTopFrame.right =
        rBorderBottomFrame.right = rScroll.right - cxSize;

    rBorderTop2.right =
        rBorderRight2.right =
        rBorderBottom2.right = rBorderOutline2.right - 1;

    rBorderRight2.left =
        rBorderInterior2.right = rBorderRight2.right - 5 * cxBorder;

    rBorderTopFrame2.right = rBorderRight2.left - cxSize;

    rBorderRightFrame2.left =
        rCaptionRight2.right = rBorderInterior2.right - 1;

    // Now for the tricky part

    rCaptionText.left = ((rCaptionLeft.left + rCaptionRight.right) >> 1)
    -(lenActive >> 1) - 2 * cxBorder;

    rCaptionText.right = rCaptionText.left + lenActive + 4 * cxBorder;

    rCaptionLeft.right = rCaptionText.left;          // - cxBorder

    rCaptionRight.left = rCaptionText.right;         // + cxBorder

    rCaptionText2.left = ((rCaptionLeft2.left + rCaptionRight2.right) >> 1)
    -(lenInactive >> 1) - 2 * cxBorder;

    rCaptionText2.right = rCaptionText2.left + lenInactive + 4 * cxBorder;

    rCaptionLeft2.right = rCaptionText2.left;        // + cxBorder

    rCaptionRight2.left = rCaptionText2.right;       // - cxBorder

    // active, inactive Textout Posns
    // 09-Apr-1987. davidhab. Redid placement of caption text to use
    // same algorithm as User in winmenu1.c

    cyTop = CharExternalLeading;
    cyTemp = (rCaptionText.bottom - rCaptionText.top) -
        (CharHeight + CharExternalLeading + 1);

    if (cyTemp > 0)
        cyTop += (cyTemp >> 1);

    ptTitleText.x = rCaptionText.left + 2 * cxBorder;
    ptTitleText.y = rCaptionText.top + cyTop;
    ptTitleText2.x = rCaptionText2.left + 2 * cxBorder;
    ptTitleText2.y = rCaptionText2.top + cyTop;

    // menu bar TextOut position
    ptMenuText.x = rMenuBar.left + 4 * cxBorder;

    // 09-Apr-1987. davidhab. Redid placement of menu text to use
    // same algorithm as User in winmenu1.c

    cyTop = CharExternalLeading;
    cyTemp = (rMenuBar.bottom - rMenuBar.top) -
        (CharHeight + CharExternalLeading + 1);

    if (cyTemp > 0)
        cyTop += (cyTemp >> 1);

    ptMenuText.y = rMenuBar.top + cyTop;

    CopyRect (&rClient, &rMDIWindow);

    rClient.left += (rMDIWindow.right - rMDIWindow.left) / 2;
    InflateRect ( &rClient, -((rClient.right - rClient.left) >> 4),
        -((rClient.bottom - rClient.top) >> 4));

    // Generate PullDown menu frame and individual menu elements

    CopyRect ( &rPullDown, &rMDIWindow);
    rPullDown.left -= 1;
    rPullDown.top -= 1;
    rPullDown.right -= (rMDIWindow.right - rMDIWindow.left) / 2;
    rPullDown.right -= (rPullDown.right - rPullDown.left) >> 4;
    rPullDown.bottom = rPullDown.top + (CharHeight + CharExternalLeading + 1)
    *2;
    CopyRect ( &rPullInside, &rPullDown);
    rPullInside.top++;
    rPullInside.left++;
    rPullInside.bottom--;
    rPullInside.right--;

    CopyRect ( &rGrayText, &rPullDown);
    rGrayText.bottom -= CharHeight + CharExternalLeading + 1;
    rGrayText.left += 1;
    rGrayText.top += 1;

    CopyRect (&rHighlight, &rPullDown);
    rHighlight.top += CharHeight + CharExternalLeading + 1;
    rHighlight.left += 1;
    rHighlight.right -= 1;
    rHighlight.bottom -= 1;

    CopyRect (&rClientFrame, &rClient);
    rClient.left++, rClient.top++;
    rClient.right--, rClient.bottom--;

    hDC = GetDC (hWnd);
    CopyRect (&rClientText, &rClient);

    // Determine text size so that we can create a rectangle for it

    DrawText (hDC, szWindow, lstrlen (szWindow), &rClientText,
        DT_CALCRECT | DT_CENTER | DT_WORDBREAK);
    ReleaseDC (hWnd, hDC);

    // Generate PushButton rectangle

    CopyRect (&rButton, &rClient);
    rButton.top += (rButton.bottom - rButton.top) / 2;
    InflateRect (&rButton, -(rButton.right - rButton.left) >> 3,
                 -(rButton.bottom - rButton.top) >> 3);

    // hIconRgn and hTempRgn3 can be created as anything.  CombineRgn demands
    // that it exist, and it will delete it and replace it with a new region.

    hTempRgn  = CreateRectRgnIndirect (&rBorderOutline);
    hTempRgn2 = CreateRectRgnIndirect (&rBorderOutline2);
    hTempRgn3 = CreateRectRgnIndirect (&rBorderOutline2);
    hIconRgn  = CreateRectRgnIndirect (&rBorderOutline2);

    if (!hTempRgn || !hTempRgn2 || !hTempRgn3 || !hIconRgn)
    {
        if (hTempRgn)
            DeleteObject (hTempRgn);
        if (hTempRgn2)
            DeleteObject (hTempRgn2);
        if (hTempRgn3)
            DeleteObject (hTempRgn3);
        if (hIconRgn)
            DeleteObject (hIconRgn);

        return (FALSE);
    }

    bError = !CombineRgn (hTempRgn3, hTempRgn, hTempRgn2, RGN_OR);

    DeleteObject (hTempRgn);
    DeleteObject (hTempRgn2);

    if (!(hTempRgn = CreateRectRgnIndirect ( &rSamples)))
    {
        DeleteObject (hIconRgn);
        DeleteObject (hTempRgn3);
        return (FALSE);
    }

    bError |= !CombineRgn (hIconRgn, hTempRgn, hTempRgn3, RGN_DIFF);
    DeleteObject (hTempRgn);
    DeleteObject (hTempRgn3);

    if (bError)
    {
        DeleteObject (hIconRgn);
        return (FALSE);
    }

    return (TRUE);
}


/* InitTuning  -  fold out the right size of the color dialog
   Returns TRUE iff we make it
*/

BOOL InitTuning (HWND  hWnd)
{
    RECT   Rect;
    DWORD  i;
    TCHAR  szTemp[8];
    TCHAR  szRHS[MAX_SCHEMESIZE];
#ifdef LATER
    HANDLE hCT = 0;
    LPINT lpNumColors;
    DWORD FAR * lpDriverRGB;
#endif  // LATER


    EnableWindow (GetDlgItem (hWnd, COLOR_ELEMENT), TRUE);
    EnableWindow (GetDlgItem (hWnd, COLOR_SAVE), TRUE);
    EnableWindow (GetDlgItem (hWnd, COLOR_MIX), TRUE);

    EnableWindow (hBox1, TRUE);
    lpprocStatic = (WNDPROC) GetWindowLong (hBox1, GWL_WNDPROC);
    SetWindowLong (hBox1, GWL_WNDPROC, (LONG)WantArrows);

    EnableWindow (hCustom1, TRUE);
    SetWindowLong (hCustom1, GWL_WNDPROC, (LONG)WantArrows);

    currentRGB = lCurColors[nElementIndex = 0];
    SendDlgItemMessage (hWnd, COLOR_ELEMENT, CB_SETCURSEL, nElementIndex, 0L);

    GetWindowRect (hBox1, &Rect);
    ScreenToClient (hWnd, (LPPOINT) & Rect.left);
    ScreenToClient (hWnd, (LPPOINT) & Rect.right);
    Rect.left += (BOX_X_MARGIN + 1) / 2;
    Rect.top += (BOX_Y_MARGIN + 1) / 2;
    Rect.right -= (BOX_X_MARGIN + 1) / 2;
    Rect.bottom -= (BOX_Y_MARGIN + 1) / 2;
    nBoxWidth = (Rect.right - Rect.left) / NUM_X_BOXES;
    nBoxHeight = (Rect.bottom - Rect.top) /
        ((COLOR_CUSTOM1 - COLOR_BOX1) / NUM_X_BOXES);

    nDriverColors = 0;  // Assume no colors from driver


#ifdef LATER
    HANDLE hInstDisplay, hRL;
    FARPROC lpfnGetDriverResourceId;
    int    resId;


// ##########################################################################
// **************************************************************************
// FIX FIX FIX ???????  done by [stevecat] per davesn's instructions 7/31/91
// NOTE:  This functionality was in the WIN 3.1 control panel but is not in
//        NT because it would require us to call a SERVER side DLL. So we will
//        just not do it for now.
//
//  [stevecat]
//  3/22/94 No Colors from Driver resources supported in NT Display Drivers.
//          Final mod of code assumes colors are 0 and always uses defaults.
// **************************************************************************
// ##########################################################################

    hInstDisplay = GetModuleHandle (szDisplay);

    // Does this driver have a proc for multires support?

    lpfnGetDriverResourceId = GetProcAddress (hInstDisplay, (LPTSTR)(DWORD)450);
    resId = 2;
    if (lpfnGetDriverResourceId)
    {
        resId = (*lpfnGetDriverResourceId)(2, (LPTSTR)szOEMBIN);
    }

    if (hRL = FindResource (hInstDisplay, (LPTSTR) MAKEINTRESOURCE (resId), szOEMBIN))
    {
        if (hCT = LoadResource (hInstDisplay, hRL))
        {
            if (lpNumColors = (LPINT)LockResource (hCT))
            {
                nDriverColors = (DWORD) *lpNumColors++;

                // More colors than boxes ?

                if (nDriverColors > COLOR_CUSTOM1 - COLOR_BOX1)

                    nDriverColors = COLOR_CUSTOM1 - COLOR_BOX1;

                lpDriverRGB = (DWORD FAR *)lpNumColors;
            }
        }
    }
#endif  // LATER


    for (i = 0; i < COLOR_CUSTOM1 - COLOR_BOX1; i++)
    {

        rColorBox[i].left = Rect.left + nBoxWidth * (i % NUM_X_BOXES);
        rColorBox[i].right = rColorBox[i].left + nBoxWidth - BOX_X_MARGIN;
        rColorBox[i].top = Rect.top + nBoxHeight * (i / NUM_X_BOXES);
        rColorBox[i].bottom = rColorBox[i].top + nBoxHeight - BOX_Y_MARGIN;

        // setup the colors.  If the driver still has colors to give, take it.  If
        // not, if the driver actually gave colors, set the color to white.  Otherwise
        // set to the default colors.

#ifdef LATER
        if (i < nDriverColors)
            rgbBoxColor[i] = *lpDriverRGB++;
        else
            rgbBoxColor[i] = nDriverColors ? 0xFFFFFF : rgbBoxColorDefault[i];
#else
        rgbBoxColor[i] = rgbBoxColorDefault[i];
#endif  // LATER
    }

    // If no driver colors, use default number

    if (!nDriverColors)
        nDriverColors = COLOR_CUSTOM1 - COLOR_BOX1;

#ifdef LATER
    // If we found the color table, unlock and release it

    if (hCT)
    {
        if (lpNumColors) // Was it actually locked ?
            GlobalUnlock (hCT);

        FreeResource (hCT);
    }
#endif  // LATER

    GetWindowRect (hCustom1, &Rect);
    ScreenToClient (hWnd, (LPPOINT) & Rect.left);
    ScreenToClient (hWnd, (LPPOINT) & Rect.right);
    Rect.left += (BOX_X_MARGIN + 1) / 2;
    Rect.top += (BOX_Y_MARGIN + 1) / 2;
    Rect.right -= (BOX_X_MARGIN + 1) / 2;
    Rect.bottom -= (BOX_Y_MARGIN + 1) / 2;

    lstrcpy (szTemp, szColorA);
    for (; i < COLORBOXES; i++)
    {
        rColorBox[i].left = Rect.left +
            nBoxWidth * ((i - (COLOR_CUSTOM1 - COLOR_BOX1)) % NUM_X_BOXES);
        rColorBox[i].right = rColorBox[i].left + nBoxWidth - BOX_X_MARGIN;
        rColorBox[i].top = Rect.top +
            nBoxHeight * ((i - (COLOR_CUSTOM1 - COLOR_BOX1)) / NUM_X_BOXES);
        rColorBox[i].bottom = rColorBox[i].top + nBoxHeight - BOX_Y_MARGIN;
        szTemp[5] = (TCHAR)TEXT('A') + (TCHAR)(i - COLOR_CUSTOM1 + COLOR_BOX1);
        GetPrivateProfileString (szCustomColors, szTemp,
                                 TEXT("FFFFFF"),             // default color: white
                                 szRHS, CharSizeOf(szRHS), szCtlIni);
        rgbBoxColor[i] = hexatol (szRHS);
    }

    nCurBox = nCurDsp = 0;
    nCurMix = COLOR_CUSTOM1 - COLOR_BOX1;
    ChangeColorBox (hWnd, currentRGB);

    if (nCurBox < nCurMix)
        nCurDsp = nCurBox;
    else
        nCurMix = nCurBox;

    SetFocus (GetDlgItem (hWnd, COLOR_ELEMENT));

    GetWindowRect (hWnd, &Rect);
    SetWindowPos(hWnd, NULL, 0, 0, Orig.right - Orig.left,
                 Orig.bottom - Orig.top, SWP_NOMOVE|SWP_NOZORDER);

//  return ((Rect.left != Orig.left) || (Rect.top != Orig.top));
    return TRUE;
}


/* InitColor
   Returns TRUE iff everything's OK.
*/
BOOL InitColor (HWND hWnd)
{
    DWORD    i;
    TCHAR     szTemp[MAX_SCHEMESIZE];
    BOOL     bErrMem = FALSE;
    RECT     Rect;
//    HANDLE   hInstDisplay, hRL, hCT;
//    LPBYTE   lpDefColors, lpItem;
//    LPSTR    pszScan;
//    char     szDefStr[MAX_SCHEMESIZE];
//    FARPROC  lpfnGetDriverResourceId;
//    int         resId;
    HDC      hDC;

    TEXTMETRIC charsize;

//    wHelpMessage = RegisterWindowMessage ("Control Panel: Color Help");

    hDCBits = CreateCompatibleDC (hDC = GetDC (hWnd));

    GetTextMetrics (hDC, &charsize);
    CharHeight = charsize.tmHeight;
    CharWidth  = charsize.tmMaxCharWidth;
    CharExternalLeading = charsize.tmExternalLeading;
    CharDescent = charsize.tmDescent;

    ReleaseDC (hWnd, hDC);

    if (!hUpArrow)
    {                 // NULL --> system

        hUpArrow = LoadBitmap (NULL, (LPTSTR) MAKEINTRESOURCE (OBM_UPARROW));
        hDownArrow = LoadBitmap (NULL, (LPTSTR) MAKEINTRESOURCE (OBM_DNARROW));
    }

    EnableWindow (GetDlgItem (hWnd, COLOR_ELEMENT), FALSE);
    EnableWindow (GetDlgItem (hWnd, COLOR_SAVE), FALSE);
    EnableWindow (GetDlgItem (hWnd, COLOR_MIX), FALSE);

    EnableWindow (hBox1 = GetDlgItem (hWnd, COLOR_BOX1), FALSE);
    EnableWindow (hCustom1 = GetDlgItem (hWnd, COLOR_CUSTOM1), FALSE);

    GetWindowRect (GetDlgItem (hWnd, COLOR_ELEMENT), &Rect);
    GetWindowRect (hWnd, &Orig);
    MoveWindow (hWnd, Orig.left, Orig.top, Rect.left - 1 - Orig.left,
                      Orig.bottom - Orig.top, TRUE);

    // Load the string representing "Fine Tuning", but do not add it to
    // the Combo Box unless it should be there

// bErrMem = bErrMem || !LoadString (hModule, i + COLOR, szTemp, CharSizeOf(szTemp));

//if (bErrMem)
//    return (FALSE);

    if (FillFromControlIni (GetDlgItem (hWnd, COLOR_SCHEMES), szColorSchemes)
         == CB_ERRSPACE)
    {

        TCHAR    szTemp2[MAX_SCHEMESIZE];

        LoadString (hModule, COLOR + COLORSCHEMES, szTemp, CharSizeOf(szTemp));
        LoadString (hModule, COLOR + COLORSCHEMES + 3, szTemp2, CharSizeOf(szTemp2));
        WritePrivateProfileString (szColorSchemes, szTemp, szTemp2, szCtlIni);
        lstrcat (szTemp, szEqual);
        lstrcat (szTemp, szTemp2);
        SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_ADDSTRING, 0, (LONG)szTemp);
    }

#ifdef LATER
// ##########################################################################
// **************************************************************************
// FIX FIX FIX ???????  done by [stevecat] per davesn's instructions 7/31/91
// NOTE:  This functionality was in the WIN 3.1 control panel but is not in
//        NT because it would require us to call a SERVER side DLL. So for now
//        we just load in some default windows values that we have in our local
//        resources.
// **************************************************************************
// ##########################################################################

    hInstDisplay = GetModuleHandle (szDisplay);

    // Does this driver have a proc for multires support?

    lpfnGetDriverResourceId = GetProcAddress (hInstDisplay, (LPTSTR)(DWORD)450);
    resId = 1;

    if (lpfnGetDriverResourceId)
    {
        resId = (*lpfnGetDriverResourceId)(1, (LPTSTR)szOEMBIN);
    }

    if (hRL = FindResource (hInstDisplay, (LPTSTR) MAKEINTRESOURCE (resId), szOEMBIN))
    {
        if (hCT = LoadResource (hInstDisplay, hRL))
        {
            if (lpDefColors = (BYTE far * )LockResource (hCT))
            {
                lpDefColors += 18;
                LoadString (hModule, COLOR + COLORSCHEMES, szDefStr, CharSizeOf(szDefStr));
                lstrcat (szDefStr, szEqual);
                pszScan = szDefStr + lstrlen (szDefStr);

                /* Don't look in the driver for InactiveTitleText
                 * or ButtonHilight; we'll just make them black and white resp.
                 */
                for (i = 0; i < COLORDEFS-2; i++)
                {
                    lpItem = lpDefColors + Xlat[i] * 4;
                    wsprintf(pszScan,TEXT("%lX,"), *(DWORD FAR *)lpItem);
                    pszScan += lstrlen(pszScan);
                }

                lstrcpy(pszScan, TEXT("0,ffffff"));

                SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_INSERTSTRING, 0,
                                          (LONG)szDefStr);
                UnlockResource (hCT);
            }
            FreeResource (hCT);
        }
    }
#else
// This is a temporary fix to get the "Windows Default" color scheme listed
// in the Schemes combobox.
    {
        TCHAR    szTemp2[MAX_SCHEMESIZE];

        LoadString (hModule, COLOR + COLORSCHEMES, szTemp, CharSizeOf(szTemp));
        LoadString (hModule, COLOR + COLORSCHEMES + 3, szTemp2, CharSizeOf(szTemp2));
        lstrcat (szTemp, szEqual);
        lstrcat (szTemp, szTemp2);
        SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_INSERTSTRING, 0, (LONG)szTemp);
    }
#endif  // LATER

    GetPrivateProfileString (szCurrent, szColorSchemes, szNull,
                             szTemp, CharSizeOf(szTemp), szCtlIni);

    if (szTemp[0])
    {                   // there's a RHS here
        i = lstrlen (szTemp);
        GetPrivateProfileString (szColorSchemes, szTemp, szNull, (szTemp + i + 1),
                                 CharSizeOf(szTemp) - i - 1, szCtlIni);
        szTemp[i] = TEXT('=');

        if (SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_SELECTSTRING,
                                (WPARAM)(LONG) -1, (LONG)szTemp) == CB_ERR)
            SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_SETCURSEL, 0, 0L);
    }

    // Enable the Remove button only if a scheme is selected, and if the
    // selected scheme is not the Microsoft Standard (index 0)

    i = SendDlgItemMessage (hWnd, COLOR_SCHEMES, CB_GETCURSEL, 0, 0L);
    EnableWindow (GetDlgItem (hWnd, COLOR_REMOVE), ((i != CB_ERR) && i));

    /* Note the Kluge added here.  I'm forcing BUTTONHIGHLIGHT to get added
       after the other button elements */

    for (i = 0; i < COLORDEFS; i++)
    {
        nElementIndex = ElementLBItems[i];

        lCurColors[nElementIndex]
                = lPrevColors[nElementIndex]
                = GetSysColor(Xlat[nElementIndex]);

        /* init listbox entries */
        bErrMem = bErrMem || !LoadString (hModule, nElementIndex + COLOR,
                                          szTemp, CharSizeOf(szTemp));
        bErrMem = bErrMem || (SendDlgItemMessage (hWnd, COLOR_ELEMENT,
                                  CB_ADDSTRING, 0, (LONG) szTemp) == CBN_ERRSPACE);
    }

    // The following was modofied by C. Stevens, Sept 90.  The indeces
    // for ACTIVESTRING, INACTIVESTRING, etc. are used, and the additional
    // components GRAYTEXTSTRING and HIGHLIGHTTEXTSTRING have been added.

    bErrMem = bErrMem || !LoadString (hModule, COLOR + ACTIVESTRING, szActive, CharSizeOf(szActive));
    bErrMem = bErrMem || !LoadString (hModule, COLOR + INACTIVESTRING, szInactive, CharSizeOf(szInactive));
    bErrMem = bErrMem || !LoadString (hModule, COLOR + MENUTEXTSTRING, szMenu, CharSizeOf(szMenu));
    bErrMem = bErrMem || !LoadString (hModule, COLOR + WINDOWTEXTSTRING, szWindow, CharSizeOf(szWindow));
    bErrMem = bErrMem || !LoadString (hModule, COLOR + GRAYTEXTSTRING, szGrayText, CharSizeOf(szGrayText));
    bErrMem = bErrMem || !LoadString (hModule, COLOR + HIGHLIGHTTEXTSTRING,
                                      szHighlightText, CharSizeOf(szHighlightText));

    currentRGB = lCurColors[nElementIndex = 0];

    cyBorder  = GetSystemMetrics (SM_CYBORDER);
    cyCaption = GetSystemMetrics (SM_CYCAPTION);
    cyMenu    = GetSystemMetrics (SM_CYMENU);
    cyIcon    = GetSystemMetrics (SM_CYICON);
    cyVScroll = GetSystemMetrics (SM_CYVSCROLL);
    cyVThumb  = GetSystemMetrics (SM_CYVTHUMB);

    cxVScroll = GetSystemMetrics (SM_CXVSCROLL);
    cxBorder  = GetSystemMetrics (SM_CXBORDER);
    cxSize    = GetSystemMetrics (SM_CXSIZE);

    fChanged = 0;

    if (!SetupScreenDiagram (hWnd))
        return (FALSE);

    ShowWindow (hWnd, SW_SHOW);
    // UpdateWindow (hWnd);
    return (TRUE);
}


//  bArrow => Non-zero = up arrow

void PaintArrow (HDC hDC, BOOL bArrow)
{
    HANDLE hArrow;
    LPRECT rArrow;
    HANDLE hObjSave;

    hArrow = (bArrow ? hUpArrow : hDownArrow);
    rArrow =  (bArrow ? &rUpArrow : &rDownArrow);

    if (hObjSave = SelectObject(hDCBits, hArrow))
    {
        BitBlt(hDC, rArrow->left, rArrow->top, cxVScroll, cyVScroll, hDCBits,
                                                          0, 0, SRCCOPY);
        SelectObject(hDCBits, hObjSave);
    }
}

// PaintElement updates individual elements within the sample screen
// NOTE: Updating the MDIWINDOW, WINDOWFRAME, or BORDER2 elements
//       alters the Clipping Region to the entire dialog box.

void PaintElement (HWND hWnd, HDC hDC, DWORD nIndex)
{
    HBRUSH hbrSwipe;
    RECT Rect, rButtonInside;
    HRGN hRgn;
    DWORD lTextSave, lBkColor;
    HANDLE hPenSave;
    DWORD nOldBkMode;
    int    i;

    switch (nIndex)
    {

    case BACKGROUND:
        if (hIconRgn)
        {
            if (hbrSwipe = CreateSolidBrush (lCurColors[BACKGROUND]))
            {
                FillRgn (hDC, hIconRgn, hbrSwipe);
                DeleteObject (hbrSwipe);
            }
        }
        break;

    case BORDER:

        if (hbrSwipe = CreateSolidBrush (lCurColors[BORDER]))
        {
            FillRect (hDC, &rBorderTop, hbrSwipe);
            FillRect (hDC, &rBorderBottom, hbrSwipe);
            FillRect (hDC, &rBorderLeft, hbrSwipe);
            FillRect (hDC, &rBorderRight, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        if (hbrSwipe = CreateSolidBrush (
                            GetNearestColor (hDC, lCurColors[WINDOWFRAME])))
        {
            BorderRect (hDC, &rBorderOutline, hbrSwipe);
            BorderRect (hDC, &rBorderInterior, hbrSwipe);
            BorderRect (hDC, &rBorderLeftFrame, hbrSwipe);
            BorderRect (hDC, &rBorderRightFrame, hbrSwipe);
            BorderRect (hDC, &rBorderTopFrame, hbrSwipe);
            BorderRect (hDC, &rBorderBottomFrame, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
        break;

    case BORDER2:

        ExcludeClipRect (hDC, rBorderOutline.left, rBorderOutline.top,
            rBorderOutline.right, rBorderOutline.bottom);
        if (hbrSwipe = CreateSolidBrush (lCurColors[BORDER2]))
        {
            FillRect (hDC, &rBorderTop2, hbrSwipe);
            FillRect (hDC, &rBorderLeft2, hbrSwipe);
            FillRect (hDC, &rBorderRight2, hbrSwipe);
            FillRect (hDC, &rBorderBottom2, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC,
                                                 lCurColors[WINDOWFRAME])))
        {
            BorderRect (hDC, &rBorderOutline2, hbrSwipe);
            BorderRect (hDC, &rBorderInterior2, hbrSwipe);
            BorderRect (hDC, &rBorderRightFrame2, hbrSwipe);
//            BorderRect (hDC, &rBorderTopFrame2, hbrSwipe);
            BorderRect (hDC, &rBorderLeftFrame2, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        // reset the clip region
        GetClientRect (hWnd, &Rect);
        if (hRgn = CreateRectRgnIndirect (&Rect))
        {
            SelectClipRgn (hDC, hRgn);
            DeleteObject (hRgn);
        }
        break;

    case WINDOWFRAME:

        PaintArrow (hDC, UPARROW);
        PaintArrow (hDC, DOWNARROW);

        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC,
                                                lCurColors[WINDOWFRAME])))
        {
            BorderRect (hDC, &rBorderOutline, hbrSwipe);
            BorderRect (hDC, &rBorderInterior, hbrSwipe);
            BorderRect (hDC, &rBorderLeftFrame, hbrSwipe);
            BorderRect (hDC, &rBorderRightFrame, hbrSwipe);
            BorderRect (hDC, &rBorderTopFrame, hbrSwipe);
            BorderRect (hDC, &rBorderBottomFrame, hbrSwipe);
            BorderRect (hDC, &rMenuFrame, hbrSwipe);
            BorderRect (hDC, &rMenuFrame2, hbrSwipe);
            BorderRect (hDC, &rClientFrame, hbrSwipe);
            BorderRect (hDC, &rScrollFrame, hbrSwipe);
            rScroll.left--, rScroll.right++;
            BorderRect (hDC, &rScroll, hbrSwipe);
            rScroll.left++, rScroll.right--;

            // This added by C. Stevens, Sept 90.  Draw outline of PullDown menu

            BorderRect (hDC, &rPullDown, hbrSwipe);

            ExcludeClipRect (hDC, rBorderOutline.left, rBorderOutline.top,
                rBorderOutline.right, rBorderOutline.bottom);

            // Bug fix here by C. Stevens, Sept 90.  rBorderLeftFrame2 and
              // rBorderTopFrame2 were not being drawn

            BorderRect (hDC, &rBorderOutline2, hbrSwipe);
            BorderRect (hDC, &rBorderInterior2, hbrSwipe);
            BorderRect (hDC, &rBorderRightFrame2, hbrSwipe);
            BorderRect (hDC, &rBorderLeftFrame2, hbrSwipe);
            BorderRect (hDC, &rBorderTopFrame2, hbrSwipe);
//            BorderRect (hDC, &rBorderBottomFrame2, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        // reset the clip region

        GetClientRect (hWnd, &Rect);
        if (hRgn = CreateRectRgnIndirect (&Rect))
        {
            SelectClipRgn (hDC, hRgn);
            DeleteObject (hRgn);
        }
        break;


    case MYCAPTION:

        if (hbrSwipe = CreateSolidBrush (lCurColors[MYCAPTION]))
        {
            FillRect (hDC, &rCaptionLeft, hbrSwipe);
            FillRect (hDC, &rCaptionText, hbrSwipe);
            FillRect (hDC, &rCaptionRight, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
        PaintElement (hWnd, hDC, CAPTIONTEXT);
        break;

    case CAPTION2:

        if (hbrSwipe = CreateSolidBrush (lCurColors[CAPTION2]))
        {
            FillRect (hDC, &rCaptionLeft2, hbrSwipe);
            FillRect (hDC, &rCaptionText2, hbrSwipe);
            FillRect (hDC, &rCaptionRight2, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
        PaintElement (hWnd, hDC, CAPTION2TEXT);
        break;

    case CAPTIONTEXT:

        lTextSave = SetTextColor (hDC, GetNearestColor (hDC, lCurColors[CAPTIONTEXT]));
        nOldBkMode = SetBkMode (hDC, TRANSPARENT);
        TextOut (hDC, ptTitleText.x, ptTitleText.y, szActive, lstrlen (szActive));
        PaintArrow (hDC, UPARROW);
        PaintArrow (hDC, DOWNARROW);
        SetBkMode (hDC, nOldBkMode);
        SetTextColor (hDC, lTextSave);
        break;

    case CAPTION2TEXT:

        lTextSave = SetTextColor (hDC,
            GetNearestColor (hDC, lCurColors[CAPTION2TEXT]));
        nOldBkMode = SetBkMode (hDC, TRANSPARENT);
        TextOut (hDC, ptTitleText2.x, ptTitleText2.y, szInactive,
            lstrlen (szInactive));
        SetBkMode (hDC, nOldBkMode);
        SetTextColor (hDC, lTextSave);
        break;

    case MENUBAR:

        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC,
                                                    lCurColors[MENUBAR])))
        {
            FillRect (hDC, &rMenuBar, hbrSwipe);
            FillRect (hDC, &rMenuBar2, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        PaintElement (hWnd, hDC, MENUTEXT);
        PaintElement (hWnd, hDC, GRAYTEXT);

        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC,
                                                lCurColors[WINDOWFRAME])))
        {
            BorderRect (hDC, &rMenuFrame2, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
        break;

    case MENUTEXT:

        lTextSave = SetTextColor (hDC, GetNearestColor (hDC, lCurColors[MENUTEXT]));
        lBkColor = SetBkColor (hDC,    GetNearestColor (hDC, lCurColors[MENUBAR]));
        nOldBkMode = SetBkMode (hDC, OPAQUE);
        TextOut (hDC, ptMenuText.x, ptMenuText.y, szMenu, lstrlen (szMenu));
        SetBkMode (hDC, nOldBkMode);
        SetBkColor (hDC, lBkColor);
        SetTextColor (hDC, lTextSave);
        break;

    case MDIWINDOW:

        ExcludeClipRect (hDC, rClientFrame.left, rClientFrame.top,
                                    rClientFrame.right, rClientFrame.bottom);
        ExcludeClipRect (hDC, rPullDown.left, rPullDown.top,
                                    rPullDown.right, rPullDown.bottom);
        if (hbrSwipe = CreateSolidBrush (lCurColors[MDIWINDOW]))
        {
            FillRect (hDC, &rMDIWindow, hbrSwipe);
            FillRect (hDC, &rMDIWindow2, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        // reset the clip region

        GetClientRect (hWnd, &Rect);
        if (hRgn = CreateRectRgnIndirect (&Rect))
        {
            SelectClipRgn (hDC, hRgn);
            DeleteObject (hRgn);
        }
        break;

    case CLIENT:

        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC, lCurColors[CLIENT])))
        {
            FillRect (hDC, &rClient, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        PaintElement (hWnd, hDC, CLIENTTEXT);

        PaintElement (hWnd, hDC, BUTTONFACE);
        break;

    case HIGHLIGHT:
    case HIGHLIGHTTEXT:
    case GRAYTEXT:

        // Draw menu outline and fill interior with menu bar colour

        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC,
                                                lCurColors[WINDOWFRAME])))
        {
            BorderRect (hDC, &rPullDown, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC,
                                                        lCurColors[MENUBAR])))
        {
            FillRect (hDC, &rPullInside, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        // Draw disabled text

        lBkColor = SetBkColor (hDC, GetNearestColor (hDC, lCurColors[MENUBAR]));
        lTextSave = SetTextColor (hDC, GetNearestColor (hDC, lCurColors[GRAYTEXT]));
        DrawText (hDC, szGrayText, -1, &rGrayText, DT_LEFT | DT_SINGLELINE);

        // Draw highlighted text

        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC,
                                                    lCurColors[HIGHLIGHT])))
        {
            FillRect (hDC, &rHighlight, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
        SetBkColor (hDC, GetNearestColor (hDC, lCurColors[HIGHLIGHT]));
        SetTextColor (hDC, GetNearestColor (hDC, lCurColors[HIGHLIGHTTEXT]));
        DrawText (hDC, szHighlightText, -1, &rHighlight, DT_LEFT | DT_SINGLELINE);

        // Restore colour settings

        SetBkColor (hDC, lBkColor);
        SetTextColor (hDC, lTextSave);
        break;

    case CLIENTTEXT:

        lBkColor = SetBkColor (hDC, GetNearestColor (hDC, lCurColors[CLIENT]));
        lTextSave = SetTextColor (hDC, GetNearestColor (hDC, lCurColors[CLIENTTEXT]));

        nOldBkMode = SetBkMode (hDC, TRANSPARENT);

        DrawText (hDC, szWindow, -1, &rClient, DT_CENTER | DT_WORDBREAK);

        SetBkMode (hDC, nOldBkMode);
        SetTextColor (hDC, lTextSave);
        SetBkColor (hDC, lBkColor);
        break;

    case SCROLLBARS:

        if (hbrSwipe = CreateSolidBrush (lCurColors[SCROLLBARS]))
        {
            FillRect (hDC, &rScroll, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
        rScroll.left--, rScroll.right++;
        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC,
                                                    lCurColors[WINDOWFRAME])))
        {
            BorderRect (hDC, &rScroll, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
        rScroll.left++, rScroll.right--;
        break;

    case BUTTONFACE:
    case BUTTONSHADOW:
    case BUTTONTEXT:
    case BUTTONHIGHLIGHT:
    {
        TCHAR szOK[10];
        HBRUSH hbrOld;
        HPEN hpenOld;

        CopyRect (&rButtonInside, &rButton);
        InflateRect (&rButtonInside, -cxBorder, -cyBorder);
        lTextSave = SetTextColor (hDC,
                            GetNearestColor (hDC, lCurColors[BUTTONTEXT]));
        lBkColor = SetBkColor (hDC,
                            GetNearestColor (hDC, lCurColors[BUTTONFACE]));
        if (hbrSwipe=CreateSolidBrush(GetNearestColor(hDC, lCurColors[CLIENT])))
            hbrOld = SelectObject(hDC, hbrSwipe);
        if (hPenSave=CreatePen(PS_SOLID, 1, GetNearestColor(hDC,
                                                    lCurColors[WINDOWFRAME])))
            hpenOld = SelectObject(hDC, hPenSave);

        // Draw button border

        Rectangle (hDC, rButton.left, rButton.top, rButton.right, rButton.bottom);

        // Clip corners of rectangle

        PatBlt (hDC, rButton.left, rButton.top, cxBorder, cyBorder, PATCOPY);
        PatBlt (hDC, rButton.right - cxBorder, rButton.top, cxBorder, cyBorder,
                                                                PATCOPY);
        PatBlt (hDC, rButton.left, rButton.bottom - cyBorder, cxBorder, cyBorder,
                                                                PATCOPY);
        PatBlt (hDC, rButton.right - cxBorder, rButton.bottom - cyBorder,
                                                cxBorder, cyBorder, PATCOPY);

        // Draw left and top shadows

        if (hbrSwipe)
        {
            if (hbrOld)
                SelectObject(hDC, hbrOld);
            DeleteObject(hbrSwipe);
        }

        if (hbrSwipe=CreateSolidBrush(lCurColors[BUTTONHIGHLIGHT]))
            hbrOld = SelectObject(hDC, hbrSwipe);

        PatBlt (hDC, rButtonInside.left, rButtonInside.top, 2 * cxBorder,
                            rButtonInside.bottom - rButtonInside.top, PATCOPY);
        PatBlt (hDC, rButtonInside.left, rButtonInside.top,
                    rButtonInside.right - rButtonInside.left, 2 * cyBorder,
                    PATCOPY);

        // Draw bottom and right shadows

        if (hbrSwipe)
        {
            if (hbrOld)
                SelectObject(hDC, hbrOld);
            DeleteObject(hbrSwipe);
        }

        if (hbrSwipe=CreateSolidBrush(lCurColors[BUTTONSHADOW]))
            hbrOld = SelectObject(hDC, hbrSwipe);

        rButtonInside.bottom -= cyBorder;
        rButtonInside.right -= cxBorder;

        for (i = 0; i <= 2; i++)
        {
            PatBlt (hDC, rButtonInside.left, rButtonInside.bottom,
                rButtonInside.right - rButtonInside.left + cxBorder,
                cyBorder, PATCOPY);
            PatBlt (hDC, rButtonInside.right, rButtonInside.top, cxBorder,
                rButtonInside.bottom - rButtonInside.top, PATCOPY);

            if (i == 0)
                InflateRect ( &rButtonInside, -cxBorder, -cyBorder);
        }

        // Draw the button face

        if (hbrSwipe)
        {
            if (hbrOld)
                SelectObject(hDC, hbrOld);
            DeleteObject(hbrSwipe);
        }

        if (hbrSwipe=CreateSolidBrush(GetNearestColor (hDC,
                                                       lCurColors[BUTTONFACE])))
            hbrOld = SelectObject(hDC, hbrSwipe);

        rButtonInside.left += cxBorder;
        rButtonInside.top += cyBorder;
        PatBlt (hDC, rButtonInside.left, rButtonInside.top,
            rButtonInside.right - rButtonInside.left,
            rButtonInside.bottom - rButtonInside.top, PATCOPY);

        // Draw the button text

        i = LoadString(hModule, COLOR+SCHEMEERR+5, szOK, CharSizeOf(szOK));
        DrawText (hDC, szOK, i, &rButton, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Restore everything

        if (hbrSwipe)
        {
            if (hbrOld)
                SelectObject(hDC, hbrOld);
            DeleteObject(hbrSwipe);
        }

        if (hPenSave)
        {
            if (hpenOld)
                SelectObject(hDC, hpenOld);
            DeleteObject(hPenSave);
        }

        SetTextColor (hDC, lTextSave);
        SetBkColor (hDC, lBkColor);
        break;
    }

    case ALLELEMENTS:

        PaintElement (hWnd, hDC, BACKGROUND);
        PaintElement (hWnd, hDC, BORDER);
        PaintElement (hWnd, hDC, MYCAPTION);
        PaintElement (hWnd, hDC, CAPTION2);
        PaintElement (hWnd, hDC, MENUBAR);
        PaintElement (hWnd, hDC, CLIENT);
        PaintElement (hWnd, hDC, SCROLLBARS);

        // Update these three last, since they alter the clipping region

        PaintElement (hWnd, hDC, MDIWINDOW);
        PaintElement (hWnd, hDC, WINDOWFRAME);  // Includes Caption Text

        PaintElement (hWnd, hDC, BORDER2);
        break;
    }
}


void ColorPaint (HWND hWnd, HDC hDC, LPRECT lpPaintRect)
{
    RECT Rect;
    short    i;

    // Paint the Current Screen Diagram Color Samples

    if (IntersectRect (&Rect, lpPaintRect, &rSamples))
    {
        PaintElement (hWnd, hDC, ALLELEMENTS);
    }

    if (bTuning)
    {
        for (i = 0; i < COLORBOXES; i++)
        {
            PaintBox (hDC, i);
        }

        if ((GetFocus () == hBox1) || (GetFocus () == hCustom1))
        {
            PaintBox (hDC, nCurDsp);
            PaintBox (hDC, nCurMix);
        }
    }

    return;

    hWnd = hWnd;
}


void StoreToWin (HWND  hWnd)
{
    int    nIndex;
    TCHAR    szRGB[13];         // build up string to write to win.ini

    TCHAR    szColor[5];        // single color translated

    BOOL bWritten = TRUE;

    for (nIndex = 0; bWritten && (nIndex < COLORDEFS); nIndex++)
    {
        MyItoa (GetRValue (lCurColors[nIndex]), szColor, 10);
        lstrcpy (szRGB, szColor);
        lstrcat (szRGB, szSpace);
        MyItoa (GetGValue (lCurColors[nIndex]), szColor, 10);
        lstrcat (szRGB, szColor);
        lstrcat (szRGB, szSpace);
        MyItoa (GetBValue (lCurColors[nIndex]), szColor, 10);
        lstrcat (szRGB, szColor);
        bWritten = bWritten && WriteProfileString (szColors,
                                                pszWinStrings[nIndex], szRGB);
    }

    SendWinIniChange (szColors);

    if (!bWritten)
        MyMessageBox(hWnd, UTILS+1, INITS+1, MB_OK|MB_ICONINFORMATION);
}


// HWND  hWnd,        Handle to List/Combobox
// LPSTR pszSection   pointer to section in SETUP.INF

DWORD FillFromControlIni (HWND  hWnd, LPTSTR pszSection)
{
    TCHAR   szItem[MAX_SCHEMESIZE];
    DWORD   nCount;
    DWORD   nSize;
    HANDLE  hLocal = INVALID_HANDLE_VALUE;
    LPTSTR  pszItem = NULL;


    if (!(hLocal = LocalAlloc (LMEM_MOVEABLE, ByteCountOf(nSize = 4096))))
        return ((DWORD)(LONG)CB_ERRSPACE);

    if (!(pszItem = LocalLock (hLocal)))
    {
        LocalFree (hLocal);
        return ((DWORD)(LONG)CB_ERRSPACE);
    }

    do
    {
        nCount = GetPrivateProfileString (pszSection, NULL, szNull, pszItem,
                                          nSize, szCtlIni);
        if (nCount >= nSize)
        {
            LocalUnlock (hLocal);

            if (!(hLocal = LocalReAlloc (hLocal, ByteCountOf(nSize += 4096), LMEM_MOVEABLE)))
                return ((DWORD)(LONG)CB_ERRSPACE);

            if (!(pszItem = LocalLock (hLocal)))
            {
                nCount = (DWORD)(LONG) CB_ERRSPACE;
                goto onlyfree;
            }
            nCount = nSize;
        }

    } while (nCount == nSize);

    while (*pszItem)
    {
        nSize = lstrlen (pszItem);

        GetPrivateProfileString (pszSection, pszItem, szNull, (szItem + nSize + 1),
                                 CharSizeOf(szItem) - nSize - 1, szCtlIni);

        if (*(szItem + nSize + 1))
        {                     // there's a RHS here
            lstrcpy (szItem, pszItem);
            szItem[nSize] = TEXT('=');

            if ((nCount = SendMessage (hWnd, CB_ADDSTRING, 0, (LONG)szItem))
                 == CB_ERRSPACE)
                goto getout;
        }
        pszItem += nSize + 1;                    /* advance to next LHS */
    }

getout:
    LocalUnlock (hLocal);

onlyfree:
    LocalFree (hLocal);

    return (nCount);
}
