#include <kbswitch.h>

#define WM_NOTIFYICONMSG (WM_USER + 248)
#define BUFSIZE 256

HINSTANCE hInst;
HWND      hwnd;

static VOID
AddTrayIcon(HWND hwnd, HICON hIcon)
{
	NOTIFYICONDATA tnid;

    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = hwnd;
    tnid.uID = 1;
    tnid.uFlags = NIF_ICON | NIF_MESSAGE;
	tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = hIcon;

    Shell_NotifyIcon(NIM_ADD, &tnid);

    if (hIcon) DestroyIcon(hIcon);
}

static VOID
DelTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA tnid;

	tnid.cbSize = sizeof(NOTIFYICONDATA);
	tnid.hWnd = hwnd;
	tnid.uID = 1;

	Shell_NotifyIcon(NIM_DELETE, &tnid);
}

static BOOL
GetLayoutName(LPCTSTR lcid, LPTSTR name)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[BUFSIZE];

    _stprintf(szBuf, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"),lcid);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)szBuf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = BUFSIZE;
        if (RegQueryValueEx(hKey,_T("Layout Text"),NULL,NULL,(LPBYTE)name,&dwBufLen) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }
    }

    return FALSE;
}

static VOID
ActivateLayout(INT LayoutID)
{
	TCHAR szLayoutID[MAX_PATH], szNewLayout[MAX_PATH];
	HKEY hKey;
	DWORD dwBufLen;

	_stprintf(szLayoutID, _T("%d"), LayoutID);

	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		dwBufLen = MAX_PATH;
		if (RegQueryValueEx(hKey, szLayoutID, NULL, NULL, (LPBYTE)szNewLayout, &dwBufLen) == ERROR_SUCCESS)
		{
			MessageBox(0, szNewLayout, _T(""), MB_OK);
			RegCloseKey(hKey);
		}
	}
}

static VOID
ShowPopupMenu(HWND hwnd, POINT pt)
{
	HMENU hMenu;
	HKEY hKey;
	DWORD dwIndex = 0, dwSize, dwType;
	LONG Ret;
	TCHAR szBuf[MAX_PATH], szPreload[MAX_PATH], szName[MAX_PATH];

	hMenu = CreatePopupMenu();

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"),
        0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		dwSize = MAX_PATH;
		Ret = RegEnumValue(hKey, dwIndex, szBuf, &dwSize, NULL, &dwType, NULL, NULL);
		if (Ret == ERROR_SUCCESS)
		{
			while (Ret == ERROR_SUCCESS)
			{
				dwSize = MAX_PATH;
				RegQueryValueEx(hKey, szBuf, NULL, NULL, (LPBYTE)szPreload, &dwSize);

				GetLayoutName(szPreload, szName);
				AppendMenu(hMenu, MF_STRING, _ttoi(szBuf), szName);

				dwIndex++;

				dwSize = MAX_PATH;
				Ret = RegEnumValue(hKey, dwIndex, szBuf, &dwSize, NULL, &dwType, NULL, NULL);
			}
		}
	}

	TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hwnd, NULL);
	DestroyMenu(hMenu);
	RegCloseKey(hKey);
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	POINT pt;

	switch (Message)
	{
		case WM_CREATE:
			AddTrayIcon(hwnd, LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAIN)));
		break;

		case WM_NOTIFYICONMSG:
			switch (lParam)
			{
				case WM_LBUTTONDBLCLK:
				case WM_LBUTTONDOWN:
				case WM_RBUTTONDOWN:
				{
					GetCursorPos(&pt);
					ShowPopupMenu(hwnd, pt);
				}
				break;
			}
		break;

		case WM_COMMAND:
			ActivateLayout(LOWORD(wParam));
		break;

		case WM_DESTROY:
			DelTrayIcon(hwnd);
			PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, Message, wParam, lParam);
}

INT WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPTSTR lpCmdLine, INT nCmdShow)
{
	WNDCLASS WndClass = {0};
	MSG msg;

	hInst = hInstance;

	WndClass.style = 0;
	WndClass.lpfnWndProc   = (WNDPROC)WndProc;
	WndClass.cbClsExtra    = 0;
	WndClass.cbWndExtra    = 0;
	WndClass.hInstance     = hInstance;
	WndClass.hIcon         = NULL;
	WndClass.hCursor       = NULL;
	WndClass.hbrBackground = NULL;
	WndClass.lpszMenuName  = NULL;
	WndClass.lpszClassName = L"kbswitch";

	if (!RegisterClass(&WndClass)) return 0;

	hwnd = CreateWindow(L"kbswitch", L"kbswitch", 0, 0, 0, 1, 1, HWND_DESKTOP, NULL, hInstance, NULL);

    while(GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
