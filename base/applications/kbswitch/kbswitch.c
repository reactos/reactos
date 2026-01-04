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
#include <undocuser.h>
#include <imm.h>
#include <immdev.h>
#include <imm32_undoc.h>
#include <assert.h>
#include "imemenu.h"

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
 * We use global hook by our indicdll.dll, to watch the shell and the windows.
 *
 * It might not work correctly on Vista+ because keyboard layout change notification
 * won't be generated in Vista+.
 */

#define WM_NOTIFYICONMSG 0x8064
#define WM_PENICONMSG 0x8065

#define NOTIFY_ICON_ID_LANGUAGE 223
#define NOTIFY_ICON_ID_SYSTEM_PEN 224

#define TIMER_ID_LANG_CHANGED_DELAYED 0x10000
#define TIMER_LANG_CHANGED_DELAY 200

#define IME_HKL_MASK 0xF000FFFF
#define IS_KOREAN_IME_HKL(hKL) ((HandleToUlong(hKL) & IME_HKL_MASK) == 0xE0000412)

#define MAX_KLS 64

typedef BOOL (APIENTRY *FN_KbSwitchSetHooks)(BOOL bDoHook);
typedef VOID (APIENTRY *FN_SetPenMenuData)(UINT nID, DWORD_PTR dwItemData); // indic!14

FN_KbSwitchSetHooks KbSwitchSetHooks = NULL;
FN_SetPenMenuData SetPenMenuData = NULL;

HINSTANCE g_hInst = NULL;
HMODULE   g_hHookDLL = NULL;
HICON     g_hTrayIcon = NULL;
HWND      g_hwndLastActive = NULL;
UINT      g_iKL = 0;
UINT      g_cKLs = 0;
HKL       g_ahKLs[MAX_KLS] = { NULL };
WORD      g_aiSysPenIcons[MAX_KLS] = { 0 };
WORD      g_anToolTipAtoms[MAX_KLS] = { 0 };
HICON     g_ahSysPenIcons[MAX_KLS] = { NULL };
BOOL      g_bSysPenNotifyAdded = FALSE;
BYTE      g_anFlags[MAX_KLS] = { 0 };
UINT      g_uTaskbarRestartMsg = 0;
UINT      g_uShellHookMessage = 0;
HWND      g_hTrayWnd = NULL;
HWND      g_hTrayNotifyWnd = NULL;

// Flags for g_anFlags
#define LAYOUTF_FAR_EAST 0x1
#define LAYOUTF_IME_ICON 0x2
#define LAYOUTF_TOOLTIP_ATOM 0x4
#define LAYOUTF_REMOVE_LEFT_DEF_MENU 0x8
#define LAYOUTF_REMOVE_RIGHT_DEF_MENU 0x10

static VOID
UpdateTrayInfo(VOID)
{
    g_hTrayWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    g_hTrayNotifyWnd = FindWindowEx(g_hTrayWnd, NULL, TEXT("TrayNotifyWnd"), NULL);
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

static HWND
GetTargetWindow(HWND hwndFore OPTIONAL)
{
    HWND hwndTarget = (hwndFore ? hwndFore : GetForegroundWindow());
    if (!hwndTarget ||
        IsWndClassName(hwndTarget, INDICATOR_CLASS) ||
        IsWndClassName(hwndTarget, TEXT("Shell_TrayWnd")))
    {
        hwndTarget = g_hwndLastActive;
    }
    return hwndTarget;
}

static HKL GetActiveKL(VOID)
{
    /* FIXME: Get correct console window's HKL when console window */
    HWND hwndTarget = GetTargetWindow(NULL);
    TRACE("hwndTarget: %p\n", hwndTarget);
    DWORD dwTID = GetWindowThreadProcessId(hwndTarget, NULL);
    return GetKeyboardLayout(dwTID);
}

static VOID
DeletePenIcon(HWND hwnd, UINT iKL)
{
    UNREFERENCED_PARAMETER(hwnd);

    if (g_ahSysPenIcons[iKL])
    {
        DestroyIcon(g_ahSysPenIcons[iKL]);
        g_ahSysPenIcons[iKL] = NULL;
    }
}

static VOID
DestroyPenIcons(VOID)
{
    INT iKL;
    for (iKL = 0; iKL < g_cKLs; ++iKL)
    {
        if (g_ahSysPenIcons[iKL])
        {
            DestroyIcon(g_ahSysPenIcons[iKL]);
            g_ahSysPenIcons[iKL] = NULL;
        }
    }
}

static UINT
GetLayoutIndexFromHKL(HKL hKL)
{
    for (UINT iKL = 0; iKL < g_cKLs; ++iKL)
    {
        if (g_ahKLs[iKL] == hKL)
            return iKL;
    }
    return 0;
}

static VOID UpdateLayoutList(HKL hKL OPTIONAL)
{
    if (!hKL)
        hKL = GetActiveKL();

    HICON ahSysPenIcons[MAX_KLS];
    WORD aiSysPenIcons[MAX_KLS];
    BYTE anFlags[MAX_KLS];
    ZeroMemory(ahSysPenIcons, sizeof(ahSysPenIcons));
    FillMemory(aiSysPenIcons, sizeof(aiSysPenIcons), 0xFF);
    ZeroMemory(anFlags, sizeof(anFlags));

    HKL ahKLs[MAX_KLS];
    UINT iKL, nKLs = GetKeyboardLayoutList(_countof(ahKLs), ahKLs);

    /* Reuse old icons and flags */
    for (iKL = 0; iKL < nKLs; ++iKL)
    {
        LANGID wLangID = LOWORD(ahKLs[iKL]);
        switch (wLangID)
        {
            case LANGID_CHINESE_SIMPLIFIED:
            case LANGID_CHINESE_TRADITIONAL:
            case LANGID_JAPANESE:
            case LANGID_KOREAN:
                anFlags[iKL] |= LAYOUTF_FAR_EAST;
                break;
            default:
                anFlags[iKL] &= ~LAYOUTF_FAR_EAST;
                break;
        }

        UINT iKL2;
        for (iKL2 = 0; iKL2 < g_cKLs; ++iKL2)
        {
            if (ahKLs[iKL] == g_ahKLs[iKL2])
            {
                ahSysPenIcons[iKL] = g_ahSysPenIcons[iKL2];
                aiSysPenIcons[iKL] = g_aiSysPenIcons[iKL2];
                anFlags[iKL] = g_anFlags[iKL2];

                g_ahSysPenIcons[iKL2] = NULL;
                break;
            }
        }
    }

    DestroyPenIcons();

    g_cKLs = nKLs;

    C_ASSERT(sizeof(g_ahKLs) == sizeof(ahKLs));
    CopyMemory(g_ahKLs, ahKLs, sizeof(g_ahKLs));

    C_ASSERT(sizeof(g_ahSysPenIcons) == sizeof(ahSysPenIcons));
    CopyMemory(g_ahSysPenIcons, ahSysPenIcons, sizeof(g_ahSysPenIcons));

    C_ASSERT(sizeof(g_anFlags) == sizeof(anFlags));
    CopyMemory(g_anFlags, anFlags, sizeof(g_anFlags));

    g_iKL = GetLayoutIndexFromHKL(hKL);
}

static HKL GetHKLFromLayoutNum(UINT iKL)
{
    return (iKL < g_cKLs) ? g_ahKLs[iKL] : GetActiveKL();
}

static VOID
GetKLIDFromLayoutNum(UINT iKL, LPTSTR szKLID, SIZE_T KLIDLength)
{
    GetKLIDFromHKL(GetHKLFromLayoutNum(iKL), szKLID, KLIDLength);
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
GetLayoutName(UINT iKL, LPTSTR szName, SIZE_T NameLength)
{
    HKEY hKey;
    HRESULT hr;
    DWORD dwBufLen;
    TCHAR szBuf[MAX_PATH], szKLID[CCH_LAYOUT_ID + 1];

    GetKLIDFromLayoutNum(iKL, szKLID, _countof(szKLID));

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
    INT nIconIndex;
    INT cxIcon, cyIcon;
    INT iIcon;
    HICON hIcon;
} LOAD_ICON, *PLOAD_ICON;

static BOOL CALLBACK
EnumResNameProc(
    HMODULE hModule,
    LPCTSTR lpszType,
    LPTSTR lpszName,
    LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lpszType);

    PLOAD_ICON pLoadIcon = (PLOAD_ICON)lParam;
    if (pLoadIcon->iIcon == pLoadIcon->nIconIndex)
    {
        pLoadIcon->hIcon = (HICON)LoadImage(hModule, lpszName, IMAGE_ICON,
                                            pLoadIcon->cxIcon, pLoadIcon->cyIcon,
                                            LR_DEFAULTCOLOR);
        if (pLoadIcon->hIcon)
            return FALSE; /* Stop enumeration */
    }

    ++pLoadIcon->iIcon;
    return TRUE;
}

static HICON
FakeExtractIcon(PCTSTR pszImeFile, INT nIconIndex)
{
    HMODULE hImeDLL = LoadLibraryEx(pszImeFile, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!hImeDLL)
        return NULL;

    LOAD_ICON LoadIcon =
    {
        nIconIndex, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CXSMICON)
    };
    EnumResourceNames(hImeDLL, RT_GROUP_ICON, EnumResNameProc, (LPARAM)&LoadIcon);

    FreeLibrary(hImeDLL);

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

static DWORD
GetImeStatus(HWND hwndTarget)
{
    HWND hwndIme = ImmGetDefaultIMEWnd(hwndTarget);
    if (!hwndIme)
        return IME_STATUS_IME_CLOSED;

    HIMC hIMC = (HIMC)SendMessage(hwndIme, WM_IME_SYSTEM, IMS_GETCONTEXT, (LPARAM)hwndTarget);
    if (!hIMC)
        return IME_STATUS_NO_IME;

    DWORD dwImeStatus = (ImmGetOpenStatus(hIMC) ? IME_STATUS_IME_OPEN : IME_STATUS_IME_CLOSED);
    if (GetACP() == 949) // Korean
    {
        DWORD dwConversion = 0, dwSentence = 0;
        if (ImmGetConversionStatus(hIMC, &dwConversion, &dwSentence))
        {
            if (dwConversion & IME_CMODE_NATIVE)
                dwImeStatus |= IME_STATUS_IME_NATIVE;

            if (dwConversion & IME_CMODE_FULLSHAPE)
                dwImeStatus |= IME_STATUS_IME_FULLSHAPE;
        }
    }

    return dwImeStatus;
}

static HICON
LoadDefaultPenIcon(PCWSTR szImeFile, HKL hKL)
{
    HWND hwndTarget = g_hwndLastActive ? g_hwndLastActive : GetForegroundWindow();
    DWORD dwImeStatus = GetImeStatus(hwndTarget);

    INT nIconID = -1;
    if (IS_KOREAN_IME_HKL(hKL)) // Korean IME?
    {
        if (dwImeStatus != IME_STATUS_NO_IME)
        {
            if (dwImeStatus & IME_STATUS_IME_CLOSED)
            {
                nIconID = IDI_KOREAN_A_HALF;
            }
            else
            {
                if (dwImeStatus & IME_STATUS_IME_FULLSHAPE)
                {
                    if (dwImeStatus & IME_STATUS_IME_NATIVE)
                        nIconID = IDI_KOREAN_JR_FULL;
                    else
                        nIconID = IDI_KOREAN_A_FULL;
                }
                else
                {
                    if (dwImeStatus & IME_STATUS_IME_NATIVE)
                        nIconID = IDI_KOREAN_JR_HALF;
                    else
                        nIconID = IDI_KOREAN_A_HALF;
                }
            }
        }
    }
    else
    {
        if (dwImeStatus & IME_STATUS_IME_CLOSED)
            nIconID = IDI_IME_CLOSED;
        else if (dwImeStatus & IME_STATUS_IME_OPEN)
            nIconID = IDI_IME_OPEN;
        else
            nIconID = IDI_IME_DISABLED;
    }

    if (nIconID < 0)
        return NULL;

    return LoadIcon(g_hHookDLL, MAKEINTRESOURCE(nIconID));
}

static VOID
DeletePenNotifyIcon(HWND hwnd)
{
    if (!g_bSysPenNotifyAdded)
        return;

    NOTIFYICONDATA nid = { sizeof(nid), hwnd, NOTIFY_ICON_ID_SYSTEM_PEN };
    if (!Shell_NotifyIcon(NIM_DELETE, &nid))
        ERR("Shell_NotifyIcon(NIM_DELETE) failed\n");
    else
        g_bSysPenNotifyAdded = FALSE;
}

static VOID
UpdatePenIcon(HWND hwnd, UINT iKL)
{
    DeletePenIcon(hwnd, iKL);

    // Not Far-East?
    if (!(g_anFlags[iKL] & LAYOUTF_FAR_EAST))
    {
        DeletePenNotifyIcon(hwnd);
        return;
    }

    // Get IME file
    TCHAR szKLID[CCH_LAYOUT_ID + 1], szImeFile[MAX_PATH];
    GetKLIDFromHKL(g_ahKLs[iKL], szKLID, _countof(szKLID));
    if (!GetImeFile(szImeFile, _countof(szImeFile), szKLID))
    {
        DeletePenNotifyIcon(hwnd);
        return;
    }

    // Load pen icon
    assert(!g_ahSysPenIcons[iKL]);
    if (g_anFlags[iKL] & LAYOUTF_IME_ICON)
        g_ahSysPenIcons[iKL] = FakeExtractIcon(szImeFile, g_aiSysPenIcons[iKL]);
    if (!g_ahSysPenIcons[iKL])
        g_ahSysPenIcons[iKL] = LoadDefaultPenIcon(szImeFile, g_ahKLs[iKL]);
    if (!g_ahSysPenIcons[iKL])
    {
        DeletePenNotifyIcon(hwnd);
        return;
    }

    // Add pen icon
    NOTIFYICONDATA nid = { sizeof(nid), hwnd, NOTIFY_ICON_ID_SYSTEM_PEN };
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_PENICONMSG;
    nid.hIcon = g_ahSysPenIcons[iKL];

    if (g_anToolTipAtoms[iKL])
        GlobalGetAtomName(g_anToolTipAtoms[iKL], nid.szTip, _countof(nid.szTip));
    else
        ImmGetDescription(g_ahKLs[iKL], nid.szTip, _countof(nid.szTip));

    if (!Shell_NotifyIcon((g_bSysPenNotifyAdded ? NIM_MODIFY : NIM_ADD), &nid))
        ERR("Shell_NotifyIcon failed\n");
    else
        g_bSysPenNotifyAdded = TRUE;
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
            return FakeExtractIcon(szPath, 0);
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
    NOTIFYICONDATA tnid =
    {
        sizeof(tnid), hwnd, NOTIFY_ICON_ID_LANGUAGE, NIF_ICON | NIF_MESSAGE | NIF_TIP
    };
    TCHAR szKLID[CCH_LAYOUT_ID + 1], szName[MAX_PATH], szImeFile[80];

    GetKLIDFromLayoutNum(g_iKL, szKLID, _countof(szKLID));
    GetLayoutName(g_iKL, szName, _countof(szName));
    GetImeFile(szImeFile, _countof(szImeFile), szKLID);

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(szKLID, szImeFile);
    StringCchCopy(tnid.szTip, _countof(tnid.szTip), szName);

    if (!Shell_NotifyIcon(NIM_ADD, &tnid))
        ERR("Shell_NotifyIcon(NIM_ADD) failed\n");

    if (g_hTrayIcon)
        DestroyIcon(g_hTrayIcon);
    g_hTrayIcon = tnid.hIcon;
}

static VOID
DeleteTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA tnid = { sizeof(tnid), hwnd, NOTIFY_ICON_ID_LANGUAGE };
    if (!Shell_NotifyIcon(NIM_DELETE, &tnid))
        ERR("Shell_NotifyIcon(NIM_DELETE) failed\n");

    if (g_hTrayIcon)
    {
        DestroyIcon(g_hTrayIcon);
        g_hTrayIcon = NULL;
    }
}

static VOID
UpdateTrayIcon(HWND hwnd, LPTSTR szKLID, LPTSTR szName)
{
    NOTIFYICONDATA tnid =
    {
        sizeof(tnid), hwnd, NOTIFY_ICON_ID_LANGUAGE, NIF_ICON | NIF_MESSAGE | NIF_TIP
    };
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
ActivateLayout(HWND hwnd, UINT iKL, HWND hwndTarget OPTIONAL, BOOL bNoActivate)
{
    HKL hKl;
    TCHAR szKLID[CCH_LAYOUT_ID + 1], szLangName[MAX_PATH];
    LANGID LangID;

    if (iKL >= g_cKLs) /* Invalid */
        return;

    GetKLIDFromLayoutNum(iKL, szKLID, _countof(szKLID));
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

    g_iKL = iKL;
}

static HMENU
BuildLeftPopupMenu(VOID)
{
    HMENU hMenu = CreatePopupMenu();
    TCHAR szName[MAX_PATH], szKLID[CCH_LAYOUT_ID + 1], szImeFile[80];
    HICON hIcon;
    MENUITEMINFO mii = { sizeof(mii) };
    UINT iKL;

    for (iKL = 0; iKL < g_cKLs; ++iKL)
    {
        GetKLIDFromHKL(g_ahKLs[iKL], szKLID, _countof(szKLID));
        GetImeFile(szImeFile, _countof(szImeFile), szKLID);

        if (!GetLayoutName(iKL, szName, _countof(szName)))
            continue;

        mii.fMask       = MIIM_ID | MIIM_STRING;
        mii.wID         = ID_LANG_BASE + iKL;
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

    CheckMenuItem(hMenu, ID_LANG_BASE + g_iKL, MF_CHECKED);

    return hMenu;
}

#define IFN_KbSwitchSetHooks 1
#define IFN_SetPenMenuData 14

static BOOL
SetHooks(VOID)
{
    g_hHookDLL = LoadLibrary(_T("indicdll.dll"));
    if (!g_hHookDLL)
        return FALSE;

    KbSwitchSetHooks =
        (FN_KbSwitchSetHooks)GetProcAddress(g_hHookDLL, MAKEINTRESOURCEA(IFN_KbSwitchSetHooks));
    SetPenMenuData =
        (FN_SetPenMenuData)GetProcAddress(g_hHookDLL, MAKEINTRESOURCEA(IFN_SetPenMenuData));

    if (!KbSwitchSetHooks || !SetPenMenuData || !KbSwitchSetHooks(TRUE))
    {
        ERR("SetHooks failed\n");
        return FALSE;
    }

    TRACE("SetHooks OK\n");
    return TRUE;
}

static VOID
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
    UINT iKL;

    for (iKL = 0; iKL < g_cKLs; ++iKL)
    {
        if (g_ahKLs[iKL] == hKL)
            return iKL;
    }

    return 0;
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
    g_iKL = GetLayoutNum(hKL);

    return 0;
}

UINT
UpdateLanguageDisplayCurrent(HWND hwnd, HWND hwndFore)
{
    UpdateLayoutList(NULL);
    DWORD dwThreadID = GetWindowThreadProcessId(GetTargetWindow(hwndFore), NULL);
    HKL hKL = GetKeyboardLayout(dwThreadID);
    UpdateLanguageDisplay(hwnd, hKL);
    UpdatePenIcon(hwnd, g_iKL);
    return 0;
}

static BOOL RememberLastActive(HWND hwnd, HWND hwndFore)
{
    hwndFore = GetAncestor(hwndFore, GA_ROOT);

    if (!IsWindowVisible(hwndFore))
        return FALSE;

    if (IsWndClassName(hwndFore, INDICATOR_CLASS) ||
        IsWndClassName(hwndFore, TEXT("Shell_TrayWnd")))
    {
        return FALSE; /* Special window */
    }

    g_hwndLastActive = hwndFore;
    return TRUE;
}

// WM_CREATE
static INT
KbSwitch_OnCreate(HWND hwnd)
{
    if (!SetHooks())
    {
        MessageBox(NULL, TEXT("SetHooks failed."), NULL, MB_ICONERROR);
        return -1; /* Failed */
    }

    LoadSpecialIds();
    UpdateTrayInfo();
    UpdateLayoutList(NULL);
    AddTrayIcon(hwnd);
    UpdatePenIcon(hwnd, g_iKL);

    ActivateLayout(hwnd, g_iKL, NULL, TRUE);
    g_uTaskbarRestartMsg = RegisterWindowMessage(TEXT("TaskbarCreated"));

    return 0; /* Success */
}

// WM_DESTROY
static void
KbSwitch_OnDestroy(HWND hwnd)
{
    KillTimer(hwnd, TIMER_ID_LANG_CHANGED_DELAYED);
    DeleteHooks();
    DeleteTrayIcon(hwnd);
    DestroyPenIcons();
    DeletePenNotifyIcon(hwnd);
    PostQuitMessage(0);
}

// WM_TIMER
static void
KbSwitch_OnTimer(HWND hwnd, UINT_PTR nTimerID)
{
    if (nTimerID == TIMER_ID_LANG_CHANGED_DELAYED)
    {
        KillTimer(hwnd, nTimerID);
        HKL hKL = GetActiveKL();
        UpdateLayoutList(hKL);
        UpdateLanguageDisplay(hwnd, hKL);
        UpdatePenIcon(hwnd, g_iKL);
    }
}

// WM_NOTIFYICONMSG
static void
KbSwitch_OnNotifyIconMsg(HWND hwnd, UINT uMouseMsg)
{
    if (uMouseMsg != WM_LBUTTONUP && uMouseMsg != WM_RBUTTONUP)
        return;

    UpdateLayoutList(NULL);

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(hwnd);

    TPMPARAMS params = { sizeof(params) };
    GetWindowRect(g_hTrayNotifyWnd, &params.rcExclude);

    INT nID;
    if (uMouseMsg == WM_LBUTTONUP)
    {
        /* Rebuild the left popup menu on every click to take care of keyboard layout changes */
        HMENU hPopupMenu = BuildLeftPopupMenu();
        UINT uFlags = TPM_VERTICAL | TPM_RIGHTALIGN | TPM_RETURNCMD | TPM_LEFTBUTTON;
        nID = TrackPopupMenuEx(hPopupMenu, uFlags, pt.x, pt.y, hwnd, &params);
        DestroyMenu(hPopupMenu);
    }
    else /* WM_RBUTTONUP */
    {
        HMENU hPopupMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_POPUP));
        HMENU hSubMenu = GetSubMenu(hPopupMenu, 0);
        UINT uFlags = TPM_VERTICAL | TPM_RIGHTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON;
        nID = TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, &params);
        DestroyMenu(hPopupMenu);
    }

    PostMessage(hwnd, WM_NULL, 0, 0);

    if (nID)
        PostMessage(hwnd, WM_COMMAND, nID, 0);
}

static BOOL
IsRegImeToolbarShown(VOID)
{
    HKEY hKey;
    LSTATUS error = RegOpenKey(HKEY_CURRENT_USER, TEXT("Control Panel\\Input Method"), &hKey);
    if (error)
    {
        ERR("Cannot open regkey: 0x%lX\n", error);
        return TRUE;
    }

    WCHAR szText[8];
    DWORD cbValue = sizeof(szText);
    error = RegQueryValueEx(hKey, TEXT("Show Status"), NULL, NULL, (PBYTE)szText, &cbValue);
    if (error)
    {
        RegCloseKey(hKey);
        return TRUE;
    }

    BOOL ret = !!_wtoi(szText);
    RegCloseKey(hKey);
    return ret;
}

static VOID
ShowImeToolbar(HWND hwndTarget, BOOL bShowToolbar)
{
    HKEY hKey;
    LSTATUS error = RegOpenKey(HKEY_CURRENT_USER, TEXT("Control Panel\\Input Method"), &hKey);
    if (error)
    {
        ERR("Cannot open regkey: 0x%lX\n", error);
        return;
    }

    WCHAR szText[8];
    StringCchPrintf(szText, _countof(szText), TEXT("%d"), bShowToolbar);

    DWORD cbValue = (lstrlen(szText) + 1) * sizeof(TCHAR);
    RegSetValueEx(hKey, TEXT("Show Status"), 0, REG_SZ, (PBYTE)szText, cbValue);
    RegCloseKey(hKey);

    HWND hwndIme = ImmGetDefaultIMEWnd(hwndTarget);
    PostMessage(hwndIme, WM_IME_SYSTEM, IMS_NOTIFYIMESHOW, bShowToolbar);
}

// WM_PENICONMSG
static VOID
KbSwitch_OnPenIconMsg(HWND hwnd, UINT uMouseMsg)
{
    if (uMouseMsg != WM_LBUTTONUP && uMouseMsg != WM_RBUTTONUP)
        return;

    if (!(g_anFlags[g_iKL] & LAYOUTF_FAR_EAST))
        return;

    POINT pt;
    GetCursorPos(&pt);

    // Get target window
    TRACE("g_hwndLastActive: %p\n", g_hwndLastActive);
    HWND hwndTarget = GetTargetWindow(g_hwndLastActive);
    TRACE("hwndTarget: %p\n", hwndTarget);

    // Get default IME window
    HWND hwndIme = ImmGetDefaultIMEWnd(hwndTarget);
    if (!hwndIme)
    {
        WARN("No default IME\n");
        return;
    }

    // Get IME context from another process
    HIMC hIMC = (HIMC)SendMessage(hwndIme, WM_IME_SYSTEM, IMS_GETCONTEXT, (LPARAM)hwndTarget);
    if (!hIMC)
    {
        WARN("No HIMC\n");
        return;
    }

    // Workaround of TrackPopupMenu's bug
    SetForegroundWindow(hwnd);

    // Create IME menu
    BOOL bRightButton = (uMouseMsg == WM_RBUTTONUP);
    PIMEMENUNODE pImeMenu = CreateImeMenu(hIMC, NULL, bRightButton);
    HMENU hMenu = MenuFromImeMenu(pImeMenu);

    HKL hKL = g_ahKLs[g_iKL];
    DWORD dwImeStatus = GetImeStatus(hwndTarget);
    BOOL bImeOn = FALSE, bSoftOn = FALSE, bShowToolbar = FALSE;
    TCHAR szText[128];
    if (bRightButton)
    {
        if (!(g_anFlags[g_iKL] & LAYOUTF_REMOVE_RIGHT_DEF_MENU)) // Add default menu items?
        {
            if (GetMenuItemCount(hMenu))
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL); // Separator

            // "Input System (IME) configuration..."
            LoadString(g_hInst, IDS_INPUTSYSTEM, szText, _countof(szText));
            AppendMenu(hMenu, MF_STRING, ID_INPUTSYSTEM, szText);
        }
    }
    else
    {
        if (!(g_anFlags[g_iKL] & LAYOUTF_REMOVE_LEFT_DEF_MENU)) // Add default menu items?
        {
            if (GetMenuItemCount(hMenu))
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL); // Separator

            if (!IS_KOREAN_IME_HKL(hKL)) // Not Korean IME?
            {
                // "IME ON / OFF"
                bImeOn = (dwImeStatus == IME_STATUS_IME_OPEN);
                UINT nId = (bImeOn ? IDS_IME_ON : IDS_IME_OFF);
                LoadString(g_hInst, nId, szText, _countof(szText));
                AppendMenu(hMenu, MF_STRING, ID_IMEONOFF, szText);
            }

            if ((ImmGetProperty(hKL, IGP_CONVERSION) & IME_CMODE_SOFTKBD) &&
                IsWindow(hwndIme)) // Is Soft Keyboard available?
            {
                // "Soft Keyboard ON / OFF"
                bSoftOn = (SendMessage(hwndIme, WM_IME_SYSTEM, IMS_GETCONVSTATUS, 0) & IME_CMODE_SOFTKBD);
                UINT nId = (bSoftOn ? IDS_SOFTKBD_ON : IDS_SOFTKBD_OFF);
                LoadString(g_hInst, nId, szText, _countof(szText));
                AppendMenu(hMenu, MF_STRING, ID_SOFTKBDONOFF, szText);
            }

            if (GetMenuItemCount(hMenu))
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL); // Separator

            // "Show toolbar"
            LoadString(g_hInst, IDS_SHOWTOOLBAR, szText, _countof(szText));
            AppendMenu(hMenu, MF_STRING, ID_SHOWTOOLBAR, szText);
            bShowToolbar = IsRegImeToolbarShown();
            if (bShowToolbar)
                CheckMenuItem(hMenu, ID_SHOWTOOLBAR, MF_CHECKED);
        }
    }

    if (!GetMenuItemCount(hMenu)) // No items?
    {
        // Clean up
        DestroyMenu(hMenu);
        CleanupImeMenus();

        SetForegroundWindow(hwndTarget);
        return;
    }

    // TrackPopupMenuEx flags
    UINT uFlags = TPM_VERTICAL | TPM_RIGHTALIGN | TPM_RETURNCMD;
    uFlags |= (bRightButton ? TPM_RIGHTBUTTON : TPM_LEFTBUTTON);

    // Exclude the notification area
    TPMPARAMS params = { sizeof(params) };
    GetWindowRect(g_hTrayNotifyWnd, &params.rcExclude);

    // Show the popup menu
    INT nID = TrackPopupMenuEx(hMenu, uFlags, pt.x, pt.y, hwnd, &params);

    // Workaround of TrackPopupMenu's bug
    PostMessage(hwnd, WM_NULL, 0, 0);

    if (nID) // Action!
    {
        if (nID >= ID_STARTIMEMENU) // IME internal menu ID?
        {
            MENUITEMINFO mii = { sizeof(mii), MIIM_DATA };
            GetMenuItemInfo(hMenu, nID, FALSE, &mii);

            if (pImeMenu)
                nID = GetRealImeMenuID(pImeMenu, nID);

            if (SetPenMenuData)
                SetPenMenuData(nID, mii.dwItemData);

            PostMessage(hwndIme, WM_IME_SYSTEM, IMS_IMEMENUITEMSELECTED, (LPARAM)hwndTarget);
        }
        else // Otherwise action of IME menu item
        {
            switch (nID)
            {
                case ID_INPUTSYSTEM:
                    if (IS_IME_HKL(hKL))
                        PostMessage(hwndIme, WM_IME_SYSTEM, IMS_CONFIGURE, (LPARAM)hKL);
                    break;
                case ID_IMEONOFF:
                    ImmSetOpenStatus(hIMC, !bImeOn);
                    break;
                case ID_SOFTKBDONOFF:
                    PostMessage(hwndIme, WM_IME_SYSTEM, IMS_SOFTKBDONOFF, !bSoftOn);
                    break;
                case ID_SHOWTOOLBAR:
                    ShowImeToolbar(hwndTarget, !bShowToolbar);
                    break;
                default:
                {
                    PostMessage(hwnd, WM_COMMAND, nID, 0);
                    break;
                }
            }
        }
    }

    // Clean up
    DestroyMenu(hMenu);
    CleanupImeMenus();

    SetForegroundWindow(hwndTarget);
}

// WM_COMMAND
static void
KbSwitch_OnCommand(HWND hwnd, UINT nID)
{
    switch (nID)
    {
        case ID_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

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
            if (nID >= ID_LANG_BASE)
            {
                if (!IsWindow(g_hwndLastActive))
                {
                    g_hwndLastActive = NULL;
                }
                ActivateLayout(hwnd, nID - ID_LANG_BASE, g_hwndLastActive, FALSE);
            }
            break;
        }
    }
}

// WM_LANG_CHANGED (HSHELL_LANGUAGE)
static LRESULT
KbSwitch_OnLangChanged(HWND hwnd, HWND hwndTarget OPTIONAL, HKL hKL OPTIONAL)
{
    TRACE("WM_LANG_CHANGED: hwndTarget:%p, hKL:%p\n", hwndTarget, hKL);
    /* Delayed action */
    KillTimer(hwnd, TIMER_ID_LANG_CHANGED_DELAYED);
    SetTimer(hwnd, TIMER_ID_LANG_CHANGED_DELAYED, TIMER_LANG_CHANGED_DELAY, NULL);
    return 0;
}

// WM_WINDOW_ACTIVATE (HCBT_ACTIVATE / HCBT_SETFOCUS / HSHELL_WINDOWACTIVATED)
static LRESULT
KbSwitch_OnWindowActivate(HWND hwnd, HWND hwndTarget OPTIONAL, LPARAM lParam OPTIONAL)
{
    TRACE("WM_WINDOW_ACTIVATE: hwndTarget:%p, lParam:%p\n", hwndTarget, lParam);
    HWND hwndFore = hwndTarget ? hwndTarget : GetForegroundWindow();
    if (RememberLastActive(hwnd, hwndFore))
        return UpdateLanguageDisplayCurrent(hwnd, hwndFore);
    return 0;
}

// WM_SETTINGCHANGE
static void
KbSwitch_OnSettingChange(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (wParam == SPI_SETNONCLIENTMETRICS)
        PostMessage(hwnd, WM_WINDOW_ACTIVATE, 0, 0);
}

static LRESULT
KbSwitch_OnDefault(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == g_uTaskbarRestartMsg)
    {
        UpdateTrayInfo();
        UpdateLayoutList(NULL);
        AddTrayIcon(hwnd);
        UpdatePenIcon(hwnd, g_iKL);
        return 0;
    }

    if (uMsg == g_uShellHookMessage)
    {
        TRACE("g_uShellHookMessage: wParam:%p, lParam:%p\n", wParam, lParam);
        if (wParam == HSHELL_LANGUAGE)
            PostMessage(hwnd, WM_LANG_CHANGED, 0, 0);
        else if (wParam == HSHELL_WINDOWACTIVATED || wParam == HSHELL_RUDEAPPACTIVATED)
            PostMessage(hwnd, WM_WINDOW_ACTIVATE, 0, 0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static VOID
OnIndicatorMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HKL hKL = (HKL)lParam;
    UINT iKL = GetLayoutIndexFromHKL(hKL);
    if (iKL >= g_cKLs)
        return;

    g_iKL = iKL;

    switch (uMsg)
    {
        case INDICM_SETIMEICON:
            if (LOWORD(wParam) == MAXWORD)
            {
                g_anFlags[iKL] &= ~LAYOUTF_IME_ICON;
                g_aiSysPenIcons[iKL] = MAXWORD;
            }
            else
            {
                g_anFlags[iKL] |= LAYOUTF_IME_ICON;
                g_aiSysPenIcons[iKL] = (WORD)wParam;
            }
            UpdatePenIcon(hwnd, iKL);
            break;

        case INDICM_SETIMETOOLTIPS:
            if (LOWORD(wParam) == MAXWORD)
            {
                g_anFlags[iKL] &= ~LAYOUTF_TOOLTIP_ATOM;
            }
            else
            {
                g_anFlags[iKL] |= LAYOUTF_TOOLTIP_ATOM;
                g_anToolTipAtoms[iKL] = LOWORD(wParam);
            }
            UpdatePenIcon(hwnd, iKL);
            break;

        case INDICM_REMOVEDEFAULTMENUITEMS:
            if (wParam)
            {
                if (wParam & RDMI_LEFT)
                    g_anFlags[iKL] |= LAYOUTF_REMOVE_LEFT_DEF_MENU;
                if (wParam & RDMI_RIGHT)
                    g_anFlags[iKL] |= LAYOUTF_REMOVE_RIGHT_DEF_MENU;
            }
            else
            {
                g_anFlags[iKL] &= ~(LAYOUTF_REMOVE_LEFT_DEF_MENU | LAYOUTF_REMOVE_RIGHT_DEF_MENU);
            }
            break;

        default:
        {
            ERR("uMsg: %u\n", uMsg);
            return;
        }
    }
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return KbSwitch_OnCreate(hwnd);

        case WM_TIMER:
            KbSwitch_OnTimer(hwnd, (UINT_PTR)wParam);
            break;

        case WM_LANG_CHANGED: /* Comes from indicdll.dll and this module */
            return KbSwitch_OnLangChanged(hwnd, (HWND)wParam, (HKL)lParam);

        case WM_WINDOW_ACTIVATE: /* Comes from indicdll.dll and this module */
            return KbSwitch_OnWindowActivate(hwnd, (HWND)wParam, lParam);

        case WM_NOTIFYICONMSG:
            KbSwitch_OnNotifyIconMsg(hwnd, (UINT)lParam);
            break;

        case WM_PENICONMSG:
            KbSwitch_OnPenIconMsg(hwnd, (UINT)lParam);
            break;

        case WM_COMMAND:
            KbSwitch_OnCommand(hwnd, LOWORD(wParam));
            break;

        case WM_SETTINGCHANGE:
            KbSwitch_OnSettingChange(hwnd, wParam, lParam);
            break;

        case WM_DESTROY:
            KbSwitch_OnDestroy(hwnd);
            break;

        case INDICM_SETIMEICON:
        case INDICM_SETIMETOOLTIPS:
        case INDICM_REMOVEDEFAULTMENUITEMS:
            if (InSendMessageEx(NULL))
                break; /* Must be a PostMessage call for quick response, not SendMessage */

            OnIndicatorMsg(hwnd, uMsg, wParam, lParam);
            break;

        case WM_INPUTLANGCHANGEREQUEST:
            TRACE("WM_INPUTLANGCHANGEREQUEST(%p, %p)\n", wParam, lParam);
            SetTimer(hwnd, TIMER_ID_LANG_CHANGED_DELAYED, TIMER_LANG_CHANGED_DELAY, NULL);
            break;

        default:
            return KbSwitch_OnDefault(hwnd, uMsg, wParam, lParam);
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

    hMutex = CreateMutex(NULL, FALSE, INDICATOR_CLASS);
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
    WndClass.lpszClassName = INDICATOR_CLASS;
    if (!RegisterClass(&WndClass))
    {
        CloseHandle(hMutex);
        return 1;
    }

    hwnd = CreateWindow(INDICATOR_CLASS, NULL, 0, 0, 0, 1, 1, HWND_DESKTOP, NULL, hInstance, NULL);
    g_uShellHookMessage = RegisterWindowMessage(L"SHELLHOOK");
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
