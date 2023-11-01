/*
 * PROJECT:         Keyboard Layout Switcher
 * FILE:            base/applications/kbswitch/kbswitch.c
 * PURPOSE:         Switching Keyboard Layouts
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Colin Finck (mail@colinfinck.de)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "kbswitch.h"
#include <shlobj.h>
#include <shlwapi_undoc.h>
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

#define IME_MASK        (0xE0000000UL)
#define SPECIAL_MASK    (0xF0000000UL)

#define IS_IME_HKL(hKL)             ((((ULONG_PTR)(hKL)) & 0xF0000000) == IME_MASK)
#define IS_SPECIAL_HKL(hKL)         ((((ULONG_PTR)(hKL)) & 0xF0000000) == SPECIAL_MASK)
#define SPECIALIDFROMHKL(hKL)       ((WORD)(HIWORD(hKL) & 0x0FFF))

#define WM_NOTIFYICONMSG (WM_USER + 248)

PKBSWITCHSETHOOKS    KbSwitchSetHooks    = NULL;
PKBSWITCHDELETEHOOKS KbSwitchDeleteHooks = NULL;
UINT ShellHookMessage = 0;

HINSTANCE hInst;
HANDLE    hProcessHeap;
HMODULE   g_hHookDLL = NULL;
INT       g_nCurrentLayoutNum = 1;
HICON     g_hTrayIcon = NULL;
HWND      g_hwndLastActive = NULL;
INT       g_cKLs = 0;
HKL       g_ahKLs[64];

typedef struct
{
    DWORD dwLayoutId;
    HKL hKL;
    TCHAR szKLID[CCH_LAYOUT_ID + 1];
} SPECIAL_ID, *PSPECIAL_ID;

SPECIAL_ID g_SpecialIds[80];
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
        dwSize = ARRAYSIZE(szKLID);
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
                          ARRAYSIZE(g_SpecialIds[g_cSpecialIds].szKLID), szKLID);
            ++g_cSpecialIds;
        }

        RegCloseKey(hLayoutKey);

        if (g_cSpecialIds >= ARRAYSIZE(g_SpecialIds))
        {
            OutputDebugStringA("g_SpecialIds is full!");
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

static VOID UpdateLayoutList(HKL hKL OPTIONAL)
{
    INT iKL;

    if (!hKL)
    {
        HWND hwndTarget = (g_hwndLastActive ? g_hwndLastActive : GetForegroundWindow());
        DWORD dwTID = GetWindowThreadProcessId(hwndTarget, NULL);
        hKL = GetKeyboardLayout(dwTID);
    }

    g_cKLs = GetKeyboardLayoutList(ARRAYSIZE(g_ahKLs), g_ahKLs);

    g_nCurrentLayoutNum = -1;
    for (iKL = 0; iKL < g_cKLs; ++iKL)
    {
        if (g_ahKLs[iKL] == hKL)
        {
            g_nCurrentLayoutNum = iKL + 1;
            break;
        }
    }

    if (g_nCurrentLayoutNum == -1 && g_cKLs < ARRAYSIZE(g_ahKLs))
    {
        g_nCurrentLayoutNum = g_cKLs;
        g_ahKLs[g_cKLs++] = hKL;
    }
}

static HKL GetHKLFromLayoutNum(INT nLayoutNum)
{
    if (0 <= (nLayoutNum - 1) && (nLayoutNum - 1) < g_cKLs)
    {
        return g_ahKLs[nLayoutNum - 1];
    }
    else
    {
        HWND hwndTarget = (g_hwndLastActive ? g_hwndLastActive : GetForegroundWindow());
        DWORD dwTID = GetWindowThreadProcessId(hwndTarget, NULL);
        return GetKeyboardLayout(dwTID);
    }
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

    GetKLIDFromLayoutNum(nLayoutNum, szKLID, ARRAYSIZE(szKLID));

    StringCchPrintf(szBuf, ARRAYSIZE(szBuf),
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

    StringCchPrintf(szBuf, ARRAYSIZE(szBuf),
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
        if (GetSystemLibraryPath(szPath, ARRAYSIZE(szPath), szImeFile))
            return FakeExtractIcon(szPath, cxIcon, cyIcon);
    }

    /* Getting "EN", "FR", etc. from English, French, ... */
    LangID = LANGIDFROMLCID(_tcstoul(szKLID, NULL, 16));
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
    TCHAR szKLID[CCH_LAYOUT_ID + 1], szName[MAX_PATH], szImeFile[80];

    GetKLIDFromLayoutNum(g_nCurrentLayoutNum, szKLID, ARRAYSIZE(szKLID));
    GetLayoutName(g_nCurrentLayoutNum, szName, ARRAYSIZE(szName));
    GetImeFile(szImeFile, ARRAYSIZE(szImeFile), szKLID);

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szKLID, szImeFile);
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
UpdateTrayIcon(HWND hwnd, LPTSTR szKLID, LPTSTR szName)
{
    NOTIFYICONDATA tnid = { sizeof(tnid), hwnd, 1, NIF_ICON | NIF_MESSAGE | NIF_TIP };
    TCHAR szImeFile[80];

    GetImeFile(szImeFile, ARRAYSIZE(szImeFile), szKLID);

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szKLID, szImeFile);
    StringCchCopy(tnid.szTip, ARRAYSIZE(tnid.szTip), szName);

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

    GetKLIDFromLayoutNum(uLayoutNum, szKLID, ARRAYSIZE(szKLID));
    LangID = (LANGID)_tcstoul(szKLID, NULL, 16);

    /* Switch to the new keyboard layout */
    GetLocaleInfo(LangID, LOCALE_SLANGUAGE, szLangName, ARRAYSIZE(szLangName));
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
        GetKLIDFromHKL(g_ahKLs[iKL], szKLID, ARRAYSIZE(szKLID));
        GetImeFile(szImeFile, ARRAYSIZE(szImeFile), szKLID);

        if (!GetLayoutName(iKL + 1, szName, ARRAYSIZE(szName)))
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

    GetKLIDFromHKL(hKL, szKLID, ARRAYSIZE(szKLID));
    LangID = (LANGID)_tcstoul(szKLID, NULL, 16);
    GetLocaleInfo(LangID, LOCALE_SLANGUAGE, szLangName, ARRAYSIZE(szLangName));
    UpdateTrayIcon(hwnd, szKLID, szLangName);
    g_nCurrentLayoutNum = GetLayoutNum(hKL);

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

    /* FIXME: CONWND needs special handling */
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

            LoadSpecialIds();

            UpdateLayoutList(NULL);
            AddTrayIcon(hwnd);

            ActivateLayout(hwnd, g_nCurrentLayoutNum, NULL, TRUE);
            s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
            break;
        }

        case WM_LANG_CHANGED: /* Comes from kbsdll.dll and this module */
        {
            UpdateLayoutList((HKL)lParam);
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
                    UpdateLayoutList(NULL);

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

                    /* FIXME: CONWND needs special handling */
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
                        uNum = GetLayoutNum(hKL);
                        if (uNum != 0)
                            g_nCurrentLayoutNum = uNum;
                    }

                    ActivateLayout(hwnd, GetNextLayout(), hwndTarget, TRUE);

                    /* FIXME: CONWND needs special handling */
                    if (bCONWND)
                        ActivateLayout(hwnd, g_nCurrentLayoutNum, hwndTargetSave, TRUE);

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
                UpdateLayoutList(NULL);
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
