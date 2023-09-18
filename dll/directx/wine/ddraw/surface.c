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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface2(IDirectDrawSurface2 *iface);
static struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface);

static inline struct ddraw_surface *impl_from_IDirectDrawGammaControl(IDirectDrawGammaControl *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawGammaControl_iface);
}

/* This is slow, of course. Also, in case of locks, we can't prevent other
 * applications from drawing to the screen while we've locked the frontbuffer.
 * We'd like to do this in wined3d instead, but for that to work wined3d needs
 * to support windowless rendering first. */
HRESULT ddraw_surface_update_frontbuffer(struct ddraw_surface *surface,
        const RECT *rect, BOOL read, unsigned int swap_interval)
{
    struct wined3d_texture *dst_texture;
    HDC surface_dc, screen_dc;
    int x, y, w, h;
    HRESULT hr;
    BOOL ret;
    RECT r;

    if (surface->ddraw->flags & DDRAW_SWAPPED && !read)
    {
        surface->ddraw->flags &= ~DDRAW_SWAPPED;
        rect = NULL;
    }

    if (!rect)
    {
        SetRect(&r, 0, 0, surface->surface_desc.dwWidth, surface->surface_desc.dwHeight);
        rect = &r;
    }

    x = rect->left;
    y = rect->top;
    w = rect->right - rect->left;
    h = rect->bottom - rect->top;

    if (w <= 0 || h <= 0)
        return DD_OK;

    if (surface->ddraw->swapchain_window)
    {
        /* Nothing to do, we control the frontbuffer, or at least the parts we
         * care about. */
        if (read)
            return DD_OK;

        if (swap_interval)
            dst_texture = wined3d_swapchain_get_back_buffer(surface->ddraw->wined3d_swapchain, 0);
        else
            dst_texture = surface->ddraw->wined3d_frontbuffer;

        if (SUCCEEDED(hr = wined3d_texture_blt(dst_texture, 0, rect, surface->wined3d_texture,
                surface->sub_resource_idx, rect, 0, NULL, WINED3D_TEXF_POINT)) && swap_interval)
        {
            hr = wined3d_swapchain_present(surface->ddraw->wined3d_swapchain, rect, rect, NULL, swap_interval, 0);
            surface->ddraw->flags |= DDRAW_SWAPPED;
        }
        return hr;
    }

    if (FAILED(hr = wined3d_texture_get_dc(surface->wined3d_texture, surface->sub_resource_idx, &surface_dc)))
    {
        ERR("Failed to get surface DC, hr %#x.\n", hr);
        return hr;
    }
    if (surface->palette)
        wined3d_palette_apply_to_dc(surface->palette->wined3d_palette, surface_dc);

    if (!(screen_dc = GetDC(NULL)))
    {
        wined3d_texture_release_dc(surface->wined3d_texture, surface->sub_resource_idx, surface_dc);
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
    wined3d_texture_release_dc(surface->wined3d_texture, surface->sub_resource_idx, surface_dc);

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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obj);

    /* According to COM docs, if the QueryInterface fails, obj should be set to NULL */
    *obj = NULL;

    if(!riid)
        return DDERR_INVALIDPARAMS;

    if (IsEqualGUID(riid, &IID_IDirectDrawSurface7))
    {
        IDirectDrawSurface7_AddRef(iface);
        *obj = iface;
        TRACE("(%p) returning IDirectDrawSurface7 interface at %p\n", This, *obj);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirectDrawSurface4))
    {
        IDirectDrawSurface4_AddRef(&This->IDirectDrawSurface4_iface);
        *obj = &This->IDirectDrawSurface4_iface;
        TRACE("(%p) returning IDirectDrawSurface4 interface at %p\n", This, *obj);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirectDrawSurface3))
    {
        IDirectDrawSurface3_AddRef(&This->IDirectDrawSurface3_iface);
        *obj = &This->IDirectDrawSurface3_iface;
        TRACE("(%p) returning IDirectDrawSurface3 interface at %p\n", This, *obj);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirectDrawSurface2))
    {
        IDirectDrawSurface2_AddRef(&This->IDirectDrawSurface2_iface);
        *obj = &This->IDirectDrawSurface2_iface;
        TRACE("(%p) returning IDirectDrawSurface2 interface at %p\n", This, *obj);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirectDrawSurface)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirectDrawSurface_AddRef(&This->IDirectDrawSurface_iface);
        *obj = &This->IDirectDrawSurface_iface;
        TRACE("(%p) returning IDirectDrawSurface interface at %p\n", This, *obj);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirectDrawGammaControl))
    {
        IDirectDrawGammaControl_AddRef(&This->IDirectDrawGammaControl_iface);
        *obj = &This->IDirectDrawGammaControl_iface;
        TRACE("(%p) returning IDirectDrawGammaControl interface at %p\n", This, *obj);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirectDrawColorControl))
    {
        WARN("Color control not implemented.\n");
        *obj = NULL;
        return E_NOINTERFACE;
    }

    if (This->version != 7)
    {
        if (IsEqualGUID(riid, &IID_D3DDEVICE_WineD3D)
                || IsEqualGUID(riid, &IID_IDirect3DHALDevice)
                || IsEqualGUID(riid, &IID_IDirect3DRGBDevice))
        {
            wined3d_mutex_lock();
            if (!This->device1)
            {
                HRESULT hr;

                if (FAILED(hr = d3d_device_create(This->ddraw, This, (IUnknown *)&This->IDirectDrawSurface_iface,
                        1, &This->device1, (IUnknown *)&This->IDirectDrawSurface_iface)))
                {
                    This->device1 = NULL;
                    wined3d_mutex_unlock();
                    WARN("Failed to create device, hr %#x.\n", hr);
                    return hr;
                }
            }
            wined3d_mutex_unlock();

            IDirect3DDevice_AddRef(&This->device1->IDirect3DDevice_iface);
            *obj = &This->device1->IDirect3DDevice_iface;
            return S_OK;
        }

        if (IsEqualGUID(&IID_IDirect3DTexture2, riid))
        {
            IDirect3DTexture2_AddRef(&This->IDirect3DTexture2_iface);
            *obj = &This->IDirect3DTexture2_iface;
            return S_OK;
        }

        if (IsEqualGUID( &IID_IDirect3DTexture, riid ))
        {
            IDirect3DTexture2_AddRef(&This->IDirect3DTexture_iface);
            *obj = &This->IDirect3DTexture_iface;
            return S_OK;
        }
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    if (This->version != 7)
        return E_INVALIDARG;

    return E_NOINTERFACE;
}

static HRESULT WINAPI ddraw_surface4_QueryInterface(IDirectDrawSurface4 *iface, REFIID riid, void **object)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&surface->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI ddraw_surface3_QueryInterface(IDirectDrawSurface3 *iface, REFIID riid, void **object)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&surface->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI ddraw_surface2_QueryInterface(IDirectDrawSurface2 *iface, REFIID riid, void **object)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&surface->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI ddraw_surface1_QueryInterface(IDirectDrawSurface *iface, REFIID riid, void **object)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&surface->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI ddraw_gamma_control_QueryInterface(IDirectDrawGammaControl *iface,
        REFIID riid, void **object)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawGammaControl(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&surface->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI d3d_texture2_QueryInterface(IDirect3DTexture2 *iface, REFIID riid, void **object)
{
    struct ddraw_surface *surface = impl_from_IDirect3DTexture2(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&surface->IDirectDrawSurface7_iface, riid, object);
}

static HRESULT WINAPI d3d_texture1_QueryInterface(IDirect3DTexture *iface, REFIID riid, void **object)
{
    struct ddraw_surface *surface = impl_from_IDirect3DTexture(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw_surface7_QueryInterface(&surface->IDirectDrawSurface7_iface, riid, object);
}

static void ddraw_surface_add_iface(struct ddraw_surface *surface)
{
    ULONG iface_count = InterlockedIncrement(&surface->iface_count);
    TRACE("%p increasing iface count to %u.\n", surface, iface_count);

    if (iface_count == 1)
    {
        if (surface->ifaceToRelease)
            IUnknown_AddRef(surface->ifaceToRelease);
        wined3d_mutex_lock();
        if (surface->wined3d_rtv)
            wined3d_rendertarget_view_incref(surface->wined3d_rtv);
        wined3d_texture_incref(surface->wined3d_texture);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface7(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface4(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface3(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface2(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawGammaControl(iface);
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
    struct ddraw_surface *surface = impl_from_IDirect3DTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_AddRef(surface->texture_outer);
}

static ULONG WINAPI d3d_texture1_AddRef(IDirect3DTexture *iface)
{
    struct ddraw_surface *surface = impl_from_IDirect3DTexture(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_AddRef(surface->texture_outer);
}

static HRESULT ddraw_surface_set_palette(struct ddraw_surface *surface, IDirectDrawPalette *palette)
{
    struct ddraw_palette *palette_impl = unsafe_impl_from_IDirectDrawPalette(palette);
    struct ddraw_palette *prev;

    TRACE("iface %p, palette %p.\n", surface, palette);

    if (palette_impl && palette_impl->flags & DDPCAPS_ALPHA
            && !(surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_TEXTURE))
    {
        WARN("Alpha palette set on non-texture surface, returning DDERR_INVALIDSURFACETYPE.\n");
        return DDERR_INVALIDSURFACETYPE;
    }

    if (!format_is_paletteindexed(&surface->surface_desc.u4.ddpfPixelFormat))
        return DDERR_INVALIDPIXELFORMAT;

    wined3d_mutex_lock();

    prev = surface->palette;
    if (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        if (prev)
            prev->flags &= ~DDPCAPS_PRIMARYSURFACE;
        if (palette_impl)
            palette_impl->flags |= DDPCAPS_PRIMARYSURFACE;
        wined3d_swapchain_set_palette(surface->ddraw->wined3d_swapchain,
                palette_impl ? palette_impl->wined3d_palette : NULL);
        ddraw_surface_update_frontbuffer(surface, NULL, FALSE, 0);
    }
    if (palette_impl)
        IDirectDrawPalette_AddRef(&palette_impl->IDirectDrawPalette_iface);
    if (prev)
        IDirectDrawPalette_Release(&prev->IDirectDrawPalette_iface);
    surface->palette = palette_impl;

    wined3d_mutex_unlock();

    return DD_OK;
}

static void ddraw_surface_cleanup(struct ddraw_surface *surface)
{
    struct ddraw_surface *surf;
    UINT i;

    TRACE("surface %p.\n", surface);

    /* The refcount test shows that the palette is detached when the surface
     * is destroyed. */
    ddraw_surface_set_palette(surface, NULL);

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
        if (!surf->is_complex_root)
        {
            struct ddraw_texture *texture = wined3d_texture_get_parent(surf->wined3d_texture);
            struct wined3d_device *wined3d_device = texture->wined3d_device;
            struct ddraw_surface *root = texture->root;

            ddraw_surface_cleanup(surf);

            if (surf == root)
                wined3d_device_decref(wined3d_device);
        }
    }

    if (surface->device1)
        IUnknown_Release(&surface->device1->IUnknown_inner);

    if (surface->iface_count > 1)
    {
        /* This can happen when a complex surface is destroyed, because the
         * 2nd surface was addref()ed when the app called
         * GetAttachedSurface(). */
        WARN("Destroying surface %p with refcounts 7: %u 4: %u 3: %u 2: %u 1: %u.\n",
                surface, surface->ref7, surface->ref4, surface->ref3, surface->ref2, surface->ref1);
    }

    if (surface->wined3d_rtv)
        wined3d_rendertarget_view_decref(surface->wined3d_rtv);
    wined3d_texture_decref(surface->wined3d_texture);
}

static ULONG ddraw_surface_release_iface(struct ddraw_surface *This)
{
    ULONG iface_count;

    /* Prevent the surface from being destroyed if it's still attached to
     * another surface. It will be destroyed when the root is destroyed. */
    if (This->iface_count == 1 && This->attached_iface)
        IUnknown_AddRef(This->attached_iface);
    iface_count = InterlockedDecrement(&This->iface_count);

    TRACE("%p decreasing iface count to %u.\n", This, iface_count);

    if (iface_count == 0)
    {
        struct ddraw_texture *texture = wined3d_texture_get_parent(This->wined3d_texture);
        struct wined3d_device *wined3d_device = texture->wined3d_device;
        IUnknown *release_iface = This->ifaceToRelease;

        /* Complex attached surfaces are destroyed implicitly when the root is released */
        wined3d_mutex_lock();
        if(!This->is_complex_root)
        {
            WARN("(%p) Attempt to destroy a surface that is not a complex root\n", This);
            wined3d_mutex_unlock();
            return iface_count;
        }
        ddraw_surface_cleanup(This);
        wined3d_mutex_unlock();

        if (release_iface)
            IUnknown_Release(release_iface);
        /* Release the device only after anything that may reference it (the
         * wined3d texture and rendertarget view in particular) is released. */
        wined3d_device_decref(wined3d_device);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface7(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface4(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface3(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface2(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface(iface);
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
    struct ddraw_surface *This = impl_from_IDirectDrawGammaControl(iface);
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
    struct ddraw_surface *surface = impl_from_IDirect3DTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_Release(surface->texture_outer);
}

static ULONG WINAPI d3d_texture1_Release(IDirect3DTexture *iface)
{
    struct ddraw_surface *surface = impl_from_IDirect3DTexture(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_Release(surface->texture_outer);
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
        DDSCAPS2 *caps, IDirectDrawSurface7 **surface)
{
    struct ddraw_surface *head_surface = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_surface *surf;
    DDSCAPS2 our_caps;
    int i;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, surface);

    if (IDirectDrawSurface7_IsLost(&head_surface->IDirectDrawSurface7_iface) != DD_OK)
    {
        WARN("Surface %p is lost.\n", head_surface);

        *surface = NULL;
        return DDERR_SURFACELOST;
    }

    wined3d_mutex_lock();

    if(head_surface->version < 7)
    {
        /* Earlier dx apps put garbage into these members, clear them */
        our_caps.dwCaps = caps->dwCaps;
        our_caps.dwCaps2 = 0;
        our_caps.dwCaps3 = 0;
        our_caps.u1.dwCaps4 = 0;
    }
    else
    {
        our_caps = *caps;
    }

    TRACE("head_surface %p, looking for caps %#x, %#x, %#x, %#x.\n", head_surface, our_caps.dwCaps,
            our_caps.dwCaps2, our_caps.dwCaps3, our_caps.u1.dwCaps4); /* FIXME: Better debugging */

    for(i = 0; i < MAX_COMPLEX_ATTACHED; i++)
    {
        surf = head_surface->complex_array[i];
        if(!surf) break;

        TRACE("Surface %p, caps %#x, %#x, %#x, %#x.\n", surf,
                surf->surface_desc.ddsCaps.dwCaps,
                surf->surface_desc.ddsCaps.dwCaps2,
                surf->surface_desc.ddsCaps.dwCaps3,
                surf->surface_desc.ddsCaps.u1.dwCaps4);

        if (((surf->surface_desc.ddsCaps.dwCaps & our_caps.dwCaps) == our_caps.dwCaps) &&
            ((surf->surface_desc.ddsCaps.dwCaps2 & our_caps.dwCaps2) == our_caps.dwCaps2)) {

            /* MSDN: "This method fails if more than one surface is attached
             * that matches the capabilities requested."
             *
             * Not sure how to test this.
             */

            TRACE("head_surface %p, returning surface %p.\n", head_surface, surf);
            *surface = &surf->IDirectDrawSurface7_iface;
            ddraw_surface7_AddRef(*surface);
            wined3d_mutex_unlock();

            return DD_OK;
        }
    }

    /* Next, look at the attachment chain */
    surf = head_surface;

    while( (surf = surf->next_attached) )
    {
        TRACE("Surface %p, caps %#x, %#x, %#x, %#x.\n", surf,
                surf->surface_desc.ddsCaps.dwCaps,
                surf->surface_desc.ddsCaps.dwCaps2,
                surf->surface_desc.ddsCaps.dwCaps3,
                surf->surface_desc.ddsCaps.u1.dwCaps4);

        if (((surf->surface_desc.ddsCaps.dwCaps & our_caps.dwCaps) == our_caps.dwCaps) &&
            ((surf->surface_desc.ddsCaps.dwCaps2 & our_caps.dwCaps2) == our_caps.dwCaps2)) {

            TRACE("head_surface %p, returning surface %p.\n", head_surface, surf);
            *surface = &surf->IDirectDrawSurface7_iface;
            ddraw_surface7_AddRef(*surface);
            wined3d_mutex_unlock();
            return DD_OK;
        }
    }

    TRACE("head_surface %p, didn't find a valid surface.\n", head_surface);

    wined3d_mutex_unlock();

    *surface = NULL;
    return DDERR_NOTFOUND;
}

static HRESULT WINAPI ddraw_surface4_GetAttachedSurface(IDirectDrawSurface4 *iface,
        DDSCAPS2 *caps, IDirectDrawSurface4 **attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct ddraw_surface *attachment_impl;
    IDirectDrawSurface7 *attachment7;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    hr = ddraw_surface7_GetAttachedSurface(&surface->IDirectDrawSurface7_iface,
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct ddraw_surface *attachment_impl;
    IDirectDrawSurface7 *attachment7;
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    caps2.dwCaps  = caps->dwCaps;
    caps2.dwCaps2 = 0;
    caps2.dwCaps3 = 0;
    caps2.u1.dwCaps4 = 0;

    hr = ddraw_surface7_GetAttachedSurface(&surface->IDirectDrawSurface7_iface,
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    struct ddraw_surface *attachment_impl;
    IDirectDrawSurface7 *attachment7;
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    caps2.dwCaps  = caps->dwCaps;
    caps2.dwCaps2 = 0;
    caps2.dwCaps3 = 0;
    caps2.u1.dwCaps4 = 0;

    hr = ddraw_surface7_GetAttachedSurface(&surface->IDirectDrawSurface7_iface,
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    struct ddraw_surface *attachment_impl;
    IDirectDrawSurface7 *attachment7;
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    caps2.dwCaps  = caps->dwCaps;
    caps2.dwCaps2 = 0;
    caps2.dwCaps3 = 0;
    caps2.u1.dwCaps4 = 0;

    hr = ddraw_surface7_GetAttachedSurface(&surface->IDirectDrawSurface7_iface,
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
 *
 *****************************************************************************/
static HRESULT surface_lock(struct ddraw_surface *surface,
        RECT *rect, DDSURFACEDESC2 *surface_desc, unsigned int surface_desc_size,
        DWORD flags, HANDLE h)
{
    struct wined3d_map_desc map_desc;
    struct wined3d_box box;
    HRESULT hr = DD_OK;

    TRACE("surface %p, rect %s, surface_desc %p, surface_desc_size %u, flags %#x, h %p.\n",
            surface, wine_dbgstr_rect(rect), surface_desc, surface_desc_size, flags, h);

    /* surface->surface_desc.dwWidth and dwHeight are changeable, thus lock */
    wined3d_mutex_lock();

    /* Should I check for the handle to be NULL?
     *
     * The DDLOCK flags and the D3DLOCK flags are equal
     * for the supported values. The others are ignored by WineD3D
     */

    /* Windows zeroes this if the rect is invalid */
    surface_desc->lpSurface = NULL;

    if (rect)
    {
        if ((rect->left < 0) || (rect->top < 0)
                || (rect->left > rect->right) || (rect->right > surface->surface_desc.dwWidth)
                || (rect->top > rect->bottom) || (rect->bottom > surface->surface_desc.dwHeight))
        {
            WARN("Trying to lock an invalid rectangle, returning DDERR_INVALIDPARAMS\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }
        wined3d_box_set(&box, rect->left, rect->top, rect->right, rect->bottom, 0, 1);
    }

    if (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        hr = ddraw_surface_update_frontbuffer(surface, rect, TRUE, 0);
    if (SUCCEEDED(hr))
        hr = wined3d_resource_map(wined3d_texture_get_resource(surface->wined3d_texture),
                surface->sub_resource_idx, &map_desc, rect ? &box : NULL,
                wined3dmapflags_from_ddrawmapflags(flags));
    if (FAILED(hr))
    {
        wined3d_mutex_unlock();
        switch(hr)
        {
            /* D3D8 and D3D9 return the general D3DERR_INVALIDCALL error, but ddraw has a more
             * specific error. But since wined3d returns that error in this only occasion,
             * keep d3d8 and d3d9 free from the return value override. There are many different
             * places where d3d8/9 would have to catch the DDERR_SURFACEBUSY, it is much easier
             * to do it in one place in ddraw.
             */
            case WINED3DERR_INVALIDCALL:    return DDERR_SURFACEBUSY;
            default:                        return hr;
        }
    }

    if (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        if (flags & DDLOCK_READONLY)
            SetRectEmpty(&surface->ddraw->primary_lock);
        else if (rect)
            surface->ddraw->primary_lock = *rect;
        else
            SetRect(&surface->ddraw->primary_lock, 0, 0, surface->surface_desc.dwWidth, surface->surface_desc.dwHeight);
    }

    /* Windows does not set DDSD_LPSURFACE on locked surfaces. */
    DD_STRUCT_COPY_BYSIZE_(surface_desc, &surface->surface_desc, surface_desc_size, surface->surface_desc.dwSize);
    surface_desc->lpSurface = map_desc.data;

    TRACE("locked surface returning description :\n");
    if (TRACE_ON(ddraw))
        DDRAW_dump_surface_desc(surface_desc);

    wined3d_mutex_unlock();

    return DD_OK;
}

static BOOL surface_validate_lock_desc(struct ddraw_surface *surface,
        const DDSURFACEDESC *desc, unsigned int *size)
{
    if (!desc)
        return FALSE;

    if (desc->dwSize == sizeof(DDSURFACEDESC) || desc->dwSize == sizeof(DDSURFACEDESC2))
    {
        *size = desc->dwSize;
        return TRUE;
    }

    if (surface->version == 7
            && surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_TEXTURE
            && !(surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        if (desc->dwSize >= sizeof(DDSURFACEDESC2))
            *size = sizeof(DDSURFACEDESC2);
        else
            *size = sizeof(DDSURFACEDESC);
        return TRUE;
    }

    WARN("Invalid structure size %u.\n", desc->dwSize);
    return FALSE;
}

static HRESULT WINAPI ddraw_surface7_Lock(IDirectDrawSurface7 *iface,
        RECT *rect, DDSURFACEDESC2 *surface_desc, DWORD flags, HANDLE h)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    unsigned int surface_desc_size;

    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_validate_lock_desc(surface, (DDSURFACEDESC *)surface_desc, &surface_desc_size))
        return DDERR_INVALIDPARAMS;

    return surface_lock(surface, rect, surface_desc, surface_desc_size, flags, h);
}

static HRESULT WINAPI ddraw_surface4_Lock(IDirectDrawSurface4 *iface, RECT *rect,
        DDSURFACEDESC2 *surface_desc, DWORD flags, HANDLE h)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);
    unsigned int surface_desc_size;

    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_validate_lock_desc(surface, (DDSURFACEDESC *)surface_desc, &surface_desc_size))
        return DDERR_INVALIDPARAMS;

    return surface_lock(surface, rect, surface_desc, surface_desc_size, flags, h);
}

static HRESULT WINAPI ddraw_surface3_Lock(IDirectDrawSurface3 *iface, RECT *rect,
        DDSURFACEDESC *surface_desc, DWORD flags, HANDLE h)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    unsigned int surface_desc_size;
    DDSURFACEDESC2 surface_desc2;
    HRESULT hr;

    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_validate_lock_desc(surface, surface_desc, &surface_desc_size))
        return DDERR_INVALIDPARAMS;

    surface_desc2.dwSize = surface_desc->dwSize;
    surface_desc2.dwFlags = 0;
    hr = surface_lock(surface, rect, &surface_desc2, surface_desc_size, flags, h);
    DDSD2_to_DDSD(&surface_desc2, surface_desc);
    surface_desc->dwSize = surface_desc2.dwSize;
    return hr;
}

static HRESULT WINAPI ddraw_surface2_Lock(IDirectDrawSurface2 *iface, RECT *rect,
        DDSURFACEDESC *surface_desc, DWORD flags, HANDLE h)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    unsigned int surface_desc_size;
    DDSURFACEDESC2 surface_desc2;
    HRESULT hr;

    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_validate_lock_desc(surface, surface_desc, &surface_desc_size))
        return DDERR_INVALIDPARAMS;

    surface_desc2.dwSize = surface_desc->dwSize;
    surface_desc2.dwFlags = 0;
    hr = surface_lock(surface, rect, &surface_desc2, surface_desc_size, flags, h);
    DDSD2_to_DDSD(&surface_desc2, surface_desc);
    surface_desc->dwSize = surface_desc2.dwSize;
    return hr;
}

static HRESULT WINAPI ddraw_surface1_Lock(IDirectDrawSurface *iface, RECT *rect,
        DDSURFACEDESC *surface_desc, DWORD flags, HANDLE h)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    unsigned int surface_desc_size;
    DDSURFACEDESC2 surface_desc2;
    HRESULT hr;

    TRACE("iface %p, rect %s, surface_desc %p, flags %#x, h %p.\n",
            iface, wine_dbgstr_rect(rect), surface_desc, flags, h);

    if (!surface_validate_lock_desc(surface, surface_desc, &surface_desc_size))
        return DDERR_INVALIDPARAMS;

    surface_desc2.dwSize = surface_desc->dwSize;
    surface_desc2.dwFlags = 0;
    hr = surface_lock(surface, rect, &surface_desc2, surface_desc_size, flags, h);
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
 *  D3D_OK on success, error code otherwise.
 *
 *****************************************************************************/
static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface7_Unlock(IDirectDrawSurface7 *iface, RECT *pRect)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(pRect));

    wined3d_mutex_lock();
    hr = wined3d_resource_unmap(wined3d_texture_get_resource(surface->wined3d_texture), surface->sub_resource_idx);
    if (SUCCEEDED(hr) && surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        hr = ddraw_surface_update_frontbuffer(surface, &surface->ddraw->primary_lock, FALSE, 0);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface4_Unlock(IDirectDrawSurface4 *iface, RECT *pRect)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, rect %p.\n", iface, pRect);

    return ddraw_surface7_Unlock(&surface->IDirectDrawSurface7_iface, pRect);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface3_Unlock(IDirectDrawSurface3 *iface, void *data)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, data %p.\n", iface, data);

    /* data might not be the LPRECT of later versions, so drop it. */
    return ddraw_surface7_Unlock(&surface->IDirectDrawSurface7_iface, NULL);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface2_Unlock(IDirectDrawSurface2 *iface, void *data)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, data %p.\n", iface, data);

    /* data might not be the LPRECT of later versions, so drop it. */
    return ddraw_surface7_Unlock(&surface->IDirectDrawSurface7_iface, NULL);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface1_Unlock(IDirectDrawSurface *iface, void *data)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, data %p.\n", iface, data);

    /* data might not be the LPRECT of later versions, so drop it. */
    return ddraw_surface7_Unlock(&surface->IDirectDrawSurface7_iface, NULL);
}

static unsigned int ddraw_swap_interval_from_flags(DWORD flags)
{
    if (flags & DDFLIP_NOVSYNC)
        return 0;

    switch (flags & (DDFLIP_INTERVAL2 | DDFLIP_INTERVAL3 | DDFLIP_INTERVAL4))
    {
        case DDFLIP_INTERVAL2:
            return 2;
        case DDFLIP_INTERVAL3:
            return 3;
        case DDFLIP_INTERVAL4:
            return 4;
        default:
            return 1;
    }
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface7_Flip(IDirectDrawSurface7 *iface,
        IDirectDrawSurface7 *src, DWORD flags)
{
    struct ddraw_surface *dst_impl = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface7(src);
    struct wined3d_rendertarget_view *tmp_rtv, *src_rtv, *rtv;
    struct ddraw_texture *ddraw_texture, *prev_ddraw_texture;
    DDSCAPS2 caps = {DDSCAPS_FLIP, 0, 0, {0}};
    struct wined3d_texture *texture;
    IDirectDrawSurface7 *current;
    HRESULT hr;

    TRACE("iface %p, src %p, flags %#x.\n", iface, src, flags);

    if (src == iface || !(dst_impl->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_OVERLAY)))
        return DDERR_NOTFLIPPABLE;

    if (IDirectDrawSurface7_IsLost(iface) == DDERR_SURFACELOST)
        return DDERR_SURFACELOST;

    wined3d_mutex_lock();

    if ((dst_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            && !(dst_impl->ddraw->cooperative_level & DDSCL_EXCLUSIVE))
    {
        WARN("Not in exclusive mode.\n");
        wined3d_mutex_unlock();
        return DDERR_NOEXCLUSIVEMODE;
    }

    tmp_rtv = ddraw_surface_get_rendertarget_view(dst_impl);
    if (dst_impl->sub_resource_idx)
        ERR("Invalid sub-resource index %u on surface %p.\n", dst_impl->sub_resource_idx, dst_impl);
    texture = dst_impl->wined3d_texture;
    rtv = wined3d_device_get_rendertarget_view(dst_impl->ddraw->wined3d_device, 0);
    ddraw_texture = wined3d_texture_get_parent(dst_impl->wined3d_texture);

    if (src_impl)
    {
        for (current = iface; current != src;)
        {
            if (FAILED(hr = ddraw_surface7_GetAttachedSurface(current, &caps, &current)))
            {
                WARN("Surface %p is not on the same flip chain as surface %p.\n", src, iface);
                wined3d_mutex_unlock();
                return DDERR_NOTFLIPPABLE;
            }
            ddraw_surface7_Release(current);
            if (current == iface)
            {
                WARN("Surface %p is not on the same flip chain as surface %p.\n", src, iface);
                wined3d_mutex_unlock();
                return DDERR_NOTFLIPPABLE;
            }
        }

        src_rtv = ddraw_surface_get_rendertarget_view(src_impl);
        if (rtv == dst_impl->wined3d_rtv)
            wined3d_device_set_rendertarget_view(dst_impl->ddraw->wined3d_device, 0, src_rtv, FALSE);
        wined3d_rendertarget_view_set_parent(src_rtv, dst_impl);
        dst_impl->wined3d_rtv = src_rtv;
        wined3d_texture_set_sub_resource_parent(src_impl->wined3d_texture, 0, dst_impl);
        prev_ddraw_texture = wined3d_texture_get_parent(src_impl->wined3d_texture);
        wined3d_resource_set_parent(wined3d_texture_get_resource(src_impl->wined3d_texture), ddraw_texture);
        if (src_impl->sub_resource_idx)
            ERR("Invalid sub-resource index %u on surface %p.\n", src_impl->sub_resource_idx, src_impl);
        dst_impl->wined3d_texture = src_impl->wined3d_texture;
        ddraw_texture = prev_ddraw_texture;
    }
    else
    {
        for (current = iface;;)
        {
            if (FAILED(hr = ddraw_surface7_GetAttachedSurface(current, &caps, &current)))
            {
                ERR("Can't find a flip target\n");
                wined3d_mutex_unlock();
                return DDERR_NOTFLIPPABLE; /* Unchecked */
            }
            ddraw_surface7_Release(current);
            if (current == iface)
            {
                dst_impl = impl_from_IDirectDrawSurface7(iface);
                break;
            }

            src_impl = impl_from_IDirectDrawSurface7(current);
            src_rtv = ddraw_surface_get_rendertarget_view(src_impl);
            if (rtv == dst_impl->wined3d_rtv)
                wined3d_device_set_rendertarget_view(dst_impl->ddraw->wined3d_device, 0, src_rtv, FALSE);
            wined3d_rendertarget_view_set_parent(src_rtv, dst_impl);
            dst_impl->wined3d_rtv = src_rtv;
            wined3d_texture_set_sub_resource_parent(src_impl->wined3d_texture, 0, dst_impl);
            prev_ddraw_texture = wined3d_texture_get_parent(src_impl->wined3d_texture);
            wined3d_resource_set_parent(wined3d_texture_get_resource(src_impl->wined3d_texture), ddraw_texture);
            ddraw_texture = prev_ddraw_texture;
            if (src_impl->sub_resource_idx)
                ERR("Invalid sub-resource index %u on surface %p.\n", src_impl->sub_resource_idx, src_impl);
            dst_impl->wined3d_texture = src_impl->wined3d_texture;
            dst_impl = src_impl;
        }
    }

    /* We don't have to worry about potential texture bindings, since
     * flippable surfaces can never be textures. */
    if (rtv == src_impl->wined3d_rtv)
        wined3d_device_set_rendertarget_view(dst_impl->ddraw->wined3d_device, 0, tmp_rtv, FALSE);
    wined3d_rendertarget_view_set_parent(tmp_rtv, src_impl);
    src_impl->wined3d_rtv = tmp_rtv;
    wined3d_texture_set_sub_resource_parent(texture, 0, src_impl);
    wined3d_resource_set_parent(wined3d_texture_get_resource(texture), ddraw_texture);
    src_impl->wined3d_texture = texture;

    if (flags & ~(DDFLIP_NOVSYNC | DDFLIP_INTERVAL2 | DDFLIP_INTERVAL3 | DDFLIP_INTERVAL4))
    {
        static UINT once;
        if (!once++)
            FIXME("Ignoring flags %#x.\n", flags);
        else
            WARN("Ignoring flags %#x.\n", flags);
    }

    if (dst_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        hr = ddraw_surface_update_frontbuffer(dst_impl, NULL, FALSE, ddraw_swap_interval_from_flags(flags));
    else
        hr = DD_OK;

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface4_Flip(IDirectDrawSurface4 *iface,
        IDirectDrawSurface4 *src, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface4(src);

    TRACE("iface %p, src %p, flags %#x.\n", iface, src, flags);

    return ddraw_surface7_Flip(&surface->IDirectDrawSurface7_iface,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, flags);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface3_Flip(IDirectDrawSurface3 *iface,
        IDirectDrawSurface3 *src, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface3(src);

    TRACE("iface %p, src %p, flags %#x.\n", iface, src, flags);

    return ddraw_surface7_Flip(&surface->IDirectDrawSurface7_iface,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, flags);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface2_Flip(IDirectDrawSurface2 *iface,
        IDirectDrawSurface2 *src, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface2(src);

    TRACE("iface %p, src %p, flags %#x.\n", iface, src, flags);

    return ddraw_surface7_Flip(&surface->IDirectDrawSurface7_iface,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, flags);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface1_Flip(IDirectDrawSurface *iface,
        IDirectDrawSurface *src, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface(src);

    TRACE("iface %p, src %p, flags %#x.\n", iface, src, flags);

    return ddraw_surface7_Flip(&surface->IDirectDrawSurface7_iface,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, flags);
}

static HRESULT ddraw_surface_blt(struct ddraw_surface *dst_surface, const RECT *dst_rect,
        struct ddraw_surface *src_surface, const RECT *src_rect, DWORD flags, DWORD fill_colour,
        const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_device *wined3d_device = dst_surface->ddraw->wined3d_device;
    struct wined3d_color colour;
    DWORD wined3d_flags;

    if (flags & DDBLT_COLORFILL)
    {
        wined3d_flags = WINED3DCLEAR_TARGET;
        if (!(flags & DDBLT_ASYNC))
            wined3d_flags |= WINED3DCLEAR_SYNCHRONOUS;

        if (!wined3d_colour_from_ddraw_colour(&dst_surface->surface_desc.u4.ddpfPixelFormat,
                dst_surface->palette, fill_colour, &colour))
            return DDERR_INVALIDPARAMS;

        return wined3d_device_clear_rendertarget_view(wined3d_device,
                ddraw_surface_get_rendertarget_view(dst_surface),
                dst_rect, wined3d_flags, &colour, 0.0f, 0);
    }

    if (flags & DDBLT_DEPTHFILL)
    {
        wined3d_flags = WINED3DCLEAR_ZBUFFER;
        if (!(flags & DDBLT_ASYNC))
            wined3d_flags |= WINED3DCLEAR_SYNCHRONOUS;

        if (!wined3d_colour_from_ddraw_colour(&dst_surface->surface_desc.u4.ddpfPixelFormat,
                dst_surface->palette, fill_colour, &colour))
            return DDERR_INVALIDPARAMS;

        return wined3d_device_clear_rendertarget_view(wined3d_device,
                ddraw_surface_get_rendertarget_view(dst_surface),
                dst_rect, wined3d_flags, NULL, colour.r, 0);
    }

    wined3d_flags = flags & ~DDBLT_ASYNC;
    if (wined3d_flags & ~WINED3D_BLT_MASK)
    {
        FIXME("Unhandled flags %#x.\n", flags);
        return E_NOTIMPL;
    }

    if (!(flags & DDBLT_ASYNC))
        wined3d_flags |= WINED3D_BLT_SYNCHRONOUS;

    return wined3d_texture_blt(dst_surface->wined3d_texture, dst_surface->sub_resource_idx, dst_rect,
            src_surface->wined3d_texture, src_surface->sub_resource_idx, src_rect, wined3d_flags, fx, filter);
}

static HRESULT ddraw_surface_blt_clipped(struct ddraw_surface *dst_surface, const RECT *dst_rect_in,
        struct ddraw_surface *src_surface, const RECT *src_rect_in, DWORD flags, DWORD fill_colour,
        const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    RECT src_rect, dst_rect;
    float scale_x, scale_y;
    const RECT *clip_rect;
    UINT clip_list_size;
    RGNDATA *clip_list;
    HRESULT hr = DD_OK;
    UINT i;

    if (!dst_rect_in)
        SetRect(&dst_rect, 0, 0, dst_surface->surface_desc.dwWidth,
                dst_surface->surface_desc.dwHeight);
    else
        dst_rect = *dst_rect_in;

    if (IsRectEmpty(&dst_rect))
        return DDERR_INVALIDRECT;

    if (src_surface)
    {
        if (!src_rect_in)
            SetRect(&src_rect, 0, 0, src_surface->surface_desc.dwWidth,
                    src_surface->surface_desc.dwHeight);
        else
            src_rect = *src_rect_in;

        if (IsRectEmpty(&src_rect))
            return DDERR_INVALIDRECT;
    }
    else
    {
        SetRectEmpty(&src_rect);
    }

    if (!dst_surface->clipper)
    {
        if (src_surface && src_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            hr = ddraw_surface_update_frontbuffer(src_surface, &src_rect, TRUE, 0);
        if (SUCCEEDED(hr))
            hr = ddraw_surface_blt(dst_surface, &dst_rect, src_surface, &src_rect, flags, fill_colour, fx, filter);
        if (SUCCEEDED(hr) && (dst_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
            hr = ddraw_surface_update_frontbuffer(dst_surface, &dst_rect, FALSE, 0);

        return hr;
    }

    if (!ddraw_clipper_is_valid(dst_surface->clipper))
    {
        FIXME("Attempting to blit with an invalid clipper.\n");
        return DDERR_INVALIDPARAMS;
    }

    scale_x = (float)(src_rect.right - src_rect.left) / (float)(dst_rect.right - dst_rect.left);
    scale_y = (float)(src_rect.bottom - src_rect.top) / (float)(dst_rect.bottom - dst_rect.top);

    if (FAILED(hr = IDirectDrawClipper_GetClipList(&dst_surface->clipper->IDirectDrawClipper_iface,
            &dst_rect, NULL, &clip_list_size)))
    {
        WARN("Failed to get clip list size, hr %#x.\n", hr);
        return hr;
    }

    if (!(clip_list = heap_alloc(clip_list_size)))
    {
        WARN("Failed to allocate clip list.\n");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = IDirectDrawClipper_GetClipList(&dst_surface->clipper->IDirectDrawClipper_iface,
            &dst_rect, clip_list, &clip_list_size)))
    {
        WARN("Failed to get clip list, hr %#x.\n", hr);
        heap_free(clip_list);
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

            if (src_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            {
                if (FAILED(hr = ddraw_surface_update_frontbuffer(src_surface, &src_rect_clipped, TRUE, 0)))
                    break;
            }
        }

        if (FAILED(hr = ddraw_surface_blt(dst_surface, &clip_rect[i],
                src_surface, &src_rect_clipped, flags, fill_colour, fx, filter)))
            break;

        if (dst_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        {
            if (FAILED(hr = ddraw_surface_update_frontbuffer(dst_surface, &clip_rect[i], FALSE, 0)))
                break;
        }
    }

    heap_free(clip_list);
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
 *  D3D_OK on success, error code otherwise.
 *
 *****************************************************************************/
static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface7_Blt(IDirectDrawSurface7 *iface, RECT *dst_rect,
        IDirectDrawSurface7 *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    struct ddraw_surface *dst_impl = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface7(src_surface);
    struct wined3d_blt_fx wined3d_fx;
    DWORD unsupported_flags;
    DWORD fill_colour = 0;
    HRESULT hr = DD_OK;
    DDBLTFX rop_fx;

    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    unsupported_flags = DDBLT_ALPHADEST
            | DDBLT_ALPHADESTCONSTOVERRIDE
            | DDBLT_ALPHADESTNEG
            | DDBLT_ALPHADESTSURFACEOVERRIDE
            | DDBLT_ALPHAEDGEBLEND
            | DDBLT_ALPHASRC
            | DDBLT_ALPHASRCCONSTOVERRIDE
            | DDBLT_ALPHASRCNEG
            | DDBLT_ALPHASRCSURFACEOVERRIDE
            | DDBLT_ZBUFFER
            | DDBLT_ZBUFFERDESTCONSTOVERRIDE
            | DDBLT_ZBUFFERDESTOVERRIDE
            | DDBLT_ZBUFFERSRCCONSTOVERRIDE
            | DDBLT_ZBUFFERSRCOVERRIDE;
    if (flags & unsupported_flags)
    {
        WARN("Ignoring unsupported flags %#x.\n", flags & unsupported_flags);
        flags &= ~unsupported_flags;
    }

    if ((flags & DDBLT_KEYSRCOVERRIDE) && (!fx || flags & DDBLT_KEYSRC))
    {
        WARN("Invalid source color key parameters, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    if ((flags & DDBLT_KEYDESTOVERRIDE) && (!fx || flags & DDBLT_KEYDEST))
    {
        WARN("Invalid destination color key parameters, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    if (flags & DDBLT_DDROPS)
    {
        FIXME("DDBLT_DDROPS not implemented.\n");
        if (fx)
            FIXME("    rop %#x, pattern %p.\n", fx->dwDDROP, fx->u5.lpDDSPattern);
        return DDERR_NORASTEROPHW;
    }

    wined3d_mutex_lock();

    if (flags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL))
    {
        if (flags & DDBLT_ROP)
        {
            wined3d_mutex_unlock();
            WARN("DDBLT_ROP used with DDBLT_COLORFILL or DDBLT_DEPTHFILL, returning DDERR_INVALIDPARAMS.\n");
            return DDERR_INVALIDPARAMS;
        }
        if (src_impl)
        {
            wined3d_mutex_unlock();
            WARN("Depth or colorfill is not compatible with source surfaces, returning DDERR_INVALIDPARAMS\n");
            return DDERR_INVALIDPARAMS;
        }
        if (!fx)
        {
            wined3d_mutex_unlock();
            WARN("Depth or colorfill used with NULL fx, returning DDERR_INVALIDPARAMS.\n");
            return DDERR_INVALIDPARAMS;
        }

        if ((flags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL)) == (DDBLT_COLORFILL | DDBLT_DEPTHFILL))
            flags &= ~DDBLT_DEPTHFILL;

        if ((dst_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) && (flags & DDBLT_COLORFILL))
        {
            wined3d_mutex_unlock();
            WARN("DDBLT_COLORFILL used on a depth buffer, returning DDERR_INVALIDPARAMS.\n");
            return DDERR_INVALIDPARAMS;
        }
        if (!(dst_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) && (flags & DDBLT_DEPTHFILL))
        {
            wined3d_mutex_unlock();
            WARN("DDBLT_DEPTHFILL used on a color buffer, returning DDERR_INVALIDPARAMS.\n");
            return DDERR_INVALIDPARAMS;
        }
    }

    if (flags & DDBLT_ROP)
    {
        if (!fx)
        {
            wined3d_mutex_unlock();
            WARN("DDBLT_ROP used with NULL fx, returning DDERR_INVALIDPARAMS.\n");
            return DDERR_INVALIDPARAMS;
        }

        if (src_impl && src_rect
                && ((ULONG)src_rect->left >= src_rect->right || src_rect->right > src_impl->surface_desc.dwWidth
                || (ULONG)src_rect->top >= src_rect->bottom || src_rect->bottom > src_impl->surface_desc.dwHeight))
        {
            wined3d_mutex_unlock();
            WARN("Invalid source rectangle.\n");
            return DDERR_INVALIDRECT;
        }

        flags &= ~DDBLT_ROP;
        switch (fx->dwROP)
        {
            case SRCCOPY:
                break;

            case WHITENESS:
            case BLACKNESS:
                rop_fx = *fx;

                if (fx->dwROP == WHITENESS)
                    rop_fx.u5.dwFillColor = 0xffffffff;
                else
                    rop_fx.u5.dwFillColor = 0;

                if (dst_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
                    flags |= DDBLT_DEPTHFILL;
                else
                    flags |= DDBLT_COLORFILL;

                fx = &rop_fx;
                break;

            default:
                wined3d_mutex_unlock();
                WARN("Unsupported ROP %#x used, returning DDERR_NORASTEROPHW.\n", fx->dwROP);
                return DDERR_NORASTEROPHW;
        }
    }

    if (!(flags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL)) && !src_impl)
    {
        WARN("No source surface.\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if (flags & DDBLT_KEYSRC && (!src_impl || !(src_impl->surface_desc.dwFlags & DDSD_CKSRCBLT)))
    {
        WARN("DDBLT_KEYSRC blit without color key in surface, returning DDERR_INVALIDPARAMS\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }
    if (flags & DDBLT_KEYDEST && !(dst_impl->surface_desc.dwFlags & DDSD_CKDESTBLT))
    {
        WARN("DDBLT_KEYDEST blit without color key in surface, returning DDERR_INVALIDPARAMS\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if (fx)
    {
        wined3d_fx.fx = fx->dwDDFX;
        fill_colour = fx->u5.dwFillColor;
        wined3d_fx.dst_color_key.color_space_low_value = fx->ddckDestColorkey.dwColorSpaceLowValue;
        wined3d_fx.dst_color_key.color_space_high_value = fx->ddckDestColorkey.dwColorSpaceHighValue;
        wined3d_fx.src_color_key.color_space_low_value = fx->ddckSrcColorkey.dwColorSpaceLowValue;
        wined3d_fx.src_color_key.color_space_high_value = fx->ddckSrcColorkey.dwColorSpaceHighValue;
    }

    hr = ddraw_surface_blt_clipped(dst_impl, dst_rect, src_impl,
            src_rect, flags, fill_colour, fx ? &wined3d_fx : NULL, WINED3D_TEXF_LINEAR);

    wined3d_mutex_unlock();
    switch(hr)
    {
        case WINED3DERR_NOTAVAILABLE:       return DDERR_UNSUPPORTED;
        default:                            return hr;
    }
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface4_Blt(IDirectDrawSurface4 *iface, RECT *dst_rect,
        IDirectDrawSurface4 *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    struct ddraw_surface *dst = impl_from_IDirectDrawSurface4(iface);
    struct ddraw_surface *src = unsafe_impl_from_IDirectDrawSurface4(src_surface);

    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return ddraw_surface7_Blt(&dst->IDirectDrawSurface7_iface, dst_rect,
            src ? &src->IDirectDrawSurface7_iface : NULL, src_rect, flags, fx);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface3_Blt(IDirectDrawSurface3 *iface, RECT *dst_rect,
        IDirectDrawSurface3 *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    struct ddraw_surface *dst = impl_from_IDirectDrawSurface3(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface3(src_surface);

    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return ddraw_surface7_Blt(&dst->IDirectDrawSurface7_iface, dst_rect,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags, fx);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface2_Blt(IDirectDrawSurface2 *iface, RECT *dst_rect,
        IDirectDrawSurface2 *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    struct ddraw_surface *dst = impl_from_IDirectDrawSurface2(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface2(src_surface);

    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return ddraw_surface7_Blt(&dst->IDirectDrawSurface7_iface, dst_rect,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags, fx);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface1_Blt(IDirectDrawSurface *iface, RECT *dst_rect,
        IDirectDrawSurface *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    struct ddraw_surface *dst = impl_from_IDirectDrawSurface(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface(src_surface);

    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return ddraw_surface7_Blt(&dst->IDirectDrawSurface7_iface, dst_rect,
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
static HRESULT ddraw_surface_attach_surface(struct ddraw_surface *This, struct ddraw_surface *Surf)
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
    if (This->ddraw->d3ddevice)
        d3d_device_update_depth_stencil(This->ddraw->d3ddevice);

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface7_AddAttachedSurface(IDirectDrawSurface7 *iface, IDirectDrawSurface7 *attachment)
{
    struct ddraw_surface *This = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface7(attachment);
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
    attachment_impl->attached_iface = (IUnknown *)attachment;
    IUnknown_AddRef(attachment_impl->attached_iface);
    return hr;
}

static HRESULT WINAPI ddraw_surface4_AddAttachedSurface(IDirectDrawSurface4 *iface, IDirectDrawSurface4 *attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface4(attachment);
    HRESULT hr;

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    /* Tests suggest that
     * -> offscreen plain surfaces can be attached to other offscreen plain surfaces
     * -> offscreen plain surfaces can be attached to primaries
     * -> primaries can be attached to offscreen plain surfaces
     * -> z buffers can be attached to primaries */
    if (surface->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OFFSCREENPLAIN)
            && attachment_impl->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OFFSCREENPLAIN))
    {
        /* Sizes have to match */
        if (attachment_impl->surface_desc.dwWidth != surface->surface_desc.dwWidth
                || attachment_impl->surface_desc.dwHeight != surface->surface_desc.dwHeight)
        {
            WARN("Surface sizes do not match.\n");
            return DDERR_CANNOTATTACHSURFACE;
        }
    }
    else if (!(surface->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE))
            || !(attachment_impl->surface_desc.ddsCaps.dwCaps & (DDSCAPS_ZBUFFER)))
    {
        WARN("Invalid attachment combination.\n");
        return DDERR_CANNOTATTACHSURFACE;
    }

    if (FAILED(hr = ddraw_surface_attach_surface(surface, attachment_impl)))
        return hr;

    attachment_impl->attached_iface = (IUnknown *)attachment;
    IUnknown_AddRef(attachment_impl->attached_iface);
    return hr;
}

static HRESULT WINAPI ddraw_surface3_AddAttachedSurface(IDirectDrawSurface3 *iface, IDirectDrawSurface3 *attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface3(attachment);
    HRESULT hr;

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    if (FAILED(hr = ddraw_surface4_AddAttachedSurface(&surface->IDirectDrawSurface4_iface,
            attachment_impl ? &attachment_impl->IDirectDrawSurface4_iface : NULL)))
        return hr;

    attachment_impl->attached_iface = (IUnknown *)attachment;
    IUnknown_AddRef(attachment_impl->attached_iface);
    ddraw_surface4_Release(&attachment_impl->IDirectDrawSurface4_iface);
    return hr;
}

static HRESULT WINAPI ddraw_surface2_AddAttachedSurface(IDirectDrawSurface2 *iface, IDirectDrawSurface2 *attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface2(attachment);
    HRESULT hr;

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    if (FAILED(hr = ddraw_surface4_AddAttachedSurface(&surface->IDirectDrawSurface4_iface,
            attachment_impl ? &attachment_impl->IDirectDrawSurface4_iface : NULL)))
        return hr;

    attachment_impl->attached_iface = (IUnknown *)attachment;
    IUnknown_AddRef(attachment_impl->attached_iface);
    ddraw_surface4_Release(&attachment_impl->IDirectDrawSurface4_iface);
    return hr;
}

static HRESULT WINAPI ddraw_surface1_AddAttachedSurface(IDirectDrawSurface *iface, IDirectDrawSurface *attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface(attachment);
    HRESULT hr;

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    if (FAILED(hr = ddraw_surface4_AddAttachedSurface(&surface->IDirectDrawSurface4_iface,
            attachment_impl ? &attachment_impl->IDirectDrawSurface4_iface : NULL)))
        return hr;

    attachment_impl->attached_iface = (IUnknown *)attachment;
    IUnknown_AddRef(attachment_impl->attached_iface);
    ddraw_surface4_Release(&attachment_impl->IDirectDrawSurface4_iface);
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
static HRESULT ddraw_surface_delete_attached_surface(struct ddraw_surface *surface,
        struct ddraw_surface *attachment, IUnknown *detach_iface)
{
    struct ddraw_surface *prev = surface;

    TRACE("surface %p, attachment %p, detach_iface %p.\n", surface, attachment, detach_iface);

    wined3d_mutex_lock();
    if (!attachment || (attachment->first_attached != surface) || (attachment == surface) )
    {
        wined3d_mutex_unlock();
        return DDERR_CANNOTDETACHSURFACE;
    }

    if (attachment->attached_iface != detach_iface)
    {
        WARN("attachment->attach_iface %p != detach_iface %p.\n", attachment->attached_iface, detach_iface);
        wined3d_mutex_unlock();
        return DDERR_SURFACENOTATTACHED;
    }

    /* Remove MIPMAPSUBLEVEL if this seemed to be one */
    if (surface->surface_desc.ddsCaps.dwCaps & attachment->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        attachment->surface_desc.ddsCaps.dwCaps2 &= ~DDSCAPS2_MIPMAPSUBLEVEL;
        /* FIXME: we should probably also subtract from dwMipMapCount of this
         * and all parent surfaces */
    }

    /* Find the predecessor of the detached surface */
    while (prev->next_attached != attachment)
    {
        if (!(prev = prev->next_attached))
        {
            ERR("Failed to find predecessor of %p.\n", attachment);
            wined3d_mutex_unlock();
            return DDERR_SURFACENOTATTACHED;
        }
    }

    /* Unchain the surface */
    prev->next_attached = attachment->next_attached;
    attachment->next_attached = NULL;
    attachment->first_attached = attachment;

    /* Check if the wined3d depth stencil needs updating. Note that we don't
     * just call d3d_device_update_depth_stencil() here since it uses
     * QueryInterface(). Some applications, SCP - Containment Breach in
     * particular, modify the QueryInterface() pointer in the surface vtbl
     * but don't cleanup properly after the relevant dll is unloaded. */
    if (attachment->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER
            && wined3d_device_get_depth_stencil_view(surface->ddraw->wined3d_device) == attachment->wined3d_rtv)
        wined3d_device_set_depth_stencil_view(surface->ddraw->wined3d_device, NULL);
    wined3d_mutex_unlock();

    /* Set attached_iface to NULL before releasing it, the surface may go
     * away. */
    attachment->attached_iface = NULL;
    IUnknown_Release(detach_iface);

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface7_DeleteAttachedSurface(IDirectDrawSurface7 *iface,
        DWORD flags, IDirectDrawSurface7 *attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface7(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(surface, attachment_impl, (IUnknown *)attachment);
}

static HRESULT WINAPI ddraw_surface4_DeleteAttachedSurface(IDirectDrawSurface4 *iface,
        DWORD flags, IDirectDrawSurface4 *attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface4(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(surface, attachment_impl, (IUnknown *)attachment);
}

static HRESULT WINAPI ddraw_surface3_DeleteAttachedSurface(IDirectDrawSurface3 *iface,
        DWORD flags, IDirectDrawSurface3 *attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface3(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(surface, attachment_impl, (IUnknown *)attachment);
}

static HRESULT WINAPI ddraw_surface2_DeleteAttachedSurface(IDirectDrawSurface2 *iface,
        DWORD flags, IDirectDrawSurface2 *attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface2(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(surface, attachment_impl, (IUnknown *)attachment);
}

static HRESULT WINAPI ddraw_surface1_DeleteAttachedSurface(IDirectDrawSurface *iface,
        DWORD flags, IDirectDrawSurface *attachment)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    struct ddraw_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface(attachment);

    TRACE("iface %p, flags %#x, attachment %p.\n", iface, flags, attachment);

    return ddraw_surface_delete_attached_surface(surface, attachment_impl, (IUnknown *)attachment);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return ddraw_surface7_AddOverlayDirtyRect(&surface->IDirectDrawSurface7_iface, rect);
}

static HRESULT WINAPI ddraw_surface3_AddOverlayDirtyRect(IDirectDrawSurface3 *iface, RECT *rect)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return ddraw_surface7_AddOverlayDirtyRect(&surface->IDirectDrawSurface7_iface, rect);
}

static HRESULT WINAPI ddraw_surface2_AddOverlayDirtyRect(IDirectDrawSurface2 *iface, RECT *rect)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return ddraw_surface7_AddOverlayDirtyRect(&surface->IDirectDrawSurface7_iface, rect);
}

static HRESULT WINAPI ddraw_surface1_AddOverlayDirtyRect(IDirectDrawSurface *iface, RECT *rect)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return ddraw_surface7_AddOverlayDirtyRect(&surface->IDirectDrawSurface7_iface, rect);
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
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_surface7_GetDC(IDirectDrawSurface7 *iface, HDC *dc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr = DD_OK;

    TRACE("iface %p, dc %p.\n", iface, dc);

    if (!dc)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if (surface->dc)
        hr = DDERR_DCALREADYCREATED;
    else if (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        hr = ddraw_surface_update_frontbuffer(surface, NULL, TRUE, 0);
    if (SUCCEEDED(hr))
        hr = wined3d_texture_get_dc(surface->wined3d_texture, surface->sub_resource_idx, dc);

    if (SUCCEEDED(hr))
    {
        surface->dc = *dc;

        if (format_is_paletteindexed(&surface->surface_desc.u4.ddpfPixelFormat))
        {
            const struct ddraw_palette *palette;

            if (surface->palette)
                palette = surface->palette;
            else if (surface->ddraw->primary)
                palette = surface->ddraw->primary->palette;
            else
                palette = NULL;

            if (palette)
                wined3d_palette_apply_to_dc(palette->wined3d_palette, *dc);
        }
    }

    wined3d_mutex_unlock();
    switch (hr)
    {
        /* Some, but not all errors set *dc to NULL. E.g. DCALREADYCREATED
         * does not touch *dc. */
        case WINED3DERR_INVALIDCALL:
            *dc = NULL;
            return DDERR_CANTCREATEDC;

        default:
            return hr;
    }
}

static HRESULT WINAPI ddraw_surface4_GetDC(IDirectDrawSurface4 *iface, HDC *dc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_GetDC(&surface->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface3_GetDC(IDirectDrawSurface3 *iface, HDC *dc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_GetDC(&surface->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface2_GetDC(IDirectDrawSurface2 *iface, HDC *dc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_GetDC(&surface->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface1_GetDC(IDirectDrawSurface *iface, HDC *dc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_GetDC(&surface->IDirectDrawSurface7_iface, dc);
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
 *  DD_OK on success, error code otherwise.
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_surface7_ReleaseDC(IDirectDrawSurface7 *iface, HDC hdc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, dc %p.\n", iface, hdc);

    wined3d_mutex_lock();
    if (!surface->dc)
    {
        hr = DDERR_NODC;
    }
    else if (SUCCEEDED(hr = wined3d_texture_release_dc(surface->wined3d_texture, surface->sub_resource_idx, hdc)))
    {
        surface->dc = NULL;
        if (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            hr = ddraw_surface_update_frontbuffer(surface, NULL, FALSE, 0);
    }
    wined3d_mutex_unlock();


    return hr;
}

static HRESULT WINAPI ddraw_surface4_ReleaseDC(IDirectDrawSurface4 *iface, HDC dc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_ReleaseDC(&surface->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface3_ReleaseDC(IDirectDrawSurface3 *iface, HDC dc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_ReleaseDC(&surface->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface2_ReleaseDC(IDirectDrawSurface2 *iface, HDC dc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_ReleaseDC(&surface->IDirectDrawSurface7_iface, dc);
}

static HRESULT WINAPI ddraw_surface1_ReleaseDC(IDirectDrawSurface *iface, HDC dc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddraw_surface7_ReleaseDC(&surface->IDirectDrawSurface7_iface, dc);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, caps %p.\n", iface, Caps);

    if(!Caps)
        return DDERR_INVALIDPARAMS;

    *Caps = surface->surface_desc.ddsCaps;

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetCaps(IDirectDrawSurface4 *iface, DDSCAPS2 *caps)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, caps %p.\n", iface, caps);

    return ddraw_surface7_GetCaps(&surface->IDirectDrawSurface7_iface, caps);
}

static HRESULT WINAPI ddraw_surface3_GetCaps(IDirectDrawSurface3 *iface, DDSCAPS *caps)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, caps);

    hr = ddraw_surface7_GetCaps(&surface->IDirectDrawSurface7_iface, &caps2);
    if (FAILED(hr)) return hr;

    caps->dwCaps = caps2.dwCaps;
    return hr;
}

static HRESULT WINAPI ddraw_surface2_GetCaps(IDirectDrawSurface2 *iface, DDSCAPS *caps)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, caps);

    hr = ddraw_surface7_GetCaps(&surface->IDirectDrawSurface7_iface, &caps2);
    if (FAILED(hr)) return hr;

    caps->dwCaps = caps2.dwCaps;
    return hr;
}

static HRESULT WINAPI ddraw_surface1_GetCaps(IDirectDrawSurface *iface, DDSCAPS *caps)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, caps);

    hr = ddraw_surface7_GetCaps(&surface->IDirectDrawSurface7_iface, &caps2);
    if (FAILED(hr)) return hr;

    caps->dwCaps = caps2.dwCaps;
    return hr;
}

static HRESULT WINAPI ddraw_surface7_SetPriority(IDirectDrawSurface7 *iface, DWORD priority)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    DWORD managed = DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE;
    HRESULT hr;
    struct wined3d_resource *resource;

    TRACE("iface %p, priority %u.\n", iface, priority);

    wined3d_mutex_lock();
    /* No need to check for offscreen plain surfaces or mipmap sublevels. SetPriority
     * calls on such surfaces segfault on Windows. */
    if (!(surface->surface_desc.ddsCaps.dwCaps2 & managed))
    {
        WARN("Called on non-managed texture returning DDERR_INVALIDPARAMS.\n");
        hr = DDERR_INVALIDPARAMS;
    }
    else
    {
        resource = wined3d_texture_get_resource(surface->wined3d_texture);
        wined3d_resource_set_priority(resource, priority);
        hr = DD_OK;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface7_GetPriority(IDirectDrawSurface7 *iface, DWORD *priority)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    const struct wined3d_resource *resource;
    DWORD managed = DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE;
    HRESULT hr;

    TRACE("iface %p, priority %p.\n", iface, priority);

    wined3d_mutex_lock();
    if (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)
    {
        WARN("Called on offscreenplain surface, returning DDERR_INVALIDOBJECT.\n");
        hr = DDERR_INVALIDOBJECT;
    }
    else if (!(surface->surface_desc.ddsCaps.dwCaps2 & managed) || !surface->is_complex_root)
    {
        WARN("Called on non-managed texture or non-root surface, returning DDERR_INVALIDPARAMS.\n");
        hr = DDERR_INVALIDPARAMS;
    }
    else
    {
        resource = wined3d_texture_get_resource(surface->wined3d_texture);
        *priority = wined3d_resource_get_priority(resource);
        hr = DD_OK;
    }
    wined3d_mutex_unlock();

    return hr;
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
 *  D3D_OK on success, error code otherwise.
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_surface7_SetPrivateData(IDirectDrawSurface7 *iface,
        REFGUID tag, void *data, DWORD size, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, tag %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(tag), data, size, flags);

    if (!data)
    {
        WARN("data is NULL, returning DDERR_INVALIDPARAMS.\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    hr = wined3d_private_store_set_private_data(&surface->private_store, tag, data, size, flags);
    wined3d_mutex_unlock();
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI ddraw_surface4_SetPrivateData(IDirectDrawSurface4 *iface,
        REFGUID tag, void *data, DWORD size, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, tag %s, data %p, data_size %u, flags %#x.\n",
                iface, debugstr_guid(tag), data, size, flags);

    return ddraw_surface7_SetPrivateData(&surface->IDirectDrawSurface7_iface, tag, data, size, flags);
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
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_surface7_GetPrivateData(IDirectDrawSurface7 *iface, REFGUID tag, void *data, DWORD *size)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    const struct wined3d_private_data *stored_data;
    HRESULT hr;

    TRACE("iface %p, tag %s, data %p, data_size %p.\n",
            iface, debugstr_guid(tag), data, size);

    wined3d_mutex_lock();
    stored_data = wined3d_private_store_get_private_data(&surface->private_store, tag);
    if (!stored_data)
    {
        hr = DDERR_NOTFOUND;
        goto done;
    }
    if (!size)
    {
        hr = DDERR_INVALIDPARAMS;
        goto done;
    }
    if (*size < stored_data->size)
    {
        *size = stored_data->size;
        hr = DDERR_MOREDATA;
        goto done;
    }
    if (!data)
    {
        hr = DDERR_INVALIDPARAMS;
        goto done;
    }

    *size = stored_data->size;
    memcpy(data, stored_data->content.data, stored_data->size);
    hr = DD_OK;

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI ddraw_surface4_GetPrivateData(IDirectDrawSurface4 *iface, REFGUID tag, void *data, DWORD *size)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, tag %s, data %p, data_size %p.\n",
                iface, debugstr_guid(tag), data, size);

    return ddraw_surface7_GetPrivateData(&surface->IDirectDrawSurface7_iface, tag, data, size);
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
 *  D3D_OK on success, error code otherwise.
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_surface7_FreePrivateData(IDirectDrawSurface7 *iface, REFGUID tag)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    struct wined3d_private_data *entry;

    TRACE("iface %p, tag %s.\n", iface, debugstr_guid(tag));

    wined3d_mutex_lock();
    entry = wined3d_private_store_get_private_data(&surface->private_store, tag);
    if (!entry)
    {
        wined3d_mutex_unlock();
        return DDERR_NOTFOUND;
    }

    wined3d_private_store_free_private_data(&surface->private_store, entry);
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_FreePrivateData(IDirectDrawSurface4 *iface, REFGUID tag)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, tag %s.\n", iface, debugstr_guid(tag));

    return ddraw_surface7_FreePrivateData(&surface->IDirectDrawSurface7_iface, tag);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageLock(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_PageLock(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageLock(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_PageLock(IDirectDrawSurface2 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageLock(&surface->IDirectDrawSurface7_iface, flags);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageUnlock(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_PageUnlock(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageUnlock(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_PageUnlock(IDirectDrawSurface2 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_PageUnlock(&surface->IDirectDrawSurface7_iface, flags);
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
static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface7_BltBatch(IDirectDrawSurface7 *iface, DDBLTBATCH *Batch, DWORD Count, DWORD Flags)
{
    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, Batch, Count, Flags);

    /* MSDN: "not currently implemented" */
    return DDERR_UNSUPPORTED;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface4_BltBatch(IDirectDrawSurface4 *iface, DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, batch, count, flags);

    return ddraw_surface7_BltBatch(&surface->IDirectDrawSurface7_iface, batch, count, flags);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface3_BltBatch(IDirectDrawSurface3 *iface, DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, batch, count, flags);

    return ddraw_surface7_BltBatch(&surface->IDirectDrawSurface7_iface, batch, count, flags);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface2_BltBatch(IDirectDrawSurface2 *iface, DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, batch, count, flags);

    return ddraw_surface7_BltBatch(&surface->IDirectDrawSurface7_iface, batch, count, flags);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface1_BltBatch(IDirectDrawSurface *iface, DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, batch %p, count %u, flags %#x.\n", iface, batch, count, flags);

    return ddraw_surface7_BltBatch(&surface->IDirectDrawSurface7_iface, batch, count, flags);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_surface *surf;
    DDSURFACEDESC2 desc;
    int i;

    /* Attached surfaces aren't handled in WineD3D */
    TRACE("iface %p, context %p, callback %p.\n", iface, context, cb);

    if(!cb)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    for(i = 0; i < MAX_COMPLEX_ATTACHED; i++)
    {
        surf = surface->complex_array[i];
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

    for (surf = surface->next_attached; surf != NULL; surf = surf->next_attached)
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
    struct ddraw_surface *surface_impl = impl_from_IDirectDrawSurface7(surface);
    const struct callback_info2 *info = context;

    ddraw_surface4_AddRef(&surface_impl->IDirectDrawSurface4_iface);
    ddraw_surface7_Release(surface);

    return info->callback(&surface_impl->IDirectDrawSurface4_iface, surface_desc, info->context);
}

static HRESULT CALLBACK EnumCallback(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *surface_desc, void *context)
{
    struct ddraw_surface *surface_impl = impl_from_IDirectDrawSurface7(surface);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct callback_info2 info;

    TRACE("iface %p, context %p, callback %p.\n", iface, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumAttachedSurfaces(&surface->IDirectDrawSurface7_iface,
            &info, EnumCallback2);
}

static HRESULT WINAPI ddraw_surface3_EnumAttachedSurfaces(IDirectDrawSurface3 *iface,
        void *context, LPDDENUMSURFACESCALLBACK callback)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct callback_info info;

    TRACE("iface %p, context %p, callback %p.\n", iface, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumAttachedSurfaces(&surface->IDirectDrawSurface7_iface,
            &info, EnumCallback);
}

static HRESULT WINAPI ddraw_surface2_EnumAttachedSurfaces(IDirectDrawSurface2 *iface,
        void *context, LPDDENUMSURFACESCALLBACK callback)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    struct callback_info info;

    TRACE("iface %p, context %p, callback %p.\n", iface, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumAttachedSurfaces(&surface->IDirectDrawSurface7_iface,
            &info, EnumCallback);
}

static HRESULT WINAPI ddraw_surface1_EnumAttachedSurfaces(IDirectDrawSurface *iface,
        void *context, LPDDENUMSURFACESCALLBACK callback)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    struct callback_info info;

    TRACE("iface %p, context %p, callback %p.\n", iface, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumAttachedSurfaces(&surface->IDirectDrawSurface7_iface,
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct callback_info2 info;

    TRACE("iface %p, flags %#x, context %p, callback %p.\n", iface, flags, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumOverlayZOrders(&surface->IDirectDrawSurface7_iface,
            flags, &info, EnumCallback2);
}

static HRESULT WINAPI ddraw_surface3_EnumOverlayZOrders(IDirectDrawSurface3 *iface,
        DWORD flags, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct callback_info info;

    TRACE("iface %p, flags %#x, context %p, callback %p.\n", iface, flags, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumOverlayZOrders(&surface->IDirectDrawSurface7_iface,
            flags, &info, EnumCallback);
}

static HRESULT WINAPI ddraw_surface2_EnumOverlayZOrders(IDirectDrawSurface2 *iface,
        DWORD flags, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    struct callback_info info;

    TRACE("iface %p, flags %#x, context %p, callback %p.\n", iface, flags, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumOverlayZOrders(&surface->IDirectDrawSurface7_iface,
            flags, &info, EnumCallback);
}

static HRESULT WINAPI ddraw_surface1_EnumOverlayZOrders(IDirectDrawSurface *iface,
        DWORD flags, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    struct callback_info info;

    TRACE("iface %p, flags %#x, context %p, callback %p.\n", iface, flags, context, callback);

    info.callback = callback;
    info.context  = context;

    return ddraw_surface7_EnumOverlayZOrders(&surface->IDirectDrawSurface7_iface,
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
 *****************************************************************************/
static HRESULT WINAPI ddraw_surface7_GetBltStatus(IDirectDrawSurface7 *iface, DWORD Flags)
{
    TRACE("iface %p, flags %#x.\n", iface, Flags);

    switch (Flags)
    {
        case WINEDDGBS_CANBLT:
        case WINEDDGBS_ISBLTDONE:
            return DD_OK;

        default:
            return DDERR_INVALIDPARAMS;
    }
}

static HRESULT WINAPI ddraw_surface4_GetBltStatus(IDirectDrawSurface4 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetBltStatus(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_GetBltStatus(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetBltStatus(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_GetBltStatus(IDirectDrawSurface2 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetBltStatus(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface1_GetBltStatus(IDirectDrawSurface *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetBltStatus(&surface->IDirectDrawSurface7_iface, flags);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface7(iface);

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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_GetColorKey(&surface->IDirectDrawSurface7_iface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface3_GetColorKey(IDirectDrawSurface3 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_GetColorKey(&surface->IDirectDrawSurface7_iface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface2_GetColorKey(IDirectDrawSurface2 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_GetColorKey(&surface->IDirectDrawSurface7_iface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface1_GetColorKey(IDirectDrawSurface *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface7_GetColorKey(&surface->IDirectDrawSurface7_iface, flags, color_key);
}

/*****************************************************************************
 * IDirectDrawSurface7::GetFlipStatus
 *
 * Returns the flipping status of the surface
 *
 * Params:
 *  Flags: DDGFS_CANFLIP of DDGFS_ISFLIPDONE
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_surface7_GetFlipStatus(IDirectDrawSurface7 *iface, DWORD Flags)
{
    TRACE("iface %p, flags %#x.\n", iface, Flags);

    /* XXX: DDERR_INVALIDSURFACETYPE */

    switch (Flags)
    {
        case WINEDDGFS_CANFLIP:
        case WINEDDGFS_ISFLIPDONE:
            return DD_OK;

        default:
            return DDERR_INVALIDPARAMS;
    }
}

static HRESULT WINAPI ddraw_surface4_GetFlipStatus(IDirectDrawSurface4 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetFlipStatus(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_GetFlipStatus(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetFlipStatus(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_GetFlipStatus(IDirectDrawSurface2 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetFlipStatus(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface1_GetFlipStatus(IDirectDrawSurface *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_GetFlipStatus(&surface->IDirectDrawSurface7_iface, flags);
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
static HRESULT WINAPI ddraw_surface7_GetOverlayPosition(IDirectDrawSurface7 *iface, LONG *x, LONG *y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    wined3d_mutex_lock();
    hr = wined3d_texture_get_overlay_position(surface->wined3d_texture,
            surface->sub_resource_idx, x, y);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_GetOverlayPosition(IDirectDrawSurface4 *iface, LONG *x, LONG *y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return ddraw_surface7_GetOverlayPosition(&surface->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface3_GetOverlayPosition(IDirectDrawSurface3 *iface, LONG *x, LONG *y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return ddraw_surface7_GetOverlayPosition(&surface->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface2_GetOverlayPosition(IDirectDrawSurface2 *iface, LONG *x, LONG *y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return ddraw_surface7_GetOverlayPosition(&surface->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface1_GetOverlayPosition(IDirectDrawSurface *iface, LONG *x, LONG *y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return ddraw_surface7_GetOverlayPosition(&surface->IDirectDrawSurface7_iface, x, y);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, pixel_format %p.\n", iface, PixelFormat);

    if(!PixelFormat)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    DD_STRUCT_COPY_BYSIZE(PixelFormat, &surface->surface_desc.u4.ddpfPixelFormat);
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetPixelFormat(IDirectDrawSurface4 *iface, DDPIXELFORMAT *pixel_format)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return ddraw_surface7_GetPixelFormat(&surface->IDirectDrawSurface7_iface, pixel_format);
}

static HRESULT WINAPI ddraw_surface3_GetPixelFormat(IDirectDrawSurface3 *iface, DDPIXELFORMAT *pixel_format)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return ddraw_surface7_GetPixelFormat(&surface->IDirectDrawSurface7_iface, pixel_format);
}

static HRESULT WINAPI ddraw_surface2_GetPixelFormat(IDirectDrawSurface2 *iface, DDPIXELFORMAT *pixel_format)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return ddraw_surface7_GetPixelFormat(&surface->IDirectDrawSurface7_iface, pixel_format);
}

static HRESULT WINAPI ddraw_surface1_GetPixelFormat(IDirectDrawSurface *iface, DDPIXELFORMAT *pixel_format)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return ddraw_surface7_GetPixelFormat(&surface->IDirectDrawSurface7_iface, pixel_format);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    if(!DDSD)
        return DDERR_INVALIDPARAMS;

    if (DDSD->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Incorrect struct size %d, returning DDERR_INVALIDPARAMS\n",DDSD->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    DD_STRUCT_COPY_BYSIZE(DDSD, &surface->surface_desc);
    TRACE("Returning surface desc:\n");
    if (TRACE_ON(ddraw)) DDRAW_dump_surface_desc(DDSD);
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetSurfaceDesc(IDirectDrawSurface4 *iface, DDSURFACEDESC2 *DDSD)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    return ddraw_surface7_GetSurfaceDesc(&surface->IDirectDrawSurface7_iface, DDSD);
}

static HRESULT WINAPI ddraw_surface3_GetSurfaceDesc(IDirectDrawSurface3 *iface, DDSURFACEDESC *surface_desc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, surface_desc);

    if (!surface_desc) return DDERR_INVALIDPARAMS;

    if (surface_desc->dwSize != sizeof(DDSURFACEDESC))
    {
        WARN("Incorrect structure size %u, returning DDERR_INVALIDPARAMS.\n", surface_desc->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    DDSD2_to_DDSD(&surface->surface_desc, surface_desc);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    return ddraw_surface3_GetSurfaceDesc(&surface->IDirectDrawSurface3_iface, DDSD);
}

static HRESULT WINAPI ddraw_surface1_GetSurfaceDesc(IDirectDrawSurface *iface, DDSURFACEDESC *DDSD)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    return ddraw_surface3_GetSurfaceDesc(&surface->IDirectDrawSurface3_iface, DDSD);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, ddraw %p, surface_desc %p.\n", iface, ddraw, surface_desc);

    return ddraw_surface7_Initialize(&surface->IDirectDrawSurface7_iface,
            ddraw, surface_desc);
}

static HRESULT WINAPI ddraw_surface3_Initialize(IDirectDrawSurface3 *iface,
        IDirectDraw *ddraw, DDSURFACEDESC *surface_desc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, ddraw %p, surface_desc %p.\n", iface, ddraw, surface_desc);

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw_surface7_Initialize(&surface->IDirectDrawSurface7_iface,
            ddraw, surface_desc ? &surface_desc2 : NULL);
}

static HRESULT WINAPI ddraw_surface2_Initialize(IDirectDrawSurface2 *iface,
        IDirectDraw *ddraw, DDSURFACEDESC *surface_desc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, ddraw %p, surface_desc %p.\n", iface, ddraw, surface_desc);

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw_surface7_Initialize(&surface->IDirectDrawSurface7_iface,
            ddraw, surface_desc ? &surface_desc2 : NULL);
}

static HRESULT WINAPI ddraw_surface1_Initialize(IDirectDrawSurface *iface,
        IDirectDraw *ddraw, DDSURFACEDESC *surface_desc)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, ddraw %p, surface_desc %p.\n", iface, ddraw, surface_desc);

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw_surface7_Initialize(&surface->IDirectDrawSurface7_iface,
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
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_surface7_IsLost(IDirectDrawSurface7 *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p.\n", iface);

    if (surface->ddraw->device_state != DDRAW_DEVICE_STATE_OK || surface->is_lost)
        return DDERR_SURFACELOST;

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_IsLost(IDirectDrawSurface4 *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_surface7_IsLost(&surface->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface3_IsLost(IDirectDrawSurface3 *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_surface7_IsLost(&surface->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface2_IsLost(IDirectDrawSurface2 *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_surface7_IsLost(&surface->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface1_IsLost(IDirectDrawSurface *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_surface7_IsLost(&surface->IDirectDrawSurface7_iface);
}

/*****************************************************************************
 * IDirectDrawSurface7::Restore
 *
 * Restores a lost surface. This makes the surface usable again, but
 * doesn't reload its old contents
 *
 * Returns:
 *  DD_OK on success, error code otherwise.
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_surface7_Restore(IDirectDrawSurface7 *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    unsigned int i;

    TRACE("iface %p.\n", iface);

    if (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        struct wined3d_swapchain *swapchain = surface->ddraw->wined3d_swapchain;
        struct wined3d_sub_resource_desc wined3d_desc;
        struct wined3d_display_mode mode;
        HRESULT hr;

        if (FAILED(hr = wined3d_swapchain_get_display_mode(swapchain, &mode, NULL)))
        {
            WARN("Failed to get display mode, hr %#x.\n", hr);
            return hr;
        }

        if (FAILED(hr = wined3d_texture_get_sub_resource_desc(surface->wined3d_texture, 0, &wined3d_desc)))
        {
            WARN("Failed to get resource desc, hr %#x.\n", hr);
            return hr;
        }

        if (mode.width != wined3d_desc.width || mode.height != wined3d_desc.height)
        {
            WARN("Display mode dimensions %ux%u don't match surface dimensions %ux%u.\n",
                    mode.width, mode.height, wined3d_desc.width, wined3d_desc.height);
            return DDERR_WRONGMODE;
        }

        if (mode.format_id != wined3d_desc.format)
        {
            WARN("Display mode format %#x doesn't match surface format %#x.\n",
                    mode.format_id, wined3d_desc.format);
            return DDERR_WRONGMODE;
        }
    }

    ddraw_update_lost_surfaces(surface->ddraw);
    surface->is_lost = FALSE;

    for(i = 0; i < MAX_COMPLEX_ATTACHED; i++)
    {
        if (surface->complex_array[i])
            surface->complex_array[i]->is_lost = FALSE;
    }

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_Restore(IDirectDrawSurface4 *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_surface7_Restore(&surface->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface3_Restore(IDirectDrawSurface3 *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_surface7_Restore(&surface->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface2_Restore(IDirectDrawSurface2 *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_surface7_Restore(&surface->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface1_Restore(IDirectDrawSurface *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_surface7_Restore(&surface->IDirectDrawSurface7_iface);
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
static HRESULT WINAPI ddraw_surface7_SetOverlayPosition(IDirectDrawSurface7 *iface, LONG x, LONG y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, x %d, y %d.\n", iface, x, y);

    wined3d_mutex_lock();
    hr = wined3d_texture_set_overlay_position(surface->wined3d_texture,
            surface->sub_resource_idx, x, y);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_SetOverlayPosition(IDirectDrawSurface4 *iface, LONG x, LONG y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, x %d, y %d.\n", iface, x, y);

    return ddraw_surface7_SetOverlayPosition(&surface->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface3_SetOverlayPosition(IDirectDrawSurface3 *iface, LONG x, LONG y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, x %d, y %d.\n", iface, x, y);

    return ddraw_surface7_SetOverlayPosition(&surface->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface2_SetOverlayPosition(IDirectDrawSurface2 *iface, LONG x, LONG y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, x %d, y %d.\n", iface, x, y);

    return ddraw_surface7_SetOverlayPosition(&surface->IDirectDrawSurface7_iface, x, y);
}

static HRESULT WINAPI ddraw_surface1_SetOverlayPosition(IDirectDrawSurface *iface, LONG x, LONG y)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, x %d, y %d.\n", iface, x, y);

    return ddraw_surface7_SetOverlayPosition(&surface->IDirectDrawSurface7_iface, x, y);
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
static HRESULT WINAPI ddraw_surface7_UpdateOverlay(IDirectDrawSurface7 *iface, RECT *src_rect,
        IDirectDrawSurface7 *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    struct ddraw_surface *src_impl = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_surface *dst_impl = unsafe_impl_from_IDirectDrawSurface7(dst_surface);
    struct wined3d_texture *dst_wined3d_texture = NULL;
    unsigned int dst_sub_resource_idx = 0;
    HRESULT hr;

    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    if (fx)
        FIXME("Ignoring fx %p.\n", fx);

    wined3d_mutex_lock();
    if (dst_impl)
    {
        dst_wined3d_texture = dst_impl->wined3d_texture;
        dst_sub_resource_idx = dst_impl->sub_resource_idx;
    }
    hr = wined3d_texture_update_overlay(src_impl->wined3d_texture, src_impl->sub_resource_idx,
            src_rect, dst_wined3d_texture, dst_sub_resource_idx, dst_rect, flags);
    wined3d_mutex_unlock();

    switch (hr)
    {
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
    struct ddraw_surface *src_impl = impl_from_IDirectDrawSurface4(iface);
    struct ddraw_surface *dst_impl = unsafe_impl_from_IDirectDrawSurface4(dst_surface);

    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return ddraw_surface7_UpdateOverlay(&src_impl->IDirectDrawSurface7_iface, src_rect,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, dst_rect, flags, fx);
}

static HRESULT WINAPI ddraw_surface3_UpdateOverlay(IDirectDrawSurface3 *iface, RECT *src_rect,
        IDirectDrawSurface3 *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    struct ddraw_surface *src_impl = impl_from_IDirectDrawSurface3(iface);
    struct ddraw_surface *dst_impl = unsafe_impl_from_IDirectDrawSurface3(dst_surface);

    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return ddraw_surface7_UpdateOverlay(&src_impl->IDirectDrawSurface7_iface, src_rect,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, dst_rect, flags, fx);
}

static HRESULT WINAPI ddraw_surface2_UpdateOverlay(IDirectDrawSurface2 *iface, RECT *src_rect,
        IDirectDrawSurface2 *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    struct ddraw_surface *src_impl = impl_from_IDirectDrawSurface2(iface);
    struct ddraw_surface *dst_impl = unsafe_impl_from_IDirectDrawSurface2(dst_surface);

    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return ddraw_surface7_UpdateOverlay(&src_impl->IDirectDrawSurface7_iface, src_rect,
            dst_impl ? &dst_impl->IDirectDrawSurface7_iface : NULL, dst_rect, flags, fx);
}

static HRESULT WINAPI ddraw_surface1_UpdateOverlay(IDirectDrawSurface *iface, RECT *src_rect,
        IDirectDrawSurface *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    struct ddraw_surface *src_impl = impl_from_IDirectDrawSurface(iface);
    struct ddraw_surface *dst_impl = unsafe_impl_from_IDirectDrawSurface(dst_surface);

    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return ddraw_surface7_UpdateOverlay(&src_impl->IDirectDrawSurface7_iface, src_rect,
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_UpdateOverlayDisplay(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface3_UpdateOverlayDisplay(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_UpdateOverlayDisplay(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface2_UpdateOverlayDisplay(IDirectDrawSurface2 *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_UpdateOverlayDisplay(&surface->IDirectDrawSurface7_iface, flags);
}

static HRESULT WINAPI ddraw_surface1_UpdateOverlayDisplay(IDirectDrawSurface *iface, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    return ddraw_surface7_UpdateOverlayDisplay(&surface->IDirectDrawSurface7_iface, flags);
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
        DWORD flags, IDirectDrawSurface7 *reference)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);

    FIXME("iface %p, flags %#x, reference %p stub!\n", iface, flags, reference);

    wined3d_mutex_lock();
    if (!(surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_OVERLAY))
    {
        WARN("Not an overlay surface.\n");
        wined3d_mutex_unlock();
        return DDERR_NOTAOVERLAYSURFACE;
    }
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_UpdateOverlayZOrder(IDirectDrawSurface4 *iface,
        DWORD flags, IDirectDrawSurface4 *reference)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct ddraw_surface *reference_impl = unsafe_impl_from_IDirectDrawSurface4(reference);

    TRACE("iface %p, flags %#x, reference %p.\n", iface, flags, reference);

    return ddraw_surface7_UpdateOverlayZOrder(&surface->IDirectDrawSurface7_iface, flags,
            reference_impl ? &reference_impl->IDirectDrawSurface7_iface : NULL);
}

static HRESULT WINAPI ddraw_surface3_UpdateOverlayZOrder(IDirectDrawSurface3 *iface,
        DWORD flags, IDirectDrawSurface3 *reference)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct ddraw_surface *reference_impl = unsafe_impl_from_IDirectDrawSurface3(reference);

    TRACE("iface %p, flags %#x, reference %p.\n", iface, flags, reference);

    return ddraw_surface7_UpdateOverlayZOrder(&surface->IDirectDrawSurface7_iface, flags,
            reference_impl ? &reference_impl->IDirectDrawSurface7_iface : NULL);
}

static HRESULT WINAPI ddraw_surface2_UpdateOverlayZOrder(IDirectDrawSurface2 *iface,
        DWORD flags, IDirectDrawSurface2 *reference)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);
    struct ddraw_surface *reference_impl = unsafe_impl_from_IDirectDrawSurface2(reference);

    TRACE("iface %p, flags %#x, reference %p.\n", iface, flags, reference);

    return ddraw_surface7_UpdateOverlayZOrder(&surface->IDirectDrawSurface7_iface, flags,
            reference_impl ? &reference_impl->IDirectDrawSurface7_iface : NULL);
}

static HRESULT WINAPI ddraw_surface1_UpdateOverlayZOrder(IDirectDrawSurface *iface,
        DWORD flags, IDirectDrawSurface *reference)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);
    struct ddraw_surface *reference_impl = unsafe_impl_from_IDirectDrawSurface(reference);

    TRACE("iface %p, flags %#x, reference %p.\n", iface, flags, reference);

    return ddraw_surface7_UpdateOverlayZOrder(&surface->IDirectDrawSurface7_iface, flags,
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface7(iface);

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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, ddraw %p.\n", iface, ddraw);

    return ddraw_surface7_GetDDInterface(&surface->IDirectDrawSurface7_iface, ddraw);
}

static HRESULT WINAPI ddraw_surface3_GetDDInterface(IDirectDrawSurface3 *iface, void **ddraw)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, ddraw %p.\n", iface, ddraw);

    return ddraw_surface7_GetDDInterface(&surface->IDirectDrawSurface7_iface, ddraw);
}

static HRESULT WINAPI ddraw_surface2_GetDDInterface(IDirectDrawSurface2 *iface, void **ddraw)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, ddraw %p.\n", iface, ddraw);

    return ddraw_surface7_GetDDInterface(&surface->IDirectDrawSurface7_iface, ddraw);
}

static HRESULT WINAPI ddraw_surface7_ChangeUniquenessValue(IDirectDrawSurface7 *iface)
{
    TRACE("iface %p.\n", iface);

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_ChangeUniquenessValue(IDirectDrawSurface4 *iface)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_surface7_ChangeUniquenessValue(&surface->IDirectDrawSurface7_iface);
}

static HRESULT WINAPI ddraw_surface7_GetUniquenessValue(IDirectDrawSurface7 *iface, DWORD *pValue)
{
    TRACE("iface %p, value %p.\n", iface, pValue);

    *pValue = 0;

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetUniquenessValue(IDirectDrawSurface4 *iface, DWORD *pValue)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, value %p.\n", iface, pValue);

    return ddraw_surface7_GetUniquenessValue(&surface->IDirectDrawSurface7_iface, pValue);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;

    TRACE("iface %p, lod %u.\n", iface, MaxLOD);

    wined3d_mutex_lock();
    if (!(surface->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDOBJECT;
    }

    hr = wined3d_texture_set_lod(surface->wined3d_texture, MaxLOD);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, lod %p.\n", iface, MaxLOD);

    if(!MaxLOD)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if (!(surface->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDOBJECT;
    }

    *MaxLOD = wined3d_texture_get_lod(surface->wined3d_texture);
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
 *  DD_OK on success, error code otherwise.
 *
 *****************************************************************************/
static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface7_BltFast(IDirectDrawSurface7 *iface,
        DWORD dst_x, DWORD dst_y, IDirectDrawSurface7 *src_surface, RECT *src_rect, DWORD trans)
{
    struct ddraw_surface *dst_impl = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface7(src_surface);
    DWORD flags = WINED3D_BLT_SYNCHRONOUS;
    DWORD src_w, src_h, dst_w, dst_h;
    HRESULT hr = DD_OK;
    RECT dst_rect, s;

    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), trans);

    dst_w = dst_impl->surface_desc.dwWidth;
    dst_h = dst_impl->surface_desc.dwHeight;

    if (!src_rect)
    {
        SetRect(&s, 0, 0, src_impl->surface_desc.dwWidth, src_impl->surface_desc.dwHeight);
        src_rect = &s;
    }

    src_w = src_rect->right - src_rect->left;
    src_h = src_rect->bottom - src_rect->top;
    if (src_w > dst_w || dst_x > dst_w - src_w
            || src_h > dst_h || dst_y > dst_h - src_h)
    {
        WARN("Destination area out of bounds, returning DDERR_INVALIDRECT.\n");
        return DDERR_INVALIDRECT;
    }

    SetRect(&dst_rect, dst_x, dst_y, dst_x + src_w, dst_y + src_h);
    if (trans & DDBLTFAST_SRCCOLORKEY)
        flags |= WINED3D_BLT_SRC_CKEY;
    if (trans & DDBLTFAST_DESTCOLORKEY)
        flags |= WINED3D_BLT_DST_CKEY;
    if (trans & DDBLTFAST_WAIT)
        flags |= WINED3D_BLT_WAIT;
    if (trans & DDBLTFAST_DONOTWAIT)
        flags |= WINED3D_BLT_DO_NOT_WAIT;

    wined3d_mutex_lock();
    if (dst_impl->clipper)
    {
        wined3d_mutex_unlock();
        WARN("Destination surface has a clipper set, returning DDERR_BLTFASTCANTCLIP.\n");
        return DDERR_BLTFASTCANTCLIP;
    }

    if (src_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        hr = ddraw_surface_update_frontbuffer(src_impl, src_rect, TRUE, 0);
    if (SUCCEEDED(hr))
        hr = wined3d_texture_blt(dst_impl->wined3d_texture, dst_impl->sub_resource_idx, &dst_rect,
                src_impl->wined3d_texture, src_impl->sub_resource_idx, src_rect, flags, NULL, WINED3D_TEXF_POINT);
    if (SUCCEEDED(hr) && (dst_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
        hr = ddraw_surface_update_frontbuffer(dst_impl, &dst_rect, FALSE, 0);
    wined3d_mutex_unlock();

    switch(hr)
    {
        case WINED3DERR_NOTAVAILABLE:           return DDERR_UNSUPPORTED;
        default:                                return hr;
    }
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface4_BltFast(IDirectDrawSurface4 *iface, DWORD dst_x, DWORD dst_y,
        IDirectDrawSurface4 *src_surface, RECT *src_rect, DWORD flags)
{
    struct ddraw_surface *dst_impl = impl_from_IDirectDrawSurface4(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface4(src_surface);

    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return ddraw_surface7_BltFast(&dst_impl->IDirectDrawSurface7_iface, dst_x, dst_y,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface3_BltFast(IDirectDrawSurface3 *iface, DWORD dst_x, DWORD dst_y,
        IDirectDrawSurface3 *src_surface, RECT *src_rect, DWORD flags)
{
    struct ddraw_surface *dst_impl = impl_from_IDirectDrawSurface3(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface3(src_surface);

    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return ddraw_surface7_BltFast(&dst_impl->IDirectDrawSurface7_iface, dst_x, dst_y,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface2_BltFast(IDirectDrawSurface2 *iface, DWORD dst_x, DWORD dst_y,
        IDirectDrawSurface2 *src_surface, RECT *src_rect, DWORD flags)
{
    struct ddraw_surface *dst_impl = impl_from_IDirectDrawSurface2(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface2(src_surface);

    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return ddraw_surface7_BltFast(&dst_impl->IDirectDrawSurface7_iface, dst_x, dst_y,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH ddraw_surface1_BltFast(IDirectDrawSurface *iface, DWORD dst_x, DWORD dst_y,
        IDirectDrawSurface *src_surface, RECT *src_rect, DWORD flags)
{
    struct ddraw_surface *dst_impl = impl_from_IDirectDrawSurface(iface);
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface(src_surface);

    TRACE("iface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return ddraw_surface7_BltFast(&dst_impl->IDirectDrawSurface7_iface, dst_x, dst_y,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, src_rect, flags);
}

static HRESULT WINAPI ddraw_surface7_GetClipper(IDirectDrawSurface7 *iface, IDirectDrawClipper **clipper)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    if (!clipper)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if (!surface->clipper)
    {
        wined3d_mutex_unlock();
        *clipper = NULL;
        return DDERR_NOCLIPPERATTACHED;
    }

    *clipper = &surface->clipper->IDirectDrawClipper_iface;
    if (ddraw_clipper_is_valid(surface->clipper))
        IDirectDrawClipper_AddRef(*clipper);
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_GetClipper(IDirectDrawSurface4 *iface, IDirectDrawClipper **clipper)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_GetClipper(&surface->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface3_GetClipper(IDirectDrawSurface3 *iface, IDirectDrawClipper **clipper)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_GetClipper(&surface->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface2_GetClipper(IDirectDrawSurface2 *iface, IDirectDrawClipper **clipper)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_GetClipper(&surface->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface1_GetClipper(IDirectDrawSurface *iface, IDirectDrawClipper **clipper)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_GetClipper(&surface->IDirectDrawSurface7_iface, clipper);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface7(iface);
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
    if (old_clipper && ddraw_clipper_is_valid(old_clipper))
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
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_SetClipper(&surface->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface3_SetClipper(IDirectDrawSurface3 *iface, IDirectDrawClipper *clipper)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_SetClipper(&surface->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface2_SetClipper(IDirectDrawSurface2 *iface, IDirectDrawClipper *clipper)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_SetClipper(&surface->IDirectDrawSurface7_iface, clipper);
}

static HRESULT WINAPI ddraw_surface1_SetClipper(IDirectDrawSurface *iface, IDirectDrawClipper *clipper)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddraw_surface7_SetClipper(&surface->IDirectDrawSurface7_iface, clipper);
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
    struct ddraw_surface *This = impl_from_IDirectDrawSurface7(iface);
    HRESULT hr;
    const DWORD allowed_flags = DDSD_LPSURFACE | DDSD_PIXELFORMAT | DDSD_WIDTH
            | DDSD_HEIGHT | DDSD_PITCH | DDSD_CAPS;
    enum wined3d_format_id format_id;
    UINT pitch, width, height;

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
    if (!(This->surface_desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
            || This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE
            || This->surface_desc.ddsCaps.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE))
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
    if (!(DDSD->dwFlags & DDSD_LPSURFACE) || !DDSD->lpSurface)
    {
        WARN("DDSD_LPSURFACE is not set or lpSurface is NULL, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }
    if ((DDSD->dwFlags & DDSD_CAPS) && DDSD->ddsCaps.dwCaps)
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
            TRACE("Surface width changed from %u to %u.\n", This->surface_desc.dwWidth, DDSD->dwWidth);
        if (DDSD->u1.lPitch != This->surface_desc.u1.lPitch)
            TRACE("Surface pitch changed from %u to %u.\n", This->surface_desc.u1.lPitch, DDSD->u1.lPitch);
        pitch = DDSD->u1.lPitch;
        width = DDSD->dwWidth;
    }
    else if (DDSD->dwFlags & DDSD_PITCH)
    {
        WARN("DDSD_PITCH is set, but DDSD_WIDTH is not, returning DDERR_INVALIDPARAMS.\n");
        return DDERR_INVALIDPARAMS;
    }
    else
    {
        pitch = This->surface_desc.u1.lPitch;
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
            TRACE("Surface height changed from %u to %u.\n", This->surface_desc.dwHeight, DDSD->dwHeight);
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
        format_id = wined3dformat_from_ddrawformat(&DDSD->u4.ddpfPixelFormat);

        if (format_id == WINED3DFMT_UNKNOWN)
        {
            ERR("Requested to set an unknown pixelformat\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }
        current_format_id = wined3dformat_from_ddrawformat(&This->surface_desc.u4.ddpfPixelFormat);
        if (format_id != current_format_id)
            TRACE("Surface format changed from %#x to %#x.\n", current_format_id, format_id);
    }
    else
    {
        format_id = wined3dformat_from_ddrawformat(&This->surface_desc.u4.ddpfPixelFormat);
    }

    if (FAILED(hr = wined3d_texture_update_desc(This->wined3d_texture, width, height,
            format_id, WINED3D_MULTISAMPLE_NONE, 0, DDSD->lpSurface, pitch)))
    {
        WARN("Failed to update surface desc, hr %#x.\n", hr);
        wined3d_mutex_unlock();
        return hr_ddraw_from_wined3d(hr);
    }

    if (DDSD->dwFlags & DDSD_WIDTH)
        This->surface_desc.dwWidth = width;
    if (DDSD->dwFlags & DDSD_PITCH)
        This->surface_desc.u1.lPitch = DDSD->u1.lPitch;
    if (DDSD->dwFlags & DDSD_HEIGHT)
        This->surface_desc.dwHeight = height;
    if (DDSD->dwFlags & DDSD_PIXELFORMAT)
        This->surface_desc.u4.ddpfPixelFormat = DDSD->u4.ddpfPixelFormat;

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_surface4_SetSurfaceDesc(IDirectDrawSurface4 *iface,
        DDSURFACEDESC2 *surface_desc, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, surface_desc %p, flags %#x.\n", iface, surface_desc, flags);

    return ddraw_surface7_SetSurfaceDesc(&surface->IDirectDrawSurface7_iface,
            surface_desc, flags);
}

static HRESULT WINAPI ddraw_surface3_SetSurfaceDesc(IDirectDrawSurface3 *iface,
        DDSURFACEDESC *surface_desc, DWORD flags)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, surface_desc %p, flags %#x.\n", iface, surface_desc, flags);

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw_surface7_SetSurfaceDesc(&surface->IDirectDrawSurface7_iface,
            surface_desc ? &surface_desc2 : NULL, flags);
}

static HRESULT WINAPI ddraw_surface7_GetPalette(IDirectDrawSurface7 *iface, IDirectDrawPalette **palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);
    struct ddraw_palette *palette_impl;
    HRESULT hr = DD_OK;

    TRACE("iface %p, palette %p.\n", iface, palette);

    if (!palette)
        return DDERR_INVALIDPARAMS;
    if (IDirectDrawSurface7_IsLost(iface) == DDERR_SURFACELOST)
    {
        WARN("Surface lost, returning DDERR_SURFACELOST.\n");
        return DDERR_SURFACELOST;
    }

    wined3d_mutex_lock();
    if ((palette_impl = surface->palette))
    {
        *palette = &palette_impl->IDirectDrawPalette_iface;
        IDirectDrawPalette_AddRef(*palette);
    }
    else
    {
        *palette = NULL;
        hr = DDERR_NOPALETTEATTACHED;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw_surface4_GetPalette(IDirectDrawSurface4 *iface, IDirectDrawPalette **palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_GetPalette(&surface->IDirectDrawSurface7_iface, palette);
}

static HRESULT WINAPI ddraw_surface3_GetPalette(IDirectDrawSurface3 *iface, IDirectDrawPalette **palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_GetPalette(&surface->IDirectDrawSurface7_iface, palette);
}

static HRESULT WINAPI ddraw_surface2_GetPalette(IDirectDrawSurface2 *iface, IDirectDrawPalette **palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_GetPalette(&surface->IDirectDrawSurface7_iface, palette);
}

static HRESULT WINAPI ddraw_surface1_GetPalette(IDirectDrawSurface *iface, IDirectDrawPalette **palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddraw_surface7_GetPalette(&surface->IDirectDrawSurface7_iface, palette);
}

static HRESULT ddraw_surface_set_color_key(struct ddraw_surface *surface, DWORD flags, DDCOLORKEY *color_key)
{
    DDCOLORKEY fixed_color_key;
    HRESULT hr = WINED3D_OK;

    if (flags & DDCKEY_COLORSPACE)
    {
        if (color_key && color_key->dwColorSpaceLowValue != color_key->dwColorSpaceHighValue)
        {
            WARN("Range color keys are not supported, returning DDERR_NOCOLORKEYHW.\n");
            return DDERR_NOCOLORKEYHW;
        }
        flags &= ~DDCKEY_COLORSPACE;
    }

    wined3d_mutex_lock();

    if (color_key)
    {
        fixed_color_key.dwColorSpaceLowValue = fixed_color_key.dwColorSpaceHighValue = color_key->dwColorSpaceLowValue;
        switch (flags & ~DDCKEY_COLORSPACE)
        {
            case DDCKEY_DESTBLT:
                surface->surface_desc.ddckCKDestBlt = fixed_color_key;
                surface->surface_desc.dwFlags |= DDSD_CKDESTBLT;
                break;

            case DDCKEY_DESTOVERLAY:
                surface->surface_desc.u3.ddckCKDestOverlay = fixed_color_key;
                surface->surface_desc.dwFlags |= DDSD_CKDESTOVERLAY;
                break;

            case DDCKEY_SRCOVERLAY:
                surface->surface_desc.ddckCKSrcOverlay = fixed_color_key;
                surface->surface_desc.dwFlags |= DDSD_CKSRCOVERLAY;
                break;

            case DDCKEY_SRCBLT:
                surface->surface_desc.ddckCKSrcBlt = fixed_color_key;
                surface->surface_desc.dwFlags |= DDSD_CKSRCBLT;
                break;

            default:
                wined3d_mutex_unlock();
                return DDERR_INVALIDPARAMS;
        }
    }
    else
    {
        switch (flags & ~DDCKEY_COLORSPACE)
        {
            case DDCKEY_DESTBLT:
                surface->surface_desc.dwFlags &= ~DDSD_CKDESTBLT;
                break;

            case DDCKEY_DESTOVERLAY:
                surface->surface_desc.dwFlags &= ~DDSD_CKDESTOVERLAY;
                break;

            case DDCKEY_SRCOVERLAY:
                surface->surface_desc.dwFlags &= ~DDSD_CKSRCOVERLAY;
                break;

            case DDCKEY_SRCBLT:
                surface->surface_desc.dwFlags &= ~DDSD_CKSRCBLT;
                break;

            default:
                wined3d_mutex_unlock();
                return DDERR_INVALIDPARAMS;
        }
    }

    if (surface->is_complex_root)
        hr = wined3d_texture_set_color_key(surface->wined3d_texture, flags,
                color_key ? (struct wined3d_color_key *)&fixed_color_key : NULL);

    wined3d_mutex_unlock();

    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI ddraw_surface7_SetColorKey(IDirectDrawSurface7 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    if (surface->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
        return DDERR_NOTONMIPMAPSUBLEVEL;

    return ddraw_surface_set_color_key(surface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface4_SetColorKey(IDirectDrawSurface4 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface_set_color_key(surface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface3_SetColorKey(IDirectDrawSurface3 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface_set_color_key(surface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface2_SetColorKey(IDirectDrawSurface2 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface_set_color_key(surface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface1_SetColorKey(IDirectDrawSurface *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, flags %#x, color_key %p.\n", iface, flags, color_key);

    return ddraw_surface_set_color_key(surface, flags, color_key);
}

static HRESULT WINAPI ddraw_surface7_SetPalette(IDirectDrawSurface7 *iface, IDirectDrawPalette *palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface7(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    if (surface->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
        return DDERR_NOTONMIPMAPSUBLEVEL;
    if (IDirectDrawSurface7_IsLost(iface) == DDERR_SURFACELOST)
    {
        WARN("Surface lost, returning DDERR_SURFACELOST.\n");
        return DDERR_SURFACELOST;
    }

    return ddraw_surface_set_palette(surface, palette);
}

static HRESULT WINAPI ddraw_surface4_SetPalette(IDirectDrawSurface4 *iface, IDirectDrawPalette *palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    if (IDirectDrawSurface4_IsLost(iface) == DDERR_SURFACELOST)
    {
        WARN("Surface lost, returning DDERR_SURFACELOST.\n");
        return DDERR_SURFACELOST;
    }

    return ddraw_surface_set_palette(surface, palette);
}

static HRESULT WINAPI ddraw_surface3_SetPalette(IDirectDrawSurface3 *iface, IDirectDrawPalette *palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    if (IDirectDrawSurface3_IsLost(iface) == DDERR_SURFACELOST)
    {
        WARN("Surface lost, returning DDERR_SURFACELOST.\n");
        return DDERR_SURFACELOST;
    }

    return ddraw_surface_set_palette(surface, palette);
}

static HRESULT WINAPI ddraw_surface2_SetPalette(IDirectDrawSurface2 *iface, IDirectDrawPalette *palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface2(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    if (IDirectDrawSurface2_IsLost(iface) == DDERR_SURFACELOST)
    {
        WARN("Surface lost, returning DDERR_SURFACELOST.\n");
        return DDERR_SURFACELOST;
    }

    return ddraw_surface_set_palette(surface, palette);
}

static HRESULT WINAPI ddraw_surface1_SetPalette(IDirectDrawSurface *iface, IDirectDrawPalette *palette)
{
    struct ddraw_surface *surface = impl_from_IDirectDrawSurface(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    if (IDirectDrawSurface_IsLost(iface) == DDERR_SURFACELOST)
    {
        WARN("Surface lost, returning DDERR_SURFACELOST.\n");
        return DDERR_SURFACELOST;
    }

    return ddraw_surface_set_palette(surface, palette);
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
    struct ddraw_surface *surface = impl_from_IDirectDrawGammaControl(iface);

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
    struct ddraw_surface *surface = impl_from_IDirectDrawGammaControl(iface);

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
    struct ddraw_surface *surface = impl_from_IDirect3DTexture(iface);

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
 * Returns handle for the texture.
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
    struct ddraw_surface *surface = impl_from_IDirect3DTexture2(iface);
    struct d3d_device *device_impl = unsafe_impl_from_IDirect3DDevice2(device);

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
    struct ddraw_surface *surface = impl_from_IDirect3DTexture(iface);
    struct d3d_device *device_impl = unsafe_impl_from_IDirect3DDevice(device);

    TRACE("iface %p, device %p, handle %p.\n", iface, device, handle);

    return d3d_texture2_GetHandle(&surface->IDirect3DTexture2_iface,
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
static struct ddraw_surface *get_sub_mimaplevel(struct ddraw_surface *surface)
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
 * implemented in DDraw only. For speed improvements an implementation which
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
    struct ddraw_surface *dst_surface = impl_from_IDirect3DTexture2(iface);
    struct ddraw_surface *src_surface = unsafe_impl_from_IDirect3DTexture2(src_texture);
    struct wined3d_resource *dst_resource, *src_resource;
    HRESULT hr;

    TRACE("iface %p, src_texture %p.\n", iface, src_texture);

    if (src_surface == dst_surface)
    {
        TRACE("copying surface %p to surface %p, why?\n", src_surface, dst_surface);
        return D3D_OK;
    }

    wined3d_mutex_lock();

    dst_resource = wined3d_texture_get_resource(dst_surface->wined3d_texture);
    src_resource = wined3d_texture_get_resource(src_surface->wined3d_texture);

    if (((src_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
            != (dst_surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP))
            || (src_surface->surface_desc.u2.dwMipMapCount != dst_surface->surface_desc.u2.dwMipMapCount))
    {
        ERR("Trying to load surfaces with different mip-map counts.\n");
    }

    for (;;)
    {
        struct ddraw_palette *dst_pal, *src_pal;
        DDSURFACEDESC *src_desc, *dst_desc;

        TRACE("Copying surface %p to surface %p.\n", src_surface, dst_surface);

        /* Suppress the ALLOCONLOAD flag */
        dst_surface->surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;

        /* Get the palettes */
        dst_pal = dst_surface->palette;
        src_pal = src_surface->palette;

        if (src_pal)
        {
            PALETTEENTRY palent[256];

            if (!dst_pal)
            {
                wined3d_mutex_unlock();
                return DDERR_NOPALETTEATTACHED;
            }
            IDirectDrawPalette_GetEntries(&src_pal->IDirectDrawPalette_iface, 0, 0, 256, palent);
            IDirectDrawPalette_SetEntries(&dst_pal->IDirectDrawPalette_iface, 0, 0, 256, palent);
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
            struct wined3d_map_desc src_map_desc, dst_map_desc;

            /* Copy the src blit color key if the source has one, don't erase
             * the destination's ckey if the source has none */
            if (src_desc->dwFlags & DDSD_CKSRCBLT)
            {
                IDirectDrawSurface7_SetColorKey(&dst_surface->IDirectDrawSurface7_iface,
                        DDCKEY_SRCBLT, &src_desc->ddckCKSrcBlt);
            }

            if (FAILED(hr = wined3d_resource_map(src_resource,
                    src_surface->sub_resource_idx, &src_map_desc, NULL, WINED3D_MAP_READ)))
            {
                ERR("Failed to lock source surface, hr %#x.\n", hr);
                wined3d_mutex_unlock();
                return D3DERR_TEXTURE_LOAD_FAILED;
            }

            if (FAILED(hr = wined3d_resource_map(dst_resource,
                    dst_surface->sub_resource_idx, &dst_map_desc, NULL, WINED3D_MAP_WRITE)))
            {
                ERR("Failed to lock destination surface, hr %#x.\n", hr);
                wined3d_resource_unmap(src_resource, src_surface->sub_resource_idx);
                wined3d_mutex_unlock();
                return D3DERR_TEXTURE_LOAD_FAILED;
            }

            if (dst_surface->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
                memcpy(dst_map_desc.data, src_map_desc.data, src_surface->surface_desc.u1.dwLinearSize);
            else
                memcpy(dst_map_desc.data, src_map_desc.data, src_map_desc.row_pitch * src_desc->dwHeight);

            wined3d_resource_unmap(dst_resource, dst_surface->sub_resource_idx);
            wined3d_resource_unmap(src_resource, src_surface->sub_resource_idx);
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
    struct ddraw_surface *dst_surface = impl_from_IDirect3DTexture(iface);
    struct ddraw_surface *src_surface = unsafe_impl_from_IDirect3DTexture(src_texture);

    TRACE("iface %p, src_texture %p.\n", iface, src_texture);

    return d3d_texture2_Load(&dst_surface->IDirect3DTexture2_iface,
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

/* Some games (e.g. Tomb Raider 3) pass the wrong version of the
 * IDirectDrawSurface interface to ddraw methods. */
struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface7(IDirectDrawSurface7 *iface)
{
    if (!iface) return NULL;
    if (iface->lpVtbl != &ddraw_surface7_vtbl)
    {
        HRESULT hr = IDirectDrawSurface7_QueryInterface(iface, &IID_IDirectDrawSurface7, (void **)&iface);
        if (FAILED(hr))
        {
            WARN("Object %p doesn't expose interface IDirectDrawSurface7.\n", iface);
            return NULL;
        }
        IDirectDrawSurface7_Release(iface);
    }
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface7_iface);
}

struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface)
{
    if (!iface) return NULL;
    if (iface->lpVtbl != &ddraw_surface4_vtbl)
    {
        HRESULT hr = IDirectDrawSurface4_QueryInterface(iface, &IID_IDirectDrawSurface4, (void **)&iface);
        if (FAILED(hr))
        {
            WARN("Object %p doesn't expose interface IDirectDrawSurface4.\n", iface);
            return NULL;
        }
        IDirectDrawSurface4_Release(iface);
    }
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface4_iface);
}

static struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface)
{
    if (!iface) return NULL;
    if (iface->lpVtbl != &ddraw_surface3_vtbl)
    {
        HRESULT hr = IDirectDrawSurface3_QueryInterface(iface, &IID_IDirectDrawSurface3, (void **)&iface);
        if (FAILED(hr))
        {
            WARN("Object %p doesn't expose interface IDirectDrawSurface3.\n", iface);
            return NULL;
        }
        IDirectDrawSurface3_Release(iface);
    }
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface3_iface);
}

static struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface2(IDirectDrawSurface2 *iface)
{
    if (!iface) return NULL;
    if (iface->lpVtbl != &ddraw_surface2_vtbl)
    {
        HRESULT hr = IDirectDrawSurface2_QueryInterface(iface, &IID_IDirectDrawSurface2, (void **)&iface);
        if (FAILED(hr))
        {
            WARN("Object %p doesn't expose interface IDirectDrawSurface2.\n", iface);
            return NULL;
        }
        IDirectDrawSurface2_Release(iface);
    }
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface2_iface);
}

struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface(IDirectDrawSurface *iface)
{
    if (!iface) return NULL;
    if (iface->lpVtbl != &ddraw_surface1_vtbl)
    {
        HRESULT hr = IDirectDrawSurface_QueryInterface(iface, &IID_IDirectDrawSurface, (void **)&iface);
        if (FAILED(hr))
        {
            WARN("Object %p doesn't expose interface IDirectDrawSurface.\n", iface);
            return NULL;
        }
        IDirectDrawSurface_Release(iface);
    }
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface_iface);
}

struct ddraw_surface *unsafe_impl_from_IDirect3DTexture2(IDirect3DTexture2 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_texture2_vtbl);
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirect3DTexture2_iface);
}

struct ddraw_surface *unsafe_impl_from_IDirect3DTexture(IDirect3DTexture *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_texture1_vtbl);
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirect3DTexture_iface);
}

static void STDMETHODCALLTYPE ddraw_surface_wined3d_object_destroyed(void *parent)
{
    struct ddraw_surface *surface = parent;

    TRACE("surface %p.\n", surface);

    /* This shouldn't happen, ddraw_surface_release_iface() should prevent the
     * surface from being destroyed in this case. */
    if (surface->first_attached != surface)
        ERR("Surface is still attached to surface %p.\n", surface->first_attached);

    while (surface->next_attached)
        if (FAILED(ddraw_surface_delete_attached_surface(surface,
                surface->next_attached, surface->next_attached->attached_iface)))
            ERR("DeleteAttachedSurface failed.\n");

    /* Having a texture handle set implies that the device still exists. */
    if (surface->Handle)
        ddraw_free_handle(&surface->ddraw->d3ddevice->handle_table, surface->Handle - 1, DDRAW_HANDLE_SURFACE);

    /* Reduce the ddraw surface count. */
    list_remove(&surface->surface_list_entry);

    if (surface->clipper && ddraw_clipper_is_valid(surface->clipper))
        IDirectDrawClipper_Release(&surface->clipper->IDirectDrawClipper_iface);

    if (surface == surface->ddraw->primary)
    {
        surface->ddraw->primary = NULL;
        surface->ddraw->gdi_surface = NULL;
    }

    wined3d_private_store_cleanup(&surface->private_store);

    heap_free(surface);
}

static const struct wined3d_parent_ops ddraw_surface_wined3d_parent_ops =
{
    ddraw_surface_wined3d_object_destroyed,
};

static void STDMETHODCALLTYPE ddraw_texture_wined3d_object_destroyed(void *parent)
{
    TRACE("parent %p.\n", parent);

    heap_free(parent);
}

static const struct wined3d_parent_ops ddraw_texture_wined3d_parent_ops =
{
    ddraw_texture_wined3d_object_destroyed,
};

static HRESULT CDECL ddraw_reset_enum_callback(struct wined3d_resource *resource)
{
    return DD_OK;
}

HRESULT ddraw_surface_create(struct ddraw *ddraw, const DDSURFACEDESC2 *surface_desc,
        struct ddraw_surface **surface, IUnknown *outer_unknown, unsigned int version)
{
    struct wined3d_sub_resource_desc wined3d_mip_desc;
    struct ddraw_surface *root, *mip, **attach;
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_texture *wined3d_texture;
    struct wined3d_display_mode mode;
    DDSURFACEDESC2 *desc, *mip_desc;
    struct ddraw_texture *texture;
    unsigned int layers = 1;
    unsigned int pitch = 0;
    UINT levels, i, j;
    HRESULT hr;

    TRACE("ddraw %p, surface_desc %p, surface %p, outer_unknown %p, version %u.\n",
            ddraw, surface_desc, surface, outer_unknown, version);
    if (TRACE_ON(ddraw))
    {
        TRACE("Requesting surface desc:\n");
        DDRAW_dump_surface_desc(surface_desc);
    }

    if (outer_unknown)
        return CLASS_E_NOAGGREGATION;

    if (!surface)
        return E_POINTER;

    if (!(texture = heap_alloc(sizeof(*texture))))
        return E_OUTOFMEMORY;

    texture->version = version;
    texture->surface_desc = *surface_desc;
    desc = &texture->surface_desc;

    /* Ensure DDSD_CAPS is always set. */
    desc->dwFlags |= DDSD_CAPS;

    if (desc->ddsCaps.dwCaps & DDSCAPS_FLIP)
    {
        if (!(desc->dwFlags & DDSD_BACKBUFFERCOUNT) || !desc->u5.dwBackBufferCount)
        {
            WARN("Tried to create a flippable surface without any back buffers.\n");
            heap_free(texture);
            return DDERR_INVALIDCAPS;
        }

        if (!(desc->ddsCaps.dwCaps & DDSCAPS_COMPLEX))
        {
            WARN("Tried to create a flippable surface without DDSCAPS_COMPLEX.\n");
            heap_free(texture);
            return DDERR_INVALIDCAPS;
        }

        if (desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
        {
            WARN("Tried to create a flippable cubemap.\n");
            heap_free(texture);
            return DDERR_INVALIDPARAMS;
        }

        if (desc->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
        {
            FIXME("Flippable textures not implemented.\n");
            heap_free(texture);
            return DDERR_INVALIDCAPS;
        }
    }
    else
    {
        if (desc->dwFlags & DDSD_BACKBUFFERCOUNT)
        {
            WARN("Tried to specify a back buffer count for a non-flippable surface.\n");
            hr = desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP ? DDERR_INVALIDPARAMS : DDERR_INVALIDCAPS;
            heap_free(texture);
            return hr;
        }
    }

    if (desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        if (desc->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
        {
            WARN("Tried to create a primary surface with DDSCAPS_TEXTURE.\n");
            heap_free(texture);
            return DDERR_INVALIDCAPS;
        }

        if ((desc->ddsCaps.dwCaps & DDSCAPS_COMPLEX) && !(desc->ddsCaps.dwCaps & DDSCAPS_FLIP))
        {
            WARN("Tried to create a flippable primary surface without both DDSCAPS_FLIP and DDSCAPS_COMPLEX.\n");
            heap_free(texture);
            return DDERR_INVALIDCAPS;
        }

        if ((desc->ddsCaps.dwCaps & DDSCAPS_FLIP) && !(ddraw->cooperative_level & DDSCL_EXCLUSIVE))
        {
            WARN("Tried to create a flippable primary surface without DDSCL_EXCLUSIVE.\n");
            heap_free(texture);
            return DDERR_NOEXCLUSIVEMODE;
        }
    }

    /* This is a special case in ddrawex, but not allowed in ddraw. */
    if ((desc->ddsCaps.dwCaps & (DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY))
            == (DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY))
    {
        WARN("Tried to create a surface in both system and video memory.\n");
        heap_free(texture);
        return DDERR_INVALIDCAPS;
    }

    if ((desc->ddsCaps.dwCaps & (DDSCAPS_ALLOCONLOAD | DDSCAPS_MIPMAP))
            && !(desc->ddsCaps.dwCaps & DDSCAPS_TEXTURE))
    {
        WARN("Caps %#x require DDSCAPS_TEXTURE.\n", desc->ddsCaps.dwCaps);
        heap_free(texture);
        return DDERR_INVALIDCAPS;
    }

    if ((desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES)
            && !(desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP))
    {
        WARN("Cube map faces requested without cube map flag.\n");
        heap_free(texture);
        return DDERR_INVALIDCAPS;
    }

    if ((desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
            && !(desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES))
    {
        WARN("Cube map without faces requested.\n");
        heap_free(texture);
        return DDERR_INVALIDPARAMS;
    }

    if ((desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
            && (desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES) != DDSCAPS2_CUBEMAP_ALLFACES)
        FIXME("Partial cube maps not implemented.\n");

    if (desc->ddsCaps.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE))
    {
        if (!(desc->ddsCaps.dwCaps & DDSCAPS_TEXTURE))
        {
            WARN("DDSCAPS2_TEXTUREMANAGE used without DDSCAPS_TEXTURE, returning DDERR_INVALIDCAPS.\n");
            heap_free(texture);
            return DDERR_INVALIDCAPS;
        }
        if (desc->ddsCaps.dwCaps & (DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY))
        {
            WARN("DDSCAPS2_TEXTUREMANAGE used with DDSCAPS_VIDEOMEMORY "
                    "or DDSCAPS_SYSTEMMEMORY, returning DDERR_INVALIDCAPS.\n");
            heap_free(texture);
            return DDERR_INVALIDCAPS;
        }
    }

    if (desc->ddsCaps.dwCaps & DDSCAPS_WRITEONLY
            && !(desc->ddsCaps.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)))
    {
        WARN("DDSCAPS_WRITEONLY used without DDSCAPS2_TEXTUREMANAGE, returning DDERR_INVALIDCAPS.\n");
        heap_free(texture);
        return DDERR_INVALIDCAPS;
    }

    if (FAILED(hr = wined3d_get_adapter_display_mode(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &mode, NULL)))
    {
        ERR("Failed to get display mode, hr %#x.\n", hr);
        heap_free(texture);
        return hr_ddraw_from_wined3d(hr);
    }

    /* No pixelformat given? Use the current screen format. */
    if (!(desc->dwFlags & DDSD_PIXELFORMAT))
    {
        desc->dwFlags |= DDSD_PIXELFORMAT;
        desc->u4.ddpfPixelFormat.dwSize = sizeof(desc->u4.ddpfPixelFormat);
        ddrawformat_from_wined3dformat(&desc->u4.ddpfPixelFormat, mode.format_id);
    }

    wined3d_desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    wined3d_desc.format = wined3dformat_from_ddrawformat(&desc->u4.ddpfPixelFormat);
    if (wined3d_desc.format == WINED3DFMT_UNKNOWN)
    {
        WARN("Unsupported / unknown pixelformat.\n");
        heap_free(texture);
        return DDERR_INVALIDPIXELFORMAT;
    }

    /* No width or no height? Use the screen size. */
    if (!(desc->dwFlags & DDSD_WIDTH) || !(desc->dwFlags & DDSD_HEIGHT))
    {
        if (!(desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
        {
            WARN("No width / height specified.\n");
            heap_free(texture);
            return DDERR_INVALIDPARAMS;
        }

        desc->dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
        desc->dwWidth = mode.width;
        desc->dwHeight = mode.height;
    }

    if (!desc->dwWidth || !desc->dwHeight)
    {
        heap_free(texture);
        return DDERR_INVALIDPARAMS;
    }

    if (desc->ddsCaps.dwCaps & DDSCAPS_FLIP)
        desc->ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;

    if (desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        /* The first surface is a front buffer, the back buffers are created
         * afterwards. */
        desc->ddsCaps.dwCaps |= DDSCAPS_VISIBLE;
        if (ddraw->cooperative_level & DDSCL_EXCLUSIVE)
        {
            struct wined3d_swapchain_desc swapchain_desc;

            wined3d_swapchain_get_desc(ddraw->wined3d_swapchain, &swapchain_desc);
            swapchain_desc.backbuffer_width = mode.width;
            swapchain_desc.backbuffer_height = mode.height;
            swapchain_desc.backbuffer_format = mode.format_id;

            if (FAILED(hr = wined3d_device_reset(ddraw->wined3d_device,
                    &swapchain_desc, NULL, ddraw_reset_enum_callback, TRUE)))
            {
                ERR("Failed to reset device.\n");
                heap_free(texture);
                return hr_ddraw_from_wined3d(hr);
            }

            if (ddraw->d3ddevice)
            {
                if (ddraw->d3ddevice->recording)
                    wined3d_stateblock_decref(ddraw->d3ddevice->recording);
                ddraw->d3ddevice->recording = NULL;
            }

            wined3d_device_set_render_state(ddraw->wined3d_device, WINED3D_RS_ZENABLE,
                    !!swapchain_desc.enable_auto_depth_stencil);
        }
    }

    wined3d_desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    wined3d_desc.multisample_quality = 0;
    wined3d_desc.usage = 0;
    wined3d_desc.bind_flags = 0;
    wined3d_desc.access = WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    wined3d_desc.width = desc->dwWidth;
    wined3d_desc.height = desc->dwHeight;
    wined3d_desc.depth = 1;
    wined3d_desc.size = 0;

    if ((desc->ddsCaps.dwCaps & DDSCAPS_3DDEVICE) && (ddraw->flags & DDRAW_NO3D))
    {
        WARN("The application requests a 3D capable surface, but the ddraw object was created without 3D support.\n");
        /* Do not fail surface creation, only fail 3D device creation. */
    }

    /* Mipmap count fixes */
    if (desc->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        if (desc->ddsCaps.dwCaps & DDSCAPS_COMPLEX)
        {
            if (desc->dwFlags & DDSD_MIPMAPCOUNT)
            {
                /* Mipmap count is given, should not be 0. */
                if (!desc->u2.dwMipMapCount)
                {
                    heap_free(texture);
                    return DDERR_INVALIDPARAMS;
                }
            }
            else
            {
                /* Undocumented feature: Create sublevels until either the
                 * width or the height is 1. */
                if (version == 7)
                    desc->u2.dwMipMapCount = wined3d_log2i(max(desc->dwWidth, desc->dwHeight)) + 1;
                else
                    desc->u2.dwMipMapCount = wined3d_log2i(min(desc->dwWidth, desc->dwHeight)) + 1;
            }
        }
        else
        {
            desc->u2.dwMipMapCount = 1;
        }

        desc->dwFlags |= DDSD_MIPMAPCOUNT;
        levels = desc->u2.dwMipMapCount;
    }
    else
    {
        levels = 1;
    }

    if (!(desc->ddsCaps.dwCaps & (DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY)))
    {
        if (!(desc->ddsCaps.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)))
        {
            unsigned int bind_flags = 0;
            DWORD usage = 0;

            if (desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
            {
                usage |= WINED3DUSAGE_LEGACY_CUBEMAP;
                bind_flags |= WINED3D_BIND_SHADER_RESOURCE;
            }
            else if (desc->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
            {
                bind_flags |= WINED3D_BIND_SHADER_RESOURCE;
            }

            if (desc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
                bind_flags |= WINED3D_BIND_DEPTH_STENCIL;
            else if (desc->ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
                bind_flags |= WINED3D_BIND_RENDER_TARGET;

            if (!(ddraw->flags & DDRAW_NO3D) && SUCCEEDED(hr = wined3d_check_device_format(ddraw->wined3d,
                    WINED3DADAPTER_DEFAULT, WINED3D_DEVICE_TYPE_HAL, mode.format_id,
                    usage, bind_flags, WINED3D_RTYPE_TEXTURE_2D, wined3d_desc.format)))
                desc->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
            else
                desc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        }
        else if (!(desc->ddsCaps.dwCaps & DDSCAPS_TEXTURE))
        {
            /* Tests show surfaces without memory flags get these flags added
             * right after creation. */
            desc->ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
        }
    }

    if ((desc->ddsCaps.dwCaps & (DDSCAPS_OVERLAY | DDSCAPS_SYSTEMMEMORY))
            == (DDSCAPS_OVERLAY | DDSCAPS_SYSTEMMEMORY))
    {
        WARN("System memory overlays are not allowed.\n");
        heap_free(texture);
        return DDERR_NOOVERLAYHW;
    }

    if (desc->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
    {
        wined3d_desc.access = WINED3D_RESOURCE_ACCESS_CPU
                | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    }
    else
    {
        if (!(ddraw->flags & DDRAW_NO3D))
        {
            if (desc->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
                wined3d_desc.bind_flags |= WINED3D_BIND_SHADER_RESOURCE;
            if (desc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
                wined3d_desc.bind_flags |= WINED3D_BIND_DEPTH_STENCIL;
            else if (desc->ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
                wined3d_desc.bind_flags |= WINED3D_BIND_RENDER_TARGET;
        }

        if (desc->ddsCaps.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE))
        {
            wined3d_desc.access = WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_CPU
                    | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
            /* Managed textures have the system memory flag set. */
            desc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        }
        else if (desc->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
        {
            /* Videomemory adds localvidmem. This is mutually exclusive with
             * systemmemory and texturemanage. */
            desc->ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM;
            /* Dynamic resources can't be written by the GPU. */
            if (!(wined3d_desc.bind_flags & (WINED3D_BIND_RENDER_TARGET | WINED3D_BIND_DEPTH_STENCIL)))
                wined3d_desc.usage |= WINED3DUSAGE_DYNAMIC;
        }
    }

    if (desc->dwFlags & DDSD_LPSURFACE)
    {
        if (wined3d_desc.access & WINED3D_RESOURCE_ACCESS_GPU)
        {
            WARN("User memory surfaces should not be GPU accessible.\n");
            heap_free(texture);
            return DDERR_INVALIDCAPS;
        }

        if (version < 4)
        {
            WARN("User memory surfaces not supported before version 4.\n");
            heap_free(texture);
            return DDERR_INVALIDPARAMS;
        }

        if (!desc->lpSurface)
        {
            WARN("NULL surface memory pointer specified.\n");
            heap_free(texture);
            return DDERR_INVALIDPARAMS;
        }

        if (format_is_compressed(&desc->u4.ddpfPixelFormat))
        {
            if (version != 4 && (desc->dwFlags & DDSD_PITCH))
            {
                WARN("Pitch specified on a compressed user memory surface.\n");
                heap_free(texture);
                return DDERR_INVALIDPARAMS;
            }

            if (!(desc->dwFlags & (DDSD_LINEARSIZE | DDSD_PITCH)))
            {
                WARN("Compressed user memory surfaces should explicitly specify the linear size.\n");
                heap_free(texture);
                return DDERR_INVALIDPARAMS;
            }

            if ((desc->dwFlags & DDSD_LINEARSIZE)
                    && desc->u1.dwLinearSize < wined3d_calculate_format_pitch(ddraw->wined3d, WINED3DADAPTER_DEFAULT,
                            wined3d_desc.format, wined3d_desc.width) * ((desc->dwHeight + 3) / 4))
            {
                WARN("Invalid linear size %u specified.\n", desc->u1.dwLinearSize);
                heap_free(texture);
                return DDERR_INVALIDPARAMS;
            }
        }
        else
        {
            if (!(desc->dwFlags & DDSD_PITCH))
            {
                WARN("User memory surfaces should explicitly specify the pitch.\n");
                heap_free(texture);
                return DDERR_INVALIDPARAMS;
            }

            if (desc->u1.lPitch < wined3d_calculate_format_pitch(ddraw->wined3d, WINED3DADAPTER_DEFAULT,
                    wined3d_desc.format, wined3d_desc.width) || desc->u1.lPitch & 3)
            {
                WARN("Invalid pitch %u specified.\n", desc->u1.lPitch);
                heap_free(texture);
                return DDERR_INVALIDPARAMS;
            }

            pitch = desc->u1.lPitch;
        }
    }

    if (((desc->dwFlags & DDSD_CKDESTOVERLAY)
            && desc->u3.ddckCKDestOverlay.dwColorSpaceLowValue != desc->u3.ddckCKDestOverlay.dwColorSpaceHighValue)
            || ((desc->dwFlags & DDSD_CKDESTBLT)
            && desc->ddckCKDestBlt.dwColorSpaceLowValue != desc->ddckCKDestBlt.dwColorSpaceHighValue)
            || ((desc->dwFlags & DDSD_CKSRCOVERLAY)
            && desc->ddckCKSrcOverlay.dwColorSpaceLowValue != desc->ddckCKSrcOverlay.dwColorSpaceHighValue)
            || ((desc->dwFlags & DDSD_CKSRCBLT)
            && desc->ddckCKSrcBlt.dwColorSpaceLowValue != desc->ddckCKSrcBlt.dwColorSpaceHighValue))
    {
        WARN("Range color keys not supported, returning DDERR_NOCOLORKEYHW.\n");
        heap_free(texture);
        return DDERR_NOCOLORKEYHW;
    }

    if ((ddraw->flags & DDRAW_NO3D) && (desc->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        WARN("Video memory surfaces not supported without 3D support.\n");
        heap_free(texture);
        return DDERR_NODIRECTDRAWHW;
    }

    if (desc->ddsCaps.dwCaps & (DDSCAPS_OVERLAY))
        wined3d_desc.usage |= WINED3DUSAGE_OVERLAY;

    if (desc->ddsCaps.dwCaps & DDSCAPS_OWNDC)
        wined3d_desc.usage |= WINED3DUSAGE_OWNDC;

    if (desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
    {
        wined3d_desc.usage |= WINED3DUSAGE_LEGACY_CUBEMAP;
        layers = 6;
    }

    /* Some applications assume surfaces will always be mapped at the same
     * address. Some of those also assume that this address is valid even when
     * the surface isn't mapped, and that updates done this way will be
     * visible on the screen. The game Nox is such an application,
     * Commandos: Behind Enemy Lines is another. Setting
     * WINED3D_TEXTURE_CREATE_GET_DC_LENIENT will ensure this. */
    if (FAILED(hr = wined3d_texture_create(ddraw->wined3d_device, &wined3d_desc, layers, levels,
            WINED3D_TEXTURE_CREATE_GET_DC_LENIENT, NULL, texture,
            &ddraw_texture_wined3d_parent_ops, &wined3d_texture)))
    {
        WARN("Failed to create wined3d texture, hr %#x.\n", hr);
        heap_free(texture);
        return hr_ddraw_from_wined3d(hr);
    }

    root = wined3d_texture_get_sub_resource_parent(wined3d_texture, 0);
    wined3d_texture_decref(wined3d_texture);
    root->is_complex_root = TRUE;
    texture->root = root;
    wined3d_device_incref(texture->wined3d_device = ddraw->wined3d_device);

    if (desc->dwFlags & DDSD_CKDESTOVERLAY)
        wined3d_texture_set_color_key(wined3d_texture, DDCKEY_DESTOVERLAY,
                (struct wined3d_color_key *)&desc->u3.ddckCKDestOverlay);
    if (desc->dwFlags & DDSD_CKDESTBLT)
        wined3d_texture_set_color_key(wined3d_texture, DDCKEY_DESTBLT,
                (struct wined3d_color_key *)&desc->ddckCKDestBlt);
    if (desc->dwFlags & DDSD_CKSRCOVERLAY)
        wined3d_texture_set_color_key(wined3d_texture, DDCKEY_SRCOVERLAY,
                (struct wined3d_color_key *)&desc->ddckCKSrcOverlay);
    if (desc->dwFlags & DDSD_CKSRCBLT)
        wined3d_texture_set_color_key(wined3d_texture, DDCKEY_SRCBLT,
                (struct wined3d_color_key *)&desc->ddckCKSrcBlt);

    for (i = 0; i < layers; ++i)
    {
        attach = &root->complex_array[layers - 1 - i];

        for (j = 0; j < levels; ++j)
        {
            mip = wined3d_texture_get_sub_resource_parent(wined3d_texture, i * levels + j);
            mip_desc = &mip->surface_desc;
            if (desc->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
                mip_desc->u2.dwMipMapCount = levels - j;

            if (j)
            {
                wined3d_texture_get_sub_resource_desc(wined3d_texture, i * levels + j, &wined3d_mip_desc);
                mip_desc->dwWidth = wined3d_mip_desc.width;
                mip_desc->dwHeight = wined3d_mip_desc.height;

                mip_desc->ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
            }
            else
            {
                mip_desc->ddsCaps.dwCaps2 &= ~DDSCAPS2_MIPMAPSUBLEVEL;
            }

            if (mip_desc->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
            {
                mip_desc->ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_ALLFACES;

                switch (i)
                {
                    case WINED3D_CUBEMAP_FACE_POSITIVE_X:
                        mip_desc->ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP_POSITIVEX;
                        break;
                    case WINED3D_CUBEMAP_FACE_NEGATIVE_X:
                        mip_desc->ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP_NEGATIVEX;
                        break;
                    case WINED3D_CUBEMAP_FACE_POSITIVE_Y:
                        mip_desc->ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP_POSITIVEY;
                        break;
                    case WINED3D_CUBEMAP_FACE_NEGATIVE_Y:
                        mip_desc->ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP_NEGATIVEY;
                        break;
                    case WINED3D_CUBEMAP_FACE_POSITIVE_Z:
                        mip_desc->ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP_POSITIVEZ;
                        break;
                    case WINED3D_CUBEMAP_FACE_NEGATIVE_Z:
                        mip_desc->ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP_NEGATIVEZ;
                        break;
                }

            }

            if (mip == root)
                continue;

            *attach = mip;
            attach = &mip->complex_array[0];
        }
    }

    if ((desc->dwFlags & DDSD_LPSURFACE) && FAILED(hr = wined3d_texture_update_desc(wined3d_texture,
            wined3d_desc.width, wined3d_desc.height, wined3d_desc.format,
            WINED3D_MULTISAMPLE_NONE, 0, desc->lpSurface, pitch)))
    {
        ERR("Failed to set surface memory, hr %#x.\n", hr);
        goto fail;
    }

    if (desc->dwFlags & DDSD_BACKBUFFERCOUNT)
    {
        unsigned int count = desc->u5.dwBackBufferCount;
        struct ddraw_surface *last = root;

        attach = &last->complex_array[0];
        for (i = 0; i < count; ++i)
        {
            if (!(texture = heap_alloc(sizeof(*texture))))
            {
                hr = E_OUTOFMEMORY;
                goto fail;
            }

            texture->version = version;
            texture->surface_desc = root->surface_desc;
            desc = &texture->surface_desc;

            /* Only one surface in the flipping chain is a back buffer, one is
             * a front buffer, the others are just flippable surfaces. */
            desc->ddsCaps.dwCaps &= ~(DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER
                    | DDSCAPS_BACKBUFFER);
            if (!i)
                desc->ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
            desc->u5.dwBackBufferCount = 0;

            if (FAILED(hr = wined3d_texture_create(ddraw->wined3d_device, &wined3d_desc, 1, 1,
                    WINED3D_TEXTURE_CREATE_GET_DC_LENIENT, NULL, texture,
                    &ddraw_texture_wined3d_parent_ops, &wined3d_texture)))
            {
                heap_free(texture);
                hr = hr_ddraw_from_wined3d(hr);
                goto fail;
            }

            last = wined3d_texture_get_sub_resource_parent(wined3d_texture, 0);
            wined3d_texture_decref(wined3d_texture);
            texture->root = last;
            wined3d_device_incref(texture->wined3d_device = ddraw->wined3d_device);

            if (desc->dwFlags & DDSD_CKDESTOVERLAY)
                wined3d_texture_set_color_key(wined3d_texture, DDCKEY_DESTOVERLAY,
                        (struct wined3d_color_key *)&desc->u3.ddckCKDestOverlay);
            if (desc->dwFlags & DDSD_CKDESTBLT)
                wined3d_texture_set_color_key(wined3d_texture, DDCKEY_DESTBLT,
                        (struct wined3d_color_key *)&desc->ddckCKDestBlt);
            if (desc->dwFlags & DDSD_CKSRCOVERLAY)
                wined3d_texture_set_color_key(wined3d_texture, DDCKEY_SRCOVERLAY,
                        (struct wined3d_color_key *)&desc->ddckCKSrcOverlay);
            if (desc->dwFlags & DDSD_CKSRCBLT)
                wined3d_texture_set_color_key(wined3d_texture, DDCKEY_SRCBLT,
                        (struct wined3d_color_key *)&desc->ddckCKSrcBlt);

            *attach = last;
            attach = &last->complex_array[0];
        }
        *attach = root;
    }

    if (surface_desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        ddraw->primary = root;
        ddraw->gdi_surface = root->wined3d_texture;
    }
    *surface = root;

    return DD_OK;

fail:
    if (version == 7)
        IDirectDrawSurface7_Release(&root->IDirectDrawSurface7_iface);
    else if (version == 4)
        IDirectDrawSurface4_Release(&root->IDirectDrawSurface4_iface);
    else
        IDirectDrawSurface_Release(&root->IDirectDrawSurface_iface);

    return hr;
}

void ddraw_surface_init(struct ddraw_surface *surface, struct ddraw *ddraw,
        struct wined3d_texture *wined3d_texture, unsigned int sub_resource_idx,
        const struct wined3d_parent_ops **parent_ops)
{
    struct ddraw_texture *texture = wined3d_texture_get_parent(wined3d_texture);
    unsigned int texture_level, row_pitch, slice_pitch;
    DDSURFACEDESC2 *desc = &surface->surface_desc;
    unsigned int version = texture->version;

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
        surface->texture_outer = (IUnknown *)&surface->IDirectDrawSurface7_iface;
    }
    else if (version == 4)
    {
        surface->ref4 = 1;
        surface->texture_outer = (IUnknown *)&surface->IDirectDrawSurface4_iface;
    }
    else
    {
        surface->ref1 = 1;
        surface->texture_outer = (IUnknown *)&surface->IDirectDrawSurface_iface;
    }

    *desc = texture->surface_desc;
    surface->first_attached = surface;

    texture_level = desc->ddsCaps.dwCaps & DDSCAPS_MIPMAP ? sub_resource_idx % desc->u2.dwMipMapCount : 0;
    wined3d_texture_get_pitch(wined3d_texture, texture_level, &row_pitch, &slice_pitch);
    if (format_is_compressed(&desc->u4.ddpfPixelFormat))
    {
        if (desc->dwFlags & DDSD_LPSURFACE)
            desc->u1.dwLinearSize = ~0u;
        else
            desc->u1.dwLinearSize = slice_pitch;
        desc->dwFlags |= DDSD_LINEARSIZE;
        desc->dwFlags &= ~(DDSD_LPSURFACE | DDSD_PITCH);
    }
    else
    {
        if (!(desc->dwFlags & DDSD_LPSURFACE))
            desc->u1.lPitch = row_pitch;
        desc->dwFlags |= DDSD_PITCH;
        desc->dwFlags &= ~(DDSD_LPSURFACE | DDSD_LINEARSIZE);
    }
    desc->lpSurface = NULL;

    wined3d_texture_incref(surface->wined3d_texture = wined3d_texture);
    surface->sub_resource_idx = sub_resource_idx;
    *parent_ops = &ddraw_surface_wined3d_parent_ops;

    wined3d_private_store_init(&surface->private_store);
}

static void STDMETHODCALLTYPE view_wined3d_object_destroyed(void *parent)
{
    struct ddraw_surface *surface = parent;

    /* If the surface reference count drops to zero, we release our reference
     * to the view, but don't clear the pointer yet, in case e.g. a
     * GetRenderTarget() call brings the surface back before the view is
     * actually destroyed. When the view is destroyed, we need to clear the
     * pointer, or a subsequent surface AddRef() would reference it again.
     *
     * This is safe because as long as the view still has a reference to the
     * texture, the surface is also still alive, and we're called before the
     * view releases that reference. */
    surface->wined3d_rtv = NULL;
}

static const struct wined3d_parent_ops ddraw_view_wined3d_parent_ops =
{
    view_wined3d_object_destroyed,
};

struct wined3d_rendertarget_view *ddraw_surface_get_rendertarget_view(struct ddraw_surface *surface)
{
    HRESULT hr;

    if (surface->wined3d_rtv)
        return surface->wined3d_rtv;

    if (FAILED(hr = wined3d_rendertarget_view_create_from_sub_resource(surface->wined3d_texture,
            surface->sub_resource_idx, surface, &ddraw_view_wined3d_parent_ops, &surface->wined3d_rtv)))
    {
        ERR("Failed to create rendertarget view, hr %#x.\n", hr);
        return NULL;
    }

    return surface->wined3d_rtv;
}
