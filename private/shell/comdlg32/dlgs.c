/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    dlgs.c

Abstract:

    This module contains the common functions for the Win32 common dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include "comdlg32.h"




//
//  Global Variables.
//

extern BOOL bInitializing;

extern DWORD g_tlsiExtError;




//
//  Function Prototypes.
//

LONG
RgbInvertRgb(
    LONG rgbOld);

const struct _ERRORMAP
{
    DWORD dwCommDlgError;
    DWORD dwWin32Error;
} ERRORMAP[] =
{
    { CDERR_INITIALIZATION  , ERROR_INVALID_PARAMETER},  
    { PDERR_INITFAILURE     , ERROR_INVALID_PARAMETER},
    { CDERR_STRUCTSIZE      , ERROR_INVALID_PARAMETER},      
    { CDERR_NOTEMPLATE      , ERROR_INVALID_PARAMETER},      
    { CDERR_NOHINSTANCE     , ERROR_INVALID_PARAMETER},
    { CDERR_NOHOOK          , ERROR_INVALID_PARAMETER},
    { CDERR_MEMALLOCFAILURE , ERROR_OUTOFMEMORY},
    { CDERR_LOCKRESFAILURE  , ERROR_INVALID_HANDLE},
    { CDERR_DIALOGFAILURE   , E_FAIL},
    { PDERR_SETUPFAILURE    , ERROR_INVALID_PARAMETER},
    { PDERR_RETDEFFAILURE   , ERROR_INVALID_PARAMETER},
    { FNERR_BUFFERTOOSMALL  , ERROR_INSUFFICIENT_BUFFER},
    { FRERR_BUFFERLENGTHZERO, ERROR_INSUFFICIENT_BUFFER},
    { FNERR_INVALIDFILENAME , ERROR_INVALID_NAME},
    { PDERR_NODEFAULTPRN    , E_FAIL},
    { CFERR_NOFONTS         , E_FAIL},
    { CFERR_MAXLESSTHANMIN  , ERROR_INVALID_PARAMETER},
};

////////////////////////////////////////////////////////////////////////////
//
//  StoreExtendedError
//
//  Stores an extended error code for the next call to
//  CommDlgExtendedError.
//
////////////////////////////////////////////////////////////////////////////

void StoreExtendedError(
    DWORD dwError)
{ 
    int i;
    for (i=0; i < ARRAYSIZE(ERRORMAP); i++)
    {
        if (ERRORMAP[i].dwCommDlgError == dwError)
        {
            SetLastError(ERRORMAP[i].dwWin32Error);
            break;
        }
    }
    TlsSetValue(g_tlsiExtError, (LPVOID)dwError);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetStoredExtendedError
//
//  Retieves the stored extended error code.
//
////////////////////////////////////////////////////////////////////////////

DWORD GetStoredExtendedError(void)
{
    DWORD dwError;

    dwError = PtrToUlong(TlsGetValue(g_tlsiExtError));

    return (dwError);
}


////////////////////////////////////////////////////////////////////////////
//
//  CommDlgExtendedError
//
//  Provides additional information about dialog failure.
//  This should be called immediately after failure.
//
//  Returns:   LO word - error code
//             HI word - error specific info
//
////////////////////////////////////////////////////////////////////////////

DWORD WINAPI CommDlgExtendedError()
{
    return (GetStoredExtendedError());
}





////////////////////////////////////////////////////////////////////////////
//
//  HourGlass
//
//  Turn hourglass on or off.
//
//  bOn - specifies ON or OFF
//
////////////////////////////////////////////////////////////////////////////

VOID HourGlass(
    BOOL bOn)
{
    //
    //  Change cursor to hourglass.
    //
    if (!bInitializing)
    {
        if (!bMouse)
        {
            ShowCursor(bCursorLock = bOn);
        }
        SetCursor(LoadCursor(NULL, bOn ? IDC_WAIT : IDC_ARROW));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadAlterBitmap
//
//  Loads a bitmap given its name and gives all the pixels that are
//  a certain color a new color.
//
//  Returns:   NULL - failed
//             handle to the bitmap loaded - success
//
////////////////////////////////////////////////////////////////////////////

HBITMAP WINAPI LoadAlterBitmap(
    int id,
    DWORD rgbReplace,
    DWORD rgbInstead)
{
    LPBITMAPINFOHEADER qbihInfo;
    HDC hdcScreen;
    BOOL fFound;
    HANDLE hresLoad;
    HANDLE hres;
    LPLONG qlng;
    DWORD *qlngReplace;       // points to bits that are replaced
    LPBYTE qbBits;
    HANDLE hbmp;
    LPBITMAPINFOHEADER lpBitmapInfo;
    UINT cbBitmapSize;

    hresLoad = FindResource(g_hinst, MAKEINTRESOURCE(id), RT_BITMAP);
    if (hresLoad == HNULL)
    {
        return (HNULL);
    }
    hres = LoadResource(g_hinst, hresLoad);
    if (hres == HNULL)
    {
        return (HNULL);
    }

    //
    //  Lock the bitmap data and make a copy of it for the mask and the
    //  bitmap.
    //
    cbBitmapSize = SizeofResource(g_hinst, hresLoad);
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hres);

    qbihInfo = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, cbBitmapSize);

    if ((lpBitmapInfo == NULL) || (qbihInfo == NULL))
    {
        return (NULL);
    }

    memcpy((TCHAR *)qbihInfo, (TCHAR *)lpBitmapInfo, cbBitmapSize);

    //
    //  Get a pointer into the color table of the bitmaps, cache the
    //  number of bits per pixel.
    //
    rgbReplace = RgbInvertRgb(rgbReplace);
    rgbInstead = RgbInvertRgb(rgbInstead);

    qlng = (LPLONG)((LPSTR)(qbihInfo) + qbihInfo->biSize);

    fFound = FALSE;
    while (!fFound)
    {
        if (*qlng == (LONG)rgbReplace)
        {
            fFound = TRUE;
            qlngReplace = (DWORD *)qlng;
            *qlng = (LONG)rgbInstead;
        }
        qlng++;
    }

    //
    //  First skip over the header structure.
    //
    qbBits = (LPBYTE)(qbihInfo + 1);

    //
    //  Skip the color table entries, if any.
    //
    qbBits += (1 << (qbihInfo->biBitCount)) * sizeof(RGBQUAD);

    //
    //  Create a color bitmap compatible with the display device.
    //
    hdcScreen = GetDC(HNULL);
    if (hdcScreen != HNULL)
    {
        hbmp = CreateDIBitmap( hdcScreen,
                               qbihInfo,
                               (LONG)CBM_INIT,
                               qbBits,
                               (LPBITMAPINFO)qbihInfo,
                               DIB_RGB_COLORS );
        ReleaseDC(HNULL, hdcScreen);
    }

    //
    //  Reset color bits to original value.
    //
    *qlngReplace = (LONG)rgbReplace;

    LocalFree(qbihInfo);
    return (hbmp);
}


////////////////////////////////////////////////////////////////////////////
//
//  RgbInvertRgb
//
//  Reverses the byte order of the RGB value (for file format).
//
//  Returns the new color value (RGB to BGR).
//
////////////////////////////////////////////////////////////////////////////

LONG RgbInvertRgb(
    LONG rgbOld)
{
    LONG lRet;
    BYTE R, G, B;

    R = GetRValue(rgbOld);
    G = GetGValue(rgbOld);
    B = GetBValue(rgbOld);

    lRet = (LONG)RGB(B, G, R);

    return (lRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  HbmpLoadBmp
//
//  Loads in a bitmap.
//
//  Returns:   Bitmap handle - success
//             HNULL         - failure
//
////////////////////////////////////////////////////////////////////////////

#if 0
HBITMAP HbmpLoadBmp(
    WORD bmp)
{
    HBITMAP hbmp;
    CHAR szBitmap[cbResNameMax];

    hbmp = HNULL;
    if (LoadString(g_hinst, bmp, (LPTSTR)szBitmap, cbResNameMax - 1))
    {
        hbmp = LoadBitmap(g_hinst, (LPCTSTR)szBitmap);
    }
    return (hbmp);
}
#endif


////////////////////////////////////////////////////////////////////////////
//
//  AddNetButton
//
//  Attempts to add a network button to the open, save, or print dialogs.
//
//  hDlg           - dialog to add button to
//  hInstance      - instance handle for dlg
//  dyBottomMargin - DUs to bottom edge
//
////////////////////////////////////////////////////////////////////////////

#define xDUsToPels(DUs, lDlgBaseUnits) \
   (int)(((DUs) * (int)LOWORD((lDlgBaseUnits))) / 4)

#define yDUsToPels(DUs, lDlgBaseUnits) \
   (int)(((DUs) * (int)HIWORD((lDlgBaseUnits))) / 8)

VOID AddNetButton(
    HWND hDlg,
    HANDLE hInstance,
    int dyBottomMargin,
    BOOL bAddAccel,
    BOOL bTryLowerRight,
    BOOL bTryLowerLeft)
{
    LONG lDlgBaseUnits;
    RECT rcDlg, rcCtrl, rcTmp;
    LONG xButton, yButton;
    LONG dxButton, dyButton;
    LONG dxDlgFrame, dyDlgFrame;
    LONG yDlgHeight, xDlgWidth;
    HWND hwndButton, hCtrl, hLastCtrl, hTmp, hSave;
    HFONT hFont;
    POINT ptTopLeft, ptTopRight, ptCenter, ptBtmLeft, ptBtmRight, ptTopLeftTmp;
    TCHAR szNetwork[MAX_PATH];

    //
    //  Make sure a network button (psh14) doesn't already exist in
    //  the dialog.
    //
    if (GetDlgItem(hDlg, psh14))
    {
        return;
    }

    //
    //  Get dialog coordinate info.
    //
    lDlgBaseUnits = GetDialogBaseUnits();

    dxDlgFrame = GetSystemMetrics(SM_CXDLGFRAME);
    dyDlgFrame = GetSystemMetrics(SM_CYDLGFRAME);

    GetWindowRect(hDlg, &rcDlg);

    rcDlg.left += dxDlgFrame;
    rcDlg.right -= dxDlgFrame;
    rcDlg.top += dyDlgFrame + GetSystemMetrics(SM_CYCAPTION);
    rcDlg.bottom -= dyDlgFrame;

    //
    //  Get the OK button.
    //
    if (!(hCtrl = GetDlgItem(hDlg, IDOK)))
    {
        return;
    }

    GetWindowRect(hCtrl, &rcCtrl);

    ptTopLeft.x = rcCtrl.left;
    ptTopLeft.y = rcCtrl.top;

    //
    //  Make sure the OK button isn't outside the dialog.
    //
    if (!PtInRect(&rcDlg, ptTopLeft))
    {
        //
        //  Try the CANCEL button.
        //
        if (!(hCtrl = GetDlgItem(hDlg, IDCANCEL)))
        {
           //
           //  Both OK and CANCEL do not exist, so return.
           //
           return;
        }

        //
        //  The check for the Cancel button outside the dialog is handled
        //  below.
        //
        GetWindowRect(hCtrl, &rcCtrl);
    }
    hSave = hCtrl;

#ifdef UNICODE
    //
    //  Get the full hDlg value if coming from WOW.
    //
    if (IS_INTRESOURCE(hDlg))
    {
        HWND hNewDlg = GetParent(hCtrl);

        if (hDlg == (HWND)LOWORD(hNewDlg))
        {
            hDlg = hNewDlg;
        }
    }
#endif

    //
    //  Save the coordinate info of the button.
    //
    dxButton = rcCtrl.right - rcCtrl.left;
    dyButton = rcCtrl.bottom - rcCtrl.top;

    xButton = rcCtrl.left;
    yButton = rcCtrl.bottom + yDUsToPels(4, lDlgBaseUnits);

    yDlgHeight = rcDlg.bottom - yDUsToPels(dyBottomMargin, lDlgBaseUnits);

    //
    //  Try to insert the network button in the lower right corner
    //  of dialog box.
    //
    if (bTryLowerRight && (hTmp = GetDlgItem(hDlg, cmb2)))
    {
        //
        //  See if the network button can be inserted in the
        //  lower right corner of the dialog box.
        //
        hLastCtrl = hCtrl;
        GetWindowRect(hTmp, &rcTmp);
        yButton = rcTmp.top;

        //
        //  Set the coordinates of the new button.
        //
        ptTopLeft.x = ptBtmLeft.x = xButton;
        ptTopLeft.y = ptTopRight.y = yButton;
        ptTopRight.x = ptBtmRight.x = xButton + dxButton;
        ptBtmLeft.y = ptBtmRight.y = yButton + dyButton;
        ptCenter.x = xButton + dxButton / 2;
        ptCenter.y = yButton + dyButton / 2;
        ScreenToClient(hDlg, (LPPOINT)&ptTopLeft);
        ScreenToClient(hDlg, (LPPOINT)&ptBtmLeft);
        ScreenToClient(hDlg, (LPPOINT)&ptTopRight);
        ScreenToClient(hDlg, (LPPOINT)&ptBtmRight);
        ScreenToClient(hDlg, (LPPOINT)&ptCenter);

        //
        //  See if the new button is over any other buttons.
        //
        if (((yButton + dyButton) < yDlgHeight) &&
            ((ChildWindowFromPoint(hDlg, ptTopLeft)  == hDlg) &&
             (ChildWindowFromPoint(hDlg, ptTopRight) == hDlg) &&
             (ChildWindowFromPoint(hDlg, ptCenter)   == hDlg) &&
             (ChildWindowFromPoint(hDlg, ptBtmLeft)  == hDlg) &&
             (ChildWindowFromPoint(hDlg, ptBtmRight) == hDlg)))
        {
            //
            //  If the last control is the OK button and there is a
            //  HELP button, then the last control should be the
            //  HELP button.
            //
            if ((hLastCtrl == GetDlgItem(hDlg, IDOK)) &&
                (hCtrl = GetDlgItem(hDlg, pshHelp)))
            {
                GetWindowRect(hCtrl, &rcCtrl);
                ptTopLeftTmp.x = rcCtrl.left;
                ptTopLeftTmp.y = rcCtrl.top;

                //
                //  Make sure the HELP button isn't outside the dialog
                //  and then set the last control to be the HELP button.
                //
                if (PtInRect(&rcDlg, ptTopLeftTmp))
                {
                    hLastCtrl = hCtrl;
                }
            }

            //
            //  If the last control is still the OK button and there is a
            //  CANCEL button, then the last control should be the
            //  CANCEL button.
            //
            if ((hLastCtrl == GetDlgItem(hDlg, IDOK)) &&
                (hCtrl = GetDlgItem(hDlg, IDCANCEL)))
            {
                GetWindowRect(hCtrl, &rcCtrl);
                ptTopLeftTmp.x = rcCtrl.left;
                ptTopLeftTmp.y = rcCtrl.top;

                //
                //  Make sure the CANCEL button isn't outside the dialog
                //  and then set the last control to be the CANCEL button.
                //
                if (PtInRect(&rcDlg, ptTopLeftTmp))
                {
                    hLastCtrl = hCtrl;
                }
            }

            goto FoundPlace;
        }

        //
        //  Reset yButton.
        //
        yButton = rcCtrl.bottom + yDUsToPels(4, lDlgBaseUnits);
    }

    //
    //  Try to insert the network button vertically below the other
    //  control buttons.
    //
    while (hCtrl != NULL)
    {
        //
        //  Move vertically down and see if there is space.
        //
        hLastCtrl = hCtrl;
        GetWindowRect(hCtrl, &rcCtrl);
        yButton = rcCtrl.bottom + yDUsToPels(4, lDlgBaseUnits);

        //
        //  Make sure there is still room in the dialog.
        //
        if ((yButton + dyButton) > yDlgHeight)
        {
            //
            //  No space.
            //
            break;
        }

        //
        //  Set the coordinates of the new button.
        //
        ptTopLeft.x = ptBtmLeft.x = xButton;
        ptTopLeft.y = ptTopRight.y = yButton;
        ptTopRight.x = ptBtmRight.x = xButton + dxButton;
        ptBtmLeft.y = ptBtmRight.y = yButton + dyButton;
        ptCenter.x = xButton + dxButton / 2;
        ptCenter.y = yButton + dyButton / 2;
        ScreenToClient(hDlg, (LPPOINT)&ptTopLeft);
        ScreenToClient(hDlg, (LPPOINT)&ptBtmLeft);
        ScreenToClient(hDlg, (LPPOINT)&ptTopRight);
        ScreenToClient(hDlg, (LPPOINT)&ptBtmRight);
        ScreenToClient(hDlg, (LPPOINT)&ptCenter);

        //
        //  See if the new button is over any other buttons.
        //
        if (((hCtrl = ChildWindowFromPoint(hDlg, ptTopLeft))  == hDlg) &&
            ((hCtrl = ChildWindowFromPoint(hDlg, ptTopRight)) == hDlg) &&
            ((hCtrl = ChildWindowFromPoint(hDlg, ptCenter))   == hDlg) &&
            ((hCtrl = ChildWindowFromPoint(hDlg, ptBtmLeft))  == hDlg) &&
            ((hCtrl = ChildWindowFromPoint(hDlg, ptBtmRight)) == hDlg))
        {
            goto FoundPlace;
        }
    }

    //
    //  Try to insert the network button in the lower left corner of
    //  the dialog box.
    //
    if (bTryLowerLeft)
    {
        //
        //  Get the width of the dialog to make sure the button doesn't
        //  go off the side of the dialog.
        //
        xDlgWidth = rcDlg.right - xDUsToPels(FILE_RIGHT_MARGIN, lDlgBaseUnits);

        //
        //  Use the OK or CANCEL button saved earlier to get the size of
        //  the buttons.
        //
        hCtrl = hSave;

        //
        //  Start at the far left of the dialog.
        //
        //  NOTE: We know that hCtrl is not NULL at this point because
        //        otherwise we would have returned earlier.
        //
        //        The print dialogs have a left margin of 8.
        //
        GetWindowRect(hCtrl, &rcCtrl);
        xButton = rcDlg.left + xDUsToPels(FILE_LEFT_MARGIN + 3, lDlgBaseUnits);
        yButton = rcCtrl.top;

        while (1)
        {
            hLastCtrl = hCtrl;

            //
            //  Make sure there is still room in the dialog.
            //
            if ((xButton + dxButton) > xDlgWidth)
            {
                //
                //  No space.
                //
                break;
            }

            //
            //  Set the coordinates of the new button.
            //
            ptTopLeft.x = ptBtmLeft.x = xButton;
            ptTopLeft.y = ptTopRight.y = yButton;
            ptTopRight.x = ptBtmRight.x = xButton + dxButton;
            ptBtmLeft.y = ptBtmRight.y = yButton + dyButton;
            ptCenter.x = xButton + dxButton / 2;
            ptCenter.y = yButton + dyButton / 2;
            ScreenToClient(hDlg, (LPPOINT)&ptTopLeft);
            ScreenToClient(hDlg, (LPPOINT)&ptBtmLeft);
            ScreenToClient(hDlg, (LPPOINT)&ptTopRight);
            ScreenToClient(hDlg, (LPPOINT)&ptBtmRight);
            ScreenToClient(hDlg, (LPPOINT)&ptCenter);

            //
            //  See if the new button is over any other buttons.
            //
            if ( ( ((hCtrl = ChildWindowFromPoint(hDlg, ptTopLeft))  == hDlg) &&
                   ((hCtrl = ChildWindowFromPoint(hDlg, ptTopRight)) == hDlg) &&
                   ((hCtrl = ChildWindowFromPoint(hDlg, ptCenter))   == hDlg) &&
                   ((hCtrl = ChildWindowFromPoint(hDlg, ptBtmLeft))  == hDlg) &&
                   ((hCtrl = ChildWindowFromPoint(hDlg, ptBtmRight)) == hDlg) ) )
            {
                //
                //  If the last control is the OK button and there is a
                //  HELP button, then the last control should be the
                //  HELP button.
                //
                if ((hLastCtrl == GetDlgItem(hDlg, IDOK)) &&
                    (hCtrl = GetDlgItem(hDlg, pshHelp)))
                {
                    GetWindowRect(hCtrl, &rcCtrl);
                    ptTopLeftTmp.x = rcCtrl.left;
                    ptTopLeftTmp.y = rcCtrl.top;

                    //
                    //  Make sure the HELP button isn't outside the dialog
                    //  and then set the last control to be the HELP button.
                    //
                    if (PtInRect(&rcDlg, ptTopLeftTmp))
                    {
                        hLastCtrl = hCtrl;
                    }
                }

                //
                //  If the last control is still the OK button and there is a
                //  CANCEL button, then the last control should be the
                //  CANCEL button.
                //
                if ((hLastCtrl == GetDlgItem(hDlg, IDOK)) &&
                    (hCtrl = GetDlgItem(hDlg, IDCANCEL)))
                {
                    GetWindowRect(hCtrl, &rcCtrl);
                    ptTopLeftTmp.x = rcCtrl.left;
                    ptTopLeftTmp.y = rcCtrl.top;

                    //
                    //  Make sure the CANCEL button isn't outside the dialog
                    //  and then set the last control to be the CANCEL button.
                    //
                    if (PtInRect(&rcDlg, ptTopLeftTmp))
                    {
                        hLastCtrl = hCtrl;
                    }
                }

                goto FoundPlace;
            }

            //
            //  Make sure we encountered another control and that we
            //  didn't go off the end of the dialog.
            //
            if (!hCtrl)
            {
                break;
            }

            //
            //  Move over to the right and see if there is space.
            //
            GetWindowRect(hCtrl, &rcCtrl);
            xButton = rcCtrl.right + xDUsToPels(4, lDlgBaseUnits);
        }
    }

    return;

FoundPlace:

    xButton = ptTopLeft.x;
    yButton = ptTopLeft.y;

    //If it a mirrored Dlg then the direction will be to the right.
    if (IS_WINDOW_RTL_MIRRORED(hDlg))
        xButton -= dxButton;

#ifndef UNICODE
    //
    //  For non-localized apps that don't include the network button as part
    //  of their template, don't add a Far East one because the characters
    //  will not be displayed correctly.
    //
    {
        #define NetworkButtonText        TEXT("Network...")
        #define NetworkButtonTextAccel   TEXT("Net&work...")

        CPINFO cpinfo;

        if ((GetCPInfo(CP_ACP, &cpinfo)) && (cpinfo.MaxCharSize > 1))
        {
            TEXTMETRIC tm;
            HFONT hPrevFont;
            HWND hIDOK = GetDlgItem(hDlg, IDOK);
            HDC hDC = GetDC(hIDOK);

            hFont = (HFONT)SendMessage(hIDOK, WM_GETFONT, 0, 0L);
            if (hFont != NULL)
            {
                hPrevFont = SelectObject(hDC, hFont);
                GetTextMetrics(hDC, &tm);
                SelectObject(hDC, hPrevFont);
                ReleaseDC(hIDOK, hDC);

                if (tm.tmCharSet == ANSI_CHARSET)
                {
                    lstrcpy( szNetwork,
                             bAddAccel ? NetworkButtonTextAccel : NetworkButtonText );
                    goto CreateNetworkButton;
                }
            }
        }
    }
#endif

    if (CDLoadString( g_hinst,
                    (bAddAccel ? iszNetworkButtonTextAccel : iszNetworkButtonText),
                    (LPTSTR)szNetwork,
                    MAX_PATH ))
    {
#ifndef UNICODE
CreateNetworkButton:
#endif
        hwndButton = CreateWindow( TEXT("button"),
                                   szNetwork,
                                   WS_VISIBLE | WS_CHILD | WS_GROUP |
                                       WS_TABSTOP | BS_PUSHBUTTON,
                                   xButton,
                                   yButton,
                                   dxButton,
                                   dyButton,
                                   hDlg,
                                   NULL,
                                   hInstance,
                                   NULL );

        if (hwndButton != NULL)
        {
            SetWindowLong(hwndButton, GWL_ID, psh14);
            SetWindowPos( hwndButton,
                          hLastCtrl,
                          0, 0, 0, 0,
                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
            hFont = (HFONT)SendDlgItemMessage(hDlg, IDOK, WM_GETFONT, 0, 0L);
            SendMessage(hwndButton, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE,0));
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IsNetworkInstalled
//
////////////////////////////////////////////////////////////////////////////

BOOL IsNetworkInstalled()
{
    if (GetSystemMetrics(SM_NETWORK) & RNC_NETWORKS)
    {
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}



#ifdef WINNT

////////////////////////////////////////////////////////////////////////////
//
//  Ssync_ANSI_UNICODE_Struct_For_WOW (This is exported for WOW)
//
//  For WOW support on NT only.
//
//  When a 16-bit app calls one of the comdlg API's, WOW thunks the 16-bit
//  comdlg struct passed by the app to a 32-bit ANSI struct.  Comdlg32 code
//  then thunks the 32-bit ANSI struct into a UNICODE struct.  This scheme
//  creates a problem for WOW apps because on Win3.1, the app and comdlg16
//  share the same structure.  When either updates the struct, the other is
//  aware of the change.
//
//  This function allows us to sychronize the UNICODE struct with the app's
//  16-bit struct & vice versa from WOW.
//
////////////////////////////////////////////////////////////////////////////

VOID Ssync_ANSI_UNICODE_Struct_For_WOW(
    HWND hDlg,
    BOOL fDirection,
    DWORD dwID)
{
    switch (dwID)
    {
        case ( WOW_CHOOSECOLOR ) :
        {
            Ssync_ANSI_UNICODE_CC_For_WOW(hDlg, fDirection);
            break;
        }
        case ( WOW_CHOOSEFONT ) :
        {
            Ssync_ANSI_UNICODE_CF_For_WOW(hDlg, fDirection);
            break;
        }
        case ( WOW_OPENFILENAME ) :
        {
            Ssync_ANSI_UNICODE_OFN_For_WOW(hDlg, fDirection);
            break;
        }
        case ( WOW_PRINTDLG ) :
        {
            Ssync_ANSI_UNICODE_PD_For_WOW(hDlg, fDirection);
            break;
        }

        // case not needed for FINDREPLACE
    }
}

#endif


#ifdef WX86

////////////////////////////////////////////////////////////////////////////
//
//  Wx86GetX86Callback
//
//  Creates a RISC-callable alias for a x86 hook function pointer.
//
//  lpfnHook - x86 address of hook
//
//  Returns a function pointer which can be called from RISC.
//
////////////////////////////////////////////////////////////////////////////

PVOID Wx86GetX86Callback(
    PVOID lpfnHook)
{
    if (!lpfnHook)
    {
        return (NULL);
    }

    if (!pfnAllocCallBx86)
    {
        HMODULE hMod;

        if (!Wx86CurrentTib())
        {
            //
            //  Wx86 is not running in this thread.  Assume a RISC app has
            //  passed a bad flag value and that lpfnHook is already a RISC
            //  function pointer.
            //
            return (lpfnHook);
        }

        hMod = GetModuleHandle(TEXT("wx86.dll"));
        if (hMod == NULL)
        {
            //
            //  Wx86 is running, but wx86.dll is not loaded!  This should
            //  never happen, but if it does, assume lpfnHook is already a
            //  RISC pointer.
            //
            return (lpfnHook);
        }
        pfnAllocCallBx86 = (PALLOCCALLBX86)GetProcAddress( hMod,
                                                           "AllocCallBx86" );
        if (!pfnAllocCallBx86)
        {
            //
            //  Something has gone terribly wrong!
            //
            return (lpfnHook);
        }
    }

    //
    //  Call into Wx86.dll to create a RISC-to-x86 callback which takes
    //  4 parameters and has no logging.
    //
    return (*pfnAllocCallBx86)(lpfnHook, 4, NULL, NULL);
}

#endif

////////////////////////////////////////////////////////////////////////////
//
//  CDLoadString
//
////////////////////////////////////////////////////////////////////////////
int CDLoadString(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax)
{
    HRSRC   hResInfo;
    int     cch = 0;
    LPWSTR  lpwsz;
    LANGID  LangID;

    if (!GET_BIDI_LOCALIZED_SYSTEM_LANGID(NULL)) {
        return LoadString(hInstance, uID, lpBuffer, nBufferMax);
    }

    LangID = (LANGID)TlsGetValue(g_tlsLangID);

    if (!LangID || MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) == LangID) {
        return LoadString(hInstance, uID, lpBuffer, nBufferMax);
    }

    if (!lpBuffer || (nBufferMax-- == 0))
        return 0;

    // String Tables are broken up into 16 string resources.  Find the resource
    // containing the string we are interested in.
    if (hResInfo = FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE((uID>>4)+1), LangID)) {

        // Load the resource.  Note LoadResource returns an address.
        if (lpwsz = (LPWSTR)LoadResource(hInstance, hResInfo)) {
            // Move past the other strings in this resource.
            // (16 strings in a segment -> & 0x0F)
            for (uID %= 16; uID; uID--) {
                lpwsz += *lpwsz + 1;
            }
            cch = min(*lpwsz, nBufferMax - 1);
#ifdef UNICODE
            // Copy the string into the buffer;
            memcpy(lpBuffer, lpwsz+1, cch*sizeof(WCHAR));
#else 
            // Copy the string into the buffer, converting to Ansi.
            cch= WideCharToMultiByte(  CP_ACP, 0, lpwsz+1, cch, lpBuffer, cch, NULL, NULL);
#endif
        }
    }

    lpBuffer[cch] = 0;
    return cch;
}

#define ENGLISH_APP     0
#define MIRRORED_APP    1
#define BIDI_APP        2

DWORD GetAppType(HWND hWnd) {
    DWORD dwExStyle = 0;
    HWND  hWndT     = hWnd;
    DWORD dwAppType = ENGLISH_APP;

#ifdef CHECK_OWNER
    //Check the window and its owners.
    while (!dwExStyle && hWndT) {
       dwExStyle = GetWindowLongA(hWndT, GWL_EXSTYLE) & (WS_EX_RIGHT | WS_EX_RTLREADING | RTL_MIRRORED_WINDOW);
        hWndT = GetWindow(hWndT, GW_OWNER);
    }

    if (!dwExStyle) {
#endif
        //If we still did not find then check the parents.
        hWndT = hWnd;
        while (!dwExStyle && hWndT) {
            dwExStyle = GetWindowLongA(hWndT, GWL_EXSTYLE) & (WS_EX_RIGHT | WS_EX_RTLREADING | RTL_MIRRORED_WINDOW);
            hWndT = GetParent(hWndT);
        }
#ifdef CHECK_OWNER
    }
#endif

    if (dwExStyle & RTL_MIRRORED_WINDOW) {
       dwAppType = MIRRORED_APP;
    } else if (dwExStyle & (WS_EX_RIGHT | WS_EX_RTLREADING)) {
       dwAppType = BIDI_APP;
    }

    return dwAppType;
}

DWORD GetTemplateType(HANDLE hDlgTemplate)
{
    DWORD dwExStyle = 0;
    DWORD dwAppType = ENGLISH_APP;
    LPDLGTEMPLATE pDlgTemplate;

    pDlgTemplate = (LPDLGTEMPLATE)LockResource(hDlgTemplate);
    if (pDlgTemplate) {
        if (((LPDLGTEMPLATEEX) pDlgTemplate)->wSignature == 0xFFFF) {
            dwExStyle = ((LPDLGTEMPLATEEX) pDlgTemplate)->dwExStyle;
        } else {
            dwExStyle = pDlgTemplate->dwExtendedStyle;
        }
    }

    if (dwExStyle & RTL_MIRRORED_WINDOW) {
       dwAppType = MIRRORED_APP;
    } else if (dwExStyle & (WS_EX_RIGHT | WS_EX_RTLREADING)) {
       dwAppType = BIDI_APP;
    }

    return dwAppType;
}

LANGID GetDialogLanguage(HWND hwndOwner, HANDLE hDlgTemplate)
{
   LANGID LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
   DWORD  dwType;

   if (GET_BIDI_LOCALIZED_SYSTEM_LANGID(&LangID)) {
       if (hDlgTemplate == NULL) {
           dwType = GetAppType(hwndOwner);
       } else {
           dwType = GetTemplateType(hDlgTemplate);
       }

       switch (dwType) {
           case ENGLISH_APP :
               LangID =  MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
               break;

           case MIRRORED_APP:
               LangID =  MAKELANGID(PRIMARYLANGID(LangID), SUBLANG_DEFAULT);
               break;

           case BIDI_APP    :
               LangID =  MAKELANGID(PRIMARYLANGID(LangID), SUBLANG_SYS_DEFAULT);
               break;
       }
   }
   return LangID;
}


