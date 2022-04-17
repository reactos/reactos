/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS uxtheme.dll
 * FILE:            dll/win32/uxtheme/themehooks.c
 * PURPOSE:         uxtheme user api hook functions
 * PROGRAMMER:      Giannis Adamopoulos
 */

#include "uxthemep.h"

USERAPIHOOK g_user32ApiHook;
BYTE gabDWPmessages[UAHOWP_MAX_SIZE];
BYTE gabMSGPmessages[UAHOWP_MAX_SIZE];
BYTE gabDLGPmessages[UAHOWP_MAX_SIZE];
BOOL g_bThemeHooksActive = FALSE;

PWND_DATA ThemeGetWndData(HWND hWnd)
{
    PWND_DATA pwndData;

    pwndData = (PWND_DATA)GetPropW(hWnd, (LPCWSTR)MAKEINTATOM(atWndContext));
    if(pwndData == NULL)
    {
        pwndData = HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(WND_DATA));
        if(pwndData == NULL)
        {
            return NULL;
        }

        SetPropW( hWnd, (LPCWSTR)MAKEINTATOM(atWndContext), pwndData);
    }

    return pwndData;
}

void ThemeDestroyWndData(HWND hWnd)
{
    PWND_DATA pwndData;
    DWORD ProcessId;

    /*Do not destroy WND_DATA of a window that belong to another process */
    GetWindowThreadProcessId(hWnd, &ProcessId);
    if(ProcessId != GetCurrentProcessId())
    {
        return;
    }

    pwndData = (PWND_DATA)GetPropW(hWnd, (LPCWSTR)MAKEINTATOM(atWndContext));
    if(pwndData == NULL)
    {
        return;
    }

    if(pwndData->HasThemeRgn)
    {
        g_user32ApiHook.SetWindowRgn(hWnd, 0, TRUE);
    }

    if (pwndData->hTabBackgroundBrush != NULL)
    {
        CloseThemeData(GetWindowTheme(hWnd));

        DeleteObject(pwndData->hTabBackgroundBrush);
    }

    if (pwndData->hTabBackgroundBmp != NULL)
    {
        DeleteObject(pwndData->hTabBackgroundBmp);
    }

    if (pwndData->hthemeWindow)
    {
        CloseThemeData(pwndData->hthemeWindow);
    }

    if (pwndData->hthemeScrollbar)
    {
        CloseThemeData(pwndData->hthemeScrollbar);
    }

    HeapFree(GetProcessHeap(), 0, pwndData);

    SetPropW( hWnd, (LPCWSTR)MAKEINTATOM(atWndContext), NULL);
}

HTHEME GetNCCaptionTheme(HWND hWnd, DWORD style)
{
    PWND_DATA pwndData;

    /* We only get the theme for the window class if the window has a caption */
    if((style & WS_CAPTION) != WS_CAPTION)
        return NULL;

    /* Get theme data for this window */
    pwndData = ThemeGetWndData(hWnd);
    if (pwndData == NULL)
        return NULL;

    if (!(GetThemeAppProperties() & STAP_ALLOW_NONCLIENT))
    {
        if (pwndData->hthemeWindow)
        {
            CloseThemeData(pwndData->hthemeWindow);
            pwndData->hthemeWindow = NULL;
        }
        return NULL;
    }

    /* If the theme data was not cached, open it now */
    if (!pwndData->hthemeWindow)
        pwndData->hthemeWindow = OpenThemeDataEx(hWnd, L"WINDOW", OTD_NONCLIENT);

    return pwndData->hthemeWindow;
}

HTHEME GetNCScrollbarTheme(HWND hWnd, DWORD style)
{
    PWND_DATA pwndData;

    /* We only get the theme for the scrollbar class if the window has a scrollbar */
    if((style & (WS_HSCROLL|WS_VSCROLL)) == 0)
        return NULL;

    /* Get theme data for this window */
    pwndData = ThemeGetWndData(hWnd);
    if (pwndData == NULL)
        return NULL;

    if (!(GetThemeAppProperties() & STAP_ALLOW_NONCLIENT))
    {
        if (pwndData->hthemeScrollbar)
        {
            CloseThemeData(pwndData->hthemeScrollbar);
            pwndData->hthemeScrollbar = NULL;
        }
        return NULL;
    }

    /* If the theme data was not cached, open it now */
    if (!pwndData->hthemeScrollbar)
        pwndData->hthemeScrollbar = OpenThemeDataEx(hWnd, L"SCROLLBAR", OTD_NONCLIENT);

    return pwndData->hthemeScrollbar;
}

static BOOL CALLBACK ThemeCleanupChildWndContext (HWND hWnd, LPARAM msg)
{
    ThemeDestroyWndData(hWnd);
    return TRUE;
}

static BOOL CALLBACK ThemeCleanupWndContext(HWND hWnd, LPARAM msg)
{
    if (hWnd == NULL)
    {
        EnumWindows (ThemeCleanupWndContext, 0);
    }
    else
    {
        ThemeDestroyWndData(hWnd);
        EnumChildWindows (hWnd, ThemeCleanupChildWndContext, 0);
    }

    return TRUE;
}

void SetThemeRegion(HWND hWnd)
{
    HTHEME hTheme;
    RECT rcWindow;
    HRGN hrgn, hrgn1;
    int CaptionHeight, iPart;
    WINDOWINFO wi;

    TRACE("SetThemeRegion %d\n", hWnd);

    wi.cbSize = sizeof(wi);
    GetWindowInfo(hWnd, &wi);

    /* Get the caption part id */
    if (wi.dwStyle & WS_MINIMIZE)
        iPart = WP_MINCAPTION;
    else if (wi.dwExStyle & WS_EX_TOOLWINDOW)
        iPart = WP_SMALLCAPTION;
    else if (wi.dwStyle & WS_MAXIMIZE)
        iPart = WP_MAXCAPTION;
    else
        iPart = WP_CAPTION;

    CaptionHeight = wi.cyWindowBorders;
    CaptionHeight += GetSystemMetrics(wi.dwExStyle & WS_EX_TOOLWINDOW ? SM_CYSMCAPTION : SM_CYCAPTION );

    GetWindowRect(hWnd, &rcWindow);
    rcWindow.right -= rcWindow.left;
    rcWindow.bottom = CaptionHeight;
    rcWindow.top = 0;
    rcWindow.left = 0;

    hTheme = GetNCCaptionTheme(hWnd, wi.dwStyle);
    GetThemeBackgroundRegion(hTheme, 0, iPart, FS_ACTIVE, &rcWindow, &hrgn);

    GetWindowRect(hWnd, &rcWindow);
    rcWindow.right -= rcWindow.left;
    rcWindow.bottom -= rcWindow.top;
    rcWindow.top = CaptionHeight;
    rcWindow.left = 0;
    hrgn1 = CreateRectRgnIndirect(&rcWindow);

    CombineRgn(hrgn, hrgn, hrgn1, RGN_OR );

    DeleteObject(hrgn1);

    g_user32ApiHook.SetWindowRgn(hWnd, hrgn, TRUE);
}

int OnPostWinPosChanged(HWND hWnd, WINDOWPOS* pWinPos)
{
    PWND_DATA pwndData;
    DWORD style;

    /* We only proceed to change the window shape if it has a caption */
    style = GetWindowLongW(hWnd, GWL_STYLE);
    if((style & WS_CAPTION)!=WS_CAPTION)
        return 0;

    /* Get theme data for this window */
    pwndData = ThemeGetWndData(hWnd);
    if (pwndData == NULL)
        return 0;

    /* Do not change the region of the window if its size wasn't changed */
    if ((pWinPos->flags & SWP_NOSIZE) != 0 && pwndData->DirtyThemeRegion == FALSE)
        return 0;

    /* We don't touch the shape of the window if the application sets it on its own */
    if (pwndData->HasAppDefinedRgn != FALSE)
        return 0;

    /* Calling SetWindowRgn will call SetWindowPos again so we need to avoid this recursion */
    if (pwndData->UpdatingRgn != FALSE)
        return 0;

    if(!IsAppThemed() || !(GetThemeAppProperties() & STAP_ALLOW_NONCLIENT))
    {
        if(pwndData->HasThemeRgn)
        {
            pwndData->HasThemeRgn = FALSE;
            g_user32ApiHook.SetWindowRgn(hWnd, 0, TRUE);
        }
        return 0;
    }

    pwndData->DirtyThemeRegion = FALSE;
    pwndData->HasThemeRgn = TRUE;
    pwndData->UpdatingRgn = TRUE;
    SetThemeRegion(hWnd);
    pwndData->UpdatingRgn = FALSE;

     return 0;
 }

/**********************************************************************
 *      Hook Functions
 */

static LRESULT CALLBACK
ThemeDefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PWND_DATA pwndData;

    pwndData = (PWND_DATA)GetPropW(hWnd, (LPCWSTR)MAKEINTATOM(atWndContext));

    if(!IsAppThemed() ||
       !(GetThemeAppProperties() & STAP_ALLOW_NONCLIENT) ||
       (pwndData && pwndData->HasAppDefinedRgn))
    {
        return g_user32ApiHook.DefWindowProcW(hWnd,
                                            Msg,
                                            wParam,
                                            lParam);
    }

    return ThemeWndProc(hWnd,
                        Msg,
                        wParam,
                        lParam,
                        g_user32ApiHook.DefWindowProcW);
}

static LRESULT CALLBACK
ThemeDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PWND_DATA pwndData;

    pwndData = (PWND_DATA)GetPropW(hWnd, (LPCWSTR)MAKEINTATOM(atWndContext));

    if(!IsAppThemed() ||
       !(GetThemeAppProperties() & STAP_ALLOW_NONCLIENT) ||
       (pwndData && pwndData->HasAppDefinedRgn))
    {
        return g_user32ApiHook.DefWindowProcA(hWnd,
                                            Msg,
                                            wParam,
                                            lParam);
    }

    return ThemeWndProc(hWnd,
                        Msg,
                        wParam,
                        lParam,
                        g_user32ApiHook.DefWindowProcA);
}

static LRESULT CALLBACK
ThemePreWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, ULONG_PTR ret,PDWORD unknown)
{
    switch(Msg)
    {
        case WM_CREATE:
        case WM_STYLECHANGED:
        case WM_SIZE:
        case WM_WINDOWPOSCHANGED:
        {
            if(IsAppThemed() && (GetThemeAppProperties() & STAP_ALLOW_NONCLIENT))
                ThemeCalculateCaptionButtonsPos(hWnd, NULL);
            break;
        }
        case WM_THEMECHANGED:
        {
            PWND_DATA pwndData = ThemeGetWndData(hWnd);

            if (GetAncestor(hWnd, GA_PARENT) == GetDesktopWindow())
                UXTHEME_LoadTheme(TRUE);

            if (pwndData == NULL)
                return 0;

            if (pwndData->hTabBackgroundBrush != NULL)
            {
                DeleteObject(pwndData->hTabBackgroundBrush);
                pwndData->hTabBackgroundBrush = NULL;
            }

            if (pwndData->hTabBackgroundBmp != NULL)
            {
                DeleteObject(pwndData->hTabBackgroundBmp);
                pwndData->hTabBackgroundBmp = NULL;
            }

            if (pwndData->hthemeWindow)
            {
                CloseThemeData(pwndData->hthemeWindow);
                pwndData->hthemeWindow = NULL;
            }

            if (pwndData->hthemeScrollbar)
            {
                CloseThemeData(pwndData->hthemeScrollbar);
                pwndData->hthemeScrollbar = NULL;
            }

            if(IsAppThemed() && (GetThemeAppProperties() & STAP_ALLOW_NONCLIENT))
                ThemeCalculateCaptionButtonsPos(hWnd, NULL);

            pwndData->DirtyThemeRegion = TRUE;
            break;
        }
        case WM_NCCREATE:
        {
            PWND_DATA pwndData = ThemeGetWndData(hWnd);
            if (pwndData == NULL)
                return 0;
            pwndData->DirtyThemeRegion = TRUE;
        }
    }

    return 0;
}


static LRESULT CALLBACK
ThemePostWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, ULONG_PTR ret,PDWORD unknown)
{
    switch(Msg)
    {
        case WM_WINDOWPOSCHANGED:
        {
            return OnPostWinPosChanged(hWnd, (WINDOWPOS*)lParam);
        }
        case WM_NCDESTROY:
        {
            ThemeDestroyWndData(hWnd);
            return 0;
        }
    }

    return 0;
}

HRESULT GetDiaogTextureBrush(HTHEME theme, HWND hwnd, HDC hdc, HBRUSH* result, BOOL changeOrigin)
{
    PWND_DATA pwndData;

    pwndData = ThemeGetWndData(hwnd);
    if (pwndData == NULL)
        return E_FAIL;

    if (pwndData->hTabBackgroundBrush == NULL)
    {
        HBITMAP hbmp;
        RECT dummy, bmpRect;
        BOOL hasImageAlpha;
        HRESULT hr;

        hr = UXTHEME_LoadImage(theme, 0, TABP_BODY, 0, &dummy, FALSE, &hbmp, &bmpRect, &hasImageAlpha);
        if (FAILED(hr))
            return hr;

        if (changeOrigin)
        {
            /* Unfortunately SetBrushOrgEx doesn't work at all */
            RECT rcWindow, rcParent;
            UINT y;
            HDC hdcPattern, hdcHackPattern;
            HBITMAP hbmpOld1, hbmpold2, hbmpHack;

            GetWindowRect(hwnd, &rcWindow);
            GetWindowRect(GetParent(hwnd), &rcParent);
            y = (rcWindow.top - rcParent.top) % bmpRect.bottom;

            hdcPattern = CreateCompatibleDC(hdc);
            hbmpOld1 = (HBITMAP)SelectObject(hdcPattern, hbmp);

            hdcHackPattern = CreateCompatibleDC(hdc);
            hbmpHack = CreateCompatibleBitmap(hdc, bmpRect.right, bmpRect.bottom);
            hbmpold2 = (HBITMAP)SelectObject(hdcHackPattern, hbmpHack);

            BitBlt(hdcHackPattern, 0, 0, bmpRect.right, bmpRect.bottom - y, hdcPattern, 0, y, SRCCOPY);
            BitBlt(hdcHackPattern, 0, bmpRect.bottom - y, bmpRect.right, y, hdcPattern, 0, 0, SRCCOPY);

            hbmpold2 = (HBITMAP)SelectObject(hdcHackPattern, hbmpold2);
            hbmpOld1 = (HBITMAP)SelectObject(hdcPattern, hbmpOld1);

            DeleteDC(hdcPattern);
            DeleteDC(hdcHackPattern);

            /* Keep the handle of the bitmap we created so that it can be used later */
            pwndData->hTabBackgroundBmp = hbmpHack;
            hbmp = hbmpHack;
        }

        /* hbmp is cached so there is no need to free it */
        pwndData->hTabBackgroundBrush = CreatePatternBrush(hbmp);
    }

    if (!pwndData->hTabBackgroundBrush)
        return E_FAIL;

    *result = pwndData->hTabBackgroundBrush;
    return S_OK;
}

void HackFillStaticBg(HWND hwnd, HDC hdc, HBRUSH* result)
{
    RECT rcStatic;

    GetClientRect(hwnd, &rcStatic);
    FillRect(hdc, &rcStatic, *result);

    SetBkMode (hdc, TRANSPARENT);
    *result = GetStockObject (NULL_BRUSH);
}

static LRESULT CALLBACK
ThemeDlgPreWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, ULONG_PTR ret,PDWORD unknown)
{
    return 0;
}

static LRESULT CALLBACK
ThemeDlgPostWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, ULONG_PTR ret,PDWORD unknown)
{
    switch(Msg)
    {
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC:
        {
            HWND hwndTarget = (HWND)lParam;
            HDC hdc = (HDC)wParam;
            HBRUSH* phbrush = (HBRUSH*)ret;
            HTHEME hTheme;

            if(!IsAppThemed() || !(GetThemeAppProperties() & STAP_ALLOW_NONCLIENT))
                break;

            if (!IsThemeDialogTextureEnabled (hWnd))
                break;

            hTheme = GetWindowTheme(hWnd);
            if (!hTheme)
                hTheme = OpenThemeData(hWnd, L"TAB");

            if (!hTheme)
                break;

            GetDiaogTextureBrush(hTheme, hwndTarget, hdc, phbrush, Msg != WM_CTLCOLORDLG);

#if 1
            {
                WCHAR controlClass[32];
                GetClassNameW (hwndTarget, controlClass, sizeof(controlClass) / sizeof(controlClass[0]));

                /* This is a hack for the static class. Windows have a v6 static class just for this. */
                if (lstrcmpiW (controlClass, WC_STATICW) == 0)
                    HackFillStaticBg(hwndTarget, hdc, phbrush);
            }
#endif
            SetBkMode( hdc, TRANSPARENT );
            break;
        }
    }

    return 0;
}

int WINAPI ThemeSetWindowRgn(HWND hWnd, HRGN hRgn, BOOL bRedraw)
{
    PWND_DATA pwndData = ThemeGetWndData(hWnd);
    if(pwndData)
    {
        pwndData->HasAppDefinedRgn = TRUE;
        pwndData->HasThemeRgn = FALSE;
    }

    return g_user32ApiHook.SetWindowRgn(hWnd, hRgn, bRedraw);
}

BOOL WINAPI ThemeGetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
    PWND_DATA pwndData;
    DWORD style;
    BOOL ret;

    /* Avoid creating a window context if it is not needed */
    if(!IsAppThemed() || !(GetThemeAppProperties() & STAP_ALLOW_NONCLIENT))
        goto dodefault;

    style = GetWindowLongW(hwnd, GWL_STYLE);
    if((style & (WS_HSCROLL|WS_VSCROLL))==0)
        goto dodefault;

    pwndData = ThemeGetWndData(hwnd);
    if (pwndData == NULL)
        goto dodefault;

    /*
     * Uxtheme needs to handle the tracking of the scrollbar itself
     * This means than if an application needs to get the track position
     * with GetScrollInfo, it will get wrong data. So uxtheme needs to
     * hook it and set the correct tracking position itself
     */
    ret = g_user32ApiHook.GetScrollInfo(hwnd, fnBar, lpsi);
    if ( lpsi &&
        (lpsi->fMask & SIF_TRACKPOS) &&
         pwndData->SCROLL_TrackingWin == hwnd &&
         pwndData->SCROLL_TrackingBar == fnBar)
    {
        lpsi->nTrackPos = pwndData->SCROLL_TrackingVal;
    }
    return ret;

dodefault:
    return g_user32ApiHook.GetScrollInfo(hwnd, fnBar, lpsi);
}

INT WINAPI ThemeSetScrollInfo(HWND hWnd, int fnBar, LPCSCROLLINFO lpsi, BOOL bRedraw)
{
    PWND_DATA pwndData;
    SCROLLINFO siout;
    LPSCROLLINFO lpsiout = &siout;
    BOOL IsThemed = FALSE;

    pwndData = ThemeGetWndData(hWnd);

    if (!pwndData)
        goto dodefault;

    if (pwndData->hthemeScrollbar)
        IsThemed = TRUE;

    memcpy(&siout, lpsi, sizeof(SCROLLINFO));
    if (IsThemed)
        siout.fMask |= SIF_THEMED;

dodefault:
    return g_user32ApiHook.SetScrollInfo(hWnd, fnBar, lpsiout, bRedraw);
}

/**********************************************************************
 *      Exports
 */

BOOL CALLBACK
ThemeInitApiHook(UAPIHK State, PUSERAPIHOOK puah)
{
    if (!puah || State != uahLoadInit)
    {
        UXTHEME_LoadTheme(FALSE);
        ThemeCleanupWndContext(NULL, 0);
        g_bThemeHooksActive = FALSE;
        return TRUE;
    }

    g_bThemeHooksActive = TRUE;

    /* Store the original functions from user32 */
    g_user32ApiHook = *puah;

    puah->DefWindowProcA = ThemeDefWindowProcA;
    puah->DefWindowProcW = ThemeDefWindowProcW;
    puah->PreWndProc = ThemePreWindowProc;
    puah->PostWndProc = ThemePostWindowProc;
    puah->PreDefDlgProc = ThemeDlgPreWindowProc;
    puah->PostDefDlgProc = ThemeDlgPostWindowProc;
    puah->DefWndProcArray.MsgBitArray  = gabDWPmessages;
    puah->DefWndProcArray.Size = UAHOWP_MAX_SIZE;
    puah->WndProcArray.MsgBitArray = gabMSGPmessages;
    puah->WndProcArray.Size = UAHOWP_MAX_SIZE;
    puah->DlgProcArray.MsgBitArray = gabDLGPmessages;
    puah->DlgProcArray.Size = UAHOWP_MAX_SIZE;

    puah->SetWindowRgn = ThemeSetWindowRgn;
    puah->GetScrollInfo = ThemeGetScrollInfo;
    puah->SetScrollInfo = ThemeSetScrollInfo;

    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCPAINT);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCACTIVATE);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCMOUSEMOVE);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCMOUSELEAVE);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCHITTEST);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCLBUTTONDOWN);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCUAHDRAWCAPTION);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCUAHDRAWFRAME);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_SETTEXT);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_WINDOWPOSCHANGED);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_CONTEXTMENU);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_STYLECHANGED);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_SETICON);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_NCDESTROY);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_SYSCOMMAND);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_CTLCOLORMSGBOX);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_CTLCOLORBTN);
    UAH_HOOK_MESSAGE(puah->DefWndProcArray, WM_CTLCOLORSTATIC);

    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_CREATE);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_SETTINGCHANGE);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_DRAWITEM);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_MEASUREITEM);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_WINDOWPOSCHANGING);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_WINDOWPOSCHANGED);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_STYLECHANGING);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_STYLECHANGED);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_NCCREATE);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_NCDESTROY);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_NCPAINT);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_MENUCHAR);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_MDISETMENU);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_THEMECHANGED);
    UAH_HOOK_MESSAGE(puah->WndProcArray, WM_UAHINIT);

    puah->DlgProcArray.MsgBitArray = gabDLGPmessages;
    puah->DlgProcArray.Size = UAHOWP_MAX_SIZE;

    UAH_HOOK_MESSAGE(puah->DlgProcArray, WM_INITDIALOG);
    UAH_HOOK_MESSAGE(puah->DlgProcArray, WM_CTLCOLORMSGBOX);
    UAH_HOOK_MESSAGE(puah->DlgProcArray, WM_CTLCOLORBTN);
    UAH_HOOK_MESSAGE(puah->DlgProcArray, WM_CTLCOLORDLG);
    UAH_HOOK_MESSAGE(puah->DlgProcArray, WM_CTLCOLORSTATIC);
    UAH_HOOK_MESSAGE(puah->DlgProcArray, WM_PRINTCLIENT);

    UXTHEME_LoadTheme(TRUE);

    return TRUE;
}

typedef BOOL (WINAPI * PREGISTER_UAH_WINXP)(HINSTANCE hInstance, USERAPIHOOKPROC CallbackFunc);
typedef BOOL (WINAPI * PREGISTER_UUAH_WIN2003)(PUSERAPIHOOKINFO puah);

BOOL WINAPI
ThemeHooksInstall()
{
    PVOID lpFunc;
    OSVERSIONINFO osvi;
    BOOL ret;

    lpFunc = GetProcAddress(GetModuleHandle("user32.dll"), "RegisterUserApiHook");

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
    {
        PREGISTER_UAH_WINXP lpfuncxp = (PREGISTER_UAH_WINXP)lpFunc;
        ret = lpfuncxp(hDllInst, ThemeInitApiHook);
    }
    else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
    {
        PREGISTER_UUAH_WIN2003 lpfunc2003 = (PREGISTER_UUAH_WIN2003)lpFunc;
        USERAPIHOOKINFO uah;

        uah.m_size = sizeof(uah);
        uah.m_dllname1 = L"uxtheme.dll";
        uah.m_funname1 = L"ThemeInitApiHook";
        uah.m_dllname2 = NULL;
        uah.m_funname2 = NULL;

        ret = lpfunc2003(&uah);
    }
    else
    {
        UNIMPLEMENTED;
        ret = FALSE;
    }

    UXTHEME_broadcast_theme_changed (NULL, TRUE);

    return ret;
}

BOOL WINAPI
ThemeHooksRemove()
{
    BOOL ret;

    ret = UnregisterUserApiHook();

    UXTHEME_broadcast_theme_changed (NULL, FALSE);

    return ret;
}

INT WINAPI ClassicSystemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
    if (g_bThemeHooksActive)
    {
        return g_user32ApiHook.SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
    }

    return SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
}

INT WINAPI ClassicSystemParametersInfoA(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
    if (g_bThemeHooksActive)
    {
        return g_user32ApiHook.SystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);
    }

    return SystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);
}

INT WINAPI ClassicGetSystemMetrics(int nIndex)
{
    if (g_bThemeHooksActive)
    {
        return g_user32ApiHook.GetSystemMetrics(nIndex);
    }

    return GetSystemMetrics(nIndex);
}

BOOL WINAPI ClassicAdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
    if (g_bThemeHooksActive)
    {
        return g_user32ApiHook.AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
    }

    return AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
}
