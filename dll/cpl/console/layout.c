/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/console/layout.c
 * PURPOSE:         Layout dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>

/* CONSOLE WINDOW PREVIEW Control *********************************************/

#define WIN_PREVIEW_CLASS L"WinPreview"

typedef struct _WINPREV_DATA
{
    HWND hWnd;      // The window which this structure refers to
    RECT rcMaxArea; // Maximum rectangle in which the preview window can be sized
    SIZE siPreview; // Actual size of the preview window
    SIZE siVirtScr; // Width and Height of the virtual screen
    PVOID pData;    // Private data
} WINPREV_DATA, *PWINPREV_DATA;

static LRESULT CALLBACK
WinPrevProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL
RegisterWinPrevClass(
    IN HINSTANCE hInstance)
{
    WNDCLASSW WndClass;

    WndClass.lpszClassName = WIN_PREVIEW_CLASS;
    WndClass.lpfnWndProc = WinPrevProc;
    WndClass.style = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = NULL;
    WndClass.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
    WndClass.hbrBackground =  (HBRUSH)(COLOR_BACKGROUND + 1);
    WndClass.lpszMenuName = NULL;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0; // sizeof(PWINPREV_DATA);

    return (RegisterClassW(&WndClass) != 0);
}

BOOL
UnRegisterWinPrevClass(
    IN HINSTANCE hInstance)
{
    return UnregisterClassW(WIN_PREVIEW_CLASS, hInstance);
}

static VOID
WinPrev_OnDisplayChange(
    IN PWINPREV_DATA pData)
{
    // RECT rcNew;

    pData->siVirtScr.cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    pData->siVirtScr.cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    /*
     * The rescaling factor "siPreview / siVirtScr" should be the minimum of the ratios
     *    pData->rcMaxArea.right  / pData->siVirtScr.cx , and
     *    pData->rcMaxArea.bottom / pData->siVirtScr.cy ,
     * or equivalently, the maximum of the inverse of these ratios.
     * This condition is equivalent to the following inequality being tested.
     */
    // if (pData->siVirtScr.cx / pData->rcMaxArea.right >= pData->siVirtScr.cy / pData->rcMaxArea.bottom)
    if (pData->siVirtScr.cx * pData->rcMaxArea.bottom >= pData->siVirtScr.cy * pData->rcMaxArea.right)
    {
        pData->siPreview.cx = MulDiv(pData->siVirtScr.cx, pData->rcMaxArea.right, pData->siVirtScr.cx);
        pData->siPreview.cy = MulDiv(pData->siVirtScr.cy, pData->rcMaxArea.right, pData->siVirtScr.cx);
    }
    else
    {
        pData->siPreview.cx = MulDiv(pData->siVirtScr.cx, pData->rcMaxArea.bottom, pData->siVirtScr.cy);
        pData->siPreview.cy = MulDiv(pData->siVirtScr.cy, pData->rcMaxArea.bottom, pData->siVirtScr.cy);
    }

    /*
     * Now, the lengths in screen-units can be rescaled into preview-units with:
     *    MulDiv(cx, pData->siPreview.cx, pData->siVirtScr.cx);
     * and:
     *    MulDiv(cy, pData->siPreview.cy, pData->siVirtScr.cy);
     */

#if 0 // TODO: Investigate!
    /*
     * Since both rcMaxArea and siPreview are client window area sizes,
     * transform them into window sizes.
     */
    SetRect(&rcNew, 0, 0, pData->siPreview.cx, pData->siPreview.cy);
    AdjustWindowRect(&rcNew,
                     WS_BORDER,
                     // GetWindowLongPtrW(pData->hWnd, GWL_STYLE) & ~WS_OVERLAPPED,
                     FALSE);
    OffsetRect(&rcNew, -rcNew.left, -rcNew.top);
    rcNew.right += 2;
    rcNew.bottom += 2;
#endif

    SetWindowPos(pData->hWnd,
                 0 /* HWND_TOP */,
                 0, 0,
                 pData->siPreview.cx, pData->siPreview.cy,
                 // rcNew.right, rcNew.bottom,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

#define RescaleCX(pData, len)   \
    MulDiv((len), (pData)->siPreview.cx, (pData)->siVirtScr.cx)

#define RescaleCY(pData, len)   \
    MulDiv((len), (pData)->siPreview.cy, (pData)->siVirtScr.cy)

#define RescaleRect(pData, rect)    \
do { \
    (rect).left   = RescaleCX((pData), (rect).left);    \
    (rect).right  = RescaleCX((pData), (rect).right);   \
    (rect).top    = RescaleCY((pData), (rect).top);     \
    (rect).bottom = RescaleCY((pData), (rect).bottom);  \
} while (0)

#if 0
static VOID
WinPrev_OnSize(VOID)
{
}
#endif

static VOID
WinPrev_OnDraw(
    IN HDC hDC,
    IN PWINPREV_DATA pData)
{
    PCONSOLE_STATE_INFO pConInfo = (PCONSOLE_STATE_INFO)pData->pData;
    HBRUSH hBrush;
    RECT rcWin, fRect;
    SIZE /*siBorder,*/ siFrame, siButton, siScroll;
    SIZE resize;

    RECT rcItem;

    GetClientRect(pData->hWnd, &rcItem);

    /*
     * Retrieve some system metrics and rescale them.
     * They will be added separately, so that to always round the sizes up.
     */

    /* Don't care about border as it is almost always 1 and <= frame size */
    /* Example: Frame = 4, or 13 ... while Border = 1 */
    // siBorder.cx = GetSystemMetrics(SM_CXBORDER);
    // siBorder.cy = GetSystemMetrics(SM_CYBORDER);

    /* Window frame size */
    siFrame.cx = GetSystemMetrics(SM_CXFRAME);
    if (siFrame.cx > 0)
    {
        siFrame.cx = RescaleCX(pData, siFrame.cx);
        siFrame.cx = max(1, siFrame.cx);
    }
    siFrame.cy = GetSystemMetrics(SM_CYFRAME);
    if (siFrame.cy > 0)
    {
        siFrame.cy = RescaleCY(pData, siFrame.cy);
        siFrame.cy = max(1, siFrame.cy);
    }

    /* Window caption buttons */
    siButton.cx = GetSystemMetrics(SM_CXSIZE);
    siButton.cx = RescaleCX(pData, siButton.cx);
    siButton.cx = max(1, siButton.cx);

    siButton.cy = GetSystemMetrics(SM_CYSIZE);
    siButton.cy = RescaleCY(pData, siButton.cy);
    siButton.cy = max(1, siButton.cy);

    /* Enlarge them for improving their appearance */
    // siButton.cx *= 2;
    siButton.cy *= 2;

    /* Dimensions of the scrollbars */
    siScroll.cx = GetSystemMetrics(SM_CXVSCROLL);
    siScroll.cx = RescaleCX(pData, siScroll.cx);
    siScroll.cx = max(1, siScroll.cx);

    siScroll.cy = GetSystemMetrics(SM_CYHSCROLL);
    siScroll.cy = RescaleCY(pData, siScroll.cy);
    siScroll.cy = max(1, siScroll.cy);


    // FIXME: Use SM_CXMIN, SM_CYMIN ??


    /*
     * Compute the console window layout
     */

    if (FontPreview.hFont == NULL)
        RefreshFontPreview(&FontPreview, pConInfo);

    /* We start with the console client area, rescaled for the preview */
    SetRect(&rcWin, 0, 0,
            pConInfo->WindowSize.X * FontPreview.CharWidth,
            pConInfo->WindowSize.Y * FontPreview.CharHeight);
    RescaleRect(pData, rcWin);

    /* Add the scrollbars if needed (does not account for any frame) */
    if (pConInfo->WindowSize.X < pConInfo->ScreenBufferSize.X)
    {
        /* Horizontal scrollbar */
        rcWin.bottom += siScroll.cy;
        // NOTE: If an additional exterior frame is needed, add +1
    }
    else
    {
        /* No scrollbar */
        siScroll.cy = 0;
    }
    if (pConInfo->WindowSize.Y < pConInfo->ScreenBufferSize.Y)
    {
        /* Vertical scrollbar */
        rcWin.right += siScroll.cx;
        // NOTE: If an additional exterior frame is needed, add +1
    }
    else
    {
        /* No scrollbar */
        siScroll.cx = 0;
    }

    /* Add the title bar, taking into account the frames */
    rcWin.top -= siButton.cy - 1;

    /* If we have a non-zero window frame size, add an interior border and the frame */
    resize.cx = (siFrame.cx > 0 ? 1 + siFrame.cx : 0);
    resize.cy = (siFrame.cy > 0 ? 1 + siFrame.cy : 0);

    /* Add the outer border */
    ++resize.cx, ++resize.cy;

    InflateRect(&rcWin, resize.cx, resize.cy);

    /* Finally, move the window rectangle back to its correct origin */
    OffsetRect(&rcWin, -rcWin.left, -rcWin.top);

    if ( pConInfo->WindowPosition.x == MAXDWORD &&
         pConInfo->WindowPosition.y == MAXDWORD )
    {
        // OffsetRect(&rcWin, (rcItem.right - rcItem.left) / 3, (rcItem.bottom - rcItem.top) / 3);
        OffsetRect(&rcWin, 0, 0);
    }
    else
    {
        OffsetRect(&rcWin,
                   RescaleCX(pData, pConInfo->WindowPosition.x),
                   RescaleCY(pData, pConInfo->WindowPosition.y));
    }


    /*
     * Paint the preview window
     */

    /* Fill the background with desktop colour */
    FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_BACKGROUND));

    /*
     * Draw the exterior frame. Use 'FillRect' instead of 'FrameRect'
     * so that, when we want to draw frames around other elements,
     * we can just instead separate them with space instead of redrawing
     * a frame with 'FrameRect'.
     */
    FillRect(hDC, &rcWin, GetSysColorBrush(COLOR_WINDOWFRAME));
    InflateRect(&rcWin, -1, -1);

    /* Draw the border */
    hBrush = GetSysColorBrush(COLOR_ACTIVEBORDER);
    if (siFrame.cx > 0)
    {
        SetRect(&fRect, rcWin.left, rcWin.top, rcWin.left + siFrame.cx, rcWin.bottom);
        FillRect(hDC, &fRect, hBrush);
        SetRect(&fRect, rcWin.right - siFrame.cx, rcWin.top, rcWin.right, rcWin.bottom);
        FillRect(hDC, &fRect, hBrush);

        InflateRect(&rcWin, -siFrame.cx, 0);
    }
    if (siFrame.cy > 0)
    {
        SetRect(&fRect, rcWin.left, rcWin.top, rcWin.right, rcWin.top + siFrame.cy);
        FillRect(hDC, &fRect, hBrush);
        SetRect(&fRect, rcWin.left, rcWin.bottom - siFrame.cy, rcWin.right, rcWin.bottom);
        FillRect(hDC, &fRect, hBrush);

        InflateRect(&rcWin, 0, -siFrame.cy);
    }

    /* Draw the interior frame if we had a border */
    if (siFrame.cx > 0 || siFrame.cy > 0)
    {
#if 0 // See the remark above
        SetRect(&fRect, rcWin.left, rcWin.top, rcWin.right, rcWin.bottom);
        FrameRect(hDC, &fRect, GetSysColorBrush(COLOR_WINDOWFRAME));
#endif
        InflateRect(&rcWin, (siFrame.cx > 0 ? -1 : 0), (siFrame.cy > 0 ? -1 : 0));
    }

    /* Draw the console window title bar */
    hBrush = GetSysColorBrush(COLOR_BTNFACE);

    /* Draw the system menu (left button) */
    SetRect(&fRect, rcWin.left, rcWin.top, rcWin.left + siButton.cx, rcWin.top + siButton.cy - 2);
    // DrawFrameControl(hDC, &fRect, DFC_CAPTION, DFCS_CAPTIONCLOSE);
    FillRect(hDC, &fRect, hBrush);
    fRect.right++; // Separation

    /* Draw the caption bar */
    SetRect(&fRect, fRect.right, fRect.top, rcWin.right - 2 * (siButton.cx + 1), fRect.bottom);
    FillRect(hDC, &fRect, GetSysColorBrush(COLOR_ACTIVECAPTION));
    fRect.right++; // Separation

    /* Draw the minimize menu (first right button) */
    SetRect(&fRect, fRect.right, fRect.top, fRect.right + siButton.cx, fRect.bottom);
    // DrawFrameControl(hDC, &fRect, DFC_CAPTION, DFCS_CAPTIONMIN);
    FillRect(hDC, &fRect, hBrush);
    fRect.right++; // Separation

    /* Draw the maximize menu (second right button) */
    SetRect(&fRect, fRect.right, fRect.top, fRect.right + siButton.cx, fRect.bottom);
    // DrawFrameControl(hDC, &fRect, DFC_CAPTION, DFCS_CAPTIONMAX);
    FillRect(hDC, &fRect, hBrush);

    rcWin.top += siButton.cy - 1;

    /* Add the scrollbars if needed */
    if (siScroll.cy > 0 || siScroll.cx > 0)
    {
        LONG right, bottom;

        right  = rcWin.right;
        bottom = rcWin.bottom;

        /*
         * If both the horizontal and vertical scrollbars are present,
         * reserve some space for the "dead square" at the bottom right.
         */
        if (siScroll.cy > 0 && siScroll.cx > 0)
        {
            right  -= (1 + siScroll.cx);
            bottom -= (1 + siScroll.cy);
        }

        hBrush = GetSysColorBrush(COLOR_SCROLLBAR);

        /* Horizontal scrollbar */
        if (siScroll.cy > 0)
        {
            SetRect(&fRect, rcWin.left, rcWin.bottom - siScroll.cy, right, rcWin.bottom);
            FillRect(hDC, &fRect, hBrush);
        }

        /* Vertical scrollbar */
        if (siScroll.cx > 0)
        {
            SetRect(&fRect, rcWin.right - siScroll.cx, rcWin.top, rcWin.right, bottom);
            FillRect(hDC, &fRect, hBrush);
        }

        /*
         * If both the horizontal and vertical scrollbars are present,
         * draw the "dead square" at the bottom right.
         */
        if (siScroll.cy > 0 && siScroll.cx > 0)
        {
            SetRect(&fRect, rcWin.right - siScroll.cx, rcWin.bottom - siScroll.cy, rcWin.right, rcWin.bottom);
            FillRect(hDC, &fRect, hBrush);
        }

        // NOTE: If an additional exterior frame is needed, remove +1 for each direction
        rcWin.right  -= siScroll.cx;
        rcWin.bottom -= siScroll.cy;
    }

    /* Draw the console background */
    hBrush = CreateSolidBrush(pConInfo->ColorTable[BkgdAttribFromAttrib(pConInfo->ScreenAttributes)]);
    FillRect(hDC, &rcWin, hBrush);
    DeleteObject(hBrush);
}

static LRESULT CALLBACK
WinPrevProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PWINPREV_DATA pData;

    pData = (PWINPREV_DATA)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (msg)
    {
        case WM_CREATE:
        {
            pData = (PWINPREV_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pData));
            if (!pData)
            {
                /* We failed to allocate our private data, halt the window creation */
                return (LRESULT)-1;
            }
            pData->hWnd  = hWnd;
            pData->pData = ConInfo;
            GetClientRect(pData->hWnd, &pData->rcMaxArea);
            // LPCREATESTRUCT::cx and cy give window (not client) size
            WinPrev_OnDisplayChange(pData);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pData);
            break;
        }

        case WM_DESTROY:
        {
            if (pData)
                HeapFree(GetProcessHeap(), 0, pData);
            break;
        }

        case WM_DISPLAYCHANGE:
        {
            WinPrev_OnDisplayChange(pData);
            UpdateWindow(hWnd);
            // InvalidateRect(hWnd, NULL, FALSE);
            break;
        }

        case WM_SIZE:
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            WinPrev_OnDraw(ps.hdc, pData);
            EndPaint(hWnd, &ps);
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}


/* CONSOLE TEXT PREVIEW *******************************************************/

const WCHAR szPreviewText[] =
    L"C:\\ReactOS> dir                       \n" \
    L"SYSTEM       <DIR>      13-04-15  5:00a\n" \
    L"SYSTEM32     <DIR>      13-04-15  5:00a\n" \
    L"readme   txt       1739 13-04-15  5:00a\n" \
    L"explorer exe    3329536 13-04-15  5:00a\n" \
    L"vgafonts cab      18736 13-04-15  5:00a\n" \
    L"setuplog txt        313 13-04-15  5:00a\n" \
    L"win      ini       7005 13-04-15  5:00a\n" ;

VOID
PaintText(
    IN LPDRAWITEMSTRUCT drawItem,
    IN PCONSOLE_STATE_INFO pConInfo,
    IN TEXT_TYPE TextMode)
{
    USHORT CurrentAttrib;
    COLORREF pbkColor, ptColor;
    COLORREF nbkColor, ntColor;
    HBRUSH hBrush;
    HFONT hOldFont;

    if (TextMode == Screen)
        CurrentAttrib = pConInfo->ScreenAttributes;
    else if (TextMode == Popup)
        CurrentAttrib = pConInfo->PopupAttributes;
    else
        return;

    nbkColor = pConInfo->ColorTable[BkgdAttribFromAttrib(CurrentAttrib)];
    ntColor  = pConInfo->ColorTable[TextAttribFromAttrib(CurrentAttrib)];

    hBrush = CreateSolidBrush(nbkColor);
    if (!hBrush) return;

    if (FontPreview.hFont == NULL)
        RefreshFontPreview(&FontPreview, pConInfo);

    hOldFont = SelectObject(drawItem->hDC, FontPreview.hFont);
    //if (hOldFont == NULL)
    //{
    //    DeleteObject(hBrush);
    //    return;
    //}

    FillRect(drawItem->hDC, &drawItem->rcItem, hBrush);

    /* Add a few space between the preview window border and the text sample */
    InflateRect(&drawItem->rcItem, -2, -2);

    ptColor  = SetTextColor(drawItem->hDC, ntColor);
    pbkColor = SetBkColor(drawItem->hDC, nbkColor);
    DrawTextW(drawItem->hDC, szPreviewText, (INT)wcslen(szPreviewText), &drawItem->rcItem, 0);
    SetTextColor(drawItem->hDC, ptColor);
    SetBkColor(drawItem->hDC, pbkColor);

    SelectObject(drawItem->hDC, hOldFont);
    DeleteObject(hBrush);
}


/* LAYOUT DIALOG **************************************************************/

INT_PTR
CALLBACK
LayoutProc(HWND hDlg,
           UINT uMsg,
           WPARAM wParam,
           LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Multi-monitor support */
            LONG  xVirtScr,  yVirtScr; // Coordinates of the top-left virtual screen
            LONG cxVirtScr, cyVirtScr; // Width and Height of the virtual screen
            LONG cxFrame  , cyFrame  ; // Thickness of the window frame

            xVirtScr  = GetSystemMetrics(SM_XVIRTUALSCREEN);
            yVirtScr  = GetSystemMetrics(SM_YVIRTUALSCREEN);
            cxVirtScr = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            cyVirtScr = GetSystemMetrics(SM_CYVIRTUALSCREEN);
            cxFrame   = GetSystemMetrics(SM_CXFRAME);
            cyFrame   = GetSystemMetrics(SM_CYFRAME);

            SendDlgItemMessageW(hDlg, IDC_UPDOWN_SCREEN_BUFFER_HEIGHT, UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));
            SendDlgItemMessageW(hDlg, IDC_UPDOWN_SCREEN_BUFFER_WIDTH , UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));
            SendDlgItemMessageW(hDlg, IDC_UPDOWN_WINDOW_SIZE_HEIGHT, UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));
            SendDlgItemMessageW(hDlg, IDC_UPDOWN_WINDOW_SIZE_WIDTH , UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));

            SetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, ConInfo->ScreenBufferSize.Y, FALSE);
            SetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH , ConInfo->ScreenBufferSize.X, FALSE);
            SetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, ConInfo->WindowSize.Y, FALSE);
            SetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_WIDTH , ConInfo->WindowSize.X, FALSE);

            SendDlgItemMessageW(hDlg, IDC_UPDOWN_WINDOW_POS_LEFT, UDM_SETRANGE, 0,
                                (LPARAM)MAKELONG(xVirtScr + cxVirtScr - cxFrame, xVirtScr - cxFrame));
            SendDlgItemMessageW(hDlg, IDC_UPDOWN_WINDOW_POS_TOP , UDM_SETRANGE, 0,
                                (LPARAM)MAKELONG(yVirtScr + cyVirtScr - cyFrame, yVirtScr - cyFrame));

            SetDlgItemInt(hDlg, IDC_EDIT_WINDOW_POS_LEFT, ConInfo->WindowPosition.x, TRUE);
            SetDlgItemInt(hDlg, IDC_EDIT_WINDOW_POS_TOP , ConInfo->WindowPosition.y, TRUE);

            if (ConInfo->AutoPosition)
            {
                EnableDlgItem(hDlg, IDC_EDIT_WINDOW_POS_LEFT, FALSE);
                EnableDlgItem(hDlg, IDC_EDIT_WINDOW_POS_TOP , FALSE);
                EnableDlgItem(hDlg, IDC_UPDOWN_WINDOW_POS_LEFT, FALSE);
                EnableDlgItem(hDlg, IDC_UPDOWN_WINDOW_POS_TOP , FALSE);
            }
            CheckDlgButton(hDlg, IDC_CHECK_SYSTEM_POS_WINDOW,
                           ConInfo->AutoPosition ? BST_CHECKED : BST_UNCHECKED);

            return TRUE;
        }

        case WM_DISPLAYCHANGE:
        {
            /* Retransmit to the preview window */
            SendDlgItemMessageW(hDlg, IDC_STATIC_LAYOUT_WINDOW_PREVIEW,
                                WM_DISPLAYCHANGE, wParam, lParam);
            break;
        }

        case WM_NOTIFY:
        {
            LPPSHNOTIFY lppsn = (LPPSHNOTIFY)lParam;

            if (lppsn->hdr.code == UDN_DELTAPOS)
            {
                LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;
                DWORD wheight, wwidth;
                DWORD sheight, swidth;
                DWORD left, top;

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_WIDTH)
                {
                    wwidth = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    wwidth = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_WIDTH, NULL, FALSE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_HEIGHT)
                {
                    wheight = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    wheight = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, NULL, FALSE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_WIDTH)
                {
                    swidth = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    swidth = GetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, NULL, FALSE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_HEIGHT)
                {
                    sheight = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    sheight = GetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, NULL, FALSE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_POS_LEFT)
                {
                    left = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    left = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_POS_LEFT, NULL, TRUE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_POS_TOP)
                {
                    top = lpnmud->iPos + lpnmud->iDelta;
                }
                else
                {
                    top = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_POS_TOP, NULL, TRUE);
                }

                if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_WIDTH || lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_HEIGHT)
                {
                    /* Automatically adjust screen buffer size when window size enlarges */
                    if (wwidth >= swidth)
                    {
                        SetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, wwidth, TRUE);
                        swidth = wwidth;
                    }
                    if (wheight >= sheight)
                    {
                        SetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, wheight, TRUE);
                        sheight = wheight;
                    }
                }

                /* Be sure that the (new) screen buffer sizes are in the correct range */
                swidth  = min(max(swidth , 1), 0xFFFF);
                sheight = min(max(sheight, 1), 0xFFFF);

                if (lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_WIDTH || lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_HEIGHT)
                {
                    /* Automatically adjust window size when screen buffer decreases */
                    if (wwidth > swidth)
                    {
                        SetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_WIDTH, swidth, TRUE);
                        wwidth = swidth;
                    }
                    if (wheight > sheight)
                    {
                        SetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, sheight, TRUE);
                        wheight = sheight;
                    }
                }

                ConInfo->ScreenBufferSize.X = (SHORT)swidth;
                ConInfo->ScreenBufferSize.Y = (SHORT)sheight;
                ConInfo->WindowSize.X = (SHORT)wwidth;
                ConInfo->WindowSize.Y = (SHORT)wheight;
                ConInfo->WindowPosition.x = left;
                ConInfo->WindowPosition.y = top;

                InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LAYOUT_WINDOW_PREVIEW), NULL, TRUE);
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }
            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                switch (LOWORD(wParam))
                {
                case IDC_EDIT_SCREEN_BUFFER_WIDTH:
                {
                    DWORD swidth, wwidth;

                    swidth = GetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, NULL, FALSE);
                    wwidth = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_WIDTH  , NULL, FALSE);

                    /* Be sure that the (new) screen buffer width is in the correct range */
                    swidth = min(max(swidth, 1), 0xFFFF);

                    /* Automatically adjust window size when screen buffer decreases */
                    if (wwidth > swidth)
                    {
                        wwidth = swidth;
                        SetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_WIDTH, wwidth, TRUE);
                    }

                    ConInfo->ScreenBufferSize.X = (SHORT)swidth;
                    ConInfo->WindowSize.X       = (SHORT)wwidth;

                    InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LAYOUT_WINDOW_PREVIEW), NULL, TRUE);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }

                case IDC_EDIT_WINDOW_SIZE_WIDTH:
                {
                    DWORD swidth, wwidth;

                    swidth = GetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, NULL, FALSE);
                    wwidth = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_WIDTH  , NULL, FALSE);

                    /* Automatically adjust screen buffer size when window size enlarges */
                    if (wwidth >= swidth)
                    {
                        swidth = wwidth;

                        /* Be sure that the (new) screen buffer width is in the correct range */
                        swidth = min(max(swidth, 1), 0xFFFF);

                        SetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, swidth, TRUE);
                    }

                    ConInfo->ScreenBufferSize.X = (SHORT)swidth;
                    ConInfo->WindowSize.X       = (SHORT)wwidth;

                    InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LAYOUT_WINDOW_PREVIEW), NULL, TRUE);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }

                case IDC_EDIT_SCREEN_BUFFER_HEIGHT:
                {
                    DWORD sheight, wheight;

                    sheight = GetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, NULL, FALSE);
                    wheight = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT  , NULL, FALSE);

                    /* Be sure that the (new) screen buffer width is in the correct range */
                    sheight = min(max(sheight, 1), 0xFFFF);

                    /* Automatically adjust window size when screen buffer decreases */
                    if (wheight > sheight)
                    {
                        wheight = sheight;
                        SetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, wheight, TRUE);
                    }

                    ConInfo->ScreenBufferSize.Y = (SHORT)sheight;
                    ConInfo->WindowSize.Y       = (SHORT)wheight;

                    InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LAYOUT_WINDOW_PREVIEW), NULL, TRUE);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }

                case IDC_EDIT_WINDOW_SIZE_HEIGHT:
                {
                    DWORD sheight, wheight;

                    sheight = GetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, NULL, FALSE);
                    wheight = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT  , NULL, FALSE);

                    /* Automatically adjust screen buffer size when window size enlarges */
                    if (wheight >= sheight)
                    {
                        sheight = wheight;

                        /* Be sure that the (new) screen buffer width is in the correct range */
                        sheight = min(max(sheight, 1), 0xFFFF);

                        SetDlgItemInt(hDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, sheight, TRUE);
                    }

                    ConInfo->ScreenBufferSize.Y = (SHORT)sheight;
                    ConInfo->WindowSize.Y       = (SHORT)wheight;

                    InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LAYOUT_WINDOW_PREVIEW), NULL, TRUE);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }

                case IDC_EDIT_WINDOW_POS_LEFT:
                case IDC_EDIT_WINDOW_POS_TOP:
                {
                    ConInfo->WindowPosition.x = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_POS_LEFT, NULL, TRUE);
                    ConInfo->WindowPosition.y = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_POS_TOP , NULL, TRUE);

                    InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LAYOUT_WINDOW_PREVIEW), NULL, TRUE);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                }
            }
            else
            if (HIWORD(wParam) == BN_CLICKED &&
                LOWORD(wParam) == IDC_CHECK_SYSTEM_POS_WINDOW)
            {
                if (IsDlgButtonChecked(hDlg, IDC_CHECK_SYSTEM_POS_WINDOW) == BST_CHECKED)
                {
                    EnableDlgItem(hDlg, IDC_EDIT_WINDOW_POS_LEFT, FALSE);
                    EnableDlgItem(hDlg, IDC_EDIT_WINDOW_POS_TOP , FALSE);
                    EnableDlgItem(hDlg, IDC_UPDOWN_WINDOW_POS_LEFT, FALSE);
                    EnableDlgItem(hDlg, IDC_UPDOWN_WINDOW_POS_TOP , FALSE);

                    ConInfo->AutoPosition = TRUE;
                    // Do not touch ConInfo->WindowPosition !!

                    InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LAYOUT_WINDOW_PREVIEW), NULL, TRUE);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }
                else
                {
                    ULONG left, top;

                    left = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_POS_LEFT, NULL, TRUE);
                    top  = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_POS_TOP , NULL, TRUE);

                    EnableDlgItem(hDlg, IDC_EDIT_WINDOW_POS_LEFT, TRUE);
                    EnableDlgItem(hDlg, IDC_EDIT_WINDOW_POS_TOP , TRUE);
                    EnableDlgItem(hDlg, IDC_UPDOWN_WINDOW_POS_LEFT, TRUE);
                    EnableDlgItem(hDlg, IDC_UPDOWN_WINDOW_POS_TOP , TRUE);

                    ConInfo->AutoPosition     = FALSE;
                    ConInfo->WindowPosition.x = left;
                    ConInfo->WindowPosition.y = top;

                    InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_LAYOUT_WINDOW_PREVIEW), NULL, TRUE);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                }
            }

            break;
        }

        default:
            break;
    }

    return FALSE;
}
