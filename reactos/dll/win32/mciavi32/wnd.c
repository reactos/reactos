/*
 * Digital video MCI Wine Driver
 *
 * Copyright 1999, 2000 Eric POUECH
 * Copyright 2003 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <string.h>
#include "private_mciavi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciavi);

static const WCHAR mciaviW[] = {'M','C','I','A','V','I',0};

static LRESULT WINAPI MCIAVI_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p msg=%x wparam=%lx lparam=%lx\n", hWnd, uMsg, wParam, lParam);

    switch (uMsg) {
    case WM_CREATE:
        SetWindowLongW(hWnd, 0, (LPARAM)((CREATESTRUCTW *)lParam)->lpCreateParams);
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);

    case WM_DESTROY:
        MCIAVI_mciClose(GetWindowLongW(hWnd, 0), MCI_WAIT, NULL);
        SetWindowLongW(hWnd, 0, 0);
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);

    case WM_ERASEBKGND:
	{
	    RECT	rect;
	    GetClientRect(hWnd, &rect);
	    FillRect((HDC)wParam, &rect, GetStockObject(BLACK_BRUSH));
	}
       return 1;

    case WM_PAINT:
        {
            WINE_MCIAVI *wma = (WINE_MCIAVI *)mciGetDriverData(GetWindowLongW(hWnd, 0));

            if (!wma)
                return DefWindowProcW(hWnd, uMsg, wParam, lParam);
            
            EnterCriticalSection(&wma->cs);

            /* the animation isn't playing, don't paint */
	    if (wma->dwStatus == MCI_MODE_NOT_READY)
            {
                LeaveCriticalSection(&wma->cs);
		/* default paint handling */
                return DefWindowProcW(hWnd, uMsg, wParam, lParam);
            }

            if (wParam)
                MCIAVI_PaintFrame(wma, (HDC)wParam);
            else
            {
	        PAINTSTRUCT ps;
                BeginPaint(hWnd, &ps);
                MCIAVI_PaintFrame(wma, ps.hdc);
	        EndPaint(hWnd, &ps);
	    }

            LeaveCriticalSection(&wma->cs);
        }
       return 1;

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

BOOL MCIAVI_UnregisterClass(void)
{
    return UnregisterClassW(mciaviW, MCIAVI_hInstance);
}

BOOL MCIAVI_RegisterClass(void)
{
    WNDCLASSW wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_DBLCLKS;
    wndClass.lpfnWndProc   = MCIAVI_WindowProc;
    wndClass.cbWndExtra    = sizeof(MCIDEVICEID);
    wndClass.hInstance     = MCIAVI_hInstance;
    wndClass.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = mciaviW;

    if (RegisterClassW(&wndClass)) return TRUE;
    if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS) return TRUE;

    return FALSE;
}

BOOL    MCIAVI_CreateWindow(WINE_MCIAVI* wma, DWORD dwFlags, LPMCI_DGV_OPEN_PARMSW lpOpenParms)
{
    static const WCHAR captionW[] = {'W','i','n','e',' ','M','C','I','-','A','V','I',' ','p','l','a','y','e','r',0};
    HWND	hParent = 0;
    DWORD	dwStyle = WS_OVERLAPPEDWINDOW;
    RECT        rc;

    /* what should be done ? */
    if (wma->hWnd) return TRUE;

    if (dwFlags & MCI_DGV_OPEN_PARENT)	hParent = lpOpenParms->hWndParent;
    if (dwFlags & MCI_DGV_OPEN_WS)	dwStyle = lpOpenParms->dwStyle;

    rc.left = rc.top = 0;
    rc.right = (wma->hic ? wma->outbih : wma->inbih)->biWidth;
    rc.bottom = (wma->hic ? wma->outbih : wma->inbih)->biHeight;
    AdjustWindowRect(&rc, dwStyle, FALSE);
    if (!(dwStyle & (WS_CHILD|WS_POPUP))) /* overlapped window ? */
    {
        rc.right -= rc.left;
        rc.bottom -= rc.top;
        rc.left = rc.top = CW_USEDEFAULT;
    }

    wma->hWnd = CreateWindowW(mciaviW, captionW,
                              dwStyle, rc.left, rc.top,
                              rc.right, rc.bottom,
                              hParent, 0, MCIAVI_hInstance,
                              ULongToPtr(wma->wDevID));
    wma->hWndPaint = wma->hWnd;
    return wma->hWnd != 0;
}

/***************************************************************************
 * 				MCIAVI_mciPut			[internal]
 */
DWORD	MCIAVI_mciPut(UINT wDevID, DWORD dwFlags, LPMCI_DGV_PUT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);
    RECT		rc;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_DGV_RECT) {
        /* In MCI, RECT structure is used differently: rc.right = width & rc.bottom = height
         * So convert input MCI RECT into a normal RECT */
        rc.left = lpParms->rc.left;
        rc.top = lpParms->rc.top;
        rc.right = lpParms->rc.left + lpParms->rc.right;
        rc.bottom = lpParms->rc.top + lpParms->rc.bottom;
    } else {
        GetClientRect(wma->hWndPaint, &rc);
    }

    if (dwFlags & MCI_DGV_PUT_CLIENT) {
        FIXME("PUT_CLIENT %s\n", wine_dbgstr_rect(&rc));
        LeaveCriticalSection(&wma->cs);
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_PUT_DESTINATION) {
        TRACE("PUT_DESTINATION %s\n", wine_dbgstr_rect(&rc));
        wma->dest = rc;
    }
    if (dwFlags & MCI_DGV_PUT_FRAME) {
        FIXME("PUT_FRAME %s\n", wine_dbgstr_rect(&rc));
        LeaveCriticalSection(&wma->cs);
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_PUT_SOURCE) {
        TRACE("PUT_SOURCE %s\n", wine_dbgstr_rect(&rc));
        wma->source = rc;
    }
    if (dwFlags & MCI_DGV_PUT_VIDEO) {
        FIXME("PUT_VIDEO %s\n", wine_dbgstr_rect(&rc));
        LeaveCriticalSection(&wma->cs);
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_PUT_WINDOW) {
        TRACE("PUT_WINDOW %s\n", wine_dbgstr_rect(&rc));
        SetWindowPos(wma->hWndPaint, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
    }
    LeaveCriticalSection(&wma->cs);
    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciWhere			[internal]
 */
DWORD	MCIAVI_mciWhere(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);
    RECT		rc;

    TRACE("(%04x, %08x, %p)\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_DGV_WHERE_DESTINATION) {
	if (dwFlags & MCI_DGV_WHERE_MAX) {
	    GetClientRect(wma->hWndPaint, &rc);
	    TRACE("WHERE_DESTINATION_MAX %s\n", wine_dbgstr_rect(&rc));
	} else {
	    TRACE("WHERE_DESTINATION %s\n", wine_dbgstr_rect(&wma->dest));
	    rc = wma->dest;
	}
    }
    if (dwFlags & MCI_DGV_WHERE_FRAME) {
	if (dwFlags & MCI_DGV_WHERE_MAX)
	    FIXME("MCI_DGV_WHERE_FRAME_MAX\n");
	else
	    FIXME("MCI_DGV_WHERE_FRAME\n");
	LeaveCriticalSection(&wma->cs);
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_WHERE_SOURCE) {
	if (dwFlags & MCI_DGV_WHERE_MAX) {
	    rc.left = 0;
	    rc.top = 0;
	    rc.right = wma->inbih->biWidth;
	    rc.bottom = wma->inbih->biHeight;
	    TRACE("WHERE_SOURCE_MAX %s\n", wine_dbgstr_rect(&rc));
 	} else {
	    TRACE("WHERE_SOURCE %s\n", wine_dbgstr_rect(&wma->source));
	    rc = wma->source;
	}
    }
    if (dwFlags & MCI_DGV_WHERE_VIDEO) {
	if (dwFlags & MCI_DGV_WHERE_MAX)
	    FIXME("WHERE_VIDEO_MAX\n");
	else
	    FIXME("WHERE_VIDEO\n");
	LeaveCriticalSection(&wma->cs);
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_WHERE_WINDOW) {
	if (dwFlags & MCI_DGV_WHERE_MAX) {
	    GetWindowRect(GetDesktopWindow(), &rc);
	    TRACE("WHERE_WINDOW_MAX %s\n", wine_dbgstr_rect(&rc));
	} else {
	    GetWindowRect(wma->hWndPaint, &rc);
	    TRACE("WHERE_WINDOW %s\n", wine_dbgstr_rect(&rc));
	}
    }

    /* In MCI, RECT structure is used differently: rc.right = width & rc.bottom = height
     * So convert the normal RECT into a MCI RECT before returning */
    lpParms->rc.left = rc.left;
    lpParms->rc.top = rc.top;
    lpParms->rc.right = rc.right - rc.left;
    lpParms->rc.bottom = rc.bottom - rc.top;

    LeaveCriticalSection(&wma->cs);
    return 0;
}

/***************************************************************************
 * 				MCIAVI_mciWindow			[internal]
 */
DWORD	MCIAVI_mciWindow(UINT wDevID, DWORD dwFlags, LPMCI_DGV_WINDOW_PARMSW lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_DGV_WINDOW_HWND) {
        if (IsWindow(lpParms->hWnd))
        {
            TRACE("Setting hWnd to %p\n", lpParms->hWnd);
            if (wma->hWnd) ShowWindow(wma->hWnd, SW_HIDE);
            wma->hWndPaint = (lpParms->hWnd == MCI_DGV_WINDOW_DEFAULT) ? wma->hWnd : lpParms->hWnd;
        }
    }
    if (dwFlags & MCI_DGV_WINDOW_STATE) {
	TRACE("Setting nCmdShow to %d\n", lpParms->nCmdShow);
        ShowWindow(wma->hWndPaint, lpParms->nCmdShow);
    }
    if (dwFlags & MCI_DGV_WINDOW_TEXT) {
	TRACE("Setting caption to %s\n", debugstr_w(lpParms->lpstrText));
        SetWindowTextW(wma->hWndPaint, lpParms->lpstrText);
    }

    LeaveCriticalSection(&wma->cs);
    return 0;
}
