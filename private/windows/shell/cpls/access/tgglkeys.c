/*******************************************************************
 *    DESCRIPTION: ToggleKeys dialog
 *******************************************************************/

#include "Access.h"


// *******************************************************************
// Toggle Keys Settings handler
// *******************************************************************
INT_PTR CALLBACK ToggleKeySettingsDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   BOOL fProcessed = TRUE;

	switch (uMsg) {
		case WM_INITDIALOG:
			CheckDlgButton(hwnd, IDC_TK_HOTKEY, 
				(g_tk.dwFlags & TKF_HOTKEYACTIVE)?TRUE:FALSE );
			break;

      case WM_HELP:
			WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
			break;

      case WM_CONTEXTMENU:
         WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
			break;

    	case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {											
				case IDC_TK_HOTKEY:				
					g_tk.dwFlags ^= MKF_HOTKEYACTIVE;
               SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDOK: case IDCANCEL:
					EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam)); break;
			}
			break;

		default: fProcessed = FALSE; break;
	}
	return(fProcessed);
}


///////////////////////////////// End of File /////////////////////////////////
