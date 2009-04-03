/*
 *IDirect3DSwapChain9 implementation
 *
 *Copyright 2002-2003 Jason Edmeades
 *Copyright 2002-2003 Raphael Junqueira
 *Copyright 2005 Oliver Stieber
 *Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 *
 *This library is free software; you can redistribute it and/or
 *modify it under the terms of the GNU Lesser General Public
 *License as published by the Free Software Foundation; either
 *version 2.1 of the License, or (at your option) any later version.
 *
 *This library is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *Lesser General Public License for more details.
 *
 *You should have received a copy of the GNU Lesser General Public
 *License along with this library; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(fps);

static void WINAPI IWineGDISwapChainImpl_Destroy(IWineD3DSwapChain *iface, D3DCB_DESTROYSURFACEFN D3DCB_DestroyRenderback) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    WINED3DDISPLAYMODE mode;

    TRACE("Destroying swapchain %p\n", iface);

    IWineD3DSwapChain_SetGammaRamp(iface, 0, &This->orig_gamma);

    /* release the ref to the front and back buffer parents */
    if(This->frontBuffer) {
        IWineD3DSurface_SetContainer(This->frontBuffer, 0);
        if(D3DCB_DestroyRenderback(This->frontBuffer) > 0) {
            FIXME("(%p) Something's still holding the front buffer\n",This);
        }
    }

    if(This->backBuffer) {
        UINT i;
        for(i = 0; i < This->presentParms.BackBufferCount; i++) {
            IWineD3DSurface_SetContainer(This->backBuffer[i], 0);
            if(D3DCB_DestroyRenderback(This->backBuffer[i]) > 0) {
                FIXME("(%p) Something's still holding the back buffer\n",This);
            }
        }
        HeapFree(GetProcessHeap(), 0, This->backBuffer);
    }

    /* Restore the screen resolution if we rendered in fullscreen
     * This will restore the screen resolution to what it was before creating the swapchain. In case of d3d8 and d3d9
     * this will be the original desktop resolution. In case of d3d7 this will be a NOP because ddraw sets the resolution
     * before starting up Direct3D, thus orig_width and orig_height will be equal to the modes in the presentation params
     */
    if(This->presentParms.Windowed == FALSE && This->presentParms.AutoRestoreDisplayMode) {
        mode.Width = This->orig_width;
        mode.Height = This->orig_height;
        mode.RefreshRate = 0;
        mode.Format = This->orig_fmt;
        IWineD3DDevice_SetDisplayMode((IWineD3DDevice *) This->wineD3DDevice, 0, &mode);
    }

    HeapFree(GetProcessHeap(), 0, This);
}

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
void x11_copy_to_screen(IWineD3DSwapChainImpl *This, const RECT *rc)
{
    IWineD3DSurfaceImpl *front = (IWineD3DSurfaceImpl *) This->frontBuffer;

    if(front->resource.usage & WINED3DUSAGE_RENDERTARGET) {
        POINT offset = {0,0};
        HWND hDisplayWnd;
        HDC hDisplayDC;
        HDC hSurfaceDC = 0;
        RECT drawrect;
        TRACE("(%p)->(%p): Copying to screen\n", front, rc);

        hSurfaceDC = front->hDC;

        hDisplayWnd = This->win_handle;
        hDisplayDC = GetDCEx(hDisplayWnd, 0, DCX_CLIPSIBLINGS|DCX_CACHE);
        if(rc) {
            TRACE(" copying rect (%d,%d)->(%d,%d), offset (%d,%d)\n",
                  rc->left, rc->top, rc->right, rc->bottom, offset.x, offset.y);
        }

        /* Front buffer coordinates are screen coordinates. Map them to the destination
         * window if not fullscreened
         */
        if(This->presentParms.Windowed) {
            ClientToScreen(hDisplayWnd, &offset);
        }
#if 0
        /* FIXME: This doesn't work... if users really want to run
        * X in 8bpp, then we need to call directly into display.drv
        * (or Wine's equivalent), and force a private colormap
        * without default entries. */
        if (front->palette) {
        SelectPalette(hDisplayDC, front->palette->hpal, FALSE);
        RealizePalette(hDisplayDC); /* sends messages => deadlocks */
    }
#endif
        drawrect.left   = 0;
        drawrect.right  = front->currentDesc.Width;
        drawrect.top    = 0;
        drawrect.bottom = front->currentDesc.Height;

#if 0
        /* TODO: Support clippers */
        if (front->clipper)
        {
        RECT xrc;
        HWND hwnd = ((IWineD3DClipperImpl *) front->clipper)->hWnd;
        if (hwnd && GetClientRect(hwnd,&xrc))
        {
        OffsetRect(&xrc,offset.x,offset.y);
        IntersectRect(&drawrect,&drawrect,&xrc);
    }
    }
#endif
        if (rc) {
            IntersectRect(&drawrect,&drawrect,rc);
        }
        else {
            /* Only use this if the caller did not pass a rectangle, since
            * due to double locking this could be the wrong one ...
            */
            if (front->lockedRect.left != front->lockedRect.right) {
                IntersectRect(&drawrect,&drawrect,&front->lockedRect);
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

static HRESULT WINAPI IWineGDISwapChainImpl_SetDestWindowOverride(IWineD3DSwapChain *iface, HWND window) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;

    This->win_handle = window;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineGDISwapChainImpl_Present(IWineD3DSwapChain *iface, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion, DWORD dwFlags) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *) iface;
    IWineD3DSurfaceImpl *front, *back;

    if(!This->backBuffer) {
        WARN("Swapchain doesn't have a backbuffer, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }
    front = (IWineD3DSurfaceImpl *) This->frontBuffer;
    back = (IWineD3DSurfaceImpl *) This->backBuffer[0];

    /* Flip the DC */
    {
        HDC tmp;
        tmp = front->hDC;
        front->hDC = back->hDC;
        back->hDC = tmp;
    }

    /* Flip the DIBsection */
    {
        HBITMAP tmp;
        tmp = front->dib.DIBsection;
        front->dib.DIBsection = back->dib.DIBsection;
        back->dib.DIBsection = tmp;
    }

    /* Flip the surface data */
    {
        void* tmp;

        tmp = front->dib.bitmap_data;
        front->dib.bitmap_data = back->dib.bitmap_data;
        back->dib.bitmap_data = tmp;

        tmp = front->resource.allocatedMemory;
        front->resource.allocatedMemory = back->resource.allocatedMemory;
        back->resource.allocatedMemory = tmp;

        if(front->resource.heapMemory) {
            ERR("GDI Surface %p has heap memory allocated\n", front);
        }
        if(back->resource.heapMemory) {
            ERR("GDI Surface %p has heap memory allocated\n", back);
        }
    }

    /* client_memory should not be different, but just in case */
    {
        BOOL tmp;
        tmp = front->dib.client_memory;
        front->dib.client_memory = back->dib.client_memory;
        back->dib.client_memory = tmp;
    }

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

    x11_copy_to_screen(This, NULL);

    return WINED3D_OK;
}

const IWineD3DSwapChainVtbl IWineGDISwapChain_Vtbl =
{
    /* IUnknown */
    IWineD3DBaseSwapChainImpl_QueryInterface,
    IWineD3DBaseSwapChainImpl_AddRef,
    IWineD3DBaseSwapChainImpl_Release,
    /* IWineD3DSwapChain */
    IWineD3DBaseSwapChainImpl_GetParent,
    IWineGDISwapChainImpl_Destroy,
    IWineD3DBaseSwapChainImpl_GetDevice,
    IWineGDISwapChainImpl_Present,
    IWineGDISwapChainImpl_SetDestWindowOverride,
    IWineD3DBaseSwapChainImpl_GetFrontBufferData,
    IWineD3DBaseSwapChainImpl_GetBackBuffer,
    IWineD3DBaseSwapChainImpl_GetRasterStatus,
    IWineD3DBaseSwapChainImpl_GetDisplayMode,
    IWineD3DBaseSwapChainImpl_GetPresentParameters,
    IWineD3DBaseSwapChainImpl_SetGammaRamp,
    IWineD3DBaseSwapChainImpl_GetGammaRamp
};
