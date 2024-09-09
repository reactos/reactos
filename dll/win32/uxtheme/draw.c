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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "uxthemep.h"

#include <stdlib.h>

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
    BOOL res;

    TRACE("(%p,0x%08x\n", hwnd, dwFlags);
    res = SetPropW (hwnd, (LPCWSTR)MAKEINTATOM(atDialogThemeEnabled), 
                    UlongToHandle(dwFlags|0x80000000));
        /* 0x80000000 serves as a "flags set" flag */
    if (!res)
          return HRESULT_FROM_WIN32(GetLastError());
    if (dwFlags & ETDT_USETABTEXTURE)
        return SetWindowTheme (hwnd, NULL, szTab);
    else
        return S_OK;
}

/***********************************************************************
 *      IsThemeDialogTextureEnabled                         (UXTHEME.@)
 */
BOOL WINAPI IsThemeDialogTextureEnabled(HWND hwnd)
{
    DWORD dwDialogTextureFlags;
    TRACE("(%p)\n", hwnd);

    dwDialogTextureFlags = HandleToUlong( GetPropW( hwnd, (LPCWSTR)MAKEINTATOM(atDialogThemeEnabled) ));
    if (dwDialogTextureFlags == 0) 
        /* Means EnableThemeDialogTexture wasn't called for this dialog */
        return FALSE;

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

    if (!IsWindow(hwnd) || !hdc)
        return E_HANDLE;

    if (prc && IsBadReadPtr (prc, sizeof(RECT)))
        return E_POINTER;

    hParent = GetParent(hwnd);
    if(!hParent)
        return S_OK;

    if(prc) {
        rt = *prc;
        MapWindowPoints(hwnd, hParent, (LPPOINT)&rt, 2);

        clip = CreateRectRgn(0,0,1,1);
        hasClip = GetClipRgn(hdc, clip);
        if(hasClip == -1)
            TRACE("Failed to get original clipping region\n");
        else
            IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }
    else {
        GetClientRect(hwnd, &rt);
        MapWindowPoints(hwnd, hParent, (LPPOINT)&rt, 2);
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
        opts.rcClip = *pClipRect;
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
    PTHEME_CLASS pClass;
    PTHEME_PROPERTY tp;
    int imageselecttype = IST_NONE;
    int i;
    int image;

    if(glyph)
        image = TMT_GLYPHIMAGEFILE;
    else
        image = TMT_IMAGEFILE;

    pClass = ValidateHandle(hTheme);
    if((tp=MSSTYLES_FindProperty(pClass, iPartId, iStateId, TMT_FILENAME, image)))
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
                    return MSSTYLES_FindProperty(pClass, iPartId, iStateId, TMT_FILENAME, i + TMT_IMAGEFILE1);
                }
            }
        }
        /* If an image couldn't be selected, choose the first one */
        return MSSTYLES_FindProperty(pClass, iPartId, iStateId, TMT_FILENAME, TMT_IMAGEFILE1);
    }
    else if(imageselecttype == IST_SIZE) {
        POINT size = {pRect->right-pRect->left, pRect->bottom-pRect->top};
        POINT reqsize;
        for(i=4; i>=0; i--) {
            PTHEME_PROPERTY fileProp = 
                MSSTYLES_FindProperty(pClass, iPartId, iStateId, TMT_FILENAME, i + TMT_IMAGEFILE1);
            if (!fileProp) continue;
            if(FAILED(GetThemePosition(hTheme, iPartId, iStateId, i + TMT_MINSIZE1, &reqsize))) {
                /* fall back to size of Nth image */
                WCHAR szPath[MAX_PATH];
                int imagelayout = IL_HORIZONTAL;
                int imagecount = 1;
                BITMAP bmp;
                HBITMAP hBmp;
                BOOL hasAlpha;

                lstrcpynW(szPath, fileProp->lpValue, 
                    min(fileProp->dwValueLen+1, sizeof(szPath)/sizeof(szPath[0])));
                hBmp = MSSTYLES_LoadBitmap(pClass, szPath, &hasAlpha);
                if(!hBmp) continue;

                GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_IMAGELAYOUT, &imagelayout);
                GetThemeInt(hTheme, iPartId, iStateId, TMT_IMAGECOUNT, &imagecount);

                GetObjectW(hBmp, sizeof(bmp), &bmp);
                if(imagelayout == IL_VERTICAL) {
                    reqsize.x = bmp.bmWidth;
                    reqsize.y = bmp.bmHeight/imagecount;
                }
                else {
                    reqsize.x = bmp.bmWidth/imagecount;
                    reqsize.y = bmp.bmHeight;
                }
            }
            if(reqsize.x <= size.x && reqsize.y <= size.y) {
                TRACE("Using image size %dx%d, image %d\n", reqsize.x, reqsize.y, i + TMT_IMAGEFILE1);
                return fileProp;
            }
        }
        /* If an image couldn't be selected, choose the smallest one */
        return MSSTYLES_FindProperty(pClass, iPartId, iStateId, TMT_FILENAME, TMT_IMAGEFILE1);
    }
    return NULL;
}

/***********************************************************************
 *      UXTHEME_LoadImage
 *
 * Load image for part/state
 */
HRESULT UXTHEME_LoadImage(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, BOOL glyph,
                          HBITMAP *hBmp, RECT *bmpRect, BOOL* hasImageAlpha)
{
    int imagelayout = IL_HORIZONTAL;
    int imagecount = 1;
    int imagenum;
    BITMAP bmp;
    WCHAR szPath[MAX_PATH];
    PTHEME_PROPERTY tp;
    PTHEME_CLASS pClass;

    pClass = ValidateHandle(hTheme);
    if (!pClass)
            return E_HANDLE;

    tp = UXTHEME_SelectImage(hTheme, hdc, iPartId, iStateId, pRect, glyph);
    if(!tp) {
        FIXME("Couldn't determine image for part/state %d/%d, invalid theme?\n", iPartId, iStateId);
        return E_PROP_ID_UNSUPPORTED;
    }
    lstrcpynW(szPath, tp->lpValue, min(tp->dwValueLen+1, sizeof(szPath)/sizeof(szPath[0])));
    *hBmp = MSSTYLES_LoadBitmap(pClass, szPath, hasImageAlpha);
    if(!*hBmp) {
        TRACE("Failed to load bitmap %s\n", debugstr_w(szPath));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_IMAGELAYOUT, &imagelayout);
    GetThemeInt(hTheme, iPartId, iStateId, TMT_IMAGECOUNT, &imagecount);

    imagenum = max (min (imagecount, iStateId), 1) - 1;
    GetObjectW(*hBmp, sizeof(bmp), &bmp);

    if(imagecount < 1) imagecount = 1;

    if(imagelayout == IL_VERTICAL) {
        int height = bmp.bmHeight/imagecount;
        bmpRect->left = 0;
        bmpRect->right = bmp.bmWidth;
        bmpRect->top = imagenum * height;
        bmpRect->bottom = bmpRect->top + height;
    }
    else {
        int width = bmp.bmWidth/imagecount;
        bmpRect->left = imagenum * width;
        bmpRect->right = bmpRect->left + width;
        bmpRect->top = 0;
        bmpRect->bottom = bmp.bmHeight;
    }
    return S_OK;
}

/* Get transparency parameters passed to UXTHEME_StretchBlt() - the parameters 
 * depend on whether the image has full alpha  or whether it is 
 * color-transparent or just opaque. */
static inline void get_transparency (HTHEME hTheme, int iPartId, int iStateId, 
                                     BOOL hasImageAlpha, INT* transparent,
                                     COLORREF* transparentcolor, BOOL glyph)
{
    if (hasImageAlpha)
    {
        *transparent = ALPHABLEND_FULL;
        *transparentcolor = RGB (255, 0, 255);
    }
    else
    {
        BOOL trans = FALSE;
        GetThemeBool(hTheme, iPartId, iStateId, 
            glyph ? TMT_GLYPHTRANSPARENT : TMT_TRANSPARENT, &trans);
        if(trans) {
            *transparent = ALPHABLEND_BINARY;
            if(FAILED(GetThemeColor(hTheme, iPartId, iStateId, 
                glyph ? TMT_GLYPHTRANSPARENTCOLOR : TMT_TRANSPARENTCOLOR, 
                transparentcolor))) {
                /* If image is transparent, but no color was specified, use magenta */
                *transparentcolor = RGB(255, 0, 255);
            }
        }
        else
            *transparent = ALPHABLEND_NONE;
    }
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
    RECT rcSrc;
    INT transparent = 0;
    COLORREF transparentcolor;
    int valign = VA_CENTER;
    int halign = HA_CENTER;
    POINT dstSize;
    POINT srcSize;
    BOOL hasAlpha;
    RECT rcDst;
    GDI_DRAW_STREAM DrawStream;

    hr = UXTHEME_LoadImage(hTheme, hdc, iPartId, iStateId, pRect, TRUE, 
        &bmpSrc, &rcSrc, &hasAlpha);
    if(FAILED(hr)) return hr;

    dstSize.x = pRect->right-pRect->left;
    dstSize.y = pRect->bottom-pRect->top;
    srcSize.x = rcSrc.right-rcSrc.left;
    srcSize.y = rcSrc.bottom-rcSrc.top;

    get_transparency (hTheme, iPartId, iStateId, hasAlpha, &transparent,
        &transparentcolor, TRUE);
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_VALIGN, &valign);
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_HALIGN, &halign);

    rcDst = *pRect;
    if(halign == HA_CENTER)      rcDst.left += (dstSize.x/2)-(srcSize.x/2);
    else if(halign == HA_RIGHT)  rcDst.left += dstSize.x-srcSize.x;
    if(valign == VA_CENTER)      rcDst.top += (dstSize.y/2)-(srcSize.y/2);
    else if(valign == VA_BOTTOM) rcDst.top += dstSize.y-srcSize.y;

    rcDst.right = rcDst.left + srcSize.x;
    rcDst.bottom = rcDst.top + srcSize.y;
    
    DrawStream.signature = 0x44727753;
    DrawStream.reserved = 0;
    DrawStream.unknown1 = 1;
    DrawStream.unknown2 = 9;
    DrawStream.hDC = hdc;
    DrawStream.hImage = bmpSrc;
    DrawStream.crTransparent = transparentcolor;
    DrawStream.rcSrc = rcSrc;
    DrawStream.rcDest = rcDst;
    DrawStream.leftSizingMargin = 0;
    DrawStream.rightSizingMargin = 0;
    DrawStream.topSizingMargin = 0;
    DrawStream.bottomSizingMargin = 0;
    DrawStream.drawOption = DS_TRUESIZE;

    if (transparent == ALPHABLEND_FULL)
        DrawStream.drawOption |= DS_TRANSPARENTALPHA;
    else if (transparent == ALPHABLEND_BINARY)
        DrawStream.drawOption |= DS_TRANSPARENTCLR;

    GdiDrawStream(hdc, sizeof(DrawStream), &DrawStream);
    return HRESULT_FROM_WIN32(GetLastError());
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
 * get_image_part_size
 *
 * Used by GetThemePartSize and UXTHEME_DrawImageBackground
 */
static HRESULT get_image_part_size (HTHEME hTheme, HDC hdc, int iPartId,
                                    int iStateId, RECT *prc, THEMESIZE eSize,
                                    POINT *psz)
{
    HRESULT hr = S_OK;
    HBITMAP bmpSrc;
    RECT rcSrc;
    BOOL hasAlpha;

    hr = UXTHEME_LoadImage(hTheme, hdc, iPartId, iStateId, prc, FALSE, 
        &bmpSrc, &rcSrc, &hasAlpha);
    if (FAILED(hr)) return hr;

    switch (eSize)
    {
        case TS_DRAW:
            if (prc != NULL)
            {
                RECT rcDst;
                POINT dstSize;
                POINT srcSize;
                int sizingtype = ST_STRETCH;
                BOOL uniformsizing = FALSE;

                rcDst = *prc;

                dstSize.x = rcDst.right-rcDst.left;
                dstSize.y = rcDst.bottom-rcDst.top;
                srcSize.x = rcSrc.right-rcSrc.left;
                srcSize.y = rcSrc.bottom-rcSrc.top;
            
                GetThemeBool(hTheme, iPartId, iStateId, TMT_UNIFORMSIZING, &uniformsizing);
                if(uniformsizing) {
                    /* Scale height and width equally */
                    if (dstSize.x*srcSize.y < dstSize.y*srcSize.x)
                    {
                        dstSize.y = MulDiv (srcSize.y, dstSize.x, srcSize.x);
                        rcDst.bottom = rcDst.top + dstSize.y;
                    }
                    else
                    {
                        dstSize.x = MulDiv (srcSize.x, dstSize.y, srcSize.y);
                        rcDst.right = rcDst.left + dstSize.x;
                    }
                }
            
                GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_SIZINGTYPE, &sizingtype);
                if(sizingtype == ST_TRUESIZE) {
                    int truesizestretchmark = 100;
            
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
                    /* Whatever TrueSizeStretchMark does - it does not seem to
                     * be what's outlined below. It appears as if native 
                     * uxtheme always stretches if dest is smaller than source
                     * (ie as if TrueSizeStretchMark==100 with the code below) */
#if 0
                    /* Only stretch when target exceeds source by truesizestretchmark percent */
                    GetThemeInt(hTheme, iPartId, iStateId, TMT_TRUESIZESTRETCHMARK, &truesizestretchmark);
#endif
                    if(dstSize.x < 0 || dstSize.y < 0 ||
                      (MulDiv(srcSize.x, 100, dstSize.x) > truesizestretchmark &&
                      MulDiv(srcSize.y, 100, dstSize.y) > truesizestretchmark)) {
                        memcpy (psz, &dstSize, sizeof (SIZE));
                    }
                    else {
                        memcpy (psz, &srcSize, sizeof (SIZE));
                    }
                }
                else
                {
                    psz->x = abs(dstSize.x);
                    psz->y = abs(dstSize.y);
                }
                break;
            }
            /* else fall through */
        case TS_MIN:
            /* FIXME: couldn't figure how native uxtheme computes min size */
        case TS_TRUE:
            psz->x = rcSrc.right - rcSrc.left;
            psz->y = rcSrc.bottom - rcSrc.top;
            break;
    }
    return hr;
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
    RECT rcSrc;
    RECT rcDst;
    POINT dstSize;
    POINT drawSize;
    int sizingtype = ST_STRETCH;
    INT transparent;
    COLORREF transparentcolor = 0;
    BOOL hasAlpha;
    MARGINS sm;
    GDI_DRAW_STREAM DrawStream;

    hr = UXTHEME_LoadImage(hTheme, hdc, iPartId, iStateId, pRect, FALSE, &bmpSrc, &rcSrc, &hasAlpha);
    if(FAILED(hr)) 
        return hr;
    get_transparency (hTheme, iPartId, iStateId, hasAlpha, &transparent, &transparentcolor, FALSE);

    rcDst = *pRect;
    dstSize.x = rcDst.right-rcDst.left;
    dstSize.y = rcDst.bottom-rcDst.top;

    GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_SIZINGMARGINS, NULL, &sm);
    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_SIZINGTYPE, &sizingtype);

    /*FIXME: Is this ever used? */
    /*GetThemeBool(hTheme, iPartId, iStateId, TMT_BORDERONLY, &borderonly);*/

    if(sizingtype == ST_TRUESIZE) {
        int valign = VA_CENTER, halign = HA_CENTER;

        get_image_part_size (hTheme, hdc, iPartId, iStateId, pRect, TS_DRAW, &drawSize);
        GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_VALIGN, &valign);
        GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_HALIGN, &halign);

        if (halign == HA_CENTER)
            rcDst.left += (dstSize.x/2)-(drawSize.x/2);
        else if (halign == HA_RIGHT)
            rcDst.left = rcDst.right - drawSize.x;
        if (valign == VA_CENTER)
            rcDst.top  += (dstSize.y/2)-(drawSize.y/2);
        else if (valign == VA_BOTTOM)
            rcDst.top = rcDst.bottom - drawSize.y;
        rcDst.right = rcDst.left + drawSize.x;
        rcDst.bottom = rcDst.top + drawSize.y;
        *pRect = rcDst;
    }

    DrawStream.signature = 0x44727753;
    DrawStream.reserved = 0;
    DrawStream.unknown1 = 1;
    DrawStream.unknown2 = 9;
    DrawStream.hDC = hdc;
    DrawStream.hImage = bmpSrc;
    DrawStream.crTransparent = transparentcolor;
    DrawStream.rcSrc = rcSrc;
    DrawStream.rcDest = rcDst;
    DrawStream.leftSizingMargin = sm.cxLeftWidth;
    DrawStream.rightSizingMargin = sm.cxRightWidth;
    DrawStream.topSizingMargin = sm.cyTopHeight;
    DrawStream.bottomSizingMargin = sm.cyBottomHeight;
    DrawStream.drawOption = 0;

    if (transparent == ALPHABLEND_FULL)
        DrawStream.drawOption |= DS_TRANSPARENTALPHA;
    else if (transparent == ALPHABLEND_BINARY)
        DrawStream.drawOption |= DS_TRANSPARENTCLR;

    if (sizingtype == ST_TILE)
        DrawStream.drawOption |= DS_TILE;
    else if (sizingtype == ST_TRUESIZE)
        DrawStream.drawOption |= DS_TRUESIZE;

    GdiDrawStream(hdc, sizeof(DrawStream), &DrawStream);  
    return HRESULT_FROM_WIN32(GetLastError());
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

    TRACE("(%d,%d,%d)\n", iPartId, iStateId, pOptions->dwFlags);

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
            a gradient ratio of 0 and 255 respectively
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
        vert[0].Alpha = 0xff00;

        vert[1].x     = pRect->right;
        vert[1].y     = pRect->bottom;
        vert[1].Red   = GetRValue(gradient2) << 8;
        vert[1].Green = GetGValue(gradient2) << 8;
        vert[1].Blue  = GetBValue(gradient2) << 8;
        vert[1].Alpha = 0xff00;

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

    rt = *pRect;

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

    TRACE("(%p,%p,%d,%d,%d,%d)\n", hTheme, hdc, iPartId, iStateId,pRect->left,pRect->top);
    if(!hTheme)
        return E_HANDLE;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
    if (bgtype == BT_NONE) return S_OK;

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
    rt = *pRect;

    if(bgtype == BT_IMAGEFILE)
        hr = UXTHEME_DrawImageBackground(hTheme, hdc, iPartId, iStateId, &rt, opts);
    else if(bgtype == BT_BORDERFILL)
        hr = UXTHEME_DrawBorderBackground(hTheme, hdc, iPartId, iStateId, pRect, opts);
    else {
        FIXME("Unknown background type\n");
        /* This should never happen, and hence I don't know what to return */
        hr = E_FAIL;
    }
#if 0
    if(SUCCEEDED(hr))
#endif
    {
        RECT rcGlyph = *pRect;
        MARGINS margin;
        hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_CONTENTMARGINS, NULL, &margin);
        if(SUCCEEDED(hr))
        {
            rcGlyph.left += margin.cxLeftWidth;
            rcGlyph.right -= margin.cxRightWidth;
            rcGlyph.top += margin.cyTopHeight;
            rcGlyph.bottom -= margin.cyBottomHeight;
        }
        hr = UXTHEME_DrawGlyph(hTheme, hdc, iPartId, iStateId, &rcGlyph, opts);
    }
    if(opts->dwFlags & DTBG_CLIPRECT) {
        if(hasClip == 0)
            SelectClipRgn(hdc, NULL);
        else if(hasClip == 1)
            SelectClipRgn(hdc, clip);
        DeleteObject(clip);
    }
    return hr;
}

/*
 * DrawThemeEdge() implementation
 *
 * Since it basically is DrawEdge() with different colors, I copied its code
 * from user32's uitools.c.
 */

enum
{
    EDGE_LIGHT,
    EDGE_HIGHLIGHT,
    EDGE_SHADOW,
    EDGE_DARKSHADOW,
    EDGE_FILL,

    EDGE_WINDOW,
    EDGE_WINDOWFRAME,

    EDGE_NUMCOLORS
};

static const struct 
{
    int themeProp;
    int sysColor;
} EdgeColorMap[EDGE_NUMCOLORS] = {
    {TMT_EDGELIGHTCOLOR,                  COLOR_3DLIGHT},
    {TMT_EDGEHIGHLIGHTCOLOR,              COLOR_BTNHIGHLIGHT},
    {TMT_EDGESHADOWCOLOR,                 COLOR_BTNSHADOW},
    {TMT_EDGEDKSHADOWCOLOR,               COLOR_3DDKSHADOW},
    {TMT_EDGEFILLCOLOR,                   COLOR_BTNFACE},
    {-1,                                  COLOR_WINDOW},
    {-1,                                  COLOR_WINDOWFRAME}
};

static const signed char LTInnerNormal[] = {
    -1,           -1,                 -1,                 -1,
    -1,           EDGE_HIGHLIGHT,     EDGE_HIGHLIGHT,     -1,
    -1,           EDGE_DARKSHADOW,    EDGE_DARKSHADOW,    -1,
    -1,           -1,                 -1,                 -1
};

static const signed char LTOuterNormal[] = {
    -1,                 EDGE_LIGHT,     EDGE_SHADOW, -1,
    EDGE_HIGHLIGHT,     EDGE_LIGHT,     EDGE_SHADOW, -1,
    EDGE_DARKSHADOW,    EDGE_LIGHT,     EDGE_SHADOW, -1,
    -1,                 EDGE_LIGHT,     EDGE_SHADOW, -1
};

static const signed char RBInnerNormal[] = {
    -1,           -1,                 -1,               -1,
    -1,           EDGE_SHADOW,        EDGE_SHADOW,      -1,
    -1,           EDGE_LIGHT,         EDGE_LIGHT,       -1,
    -1,           -1,                 -1,               -1
};

static const signed char RBOuterNormal[] = {
    -1,               EDGE_DARKSHADOW,  EDGE_HIGHLIGHT, -1,
    EDGE_SHADOW,      EDGE_DARKSHADOW,  EDGE_HIGHLIGHT, -1,
    EDGE_LIGHT,       EDGE_DARKSHADOW,  EDGE_HIGHLIGHT, -1,
    -1,               EDGE_DARKSHADOW,  EDGE_HIGHLIGHT, -1
};

static const signed char LTInnerSoft[] = {
    -1,                  -1,                -1,               -1,
    -1,                  EDGE_LIGHT,        EDGE_LIGHT,       -1,
    -1,                  EDGE_SHADOW,       EDGE_SHADOW,      -1,
    -1,                  -1,                -1,               -1
};

static const signed char LTOuterSoft[] = {
    -1,               EDGE_HIGHLIGHT, EDGE_DARKSHADOW, -1,
    EDGE_LIGHT,       EDGE_HIGHLIGHT, EDGE_DARKSHADOW, -1,
    EDGE_SHADOW,      EDGE_HIGHLIGHT, EDGE_DARKSHADOW, -1,
    -1,               EDGE_HIGHLIGHT, EDGE_DARKSHADOW, -1
};

#define RBInnerSoft RBInnerNormal   /* These are the same */
#define RBOuterSoft RBOuterNormal

static const signed char LTRBOuterMono[] = {
    -1,           EDGE_WINDOWFRAME, EDGE_WINDOWFRAME, EDGE_WINDOWFRAME,
    EDGE_WINDOW,  EDGE_WINDOWFRAME, EDGE_WINDOWFRAME, EDGE_WINDOWFRAME,
    EDGE_WINDOW,  EDGE_WINDOWFRAME, EDGE_WINDOWFRAME, EDGE_WINDOWFRAME,
    EDGE_WINDOW,  EDGE_WINDOWFRAME, EDGE_WINDOWFRAME, EDGE_WINDOWFRAME,
};

static const signed char LTRBInnerMono[] = {
    -1, -1,           -1,           -1,
    -1, EDGE_WINDOW,  EDGE_WINDOW,  EDGE_WINDOW,
    -1, EDGE_WINDOW,  EDGE_WINDOW,  EDGE_WINDOW,
    -1, EDGE_WINDOW,  EDGE_WINDOW,  EDGE_WINDOW,
};

static const signed char LTRBOuterFlat[] = {
    -1,                 EDGE_SHADOW, EDGE_SHADOW, EDGE_SHADOW,
    EDGE_FILL,          EDGE_SHADOW, EDGE_SHADOW, EDGE_SHADOW,
    EDGE_FILL,          EDGE_SHADOW, EDGE_SHADOW, EDGE_SHADOW,
    EDGE_FILL,          EDGE_SHADOW, EDGE_SHADOW, EDGE_SHADOW,
};

static const signed char LTRBInnerFlat[] = {
    -1, -1,               -1,               -1,
    -1, EDGE_FILL,        EDGE_FILL,        EDGE_FILL,
    -1, EDGE_FILL,        EDGE_FILL,        EDGE_FILL,
    -1, EDGE_FILL,        EDGE_FILL,        EDGE_FILL,
};

static COLORREF get_edge_color (int edgeType, HTHEME theme, int part, int state)
{
    COLORREF col;
    if ((EdgeColorMap[edgeType].themeProp == -1)
        || FAILED (GetThemeColor (theme, part, state, 
            EdgeColorMap[edgeType].themeProp, &col)))
        col = GetSysColor (EdgeColorMap[edgeType].sysColor);
    return col;
}

static inline HPEN get_edge_pen (int edgeType, HTHEME theme, int part, int state)
{
    return CreatePen (PS_SOLID, 1, get_edge_color (edgeType, theme, part, state));
}

static inline HBRUSH get_edge_brush (int edgeType, HTHEME theme, int part, int state)
{
    return CreateSolidBrush (get_edge_color (edgeType, theme, part, state));
}

/***********************************************************************
 *           draw_diag_edge
 *
 * Same as DrawEdge invoked with BF_DIAGONAL
 */
static HRESULT draw_diag_edge (HDC hdc, HTHEME theme, int part, int state,
                               const RECT* rc, UINT uType, 
                               UINT uFlags, LPRECT contentsRect)
{
    POINT Points[4];
    signed char InnerI, OuterI;
    HPEN InnerPen, OuterPen;
    POINT SavePoint;
    HPEN SavePen;
    int spx, spy;
    int epx, epy;
    int Width = rc->right - rc->left;
    int Height= rc->bottom - rc->top;
    int SmallDiam = Width > Height ? Height : Width;
    HRESULT retval = (((uType & BDR_INNER) == BDR_INNER
                       || (uType & BDR_OUTER) == BDR_OUTER)
                      && !(uFlags & (BF_FLAT|BF_MONO)) ) ? E_FAIL : S_OK;
    int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0)
            + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0);

    /* Init some vars */
    OuterPen = InnerPen = GetStockObject(NULL_PEN);
    SavePen = SelectObject(hdc, InnerPen);
    spx = spy = epx = epy = 0; /* Satisfy the compiler... */

    /* Determine the colors of the edges */
    if(uFlags & BF_MONO)
    {
        InnerI = LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)];
        OuterI = LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)];
    }
    else if(uFlags & BF_FLAT)
    {
        InnerI = LTRBInnerFlat[uType & (BDR_INNER|BDR_OUTER)];
        OuterI = LTRBOuterFlat[uType & (BDR_INNER|BDR_OUTER)];
    }
    else if(uFlags & BF_SOFT)
    {
        if(uFlags & BF_BOTTOM)
        {
            InnerI = RBInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = RBOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
        }
        else
        {
            InnerI = LTInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = LTOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
        }
    }
    else
    {
        if(uFlags & BF_BOTTOM)
        {
            InnerI = RBInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = RBOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
        }
        else
        {
            InnerI = LTInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
            OuterI = LTOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
        }
    }

    if(InnerI != -1) InnerPen = get_edge_pen (InnerI, theme, part, state);
    if(OuterI != -1) OuterPen = get_edge_pen (OuterI, theme, part, state);

    MoveToEx(hdc, 0, 0, &SavePoint);

    /* Don't ask me why, but this is what is visible... */
    /* This must be possible to do much simpler, but I fail to */
    /* see the logic in the MS implementation (sigh...). */
    /* So, this might look a bit brute force here (and it is), but */
    /* it gets the job done;) */

    switch(uFlags & BF_RECT)
    {
    case 0:
    case BF_LEFT:
    case BF_BOTTOM:
    case BF_BOTTOMLEFT:
        /* Left bottom endpoint */
        epx = rc->left-1;
        spx = epx + SmallDiam;
        epy = rc->bottom;
        spy = epy - SmallDiam;
        break;

    case BF_TOPLEFT:
    case BF_BOTTOMRIGHT:
        /* Left top endpoint */
        epx = rc->left-1;
        spx = epx + SmallDiam;
        epy = rc->top-1;
        spy = epy + SmallDiam;
        break;

    case BF_TOP:
    case BF_RIGHT:
    case BF_TOPRIGHT:
    case BF_RIGHT|BF_LEFT:
    case BF_RIGHT|BF_LEFT|BF_TOP:
    case BF_BOTTOM|BF_TOP:
    case BF_BOTTOM|BF_TOP|BF_LEFT:
    case BF_BOTTOMRIGHT|BF_LEFT:
    case BF_BOTTOMRIGHT|BF_TOP:
    case BF_RECT:
        /* Right top endpoint */
        spx = rc->left;
        epx = spx + SmallDiam;
        spy = rc->bottom-1;
        epy = spy - SmallDiam;
        break;
    }

    MoveToEx(hdc, spx, spy, NULL);
    SelectObject(hdc, OuterPen);
    LineTo(hdc, epx, epy);

    SelectObject(hdc, InnerPen);

    switch(uFlags & (BF_RECT|BF_DIAGONAL))
    {
    case BF_DIAGONAL_ENDBOTTOMLEFT:
    case (BF_DIAGONAL|BF_BOTTOM):
    case BF_DIAGONAL:
    case (BF_DIAGONAL|BF_LEFT):
        MoveToEx(hdc, spx-1, spy, NULL);
        LineTo(hdc, epx, epy-1);
        Points[0].x = spx-add;
        Points[0].y = spy;
        Points[1].x = rc->left;
        Points[1].y = rc->top;
        Points[2].x = epx+1;
        Points[2].y = epy-1-add;
        Points[3] = Points[2];
        break;

    case BF_DIAGONAL_ENDBOTTOMRIGHT:
        MoveToEx(hdc, spx-1, spy, NULL);
        LineTo(hdc, epx, epy+1);
        Points[0].x = spx-add;
        Points[0].y = spy;
        Points[1].x = rc->left;
        Points[1].y = rc->bottom-1;
        Points[2].x = epx+1;
        Points[2].y = epy+1+add;
        Points[3] = Points[2];
        break;

    case (BF_DIAGONAL|BF_BOTTOM|BF_RIGHT|BF_TOP):
    case (BF_DIAGONAL|BF_BOTTOM|BF_RIGHT|BF_TOP|BF_LEFT):
    case BF_DIAGONAL_ENDTOPRIGHT:
    case (BF_DIAGONAL|BF_RIGHT|BF_TOP|BF_LEFT):
        MoveToEx(hdc, spx+1, spy, NULL);
        LineTo(hdc, epx, epy+1);
        Points[0].x = epx-1;
        Points[0].y = epy+1+add;
        Points[1].x = rc->right-1;
        Points[1].y = rc->top+add;
        Points[2].x = rc->right-1;
        Points[2].y = rc->bottom-1;
        Points[3].x = spx+add;
        Points[3].y = spy;
        break;

    case BF_DIAGONAL_ENDTOPLEFT:
        MoveToEx(hdc, spx, spy-1, NULL);
        LineTo(hdc, epx+1, epy);
        Points[0].x = epx+1+add;
        Points[0].y = epy+1;
        Points[1].x = rc->right-1;
        Points[1].y = rc->top;
        Points[2].x = rc->right-1;
        Points[2].y = rc->bottom-1-add;
        Points[3].x = spx;
        Points[3].y = spy-add;
        break;

    case (BF_DIAGONAL|BF_TOP):
    case (BF_DIAGONAL|BF_BOTTOM|BF_TOP):
    case (BF_DIAGONAL|BF_BOTTOM|BF_TOP|BF_LEFT):
        MoveToEx(hdc, spx+1, spy-1, NULL);
        LineTo(hdc, epx, epy);
        Points[0].x = epx-1;
        Points[0].y = epy+1;
        Points[1].x = rc->right-1;
        Points[1].y = rc->top;
        Points[2].x = rc->right-1;
        Points[2].y = rc->bottom-1-add;
        Points[3].x = spx+add;
        Points[3].y = spy-add;
        break;

    case (BF_DIAGONAL|BF_RIGHT):
    case (BF_DIAGONAL|BF_RIGHT|BF_LEFT):
    case (BF_DIAGONAL|BF_RIGHT|BF_LEFT|BF_BOTTOM):
        MoveToEx(hdc, spx, spy, NULL);
        LineTo(hdc, epx-1, epy+1);
        Points[0].x = spx;
        Points[0].y = spy;
        Points[1].x = rc->left;
        Points[1].y = rc->top+add;
        Points[2].x = epx-1-add;
        Points[2].y = epy+1+add;
        Points[3] = Points[2];
        break;
    }

    /* Fill the interior if asked */
    if((uFlags & BF_MIDDLE) && retval)
    {
        HBRUSH hbsave;
        HBRUSH hb = get_edge_brush ((uFlags & BF_MONO) ? EDGE_WINDOW : EDGE_FILL, 
            theme, part, state);
        HPEN hpsave;
        HPEN hp = get_edge_pen ((uFlags & BF_MONO) ? EDGE_WINDOW : EDGE_FILL, 
            theme, part, state);
        hbsave = SelectObject(hdc, hb);
        hpsave = SelectObject(hdc, hp);
        Polygon(hdc, Points, 4);
        SelectObject(hdc, hbsave);
        SelectObject(hdc, hpsave);
        DeleteObject (hp);
        DeleteObject (hb);
    }

    /* Adjust rectangle if asked */
    if(uFlags & BF_ADJUST)
    {
        *contentsRect = *rc;
        if(uFlags & BF_LEFT)   contentsRect->left   += add;
        if(uFlags & BF_RIGHT)  contentsRect->right  -= add;
        if(uFlags & BF_TOP)    contentsRect->top    += add;
        if(uFlags & BF_BOTTOM) contentsRect->bottom -= add;
    }

    /* Cleanup */
    SelectObject(hdc, SavePen);
    MoveToEx(hdc, SavePoint.x, SavePoint.y, NULL);
    if(InnerI != -1) DeleteObject (InnerPen);
    if(OuterI != -1) DeleteObject (OuterPen);

    return retval;
}

/***********************************************************************
 *           draw_rect_edge
 *
 * Same as DrawEdge invoked without BF_DIAGONAL
 */
static HRESULT draw_rect_edge (HDC hdc, HTHEME theme, int part, int state,
                               const RECT* rc, UINT uType, 
                               UINT uFlags, LPRECT contentsRect)
{
    signed char LTInnerI, LTOuterI;
    signed char RBInnerI, RBOuterI;
    HPEN LTInnerPen, LTOuterPen;
    HPEN RBInnerPen, RBOuterPen;
    RECT InnerRect = *rc;
    POINT SavePoint;
    HPEN SavePen;
    int LBpenplus = 0;
    int LTpenplus = 0;
    int RTpenplus = 0;
    int RBpenplus = 0;
    HRESULT retval = (((uType & BDR_INNER) == BDR_INNER
                       || (uType & BDR_OUTER) == BDR_OUTER)
                      && !(uFlags & (BF_FLAT|BF_MONO)) ) ? E_FAIL : S_OK;

    /* Init some vars */
    LTInnerPen = LTOuterPen = RBInnerPen = RBOuterPen = GetStockObject(NULL_PEN);
    SavePen = SelectObject(hdc, LTInnerPen);

    /* Determine the colors of the edges */
    if(uFlags & BF_MONO)
    {
        LTInnerI = RBInnerI = LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = RBOuterI = LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)];
    }
    else if(uFlags & BF_FLAT)
    {
        LTInnerI = RBInnerI = LTRBInnerFlat[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = RBOuterI = LTRBOuterFlat[uType & (BDR_INNER|BDR_OUTER)];

        if( LTInnerI != -1 ) LTInnerI = RBInnerI = EDGE_FILL;
    }
    else if(uFlags & BF_SOFT)
    {
        LTInnerI = LTInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = LTOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
        RBInnerI = RBInnerSoft[uType & (BDR_INNER|BDR_OUTER)];
        RBOuterI = RBOuterSoft[uType & (BDR_INNER|BDR_OUTER)];
    }
    else
    {
        LTInnerI = LTInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
        LTOuterI = LTOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
        RBInnerI = RBInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
        RBOuterI = RBOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
    }

    if((uFlags & BF_BOTTOMLEFT) == BF_BOTTOMLEFT)   LBpenplus = 1;
    if((uFlags & BF_TOPRIGHT) == BF_TOPRIGHT)       RTpenplus = 1;
    if((uFlags & BF_BOTTOMRIGHT) == BF_BOTTOMRIGHT) RBpenplus = 1;
    if((uFlags & BF_TOPLEFT) == BF_TOPLEFT)         LTpenplus = 1;

    if(LTInnerI != -1) LTInnerPen = get_edge_pen (LTInnerI, theme, part, state);
    if(LTOuterI != -1) LTOuterPen = get_edge_pen (LTOuterI, theme, part, state);
    if(RBInnerI != -1) RBInnerPen = get_edge_pen (RBInnerI, theme, part, state);
    if(RBOuterI != -1) RBOuterPen = get_edge_pen (RBOuterI, theme, part, state);

    MoveToEx(hdc, 0, 0, &SavePoint);

    /* Draw the outer edge */
    SelectObject(hdc, LTOuterPen);
    if(uFlags & BF_TOP)
    {
        MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
        LineTo(hdc, InnerRect.right, InnerRect.top);
    }
    if(uFlags & BF_LEFT)
    {
        MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
        LineTo(hdc, InnerRect.left, InnerRect.bottom);
    }
    SelectObject(hdc, RBOuterPen);
    if(uFlags & BF_BOTTOM)
    {
        MoveToEx(hdc, InnerRect.right-1, InnerRect.bottom-1, NULL);
        LineTo(hdc, InnerRect.left-1, InnerRect.bottom-1);
    }
    if(uFlags & BF_RIGHT)
    {
        MoveToEx(hdc, InnerRect.right-1, InnerRect.bottom-1, NULL);
        LineTo(hdc, InnerRect.right-1, InnerRect.top-1);
    }

    /* Draw the inner edge */
    SelectObject(hdc, LTInnerPen);
    if(uFlags & BF_TOP)
    {
        MoveToEx(hdc, InnerRect.left+LTpenplus, InnerRect.top+1, NULL);
        LineTo(hdc, InnerRect.right-RTpenplus, InnerRect.top+1);
    }
    if(uFlags & BF_LEFT)
    {
        MoveToEx(hdc, InnerRect.left+1, InnerRect.top+LTpenplus, NULL);
        LineTo(hdc, InnerRect.left+1, InnerRect.bottom-LBpenplus);
    }
    SelectObject(hdc, RBInnerPen);
    if(uFlags & BF_BOTTOM)
    {
        MoveToEx(hdc, InnerRect.right-1-RBpenplus, InnerRect.bottom-2, NULL);
        LineTo(hdc, InnerRect.left-1+LBpenplus, InnerRect.bottom-2);
    }
    if(uFlags & BF_RIGHT)
    {
        MoveToEx(hdc, InnerRect.right-2, InnerRect.bottom-1-RBpenplus, NULL);
        LineTo(hdc, InnerRect.right-2, InnerRect.top-1+RTpenplus);
    }

    if( ((uFlags & BF_MIDDLE) && retval) || (uFlags & BF_ADJUST) )
    {
        int add = (LTRBInnerMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0)
                + (LTRBOuterMono[uType & (BDR_INNER|BDR_OUTER)] != -1 ? 1 : 0);

        if(uFlags & BF_LEFT)   InnerRect.left   += add;
        if(uFlags & BF_RIGHT)  InnerRect.right  -= add;
        if(uFlags & BF_TOP)    InnerRect.top    += add;
        if(uFlags & BF_BOTTOM) InnerRect.bottom -= add;

        if((uFlags & BF_MIDDLE) && retval)
        {
            HBRUSH br = get_edge_brush ((uFlags & BF_MONO) ? EDGE_WINDOW : EDGE_FILL, 
                theme, part, state);
            FillRect(hdc, &InnerRect, br);
            DeleteObject (br);
        }

        if(uFlags & BF_ADJUST)
            *contentsRect = InnerRect;
    }

    /* Cleanup */
    SelectObject(hdc, SavePen);
    MoveToEx(hdc, SavePoint.x, SavePoint.y, NULL);
    if(LTInnerI != -1) DeleteObject (LTInnerPen);
    if(LTOuterI != -1) DeleteObject (LTOuterPen);
    if(RBInnerI != -1) DeleteObject (RBInnerPen);
    if(RBOuterI != -1) DeleteObject (RBOuterPen);
    return retval;
}


/***********************************************************************
 *      DrawThemeEdge                                       (UXTHEME.@)
 *
 * DrawThemeEdge() is pretty similar to the vanilla DrawEdge() - the
 * difference is that it does not rely on the system colors alone, but
 * also allows color specification in the theme.
 */
HRESULT WINAPI DrawThemeEdge(HTHEME hTheme, HDC hdc, int iPartId,
                             int iStateId, const RECT *pDestRect, UINT uEdge,
                             UINT uFlags, RECT *pContentRect)
{
    TRACE("%d %d 0x%08x 0x%08x\n", iPartId, iStateId, uEdge, uFlags);
    if(!hTheme)
        return E_HANDLE;
     
    if(uFlags & BF_DIAGONAL)
        return draw_diag_edge (hdc, hTheme, iPartId, iStateId, pDestRect,
            uEdge, uFlags, pContentRect);
    else
        return draw_rect_edge (hdc, hTheme, iPartId, iStateId, pDestRect,
            uEdge, uFlags, pContentRect);
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
    return E_NOTIMPL;
}

typedef int (WINAPI * DRAWSHADOWTEXT)(HDC hdc, LPCWSTR pszText, UINT cch, RECT *prc, DWORD dwFlags,
                          COLORREF crText, COLORREF crShadow, int ixOffset, int iyOffset);

/***********************************************************************
 *      DrawThemeTextEx                                     (UXTHEME.@)
 */
HRESULT
WINAPI
DrawThemeTextEx(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ int iPartId,
    _In_ int iStateId,
    _In_ LPCWSTR pszText,
    _In_ int iCharCount,
    _In_ DWORD dwTextFlags,
    _Inout_ LPRECT pRect,
    _In_ const DTTOPTS *options
)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;
    COLORREF textColor;
    COLORREF oldTextColor;
    COLORREF shadowColor;
    POINT ptShadowOffset;
    int oldBkMode;
    RECT rt;
    int iShadowType;
    DWORD optFlags;

    if(!hTheme)
        return E_HANDLE;
    if (!options)
        return E_NOTIMPL;

    optFlags = options->dwFlags;

    hr = GetThemeFont(hTheme, hdc, iPartId, iStateId, TMT_FONT, &logfont);
    if(SUCCEEDED(hr)) 
    {
        hFont = CreateFontIndirectW(&logfont);
        if(!hFont)
        {
            ERR("Failed to create font\n");
        }
    }

    CopyRect(&rt, pRect);
    if(hFont)
        oldFont = SelectObject(hdc, hFont);

    oldBkMode = SetBkMode(hdc, TRANSPARENT);

    if (optFlags & DTT_TEXTCOLOR)
    {
        textColor = options->crText;
    }
    else
    {
        int textColorProp = TMT_TEXTCOLOR;
        if (optFlags & DTT_COLORPROP)
            textColorProp = options->iColorPropId;

        if (FAILED(GetThemeColor(hTheme, iPartId, iStateId, textColorProp, &textColor)))
            textColor = GetTextColor(hdc);
    }

    hr = GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_TEXTSHADOWTYPE, &iShadowType);
    if (SUCCEEDED(hr))
    {
        hr = GetThemeColor(hTheme, iPartId, iStateId, TMT_TEXTSHADOWCOLOR, &shadowColor);
        if (FAILED(hr))
        {
            ERR("GetThemeColor failed\n");
        }

        hr = GetThemePosition(hTheme, iPartId, iStateId, TMT_TEXTSHADOWOFFSET, &ptShadowOffset);
        if (FAILED(hr))
        {
            ERR("GetThemePosition failed\n");
        }

        if (iShadowType == TST_SINGLE)
        {
            oldTextColor = SetTextColor(hdc, shadowColor);
            OffsetRect(&rt, ptShadowOffset.x, ptShadowOffset.y);
            DrawTextW(hdc, pszText, iCharCount, &rt, dwTextFlags);
            OffsetRect(&rt, -ptShadowOffset.x, -ptShadowOffset.y);
            SetTextColor(hdc, oldTextColor);
        }
        else if (iShadowType == TST_CONTINUOUS)
        {
            HANDLE hcomctl32 = GetModuleHandleW(L"comctl32.dll");
            DRAWSHADOWTEXT pDrawShadowText;
            if (!hcomctl32)
            {
                hcomctl32 = LoadLibraryW(L"comctl32.dll");
                if (!hcomctl32)
                    ERR("Failed to load comctl32\n");
            }

            pDrawShadowText = (DRAWSHADOWTEXT)GetProcAddress(hcomctl32, "DrawShadowText");
            if (pDrawShadowText)
            {
                pDrawShadowText(hdc, pszText, iCharCount, &rt, dwTextFlags, textColor, shadowColor, ptShadowOffset.x, ptShadowOffset.y);
                goto cleanup;
            }
        }
    }

    oldTextColor = SetTextColor(hdc, textColor);
    DrawTextW(hdc, pszText, iCharCount, &rt, dwTextFlags);
    SetTextColor(hdc, oldTextColor);
cleanup:
    SetBkMode(hdc, oldBkMode);

    if(hFont) {
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    return S_OK;
}

/***********************************************************************
 *      DrawThemeText                                       (UXTHEME.@)
 */
HRESULT WINAPI DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             LPCWSTR pszText, int iCharCount, DWORD dwTextFlags,
                             DWORD dwTextFlags2, const RECT *pRect)
{
    DTTOPTS opts = { 0 };
    RECT rt = *pRect;

    TRACE("(%p %p %d %d %s:%d 0x%08lx 0x%08lx %p)\n", hTheme, hdc, iPartId, iStateId,
        debugstr_wn(pszText, iCharCount), iCharCount, dwTextFlags, dwTextFlags2, pRect);

    if (dwTextFlags2 & DTT_GRAYED)
    {
        opts.dwFlags = DTT_TEXTCOLOR;
        opts.crText = GetSysColor(COLOR_GRAYTEXT);
    }
    opts.dwSize = sizeof(opts);

    return DrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, iCharCount, dwTextFlags, &rt, &opts);
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
        *pContentRect = *pBoundingRect;

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

    TRACE("%s\n", wine_dbgstr_rect(pContentRect));

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

    /* try content margins property... */
    hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, TMT_CONTENTMARGINS, NULL, &margin);
    if(SUCCEEDED(hr)) {
        pExtentRect->left = pContentRect->left - margin.cxLeftWidth;
        pExtentRect->top  = pContentRect->top - margin.cyTopHeight;
        pExtentRect->right = pContentRect->right + margin.cxRightWidth;
        pExtentRect->bottom = pContentRect->bottom + margin.cyBottomHeight;
    } else {
        /* otherwise, try to determine content rect from the background type and props */
        int bgtype = BT_BORDERFILL;
        *pExtentRect = *pContentRect;

        GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
        if(bgtype == BT_BORDERFILL) {
            int bordersize = 1;

            GetThemeInt(hTheme, iPartId, iStateId, TMT_BORDERSIZE, &bordersize);
            InflateRect(pExtentRect, bordersize, bordersize);
        } else if ((bgtype == BT_IMAGEFILE)
                && (SUCCEEDED(hr = GetThemeMargins(hTheme, hdc, iPartId, iStateId, 
                TMT_SIZINGMARGINS, NULL, &margin)))) {
            pExtentRect->left = pContentRect->left - margin.cxLeftWidth;
            pExtentRect->top  = pContentRect->top - margin.cyTopHeight;
            pExtentRect->right = pContentRect->right + margin.cxRightWidth;
            pExtentRect->bottom = pContentRect->bottom + margin.cyBottomHeight;
        }
        /* If nothing was found, leave unchanged */
    }

    TRACE("%s\n", wine_dbgstr_rect(pExtentRect));

    return S_OK;
}


static HBITMAP UXTHEME_DrawThemePartToDib(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect)
{
    HDC hdcMem;
    BITMAPINFO bmi;
    HBITMAP hbmp, hbmpOld;
    HBRUSH hbrBack;

    hdcMem = CreateCompatibleDC(0);

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = pRect->right;
    bmi.bmiHeader.biHeight = -pRect->bottom;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    hbmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS , NULL, 0, 0);

    hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmp);
    
    /* FIXME: use an internal function that doesn't do transparent blt */
    hbrBack = CreateSolidBrush(RGB(255,0,255));

    FillRect(hdcMem, pRect, hbrBack);

    DrawThemeBackground(hTheme, hdcMem, iPartId, iStateId, pRect, NULL);

    DeleteObject(hbrBack);
    SelectObject(hdcMem, hbmpOld);
    DeleteObject(hdcMem);

    return hbmp;
}

#define PT_IN_RECT(lprc,x,y) ( x >= lprc->left && x < lprc->right && \
                               y >= lprc->top  && y < lprc->bottom)

static HRGN UXTHEME_RegionFromDibBits(RGBQUAD* pBuffer, RGBQUAD* pclrTransparent, LPCRECT pRect)
{
    int x, y, xstart;
    int cMaxRgnRects, cRgnDataSize, cRgnRects;
    RECT* prcCurrent;
    PRGNDATA prgnData;
    ULONG clrTransparent, *pclrCurrent;
    HRGN hrgnRet;

    pclrCurrent = (PULONG)pBuffer;
    clrTransparent = *(PULONG)pclrTransparent;

    /* Create a region and pre-allocate memory enough for 3 spaces in one row*/
    cRgnRects = 0;
    cMaxRgnRects = 4* (pRect->bottom-pRect->top);
    cRgnDataSize = sizeof(RGNDATA) + cMaxRgnRects * sizeof(RECT);

    /* Allocate the region data */
    prgnData = (PRGNDATA)HeapAlloc(GetProcessHeap(), 0, cRgnDataSize);

    prcCurrent = (PRECT)prgnData->Buffer;
    
    /* Calculate the region rects */
    y=0;
    /* Scan each line of the bitmap */
    while(y<pRect->bottom)
    {
        x=0;
        /* Scan each pixel */
        while (x<pRect->right)
        {
            /* Check if the pixel is not transparent and it is in the requested rect */
            if(*pclrCurrent != clrTransparent && PT_IN_RECT(pRect,x,y))
            {
                xstart = x;
                /* Find the end of the opaque row of pixels */
                while (x<pRect->right)
                {
                    if(*pclrCurrent == clrTransparent || !PT_IN_RECT(pRect,x,y))
                        break;
                    x++;
                    pclrCurrent++;
                }

                /* Add the scaned line to the region */
                SetRect(prcCurrent, xstart, y,x,y+1);
                prcCurrent++;
                cRgnRects++;

                /* Increase the size of the buffer if it is full */
                if(cRgnRects == cMaxRgnRects)
                {
                    cMaxRgnRects *=2;
                    cRgnDataSize = sizeof(RGNDATA) + cMaxRgnRects * sizeof(RECT);
                    prgnData = (PRGNDATA)HeapReAlloc(GetProcessHeap(), 
                                                     0, 
                                                     prgnData, 
                                                     cRgnDataSize);
                    prcCurrent = (RECT*)prgnData->Buffer + cRgnRects;
                }
            }
            else
            {
                x++;
                pclrCurrent++;
            }
        }
        y++;
    }

    /* Fill the region data header */
    prgnData->rdh.dwSize = sizeof(prgnData->rdh);
    prgnData->rdh.iType = RDH_RECTANGLES;
    prgnData->rdh.nCount = cRgnRects;
    prgnData->rdh.nRgnSize = cRgnDataSize;
    prgnData->rdh.rcBound = *pRect;

    /* Create the region*/
    hrgnRet = ExtCreateRegion (NULL, cRgnDataSize, prgnData);

    /* Free the region data*/
    HeapFree(GetProcessHeap(),0,prgnData);

    /* return the region*/
    return hrgnRet;
}

HRESULT UXTHEME_GetImageBackBackgroundRegion(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, HRGN *pRegion)
{
    HBITMAP hbmp;
    DIBSECTION dib;
    RGBQUAD clrTransparent = {0xFF,0x0, 0xFF,0x0};

    /* Draw the theme part to a dib */
    hbmp = UXTHEME_DrawThemePartToDib(hTheme, hdc, iPartId, iStateId, pRect);

    /* Retrieve the info of the dib section */
    GetObjectW(hbmp, sizeof (DIBSECTION), &dib);

    /* Convert the bits of the dib section to a region */
    *pRegion = UXTHEME_RegionFromDibBits((RGBQUAD*)dib.dsBm.bmBits, &clrTransparent, pRect);

    /* Free the temp bitmap */
    DeleteObject(hbmp);

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
        hr = UXTHEME_GetImageBackBackgroundRegion(hTheme, hdc, iPartId, iStateId, pRect, pRegion);
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

/* compute part size for "borderfill" backgrounds */
static HRESULT get_border_background_size (HTHEME hTheme, int iPartId,
                                           int iStateId, THEMESIZE eSize, POINT* psz)
{
    HRESULT hr = S_OK;
    int bordersize = 1;

    if (SUCCEEDED (hr = GetThemeInt(hTheme, iPartId, iStateId, TMT_BORDERSIZE, 
        &bordersize)))
    {
        psz->x = psz->y = 2*bordersize;
        if (eSize != TS_MIN)
        {
            psz->x++;
            psz->y++; 
        }
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
    int bgtype = BT_BORDERFILL;
    HRESULT hr = S_OK;
    POINT size = {1, 1};

    if(!hTheme)
        return E_HANDLE;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);
    if (bgtype == BT_NONE)
        /* do nothing */;
    else if(bgtype == BT_IMAGEFILE)
        hr = get_image_part_size (hTheme, hdc, iPartId, iStateId, prc, eSize, &size);
    else if(bgtype == BT_BORDERFILL)
        hr = get_border_background_size (hTheme, iPartId, iStateId, eSize, &size);
    else {
        FIXME("Unknown background type\n");
        /* This should never happen, and hence I don't know what to return */
        hr = E_FAIL;
    }
    psz->cx = size.x;
    psz->cy = size.y;
    return hr;
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
        rt = *pBoundingRect;

    hr = GetThemeFont(hTheme, hdc, iPartId, iStateId, TMT_FONT, &logfont);
    if(SUCCEEDED(hr)) {
        hFont = CreateFontIndirectW(&logfont);
        if(!hFont)
            TRACE("Failed to create font\n");
    }
    if(hFont)
        oldFont = SelectObject(hdc, hFont);
        
    DrawTextW(hdc, pszText, iCharCount, &rt, dwTextFlags|DT_CALCRECT);
    *pExtentRect = rt;

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
    int bgtype = BT_BORDERFILL;
    RECT rect = {0, 0, 0, 0};
    HBITMAP bmpSrc;
    RECT rcSrc;
    BOOL hasAlpha;
    INT transparent;
    COLORREF transparentcolor;

    TRACE("(%d,%d)\n", iPartId, iStateId);

    if(!hTheme)
        return FALSE;

    GetThemeEnumValue(hTheme, iPartId, iStateId, TMT_BGTYPE, &bgtype);

#ifdef __REACTOS__
    if (bgtype == BT_NONE) return TRUE;
#endif
    if (bgtype != BT_IMAGEFILE) return FALSE;

    if(FAILED (UXTHEME_LoadImage (hTheme, 0, iPartId, iStateId, &rect, FALSE, 
                                  &bmpSrc, &rcSrc, &hasAlpha))) 
        return FALSE;

    get_transparency (hTheme, iPartId, iStateId, hasAlpha, &transparent,
        &transparentcolor, FALSE);
    return (transparent != ALPHABLEND_NONE);
}
