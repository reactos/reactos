/*
 * Implementation of IDirect3DRMMaterial2 interface
 *
 * Copyright 2012 Christian Costa
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

struct color_rgb
{
    D3DVALUE r;
    D3DVALUE g;
    D3DVALUE b;
};

struct d3drm_material
{
    IDirect3DRMMaterial2 IDirect3DRMMaterial2_iface;
    LONG ref;
    struct color_rgb emissive;
    struct color_rgb specular;
    D3DVALUE power;
    struct color_rgb ambient;
};

static inline struct d3drm_material *impl_from_IDirect3DRMMaterial2(IDirect3DRMMaterial2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_material, IDirect3DRMMaterial2_iface);
}

static HRESULT WINAPI d3drm_material_QueryInterface(IDirect3DRMMaterial2 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMMaterial2)
            || IsEqualGUID(riid, &IID_IDirect3DRMMaterial)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DRMMaterial2_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3drm_material_AddRef(IDirect3DRMMaterial2 *iface)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);
    ULONG refcount = InterlockedIncrement(&material->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_material_Release(IDirect3DRMMaterial2 *iface)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);
    ULONG refcount = InterlockedDecrement(&material->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, material);

    return refcount;
}

static HRESULT WINAPI d3drm_material_Clone(IDirect3DRMMaterial2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_material_AddDestroyCallback(IDirect3DRMMaterial2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_material_DeleteDestroyCallback(IDirect3DRMMaterial2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_material_SetAppData(IDirect3DRMMaterial2 *iface, DWORD data)
{
    FIXME("iface %p, data %#x stub!\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_material_GetAppData(IDirect3DRMMaterial2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_material_SetName(IDirect3DRMMaterial2 *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_material_GetName(IDirect3DRMMaterial2 *iface, DWORD *size, char *name)
{
    FIXME("iface %p, size %p, name %p stub!\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_material_GetClassName(IDirect3DRMMaterial2 *iface, DWORD *size, char *name)
{
    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    if (!size || *size < strlen("Material") || !name)
        return E_INVALIDARG;

    strcpy(name, "Material");
    *size = sizeof("Material");

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_material_SetPower(IDirect3DRMMaterial2 *iface, D3DVALUE power)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("iface %p, power %.8e.\n", iface, power);

    material->power = power;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_material_SetSpecular(IDirect3DRMMaterial2 *iface,
        D3DVALUE r, D3DVALUE g, D3DVALUE b)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("iface %p, r %.8e, g %.8e, b %.8e.\n", iface, r, g, b);

    material->specular.r = r;
    material->specular.g = g;
    material->specular.b = b;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_material_SetEmissive(IDirect3DRMMaterial2 *iface,
        D3DVALUE r, D3DVALUE g, D3DVALUE b)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("iface %p, r %.8e, g %.8e, b %.8e.\n", iface, r, g, b);

    material->emissive.r = r;
    material->emissive.g = g;
    material->emissive.b = b;

    return D3DRM_OK;
}

static D3DVALUE WINAPI d3drm_material_GetPower(IDirect3DRMMaterial2 *iface)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("iface %p.\n", iface);

    return material->power;
}

static HRESULT WINAPI d3drm_material_GetSpecular(IDirect3DRMMaterial2 *iface,
        D3DVALUE *r, D3DVALUE *g, D3DVALUE *b)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("iface %p, r %p, g %p, b %p.\n", iface, r, g, b);

    *r = material->specular.r;
    *g = material->specular.g;
    *b = material->specular.b;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_material_GetEmissive(IDirect3DRMMaterial2 *iface,
        D3DVALUE *r, D3DVALUE *g, D3DVALUE *b)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("iface %p, r %p, g %p, b %p.\n", iface, r, g, b);

    *r = material->emissive.r;
    *g = material->emissive.g;
    *b = material->emissive.b;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_material_GetAmbient(IDirect3DRMMaterial2 *iface,
        D3DVALUE *r, D3DVALUE *g, D3DVALUE *b)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("iface %p, r %p, g %p, b %p.\n", iface, r, g, b);

    *r = material->ambient.r;
    *g = material->ambient.g;
    *b = material->ambient.b;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_material_SetAmbient(IDirect3DRMMaterial2 *iface,
        D3DVALUE r, D3DVALUE g, D3DVALUE b)
{
    struct d3drm_material *material = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("iface %p, r %.8e, g %.8e, b %.8e.\n", iface, r, g, b);

    material->ambient.r = r;
    material->ambient.g = g;
    material->ambient.b = b;

    return D3DRM_OK;
}

static const struct IDirect3DRMMaterial2Vtbl d3drm_material_vtbl =
{
    d3drm_material_QueryInterface,
    d3drm_material_AddRef,
    d3drm_material_Release,
    d3drm_material_Clone,
    d3drm_material_AddDestroyCallback,
    d3drm_material_DeleteDestroyCallback,
    d3drm_material_SetAppData,
    d3drm_material_GetAppData,
    d3drm_material_SetName,
    d3drm_material_GetName,
    d3drm_material_GetClassName,
    d3drm_material_SetPower,
    d3drm_material_SetSpecular,
    d3drm_material_SetEmissive,
    d3drm_material_GetPower,
    d3drm_material_GetSpecular,
    d3drm_material_GetEmissive,
    d3drm_material_GetAmbient,
    d3drm_material_SetAmbient,
};

HRESULT Direct3DRMMaterial_create(IDirect3DRMMaterial2 **out)
{
    struct d3drm_material *object;

    TRACE("out %p.\n", out);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMMaterial2_iface.lpVtbl = &d3drm_material_vtbl;
    object->ref = 1;

    object->specular.r = 1.0f;
    object->specular.g = 1.0f;
    object->specular.b = 1.0f;

    *out = &object->IDirect3DRMMaterial2_iface;

    return S_OK;
}
