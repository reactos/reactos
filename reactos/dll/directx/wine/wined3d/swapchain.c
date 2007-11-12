/*
 *IDirect3DSwapChain9 implementation
 *
 *Copyright 2002-2003 Jason Edmeades
 *Copyright 2002-2003 Raphael Junqueira
 *Copyright 2005 Oliver Stieber
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


/*TODO: some of the additional parameters may be required to
    set the gamma ramp (for some weird reason microsoft have left swap gammaramp in device
    but it operates on a swapchain, it may be a good idea to move it to IWineD3DSwapChain for IWineD3D)*/


WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(fps);

#define GLINFO_LOCATION This->wineD3DDevice->adapter->gl_info

/* IDirect3DSwapChain IUnknown parts follow: */
static ULONG WINAPI IWineD3DSwapChainImpl_AddRef(IWineD3DSwapChain *iface) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    DWORD refCount = InterlockedIncrement(&This->ref);
    TRACE("(%p) : AddRef increasing from %d\n", This, refCount - 1);
    return refCount;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_QueryInterface(IWineD3DSwapChain *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DSwapChain)){
        IWineD3DSwapChainImpl_AddRef(iface);
        if(ppobj == NULL){
            ERR("Query interface called but now data allocated\n");
            return E_NOINTERFACE;
        }
        *ppobj = This;
        return WINED3D_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}


static ULONG WINAPI IWineD3DSwapChainImpl_Release(IWineD3DSwapChain *iface) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    DWORD refCount;
    refCount = InterlockedDecrement(&This->ref);
    TRACE("(%p) : ReleaseRef to %d\n", This, refCount);
    if (refCount == 0) {
        IWineD3DSwapChain_Destroy(iface, D3DCB_DefaultDestroySurface);
    }
    return refCount;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetParent(IWineD3DSwapChain *iface, IUnknown ** ppParent){
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    *ppParent = This->parent;
    IUnknown_AddRef(*ppParent);
    TRACE("(%p) returning %p\n", This , *ppParent);
    return WINED3D_OK;
}

/*IWineD3DSwapChain parts follow: */
static void WINAPI IWineD3DSwapChainImpl_Destroy(IWineD3DSwapChain *iface, D3DCB_DESTROYSURFACEFN D3DCB_DestroyRenderTarget) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    WINED3DDISPLAYMODE mode;
    int i;

    /* release the ref to the front and back buffer parents */
    if(This->frontBuffer) {
        IWineD3DSurface_SetContainer(This->frontBuffer, 0);
        if(D3DCB_DestroyRenderTarget(This->frontBuffer) > 0) {
            FIXME("(%p) Something's still holding the front buffer\n",This);
        }
    }

    if(This->backBuffer) {
        int i;
        for(i = 0; i < This->presentParms.BackBufferCount; i++) {
            IWineD3DSurface_SetContainer(This->backBuffer[i], 0);
            if(D3DCB_DestroyRenderTarget(This->backBuffer[i]) > 0) {
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
    if(This->presentParms.Windowed == FALSE) {
        mode.Width = This->orig_width;
        mode.Height = This->orig_height;
        mode.RefreshRate = 0;
        mode.Format = This->orig_fmt;
        IWineD3DDevice_SetDisplayMode((IWineD3DDevice *) This->wineD3DDevice, 0, &mode);
    }
    for(i = 0; i < This->num_contexts; i++) {
        DestroyContext(This->wineD3DDevice, This->context[i]);
    }
    HeapFree(GetProcessHeap(), 0, This->context);

    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI IWineD3DSwapChainImpl_Present(IWineD3DSwapChain *iface, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion, DWORD dwFlags) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    unsigned int sync;
    int retval;


    ActivateContext(This->wineD3DDevice, This->backBuffer[0], CTXUSAGE_RESOURCELOAD);

    /* Render the cursor onto the back buffer, using our nifty directdraw blitting code :-) */
    if(This->wineD3DDevice->bCursorVisible && This->wineD3DDevice->cursorTexture) {
        IWineD3DSurfaceImpl cursor;
        RECT destRect = {This->wineD3DDevice->xScreenSpace - This->wineD3DDevice->xHotSpot,
                         This->wineD3DDevice->yScreenSpace - This->wineD3DDevice->yHotSpot,
                         This->wineD3DDevice->xScreenSpace + This->wineD3DDevice->cursorWidth - This->wineD3DDevice->xHotSpot,
                         This->wineD3DDevice->yScreenSpace + This->wineD3DDevice->cursorHeight - This->wineD3DDevice->yHotSpot};
        TRACE("Rendering the cursor. Creating fake surface at %p\n", &cursor);
        /* Build a fake surface to call the Blitting code. It is not possible to use the interface passed by
         * the application because we are only supposed to copy the information out. Using a fake surface
         * allows to use the Blitting engine and avoid copying the whole texture -> render target blitting code.
         */
        memset(&cursor, 0, sizeof(cursor));
        cursor.lpVtbl = &IWineD3DSurface_Vtbl;
        cursor.resource.ref = 1;
        cursor.resource.wineD3DDevice = This->wineD3DDevice;
        cursor.resource.pool = WINED3DPOOL_SCRATCH;
        cursor.resource.format = WINED3DFMT_A8R8G8B8;
        cursor.resource.resourceType = WINED3DRTYPE_SURFACE;
        cursor.glDescription.textureName = This->wineD3DDevice->cursorTexture;
        cursor.glDescription.target = GL_TEXTURE_2D;
        cursor.glDescription.level = 0;
        cursor.currentDesc.Width = This->wineD3DDevice->cursorWidth;
        cursor.currentDesc.Height = This->wineD3DDevice->cursorHeight;
        cursor.glRect.left = 0;
        cursor.glRect.top = 0;
        cursor.glRect.right = cursor.currentDesc.Width;
        cursor.glRect.bottom = cursor.currentDesc.Height;
        /* The cursor must have pow2 sizes */
        cursor.pow2Width = cursor.currentDesc.Width;
        cursor.pow2Height = cursor.currentDesc.Height;
        /* The surface is in the texture */
        cursor.Flags |= SFLAG_INTEXTURE;
        /* DDBLT_KEYSRC will cause BltOverride to enable the alpha test with GL_NOTEQUAL, 0.0,
         * which is exactly what we want :-)
         */
        if (This->presentParms.Windowed) {
            MapWindowPoints(NULL, This->win_handle, (LPPOINT)&destRect, 2);
        }
        IWineD3DSurface_Blt(This->backBuffer[0], &destRect, (IWineD3DSurface *) &cursor, NULL, WINEDDBLT_KEYSRC, NULL, WINED3DTEXF_NONE);
    }
    if(This->wineD3DDevice->logo_surface) {
        /* Blit the logo into the upper left corner of the drawable */
        IWineD3DSurface_BltFast(This->backBuffer[0], 0, 0, This->wineD3DDevice->logo_surface, NULL, WINEDDBLTFAST_SRCCOLORKEY);
    }

    if (pSourceRect || pDestRect) FIXME("Unhandled present options %p/%p\n", pSourceRect, pDestRect);
    /* TODO: If only source rect or dest rect are supplied then clip the window to match */
    TRACE("preseting HDC %p\n", This->context[0]->hdc);

    /* Don't call checkGLcall, as glGetError is not applicable here */
    if (hDestWindowOverride && This->win_handle != hDestWindowOverride) {
        WINED3DLOCKED_RECT r;
        BYTE *mem;

        TRACE("Performing dest override of swapchain %p from window %p to %p\n", This, This->win_handle, hDestWindowOverride);
        if(This->context[0] == This->wineD3DDevice->contexts[0]) {
            /* The primary context 'owns' all the opengl resources. Destroying and recreating that context would require downloading
             * all opengl resources, deleting the gl resources, destroying all other contexts, then recreating all other contexts
             * and reload the resources
             */
            ERR("Cannot change the destination window of the owner of the primary context\n");
        } else {
            This->win_handle             = hDestWindowOverride;

            /* The old back buffer has to be copied over to the new back buffer. A lockrect - switchcontext - unlockrect
             * would suffice in theory, but it is rather nasty and may cause troubles with future changes of the locking code
             * So lock read only, copy the surface out, then lock with the discard flag and write back
             */
            IWineD3DSurface_LockRect(This->backBuffer[0], &r, NULL, WINED3DLOCK_READONLY);
            mem = HeapAlloc(GetProcessHeap(), 0, r.Pitch * ((IWineD3DSurfaceImpl *) This->backBuffer[0])->currentDesc.Height);
            memcpy(mem, r.pBits, r.Pitch * ((IWineD3DSurfaceImpl *) This->backBuffer[0])->currentDesc.Height);
            IWineD3DSurface_UnlockRect(This->backBuffer[0]);

            DestroyContext(This->wineD3DDevice, This->context[0]);
            This->context[0] = CreateContext(This->wineD3DDevice, (IWineD3DSurfaceImpl *) This->frontBuffer, This->win_handle, FALSE /* pbuffer */, &This->presentParms);

            IWineD3DSurface_LockRect(This->backBuffer[0], &r, NULL, WINED3DLOCK_DISCARD);
            memcpy(r.pBits, mem, r.Pitch * ((IWineD3DSurfaceImpl *) This->backBuffer[0])->currentDesc.Height);
            HeapFree(GetProcessHeap(), 0, mem);
            IWineD3DSurface_UnlockRect(This->backBuffer[0]);
        }
    }

    SwapBuffers(This->context[0]->hdc); /* TODO: cycle through the swapchain buffers */

    TRACE("SwapBuffers called, Starting new frame\n");
    /* FPS support */
    if (TRACE_ON(fps))
    {
        DWORD time = GetTickCount();
        This->frames++;
        /* every 1.5 seconds */
        if (time - This->prev_time > 1500) {
            TRACE_(fps)("%p @ approx %.2ffps\n", This, 1000.0*This->frames/(time - This->prev_time));
            This->prev_time = time;
            This->frames = 0;
        }
    }

#if defined(FRAME_DEBUGGING)
{
    if (GetFileAttributesA("C:\\D3DTRACE") != INVALID_FILE_ATTRIBUTES) {
        if (!isOn) {
            isOn = TRUE;
            FIXME("Enabling D3D Trace\n");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 1);
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Singe Frame snapshots Starting\n");
            isDumpingFrames = TRUE;
            ENTER_GL();
            glClear(GL_COLOR_BUFFER_BIT);
            LEAVE_GL();
#endif

#if defined(SINGLE_FRAME_DEBUGGING)
        } else {
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Singe Frame snapshots Finishing\n");
            isDumpingFrames = FALSE;
#endif
            FIXME("Singe Frame trace complete\n");
            DeleteFileA("C:\\D3DTRACE");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 0);
#endif
        }
    } else {
        if (isOn) {
            isOn = FALSE;
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Single Frame snapshots Finishing\n");
            isDumpingFrames = FALSE;
#endif
            FIXME("Disabling D3D Trace\n");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 0);
        }
    }
}
#endif

    if (This->presentParms.SwapEffect == WINED3DSWAPEFFECT_DISCARD) {
        TRACE("Clearing the color buffer with pink color\n");

        IWineD3DDevice_Clear((IWineD3DDevice*)This->wineD3DDevice, 0, NULL,
                              WINED3DCLEAR_TARGET, 0xff00ffff, 1.0, 0);
    }

    if(((IWineD3DSurfaceImpl *) This->frontBuffer)->Flags   & SFLAG_INSYSMEM ||
       ((IWineD3DSurfaceImpl *) This->backBuffer[0])->Flags & SFLAG_INSYSMEM ) {
        /* Both memory copies of the surfaces are ok, flip them around too instead of dirtifying */
        IWineD3DSurfaceImpl *front = (IWineD3DSurfaceImpl *) This->frontBuffer;
        IWineD3DSurfaceImpl *back = (IWineD3DSurfaceImpl *) This->backBuffer[0];
        BOOL frontuptodate = front->Flags & SFLAG_INSYSMEM;
        BOOL backuptodate = back->Flags & SFLAG_INSYSMEM;

        if(front->resource.size == back->resource.size) {
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
                BOOL hasDib = front->Flags & SFLAG_DIBSECTION;
                tmp = front->dib.DIBsection;
                front->dib.DIBsection = back->dib.DIBsection;
                back->dib.DIBsection = tmp;

                if(back->Flags & SFLAG_DIBSECTION) front->Flags |= SFLAG_DIBSECTION;
                else front->Flags &= ~SFLAG_DIBSECTION;
                if(hasDib) back->Flags |= SFLAG_DIBSECTION;
                else back->Flags &= ~SFLAG_DIBSECTION;
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

                tmp = front->resource.heapMemory;
                front->resource.heapMemory = back->resource.heapMemory;
                back->resource.heapMemory = tmp;
            }

            /* Flip the PBO */
            {
                DWORD tmp_flags = front->Flags;

                GLuint tmp_pbo = front->pbo;
                front->pbo = back->pbo;
                back->pbo = tmp_pbo;

                if(back->Flags & SFLAG_PBO)
                    front->Flags |= SFLAG_PBO;
                else
                    front->Flags &= ~SFLAG_PBO;

                if(tmp_flags & SFLAG_PBO)
                    back->Flags |= SFLAG_PBO;
                else
                    back->Flags &= ~SFLAG_PBO;
            }

            /* client_memory should not be different, but just in case */
            {
                BOOL tmp;
                tmp = front->dib.client_memory;
                front->dib.client_memory = back->dib.client_memory;
                back->dib.client_memory = tmp;
            }
            if(frontuptodate) back->Flags |= SFLAG_INSYSMEM;
            else back->Flags &= ~SFLAG_INSYSMEM;
            if(backuptodate) front->Flags |= SFLAG_INSYSMEM;
            else front->Flags &= ~SFLAG_INSYSMEM;
        } else {
            IWineD3DSurface_ModifyLocation((IWineD3DSurface *) front, SFLAG_INDRAWABLE, TRUE);
            IWineD3DSurface_ModifyLocation((IWineD3DSurface *) back, SFLAG_INDRAWABLE, TRUE);
        }
    }

    if(This->presentParms.PresentationInterval != WINED3DPRESENT_INTERVAL_IMMEDIATE && GL_SUPPORT(SGI_VIDEO_SYNC)) {
        retval = GL_EXTCALL(glXGetVideoSyncSGI(&sync));
        if(retval != 0) {
            ERR("glXGetVideoSyncSGI failed(retval = %d\n", retval);
        }

        switch(This->presentParms.PresentationInterval) {
            case WINED3DPRESENT_INTERVAL_DEFAULT:
            case WINED3DPRESENT_INTERVAL_ONE:
                if(sync <= This->vSyncCounter) {
                    retval = GL_EXTCALL(glXWaitVideoSyncSGI(1, 0, &This->vSyncCounter));
                } else {
                    This->vSyncCounter = sync;
                }
                break;
            case WINED3DPRESENT_INTERVAL_TWO:
                if(sync <= This->vSyncCounter + 1) {
                    retval = GL_EXTCALL(glXWaitVideoSyncSGI(2, This->vSyncCounter & 0x1, &This->vSyncCounter));
                } else {
                    This->vSyncCounter = sync;
                }
                break;
            case WINED3DPRESENT_INTERVAL_THREE:
                if(sync <= This->vSyncCounter + 2) {
                    retval = GL_EXTCALL(glXWaitVideoSyncSGI(3, This->vSyncCounter % 0x3, &This->vSyncCounter));
                } else {
                    This->vSyncCounter = sync;
                }
                break;
            case WINED3DPRESENT_INTERVAL_FOUR:
                if(sync <= This->vSyncCounter + 3) {
                    retval = GL_EXTCALL(glXWaitVideoSyncSGI(4, This->vSyncCounter & 0x3, &This->vSyncCounter));
                } else {
                    This->vSyncCounter = sync;
                }
                break;
            default:
                FIXME("Unknown presentation interval %08x\n", This->presentParms.PresentationInterval);
        }
    }

    TRACE("returning\n");
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetFrontBufferData(IWineD3DSwapChain *iface, IWineD3DSurface *pDestSurface) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    POINT start;

    TRACE("(%p) : iface(%p) pDestSurface(%p)\n", This, iface, pDestSurface);

    start.x = 0;
    start.y = 0;

    if (This->presentParms.Windowed) {
        MapWindowPoints(This->win_handle, NULL, &start, 1);
    }

    IWineD3DSurface_BltFast(pDestSurface, start.x, start.y, This->frontBuffer, NULL, 0);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetBackBuffer(IWineD3DSwapChain *iface, UINT iBackBuffer, WINED3DBACKBUFFER_TYPE Type, IWineD3DSurface **ppBackBuffer) {

    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;

    if (iBackBuffer > This->presentParms.BackBufferCount - 1) {
        TRACE("Back buffer count out of range\n");
        /* Native d3d9 doesn't set NULL here, just as wine's d3d9. But set it here
         * in wined3d to avoid problems in other libs
         */
        *ppBackBuffer = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    *ppBackBuffer = This->backBuffer[iBackBuffer];
    TRACE("(%p) : BackBuf %d Type %d  returning %p\n", This, iBackBuffer, Type, *ppBackBuffer);

    /* Note inc ref on returned surface */
    if(*ppBackBuffer) IWineD3DSurface_AddRef(*ppBackBuffer);
    return WINED3D_OK;

}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetRasterStatus(IWineD3DSwapChain *iface, WINED3DRASTER_STATUS *pRasterStatus) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    static BOOL showFixmes = TRUE;
    pRasterStatus->InVBlank = TRUE;
    pRasterStatus->ScanLine = 0;
    /* No openGL equivalent */
    if(showFixmes) {
        FIXME("(%p) : stub (once)\n", This);
        showFixmes = FALSE;
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetDisplayMode(IWineD3DSwapChain *iface, WINED3DDISPLAYMODE*pMode) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    HDC                 hdc;
    int                 bpp = 0;

    pMode->Width        = GetSystemMetrics(SM_CXSCREEN);
    pMode->Height       = GetSystemMetrics(SM_CYSCREEN);
    pMode->RefreshRate  = 85; /* FIXME: How to identify? */

    hdc = GetDC(0);
    bpp = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);

    switch (bpp) {
    case  8: pMode->Format       = WINED3DFMT_R8G8B8; break;
    case 16: pMode->Format       = WINED3DFMT_R5G6B5; break;
    case 24: /*pMode->Format       = WINED3DFMT_R8G8B8; break; */ /* 32bpp and 24bpp can be aliased for X */
    case 32: pMode->Format       = WINED3DFMT_A8R8G8B8; break;
    default:
       FIXME("Unrecognized display mode format\n");
       pMode->Format       = WINED3DFMT_UNKNOWN;
    }

    TRACE("(%p) : returning w(%d) h(%d) rr(%d) fmt(%u,%s)\n", This, pMode->Width, pMode->Height, pMode->RefreshRate,
    pMode->Format, debug_d3dformat(pMode->Format));
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetDevice(IWineD3DSwapChain *iface, IWineD3DDevice**ppDevice) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;

    *ppDevice = (IWineD3DDevice *) This->wineD3DDevice;

    /* Note  Calling this method will increase the internal reference count
       on the IDirect3DDevice9 interface. */
    IWineD3DDevice_AddRef(*ppDevice);
    TRACE("(%p) : returning %p\n", This, *ppDevice);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetPresentParameters(IWineD3DSwapChain *iface, WINED3DPRESENT_PARAMETERS *pPresentationParameters) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    TRACE("(%p)\n", This);

    *pPresentationParameters = This->presentParms;

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_SetGammaRamp(IWineD3DSwapChain *iface, DWORD Flags, CONST WINED3DGAMMARAMP *pRamp){

    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    HDC hDC;
    TRACE("(%p) : pRamp@%p flags(%d)\n", This, pRamp, Flags);
    hDC = GetDC(This->win_handle);
    SetDeviceGammaRamp(hDC, (LPVOID)pRamp);
    ReleaseDC(This->win_handle, hDC);
    return WINED3D_OK;

}

static HRESULT WINAPI IWineD3DSwapChainImpl_GetGammaRamp(IWineD3DSwapChain *iface, WINED3DGAMMARAMP *pRamp){

    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    HDC hDC;
    TRACE("(%p) : pRamp@%p\n", This, pRamp);
    hDC = GetDC(This->win_handle);
    GetDeviceGammaRamp(hDC, pRamp);
    ReleaseDC(This->win_handle, hDC);
    return WINED3D_OK;

}


const IWineD3DSwapChainVtbl IWineD3DSwapChain_Vtbl =
{
    /* IUnknown */
    IWineD3DSwapChainImpl_QueryInterface,
    IWineD3DSwapChainImpl_AddRef,
    IWineD3DSwapChainImpl_Release,
    /* IWineD3DSwapChain */
    IWineD3DSwapChainImpl_GetParent,
    IWineD3DSwapChainImpl_Destroy,
    IWineD3DSwapChainImpl_GetDevice,
    IWineD3DSwapChainImpl_Present,
    IWineD3DSwapChainImpl_GetFrontBufferData,
    IWineD3DSwapChainImpl_GetBackBuffer,
    IWineD3DSwapChainImpl_GetRasterStatus,
    IWineD3DSwapChainImpl_GetDisplayMode,
    IWineD3DSwapChainImpl_GetPresentParameters,
    IWineD3DSwapChainImpl_SetGammaRamp,
    IWineD3DSwapChainImpl_GetGammaRamp
};

WineD3DContext *IWineD3DSwapChainImpl_CreateContextForThread(IWineD3DSwapChain *iface) {
    WineD3DContext *ctx;
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *) iface;
    WineD3DContext **newArray;

    TRACE("Creating a new context for swapchain %p, thread %d\n", This, GetCurrentThreadId());

    ctx = CreateContext(This->wineD3DDevice, (IWineD3DSurfaceImpl *) This->frontBuffer,
                        This->context[0]->win_handle, FALSE /* pbuffer */, &This->presentParms);
    if(!ctx) {
        ERR("Failed to create a new context for the swapchain\n");
        return NULL;
    }

    newArray = HeapAlloc(GetProcessHeap(), 0, sizeof(*newArray) * This->num_contexts + 1);
    if(!newArray) {
        ERR("Out of memory when trying to allocate a new context array\n");
        DestroyContext(This->wineD3DDevice, ctx);
        return NULL;
    }
    memcpy(newArray, This->context, sizeof(*newArray) * This->num_contexts);
    HeapFree(GetProcessHeap(), 0, This->context);
    newArray[This->num_contexts] = ctx;
    This->context = newArray;
    This->num_contexts++;

    TRACE("Returning context %p\n", ctx);
    return ctx;
}
