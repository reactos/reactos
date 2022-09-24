/*
 * PROJECT:         Keyboard Layout Switcher
 * FILE:            base/applications/kbswitch/kbswitch.c
 * PURPOSE:         Switching Keyboard Layouts
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Colin Finck (mail@colinfinck.de)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "kbswitch.h"

#define WM_NOTIFYICONMSG (WM_USER + 248)
#define CX_ICON 16
#define CY_ICON 16

PKBSWITCHSETHOOKS    KbSwitchSetHooks    = NULL;
PKBSWITCHDELETEHOOKS KbSwitchDeleteHooks = NULL;
UINT ShellHookMessage = 0;

HINSTANCE hInst;
HANDLE    hProcessHeap;
HMODULE   g_hHookDLL = NULL;
ULONG     ulCurrentLayoutNum = 1;
HICON     g_hTrayIcon = NULL;

static BOOL
GetLayoutID(LPCTSTR szLayoutNum, LPTSTR szLCID, SIZE_T LCIDLength)
{
    DWORD dwBufLen, dwRes;
    HKEY hKey;
    TCHAR szTempLCID[CCH_LAYOUT_ID + 1];

    /* Get the Layout ID */
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_QUERY_VALUE,
                     &hKey) == ERROR_SUCCESS)
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

    /* Look for a substitute of this layout */
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0,
                     KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szTempLCID);
        if (RegQueryValueEx(hKey, szTempLCID, NULL, NULL, (LPBYTE)szLCID, &dwBufLen) != ERROR_SUCCESS)
        {
            /* No substitute found, then use the old LCID */
            StringCchCopy(szLCID, LCIDLength, szTempLCID);
        }

        RegCloseKey(hKey);
    }
    else
    {
        /* Substitutes key couldn't be opened, so use the old LCID */
        StringCchCopy(szLCID, LCIDLength, szTempLCID);
    }

    return TRUE;
}

static BOOL
GetLayoutName(LPCTSTR szLayoutNum, LPTSTR szName, SIZE_T NameLength)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[MAX_PATH], szDispName[MAX_PATH], szIndex[MAX_PATH], szPath[MAX_PATH];
    TCHAR szLCID[CCH_LAYOUT_ID + 1];
    HANDLE hLib;
    UINT i, j, k;

    if (!GetLayoutID(szLayoutNum, szLCID, ARRAYSIZE(szLCID)))
        return FALSE;

    StringCchPrintf(szBuf, ARRAYSIZE(szBuf),
                    _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), szLCID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    /* Use "Layout Display Name" value as an entry name if possible */
    dwBufLen = sizeof(szDispName);
    if (RegQueryValueEx(hKey, _T("Layout Display Name"), NULL, NULL,
                        (LPBYTE)szDispName, &dwBufLen) == ERROR_SUCCESS)
    {
        /* FIXME: Use shlwapi!SHLoadRegUIStringW instead if it was implemented */
        if (szDispName[0] == '@')
        {
            size_t len = _tcslen(szDispName);

            for (i = 0; i < len; i++)
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

            if (ExpandEnvironmentStrings(szDispName, szPath, ARRAYSIZE(szPath)))
            {
                hLib = LoadLibrary(szPath);
                if (hLib)
                {
                    if (LoadString(hLib, _ttoi(szIndex), szPath, ARRAYSIZE(szPath)))
                    {
                        StringCchCopy(szName, NameLength, szPath);
                        RegCloseKey(hKey);
                        FreeLibrary(hLib);
                        return TRUE;
                    }
                    FreeLibrary(hLib);
                }
            }
        }
    }

    /* Otherwise, use "Layout Text" value as an entry name */
    dwBufLen = NameLength * sizeof(TCHAR);
    if (RegQueryValueEx(hKey, _T("Layout Text"), NULL, NULL,
                        (LPBYTE)szName, &dwBufLen) == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return TRUE;
    }

    RegCloseKey(hKey);
    return FALSE;
}

static HICON
CreateTrayIcon(LPTSTR szLCID)
{
    LANGID LangID;
    TCHAR szBuf[4];
    HDC hdc;
    HBITMAP hbmColor, hbmMono, hBmpOld;
    RECT rect;
    HFONT hFontOld, hFont;
    ICONINFO IconInfo;
    HICON hIcon;
    LOGFONT lf;
    BITMAPINFO bmi;

    /* Getting "EN", "FR", etc. from English, French, ... */
    LangID = LOWORD(_tcstoul(szLCID, NULL, 16));
    if (!GetLocaleInfo(LangID, LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                       szBuf, ARRAYSIZE(szBuf)))
    {
        StringCchCopy(szBuf, ARRAYSIZE(szBuf), _T("??"));
    }
    szBuf[2] = 0; /* "ENG" --> "EN" etc. */

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = CX_ICON;
    bmi.bmiHeader.biHeight = CY_ICON;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;

    /* Create hdc, hbmColor and hbmMono */
    hdc = CreateCompatibleDC(NULL);
    hbmColor = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    hbmMono = CreateBitmap(CX_ICON, CY_ICON, 1, 1, NULL);

    /* Create a font */
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);
    hFont = CreateFontIndirect(&lf);

    /* Checking NULL */
    if (!hdc || !hbmColor || !hbmMono || !hFont)
    {
        if (hdc)
            DeleteDC(hdc);
        if (hbmColor)
            DeleteObject(hbmColor);
        if (hbmMono)
            DeleteObject(hbmMono);
        if (hFont)
            DeleteObject(hFont);
        return NULL;
    }

    SetRect(&rect, 0, 0, CX_ICON, CY_ICON);

    /* Draw hbmColor */
    hBmpOld = SelectObject(hdc, hbmColor);
    SetDCBrushColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
    hFontOld = SelectObject(hdc, hFont);
    SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, szBuf, 2, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    SelectObject(hdc, hFontOld);
    SelectObject(hdc, hBmpOld);

    /* Fill hbmMono by black */
    hBmpOld = SelectObject(hdc, hbmMono);
    PatBlt(hdc, 0, 0, CX_ICON, CY_ICON, BLACKNESS);
    SelectObject(hdc, hBmpOld);

    /* Create an icon from hbmColor and hbmMono */
    IconInfo.hbmColor = hbmColor;
    IconInfo.hbmMask = hbmMono;
    IconInfo.fIcon = TRUE;
    hIcon = CreateIconIndirect(&IconInfo);

    /* Clean up */
    DeleteObject(hbmColor);
    DeleteObject(hbmMono);
    DeleteObject(hFont);
    DeleteDC(hdc);

    return hIcon;
}

static VOID
AddTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA tnid = { sizeof(tnid), hwnd, 1, NIF_ICON | NIF_MESSAGE | NIF_TIP };
    TCHAR szLCID[CCH_LAYOUT_ID + 1], szName[MAX_PATH];

    GetLayoutID(_T("1"), szLCID, ARRAYSIZE(szLCID));
    GetLayoutName(_T("1"), szName, ARRAYSIZE(szName));

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szLCID);
    StringCchCopy(tnid.szTip, ARRAYSIZE(tnid.szTip), szName);

    Shell_NotifyIcon(NIM_ADD, &tnid);

    if (g_hTrayIcon)
        DestroyIcon(g_hTrayIcon);
    g_hTrayIcon = tnid.hIcon;
}

static VOID
DeleteTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA tnid = { sizeof(tnid), hwnd, 1 };
    Shell_NotifyIcon(NIM_DELETE, &tnid);

    if (g_hTrayIcon)
    {
        DestroyIcon(g_hTrayIcon);
        g_hTrayIcon = NULL;
    }
}

static VOID
UpdateTrayIcon(HWND hwnd, LPTSTR szLCID, LPTSTR szName)
{
    NOTIFYICONDATA tnid = { sizeof(tnid), hwnd, 1, NIF_ICON | NIF_MESSAGE | NIF_TIP };
    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szLCID);
    StringCchCopy(tnid.szTip, ARRAYSIZE(tnid.szTip), szName);

    Shell_NotifyIcon(NIM_MODIFY, &tnid);

    if (g_hTrayIcon)
        DestroyIcon(g_hTrayIcon);
    g_hTrayIcon = tnid.hIcon;
}

static VOID
GetLayoutIDByHkl(HKL hKl, LPTSTR szLayoutID, SIZE_T LayoutIDLength)
{
    StringCchPrintf(szLayoutID, LayoutIDLength, _T("%08lx"), (DWORD)(DWORD_PTR)(hKl));
}

static BOOL CALLBACK
EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, lParam);
    return TRUE;
}

static VOID
ActivateLayout(HWND hwnd, ULONG uLayoutNum)
{
    HKL hKl;
    TCHAR szLayoutNum[CCH_ULONG_DEC + 1], szLCID[CCH_LAYOUT_ID + 1], szLangName[MAX_PATH];
    LANGID LangID;

    /* The layout number starts from one. Zero is invalid */
    if (uLayoutNum == 0 || uLayoutNum > 0xFF) /* Invalid */
        return;

    _ultot(uLayoutNum, szLayoutNum, 10);
    GetLayoutID(szLayoutNum, szLCID, ARRAYSIZE(szLCID));
    LangID = (LANGID)_tcstoul(szLCID, NULL, 16);

    /* Switch to the new keyboard layout */
    GetLocaleInfo(LangID, LOCALE_SLANGUAGE, szLangName, ARRAYSIZE(szLangName));
    UpdateTrayIcon(hwnd, szLCID, szLangName);
    hKl = LoadKeyboardLayout(szLCID, KLF_ACTIVATE);

    /* Post WM_INPUTLANGCHANGEREQUEST to every top-level window */
    EnumWindows(EnumWindowsProc, (LPARAM) hKl);

    ulCurrentLayoutNum = uLayoutNum;
}

static HMENU
BuildLeftPopupMenu(VOID)
{
    HMENU hMenu = CreatePopupMenu();
    HKEY hKey;
    DWORD dwIndex, dwSize;
    TCHAR szLayoutNum[CCH_ULONG_DEC + 1], szName[MAX_PATH];

    /* Add the keyboard layouts to the popup menu */
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0,
                     KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        for (dwIndex = 0; ; dwIndex++)
        {
            dwSize = sizeof(szLayoutNum);
            if (RegEnumValue(hKey, dwIndex, szLayoutNum, &dwSize, NULL, NULL,
                             NULL, NULL) != ERROR_SUCCESS)
            {
                break;
            }

            if (!GetLayoutName(szLayoutNum, szName, ARRAYSIZE(szName)))
                break;

            AppendMenu(hMenu, MF_STRING, _ttoi(szLayoutNum), szName);
        }

        CheckMenuItem(hMenu, ulCurrentLayoutNum, MF_CHECKED);

        RegCloseKey(hKey);
    }

    return hMenu;
}

static ULONG
GetMaxLayoutNum(VOID)
{
    HKEY hKey;
    ULONG dwIndex, dwSize, uLayoutNum, uMaxLayoutNum = 0;
    TCHAR szLayoutNum[CCH_ULONG_DEC + 1], szLayoutID[CCH_LAYOUT_ID + 1];

    /* Get the maximum layout number in the Preload key */
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0,
                     KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        for (dwIndex = 0; ; dwIndex++)
        {
            dwSize = sizeof(szLayoutNum);
            if (RegEnumValue(hKey, dwIndex, szLayoutNum, &dwSize, NULL, NULL,
                             NULL, NULL) != ERROR_SUCCESS)
            {
                break;
            }

            if (GetLayoutID(szLayoutNum, szLayoutID, ARRAYSIZE(szLayoutID)))
            {
                uLayoutNum = _ttoi(szLayoutNum);
                if (uMaxLayoutNum < uLayoutNum)
                    uMaxLayoutNum = uLayoutNum;
            }
        }

        RegCloseKey(hKey);
    }

    return uMaxLayoutNum;
}

BOOL
SetHooks(VOID)
{
    g_hHookDLL = LoadLibrary(_T("kbsdll.dll"));
    if (!g_hHookDLL)
    {
        return FALSE;
    }

    KbSwitchSetHooks    = (PKBSWITCHSETHOOKS) GetProcAddress(g_hHookDLL, "KbSwitchSetHooks");
    KbSwitchDeleteHooks = (PKBSWITCHDELETEHOOKS) GetProcAddress(g_hHookDLL, "KbSwitchDeleteHooks");

    if (KbSwitchSetHooks == NULL || KbSwitchDeleteHooks == NULL)
    {
        return FALSE;
    }

    return KbSwitchSetHooks();
}

VOID
DeleteHooks(VOID)
{
    if (KbSwitchDeleteHooks) KbSwitchDeleteHooks();
    if (g_hHookDLL) FreeLibrary(g_hHookDLL);
}

ULONG
GetNextLayout(VOID)
{
    TCHAR szLayoutNum[3 + 1], szLayoutID[CCH_LAYOUT_ID + 1];
    ULONG uLayoutNum, uMaxNum = GetMaxLayoutNum();

    for (uLayoutNum = ulCurrentLayoutNum + 1; ; ++uLayoutNum)
    {
        if (uLayoutNum > uMaxNum)
            uLayoutNum = 1;
        if (uLayoutNum == ulCurrentLayoutNum)
            break;

        _ultot(uLayoutNum, szLayoutNum, 10);
        if (GetLayoutID(szLayoutNum, szLayoutID, ARRAYSIZE(szLayoutID)))
            return uLayoutNum;
    }

    return ulCurrentLayoutNum;
}

LRESULT
UpdateLanguageDisplay(HWND hwnd, HKL hKl)
{
    TCHAR szLCID[MAX_PATH], szLangName[MAX_PATH];
    LANGID LangID;

    GetLayoutIDByHkl(hKl, szLCID, ARRAYSIZE(szLCID));
    LangID = (LANGID)_tcstoul(szLCID, NULL, 16);
    GetLocaleInfo(LangID, LOCALE_SLANGUAGE, szLangName, ARRAYSIZE(szLangName));
    UpdateTrayIcon(hwnd, szLCID, szLangName);

    return 0;
}

LRESULT
UpdateLanguageDisplayCurrent(HWND hwnd, WPARAM wParam)
{
    DWORD dwThreadID = GetWindowThreadProcessId((HWND)wParam, 0);
    HKL hKL = GetKeyboardLayout(dwThreadID);
    return UpdateLanguageDisplay(hwnd, hKL);
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static HMENU s_hMenu = NULL, s_hRightPopupMenu = NULL;
    static UINT s_uTaskbarRestart;
    POINT pt;
    HMENU hLeftPopupMenu;

    switch (Message)
    {
        case WM_CREATE:
        {
            if (!SetHooks())
                return -1;

            AddTrayIcon(hwnd);

            ActivateLayout(hwnd, ulCurrentLayoutNum);
            s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
            break;
        }

        case WM_LANG_CHANGED:
        {
            return UpdateLanguageDisplay(hwnd, (HKL)lParam);
        }

        case WM_LOAD_LAYOUT:
        {
            ULONG uNextNum = GetNextLayout();
            if (ulCurrentLayoutNum != uNextNum)
                ActivateLayout(hwnd, uNextNum);
            break;
        }

        case WM_WINDOW_ACTIVATE:
        {
            return UpdateLanguageDisplayCurrent(hwnd, wParam);
        }

        case WM_NOTIFYICONMSG:
        {
            switch (lParam)
            {
                case WM_RBUTTONUP:
                case WM_LBUTTONUP:
                {
                    GetCursorPos(&pt);
                    SetForegroundWindow(hwnd);

                    if (lParam == WM_LBUTTONUP)
                    {
                        /* Rebuild the left popup menu on every click to take care of keyboard layout changes */
                        hLeftPopupMenu = BuildLeftPopupMenu();
                        TrackPopupMenu(hLeftPopupMenu, 0, pt.x, pt.y, 0, hwnd, NULL);
                        DestroyMenu(hLeftPopupMenu);
                    }
                    else
                    {
                        if (!s_hRightPopupMenu)
                        {
                            s_hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_POPUP));
                            s_hRightPopupMenu = GetSubMenu(s_hMenu, 0);
                        }
                        TrackPopupMenu(s_hRightPopupMenu, 0, pt.x, pt.y, 0, hwnd, NULL);
                    }

                    PostMessage(hwnd, WM_NULL, 0, 0);
                    break;
                }
            }
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case ID_EXIT:
                {
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                }

                case ID_PREFERENCES:
                {
                    SHELLEXECUTEINFO shInputDll = { sizeof(shInputDll) };
                    shInputDll.hwnd = hwnd;
                    shInputDll.lpVerb = _T("open");
                    shInputDll.lpFile = _T("rundll32.exe");
                    shInputDll.lpParameters = _T("shell32.dll,Control_RunDLL input.dll");
                    if (!ShellExecuteEx(&shInputDll))
                        MessageBox(hwnd, _T("Can't start input.dll"), NULL, MB_OK | MB_ICONERROR);

                    break;
                }

                default:
                {
                    ActivateLayout(hwnd, LOWORD(wParam));
                    break;
                }
            }
            break;

        case WM_SETTINGCHANGE:
        {
            if (wParam == SPI_SETDEFAULTINPUTLANG)
            {
                //FIXME: Should detect default language changes by CPL applet or by other tools and update UI
            }
            if (wParam == SPI_SETNONCLIENTMETRICS)
            {
                return UpdateLanguageDisplayCurrent(hwnd, wParam);
            }
        }
        break;

        case WM_DESTROY:
        {
            DeleteHooks();
            DestroyMenu(s_hMenu);
            DeleteTrayIcon(hwnd);
            PostQuitMessage(0);
            break;
        }

        default:
        {
            if (Message == s_uTaskbarRestart)
            {
                AddTrayIcon(hwnd);
                break;
            }
            else if (Message == ShellHookMessage && wParam == HSHELL_LANGUAGE)
            {
                PostMessage(hwnd, WM_LANG_CHANGED, wParam, lParam);
                break;
            }
            return DefWindowProc(hwnd, Message, wParam, lParam);
        }
    }

    return 0;
}

INT WINAPI
_tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPTSTR lpCmdLine, INT nCmdShow)
{
    WNDCLASS WndClass;
    MSG msg;
    HANDLE hMutex;
    HWND hwnd;

    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
            SetProcessDefaultLayout(LAYOUT_RTL);
            break;
        default:
            break;
    }

    hMutex = CreateMutex(NULL, FALSE, szKbSwitcherName);
    if (!hMutex)
        return 1;

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hMutex);
        return 1;
    }

    hInst = hInstance;
    hProcessHeap = GetProcessHeap();

    ZeroMemory(&WndClass, sizeof(WndClass));
    WndClass.lpfnWndProc   = WndProc;
    WndClass.hInstance     = hInstance;
    WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WndClass.lpszClassName = szKbSwitcherName;
    if (!RegisterClass(&WndClass))
    {
        CloseHandle(hMutex);
        return 1;
    }

    hwnd = CreateWindow(szKbSwitcherName, NULL, 0, 0, 0, 1, 1, HWND_DESKTOP, NULL, hInstance, NULL);
    ShellHookMessage = RegisterWindowMessage(L"SHELLHOOK");
    RegisterShellHookWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(hMutex);
    return 0;
}
