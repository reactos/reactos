/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    dlgs.c

Abstract:

    This module contains the common functions for the Win32 common dialogs.

Revision History:

--*/



//
//  Include Files.
//

#include "windows.h"
#include <port1632.h>
#include "privcomd.h"




//
//  Global Variables.
//

extern BOOL bInitializing;

extern DWORD g_tlsiExtError;
extern CRITICAL_SECTION g_csExtError;




//
//  Function Prototypes.
//

LONG
RgbInvertRgb(
    LONG rgbOld);





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
//  StoreExtendedError
//
//  Stores an extended error code for the next call to
//  CommDlgExtendedError.
//
////////////////////////////////////////////////////////////////////////////

void StoreExtendedError(
    DWORD dwError)
{
    EnterCriticalSection(&g_csExtError);

    TlsSetValue(g_tlsiExtError, (LPVOID)dwError);

    LeaveCriticalSection(&g_csExtError);
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

    EnterCriticalSection(&g_csExtError);

    dwError = (DWORD)TlsGetValue(g_tlsiExtError);

    LeaveCriticalSection(&g_csExtError);

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
    INT id,
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
   (INT)(((DUs) * (INT)LOWORD((lDlgBaseUnits))) / 4)

#define yDUsToPels(DUs, lDlgBaseUnits) \
   (INT)(((DUs) * (int)HIWORD((lDlgBaseUnits))) / 8)

VOID AddNetButton(
    HWND hDlg,
    HANDLE hInstance,
    INT dyBottomMargin,
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
    POINT ptTopLeft, ptTopRight, ptCenter, ptBtmLeft, ptBtmRight;
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
                //  CANCEL button, then the last control should be the
                //  CANCEL button.
                //
                if ((hLastCtrl == GetDlgItem(hDlg, IDOK)) &&
                    (hCtrl = GetDlgItem(hDlg, IDCANCEL)))
                {
                    POINT ptTopLeftTmp;

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

    if (LoadString( g_hinst,
                    (bAddAccel ? iszNetworkButtonTextAccel : iszNetworkButtonText),
                    (LPTSTR)szNetwork,
                    MAX_PATH ))
    {
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
    BOOL bNetwork = FALSE;
    DWORD dwErr;
    HKEY hKey;
    DWORD dwcbBuffer = 0;

    dwErr = RegOpenKey(
             HKEY_LOCAL_MACHINE,
             TEXT("System\\CurrentControlSet\\Control\\NetworkProvider\\Order"),
             &hKey );

    if (!dwErr)
    {
        dwErr = RegQueryValueEx( hKey,
                                 TEXT("ProviderOrder"),
                                 NULL,
                                 NULL,
                                 NULL,
                                 &dwcbBuffer );

        if (ERROR_SUCCESS == dwErr && dwcbBuffer > 1)
        {
            bNetwork = TRUE;
        }

        RegCloseKey(hKey);
    }

    return (bNetwork);
}

