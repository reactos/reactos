/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    keybdspd.c

Abstract:

    This module contains the main routines for the Keyboard applet's
    Speed property page.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "rc.h"
#include "util.h"
#include <regstr.h>
#include <help.h>




//
//  Constant Declarations.
//

#define KSPEED_MIN      0
#define KSPEED_MAX      31
#define KSPEED_RANGE    (KSPEED_MAX - KSPEED_MIN + 1)

//
//  For keyboard delay control.
//
#define KDELAY_MIN      0
#define KDELAY_MAX      3
#define KDELAY_RANGE    (KDELAY_MAX - KDELAY_MIN + 1)

//
//  For control of the cursor blink rate.
//
#define BLINK           1000

#define CURSORMIN       200
#define CURSORMAX       1200
#define CURSORSUM       (CURSORMIN + CURSORMAX)
#define CURSORRANGE     (CURSORMAX - CURSORMIN)

static ARROWVSCROLL avsDelay =  { -1,
                                  1,
                                  -KDELAY_RANGE / 4,
                                  KDELAY_RANGE / 4,
                                  KDELAY_MAX,
                                  KDELAY_MIN
                                };
static ARROWVSCROLL avsSpeed  = { -1,
                                  1,
                                  -KSPEED_RANGE / 4,
                                  KSPEED_RANGE / 4,
                                  KSPEED_MAX,
                                  KSPEED_MIN
                                };
static ARROWVSCROLL avsCursor = { -1,
                                  1,
                                  -CURSORRANGE / 400,
                                  CURSORRANGE / 400,
                                  CURSORMAX / 100,
                                  CURSORMIN / 100,
                                  0,
                                  0
                                };




//
//  Context Help Ids.
//

static DWORD aKbdHelpIds[] =
{
    KDELAY_GROUP,        IDH_COMM_GROUPBOX,
    KBLINK_GROUP,        IDH_COMM_GROUPBOX,
    KDELAY_SCROLL,       IDH_DLGKEY_REPDEL,
    KSPEED_SCROLL,       IDH_DLGKEY_REPSPEED,
    KREPEAT_EDIT,        IDH_DLGKEY_REPTEST,
    KBLINK_EDIT,         IDH_DLGKEY_CURSOR_GRAPHIC,
    KCURSOR_BLINK,       IDH_DLGKEY_CURSBLNK,
    KCURSOR_SCROLL,      IDH_DLGKEY_CURSBLNK,

    0, 0
};




//
//  Global Variables.
//

//
//  BUGBUG - these should be moved into the KeyboardSpdStr structure
//
static UINT uOriginalDelay, uOriginalSpeed;
static UINT uBlinkTime;
static UINT uNewBlinkTime;
static BOOL bKbNeedsReset = FALSE;
static HWND hwndCursorScroll;
static RECT rCursor;




//
//  Typedef Declarations.
//

typedef struct tag_KeyboardSpdStr
{
    HWND hDlg;        // HWND hKeyboardSpdDlg;

} KEYBOARDSPDSTR, *PKEYBOARDSPDSTR;





////////////////////////////////////////////////////////////////////////////
//
//  KeyboardSpeedSupported
//
////////////////////////////////////////////////////////////////////////////

BOOL KeyboardSpeedSupported()
{
#ifdef WINNT
    //
    // BUGBUG  For Windows NT we assume that all keyboards can
    //         handle the SetSpeed - we might be able to do a
    //         better check in the future if KEYBOARD.DLL is available.
    //
    return (TRUE);
#else
    HANDLE hKeyboardModule = LoadLibrary16(TEXT("KEYBOARD"));
    BOOL bCanDorkWithTheSpeed = FALSE;

    if (hKeyboardModule)
    {
        if (GetProcAddress16(hKeyboardModule, TEXT("SetSpeed")))
        {
            bCanDorkWithTheSpeed = TRUE;
        }

        FreeLibrary16(hKeyboardModule);
    }

    return (bCanDorkWithTheSpeed);
#endif
}


////////////////////////////////////////////////////////////////////////////
//
//  SetDelayAndSpeed
//
////////////////////////////////////////////////////////////////////////////

void SetDelayAndSpeed(
    HWND hDlg,
    int nDelay,
    int nSpeed,
    UINT uFlags)
{
    if (nDelay < 0)
    {
        nDelay = (int)SendDlgItemMessage( hDlg,
                                          KDELAY_SCROLL,
                                          TBM_GETPOS,
                                          0,
                                          0L );
    }

    if (nSpeed < 0)
    {
        nSpeed = (int)SendDlgItemMessage( hDlg,
                                          KSPEED_SCROLL,
                                          TBM_GETPOS,
                                          0,
                                          0L );
    }

    //
    //  Only send the WININICHANGE once.
    //
    SystemParametersInfo( SPI_SETKEYBOARDSPEED,
                          nSpeed,
                          0,
                          uFlags & ~SPIF_SENDWININICHANGE );
    SystemParametersInfo( SPI_SETKEYBOARDDELAY,
                          KDELAY_MAX - nDelay + KDELAY_MIN,
                          0L,
                          uFlags );
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyKeyboardSpdDlg
//
////////////////////////////////////////////////////////////////////////////

void DestroyKeyboardSpdDlg(
    PKEYBOARDSPDSTR pKstr)
{
    HWND hDlg;

    if (pKstr)
    {
        hDlg = pKstr->hDlg;

        LocalFree((HGLOBAL)pKstr);

        SetWindowLongPtr(hDlg, DWLP_USER, 0);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetSpeedGlobals
//
//  Get Repeat Speed, Delay, and Blink Time.
//
////////////////////////////////////////////////////////////////////////////

VOID GetSpeedGlobals()
{
    SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &uOriginalSpeed, FALSE);
    SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &uOriginalDelay, FALSE);

    uOriginalDelay = KDELAY_MAX - uOriginalDelay + KDELAY_MIN;

    uBlinkTime = uNewBlinkTime = GetCaretBlinkTime();
}


////////////////////////////////////////////////////////////////////////////
//
//  InitKeyboardSpdDlg
//
////////////////////////////////////////////////////////////////////////////

BOOL InitKeyboardSpdDlg(
    HWND hDlg)
{
    HourGlass(TRUE);

    if (!KeyboardSpeedSupported())
    {
        MyMessageBox( hDlg,
                      IDS_KEYBD_NOSETSPEED,
                      IDS_KEYBD_TITLE,
                      MB_OK | MB_ICONINFORMATION );

        HourGlass(FALSE);
        return (FALSE);
    }

    //
    //  Get Repeat Speed, Delay, and Blink Time.
    //
    GetSpeedGlobals();

    TrackInit(GetDlgItem(hDlg, KSPEED_SCROLL), uOriginalSpeed, &avsSpeed);
    TrackInit(GetDlgItem(hDlg, KDELAY_SCROLL), uOriginalDelay, &avsDelay);
    TrackInit(GetDlgItem(hDlg, KCURSOR_SCROLL), (CURSORSUM - uBlinkTime) / 100, &avsCursor );

    hwndCursorScroll = GetDlgItem(hDlg, KCURSOR_SCROLL);

    GetWindowRect(GetDlgItem(hDlg, KCURSOR_BLINK), &rCursor);
    ScreenToClient(hDlg, (LPPOINT)&rCursor.left);
    ScreenToClient(hDlg, (LPPOINT)&rCursor.right);

    SetTimer(hDlg, BLINK, uBlinkTime, NULL);

    HourGlass(FALSE);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KeyboardSpdDlg
//
////////////////////////////////////////////////////////////////////////////

static const TCHAR c_szUserDesktopKey[] = REGSTR_PATH_DESKTOP;
static const TCHAR c_szCursorBlink[] = TEXT("CursorBlinkRate");

INT_PTR CALLBACK KeyboardSpdDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PKEYBOARDSPDSTR pKstr = (PKEYBOARDSPDSTR)GetWindowLongPtr(hDlg, DWLP_USER);
    static BOOL fBlink = FALSE;
    
    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            bKbNeedsReset = FALSE;
            return (InitKeyboardSpdDlg(hDlg));
            break;
        }
        case ( WM_DESTROY ) :
        {
            DestroyKeyboardSpdDlg(pKstr);
            break;
        }
        case ( WM_HSCROLL ) :
        {
            if ((HWND)lParam == hwndCursorScroll)
            {
                int nCurrent = (int)SendMessage( (HWND)lParam,
                                                 TBM_GETPOS,
                                                 0,
                                                 0L );

                uNewBlinkTime = CURSORSUM - (nCurrent * 100);
                SetCaretBlinkTime(uNewBlinkTime);
                KillTimer(hDlg, BLINK);
                SetTimer(hDlg, BLINK, CURSORSUM - (nCurrent * 100), NULL);
            }
            else
            {
                SetDelayAndSpeed(hDlg, -1, -1, 0);
                bKbNeedsReset = TRUE;
            }

            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;
        }
        case ( WM_TIMER ) :
        {
            if (wParam == BLINK)
            {
                HDC hDC = GetDC(hDlg);
                if (fBlink)
                {
                    HBRUSH hBrush = CreateSolidBrush((COLORREF)0);
                    FillRect(hDC, &rCursor, hBrush);
                    DeleteObject(hBrush);
                }
                else
                    InvalidateRect(hDlg, &rCursor, TRUE);
                fBlink = !fBlink;            
                ReleaseDC(hDlg,hDC);
            }
            break;
        }
        case ( WM_WININICHANGE ) :
        case ( WM_SYSCOLORCHANGE ) :
        case ( WM_DISPLAYCHANGE ) :
        {
            SHPropagateMessage(hDlg, message, wParam, lParam, TRUE);
            break;
        }

        case ( WM_HELP ) :             // F1
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aKbdHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aKbdHelpIds );
            break;
        }
        case ( WM_NOTIFY ) :
        {
            switch (((NMHDR *)lParam)->code)
            {
                case ( PSN_APPLY ) :
                {
                    HKEY hk;

                    HourGlass(TRUE);

                    if (RegCreateKey( HKEY_CURRENT_USER,
                                      c_szUserDesktopKey,
                                      &hk ) == ERROR_SUCCESS)
                    {
                        TCHAR buf[16];

                        wsprintf(buf, TEXT("%d"), uNewBlinkTime);
                        RegSetValueEx( hk,
                                       c_szCursorBlink,
                                       0,
                                       REG_SZ,
                                       (LPBYTE)buf,
                                       (DWORD)(lstrlen(buf) + 1) * sizeof(TCHAR) );

                        RegCloseKey(hk);
                    }

                    SetDelayAndSpeed( hDlg,
                                      -1,
                                      -1,
                                      SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE );
                    GetSpeedGlobals();
                    HourGlass(FALSE);

                    break;
                }
                case ( PSN_RESET ) :
                {
                    SetCaretBlinkTime(uBlinkTime);

                    if (bKbNeedsReset)
                    {
                        //
                        //  Restore the original keyboard speed.
                        //
                        SetDelayAndSpeed( hDlg,
                                          uOriginalDelay,
                                          uOriginalSpeed,
                                          0 );
                    }
                    break;
                }
            }
            break;
        }
        default :
        {
            return FALSE;
        }
    }

    return (TRUE);
}
