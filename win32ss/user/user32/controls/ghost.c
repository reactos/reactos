/*
 * PROJECT:     ReactOS user32.dll
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Ghost window class
 * COPYRIGHT:   Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <user32.h>
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(ghost);

#define GHOST_TIMER_ID  0xFACEDEAD
#define GHOST_INTERVAL  1000        // one second

extern HINSTANCE User32Instance;

const struct builtin_class_descr GHOST_builtin_class =
{
    L"Ghost",                   /* name */
    0,                          /* style  */
    GhostWndProcA,              /* procA */
    GhostWndProcW,              /* procW */
    0,                          /* extra */
    IDC_WAIT,                   /* cursor */
    NULL                        /* brush */
};

static HBITMAP
IntCreate32BppBitmap(INT cx, INT cy)
{
    HBITMAP hbm = NULL;
    BITMAPINFO bi;
    HDC hdc;
    LPVOID pvBits;

    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;

    hdc = CreateCompatibleDC(NULL);
    if (hdc)
    {
        hbm = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
        DeleteDC(hdc);
    }
    return hbm;
}

static HBITMAP
IntGetWindowBitmap(HWND hwnd, INT cx, INT cy)
{
    HBITMAP hbm = NULL;
    HDC hdc, hdcMem;
    HGDIOBJ hbmOld;

    hdc = GetWindowDC(hwnd);
    if (hdc)
    {
        hdcMem = CreateCompatibleDC(hdc);
        if (hdcMem)
        {
            hbm = IntCreate32BppBitmap(cx, cy);
            if (hbm)
            {
                hbmOld = SelectObject(hdcMem, hbm);
                {
                    BitBlt(hdcMem, 0, 0, cx, cy, hdc, 0, 0, SRCCOPY | CAPTUREBLT);
                }
                SelectObject(hdcMem, hbmOld);
            }
            DeleteDC(hdcMem);
        }
        ReleaseDC(hwnd, hdc);
    }
    return hbm;
}

static VOID
IntMakeGhostImage(HBITMAP hbm)
{
    BITMAP bm;
    DWORD i, *pdw;

    GetObject(hbm, sizeof(bm), &bm);

    if (bm.bmBitsPixel != 32 || !bm.bmBits)
    {
        ERR("bm.bmBitsPixel == %d, bm.bmBits == %p\n",
            bm.bmBitsPixel, bm.bmBits);
        return;
    }

    pdw = bm.bmBits;
    for (i = 0; i < bm.bmWidth * bm.bmHeight; ++i)
    {
        *pdw = *pdw | 0x00C0C0C0;   // bitwise-OR with ARGB #C0C0C0
        ++pdw;
    }
}

/****************************************************************************/

typedef struct GHOST_DATA
{
    HWND hwndTarget;
    HBITMAP hbm32bpp;
} GHOST_DATA;

static GHOST_DATA *
Ghost_GetData(HWND hwnd)
{
    return (GHOST_DATA *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
}

static HWND
Ghost_GetTarget(HWND hwnd)
{
    GHOST_DATA *pData = Ghost_GetData(hwnd);
    if (!pData)
        return NULL;
    return pData->hwndTarget;
}

static LPWSTR
Ghost_GetText(HWND hwndTarget, INT *pcchTextW, INT cchExtra)
{
    LPWSTR pszTextW = NULL, pszTextNewW;
    INT cchNonExtra, cchTextW = *pcchTextW;

    pszTextNewW = HeapAlloc(GetProcessHeap(), 0, cchTextW * sizeof(WCHAR));
    for (;;)
    {
        if (!pszTextNewW)
        {
            if (pszTextW)
                HeapFree(GetProcessHeap(), 0, pszTextW);
            return NULL;
        }
        pszTextW = pszTextNewW;

        cchNonExtra = cchTextW - cchExtra;
        if (InternalGetWindowText(hwndTarget, pszTextW, cchNonExtra) < cchNonExtra - 1)
        {
            break;
        }

        cchTextW *= 2;
        pszTextNewW = HeapReAlloc(GetProcessHeap(), 0, pszTextW, cchTextW * sizeof(WCHAR));
    }

    *pcchTextW = cchTextW;
    return pszTextW;
}

static BOOL
Ghost_OnCreate(HWND hwnd, CREATESTRUCTW *lpcs)
{
    HBITMAP hbm32bpp;
    HWND hwndTarget, hwndPrev;
    GHOST_DATA *pData;
    RECT rc;
    DWORD style, exstyle;
    WCHAR szNotRespondingW[64];
    LPWSTR pszTextW;
    INT cchTextW;
    PWND pWnd = ValidateHwnd(hwnd);
    if (pWnd)
    {
        if (!pWnd->fnid)
        {
            NtUserSetWindowFNID(hwnd, FNID_GHOST);
        }
        else if (pWnd->fnid != FNID_GHOST)
        {
             ERR("Wrong window class for Ghost! fnId 0x%x\n", pWnd->fnid);
             return 0;
        }
    }

    // get the target
    hwndTarget = (HWND)lpcs->lpCreateParams;
    if (!hwndTarget || !IsWindowVisible(hwndTarget) || IsIconic(hwndTarget) ||
        GetParent(hwndTarget))
    {
        return FALSE;
    }

    // create user data
    pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GHOST_DATA));
    if (!pData)
        return FALSE;

    // get window image
    GetWindowRect(hwndTarget, &rc);
    hbm32bpp = IntGetWindowBitmap(hwndTarget, rc.right - rc.left, rc.bottom - rc.top);
    if (!hbm32bpp)
    {
        HeapFree(GetProcessHeap(), 0, pData);
        return FALSE;
    }
    // make a ghost image
    IntMakeGhostImage(hbm32bpp);

    // set user data and prop
    pData->hwndTarget = hwndTarget;
    pData->hbm32bpp = hbm32bpp;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pData);
    SetPropW(hwndTarget, L"GhostProp", hwnd);

    // get style
    style = GetWindowLongPtrW(hwndTarget, GWL_STYLE);
    exstyle = GetWindowLongPtrW(hwndTarget, GWL_EXSTYLE);

    // get text
    cchTextW = 512 + ARRAYSIZE(szNotRespondingW) + 1;
    pszTextW = Ghost_GetText(hwndTarget, &cchTextW, ARRAYSIZE(szNotRespondingW));
    if (!pszTextW)
    {
        DeleteObject(hbm32bpp);
        HeapFree(GetProcessHeap(), 0, pData);
        return FALSE;
    }

    // don't use scrollbars.
    style &= ~(WS_HSCROLL | WS_VSCROLL | WS_VISIBLE);

    // set style
    SetWindowLongPtrW(hwnd, GWL_STYLE, style);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exstyle);

    // set text with " (Not Responding)"
    LoadStringW(User32Instance, IDS_NOT_RESPONDING,
                szNotRespondingW, ARRAYSIZE(szNotRespondingW));
    StringCchCatW(pszTextW, cchTextW, szNotRespondingW);
    SetWindowTextW(hwnd, pszTextW);

    // free the text buffer
    HeapFree(GetProcessHeap(), 0, pszTextW);

    // get previous window of target
    hwndPrev = GetWindow(hwndTarget, GW_HWNDPREV);

    // hide target
    ShowWindowAsync(hwndTarget, SW_HIDE);

    // shrink the ghost to zero size and insert.
    // this will avoid effects.
    SetWindowPos(hwnd, hwndPrev, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_DRAWFRAME);

    // resume the position and size of ghost
    MoveWindow(hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

    // make ghost visible
    SetWindowLongPtrW(hwnd, GWL_STYLE, style | WS_VISIBLE);

    // redraw
    InvalidateRect(hwnd, NULL, TRUE);

    // start timer
    SetTimer(hwnd, GHOST_TIMER_ID, GHOST_INTERVAL, NULL);

    return TRUE;
}

static void
Ghost_Unenchant(HWND hwnd, BOOL bDestroyTarget)
{
    DWORD pid;
    HANDLE hProcess;
    GHOST_DATA *pData;
    RECT rc;

    pData = Ghost_GetData(hwnd);
    if (!pData)
        return;

    RemovePropW(pData->hwndTarget, L"GhostProp");

    DeleteObject(pData->hbm32bpp);
    pData->hbm32bpp = NULL;

    if (bDestroyTarget)
    {
        GetWindowThreadProcessId(pData->hwndTarget, &pid);
        hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess)
        {
            TerminateProcess(hProcess, -1);
            CloseHandle(hProcess);
        }

        DestroyWindow(hwnd);
        ShowWindowAsync(pData->hwndTarget, SW_SHOWNOACTIVATE);

        DestroyWindow(pData->hwndTarget);
        pData->hwndTarget = NULL;
    }
    else
    {
        GetWindowRect(hwnd, &rc);
        SetWindowPos(pData->hwndTarget, NULL, rc.left, rc.top, 0, 0,
            SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

        ShowWindowAsync(pData->hwndTarget, SW_SHOWNOACTIVATE);
        DestroyWindow(hwnd);
    }
}

static void
Ghost_OnDraw(HWND hwnd, HDC hdc)
{
    BITMAP bm;
    HDC hdcMem;
    GHOST_DATA *pData = Ghost_GetData(hwnd);

    if (!pData || !GetObject(pData->hbm32bpp, sizeof(bm), &bm))
        return;

    hdcMem = CreateCompatibleDC(hdc);
    if (hdcMem)
    {
        HGDIOBJ hbmOld = SelectObject(hdcMem, pData->hbm32bpp);
        {
            BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY | CAPTUREBLT);
        }
        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);
    }
}

static void
Ghost_OnNCPaint(HWND hwnd, HRGN hrgn)
{
    HDC hdc;

    // do the default behaivour
    DefWindowProcW(hwnd, WM_NCPAINT, (WPARAM)hrgn, 0);

    // draw the original image
    hdc = GetWindowDC(hwnd);
    if (hdc)
    {
        Ghost_OnDraw(hwnd, hdc);
        ReleaseDC(hwnd, hdc);
    }
}

static void
Ghost_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    if (hdc)
    {
        // don't draw at here
        EndPaint(hwnd, &ps);
    }
}

static void
Ghost_OnMove(HWND hwnd, int x, int y)
{
    RECT rc;
    HWND hwndTarget = Ghost_GetTarget(hwnd);

    GetWindowRect(hwnd, &rc);

    // move the target
    SetWindowPos(hwndTarget, NULL, rc.left, rc.top, 0, 0,
        SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

static void
Ghost_OnDestroy(HWND hwnd)
{
    KillTimer(hwnd, GHOST_TIMER_ID);
}

static void
Ghost_OnNCDestroy(HWND hwnd)
{
    // delete the user data
    GHOST_DATA *pData = Ghost_GetData(hwnd);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    HeapFree(GetProcessHeap(), 0, pData);

    NtUserSetWindowFNID(hwnd, FNID_DESTROY);

    PostQuitMessage(0);
}

static void
Ghost_OnClose(HWND hwnd)
{
    INT id;
    WCHAR szAskTerminate[128];
    WCHAR szHungUpTitle[128];

    // stop timer
    KillTimer(hwnd, GHOST_TIMER_ID);

    LoadStringW(User32Instance, IDS_ASK_TERMINATE,
                szAskTerminate, ARRAYSIZE(szAskTerminate));
    LoadStringW(User32Instance, IDS_HUNG_UP_TITLE,
                szHungUpTitle, ARRAYSIZE(szHungUpTitle));

    id = MessageBoxW(hwnd, szAskTerminate, szHungUpTitle,
                     MB_ICONINFORMATION | MB_YESNO);
    if (id == IDYES)
    {
        // destroy the target
        Ghost_Unenchant(hwnd, TRUE);
        return;
    }

    // restart timer
    SetTimer(hwnd, GHOST_TIMER_ID, 1000, NULL);
}

static void
Ghost_OnTimer(HWND hwnd, UINT id)
{
    HWND hwndTarget;
    DWORD_PTR dwResult;
    DWORD dwTimeout;
    DWORD dwTick1, dwTick2;
    GHOST_DATA *pData = Ghost_GetData(hwnd);

    if (id != GHOST_TIMER_ID || !pData)
        return;

    // stop the timer
    KillTimer(hwnd, id);

    hwndTarget = pData->hwndTarget;
    if (!IsWindow(hwndTarget) || IsIconic(hwnd) || IsIconic(hwndTarget))
    {
        // resume if window is destroyed
        Ghost_Unenchant(hwnd, FALSE);
        return;
    }

    // resume if responding
    dwTimeout = 200;
    dwTick1 = GetTickCount();
    SendMessageTimeout(hwndTarget, WM_NULL, 0, 0, 0, dwTimeout, &dwResult);
    dwTick2 = GetTickCount();
    if (dwTick2 - dwTick1 < dwTimeout)
    {
        Ghost_Unenchant(hwnd, FALSE);
        return;
    }

    // restart the timer
    SetTimer(hwnd, GHOST_TIMER_ID, 1000, NULL);
}

static HICON
Ghost_GetIcon(HWND hwnd, INT fType)
{
    GHOST_DATA *pData = Ghost_GetData(hwnd);
    HICON hIcon = NULL;

    if (!pData)
        return NULL;

    // same as the original icon
    switch (fType)
    {
        case ICON_BIG:
            hIcon = (HICON)GetClassLongPtrW(pData->hwndTarget, GCLP_HICON);
            break;

        case ICON_SMALL:
            hIcon = (HICON)GetClassLongPtrW(pData->hwndTarget, GCLP_HICONSM);
            break;
    }

    return hIcon;
}

LRESULT WINAPI
GhostWndProc_common(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL unicode)
{
    switch (uMsg)
    {
        case WM_CREATE:
            if (!Ghost_OnCreate(hwnd, (CREATESTRUCTW *)lParam))
                return -1;
            break;

        case WM_NCPAINT:
            Ghost_OnNCPaint(hwnd, (HRGN)wParam);
            return 0;

        case WM_ERASEBKGND:
            Ghost_OnNCPaint(hwnd, NULL);
            return TRUE;

        case WM_PAINT:
            Ghost_OnPaint(hwnd);
            break;

        case WM_MOVE:
            Ghost_OnMove(hwnd, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));
            break;

        case WM_SIZE:
            break;

        case WM_SIZING:
            return TRUE;

        case WM_SYSCOMMAND:
            switch ((UINT)wParam)
            {
                case SC_MAXIMIZE:
                case SC_MINIMIZE:
                case SC_SIZE:
                    // sizing-related
                    return 0;
            }
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);

        case WM_CLOSE:
            Ghost_OnClose(hwnd);
            break;

        case WM_TIMER:
            Ghost_OnTimer(hwnd, (UINT)wParam);
            break;

        case WM_NCMOUSEMOVE:
            DefWindowProcW(hwnd, uMsg, wParam, lParam);
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            return 0;

        case WM_GETICON:
            return (LRESULT)Ghost_GetIcon(hwnd, (INT)wParam);

        case WM_COMMAND:
            if (LOWORD(wParam) == 3333)
                Ghost_Unenchant(hwnd, FALSE);
            break;

        case WM_DESTROY:
            Ghost_OnDestroy(hwnd);
            break;

        case WM_NCDESTROY:
            Ghost_OnNCDestroy(hwnd);
            break;

        default:
        {
            if (unicode)
                return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            else
                return DefWindowProcA(hwnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

LRESULT CALLBACK GhostWndProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return GhostWndProc_common(hwnd, uMsg, wParam, lParam, FALSE);
}

LRESULT CALLBACK GhostWndProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return GhostWndProc_common(hwnd, uMsg, wParam, lParam, TRUE);
}
