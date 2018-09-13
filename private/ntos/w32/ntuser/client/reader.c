/****************************** Module Header ******************************\
* Module Name: reader.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Implements support reader-mode routines for auto-scrolling and panning.
*
* History:
* 31-Jan-1997   vadimg    created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define TIMERID 1

__inline FReader2Dim(PREADERINFO prdr)
{
    return ((prdr->dwFlags & (RDRMODE_HORZ | RDRMODE_VERT)) == 
            (RDRMODE_HORZ | RDRMODE_VERT));
}
__inline FReaderVert(PREADERINFO prdr)
{
    return (prdr->dwFlags & RDRMODE_VERT);
}
__inline FReaderHorz(PREADERINFO prdr)
{
    return (prdr->dwFlags & RDRMODE_HORZ);
}
__inline FReaderDiag(PREADERINFO prdr)
{
    return (prdr->dwFlags & RDRMODE_DIAG);
}

/***************************************************************************\
* ReaderSetCursor
*
\***************************************************************************/

void ReaderSetCursor(PREADERINFO prdr, UINT uCursor)
{
    if (prdr->uCursor != uCursor) {
        NtUserSetCursor(LoadCursor(NULL, MAKEINTRESOURCE(uCursor)));
        prdr->uCursor = uCursor;
    }
}

/***************************************************************************\
* ReaderMouseMove
*
* Calculate dx and dy based on the flags passed in.  Provide visual 
* feedback for the reader mode by setting the correct cursor.
*
* 2-Feb-1997   vadimg   created
\***************************************************************************/

void ReaderMouseMove(PWND pwnd, PREADERINFO prdr, LPARAM lParam)
{
    int dx = 0, dy = 0;
    LPRECT prc = &pwnd->rcWindow;
    UINT uCursor;
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    
    _ClientToScreen(pwnd, &pt);

    if (FReaderVert(prdr)) {
        if (pt.y < prc->top) {
            dy = pt.y - prc->top;
        } else if (pt.y > prc->bottom) {
            dy = pt.y - prc->bottom;
        }
    }

    if (FReaderHorz(prdr)) {
        if (pt.x < prc->left) {
            dx = pt.x - prc->left;
        } else if (pt.x > prc->right) {
            dx = pt.x - prc->right;
        }
    }
    
    if (FReader2Dim(prdr)) {
        if (dx == 0 && dy == 0) {
            ReaderSetCursor(prdr, OCR_RDR2DIM);
            goto Exit;
        }
        if (!FReaderDiag(prdr)) {
            if (prdr->dy != 0) {
                if (abs(dx) > abs(prdr->dy)) {
                    dy = 0;
                } else {
                    dx = 0;
                }
            } else if (prdr->dx != 0) {
                if (abs(dy) > abs(prdr->dx)) {
                    dx = 0;
                } else {
                    dy = 0;
                }
            } else if (dy != 0) {
                dx = 0;
            }
        }
    } else if (FReaderVert(prdr) && dy == 0) {
        ReaderSetCursor(prdr, OCR_RDRVERT);
        goto Exit;
    } else if (FReaderHorz(prdr) && dx == 0) {
        ReaderSetCursor(prdr, OCR_RDRHORZ);
        goto Exit;
    }

    if (dx == 0) {
        uCursor = (dy > 0) ? OCR_RDRSOUTH : OCR_RDRNORTH;
    } else if (dx > 0) {
        if (dy == 0) {
            uCursor = OCR_RDREAST;
        } else {
            uCursor = (dy > 0) ? OCR_RDRSOUTHEAST : OCR_RDRNORTHEAST;
        }
    } else if (dx < 0) {
        if (dy == 0) {
            uCursor = OCR_RDRWEST;
        } else {
            uCursor = (dy > 0) ? OCR_RDRSOUTHWEST : OCR_RDRNORTHWEST;
        }
    }

    ReaderSetCursor(prdr, uCursor);

Exit:
    prdr->dx = dx;
    prdr->dy = dy;
}

/***************************************************************************\
* ReaderFeedback
*
* 2-Feb-1997   vadimg   created
\***************************************************************************/

void ReaderFeedback(PWND pwnd, PREADERINFO prdr)
{
    if (prdr->dx == 0 && prdr->dy == 0)
        return;

    if (prdr->pfnReaderModeProc(prdr->lParam, RDRCODE_SCROLL, 
            prdr->dx, prdr->dy) == 0) {
        NtUserDestroyWindow(PtoH(pwnd));
    }
}

/***************************************************************************\
* ReaderWndProc
*
* 31-Jan-1997   vadimg   created
\***************************************************************************/

LRESULT CALLBACK ReaderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc, hdcMem;
    HPEN hpen, hpenOld;
    HBRUSH hbrOld;
    HRGN hrgn;
    RECT rc;
    POINT pt;
    int nBitmap, cx, cy;
    PREADERINFO prdr;
    PWND pwnd;
    LPCREATESTRUCT pcs;
    PREADERMODE prdrm;
    BITMAP bmp;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL)
        return 0;

    prdr = ((PREADERWND)pwnd)->prdr;

    switch (msg) {
    case WM_TIMER:
        ReaderFeedback(pwnd, prdr);
        return 0;

    case WM_MOUSEWHEEL:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_XBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_KEYDOWN:
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        ReaderMouseMove(pwnd, prdr, lParam);
        return 0;

    case WM_MBUTTONUP:
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        GetClientRect(hwnd, &rc);
        if (!PtInRect(&rc, pt)) {
            ReleaseCapture();
        }
        return 0;

    case WM_CAPTURECHANGED:
        NtUserDestroyWindow(hwnd);
        return 0;

    case WM_NCDESTROY:
        NtUserKillTimer(hwnd, TIMERID);
        
        prdr->pfnReaderModeProc(prdr->lParam, RDRCODE_END, 0, 0);
        
        if (prdr->hbm != NULL) {
            DeleteObject(prdr->hbm);
        }
        UserLocalFree(prdr);
        return 0;

    case WM_CREATE:
        if ((prdr = UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(READERINFO))) == NULL)
            return -1;

        pcs = (LPCREATESTRUCT)lParam;
        prdrm = (PREADERMODE)pcs->lpCreateParams;
        RtlCopyMemory(prdr, prdrm, sizeof(READERMODE));
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)prdr);

        if (prdr->pfnReaderModeProc == NULL) {
            return -1;
        }

        if (FReader2Dim(prdr)) {
            nBitmap = OBM_RDR2DIM;
        } else if (FReaderVert(prdr)) {
            nBitmap = OBM_RDRVERT;
        } else if (FReaderHorz(prdr)) {
            nBitmap = OBM_RDRHORZ;
        } else {
            return -1;
        }

        SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
        SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_CLIPSIBLINGS);

        prdr->hbm = LoadBitmap(hmodUser, MAKEINTRESOURCE(nBitmap));
        if (prdr->hbm == NULL || 
                GetObject(prdr->hbm, sizeof(BITMAP), &bmp) == 0) {
            return -1;
        }

        if (prdr->pfnReaderModeProc(prdr->lParam, RDRCODE_START, 0, 0) == 0) {
            return -1;
        }

        prdr->dxBmp = bmp.bmWidth;
        prdr->dyBmp = bmp.bmHeight;

        cx = bmp.bmWidth + 1;
        cy = bmp.bmHeight + 1;

        GetCursorPos(&pt);
        pt.x -= cx/2;
        pt.y -= cy/2;

        if ((hrgn = CreateEllipticRgn(0, 0, cx, cy)) != NULL) {
            SetWindowRgn(hwnd, hrgn, FALSE);
        }

        NtUserSetWindowPos(hwnd, HWND_TOPMOST, pt.x, pt.y, cx, cy,
                SWP_SHOWWINDOW | SWP_NOACTIVATE);

        NtUserSetCapture(hwnd);
        NtUserSetFocus(hwnd);
        NtUserSetTimer(hwnd, TIMERID, 10, NULL);
        return 0;

    case WM_ERASEBKGND:
        hdc = (HDC)wParam;
    
        if ((hdcMem = CreateCompatibleDC(hdc)) == NULL)
            return FALSE;

        SelectObject(hdcMem, prdr->hbm);
        hpen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        hpenOld = (HPEN)SelectObject(hdc, hpen);
        hbrOld = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        
        BitBlt(hdc, 0, 0, prdr->dxBmp, prdr->dyBmp, hdcMem, 0, 0, SRCCOPY);
        Ellipse(hdc, 0, 0, prdr->dxBmp, prdr->dyBmp);
        
        SelectObject(hdc, hpenOld);
        SelectObject(hdc, hbrOld);

        DeleteObject(hpen);
        DeleteObject(hdcMem);
        return TRUE;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/***************************************************************************\
* ReaderProcInternal
*
\***************************************************************************/

LONG ReaderProcInternal(LPARAM lParam, int nCode, int dx, int dy)
{
    DWORD dwDelay;
    UINT uMsg, uCode;
    int n, nAbs;

    if (nCode != RDRCODE_SCROLL)
        return TRUE;

    if (dy != 0) {
        uCode = SB_LINEUP;
        uMsg = WM_VSCROLL;
        n = dy;
    } else {
        uCode = SB_LINELEFT;
        uMsg = WM_HSCROLL;
        n = dx;
    }

    nAbs = abs(n);
    if (nAbs >= 120) {
        uCode += 2;
        dwDelay = 0;
    } else {
        dwDelay = 1000 - (nAbs / 2) * 15;
    }

    if (n > 0) {
        uCode += 1;
    }

    SendMessage((HWND)lParam, uMsg, MAKELONG(uCode, dwDelay), 0);
    UpdateWindow((HWND)lParam);
    return TRUE;
}

/***************************************************************************\
* EnterReaderMode
*
\***************************************************************************/

#define READERCLASS L"User32_ReaderMode"
ATOM gatomReaderMode = 0;

BOOL EnterReaderMode(PREADERMODE prdrm)
{
    WNDCLASSEX wce;

    if (GetCapture() != NULL)
        return FALSE;

    if (gatomReaderMode == 0) {
        wce.cbSize = sizeof(wce);
        wce.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wce.lpfnWndProc = ReaderWndProc;
        wce.cbClsExtra = 0;
        wce.cbWndExtra = sizeof(PREADERINFO);
        wce.hInstance = hmodUser;
        wce.hIcon = NULL;
        wce.hCursor = LoadCursor(NULL, IDC_ARROW);
        wce.hbrBackground = GetStockObject(WHITE_BRUSH);
        wce.lpszMenuName = NULL;
        wce.lpszClassName = READERCLASS;
        wce.hIconSm = NULL;

        if ((gatomReaderMode = RegisterClassExWOWW(&wce, NULL, FNID_DDE_BIT)) == 0) {
            RIPMSG0(RIP_WARNING, "EnterReaderMode: failed to register ReaderMode");
            return 0;
        }
    }

    return (_CreateWindowEx(0, READERCLASS, NULL, 0, 0, 0, 0, 0,
            NULL, NULL, hmodUser, (PVOID)prdrm, 0) != NULL);
}

/***************************************************************************\
* FScrollEnabled
*
\***************************************************************************/

BOOL FScrollEnabled(PWND pwnd, BOOL fVert)
{
    PSBINFO pw;
    int nFlags;

    if (!TestWF(pwnd, fVert ? WFVPRESENT : WFHPRESENT))
        return FALSE;

    if ((pw = (PSBINFO)REBASEALWAYS(pwnd, pSBInfo)) == NULL)
        return FALSE;

    nFlags = fVert ? (pw->WSBflags >> 2) : pw->WSBflags;

    if ((nFlags & SB_DISABLE_MASK) == SB_DISABLE_MASK)
        return FALSE;

    return TRUE;
}

/***************************************************************************\
* EnterReaderModeHelper
*
\***************************************************************************/

BOOL EnterReaderModeHelper(HWND hwnd)
{
    PWND pwnd = ValidateHwnd(hwnd);
    READERMODE rdrm;

    rdrm.cbSize = sizeof(READERMODE);
    rdrm.pfnReaderModeProc = ReaderProcInternal;
    rdrm.lParam = (LPARAM)hwnd;
    rdrm.dwFlags = 0;

    if (FScrollEnabled(pwnd, TRUE)) {
        rdrm.dwFlags |= RDRMODE_VERT;
    }
    if (FScrollEnabled(pwnd, FALSE)) {
        rdrm.dwFlags |= RDRMODE_HORZ;
    }

    return EnterReaderMode(&rdrm);
}
