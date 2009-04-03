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
    int         size;
    int         typesize;
} D3DDECLTYPE_INFO;

static D3DDECLTYPE_INFO const d3d_dtype_lookup[D3DDECLTYPE_UNUSED] = {
   {D3DDECLTYPE_FLOAT1,    1, sizeof(float)},
   {D3DDECLTYPE_FLOAT2,    2, sizeof(float)},
   {D3DDECLTYPE_FLOAT3,    3, sizeof(float)},
   {D3DDECLTYPE_FLOAT4,    4, sizeof(float)},
   {D3DDECLTYPE_D3DCOLOR,  4, sizeof(BYTE)},
   {D3DDECLTYPE_UBYTE4,    4, sizeof(BYTE)},
   {D3DDECLTYPE_SHORT2,    2, sizeof(short int)},
   {D3DDECLTYPE_SHORT4,    4, sizeof(short int)},
   {D3DDECLTYPE_UBYTE4N,   4, sizeof(BYTE)},
   {D3DDECLTYPE_SHORT2N,   2, sizeof(short int)},
   {D3DDECLTYPE_SHORT4N,   4, sizeof(short int)},
   {D3DDECLTYPE_USHORT2N,  2, sizeof(short int)},
   {D3DDECLTYPE_USHORT4N,  4, sizeof(short int)},
   {D3DDECLTYPE_UDEC3,     3, sizeof(short int)},
   {D3DDECLTYPE_DEC3N,     3, sizeof(short int)},
   {D3DDECLTYPE_FLOAT16_2, 2, sizeof(short int)},
   {D3DDECLTYPE_FLOAT16_4, 4, sizeof(short int)}};

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
    if (!elements) 
        return D3DERR_OUTOFVIDEOMEMORY;

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

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    if(ref == 1) {
        IDirect3DDevice9Ex_AddRef(This->parentDevice);
    }

    return ref;
}

void IDirect3DVertexDeclaration9Impl_Destroy(LPDIRECT3DVERTEXDECLARATION9 iface) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;

    if(This->ref != 0) {
        /* Should not happen unless wine has a bug or the application releases references it does not own */
        ERR("Destroying vdecl with ref != 0\n");
    }
    EnterCriticalSection(&d3d9_cs);
    IWineD3DVertexDeclaration_Release(This->wineD3DVertexDeclaration);
    LeaveCriticalSection(&d3d9_cs);
    HeapFree(GetProcessHeap(), 0, This->elements);
    HeapFree(GetProcessHeap(), 0, This);
}

static ULONG WINAPI IDirect3DVertexDeclaration9Impl_Release(LPDIRECT3DVERTEXDECLARATION9 iface) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        if(!This->convFVF) {
            IDirect3DVertexDeclaration9Impl_Release(iface);
        }
        IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DVertexDeclaration9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexDeclaration9Impl_GetDevice(LPDIRECT3DVERTEXDECLARATION9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;
    IWineD3DDevice *myDevice = NULL;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : Relay\n", iface);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DVertexDeclaration_GetDevice(This->wineD3DVertexDeclaration, &myDevice);
    if (hr == D3D_OK && myDevice != NULL) {
        hr = IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(myDevice);
    }
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexDeclaration9Impl_GetDeclaration(LPDIRECT3DVERTEXDECLARATION9 iface, D3DVERTEXELEMENT9* pDecl, UINT* pNumElements) {
    IDirect3DVertexDeclaration9Impl *This = (IDirect3DVertexDeclaration9Impl *)iface;

    TRACE("(%p) : pDecl %p, pNumElements %p)\n", This, pDecl, pNumElements);

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

static UINT convert_to_wined3d_declaration(const D3DVERTEXELEMENT9* d3d9_elements, WINED3DVERTEXELEMENT **wined3d_elements) {
    const D3DVERTEXELEMENT9* element;
    UINT element_count = 1;
    UINT i;

    TRACE("d3d9_elements %p, wined3d_elements %p\n", d3d9_elements, wined3d_elements);

    element = d3d9_elements;
    while (element++->Stream != 0xff && element_count++ < 128);

    if (element_count == 128) {
        return 0;
    }

    *wined3d_elements = HeapAlloc(GetProcessHeap(), 0, element_count * sizeof(WINED3DVERTEXELEMENT));
    if (!*wined3d_elements) {
        FIXME("Memory allocation failed\n");
        return 0;
    }

    for (i = 0; i < element_count; ++i) {
        CopyMemory(*wined3d_elements + i, d3d9_elements + i, sizeof(D3DVERTEXELEMENT9));
        (*wined3d_elements)[i].Reg = -1;
    }

    return element_count;
}

/* IDirect3DDevice9 IDirect3DVertexDeclaration9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexDeclaration(LPDIRECT3DDEVICE9EX iface, CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) {
    
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9Impl *object = NULL;
    WINED3DVERTEXELEMENT* wined3d_elements;
    UINT element_count;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : Relay\n", iface);
    if (NULL == ppDecl) {
        WARN("(%p) : Caller passed NULL As ppDecl, returning D3DERR_INVALIDCALL\n",This);
        return D3DERR_INVALIDCALL;
    }

    element_count = convert_to_wined3d_declaration(pVertexElements, &wined3d_elements);
    if (!element_count) {
        FIXME("(%p) : Error parsing vertex declaration\n", This);
        return D3DERR_INVALIDCALL;
    }

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexDeclaration9Impl));
    if (NULL == object) {
        HeapFree(GetProcessHeap(), 0, wined3d_elements);
        FIXME("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVertexDeclaration9_Vtbl;
    object->ref = 0;

    object->elements = HeapAlloc(GetProcessHeap(), 0, element_count * sizeof(D3DVERTEXELEMENT9));
    if (!object->elements) {
        HeapFree(GetProcessHeap(), 0, wined3d_elements);
        HeapFree(GetProcessHeap(), 0, object);
        ERR("Memory allocation failed\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    CopyMemory(object->elements, pVertexElements, element_count * sizeof(D3DVERTEXELEMENT9));
    object->element_count = element_count;

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_CreateVertexDeclaration(This->WineD3DDevice, &object->wineD3DVertexDeclaration, (IUnknown *)object, wined3d_elements, element_count);
    LeaveCriticalSection(&d3d9_cs);

    HeapFree(GetProcessHeap(), 0, wined3d_elements);

    if (FAILED(hr)) {

        /* free up object */
        WARN("(%p) call to IWineD3DDevice_CreateVertexDeclaration failed\n", This);
        HeapFree(GetProcessHeap(), 0, object->elements);
        HeapFree(GetProcessHeap(), 0, object);
    } else {
        object->parentDevice = iface;
        *ppDecl = (LPDIRECT3DVERTEXDECLARATION9) object;
        IDirect3DVertexDeclaration9_AddRef(*ppDecl);
         TRACE("(%p) : Created vertex declaration %p\n", This, object);
    }
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexDeclaration(LPDIRECT3DDEVICE9EX iface, IDirect3DVertexDeclaration9* pDecl) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9Impl *pDeclImpl = (IDirect3DVertexDeclaration9Impl *)pDecl;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : Relay\n", iface);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetVertexDeclaration(This->WineD3DDevice, pDeclImpl == NULL ? NULL : pDeclImpl->wineD3DVertexDeclaration);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexDeclaration(LPDIRECT3DDEVICE9EX iface, IDirect3DVertexDeclaration9** ppDecl) {
    IDirect3DDevice9Impl* This = (IDirect3DDevice9Impl*) iface;
    IWineD3DVertexDeclaration* pTest = NULL;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : Relay+\n", iface);

    if (NULL == ppDecl) {
      return D3DERR_INVALIDCALL;
    }

    *ppDecl = NULL;
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetVertexDeclaration(This->WineD3DDevice, &pTest);
    if (hr == D3D_OK && NULL != pTest) {
        IWineD3DVertexDeclaration_GetParent(pTest, (IUnknown **)ppDecl);
        IWineD3DVertexDeclaration_Release(pTest);
    } else {
        *ppDecl = NULL;
    }
    LeaveCriticalSection(&d3d9_cs);
    TRACE("(%p) : returning %p\n", This, *ppDecl);
    return hr;
}
