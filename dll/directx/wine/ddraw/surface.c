/* DirectDraw Surface Implementation
 *
 * Copyright (c) 1997-2000 Marcus Meissner
 * Copyright (c) 1998-2000 Lionel Ulmer
 * Copyright (c) 2000-2001 TransGaming Technologies Inc.
 * Copyright (c) 2006 Stefan Dösinger
 * Copyright (c) 2011 Ričardas Barkauskas for CodeWeavers
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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface2(IDirectDrawSurface2 *iface);
static IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface);

static inline IDirectDrawSurfaceImpl *impl_from_IDirectDrawGammaControl(IDirectDrawGammaControl *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawGammaControl_iface);
}

/* This is slow, of course. Also, in case of locks, we can't prevent other
 * applications from drawing to the screen while we've locked the frontbuffer.
 * We'd like to do this in wined3d instead, but for that to work wined3d needs
 * to support windowless rendering first. */
static HRESULT ddraw_surface_update_frontbuffer(IDirectDrawSurfaceImpl *surface, const RECT *rect, BOOL read)
{
    HDC surface_dc, screen_dc;
    int x, y, w, h;
    HRESULT hr;
    BOOL ret;

    if (!rect)
    {
        x = 0;
        y = 0;
        w = surface->surface_desc.dwWidth;
        h = surface->surface_desc.dwHeight;
    }
    else
    {
        x = rect->left;
        y = rect->top;
        w = rect->right - rect->left;
        h = rect->bottom - rect->top;
    }

    if (w <= 0 || h <= 0)
        return DD_OK;

    if (surface->ddraw->swapchain_window)
    {
        /* Nothing to do, we control the frontbuffer, or at least the parts we
         * care about. */
        if (read)
            return DD_OK;

        return wined3d_surface_blt(surface->ddraw->wined3d_frontbuffer, rect,
                surface->wined3d_surface, rect, 0, NULL, WINED3D_TEXF_POINT);
    }

    if (FAILED(hr = wined3d_surface_getdc(surface->wined3d_surface, &surface_dc)))
    {
        ERR("Failed to get surface DC, hr %#x.\n", hr);
        return hr;
    }

    if (!(screen_dc = GetDC(NULL)))
    {
        wined3d_surface_releasedc(surface->wined3d_surface, surface_dc);
        ERR("Failed to get screen DC.\n");
        return E_FAIL;
    }

    if (read)
        ret = BitBlt(surface_dc, x, y, w, h,
                screen_dc, x, y, SRCCOPY);
    else
        ret = BitBlt(screen_dc, x, y, w, h,
                surface_dc, x, y, SRCCOPY);

    ReleaseDC(NULL, screen_dc);
    wined3d_surface_releasedc(surface->wined3d_surface, surface_dc);

    if (!ret)
    {
        ERR("Failed to blit to/from screen.\n");
        return E_FAIL;
    }

    return DD_OK;
}

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
static HRESULT WINAPI ddraw_surface7_QueryInterface(IDirectDrawSurface7 *iface, REFIID riid, void **obj)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obj);

    /* According to COM docs, if the QueryInterface fails, obj should be set to NULL */
    *obj = NULL;

    if(!riid)
        return DDERR_INVALIDPARAMS;

    if (IsEqualGUID(riid, &IID_IUnknown)
     || IsEqualGUID(riid, &IID_IDirectDrawSurface7) )
    {
        IDirectDrawSurface7_AddRef(iface);
        *obj = iface;
        TRACE("(%p) returning IDirectDrawSurface7 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IDirectDrawSurface4))
    {
        IDirectDrawSurface4_AddRef(&This->IDirectDrawSurface4_iface);
        *obj = &This->IDirectDrawSurface4_iface;
        TRACE("(%p) returning IDirectDrawSurface4 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IDirectDrawSurface3))
    {
        IDirectDrawSurface3_AddRef(&This->IDirectDrawSurface3_iface);
        *obj = &This->IDirectDrawSurface3_iface;
        TRACE("(%p) returning IDirectDrawSurface3 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IDirectDrawSurface2))
    {
        IDirectDrawSurface2_AddRef(&This->IDirectDrawSurface2_iface);
        *obj = &This->IDirectDrawSurface2_iface;
        TRACE("(%p) returning IDirectDrawSurface2 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IDirectDrawSurface))
    {
        IDirectDrawSurface_AddRef(&This->IDirectDrawSurface_iface);
        *obj = &This->IDirectDrawSurface_iface;
        TRACE("(%p) returning IDirectDrawSurface interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_IDirectDrawGammaControl) )
    {
        IDirectDrawGammaControl_AddRef(&This->IDirectDrawGammaControl_iface);
        *obj = &This->IDirectDrawGammaControl_iface;
        TRACE("(%p) returning IDirectDrawGammaControl interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_D3DDEVICE_WineD3D) ||
             IsEqualGUID(riid, &IID_IDirect3DHALDevice)||
             IsEqualGUID(riid, &IID_IDirect3DRGBDevice) )
    {
        IDirect3DDevice7 *d3d;
        IDirect3DDeviceImpl *device_impl;

        /* Call into IDirect3D7 for creation */
        IDirect3D7_CreateDevice(&This->ddraw->IDirect3D7_iface, riid, &This->IDirectDrawSurface7_iface,
                &d3d);

        if (d3d)
        {
            device_impl = impl_from_IDirect3DDevice7(d3d);
            device_impl->from_surface = TRUE;
            *obj = &device_impl->IDirect3DDevice_iface;
            TRACE("(%p) Returning IDirect3DDevice interface at %p\n", This, *obj);
            return S_OK;
        }

        WARN("Unable to create a IDirect3DDevice instance, returning E_NOINTERFACE\n");
        return E_NOINTERFACE;
    }
    else if (IsEqualGUID( &IID_IDirect3DTexture, riid ) ||
             IsEqualGUID( &IID_IDirect3DTexture2, riid ))
    {
        if (IsEqualGUID( &IID_IDirect3DTexture, riid ))
        {
            *obj = &This->IDirect3DTexture_iface;
            TRACE(" returning Direct3DTexture interface at %p.\n", *obj);
        }
        else
        {
            *obj = &This->IDirect3DTexture2_iface;
            TRACE(" returning Direct3DTexture2 interface at %p.\n", *obj);
        }
        IUnknown_AddRef( (IUnknown *) *obj);
        return S_OK;
    }

    ERR("No interface\n");
    return E_NOINTERFACE;
}

static HRESULT WINAPI ddraw_surface4_QueryInterface(IDirectDrawSurface4 *iface, REFIID riid, void **object)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&This->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI ddraw_surface3_QueryInterface(IDirectDrawSurface3 *iface, REFIID riid, void **object)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&This->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI ddraw_surface2_QueryInterface(IDirectDrawSurface2 *iface, REFIID riid, void **object)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&This->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI ddraw_surface1_QueryInterface(IDirectDrawSurface *iface, REFIID riid, void **object)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&This->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI ddraw_gamma_control_QueryInterface(IDirectDrawGammaControl *iface,
        REFIID riid, void **object)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawGammaControl(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&This->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI d3d_texture2_QueryInterface(IDirect3DTexture2 *iface, REFIID riid, void **object)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirect3DTexture2(iface);
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&This->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI d3d_texture1_QueryInterface(IDirect3DTexture *iface, REFIID riid, void **object)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirect3DTexture(iface);
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&This->IDirectDrawSurface7_iface, riid, object);
}

static void ddraw_surface_add_iface(IDirectDrawSurfaceImpl *This)
{
    ULONG iface_count = InterlockedIncrement(&This->iface_count);
    TRACE("%p increasing iface count to %u.\n", This, iface_count);

    if (iface_count == 1)
    {
        wined3d_mutex_lock();
        if (This->wined3d_surface)
            wined3d_surface_incref(This->wined3d_surface);
        if (This->wined3d_texture)
            wined3d_texture_incref(This->wined3d_texture);
        wined3d_mutex_unlock();
    }
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
static ULONG WINAPI ddraw_surface7_AddRef(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    ULONG refcount = InterlockedIncrement(&This->ref7);

    TRACE("iface %p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        ddraw_surface_add_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_surface4_AddRef(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    ULONG refcount = InterlockedIncrement(&This->ref4);

    TRACE("iface %p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        ddraw_surface_add_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_surface3_AddRef(IDirectDrawSurface3 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    ULONG refcount = InterlockedIncrement(&This->ref3);

    TRACE("iface %p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        ddraw_surface_add_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_surface2_AddRef(IDirectDrawSurface2 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    ULONG refcount = InterlockedIncrement(&This->ref2);

    TRACE("iface %p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        ddraw_surface_add_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_surface1_AddRef(IDirectDrawSurface *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    ULONG refcount = InterlockedIncrement(&This->ref1);

    TRACE("iface %p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        ddraw_surface_add_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_gamma_control_AddRef(IDirectDrawGammaControl *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawGammaControl(iface);
    ULONG refcount = InterlockedIncrement(&This->gamma_count);

    TRACE("iface %p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        ddraw_surface_add_iface(This);
    }

    return refcount;
}

static ULONG WINAPI d3d_texture2_AddRef(IDirect3DTexture2 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirect3DTexture2(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface1_AddRef(&This->IDirectDrawSurface_iface);
}

static ULONG WINAPI d3d_texture1_AddRef(IDirect3DTexture *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirect3DTexture(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface1_AddRef(&This->IDirectDrawSurface_iface);
}

/*****************************************************************************
 * ddraw_surface_destroy
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
static void ddraw_surface_destroy(IDirectDrawSurfaceImpl *This)
{
    TRACE("surface %p.\n", This);

    /* Check the iface count and give a warning */
    if(This->iface_count > 1)
    {
        /* This can happen when a complex surface is destroyed,
         * because the 2nd surface was addref()ed when the app
         * called GetAttachedSurface
         */
        WARN("(%p): Destroying surface with refcounts 7: %d 4: %d 3: %d 2: %d 1: %d\n",
                This, This->ref7, This->ref4, This->ref3, This->ref2, This->ref1);
    }

    if (This->wined3d_surface)
        wined3d_surface_decref(This->wined3d_surface);
}

static void ddraw_surface_cleanup(IDirectDrawSurfaceImpl *surface)
{
    IDirectDrawSurfaceImpl *surf;
    IUnknown *ifaceToRelease;
    UINT i;

    TRACE("surface %p.\n", surface);

    /* The refcount test shows that the palette is detached when the surface
     * is destroyed. */
    IDirectDrawSurface7_SetPalette(&surface->IDirectDrawSurface7_iface, NULL);

    /* Loop through all complex attached surfaces and destroy them.
     *
     * Yet again, only the root can have more than one complexly attached
     * surface, all the others have a total of one. */
    for (i = 0; i < MAX_COMPLEX_ATTACHED; ++i)
    {
        if (!surface->complex_array[i])
            break;

        surf = surface->complex_array[i];
        surface->complex_array[i] = NULL;
        while (surf)
        {
            IDirectDrawSurfaceImpl *destroy = surf;
            surf = surf->complex_array[0];              /* Iterate through the "tree" */
            ddraw_surface_destroy(destroy);             /* Destroy it */
        }
    }

    ifaceToRelease = surface->ifaceToRelease;

    /* Destroy the root surface. */
    ddraw_surface_destroy(surface);

    /* Reduce the ddraw refcount */
    if (ifaceToRelease)
        IUnknown_Release(ifaceToRelease);
}

ULONG ddraw_surface_release_iface(IDirectDrawSurfaceImpl *This)
{
    ULONG iface_count = InterlockedDecrement(&This->iface_count);
    TRACE("%p decreasing iface count to %u.\n", This, iface_count);

    if (iface_count == 0)
    {
        /* Complex attached surfaces are destroyed implicitly when the root is released */
        wined3d_mutex_lock();
        if(!This->is_complex_root)
        {
            WARN("(%p) Attempt to destroy a surface that is not a complex root\n", This);
            wined3d_mutex_unlock();
            return iface_count;
        }
        if (This->wined3d_texture) /* If it's a texture, destroy the wined3d texture. */
            wined3d_texture_decref(This->wined3d_texture);
        else
            ddraw_surface_cleanup(This);
        wined3d_mutex_unlock();
    }

    return iface_count;
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
static ULONG WINAPI ddraw_surface7_Release(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    ULONG refcount = InterlockedDecrement(&This->ref7);

    TRACE("iface %p decreasing refcount to %u.\n", iface, refcount);

    if (refcount == 0)
    {
        ddraw_surface_release_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_surface4_Release(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    ULONG refcount = InterlockedDecrement(&This->ref4);

    TRACE("iface %p decreasing refcount to %u.\n", iface, refcount);

    if (refcount == 0)
    {
        ddraw_surface_release_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_surface3_Release(IDirectDrawSurface3 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    ULONG refcount = InterlockedDecrement(&This->ref3);

    TRACE("iface %p decreasing refcount to %u.\n", iface, refcount);

    if (refcount == 0)
    {
        ddraw_surface_release_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_surface2_Release(IDirectDrawSurface2 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    ULONG refcount = InterlockedDecrement(&This->ref2);

    TRACE("iface %p decreasing refcount to %u.\n", iface, refcount);

    if (refcount == 0)
    {
        ddraw_surface_release_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_surface1_Release(IDirectDrawSurface *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    ULONG refcount = InterlockedDecrement(&This->ref1);

    TRACE("iface %p decreasing refcount to %u.\n", iface, refcount);

    if (refcount == 0)
    {
        ddraw_surface_release_iface(This);
    }

    return refcount;
}

static ULONG WINAPI ddraw_gamma_control_Release(IDirectDrawGammaControl *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawGammaControl(iface);
    ULONG refcount = InterlockedDecrement(&This->gamma_count);

    TRACE("iface %p decreasing refcount to %u.\n", iface, refcount);

    if (refcount == 0)
    {
        ddraw_surface_release_iface(This);
    }

    return refcount;
}

static ULONG WINAPI d3d_texture2_Release(IDirect3DTexture2 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirect3DTexture2(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface1_Release(&This->IDirectDrawSurface_iface);
}

static ULONG WINAPI d3d_texture1_Release(IDirect3DTexture *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirect3DTexture(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface1_Release(&This->IDirectDrawSurface_iface);
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
static HRESULT WINAPI ddraw_surface7_GetAttachedSurface(IDirectDrawSurface7 *iface,
        DDSCAPS2 *Caps, IDirectDrawSurface7 **Surface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawSurfaceImpl *surf;
    DDSCAPS2 our_caps;
    int i;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, Caps, Surface);

    wined3d_mutex_lock();

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
            *Surface = &surf->IDirectDrawSurface7_iface;
            ddraw_surface7_AddRef(*Surface);
            wined3d_mutex_unlock();

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
            *Surface = &surf->IDirectDrawSurface7_iface;
            ddraw_surface7_AddRef(*Surface);
            wined3d_mutex_unlock();
            return DD_OK;
        }
    }

    TRACE("(%p) Didn't find a valid surface\n", This);

    wined3d_mutex_unlock();

    *Surface = NULL;
    return DDERR_NOTFOUND;
}

static HRESULT WINAPI ddraw_surface4_GetAttachedSurface(IDirectDrawSurface4 *iface,
        DDSCAPS2 *caps, IDirectDrawSurface4 **attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurface7 *attachment7;
    IDirectDrawSurfaceImpl *attachment_impl;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    hr = ddraw_surface7_GetAttachedSurface(&This->IDirectDrawSurface7_iface,
            caps, &attachment7);
    if (FAILED(hr))
    {
        *attachment = NULL;
        return hr;
    }
    attachment_impl = impl_from_IDirectDrawSurface7(attachment7);
    *attachment = &attachment_impl->IDirectDrawSurface4_iface;
    ddraw_surface4_AddRef(*attachment);
    ddraw_surface7_Release(attachment7);

    return hr;
}

static HRESULT WINAPI ddraw_surface3_GetAttachedSurface(IDirectDrawSurface3 *iface,
        DDSCAPS *caps, IDirectDrawSurface3 **attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurface7 *attachment7;
    IDirectDrawSurfaceImpl *attachment_impl;
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    caps2.dwCaps  = caps->dwCaps;
    caps2.dwCaps2 = 0;
    caps2.dwCaps3 = 0;
    caps2.u1.dwCaps4 = 0;

    hr = ddraw_surface7_GetAttachedSurface(&This->IDirectDrawSurface7_iface,
            &caps2, &attachment7);
    if (FAILED(hr))
    {
        *attachment = NULL;
        return hr;
    }
    attachment_impl = impl_from_IDirectDrawSurface7(attachment7);
    *attachment = &attachment_impl->IDirectDrawSurface3_iface;
    ddraw_surface3_AddRef(*attachment);
    ddraw_surface7_Release(attachment7);

    return hr;
}

static HRESULT WINAPI ddraw_surface2_GetAttachedSurface(IDirectDrawSurface2 *iface,
        DDSCAPS *caps, IDirectDrawSurface2 **attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    IDirectDrawSurface7 *attachment7;
    IDirectDrawSurfaceImpl *attachment_impl;
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    caps2.dwCaps  = caps->dwCaps;
    caps2.dwCaps2 = 0;
    caps2.dwCaps3 = 0;
    caps2.u1.dwCaps4 = 0;

    hr = ddraw_surface7_GetAttachedSurface(&This->IDirectDrawSurface7_iface,
            &caps2, &attachment7);
    if (FAILED(hr))
    {
        *attachment = NULL;
        return hr;
    }
    attachment_impl = impl_from_IDirectDrawSurface7(attachment7);
    *attachment = &attachment_impl->IDirectDrawSurface2_iface;
    ddraw_surface2_AddRef(*attachment);
    ddraw_surface7_Release(attachment7);

    return hr;
}

static HRESULT WINAPI ddraw_surface1_GetAttachedSurface(IDirectDrawSurface *iface,
        DDSCAPS *caps, IDirectDrawSurface **attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    IDirectDrawSurface7 *attachment7;
    IDirectDrawSurfaceImpl *attachment_impl;
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    caps2.dwCaps  = caps->dwCaps;
    caps2.dwCaps2 = 0;
    caps2.dwCaps3 = 0;
    caps2.u1.dwCaps4 = 0;

    hr = ddraw_surface7_GetAttachedSurface(&This->IDirectDrawSurface7_iface,
            &caps2, &attachment7);
    if (FAILED(hr))
    {
        *attachment = NULL;
        return hr;
    }
    attachment_impl = impl_from_IDirectDrawSurface7(attachment7);
    *attachment = &attachment_impl->IDirectDrawSurface_iface;
    ddraw_surface1_AddRef(*attachment);
    ddraw_surface7_Release(attachment7);

    return hr;
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
static HRESULT surface_lock(IDirectDrawSurfaceImpl *This,
        RECT *Rect, DDSURFACEDESC2 *DDSD, DWORD Flags, HANDLE h)
{
    struct wined3d_mapped_rect mapped_rect;
    HRESULT hr = DD_OK;

    TRACE("This %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            This, wine_dbgstr_rect(Rect), DDSD, Flags, h);

    /* This->surface_desc.dwWidth and dwHeight are changeable, thus lock */
    wined3d_mutex_lock();

    /* Should I check for the handle to be NULL?
     *
     * The DDLOCK flags and the D3DLOCK flags are equal
     * for the supported values. The others are ignored by WineD3D
     */

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
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }
    }

    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        hr = ddraw_surface_update_frontbuffer(This, Rect, TRUE);
    if (SUCCEEDED(hr))
        hr = wined3d_surface_map(This->wined3d_surface, &mapped_rect, Rect, Flags);
    if (FAILED(hr))
    {
        wined3d_mutex_unlock();
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

    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
    {
        if (Flags & DDLOCK_READONLY)
            memset(&This->ddraw->primary_lock, 0, sizeof(This->ddraw->primary_lock));
        else if (Rect)
            This->ddraw->primary_lock = *Rect;
        else
            SetRect(&This->ddraw->primary_lock, 0, 0, This->surface_desc.dwWidth, This->surface_desc.dwHeight);
    }

    /* Override the memory area. The pitch should be set already. Strangely windows
     * does not set the LPSURFACE flag on locked surfaces !?!.
     * DDSD->dwFlags |= DDSD_LPSURFACE;
     */
    This->surface_desc.lpSurface = mapped_rect.data;
    DD_STRUCT_COPY_BYSIZE(DDSD,&(This->surface_desc));

    TRACE("locked surface returning description :\n");
    if (TRACE_ON(ddraw)) DDRAW_dump_surface_desc(DDSD);

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface7_Lock(IDirectDrawSurface7 *iface,
        RECT *rect, DDSURFACEDESC2 *surface_desc, DWORD flags, HANDLE h)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_desc) return DDERR_INVALIDPARAMS;
    if (surface_desc->dwSize != sizeof(DDSURFACEDESC) &&
            surface_desc->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Invalid structure size %d, returning DDERR_INVALIDPARAMS\n", surface_desc->dwSize);
        return DDERR_INVALIDPARAMS;
    }
    return surface_lock(This, rect, surface_desc, flags, h);
}

static HRESULT WINAPI ddraw_surface4_Lock(IDirectDrawSurface4 *iface, RECT *rect,
        DDSURFACEDESC2 *surface_desc, DWORD flags, HANDLE h)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_desc) return DDERR_INVALIDPARAMS;
    if (surface_desc->dwSize != sizeof(DDSURFACEDESC) &&
            surface_desc->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Invalid structure size %d, returning DDERR_INVALIDPARAMS\n", surface_desc->dwSize);
        return DDERR_INVALIDPARAMS;
    }
    return surface_lock(This, rect, surface_desc, flags, h);
}

static HRESULT WINAPI ddraw_surface3_Lock(IDirectDrawSurface3 *iface, RECT *rect,
        DDSURFACEDESC *surface_desc, DWORD flags, HANDLE h)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 surface_desc2;
    HRESULT hr;
    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_desc) return DDERR_INVALIDPARAMS;
    if (surface_desc->dwSize != sizeof(DDSURFACEDESC) &&
            surface_desc->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Invalid structure size %d, returning DDERR_INVALIDPARAMS\n", surface_desc->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    surface_desc2.dwSize = surface_desc->dwSize;
    surface_desc2.dwFlags = 0;
    hr = surface_lock(This, rect, &surface_desc2, flags, h);
    DDSD2_to_DDSD(&surface_desc2, surface_desc);
    surface_desc->dwSize = surface_desc2.dwSize;
    return hr;
}

static HRESULT WINAPI ddraw_surface2_Lock(IDirectDrawSurface2 *iface, RECT *rect,
        DDSURFACEDESC *surface_desc, DWORD flags, HANDLE h)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    DDSURFACEDESC2 surface_desc2;
    HRESULT hr;
    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_desc) return DDERR_INVALIDPARAMS;
    if (surface_desc->dwSize != sizeof(DDSURFACEDESC) &&
            surface_desc->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Invalid structure size %d, returning DDERR_INVALIDPARAMS\n", surface_desc->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    surface_desc2.dwSize = surface_desc->dwSize;
    surface_desc2.dwFlags = 0;
    hr = surface_lock(This, rect, &surface_desc2, flags, h);
    DDSD2_to_DDSD(&surface_desc2, surface_desc);
    surface_desc->dwSize = surface_desc2.dwSize;
    return hr;
}

static HRESULT WINAPI ddraw_surface1_Lock(IDirectDrawSurface *iface, RECT *rect,
        DDSURFACEDESC *surface_desc, DWORD flags, HANDLE h)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    DDSURFACEDESC2 surface_desc2;
    HRESULT hr;
    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_desc) return DDERR_INVALIDPARAMS;
    if (surface_desc->dwSize != sizeof(DDSURFACEDESC) &&
            surface_desc->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Invalid structure size %d, returning DDERR_INVALIDPARAMS\n", surface_desc->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    surface_desc2.dwSize = surface_desc->dwSize;
    surface_desc2.dwFlags = 0;
    hr = surface_lock(This, rect, &surface_desc2, flags, h);
    DDSD2_to_DDSD(&surface_desc2, surface_desc);
    surface_desc->dwSize = surface_desc2.dwSize;
    return hr;
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
static HRESULT WINAPI ddraw_surface7_Unlock(IDirectDrawSurface7 *iface, RECT *pRect)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(pRect));

    wined3d_mutex_lock();
    hr = wined3d_surface_unmap(This->wined3d_surface);
    if (SUCCEEDED(hr))
    {
        if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
            hr = ddraw_surface_update_frontbuffer(This, &This->ddraw->primary_lock, FALSE);
        This->surface_desc.lpSurface = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_Unlock(IDirectDrawSurface4 *iface, RECT *pRect)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, rect %p.\n", iface, pRect);

    return ddraw_surface7_Unlock(&This->IDirectDrawSurface7_iface, pRect);
}

static HRESULT WINAPI ddraw_surface3_Unlock(IDirectDrawSurface3 *iface, void *data)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, data %p.\n", iface, data);

    /* data might not be the LPRECT of later versions, so drop it. */
    return ddraw_surface7_Unlock(&This->IDirectDrawSurface7_iface, NULL);
}

static HRESULT WINAPI ddraw_surface2_Unlock(IDirectDrawSurface2 *iface, void *data)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, data %p.\n", iface, data);

    /* data might not be the LPRECT of later versions, so drop it. */
    return ddraw_surface7_Unlock(&This->IDirectDrawSurface7_iface, NULL);
}

static HRESULT WINAPI ddraw_surface1_Unlock(IDirectDrawSurface *iface, void *data)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, data %p.\n", iface, data);

    /* data might not be the LPRECT of later versions, so drop it. */
    return ddraw_surface7_Unlock(&This->IDirectDrawSurface7_iface, NULL);
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
static HRESULT WINAPI ddraw_surface7_Flip(IDirectDrawSurface7 *iface, IDirectDrawSurface7 *DestOverride, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawSurfaceImpl *Override = unsafe_impl_from_IDirectDrawSurface7(DestOverride);
    IDirectDrawSurface7 *Override7;
    HRESULT hr;

    TRACE("iface %p, dst %p, flags %#x.\n", iface, DestOverride, Flags);

    /* Flip has to be called from a front buffer
     * What about overlay surfaces, AFAIK they can flip too?
     */
    if( !(This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_OVERLAY)) )
        return DDERR_INVALIDOBJECT; /* Unchecked */

    wined3d_mutex_lock();

    /* WineD3D doesn't keep track of attached surface, so find the target */
    if(!Override)
    {
        DDSCAPS2 Caps;

        memset(&Caps, 0, sizeof(Caps));
        Caps.dwCaps |= DDSCAPS_BACKBUFFER;
        hr = ddraw_surface7_GetAttachedSurface(iface, &Caps, &Override7);
        if(hr != DD_OK)
        {
            ERR("Can't find a flip target\n");
            wined3d_mutex_unlock();
            return DDERR_NOTFLIPPABLE; /* Unchecked */
        }
        Override = impl_from_IDirectDrawSurface7(Override7);

        /* For the GetAttachedSurface */
        ddraw_surface7_Release(Override7);
    }

    hr = wined3d_surface_flip(This->wined3d_surface, Override->wined3d_surface, Flags);
    if (SUCCEEDED(hr) && This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        hr = ddraw_surface_update_frontbuffer(This, NULL, FALSE);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_Flip(IDirectDrawSurface4 *iface, IDirectDrawSurface4 *dst, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *dst_impl = unsafe_impl_from_IDirectDrawSurface4(dst);
    TRACE("iface %p, dst %p, flags %#x.\n", iface, dst, flags);

    return ddraw_surface7_Flip(&This->IDirectDrawSurface7_iface,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, flags);
}

static HRESULT WINAPI ddraw_surface3_Flip(IDirectDrawSurface3 *iface, IDirectDrawSurface3 *dst, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *dst_impl = unsafe_impl_from_IDirectDrawSurface3(dst);
    TRACE("iface %p, dst %p, flags %#x.\n", iface, dst, flags);

    return ddraw_surface7_Flip(&This->IDirectDrawSurface7_iface,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, flags);
}

static HRESULT WINAPI ddraw_surface2_Flip(IDirectDrawSurface2 *iface, IDirectDrawSurface2 *dst, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    IDirectDrawSurfaceImpl *dst_impl = unsafe_impl_from_IDirectDrawSurface2(dst);
    TRACE("iface %p, dst %p, flags %#x.\n", iface, dst, flags);

    return ddraw_surface7_Flip(&This->IDirectDrawSurface7_iface,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, flags);
}

static HRESULT WINAPI ddraw_surface1_Flip(IDirectDrawSurface *iface, IDirectDrawSurface *dst, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    IDirectDrawSurfaceImpl *dst_impl = unsafe_impl_from_IDirectDrawSurface(dst);
    TRACE("iface %p, dst %p, flags %#x.\n", iface, dst, flags);

    return ddraw_surface7_Flip(&This->IDirectDrawSurface7_iface,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, flags);
}

static HRESULT ddraw_surface_blt_clipped(IDirectDrawSurfaceImpl *dst_surface, const RECT *dst_rect_in,
        IDirectDrawSurfaceImpl *src_surface, const RECT *src_rect_in, DWORD flags,
        const WINEDDBLTFX *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_surface *wined3d_src_surface = src_surface ? src_surface->wined3d_surface : NULL;
    RECT src_rect, dst_rect;
    float scale_x, scale_y;
    const RECT *clip_rect;
    UINT clip_list_size;
    RGNDATA *clip_list;
    HRESULT hr = DD_OK;
    UINT i;

    if (!dst_surface->clipper)
    {
        if (src_surface && src_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
            hr = ddraw_surface_update_frontbuffer(src_surface, src_rect_in, TRUE);
        if (SUCCEEDED(hr))
            hr = wined3d_surface_blt(dst_surface->wined3d_surface, dst_rect_in,
                    wined3d_src_surface, src_rect_in, flags, fx, filter);
        if (SUCCEEDED(hr) && (dst_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER))
            hr = ddraw_surface_update_frontbuffer(dst_surface, dst_rect_in, FALSE);

        return hr;
    }

    if (!dst_rect_in)
    {
        dst_rect.left = 0;
        dst_rect.top = 0;
        dst_rect.right = dst_surface->surface_desc.dwWidth;
        dst_rect.bottom = dst_surface->surface_desc.dwHeight;
    }
    else
    {
        dst_rect = *dst_rect_in;
    }

    if (IsRectEmpty(&dst_rect))
        return DDERR_INVALIDRECT;

    if (src_surface)
    {
        if (!src_rect_in)
        {
            src_rect.left = 0;
            src_rect.top = 0;
            src_rect.right = src_surface->surface_desc.dwWidth;
            src_rect.bottom = src_surface->surface_desc.dwHeight;
        }
        else
        {
            src_rect = *src_rect_in;
        }

        if (IsRectEmpty(&src_rect))
            return DDERR_INVALIDRECT;
    }
    else
    {
        SetRect(&src_rect, 0, 0, 0, 0);
    }

    scale_x = (float)(src_rect.right - src_rect.left) / (float)(dst_rect.right - dst_rect.left);
    scale_y = (float)(src_rect.bottom - src_rect.top) / (float)(dst_rect.bottom - dst_rect.top);

    if (FAILED(hr = IDirectDrawClipper_GetClipList(&dst_surface->clipper->IDirectDrawClipper_iface,
            &dst_rect, NULL, &clip_list_size)))
    {
        WARN("Failed to get clip list size, hr %#x.\n", hr);
        return hr;
    }

    if (!(clip_list = HeapAlloc(GetProcessHeap(), 0, clip_list_size)))
    {
        WARN("Failed to allocate clip list.\n");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = IDirectDrawClipper_GetClipList(&dst_surface->clipper->IDirectDrawClipper_iface,
            &dst_rect, clip_list, &clip_list_size)))
    {
        WARN("Failed to get clip list, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, clip_list);
        return hr;
    }

    clip_rect = (RECT *)clip_list->Buffer;
    for (i = 0; i < clip_list->rdh.nCount; ++i)
    {
        RECT src_rect_clipped = src_rect;

        if (src_surface)
        {
            src_rect_clipped.left += (LONG)((clip_rect[i].left - dst_rect.left) * scale_x);
            src_rect_clipped.top += (LONG)((clip_rect[i].top - dst_rect.top) * scale_y);
            src_rect_clipped.right -= (LONG)((dst_rect.right - clip_rect[i].right) * scale_x);
            src_rect_clipped.bottom -= (LONG)((dst_rect.bottom - clip_rect[i].bottom) * scale_y);

            if (src_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
            {
                if (FAILED(hr = ddraw_surface_update_frontbuffer(src_surface, &src_rect_clipped, TRUE)))
                    break;
            }
        }

        if (FAILED(hr = wined3d_surface_blt(dst_surface->wined3d_surface, &clip_rect[i],
                wined3d_src_surface, &src_rect_clipped, flags, fx, filter)))
            break;

        if (dst_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        {
            if (FAILED(hr = ddraw_surface_update_frontbuffer(dst_surface, &clip_rect[i], FALSE)))
                break;
        }
    }

    HeapFree(GetProcessHeap(), 0, clip_list);
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
static HRESULT WINAPI ddraw_surface7_Blt(IDirectDrawSurface7 *iface, RECT *DestRect,
        IDirectDrawSurface7 *SrcSurface, RECT *SrcRect, DWORD Flags, DDBLTFX *DDBltFx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawSurfaceImpl *Src = unsafe_impl_from_IDirectDrawSurface7(SrcSurface);
    HRESULT hr = DD_OK;

    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(DestRect), SrcSurface, wine_dbgstr_rect(SrcRect), Flags, DDBltFx);

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

    wined3d_mutex_lock();

    if(Flags & DDBLT_KEYSRC && (!Src || !(Src->surface_desc.dwFlags & DDSD_CKSRCBLT))) {
        WARN("DDBLT_KEYDEST blit without color key in surface, returning DDERR_INVALIDPARAMS\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    /* TODO: Check if the DDBltFx contains any ddraw surface pointers. If it
     * does, copy the struct, and replace the ddraw surfaces with the wined3d
     * surfaces. So far no blitting operations using surfaces in the bltfx
     * struct are supported anyway. */
    hr = ddraw_surface_blt_clipped(This, DestRect, Src, SrcRect,
            Flags, (WINEDDBLTFX *)DDBltFx, WINED3D_TEXF_LINEAR);

    wined3d_mutex_unlock();
    switch(hr)
    {
        case WINED3DERR_NOTAVAILABLE:       return DDERR_UNSUPPORTED;
        case WINED3DERR_WRONGTEXTUREFORMAT: return DDERR_INVALIDPIXELFORMAT;
        default:                            return hr;
    }
}

static HRESULT WINAPI ddraw_surface4_Blt(IDirectDrawSurface4 *iface, RECT *dst_rect,
        IDirectDrawSurface4 *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *src = unsafe_impl_from_IDirectDrawSurface4(src_surface);
    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return ddraw_surface7_Blt(&This->IDirectDrawSurface7_iface, dst_rect,
            src ? &src->IDirectDrawSurface7_iface : NULL, src_rect, flags, fx);
}

static HRESULT WINAPI ddraw_surface3_Blt(IDirectDrawSurface3 *iface, RECT *dst_rect,
        IDirectDrawSurface3 *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *src_impl = unsafe_impl_from_IDirectDrawSurface3(src_surface);
    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return ddraw_surface7_Blt(&This->IDirectDrawSurface7_iface, dst_rect,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags, fx);
}

static HRESULT WINAPI ddraw_surface2_Blt(IDirectDrawSurface2 *iface, RECT *dst_rect,
        IDirectDrawSurface2 *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    IDirectDrawSurfaceImpl *src_impl = unsafe_impl_from_IDirectDrawSurface2(src_surface);
    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return ddraw_surface7_Blt(&This->IDirectDrawSurface7_iface, dst_rect,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags, fx);
}

static HRESULT WINAPI ddraw_surface1_Blt(IDirectDrawSurface *iface, RECT *dst_rect,
        IDirectDrawSurface *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    IDirectDrawSurfaceImpl *src_impl = unsafe_impl_from_IDirectDrawSurface(src_surface);
    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return ddraw_surface7_Blt(&This->IDirectDrawSurface7_iface, dst_rect,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags, fx);
}

/*****************************************************************************
 * IDirectDrawSurface7::AddAttachedSurface
 *
 * Attaches a surface to another surface. How the surface attachments work
 * is not totally understood yet, and this method is prone to problems.
 * The surface that is attached is AddRef-ed.
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
 * ddraw_surface_attach_surface is the real thing,
 * ddraw_surface7_AddAttachedSurface is a wrapper around it that
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
static HRESULT ddraw_surface_attach_surface(IDirectDrawSurfaceImpl *This, IDirectDrawSurfaceImpl *Surf)
{
    TRACE("surface %p, attachment %p.\n", This, Surf);

    if(Surf == This)
        return DDERR_CANNOTATTACHSURFACE; /* unchecked */

    wined3d_mutex_lock();

    /* Check if the surface is already attached somewhere */
    if (Surf->next_attached || Surf->first_attached != Surf)
    {
        /* TODO: Test for the structure of the manual attachment. Is it a
         * chain or a list? What happens if one surface is attached to 2
         * different surfaces? */
        WARN("Surface %p is already attached somewhere. next_attached %p, first_attached %p.\n",
                Surf, Surf->next_attached, Surf->first_attached);

        wined3d_mutex_unlock();
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

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface7_AddAttachedSurface(IDirectDrawSurface7 *iface, IDirectDrawSurface7 *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface7(attachment);
    HRESULT hr;

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    /* Version 7 of this interface seems to refuse everything except z buffers, as per msdn */
    if(!(attachment_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
    {

        WARN("Application tries to attach a non Z buffer surface. caps %08x\n",
              attachment_impl->surface_desc.ddsCaps.dwCaps);
        return DDERR_CANNOTATTACHSURFACE;
    }

    hr = ddraw_surface_attach_surface(This, attachment_impl);
    if (FAILED(hr))
    {
        return hr;
    }
    ddraw_surface7_AddRef(attachment);
    attachment_impl->attached_iface = (IUnknown *)attachment;
    return hr;
}

static HRESULT WINAPI ddraw_surface4_AddAttachedSurface(IDirectDrawSurface4 *iface, IDirectDrawSurface4 *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface4(attachment);
    HRESULT hr;

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    hr = ddraw_surface7_AddAttachedSurface(&This->IDirectDrawSurface7_iface,
            attachment_impl ? &attachment_impl->IDirectDrawSurface7_iface : NULL);
    if (FAILED(hr))
    {
        return hr;
    }
    ddraw_surface4_AddRef(attachment);
    ddraw_surface7_Release(&attachment_impl->IDirectDrawSurface7_iface);
    attachment_impl->attached_iface = (IUnknown *)attachment;
    return hr;
}
static HRESULT WINAPI ddraw_surface3_AddAttachedSurface(IDirectDrawSurface3 *iface, IDirectDrawSurface3 *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface3(attachment);
    HRESULT hr;

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    /* Tests suggest that
     * -> offscreen plain surfaces can be attached to other offscreen plain surfaces
     * -> offscreen plain surfaces can be attached to primaries
     * -> primaries can be attached to offscreen plain surfaces
     * -> z buffers can be attached to primaries */
    if (This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OFFSCREENPLAIN)
            && attachment_impl->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OFFSCREENPLAIN))
    {
        /* Sizes have to match */
        if (attachment_impl->surface_desc.dwWidth != This->surface_desc.dwWidth
                || attachment_impl->surface_desc.dwHeight != This->surface_desc.dwHeight)
        {
            WARN("Surface sizes do not match.\n");
            return DDERR_CANNOTATTACHSURFACE;
        }
        /* OK */
    }
    else if (This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE)
            && attachment_impl->surface_desc.ddsCaps.dwCaps & (DDSCAPS_ZBUFFER))
    {
        /* OK */
    }
    else
    {
        WARN("Invalid attachment combination.\n");
        return DDERR_CANNOTATTACHSURFACE;
    }

    hr = ddraw_surface_attach_surface(This, attachment_impl);
    if (FAILED(hr))
    {
        return hr;
    }
    ddraw_surface3_AddRef(attachment);
    attachment_impl->attached_iface = (IUnknown *)attachment;
    return hr;
}

static HRESULT WINAPI ddraw_surface2_AddAttachedSurface(IDirectDrawSurface2 *iface, IDirectDrawSurface2 *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface2(attachment);
    HRESULT hr;

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    hr = ddraw_surface3_AddAttachedSurface(&This->IDirectDrawSurface3_iface,
            attachment_impl ? &attachment_impl->IDirectDrawSurface3_iface : NULL);
    if (FAILED(hr))
    {
        return hr;
    }
    ddraw_surface2_AddRef(attachment);
    ddraw_surface3_Release(&attachment_impl->IDirectDrawSurface3_iface);
    attachment_impl->attached_iface = (IUnknown *)attachment;
    return hr;
}

static HRESULT WINAPI ddraw_surface1_AddAttachedSurface(IDirectDrawSurface *iface, IDirectDrawSurface *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface(attachment);
    HRESULT hr;

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    hr = ddraw_surface3_AddAttachedSurface(&This->IDirectDrawSurface3_iface,
            attachment_impl ? &attachment_impl->IDirectDrawSurface3_iface : NULL);
    if (FAILED(hr))
    {
        return hr;
    }
    ddraw_surface1_AddRef(attachment);
    ddraw_surface3_Release(&attachment_impl->IDirectDrawSurface3_iface);
    attachment_impl->attached_iface = (IUnknown *)attachment;
    return hr;
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
static HRESULT ddraw_surface_delete_attached_surface(IDirectDrawSurfaceImpl *This,
        IDirectDrawSurfaceImpl *Surf, IUnknown *detach_iface)
{
    IDirectDrawSurfaceImpl *Prev = This;

    TRACE("surface %p, attachment %p, detach_iface %p.\n", This, Surf, detach_iface);

    wined3d_mutex_lock();
    if (!Surf || (Surf->first_attached != This) || (Surf == This) )
    {
        wined3d_mutex_unlock();
        return DDERR_CANNOTDETACHSURFACE;
    }

    if (Surf->attached_iface != detach_iface)
    {
        WARN("Surf->attach_iface %p != detach_iface %p.\n", Surf->attached_iface, detach_iface);
        wined3d_mutex_unlock();
        return DDERR_SURFACENOTATTACHED;
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
    wined3d_mutex_unlock();

    /* Set attached_iface to NULL before releasing it, the surface may go
     * away. */
    Surf->attached_iface = NULL;
    IUnknown_Release(detach_iface);

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface7_DeleteAttachedSurface(IDirectDrawSurface7 *iface,
        DWORD flags, IDirectDrawSurface7 *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface7(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(This, attachment_impl, (IUnknown *)attachment);
}

static HRESULT WINAPI ddraw_surface4_DeleteAttachedSurface(IDirectDrawSurface4 *iface,
        DWORD flags, IDirectDrawSurface4 *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface4(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(This, attachment_impl, (IUnknown *)attachment);
}

static HRESULT WINAPI ddraw_surface3_DeleteAttachedSurface(IDirectDrawSurface3 *iface,
        DWORD flags, IDirectDrawSurface3 *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface3(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(This, attachment_impl, (IUnknown *)attachment);
}

static HRESULT WINAPI ddraw_surface2_DeleteAttachedSurface(IDirectDrawSurface2 *iface,
        DWORD flags, IDirectDrawSurface2 *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface2(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(This, attachment_impl, (IUnknown *)attachment);
}

static HRESULT WINAPI ddraw_surface1_DeleteAttachedSurface(IDirectDrawSurface *iface,
        DWORD flags, IDirectDrawSurface *attachment)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    IDirectDrawSurfaceImpl *attachment_impl = unsafe_impl_from_IDirectDrawSurface(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(This, attachment_impl, (IUnknown *)attachment);
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
static HRESULT WINAPI ddraw_surface7_AddOverlayDirtyRect(IDirectDrawSurface7 *iface, RECT *Rect)
{
    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(Rect));

    return DDERR_UNSUPPORTED; /* unchecked */
}

static HRESULT WINAPI ddraw_surface4_AddOverlayDirtyRect(IDirectDrawSurface4 *iface, RECT *rect)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return ddraw_surface7_AddOverlayDirtyRect(&This->IDirectDrawSurface7_iface, rect);
}

static HRESULT WINAPI ddraw_surface3_AddOverlayDirtyRect(IDirectDrawSurface3 *iface, RECT *rect)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return ddraw_surface7_AddOverlayDirtyRect(&This->IDirectDrawSurface7_iface, rect);
}

static HRESULT WINAPI ddraw_surface2_AddOverlayDirtyRect(IDirectDrawSurface2 *iface, RECT *rect)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return ddraw_surface7_AddOverlayDirtyRect(&This->IDirectDrawSurface7_iface, rect);
}

static HRESULT WINAPI ddraw_surface1_AddOverlayDirtyRect(IDirectDrawSurface *iface, RECT *rect)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return ddraw_surface7_AddOverlayDirtyRect(&This->IDirectDrawSurface7_iface, rect);
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
static HRESULT WINAPI ddraw_surface7_GetDC(IDirectDrawSurface7 *iface, HDC *hdc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr = DD_OK;

    TRACE("iface %p, dc %p.\n", iface, hdc);

    if(!hdc)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        hr = ddraw_surface_update_frontbuffer(This, NULL, TRUE);
    if (SUCCEEDED(hr))
        hr = wined3d_surface_getdc(This->wined3d_surface, hdc);
    wined3d_mutex_unlock();
    switch(hr)
    {
        /* Some, but not all errors set *hdc to NULL. E.g. DCALREADYCREATED does not
         * touch *hdc
         */
        case WINED3DERR_INVALIDCALL:
            if(hdc) *hdc = NULL;
            return DDERR_INVALIDPARAMS;

        default: return hr;
    }
}

static HRESULT WINAPI ddraw_surface4_GetDC(IDirectDrawSurface4 *iface, HDC *dc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_GetDC(&This->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface3_GetDC(IDirectDrawSurface3 *iface, HDC *dc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_GetDC(&This->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface2_GetDC(IDirectDrawSurface2 *iface, HDC *dc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_GetDC(&This->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface1_GetDC(IDirectDrawSurface *iface, HDC *dc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_GetDC(&This->IDirectDrawSurface7_iface, dc);
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
static HRESULT WINAPI ddraw_surface7_ReleaseDC(IDirectDrawSurface7 *iface, HDC hdc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, dc %p.\n", iface, hdc);

    wined3d_mutex_lock();
    hr = wined3d_surface_releasedc(This->wined3d_surface, hdc);
    if (SUCCEEDED(hr) && (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER))
        hr = ddraw_surface_update_frontbuffer(This, NULL, FALSE);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_ReleaseDC(IDirectDrawSurface4 *iface, HDC dc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_ReleaseDC(&This->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface3_ReleaseDC(IDirectDrawSurface3 *iface, HDC dc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_ReleaseDC(&This->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface2_ReleaseDC(IDirectDrawSurface2 *iface, HDC dc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_ReleaseDC(&This->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface1_ReleaseDC(IDirectDrawSurface *iface, HDC dc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_ReleaseDC(&This->IDirectDrawSurface7_iface, dc);
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
static HRESULT WINAPI ddraw_surface7_GetCaps(IDirectDrawSurface7 *iface, DDSCAPS2 *Caps)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, caps %p.\n", iface, Caps);

    if(!Caps)
        return DDERR_INVALIDPARAMS;

    *Caps = This->surface_desc.ddsCaps;
    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetCaps(IDirectDrawSurface4 *iface, DDSCAPS2 *caps)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, caps %p.\n", iface, caps);

    return ddraw_surface7_GetCaps(&This->IDirectDrawSurface7_iface, caps);
}

static HRESULT WINAPI ddraw_surface3_GetCaps(IDirectDrawSurface3 *iface, DDSCAPS *caps)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, caps);

    hr = ddraw_surface7_GetCaps(&This->IDirectDrawSurface7_iface, &caps2);
    if (FAILED(hr)) return hr;

    caps->dwCaps = caps2.dwCaps;
    return hr;
}

static HRESULT WINAPI ddraw_surface2_GetCaps(IDirectDrawSurface2 *iface, DDSCAPS *caps)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, caps);

    hr = ddraw_surface7_GetCaps(&This->IDirectDrawSurface7_iface, &caps2);
    if (FAILED(hr)) return hr;

    caps->dwCaps = caps2.dwCaps;
    return hr;
}

static HRESULT WINAPI ddraw_surface1_GetCaps(IDirectDrawSurface *iface, DDSCAPS *caps)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, caps);

    hr = ddraw_surface7_GetCaps(&This->IDirectDrawSurface7_iface, &caps2);
    if (FAILED(hr)) return hr;

    caps->dwCaps = caps2.dwCaps;
    return hr;
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
static HRESULT WINAPI ddraw_surface7_SetPriority(IDirectDrawSurface7 *iface, DWORD Priority)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, priority %u.\n", iface, Priority);

    wined3d_mutex_lock();
    hr = wined3d_surface_set_priority(This->wined3d_surface, Priority);
    wined3d_mutex_unlock();

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
static HRESULT WINAPI ddraw_surface7_GetPriority(IDirectDrawSurface7 *iface, DWORD *Priority)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, priority %p.\n", iface, Priority);

    if(!Priority)
    {
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    *Priority = wined3d_surface_get_priority(This->wined3d_surface);
    wined3d_mutex_unlock();

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
static HRESULT WINAPI ddraw_surface7_SetPrivateData(IDirectDrawSurface7 *iface,
        REFGUID tag, void *Data, DWORD Size, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, tag %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(tag), Data, Size, Flags);

    wined3d_mutex_lock();
    resource = wined3d_surface_get_resource(This->wined3d_surface);
    hr = wined3d_resource_set_private_data(resource, tag, Data, Size, Flags);
    wined3d_mutex_unlock();

    switch(hr)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return hr;
    }
}

static HRESULT WINAPI ddraw_surface4_SetPrivateData(IDirectDrawSurface4 *iface,
        REFGUID tag, void *data, DWORD size, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, tag %s, data %p, data_size %u, flags %#x.\n",
                iface, debugstr_guid(tag), data, size, flags);

    return ddraw_surface7_SetPrivateData(&This->IDirectDrawSurface7_iface, tag, data, size, flags);
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
static HRESULT WINAPI ddraw_surface7_GetPrivateData(IDirectDrawSurface7 *iface, REFGUID tag, void *Data, DWORD *Size)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, tag %s, data %p, data_size %p.\n",
            iface, debugstr_guid(tag), Data, Size);

    if(!Data)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    resource = wined3d_surface_get_resource(This->wined3d_surface);
    hr = wined3d_resource_get_private_data(resource, tag, Data, Size);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_GetPrivateData(IDirectDrawSurface4 *iface, REFGUID tag, void *data, DWORD *size)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, tag %s, data %p, data_size %p.\n",
                iface, debugstr_guid(tag), data, size);

    return ddraw_surface7_GetPrivateData(&This->IDirectDrawSurface7_iface, tag, data, size);
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
static HRESULT WINAPI ddraw_surface7_FreePrivateData(IDirectDrawSurface7 *iface, REFGUID tag)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, tag %s.\n", iface, debugstr_guid(tag));

    wined3d_mutex_lock();
    resource = wined3d_surface_get_resource(This->wined3d_surface);
    hr = wined3d_resource_free_private_data(resource, tag);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_FreePrivateData(IDirectDrawSurface4 *iface, REFGUID tag)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, tag %s.\n", iface, debugstr_guid(tag));

    return ddraw_surface7_FreePrivateData(&This->IDirectDrawSurface7_iface, tag);
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
static HRESULT WINAPI ddraw_surface7_PageLock(IDirectDrawSurface7 *iface, DWORD Flags)
{
    TRACE("iface %p, flags %#x.\n", iface, Flags);

    /* This is Windows memory management related - we don't need this */
    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_PageLock(IDirectDrawSurface4 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageLock(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_PageLock(IDirectDrawSurface3 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageLock(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_PageLock(IDirectDrawSurface2 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageLock(&This->IDirectDrawSurface7_iface, flags);
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
static HRESULT WINAPI ddraw_surface7_PageUnlock(IDirectDrawSurface7 *iface, DWORD Flags)
{
    TRACE("iface %p, flags %#x.\n", iface, Flags);

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_PageUnlock(IDirectDrawSurface4 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageUnlock(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_PageUnlock(IDirectDrawSurface3 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageUnlock(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_PageUnlock(IDirectDrawSurface2 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageUnlock(&This->IDirectDrawSurface7_iface, flags);
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
static HRESULT WINAPI ddraw_surface7_BltBatch(IDirectDrawSurface7 *iface, DDBLTBATCH *Batch, DWORD Count, DWORD Flags)
{
    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, Batch, Count, Flags);

    /* MSDN: "not currently implemented" */
    return DDERR_UNSUPPORTED;
}

static HRESULT WINAPI ddraw_surface4_BltBatch(IDirectDrawSurface4 *iface, DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, batch, count, flags);

    return ddraw_surface7_BltBatch(&This->IDirectDrawSurface7_iface, batch, count, flags);
}

static HRESULT WINAPI ddraw_surface3_BltBatch(IDirectDrawSurface3 *iface, DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, batch, count, flags);

    return ddraw_surface7_BltBatch(&This->IDirectDrawSurface7_iface, batch, count, flags);
}

static HRESULT WINAPI ddraw_surface2_BltBatch(IDirectDrawSurface2 *iface, DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, batch, count, flags);

    return ddraw_surface7_BltBatch(&This->IDirectDrawSurface7_iface, batch, count, flags);
}

static HRESULT WINAPI ddraw_surface1_BltBatch(IDirectDrawSurface *iface, DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, batch, count, flags);

    return ddraw_surface7_BltBatch(&This->IDirectDrawSurface7_iface, batch, count, flags);
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
static HRESULT WINAPI ddraw_surface7_EnumAttachedSurfaces(IDirectDrawSurface7 *iface,
        void *context, LPDDENUMSURFACESCALLBACK7 cb)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawSurfaceImpl *surf;
    DDSURFACEDESC2 desc;
    int i;

    /* Attached surfaces aren't handled in WineD3D */
    TRACE("iface %p, context %p, callback %p.\n", iface, context, cb);

    if(!cb)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    for(i = 0; i < MAX_COMPLEX_ATTACHED; i++)
    {
        surf = This->complex_array[i];
        if(!surf) break;

        ddraw_surface7_AddRef(&surf->IDirectDrawSurface7_iface);
        desc = surf->surface_desc;
        /* check: != DDENUMRET_OK or == DDENUMRET_CANCEL? */
        if (cb(&surf->IDirectDrawSurface7_iface, &desc, context) == DDENUMRET_CANCEL)
        {
            wined3d_mutex_unlock();
            return DD_OK;
        }
    }

    for (surf = This->next_attached; surf != NULL; surf = surf->next_attached)
    {
        ddraw_surface7_AddRef(&surf->IDirectDrawSurface7_iface);
        desc = surf->surface_desc;
        /* check: != DDENUMRET_OK or == DDENUMRET_CANCEL? */
        if (cb(&surf->IDirectDrawSurface7_iface, &desc, context) == DDENUMRET_CANCEL)
        {
            wined3d_mutex_unlock();
            return DD_OK;
        }
    }

    TRACE(" end of enumeration.\n");

    wined3d_mutex_unlock();

    return DD_OK;
}

struct callback_info2
{
    LPDDENUMSURFACESCALLBACK2 callback;
    void *context;
};

struct callback_info
{
    LPDDENUMSURFACESCALLBACK callback;
    void *context;
};

static HRESULT CALLBACK EnumCallback2(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *surface_desc, void *context)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(surface);
    const struct callback_info2 *info = context;

    ddraw_surface4_AddRef(&This->IDirectDrawSurface4_iface);
    ddraw_surface7_Release(surface);

    return info->callback(&This->IDirectDrawSurface4_iface, surface_desc, info->context);
}

static HRESULT CALLBACK EnumCallback(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *surface_desc, void *context)
{
    IDirectDrawSurfaceImpl *surface_impl = impl_from_IDirectDrawSurface7(surface);
    const struct callback_info *info = context;

    ddraw_surface1_AddRef(&surface_impl->IDirectDrawSurface_iface);
    ddraw_surface7_Release(surface);

    /* FIXME: Check surface_test.dwSize */
    return info->callback(&surface_impl->IDirectDrawSurface_iface,
            (DDSURFACEDESC *)surface_desc, info->context);
}

static HRESULT WINAPI ddraw_surface4_EnumAttachedSurfaces(IDirectDrawSurface4 *iface,
        void *context, LPDDENUMSURFACESCALLBACK2 callback)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    struct callback_info2 info;

    TRACE("iface %p, context %p, callback %p.\n", iface, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumAttachedSurfaces(&This->IDirectDrawSurface7_iface,
            &info, EnumCallback2);
}

static HRESULT WINAPI ddraw_surface3_EnumAttachedSurfaces(IDirectDrawSurface3 *iface,
        void *context, LPDDENUMSURFACESCALLBACK callback)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    struct callback_info info;

    TRACE("iface %p, context %p, callback %p.\n", iface, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumAttachedSurfaces(&This->IDirectDrawSurface7_iface,
            &info, EnumCallback);
}

static HRESULT WINAPI ddraw_surface2_EnumAttachedSurfaces(IDirectDrawSurface2 *iface,
        void *context, LPDDENUMSURFACESCALLBACK callback)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    struct callback_info info;

    TRACE("iface %p, context %p, callback %p.\n", iface, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumAttachedSurfaces(&This->IDirectDrawSurface7_iface,
            &info, EnumCallback);
}

static HRESULT WINAPI ddraw_surface1_EnumAttachedSurfaces(IDirectDrawSurface *iface,
        void *context, LPDDENUMSURFACESCALLBACK callback)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    struct callback_info info;

    TRACE("iface %p, context %p, callback %p.\n", iface, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumAttachedSurfaces(&This->IDirectDrawSurface7_iface,
            &info, EnumCallback);
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
static HRESULT WINAPI ddraw_surface7_EnumOverlayZOrders(IDirectDrawSurface7 *iface,
        DWORD Flags, void *context, LPDDENUMSURFACESCALLBACK7 cb)
{
    FIXME("iface %p, flags %#x, context %p, callback %p stub!\n", iface, Flags, context, cb);

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_EnumOverlayZOrders(IDirectDrawSurface4 *iface,
        DWORD flags, void *context, LPDDENUMSURFACESCALLBACK2 callback)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    struct callback_info2 info;

    TRACE("iface %p, flags %#x, context %p, callback %p.\n", iface, flags, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumOverlayZOrders(&This->IDirectDrawSurface7_iface,
            flags, &info, EnumCallback2);
}

static HRESULT WINAPI ddraw_surface3_EnumOverlayZOrders(IDirectDrawSurface3 *iface,
        DWORD flags, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    struct callback_info info;

    TRACE("iface %p, flags %#x, context %p, callback %p.\n", iface, flags, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumOverlayZOrders(&This->IDirectDrawSurface7_iface,
            flags, &info, EnumCallback);
}

static HRESULT WINAPI ddraw_surface2_EnumOverlayZOrders(IDirectDrawSurface2 *iface,
        DWORD flags, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    struct callback_info info;

    TRACE("iface %p, flags %#x, context %p, callback %p.\n", iface, flags, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumOverlayZOrders(&This->IDirectDrawSurface7_iface,
            flags, &info, EnumCallback);
}

static HRESULT WINAPI ddraw_surface1_EnumOverlayZOrders(IDirectDrawSurface *iface,
        DWORD flags, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    struct callback_info info;

    TRACE("iface %p, flags %#x, context %p, callback %p.\n", iface, flags, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumOverlayZOrders(&This->IDirectDrawSurface7_iface,
            flags, &info, EnumCallback);
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
static HRESULT WINAPI ddraw_surface7_GetBltStatus(IDirectDrawSurface7 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x.\n", iface, Flags);

    wined3d_mutex_lock();
    hr = wined3d_surface_get_blt_status(This->wined3d_surface, Flags);
    wined3d_mutex_unlock();
    switch(hr)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return hr;
    }
}

static HRESULT WINAPI ddraw_surface4_GetBltStatus(IDirectDrawSurface4 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetBltStatus(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_GetBltStatus(IDirectDrawSurface3 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetBltStatus(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_GetBltStatus(IDirectDrawSurface2 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetBltStatus(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface1_GetBltStatus(IDirectDrawSurface *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetBltStatus(&This->IDirectDrawSurface7_iface, flags);
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
static HRESULT WINAPI ddraw_surface7_GetColorKey(IDirectDrawSurface7 *iface, DWORD Flags, DDCOLORKEY *CKey)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, Flags, CKey);

    if(!CKey)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    switch (Flags)
    {
    case DDCKEY_DESTBLT:
        if (!(This->surface_desc.dwFlags & DDSD_CKDESTBLT))
        {
            wined3d_mutex_unlock();
            return DDERR_NOCOLORKEY;
        }
        *CKey = This->surface_desc.ddckCKDestBlt;
        break;

    case DDCKEY_DESTOVERLAY:
        if (!(This->surface_desc.dwFlags & DDSD_CKDESTOVERLAY))
        {
            wined3d_mutex_unlock();
            return DDERR_NOCOLORKEY;
        }
        *CKey = This->surface_desc.u3.ddckCKDestOverlay;
        break;

    case DDCKEY_SRCBLT:
        if (!(This->surface_desc.dwFlags & DDSD_CKSRCBLT))
        {
            wined3d_mutex_unlock();
            return DDERR_NOCOLORKEY;
        }
        *CKey = This->surface_desc.ddckCKSrcBlt;
        break;

    case DDCKEY_SRCOVERLAY:
        if (!(This->surface_desc.dwFlags & DDSD_CKSRCOVERLAY))
        {
            wined3d_mutex_unlock();
            return DDERR_NOCOLORKEY;
        }
        *CKey = This->surface_desc.ddckCKSrcOverlay;
        break;

    default:
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetColorKey(IDirectDrawSurface4 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_GetColorKey(&This->IDirectDrawSurface7_iface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface3_GetColorKey(IDirectDrawSurface3 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_GetColorKey(&This->IDirectDrawSurface7_iface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface2_GetColorKey(IDirectDrawSurface2 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_GetColorKey(&This->IDirectDrawSurface7_iface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface1_GetColorKey(IDirectDrawSurface *iface, DWORD flags, DDCOLORKEY *color_key)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_GetColorKey(&This->IDirectDrawSurface7_iface, flags, color_key);
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
static HRESULT WINAPI ddraw_surface7_GetFlipStatus(IDirectDrawSurface7 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x.\n", iface, Flags);

    wined3d_mutex_lock();
    hr = wined3d_surface_get_flip_status(This->wined3d_surface, Flags);
    wined3d_mutex_unlock();

    switch(hr)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return hr;
    }
}

static HRESULT WINAPI ddraw_surface4_GetFlipStatus(IDirectDrawSurface4 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetFlipStatus(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_GetFlipStatus(IDirectDrawSurface3 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetFlipStatus(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_GetFlipStatus(IDirectDrawSurface2 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetFlipStatus(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface1_GetFlipStatus(IDirectDrawSurface *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetFlipStatus(&This->IDirectDrawSurface7_iface, flags);
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
static HRESULT WINAPI ddraw_surface7_GetOverlayPosition(IDirectDrawSurface7 *iface, LONG *X, LONG *Y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, x %p, y %p.\n", iface, X, Y);

    wined3d_mutex_lock();
    hr = wined3d_surface_get_overlay_position(This->wined3d_surface, X, Y);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_GetOverlayPosition(IDirectDrawSurface4 *iface, LONG *x, LONG *y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return ddraw_surface7_GetOverlayPosition(&This->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface3_GetOverlayPosition(IDirectDrawSurface3 *iface, LONG *x, LONG *y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return ddraw_surface7_GetOverlayPosition(&This->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface2_GetOverlayPosition(IDirectDrawSurface2 *iface, LONG *x, LONG *y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return ddraw_surface7_GetOverlayPosition(&This->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface1_GetOverlayPosition(IDirectDrawSurface *iface, LONG *x, LONG *y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return ddraw_surface7_GetOverlayPosition(&This->IDirectDrawSurface7_iface, x, y);
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
static HRESULT WINAPI ddraw_surface7_GetPixelFormat(IDirectDrawSurface7 *iface, DDPIXELFORMAT *PixelFormat)
{
    /* What is DDERR_INVALIDSURFACETYPE for here? */
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, pixel_format %p.\n", iface, PixelFormat);

    if(!PixelFormat)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    DD_STRUCT_COPY_BYSIZE(PixelFormat,&This->surface_desc.u4.ddpfPixelFormat);
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetPixelFormat(IDirectDrawSurface4 *iface, DDPIXELFORMAT *pixel_format)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return ddraw_surface7_GetPixelFormat(&This->IDirectDrawSurface7_iface, pixel_format);
}

static HRESULT WINAPI ddraw_surface3_GetPixelFormat(IDirectDrawSurface3 *iface, DDPIXELFORMAT *pixel_format)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return ddraw_surface7_GetPixelFormat(&This->IDirectDrawSurface7_iface, pixel_format);
}

static HRESULT WINAPI ddraw_surface2_GetPixelFormat(IDirectDrawSurface2 *iface, DDPIXELFORMAT *pixel_format)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return ddraw_surface7_GetPixelFormat(&This->IDirectDrawSurface7_iface, pixel_format);
}

static HRESULT WINAPI ddraw_surface1_GetPixelFormat(IDirectDrawSurface *iface, DDPIXELFORMAT *pixel_format)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return ddraw_surface7_GetPixelFormat(&This->IDirectDrawSurface7_iface, pixel_format);
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
static HRESULT WINAPI ddraw_surface7_GetSurfaceDesc(IDirectDrawSurface7 *iface, DDSURFACEDESC2 *DDSD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    if(!DDSD)
        return DDERR_INVALIDPARAMS;

    if (DDSD->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Incorrect struct size %d, returning DDERR_INVALIDPARAMS\n",DDSD->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    DD_STRUCT_COPY_BYSIZE(DDSD,&This->surface_desc);
    TRACE("Returning surface desc:\n");
    if (TRACE_ON(ddraw)) DDRAW_dump_surface_desc(DDSD);
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetSurfaceDesc(IDirectDrawSurface4 *iface, DDSURFACEDESC2 *DDSD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    return ddraw_surface7_GetSurfaceDesc(&This->IDirectDrawSurface7_iface, DDSD);
}

static HRESULT WINAPI ddraw_surface3_GetSurfaceDesc(IDirectDrawSurface3 *iface, DDSURFACEDESC *surface_desc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, surface_desc);

    if (!surface_desc) return DDERR_INVALIDPARAMS;

    if (surface_desc->dwSize != sizeof(DDSURFACEDESC))
    {
        WARN("Incorrect structure size %u, returning DDERR_INVALIDPARAMS.\n", surface_desc->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    DDSD2_to_DDSD(&This->surface_desc, surface_desc);
    TRACE("Returning surface desc:\n");
    if (TRACE_ON(ddraw))
    {
        /* DDRAW_dump_surface_desc handles the smaller size */
        DDRAW_dump_surface_desc((DDSURFACEDESC2 *)surface_desc);
    }
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface2_GetSurfaceDesc(IDirectDrawSurface2 *iface, DDSURFACEDESC *DDSD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    return ddraw_surface3_GetSurfaceDesc(&This->IDirectDrawSurface3_iface, DDSD);
}

static HRESULT WINAPI ddraw_surface1_GetSurfaceDesc(IDirectDrawSurface *iface, DDSURFACEDESC *DDSD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    return ddraw_surface3_GetSurfaceDesc(&This->IDirectDrawSurface3_iface, DDSD);
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
static HRESULT WINAPI ddraw_surface7_Initialize(IDirectDrawSurface7 *iface,
        IDirectDraw *ddraw, DDSURFACEDESC2 *surface_desc)
{
    TRACE("iface %p, ddraw %p, surface_desc %p.\n", iface, ddraw, surface_desc);

    return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI ddraw_surface4_Initialize(IDirectDrawSurface4 *iface,
        IDirectDraw *ddraw, DDSURFACEDESC2 *surface_desc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, ddraw %p, surface_desc %p.\n", iface, ddraw, surface_desc);

    return ddraw_surface7_Initialize(&This->IDirectDrawSurface7_iface,
            ddraw, surface_desc);
}

static HRESULT WINAPI ddraw_surface3_Initialize(IDirectDrawSurface3 *iface,
        IDirectDraw *ddraw, DDSURFACEDESC *surface_desc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 surface_desc2;
    TRACE("iface %p, ddraw %p, surface_desc %p.\n", iface, ddraw, surface_desc);

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw_surface7_Initialize(&This->IDirectDrawSurface7_iface,
            ddraw, surface_desc ? &surface_desc2 : NULL);
}

static HRESULT WINAPI ddraw_surface2_Initialize(IDirectDrawSurface2 *iface,
        IDirectDraw *ddraw, DDSURFACEDESC *surface_desc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    DDSURFACEDESC2 surface_desc2;
    TRACE("iface %p, ddraw %p, surface_desc %p.\n", iface, ddraw, surface_desc);

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw_surface7_Initialize(&This->IDirectDrawSurface7_iface,
            ddraw, surface_desc ? &surface_desc2 : NULL);
}

static HRESULT WINAPI ddraw_surface1_Initialize(IDirectDrawSurface *iface,
        IDirectDraw *ddraw, DDSURFACEDESC *surface_desc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    DDSURFACEDESC2 surface_desc2;
    TRACE("iface %p, ddraw %p, surface_desc %p.\n", iface, ddraw, surface_desc);

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw_surface7_Initialize(&This->IDirectDrawSurface7_iface,
            ddraw, surface_desc ? &surface_desc2 : NULL);
}

/*****************************************************************************
 * IDirect3DTexture1::Initialize
 *
 * The sdk says it's not implemented
 *
 * Params:
 *  ?
 *
 * Returns
 *  DDERR_UNSUPPORTED
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_texture1_Initialize(IDirect3DTexture *iface,
        IDirect3DDevice *device, IDirectDrawSurface *surface)
{
    TRACE("iface %p, device %p, surface %p.\n", iface, device, surface);

    return DDERR_UNSUPPORTED; /* Unchecked */
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
static HRESULT WINAPI ddraw_surface7_IsLost(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_surface_is_lost(This->wined3d_surface);
    wined3d_mutex_unlock();

    switch(hr)
    {
        /* D3D8 and 9 loose full devices, thus there's only a DEVICELOST error.
         * WineD3D uses the same error for surfaces
         */
        case WINED3DERR_DEVICELOST:         return DDERR_SURFACELOST;
        default:                            return hr;
    }
}

static HRESULT WINAPI ddraw_surface4_IsLost(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface7_IsLost(&This->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface3_IsLost(IDirectDrawSurface3 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface7_IsLost(&This->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface2_IsLost(IDirectDrawSurface2 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface7_IsLost(&This->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface1_IsLost(IDirectDrawSurface *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface7_IsLost(&This->IDirectDrawSurface7_iface);
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
static HRESULT WINAPI ddraw_surface7_Restore(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_surface_restore(This->wined3d_surface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_Restore(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface7_Restore(&This->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface3_Restore(IDirectDrawSurface3 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface7_Restore(&This->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface2_Restore(IDirectDrawSurface2 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface7_Restore(&This->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface1_Restore(IDirectDrawSurface *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface7_Restore(&This->IDirectDrawSurface7_iface);
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
static HRESULT WINAPI ddraw_surface7_SetOverlayPosition(IDirectDrawSurface7 *iface, LONG X, LONG Y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, x %d, y %d.\n", iface, X, Y);

    wined3d_mutex_lock();
    hr = wined3d_surface_set_overlay_position(This->wined3d_surface, X, Y);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_SetOverlayPosition(IDirectDrawSurface4 *iface, LONG x, LONG y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, x %d, y %d.\n", iface, x, y);

    return ddraw_surface7_SetOverlayPosition(&This->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface3_SetOverlayPosition(IDirectDrawSurface3 *iface, LONG x, LONG y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, x %d, y %d.\n", iface, x, y);

    return ddraw_surface7_SetOverlayPosition(&This->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface2_SetOverlayPosition(IDirectDrawSurface2 *iface, LONG x, LONG y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, x %d, y %d.\n", iface, x, y);

    return ddraw_surface7_SetOverlayPosition(&This->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface1_SetOverlayPosition(IDirectDrawSurface *iface, LONG x, LONG y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, x %d, y %d.\n", iface, x, y);

    return ddraw_surface7_SetOverlayPosition(&This->IDirectDrawSurface7_iface, x, y);
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
static HRESULT WINAPI ddraw_surface7_UpdateOverlay(IDirectDrawSurface7 *iface, RECT *SrcRect,
        IDirectDrawSurface7 *DstSurface, RECT *DstRect, DWORD Flags, DDOVERLAYFX *FX)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawSurfaceImpl *Dst = unsafe_impl_from_IDirectDrawSurface7(DstSurface);
    HRESULT hr;

    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(SrcRect), DstSurface, wine_dbgstr_rect(DstRect), Flags, FX);

    wined3d_mutex_lock();
    hr = wined3d_surface_update_overlay(This->wined3d_surface, SrcRect,
            Dst ? Dst->wined3d_surface : NULL, DstRect, Flags, (WINEDDOVERLAYFX *)FX);
    wined3d_mutex_unlock();

    switch(hr) {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        case WINEDDERR_NOTAOVERLAYSURFACE:  return DDERR_NOTAOVERLAYSURFACE;
        case WINEDDERR_OVERLAYNOTVISIBLE:   return DDERR_OVERLAYNOTVISIBLE;
        default:
            return hr;
    }
}

static HRESULT WINAPI ddraw_surface4_UpdateOverlay(IDirectDrawSurface4 *iface, RECT *src_rect,
        IDirectDrawSurface4 *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *dst_impl = unsafe_impl_from_IDirectDrawSurface4(dst_surface);
    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return ddraw_surface7_UpdateOverlay(&This->IDirectDrawSurface7_iface, src_rect,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, dst_rect, flags, fx);
}

static HRESULT WINAPI ddraw_surface3_UpdateOverlay(IDirectDrawSurface3 *iface, RECT *src_rect,
        IDirectDrawSurface3 *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *dst_impl = unsafe_impl_from_IDirectDrawSurface3(dst_surface);
    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return ddraw_surface7_UpdateOverlay(&This->IDirectDrawSurface7_iface, src_rect,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, dst_rect, flags, fx);
}

static HRESULT WINAPI ddraw_surface2_UpdateOverlay(IDirectDrawSurface2 *iface, RECT *src_rect,
        IDirectDrawSurface2 *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    IDirectDrawSurfaceImpl *dst_impl = unsafe_impl_from_IDirectDrawSurface2(dst_surface);
    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return ddraw_surface7_UpdateOverlay(&This->IDirectDrawSurface7_iface, src_rect,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, dst_rect, flags, fx);
}

static HRESULT WINAPI ddraw_surface1_UpdateOverlay(IDirectDrawSurface *iface, RECT *src_rect,
        IDirectDrawSurface *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    IDirectDrawSurfaceImpl *dst_impl = unsafe_impl_from_IDirectDrawSurface(dst_surface);
    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return ddraw_surface7_UpdateOverlay(&This->IDirectDrawSurface7_iface, src_rect,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, dst_rect, flags, fx);
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
static HRESULT WINAPI ddraw_surface7_UpdateOverlayDisplay(IDirectDrawSurface7 *iface, DWORD Flags)
{
    TRACE("iface %p, flags %#x.\n", iface, Flags);

    return DDERR_UNSUPPORTED;
}

static HRESULT WINAPI ddraw_surface4_UpdateOverlayDisplay(IDirectDrawSurface4 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_UpdateOverlayDisplay(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_UpdateOverlayDisplay(IDirectDrawSurface3 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_UpdateOverlayDisplay(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_UpdateOverlayDisplay(IDirectDrawSurface2 *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_UpdateOverlayDisplay(&This->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface1_UpdateOverlayDisplay(IDirectDrawSurface *iface, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_UpdateOverlayDisplay(&This->IDirectDrawSurface7_iface, flags);
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
static HRESULT WINAPI ddraw_surface7_UpdateOverlayZOrder(IDirectDrawSurface7 *iface,
        DWORD Flags, IDirectDrawSurface7 *DDSRef)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawSurfaceImpl *Ref = unsafe_impl_from_IDirectDrawSurface7(DDSRef);
    HRESULT hr;

    TRACE("iface %p, flags %#x, reference %p.\n", iface, Flags, DDSRef);

    wined3d_mutex_lock();
    hr = wined3d_surface_update_overlay_z_order(This->wined3d_surface,
            Flags, Ref ? Ref->wined3d_surface : NULL);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_UpdateOverlayZOrder(IDirectDrawSurface4 *iface,
        DWORD flags, IDirectDrawSurface4 *reference)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *reference_impl = unsafe_impl_from_IDirectDrawSurface4(reference);
    TRACE("iface %p, flags %#x, reference %p.\n", iface, flags, reference);

    return ddraw_surface7_UpdateOverlayZOrder(&This->IDirectDrawSurface7_iface, flags,
            reference_impl ? &reference_impl->IDirectDrawSurface7_iface : NULL);
}

static HRESULT WINAPI ddraw_surface3_UpdateOverlayZOrder(IDirectDrawSurface3 *iface,
        DWORD flags, IDirectDrawSurface3 *reference)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *reference_impl = unsafe_impl_from_IDirectDrawSurface3(reference);
    TRACE("iface %p, flags %#x, reference %p.\n", iface, flags, reference);

    return ddraw_surface7_UpdateOverlayZOrder(&This->IDirectDrawSurface7_iface, flags,
            reference_impl ? &reference_impl->IDirectDrawSurface7_iface : NULL);
}

static HRESULT WINAPI ddraw_surface2_UpdateOverlayZOrder(IDirectDrawSurface2 *iface,
        DWORD flags, IDirectDrawSurface2 *reference)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    IDirectDrawSurfaceImpl *reference_impl = unsafe_impl_from_IDirectDrawSurface2(reference);
    TRACE("iface %p, flags %#x, reference %p.\n", iface, flags, reference);

    return ddraw_surface7_UpdateOverlayZOrder(&This->IDirectDrawSurface7_iface, flags,
            reference_impl ? &reference_impl->IDirectDrawSurface7_iface : NULL);
}

static HRESULT WINAPI ddraw_surface1_UpdateOverlayZOrder(IDirectDrawSurface *iface,
        DWORD flags, IDirectDrawSurface *reference)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    IDirectDrawSurfaceImpl *reference_impl = unsafe_impl_from_IDirectDrawSurface(reference);
    TRACE("iface %p, flags %#x, reference %p.\n", iface, flags, reference);

    return ddraw_surface7_UpdateOverlayZOrder(&This->IDirectDrawSurface7_iface, flags,
            reference_impl ? &reference_impl->IDirectDrawSurface7_iface : NULL);
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
static HRESULT WINAPI ddraw_surface7_GetDDInterface(IDirectDrawSurface7 *iface, void **DD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, ddraw %p.\n", iface, DD);

    if(!DD)
        return DDERR_INVALIDPARAMS;

    switch(This->version)
    {
        case 7:
            *DD = &This->ddraw->IDirectDraw7_iface;
            break;

        case 4:
            *DD = &This->ddraw->IDirectDraw4_iface;
            break;

        case 2:
            *DD = &This->ddraw->IDirectDraw2_iface;
            break;

        case 1:
            *DD = &This->ddraw->IDirectDraw_iface;
            break;

    }
    IUnknown_AddRef((IUnknown *)*DD);

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetDDInterface(IDirectDrawSurface4 *iface, void **ddraw)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, ddraw %p.\n", iface, ddraw);

    return ddraw_surface7_GetDDInterface(&This->IDirectDrawSurface7_iface, ddraw);
}

static HRESULT WINAPI ddraw_surface3_GetDDInterface(IDirectDrawSurface3 *iface, void **ddraw)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, ddraw %p.\n", iface, ddraw);

    return ddraw_surface7_GetDDInterface(&This->IDirectDrawSurface7_iface, ddraw);
}

static HRESULT WINAPI ddraw_surface2_GetDDInterface(IDirectDrawSurface2 *iface, void **ddraw)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, ddraw %p.\n", iface, ddraw);

    return ddraw_surface7_GetDDInterface(&This->IDirectDrawSurface7_iface, ddraw);
}

/* This seems also windows implementation specific - I don't think WineD3D needs this */
static HRESULT WINAPI ddraw_surface7_ChangeUniquenessValue(IDirectDrawSurface7 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    volatile IDirectDrawSurfaceImpl* vThis = This;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
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

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_ChangeUniquenessValue(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p.\n", iface);

    return ddraw_surface7_ChangeUniquenessValue(&This->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface7_GetUniquenessValue(IDirectDrawSurface7 *iface, DWORD *pValue)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, value %p.\n", iface, pValue);

    wined3d_mutex_lock();
    *pValue = This->uniqueness_value;
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetUniquenessValue(IDirectDrawSurface4 *iface, DWORD *pValue)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, value %p.\n", iface, pValue);

    return ddraw_surface7_GetUniquenessValue(&This->IDirectDrawSurface7_iface, pValue);
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
static HRESULT WINAPI ddraw_surface7_SetLOD(IDirectDrawSurface7 *iface, DWORD MaxLOD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, lod %u.\n", iface, MaxLOD);

    wined3d_mutex_lock();
    if (!(This->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDOBJECT;
    }

    if (!This->wined3d_texture)
    {
        ERR("(%p) The DirectDraw texture has no WineD3DTexture!\n", This);
        wined3d_mutex_unlock();
        return DDERR_INVALIDOBJECT;
    }

    hr = wined3d_texture_set_lod(This->wined3d_texture, MaxLOD);
    wined3d_mutex_unlock();

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
static HRESULT WINAPI ddraw_surface7_GetLOD(IDirectDrawSurface7 *iface, DWORD *MaxLOD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, lod %p.\n", iface, MaxLOD);

    if(!MaxLOD)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if (!(This->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDOBJECT;
    }

    *MaxLOD = wined3d_texture_get_lod(This->wined3d_texture);
    wined3d_mutex_unlock();

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
static HRESULT WINAPI ddraw_surface7_BltFast(IDirectDrawSurface7 *iface, DWORD dstx, DWORD dsty,
        IDirectDrawSurface7 *Source, RECT *rsrc, DWORD trans)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawSurfaceImpl *src = unsafe_impl_from_IDirectDrawSurface7(Source);
    DWORD src_w, src_h, dst_w, dst_h;
    HRESULT hr = DD_OK;
    DWORD flags = 0;
    RECT dst_rect;

    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dstx, dsty, Source, wine_dbgstr_rect(rsrc), trans);

    dst_w = This->surface_desc.dwWidth;
    dst_h = This->surface_desc.dwHeight;

    /* Source must be != NULL, This is not checked by windows. Windows happily throws a 0xc0000005
     * in that case
     */
    if(rsrc)
    {
        src_w = rsrc->right - rsrc->left;
        src_h = rsrc->bottom - rsrc->top;
    }
    else
    {
        src_w = src->surface_desc.dwWidth;
        src_h = src->surface_desc.dwHeight;
    }

    if (src_w > dst_w || dstx > dst_w - src_w
            || src_h > dst_h || dsty > dst_h - src_h)
    {
        WARN("Destination area out of bounds, returning DDERR_INVALIDRECT.\n");
        return DDERR_INVALIDRECT;
    }

    SetRect(&dst_rect, dstx, dsty, dstx + src_w, dsty + src_h);
    if (trans & DDBLTFAST_SRCCOLORKEY)
        flags |= WINEDDBLT_KEYSRC;
    if (trans & DDBLTFAST_DESTCOLORKEY)
        flags |= WINEDDBLT_KEYDEST;
    if (trans & DDBLTFAST_WAIT)
        flags |= WINEDDBLT_WAIT;
    if (trans & DDBLTFAST_DONOTWAIT)
        flags |= WINEDDBLT_DONOTWAIT;

    wined3d_mutex_lock();
    if (This->clipper)
    {
        wined3d_mutex_unlock();
        WARN("Destination surface has a clipper set, returning DDERR_BLTFASTCANTCLIP.\n");
        return DDERR_BLTFASTCANTCLIP;
    }

    if (src->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        hr = ddraw_surface_update_frontbuffer(src, rsrc, TRUE);
    if (SUCCEEDED(hr))
        hr = wined3d_surface_blt(This->wined3d_surface, &dst_rect,
                src->wined3d_surface, rsrc, flags, NULL, WINED3D_TEXF_POINT);
    if (SUCCEEDED(hr) && (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER))
        hr = ddraw_surface_update_frontbuffer(This, &dst_rect, FALSE);
    wined3d_mutex_unlock();

    switch(hr)
    {
        case WINED3DERR_NOTAVAILABLE:           return DDERR_UNSUPPORTED;
        case WINED3DERR_WRONGTEXTUREFORMAT:     return DDERR_INVALIDPIXELFORMAT;
        default:                                return hr;
    }
}

static HRESULT WINAPI ddraw_surface4_BltFast(IDirectDrawSurface4 *iface, DWORD dst_x, DWORD dst_y,
        IDirectDrawSurface4 *src_surface, RECT *src_rect, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *src_impl = unsafe_impl_from_IDirectDrawSurface4(src_surface);
    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return ddraw_surface7_BltFast(&This->IDirectDrawSurface7_iface, dst_x, dst_y,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags);
}

static HRESULT WINAPI ddraw_surface3_BltFast(IDirectDrawSurface3 *iface, DWORD dst_x, DWORD dst_y,
        IDirectDrawSurface3 *src_surface, RECT *src_rect, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *src_impl = unsafe_impl_from_IDirectDrawSurface3(src_surface);
    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return ddraw_surface7_BltFast(&This->IDirectDrawSurface7_iface, dst_x, dst_y,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags);
}

static HRESULT WINAPI ddraw_surface2_BltFast(IDirectDrawSurface2 *iface, DWORD dst_x, DWORD dst_y,
        IDirectDrawSurface2 *src_surface, RECT *src_rect, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    IDirectDrawSurfaceImpl *src_impl = unsafe_impl_from_IDirectDrawSurface2(src_surface);
    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return ddraw_surface7_BltFast(&This->IDirectDrawSurface7_iface, dst_x, dst_y,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags);
}

static HRESULT WINAPI ddraw_surface1_BltFast(IDirectDrawSurface *iface, DWORD dst_x, DWORD dst_y,
        IDirectDrawSurface *src_surface, RECT *src_rect, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    IDirectDrawSurfaceImpl *src_impl = unsafe_impl_from_IDirectDrawSurface(src_surface);
    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return ddraw_surface7_BltFast(&This->IDirectDrawSurface7_iface, dst_x, dst_y,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags);
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
static HRESULT WINAPI ddraw_surface7_GetClipper(IDirectDrawSurface7 *iface, IDirectDrawClipper **Clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, clipper %p.\n", iface, Clipper);

    if (!Clipper)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if(This->clipper == NULL)
    {
        wined3d_mutex_unlock();
        return DDERR_NOCLIPPERATTACHED;
    }

    *Clipper = (IDirectDrawClipper *)This->clipper;
    IDirectDrawClipper_AddRef(*Clipper);
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetClipper(IDirectDrawSurface4 *iface, IDirectDrawClipper **clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_GetClipper(&This->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface3_GetClipper(IDirectDrawSurface3 *iface, IDirectDrawClipper **clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_GetClipper(&This->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface2_GetClipper(IDirectDrawSurface2 *iface, IDirectDrawClipper **clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_GetClipper(&This->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface1_GetClipper(IDirectDrawSurface *iface, IDirectDrawClipper **clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_GetClipper(&This->IDirectDrawSurface7_iface, clipper);
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
static HRESULT WINAPI ddraw_surface7_SetClipper(IDirectDrawSurface7 *iface,
        IDirectDrawClipper *iclipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_clipper *clipper = unsafe_impl_from_IDirectDrawClipper(iclipper);
    struct ddraw_clipper *old_clipper = This->clipper;
    HWND clipWindow;

    TRACE("iface %p, clipper %p.\n", iface, iclipper);

    wined3d_mutex_lock();
    if (clipper == This->clipper)
    {
        wined3d_mutex_unlock();
        return DD_OK;
    }

    This->clipper = clipper;

    if (clipper != NULL)
        IDirectDrawClipper_AddRef(iclipper);
    if (old_clipper)
        IDirectDrawClipper_Release(&old_clipper->IDirectDrawClipper_iface);

    if ((This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) && This->ddraw->wined3d_swapchain)
    {
        clipWindow = NULL;
        if(clipper) {
            IDirectDrawClipper_GetHWnd(iclipper, &clipWindow);
        }

        if (clipWindow)
        {
            wined3d_swapchain_set_window(This->ddraw->wined3d_swapchain, clipWindow);
            ddraw_set_swapchain_window(This->ddraw, clipWindow);
        }
        else
        {
            wined3d_swapchain_set_window(This->ddraw->wined3d_swapchain, This->ddraw->d3d_window);
            ddraw_set_swapchain_window(This->ddraw, This->ddraw->dest_window);
        }
    }

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_SetClipper(IDirectDrawSurface4 *iface, IDirectDrawClipper *clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_SetClipper(&This->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface3_SetClipper(IDirectDrawSurface3 *iface, IDirectDrawClipper *clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_SetClipper(&This->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface2_SetClipper(IDirectDrawSurface2 *iface, IDirectDrawClipper *clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_SetClipper(&This->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface1_SetClipper(IDirectDrawSurface *iface, IDirectDrawClipper *clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_SetClipper(&This->IDirectDrawSurface7_iface, clipper);
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
static HRESULT WINAPI ddraw_surface7_SetSurfaceDesc(IDirectDrawSurface7 *iface, DDSURFACEDESC2 *DDSD, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;
    const DWORD allowed_flags = DDSD_LPSURFACE | DDSD_PIXELFORMAT | DDSD_WIDTH
            | DDSD_HEIGHT | DDSD_PITCH | DDSD_CAPS;
    enum wined3d_format_id format_id;
    BOOL update_wined3d = FALSE;
    UINT width, height;

    TRACE("iface %p, surface_desc %p, flags %#x.\n", iface, DDSD, Flags);

    if (!DDSD)
    {
        WARN("DDSD is NULL, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }
    if (Flags)
    {
        WARN("Flags is %x, returning DDERR_INVALIDPARAMS\n", Flags);
        return DDERR_INVALIDPARAMS;
    }
    if (!(This->surface_desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
    {
        WARN("Surface is not in system memory, returning DDERR_INVALIDSURFACETYPE.\n");
        return DDERR_INVALIDSURFACETYPE;
    }

    /* Tests show that only LPSURFACE and PIXELFORMAT can be set, and LPSURFACE is required
     * for PIXELFORMAT to work */
    if (DDSD->dwFlags & ~allowed_flags)
    {
        WARN("Invalid flags (0x%08x) set, returning DDERR_INVALIDPARAMS\n", DDSD->dwFlags);
        return DDERR_INVALIDPARAMS;
    }
    if (!(DDSD->dwFlags & DDSD_LPSURFACE))
    {
        WARN("DDSD_LPSURFACE is not set, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }
    if (DDSD->dwFlags & DDSD_CAPS)
    {
        WARN("DDSD_CAPS is set, returning DDERR_INVALIDCAPS.\n");
        return DDERR_INVALIDCAPS;
    }
    if (DDSD->dwFlags & DDSD_WIDTH)
    {
        if (!(DDSD->dwFlags & DDSD_PITCH))
        {
            WARN("DDSD_WIDTH is set, but DDSD_PITCH is not, returning DDERR_INVALIDPARAMS.\n");
            return DDERR_INVALIDPARAMS;
        }
        if (!DDSD->dwWidth || DDSD->u1.lPitch <= 0 || DDSD->u1.lPitch & 0x3)
        {
            WARN("Pitch is %d, width is %u, returning DDERR_INVALIDPARAMS.\n",
                    DDSD->u1.lPitch, DDSD->dwWidth);
            return DDERR_INVALIDPARAMS;
        }
        if (DDSD->dwWidth != This->surface_desc.dwWidth)
        {
            TRACE("Surface width changed from %u to %u.\n", This->surface_desc.dwWidth, DDSD->dwWidth);
            update_wined3d = TRUE;
        }
        if (DDSD->u1.lPitch != This->surface_desc.u1.lPitch)
        {
            TRACE("Surface pitch changed from %u to %u.\n", This->surface_desc.u1.lPitch, DDSD->u1.lPitch);
            update_wined3d = TRUE;
        }
        width = DDSD->dwWidth;
    }
    else if (DDSD->dwFlags & DDSD_PITCH)
    {
        WARN("DDSD_PITCH is set, but DDSD_WIDTH is not, returning DDERR_INVALIDPARAMS.\n");
        return DDERR_INVALIDPARAMS;
    }
    else
    {
        width = This->surface_desc.dwWidth;
    }

    if (DDSD->dwFlags & DDSD_HEIGHT)
    {
        if (!DDSD->dwHeight)
        {
            WARN("Height is 0, returning DDERR_INVALIDPARAMS.\n");
            return DDERR_INVALIDPARAMS;
        }
        if (DDSD->dwHeight != This->surface_desc.dwHeight)
        {
            TRACE("Surface height changed from %u to %u.\n", This->surface_desc.dwHeight, DDSD->dwHeight);
            update_wined3d = TRUE;
        }
        height = DDSD->dwHeight;
    }
    else
    {
        height = This->surface_desc.dwHeight;
    }

    wined3d_mutex_lock();
    if (DDSD->dwFlags & DDSD_PIXELFORMAT)
    {
        enum wined3d_format_id current_format_id;
        format_id = PixelFormat_DD2WineD3D(&DDSD->u4.ddpfPixelFormat);

        if (format_id == WINED3DFMT_UNKNOWN)
        {
            ERR("Requested to set an unknown pixelformat\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }
        current_format_id = PixelFormat_DD2WineD3D(&This->surface_desc.u4.ddpfPixelFormat);
        if (format_id != current_format_id)
        {
            TRACE("Surface format changed from %#x to %#x.\n", current_format_id, format_id);
            update_wined3d = TRUE;
        }
    }
    else
    {
        format_id = PixelFormat_DD2WineD3D(&This->surface_desc.u4.ddpfPixelFormat);
    }

    if (update_wined3d)
    {
        if (FAILED(hr = wined3d_surface_update_desc(This->wined3d_surface, width, height,
                format_id, WINED3D_MULTISAMPLE_NONE, 0)))
        {
            WARN("Failed to update surface desc, hr %#x.\n", hr);
            wined3d_mutex_unlock();
            return hr;
        }

        if (DDSD->dwFlags & DDSD_WIDTH)
            This->surface_desc.dwWidth = width;
        if (DDSD->dwFlags & DDSD_PITCH)
            This->surface_desc.u1.lPitch = DDSD->u1.lPitch;
        if (DDSD->dwFlags & DDSD_HEIGHT)
            This->surface_desc.dwHeight = height;
        if (DDSD->dwFlags & DDSD_PIXELFORMAT)
            This->surface_desc.u4.ddpfPixelFormat = DDSD->u4.ddpfPixelFormat;
    }

    if (DDSD->dwFlags & DDSD_LPSURFACE && DDSD->lpSurface)
    {
        hr = wined3d_surface_set_mem(This->wined3d_surface, DDSD->lpSurface);
        if (FAILED(hr))
        {
            /* No need for a trace here, wined3d does that for us */
            switch(hr)
            {
                case WINED3DERR_INVALIDCALL:
                    wined3d_mutex_unlock();
                    return DDERR_INVALIDPARAMS;
                default:
                    break; /* Go on */
            }
        }
        /* DDSD->lpSurface is set by Lock() */
    }

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_SetSurfaceDesc(IDirectDrawSurface4 *iface,
        DDSURFACEDESC2 *surface_desc, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, surface_desc %p, flags %#x.\n", iface, surface_desc, flags);

    return ddraw_surface7_SetSurfaceDesc(&This->IDirectDrawSurface7_iface,
            surface_desc, flags);
}

static HRESULT WINAPI ddraw_surface3_SetSurfaceDesc(IDirectDrawSurface3 *iface,
        DDSURFACEDESC *surface_desc, DWORD flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 surface_desc2;
    TRACE("iface %p, surface_desc %p, flags %#x.\n", iface, surface_desc, flags);

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw_surface7_SetSurfaceDesc(&This->IDirectDrawSurface7_iface,
            surface_desc ? &surface_desc2 : NULL, flags);
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
static HRESULT WINAPI ddraw_surface7_GetPalette(IDirectDrawSurface7 *iface, IDirectDrawPalette **Pal)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    struct wined3d_palette *wined3d_palette;
    HRESULT hr = DD_OK;

    TRACE("iface %p, palette %p.\n", iface, Pal);

    if(!Pal)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    wined3d_palette = wined3d_surface_get_palette(This->wined3d_surface);
    if (wined3d_palette)
    {
        *Pal = wined3d_palette_get_parent(wined3d_palette);
        IDirectDrawPalette_AddRef(*Pal);
    }
    else
    {
        *Pal = NULL;
        hr = DDERR_NOPALETTEATTACHED;
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_GetPalette(IDirectDrawSurface4 *iface, IDirectDrawPalette **palette)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_GetPalette(&This->IDirectDrawSurface7_iface, palette);
}

static HRESULT WINAPI ddraw_surface3_GetPalette(IDirectDrawSurface3 *iface, IDirectDrawPalette **palette)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_GetPalette(&This->IDirectDrawSurface7_iface, palette);
}

static HRESULT WINAPI ddraw_surface2_GetPalette(IDirectDrawSurface2 *iface, IDirectDrawPalette **palette)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_GetPalette(&This->IDirectDrawSurface7_iface, palette);
}

static HRESULT WINAPI ddraw_surface1_GetPalette(IDirectDrawSurface *iface, IDirectDrawPalette **palette)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_GetPalette(&This->IDirectDrawSurface7_iface, palette);
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
    struct wined3d_color_key *color_key;
    DWORD Flags;
};

static HRESULT WINAPI
SetColorKeyEnum(IDirectDrawSurface7 *surface,
                DDSURFACEDESC2 *desc,
                void *context)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(surface);
    struct SCKContext *ctx = context;
    HRESULT hr;

    hr = wined3d_surface_set_color_key(This->wined3d_surface, ctx->Flags, ctx->color_key);
    if (FAILED(hr))
    {
        WARN("IWineD3DSurface_SetColorKey failed, hr = %08x\n", hr);
        ctx->ret = hr;
    }

    ddraw_surface7_EnumAttachedSurfaces(surface, context, SetColorKeyEnum);
    ddraw_surface7_Release(surface);

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
static HRESULT WINAPI ddraw_surface7_SetColorKey(IDirectDrawSurface7 *iface, DWORD Flags, DDCOLORKEY *CKey)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    DDCOLORKEY FixedCKey;
    struct SCKContext ctx = { DD_OK, (struct wined3d_color_key *)(CKey ? &FixedCKey : NULL), Flags };

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, Flags, CKey);

    wined3d_mutex_lock();
    if (CKey)
    {
        FixedCKey = *CKey;
        /* Handle case where dwColorSpaceHighValue < dwColorSpaceLowValue */
        if (FixedCKey.dwColorSpaceHighValue < FixedCKey.dwColorSpaceLowValue)
            FixedCKey.dwColorSpaceHighValue = FixedCKey.dwColorSpaceLowValue;

        switch (Flags & ~DDCKEY_COLORSPACE)
        {
        case DDCKEY_DESTBLT:
            This->surface_desc.ddckCKDestBlt = FixedCKey;
            This->surface_desc.dwFlags |= DDSD_CKDESTBLT;
            break;

        case DDCKEY_DESTOVERLAY:
            This->surface_desc.u3.ddckCKDestOverlay = FixedCKey;
            This->surface_desc.dwFlags |= DDSD_CKDESTOVERLAY;
            break;

        case DDCKEY_SRCOVERLAY:
            This->surface_desc.ddckCKSrcOverlay = FixedCKey;
            This->surface_desc.dwFlags |= DDSD_CKSRCOVERLAY;
            break;

        case DDCKEY_SRCBLT:
            This->surface_desc.ddckCKSrcBlt = FixedCKey;
            This->surface_desc.dwFlags |= DDSD_CKSRCBLT;
            break;

        default:
            wined3d_mutex_unlock();
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
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }
    }
    ctx.ret = wined3d_surface_set_color_key(This->wined3d_surface, Flags, ctx.color_key);
    ddraw_surface7_EnumAttachedSurfaces(iface, &ctx, SetColorKeyEnum);
    wined3d_mutex_unlock();

    switch(ctx.ret)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return ctx.ret;
    }
}

static HRESULT WINAPI ddraw_surface4_SetColorKey(IDirectDrawSurface4 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_SetColorKey(&This->IDirectDrawSurface7_iface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface3_SetColorKey(IDirectDrawSurface3 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_SetColorKey(&This->IDirectDrawSurface7_iface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface2_SetColorKey(IDirectDrawSurface2 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_SetColorKey(&This->IDirectDrawSurface7_iface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface1_SetColorKey(IDirectDrawSurface *iface, DWORD flags, DDCOLORKEY *color_key)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_SetColorKey(&This->IDirectDrawSurface7_iface, flags, color_key);
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
static HRESULT WINAPI ddraw_surface7_SetPalette(IDirectDrawSurface7 *iface, IDirectDrawPalette *Pal)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface7(iface);
    IDirectDrawPalette *oldPal;
    IDirectDrawSurfaceImpl *surf;
    IDirectDrawPaletteImpl *PalImpl = unsafe_impl_from_IDirectDrawPalette(Pal);
    HRESULT hr;

    TRACE("iface %p, palette %p.\n", iface, Pal);

    if (!(This->surface_desc.u4.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 |
            DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXEDTO8))) {
        return DDERR_INVALIDPIXELFORMAT;
    }

    if (This->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
    {
        return DDERR_NOTONMIPMAPSUBLEVEL;
    }

    /* Find the old palette */
    wined3d_mutex_lock();
    hr = IDirectDrawSurface_GetPalette(iface, &oldPal);
    if(hr != DD_OK && hr != DDERR_NOPALETTEATTACHED)
    {
        wined3d_mutex_unlock();
        return hr;
    }
    if(oldPal) IDirectDrawPalette_Release(oldPal);  /* For the GetPalette */

    /* Set the new Palette */
    wined3d_surface_set_palette(This->wined3d_surface, PalImpl ? PalImpl->wineD3DPalette : NULL);
    /* AddRef the Palette */
    if(Pal) IDirectDrawPalette_AddRef(Pal);

    /* Release the old palette */
    if(oldPal) IDirectDrawPalette_Release(oldPal);

    /* Update the wined3d frontbuffer if this is the frontbuffer. */
    if ((This->surface_desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) && This->ddraw->wined3d_frontbuffer)
    {
        hr = wined3d_surface_set_palette(This->ddraw->wined3d_frontbuffer, PalImpl ? PalImpl->wineD3DPalette : NULL);
        if (FAILED(hr))
            ERR("Failed to set frontbuffer palette, hr %#x.\n", hr);
    }

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
            hr = ddraw_surface7_GetAttachedSurface(&surf->IDirectDrawSurface7_iface, &caps2, &attach);
            if(hr != DD_OK)
            {
                break;
            }

            TRACE("Setting palette on %p\n", attach);
            ddraw_surface7_SetPalette(attach, Pal);
            surf = impl_from_IDirectDrawSurface7(attach);
            ddraw_surface7_Release(attach);
        }
    }

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_SetPalette(IDirectDrawSurface4 *iface, IDirectDrawPalette *palette)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_SetPalette(&This->IDirectDrawSurface7_iface, palette);
}

static HRESULT WINAPI ddraw_surface3_SetPalette(IDirectDrawSurface3 *iface, IDirectDrawPalette *palette)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_SetPalette(&This->IDirectDrawSurface7_iface, palette);
}

static HRESULT WINAPI ddraw_surface2_SetPalette(IDirectDrawSurface2 *iface, IDirectDrawPalette *palette)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface2(iface);
    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_SetPalette(&This->IDirectDrawSurface7_iface, palette);
}

static HRESULT WINAPI ddraw_surface1_SetPalette(IDirectDrawSurface *iface, IDirectDrawPalette *palette)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface(iface);
    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_SetPalette(&This->IDirectDrawSurface7_iface, palette);
}

/**********************************************************
 * IDirectDrawGammaControl::GetGammaRamp
 *
 * Returns the current gamma ramp for a surface
 *
 * Params:
 *  flags: Ignored
 *  gamma_ramp: Address to write the ramp to
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if gamma_ramp is NULL
 *
 **********************************************************/
static HRESULT WINAPI ddraw_gamma_control_GetGammaRamp(IDirectDrawGammaControl *iface,
        DWORD flags, DDGAMMARAMP *gamma_ramp)
{
    IDirectDrawSurfaceImpl *surface = impl_from_IDirectDrawGammaControl(iface);

    TRACE("iface %p, flags %#x, gamma_ramp %p.\n", iface, flags, gamma_ramp);

    if (!gamma_ramp)
    {
        WARN("Invalid gamma_ramp passed.\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    if (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        /* Note: DDGAMMARAMP is compatible with struct wined3d_gamma_ramp. */
        wined3d_device_get_gamma_ramp(surface->ddraw->wined3d_device, 0, (struct wined3d_gamma_ramp *)gamma_ramp);
    }
    else
    {
        ERR("Not implemented for non-primary surfaces.\n");
    }
    wined3d_mutex_unlock();

    return DD_OK;
}

/**********************************************************
 * IDirectDrawGammaControl::SetGammaRamp
 *
 * Sets the red, green and blue gamma ramps for
 *
 * Params:
 *  flags: Can be DDSGR_CALIBRATE to request calibration
 *  gamma_ramp: Structure containing the new gamma ramp
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if gamma_ramp is NULL
 *
 **********************************************************/
static HRESULT WINAPI ddraw_gamma_control_SetGammaRamp(IDirectDrawGammaControl *iface,
        DWORD flags, DDGAMMARAMP *gamma_ramp)
{
    IDirectDrawSurfaceImpl *surface = impl_from_IDirectDrawGammaControl(iface);

    TRACE("iface %p, flags %#x, gamma_ramp %p.\n", iface, flags, gamma_ramp);

    if (!gamma_ramp)
    {
        WARN("Invalid gamma_ramp passed.\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    if (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        /* Note: DDGAMMARAMP is compatible with struct wined3d_gamma_ramp. */
        wined3d_device_set_gamma_ramp(surface->ddraw->wined3d_device,
                0, flags, (struct wined3d_gamma_ramp *)gamma_ramp);
    }
    else
    {
        ERR("Not implemented for non-primary surfaces.\n");
    }
    wined3d_mutex_unlock();

    return DD_OK;
}

/*****************************************************************************
 * IDirect3DTexture2::PaletteChanged
 *
 * Informs the texture about a palette change
 *
 * Params:
 *  start: Start index of the change
 *  count: The number of changed entries
 *
 * Returns
 *  D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_texture2_PaletteChanged(IDirect3DTexture2 *iface, DWORD start, DWORD count)
{
    FIXME("iface %p, start %u, count %u stub!\n", iface, start, count);

    return D3D_OK;
}

static HRESULT WINAPI d3d_texture1_PaletteChanged(IDirect3DTexture *iface, DWORD start, DWORD count)
{
    IDirectDrawSurfaceImpl *surface = impl_from_IDirect3DTexture(iface);

    TRACE("iface %p, start %u, count %u.\n", iface, start, count);

    return d3d_texture2_PaletteChanged(&surface->IDirect3DTexture2_iface, start, count);
}

/*****************************************************************************
 * IDirect3DTexture::Unload
 *
 * DX5 SDK: "The IDirect3DTexture2::Unload method is not implemented
 *
 *
 * Returns:
 *  DDERR_UNSUPPORTED
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_texture1_Unload(IDirect3DTexture *iface)
{
    WARN("iface %p. Not implemented.\n", iface);

    return DDERR_UNSUPPORTED;
}

/*****************************************************************************
 * IDirect3DTexture2::GetHandle
 *
 * Returns handle for the texture. At the moment, the interface
 * to the IWineD3DTexture is used.
 *
 * Params:
 *  device: Device this handle is assigned to
 *  handle: Address to store the handle at.
 *
 * Returns:
 *  D3D_OK
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_texture2_GetHandle(IDirect3DTexture2 *iface,
        IDirect3DDevice2 *device, D3DTEXTUREHANDLE *handle)
{
    IDirectDrawSurfaceImpl *surface = impl_from_IDirect3DTexture2(iface);
    IDirect3DDeviceImpl *device_impl = unsafe_impl_from_IDirect3DDevice2(device);

    TRACE("iface %p, device %p, handle %p.\n", iface, device, handle);

    wined3d_mutex_lock();

    if (!surface->Handle)
    {
        DWORD h = ddraw_allocate_handle(&device_impl->handle_table, surface, DDRAW_HANDLE_SURFACE);
        if (h == DDRAW_INVALID_HANDLE)
        {
            ERR("Failed to allocate a texture handle.\n");
            wined3d_mutex_unlock();
            return DDERR_OUTOFMEMORY;
        }

        surface->Handle = h + 1;
    }

    TRACE("Returning handle %08x.\n", surface->Handle);
    *handle = surface->Handle;

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_texture1_GetHandle(IDirect3DTexture *iface,
        IDirect3DDevice *device, D3DTEXTUREHANDLE *handle)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirect3DTexture(iface);
    IDirect3DDeviceImpl *device_impl = unsafe_impl_from_IDirect3DDevice(device);

    TRACE("iface %p, device %p, handle %p.\n", iface, device, handle);

    return d3d_texture2_GetHandle(&This->IDirect3DTexture2_iface,
            device_impl ? &device_impl->IDirect3DDevice2_iface : NULL, handle);
}

/*****************************************************************************
 * get_sub_mimaplevel
 *
 * Helper function that returns the next mipmap level
 *
 * tex_ptr: Surface of which to return the next level
 *
 *****************************************************************************/
static IDirectDrawSurfaceImpl *get_sub_mimaplevel(IDirectDrawSurfaceImpl *surface)
{
    /* Now go down the mipmap chain to the next surface */
    static DDSCAPS2 mipmap_caps = { DDSCAPS_MIPMAP | DDSCAPS_TEXTURE, 0, 0, {0} };
    IDirectDrawSurface7 *next_level;
    HRESULT hr;

    hr = ddraw_surface7_GetAttachedSurface(&surface->IDirectDrawSurface7_iface, &mipmap_caps, &next_level);
    if (FAILED(hr)) return NULL;

    ddraw_surface7_Release(next_level);

    return impl_from_IDirectDrawSurface7(next_level);
}

/*****************************************************************************
 * IDirect3DTexture2::Load
 *
 * Loads a texture created with the DDSCAPS_ALLOCONLOAD
 *
 * This function isn't relayed to WineD3D because the whole interface is
 * implemented in DDraw only. For speed improvements a implementation which
 * takes OpenGL more into account could be placed into WineD3D.
 *
 * Params:
 *  src_texture: Address of the texture to load
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_TEXTURE_LOAD_FAILED.
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_texture2_Load(IDirect3DTexture2 *iface, IDirect3DTexture2 *src_texture)
{
    IDirectDrawSurfaceImpl *dst_surface = impl_from_IDirect3DTexture2(iface);
    IDirectDrawSurfaceImpl *src_surface = unsafe_impl_from_IDirect3DTexture2(src_texture);
    HRESULT hr;

    TRACE("iface %p, src_texture %p.\n", iface, src_texture);

    if (src_surface == dst_surface)
    {
        TRACE("copying surface %p to surface %p, why?\n", src_surface, dst_surface);
        return D3D_OK;
    }

    wined3d_mutex_lock();

    if (((src_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
            != (dst_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP))
            || (src_surface->surface_desc.u2.dwMipMapCount != dst_surface->surface_desc.u2.dwMipMapCount))
    {
        ERR("Trying to load surfaces with different mip-map counts.\n");
    }

    for (;;)
    {
        struct wined3d_palette *wined3d_dst_pal, *wined3d_src_pal;
        IDirectDrawPalette *dst_pal = NULL, *src_pal = NULL;
        DDSURFACEDESC *src_desc, *dst_desc;

        TRACE("Copying surface %p to surface %p (mipmap level %d).\n",
                src_surface, dst_surface, src_surface->mipmap_level);

        /* Suppress the ALLOCONLOAD flag */
        dst_surface->surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;

        /* Get the palettes */
        wined3d_dst_pal = wined3d_surface_get_palette(dst_surface->wined3d_surface);
        if (wined3d_dst_pal)
            dst_pal = wined3d_palette_get_parent(wined3d_dst_pal);

        wined3d_src_pal = wined3d_surface_get_palette(src_surface->wined3d_surface);
        if (wined3d_src_pal)
            src_pal = wined3d_palette_get_parent(wined3d_src_pal);

        if (src_pal)
        {
            PALETTEENTRY palent[256];

            if (!dst_pal)
            {
                wined3d_mutex_unlock();
                return DDERR_NOPALETTEATTACHED;
            }
            IDirectDrawPalette_GetEntries(src_pal, 0, 0, 256, palent);
            IDirectDrawPalette_SetEntries(dst_pal, 0, 0, 256, palent);
        }

        /* Copy one surface on the other */
        dst_desc = (DDSURFACEDESC *)&(dst_surface->surface_desc);
        src_desc = (DDSURFACEDESC *)&(src_surface->surface_desc);

        if ((src_desc->dwWidth != dst_desc->dwWidth) || (src_desc->dwHeight != dst_desc->dwHeight))
        {
            /* Should also check for same pixel format, u1.lPitch, ... */
            ERR("Error in surface sizes.\n");
            wined3d_mutex_unlock();
            return D3DERR_TEXTURE_LOAD_FAILED;
        }
        else
        {
            struct wined3d_mapped_rect src_rect, dst_rect;

            /* Copy also the ColorKeying stuff */
            if (src_desc->dwFlags & DDSD_CKSRCBLT)
            {
                dst_desc->dwFlags |= DDSD_CKSRCBLT;
                dst_desc->ddckCKSrcBlt.dwColorSpaceLowValue = src_desc->ddckCKSrcBlt.dwColorSpaceLowValue;
                dst_desc->ddckCKSrcBlt.dwColorSpaceHighValue = src_desc->ddckCKSrcBlt.dwColorSpaceHighValue;
            }

            /* Copy the main memory texture into the surface that corresponds
             * to the OpenGL texture object. */

            hr = wined3d_surface_map(src_surface->wined3d_surface, &src_rect, NULL, 0);
            if (FAILED(hr))
            {
                ERR("Failed to lock source surface, hr %#x.\n", hr);
                wined3d_mutex_unlock();
                return D3DERR_TEXTURE_LOAD_FAILED;
            }

            hr = wined3d_surface_map(dst_surface->wined3d_surface, &dst_rect, NULL, 0);
            if (FAILED(hr))
            {
                ERR("Failed to lock destination surface, hr %#x.\n", hr);
                wined3d_surface_unmap(src_surface->wined3d_surface);
                wined3d_mutex_unlock();
                return D3DERR_TEXTURE_LOAD_FAILED;
            }

            if (dst_surface->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
                memcpy(dst_rect.data, src_rect.data, src_surface->surface_desc.u1.dwLinearSize);
            else
                memcpy(dst_rect.data, src_rect.data, src_rect.row_pitch * src_desc->dwHeight);

            wined3d_surface_unmap(src_surface->wined3d_surface);
            wined3d_surface_unmap(dst_surface->wined3d_surface);
        }

        if (src_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
            src_surface = get_sub_mimaplevel(src_surface);
        else
            src_surface = NULL;

        if (dst_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
            dst_surface = get_sub_mimaplevel(dst_surface);
        else
            dst_surface = NULL;

        if (!src_surface || !dst_surface)
        {
            if (src_surface != dst_surface)
                ERR("Loading surface with different mipmap structure.\n");
            break;
        }
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_texture1_Load(IDirect3DTexture *iface, IDirect3DTexture *src_texture)
{
    IDirectDrawSurfaceImpl* This = impl_from_IDirect3DTexture(iface);
    IDirectDrawSurfaceImpl* src_surface = unsafe_impl_from_IDirect3DTexture(src_texture);
    TRACE("iface %p, src_texture %p.\n", iface, src_texture);

    return d3d_texture2_Load(&This->IDirect3DTexture2_iface,
            src_surface ? &src_surface->IDirect3DTexture2_iface : NULL);
}

/*****************************************************************************
 * The VTable
 *****************************************************************************/

static const struct IDirectDrawSurface7Vtbl ddraw_surface7_vtbl =
{
    /* IUnknown */
    ddraw_surface7_QueryInterface,
    ddraw_surface7_AddRef,
    ddraw_surface7_Release,
    /* IDirectDrawSurface */
    ddraw_surface7_AddAttachedSurface,
    ddraw_surface7_AddOverlayDirtyRect,
    ddraw_surface7_Blt,
    ddraw_surface7_BltBatch,
    ddraw_surface7_BltFast,
    ddraw_surface7_DeleteAttachedSurface,
    ddraw_surface7_EnumAttachedSurfaces,
    ddraw_surface7_EnumOverlayZOrders,
    ddraw_surface7_Flip,
    ddraw_surface7_GetAttachedSurface,
    ddraw_surface7_GetBltStatus,
    ddraw_surface7_GetCaps,
    ddraw_surface7_GetClipper,
    ddraw_surface7_GetColorKey,
    ddraw_surface7_GetDC,
    ddraw_surface7_GetFlipStatus,
    ddraw_surface7_GetOverlayPosition,
    ddraw_surface7_GetPalette,
    ddraw_surface7_GetPixelFormat,
    ddraw_surface7_GetSurfaceDesc,
    ddraw_surface7_Initialize,
    ddraw_surface7_IsLost,
    ddraw_surface7_Lock,
    ddraw_surface7_ReleaseDC,
    ddraw_surface7_Restore,
    ddraw_surface7_SetClipper,
    ddraw_surface7_SetColorKey,
    ddraw_surface7_SetOverlayPosition,
    ddraw_surface7_SetPalette,
    ddraw_surface7_Unlock,
    ddraw_surface7_UpdateOverlay,
    ddraw_surface7_UpdateOverlayDisplay,
    ddraw_surface7_UpdateOverlayZOrder,
    /* IDirectDrawSurface2 */
    ddraw_surface7_GetDDInterface,
    ddraw_surface7_PageLock,
    ddraw_surface7_PageUnlock,
    /* IDirectDrawSurface3 */
    ddraw_surface7_SetSurfaceDesc,
    /* IDirectDrawSurface4 */
    ddraw_surface7_SetPrivateData,
    ddraw_surface7_GetPrivateData,
    ddraw_surface7_FreePrivateData,
    ddraw_surface7_GetUniquenessValue,
    ddraw_surface7_ChangeUniquenessValue,
    /* IDirectDrawSurface7 */
    ddraw_surface7_SetPriority,
    ddraw_surface7_GetPriority,
    ddraw_surface7_SetLOD,
    ddraw_surface7_GetLOD,
};

static const struct IDirectDrawSurface4Vtbl ddraw_surface4_vtbl =
{
    /* IUnknown */
    ddraw_surface4_QueryInterface,
    ddraw_surface4_AddRef,
    ddraw_surface4_Release,
    /* IDirectDrawSurface */
    ddraw_surface4_AddAttachedSurface,
    ddraw_surface4_AddOverlayDirtyRect,
    ddraw_surface4_Blt,
    ddraw_surface4_BltBatch,
    ddraw_surface4_BltFast,
    ddraw_surface4_DeleteAttachedSurface,
    ddraw_surface4_EnumAttachedSurfaces,
    ddraw_surface4_EnumOverlayZOrders,
    ddraw_surface4_Flip,
    ddraw_surface4_GetAttachedSurface,
    ddraw_surface4_GetBltStatus,
    ddraw_surface4_GetCaps,
    ddraw_surface4_GetClipper,
    ddraw_surface4_GetColorKey,
    ddraw_surface4_GetDC,
    ddraw_surface4_GetFlipStatus,
    ddraw_surface4_GetOverlayPosition,
    ddraw_surface4_GetPalette,
    ddraw_surface4_GetPixelFormat,
    ddraw_surface4_GetSurfaceDesc,
    ddraw_surface4_Initialize,
    ddraw_surface4_IsLost,
    ddraw_surface4_Lock,
    ddraw_surface4_ReleaseDC,
    ddraw_surface4_Restore,
    ddraw_surface4_SetClipper,
    ddraw_surface4_SetColorKey,
    ddraw_surface4_SetOverlayPosition,
    ddraw_surface4_SetPalette,
    ddraw_surface4_Unlock,
    ddraw_surface4_UpdateOverlay,
    ddraw_surface4_UpdateOverlayDisplay,
    ddraw_surface4_UpdateOverlayZOrder,
    /* IDirectDrawSurface2 */
    ddraw_surface4_GetDDInterface,
    ddraw_surface4_PageLock,
    ddraw_surface4_PageUnlock,
    /* IDirectDrawSurface3 */
    ddraw_surface4_SetSurfaceDesc,
    /* IDirectDrawSurface4 */
    ddraw_surface4_SetPrivateData,
    ddraw_surface4_GetPrivateData,
    ddraw_surface4_FreePrivateData,
    ddraw_surface4_GetUniquenessValue,
    ddraw_surface4_ChangeUniquenessValue,
};

static const struct IDirectDrawSurface3Vtbl ddraw_surface3_vtbl =
{
    /* IUnknown */
    ddraw_surface3_QueryInterface,
    ddraw_surface3_AddRef,
    ddraw_surface3_Release,
    /* IDirectDrawSurface */
    ddraw_surface3_AddAttachedSurface,
    ddraw_surface3_AddOverlayDirtyRect,
    ddraw_surface3_Blt,
    ddraw_surface3_BltBatch,
    ddraw_surface3_BltFast,
    ddraw_surface3_DeleteAttachedSurface,
    ddraw_surface3_EnumAttachedSurfaces,
    ddraw_surface3_EnumOverlayZOrders,
    ddraw_surface3_Flip,
    ddraw_surface3_GetAttachedSurface,
    ddraw_surface3_GetBltStatus,
    ddraw_surface3_GetCaps,
    ddraw_surface3_GetClipper,
    ddraw_surface3_GetColorKey,
    ddraw_surface3_GetDC,
    ddraw_surface3_GetFlipStatus,
    ddraw_surface3_GetOverlayPosition,
    ddraw_surface3_GetPalette,
    ddraw_surface3_GetPixelFormat,
    ddraw_surface3_GetSurfaceDesc,
    ddraw_surface3_Initialize,
    ddraw_surface3_IsLost,
    ddraw_surface3_Lock,
    ddraw_surface3_ReleaseDC,
    ddraw_surface3_Restore,
    ddraw_surface3_SetClipper,
    ddraw_surface3_SetColorKey,
    ddraw_surface3_SetOverlayPosition,
    ddraw_surface3_SetPalette,
    ddraw_surface3_Unlock,
    ddraw_surface3_UpdateOverlay,
    ddraw_surface3_UpdateOverlayDisplay,
    ddraw_surface3_UpdateOverlayZOrder,
    /* IDirectDrawSurface2 */
    ddraw_surface3_GetDDInterface,
    ddraw_surface3_PageLock,
    ddraw_surface3_PageUnlock,
    /* IDirectDrawSurface3 */
    ddraw_surface3_SetSurfaceDesc,
};

static const struct IDirectDrawSurface2Vtbl ddraw_surface2_vtbl =
{
    /* IUnknown */
    ddraw_surface2_QueryInterface,
    ddraw_surface2_AddRef,
    ddraw_surface2_Release,
    /* IDirectDrawSurface */
    ddraw_surface2_AddAttachedSurface,
    ddraw_surface2_AddOverlayDirtyRect,
    ddraw_surface2_Blt,
    ddraw_surface2_BltBatch,
    ddraw_surface2_BltFast,
    ddraw_surface2_DeleteAttachedSurface,
    ddraw_surface2_EnumAttachedSurfaces,
    ddraw_surface2_EnumOverlayZOrders,
    ddraw_surface2_Flip,
    ddraw_surface2_GetAttachedSurface,
    ddraw_surface2_GetBltStatus,
    ddraw_surface2_GetCaps,
    ddraw_surface2_GetClipper,
    ddraw_surface2_GetColorKey,
    ddraw_surface2_GetDC,
    ddraw_surface2_GetFlipStatus,
    ddraw_surface2_GetOverlayPosition,
    ddraw_surface2_GetPalette,
    ddraw_surface2_GetPixelFormat,
    ddraw_surface2_GetSurfaceDesc,
    ddraw_surface2_Initialize,
    ddraw_surface2_IsLost,
    ddraw_surface2_Lock,
    ddraw_surface2_ReleaseDC,
    ddraw_surface2_Restore,
    ddraw_surface2_SetClipper,
    ddraw_surface2_SetColorKey,
    ddraw_surface2_SetOverlayPosition,
    ddraw_surface2_SetPalette,
    ddraw_surface2_Unlock,
    ddraw_surface2_UpdateOverlay,
    ddraw_surface2_UpdateOverlayDisplay,
    ddraw_surface2_UpdateOverlayZOrder,
    /* IDirectDrawSurface2 */
    ddraw_surface2_GetDDInterface,
    ddraw_surface2_PageLock,
    ddraw_surface2_PageUnlock,
};

static const struct IDirectDrawSurfaceVtbl ddraw_surface1_vtbl =
{
    /* IUnknown */
    ddraw_surface1_QueryInterface,
    ddraw_surface1_AddRef,
    ddraw_surface1_Release,
    /* IDirectDrawSurface */
    ddraw_surface1_AddAttachedSurface,
    ddraw_surface1_AddOverlayDirtyRect,
    ddraw_surface1_Blt,
    ddraw_surface1_BltBatch,
    ddraw_surface1_BltFast,
    ddraw_surface1_DeleteAttachedSurface,
    ddraw_surface1_EnumAttachedSurfaces,
    ddraw_surface1_EnumOverlayZOrders,
    ddraw_surface1_Flip,
    ddraw_surface1_GetAttachedSurface,
    ddraw_surface1_GetBltStatus,
    ddraw_surface1_GetCaps,
    ddraw_surface1_GetClipper,
    ddraw_surface1_GetColorKey,
    ddraw_surface1_GetDC,
    ddraw_surface1_GetFlipStatus,
    ddraw_surface1_GetOverlayPosition,
    ddraw_surface1_GetPalette,
    ddraw_surface1_GetPixelFormat,
    ddraw_surface1_GetSurfaceDesc,
    ddraw_surface1_Initialize,
    ddraw_surface1_IsLost,
    ddraw_surface1_Lock,
    ddraw_surface1_ReleaseDC,
    ddraw_surface1_Restore,
    ddraw_surface1_SetClipper,
    ddraw_surface1_SetColorKey,
    ddraw_surface1_SetOverlayPosition,
    ddraw_surface1_SetPalette,
    ddraw_surface1_Unlock,
    ddraw_surface1_UpdateOverlay,
    ddraw_surface1_UpdateOverlayDisplay,
    ddraw_surface1_UpdateOverlayZOrder,
};

static const struct IDirectDrawGammaControlVtbl ddraw_gamma_control_vtbl =
{
    ddraw_gamma_control_QueryInterface,
    ddraw_gamma_control_AddRef,
    ddraw_gamma_control_Release,
    ddraw_gamma_control_GetGammaRamp,
    ddraw_gamma_control_SetGammaRamp,
};

static const struct IDirect3DTexture2Vtbl d3d_texture2_vtbl =
{
    d3d_texture2_QueryInterface,
    d3d_texture2_AddRef,
    d3d_texture2_Release,
    d3d_texture2_GetHandle,
    d3d_texture2_PaletteChanged,
    d3d_texture2_Load,
};

static const struct IDirect3DTextureVtbl d3d_texture1_vtbl =
{
    d3d_texture1_QueryInterface,
    d3d_texture1_AddRef,
    d3d_texture1_Release,
    d3d_texture1_Initialize,
    d3d_texture1_GetHandle,
    d3d_texture1_PaletteChanged,
    d3d_texture1_Load,
    d3d_texture1_Unload,
};

IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface7(IDirectDrawSurface7 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &ddraw_surface7_vtbl);
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface7_iface);
}

IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &ddraw_surface4_vtbl);
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface4_iface);
}

static IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &ddraw_surface3_vtbl);
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface3_iface);
}

static IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface2(IDirectDrawSurface2 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &ddraw_surface2_vtbl);
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface2_iface);
}

IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface(IDirectDrawSurface *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &ddraw_surface1_vtbl);
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface_iface);
}

IDirectDrawSurfaceImpl *unsafe_impl_from_IDirect3DTexture2(IDirect3DTexture2 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_texture2_vtbl);
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirect3DTexture2_iface);
}

IDirectDrawSurfaceImpl *unsafe_impl_from_IDirect3DTexture(IDirect3DTexture *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_texture1_vtbl);
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirect3DTexture_iface);
}

static void STDMETHODCALLTYPE ddraw_surface_wined3d_object_destroyed(void *parent)
{
    IDirectDrawSurfaceImpl *surface = parent;

    TRACE("surface %p.\n", surface);

    /* Check for attached surfaces and detach them. */
    if (surface->first_attached != surface)
    {
        /* Well, this shouldn't happen: The surface being attached is
         * referenced in AddAttachedSurface(), so it shouldn't be released
         * until DeleteAttachedSurface() is called, because the refcount is
         * held. It looks like the application released it often enough to
         * force this. */
        WARN("Surface is still attached to surface %p.\n", surface->first_attached);

        /* The refcount will drop to -1 here */
        if (FAILED(ddraw_surface_delete_attached_surface(surface->first_attached, surface, surface->attached_iface)))
            ERR("DeleteAttachedSurface failed.\n");
    }

    while (surface->next_attached)
        if (FAILED(ddraw_surface_delete_attached_surface(surface,
                surface->next_attached, surface->next_attached->attached_iface)))
            ERR("DeleteAttachedSurface failed.\n");

    /* Having a texture handle set implies that the device still exists. */
    if (surface->Handle)
        ddraw_free_handle(&surface->ddraw->d3ddevice->handle_table, surface->Handle - 1, DDRAW_HANDLE_SURFACE);

    /* Reduce the ddraw surface count. */
    list_remove(&surface->surface_list_entry);

    if (surface == surface->ddraw->primary)
        surface->ddraw->primary = NULL;

    HeapFree(GetProcessHeap(), 0, surface);
}

const struct wined3d_parent_ops ddraw_surface_wined3d_parent_ops =
{
    ddraw_surface_wined3d_object_destroyed,
};

static void STDMETHODCALLTYPE ddraw_texture_wined3d_object_destroyed(void *parent)
{
    IDirectDrawSurfaceImpl *surface = parent;

    TRACE("surface %p.\n", surface);

    ddraw_surface_cleanup(surface);
}

static const struct wined3d_parent_ops ddraw_texture_wined3d_parent_ops =
{
    ddraw_texture_wined3d_object_destroyed,
};

HRESULT ddraw_surface_create_texture(IDirectDrawSurfaceImpl *surface)
{
    const DDSURFACEDESC2 *desc = &surface->surface_desc;
    enum wined3d_format_id format;
    enum wined3d_pool pool;
    UINT levels;

    if (desc->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
        levels = desc->u2.dwMipMapCount;
    else
        levels = 1;

    /* DDSCAPS_SYSTEMMEMORY textures are in WINED3D_POOL_SYSTEM_MEM.
     * Should I forward the MANAGED cap to the managed pool? */
    if (desc->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        pool = WINED3D_POOL_SYSTEM_MEM;
    else
        pool = WINED3D_POOL_DEFAULT;

    format = PixelFormat_DD2WineD3D(&surface->surface_desc.u4.ddpfPixelFormat);
    if (desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
        return wined3d_texture_create_cube(surface->ddraw->wined3d_device, desc->dwWidth,
                levels, 0, format, pool, surface, &ddraw_texture_wined3d_parent_ops, &surface->wined3d_texture);
    else
        return wined3d_texture_create_2d(surface->ddraw->wined3d_device, desc->dwWidth, desc->dwHeight,
                levels, 0, format, pool, surface, &ddraw_texture_wined3d_parent_ops, &surface->wined3d_texture);
}

HRESULT ddraw_surface_init(IDirectDrawSurfaceImpl *surface, IDirectDrawImpl *ddraw,
        DDSURFACEDESC2 *desc, UINT mip_level, UINT version)
{
    enum wined3d_pool pool = WINED3D_POOL_DEFAULT;
    DWORD flags = WINED3D_SURFACE_MAPPABLE;
    enum wined3d_format_id format;
    DWORD usage = 0;
    HRESULT hr;

    if (!(desc->ddsCaps.dwCaps & (DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY))
            && !((desc->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
            && (desc->ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)))
    {
        /* Tests show surfaces without memory flags get these flags added
         * right after creation. */
        desc->ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    }

    if (desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        /* Some applications assume that the primary surface will always be
         * mapped at the same address. Some of those also assume that this
         * address is valid even when the surface isn't mapped, and that
         * updates done this way will be visible on the screen. The game Nox
         * is such an application. */
        if (version == 1)
            flags |= WINED3D_SURFACE_PIN_SYSMEM;
        usage |= WINED3DUSAGE_RENDERTARGET;
        desc->ddsCaps.dwCaps |= DDSCAPS_VISIBLE;
    }

    if ((desc->ddsCaps.dwCaps & DDSCAPS_3DDEVICE) && !(desc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
    {
        usage |= WINED3DUSAGE_RENDERTARGET;
    }

    if (desc->ddsCaps.dwCaps & (DDSCAPS_OVERLAY))
    {
        usage |= WINED3DUSAGE_OVERLAY;
    }

    if (desc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        usage |= WINED3DUSAGE_DEPTHSTENCIL;

    if (desc->ddsCaps.dwCaps & DDSCAPS_OWNDC)
        usage |= WINED3DUSAGE_OWNDC;

    if (desc->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
    {
        pool = WINED3D_POOL_SYSTEM_MEM;
    }
    else if (desc->ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
    {
        pool = WINED3D_POOL_MANAGED;
        /* Managed textures have the system memory flag set. */
        desc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    }
    else if (desc->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
    {
        /* Videomemory adds localvidmem. This is mutually exclusive with
         * systemmemory and texturemanage. */
        desc->ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM;
    }

    format = PixelFormat_DD2WineD3D(&desc->u4.ddpfPixelFormat);
    if (format == WINED3DFMT_UNKNOWN)
    {
        WARN("Unsupported / unknown pixelformat.\n");
        return DDERR_INVALIDPIXELFORMAT;
    }

    surface->IDirectDrawSurface7_iface.lpVtbl = &ddraw_surface7_vtbl;
    surface->IDirectDrawSurface4_iface.lpVtbl = &ddraw_surface4_vtbl;
    surface->IDirectDrawSurface3_iface.lpVtbl = &ddraw_surface3_vtbl;
    surface->IDirectDrawSurface2_iface.lpVtbl = &ddraw_surface2_vtbl;
    surface->IDirectDrawSurface_iface.lpVtbl = &ddraw_surface1_vtbl;
    surface->IDirectDrawGammaControl_iface.lpVtbl = &ddraw_gamma_control_vtbl;
    surface->IDirect3DTexture2_iface.lpVtbl = &d3d_texture2_vtbl;
    surface->IDirect3DTexture_iface.lpVtbl = &d3d_texture1_vtbl;
    surface->iface_count = 1;
    surface->version = version;
    surface->ddraw = ddraw;

    if (version == 7)
    {
        surface->ref7 = 1;
    }
    else if (version == 4)
    {
        surface->ref4 = 1;
    }
    else
    {
        surface->ref1 = 1;
    }

    copy_to_surfacedesc2(&surface->surface_desc, desc);

    surface->first_attached = surface;

    hr = wined3d_surface_create(ddraw->wined3d_device, desc->dwWidth, desc->dwHeight, format, mip_level,
            usage, pool, WINED3D_MULTISAMPLE_NONE, 0, DefaultSurfaceType, flags,
            surface, &ddraw_surface_wined3d_parent_ops, &surface->wined3d_surface);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d surface, hr %#x.\n", hr);
        return hr;
    }

    /* Anno 1602 stores the pitch right after surface creation, so make sure
     * it's there. TODO: Test other fourcc formats. */
    if (format == WINED3DFMT_DXT1 || format == WINED3DFMT_DXT2 || format == WINED3DFMT_DXT3
            || format == WINED3DFMT_DXT4 || format == WINED3DFMT_DXT5)
    {
        surface->surface_desc.dwFlags |= DDSD_LINEARSIZE;
        if (format == WINED3DFMT_DXT1)
        {
            surface->surface_desc.u1.dwLinearSize = max(4, desc->dwWidth) * max(4, desc->dwHeight) / 2;
        }
        else
        {
            surface->surface_desc.u1.dwLinearSize = max(4, desc->dwWidth) * max(4, desc->dwHeight);
        }
    }
    else
    {
        surface->surface_desc.dwFlags |= DDSD_PITCH;
        surface->surface_desc.u1.lPitch = wined3d_surface_get_pitch(surface->wined3d_surface);
    }

    if (desc->dwFlags & DDSD_CKDESTOVERLAY)
    {
        wined3d_surface_set_color_key(surface->wined3d_surface, DDCKEY_DESTOVERLAY,
                (struct wined3d_color_key *)&desc->u3.ddckCKDestOverlay);
    }
    if (desc->dwFlags & DDSD_CKDESTBLT)
    {
        wined3d_surface_set_color_key(surface->wined3d_surface, DDCKEY_DESTBLT,
                (struct wined3d_color_key *)&desc->ddckCKDestBlt);
    }
    if (desc->dwFlags & DDSD_CKSRCOVERLAY)
    {
        wined3d_surface_set_color_key(surface->wined3d_surface, DDCKEY_SRCOVERLAY,
                (struct wined3d_color_key *)&desc->ddckCKSrcOverlay);
    }
    if (desc->dwFlags & DDSD_CKSRCBLT)
    {
        wined3d_surface_set_color_key(surface->wined3d_surface, DDCKEY_SRCBLT,
                (struct wined3d_color_key *)&desc->ddckCKSrcBlt);
    }
    if (desc->dwFlags & DDSD_LPSURFACE)
    {
        hr = wined3d_surface_set_mem(surface->wined3d_surface, desc->lpSurface);
        if (FAILED(hr))
        {
            ERR("Failed to set surface memory, hr %#x.\n", hr);
            wined3d_surface_decref(surface->wined3d_surface);
            return hr;
        }
    }

    return DD_OK;
}
