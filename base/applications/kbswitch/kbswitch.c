/*
 * PROJECT:         Keyboard Layout Switcher
 * FILE:            base/applications/kbswitch/kbswitch.c
 * PURPOSE:         Switching Keyboard Layouts
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Colin Finck (mail@colinfinck.de)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "kbswitch.h"

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

// Shell_NotifyIcon's message ID
#define WM_NOTIFYICONMSG (WM_USER + 248)
// Is hKL an IME HKL?
#define IS_IME_HKL(hKL) ((((ULONG_PTR)(hKL)) & 0xF0000000) == 0xE0000000)
// Is hKL a variant HKL?
#define IS_VARIANT_HKL(hKL) ((((ULONG_PTR)(hKL)) & 0xF0000000) == 0xF0000000)
// Get hKL's variant
#define GET_HKL_VARIANT(hKL) (HIWORD(hKL) & 0xFFF)

#define TIMER_ID 999
#define TIMER_INTERVAL 1000

typedef BOOL (APIENTRY *FN_KBS_HOOK)(HWND hwnd);
typedef VOID (APIENTRY *FN_KBS_UNHOOK)(VOID);
HINSTANCE g_hDLL = NULL;
FN_KBS_HOOK g_fnKbsHook = NULL;
FN_KBS_UNHOOK g_fnKbsUnhook = NULL;

HINSTANCE g_hInstance = NULL;
HICON g_hTrayIcon = NULL;
UINT g_uTaskbarRestart = 0;
HWND g_hwndTrayWnd = NULL;
HMENU g_hMenu = NULL;
HMENU g_hRightPopupMenu = NULL;
DWORD g_dwCodePageBitField = 0;
HWND g_hwndLastActive = NULL;
HKL g_hKL = NULL;

typedef struct tagENTRY
{
    DWORD dwKLID;
    LPWSTR pszText; // malloc'ed
    DWORD dwVariant;
} ENTRY, *PENTRY;

// The entries of Keyboard Layouts
PENTRY g_pEntries = NULL; // LocalAlloc'ed
UINT g_cEntries = 0, g_cCapacity = 0;

static VOID
FreeEntries(VOID)
{
    UINT i;
    for (i = 0; i < g_cEntries; ++i)
    {
        free(g_pEntries[i].pszText);
    }

    g_cEntries = g_cCapacity = 0;
    LocalFree(g_pEntries);
    g_pEntries = NULL;
}

static BOOL
GetSystemLibraryPath(LPWSTR szPath, SIZE_T cchPath, LPCWSTR FileName)
{
    if (!GetSystemDirectoryW(szPath, cchPath))
        return FALSE;

    StringCchCatW(szPath, cchPath, L"\\");
    StringCchCatW(szPath, cchPath, FileName);
    return TRUE;
}

static INT
AddEntry(LPCWSTR pszKLID)
{
    HKEY hKey;
    WCHAR szSubKey[MAX_PATH], szText[MAX_PATH];
    LONG error;
    DWORD cb, dwNewCount;
    PENTRY pEntry;
    INT iEntry = -1;

    StringCchCopyW(szSubKey, _countof(szSubKey), L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts");
    StringCchCatW(szSubKey, _countof(szSubKey), L"\\");
    StringCchCatW(szSubKey, _countof(szSubKey), pszKLID);

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &hKey);
    if (error != ERROR_SUCCESS)
        return -1;

    if (g_cCapacity < g_cEntries + 1)
    {
        dwNewCount = g_cEntries + 16;
        cb = dwNewCount * sizeof(ENTRY);
        g_pEntries = LocalReAlloc(g_pEntries, cb, LPTR);
        if (g_pEntries == NULL)
        {
            RegCloseKey(hKey);
            return -1;
        }

        g_cCapacity = dwNewCount;
    }

    pEntry = &g_pEntries[g_cEntries];

    // "Layout Id"
    pEntry->dwVariant = 0;
    szText[0] = UNICODE_NULL;
    cb = sizeof(szText);
    error = RegQueryValueExW(hKey, L"Layout Id", NULL, NULL, (LPBYTE)szText, &cb);
    if (error == ERROR_SUCCESS)
    {
        pEntry->dwVariant = _tcstoul(szText, NULL, 16);
    }

    // "Layout Text"
    szText[0] = UNICODE_NULL;
    cb = sizeof(szText);
    error = RegQueryValueExW(hKey, L"Layout Text", NULL, NULL, (LPBYTE)szText, &cb);
    if (error == ERROR_SUCCESS && cb > sizeof(WCHAR))
    {
        // "Layout Display Name"
        SHLoadRegUIStringW(hKey, L"Layout Display Name", szText, _countof(szText));

        pEntry->pszText = _tcsdup(szText);

        // dwKLID
        pEntry->dwKLID = _tcstoul(pszKLID, NULL, 16);

        iEntry = g_cEntries++;
    }

    RegCloseKey(hKey);
    return iEntry;
}

static BOOL
LoadEntries(VOID)
{
    HKEY hLayoutsKey;
    LONG error;
    DWORD dwIndex, dwNewCount = 256;
    WCHAR szKeyName[MAX_PATH];

    FreeEntries();

    g_pEntries = LocalAlloc(LPTR, dwNewCount * sizeof(ENTRY));
    if (g_pEntries == NULL)
    {
        return FALSE;
    }
    g_cCapacity = dwNewCount;

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts",
                          0,
                          KEY_READ,
                          &hLayoutsKey);
    if (error != ERROR_SUCCESS)
    {
        return FALSE;
    }

    for (dwIndex = 0; dwIndex < 256; ++dwIndex)
    {
        szKeyName[0] = UNICODE_NULL;
        error = RegEnumKeyW(hLayoutsKey, dwIndex, szKeyName, _countof(szKeyName));
        if (error != ERROR_SUCCESS)
            break;

        AddEntry(szKeyName);
    }

    RegCloseKey(hLayoutsKey);
    return g_cEntries > 0;
}

static INT
FindEntry(HKL hKL)
{
    UINT i;
    WCHAR szKLID[KL_NAMELENGTH + 1];

    if (IS_IME_HKL(hKL))
    {
        for (i = 0; i < g_cEntries; ++i)
        {
            if (hKL == (HKL)(DWORD_PTR)g_pEntries[i].dwKLID)
                return i;
        }

        StringCchPrintfW(szKLID, _countof(szKLID), L"%08lX", (DWORD)(DWORD_PTR)hKL);
        return AddEntry(szKLID);
    }
    else if (IS_VARIANT_HKL(hKL))
    {
        DWORD dwVariant = GET_HKL_VARIANT(hKL);
        for (i = 0; i < g_cEntries; ++i)
        {
            if (g_pEntries[i].dwVariant == dwVariant &&
                LOWORD(hKL) == LOWORD(g_pEntries[i].dwKLID))
            {
                return i;
            }
        }
    }
    else
    {
        LANGID LangID = LOWORD(hKL);
        for (i = 0; i < g_cEntries; ++i)
        {
            if (LangID == LOWORD(g_pEntries[i].dwKLID))
                return i;
        }
        LangID = HIWORD(hKL);
        for (i = 0; i < g_cEntries; ++i)
        {
            if (LangID == LOWORD(g_pEntries[i].dwKLID))
                return i;
        }
    }

    return -1;
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
CreateTrayIcon(HKL hKL, LPCWSTR szImeFile OPTIONAL)
{
    LANGID LangID;
    WCHAR szBuf[4];
    HDC hdcScreen, hdc;
    HBITMAP hbmColor, hbmMono, hBmpOld;
    HFONT hFont, hFontOld;
    LOGFONTW lf;
    RECT rect;
    ICONINFO IconInfo;
    HICON hIcon;
    INT cxIcon = GetSystemMetrics(SM_CXSMICON);
    INT cyIcon = GetSystemMetrics(SM_CYSMICON);
    WCHAR szPath[MAX_PATH];

    if (szImeFile && szImeFile[0])
    {
        if (GetSystemLibraryPath(szPath, _countof(szPath), szImeFile))
        {
            ExtractIconExW(szPath, 0, NULL, &hIcon, 1);
            if (hIcon)
                return hIcon;
        }
    }

    /* Getting "EN", "FR", etc. from English, French, ... */
    LangID = LOWORD(hKL);
    if (GetLocaleInfoW(LangID,
                       LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                       szBuf,
                       _countof(szBuf)) == 0)
    {
        szBuf[0] = szBuf[1] = L'?';
    }

    /* Truncate the identifier to two characters: "ENG" --> "EN" etc. */
    szBuf[2] = UNICODE_NULL;

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
    if (SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
    {
        /* Override the current size with something manageable */
        lf.lfHeight = -11;
        lf.lfWidth = 0;
        hFont = CreateFontIndirectW(&lf);
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
    DrawTextW(hdc, szBuf, 2, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
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

#define FIRST_KEYBOARD_ITEM_ID 300

static HKL
ShowKeyboardMenu(HWND hwnd, HKL hCheckKL, POINT pt)
{
    HKL hKL, ahKLs[256];
    UINT iKL, cKLs;
    HMENU hMenu = CreatePopupMenu();
    MENUITEMINFOW mii = { sizeof(mii) };
    WCHAR szText[MAX_PATH], szImeFile[MAX_PATH];

    cKLs = GetKeyboardLayoutList(_countof(ahKLs), ahKLs);
    for (iKL = 0; iKL < cKLs; ++iKL)
    {
        hKL = ahKLs[iKL];

        szText[0] = szImeFile[0] = UNICODE_NULL;

        if (IS_IME_HKL(hKL))
        {
            ImmGetDescriptionW(hKL, szText, _countof(szText));
            ImmGetIMEFileNameW(hKL, szImeFile, _countof(szImeFile));
        }
        else
        {
            GetLocaleInfoW(LOWORD(hKL), LOCALE_SLANGUAGE, szText, _countof(szText));
            if (LOWORD(hKL) != HIWORD(hKL))
            {
                INT iEntry = FindEntry(hKL);
                if (iEntry != -1)
                {
                    PENTRY pEntry = &g_pEntries[iEntry];
                    if (pEntry->pszText)
                    {
                        StringCchCat(szText, _countof(szText), L" - ");
                        StringCchCat(szText, _countof(szText), pEntry->pszText);
                    }
                }
            }
        }

        mii.fMask       = MIIM_ID | MIIM_STRING;
        mii.wID         = FIRST_KEYBOARD_ITEM_ID + iKL;
        mii.dwTypeData  = szText;

        HICON hIcon = CreateTrayIcon(hKL, szImeFile);
        if (hIcon)
        {
            mii.hbmpItem = BitmapFromIcon(hIcon);
            if (mii.hbmpItem)
                mii.fMask |= MIIM_BITMAP;
        }

        if (hKL == hCheckKL)
        {
            mii.fMask |= MIIM_STATE;
            mii.fState = MFS_CHECKED;
        }

        InsertMenuItemW(hMenu, -1, TRUE, &mii);
        if (hIcon)
            DestroyIcon(hIcon);
    }

    hKL = NULL;
    INT nID = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
    if (nID != 0)
    {
        hKL = ahKLs[nID - FIRST_KEYBOARD_ITEM_ID];
    }
    DestroyMenu(hMenu);

    return hKL;
}

static HWND GetTrayWnd(VOID)
{
    return FindWindowW(L"Shell_TrayWnd", NULL);
}

static BOOL IsWndClass(HWND hwnd, LPCWSTR pszClass)
{
    WCHAR szClass[128];
    return (GetClassNameW(hwnd, szClass, _countof(szClass)) && _wcsicmp(szClass, pszClass) == 0);
}

static BOOL IsKbswitchWindow(HWND hwnd)
{
    return IsWndClass(hwnd, KBSWITCH_CLASS);
}

// NOTE: GetWindowThreadProcessId function for console window doesn't return the
//       expected value.
static BOOL IsConsoleWnd(HWND hwnd)
{
    return IsWndClass(hwnd, TEXT("ConsoleWindowClass"));
}

static BOOL IsTrayWnd(HWND hwnd)
{
    return g_hwndTrayWnd == hwnd;
}

static HWND GetTopLevelOwner(HWND hwndTarget)
{
    HWND hwndDesktop = GetDesktopWindow();
    HWND hTopWnd = hwndTarget;

    for (;;)
    {
        if (hwndTarget == NULL || hwndTarget == hwndDesktop)
            break;
        hTopWnd = hwndTarget;
        if ((GetWindowLongPtrW(hwndTarget, GWL_STYLE) & WS_CHILD) == 0)
            hwndTarget = GetWindow(hwndTarget, GW_OWNER);
        else
            hwndTarget = GetParent(hwndTarget);
    }

    return hTopWnd;
}

static HWND RealGetTopLevelOwner(HWND hwndTarget)
{
    HWND hwndTopLevel = GetTopLevelOwner(hwndTarget);
    DWORD dwTID1 = GetWindowThreadProcessId(hwndTopLevel, NULL);
    DWORD dwTID2 = GetWindowThreadProcessId(hwndTarget, NULL);
    if (dwTID1 != dwTID2)
        hwndTopLevel = hwndTarget;

    return hwndTopLevel;
}

static BOOL IsWndIgnored(HWND hwndTarget)
{
    HWND hwndTopLevel = RealGetTopLevelOwner(hwndTarget);
    return !IsWindowVisible(hwndTopLevel) ||
           IsTrayWnd(hwndTopLevel) ||
           IsKbswitchWindow(hwndTopLevel);
}

static VOID SetLastActive(HWND hwndTarget, INT line)
{
    if (g_hwndLastActive != hwndTarget)
    {
        g_hwndLastActive = hwndTarget;
    }
}

static DWORD
GetCodePageBitField(HWND hwnd)
{
    CHARSETINFO CharSet;
    HDC hDC = GetDC(hwnd);
    UINT uCharSet = GetTextCharset(hDC);
    TranslateCharsetInfo((LPDWORD)(UINT_PTR)uCharSet, &CharSet, TCI_SRCCHARSET);
    ReleaseDC(hwnd, hDC);
    return CharSet.fs.fsCsb[0];
}

static BOOL
IsHKLCharSetSupported(HKL hKL)
{
    LOCALESIGNATURE Signature;
    if (!GetLocaleInfoW(LOWORD(hKL), LOCALE_FONTSIGNATURE, (LPWSTR)&Signature, sizeof(Signature)))
        return FALSE;
    return !!(Signature.lsCsbSupported[0] & g_dwCodePageBitField);
}

static BOOL
GetImeFile(LPWSTR szImeFile, SIZE_T cchImeFile, HKL hKL)
{
    szImeFile[0] = UNICODE_NULL;

    if (!IS_IME_HKL(hKL))
        return FALSE;

    return ImmGetIMEFileNameW(hKL, szImeFile, cchImeFile);
}

static VOID
AddTrayIcon(HWND hwnd, HKL hKL)
{
    NOTIFYICONDATAW tnid = { sizeof(tnid), hwnd, 1, NIF_ICON | NIF_MESSAGE | NIF_TIP };
    WCHAR szImeFile[80];
    INT iEntry;

    iEntry = FindEntry(hKL);
    if (iEntry == -1)
        return;

    GetImeFile(szImeFile, _countof(szImeFile), hKL);

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(hKL, szImeFile);
    StringCchCopyW(tnid.szTip, _countof(tnid.szTip), g_pEntries[iEntry].pszText);

    Shell_NotifyIconW(NIM_ADD, &tnid);

    if (g_hTrayIcon)
        DestroyIcon(g_hTrayIcon);
    g_hTrayIcon = tnid.hIcon;
}

static VOID
DestroyTrayIcon(HWND hwnd)
{
    NOTIFYICONDATAW tnid = { sizeof(tnid), hwnd, 1 };
    Shell_NotifyIconW(NIM_DELETE, &tnid);

    if (g_hTrayIcon)
    {
        DestroyIcon(g_hTrayIcon);
        g_hTrayIcon = NULL;
    }
}

static VOID
UpdateTrayIcon(HWND hwnd, HKL hKL)
{
    NOTIFYICONDATAW tnid = { sizeof(tnid), hwnd, 1, NIF_ICON | NIF_MESSAGE | NIF_TIP };
    WCHAR szImeFile[80];
    INT iEntry;

    iEntry = FindEntry(hKL);
    if (iEntry == -1)
        return;

    GetImeFile(szImeFile, _countof(szImeFile), hKL);

    tnid.uCallbackMessage = WM_NOTIFYICONMSG;
    tnid.hIcon = CreateTrayIcon(hKL, szImeFile);
    StringCchCopyW(tnid.szTip, _countof(tnid.szTip), g_pEntries[iEntry].pszText);

    Shell_NotifyIconW(NIM_MODIFY, &tnid);

    if (g_hTrayIcon)
        DestroyIcon(g_hTrayIcon);
    g_hTrayIcon = tnid.hIcon;
}

static VOID
RememberWindowHKL(HWND hwnd, HWND hwndTarget, HKL hKL)
{
    WCHAR szHWND[32];
    StringCchPrintfW(szHWND, _countof(szHWND), TEXT("%p"), hwndTarget);
    SetPropW(hwnd, szHWND, hKL);
}

static HKL
RecallWindowHKL(HWND hwnd, HWND hwndTarget)
{
    WCHAR szHWND[32];
    DWORD dwThreadId;
    HKL hKL;

    StringCchPrintfW(szHWND, _countof(szHWND), TEXT("%p"), hwndTarget);

    hKL = (HKL)GetPropW(hwnd, szHWND);
    if (hKL == NULL)
    {
        dwThreadId = GetWindowThreadProcessId(hwndTarget, NULL);
        hKL = GetKeyboardLayout(dwThreadId);
        if (hKL == NULL)
            hKL = GetKeyboardLayout(0);
    }

    return hKL;
}

static VOID
ForgetWindowHKL(HWND hwnd, HWND hwndTarget)
{
    WCHAR szHWND[32];
    StringCchPrintfW(szHWND, _countof(szHWND), TEXT("%p"), hwndTarget);
    RemovePropW(hwnd, szHWND);
}

static BOOL
OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    if (!LoadEntries())
        return FALSE;

    g_hDLL = LoadLibraryW(L"kbsdll.dll");
    if (!g_hDLL)
        return TRUE;

    g_fnKbsHook = (FN_KBS_HOOK)GetProcAddress(g_hDLL, "KbsHook");
    g_fnKbsUnhook = (FN_KBS_UNHOOK)GetProcAddress(g_hDLL, "KbsUnhook");
    if (!g_fnKbsHook || !g_fnKbsUnhook)
    {
        FreeLibrary(g_hDLL);
        g_hDLL = NULL;
        return FALSE;
    }

    g_hKL = GetKeyboardLayout(0);
    AddTrayIcon(hwnd, g_hKL);
    g_hwndTrayWnd = GetTrayWnd();
    g_uTaskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");

    g_dwCodePageBitField = GetCodePageBitField(hwnd);

    g_fnKbsHook(hwnd);
    SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
    return TRUE;
}

static VOID
OnTimer(HWND hwnd, UINT id)
{
    if (id != TIMER_ID)
        return;

    HWND hwndTarget = GetForegroundWindow();
    if (IsWndIgnored(hwndTarget))
        return;

    SetLastActive(hwndTarget, __LINE__);

    DWORD dwThreadId = GetWindowThreadProcessId(hwndTarget, NULL);
    HKL hKL = GetKeyboardLayout(dwThreadId);
    if (hKL == NULL)
    {
        hKL = RecallWindowHKL(hwnd, hwndTarget);
    }

    UpdateTrayIcon(hwnd, hKL);
    g_hKL = hKL;
}

static BOOL CALLBACK
RemovePropProc(HWND hwnd, LPCWSTR lpszString, HANDLE hData)
{
    RemovePropW(hwnd, lpszString);
    return TRUE;
}

static VOID
OnDestroy(HWND hwnd)
{
    KillTimer(hwnd, TIMER_ID);

    if (g_hMenu)
    {
        DestroyMenu(g_hMenu);
    }

    DestroyTrayIcon(hwnd);

    if (g_hDLL)
    {
        g_fnKbsUnhook();
        FreeLibrary(g_hDLL);
    }

    EnumPropsW(hwnd, RemovePropProc);

    FreeEntries();

    PostQuitMessage(0);
}

static VOID
ChooseLayout(HWND hwnd, HKL hKL)
{
    HWND hwndTarget = g_hwndLastActive;
    if (hwndTarget == NULL)
        return;

    HWND hwndTopLevel = RealGetTopLevelOwner(hwndTarget);
    HWND hwndLastActive = GetLastActivePopup(hwndTopLevel);
    SetForegroundWindow(hwndLastActive);

    WPARAM wParam = (IsHKLCharSetSupported(hKL) ? INPUTLANGCHANGE_SYSCHARSET : 0);
    PostMessageW(hwndLastActive, WM_INPUTLANGCHANGEREQUEST, wParam, (LPARAM)hKL);

    // We can't get the real KL from console window, so remember the KL.
    if (IsConsoleWnd(hwndTarget))
    {
        RememberWindowHKL(hwnd, hwndTarget, hKL);
    }
}

static VOID
OnNotifyIcon(HWND hwnd, LPARAM lParam)
{
    switch (lParam)
    {
        case WM_RBUTTONUP:
        case WM_LBUTTONUP:
        {
            KillTimer(hwnd, TIMER_ID);

            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);

            if (lParam == WM_LBUTTONUP)
            {
                /* Rebuild the left popup menu on every click to take care of keyboard layout changes */
                HKL hKL = ShowKeyboardMenu(hwnd, g_hKL, pt);
                if (hKL)
                {
                    ChooseLayout(hwnd, hKL);
                    g_hKL = hKL;
                }
            }
            else
            {
                if (!g_hRightPopupMenu)
                {
                    g_hMenu = LoadMenuW(g_hInstance, MAKEINTRESOURCEW(IDR_POPUP));
                    g_hRightPopupMenu = GetSubMenu(g_hMenu, 0);
                }
                TrackPopupMenu(g_hRightPopupMenu, 0, pt.x, pt.y, 0, hwnd, NULL);
            }

            PostMessageW(hwnd, WM_NULL, 0, 0);

            SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
            break;
        }
    }
}

static HKL
GetNextLayout(VOID)
{
    HKL ahKLs[256];
    UINT iKL, cKLs;

    cKLs = GetKeyboardLayoutList(_countof(ahKLs), ahKLs);
    for (iKL = 0; iKL < cKLs; ++iKL)
    {
        if (g_hKL == ahKLs[iKL])
        {
            return ahKLs[(iKL + 1) % cKLs];
        }
    }

    return NULL;
}

static VOID
OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case ID_EXIT:
        {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
            break;
        }

        case ID_PREFERENCES:
        {
            INT_PTR ret = (INT_PTR)ShellExecuteW(hwnd, NULL,
                                                 L"control.exe", L"input.dll",
                                                 NULL, SW_SHOWNORMAL);
            if (ret <= 32)
                MessageBoxW(hwnd, L"Can't start input.dll", NULL, MB_ICONERROR);
            break;
        }

        case ID_NEXTLAYOUT:
        {
            ChooseLayout(hwnd, GetNextLayout());
            break;
        }

        default:
            break;
    }
}

static LRESULT CALLBACK
WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        case WM_NOTIFYICONMSG:
        {
            OnNotifyIcon(hwnd, lParam);
            break;
        }
        case WM_LANGUAGE: // HSHELL_LANGUAGE
        {
            HWND hwndTarget = (HWND)wParam;
            HKL hKL = (HKL)lParam;
            if (hKL == NULL || hwndTarget == NULL)
                break;
            if (IsWndIgnored(hwndTarget))
                break;
            // We can't get the real KL from console window, so remember the KL.
            if (IsConsoleWnd(hwndTarget) && hKL)
                RememberWindowHKL(hwnd, hwndTarget, hKL);
            g_hKL = hKL;
            UpdateTrayIcon(hwnd, g_hKL);
            break;
        }
        case WM_WINDOWACTIVATED: // HSHELL_WINDOWACTIVATED
        {
            HWND hwndTarget = (HWND)wParam;
            DWORD dwThreadId = 0;
            HKL hKL = NULL;

            if (IsWndIgnored(hwndTarget))
                break;

            if (IsConsoleWnd(hwndTarget))
            {
                hKL = RecallWindowHKL(hwnd, hwndTarget);
            }
            else
            {
                dwThreadId = GetWindowThreadProcessId(hwndTarget, NULL);
                hKL = GetKeyboardLayout(dwThreadId);
            }

            g_hKL = hKL;
            UpdateTrayIcon(hwnd, g_hKL);
            break;
        }
        case WM_WINDOWCREATED: // HSHELL_WINDOWCREATED
        {
            // TODO:
            break;
        }
        case WM_WINDOWDESTROYED: // HSHELL_WINDOWCREATED
        {
            HWND hwndTarget = (HWND)wParam;
            ForgetWindowHKL(hwnd, hwndTarget);
            break;
        }
        case WM_WINDOWSETFOCUS: // HCBT_SETFOCUS
        {
            // TODO:
            break;
        }
        default:
        {
            if (uMsg == g_uTaskbarRestart)
            {
                AddTrayIcon(hwnd, g_hKL);
                g_hwndTrayWnd = GetTrayWnd();
                break;
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

INT WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    WNDCLASSW WndClass;
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

    hMutex = CreateMutexW(NULL, FALSE, KBSWITCH_CLASS);
    if (!hMutex)
        return 1;

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hMutex);
        return 1;
    }

    g_hInstance = hInstance;

    ZeroMemory(&WndClass, sizeof(WndClass));
    WndClass.lpfnWndProc   = WndProc;
    WndClass.hInstance     = hInstance;
    WndClass.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    WndClass.lpszClassName = KBSWITCH_CLASS;
    if (!RegisterClassW(&WndClass))
    {
        CloseHandle(hMutex);
        return 1;
    }

    hwnd = CreateWindowW(KBSWITCH_CLASS, NULL, WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, NULL, NULL,
                         hInstance, NULL);
    if (hwnd == NULL)
    {
        CloseHandle(hMutex);
        MessageBoxW(NULL, L"CreateWindow failed", KBSWITCH_CLASS, MB_ICONERROR);
        return 1;
    }

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CloseHandle(hMutex);
    return 0;
}
