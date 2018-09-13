/*******************************************************************
 *    DESCRIPTION: Sound Dialog handler
 *******************************************************************/

#include "Access.h"

// **************************************************************************
// SoundSentryDlg
// Process the SoundSentry  dialog.
// **************************************************************************

INT_PTR WINAPI SoundSentryDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int i;
   BOOL fProcessed = TRUE;
   TCHAR szBuf[256];

	switch (uMsg) {
		case WM_INITDIALOG:
			for (i= 0; i < 4; i++) {
				LoadString(g_hinst, IDS_WINDOWED + i, szBuf, ARRAY_SIZE(szBuf));
				ComboBox_AddString(GetDlgItem(hwnd, IDC_SS_WINDOWED), szBuf);				

            LoadString(g_hinst, IDS_TEXT + i, szBuf, ARRAY_SIZE(szBuf));
				ComboBox_AddString(GetDlgItem(hwnd, IDC_SS_TEXT), szBuf);
         }

			// Select the correct items from the comboboxes.
			ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SS_WINDOWED), g_ss.iWindowsEffect);				
			ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SS_TEXT), g_ss.iFSTextEffect);
         if (g_fWinNT) {
            ShowWindow(GetWindow(GetDlgItem(hwnd, IDC_SS_TEXT), GW_HWNDPREV), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_SS_TEXT), SW_HIDE);
         }
			break;

      case WM_HELP:	 // F1
			WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
			break;

      case WM_CONTEXTMENU:	// right mouse click
         WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
			break;

		case WM_COMMAND:
      	switch (GET_WM_COMMAND_ID(wParam, lParam)) {
				case IDC_SS_WINDOWED:
					switch (HIWORD(wParam)) {
						case CBN_CLOSEUP:
							g_ss.iWindowsEffect = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SS_WINDOWED));
							break;
					}					
					break;

				case IDC_SS_TEXT:
					switch (HIWORD(wParam)) {
						case CBN_SELENDOK:
							g_ss.iFSTextEffect = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SS_TEXT));				
							break;
					}
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
// SoundDialog handler
// *******************************************************************
INT_PTR CALLBACK SoundDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	SOUNDSENTRY ss;
   BOOL fProcessed = TRUE;
				
	switch (uMsg) {
		case WM_INITDIALOG:
			CheckDlgButton(hwnd, IDC_SS_ENABLE_SOUND, (g_ss.dwFlags & SSF_SOUNDSENTRYON) ? TRUE : FALSE);
			if (!(g_ss.dwFlags & SSF_AVAILABLE)) {
				EnableWindow(GetDlgItem(hwnd, IDC_SS_SETTINGS), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_SS_ENABLE_SOUND), FALSE);
			}
			CheckDlgButton(hwnd, IDC_SS_ENABLE_SHOW, g_fShowSounds);
			break;

      case WM_HELP:	 // F1
			WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
			break;

      case WM_CONTEXTMENU:	// right mouse click
         WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
			break;

    	case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))  {
				case IDC_SS_ENABLE_SOUND:
					g_ss.dwFlags ^= SSF_SOUNDSENTRYON;
               SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_SS_ENABLE_SHOW:
					g_fShowSounds = !g_fShowSounds;
               SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_SS_SETTINGS:
					ss = g_ss;
					if (DialogBox(g_hinst, MAKEINTRESOURCE(IDD_SOUNDSETTINGS), hwnd, SoundSentryDlg) == IDCANCEL) {
						g_ss = ss;
					} else SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;					
			}
			break;

		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code) {				
				case PSN_APPLY: SetAccessibilitySettings(); break;
					break;
			}
			break;

		default: fProcessed = FALSE; break;
	}
	return(fProcessed);
}


///////////////////////////////// End of File /////////////////////////////////
