/*
 * PROJECT:         Keyboard Layout Switcher
 * FILE:            base\applications\kbswitch\kbswitch.c
 * PURPOSE:         Switching Keyboard Layouts
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Colin Finck (mail@colinfinck.de)
 */

#include "kbswitch.h"

#define WM_NOTIFYICONMSG (WM_USER + 248)

PROC KbSwitchSetHooks    = NULL;
PROC KbSwitchDeleteHooks = NULL;


static BOOL
GetLayoutID(LPTSTR szLayoutNum, LPTSTR szLCID);

static BOOL
GetLayoutName(LPTSTR szLayoutNum, LPTSTR szName);

HINSTANCE hInst;
HANDLE    hProcessHeap;
HMODULE   hDllLib;
ULONG     ulCurrentLayoutNum = 1;

static HICON
CreateTrayIcon(LPTSTR szLCID)
{
    LANGID lId;
    TCHAR szBuf[3];
    HDC hdc, hdcsrc;
    HBITMAP hBitmap, hBmpNew, hBmpOld;
    RECT rect;
    DWORD bkColor, bkText;
    HFONT hFontOld, hFont = NULL;
    ICONINFO IconInfo;
    HICON hIcon = NULL;

    lId = (LANGID)_tcstoul(szLCID, NULL, 16);
    if (GetLocaleInfo(lId,
                      LOCALE_SISO639LANGNAME,
                      szBuf,
                      sizeof(szBuf) / sizeof(TCHAR)) == 0)
    {
        lstrcpy(szBuf, _T("??\0"));
    }

    hdcsrc = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcsrc);
    hBitmap = CreateCompatibleBitmap(hdcsrc, 16, 16);
    ReleaseDC(NULL, hdcsrc);

    if (hdc && hBitmap)
    {
        hBmpNew = CreateBitmap(16, 16, 1, 1, NULL);
        if (hBmpNew)
        {
            hBmpOld = SelectObject(hdc, hBitmap);
            rect.right = 16;
            rect.left = 0;
            rect.bottom = 16;
            rect.top = 0;

            bkColor = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
            bkText  = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

            ExtTextOut(hdc, rect.left, rect.top, ETO_OPAQUE, &rect, _T(""), 0, NULL);

            hFont = CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, FF_DONTCARE, _T("Tahoma"));

            hFontOld = SelectObject(hdc, hFont);
            DrawText(hdc, _tcsupr(szBuf), 2, &rect, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
            SelectObject(hdc, hBmpNew);
            PatBlt(hdc, 0, 0, 16, 16, BLACKNESS);
            SelectObject(hdc, hBmpOld);
            SelectObject(hdc, hFontOld);

            IconInfo.hbmColor = hBitmap;
            IconInfo.hbmMask = hBmpNew;
            IconInfo.fIcon = TRUE;

            hIcon = CreateIconIndirect(&IconInfo);

            DeleteObject(hBmpNew);
            DeleteObject(hBmpOld);
            DeleteObject(hFont);
        }
    }

    DeleteDC(hdc);
    DeleteObject(hBitmap);

    return hIcon;
}

static VOID
AddTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA tnid;
    TCHAR szLCID[CCH_LAYOUT_ID + 1];
    TCHAR szName[MAX_PATH];

    GetLayoutID(_T("1"), szLCID);
    GetLayoutName(_T("1"), szName);

    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = hwnd;
    tnid.uID = 1;
    tnid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szLCID);

    lstrcpyn(tnid.szTip, szName, sizeof(tnid.szTip));

    Shell_NotifyIcon(NIM_ADD, &tnid);
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

static VOID
UpdateTrayIcon(HWND hwnd, LPTSTR szLCID, LPTSTR szName)
{
    NOTIFYICONDATA tnid;

    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = hwnd;
    tnid.uID = 1;
    tnid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szLCID);

    lstrcpyn(tnid.szTip, szName, sizeof(tnid.szTip));

    Shell_NotifyIcon(NIM_MODIFY, &tnid);
}

static BOOL
GetLayoutID(LPTSTR szLayoutNum, LPTSTR szLCID)
{
    DWORD dwBufLen;
    DWORD dwRes;
    HKEY hKey;
    TCHAR szTempLCID[CCH_LAYOUT_ID + 1];

    // Get the Layout ID
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szTempLCID);
        dwRes = RegQueryValueEx(hKey, szLayoutNum, NULL, NULL, (LPBYTE)szTempLCID, &dwBufLen);

        if (dwRes != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        RegCloseKey(hKey);
    }

    // Look for a substitude of this layout
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szTempLCID);

        if (RegQueryValueEx(hKey, szTempLCID, NULL, NULL, (LPBYTE)szLCID, &dwBufLen) != ERROR_SUCCESS)
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

VOID
GetLayoutIDByHkl(HKL hKl, LPTSTR szLayoutID)
{
    /*
        FIXME!!! This way of getting layout ID incorrect!
                 This will not work correctly for 0001040a, 00010410, etc
    */
    wsprintf(szLayoutID, _T("%08x"), LOWORD(hKl));
}

static BOOL
GetLayoutName(LPTSTR szLayoutNum, LPTSTR szName)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[MAX_PATH], szDispName[MAX_PATH], szIndex[MAX_PATH], szPath[MAX_PATH];
    TCHAR szLCID[CCH_LAYOUT_ID + 1];
    HANDLE hLib;
    UINT i, j, k;

    if(!GetLayoutID(szLayoutNum, szLCID))
        return FALSE;

    wsprintf(szBuf, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), szLCID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)szBuf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szBuf);

        if (RegQueryValueEx(hKey, _T("Layout Display Name"), NULL, NULL, (LPBYTE)szDispName, &dwBufLen) == ERROR_SUCCESS)
        {
            if (szDispName[0] == '@')
            {
                for (i = 0; i < _tcslen(szDispName); i++)
                {
                    if ((szDispName[i] == ',') && (szDispName[i + 1] == '-'))
                    {
                        for (j = i + 2, k = 0; j < _tcslen(szDispName)+1; j++, k++)
                        {
                            szIndex[k] = szDispName[j];
                        }
                        szDispName[i - 1] = '\0';
                        break;
                    }
                    else szDispName[i] = szDispName[i + 1];
                }

                if (ExpandEnvironmentStrings(szDispName, szPath, MAX_PATH))
                {
                    hLib = LoadLibrary(szPath);
                    if (hLib)
                    {
                        if (LoadString(hLib, _ttoi(szIndex), szPath, sizeof(szPath) / sizeof(TCHAR)) != 0)
                        {
                            _tcscpy(szName, szPath);
                            RegCloseKey(hKey);
                            FreeLibrary(hLib);
                            return TRUE;
                        }
                        FreeLibrary(hLib);
                    }
                }
            }
        }

        dwBufLen = sizeof(szBuf);

        if (RegQueryValueEx(hKey, _T("Layout Text"), NULL, NULL, (LPBYTE)szName, &dwBufLen) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CALLBACK
EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, lParam);
    return TRUE;
}

static VOID
ActivateLayout(HWND hwnd, ULONG uLayoutNum)
{
    HKL hKl;
    TCHAR szLayoutNum[CCH_ULONG_DEC + 1];
    TCHAR szLCID[CCH_LAYOUT_ID + 1];
    TCHAR szLangName[MAX_PATH];

    _ultot(uLayoutNum, szLayoutNum, 10);
    GetLayoutID(szLayoutNum, szLCID);

    // Switch to the new keyboard layout
    GetLocaleInfo((LANGID)_tcstoul(szLCID, NULL, 16), LOCALE_SLANGUAGE, (LPTSTR)szLangName, sizeof(szLangName) / sizeof(TCHAR));
    UpdateTrayIcon(hwnd, szLCID, szLangName);
    hKl = LoadKeyboardLayout(szLCID, KLF_ACTIVATE);

    EnumWindows(EnumWindowsProc, (LPARAM) hKl);

    ulCurrentLayoutNum = uLayoutNum;
}

static HMENU
BuildLeftPopupMenu()
{
    HMENU hMenu;
    HKEY hKey;
    DWORD dwIndex, dwSize;
    TCHAR szLayoutNum[CCH_ULONG_DEC + 1];
    TCHAR szName[MAX_PATH];

    hMenu = CreatePopupMenu();

    // Add the keyboard layouts to the popup menu
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

        (void)CheckMenuItem(hMenu, ulCurrentLayoutNum, MF_CHECKED);

        RegCloseKey(hKey);
    }

    return hMenu;
}

BOOL
SetHooks()
{
    hDllLib = LoadLibrary(_T("kbsdll.dll"));
    if (!hDllLib) return FALSE;

    KbSwitchSetHooks    = (PROC) GetProcAddress(hDllLib, MAKEINTRESOURCEA(1));
    KbSwitchDeleteHooks = (PROC) GetProcAddress(hDllLib, MAKEINTRESOURCEA(2));

    if ((KbSwitchSetHooks == NULL)||(KbSwitchDeleteHooks == NULL))
        return FALSE;

    return KbSwitchSetHooks();
}

VOID
DeleteHooks()
{
    if (KbSwitchDeleteHooks) KbSwitchDeleteHooks();
    if (hDllLib) FreeLibrary(hDllLib);
}

ULONG
GetNextLayout()
{
    TCHAR szLayoutNum[3 + 1], szLayoutID[CCH_LAYOUT_ID + 1];
    ULONG Ret = ulCurrentLayoutNum;

    _ultot(ulCurrentLayoutNum, szLayoutNum, 10);
    if (!GetLayoutID(szLayoutNum, szLayoutID))
    {
        return -1;
    }

    _ultot(Ret + 1, szLayoutNum, 10);

    if (GetLayoutID(szLayoutNum, szLayoutID))
    {
        return (Ret + 1);
    }
    else
    {
        _ultot(Ret - 1, szLayoutNum, 10);
        if (GetLayoutID(szLayoutNum, szLayoutID))
            return (Ret - 1);
        else
            return -1;
    }

    return -1;
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static HMENU hRightPopupMenu;
    static TCHAR szLCID[MAX_PATH], szLangName[MAX_PATH];

    switch (Message)
    {
        case WM_CREATE:
        {
            SetHooks();
            AddTrayIcon(hwnd);
            hRightPopupMenu = GetSubMenu(LoadMenu(hInst, MAKEINTRESOURCE(IDR_POPUP)), 0);

            ActivateLayout(hwnd, ulCurrentLayoutNum);

            return 0;
        }

        case WM_LANG_CHANGED:
        {
            GetLayoutIDByHkl((HKL)lParam, szLCID);
            GetLocaleInfo((LANGID)_tcstoul(szLCID, NULL, 16), LOCALE_SLANGUAGE, (LPTSTR)szLangName, sizeof(szLangName) / sizeof(TCHAR));
            UpdateTrayIcon(hwnd, szLCID, szLangName);

            return 0;
        }

        case WM_LOAD_LAYOUT:
        {
            ActivateLayout(hwnd, GetNextLayout());

            return 0;
        }

        case WM_WINDOW_ACTIVATE:
        {
            GetLayoutIDByHkl(GetKeyboardLayout(GetWindowThreadProcessId((HWND)wParam, 0)), szLCID);
            GetLocaleInfo((LANGID)_tcstoul(szLCID, NULL, 16), LOCALE_SLANGUAGE, (LPTSTR)szLangName, sizeof(szLangName) / sizeof(TCHAR));
            UpdateTrayIcon(hwnd, szLCID, szLangName);

            return 0;
        }

        case WM_NOTIFYICONMSG:
            switch (lParam)
            {
                case WM_RBUTTONDOWN:
                case WM_LBUTTONDOWN:
                {
                    POINT pt;

                    GetCursorPos(&pt);
                    SetForegroundWindow(hwnd);

                    if (lParam == WM_LBUTTONDOWN)
                    {
                        HMENU hLeftPopupMenu;

                        /* Rebuild the left popup menu on every click to take care of keyboard layout changes */
                        hLeftPopupMenu = BuildLeftPopupMenu();
                        TrackPopupMenu(hLeftPopupMenu, 0, pt.x, pt.y, 0, hwnd, NULL);
                        DestroyMenu(hLeftPopupMenu);
                    }
                    else
                    {
                        TrackPopupMenu(hRightPopupMenu, 0, pt.x, pt.y, 0, hwnd, NULL);
                    }

                    PostMessage(hwnd, WM_NULL, 0, 0);

                    return 0;
                }
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case ID_EXIT:
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;

                case ID_PREFERENCES:
                {
                    SHELLEXECUTEINFO shInputDll = {0};

                    shInputDll.cbSize = sizeof(shInputDll);
                    shInputDll.hwnd = hwnd;
                    shInputDll.lpVerb = _T("open");
                    shInputDll.lpFile = _T("rundll32.exe");
                    shInputDll.lpParameters = _T("shell32.dll,Control_RunDLL input.dll");

                    if (!ShellExecuteEx(&shInputDll))
                        MessageBox(hwnd, _T("Can't start input.dll"), NULL, MB_OK | MB_ICONERROR);
                }

                default:
                    ActivateLayout(hwnd, LOWORD(wParam));
                    return 0;
            }
            break;

        case WM_SETTINGCHANGE:
        {
            if (wParam == SPI_SETDEFAULTINPUTLANG)
            {
                //FIXME: Should detect default language changes by CPL applet or by other tools and update UI
            }
        }
        break;

        case WM_DESTROY:
        {
            DeleteHooks();
            DestroyMenu(hRightPopupMenu);
            DelTrayIcon(hwnd);
            PostQuitMessage(0);

            return 0;
        }
    }

    return DefWindowProc(hwnd, Message, wParam, lParam);
}

INT WINAPI
_tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPTSTR lpCmdLine, INT nCmdShow)
{
    WNDCLASS WndClass = {0};
    MSG msg;
    HANDLE hMutex;

    hMutex = CreateMutex(NULL, FALSE, szKbSwitcherName);
    if ((!hMutex) || (GetLastError() == ERROR_ALREADY_EXISTS))
        return 1;

    hInst = hInstance;
    hProcessHeap = GetProcessHeap();

    WndClass.style = 0;
    WndClass.lpfnWndProc   = (WNDPROC)WndProc;
    WndClass.cbClsExtra    = 0;
    WndClass.cbWndExtra    = 0;
    WndClass.hInstance     = hInstance;
    WndClass.hIcon         = NULL;
    WndClass.hCursor       = NULL;
    WndClass.hbrBackground = NULL;
    WndClass.lpszMenuName  = NULL;
    WndClass.lpszClassName = szKbSwitcherName;

    if (!RegisterClass(&WndClass))
        return 1;

    CreateWindow(szKbSwitcherName, NULL, 0, 0, 0, 1, 1, HWND_DESKTOP, NULL, hInstance, NULL);

    while(GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hMutex) CloseHandle(hMutex);

    return 0;
}
