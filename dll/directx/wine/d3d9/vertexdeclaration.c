/*
 * IDirect3DVertexDeclaration9 implementation
 *
 * Copyright 2002-2003 Raphael Junqueira
 *                     Jason Edmeades
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

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

typedef struct _D3DDECLTYPE_INFO {
    D3DDECLTYPE d3dType;
    enum wined3d_format_id format;
    int         size;
    int         typesize;
} D3DDECLTYPE_INFO;

static D3DDECLTYPE_INFO const d3d_dtype_lookup[D3DDECLTYPE_UNUSED] = {
   {D3DDECLTYPE_FLOAT1,    WINED3DFMT_R32_FLOAT,          1, sizeof(float)},
   {D3DDECLTYPE_FLOAT2,    WINED3DFMT_R32G32_FLOAT,       2, sizeof(float)},
   {D3DDECLTYPE_FLOAT3,    WINED3DFMT_R32G32B32_FLOAT,    3, sizeof(float)},
   {D3DDECLTYPE_FLOAT4,    WINED3DFMT_R32G32B32A32_FLOAT, 4, sizeof(float)},
   {D3DDECLTYPE_D3DCOLOR,  WINED3DFMT_B8G8R8A8_UNORM,     4, sizeof(BYTE)},
   {D3DDECLTYPE_UBYTE4,    WINED3DFMT_R8G8B8A8_UINT,      4, sizeof(BYTE)},
   {D3DDECLTYPE_SHORT2,    WINED3DFMT_R16G16_SINT,        2, sizeof(short int)},
   {D3DDECLTYPE_SHORT4,    WINED3DFMT_R16G16B16A16_SINT,  4, sizeof(short int)},
   {D3DDECLTYPE_UBYTE4N,   WINED3DFMT_R8G8B8A8_UNORM,     4, sizeof(BYTE)},
   {D3DDECLTYPE_SHORT2N,   WINED3DFMT_R16G16_SNORM,       2, sizeof(short int)},
   {D3DDECLTYPE_SHORT4N,   WINED3DFMT_R16G16B16A16_SNORM, 4, sizeof(short int)},
   {D3DDECLTYPE_USHORT2N,  WINED3DFMT_R16G16_UNORM,       2, sizeof(short int)},
   {D3DDECLTYPE_USHORT4N,  WINED3DFMT_R16G16B16A16_UNORM, 4, sizeof(short int)},
   {D3DDECLTYPE_UDEC3,     WINED3DFMT_R10G10B10A2_UINT,   3, sizeof(short int)},
   {D3DDECLTYPE_DEC3N,     WINED3DFMT_R10G10B10A2_SNORM,  3, sizeof(short int)},
   {D3DDECLTYPE_FLOAT16_2, WINED3DFMT_R16G16_FLOAT,       2, sizeof(short int)},
   {D3DDECLTYPE_FLOAT16_4, WINED3DFMT_R16G16B16A16_FLOAT, 4, sizeof(short int)}};

#define D3D_DECL_SIZE(type)          d3d_dtype_lookup[type].size
#define D3D_DECL_TYPESIZE(type)      d3d_dtype_lookup[type].typesize

HRESULT vdecl_convert_fvf(
    DWORD fvf,
    D3DVERTEXELEMENT9** ppVertexElements) {

    unsigned int idx, idx2;
    unsigned int offset;
    BOOL has_pos = (fvf & D3DFVF_POSITION_MASK) != 0;
    BOOL has_blend = (fvf & D3DFVF_XYZB5) > D3DFVF_XYZRHW;
    BOOL has_blend_idx = has_blend &&
       (((fvf & D3DFVF_XYZB5) == D3DFVF_XYZB5) ||
        (fvf & D3DFVF_LASTBETA_D3DCOLOR) ||
        (fvf & D3DFVF_LASTBETA_UBYTE4));
    BOOL has_normal = (fvf & D3DFVF_NORMAL) != 0;
    BOOL has_psize = (fvf & D3DFVF_PSIZE) != 0;

    BOOL has_diffuse = (fvf & D3DFVF_DIFFUSE) != 0;
    BOOL has_specular = (fvf & D3DFVF_SPECULAR) !=0;

    DWORD num_textures = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    DWORD texcoords = (fvf & 0xFFFF0000) >> 16;

    D3DVERTEXELEMENT9 end_element = D3DDECL_END();
    D3DVERTEXELEMENT9 *elements = NULL;

    unsigned int size;
    DWORD num_blends = 1 + (((fvf & D3DFVF_XYZB5) - D3DFVF_XYZB1) >> 1);
    if (has_blend_idx) num_blends--;

    /* Compute declaration size */
    size = has_pos + (has_blend && num_blends > 0) + has_blend_idx + has_normal +
           has_psize + has_diffuse + has_specular + num_textures + 1;

    /* convert the declaration */
    elements = HeapAlloc(GetProcessHeap(), 0, size * sizeof(D3DVERTEXELEMENT9));
    if (!elements) return D3DERR_OUTOFVIDEOMEMORY;

    elements[size-1] = end_element;
    idx = 0;
    if (has_pos) {
        if (!has_blend && (fvf & D3DFVF_XYZRHW)) {
            elements[idx].Type = D3DDECLTYPE_FLOAT4;
            elements[idx].Usage = D3DDECLUSAGE_POSITIONT;
        }
        else if (!has_blend && (fvf & D3DFVF_XYZW) == D3DFVF_XYZW) {
            elements[idx].Type = D3DDECLTYPE_FLOAT4;
            elements[idx].Usage = D3DDECLUSAGE_POSITION;
        }
        else {
            elements[idx].Type = D3DDECLTYPE_FLOAT3;
            elements[idx].Usage = D3DDECLUSAGE_POSITION;
        }
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_blend && (num_blends > 0)) {
        if (((fvf & D3DFVF_XYZB5) == D3DFVF_XYZB2) && (fvf & D3DFVF_LASTBETA_D3DCOLOR))
            elements[idx].Type = D3DDECLTYPE_D3DCOLOR;
        else {
            switch(num_blends) {
                case 1: elements[idx].Type = D3DDECLTYPE_FLOAT1; break;
                case 2: elements[idx].Type = D3DDECLTYPE_FLOAT2; break;
                case 3: elements[idx].Type = D3DDECLTYPE_FLOAT3; break;
                case 4: elements[idx].Type = D3DDECLTYPE_FLOAT4; break;
                default:
                    ERR("Unexpected amount of blend values: %u\n", num_blends);
            }
        }
        elements[idx].Usage = D3DDECLUSAGE_BLENDWEIGHT;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_blend_idx) {
        if (fvf & D3DFVF_LASTBETA_UBYTE4 ||
            (((fvf & D3DFVF_XYZB5) == D3DFVF_XYZB2) && (fvf & D3DFVF_LASTBETA_D3DCOLOR)))
            elements[idx].Type = D3DDECLTYPE_UBYTE4;
        else if (fvf & D3DFVF_LASTBETA_D3DCOLOR)
            elements[idx].Type = D3DDECLTYPE_D3DCOLOR;
        else
            elements[idx].Type = D3DDECLTYPE_FLOAT1;
        elements[idx].Usage = D3DDECLUSAGE_BLENDINDICES;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_normal) {
        elements[idx].Type = D3DDECLTYPE_FLOAT3;
        elements[idx].Usage = D3DDECLUSAGE_NORMAL;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_psize) {
        elements[idx].Type = D3DDECLTYPE_FLOAT1;
        elements[idx].Usage = D3DDECLUSAGE_PSIZE;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_diffuse) {
        elements[idx].Type = D3DDECLTYPE_D3DCOLOR;
        elements[idx].Usage = D3DDECLUSAGE_COLOR;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_specular) {
        elements[idx].Type = D3DDECLTYPE_D3DCOLOR;
        elements[idx].Usage = D3DDECLUSAGE_COLOR;
        elements[idx].UsageIndex = 1;
        idx++;
    }
    for (idx2 = 0; idx2 < num_textures; idx2++) {
        unsigned int numcoords = (texcoords >> (idx2*2)) & 0x03;
        switch (numcoords) {
            case D3DFVF_TEXTUREFORMAT1:
                elements[idx].Type = D3DDECLTYPE_FLOAT1;
                break;
            case D3DFVF_TEXTUREFORMAT2:
                elements[idx].Type = D3DDECLTYPE_FLOAT2;
                break;
            case D3DFVF_TEXTUREFORMAT3:
                elements[idx].Type = D3DDECLTYPE_FLOAT3;
                break;
            case D3DFVF_TEXTUREFORMAT4:
                elements[idx].Type = D3DDECLTYPE_FLOAT4;
                break;
        }
        elements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
        elements[idx].UsageIndex = idx2;
        idx++;
    }

    /* Now compute offsets, and initialize the rest of the fields */
    for (idx = 0, offset = 0; idx < size-1; idx++) {
        elements[idx].Stream = 0;
        elements[idx].Method = D3DDECLMETHOD_DEFAULT;
        elements[idx].Offset = offset;
        offset += D3D_DECL_SIZE(elements[idx].Type) * D3D_DECL_TYPESIZE(elements[idx].Type);
    }

    *ppVertexElements = elements;
    return D3D_OK;
}

/* IDirect3DVertexDeclaration9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVertexDeclaration9Impl_QueryInterface(LPDIRECT3DVERTEXDECLARATION9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVertexDeclaration9)) {
        IDirect3DVertexDeclaration9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexDeclaration9Impl_AddRef(LPDIRECT3DVERTEXDECLARATION9 iface) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if(ref == 1) {
        IDirect3DDevice9Ex_AddRef(This->parentDevice);
        if (!This->convFVF)
        {
            wined3d_mutex_lock();
            wined3d_vertex_declaration_incref(This->wineD3DVertexDeclaration);
            wined3d_mutex_unlock();
        }
    }

    return ref;
}

void IDirect3DVertexDeclaration9Impl_Destroy(LPDIRECT3DVERTEXDECLARATION9 iface) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;

    if(This->ref != 0) {
        /* Should not happen unless wine has a bug or the application releases references it does not own */
        ERR("Destroying vdecl with ref != 0\n");
    }

    wined3d_mutex_lock();
    wined3d_vertex_declaration_decref(This->wineD3DVertexDeclaration);
    wined3d_mutex_unlock();
}

static ULONG WINAPI IDirect3DVertexDeclaration9Impl_Release(LPDIRECT3DVERTEXDECLARATION9 iface) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        if(!This->convFVF) {
            IDirect3DVertexDeclaration9Impl_Destroy(iface);
        }

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DVertexDeclaration9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexDeclaration9Impl_GetDevice(IDirect3DVertexDeclaration9 *iface,
        IDirect3DDevice9 **device)
{
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVertexDeclaration9Impl_GetDeclaration(LPDIRECT3DVERTEXDECLARATION9 iface, D3DVERTEXELEMENT9* pDecl, UINT* pNumElements) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;

    TRACE("iface %p, elements %p, element_count %p.\n", iface, pDecl, pNumElements);

    *pNumElements = This->element_count;

    /* Passing a NULL pDecl is used to just retrieve the number of elements */
    if (!pDecl) {
        TRACE("NULL pDecl passed. Returning D3D_OK.\n");
        return D3D_OK;
    }

    TRACE("Copying %p to %p\n", This->elements, pDecl);
    CopyMemory(pDecl, This->elements, This->element_count * sizeof(D3DVERTEXELEMENT9));

    return D3D_OK;
}

static const IDirect3DVertexDeclaration9Vtbl Direct3DVertexDeclaration9_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexDeclaration9Impl_QueryInterface,
    IDirect3DVertexDeclaration9Impl_AddRef,
    IDirect3DVertexDeclaration9Impl_Release,
    /* IDirect3DVertexDeclaration9 */
    IDirect3DVertexDeclaration9Impl_GetDevice,
    IDirect3DVertexDeclaration9Impl_GetDeclaration
};

static void STDMETHODCALLTYPE d3d9_vertexdeclaration_wined3d_object_destroyed(void *parent)
{
    IDirect3DVertexDeclaration9Impl *declaration = parent;
    HeapFree(GetProcessHeap(), 0, declaration->elements);
    HeapFree(GetProcessHeap(), 0, declaration);
}

static const struct wined3d_parent_ops d3d9_vertexdeclaration_wined3d_parent_ops =
{
    d3d9_vertexdeclaration_wined3d_object_destroyed,
};

static HRESULT convert_to_wined3d_declaration(const D3DVERTEXELEMENT9 *d3d9_elements,
        struct wined3d_vertex_element **wined3d_elements, UINT *element_count)
{
    const D3DVERTEXELEMENT9* element;
    UINT count = 1;
    UINT i;

    TRACE("d3d9_elements %p, wined3d_elements %p\n", d3d9_elements, wined3d_elements);

    element = d3d9_elements;
    while (element++->Stream != 0xff && count++ < 128);

    if (count == 128) return E_FAIL;

    /* Skip the END element */
    --count;

    *wined3d_elements = HeapAlloc(GetProcessHeap(), 0, count * sizeof(**wined3d_elements));
    if (!*wined3d_elements) {
        FIXME("Memory allocation failed\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    for (i = 0; i < count; ++i)
    {
        if (d3d9_elements[i].Type >= (sizeof(d3d_dtype_lookup) / sizeof(*d3d_dtype_lookup)))
        {
            WARN("Invalid element type %#x.\n", d3d9_elements[i].Type);
            HeapFree(GetProcessHeap(), 0, *wined3d_elements);
            return E_FAIL;
        }
        (*wined3d_elements)[i].format = d3d_dtype_lookup[d3d9_elements[i].Type].format;
        (*wined3d_elements)[i].input_slot = d3d9_elements[i].Stream;
        (*wined3d_elements)[i].offset = d3d9_elements[i].Offset;
        (*wined3d_elements)[i].output_slot = ~0U;
        (*wined3d_elements)[i].method = d3d9_elements[i].Method;
        (*wined3d_elements)[i].usage = d3d9_elements[i].Usage;
        (*wined3d_elements)[i].usage_idx = d3d9_elements[i].UsageIndex;
    }

    *element_count = count;

    return D3D_OK;
}

HRESULT vertexdeclaration_init(IDirect3DVertexDeclaration9Impl *declaration,
        IDirect3DDevice9Impl *device, const D3DVERTEXELEMENT9 *elements)
{
    struct wined3d_vertex_element *wined3d_elements;
    UINT wined3d_element_count;
    UINT element_count;
    HRESULT hr;

    hr = convert_to_wined3d_declaration(elements, &wined3d_elements, &wined3d_element_count);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d vertex declaration elements, hr %#x.\n", hr);
        return hr;
    }

    declaration->lpVtbl = &Direct3DVertexDeclaration9_Vtbl;
    declaration->ref = 1;

    element_count = wined3d_element_count + 1;
    declaration->elements = HeapAlloc(GetProcessHeap(), 0, element_count * sizeof(*declaration->elements));
    if (!declaration->elements)
    {
        HeapFree(GetProcessHeap(), 0, wined3d_elements);
        ERR("Failed to allocate vertex declaration elements memory.\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    memcpy(declaration->elements, elements, element_count * sizeof(*elements));
    declaration->element_count = element_count;

    wined3d_mutex_lock();
    hr = wined3d_vertex_declaration_create(device->wined3d_device, wined3d_elements, wined3d_element_count,
            declaration, &d3d9_vertexdeclaration_wined3d_parent_ops, &declaration->wineD3DVertexDeclaration);
    wined3d_mutex_unlock();
    HeapFree(GetProcessHeap(), 0, wined3d_elements);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, declaration->elements);
        WARN("Failed to create wined3d vertex declaration, hr %#x.\n", hr);
        return hr;
    }

    declaration->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(declaration->parentDevice);

    return D3D_OK;
}
