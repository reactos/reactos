/*
 * Implementation of IDirect3DRMDevice Interface
 *
 * Copyright 2011, 2012 André Hentschel
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

#include "d3drm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

static inline struct d3drm_device *impl_from_IDirect3DRMDevice(IDirect3DRMDevice *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_device, IDirect3DRMDevice_iface);
}

static inline struct d3drm_device *impl_from_IDirect3DRMDevice2(IDirect3DRMDevice2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_device, IDirect3DRMDevice2_iface);
}

static inline struct d3drm_device *impl_from_IDirect3DRMDevice3(IDirect3DRMDevice3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_device, IDirect3DRMDevice3_iface);
}

void d3drm_device_destroy(struct d3drm_device *device)
{
    d3drm_object_cleanup((IDirect3DRMObject *)&device->IDirect3DRMDevice_iface, &device->obj);
    if (device->device)
    {
        TRACE("Releasing attached ddraw interfaces.\n");
        IDirect3DDevice_Release(device->device);
    }
    if (device->render_target)
        IDirectDrawSurface_Release(device->render_target);
    if (device->primary_surface)
    {
        TRACE("Releasing primary surface and attached clipper.\n");
        IDirectDrawSurface_Release(device->primary_surface);
        IDirectDrawClipper_Release(device->clipper);
    }
    if (device->ddraw)
    {
        IDirectDraw_Release(device->ddraw);
        IDirect3DRM_Release(device->d3drm);
    }
    free(device);
}

static inline struct d3drm_device *impl_from_IDirect3DRMWinDevice(IDirect3DRMWinDevice *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_device, IDirect3DRMWinDevice_iface);
}

HRESULT d3drm_device_create_surfaces_from_clipper(struct d3drm_device *object, IDirectDraw *ddraw, IDirectDrawClipper *clipper, int width, int height, IDirectDrawSurface **surface)
{
    DDSURFACEDESC surface_desc;
    IDirectDrawSurface *primary_surface, *render_target;
    HWND window;
    HRESULT hr;

    hr = IDirectDrawClipper_GetHWnd(clipper, &window);
    if (FAILED(hr))
        return hr;

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    if (FAILED(hr))
        return hr;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &primary_surface, NULL);
    if (FAILED(hr))
        return hr;
    hr = IDirectDrawSurface_SetClipper(primary_surface, clipper);
    if (FAILED(hr))
    {
        IDirectDrawSurface_Release(primary_surface);
        return hr;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = width;
    surface_desc.dwHeight = height;

    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &render_target, NULL);
    if (FAILED(hr))
    {
        IDirectDrawSurface_Release(primary_surface);
        return hr;
    }

    object->primary_surface = primary_surface;
    object->clipper = clipper;
    IDirectDrawClipper_AddRef(clipper);
    *surface = render_target;

    return D3DRM_OK;
}

HRESULT d3drm_device_init(struct d3drm_device *device, UINT version, IDirectDraw *ddraw, IDirectDrawSurface *surface,
            BOOL create_z_surface)
{
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    IDirectDrawSurface *ds = NULL;
    IDirect3DDevice *device1 = NULL;
    IDirect3DDevice2 *device2 = NULL;
    IDirect3DDevice3 *device3 = NULL;
    IDirect3D2 *d3d2 = NULL;
    IDirect3D3 *d3d3 = NULL;
    DDSURFACEDESC desc, surface_desc;
    HRESULT hr;

    device->ddraw = ddraw;
    IDirectDraw_AddRef(ddraw);
    IDirect3DRM_AddRef(device->d3drm);
    device->render_target = surface;
    IDirectDrawSurface_AddRef(surface);

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    if (FAILED(hr))
        return hr;

    if (!(desc.ddsCaps.dwCaps & DDSCAPS_3DDEVICE))
        return DDERR_INVALIDCAPS;

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &ds);
    if (SUCCEEDED(hr))
    {
        create_z_surface = FALSE;
        IDirectDrawSurface_Release(ds);
        ds = NULL;
    }

    if (create_z_surface)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        surface_desc.dwZBufferBitDepth = 16;
        surface_desc.dwWidth = desc.dwWidth;
        surface_desc.dwHeight = desc.dwHeight;
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &ds, NULL);
        if (FAILED(hr))
            return hr;

        hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
        IDirectDrawSurface_Release(ds);
        if (FAILED(hr))
            return hr;
    }

    if (version == 1)
        hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DRGBDevice, (void **)&device1);
    else if (version == 2)
    {
        IDirectDraw_QueryInterface(ddraw, &IID_IDirect3D2, (void**)&d3d2);
        hr = IDirect3D2_CreateDevice(d3d2, &IID_IDirect3DRGBDevice, surface, &device2);
        IDirect3D2_Release(d3d2);
    }
    else
    {
        IDirectDrawSurface4 *surface4 = NULL;

        IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface4, (void**)&surface4);
        IDirectDraw_QueryInterface(ddraw, &IID_IDirect3D3, (void**)&d3d3);
        hr = IDirect3D3_CreateDevice(d3d3, &IID_IDirect3DRGBDevice, surface4, &device3, NULL);
        IDirectDrawSurface4_Release(surface4);
        IDirect3D3_Release(d3d3);
    }
    if (FAILED(hr))
    {
        IDirectDrawSurface_DeleteAttachedSurface(surface, 0, ds);
        return hr;
    }

    if (version == 2)
    {
        hr = IDirect3DDevice2_QueryInterface(device2, &IID_IDirect3DDevice, (void**)&device1);
        IDirect3DDevice2_Release(device2);
        if (FAILED(hr))
        {
            IDirectDrawSurface_DeleteAttachedSurface(surface, 0, ds);
            return hr;
        }
    }
    else if (version == 3)
    {
        hr = IDirect3DDevice3_QueryInterface(device3, &IID_IDirect3DDevice, (void**)&device1);
        IDirect3DDevice3_Release(device3);
        if (FAILED(hr))
        {
            IDirectDrawSurface_DeleteAttachedSurface(surface, 0, ds);
            return hr;
        }
    }
    device->device = device1;
    device->width = desc.dwWidth;
    device->height = desc.dwHeight;

    return hr;
}

static HRESULT d3drm_device_set_ddraw_device_d3d(struct d3drm_device *device, IDirect3D *d3d, IDirect3DDevice *d3d_device)
{
    IDirectDraw *ddraw;
    IDirectDrawSurface *surface;
    IDirect3DDevice2 *d3d_device2 = NULL;
    DDSURFACEDESC desc;
    HRESULT hr;

    /* AddRef these interfaces beforehand for the intentional leak on reinitialization. */
    if (FAILED(hr = IDirect3D_QueryInterface(d3d, &IID_IDirectDraw, (void **)&ddraw)))
        return hr;
    IDirect3DRM_AddRef(device->d3drm);
    IDirect3DDevice_AddRef(d3d_device);

    /* Fetch render target and get width/height from there */
    if (FAILED(hr = IDirect3DDevice_QueryInterface(d3d_device, &IID_IDirectDrawSurface, (void **)&surface)))
    {
        if (FAILED(hr = IDirect3DDevice_QueryInterface(d3d_device, &IID_IDirect3DDevice2, (void **)&d3d_device2)))
            return hr;
        hr = IDirect3DDevice2_GetRenderTarget(d3d_device2, &surface);
        IDirect3DDevice2_Release(d3d_device2);
        if (FAILED(hr))
            return hr;
    }

    if (device->ddraw)
    {
        if (d3d_device2)
            IDirectDrawSurface_Release(surface);
        return D3DRMERR_BADOBJECT;
    }

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc);
    if (FAILED(hr))
    {
        IDirectDrawSurface_Release(surface);
        return hr;
    }

    device->ddraw = ddraw;
    device->width = desc.dwWidth;
    device->height = desc.dwHeight;
    device->device = d3d_device;
    device->render_target = surface;

    return hr;
}

static HRESULT WINAPI d3drm_device3_QueryInterface(IDirect3DRMDevice3 *iface, REFIID riid, void **out)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMDevice)
            || IsEqualGUID(riid, &IID_IDirect3DRMObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &device->IDirect3DRMDevice_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMDevice2))
    {
        *out = &device->IDirect3DRMDevice2_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMDevice3))
    {
        *out = &device->IDirect3DRMDevice3_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMWinDevice))
    {
        *out = &device->IDirect3DRMWinDevice_iface;
    }
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(riid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI d3drm_device2_QueryInterface(IDirect3DRMDevice2 *iface, REFIID riid, void **out)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return d3drm_device3_QueryInterface(&device->IDirect3DRMDevice3_iface, riid, out);
}

static HRESULT WINAPI d3drm_device1_QueryInterface(IDirect3DRMDevice *iface, REFIID riid, void **out)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return d3drm_device3_QueryInterface(&device->IDirect3DRMDevice3_iface, riid, out);
}

static ULONG WINAPI d3drm_device3_AddRef(IDirect3DRMDevice3 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);
    ULONG refcount = InterlockedIncrement(&device->obj.ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_device2_AddRef(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_AddRef(&device->IDirect3DRMDevice3_iface);
}

static ULONG WINAPI d3drm_device1_AddRef(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_AddRef(&device->IDirect3DRMDevice3_iface);
}

static ULONG WINAPI d3drm_device3_Release(IDirect3DRMDevice3 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);
    ULONG refcount = InterlockedDecrement(&device->obj.ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        d3drm_device_destroy(device);

    return refcount;
}

static ULONG WINAPI d3drm_device2_Release(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_Release(&device->IDirect3DRMDevice3_iface);
}

static ULONG WINAPI d3drm_device1_Release(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_Release(&device->IDirect3DRMDevice3_iface);
}

static HRESULT WINAPI d3drm_device3_Clone(IDirect3DRMDevice3 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device2_Clone(IDirect3DRMDevice2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, outer %p, iid %s, out %p\n", iface, outer, debugstr_guid(iid), out);

    return d3drm_device3_Clone(&device->IDirect3DRMDevice3_iface, outer, iid, out);
}

static HRESULT WINAPI d3drm_device1_Clone(IDirect3DRMDevice *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    return d3drm_device3_Clone(&device->IDirect3DRMDevice3_iface, outer, iid, out);
}

static HRESULT WINAPI d3drm_device3_AddDestroyCallback(IDirect3DRMDevice3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_object_add_destroy_callback(&device->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_device2_AddDestroyCallback(IDirect3DRMDevice2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_device3_AddDestroyCallback(&device->IDirect3DRMDevice3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_device1_AddDestroyCallback(IDirect3DRMDevice *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_device3_AddDestroyCallback(&device->IDirect3DRMDevice3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_device3_DeleteDestroyCallback(IDirect3DRMDevice3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_object_delete_destroy_callback(&device->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_device2_DeleteDestroyCallback(IDirect3DRMDevice2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_device3_DeleteDestroyCallback(&device->IDirect3DRMDevice3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_device1_DeleteDestroyCallback(IDirect3DRMDevice *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_device3_DeleteDestroyCallback(&device->IDirect3DRMDevice3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_device3_SetAppData(IDirect3DRMDevice3 *iface, DWORD data)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    device->obj.appdata = data;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device2_SetAppData(IDirect3DRMDevice2 *iface, DWORD data)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    return d3drm_device3_SetAppData(&device->IDirect3DRMDevice3_iface, data);
}

static HRESULT WINAPI d3drm_device1_SetAppData(IDirect3DRMDevice *iface, DWORD data)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    return d3drm_device3_SetAppData(&device->IDirect3DRMDevice3_iface, data);
}

static DWORD WINAPI d3drm_device3_GetAppData(IDirect3DRMDevice3 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p.\n", iface);

    return device->obj.appdata;
}

static DWORD WINAPI d3drm_device2_GetAppData(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetAppData(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device1_GetAppData(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetAppData(&device->IDirect3DRMDevice3_iface);
}

static HRESULT WINAPI d3drm_device3_SetName(IDirect3DRMDevice3 *iface, const char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_object_set_name(&device->obj, name);
}

static HRESULT WINAPI d3drm_device2_SetName(IDirect3DRMDevice2 *iface, const char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_device3_SetName(&device->IDirect3DRMDevice3_iface, name);
}

static HRESULT WINAPI d3drm_device1_SetName(IDirect3DRMDevice *iface, const char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_device3_SetName(&device->IDirect3DRMDevice3_iface, name);
}

static HRESULT WINAPI d3drm_device3_GetName(IDirect3DRMDevice3 *iface, DWORD *size, char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_name(&device->obj, size, name);
}

static HRESULT WINAPI d3drm_device2_GetName(IDirect3DRMDevice2 *iface, DWORD *size, char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_device3_GetName(&device->IDirect3DRMDevice3_iface, size, name);
}

static HRESULT WINAPI d3drm_device1_GetName(IDirect3DRMDevice *iface, DWORD *size, char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_device3_GetName(&device->IDirect3DRMDevice3_iface, size, name);
}

static HRESULT WINAPI d3drm_device3_GetClassName(IDirect3DRMDevice3 *iface, DWORD *size, char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_class_name(&device->obj, size, name);
}

static HRESULT WINAPI d3drm_device2_GetClassName(IDirect3DRMDevice2 *iface, DWORD *size, char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_device3_GetClassName(&device->IDirect3DRMDevice3_iface, size, name);
}

static HRESULT WINAPI d3drm_device1_GetClassName(IDirect3DRMDevice *iface, DWORD *size, char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_device3_GetClassName(&device->IDirect3DRMDevice3_iface, size, name);
}

static HRESULT WINAPI d3drm_device3_Init(IDirect3DRMDevice3 *iface, ULONG width, ULONG height)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    FIXME("iface %p, width %lu, height %lu stub!\n", iface, width, height);

    device->height = height;
    device->width = width;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device2_Init(IDirect3DRMDevice2 *iface, ULONG width, ULONG height)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, width %lu, height %lu.\n", iface, width, height);

    return d3drm_device3_Init(&device->IDirect3DRMDevice3_iface, width, height);
}

static HRESULT WINAPI d3drm_device1_Init(IDirect3DRMDevice *iface, ULONG width, ULONG height)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, width %lu, height %lu.\n", iface, width, height);

    return d3drm_device3_Init(&device->IDirect3DRMDevice3_iface, width, height);
}

static HRESULT WINAPI d3drm_device3_InitFromD3D(IDirect3DRMDevice3 *iface,
        IDirect3D *d3d, IDirect3DDevice *d3d_device)
{
    FIXME("iface %p, d3d %p, d3d_device %p stub!\n", iface, d3d, d3d_device);

    if (!d3d || !d3d_device)
        return D3DRMERR_BADVALUE;

    return E_NOINTERFACE;
}

static HRESULT WINAPI d3drm_device2_InitFromD3D(IDirect3DRMDevice2 *iface,
        IDirect3D *d3d, IDirect3DDevice *d3d_device)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, d3d %p, d3d_device %p.\n", iface, d3d, d3d_device);

    return d3drm_device3_InitFromD3D(&device->IDirect3DRMDevice3_iface, d3d, d3d_device);
}

static HRESULT WINAPI d3drm_device1_InitFromD3D(IDirect3DRMDevice *iface,
        IDirect3D *d3d, IDirect3DDevice *d3d_device)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, d3d %p, d3d_device %p.\n", iface, d3d, d3d_device);

    if (!d3d || !d3d_device)
        return D3DRMERR_BADVALUE;

    return d3drm_device_set_ddraw_device_d3d(device, d3d, d3d_device);
}

static HRESULT WINAPI d3drm_device3_InitFromClipper(IDirect3DRMDevice3 *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    FIXME("iface %p, clipper %p, guid %s, width %d, height %d stub!\n",
            iface, clipper, debugstr_guid(guid), width, height);

    device->height = height;
    device->width = width;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device2_InitFromClipper(IDirect3DRMDevice2 *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, clipper %p, guid %s, width %d, height %d.\n",
            iface, clipper, debugstr_guid(guid), width, height);

    return d3drm_device3_InitFromClipper(&device->IDirect3DRMDevice3_iface,
            clipper, guid, width, height);
}

static HRESULT WINAPI d3drm_device1_InitFromClipper(IDirect3DRMDevice *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, clipper %p, guid %s, width %d, height %d.\n",
        iface, clipper, debugstr_guid(guid), width, height);

    return d3drm_device3_InitFromClipper(&device->IDirect3DRMDevice3_iface,
        clipper, guid, width, height);
}

static HRESULT WINAPI d3drm_device3_Update(IDirect3DRMDevice3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device2_Update(IDirect3DRMDevice2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device1_Update(IDirect3DRMDevice *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device3_AddUpdateCallback(IDirect3DRMDevice3 *iface,
        D3DRMUPDATECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device2_AddUpdateCallback(IDirect3DRMDevice2 *iface,
        D3DRMUPDATECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device1_AddUpdateCallback(IDirect3DRMDevice *iface,
        D3DRMUPDATECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device3_DeleteUpdateCallback(IDirect3DRMDevice3 *iface,
        D3DRMUPDATECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device2_DeleteUpdateCallback(IDirect3DRMDevice2 *iface,
        D3DRMUPDATECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device1_DeleteUpdateCallback(IDirect3DRMDevice *iface,
        D3DRMUPDATECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device3_SetBufferCount(IDirect3DRMDevice3 *iface, DWORD count)
{
    FIXME("iface %p, count %lu stub!\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device2_SetBufferCount(IDirect3DRMDevice2 *iface, DWORD count)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, count %lu.\n", iface, count);

    return d3drm_device3_SetBufferCount(&device->IDirect3DRMDevice3_iface, count);
}

static HRESULT WINAPI d3drm_device1_SetBufferCount(IDirect3DRMDevice *iface, DWORD count)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, count %lu.\n", iface, count);

    return d3drm_device3_SetBufferCount(&device->IDirect3DRMDevice3_iface, count);
}

static DWORD WINAPI d3drm_device3_GetBufferCount(IDirect3DRMDevice3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_device2_GetBufferCount(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetBufferCount(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device1_GetBufferCount(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetBufferCount(&device->IDirect3DRMDevice3_iface);
}

static HRESULT WINAPI d3drm_device3_SetDither(IDirect3DRMDevice3 *iface, BOOL enable)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, enable %#x.\n", iface, enable);

    device->dither = enable;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device2_SetDither(IDirect3DRMDevice2 *iface, BOOL enable)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, enabled %#x.\n", iface, enable);

    return d3drm_device3_SetDither(&device->IDirect3DRMDevice3_iface, enable);
}

static HRESULT WINAPI d3drm_device1_SetDither(IDirect3DRMDevice *iface, BOOL enable)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, enabled %#x.\n", iface, enable);

    return d3drm_device3_SetDither(&device->IDirect3DRMDevice3_iface, enable);
}

static HRESULT WINAPI d3drm_device3_SetShades(IDirect3DRMDevice3 *iface, DWORD count)
{
    FIXME("iface %p, count %lu stub!\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device2_SetShades(IDirect3DRMDevice2 *iface, DWORD count)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, count %lu.\n", iface, count);

    return d3drm_device3_SetShades(&device->IDirect3DRMDevice3_iface, count);
}

static HRESULT WINAPI d3drm_device1_SetShades(IDirect3DRMDevice *iface, DWORD count)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, count %lu.\n", iface, count);

    return d3drm_device3_SetShades(&device->IDirect3DRMDevice3_iface, count);
}

static HRESULT WINAPI d3drm_device3_SetQuality(IDirect3DRMDevice3 *iface, D3DRMRENDERQUALITY quality)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, quality %lu.\n", iface, quality);

    device->quality = quality;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device2_SetQuality(IDirect3DRMDevice2 *iface, D3DRMRENDERQUALITY quality)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, quality %lu.\n", iface, quality);

    return d3drm_device3_SetQuality(&device->IDirect3DRMDevice3_iface, quality);
}

static HRESULT WINAPI d3drm_device1_SetQuality(IDirect3DRMDevice *iface, D3DRMRENDERQUALITY quality)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, quality %lu.\n", iface, quality);

    return d3drm_device3_SetQuality(&device->IDirect3DRMDevice3_iface, quality);
}

static HRESULT WINAPI d3drm_device3_SetTextureQuality(IDirect3DRMDevice3 *iface, D3DRMTEXTUREQUALITY quality)
{
    FIXME("iface %p, quality %u stub!\n", iface, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device2_SetTextureQuality(IDirect3DRMDevice2 *iface, D3DRMTEXTUREQUALITY quality)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, quality %u.\n", iface, quality);

    return d3drm_device3_SetTextureQuality(&device->IDirect3DRMDevice3_iface, quality);
}

static HRESULT WINAPI d3drm_device1_SetTextureQuality(IDirect3DRMDevice *iface, D3DRMTEXTUREQUALITY quality)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, quality %u.\n", iface, quality);

    return d3drm_device3_SetTextureQuality(&device->IDirect3DRMDevice3_iface, quality);
}

static HRESULT WINAPI d3drm_device3_GetViewports(IDirect3DRMDevice3 *iface, IDirect3DRMViewportArray **array)
{
    FIXME("iface %p, array %p stub!\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device2_GetViewports(IDirect3DRMDevice2 *iface, IDirect3DRMViewportArray **array)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, array %p.\n", iface, array);

    return d3drm_device3_GetViewports(&device->IDirect3DRMDevice3_iface, array);
}

static HRESULT WINAPI d3drm_device1_GetViewports(IDirect3DRMDevice *iface, IDirect3DRMViewportArray **array)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, array %p.\n", iface, array);

    return d3drm_device3_GetViewports(&device->IDirect3DRMDevice3_iface, array);
}

static BOOL WINAPI d3drm_device3_GetDither(IDirect3DRMDevice3 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p.\n", iface);

    return device->dither;
}

static BOOL WINAPI d3drm_device2_GetDither(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetDither(&device->IDirect3DRMDevice3_iface);
}

static BOOL WINAPI d3drm_device1_GetDither(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetDither(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device3_GetShades(IDirect3DRMDevice3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_device2_GetShades(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetShades(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device1_GetShades(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetShades(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device3_GetHeight(IDirect3DRMDevice3 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p.\n", iface);

    return device->height;
}

static DWORD WINAPI d3drm_device2_GetHeight(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetHeight(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device1_GetHeight(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetHeight(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device3_GetWidth(IDirect3DRMDevice3 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p.\n", iface);

    return device->width;
}

static DWORD WINAPI d3drm_device2_GetWidth(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetWidth(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device1_GetWidth(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetWidth(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device3_GetTrianglesDrawn(IDirect3DRMDevice3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_device2_GetTrianglesDrawn(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetTrianglesDrawn(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device1_GetTrianglesDrawn(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetTrianglesDrawn(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device3_GetWireframeOptions(IDirect3DRMDevice3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_device2_GetWireframeOptions(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetWireframeOptions(&device->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI d3drm_device1_GetWireframeOptions(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetWireframeOptions(&device->IDirect3DRMDevice3_iface);
}

static D3DRMRENDERQUALITY WINAPI d3drm_device3_GetQuality(IDirect3DRMDevice3 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p.\n", iface);

    return device->quality;
}

static D3DRMRENDERQUALITY WINAPI d3drm_device2_GetQuality(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetQuality(&device->IDirect3DRMDevice3_iface);
}

static D3DRMRENDERQUALITY WINAPI d3drm_device1_GetQuality(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetQuality(&device->IDirect3DRMDevice3_iface);
}

static D3DCOLORMODEL WINAPI d3drm_device3_GetColorModel(IDirect3DRMDevice3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static D3DCOLORMODEL WINAPI d3drm_device2_GetColorModel(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetColorModel(&device->IDirect3DRMDevice3_iface);
}

static D3DCOLORMODEL WINAPI d3drm_device1_GetColorModel(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetColorModel(&device->IDirect3DRMDevice3_iface);
}

static D3DRMTEXTUREQUALITY WINAPI d3drm_device3_GetTextureQuality(IDirect3DRMDevice3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static D3DRMTEXTUREQUALITY WINAPI d3drm_device2_GetTextureQuality(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetTextureQuality(&device->IDirect3DRMDevice3_iface);
}

static D3DRMTEXTUREQUALITY WINAPI d3drm_device1_GetTextureQuality(IDirect3DRMDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetTextureQuality(&device->IDirect3DRMDevice3_iface);
}

static HRESULT WINAPI d3drm_device3_GetDirect3DDevice(IDirect3DRMDevice3 *iface, IDirect3DDevice **d3d_device)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);
    TRACE("iface %p, d3d_device %p!\n", iface, d3d_device);

    *d3d_device = device->device;
    IDirect3DDevice_AddRef(*d3d_device);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device2_GetDirect3DDevice(IDirect3DRMDevice2 *iface, IDirect3DDevice **d3d_device)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, d3d_device %p.\n", iface, d3d_device);

    return d3drm_device3_GetDirect3DDevice(&device->IDirect3DRMDevice3_iface, d3d_device);
}

static HRESULT WINAPI d3drm_device1_GetDirect3DDevice(IDirect3DRMDevice *iface, IDirect3DDevice **d3d_device)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice(iface);

    TRACE("iface %p, d3d_device %p.\n", iface, d3d_device);

    return d3drm_device3_GetDirect3DDevice(&device->IDirect3DRMDevice3_iface, d3d_device);
}

static HRESULT WINAPI d3drm_device3_InitFromD3D2(IDirect3DRMDevice3 *iface,
        IDirect3D2 *d3d, IDirect3DDevice2 *d3d_device)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);
    IDirect3D *d3d1;
    IDirect3DDevice *d3d_device1;
    HRESULT hr;

    TRACE("iface %p, d3d %p, d3d_device %p.\n", iface, d3d, d3d_device);

    if (!d3d || !d3d_device)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = IDirect3D2_QueryInterface(d3d, &IID_IDirect3D, (void **)&d3d1)))
        return hr;
    if (FAILED(hr = IDirect3DDevice2_QueryInterface(d3d_device, &IID_IDirect3DDevice, (void **)&d3d_device1)))
    {
        IDirect3D_Release(d3d1);
        return hr;
    }

    hr = d3drm_device_set_ddraw_device_d3d(device, d3d1, d3d_device1);
    IDirect3D_Release(d3d1);
    IDirect3DDevice_Release(d3d_device1);

    return hr;
}

static HRESULT WINAPI d3drm_device2_InitFromD3D2(IDirect3DRMDevice2 *iface,
        IDirect3D2 *d3d, IDirect3DDevice2 *d3d_device)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, d3d %p, d3d_device %p.\n", iface, d3d, d3d_device);

    return d3drm_device3_InitFromD3D2(&device->IDirect3DRMDevice3_iface, d3d, d3d_device);
}

static HRESULT WINAPI d3drm_device3_InitFromSurface(IDirect3DRMDevice3 *iface,
        GUID *guid, IDirectDraw *ddraw, IDirectDrawSurface *backbuffer)
{
    FIXME("iface %p, guid %s, ddraw %p, backbuffer %p stub!\n",
            iface, debugstr_guid(guid), ddraw, backbuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device2_InitFromSurface(IDirect3DRMDevice2 *iface,
        GUID *guid, IDirectDraw *ddraw, IDirectDrawSurface *backbuffer)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, guid %s, ddraw %p, backbuffer %p.\n",
            iface, debugstr_guid(guid), ddraw, backbuffer);

    return d3drm_device3_InitFromSurface(&device->IDirect3DRMDevice3_iface, guid, ddraw, backbuffer);
}

static HRESULT WINAPI d3drm_device3_SetRenderMode(IDirect3DRMDevice3 *iface, DWORD flags)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    device->rendermode = flags;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device2_SetRenderMode(IDirect3DRMDevice2 *iface, DWORD flags)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return d3drm_device3_SetRenderMode(&device->IDirect3DRMDevice3_iface, flags);
}

static DWORD WINAPI d3drm_device3_GetRenderMode(IDirect3DRMDevice3 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p.\n", iface);

    return device->rendermode;
}

static DWORD WINAPI d3drm_device2_GetRenderMode(IDirect3DRMDevice2 *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetRenderMode(&device->IDirect3DRMDevice3_iface);
}

static HRESULT WINAPI d3drm_device3_GetDirect3DDevice2(IDirect3DRMDevice3 *iface, IDirect3DDevice2 **d3d_device)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice3(iface);

    TRACE("iface %p, d3d_device %p.\n", iface, d3d_device);

    if (FAILED(IDirect3DDevice_QueryInterface(device->device, &IID_IDirect3DDevice2, (void**)d3d_device)))
        return D3DRMERR_BADOBJECT;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device2_GetDirect3DDevice2(IDirect3DRMDevice2 *iface, IDirect3DDevice2 **d3d_device)
{
    struct d3drm_device *device = impl_from_IDirect3DRMDevice2(iface);

    TRACE("iface %p, d3d_device %p.\n", iface, d3d_device);

    IDirect3DDevice_QueryInterface(device->device, &IID_IDirect3DDevice2, (void**)d3d_device);

    /* d3drm returns D3DRM_OK even if the call fails. */
    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device3_FindPreferredTextureFormat(IDirect3DRMDevice3 *iface,
        DWORD bitdepths, DWORD flags, DDPIXELFORMAT *pf)
{
    FIXME("iface %p, bitdepths %lu, flags %#lx, pf %p stub!\n", iface, bitdepths, flags, pf);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device3_RenderStateChange(IDirect3DRMDevice3 *iface,
        D3DRENDERSTATETYPE state, DWORD value, DWORD flags)
{
    FIXME("iface %p, state %#x, value %#lx, flags %#lx stub!\n", iface, state, value, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device3_LightStateChange(IDirect3DRMDevice3 *iface,
        D3DLIGHTSTATETYPE state, DWORD value, DWORD flags)
{
    FIXME("iface %p, state %#x, value %#lx, flags %#lx stub!\n", iface, state, value, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device3_GetStateChangeOptions(IDirect3DRMDevice3 *iface,
        DWORD state_class, DWORD state_idx, DWORD *flags)
{
    FIXME("iface %p, state_class %#lx, state_idx %#lx, flags %p stub!\n",
            iface, state_class, state_idx, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device3_SetStateChangeOptions(IDirect3DRMDevice3 *iface,
        DWORD state_class, DWORD state_idx, DWORD flags)
{
    FIXME("iface %p, state_class %#lx, state_idx %#lx, flags %#lx stub!\n",
            iface, state_class, state_idx, flags);

    return E_NOTIMPL;
}

static const struct IDirect3DRMDevice3Vtbl d3drm_device3_vtbl =
{
    d3drm_device3_QueryInterface,
    d3drm_device3_AddRef,
    d3drm_device3_Release,
    d3drm_device3_Clone,
    d3drm_device3_AddDestroyCallback,
    d3drm_device3_DeleteDestroyCallback,
    d3drm_device3_SetAppData,
    d3drm_device3_GetAppData,
    d3drm_device3_SetName,
    d3drm_device3_GetName,
    d3drm_device3_GetClassName,
    d3drm_device3_Init,
    d3drm_device3_InitFromD3D,
    d3drm_device3_InitFromClipper,
    d3drm_device3_Update,
    d3drm_device3_AddUpdateCallback,
    d3drm_device3_DeleteUpdateCallback,
    d3drm_device3_SetBufferCount,
    d3drm_device3_GetBufferCount,
    d3drm_device3_SetDither,
    d3drm_device3_SetShades,
    d3drm_device3_SetQuality,
    d3drm_device3_SetTextureQuality,
    d3drm_device3_GetViewports,
    d3drm_device3_GetDither,
    d3drm_device3_GetShades,
    d3drm_device3_GetHeight,
    d3drm_device3_GetWidth,
    d3drm_device3_GetTrianglesDrawn,
    d3drm_device3_GetWireframeOptions,
    d3drm_device3_GetQuality,
    d3drm_device3_GetColorModel,
    d3drm_device3_GetTextureQuality,
    d3drm_device3_GetDirect3DDevice,
    d3drm_device3_InitFromD3D2,
    d3drm_device3_InitFromSurface,
    d3drm_device3_SetRenderMode,
    d3drm_device3_GetRenderMode,
    d3drm_device3_GetDirect3DDevice2,
    d3drm_device3_FindPreferredTextureFormat,
    d3drm_device3_RenderStateChange,
    d3drm_device3_LightStateChange,
    d3drm_device3_GetStateChangeOptions,
    d3drm_device3_SetStateChangeOptions,
};

static const struct IDirect3DRMDevice2Vtbl d3drm_device2_vtbl =
{
    d3drm_device2_QueryInterface,
    d3drm_device2_AddRef,
    d3drm_device2_Release,
    d3drm_device2_Clone,
    d3drm_device2_AddDestroyCallback,
    d3drm_device2_DeleteDestroyCallback,
    d3drm_device2_SetAppData,
    d3drm_device2_GetAppData,
    d3drm_device2_SetName,
    d3drm_device2_GetName,
    d3drm_device2_GetClassName,
    d3drm_device2_Init,
    d3drm_device2_InitFromD3D,
    d3drm_device2_InitFromClipper,
    d3drm_device2_Update,
    d3drm_device2_AddUpdateCallback,
    d3drm_device2_DeleteUpdateCallback,
    d3drm_device2_SetBufferCount,
    d3drm_device2_GetBufferCount,
    d3drm_device2_SetDither,
    d3drm_device2_SetShades,
    d3drm_device2_SetQuality,
    d3drm_device2_SetTextureQuality,
    d3drm_device2_GetViewports,
    d3drm_device2_GetDither,
    d3drm_device2_GetShades,
    d3drm_device2_GetHeight,
    d3drm_device2_GetWidth,
    d3drm_device2_GetTrianglesDrawn,
    d3drm_device2_GetWireframeOptions,
    d3drm_device2_GetQuality,
    d3drm_device2_GetColorModel,
    d3drm_device2_GetTextureQuality,
    d3drm_device2_GetDirect3DDevice,
    d3drm_device2_InitFromD3D2,
    d3drm_device2_InitFromSurface,
    d3drm_device2_SetRenderMode,
    d3drm_device2_GetRenderMode,
    d3drm_device2_GetDirect3DDevice2,
};

static const struct IDirect3DRMDeviceVtbl d3drm_device1_vtbl =
{
    d3drm_device1_QueryInterface,
    d3drm_device1_AddRef,
    d3drm_device1_Release,
    d3drm_device1_Clone,
    d3drm_device1_AddDestroyCallback,
    d3drm_device1_DeleteDestroyCallback,
    d3drm_device1_SetAppData,
    d3drm_device1_GetAppData,
    d3drm_device1_SetName,
    d3drm_device1_GetName,
    d3drm_device1_GetClassName,
    d3drm_device1_Init,
    d3drm_device1_InitFromD3D,
    d3drm_device1_InitFromClipper,
    d3drm_device1_Update,
    d3drm_device1_AddUpdateCallback,
    d3drm_device1_DeleteUpdateCallback,
    d3drm_device1_SetBufferCount,
    d3drm_device1_GetBufferCount,
    d3drm_device1_SetDither,
    d3drm_device1_SetShades,
    d3drm_device1_SetQuality,
    d3drm_device1_SetTextureQuality,
    d3drm_device1_GetViewports,
    d3drm_device1_GetDither,
    d3drm_device1_GetShades,
    d3drm_device1_GetHeight,
    d3drm_device1_GetWidth,
    d3drm_device1_GetTrianglesDrawn,
    d3drm_device1_GetWireframeOptions,
    d3drm_device1_GetQuality,
    d3drm_device1_GetColorModel,
    d3drm_device1_GetTextureQuality,
    d3drm_device1_GetDirect3DDevice,
};

static HRESULT WINAPI d3drm_device_win_QueryInterface(IDirect3DRMWinDevice *iface, REFIID riid, void **out)
{
    struct d3drm_device *device = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return d3drm_device3_QueryInterface(&device->IDirect3DRMDevice3_iface, riid, out);
}

static ULONG WINAPI d3drm_device_win_AddRef(IDirect3DRMWinDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_AddRef(&device->IDirect3DRMDevice3_iface);
}

static ULONG WINAPI d3drm_device_win_Release(IDirect3DRMWinDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_Release(&device->IDirect3DRMDevice3_iface);
}

static HRESULT WINAPI d3drm_device_win_Clone(IDirect3DRMWinDevice *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    struct d3drm_device *device = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("iface %p, outer %p, iid %s, out %p\n", iface, outer, debugstr_guid(iid), out);

    return d3drm_device3_Clone(&device->IDirect3DRMDevice3_iface, outer, iid, out);
}

static HRESULT WINAPI d3drm_device_win_AddDestroyCallback(IDirect3DRMWinDevice *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device_win_DeleteDestroyCallback(IDirect3DRMWinDevice *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_device_win_SetAppData(IDirect3DRMWinDevice *iface, DWORD data)
{
    struct d3drm_device *device = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    return d3drm_device3_SetAppData(&device->IDirect3DRMDevice3_iface, data);
}

static DWORD WINAPI d3drm_device_win_GetAppData(IDirect3DRMWinDevice *iface)
{
    struct d3drm_device *device = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_device3_GetAppData(&device->IDirect3DRMDevice3_iface);
}

static HRESULT WINAPI d3drm_device_win_SetName(IDirect3DRMWinDevice *iface, const char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_device3_SetName(&device->IDirect3DRMDevice3_iface, name);
}

static HRESULT WINAPI d3drm_device_win_GetName(IDirect3DRMWinDevice *iface, DWORD *size, char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_device3_GetName(&device->IDirect3DRMDevice3_iface, size, name);
}

static HRESULT WINAPI d3drm_device_win_GetClassName(IDirect3DRMWinDevice *iface, DWORD *size, char *name)
{
    struct d3drm_device *device = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_device3_GetClassName(&device->IDirect3DRMDevice3_iface, size, name);
}

static HRESULT WINAPI d3drm_device_win_HandlePaint(IDirect3DRMWinDevice *iface, HDC dc)
{
    FIXME("iface %p, dc %p stub!\n", iface, dc);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_device_win_HandleActivate(IDirect3DRMWinDevice *iface, WORD wparam)
{
    FIXME("iface %p, wparam %#x stub!\n", iface, wparam);

    return D3DRM_OK;
}

static const struct IDirect3DRMWinDeviceVtbl d3drm_device_win_vtbl =
{
    d3drm_device_win_QueryInterface,
    d3drm_device_win_AddRef,
    d3drm_device_win_Release,
    d3drm_device_win_Clone,
    d3drm_device_win_AddDestroyCallback,
    d3drm_device_win_DeleteDestroyCallback,
    d3drm_device_win_SetAppData,
    d3drm_device_win_GetAppData,
    d3drm_device_win_SetName,
    d3drm_device_win_GetName,
    d3drm_device_win_GetClassName,
    d3drm_device_win_HandlePaint,
    d3drm_device_win_HandleActivate,
};

struct d3drm_device *unsafe_impl_from_IDirect3DRMDevice3(IDirect3DRMDevice3 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3drm_device3_vtbl);

    return impl_from_IDirect3DRMDevice3(iface);
}

HRESULT d3drm_device_create(struct d3drm_device **device, IDirect3DRM *d3drm)
{
    static const char classname[] = "Device";
    struct d3drm_device *object;

    TRACE("device %p, d3drm %p.\n", device, d3drm);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMDevice_iface.lpVtbl = &d3drm_device1_vtbl;
    object->IDirect3DRMDevice2_iface.lpVtbl = &d3drm_device2_vtbl;
    object->IDirect3DRMDevice3_iface.lpVtbl = &d3drm_device3_vtbl;
    object->IDirect3DRMWinDevice_iface.lpVtbl = &d3drm_device_win_vtbl;
    object->d3drm = d3drm;
    d3drm_object_init(&object->obj, classname);

    *device = object;

    return D3DRM_OK;
}
