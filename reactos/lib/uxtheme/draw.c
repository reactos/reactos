/*
 * Win32 5.1 Theme drawing
 *
 * Copyright (C) 2003 Kevin Koltzau
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "uxtheme.h"
#include "tmschema.h"

#include "msstyles.h"
#include "uxthemedll.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

/***********************************************************************
 * Defines and global variables
 */

extern ATOM atDialogThemeEnabled;

/***********************************************************************/

/***********************************************************************
 *      EnableThemeDialogTexture                            (UXTHEME.@)
 */
HRESULT WINAPI EnableThemeDialogTexture(HWND hwnd, DWORD dwFlags)
{
    static const WCHAR szTab[] = { 'T','a','b',0 };
    HRESULT hr;

    TRACE("(%p,0x%08lx\n", hwnd, dwFlags);
    hr = SetPropW (hwnd, MAKEINTATOMW (atDialogThemeEnabled), 
        (HANDLE)(dwFlags|0x80000000)); 
        /* 0x80000000 serves as a "flags set" flag */
    if (FAILED(hr))
          return hr;
    if (dwFlags & ETDT_USETABTEXTURE)
        return SetWindowTheme (hwnd, NULL, szTab);
    else
        return SetWindowTheme (hwnd, NULL, NULL);
    return S_OK;
 }

/***********************************************************************
 *      IsThemeDialogTextureEnabled                         (UXTHEME.@)
 */
BOOL WINAPI IsThemeDialogTextureEnabled(HWND hwnd)
{
    DWORD dwDialogTextureFlags;
    TRACE("(%p)\n", hwnd);

    dwDialogTextureFlags = (DWORD)GetPropW (hwnd, 
        MAKEINTATOMW (atDialogThemeEnabled));
    if (dwDialogTextureFlags == 0) 
        /* Means EnableThemeDialogTexture wasn't called for this dialog */
        return TRUE;

    return (dwDialogTextureFlags & ETDT_ENABLE) && !(dwDialogTextureFlags & ETDT_DISABLE);
}

/***********************************************************************
 *      DrawThemeParentBackground                           (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeParentBackground(HWND hwnd, HDC hdc, RECT *prc)
{
    RECT rt;
    POINT org;
    HWND hParent;
    HRGN clip = NULL;
    int hasClip = -1;
    
    TRACE("(%p,%p,%p)\n", hwnd, hdc, prc);
    hParent = GetParent(hwnd);
    if(!hParent)
        hParent = hwnd;
    if(prc) {
        CopyRect(&rt, prc);
        MapWindowPoints(hwnd, NULL, (LPPOINT)&rt, 2);
        
        clip = CreateRectRgn(0,0,1,1);
        hasClip = GetClipRgn(hdc, clip);
        if(hasClip == -1)
            TRACE("Failed to get original clipping region\n");
        else
            IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }
    else {
        GetClientRect(hParent, &rt);
        MapWindowPoints(hParent, NULL, (LPPOINT)&rt, 2);
    }

    OffsetViewportOrgEx(hdc, -rt.left, -rt.top, &org);

    SendMessageW(hParent, WM_ERASEBKGND, (WPARAM)hdc, 0);
    SendMessageW(hParent, WM_PRINTCLIENT, (WPARAM)hdc, PRF_CLIENT);

    SetViewportOrgEx(hdc, org.x, org.y, NULL);
    if(prc) {
        if(hasClip == 0)
            SelectClipRgn(hdc, NULL);
        else if(hasClip == 1)
            SelectClipRgn(hdc, clip);
        DeleteObject(clip);
    }
    return S_OK;
}


/***********************************************************************
 *      DrawThemeBackground                                 (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId,
                                   int iStateId, const RECT *pRect,
                                   const RECT *pClipRect)
{
    DTBGOPTS opts;
    opts.dwSize = sizeof(DTBGOPTS);
    opts.dwFlags = 0;
    if(pClipRect) {
        opts.dwFlags |= DTBG_CLIPRECT;
        CopyRect(&opts.rcClip, pClipRect);
    }
    return DrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, &opts);
}

/***********************************************************************
 *      UXTHEME_SelectImage
 *
 * Select the image to use
 */
static PTHEME_PROPERTY UXTHEME_SelectImage(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, BOOL glyph)
{
    PTHEME_PROPERTY tp;
    int imageselecttype = IST_NONE;
    int i;
    int image;
    if(glyph)
        image = TMT_GLYPHIMAGEFILE;
    else
        image = TMT_IMAGEFILE;

    if((tp=MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, image)))
        return tp;
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_IMAGESELECTTYPE, &imageselecttype);

    if(imageselecttype == IST_DPI) {
        int reqdpi = 0;
        int screendpi = GetDeviceCaps(hdc, LOGPIXELSX);
        for(i=4; i>=0; i--) {
            reqdpi = 0;
            if(SUCCEEDED(GetThemeInt(hTheme, iPartId, iStateId, i + TMT_MINDPI1, &reqdpi))) {
                if(reqdpi != 0 && screendpi >= reqdpi) {
                    TRACE("Using %d DPI, image %d\n", reqdpi, i + TMT_IMAGEFILE1);
                    return MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, i + TMT_IMAGEFILE1);
                }
            }
        }
        /* If an image couldnt be selected, choose the first one */
        return MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, TMT_IMAGEFILE1);
    }
    else if(imageselecttype == IST_SIZE) {
        POINT size = {pRect->right-pRect->left, pRect->bottom-pRect->top};
        POINT reqsize;
        for(i=4; i>=0; i--) {
            if(SUCCEEDED(GetThemePosition(hTheme, iPartId, iStateId, i + TMT_MINSIZE1, &reqsize))) {
                if(reqsize.x >= size.x && reqsize.y >= size.y) {
                    TRACE("Using image size %ldx%ld, image %d\n", reqsize.x, reqsize.y, i + TMT_IMAGEFILE1);
                    return MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, i + TMT_IMAGEFILE1);
                }
            }
        }
        /* If an image couldnt be selected, choose the smallest one */
        return MSSTYLES_FindProperty(hTheme, iPartId, iStateId, TMT_FILENAME, TMT_IMAGEFILE1);
    }
    return NULL;
}

/***********************************************************************
 *      UXTHEME_LoadImage
 *
 * Load image for part/state
 */
static HRESULT UXTHEME_LoadImage(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, BOOL glyph,
                          HBITMAP *hBmp, RECT *bmpRect)
{
    int imagelayout = IL_VERTICAL;
    int imagecount = 1;
    BITMAP bmp;
    WCHAR szPath[MAX_PATH];
    PTHEME_PROPERTY tp = UXTHEME_SelectImage(hTheme, hdc, iPartId, iStateId, pRect, glyph);
    if(!tp) {
        FIXME("Couldn't determine image for part/state %d/%d, invalid theme?\n", iPartId, iStateId);
        return E_PROP_ID_UNSUPPORTED;
    }
    lstrcpynW(szPath, tp->lpValue, min(tp->dwValueLen+1, sizeof(szPath)/sizeof(szPath[0])));
    *hBmp = MSSTYLES_LoadBitmap(hdc, hTheme, szPath);
    if(!*hBmp) {
        TRACE("Failed to load bitmap %s\n", debugstr_w(szPath));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_IMAGELAYOUT, &imagelayout);
    GetThemeInt(hTheme, iPartId, iStateId, TMT_IMAGECOUNT, &imagecount);

    GetObjectW(*hBmp, sizeof(bmp), &bmp);
    if(imagelayout == IL_VERTICAL) {
        int height = bmp.bmHeight/imagecount;
        bmpRect->left = 0;
        bmpRect->right = bmp.bmWidth;
        bmpRect->top = (max(min(imagecount, iStateId), 1)-1) * height;
        bmpRect->bottom = bmpRect->top + height;
    }
    else {
        int width = bmp.bmWidth/imagecount;
        bmpRect->left = (max(min(imagecount, iStateId), 1)-1) * width;
        bmpRect->right = bmpRect->left + width;
        bmpRect->top = 0;
        bmpRect->bottom = bmp.bmHeight;
    }
    return S_OK;
}

/***********************************************************************
 *      UXTHEME_StretchBlt
 *
 * Psudo TransparentBlt/StretchBlt
 */
static inline BOOL UXTHEME_StretchBlt(HDC hdcDst, int nXOriginDst, int nYOriginDst, int nWidthDst, int nHeightDst,
                                      HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc,
                                      BOOL transparent, COLORREF transcolor)
{
    if(transparent) {
        /* Ensure we don't pass any negative values to TransparentBlt */
        return TransparentBlt(hdcDst, nXOriginDst, nYOriginDst, abs(nWidthDst), abs(nHeightDst),
                              hdcSrc, nXOriginSrc, nYOriginSrc, abs(nWidthSrc), abs(nHeightSrc),
                              transcolor);
    }
    /* This should be using AlphaBlend */
    return StretchBlt(hdcDst, nXOriginDst, nYOriginDst, nWidthDst, nHeightDst,
                        hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc,
                        SRCCOPY);
}

/***********************************************************************
 *      UXTHEME_Blt
 *
 * Simplify sending same width/height for both source and dest
 */
static inline BOOL UXTHEME_Blt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
                               HDC hdcSrc, int nXOriginSrc, int nYOriginSrc,
                               BOOL transparent, COLORREF transcolor)
{
    return UXTHEME_StretchBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
                              hdcSrc, nXOriginSrc, nYOriginSrc, nWidthDest, nHeightDest,
                              transparent, transcolor);
}


/***********************************************************************
 *      UXTHEME_DrawImageGlyph
 *
 * Draw an imagefile glyph
 */
static HRESULT UXTHEME_DrawImageGlyph(HTHEME hTheme, HDC hdc, int iPartId,
                               int iStateId, RECT *pRect,
                               const DTBGOPTS *pOptions)
{
    HRESULT hr;
    HBITMAP bmpSrc = NULL;
    HDC hdcSrc = NULL;
    HGDIOBJ oldSrc = NULL;
    RECT rcSrc;
    BOOL transparent = FALSE;
    COLORREF transparentcolor = 0;
    int valign = VA_CENTER;
    int halign = HA_CENTER;
    POINT dstSize;
    POINT srcSize;
    POINT topleft;

    hr = UXTHEME_LoadImage(hTheme, hdc, iPartId, iStateId, pRect, TRUE, &bmpSrc, &rcSrc);
    if(FAILED(hr)) return hr;
    hdcSrc = CreateCompatibleDC(hdc);
    if(!hdcSrc) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DeleteObject(bmpSrc);
        return hr;
    }
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    dstSize.x = pRect->right-pRect->left;
    dstSize.y = pRect->bottom-pRect->top;
    srcSize.x = rcSrc.right-rcSrc.left;
    srcSize.y = rcSrc.bottom-rcSrc.top;

    GetThemeBool(hTheme, iPartId, iStateId, TMT_GLYPHTRANSPARENT, &transparent);
    if(transparent) {
        if(FAILED(GetThemeColor(hTheme, iPartId, iStateId, TMT_GLYPHTRANSPARENTCOLOR, &transparentcolor))) {
            /* If image is transparent, but no color was specified, use magenta */
            transparentcolor = RGB(255, 0, 255);
        }
    }
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_VALIGN, &valign);
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_HALIGN, &halign);

    topleft.x = pRect->left;
    topleft.y = pRect->top;
    if(halign == HA_CENTER)      topleft.x += (dstSize.x/2)-(srcSize.x/2);
    else if(halign == HA_RIGHT)  topleft.x += dstSize.x-srcSize.x;
    if(valign == VA_CENTER)      topleft.y += (dstSize.y/2)-(srcSize.y/2);
    else if(valign == VA_BOTTOM) topleft.y += dstSize.y-srcSize.y;

    if(!UXTHEME_Blt(hdc, topleft.x, topleft.y, srcSize.x, srcSize.y,
                    hdcSrc, rcSrc.left, rcSrc.top,
                    transparent, transparentcolor)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    SelectObject(hdcSrc, oldSrc);
    DeleteDC(hdcSrc);
    DeleteObject(bmpSrc);
    return hr;
}

/***********************************************************************
 *      UXTHEME_DrawImageGlyph
 *
 * Draw glyph on top of background, if appropriate
 */
static HRESULT UXTHEME_DrawGlyph(HTHEME hTheme, HDC hdc, int iPartId,
                                    int iStateId, RECT *pRect,
                                    const DTBGOPTS *pOptions)
{
    int glyphtype = GT_NONE;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_GLYPHTYPE, &glyphtype);

    if(glyphtype == GT_IMAGEGLYPH) {
        return UXTHEME_DrawImageGlyph(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
    }
    else if(glyphtype == GT_FONTGLYPH) {
        /* I don't know what a font glyph is, I've never seen it used in any themes */
        FIXME("Font glyph\n");
    }
    return S_OK;
}

/***********************************************************************
 *      UXTHEME_DrawImageBackground
 *
 * Draw an imagefile background
 */
static HRESULT UXTHEME_DrawImageBackground(HTHEME hTheme, HDC hdc, int iPartId,
                                    int iStateId, RECT *pRect,
                                    const DTBGOPTS *pOptions)
{
    HRESULT hr = S_OK;
    HBITMAP bmpSrc;
    HGDIOBJ oldSrc;
    HDC hdcSrc;
    RECT rcSrc;
    RECT rcDst;
    POINT dstSize;
    POINT srcSize;
    int sizingtype = ST_TRUESIZE;
    BOOL uniformsizing = FALSE;
    BOOL transparent = FALSE;
    COLORREF transparentcolor = 0;

    hr = UXTHEME_LoadImage(hTheme, hdc, iPartId, iStateId, pRect, FALSE, &bmpSrc, &rcSrc);
    if(FAILED(hr)) return hr;
    hdcSrc = CreateCompatibleDC(hdc);
    if(!hdcSrc) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DeleteObject(bmpSrc);
        return hr;
    }
    oldSrc = SelectObject(hdcSrc, bmpSrc);

    CopyRect(&rcDst, pRect);
    
    GetThemeBool(hTheme, iPartId, iStateId, TMT_TRANSPARENT, &transparent);
    if(transparent) {
        if(FAILED(GetThemeColor(hTheme, iPartId, iStateId, TMT_TRANSPARENTCOLOR, &transparentcolor))) {
            /* If image is transparent, but no color was specified, get the color of the upper left corner */
            transparentcolor = GetPixel(hdcSrc, 0, 0);
        }
    }

    dstSize.x = rcDst.right-rcDst.left;
    dstSize.y = rcDst.bottom-rcDst.top;
    srcSize.x = rcSrc.right-rcSrc.left;
    srcSize.y = rcSrc.bottom-rcSrc.top;

    GetThemeBool(hTheme, iPartId, iStateId, TMT_UNIFORMSIZING, &uniformsizing);
    if(uniformsizing) {
        /* Scale height and width equally */
        int widthDiff = abs(srcSize.x-dstSize.x);
        int heightDiff = abs(srcSize.y-dstSize.x);
        if(widthDiff > heightDiff) {
            dstSize.y -= widthDiff-heightDiff;
            rcDst.bottom = rcDst.top + dstSize.y;
        }
        else if(heightDiff > widthDiff) {
            dstSize.x -= heightDiff-widthDiff;
            rcDst.right = rcDst.left + dstSize.x;
        }
    }

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_SIZINGTYPE, &sizingtype);
    if(sizingtype == ST_TRUESIZE) {
        int truesizestretchmark = 0;

        if(dstSize.x < 0 || dstSize.y < 0) {
            BOOL mirrorimage = TRUE;
            GetThemeBool(hTheme, iPartId, iStateId, TMT_MIRRORIMAGE, &mirrorimage);
            if(mirrorimage) {
                if(dstSize.x < 0) {
                    rcDst.left += dstSize.x;
                    rcDst.right += dstSize.x;
                }
                if(dstSize.y < 0) {
                    rcDst.top += dstSize.y;
                    rcDst.bottom += dstSize.y;
                }
            }
        }
        /* Only stretch when target exceeds source by truesizestretchmark percent */
        GetThemeInt(hTheme, iPartId, iStateId, TMT_TRUESIZESTRETCHMARK, &truesizestretchmark);
        if(dstSize.x < 0 || dstSize.y < 0 ||
           MulDiv(srcSize.x, 100, dstSize.x) > truesizestretchmark ||
           MulDiv(srcSize.y, 100, dstSize.y) > truesizestretchmark) {
            if(!UXTHEME_StretchBlt(hdc, rcDst.left, rcDst.top, dstSize.x, dstSize.y,
                                   hdcSrc, rcSrc.left, rcSrc.top, srcSize.x, srcSize.y,
                                   transparent, transparentcolor))
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else {
            rcDst.left += (dstSize.x/2)-(srcSize.x/2);
            rcDst.top  += (dstSize.y/2)-(srcSize.y/2);
            rcDst.right = rcDst.left + srcSize.x;
            rcDst.bottom = rcDst.top + srcSize.y;
            if(!UXTHEME_Blt(hdc, rcDst.left, rcDst.top, srcSize.x, srcSize.y,
                            hdcSrc, rcSrc.left, rcSrc.top,
                            transparent, transparentcolor))
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else {
        HDC hdcDst = NULL;
        HBITMAP bmpDst = NULL;
        HGDIOBJ oldDst = NULL;
        MARGINS sm;

        dstSize.x = abs(dstSize.x);
        dstSize.y = abs(dstSize.y);

        GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_SIZINGMARGINS, NULL, &sm);

        hdcDst = CreateCompatibleDC(hdc);
        if(!hdcDst) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }
        bmpDst = CreateCompatibleBitmap(hdc, dstSize.x, dstSize.y);
        if(!bmpDst) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }
        oldDst = SelectObject(hdcDst, bmpDst);

        /* Upper left corner */
        if(!BitBlt(hdcDst, 0, 0, sm.cxLeftWidth, sm.cyTopHeight,
                   hdcSrc, rcSrc.left, rcSrc.top, SRCCOPY)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }
        /* Upper right corner */
        if(!BitBlt(hdcDst, dstSize.x-sm.cxRightWidth, 0, sm.cxRightWidth, sm.cyTopHeight,
                   hdcSrc, rcSrc.right-sm.cxRightWidth, rcSrc.top, SRCCOPY)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }
        /* Lower left corner */
        if(!BitBlt(hdcDst, 0, dstSize.y-sm.cyBottomHeight, sm.cxLeftWidth, sm.cyBottomHeight,
                   hdcSrc, rcSrc.left, rcSrc.bottom-sm.cyBottomHeight, SRCCOPY)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }
        /* Lower right corner */
        if(!BitBlt(hdcDst, dstSize.x-sm.cxRightWidth, dstSize.y-sm.cyBottomHeight, sm.cxRightWidth, sm.cyBottomHeight,
                   hdcSrc, rcSrc.right-sm.cxRightWidth, rcSrc.bottom-sm.cyBottomHeight, SRCCOPY)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto draw_error; 
        }

        if(sizingtype == ST_TILE) {
            FIXME("Tile\n");
            sizingtype = ST_STRETCH; /* Just use stretch for now */
        }
        if(sizingtype == ST_STRETCH) {
            int destCenterWidth  = dstSize.x - (sm.cxLeftWidth + sm.cxRightWidth);
            int srcCenterWidth   = srcSize.x - (sm.cxLeftWidth + sm.cxRightWidth);
            int destCenterHeight = dstSize.y - (sm.cyTopHeight + sm.cyBottomHeight);
            int srcCenterHeight  = srcSize.y - (sm.cyTopHeight + sm.cyBottomHeight);

            if(destCenterWidth > 0) {
                /* Center top */
                if(!StretchBlt(hdcDst, sm.cxLeftWidth, 0, destCenterWidth, sm.cyTopHeight,
                               hdcSrc, rcSrc.left+sm.cxLeftWidth, rcSrc.top, srcCenterWidth, sm.cyTopHeight, SRCCOPY)) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto draw_error; 
                }
                /* Center bottom */
                if(!StretchBlt(hdcDst, sm.cxLeftWidth, dstSize.y-sm.cyBottomHeight, destCenterWidth, sm.cyBottomHeight,
                               hdcSrc, rcSrc.left+sm.cxLeftWidth, rcSrc.bottom-sm.cyBottomHeight, srcCenterWidth, sm.cyTopHeight, SRCCOPY)) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto draw_error; 
                }
            }
            if(destCenterHeight > 0) {
                /* Left center */
                if(!StretchBlt(hdcDst, 0, sm.cyTopHeight, sm.cxLeftWidth, destCenterHeight,
                           hdcSrc, rcSrc.left, rcSrc.top+sm.cyTopHeight, sm.cxLeftWidth, srcCenterHeight, SRCCOPY)) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto draw_error; 
                }
                /* Right center */
                if(!StretchBlt(hdcDst, dstSize.x-sm.cxRightWidth, sm.cyTopHeight, sm.cxRightWidth, destCenterHeight,
                               hdcSrc, rcSrc.right-sm.cxRightWidth, rcSrc.top+sm.cyTopHeight, sm.cxRightWidth, srcCenterHeight, SRCCOPY)) {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto draw_error; 
                }
            }
            if(destCenterHeight > 0 && destCenterWidth > 0) {
                BOOL borderonly = FALSE;
                GetThemeBool(hTheme, iPartId, iStateId, TMT_BORDERONLY, &borderonly);
                if(!borderonly) {
                    /* Center */
                    if(!StretchBlt(hdcDst, sm.cxLeftWidth, sm.cyTopHeight, destCenterWidth, destCenterHeight,
                                   hdcSrc, rcSrc.left+sm.cxLeftWidth, rcSrc.top+sm.cyTopHeight, srcCenterWidth, srcCenterHeight, SRCCOPY)) {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto draw_error; 
                    }
                }
            }
        }

        if(!UXTHEME_Blt(hdc, rcDst.left, rcDst.top, dstSize.x, dstSize.y,
                        hdcDst, 0, 0,
                        transparent, transparentcolor))
            hr = HRESULT_FROM_WIN32(GetLastError());

draw_error:
        if(hdcDst) {
            SelectObject(hdcDst, oldDst);
            DeleteDC(hdcDst);
        }
        if(bmpDst) DeleteObject(bmpDst);
    }
    SelectObject(hdcSrc, oldSrc);
    DeleteObject(bmpSrc);
    DeleteDC(hdcSrc);
    CopyRect(pRect, &rcDst);
    return hr;
}

/***********************************************************************
 *      UXTHEME_DrawBorderRectangle
 *
 * Draw the bounding rectangle for a borderfill background
 */
static HRESULT UXTHEME_DrawBorderRectangle(HTHEME hTheme, HDC hdc, int iPartId,
                                    int iStateId, RECT *pRect,
                                    const DTBGOPTS *pOptions)
{
    HRESULT hr = S_OK;
    HPEN hPen;
    HGDIOBJ oldPen;
    COLORREF bordercolor = RGB(0,0,0);
    int bordersize = 1;

    GetThemeInt(hTheme, iPartId, iStateId, TMT_BORDERSIZE, &bordersize);
    if(bordersize > 0) {
        POINT ptCorners[5];
        ptCorners[0].x = pRect->left;
        ptCorners[0].y = pRect->top;
        ptCorners[1].x = pRect->right-1;
        ptCorners[1].y = pRect->top;
        ptCorners[2].x = pRect->right-1;
        ptCorners[2].y = pRect->bottom-1;
        ptCorners[3].x = pRect->left;
        ptCorners[3].y = pRect->bottom-1;
        ptCorners[4].x = pRect->left;
        ptCorners[4].y = pRect->top;

        InflateRect(pRect, -bordersize, -bordersize);
        if(pOptions->dwFlags & DTBG_OMITBORDER)
            return S_OK;
        GetThemeColor(hTheme, iPartId, iStateId, TMT_BORDERCOLOR, &bordercolor);
        hPen = CreatePen(PS_SOLID, bordersize, bordercolor);
        if(!hPen)
            return HRESULT_FROM_WIN32(GetLastError());
        oldPen = SelectObject(hdc, hPen);

        if(!Polyline(hdc, ptCorners, 5))
            hr = HRESULT_FROM_WIN32(GetLastError());

        SelectObject(hdc, oldPen);
        DeleteObject(hPen);
    }
    return hr;
}

/***********************************************************************
 *      UXTHEME_DrawBackgroundFill
 *
 * Fill a borderfill background rectangle
 */
static HRESULT UXTHEME_DrawBackgroundFill(HTHEME hTheme, HDC hdc, int iPartId,
                                   int iStateId, RECT *pRect,
                                   const DTBGOPTS *pOptions)
{
    HRESULT hr = S_OK;
    int filltype = FT_SOLID;

    TRACE("(%d,%d,%ld)\n", iPartId, iStateId, pOptions->dwFlags);

    if(pOptions->dwFlags & DTBG_OMITCONTENT)
        return S_OK;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_FILLTYPE, &filltype);

    if(filltype == FT_SOLID) {
        HBRUSH hBrush;
        COLORREF fillcolor = RGB(255,255,255);

        GetThemeColor(hTheme, iPartId, iStateId, TMT_FILLCOLOR, &fillcolor);
        hBrush = CreateSolidBrush(fillcolor);
        if(!FillRect(hdc, pRect, hBrush))
            hr = HRESULT_FROM_WIN32(GetLastError());
        DeleteObject(hBrush);
    }
    else if(filltype == FT_VERTGRADIENT || filltype == FT_HORZGRADIENT) {
        /* FIXME: This only accounts for 2 gradient colors (out of 5) and ignores
            the gradient ratios (no idea how those work)
            Few themes use this, and the ones I've seen only use 2 colors with
            a gradient ratio of 0 and 255 respectivly
        */

        COLORREF gradient1 = RGB(0,0,0);
        COLORREF gradient2 = RGB(255,255,255);
        TRIVERTEX vert[2];
        GRADIENT_RECT gRect;

        FIXME("Gradient implementation not complete\n");

        GetThemeColor(hTheme, iPartId, iStateId, TMT_GRADIENTCOLOR1, &gradient1);
        GetThemeColor(hTheme, iPartId, iStateId, TMT_GRADIENTCOLOR2, &gradient2);

        vert[0].x     = pRect->left;
        vert[0].y     = pRect->top;
        vert[0].Red   = GetRValue(gradient1) << 8;
        vert[0].Green = GetGValue(gradient1) << 8;
        vert[0].Blue  = GetBValue(gradient1) << 8;
        vert[0].Alpha = 0x0000;

        vert[1].x     = pRect->right;
        vert[1].y     = pRect->bottom;
        vert[1].Red   = GetRValue(gradient2) << 8;
        vert[1].Green = GetGValue(gradient2) << 8;
        vert[1].Blue  = GetBValue(gradient2) << 8;
        vert[1].Alpha = 0x0000;

        gRect.UpperLeft  = 0;
        gRect.LowerRight = 1;
        GradientFill(hdc,vert,2,&gRect,1,filltype==FT_HORZGRADIENT?GRADIENT_FILL_RECT_H:GRADIENT_FILL_RECT_V);
    }
    else if(filltype == FT_RADIALGRADIENT) {
        /* I've never seen this used in a theme */
        FIXME("Radial gradient\n");
    }
    else if(filltype == FT_TILEIMAGE) {
        /* I've never seen this used in a theme */
        FIXME("Tile image\n");
    }
    return hr;
}

/***********************************************************************
 *      UXTHEME_DrawBorderBackground
 *
 * Draw an imagefile background
 */
static HRESULT UXTHEME_DrawBorderBackground(HTHEME hTheme, HDC hdc, int iPartId,
                                     int iStateId, const RECT *pRect,
                                     const DTBGOPTS *pOptions)
{
    HRESULT hr;
    RECT rt;

    CopyRect(&rt, pRect);

    hr = UXTHEME_DrawBorderRectangle(hTheme, hdc, iPartId, iStateId, &rt, pOptions);
    if(FAILED(hr))
        return hr;
    return UXTHEME_DrawBackgroundFill(hTheme, hdc, iPartId, iStateId, &rt, pOptions);
}

/***********************************************************************
 *      DrawThemeBackgroundEx                               (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeBackgroundEx(HTHEME hTheme, HDC hdc, int iPartId,
                                     int iStateId, const RECT *pRect,
                                     const DTBGOPTS *pOptions)
{
    HRESULT hr;
    const DTBGOPTS defaultOpts = {sizeof(DTBGOPTS), 0, {0,0,0,0}};
    const DTBGOPTS *opts;
    HRGN clip = NULL;
    int hasClip = -1;
    int bgtype = BT_BORDERFILL;
    RECT rt;

    TRACE("(%p,%p,%d,%d,%ld,%ld)\n", hTheme, hdc, iPartId, iStateId,pRect->left,pRect->top);
    if(!hTheme)
        return E_HANDLE;

    /* Ensure we have a DTBGOPTS structure available, simplifies some of the code */
    opts = pOptions;
    if(!opts) opts = &defaultOpts;

    if(opts->dwFlags & DTBG_CLIPRECT) {
        clip = CreateRectRgn(0,0,1,1);
        hasClip = GetClipRgn(hdc, clip);
        if(hasClip == -1)
            TRACE("Failed to get original clipping region\n");
        else
            IntersectClipRect(hdc, opts->rcClip.left, opts->rcClip.top, opts->rcClip.right, opts->rcClip.bottom);
    }
    CopyRect(&rt, pRect);

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
    if(bgtype == BT_IMAGEFILE)
        hr = UXTHEME_DrawImageBackground(hTheme, hdc, iPartId, iStateId, &rt, opts);
    else if(bgtype == BT_BORDERFILL)
        hr = UXTHEME_DrawBorderBackground(hTheme, hdc, iPartId, iStateId, pRect, opts);
    else {
        FIXME("Unknown background type\n");
        /* This should never happen, and hence I don't know what to return */
        hr = E_FAIL;
    }
    if(SUCCEEDED(hr))
        hr = UXTHEME_DrawGlyph(hTheme, hdc, iPartId, iStateId, &rt, opts);
    if(opts->dwFlags & DTBG_CLIPRECT) {
        if(hasClip == 0)
            SelectClipRgn(hdc, NULL);
        else if(hasClip == 1)
            SelectClipRgn(hdc, clip);
        DeleteObject(clip);
    }
    return hr;
}

/***********************************************************************
 *      DrawThemeEdge                                       (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeEdge(HTHEME hTheme, HDC hdc, int iPartId,
                             int iStateId, const RECT *pDestRect, UINT uEdge,
                             UINT uFlags, RECT *pContentRect)
{
    FIXME("%d %d 0x%08x 0x%08x: stub\n", iPartId, iStateId, uEdge, uFlags);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      DrawThemeIcon                                       (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeIcon(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             const RECT *pRect, HIMAGELIST himl, int iImageIndex)
{
    FIXME("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *      DrawThemeText                                       (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             LPCWSTR pszText, int iCharCount, DWORD dwTextFlags,
                             DWORD dwTextFlags2, const RECT *pRect)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;
    COLORREF textColor;
    COLORREF oldTextColor;
    int oldBkMode;
    RECT rt;
    
    TRACE("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    
    hr = GetThemeFont(hTheme, hdc, iPartId, iStateId, TMT_FONT, &logfont);
    if(SUCCEEDED(hr)) {
        hFont = CreateFontIndirectW(&logfont);
        if(!hFont)
            TRACE("Failed to create font\n");
    }
    CopyRect(&rt, pRect);
    if(hFont)
        oldFont = SelectObject(hdc, hFont);
        
    if(dwTextFlags2 & DTT_GRAYED)
        textColor = GetSysColor(COLOR_GRAYTEXT);
    else {
        if(FAILED(GetThemeColor(hTheme, iPartId, iStateId, TMT_TEXTCOLOR, &textColor)))
            textColor = GetTextColor(hdc);
    }
    oldTextColor = SetTextColor(hdc, textColor);
    oldBkMode = SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, pszText, iCharCount, &rt, dwTextFlags);
    SetBkMode(hdc, oldBkMode);
    SetTextColor(hdc, oldTextColor);

    if(hFont) {
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    return S_OK;
}

/***********************************************************************
 *      GetThemeBackgroundContentRect                       (UXTHEME.@)
 */
HRESULT WINAPI GetThemeBackgroundContentRect(HTHEME hTheme, HDC hdc, int iPartId,
                                             int iStateId,
                                             const RECT *pBoundingRect,
                                             RECT *pContentRect)
{
    MARGINS margin;
    HRESULT hr;

    TRACE("(%d,%d)\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;

    /* try content margins property... */
    hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_CONTENTMARGINS, NULL, &margin);
    if(SUCCEEDED(hr)) {
        pContentRect->left = pBoundingRect->left + margin.cxLeftWidth;
        pContentRect->top  = pBoundingRect->top + margin.cyTopHeight;
        pContentRect->right = pBoundingRect->right - margin.cxRightWidth;
        pContentRect->bottom = pBoundingRect->bottom - margin.cyBottomHeight;
    } else {
        /* otherwise, try to determine content rect from the background type and props */
        int bgtype = BT_BORDERFILL;
        memcpy(pContentRect, pBoundingRect, sizeof(RECT));

        GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
        if(bgtype == BT_BORDERFILL) {
            int bordersize = 1;
    
            GetThemeInt(hTheme, iPartId, iStateId, TMT_BORDERSIZE, &bordersize);
            InflateRect(pContentRect, -bordersize, -bordersize);
        } else if ((bgtype == BT_IMAGEFILE)
                && (SUCCEEDED(hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, 
                TMT_SIZINGMARGINS, NULL, &margin)))) {
            pContentRect->left = pBoundingRect->left + margin.cxLeftWidth;
            pContentRect->top  = pBoundingRect->top + margin.cyTopHeight;
            pContentRect->right = pBoundingRect->right - margin.cxRightWidth;
            pContentRect->bottom = pBoundingRect->bottom - margin.cyBottomHeight;
        }
        /* If nothing was found, leave unchanged */
    }

    TRACE("left:%ld,top:%ld,right:%ld,bottom:%ld\n", pContentRect->left, pContentRect->top, pContentRect->right, pContentRect->bottom);

    return S_OK;
}

/***********************************************************************
 *      GetThemeBackgroundExtent                            (UXTHEME.@)
 */
HRESULT WINAPI GetThemeBackgroundExtent(HTHEME hTheme, HDC hdc, int iPartId,
                                        int iStateId, const RECT *pContentRect,
                                        RECT *pExtentRect)
{
    MARGINS margin;
    HRESULT hr;

    TRACE("(%d,%d)\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;

    hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_CONTENTMARGINS, NULL, &margin);
    if(FAILED(hr)) {
        TRACE("Margins not found\n");
        return hr;
    }
    pExtentRect->left = pContentRect->left - margin.cxLeftWidth;
    pExtentRect->top  = pContentRect->top - margin.cyTopHeight;
    pExtentRect->right = pContentRect->right + margin.cxRightWidth;
    pExtentRect->bottom = pContentRect->bottom + margin.cyBottomHeight;

    TRACE("left:%ld,top:%ld,right:%ld,bottom:%ld\n", pExtentRect->left, pExtentRect->top, pExtentRect->right, pExtentRect->bottom);

    return S_OK;
}

/***********************************************************************
 *      GetThemeBackgroundRegion                            (UXTHEME.@)
 *
 * Calculate the background region, taking into consideration transparent areas
 * of the background image.
 */
HRESULT WINAPI GetThemeBackgroundRegion(HTHEME hTheme, HDC hdc, int iPartId,
                                        int iStateId, const RECT *pRect,
                                        HRGN *pRegion)
{
    HRESULT hr = S_OK;
    int bgtype = BT_BORDERFILL;

    TRACE("(%p,%p,%d,%d)\n", hTheme, hdc, iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;
    if(!pRect || !pRegion)
        return E_POINTER;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
    if(bgtype == BT_IMAGEFILE) {
        FIXME("Images not handled yet\n");
        hr = ERROR_CALL_NOT_IMPLEMENTED;
    }
    else if(bgtype == BT_BORDERFILL) {
        *pRegion = CreateRectRgn(pRect->left, pRect->top, pRect->right, pRect->bottom);
        if(!*pRegion)
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else {
        FIXME("Unknown background type\n");
        /* This should never happen, and hence I don't know what to return */
        hr = E_FAIL;
    }
    return hr;
}

/***********************************************************************
 *      GetThemePartSize                                    (UXTHEME.@)
 */
HRESULT WINAPI GetThemePartSize(HTHEME hTheme, HDC hdc, int iPartId,
                                int iStateId, RECT *prc, THEMESIZE eSize,
                                SIZE *psz)
{
    FIXME("%d %d %d: stub\n", iPartId, iStateId, eSize);
    if(!hTheme)
        return E_HANDLE;
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 *      GetThemeTextExtent                                  (UXTHEME.@)
 */
HRESULT WINAPI GetThemeTextExtent(HTHEME hTheme, HDC hdc, int iPartId,
                                  int iStateId, LPCWSTR pszText, int iCharCount,
                                  DWORD dwTextFlags, const RECT *pBoundingRect,
                                  RECT *pExtentRect)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;
    RECT rt = {0,0,0xFFFF,0xFFFF};
    
    TRACE("%d %d: stub\n", iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;

    if(pBoundingRect)
        CopyRect(&rt, pBoundingRect);
            
    hr = GetThemeFont(hTheme, hdc, iPartId, iStateId, TMT_FONT, &logfont);
    if(SUCCEEDED(hr)) {
        hFont = CreateFontIndirectW(&logfont);
        if(!hFont)
            TRACE("Failed to create font\n");
    }
    if(hFont)
        oldFont = SelectObject(hdc, hFont);
        
    DrawTextW(hdc, pszText, iCharCount, &rt, dwTextFlags|DT_CALCRECT);
    CopyRect(pExtentRect, &rt);

    if(hFont) {
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    return S_OK;
}

/***********************************************************************
 *      GetThemeTextMetrics                                 (UXTHEME.@)
 */
HRESULT WINAPI GetThemeTextMetrics(HTHEME hTheme, HDC hdc, int iPartId,
                                   int iStateId, TEXTMETRICW *ptm)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;

    TRACE("(%p, %p, %d, %d)\n", hTheme, hdc, iPartId, iStateId);
    if(!hTheme)
        return E_HANDLE;

    hr = GetThemeFont(hTheme, hdc, iPartId, iStateId, TMT_FONT, &logfont);
    if(SUCCEEDED(hr)) {
        hFont = CreateFontIndirectW(&logfont);
        if(!hFont)
            TRACE("Failed to create font\n");
    }
    if(hFont)
        oldFont = SelectObject(hdc, hFont);

    if(!GetTextMetricsW(hdc, ptm))
        hr = HRESULT_FROM_WIN32(GetLastError());

    if(hFont) {
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    return hr;
}

/***********************************************************************
 *      IsThemeBackgroundPartiallyTransparent               (UXTHEME.@)
 */
BOOL WINAPI IsThemeBackgroundPartiallyTransparent(HTHEME hTheme, int iPartId,
                                                  int iStateId)
{
    BOOL transparent = FALSE;
    TRACE("(%d,%d)\n", iPartId, iStateId);
    GetThemeBool(hTheme, iPartId, iStateId, TMT_TRANSPARENT, &transparent);
    return transparent;
}
