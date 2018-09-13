/*  LOOKDLG.C
**
**  Copyright (C) Microsoft, 1993, All Rights Reserved.
**
**
**  History:
**
*/
#include <windows.h>
#include "desk.h"
#include "deskid.h"
#include "look.h"
#include <commdlg.h>

#define NUM_COLORSMAX    64
#define NUM_COLORSPERROW 4
#define NUM_COLORSPERCOL (pmd->iNumColors / NUM_COLORSPERROW)

typedef struct {
    LPCOLORPICK_INFO lpcpi;
    int dxColor;
    int dyColor;
    int iCurColor;
    int iNumColors;
    BOOL capturing;
    BOOL justdropped;
    COLORREF Colors[NUM_COLORSMAX];
} MYDATA, * PMYDATA, FAR * LPMYDATA;

BOOL g_bCursorHidden;

BOOL CALLBACK  ColorPickDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam);

BOOL NEAR PASCAL UseColorPicker( LPCOLORPICK_INFO lpcpi )
{
    CHOOSECOLOR cc;
    // BUGBUG
    extern COLORREF g_CustomColors[16];

    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = lpcpi->hwndParent; // NOT lpcpi->hwndOwner
    cc.hInstance = NULL;
    cc.rgbResult = lpcpi->rgb;
    cc.lpCustColors = g_CustomColors;
    cc.Flags = CC_RGBINIT | lpcpi->flags;
    cc.lCustData = 0L;
    cc.lpfnHook = NULL;
    cc.lpTemplateName = NULL;

    if (ChooseColor(&cc))
    {
        lpcpi->rgb = cc.rgbResult;
        return TRUE;
    }

    return FALSE;
}

void NEAR PASCAL DrawColorSquare(HDC hdc, int iColor, PMYDATA pmd)
{
    RECT rc;
    COLORREF rgb;
    HPALETTE hpalOld = NULL;
    HBRUSH hbr;

    // custom color
    if (iColor == pmd->iNumColors)
    {
        rc.left = 0;
        rc.top = 0;
        rgb = pmd->lpcpi->rgb;
    }
    else
    {
        rc.left = (iColor % NUM_COLORSPERROW) * pmd->dxColor;
        rc.top = (iColor / NUM_COLORSPERROW) * pmd->dyColor;
        rgb = pmd->Colors[iColor];
    }
    rc.right = rc.left + pmd->dxColor;
    rc.bottom = rc.top + pmd->dyColor;

    // focused one
    if (iColor == pmd->iCurColor)
    {
        PatBlt(hdc, rc.left, rc.top, pmd->dxColor, 3, BLACKNESS);
        PatBlt(hdc, rc.left, rc.bottom - 3, pmd->dxColor, 3, BLACKNESS);
        PatBlt(hdc, rc.left, rc.top + 3, 3, pmd->dyColor - 6, BLACKNESS);
        PatBlt(hdc, rc.right - 3, rc.top + 3, 3, pmd->dyColor - 6, BLACKNESS);
        InflateRect(&rc, -1, -1);
        FrameRect(hdc, &rc, GetStockObject(WHITE_BRUSH));
        InflateRect(&rc, -2, -2);
    }
    else
    {
        // clean up possible focus thing from above
        FrameRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));

        InflateRect(&rc, -cxBorder, -cyBorder);
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    }

    if ((pmd->lpcpi->flags & CC_SOLIDCOLOR) && !(rgb & 0xFF000000))
        rgb = GetNearestColor(hdc, rgb);

    hbr = CreateSolidBrush(rgb);
    if (pmd->lpcpi->hpal)
    {
        hpalOld = SelectPalette(hdc, pmd->lpcpi->hpal, FALSE);
        RealizePalette(hdc);
    }
    hbr = SelectObject(hdc, hbr);
    PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
    hbr = SelectObject(hdc, hbr);

    if (hpalOld)
    {
        hpalOld = SelectPalette(hdc, hpalOld, TRUE);
        RealizePalette(hdc);
    }

    DeleteObject(hbr);
}

/*
** set the focus to the given color.
**
** in the process, also take the focus off of the old focus color.
*/
void NEAR PASCAL FocusColor(HWND hDlg, int iNewColor, PMYDATA pmd)
{
    int i;
    HDC hdc = NULL;
    HWND hwnd;

    if (iNewColor == pmd->iCurColor)
        return;

    i = pmd->iCurColor;
    pmd->iCurColor = iNewColor;

    // unfocus the old one
    if( i >= 0 )
    {
        if (i == pmd->iNumColors)
            hwnd = GetDlgItem(hDlg, IDC_COLORCUST);
        else
            hwnd = GetDlgItem(hDlg, IDC_16COLORS);
        hdc = GetDC(hwnd);
        DrawColorSquare(hdc, i, pmd);
        ReleaseDC(hwnd, hdc);
    }

    // focus the new one
    if( iNewColor >= 0 )
    {
        if (iNewColor == pmd->iNumColors)
            hwnd = GetDlgItem(hDlg, IDC_COLORCUST);
        else
            hwnd = GetDlgItem(hDlg, IDC_16COLORS);
        hdc = GetDC(hwnd);
        DrawColorSquare(hdc, iNewColor, pmd);
        ReleaseDC(hwnd, hdc);
    }
}

void NEAR PASCAL Color_TrackMouse(HWND hDlg, POINT pt, PMYDATA pmd)
{
    HWND hwndKid;
    int id;

    hwndKid = ChildWindowFromPoint(hDlg, pt);
    if (hwndKid == NULL || hwndKid == hDlg)
        return;

    id = GetWindowLong(hwndKid, GWL_ID);
    switch (id)
    {
        case IDC_16COLORS:
            MapWindowPoints(hDlg, GetDlgItem(hDlg, IDC_16COLORS), &pt, 1);
            pt.x /= pmd->dxColor;
            pt.y /= pmd->dyColor;
            FocusColor(hDlg, pt.x + (pt.y * NUM_COLORSPERROW), pmd);
            break;

        case IDC_COLORCUST:
            if (IsWindowVisible(hwndKid))
                FocusColor(hDlg, pmd->iNumColors, pmd);
            break;

        case IDC_COLOROTHER:
            FocusColor(hDlg, -1, pmd);
            break;
    }
}

void NEAR PASCAL Color_DrawItem(HWND hDlg, LPDRAWITEMSTRUCT lpdis, PMYDATA pmd)
{
    int i;

    if (lpdis->CtlID == IDC_COLORCUST)
    {
        DrawColorSquare(lpdis->hDC, pmd->iNumColors, pmd);
    }
    else
    {
        for (i = 0; i < pmd->iNumColors; i++)
            DrawColorSquare(lpdis->hDC, i, pmd);
    }
}

/*
** init the mini-color-picker
**
** the dialog is pretending to be a menu, so figure out where to pop
** it up so that it is visible all around.
**
** also because this dialog is pretty darn concerned with its look,
** hand-align the components in pixel units.  THIS IS GROSS!
*/
void NEAR PASCAL Color_InitDialog(HWND hDlg, PMYDATA pmd)
{
    RECT rcOwner;
    RECT rc, rc2;
    int dx, dy;
    int dxScreen, dyScreen;
    int x, y;
    int i;
    HWND hwndColors, hwnd;
#ifdef DBCS
    HWND hwndEtch, hwndCust;
    int  width, widthCust, widthEtch;
#endif
    int cyEdge = GetSystemMetrics(SM_CYEDGE);
    HPALETTE hpal = pmd->lpcpi->hpal;

    if (hpal == NULL)
        hpal = GetStockObject(DEFAULT_PALETTE);

    pmd->iNumColors = 0;
    GetObject(hpal, sizeof(int), &pmd->iNumColors);

    if (pmd->iNumColors > NUM_COLORSMAX)
        pmd->iNumColors = NUM_COLORSMAX;

    GetPaletteEntries(hpal,0, pmd->iNumColors, (LPPALETTEENTRY)pmd->Colors);

    // size the 16 colors to be square
    hwndColors = GetDlgItem(hDlg, IDC_16COLORS);
    GetClientRect(hwndColors, &rc);
#ifdef DBCS
    // To make localization easy..
    //
    hwndEtch=GetDlgItem(hDlg, IDC_COLORETCH);
    GetClientRect(hwndEtch, &rc2);
    widthEtch = rc2.right-rc2.left;

    hwndCust=GetDlgItem(hDlg, IDC_COLORCUST);
    GetClientRect(hwndCust, &rc2);
    widthCust = rc2.right-rc2.left;

    hwnd = GetDlgItem(hDlg, IDC_COLOROTHER);
    GetWindowRect(hwnd, &rc2); // we must initialize rc2 with this control.

    // Take possible biggest width to calculate sels
    //
    width = (widthEtch > widthCust+(rc2.right-rc2.left)) ? widthEtch : widthCust+(rc2.right-rc2.left);
    width = (width > rc.right-rc.left) ? width: rc.right-rc.left;

    pmd->dxColor = pmd->dyColor
    = ((rc.bottom - rc.top) / NUM_COLORSPERCOL > width / NUM_COLORSPERROW )
      ?  (rc.bottom - rc.top) / NUM_COLORSPERCOL : width / NUM_COLORSPERROW;

    // Make sure custum color can fit
    //
    if (pmd->dxColor*(NUM_COLORSPERROW-1) < rc2.right-rc2.left )
        pmd->dxColor = pmd->dyColor = (rc2.right-rc2.left)/(NUM_COLORSPERROW-1);
#else
    pmd->dxColor = pmd->dyColor = (rc.bottom - rc.top) / NUM_COLORSPERCOL;
#endif
    // make each color square's width the same as the height
    SetWindowPos(hwndColors, NULL, 0, 0, pmd->dxColor * NUM_COLORSPERROW,
                pmd->dyColor * NUM_COLORSPERCOL, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);
    rc.right = rc.left + pmd->dxColor * NUM_COLORSPERROW;
    rc.bottom = rc.top + pmd->dyColor * NUM_COLORSPERCOL;

    MapWindowPoints(hwndColors, hDlg, (LPPOINT)(LPRECT)&rc, 2);

    // move/size the etch to the right place
    // (compensate for the colors being "inset" by one)
#ifdef DBCS
    MoveWindow(hwndEtch, rc.left + 1, rc.bottom + cyEdge,
                                rc.right - rc.left - 2, cyEdge, FALSE);
#else
    MoveWindow(GetDlgItem(hDlg, IDC_COLORETCH), rc.left + 1, rc.bottom + cyEdge,
                                rc.right - rc.left - 2, cyEdge, FALSE);
#endif

    y = rc.bottom + 3 * cyEdge;

    // size the custom color to the same square and right-align
#ifdef DBCS
    MoveWindow(hwndCust, rc.right - pmd->dxColor, y,
                                pmd->dxColor, pmd->dyColor, FALSE);
#else
    MoveWindow(GetDlgItem(hDlg, IDC_COLORCUST), rc.right - pmd->dxColor, y,
                                pmd->dxColor, pmd->dyColor, FALSE);
#endif

    // do same for button
#ifndef DBCS
    hwnd = GetDlgItem(hDlg, IDC_COLOROTHER);
    GetWindowRect(hwnd, &rc2);
#endif
    MapWindowPoints(NULL, hDlg, (LPPOINT)(LPRECT)&rc2, 2);
    // BUGBUG assume custom color and 'other' button are same size
    MoveWindow(hwnd, rc2.left, y, rc2.right - rc2.left, pmd->dyColor, FALSE);

    // now figure out the size for the dialog itself
    rc.left = rc.top = 0;
    rc.right = rc.left + pmd->dxColor * NUM_COLORSPERROW;
    // (compensate for the colors being "inset" by one)
    rc.bottom = y + pmd->dyColor + 1;

    AdjustWindowRect(&rc, GetWindowLong(hDlg, GWL_STYLE), FALSE);
    dx = rc.right - rc.left;
    dy = rc.bottom - rc.top;

    GetWindowRect(pmd->lpcpi->hwndOwner, &rcOwner);

    dxScreen = GetSystemMetrics(SM_CXSCREEN);
    dyScreen = GetSystemMetrics(SM_CYSCREEN);

    if (rcOwner.bottom + dy < dyScreen)
    {
        y = rcOwner.bottom;
        if (rcOwner.left + dx < dxScreen)
            x = rcOwner.left;
        else
            x = dxScreen - dx - 1;
    }
    else
    {
        y = rcOwner.top - dy;
        if (rcOwner.left + dx < dxScreen)
            x = rcOwner.left;
        else
            x = dxScreen - dx - 1;
    }
    MoveWindow(hDlg, x, y, dx, dy, FALSE);

    for (i = 0; i < pmd->iNumColors; i++)
    {
        pmd->Colors[i] &= 0x00FFFFFF;
        pmd->Colors[i] |= 0x02000000;
    }

    for (i = 0; i < pmd->iNumColors; i++)
    {
        if ((pmd->Colors[i] & 0x00FFFFFF) == (pmd->lpcpi->rgb & 0x00FFFFFF))
        {
            ShowWindow(GetDlgItem(hDlg, IDC_COLORCUST), SW_HIDE);
            break;
        }
    }
    // current is either one of 16 or the custom color (== pmd->iNumColors
    pmd->iCurColor = i;
}

BOOL CALLBACK  ColorPickDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    PMYDATA pmd = (PMYDATA)GetWindowLong(hDlg, DWL_USER);
    HWND hwndKid;
    int wRet;
    int id;
    POINT pt;
    BOOL fEnd = FALSE;

    switch(message)
    {
        case WM_INITDIALOG:
            pmd = (PMYDATA)LocalAlloc(LPTR, sizeof(MYDATA));
            SetWindowLong(hDlg, DWL_USER, (LONG)pmd);
            pmd->lpcpi = (LPCOLORPICK_INFO)lParam;
            pmd->capturing = FALSE;
            pmd->justdropped = TRUE;

            Color_InitDialog(hDlg, pmd);
            SetFocus(GetDlgItem(hDlg, IDC_16COLORS));

            // post self a message to setcapture after painting
            PostMessage(hDlg, WM_APP+1, 0, 0L);
            return FALSE;

        case WM_APP+1:
            if (g_bCursorHidden)
            {
                ShowCursor(TRUE);
                g_bCursorHidden = FALSE;
            }
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            pmd->capturing = TRUE;
            SetCapture(hDlg);
            pmd->capturing = FALSE;
            break;

        case WM_DESTROY:
            LocalFree((HLOCAL)pmd);
            break;

        case WM_CAPTURECHANGED:
            if( pmd->capturing )
                return TRUE;   // ignore if we're doing this on purpose

            // if this wasn't a button in the dialog, dismiss ourselves
            if( !pmd->justdropped || (HWND)lParam == NULL || GetParent((HWND)lParam) != hDlg)
            {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;

        case WM_MOUSEMOVE:
            LPARAM2POINT(lParam, &pt );

            Color_TrackMouse(hDlg, pt, pmd);
            break;

        // if button up is on the parent, leave picker up and untrammeled.
        // otherwise, we must have "menu-tracked" to get here, so select.
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            LPARAM2POINT(lParam, &pt);
            MapWindowPoints(hDlg, pmd->lpcpi->hwndOwner, &pt, 1);
            if (ChildWindowFromPoint(pmd->lpcpi->hwndOwner, pt))
                return 0;
            pmd->capturing = TRUE;
            pmd->justdropped = FALSE;  // user could not be dragging from owner
            ReleaseCapture();
            pmd->capturing = FALSE;
            fEnd = TRUE;
        // || fall    ||
        // || through ||
        // \/         \/
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            LPARAM2POINT(lParam, &pt);
            hwndKid = ChildWindowFromPoint(hDlg, pt);
            // assume it's a dismissal if we're going to close...
            wRet = IDCANCEL;

            // if not on parent, dismiss picker
            if (hwndKid != NULL && hwndKid != hDlg)
            {
                id = GetWindowLong(hwndKid, GWL_ID);
                switch (id)
                {
                    case IDC_16COLORS:
                        // make sure that iCurColor is valid
                        Color_TrackMouse(hDlg, pt, pmd);
                        pmd->lpcpi->rgb = pmd->Colors[pmd->iCurColor] & 0x00FFFFFF;

                        //BOGUS
                        //if (pmd->iCurColor >= 16)
                        //    pmd->lpcpi->rgb |= 0x02000000;

                        wRet = IDOK;
                        break;

                    case IDC_COLOROTHER:
                        FocusColor(hDlg, -1, pmd);
                        wRet = id;   // this will fall thru to use the picker
                        fEnd = TRUE; // we have capture, the button won't click
                        break;

                    default:
                        // if this is a down, we will track until the up
                        // if this is an up, we will close with no change
                        break;
                }
            }

            if( fEnd )
            {
                EndDialog(hDlg, wRet);
                return TRUE;
            }

            // make sure we have the capture again since we didn't close
            pmd->capturing = TRUE;
            SetCapture(hDlg);
            pmd->capturing = FALSE;
            break;

        case WM_DRAWITEM:
            Color_DrawItem(hDlg, (LPDRAWITEMSTRUCT)lParam, pmd);
            break;

        case WM_COMMAND:
            // all commands close the dialog
            // note IDC_COLOROTHER will fall through to the caller...
            // cannot pass ok with no color selected
            if( LOWORD(wParam) == IDOK && pmd->iCurColor < 0 )
                LOWORD(wParam) = IDCANCEL;

            EndDialog( hDlg, LOWORD(wParam) );
            break;
    }
    return FALSE;
}

BOOL WINAPI ChooseColorMini(LPCOLORPICK_INFO lpcpi)
{
    int iAnswer;

    ShowCursor(FALSE);
    g_bCursorHidden = TRUE;

    iAnswer = DialogBoxParam(hInstance, MAKEINTRESOURCE(DLG_COLORPICK),
                        lpcpi->hwndOwner, ColorPickDlgProc, (LPARAM)lpcpi);

    if (g_bCursorHidden)
    {
        ShowCursor(TRUE);
        g_bCursorHidden = FALSE;
    }

    switch( iAnswer )
    {
        case IDC_COLOROTHER:  // the user picked the "Other..." button
            return UseColorPicker( lpcpi );

        case IDOK:            // the user picked a color in our little window
            return TRUE;

        default:
            break;
    }

    return FALSE;
}
