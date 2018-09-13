/*******************************************************************
 *    DESCRIPTION: General Dialog handler
 *******************************************************************/

#include "Access.h"

extern INT_PTR WINAPI SerialKeyDlg(HWND, UINT, WPARAM, LPARAM);

// *******************************************************************
// GeneralDialog handler
// *******************************************************************
INT_PTR WINAPI GeneralDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   int i;
	SERIALKEYS serk;
   BOOL fProcessed = TRUE;
   char tempPort[10];   // Max 10 characters for a port name
   BOOL isAdmin = TRUE;

	switch (uMsg) {
		case WM_INITDIALOG:
			CheckDlgButton(hwnd, IDC_TO_ENABLE, (g_ato.dwFlags & ATF_TIMEOUTON) ? TRUE : FALSE);

			// Init the timeout combobox
			for (i= 0; i < 6; i++) {
				TCHAR szBuf[256];
				wsprintf(szBuf, __TEXT("%d"), ((i + 1) * 5));
				ComboBox_AddString(GetDlgItem(hwnd, IDC_TO_TIMEOUTVAL), szBuf);
			}
			ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_TO_TIMEOUTVAL), g_ato.iTimeOutMSec / (1000 * 60 * 5) - 1);
			if (!(g_ato.dwFlags & ATF_TIMEOUTON))
				EnableWindow(GetDlgItem(hwnd, IDC_TO_TIMEOUTVAL), FALSE);

         // Notification: Give wanring...         
			CheckDlgButton(hwnd, IDC_WARNING_SOUND, g_fShowWarnMsgOnFeatureActivate);				

         // Notification: Make a sound...
			CheckDlgButton(hwnd, IDC_SOUND_ONOFF, g_fPlaySndOnFeatureActivate);				

         // Support SerialKey devices
			CheckDlgButton(hwnd, IDC_SK_ENABLE, (g_serk.dwFlags & SERKF_SERIALKEYSON) ? TRUE : FALSE);
			if (!(g_serk.dwFlags & SERKF_AVAILABLE)) {
				EnableWindow(GetDlgItem(hwnd, IDC_SK_SETTINGS), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_SK_ENABLE), FALSE);
			}

         // JMR: What is this for?
         CheckDlgButton(hwnd, IDC_SAVE_SETTINGS, !g_fSaveSettings);

		 // Administrative options: 
		 // Enable/Disable all admin options.
		 isAdmin = IsDefaultWritable();
		 EnableWindow(GetDlgItem(hwnd, IDC_GEN_GROUP_4), isAdmin);
		 EnableWindow(GetDlgItem(hwnd, IDC_ADMIN_LOGON), isAdmin);
		 EnableWindow(GetDlgItem(hwnd, IDC_ADMIN_DEFAULT), isAdmin);
			break;

      case WM_HELP:
			WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
			break;

      case WM_CONTEXTMENU:
         WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
			break;

    	case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))  {
				case IDC_WARNING_SOUND:
					g_fShowWarnMsgOnFeatureActivate = !g_fShowWarnMsgOnFeatureActivate;
	    	      SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_SOUND_ONOFF:
					g_fPlaySndOnFeatureActivate = !g_fPlaySndOnFeatureActivate;
	    	      SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_SAVE_SETTINGS:
					g_fSaveSettings = !g_fSaveSettings;
	    	      SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_TO_ENABLE:
					g_ato.dwFlags ^= ATF_TIMEOUTON;
					if (!(g_ato.dwFlags & ATF_TIMEOUTON))
						EnableWindow(GetDlgItem(hwnd, IDC_TO_TIMEOUTVAL), FALSE);
					else
						EnableWindow(GetDlgItem(hwnd, IDC_TO_TIMEOUTVAL), TRUE);
	    	      SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_SK_ENABLE:
					g_serk.dwFlags ^= SERKF_SERIALKEYSON;
	    	      SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_SK_SETTINGS:

                    // HACK. Here the pointers are copied. So, When you change 
                    // in the dialog. temporary variable 'serk' looses its original value
                    // Save that in tempPort and use that instead a-anilk
					 serk = g_serk;
                     lstrcpy((LPTSTR)tempPort, g_serk.lpszActivePort);

					if (DialogBox(g_hinst, MAKEINTRESOURCE(IDD_SERKEYSETTING), hwnd, SerialKeyDlg) == IDCANCEL) 
                    {
                         g_serk = serk;
                         lstrcpy(g_serk.lpszActivePort, (LPCTSTR)tempPort);
                    }
					else SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_TO_TIMEOUTVAL:
					switch(HIWORD(wParam)) {
						case CBN_CLOSEUP:
							i = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_TO_TIMEOUTVAL));
							g_ato.iTimeOutMSec = (ULONG) ((long) ((i + 1) * 5) * 60 * 1000);
		               SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
							break;
					}					
					break;
				case IDC_ADMIN_LOGON:
					g_fCopyToLogon = !g_fCopyToLogon;
					SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_ADMIN_DEFAULT:
					g_fCopyToDefault = !g_fCopyToDefault;
					SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;
			}
			break;

        case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code) {
				case PSN_APPLY: SetAccessibilitySettings(); break;
			}
			break;

		default:
			fProcessed = FALSE;
			break;
	}
	return(fProcessed);
}

///////////////////////////////// End of File /////////////////////////////////
