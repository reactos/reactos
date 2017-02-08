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

struct d3drm_viewport
{
    IDirect3DRMViewport IDirect3DRMViewport_iface;
    IDirect3DRMViewport2 IDirect3DRMViewport2_iface;
    LONG ref;
    D3DVALUE back;
    D3DVALUE front;
    D3DVALUE field;
    D3DRMPROJECTIONTYPE projection;
};

static inline struct d3drm_viewport *impl_from_IDirect3DRMViewport(IDirect3DRMViewport *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_viewport, IDirect3DRMViewport_iface);
}

static inline struct d3drm_viewport *impl_from_IDirect3DRMViewport2(IDirect3DRMViewport2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_viewport, IDirect3DRMViewport2_iface);
}

static HRESULT WINAPI d3drm_viewport1_QueryInterface(IDirect3DRMViewport *iface, REFIID riid, void **out)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMViewport)
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
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI d3drm_viewport1_AddRef(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);
    ULONG refcount = InterlockedIncrement(&viewport->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_viewport1_Release(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);
    ULONG refcount = InterlockedDecrement(&viewport->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, viewport);

    return refcount;
}

static HRESULT WINAPI d3drm_viewport1_Clone(IDirect3DRMViewport *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_AddDestroyCallback(IDirect3DRMViewport *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_DeleteDestroyCallback(IDirect3DRMViewport *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_SetAppData(IDirect3DRMViewport *iface, DWORD data)
{
    FIXME("iface %p, data %#x stub!\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport1_GetAppData(IDirect3DRMViewport *iface)
{
    FIXME("iface %p.\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_viewport1_SetName(IDirect3DRMViewport *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_GetName(IDirect3DRMViewport *iface, DWORD *size, char *name)
{
    FIXME("iface %p, size %p, name %p stub!\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_GetClassName(IDirect3DRMViewport *iface, DWORD *size, char *name)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMViewport2_GetClassName(&viewport->IDirect3DRMViewport2_iface, size, name);
}

static HRESULT WINAPI d3drm_viewport1_Init(IDirect3DRMViewport *iface, IDirect3DRMDevice *device,
        IDirect3DRMFrame *camera, DWORD x, DWORD y, DWORD width, DWORD height)
{
    FIXME("iface %p, device %p, camera %p, x %u, y %u, width %u, height %u stub!\n",
            iface, device, camera, x, y, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_Clear(IDirect3DRMViewport *iface)
{
    FIXME("iface %p.\n", iface);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_Render(IDirect3DRMViewport *iface, IDirect3DRMFrame *frame)
{
    FIXME("iface %p, frame %p stub!\n", iface, frame);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport1_SetFront(IDirect3DRMViewport *iface, D3DVALUE front)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, front %.8e.\n", iface, front);

    return IDirect3DRMViewport2_SetFront(&viewport->IDirect3DRMViewport2_iface, front);
}

static HRESULT WINAPI d3drm_viewport1_SetBack(IDirect3DRMViewport *iface, D3DVALUE back)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, back %.8e.\n", iface, back);

    return IDirect3DRMViewport2_SetBack(&viewport->IDirect3DRMViewport2_iface, back);
}

static HRESULT WINAPI d3drm_viewport1_SetField(IDirect3DRMViewport *iface, D3DVALUE field)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, field %.8e.\n", iface, field);

    return IDirect3DRMViewport2_SetField(&viewport->IDirect3DRMViewport2_iface, field);
}

static HRESULT WINAPI d3drm_viewport1_SetUniformScaling(IDirect3DRMViewport *iface, BOOL b)
{
    FIXME("iface %p, b %#x stub!\n", iface, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_SetCamera(IDirect3DRMViewport *iface, IDirect3DRMFrame *camera)
{
    FIXME("iface %p, camera %p stub!\n", iface, camera);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_SetProjection(IDirect3DRMViewport *iface, D3DRMPROJECTIONTYPE type)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p, type %#x.\n", iface, type);

    return IDirect3DRMViewport2_SetProjection(&viewport->IDirect3DRMViewport2_iface, type);
}

static HRESULT WINAPI d3drm_viewport1_Transform(IDirect3DRMViewport *iface, D3DRMVECTOR4D *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_InverseTransform(IDirect3DRMViewport *iface, D3DVECTOR *d, D3DRMVECTOR4D *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_Configure(IDirect3DRMViewport *iface,
        LONG x, LONG y, DWORD width, DWORD height)
{
    FIXME("iface %p, x %d, y %d, width %u, height %u stub!\n", iface, x, y, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_ForceUpdate(IDirect3DRMViewport *iface,
        DWORD x1, DWORD y1, DWORD x2, DWORD y2)
{
    FIXME("iface %p, x1 %u, y1 %u, x2 %u, y2 %u stub!\n", iface, x1, y1, x2, y2);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_SetPlane(IDirect3DRMViewport *iface,
        D3DVALUE left, D3DVALUE right, D3DVALUE bottom, D3DVALUE top)
{
    FIXME("iface %p, left %.8e, right %.8e, bottom %.8e, top %.8e stub!\n",
            iface, left, right, bottom, top);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_GetCamera(IDirect3DRMViewport *iface, IDirect3DRMFrame **camera)
{
    FIXME("iface %p, camera %p stub!\n", iface, camera);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_GetDevice(IDirect3DRMViewport *iface, IDirect3DRMDevice **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_GetPlane(IDirect3DRMViewport *iface,
        D3DVALUE *left, D3DVALUE *right, D3DVALUE *bottom, D3DVALUE *top)
{
    FIXME("iface %p, left %p, right %p, bottom %p, top %p stub!\n",
            iface, left, right, bottom, top);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport1_Pick(IDirect3DRMViewport *iface,
        LONG x, LONG y, IDirect3DRMPickedArray **visuals)
{
    FIXME("iface %p, x %d, y %d, visuals %p stub!\n", iface, x, y, visuals);

    return E_NOTIMPL;
}

static BOOL WINAPI d3drm_viewport1_GetUniformScaling(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static LONG WINAPI d3drm_viewport1_GetX(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static LONG WINAPI d3drm_viewport1_GetY(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport1_GetWidth(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport1_GetHeight(IDirect3DRMViewport *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static D3DVALUE WINAPI d3drm_viewport1_GetField(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMViewport2_GetField(&viewport->IDirect3DRMViewport2_iface);
}

static D3DVALUE WINAPI d3drm_viewport1_GetBack(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMViewport2_GetBack(&viewport->IDirect3DRMViewport2_iface);
}

static D3DVALUE WINAPI d3drm_viewport1_GetFront(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMViewport2_GetFront(&viewport->IDirect3DRMViewport2_iface);
}

static D3DRMPROJECTIONTYPE WINAPI d3drm_viewport1_GetProjection(IDirect3DRMViewport *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMViewport2_GetProjection(&viewport->IDirect3DRMViewport2_iface);
}

static HRESULT WINAPI d3drm_viewport1_GetDirect3DViewport(IDirect3DRMViewport *iface,
        IDirect3DViewport **viewport)
{
    FIXME("iface %p, viewport %p stub!\n", iface, viewport);

    return E_NOTIMPL;
}

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

static HRESULT WINAPI d3drm_viewport2_QueryInterface(IDirect3DRMViewport2 *iface, REFIID riid, void **out)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return d3drm_viewport1_QueryInterface(&viewport->IDirect3DRMViewport_iface, riid, out);
}

static ULONG WINAPI d3drm_viewport2_AddRef(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport1_AddRef(&viewport->IDirect3DRMViewport_iface);
}

static ULONG WINAPI d3drm_viewport2_Release(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_viewport1_Release(&viewport->IDirect3DRMViewport_iface);
}

static HRESULT WINAPI d3drm_viewport2_Clone(IDirect3DRMViewport2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_AddDestroyCallback(IDirect3DRMViewport2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_DeleteDestroyCallback(IDirect3DRMViewport2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_SetAppData(IDirect3DRMViewport2 *iface, DWORD data)
{
    FIXME("iface %p, data %#x stub!\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport2_GetAppData(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_viewport2_SetName(IDirect3DRMViewport2 *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_GetName(IDirect3DRMViewport2 *iface, DWORD *size, char *name)
{
    FIXME("iface %p, size %p, name %p stub!\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_GetClassName(IDirect3DRMViewport2 *iface, DWORD *size, char *name)
{
    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    if (!size || *size < strlen("Viewport") || !name)
        return E_INVALIDARG;

    strcpy(name, "Viewport");
    *size = sizeof("Viewport");

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_Init(IDirect3DRMViewport2 *iface, IDirect3DRMDevice3 *device,
        IDirect3DRMFrame3 *camera, DWORD x, DWORD y, DWORD width, DWORD height)
{
    FIXME("iface %p, device %p, camera %p, x %u, y %u, width %u, height %u stub!\n",
            iface, device, camera, x, y, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_Clear(IDirect3DRMViewport2 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#x.\n", iface, flags);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_Render(IDirect3DRMViewport2 *iface, IDirect3DRMFrame3 *frame)
{
    FIXME("iface %p, frame %p stub!\n", iface, frame);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_SetFront(IDirect3DRMViewport2 *iface, D3DVALUE front)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, front %.8e.\n", iface, front);

    viewport->front = front;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_SetBack(IDirect3DRMViewport2 *iface, D3DVALUE back)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, back %.8e.\n", iface, back);

    viewport->back = back;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_SetField(IDirect3DRMViewport2 *iface, D3DVALUE field)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, field %.8e.\n", iface, field);

    viewport->field = field;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_SetUniformScaling(IDirect3DRMViewport2 *iface, BOOL b)
{
    FIXME("iface %p, b %#x stub!\n", iface, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_SetCamera(IDirect3DRMViewport2 *iface, IDirect3DRMFrame3 *camera)
{
    FIXME("iface %p, camera %p stub!\n", iface, camera);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_SetProjection(IDirect3DRMViewport2 *iface, D3DRMPROJECTIONTYPE type)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p, type %#x.\n", iface, type);

    viewport->projection = type;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_viewport2_Transform(IDirect3DRMViewport2 *iface, D3DRMVECTOR4D *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_InverseTransform(IDirect3DRMViewport2 *iface, D3DVECTOR *d, D3DRMVECTOR4D *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_Configure(IDirect3DRMViewport2 *iface,
        LONG x, LONG y, DWORD width, DWORD height)
{
    FIXME("iface %p, x %d, y %d, width %u, height %u stub!\n", iface, x, y, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_ForceUpdate(IDirect3DRMViewport2* iface,
        DWORD x1, DWORD y1, DWORD x2, DWORD y2)
{
    FIXME("iface %p, x1 %u, y1 %u, x2 %u, y2 %u stub!\n", iface, x1, y1, x2, y2);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_SetPlane(IDirect3DRMViewport2 *iface,
        D3DVALUE left, D3DVALUE right, D3DVALUE bottom, D3DVALUE top)
{
    FIXME("iface %p, left %.8e, right %.8e, bottom %.8e, top %.8e stub!\n",
            iface, left, right, bottom, top);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_GetCamera(IDirect3DRMViewport2 *iface, IDirect3DRMFrame3 **camera)
{
    FIXME("iface %p, camera %p stub!\n", iface, camera);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_GetDevice(IDirect3DRMViewport2 *iface, IDirect3DRMDevice3 **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_GetPlane(IDirect3DRMViewport2 *iface,
        D3DVALUE *left, D3DVALUE *right, D3DVALUE *bottom, D3DVALUE *top)
{
    FIXME("iface %p, left %p, right %p, bottom %p, top %p stub!\n",
            iface, left, right, bottom, top);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_Pick(IDirect3DRMViewport2 *iface,
        LONG x, LONG y, IDirect3DRMPickedArray **visuals)
{
    FIXME("iface %p, x %d, y %d, visuals %p stub!\n", iface, x, y, visuals);

    return E_NOTIMPL;
}

static BOOL WINAPI d3drm_viewport2_GetUniformScaling(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static LONG WINAPI d3drm_viewport2_GetX(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static LONG WINAPI d3drm_viewport2_GetY(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport2_GetWidth(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_viewport2_GetHeight(IDirect3DRMViewport2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static D3DVALUE WINAPI d3drm_viewport2_GetField(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    return viewport->field;
}

static D3DVALUE WINAPI d3drm_viewport2_GetBack(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    return viewport->back;
}

static D3DVALUE WINAPI d3drm_viewport2_GetFront(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    return viewport->front;
}

static D3DRMPROJECTIONTYPE WINAPI d3drm_viewport2_GetProjection(IDirect3DRMViewport2 *iface)
{
    struct d3drm_viewport *viewport = impl_from_IDirect3DRMViewport2(iface);

    TRACE("iface %p.\n", iface);

    return viewport->projection;
}

static HRESULT WINAPI d3drm_viewport2_GetDirect3DViewport(IDirect3DRMViewport2 *iface,
        IDirect3DViewport **viewport)
{
    FIXME("iface %p, viewport %p stub!\n", iface, viewport);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_TransformVectors(IDirect3DRMViewport2 *iface,
        DWORD vector_count, D3DRMVECTOR4D *dst, D3DVECTOR *src)
{
    FIXME("iface %p, vector_count %u, dst %p, src %p stub!\n", iface, vector_count, dst, src);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_viewport2_InverseTransformVectors(IDirect3DRMViewport2 *iface,
        DWORD vector_count, D3DVECTOR *dst, D3DRMVECTOR4D *src)
{
    FIXME("iface %p, vector_count %u, dst %p, src %p stub!\n", iface, vector_count, dst, src);

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

HRESULT Direct3DRMViewport_create(REFIID riid, IUnknown **out)
{
    struct d3drm_viewport *object;

    TRACE("riid %s, out %p.\n", debugstr_guid(riid), out);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMViewport_iface.lpVtbl = &d3drm_viewport1_vtbl;
    object->IDirect3DRMViewport2_iface.lpVtbl = &d3drm_viewport2_vtbl;
    object->ref = 1;

    if (IsEqualGUID(riid, &IID_IDirect3DRMViewport2))
        *out = (IUnknown *)&object->IDirect3DRMViewport2_iface;
    else
        *out = (IUnknown *)&object->IDirect3DRMViewport_iface;

    return S_OK;
}
