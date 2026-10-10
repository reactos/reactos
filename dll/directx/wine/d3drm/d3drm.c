/*
 * Implementation of IDirect3DRM Interface
 *
 * Copyright 2010, 2012 Christian Costa
 * Copyright 2011 André Hentschel
 * Copyright 2016 Aaryaman Vasishta
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

static HRESULT d3drm_create_texture_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_texture *texture;
    HRESULT hr;

    if (FAILED(hr = d3drm_texture_create(&texture, d3drm)))
        return hr;

    *object = &texture->IDirect3DRMTexture_iface;

    return hr;
}

static HRESULT d3drm_create_device_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_device *device;
    HRESULT hr;

    if (FAILED(hr = d3drm_device_create(&device, d3drm)))
        return hr;

    *object = &device->IDirect3DRMDevice_iface;

    return hr;
}

static HRESULT d3drm_create_viewport_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_viewport *viewport;
    HRESULT hr;

    if (FAILED(hr = d3drm_viewport_create(&viewport, d3drm)))
        return hr;

    *object = &viewport->IDirect3DRMViewport_iface;

    return hr;
}

static HRESULT d3drm_create_face_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_face *face;
    HRESULT hr;

    if (FAILED(hr = d3drm_face_create(&face)))
        return hr;

    *object = &face->IDirect3DRMFace_iface;

    return hr;
}

static HRESULT d3drm_create_mesh_builder_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_mesh_builder *mesh_builder;
    HRESULT hr;

    if (FAILED(hr = d3drm_mesh_builder_create(&mesh_builder, d3drm)))
        return hr;

    *object = &mesh_builder->IDirect3DRMMeshBuilder2_iface;

    return hr;
}

static HRESULT d3drm_create_frame_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_frame *frame;
    HRESULT hr;

    if (FAILED(hr = d3drm_frame_create(&frame, NULL, d3drm)))
        return hr;

    *object = &frame->IDirect3DRMFrame_iface;

    return hr;
}

static HRESULT d3drm_create_light_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_light *light;
    HRESULT hr;

    if (FAILED(hr = d3drm_light_create(&light, d3drm)))
        return hr;

    *object = &light->IDirect3DRMLight_iface;

    return hr;
}

static HRESULT d3drm_create_material_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_material *material;
    HRESULT hr;

    if (FAILED(hr = d3drm_material_create(&material, d3drm)))
        return hr;

    *object = &material->IDirect3DRMMaterial2_iface;

    return hr;
}

static HRESULT d3drm_create_mesh_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_mesh *mesh;
    HRESULT hr;

    if (FAILED(hr = d3drm_mesh_create(&mesh, d3drm)))
        return hr;

    *object = &mesh->IDirect3DRMMesh_iface;

    return hr;
}

static HRESULT d3drm_create_animation_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_animation *animation;
    HRESULT hr;

    if (FAILED(hr = d3drm_animation_create(&animation, d3drm)))
        return hr;

    *object = &animation->IDirect3DRMAnimation_iface;

    return hr;
}

static HRESULT d3drm_create_wrap_object(void **object, IDirect3DRM *d3drm)
{
    struct d3drm_wrap *wrap;
    HRESULT hr;

    if (FAILED(hr = d3drm_wrap_create(&wrap, d3drm)))
        return hr;

    *object = &wrap->IDirect3DRMWrap_iface;

    return hr;
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
    TRACE("d3drm object %p is being destroyed.\n", d3drm);
    free(d3drm);
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

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    if (refcount == 1)
        InterlockedIncrement(&d3drm->iface_count);

    return refcount;
}

static ULONG WINAPI d3drm1_Release(IDirect3DRM *iface)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);
    ULONG refcount = InterlockedDecrement(&d3drm->ref1);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount && !InterlockedDecrement(&d3drm->iface_count))
        d3drm_destroy(d3drm);

    return refcount;
}

static HRESULT WINAPI d3drm1_CreateObject(IDirect3DRM *iface,
        REFCLSID clsid, IUnknown *outer, REFIID iid, void **out)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, clsid %s, outer %p, iid %s, out %p.\n",
            iface, debugstr_guid(clsid), outer, debugstr_guid(iid), out);

    return IDirect3DRM3_CreateObject(&d3drm->IDirect3DRM3_iface, clsid, outer, iid, out);
}

static HRESULT WINAPI d3drm1_CreateFrame(IDirect3DRM *iface,
        IDirect3DRMFrame *parent_frame, IDirect3DRMFrame **frame)
{
    struct d3drm_frame *object;
    HRESULT hr;

    TRACE("iface %p, parent_frame %p, frame %p.\n", iface, parent_frame, frame);

    if (FAILED(hr = d3drm_frame_create(&object, (IUnknown *)parent_frame, iface)))
        return hr;

    *frame = &object->IDirect3DRMFrame_iface;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm1_CreateMesh(IDirect3DRM *iface, IDirect3DRMMesh **mesh)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return IDirect3DRM3_CreateMesh(&d3drm->IDirect3DRM3_iface, mesh);
}

static HRESULT WINAPI d3drm1_CreateMeshBuilder(IDirect3DRM *iface, IDirect3DRMMeshBuilder **mesh_builder)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, mesh_builder %p.\n", iface, mesh_builder);

    return IDirect3DRM2_CreateMeshBuilder(&d3drm->IDirect3DRM2_iface, (IDirect3DRMMeshBuilder2 **)mesh_builder);
}

static HRESULT WINAPI d3drm1_CreateFace(IDirect3DRM *iface, IDirect3DRMFace **face)
{
    struct d3drm_face *object;
    HRESULT hr;

    TRACE("iface %p, face %p.\n", iface, face);

    if (FAILED(hr = d3drm_face_create(&object)))
        return hr;

    *face = &object->IDirect3DRMFace_iface;

    return S_OK;
}

static HRESULT WINAPI d3drm1_CreateAnimation(IDirect3DRM *iface, IDirect3DRMAnimation **animation)
{
    struct d3drm_animation *object;
    HRESULT hr;

    TRACE("iface %p, animation %p.\n", iface, animation);

    if (!animation)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = d3drm_animation_create(&object, iface)))
        return hr;

    *animation = &object->IDirect3DRMAnimation_iface;

    return S_OK;
}

static HRESULT WINAPI d3drm1_CreateAnimationSet(IDirect3DRM *iface, IDirect3DRMAnimationSet **set)
{
    FIXME("iface %p, set %p stub!\n", iface, set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_CreateTexture(IDirect3DRM *iface,
        D3DRMIMAGE *image, IDirect3DRMTexture **texture)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);
    IDirect3DRMTexture3 *texture3;
    HRESULT hr;

    TRACE("iface %p, image %p, texture %p.\n", iface, image, texture);

    if (!texture)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = IDirect3DRM3_CreateTexture(&d3drm->IDirect3DRM3_iface, image, &texture3)))
    {
        *texture = NULL;
        return hr;
    }

    hr = IDirect3DRMTexture3_QueryInterface(texture3, &IID_IDirect3DRMTexture, (void **)texture);
    IDirect3DRMTexture3_Release(texture3);

    return hr;
}

static HRESULT WINAPI d3drm1_CreateLight(IDirect3DRM *iface,
        D3DRMLIGHTTYPE type, D3DCOLOR color, IDirect3DRMLight **light)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, type %#x, color 0x%08lx, light %p.\n", iface, type, color, light);

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
    TRACE("iface %p, width %lu, height %lu, device %p.\n", iface, width, height, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    return D3DRMERR_BADDEVICE;
}

static HRESULT WINAPI d3drm1_CreateDeviceFromSurface(IDirect3DRM *iface, GUID *guid,
        IDirectDraw *ddraw, IDirectDrawSurface *backbuffer, IDirect3DRMDevice **device)
{
    struct d3drm_device *object;
    HRESULT hr;

    TRACE("iface %p, guid %s, ddraw %p, backbuffer %p, device %p.\n",
            iface, debugstr_guid(guid), ddraw, backbuffer, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    if (!backbuffer || !ddraw)
        return D3DRMERR_BADDEVICE;

    if (FAILED(hr = d3drm_device_create(&object, iface)))
        return hr;

    if (SUCCEEDED(hr = d3drm_device_init(object, 1, ddraw, backbuffer, TRUE)))
        *device = &object->IDirect3DRMDevice_iface;
    else
        d3drm_device_destroy(object);

    return hr;
}

static HRESULT WINAPI d3drm1_CreateDeviceFromD3D(IDirect3DRM *iface,
        IDirect3D *d3d, IDirect3DDevice *d3d_device, IDirect3DRMDevice **device)
{
    struct d3drm_device *object;
    HRESULT hr;
    TRACE("iface %p, d3d %p, d3d_device %p, device %p.\n",
            iface, d3d, d3d_device, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    if (FAILED(hr = d3drm_device_create(&object, iface)))
        return hr;

    if (FAILED(hr = IDirect3DRMDevice_InitFromD3D(&object->IDirect3DRMDevice_iface, d3d, d3d_device)))
    {
        d3drm_device_destroy(object);
        return hr;
    }
    *device = &object->IDirect3DRMDevice_iface;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm1_CreateDeviceFromClipper(IDirect3DRM *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height,
        IDirect3DRMDevice **device)
{
    struct d3drm_device *object;
    IDirectDraw *ddraw;
    IDirectDrawSurface *render_target;
    HRESULT hr;

    TRACE("iface %p, clipper %p, guid %s, width %d, height %d, device %p.\n",
            iface, clipper, debugstr_guid(guid), width, height, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    if (!clipper || !width || !height)
        return D3DRMERR_BADVALUE;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = d3drm_device_create(&object, iface)))
    {
        IDirectDraw_Release(ddraw);
        return hr;
    }

    hr = d3drm_device_create_surfaces_from_clipper(object, ddraw, clipper, width, height, &render_target);
    if (FAILED(hr))
    {
        IDirectDraw_Release(ddraw);
        d3drm_device_destroy(object);
        return hr;
    }

    hr = d3drm_device_init(object, 1, ddraw, render_target, TRUE);
    IDirectDraw_Release(ddraw);
    IDirectDrawSurface_Release(render_target);
    if (FAILED(hr))
        d3drm_device_destroy(object);
    else
        *device = &object->IDirect3DRMDevice_iface;

    return hr;
}

static HRESULT WINAPI d3drm1_CreateTextureFromSurface(IDirect3DRM *iface,
        IDirectDrawSurface *surface, IDirect3DRMTexture **texture)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);
    IDirect3DRMTexture3 *texture3;
    HRESULT hr;

    TRACE("iface %p, surface %p, texture %p.\n", iface, surface, texture);

    if (!texture)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = IDirect3DRM3_CreateTextureFromSurface(&d3drm->IDirect3DRM3_iface, surface, &texture3)))
    {
        *texture = NULL;
        return hr;
    }

    hr = IDirect3DRMTexture3_QueryInterface(texture3, &IID_IDirect3DRMTexture, (void **)texture);
    IDirect3DRMTexture3_Release(texture3);

    return hr;
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
    struct d3drm *d3drm = impl_from_IDirect3DRM(iface);
    IDirect3DRMDevice3 *device3;
    IDirect3DRMFrame3 *camera3;
    IDirect3DRMViewport2 *viewport2;
    HRESULT hr;

    TRACE("iface %p, device %p, camera %p, x %lu, y %lu, width %lu, height %lu, viewport %p.\n",
            iface, device, camera, x, y, width, height, viewport);

    if (!viewport)
        return D3DRMERR_BADVALUE;
    *viewport = NULL;

    if (!device || !camera)
        return D3DRMERR_BADOBJECT;

    if (FAILED(hr = IDirect3DRMDevice_QueryInterface(device, &IID_IDirect3DRMDevice3, (void **)&device3)))
        return hr;

    if (FAILED(hr = IDirect3DRMFrame_QueryInterface(camera, &IID_IDirect3DRMFrame3, (void **)&camera3)))
    {
        IDirect3DRMDevice3_Release(device3);
        return hr;
    }

    hr = IDirect3DRM3_CreateViewport(&d3drm->IDirect3DRM3_iface, device3, camera3, x, y, width, height, &viewport2);
    IDirect3DRMDevice3_Release(device3);
    IDirect3DRMFrame3_Release(camera3);
    if (FAILED(hr))
        return hr;

    hr = IDirect3DRMViewport2_QueryInterface(viewport2, &IID_IDirect3DRMViewport, (void **)viewport);
    IDirect3DRMViewport2_Release(viewport2);

    return hr;
}

static HRESULT WINAPI d3drm1_CreateWrap(IDirect3DRM *iface, D3DRMWRAPTYPE type, IDirect3DRMFrame *frame,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv,
        IDirect3DRMWrap **wrap)
{
    struct d3drm_wrap *object;
    HRESULT hr;

    FIXME("iface %p, type %#x, frame %p, ox %.8e, oy %.8e, oz %.8e, dx %.8e, dy %.8e, dz %.8e, "
            "ux %.8e, uy %.8e, uz %.8e, ou %.8e, ov %.8e, su %.8e, sv %.8e, wrap %p, semi-stub.\n",
            iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, wrap);

    if (!wrap)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = d3drm_wrap_create(&object, iface)))
        return hr;

    *wrap = &object->IDirect3DRMWrap_iface;

    return S_OK;
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
    struct d3drm_texture *object;
    HRESULT hr;

    TRACE("iface %p, filename %s, texture %p.\n", iface, debugstr_a(filename), texture);

    if (!texture)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = d3drm_texture_create(&object, iface)))
        return hr;

    *texture = &object->IDirect3DRMTexture_iface;
    if (FAILED(hr = IDirect3DRMTexture_InitFromFile(*texture, filename)))
    {
        IDirect3DRMTexture_Release(*texture);
        *texture = NULL;
        if (!filename)
            return D3DRMERR_BADVALUE;

        return hr == D3DRMERR_BADOBJECT ? D3DRMERR_FILENOTFOUND : hr;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm1_LoadTextureFromResource(IDirect3DRM *iface,
        HRSRC resource, IDirect3DRMTexture **texture)
{
    struct d3drm_texture *object;
    HRESULT hr;

    FIXME("iface %p, resource %p, texture %p stub!\n", iface, resource, texture);

    if (FAILED(hr = d3drm_texture_create(&object, iface)))
        return hr;

    *texture = &object->IDirect3DRMTexture_iface;

    return D3DRM_OK;
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
    FIXME("iface %p, color_count %lu stub!\n", iface, color_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm1_SetDefaultTextureShades(IDirect3DRM *iface, DWORD shade_count)
{
    FIXME("iface %p, shade_count %lu stub!\n", iface, shade_count);

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

    TRACE("iface %p, source %p, object_id %p, iids %p, iid_count %lu, flags %#lx, "
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

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    if (refcount == 1)
        InterlockedIncrement(&d3drm->iface_count);

    return refcount;
}

static ULONG WINAPI d3drm2_Release(IDirect3DRM2 *iface)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    ULONG refcount = InterlockedDecrement(&d3drm->ref2);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount && !InterlockedDecrement(&d3drm->iface_count))
        d3drm_destroy(d3drm);

    return refcount;
}

static HRESULT WINAPI d3drm2_CreateObject(IDirect3DRM2 *iface,
        REFCLSID clsid, IUnknown *outer, REFIID iid, void **out)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, clsid %s, outer %p, iid %s, out %p.\n",
            iface, debugstr_guid(clsid), outer, debugstr_guid(iid), out);

    return IDirect3DRM3_CreateObject(&d3drm->IDirect3DRM3_iface, clsid, outer, iid, out);
}

static HRESULT WINAPI d3drm2_CreateFrame(IDirect3DRM2 *iface,
        IDirect3DRMFrame *parent_frame, IDirect3DRMFrame2 **frame)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    struct d3drm_frame *object;
    HRESULT hr;

    TRACE("iface %p, parent_frame %p, frame %p.\n", iface, parent_frame, frame);

    if (FAILED(hr = d3drm_frame_create(&object, (IUnknown *)parent_frame, &d3drm->IDirect3DRM_iface)))
        return hr;

    *frame = &object->IDirect3DRMFrame2_iface;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm2_CreateMesh(IDirect3DRM2 *iface, IDirect3DRMMesh **mesh)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return IDirect3DRM3_CreateMesh(&d3drm->IDirect3DRM3_iface, mesh);
}

static HRESULT WINAPI d3drm2_CreateMeshBuilder(IDirect3DRM2 *iface, IDirect3DRMMeshBuilder2 **mesh_builder)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    struct d3drm_mesh_builder *object;
    HRESULT hr;

    TRACE("iface %p, mesh_builder %p.\n", iface, mesh_builder);

    if (FAILED(hr = d3drm_mesh_builder_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *mesh_builder = &object->IDirect3DRMMeshBuilder2_iface;

    return S_OK;
}

static HRESULT WINAPI d3drm2_CreateFace(IDirect3DRM2 *iface, IDirect3DRMFace **face)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, face %p.\n", iface, face);

    return IDirect3DRM_CreateFace(&d3drm->IDirect3DRM_iface, face);
}

static HRESULT WINAPI d3drm2_CreateAnimation(IDirect3DRM2 *iface, IDirect3DRMAnimation **animation)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, animation %p.\n", iface, animation);

    return IDirect3DRM_CreateAnimation(&d3drm->IDirect3DRM_iface, animation);
}

static HRESULT WINAPI d3drm2_CreateAnimationSet(IDirect3DRM2 *iface, IDirect3DRMAnimationSet **set)
{
    FIXME("iface %p, set %p stub!\n", iface, set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_CreateTexture(IDirect3DRM2 *iface,
        D3DRMIMAGE *image, IDirect3DRMTexture2 **texture)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    IDirect3DRMTexture3 *texture3;
    HRESULT hr;

    TRACE("iface %p, image %p, texture %p.\n", iface, image, texture);

    if (!texture)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = IDirect3DRM3_CreateTexture(&d3drm->IDirect3DRM3_iface, image, &texture3)))
    {
        *texture = NULL;
        return hr;
    }

    hr = IDirect3DRMTexture3_QueryInterface(texture3, &IID_IDirect3DRMTexture2, (void **)texture);
    IDirect3DRMTexture3_Release(texture3);

    return hr;
}

static HRESULT WINAPI d3drm2_CreateLight(IDirect3DRM2 *iface,
        D3DRMLIGHTTYPE type, D3DCOLOR color, IDirect3DRMLight **light)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, type %#x, color 0x%08lx, light %p.\n", iface, type, color, light);

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
    TRACE("iface %p, width %lu, height %lu, device %p.\n", iface, width, height, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    return D3DRMERR_BADDEVICE;
}

static HRESULT WINAPI d3drm2_CreateDeviceFromSurface(IDirect3DRM2 *iface, GUID *guid,
        IDirectDraw *ddraw, IDirectDrawSurface *backbuffer, IDirect3DRMDevice2 **device)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    IDirect3DRMDevice3 *device3;
    HRESULT hr;
    TRACE("iface %p, guid %s, ddraw %p, backbuffer %p, device %p.\n",
            iface, debugstr_guid(guid), ddraw, backbuffer, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;
    hr = IDirect3DRM3_CreateDeviceFromSurface(&d3drm->IDirect3DRM3_iface, guid, ddraw, backbuffer, 0, &device3);
    if (FAILED(hr))
        return hr;

    hr = IDirect3DRMDevice3_QueryInterface(device3, &IID_IDirect3DRMDevice2, (void**)device);
    IDirect3DRMDevice3_Release(device3);

    return hr;
}

static HRESULT WINAPI d3drm2_CreateDeviceFromD3D(IDirect3DRM2 *iface,
    IDirect3D2 *d3d, IDirect3DDevice2 *d3d_device, IDirect3DRMDevice2 **device)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    IDirect3DRMDevice3 *device3;
    HRESULT hr;

    TRACE("iface %p, d3d %p, d3d_device %p, device %p.\n",
            iface, d3d, d3d_device, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    hr = IDirect3DRM3_CreateDeviceFromD3D(&d3drm->IDirect3DRM3_iface, d3d, d3d_device, &device3);
    if (FAILED(hr))
        return hr;

    hr = IDirect3DRMDevice3_QueryInterface(device3, &IID_IDirect3DRMDevice2, (void **)device);
    IDirect3DRMDevice3_Release(device3);

    return hr;
}

static HRESULT WINAPI d3drm2_CreateDeviceFromClipper(IDirect3DRM2 *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height,
        IDirect3DRMDevice2 **device)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    IDirect3DRMDevice3 *device3;
    HRESULT hr;

    TRACE("iface %p, clipper %p, guid %s, width %d, height %d, device %p.\n",
            iface, clipper, debugstr_guid(guid), width, height, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;
    hr = IDirect3DRM3_CreateDeviceFromClipper(&d3drm->IDirect3DRM3_iface, clipper, guid, width, height, &device3);
    if (FAILED(hr))
        return hr;

    hr = IDirect3DRMDevice3_QueryInterface(device3, &IID_IDirect3DRMDevice2, (void**)device);
    IDirect3DRMDevice3_Release(device3);

    return hr;
}

static HRESULT WINAPI d3drm2_CreateTextureFromSurface(IDirect3DRM2 *iface,
        IDirectDrawSurface *surface, IDirect3DRMTexture2 **texture)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    IDirect3DRMTexture3 *texture3;
    HRESULT hr;

    TRACE("iface %p, surface %p, texture %p.\n", iface, surface, texture);

    if (!texture)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = IDirect3DRM3_CreateTextureFromSurface(&d3drm->IDirect3DRM3_iface, surface, &texture3)))
    {
        *texture = NULL;
        return hr;
    }

    hr = IDirect3DRMTexture3_QueryInterface(texture3, &IID_IDirect3DRMTexture2, (void **)texture);
    IDirect3DRMTexture3_Release(texture3);

    return hr;
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
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    IDirect3DRMDevice3 *device3;
    IDirect3DRMFrame3 *camera3;
    IDirect3DRMViewport2 *viewport2;
    HRESULT hr;

    TRACE("iface %p, device %p, camera %p, x %lu, y %lu, width %lu, height %lu, viewport %p.\n",
          iface, device, camera, x, y, width, height, viewport);

    if (!viewport)
        return D3DRMERR_BADVALUE;
    *viewport = NULL;

    if (!device || !camera)
        return D3DRMERR_BADOBJECT;

    if (FAILED(hr = IDirect3DRMDevice_QueryInterface(device, &IID_IDirect3DRMDevice3, (void **)&device3)))
        return hr;

    if (FAILED(hr = IDirect3DRMFrame_QueryInterface(camera, &IID_IDirect3DRMFrame3, (void **)&camera3)))
    {
        IDirect3DRMDevice3_Release(device3);
        return hr;
    }

    hr = IDirect3DRM3_CreateViewport(&d3drm->IDirect3DRM3_iface, device3, camera3, x, y, width, height, &viewport2);
    IDirect3DRMDevice3_Release(device3);
    IDirect3DRMFrame3_Release(camera3);
    if (FAILED(hr))
        return hr;

    hr = IDirect3DRMViewport2_QueryInterface(viewport2, &IID_IDirect3DRMViewport, (void **)viewport);
    IDirect3DRMViewport2_Release(viewport2);

    return hr;
}

static HRESULT WINAPI d3drm2_CreateWrap(IDirect3DRM2 *iface, D3DRMWRAPTYPE type, IDirect3DRMFrame *frame,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv,
        IDirect3DRMWrap **wrap)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, type %#x, frame %p, ox %.8e, oy %.8e, oz %.8e, dx %.8e, dy %.8e, dz %.8e, "
            "ux %.8e, uy %.8e, uz %.8e, ou %.8e, ov %.8e, su %.8e, sv %.8e, wrap %p.\n",
            iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, wrap);

    return IDirect3DRM_CreateWrap(&d3drm->IDirect3DRM_iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz,
            ou, ov, su, sv, wrap);
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
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    IDirect3DRMTexture3 *texture3;
    HRESULT hr;

    TRACE("iface %p, filename %s, texture %p.\n", iface, debugstr_a(filename), texture);

    if (!texture)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = IDirect3DRM3_LoadTexture(&d3drm->IDirect3DRM3_iface, filename, &texture3)))
    {
        *texture = NULL;
        return hr;
    }

    hr = IDirect3DRMTexture3_QueryInterface(texture3, &IID_IDirect3DRMTexture2, (void **)texture);
    IDirect3DRMTexture3_Release(texture3);

    return hr;
}

static HRESULT WINAPI d3drm2_LoadTextureFromResource(IDirect3DRM2 *iface, HMODULE module,
        const char *resource_name, const char *resource_type, IDirect3DRMTexture2 **texture)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM2(iface);
    struct d3drm_texture *object;
    HRESULT hr;

    FIXME("iface %p, resource_name %s, resource_type %s, texture %p stub!\n",
            iface, debugstr_a(resource_name), debugstr_a(resource_type), texture);

    if (FAILED(hr = d3drm_texture_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *texture = &object->IDirect3DRMTexture2_iface;

    return D3DRM_OK;
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
    FIXME("iface %p, color_count %lu stub!\n", iface, color_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm2_SetDefaultTextureShades(IDirect3DRM2 *iface, DWORD shade_count)
{
    FIXME("iface %p, shade_count %lu stub!\n", iface, shade_count);

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

    TRACE("iface %p, source %p, object_id %p, iids %p, iid_count %lu, flags %#lx, "
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

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    if (refcount == 1)
        InterlockedIncrement(&d3drm->iface_count);

    return refcount;
}

static ULONG WINAPI d3drm3_Release(IDirect3DRM3 *iface)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    ULONG refcount = InterlockedDecrement(&d3drm->ref3);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount && !InterlockedDecrement(&d3drm->iface_count))
        d3drm_destroy(d3drm);

    return refcount;
}

static HRESULT WINAPI d3drm3_CreateObject(IDirect3DRM3 *iface,
        REFCLSID clsid, IUnknown *outer, REFIID iid, void **out)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    IUnknown *object;
    unsigned int i;
    HRESULT hr;

    static const struct
    {
        const CLSID *clsid;
        HRESULT (*create_object)(void **object, IDirect3DRM *d3drm);
    }
    object_table[] =
    {
        {&CLSID_CDirect3DRMTexture, d3drm_create_texture_object},
        {&CLSID_CDirect3DRMDevice, d3drm_create_device_object},
        {&CLSID_CDirect3DRMViewport, d3drm_create_viewport_object},
        {&CLSID_CDirect3DRMFace, d3drm_create_face_object},
        {&CLSID_CDirect3DRMMeshBuilder, d3drm_create_mesh_builder_object},
        {&CLSID_CDirect3DRMFrame, d3drm_create_frame_object},
        {&CLSID_CDirect3DRMLight, d3drm_create_light_object},
        {&CLSID_CDirect3DRMMaterial, d3drm_create_material_object},
        {&CLSID_CDirect3DRMMesh, d3drm_create_mesh_object},
        {&CLSID_CDirect3DRMAnimation, d3drm_create_animation_object},
        {&CLSID_CDirect3DRMWrap, d3drm_create_wrap_object},
    };

    TRACE("iface %p, clsid %s, outer %p, iid %s, out %p.\n",
            iface, debugstr_guid(clsid), outer, debugstr_guid(iid), out);

    if (!out)
        return D3DRMERR_BADVALUE;

    if (!clsid || !iid)
    {
        *out = NULL;
        return D3DRMERR_BADVALUE;
    }

    if (outer)
    {
        FIXME("COM aggregation for outer IUnknown (%p) not implemented. Returning E_NOTIMPL.\n", outer);
        *out = NULL;
        return E_NOTIMPL;
    }

    for (i = 0; i < ARRAY_SIZE(object_table); ++i)
    {
        if (IsEqualGUID(clsid, object_table[i].clsid))
        {
            if (FAILED(hr = object_table[i].create_object((void **)&object, &d3drm->IDirect3DRM_iface)))
            {
                *out = NULL;
                return hr;
            }
            break;
        }
    }
    if (i == ARRAY_SIZE(object_table))
    {
        FIXME("%s not implemented. Returning CLASSFACTORY_E_FIRST.\n", debugstr_guid(clsid));
        *out = NULL;
        return CLASSFACTORY_E_FIRST;
    }

    if (FAILED(hr = IUnknown_QueryInterface(object, iid, out)))
        *out = NULL;
    IUnknown_Release(object);

    return hr;
}

static HRESULT WINAPI d3drm3_CreateFrame(IDirect3DRM3 *iface,
        IDirect3DRMFrame3 *parent, IDirect3DRMFrame3 **frame)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_frame *object;
    HRESULT hr;

    TRACE("iface %p, parent %p, frame %p.\n", iface, parent, frame);

    if (FAILED(hr = d3drm_frame_create(&object, (IUnknown *)parent, &d3drm->IDirect3DRM_iface)))
        return hr;

    *frame = &object->IDirect3DRMFrame3_iface;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm3_CreateMesh(IDirect3DRM3 *iface, IDirect3DRMMesh **mesh)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_mesh *object;
    HRESULT hr;

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    if (FAILED(hr = d3drm_mesh_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *mesh = &object->IDirect3DRMMesh_iface;

    return S_OK;
}

static HRESULT WINAPI d3drm3_CreateMeshBuilder(IDirect3DRM3 *iface, IDirect3DRMMeshBuilder3 **mesh_builder)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_mesh_builder *object;
    HRESULT hr;

    TRACE("iface %p, mesh_builder %p.\n", iface, mesh_builder);

    if (FAILED(hr = d3drm_mesh_builder_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *mesh_builder = &object->IDirect3DRMMeshBuilder3_iface;

    return S_OK;
}

static HRESULT WINAPI d3drm3_CreateFace(IDirect3DRM3 *iface, IDirect3DRMFace2 **face)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    IDirect3DRMFace *face1;
    HRESULT hr;

    TRACE("iface %p, face %p.\n", iface, face);

    if (FAILED(hr = IDirect3DRM_CreateFace(&d3drm->IDirect3DRM_iface, &face1)))
        return hr;

    hr = IDirect3DRMFace_QueryInterface(face1, &IID_IDirect3DRMFace2, (void **)face);
    IDirect3DRMFace_Release(face1);

    return hr;
}

static HRESULT WINAPI d3drm3_CreateAnimation(IDirect3DRM3 *iface, IDirect3DRMAnimation2 **animation)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_animation *object;
    HRESULT hr;

    TRACE("iface %p, animation %p.\n", iface, animation);

    if (FAILED(hr = d3drm_animation_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *animation = &object->IDirect3DRMAnimation2_iface;

    return hr;
}

static HRESULT WINAPI d3drm3_CreateAnimationSet(IDirect3DRM3 *iface, IDirect3DRMAnimationSet2 **set)
{
    FIXME("iface %p, set %p stub!\n", iface, set);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_CreateTexture(IDirect3DRM3 *iface,
        D3DRMIMAGE *image, IDirect3DRMTexture3 **texture)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_texture *object;
    HRESULT hr;

    TRACE("iface %p, image %p, texture %p.\n", iface, image, texture);

    if (!texture)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = d3drm_texture_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *texture = &object->IDirect3DRMTexture3_iface;

    if (FAILED(IDirect3DRMTexture3_InitFromImage(*texture, image)))
    {
        IDirect3DRMTexture3_Release(*texture);
        *texture = NULL;
        return D3DRMERR_BADVALUE;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm3_CreateLight(IDirect3DRM3 *iface,
        D3DRMLIGHTTYPE type, D3DCOLOR color, IDirect3DRMLight **light)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_light *object;
    HRESULT hr;

    TRACE("iface %p, type %#x, color 0x%08lx, light %p.\n", iface, type, color, light);

    if (SUCCEEDED(hr = d3drm_light_create(&object, &d3drm->IDirect3DRM_iface)))
    {
        IDirect3DRMLight_SetType(&object->IDirect3DRMLight_iface, type);
        IDirect3DRMLight_SetColor(&object->IDirect3DRMLight_iface, color);
    }

    *light = &object->IDirect3DRMLight_iface;

    return hr;
}

static HRESULT WINAPI d3drm3_CreateLightRGB(IDirect3DRM3 *iface, D3DRMLIGHTTYPE type,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue, IDirect3DRMLight **light)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_light *object;
    HRESULT hr;

    TRACE("iface %p, type %#x, red %.8e, green %.8e, blue %.8e, light %p.\n",
            iface, type, red, green, blue, light);

    if (SUCCEEDED(hr = d3drm_light_create(&object, &d3drm->IDirect3DRM_iface)))
    {
        IDirect3DRMLight_SetType(&object->IDirect3DRMLight_iface, type);
        IDirect3DRMLight_SetColorRGB(&object->IDirect3DRMLight_iface, red, green, blue);
    }

    *light = &object->IDirect3DRMLight_iface;

    return hr;
}

static HRESULT WINAPI d3drm3_CreateMaterial(IDirect3DRM3 *iface,
        D3DVALUE power, IDirect3DRMMaterial2 **material)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_material *object;
    HRESULT hr;

    TRACE("iface %p, power %.8e, material %p.\n", iface, power, material);

    if (SUCCEEDED(hr = d3drm_material_create(&object, &d3drm->IDirect3DRM_iface)))
        IDirect3DRMMaterial2_SetPower(&object->IDirect3DRMMaterial2_iface, power);

    *material = &object->IDirect3DRMMaterial2_iface;

    return hr;
}

static HRESULT WINAPI d3drm3_CreateDevice(IDirect3DRM3 *iface,
        DWORD width, DWORD height, IDirect3DRMDevice3 **device)
{
    TRACE("iface %p, width %lu, height %lu, device %p.\n", iface, width, height, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    return D3DRMERR_BADDEVICE;
}

static HRESULT WINAPI d3drm3_CreateDeviceFromSurface(IDirect3DRM3 *iface, GUID *guid,
        IDirectDraw *ddraw, IDirectDrawSurface *backbuffer, DWORD flags, IDirect3DRMDevice3 **device)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_device *object;
    BOOL use_z_surface;
    HRESULT hr;

    TRACE("iface %p, guid %s, ddraw %p, backbuffer %p, flags %#lx, device %p.\n",
            iface, debugstr_guid(guid), ddraw, backbuffer, flags, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    if (!backbuffer || !ddraw)
        return D3DRMERR_BADDEVICE;

    if (FAILED(hr = d3drm_device_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    use_z_surface = !(flags & D3DRMDEVICE_NOZBUFFER);

    if (SUCCEEDED(hr = d3drm_device_init(object, 3, ddraw, backbuffer, use_z_surface)))
        *device = &object->IDirect3DRMDevice3_iface;
    else
        d3drm_device_destroy(object);

    return hr;
}

static HRESULT WINAPI d3drm3_CreateDeviceFromD3D(IDirect3DRM3 *iface,
        IDirect3D2 *d3d, IDirect3DDevice2 *d3d_device, IDirect3DRMDevice3 **device)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_device *object;
    HRESULT hr;

    TRACE("iface %p, d3d %p, d3d_device %p, device %p.\n",
            iface, d3d, d3d_device, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    if (FAILED(hr = d3drm_device_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    if (FAILED(hr = IDirect3DRMDevice3_InitFromD3D2(&object->IDirect3DRMDevice3_iface, d3d, d3d_device)))
    {
        d3drm_device_destroy(object);
        return hr;
    }
    *device = &object->IDirect3DRMDevice3_iface;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm3_CreateDeviceFromClipper(IDirect3DRM3 *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height,
        IDirect3DRMDevice3 **device)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_device *object;
    IDirectDraw *ddraw;
    IDirectDrawSurface *render_target;
    HRESULT hr;

    TRACE("iface %p, clipper %p, guid %s, width %d, height %d, device %p.\n",
            iface, clipper, debugstr_guid(guid), width, height, device);

    if (!device)
        return D3DRMERR_BADVALUE;
    *device = NULL;

    if (!clipper || !width || !height)
        return D3DRMERR_BADVALUE;

    hr = DirectDrawCreate(NULL, &ddraw, NULL);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = d3drm_device_create(&object, &d3drm->IDirect3DRM_iface)))
    {
        IDirectDraw_Release(ddraw);
        return hr;
    }

    hr = d3drm_device_create_surfaces_from_clipper(object, ddraw, clipper, width, height, &render_target);
    if (FAILED(hr))
    {
        IDirectDraw_Release(ddraw);
        d3drm_device_destroy(object);
        return hr;
    }

    hr = d3drm_device_init(object, 3, ddraw, render_target, TRUE);
    IDirectDraw_Release(ddraw);
    IDirectDrawSurface_Release(render_target);
    if (FAILED(hr))
        d3drm_device_destroy(object);
    else
        *device = &object->IDirect3DRMDevice3_iface;

    return hr;
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
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_texture *object;
    HRESULT hr;

    TRACE("iface %p, surface %p, texture %p.\n", iface, surface, texture);

    if (!texture)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = d3drm_texture_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *texture = &object->IDirect3DRMTexture3_iface;

    if (FAILED(IDirect3DRMTexture3_InitFromSurface(*texture, surface)))
    {
        IDirect3DRMTexture3_Release(*texture);
        *texture = NULL;
        return D3DRMERR_BADVALUE;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm3_CreateViewport(IDirect3DRM3 *iface, IDirect3DRMDevice3 *device,
        IDirect3DRMFrame3 *camera, DWORD x, DWORD y, DWORD width, DWORD height, IDirect3DRMViewport2 **viewport)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_viewport *object;
    HRESULT hr;

    TRACE("iface %p, device %p, camera %p, x %lu, y %lu, width %lu, height %lu, viewport %p.\n",
            iface, device, camera, x, y, width, height, viewport);

    if (!viewport)
        return D3DRMERR_BADVALUE;
    *viewport = NULL;

    if (!device || !camera)
        return D3DRMERR_BADOBJECT;

    if (FAILED(hr = d3drm_viewport_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *viewport = &object->IDirect3DRMViewport2_iface;

    if (FAILED(hr = IDirect3DRMViewport2_Init(*viewport, device, camera, x, y, width, height)))
    {
        IDirect3DRMViewport2_Release(*viewport);
        *viewport = NULL;
        return D3DRMERR_BADVALUE;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm3_CreateWrap(IDirect3DRM3 *iface, D3DRMWRAPTYPE type, IDirect3DRMFrame3 *frame,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv,
        IDirect3DRMWrap **wrap)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_wrap *object;
    HRESULT hr;

    FIXME("iface %p, type %#x, frame %p, ox %.8e, oy %.8e, oz %.8e, dx %.8e, dy %.8e, dz %.8e, "
            "ux %.8e, uy %.8e, uz %.8e, ou %.8e, ov %.8e, su %.8e, sv %.8e, wrap %p, semi-stub.\n",
            iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, wrap);

    if (!wrap)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = d3drm_wrap_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *wrap = &object->IDirect3DRMWrap_iface;

    return S_OK;
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
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_texture *object;
    HRESULT hr;

    TRACE("iface %p, filename %s, texture %p.\n", iface, debugstr_a(filename), texture);

    if (!texture)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = d3drm_texture_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *texture = &object->IDirect3DRMTexture3_iface;
    if (FAILED(hr = IDirect3DRMTexture3_InitFromFile(*texture, filename)))
    {
        IDirect3DRMTexture3_Release(*texture);
        *texture = NULL;
        return hr == D3DRMERR_BADOBJECT ? D3DRMERR_FILENOTFOUND : hr;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm3_LoadTextureFromResource(IDirect3DRM3 *iface, HMODULE module,
        const char *resource_name, const char *resource_type, IDirect3DRMTexture3 **texture)
{
    struct d3drm *d3drm = impl_from_IDirect3DRM3(iface);
    struct d3drm_texture *object;
    HRESULT hr;

    FIXME("iface %p, module %p, resource_name %s, resource_type %s, texture %p stub!\n",
            iface, module, debugstr_a(resource_name), debugstr_a(resource_type), texture);

    if (FAILED(hr = d3drm_texture_create(&object, &d3drm->IDirect3DRM_iface)))
        return hr;

    *texture = &object->IDirect3DRMTexture3_iface;

    return D3DRM_OK;
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
    FIXME("iface %p, color_count %lu stub!\n", iface, color_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm3_SetDefaultTextureShades(IDirect3DRM3 *iface, DWORD shade_count)
{
    FIXME("iface %p, shade_count %lu stub!\n", iface, shade_count);

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

    TRACE("iface %p, source %p, object_id %p, iids %p, iid_count %lu, flags %#lx, "
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
        FIXME("Load options %#lx not supported yet.\n", flags);
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

    TRACE("Version is %u.%u, flags %#lx.\n", header->major, header->minor, header->flags);

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
    FIXME("iface %p, flags %#lx stub!\n", iface, flags);

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

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRM_iface.lpVtbl = &d3drm1_vtbl;
    object->IDirect3DRM2_iface.lpVtbl = &d3drm2_vtbl;
    object->IDirect3DRM3_iface.lpVtbl = &d3drm3_vtbl;
    object->ref1 = 1;
    object->iface_count = 1;

    *d3drm = &object->IDirect3DRM_iface;

    return S_OK;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    TRACE("(%s, %s, %p): stub\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(!ppv)
        return E_INVALIDARG;

    return CLASS_E_CLASSNOTAVAILABLE;
}
