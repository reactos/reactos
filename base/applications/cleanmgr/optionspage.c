#include "resource.h"
#include "precomp.h"

INT_PTR CALLBACK OptionsPageDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
		SetWindowPos(hwnd, NULL, 10, 32, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
		return TRUE;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CLEAN_PROGRAMS:
			ShellExecute(hwnd, NULL, L"control.exe", L"appwiz.cpl", NULL, SW_SHOW);
			break;
		}

	default:
		return FALSE;
	}
	return TRUE;
}
