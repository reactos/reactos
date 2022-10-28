/*
 * PROJECT:         Keyboard Layout Switcher
 * FILE:            base/applications/kbswitch/kbswitch.c
 * PURPOSE:         Switching Keyboard Layouts
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Colin Finck (mail@colinfinck.de)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "kbswitch.h"
#include <imm.h>

/*
 * This program kbswitch is a mimic of Win2k's internat.exe.
 * However, there are some differences.
 *
 * Comparing with WinNT4 ActivateKeyboardLayout, WinXP ActivateKeyboardLayout has
 * process boundary, so we cannot activate the IME keyboard layout from the outer process.
 * It needs special care.
 *
 * We use global hook by our kbsdll.dll, to watch the shell and the windows.
 *
 * It might not work correctly on Vista+ because keyboard layout change notification
 * won't be generated in Vista+.
 */

#define WM_NOTIFYICONMSG (WM_USER + 248)

PKBSWITCHSETHOOKS    KbSwitchSetHooks    = NULL;
PKBSWITCHDELETEHOOKS KbSwitchDeleteHooks = NULL;
UINT ShellHookMessage = 0;

HINSTANCE hInst;
HANDLE    hProcessHeap;
HMODULE   g_hHookDLL = NULL;
ULONG     ulCurrentLayoutNum = 1;
HICON     g_hTrayIcon = NULL;
HWND      g_hwndLastActive = NULL;

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
GetSystemLibraryPath(LPTSTR szPath, SIZE_T cchPath, LPCTSTR FileName)
{
    if (!GetSystemDirectory(szPath, cchPath))
        return FALSE;

    StringCchCat(szPath, cchPath, TEXT("\\"));
    StringCchCat(szPath, cchPath, FileName);
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
                        (LPBYTE)szName, &dwBufLen) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);
    return TRUE;
}

static BOOL GetImeFile(LPTSTR szImeFile, SIZE_T cchImeFile, LPCTSTR szLCID)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[MAX_PATH];

    szImeFile[0] = UNICODE_NULL;

    if (_tcslen(szLCID) != CCH_LAYOUT_ID)
        return FALSE; /* Invalid LCID */

    if (szLCID[0] != TEXT('E') && szLCID[0] != TEXT('e'))
        return FALSE; /* Not an IME HKL */

    StringCchPrintf(szBuf, ARRAYSIZE(szBuf),
                    _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), szLCID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwBufLen = cchImeFile * sizeof(TCHAR);
    if (RegQueryValueEx(hKey, _T("IME File"), NULL, NULL,
                        (LPBYTE)szImeFile, &dwBufLen) != ERROR_SUCCESS)
    {
        szImeFile[0] = UNICODE_NULL;
    }

    RegCloseKey(hKey);

    return (szImeFile[0] != UNICODE_NULL);
}

typedef struct tagLOAD_ICON
{
    INT cxIcon, cyIcon;
    HICON hIcon;
} LOAD_ICON, *PLOAD_ICON;

static BOOL CALLBACK
EnumResNameProc(
    HMODULE hModule,
    LPCTSTR lpszType,
    LPTSTR lpszName,
    LPARAM lParam)
{
    PLOAD_ICON pLoadIcon = (PLOAD_ICON)lParam;
    pLoadIcon->hIcon = (HICON)LoadImage(hModule, lpszName, IMAGE_ICON,
                                        pLoadIcon->cxIcon, pLoadIcon->cyIcon,
                                        LR_DEFAULTCOLOR);
    if (pLoadIcon->hIcon)
        return FALSE; /* Stop enumeration */
    return TRUE;
}

static HICON FakeExtractIcon(LPCTSTR szIconPath, INT cxIcon, INT cyIcon)
{
    LOAD_ICON LoadIcon = { cxIcon, cyIcon, NULL };
    HMODULE hImeDLL = LoadLibraryEx(szIconPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hImeDLL)
    {
        EnumResourceNames(hImeDLL, RT_GROUP_ICON, EnumResNameProc, (LPARAM)&LoadIcon);
        FreeLibrary(hImeDLL);
    }
    return LoadIcon.hIcon;
}

static HICON
CreateTrayIcon(LPTSTR szLCID, LPCTSTR szImeFile OPTIONAL)
{
    LANGID LangID;
    TCHAR szBuf[4];
    HDC hdcScreen, hdc;
    HBITMAP hbmColor, hbmMono, hBmpOld;
    HFONT hFont, hFontOld;
    LOGFONT lf;
    RECT rect;
    ICONINFO IconInfo;
    HICON hIcon;
    INT cxIcon = GetSystemMetrics(SM_CXSMICON);
    INT cyIcon = GetSystemMetrics(SM_CYSMICON);
    TCHAR szPath[MAX_PATH];

    if (szImeFile && szImeFile[0])
    {
        if (GetSystemLibraryPath(szPath, ARRAYSIZE(szPath), szImeFile))
            return FakeExtractIcon(szPath, cxIcon, cyIcon);
    }

    /* Getting "EN", "FR", etc. from English, French, ... */
    LangID = LANGIDFROMLCID(_tcstoul(szLCID, NULL, 16));
    if (GetLocaleInfo(LangID,
                      LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                      szBuf,
                      ARRAYSIZE(szBuf)) == 0)
    {
        szBuf[0] = szBuf[1] = _T('?');
    }
    szBuf[2] = 0; /* Truncate the identifier to two characters: "ENG" --> "EN" etc. */

    /* Create hdc, hbmColor and hbmMono */
    hdcScreen = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcScreen);
    hbmColor = CreateCompatibleBitmap(hdcScreen, cxIcon, cyIcon);
    ReleaseDC(NULL, hdcScreen);
    hbmMono = CreateBitmap(cxIcon, cyIcon, 1, 1, NULL);

    /* Checking NULL */
    if (!hdc || !hbmColor || !hbmMono)
    {
        if (hbmMono)
            DeleteObject(hbmMono);
        if (hbmColor)
            DeleteObject(hbmColor);
        if (hdc)
            DeleteDC(hdc);
        return NULL;
    }

    /* Create a font */
    hFont = NULL;
    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
    {
        /* Override the current size with something manageable */
        lf.lfHeight = -11;
        lf.lfWidth = 0;
        hFont = CreateFontIndirect(&lf);
    }
    if (!hFont)
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    SetRect(&rect, 0, 0, cxIcon, cyIcon);

    /* Draw hbmColor */
    hBmpOld = SelectObject(hdc, hbmColor);
    SetDCBrushColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
    hFontOld = SelectObject(hdc, hFont);
    SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, szBuf, 2, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    SelectObject(hdc, hFontOld);

    /* Fill hbmMono with black */
    SelectObject(hdc, hbmMono);
    PatBlt(hdc, 0, 0, cxIcon, cyIcon, BLACKNESS);
    SelectObject(hdc, hBmpOld);

    /* Create an icon from hbmColor and hbmMono */
    IconInfo.fIcon = TRUE;
    IconInfo.xHotspot = IconInfo.yHotspot = 0;
    IconInfo.hbmColor = hbmColor;
    IconInfo.hbmMask = hbmMono;
    hIcon = CreateIconIndirect(&IconInfo);

    /* Clean up */
    DeleteObject(hFont);
    DeleteObject(hbmMono);
    DeleteObject(hbmColor);
    DeleteDC(hdc);

    return hIcon;
}

static VOID
AddTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA tnid = { sizeof(tnid), hwnd, 1, NIF_ICON | NIF_MESSAGE | NIF_TIP };
    TCHAR szLCID[CCH_LAYOUT_ID + 1], szName[MAX_PATH];
    TCHAR szImeFile[80];

    GetLayoutID(_T("1"), szLCID, ARRAYSIZE(szLCID));
    GetLayoutName(_T("1"), szName, ARRAYSIZE(szName));
    GetImeFile(szImeFile, ARRAYSIZE(szImeFile), szLCID);

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szLCID, szImeFile);
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
    TCHAR szImeFile[80];

    GetImeFile(szImeFile, ARRAYSIZE(szImeFile), szLCID);

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szLCID, szImeFile);
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
    PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, INPUTLANGCHANGE_SYSCHARSET, lParam);
    return TRUE;
}

static VOID
ActivateLayout(HWND hwnd, ULONG uLayoutNum, HWND hwndTarget OPTIONAL)
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

    if (hwndTarget)
        SetForegroundWindow(hwndTarget);

    hKl = LoadKeyboardLayout(szLCID, KLF_ACTIVATE);
    if (hKl)
        ActivateKeyboardLayout(hKl, KLF_SETFORPROCESS);

    /* Post WM_INPUTLANGCHANGEREQUEST */
    if (hwndTarget)
    {
        PostMessage(hwndTarget, WM_INPUTLANGCHANGEREQUEST,
                    INPUTLANGCHANGE_SYSCHARSET, (LPARAM)hKl);
    }
    else
    {
        EnumWindows(EnumWindowsProc, (LPARAM) hKl);
    }

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
                continue;

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
    if (KbSwitchDeleteHooks)
    {
        KbSwitchDeleteHooks();
        KbSwitchDeleteHooks = NULL;
    }
    if (g_hHookDLL)
    {
        FreeLibrary(g_hHookDLL);
        g_hHookDLL = NULL;
    }
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

UINT
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

HWND
GetTargetWindow(HWND hwndFore)
{
    TCHAR szClass[64];
    HWND hwndIME;
    HWND hwndTarget = hwndFore;
    if (hwndTarget == NULL)
        hwndTarget = GetForegroundWindow();

    GetClassName(hwndTarget, szClass, ARRAYSIZE(szClass));
    if (_tcsicmp(szClass, szKbSwitcherName) == 0)
        hwndTarget = g_hwndLastActive;

    hwndIME = ImmGetDefaultIMEWnd(hwndTarget);
    return (hwndIME ? hwndIME : hwndTarget);
}

UINT
UpdateLanguageDisplayCurrent(HWND hwnd, HWND hwndFore)
{
    DWORD dwThreadID = GetWindowThreadProcessId(GetTargetWindow(hwndFore), NULL);
    HKL hKL = GetKeyboardLayout(dwThreadID);
    UpdateLanguageDisplay(hwnd, hKL);

    if (IsWindow(g_hwndLastActive))
        SetForegroundWindow(g_hwndLastActive);

    return 0;
}

static UINT GetCurLayoutNum(HKL hKL)
{
    UINT i, nCount;
    HKL ahKL[256];

    nCount = GetKeyboardLayoutList(ARRAYSIZE(ahKL), ahKL);
    for (i = 0; i < nCount; ++i)
    {
        if (ahKL[i] == hKL)
            return i + 1;
    }

    return 0;
}

static BOOL RememberLastActive(HWND hwnd, HWND hwndFore)
{
    TCHAR szClass[64];

    hwndFore = GetAncestor(hwndFore, GA_ROOT);

    if (!IsWindowVisible(hwndFore) || !GetClassName(hwndFore, szClass, ARRAYSIZE(szClass)))
        return FALSE;

    if (_tcsicmp(szClass, szKbSwitcherName) == 0 ||
        _tcsicmp(szClass, TEXT("Shell_TrayWnd")) == 0)
    {
        return FALSE; /* Special window */
    }

    /* FIXME: CONWND is multithreaded but KLF_SETFORPROCESS and
              DefWindowProc.WM_INPUTLANGCHANGEREQUEST won't work yet */
    if (_tcsicmp(szClass, TEXT("ConsoleWindowClass")) == 0)
    {
        HKL hKL = GetKeyboardLayout(0);
        UpdateLanguageDisplay(hwnd, hKL);
    }

    g_hwndLastActive = hwndFore;
    return TRUE;
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
            {
                MessageBox(NULL, TEXT("SetHooks failed."), NULL, MB_ICONERROR);
                return -1;
            }

            AddTrayIcon(hwnd);

            ActivateLayout(hwnd, ulCurrentLayoutNum, NULL);
            s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
            break;
        }

        case WM_LANG_CHANGED: /* Comes from kbsdll.dll and this module */
        {
            UpdateLanguageDisplay(hwnd, (HKL)lParam);
            break;
        }

        case WM_WINDOW_ACTIVATE: /* Comes from kbsdll.dll and this module */
        {
            HWND hwndFore = GetForegroundWindow();
            if (RememberLastActive(hwnd, hwndFore))
                return UpdateLanguageDisplayCurrent(hwnd, hwndFore);
            break;
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
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                }

                case ID_PREFERENCES:
                {
                    INT_PTR ret = (INT_PTR)ShellExecute(hwnd, NULL,
                                                        TEXT("control.exe"), TEXT("input.dll"),
                                                        NULL, SW_SHOWNORMAL);
                    if (ret <= 32)
                        MessageBox(hwnd, _T("Can't start input.dll"), NULL, MB_ICONERROR);
                    break;
                }

                case ID_NEXTLAYOUT:
                {
                    HWND hwndTarget = (HWND)lParam, hwndTargetSave = NULL;
                    DWORD dwThreadID;
                    HKL hKL;
                    UINT uNum;
                    TCHAR szClass[64];
                    BOOL bCONWND = FALSE;

                    if (hwndTarget == NULL)
                        hwndTarget = g_hwndLastActive;

                    /* FIXME: CONWND is multithreaded but KLF_SETFORPROCESS and
                              DefWindowProc.WM_INPUTLANGCHANGEREQUEST won't work yet */
                    if (hwndTarget &&
                        GetClassName(hwndTarget, szClass, ARRAYSIZE(szClass)) &&
                        _tcsicmp(szClass, TEXT("ConsoleWindowClass")) == 0)
                    {
                        bCONWND = TRUE;
                        hwndTargetSave = hwndTarget;
                        hwndTarget = NULL;
                    }

                    if (hwndTarget)
                    {
                        dwThreadID = GetWindowThreadProcessId(hwndTarget, NULL);
                        hKL = GetKeyboardLayout(dwThreadID);
                        uNum = GetCurLayoutNum(hKL);
                        if (uNum != 0)
                            ulCurrentLayoutNum = uNum;
                    }

                    ActivateLayout(hwnd, GetNextLayout(), hwndTarget);

                    /* FIXME: CONWND is multithreaded but KLF_SETFORPROCESS and
                              DefWindowProc.WM_INPUTLANGCHANGEREQUEST won't work yet */
                    if (bCONWND)
                    {
                        ActivateLayout(hwnd, ulCurrentLayoutNum, hwndTargetSave);
                    }
                    break;
                }

                default:
                {
                    if (1 <= LOWORD(wParam) && LOWORD(wParam) <= 1000)
                    {
                        if (!IsWindow(g_hwndLastActive))
                        {
                            g_hwndLastActive = NULL;
                        }
                        ActivateLayout(hwnd, LOWORD(wParam), g_hwndLastActive);
                    }
                    break;
                }
            }
            break;

        case WM_SETTINGCHANGE:
        {
            if (wParam == SPI_SETNONCLIENTMETRICS)
            {
                PostMessage(hwnd, WM_WINDOW_ACTIVATE, wParam, lParam);
                break;
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
            else if (Message == ShellHookMessage)
            {
                if (wParam == HSHELL_LANGUAGE)
                    PostMessage(hwnd, WM_LANG_CHANGED, wParam, lParam);
                else if (wParam == HSHELL_WINDOWACTIVATED)
                    PostMessage(hwnd, WM_WINDOW_ACTIVATE, wParam, lParam);

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
