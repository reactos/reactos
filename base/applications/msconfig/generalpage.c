#include <precomp.h>

HWND hGeneralPage;
HWND hGeneralDialog;

INT_PTR CALLBACK
GeneralPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        hGeneralDialog = hDlg;
	    SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
		return TRUE;
	}

  return 0;
}
