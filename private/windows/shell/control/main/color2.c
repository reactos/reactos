/** FILE: color2.c ********* Module Header ********************************
 *
 *  Control panel applet for Color configuration.  This file holds
 *  definitions and other common include file items that deal with custom
 *  colors and color schemes.  The Rainbox Color Dialog procedure is in this
 *  file.
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

// Application specific
#include "main.h"


//==========================================================================
//                            Local Definitions
//==========================================================================
#define  RANGE   240                 /* range of values for HLS scrollbars */
                                     /* HLS-RGB conversions work best when
                                        RANGE is divisible by 6 */
#define HUEINC 4
#define SATINC 8
#define LUMINC 8
#define SETHUEUNDEFINED 1
#define VERTICALSPLIT   0
#define ACTIVATECHANGE  1

#define RGBBOX 0

#define RGBBOXES    9
#define RGBBOXRES   9
#define RGBOFFSETX 15
#define RGBOFFSETY  5

#define  HLSMAX   RANGE
#define  RGBMAX   255


//==========================================================================
//                            External Declarations
//==========================================================================
/* Functions */

/* Data */

//==========================================================================
//                            Local Data Declarations
//==========================================================================

//==========================================================================
//                            Local Function Prototypes
//==========================================================================

//==========================================================================
//                                Functions
//==========================================================================

void ChangeColorSettings (HWND  hWnd, DWORD dwRGBcolor)
{
    HDC hDC;

    RGBtoHLS (dwRGBcolor);
    if ((WORD) L != currentLum)
    {
        hDC = GetDC (hWnd);
        EraseLumArrow (hDC);
        currentLum = L;
        HLStoHLSPos (COLOR_LUM);
        LumArrowPaint (hDC);
        ReleaseDC (hWnd, hDC);
    }

    if (((WORD) H != currentHue) || ((WORD) S != currentSat))
    {
        currentHue = H;
        currentSat = S;
        InvalidateRect (hWnd, &rLumPaint, FALSE);
        hDC = GetDC (hWnd);
        EraseCrossHair (hDC);
        HLStoHLSPos (COLOR_HUE);
        HLStoHLSPos (COLOR_SAT);
        CrossHairPaint (hDC, nHuePos, nSatPos);
        ReleaseDC (hWnd, hDC);
    }

    // Send a message to the parent noting color change ?

    InvalidateRect (hWnd, &rColorSamples, FALSE);
    UpdateWindow (hWnd);
}


void LumArrowPaint (HDC hDC)
{
    HPEN   hPen;
    HBRUSH hBrush;
    POINT  Triang[3];
    int d;

    d = ((rLumScroll.left+2)-(rLumScroll.right-1));

    Triang[0].x = rLumScroll.left + 2;
    Triang[0].y = nLumPos;
    Triang[1].x = Triang[2].x = rLumScroll.right - 1;
//  Triang[1].y = nLumPos - (cyCaption >> 2);
    Triang[1].y = nLumPos - d;
//  Triang[2].y = nLumPos + (cyCaption >> 2);
    Triang[2].y = nLumPos + d;

    hPen = SelectObject(hDC, GetStockObject(BLACK_PEN));
    hBrush = SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    Polygon(hDC, (LPPOINT) Triang, 3);
    SelectObject(hDC, hPen);
    SelectObject(hDC, hBrush);
    return;
}


void EraseLumArrow (HDC hDC)
{
    HBRUSH hBrush;
    RECT   Rect;
    int d;

    if (hBrush = CreateSolidBrush(GetNearestColor(hDC, lPrevColors[CLIENT])))
    {
        d = ((rLumScroll.left+2)-(rLumScroll.right-1));
        Rect.left = rLumScroll.left + 1;
        //  Rect.top = nLumPos - (cyCaption >> 2) - 1;
        Rect.top = nLumPos - d + 1;
        Rect.right = rLumScroll.right;
        //  Rect.bottom = nLumPos + (cyCaption >> 2) + 1;
        Rect.bottom = nLumPos + d - 1;
        FillRect(hDC, (LPRECT) &Rect, hBrush);
        DeleteObject(hBrush);
    }
}


void EraseCrossHair (HDC hDC)
{
    HBITMAP hOldBitmap;
    DWORD distancex, distancey;
    DWORD topy, bottomy, leftx, rightx;

    distancex = 10 * cxBorder;
    distancey = 10 * cyBorder;
    topy = ((DWORD)rRainbow.top > nSatPos - distancey) ?
        (DWORD)rRainbow.top : nSatPos - distancey;
    bottomy = ((DWORD)rRainbow.bottom < nSatPos + distancey) ?
        (DWORD)rRainbow.bottom : nSatPos + distancey;
    leftx = ((DWORD)rRainbow.left > nHuePos - distancex) ?
        (DWORD)rRainbow.left : nHuePos - distancex;
    rightx = ((DWORD)rRainbow.right < nHuePos + distancex) ?
        (DWORD)rRainbow.right : nHuePos + distancex;

    hOldBitmap = SelectObject (hDCBits, hRainbowBitmap);
    BitBlt (hDC, leftx, topy, rightx - leftx + 1, bottomy - topy + 1,
        hDCBits, leftx - (DWORD)rRainbow.left, topy - (DWORD)rRainbow.top, SRCCOPY);
    SelectObject (hDCBits, hOldBitmap);
    return;
}


void CrossHairPaint (HDC hDC, DWORD x, DWORD y)
{
    DWORD distancex, distancey;
    DWORD topy, bottomy, topy2, bottomy2;
    DWORD leftx, rightx, leftx2, rightx2;
    POINT    tPoint;

    distancex = 5 * cxBorder;
    distancey = 5 * cyBorder;
    topy = ((DWORD)rRainbow.top > y - 2 * distancey) ? (DWORD)rRainbow.top : y - 2 * distancey;
    bottomy = ((DWORD)rRainbow.bottom < y + 2 * distancey) ? (DWORD)rRainbow.bottom
                                                    : y + 2 * distancey;
    leftx = ((DWORD)rRainbow.left > x - 2 * distancex) ? (DWORD)rRainbow.left
                                                    : x - 2 * distancex;
    rightx = ((DWORD)rRainbow.right < x + 2 * distancex) ? (DWORD)rRainbow.right
                                                    : x + 2 * distancex;
    topy2 = ((DWORD)rRainbow.top > y - distancey) ? (DWORD)rRainbow.top : y - distancey;
    bottomy2 = ((DWORD)rRainbow.bottom < y + distancey) ? (DWORD)rRainbow.bottom
                                                    : y + distancey;
    leftx2 = ((DWORD)rRainbow.left > x - distancex) ? (DWORD)rRainbow.left : x - distancex;
    rightx2 = ((DWORD)rRainbow.right < x + distancex) ? (DWORD)rRainbow.right : x + distancex;

    if ((DWORD)rRainbow.top < topy2)
    {
        if ((x - 1) >= (DWORD)rRainbow.left)
        {
            MoveToEx (hDC, x - 1, topy2, &tPoint);
            LineTo (hDC, x - 1, topy);
        }

        if (x < (DWORD)rRainbow.right)
        {
            MoveToEx (hDC, x, topy2, &tPoint);
            LineTo (hDC, x, topy);
        }

        if ((x + 1) < (DWORD)rRainbow.right)
        {
            MoveToEx (hDC, x + 1, topy2, &tPoint);
            LineTo (hDC, x + 1, topy);
        }
    }

    if ((DWORD)rRainbow.bottom > bottomy2)
    {
        if ((x - 1) >= (DWORD)rRainbow.left)
        {
            MoveToEx (hDC, x - 1, bottomy2, &tPoint);
            LineTo (hDC, x - 1, bottomy);
        }

        if (x < (DWORD)rRainbow.right)
        {
            MoveToEx (hDC, x, bottomy2, &tPoint);
            LineTo (hDC, x, bottomy);
        }

        if ((x + 1) < (DWORD)rRainbow.right)
        {
            MoveToEx (hDC, x + 1, bottomy2, &tPoint);
            LineTo (hDC, x + 1, bottomy);
        }
    }

    if ((DWORD)rRainbow.left < leftx2)
    {
        if ((y - 1) >= (DWORD)rRainbow.top)
        {
            MoveToEx (hDC, leftx2, y - 1, &tPoint);
            LineTo (hDC, leftx, y - 1);
        }

        if (y < (DWORD)rRainbow.bottom)
        {
            MoveToEx (hDC, leftx2, y, &tPoint);
            LineTo (hDC, leftx, y);
        }

        if ((y + 1) < (DWORD)rRainbow.bottom)
        {
            MoveToEx (hDC, leftx2, y + 1, &tPoint);
            LineTo (hDC, leftx, y + 1);
        }
    }

    if ((DWORD)rRainbow.right > rightx2)
    {
        if ((y - 1) >= (DWORD)rRainbow.top)
        {
            MoveToEx (hDC, rightx2, y - 1, &tPoint);
            LineTo (hDC, rightx, y - 1);
        }

        if (y < (DWORD)rRainbow.bottom)
        {
            MoveToEx (hDC, rightx2, y, &tPoint);
            LineTo (hDC, rightx, y);
        }

        if ((y + 1) < (DWORD)rRainbow.bottom)
        {
            MoveToEx (hDC, rightx2, y + 1, &tPoint);
            LineTo (hDC, rightx, y + 1);
        }
    }
    return;
}


void NearestSolid (HWND hDlg)
{
    HDC hDC;
    DWORD TempRGB;

    hDC = GetDC (hDlg);
    TempRGB = GetNearestColor (hDC, rainbowRGB);

    if (TempRGB == rainbowRGB)
    {                         // If color is already solid return immediately
        ReleaseDC (hDlg, hDC);
        return;
    }

    rainbowRGB = TempRGB;
    EraseCrossHair (hDC);
    EraseLumArrow (hDC);
    RGBtoHLS (rainbowRGB = GetNearestColor (hDC, rainbowRGB));
    currentHue = H;
    currentLum = L;
    currentSat = S;
    HLStoHLSPos (0);
    CrossHairPaint (hDC, nHuePos, nSatPos);
    LumArrowPaint (hDC);
    ReleaseDC (hDlg, hDC);
    SetHLSEdit (0);
    SetRGBEdit (0);
    InvalidateRect (hDlg, (LPRECT) & rColorSamples, FALSE);
    InvalidateRect (hDlg, (LPRECT) & rLumPaint, FALSE);
    return;
}


void SetupRainbowCapture (HWND  hWnd)
{
    HWND hCurrentColor;

    GetWindowRect (GetDlgItem (hWnd, COLOR_RAINBOW), &rRainbow);
    rRainbow.left++, rRainbow.top++;
    rRainbow.right--, rRainbow.bottom--;
    CopyRect (&rRainbowCapture, &rRainbow);
    ScreenToClient (hWnd, (LPPOINT) & rRainbow.left);
    ScreenToClient (hWnd, (LPPOINT) & rRainbow.right);

    GetWindowRect (GetDlgItem (hWnd, COLOR_LUMSCROLL), &rLumPaint);
    CopyRect (&rLumCapture, &rLumPaint);
    rLumCapture.right += (cxSize >> 1);

    ScreenToClient (hWnd, (LPPOINT) & rLumPaint.left);
    ScreenToClient (hWnd, (LPPOINT) & rLumPaint.right);
    CopyRect (&rLumScroll, &rLumPaint);
    rLumScroll.left = rLumScroll.right;
    rLumScroll.right += (cxSize >> 1);
    nLumHeight = (WORD)(rLumPaint.bottom - rLumPaint.top);

    hCurrentColor = GetDlgItem (hWnd, COLOR_CURRENT);
    GetWindowRect (hCurrentColor, &rCurrentColor);

#if VERTICALSPLIT

    ++rCurrentColor.top;
    rNearestPure.bottom = rCurrentColor.bottom - 1;
    rNearestPure.left = ++rCurrentColor.left;
    rNearestPure.right = --rCurrentColor.right;
    rCurrentColor.bottom += rCurrentColor.top;
    rCurrentColor.bottom /= 2;
    rNearestPure.top = rCurrentColor.bottom;

#else

    ++rCurrentColor.left;
    rNearestPure.right = rCurrentColor.right - 1;
    rNearestPure.top = ++rCurrentColor.top;
    rNearestPure.bottom = --rCurrentColor.bottom;
    rCurrentColor.right += rCurrentColor.left;
    rCurrentColor.right /= 2;
    rNearestPure.left = rCurrentColor.right;

#endif

    ScreenToClient (hWnd, (LPPOINT) & rCurrentColor.left);
    ScreenToClient (hWnd, (LPPOINT) & rCurrentColor.right);
    ScreenToClient (hWnd, (LPPOINT) & rNearestPure.left);
    ScreenToClient (hWnd, (LPPOINT) & rNearestPure.right);

    rColorSamples.left = rCurrentColor.left;
    rColorSamples.right = rNearestPure.right;
    rColorSamples.top = rCurrentColor.top;
    rColorSamples.bottom = rNearestPure.bottom;

    return;
}


BOOL APIENTRY RainbowDlg (HWND hWnd, UINT message, DWORD wParam, LONG lParam)
{
    BOOL        bHighLum, bTemp;
    PAINTSTRUCT ps;
    HDC         hDC;
    short       id;
    int         nVal, nNewVal;
    int         nDelta, limit;
    BOOL        bOK;
    BOOL        bUpdateExample = FALSE;
    DWORD       nOldBkMode;
    TCHAR       cEdit[3];
    POINT       Point;
    DWORD       dwSave;

    switch (message)
    {

    case WM_INITDIALOG:

        HourGlass (TRUE); // change cursor to hourglass

        bTemp = InitRainbow (hWnd);

        HourGlass (FALSE); // change cursor back to arrow

        if (!bTemp)
        {
            EndDialog (hWnd, 0L);
            break;
        }

        ShowWindow (hWnd, SHOW_OPENWINDOW); // make visible

        UpdateWindow (hWnd);
        break;

#if ACTIVATECHANGE
    case WM_ACTIVATE:

        if (wParam)
            ChangeColorSettings (hWnd, rainbowRGB);

        return (FALSE);
        break;
#endif

    case WM_RBUTTONDOWN:
        break;

    case WM_LBUTTONDBLCLK:

        Point.x = (SHORT)LOWORD(lParam);
        Point.y = (SHORT)HIWORD(lParam);

        if (PtInRect (&rNearestPure, Point))
        {
            NearestSolid (hWnd);
            bUpdateExample = TRUE;
        }
        break;


    case WM_MOUSEMOVE:        // Dialog Boxes don't receive MOUSEMOVE unless
        // mouse is captured
        if (!bMouseCapture)    // if mouse isn't captured, break
            break;

    case WM_LBUTTONDOWN:

        Point.x = (SHORT)LOWORD(lParam);
        Point.y = (SHORT)HIWORD(lParam);

        if (PtInRect (&rRainbow, Point))
        {
            SetupRainbowCapture (hWnd);
            bUpdateExample = TRUE;

            if (message == WM_LBUTTONDOWN)
            {
                hDC = GetDC (hWnd);
                EraseCrossHair (hDC);
                ReleaseDC (hWnd, hDC);
            }

            nHuePos = (WORD)Point.x;
            HLSPostoHLS (COLOR_HUE);
            SetHLSEdit (COLOR_HUE);

            nSatPos = HIWORD(lParam);
            HLSPostoHLS (COLOR_SAT);
            SetHLSEdit (COLOR_SAT);
            rainbowRGB = HLStoRGB (currentHue, currentLum, currentSat);

            hDC = GetDC (hWnd);
            RainbowPaint (hDC, (LPRECT) & rLumPaint);
            RainbowPaint (hDC, (LPRECT) & rColorSamples);
            ReleaseDC (hWnd, hDC);

            SetRGBEdit (0);
            SetCapture (hWnd);
            ClipCursor ((LPRECT) & rRainbowCapture);
            bMouseCapture = TRUE;
        }
        else if (PtInRect (&rLumPaint, Point) ||
            PtInRect (&rLumScroll, Point))
        {
            SetupRainbowCapture (hWnd);
            bUpdateExample = TRUE;

            hDC = GetDC (hWnd);
            EraseLumArrow (hDC);
            nLumPos = (WORD)Point.y;
            LumArrowPaint (hDC);
            HLSPostoHLS (COLOR_LUM);
            SetHLSEdit (COLOR_LUM);
            rainbowRGB = HLStoRGB (currentHue, currentLum, currentSat);

            RainbowPaint (hDC, &rColorSamples);
            ReleaseDC (hWnd, hDC);
            ValidateRect (hWnd, &rLumScroll);
            ValidateRect (hWnd, &rColorSamples);

            SetRGBEdit (0);
            SetCapture (hWnd);
            ClipCursor (&rLumCapture);
            bMouseCapture = TRUE;
        }

        break;

    case WM_LBUTTONUP:

        Point.x = (SHORT)LOWORD(lParam);
        Point.y = (SHORT)HIWORD(lParam);

        if (bMouseCapture)
        {
            bMouseCapture = FALSE;
            ReleaseCapture ();
            ClipCursor ((LPRECT) NULL);

            if (PtInRect (&rRainbow, Point))
            {
                bUpdateExample = TRUE;
                hDC = GetDC (hWnd);
                nHuePos = (WORD)Point.x;
                nSatPos = (WORD)Point.y;
                CrossHairPaint (hDC, nHuePos, nSatPos);
                RainbowPaint (hDC, &rLumPaint);
                ReleaseDC (hWnd, hDC);
                ValidateRect (hWnd, &rRainbow);
            }
            else if (PtInRect (&rLumPaint, Point))
            {
                bUpdateExample = TRUE;           // Update Sample Shown

                hDC = GetDC (hWnd);
                LumArrowPaint (hDC);
                ReleaseDC (hWnd, hDC);
                ValidateRect (hWnd, &rLumPaint);
            }
        }

        break;

    case WM_KEYDOWN:

        bTemp = 0;
        switch (wParam)
        {

        case VK_UP:

            if (currentSat < RANGE - 1)
            {
                bTemp++;
                currentSat += 1;
                bHighLum = (currentLum > RANGE / 2);

                if (currentSat > (WORD) (2 * (bHighLum ?
                    RANGE - currentLum
                     : currentLum)))
                {
                    if (bHighLum)
                    {
                        currentLum--;
                    }
                    else
                    {
                        currentLum++;
                    }
                }
            }
            break;

        case VK_DOWN:

            if (currentSat)
            {
                bTemp++;
                currentSat -= 1;
            }
            break;

        case VK_LEFT:

            if (currentLum)
            {
                bTemp++;
                currentLum -= 1;
                bHighLum = (currentLum > RANGE / 2);

                if (currentSat > (WORD) (2 * (bHighLum ?
                    RANGE - currentLum
                     : currentLum)))
                {
                    if (bHighLum)
                    {
                        currentSat--;
                    }
                    else
                    {
                        currentSat++;
                    }
                }
            }
            break;

        case VK_RIGHT:

            if (currentLum < RANGE - 1)
            {
                bTemp++;
                currentLum += 1;
                bHighLum = (currentLum > RANGE / 2);
                if (currentSat > (WORD) (2 * (bHighLum ?
                    RANGE - currentLum
                     : currentLum)))
                {
                    currentSat--;
                }
            }

            break;

        }
        break;

    case WM_VSCROLL:

        for (id = 0; id < 6; id++)
        {
            if (hHSLRGB[id] == (HWND)lParam)
                break;
        }

        if (id >= 6)
            return (FALSE);

        id += COLOR_HUE;

        if (id == COLOR_HUE)
            limit = RANGE - 1;

        else if (id < COLOR_RED)
            limit = RANGE;

        else
            limit = RGBMAX;

        nNewVal = nVal = GetDlgItemInt (hWnd, id, &bOK, FALSE);

        switch (LOWORD(wParam))
        {

        case SB_LINEUP:
            nDelta = 1;
            break;

        case SB_LINEDOWN:
            nDelta = -1;
            break;

        case SB_PAGEUP:
            nDelta = (id < COLOR_RED) ? (RANGE / 6) : (RGBMAX + 1) / 8;
            break;

        case SB_PAGEDOWN:
            nDelta = (id < COLOR_RED) ? -(RANGE / 6) : -(RGBMAX + 1) / 8;
            break;

        case SB_TOP:
            nDelta = RGBMAX;
            break;

        case SB_BOTTOM:
            nDelta = -RGBMAX;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            nVal = -1;
            nDelta = 0;
            break;

        case SB_ENDSCROLL:
            nDelta = 0;
            break;

        default:
            return (FALSE);
            break;
        }

        if (LOWORD(wParam) == SB_ENDSCROLL)
        {
            SendDlgItemMessage (hWnd, id, EM_SETSEL, 0, 32767);
            break;
        }

        if (nVal == -1)           /* Right Mouse Double Click?? */
            nVal = (limit + 1) / 2;
        else if (nVal + nDelta > limit)
        {
            nVal = (id != COLOR_HUE) ? limit : 0; /* Rollover for HUE */
            nDelta = 0;
        }
        else if (nVal + nDelta < 0)
        {
            nVal = (id == COLOR_HUE) ? limit : 0; /* Rollover for HUE */
            nDelta = 0;
        }

        if (nNewVal != nVal + nDelta)
        {
            SetDlgItemInt (hWnd, id, nVal + nDelta, FALSE);
            SendMessage (hWnd, WM_COMMAND, id, MAKELONG (0, EN_CHANGE));
        }

        SetFocus (GetDlgItem (hWnd, id));
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
            goto DoHelp;

        case IDOK:
        case IDCANCEL:
            if (bMouseCapture)
            {
                bMouseCapture = FALSE;
                ReleaseCapture ();
                ClipCursor ((LPRECT) NULL);
            }

            if (hRainbowBitmap)              // Condition shouldn't be necessary,
                DeleteObject (hRainbowBitmap); // shouldn't get here if !hRainbowBitmap

            EnableWindow (GetDlgItem (GetWindow (hWnd, GW_OWNER), COLOR_MIX), TRUE);

            DestroyWindow (hWnd);
            hRainbowDlg = 0;
            break;

        case COLOR_RESET:

            NearestSolid (hWnd);
            bUpdateExample = TRUE;
            break;

        case COLOR_ADD:

            fChanged |= 0x01;
            SendMessage (GetWindow (hWnd, GW_OWNER), WM_USER + 0x400, 0, rainbowRGB);
            break;

        case COLOR_RED:
        case COLOR_GREEN:
        case COLOR_BLUE:

            if (HIWORD(wParam) == EN_CHANGE)
            {
                RGBEditChange (hWnd, (DWORD) LOWORD(wParam));
                bUpdateExample = TRUE;
            }
            else if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                GetDlgItemInt (hWnd, LOWORD(wParam), &bOK, FALSE);

                if (!bOK)
                {
                    SetRGBEdit ((DWORD) LOWORD(wParam));
                }
            }

            break;

        case COLOR_HUE:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                bUpdateExample = TRUE;
                nVal = GetDlgItemInt (hWnd, COLOR_HUE, &bOK, FALSE);

                if (bOK)
                {
                    if (nVal >  RANGE - 1)
                    {
                        nVal = RANGE - 1;
                        SetDlgItemInt (hRainbowDlg, COLOR_HUE, nVal, FALSE);
                    }

                    if ((WORD) nVal != currentHue)
                    {
                        hDC = GetDC (hWnd);
                        EraseCrossHair (hDC);
                        currentHue = (WORD)nVal;
                        rainbowRGB = HLStoRGB ((WORD)nVal, currentLum, currentSat);
                        SetRGBEdit (0);
                        HLStoHLSPos (COLOR_HUE);
                        CrossHairPaint (hDC, nHuePos, nSatPos);
                        ReleaseDC (hWnd, hDC);
                        InvalidateRect (hWnd, (LPRECT) & rLumPaint, FALSE);
                        InvalidateRect (hRainbowDlg,
                            (LPRECT) & rColorSamples, FALSE);
                        UpdateWindow (hWnd);
                    }
                }
                else if (GetDlgItemText (hWnd, LOWORD(wParam), cEdit, CharSizeOf(cEdit)))
                {
                    SetHLSEdit (COLOR_HUE);
                    SendDlgItemMessage (hWnd, LOWORD(wParam), EM_SETSEL, 0, 32767);
                }
            }

#if DAVIDDOSIT
            else if (HIWORD(wParam) == EN_SETFOCUS)
            {
                SendDlgItemMessage (hWnd, LOWORD(wParam), EM_SETSEL, 0, 32767);
            }
            else if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                SendDlgItemMessage (hWnd, LOWORD(wParam), EM_SETSEL, 0, 0L);
            }
#endif
            break;

        case COLOR_SAT:

            if (HIWORD(wParam) == EN_CHANGE)
            {
                bUpdateExample = TRUE;
                nVal = GetDlgItemInt (hWnd, COLOR_SAT, &bOK, FALSE);

                if (bOK)
                {
                    if (nVal >  RANGE)
                    {
                        nVal = RANGE;
                        SetDlgItemInt (hRainbowDlg, COLOR_SAT, nVal, FALSE);
                    }

                    if ((WORD) nVal != currentSat)
                    {
                        hDC = GetDC (hWnd);
                        EraseCrossHair (hDC);
                        currentSat = (WORD)nVal;
                        rainbowRGB = HLStoRGB (currentHue, currentLum, (WORD)nVal);
                        SetRGBEdit (0);
                        HLStoHLSPos (COLOR_SAT);
                        CrossHairPaint (hDC, nHuePos, nSatPos);
                        ReleaseDC (hWnd, hDC);
                        InvalidateRect (hWnd, &rLumPaint, FALSE);
                        InvalidateRect (hRainbowDlg, &rColorSamples, FALSE);
                        UpdateWindow (hWnd);
                    }

                }
                else if (GetDlgItemText (hWnd, LOWORD(wParam), cEdit, CharSizeOf(cEdit)))
                {
                    SetHLSEdit (COLOR_SAT);
                    SendDlgItemMessage (hWnd, LOWORD(wParam), EM_SETSEL, 0, 32767);
                }
            }
#if DAVIDDOSIT
            else if (HIWORD(wParam) == EN_SETFOCUS)
            {
                SendDlgItemMessage (hWnd, LOWORD(wParam), EM_SETSEL, 0, 32767);
            }
            else if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                SendDlgItemMessage (hWnd, LOWORD(wParam), EM_SETSEL, 0, 0L);
            }
#endif
            break;

        case COLOR_LUM:

            if (HIWORD(wParam) == EN_CHANGE)
            {
                bUpdateExample = TRUE;
                nVal = GetDlgItemInt (hWnd, COLOR_LUM, &bOK, FALSE);

                if (bOK)
                {
                    if (nVal >  RANGE)
                    {
                        nVal = RANGE;
                        SetDlgItemInt (hRainbowDlg, COLOR_LUM, nVal, FALSE);
                    }

                    if ((WORD) nVal != currentLum)
                    {
                        hDC = GetDC (hWnd);
                        EraseLumArrow (hDC);
                        currentLum = nVal;
                        HLStoHLSPos (COLOR_LUM);
                        rainbowRGB = HLStoRGB (currentHue, (WORD)nVal, currentSat);
                        SetRGBEdit (0);
                        LumArrowPaint (hDC);
                        ReleaseDC (hWnd, hDC);
                        InvalidateRect (hRainbowDlg, &rColorSamples, FALSE);
                        UpdateWindow (hWnd);
                    }

                }
                else if (GetDlgItemText (hWnd, LOWORD(wParam), cEdit, CharSizeOf(cEdit)))
                {
                    SetHLSEdit (COLOR_LUM);
                    SendDlgItemMessage (hWnd, LOWORD(wParam), EM_SETSEL, 0, 32767);
                }
            }
#if DAVIDDOSIT
            else if (HIWORD(wParam) == EN_SETFOCUS)
            {
                SendDlgItemMessage (hWnd, LOWORD(wParam), EM_SETSEL, 0, 32767);
            }
            else if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                SendDlgItemMessage (hWnd, LOWORD(wParam), EM_SETSEL, 0, 0L);
            }
#endif
            break;
        }
        break;

    case WM_PAINT:

        BeginPaint (hWnd, &ps);
        nOldBkMode = SetBkMode (ps.hdc, TRANSPARENT);
        RainbowPaint (ps.hdc, &ps.rcPaint);
        SetBkMode (ps.hdc, nOldBkMode);
        EndPaint (hWnd, &ps);
        break;

    case WM_MOVE:

        SetupRainbowCapture (hWnd);
        return (FALSE);
        break;

    default:

        if (message == wHelpMessage)
        {
DoHelp:
            dwSave = dwContext;
            dwContext = IDH_DLG_COLORDEFINE;
            CPHelp (hWnd);
            dwContext = dwSave;
            return (TRUE);
        }
        else
            return (FALSE);
    }

    return TRUE;
}


void HLSPostoHLS (DWORD nHLSEdit)
{
    switch (nHLSEdit)
    {

    case COLOR_HUE:

        currentHue = (WORD) ((nHuePos - (DWORD)rRainbow.left) * (RANGE - 1) / (nHueWidth - 1));
        break;

    case COLOR_SAT:

        currentSat = (WORD) (RANGE - (nSatPos - (DWORD)rRainbow.top) * RANGE / (nSatHeight - 1));
        break;

    case COLOR_LUM:

        currentLum = (WORD) (RANGE - (nLumPos - rLumPaint.top) * RANGE / (nLumHeight - 1));
        break;

    default:

        currentHue = (WORD) ((nHuePos - (DWORD)rRainbow.left) * (RANGE - 1) / nHueWidth);
        currentSat = (WORD) (RANGE - (nSatPos - (DWORD)rRainbow.top) * RANGE / nSatHeight);
        currentLum = (WORD) (RANGE - (nLumPos - rLumPaint.top) * RANGE / nLumHeight);
        break;
    }

    return;
}


void HLStoHLSPos (DWORD nHLSEdit)
{
    switch (nHLSEdit)
    {

    case COLOR_HUE:

        nHuePos = (WORD) ((DWORD)rRainbow.left + currentHue * nHueWidth / (RANGE - 1));
        break;

    case COLOR_SAT:

        nSatPos = (WORD) ((DWORD)rRainbow.top + (RANGE - currentSat) * (nSatHeight - 1) / RANGE);
        break;

    case COLOR_LUM:

        nLumPos = (WORD) (rLumPaint.top + (RANGE - currentLum) * (nLumHeight - 1) / RANGE);
        break;

    default:

        nHuePos = (WORD) ((DWORD)rRainbow.left + currentHue * nHueWidth / (RANGE - 1));
        nSatPos = (WORD) ((DWORD)rRainbow.top + (RANGE - currentSat) * (nSatHeight - 1) / RANGE);
        nLumPos = (WORD) (rLumPaint.top + (RANGE - currentLum) * (nLumHeight - 1) / RANGE);
        break;
    }

    return;
}


void SetHLSEdit (DWORD nHLSEdit)
{
    switch (nHLSEdit)
    {

    case COLOR_HUE:

        SetDlgItemInt (hRainbowDlg, COLOR_HUE, currentHue, FALSE);
        break;

    case COLOR_SAT:

        SetDlgItemInt (hRainbowDlg, COLOR_SAT, currentSat, FALSE);
        break;

    case COLOR_LUM:

        SetDlgItemInt (hRainbowDlg, COLOR_LUM, currentLum, FALSE);
        break;

    default:

        SetDlgItemInt (hRainbowDlg, COLOR_HUE, currentHue, FALSE);
        SetDlgItemInt (hRainbowDlg, COLOR_SAT, currentSat, FALSE);
        SetDlgItemInt (hRainbowDlg, COLOR_LUM, currentLum, FALSE);
        break;
    }

    return;
}


void SetRGBEdit (DWORD nRGBEdit)
{
    switch (nRGBEdit)
    {

    case COLOR_RED:

        SetDlgItemInt (hRainbowDlg, COLOR_RED, GetRValue (rainbowRGB), FALSE);
        break;

    case COLOR_GREEN:

        SetDlgItemInt (hRainbowDlg, COLOR_GREEN, GetGValue (rainbowRGB), FALSE);
        break;

    case COLOR_BLUE:

        SetDlgItemInt (hRainbowDlg, COLOR_BLUE, GetBValue (rainbowRGB), FALSE);
        break;

    default:

        SetDlgItemInt (hRainbowDlg, COLOR_RED, GetRValue (rainbowRGB), FALSE);
        SetDlgItemInt (hRainbowDlg, COLOR_GREEN, GetGValue (rainbowRGB), FALSE);
        SetDlgItemInt (hRainbowDlg, COLOR_BLUE, GetBValue (rainbowRGB), FALSE);
        break;
    }

    return;
}


BOOL InitRainbow (HWND hWnd)
{
    HDC hDC;
    WORD Sat, Hue;
    HBITMAP hOldBitmap;
    RECT Rect;
    HBRUSH hbrSwipe;

    // Make certain arrows are symmetric

    OddArrowWindow (hHSLRGB[0] = GetDlgItem (hWnd, HUESCROLL));
    OddArrowWindow (hHSLRGB[1] = GetDlgItem (hWnd, SATSCROLL));
    OddArrowWindow (hHSLRGB[2] = GetDlgItem (hWnd, LUMSCROLL));
    OddArrowWindow (hHSLRGB[3] = GetDlgItem (hWnd, REDSCROLL));
    OddArrowWindow (hHSLRGB[4] = GetDlgItem (hWnd, GREENSCROLL));
    OddArrowWindow (hHSLRGB[5] = GetDlgItem (hWnd, BLUESCROLL));

    RGBtoHLS (rainbowRGB = currentRGB);

    hRainbowDlg = hWnd;

    SetupRainbowCapture (hWnd);

    nHueWidth = (WORD)(rRainbow.right - rRainbow.left);
    nSatHeight = (WORD)(rRainbow.bottom - rRainbow.top);

    currentHue = H;
    currentSat = S;
    currentLum = L;

    HLStoHLSPos (0);
    SetRGBEdit (0);
    SetHLSEdit (0);

    hDC = GetDC (hWnd);
    hRainbowBitmap = CreateCompatibleBitmap (hDC, nHueWidth, nSatHeight);
    if (!hRainbowBitmap)
        return (FALSE);
    hOldBitmap = SelectObject (hDCBits, hRainbowBitmap);

/*
   NOTE: The final pass through this loop paints past the end of the
   selected bitmap.  Windows is a good product, and doesn't let such
   foolishness happen.  The reason it's allowed in this code is that
   there's no reason to have such checking in several places.
*/

    Rect.bottom = 0;
    for (Sat = RANGE; Sat > 0; Sat -= SATINC)
    {
        Rect.top = Rect.bottom;
        Rect.bottom = (nSatHeight * RANGE - (Sat - SATINC) * nSatHeight) / RANGE;
        Rect.right = 0;

        for (Hue = 0; Hue < (RANGE - 1); Hue += HUEINC)
        {
            Rect.left = Rect.right;
            Rect.right = ((Hue + HUEINC) * nHueWidth) / RANGE;
            if (hbrSwipe = CreateSolidBrush (HLStoRGB (Hue, RANGE / 2, Sat)))
            {
                FillRect (hDCBits, &Rect, hbrSwipe);
                DeleteObject (hbrSwipe);
            }
        }
    }

    SelectObject (hDCBits, hOldBitmap);
    ReleaseDC (hWnd, hDC);

    UpdateWindow (hWnd);

    return (TRUE);
}


void PaintRainbow (HDC  hDC, LPRECT lpRect)
{
    HBITMAP hOldBitmap;

    if (!hRainbowBitmap)
        return;

    hOldBitmap = SelectObject (hDCBits, hRainbowBitmap);
    BitBlt (hDC, lpRect->left, lpRect->top, lpRect->right - lpRect->left,
        lpRect->bottom - lpRect->top, hDCBits,
        lpRect->left - rRainbow.left, lpRect->top - rRainbow.top,
        SRCCOPY);

    SelectObject (hDCBits, hOldBitmap);
    CrossHairPaint (hDC, nHuePos, nSatPos);
    UpdateWindow (hRainbowDlg);

    return;
}


void RainbowPaint (HDC hDC, LPRECT lpPaintRect)
{
    WORD Lum;
    RECT Rect;
    HBRUSH hbrSwipe;

    // Paint the Current Color Sample

    if (IntersectRect (&Rect, lpPaintRect, &rCurrentColor))
    {
        if (hbrSwipe = CreateSolidBrush (rainbowRGB))
        {
            FillRect (hDC, (LPRECT) & Rect, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
    }

    // Paint the Nearest Pure Color Sample

    if (IntersectRect (&Rect, lpPaintRect, &rNearestPure))
    {
        if (hbrSwipe = CreateSolidBrush (GetNearestColor (hDC, rainbowRGB)))
        {
            FillRect (hDC, &Rect, hbrSwipe);
            DeleteObject (hbrSwipe);
        }
    }

    // Paint the Luminosity Range

    if (IntersectRect (&Rect, lpPaintRect, &rLumPaint))
    {
        Rect.left = rLumPaint.left;
        Rect.right = rLumPaint.right;
        Rect.top = rLumPaint.bottom - LUMINC / 2;
        Rect.bottom = rLumPaint.bottom;
        if (hbrSwipe = CreateSolidBrush ( HLStoRGB(currentHue, 0, currentSat)))
        {
            FillRect (hDC, &Rect, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        for (Lum = LUMINC; Lum < RANGE; Lum += LUMINC)
        {
            Rect.bottom = Rect.top;
            Rect.top = (short) (((rLumPaint.bottom + LUMINC / 2) * (DWORD)RANGE
                -(Lum + LUMINC) * nLumHeight) / RANGE);
            if (hbrSwipe = CreateSolidBrush (HLStoRGB(currentHue, Lum, currentSat)))
            {
                FillRect (hDC, &Rect, hbrSwipe);
                DeleteObject (hbrSwipe);
            }
        }

        Rect.bottom = Rect.top;
        Rect.top = rLumPaint.top;
        if (hbrSwipe = CreateSolidBrush (HLStoRGB(currentHue, RANGE, currentSat)))
        {
            FillRect (hDC, &Rect, hbrSwipe);
            DeleteObject (hbrSwipe);
        }

        // Paint the bounding rectangle only when it might be necessary

        if (!EqualRect (lpPaintRect,  &rLumPaint))
        {
            hbrSwipe = SelectObject (hDC, GetStockObject (NULL_BRUSH));
            Rectangle (hDC, rLumPaint.left - 1, rLumPaint.top - 1,
                rLumPaint.right + 1, rLumPaint.bottom + 1);
            SelectObject (hDC, hbrSwipe);
        }
    }

    // Paint the Luminosity Arrow

    if (IntersectRect (&Rect, lpPaintRect, &rLumScroll))
    {
        LumArrowPaint (hDC);
    }

    if (IntersectRect (&Rect, lpPaintRect, &rRainbow))
    {
        PaintRainbow (hDC, (LPRECT) & Rect);
    }

    return;
}


/* Color conversion routines --

   RGBtoHLS () takes a DWORD RGB value, translates it to HLS, and stores the
   results in the global vars H, L, and S.  HLStoRGB takes the current values
   of H, L, and S and returns the equivalent value in an RGB DWORD.  The vars
   H, L and S are written to only by 1) RGBtoHLS (initialization) or 2) the
   scrollbar handlers.

   A point of reference for the algorithms is Foley and Van Dam, pp. 618-19.
   Their algorithm is in floating point.  CHART implements a less general
   (hardwired ranges) integral algorithm.

*/

/* There are potential roundoff errors lurking throughout here.
   (0.5 + x/y) without floating point,
      (x/y) phrased ((x + (y/2))/y)
   yields very small roundoff error.
   This makes many of the following divisions look funny.
*/

/* H,L, and S vary over 0-HLSMAX */
/* R,G, and B vary over 0-RGBMAX */
/* HLSMAX BEST IF DIVISIBLE BY 6 */
/* RGBMAX, HLSMAX must each fit in a byte. */

#define UNDEFINED (HLSMAX*2/3)/* Hue is undefined if Saturation is 0 (grey-scale) */
/* This value determines where the Hue scrollbar is */
/* initially set for achromatic colors */

void RGBtoHLS (DWORD lRGBColor)
{
    short    R, G, B;                  // input RGB values */

    WORD cMax, cMin;               // max and min RGB values */

    WORD cSum, cDif;
    short    Rdelta, Gdelta, Bdelta;  // intermediate value: % of spread from max

    // get R, G, and B out of DWORD

    R = GetRValue (lRGBColor);
    G = GetGValue (lRGBColor);
    B = GetBValue (lRGBColor);

    // calculate lightness

    cMax = max ( max (R, G), B);
    cMin = min ( min (R, G), B);
    cSum = cMax + cMin;

    L = (WORD)(((cSum * (DWORD)HLSMAX) + RGBMAX ) / (2 * RGBMAX));

    cDif = cMax - cMin;
    if (!cDif)
    {                  // r=g=b --> achromatic case

        S = 0;                     // saturation

#ifdef SETHUEUNDEFINED
        H = UNDEFINED;             // hue
#endif
    }
    else
    {                      // chromatic case
        // saturation

        if (L <= (HLSMAX / 2))
            S = (WORD) (((cDif * (DWORD) HLSMAX) + (cSum / 2) ) / cSum);
        else
            S = (WORD) ((DWORD) ((cDif * (DWORD) HLSMAX) + (DWORD)((2 * RGBMAX - cSum) / 2) )
                 / (2 * RGBMAX - cSum));
        // hue

        Rdelta = (short) (( ((cMax - R) * (DWORD)(HLSMAX / 6)) + (cDif / 2) ) / cDif);
        Gdelta = (short) (( ((cMax - G) * (DWORD)(HLSMAX / 6)) + (cDif / 2) ) / cDif);
        Bdelta = (short) (( ((cMax - B) * (DWORD)(HLSMAX / 6)) + (cDif / 2) ) / cDif);

        if ((WORD) R == cMax)

            H = Bdelta - Gdelta;

        else if ((WORD) G == cMax)

            H = (HLSMAX / 3) + Rdelta - Bdelta;

        else /* B == cMax */

            H = ((2 * HLSMAX) / 3) + Gdelta - Rdelta;

        if (H < 0)
            H += HLSMAX;
        if (H > HLSMAX)
            H -= HLSMAX;
    }
}


WORD HueToRGB (WORD n1, WORD n2, WORD hue)
{
    // range check: note values passed add/subtract thirds of range
    // The following is redundant for WORD (unsigned int)

#if 0
    if (hue < 0)
        hue += HLSMAX;
#endif

    if (hue > HLSMAX)
        hue -= HLSMAX;

    // return r,g, or b value from this tridrant

    if (hue < (HLSMAX / 6))
        return ( n1 + (((n2 - n1) * hue + (HLSMAX / 12)) / (HLSMAX / 6)) );

    if (hue < (HLSMAX / 2))
        return ( n2 );

    if (hue < ((HLSMAX * 2) / 3))
        return ( n1 + (((n2 - n1) * (((HLSMAX * 2) / 3) - hue) + (HLSMAX / 12)) / (HLSMAX / 6)) );
    else
        return ( n1 );
}


DWORD HLStoRGB (WORD hue, WORD lum, WORD sat)
{
    WORD R, G, B;                   // RGB component values

    WORD  Magic1, Magic2;          // calculated magic numbers (really!)

    if (sat == 0)
    {               // achromatic case

        R = G = B = (lum * RGBMAX) / HLSMAX;

#ifdef SETHUEUNDEFINED
        if (hue != UNDEFINED)
        {    // ERROR

        }
#endif

    }
    else
    {                      // chromatic case
        // set up magic numbers
        if (lum <= (HLSMAX / 2))
            Magic2 = (WORD)((lum * ((DWORD)HLSMAX + sat) + (HLSMAX / 2)) / HLSMAX);
        else
            Magic2 = lum + sat - (WORD)(((lum * sat) + (DWORD)(HLSMAX / 2)) / HLSMAX);

        Magic1 = 2 * lum - Magic2;

        // get RGB, change units from HLSMAX to RGBMAX

        R = (WORD)((HueToRGB (Magic1, Magic2, (WORD) (hue + (HLSMAX / 3))) * (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX;
        G = (WORD)((HueToRGB (Magic1, Magic2, hue) * (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX;
        B = (WORD)((HueToRGB (Magic1, Magic2, (WORD) (hue - (HLSMAX / 3))) * (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX;
    }

    return (RGB (R, G, B));
}


//  RGBEditChange
// Checks the edit box for a valid entry and updates the Hue, Sat, and Lum
// edit controls if appropriate.  Also updates Lum picture and current
// color sample.

// Input:
//     nDlgID   Dialog ID of Red, Green or Blue edit control.


BOOL RGBEditChange (HWND  hWnd, DWORD nDlgID)
{
    BOOL bOK;               /* Check that value in edit control is unsigned int */
    BYTE * currentValue;     /* Pointer to byte in RGB to change (or reset) */
    int    nVal;
    TCHAR    cEdit[3];

    currentValue = (BYTE * ) & rainbowRGB;
    switch (nDlgID)
    {

    case COLOR_RED:

        break;

    case COLOR_GREEN:

        currentValue++;
        break;

    case COLOR_BLUE:

        currentValue += 2;
        break;

    default:
        return (INVALID_ID);

    }

    nVal = GetDlgItemInt (hRainbowDlg, nDlgID, (BOOL FAR * ) & bOK, FALSE);

    if (bOK)
    {
        if (nVal >  RGBMAX)
        {
            nVal = RGBMAX;
            SetDlgItemInt (hRainbowDlg, nDlgID, nVal, FALSE);
        }

        if (nVal != (short) *currentValue)
        {
            *currentValue = LOBYTE (nVal);
            ChangeColorSettings (hWnd, rainbowRGB);
            SetHLSEdit (nDlgID);
        }
    }
    else if (GetDlgItemText (hRainbowDlg, nDlgID, cEdit, CharSizeOf(cEdit)))
    {
        SetRGBEdit (nDlgID);
        SendDlgItemMessage (hRainbowDlg, nDlgID, EM_SETSEL, 0, 32767);
    }

    return (bOK ? VALID_ENTRY : INVALID_ENTRY);
}


