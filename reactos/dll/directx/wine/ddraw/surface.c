/* DirectDraw Surface Implementation
 *
 * Copyright (c) 1997-2000 Marcus Meissner
 * Copyright (c) 1998-2000 Lionel Ulmer
 * Copyright (c) 2000-2001 TransGaming Technologies Inc.
 * Copyright (c) 2006 Stefan Dösinger
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "wine/exception.h"

#include "ddraw.h"
#include "d3d.h"

#include "ddraw_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/*****************************************************************************
 * IUnknown parts follow
 *****************************************************************************/

/*****************************************************************************
 * IDirectDrawSurface7::QueryInterface
 *
 * A normal QueryInterface implementation. For QueryInterface rules
 * see ddraw.c, IDirectDraw7::QueryInterface. This method
 * can Query IDirectDrawSurface interfaces in all version, IDirect3DTexture
 * in all versions, the IDirectDrawGammaControl interface and it can
 * create an IDirect3DDevice. (Uses IDirect3D7::CreateDevice)
 *
 * Params:
 *  riid: The interface id queried for
 *  obj: Address to write the pointer to
 *
 * Returns:
 *  S_OK on success
 *  E_NOINTERFACE if the requested interface wasn't found
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_QueryInterface(IDirectDrawSurface7 *iface,
                                      REFIID riid,
                                      void **obj)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    /* According to COM docs, if the QueryInterface fails, obj should be set to NULL */
    *obj = NULL;

    if(!riid)
        return DDERR_INVALIDPARAMS;

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),obj);
    if (IsEqualGUID(riid, &IID_IUnknown)
     || IsEqualGUID(riid, &IID_IDirectDrawSurface7)
     || IsEqualGUID(riid, &IID_IDirectDrawSurface4) )
    {
        IUnknown_AddRef(iface);
        *obj = iface;
        TRACE("(%p) returning IDirectDrawSurface7 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_IDirectDrawSurface3)
          || IsEqualGUID(riid, &IID_IDirectDrawSurface2)
          || IsEqualGUID(riid, &IID_IDirectDrawSurface) )
    {
        IUnknown_AddRef(iface);
        *obj = &This->IDirectDrawSurface3_vtbl;
        TRACE("(%p) returning IDirectDrawSurface3 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_IDirectDrawGammaControl) )
    {
        IUnknown_AddRef(iface);
        *obj = &This->IDirectDrawGammaControl_vtbl;
        TRACE("(%p) returning IDirectDrawGammaControl interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_D3DDEVICE_WineD3D) ||
             IsEqualGUID(riid, &IID_IDirect3DHALDevice)||
             IsEqualGUID(riid, &IID_IDirect3DRGBDevice) )
    {
        IDirect3DDevice7 *d3d;

        /* Call into IDirect3D7 for creation */
        IDirect3D7_CreateDevice((IDirect3D7 *)&This->ddraw->IDirect3D7_vtbl, riid, (IDirectDrawSurface7 *)This, &d3d);

        *obj = d3d ? (IDirect3DDevice *)&((IDirect3DDeviceImpl *)d3d)->IDirect3DDevice_vtbl : NULL;
        TRACE("(%p) Returning IDirect3DDevice interface at %p\n", This, *obj);

        return S_OK;
    }
    else if (IsEqualGUID( &IID_IDirect3DTexture, riid ) ||
             IsEqualGUID( &IID_IDirect3DTexture2, riid ))
    {
        if (IsEqualGUID( &IID_IDirect3DTexture, riid ))
        {
            *obj = &This->IDirect3DTexture_vtbl;
            TRACE(" returning Direct3DTexture interface at %p.\n", *obj);
        }
        else
        {
            *obj = &This->IDirect3DTexture2_vtbl;
            TRACE(" returning Direct3DTexture2 interface at %p.\n", *obj);
        }
        IUnknown_AddRef( (IUnknown *) *obj);
        return S_OK;
    }

    ERR("No interface\n");
    return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirectDrawSurface7::AddRef
 *
 * A normal addref implementation
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirectDrawSurfaceImpl_AddRef(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef increasing from %d\n", This, refCount - 1);
    return refCount;
}

/*****************************************************************************
 * IDirectDrawSurfaceImpl_Destroy
 *
 * A helper function for IDirectDrawSurface7::Release
 *
 * Frees the surface, regardless of its refcount.
 *  See IDirectDrawSurface7::Release for more information
 *
 * Params:
 *  This: Surface to free
 *
 *****************************************************************************/
void IDirectDrawSurfaceImpl_Destroy(IDirectDrawSurfaceImpl *This)
{
    TRACE("(%p)\n", This);

    /* Check the refcount and give a warning */
    if(This->ref > 1)
    {
        /* This can happen when a complex surface is destroyed,
         * because the 2nd surface was addref()ed when the app
         * called GetAttachedSurface
         */
        WARN("(%p): Destroying surface with refount %d\n", This, This->ref);
    }

    /* Check for attached surfaces and detach them */
    if(This->first_attached != This)
    {
        /* Well, this shouldn't happen: The surface being attached is addref()ed
          * in AddAttachedSurface, so it shouldn't be released until DeleteAttachedSurface
          * is called, because the refcount is held. It looks like the app released()
          * it often enough to force this
          */
        IDirectDrawSurface7 *root = (IDirectDrawSurface7 *)This->first_attached;
        IDirectDrawSurface7 *detach = (IDirectDrawSurface7 *)This;

        FIXME("(%p) Freeing a surface that is attached to surface %p\n", This, This->first_attached);

        /* The refcount will drop to -1 here */
        if(IDirectDrawSurface7_DeleteAttachedSurface(root, 0, detach) != DD_OK)
        {
            ERR("(%p) DeleteAttachedSurface failed!\n", This);
        }
    }

    while(This->next_attached != NULL)
    {
        IDirectDrawSurface7 *root = (IDirectDrawSurface7 *)This;
        IDirectDrawSurface7 *detach = (IDirectDrawSurface7 *)This->next_attached;

        if(IDirectDrawSurface7_DeleteAttachedSurface(root, 0, detach) != DD_OK)
        {
            ERR("(%p) DeleteAttachedSurface failed!\n", This);
            assert(0);
        }
    }

    /* Now destroy the surface. Wait: It could have been released if we are a texture */
    if(This->WineD3DSurface)
        IWineD3DSurface_Release(This->WineD3DSurface);

    /* Having a texture handle set implies that the device still exists */
    if(This->Handle)
    {
        This->ddraw->d3ddevice->Handles[This->Handle - 1].ptr = NULL;
        This->ddraw->d3ddevice->Handles[This->Handle - 1].type = DDrawHandle_Unknown;
    }

    /* Reduce the ddraw surface count */
    InterlockedDecrement(&This->ddraw->surfaces);
    list_remove(&This->surface_list_entry);

    HeapFree(GetProcessHeap(), 0, This);
}

/*****************************************************************************
 * IDirectDrawSurface7::Release
 *
 * Reduces the surface's refcount by 1. If the refcount falls to 0, the
 * surface is destroyed.
 *
 * Destroying the surface is a bit tricky. For the connection between
 * WineD3DSurfaces and DirectDrawSurfaces see IDirectDraw7::CreateSurface
 * It has a nice graph explaining the connection.
 *
 * What happens here is basically this:
 * When a surface is destroyed, its WineD3DSurface is released,
 * and the refcount of the DirectDraw interface is reduced by 1. If it has
 * complex surfaces attached to it, then these surfaces are destroyed too,
 * regardless of their refcount. If any surface being destroyed has another
 * surface attached to it (with a "soft" attachment, not complex), then
 * this surface is detached with DeleteAttachedSurface.
 *
 * When the surface is a texture, the WineD3DTexture is released.
 * If the surface is the Direct3D render target, then the D3D
 * capabilities of the WineD3DDevice are uninitialized, which causes the
 * swapchain to be released.
 *
 * When a complex sublevel falls to ref zero, then this is ignored.
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirectDrawSurfaceImpl_Release(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
    {

        IDirectDrawSurfaceImpl *surf;
        IDirectDrawImpl *ddraw;
        IUnknown *ifaceToRelease = This->ifaceToRelease;
        UINT i;

        /* Complex attached surfaces are destroyed implicitly when the root is released */
        EnterCriticalSection(&ddraw_cs);
        if(!This->is_complex_root)
        {
            WARN("(%p) Attempt to destroy a surface that is not a complex root\n", This);
            LeaveCriticalSection(&ddraw_cs);
            return ref;
        }
        ddraw = This->ddraw;

        /* If it's a texture, destroy the WineD3DTexture.
         * WineD3D will destroy the IParent interfaces
         * of the sublevels, which destroys the WineD3DSurfaces.
         * Set the surfaces to NULL to avoid destroying them again later
         */
        if(This->wineD3DTexture)
        {
            IWineD3DBaseTexture_Release(This->wineD3DTexture);
        }
        /* If it's the RenderTarget, destroy the d3ddevice */
        else if(This->wineD3DSwapChain)
        {
            if((ddraw->d3d_initialized) && (This == ddraw->d3d_target)) {
                TRACE("(%p) Destroying the render target, uninitializing D3D\n", This);

                /* Unset any index buffer, just to be sure */
                IWineD3DDevice_SetIndices(ddraw->wineD3DDevice, NULL, WINED3DFMT_UNKNOWN);
                IWineD3DDevice_SetDepthStencilSurface(ddraw->wineD3DDevice, NULL);
                IWineD3DDevice_SetVertexDeclaration(ddraw->wineD3DDevice, NULL);
                for(i = 0; i < ddraw->numConvertedDecls; i++)
                {
                    IWineD3DVertexDeclaration_Release(ddraw->decls[i].decl);
                }
                HeapFree(GetProcessHeap(), 0, ddraw->decls);
                ddraw->numConvertedDecls = 0;

                if(IWineD3DDevice_Uninit3D(ddraw->wineD3DDevice, D3D7CB_DestroyDepthStencilSurface, D3D7CB_DestroySwapChain) != D3D_OK)
                {
                    /* Not good */
                    ERR("(%p) Failed to uninit 3D\n", This);
                }
                else
                {
                    /* Free the d3d window if one was created */
                    if(ddraw->d3d_window != 0 && ddraw->d3d_window != ddraw->dest_window)
                    {
                        TRACE(" (%p) Destroying the hidden render window %p\n", This, ddraw->d3d_window);
                        DestroyWindow(ddraw->d3d_window);
                        ddraw->d3d_window = 0;
                    }
                    /* Unset the pointers */
                }

                This->wineD3DSwapChain = NULL; /* Uninit3D releases the swapchain */
                ddraw->d3d_initialized = FALSE;
                ddraw->d3d_target = NULL;
            } else {
                IWineD3DDevice_UninitGDI(ddraw->wineD3DDevice, D3D7CB_DestroySwapChain);
                This->wineD3DSwapChain = NULL;
            }

            /* Reset to the default surface implementation type. This is needed if apps use
             * non render target surfaces and expect blits to work after destroying the render
             * target.
             *
             * TODO: Recreate existing offscreen surfaces
             */
            ddraw->ImplType = DefaultSurfaceType;

            /* Write a trace because D3D unloading was the reason for many
             * crashes during development.
             */
            TRACE("(%p) D3D unloaded\n", This);
        }
        else if(This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE |
                                                     DDSCAPS_3DDEVICE       |
                                                     DDSCAPS_TEXTURE        ) )
        {
            /* It's a render target, but no swapchain was created.
             * The IParent interfaces have to be released manually.
             * The same applies for textures without an
             * IWineD3DTexture object attached
             */
            IParent *Parent;

            for(i = 0; i < MAX_COMPLEX_ATTACHED; i++)
            {
                if(This->complex_array[i])
                {
                    /* Only the topmost level can have more than 1 surfaces in the complex
                     * attachment array(Cube texture roots), for all others there is only
                     * one
                     */
                    surf = This->complex_array[i];
                    while(surf)
                    {
                        IWineD3DSurface_GetParent(surf->WineD3DSurface,
                                                  (IUnknown **) &Parent);
                        IParent_Release(Parent);  /* For the getParent */
                        IParent_Release(Parent);  /* To release it */
                        surf = surf->complex_array[0];
                    }
                }
            }

            /* Now the top-level surface */
            IWineD3DSurface_GetParent(This->WineD3DSurface,
                                      (IUnknown **) &Parent);
            IParent_Release(Parent);  /* For the getParent */
            IParent_Release(Parent);  /* To release it */
        }

        /* The refcount test shows that the palette is detached when the surface is destroyed */
        IDirectDrawSurface7_SetPalette((IDirectDrawSurface7 *)This, NULL);

        /* Loop through all complex attached surfaces,
         * and destroy them.
         *
         * Yet again, only the root can have more than one complexly attached surface, all the others
         * have a total of one;
         */
        for(i = 0; i < MAX_COMPLEX_ATTACHED; i++)
        {
            if(!This->complex_array[i]) break;

            surf = This->complex_array[i];
            This->complex_array[i] = NULL;
            while(surf)
            {
                IDirectDrawSurfaceImpl *destroy = surf;
                surf = surf->complex_array[0];              /* Iterate through the "tree" */
                IDirectDrawSurfaceImpl_Destroy(destroy);    /* Destroy it */
            }
        }

        /* Destroy the root surface.
         */
        IDirectDrawSurfaceImpl_Destroy(This);

        /* Reduce the ddraw refcount */
        if(ifaceToRelease) IUnknown_Release(ifaceToRelease);
        LeaveCriticalSection(&ddraw_cs);
    }

    return ref;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetAttachedSurface
 *
 * Returns an attached surface with the requested caps. Surface attachment
 * and complex surfaces are not clearly described by the MSDN or sdk,
 * so this method is tricky and likely to contain problems.
 * This implementation searches the complex list first, then the
 * attachment chain.
 *
 * The chains are searched from This down to the last surface in the chain,
 * not from the first element in the chain. The first surface found is
 * returned. The MSDN says that this method fails if more than one surface
 * matches the caps, but it is not sure if that is right. The attachment
 * structure may not even allow two matching surfaces.
 *
 * The found surface is AddRef-ed before it is returned.
 *
 * Params:
 *  Caps: Pointer to a DDCAPS2 structure describing the caps asked for
 *  Surface: Address to store the found surface
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if Caps or Surface is NULL
 *  DDERR_NOTFOUND if no surface was found
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetAttachedSurface(IDirectDrawSurface7 *iface,
                                          DDSCAPS2 *Caps,
                                          IDirectDrawSurface7 **Surface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl *surf;
    DDSCAPS2 our_caps;
    int i;

    TRACE("(%p)->(%p,%p)\n", This, Caps, Surface);
    EnterCriticalSection(&ddraw_cs);

    if(This->version < 7)
    {
        /* Earlier dx apps put garbage into these members, clear them */
        our_caps.dwCaps = Caps->dwCaps;
        our_caps.dwCaps2 = 0;
        our_caps.dwCaps3 = 0;
        our_caps.u1.dwCaps4 = 0;
    }
    else
    {
        our_caps = *Caps;
    }

    TRACE("(%p): Looking for caps: %x,%x,%x,%x\n", This, our_caps.dwCaps, our_caps.dwCaps2, our_caps.dwCaps3, our_caps.u1.dwCaps4); /* FIXME: Better debugging */

    for(i = 0; i < MAX_COMPLEX_ATTACHED; i++)
    {
        surf = This->complex_array[i];
        if(!surf) break;

        if (TRACE_ON(ddraw))
        {
            TRACE("Surface: (%p) caps: %x,%x,%x,%x\n", surf,
                   surf->surface_desc.ddsCaps.dwCaps,
                   surf->surface_desc.ddsCaps.dwCaps2,
                   surf->surface_desc.ddsCaps.dwCaps3,
                   surf->surface_desc.ddsCaps.u1.dwCaps4);
        }

        if (((surf->surface_desc.ddsCaps.dwCaps & our_caps.dwCaps) == our_caps.dwCaps) &&
            ((surf->surface_desc.ddsCaps.dwCaps2 & our_caps.dwCaps2) == our_caps.dwCaps2)) {

            /* MSDN: "This method fails if more than one surface is attached
             * that matches the capabilities requested."
             *
             * Not sure how to test this.
             */

            TRACE("(%p): Returning surface %p\n", This, surf);
            TRACE("(%p): mipmapcount=%d\n", This, surf->mipmap_level);
            *Surface = (IDirectDrawSurface7 *)surf;
            IDirectDrawSurface7_AddRef(*Surface);
            LeaveCriticalSection(&ddraw_cs);
            return DD_OK;
        }
    }

    /* Next, look at the attachment chain */
    surf = This;

    while( (surf = surf->next_attached) )
    {
        if (TRACE_ON(ddraw))
        {
            TRACE("Surface: (%p) caps: %x,%x,%x,%x\n", surf,
                   surf->surface_desc.ddsCaps.dwCaps,
                   surf->surface_desc.ddsCaps.dwCaps2,
                   surf->surface_desc.ddsCaps.dwCaps3,
                   surf->surface_desc.ddsCaps.u1.dwCaps4);
        }

        if (((surf->surface_desc.ddsCaps.dwCaps & our_caps.dwCaps) == our_caps.dwCaps) &&
            ((surf->surface_desc.ddsCaps.dwCaps2 & our_caps.dwCaps2) == our_caps.dwCaps2)) {

            TRACE("(%p): Returning surface %p\n", This, surf);
            *Surface = (IDirectDrawSurface7 *)surf;
            IDirectDrawSurface7_AddRef(*Surface);
            LeaveCriticalSection(&ddraw_cs);
            return DD_OK;
        }
    }

    TRACE("(%p) Didn't find a valid surface\n", This);
    LeaveCriticalSection(&ddraw_cs);

    *Surface = NULL;
    return DDERR_NOTFOUND;
}

/*****************************************************************************
 * IDirectDrawSurface7::Lock
 *
 * Locks the surface and returns a pointer to the surface's memory
 *
 * Params:
 *  Rect: Rectangle to lock. If NULL, the whole surface is locked
 *  DDSD: Pointer to a DDSURFACEDESC2 which shall receive the surface's desc.
 *  Flags: Locking flags, e.g Read only or write only
 *  h: An event handle that's not used and must be NULL
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if DDSD is NULL
 *  For more details, see IWineD3DSurface::LockRect
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_Lock(IDirectDrawSurface7 *iface,
                            RECT *Rect,
                            DDSURFACEDESC2 *DDSD,
                            DWORD Flags,
                            HANDLE h)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    WINED3DLOCKED_RECT LockedRect;
    HRESULT hr;
    TRACE("(%p)->(%p,%p,%x,%p)\n", This, Rect, DDSD, Flags, h);

    if(!DDSD)
        return DDERR_INVALIDPARAMS;

    /* This->surface_desc.dwWidth and dwHeight are changeable, thus lock */
    EnterCriticalSection(&ddraw_cs);

    /* Should I check for the handle to be NULL?
     *
     * The DDLOCK flags and the D3DLOCK flags are equal
     * for the supported values. The others are ignored by WineD3D
     */

    if(DDSD->dwSize != sizeof(DDSURFACEDESC) &&
       DDSD->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Invalid structure size %d, returning DDERR_INVALIDPARAMS\n", DDERR_INVALIDPARAMS);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* Windows zeroes this if the rect is invalid */
    DDSD->lpSurface = 0;

    if (Rect)
    {
        if ((Rect->left < 0)
                || (Rect->top < 0)
                || (Rect->left > Rect->right)
                || (Rect->top > Rect->bottom)
                || (Rect->right > This->surface_desc.dwWidth)
                || (Rect->bottom > This->surface_desc.dwHeight))
        {
            WARN("Trying to lock an invalid rectangle, returning DDERR_INVALIDPARAMS\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }
    }

    hr = IWineD3DSurface_LockRect(This->WineD3DSurface,
                                  &LockedRect,
                                  Rect,
                                  Flags);
    if(hr != D3D_OK)
    {
        LeaveCriticalSection(&ddraw_cs);
        switch(hr)
        {
            /* D3D8 and D3D9 return the general D3DERR_INVALIDCALL error, but ddraw has a more
             * specific error. But since IWineD3DSurface::LockRect returns that error in this
             * only occasion, keep d3d8 and d3d9 free from the return value override. There are
             * many different places where d3d8/9 would have to catch the DDERR_SURFACEBUSY, it
             * is much easier to do it in one place in ddraw
             */
            case WINED3DERR_INVALIDCALL:    return DDERR_SURFACEBUSY;
            default:                        return hr;
        }
    }

    /* Override the memory area. The pitch should be set already. Strangely windows
     * does not set the LPSURFACE flag on locked surfaces !?!.
     * DDSD->dwFlags |= DDSD_LPSURFACE;
     */
    This->surface_desc.lpSurface = LockedRect.pBits;
    DD_STRUCT_COPY_BYSIZE(DDSD,&(This->surface_desc));

    TRACE("locked surface returning description :\n");
    if (TRACE_ON(ddraw)) DDRAW_dump_surface_desc(DDSD);

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::Unlock
 *
 * Unlocks an locked surface
 *
 * Params:
 *  Rect: Not used by this implementation
 *
 * Returns:
 *  D3D_OK on success
 *  For more details, see IWineD3DSurface::UnlockRect
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_Unlock(IDirectDrawSurface7 *iface,
                              RECT *pRect)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, pRect);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_UnlockRect(This->WineD3DSurface);
    if(SUCCEEDED(hr))
    {
        This->surface_desc.lpSurface = NULL;
    }
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::Flip
 *
 * Flips a surface with the DDSCAPS_FLIP flag. The flip is relayed to
 * IWineD3DSurface::Flip. Because WineD3D doesn't handle attached surfaces,
 * the flip target is passed to WineD3D, even if the app didn't specify one
 *
 * Params:
 *  DestOverride: Specifies the surface that will become the new front
 *                buffer. If NULL, the current back buffer is used
 *  Flags: some DirectDraw flags, see include/ddraw.h
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_NOTFLIPPABLE if no flip target could be found
 *  DDERR_INVALIDOBJECT if the surface isn't a front buffer
 *  For more details, see IWineD3DSurface::Flip
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_Flip(IDirectDrawSurface7 *iface,
                            IDirectDrawSurface7 *DestOverride,
                            DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl *Override = (IDirectDrawSurfaceImpl *)DestOverride;
    IDirectDrawSurface7 *Override7;
    HRESULT hr;
    TRACE("(%p)->(%p,%x)\n", This, DestOverride, Flags);

    /* Flip has to be called from a front buffer
     * What about overlay surfaces, AFAIK they can flip too?
     */
    if( !(This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_OVERLAY)) )
        return DDERR_INVALIDOBJECT; /* Unchecked */

    EnterCriticalSection(&ddraw_cs);

    /* WineD3D doesn't keep track of attached surface, so find the target */
    if(!Override)
    {
        DDSCAPS2 Caps;

        memset(&Caps, 0, sizeof(Caps));
        Caps.dwCaps |= DDSCAPS_BACKBUFFER;
        hr = IDirectDrawSurface7_GetAttachedSurface(iface, &Caps, &Override7);
        if(hr != DD_OK)
        {
            ERR("Can't find a flip target\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_NOTFLIPPABLE; /* Unchecked */
        }
        Override = (IDirectDrawSurfaceImpl *)Override7;

        /* For the GetAttachedSurface */
        IDirectDrawSurface7_Release(Override7);
    }

    hr = IWineD3DSurface_Flip(This->WineD3DSurface,
                              Override->WineD3DSurface,
                              Flags);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::Blt
 *
 * Performs a blit on the surface
 *
 * Params:
 *  DestRect: Destination rectangle, can be NULL
 *  SrcSurface: Source surface, can be NULL
 *  SrcRect: Source rectangle, can be NULL
 *  Flags: Blt flags
 *  DDBltFx: Some extended blt parameters, connected to the flags
 *
 * Returns:
 *  D3D_OK on success
 *  See IWineD3DSurface::Blt for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_Blt(IDirectDrawSurface7 *iface,
                           RECT *DestRect,
                           IDirectDrawSurface7 *SrcSurface,
                           RECT *SrcRect,
                           DWORD Flags,
                           DDBLTFX *DDBltFx)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl *Src = (IDirectDrawSurfaceImpl *)SrcSurface;
    HRESULT hr;
    TRACE("(%p)->(%p,%p,%p,%x,%p)\n", This, DestRect, Src, SrcRect, Flags, DDBltFx);

    /* Check for validity of the flags here. WineD3D Has the software-opengl selection path and would have
     * to check at 2 places, and sometimes do double checks. This also saves the call to wined3d :-)
     */
    if((Flags & DDBLT_KEYSRCOVERRIDE) && (!DDBltFx || Flags & DDBLT_KEYSRC)) {
        WARN("Invalid source color key parameters, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    if((Flags & DDBLT_KEYDESTOVERRIDE) && (!DDBltFx || Flags & DDBLT_KEYDEST)) {
        WARN("Invalid destination color key parameters, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    /* Sizes can change, therefore hold the lock when testing the rectangles */
    EnterCriticalSection(&ddraw_cs);
    if(DestRect)
    {
        if(DestRect->top >= DestRect->bottom || DestRect->left >= DestRect->right ||
           DestRect->right > This->surface_desc.dwWidth ||
           DestRect->bottom > This->surface_desc.dwHeight)
        {
            WARN("Destination rectangle is invalid, returning DDERR_INVALIDRECT\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDRECT;
        }
    }
    if(Src && SrcRect)
    {
        if(SrcRect->top >= SrcRect->bottom || SrcRect->left >=SrcRect->right ||
           SrcRect->right > Src->surface_desc.dwWidth ||
           SrcRect->bottom > Src->surface_desc.dwHeight)
        {
            WARN("Source rectangle is invalid, returning DDERR_INVALIDRECT\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDRECT;
        }
    }

    if(Flags & DDBLT_KEYSRC && (!Src || !(Src->surface_desc.dwFlags & DDSD_CKSRCBLT))) {
        WARN("DDBLT_KEYDEST blit without color key in surface, returning DDERR_INVALIDPARAMS\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* TODO: Check if the DDBltFx contains any ddraw surface pointers. If it does, copy the struct,
     * and replace the ddraw surfaces with the wined3d surfaces
     * So far no blitting operations using surfaces in the bltfx struct are supported anyway.
     */
    hr = IWineD3DSurface_Blt(This->WineD3DSurface,
                             DestRect,
                             Src ? Src->WineD3DSurface : NULL,
                             SrcRect,
                             Flags,
                             (WINEDDBLTFX *) DDBltFx,
                             WINED3DTEXF_POINT);

    LeaveCriticalSection(&ddraw_cs);
    switch(hr)
    {
        case WINED3DERR_NOTAVAILABLE:       return DDERR_UNSUPPORTED;
        case WINED3DERR_WRONGTEXTUREFORMAT: return DDERR_INVALIDPIXELFORMAT;
        default:                            return hr;
    }
}

/*****************************************************************************
 * IDirectDrawSurface7::AddAttachedSurface
 *
 * Attaches a surface to another surface. How the surface attachments work
 * is not totally understood yet, and this method is prone to problems.
 * he surface that is attached is AddRef-ed.
 *
 * Tests with complex surfaces suggest that the surface attachments form a
 * tree, but no method to test this has been found yet.
 *
 * The attachment list consists of a first surface (first_attached) and
 * for each surface a pointer to the next attached surface (next_attached).
 * For the first surface, and a surface that has no attachments
 * first_attached points to the surface itself. A surface that has
 * no successors in the chain has next_attached set to NULL.
 *
 * Newly attached surfaces are attached right after the root surface.
 * If a surface is attached to a complex surface compound, it's attached to
 * the surface that the app requested, not the complex root. See
 * GetAttachedSurface for a description how surfaces are found.
 *
 * This is how the current implementation works, and it was coded by looking
 * at the needs of the applications.
 *
 * So far only Z-Buffer attachments are tested, and they are activated in
 * WineD3D. Mipmaps could be tricky to activate in WineD3D.
 * Back buffers should work in 2D mode, but they are not tested(They can be
 * attached in older iface versions). Rendering to the front buffer and
 * switching between that and double buffering is not yet implemented in
 * WineD3D, so for 3D it might have unexpected results.
 *
 * IDirectDrawSurfaceImpl_AddAttachedSurface is the real thing,
 * IDirectDrawSurface7Impl_AddAttachedSurface is a wrapper around it that
 * performs additional checks. Version 7 of this interface is much more restrictive
 * than its predecessors.
 *
 * Params:
 *  Attach: Surface to attach to iface
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_CANNOTATTACHSURFACE if the surface can't be attached for some reason
 *
 *****************************************************************************/
HRESULT WINAPI
IDirectDrawSurfaceImpl_AddAttachedSurface(IDirectDrawSurfaceImpl *This,
                                          IDirectDrawSurfaceImpl *Surf)
{
    TRACE("(%p)->(%p)\n", This, Surf);

    if(Surf == This)
        return DDERR_CANNOTATTACHSURFACE; /* unchecked */

    EnterCriticalSection(&ddraw_cs);

    /* Check if the surface is already attached somewhere */
    if( (Surf->next_attached != NULL) ||
        (Surf->first_attached != Surf) )
    {
        /* TODO: Test for the structure of the manual attachment. Is it a chain or a list?
         * What happens if one surface is attached to 2 different surfaces?
         */
        FIXME("(%p) The Surface %p is already attached somewhere else: next_attached = %p, first_attached = %p, can't handle by now\n", This, Surf, Surf->next_attached, Surf->first_attached);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_SURFACEALREADYATTACHED;
    }

    /* This inserts the new surface at the 2nd position in the chain, right after the root surface */
    Surf->next_attached = This->next_attached;
    Surf->first_attached = This->first_attached;
    This->next_attached = Surf;

    /* Check if the WineD3D depth stencil needs updating */
    if(This->ddraw->d3ddevice)
    {
        IDirect3DDeviceImpl_UpdateDepthStencil(This->ddraw->d3ddevice);
    }

    /* MSDN: 
     * "This method increments the reference count of the surface being attached."
     */
    IDirectDrawSurface7_AddRef((IDirectDrawSurface7 *)Surf);
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI
IDirectDrawSurface7Impl_AddAttachedSurface(IDirectDrawSurface7 *iface,
                                           IDirectDrawSurface7 *Attach)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl *Surf = (IDirectDrawSurfaceImpl *)Attach;

    /* Version 7 of this interface seems to refuse everything except z buffers, as per msdn */
    if(!(Surf->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
    {

        WARN("Application tries to attach a non Z buffer surface. caps %08x\n",
              Surf->surface_desc.ddsCaps.dwCaps);
        return DDERR_CANNOTATTACHSURFACE;
    }

    return IDirectDrawSurfaceImpl_AddAttachedSurface(This,
                                                     Surf);
}
/*****************************************************************************
 * IDirectDrawSurface7::DeleteAttachedSurface
 *
 * Removes a surface from the attachment chain. The surface's refcount
 * is decreased by one after it has been removed
 *
 * Params:
 *  Flags: Some flags, not used by this implementation
 *  Attach: Surface to detach
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_SURFACENOTATTACHED if the surface isn't attached to
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_DeleteAttachedSurface(IDirectDrawSurface7 *iface,
                                             DWORD Flags,
                                             IDirectDrawSurface7 *Attach)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl *Surf = (IDirectDrawSurfaceImpl *)Attach;
    IDirectDrawSurfaceImpl *Prev = This;
    TRACE("(%p)->(%08x,%p)\n", This, Flags, Surf);

    EnterCriticalSection(&ddraw_cs);
    if (!Surf || (Surf->first_attached != This) || (Surf == This) )
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_CANNOTDETACHSURFACE;
    }

    /* Remove MIPMAPSUBLEVEL if this seemed to be one */
    if (This->surface_desc.ddsCaps.dwCaps &
        Surf->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        Surf->surface_desc.ddsCaps.dwCaps2 &= ~DDSCAPS2_MIPMAPSUBLEVEL;
        /* FIXME: we should probably also subtract from dwMipMapCount of this
         * and all parent surfaces */
    }

    /* Find the predecessor of the detached surface */
    while(Prev)
    {
        if(Prev->next_attached == Surf) break;
        Prev = Prev->next_attached;
    }

    /* There must be a surface, otherwise there's a bug */
    assert(Prev != NULL);

    /* Unchain the surface */
    Prev->next_attached = Surf->next_attached;
    Surf->next_attached = NULL;
    Surf->first_attached = Surf;

    /* Check if the WineD3D depth stencil needs updating */
    if(This->ddraw->d3ddevice)
    {
        IDirect3DDeviceImpl_UpdateDepthStencil(This->ddraw->d3ddevice);
    }

    IDirectDrawSurface7_Release(Attach);
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::AddOverlayDirtyRect
 *
 * "This method is not currently implemented"
 *
 * Params:
 *  Rect: ?
 *
 * Returns:
 *  DDERR_UNSUPPORTED
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_AddOverlayDirtyRect(IDirectDrawSurface7 *iface,
                                           LPRECT Rect)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    TRACE("(%p)->(%p)\n",This,Rect);

    /* MSDN says it's not implemented. I could forward it to WineD3D, 
     * then we'd implement it, but I don't think that's a good idea
     * (Stefan Dösinger)
     */
#if 0
    return IWineD3DSurface_AddOverlayDirtyRect(This->WineD3DSurface, pRect);
#endif
    return DDERR_UNSUPPORTED; /* unchecked */
}

/*****************************************************************************
 * IDirectDrawSurface7::GetDC
 *
 * Returns a GDI device context for the surface
 *
 * Params:
 *  hdc: Address of a HDC variable to store the dc to
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if hdc is NULL
 *  For details, see IWineD3DSurface::GetDC
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetDC(IDirectDrawSurface7 *iface,
                             HDC *hdc)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p): Relay\n", This, hdc);

    if(!hdc)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_GetDC(This->WineD3DSurface,
                               hdc);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::ReleaseDC
 *
 * Releases the DC that was constructed with GetDC
 *
 * Params:
 *  hdc: HDC to release
 *
 * Returns:
 *  DD_OK on success
 *  For more details, see IWineD3DSurface::ReleaseDC
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_ReleaseDC(IDirectDrawSurface7 *iface,
                                 HDC hdc)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p): Relay\n", This, hdc);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_ReleaseDC(This->WineD3DSurface, hdc);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetCaps
 *
 * Returns the surface's caps
 *
 * Params:
 *  Caps: Address to write the caps to
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if Caps is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetCaps(IDirectDrawSurface7 *iface,
                               DDSCAPS2 *Caps)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    TRACE("(%p)->(%p)\n",This,Caps);

    if(!Caps)
        return DDERR_INVALIDPARAMS;

    *Caps = This->surface_desc.ddsCaps;
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::SetPriority
 *
 * Sets a texture priority for managed textures.
 *
 * Params:
 *  Priority: The new priority
 *
 * Returns:
 *  DD_OK on success
 *  For more details, see IWineD3DSurface::SetPriority
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_SetPriority(IDirectDrawSurface7 *iface, DWORD Priority)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d): Relay!\n",This,Priority);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_SetPriority(This->WineD3DSurface, Priority);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetPriority
 *
 * Returns the surface's priority
 *
 * Params:
 *  Priority: Address of a variable to write the priority to
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Priority == NULL
 *  For more details, see IWineD3DSurface::GetPriority
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetPriority(IDirectDrawSurface7 *iface,
                                   DWORD *Priority)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    TRACE("(%p)->(%p): Relay\n",This,Priority);

    if(!Priority)
    {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&ddraw_cs);
    *Priority = IWineD3DSurface_GetPriority(This->WineD3DSurface);
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::SetPrivateData
 *
 * Stores some data in the surface that is intended for the application's
 * use.
 *
 * Params:
 *  tag: GUID that identifies the data
 *  Data: Pointer to the private data
 *  Size: Size of the private data
 *  Flags: Some flags
 *
 * Returns:
 *  D3D_OK on success
 *  For more details, see IWineD3DSurface::SetPrivateData
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_SetPrivateData(IDirectDrawSurface7 *iface,
                                      REFGUID tag,
                                      void *Data,
                                      DWORD Size,
                                      DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%s,%p,%d,%x): Relay\n", This, debugstr_guid(tag), Data, Size, Flags);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_SetPrivateData(This->WineD3DSurface,
                                        tag,
                                        Data,
                                        Size,
                                        Flags);
    LeaveCriticalSection(&ddraw_cs);
    switch(hr)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return hr;
    }
}

/*****************************************************************************
 * IDirectDrawSurface7::GetPrivateData
 *
 * Returns the private data set with IDirectDrawSurface7::SetPrivateData
 *
 * Params:
 *  tag: GUID of the data to return
 *  Data: Address where to write the data to
 *  Size: Size of the buffer at Data
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if Data is NULL
 *  For more details, see IWineD3DSurface::GetPrivateData
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetPrivateData(IDirectDrawSurface7 *iface,
                                      REFGUID tag,
                                      void *Data,
                                      DWORD *Size)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%s,%p,%p): Relay\n", This, debugstr_guid(tag), Data, Size);

    if(!Data)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_GetPrivateData(This->WineD3DSurface,
                                        tag,
                                        Data,
                                        Size);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::FreePrivateData
 *
 * Frees private data stored in the surface
 *
 * Params:
 *  tag: Tag of the data to free
 *
 * Returns:
 *  D3D_OK on success
 *  For more details, see IWineD3DSurface::FreePrivateData
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_FreePrivateData(IDirectDrawSurface7 *iface,
                                       REFGUID tag)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%s): Relay\n", This, debugstr_guid(tag));

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_FreePrivateData(This->WineD3DSurface, tag);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::PageLock
 *
 * Prevents a sysmem surface from being paged out
 *
 * Params:
 *  Flags: Not used, must be 0(unchecked)
 *
 * Returns:
 *  DD_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_PageLock(IDirectDrawSurface7 *iface,
                                DWORD Flags)
{
    TRACE("(%p)->(%x)\n", iface, Flags);

    /* This is Windows memory management related - we don't need this */
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::PageUnlock
 *
 * Allows a sysmem surface to be paged out
 *
 * Params:
 *  Flags: Not used, must be 0(unchecked)
 *
 * Returns:
 *  DD_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_PageUnlock(IDirectDrawSurface7 *iface,
                                  DWORD Flags)
{
    TRACE("(%p)->(%x)\n", iface, Flags);

    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::BltBatch
 *
 * An unimplemented function
 *
 * Params:
 *  ?
 *
 * Returns:
 *  DDERR_UNSUPPORTED
 *
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawSurfaceImpl_BltBatch(IDirectDrawSurface7 *iface, DDBLTBATCH *Batch, DWORD Count, DWORD Flags)
{
    TRACE("(%p)->(%p,%d,%08x)\n",iface,Batch,Count,Flags);

    /* MSDN: "not currently implemented" */
    return DDERR_UNSUPPORTED;
}

/*****************************************************************************
 * IDirectDrawSurface7::EnumAttachedSurfaces
 *
 * Enumerates all surfaces attached to this surface
 *
 * Params:
 *  context: Pointer to pass unmodified to the callback
 *  cb: Callback function to call for each surface
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if cb is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_EnumAttachedSurfaces(IDirectDrawSurface7 *iface,
                                            void *context,
                                            LPDDENUMSURFACESCALLBACK7 cb)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl *surf;
    DDSURFACEDESC2 desc;
    int i;

    /* Attached surfaces aren't handled in WineD3D */
    TRACE("(%p)->(%p,%p)\n",This,context,cb);

    if(!cb)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    for(i = 0; i < MAX_COMPLEX_ATTACHED; i++)
    {
        surf = This->complex_array[i];
        if(!surf) break;

        IDirectDrawSurface7_AddRef((IDirectDrawSurface7 *)surf);
        desc = surf->surface_desc;
        /* check: != DDENUMRET_OK or == DDENUMRET_CANCEL? */
        if (cb((IDirectDrawSurface7 *)surf, &desc, context) == DDENUMRET_CANCEL)
        {
            LeaveCriticalSection(&ddraw_cs);
            return DD_OK;
        }
    }

    for (surf = This->next_attached; surf != NULL; surf = surf->next_attached)
    {
        IDirectDrawSurface7_AddRef((IDirectDrawSurface7 *)surf);
        desc = surf->surface_desc;
        /* check: != DDENUMRET_OK or == DDENUMRET_CANCEL? */
        if (cb((IDirectDrawSurface7 *)surf, &desc, context) == DDENUMRET_CANCEL)
        {
            LeaveCriticalSection(&ddraw_cs);
            return DD_OK;
        }
    }

    TRACE(" end of enumeration.\n");

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::EnumOverlayZOrders
 *
 * "Enumerates the overlay surfaces on the specified destination"
 *
 * Params:
 *  Flags: DDENUMOVERLAYZ_BACKTOFRONT  or DDENUMOVERLAYZ_FRONTTOBACK
 *  context: context to pass back to the callback
 *  cb: callback function to call for each enumerated surface
 *
 * Returns:
 *  DD_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_EnumOverlayZOrders(IDirectDrawSurface7 *iface,
                                          DWORD Flags,
                                          void *context,
                                          LPDDENUMSURFACESCALLBACK7 cb)
{
     FIXME("(%p)->(%x,%p,%p): Stub!\n", iface, Flags, context, cb);

    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetBltStatus
 *
 * Returns the blitting status
 *
 * Params:
 *  Flags: DDGBS_CANBLT or DDGBS_ISBLTDONE
 *
 * Returns:
 *  See IWineD3DSurface::Blt
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetBltStatus(IDirectDrawSurface7 *iface,
                                    DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%x): Relay\n", This, Flags);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_GetBltStatus(This->WineD3DSurface, Flags);
    LeaveCriticalSection(&ddraw_cs);
    switch(hr)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return hr;
    }
}

/*****************************************************************************
 * IDirectDrawSurface7::GetColorKey
 *
 * Returns the color key assigned to the surface
 *
 * Params:
 *  Flags: Some flags
 *  CKey: Address to store the key to
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if CKey is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetColorKey(IDirectDrawSurface7 *iface,
                                   DWORD Flags,
                                   DDCOLORKEY *CKey)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    TRACE("(%p)->(%08x,%p)\n", This, Flags, CKey);

    if(!CKey)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);

    switch (Flags)
    {
    case DDCKEY_DESTBLT:
        if (!(This->surface_desc.dwFlags & DDSD_CKDESTBLT))
        {
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_NOCOLORKEY;
        }
        *CKey = This->surface_desc.ddckCKDestBlt;
        break;

    case DDCKEY_DESTOVERLAY:
        if (!(This->surface_desc.dwFlags & DDSD_CKDESTOVERLAY))
            {
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_NOCOLORKEY;
            }
        *CKey = This->surface_desc.u3.ddckCKDestOverlay;
        break;

    case DDCKEY_SRCBLT:
        if (!(This->surface_desc.dwFlags & DDSD_CKSRCBLT))
        {
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_NOCOLORKEY;
        }
        *CKey = This->surface_desc.ddckCKSrcBlt;
        break;

    case DDCKEY_SRCOVERLAY:
        if (!(This->surface_desc.dwFlags & DDSD_CKSRCOVERLAY))
        {
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_NOCOLORKEY;
        }
        *CKey = This->surface_desc.ddckCKSrcOverlay;
        break;

    default:
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetFlipStatus
 *
 * Returns the flipping status of the surface
 *
 * Params:
 *  Flags: DDGFS_CANFLIP of DDGFS_ISFLIPDONE
 *
 * Returns:
 *  See IWineD3DSurface::GetFlipStatus
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetFlipStatus(IDirectDrawSurface7 *iface,
                                     DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%x): Relay\n", This, Flags);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_GetFlipStatus(This->WineD3DSurface, Flags);
    LeaveCriticalSection(&ddraw_cs);
    switch(hr)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return hr;
    }
}

/*****************************************************************************
 * IDirectDrawSurface7::GetOverlayPosition
 *
 * Returns the display coordinates of a visible and active overlay surface
 *
 * Params:
 *  X
 *  Y
 *
 * Returns:
 *  DDERR_NOTAOVERLAYSURFACE, because it's a stub
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetOverlayPosition(IDirectDrawSurface7 *iface,
                                          LONG *X,
                                          LONG *Y) {
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p,%p): Relay\n", This, X, Y);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_GetOverlayPosition(This->WineD3DSurface,
                                            X,
                                            Y);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetPixelFormat
 *
 * Returns the pixel format of the Surface
 *
 * Params:
 *  PixelFormat: Pointer to a DDPIXELFORMAT structure to which the pixel
 *               format should be written
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if PixelFormat is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetPixelFormat(IDirectDrawSurface7 *iface,
                                      DDPIXELFORMAT *PixelFormat)
{
    /* What is DDERR_INVALIDSURFACETYPE for here? */
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    TRACE("(%p)->(%p)\n",This,PixelFormat);

    if(!PixelFormat)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    DD_STRUCT_COPY_BYSIZE(PixelFormat,&This->surface_desc.u4.ddpfPixelFormat);
    LeaveCriticalSection(&ddraw_cs);

    return DD_OK;
}


/*****************************************************************************
 * IDirectDrawSurface7::GetSurfaceDesc
 *
 * Returns the description of this surface
 *
 * Params:
 *  DDSD: Address of a DDSURFACEDESC2 structure that is to be filled with the
 *        surface desc
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if DDSD is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetSurfaceDesc(IDirectDrawSurface7 *iface,
                                      DDSURFACEDESC2 *DDSD)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,DDSD);

    if(!DDSD)
        return DDERR_INVALIDPARAMS;

    if (DDSD->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Incorrect struct size %d, returning DDERR_INVALIDPARAMS\n",DDSD->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&ddraw_cs);
    DD_STRUCT_COPY_BYSIZE(DDSD,&This->surface_desc);
    TRACE("Returning surface desc:\n");
    if (TRACE_ON(ddraw)) DDRAW_dump_surface_desc(DDSD);

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::Initialize
 *
 * Initializes the surface. This is a no-op in Wine
 *
 * Params:
 *  DD: Pointer to an DirectDraw interface
 *  DDSD: Surface description for initialization
 *
 * Returns:
 *  DDERR_ALREADYINITIALIZED
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_Initialize(IDirectDrawSurface7 *iface,
                                  IDirectDraw *DD,
                                  DDSURFACEDESC2 *DDSD)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawImpl *ddimpl = DD ? ddraw_from_ddraw1(DD) : NULL;
    TRACE("(%p)->(%p,%p)\n",This,ddimpl,DDSD);

    return DDERR_ALREADYINITIALIZED;
}

/*****************************************************************************
 * IDirectDrawSurface7::IsLost
 *
 * Checks if the surface is lost
 *
 * Returns:
 *  DD_OK, if the surface is usable
 *  DDERR_ISLOST if the surface is lost
 *  See IWineD3DSurface::IsLost for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_IsLost(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)\n", This);

    EnterCriticalSection(&ddraw_cs);
    /* We lose the surface if the implementation was changed */
    if(This->ImplType != This->ddraw->ImplType)
    {
        /* But this shouldn't happen. When we change the implementation,
         * all surfaces are re-created automatically, and their content
         * is copied
         */
        ERR(" (%p) Implementation was changed from %d to %d\n", This, This->ImplType, This->ddraw->ImplType);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_SURFACELOST;
    }

    hr = IWineD3DSurface_IsLost(This->WineD3DSurface);
    LeaveCriticalSection(&ddraw_cs);
    switch(hr)
    {
        /* D3D8 and 9 loose full devices, thus there's only a DEVICELOST error.
         * WineD3D uses the same error for surfaces
         */
        case WINED3DERR_DEVICELOST:         return DDERR_SURFACELOST;
        default:                            return hr;
    }
}

/*****************************************************************************
 * IDirectDrawSurface7::Restore
 *
 * Restores a lost surface. This makes the surface usable again, but
 * doesn't reload its old contents
 *
 * Returns:
 *  DD_OK on success
 *  See IWineD3DSurface::Restore for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_Restore(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)\n", This);

    EnterCriticalSection(&ddraw_cs);
    if(This->ImplType != This->ddraw->ImplType)
    {
        /* Call the recreation callback. Make sure to AddRef first */
        IDirectDrawSurface_AddRef(iface);
        IDirectDrawImpl_RecreateSurfacesCallback(iface,
                                                 &This->surface_desc,
                                                 NULL /* Not needed */);
    }
    hr = IWineD3DSurface_Restore(This->WineD3DSurface);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::SetOverlayPosition
 *
 * Changes the display coordinates of an overlay surface
 *
 * Params:
 *  X:
 *  Y:
 *
 * Returns:
 *   DDERR_NOTAOVERLAYSURFACE, because we don't support overlays right now
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_SetOverlayPosition(IDirectDrawSurface7 *iface,
                                          LONG X,
                                          LONG Y)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d,%d): Relay\n", This, X, Y);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_SetOverlayPosition(This->WineD3DSurface,
                                            X,
                                            Y);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::UpdateOverlay
 *
 * Modifies the attributes of an overlay surface.
 *
 * Params:
 *  SrcRect: The section of the source being used for the overlay
 *  DstSurface: Address of the surface that is overlaid
 *  DstRect: Place of the overlay
 *  Flags: some DDOVER_* flags
 *
 * Returns:
 *  DDERR_UNSUPPORTED, because we don't support overlays
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_UpdateOverlay(IDirectDrawSurface7 *iface,
                                     LPRECT SrcRect,
                                     IDirectDrawSurface7 *DstSurface,
                                     LPRECT DstRect,
                                     DWORD Flags,
                                     LPDDOVERLAYFX FX)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl *Dst = (IDirectDrawSurfaceImpl *)DstSurface;
    HRESULT hr;
    TRACE("(%p)->(%p,%p,%p,%x,%p): Relay\n", This, SrcRect, Dst, DstRect, Flags, FX);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_UpdateOverlay(This->WineD3DSurface,
                                       SrcRect,
                                       Dst ? Dst->WineD3DSurface : NULL,
                                       DstRect,
                                       Flags,
                                       (WINEDDOVERLAYFX *) FX);
    LeaveCriticalSection(&ddraw_cs);
    switch(hr) {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        case WINEDDERR_NOTAOVERLAYSURFACE:  return DDERR_NOTAOVERLAYSURFACE;
        case WINEDDERR_OVERLAYNOTVISIBLE:   return DDERR_OVERLAYNOTVISIBLE;
        default:
            return hr;
    }
}

/*****************************************************************************
 * IDirectDrawSurface7::UpdateOverlayDisplay
 *
 * The DX7 sdk says that it's not implemented
 *
 * Params:
 *  Flags: ?
 *
 * Returns: DDERR_UNSUPPORTED, because we don't support overlays
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_UpdateOverlayDisplay(IDirectDrawSurface7 *iface,
                                            DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    TRACE("(%p)->(%x)\n", This, Flags);
    return DDERR_UNSUPPORTED;
}

/*****************************************************************************
 * IDirectDrawSurface7::UpdateOverlayZOrder
 *
 * Sets an overlay's Z order
 *
 * Params:
 *  Flags: DDOVERZ_* flags
 *  DDSRef: Defines the relative position in the overlay chain
 *
 * Returns:
 *  DDERR_NOTOVERLAYSURFACE, because we don't support overlays
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_UpdateOverlayZOrder(IDirectDrawSurface7 *iface,
                                           DWORD Flags,
                                           IDirectDrawSurface7 *DDSRef)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl *Ref = (IDirectDrawSurfaceImpl *)DDSRef;
    HRESULT hr;

    TRACE("(%p)->(%x,%p): Relay\n", This, Flags, Ref);
    EnterCriticalSection(&ddraw_cs);
    hr =  IWineD3DSurface_UpdateOverlayZOrder(This->WineD3DSurface,
                                              Flags,
                                              Ref ? Ref->WineD3DSurface : NULL);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetDDInterface
 *
 * Returns the IDirectDraw7 interface pointer of the DirectDraw object this
 * surface belongs to
 *
 * Params:
 *  DD: Address to write the interface pointer to
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if DD is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetDDInterface(IDirectDrawSurface7 *iface,
                                      void **DD)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,DD);

    if(!DD)
        return DDERR_INVALIDPARAMS;

    switch(This->version)
    {
        case 7:
            *DD = This->ddraw;
            break;

        case 4:
            *DD = &This->ddraw->IDirectDraw4_vtbl;
            break;

        case 2:
            *DD = &This->ddraw->IDirectDraw2_vtbl;
            break;

        case 1:
            *DD = &This->ddraw->IDirectDraw_vtbl;
            break;

    }
    IUnknown_AddRef((IUnknown *)*DD);

    return DD_OK;
}

/* This seems also windows implementation specific - I don't think WineD3D needs this */
static HRESULT WINAPI IDirectDrawSurfaceImpl_ChangeUniquenessValue(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    volatile IDirectDrawSurfaceImpl* vThis = This;

    TRACE("(%p)\n",This);
    EnterCriticalSection(&ddraw_cs);
    /* A uniqueness value of 0 is apparently special.
     * This needs to be checked.
     * TODO: Write tests for this code and check if the volatile, interlocked stuff is really needed
     */
    while (1) {
        DWORD old_uniqueness_value = vThis->uniqueness_value;
        DWORD new_uniqueness_value = old_uniqueness_value+1;

        if (old_uniqueness_value == 0) break;
        if (new_uniqueness_value == 0) new_uniqueness_value = 1;

        if (InterlockedCompareExchange((LONG*)&vThis->uniqueness_value,
                                      old_uniqueness_value,
                                      new_uniqueness_value)
            == old_uniqueness_value)
            break;
    }

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurfaceImpl_GetUniquenessValue(IDirectDrawSurface7 *iface, LPDWORD pValue)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,pValue);
    EnterCriticalSection(&ddraw_cs);
    *pValue = This->uniqueness_value;
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::SetLOD
 *
 * Sets the level of detail of a texture
 *
 * Params:
 *  MaxLOD: LOD to set
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDOBJECT if the surface is invalid for this method
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_SetLOD(IDirectDrawSurface7 *iface,
                              DWORD MaxLOD)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d)\n", This, MaxLOD);

    EnterCriticalSection(&ddraw_cs);
    if (!(This->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDOBJECT;
    }

    if(!This->wineD3DTexture)
    {
        ERR("(%p) The DirectDraw texture has no WineD3DTexture!\n", This);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDOBJECT;
    }

    hr = IWineD3DBaseTexture_SetLOD(This->wineD3DTexture,
                                    MaxLOD);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetLOD
 *
 * Returns the level of detail of a Direct3D texture
 *
 * Params:
 *  MaxLOD: Address to write the LOD to
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if MaxLOD is NULL
 *  DDERR_INVALIDOBJECT if the surface is invalid for this method
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetLOD(IDirectDrawSurface7 *iface,
                              DWORD *MaxLOD)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    TRACE("(%p)->(%p)\n", This, MaxLOD);

    if(!MaxLOD)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    if (!(This->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDOBJECT;
    }

    *MaxLOD = IWineD3DBaseTexture_GetLOD(This->wineD3DTexture);
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::BltFast
 *
 * Performs a fast Blit.
 *
 * Params:
 *  dstx: The x coordinate to blit to on the destination
 *  dsty: The y coordinate to blit to on the destination
 *  Source: The source surface
 *  rsrc: The source rectangle
 *  trans: Type of transfer. Some DDBLTFAST_* flags
 *
 * Returns:
 *  DD_OK on success
 *  For more details, see IWineD3DSurface::BltFast
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_BltFast(IDirectDrawSurface7 *iface,
                               DWORD dstx,
                               DWORD dsty,
                               IDirectDrawSurface7 *Source,
                               RECT *rsrc,
                               DWORD trans)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl *src = (IDirectDrawSurfaceImpl *)Source;
    HRESULT hr;
    TRACE("(%p)->(%d,%d,%p,%p,%d): Relay\n", This, dstx, dsty, Source, rsrc, trans);

    /* Source must be != NULL, This is not checked by windows. Windows happily throws a 0xc0000005
     * in that case
     */
    if(rsrc)
    {
        if(rsrc->top > rsrc->bottom || rsrc->left > rsrc->right ||
           rsrc->right > src->surface_desc.dwWidth ||
           rsrc->bottom > src->surface_desc.dwHeight)
        {
            WARN("Source rectangle is invalid, returning DDERR_INVALIDRECT\n");
            return DDERR_INVALIDRECT;
        }
        if(dstx + rsrc->right - rsrc->left > This->surface_desc.dwWidth ||
           dsty + rsrc->bottom - rsrc->top > This->surface_desc.dwHeight)
        {
            WARN("Destination area out of bounds, returning DDERR_INVALIDRECT\n");
            return DDERR_INVALIDRECT;
        }
    }
    else
    {
        if(dstx + src->surface_desc.dwWidth > This->surface_desc.dwWidth ||
           dsty + src->surface_desc.dwHeight > This->surface_desc.dwHeight)
        {
            WARN("Destination area out of bounds, returning DDERR_INVALIDRECT\n");
            return DDERR_INVALIDRECT;
        }
    }

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_BltFast(This->WineD3DSurface,
                                 dstx, dsty,
                                 src ? src->WineD3DSurface : NULL,
                                 rsrc,
                                 trans);
    LeaveCriticalSection(&ddraw_cs);
    switch(hr)
    {
        case WINED3DERR_NOTAVAILABLE:           return DDERR_UNSUPPORTED;
        case WINED3DERR_WRONGTEXTUREFORMAT:     return DDERR_INVALIDPIXELFORMAT;
        default:                                return hr;
    }
}

/*****************************************************************************
 * IDirectDrawSurface7::GetClipper
 *
 * Returns the IDirectDrawClipper interface of the clipper assigned to this
 * surface
 *
 * Params:
 *  Clipper: Address to store the interface pointer at
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if Clipper is NULL
 *  DDERR_NOCLIPPERATTACHED if there's no clipper attached
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetClipper(IDirectDrawSurface7 *iface,
                                  IDirectDrawClipper **Clipper)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    TRACE("(%p)->(%p)\n", This, Clipper);

    if(!Clipper)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&ddraw_cs);
    if(This->clipper == NULL)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_NOCLIPPERATTACHED;
    }

    *Clipper = (IDirectDrawClipper *)This->clipper;
    IDirectDrawClipper_AddRef(*Clipper);
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::SetClipper
 *
 * Sets a clipper for the surface
 *
 * Params:
 *  Clipper: IDirectDrawClipper interface of the clipper to set
 *
 * Returns:
 *  DD_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_SetClipper(IDirectDrawSurface7 *iface,
                                  IDirectDrawClipper *Clipper)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawClipperImpl *oldClipper = This->clipper;
    HWND clipWindow;
    HRESULT hr;
    TRACE("(%p)->(%p)\n",This,Clipper);

    EnterCriticalSection(&ddraw_cs);
    if ((IDirectDrawClipperImpl *)Clipper == This->clipper)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DD_OK;
    }

    This->clipper = (IDirectDrawClipperImpl *)Clipper;

    if (Clipper != NULL)
        IDirectDrawClipper_AddRef(Clipper);
    if(oldClipper)
        IDirectDrawClipper_Release((IDirectDrawClipper *)oldClipper);

    hr = IWineD3DSurface_SetClipper(This->WineD3DSurface, This->clipper ? This->clipper->wineD3DClipper : NULL);

    if(This->wineD3DSwapChain) {
        clipWindow = NULL;
        if(Clipper) {
            IDirectDrawClipper_GetHWnd(Clipper, &clipWindow);
        }

        if(clipWindow) {
            IWineD3DSwapChain_SetDestWindowOverride(This->wineD3DSwapChain,
                                                    clipWindow);
        } else {
            IWineD3DSwapChain_SetDestWindowOverride(This->wineD3DSwapChain,
                                                    This->ddraw->d3d_window);
        }
    }

    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawSurface7::SetSurfaceDesc
 *
 * Sets the surface description. It can override the pixel format, the surface
 * memory, ...
 * It's not really tested.
 *
 * Params:
 * DDSD: Pointer to the new surface description to set
 * Flags: Some flags
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if DDSD is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_SetSurfaceDesc(IDirectDrawSurface7 *iface,
                                      DDSURFACEDESC2 *DDSD,
                                      DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    WINED3DFORMAT newFormat = WINED3DFMT_UNKNOWN;
    HRESULT hr;
    TRACE("(%p)->(%p,%x)\n", This, DDSD, Flags);

    if(!DDSD)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    if (DDSD->dwFlags & DDSD_PIXELFORMAT)
    {
        newFormat = PixelFormat_DD2WineD3D(&DDSD->u4.ddpfPixelFormat);

        if(newFormat == WINED3DFMT_UNKNOWN)
        {
            ERR("Requested to set an unknown pixelformat\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }
        if(newFormat != PixelFormat_DD2WineD3D(&This->surface_desc.u4.ddpfPixelFormat) )
        {
            hr = IWineD3DSurface_SetFormat(This->WineD3DSurface,
                                           newFormat);
            if(hr != DD_OK)
            {
                LeaveCriticalSection(&ddraw_cs);
                return hr;
            }
        }
    }
    if (DDSD->dwFlags & DDSD_CKDESTOVERLAY)
    {
        IWineD3DSurface_SetColorKey(This->WineD3DSurface,
                                    DDCKEY_DESTOVERLAY,
                                    (WINEDDCOLORKEY *) &DDSD->u3.ddckCKDestOverlay);
    }
    if (DDSD->dwFlags & DDSD_CKDESTBLT)
    {
        IWineD3DSurface_SetColorKey(This->WineD3DSurface,
                                    DDCKEY_DESTBLT,
                                    (WINEDDCOLORKEY *) &DDSD->ddckCKDestBlt);
    }
    if (DDSD->dwFlags & DDSD_CKSRCOVERLAY)
    {
        IWineD3DSurface_SetColorKey(This->WineD3DSurface,
                                    DDCKEY_SRCOVERLAY,
                                    (WINEDDCOLORKEY *) &DDSD->ddckCKSrcOverlay);
    }
    if (DDSD->dwFlags & DDSD_CKSRCBLT)
    {
        IWineD3DSurface_SetColorKey(This->WineD3DSurface,
                                    DDCKEY_SRCBLT,
                                    (WINEDDCOLORKEY *) &DDSD->ddckCKSrcBlt);
    }
    if (DDSD->dwFlags & DDSD_LPSURFACE && DDSD->lpSurface)
    {
        hr = IWineD3DSurface_SetMem(This->WineD3DSurface, DDSD->lpSurface);
        if(hr != WINED3D_OK)
        {
            /* No need for a trace here, wined3d does that for us */
            switch(hr)
            {
                case WINED3DERR_INVALIDCALL:
                    LeaveCriticalSection(&ddraw_cs);
                    return DDERR_INVALIDPARAMS;
                default:
                    break; /* Go on */
            }
        }
    }

    This->surface_desc = *DDSD;

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetPalette
 *
 * Returns the IDirectDrawPalette interface of the palette currently assigned
 * to the surface
 *
 * Params:
 *  Pal: Address to write the interface pointer to
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if Pal is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_GetPalette(IDirectDrawSurface7 *iface,
                                  IDirectDrawPalette **Pal)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IWineD3DPalette *wPal;
    HRESULT hr;
    TRACE("(%p)->(%p): Relay\n", This, Pal);

    if(!Pal)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DSurface_GetPalette(This->WineD3DSurface, &wPal);
    if(hr != DD_OK)
    {
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    if(wPal)
    {
        hr = IWineD3DPalette_GetParent(wPal, (IUnknown **) Pal);
    }
    else
    {
        *Pal = NULL;
        hr = DDERR_NOPALETTEATTACHED;
    }

    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * SetColorKeyEnum
 *
 * EnumAttachedSurface callback for SetColorKey. Used to set color keys
 * recursively in the surface tree
 *
 *****************************************************************************/
struct SCKContext
{
    HRESULT ret;
    WINEDDCOLORKEY *CKey;
    DWORD Flags;
};

static HRESULT WINAPI
SetColorKeyEnum(IDirectDrawSurface7 *surface,
                DDSURFACEDESC2 *desc,
                void *context)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)surface;
    struct SCKContext *ctx = context;
    HRESULT hr;

    hr = IWineD3DSurface_SetColorKey(This->WineD3DSurface,
                                     ctx->Flags,
                                     ctx->CKey);
    if(hr != DD_OK)
    {
        WARN("IWineD3DSurface_SetColorKey failed, hr = %08x\n", hr);
        ctx->ret = hr;
    }

    IDirectDrawSurface7_EnumAttachedSurfaces(surface,
                                             context,
                                             SetColorKeyEnum);
    IDirectDrawSurface7_Release(surface);
    return DDENUMRET_OK;
}

/*****************************************************************************
 * IDirectDrawSurface7::SetColorKey
 *
 * Sets the color keying options for the surface. Observations showed that
 * in case of complex surfaces the color key has to be assigned to all
 * sublevels.
 *
 * Params:
 *  Flags: DDCKEY_*
 *  CKey: The new color key
 *
 * Returns:
 *  DD_OK on success
 *  See IWineD3DSurface::SetColorKey for details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_SetColorKey(IDirectDrawSurface7 *iface,
                                   DWORD Flags,
                                   DDCOLORKEY *CKey)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    struct SCKContext ctx = { DD_OK, (WINEDDCOLORKEY *) CKey, Flags };
    TRACE("(%p)->(%x,%p)\n", This, Flags, CKey);

    EnterCriticalSection(&ddraw_cs);
    if (CKey)
    {
        switch (Flags & ~DDCKEY_COLORSPACE)
        {
        case DDCKEY_DESTBLT:
            This->surface_desc.ddckCKDestBlt = *CKey;
            This->surface_desc.dwFlags |= DDSD_CKDESTBLT;
            break;

        case DDCKEY_DESTOVERLAY:
            This->surface_desc.u3.ddckCKDestOverlay = *CKey;
            This->surface_desc.dwFlags |= DDSD_CKDESTOVERLAY;
            break;

        case DDCKEY_SRCOVERLAY:
            This->surface_desc.ddckCKSrcOverlay = *CKey;
            This->surface_desc.dwFlags |= DDSD_CKSRCOVERLAY;
            break;

        case DDCKEY_SRCBLT:
            This->surface_desc.ddckCKSrcBlt = *CKey;
            This->surface_desc.dwFlags |= DDSD_CKSRCBLT;
            break;

        default:
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }
    }
    else
    {
        switch (Flags & ~DDCKEY_COLORSPACE)
        {
        case DDCKEY_DESTBLT:
            This->surface_desc.dwFlags &= ~DDSD_CKDESTBLT;
            break;

        case DDCKEY_DESTOVERLAY:
            This->surface_desc.dwFlags &= ~DDSD_CKDESTOVERLAY;
            break;

        case DDCKEY_SRCOVERLAY:
            This->surface_desc.dwFlags &= ~DDSD_CKSRCOVERLAY;
            break;

        case DDCKEY_SRCBLT:
            This->surface_desc.dwFlags &= ~DDSD_CKSRCBLT;
            break;

        default:
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }
    }
    ctx.ret = IWineD3DSurface_SetColorKey(This->WineD3DSurface,
                                          Flags,
                                          ctx.CKey);
    IDirectDrawSurface7_EnumAttachedSurfaces(iface,
                                             &ctx,
                                             SetColorKeyEnum);
    LeaveCriticalSection(&ddraw_cs);
    switch(ctx.ret)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return ctx.ret;
    }
}

/*****************************************************************************
 * IDirectDrawSurface7::SetPalette
 *
 * Assigns a DirectDrawPalette object to the surface
 *
 * Params:
 *  Pal: Interface to the palette to set
 *
 * Returns:
 *  DD_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_SetPalette(IDirectDrawSurface7 *iface,
                                  IDirectDrawPalette *Pal)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawPalette *oldPal;
    IDirectDrawSurfaceImpl *surf;
    IDirectDrawPaletteImpl *PalImpl = (IDirectDrawPaletteImpl *)Pal;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, Pal);

    if (!(This->surface_desc.u4.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 |
            DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXEDTO8))) {
        return DDERR_INVALIDPIXELFORMAT;
    }

    /* Find the old palette */
    EnterCriticalSection(&ddraw_cs);
    hr = IDirectDrawSurface_GetPalette(iface, &oldPal);
    if(hr != DD_OK && hr != DDERR_NOPALETTEATTACHED)
    {
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }
    if(oldPal) IDirectDrawPalette_Release(oldPal);  /* For the GetPalette */

    /* Set the new Palette */
    IWineD3DSurface_SetPalette(This->WineD3DSurface,
                               PalImpl ? PalImpl->wineD3DPalette : NULL);
    /* AddRef the Palette */
    if(Pal) IDirectDrawPalette_AddRef(Pal);

    /* Release the old palette */
    if(oldPal) IDirectDrawPalette_Release(oldPal);

    /* If this is a front buffer, also update the back buffers
     * TODO: How do things work for palettized cube textures?
     */
    if(This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
    {
        /* For primary surfaces the tree is just a list, so the simpler scheme fits too */
        DDSCAPS2 caps2 = { DDSCAPS_PRIMARYSURFACE, 0, 0, {0} };

        surf = This;
        while(1)
        {
            IDirectDrawSurface7 *attach;
            HRESULT hr;
            hr = IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)surf, &caps2, &attach);
            if(hr != DD_OK)
            {
                break;
            }

            TRACE("Setting palette on %p\n", attach);
            IDirectDrawSurface7_SetPalette(attach,
                                           Pal);
            surf = (IDirectDrawSurfaceImpl *)attach;
            IDirectDrawSurface7_Release(attach);
        }
    }

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * The VTable
 *****************************************************************************/

const IDirectDrawSurface7Vtbl IDirectDrawSurface7_Vtbl =
{
    /*** IUnknown ***/
    IDirectDrawSurfaceImpl_QueryInterface,
    IDirectDrawSurfaceImpl_AddRef,
    IDirectDrawSurfaceImpl_Release,
    /*** IDirectDrawSurface ***/
    IDirectDrawSurface7Impl_AddAttachedSurface,
    IDirectDrawSurfaceImpl_AddOverlayDirtyRect,
    IDirectDrawSurfaceImpl_Blt,
    IDirectDrawSurfaceImpl_BltBatch,
    IDirectDrawSurfaceImpl_BltFast,
    IDirectDrawSurfaceImpl_DeleteAttachedSurface,
    IDirectDrawSurfaceImpl_EnumAttachedSurfaces,
    IDirectDrawSurfaceImpl_EnumOverlayZOrders,
    IDirectDrawSurfaceImpl_Flip,
    IDirectDrawSurfaceImpl_GetAttachedSurface,
    IDirectDrawSurfaceImpl_GetBltStatus,
    IDirectDrawSurfaceImpl_GetCaps,
    IDirectDrawSurfaceImpl_GetClipper,
    IDirectDrawSurfaceImpl_GetColorKey,
    IDirectDrawSurfaceImpl_GetDC,
    IDirectDrawSurfaceImpl_GetFlipStatus,
    IDirectDrawSurfaceImpl_GetOverlayPosition,
    IDirectDrawSurfaceImpl_GetPalette,
    IDirectDrawSurfaceImpl_GetPixelFormat,
    IDirectDrawSurfaceImpl_GetSurfaceDesc,
    IDirectDrawSurfaceImpl_Initialize,
    IDirectDrawSurfaceImpl_IsLost,
    IDirectDrawSurfaceImpl_Lock,
    IDirectDrawSurfaceImpl_ReleaseDC,
    IDirectDrawSurfaceImpl_Restore,
    IDirectDrawSurfaceImpl_SetClipper,
    IDirectDrawSurfaceImpl_SetColorKey,
    IDirectDrawSurfaceImpl_SetOverlayPosition,
    IDirectDrawSurfaceImpl_SetPalette,
    IDirectDrawSurfaceImpl_Unlock,
    IDirectDrawSurfaceImpl_UpdateOverlay,
    IDirectDrawSurfaceImpl_UpdateOverlayDisplay,
    IDirectDrawSurfaceImpl_UpdateOverlayZOrder,
    /*** IDirectDrawSurface2 ***/
    IDirectDrawSurfaceImpl_GetDDInterface,
    IDirectDrawSurfaceImpl_PageLock,
    IDirectDrawSurfaceImpl_PageUnlock,
    /*** IDirectDrawSurface3 ***/
    IDirectDrawSurfaceImpl_SetSurfaceDesc,
    /*** IDirectDrawSurface4 ***/
    IDirectDrawSurfaceImpl_SetPrivateData,
    IDirectDrawSurfaceImpl_GetPrivateData,
    IDirectDrawSurfaceImpl_FreePrivateData,
    IDirectDrawSurfaceImpl_GetUniquenessValue,
    IDirectDrawSurfaceImpl_ChangeUniquenessValue,
    /*** IDirectDrawSurface7 ***/
    IDirectDrawSurfaceImpl_SetPriority,
    IDirectDrawSurfaceImpl_GetPriority,
    IDirectDrawSurfaceImpl_SetLOD,
    IDirectDrawSurfaceImpl_GetLOD
};
