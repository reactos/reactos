//
// Downlevel (NT4, win9X) Install/Unistall page
//

STDAPI DL_FillAppListBox(HWND hwndListBox, DWORD* pdwApps);
STDAPI_(BOOL) DL_ConfigureButtonsAndStatic(HWND hwndPage, HWND hwndListBox, int iSel);
STDAPI_(BOOL) DL_InvokeAction(int iButtonID, HWND hwndPage, HWND hwndListBox, int iSel);

