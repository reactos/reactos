/*
 * Implementation of IDirect3DRMViewport Interface
 *
 * Copyright 2012 André Hentschel
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

static inline struct d3drm_viewport *impl_from_IDirect3DRMViewport(IDirect3DRMViewport *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_viewport, IDirect3DRMViewport_iface);
}

static inline struct d3drm_viewport *impl_from_IDirect3DRMViewport2(IDirect3DRMViewport2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_viewport, IDirect3DRMViewport2_iface);
}

static inline void d3drm_normalize_d3d_color(D3DCOLORVALUE *color_value, D3DCOLOR color)
{
    color_value->r = RGBA_GETRED(color) / 255.0f;
    color_value->g = RGBA_GETGREEN(color) / 255.0f;
    color_value->b = RGBA_GETBLUE(color) / 255.0f;
    color_value->a = RGBA_GETALPHA(color) / 255.0f;
}

static HRESULT d3drm_update_background_material(struct d3drm_viewport *viewport)
{
    IDirect3DRMFrame *root_frame;
    D3DCOLOR color;
    D3DMATERIAL mat;
    HRESULT hr;

    if (FAILED(hr = IDirect3DRMFrame_GetScene(viewport->camera, &root_frame)))
        return hr;
    color = IDirect3DRMFrame_GetSceneBackground(root_frame);
    IDirect3DRMFrame_Release(root_frame);

    memset(&mat, 0, sizeof(mat));
    mat.dwSize = sizeof(mat);
    d3drm_normalize_d3d_color(&mat.diffuse, color);

    return IDirect3DMaterial_SetMaterial(viewport->material, &mat);
}

static void d3drm_viewport_destroy(struct d3drm_viewport *viewport)
{
    TRACE("viewport %p releasing attached interfaces.\n", viewport);

    d3drm_object_cleanup((IDirect3DRMObject *)&viewport->IDirect3DRMViewport_iface, &viewport->obj);

    if (viewport->d3d_viewport)
    {
        IDirect3DViewport_Release(viewport->d3d_viewport);
        IDirect3DMaterial_Release(viewport->material);
        IDirect3DRMFrame_Release(viewport->camera);
        IDirect3DRM_Release(viewport->d3drm);
    }

    free(viewport);
}

static HRESULT WINAPI d3drm_viewport2_QueryInterface(IDirect3DRMViewport2 *iface, REFIID riid, void **out)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMViewport)
            || IsEqualGUID(riid, &IID_IDirect3DRMObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &viewport->IDirect3DRMViewport_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMViewport2))
    {
        *out = &viewport->IDirect3DRMViewport2_iface;
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

static HRESULT WINAPI d3drm_viewport1_QueryInterface(IDirect3DRMViewport *iface, REFIID riid, void **out)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return d3drm_viewport2_QueryInterface(&viewport->IDirect3DRMViewport2_iface, riid, out);
}

static ULONG WINAPI d3drm_viewport2_AddRef(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);
    ULONG refcount = InterlockedIncrement(&viewport->obj.ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_viewport1_AddRef(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport2_AddRef(&viewport->IDirect3DRMViewport2_iface);
}

static ULONG WINAPI d3drm_viewport2_Release(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);
    ULONG refcount = InterlockedDecrement(&viewport->obj.ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        d3drm_viewport_destroy(viewport);

    return refcount;
}

static ULONG WINAPI d3drm_viewport1_Release(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport2_Release(&viewport->IDirect3DRMViewport2_iface);
}

static HRESULT WINAPI d3drm_viewport2_Clone(IDirect3DRMViewport2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_Clone(IDirect3DRMViewport *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_AddDestroyCallback(IDirect3DRMViewport2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return d3drm_object_add_destroy_callback(&viewport->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_viewport1_AddDestroyCallback(IDirect3DRMViewport *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return d3drm_viewport2_AddDestroyCallback(&viewport->IDirect3DRMViewport2_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_viewport2_DeleteDestroyCallback(IDirect3DRMViewport2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return d3drm_object_delete_destroy_callback(&viewport->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_viewport1_DeleteDestroyCallback(IDirect3DRMViewport *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, cb %p, ctx %p\n", iface, cb, ctx);

    return d3drm_viewport2_DeleteDestroyCallback(&viewport->IDirect3DRMViewport2_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_viewport2_SetAppData(IDirect3DRMViewport2 *iface, DWORD data)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, data %#lx\n", iface, data);

    viewport->obj.appdata = data;
    return S_OK;
}

static HRESULT WINAPI d3drm_viewport1_SetAppData(IDirect3DRMViewport *iface, DWORD data)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, data %#lx\n", iface, data);

    return d3drm_viewport2_SetAppData(&viewport->IDirect3DRMViewport2_iface, data);
}

static DWORD WINAPI d3drm_viewport2_GetAppData(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p\n", iface);

    return viewport->obj.appdata;
}

static DWORD WINAPI d3drm_viewport1_GetAppData(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport2_GetAppData(&viewport->IDirect3DRMViewport2_iface);
}

static HRESULT WINAPI d3drm_viewport2_SetName(IDirect3DRMViewport2 *iface, const char *name)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_object_set_name(&viewport->obj, name);
}

static HRESULT WINAPI d3drm_viewport1_SetName(IDirect3DRMViewport *iface, const char *name)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_viewport2_SetName(&viewport->IDirect3DRMViewport2_iface, name);
}

static HRESULT WINAPI d3drm_viewport2_GetName(IDirect3DRMViewport2 *iface, DWORD *size, char *name)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_name(&viewport->obj, size, name);
}

static HRESULT WINAPI d3drm_viewport1_GetName(IDirect3DRMViewport *iface, DWORD *size, char *name)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_viewport2_GetName(&viewport->IDirect3DRMViewport2_iface, size, name);
}

static HRESULT WINAPI d3drm_viewport2_GetClassName(IDirect3DRMViewport2 *iface, DWORD *size, char *name)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_class_name(&viewport->obj, size, name);
}

static HRESULT WINAPI d3drm_viewport1_GetClassName(IDirect3DRMViewport *iface, DWORD *size, char *name)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_viewport2_GetClassName(&viewport->IDirect3DRMViewport2_iface, size, name);
}

static HRESULT WINAPI d3drm_viewport2_Init(IDirect3DRMViewport2 *iface, IDirect3DRMDevice3 *device,
        IDirect3DRMFrame3 *camera, DWORD x, DWORD y, DWORD width, DWORD height)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);
    struct d3drm_device *device_obj = unsafe_impl_from_IDirect3DRMDevice3(device);
    D3DVIEWPORT vp;
    D3DVALUE scale;
    IDirect3D *d3d1 = NULL;
    IDirect3DDevice *d3d_device = NULL;
    IDirect3DMaterial *material = NULL;
    D3DMATERIALHANDLE hmat;
    HRESULT hr = D3DRM_OK;

    TRACE("iface %p, device %p, camera %p, x %lu, y %lu, width %lu, height %lu.\n",
            iface, device, camera, x, y, width, height);

    if (!device_obj || !camera
            || width > device_obj->width
            || height > device_obj->height)
    {
        return D3DRMERR_BADOBJECT;
    }

    if (viewport->d3d_viewport)
        return D3DRMERR_BADOBJECT;

    IDirect3DRM_AddRef(viewport->d3drm);

    if (FAILED(hr = IDirect3DRMDevice3_GetDirect3DDevice(device, &d3d_device)))
        goto cleanup;

    if (FAILED(hr = IDirect3DDevice_GetDirect3D(d3d_device, &d3d1)))
        goto cleanup;

    if (FAILED(hr = IDirect3D_CreateViewport(d3d1, &viewport->d3d_viewport, NULL)))
        goto cleanup;

    if (FAILED(hr = IDirect3DDevice_AddViewport(d3d_device, viewport->d3d_viewport)))
        goto cleanup;

    vp.dwSize = sizeof(vp);
    vp.dwWidth = width;
    vp.dwHeight = height;
    vp.dwX = x;
    vp.dwY = y;
    scale = width > height ? (float)width / 2.0f : (float)height / 2.0f;
    vp.dvScaleX = scale;
    vp.dvScaleY = scale;
    vp.dvMaxX = vp.dwWidth / (2.0f * vp.dvScaleX);
    vp.dvMaxY = vp.dwHeight / (2.0f * vp.dvScaleY);
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;

    if (FAILED(hr = IDirect3DViewport_SetViewport(viewport->d3d_viewport, &vp)))
        goto cleanup;

    if (FAILED(hr = IDirect3DRMFrame3_QueryInterface(camera, &IID_IDirect3DRMFrame, (void **)&viewport->camera)))
        goto cleanup;

    if (FAILED(hr = IDirect3D_CreateMaterial(d3d1, &material, NULL)))
        goto cleanup;

    if (FAILED(hr = IDirect3DMaterial_GetHandle(material, d3d_device, &hmat)))
        goto cleanup;

    hr = IDirect3DViewport_SetBackground(viewport->d3d_viewport, hmat);
    viewport->material = material;
    viewport->device = device_obj;

    viewport->clip.left = -0.5f;
    viewport->clip.top = 0.5f;
    viewport->clip.right = 0.5f;
    viewport->clip.bottom = -0.5f;
    viewport->clip.front = 1.0f;
    viewport->clip.back = 100.0f;

cleanup:

    if (FAILED(hr))
    {
        if (viewport->d3d_viewport)
        {
            IDirect3DViewport_Release(viewport->d3d_viewport);
            viewport->d3d_viewport = NULL;
        }
        if (viewport->camera)
            IDirect3DRMFrame_Release(viewport->camera);
        if (material)
            IDirect3DMaterial_Release(material);
        IDirect3DRM_Release(viewport->d3drm);
    }
    if (d3d_device)
        IDirect3DDevice_Release(d3d_device);
    if (d3d1)
        IDirect3D_Release(d3d1);

    return hr;
}

static HRESULT WINAPI d3drm_viewport1_Init(IDirect3DRMViewport *iface, IDirect3DRMDevice *device,
        IDirect3DRMFrame *camera, DWORD x, DWORD y, DWORD width, DWORD height)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);
    struct d3drm_frame *frame = unsafe_impl_from_IDirect3DRMFrame(camera);
    IDirect3DRMDevice3 *device3;
    HRESULT hr;

    TRACE("iface %p, device %p, camera %p, x %lu, y %lu, width %lu, height %lu.\n",
          iface, device, camera, x, y, width, height);

    if (!device || !frame)
        return D3DRMERR_BADOBJECT;

    if (FAILED(hr = IDirect3DRMDevice_QueryInterface(device, &IID_IDirect3DRMDevice3, (void **)&device3)))
        return hr;

    hr = d3drm_viewport2_Init(&viewport->IDirect3DRMViewport2_iface, device3, &frame->IDirect3DRMFrame3_iface,
            x, y, width, height);
    IDirect3DRMDevice3_Release(device3);

    return hr;
}

static HRESULT WINAPI d3drm_viewport2_Clear(IDirect3DRMViewport2 *iface, DWORD flags)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);
    DDSCAPS caps = { DDSCAPS_ZBUFFER };
    HRESULT hr;
    D3DRECT clear_rect;
    IDirectDrawSurface *ds;
    DWORD clear_flags = 0;

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    clear_rect.x1 = clear_rect.y1 = 0;
    clear_rect.x2 = viewport->device->width;
    clear_rect.y2 = viewport->device->height;

    if (flags & D3DRMCLEAR_TARGET)
    {
        clear_flags |= D3DCLEAR_TARGET;
        d3drm_update_background_material(viewport);
    }
    if (flags & D3DRMCLEAR_ZBUFFER)
    {
        hr = IDirectDrawSurface_GetAttachedSurface(viewport->device->render_target, &caps, &ds);
        if (SUCCEEDED(hr))
        {
            clear_flags |= D3DCLEAR_ZBUFFER;
            IDirectDrawSurface_Release(ds);
        }
    }
    if (flags & D3DRMCLEAR_DIRTYRECTS)
        FIXME("Flag D3DRMCLEAR_DIRTYRECT not implemented yet.\n");

    if (FAILED(hr = IDirect3DViewport_Clear(viewport->d3d_viewport, 1, &clear_rect, clear_flags)))
        return hr;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_Clear(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport2_Clear(&viewport->IDirect3DRMViewport2_iface, D3DRMCLEAR_ALL);
}

static HRESULT WINAPI d3drm_viewport2_Render(IDirect3DRMViewport2 *iface, IDirect3DRMFrame3 *frame)
{
    FIXME("iface %p, frame %p stub!\n", iface, frame);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_Render(IDirect3DRMViewport *iface, IDirect3DRMFrame *frame)
{
    FIXME("iface %p, frame %p stub!\n", iface, frame);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_SetFront(IDirect3DRMViewport2 *iface, D3DVALUE front)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, front %.8e.\n", iface, front);

    if (!viewport->d3d_viewport)
        return D3DRMERR_BADOBJECT;

    if (front <= 0.0f)
        return D3DRMERR_BADVALUE;

    viewport->clip.front = front;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_SetFront(IDirect3DRMViewport *iface, D3DVALUE front)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, front %.8e.\n", iface, front);

    return d3drm_viewport2_SetFront(&viewport->IDirect3DRMViewport2_iface, front);
}

static HRESULT WINAPI d3drm_viewport2_SetBack(IDirect3DRMViewport2 *iface, D3DVALUE back)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, back %.8e.\n", iface, back);

    if (!viewport->d3d_viewport)
        return D3DRMERR_BADOBJECT;

    if (back <= viewport->clip.front)
        return D3DRMERR_BADVALUE;

    viewport->clip.back = back;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_SetBack(IDirect3DRMViewport *iface, D3DVALUE back)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, back %.8e.\n", iface, back);

    return d3drm_viewport2_SetBack(&viewport->IDirect3DRMViewport2_iface, back);
}

static HRESULT WINAPI d3drm_viewport2_SetField(IDirect3DRMViewport2 *iface, D3DVALUE field)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, field %.8e.\n", iface, field);

    if (!viewport->d3d_viewport)
        return D3DRMERR_BADOBJECT;

    if (field <= 0.0f)
        return D3DRMERR_BADVALUE;

    viewport->clip.left = -field;
    viewport->clip.right = field;
    viewport->clip.bottom = -field;
    viewport->clip.top = field;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_SetField(IDirect3DRMViewport *iface, D3DVALUE field)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, field %.8e.\n", iface, field);

    return d3drm_viewport2_SetField(&viewport->IDirect3DRMViewport2_iface, field);
}

static HRESULT WINAPI d3drm_viewport2_SetUniformScaling(IDirect3DRMViewport2 *iface, BOOL b)
{
    FIXME("iface %p, b %#x stub!\n", iface, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_SetUniformScaling(IDirect3DRMViewport *iface, BOOL b)
{
    FIXME("iface %p, b %#x stub!\n", iface, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_SetCamera(IDirect3DRMViewport2 *iface, IDirect3DRMFrame3 *camera)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);
    struct d3drm_frame *frame = unsafe_impl_from_IDirect3DRMFrame3(camera);

    TRACE("iface %p, camera %p.\n", iface, camera);

    if (!camera || !viewport->camera)
        return D3DRMERR_BADOBJECT;

    IDirect3DRMFrame_AddRef(&frame->IDirect3DRMFrame_iface);
    IDirect3DRMFrame_Release(viewport->camera);
    viewport->camera = &frame->IDirect3DRMFrame_iface;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_SetCamera(IDirect3DRMViewport *iface, IDirect3DRMFrame *camera)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);
    struct d3drm_frame *frame = unsafe_impl_from_IDirect3DRMFrame(camera);

    TRACE("iface %p, camera %p.\n", iface, camera);

    return d3drm_viewport2_SetCamera(&viewport->IDirect3DRMViewport2_iface,
            frame ? &frame->IDirect3DRMFrame3_iface : NULL);
}

static HRESULT WINAPI d3drm_viewport2_SetProjection(IDirect3DRMViewport2 *iface, D3DRMPROJECTIONTYPE type)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, type %#x.\n", iface, type);

    if (!viewport->d3d_viewport)
        return D3DRMERR_BADOBJECT;

    viewport->projection = type;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_SetProjection(IDirect3DRMViewport *iface, D3DRMPROJECTIONTYPE type)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, type %#x.\n", iface, type);

    return d3drm_viewport2_SetProjection(&viewport->IDirect3DRMViewport2_iface, type);
}

static HRESULT WINAPI d3drm_viewport2_Transform(IDirect3DRMViewport2 *iface, D3DRMVECTOR4D *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_Transform(IDirect3DRMViewport *iface, D3DRMVECTOR4D *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_InverseTransform(IDirect3DRMViewport2 *iface, D3DVECTOR *d, D3DRMVECTOR4D *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_InverseTransform(IDirect3DRMViewport *iface, D3DVECTOR *d, D3DRMVECTOR4D *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_Configure(IDirect3DRMViewport2 *iface,
        LONG x, LONG y, DWORD width, DWORD height)
{
    FIXME("iface %p, x %ld, y %ld, width %lu, height %lu stub!\n", iface, x, y, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_Configure(IDirect3DRMViewport *iface,
        LONG x, LONG y, DWORD width, DWORD height)
{
    FIXME("iface %p, x %ld, y %ld, width %lu, height %lu stub!\n", iface, x, y, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_ForceUpdate(IDirect3DRMViewport2* iface,
        DWORD x1, DWORD y1, DWORD x2, DWORD y2)
{
    FIXME("iface %p, x1 %lu, y1 %lu, x2 %lu, y2 %lu stub!\n", iface, x1, y1, x2, y2);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_ForceUpdate(IDirect3DRMViewport *iface,
        DWORD x1, DWORD y1, DWORD x2, DWORD y2)
{
    FIXME("iface %p, x1 %lu, y1 %lu, x2 %lu, y2 %lu stub!\n", iface, x1, y1, x2, y2);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_SetPlane(IDirect3DRMViewport2 *iface,
        D3DVALUE left, D3DVALUE right, D3DVALUE bottom, D3DVALUE top)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, left %.8e, right %.8e, bottom %.8e, top %.8e.\n",
            iface, left, right, bottom, top);

    if (!viewport->d3d_viewport)
        return D3DRMERR_BADOBJECT;

    viewport->clip.left = left;
    viewport->clip.right = right;
    viewport->clip.bottom = bottom;
    viewport->clip.top = top;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_SetPlane(IDirect3DRMViewport *iface,
        D3DVALUE left, D3DVALUE right, D3DVALUE bottom, D3DVALUE top)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, left %.8e, right %.8e, bottom %.8e, top %.8e.\n",
            iface, left, right, bottom, top);

    return d3drm_viewport2_SetPlane(&viewport->IDirect3DRMViewport2_iface, left, right, bottom, top);
}

static HRESULT WINAPI d3drm_viewport2_GetCamera(IDirect3DRMViewport2 *iface, IDirect3DRMFrame3 **camera)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, camera %p.\n", iface, camera);

    if (!camera)
        return D3DRMERR_BADVALUE;

    if (!viewport->camera)
        return D3DRMERR_BADOBJECT;

    return IDirect3DRMFrame_QueryInterface(viewport->camera, &IID_IDirect3DRMFrame3, (void **)camera);
}

static HRESULT WINAPI d3drm_viewport1_GetCamera(IDirect3DRMViewport *iface, IDirect3DRMFrame **camera)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);
    struct d3drm_frame *camera_impl;
    IDirect3DRMFrame3 *camera3;
    HRESULT hr;

    TRACE("iface %p, camera %p.\n", iface, camera);

    if (!camera)
        return D3DRMERR_BADVALUE;

    if (FAILED(hr = d3drm_viewport2_GetCamera(&viewport->IDirect3DRMViewport2_iface, &camera3)))
        return hr;

    camera_impl = unsafe_impl_from_IDirect3DRMFrame3(camera3);
    *camera = &camera_impl->IDirect3DRMFrame_iface;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_GetDevice(IDirect3DRMViewport2 *iface, IDirect3DRMDevice3 **device)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (!device)
        return D3DRMERR_BADVALUE;

    if (!viewport->device)
        return D3DRMERR_BADOBJECT;

    *device = &viewport->device->IDirect3DRMDevice3_iface;
    IDirect3DRMDevice3_AddRef(*device);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_GetDevice(IDirect3DRMViewport *iface, IDirect3DRMDevice **device)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, device %p.\n\n", iface, device);

    if (!device)
        return D3DRMERR_BADVALUE;

    if (!viewport->device)
        return D3DRMERR_BADOBJECT;

    *device = &viewport->device->IDirect3DRMDevice_iface;
    IDirect3DRMDevice_AddRef(*device);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_GetPlane(IDirect3DRMViewport2 *iface,
        D3DVALUE *left, D3DVALUE *right, D3DVALUE *bottom, D3DVALUE *top)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, left %p, right %p, bottom %p, top %p.\n",
            iface, left, right, bottom, top);

    if (!viewport->d3d_viewport)
        return D3DRMERR_BADOBJECT;

    *left = viewport->clip.left;
    *right = viewport->clip.right;
    *bottom = viewport->clip.bottom;
    *top = viewport->clip.top;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_GetPlane(IDirect3DRMViewport *iface,
        D3DVALUE *left, D3DVALUE *right, D3DVALUE *bottom, D3DVALUE *top)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, left %p, right %p, bottom %p, top %p.\n",
            iface, left, right, bottom, top);

    return d3drm_viewport2_GetPlane(&viewport->IDirect3DRMViewport2_iface, left, right, bottom, top);
}

static HRESULT WINAPI d3drm_viewport2_Pick(IDirect3DRMViewport2 *iface,
        LONG x, LONG y, IDirect3DRMPickedArray **visuals)
{
    FIXME("iface %p, x %ld, y %ld, visuals %p stub!\n", iface, x, y, visuals);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_Pick(IDirect3DRMViewport *iface,
        LONG x, LONG y, IDirect3DRMPickedArray **visuals)
{
    FIXME("iface %p, x %ld, y %ld, visuals %p stub!\n", iface, x, y, visuals);

    return E_NOTIMPL;
}

static BOOL WINAPI d3drm_viewport2_GetUniformScaling(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static BOOL WINAPI d3drm_viewport1_GetUniformScaling(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static LONG WINAPI d3drm_viewport2_GetX(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static LONG WINAPI d3drm_viewport1_GetX(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static LONG WINAPI d3drm_viewport2_GetY(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static LONG WINAPI d3drm_viewport1_GetY(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport2_GetWidth(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport1_GetWidth(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport2_GetHeight(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport1_GetHeight(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static D3DVALUE WINAPI d3drm_viewport2_GetField(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    if (!viewport->d3d_viewport)
        return -1.0f;

    return (viewport->clip.right - viewport->clip.left
            + viewport->clip.top - viewport->clip.bottom) / 4.0f;
}

static D3DVALUE WINAPI d3drm_viewport1_GetField(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport2_GetField(&viewport->IDirect3DRMViewport2_iface);
}

static D3DVALUE WINAPI d3drm_viewport2_GetBack(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    if (!viewport->d3d_viewport)
        return -1.0f;

    return viewport->clip.back;
}

static D3DVALUE WINAPI d3drm_viewport1_GetBack(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport2_GetBack(&viewport->IDirect3DRMViewport2_iface);
}

static D3DVALUE WINAPI d3drm_viewport2_GetFront(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    if (!viewport->d3d_viewport)
        return -1.0f;

    return viewport->clip.front;
}

static D3DVALUE WINAPI d3drm_viewport1_GetFront(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport2_GetFront(&viewport->IDirect3DRMViewport2_iface);
}

static D3DRMPROJECTIONTYPE WINAPI d3drm_viewport2_GetProjection(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    if (!viewport->d3d_viewport)
        return ~0u;

    return viewport->projection;
}

static D3DRMPROJECTIONTYPE WINAPI d3drm_viewport1_GetProjection(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport2_GetProjection(&viewport->IDirect3DRMViewport2_iface);
}

static HRESULT WINAPI d3drm_viewport2_GetDirect3DViewport(IDirect3DRMViewport2 *iface,
        IDirect3DViewport **viewport)
{
    struct d3drm_viewport *viewport_object = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    if (!viewport)
        return D3DRMERR_BADVALUE;

    if (!viewport_object->d3d_viewport)
        return D3DRMERR_BADOBJECT;

    *viewport = viewport_object->d3d_viewport;
    IDirect3DViewport_AddRef(*viewport);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_GetDirect3DViewport(IDirect3DRMViewport *iface,
        IDirect3DViewport **viewport)
{
    struct d3drm_viewport *viewport_object = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    return d3drm_viewport2_GetDirect3DViewport(&viewport_object->IDirect3DRMViewport2_iface, viewport);
}

static HRESULT WINAPI d3drm_viewport2_TransformVectors(IDirect3DRMViewport2 *iface,
        DWORD vector_count, D3DRMVECTOR4D *dst, D3DVECTOR *src)
{
    FIXME("iface %p, vector_count %lu, dst %p, src %p stub!\n", iface, vector_count, dst, src);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_InverseTransformVectors(IDirect3DRMViewport2 *iface,
        DWORD vector_count, D3DVECTOR *dst, D3DRMVECTOR4D *src)
{
    FIXME("iface %p, vector_count %lu, dst %p, src %p stub!\n", iface, vector_count, dst, src);

    return E_NOTIMPL;
}

static const struct IDirect3DRMViewport2Vtbl d3drm_viewport2_vtbl =
{
    d3drm_viewport2_QueryInterface,
    d3drm_viewport2_AddRef,
    d3drm_viewport2_Release,
    d3drm_viewport2_Clone,
    d3drm_viewport2_AddDestroyCallback,
    d3drm_viewport2_DeleteDestroyCallback,
    d3drm_viewport2_SetAppData,
    d3drm_viewport2_GetAppData,
    d3drm_viewport2_SetName,
    d3drm_viewport2_GetName,
    d3drm_viewport2_GetClassName,
    d3drm_viewport2_Init,
    d3drm_viewport2_Clear,
    d3drm_viewport2_Render,
    d3drm_viewport2_SetFront,
    d3drm_viewport2_SetBack,
    d3drm_viewport2_SetField,
    d3drm_viewport2_SetUniformScaling,
    d3drm_viewport2_SetCamera,
    d3drm_viewport2_SetProjection,
    d3drm_viewport2_Transform,
    d3drm_viewport2_InverseTransform,
    d3drm_viewport2_Configure,
    d3drm_viewport2_ForceUpdate,
    d3drm_viewport2_SetPlane,
    d3drm_viewport2_GetCamera,
    d3drm_viewport2_GetDevice,
    d3drm_viewport2_GetPlane,
    d3drm_viewport2_Pick,
    d3drm_viewport2_GetUniformScaling,
    d3drm_viewport2_GetX,
    d3drm_viewport2_GetY,
    d3drm_viewport2_GetWidth,
    d3drm_viewport2_GetHeight,
    d3drm_viewport2_GetField,
    d3drm_viewport2_GetBack,
    d3drm_viewport2_GetFront,
    d3drm_viewport2_GetProjection,
    d3drm_viewport2_GetDirect3DViewport,
    d3drm_viewport2_TransformVectors,
    d3drm_viewport2_InverseTransformVectors,
};

static const struct IDirect3DRMViewportVtbl d3drm_viewport1_vtbl =
{
    d3drm_viewport1_QueryInterface,
    d3drm_viewport1_AddRef,
    d3drm_viewport1_Release,
    d3drm_viewport1_Clone,
    d3drm_viewport1_AddDestroyCallback,
    d3drm_viewport1_DeleteDestroyCallback,
    d3drm_viewport1_SetAppData,
    d3drm_viewport1_GetAppData,
    d3drm_viewport1_SetName,
    d3drm_viewport1_GetName,
    d3drm_viewport1_GetClassName,
    d3drm_viewport1_Init,
    d3drm_viewport1_Clear,
    d3drm_viewport1_Render,
    d3drm_viewport1_SetFront,
    d3drm_viewport1_SetBack,
    d3drm_viewport1_SetField,
    d3drm_viewport1_SetUniformScaling,
    d3drm_viewport1_SetCamera,
    d3drm_viewport1_SetProjection,
    d3drm_viewport1_Transform,
    d3drm_viewport1_InverseTransform,
    d3drm_viewport1_Configure,
    d3drm_viewport1_ForceUpdate,
    d3drm_viewport1_SetPlane,
    d3drm_viewport1_GetCamera,
    d3drm_viewport1_GetDevice,
    d3drm_viewport1_GetPlane,
    d3drm_viewport1_Pick,
    d3drm_viewport1_GetUniformScaling,
    d3drm_viewport1_GetX,
    d3drm_viewport1_GetY,
    d3drm_viewport1_GetWidth,
    d3drm_viewport1_GetHeight,
    d3drm_viewport1_GetField,
    d3drm_viewport1_GetBack,
    d3drm_viewport1_GetFront,
    d3drm_viewport1_GetProjection,
    d3drm_viewport1_GetDirect3DViewport,
};

HRESULT d3drm_viewport_create(struct d3drm_viewport **viewport, IDirect3DRM *d3drm)
{
    static const char classname[] = "Viewport";
    struct d3drm_viewport *object;

    TRACE("viewport %p, d3drm %p.\n", viewport, d3drm);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMViewport_iface.lpVtbl = &d3drm_viewport1_vtbl;
    object->IDirect3DRMViewport2_iface.lpVtbl = &d3drm_viewport2_vtbl;
    object->d3drm = d3drm;
    d3drm_object_init(&object->obj, classname);

    *viewport = object;

    return S_OK;
}
