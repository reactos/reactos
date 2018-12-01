/*
 * PROJECT:         ReactOS user32.dll
 * PURPOSE:         Ghost window
 * FILE:            win32ss/user/user32/controls/ghost.c
 * PROGRAMER:       Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(ghost);

#define GHOST_TIMER_ID  0xFACEDEAD
#define GHOST_INTERVAL  1000        // one second

static const WCHAR ghostW[] = L"Ghost";
const struct builtin_class_descr GHOST_builtin_class =
{
    ghostW,                     /* name */
    0,                          /* style  */
    GhostWndProcA,              /* procA */
    GhostWndProcW,              /* procW */
    0,                          /* extra */
    IDC_WAIT,                   /* cursor */
    (HBRUSH)(COLOR_3DFACE + 1)  /* brush */
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
        hbm = IntCreate32BppBitmap(cx, cy);
        if (hbm)
        {
            hdcMem = CreateCompatibleDC(hdc);
            if (hdcMem)
            {
                hbmOld = SelectObject(hdcMem, hbm);
                {
                    BitBlt(hdcMem, 0, 0, cx, cy, hdc, 0, 0, SRCCOPY | CAPTUREBLT);
                }
                SelectObject(hdcMem, hbmOld);
                DeleteDC(hdcMem);
            }
        }
        ReleaseDC(hwnd, hdc);
    }
    return hbm;
}

static VOID
IntWhiten32BppBitmap(HBITMAP hbm)
{
    BITMAP bm;
    DWORD i, *pdw;

    GetObject(hbm, sizeof(bm), &bm);

    if (bm.bmBitsPixel != 32 || !bm.bmBits)
        return;

    pdw = (DWORD *)bm.bmBits;
    for (i = 0; i < bm.bmWidth * bm.bmHeight; ++i)
    {
        *pdw = *pdw | 0x00C0C0C0;   // ARGB
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
    if (IsWindowUnicode(hwnd))
        return (GHOST_DATA *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    else
        return (GHOST_DATA *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
}

static HWND
Ghost_GetTarget(HWND hwnd)
{
    GHOST_DATA *pData = Ghost_GetData(hwnd);
    if (!pData)
        return NULL;
    return pData->hwndTarget;
}

static BOOL
Ghost_OnCreate(HWND hwnd, CREATESTRUCTW *lpcs)
{
    HBITMAP hbm32bpp;
    HWND hwndTarget, hwndPrev;
    GHOST_DATA *pData;
    RECT rc;
    DWORD style, exstyle;
    CHAR szTextA[128];
    WCHAR szTextW[128];

#ifdef __REACTOS__
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
#endif

    // get the target
    hwndTarget = (HWND)lpcs->lpCreateParams;
    if (!hwndTarget || !IsWindowVisible(hwndTarget) || IsIconic(hwndTarget) ||
        GetParent(hwndTarget))
    {
        return FALSE;
    }

    // create data
    pData = (GHOST_DATA *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GHOST_DATA));
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
    // whiten
    IntWhiten32BppBitmap(hbm32bpp);

    // get style and text
    if (IsWindowUnicode(hwnd))
    {
        style = GetWindowLongPtrW(hwndTarget, GWL_STYLE);
        exstyle = GetWindowLongPtrW(hwndTarget, GWL_EXSTYLE);
        GetWindowTextW(hwndTarget, szTextW, ARRAYSIZE(szTextW) - 32);
    }
    else
    {
        style = GetWindowLongPtrA(hwndTarget, GWL_STYLE);
        exstyle = GetWindowLongPtrA(hwndTarget, GWL_EXSTYLE);
        GetWindowTextA(hwndTarget, szTextA, ARRAYSIZE(szTextW) - 32);
    }

    style &= ~(WS_HSCROLL | WS_VSCROLL | WS_VISIBLE);

    // set style and text
    if (IsWindowUnicode(hwnd))
    {
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exstyle);

        lstrcatW(szTextW, L" (Not Responding)");
        SetWindowTextW(hwnd, szTextW);
    }
    else
    {
        SetWindowLongPtrA(hwnd, GWL_STYLE, style);
        SetWindowLongPtrA(hwnd, GWL_EXSTYLE, exstyle);

        lstrcatA(szTextA, " (Not Responding)");
        SetWindowTextA(hwnd, szTextA);
    }

    hwndPrev = GetWindow(hwndTarget, GW_HWNDPREV);

    // hide target
    ShowWindowAsync(hwndTarget, SW_HIDE);

    // shrink the ghost to zero size and insert
    SetWindowPos(hwnd, hwndPrev, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_DRAWFRAME);

    // resume the position and size of ghost
    MoveWindow(hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

    // make ghost visible
    if (IsWindowUnicode(hwnd))
        SetWindowLongPtrW(hwnd, GWL_STYLE, style | WS_VISIBLE);
    else
        SetWindowLongPtrA(hwnd, GWL_STYLE, style | WS_VISIBLE);

    // redraw
    InvalidateRect(hwnd, NULL, TRUE);

    // set user data
    pData->hwndTarget = hwndTarget;
    pData->hbm32bpp = hbm32bpp;
    if (IsWindowUnicode(hwnd))
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pData);
    else
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)pData);

    SetPropW(hwndTarget, L"GhostProp", hwnd);

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
    if (IsWindowUnicode(hwnd))
        DefWindowProcW(hwnd, WM_NCPAINT, (WPARAM)hrgn, 0);
    else
        DefWindowProcA(hwnd, WM_NCPAINT, (WPARAM)hrgn, 0);

    // draw the original image
    hdc = GetWindowDC(hwnd);
    if (hdc)
    {
        Ghost_OnDraw(hwnd, hdc);
        ReleaseDC(hwnd, hdc);
    }
}

static BOOL
Ghost_OnEraseBkgnd(HWND hwnd, HDC hdc)
{
    // draw the original image
    Ghost_OnNCPaint(hwnd, NULL);
    return TRUE;
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
    if (!IsWindowVisible(hwndTarget))
        return;

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
    if (IsWindowUnicode(hwnd))
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    else
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, 0);
    HeapFree(GetProcessHeap(), 0, pData);

#ifdef __REACTOS__
    NtUserSetWindowFNID(hwnd, FNID_DESTROY);
#endif
}

static void
Ghost_OnClose(HWND hwnd)
{
    KillTimer(hwnd, GHOST_TIMER_ID);

    if (IDYES == MessageBoxW(hwnd, L"Terminate app?", L"Not responding",
                             MB_ICONINFORMATION | MB_YESNO))
    {
        // destroy the target
        Ghost_Unenchant(hwnd, TRUE);
        return;
    }

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

    if (1)
    {
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
        if (IsWindowUnicode(hwnd))
            hIcon = (HICON)GetClassLongPtrW(pData->hwndTarget, GCLP_HICON);
        else
            hIcon = (HICON)GetClassLongPtrA(pData->hwndTarget, GCLP_HICON);
        break;
    case ICON_SMALL:
        if (IsWindowUnicode(hwnd))
            hIcon = (HICON)GetClassLongPtrW(pData->hwndTarget, GCLP_HICONSM);
        else
            hIcon = (HICON)GetClassLongPtrA(pData->hwndTarget, GCLP_HICONSM);
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
            return Ghost_OnEraseBkgnd(hwnd, (HDC)wParam);

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
            if (unicode)
                return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            else
                return DefWindowProcA(hwnd, uMsg, wParam, lParam);

        case WM_CLOSE:
            Ghost_OnClose(hwnd);
            break;

        case WM_TIMER:
            Ghost_OnTimer(hwnd, (UINT)wParam);
            break;

        case WM_NCMOUSEMOVE:
            if (unicode)
                DefWindowProcW(hwnd, uMsg, wParam, lParam);
            else
                DefWindowProcA(hwnd, uMsg, wParam, lParam);
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
