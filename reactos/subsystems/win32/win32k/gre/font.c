/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/font.c
 * PURPOSE:         ReactOS font support routines
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

static void SharpGlyphMono(PDC physDev, INT x, INT y,
                           void *bitmap, GlyphInfo *gi, EBRUSHOBJ *pTextBrush)
{
#if 1
    unsigned char   *srcLine = bitmap, *src;
    unsigned char   bits, bitsMask;
    int             width = gi->width;
    int             stride = ((width + 31) & ~31) >> 3;
    int             height = gi->height;
    int             w;
    int             xspan, lenspan;
    RECTL           rcBounds;

    x -= gi->x;
    y -= gi->y;
    while (height--)
    {
        src = srcLine;
        srcLine += stride;
        w = width;
        
        bitsMask = 0x80;    /* FreeType is always MSB first */
        bits = *src++;
        
        xspan = x;
        while (w)
        {
            if (bits & bitsMask)
            {
                lenspan = 0;
                do
                {
                    lenspan++;
                    if (lenspan == w)
                        break;
                    bitsMask = bitsMask >> 1;
                    if (!bitsMask)
                    {
                        bits = *src++;
                        bitsMask = 0x80;
                    }
                } while (bits & bitsMask);
                rcBounds.left = xspan; rcBounds.top = y;
                rcBounds.right = xspan+lenspan; rcBounds.bottom = y+1;
                GreLineTo(&physDev->dclevel.pSurface->SurfObj,
                    physDev->CombinedClip,
                    &pTextBrush->BrushObject,
                    xspan,
                    y,
                    xspan + lenspan,
                    y,
                    &rcBounds,
                    0);
                xspan += lenspan;
                w -= lenspan;
            }
            else
            {
                do
                {
                    w--;
                    xspan++;
                    if (!w)
                        break;
                    bitsMask = bitsMask >> 1;
                    if (!bitsMask)
                    {
                        bits = *src++;
                        bitsMask = 0x80;
                    }
                } while (!(bits & bitsMask));
            }
        }
        y++;
    }
#else
    HBITMAP hBmp;
    SIZEL szSize;
    SURFOBJ *pSurfObj;
    POINT ptBrush, ptSrc;
    RECTL rcTarget;

    szSize.cx = gi->width;
    szSize.cy = gi->height;

    hBmp = GreCreateBitmap(szSize, ((gi->width + 31) & ~31) >> 3, BMF_1BPP, 0, bitmap);

    pSurfObj = EngLockSurface((HSURF)hBmp);

    ptBrush.x = 0; ptBrush.y = 0;
    ptSrc.x = 0; ptSrc.y = 0;
    rcTarget.left = x; rcTarget.top = y;
    rcTarget.right = x + szSize.cx; rcTarget.bottom = y + szSize.cy;

    GrepBitBltEx(
        &physDev->dclevel.pSurface->SurfObj,
        pSurfObj,
        NULL,
        NULL,
        NULL,
        &rcTarget,
        &ptSrc,
        NULL,
        &physDev->dclevel.pbrLine->BrushObject,
        &ptBrush,
        ROP3_TO_ROP4(SRCCOPY),
        TRUE);

    EngUnlockSurface(pSurfObj);

    GreDeleteObject(hBmp);
#endif
}

static void SharpGlyphGray(PDC physDev, INT x, INT y,
                           void *bitmap, GlyphInfo *gi, EBRUSHOBJ *pTextBrush)
{
    unsigned char   *srcLine = bitmap, *src, bits;
    int             width = gi->width;
    int             stride = ((width + 3) & ~3);
    int             height = gi->height;
    int             w;
    int             xspan, lenspan;
    RECTL           rcBounds;

    x -= gi->x;
    y -= gi->y;
    while (height--)
    {
        src = srcLine;
        srcLine += stride;
        w = width;
        
        bits = *src++;
        xspan = x;
        while (w)
        {
            if (bits >= 0x80)
            {
                lenspan = 0;
                do
                {
                    lenspan++;
                    if (lenspan == w)
                        break;
                    bits = *src++;
                } while (bits >= 0x80);
                rcBounds.left = xspan; rcBounds.top = y;
                rcBounds.right = xspan+lenspan; rcBounds.bottom = y+1;
                GreLineTo(&physDev->dclevel.pSurface->SurfObj,
                    physDev->CombinedClip,
                    &pTextBrush->BrushObject,
                    xspan,
                    y,
                    xspan + lenspan,
                    y,
                    &rcBounds,
                    0);
                xspan += lenspan;
                w -= lenspan;
            }
            else
            {
                do
                {
                    w--;
                    xspan++;
                    if (!w)
                        break;
                    bits = *src++;
                } while (bits < 0x80);
            }
        }
        y++;
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

VOID NTAPI
GreTextOut(PDC pDC, INT x, INT y, UINT flags,
           const RECT *lprect, LPCWSTR wstr, UINT count,
           const INT *lpDx, gsCacheEntryFormat *formatEntry,
           AA_Type aa_type)
{
    POINT offset = {0, 0};
    INT idx;
    EBRUSHOBJ pTextPen;
    PBRUSH ppen;
    HPEN hpen;

    /* Create pen for text output */
    hpen  = GreCreatePen(PS_SOLID, 1, pDC->crForegroundClr, NULL);
    if(!hpen)
        return;
    ppen = PEN_ShareLockPen(hpen); 
    if(!ppen)
        return;
    EBRUSHOBJ_vInit(&pTextPen, ppen, pDC);

    if(aa_type == AA_None || pDC->dclevel.pSurface->SurfObj.iBitmapFormat == BMF_1BPP)
    {
        void (* sharp_glyph_fn)(PDC, INT, INT, void *, GlyphInfo *, EBRUSHOBJ *);

        if(aa_type == AA_None)
            sharp_glyph_fn = SharpGlyphMono;
        else
            sharp_glyph_fn = SharpGlyphGray;

        for(idx = 0; idx < count; idx++) {
            sharp_glyph_fn(pDC,
                           pDC->ptlDCOrig.x + x + offset.x,
                           pDC->ptlDCOrig.y + y + offset.y,
                formatEntry->bitmaps[wstr[idx]],
                &formatEntry->gis[wstr[idx]],
                &pTextPen);
            if(lpDx)
            {
                if(flags & ETO_PDY)
                {
                    offset.x += lpDx[idx * 2];
                    offset.y += lpDx[idx * 2 + 1];
            }
                else
                    offset.x += lpDx[idx];
        }
            else
            {
                offset.x += formatEntry->gis[wstr[idx]].xOff;
                offset.y += formatEntry->gis[wstr[idx]].yOff;
            }
        }
    } else {
        UNIMPLEMENTED;
#if 0
        OUTDATED (need to merge 47289 and higher)
        XImage *image;
        int image_x, image_y, image_off_x, image_off_y, image_w, image_h;
        RECT extents = {0, 0, 0, 0};
        POINT cur = {0, 0};
        int w = physDev->drawable_rect.right - physDev->drawable_rect.left;
        int h = physDev->drawable_rect.bottom - physDev->drawable_rect.top;

        TRACE("drawable %dx%d\n", w, h);

        for(idx = 0; idx < count; idx++) {
            if(extents.left > cur.x - formatEntry->gis[wstr[idx]].x)
                extents.left = cur.x - formatEntry->gis[wstr[idx]].x;
            if(extents.top > cur.y - formatEntry->gis[wstr[idx]].y)
                extents.top = cur.y - formatEntry->gis[wstr[idx]].y;
            if(extents.right < cur.x - formatEntry->gis[wstr[idx]].x + formatEntry->gis[wstr[idx]].width)
                extents.right = cur.x - formatEntry->gis[wstr[idx]].x + formatEntry->gis[wstr[idx]].width;
            if(extents.bottom < cur.y - formatEntry->gis[wstr[idx]].y + formatEntry->gis[wstr[idx]].height)
                extents.bottom = cur.y - formatEntry->gis[wstr[idx]].y + formatEntry->gis[wstr[idx]].height;
            if(lpDx) {
                offset += lpDx[idx];
                cur.x = offset * cosEsc;
                cur.y = offset * -sinEsc;
            } else {
                cur.x += formatEntry->gis[wstr[idx]].xOff;
                cur.y += formatEntry->gis[wstr[idx]].yOff;
            }
        }
        TRACE("glyph extents %d,%d - %d,%d drawable x,y %d,%d\n", extents.left, extents.top,
            extents.right, extents.bottom, physDev->dc_rect.left + x, physDev->dc_rect.top + y);

        if(physDev->dc_rect.left + x + extents.left >= 0) {
            image_x = physDev->dc_rect.left + x + extents.left;
            image_off_x = 0;
        } else {
            image_x = 0;
            image_off_x = physDev->dc_rect.left + x + extents.left;
        }
        if(physDev->dc_rect.top + y + extents.top >= 0) {
            image_y = physDev->dc_rect.top + y + extents.top;
            image_off_y = 0;
        } else {
            image_y = 0;
            image_off_y = physDev->dc_rect.top + y + extents.top;
        }
        if(physDev->dc_rect.left + x + extents.right < w)
            image_w = physDev->dc_rect.left + x + extents.right - image_x;
        else
            image_w = w - image_x;
        if(physDev->dc_rect.top + y + extents.bottom < h)
            image_h = physDev->dc_rect.top + y + extents.bottom - image_y;
        else
            image_h = h - image_y;

        if(image_w <= 0 || image_h <= 0) goto no_image;

        X11DRV_expect_error(gdi_display, XRenderErrorHandler, NULL);
        image = XGetImage(gdi_display, physDev->drawable,
            image_x, image_y, image_w, image_h,
            AllPlanes, ZPixmap);
        X11DRV_check_error();

        TRACE("XGetImage(%p, %x, %d, %d, %d, %d, %lx, %x) depth = %d rets %p\n",
            gdi_display, (int)physDev->drawable, image_x, image_y,
            image_w, image_h, AllPlanes, ZPixmap,
            physDev->depth, image);
        if(!image) {
            Pixmap xpm = XCreatePixmap(gdi_display, root_window, image_w, image_h,
                physDev->depth);
            GC gc;
            XGCValues gcv;

            gcv.graphics_exposures = False;
            gc = XCreateGC(gdi_display, xpm, GCGraphicsExposures, &gcv);
            XCopyArea(gdi_display, physDev->drawable, xpm, gc, image_x, image_y,
                image_w, image_h, 0, 0);
            XFreeGC(gdi_display, gc);
            X11DRV_expect_error(gdi_display, XRenderErrorHandler, NULL);
            image = XGetImage(gdi_display, xpm, 0, 0, image_w, image_h, AllPlanes,
                ZPixmap);
            X11DRV_check_error();
            XFreePixmap(gdi_display, xpm);
        }
        if(!image) goto no_image;

        image->red_mask = visual->red_mask;
        image->green_mask = visual->green_mask;
        image->blue_mask = visual->blue_mask;

        offset = xoff = yoff = 0;
        for(idx = 0; idx < count; idx++) {
            SmoothGlyphGray(image, xoff + image_off_x - extents.left,
                yoff + image_off_y - extents.top,
                formatEntry->bitmaps[wstr[idx]],
                &formatEntry->gis[wstr[idx]],
                physDev->textPixel);
            if(lpDx) {
                offset += lpDx[idx];
                xoff = offset * cosEsc;
                yoff = offset * -sinEsc;
            } else {
                xoff += formatEntry->gis[wstr[idx]].xOff;
                yoff += formatEntry->gis[wstr[idx]].yOff;
            }
        }
        XPutImage(gdi_display, physDev->drawable, physDev->gc, image, 0, 0,
            image_x, image_y, image_w, image_h);
        XDestroyImage(image);
#endif
    }
//no_image:

    //Cleanup the temporary pen
    EBRUSHOBJ_vCleanup(&pTextPen);
    BRUSH_ShareUnlockBrush(ppen);
    GreDeleteObject(hpen);
}

/* EOF */
