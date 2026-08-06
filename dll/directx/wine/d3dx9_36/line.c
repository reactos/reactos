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


#include "d3dx9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct d3dx9_line
{
    ID3DXLine ID3DXLine_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    IDirect3DStateBlock9 *state;
    float width;
};

static inline struct d3dx9_line *impl_from_ID3DXLine(ID3DXLine *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_line, ID3DXLine_iface);
}

static HRESULT WINAPI d3dx9_line_QueryInterface(ID3DXLine *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXLine)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3DXLine_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_line_AddRef(ID3DXLine *iface)
{
    struct d3dx9_line *line = impl_from_ID3DXLine(iface);
    ULONG refcount = InterlockedIncrement(&line->ref);

    TRACE("%p increasing refcount to %lu.\n", line, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_line_Release(ID3DXLine *iface)
{
    struct d3dx9_line *line = impl_from_ID3DXLine(iface);
    ULONG refcount = InterlockedDecrement(&line->ref);

    TRACE("%p decreasing refcount to %lu.\n", line, refcount);

    if (!refcount)
    {
        IDirect3DDevice9_Release(line->device);
        free(line);
    }

    return refcount;
}

static HRESULT WINAPI d3dx9_line_GetDevice(struct ID3DXLine *iface, struct IDirect3DDevice9 **device)
{
    struct d3dx9_line *line = impl_from_ID3DXLine(iface);

    TRACE("iface %p, device %p.\n", iface, line);

    if (!device)
        return D3DERR_INVALIDCALL;

    *device = line->device;
    IDirect3DDevice9_AddRef(line->device);

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_line_Begin(ID3DXLine *iface)
{
    struct d3dx9_line *line = impl_from_ID3DXLine(iface);
    D3DXMATRIX identity, projection;
    D3DVIEWPORT9 vp;

    TRACE("iface %p.\n", iface);

    if (line->state)
        return D3DERR_INVALIDCALL;

    if (FAILED(IDirect3DDevice9_CreateStateBlock(line->device, D3DSBT_ALL, &line->state)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(IDirect3DDevice9_GetViewport(line->device, &vp)))
        goto failed;

    D3DXMatrixIdentity(&identity);
    D3DXMatrixOrthoOffCenterLH(&projection, 0.0, (FLOAT)vp.Width, (FLOAT)vp.Height, 0.0, 0.0, 1.0);

    if (FAILED(IDirect3DDevice9_SetTransform(line->device, D3DTS_WORLD, &identity)))
        goto failed;
    if (FAILED(IDirect3DDevice9_SetTransform(line->device, D3DTS_VIEW, &identity)))
        goto failed;
    if (FAILED(IDirect3DDevice9_SetTransform(line->device, D3DTS_PROJECTION, &projection)))
        goto failed;

    if (FAILED(IDirect3DDevice9_SetRenderState(line->device, D3DRS_LIGHTING, FALSE)))
        goto failed;
    if (FAILED(IDirect3DDevice9_SetRenderState(line->device, D3DRS_FOGENABLE, FALSE)))
        goto failed;
    if (FAILED(IDirect3DDevice9_SetRenderState(line->device, D3DRS_SHADEMODE, D3DSHADE_FLAT)))
        goto failed;
    if (FAILED(IDirect3DDevice9_SetRenderState(line->device, D3DRS_ALPHABLENDENABLE, TRUE)))
        goto failed;
    if (FAILED(IDirect3DDevice9_SetRenderState(line->device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA)))
        goto failed;
    if (FAILED(IDirect3DDevice9_SetRenderState(line->device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA)))
        goto failed;

    return D3D_OK;

failed:
    IDirect3DStateBlock9_Apply(line->state);
    IDirect3DStateBlock9_Release(line->state);
    line->state = NULL;
    return D3DXERR_INVALIDDATA;
}

static HRESULT WINAPI d3dx9_line_Draw(ID3DXLine *iface, const D3DXVECTOR2 *vertex_list,
        DWORD vertex_list_count, D3DCOLOR color)
{
    FIXME("iface %p, vertex_list %p, vertex_list_count %lu, color 0x%08lx stub!\n",
            iface, vertex_list, vertex_list_count, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_line_DrawTransform(ID3DXLine *iface, const D3DXVECTOR3 *vertex_list,
        DWORD vertex_list_count, const D3DXMATRIX *transform, D3DCOLOR color)
{
    FIXME("iface %p, vertex_list %p, vertex_list_count %lu, transform %p, color 0x%08lx stub!\n",
            iface, vertex_list, vertex_list_count, transform, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_line_SetPattern(ID3DXLine *iface, DWORD pattern)
{
    FIXME("iface %p, pattern 0x%08lx stub!\n", iface, pattern);

    return E_NOTIMPL;
}

static DWORD WINAPI d3dx9_line_GetPattern(ID3DXLine *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0xffffffff;
}

static HRESULT WINAPI d3dx9_line_SetPatternScale(ID3DXLine *iface, float scale)
{
    FIXME("iface %p, scale %.8e stub!\n", iface, scale);

    return E_NOTIMPL;
}

static float WINAPI d3dx9_line_GetPatternScale(ID3DXLine *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 1.0f;
}

static HRESULT WINAPI d3dx9_line_SetWidth(ID3DXLine *iface, float width)
{
    struct d3dx9_line *line = impl_from_ID3DXLine(iface);

    TRACE("iface %p, width %.8e.\n", iface, width);

    if (width <= 0.0f)
        return D3DERR_INVALIDCALL;

    line->width = width;

    return D3D_OK;
}

static float WINAPI d3dx9_line_GetWidth(ID3DXLine *iface)
{
    struct d3dx9_line *line = impl_from_ID3DXLine(iface);

    TRACE("iface %p.\n", iface);

    return line->width;
}

static HRESULT WINAPI d3dx9_line_SetAntialias(ID3DXLine *iface, BOOL antialias)
{
    FIXME("iface %p, antialias %#x stub!\n", iface, antialias);

    return E_NOTIMPL;
}

static BOOL WINAPI d3dx9_line_GetAntialias(ID3DXLine *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT WINAPI d3dx9_line_SetGLLines(ID3DXLine *iface, BOOL gl_lines)
{
    FIXME("iface %p, gl_lines %#x stub!\n", iface, gl_lines);

    return E_NOTIMPL;
}

static BOOL WINAPI d3dx9_line_GetGLLines(ID3DXLine *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT WINAPI d3dx9_line_End(ID3DXLine *iface)
{
    struct d3dx9_line *line = impl_from_ID3DXLine(iface);

    HRESULT hr;

    TRACE("iface %p.\n", iface);

    if (!line->state)
        return D3DERR_INVALIDCALL;

    hr = IDirect3DStateBlock9_Apply(line->state);
    IDirect3DStateBlock9_Release(line->state);
    line->state = NULL;

    if (FAILED(hr))
        return D3DXERR_INVALIDDATA;

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_line_OnLostDevice(ID3DXLine *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}
static HRESULT WINAPI d3dx9_line_OnResetDevice(ID3DXLine *iface)
{
    FIXME("iface %p stub!\n", iface);

    return S_OK;
}

static const struct ID3DXLineVtbl d3dx9_line_vtbl =
{
    d3dx9_line_QueryInterface,
    d3dx9_line_AddRef,
    d3dx9_line_Release,
    d3dx9_line_GetDevice,
    d3dx9_line_Begin,
    d3dx9_line_Draw,
    d3dx9_line_DrawTransform,
    d3dx9_line_SetPattern,
    d3dx9_line_GetPattern,
    d3dx9_line_SetPatternScale,
    d3dx9_line_GetPatternScale,
    d3dx9_line_SetWidth,
    d3dx9_line_GetWidth,
    d3dx9_line_SetAntialias,
    d3dx9_line_GetAntialias,
    d3dx9_line_SetGLLines,
    d3dx9_line_GetGLLines,
    d3dx9_line_End,
    d3dx9_line_OnLostDevice,
    d3dx9_line_OnResetDevice,
};

HRESULT WINAPI D3DXCreateLine(struct IDirect3DDevice9 *device, struct ID3DXLine **line)
{
    struct d3dx9_line *object;

    TRACE("device %p, line %p.\n", device, line);

    if (!device || !line)
        return D3DERR_INVALIDCALL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID3DXLine_iface.lpVtbl = &d3dx9_line_vtbl;
    object->ref = 1;
    object->device = device;
    IDirect3DDevice9_AddRef(device);
    object->width = 1.0f;

    *line = &object->ID3DXLine_iface;

    return D3D_OK;
}
