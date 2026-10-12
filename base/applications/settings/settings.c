/*
 * PROJECT:     ReactOS Settings
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Windows 11 style Settings app: categories + searchable index
 *              of the settings that live in the classic Control Panel applets.
 *
 * Command line:
 *   settings.exe                     open on the first category
 *   settings.exe /search <text>      open with a search already typed
 *   settings.exe /open <text>        run the best match for <text> directly
 */

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <strsafe.h>

#include <undocuser.h>

#include "resource.h"
#include "search.h"
#include "settings_index.h"

/* ------------------------------------------------------------------------ */
/* Index tables, built from the X-macros in settings_index.h                 */

enum
{
#define CATEGORY(Id, Name, Desc, IconFile, IconIndex) CAT_##Id,
    SETTINGS_CATEGORIES
#undef CATEGORY
    CAT_COUNT
};

typedef struct _CATEGORY_INFO
{
    PCWSTR Name;
    PCWSTR Description;
    PCWSTR IconFile;
    INT IconIndex;
    HICON hIcon;
} CATEGORY_INFO;

static CATEGORY_INFO g_Categories[] =
{
#define CATEGORY(Id, Name, Desc, IconFile, IconIndex) { Name, Desc, IconFile, IconIndex, NULL },
    SETTINGS_CATEGORIES
#undef CATEGORY
};

typedef struct _SETTING_INFO
{
    INT Category;
    PCWSTR Program;
    PCWSTR Parameters;
    HICON hIcon;
} SETTING_INFO;

static SETTING_INFO g_Settings[] =
{
#define SETTING(Id, Cat, T, D, K, Program, Params) { CAT_##Cat, Program, Params, NULL },
    SETTINGS_INDEX
#undef SETTING
};

static SEARCH_ITEM g_SearchItems[] =
{
#define SETTING(Id, Cat, T, D, K, Program, Params) { T, K, D, NULL },
    SETTINGS_INDEX
#undef SETTING
};

#define SETTING_COUNT ((INT)_countof(g_Settings))

enum
{
#define SETTING(Id, Cat, T, D, K, Program, Params) SET_##Id,
    SETTINGS_INDEX
#undef SETTING
};

/* ------------------------------------------------------------------------ */
/* Look                                                                      */

/* Light colors; with a dark visual style (color scheme "Dark") UIC() gives their
 * dark mode counterparts */
#define CLR_WINDOW       UIC(RGB(243, 243, 243))
#define CLR_CARD         UIC(RGB(251, 251, 251))
#define CLR_CARD_HOT     UIC(RGB(246, 246, 246))
#define CLR_CARD_BORDER  UIC(RGB(229, 229, 229))
#define CLR_NAV_SELECTED UIC(RGB(234, 234, 234))
#define CLR_ACCENT       RGB(0, 95, 184)
#define CLR_TEXT         UIC(RGB(26, 26, 26))
#define CLR_TEXT_2       UIC(RGB(96, 96, 96))
#define CLR_FIELD        UIC(RGB(255, 255, 255))

#define NAV_WIDTH   300
#define MARGIN      16
#define NAV_ITEM_H  40
#define CARD_H      70
#define HEADER_H    76

#define IDC_SEARCH  101
#define IDC_NAV     102
#define IDC_CARDS   103

static HINSTANCE g_hInstance;
static HWND g_hMainWnd, g_hSearch, g_hNav, g_hCards;
static HFONT g_hFont, g_hFontBold, g_hFontTitle, g_hFontHeader, g_hFontChevron;
static HBRUSH g_hbrWindow, g_hbrField;
static BOOL g_bDark;

/* dark when the visual style's color scheme is "Dark" (e.g. Eleven Dark) */
static BOOL IsDarkTheme(VOID)
{
    /* Read from the registry: uxtheme only knows the theme of this process once
     * its first window exists, and the colors are needed before that */
    static const WCHAR szKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager";
    WCHAR szValue[64];
    DWORD cb;
    HKEY hKey;
    BOOL bDark = FALSE;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, szKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;
    cb = sizeof(szValue) - sizeof(WCHAR);
    ZeroMemory(szValue, sizeof(szValue));
    if (RegQueryValueExW(hKey, L"ThemeActive", NULL, NULL, (LPBYTE)szValue, &cb) == ERROR_SUCCESS &&
        szValue[0] == L'1')
    {
        cb = sizeof(szValue) - sizeof(WCHAR);
        ZeroMemory(szValue, sizeof(szValue));
        bDark = RegQueryValueExW(hKey, L"ColorName", NULL, NULL, (LPBYTE)szValue, &cb) == ERROR_SUCCESS &&
                !_wcsicmp(szValue, L"Dark");
    }
    RegCloseKey(hKey);
    return bDark;
}

/* grays are inverted in dark mode (light surfaces -> near-black, dark text -> light) */
static COLORREF UIC(COLORREF cr)
{
    INT v;
    if (!g_bDark)
        return cr;
    v = 288 - (GetRValue(cr) + GetGValue(cr) + GetBValue(cr)) / 3;
    v = max(0, min(255, v));
    return RGB(v, v, v);
}

static VOID RefreshUiColors(VOID)
{
    g_bDark = IsDarkTheme();
    if (g_hbrWindow)
        DeleteObject(g_hbrWindow);
    if (g_hbrField)
        DeleteObject(g_hbrField);
    g_hbrWindow = CreateSolidBrush(CLR_WINDOW);
    g_hbrField = CreateSolidBrush(CLR_FIELD);
}
static INT g_CurrentCategory = 0;
static INT g_HotCard = -1;
static WCHAR g_szQuery[256];
static WNDPROC g_pfnOldSearchProc, g_pfnOldCardsProc;

/* ------------------------------------------------------------------------ */

static HFONT
CreateUiFont(INT Points, INT Weight, PCWSTR Face)
{
    HDC hdc = GetDC(NULL);
    INT Height = -MulDiv(Points, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);
    return CreateFontW(Height, 0, 0, 0, Weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                       DEFAULT_PITCH | FF_SWISS, Face);
}

static INT g_Dpi = 96;

static VOID
UpdateDpi(VOID)
{
    HDC hdc = GetDC(NULL);
    g_Dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
}

static INT
DpiScale(INT Value)
{
    return MulDiv(Value, g_Dpi, 96);
}

static VOID
CreateFonts(VOID)
{
    g_hFont = CreateUiFont(9, FW_NORMAL, L"Selawik");
    g_hFontBold = CreateUiFont(9, FW_SEMIBOLD, L"Selawik Semibold");
    g_hFontHeader = CreateUiFont(20, FW_SEMIBOLD, L"Selawik Semibold");
    g_hFontTitle = CreateUiFont(9, FW_NORMAL, L"Selawik");
    g_hFontChevron = CreateUiFont(14, FW_NORMAL, L"Selawik");
}

static VOID
DestroyFonts(VOID)
{
    DeleteObject(g_hFont);
    DeleteObject(g_hFontBold);
    DeleteObject(g_hFontHeader);
    DeleteObject(g_hFontTitle);
    DeleteObject(g_hFontChevron);
}

/* Icon for a setting: the applet or program it opens */
static HICON
GetSettingIcon(INT i)
{
    SETTING_INFO *s = &g_Settings[i];
    WCHAR szFile[MAX_PATH];
    PCWSTR p;
    HICON hIcon = NULL;

    if (s->hIcon)
        return s->hIcon;

    StringCchCopyW(szFile, _countof(szFile), s->Program);
    p = s->Parameters ? wcsstr(s->Parameters, L"Control_RunDLL ") : NULL;
    if (p)
    {
        /* "...Control_RunDLL desk.cpl,,2" -> "desk.cpl" */
        PCWSTR pEnd;
        p += wcslen(L"Control_RunDLL ");
        pEnd = wcschr(p, L',');
        StringCchCopyNW(szFile, _countof(szFile), p, pEnd ? (size_t)(pEnd - p) : wcslen(p));
    }
    else if (s->Parameters && wcsstr(s->Parameters, L"Options_RunDLL"))
    {
        /* Folder options / taskbar: use the category icon */
        szFile[0] = UNICODE_NULL;
    }

    if (szFile[0])
        PrivateExtractIconsW(szFile, 0, DpiScale(32), DpiScale(32), &hIcon, NULL, 1, 0);
    if (!hIcon)
    {
        CATEGORY_INFO *c = &g_Categories[s->Category];
        PrivateExtractIconsW(c->IconFile, c->IconIndex, DpiScale(32), DpiScale(32), &hIcon, NULL, 1, 0);
    }
    s->hIcon = hIcon;
    return hIcon;
}

static HICON
GetCategoryIcon(INT i)
{
    CATEGORY_INFO *c = &g_Categories[i];
    if (!c->hIcon)
        PrivateExtractIconsW(c->IconFile, c->IconIndex, DpiScale(16), DpiScale(16), &c->hIcon, NULL, 1, 0);
    return c->hIcon;
}

#define WM_APP_SCALEMENU (WM_APP + 1)

/* "Scale": pick 100% .. 200% like Windows 11; applied immediately */
static VOID
ShowScaleMenu(VOID)
{
    static const UINT Dpis[] = { 96, 120, 144, 168, 192 };
    HMENU hMenu = CreatePopupMenu();
    UINT uCurrent = 96, i;
    POINT pt;
    INT Cmd;

    SystemParametersInfoW(SPI_GETLOGICALDPIOVERRIDE, 0, &uCurrent, 0);
    for (i = 0; i < _countof(Dpis); i++)
    {
        WCHAR szItem[64];
        StringCchPrintfW(szItem, _countof(szItem), L"%u%%%s", Dpis[i] * 100 / 96,
                         i == 0 ? L" (Recommended)" : L"");
        AppendMenuW(hMenu, MF_STRING | (Dpis[i] == uCurrent ? MF_CHECKED : 0), i + 1, szItem);
    }

    /* Under the right end of the Scale card (works from the keyboard too) */
    GetCursorPos(&pt);
    for (i = 0; i < (UINT)SendMessageW(g_hCards, LB_GETCOUNT, 0, 0); i++)
    {
        RECT rc;
        if (SendMessageW(g_hCards, LB_GETITEMDATA, i, 0) == SET_DPI &&
            SendMessageW(g_hCards, LB_GETITEMRECT, i, (LPARAM)&rc) != LB_ERR)
        {
            pt.x = rc.right - DpiScale(8);
            pt.y = rc.bottom - DpiScale(4);
            ClientToScreen(g_hCards, &pt);
            break;
        }
    }
    SetForegroundWindow(g_hMainWnd);
    Cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_RIGHTALIGN, pt.x, pt.y, 0, g_hMainWnd, NULL);
    DestroyMenu(hMenu);

    if (Cmd > 0 && Dpis[Cmd - 1] != uCurrent)
        SystemParametersInfoW(SPI_SETLOGICALDPIOVERRIDE, Dpis[Cmd - 1], NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

static VOID
LaunchSetting(INT i)
{
    if (i < 0 || i >= SETTING_COUNT)
        return;
    if (i == SET_DPI)
    {
        /* Not from inside a key or mouse handler: the menu needs its own message loop */
        PostMessageW(g_hMainWnd, WM_APP_SCALEMENU, 0, 0);
        return;
    }
    ShellExecuteW(g_hMainWnd, NULL, g_Settings[i].Program, g_Settings[i].Parameters, NULL, SW_SHOWNORMAL);
}

/* Fill the card list with the settings of the current category, or the search results */
static VOID
RefreshCards(VOID)
{
    INT i;

    SendMessageW(g_hCards, WM_SETREDRAW, FALSE, 0);
    SendMessageW(g_hCards, LB_RESETCONTENT, 0, 0);

    if (g_szQuery[0])
    {
        INT Results[64], Count;
        Count = SearchItems(g_SearchItems, SETTING_COUNT, g_szQuery, Results, _countof(Results));
        for (i = 0; i < Count; i++)
            SendMessageW(g_hCards, LB_ADDSTRING, 0, Results[i]);
    }
    else
    {
        for (i = 0; i < SETTING_COUNT; i++)
        {
            if (g_Settings[i].Category == g_CurrentCategory)
                SendMessageW(g_hCards, LB_ADDSTRING, 0, i);
        }
    }

    g_HotCard = -1;
    SendMessageW(g_hCards, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hCards, NULL, TRUE);

    /* Header text changed too */
    {
        RECT rc;
        GetClientRect(g_hMainWnd, &rc);
        rc.left = DpiScale(NAV_WIDTH);
        rc.bottom = DpiScale(HEADER_H);
        InvalidateRect(g_hMainWnd, &rc, TRUE);
    }
}

/* ------------------------------------------------------------------------ */
/* Drawing                                                                   */

static VOID
FillRounded(HDC hdc, const RECT *prc, INT Radius, COLORREF Fill, COLORREF Border)
{
    HBRUSH hbr = CreateSolidBrush(Fill);
    HPEN hpen = CreatePen(PS_SOLID, 1, Border);
    HGDIOBJ hOldBrush = SelectObject(hdc, hbr), hOldPen = SelectObject(hdc, hpen);
    RoundRect(hdc, prc->left, prc->top, prc->right, prc->bottom, Radius, Radius);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hbr);
    DeleteObject(hpen);
}

static VOID
DrawNavItem(const DRAWITEMSTRUCT *dis)
{
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem, rcText;
    INT i = (INT)dis->itemData;
    BOOL bSelected = (i == g_CurrentCategory) && !g_szQuery[0];
    INT IconSize = DpiScale(16);

    FillRect(hdc, &rc, g_hbrWindow);
    InflateRect(&rc, -DpiScale(4), -DpiScale(2));
    if (bSelected)
    {
        RECT rcBar;
        FillRounded(hdc, &rc, DpiScale(8), CLR_NAV_SELECTED, CLR_NAV_SELECTED);
        /* The Windows 11 navigation indicator: a short accent bar on the left */
        SetRect(&rcBar, rc.left, rc.top + (rc.bottom - rc.top) / 2 - DpiScale(8),
                rc.left + DpiScale(3), rc.top + (rc.bottom - rc.top) / 2 + DpiScale(8));
        FillRounded(hdc, &rcBar, DpiScale(3), CLR_ACCENT, CLR_ACCENT);
    }

    DrawIconEx(hdc, rc.left + DpiScale(14), rc.top + (rc.bottom - rc.top - IconSize) / 2,
               GetCategoryIcon(i), IconSize, IconSize, 0, NULL, DI_NORMAL);

    rcText = rc;
    rcText.left += DpiScale(14) + IconSize + DpiScale(14);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, CLR_TEXT);
    SelectObject(hdc, bSelected ? g_hFontBold : g_hFont);
    DrawTextW(hdc, g_Categories[i].Name, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
}

static VOID
DrawCard(const DRAWITEMSTRUCT *dis)
{
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem, rcText;
    INT i = (INT)dis->itemData;
    BOOL bHot = ((INT)dis->itemID == g_HotCard) || (dis->itemState & ODS_FOCUS);
    INT IconSize = DpiScale(32);
    WCHAR szDesc[256];

    FillRect(hdc, &rc, g_hbrWindow);
    rc.bottom -= DpiScale(4);   /* gap between cards */
    FillRounded(hdc, &rc, DpiScale(8), bHot ? CLR_CARD_HOT : CLR_CARD, CLR_CARD_BORDER);

    DrawIconEx(hdc, rc.left + DpiScale(18), rc.top + (rc.bottom - rc.top - IconSize) / 2,
               GetSettingIcon(i), IconSize, IconSize, 0, NULL, DI_NORMAL);

    SetBkMode(hdc, TRANSPARENT);

    rcText = rc;
    rcText.left += DpiScale(18) + IconSize + DpiScale(16);
    rcText.right -= DpiScale(48);
    rcText.bottom = rc.top + (rc.bottom - rc.top) / 2 + DpiScale(1);
    SetTextColor(hdc, CLR_TEXT);
    SelectObject(hdc, g_hFont);
    DrawTextW(hdc, g_SearchItems[i].Title, -1, &rcText, DT_SINGLELINE | DT_BOTTOM | DT_END_ELLIPSIS | DT_NOPREFIX);

    /* In search results, say where the setting lives */
    if (g_szQuery[0])
        StringCchPrintfW(szDesc, _countof(szDesc), L"%s  \x203A  %s",
                         g_Categories[g_Settings[i].Category].Name, g_SearchItems[i].Description);
    else
        StringCchCopyW(szDesc, _countof(szDesc), g_SearchItems[i].Description);

    rcText.top = rcText.bottom + DpiScale(2);
    rcText.bottom = rc.bottom;
    SetTextColor(hdc, CLR_TEXT_2);
    DrawTextW(hdc, szDesc, -1, &rcText, DT_SINGLELINE | DT_TOP | DT_END_ELLIPSIS | DT_NOPREFIX);

    /* Chevron */
    rcText = rc;
    rcText.left = rc.right - DpiScale(40);
    SelectObject(hdc, g_hFontChevron);
    DrawTextW(hdc, L"\x203A", 1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
}

static VOID
PaintMain(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc, rcText;
    WCHAR szFormat[128], szHeader[400];

    GetClientRect(hwnd, &rc);
    FillRect(hdc, &ps.rcPaint, g_hbrWindow);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, CLR_TEXT);

    /* App title, top left */
    LoadStringW(g_hInstance, IDS_APPTITLE, szFormat, _countof(szFormat));
    SetRect(&rcText, DpiScale(MARGIN + 4), DpiScale(10), DpiScale(NAV_WIDTH), DpiScale(40));
    SelectObject(hdc, g_hFontBold);
    DrawTextW(hdc, szFormat, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

    /* Page header */
    if (g_szQuery[0])
    {
        BOOL bEmpty = SendMessageW(g_hCards, LB_GETCOUNT, 0, 0) == 0;
        LoadStringW(g_hInstance, bEmpty ? IDS_NO_RESULTS : IDS_RESULTS_FOR, szFormat, _countof(szFormat));
        StringCchPrintfW(szHeader, _countof(szHeader), szFormat, g_szQuery);
        if (bEmpty)
        {
            WCHAR szHint[200];
            RECT rcHint;
            LoadStringW(g_hInstance, IDS_NO_RESULTS_HINT, szHint, _countof(szHint));
            SetRect(&rcHint, DpiScale(NAV_WIDTH + 4), DpiScale(HEADER_H + 4), rc.right - DpiScale(MARGIN), DpiScale(HEADER_H + 40));
            SelectObject(hdc, g_hFont);
            SetTextColor(hdc, CLR_TEXT_2);
            DrawTextW(hdc, szHint, -1, &rcHint, DT_SINGLELINE | DT_NOPREFIX);
            SetTextColor(hdc, CLR_TEXT);
        }
    }
    else
    {
        StringCchCopyW(szHeader, _countof(szHeader), g_Categories[g_CurrentCategory].Name);
    }
    SetRect(&rcText, DpiScale(NAV_WIDTH + 4), DpiScale(18), rc.right - DpiScale(MARGIN), DpiScale(HEADER_H - 8));
    SelectObject(hdc, g_hFontHeader);
    DrawTextW(hdc, szHeader, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

    /* Thin rounded frame around the search box (Windows 11 text box look) */
    {
        RECT rcSearch;
        GetWindowRect(g_hSearch, &rcSearch);
        MapWindowPoints(NULL, hwnd, (LPPOINT)&rcSearch, 2);
        InflateRect(&rcSearch, DpiScale(10), DpiScale(7));
        FillRounded(hdc, &rcSearch, DpiScale(8), CLR_FIELD, CLR_CARD_BORDER);
        /* accent underline while typing */
        if (GetFocus() == g_hSearch)
        {
            RECT rcLine = rcSearch;
            rcLine.left += DpiScale(4);
            rcLine.right -= DpiScale(4);
            rcLine.top = rcLine.bottom - DpiScale(2);
            FillRounded(hdc, &rcLine, 2, CLR_ACCENT, CLR_ACCENT);
        }
    }

    EndPaint(hwnd, &ps);
}

/* ------------------------------------------------------------------------ */
/* Input                                                                     */

static LRESULT CALLBACK
SearchProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_GETDLGCODE)
    {
        /* Keep Enter/Escape for ourselves instead of IsDialogMessage */
        return CallWindowProcW(g_pfnOldSearchProc, hwnd, uMsg, wParam, lParam) | DLGC_WANTALLKEYS;
    }
    else if (uMsg == WM_KEYDOWN)
    {
        if (wParam == VK_RETURN)
        {
            /* Enter runs the best match */
            if (SendMessageW(g_hCards, LB_GETCOUNT, 0, 0) > 0)
                LaunchSetting((INT)SendMessageW(g_hCards, LB_GETITEMDATA, 0, 0));
            return 0;
        }
        if (wParam == VK_DOWN)
        {
            SetFocus(g_hCards);
            SendMessageW(g_hCards, LB_SETCURSEL, 0, 0);
            return 0;
        }
        if (wParam == VK_ESCAPE)
        {
            SetWindowTextW(hwnd, L"");
            return 0;
        }
    }
    else if (uMsg == WM_CHAR && (wParam == '\r' || wParam == 27))
    {
        return 0; /* no beep */
    }
    else if (uMsg == WM_SETFOCUS || uMsg == WM_KILLFOCUS)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        MapWindowPoints(NULL, g_hMainWnd, (LPPOINT)&rc, 2);
        InflateRect(&rc, DpiScale(12), DpiScale(9));
        InvalidateRect(g_hMainWnd, &rc, TRUE);
    }
    return CallWindowProcW(g_pfnOldSearchProc, hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK
CardsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_GETDLGCODE:
            return CallWindowProcW(g_pfnOldCardsProc, hwnd, uMsg, wParam, lParam) | DLGC_WANTALLKEYS;

        case WM_MOUSEMOVE:
        {
            DWORD dw = (DWORD)SendMessageW(hwnd, LB_ITEMFROMPOINT, 0, lParam);
            INT Hot = HIWORD(dw) ? -1 : LOWORD(dw);
            if (Hot != g_HotCard)
            {
                RECT rc;
                if (g_HotCard >= 0 && SendMessageW(hwnd, LB_GETITEMRECT, g_HotCard, (LPARAM)&rc) != LB_ERR)
                    InvalidateRect(hwnd, &rc, FALSE);
                g_HotCard = Hot;
                if (g_HotCard >= 0 && SendMessageW(hwnd, LB_GETITEMRECT, g_HotCard, (LPARAM)&rc) != LB_ERR)
                    InvalidateRect(hwnd, &rc, FALSE);
                if (Hot >= 0)
                {
                    TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                    TrackMouseEvent(&tme);
                }
            }
            break;
        }
        case WM_MOUSELEAVE:
            g_HotCard = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_LBUTTONUP:
        {
            /* Like Windows 11: a single click opens the setting */
            DWORD dw = (DWORD)SendMessageW(hwnd, LB_ITEMFROMPOINT, 0, lParam);
            if (!HIWORD(dw))
                LaunchSetting((INT)SendMessageW(hwnd, LB_GETITEMDATA, LOWORD(dw), 0));
            break;
        }
        case WM_KEYDOWN:
            if (wParam == VK_RETURN || wParam == VK_SPACE)
            {
                INT Sel = (INT)SendMessageW(hwnd, LB_GETCURSEL, 0, 0);
                if (Sel >= 0)
                    LaunchSetting((INT)SendMessageW(hwnd, LB_GETITEMDATA, Sel, 0));
                return 0;
            }
            if (wParam == VK_UP && SendMessageW(hwnd, LB_GETCURSEL, 0, 0) == 0)
            {
                SetFocus(g_hSearch);
                return 0;
            }
            break;
    }
    return CallWindowProcW(g_pfnOldCardsProc, hwnd, uMsg, wParam, lParam);
}

/* ------------------------------------------------------------------------ */

static VOID
Layout(HWND hwnd)
{
    RECT rc;
    INT SearchTop = DpiScale(52), SearchH = DpiScale(20);

    GetClientRect(hwnd, &rc);
    MoveWindow(g_hSearch, DpiScale(MARGIN + 12), SearchTop, DpiScale(NAV_WIDTH - MARGIN * 2 - 24), SearchH, TRUE);
    MoveWindow(g_hNav, DpiScale(MARGIN - 4), SearchTop + SearchH + DpiScale(24),
               DpiScale(NAV_WIDTH - MARGIN * 2 + 8), rc.bottom - (SearchTop + SearchH + DpiScale(24)) - DpiScale(MARGIN), TRUE);
    MoveWindow(g_hCards, DpiScale(NAV_WIDTH), DpiScale(HEADER_H),
               rc.right - DpiScale(NAV_WIDTH) - DpiScale(MARGIN * 2), rc.bottom - DpiScale(HEADER_H) - DpiScale(MARGIN), TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
}

static LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            WCHAR szCue[64];
            INT i;

            g_hMainWnd = hwnd;
            g_hSearch = CreateWindowExW(0, WC_EDITW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                        0, 0, 0, 0, hwnd, (HMENU)IDC_SEARCH, g_hInstance, NULL);
            g_hNav = CreateWindowExW(0, WC_LISTBOXW, NULL,
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_OWNERDRAWFIXED | LBS_NOTIFY |
                                     LBS_NOINTEGRALHEIGHT,
                                     0, 0, 0, 0, hwnd, (HMENU)IDC_NAV, g_hInstance, NULL);
            g_hCards = CreateWindowExW(0, WC_LISTBOXW, NULL,
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_OWNERDRAWFIXED |
                                       LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                                       0, 0, 0, 0, hwnd, (HMENU)IDC_CARDS, g_hInstance, NULL);

            SendMessageW(g_hSearch, WM_SETFONT, (WPARAM)g_hFont, FALSE);
            LoadStringW(g_hInstance, IDS_SEARCH_CUE, szCue, _countof(szCue));
            SendMessageW(g_hSearch, EM_SETCUEBANNER, TRUE, (LPARAM)szCue);
            g_pfnOldSearchProc = (WNDPROC)SetWindowLongPtrW(g_hSearch, GWLP_WNDPROC, (LONG_PTR)SearchProc);
            g_pfnOldCardsProc = (WNDPROC)SetWindowLongPtrW(g_hCards, GWLP_WNDPROC, (LONG_PTR)CardsProc);

            for (i = 0; i < CAT_COUNT; i++)
                SendMessageW(g_hNav, LB_ADDSTRING, 0, i);
            SendMessageW(g_hNav, LB_SETCURSEL, 0, 0);

            Layout(hwnd);
            RefreshCards();
            return 0;
        }

        case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
            mis->itemHeight = DpiScale(mis->CtlID == IDC_NAV ? NAV_ITEM_H : CARD_H);
            return TRUE;
        }

        case WM_DRAWITEM:
        {
            const DRAWITEMSTRUCT *dis = (const DRAWITEMSTRUCT *)lParam;
            if (dis->itemID == (UINT)-1)
                return TRUE;
            if (dis->CtlID == IDC_NAV)
                DrawNavItem(dis);
            else
                DrawCard(dis);
            return TRUE;
        }

        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLOREDIT:
            if ((HWND)lParam == g_hSearch)
            {
                SetTextColor((HDC)wParam, CLR_TEXT);
                SetBkColor((HDC)wParam, CLR_FIELD);
                return (LRESULT)g_hbrField;
            }
            SetTextColor((HDC)wParam, CLR_TEXT);
            SetBkColor((HDC)wParam, CLR_WINDOW);
            return (LRESULT)g_hbrWindow;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_SEARCH && HIWORD(wParam) == EN_CHANGE)
            {
                GetWindowTextW(g_hSearch, g_szQuery, _countof(g_szQuery));
                RefreshCards();
                InvalidateRect(g_hNav, NULL, FALSE);
            }
            else if (LOWORD(wParam) == IDC_NAV && HIWORD(wParam) == LBN_SELCHANGE)
            {
                g_CurrentCategory = (INT)SendMessageW(g_hNav, LB_GETCURSEL, 0, 0);
                if (g_szQuery[0])
                    SetWindowTextW(g_hSearch, L"");   /* leaves search mode, refreshes */
                else
                    RefreshCards();
                InvalidateRect(g_hNav, NULL, FALSE);
            }
            return 0;

        case WM_PAINT:
            PaintMain(hwnd);
            return 0;

        case WM_APP_SCALEMENU:
            ShowScaleMenu();
            return 0;

        case WM_THEMECHANGED:
        case WM_SYSCOLORCHANGE:
            /* switched between the light and dark color scheme */
            RefreshUiColors();
            InvalidateRect(hwnd, NULL, TRUE);
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
            break;

        case WM_DPICHANGED:
        {
            /* The display scale changed (sent by the system, like on Windows):
             * rebuild fonts, icons, row heights and layout, then take the
             * suggested window size */
            const RECT *prc = (const RECT *)lParam;
            INT i;
            {
                UpdateDpi();
                for (i = 0; i < CAT_COUNT; i++)
                {
                    if (g_Categories[i].hIcon)
                        DestroyIcon(g_Categories[i].hIcon);
                    g_Categories[i].hIcon = NULL;
                }
                for (i = 0; i < SETTING_COUNT; i++)
                {
                    if (g_Settings[i].hIcon)
                        DestroyIcon(g_Settings[i].hIcon);
                    g_Settings[i].hIcon = NULL;
                }
                DestroyFonts();
                CreateFonts();
                SendMessageW(g_hSearch, WM_SETFONT, (WPARAM)g_hFont, TRUE);
                SendMessageW(g_hNav, LB_SETITEMHEIGHT, 0, DpiScale(NAV_ITEM_H));
                SendMessageW(g_hCards, LB_SETITEMHEIGHT, 0, DpiScale(CARD_H));
                SetWindowPos(hwnd, NULL, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
                Layout(hwnd);
                InvalidateRect(g_hNav, NULL, TRUE);
                InvalidateRect(g_hCards, NULL, TRUE);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE:
            Layout(hwnd);
            return 0;

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *mmi = (MINMAXINFO *)lParam;
            mmi->ptMinTrackSize.x = DpiScale(640);
            mmi->ptMinTrackSize.y = DpiScale(420);
            return 0;
        }

        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_INACTIVE)
                SetFocus(g_hSearch);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEXW wc = { sizeof(wc) };
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    WCHAR szTitle[64];
    LPWSTR *argv;
    int argc, i;
    MSG msg;
    HWND hwnd, hwndExisting;

    g_hInstance = hInstance;
    InitCommonControlsEx(&icc);

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    /* settings.exe /open <words>: run the best match directly, no window */
    if (argv && argc >= 3 && !_wcsicmp(argv[1], L"/open"))
    {
        INT Result;
        WCHAR szQuery[256] = L"";
        for (i = 2; i < argc; i++)
        {
            StringCchCatW(szQuery, _countof(szQuery), argv[i]);
            StringCchCatW(szQuery, _countof(szQuery), L" ");
        }
        if (SearchItems(g_SearchItems, SETTING_COUNT, szQuery, &Result, 1) == 1)
            LaunchSetting(Result);
        LocalFree(argv);
        return 0;
    }

    /* Only one Settings window */
    LoadStringW(hInstance, IDS_APPTITLE, szTitle, _countof(szTitle));
    hwndExisting = FindWindowW(L"ReactOSSettingsWnd", NULL);
    if (hwndExisting && !(argv && argc >= 3))
    {
        if (IsIconic(hwndExisting))
            ShowWindow(hwndExisting, SW_RESTORE);
        SetForegroundWindow(hwndExisting);
        LocalFree(argv);
        return 0;
    }

    for (i = 0; i < SETTING_COUNT; i++)
        g_SearchItems[i].Category = g_Categories[g_Settings[i].Category].Name;

    UpdateDpi();
    CreateFonts();
    RefreshUiColors();

    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"ReactOSSettingsWnd";
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_SETTINGS));
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_SETTINGS), IMAGE_ICON,
                                   GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    if (!RegisterClassExW(&wc))
        return 1;

    /* 980x660 like Windows 11, but never larger than the work area */
    {
        RECT rcWork;
        INT cx = DpiScale(980), cy = DpiScale(660);
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);
        if (cx > (rcWork.right - rcWork.left) * 95 / 100)
            cx = (rcWork.right - rcWork.left) * 95 / 100;
        if (cy > (rcWork.bottom - rcWork.top) * 95 / 100)
            cy = (rcWork.bottom - rcWork.top) * 95 / 100;
        hwnd = CreateWindowExW(0, wc.lpszClassName, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                               rcWork.left + (rcWork.right - rcWork.left - cx) / 2,
                               rcWork.top + (rcWork.bottom - rcWork.top - cy) / 2,
                               cx, cy, NULL, NULL, hInstance, NULL);
    }
    if (!hwnd)
        return 1;

    /* settings.exe /search <words> */
    if (argv && argc >= 3 && !_wcsicmp(argv[1], L"/search"))
    {
        WCHAR szQuery[256] = L"";
        for (i = 2; i < argc; i++)
        {
            if (i > 2)
                StringCchCatW(szQuery, _countof(szQuery), L" ");
            StringCchCatW(szQuery, _countof(szQuery), argv[i]);
        }
        SetWindowTextW(g_hSearch, szQuery);
        SendMessageW(g_hSearch, EM_SETSEL, wcslen(szQuery), wcslen(szQuery));
    }
    if (argv)
        LocalFree(argv);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!IsDialogMessageW(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return (int)msg.wParam;
}
