/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mousemov.c

Abstract:

    This module contains the routines for the Mouse Pointer Property Sheet
    page.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "util.h"
#include "rc.h"
#include "mousehlp.h"




//
//  Constant Declarations.
//

#ifdef WINNT                 // NT does not currently support Mouse Trails
  #define  NO_MOUSETRAILS  1
#endif

#define ACCELMIN   0
#define ACCELMAX   (ACCELMIN + 6)      // range of 7 settings
#define TRAILMIN   2
#define TRAILMAX   (TRAILMIN + 5)      // range of 8 settings

//
// From shell\inc\shsemip.h
//
#define Assert(f)

//
//  Typedef Declarations.
//

//
//  Struct for SPI_GETMOUSE.
//
typedef struct tag_GetMouse
{
    int Thresh1;
    int Thresh2;
    int Speed;
} GETMOUSE, *LPGETMOUSE;


//
//  Dialog Data.
//
typedef struct tag_MouseGenStr
{
    GETMOUSE  gmOrig;
    GETMOUSE  gmNew;

    short     nSpeed;
    short     nOrigSpeed;

    int       nSensitivity;
    int       nOrigSensitivity;

#ifndef NO_MOUSETRAILS       // Mouse Trails are not implemented on NT
    short     nTrailSize;
    short     nOrigTrailSize;

    HWND      hWndTrailScroll;
#endif

    BOOL      fOrigSnapTo;

    HWND      hWndSpeedScroll;
    HWND      hDlg;

} MOUSEPTRSTR, *PMOUSEPTRSTR, *LPMOUSEPTRSTR;




//
//  Context Help Ids.
//

const DWORD aMouseMoveHelpIds[] =
{
    IDC_GROUPBOX_1,         IDH_DLGMOUSE_POINTMO,
    IDC_GROUPBOX_2,         IDH_COMM_GROUPBOX,
    MOUSE_SPEEDBMP,         NO_HELP,
    MOUSE_SPEEDSCROLL,      IDH_DLGMOUSE_POINTMO,
    IDC_GROUPBOX_3,         IDH_DLGMOUSE_ACCELERATION,
    MOUSE_ACCELNONE,        IDH_DLGMOUSE_ACCELERATION,
    MOUSE_ACCELLOW,         IDH_DLGMOUSE_ACCELERATION,
    MOUSE_ACCELMEDIUM,      IDH_DLGMOUSE_ACCELERATION,
    MOUSE_ACCELHIGH,        IDH_DLGMOUSE_ACCELERATION,
    MOUSE_PTRTRAIL,         NO_HELP,
    MOUSE_TRAILS,           IDH_DLGMOUSE_SHOWTRAIL,
    MOUSE_TRAILSCROLLTXT1,  IDH_DLGMOUSE_TRAILLENGTH,
    MOUSE_TRAILSCROLLTXT2,  IDH_DLGMOUSE_TRAILLENGTH,
    MOUSE_TRAILSCROLL,      IDH_DLGMOUSE_TRAILLENGTH,
    MOUSE_PTRSNAPDEF,       NO_HELP,
    IDC_GROUPBOX_4,         IDH_DLGMOUSE_SNAPDEF,
    MOUSE_SNAPDEF,          IDH_DLGMOUSE_SNAPDEF,

    0, 0
};





////////////////////////////////////////////////////////////////////////////
//
//  DestroyMousePtrDlg
//
////////////////////////////////////////////////////////////////////////////

void DestroyMousePtrDlg(
    PMOUSEPTRSTR pMstr)
{
    HWND hDlg;

    Assert( pMstr )

    if( pMstr )
    {
        hDlg = pMstr->hDlg;

        LocalFree( (HGLOBAL)pMstr );

        SetWindowLongPtr( hDlg, DWLP_USER, 0 );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  EnableTrailScroll
//
////////////////////////////////////////////////////////////////////////////

#ifndef NO_MOUSETRAILS   // Mouse Trails are not implemented on NT
void EnableTrailScroll(
    HWND hDlg,
    BOOL val)
{
    EnableWindow(GetDlgItem(hDlg, MOUSE_TRAILSCROLL), val);
    EnableWindow(GetDlgItem(hDlg, MOUSE_TRAILSCROLLTXT1), val);
    EnableWindow(GetDlgItem(hDlg, MOUSE_TRAILSCROLLTXT2), val);
}
#endif


////////////////////////////////////////////////////////////////////////////
//
//  InitMousePtrDlg
//
////////////////////////////////////////////////////////////////////////////

BOOL InitMousePtrDlg(
    HWND hDlg)
{
    PMOUSEPTRSTR pMstr;
    BOOL fSnapTo;

    pMstr = (PMOUSEPTRSTR)LocalAlloc(LPTR, sizeof(MOUSEPTRSTR));

    if (pMstr == NULL)
    {
        return (TRUE);
    }

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pMstr);

    pMstr->hDlg = hDlg;

#ifndef NO_MOUSETRAILS   // Mouse trails are not implemented on NT
    //
    //  Enable or disable the Mouse Trails Checkbutton.
    //
    if (SystemParametersInfo(SPI_GETMOUSETRAILS, 0, &pMstr->nTrailSize, 0))
    {
        pMstr->nOrigTrailSize = pMstr->nTrailSize;

        EnableWindow(GetDlgItem(hDlg,MOUSE_TRAILS), TRUE);

        SendDlgItemMessage( hDlg,
                            MOUSE_TRAILSCROLL,
                            TBM_SETRANGE,
                            0,
                            MAKELONG(TRAILMIN, TRAILMAX) );

        CheckDlgButton(hDlg, MOUSE_TRAILS, (pMstr->nTrailSize > 1));

        if (pMstr->nTrailSize > 1)
        {
            SendDlgItemMessage( hDlg,
                                MOUSE_TRAILSCROLL,
                                TBM_SETPOS,
                                TRUE,
                                (LONG)pMstr->nTrailSize );
        }
        else
        {
            pMstr->nTrailSize = TRAILMAX;

            EnableTrailScroll(hDlg, FALSE);

            SendDlgItemMessage( hDlg,
                                MOUSE_TRAILSCROLL,
                                TBM_SETPOS,
                                TRUE,
                                (LONG)pMstr->nTrailSize );
        }
    }
    else
    {
        CheckDlgButton(hDlg, MOUSE_TRAILS, FALSE);

        EnableWindow(GetDlgItem(hDlg, MOUSE_TRAILS), FALSE);

        EnableTrailScroll(hDlg, FALSE);
    }
#endif

    //
    // Enable or disable the Snap To Default Checkbutton
    //
    if (SystemParametersInfo(SPI_GETSNAPTODEFBUTTON, 0, (PVOID)&fSnapTo, FALSE))
    {
        pMstr->fOrigSnapTo = fSnapTo;
    }
    CheckDlgButton(hDlg, MOUSE_SNAPDEF, fSnapTo);

    SystemParametersInfo(SPI_GETMOUSE, 0, &pMstr->gmNew, FALSE);

    SystemParametersInfo(SPI_GETMOUSESPEED, 0, &pMstr->nOrigSensitivity, FALSE);
    if ((pMstr->nOrigSensitivity < 1) || (pMstr->nOrigSensitivity > 20))
    {
        pMstr->nOrigSensitivity = 10;
    }
    pMstr->nSensitivity = pMstr->nOrigSensitivity;

    pMstr->gmOrig.Thresh1 = pMstr->gmNew.Thresh1;
    pMstr->gmOrig.Thresh2 = pMstr->gmNew.Thresh2;
    pMstr->gmOrig.Speed   = pMstr->gmNew.Speed;
    if (pMstr->gmOrig.Speed == 0)
    {
        CheckRadioButton(hDlg, MOUSE_ACCELNONE, MOUSE_ACCELHIGH, MOUSE_ACCELNONE);
    }
    else if (pMstr->gmOrig.Speed == 1)
    {
        CheckRadioButton(hDlg, MOUSE_ACCELNONE, MOUSE_ACCELHIGH, MOUSE_ACCELLOW);
    }
    else if ((pMstr->gmOrig.Speed == 2) && (pMstr->gmOrig.Thresh2 >= 9))
    {
        CheckRadioButton(hDlg, MOUSE_ACCELNONE, MOUSE_ACCELHIGH, MOUSE_ACCELMEDIUM);
    }
    else
    {
        CheckRadioButton(hDlg, MOUSE_ACCELNONE, MOUSE_ACCELHIGH, MOUSE_ACCELHIGH);
    }

#ifndef NO_MOUSETRAILS       // Mouse Trails are not implemented on NT
    pMstr->hWndTrailScroll = GetDlgItem(hDlg, MOUSE_TRAILSCROLL);
#endif
    pMstr->hWndSpeedScroll = GetDlgItem(hDlg, MOUSE_SPEEDSCROLL);

    //
    //  0 Acc               = 4
    //  1 Acc, 5 xThreshold = 5
    //  1 Acc, 4 xThreshold = 6
    //  1 Acc, 3 xThreshold = 7
    //  1 Acc, 2 xThreshold = 8
    //  1 Acc, 1 xThreshold = 9
    //  2 Acc, 5 xThreshold = 10
    //  2 Acc, 4 xThreshold = 11
    //  2 Acc, 3 xThreshold = 12
    //  2 Acc, 2 xThreshold = 13
    //

    pMstr->nOrigSpeed = pMstr->nSpeed = ACCELMIN;

    if (pMstr->gmNew.Speed == 2)
    {
        pMstr->nSpeed += (24 - pMstr->gmNew.Thresh2) / 3;
    }
    else if (pMstr->gmNew.Speed == 1)
    {
        pMstr->nSpeed += (13 - pMstr->gmNew.Thresh1) / 3;
    }

    pMstr->nOrigSpeed = pMstr->nSpeed;

    SendDlgItemMessage( hDlg,
                        MOUSE_SPEEDSCROLL,
                        TBM_SETRANGE,
                        0,
                        MAKELONG(0, 10) );

    SendDlgItemMessage( hDlg,
                        MOUSE_SPEEDSCROLL,
                        TBM_SETPOS,
                        TRUE,
                        (LONG)pMstr->nOrigSensitivity / 2 );

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  TrailScroll
//
////////////////////////////////////////////////////////////////////////////

#ifndef NO_MOUSETRAILS
void TrailScroll(
    WPARAM wParam,
    LPARAM lParam,
    PMOUSEPTRSTR pMstr)
{
    pMstr->nTrailSize = (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0L);
    SystemParametersInfo(SPI_SETMOUSETRAILS, pMstr->nTrailSize, 0, 0);
}
#endif


////////////////////////////////////////////////////////////////////////////
//
//  SpeedScroll
//
////////////////////////////////////////////////////////////////////////////

void SpeedScroll(
    WPARAM wParam,
    LPARAM lParam,
    PMOUSEPTRSTR pMstr)
{
    pMstr->nSensitivity = (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0L) * 2;
    if (!pMstr->nSensitivity)
    {
        pMstr->nSensitivity = 1;
    }

    //
    //  Update speed when they let go of the thumb.
    //
    if (LOWORD(wParam) == SB_ENDSCROLL)
    {
        SystemParametersInfo( SPI_SETMOUSESPEED,
                              0,
                              (PVOID)pMstr->nSensitivity,
                              SPIF_SENDCHANGE );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseMovDlg
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK MouseMovDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PMOUSEPTRSTR pMstr;
    BOOL bRet;
    BOOL fSnapTo;

    pMstr = (PMOUSEPTRSTR)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            bRet = InitMousePtrDlg(hDlg);
            break;
        }
        case ( WM_DESTROY ) :
        {
            DestroyMousePtrDlg(pMstr);
            break;
        }
        case ( WM_HSCROLL ) :
        {
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);

            if ((HWND)lParam == pMstr->hWndSpeedScroll)
            {
                SpeedScroll(wParam, lParam, pMstr);
            }
#ifndef NO_MOUSETRAILS       // Mouse Trails are not implemented on NT
            else if ((HWND)lParam == pMstr->hWndTrailScroll)
            {
                TrailScroll(wParam, lParam, pMstr);
            }
#endif
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
#ifndef NO_MOUSETRAILS       // Mouse Trails are not implemented on NT
                case ( MOUSE_TRAILS ) :
                {
                    if (IsDlgButtonChecked(hDlg, MOUSE_TRAILS))
                    {
                        EnableTrailScroll(hDlg, TRUE);

                        pMstr->nTrailSize =
                            (int)SendMessage( pMstr->hWndTrailScroll,
                                              TBM_GETPOS,
                                              0,
                                              0 );

                        SystemParametersInfo( SPI_SETMOUSETRAILS,
                                              pMstr->nTrailSize,
                                              0,
                                              0 );
                    }
                    else
                    {
                        EnableTrailScroll(hDlg, FALSE);
                        SystemParametersInfo(SPI_SETMOUSETRAILS, 0, 0, 0);
                    }
                    SendMessage( GetParent(hDlg),
                                 PSM_CHANGED,
                                 (WPARAM)hDlg,
                                 0L );
                    break;
                }
#endif
                case ( MOUSE_SNAPDEF ) :
                {
                    SystemParametersInfo( SPI_SETSNAPTODEFBUTTON,
                                          IsDlgButtonChecked(hDlg, MOUSE_SNAPDEF),
                                          0,
                                          FALSE );
                    SendMessage( GetParent(hDlg),
                                 PSM_CHANGED,
                                 (WPARAM)hDlg,
                                 0L );
                    break;
                }
                case ( MOUSE_ACCELNONE ) :
                {
                    pMstr->gmNew.Speed   = 0;
                    pMstr->gmNew.Thresh1 = 0;
                    pMstr->gmNew.Thresh2 = 0;
                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                    SystemParametersInfo(SPI_SETMOUSE, 0, &pMstr->gmNew, FALSE);
                    break;
                }
                case ( MOUSE_ACCELLOW ) :
                {
                    pMstr->gmNew.Speed   = 1;
                    pMstr->gmNew.Thresh1 = 7;
                    pMstr->gmNew.Thresh2 = 0;
                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                    SystemParametersInfo(SPI_SETMOUSE, 0, &pMstr->gmNew, FALSE);
                    break;
                }
                case ( MOUSE_ACCELMEDIUM ) :
                {
                    pMstr->gmNew.Speed   = 2;
                    pMstr->gmNew.Thresh1 = 4;
                    pMstr->gmNew.Thresh2 = 12;
                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                    SystemParametersInfo(SPI_SETMOUSE, 0, &pMstr->gmNew, FALSE);
                    break;
                }
                case ( MOUSE_ACCELHIGH ) :
                {
                    pMstr->gmNew.Speed   = 2;
                    pMstr->gmNew.Thresh1 = 4;
                    pMstr->gmNew.Thresh2 = 6;
                    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                    SystemParametersInfo(SPI_SETMOUSE, 0, &pMstr->gmNew, FALSE);
                    break;
                }
            }
            break;
        }
        case ( WM_NOTIFY ) :
        {
            switch (((NMHDR *)lParam)->code)
            {
                case ( PSN_APPLY ) :
                {
                    //
                    //  Change cursor to hour glass.
                    //
                    HourGlass(TRUE);

#ifndef NO_MOUSETRAILS       // Mouse Trails are not implemented on NT.
                    //
                    //  Support mouse trails.
                    //
                    if (IsWindowEnabled(GetDlgItem(hDlg, MOUSE_TRAILS)))
                    {
                        if (IsDlgButtonChecked(hDlg, MOUSE_TRAILS))
                        {
                            SystemParametersInfo( SPI_SETMOUSETRAILS,
                                                  pMstr->nTrailSize,
                                                  0,
                                                  SPIF_UPDATEINIFILE |
                                                    SPIF_SENDCHANGE );
                        }
                        else
                        {
                            SystemParametersInfo( SPI_SETMOUSETRAILS,
                                                  0,
                                                  0,
                                                  SPIF_UPDATEINIFILE |
                                                    SPIF_SENDCHANGE );
                            pMstr->nTrailSize = 0;
                        }

                        //
                        //  New original once applied.
                        //
                        pMstr->nOrigTrailSize = pMstr->nTrailSize;
                    }
#endif
                    //
                    //  Support snap to default.
                    //
                    if (IsWindowEnabled(GetDlgItem(hDlg, MOUSE_SNAPDEF)))
                    {
                        fSnapTo = IsDlgButtonChecked(hDlg, MOUSE_SNAPDEF);

                        if (fSnapTo != pMstr->fOrigSnapTo)
                        {
                            SystemParametersInfo( SPI_SETSNAPTODEFBUTTON,
                                                  fSnapTo,
                                                  0,
                                                  SPIF_UPDATEINIFILE |
                                                    SPIF_SENDCHANGE );
                        }

                        //
                        //  New original once applied.
                        //
                        pMstr->fOrigSnapTo = fSnapTo;
                    }

                    //
                    //  Apply mouse speed.
                    //
                    SystemParametersInfo( SPI_SETMOUSESPEED,
                                          0,
                                          (PVOID)pMstr->nSensitivity,
                                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
                    pMstr->nOrigSensitivity = pMstr->nSensitivity;

                    //
                    //  Apply mouse acceleration.
                    //
                    SystemParametersInfo( SPI_SETMOUSE,
                                          0,
                                          &pMstr->gmNew,
                                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
                    pMstr->gmOrig = pMstr->gmNew;


                    HourGlass(FALSE);
                    break;
                }
                case ( PSN_RESET ) :
                {
#ifndef NO_MOUSETRAILS       // Mouse Trails are not implemented on NT
                    //
                    //  Support mouse trails.
                    //
                    if (IsWindowEnabled(GetDlgItem(hDlg, MOUSE_TRAILS)))
                    {
                        pMstr->nTrailSize = pMstr->nOrigTrailSize;

                        SystemParametersInfo( SPI_SETMOUSETRAILS,
                                              pMstr->nTrailSize,
                                              0,
                                              0 );
                    }
#endif
                    //
                    //  Support snap to default.
                    //
                    if (IsWindowEnabled(GetDlgItem(hDlg, MOUSE_SNAPDEF)))
                    {
                        CheckDlgButton(hDlg, MOUSE_SNAPDEF, pMstr->fOrigSnapTo);

                        SystemParametersInfo( SPI_SETSNAPTODEFBUTTON,
                                              pMstr->fOrigSnapTo,
                                              0,
                                              0 );
                    }

                    SystemParametersInfo( SPI_SETMOUSE,
                                          0,
                                          &pMstr->gmOrig,
                                          FALSE );

                    //
                    //  Restore the original mouse sensitivity.
                    //
                    SystemParametersInfo( SPI_SETMOUSESPEED,
                                          0,
                                          (PVOID)pMstr->nOrigSensitivity,
                                          FALSE );
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_HELP ) :             // F1
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     HELP_FILE,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aMouseMoveHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     HELP_FILE,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aMouseMoveHelpIds );
            break;
        }

        case ( WM_DISPLAYCHANGE ) :
        case ( WM_WININICHANGE ) :
        case ( WM_SYSCOLORCHANGE ) :
        {
            SHPropagateMessage(hDlg, message, wParam, lParam, TRUE);
            return TRUE;
        }

        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}
