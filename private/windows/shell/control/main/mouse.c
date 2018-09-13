/** FILE: mouse.c ********** Module Header ********************************
 *
 *  Control panel applet for Mouse configuration.  This file holds
 *  everything to do with the "Mouse" dialog box in the Control Panel.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"
#include "winuserp.h"
#undef MOUSETRAILS

//==========================================================================
//                            Local Definitions
//==========================================================================
#define THRESH1     0       /* 1st MouseThreshold */
#define THRESH2     1       /* 2nd MouseThreshold */
#define SPEED       2       /* MouseSpeed      */
#define ACCELMIN    0
#define ACCELMAX   (ACCELMIN + 6)   /* Range of 7 settings */


//==========================================================================
//                            External Declarations
//==========================================================================


//==========================================================================
//                            Local Data Declarations
//==========================================================================
TCHAR szDblClkSpeed[] = TEXT("DoubleClickSpeed");
TCHAR szSnapTo[] = TEXT("SnapToDefaultButton");

int    rgwMouse[3];
int    rgwOrigMse[3];

HWND hMouseDlg;

BOOL bSwap;
BOOL bOriginalSwap;

short nSpeed;
short nOrigSpeed;

short ClickSpeed;
short nOriginalDblClkSpeed;

BOOL bSnapTo;
BOOL bOriginalSnapTo;

WORD fInvert;

HWND hWndMLeftText;                // global variables are bad things!
HWND hWndMRightText;            // get rid of these if possible!
HWND hWndDblClkScroll;
RECT OrigRect;
RECT LeftMouseRect;
RECT RightMouseRect;
RECT DblClkRect;

//==========================================================================
//                            Local Function Prototypes
//==========================================================================
void InvertMButton (HDC hDC, WORD wButton);
BOOL InitMouseDlg (HWND hDlg);
void MousePaint (HDC hDC);

//==========================================================================
//                                Functions
//==========================================================================

BOOL InitMouseDlg (HWND hDlg)
{
    POINT Origin;

#ifdef MOUSETRAILS
    HDC hDC;
    int nTrails;
    int nTemp;
#endif  // MOUSETRAILS

    LoadAccelerators(hModule, (LPTSTR) MAKEINTRESOURCE(CP_ACCEL));

    hMouseDlg = hDlg;
    Origin.x = Origin.y = 0;
    ClientToScreen (hDlg, (LPPOINT) &Origin);

    hWndMLeftText = GetDlgItem (hDlg, MOUSE_LEFT);
    hWndMRightText = GetDlgItem (hDlg, MOUSE_RIGHT);
    GetWindowRect (hDlg, &OrigRect);

    OrigRect.left -= Origin.x;
    OrigRect.right -= Origin.x;

    GetWindowRect (GetDlgItem (hDlg, MOUSE_DBLCLKFRAME), &DblClkRect);
    DblClkRect.top -= Origin.y;
    DblClkRect.bottom -= Origin.y;
    DblClkRect.left -= Origin.x;
    DblClkRect.right -= Origin.x;
    GetWindowRect (GetDlgItem (hDlg, MOUSE_LBUTTON), &LeftMouseRect);
    LeftMouseRect.top -= Origin.y - 1;
    LeftMouseRect.bottom -= Origin.y + 1;
    LeftMouseRect.left -= Origin.x - 1;
    LeftMouseRect.right -= Origin.x + 1;
    GetWindowRect (GetDlgItem (hDlg, MOUSE_RBUTTON), &RightMouseRect);
    RightMouseRect.top -= Origin.y - 1;
    RightMouseRect.bottom -= Origin.y + 1;
    RightMouseRect.left -= Origin.x - 1;
    RightMouseRect.right -= Origin.x + 1;

    SwapMouseButton (bOriginalSwap = bSwap = SwapMouseButton (TRUE));
    if (bSwap)
    {
        TCHAR    szLeft[4], szRight[4];

        GetDlgItemText (hDlg, MOUSE_LEFT, szRight, CharSizeOf(szRight));
        GetDlgItemText (hDlg, MOUSE_RIGHT, szLeft, CharSizeOf(szLeft));
        SetDlgItemText (hDlg, MOUSE_LEFT, szLeft);
        SetDlgItemText (hDlg, MOUSE_RIGHT, szRight);
    }
    CheckDlgButton (hDlg, MOUSE_SWAP, (WORD) bSwap);

    SystemParametersInfo (SPI_GETSNAPTODEFBUTTON, 0, (PVOID)(LPBOOL)&bSnapTo, FALSE);
    bOriginalSnapTo = bSnapTo;
    CheckDlgButton (hDlg, MOUSE_SNAP, (WORD) bSnapTo);


#ifdef MOUSETRAILS
    if (!(hDC = GetDC (NULL)))
    {
        CheckDlgButton (hDlg, MOUSE_TRAILS, FALSE);
        EnableWindow (GetDlgItem (hDlg, MOUSE_TRAILS), FALSE);
    }
    else
    {
        nTemp = MOUSETRAILS;
        nTrails = Escape (hDC, QUERYESCSUPPORT, sizeof (int),
                          (LPSTR) &nTemp, NULL);
        EnableWindow (GetDlgItem (hDlg, MOUSE_TRAILS), nTrails);
        CheckDlgButton (hDlg, MOUSE_TRAILS, (nTrails > 1));
        ReleaseDC (NULL,hDC);
    }
#endif  // MOUSETRAILS

    fInvert = (WORD) ((GetAsyncKeyState (VK_LBUTTON) & 0x80) ? MK_LBUTTON : 0);
    fInvert |= (GetAsyncKeyState (VK_RBUTTON) & 0x80) ? MK_RBUTTON : 0;

    hWndDblClkScroll     = GetDlgItem (hDlg, MOUSE_CLICKSCROLL);
    ClickSpeed = (short) GetProfileInt ((LPTSTR)szWindows, (LPTSTR)szDblClkSpeed,
                                        CLICKMIN + CLICKRANGE / 2);
    nOriginalDblClkSpeed = ClickSpeed;

    SetScrollRange (hWndDblClkScroll, SB_CTL, CLICKMIN, CLICKMAX, FALSE);
    SetScrollPos (hWndDblClkScroll, SB_CTL, CLICKMIN + CLICKMAX - ClickSpeed, TRUE);
    SetDoubleClickTime (ClickSpeed);

    SystemParametersInfo (SPI_GETMOUSE, 0, (PVOID)(LPINT)rgwMouse, FALSE);

    rgwOrigMse[THRESH1] = rgwMouse[THRESH1];
    rgwOrigMse[THRESH2] = rgwMouse[THRESH2];
    rgwOrigMse[SPEED]   = rgwMouse[SPEED];

/*  0 Acc               = 4
    1 Acc, 5 xThreshold = 5
    1 Acc, 4 xThreshold = 6
    1 Acc, 3 xThreshold = 7
    1 Acc, 2 xThreshold = 8
    1 Acc, 1 xThreshold = 9
    2 Acc, 5 xThreshold = 10
    2 Acc, 4 xThreshold = 11
    2 Acc, 3 xThreshold = 12
    2 Acc, 2 xThreshold = 13
*/
    nOrigSpeed = nSpeed = ACCELMIN;
    if (rgwMouse[SPEED] == 2)
        nSpeed +=  (24 - rgwMouse[THRESH2]) / 3;
    else if (rgwMouse[SPEED] == 1)
        nSpeed +=  (13 - rgwMouse[THRESH1]) / 3;
    nOrigSpeed = nSpeed;

    SetScrollRange (GetDlgItem (hDlg, MOUSE_SPEEDSCROLL), SB_CTL, ACCELMIN, ACCELMAX, FALSE);
    SetScrollPos (GetDlgItem (hDlg, MOUSE_SPEEDSCROLL), SB_CTL, nSpeed, TRUE);

    return TRUE;
}


void InvertMButton (HDC hDC, WORD wButton)
{
    InvertRect (hDC,
        (LPRECT) ((wButton & MK_LBUTTON) ? &LeftMouseRect : &RightMouseRect));
    fInvert ^= wButton;
}


void MousePaint (HDC hDC)
{
    WORD wParam;

    UpdateWindow (hWndMLeftText);
    UpdateWindow (hWndMRightText);
    wParam = (WORD) ((GetKeyState (VK_LBUTTON) & 0x8000) ? MK_LBUTTON : 0);
    if ((fInvert ^ wParam) & MK_LBUTTON)
    {
        InvertMButton (hDC, MK_LBUTTON);
    }
    wParam = (WORD) ((GetKeyState (VK_RBUTTON) & 0x8000) ? MK_RBUTTON : 0);
    if ((fInvert ^ wParam) & MK_RBUTTON)
    {
        InvertMButton (hDC, MK_RBUTTON);
    }
    return;
}


BOOL MouseDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    PAINTSTRUCT ps;

    TCHAR szTemp[10];
    HDC  hDC;
    RECT Rect, rI;         /* Used in WM_PAINT, testing with IntersectRect */
    POINT pt;

#ifdef MOUSETRAILS
    int  nTrailSize;
    static BOOL bTrails;
#endif  // MOUSETRAILS

    switch (message)
    {
    case WM_INITDIALOG:
        InitMouseDlg (hDlg);
#ifdef MOUSETRAILS
        bTrails = IsDlgButtonChecked (hDlg, MOUSE_TRAILS);
#endif  // MOUSETRAILS
        break;

    case WM_HSCROLL:
        if ((HWND) lParam == hWndDblClkScroll)
        {
            switch (LOWORD(wParam))
            {
            case SB_LINEUP:
                ClickSpeed += CLICKRANGE / 50;
                break;

            case SB_LINEDOWN:
                ClickSpeed -= CLICKRANGE / 50;
                break;

            case SB_PAGEUP:
                ClickSpeed += CLICKRANGE / 5;
                break;

            case SB_PAGEDOWN:
                ClickSpeed -= CLICKRANGE / 5;
                break;

            case SB_THUMBPOSITION:
                ClickSpeed = (short) (CLICKMAX + CLICKMIN - HIWORD(wParam));
                break;

            case SB_TOP:
                ClickSpeed = CLICKMIN;
                break;

            case SB_BOTTOM:
                ClickSpeed = CLICKMAX;
                break;

            case SB_THUMBTRACK:
            case SB_ENDSCROLL:
                return TRUE;
                break;
            }
            if (ClickSpeed > CLICKMAX)
                ClickSpeed = CLICKMAX;
            else if (ClickSpeed < CLICKMIN)
                ClickSpeed = CLICKMIN;

            SetScrollPos ((HWND) lParam, SB_CTL, CLICKMAX + CLICKMIN - ClickSpeed, TRUE);
            SetDoubleClickTime (ClickSpeed);
        }
        else
        {
            switch (LOWORD(wParam))
            {
            case SB_LINEUP:
                nSpeed--;
                break;

            case SB_LINEDOWN:
                nSpeed++;
                break;

            case SB_PAGEUP:
                nSpeed -= 3;
                break;

            case SB_PAGEDOWN:
                nSpeed += 3;
                break;

            case SB_THUMBPOSITION:
                nSpeed = HIWORD(wParam);
                break;

            case SB_TOP:
                nSpeed = ACCELMAX;
                break;

            case SB_BOTTOM:
                nSpeed = ACCELMIN;
                break;

            case SB_THUMBTRACK:
            case SB_ENDSCROLL:
                return TRUE;
                break;
            }
            if (nSpeed > ACCELMAX)
                nSpeed = ACCELMAX;
            else if (nSpeed < ACCELMIN)
                nSpeed = ACCELMIN;

            SetScrollPos ((HWND) lParam, SB_CTL, nSpeed, TRUE);
            if (nSpeed == 0)
            {
                rgwMouse[THRESH1] = rgwMouse[THRESH2] = rgwMouse[SPEED] = 0;
            }
            else if (nSpeed < 4)
            {
                rgwMouse[SPEED] = 1;
                rgwMouse[THRESH1] = 13 - 3 * nSpeed;
                rgwMouse[THRESH2] = 0;
            }
            else
            {
                rgwMouse[SPEED] = 2;
                rgwMouse[THRESH1] = 4;
                rgwMouse[THRESH2] = 24 - 3 * nSpeed;
            }
            SystemParametersInfo (SPI_SETMOUSE, 0, (PVOID)(LPINT)rgwMouse, FALSE);
        }
        break;

    case WM_RBUTTONDBLCLK:
    case WM_LBUTTONDBLCLK:
        LONG2POINT (lParam, pt);
        if ( PtInRect (&DblClkRect, pt) )
        {
            hDC = GetDC (hDlg);
            InvertRect (hDC,  &DblClkRect);
            ReleaseDC (hDlg, hDC);
            ValidateRect (hDlg,  &DblClkRect);
        }
/* fall through */

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
        hDC = GetDC (hDlg);
        MousePaint (hDC);
        ReleaseDC (hDlg, hDC);
        ValidateRect (hDlg, &LeftMouseRect);
        ValidateRect (hDlg, &RightMouseRect);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
            goto DoHelp;

#ifdef MOUSETRAILS
        case MOUSE_TRAILS:

             if (hDC = GetDC (NULL))
             {
                if (IsDlgButtonChecked (hDlg, MOUSE_TRAILS))
                {
                   nTrailSize = -1;
                   Escape (hDC, MOUSETRAILS, sizeof (int),
                           (LPSTR) ((LPINT) &nTrailSize), NULL);
                }
                else
                {
                   nTrailSize = 0;
                   Escape (hDC, MOUSETRAILS, sizeof (int),
                           (LPSTR) ((LPINT) &nTrailSize), NULL);
                }
                ReleaseDC (NULL, hDC);
             }
             break;
#endif  // MOUSETRAILS

        case MOUSE_SNAP:
            bSnapTo = IsDlgButtonChecked (hDlg, MOUSE_SNAP);
            SystemParametersInfo (SPI_SETSNAPTODEFBUTTON, bSnapTo, 0, FALSE);
            break;

        case MOUSE_SWAP:
            CheckDlgButton (hDlg, MOUSE_SWAP, (WORD) (bSwap = !IsDlgButtonChecked (hDlg, MOUSE_SWAP)));
            SwapMouseButton (bSwap);

            {
                TCHAR    szLeft[4], szRight[4];

                GetDlgItemText (hDlg, MOUSE_LEFT, szRight, CharSizeOf(szRight));
                GetDlgItemText (hDlg, MOUSE_RIGHT, szLeft, CharSizeOf(szLeft));
                SetDlgItemText (hDlg, MOUSE_LEFT, szLeft);
                SetDlgItemText (hDlg, MOUSE_RIGHT, szRight);
            }
            InvalidateRect (hDlg, &LeftMouseRect, TRUE);
            InvalidateRect (hDlg, &RightMouseRect, TRUE);
            UpdateWindow (hDlg);
            fInvert = (WORD) ((GetAsyncKeyState (VK_LBUTTON) & 0x80) ? MK_LBUTTON : 0);
            fInvert |= (GetAsyncKeyState (VK_RBUTTON) & 0x80) ? MK_RBUTTON : 0;
#ifdef JAPAN
            //We need the default Dialog handling.
            //yutakas 1992.11.11
            return FALSE;
#endif
            break;

        case IDOK:
            /* change cursor to hourglass */
            HourGlass (TRUE);

            if (bSnapTo != bOriginalSnapTo)
                SystemParametersInfo (SPI_SETSNAPTODEFBUTTON, bSnapTo, 0, SPIF_UPDATEINIFILE);

            if (bSwap != bOriginalSwap)
                SystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, bSwap, 0, SPIF_UPDATEINIFILE);

#ifdef MOUSETRAILS
            /* Support mouse trails */

            if (IsWindowEnabled (GetDlgItem (hDlg, MOUSE_TRAILS)))
            {
                if (hDC = GetDC (NULL))
                {
                    if (IsDlgButtonChecked (hDlg, MOUSE_TRAILS))
                    {
                       nTrailSize = -1;
                       Escape (hDC, MOUSETRAILS, sizeof (int),
                               (LPSTR) ((LPINT) &nTrailSize), NULL);
                    }
                    else
                    {
                       nTrailSize = 0;
                       Escape (hDC, MOUSETRAILS, sizeof (int),
                               (LPSTR) ((LPINT) &nTrailSize), NULL);
                    }
                    ReleaseDC (NULL, hDC);
                }
            }
#endif  // MOUSETRAILS

            if (ClickSpeed != nOriginalDblClkSpeed)
            {
                MyItoa (ClickSpeed, szTemp, 10);
                WriteProfileString (szWindows, szDblClkSpeed, szTemp);
            }

             // this sends the win.ini change message

            SystemParametersInfo (SPI_SETMOUSE, 0, (PVOID)(LPINT)rgwMouse,
                    SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

            HourGlass (FALSE);

            EndDialog (hDlg, 0L);
            break;

        case IDCANCEL:
#ifdef MOUSETRAILS
            /* Support mouse trails */

            if (IsWindowEnabled (GetDlgItem (hDlg, MOUSE_TRAILS)))
            {
                if (hDC = GetDC (NULL))
                {
                    if (bTrails)
                    {
                        nTrailSize = -1;
                        Escape (hDC, MOUSETRAILS, sizeof (int),
                               (LPSTR) ((LPINT) &nTrailSize), NULL);
                    }
                    else
                    {
                        nTrailSize = 0;
                        Escape (hDC, MOUSETRAILS, sizeof (int),
                               (LPSTR) ((LPINT) &nTrailSize), NULL);
                    }
                    ReleaseDC (NULL, hDC);
                }
            }
#endif  // MOUSETRAILS

            SetDoubleClickTime (nOriginalDblClkSpeed);
            SwapMouseButton (bOriginalSwap);
            SystemParametersInfo (SPI_SETMOUSE, 0, (PVOID)(LPINT)rgwOrigMse, FALSE);
            SystemParametersInfo (SPI_SETSNAPTODEFBUTTON, bOriginalSnapTo, 0, FALSE);
            EndDialog (hDlg, 0L);
            break;
        }
        break;

    case WM_ACTIVATE:
    case WM_MOUSEACTIVATE:
    case WM_ACTIVATEAPP:
        InvalidateRect (hDlg, &LeftMouseRect, TRUE);
        InvalidateRect (hDlg, &RightMouseRect, TRUE);
        UpdateWindow (hDlg);
        fInvert = (WORD) ((GetKeyState (VK_LBUTTON) & 0x80) ? MK_LBUTTON : 0);
        fInvert |= (GetKeyState (VK_RBUTTON) & 0x80) ? MK_RBUTTON : 0;
        return FALSE;
        break;

    case WM_PAINT:
        GetUpdateRect (hDlg, &Rect, FALSE);
        if (IntersectRect (&rI, &Rect, &LeftMouseRect))
        {
            InvalidateRect (hDlg,  &LeftMouseRect, TRUE);
            fInvert &= !MK_LBUTTON;
        }
        if (IntersectRect (&rI, &Rect, &RightMouseRect))
        {
            InvalidateRect (hDlg,  &RightMouseRect, TRUE);
            fInvert &= !MK_RBUTTON;
        }
        if (IntersectRect (&rI, &Rect, &DblClkRect))
        {
            InvalidateRect (hDlg,  &DblClkRect, TRUE);
        }
        hDC = BeginPaint (hDlg, &ps);
        MousePaint (hDC);
        EndPaint (hDlg, &ps);
        break;

    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp (hDlg);
            return TRUE;
        }
        else
            return FALSE;
        break;
    }

    return (TRUE);
}
