/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Implementing IME Software Keyboard
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

static UINT guScanCode[256]; /* Mapping: virtual key --> scan code */
static POINT gptRaiseEdge; /* Border + Edge metrics */
static BOOL g_bWantSoftKBDMetrics = TRUE;

static inline BOOL
Imm32PtInRect(
    _In_ const POINT *ppt,
    _In_ LONG x,
    _In_ LONG y,
    _In_ LONG cx,
    _In_ LONG cy)
{
    return (x <= ppt->x) && (ppt->x < x + cx) && (y <= ppt->y) && (ppt->y < y + cy);
}

static inline INT
Imm32Clamp(
    _In_ INT x,
    _In_ INT xMin,
    _In_ INT xMax)
{
    if (x < xMin)
        return xMin;
    if (x > xMax)
        return xMax;
    return x;
}

static VOID
Imm32GetAllMonitorSize(
    _Out_ LPRECT prcWork)
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
Imm32GetNearestWorkArea(
    _In_opt_ HWND hwnd,
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
    {
        ERR("hwnd: %p\n", hwnd);
        return FALSE;
    }

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hMonitor, &mi);
    *prcWork = mi.rcWork;
    return TRUE;
}

/*****************************************************************************
 * Internal codes
 */

#define IC_MAX 60

#undef DEFINE_IC
#define DEFINE_IC(internal_code, virtual_key_code, internal_code_name, virtual_key_name, is_special) \
    internal_code_name = internal_code,

/* Define internal codes */
typedef enum INTERNAL_CODE
{
#include "internal.h"
} INTERNAL_CODE;

#undef DEFINE_IC
#define DEFINE_IC(internal_code, virtual_key_code, internal_code_name, virtual_key_name, is_special) \
    virtual_key_code,

/* Mapping: Internal Code --> Virtual Key */
const BYTE gIC2VK[IC_MAX] =
{
#include "internal.h"
};

/*****************************************************************************
 * T1 software keyboard (for Traditional Chinese)
 */

#define T1_CLASSNAMEW L"SoftKBDClsT1"

typedef struct T1WINDOW
{
    INT cxDefWidth;           /* Regular key width */
    INT cxWidth47;            /* [BackSpace] width */
    INT cxWidth48;            /* [Tab] width */
    INT cxWidth49;            /* [Caps] width */
    INT cxWidth50;            /* [Enter] width */
    INT cxWidth51or52;        /* [Shift] width */
    INT cxWidth53or54;        /* [Ctrl] width */
    INT cxWidth55or56;        /* [Alt] width */
    INT cxWidth57;            /* [Esc] width */
    INT cxWidth58;            /* [Space] width */
    INT cyDefHeight;          /* Regular key height */
    INT cyHeight50;           /* [Enter] height */
    POINT KeyPos[IC_MAX];     /* Internal Code --> POINT */
    WCHAR chKeyChar[48];      /* Internal Code --> WCHAR */
    HBITMAP hbmKeyboard;      /* The keyboard image */
    DWORD CharSet;            /* LOGFONT.lfCharSet */
    UINT PressedKey;          /* Currently pressed key */
    POINT pt0, pt1;           /* The soft keyboard window position */
    DWORD KeyboardSubType;    /* See IMC_GETSOFTKBDSUBTYPE/IMC_SETSOFTKBDSUBTYPE */
} T1WINDOW, *PT1WINDOW;

#define T1_KEYPOS(iKey) pT1->KeyPos[iKey]

static LOGFONTW g_T1LogFont;

static void
T1_GetTextMetric(_Out_ LPTEXTMETRICW ptm)
{
    WCHAR wch;
    SIZE textSize;
    HFONT hFont;
    HGDIOBJ hFontOld;
    HDC hDC;
#ifndef NDEBUG
    WCHAR szFace[LF_FACESIZE];
#endif

    ZeroMemory(&g_T1LogFont, sizeof(g_T1LogFont));
    g_T1LogFont.lfHeight = -12;
    g_T1LogFont.lfWeight = FW_NORMAL;
    g_T1LogFont.lfCharSet = CHINESEBIG5_CHARSET;
#ifdef NO_HACK /* FIXME: We lack proper Asian fonts! */
    g_T1LogFont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    g_T1LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    g_T1LogFont.lfQuality = PROOF_QUALITY;
    g_T1LogFont.lfPitchAndFamily = FF_MODERN | FIXED_PITCH;
#else
    /* "新細明體" */
    StringCchCopyW(g_T1LogFont.lfFaceName, _countof(g_T1LogFont.lfFaceName),
                   L"\u65B0\u7D30\u660E\x9AD4");
#endif
    hFont = CreateFontIndirectW(&g_T1LogFont);

    hDC = GetDC(NULL);
    hFontOld = SelectObject(hDC, hFont);

#ifndef NDEBUG
    GetTextFaceW(hDC, _countof(szFace), szFace);
    TRACE("szFace: %s\n", debugstr_w(szFace));
#endif

    GetTextMetricsW(hDC, ptm);

    wch = 0x4E11; /* U+4E11: 丑 */
    if (GetTextExtentPoint32W(hDC, &wch, 1, &textSize) && textSize.cx > ptm->tmMaxCharWidth)
        ptm->tmMaxCharWidth = textSize.cx;

    DeleteObject(SelectObject(hDC, hFontOld));
    ReleaseDC(NULL, hDC);
}

static void
T1_InitButtonPos(_Out_ PT1WINDOW pT1)
{
    TEXTMETRICW tm;
    LONG cxLarge, cyLarge;
    LONG xKey1, yKey1, xKey2, yKey2, xKey3, yKey3;
    LONG yKey4, xKey4, xKey5, yKey5, xKey6, xKey7;
    INT iKey;

    T1_GetTextMetric(&tm);

    cxLarge = (3 * tm.tmMaxCharWidth + 18) / 2;
    cyLarge = tm.tmHeight + 8;

    /* key widths and heights */
    pT1->cxDefWidth = (2 * tm.tmMaxCharWidth + 12) / 2;
    pT1->cxWidth47 = (2 * tm.tmMaxCharWidth + 12) / 2 + 1;
    pT1->cxWidth49 = (4 * tm.tmMaxCharWidth + 24) / 2 + 3;
    pT1->cxWidth51or52 = (5 * tm.tmMaxCharWidth + 30) / 2 + 5;
    pT1->cxWidth58 = 4 * (3 * tm.tmMaxCharWidth + 18) / 2 + 15;
    pT1->cxWidth48 = pT1->cxWidth50 = cxLarge + 2;
    pT1->cxWidth53or54 = pT1->cxWidth55or56 = cxLarge + 2;
    pT1->cyHeight50 = 2 * (tm.tmHeight + 8) + 3;
    pT1->cxWidth57 = cxLarge + 1;
    pT1->cyDefHeight = cyLarge;

    /* First row */
    xKey1 = gptRaiseEdge.x + 3;
    yKey1 = gptRaiseEdge.y + 3;
    for (iKey = 0; iKey < IC_Q; ++iKey)
    {
        T1_KEYPOS(iKey).x = xKey1;
        T1_KEYPOS(iKey).y = yKey1;
        xKey1 += pT1->cxDefWidth + 3;
    }
    T1_KEYPOS(IC_BACKSPACE).y = yKey1;
    T1_KEYPOS(IC_BACKSPACE).x = xKey1;

    /* 2nd row */
    xKey2 = 3 + gptRaiseEdge.x + pT1->cxWidth48 + 3;
    yKey2 = 3 + yKey1 + cyLarge;
    T1_KEYPOS(IC_TAB).x = gptRaiseEdge.x + 3;
    T1_KEYPOS(IC_TAB).y = yKey2;
    for (iKey = IC_Q; iKey < IC_A; ++iKey)
    {
        T1_KEYPOS(iKey).x = xKey2;
        T1_KEYPOS(iKey).y = yKey2;
        xKey2 += pT1->cxDefWidth + 3;
    }
    T1_KEYPOS(IC_ENTER).x = xKey2;
    T1_KEYPOS(IC_ENTER).y = yKey2;

    /* 3rd row */
    xKey3 = gptRaiseEdge.x + 3 + pT1->cxWidth49 + 3;
    yKey3 = yKey2 + cyLarge + 3;
    T1_KEYPOS(IC_CAPS).x = gptRaiseEdge.x + 3;
    T1_KEYPOS(IC_CAPS).y = yKey3;
    for (iKey = IC_A; iKey < IC_Z; ++iKey)
    {
        T1_KEYPOS(iKey).x = xKey3;
        T1_KEYPOS(iKey).y = yKey3;
        xKey3 += pT1->cxDefWidth + 3;
    }

    /* 4th row */
    xKey4 = gptRaiseEdge.x + pT1->cxWidth51or52 + 3 + 3;
    yKey4 = yKey3 + cyLarge + 3;
    T1_KEYPOS(IC_L_SHIFT).x = gptRaiseEdge.x + 3;
    T1_KEYPOS(IC_L_SHIFT).y = yKey4;
    for (iKey = IC_Z; iKey < IC_BACKSPACE; ++iKey)
    {
        T1_KEYPOS(iKey).x = xKey4;
        T1_KEYPOS(iKey).y = yKey4;
        xKey4 += pT1->cxDefWidth + 3;
    }
    T1_KEYPOS(IC_R_SHIFT).x = xKey4;
    T1_KEYPOS(IC_R_SHIFT).y = yKey4;

    /* 5th row */
    xKey5 = gptRaiseEdge.x + 3 + pT1->cxWidth53or54 + 3;
    T1_KEYPOS(IC_L_CTRL).x = gptRaiseEdge.x + 3;
    T1_KEYPOS(IC_ESCAPE).x = xKey5;
    T1_KEYPOS(IC_L_ALT).x = xKey5 + pT1->cxWidth57 + 3;

    yKey5 = yKey4 + cyLarge + 3;
    T1_KEYPOS(IC_L_CTRL).y = T1_KEYPOS(IC_ESCAPE).y = T1_KEYPOS(IC_L_ALT).y = yKey5;
    T1_KEYPOS(IC_R_ALT).y = T1_KEYPOS(IC_SPACE).y = T1_KEYPOS(IC_R_CTRL).y = yKey5;

    xKey6 = xKey5 + pT1->cxWidth57 + 3 + pT1->cxWidth55or56 + 3;
    T1_KEYPOS(IC_SPACE).x = xKey6;

    xKey7 = xKey6 + pT1->cxWidth58 + 3;
    T1_KEYPOS(IC_R_ALT).x = xKey7;
    T1_KEYPOS(IC_R_CTRL).x = xKey7 + pT1->cxWidth57 + pT1->cxWidth55or56 + 6;
}

/* Draw keyboard key edge */
static void
T1_DrawConvexRect(
    _In_ HDC hDC,
    _In_ INT x,
    _In_ INT y,
    _In_ INT width,
    _In_ INT height)
{
    HGDIOBJ hBlackPen = GetStockObject(BLACK_PEN);
    HGDIOBJ hLtGrayBrush = GetStockObject(LTGRAY_BRUSH);
    HGDIOBJ hGrayBrush = GetStockObject(GRAY_BRUSH);
    INT dx = width + 4, dy = height + 4;
    INT x0 = x - 2, y0 = y + height + 2;

    /* Face */
    SelectObject(hDC, hBlackPen);
    SelectObject(hDC, hLtGrayBrush);
    Rectangle(hDC, x0, y - 2, x0 + dx, y0);

    /* Rounded corners */
    PatBlt(hDC, x0, y - 2, 1, 1, PATCOPY);
    PatBlt(hDC, x0, y0, 1, -1, PATCOPY);
    PatBlt(hDC, x0 + dx, y - 2, -1, 1, PATCOPY);
    PatBlt(hDC, x0 + dx, y0, -1, -1, PATCOPY);

    /* Light edge */
    PatBlt(hDC, x0 + 1, y + dy - 3, 1, 2 - dy, WHITENESS);
    PatBlt(hDC, x0 + 1, y - 1, dx - 2, 1, WHITENESS);

    /* Dark edge */
    SelectObject(hDC, hGrayBrush);
    PatBlt(hDC, x0 + 1, y + dy - 3, dx - 2, -1, PATCOPY);
    PatBlt(hDC, x0 + dx - 1, y + dy - 3, -1, 2 - dy, PATCOPY);
}

static void
T1_DrawBitmap(
    _In_ HDC hDC,
    _In_ INT x,
    _In_ INT y,
    _In_ INT cx,
    _In_ INT cy,
    _In_ INT nBitmapID)
{
    HBITMAP hBitmap = LoadBitmapW(ghImm32Inst, MAKEINTRESOURCEW(nBitmapID));
    HDC hMemDC = CreateCompatibleDC(hDC);
    HGDIOBJ hbmOld = SelectObject(hMemDC, hBitmap);
    BitBlt(hDC, x, y, cx, cy, hMemDC, 0, 0, SRCCOPY);
    SelectObject(hMemDC, hbmOld);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
}

static void
T1_DrawLabels(
    _In_ HDC hDC,
    _In_ const T1WINDOW *pT1,
    _In_ LPCWSTR pszBmpName)
{
    HBITMAP hBitmap = LoadBitmapW(ghImm32Inst, pszBmpName);
    HDC hdcMem = CreateCompatibleDC(hDC);
    HGDIOBJ hbmOld = SelectObject(hdcMem, hBitmap);
    INT iKey;
    for (iKey = 0; iKey < IC_BACKSPACE; ++iKey)
    {
        const POINT *ppt = &T1_KEYPOS(iKey);
        BitBlt(hDC, ppt->x, ppt->y, 8, 8, hdcMem, iKey * 8, 0, SRCCOPY);
    }
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    DeleteObject(hBitmap);
}

static void
T1_InitBitmap(
    _In_ HWND hWnd,
    _Inout_ PT1WINDOW pT1)
{
    HDC hDC, hMemDC;
    HGDIOBJ hNullPen = GetStockObject(NULL_PEN), hbrLtGray = GetStockObject(LTGRAY_BRUSH);
    RECT rc;
    INT iKey;

    /* Create the bitmap */
    hDC = GetDC(hWnd);
    hMemDC = CreateCompatibleDC(hDC);
    GetClientRect(hWnd, &rc);
    pT1->hbmKeyboard = CreateCompatibleBitmap(hDC, rc.right - rc.left, rc.bottom - rc.top);
    ReleaseDC(hWnd, hDC);

    /* Draw keyboard face */
    SelectObject(hMemDC, pT1->hbmKeyboard);
    SelectObject(hMemDC, hNullPen);
    SelectObject(hMemDC, hbrLtGray);
    Rectangle(hMemDC, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
    DrawEdge(hMemDC, &rc, EDGE_RAISED, BF_RECT);

    /* 53 --> Left [Ctrl] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_L_CTRL).x, T1_KEYPOS(IC_L_CTRL).y,
                      pT1->cxWidth53or54, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth53or54 / 2 + T1_KEYPOS(IC_L_CTRL).x - 8,
                  pT1->cyDefHeight   / 2 + T1_KEYPOS(IC_L_CTRL).y - 4,
                  16, 9, IDB_T1_CTRL);

    /* 54 --> Right [Ctrl] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_R_CTRL).x, T1_KEYPOS(IC_R_CTRL).y,
                      pT1->cxWidth53or54, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth53or54 / 2 + T1_KEYPOS(IC_R_CTRL).x - 8,
                  pT1->cyDefHeight   / 2 + T1_KEYPOS(IC_R_CTRL).y - 4,
                  16, 9, IDB_T1_CTRL);

    /* 57 --> [Esc] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_ESCAPE).x, T1_KEYPOS(IC_ESCAPE).y,
                      pT1->cxWidth57, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth57   / 2 + T1_KEYPOS(IC_ESCAPE).x - 9,
                  pT1->cyDefHeight / 2 + T1_KEYPOS(IC_ESCAPE).y - 4,
                  18, 9, IDB_T1_ESCAPE);

    /* 55 --> Left [Alt] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_L_ALT).x, T1_KEYPOS(IC_L_ALT).y,
                      pT1->cxWidth55or56, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth55or56 / 2 + T1_KEYPOS(IC_L_ALT).x - 8,
                  pT1->cyDefHeight   / 2 + T1_KEYPOS(IC_L_ALT).y - 4,
                  16, 9, IDB_T1_ALT);

    /* 56 --> Right [Alt] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_R_ALT).x, T1_KEYPOS(IC_R_ALT).y,
                      pT1->cxWidth55or56, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth55or56 / 2 + T1_KEYPOS(IC_R_ALT).x - 8,
                  pT1->cyDefHeight   / 2 + T1_KEYPOS(IC_R_ALT).y - 4,
                  16, 9, IDB_T1_ALT);

    /* 58 --> [Space] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_SPACE).x, T1_KEYPOS(IC_SPACE).y,
                      pT1->cxWidth58, pT1->cyDefHeight);

    /* 51 --> Left [Shift] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_L_SHIFT).x, T1_KEYPOS(IC_L_SHIFT).y,
                      pT1->cxWidth51or52, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth51or52 / 2 + T1_KEYPOS(IC_L_SHIFT).x - 11,
                  pT1->cyDefHeight   / 2 + T1_KEYPOS(IC_L_SHIFT).y - 4,
                  23, 9, IDB_T1_SHIFT);

    /* 52 --> Right [Shift] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_R_SHIFT).x, T1_KEYPOS(IC_R_SHIFT).y,
                      pT1->cxWidth51or52, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth51or52 / 2 + T1_KEYPOS(IC_R_SHIFT).x - 11,
                  pT1->cyDefHeight   / 2 + T1_KEYPOS(IC_R_SHIFT).y - 4,
                  23, 9, IDB_T1_SHIFT);

    /* 49 --> [Caps] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_CAPS).x, T1_KEYPOS(IC_CAPS).y,
                      pT1->cxWidth49, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth49   / 2 + T1_KEYPOS(IC_CAPS).x - 11,
                  pT1->cyDefHeight / 2 + T1_KEYPOS(IC_CAPS).y - 4,
                  22, 9, IDB_T1_CAPS);

    /* 48 --> [Tab] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_TAB).x, T1_KEYPOS(IC_TAB).y,
                      pT1->cxWidth48, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth48   / 2 + T1_KEYPOS(IC_TAB).x - 8,
                  pT1->cyDefHeight / 2 + T1_KEYPOS(IC_TAB).y - 4,
                  16, 9, IDB_T1_TAB);

    /* 50 --> [Enter] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_ENTER).x, T1_KEYPOS(IC_ENTER).y,
                      pT1->cxWidth50, pT1->cyHeight50);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth50  / 2 + T1_KEYPOS(IC_ENTER).x - 13,
                  pT1->cyHeight50 / 2 + T1_KEYPOS(IC_ENTER).y - 4,
                  26, 9, IDB_T1_ENTER);

    /* 47 --> [BackSpace] */
    T1_DrawConvexRect(hMemDC,
                      T1_KEYPOS(IC_BACKSPACE).x, T1_KEYPOS(IC_BACKSPACE).y,
                      pT1->cxWidth47, pT1->cyDefHeight);
    T1_DrawBitmap(hMemDC,
                  pT1->cxWidth47   / 2 + T1_KEYPOS(IC_BACKSPACE).x - 8,
                  pT1->cyDefHeight / 2 + T1_KEYPOS(IC_BACKSPACE).y - 4,
                  16, 9, IDB_T1_BACKSPACE);

    /* Regular keys */
    for (iKey = 0; iKey < IC_BACKSPACE; ++iKey)
    {
        LPPOINT ppt = &T1_KEYPOS(iKey);
        T1_DrawConvexRect(hMemDC, ppt->x, ppt->y, pT1->cxDefWidth, pT1->cyDefHeight);
    }

    T1_DrawLabels(hMemDC, pT1, MAKEINTRESOURCEW(IDB_T1_CHARS));
    DeleteDC(hMemDC);
}

static INT
T1_OnCreate(
    _In_ HWND hWnd)
{
    PT1WINDOW pT1;
    HGLOBAL hGlobal = GlobalAlloc(GHND, sizeof(T1WINDOW));
    if (!hGlobal)
        return -1;

    pT1 = (PT1WINDOW)GlobalLock(hGlobal);
    if (!pT1)
    {
        GlobalFree(hGlobal);
        return -1;
    }

    SetWindowLongPtrW(hWnd, 0, (LONG_PTR)hGlobal);
    pT1->pt1.x = pT1->pt1.y = -1;
    pT1->PressedKey = IC_NONE;
    pT1->CharSet = CHINESEBIG5_CHARSET;

    T1_InitButtonPos(pT1);
    T1_InitBitmap(hWnd, pT1);
    GlobalUnlock(hGlobal);

    return 0;
}

static void
T1_DrawDragBorder(
    _In_ HWND hWnd,
    _In_ const POINT *ppt1,
    _In_ const POINT *ppt2)
{
    INT cxBorder = GetSystemMetrics(SM_CXBORDER), cyBorder = GetSystemMetrics(SM_CYBORDER);
    INT x = ppt1->x - ppt2->x, y = ppt1->y - ppt2->y;
    HGDIOBJ hGrayBrush = GetStockObject(GRAY_BRUSH);
    RECT rc;
    HDC hDisplayDC;

    GetWindowRect(hWnd, &rc);
    hDisplayDC = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
    SelectObject(hDisplayDC, hGrayBrush);
    PatBlt(hDisplayDC, x, y, rc.right - rc.left - cxBorder, cyBorder, PATINVERT);
    PatBlt(hDisplayDC, x, cyBorder + y, cxBorder, rc.bottom - rc.top - cyBorder, PATINVERT);
    PatBlt(hDisplayDC, x + cxBorder, y + rc.bottom - rc.top, rc.right - rc.left - cxBorder, -cyBorder, PATINVERT);
    PatBlt(hDisplayDC, x + rc.right - rc.left, y, -cxBorder, rc.bottom - rc.top - cyBorder, PATINVERT);
    DeleteDC(hDisplayDC);
}

static void
T1_OnDestroy(
    _In_ HWND hWnd)
{
    HGLOBAL hGlobal;
    PT1WINDOW pT1;
    HWND hwndOwner;

    hGlobal = (HGLOBAL)GetWindowLongPtrW(hWnd, 0);
    pT1 = (PT1WINDOW)GlobalLock(hGlobal);
    if (!hGlobal || !pT1)
        return;

    if (pT1->pt1.x != -1 && pT1->pt1.y != -1)
        T1_DrawDragBorder(hWnd, &pT1->pt0, &pT1->pt1);

    DeleteObject(pT1->hbmKeyboard);
    GlobalUnlock(hGlobal);
    GlobalFree(hGlobal);

    hwndOwner = GetWindow(hWnd, GW_OWNER);
    if (hwndOwner)
        SendMessageW(hwndOwner, WM_IME_NOTIFY, IMN_SOFTKBDDESTROYED, 0);
}

static void
T1_InvertButton(
    _In_ HWND hWnd,
    _In_ HDC hDC,
    _In_ const T1WINDOW *pT1,
    _In_ UINT iPressed)
{
    INT cxWidth = pT1->cxDefWidth, cyHeight = pT1->cyDefHeight;
    HDC hChoiceDC;

    if (iPressed >= IC_NONE)
        return;

    if (hDC)
        hChoiceDC = hDC;
    else
        hChoiceDC = GetDC(hWnd);

    if (iPressed >= IC_BACKSPACE)
    {
        switch (iPressed)
        {
            case IC_BACKSPACE:
                cxWidth = pT1->cxWidth47;
                break;
            case IC_TAB:
                cxWidth = pT1->cxWidth48;
                break;
            case IC_ENTER:
                pT1 = pT1;
                cxWidth = pT1->cxWidth50;
                cyHeight = pT1->cyHeight50;
                break;
            case IC_ESCAPE:
                cxWidth = pT1->cxWidth57;
                break;
            case IC_SPACE:
                cxWidth = pT1->cxWidth58;
                break;
            default:
                cxWidth = 0;
                MessageBeep(0xFFFFFFFF);
                break;
        }
    }

    if (cxWidth > 0)
    {
        PatBlt(hChoiceDC,
               T1_KEYPOS(iPressed).x - 1, T1_KEYPOS(iPressed).y - 1,
               cxWidth + 2, cyHeight + 2,
               DSTINVERT);
    }

    if (!hDC)
        ReleaseDC(hWnd, hChoiceDC);
}

static void
T1_OnDraw(
    _In_ HDC hDC,
    _In_ HWND hWnd)
{
    HGLOBAL hGlobal;
    PT1WINDOW pT1;
    HDC hMemDC;
    RECT rc;

    hGlobal = (HGLOBAL)GetWindowLongPtrW(hWnd, 0);
    pT1 = (PT1WINDOW)GlobalLock(hGlobal);
    if (!hGlobal || !pT1)
        return;

    hMemDC = CreateCompatibleDC(hDC);
    SelectObject(hMemDC, pT1->hbmKeyboard);
    GetClientRect(hWnd, &rc);
    BitBlt(hDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, hMemDC, 0, 0, SRCCOPY);
    DeleteDC(hMemDC);

    if (pT1->PressedKey < IC_NONE)
        T1_InvertButton(hWnd, hDC, pT1, pT1->PressedKey);

    GlobalUnlock(hGlobal);
}

static UINT
T1_HitTest(
    _In_ const T1WINDOW *pT1,
    _In_ const POINT *ppt)
{
    INT iKey;
    for (iKey = 0; iKey < IC_BACKSPACE; ++iKey)
    {
        const POINT *pptKey = &T1_KEYPOS(iKey);
        if (Imm32PtInRect(ppt, pptKey->x, pptKey->y, pT1->cxDefWidth, pT1->cyDefHeight))
            return iKey;
    }

    if (Imm32PtInRect(ppt, T1_KEYPOS(IC_BACKSPACE).x, T1_KEYPOS(IC_BACKSPACE).y, pT1->cxWidth47, pT1->cyDefHeight))
        return IC_BACKSPACE;

    if (Imm32PtInRect(ppt, T1_KEYPOS(IC_TAB).x, T1_KEYPOS(IC_TAB).y, pT1->cxWidth48, pT1->cyDefHeight))
        return IC_TAB;

    if (Imm32PtInRect(ppt, T1_KEYPOS(IC_CAPS).x, T1_KEYPOS(IC_CAPS).y, pT1->cxWidth49, pT1->cyDefHeight))
        return IC_CAPS;

    if (Imm32PtInRect(ppt, T1_KEYPOS(IC_ENTER).x, T1_KEYPOS(IC_ENTER).y, pT1->cxWidth50, pT1->cyHeight50))
        return IC_ENTER;

    if (Imm32PtInRect(ppt, T1_KEYPOS(IC_L_SHIFT).x, T1_KEYPOS(IC_L_SHIFT).y, pT1->cxWidth51or52, pT1->cyDefHeight) ||
        Imm32PtInRect(ppt, T1_KEYPOS(IC_R_SHIFT).x, T1_KEYPOS(IC_R_SHIFT).y, pT1->cxWidth51or52, pT1->cyDefHeight))
    {
        return IC_L_SHIFT;
    }

    if (Imm32PtInRect(ppt, T1_KEYPOS(IC_L_CTRL).x, T1_KEYPOS(IC_L_CTRL).y, pT1->cxWidth53or54, pT1->cyDefHeight) ||
        Imm32PtInRect(ppt, T1_KEYPOS(IC_R_CTRL).x, T1_KEYPOS(IC_R_CTRL).y, pT1->cxWidth53or54, pT1->cyDefHeight))
    {
        return IC_L_CTRL;
    }

    if (Imm32PtInRect(ppt, T1_KEYPOS(IC_L_ALT).x, T1_KEYPOS(IC_L_ALT).y, pT1->cxWidth55or56, pT1->cyDefHeight) ||
        Imm32PtInRect(ppt, T1_KEYPOS(IC_R_ALT).x, T1_KEYPOS(IC_R_ALT).y, pT1->cxWidth55or56, pT1->cyDefHeight))
    {
        return IC_L_ALT;
    }

    if (Imm32PtInRect(ppt, T1_KEYPOS(IC_ESCAPE).x, T1_KEYPOS(IC_ESCAPE).y, pT1->cxWidth57, pT1->cyDefHeight))
        return IC_ESCAPE;

    if (Imm32PtInRect(ppt, T1_KEYPOS(IC_SPACE).x, T1_KEYPOS(IC_SPACE).y, pT1->cxWidth58, pT1->cyDefHeight))
        return IC_SPACE;

    return IC_NONE;
}

static BOOL
T1_IsValidButton(
    _In_ UINT iKey,
    _In_ const T1WINDOW *pT1)
{
    if (iKey < IC_BACKSPACE)
        return !!pT1->chKeyChar[iKey];
    return iKey <= IC_TAB || iKey == IC_ENTER || (IC_ESCAPE <= iKey && iKey <= IC_SPACE);
}

/**
 * NOTE: The window that has WS_DISABLED style doesn't receive some mouse messages.
 * Use WM_SETCURSOR handling to detect mouse events.
 */
static BOOL
T1_OnSetCursor(
    _In_ HWND hWnd,
    _In_ LPARAM lParam)
{
    HGLOBAL hGlobal;
    PT1WINDOW pT1;
    HCURSOR hCursor;
    UINT iPressed, iKey;
    RECT rc, rcWork;

    hGlobal = (HGLOBAL)GetWindowLongPtrW(hWnd, 0);
    pT1 = (PT1WINDOW)GlobalLock(hGlobal);
    if (!hGlobal || !pT1)
        return FALSE;

    if (pT1->pt1.x != -1 && pT1->pt1.y != -1)
    {
        SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_SIZEALL));
        GlobalUnlock(hGlobal);
        return TRUE;
    }

    GetCursorPos(&pT1->pt0);
    ScreenToClient(hWnd, &pT1->pt0);

    iKey = T1_HitTest(pT1, &pT1->pt0);
    if (iKey >= IC_NONE)
        hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_SIZEALL);
    else
        hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
    SetCursor(hCursor);

    if (HIWORD(lParam) == WM_LBUTTONDOWN)
    {
        SetCapture(hWnd);

        iPressed = pT1->PressedKey;
        if (iPressed < IC_NONE)
        {
            UINT iVK = gIC2VK[iPressed];
            keybd_event(iVK, guScanCode[iVK], KEYEVENTF_KEYUP, 0);
            T1_InvertButton(hWnd, NULL, pT1, pT1->PressedKey);
            pT1->PressedKey = IC_NONE;
        }

        if (iKey >= IC_NONE)
        {
            Imm32GetAllMonitorSize(&rcWork);
            GetCursorPos(&pT1->pt0);
            GetWindowRect(hWnd, &rc);
            pT1->pt1.x = pT1->pt0.x - rc.left;
            pT1->pt1.y = pT1->pt0.y - rc.top;
            T1_DrawDragBorder(hWnd, &pT1->pt0, &pT1->pt1);
        }
        else if (T1_IsValidButton(iKey, pT1))
        {
            UINT iVK = gIC2VK[iKey];
            keybd_event(iVK, guScanCode[iVK], 0, 0);
            pT1->PressedKey = iKey;
            T1_InvertButton(hWnd, 0, pT1, iKey);
        }
        else
        {
            MessageBeep(0xFFFFFFFF);
        }
    }

    return TRUE;
}

static BOOL
T1_OnMouseMove(
    _In_ HWND hWnd)
{
    BOOL ret = FALSE;
    HGLOBAL hGlobal;
    PT1WINDOW pT1;

    hGlobal = (HGLOBAL)GetWindowLongPtrW(hWnd, 0);
    pT1 = (PT1WINDOW)GlobalLock(hGlobal);
    if (!hGlobal || !pT1)
        return FALSE;

    if (pT1->pt1.x != -1 && pT1->pt1.y != -1)
    {
        T1_DrawDragBorder(hWnd, &pT1->pt0, &pT1->pt1);
        GetCursorPos(&pT1->pt0);
        T1_DrawDragBorder(hWnd, &pT1->pt0, &pT1->pt1);
        ret = TRUE;
    }

    GlobalUnlock(hGlobal);
    return ret;
}

static BOOL
T1_OnButtonUp(
    _In_ HWND hWnd)
{
    BOOL ret = FALSE;
    HGLOBAL hGlobal;
    PT1WINDOW pT1;
    INT x, y, iPressed;
    HWND hwndOwner, hwndCapture = GetCapture();
    HIMC hIMC;
    LPINPUTCONTEXT pIC;

    if (hwndCapture == hWnd)
        ReleaseCapture();

    hGlobal = (HGLOBAL)GetWindowLongPtrW(hWnd, 0);
    pT1 = (PT1WINDOW)GlobalLock(hGlobal);
    if (!hGlobal || !pT1)
        return FALSE;

    iPressed = pT1->PressedKey;
    if (iPressed >= IC_NONE)
    {
        if (pT1->pt1.x != -1 && pT1->pt1.y != -1 )
        {
            T1_DrawDragBorder(hWnd, &pT1->pt0, &pT1->pt1);
            x = pT1->pt0.x - pT1->pt1.x;
            y = pT1->pt0.y - pT1->pt1.y;
            SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
            pT1->pt1.x = pT1->pt1.y = -1;
            pT1->PressedKey = IC_NONE;
            ret = TRUE;

            hwndOwner = GetWindow(hWnd, GW_OWNER);
            hIMC = (HIMC)GetWindowLongPtrW(hwndOwner, 0);
            if (hIMC)
            {
                pIC = ImmLockIMC(hIMC);
                if (pIC)
                {
                    pIC->fdwInit |= INIT_SOFTKBDPOS;
                    pIC->ptSoftKbdPos.x = x;
                    pIC->ptSoftKbdPos.y = y;
                    ImmUnlockIMC(hIMC);
                }
            }
        }
    }
    else
    {
        UINT iVK = gIC2VK[iPressed];
        keybd_event(iVK, guScanCode[iVK], KEYEVENTF_KEYUP, 0);

        T1_InvertButton(hWnd, 0, pT1, pT1->PressedKey);
        pT1->PressedKey = IC_NONE;
        ret = TRUE;
    }

    GlobalUnlock(hGlobal);
    return ret;
}

static LRESULT
T1_SetData(
    _In_ HWND hWnd,
    _In_ const SOFTKBDDATA *pData)
{
    HGLOBAL hGlobal;
    PT1WINDOW pT1;
    HDC hDC, hMemDC;
    HFONT hFont;
    HGDIOBJ hFontOld;
    RECT rc;
    INT iKey;

    hGlobal = (HGLOBAL)GetWindowLongPtrW(hWnd, 0);
    pT1 = (PT1WINDOW)GlobalLock(hGlobal);
    if (!hGlobal || !pT1)
        return 1;

    hDC = GetDC(hWnd);
    hMemDC = CreateCompatibleDC(hDC);
    ReleaseDC(hWnd, hDC);

    SelectObject(hMemDC, pT1->hbmKeyboard);
    SetTextColor(hMemDC, RGB(0, 0, 0));
    SetBkColor(hMemDC, RGB(192, 192, 192));

    if (pT1->CharSet == DEFAULT_CHARSET)
    {
        hFont = CreateFontIndirectW(&g_T1LogFont);
    }
    else
    {
        LOGFONTW lf = g_T1LogFont;
        lf.lfCharSet = (BYTE)pT1->CharSet;
        hFont = CreateFontIndirectW(&lf);
    }
    hFontOld = SelectObject(hMemDC, hFont);

    for (iKey = 0; iKey < IC_BACKSPACE; ++iKey)
    {
        INT x0 = T1_KEYPOS(iKey).x, y0 = T1_KEYPOS(iKey).y;
        INT x = x0 + 6, y = y0 + 8;
        WCHAR wch = pT1->chKeyChar[iKey] = pData->wCode[0][gIC2VK[iKey]];
        SetRect(&rc, x, y, x0 + pT1->cxDefWidth, y0 + pT1->cyDefHeight);
        ExtTextOutW(hDC, x, y, ETO_OPAQUE, &rc, &wch, wch != 0, NULL);
    }

    DeleteObject(SelectObject(hMemDC, hFontOld));
    DeleteDC(hMemDC);
    GlobalUnlock(hGlobal);
    return 0;
}

static LRESULT
T1_OnImeControl(
    _In_ HWND hWnd,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam)
{
    LRESULT ret = 1;
    PT1WINDOW pT1;
    HGLOBAL hGlobal;

    switch (wParam)
    {
        case IMC_GETSOFTKBDFONT:
        {
            hGlobal = (HGLOBAL)GetWindowLongPtrW(hWnd, 0);
            pT1 = (PT1WINDOW)GlobalLock(hGlobal);
            if (hGlobal && pT1)
            {
                LPLOGFONTW plf = (LPLOGFONTW)lParam;
                DWORD CharSet = pT1->CharSet;
                GlobalUnlock(hGlobal);

                *plf = g_T1LogFont;
                if (CharSet != DEFAULT_CHARSET)
                    plf->lfCharSet = (BYTE)CharSet;

                ret = 0;
            }
            break;
        }
        case IMC_SETSOFTKBDFONT:
        {
            const LOGFONTW *plf = (LPLOGFONTW)lParam;
            if (g_T1LogFont.lfCharSet == plf->lfCharSet)
                return 0;

            hGlobal = (HGLOBAL)GetWindowLongPtrW(hWnd, 0);
            pT1 = (PT1WINDOW)GlobalLock(hGlobal);
            if (pT1)
            {
                pT1->CharSet = plf->lfCharSet;
                GlobalUnlock(hGlobal);
                return 0;
            }

            break;
        }
        case IMC_GETSOFTKBDPOS:
        {
            RECT rc;
            GetWindowRect(hWnd, &rc);
            return MAKELRESULT(rc.left, rc.top);
        }
        case IMC_SETSOFTKBDPOS:
        {
            POINT pt;
            HWND hwndParent;

            POINTSTOPOINT(pt, lParam);
            SetWindowPos(hWnd, NULL, pt.x, pt.y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);

            hwndParent = GetParent(hWnd);
            if (hwndParent)
            {
                HIMC hIMC = (HIMC)GetWindowLongPtrW(hwndParent, 0);
                if (hIMC)
                {
                    LPINPUTCONTEXT pIC = ImmLockIMC(hIMC);
                    if (pIC)
                    {
                        pIC->ptSoftKbdPos.x = pt.x;
                        pIC->ptSoftKbdPos.y = pt.y;
                        ImmUnlockIMC(hIMC);
                        return 0;
                    }
                }
            }
            break;
        }
        case IMC_GETSOFTKBDSUBTYPE:
        case IMC_SETSOFTKBDSUBTYPE:
        {
            hGlobal = (HGLOBAL)GetWindowLongPtrW(hWnd, 0);
            pT1 = (PT1WINDOW)GlobalLock(hGlobal);
            if (!hGlobal || !pT1)
                return -1;

            ret = pT1->KeyboardSubType;

            if (wParam == IMC_SETSOFTKBDSUBTYPE)
                pT1->KeyboardSubType = (DWORD)lParam;

            GlobalUnlock(hGlobal);
            break;
        }
        case IMC_SETSOFTKBDDATA:
        {
            ret = T1_SetData(hWnd, (SOFTKBDDATA*)lParam);
            if (!ret)
            {
                InvalidateRect(hWnd, NULL, FALSE);
                PostMessageW(hWnd, WM_PAINT, 0, 0);
            }
            break;
        }
    }

    return ret;
}

static LRESULT CALLBACK
T1_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            return T1_OnCreate(hWnd);
        }
        case WM_DESTROY:
        {
            T1_OnDestroy(hWnd);
            break;
        }
        case WM_SETCURSOR:
        {
            if (T1_OnSetCursor(hWnd, lParam))
                break;
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
        case WM_MOUSEMOVE:
        {
            if (T1_OnMouseMove(hWnd))
                break;
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);
            T1_OnDraw(hDC, hWnd);
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_SHOWWINDOW:
        {
            if (!lParam && wParam != SW_SHOWNORMAL)
                T1_OnButtonUp(hWnd);
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
        case WM_MOUSEACTIVATE:
        {
            return MA_NOACTIVATE;
        }
        case WM_LBUTTONUP:
        {
            if (T1_OnButtonUp(hWnd))
                break;
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
        case WM_IME_CONTROL:
        {
            return T1_OnImeControl(hWnd, wParam, lParam);
        }
        default:
        {
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

/*****************************************************************************
 * C1 software keyboard (for Simplified Chinese)
 */

#define C1_CLASSNAMEW L"SoftKBDClsC1"

static LRESULT CALLBACK
C1_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

/*****************************************************************************/

static BOOL
Imm32RegisterSoftKeyboard(
    _In_ UINT uType)
{
    WNDCLASSEXW wcx;
    LPCWSTR pszClass = ((uType == 1) ? T1_CLASSNAMEW : C1_CLASSNAMEW);
    if (GetClassInfoExW(ghImm32Inst, pszClass, &wcx))
        return TRUE;

    ZeroMemory(&wcx, sizeof(wcx));
    wcx.cbSize        = sizeof(wcx);
    wcx.style         = CS_IME;
    wcx.cbWndExtra    = sizeof(PT1WINDOW);
    wcx.hIcon         = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wcx.hInstance     = ghImm32Inst;
    wcx.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_SIZEALL);
    wcx.lpszClassName = pszClass;

    if (uType == 1)
    {
        wcx.lpfnWndProc = T1_WindowProc;
        wcx.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    }
    else
    {
        wcx.lpfnWndProc = C1_WindowProc;
        wcx.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    }

    return !!RegisterClassExW(&wcx);
}

static void
Imm32GetSoftKeyboardDimension(
    _In_ UINT uType,
    _Out_ LPINT pcx,
    _Out_ LPINT pcy)
{
    if (uType == 1)
    {
        TEXTMETRICW tm;
        T1_GetTextMetric(&tm);
        *pcx = 15 * tm.tmMaxCharWidth + 2 * gptRaiseEdge.x + 139;
        *pcy = 5 * tm.tmHeight + 2 * gptRaiseEdge.y + 58;
    }
    else
    {
        INT cxEdge = GetSystemMetrics(SM_CXEDGE), cyEdge = GetSystemMetrics(SM_CXEDGE);
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
    UINT iVK;
    INT xSoftKBD, ySoftKBD, cxSoftKBD, cySoftKBD, cxEdge, cyEdge;
    HWND hwndSoftKBD;
    DWORD Style, ExStyle, UICaps;
    LPCWSTR pszClass;
    RECT rcWorkArea;

    if (uType != 1 && uType != 2)
    {
        ERR("uType: %u\n", uType);
        return NULL; /* Invalid keyboard type */
    }

    /* Check IME */
    hKL = GetKeyboardLayout(0);
    pImeDpi = ImmLockImeDpi(hKL);
    if (IS_NULL_UNEXPECTEDLY(pImeDpi))
        return NULL; /* No IME */

    UICaps = pImeDpi->ImeInfo.fdwUICaps;
    ImmUnlockImeDpi(pImeDpi);

    /* Check IME capability */
    if (!(UICaps & UI_CAP_SOFTKBD))
    {
        ERR("UICaps: 0x%X\n", UICaps);
        return NULL; /* No capability for soft keyboard */
    }

    /* Want metrics? */
    if (g_bWantSoftKBDMetrics)
    {
        for (iVK = 0; iVK < 0xFF; ++iVK)
        {
            guScanCode[iVK] = MapVirtualKeyW(iVK, 0);
        }

        cxEdge = GetSystemMetrics(SM_CXEDGE);
        cyEdge = GetSystemMetrics(SM_CYEDGE);
        gptRaiseEdge.x = GetSystemMetrics(SM_CXBORDER) + cxEdge;
        gptRaiseEdge.y = GetSystemMetrics(SM_CYBORDER) + cyEdge;

        g_bWantSoftKBDMetrics = FALSE;
    }

    if (!Imm32GetNearestWorkArea(hwndParent, &rcWorkArea))
        return NULL;

    /* Register the window class */
    if (!Imm32RegisterSoftKeyboard(uType))
    {
        ERR("\n");
        return NULL;
    }

    /* Calculate keyboard size */
    Imm32GetSoftKeyboardDimension(uType, &cxSoftKBD, &cySoftKBD);

    /* Adjust keyboard position */
    xSoftKBD = Imm32Clamp(x, rcWorkArea.left, rcWorkArea.right  - cxSoftKBD);
    ySoftKBD = Imm32Clamp(y, rcWorkArea.top , rcWorkArea.bottom - cySoftKBD);

    /* Create soft keyboard window */
    if (uType == 1)
    {
        Style = (WS_POPUP | WS_DISABLED);
        ExStyle = 0;
        pszClass = T1_CLASSNAMEW;
    }
    else
    {
        Style = (WS_POPUP | WS_DISABLED | WS_BORDER);
        ExStyle = (WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME);
        pszClass = C1_CLASSNAMEW;
    }
    hwndSoftKBD = CreateWindowExW(ExStyle, pszClass, NULL, Style,
                                  xSoftKBD, ySoftKBD, cxSoftKBD, cySoftKBD,
                                  hwndParent, NULL, ghImm32Inst, NULL);
    /* Initial is hidden */
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

    if (nCmdShow != SW_HIDE && nCmdShow != SW_SHOWNOACTIVATE)
        WARN("nCmdShow %d is unexpected\n", nCmdShow);

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
