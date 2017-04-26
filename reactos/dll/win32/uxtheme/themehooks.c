/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS uxtheme.dll
 * FILE:            dll/win32/uxtheme/themehooks.c
 * PURPOSE:         uxtheme user api hook functions
 * PROGRAMMER:      Giannis Adamopoulos
 */
 
#include "uxthemep.h"

USERAPIHOOK user32ApiHook;
BYTE gabDWPmessages[UAHOWP_MAX_SIZE];
BYTE gabMSGPmessages[UAHOWP_MAX_SIZE];
BYTE gabDLGPmessages[UAHOWP_MAX_SIZE];
BOOL gbThemeHooksActive = FALSE;

PWND_CONTEXT ThemeGetWndContext(HWND hWnd)
{
    PWND_CONTEXT pcontext;

    pcontext = (PWND_CONTEXT)GetPropW(hWnd, (LPCWSTR)MAKEINTATOM(atWndContext));
    if(pcontext == NULL)
    {
        pcontext = HeapAlloc(GetProcessHeap(), 
                            HEAP_ZERO_MEMORY, 
                            sizeof(WND_CONTEXT));
        if(pcontext == NULL)
        {
            return NULL;
        }
        
        SetPropW( hWnd, (LPCWSTR)MAKEINTATOM(atWndContext), pcontext);
    }

    return pcontext;
}

void ThemeDestroyWndContext(HWND hWnd)
{
    PWND_CONTEXT pContext;
    DWORD ProcessId;

    /*Do not destroy WND_CONTEXT of a window that belong to another process */
    GetWindowThreadProcessId(hWnd, &ProcessId);
    if(ProcessId != GetCurrentProcessId())
    {
        return;
    }

    pContext = (PWND_CONTEXT)GetPropW(hWnd, (LPCWSTR)MAKEINTATOM(atWndContext));
    if(pContext == NULL)
    {
        return;
    }

    if(pContext->HasThemeRgn)
    {
        user32ApiHook.SetWindowRgn(hWnd, 0, TRUE);
    }

    if (pContext->hTabBackgroundBrush != NULL)
    {
        CloseThemeData(GetWindowTheme(hWnd));

        DeleteObject(pContext->hTabBackgroundBrush);
        pContext->hTabBackgroundBrush = NULL;
    }

    if (pContext->hTabBackgroundBmp != NULL)
    {
        DeleteObject(pContext->hTabBackgroundBmp);
        pContext->hTabBackgroundBmp = NULL;
    }

    HeapFree(GetProcessHeap(), 0, pContext);

    SetPropW( hWnd, (LPCWSTR)MAKEINTATOM(atWndContext), NULL);
}

static BOOL CALLBACK ThemeCleanupChildWndContext (HWND hWnd, LPARAM msg)
{
    ThemeDestroyWndContext(hWnd);
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
        ThemeDestroyWndContext(hWnd);
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

    hTheme = MSSTYLES_OpenThemeClass(ActiveThemeFile, NULL, L"WINDOW");
    GetThemeBackgroundRegion(hTheme, 0, iPart, FS_ACTIVE, &rcWindow, &hrgn);
    CloseThemeData(hTheme);

    GetWindowRect(hWnd, &rcWindow);
    rcWindow.right -= rcWindow.left;
    rcWindow.bottom -= rcWindow.top;
    rcWindow.top = CaptionHeight;
    rcWindow.left = 0;
    hrgn1 = CreateRectRgnIndirect(&rcWindow);

    CombineRgn(hrgn, hrgn, hrgn1, RGN_OR );

    DeleteObject(hrgn1);

    user32ApiHook.SetWindowRgn(hWnd, hrgn, TRUE);
}

int OnPostWinPosChanged(HWND hWnd, WINDOWPOS* pWinPos)
{
    PWND_CONTEXT pcontext;
    DWORD style;

    /* We only proceed to change the window shape if it has a caption */
    style = GetWindowLongW(hWnd, GWL_STYLE);
    if((style & WS_CAPTION)!=WS_CAPTION)
        return 0;

    /* Get theme data for this window */
    pcontext = ThemeGetWndContext(hWnd);
    if (pcontext == NULL)
        return 0;

    /* Do not change the region of the window if its size wasn't changed */
    if ((pWinPos->flags & SWP_NOSIZE) != 0 && pcontext->DirtyThemeRegion == FALSE)
        return 0;

    /* We don't touch the shape of the window if the application sets it on its own */
    if (pcontext->HasAppDefinedRgn == TRUE)
        return 0;

    /* Calling SetWindowRgn will call SetWindowPos again so we need to avoid this recursion */
    if (pcontext->UpdatingRgn == TRUE)
        return 0;

    if(!IsAppThemed())
    {
        if(pcontext->HasThemeRgn)
        {
            pcontext->HasThemeRgn = FALSE;
            user32ApiHook.SetWindowRgn(hWnd, 0, TRUE);
        }
        return 0;
    }

    pcontext->DirtyThemeRegion = FALSE;
    pcontext->HasThemeRgn = TRUE;
    pcontext->UpdatingRgn = TRUE;
    SetThemeRegion(hWnd);
    pcontext->UpdatingRgn = FALSE;

     return 0;
 }

/**********************************************************************
 *      Hook Functions
 */

static LRESULT CALLBACK
ThemeDefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{      
    if(!IsAppThemed())
    {
        return user32ApiHook.DefWindowProcW(hWnd, 
                                            Msg, 
                                            wParam, 
                                            lParam);
    }

    return ThemeWndProc(hWnd, 
                        Msg, 
                        wParam, 
                        lParam, 
                        user32ApiHook.DefWindowProcW);
}

static LRESULT CALLBACK
ThemeDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if(!IsAppThemed())
    {
        return user32ApiHook.DefWindowProcA(hWnd, 
                                            Msg, 
                                            wParam, 
                                            lParam);
    }

    return ThemeWndProc(hWnd, 
                        Msg, 
                        wParam, 
                        lParam, 
                        user32ApiHook.DefWindowProcA);
}

static LRESULT CALLBACK
ThemePreWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, ULONG_PTR ret,PDWORD unknown)
{
    switch(Msg)
    {
        case WM_THEMECHANGED:
        {
            PWND_CONTEXT pcontext = ThemeGetWndContext(hWnd);

            if (GetAncestor(hWnd, GA_PARENT) == GetDesktopWindow())
                UXTHEME_LoadTheme(TRUE);

            if (pcontext == NULL)
                return 0;

            if (pcontext->hTabBackgroundBrush != NULL)
            {
                DeleteObject(pcontext->hTabBackgroundBrush);
                pcontext->hTabBackgroundBrush = NULL;
            }

            if (pcontext->hTabBackgroundBmp != NULL)
            {
                DeleteObject(pcontext->hTabBackgroundBmp);
                pcontext->hTabBackgroundBmp = NULL;
            }
        }
        case WM_NCCREATE:
        {
            PWND_CONTEXT pcontext = ThemeGetWndContext(hWnd);
            if (pcontext == NULL)
                return 0;
            pcontext->DirtyThemeRegion = TRUE;
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
            ThemeDestroyWndContext(hWnd);
            return 0;
        }
    }

    return 0;
}

HRESULT GetDiaogTextureBrush(HTHEME theme, HWND hwnd, HDC hdc, HBRUSH* result, BOOL changeOrigin)
{
    PWND_CONTEXT pcontext;

    pcontext = ThemeGetWndContext(hwnd);
    if (pcontext == NULL)
        return E_FAIL;

    if (pcontext->hTabBackgroundBrush == NULL)
    {
        HBITMAP hbmp;
        RECT dummy, bmpRect;
        BOOL hasImageAlpha;

        UXTHEME_LoadImage(theme, 0, TABP_BODY, 0, &dummy, FALSE, &hbmp, &bmpRect, &hasImageAlpha);
        if (changeOrigin)
        {
            /* Unfortunately SetBrushOrgEx doesn't work at all */
            RECT rcWindow, rcParent;
            POINT pt;
            HDC hdcPattern, hdcHackPattern;
            HBITMAP hbmpOld1, hbmpold2, hbmpHack;

            GetWindowRect(hwnd, &rcWindow);
            GetWindowRect(GetParent(hwnd), &rcParent);
            pt.x = rcWindow.left - rcParent.left;
            pt.y = rcWindow.top - rcParent.top;

            hdcPattern = CreateCompatibleDC(hdc);
            hbmpOld1 = (HBITMAP)SelectObject(hdcPattern, hbmp);

            hdcHackPattern = CreateCompatibleDC(hdc);
            hbmpHack = CreateCompatibleBitmap(hdc, bmpRect.right, bmpRect.bottom);
            hbmpold2 = (HBITMAP)SelectObject(hdcHackPattern, hbmpHack);

            BitBlt(hdcHackPattern, 0, 0, bmpRect.right, bmpRect.bottom - pt.y, hdcPattern, 0, pt.y, SRCCOPY);
            BitBlt(hdcHackPattern, 0, bmpRect.bottom - pt.y, bmpRect.right, pt.y, hdcPattern, 0, 0, SRCCOPY);

            hbmpold2 = (HBITMAP)SelectObject(hdcHackPattern, hbmpold2);
            hbmpOld1 = (HBITMAP)SelectObject(hdcPattern, hbmpOld1);

            DeleteDC(hdcPattern);
            DeleteDC(hdcHackPattern);

            /* Keep the handle of the bitmap we created so that it can be used later */
            pcontext->hTabBackgroundBmp = hbmpHack;
            hbmp = hbmpHack;
        }

        /* hbmp is cached so there is no need to free it */
        pcontext->hTabBackgroundBrush = CreatePatternBrush(hbmp);
    }

    if (!pcontext->hTabBackgroundBrush)
        return E_FAIL;

    *result = pcontext->hTabBackgroundBrush;
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

            if (!IsAppThemed())
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
            break;
        }
    }

    return 0;
}

int WINAPI ThemeSetWindowRgn(HWND hWnd, HRGN hRgn, BOOL bRedraw)
{
    PWND_CONTEXT pcontext = ThemeGetWndContext(hWnd);
    if(pcontext)
    {
        pcontext->HasAppDefinedRgn = TRUE;
        pcontext->HasThemeRgn = FALSE;
    }

    return user32ApiHook.SetWindowRgn(hWnd, hRgn, bRedraw);
}

BOOL WINAPI ThemeGetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
    PWND_CONTEXT pwndContext;
    DWORD style;
    BOOL ret;

    /* Avoid creating a window context if it is not needed */
    if(!IsAppThemed())
        goto dodefault;

    style = GetWindowLongW(hwnd, GWL_STYLE);
    if((style & (WS_HSCROLL|WS_VSCROLL))==0)
        goto dodefault;

    pwndContext = ThemeGetWndContext(hwnd);
    if (pwndContext == NULL)
        goto dodefault;

    /* 
     * Uxtheme needs to handle the tracking of the scrollbar itself 
     * This means than if an application needs to get the track position
     * with GetScrollInfo, it will get wrong data. So uxtheme needs to
     * hook it and set the correct tracking position itself
     */
    ret = user32ApiHook.GetScrollInfo(hwnd, fnBar, lpsi);
    if ( lpsi && 
        (lpsi->fMask & SIF_TRACKPOS) &&
         pwndContext->SCROLL_TrackingWin == hwnd && 
         pwndContext->SCROLL_TrackingBar == fnBar)
    {
        lpsi->nTrackPos = pwndContext->SCROLL_TrackingVal;
    }
    return ret;

dodefault:
    return user32ApiHook.GetScrollInfo(hwnd, fnBar, lpsi);
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
        gbThemeHooksActive = FALSE;
        return TRUE;
    }

    gbThemeHooksActive = TRUE;

    /* Store the original functions from user32 */
    user32ApiHook = *puah;
    
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

    UXTHEME_broadcast_msg (NULL, WM_THEMECHANGED);

    return ret;
}

BOOL WINAPI
ThemeHooksRemove()
{
    BOOL ret;

    ret = UnregisterUserApiHook();

    UXTHEME_broadcast_msg (NULL, WM_THEMECHANGED);

    return ret;
}

INT WINAPI ClassicSystemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
    if (gbThemeHooksActive)
    {
        return user32ApiHook.SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
    }

    return SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
}

INT WINAPI ClassicSystemParametersInfoA(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
    if (gbThemeHooksActive)
    {
        return user32ApiHook.SystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);
    }

    return SystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);
}

INT WINAPI ClassicGetSystemMetrics(int nIndex)
{
    if (gbThemeHooksActive)
    {
        return user32ApiHook.GetSystemMetrics(nIndex);
    }

    return GetSystemMetrics(nIndex);
}

BOOL WINAPI ClassicAdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
    if (gbThemeHooksActive)
    {
        return user32ApiHook.AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
    }

    return AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
}
