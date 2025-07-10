/*
 * Copyright 2021 Nikolay Sivov for CodeWeavers
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
 *
 */

#define COBJMACROS
#include "d3dx10.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

#define D3DERR_INVALIDCALL 0x8876086c

struct d3dx10_sprite
{
    ID3DX10Sprite ID3DX10Sprite_iface;
    LONG refcount;

    D3DXMATRIX projection;
    ID3D10Device *device;
};

static inline struct d3dx10_sprite *impl_from_ID3DX10Sprite(ID3DX10Sprite *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx10_sprite, ID3DX10Sprite_iface);
}

static HRESULT WINAPI d3dx10_sprite_QueryInterface(ID3DX10Sprite *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DX10Sprite)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx10_sprite_AddRef(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);
    ULONG refcount = InterlockedIncrement(&sprite->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3dx10_sprite_Release(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);
    ULONG refcount = InterlockedDecrement(&sprite->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID3D10Device_Release(sprite->device);
        free(sprite);
    }

    return refcount;
}

static HRESULT WINAPI d3dx10_sprite_Begin(ID3DX10Sprite *iface, UINT flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_DrawSpritesBuffered(ID3DX10Sprite *iface,
        D3DX10_SPRITE *sprites, UINT count)
{
    FIXME("iface %p, sprites %p, count %u stub!\n", iface, sprites, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_Flush(ID3DX10Sprite *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_DrawSpritesImmediate(ID3DX10Sprite *iface,
        D3DX10_SPRITE *sprites, UINT count, UINT size, UINT flags)
{
    FIXME("iface %p, sprites %p, count %u, size %u, flags %#x stub!\n",
            iface, sprites, count, size, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_End(ID3DX10Sprite *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_GetViewTransform(ID3DX10Sprite *iface, D3DXMATRIX *transform)
{
    FIXME("iface %p, transform %p stub!\n", iface, transform);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_SetViewTransform(ID3DX10Sprite *iface, D3DXMATRIX *transform)
{
    FIXME("iface %p, transform %p stub!\n", iface, transform);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx10_sprite_GetProjectionTransform(ID3DX10Sprite *iface,
        D3DXMATRIX *transform)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    if (!transform)
        return E_FAIL;

    *transform = sprite->projection;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_SetProjectionTransform(ID3DX10Sprite *iface, D3DXMATRIX *transform)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    if (!transform)
        return E_FAIL;

    sprite->projection = *transform;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_GetDevice(ID3DX10Sprite *iface, ID3D10Device **device)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (!device)
        return E_FAIL;

    *device = sprite->device;
    ID3D10Device_AddRef(*device);

    return S_OK;
}

static const ID3DX10SpriteVtbl d3dx10_sprite_vtbl =
{
    d3dx10_sprite_QueryInterface,
    d3dx10_sprite_AddRef,
    d3dx10_sprite_Release,
    d3dx10_sprite_Begin,
    d3dx10_sprite_DrawSpritesBuffered,
    d3dx10_sprite_Flush,
    d3dx10_sprite_DrawSpritesImmediate,
    d3dx10_sprite_End,
    d3dx10_sprite_GetViewTransform,
    d3dx10_sprite_SetViewTransform,
    d3dx10_sprite_GetProjectionTransform,
    d3dx10_sprite_SetProjectionTransform,
    d3dx10_sprite_GetDevice,
};

HRESULT WINAPI D3DX10CreateSprite(ID3D10Device *device, UINT size, ID3DX10Sprite **sprite)
{
    struct d3dx10_sprite *object;

    TRACE("device %p, size %u, sprite %p.\n", device, size, sprite);

    if (!device || !sprite)
        return D3DERR_INVALIDCALL;

    *sprite = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID3DX10Sprite_iface.lpVtbl = &d3dx10_sprite_vtbl;
    object->refcount = 1;
    object->device = device;
    ID3D10Device_AddRef(device);
    object->projection._11 = 1.0f;
    object->projection._22 = 1.0f;
    object->projection._33 = 1.0f;
    object->projection._44 = 1.0f;

    *sprite = &object->ID3DX10Sprite_iface;

    return S_OK;
}
