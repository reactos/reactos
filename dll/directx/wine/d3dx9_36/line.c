/*
 * Copyright 2010 Christian Costa
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

#include "wine/debug.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

typedef struct ID3DXLineImpl {
    ID3DXLine ID3DXLine_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    IDirect3DStateBlock9 *state;
} ID3DXLineImpl;

static inline ID3DXLineImpl *impl_from_ID3DXLine(ID3DXLine *iface)
{
    return CONTAINING_RECORD(iface, ID3DXLineImpl, ID3DXLine_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXLineImpl_QueryInterface(ID3DXLine* iface, REFIID riid, LPVOID* object)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXLine))
    {
        ID3DXLine_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    ERR("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXLineImpl_AddRef(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    TRACE("(%p)->(): AddRef from %u\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXLineImpl_Release(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %u\n", This, ref + 1);

    if (!ref)
    {
        IDirect3DDevice9_Release(This->device);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXLine methods ***/
static HRESULT WINAPI ID3DXLineImpl_GetDevice(struct ID3DXLine *iface, struct IDirect3DDevice9 **device)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    TRACE ("(%p)->(%p)\n", This, device);

    if (device == NULL) return D3DERR_INVALIDCALL;

    *device = This->device;
    IDirect3DDevice9_AddRef(This->device);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXLineImpl_Begin(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    HRESULT hr;
    D3DXMATRIX identity, projection;
    D3DVIEWPORT9 vp;

    TRACE ("(%p)->()\n", This);

    if (This->state != NULL) /* We already began. Return error. */
        return D3DERR_INVALIDCALL;

    hr = IDirect3DDevice9_CreateStateBlock(This->device, D3DSBT_ALL, &This->state);
    if (FAILED(hr)) return D3DXERR_INVALIDDATA;

    hr = IDirect3DDevice9_GetViewport(This->device, &vp);
    if (FAILED(hr)) goto failed;

    D3DXMatrixIdentity(&identity);
    D3DXMatrixOrthoOffCenterLH(&projection,
                               0.0, vp.Width, /* Min and max x */
                               vp.Height, 0.0, /* Min and max y. Screen y is on top so this is inverted */
                               0.0, 1.0); /* Min and max z */

    hr = IDirect3DDevice9_SetTransform(This->device, D3DTS_WORLD, &identity);
    if (FAILED(hr)) goto failed;

    hr = IDirect3DDevice9_SetTransform(This->device, D3DTS_VIEW, &identity);
    if (FAILED(hr)) goto failed;

    hr = IDirect3DDevice9_SetTransform(This->device, D3DTS_PROJECTION, &projection);
    if (FAILED(hr)) goto failed;

    /* Windows sets similar states so we do the same */

    hr = IDirect3DDevice9_SetRenderState(This->device, D3DRS_LIGHTING, FALSE);
    if (FAILED(hr)) goto failed;

    hr = IDirect3DDevice9_SetRenderState(This->device, D3DRS_FOGENABLE, FALSE);
    if (FAILED(hr)) goto failed;

    hr = IDirect3DDevice9_SetRenderState(This->device, D3DRS_SHADEMODE, D3DSHADE_FLAT);
    if (FAILED(hr)) goto failed;

    hr = IDirect3DDevice9_SetRenderState(This->device, D3DRS_ALPHABLENDENABLE, TRUE);
    if (FAILED(hr)) goto failed;

    hr = IDirect3DDevice9_SetRenderState(This->device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    if (FAILED(hr)) goto failed;

    hr = IDirect3DDevice9_SetRenderState(This->device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    if (FAILED(hr)) goto failed;

    return D3D_OK;
failed:
    IDirect3DStateBlock9_Apply(This->state);
    IDirect3DStateBlock9_Release(This->state);
    This->state = NULL;
    return D3DXERR_INVALIDDATA;
}

static HRESULT WINAPI ID3DXLineImpl_Draw(ID3DXLine* iface, CONST D3DXVECTOR2* vertexlist, DWORD vertexlistcount, D3DCOLOR color)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(%p, %u, %#x): stub\n", This, vertexlist, vertexlistcount, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXLineImpl_DrawTransform(ID3DXLine* iface, CONST D3DXVECTOR3* vertexlist, DWORD vertexlistcount,
                                                  CONST D3DXMATRIX* transform, D3DCOLOR color)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(%p, %u, %p, %#x): stub\n", This, vertexlist, vertexlistcount, transform, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXLineImpl_SetPattern(ID3DXLine* iface, DWORD pattern)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(%#x): stub\n", This, pattern);

    return E_NOTIMPL;
}

static DWORD WINAPI ID3DXLineImpl_GetPattern(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(): stub\n", This);

    return 0xFFFFFFFF;
}

static HRESULT WINAPI ID3DXLineImpl_SetPatternScale(ID3DXLine* iface, FLOAT scale)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(%f): stub\n", This, scale);

    return E_NOTIMPL;
}

static FLOAT WINAPI ID3DXLineImpl_GetPatternScale(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(): stub\n", This);

    return 1.0f;
}

static HRESULT WINAPI ID3DXLineImpl_SetWidth(ID3DXLine* iface, FLOAT width)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(%f): stub\n", This, width);

    return E_NOTIMPL;
}

static FLOAT WINAPI ID3DXLineImpl_GetWidth(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(): stub\n", This);

    return 1.0f;
}

static HRESULT WINAPI ID3DXLineImpl_SetAntialias(ID3DXLine* iface, BOOL antialias)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(%u): stub\n", This, antialias);

    return E_NOTIMPL;
}

static BOOL WINAPI ID3DXLineImpl_GetAntialias(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(): stub\n", This);

    return FALSE;
}

static HRESULT WINAPI ID3DXLineImpl_SetGLLines(ID3DXLine* iface, BOOL gl_lines)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(%u): stub\n", This, gl_lines);

    return E_NOTIMPL;
}

static BOOL WINAPI ID3DXLineImpl_GetGLLines(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(): stub\n", This);

    return FALSE;
}

static HRESULT WINAPI ID3DXLineImpl_End(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    HRESULT hr;

    TRACE ("(%p)->()\n", This);

    if (This->state == NULL) /* We haven't begun yet. */
        return D3DERR_INVALIDCALL;

    hr = IDirect3DStateBlock9_Apply(This->state);
    IDirect3DStateBlock9_Release(This->state);
    This->state = NULL;

    if (FAILED(hr)) return D3DXERR_INVALIDDATA;
    else return D3D_OK;
}

static HRESULT WINAPI ID3DXLineImpl_OnLostDevice(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}
static HRESULT WINAPI ID3DXLineImpl_OnResetDevice(ID3DXLine* iface)
{
    ID3DXLineImpl *This = impl_from_ID3DXLine(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static const struct ID3DXLineVtbl ID3DXLine_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXLineImpl_QueryInterface,
    ID3DXLineImpl_AddRef,
    ID3DXLineImpl_Release,
    /*** ID3DXLine methods ***/
    ID3DXLineImpl_GetDevice,
    ID3DXLineImpl_Begin,
    ID3DXLineImpl_Draw,
    ID3DXLineImpl_DrawTransform,
    ID3DXLineImpl_SetPattern,
    ID3DXLineImpl_GetPattern,
    ID3DXLineImpl_SetPatternScale,
    ID3DXLineImpl_GetPatternScale,
    ID3DXLineImpl_SetWidth,
    ID3DXLineImpl_GetWidth,
    ID3DXLineImpl_SetAntialias,
    ID3DXLineImpl_GetAntialias,
    ID3DXLineImpl_SetGLLines,
    ID3DXLineImpl_GetGLLines,
    ID3DXLineImpl_End,
    ID3DXLineImpl_OnLostDevice,
    ID3DXLineImpl_OnResetDevice
};

HRESULT WINAPI D3DXCreateLine(struct IDirect3DDevice9 *device, struct ID3DXLine **line)
{
    ID3DXLineImpl* object;

    TRACE("(%p, %p)\n", device, line);

    if (!device || !line)
        return D3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXLineImpl));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXLine_iface.lpVtbl = &ID3DXLine_Vtbl;
    object->ref = 1;
    object->device = device;
    object->state = NULL; /* We only initialize it on Begin */
    IDirect3DDevice9_AddRef(device);

    *line = &object->ID3DXLine_iface;

    return D3D_OK;
}
