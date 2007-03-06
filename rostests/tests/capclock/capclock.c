/* $Id$
 *
 * DESCRIPTION: Simple Win32 Caption Clock
 * PROJECT    : ReactOS (test applications)
 * AUTHOR     : Emanuele Aliberti
 * DATE       : 2003-09-03
 * LICENSE    : GNU GPL v2.0
 */
#include <windows.h>
#include <string.h>

UINT Timer = 1;

static BOOL CALLBACK DialogFunc(HWND,UINT,WPARAM,LPARAM);
static VOID CALLBACK TimerProc(HWND,UINT,UINT,DWORD);


INT STDCALL WinMain (HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR lpCmdLine, INT nCmdShow)
{
	WNDCLASS wc;

	ZeroMemory (& wc, sizeof wc);
	wc.lpfnWndProc    = DefDlgProc;
	wc.cbWndExtra     = DLGWINDOWEXTRA;
	wc.hInstance      = hinst;
	wc.hCursor        = LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
	wc.hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
	wc.lpszClassName  = "CapClock";
	RegisterClass (& wc);
	return DialogBox(hinst, MAKEINTRESOURCE(2), NULL, DialogFunc);

}
static int InitializeApp (HWND hDlg,WPARAM wParam, LPARAM lParam)
{
	Timer = SetTimer (hDlg,Timer,1000,TimerProc);
	TimerProc (hDlg,0,0,0);
	return 1;
}
static INT_PTR CALLBACK DialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		InitializeApp(hwndDlg,wParam,lParam);
		return TRUE;
	case WM_CLOSE:
		KillTimer (hwndDlg,Timer);
		EndDialog(hwndDlg,0);
		return TRUE;
	}
	return FALSE;
}
static VOID CALLBACK TimerProc (HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	CHAR text [20];
	SYSTEMTIME lt;

	GetLocalTime (& lt);
	wsprintf (
		text,
		"%d-%02d-%02d %02d:%02d:%02d",
		lt.wYear,
		lt.wMonth,
		lt.wDay,
		lt.wHour,
		lt.wMinute,
		lt.wSecond);
	SetWindowText (hwnd, text);
}
/* EOF */
