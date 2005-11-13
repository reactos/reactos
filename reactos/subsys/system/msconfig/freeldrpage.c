#include <precomp.h>

HWND hFreeLdrPage;
HWND hFreeLdrDialog;

INT_PTR CALLBACK
FreeLdrPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        hFreeLdrDialog = hDlg;
	    SetWindowPos(hDlg, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
		return TRUE;
	}

  return 0;
}
