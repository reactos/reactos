/*******************************************************************
 *
 *    DESCRIPTION: Mouse Dialog handler
 *
 *    HISTORY:    			 
 *
 *******************************************************************/

#include "Access.h"
#include <assert.h>

/*******************************************************************
 *
 *    DESCRIPTION: Mouse Keys Dialog handler
 *
 *    HISTORY:    			 
 *
 *******************************************************************/

#include "Access.h"

#define TICKCOUNT 9

INT_PTR WINAPI MouseKeyDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static UINT uSpeedTable[TICKCOUNT] = 
              { 10, 20, 30, 40, 60, 80, 120, 180, 360 };

    int  i;
    BOOL fProcessed = TRUE;

    switch (uMsg) {
    case WM_INITDIALOG:
        CheckDlgButton(hwnd, IDC_MK_HOTKEY, (g_mk.dwFlags & MKF_HOTKEYACTIVE) ? TRUE : FALSE);

        // Determine settings on our scroll bars for accel and top speed.

        for (i = 0;
            i < ARRAY_SIZE(uSpeedTable) && uSpeedTable[i] < g_mk.iMaxSpeed;
                i++)
        {
        }

        if (ARRAY_SIZE(uSpeedTable) <= i)
        {
                i = ARRAY_SIZE(uSpeedTable);
        }

        SendDlgItemMessage(
                hwnd,
                IDC_MK_TOPSPEED,
                TBM_SETRANGE,
                TRUE,
                MAKELONG(0, ARRAY_SIZE(uSpeedTable)-1));

        SendDlgItemMessage(
                hwnd, IDC_MK_TOPSPEED, TBM_SETPOS, TRUE, i);

        // Acceleration
        i = (TICKCOUNT+1) - g_mk.iTimeToMaxSpeed/500;
        if (i > TICKCOUNT-1)
        {
                i = TICKCOUNT-1;
        }
        if (i < 0)
        {
                i = 0;
        }

        SendDlgItemMessage(
                hwnd,
                IDC_MK_ACCEL,
                TBM_SETRANGE,
                TRUE,
                MAKELONG(0, TICKCOUNT-1));

        SendDlgItemMessage(
                hwnd,
                IDC_MK_ACCEL,
                TBM_SETPOS,
                TRUE,
                i);

        // Hold down Ctrl to speed up and Shift to slow down
        CheckDlgButton(hwnd, IDC_MK_USEMODKEYS, (g_mk.dwFlags & MKF_MODIFIERS) ? TRUE : FALSE);

        // Use MouseKeys when NumLock is on/off
        if (g_mk.dwFlags & MKF_REPLACENUMBERS)
            CheckRadioButton(hwnd, IDC_MK_NLOFF, IDC_MK_NLON, IDC_MK_NLON);
        else
            CheckRadioButton(hwnd, IDC_MK_NLOFF, IDC_MK_NLON, IDC_MK_NLOFF);

         // Show MouseKey status on screen
        CheckDlgButton(hwnd, IDC_MK_STATUS, (g_mk.dwFlags & MKF_INDICATOR) ? TRUE : FALSE);

       // 3/15/95 -
       // Always init the control speed to 1/8 of the screen width/
       g_mk.iCtrlSpeed = GetSystemMetrics(SM_CXSCREEN) / 16;
       break;

    case WM_HSCROLL:
    {
        int nScrollCode = (int) LOWORD(wParam); // scroll bar value
        int nPos = (short int) HIWORD(wParam);  // scroll box position
        HWND hwndScrollBar = (HWND) lParam;     // handle of scroll bar

        // Set the scrolls position
        i = HandleScroll(hwnd, wParam, hwndScrollBar);
        if (-1 != i)
        {
            // Update it.
            switch(GetWindowLong(hwndScrollBar, GWL_ID))
            {
            case IDC_MK_TOPSPEED:
                g_mk.iMaxSpeed = uSpeedTable[i];
                break;
            case IDC_MK_ACCEL:
                g_mk.iTimeToMaxSpeed = (TICKCOUNT+1-i) * 500;
                break;
            default:
                Assert(!"Got WM_HSCROLL from unknown control");
                break;
            }
        }
    }
        break;

    case WM_HELP:      // F1
                      WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
                      break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
        break;

              // Handle the generic commands
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDC_MK_HOTKEY:
                g_mk.dwFlags ^= MKF_HOTKEYACTIVE; break;

        case IDC_MK_STATUS:
                g_mk.dwFlags ^= MKF_INDICATOR; break;

        case IDC_MK_USEMODKEYS:
                g_mk.dwFlags ^= MKF_MODIFIERS; break;

        case IDC_MK_NLOFF:
                g_mk.dwFlags &= ~MKF_REPLACENUMBERS;
                CheckRadioButton(hwnd, IDC_MK_NLOFF, IDC_MK_NLON, IDC_MK_NLOFF);
                break;

        case IDC_MK_NLON:
                g_mk.dwFlags |= MKF_REPLACENUMBERS;
                CheckRadioButton(hwnd, IDC_MK_NLOFF,IDC_MK_NLON, IDC_MK_NLON);
                break;

        case IDOK:
                EndDialog(hwnd, IDOK);
                break;

        case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                break;

        }
        break;

        default: fProcessed = FALSE; break;
    }
    return(fProcessed);
}



// *******************************************************************
// Mouse Dialog handler
// *******************************************************************
INT_PTR WINAPI MouseDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MOUSEKEYS mk;
    BOOL fProcessed = TRUE;

    switch (uMsg) {
    case WM_INITDIALOG:
        CheckDlgButton(hwnd, IDC_MK_ENABLE, (g_mk.dwFlags & MKF_MOUSEKEYSON) ? TRUE : FALSE);
        if (!(g_mk.dwFlags & MKF_AVAILABLE)) {
            EnableWindow(GetDlgItem(hwnd, IDC_MK_SETTINGS), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_MK_ENABLE), FALSE);
        }
        break;

    case WM_HELP:      // F1
        WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
                      break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDC_MK_ENABLE:
            g_mk.dwFlags ^= MKF_MOUSEKEYSON;
            SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
                                    break;

        case IDC_MK_SETTINGS:
            mk = g_mk;  // Save settings before letting the user play with global
            if (DialogBox(g_hinst, MAKEINTRESOURCE(IDD_MOUSESETTINGS), hwnd, MouseKeyDlg) == IDCANCEL) {
              // User cancelled, restore settings.
                g_mk = mk;
            } else SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_APPLY: SetAccessibilitySettings(); break;
        }
        break;

    default: fProcessed = FALSE; break;
    }

    return(fProcessed);
}


///////////////////////////////// End of File /////////////////////////////////
