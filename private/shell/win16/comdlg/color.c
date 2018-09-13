/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    color.c

Abstract:

    This module implements the Win32 color dialog.

Revision History:

--*/



//
//  Include Files.
//

#include <windows.h>
#include <port1632.h>

#include "privcomd.h"
#include "color.h"




//
//  Global Variables.
//

DWORD rgbBoxColorDefault[COLORBOXES] =
{
 0x8080FF, 0x80FFFF, 0x80FF80, 0x80FF00, 0xFFFF80, 0xFF8000, 0xC080FF, 0xFF80FF,
 0x0000FF, 0x00FFFF, 0x00FF80, 0x40FF00, 0xFFFF00, 0xC08000, 0xC08080, 0xFF00FF,
 0x404080, 0x4080FF, 0x00FF00, 0x808000, 0x804000, 0xFF8080, 0x400080, 0x8000FF,
 0x000080, 0x0080FF, 0x008000, 0x408000, 0xFF0000, 0xA00000, 0x800080, 0xFF0080,
 0x000040, 0x004080, 0x004000, 0x404000, 0x800000, 0x400000, 0x400040, 0x800040,
 0x000000, 0x008080, 0x408080, 0x808080, 0x808040, 0xC0C0C0, 0x400040, 0xFFFFFF,
 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF
};

RECT rColorBox[COLORBOXES];

UINT msgCOLOROKA;
UINT msgSETRGBA;

UINT msgCOLOROKW;
UINT msgSETRGBW;

LPCCHOOKPROC glpfnColorHook = 0;





#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ChooseColorA
//
//  ANSI entry point for ChooseColor when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI ChooseColorA(
    LPCHOOSECOLORA pCCA)
{
    LPCHOOSECOLORW pCCW;
    BOOL bRet;
    DWORD cbLen;
    COLORINFO CI;

    ZeroMemory(&CI, sizeof(COLORINFO));

    if (!pCCA)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pCCA->lStructSize != sizeof(CHOOSECOLORA))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if (!(pCCW = (LPCHOOSECOLORW)LocalAlloc(LPTR, sizeof(CHOOSECOLORW))))
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    //
    //  Init simple invariants.
    //
    pCCW->lStructSize = sizeof(CHOOSECOLORW);
    pCCW->hwndOwner = pCCA->hwndOwner;
    pCCW->hInstance = pCCA->hInstance;
    pCCW->lpfnHook = pCCA->lpfnHook;

    //
    //  TemplateName array invariant.
    //
    if (pCCA->Flags & CC_ENABLETEMPLATE)
    {
        if (HIWORD(pCCA->lpTemplateName))
        {
            cbLen = lstrlenA(pCCA->lpTemplateName) + 1;
            if (!(pCCW->lpTemplateName = (LPWSTR)LocalAlloc(LPTR, (cbLen * sizeof(WCHAR)))))
            {
                StoreExtendedError(CDERR_MEMALLOCFAILURE);
                return (FALSE);
            }
            else
            {
                MultiByteToWideChar( CP_ACP,
                                     0,
                                     pCCA->lpTemplateName,
                                     -1,
                                     (LPWSTR)pCCW->lpTemplateName,
                                     cbLen );
            }
        }
        else
        {
            pCCW->lpTemplateName = (LPWSTR)pCCA->lpTemplateName;
        }
    }
    else
    {
        pCCW->lpTemplateName = NULL;
    }

    CI.pCC = pCCW;
    CI.pCCA = pCCA;
    CI.ApiType = COMDLG_ANSI;

    ThunkChooseColorA2W(&CI);
    if (bRet = ChooseColorX(&CI))
    {
        ThunkChooseColorW2A(&CI);
    }

    if (HIWORD(pCCW->lpTemplateName))
    {
        LocalFree((HLOCAL)pCCW->lpTemplateName);
    }

    LocalFree(pCCW);

    return (bRet);
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  ChooseColorW
//
//  Stub UNICODE function for ChooseColor when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI ChooseColorW(
    LPCHOOSECOLORW pCCW)
{
    SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#endif



////////////////////////////////////////////////////////////////////////////
//
//  ChooseColor
//
//  The ChooseColor function creates a system-defined dialog box from
//  which the user can select a color.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI ChooseColor(
    LPCHOOSECOLOR pCC)
{
    COLORINFO CI;

    ZeroMemory(&CI, sizeof(COLORINFO));

    CI.pCC = pCC;
    CI.ApiType = COMDLG_WIDE;

    return ( ChooseColorX(&CI) );
}


////////////////////////////////////////////////////////////////////////////
//
//  ChooseColorX
//
//  Worker routine for the ChooseColor api.
//
////////////////////////////////////////////////////////////////////////////

BOOL ChooseColorX(
    PCOLORINFO pCI)
{
    LPCHOOSECOLOR pCC = pCI->pCC;
    INT iRet = FALSE;
    TCHAR szDialog[cbDlgNameMax];
    LPTSTR lpDlg;
    HANDLE hDlgTemplate;
    HRSRC hRes;
#ifdef UNICODE
    UINT uiWOWFlag = 0;
#endif

    //
    //  Initialize the error code.
    //
    StoreExtendedError(0);
    bUserPressedCancel = FALSE;

    if (!pCC)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pCC->lStructSize != sizeof(CHOOSECOLOR))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if (pCC->Flags & CC_ENABLEHOOK)
    {
        if (!pCC->lpfnHook)
        {
            StoreExtendedError(CDERR_NOHOOK);
            return (FALSE);
        }
    }
    else
    {
        pCC->lpfnHook = 0;
    }

    if (pCC->Flags & CC_ENABLETEMPLATE)
    {
        //
        //  Both custom instance handle and the dialog template name are
        //  user specified. Locate the dialog resource in the specified
        //  instance block and load it.
        //
        if (!(hRes = FindResource( (HMODULE)pCC->hInstance,
                                   pCC->lpTemplateName,
                                   RT_DIALOG )))
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (FALSE);
        }
        if (!(hDlgTemplate = LoadResource((HMODULE)pCC->hInstance, hRes)))
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (FALSE);
        }
    }
    else if (pCC->Flags & CC_ENABLETEMPLATEHANDLE)
    {
        //
        //  A handle to the pre-loaded resource has been specified.
        //
        hDlgTemplate = pCC->hInstance;
    }
    else
    {
        if (!LoadString( g_hinst,
                         dlgChooseColor,
                         szDialog,
                         cbDlgNameMax - 1 ))
        {
            StoreExtendedError(CDERR_LOADSTRFAILURE);
            return (FALSE);
        }
        lpDlg = szDialog;

        if (!(hRes = FindResource(g_hinst, lpDlg, RT_DIALOG)))
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (FALSE);
        }
        if (!(hDlgTemplate = LoadResource(g_hinst, hRes)))
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (FALSE);
        }
    }

    if (LockResource(hDlgTemplate))
    {
        if (pCI->pCC->Flags & CC_ENABLEHOOK)
        {
            glpfnColorHook = pCI->pCC->lpfnHook;
        }

#ifdef UNICODE
        if (pCC->Flags & CD_WOWAPP)
        {
            uiWOWFlag = SCDLG_16BIT;
        }

        iRet = DialogBoxIndirectParamAorW( g_hinst,
                                           (LPDLGTEMPLATE)hDlgTemplate,
                                           pCC->hwndOwner,
                                           (DLGPROC)ColorDlgProc,
                                           (LPARAM)pCI,
                                           uiWOWFlag );
#else
        iRet = DialogBoxIndirectParam( g_hinst,
                                       (LPDLGTEMPLATE)hDlgTemplate,
                                       pCC->hwndOwner,
                                       (DLGPROC)ColorDlgProc,
                                       (LPARAM)pCI );
#endif
        glpfnColorHook = 0;
        if ((iRet == 0) && (!bUserPressedCancel) && (!GetStoredExtendedError()))
        {
            StoreExtendedError(CDERR_DIALOGFAILURE);
        }
    }
    else
    {
        StoreExtendedError(CDERR_LOCKRESFAILURE);
    }

    return (iRet == IDOK);
}


////////////////////////////////////////////////////////////////////////////
//
//  ColorDlgProc
//
//  Color Dialog.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI ColorDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PCOLORINFO pCI;
    BOOL bRet;
    BOOL bHookRet = FALSE;

    INT temp;
    PAINTSTRUCT ps;
    HDC hDC;
    RECT rRect;
    RECT rcTemp;
    SHORT id;
    WORD nVal;
    BOOL bUpdateExample = FALSE;
    BOOL bOK;
    HWND hPointWnd;
    TCHAR cEdit[3];
    DWORD FAR *lpCust ;
    int i;
    POINT pt;

    //
    //  The call to PvGetInst will fail until set under WM_INITDIALOG.
    //
    if (pCI = (PCOLORINFO)GetProp(hDlg, COLORPROP))
    {
        if ((pCI->pCC->lpfnHook) &&
            (bRet = (* pCI->pCC->lpfnHook)(hDlg, wMsg, wParam, lParam)))
        {
            if ((wMsg == WM_COMMAND) &&
                (GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL))
            {
                //
                //  Set global flag stating that the user pressed cancel.
                //
                bUserPressedCancel = TRUE;
            }

            return (bRet);
        }
    }
    else if (glpfnColorHook && (wMsg != WM_INITDIALOG) &&
             (bRet = (*glpfnColorHook)(hDlg, wMsg,wParam, lParam)))
    {
        return (bRet);
    }

    switch (wMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            //
            //  Change cursor to hourglass.
            //
            HourGlass(TRUE);

            pCI = (PCOLORINFO)lParam;

            SetProp(hDlg, COLORPROP, (HANDLE)pCI);
            glpfnColorHook = 0;

            bRet = InitColor(hDlg, wParam, pCI);

            //
            //  Change cursor back to arrow.
            //
            HourGlass(FALSE);

            return (bRet);
            break;
        }
        case ( WM_MOVE ) :
        {
            if (pCI)
            {
                SetupRainbowCapture(pCI);
            }
            return(FALSE);
            break;
        }
        case ( WM_LBUTTONDBLCLK ) :
        {
            LONG2POINT(lParam, pt);
            if (PtInRect((LPRECT)&pCI->rNearestPure, pt))
            {
                NearestSolid(pCI);
            }
            break;
        }
        case ( WM_MOUSEMOVE ) :
        {
            //
            //  Dialog Boxes don't receive MOUSEMOVE unless mouse is captured.
            //  If mouse isn't captured, break.
            //
            if (!bMouseCapture)
            {
                break;
            }

            // Fall Thru...
        }
        case ( WM_LBUTTONDOWN ) :
        {
            LONG2POINT(lParam, pt);
            if (PtInRect((LPRECT)&pCI->rRainbow, pt))
            {
                if (wMsg == WM_LBUTTONDOWN)
                {
                    hDC = GetDC(hDlg);
                    EraseCrossHair(hDC, pCI);
                    ReleaseDC(hDlg, hDC);
                }

                pCI->nHuePos = LOWORD(lParam);
                HLSPostoHLS(COLOR_HUE, pCI);
                SetHLSEdit(COLOR_HUE, pCI);

                pCI->nSatPos = HIWORD(lParam);
                HLSPostoHLS(COLOR_SAT, pCI);
                SetHLSEdit(COLOR_SAT, pCI);
                pCI->currentRGB = HLStoRGB( pCI->currentHue,
                                            pCI->currentLum,
                                            pCI->currentSat );

                hDC = GetDC(hDlg);
                RainbowPaint(pCI, hDC, (LPRECT)&pCI->rLumPaint);
                RainbowPaint(pCI, hDC, (LPRECT)&pCI->rColorSamples);
                ReleaseDC(hDlg, hDC);

                SetRGBEdit(0, pCI);

                if (!bMouseCapture)
                {
                    SetCapture(hDlg);
                    CopyRect(&rcTemp, &pCI->rRainbow);
                    ClientToScreen(hDlg, (LPPOINT)&rcTemp.left);
                    ClientToScreen(hDlg, (LPPOINT)&rcTemp.right);
                    ClipCursor(&rcTemp);
                    bMouseCapture = TRUE;
                }
            }
            else if ( PtInRect((LPRECT)&pCI->rLumPaint, pt) ||
                      PtInRect((LPRECT)&pCI->rLumScroll, pt) )
            {
                hDC = GetDC(hDlg);
                EraseLumArrow(hDC, pCI);
                LumArrowPaint(hDC, pCI->nLumPos = HIWORD(lParam), pCI);
                HLSPostoHLS(COLOR_LUM, pCI);
                SetHLSEdit(COLOR_LUM, pCI);
                pCI->currentRGB = HLStoRGB( pCI->currentHue,
                                            pCI->currentLum,
                                            pCI->currentSat );

                RainbowPaint(pCI, hDC, (LPRECT)&pCI->rColorSamples);
                ReleaseDC(hDlg, hDC);
                ValidateRect(hDlg, (LPRECT)&pCI->rLumScroll);
                ValidateRect(hDlg, (LPRECT)&pCI->rColorSamples);

                SetRGBEdit(0, pCI);

                if (!bMouseCapture)
                {
                    SetCapture(hDlg);
                    CopyRect(&rcTemp, &pCI->rLumScroll);
                    ClientToScreen(hDlg, (LPPOINT)&rcTemp.left);
                    ClientToScreen(hDlg, (LPPOINT)&rcTemp.right);
                    ClipCursor(&rcTemp);
                    bMouseCapture = TRUE;
                }
            }
            else
            {
                hPointWnd = ChildWindowFromPoint(hDlg, pt);
                if (hPointWnd == GetDlgItem(hDlg, COLOR_BOX1))
                {
                    rRect.top    = rColorBox[0].top;
                    rRect.left   = rColorBox[0].left;
                    rRect.right  = rColorBox[NUM_BASIC_COLORS - 1].right +
                                   BOX_X_MARGIN;
                    rRect.bottom = rColorBox[NUM_BASIC_COLORS - 1].bottom +
                                   BOX_Y_MARGIN;
                    temp = (NUM_BASIC_COLORS) / NUM_X_BOXES;
                    id = 0;
                }
                else if (hPointWnd == GetDlgItem(hDlg, COLOR_CUSTOM1))
                {
                    rRect.top    = rColorBox[NUM_BASIC_COLORS].top;
                    rRect.left   = rColorBox[NUM_BASIC_COLORS].left;
                    rRect.right  = rColorBox[COLORBOXES - 1].right + BOX_X_MARGIN;
                    rRect.bottom = rColorBox[COLORBOXES - 1].bottom + BOX_Y_MARGIN;
                    temp = (NUM_CUSTOM_COLORS) / NUM_X_BOXES;
                    id = NUM_BASIC_COLORS;
                }
                else
                {
                    return (FALSE);
                }

                if (hPointWnd != GetFocus())
                {
                    SetFocus(hPointWnd);
                }

                if (HIWORD(lParam) >= (WORD)rRect.bottom)
                {
                    break;
                }
                if (LOWORD(lParam) >= (WORD)rRect.right)
                {
                    break;
                }
                if ( ((LOWORD(lParam) - rRect.left) % nBoxWidth) >=
                     (nBoxWidth - BOX_X_MARGIN) )
                {
                    break;
                }
                if ( ((HIWORD(lParam) - rRect.top) % nBoxHeight) >=
                     (nBoxHeight - BOX_Y_MARGIN) )
                {
                    break;
                }

                id += ((HIWORD(lParam) - rRect.top) * temp /
                       (rRect.bottom - rRect.top)) * NUM_X_BOXES;

                id += ((LOWORD(lParam) - rRect.left) * NUM_X_BOXES) /
                       (SHORT)(rRect.right - rRect.left);

                if ((id < nDriverColors) || (id >= NUM_BASIC_COLORS))
                {
                    ChangeBoxSelection(pCI, id);
                    pCI->nCurBox = id;
                    ChangeBoxFocus(pCI, id);
                    if (id >= NUM_BASIC_COLORS)
                    {
                        pCI->nCurMix = pCI->nCurBox;
                    }
                    else
                    {
                        pCI->nCurDsp = pCI->nCurBox;
                    }
                    pCI->currentRGB = pCI->rgbBoxColor[pCI->nCurBox];
                    hDC = GetDC(hDlg);
                    if (pCI->bFoldOut)
                    {
                        ChangeColorSettings(pCI);
                        SetHLSEdit(0, pCI);
                        SetRGBEdit(0, pCI);
                        RainbowPaint(pCI, hDC, (LPRECT)&pCI->rColorSamples);
                    }
                    PaintBox(pCI, hDC, pCI->nCurDsp);
                    PaintBox(pCI, hDC, pCI->nCurMix);
                    ReleaseDC(hDlg, hDC);
                }
            }
            break;
        }
        case ( WM_LBUTTONUP ) :
        {
            LONG2POINT(lParam, pt);
            if (bMouseCapture)
            {
                bMouseCapture = FALSE;
                SetCapture(NULL);
                ClipCursor((LPRECT)NULL);
                if (PtInRect((LPRECT)&pCI->rRainbow, pt))
                {
                    hDC = GetDC(hDlg);
                    CrossHairPaint( hDC,
                                    pCI->nHuePos = LOWORD(lParam),
                                    pCI->nSatPos = HIWORD(lParam),
                                    pCI );
                    RainbowPaint(pCI, hDC, (LPRECT)&pCI->rLumPaint);
                    ReleaseDC(hDlg, hDC);
                    ValidateRect(hDlg, (LPRECT)&pCI->rRainbow);
                }
                else if (PtInRect((LPRECT)&pCI->rLumPaint, pt))
                {
                    //
                    //  Update Sample Shown.
                    //
                    hDC = GetDC(hDlg);
                    LumArrowPaint(hDC, pCI->nLumPos, pCI);
                    ReleaseDC(hDlg, hDC);
                    ValidateRect(hDlg, (LPRECT)&pCI->rLumPaint);
                }
            }
            break;
        }
        case ( WM_CHAR ) :
        {
            if (wParam == VK_SPACE)
            {
                if (GetFocus() == GetDlgItem(hDlg, COLOR_BOX1))
                {
                    temp = pCI->nCurDsp;
                }
                else if (GetFocus() == GetDlgItem(hDlg, COLOR_CUSTOM1))
                {
                    temp = pCI->nCurMix;
                }
                else
                {
                    return (FALSE);
                }
                pCI->currentRGB = pCI->rgbBoxColor[temp];
                if (pCI->bFoldOut)
                {
                    ChangeColorSettings(pCI);
                    SetHLSEdit(0, pCI);
                    SetRGBEdit(0, pCI);
                }
                InvalidateRect(hDlg, (LPRECT)&pCI->rColorSamples, FALSE);
                ChangeBoxSelection(pCI, (short)temp);
                pCI->nCurBox = (short)temp;
                bUpdateExample = TRUE;
            }
            break;
        }
        case ( WM_KEYDOWN ) :
        {
            if (ColorKeyDown(wParam, &temp, pCI))
            {
                ChangeBoxFocus(pCI, (SHORT)temp);
            }
            break;
        }
        case ( WM_GETDLGCODE ) :
        {
            return (DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_HASSETSEL);
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ( IDOK ) :
                {
                    pCI->pCC->rgbResult = pCI->currentRGB;
                    goto LeaveDialog;
                }
                case ( IDCANCEL ) :
                {
                    bUserPressedCancel = TRUE;
LeaveDialog:
                    if (bMouseCapture)
                    {
                        bMouseCapture = FALSE;
                        SetCapture(NULL);
                        ClipCursor((LPRECT)NULL);
                    }
                    lpCust = pCI->pCC->lpCustColors;
                    for ( i = NUM_BASIC_COLORS;
                          i < NUM_BASIC_COLORS + NUM_CUSTOM_COLORS;
                          i++ )
                    {
                        *lpCust++ = pCI->rgbBoxColor[i];
                    }

                    bRet = (GET_WM_COMMAND_ID(wParam, lParam) == IDOK);

#ifdef UNICODE
                    if (pCI->ApiType == COMDLG_ANSI)
                    {
                        if (bRet && pCI->pCC->lpfnHook)
                        {
                            ThunkChooseColorW2A(pCI);
                            bHookRet = (*pCI->pCC->lpfnHook)(
                                               hDlg,
                                               msgCOLOROKA,
                                               0,
                                               (LONG)(LPTSTR)pCI->pCCA );
                        }
                    }
                    else
#endif
                    {
                        if (bRet && pCI->pCC->lpfnHook)
                        {
                            bHookRet = (*pCI->pCC->lpfnHook)(
                                               hDlg,
                                               msgCOLOROKW,
                                               0,
                                               (LONG)(LPTSTR)pCI->pCC );
                        }
                    }

                    if (bHookRet)
                    {
#ifdef UNICODE
                        if (pCI->ApiType == COMDLG_ANSI)
                        {
                            ThunkChooseColorA2W(pCI);
                            pCI->pCC->lCustData = pCI->pCCA->lCustData;
                        }
#endif
                        break;
                    }
                }
                case ( IDABORT ) :
                {
                    if (pCI->pCC->Flags & CC_ENABLEHOOK)
                    {
                        glpfnColorHook = pCI->pCC->lpfnHook;
                    }

                    RemoveProp(hDlg, COLORPROP);
                    EndDialog( hDlg,
                               (GET_WM_COMMAND_ID(wParam, lParam) == IDABORT)
                                   ? (BOOL)lParam
                                   : bRet );

                    if (hRainbowBitmap)
                    {
                        DeleteObject(hRainbowBitmap);
                        hRainbowBitmap = NULL;
                    }
                    if (hDCFastBlt)
                    {
                        DeleteDC(hDCFastBlt);
                        hDCFastBlt = 0;
                    }
                    break;
                }
                case ( psh15 ) :
                {
#ifdef UNICODE
                    if (pCI->ApiType == COMDLG_ANSI)
                    {
                        if (msgHELPA && pCI->pCC->hwndOwner)
                        {
                            ThunkChooseColorW2A(pCI);
                            SendMessage( pCI->pCC->hwndOwner,
                                         msgHELPA,
                                         (WPARAM)hDlg,
                                         (LPARAM)pCI->pCCA );
                            ThunkChooseColorA2W(pCI);
                            pCI->pCC->lCustData = pCI->pCCA->lCustData;
                        }
                    }
                    else
#endif
                    {
                        if (msgHELPW && pCI->pCC->hwndOwner)
                        {
                            SendMessage( pCI->pCC->hwndOwner,
                                         msgHELPW,
                                         (WPARAM)hDlg,
                                         (LPARAM)pCI->pCC );
                        }
                    }
                    break;
                }
                case ( COLOR_SOLID ) :
                {
                    NearestSolid(pCI);
                    break;
                }
                case ( COLOR_RED ) :
                case ( COLOR_GREEN ) :
                case ( COLOR_BLUE ) :
                {
                    if (GET_WM_COMMAND_CMD(wParam,lParam) == EN_CHANGE)
                    {
                        RGBEditChange(GET_WM_COMMAND_ID(wParam,lParam), pCI);
                        InvalidateRect(hDlg, (LPRECT)&pCI->rColorSamples, FALSE);
                    }
                    else if (GET_WM_COMMAND_CMD(wParam,lParam) == EN_KILLFOCUS)
                    {
                        GetDlgItemInt( hDlg,
                                       GET_WM_COMMAND_ID(wParam, lParam),
                                       &bOK,
                                       FALSE );
                        if (!bOK)
                        {
                            SetRGBEdit(GET_WM_COMMAND_ID(wParam,lParam), pCI);
                        }
                    }
                    break;
                }
                case ( COLOR_HUE ) :
                {
                    if (GET_WM_COMMAND_CMD(wParam,lParam) == EN_CHANGE)
                    {
                        nVal = (WORD)GetDlgItemInt(hDlg, COLOR_HUE, &bOK, FALSE);
                        if (bOK)
                        {
                            if (nVal > RANGE - 1)
                            {
                                nVal = RANGE - 1;
                                SetDlgItemInt(hDlg, COLOR_HUE, (INT)nVal, FALSE);
                            }
                            if (nVal != pCI->currentHue)
                            {
                                hDC = GetDC(hDlg);
                                EraseCrossHair(hDC, pCI);
                                pCI->currentHue = nVal;
                                pCI->currentRGB = HLStoRGB( nVal,
                                                            pCI->currentLum,
                                                            pCI->currentSat );
                                SetRGBEdit(0, pCI);
                                HLStoHLSPos(COLOR_HUE, pCI);
                                CrossHairPaint( hDC,
                                                pCI->nHuePos,
                                                pCI->nSatPos,
                                                pCI );
                                ReleaseDC(hDlg, hDC);
                                InvalidateRect( hDlg,
                                                (LPRECT)&pCI->rLumPaint,
                                                FALSE );
                                InvalidateRect( hDlg,
                                                (LPRECT)&pCI->rColorSamples,
                                                FALSE );
                                UpdateWindow(hDlg);
                            }
                        }
                        else if (GetDlgItemText(
                                       hDlg,
                                       COLOR_HUE,
                                       (LPTSTR)cEdit,
                                       2 ))
                        {
                            SetHLSEdit(COLOR_HUE, pCI);
                            SendDlgItemMessage(
                                       hDlg,
                                       COLOR_HUE,
                                       EM_SETSEL,
                                       (WPARAM)0,
                                       (LPARAM)-1 );
                        }
                    }
                    else if (GET_WM_COMMAND_CMD(wParam,lParam) == EN_KILLFOCUS)
                    {
                        GetDlgItemInt(hDlg, COLOR_HUE, &bOK, FALSE);
                        if (!bOK)
                        {
                            SetHLSEdit(COLOR_HUE, pCI);
                        }
                    }
                    break;
                }
                case ( COLOR_SAT ) :
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        nVal = (WORD)GetDlgItemInt(hDlg, COLOR_SAT, &bOK, FALSE);
                        if (bOK)
                        {
                            if (nVal > RANGE)
                            {
                                nVal = RANGE;
                                SetDlgItemInt(hDlg, COLOR_SAT, (INT)nVal, FALSE);
                            }
                            if (nVal != pCI->currentSat)
                            {
                                hDC = GetDC(hDlg);
                                EraseCrossHair(hDC, pCI);
                                pCI->currentSat = nVal;
                                pCI->currentRGB = HLStoRGB( pCI->currentHue,
                                                            pCI->currentLum,
                                                            nVal );
                                SetRGBEdit(0, pCI);
                                HLStoHLSPos(COLOR_SAT, pCI);
                                CrossHairPaint( hDC,
                                                pCI->nHuePos,
                                                pCI->nSatPos,
                                                pCI );
                                ReleaseDC(hDlg, hDC);
                                InvalidateRect( hDlg,
                                                (LPRECT)&pCI->rLumPaint,
                                                FALSE );
                                InvalidateRect( hDlg,
                                                (LPRECT)&pCI->rColorSamples,
                                                FALSE );
                                UpdateWindow(hDlg);
                            }
                        }
                        else if (GetDlgItemText(
                                       hDlg,
                                       COLOR_SAT,
                                       (LPTSTR)cEdit,
                                       2 ))
                        {
                            SetHLSEdit(COLOR_SAT, pCI);
                            SendDlgItemMessage(
                                       hDlg,
                                       COLOR_SAT,
                                       EM_SETSEL,
                                       (WPARAM)0,
                                       (LPARAM)-1 );
                        }
                    }
                    else if (GET_WM_COMMAND_CMD(wParam,lParam) == EN_KILLFOCUS)
                    {
                        GetDlgItemInt(hDlg, COLOR_SAT, &bOK, FALSE);
                        if (!bOK)
                        {
                            SetHLSEdit(COLOR_SAT, pCI);
                        }
                    }
                    break;
                }
                case ( COLOR_LUM ) :
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        nVal = (WORD)GetDlgItemInt(hDlg, COLOR_LUM, &bOK, FALSE);
                        if (bOK)
                        {
                            if (nVal > RANGE)
                            {
                                nVal = RANGE;
                                SetDlgItemInt(hDlg, COLOR_LUM, (INT)nVal, FALSE);
                            }
                            if (nVal != pCI->currentLum)
                            {
                                hDC = GetDC(hDlg);
                                EraseLumArrow(hDC, pCI);
                                pCI->currentLum = nVal;
                                HLStoHLSPos(COLOR_LUM, pCI);
                                pCI->currentRGB = HLStoRGB( pCI->currentHue,
                                                            nVal,
                                                            pCI->currentSat );
                                SetRGBEdit(0, pCI);
                                LumArrowPaint(hDC, pCI->nLumPos, pCI);
                                ReleaseDC(hDlg, hDC);
                                InvalidateRect( hDlg,
                                                (LPRECT)&pCI->rColorSamples,
                                                FALSE );
                                UpdateWindow(hDlg);
                            }
                        }
                        else if (GetDlgItemText(
                                       hDlg,
                                       COLOR_LUM,
                                       (LPTSTR)cEdit,
                                       2 ))
                        {
                            SetHLSEdit(COLOR_LUM, pCI);
                            SendDlgItemMessage(
                                       hDlg,
                                       COLOR_LUM,
                                       EM_SETSEL,
                                       (WPARAM)0,
                                       (LPARAM)-1 );
                        }
                    }
                    else if (GET_WM_COMMAND_CMD(wParam,lParam) == EN_KILLFOCUS)
                    {
                        GetDlgItemInt(hDlg, COLOR_LUM, &bOK, FALSE);
                        if (!bOK)
                        {
                            SetHLSEdit(COLOR_LUM, pCI);
                        }
                    }
                    break;
                }
                case ( COLOR_ADD ) :
                {
                    pCI->rgbBoxColor[pCI->nCurMix] = pCI->currentRGB;
                    InvalidateRect(hDlg, (LPRECT)rColorBox + pCI->nCurMix, FALSE);

                    if (pCI->nCurMix >= COLORBOXES - 1)
                    {
                        pCI->nCurMix = NUM_BASIC_COLORS;
                    }
#if HORIZONTELINC
                    else
                    {
                        pCI->nCurMix++;
                    }
#else
                    else if (pCI->nCurMix >= NUM_BASIC_COLORS + 8)
                    {
                        //
                        //  Increment nCurBox VERTICALLY!  Foolish extra code
                        //  for vertical instead of horizontal increment.
                        //
                        pCI->nCurMix -= 7;
                    }
                    else
                    {
                        pCI->nCurMix += 8;
                    }
#endif
                    break;
                }
                case ( COLOR_MIX ) :
                {
                    //
                    //  Change cursor to hourglass.
                    //
                    HourGlass(TRUE);

                    InitRainbow(pCI);

                    //
                    //  Code relies on COLOR_HUE through COLOR_BLUE being
                    //  consecutive.
                    //
                    for (temp = COLOR_HUE; temp <= COLOR_BLUE; temp++)
                    {
                        EnableWindow(GetDlgItem(hDlg, temp), TRUE);
                    }
                    for (temp = COLOR_HUEACCEL; temp <= COLOR_BLUEACCEL; temp++)
                    {
                        EnableWindow(GetDlgItem(hDlg, temp), TRUE);
                    }
                    EnableWindow(GetDlgItem(hDlg, COLOR_ADD), TRUE);
                    EnableWindow(GetDlgItem(hDlg, COLOR_MIX), FALSE);

                    GetWindowRect(hDlg, (LPRECT)&rcTemp);

                    SetWindowPos( hDlg,
                                  NULL,
                                  pCI->rOriginal.left,
                                  pCI->rOriginal.top,
                                  pCI->rOriginal.right - pCI->rOriginal.left,
                                  pCI->rOriginal.bottom - pCI->rOriginal.top,
                                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

                    //
                    //  Only invalidate exposed area.
                    //
                    rcTemp.right = rcTemp.left;
                    rcTemp.left = pCI->rOriginal.left;
                    InvalidateRect(hDlg, (LPRECT)&rcTemp, FALSE);

                    //
                    //  Change cursor back to arrow.
                    //
                    HourGlass(FALSE);

                    SetFocus(GetDlgItem(hDlg, COLOR_HUE));
                    pCI->bFoldOut = TRUE;

                    break;
                }
            }
            break;
        }
        case ( WM_PAINT ) :
        {
            BeginPaint(hDlg, (LPPAINTSTRUCT)&ps);
            ColorPaint(hDlg, pCI, ps.hdc, (LPRECT)&ps.rcPaint);
            EndPaint(hDlg, (LPPAINTSTRUCT)&ps);
            break;
        }
        default :
        {
            if (wMsg == msgSETRGBA || wMsg == msgSETRGBW)
            {
                if (ChangeColorBox(pCI, (DWORD)lParam))
                {
                    pCI->currentRGB = lParam;

                    if (pCI->nCurBox < pCI->nCurMix)
                    {
                        pCI->nCurDsp = pCI->nCurBox;
                    }
                    else
                    {
                        pCI->nCurMix = pCI->nCurBox;
                    }
                }
                if (pCI->bFoldOut)
                {
                    pCI->currentRGB = lParam;
                    ChangeColorSettings(pCI);
                    SetHLSEdit(0, pCI);
                    SetRGBEdit(0, pCI);
                    hDC = GetDC(hDlg);
                    RainbowPaint(pCI, hDC, (LPRECT)&pCI->rColorSamples);
                    ReleaseDC(hDlg, hDC);
                }
                break;
            }
            return (FALSE);
            break;
        }
    }
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ChangeColorBox
//
//  Update box shown.
//
////////////////////////////////////////////////////////////////////////////

BOOL ChangeColorBox(
    register PCOLORINFO pCI,
    DWORD dwRGBcolor)
{
    register short nBox;

    for (nBox = 0; nBox < COLORBOXES; nBox++)
    {
        if (pCI->rgbBoxColor[nBox] == dwRGBcolor)
        {
            break;
        }
    }
    if (nBox >= COLORBOXES)
    {
        //
        //  Color Not Found.  Now What Should We Do?
        //
    }
    else
    {
        ChangeBoxSelection(pCI, nBox);
        pCI->nCurBox = nBox;
    }

    return (nBox < COLORBOXES);
}


////////////////////////////////////////////////////////////////////////////
//
//  HiLiteBox
//
////////////////////////////////////////////////////////////////////////////

VOID HiLiteBox(
    HDC hDC,
    SHORT nBox,
    SHORT fStyle)
{
    RECT rRect;
    HBRUSH hBrush;

    CopyRect((LPRECT)&rRect, (LPRECT)rColorBox + nBox);
    rRect.left--, rRect.top--, rRect.right++, rRect.bottom++;
    hBrush = CreateSolidBrush((fStyle & 1) ? 0L : GetSysColor(COLOR_3DFACE));
    FrameRect(hDC, (LPRECT)&rRect, hBrush);
    DeleteObject(hBrush);
}


////////////////////////////////////////////////////////////////////////////
//
//  ChangeBoxSelection
//
////////////////////////////////////////////////////////////////////////////

VOID ChangeBoxSelection(
    PCOLORINFO pCI,
    SHORT nNewBox)
{
    register HDC hDC;
    register HWND hDlg = pCI->hDialog;

    hDC = GetDC(hDlg);
    HiLiteBox(hDC, pCI->nCurBox, 0);
    HiLiteBox(hDC, nNewBox, 1);
    ReleaseDC(hDlg, hDC);
    pCI->currentRGB = pCI->rgbBoxColor[nNewBox];
}


////////////////////////////////////////////////////////////////////////////
//
//  ChangeBoxFocus
//
//  Can't trust the state of the XOR for DrawFocusRect, so must draw
//  the rectangle in the window background color first.
//
////////////////////////////////////////////////////////////////////////////

VOID ChangeBoxFocus(
    PCOLORINFO pCI,
    SHORT nNewBox)
{
    HANDLE hDlg = pCI->hDialog;
    HDC    hDC;
    RECT   rRect;
    LPWORD nCur = (LPWORD)((nNewBox < (NUM_BASIC_COLORS))
                               ? (LONG)&pCI->nCurDsp
                               : (LONG)&pCI->nCurMix);
    HPEN   hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DFACE));
    HBRUSH hBrush = GetStockObject(HOLLOW_BRUSH);

    hDC = GetDC(hDlg);
    hPen = SelectObject(hDC, hPen);
    hBrush = SelectObject(hDC, hBrush);
    CopyRect((LPRECT)&rRect, (LPRECT)rColorBox + *nCur);
    InflateRect((LPRECT)&rRect, 3, 3);
    Rectangle(hDC, rRect.left, rRect.top, rRect.right, rRect.bottom);
    CopyRect((LPRECT)&rRect, (LPRECT)rColorBox + (*nCur = nNewBox));
    InflateRect((LPRECT)&rRect, 3, 3);
    Rectangle(hDC, rRect.left, rRect.top, rRect.right, rRect.bottom);
#ifndef NTWIN
    DrawFocusRect(hDC, (LPRECT)&rRect);
#endif
    hPen = SelectObject(hDC, hPen);
    SelectObject(hDC, hBrush);
    ReleaseDC(hDlg, hDC);
    DeleteObject(hPen);
}


////////////////////////////////////////////////////////////////////////////
//
//  ColorKeyDown
//
////////////////////////////////////////////////////////////////////////////

BOOL ColorKeyDown(
    WPARAM wParam,
    INT FAR *id,
    PCOLORINFO pCI)
{
    WORD temp;

    temp = (WORD)GetWindowLong(GetFocus(), GWL_ID);
    if (temp == COLOR_BOX1)
    {
        temp = pCI->nCurDsp;
    }
    else if (temp == COLOR_CUSTOM1)
    {
        temp = pCI->nCurMix;
    }
    else
    {
        return (FALSE);
    }

    switch (wParam)
    {
        case ( VK_UP ) :
        {
            if (temp >= (NUM_BASIC_COLORS + NUM_X_BOXES))
            {
                temp -= NUM_X_BOXES;
            }
            else if ((temp < NUM_BASIC_COLORS) && (temp >= NUM_X_BOXES))
            {
                temp -= NUM_X_BOXES;
            }
            break;
        }
#if 1
        case ( VK_HOME ) :
        {
            if (temp == pCI->nCurDsp)
            {
                temp = 0;
            }
            else
            {
                temp = NUM_BASIC_COLORS;
            }
            break;
        }
        case ( VK_END ) :
        {
            if (temp == pCI->nCurDsp)
            {
                temp = (WORD)(nDriverColors - 1);
            }
            else
            {
                temp = COLORBOXES - 1;
            }
            break;
        }
#endif
        case ( VK_DOWN ) :
        {
            if (temp < (NUM_BASIC_COLORS - NUM_X_BOXES))
            {
                temp += NUM_X_BOXES;
            }
            else if ((temp >= (NUM_BASIC_COLORS)) &&
                     (temp < (COLORBOXES - NUM_X_BOXES)))
            {
                temp += NUM_X_BOXES;
            }
            break;
        }
        case ( VK_LEFT ) :
        {
            if (temp % NUM_X_BOXES)
            {
                temp--;
            }
            break;
        }
        case ( VK_RIGHT ) :
        {
            if (!(++temp % NUM_X_BOXES))
            {
                --temp;
            }
            break;
        }
    }

    //
    //  If we've received colors from the driver, make certain the arrow would
    //  not take us to an undefined color.
    //
    if ((temp >= (WORD)nDriverColors) && (temp < NUM_BASIC_COLORS))
    {
        temp = pCI->nCurDsp;
    }

    *id = temp;
    return ((temp != pCI->nCurDsp) && (temp != pCI->nCurMix));
}


////////////////////////////////////////////////////////////////////////////
//
//  PaintBox
//
////////////////////////////////////////////////////////////////////////////

VOID PaintBox(
    PCOLORINFO pCI,
    register HDC hDC,
    SHORT i)
{
    register HBRUSH hBrush, hBrushOld;

    if ((i < NUM_BASIC_COLORS) && (i >= nDriverColors))
    {
        return;
    }
    hBrush = CreateSolidBrush(pCI->rgbBoxColor[i]);

    hBrushOld = SelectObject(hDC, hBrush);
    Rectangle( hDC,
               rColorBox[i].left,
               rColorBox[i].top,
               rColorBox[i].right,
               rColorBox[i].bottom );
    hBrush = SelectObject(hDC, hBrushOld);
    DeleteObject(hBrush);

    if (i == (short)pCI->nCurBox)
    {
        HiLiteBox(hDC, i, 1);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  InitScreenCoords
//
//  Returns TRUE iff we make it.
//
////////////////////////////////////////////////////////////////////////////

BOOL InitScreenCoords(
    HWND hDlg,
    PCOLORINFO pCI)
{
    RECT rRect;
    SHORT i;
    DWORD FAR *lpDriverRGB;
    HWND hBox1, hCustom1;

    hBox1 = GetDlgItem(hDlg, COLOR_BOX1);
    hCustom1 = GetDlgItem(hDlg, COLOR_CUSTOM1);
    lpprocStatic = (WNDPROC)GetWindowLong(hBox1, GWL_WNDPROC);
    SetWindowLong(hBox1, GWL_WNDPROC, (LONG)WantArrows);
    SetWindowLong(hCustom1, GWL_WNDPROC, (LONG)WantArrows);

    GetWindowRect(hBox1, (LPRECT)&rRect);
    ScreenToClient(hDlg, (LPPOINT)&rRect.left);
    ScreenToClient(hDlg, (LPPOINT)&rRect.right);
    rRect.left += (BOX_X_MARGIN + 1) / 2;
    rRect.top += (BOX_Y_MARGIN + 1) / 2;
    rRect.right -= (BOX_X_MARGIN + 1) / 2;
    rRect.bottom -= (BOX_Y_MARGIN + 1) / 2;
    nBoxWidth = (SHORT)((rRect.right - rRect.left) / NUM_X_BOXES);
    nBoxHeight = (SHORT)((rRect.bottom - rRect.top) /
                         (NUM_BASIC_COLORS / NUM_X_BOXES));

    //
    //  Assume no colors from driver.
    //
    nDriverColors = 0;

    for (i = 0; i < NUM_BASIC_COLORS; i++)
    {
        rColorBox[i].left = rRect.left + nBoxWidth * (i % NUM_X_BOXES);
        rColorBox[i].right = rColorBox[i].left + nBoxWidth - BOX_X_MARGIN;
        rColorBox[i].top = rRect.top + nBoxHeight * (i / NUM_X_BOXES);
        rColorBox[i].bottom = rColorBox[i].top + nBoxHeight - BOX_Y_MARGIN;

        //
        //  Setup the colors.  If the driver still has colors to give, take it.
        //  If not, if the driver actually gave colors, set the color to white.
        //  Otherwise set to the default colors.
        //
        if (i < nDriverColors)
        {
            pCI->rgbBoxColor[i] = *lpDriverRGB++;
        }
        else
        {
            pCI->rgbBoxColor[i] = nDriverColors
                                      ? 0xFFFFFF
                                      : rgbBoxColorDefault[i];
        }
    }

    //
    //  If no driver colors, use default number.
    //
    if (!nDriverColors)
    {
        nDriverColors = NUM_BASIC_COLORS;
    }

    GetWindowRect(hCustom1, (LPRECT)&rRect);
    ScreenToClient(hDlg, (LPPOINT)&rRect.left);
    ScreenToClient(hDlg, (LPPOINT)&rRect.right);
    rRect.left += (BOX_X_MARGIN + 1) / 2;
    rRect.top += (BOX_Y_MARGIN + 1) / 2;
    rRect.right -= (BOX_X_MARGIN + 1) / 2;
    rRect.bottom -= (BOX_Y_MARGIN + 1) / 2;

    for (; i < COLORBOXES; i++)
    {
        rColorBox[i].left = rRect.left +
                       nBoxWidth * ((i - (NUM_BASIC_COLORS)) % NUM_X_BOXES);
        rColorBox[i].right = rColorBox[i].left + nBoxWidth - BOX_X_MARGIN;
        rColorBox[i].top = rRect.top +
                       nBoxHeight * ((i - (NUM_BASIC_COLORS)) / NUM_X_BOXES);
        rColorBox[i].bottom = rColorBox[i].top + nBoxHeight - BOX_Y_MARGIN;
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetupRainbowCapture
//
////////////////////////////////////////////////////////////////////////////

VOID SetupRainbowCapture(
    PCOLORINFO pCI)
{
    HWND hCurrentColor;
    HWND hDlg = pCI->hDialog;

    GetWindowRect(GetDlgItem(hDlg, COLOR_RAINBOW), &pCI->rRainbow);
    pCI->rRainbow.left++, pCI->rRainbow.top++;
    pCI->rRainbow.right--, pCI->rRainbow.bottom--;

    ScreenToClient(hDlg, (LPPOINT)&pCI->rRainbow.left);
    ScreenToClient(hDlg, (LPPOINT)&pCI->rRainbow.right);

    GetWindowRect(GetDlgItem(hDlg, COLOR_LUMSCROLL), &pCI->rLumPaint);
    pCI->rLumPaint.right += (cxSize >> 1);

    ScreenToClient(hDlg, (LPPOINT)&pCI->rLumPaint.left);
    ScreenToClient(hDlg, (LPPOINT)&pCI->rLumPaint.right);
    CopyRect(&pCI->rLumScroll, &pCI->rLumPaint);
    pCI->rLumScroll.left = pCI->rLumScroll.right;
    pCI->rLumScroll.right += (cxSize >> 1);
    pCI->nLumHeight = (WORD)(pCI->rLumPaint.bottom - pCI->rLumPaint.top);

    hCurrentColor = GetDlgItem(hDlg, COLOR_CURRENT);
    GetWindowRect(hCurrentColor, &pCI->rCurrentColor);

    ++pCI->rCurrentColor.left;
    pCI->rNearestPure.right = pCI->rCurrentColor.right - 1;
    pCI->rNearestPure.top = ++pCI->rCurrentColor.top;
    pCI->rNearestPure.bottom = --pCI->rCurrentColor.bottom;
    pCI->rCurrentColor.right += pCI->rCurrentColor.left;
    pCI->rCurrentColor.right /= 2;
    pCI->rNearestPure.left = pCI->rCurrentColor.right;

    ScreenToClient(hDlg, (LPPOINT)&pCI->rCurrentColor.left);
    ScreenToClient(hDlg, (LPPOINT)&pCI->rCurrentColor.right);
    ScreenToClient(hDlg, (LPPOINT)&pCI->rNearestPure.left);
    ScreenToClient(hDlg, (LPPOINT)&pCI->rNearestPure.right);

    pCI->rColorSamples.left = pCI->rCurrentColor.left;
    pCI->rColorSamples.right = pCI->rNearestPure.right;
    pCI->rColorSamples.top = pCI->rCurrentColor.top;
    pCI->rColorSamples.bottom = pCI->rNearestPure.bottom;
}


////////////////////////////////////////////////////////////////////////////
//
//  InitColor
//
//  Returns TRUE iff everything's OK.
//
////////////////////////////////////////////////////////////////////////////

BOOL InitColor(
    HWND hDlg,
    WPARAM wParam,
    PCOLORINFO pCI)
{
    SHORT i;
    RECT rRect;
    LPCHOOSECOLOR pCC = pCI->pCC;
    HDC hDC;
    DWORD FAR *lpCust;
    BOOL bRet;

    if (!hDCFastBlt)
    {
        hDC = GetDC(hDlg);
        hDCFastBlt = CreateCompatibleDC(hDC);
        ReleaseDC(hDlg, hDC);
        if (!hDCFastBlt)
        {
            return(FALSE);
        }
    }

    pCI->hDialog = hDlg;

    SetupRainbowCapture(pCI);

    if (pCC->Flags & CC_RGBINIT)
    {
        pCI->currentRGB = pCC->rgbResult;
    }
    else
    {
        pCI->currentRGB = 0L;
    }
    if (pCC->Flags & (CC_PREVENTFULLOPEN | CC_FULLOPEN))
    {
        EnableWindow(GetDlgItem(hDlg, COLOR_MIX), FALSE);
    }

    if (pCC->Flags & CC_FULLOPEN)
    {
        InitRainbow(pCI);
        pCI->bFoldOut = TRUE;
        RGBtoHLS(pCI->currentRGB);
        pCI->currentHue = H;
        pCI->currentSat = S;
        pCI->currentLum = L;
        SetRGBEdit(0, pCI);
        SetHLSEdit(0, pCI);
    }
    else
    {
        //
        //  Code relies on COLOR_HUE through COLOR_BLUE being consecutive.
        //
        for (i = COLOR_HUE; i <= COLOR_BLUE; i++)
        {
            EnableWindow(GetDlgItem(hDlg, i), FALSE);
        }
        for (i = COLOR_HUEACCEL; i <= COLOR_BLUEACCEL; i++)
        {
            EnableWindow(GetDlgItem(hDlg, i), FALSE);
        }

        EnableWindow(GetDlgItem(hDlg, COLOR_ADD), FALSE);

        pCI->bFoldOut = FALSE;

        GetWindowRect(GetDlgItem(hDlg, COLOR_BOX1), &rRect);
        i = (SHORT)rRect.right;
        GetWindowRect(GetDlgItem(hDlg, COLOR_RAINBOW), &rRect);
        GetWindowRect(hDlg, &(pCI->rOriginal));
        MoveWindow( hDlg,
                    pCI->rOriginal.left,
                    pCI->rOriginal.top,
                    (rRect.left + i) / 2 - pCI->rOriginal.left,
                    pCI->rOriginal.bottom - pCI->rOriginal.top,
                    FALSE );
    }

    InitScreenCoords(hDlg, pCI);

    lpCust = pCC->lpCustColors;
    for (i = NUM_BASIC_COLORS; i < NUM_BASIC_COLORS + NUM_CUSTOM_COLORS; i++)
    {
        pCI->rgbBoxColor[i] = *lpCust++;
    }

    pCI->nCurBox = pCI->nCurDsp = 0;
    pCI->nCurMix = NUM_BASIC_COLORS;
    ChangeColorBox(pCI, pCI->currentRGB);
    if (pCI->nCurBox < pCI->nCurMix)
    {
        pCI->nCurDsp = pCI->nCurBox;
    }
    else
    {
        pCI->nCurMix = pCI->nCurBox;
    }

    if (!(pCC->Flags & CC_SHOWHELP))
    {
        HWND hHelp;

        EnableWindow(hHelp = GetDlgItem(hDlg, psh15), FALSE);
        ShowWindow(hHelp, SW_HIDE);
    }

    if (pCC->lpfnHook)
    {
#ifdef UNICODE
        if (pCI->ApiType == COMDLG_ANSI)
        {
            ThunkChooseColorW2A(pCI);
            bRet = ((* pCC->lpfnHook)( hDlg,
                                        WM_INITDIALOG,
                                        wParam,
                                        (LPARAM)pCI->pCCA ));

            //
            //  Strange win 31 example uses lCustData to hold a temporary
            //  variable that it passes back to calling function.
            //
            ThunkChooseColorA2W(pCI);
            pCC->lCustData = pCI->pCCA->lCustData;
        }
        else
#endif
        {
            bRet = ((* pCC->lpfnHook)( hDlg,
                                        WM_INITDIALOG,
                                        wParam,
                                        (LPARAM)pCC ));
        }
    }
    else
    {
        bRet = TRUE;
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  ColorPaint
//
////////////////////////////////////////////////////////////////////////////

VOID ColorPaint(
    HWND hDlg,
    PCOLORINFO pCI,
    HDC hDC,
    LPRECT lpPaintRect)
{
    SHORT i;
    HWND hFocus;

    for (i = 0; i < nDriverColors; i++)
    {
        PaintBox(pCI, hDC, i);
    }
    for (i = NUM_BASIC_COLORS; i < COLORBOXES; i++)
    {
        PaintBox(pCI, hDC, i);
    }

    //
    //  Must redraw focus as well as paint boxes.
    //
    hFocus = GetFocus();
    if (hFocus == GetDlgItem(hDlg, COLOR_BOX1))
    {
        i = pCI->nCurDsp;
    }
    else if (hFocus == GetDlgItem(hDlg, COLOR_CUSTOM1))
    {
        i = pCI->nCurMix;
    }
    else
    {
        goto NoDrawFocus;
    }
    ChangeBoxFocus(pCI, i);

NoDrawFocus:
    RainbowPaint(pCI, hDC, lpPaintRect);
}


////////////////////////////////////////////////////////////////////////////
//
//  WantArrows
//
////////////////////////////////////////////////////////////////////////////

LONG WINAPI WantArrows(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    PCOLORINFO pCI;
    RECT rcTemp;
    HDC hDC;
    WORD temp;

    switch (msg)
    {
        case ( WM_GETDLGCODE ) :
        {
            return (DLGC_WANTARROWS | DLGC_WANTCHARS);
            break;
        }
        case WM_KEYDOWN:
        case WM_CHAR:
        {
            return (SendMessage(GetParent(hWnd), msg, wParam, lParam));
            break;
        }
        case ( WM_SETFOCUS ) :
        case ( WM_KILLFOCUS ) :
        {
            if (pCI = (PCOLORINFO) GetProp(GetParent(hWnd), COLORPROP))
            {
                if (GetWindowLong(hWnd, GWL_ID) == COLOR_BOX1)
                {
                    temp = pCI->nCurDsp;
                }
                else
                {
                    temp = pCI->nCurMix;
                }

                hDC = GetDC(GetParent(hWnd));
                CopyRect((LPRECT)&rcTemp, (LPRECT)rColorBox + temp);
                InflateRect((LPRECT)&rcTemp, 3, 3);
#ifndef NTWIN
                DrawFocusRect(hDC, (LPRECT)&rcTemp);
#endif
                ReleaseDC(GetParent(hWnd), hDC);
                break;
            }

            //  Fall thru...
        }
        default :
        {
            return (CallWindowProc(lpprocStatic, hWnd, msg, wParam, lParam));
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  TermColor
//
////////////////////////////////////////////////////////////////////////////

VOID TermColor()
{
    if (hRainbowBitmap)
    {
        DeleteObject(hRainbowBitmap);
        hRainbowBitmap = NULL;
    }
    if (hDCFastBlt)
    {
        DeleteDC(hDCFastBlt);
        hDCFastBlt = 0;
    }
}






/*========================================================================*/
/*                 Ansi->Unicode Thunk routines                           */
/*========================================================================*/

#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ThunkChooseColorA2W
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkChooseColorA2W(
    PCOLORINFO pCI)
{
    LPCHOOSECOLORW pCCW = pCI->pCC;
    LPCHOOSECOLORA pCCA = pCI->pCCA;

    pCCW->lCustData = pCCA->lCustData;
    pCCW->Flags = pCCA->Flags;

    pCCW->hInstance = pCCA->hInstance;

    pCCW->lpfnHook = pCCA->lpfnHook;

    //
    //  CC_RGBINIT conditional = time it takes to do it => just do it.
    //
    pCCW->rgbResult = pCCA->rgbResult;

    pCCW->lpCustColors = pCCA->lpCustColors;
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkChooseColorW2A
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkChooseColorW2A(
    PCOLORINFO pCI)
{
    LPCHOOSECOLORW pCCW = pCI->pCC;
    LPCHOOSECOLORA pCCA = pCI->pCCA;

    //
    //  Supposedly invariant, but not necessarily.
    //
    pCCA->Flags = pCCW->Flags;
    pCCA->lCustData = pCCW->lCustData;

    pCCA->lpfnHook = pCCW->lpfnHook;

    pCCA->rgbResult = pCCW->rgbResult;
    pCCA->lpCustColors = pCCW->lpCustColors;
}

#endif
