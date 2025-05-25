/*
 * PROJECT:     ReactOS Keyboard Layout Switcher
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Switching Keyboard Layouts
 * COPYRIGHT:   Copyright Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright Colin Finck (mail@colinfinck.de)
 *              Copyright 2022-2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "kbswitch.h"
#include <shlobj.h>
#include <shlwapi_undoc.h>
#include <imm.h>
#include <imm32_undoc.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(internat);

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

#define TIMER_ID_LANG_CHANGED_DELAYED 0x10000
#define TIMER_LANG_CHANGED_DELAY 200

FN_KbSwitchSetHooks KbSwitchSetHooks = NULL;
UINT ShellHookMessage = 0;

HINSTANCE g_hInst = NULL;
HMODULE   g_hHookDLL = NULL;
INT       g_nCurrentLayoutNum = 1;
HICON     g_hTrayIcon = NULL;
HWND      g_hwndLastActive = NULL;
INT       g_cKLs = 0;
HKL       g_ahKLs[64];

/* Debug logging */
ULONG NTAPI
vDbgPrintExWithPrefix(IN PCCH Prefix,
                      IN ULONG ComponentId,
                      IN ULONG Level,
                      IN PCCH Format,
                      IN va_list ap)
{
    CHAR Buffer[512];
    SIZE_T PrefixLength = strlen(Prefix);
    strncpy(Buffer, Prefix, PrefixLength);
    _vsnprintf(Buffer + PrefixLength, _countof(Buffer) - PrefixLength, Format, ap);
    Buffer[_countof(Buffer) - 1] = ANSI_NULL; /* Avoid buffer overrun */
    OutputDebugStringA(Buffer);
    return 0;
}

typedef struct tagSPECIAL_ID
{
    DWORD dwLayoutId;
    HKL hKL;
    TCHAR szKLID[CCH_LAYOUT_ID + 1];
} SPECIAL_ID, *PSPECIAL_ID;

#define MAX_SPECIAL_IDS 256

SPECIAL_ID g_SpecialIds[MAX_SPECIAL_IDS];
INT g_cSpecialIds = 0;

static VOID LoadSpecialIds(VOID)
{
    TCHAR szKLID[KL_NAMELENGTH], szLayoutId[16];
    DWORD dwSize, dwIndex;
    HKEY hKey, hLayoutKey;

    g_cSpecialIds = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts"),
                     0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    for (dwIndex = 0; dwIndex < 1000; ++dwIndex)
    {
        dwSize = _countof(szKLID);
        if (RegEnumKeyEx(hKey, dwIndex, szKLID, &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        if (RegOpenKeyEx(hKey, szKLID, 0, KEY_READ, &hLayoutKey) != ERROR_SUCCESS)
            continue;

        dwSize = sizeof(szLayoutId);
        if (RegQueryValueEx(hLayoutKey, TEXT("Layout Id"), NULL, NULL,
                            (LPBYTE)szLayoutId, &dwSize) == ERROR_SUCCESS)
        {
            DWORD dwKLID = _tcstoul(szKLID, NULL, 16);
            WORD wLangId = LOWORD(dwKLID), wLayoutId = LOWORD(_tcstoul(szLayoutId, NULL, 16));
            HKL hKL = (HKL)(LONG_PTR)(SPECIAL_MASK | MAKELONG(wLangId, wLayoutId));

            /* Add a special ID */
            g_SpecialIds[g_cSpecialIds].dwLayoutId = wLayoutId;
            g_SpecialIds[g_cSpecialIds].hKL = hKL;
            StringCchCopy(g_SpecialIds[g_cSpecialIds].szKLID,
                          _countof(g_SpecialIds[g_cSpecialIds].szKLID), szKLID);
            ++g_cSpecialIds;
        }

        RegCloseKey(hLayoutKey);

        if (g_cSpecialIds >= _countof(g_SpecialIds))
        {
            ERR("g_SpecialIds is full!");
            break;
        }
    }

    RegCloseKey(hKey);
}

static VOID
GetKLIDFromHKL(HKL hKL, LPTSTR szKLID, SIZE_T KLIDLength)
{
    szKLID[0] = 0;

    if (IS_IME_HKL(hKL))
    {
        StringCchPrintf(szKLID, KLIDLength, _T("%08lx"), (DWORD)(DWORD_PTR)hKL);
        return;
    }

    if (IS_SPECIAL_HKL(hKL))
    {
        INT i;
        for (i = 0; i < g_cSpecialIds; ++i)
        {
            if (g_SpecialIds[i].hKL == hKL)
            {
                StringCchCopy(szKLID, KLIDLength, g_SpecialIds[i].szKLID);
                return;
            }
        }
    }
    else
    {
        StringCchPrintf(szKLID, KLIDLength, _T("%08lx"), LOWORD(hKL));
    }
}

static HKL GetActiveKL(VOID)
{
    /* FIXME: Get correct console window's HKL when console window */
    HWND hwndTarget = (g_hwndLastActive ? g_hwndLastActive : GetForegroundWindow());
    DWORD dwTID = GetWindowThreadProcessId(hwndTarget, NULL);
    return GetKeyboardLayout(dwTID);
}

static VOID UpdateLayoutList(HKL hKL OPTIONAL)
{
    INT iKL;

    if (!hKL)
        hKL = GetActiveKL();

    g_cKLs = GetKeyboardLayoutList(_countof(g_ahKLs), g_ahKLs);

    g_nCurrentLayoutNum = -1;
    for (iKL = 0; iKL < g_cKLs; ++iKL)
    {
        if (g_ahKLs[iKL] == hKL)
        {
            g_nCurrentLayoutNum = iKL + 1;
            break;
        }
    }

    if (g_nCurrentLayoutNum == -1 && g_cKLs < _countof(g_ahKLs))
    {
        g_nCurrentLayoutNum = g_cKLs;
        g_ahKLs[g_cKLs++] = hKL;
    }
}

static HKL GetHKLFromLayoutNum(INT nLayoutNum)
{
    if (0 <= (nLayoutNum - 1) && (nLayoutNum - 1) < g_cKLs)
        return g_ahKLs[nLayoutNum - 1];
    else
        return GetActiveKL();
}

static VOID
GetKLIDFromLayoutNum(INT nLayoutNum, LPTSTR szKLID, SIZE_T KLIDLength)
{
    GetKLIDFromHKL(GetHKLFromLayoutNum(nLayoutNum), szKLID, KLIDLength);
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
GetLayoutName(INT nLayoutNum, LPTSTR szName, SIZE_T NameLength)
{
    HKEY hKey;
    HRESULT hr;
    DWORD dwBufLen;
    TCHAR szBuf[MAX_PATH], szKLID[CCH_LAYOUT_ID + 1];

    GetKLIDFromLayoutNum(nLayoutNum, szKLID, _countof(szKLID));

    StringCchPrintf(szBuf, _countof(szBuf),
                    _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), szKLID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;

    /* Use "Layout Display Name" value as an entry name if possible */
    hr = SHLoadRegUIString(hKey, _T("Layout Display Name"), szName, NameLength);
    if (SUCCEEDED(hr))
    {
        RegCloseKey(hKey);
        return TRUE;
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

static BOOL GetImeFile(LPTSTR szImeFile, SIZE_T cchImeFile, LPCTSTR szKLID)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[MAX_PATH];

    szImeFile[0] = UNICODE_NULL;

    if (_tcslen(szKLID) != CCH_LAYOUT_ID)
        return FALSE; /* Invalid LCID */

    if (szKLID[0] != TEXT('E') && szKLID[0] != TEXT('e'))
        return FALSE; /* Not an IME HKL */

    StringCchPrintf(szBuf, _countof(szBuf),
                    _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), szKLID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return FALSE;

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

static HBITMAP BitmapFromIcon(HICON hIcon)
{
    HDC hdcScreen = GetDC(NULL);
    HDC hdc = CreateCompatibleDC(hdcScreen);
    INT cxIcon = GetSystemMetrics(SM_CXSMICON);
    INT cyIcon = GetSystemMetrics(SM_CYSMICON);
    HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, cxIcon, cyIcon);
    HGDIOBJ hbmOld;

    if (hbm != NULL)
    {
        hbmOld = SelectObject(hdc, hbm);
        DrawIconEx(hdc, 0, 0, hIcon, cxIcon, cyIcon, 0, GetSysColorBrush(COLOR_MENU), DI_NORMAL);
        SelectObject(hdc, hbmOld);
    }

    DeleteDC(hdc);
    ReleaseDC(NULL, hdcScreen);
    return hbm;
}

static HICON
CreateTrayIcon(LPTSTR szKLID, LPCTSTR szImeFile OPTIONAL)
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
        if (GetSystemLibraryPath(szPath, _countof(szPath), szImeFile))
            return FakeExtractIcon(szPath, cxIcon, cyIcon);
    }

    /* Getting "EN", "FR", etc. from English, French, ... */
    LangID = LANGIDFROMLCID(_tcstoul(szKLID, NULL, 16));
    if (GetLocaleInfo(LangID,
                      LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                      szBuf,
                      _countof(szBuf)) == 0)
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
    TCHAR szKLID[CCH_LAYOUT_ID + 1], szName[MAX_PATH], szImeFile[80];

    GetKLIDFromLayoutNum(g_nCurrentLayoutNum, szKLID, _countof(szKLID));
    GetLayoutName(g_nCurrentLayoutNum, szName, _countof(szName));
    GetImeFile(szImeFile, _countof(szImeFile), szKLID);

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szKLID, szImeFile);
    StringCchCopy(tnid.szTip, _countof(tnid.szTip), szName);

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
UpdateTrayIcon(HWND hwnd, LPTSTR szKLID, LPTSTR szName)
{
    NOTIFYICONDATA tnid = { sizeof(tnid), hwnd, 1, NIF_ICON | NIF_MESSAGE | NIF_TIP };
    TCHAR szImeFile[80];

    GetImeFile(szImeFile, _countof(szImeFile), szKLID);

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szKLID, szImeFile);
    StringCchCopy(tnid.szTip, _countof(tnid.szTip), szName);

    Shell_NotifyIcon(NIM_MODIFY, &tnid);

    if (g_hTrayIcon)
        DestroyIcon(g_hTrayIcon);
    g_hTrayIcon = tnid.hIcon;
}

static BOOL CALLBACK
EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, INPUTLANGCHANGE_SYSCHARSET, lParam);
    return TRUE;
}

static VOID
ActivateLayout(HWND hwnd, ULONG uLayoutNum, HWND hwndTarget OPTIONAL, BOOL bNoActivate)
{
    HKL hKl;
    TCHAR szKLID[CCH_LAYOUT_ID + 1], szLangName[MAX_PATH];
    LANGID LangID;

    /* The layout number starts from one. Zero is invalid */
    if (uLayoutNum == 0 || uLayoutNum > 0xFF) /* Invalid */
        return;

    GetKLIDFromLayoutNum(uLayoutNum, szKLID, _countof(szKLID));
    LangID = (LANGID)_tcstoul(szKLID, NULL, 16);

    /* Switch to the new keyboard layout */
    GetLocaleInfo(LangID, LOCALE_SLANGUAGE, szLangName, _countof(szLangName));
    UpdateTrayIcon(hwnd, szKLID, szLangName);

    if (hwndTarget && !bNoActivate)
        SetForegroundWindow(hwndTarget);

    hKl = LoadKeyboardLayout(szKLID, KLF_ACTIVATE);
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

    g_nCurrentLayoutNum = uLayoutNum;
}

static HMENU
BuildLeftPopupMenu(VOID)
{
    HMENU hMenu = CreatePopupMenu();
    TCHAR szName[MAX_PATH], szKLID[CCH_LAYOUT_ID + 1], szImeFile[80];
    HICON hIcon;
    MENUITEMINFO mii = { sizeof(mii) };
    INT iKL;

    for (iKL = 0; iKL < g_cKLs; ++iKL)
    {
        GetKLIDFromHKL(g_ahKLs[iKL], szKLID, _countof(szKLID));
        GetImeFile(szImeFile, _countof(szImeFile), szKLID);

        if (!GetLayoutName(iKL + 1, szName, _countof(szName)))
            continue;

        mii.fMask       = MIIM_ID | MIIM_STRING;
        mii.wID         = iKL + 1;
        mii.dwTypeData  = szName;

        hIcon = CreateTrayIcon(szKLID, szImeFile);
        if (hIcon)
        {
            mii.hbmpItem = BitmapFromIcon(hIcon);
            if (mii.hbmpItem)
                mii.fMask |= MIIM_BITMAP;
        }

        InsertMenuItem(hMenu, -1, TRUE, &mii);
        DestroyIcon(hIcon);
    }

    CheckMenuItem(hMenu, g_nCurrentLayoutNum, MF_CHECKED);

    return hMenu;
}

BOOL
SetHooks(VOID)
{
    g_hHookDLL = LoadLibrary(_T("kbsdll.dll"));
    if (!g_hHookDLL)
    {
        return FALSE;
    }

#define IHOOK_SET 1
    KbSwitchSetHooks = (FN_KbSwitchSetHooks)GetProcAddress(g_hHookDLL, MAKEINTRESOURCEA(IHOOK_SET));

    if (!KbSwitchSetHooks || !KbSwitchSetHooks(TRUE))
    {
        ERR("SetHooks failed\n");
        return FALSE;
    }

    TRACE("SetHooks OK\n");
    return TRUE;
}

VOID
DeleteHooks(VOID)
{
    if (KbSwitchSetHooks)
    {
        KbSwitchSetHooks(FALSE);
        KbSwitchSetHooks = NULL;
    }

    if (g_hHookDLL)
    {
        FreeLibrary(g_hHookDLL);
        g_hHookDLL = NULL;
    }

    TRACE("DeleteHooks OK\n");
}

static UINT GetLayoutNum(HKL hKL)
{
    INT iKL;

    for (iKL = 0; iKL < g_cKLs; ++iKL)
    {
        if (g_ahKLs[iKL] == hKL)
            return iKL + 1;
    }

    return 0;
}

ULONG
GetNextLayout(VOID)
{
    return (g_nCurrentLayoutNum % g_cKLs) + 1;
}

UINT
UpdateLanguageDisplay(HWND hwnd, HKL hKL)
{
    TCHAR szKLID[MAX_PATH], szLangName[MAX_PATH];
    LANGID LangID;

    GetKLIDFromHKL(hKL, szKLID, _countof(szKLID));
    LangID = (LANGID)_tcstoul(szKLID, NULL, 16);
    GetLocaleInfo(LangID, LOCALE_SLANGUAGE, szLangName, _countof(szLangName));
    UpdateTrayIcon(hwnd, szKLID, szLangName);
    g_nCurrentLayoutNum = GetLayoutNum(hKL);

    return 0;
}

HWND
GetTargetWindow(HWND hwndFore OPTIONAL)
{
    HWND hwndTarget = (hwndFore ? hwndFore : GetForegroundWindow());
    if (IsWndClassName(hwndTarget, szKbSwitcherName))
        hwndTarget = g_hwndLastActive;
    return hwndTarget;
}

UINT
UpdateLanguageDisplayCurrent(HWND hwnd, HWND hwndFore)
{
    DWORD dwThreadID = GetWindowThreadProcessId(GetTargetWindow(hwndFore), NULL);
    HKL hKL = GetKeyboardLayout(dwThreadID);
    UpdateLanguageDisplay(hwnd, hKL);

    return 0;
}

static BOOL RememberLastActive(HWND hwnd, HWND hwndFore)
{
    hwndFore = GetAncestor(hwndFore, GA_ROOT);

    if (!IsWindowVisible(hwndFore))
        return FALSE;

    if (IsWndClassName(hwndFore, szKbSwitcherName) ||
        IsWndClassName(hwndFore, TEXT("Shell_TrayWnd")))
    {
        return FALSE; /* Special window */
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

            LoadSpecialIds();

            UpdateLayoutList(NULL);
            AddTrayIcon(hwnd);

            ActivateLayout(hwnd, g_nCurrentLayoutNum, NULL, TRUE);
            s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
            break;
        }

        case WM_TIMER:
        {
            if (wParam == TIMER_ID_LANG_CHANGED_DELAYED)
            {
                KillTimer(hwnd, TIMER_ID_LANG_CHANGED_DELAYED);
                HKL hKL = GetActiveKL();
                UpdateLayoutList(hKL);
                UpdateLanguageDisplay(hwnd, hKL);
            }
            break;
        }

        // WM_LANG_CHANGED message:
        //   wParam: HWND hwndTarget or zero
        //   lParam: HKL hKL or zero
        case WM_LANG_CHANGED: /* Comes from kbsdll.dll and this module */
        {
            TRACE("WM_LANG_CHANGED: wParam:%p, lParam:%p\n", wParam, lParam);
            /* Delayed action */
            KillTimer(hwnd, TIMER_ID_LANG_CHANGED_DELAYED);
            SetTimer(hwnd, TIMER_ID_LANG_CHANGED_DELAYED, TIMER_LANG_CHANGED_DELAY, NULL);
            break;
        }

        // WM_WINDOW_ACTIVATE message:
        //   wParam: HWND hwndTarget or zero
        //   lParam: zero
        case WM_WINDOW_ACTIVATE: /* Comes from kbsdll.dll and this module */
        {
            TRACE("WM_WINDOW_ACTIVATE: wParam:%p, lParam:%p\n", wParam, lParam);
            HWND hwndFore = wParam ? (HWND)wParam : GetForegroundWindow();
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
                    UpdateLayoutList(NULL);

                    GetCursorPos(&pt);
                    SetForegroundWindow(hwnd);

                    INT nID;
                    if (lParam == WM_LBUTTONUP)
                    {
                        /* Rebuild the left popup menu on every click to take care of keyboard layout changes */
                        hLeftPopupMenu = BuildLeftPopupMenu();
                        nID = TrackPopupMenu(hLeftPopupMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
                        DestroyMenu(hLeftPopupMenu);
                    }
                    else
                    {
                        if (!s_hRightPopupMenu)
                        {
                            s_hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_POPUP));
                            s_hRightPopupMenu = GetSubMenu(s_hMenu, 0);
                        }
                        nID = TrackPopupMenu(s_hRightPopupMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
                    }

                    PostMessage(hwnd, WM_NULL, 0, 0);

                    if (nID)
                        PostMessage(hwnd, WM_COMMAND, nID, 0);

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

                default:
                {
                    if (1 <= LOWORD(wParam) && LOWORD(wParam) <= 1000)
                    {
                        if (!IsWindow(g_hwndLastActive))
                        {
                            g_hwndLastActive = NULL;
                        }
                        ActivateLayout(hwnd, LOWORD(wParam), g_hwndLastActive, FALSE);
                    }
                    break;
                }
            }
            break;

        case WM_SETTINGCHANGE:
        {
            if (wParam == SPI_SETNONCLIENTMETRICS)
            {
                PostMessage(hwnd, WM_WINDOW_ACTIVATE, 0, 0);
                break;
            }
        }
        break;

        case WM_DESTROY:
        {
            KillTimer(hwnd, TIMER_ID_LANG_CHANGED_DELAYED);
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
                UpdateLayoutList(NULL);
                AddTrayIcon(hwnd);
                break;
            }
            else if (Message == ShellHookMessage)
            {
                TRACE("ShellHookMessage: wParam:%p, lParam:%p\n", wParam, lParam);
                if (wParam == HSHELL_LANGUAGE)
                    PostMessage(hwnd, WM_LANG_CHANGED, 0, 0);
                else if (wParam == HSHELL_WINDOWACTIVATED || wParam == HSHELL_RUDEAPPACTIVATED)
                    PostMessage(hwnd, WM_WINDOW_ACTIVATE, 0, 0);

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
            TRACE("LAYOUT_RTL\n");
            SetProcessDefaultLayout(LAYOUT_RTL);
            break;
        default:
            break;
    }

    hMutex = CreateMutex(NULL, FALSE, szKbSwitcherName);
    if (!hMutex)
    {
        ERR("!hMutex\n");
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        ERR("Another instance is already running\n");
        CloseHandle(hMutex);
        return 1;
    }

    g_hInst = hInstance;

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
    if (!RegisterShellHookWindow(hwnd))
    {
        ERR("RegisterShellHookWindow failed\n");
        DestroyWindow(hwnd);
        CloseHandle(hMutex);
        return 1;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(hMutex);
    return 0;
}
