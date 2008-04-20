/*
 * PROJECT:         Keyboard Layout Switcher
 * FILE:            base\applications\kbswitch\kbswitch.c
 * PURPOSE:         Switching Keyboard Layouts
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Colin Finck (mail@colinfinck.de)
 */

#include "kbswitch.h"

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
GetLayoutID(LPTSTR szLayoutNum, LPTSTR szLCID)
{
    DWORD dwBufLen;
    DWORD dwTest;
    HKEY hKey;
    TCHAR szTempLCID[CCH_LAYOUT_ID + 1];

    // Get the Layout ID
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szTempLCID);

        if(RegQueryValueEx(hKey, szLayoutNum, NULL, NULL, (LPBYTE)szTempLCID, &dwBufLen) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        RegCloseKey(hKey);
    }

    // Look for a substitude of this layout
    if(RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szTempLCID);

        if(RegQueryValueEx(hKey, szTempLCID, NULL, NULL, (LPBYTE)szLCID, &dwBufLen) != ERROR_SUCCESS)
        {
            // No substitute found, then use the old LCID
            lstrcpy(szLCID, szTempLCID);
        }

        RegCloseKey(hKey);
    }
    else
    {
        // Substitutes key couldn't be opened, so use the old LCID
        lstrcpy(szLCID, szTempLCID);
    }

    return TRUE;
}

static BOOL
GetLayoutName(LPTSTR szLayoutNum, LPTSTR szName)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[MAX_PATH];
    TCHAR szLCID[CCH_LAYOUT_ID + 1];

    if(!GetLayoutID(szLayoutNum, szLCID))
        return FALSE;

    wsprintf(szBuf, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), szLCID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)szBuf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = MAX_PATH * sizeof(TCHAR);

        if(RegQueryValueEx(hKey, _T("Layout Text"), NULL, NULL, (LPBYTE)szName, &dwBufLen) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        RegCloseKey(hKey);
    }

    return TRUE;
}

static VOID
ActivateLayout(ULONG uLayoutNum)
{
    HKL hKl;
    TCHAR szLayoutNum[CCH_ULONG_DEC + 1];
    TCHAR szLCID[CCH_LAYOUT_ID + 1];
    DWORD Ret;

    _ultot(uLayoutNum, szLayoutNum, 10);
    GetLayoutID(szLayoutNum, szLCID);

    // Switch to the new keyboard layout
    hKl = LoadKeyboardLayout(szLCID, KLF_ACTIVATE);
    Ret = SystemParametersInfo(SPI_SETDEFAULTINPUTLANG, 0, &hKl, SPIF_SENDWININICHANGE);
}

static HMENU
BuildPopupMenu()
{
    HMENU hMenu;
    HKEY hKey;
    DWORD dwIndex, dwSize;
    TCHAR szLayoutNum[CCH_ULONG_DEC + 1];
    TCHAR szName[MAX_PATH];

    hMenu = CreatePopupMenu();

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        for(dwIndex = 0; ; dwIndex++)
        {
            dwSize = sizeof(szLayoutNum);
            if(RegEnumValue(hKey, dwIndex, szLayoutNum, &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                break;

            if(!GetLayoutName(szLayoutNum, szName))
                break;

            AppendMenu(hMenu, MF_STRING, _ttoi(szLayoutNum), szName);
        }

        RegCloseKey(hKey);
    }

    return hMenu;
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static HMENU hPopupMenu;

    switch (Message)
    {
        case WM_CREATE:
            AddTrayIcon(hwnd, LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAIN)));
            hPopupMenu = BuildPopupMenu(hwnd);
            break;

        case WM_NOTIFYICONMSG:
            switch (lParam)
            {
                case WM_LBUTTONDBLCLK:
                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                {
                    POINT pt;

                    GetCursorPos(&pt);
                    TrackPopupMenu(hPopupMenu, 0, pt.x, pt.y, 0, hwnd, NULL);
                }
                break;
            }
            break;

        case WM_COMMAND:
            ActivateLayout(LOWORD(wParam));
            break;

        case WM_DESTROY:
            DestroyMenu(hPopupMenu);
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
