/*
 * PROJECT:     ReactOS user32.dll
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Ghost window class
 * COPYRIGHT:   Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <user32.h>
#include <strsafe.h>
#include <undocuser.h>
#include "ghostwnd.h"

WINE_DEFAULT_DEBUG_CHANNEL(UserInput);

#define GHOST_TIMER_ID  0xFACEDEAD
#define GHOST_INTERVAL  1000        // one second

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

    hdc = GetDC(hwnd);
    if (!hdc)
        return NULL;

    hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem)
        goto earth;

    hbm = IntCreate32BppBitmap(cx, cy);
    if (hbm)
    {
        hbmOld = SelectObject(hdcMem, hbm);
        BitBlt(hdcMem, 0, 0, cx, cy, hdc, 0, 0, SRCCOPY | CAPTUREBLT);
        SelectObject(hdcMem, hbmOld);
    }

    DeleteDC(hdcMem);

earth:
    ReleaseDC(hwnd, hdc);

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
        *pdw = *pdw | 0x00A0A0A0;   // bitwise-OR with ARGB #A0A0A0
        ++pdw;
    }
}

/****************************************************************************/

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
            ERR("HeapAlloc failed\n");
            if (pszTextW)
                HeapFree(GetProcessHeap(), 0, pszTextW);
            return NULL;
        }
        pszTextW = pszTextNewW;

        cchNonExtra = cchTextW - cchExtra;
        if (InternalGetWindowText(hwndTarget, pszTextW,
                                  cchNonExtra) < cchNonExtra - 1)
        {
            break;
        }

        cchTextW *= 2;
        pszTextNewW = HeapReAlloc(GetProcessHeap(), 0, pszTextW,
                                  cchTextW * sizeof(WCHAR));
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
    WCHAR szClassW[64], szTextW[320], szNotRespondingW[32];
    LPWSTR pszTextW;
    INT cchTextW, cchExtraW, cchNonExtraW;
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
             return FALSE;
        }
    }

    // get the target
    hwndTarget = (HWND)lpcs->lpCreateParams;
    if (!IsWindowVisible(hwndTarget) ||                             // invisible?
        (GetWindowLongPtrW(hwndTarget, GWL_STYLE) & WS_CHILD) ||    // child?
        !IsHungAppWindow(hwndTarget))                               // not hung?
    {
        ERR("Ghost: invalid target\n");
        return FALSE;
    }

    // get class name
    if (!GetClassNameW(hwndTarget, szClassW, ARRAYSIZE(szClassW)))
    {
        ERR("GetClassNameW(hwndTarget: %p)\n", (void *)hwndTarget);
        return FALSE;
    }

    TRACE("Ghost_OnCreate: hwndTarget(%p), szClassW(%S)\n", (void *)hwndTarget,
          szClassW);

    if (lstrcmpiW(szClassW, L"Ghost") == 0)
    {
        ERR("hwndTarget:%p was ghost window\n", (void *)hwndTarget);
        return FALSE;
    }

    if (lstrcmpiW(szClassW, L"Progman") == 0 ||
        lstrcmpiW(szClassW, L"Shell_TrayWnd") == 0)
    {
        ERR("hwndTarget:%p was special window\n", (void *)hwndTarget);
        return FALSE;
    }

    GetWindowRect(hwndTarget, &rc);
    if (IsRectEmpty(&rc))
    {
        ERR("IsRectEmpty: %p\n", (void *)hwndTarget);
        return FALSE;
    }

    // create user data
    pData = HeapAlloc(GetProcessHeap(), 0, sizeof(GHOST_DATA));
    if (!pData)
    {
        ERR("HeapAlloc failed\n");
        return FALSE;
    }

    pData->rcWindow = rc;

    // get window image
    GetClientRect(hwndTarget, &rc);
    hbm32bpp = IntGetWindowBitmap(hwndTarget,
                                  rc.right - rc.left,
                                  rc.bottom - rc.top);
    if (!hbm32bpp)
    {
        ERR("hbm32bpp was NULL\n");
        HeapFree(GetProcessHeap(), 0, pData);
        return FALSE;
    }
    // make a ghost image
    IntMakeGhostImage(hbm32bpp);

    // set user data
    pData->hwndTarget = hwndTarget;
    pData->hbm32bpp = hbm32bpp;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pData);

    // get style
    style = GetWindowLongPtrW(hwndTarget, GWL_STYLE);
    exstyle = GetWindowLongPtrW(hwndTarget, GWL_EXSTYLE);
    pData->style = style;
    pData->exstyle = exstyle;

    // get text
    cchTextW = ARRAYSIZE(szTextW);
    cchExtraW = ARRAYSIZE(szNotRespondingW);
    cchNonExtraW = cchTextW - cchExtraW;
    if (InternalGetWindowText(hwndTarget, szTextW,
                              cchNonExtraW) < cchNonExtraW - 1)
    {
        pszTextW = szTextW;
    }
    else
    {
        cchTextW *= 2;
        pszTextW = Ghost_GetText(hwndTarget, &cchTextW, cchExtraW);
        if (!pszTextW)
        {
            ERR("Ghost_GetText failed\n");
            DeleteObject(hbm32bpp);
            HeapFree(GetProcessHeap(), 0, pData);
            return FALSE;
        }
    }

    // don't use scrollbars.
    style &= ~(WS_HSCROLL | WS_VSCROLL | WS_VISIBLE);

    // set style
    SetWindowLongPtrW(hwnd, GWL_STYLE, style);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exstyle | WS_EX_MAKEVISIBLEWHENUNGHOSTED);

    // set text with " (Not Responding)"
    LoadStringW(User32Instance, IDS_NOT_RESPONDING,
                szNotRespondingW, ARRAYSIZE(szNotRespondingW));
    StringCchCatW(pszTextW, cchTextW, szNotRespondingW);
    SetWindowTextW(hwnd, pszTextW);

    // free the text buffer
    if (szTextW != pszTextW)
        HeapFree(GetProcessHeap(), 0, pszTextW);

    // get previous window of target
    hwndPrev = GetWindow(hwndTarget, GW_HWNDPREV);

    // hide target by setting zero size
    SetWindowPos(hwndTarget, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER |
                 SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_NOZORDER);
    // hide from taskbar
    SetWindowLongPtrW(hwndTarget, GWL_EXSTYLE, pData->exstyle | WS_EX_TOOLWINDOW);

    // insert the ghost
    GetWindowRect(hwndTarget, &rc);
    SetWindowPos(hwnd, hwndPrev,
                 rc.left, rc.top,
                 rc.right - rc.left, rc.bottom - rc.top,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_DRAWFRAME);

    // redraw
    InvalidateRect(hwnd, NULL, TRUE);

    // start timer
    SetTimer(hwnd, GHOST_TIMER_ID, GHOST_INTERVAL, NULL);

    return TRUE;
}

static BOOL
Ghost_Unenchant(HWND hwnd, BOOL bDestroyTarget)
{
    DWORD exstyle;
    GHOST_DATA *pData = Ghost_GetData(hwnd);
    if (!pData)
        return FALSE;

    exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (bDestroyTarget)
    {
        exstyle &= ~WS_EX_MAKEVISIBLEWHENUNGHOSTED;
    }
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exstyle);

    return DestroyWindow(hwnd);
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
        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight,
               hdcMem, 0, 0, SRCCOPY | CAPTUREBLT);
        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);
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
                 SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE |
                 SWP_NOSENDCHANGING);
}

static void
Ghost_OnDestroy(HWND hwnd)
{
    KillTimer(hwnd, GHOST_TIMER_ID);
}

static void
Ghost_DestroyTarget(GHOST_DATA *pData)
{
    HWND hwndTarget = pData->hwndTarget;
    DWORD pid;
    HANDLE hProcess;

    pData->hwndTarget = NULL;
    GetWindowThreadProcessId(hwndTarget, &pid);

    hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess)
    {
        TerminateProcess(hProcess, -1);
        CloseHandle(hProcess);
    }

    DestroyWindow(hwndTarget);
}

static void
Ghost_OnNCDestroy(HWND hwnd)
{
    DWORD dwThreadID = GetWindowThreadProcessId(hwnd, NULL);
    HWND hwndTarget = NULL;
    DWORD exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    // delete the user data
    GHOST_DATA *pData = Ghost_GetData(hwnd);
    if (pData)
    {
        hwndTarget = pData->hwndTarget;

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);

        // delete image
        DeleteObject(pData->hbm32bpp);
        pData->hbm32bpp = NULL;

        // show target
        SetWindowPos(pData->hwndTarget, NULL,
                     pData->rcWindow.left,
                     pData->rcWindow.top,
                     pData->rcWindow.right - pData->rcWindow.left,
                     pData->rcWindow.bottom - pData->rcWindow.top,
                     SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_NOOWNERZORDER |
                     SWP_NOREPOSITION | SWP_NOSENDCHANGING | SWP_NOZORDER);
        SetWindowLongPtr(pData->hwndTarget, GWL_STYLE, pData->style);
        SetWindowLongPtr(pData->hwndTarget, GWL_EXSTYLE, pData->exstyle);

        // destroy target if necessary
        if (!(exstyle & WS_EX_MAKEVISIBLEWHENUNGHOSTED))
        {
            Ghost_DestroyTarget(pData);
        }

        HeapFree(GetProcessHeap(), 0, pData);
    }

    NtUserSetWindowFNID(hwnd, FNID_DESTROY);

    PostThreadMessage(dwThreadID, GTM_GHOST_DESTROYED,
                      (WPARAM)hwndTarget, (LPARAM)hwnd);
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
    SetTimer(hwnd, GHOST_TIMER_ID, GHOST_INTERVAL, NULL);
}

static void
Ghost_OnTimer(HWND hwnd, UINT id)
{
    HWND hwndTarget;
    GHOST_DATA *pData = Ghost_GetData(hwnd);

    if (id != GHOST_TIMER_ID || !pData)
        return;

    // stop the timer
    KillTimer(hwnd, id);

    hwndTarget = pData->hwndTarget;
    if (!IsWindow(hwndTarget) || !IsHungAppWindow(hwndTarget))
    {
        // resume if window is destroyed or responding
        Ghost_Unenchant(hwnd, FALSE);
        return;
    }

    // restart the timer
    SetTimer(hwnd, GHOST_TIMER_ID, GHOST_INTERVAL, NULL);
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
        {
            hIcon = (HICON)GetClassLongPtrW(pData->hwndTarget, GCLP_HICON);
            break;
        }

        case ICON_SMALL:
        {
            hIcon = (HICON)GetClassLongPtrW(pData->hwndTarget, GCLP_HICONSM);
            break;
        }
    }

    return hIcon;
}

LRESULT WINAPI
GhostWndProc_common(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                    BOOL unicode)
{
    switch (uMsg)
    {
        case WM_CREATE:
            if (!Ghost_OnCreate(hwnd, (CREATESTRUCTW *)lParam))
                return -1;
            break;

        case WM_ERASEBKGND:
            Ghost_OnDraw(hwnd, (HDC)wParam);
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

        case GWM_UNGHOST:
            return Ghost_Unenchant(hwnd, (BOOL)wParam);

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

LRESULT CALLBACK
GhostWndProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return GhostWndProc_common(hwnd, uMsg, wParam, lParam, FALSE);
}

LRESULT CALLBACK
GhostWndProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return GhostWndProc_common(hwnd, uMsg, wParam, lParam, TRUE);
}
