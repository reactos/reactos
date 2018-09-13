/*******************************************************************
 *
 *    DESCRIPTION: Serial Keys Dialog handler
 *
 *    HISTORY:    			
 *
 *******************************************************************/

#include "Access.h"

#define NUMPORTS 8
#define NUMRATES 6


INT_PTR WINAPI SerialKeyDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int  i;
	UINT uBaud;
   UINT uBaudRates[] = { 300, 1200, 2400, 4800, 9600, 19200 };
	TCHAR szBuf[256];
   BOOL fProcessed = TRUE;

	switch (uMsg) {
		case WM_INITDIALOG:
  			LoadString(g_hinst, IDS_COMPORT, szBuf, ARRAY_SIZE(szBuf));
			for (i=1; i <= 4; i++) {
				TCHAR szBuf2[256];

				// Make a correct port name and add it to the list box.
				wsprintf(szBuf2, __TEXT("%s%d"), szBuf, i);
				ComboBox_AddString(GetDlgItem(hwnd, IDC_SK_PORT), szBuf2);									
			}

			// Select the current com port.
			if (g_serk.lpszActivePort[0] != '\0') {
				int cport;

				// For now we assume that the format of the string is
				// com[digit].  So comport[3] = the com port number
				// Set all invalid ports to 'COM1'
				cport = g_serk.lpszActivePort[3] - '1';
				if (cport < 0) cport = 0;
				if (cport > 4) cport = 0;

				// Set the active port.
				ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SK_PORT), cport);				
			} else {
				// Else default to COM1.
				ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SK_PORT), 0);
				lstrcpy(g_serk.lpszActivePort, __TEXT("COM1"));
			}

			// Fill in the BAUD RATE options
			uBaud = 1;		// Default baud rate.

			for (i = 0; i < NUMRATES; i++) {
				TCHAR szBuf[256];
				wsprintf(szBuf, __TEXT("%d"), uBaudRates[i]);
				ComboBox_AddString(GetDlgItem(hwnd, IDC_SK_BAUD), szBuf);				
				if (g_serk.iBaudRate == uBaudRates[i]) uBaud = i;	
			}
			ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SK_BAUD), uBaud);
			break;

      case WM_HELP:	 // F1
			WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
			break;

      case WM_CONTEXTMENU:	// right mouse click
         WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
			break;

		case WM_COMMAND:
      	switch (GET_WM_COMMAND_ID(wParam, lParam)) {
				// Watch for combobox changes.
				case IDC_SK_BAUD:
					switch (HIWORD(wParam)) {
						case CBN_CLOSEUP:
						case CBN_SELCHANGE:
							i = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SK_BAUD));
                     g_serk.iBaudRate = uBaudRates[i];
		    	         SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
							break;
					}					
					break;

				case IDC_SK_PORT:
					switch (HIWORD(wParam)) {
						case CBN_SELCHANGE:
							i = 1 + ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SK_PORT));
                     wsprintf(g_serk.lpszActivePort, __TEXT("COM%d"), i);
                     EnableWindow(GetDlgItem(hwnd, IDC_SK_BAUD), TRUE);
							break;
					}					
					break;

				case IDOK: case IDCANCEL:
					EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam));
					break;
			}
			break;

		default:
			fProcessed = FALSE; break;
	}
	return(fProcessed);
}


///////////////////////////////// End of File /////////////////////////////////
