/*
 * 2D Surface implementation without OpenGL
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Stefan Dösinger
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
#include "wine/port.h"
#include "wined3d_private.h"

#include <assert.h>
#include <stdio.h>

/* Use the d3d_surface debug channel to have one channel for all surfaces */
WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);
WINE_DECLARE_DEBUG_CHANNEL(fps);

/*****************************************************************************
 * x11_copy_to_screen
 *
 * Helper function that blts the front buffer contents to the target window
 *
 * Params:
 *  This: Surface to copy from
 *  rc: Rectangle to copy
 *
 *****************************************************************************/
static void
x11_copy_to_screen(IWineD3DSurfaceImpl *This,
                   LPRECT rc)
{
    if(This->resource.usage & WINED3DUSAGE_RENDERTARGET)
    {
        POINT offset = {0,0};
        HWND hDisplayWnd;
        HDC hDisplayDC;
        HDC hSurfaceDC = 0;
        RECT drawrect;
        TRACE("(%p)->(%p): Copying to screen\n", This, rc);

        hSurfaceDC = This->hDC;

        hDisplayWnd = This->resource.wineD3DDevice->ddraw_window;
        hDisplayDC = GetDCEx(hDisplayWnd, 0, DCX_CLIPSIBLINGS|DCX_CACHE);
        if(rc)
        {
            TRACE(" copying rect (%d,%d)->(%d,%d), offset (%d,%d)\n",
            rc->left, rc->top, rc->right, rc->bottom, offset.x, offset.y);
        }

        /* Front buffer coordinates are screen coordinates. Map them to the destination
         * window if not fullscreened
         */
        if(!This->resource.wineD3DDevice->ddraw_fullscreen) {
            ClientToScreen(hDisplayWnd, &offset);
        }
#if 0
        /* FIXME: this doesn't work... if users really want to run
        * X in 8bpp, then we need to call directly into display.drv
        * (or Wine's equivalent), and force a private colormap
        * without default entries. */
        if (This->palette) {
            SelectPalette(hDisplayDC, This->palette->hpal, FALSE);
            RealizePalette(hDisplayDC); /* sends messages => deadlocks */
        }
#endif
        drawrect.left	= 0;
        drawrect.right	= This->currentDesc.Width;
        drawrect.top	= 0;
        drawrect.bottom	= This->currentDesc.Height;

#if 0
        /* TODO: Support clippers */
        if (This->clipper)
        {
            RECT xrc;
            HWND hwnd = ((IWineD3DClipperImpl *) This->clipper)->hWnd;
            if (hwnd && GetClientRect(hwnd,&xrc))
            {
                OffsetRect(&xrc,offset.x,offset.y);
                IntersectRect(&drawrect,&drawrect,&xrc);
            }
        }
#endif
        if (rc)
        {
            IntersectRect(&drawrect,&drawrect,rc);
        }
        else
        {
            /* Only use this if the caller did not pass a rectangle, since
             * due to double locking this could be the wrong one ...
             */
            if (This->lockedRect.left != This->lockedRect.right)
            {
                IntersectRect(&drawrect,&drawrect,&This->lockedRect);
            }
        }

        BitBlt(hDisplayDC,
               drawrect.left-offset.x, drawrect.top-offset.y,
               drawrect.right-drawrect.left, drawrect.bottom-drawrect.top,
               hSurfaceDC,
               drawrect.left, drawrect.top,
               SRCCOPY);
        ReleaseDC(hDisplayWnd, hDisplayDC);
    }
}

/*****************************************************************************
 * IWineD3DSurface::PreLoad, GDI version
 *
 * This call is unsupported on GDI surfaces, if it's called something went
 * wrong in the parent library. Write an informative warning
 *
 *****************************************************************************/
static void WINAPI
IWineGDISurfaceImpl_PreLoad(IWineD3DSurface *iface)
{
    ERR("(%p): PreLoad is not supported on X11 surfaces!\n", iface);
    ERR("(%p): Most likely the parent library did something wrong.\n", iface);
    ERR("(%p): Please report to wine-devel\n", iface);
}

/*****************************************************************************
 * IWineD3DSurface::LockRect, GDI version
 *
 * Locks the surface and returns a pointer to the surface memory
 *
 * Params:
 *  pLockedRect: Address to return the locking info at
 *  pRect: Rectangle to lock
 *  Flags: Some flags
 *
 * Returns:
 *  WINED3D_OK on success
 *  WINED3DERR_INVALIDCALL on errors
 *
 *****************************************************************************/
static HRESULT WINAPI
IWineGDISurfaceImpl_LockRect(IWineD3DSurface *iface,
                             WINED3DLOCKED_RECT* pLockedRect,
                             CONST RECT* pRect,
                             DWORD Flags)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    /* Already locked? */
    if(This->Flags & SFLAG_LOCKED)
    {
        ERR("(%p) Surface already locked\n", This);
        /* What should I return here? */
        return WINED3DERR_INVALIDCALL;
    }

    if (!(This->Flags & SFLAG_LOCKABLE))
    {
        /* This is some GL specific thing, see the OpenGL version of
         * this method, but check for the flag and write a trace
         */
        TRACE("Warning: trying to lock unlockable surf@%p\n", This);
    }

    TRACE("(%p) : rect@%p flags(%08x), output lockedRect@%p, memory@%p\n",
          This, pRect, Flags, pLockedRect, This->resource.allocatedMemory);

    if(!This->resource.allocatedMemory) {
        HDC hdc;
        HRESULT hr;
        /* This happens on gdi surfaces if the application set a user pointer and resets it.
         * Recreate the DIB section
         */
        hr = IWineD3DSurface_GetDC(iface, &hdc);  /* will recursively call lockrect, do not set the LOCKED flag to this line */
        if(hr != WINED3D_OK) return hr;
        hr = IWineD3DSurface_ReleaseDC(iface, hdc);
        if(hr != WINED3D_OK) return hr;
    }

    pLockedRect->Pitch = IWineD3DSurface_GetPitch(iface);

    if (NULL == pRect)
    {
        pLockedRect->pBits = This->resource.allocatedMemory;
        This->lockedRect.left   = 0;
        This->lockedRect.top    = 0;
        This->lockedRect.right  = This->currentDesc.Width;
        This->lockedRect.bottom = This->currentDesc.Height;

        TRACE("Locked Rect (%p) = l %d, t %d, r %d, b %d\n",
        &This->lockedRect, This->lockedRect.left, This->lockedRect.top,
        This->lockedRect.right, This->lockedRect.bottom);
    }
    else
    {
        TRACE("Lock Rect (%p) = l %d, t %d, r %d, b %d\n",
              pRect, pRect->left, pRect->top, pRect->right, pRect->bottom);

        if (This->resource.format == WINED3DFMT_DXT1)
        {
            /* DXT1 is half byte per pixel */
            pLockedRect->pBits = This->resource.allocatedMemory +
                                  (pLockedRect->Pitch * pRect->top) +
                                  ((pRect->left * This->bytesPerPixel / 2));
        }
        else
        {
            pLockedRect->pBits = This->resource.allocatedMemory +
                                 (pLockedRect->Pitch * pRect->top) +
                                 (pRect->left * This->bytesPerPixel);
        }
        This->lockedRect.left   = pRect->left;
        This->lockedRect.top    = pRect->top;
        This->lockedRect.right  = pRect->right;
        This->lockedRect.bottom = pRect->bottom;
    }

    /* No dirtifying is needed for this surface implementation */
    TRACE("returning memory@%p, pitch(%d)\n", pLockedRect->pBits, pLockedRect->Pitch);

    This->Flags |= SFLAG_LOCKED;
    return WINED3D_OK;
}

/*****************************************************************************
 * IWineD3DSurface::UnlockRect, GDI version
 *
 * Unlocks a surface. This implementation doesn't do much, except updating
 * the window if the front buffer is unlocked
 *
 * Returns:
 *  WINED3D_OK on success
 *  WINED3DERR_INVALIDCALL on failure
 *
 *****************************************************************************/
static HRESULT WINAPI
IWineGDISurfaceImpl_UnlockRect(IWineD3DSurface *iface)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DDeviceImpl *dev = (IWineD3DDeviceImpl *) This->resource.wineD3DDevice;
    TRACE("(%p)\n", This);

    if (!(This->Flags & SFLAG_LOCKED))
    {
        WARN("trying to Unlock an unlocked surf@%p\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    /* Can be useful for debugging */
#if 0
        {
            static unsigned int gen = 0;
            char buffer[4096];
            ++gen;
            if ((gen % 10) == 0) {
                snprintf(buffer, sizeof(buffer), "/tmp/surface%p_type%u_level%u_%u.ppm", This, This->glDescription.target, This->glDescription.level, gen);
                IWineD3DSurfaceImpl_SaveSnapshot(iface, buffer);
            }
            /*
             * debugging crash code
            if (gen == 250) {
              void** test = NULL;
              *test = 0;
            }
            */
        }
#endif

    /* Update the screen */
    if(This == (IWineD3DSurfaceImpl *) dev->ddraw_primary)
    {
        x11_copy_to_screen(This, &This->lockedRect);
    }

    This->Flags &= ~SFLAG_LOCKED;
    memset(&This->lockedRect, 0, sizeof(RECT));
    return WINED3D_OK;
}

/*****************************************************************************
 * IWineD3DSurface::Flip, GDI version
 *
 * Flips 2 flipping enabled surfaces. Determining the 2 targets is done by
 * the parent library. This implementation changes the data pointers of the
 * surfaces and copies the new front buffer content to the screen
 *
 * Params:
 *  override: Flipping target(e.g. back buffer)
 *
 * Returns:
 *  WINED3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IWineGDISurfaceImpl_Flip(IWineD3DSurface *iface,
                         IWineD3DSurface *override,
                         DWORD Flags)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Target = (IWineD3DSurfaceImpl *) override;
    TRACE("(%p)->(%p,%x)\n", This, override, Flags);

    TRACE("(%p) Flipping to surface %p\n", This, Target);

    if(Target == NULL)
    {
        ERR("(%p): Can't flip without a target\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    /* Flip the DC */
    {
        HDC tmp;
        tmp = This->hDC;
        This->hDC = Target->hDC;
        Target->hDC = tmp;
    }

    /* Flip the DIBsection */
    {
        HBITMAP tmp;
        tmp = This->dib.DIBsection;
        This->dib.DIBsection = Target->dib.DIBsection;
        Target->dib.DIBsection = tmp;
    }

    /* Flip the surface data */
    {
        void* tmp;

        tmp = This->dib.bitmap_data;
        This->dib.bitmap_data = Target->dib.bitmap_data;
        Target->dib.bitmap_data = tmp;

        tmp = This->resource.allocatedMemory;
        This->resource.allocatedMemory = Target->resource.allocatedMemory;
        Target->resource.allocatedMemory = tmp;
    }

    /* client_memory should not be different, but just in case */
    {
        BOOL tmp;
        tmp = This->dib.client_memory;
        This->dib.client_memory = Target->dib.client_memory;
        Target->dib.client_memory = tmp;
    }

    /* Useful for debugging */
#if 0
        {
            static unsigned int gen = 0;
            char buffer[4096];
            ++gen;
            if ((gen % 10) == 0) {
                snprintf(buffer, sizeof(buffer), "/tmp/surface%p_type%u_level%u_%u.ppm", This, This->glDescription.target, This->glDescription.level, gen);
                IWineD3DSurfaceImpl_SaveSnapshot(iface, buffer);
            }
            /*
             * debugging crash code
            if (gen == 250) {
              void** test = NULL;
              *test = 0;
            }
            */
        }
#endif

    /* Update the screen */
    x11_copy_to_screen(This, NULL);

    /* FPS support */
    if (TRACE_ON(fps))
    {
        static long prev_time, frames;

        DWORD time = GetTickCount();
        frames++;
        /* every 1.5 seconds */
        if (time - prev_time > 1500) {
            TRACE_(fps)("@ approx %.2ffps\n", 1000.0*frames/(time - prev_time));
            prev_time = time;
            frames = 0;
        }
    }

    return WINED3D_OK;
}

/*****************************************************************************
 * _Blt_ColorFill
 *
 * Helper function that fills a memory area with a specific color
 *
 * Params:
 *  buf: memory address to start filling at
 *  width, height: Dimensions of the area to fill
 *  bpp: Bit depth of the surface
 *  lPitch: pitch of the surface
 *  color: Color to fill with
 *
 *****************************************************************************/
static HRESULT
_Blt_ColorFill(BYTE *buf,
               int width, int height,
               int bpp, LONG lPitch,
               DWORD color)
{
    int x, y;
    LPBYTE first;

    /* Do first row */

#define COLORFILL_ROW(type) \
{ \
    type *d = (type *) buf; \
    for (x = 0; x < width; x++) \
	d[x] = (type) color; \
    break; \
}
    switch(bpp)
    {
        case 1: COLORFILL_ROW(BYTE)
        case 2: COLORFILL_ROW(WORD)
        case 3:
        {
            BYTE *d = (BYTE *) buf;
            for (x = 0; x < width; x++,d+=3)
            {
                d[0] = (color    ) & 0xFF;
                d[1] = (color>> 8) & 0xFF;
                d[2] = (color>>16) & 0xFF;
            }
            break;
        }
        case 4: COLORFILL_ROW(DWORD)
        default:
            FIXME("Color fill not implemented for bpp %d!\n", bpp*8);
            return WINED3DERR_NOTAVAILABLE;
    }

#undef COLORFILL_ROW

    /* Now copy first row */
    first = buf;
    for (y = 1; y < height; y++)
    {
        buf += lPitch;
        memcpy(buf, first, width * bpp);
    }
    return WINED3D_OK;
}

/*****************************************************************************
 * IWineD3DSurface::Blt, GDI version
 *
 * Performs blits to a surface, eigher from a source of source-less blts
 * This is the main functionality of DirectDraw
 *
 * Params:
 *  DestRect: Destination rectangle to write to
 *  SrcSurface: Source surface, can be NULL
 *  SrcRect: Source rectangle
 *****************************************************************************/
HRESULT WINAPI
IWineGDISurfaceImpl_Blt(IWineD3DSurface *iface,
                        RECT *DestRect,
                        IWineD3DSurface *SrcSurface,
                        RECT *SrcRect,
                        DWORD Flags,
                        WINEDDBLTFX *DDBltFx,
                        WINED3DTEXTUREFILTERTYPE Filter)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;
    RECT		xdst,xsrc;
    HRESULT		ret = WINED3D_OK;
    WINED3DLOCKED_RECT  dlock, slock;
    WINED3DFORMAT       dfmt = WINED3DFMT_UNKNOWN, sfmt = WINED3DFMT_UNKNOWN;
    int bpp, srcheight, srcwidth, dstheight, dstwidth, width;
    int x, y;
    const StaticPixelFormatDesc *sEntry, *dEntry;
    LPBYTE dbuf, sbuf;
    TRACE("(%p)->(%p,%p,%p,%x,%p)\n", This, DestRect, Src, SrcRect, Flags, DDBltFx);

    if (TRACE_ON(d3d_surface))
    {
        if (DestRect) TRACE("\tdestrect :%dx%d-%dx%d\n",
        DestRect->left, DestRect->top, DestRect->right, DestRect->bottom);
        if (SrcRect) TRACE("\tsrcrect  :%dx%d-%dx%d\n",
        SrcRect->left, SrcRect->top, SrcRect->right, SrcRect->bottom);
#if 0
        TRACE("\tflags: ");
        DDRAW_dump_DDBLT(Flags);
        if (Flags & WINEDDBLT_DDFX)
        {
            TRACE("\tblitfx: ");
            DDRAW_dump_DDBLTFX(DDBltFx->dwDDFX);
        }
#endif
    }

    if ( (This->Flags & SFLAG_LOCKED) || ((Src != NULL) && (Src->Flags & SFLAG_LOCKED)))
    {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

    if(Filter != WINED3DTEXF_NONE && Filter != WINED3DTEXF_POINT) {
        /* Can happen when d3d9 apps do a StretchRect call which isn't handled in gl */
        FIXME("Filters not supported in software blit\n");
    }

    if (Src == This)
    {
        IWineD3DSurface_LockRect(iface, &dlock, NULL, 0);
        dfmt = This->resource.format;
        slock = dlock;
        sfmt = dfmt;
        sEntry = getFormatDescEntry(sfmt, NULL, NULL);
        dEntry = sEntry;
    }
    else
    {
        if (Src)
        {
            IWineD3DSurface_LockRect(SrcSurface, &slock, NULL, WINED3DLOCK_READONLY);
            sfmt = Src->resource.format;
        }
        sEntry = getFormatDescEntry(sfmt, NULL, NULL);
        dfmt = This->resource.format;
        dEntry = getFormatDescEntry(dfmt, NULL, NULL);
        IWineD3DSurface_LockRect(iface, &dlock,NULL,0);
    }

    if (!DDBltFx || !(DDBltFx->dwDDFX)) Flags &= ~WINEDDBLT_DDFX;

    if (sEntry->isFourcc && dEntry->isFourcc)
    {
        if (sfmt != dfmt)
        {
            FIXME("FOURCC->FOURCC copy only supported for the same type of surface\n");
            ret = WINED3DERR_WRONGTEXTUREFORMAT;
            goto release;
        }
        memcpy(dlock.pBits, slock.pBits, This->resource.size);
        goto release;
    }

    if (sEntry->isFourcc && !dEntry->isFourcc)
    {
        FIXME("DXTC decompression not supported right now\n");
        goto release;
    }

    if (DestRect)
    {
        memcpy(&xdst,DestRect,sizeof(xdst));
    }
    else
    {
        xdst.top	= 0;
        xdst.bottom	= This->currentDesc.Height;
        xdst.left	= 0;
        xdst.right	= This->currentDesc.Width;
    }

    if (SrcRect)
    {
        memcpy(&xsrc,SrcRect,sizeof(xsrc));
    }
    else
    {
        if (Src)
        {
            xsrc.top	= 0;
            xsrc.bottom	= Src->currentDesc.Height;
            xsrc.left	= 0;
            xsrc.right	= Src->currentDesc.Width;
        }
        else
        {
            memset(&xsrc,0,sizeof(xsrc));
        }
    }

    /* First check for the validity of source / destination rectangles. This was
      verified using a test application + by MSDN.
    */
    if ((Src != NULL) &&
        ((xsrc.bottom > Src->currentDesc.Height) || (xsrc.bottom < 0) ||
        (xsrc.top     > Src->currentDesc.Height) || (xsrc.top    < 0) ||
        (xsrc.left    > Src->currentDesc.Width)  || (xsrc.left   < 0) ||
        (xsrc.right   > Src->currentDesc.Width)  || (xsrc.right  < 0) ||
        (xsrc.right   < xsrc.left)               || (xsrc.bottom < xsrc.top)))
    {
        WARN("Application gave us bad source rectangle for Blt.\n");
        ret = WINEDDERR_INVALIDRECT;
        goto release;
    }
    /* For the Destination rect, it can be out of bounds on the condition that a clipper
      is set for the given surface.
    */
    if ((/*This->clipper == NULL*/ TRUE) &&
        ((xdst.bottom  > This->currentDesc.Height) || (xdst.bottom < 0) ||
        (xdst.top      > This->currentDesc.Height) || (xdst.top    < 0) ||
        (xdst.left     > This->currentDesc.Width)  || (xdst.left   < 0) ||
        (xdst.right    > This->currentDesc.Width)  || (xdst.right  < 0) ||
        (xdst.right    < xdst.left)                || (xdst.bottom < xdst.top)))
    {
        WARN("Application gave us bad destination rectangle for Blt without a clipper set.\n");
        ret = WINEDDERR_INVALIDRECT;
        goto release;
    }

    /* Now handle negative values in the rectangles. Warning: only supported for now
      in the 'simple' cases (ie not in any stretching / rotation cases).

      First, the case where nothing is to be done.
    */
    if (((xdst.bottom <= 0) || (xdst.right <= 0)         ||
         (xdst.top    >= (int) This->currentDesc.Height) ||
         (xdst.left   >= (int) This->currentDesc.Width)) ||
        ((Src != NULL) &&
        ((xsrc.bottom <= 0) || (xsrc.right <= 0)     ||
         (xsrc.top >= (int) Src->currentDesc.Height) ||
         (xsrc.left >= (int) Src->currentDesc.Width))  ))
    {
        TRACE("Nothing to be done !\n");
        goto release;
    }

    /* The easy case : the source-less blits.... */
    if (Src == NULL)
    {
        RECT full_rect;
        RECT temp_rect; /* No idea if intersect rect can be the same as one of the source rect */

        full_rect.left   = 0;
        full_rect.top    = 0;
        full_rect.right  = This->currentDesc.Width;
        full_rect.bottom = This->currentDesc.Height;
        IntersectRect(&temp_rect, &full_rect, &xdst);
        xdst = temp_rect;
    }
    else
    {
        /* Only handle clipping on the destination rectangle */
        int clip_horiz = (xdst.left < 0) || (xdst.right  > (int) This->currentDesc.Width );
        int clip_vert  = (xdst.top  < 0) || (xdst.bottom > (int) This->currentDesc.Height);
        if (clip_vert || clip_horiz)
        {
            /* Now check if this is a special case or not... */
            if ((((xdst.bottom - xdst.top ) != (xsrc.bottom - xsrc.top )) && clip_vert ) ||
                (((xdst.right  - xdst.left) != (xsrc.right  - xsrc.left)) && clip_horiz) ||
                (Flags & WINEDDBLT_DDFX))
            {
                WARN("Out of screen rectangle in special case. Not handled right now.\n");
                goto release;
            }

            if (clip_horiz)
            {
                if (xdst.left < 0) { xsrc.left -= xdst.left; xdst.left = 0; }
                if (xdst.right > This->currentDesc.Width)
                {
                    xsrc.right -= (xdst.right - (int) This->currentDesc.Width);
                    xdst.right = (int) This->currentDesc.Width;
                }
            }
            if (clip_vert)
            {
                if (xdst.top < 0)
                {
                    xsrc.top -= xdst.top;
                    xdst.top = 0;
                }
                if (xdst.bottom > This->currentDesc.Height)
                {
                    xsrc.bottom -= (xdst.bottom - (int) This->currentDesc.Height);
                    xdst.bottom = (int) This->currentDesc.Height;
                }
            }
            /* And check if after clipping something is still to be done... */
            if ((xdst.bottom <= 0)   || (xdst.right <= 0)       ||
                (xdst.top   >= (int) This->currentDesc.Height)  ||
                (xdst.left  >= (int) This->currentDesc.Width)   ||
                (xsrc.bottom <= 0)   || (xsrc.right <= 0)       ||
                (xsrc.top >= (int) Src->currentDesc.Height)     ||
                (xsrc.left >= (int) Src->currentDesc.Width))
            {
                TRACE("Nothing to be done after clipping !\n");
                goto release;
            }
        }
    }

    bpp = This->bytesPerPixel;
    srcheight = xsrc.bottom - xsrc.top;
    srcwidth = xsrc.right - xsrc.left;
    dstheight = xdst.bottom - xdst.top;
    dstwidth = xdst.right - xdst.left;
    width = (xdst.right - xdst.left) * bpp;

    assert(width <= dlock.Pitch);

    dbuf = (BYTE*)dlock.pBits+(xdst.top*dlock.Pitch)+(xdst.left*bpp);

    if (Flags & WINEDDBLT_WAIT)
    {
        Flags &= ~WINEDDBLT_WAIT;
    }
    if (Flags & WINEDDBLT_ASYNC)
    {
        static BOOL displayed = FALSE;
        if (!displayed)
            FIXME("Can't handle WINEDDBLT_ASYNC flag right now.\n");
        displayed = TRUE;
        Flags &= ~WINEDDBLT_ASYNC;
    }
    if (Flags & WINEDDBLT_DONOTWAIT)
    {
        /* WINEDDBLT_DONOTWAIT appeared in DX7 */
        static BOOL displayed = FALSE;
        if (!displayed)
            FIXME("Can't handle WINEDDBLT_DONOTWAIT flag right now.\n");
        displayed = TRUE;
        Flags &= ~WINEDDBLT_DONOTWAIT;
    }

    /* First, all the 'source-less' blits */
    if (Flags & WINEDDBLT_COLORFILL)
    {
        ret = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp,
                            dlock.Pitch, DDBltFx->u5.dwFillColor);
        Flags &= ~WINEDDBLT_COLORFILL;
    }

    if (Flags & WINEDDBLT_DEPTHFILL)
    {
        FIXME("DDBLT_DEPTHFILL needs to be implemented!\n");
    }
    if (Flags & WINEDDBLT_ROP)
    {
        /* Catch some degenerate cases here */
        switch(DDBltFx->dwROP)
        {
            case BLACKNESS:
                ret = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,dlock.Pitch,0);
                break;
            case 0xAA0029: /* No-op */
                break;
            case WHITENESS:
                ret = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,dlock.Pitch,~0);
                break;
            case SRCCOPY: /* well, we do that below ? */
                break;
            default:
                FIXME("Unsupported raster op: %08x  Pattern: %p\n", DDBltFx->dwROP, DDBltFx->u5.lpDDSPattern);
                goto error;
        }
        Flags &= ~WINEDDBLT_ROP;
    }
    if (Flags & WINEDDBLT_DDROPS)
    {
        FIXME("\tDdraw Raster Ops: %08x  Pattern: %p\n", DDBltFx->dwDDROP, DDBltFx->u5.lpDDSPattern);
    }
    /* Now the 'with source' blits */
    if (Src)
    {
        LPBYTE sbase;
        int sx, xinc, sy, yinc;

        if (!dstwidth || !dstheight) /* hmm... stupid program ? */
            goto release;
        sbase = (BYTE*)slock.pBits+(xsrc.top*slock.Pitch)+xsrc.left*bpp;
        xinc = (srcwidth << 16) / dstwidth;
        yinc = (srcheight << 16) / dstheight;

        if (!Flags)
        {
            /* No effects, we can cheat here */
            if (dstwidth == srcwidth)
            {
                if (dstheight == srcheight)
                {
                    /* No stretching in either direction. This needs to be as
                    * fast as possible */
                    sbuf = sbase;

                    /* check for overlapping surfaces */
                    if (SrcSurface != iface || xdst.top < xsrc.top ||
                        xdst.right <= xsrc.left || xsrc.right <= xdst.left)
                    {
                        /* no overlap, or dst above src, so copy from top downwards */
                        for (y = 0; y < dstheight; y++)
                        {
                            memcpy(dbuf, sbuf, width);
                            sbuf += slock.Pitch;
                            dbuf += dlock.Pitch;
                        }
                    }
                    else if (xdst.top > xsrc.top)  /* copy from bottom upwards */
                    {
                        sbuf += (slock.Pitch*dstheight);
                        dbuf += (dlock.Pitch*dstheight);
                        for (y = 0; y < dstheight; y++)
                        {
                            sbuf -= slock.Pitch;
                            dbuf -= dlock.Pitch;
                            memcpy(dbuf, sbuf, width);
                        }
                    }
                    else /* src and dst overlapping on the same line, use memmove */
                    {
                        for (y = 0; y < dstheight; y++)
                        {
                            memmove(dbuf, sbuf, width);
                            sbuf += slock.Pitch;
                            dbuf += dlock.Pitch;
                        }
                    }
                } else {
                    /* Stretching in Y direction only */
                    for (y = sy = 0; y < dstheight; y++, sy += yinc) {
                        sbuf = sbase + (sy >> 16) * slock.Pitch;
                        memcpy(dbuf, sbuf, width);
                        dbuf += dlock.Pitch;
                    }
                }
            }
            else
            {
                /* Stretching in X direction */
                int last_sy = -1;
                for (y = sy = 0; y < dstheight; y++, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * slock.Pitch;

                    if ((sy >> 16) == (last_sy >> 16))
                    {
                        /* this sourcerow is the same as last sourcerow -
                         * copy already stretched row
                         */
                        memcpy(dbuf, dbuf - dlock.Pitch, width);
                    }
                    else
                    {
#define STRETCH_ROW(type) { \
                    type *s = (type *) sbuf, *d = (type *) dbuf; \
                    for (x = sx = 0; x < dstwidth; x++, sx += xinc) \
                    d[x] = s[sx >> 16]; \
                    break; }

                    switch(bpp)
                    {
                        case 1: STRETCH_ROW(BYTE)
                        case 2: STRETCH_ROW(WORD)
                        case 4: STRETCH_ROW(DWORD)
                        case 3:
                        {
                            LPBYTE s,d = dbuf;
                            for (x = sx = 0; x < dstwidth; x++, sx+= xinc)
                            {
                                DWORD pixel;

                                s = sbuf+3*(sx>>16);
                                pixel = s[0]|(s[1]<<8)|(s[2]<<16);
                                d[0] = (pixel    )&0xff;
                                d[1] = (pixel>> 8)&0xff;
                                d[2] = (pixel>>16)&0xff;
                                d+=3;
                            }
                            break;
                    }
                    default:
                        FIXME("Stretched blit not implemented for bpp %d!\n", bpp*8);
                        ret = WINED3DERR_NOTAVAILABLE;
                        goto error;
                    }
#undef STRETCH_ROW
                    }
                    dbuf += dlock.Pitch;
                    last_sy = sy;
                }
            }
        }
        else
        {
          LONG dstyinc = dlock.Pitch, dstxinc = bpp;
          DWORD keylow = 0xFFFFFFFF, keyhigh = 0, keymask = 0xFFFFFFFF;
          DWORD destkeylow = 0x0, destkeyhigh = 0xFFFFFFFF, destkeymask = 0xFFFFFFFF;
          if (Flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYDEST | WINEDDBLT_KEYSRCOVERRIDE | WINEDDBLT_KEYDESTOVERRIDE))
          {
              /* The color keying flags are checked for correctness in ddraw */
              if (Flags & WINEDDBLT_KEYSRC)
              {
                keylow  = Src->SrcBltCKey.dwColorSpaceLowValue;
                keyhigh = Src->SrcBltCKey.dwColorSpaceHighValue;
              }
              else  if (Flags & WINEDDBLT_KEYSRCOVERRIDE)
              {
                keylow  = DDBltFx->ddckSrcColorkey.dwColorSpaceLowValue;
                keyhigh = DDBltFx->ddckSrcColorkey.dwColorSpaceHighValue;
              }

              if (Flags & WINEDDBLT_KEYDEST)
              {
                /* Destination color keys are taken from the source surface ! */
                destkeylow  = Src->DestBltCKey.dwColorSpaceLowValue;
                destkeyhigh = Src->DestBltCKey.dwColorSpaceHighValue;
              }
              else if (Flags & WINEDDBLT_KEYDESTOVERRIDE)
              {
                destkeylow  = DDBltFx->ddckDestColorkey.dwColorSpaceLowValue;
                destkeyhigh = DDBltFx->ddckDestColorkey.dwColorSpaceHighValue;
              }

              if(bpp == 1)
              {
                  keymask = 0xff;
              }
              else
              {
                  keymask = sEntry->redMask   |
                            sEntry->greenMask |
                            sEntry->blueMask;
              }
              Flags &= ~(WINEDDBLT_KEYSRC | WINEDDBLT_KEYDEST | WINEDDBLT_KEYSRCOVERRIDE | WINEDDBLT_KEYDESTOVERRIDE);
          }

          if (Flags & WINEDDBLT_DDFX)
          {
              LPBYTE dTopLeft, dTopRight, dBottomLeft, dBottomRight, tmp;
              LONG tmpxy;
              dTopLeft     = dbuf;
              dTopRight    = dbuf+((dstwidth-1)*bpp);
              dBottomLeft  = dTopLeft+((dstheight-1)*dlock.Pitch);
              dBottomRight = dBottomLeft+((dstwidth-1)*bpp);

              if (DDBltFx->dwDDFX & WINEDDBLTFX_ARITHSTRETCHY)
              {
                /* I don't think we need to do anything about this flag */
                WARN("Flags=DDBLT_DDFX nothing done for WINEDDBLTFX_ARITHSTRETCHY\n");
              }
              if (DDBltFx->dwDDFX & WINEDDBLTFX_MIRRORLEFTRIGHT)
              {
                tmp          = dTopRight;
                dTopRight    = dTopLeft;
                dTopLeft     = tmp;
                tmp          = dBottomRight;
                dBottomRight = dBottomLeft;
                dBottomLeft  = tmp;
                dstxinc = dstxinc *-1;
              }
              if (DDBltFx->dwDDFX & WINEDDBLTFX_MIRRORUPDOWN)
              {
                tmp          = dTopLeft;
                dTopLeft     = dBottomLeft;
                dBottomLeft  = tmp;
                tmp          = dTopRight;
                dTopRight    = dBottomRight;
                dBottomRight = tmp;
                dstyinc = dstyinc *-1;
              }
              if (DDBltFx->dwDDFX & WINEDDBLTFX_NOTEARING)
              {
                /* I don't think we need to do anything about this flag */
                WARN("Flags=DDBLT_DDFX nothing done for WINEDDBLTFX_NOTEARING\n");
              }
              if (DDBltFx->dwDDFX & WINEDDBLTFX_ROTATE180)
              {
                tmp          = dBottomRight;
                dBottomRight = dTopLeft;
                dTopLeft     = tmp;
                tmp          = dBottomLeft;
                dBottomLeft  = dTopRight;
                dTopRight    = tmp;
                dstxinc = dstxinc * -1;
                dstyinc = dstyinc * -1;
              }
              if (DDBltFx->dwDDFX & WINEDDBLTFX_ROTATE270)
              {
                tmp          = dTopLeft;
                dTopLeft     = dBottomLeft;
                dBottomLeft  = dBottomRight;
                dBottomRight = dTopRight;
                dTopRight    = tmp;
                tmpxy   = dstxinc;
                dstxinc = dstyinc;
                dstyinc = tmpxy;
                dstxinc = dstxinc * -1;
              }
              if (DDBltFx->dwDDFX & WINEDDBLTFX_ROTATE90)
              {
                tmp          = dTopLeft;
                dTopLeft     = dTopRight;
                dTopRight    = dBottomRight;
                dBottomRight = dBottomLeft;
                dBottomLeft  = tmp;
                tmpxy   = dstxinc;
                dstxinc = dstyinc;
                dstyinc = tmpxy;
                dstyinc = dstyinc * -1;
              }
              if (DDBltFx->dwDDFX & WINEDDBLTFX_ZBUFFERBASEDEST)
              {
                /* I don't think we need to do anything about this flag */
                WARN("Flags=WINEDDBLT_DDFX nothing done for WINEDDBLTFX_ZBUFFERBASEDEST\n");
              }
              dbuf = dTopLeft;
              Flags &= ~(WINEDDBLT_DDFX);
          }

#define COPY_COLORKEY_FX(type) { \
            type *s, *d = (type *) dbuf, *dx, tmp; \
            for (y = sy = 0; y < dstheight; y++, sy += yinc) { \
              s = (type*)(sbase + (sy >> 16) * slock.Pitch); \
              dx = d; \
              for (x = sx = 0; x < dstwidth; x++, sx += xinc) { \
                  tmp = s[sx >> 16]; \
                  if (((tmp & keymask) < keylow || (tmp & keymask) > keyhigh) && \
                      ((dx[0] & destkeymask) >= destkeylow && (dx[0] & destkeymask) <= destkeyhigh)) { \
                       dx[0] = tmp; \
                     } \
                  dx = (type*)(((LPBYTE)dx)+dstxinc); \
              } \
              d = (type*)(((LPBYTE)d)+dstyinc); \
            } \
            break; }

            switch (bpp) {
            case 1: COPY_COLORKEY_FX(BYTE)
            case 2: COPY_COLORKEY_FX(WORD)
            case 4: COPY_COLORKEY_FX(DWORD)
            case 3:
            {
                LPBYTE s,d = dbuf, dx;
                for (y = sy = 0; y < dstheight; y++, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * slock.Pitch;
                    dx = d;
                    for (x = sx = 0; x < dstwidth; x++, sx+= xinc)
                    {
                        DWORD pixel, dpixel = 0;
                        s = sbuf+3*(sx>>16);
                        pixel = s[0]|(s[1]<<8)|(s[2]<<16);
                        dpixel = dx[0]|(dx[1]<<8)|(dx[2]<<16);
                        if (((pixel & keymask) < keylow || (pixel & keymask) > keyhigh) &&
                            ((dpixel & keymask) >= destkeylow || (dpixel & keymask) <= keyhigh))
                        {
                            dx[0] = (pixel    )&0xff;
                            dx[1] = (pixel>> 8)&0xff;
                            dx[2] = (pixel>>16)&0xff;
                        }
                        dx+= dstxinc;
                    }
                    d += dstyinc;
                }
                break;
            }
            default:
              FIXME("%s color-keyed blit not implemented for bpp %d!\n",
                  (Flags & WINEDDBLT_KEYSRC) ? "Source" : "Destination", bpp*8);
                  ret = WINED3DERR_NOTAVAILABLE;
                  goto error;
#undef COPY_COLORKEY_FX
            }
        }
    }

error:
    if (Flags && FIXME_ON(d3d_surface))
    {
        FIXME("\tUnsupported flags: %08x\n", Flags);
    }

release:
    IWineD3DSurface_UnlockRect(iface);
    if (SrcSurface && SrcSurface != iface) IWineD3DSurface_UnlockRect(SrcSurface);
    return ret;
}

/*****************************************************************************
 * IWineD3DSurface::BltFast, GDI version
 *
 * This is the software implementation of BltFast, as used by GDI surfaces
 * and as a fallback for OpenGL surfaces. This code is taken from the old
 * DirectDraw code, and was originally written by TransGaming.
 *
 * Params:
 *  dstx:
 *  dsty:
 *  Source: Source surface to copy from
 *  rsrc: Source rectangle
 *  trans: Some Flags
 *
 * Returns:
 *  WINED3D_OK on success
 *
 *****************************************************************************/
HRESULT WINAPI
IWineGDISurfaceImpl_BltFast(IWineD3DSurface *iface,
                            DWORD dstx,
                            DWORD dsty,
                            IWineD3DSurface *Source,
                            RECT *rsrc,
                            DWORD trans)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) Source;

    int                 bpp, w, h, x, y;
    WINED3DLOCKED_RECT  dlock,slock;
    HRESULT             ret = WINED3D_OK;
    RECT                rsrc2;
    RECT                lock_src, lock_dst, lock_union;
    BYTE                *sbuf, *dbuf;
    const StaticPixelFormatDesc *sEntry, *dEntry;

    if (TRACE_ON(d3d_surface))
    {
        TRACE("(%p)->(%d,%d,%p,%p,%08x)\n", This,dstx,dsty,Src,rsrc,trans);

        if (rsrc)
        {
            TRACE("\tsrcrect: %dx%d-%dx%d\n",rsrc->left,rsrc->top,
                  rsrc->right,rsrc->bottom);
        }
        else
        {
            TRACE(" srcrect: NULL\n");
        }
    }

    if ((This->Flags & SFLAG_LOCKED) ||
        ((Src != NULL) && (Src->Flags & SFLAG_LOCKED)))
    {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

    if (!rsrc)
    {
        WARN("rsrc is NULL!\n");
        rsrc = &rsrc2;
        rsrc->left = 0;
        rsrc->top = 0;
        rsrc->right = Src->currentDesc.Width;
        rsrc->bottom = Src->currentDesc.Height;
    }

    /* Check source rect for validity. Copied from normal Blt. Fixes Baldur's Gate.*/
    if ((rsrc->bottom > Src->currentDesc.Height) || (rsrc->bottom < 0) ||
        (rsrc->top    > Src->currentDesc.Height) || (rsrc->top    < 0) ||
        (rsrc->left   > Src->currentDesc.Width)  || (rsrc->left   < 0) ||
        (rsrc->right  > Src->currentDesc.Width)  || (rsrc->right  < 0) ||
        (rsrc->right  < rsrc->left)              || (rsrc->bottom < rsrc->top))
    {
        WARN("Application gave us bad source rectangle for BltFast.\n");
        return WINEDDERR_INVALIDRECT;
    }

    h = rsrc->bottom - rsrc->top;
    if (h > This->currentDesc.Height-dsty) h = This->currentDesc.Height-dsty;
    if (h > Src->currentDesc.Height-rsrc->top) h=Src->currentDesc.Height-rsrc->top;
    if (h <= 0) return WINEDDERR_INVALIDRECT;

    w = rsrc->right - rsrc->left;
    if (w > This->currentDesc.Width-dstx) w = This->currentDesc.Width-dstx;
    if (w > Src->currentDesc.Width-rsrc->left) w = Src->currentDesc.Width-rsrc->left;
    if (w <= 0) return WINEDDERR_INVALIDRECT;

    /* Now compute the locking rectangle... */
    lock_src.left = rsrc->left;
    lock_src.top = rsrc->top;
    lock_src.right = lock_src.left + w;
    lock_src.bottom = lock_src.top + h;

    lock_dst.left = dstx;
    lock_dst.top = dsty;
    lock_dst.right = dstx + w;
    lock_dst.bottom = dsty + h;

    bpp = This->bytesPerPixel;

    /* We need to lock the surfaces, or we won't get refreshes when done. */
    if (Src == This)
    {
        int pitch;

        UnionRect(&lock_union, &lock_src, &lock_dst);

        /* Lock the union of the two rectangles */
        ret = IWineD3DSurface_LockRect(iface, &dlock, &lock_union, 0);
        if(ret != WINED3D_OK) goto error;

        pitch = dlock.Pitch;
        slock.Pitch = dlock.Pitch;

        /* Since slock was originally copied from this surface's description, we can just reuse it */
        assert(This->resource.allocatedMemory != NULL);
        sbuf = (BYTE *)This->resource.allocatedMemory + lock_src.top * pitch + lock_src.left * bpp;
        dbuf = (BYTE *)This->resource.allocatedMemory + lock_dst.top * pitch + lock_dst.left * bpp;
        sEntry = getFormatDescEntry(Src->resource.format, NULL, NULL);
        dEntry = sEntry;
    }
    else
    {
        ret = IWineD3DSurface_LockRect(Source, &slock, &lock_src, WINED3DLOCK_READONLY);
        if(ret != WINED3D_OK) goto error;
        ret = IWineD3DSurface_LockRect(iface, &dlock, &lock_dst, 0);
        if(ret != WINED3D_OK) goto error;

        sbuf = slock.pBits;
        dbuf = dlock.pBits;
        TRACE("Dst is at %p, Src is at %p\n", dbuf, sbuf);

        sEntry = getFormatDescEntry(Src->resource.format, NULL, NULL);
        dEntry = getFormatDescEntry(This->resource.format, NULL, NULL);
    }

    /* Handle first the FOURCC surfaces... */
    if (sEntry->isFourcc && dEntry->isFourcc)
    {
        TRACE("Fourcc -> Fourcc copy\n");
        if (trans)
            FIXME("trans arg not supported when a FOURCC surface is involved\n");
        if (dstx || dsty)
            FIXME("offset for destination surface is not supported\n");
        if (Src->resource.format != This->resource.format)
        {
            FIXME("FOURCC->FOURCC copy only supported for the same type of surface\n");
            ret = WINED3DERR_WRONGTEXTUREFORMAT;
            goto error;
        }
        /* FIXME: Watch out that the size is correct for FOURCC surfaces */
        memcpy(dbuf, sbuf, This->resource.size);
        goto error;
    }
    if (sEntry->isFourcc && !dEntry->isFourcc)
    {
        /* TODO: Use the libtxc_dxtn.so shared library to do
         * software decompression
         */
        ERR("DXTC decompression not supported by now\n");
        goto error;
    }

    if (trans & (WINEDDBLTFAST_SRCCOLORKEY | WINEDDBLTFAST_DESTCOLORKEY))
    {
        DWORD keylow, keyhigh;
        TRACE("Color keyed copy\n");
        if (trans & WINEDDBLTFAST_SRCCOLORKEY)
        {
            keylow  = Src->SrcBltCKey.dwColorSpaceLowValue;
            keyhigh = Src->SrcBltCKey.dwColorSpaceHighValue;
        }
        else
        {
            /* I'm not sure if this is correct */
            FIXME("WINEDDBLTFAST_DESTCOLORKEY not fully supported yet.\n");
            keylow  = This->DestBltCKey.dwColorSpaceLowValue;
            keyhigh = This->DestBltCKey.dwColorSpaceHighValue;
        }

#define COPYBOX_COLORKEY(type) { \
            type *d, *s, tmp; \
            s = (type *) sbuf; \
            d = (type *) dbuf; \
            for (y = 0; y < h; y++) { \
                for (x = 0; x < w; x++) { \
                    tmp = s[x]; \
                    if (tmp < keylow || tmp > keyhigh) d[x] = tmp; \
                } \
                s = (type *)((BYTE *)s + slock.Pitch); \
                d = (type *)((BYTE *)d + dlock.Pitch); \
            } \
            break; \
        }

        switch (bpp) {
            case 1: COPYBOX_COLORKEY(BYTE)
            case 2: COPYBOX_COLORKEY(WORD)
            case 4: COPYBOX_COLORKEY(DWORD)
            case 3:
            {
                BYTE *d, *s;
                DWORD tmp;
                s = (BYTE *) sbuf;
                d = (BYTE *) dbuf;
                for (y = 0; y < h; y++)
                {
                    for (x = 0; x < w * 3; x += 3)
                    {
                        tmp = (DWORD)s[x] + ((DWORD)s[x + 1] << 8) + ((DWORD)s[x + 2] << 16);
                        if (tmp < keylow || tmp > keyhigh)
                        {
                            d[x + 0] = s[x + 0];
                            d[x + 1] = s[x + 1];
                            d[x + 2] = s[x + 2];
                        }
                    }
                    s += slock.Pitch;
                    d += dlock.Pitch;
                }
                break;
            }
            default:
                FIXME("Source color key blitting not supported for bpp %d\n",bpp*8);
                ret = WINED3DERR_NOTAVAILABLE;
                goto error;
        }
#undef COPYBOX_COLORKEY
        TRACE("Copy Done\n");
    }
    else
    {
        int width = w * bpp;
        TRACE("NO color key copy\n");
        for (y = 0; y < h; y++)
        {
            /* This is pretty easy, a line for line memcpy */
            memcpy(dbuf, sbuf, width);
            sbuf += slock.Pitch;
            dbuf += dlock.Pitch;
        }
        TRACE("Copy done\n");
    }

error:
    if (Src == This)
    {
        IWineD3DSurface_UnlockRect(iface);
    }
    else
    {
        IWineD3DSurface_UnlockRect(iface);
        IWineD3DSurface_UnlockRect(Source);
    }

    return ret;
}

/*****************************************************************************
 * IWineD3DSurface::LoadTexture, GDI version
 *
 * This is mutually unsupported by GDI surfaces
 *
 * Returns:
 *  D3DERR_INVALIDCALL
 *
 *****************************************************************************/
HRESULT WINAPI
IWineGDISurfaceImpl_LoadTexture(IWineD3DSurface *iface, BOOL srgb_mode)
{
    ERR("Unsupported on X11 surfaces\n");
    return WINED3DERR_INVALIDCALL;
}

/*****************************************************************************
 * IWineD3DSurface::SaveSnapshot, GDI version
 *
 * This method writes the surface's contents to the in tga format to the
 * file specified in filename.
 *
 * Params:
 *  filename: File to write to
 *
 * Returns:
 *  WINED3DERR_INVALIDCALL if the file couldn't be opened
 *  WINED3D_OK on success
 *
 *****************************************************************************/
static int get_shift(DWORD color_mask) {
    int shift = 0;
    while (color_mask > 0xFF) {
        color_mask >>= 1;
        shift += 1;
    }
    while ((color_mask & 0x80) == 0) {
        color_mask <<= 1;
        shift -= 1;
    }
    return shift;
}


HRESULT WINAPI
IWineGDISurfaceImpl_SaveSnapshot(IWineD3DSurface *iface,
const char* filename)
{
    FILE* f = NULL;
    UINT y = 0, x = 0;
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    static char *output = NULL;
    static int size = 0;
    const StaticPixelFormatDesc *formatEntry = getFormatDescEntry(This->resource.format, NULL, NULL);

    if (This->pow2Width > size) {
        output = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->pow2Width * 3);
        size = This->pow2Width;
    }


    f = fopen(filename, "w+");
    if (NULL == f) {
        ERR("opening of %s failed with\n", filename);
        return WINED3DERR_INVALIDCALL;
    }
    fprintf(f, "P6\n%d %d\n255\n", This->pow2Width, This->pow2Height);

    if (This->resource.format == WINED3DFMT_P8) {
        unsigned char table[256][3];
        int i;

        if (This->palette == NULL) {
            fclose(f);
            return WINED3DERR_INVALIDCALL;
        }
        for (i = 0; i < 256; i++) {
            table[i][0] = This->palette->palents[i].peRed;
            table[i][1] = This->palette->palents[i].peGreen;
            table[i][2] = This->palette->palents[i].peBlue;
        }
        for (y = 0; y < This->pow2Height; y++) {
            unsigned char *src = (unsigned char *) This->resource.allocatedMemory + (y * 1 * IWineD3DSurface_GetPitch(iface));
            for (x = 0; x < This->pow2Width; x++) {
                unsigned char color = *src;
                src += 1;

                output[3 * x + 0] = table[color][0];
                output[3 * x + 1] = table[color][1];
                output[3 * x + 2] = table[color][2];
            }
            fwrite(output, 3 * This->pow2Width, 1, f);
        }
    } else {
        int red_shift, green_shift, blue_shift, pix_width;

        pix_width = This->bytesPerPixel;

        red_shift = get_shift(formatEntry->redMask);
        green_shift = get_shift(formatEntry->greenMask);
        blue_shift = get_shift(formatEntry->blueMask);

        for (y = 0; y < This->pow2Height; y++) {
            unsigned char *src = (unsigned char *) This->resource.allocatedMemory + (y * 1 * IWineD3DSurface_GetPitch(iface));
            for (x = 0; x < This->pow2Width; x++) {	    
                unsigned int color;
                unsigned int comp;
                int i;

                color = 0;
                for (i = 0; i < pix_width; i++) {
                    color |= src[i] << (8 * i);
                }
                src += 1 * pix_width;

                comp = color & formatEntry->redMask;
                output[3 * x + 0] = red_shift > 0 ? comp >> red_shift : comp << -red_shift;
                comp = color & formatEntry->greenMask;
                output[3 * x + 1] = green_shift > 0 ? comp >> green_shift : comp << -green_shift;
                comp = color & formatEntry->blueMask;
                output[3 * x + 2] = blue_shift > 0 ? comp >> blue_shift : comp << -blue_shift;
            }
            fwrite(output, 3 * This->pow2Width, 1, f);
        }
    }
    fclose(f);
    return WINED3D_OK;
}

/*****************************************************************************
 * IWineD3DSurface::PrivateSetup, GDI version
 *
 * Initializes the GDI surface, aka creates the DIB section we render to
 * The DIB section creation is done by calling GetDC, which will create the
 * section and releasing the dc to allow the app to use it. The dib section
 * will stay until the surface is released
 *
 * GDI surfaces do not need to be a power of 2 in size, so the pow2 sizes
 * are set to the real sizes to save memory. The NONPOW2 flag is unset to
 * avoid confusion in the shared surface code.
 *
 * Returns:
 *  WINED3D_OK on success
 *  The return values of called methods on failure
 *
 *****************************************************************************/
HRESULT WINAPI
IWineGDISurfaceImpl_PrivateSetup(IWineD3DSurface *iface)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    HRESULT hr;
    HDC hdc;

    if(This->resource.usage & WINED3DUSAGE_OVERLAY)
    {
        ERR("(%p) Overlays not yet supported by GDI surfaces\n", This);
        return WINED3DERR_INVALIDCALL;
    }
    /* Sysmem textures have memory already allocated -
     * release it, this avoids an unnecessary memcpy
     */
    HeapFree(GetProcessHeap(), 0, This->resource.allocatedMemory);
    This->resource.allocatedMemory = NULL;

    /* We don't mind the nonpow2 stuff in GDI */
    This->pow2Width = This->currentDesc.Width;
    This->pow2Height = This->currentDesc.Height;

    /* Call GetDC to create a DIB section. We will use that
     * DIB section for rendering
     *
     * Release the DC afterwards to allow the app to use it
     */
    hr = IWineD3DSurface_GetDC(iface, &hdc);
    if(FAILED(hr))
    {
        ERR("(%p) IWineD3DSurface::GetDC failed with hr %08x\n", This, hr);
        return hr;
    }
    hr = IWineD3DSurface_ReleaseDC(iface, hdc);
    if(FAILED(hr))
    {
        ERR("(%p) IWineD3DSurface::ReleaseDC failed with hr %08x\n", This, hr);
        return hr;
    }

    return WINED3D_OK;
}

const IWineD3DSurfaceVtbl IWineGDISurface_Vtbl =
{
    /* IUnknown */
    IWineD3DSurfaceImpl_QueryInterface,
    IWineD3DSurfaceImpl_AddRef,
    IWineD3DSurfaceImpl_Release,
    /* IWineD3DResource */
    IWineD3DSurfaceImpl_GetParent,
    IWineD3DSurfaceImpl_GetDevice,
    IWineD3DSurfaceImpl_SetPrivateData,
    IWineD3DSurfaceImpl_GetPrivateData,
    IWineD3DSurfaceImpl_FreePrivateData,
    IWineD3DSurfaceImpl_SetPriority,
    IWineD3DSurfaceImpl_GetPriority,
    IWineGDISurfaceImpl_PreLoad,
    IWineD3DSurfaceImpl_GetType,
    /* IWineD3DSurface */
    IWineD3DSurfaceImpl_GetContainer,
    IWineD3DSurfaceImpl_GetDesc,
    IWineGDISurfaceImpl_LockRect,
    IWineGDISurfaceImpl_UnlockRect,
    IWineD3DSurfaceImpl_GetDC,
    IWineD3DSurfaceImpl_ReleaseDC,
    IWineGDISurfaceImpl_Flip,
    IWineGDISurfaceImpl_Blt,
    IWineD3DSurfaceImpl_GetBltStatus,
    IWineD3DSurfaceImpl_GetFlipStatus,
    IWineD3DSurfaceImpl_IsLost,
    IWineD3DSurfaceImpl_Restore,
    IWineGDISurfaceImpl_BltFast,
    IWineD3DSurfaceImpl_GetPalette,
    IWineD3DSurfaceImpl_SetPalette,
    IWineD3DSurfaceImpl_RealizePalette,
    IWineD3DSurfaceImpl_SetColorKey,
    IWineD3DSurfaceImpl_GetPitch,
    IWineD3DSurfaceImpl_SetMem,
    IWineD3DSurfaceImpl_SetOverlayPosition,
    IWineD3DSurfaceImpl_GetOverlayPosition,
    IWineD3DSurfaceImpl_UpdateOverlayZOrder,
    IWineD3DSurfaceImpl_UpdateOverlay,
    IWineD3DSurfaceImpl_SetClipper,
    IWineD3DSurfaceImpl_GetClipper,
    /* Internal use: */
    IWineD3DSurfaceImpl_AddDirtyRect,
    IWineGDISurfaceImpl_LoadTexture,
    IWineGDISurfaceImpl_SaveSnapshot,
    IWineD3DSurfaceImpl_SetContainer,
    IWineD3DSurfaceImpl_SetGlTextureDesc,
    IWineD3DSurfaceImpl_GetGlDesc,
    IWineD3DSurfaceImpl_GetData,
    IWineD3DSurfaceImpl_SetFormat,
    IWineGDISurfaceImpl_PrivateSetup
};
