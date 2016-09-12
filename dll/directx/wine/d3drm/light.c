/*
 * Implementation of IDirect3DRMLight Interface
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

struct d3drm_light
{
    IDirect3DRMLight IDirect3DRMLight_iface;
    LONG ref;
    D3DRMLIGHTTYPE type;
    D3DCOLOR color;
    D3DVALUE range;
    D3DVALUE cattenuation;
    D3DVALUE lattenuation;
    D3DVALUE qattenuation;
    D3DVALUE umbra;
    D3DVALUE penumbra;
};

static inline struct d3drm_light *impl_from_IDirect3DRMLight(IDirect3DRMLight *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_light, IDirect3DRMLight_iface);
}

static HRESULT WINAPI d3drm_light_QueryInterface(IDirect3DRMLight *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMLight)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DRMLight_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3drm_light_AddRef(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);
    ULONG refcount = InterlockedIncrement(&light->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_light_Release(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);
    ULONG refcount = InterlockedDecrement(&light->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, light);

    return refcount;
}

static HRESULT WINAPI d3drm_light_Clone(IDirect3DRMLight *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_light_AddDestroyCallback(IDirect3DRMLight *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_light_DeleteDestroyCallback(IDirect3DRMLight *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_light_SetAppData(IDirect3DRMLight *iface, DWORD data)
{
    FIXME("iface %p, data %#x stub!\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_light_GetAppData(IDirect3DRMLight *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_light_SetName(IDirect3DRMLight *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_light_GetName(IDirect3DRMLight *iface, DWORD *size, char *name)
{
    FIXME("iface %p, size %p, name %p stub!\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_light_GetClassName(IDirect3DRMLight *iface, DWORD *size, char *name)
{
    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    if (!size || *size < strlen("Light") || !name)
        return E_INVALIDARG;

    strcpy(name, "Light");
    *size = sizeof("Light");

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_light_SetType(IDirect3DRMLight *iface, D3DRMLIGHTTYPE type)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p, type %#x.\n", iface, type);

    light->type = type;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_light_SetColor(IDirect3DRMLight *iface, D3DCOLOR color)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p, color 0x%08x.\n", iface, color);

    light->color = color;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_light_SetColorRGB(IDirect3DRMLight *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e.\n", iface, red, green, blue);

    light->color = RGBA_MAKE((BYTE)(red * 255.0f), (BYTE)(green * 255.0f), (BYTE)(blue * 255.0f), 0xff);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_light_SetRange(IDirect3DRMLight *iface, D3DVALUE range)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p, range %.8e.\n", iface, range);

    light->range = range;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_light_SetUmbra(IDirect3DRMLight *iface, D3DVALUE umbra)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p, umbra %.8e.\n", iface, umbra);

    light->umbra = umbra;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_light_SetPenumbra(IDirect3DRMLight *iface, D3DVALUE penumbra)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p, penumbra %.8e.\n", iface, penumbra);

    light->penumbra = penumbra;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_light_SetConstantAttenuation(IDirect3DRMLight *iface, D3DVALUE attenuation)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p, attenuation %.8e.\n", iface, attenuation);

    light->cattenuation = attenuation;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_light_SetLinearAttenuation(IDirect3DRMLight *iface, D3DVALUE attenuation)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p, attenuation %.8e.\n", iface, attenuation);

    light->lattenuation = attenuation;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_light_SetQuadraticAttenuation(IDirect3DRMLight *iface, D3DVALUE attenuation)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p, attenuation %.8e.\n", iface, attenuation);

    light->qattenuation = attenuation;

    return D3DRM_OK;
}

static D3DVALUE WINAPI d3drm_light_GetRange(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p.\n", iface);

    return light->range;
}

static D3DVALUE WINAPI d3drm_light_GetUmbra(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p.\n", light);

    return light->umbra;
}

static D3DVALUE WINAPI d3drm_light_GetPenumbra(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p.\n", iface);

    return light->penumbra;
}

static D3DVALUE WINAPI d3drm_light_GetConstantAttenuation(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p.\n", iface);

    return light->cattenuation;
}

static D3DVALUE WINAPI d3drm_light_GetLinearAttenuation(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p.\n", iface);

    return light->lattenuation;
}

static D3DVALUE WINAPI d3drm_light_GetQuadraticAttenuation(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p.\n", iface);

    return light->qattenuation;
}

static D3DCOLOR WINAPI d3drm_light_GetColor(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p.\n", iface);

    return light->color;
}

static D3DRMLIGHTTYPE WINAPI d3drm_light_GetType(IDirect3DRMLight *iface)
{
    struct d3drm_light *light = impl_from_IDirect3DRMLight(iface);

    TRACE("iface %p.\n", iface);

    return light->type;
}

static HRESULT WINAPI d3drm_light_SetEnableFrame(IDirect3DRMLight *iface, IDirect3DRMFrame *frame)
{
    FIXME("iface %p, frame %p stub!\n", iface, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_light_GetEnableFrame(IDirect3DRMLight *iface, IDirect3DRMFrame **frame)
{
    FIXME("iface %p, frame %p stub!\n", iface, frame);

    return E_NOTIMPL;
}

static const struct IDirect3DRMLightVtbl d3drm_light_vtbl =
{
    d3drm_light_QueryInterface,
    d3drm_light_AddRef,
    d3drm_light_Release,
    d3drm_light_Clone,
    d3drm_light_AddDestroyCallback,
    d3drm_light_DeleteDestroyCallback,
    d3drm_light_SetAppData,
    d3drm_light_GetAppData,
    d3drm_light_SetName,
    d3drm_light_GetName,
    d3drm_light_GetClassName,
    d3drm_light_SetType,
    d3drm_light_SetColor,
    d3drm_light_SetColorRGB,
    d3drm_light_SetRange,
    d3drm_light_SetUmbra,
    d3drm_light_SetPenumbra,
    d3drm_light_SetConstantAttenuation,
    d3drm_light_SetLinearAttenuation,
    d3drm_light_SetQuadraticAttenuation,
    d3drm_light_GetRange,
    d3drm_light_GetUmbra,
    d3drm_light_GetPenumbra,
    d3drm_light_GetConstantAttenuation,
    d3drm_light_GetLinearAttenuation,
    d3drm_light_GetQuadraticAttenuation,
    d3drm_light_GetColor,
    d3drm_light_GetType,
    d3drm_light_SetEnableFrame,
    d3drm_light_GetEnableFrame,
};

HRESULT Direct3DRMLight_create(IUnknown **out)
{
    struct d3drm_light *object;

    TRACE("out %p.\n", out);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMLight_iface.lpVtbl = &d3drm_light_vtbl;
    object->ref = 1;

    *out = (IUnknown *)&object->IDirect3DRMLight_iface;

    return S_OK;
}
