/*
 * Implementation of IDirect3DRM Interface
 *
 * Copyright 2010, 2012 Christian Costa
 * Copyright 2011 André Hentschel
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

static const char* get_IID_string(const GUID* guid)
{
    if (IsEqualGUID(guid, &IID_IDirect3DRMFrame))
        return "IID_IDirect3DRMFrame";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMFrame2))
        return "IID_IDirect3DRMFrame2";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMFrame3))
        return "IID_IDirect3DRMFrame3";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMMeshBuilder))
        return "IID_IDirect3DRMMeshBuilder";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMMeshBuilder2))
        return "IID_IDirect3DRMMeshBuilder2";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMMeshBuilder3))
        return "IID_IDirect3DRMMeshBuilder3";

    return "?";
}

struct d3drm
{
    IDirect3DRM IDirect3DRM_iface;
    IDirect3DRM2 IDirect3DRM2_iface;
    IDirect3DRM3 IDirect3DRM3_iface;
    LONG ref1, ref2, ref3, iface_count;
};

static inline struct d3drm *impl_from_IDirect3DRM(IDirect3DRM *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm, IDirect3DRM_iface);
}

static inline struct d3drm *impl_from_IDirect3DRM2(IDirect3DRM2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm, IDirect3DRM2_iface);
}

static inline struct d3drm *impl_from_IDirect3DRM3(IDirect3DRM3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm, IDirect3DRM3_iface);
}

static void d3drm_destroy(struct d3drm *d3drm)
{
    HeapFree(GetProcessHeap(), 0, d3drm);
    TRACE("d3drm object %p is being destroyed.\n", d3drm);
}

static HRESULT WINAPI d3drm1_QueryInterface(IDirect3DRM *iface, REFIID riid, void **out)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRM)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &d3drm->IDirect3DRM_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRM2))
    {
        *out = &d3drm->IDirect3DRM2_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRM3))
    {
        *out = &d3drm->IDirect3DRM3_iface;
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

static ULONG WINAPI d3drm1_AddRef(IDirect3DRM *iface)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);
    ULONG refcount = InterlockedIncrement(&d3drm->ref1);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
        InterlockedIncrement(&d3drm->iface_count);

    return refcount;
}

static ULONG WINAPI d3drm1_Release(IDirect3DRM *iface)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);
    ULONG refcount = InterlockedDecrement(&d3drm->ref1);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount && !InterlockedDecrement(&d3drm->iface_count))
        d3drm_destroy(d3drm);

    return refcount;
}

static HRESULT WINAPI d3drm1_CreateObject(IDirect3DRM *iface,
        REFCLSID clsid, IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, clsid %s, outer %p, iid %s, out %p stub!\n",
            iface, debugstr_guid(clsid), outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_CreateFrame(IDirect3DRM *iface,
        IDirect3DRMFrame *parent_frame, IDirect3DRMFrame **frame)
{
    TRACE("iface %p, parent_frame %p, frame %p.\n", iface, parent_frame, frame);

    return Direct3DRMFrame_create(&IID_IDirect3DRMFrame, (IUnknown *)parent_frame, (IUnknown **)frame);
}

static HRESULT WINAPI d3drm1_CreateMesh(IDirect3DRM *iface, IDirect3DRMMesh **mesh)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return IDirect3DRM3_CreateMesh(&d3drm->IDirect3DRM3_iface, mesh);
}

static HRESULT WINAPI d3drm1_CreateMeshBuilder(IDirect3DRM *iface, IDirect3DRMMeshBuilder **mesh_builder)
{
    TRACE("iface %p, mesh_builder %p.\n", iface, mesh_builder);

    return Direct3DRMMeshBuilder_create(&IID_IDirect3DRMMeshBuilder, (IUnknown **)mesh_builder);
}

static HRESULT WINAPI d3drm1_CreateFace(IDirect3DRM *iface, IDirect3DRMFace **face)
{
    TRACE("iface %p, face %p.\n", iface, face);

    return Direct3DRMFace_create(&IID_IDirect3DRMFace, (IUnknown **)face);
}

static HRESULT WINAPI d3drm1_CreateAnimation(IDirect3DRM *iface, IDirect3DRMAnimation **animation)
{
    FIXME("iface %p, animation %p stub!\n", iface, animation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_CreateAnimationSet(IDirect3DRM *iface, IDirect3DRMAnimationSet **set)
{
    FIXME("iface %p, set %p stub!\n", iface, set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_CreateTexture(IDirect3DRM *iface,
        D3DRMIMAGE *image, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, image %p, texture %p partial stub.\n", iface, image, texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture, (IUnknown **)texture);
}

static HRESULT WINAPI d3drm1_CreateLight(IDirect3DRM *iface,
        D3DRMLIGHTTYPE type, D3DCOLOR color, IDirect3DRMLight **light)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, type %#x, color 0x%08x, light %p.\n", iface, type, color, light);

    return IDirect3DRM3_CreateLight(&d3drm->IDirect3DRM3_iface, type, color, light);
}

static HRESULT WINAPI d3drm1_CreateLightRGB(IDirect3DRM *iface, D3DRMLIGHTTYPE type,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue, IDirect3DRMLight **light)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, type %#x, red %.8e, green %.8e, blue %.8e, light %p.\n",
            iface, type, red, green, blue, light);

    return IDirect3DRM3_CreateLightRGB(&d3drm->IDirect3DRM3_iface, type, red, green, blue, light);
}

static HRESULT WINAPI d3drm1_CreateMaterial(IDirect3DRM *iface,
        D3DVALUE power, IDirect3DRMMaterial **material)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, power %.8e, material %p.\n", iface, power, material);

    return IDirect3DRM3_CreateMaterial(&d3drm->IDirect3DRM3_iface, power, (IDirect3DRMMaterial2 **)material);
}

static HRESULT WINAPI d3drm1_CreateDevice(IDirect3DRM *iface,
        DWORD width, DWORD height, IDirect3DRMDevice **device)
{
    FIXME("iface %p, width %u, height %u, device %p partial stub!\n", iface, width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice, (IUnknown **)device);
}

static HRESULT WINAPI d3drm1_CreateDeviceFromSurface(IDirect3DRM *iface, GUID *guid,
        IDirectDraw *ddraw, IDirectDrawSurface *backbuffer, IDirect3DRMDevice **device)
{
    FIXME("iface %p, guid %s, ddraw %p, backbuffer %p, device %p partial stub.\n",
            iface, debugstr_guid(guid), ddraw, backbuffer, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice, (IUnknown **)device);
}

static HRESULT WINAPI d3drm1_CreateDeviceFromD3D(IDirect3DRM *iface,
        IDirect3D *d3d, IDirect3DDevice *d3d_device, IDirect3DRMDevice **device)
{
    FIXME("iface %p, d3d %p, d3d_device %p, device %p partial stub.\n",
            iface, d3d, d3d_device, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice, (IUnknown **)device);
}

static HRESULT WINAPI d3drm1_CreateDeviceFromClipper(IDirect3DRM *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height,
        IDirect3DRMDevice **device)
{
    FIXME("iface %p, clipper %p, guid %s, width %d, height %d, device %p.\n",
            iface, clipper, debugstr_guid(guid), width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice, (IUnknown **)device);
}

static HRESULT WINAPI d3drm1_CreateTextureFromSurface(IDirect3DRM *iface,
        IDirectDrawSurface *surface, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, surface %p, texture %p stub!\n", iface, surface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_CreateShadow(IDirect3DRM *iface, IDirect3DRMVisual *visual,
        IDirect3DRMLight *light, D3DVALUE px, D3DVALUE py, D3DVALUE pz, D3DVALUE nx, D3DVALUE ny, D3DVALUE nz,
        IDirect3DRMVisual **shadow)
{
    FIXME("iface %p, visual %p, light %p, px %.8e, py %.8e, pz %.8e, nx %.8e, ny %.8e, nz %.8e, shadow %p stub!\n",
            iface, visual, light, px, py, pz, nx, ny, nz, shadow);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_CreateViewport(IDirect3DRM *iface, IDirect3DRMDevice *device,
        IDirect3DRMFrame *camera, DWORD x, DWORD y, DWORD width, DWORD height, IDirect3DRMViewport **viewport)
{
    FIXME("iface %p, device %p, camera %p, x %u, y %u, width %u, height %u, viewport %p partial stub!\n",
            iface, device, camera, x, y, width, height, viewport);

    return Direct3DRMViewport_create(&IID_IDirect3DRMViewport, (IUnknown **)viewport);
}

static HRESULT WINAPI d3drm1_CreateWrap(IDirect3DRM *iface, D3DRMWRAPTYPE type, IDirect3DRMFrame *frame,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv,
        IDirect3DRMWrap **wrap)
{
    FIXME("iface %p, type %#x, frame %p, ox %.8e, oy %.8e, oz %.8e, dx %.8e, dy %.8e, dz %.8e, "
            "ux %.8e, uy %.8e, uz %.8e, ou %.8e, ov %.8e, su %.8e, sv %.8e, wrap %p stub!\n",
            iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, wrap);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_CreateUserVisual(IDirect3DRM *iface,
        D3DRMUSERVISUALCALLBACK cb, void *ctx, IDirect3DRMUserVisual **visual)
{
    FIXME("iface %p, cb %p, ctx %p visual %p stub!\n", iface, cb, ctx, visual);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_LoadTexture(IDirect3DRM *iface,
        const char *filename, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, filename %s, texture %p stub!\n", iface, debugstr_a(filename), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture, (IUnknown **)texture);
}

static HRESULT WINAPI d3drm1_LoadTextureFromResource(IDirect3DRM *iface,
        HRSRC resource, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, resource %p, texture %p stub!\n", iface, resource, texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture, (IUnknown **)texture);
}

static HRESULT WINAPI d3drm1_SetSearchPath(IDirect3DRM *iface, const char *path)
{
    FIXME("iface %p, path %s stub!\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_AddSearchPath(IDirect3DRM *iface, const char *path)
{
    FIXME("iface %p, path %s stub!\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_GetSearchPath(IDirect3DRM *iface, DWORD *size, char *path)
{
    FIXME("iface %p, size %p, path %p stub!\n", iface, size, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_SetDefaultTextureColors(IDirect3DRM *iface, DWORD color_count)
{
    FIXME("iface %p, color_count %u stub!\n", iface, color_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_SetDefaultTextureShades(IDirect3DRM *iface, DWORD shade_count)
{
    FIXME("iface %p, shade_count %u stub!\n", iface, shade_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_GetDevices(IDirect3DRM *iface, IDirect3DRMDeviceArray **array)
{
    FIXME("iface %p, array %p stub!\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_GetNamedObject(IDirect3DRM *iface,
        const char *name, IDirect3DRMObject **object)
{
    FIXME("iface %p, name %s, object %p stub!\n", iface, debugstr_a(name), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_EnumerateObjects(IDirect3DRM *iface, D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_Load(IDirect3DRM *iface, void *source, void *object_id, IID **iids,
        DWORD iid_count, D3DRMLOADOPTIONS flags, D3DRMLOADCALLBACK load_cb, void *load_ctx,
        D3DRMLOADTEXTURECALLBACK load_tex_cb, void *load_tex_ctx, IDirect3DRMFrame *parent_frame)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);
    IDirect3DRMFrame3 *parent_frame3 = NULL;
    HRESULT hr = D3DRM_OK;

    TRACE("iface %p, source %p, object_id %p, iids %p, iid_count %u, flags %#x, "
            "load_cb %p, load_ctx %p, load_tex_cb %p, load_tex_ctx %p, parent_frame %p.\n",
            iface, source, object_id, iids, iid_count, flags,
            load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame);

    if (parent_frame)
        hr = IDirect3DRMFrame_QueryInterface(parent_frame, &IID_IDirect3DRMFrame3, (void **)&parent_frame3);
    if (SUCCEEDED(hr))
        hr = IDirect3DRM3_Load(&d3drm->IDirect3DRM3_iface, source, object_id, iids, iid_count,
                flags, load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame3);
    if (parent_frame3)
        IDirect3DRMFrame3_Release(parent_frame3);

    return hr;
}

static HRESULT WINAPI d3drm1_Tick(IDirect3DRM *iface, D3DVALUE tick)
{
    FIXME("iface %p, tick %.8e stub!\n", iface, tick);

    return E_NOTIMPL;
}

static const struct IDirect3DRMVtbl d3drm1_vtbl =
{
    d3drm1_QueryInterface,
    d3drm1_AddRef,
    d3drm1_Release,
    d3drm1_CreateObject,
    d3drm1_CreateFrame,
    d3drm1_CreateMesh,
    d3drm1_CreateMeshBuilder,
    d3drm1_CreateFace,
    d3drm1_CreateAnimation,
    d3drm1_CreateAnimationSet,
    d3drm1_CreateTexture,
    d3drm1_CreateLight,
    d3drm1_CreateLightRGB,
    d3drm1_CreateMaterial,
    d3drm1_CreateDevice,
    d3drm1_CreateDeviceFromSurface,
    d3drm1_CreateDeviceFromD3D,
    d3drm1_CreateDeviceFromClipper,
    d3drm1_CreateTextureFromSurface,
    d3drm1_CreateShadow,
    d3drm1_CreateViewport,
    d3drm1_CreateWrap,
    d3drm1_CreateUserVisual,
    d3drm1_LoadTexture,
    d3drm1_LoadTextureFromResource,
    d3drm1_SetSearchPath,
    d3drm1_AddSearchPath,
    d3drm1_GetSearchPath,
    d3drm1_SetDefaultTextureColors,
    d3drm1_SetDefaultTextureShades,
    d3drm1_GetDevices,
    d3drm1_GetNamedObject,
    d3drm1_EnumerateObjects,
    d3drm1_Load,
    d3drm1_Tick,
};

static HRESULT WINAPI d3drm2_QueryInterface(IDirect3DRM2 *iface, REFIID riid, void **out)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    return d3drm1_QueryInterface(&d3drm->IDirect3DRM_iface, riid, out);
}

static ULONG WINAPI d3drm2_AddRef(IDirect3DRM2 *iface)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    ULONG refcount = InterlockedIncrement(&d3drm->ref2);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
        InterlockedIncrement(&d3drm->iface_count);

    return refcount;
}

static ULONG WINAPI d3drm2_Release(IDirect3DRM2 *iface)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    ULONG refcount = InterlockedDecrement(&d3drm->ref2);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount && !InterlockedDecrement(&d3drm->iface_count))
        d3drm_destroy(d3drm);

    return refcount;
}

static HRESULT WINAPI d3drm2_CreateObject(IDirect3DRM2 *iface,
        REFCLSID clsid, IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, clsid %s, outer %p, iid %s, out %p stub!\n",
            iface, debugstr_guid(clsid), outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_CreateFrame(IDirect3DRM2 *iface,
        IDirect3DRMFrame *parent_frame, IDirect3DRMFrame2 **frame)
{
    TRACE("iface %p, parent_frame %p, frame %p.\n", iface, parent_frame, frame);

    return Direct3DRMFrame_create(&IID_IDirect3DRMFrame2, (IUnknown*)parent_frame, (IUnknown**)frame);
}

static HRESULT WINAPI d3drm2_CreateMesh(IDirect3DRM2 *iface, IDirect3DRMMesh **mesh)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return IDirect3DRM3_CreateMesh(&d3drm->IDirect3DRM3_iface, mesh);
}

static HRESULT WINAPI d3drm2_CreateMeshBuilder(IDirect3DRM2 *iface, IDirect3DRMMeshBuilder2 **mesh_builder)
{
    TRACE("iface %p, mesh_builder %p.\n", iface, mesh_builder);

    return Direct3DRMMeshBuilder_create(&IID_IDirect3DRMMeshBuilder2, (IUnknown **)mesh_builder);
}

static HRESULT WINAPI d3drm2_CreateFace(IDirect3DRM2 *iface, IDirect3DRMFace **face)
{
    TRACE("iface %p, face %p.\n", iface, face);

    return Direct3DRMFace_create(&IID_IDirect3DRMFace, (IUnknown **)face);
}

static HRESULT WINAPI d3drm2_CreateAnimation(IDirect3DRM2 *iface, IDirect3DRMAnimation **animation)
{
    FIXME("iface %p, animation %p stub!\n", iface, animation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_CreateAnimationSet(IDirect3DRM2 *iface, IDirect3DRMAnimationSet **set)
{
    FIXME("iface %p, set %p stub!\n", iface, set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_CreateTexture(IDirect3DRM2 *iface,
        D3DRMIMAGE *image, IDirect3DRMTexture2 **texture)
{
    FIXME("iface %p, image %p, texture %p partial stub.\n", iface, image, texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture2, (IUnknown **)texture);
}

static HRESULT WINAPI d3drm2_CreateLight(IDirect3DRM2 *iface,
        D3DRMLIGHTTYPE type, D3DCOLOR color, IDirect3DRMLight **light)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, type %#x, color 0x%08x, light %p.\n", iface, type, color, light);

    return IDirect3DRM3_CreateLight(&d3drm->IDirect3DRM3_iface, type, color, light);
}

static HRESULT WINAPI d3drm2_CreateLightRGB(IDirect3DRM2 *iface, D3DRMLIGHTTYPE type,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue, IDirect3DRMLight **light)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, type %#x, red %.8e, green %.8e, blue %.8e, light %p.\n",
            iface, type, red, green, blue, light);

    return IDirect3DRM3_CreateLightRGB(&d3drm->IDirect3DRM3_iface, type, red, green, blue, light);
}

static HRESULT WINAPI d3drm2_CreateMaterial(IDirect3DRM2 *iface,
        D3DVALUE power, IDirect3DRMMaterial **material)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, power %.8e, material %p.\n", iface, power, material);

    return IDirect3DRM3_CreateMaterial(&d3drm->IDirect3DRM3_iface, power, (IDirect3DRMMaterial2 **)material);
}

static HRESULT WINAPI d3drm2_CreateDevice(IDirect3DRM2 *iface,
        DWORD width, DWORD height, IDirect3DRMDevice2 **device)
{
    FIXME("iface %p, width %u, height %u, device %p.\n", iface, width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice2, (IUnknown **)device);
}

static HRESULT WINAPI d3drm2_CreateDeviceFromSurface(IDirect3DRM2 *iface, GUID *guid,
        IDirectDraw *ddraw, IDirectDrawSurface *backbuffer, IDirect3DRMDevice2 **device)
{
    FIXME("iface %p, guid %s, ddraw %p, backbuffer %p, device %p partial stub.\n",
            iface, debugstr_guid(guid), ddraw, backbuffer, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice2, (IUnknown **)device);
}

static HRESULT WINAPI d3drm2_CreateDeviceFromD3D(IDirect3DRM2 *iface,
        IDirect3D2 *d3d, IDirect3DDevice2 *d3d_device, IDirect3DRMDevice2 **device)
{
    FIXME("iface %p, d3d %p, d3d_device %p, device %p partial stub.\n",
            iface, d3d, d3d_device, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice2, (IUnknown **)device);
}

static HRESULT WINAPI d3drm2_CreateDeviceFromClipper(IDirect3DRM2 *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height,
        IDirect3DRMDevice2 **device)
{
    FIXME("iface %p, clipper %p, guid %s, width %d, height %d, device %p partial stub.\n",
            iface, clipper, debugstr_guid(guid), width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice2, (IUnknown **)device);
}

static HRESULT WINAPI d3drm2_CreateTextureFromSurface(IDirect3DRM2 *iface,
        IDirectDrawSurface *surface, IDirect3DRMTexture2 **texture)
{
    FIXME("iface %p, surface %p, texture %p stub!\n", iface, surface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_CreateShadow(IDirect3DRM2 *iface, IDirect3DRMVisual *visual,
        IDirect3DRMLight *light, D3DVALUE px, D3DVALUE py, D3DVALUE pz, D3DVALUE nx, D3DVALUE ny, D3DVALUE nz,
        IDirect3DRMVisual **shadow)
{
    FIXME("iface %p, visual %p, light %p, px %.8e, py %.8e, pz %.8e, nx %.8e, ny %.8e, nz %.8e, shadow %p stub!\n",
            iface, visual, light, px, py, pz, nx, ny, nz, shadow);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_CreateViewport(IDirect3DRM2 *iface, IDirect3DRMDevice *device,
        IDirect3DRMFrame *camera, DWORD x, DWORD y, DWORD width, DWORD height, IDirect3DRMViewport **viewport)
{
    FIXME("iface %p, device %p, camera %p, x %u, y %u, width %u, height %u, viewport %p partial stub!\n",
            iface, device, camera, x, y, width, height, viewport);

    return Direct3DRMViewport_create(&IID_IDirect3DRMViewport, (IUnknown **)viewport);
}

static HRESULT WINAPI d3drm2_CreateWrap(IDirect3DRM2 *iface, D3DRMWRAPTYPE type, IDirect3DRMFrame *frame,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv,
        IDirect3DRMWrap **wrap)
{
    FIXME("iface %p, type %#x, frame %p, ox %.8e, oy %.8e, oz %.8e, dx %.8e, dy %.8e, dz %.8e, "
            "ux %.8e, uy %.8e, uz %.8e, ou %.8e, ov %.8e, su %.8e, sv %.8e, wrap %p stub!\n",
            iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, wrap);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_CreateUserVisual(IDirect3DRM2 *iface,
        D3DRMUSERVISUALCALLBACK cb, void *ctx, IDirect3DRMUserVisual **visual)
{
    FIXME("iface %p, cb %p, ctx %p, visual %p stub!\n", iface, cb, ctx, visual);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_LoadTexture(IDirect3DRM2 *iface,
        const char *filename, IDirect3DRMTexture2 **texture)
{
    FIXME("iface %p, filename %s, texture %p stub!\n", iface, debugstr_a(filename), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture2, (IUnknown **)texture);
}

static HRESULT WINAPI d3drm2_LoadTextureFromResource(IDirect3DRM2 *iface, HMODULE module,
        const char *resource_name, const char *resource_type, IDirect3DRMTexture2 **texture)
{
    FIXME("iface %p, resource_name %s, resource_type %s, texture %p stub!\n",
            iface, debugstr_a(resource_name), debugstr_a(resource_type), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture2, (IUnknown **)texture);
}

static HRESULT WINAPI d3drm2_SetSearchPath(IDirect3DRM2 *iface, const char *path)
{
    FIXME("iface %p, path %s stub!\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_AddSearchPath(IDirect3DRM2 *iface, const char *path)
{
    FIXME("iface %p, path %s stub!\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_GetSearchPath(IDirect3DRM2 *iface, DWORD *size, char *path)
{
    FIXME("iface %p, size %p, path %p stub!\n", iface, size, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_SetDefaultTextureColors(IDirect3DRM2 *iface, DWORD color_count)
{
    FIXME("iface %p, color_count %u stub!\n", iface, color_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_SetDefaultTextureShades(IDirect3DRM2 *iface, DWORD shade_count)
{
    FIXME("iface %p, shade_count %u stub!\n", iface, shade_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_GetDevices(IDirect3DRM2 *iface, IDirect3DRMDeviceArray **array)
{
    FIXME("iface %p, array %p stub!\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_GetNamedObject(IDirect3DRM2 *iface,
        const char *name, IDirect3DRMObject **object)
{
    FIXME("iface %p, name %s, object %p stub!\n", iface, debugstr_a(name), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_EnumerateObjects(IDirect3DRM2 *iface, D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_Load(IDirect3DRM2 *iface, void *source, void *object_id, IID **iids,
        DWORD iid_count, D3DRMLOADOPTIONS flags, D3DRMLOADCALLBACK load_cb, void *load_ctx,
        D3DRMLOADTEXTURECALLBACK load_tex_cb, void *load_tex_ctx, IDirect3DRMFrame *parent_frame)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    IDirect3DRMFrame3 *parent_frame3 = NULL;
    HRESULT hr = D3DRM_OK;

    TRACE("iface %p, source %p, object_id %p, iids %p, iid_count %u, flags %#x, "
            "load_cb %p, load_ctx %p, load_tex_cb %p, load_tex_ctx %p, parent_frame %p.\n",
            iface, source, object_id, iids, iid_count, flags,
            load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame);

    if (parent_frame)
        hr = IDirect3DRMFrame_QueryInterface(parent_frame, &IID_IDirect3DRMFrame3, (void **)&parent_frame3);
    if (SUCCEEDED(hr))
        hr = IDirect3DRM3_Load(&d3drm->IDirect3DRM3_iface, source, object_id, iids, iid_count,
                flags, load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame3);
    if (parent_frame3)
        IDirect3DRMFrame3_Release(parent_frame3);

    return hr;
}

static HRESULT WINAPI d3drm2_Tick(IDirect3DRM2 *iface, D3DVALUE tick)
{
    FIXME("iface %p, tick %.8e stub!\n", iface, tick);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_CreateProgressiveMesh(IDirect3DRM2 *iface, IDirect3DRMProgressiveMesh **mesh)
{
    FIXME("iface %p, mesh %p stub!\n", iface, mesh);

    return E_NOTIMPL;
}

static const struct IDirect3DRM2Vtbl d3drm2_vtbl =
{
    d3drm2_QueryInterface,
    d3drm2_AddRef,
    d3drm2_Release,
    d3drm2_CreateObject,
    d3drm2_CreateFrame,
    d3drm2_CreateMesh,
    d3drm2_CreateMeshBuilder,
    d3drm2_CreateFace,
    d3drm2_CreateAnimation,
    d3drm2_CreateAnimationSet,
    d3drm2_CreateTexture,
    d3drm2_CreateLight,
    d3drm2_CreateLightRGB,
    d3drm2_CreateMaterial,
    d3drm2_CreateDevice,
    d3drm2_CreateDeviceFromSurface,
    d3drm2_CreateDeviceFromD3D,
    d3drm2_CreateDeviceFromClipper,
    d3drm2_CreateTextureFromSurface,
    d3drm2_CreateShadow,
    d3drm2_CreateViewport,
    d3drm2_CreateWrap,
    d3drm2_CreateUserVisual,
    d3drm2_LoadTexture,
    d3drm2_LoadTextureFromResource,
    d3drm2_SetSearchPath,
    d3drm2_AddSearchPath,
    d3drm2_GetSearchPath,
    d3drm2_SetDefaultTextureColors,
    d3drm2_SetDefaultTextureShades,
    d3drm2_GetDevices,
    d3drm2_GetNamedObject,
    d3drm2_EnumerateObjects,
    d3drm2_Load,
    d3drm2_Tick,
    d3drm2_CreateProgressiveMesh,
};

static HRESULT WINAPI d3drm3_QueryInterface(IDirect3DRM3 *iface, REFIID riid, void **out)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);

    return d3drm1_QueryInterface(&d3drm->IDirect3DRM_iface, riid, out);
}

static ULONG WINAPI d3drm3_AddRef(IDirect3DRM3 *iface)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    ULONG refcount = InterlockedIncrement(&d3drm->ref3);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
        InterlockedIncrement(&d3drm->iface_count);

    return refcount;
}

static ULONG WINAPI d3drm3_Release(IDirect3DRM3 *iface)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    ULONG refcount = InterlockedDecrement(&d3drm->ref3);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount && !InterlockedDecrement(&d3drm->iface_count))
        d3drm_destroy(d3drm);

    return refcount;
}

static HRESULT WINAPI d3drm3_CreateObject(IDirect3DRM3 *iface,
        REFCLSID clsid, IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, clsid %s, outer %p, iid %s, out %p stub!\n",
            iface, debugstr_guid(clsid), outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_CreateFrame(IDirect3DRM3 *iface,
        IDirect3DRMFrame3 *parent, IDirect3DRMFrame3 **frame)
{
    TRACE("iface %p, parent %p, frame %p.\n", iface, parent, frame);

    return Direct3DRMFrame_create(&IID_IDirect3DRMFrame3, (IUnknown *)parent, (IUnknown **)frame);
}

static HRESULT WINAPI d3drm3_CreateMesh(IDirect3DRM3 *iface, IDirect3DRMMesh **mesh)
{
    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return Direct3DRMMesh_create(mesh);
}

static HRESULT WINAPI d3drm3_CreateMeshBuilder(IDirect3DRM3 *iface, IDirect3DRMMeshBuilder3 **mesh_builder)
{
    TRACE("iface %p, mesh_builder %p.\n", iface, mesh_builder);

    return Direct3DRMMeshBuilder_create(&IID_IDirect3DRMMeshBuilder3, (IUnknown **)mesh_builder);
}

static HRESULT WINAPI d3drm3_CreateFace(IDirect3DRM3 *iface, IDirect3DRMFace2 **face)
{
    TRACE("iface %p, face %p.\n", iface, face);

    return Direct3DRMFace_create(&IID_IDirect3DRMFace2, (IUnknown **)face);
}

static HRESULT WINAPI d3drm3_CreateAnimation(IDirect3DRM3 *iface, IDirect3DRMAnimation2 **animation)
{
    FIXME("iface %p, animation %p stub!\n", iface, animation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_CreateAnimationSet(IDirect3DRM3 *iface, IDirect3DRMAnimationSet2 **set)
{
    FIXME("iface %p, set %p stub!\n", iface, set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_CreateTexture(IDirect3DRM3 *iface,
        D3DRMIMAGE *image, IDirect3DRMTexture3 **texture)
{
    FIXME("iface %p, image %p, texture %p partial stub.\n", iface, image, texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture3, (IUnknown **)texture);
}

static HRESULT WINAPI d3drm3_CreateLight(IDirect3DRM3 *iface,
        D3DRMLIGHTTYPE type, D3DCOLOR color, IDirect3DRMLight **light)
{
    HRESULT hr;

    FIXME("iface %p, type %#x, color 0x%08x, light %p partial stub!\n", iface, type, color, light);

    if (SUCCEEDED(hr = Direct3DRMLight_create((IUnknown **)light)))
    {
        IDirect3DRMLight_SetType(*light, type);
        IDirect3DRMLight_SetColor(*light, color);
    }

    return hr;
}

static HRESULT WINAPI d3drm3_CreateLightRGB(IDirect3DRM3 *iface, D3DRMLIGHTTYPE type,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue, IDirect3DRMLight **light)
{
    HRESULT hr;

    FIXME("iface %p, type %#x, red %.8e, green %.8e, blue %.8e, light %p partial stub!\n",
            iface, type, red, green, blue, light);

    if (SUCCEEDED(hr = Direct3DRMLight_create((IUnknown **)light)))
    {
        IDirect3DRMLight_SetType(*light, type);
        IDirect3DRMLight_SetColorRGB(*light, red, green, blue);
    }

    return hr;
}

static HRESULT WINAPI d3drm3_CreateMaterial(IDirect3DRM3 *iface,
        D3DVALUE power, IDirect3DRMMaterial2 **material)
{
    HRESULT hr;

    TRACE("iface %p, power %.8e, material %p.\n", iface, power, material);

    if (SUCCEEDED(hr = Direct3DRMMaterial_create(material)))
        IDirect3DRMMaterial2_SetPower(*material, power);

    return hr;
}

static HRESULT WINAPI d3drm3_CreateDevice(IDirect3DRM3 *iface,
        DWORD width, DWORD height, IDirect3DRMDevice3 **device)
{
    FIXME("iface %p, width %u, height %u, device %p partial stub!\n", iface, width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice3, (IUnknown **)device);
}

static HRESULT WINAPI d3drm3_CreateDeviceFromSurface(IDirect3DRM3 *iface, GUID *guid,
        IDirectDraw *ddraw, IDirectDrawSurface *backbuffer, DWORD flags, IDirect3DRMDevice3 **device)
{
    FIXME("iface %p, guid %s, ddraw %p, backbuffer %p, flags %#x, device %p partial stub.\n",
            iface, debugstr_guid(guid), ddraw, backbuffer, flags, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice3, (IUnknown **)device);
}

static HRESULT WINAPI d3drm3_CreateDeviceFromD3D(IDirect3DRM3 *iface,
        IDirect3D2 *d3d, IDirect3DDevice2 *d3d_device, IDirect3DRMDevice3 **device)
{
    FIXME("iface %p, d3d %p, d3d_device %p, device %p partial stub.\n",
            iface, d3d, d3d_device, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice3, (IUnknown **)device);
}

static HRESULT WINAPI d3drm3_CreateDeviceFromClipper(IDirect3DRM3 *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height,
        IDirect3DRMDevice3 **device)
{
    FIXME("iface %p, clipper %p, guid %s, width %d, height %d, device %p partial stub.\n",
            iface, clipper, debugstr_guid(guid), width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice3, (IUnknown **)device);
}

static HRESULT WINAPI d3drm3_CreateShadow(IDirect3DRM3 *iface, IUnknown *object, IDirect3DRMLight *light,
        D3DVALUE px, D3DVALUE py, D3DVALUE pz, D3DVALUE nx, D3DVALUE ny, D3DVALUE nz, IDirect3DRMShadow2 **shadow)
{
    FIXME("iface %p, object %p, light %p, px %.8e, py %.8e, pz %.8e, nx %.8e, ny %.8e, nz %.8e, shadow %p stub!\n",
            iface, object, light, px, py, pz, nx, ny, nz, shadow);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_CreateTextureFromSurface(IDirect3DRM3 *iface,
        IDirectDrawSurface *surface, IDirect3DRMTexture3 **texture)
{
    FIXME("iface %p, surface %p, texture %p stub!\n", iface, surface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_CreateViewport(IDirect3DRM3 *iface, IDirect3DRMDevice3 *device,
        IDirect3DRMFrame3 *camera, DWORD x, DWORD y, DWORD width, DWORD height, IDirect3DRMViewport2 **viewport)
{
    FIXME("iface %p, device %p, camera %p, x %u, y %u, width %u, height %u, viewport %p partial stub!\n",
            iface, device, camera, x, y, width, height, viewport);

    return Direct3DRMViewport_create(&IID_IDirect3DRMViewport2, (IUnknown **)viewport);
}

static HRESULT WINAPI d3drm3_CreateWrap(IDirect3DRM3 *iface, D3DRMWRAPTYPE type, IDirect3DRMFrame3 *frame,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv,
        IDirect3DRMWrap **wrap)
{
    FIXME("iface %p, type %#x, frame %p, ox %.8e, oy %.8e, oz %.8e, dx %.8e, dy %.8e, dz %.8e, "
            "ux %.8e, uy %.8e, uz %.8e, ou %.8e, ov %.8e, su %.8e, sv %.8e, wrap %p stub!\n",
            iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, wrap);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_CreateUserVisual(IDirect3DRM3 *iface,
        D3DRMUSERVISUALCALLBACK cb, void *ctx, IDirect3DRMUserVisual **visual)
{
    FIXME("iface %p, cb %p, ctx %p, visual %p stub!\n", iface, cb, ctx, visual);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_LoadTexture(IDirect3DRM3 *iface,
        const char *filename, IDirect3DRMTexture3 **texture)
{
    FIXME("iface %p, filename %s, texture %p stub!\n", iface, debugstr_a(filename), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture3, (IUnknown **)texture);
}

static HRESULT WINAPI d3drm3_LoadTextureFromResource(IDirect3DRM3 *iface, HMODULE module,
        const char *resource_name, const char *resource_type, IDirect3DRMTexture3 **texture)
{
    FIXME("iface %p, module %p, resource_name %s, resource_type %s, texture %p stub!\n",
            iface, module, debugstr_a(resource_name), debugstr_a(resource_type), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture3, (IUnknown **)texture);
}

static HRESULT WINAPI d3drm3_SetSearchPath(IDirect3DRM3 *iface, const char *path)
{
    FIXME("iface %p, path %s stub!\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_AddSearchPath(IDirect3DRM3 *iface, const char *path)
{
    FIXME("iface %p, path %s stub!\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_GetSearchPath(IDirect3DRM3 *iface, DWORD *size, char *path)
{
    FIXME("iface %p, size %p, path %p stub!\n", iface, size, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_SetDefaultTextureColors(IDirect3DRM3 *iface, DWORD color_count)
{
    FIXME("iface %p, color_count %u stub!\n", iface, color_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_SetDefaultTextureShades(IDirect3DRM3 *iface, DWORD shade_count)
{
    FIXME("iface %p, shade_count %u stub!\n", iface, shade_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_GetDevices(IDirect3DRM3 *iface, IDirect3DRMDeviceArray **array)
{
    FIXME("iface %p, array %p stub!\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_GetNamedObject(IDirect3DRM3 *iface,
        const char *name, IDirect3DRMObject **object)
{
    FIXME("iface %p, name %s, object %p stub!\n", iface, debugstr_a(name), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_EnumerateObjects(IDirect3DRM3 *iface, D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT load_data(IDirect3DRM3 *iface, IDirectXFileData *data_object, IID **GUIDs, DWORD nb_GUIDs, D3DRMLOADCALLBACK LoadProc,
                         void *ArgLP, D3DRMLOADTEXTURECALLBACK LoadTextureProc, void *ArgLTP, IDirect3DRMFrame3 *parent_frame)
{
    HRESULT ret = D3DRMERR_BADOBJECT;
    HRESULT hr;
    const GUID* guid;
    DWORD i;
    BOOL requested = FALSE;

    hr = IDirectXFileData_GetType(data_object, &guid);
    if (hr != DXFILE_OK)
        goto end;

    TRACE("Found object type whose GUID = %s\n", debugstr_guid(guid));

    /* Load object only if it is top level and requested or if it is part of another object */

    if (IsEqualGUID(guid, &TID_D3DRMMesh))
    {
        TRACE("Found TID_D3DRMMesh\n");

        for (i = 0; i < nb_GUIDs; i++)
            if (IsEqualGUID(GUIDs[i], &IID_IDirect3DRMMeshBuilder) ||
                IsEqualGUID(GUIDs[i], &IID_IDirect3DRMMeshBuilder2) ||
                IsEqualGUID(GUIDs[i], &IID_IDirect3DRMMeshBuilder3))
            {
                requested = TRUE;
                break;
            }

        if (requested || parent_frame)
        {
            IDirect3DRMMeshBuilder3 *meshbuilder;

            TRACE("Load mesh data\n");

            hr = IDirect3DRM3_CreateMeshBuilder(iface, &meshbuilder);
            if (SUCCEEDED(hr))
            {
                hr = load_mesh_data(meshbuilder, data_object, LoadTextureProc, ArgLTP);
                if (SUCCEEDED(hr))
                {
                    /* Only top level objects are notified */
                    if (!parent_frame)
                    {
                        IDirect3DRMObject *object;

                        hr = IDirect3DRMMeshBuilder3_QueryInterface(meshbuilder, GUIDs[i], (void**)&object);
                        if (SUCCEEDED(hr))
                        {
                            LoadProc(object, GUIDs[i], ArgLP);
                            IDirect3DRMObject_Release(object);
                        }
                    }
                    else
                    {
                        IDirect3DRMFrame3_AddVisual(parent_frame, (IUnknown*)meshbuilder);
                    }
                }
                IDirect3DRMMeshBuilder3_Release(meshbuilder);
            }

            if (FAILED(hr))
                ERR("Cannot process mesh\n");
        }
    }
    else if (IsEqualGUID(guid, &TID_D3DRMFrame))
    {
        TRACE("Found TID_D3DRMFrame\n");

        for (i = 0; i < nb_GUIDs; i++)
            if (IsEqualGUID(GUIDs[i], &IID_IDirect3DRMFrame) ||
                IsEqualGUID(GUIDs[i], &IID_IDirect3DRMFrame2) ||
                IsEqualGUID(GUIDs[i], &IID_IDirect3DRMFrame3))
            {
                requested = TRUE;
                break;
            }

        if (requested || parent_frame)
        {
            IDirect3DRMFrame3 *frame;

            TRACE("Load frame data\n");

            hr = IDirect3DRM3_CreateFrame(iface, parent_frame, &frame);
            if (SUCCEEDED(hr))
            {
                IDirectXFileObject *child;

                while (SUCCEEDED(hr = IDirectXFileData_GetNextObject(data_object, &child)))
                {
                    IDirectXFileData *data;
                    IDirectXFileDataReference *reference;
                    IDirectXFileBinary *binary;

                    if (SUCCEEDED(IDirectXFileObject_QueryInterface(child,
                            &IID_IDirectXFileBinary, (void **)&binary)))
                    {
                        FIXME("Binary Object not supported yet\n");
                        IDirectXFileBinary_Release(binary);
                    }
                    else if (SUCCEEDED(IDirectXFileObject_QueryInterface(child,
                            &IID_IDirectXFileData, (void **)&data)))
                    {
                        TRACE("Found Data Object\n");
                        hr = load_data(iface, data, GUIDs, nb_GUIDs, LoadProc, ArgLP, LoadTextureProc, ArgLTP, frame);
                        IDirectXFileData_Release(data);
                    }
                    else if (SUCCEEDED(IDirectXFileObject_QueryInterface(child,
                            &IID_IDirectXFileDataReference, (void **)&reference)))
                    {
                        TRACE("Found Data Object Reference\n");
                        IDirectXFileDataReference_Resolve(reference, &data);
                        hr = load_data(iface, data, GUIDs, nb_GUIDs, LoadProc, ArgLP, LoadTextureProc, ArgLTP, frame);
                        IDirectXFileData_Release(data);
                        IDirectXFileDataReference_Release(reference);
                    }
                    IDirectXFileObject_Release(child);
                }

                if (hr != DXFILEERR_NOMOREOBJECTS)
                {
                    IDirect3DRMFrame3_Release(frame);
                    goto end;
                }
                hr = S_OK;

                /* Only top level objects are notified */
                if (!parent_frame)
                {
                    IDirect3DRMObject *object;

                    hr = IDirect3DRMFrame3_QueryInterface(frame, GUIDs[i], (void**)&object);
                    if (SUCCEEDED(hr))
                    {
                        LoadProc(object, GUIDs[i], ArgLP);
                        IDirect3DRMObject_Release(object);
                    }
                }
                IDirect3DRMFrame3_Release(frame);
            }

            if (FAILED(hr))
                ERR("Cannot process frame\n");
        }
    }
    else if (IsEqualGUID(guid, &TID_D3DRMMaterial))
    {
        TRACE("Found TID_D3DRMMaterial\n");

        /* Cannot be requested so nothing to do */
    }
    else if (IsEqualGUID(guid, &TID_D3DRMFrameTransformMatrix))
    {
        TRACE("Found TID_D3DRMFrameTransformMatrix\n");

        /* Cannot be requested */
        if (parent_frame)
        {
            D3DRMMATRIX4D *matrix;
            DWORD size;

            TRACE("Load Frame Transform Matrix data\n");

            hr = IDirectXFileData_GetData(data_object, NULL, &size, (void**)&matrix);
            if ((hr != DXFILE_OK) || (size != sizeof(matrix)))
                goto end;

            hr = IDirect3DRMFrame3_AddTransform(parent_frame, D3DRMCOMBINE_REPLACE, *matrix);
            if (FAILED(hr))
                goto end;
        }
    }
    else
    {
        FIXME("Found unknown TID %s\n", debugstr_guid(guid));
    }

    ret = D3DRM_OK;

end:

    return ret;
}

static HRESULT WINAPI d3drm3_Load(IDirect3DRM3 *iface, void *source, void *object_id, IID **iids,
        DWORD iid_count, D3DRMLOADOPTIONS flags, D3DRMLOADCALLBACK load_cb, void *load_ctx,
        D3DRMLOADTEXTURECALLBACK load_tex_cb, void *load_tex_ctx, IDirect3DRMFrame3 *parent_frame)
{
    DXFILELOADOPTIONS load_options;
    IDirectXFile *file = NULL;
    IDirectXFileEnumObject *enum_object = NULL;
    IDirectXFileData *data = NULL;
    HRESULT hr;
    const GUID* pGuid;
    DWORD size;
    struct d3drm_file_header *header;
    HRESULT ret = D3DRMERR_BADOBJECT;
    DWORD i;

    TRACE("iface %p, source %p, object_id %p, iids %p, iid_count %u, flags %#x, "
            "load_cb %p, load_ctx %p, load_tex_cb %p, load_tex_ctx %p, parent_frame %p.\n",
            iface, source, object_id, iids, iid_count, flags,
            load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame);

    TRACE("Looking for GUIDs:\n");
    for (i = 0; i < iid_count; ++i)
        TRACE("- %s (%s)\n", debugstr_guid(iids[i]), get_IID_string(iids[i]));

    if (flags == D3DRMLOAD_FROMMEMORY)
    {
        load_options = DXFILELOAD_FROMMEMORY;
    }
    else if (flags == D3DRMLOAD_FROMFILE)
    {
        load_options = DXFILELOAD_FROMFILE;
        TRACE("Loading from file %s\n", debugstr_a(source));
    }
    else
    {
        FIXME("Load options %#x not supported yet.\n", flags);
        return E_NOTIMPL;
    }

    hr = DirectXFileCreate(&file);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFile_RegisterTemplates(file, templates, strlen(templates));
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFile_CreateEnumObject(file, source, load_options, &enum_object);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFileEnumObject_GetNextDataObject(enum_object, &data);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFileData_GetType(data, &pGuid);
    if (hr != DXFILE_OK)
        goto end;

    TRACE("Found object type whose GUID = %s\n", debugstr_guid(pGuid));

    if (!IsEqualGUID(pGuid, &TID_DXFILEHeader))
    {
        ret = D3DRMERR_BADFILE;
        goto end;
    }

    hr = IDirectXFileData_GetData(data, NULL, &size, (void **)&header);
    if ((hr != DXFILE_OK) || (size != sizeof(*header)))
        goto end;

    TRACE("Version is %u.%u, flags %#x.\n", header->major, header->minor, header->flags);

    /* Version must be 1.0.x */
    if ((header->major != 1) || (header->minor != 0))
    {
        ret = D3DRMERR_BADFILE;
        goto end;
    }

    IDirectXFileData_Release(data);
    data = NULL;

    while (1)
    {
        hr = IDirectXFileEnumObject_GetNextDataObject(enum_object, &data);
        if (hr == DXFILEERR_NOMOREOBJECTS)
        {
            TRACE("No more object\n");
            break;
        }
        else if (hr != DXFILE_OK)
        {
            ret = D3DRMERR_BADFILE;
            goto end;
        }

        ret = load_data(iface, data, iids, iid_count, load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame);
        if (ret != D3DRM_OK)
            goto end;

        IDirectXFileData_Release(data);
        data = NULL;
    }

    ret = D3DRM_OK;

end:
    if (data)
        IDirectXFileData_Release(data);
    if (enum_object)
        IDirectXFileEnumObject_Release(enum_object);
    if (file)
        IDirectXFile_Release(file);

    return ret;
}

static HRESULT WINAPI d3drm3_Tick(IDirect3DRM3 *iface, D3DVALUE tick)
{
    FIXME("iface %p, tick %.8e stub!\n", iface, tick);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_CreateProgressiveMesh(IDirect3DRM3 *iface, IDirect3DRMProgressiveMesh **mesh)
{
    FIXME("iface %p, mesh %p stub!\n", iface, mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_RegisterClient(IDirect3DRM3 *iface, REFGUID guid, DWORD *id)
{
    FIXME("iface %p, guid %s, id %p stub!\n", iface, debugstr_guid(guid), id);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_UnregisterClient(IDirect3DRM3 *iface, REFGUID guid)
{
    FIXME("iface %p, guid %s stub!\n", iface, debugstr_guid(guid));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_CreateClippedVisual(IDirect3DRM3 *iface,
        IDirect3DRMVisual *visual, IDirect3DRMClippedVisual **clipped_visual)
{
    FIXME("iface %p, visual %p, clipped_visual %p stub!\n", iface, visual, clipped_visual);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_SetOptions(IDirect3DRM3 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_GetOptions(IDirect3DRM3 *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);

    return E_NOTIMPL;
}

static const struct IDirect3DRM3Vtbl d3drm3_vtbl =
{
    d3drm3_QueryInterface,
    d3drm3_AddRef,
    d3drm3_Release,
    d3drm3_CreateObject,
    d3drm3_CreateFrame,
    d3drm3_CreateMesh,
    d3drm3_CreateMeshBuilder,
    d3drm3_CreateFace,
    d3drm3_CreateAnimation,
    d3drm3_CreateAnimationSet,
    d3drm3_CreateTexture,
    d3drm3_CreateLight,
    d3drm3_CreateLightRGB,
    d3drm3_CreateMaterial,
    d3drm3_CreateDevice,
    d3drm3_CreateDeviceFromSurface,
    d3drm3_CreateDeviceFromD3D,
    d3drm3_CreateDeviceFromClipper,
    d3drm3_CreateTextureFromSurface,
    d3drm3_CreateShadow,
    d3drm3_CreateViewport,
    d3drm3_CreateWrap,
    d3drm3_CreateUserVisual,
    d3drm3_LoadTexture,
    d3drm3_LoadTextureFromResource,
    d3drm3_SetSearchPath,
    d3drm3_AddSearchPath,
    d3drm3_GetSearchPath,
    d3drm3_SetDefaultTextureColors,
    d3drm3_SetDefaultTextureShades,
    d3drm3_GetDevices,
    d3drm3_GetNamedObject,
    d3drm3_EnumerateObjects,
    d3drm3_Load,
    d3drm3_Tick,
    d3drm3_CreateProgressiveMesh,
    d3drm3_RegisterClient,
    d3drm3_UnregisterClient,
    d3drm3_CreateClippedVisual,
    d3drm3_SetOptions,
    d3drm3_GetOptions,
};

HRESULT WINAPI Direct3DRMCreate(IDirect3DRM **d3drm)
{
    struct d3drm *object;

    TRACE("d3drm %p.\n", d3drm);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRM_iface.lpVtbl = &d3drm1_vtbl;
    object->IDirect3DRM2_iface.lpVtbl = &d3drm2_vtbl;
    object->IDirect3DRM3_iface.lpVtbl = &d3drm3_vtbl;
    object->ref1 = 1;
    object->iface_count = 1;

    *d3drm = &object->IDirect3DRM_iface;

    return S_OK;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    TRACE("(%s, %s, %p): stub\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(!ppv)
        return E_INVALIDARG;

    return CLASS_E_CLASSNOTAVAILABLE;
}
