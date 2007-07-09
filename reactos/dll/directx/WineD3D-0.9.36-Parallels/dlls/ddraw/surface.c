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

//#ifndef WINE_NATIVEWIN32
# include "windef.h"
# include "winbase.h"
# include "winnls.h"
# include "winerror.h"
# include "wingdi.h"
//#endif


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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);

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
        *obj = ICOM_INTERFACE(This, IDirectDrawSurface7);
        TRACE("(%p) returning IDirectDrawSurface7 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_IDirectDrawSurface3)
          || IsEqualGUID(riid, &IID_IDirectDrawSurface2)
          || IsEqualGUID(riid, &IID_IDirectDrawSurface) )
    {
        IUnknown_AddRef(iface);
        *obj = ICOM_INTERFACE(This, IDirectDrawSurface3);
        TRACE("(%p) returning IDirectDrawSurface3 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_IDirectDrawGammaControl) )
    {
        IUnknown_AddRef(iface);
        *obj = ICOM_INTERFACE(This, IDirectDrawGammaControl);
        TRACE("(%p) returning IDirectDrawGammaControl interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_D3DDEVICE_WineD3D) ||
             IsEqualGUID(riid, &IID_IDirect3DHALDevice) )
    {
        IDirect3DDevice7 *d3d;

        /* Call into IDirect3D7 for creation */
        IDirect3D7_CreateDevice(ICOM_INTERFACE(This->ddraw, IDirect3D7),
                                riid,
                                ICOM_INTERFACE(This, IDirectDrawSurface7),
                                &d3d);

        *obj = COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice7, IDirect3DDevice, d3d);
        TRACE("(%p) Returning IDirect3DDevice interface at %p\n", This, *obj);

        return S_OK;
    }
    else if (IsEqualGUID( &IID_IDirect3DTexture, riid ) ||
             IsEqualGUID( &IID_IDirect3DTexture2, riid ))
    {
        if (IsEqualGUID( &IID_IDirect3DTexture, riid ))
        {
            *obj = ICOM_INTERFACE(This, IDirect3DTexture);
            TRACE(" returning Direct3DTexture interface at %p.\n", *obj);
        }
        else
        {
            *obj = ICOM_INTERFACE(This, IDirect3DTexture2);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
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
static void IDirectDrawSurfaceImpl_Destroy(IDirectDrawSurfaceImpl *This)
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
        IDirectDrawSurface7 *root = ICOM_INTERFACE(This->first_attached, IDirectDrawSurface7);
        IDirectDrawSurface7 *detach = ICOM_INTERFACE(This, IDirectDrawSurface7);

        FIXME("(%p) Freeing a surface that is attached to surface %p\n", This, This->first_attached);

        /* The refcount will drop to -1 here */
        if(IDirectDrawSurface7_DeleteAttachedSurface(root, 0, detach) != DD_OK)
        {
            ERR("(%p) DeleteAttachedSurface failed!\n", This);
        }
    }

    while(This->next_attached != NULL)
    {
        IDirectDrawSurface7 *root = ICOM_INTERFACE(This, IDirectDrawSurface7);
        IDirectDrawSurface7 *detach = ICOM_INTERFACE(This->next_attached, IDirectDrawSurface7);

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
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirectDrawSurfaceImpl_Release(IDirectDrawSurface7 *iface)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
    {

        IDirectDrawSurfaceImpl *surf;
        IDirectDrawImpl *ddraw;
        IUnknown *ifaceToRelease = This->ifaceToRelease;

        /* Complex attached surfaces are destroyed implicitely when the root is released */
        if(This->first_complex != This)
        {
            WARN("(%p) Attempt to destroy a surface that is attached to a complex root %p\n", This, This->first_complex);
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
            IWineD3DTexture_Release(This->wineD3DTexture);
        }
        /* If it's the RenderTarget, destroy the d3ddevice */
        else if( (ddraw->d3d_initialized) && (This == ddraw->d3d_target))
        {
            TRACE("(%p) Destroying the render target, uninitializing D3D\n", This);

            /* Unset any index buffer, just to be sure */
            IWineD3DDevice_SetIndices(ddraw->wineD3DDevice, NULL, 0);
            IWineD3DDevice_SetDepthStencilSurface(ddraw->wineD3DDevice, NULL);

            if(IWineD3DDevice_Uninit3D(ddraw->wineD3DDevice, D3D7CB_DestroyDepthStencilSurface, D3D7CB_DestroySwapChain) != D3D_OK)
            {
                /* Not good */
                ERR("(%p) Failed to uninit 3D\n", This);
            }
            else
            {
                /* Free the d3d window if one was created */
                if(ddraw->d3d_window != 0)
                {
                    TRACE(" (%p) Destroying the hidden render window %p\n", This, ddraw->d3d_window);
                    DestroyWindow(ddraw->d3d_window);
                    ddraw->d3d_window = 0;
                }
                /* Unset the pointers */
            }

            ddraw->d3d_initialized = FALSE;
            ddraw->d3d_target = NULL;

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
            surf = This;
            while(surf)
            {
                IParent *Parent;

                IWineD3DSurface_GetParent(surf->WineD3DSurface,
                                          (IUnknown **) &Parent);
                IParent_Release(Parent);  /* For the getParent */
                IParent_Release(Parent);  /* To release it */
                surf = surf->next_complex;
            }
        }

        /* The refcount test shows that the palette is detached when the surface is destroyed */
        IDirectDrawSurface7_SetPalette(ICOM_INTERFACE(This, IDirectDrawSurface7),
                                                      NULL);

        /* Loop through all complex attached surfaces,
         * and destroy them
         */
        while( (surf = This->next_complex) )
        {
            This->next_complex = surf->next_complex;  /* Unchain it from the complex listing */
            IDirectDrawSurfaceImpl_Destroy(surf);     /* Destroy it */
        }

        /* Destroy the surface.
         */
        IDirectDrawSurfaceImpl_Destroy(This);

        /* Reduce the ddraw refcount */
        if(ifaceToRelease) IUnknown_Release(ifaceToRelease);
    }

    return ref;
}

/*****************************************************************************
 * IDirectDrawSurface7::GetAttachedSurface
 *
 * Returns an attached surface with the requested caps. Surface attachment
 * and complex surfaces are not clearly described by the MSDN or sdk,
 * so this method is tricky and likely to contain problems.
 * This implementation searches the complex chain first, then the
 * attachment chain, and then it checks if the caps match to itself.
 *
 * The chains are searched from This down to the last surface in the chain,
 * not from the first element in the chain. The first surface found is
 * returned. The MSDN says that this method fails if more than one surface
 * matches the caps, but apparently this is incorrect.
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawSurfaceImpl *surf;
    DDSCAPS2 our_caps;

    TRACE("(%p)->(%p,%p)\n", This, Caps, Surface);

    our_caps = *Caps;

    if(This->version < 7)
    {
        /* Earlier dx apps put garbage into these members, clear them */
        our_caps.dwCaps2 = 0;
        our_caps.dwCaps3 = 0;
#ifndef WINE_NATIVEWIN32
        our_caps.dwCaps4 = 0;
#else
        our_caps.u1.dwCaps4 = 0;
#endif
    }

#ifndef WINE_NATIVEWIN32
    TRACE("(%p): Looking for caps: %x,%x,%x,%x\n", This, our_caps.dwCaps, our_caps.dwCaps2, our_caps.dwCaps3, our_caps.dwCaps4); /* FIXME: Better debugging */
#else
    TRACE("(%p): Looking for caps: %x,%x,%x,%x\n", This, our_caps.dwCaps, our_caps.dwCaps2, our_caps.dwCaps3, our_caps.u1.dwCaps4); /* FIXME: Better debugging */
#endif

    /* First, look at the complex chain */
    surf = This;

    while( (surf = surf->next_complex) )
    {
        if (TRACE_ON(ddraw))
        {
            TRACE("Surface: (%p) caps: %x,%x,%x,%x\n", surf,
                   surf->surface_desc.ddsCaps.dwCaps,
                   surf->surface_desc.ddsCaps.dwCaps2,
                   surf->surface_desc.ddsCaps.dwCaps3,
#ifndef WINE_NATIVEWIN32
                   surf->surface_desc.ddsCaps.dwCaps4);
#else
                   surf->surface_desc.ddsCaps.u1.dwCaps4);
#endif
        }

        if (((surf->surface_desc.ddsCaps.dwCaps & our_caps.dwCaps) == our_caps.dwCaps) &&
            ((surf->surface_desc.ddsCaps.dwCaps2 & our_caps.dwCaps2) == our_caps.dwCaps2)) {

            /* MSDN: "This method fails if more than one surface is attached
             * that matches the capabilities requested."
             *
             * The mipmap demo of the DirectX7 sdk shows what to do here:
             * apparently apps expect the first found surface to be returned.
             */

            TRACE("(%p): Returning surface %p\n", This, surf);
            TRACE("(%p): mipmapcount=%d\n", This, surf->mipmap_level);
            *Surface = ICOM_INTERFACE(surf, IDirectDrawSurface7);
            IDirectDrawSurface7_AddRef(*Surface);
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
#ifndef WINE_NATIVEWIN32
                   surf->surface_desc.ddsCaps.dwCaps4);
#else
                   surf->surface_desc.ddsCaps.u1.dwCaps4);
#endif
        }

        if (((surf->surface_desc.ddsCaps.dwCaps & our_caps.dwCaps) == our_caps.dwCaps) &&
            ((surf->surface_desc.ddsCaps.dwCaps2 & our_caps.dwCaps2) == our_caps.dwCaps2)) {

            /* MSDN: "This method fails if more than one surface is attached
             * that matches the capabilities requested."
             *
             * The mipmap demo of the DirectX7 sdk shows what to do here:
             * apparently apps expect the first found surface to be returned.
             */

            TRACE("(%p): Returning surface %p\n", This, surf);
            *Surface = ICOM_INTERFACE(surf, IDirectDrawSurface7);
            IDirectDrawSurface7_AddRef(*Surface);
            return DD_OK;
        }
    }

    /* Is this valid? */
#if 0
    if (((This->surface_desc.ddsCaps.dwCaps & our_caps.dwCaps) == our_caps.dwCaps) &&
        ((This->surface_desc.ddsCaps.dwCaps2 & our_caps.dwCaps2) == our_caps.dwCaps2) && 
        This == This->first_complex)
    {

        TRACE("(%p): Returning surface %p\n", This, This);
        *Surface = ICOM_INTERFACE(This, IDirectDrawSurface7);
        IDirectDrawSurface7_AddRef(*Surface);
        return DD_OK;
    }
#endif

    /* What to do here? Continue with the surface root?? */

    TRACE("(%p) Didn't find a valid surface\n", This);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    WINED3DLOCKED_RECT LockedRect;
    HRESULT hr;
    TRACE("(%p)->(%p,%p,%x,%p)\n", This, Rect, DDSD, Flags, h);

    if(!DDSD)
        return DDERR_INVALIDPARAMS;

    /* Should I check for the handle to be NULL?
     *
     * The DDLOCK flags and the D3DLOCK flags are equal
     * for the supported values. The others are ignored by WineD3D
     */

    /* Hmm. Anarchy online passes an uninitialized surface descriptor,
     * that means it doesn't have dwSize set. Init it to some sane
     * value
     */
    if(DDSD->dwSize <= sizeof(DDSURFACEDESC))
    {
        DDSD->dwSize = sizeof(DDSURFACEDESC);
    }
    else
    {
        DDSD->dwSize = sizeof(DDSURFACEDESC2);
    }

    DD_STRUCT_COPY_BYSIZE(DDSD,&(This->surface_desc));
    hr = IWineD3DSurface_LockRect(This->WineD3DSurface,
                                  &LockedRect,
                                  Rect,
                                  Flags);
    if(hr != D3D_OK) return hr;

    /* Override the memory area and the pitch */
    DDSD->dwFlags |= DDSD_LPSURFACE;
    DDSD->lpSurface = LockedRect.pBits;
    DDSD->dwFlags |= DDSD_PITCH;
    DDSD->u1.lPitch = LockedRect.Pitch;

    TRACE("locked surface returning description :\n");
    if (TRACE_ON(ddraw)) DDRAW_dump_surface_desc(DDSD);

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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%p)\n", This, pRect);

    return IWineD3DSurface_UnlockRect(This->WineD3DSurface);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawSurfaceImpl *Override = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, DestOverride);
    IDirectDrawSurface7 *Override7;
    HRESULT hr;
    TRACE("(%p)->(%p,%x)\n", This, DestOverride, Flags);

    /* Flip has to be called from a front buffer
     * What about overlay surfaces, AFAIK they can flip too?
     */
    if( !(This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) )
        return DDERR_INVALIDOBJECT; /* Unckecked */

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
            return DDERR_NOTFLIPPABLE; /* Unchecked */
        }
        Override = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, Override7);

        /* For the GetAttachedSurface */
        IDirectDrawSurface7_Release(Override7);
    }

    return  IWineD3DSurface_Flip(This->WineD3DSurface,
                                 Override->WineD3DSurface,
                                 Flags);
}

/*****************************************************************************
 * IDirectDrawSurface7::Blt
 *
 * Performs a blit on the surface
 *
 * Params:
 *  DestRect: Destination rectangle, can be NULL
 *  SrcSurface: Source surface, can be NULL
 *  SrcRect: Source rectange, can be NULL
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    HRESULT hr;
    IDirectDrawSurfaceImpl *Src = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, SrcSurface);
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

    if(Flags & DDBLT_KEYSRC && (!Src || !(Src->surface_desc.dwFlags & DDSD_CKSRCBLT))) {
        WARN("DDBLT_KEYDEST blit without color key in surface, returning DDERR_INVALIDPARAMS\n");
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
                             WINED3DTEXF_NONE);
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
 * Attaches a surface to another surface. Surface attachments are
 * incorrectly described in the SDK and the MSDN, and this method
 * is prone to bugs. The surface that is attached is AddRef-ed.
 *
 * The attachment list consists of a first surface (first_attached) and
 * for each surface a pointer to the next attached surface (next_attached).
 * For the first surface, and a surface that has no attachments
 * first_attached points to the surface itself. A surface that has
 * no successors in the chain has next_attached set to NULL.
 *
 * Newly attached surfaces are attached right after the root surface. The
 * complex chains are handled separately in a similar chain, with
 * first_complex and next_complex. If a surface is attached to a complex
 * surface compound, it's attached to the surface that the app requested,
 * not the complex root. See GetAttachedSurface for a description
 * how surfaces are found.
 *
 * This is how the current implementation works, and it was coded by looking
 * at the needs of the applications.
 *
 * So far only Z-Buffer attachments are tested, but there's no code yet
 * to activate them. Mipmaps could be tricky to activate in WineD3D.
 * Back buffers should work in 2D mode, but they are not tested.
 * Rendering to the primary surface and switching between that and
 * double buffering is not yet implemented in WineD3D, so for 3D it might
 * have unexpected results.
 *
 * Params:
 *  Attach: Surface to attach to iface
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_CANNOTATTACHSURFACE if the surface can't be attached for some reason
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_AddAttachedSurface(IDirectDrawSurface7 *iface,
                                          IDirectDrawSurface7 *Attach)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawSurfaceImpl *Surf = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, Attach);
    TRACE("(%p)->(%p)\n", This, Surf);

    /* Should I make sure to add it to the first complex surface? */

    if(Surf == This)
        return DDERR_CANNOTATTACHSURFACE; /* unchecked */

    /* TODO MSDN: "You can attach only z-buffer surfaces with this method."
     * But apparently backbuffers and mipmaps can be attached too. */

    /* Set MIPMAPSUBLEVEL if this seems to be one */
    if (This->surface_desc.ddsCaps.dwCaps &
        Surf->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        Surf->surface_desc.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
        /* FIXME: we should probably also add to dwMipMapCount of this
          * and all parent surfaces (update create_texture if you do) */
    }

    /* Check if the surface is already attached somewhere */
    if( (Surf->next_attached != NULL) ||
        (Surf->first_attached != Surf) )
    {
         ERR("(%p) The Surface %p is already attached somewhere else: next_attached = %p, first_attached = %p, can't handle by now\n", This, Surf, Surf->next_attached, Surf->first_attached);
        return DDERR_CANNOTATTACHSURFACE;
    }

    /* This inserts the new surface at the 2nd position in the chain, right after the root surface */
    Surf->next_attached = This->next_attached;
    Surf->first_attached = This->first_attached;
    This->next_attached = Surf;

    /* Check if we attach a back buffer to the primary */
    if(Surf->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER &&
       This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        IWineD3DDevice_SetDepthStencilSurface(This->ddraw->wineD3DDevice,
                                              Surf->WineD3DSurface);
    }

    /* MSDN: 
     * "This method increments the reference count of the surface being attached."
     */
    IDirectDrawSurface7_AddRef(Attach);
    return DD_OK;
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawSurfaceImpl *Surf = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, Attach);
    IDirectDrawSurfaceImpl *Prev = This;
    TRACE("(%p)->(%08x,%p)\n", This, Flags, Surf);

    if (!Surf || (Surf->first_attached != This) || (Surf == This) )
        return DDERR_SURFACENOTATTACHED; /* unchecked */

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

    /* Check if we attach a back buffer to the primary */
    if(Surf->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER &&
       This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        IWineD3DDevice_SetDepthStencilSurface(This->ddraw->wineD3DDevice,
                                              NULL);
    }

    IDirectDrawSurface7_Release(Attach);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%p): Relay\n", This, hdc);

    if(!hdc)
        return DDERR_INVALIDPARAMS;

    return IWineD3DSurface_GetDC(This->WineD3DSurface,
                                 hdc);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%p): Relay\n", This, hdc);

    return IWineD3DSurface_ReleaseDC(This->WineD3DSurface, hdc);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%d): Relay!\n",This,Priority);

    return IWineD3DSurface_SetPriority(This->WineD3DSurface, Priority);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%p): Relay\n",This,Priority);

    if(!Priority)
        return DDERR_INVALIDPARAMS;

    *Priority = IWineD3DSurface_GetPriority(This->WineD3DSurface);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%s,%p,%d,%x): Relay\n", This, debugstr_guid(tag), Data, Size, Flags);

    return IWineD3DSurface_SetPrivateData(This->WineD3DSurface,
                                          tag,
                                          Data,
                                          Size,
                                          Flags);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%s,%p,%p): Relay\n", This, debugstr_guid(tag), Data, Size);

    if(!Data)
        return DDERR_INVALIDPARAMS;

    return IWineD3DSurface_GetPrivateData(This->WineD3DSurface,
                                          tag,
                                          Data,
                                          Size);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%s): Relay\n", This, debugstr_guid(tag));

    return IWineD3DSurface_FreePrivateData(This->WineD3DSurface, tag);
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
 *  Flags: Not used, must be 0(unckeched)
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawSurfaceImpl *surf;
    DDSURFACEDESC2 desc;

    /* Attached surfaces aren't handled in WineD3D */
    TRACE("(%p)->(%p,%p)\n",This,context,cb);

    if(!cb)
        return DDERR_INVALIDPARAMS;

    for (surf = This->next_complex; surf != NULL; surf = surf->next_complex)
    {
        IDirectDrawSurface7_AddRef(ICOM_INTERFACE(surf, IDirectDrawSurface7));
        desc = surf->surface_desc;
        /* check: != DDENUMRET_OK or == DDENUMRET_CANCEL? */
        if (cb(ICOM_INTERFACE(surf, IDirectDrawSurface7), &desc, context) == DDENUMRET_CANCEL)
            return DD_OK;
    }

    for (surf = This->next_attached; surf != NULL; surf = surf->next_attached)
    {
        IDirectDrawSurface7_AddRef(ICOM_INTERFACE(surf, IDirectDrawSurface7));
        desc = surf->surface_desc;
        /* check: != DDENUMRET_OK or == DDENUMRET_CANCEL? */
        if (cb( ICOM_INTERFACE(surf, IDirectDrawSurface7), &desc, context) == DDENUMRET_CANCEL)
            return DD_OK;
    }

    TRACE(" end of enumeration.\n");

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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    HRESULT hr;
    TRACE("(%p)->(%x): Relay\n", This, Flags);

    hr = IWineD3DSurface_GetBltStatus(This->WineD3DSurface, Flags);
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
    /* There is a DDERR_NOCOLORKEY error, but how do we know if a color key
     * isn't there? That's like saying that an int isn't there. (Which MS
     * has done in other docs.) */
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%08x,%p)\n", This, Flags, CKey);

    if(!CKey)
        return DDERR_INVALIDPARAMS;

    switch (Flags)
    {
    case DDCKEY_DESTBLT:
        *CKey = This->surface_desc.ddckCKDestBlt;
        break;

    case DDCKEY_DESTOVERLAY:
        *CKey = This->surface_desc.u3.ddckCKDestOverlay;
        break;

    case DDCKEY_SRCBLT:
        *CKey = This->surface_desc.ddckCKSrcBlt;
        break;

    case DDCKEY_SRCOVERLAY:
        *CKey = This->surface_desc.ddckCKSrcOverlay;
        break;

    default:
        return DDERR_INVALIDPARAMS;
    }

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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    HRESULT hr;
    TRACE("(%p)->(%x): Relay\n", This, Flags);

    hr = IWineD3DSurface_GetFlipStatus(This->WineD3DSurface, Flags);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%p,%p): Relay\n", This, X, Y);

    return IWineD3DSurface_GetOverlayPosition(This->WineD3DSurface,
                                              X,
                                              Y);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%p)\n",This,PixelFormat);

    if(!PixelFormat)
        return DDERR_INVALIDPARAMS;

    DD_STRUCT_COPY_BYSIZE(PixelFormat,&This->surface_desc.u4.ddpfPixelFormat);

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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);

    TRACE("(%p)->(%p)\n",This,DDSD);

    if(!DDSD)
        return DDERR_INVALIDPARAMS;

    if ((DDSD->dwSize < sizeof(DDSURFACEDESC)) ||
        (DDSD->dwSize > sizeof(DDSURFACEDESC2)))
    {
        ERR("Impossible/Strange struct size %d.\n",DDSD->dwSize);
        return DDERR_GENERIC;
    }

    DD_STRUCT_COPY_BYSIZE(DDSD,&This->surface_desc);
    TRACE("Returning surface desc:\n");
    if (TRACE_ON(ddraw)) DDRAW_dump_surface_desc(DDSD);

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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawImpl *ddimpl = ICOM_OBJECT(IDirectDrawImpl, IDirectDraw, DD);
    TRACE("(%p)->(%p,%p)\n",This,ddimpl,DDSD);

    return DDERR_ALREADYINITIALIZED;
}

/*****************************************************************************
 * IDirectDrawSurface7::IsLost
 *
 * Checks if the surface is lost
 *
 * Returns:
 *  DD_OK, if the surface is useable
 *  DDERR_ISLOST if the surface is lost
 *  See IWineD3DSurface::IsLost for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawSurfaceImpl_IsLost(IDirectDrawSurface7 *iface)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    HRESULT hr;
    TRACE("(%p)\n", This);

    /* We lose the surface if the implementation was changed */
    if(This->ImplType != This->ddraw->ImplType)
    {
        /* But this shouldn't happen. When we change the implementation,
         * all surfaces are re-created automatically, and their content
         * is copied
         */
        ERR(" (%p) Implementation was changed from %d to %d\n", This, This->ImplType, This->ddraw->ImplType);
        return DDERR_SURFACELOST;
    }

    hr = IWineD3DSurface_IsLost(This->WineD3DSurface);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)\n", This);

    if(This->ImplType != This->ddraw->ImplType)
    {
        /* Call the recreation callback. Make sure to AddRef first */
        IDirectDrawSurface_AddRef(iface);
        IDirectDrawImpl_RecreateSurfacesCallback(iface,
                                                 &This->surface_desc,
                                                 NULL /* Not needed */);
    }
    return IWineD3DSurface_Restore(This->WineD3DSurface);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%d,%d): Relay\n", This, X, Y);

    return IWineD3DSurface_SetOverlayPosition(This->WineD3DSurface,
                                              X,
                                              Y);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawSurfaceImpl *Dst = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, DstSurface);
    TRACE("(%p)->(%p,%p,%p,%x,%p): Relay\n", This, SrcRect, Dst, DstRect, Flags, FX);

    return IWineD3DSurface_UpdateOverlay(This->WineD3DSurface,
                                         SrcRect,
                                         Dst ? Dst->WineD3DSurface : NULL,
                                         DstRect,
                                         Flags,
                                         (WINEDDOVERLAYFX *) FX);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawSurfaceImpl *Ref = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, DDSRef);

    TRACE("(%p)->(%x,%p): Relay\n", This, Flags, Ref);
    return IWineD3DSurface_UpdateOverlayZOrder(This->WineD3DSurface,
                                               Flags,
                                               Ref ? Ref->WineD3DSurface : NULL);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);

    TRACE("(%p)->(%p)\n",This,DD);

    if(!DD)
        return DDERR_INVALIDPARAMS;

    switch(This->version)
    {
        case 7:
            *((IDirectDraw7 **) DD) = ICOM_INTERFACE(This->ddraw, IDirectDraw7);
            IDirectDraw7_AddRef(*(IDirectDraw7 **) DD);
            break;

        case 4:
            *((IDirectDraw4 **) DD) = ICOM_INTERFACE(This->ddraw, IDirectDraw4);
            IDirectDraw4_AddRef(*(IDirectDraw4 **) DD);
            break;

        case 2:
            *((IDirectDraw2 **) DD) = ICOM_INTERFACE(This->ddraw, IDirectDraw2);
            IDirectDraw_AddRef( *(IDirectDraw2 **) DD);
            break;

        case 1:
            *((IDirectDraw **) DD) = ICOM_INTERFACE(This->ddraw, IDirectDraw);
            IDirectDraw_AddRef( *(IDirectDraw **) DD);
            break;

    }

    return DD_OK;
}

/* This seems also windows implementation specific - I don't think WineD3D needs this */
static HRESULT WINAPI IDirectDrawSurfaceImpl_ChangeUniquenessValue(IDirectDrawSurface7 *iface)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    volatile IDirectDrawSurfaceImpl* vThis = This;

    TRACE("(%p)\n",This);
    /* A uniqueness value of 0 is apparently special.
    * This needs to be checked. */
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

    return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurfaceImpl_GetUniquenessValue(IDirectDrawSurface7 *iface, LPDWORD pValue)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);

    TRACE("(%p)->(%p)\n",This,pValue);
    *pValue = This->uniqueness_value;
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%d)\n", This, MaxLOD);

    if (!(This->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
        return DDERR_INVALIDOBJECT;

    if(!This->wineD3DTexture)
    {
        ERR("(%p) The DirectDraw texture has no WineD3DTexture!\n", This);
        return DDERR_INVALIDOBJECT;
    }

    return IWineD3DTexture_SetLOD(This->wineD3DTexture,
                                  MaxLOD);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%p)\n", This, MaxLOD);

    if(!MaxLOD)
        return DDERR_INVALIDPARAMS;

    if (!(This->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
        return DDERR_INVALIDOBJECT;

    *MaxLOD = IWineD3DTexture_GetLOD(This->wineD3DTexture);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    HRESULT hr;
    IDirectDrawSurfaceImpl *src = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, Source);
    TRACE("(%p)->(%d,%d,%p,%p,%d): Relay\n", This, dstx, dsty, Source, rsrc, trans);

    hr = IWineD3DSurface_BltFast(This->WineD3DSurface,
                                 dstx, dsty,
                                 src ? src->WineD3DSurface : NULL,
                                 rsrc,
                                 trans);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    TRACE("(%p)->(%p)\n", This, Clipper);

    if(!Clipper)
        return DDERR_INVALIDPARAMS;

    if(This->clipper == NULL)
        return DDERR_NOCLIPPERATTACHED;

    *Clipper = ICOM_INTERFACE(This->clipper, IDirectDrawClipper);
    IDirectDrawClipper_AddRef(*Clipper);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawClipperImpl *oldClipper = This->clipper;

    TRACE("(%p)->(%p)\n",This,Clipper);
    if (ICOM_OBJECT(IDirectDrawClipperImpl, IDirectDrawClipper, Clipper) == This->clipper)
        return DD_OK;

    This->clipper = ICOM_OBJECT(IDirectDrawClipperImpl, IDirectDrawClipper, Clipper);

    if (Clipper != NULL)
        IDirectDrawClipper_AddRef(Clipper);
    if(oldClipper)
        IDirectDrawClipper_Release(ICOM_INTERFACE(oldClipper, IDirectDrawClipper));

    return DD_OK;
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    WINED3DFORMAT newFormat = WINED3DFMT_UNKNOWN;
    HRESULT hr;
    TRACE("(%p)->(%p,%x)\n", This, DDSD, Flags);

    if(!DDSD)
        return DDERR_INVALIDPARAMS;

    if (DDSD->dwFlags & DDSD_LPSURFACE && DDSD->lpSurface)
    {
        ERR("Setting the surface memory isn't supported yet\n");
        return DDERR_INVALIDPARAMS;

    }
    if (DDSD->dwFlags & DDSD_PIXELFORMAT)
    {
        newFormat = PixelFormat_DD2WineD3D(&DDSD->u4.ddpfPixelFormat);

        if(newFormat == WINED3DFMT_UNKNOWN)
        {
            ERR("Requested to set an unknown pixelformat\n");
            return DDERR_INVALIDPARAMS;
        }
        if(newFormat != PixelFormat_DD2WineD3D(&This->surface_desc.u4.ddpfPixelFormat) )
        {
            hr = IWineD3DSurface_SetFormat(This->WineD3DSurface,
                                           newFormat);
            if(hr != DD_OK) return hr;
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
    if (DDSD->dwFlags & DDSD_LPSURFACE)
    {
        hr = IWineD3DSurface_SetMem(This->WineD3DSurface, DDSD->lpSurface);
        if(hr != WINED3D_OK)
        {
            /* No need for a trace here, wined3d does that for us */
            return hr;
        }
    }

    This->surface_desc = *DDSD;

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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IWineD3DPalette *wPal;
    HRESULT hr;
    TRACE("(%p)->(%p): Relay\n", This, Pal);

    if(!Pal)
        return DDERR_INVALIDPARAMS;

    hr = IWineD3DSurface_GetPalette(This->WineD3DSurface, &wPal);
    if(hr != DD_OK) return hr;

    if(wPal)
    {
        hr = IWineD3DPalette_GetParent(wPal, (IUnknown **) Pal);
    }
    else
    {
        *Pal = NULL;
        hr = DDERR_NOPALETTEATTACHED;
    }

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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, surface);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    struct SCKContext ctx = { DD_OK, (WINEDDCOLORKEY *) CKey, Flags };
    TRACE("(%p)->(%x,%p)\n", This, Flags, CKey);

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
            return DDERR_INVALIDPARAMS;
        }
    }
    ctx.ret = IWineD3DSurface_SetColorKey(This->WineD3DSurface,
                                          Flags,
                                          ctx.CKey);
    IDirectDrawSurface7_EnumAttachedSurfaces(iface,
                                             (void *) &ctx,
                                             SetColorKeyEnum);
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
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawSurface7, iface);
    IDirectDrawPalette *oldPal;
    IDirectDrawSurfaceImpl *surf;
    IDirectDrawPaletteImpl *PalImpl = ICOM_OBJECT(IDirectDrawPaletteImpl, IDirectDrawPalette, Pal);
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, Pal);

    /* Find the old palette */
    hr = IDirectDrawSurface_GetPalette(iface, &oldPal);
    if(hr != DD_OK && hr != DDERR_NOPALETTEATTACHED) return hr;
    if(oldPal) IDirectDrawPalette_Release(oldPal);  /* For the GetPalette */

    /* Set the new Palette */
    IWineD3DSurface_SetPalette(This->WineD3DSurface,
                               PalImpl ? PalImpl->wineD3DPalette : NULL);
    /* AddRef the Palette */
    if(Pal) IDirectDrawPalette_AddRef(Pal);

    /* Release the old palette */
    if(oldPal) IDirectDrawPalette_Release(oldPal);

    /* If this is a front buffer, also update the back buffers */
    if(This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
    {
        for(surf = This->next_complex; surf != NULL; surf = surf->next_complex)
        {
            IDirectDrawSurface7_SetPalette(ICOM_INTERFACE(surf, IDirectDrawSurface7),
                                           Pal);
        }
    }

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
    IDirectDrawSurfaceImpl_AddAttachedSurface,
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
