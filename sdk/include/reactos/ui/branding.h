/*
 * PROJECT:     ReactOS UI Headers Library
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Branding support for dialog boxes.
 * COPYRIGHT:   Copyright 2006 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2019 Stanislav Motylkov <x86corez@gmail.com>
 *              Copyright 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *              Copyright 2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/**
 * @file    branding.h
 * @ingroup RosBrand
 *
 * @defgroup RosBrand
 * ReactOS UI Headers Library - Branding
 *
 * This header library defines structures and methods used to display
 * a standardized branding banner in windows and dialog boxes.
 *
 * The general layout of the branding in a window/dialog is as follows:
 * \verbatim
 * +-----------------------------------------------------------------+ \
 * |  .......  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  .......  | |
 * |  .Left .  ~~~~~~~~~~~~  B R A N D I N G  ~~~~~~~~~~~~  .Right.  | |  Custom
 * |  .Pad...  ~~~~~~~~~~~~~~    L O G O    ~~~~~~~~~~~~~~  ...Pad.  | | Branding
 * |  .......  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  .......  | |
 * |××××××||¤¤¤¤¤¤¤¤¤¤ B r a n d i n g   P r o g r e s s   B a r ××××| /
 * |                                                                 |
 * |  User     [x]             { Win32|       }                      |
 * |  Controls [¤]                                                   |
 * |                                            [  OK  ] [ Cancel ]  |
 * +-----------------------------------------------------------------+
 * \endverbatim
 *
 * There are two customizable bitmap elements present:
 * the branding ("Logo") and the (optionally) rotating progress bar ("Bar").
 * They are placed at the top of the window, one below the other.
 * When branding is applied programmatically to a window, all the window
 * children controls are automatically moved down to make space for it.
 *
 * \li "Logo":
 * The logo is placed at the top, either left-aligned (DT_LEFT), centered
 * (DT_CENTER), or right-aligned (DT_RIGHT). If the window is larger than
 * the logo, then left- or right-padding is added. The filling color used
 * for the left (resp. right) padding is determined from the top-left (resp.
 * top-right) pixel color of the branding logo. If the window is smaller than
 * the logo, the latter is left-aligned and shown truncated on the right.
 *
 * \li "Bar":
 * The optional bar is shown just below the logo. By default, it is always
 * stretched to accomodate for the window's width. A compile-time define
 * _TO_BE_DETERMINED_ can be specified to choose instead to stretch the bar
 * _only if_ the window's width is larger than the bar's, and otherwise
 * display only the central region of the bar unstretched (truncating its
 * left and right sides).
 * The bar can be made to optionally "rotate", by specifying an offset value
 * between 0 and the bar's width.
 **/

#ifndef __BRANDING_H__
#define __BRANDING_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <wingdi.h>

/**
 * @brief
 * Branding info structure used to intialize branding support with
 * the Brand_LoadBitmapsEx() function.
 **/
typedef struct _BRAND_INFO
{
    HINSTANCE hInstance;    ///< Module instance handle where to load the bitmap resources from.
    HANDLE hLogoBitmap;     ///< Handle to loaded logo bitmap, or resource ID (with MAKEINTRESOURCEW).
    HANDLE hBarBitmap;      ///< Handle to loaded bar bitmap, or resource ID (with MAKEINTRESOURCEW).
    UINT uDtAlign;          ///< Branding alignment using DT_* DrawText() flags (DT_LEFT, DT_CENTER, DT_RIGHT).
} BRAND_INFO, *PBRAND_INFO;

/**
 * @brief
 * Branding data context structure.
 **/
typedef struct _BRAND
{
    HBITMAP hLogoBitmap;
    HBITMAP hBarBitmap;
    HBRUSH hbrFill[2];
    SIZE LogoSize;
    SIZE BarSize;
    RECT rcClient;
    UINT uFlags;
} BRAND, *PBRAND;

/**
 * Forward declarations
 **/

/**
 * @brief
 * Loads branding bitmap resources and initialize a branding context.
 *
 * @param[in,out]   pBrand
 * Branding data context to be initialized.
 *
 * @param[in]   hInstance
 * Optional module instance handle where to load the bitmap resources from.
 *
 * @param[in]   uIDLogo
 * Resource ID for the logo bitmap.
 *
 * @param[in]   uIDBar
 * Resource ID for the bar bitmap.
 *
 * @param[in]   uDtAlign
 * Branding alignment using DT_* DrawText() flags (\b DT_LEFT, \b DT_CENTER, \b DT_RIGHT).
 *
 * @return  None.
 *
 * @remark
 * This is a convenience function for the Brand_LoadBitmapsEx() one, the latter
 * taking instead a BRAND_INFO structure for initializing the branding context.
 **/
VOID
Brand_LoadBitmaps(
    _Inout_ PBRAND pBrand,
    _In_opt_ HINSTANCE hInstance,
    _In_ WORD uIDLogo,
    _In_ WORD uIDBar,
    _In_ UINT uDtAlign);

/**
 * @brief
 * Loads branding bitmap resources and initialize a branding context.
 *
 * @param[in,out]   pBrand
 * Branding data context to be initialized.
 *
 * @param[in]       pBrandInfo
 * A user-defined BRAND_INFO structure that specifies the branding bitmaps
 * to be loaded and the alignment to be used.
 *
 * @return  None.
 *
 * @remark
 * This is the compact form equivalent to the Brand_LoadBitmaps() function.
 **/
VOID
Brand_LoadBitmapsEx(
    _Inout_ PBRAND pBrand,
    _In_ PBRAND_INFO pBrandInfo);

/**
 * @brief
 * Cleans up and frees resources associated to a branding context.
 *
 * @param[in,out]   pBrand
 * Branding data context to be cleaned up.
 *
 * @return  None.
 **/
VOID
Brand_Cleanup(
    _Inout_ PBRAND pBrand);

/**
 * @brief
 * Brands and repositions children controls of a window.
 *
 * @param[in]   hWnd
 * Handle to a window to be branded and its children controls to be
 * repositioned accordingly.
 *
 * @param[in]   pBrand
 * Branding data context.
 *
 * @return  None.
 **/
VOID
Brand_MoveControls(
    _In_ HWND hWnd,
    _In_ PBRAND pBrand);

/* See windowsx.h */
/**
 * @brief   SendMessage macro.
 **/
#ifndef SNDMSG
#ifdef __cplusplus
#define SNDMSG  ::SendMessage
#else
#define SNDMSG  SendMessage
#endif
#endif /* !SNDMSG */

/**
 * @brief   WM_CTLCOLORDLG forwarding macro.
 **/
#ifndef FORWARD_WM_CTLCOLORDLG
#define FORWARD_WM_CTLCOLORDLG(hwnd, hdc, hwndChild, fn) \
    (HBRUSH)(UINT_PTR)(fn)((hwnd), WM_CTLCOLORDLG, (WPARAM)(HDC)(hdc), (LPARAM)(HWND)(hwndChild))
#endif

/**
 * @brief   Short-form helper to retrieve the background brush of a dialog
 *          (sends a WM_CTLCOLORDLG message).
 **/
#define GetColorDlg(hDlg, hdc)  FORWARD_WM_CTLCOLORDLG(hDlg, hdc, hDlg, SNDMSG)

/**
 * @brief
 * Paints branding on a window.
 * Depending on the user's wishes, this function may be called at different
 * places (e.g. in the WM_PAINT handler, or WM_ERASEBKGND, or WM_TIMER, ...).
 *
 * @param[in]   hWnd
 * Handle to a window to be branded.
 *
 * @param[in]   hDC
 * Optional handle to a device context (DC) for the client area of the
 * specified window. If NULL, the function calls GetDC(hWnd) and releases
 * the obtained DC after usage.
 *
 * @param[in]   pBrand
 * Branding data context.
 *
 * @param[in]   bBarAnim
 * Whether to only draw the progress bar, instead of the full logo and bar
 * (useful for bar animation).
 *
 * @param[in]   uBarOffset
 * Drawing progress bar offset, between 0 and the bar's width
 * (useful for bar animation).
 *
 * @param[in]   hbrBkgd
 * Optional brush, or a color value, to be used to paint the default background
 * of the window, and as a fallback brush for the left and right logo padding.
 * If a color value is specified, it must be one of the standard system colors
 * (and the value 1 must be added to it).
 *
 * @return  None.
 **/
VOID
Brand_Paint(
    _In_ HWND hWnd,
    _In_opt_ HDC hDC,
    _In_ PBRAND pBrand,
    _In_ BOOL bBarAnim,
    _In_ UINT uBarOffset,
    _In_opt_ HBRUSH hbrBkgd);

/**
 * @brief
 * Convenience function for filling a rectangle by using the specified brush.
 *
 * @param[in]   hDC
 * Handle to the device context.
 *
 * @param[in]   lprc
 * Pointer to a RECT structure that contains the logical coordinates of the
 * rectangle to be filled.
 *
 * @param[in]   hbr
 * Handle to the brush used to fill the rectangle, or a color value.
 * If a color value is specified, it must be one of the standard system colors
 * (and the value 1 must be added to it).
 *
 * @return
 * A non-zero value if success, or zero in case of failure.
 *
 * @remark
 * This function is a wrapper around the Win32 FillRect() function.
 *
 * @see <a href="https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-fillrect">FillRect() (on MSDN)</a>
 **/
INT
FillRectEx(
    _In_ HDC hDC,
    _In_ CONST RECT *lprc,
    _In_ HBRUSH hbr);


/**
 * Library implementation.
 **/
#if defined(BRANDING_LIB_IMPL) || !defined(BRANDING_LIB)

VOID
Brand_LoadBitmaps(
    _Inout_ PBRAND pBrand,
    _In_opt_ HINSTANCE hInstance,
    _In_ WORD uIDLogo,
    _In_ WORD uIDBar,
    _In_ UINT uDtAlign)
{
    BRAND_INFO bi = {hInstance,
                     MAKEINTRESOURCEW(uIDLogo),
                     MAKEINTRESOURCEW(uIDBar),
                     uDtAlign};
    Brand_LoadBitmapsEx(pBrand, &bi);
}

VOID
Brand_LoadBitmapsEx(
    _Inout_ PBRAND pBrand,
    _In_ PBRAND_INFO pBrandInfo)
{
    BITMAP bm;

    ZeroMemory(pBrand, sizeof(*pBrand));

    /* Save the branding band alignment */
    pBrand->uFlags |= pBrandInfo->uDtAlign;

    /* Logo band */
    // GetObjectType(pBrandInfo->hLogoBitmap) != OBJ_BITMAP;
    if (IS_INTRESOURCE(pBrandInfo->hLogoBitmap))
    {
        pBrand->hLogoBitmap = LoadImageW(pBrandInfo->hInstance,
                                         (LPCWSTR)pBrandInfo->hLogoBitmap, IMAGE_BITMAP,
                                         0, 0, LR_DEFAULTCOLOR);
    }
    else
    {
        // pBrand->hLogoBitmap = CopyImage(pBrandInfo->hLogoBitmap, IMAGE_BITMAP, 0, 0, 0);
        pBrand->hLogoBitmap = pBrandInfo->hLogoBitmap;
    }
    if (pBrand->hLogoBitmap)
    {
        GetObjectW(pBrand->hLogoBitmap, sizeof(bm), &bm);
        pBrand->LogoSize.cx = bm.bmWidth;
        pBrand->LogoSize.cy = bm.bmHeight;
    }

    /* If the logo failed to be created, do not bother creating the bar */
    if (!pBrand->hLogoBitmap)
        return;

    /* Bar */
    // GetObjectType(pBrandInfo->hBarBitmap) != OBJ_BITMAP;
    if (IS_INTRESOURCE(pBrandInfo->hBarBitmap))
    {
        pBrand->hBarBitmap = LoadImageW(pBrandInfo->hInstance,
                                        (LPCWSTR)pBrandInfo->hBarBitmap, IMAGE_BITMAP,
                                        0, 0, LR_DEFAULTCOLOR);
    }
    else
    {
        // pBrand->hBarBitmap = CopyImage(pBrandInfo->hBarBitmap, IMAGE_BITMAP, 0, 0, 0);
        pBrand->hBarBitmap = pBrandInfo->hBarBitmap;
    }
    if (pBrand->hBarBitmap)
    {
        GetObjectW(pBrand->hBarBitmap, sizeof(bm), &bm);
        pBrand->BarSize.cx = bm.bmWidth;
        pBrand->BarSize.cy = bm.bmHeight;
    }

    /* Create the left and right brushes used to fill around the logo band */
    if (pBrand->hLogoBitmap)
    {
        HDC hDC = CreateCompatibleDC(NULL);
        if (hDC)
        {
            if (SelectObject(hDC, pBrand->hLogoBitmap))
            {
                COLORREF clr;

                /* Left-brush: use the bitmap's top-left pixel color */
                clr = GetPixel(hDC, 0, 0);
                if (clr != CLR_INVALID)
                    pBrand->hbrFill[0] = CreateSolidBrush(clr);

                /* Right-brush: use the bitmap's top-right pixel color */
                clr = GetPixel(hDC, pBrand->LogoSize.cx-1, 0);
                if (clr != CLR_INVALID)
                    pBrand->hbrFill[1] = CreateSolidBrush(clr);
            }
            DeleteDC(hDC);
        }
    }
    /* Fall back to one brush if the other one failed to be initialized */
    if (pBrand->hbrFill[0] && !pBrand->hbrFill[1])
        pBrand->hbrFill[1] = pBrand->hbrFill[0];
    else if (!pBrand->hbrFill[0] && pBrand->hbrFill[1])
        pBrand->hbrFill[0] = pBrand->hbrFill[1];
    /* If both are NULL, we will optionally use a default brush during painting */
}

VOID
Brand_Cleanup(
    _Inout_ PBRAND pBrand)
{
    if (pBrand->hbrFill[1] && (pBrand->hbrFill[1] != pBrand->hbrFill[0]))
        DeleteObject(pBrand->hbrFill[1]);

    if (pBrand->hbrFill[0])
        DeleteObject(pBrand->hbrFill[0]);

    if (pBrand->hBarBitmap)
        DeleteObject(pBrand->hBarBitmap);

    if (pBrand->hLogoBitmap)
        DeleteObject(pBrand->hLogoBitmap);

    ZeroMemory(pBrand, sizeof(*pBrand));
}

// LayoutWindowPos
static
_Ret_maybenull_
HDWP
DoWindowPos(
    _In_opt_ HDWP hdwp, // hWinPosInfo,
    _In_ HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_ int x,
    _In_ int y,
    _In_ int cx,
    _In_ int cy,
    _In_ UINT uFlags)
{
    /*
     * Now position the window. If DeferWindowPos fails,
     * move and resize the window manually below.
     */
    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hWnd,
                              hWndInsertAfter,
                              x, y,
                              cx, cy,
                              uFlags);
    }

    /*
     * If the DeferWindowPos structure was not set up, or if we just
     * tore it down above by adding more controls than it could deal
     * with, move and resize manually here.
     */
    if (!hdwp)
    {
        SetWindowPos(hWnd,
                     hWndInsertAfter,
                     x, y,
                     cx, cy,
                     uFlags);
    }

    return hdwp;
}

static
_Ret_maybenull_
HDWP
AdjustWindowSize(
    _In_opt_ HDWP hdwp,
    _In_ HWND hWnd,
    _In_ INT cxDelta,
    _In_ INT cyDelta)
{
    RECT rc;
    POINT pt;
    SIZE size;

    /* If no deltas then no need to resize */
    if (cxDelta == 0 && cyDelta == 0)
        return hdwp;

    /* Get the current window position and size */
    GetWindowRect(hWnd, &rc);
    pt.x = rc.left;
    pt.y = rc.top;
    size.cx = (rc.right - rc.left) + cxDelta;
    size.cy = (rc.bottom - rc.top) + cyDelta;

    /* Center the newly-resized window if necessary,
     * otherwise keep the same origin. */
    if (GetWindowLongPtrW(hWnd, GWL_STYLE) & DS_CENTER)
    {
        pt.x -= cxDelta / 2;
        pt.y -= cyDelta / 2;
    }
    /* Move the window back into the screen if necessary */
    pt.x = max(pt.x, 0);
    pt.y = max(pt.y, 0);

    return DoWindowPos(hdwp, hWnd, HWND_TOP, pt.x, pt.y, size.cx, size.cy,
                       SWP_NOZORDER | SWP_NOACTIVATE /* | SWP_NOREDRAW */);
}

VOID
Brand_MoveControls(
    _In_ HWND hWnd,
    _In_ PBRAND pBrand)
{
    UINT nChildren;
    HDWP hdwp;
    HWND hwndChild;
    SIZE sDelta;
    RECT rc;

    /* If the logo failed to be created, do not bother moving anything */
    if (!pBrand->hLogoBitmap)
        return;

    /* Retrieve the total height of logo + bar */
    sDelta.cx = sDelta.cy = 0;
    sDelta.cy += pBrand->LogoSize.cy;
    if (pBrand->hBarBitmap)
        sDelta.cy += pBrand->BarSize.cy;

    /*
     * Resize and reposition the parent window accordingly.
     * Note that this cannot be done within the Begin/EndDeferWindowPos()
     * loop, since only windows belonging to the *same* parent
     * can be moved with these functions.
     */
    AdjustWindowSize(NULL, hWnd, sDelta.cx, sDelta.cy);

    /* Count the number of children in the parent window */
    nChildren = 0;
    for (hwndChild = GetWindow(hWnd, GW_CHILD);
         hwndChild;
         hwndChild = GetNextWindow(hwndChild, GW_HWNDNEXT))
    {
        ++nChildren;
    }

    /* Move each child window */
    hdwp = BeginDeferWindowPos(nChildren);

    for (hwndChild = GetWindow(hWnd, GW_CHILD);
         hwndChild;
         hwndChild = GetNextWindow(hwndChild, GW_HWNDNEXT))
    {
        GetWindowRect(hwndChild, &rc);
        /* NOTE: MapWindowRect() in windowsx.h */
        MapWindowPoints(HWND_DESKTOP /*NULL*/, hWnd, (LPPOINT)&rc, sizeof(RECT)/sizeof(POINT));

        hdwp = DoWindowPos(hdwp,
                           hwndChild,
                           HWND_TOP,
                           rc.left, sDelta.cy + rc.top,
                           0, 0,
                           SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (hdwp)
        EndDeferWindowPos(hdwp);

    /* Retrieve/Update the window client size */
    GetClientRect(hWnd, &pBrand->rcClient);
}


/**
 * OLD_WAY:
 * The "old" way that would work when non-stretched, but that looses precision when stretching.
 * (New way is to do the two-bar technique and using the fact that the DC is clipped. À la MSGINA.)
 *
 * _TO_BE_DETERMINED_:
 * Stretch the bar only when its size is < window size, otherwise truncate it (à la MSGINA).
 * Otherwise, always stretch the bar, making it smaller if needed.
 *
 * R = wndWidth
 * uB = uBarOffset
 *
 * <--------- Window width --------->
 * 0       uB                       R
 * ║×× 1 ××||¤¤¤¤¤¤¤¤¤¤¤ 2 ×××××××××║
 *
 *
 * <-- Clipped Out (CO) --><--------- Window width ---------><--CO-->
 *                         0       uB                       R
 * |¤¤¤¤¤¤¤¤¤¤¤ 2 ×××××××××║×× 1 ××||¤¤¤¤¤¤¤¤¤¤¤ 2 ×××××××××║×× 1 ××|
 *
 **/

//#define OLD_WAY
//#define _TO_BE_DETERMINED_

static VOID
Brand_AnimateBar(
    _In_ CONST RECT *prcWnd,
    _In_ HDC hDC,
    _In_ HDC hMemDC,
    _In_ PBRAND pBrand,
    _In_ UINT uBarOffset)
{
    HGDIOBJ hObj;
    RECT rcWnd = *prcWnd;
    POINT ptSrc, ptDst;
    SIZE sSrc, sDst;
    LONG wndWidth;
//#ifdef OLD_WAY
    UINT uBarOffsetResc;
//#endif
    INT iBarWidth;

    if (!pBrand->hBarBitmap || (pBrand->BarSize.cx == 0))
        return;

    hObj = SelectObject(hMemDC, pBrand->hBarBitmap);
    if (!hObj)
        return;

    wndWidth = rcWnd.right - rcWnd.left;

    /* Clamp the bar offset and rescale it to the destination size */
    uBarOffset %= pBrand->BarSize.cx;
//#ifdef OLD_WAY
    uBarOffsetResc = uBarOffset * wndWidth / pBrand->BarSize.cx;
//#else
    //uBarOffset = uBarOffset * wndWidth / pBrand->BarSize.cx;
//#endif

    iBarWidth = pBrand->BarSize.cx;
#ifdef _TO_BE_DETERMINED_
    /* Either use the bar width, or the window width instead in case the bar is too large */
    if (iBarWidth > wndWidth)
        iBarWidth = wndWidth;

    uBarOffset = uBarOffset * iBarWidth / pBrand->BarSize.cx;

    ptSrc.x = (pBrand->BarSize.cx - iBarWidth) / 2;
#else
    ptSrc.x = 0;
#endif

    // ptSrc.y = 0;

    /* Draw the bar control just beneath the logo */
    ptDst.y = 0 + pBrand->LogoSize.cy;

    /* BitBlt(hDC, off, 0, BarWidth - off, BarHeight, hdcMem, 0, 0, SRCCOPY); */
#ifdef OLD_WAY
    ptDst.x = uBarOffsetResc;
    sDst.cx = wndWidth - uBarOffsetResc;
    sSrc.cx = iBarWidth - uBarOffset;
#else // OLD_WAY
    ptDst.x = uBarOffsetResc; // uBarOffset;
    sDst.cx = wndWidth;
    sSrc.cx = iBarWidth;
#endif // OLD_WAY

    StretchBlt(hDC,
               ptDst.x,
               ptDst.y,
               sDst.cx,
               pBrand->BarSize.cy,
               hMemDC,
               ptSrc.x,
               0,
               sSrc.cx,
               pBrand->BarSize.cy,
               SRCCOPY);

    if (uBarOffset)
    {
        /* BitBlt(hDC, 0, 0, off, BarHeight, hdcMem, BarWidth - off, 0, SRCCOPY); */
#ifdef OLD_WAY
        sDst.cx = ptDst.x; // uBarOffsetResc;
        ptDst.x = 0;
        ptSrc.x += sSrc.cx; // iBarWidth - uBarOffset;
        sSrc.cx = uBarOffset;
#else // OLD_WAY
        ptDst.x = uBarOffsetResc /*uBarOffset*/ - wndWidth;
#endif // OLD_WAY

        StretchBlt(hDC,
                   ptDst.x,
                   ptDst.y,
                   sDst.cx,
                   pBrand->BarSize.cy,
                   hMemDC,
                   ptSrc.x,
                   0,
                   sSrc.cx,
                   pBrand->BarSize.cy,
                   SRCCOPY);
    }

    SelectObject(hMemDC, hObj);
}

VOID
Brand_Paint(
    _In_ HWND hWnd,
    _In_opt_ HDC hDC,
    _In_ PBRAND pBrand,
    _In_ BOOL bBarAnim,
    _In_ UINT uBarOffset,
    _In_opt_ HBRUSH hbrBkgd)
{
    HDC hNewDC = NULL;
    HDC hMemDC;
    HGDIOBJ hObj;
    HBRUSH hBrush;
    RECT rcWnd, rc;

    /* Get a window client DC if we do not have one already */
    if (!hDC)
        hDC = hNewDC = GetDC(hWnd);

    hMemDC = CreateCompatibleDC(hDC);
    if (!hMemDC)
        goto Quit;

    /* Retrieve/Update the window client size */
    if (IsRectEmpty(&pBrand->rcClient))
        GetClientRect(hWnd, &pBrand->rcClient);

    rcWnd = pBrand->rcClient;

    // TODO: Handle RTL layouts.

    if (bBarAnim)
        goto PaintBar; /* Only paint the bar */

    /*
     * Paint the logo band.
     */
#define GET_BRUSH(hbrStore, hbr1, hbr2) {hbrStore = hbr1; if (!hbrStore) hbrStore = hbr2;}

    hObj = SelectObject(hMemDC, pBrand->hLogoBitmap);
    if (!hObj)
        goto Quit;

    /* Initial logo band rectangle */
    SetRect(&rc, 0, 0, 0, pBrand->LogoSize.cy);

    // TODO: If we support transparent logo bands, then do a one-time filling
    // of the rectangle {0, 0, rcWnd.right - rcWnd.left, pBrand->LogoSize.cy}
    // with the pBrand->hbrFill[0] brush, instead of doing it by parts as we
    // do below for non-transparent logos.

    /* Fill the left margin */
    if (pBrand->uFlags & (DT_RIGHT | DT_CENTER))
    {
        rc.right = (rcWnd.right - rcWnd.left) - pBrand->LogoSize.cx;
        if (pBrand->uFlags & DT_CENTER)
            rc.right /= 2;

        /* Ensure the logo's left side is shown if the window is too short */
        rc.right = max(rc.right, 0);

        /* Fill the left margin */
        GET_BRUSH(hBrush, pBrand->hbrFill[0], hbrBkgd);
        if (hBrush)
            FillRect(hDC, &rc, hBrush);
    }

    /* Paint the logo band */
    rc.left = rc.right;
    rc.right += pBrand->LogoSize.cx;
    BitBlt(hDC,
           rc.left, rc.top,
           rc.right - rc.left,  // == pBrand->LogoSize.cx
           pBrand->LogoSize.cy, // == rc.bottom - rc.top
           hMemDC,
           0, 0,
           SRCCOPY);

    /* Fill the right margin */
    if ((pBrand->uFlags & DT_RIGHT) == 0)
    {
        rc.left = rc.right;
        rc.right = rcWnd.right - rcWnd.left;

        GET_BRUSH(hBrush, pBrand->hbrFill[1], hbrBkgd);
        if (hBrush)
            FillRect(hDC, &rc, hBrush);
    }

    SelectObject(hMemDC, hObj);

#undef GET_BRUSH

    /*
     * Paint the bar.
     */
PaintBar:
    Brand_AnimateBar(&rcWnd, hDC, hMemDC, pBrand, uBarOffset);

Quit:
    if (hMemDC)
        DeleteDC(hMemDC);
    if (hNewDC)
        ReleaseDC(hWnd, hNewDC);
}

INT
FillRectEx(
    _In_ HDC hDC,
    _In_ CONST RECT *lprc,
    _In_ HBRUSH hbr)
{
#if 1
#if 0 // Should we need this?
    UnrealizeObject(hbr);
    SetBrushOrgEx(hDC, 0, 0, NULL);
#endif
    return FillRect(hDC, lprc, hbr);
#else
    // Equivalent of FillRect()
    BOOL Ret;
    HGDIOBJ hObj = SelectObject(hDC, hbr);
    Ret = PatBlt(hDC,
                 lprc->left,
                 lprc->top,
                 lprc->right - lprc->left,
                 lprc->bottom - lprc->top,
                 PATCOPY);
    SelectObject(hDC, hObj);
    return (INT)Ret;
#endif
}

#endif // defined(BRANDING_LIB_IMPL) || !defined(BRANDING_LIB)


#ifdef __cplusplus
}
#endif

#endif /* __BRANDING_H__ */

/* EOF */
