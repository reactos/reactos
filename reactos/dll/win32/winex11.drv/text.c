/*
 * X11 graphics driver text functions
 *
 * Copyright 1993,1994 Alexandre Julliard
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "x11font.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(text);

#define IROUND(x) (int)((x)>0? (x)+0.5 : (x) - 0.5)


/***********************************************************************
 *           X11DRV_ExtTextOut
 */
BOOL CDECL
X11DRV_ExtTextOut( X11DRV_PDEVICE *physDev, INT x, INT y, UINT flags,
                   const RECT *lprect, LPCWSTR wstr, UINT count,
                   const INT *lpDx )
{
    unsigned int i;
    fontObject*		pfo;
    XFontStruct*	font;
    BOOL		rotated = FALSE;
    XChar2b		*str2b = NULL;
    BOOL		dibUpdateFlag = FALSE;
    BOOL                result = TRUE;
    HRGN                saved_region = 0;

    if(physDev->has_gdi_font)
        return X11DRV_XRender_ExtTextOut(physDev, x, y, flags, lprect, wstr, count, lpDx);

    if (!X11DRV_SetupGCForText( physDev )) return TRUE;

    pfo = XFONT_GetFontObject( physDev->font );
    font = pfo->fs;

    if (pfo->lf.lfEscapement && pfo->lpX11Trans)
        rotated = TRUE;

    TRACE("hdc=%p df=%04x %d,%d rc %s %s, %d  flags=%d lpDx=%p\n",
          physDev->hdc, (UINT16)physDev->font, x, y, wine_dbgstr_rect(lprect),
	  debugstr_wn (wstr, count), count, flags, lpDx);

      /* Draw the rectangle */

    if (flags & ETO_OPAQUE)
    {
        X11DRV_LockDIBSection( physDev, DIB_Status_GdiMod );
        dibUpdateFlag = TRUE;
        wine_tsx11_lock();
        XSetForeground( gdi_display, physDev->gc, physDev->backgroundPixel );
        XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                        physDev->dc_rect.left + lprect->left, physDev->dc_rect.top + lprect->top,
                        lprect->right - lprect->left, lprect->bottom - lprect->top );
        wine_tsx11_unlock();
    }
    if (!count) goto END;  /* Nothing more to do */


      /* Set the clip region */

    if (flags & ETO_CLIPPED)
    {
        HRGN clip_region;

        clip_region = CreateRectRgnIndirect( lprect );
        /* make a copy of the current device region */
        saved_region = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( saved_region, physDev->region, 0, RGN_COPY );
        X11DRV_SetDeviceClipping( physDev, saved_region, clip_region );
        DeleteObject( clip_region );
    }

      /* Draw the text background if necessary */

    if (!dibUpdateFlag)
    {
        X11DRV_LockDIBSection( physDev, DIB_Status_GdiMod );
        dibUpdateFlag = TRUE;
    }


    /* Draw the text (count > 0 verified) */
    if (!(str2b = X11DRV_cptable[pfo->fi->cptable].punicode_to_char2b( pfo, wstr, count )))
        goto FAIL;

    wine_tsx11_lock();
    XSetForeground( gdi_display, physDev->gc, physDev->textPixel );
    wine_tsx11_unlock();
    if(!rotated)
    {
        if (!lpDx)
        {
            X11DRV_cptable[pfo->fi->cptable].pDrawString(
                           pfo, gdi_display, physDev->drawable, physDev->gc,
                           physDev->dc_rect.left + x, physDev->dc_rect.top + y, str2b, count );
        }
        else
        {
            XTextItem16 *items;

            items = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XTextItem16) );
            if(items == NULL) goto FAIL;

            items[0].chars = str2b;
            items[0].delta = 0;
            items[0].nchars = 1;
            items[0].font = None;
            for(i = 1; i < count; i++)
            {
                items[i].chars  = str2b + i;
                items[i].delta  = (flags & ETO_PDY) ? lpDx[(i - 1) * 2] : lpDx[i - 1];
                items[i].delta -= X11DRV_cptable[pfo->fi->cptable].pTextWidth( pfo, str2b + i - 1, 1 );
                items[i].nchars = 1;
                items[i].font   = None;
            }
            X11DRV_cptable[pfo->fi->cptable].pDrawText( pfo, gdi_display,
                                  physDev->drawable, physDev->gc,
                                  physDev->dc_rect.left + x, physDev->dc_rect.top + y, items, count );
            HeapFree( GetProcessHeap(), 0, items );
        }
    }
    else /* rotated */
    {
        /* have to render character by character. */
        double offset = 0.0;
        UINT i;

        for (i=0; i<count; i++)
        {
            int char_metric_offset = str2b[i].byte2 + (str2b[i].byte1 << 8)
                - font->min_char_or_byte2;
            int x_i = IROUND((double) (physDev->dc_rect.left + x) + offset *
                             pfo->lpX11Trans->a / pfo->lpX11Trans->pixelsize );
            int y_i = IROUND((double) (physDev->dc_rect.top + y) - offset *
                             pfo->lpX11Trans->b / pfo->lpX11Trans->pixelsize );

            X11DRV_cptable[pfo->fi->cptable].pDrawString(
                                    pfo, gdi_display, physDev->drawable, physDev->gc,
                                    x_i, y_i, &str2b[i], 1);
            if (lpDx)
            {
                offset += (flags & ETO_PDY) ? lpDx[i * 2] : lpDx[i];
            }
            else
            {
                offset += (double) (font->per_char ?
                                    font->per_char[char_metric_offset].attributes:
                                    font->min_bounds.attributes)
                    * pfo->lpX11Trans->pixelsize / 1000.0;
            }
        }
    }
    HeapFree( GetProcessHeap(), 0, str2b );

    if (flags & ETO_CLIPPED)
    {
        /* restore the device region */
        X11DRV_SetDeviceClipping( physDev, saved_region, 0 );
        DeleteObject( saved_region );
    }
    goto END;

FAIL:
    HeapFree( GetProcessHeap(), 0, str2b );
    result = FALSE;

END:
    if (dibUpdateFlag) X11DRV_UnlockDIBSection( physDev, TRUE );
    return result;
}


/***********************************************************************
 *           X11DRV_GetTextExtentExPoint
 */
BOOL CDECL X11DRV_GetTextExtentExPoint( X11DRV_PDEVICE *physDev, LPCWSTR str, INT count,
                                        INT maxExt, LPINT lpnFit, LPINT alpDx, LPSIZE size )
{
    fontObject* pfo = XFONT_GetFontObject( physDev->font );

    TRACE("%s %d\n", debugstr_wn(str,count), count);
    if( pfo ) {
	XChar2b *p = X11DRV_cptable[pfo->fi->cptable].punicode_to_char2b( pfo, str, count );
	if (!p) return FALSE;
        if( !pfo->lpX11Trans ) {
	    int dir, ascent, descent;
	    int info_width;
	    X11DRV_cptable[pfo->fi->cptable].pTextExtents( pfo, p,
				count, &dir, &ascent, &descent, &info_width,
				maxExt, lpnFit, alpDx );

          size->cx = info_width;
          size->cy = pfo->fs->ascent + pfo->fs->descent;
	} else {
	    INT i;
	    INT nfit = 0;
	    float x = 0.0, y = 0.0;
	    float scaled_x = 0.0, pixsize = pfo->lpX11Trans->pixelsize;
	    /* FIXME: Deal with *_char_or_byte2 != 0 situations */
	    for(i = 0; i < count; i++) {
	        x += pfo->fs->per_char ?
	   pfo->fs->per_char[p[i].byte2 - pfo->fs->min_char_or_byte2].attributes :
	   pfo->fs->min_bounds.attributes;
	        scaled_x = x * pixsize / 1000.0;
	        if (alpDx)
	            alpDx[i] = scaled_x;
	        if (scaled_x <= maxExt)
	            ++nfit;
	    }
	    y = pfo->lpX11Trans->RAW_ASCENT + pfo->lpX11Trans->RAW_DESCENT;
	    TRACE("x = %f y = %f\n", x, y);
	    size->cx = x * pfo->lpX11Trans->pixelsize / 1000.0;
	    size->cy = y * pfo->lpX11Trans->pixelsize / 1000.0;
	    if (lpnFit)
	        *lpnFit = nfit;
	}
	size->cx *= pfo->rescale;
	size->cy *= pfo->rescale;
	HeapFree( GetProcessHeap(), 0, p );
	return TRUE;
    }
    return FALSE;
}
