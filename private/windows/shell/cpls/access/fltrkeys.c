// **************************************************************************
// Filterkeys dialogs
// Process the filterkeys dialogs
// **************************************************************************


#include "Access.h"

extern DWORD g_dwOrigFKFlags;
extern BOOL g_bFKOn;

#define SWAP(A, B)   ( A ^= B, B ^= A, A ^= B )

// Prototypes
INT_PTR WINAPI BKDlg (HWND, UINT, WPARAM, LPARAM);
INT_PTR WINAPI RKDlg (HWND, UINT, WPARAM, LPARAM);
BOOL WINAPI NotificationDlg (HWND, UINT, WPARAM, LPARAM);
BOOL SubclassFilterKeysTestBox (UINT uIdTestBox,HWND hDlg);
BOOL SubclassRepeatKeysTestBox (UINT uIdTestBox,HWND hDlg);

// All these are for subclassing, so that pressing TAB stops at the next 
// control after test areas. a-anilk
LRESULT CALLBACK SubclassWndProcFKPrev(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK SubclassWndProcFKNext(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK SubclassWndProcRKPrev(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK SubclassWndProcRKNext(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

// Times are in milliseconds
#define DELAYSIZE	5
UINT uDelayTable[] = { 300, 700, 1000, 1500, 2000 };

// Times are in milliseconds
#define RATESIZE 6
UINT uRateTable[] = { 300, 500, 700, 1000, 1500, 2000 };

// Times are in milliseconds
#define BOUNCESIZE 5
UINT uBounceTable[] = { 500, 700, 1000, 1500, 2000 };

// Times are in milliseconds
#define ACCEPTSIZE 7
UINT uAcceptTable[] = { 0, 300, 500, 700, 1000, 1400, 2000 };

// these are wndprocs for subclassed windows to ignore repeated tab keys in
// some situations.
WNDPROC g_WndProcFKPrev = NULL;
WNDPROC g_WndProcFKNext = NULL;
WNDPROC g_WndProcRKPrev = NULL;
WNDPROC g_WndProcRKNext = NULL;

// other definitions for the keyboard
// UP means key was up before this message, DOWN means key was down
// PRESS means the key is being pressed, RELEASE means key being released

#define KEY_UP      0
#define KEY_DOWN    1

#define KEY_PRESS   0
#define KEY_RELEASE 1

// Macros to look at the lParam of keyboard messages
//
#define SCAN_CODE(theParam)  (LOBYTE (HIWORD(theParam)))
#define EXTENDED(theParam)   ( (HIWORD (theParam) & 0x0100) > 0)
#define SYSKEY(theParam)     ( (HIWORD (theParam) & 0x2000) > 0)
#define MENUMODE(theParam)   ( (HIWORD (theParam) & 0x1000) > 0)
#define PREV_STATE(theParam) ( (HIWORD (theParam) & 0x4000) > 0)
#define TRAN_STATE(theParam) ( (HIWORD (theParam) & 0x8000) > 0)

#define MAKE(theParam)    (TRAN_STATE(theParam) == KEY_PRESS)
#define BREAK(theParam)   (TRAN_STATE(theParam) == KEY_RELEASE)
#define WASUP(theParam)   (PREV_STATE(theParam) == KEY_UP)
#define WASDOWN(theParam) (PREV_STATE(theParam) == KEY_DOWN)

#define FIRSTHIT(theParam) (WASUP(theParam) && MAKE(theParam))

// *************************************************************************
// Process the scrolling messages from our trackbars.
// GENERIC CODE - called for any TrackBar handler.
// Passed in the hwnd, wParam, hwndScroll
// 	we can do all handling and return the new trackbar value without
//    knowing what control it is.
// Returns -1 to mean don't do anything 
// *************************************************************************
int HandleScroll (HWND hwnd, WPARAM wParam, HWND hwndScroll) {
    int nCurSliderPos = (int) SendMessage(
            hwndScroll, TBM_GETPOS, 0, 0);
    int nMaxVal = (int) SendMessage(
                            hwndScroll, TBM_GETRANGEMAX, 0, 0);
    int nMinVal = (int) SendMessage(
                            hwndScroll, TBM_GETRANGEMIN, 0, 0);

    switch (LOWORD(wParam)) {
    case TB_LINEUP:
    case TB_LINEDOWN:
    case TB_THUMBTRACK:
    case TB_THUMBPOSITION:
    case SB_ENDSCROLL:
        break;

    case TB_PAGEUP:
        if (hwndScroll == GetDlgItem(hwnd, IDC_RK_DELAYRATE) ||
                     hwndScroll == GetDlgItem(hwnd, IDC_BK_BOUNCERATE))
            nCurSliderPos--;
        break;

    case TB_PAGEDOWN:
        if (hwndScroll == GetDlgItem(hwnd, IDC_RK_DELAYRATE) ||
                     hwndScroll == GetDlgItem(hwnd, IDC_BK_BOUNCERATE))
            nCurSliderPos++;
        break;

    case TB_BOTTOM:
        nCurSliderPos = nMaxVal;
        break;

    case TB_TOP:
        nCurSliderPos = nMinVal;
        break;
    }

    if (nCurSliderPos < nMinVal)
    {
        nCurSliderPos = nMinVal;
    }

    if (nCurSliderPos > nMaxVal)
    {
        nCurSliderPos = nMaxVal;
    }

   SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
   return(nCurSliderPos);
}

void TestFilterKeys (BOOL fTurnTestOn)
{
	if (fTurnTestOn) 
	{
		g_fk.dwFlags &= ~FKF_INDICATOR;
		g_fk.dwFlags |= FKF_FILTERKEYSON;
	} 
	else 
	{
		if (g_dwOrigFKFlags & FKF_FILTERKEYSON)
		{
			g_fk.dwFlags |= FKF_FILTERKEYSON;
		}
		else
		{
			g_fk.dwFlags &= ~FKF_FILTERKEYSON;
		}

		if (g_dwOrigFKFlags & FKF_INDICATOR)
		{
			g_fk.dwFlags |= FKF_INDICATOR;
		}
		else
		{
			g_fk.dwFlags &= ~FKF_INDICATOR;
		}
	}

	AccessSystemParametersInfo(SPI_SETFILTERKEYS, sizeof(g_fk), &g_fk, 0);
}


// ****************************************************************************
// Main filter keys dialog handler
// ****************************************************************************

INT_PTR WINAPI FilterKeyDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    FILTERKEYS fk;
    BOOL fProcessed = TRUE;

    switch (uMsg) {
    case WM_INITDIALOG:
        // Setup hotkey
        CheckDlgButton(hwnd, IDC_FK_HOTKEY, (g_fk.dwFlags & FKF_HOTKEYACTIVE) ? TRUE : FALSE);

        // Setup the radio buttons for SLOW vs BOUNCE keys
        if (0 != g_fk.iBounceMSec) {
            // Bounce keys enabeled
            CheckRadioButton(hwnd, IDC_FK_BOUNCE, IDC_FK_REPEAT, IDC_FK_BOUNCE);
            EnableWindow(GetDlgItem(hwnd, IDC_BK_SETTINGS), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_RK_SETTINGS), FALSE);
        }
        else
        {
            // Slow key enabled
            CheckRadioButton(hwnd, IDC_FK_BOUNCE, IDC_FK_REPEAT, IDC_FK_REPEAT);
            EnableWindow(GetDlgItem(hwnd, IDC_BK_SETTINGS), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_RK_SETTINGS), TRUE);
        }

        CheckDlgButton(hwnd, IDC_FK_SOUND, (g_fk.dwFlags & FKF_CLICKON) ? TRUE : FALSE);
        CheckDlgButton(hwnd, IDC_FK_STATUS, (g_fk.dwFlags & FKF_INDICATOR) ? TRUE : FALSE);
        // 
        // SteveDon 5/15/98
        // If the focus is in the TestBox and "Ignore Quick Keystrokes" is on,
        // you have to hold down tab to get out. But as soon as focus leaves,
        // Ignore Quick Keystrokes gets turned off and the tab keys ends up
        // autorepeating very quickly, which (usually) lands you back in the 
        // TestBox. 
        // Solution: ignore repeated tabs in this dialog.
        // Problem: keys don't go to the dialog, they go to the focused
        // control. So: we can try to ignore repeated tab keys for the controls
        // just after the test box and just before the test box, which means 
        // that we need to subclass those window procs.
        if (!SubclassFilterKeysTestBox (IDC_FK_TESTBOX,hwnd))
            return (FALSE);

        break;

    case WM_HELP:
        WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
        break;
         
    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
               break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDC_FK_HOTKEY:
            g_fk.dwFlags ^= FKF_HOTKEYACTIVE;
            break;

        case IDC_FK_REPEAT:
            g_fk.iBounceMSec = 0;

            if (g_fk.iDelayMSec == 0)
            {
               g_fk.iDelayMSec = g_nLastRepeatDelay;
               g_fk.iRepeatMSec = g_nLastRepeatRate;
               g_fk.iWaitMSec = g_nLastWait;
            }

            CheckRadioButton(hwnd, IDC_FK_REPEAT, IDC_FK_BOUNCE, IDC_FK_REPEAT);
            EnableWindow(GetDlgItem(hwnd, IDC_BK_SETTINGS), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_RK_SETTINGS), TRUE);
            break;

        case IDC_FK_BOUNCE:
            g_fk.iDelayMSec = 0;
            g_fk.iRepeatMSec = 0;
            g_fk.iWaitMSec = 0;

            if (g_fk.iBounceMSec == 0)
            {
                g_fk.iBounceMSec = g_dwLastBounceKeySetting;
            }

            CheckRadioButton(hwnd, IDC_FK_REPEAT, IDC_FK_BOUNCE, IDC_FK_BOUNCE);
            EnableWindow(GetDlgItem(hwnd, IDC_BK_SETTINGS), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_RK_SETTINGS), FALSE);
            break;

        // Settings dialogs
        case IDC_RK_SETTINGS:  // This is RepeatKeys
            fk = g_fk;
            if (DialogBox(g_hinst, MAKEINTRESOURCE(IDD_ADVCHARREPEAT), hwnd, RKDlg) == IDCANCEL) {
                    g_fk = fk;
            }
            break;

        case IDC_BK_SETTINGS:    // This is BounceKeys
            fk = g_fk;
            if (DialogBox(g_hinst, MAKEINTRESOURCE(IDD_ADVKEYBOUNCE), hwnd, BKDlg) == IDCANCEL) {
                    g_fk = fk;
            }
            break;

        case IDC_FK_SOUND:
            g_fk.dwFlags ^= FKF_CLICKON;
            break;

        case IDC_FK_STATUS:
            g_fk.dwFlags ^= FKF_INDICATOR;
            break;

        // The test edit box is a special control for us.  When we get the
        // focus we turn on the current filterkeys settings, when we
        // leave the text box, we turn them back to what they were.
        case IDC_FK_TESTBOX:
            switch (HIWORD(wParam)) {
            case EN_SETFOCUS:  TestFilterKeys(TRUE); break;
            case EN_KILLFOCUS: TestFilterKeys(FALSE); break;
            }
            break;

        case IDOK:
            if (g_dwLastBounceKeySetting == 0)
                g_dwLastBounceKeySetting = uBounceTable[0];
            EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        }
        break;

    default:
        fProcessed = FALSE; break;
    }
    return(fProcessed);
}


void PutNumInEdit (HWND hwndEdit, int nNum) 
{
   TCHAR szBuf[10], szBuf2[20];
   wsprintf(szBuf, __TEXT("%d.%d"), nNum / 1000, (nNum % 1000) / 100);
   GetNumberFormat(LOCALE_USER_DEFAULT, 0, szBuf, NULL, szBuf2, 20);
   SetWindowText(hwndEdit, szBuf2);
}


// **************************************************************************
// BKDlg
// Process the BounceKeys dialog.
// **************************************************************************
INT_PTR WINAPI BKDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    int     i;
    BOOL fProcessed = TRUE;
   
    switch (uMsg) {
    case WM_INITDIALOG:
        // Determine the bounce.
        SendDlgItemMessage(hwnd, IDC_BK_BOUNCERATE, TBM_SETRANGE, TRUE, MAKELONG(1, BOUNCESIZE));
        // Make sure its a valide value.
        if (g_dwLastBounceKeySetting == 0)
            g_dwLastBounceKeySetting = 500;

        if (g_fk.iBounceMSec == 0)
            g_fk.iBounceMSec = g_dwLastBounceKeySetting;

        // Find the current value in our table
        for (i = 0; i < BOUNCESIZE; i++) {
            if (uBounceTable[i] >= g_fk.iBounceMSec) break;
        }

        // If invalid value, make it valid.
        SendDlgItemMessage(hwnd, IDC_BK_BOUNCERATE, TBM_SETPOS, TRUE, i + 1);
        PutNumInEdit(GetDlgItem(hwnd, IDC_BK_TIME), uBounceTable[i]);
        break;

     // Handle the track bars.
    case WM_HSCROLL:
        i = HandleScroll(hwnd, wParam, (HWND)lParam);
        if (i == -1) return(TRUE);
        g_fk.iBounceMSec = uBounceTable[--i];
        PutNumInEdit(GetDlgItem(hwnd, IDC_BK_TIME), g_fk.iBounceMSec);
        break;

    case WM_HELP:      // F1
        WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        // The test edit box is a special control for us.  When we get the
        // focus we turn on the current filterkeys settings, when we
        // leave the text box, we turn them back to what they were.
        case IDC_BK_TESTBOX:
            switch (HIWORD(wParam)) {
            case EN_SETFOCUS:  TestFilterKeys(TRUE); break;
            case EN_KILLFOCUS: TestFilterKeys(FALSE); break;
            }
            break;

        case IDOK:
            // Save the last known valid setting.
            g_dwLastBounceKeySetting = g_fk.iBounceMSec;
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


// **************************************************************************
// RKDlg
// Process the RepeatKeys dialog.
// **************************************************************************

INT_PTR WINAPI RKDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    int     i;
    BOOL  fProcessed = TRUE;
    static s_fRepeating = TRUE;
    static DWORD s_nLastRepeatDelayOld;
    static DWORD s_nLastRepeatRateOld;
    static DWORD s_nLastWaitOld;

    switch(uMsg) {
    case WM_INITDIALOG:
        s_nLastRepeatDelayOld = g_nLastRepeatDelay;
        s_nLastRepeatRateOld = g_nLastRepeatRate;
        s_nLastWaitOld = g_nLastWait;

        s_fRepeating = (0 != g_fk.iDelayMSec);
        CheckRadioButton(hwnd, IDC_RK_NOREPEAT, IDC_RK_REPEAT,
             s_fRepeating ? IDC_RK_REPEAT : IDC_RK_NOREPEAT);

        if (!s_fRepeating) {
            // Set FilterKey values to LastRepeat values
            // so the sliders will still get initialized correctly
            g_fk.iDelayMSec = g_nLastRepeatDelay;
            g_fk.iRepeatMSec = g_nLastRepeatRate;
        }

        // Initialize the Acceptance slider to last valid state
        SendDlgItemMessage(hwnd, IDC_RK_ACCEPTRATE, TBM_SETRANGE, TRUE, MAKELONG(1, ACCEPTSIZE));
        for (i = 0; i < ACCEPTSIZE; i++) {
            if (uAcceptTable[i] >= g_fk.iWaitMSec) break;
        }

        SendDlgItemMessage(hwnd, IDC_RK_ACCEPTRATE, TBM_SETPOS, TRUE, i + 1);
        if (i < 0) i = 0;
        if (i >= ACCEPTSIZE) i = ACCEPTSIZE - 1;

        PutNumInEdit(GetDlgItem(hwnd, IDC_RK_WAITTIME), uAcceptTable[i]);
        g_fk.iWaitMSec = uAcceptTable[i];

        // Initialize the Delay slider
        SendDlgItemMessage(hwnd, IDC_RK_DELAYRATE, TBM_SETRANGE, TRUE, MAKELONG(1, DELAYSIZE));
        for (i = 0; i < DELAYSIZE; i++) {
            if (uDelayTable[i] >= g_fk.iDelayMSec) break;
        }

        SendDlgItemMessage(hwnd, IDC_RK_DELAYRATE, TBM_SETPOS, TRUE, i + 1);
        if (i < 0) i = 0;
        if (i >= DELAYSIZE) i = DELAYSIZE - 1;
        PutNumInEdit(GetDlgItem(hwnd, IDC_RK_DELAYTIME), uDelayTable[i]);
                g_fk.iDelayMSec = uDelayTable[i];

        // Initialize the Repeat Rate Slider  Note -1 is set via the checkbox.
        SendDlgItemMessage(hwnd, IDC_RK_REPEATRATE, TBM_SETRANGE, TRUE, MAKELONG(1, RATESIZE));
        for (i = 0; i < RATESIZE; i++) {
            if (uRateTable[i] >= g_fk.iRepeatMSec) break;
        }

        SendDlgItemMessage(hwnd, IDC_RK_REPEATRATE, TBM_SETPOS, TRUE, i + 1);
        if (i < 0) i = 0;
        if (i >= RATESIZE) i = RATESIZE -1;
        PutNumInEdit(GetDlgItem(hwnd, IDC_RK_REPEATTIME), uRateTable[i]);
        g_fk.iRepeatMSec = uRateTable[i];

        // Now cleanup from initialization. Disable controls
        // that usable... Swap back any params needed
        if (!s_fRepeating) {
            EnableWindow(GetDlgItem(hwnd, IDC_RK_REPEATRATE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_RK_DELAYRATE), FALSE);

            // If we're not repeating, now set the value to 0
            // which indicates max repeat rate.
            g_fk.iDelayMSec = 0;
            g_fk.iRepeatMSec = 0;
        }
        // 
        // SteveDon 5/15/98
        // If the focus is in the TestBox and "Ignore Quick Keystrokes" is on,
        // you have to hold down tab to get out. But as soon as focus leaves,
        // Ignore Quick Keystrokes gets turned off and the tab keys ends up
        // autorepeating very quickly, which (usually) lands you back in the 
        // TestBox. 
        // Solution: ignore repeated tabs in this dialog.
        // Problem: keys don't go to the dialog, they go to the focused
        // control. So: we can try to ignore repeated tab keys for the controls
        // just after the test box and just before the test box, which means 
        // that we need to subclass those window procs.
        if (!SubclassRepeatKeysTestBox (IDC_RK_TESTBOX,hwnd))
            return (FALSE);
        break;

    case WM_HSCROLL:
        switch (GetWindowLongPtr((HWND) lParam, GWL_ID)) {
        case IDC_RK_ACCEPTRATE:
            i = HandleScroll(hwnd, wParam, (HWND)lParam);
            if (i == -1) return TRUE;
            g_fk.iWaitMSec = uAcceptTable[--i];
            PutNumInEdit(GetDlgItem(hwnd, IDC_RK_WAITTIME), g_fk.iWaitMSec);
            break;

        case IDC_RK_DELAYRATE:
            i = HandleScroll(hwnd, wParam, (HWND)lParam);
            if (i == -1) return TRUE;
            g_fk.iDelayMSec = uDelayTable[--i];
            PutNumInEdit(GetDlgItem(hwnd, IDC_RK_DELAYTIME), g_fk.iDelayMSec);
            g_nLastRepeatDelay = g_fk.iDelayMSec;
            break;

        case IDC_RK_REPEATRATE:
            i = HandleScroll(hwnd, wParam, (HWND)lParam);
            if (i == -1) return TRUE;
            g_fk.iRepeatMSec = uRateTable[--i];
            PutNumInEdit(GetDlgItem(hwnd, IDC_RK_REPEATTIME), g_fk.iRepeatMSec);
            g_nLastRepeatRate = g_fk.iRepeatMSec;
            break;
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
        // Turn on repeat keys - We're disabling via CPL rather than any flags in the call
        case IDC_RK_REPEAT:
            if (!s_fRepeating) {
                g_fk.iDelayMSec = g_nLastRepeatDelay;
                g_fk.iRepeatMSec = g_nLastRepeatRate;
            }

            // Now that we have valid parameters, continue with setting the sliders.
            s_fRepeating = TRUE;
            CheckRadioButton(hwnd, IDC_RK_NOREPEAT, IDC_RK_REPEAT, IDC_RK_REPEAT);
            if (g_fk.iRepeatMSec == 0) {
                g_fk.iRepeatMSec = 300;
                SendDlgItemMessage(hwnd, IDC_RK_REPEATRATE, TBM_SETPOS, TRUE, 1);
                PutNumInEdit(GetDlgItem(hwnd, IDC_RK_REPEATTIME), g_fk.iRepeatMSec);
            }
            EnableWindow(GetDlgItem(hwnd, IDC_RK_REPEATRATE), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_RK_DELAYRATE), TRUE);
            break;

        // Turn OFF repeat keys
        case IDC_RK_NOREPEAT:
            s_fRepeating = FALSE;
            CheckRadioButton(hwnd, IDC_RK_NOREPEAT, IDC_RK_REPEAT, IDC_RK_NOREPEAT);
            g_fk.iDelayMSec = 0;
            g_fk.iRepeatMSec = 0;
            EnableWindow(GetDlgItem(hwnd, IDC_RK_DELAYRATE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_RK_REPEATRATE), FALSE);
            break;

        // Process the test box - turnon filterkeys while inside it.
        case IDC_RK_TESTBOX:
            switch (HIWORD(wParam)) {
            case EN_SETFOCUS:  TestFilterKeys(TRUE); break;
            case EN_KILLFOCUS: TestFilterKeys(FALSE); break;
            }
            break;

        case IDOK:
            // Save off repeating values to registry
            EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            g_nLastRepeatDelay = s_nLastRepeatDelayOld;
            g_nLastRepeatRate = s_nLastRepeatRateOld;
            g_nLastWait = s_nLastWaitOld;

            EndDialog(hwnd, IDCANCEL);
            break;
        }
        break;

    default:
        fProcessed = FALSE;
        break;
    }
    return(fProcessed);
}

// **************************************************************************
// SubclassFilterKeysTestBox
//
// This takes the dialog ID of an edit field, and then finds the controls
// near the edit field (the controls 2 windows before and 2 windows after the 
// edit control in the z-order). These are the nearest controls that get
// keyboard messages. It subclasses both of these controls
// so that they ignore any WM_KEYDOWN messages when the key being pressed is
// the tab key and the key is already down (i.e. this is a repeated message)
//
// **************************************************************************
BOOL SubclassFilterKeysTestBox (UINT uIdTestBox,HWND hDlg)
{
	HWND	hwndPrev,
			hwndNext,
			hwndTestBox;

    hwndTestBox = GetDlgItem (hDlg,uIdTestBox);
	// BE CAREFUL IF DIALOG CHANGES! Right now the 
	// Previous Previous window is the "S&ettings" push button,
	// and the Next Next is the "&Beep when keys pressed..."
	// checkbox. If the order changes, this code might have to change too.
	// Could make it more general where it searches for controls before
	// and after that can get keyboard focus.
    hwndPrev = GetNextDlgTabItem (hDlg,hwndTestBox,TRUE);
	g_WndProcFKPrev = (WNDPROC) GetWindowLongPtr (hwndPrev, GWLP_WNDPROC);
	SetWindowLongPtr (hwndPrev,GWLP_WNDPROC,(LPARAM)SubclassWndProcFKPrev);

    hwndNext = GetNextDlgTabItem (hDlg,hwndTestBox,FALSE);
	g_WndProcFKNext = (WNDPROC) GetWindowLongPtr (hwndNext, GWLP_WNDPROC);
	SetWindowLongPtr (hwndNext,GWLP_WNDPROC,(LPARAM)SubclassWndProcFKNext);
	
	return (TRUE);
}

// **************************************************************************
// SubclassWndProcFKPrev
//
//  This is the WndProc used to ignore repeated presses of the tab key for 
//  the first focusable control that precedes the test box.
//
// **************************************************************************
LRESULT CALLBACK SubclassWndProcFKPrev(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
		case WM_KEYDOWN:
			if ((int)wParam == VK_TAB)
			{
				if (WASDOWN (lParam))
				{
					return (0);
				}
				// if not a repeat, need to move the focus. For some reason,
				// just calling CallWindowProc won't do it for us.
				if (GetKeyState(VK_SHIFT) < 0)
				{
					SendMessage (GetParent(hwnd),WM_NEXTDLGCTL,1,0);
				}
				else
				{
					SendMessage (GetParent(hwnd),WM_NEXTDLGCTL,0,0);
				}
			}
			break;
		
		case WM_GETDLGCODE:
			return (DLGC_WANTTAB | CallWindowProc (g_WndProcFKPrev,hwnd,uMsg,wParam,lParam));
			break;
    }
    return (CallWindowProc(g_WndProcFKPrev,hwnd,uMsg,wParam,lParam));
}

// **************************************************************************
// SubclassWndProcFKNext
//
//  This is the WndProc used to ignore repeated presses of the tab key for 
//  the first focusable control that follows the test box.
//
// **************************************************************************
LRESULT CALLBACK SubclassWndProcFKNext(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
		case WM_KEYDOWN:
			if ((int)wParam == VK_TAB)
			{
				if (WASDOWN(lParam))
				{
					return (0);
				}
				// if not a repeat, need to move the focus. For some reason,
				// just calling CallWindowProc won't do it for us.
				if (GetKeyState(VK_SHIFT) < 0)
				{
					SendMessage (GetParent(hwnd),WM_NEXTDLGCTL,1,0);
				}
				else
				{
					SendMessage (GetParent(hwnd),WM_NEXTDLGCTL,0,0);
				}
			}
			break;
		
		case WM_GETDLGCODE:
			return (DLGC_WANTTAB | CallWindowProc (g_WndProcFKNext,hwnd,uMsg,wParam,lParam));
			break;
    }
    return (CallWindowProc(g_WndProcFKNext,hwnd,uMsg,wParam,lParam));
}

// **************************************************************************
// SubclassRepeatKeysTestBox
//
//  Same as SubclassFilterKeysTestBox, but keeps it's info in different
//  globals so that one doesn't overwrite the other.
//
// **************************************************************************
BOOL SubclassRepeatKeysTestBox (UINT uIdTestBox,HWND hDlg)
{
	HWND	hwndPrev,
			hwndNext,
			hwndTestBox;

    hwndTestBox = GetDlgItem (hDlg,uIdTestBox);
	// BE CAREFUL IF DIALOG CHANGES! Right now the 
	// Previous Previous window is the "S&ettings" push button,
	// and the Next Next is the "&Beep when keys pressed..."
	// checkbox. If the order changes, this code might have to change too.
	// Could make it more general where it searches for controls before
	// and after that can get keyboard focus.
    hwndPrev = GetNextDlgTabItem (hDlg,hwndTestBox,TRUE);
	g_WndProcRKPrev = (WNDPROC) GetWindowLongPtr (hwndPrev,GWLP_WNDPROC);
	SetWindowLongPtr (hwndPrev,GWLP_WNDPROC,(LPARAM)SubclassWndProcRKPrev);

    hwndNext = GetNextDlgTabItem (hDlg,hwndTestBox,FALSE);
	g_WndProcRKNext = (WNDPROC) GetWindowLongPtr (hwndNext,GWLP_WNDPROC);
	SetWindowLongPtr (hwndNext,GWLP_WNDPROC,(LPARAM)SubclassWndProcRKNext);
	
	return (TRUE);
}

// **************************************************************************
// SubclassWndProcRKPrev
//
// **************************************************************************
LRESULT CALLBACK SubclassWndProcRKPrev(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
		case WM_KEYDOWN:
			if ((int)wParam == VK_TAB)
			{
				if (WASDOWN (lParam))
				{
					return (0);
				}
				// if not a repeat, need to move the focus. For some reason,
				// just calling CallWindowProc won't do it for us.
				if (GetKeyState(VK_SHIFT) < 0)
				{
					SendMessage (GetParent(hwnd),WM_NEXTDLGCTL,1,0);
				}
				else
				{
					SendMessage (GetParent(hwnd),WM_NEXTDLGCTL,0,0);
				}
			}
			break;
		
		case WM_GETDLGCODE:
			return (DLGC_WANTTAB | CallWindowProc (g_WndProcRKPrev,hwnd,uMsg,wParam,lParam));
			break;
    }
    return (CallWindowProc(g_WndProcRKPrev,hwnd,uMsg,wParam,lParam));
}

// **************************************************************************
// SubclassWndProcRKNext
//
// **************************************************************************
LRESULT CALLBACK SubclassWndProcRKNext(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch (uMsg)
    {
		case WM_KEYDOWN:
			if ((int)wParam == VK_TAB)
			{
				if (WASDOWN(lParam))
				{
					return (0);
				}
				// if not a repeat, need to move the focus. For some reason,
				// just calling CallWindowProc won't do it for us.
				if (GetKeyState(VK_SHIFT) < 0)
				{
					SendMessage (GetParent(hwnd),WM_NEXTDLGCTL,1,0);
				}
				else
				{
					SendMessage (GetParent(hwnd),WM_NEXTDLGCTL,0,0);
				}
			}
			break;
		
		case WM_GETDLGCODE:
			return (DLGC_WANTTAB | CallWindowProc (g_WndProcRKNext,hwnd,uMsg,wParam,lParam));
			break;
    }
    return (CallWindowProc(g_WndProcRKNext,hwnd,uMsg,wParam,lParam));
}

///////////////////////////////// End of File /////////////////////////////////
