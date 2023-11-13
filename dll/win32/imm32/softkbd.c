/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IMM Software Keyboard
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

static UINT s_uScanCode[256];
static RECT s_rcWorkArea;
static POINT s_ptRaiseEdge;
static LOGFONTW s_lfSKT1Font;
static BOOL s_bWannaInitSoftKBD = TRUE;

static VOID
Imm32GetAllMonitorSize(_Out_ LPRECT prcWork)
{
    if (GetSystemMetrics(SM_CMONITORS) == 1)
    {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, prcWork, 0);
        return;
    }

    prcWork->left   = GetSystemMetrics(SM_XVIRTUALSCREEN);
    prcWork->top    = GetSystemMetrics(SM_YVIRTUALSCREEN);
    prcWork->right  = prcWork->left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    prcWork->bottom = prcWork->top  + GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

static BOOL
Imm32GetNearestMonitorSize(
    _In_ HWND hwnd,
    _Out_ LPRECT prcWork)
{
    HMONITOR hMonitor;
    MONITORINFO mi;

    if (GetSystemMetrics(SM_CMONITORS) == 1)
    {
        Imm32GetAllMonitorSize(prcWork);
        return TRUE;
    }

    hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!hMonitor)
        return FALSE;

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hMonitor, &mi);

    *prcWork = mi.rcWork;
    return TRUE;
}

/* Software keyboard window procedure (Traditional Chinese) */
static LRESULT CALLBACK
SKWndProcT1(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            FIXME("stub\n");
            return -1;
        }

        default:
        {
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

/* Software keyboard window procedure (Simplified Chinese) */
static LRESULT CALLBACK
SKWndProcC1(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            FIXME("stub\n");
            return -1;
        }

        default:
        {
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

static BOOL
Imm32RegisterSoftKeyboard(_In_ UINT uType)
{
    LPCWSTR pszClass;
    WNDCLASSEXW wcx;

    if (uType == 1)
        pszClass = L"SoftKBDClsT1";
    else if (uType == 2)
        pszClass = L"SoftKBDClsC1";
    else
        return FALSE;

    if (GetClassInfoExW(ghImm32Inst, pszClass, &wcx))
        return TRUE;

    ZeroMemory(&wcx, sizeof(wcx));
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_IME;
    wcx.cbWndExtra = sizeof(LONG_PTR);
    wcx.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wcx.hInstance = ghImm32Inst;
    wcx.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_SIZEALL);
    wcx.lpszClassName = pszClass;

    if (uType == 1)
    {
        wcx.lpfnWndProc = SKWndProcT1;
        wcx.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    }
    else
    {
        wcx.lpfnWndProc = SKWndProcC1;
        wcx.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    }

    return !!RegisterClassExW(&wcx);
}

static VOID
Imm32GetSKT1TextMetric(_Out_ LPTEXTMETRICW lptm)
{
    HDC hDC;
    HFONT hFont;
    SIZE size;
    HGDIOBJ hFontOld;
    WCHAR szText[2] = { 0x894E, 0 };

    hDC = GetDC(NULL);

    ZeroMemory(&s_lfSKT1Font, sizeof(s_lfSKT1Font));
    s_lfSKT1Font.lfHeight = -12;
    s_lfSKT1Font.lfWeight = FW_NORMAL;
    s_lfSKT1Font.lfCharSet = CHINESEBIG5_CHARSET;
    s_lfSKT1Font.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    s_lfSKT1Font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    s_lfSKT1Font.lfQuality = PROOF_QUALITY;
    s_lfSKT1Font.lfPitchAndFamily = 49;
    hFont = CreateFontIndirectW(&s_lfSKT1Font);

    hFontOld = SelectObject(hDC, hFont);

    GetTextMetricsW(hDC, lptm);

    if (GetTextExtentPoint32W(hDC, szText, 1, &size) && lptm->tmMaxCharWidth < size.cx )
        lptm->tmMaxCharWidth = size.cx;

    DeleteObject(SelectObject(hDC, hFontOld));
    ReleaseDC(NULL, hDC);
}

static VOID
Imm32GetSoftKeyboardDimension(
    _In_ UINT uType,
    _Out_ LPINT pcx,
    _Out_ LPINT pcy)
{
    INT cxEdge, cyEdge;
    TEXTMETRICW tm;

    if (uType == 1)
    {
        Imm32GetSKT1TextMetric(&tm);
        *pcx = 15 * tm.tmMaxCharWidth + 2 * s_ptRaiseEdge.x + 139;
        *pcy = 5 * tm.tmHeight + 2 * s_ptRaiseEdge.y + 58;
    }
    else
    {
        cxEdge = GetSystemMetrics(SM_CXEDGE);
        cyEdge = GetSystemMetrics(SM_CYEDGE);
        *pcx = 2 * (GetSystemMetrics(SM_CXBORDER) + cxEdge) + 348;
        *pcy = 2 * (GetSystemMetrics(SM_CYBORDER) + cyEdge) + 136;
    }
}

/***********************************************************************
 *		ImmCreateSoftKeyboard (IMM32.@)
 *
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImmCreateSoftKeyboard.html
 */
HWND WINAPI
ImmCreateSoftKeyboard(
    _In_ UINT uType,
    _In_ HWND hwndParent,
    _In_ INT x,
    _In_ INT y)
{
    HKL hKL;
    PIMEDPI pImeDpi;
    DWORD dwUICaps, style = (WS_POPUP | WS_DISABLED);
    UINT i;
    INT xSoftKBD, ySoftKBD, cxSoftKBD, cySoftKBD;
    HWND hwndSoftKBD;

    TRACE("(%u, %p, %d, %d)\n", uType, hwndParent, x, y);

    if (uType != 1 && uType != 2)
        return 0;

    hKL = GetKeyboardLayout(0);
    pImeDpi = ImmLockImeDpi(hKL);
    if (!pImeDpi)
        return NULL;

    dwUICaps = pImeDpi->ImeInfo.fdwUICaps;
    ImmUnlockImeDpi(pImeDpi);

    if (!(dwUICaps & UI_CAP_SOFTKBD))
        return NULL;

    if (s_bWannaInitSoftKBD)
    {
        if (!Imm32GetNearestMonitorSize(hwndParent, &s_rcWorkArea))
            return NULL;

        for (i = 0; i < 0xFF; ++i)
            s_uScanCode[i] = MapVirtualKeyW(i, 0);

        s_ptRaiseEdge.x = GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXEDGE);
        s_ptRaiseEdge.y = GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYEDGE);

        s_bWannaInitSoftKBD = FALSE;
    }

    if (!Imm32RegisterSoftKeyboard(uType))
        return NULL;

    Imm32GetSoftKeyboardDimension(uType, &cxSoftKBD, &cySoftKBD);

    xSoftKBD = max(s_rcWorkArea.left, min(x, s_rcWorkArea.right  - cxSoftKBD));
    ySoftKBD = max(s_rcWorkArea.top,  min(y, s_rcWorkArea.bottom - cySoftKBD));

    if (uType == 1) /* Traditional Chinese */
    {
        hwndSoftKBD = CreateWindowExW(0,
                                      L"SoftKBDClsT1", NULL, style,
                                      xSoftKBD, ySoftKBD, cxSoftKBD, cySoftKBD,
                                      hwndParent, NULL, ghImm32Inst, NULL);
    }
    else /* Simplified Chinese (uType == 2) */
    {
        style |= WS_BORDER;
        hwndSoftKBD = CreateWindowExW(WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME,
                                      L"SoftKBDClsC1", NULL, style,
                                      xSoftKBD, ySoftKBD, cxSoftKBD, cySoftKBD,
                                      hwndParent, NULL, ghImm32Inst, NULL);
    }

    ShowWindow(hwndSoftKBD, SW_HIDE);
    UpdateWindow(hwndSoftKBD);

    return hwndSoftKBD;
}

/***********************************************************************
 *		ImmShowSoftKeyboard (IMM32.@)
 *
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImmShowSoftKeyboard.html
 */
BOOL WINAPI
ImmShowSoftKeyboard(
    _In_ HWND hwndSoftKBD,
    _In_ INT nCmdShow)
{
    TRACE("(%p, %d)\n", hwndSoftKBD, nCmdShow);
    return hwndSoftKBD && ShowWindow(hwndSoftKBD, nCmdShow);
}

/***********************************************************************
 *		ImmDestroySoftKeyboard (IMM32.@)
 *
 * @see https://katahiromz.web.fc2.com/colony3rd/imehackerz/en/ImmDestroySoftKeyboard.html
 */
BOOL WINAPI
ImmDestroySoftKeyboard(
    _In_ HWND hwndSoftKBD)
{
    TRACE("(%p)\n", hwndSoftKBD);
    return DestroyWindow(hwndSoftKBD);
}
