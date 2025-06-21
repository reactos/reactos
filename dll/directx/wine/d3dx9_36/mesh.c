 /*
 * Mesh operations specific to D3DX9.
 *
 * Copyright (C) 2005 Henri Verbeet
 * Copyright (C) 2006 Ivan Gyurdiev
 * Copyright (C) 2009 David Adam
 * Copyright (C) 2010 Tony Wasserka
 * Copyright (C) 2011 Dylan Smith
 * Copyright (C) 2011 Michael Mc Donnell
 * Copyright (C) 2013 Christian Costa
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


#include <assert.h>
#include <float.h>

#include "d3dx9_private.h"
#undef MAKE_DDHRESULT
#include "dxfile.h"
#include "rmxfguid.h"
#include "rmxftmpl.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/* for provide_flags parameters */
#define PROVIDE_MATERIALS 0x1
#define PROVIDE_SKININFO  0x2
#define PROVIDE_ADJACENCY 0x4

struct d3dx9_mesh
{
    ID3DXMesh ID3DXMesh_iface;
    LONG ref;

    DWORD numfaces;
    DWORD numvertices;
    DWORD options;
    DWORD fvf;
    IDirect3DDevice9 *device;
    D3DVERTEXELEMENT9 cached_declaration[MAX_FVF_DECL_SIZE];
    IDirect3DVertexDeclaration9 *vertex_declaration;
    UINT vertex_declaration_size;
    UINT num_elem;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    DWORD *attrib_buffer;
    LONG attrib_buffer_lock_count;
    DWORD attrib_table_size;
    D3DXATTRIBUTERANGE *attrib_table;
};

static const UINT d3dx_decltype_size[] =
{
   /* D3DDECLTYPE_FLOAT1    */ sizeof(FLOAT),
   /* D3DDECLTYPE_FLOAT2    */ sizeof(D3DXVECTOR2),
   /* D3DDECLTYPE_FLOAT3    */ sizeof(D3DXVECTOR3),
   /* D3DDECLTYPE_FLOAT4    */ sizeof(D3DXVECTOR4),
   /* D3DDECLTYPE_D3DCOLOR  */ sizeof(D3DCOLOR),
   /* D3DDECLTYPE_UBYTE4    */ 4 * sizeof(BYTE),
   /* D3DDECLTYPE_SHORT2    */ 2 * sizeof(SHORT),
   /* D3DDECLTYPE_SHORT4    */ 4 * sizeof(SHORT),
   /* D3DDECLTYPE_UBYTE4N   */ 4 * sizeof(BYTE),
   /* D3DDECLTYPE_SHORT2N   */ 2 * sizeof(SHORT),
   /* D3DDECLTYPE_SHORT4N   */ 4 * sizeof(SHORT),
   /* D3DDECLTYPE_USHORT2N  */ 2 * sizeof(USHORT),
   /* D3DDECLTYPE_USHORT4N  */ 4 * sizeof(USHORT),
   /* D3DDECLTYPE_UDEC3     */ 4, /* 3 * 10 bits + 2 padding */
   /* D3DDECLTYPE_DEC3N     */ 4,
   /* D3DDECLTYPE_FLOAT16_2 */ 2 * sizeof(D3DXFLOAT16),
   /* D3DDECLTYPE_FLOAT16_4 */ 4 * sizeof(D3DXFLOAT16),
};

static inline struct d3dx9_mesh *impl_from_ID3DXMesh(ID3DXMesh *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_mesh, ID3DXMesh_iface);
}

static HRESULT WINAPI d3dx9_mesh_QueryInterface(ID3DXMesh *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXBaseMesh) ||
        IsEqualGUID(riid, &IID_ID3DXMesh))
    {
        iface->lpVtbl->AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("Interface %s not found.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_mesh_AddRef(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);
    ULONG refcount = InterlockedIncrement(&mesh->ref);

    TRACE("%p increasing refcount to %lu.\n", mesh, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_mesh_Release(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);
    ULONG refcount = InterlockedDecrement(&mesh->ref);

    TRACE("%p decreasing refcount to %lu.\n", mesh, refcount);

    if (!refcount)
    {
        IDirect3DIndexBuffer9_Release(mesh->index_buffer);
        IDirect3DVertexBuffer9_Release(mesh->vertex_buffer);
        if (mesh->vertex_declaration)
            IDirect3DVertexDeclaration9_Release(mesh->vertex_declaration);
        IDirect3DDevice9_Release(mesh->device);
        free(mesh->attrib_buffer);
        free(mesh->attrib_table);
        free(mesh);
    }

    return refcount;
}

static HRESULT WINAPI d3dx9_mesh_DrawSubset(ID3DXMesh *iface, DWORD attrib_id)
{
    struct d3dx9_mesh *This = impl_from_ID3DXMesh(iface);
    HRESULT hr;
    DWORD face_start;
    DWORD face_end = 0;
    DWORD vertex_size;

    TRACE("iface %p, attrib_id %lu.\n", iface, attrib_id);

    if (!This->vertex_declaration)
    {
        WARN("Can't draw a mesh with an invalid vertex declaration.\n");
        return E_FAIL;
    }

    vertex_size = iface->lpVtbl->GetNumBytesPerVertex(iface);

    hr = IDirect3DDevice9_SetVertexDeclaration(This->device, This->vertex_declaration);
    if (FAILED(hr)) return hr;
    hr = IDirect3DDevice9_SetStreamSource(This->device, 0, This->vertex_buffer, 0, vertex_size);
    if (FAILED(hr)) return hr;
    hr = IDirect3DDevice9_SetIndices(This->device, This->index_buffer);
    if (FAILED(hr)) return hr;

    while (face_end < This->numfaces)
    {
        for (face_start = face_end; face_start < This->numfaces; face_start++)
        {
            if (This->attrib_buffer[face_start] == attrib_id)
                break;
        }
        if (face_start >= This->numfaces)
            break;
        for (face_end = face_start + 1; face_end < This->numfaces; face_end++)
        {
            if (This->attrib_buffer[face_end] != attrib_id)
                break;
        }

        hr = IDirect3DDevice9_DrawIndexedPrimitive(This->device, D3DPT_TRIANGLELIST,
                0, 0, This->numvertices, face_start * 3, face_end - face_start);
        if (FAILED(hr)) return hr;
    }

    return D3D_OK;
}

static DWORD WINAPI d3dx9_mesh_GetNumFaces(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p.\n", iface);

    return mesh->numfaces;
}

static DWORD WINAPI d3dx9_mesh_GetNumVertices(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p.\n", iface);

    return mesh->numvertices;
}

static DWORD WINAPI d3dx9_mesh_GetFVF(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p.\n", iface);

    return mesh->fvf;
}

static void copy_declaration(D3DVERTEXELEMENT9 *dst, const D3DVERTEXELEMENT9 *src, UINT num_elem)
{
    memcpy(dst, src, num_elem * sizeof(*src));
}

static HRESULT WINAPI d3dx9_mesh_GetDeclaration(ID3DXMesh *iface, D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p, declaration %p.\n", iface, declaration);

    if (!declaration)
        return D3DERR_INVALIDCALL;

    copy_declaration(declaration, mesh->cached_declaration, mesh->num_elem);

    return D3D_OK;
}

static DWORD WINAPI d3dx9_mesh_GetNumBytesPerVertex(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p.\n", iface);

    return mesh->vertex_declaration_size;
}

static DWORD WINAPI d3dx9_mesh_GetOptions(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p.\n", iface);

    return mesh->options;
}

static HRESULT WINAPI d3dx9_mesh_GetDevice(struct ID3DXMesh *iface, struct IDirect3DDevice9 **device)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (!device)
        return D3DERR_INVALIDCALL;
    *device = mesh->device;
    IDirect3DDevice9_AddRef(mesh->device);

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_mesh_CloneMeshFVF(struct ID3DXMesh *iface, DWORD options, DWORD fvf,
        struct IDirect3DDevice9 *device, struct ID3DXMesh **clone_mesh)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];

    TRACE("iface %p, options %#lx, fvf %#lx, device %p, clone_mesh %p.\n",
            iface, options, fvf, device, clone_mesh);

    if (FAILED(hr = D3DXDeclaratorFromFVF(fvf, declaration)))
        return hr;

    return iface->lpVtbl->CloneMesh(iface, options, declaration, device, clone_mesh);
}

static FLOAT scale_clamp_ubyten(FLOAT value)
{
    value = value * UCHAR_MAX;

    if (value < 0.0f)
    {
        return 0.0f;
    }
    else
    {
        if (value > UCHAR_MAX) /* Clamp at 255 */
            return UCHAR_MAX;
        else
            return value;
    }
}

static FLOAT scale_clamp_shortn(FLOAT value)
{
    value = value * SHRT_MAX;

    /* The tests show that the range is SHRT_MIN + 1 to SHRT_MAX. */
    if (value <= SHRT_MIN)
    {
        return SHRT_MIN + 1;
    }
    else if (value > SHRT_MAX)
    {
         return SHRT_MAX;
    }
    else
    {
        return value;
    }
}

static FLOAT scale_clamp_ushortn(FLOAT value)
{
    value = value * USHRT_MAX;

    if (value < 0.0f)
    {
        return 0.0f;
    }
    else
    {
        if (value > USHRT_MAX) /* Clamp at 65535 */
            return USHRT_MAX;
        else
            return value;
    }
}

static INT simple_round(FLOAT value)
{
    int res = (INT)(value + 0.5f);

    return res;
}

static void convert_float4(BYTE *dst, const D3DXVECTOR4 *src, D3DDECLTYPE type_dst)
{
    BOOL fixme_once = FALSE;

    switch (type_dst)
    {
        case D3DDECLTYPE_FLOAT1:
        {
            FLOAT *dst_ptr = (FLOAT*)dst;
            *dst_ptr = src->x;
            break;
        }
        case D3DDECLTYPE_FLOAT2:
        {
            D3DXVECTOR2 *dst_ptr = (D3DXVECTOR2*)dst;
            dst_ptr->x = src->x;
            dst_ptr->y = src->y;
            break;
        }
        case D3DDECLTYPE_FLOAT3:
        {
            D3DXVECTOR3 *dst_ptr = (D3DXVECTOR3*)dst;
            dst_ptr->x = src->x;
            dst_ptr->y = src->y;
            dst_ptr->z = src->z;
            break;
        }
        case D3DDECLTYPE_FLOAT4:
        {
            D3DXVECTOR4 *dst_ptr = (D3DXVECTOR4*)dst;
            dst_ptr->x = src->x;
            dst_ptr->y = src->y;
            dst_ptr->z = src->z;
            dst_ptr->w = src->w;
            break;
        }
        case D3DDECLTYPE_D3DCOLOR:
        {
            dst[0] = (BYTE)simple_round(scale_clamp_ubyten(src->z));
            dst[1] = (BYTE)simple_round(scale_clamp_ubyten(src->y));
            dst[2] = (BYTE)simple_round(scale_clamp_ubyten(src->x));
            dst[3] = (BYTE)simple_round(scale_clamp_ubyten(src->w));
            break;
        }
        case D3DDECLTYPE_UBYTE4:
        {
            dst[0] = src->x < 0.0f ? 0 : (BYTE)simple_round(src->x);
            dst[1] = src->y < 0.0f ? 0 : (BYTE)simple_round(src->y);
            dst[2] = src->z < 0.0f ? 0 : (BYTE)simple_round(src->z);
            dst[3] = src->w < 0.0f ? 0 : (BYTE)simple_round(src->w);
            break;
        }
        case D3DDECLTYPE_SHORT2:
        {
            SHORT *dst_ptr = (SHORT*)dst;
            dst_ptr[0] = (SHORT)simple_round(src->x);
            dst_ptr[1] = (SHORT)simple_round(src->y);
            break;
        }
        case D3DDECLTYPE_SHORT4:
        {
            SHORT *dst_ptr = (SHORT*)dst;
            dst_ptr[0] = (SHORT)simple_round(src->x);
            dst_ptr[1] = (SHORT)simple_round(src->y);
            dst_ptr[2] = (SHORT)simple_round(src->z);
            dst_ptr[3] = (SHORT)simple_round(src->w);
            break;
        }
        case D3DDECLTYPE_UBYTE4N:
        {
            dst[0] = (BYTE)simple_round(scale_clamp_ubyten(src->x));
            dst[1] = (BYTE)simple_round(scale_clamp_ubyten(src->y));
            dst[2] = (BYTE)simple_round(scale_clamp_ubyten(src->z));
            dst[3] = (BYTE)simple_round(scale_clamp_ubyten(src->w));
            break;
        }
        case D3DDECLTYPE_SHORT2N:
        {
            SHORT *dst_ptr = (SHORT*)dst;
            dst_ptr[0] = (SHORT)simple_round(scale_clamp_shortn(src->x));
            dst_ptr[1] = (SHORT)simple_round(scale_clamp_shortn(src->y));
            break;
        }
        case D3DDECLTYPE_SHORT4N:
        {
            SHORT *dst_ptr = (SHORT*)dst;
            dst_ptr[0] = (SHORT)simple_round(scale_clamp_shortn(src->x));
            dst_ptr[1] = (SHORT)simple_round(scale_clamp_shortn(src->y));
            dst_ptr[2] = (SHORT)simple_round(scale_clamp_shortn(src->z));
            dst_ptr[3] = (SHORT)simple_round(scale_clamp_shortn(src->w));
            break;
        }
        case D3DDECLTYPE_USHORT2N:
        {
            USHORT *dst_ptr = (USHORT*)dst;
            dst_ptr[0] = (USHORT)simple_round(scale_clamp_ushortn(src->x));
            dst_ptr[1] = (USHORT)simple_round(scale_clamp_ushortn(src->y));
            break;
        }
        case D3DDECLTYPE_USHORT4N:
        {
            USHORT *dst_ptr = (USHORT*)dst;
            dst_ptr[0] = (USHORT)simple_round(scale_clamp_ushortn(src->x));
            dst_ptr[1] = (USHORT)simple_round(scale_clamp_ushortn(src->y));
            dst_ptr[2] = (USHORT)simple_round(scale_clamp_ushortn(src->z));
            dst_ptr[3] = (USHORT)simple_round(scale_clamp_ushortn(src->w));
            break;
        }
        case D3DDECLTYPE_FLOAT16_2:
        {
            D3DXFloat32To16Array((D3DXFLOAT16*)dst, (FLOAT*)src, 2);
            break;
        }
        case D3DDECLTYPE_FLOAT16_4:
        {
            D3DXFloat32To16Array((D3DXFLOAT16*)dst, (FLOAT*)src, 4);
            break;
        }
        default:
            if (!fixme_once++)
                FIXME("Conversion from D3DDECLTYPE_FLOAT4 to %d not implemented.\n", type_dst);
            break;
    }
}

static void convert_component(BYTE *dst, BYTE *src, D3DDECLTYPE type_dst, D3DDECLTYPE type_src)
{
    BOOL fixme_once = FALSE;

    switch (type_src)
    {
        case D3DDECLTYPE_FLOAT1:
        {
            FLOAT *src_ptr = (FLOAT*)src;
            D3DXVECTOR4 src_float4 = {*src_ptr, 0.0f, 0.0f, 1.0f};
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_FLOAT2:
        {
            D3DXVECTOR2 *src_ptr = (D3DXVECTOR2*)src;
            D3DXVECTOR4 src_float4 = {src_ptr->x, src_ptr->y, 0.0f, 1.0f};
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_FLOAT3:
        {
            D3DXVECTOR3 *src_ptr = (D3DXVECTOR3*)src;
            D3DXVECTOR4 src_float4 = {src_ptr->x, src_ptr->y, src_ptr->z, 1.0f};
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_FLOAT4:
        {
            D3DXVECTOR4 *src_ptr = (D3DXVECTOR4*)src;
            D3DXVECTOR4 src_float4 = {src_ptr->x, src_ptr->y, src_ptr->z, src_ptr->w};
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_D3DCOLOR:
        {
            D3DXVECTOR4 src_float4 =
            {
                (FLOAT)src[2]/UCHAR_MAX,
                (FLOAT)src[1]/UCHAR_MAX,
                (FLOAT)src[0]/UCHAR_MAX,
                (FLOAT)src[3]/UCHAR_MAX
            };
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_UBYTE4:
        {
            D3DXVECTOR4 src_float4 = {src[0], src[1], src[2], src[3]};
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_SHORT2:
        {
            SHORT *src_ptr = (SHORT*)src;
            D3DXVECTOR4 src_float4 = {src_ptr[0], src_ptr[1], 0.0f, 1.0f};
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_SHORT4:
        {
            SHORT *src_ptr = (SHORT*)src;
            D3DXVECTOR4 src_float4 = {src_ptr[0], src_ptr[1], src_ptr[2], src_ptr[3]};
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_UBYTE4N:
        {
            D3DXVECTOR4 src_float4 =
            {
                (FLOAT)src[0]/UCHAR_MAX,
                (FLOAT)src[1]/UCHAR_MAX,
                (FLOAT)src[2]/UCHAR_MAX,
                (FLOAT)src[3]/UCHAR_MAX
            };
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_SHORT2N:
        {
            SHORT *src_ptr = (SHORT*)src;
            D3DXVECTOR4 src_float4 = {(FLOAT)src_ptr[0]/SHRT_MAX, (FLOAT)src_ptr[1]/SHRT_MAX, 0.0f, 1.0f};
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_SHORT4N:
        {
            SHORT *src_ptr = (SHORT*)src;
            D3DXVECTOR4 src_float4 =
            {
                (FLOAT)src_ptr[0]/SHRT_MAX,
                (FLOAT)src_ptr[1]/SHRT_MAX,
                (FLOAT)src_ptr[2]/SHRT_MAX,
                (FLOAT)src_ptr[3]/SHRT_MAX
            };
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_FLOAT16_2:
        {
            D3DXVECTOR4 src_float4 = {0.0f, 0.0f, 0.0f, 1.0f};
            D3DXFloat16To32Array((FLOAT*)&src_float4, (D3DXFLOAT16*)src, 2);
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        case D3DDECLTYPE_FLOAT16_4:
        {
            D3DXVECTOR4 src_float4;
            D3DXFloat16To32Array((FLOAT*)&src_float4, (D3DXFLOAT16*)src, 4);
            convert_float4(dst, &src_float4, type_dst);
            break;
        }
        default:
            if (!fixme_once++)
                FIXME("Conversion of D3DDECLTYPE %d to %d not implemented.\n", type_src, type_dst);
            break;
    }
}

static INT get_equivalent_declaration_index(D3DVERTEXELEMENT9 orig_declaration, D3DVERTEXELEMENT9 *declaration)
{
    INT i;

    for (i = 0; declaration[i].Stream != 0xff; i++)
    {
        if (orig_declaration.Usage == declaration[i].Usage
            && orig_declaration.UsageIndex == declaration[i].UsageIndex)
        {
            return i;
        }
    }

    return -1;
}

static HRESULT convert_vertex_buffer(ID3DXMesh *mesh_dst, ID3DXMesh *mesh_src)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 orig_declaration[MAX_FVF_DECL_SIZE] = {D3DDECL_END()};
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE] = {D3DDECL_END()};
    BYTE *vb_dst = NULL;
    BYTE *vb_src = NULL;
    UINT i;
    UINT num_vertices = mesh_src->lpVtbl->GetNumVertices(mesh_src);
    UINT dst_vertex_size = mesh_dst->lpVtbl->GetNumBytesPerVertex(mesh_dst);
    UINT src_vertex_size = mesh_src->lpVtbl->GetNumBytesPerVertex(mesh_src);

    hr = mesh_src->lpVtbl->GetDeclaration(mesh_src, orig_declaration);
    if (FAILED(hr)) return hr;
    hr = mesh_dst->lpVtbl->GetDeclaration(mesh_dst, declaration);
    if (FAILED(hr)) return hr;

    hr = mesh_src->lpVtbl->LockVertexBuffer(mesh_src, D3DLOCK_READONLY, (void**)&vb_src);
    if (FAILED(hr)) goto cleanup;
    hr = mesh_dst->lpVtbl->LockVertexBuffer(mesh_dst, 0, (void**)&vb_dst);
    if (FAILED(hr)) goto cleanup;

    /* Clear all new fields by clearing the entire vertex buffer. */
    memset(vb_dst, 0, num_vertices * dst_vertex_size);

    for (i = 0; orig_declaration[i].Stream != 0xff; i++)
    {
        INT eq_idx = get_equivalent_declaration_index(orig_declaration[i], declaration);

        if (eq_idx >= 0)
        {
            UINT j;
            for (j = 0; j < num_vertices; j++)
            {
                UINT idx_dst = dst_vertex_size * j + declaration[eq_idx].Offset;
                UINT idx_src = src_vertex_size * j + orig_declaration[i].Offset;
                UINT type_size = d3dx_decltype_size[orig_declaration[i].Type];

                if (orig_declaration[i].Type == declaration[eq_idx].Type)
                    memcpy(&vb_dst[idx_dst], &vb_src[idx_src], type_size);
                else
                   convert_component(&vb_dst[idx_dst], &vb_src[idx_src], declaration[eq_idx].Type, orig_declaration[i].Type);
            }
        }
    }

    hr = D3D_OK;
cleanup:
    if (vb_dst) mesh_dst->lpVtbl->UnlockVertexBuffer(mesh_dst);
    if (vb_src) mesh_src->lpVtbl->UnlockVertexBuffer(mesh_src);

    return hr;
}

static BOOL declaration_equals(const D3DVERTEXELEMENT9 *declaration1, const D3DVERTEXELEMENT9 *declaration2)
{
    UINT size1 = 0, size2 = 0;

    /* Find the size of each declaration */
    while (declaration1[size1].Stream != 0xff) size1++;
    while (declaration2[size2].Stream != 0xff) size2++;

    /* If not same size then they are definitely not equal */
    if (size1 != size2)
        return FALSE;

    /* Check that all components are the same */
    if (memcmp(declaration1, declaration2, size1*sizeof(*declaration1)) == 0)
        return TRUE;

    return FALSE;
}

static HRESULT WINAPI d3dx9_mesh_CloneMesh(struct ID3DXMesh *iface, DWORD options,
        const D3DVERTEXELEMENT9 *declaration, struct IDirect3DDevice9 *device, struct ID3DXMesh **clone_mesh_out)
{
    struct d3dx9_mesh *This = impl_from_ID3DXMesh(iface);
    struct d3dx9_mesh *cloned_this;
    ID3DXMesh *clone_mesh;
    D3DVERTEXELEMENT9 orig_declaration[MAX_FVF_DECL_SIZE] = { D3DDECL_END() };
    void *data_in, *data_out;
    DWORD vertex_size;
    HRESULT hr;
    BOOL same_declaration;

    TRACE("iface %p, options %#lx, declaration %p, device %p, clone_mesh_out %p.\n",
            iface, options, declaration, device, clone_mesh_out);

    if (!clone_mesh_out)
        return D3DERR_INVALIDCALL;

    hr = iface->lpVtbl->GetDeclaration(iface, orig_declaration);
    if (FAILED(hr)) return hr;

    hr = D3DXCreateMesh(This->numfaces, This->numvertices, options & ~D3DXMESH_VB_SHARE,
                        declaration, device, &clone_mesh);
    if (FAILED(hr)) return hr;

    cloned_this = impl_from_ID3DXMesh(clone_mesh);
    vertex_size = clone_mesh->lpVtbl->GetNumBytesPerVertex(clone_mesh);
    same_declaration = declaration_equals(declaration, orig_declaration);

    if (options & D3DXMESH_VB_SHARE) {
        if (!same_declaration) {
            hr = D3DERR_INVALIDCALL;
            goto error;
        }
        IDirect3DVertexBuffer9_AddRef(This->vertex_buffer);
        /* FIXME: refactor to avoid creating a new vertex buffer */
        IDirect3DVertexBuffer9_Release(cloned_this->vertex_buffer);
        cloned_this->vertex_buffer = This->vertex_buffer;
    } else if (same_declaration) {
        hr = iface->lpVtbl->LockVertexBuffer(iface, D3DLOCK_READONLY, &data_in);
        if (FAILED(hr)) goto error;
        hr = clone_mesh->lpVtbl->LockVertexBuffer(clone_mesh, 0, &data_out);
        if (FAILED(hr)) {
            iface->lpVtbl->UnlockVertexBuffer(iface);
            goto error;
        }
        memcpy(data_out, data_in, This->numvertices * vertex_size);
        clone_mesh->lpVtbl->UnlockVertexBuffer(clone_mesh);
        iface->lpVtbl->UnlockVertexBuffer(iface);
    } else {
        hr = convert_vertex_buffer(clone_mesh, iface);
        if (FAILED(hr)) goto error;
    }

    hr = iface->lpVtbl->LockIndexBuffer(iface, D3DLOCK_READONLY, &data_in);
    if (FAILED(hr)) goto error;
    hr = clone_mesh->lpVtbl->LockIndexBuffer(clone_mesh, 0, &data_out);
    if (FAILED(hr)) {
        iface->lpVtbl->UnlockIndexBuffer(iface);
        goto error;
    }
    if ((options ^ This->options) & D3DXMESH_32BIT) {
        DWORD i;
        if (options & D3DXMESH_32BIT) {
            for (i = 0; i < This->numfaces * 3; i++)
                ((DWORD*)data_out)[i] = ((WORD*)data_in)[i];
        } else {
            for (i = 0; i < This->numfaces * 3; i++)
                ((WORD*)data_out)[i] = ((DWORD*)data_in)[i];
        }
    } else {
        memcpy(data_out, data_in, This->numfaces * 3 * (options & D3DXMESH_32BIT ? 4 : 2));
    }
    clone_mesh->lpVtbl->UnlockIndexBuffer(clone_mesh);
    iface->lpVtbl->UnlockIndexBuffer(iface);

    memcpy(cloned_this->attrib_buffer, This->attrib_buffer, This->numfaces * sizeof(*This->attrib_buffer));

    if (This->attrib_table_size)
    {
        cloned_this->attrib_table_size = This->attrib_table_size;
        cloned_this->attrib_table = malloc(This->attrib_table_size * sizeof(*This->attrib_table));
        if (!cloned_this->attrib_table) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        memcpy(cloned_this->attrib_table, This->attrib_table, This->attrib_table_size * sizeof(*This->attrib_table));
    }

    *clone_mesh_out = clone_mesh;

    return D3D_OK;
error:
    IUnknown_Release(clone_mesh);
    return hr;
}

static HRESULT WINAPI d3dx9_mesh_GetVertexBuffer(struct ID3DXMesh *iface,
        struct IDirect3DVertexBuffer9 **vertex_buffer)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p, vertex_buffer %p.\n", iface, vertex_buffer);

    if (!vertex_buffer)
        return D3DERR_INVALIDCALL;
    *vertex_buffer = mesh->vertex_buffer;
    IDirect3DVertexBuffer9_AddRef(mesh->vertex_buffer);

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_mesh_GetIndexBuffer(struct ID3DXMesh *iface,
        struct IDirect3DIndexBuffer9 **index_buffer)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p, index_buffer %p.\n", iface, index_buffer);

    if (!index_buffer)
        return D3DERR_INVALIDCALL;
    *index_buffer = mesh->index_buffer;
    IDirect3DIndexBuffer9_AddRef(mesh->index_buffer);

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_mesh_LockVertexBuffer(ID3DXMesh *iface, DWORD flags, void **data)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p, flags %#lx, data %p.\n", iface, flags, data);

    return IDirect3DVertexBuffer9_Lock(mesh->vertex_buffer, 0, 0, data, flags);
}

static HRESULT WINAPI d3dx9_mesh_UnlockVertexBuffer(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DVertexBuffer9_Unlock(mesh->vertex_buffer);
}

static HRESULT WINAPI d3dx9_mesh_LockIndexBuffer(ID3DXMesh *iface, DWORD flags, void **data)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p, flags %#lx, data %p.\n", iface, flags, data);

    return IDirect3DIndexBuffer9_Lock(mesh->index_buffer, 0, 0, data, flags);
}

static HRESULT WINAPI d3dx9_mesh_UnlockIndexBuffer(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DIndexBuffer9_Unlock(mesh->index_buffer);
}

/* FIXME: This looks just wrong, we never check *attrib_table_size before
 * copying the data. */
static HRESULT WINAPI d3dx9_mesh_GetAttributeTable(ID3DXMesh *iface,
        D3DXATTRIBUTERANGE *attrib_table, DWORD *attrib_table_size)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p, attrib_table %p, attrib_table_size %p.\n",
            iface, attrib_table, attrib_table_size);

    if (attrib_table_size)
        *attrib_table_size = mesh->attrib_table_size;

    if (attrib_table)
        memcpy(attrib_table, mesh->attrib_table, mesh->attrib_table_size * sizeof(*attrib_table));

    return D3D_OK;
}

struct edge_face
{
    struct list entry;
    DWORD v2;
    DWORD face;
};

struct edge_face_map
{
    struct list *lists;
    struct edge_face *entries;
};

/* Builds up a map of which face a new edge belongs to. That way the adjacency
 * of another edge can be looked up. An edge has an adjacent face if there
 * is an edge going in the opposite direction in the map. For example if the
 * edge (v1, v2) belongs to face 4, and there is a mapping (v2, v1)->7, then
 * face 4 and 7 are adjacent.
 *
 * Each edge might have been replaced with another edge, or none at all. There
 * is at most one edge to face mapping, i.e. an edge can only belong to one
 * face.
 */
static HRESULT init_edge_face_map(struct edge_face_map *edge_face_map, const DWORD *index_buffer,
        const DWORD *point_reps, DWORD num_faces)
{
    DWORD face, edge;
    DWORD i;

    edge_face_map->lists = malloc(3 * num_faces * sizeof(*edge_face_map->lists));
    if (!edge_face_map->lists) return E_OUTOFMEMORY;

    edge_face_map->entries = malloc(3 * num_faces * sizeof(*edge_face_map->entries));
    if (!edge_face_map->entries) return E_OUTOFMEMORY;


    /* Initialize all lists */
    for (i = 0; i < 3 * num_faces; i++)
    {
        list_init(&edge_face_map->lists[i]);
    }
    /* Build edge face mapping */
    for (face = 0; face < num_faces; face++)
    {
        for (edge = 0; edge < 3; edge++)
        {
            DWORD v1 = index_buffer[3*face + edge];
            DWORD v2 = index_buffer[3*face + (edge+1)%3];
            DWORD new_v1 = point_reps[v1]; /* What v1 has been replaced with */
            DWORD new_v2 = point_reps[v2];

            if (v1 != v2) /* Only map non-collapsed edges */
            {
                i = 3*face + edge;
                edge_face_map->entries[i].v2 = new_v2;
                edge_face_map->entries[i].face = face;
                list_add_head(&edge_face_map->lists[new_v1], &edge_face_map->entries[i].entry);
            }
        }
    }

    return D3D_OK;
}

static DWORD find_adjacent_face(struct edge_face_map *edge_face_map, DWORD vertex1, DWORD vertex2, DWORD num_faces)
{
    struct edge_face *edge_face_ptr;

    LIST_FOR_EACH_ENTRY(edge_face_ptr, &edge_face_map->lists[vertex2], struct edge_face, entry)
    {
        if (edge_face_ptr->v2 == vertex1)
            return edge_face_ptr->face;
    }

    return -1;
}

static DWORD *generate_identity_point_reps(DWORD num_vertices)
{
        DWORD *id_point_reps;
        DWORD i;

        id_point_reps = malloc(num_vertices * sizeof(*id_point_reps));
        if (!id_point_reps)
            return NULL;

        for (i = 0; i < num_vertices; i++)
        {
            id_point_reps[i] = i;
        }

        return id_point_reps;
}

static HRESULT WINAPI d3dx9_mesh_ConvertPointRepsToAdjacency(ID3DXMesh *iface,
        const DWORD *point_reps, DWORD *adjacency)
{
    HRESULT hr;
    DWORD num_faces = iface->lpVtbl->GetNumFaces(iface);
    DWORD num_vertices = iface->lpVtbl->GetNumVertices(iface);
    DWORD options = iface->lpVtbl->GetOptions(iface);
    BOOL indices_are_16_bit = !(options & D3DXMESH_32BIT);
    DWORD *ib = NULL;
    void *ib_ptr = NULL;
    DWORD face;
    DWORD edge;
    struct edge_face_map edge_face_map = {0};
    const DWORD *point_reps_ptr = NULL;
    DWORD *id_point_reps = NULL;

    TRACE("iface %p, point_reps %p, adjacency %p.\n", iface, point_reps, adjacency);

    if (!adjacency) return D3DERR_INVALIDCALL;

    if (!point_reps) /* Identity point reps */
    {
        id_point_reps = generate_identity_point_reps(num_vertices);
        if (!id_point_reps)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        point_reps_ptr = id_point_reps;
    }
    else
    {
        point_reps_ptr = point_reps;
    }

    hr = iface->lpVtbl->LockIndexBuffer(iface, D3DLOCK_READONLY, &ib_ptr);
    if (FAILED(hr)) goto cleanup;

    if (indices_are_16_bit)
    {
        /* Widen 16 bit to 32 bit */
        DWORD i;
        WORD *ib_16bit = ib_ptr;
        ib = malloc(3 * num_faces * sizeof(DWORD));
        if (!ib)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        for (i = 0; i < 3 * num_faces; i++)
        {
            ib[i] = ib_16bit[i];
        }
    }
    else
    {
        ib = ib_ptr;
    }

    hr = init_edge_face_map(&edge_face_map, ib, point_reps_ptr, num_faces);
    if (FAILED(hr)) goto cleanup;

    /* Create adjacency */
    for (face = 0; face < num_faces; face++)
    {
        for (edge = 0; edge < 3; edge++)
        {
            DWORD v1 = ib[3*face + edge];
            DWORD v2 = ib[3*face + (edge+1)%3];
            DWORD new_v1 = point_reps_ptr[v1];
            DWORD new_v2 = point_reps_ptr[v2];
            DWORD adj_face;

            adj_face = find_adjacent_face(&edge_face_map, new_v1, new_v2, num_faces);
            adjacency[3*face + edge] = adj_face;
        }
    }

    hr = D3D_OK;
cleanup:
    free(id_point_reps);
    if (indices_are_16_bit) free(ib);
    free(edge_face_map.lists);
    free(edge_face_map.entries);
    if(ib_ptr) iface->lpVtbl->UnlockIndexBuffer(iface);
    return hr;
}

/* ConvertAdjacencyToPointReps helper function.
 *
 * Goes around the edges of each face and replaces the vertices in any adjacent
 * face's edge with its own vertices(if its vertices have a lower index). This
 * way as few as possible low index vertices are shared among the faces. The
 * re-ordered index buffer is stored in new_indices.
 *
 * The vertices in a point representation must be ordered sequentially, e.g.
 * index 5 holds the index of the vertex that replaces vertex 5, i.e. if
 * vertex 5 is replaced by vertex 3 then index 5 would contain 3. If no vertex
 * replaces it, then it contains the same number as the index itself, e.g.
 * index 5 would contain 5. */
static HRESULT propagate_face_vertices(const DWORD *adjacency, DWORD *point_reps,
        const uint32_t *indices, uint32_t *new_indices, unsigned int face_idx, unsigned int face_count)
{
    unsigned int face_base = 3 * face_idx;
    unsigned int edge, opp_edge;

    for (edge = 0; edge < 3; ++edge)
    {
        unsigned int adj_face = adjacency[face_base + edge];
        unsigned int adj_face_base;
        unsigned int i;

        if (adj_face == ~0u) /* No adjacent face. */
            continue;
        else if (adj_face >= face_count)
        {
            /* This crashes on Windows. */
            WARN("Index out of bounds. Got %u, expected less than %u.\n", adj_face, face_count);
            return D3DERR_INVALIDCALL;
        }
        adj_face_base = 3 * adj_face;

        /* Find opposite edge in adjacent face. */
        for (opp_edge = 0; opp_edge < 3; ++opp_edge)
        {
            unsigned int opp_edge_index = adj_face_base + opp_edge;

            if (adjacency[opp_edge_index] == face_idx)
                break; /* Found opposite edge. */
        }

        /* Replaces vertices in opposite edge with vertices from current edge. */
        for (i = 0; i < 2; ++i)
        {
            unsigned int from = face_base + (edge + (1 - i)) % 3;
            unsigned int to = adj_face_base + (opp_edge + i) % 3;

            /* Propagate lowest index. */
            if (new_indices[to] > new_indices[from])
            {
                new_indices[to] = new_indices[from];
                point_reps[indices[to]] = new_indices[from];
            }
        }
    }

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_mesh_ConvertAdjacencyToPointReps(ID3DXMesh *iface,
        const DWORD *adjacency, DWORD *point_reps)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);
    uint16_t *indices_16bit = NULL;
    uint32_t *new_indices = NULL;
    uint32_t *indices = NULL;
    unsigned int face, i;
    HRESULT hr;

    TRACE("iface %p, adjacency %p, point_reps %p.\n", iface, adjacency, point_reps);

    if (!adjacency)
    {
        WARN("NULL adjacency.\n");
        return D3DERR_INVALIDCALL;
    }

    if (!point_reps)
    {
        WARN("NULL point_reps.\n");
        return D3DERR_INVALIDCALL;
    }

    /* Should never happen as CreateMesh does not allow meshes with 0 faces. */
    if (!mesh->numfaces)
    {
        ERR("Number of faces was zero.\n");
        return D3DERR_INVALIDCALL;
    }

    if (!(new_indices = malloc(3 * mesh->numfaces * sizeof(*indices))))
        return E_OUTOFMEMORY;

    if (mesh->options & D3DXMESH_32BIT)
    {
        if (FAILED(hr = iface->lpVtbl->LockIndexBuffer(iface, D3DLOCK_READONLY, (void **)&indices)))
            goto cleanup;
        memcpy(new_indices, indices, 3 * mesh->numfaces * sizeof(*indices));
    }
    else
    {
        /* Make a widening copy of indices_16bit into indices and new_indices
         * in order to re-use the helper function. */
        if (FAILED(hr = iface->lpVtbl->LockIndexBuffer(iface, D3DLOCK_READONLY, (void **)&indices_16bit)))
            goto cleanup;

        if (!(indices = malloc(3 * mesh->numfaces * sizeof(*indices))))
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        for (i = 0; i < 3 * mesh->numfaces; ++i)
        {
            new_indices[i] = indices_16bit[i];
            indices[i] = indices_16bit[i];
        }
    }

    /* Vertices are ordered sequentially in the point representation. */
    for (i = 0; i < mesh->numvertices; ++i)
        point_reps[i] = i;

    /* Propagate vertices with low indices so as few vertices as possible are
     * used in the mesh. */
    for (face = 0; face < mesh->numfaces; ++face)
    {
        if (FAILED(hr = propagate_face_vertices(adjacency, point_reps, indices, new_indices, face, mesh->numfaces)))
            goto cleanup;
    }
    /* Go in opposite direction to catch all face orderings. */
    for (face = 0; face < mesh->numfaces; ++face)
    {
        if (FAILED(hr = propagate_face_vertices(adjacency, point_reps, indices, new_indices,
                (mesh->numfaces - 1) - face, mesh->numfaces)))
            goto cleanup;
    }

    hr = D3D_OK;

 cleanup:
    if (mesh->options & D3DXMESH_32BIT)
    {
        if (indices)
            iface->lpVtbl->UnlockIndexBuffer(iface);
    }
    else
    {
        if (indices_16bit)
            iface->lpVtbl->UnlockIndexBuffer(iface);
        free(indices);
    }
    free(new_indices);
    return hr;
}

struct vertex_metadata {
  float key;
  DWORD vertex_index;
  DWORD first_shared_index;
};

static int __cdecl compare_vertex_keys(const void *a, const void *b)
{
    const struct vertex_metadata *left = a;
    const struct vertex_metadata *right = b;
    if (left->key == right->key)
        return 0;
    return left->key < right->key ? -1 : 1;
}

static HRESULT WINAPI d3dx9_mesh_GenerateAdjacency(ID3DXMesh *iface, float epsilon, DWORD *adjacency)
{
    struct d3dx9_mesh *This = impl_from_ID3DXMesh(iface);
    HRESULT hr;
    BYTE *vertices = NULL;
    const DWORD *indices = NULL;
    DWORD vertex_size;
    DWORD buffer_size;
    /* sort the vertices by (x + y + z) to quickly find coincident vertices */
    struct vertex_metadata *sorted_vertices;
    /* shared_indices links together identical indices in the index buffer so
     * that adjacency checks can be limited to faces sharing a vertex */
    DWORD *shared_indices = NULL;
    const FLOAT epsilon_sq = epsilon * epsilon;
    DWORD i;

    TRACE("iface %p, epsilon %.8e, adjacency %p.\n", iface, epsilon, adjacency);

    if (!adjacency)
        return D3DERR_INVALIDCALL;

    buffer_size = This->numfaces * 3 * sizeof(*shared_indices) + This->numvertices * sizeof(*sorted_vertices);
    if (!(This->options & D3DXMESH_32BIT))
        buffer_size += This->numfaces * 3 * sizeof(*indices);
    shared_indices = malloc(buffer_size);
    if (!shared_indices)
        return E_OUTOFMEMORY;
    sorted_vertices = (struct vertex_metadata*)(shared_indices + This->numfaces * 3);

    hr = iface->lpVtbl->LockVertexBuffer(iface, D3DLOCK_READONLY, (void**)&vertices);
    if (FAILED(hr)) goto cleanup;
    hr = iface->lpVtbl->LockIndexBuffer(iface, D3DLOCK_READONLY, (void**)&indices);
    if (FAILED(hr)) goto cleanup;

    if (!(This->options & D3DXMESH_32BIT)) {
        const WORD *word_indices = (const WORD*)indices;
        DWORD *dword_indices = (DWORD*)(sorted_vertices + This->numvertices);
        indices = dword_indices;
        for (i = 0; i < This->numfaces * 3; i++)
            *dword_indices++ = *word_indices++;
    }

    vertex_size = iface->lpVtbl->GetNumBytesPerVertex(iface);
    for (i = 0; i < This->numvertices; i++) {
        D3DXVECTOR3 *vertex = (D3DXVECTOR3*)(vertices + vertex_size * i);
        sorted_vertices[i].first_shared_index = -1;
        sorted_vertices[i].key = vertex->x + vertex->y + vertex->z;
        sorted_vertices[i].vertex_index = i;
    }
    for (i = 0; i < This->numfaces * 3; i++) {
        DWORD *first_shared_index = &sorted_vertices[indices[i]].first_shared_index;
        shared_indices[i] = *first_shared_index;
        *first_shared_index = i;
        adjacency[i] = -1;
    }
    qsort(sorted_vertices, This->numvertices, sizeof(*sorted_vertices), compare_vertex_keys);

    for (i = 0; i < This->numvertices; i++) {
        struct vertex_metadata *sorted_vertex_a = &sorted_vertices[i];
        D3DXVECTOR3 *vertex_a = (D3DXVECTOR3*)(vertices + sorted_vertex_a->vertex_index * vertex_size);
        DWORD shared_index_a = sorted_vertex_a->first_shared_index;

        while (shared_index_a != -1) {
            DWORD j = i;
            DWORD shared_index_b = shared_indices[shared_index_a];
            struct vertex_metadata *sorted_vertex_b = sorted_vertex_a;

            while (TRUE) {
                while (shared_index_b != -1) {
                    /* faces are adjacent if they have another coincident vertex */
                    DWORD base_a = (shared_index_a / 3) * 3;
                    DWORD base_b = (shared_index_b / 3) * 3;
                    BOOL adjacent = FALSE;
                    int k;

                    for (k = 0; k < 3; k++) {
                        if (adjacency[base_b + k] == shared_index_a / 3) {
                            adjacent = TRUE;
                            break;
                        }
                    }
                    if (!adjacent) {
                        for (k = 1; k <= 2; k++) {
                            DWORD vertex_index_a = base_a + (shared_index_a + k) % 3;
                            DWORD vertex_index_b = base_b + (shared_index_b + (3 - k)) % 3;
                            adjacent = indices[vertex_index_a] == indices[vertex_index_b];
                            if (!adjacent && epsilon >= 0.0f) {
                                D3DXVECTOR3 delta = {0.0f, 0.0f, 0.0f};
                                FLOAT length_sq;

                                D3DXVec3Subtract(&delta,
                                                 (D3DXVECTOR3*)(vertices + indices[vertex_index_a] * vertex_size),
                                                 (D3DXVECTOR3*)(vertices + indices[vertex_index_b] * vertex_size));
                                length_sq = D3DXVec3LengthSq(&delta);
                                adjacent = epsilon == 0.0f ? length_sq == 0.0f : length_sq < epsilon_sq;
                            }
                            if (adjacent) {
                                DWORD adj_a = base_a + 2 - (vertex_index_a + shared_index_a + 1) % 3;
                                DWORD adj_b = base_b + 2 - (vertex_index_b + shared_index_b + 1) % 3;
                                if (adjacency[adj_a] == -1 && adjacency[adj_b] == -1) {
                                    adjacency[adj_a] = base_b / 3;
                                    adjacency[adj_b] = base_a / 3;
                                    break;
                                }
                            }
                        }
                    }

                    shared_index_b = shared_indices[shared_index_b];
                }
                while (++j < This->numvertices) {
                    D3DXVECTOR3 *vertex_b;

                    sorted_vertex_b++;
                    if (sorted_vertex_b->key - sorted_vertex_a->key > epsilon * 3.0f) {
                        /* no more coincident vertices to try */
                        j = This->numvertices;
                        break;
                    }
                    /* check for coincidence */
                    vertex_b = (D3DXVECTOR3*)(vertices + sorted_vertex_b->vertex_index * vertex_size);
                    if (fabsf(vertex_a->x - vertex_b->x) <= epsilon &&
                        fabsf(vertex_a->y - vertex_b->y) <= epsilon &&
                        fabsf(vertex_a->z - vertex_b->z) <= epsilon)
                    {
                        break;
                    }
                }
                if (j >= This->numvertices)
                    break;
                shared_index_b = sorted_vertex_b->first_shared_index;
            }

            sorted_vertex_a->first_shared_index = shared_indices[sorted_vertex_a->first_shared_index];
            shared_index_a = sorted_vertex_a->first_shared_index;
        }
    }

    hr = D3D_OK;
cleanup:
    if (indices) iface->lpVtbl->UnlockIndexBuffer(iface);
    if (vertices) iface->lpVtbl->UnlockVertexBuffer(iface);
    free(shared_indices);
    return hr;
}

static HRESULT WINAPI d3dx9_mesh_UpdateSemantics(ID3DXMesh *iface, D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    struct d3dx9_mesh *This = impl_from_ID3DXMesh(iface);
    HRESULT hr;
    UINT vertex_declaration_size;
    int i;

    TRACE("iface %p, declaration %p.\n", iface, declaration);

    if (!declaration)
    {
        WARN("Invalid declaration. Can't use NULL declaration.\n");
        return D3DERR_INVALIDCALL;
    }

    /* New declaration must be same size as original */
    vertex_declaration_size = D3DXGetDeclVertexSize(declaration, declaration[0].Stream);
    if (vertex_declaration_size != This->vertex_declaration_size)
    {
        WARN("Invalid declaration. New vertex size does not match the original vertex size.\n");
        return D3DERR_INVALIDCALL;
    }

    /* New declaration must not contain non-zero Stream value  */
    for (i = 0; declaration[i].Stream != 0xff; i++)
    {
        if (declaration[i].Stream != 0)
        {
            WARN("Invalid declaration. New declaration contains non-zero Stream value.\n");
            return D3DERR_INVALIDCALL;
        }
    }

    This->num_elem = i + 1;
    copy_declaration(This->cached_declaration, declaration, This->num_elem);

    if (This->vertex_declaration)
        IDirect3DVertexDeclaration9_Release(This->vertex_declaration);

    /* An application can pass an invalid declaration to UpdateSemantics and
     * still expect D3D_OK (see tests). If the declaration is invalid, then
     * subsequent calls to DrawSubset will fail. This is handled by setting the
     * vertex declaration to NULL.
     *     GetDeclaration, GetNumBytesPerVertex must, however, use the new
     * invalid declaration. This is handled by them using the cached vertex
     * declaration instead of the actual vertex declaration.
     */
    hr = IDirect3DDevice9_CreateVertexDeclaration(This->device,
                                                  declaration,
                                                  &This->vertex_declaration);
    if (FAILED(hr))
    {
        WARN("Using invalid declaration. Calls to DrawSubset will fail.\n");
        This->vertex_declaration = NULL;
    }

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_mesh_LockAttributeBuffer(ID3DXMesh *iface, DWORD flags, DWORD **data)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);

    TRACE("iface %p, flags %#lx, data %p.\n", iface, flags, data);

    InterlockedIncrement(&mesh->attrib_buffer_lock_count);

    if (!(flags & D3DLOCK_READONLY))
    {
        D3DXATTRIBUTERANGE *attrib_table = mesh->attrib_table;
        mesh->attrib_table_size = 0;
        mesh->attrib_table = NULL;
        free(attrib_table);
    }

    *data = mesh->attrib_buffer;

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_mesh_UnlockAttributeBuffer(ID3DXMesh *iface)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);
    int lock_count;

    TRACE("iface %p.\n", iface);

    lock_count = InterlockedDecrement(&mesh->attrib_buffer_lock_count);
    if (lock_count < 0)
    {
        InterlockedIncrement(&mesh->attrib_buffer_lock_count);
        return D3DERR_INVALIDCALL;
    }

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_mesh_Optimize(ID3DXMesh *iface, DWORD flags, const DWORD *adjacency_in,
        DWORD *adjacency_out, DWORD *face_remap, ID3DXBuffer **vertex_remap, ID3DXMesh **opt_mesh)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE] = { D3DDECL_END() };
    ID3DXMesh *optimized_mesh;

    TRACE("iface %p, flags %#lx, adjacency_in %p, adjacency_out %p, face_remap %p, vertex_remap %p, opt_mesh %p.\n",
            iface, flags, adjacency_in, adjacency_out, face_remap, vertex_remap, opt_mesh);

    if (!opt_mesh)
        return D3DERR_INVALIDCALL;

    hr = iface->lpVtbl->GetDeclaration(iface, declaration);
    if (FAILED(hr)) return hr;

    if (FAILED(hr = iface->lpVtbl->CloneMesh(iface, mesh->options, declaration, mesh->device, &optimized_mesh)))
        return hr;

    hr = optimized_mesh->lpVtbl->OptimizeInplace(optimized_mesh, flags, adjacency_in, adjacency_out, face_remap, vertex_remap);
    if (SUCCEEDED(hr))
        *opt_mesh = optimized_mesh;
    else
        IUnknown_Release(optimized_mesh);
    return hr;
}

/* Creates a vertex_remap that removes unused vertices.
 * Indices are updated according to the vertex_remap. */
static HRESULT compact_mesh(struct d3dx9_mesh *This, DWORD *indices,
        DWORD *new_num_vertices, ID3DXBuffer **vertex_remap)
{
    HRESULT hr;
    DWORD *vertex_remap_ptr;
    DWORD num_used_vertices;
    DWORD i;

    hr = D3DXCreateBuffer(This->numvertices * sizeof(DWORD), vertex_remap);
    if (FAILED(hr)) return hr;
    vertex_remap_ptr = ID3DXBuffer_GetBufferPointer(*vertex_remap);

    for (i = 0; i < This->numfaces * 3; i++)
        vertex_remap_ptr[indices[i]] = 1;

    /* create old->new vertex mapping */
    num_used_vertices = 0;
    for (i = 0; i < This->numvertices; i++) {
        if (vertex_remap_ptr[i])
            vertex_remap_ptr[i] = num_used_vertices++;
        else
            vertex_remap_ptr[i] = -1;
    }
    /* convert indices */
    for (i = 0; i < This->numfaces * 3; i++)
        indices[i] = vertex_remap_ptr[indices[i]];

    /* create new->old vertex mapping */
    num_used_vertices = 0;
    for (i = 0; i < This->numvertices; i++) {
        if (vertex_remap_ptr[i] != -1)
            vertex_remap_ptr[num_used_vertices++] = i;
    }
    for (i = num_used_vertices; i < This->numvertices; i++)
        vertex_remap_ptr[i] = -1;

    *new_num_vertices = num_used_vertices;

    return D3D_OK;
}

/* count the number of unique attribute values in a sorted attribute buffer */
static DWORD count_attributes(const DWORD *attrib_buffer, DWORD numfaces)
{
    DWORD last_attribute = attrib_buffer[0];
    DWORD attrib_table_size = 1;
    DWORD i;
    for (i = 1; i < numfaces; i++) {
        if (attrib_buffer[i] != last_attribute) {
            last_attribute = attrib_buffer[i];
            attrib_table_size++;
        }
    }
    return attrib_table_size;
}

static void fill_attribute_table(DWORD *attrib_buffer, DWORD numfaces, void *indices,
                                 BOOL is_32bit_indices, D3DXATTRIBUTERANGE *attrib_table)
{
    DWORD attrib_table_size = 0;
    DWORD last_attribute = attrib_buffer[0];
    DWORD min_vertex, max_vertex;
    DWORD i;

    attrib_table[0].AttribId = last_attribute;
    attrib_table[0].FaceStart = 0;
    min_vertex = (DWORD)-1;
    max_vertex = 0;
    for (i = 0; i < numfaces; i++) {
        DWORD j;

        if (attrib_buffer[i] != last_attribute) {
            last_attribute = attrib_buffer[i];
            attrib_table[attrib_table_size].FaceCount = i - attrib_table[attrib_table_size].FaceStart;
            attrib_table[attrib_table_size].VertexStart = min_vertex;
            attrib_table[attrib_table_size].VertexCount = max_vertex - min_vertex + 1;
            attrib_table_size++;
            attrib_table[attrib_table_size].AttribId = attrib_buffer[i];
            attrib_table[attrib_table_size].FaceStart = i;
            min_vertex = (DWORD)-1;
            max_vertex = 0;
        }
        for (j = 0; j < 3; j++) {
            DWORD vertex_index = is_32bit_indices ? ((DWORD*)indices)[i * 3 + j] : ((WORD*)indices)[i * 3 + j];
            if (vertex_index < min_vertex)
                min_vertex = vertex_index;
            if (vertex_index > max_vertex)
                max_vertex = vertex_index;
        }
    }
    attrib_table[attrib_table_size].FaceCount = i - attrib_table[attrib_table_size].FaceStart;
    attrib_table[attrib_table_size].VertexStart = min_vertex;
    attrib_table[attrib_table_size].VertexCount = max_vertex - min_vertex + 1;
    attrib_table_size++;
}

static int __cdecl attrib_entry_compare(const void *a, const void *b)
{
    const DWORD *ptr_a = *(const DWORD **)a;
    const DWORD *ptr_b = *(const DWORD **)b;
    int delta = *ptr_a - *ptr_b;

    if (delta)
        return delta;

    delta = ptr_a - ptr_b; /* for stable sort */
    return delta;
}

/* Create face_remap, a new attribute buffer for attribute sort optimization. */
static HRESULT remap_faces_for_attrsort(struct d3dx9_mesh *This, const DWORD *indices,
        DWORD *attrib_buffer, DWORD **sorted_attrib_buffer, DWORD **face_remap)
{
    DWORD **sorted_attrib_ptr_buffer = NULL;
    DWORD i;

    sorted_attrib_ptr_buffer = malloc(This->numfaces * sizeof(*sorted_attrib_ptr_buffer));
    if (!sorted_attrib_ptr_buffer)
        return E_OUTOFMEMORY;

    *face_remap = malloc(This->numfaces * sizeof(**face_remap));
    if (!*face_remap)
    {
        free(sorted_attrib_ptr_buffer);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < This->numfaces; i++)
        sorted_attrib_ptr_buffer[i] = &attrib_buffer[i];
    qsort(sorted_attrib_ptr_buffer, This->numfaces, sizeof(*sorted_attrib_ptr_buffer), attrib_entry_compare);

    for (i = 0; i < This->numfaces; i++)
    {
        DWORD old_face = sorted_attrib_ptr_buffer[i] - attrib_buffer;
        (*face_remap)[old_face] = i;
    }

    /* overwrite sorted_attrib_ptr_buffer with the values themselves */
    *sorted_attrib_buffer = (DWORD*)sorted_attrib_ptr_buffer;
    for (i = 0; i < This->numfaces; i++)
        (*sorted_attrib_buffer)[(*face_remap)[i]] = attrib_buffer[i];

    return D3D_OK;
}

static DWORD adjacency_remap(DWORD *face_remap, DWORD index)
{
    if (index == 0xffffffff)
        return index;
    return face_remap[index];
}

static HRESULT WINAPI d3dx9_mesh_OptimizeInplace(ID3DXMesh *iface, DWORD flags, const DWORD *adjacency_in,
        DWORD *adjacency_out, DWORD *face_remap_out, ID3DXBuffer **vertex_remap_out)
{
    struct d3dx9_mesh *This = impl_from_ID3DXMesh(iface);
    void *indices = NULL;
    DWORD *attrib_buffer = NULL;
    HRESULT hr;
    ID3DXBuffer *vertex_remap = NULL;
    DWORD *face_remap = NULL; /* old -> new mapping */
    DWORD *dword_indices = NULL;
    DWORD new_num_vertices = 0;
    DWORD new_num_alloc_vertices = 0;
    IDirect3DVertexBuffer9 *vertex_buffer = NULL;
    DWORD *sorted_attrib_buffer = NULL;
    DWORD i;

    TRACE("iface %p, flags %#lx, adjacency_in %p, adjacency_out %p, face_remap_out %p, vertex_remap_out %p.\n",
            iface, flags, adjacency_in, adjacency_out, face_remap_out, vertex_remap_out);

    if (!flags)
        return D3DERR_INVALIDCALL;
    if (!adjacency_in && (flags & (D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_STRIPREORDER)))
        return D3DERR_INVALIDCALL;
    if ((flags & (D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_STRIPREORDER)) == (D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_STRIPREORDER))
        return D3DERR_INVALIDCALL;

    if (flags & (D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_STRIPREORDER))
    {
        if (flags & D3DXMESHOPT_VERTEXCACHE)
            FIXME("D3DXMESHOPT_VERTEXCACHE not implemented.\n");
        if (flags & D3DXMESHOPT_STRIPREORDER)
            FIXME("D3DXMESHOPT_STRIPREORDER not implemented.\n");
        return E_NOTIMPL;
    }

    hr = iface->lpVtbl->LockIndexBuffer(iface, 0, &indices);
    if (FAILED(hr)) goto cleanup;

    dword_indices = malloc(This->numfaces * 3 * sizeof(DWORD));
    if (!dword_indices) return E_OUTOFMEMORY;
    if (This->options & D3DXMESH_32BIT) {
        memcpy(dword_indices, indices, This->numfaces * 3 * sizeof(DWORD));
    } else {
        WORD *word_indices = indices;
        for (i = 0; i < This->numfaces * 3; i++)
            dword_indices[i] = *word_indices++;
    }

    if ((flags & (D3DXMESHOPT_COMPACT | D3DXMESHOPT_IGNOREVERTS | D3DXMESHOPT_ATTRSORT)) == D3DXMESHOPT_COMPACT)
    {
        new_num_alloc_vertices = This->numvertices;
        hr = compact_mesh(This, dword_indices, &new_num_vertices, &vertex_remap);
        if (FAILED(hr)) goto cleanup;
    } else if (flags & D3DXMESHOPT_ATTRSORT) {
        if (!(flags & D3DXMESHOPT_IGNOREVERTS))
            FIXME("D3DXMESHOPT_ATTRSORT vertex reordering not implemented.\n");

        hr = iface->lpVtbl->LockAttributeBuffer(iface, 0, &attrib_buffer);
        if (FAILED(hr)) goto cleanup;

        hr = remap_faces_for_attrsort(This, dword_indices, attrib_buffer, &sorted_attrib_buffer, &face_remap);
        if (FAILED(hr)) goto cleanup;
    }

    if (vertex_remap)
    {
        /* reorder the vertices using vertex_remap */
        D3DVERTEXBUFFER_DESC vertex_desc;
        DWORD *vertex_remap_ptr = ID3DXBuffer_GetBufferPointer(vertex_remap);
        DWORD vertex_size = iface->lpVtbl->GetNumBytesPerVertex(iface);
        BYTE *orig_vertices;
        BYTE *new_vertices;

        hr = IDirect3DVertexBuffer9_GetDesc(This->vertex_buffer, &vertex_desc);
        if (FAILED(hr)) goto cleanup;

        hr = IDirect3DDevice9_CreateVertexBuffer(This->device, new_num_alloc_vertices * vertex_size,
                vertex_desc.Usage, This->fvf, vertex_desc.Pool, &vertex_buffer, NULL);
        if (FAILED(hr)) goto cleanup;

        hr = IDirect3DVertexBuffer9_Lock(This->vertex_buffer, 0, 0, (void**)&orig_vertices, D3DLOCK_READONLY);
        if (FAILED(hr)) goto cleanup;

        hr = IDirect3DVertexBuffer9_Lock(vertex_buffer, 0, 0, (void**)&new_vertices, 0);
        if (FAILED(hr)) {
            IDirect3DVertexBuffer9_Unlock(This->vertex_buffer);
            goto cleanup;
        }

        for (i = 0; i < new_num_vertices; i++)
            memcpy(new_vertices + i * vertex_size, orig_vertices + vertex_remap_ptr[i] * vertex_size, vertex_size);

        IDirect3DVertexBuffer9_Unlock(This->vertex_buffer);
        IDirect3DVertexBuffer9_Unlock(vertex_buffer);
    } else if (vertex_remap_out) {
        DWORD *vertex_remap_ptr;

        hr = D3DXCreateBuffer(This->numvertices * sizeof(DWORD), &vertex_remap);
        if (FAILED(hr)) goto cleanup;
        vertex_remap_ptr = ID3DXBuffer_GetBufferPointer(vertex_remap);
        for (i = 0; i < This->numvertices; i++)
            *vertex_remap_ptr++ = i;
    }

    if (flags & D3DXMESHOPT_ATTRSORT)
    {
        D3DXATTRIBUTERANGE *attrib_table;
        DWORD attrib_table_size;

        attrib_table_size = count_attributes(sorted_attrib_buffer, This->numfaces);
        attrib_table = malloc(attrib_table_size * sizeof(*attrib_table));
        if (!attrib_table) {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        memcpy(attrib_buffer, sorted_attrib_buffer, This->numfaces * sizeof(*attrib_buffer));

        /* reorder the indices using face_remap */
        if (This->options & D3DXMESH_32BIT) {
            for (i = 0; i < This->numfaces; i++)
                memcpy((DWORD*)indices + face_remap[i] * 3, dword_indices + i * 3, 3 * sizeof(DWORD));
        } else {
            WORD *word_indices = indices;
            for (i = 0; i < This->numfaces; i++) {
                DWORD new_pos = face_remap[i] * 3;
                DWORD old_pos = i * 3;
                word_indices[new_pos++] = dword_indices[old_pos++];
                word_indices[new_pos++] = dword_indices[old_pos++];
                word_indices[new_pos] = dword_indices[old_pos];
            }
        }

        fill_attribute_table(attrib_buffer, This->numfaces, indices,
                             This->options & D3DXMESH_32BIT, attrib_table);

        free(This->attrib_table);
        This->attrib_table = attrib_table;
        This->attrib_table_size = attrib_table_size;
    } else {
        if (This->options & D3DXMESH_32BIT) {
            memcpy(indices, dword_indices, This->numfaces * 3 * sizeof(DWORD));
        } else {
            WORD *word_indices = indices;
            for (i = 0; i < This->numfaces * 3; i++)
                *word_indices++ = dword_indices[i];
        }
    }

    if (adjacency_out) {
        if (face_remap) {
            for (i = 0; i < This->numfaces; i++) {
                DWORD old_pos = i * 3;
                DWORD new_pos = face_remap[i] * 3;

                adjacency_out[new_pos++] = adjacency_remap(face_remap, adjacency_in[old_pos++]);
                adjacency_out[new_pos++] = adjacency_remap(face_remap, adjacency_in[old_pos++]);
                adjacency_out[new_pos] = adjacency_remap(face_remap, adjacency_in[old_pos]);
            }
        } else {
            memcpy(adjacency_out, adjacency_in, This->numfaces * 3 * sizeof(*adjacency_out));
        }
    }
    if (face_remap_out) {
        if (face_remap) {
            for (i = 0; i < This->numfaces; i++)
                face_remap_out[face_remap[i]] = i;
        } else {
            for (i = 0; i < This->numfaces; i++)
                face_remap_out[i] = i;
        }
    }
    if (vertex_remap_out)
        *vertex_remap_out = vertex_remap;
    vertex_remap = NULL;

    if (vertex_buffer) {
        IDirect3DVertexBuffer9_Release(This->vertex_buffer);
        This->vertex_buffer = vertex_buffer;
        vertex_buffer = NULL;
        This->numvertices = new_num_vertices;
    }

    hr = D3D_OK;
cleanup:
    free(sorted_attrib_buffer);
    free(face_remap);
    free(dword_indices);
    if (vertex_remap) ID3DXBuffer_Release(vertex_remap);
    if (vertex_buffer) IDirect3DVertexBuffer9_Release(vertex_buffer);
    if (attrib_buffer) iface->lpVtbl->UnlockAttributeBuffer(iface);
    if (indices) iface->lpVtbl->UnlockIndexBuffer(iface);
    return hr;
}

static HRESULT WINAPI d3dx9_mesh_SetAttributeTable(ID3DXMesh *iface,
        const D3DXATTRIBUTERANGE *attrib_table, DWORD attrib_table_size)
{
    struct d3dx9_mesh *mesh = impl_from_ID3DXMesh(iface);
    D3DXATTRIBUTERANGE *new_table = NULL;

    TRACE("iface %p, attrib_table %p, attrib_table_size %lu.\n", iface, attrib_table, attrib_table_size);

    if (attrib_table_size) {
        size_t size = attrib_table_size * sizeof(*attrib_table);

        new_table = malloc(size);
        if (!new_table)
            return E_OUTOFMEMORY;

        CopyMemory(new_table, attrib_table, size);
    } else if (attrib_table) {
        return D3DERR_INVALIDCALL;
    }
    free(mesh->attrib_table);
    mesh->attrib_table = new_table;
    mesh->attrib_table_size = attrib_table_size;

    return D3D_OK;
}

static const struct ID3DXMeshVtbl D3DXMesh_Vtbl =
{
    d3dx9_mesh_QueryInterface,
    d3dx9_mesh_AddRef,
    d3dx9_mesh_Release,
    d3dx9_mesh_DrawSubset,
    d3dx9_mesh_GetNumFaces,
    d3dx9_mesh_GetNumVertices,
    d3dx9_mesh_GetFVF,
    d3dx9_mesh_GetDeclaration,
    d3dx9_mesh_GetNumBytesPerVertex,
    d3dx9_mesh_GetOptions,
    d3dx9_mesh_GetDevice,
    d3dx9_mesh_CloneMeshFVF,
    d3dx9_mesh_CloneMesh,
    d3dx9_mesh_GetVertexBuffer,
    d3dx9_mesh_GetIndexBuffer,
    d3dx9_mesh_LockVertexBuffer,
    d3dx9_mesh_UnlockVertexBuffer,
    d3dx9_mesh_LockIndexBuffer,
    d3dx9_mesh_UnlockIndexBuffer,
    d3dx9_mesh_GetAttributeTable,
    d3dx9_mesh_ConvertPointRepsToAdjacency,
    d3dx9_mesh_ConvertAdjacencyToPointReps,
    d3dx9_mesh_GenerateAdjacency,
    d3dx9_mesh_UpdateSemantics,
    d3dx9_mesh_LockAttributeBuffer,
    d3dx9_mesh_UnlockAttributeBuffer,
    d3dx9_mesh_Optimize,
    d3dx9_mesh_OptimizeInplace,
    d3dx9_mesh_SetAttributeTable,
};


/* Algorithm taken from the article: An Efficient and Robust Ray-Box Intersection Algorithm
Amy Williams             University of Utah
Steve Barrus             University of Utah
R. Keith Morley          University of Utah
Peter Shirley            University of Utah

International Conference on Computer Graphics and Interactive Techniques  archive
ACM SIGGRAPH 2005 Courses
Los Angeles, California

This algorithm is free of patents or of copyrights, as confirmed by Peter Shirley himself.

Algorithm: Consider the box as the intersection of three slabs. Clip the ray
against each slab, if there's anything left of the ray after we're
done we've got an intersection of the ray with the box. */
BOOL WINAPI D3DXBoxBoundProbe(const D3DXVECTOR3 *pmin, const D3DXVECTOR3 *pmax,
        const D3DXVECTOR3 *prayposition, const D3DXVECTOR3 *praydirection)
{
    FLOAT div, tmin, tmax, tymin, tymax, tzmin, tzmax;

    div = 1.0f / praydirection->x;
    if ( div >= 0.0f )
    {
        tmin = ( pmin->x - prayposition->x ) * div;
        tmax = ( pmax->x - prayposition->x ) * div;
    }
    else
    {
        tmin = ( pmax->x - prayposition->x ) * div;
        tmax = ( pmin->x - prayposition->x ) * div;
    }

    if ( tmax < 0.0f ) return FALSE;

    div = 1.0f / praydirection->y;
    if ( div >= 0.0f )
    {
        tymin = ( pmin->y - prayposition->y ) * div;
        tymax = ( pmax->y - prayposition->y ) * div;
    }
    else
    {
        tymin = ( pmax->y - prayposition->y ) * div;
        tymax = ( pmin->y - prayposition->y ) * div;
    }

    if ( ( tymax < 0.0f ) || ( tmin > tymax ) || ( tymin > tmax ) ) return FALSE;

    if ( tymin > tmin ) tmin = tymin;
    if ( tymax < tmax ) tmax = tymax;

    div = 1.0f / praydirection->z;
    if ( div >= 0.0f )
    {
        tzmin = ( pmin->z - prayposition->z ) * div;
        tzmax = ( pmax->z - prayposition->z ) * div;
    }
    else
    {
        tzmin = ( pmax->z - prayposition->z ) * div;
        tzmax = ( pmin->z - prayposition->z ) * div;
    }

    if ( (tzmax < 0.0f ) || ( tmin > tzmax ) || ( tzmin > tmax ) ) return FALSE;

    return TRUE;
}

HRESULT WINAPI D3DXComputeBoundingBox(const D3DXVECTOR3 *pfirstposition,
        DWORD numvertices, DWORD dwstride, D3DXVECTOR3 *pmin, D3DXVECTOR3 *pmax)
{
    D3DXVECTOR3 vec;
    unsigned int i;

    if( !pfirstposition || !pmin || !pmax ) return D3DERR_INVALIDCALL;

    *pmin = *pfirstposition;
    *pmax = *pmin;

    for(i=0; i<numvertices; i++)
    {
        vec = *( (const D3DXVECTOR3*)((const char*)pfirstposition + dwstride * i) );

        if ( vec.x < pmin->x ) pmin->x = vec.x;
        if ( vec.x > pmax->x ) pmax->x = vec.x;

        if ( vec.y < pmin->y ) pmin->y = vec.y;
        if ( vec.y > pmax->y ) pmax->y = vec.y;

        if ( vec.z < pmin->z ) pmin->z = vec.z;
        if ( vec.z > pmax->z ) pmax->z = vec.z;
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXComputeBoundingSphere(const D3DXVECTOR3 *pfirstposition,
        DWORD numvertices, DWORD dwstride, D3DXVECTOR3 *pcenter, float *pradius)
{
    D3DXVECTOR3 temp;
    FLOAT d;
    unsigned int i;

    if( !pfirstposition || !pcenter || !pradius ) return D3DERR_INVALIDCALL;

    temp.x = 0.0f;
    temp.y = 0.0f;
    temp.z = 0.0f;
    *pradius = 0.0f;

    for(i=0; i<numvertices; i++)
        D3DXVec3Add(&temp, &temp, (const D3DXVECTOR3*)((const char*)pfirstposition + dwstride * i));

    D3DXVec3Scale(pcenter, &temp, 1.0f / numvertices);

    for(i=0; i<numvertices; i++)
    {
        d = D3DXVec3Length(D3DXVec3Subtract(&temp, (const D3DXVECTOR3*)((const char*)pfirstposition + dwstride * i), pcenter));
        if ( d > *pradius ) *pradius = d;
    }
    return D3D_OK;
}

static void append_decl_element(D3DVERTEXELEMENT9 *declaration, UINT *idx, UINT *offset,
        D3DDECLTYPE type, D3DDECLUSAGE usage, UINT usage_idx)
{
    declaration[*idx].Stream = 0;
    declaration[*idx].Offset = *offset;
    declaration[*idx].Type = type;
    declaration[*idx].Method = D3DDECLMETHOD_DEFAULT;
    declaration[*idx].Usage = usage;
    declaration[*idx].UsageIndex = usage_idx;

    *offset += d3dx_decltype_size[type];
    ++(*idx);
}

/*************************************************************************
 * D3DXDeclaratorFromFVF
 */
HRESULT WINAPI D3DXDeclaratorFromFVF(DWORD fvf, D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    static const D3DVERTEXELEMENT9 end_element = D3DDECL_END();
    DWORD tex_count = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    unsigned int offset = 0;
    unsigned int idx = 0;
    unsigned int i;

    TRACE("fvf %#lx, declaration %p.\n", fvf, declaration);

    if (fvf & (D3DFVF_RESERVED0 | D3DFVF_RESERVED2)) return D3DERR_INVALIDCALL;

    if (fvf & D3DFVF_POSITION_MASK)
    {
        BOOL has_blend = (fvf & D3DFVF_XYZB5) >= D3DFVF_XYZB1;
        DWORD blend_count = 1 + (((fvf & D3DFVF_XYZB5) - D3DFVF_XYZB1) >> 1);
        BOOL has_blend_idx = (fvf & D3DFVF_LASTBETA_D3DCOLOR) || (fvf & D3DFVF_LASTBETA_UBYTE4);

        if (has_blend_idx) --blend_count;

        if ((fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZW
                || (has_blend && blend_count > 4))
            return D3DERR_INVALIDCALL;

        if ((fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
            append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_POSITIONT, 0);
        else
            append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);

        if (has_blend)
        {
            switch (blend_count)
            {
                 case 0:
                    break;
                 case 1:
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_BLENDWEIGHT, 0);
                    break;
                 case 2:
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_BLENDWEIGHT, 0);
                    break;
                 case 3:
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_BLENDWEIGHT, 0);
                    break;
                 case 4:
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_BLENDWEIGHT, 0);
                    break;
                 default:
                     ERR("Invalid blend count %lu.\n", blend_count);
                     break;
            }

            if (has_blend_idx)
            {
                if (fvf & D3DFVF_LASTBETA_UBYTE4)
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_UBYTE4, D3DDECLUSAGE_BLENDINDICES, 0);
                else if (fvf & D3DFVF_LASTBETA_D3DCOLOR)
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_BLENDINDICES, 0);
            }
        }
    }

    if (fvf & D3DFVF_NORMAL)
        append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);
    if (fvf & D3DFVF_PSIZE)
        append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_PSIZE, 0);
    if (fvf & D3DFVF_DIFFUSE)
        append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0);
    if (fvf & D3DFVF_SPECULAR)
        append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 1);

    for (i = 0; i < tex_count; ++i)
    {
        switch ((fvf >> (16 + 2 * i)) & 0x03)
        {
            case D3DFVF_TEXTUREFORMAT1:
                append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_TEXCOORD, i);
                break;
            case D3DFVF_TEXTUREFORMAT2:
                append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, i);
                break;
            case D3DFVF_TEXTUREFORMAT3:
                append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TEXCOORD, i);
                break;
            case D3DFVF_TEXTUREFORMAT4:
                append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_TEXCOORD, i);
                break;
        }
    }

    declaration[idx] = end_element;

    return D3D_OK;
}

/*************************************************************************
 * D3DXFVFFromDeclarator
 */
HRESULT WINAPI D3DXFVFFromDeclarator(const D3DVERTEXELEMENT9 *declaration, DWORD *fvf)
{
    unsigned int i = 0, texture, offset;

    TRACE("(%p, %p)\n", declaration, fvf);

    *fvf = 0;
    if (declaration[0].Type == D3DDECLTYPE_FLOAT3 && declaration[0].Usage == D3DDECLUSAGE_POSITION)
    {
        if ((declaration[1].Type == D3DDECLTYPE_FLOAT4 && declaration[1].Usage == D3DDECLUSAGE_BLENDWEIGHT &&
             declaration[1].UsageIndex == 0) &&
            (declaration[2].Type == D3DDECLTYPE_FLOAT1 && declaration[2].Usage == D3DDECLUSAGE_BLENDINDICES &&
             declaration[2].UsageIndex == 0))
        {
            return D3DERR_INVALIDCALL;
        }
        else if ((declaration[1].Type == D3DDECLTYPE_UBYTE4 || declaration[1].Type == D3DDECLTYPE_D3DCOLOR) &&
                 declaration[1].Usage == D3DDECLUSAGE_BLENDINDICES && declaration[1].UsageIndex == 0)
        {
            if (declaration[1].Type == D3DDECLTYPE_UBYTE4)
            {
                *fvf |= D3DFVF_XYZB1 | D3DFVF_LASTBETA_UBYTE4;
            }
            else
            {
                *fvf |= D3DFVF_XYZB1 | D3DFVF_LASTBETA_D3DCOLOR;
            }
            i = 2;
        }
        else if (declaration[1].Type <= D3DDECLTYPE_FLOAT4 && declaration[1].Usage == D3DDECLUSAGE_BLENDWEIGHT &&
                 declaration[1].UsageIndex == 0)
        {
            if ((declaration[2].Type == D3DDECLTYPE_UBYTE4 || declaration[2].Type == D3DDECLTYPE_D3DCOLOR) &&
                declaration[2].Usage == D3DDECLUSAGE_BLENDINDICES && declaration[2].UsageIndex == 0)
            {
                if (declaration[2].Type == D3DDECLTYPE_UBYTE4)
                {
                    *fvf |= D3DFVF_LASTBETA_UBYTE4;
                }
                else
                {
                    *fvf |= D3DFVF_LASTBETA_D3DCOLOR;
                }
                switch (declaration[1].Type)
                {
                    case D3DDECLTYPE_FLOAT1: *fvf |= D3DFVF_XYZB2; break;
                    case D3DDECLTYPE_FLOAT2: *fvf |= D3DFVF_XYZB3; break;
                    case D3DDECLTYPE_FLOAT3: *fvf |= D3DFVF_XYZB4; break;
                    case D3DDECLTYPE_FLOAT4: *fvf |= D3DFVF_XYZB5; break;
                }
                i = 3;
            }
            else
            {
                switch (declaration[1].Type)
                {
                    case D3DDECLTYPE_FLOAT1: *fvf |= D3DFVF_XYZB1; break;
                    case D3DDECLTYPE_FLOAT2: *fvf |= D3DFVF_XYZB2; break;
                    case D3DDECLTYPE_FLOAT3: *fvf |= D3DFVF_XYZB3; break;
                    case D3DDECLTYPE_FLOAT4: *fvf |= D3DFVF_XYZB4; break;
                }
                i = 2;
            }
        }
        else
        {
            *fvf |= D3DFVF_XYZ;
            i = 1;
        }
    }
    else if (declaration[0].Type == D3DDECLTYPE_FLOAT4 && declaration[0].Usage == D3DDECLUSAGE_POSITIONT &&
             declaration[0].UsageIndex == 0)
    {
        *fvf |= D3DFVF_XYZRHW;
        i = 1;
    }

    if (declaration[i].Type == D3DDECLTYPE_FLOAT3 && declaration[i].Usage == D3DDECLUSAGE_NORMAL)
    {
        *fvf |= D3DFVF_NORMAL;
        i++;
    }
    if (declaration[i].Type == D3DDECLTYPE_FLOAT1 && declaration[i].Usage == D3DDECLUSAGE_PSIZE &&
        declaration[i].UsageIndex == 0)
    {
        *fvf |= D3DFVF_PSIZE;
        i++;
    }
    if (declaration[i].Type == D3DDECLTYPE_D3DCOLOR && declaration[i].Usage == D3DDECLUSAGE_COLOR &&
        declaration[i].UsageIndex == 0)
    {
        *fvf |= D3DFVF_DIFFUSE;
        i++;
    }
    if (declaration[i].Type == D3DDECLTYPE_D3DCOLOR && declaration[i].Usage == D3DDECLUSAGE_COLOR &&
        declaration[i].UsageIndex == 1)
    {
        *fvf |= D3DFVF_SPECULAR;
        i++;
    }

    for (texture = 0; texture < D3DDP_MAXTEXCOORD; i++, texture++)
    {
        if (declaration[i].Stream == 0xFF)
        {
            break;
        }
        else if (declaration[i].Type == D3DDECLTYPE_FLOAT1 && declaration[i].Usage == D3DDECLUSAGE_TEXCOORD &&
                 declaration[i].UsageIndex == texture)
        {
            *fvf |= D3DFVF_TEXCOORDSIZE1(declaration[i].UsageIndex);
        }
        else if (declaration[i].Type == D3DDECLTYPE_FLOAT2 && declaration[i].Usage == D3DDECLUSAGE_TEXCOORD &&
                 declaration[i].UsageIndex == texture)
        {
            *fvf |= D3DFVF_TEXCOORDSIZE2(declaration[i].UsageIndex);
        }
        else if (declaration[i].Type == D3DDECLTYPE_FLOAT3 && declaration[i].Usage == D3DDECLUSAGE_TEXCOORD &&
                 declaration[i].UsageIndex == texture)
        {
            *fvf |= D3DFVF_TEXCOORDSIZE3(declaration[i].UsageIndex);
        }
        else if (declaration[i].Type == D3DDECLTYPE_FLOAT4 && declaration[i].Usage == D3DDECLUSAGE_TEXCOORD &&
                 declaration[i].UsageIndex == texture)
        {
            *fvf |= D3DFVF_TEXCOORDSIZE4(declaration[i].UsageIndex);
        }
        else
        {
            return D3DERR_INVALIDCALL;
        }
    }

    *fvf |= (texture << D3DFVF_TEXCOUNT_SHIFT);

    for (offset = 0, i = 0; declaration[i].Stream != 0xFF;
         offset += d3dx_decltype_size[declaration[i].Type], i++)
    {
        if (declaration[i].Offset != offset)
        {
            return D3DERR_INVALIDCALL;
        }
    }

    return D3D_OK;
}

/*************************************************************************
 * D3DXGetFVFVertexSize
 */
static UINT Get_TexCoord_Size_From_FVF(DWORD FVF, int tex_num)
{
    return (((((FVF) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1);
}

UINT WINAPI D3DXGetFVFVertexSize(DWORD FVF)
{
    DWORD size = 0;
    UINT i;
    UINT numTextures = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

    if (FVF & D3DFVF_NORMAL) size += sizeof(D3DXVECTOR3);
    if (FVF & D3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (FVF & D3DFVF_SPECULAR) size += sizeof(DWORD);
    if (FVF & D3DFVF_PSIZE) size += sizeof(DWORD);

    switch (FVF & D3DFVF_POSITION_MASK)
    {
        case D3DFVF_XYZ:    size += sizeof(D3DXVECTOR3); break;
        case D3DFVF_XYZRHW: size += 4 * sizeof(FLOAT); break;
        case D3DFVF_XYZB1:  size += 4 * sizeof(FLOAT); break;
        case D3DFVF_XYZB2:  size += 5 * sizeof(FLOAT); break;
        case D3DFVF_XYZB3:  size += 6 * sizeof(FLOAT); break;
        case D3DFVF_XYZB4:  size += 7 * sizeof(FLOAT); break;
        case D3DFVF_XYZB5:  size += 8 * sizeof(FLOAT); break;
        case D3DFVF_XYZW:   size += 4 * sizeof(FLOAT); break;
    }

    for (i = 0; i < numTextures; i++)
    {
        size += Get_TexCoord_Size_From_FVF(FVF, i) * sizeof(FLOAT);
    }

    return size;
}

/*************************************************************************
 * D3DXGetDeclVertexSize
 */
UINT WINAPI D3DXGetDeclVertexSize(const D3DVERTEXELEMENT9 *decl, DWORD stream_idx)
{
    const D3DVERTEXELEMENT9 *element;
    UINT size = 0;

    TRACE("decl %p, stream_idx %lu.\n", decl, stream_idx);

    if (!decl) return 0;

    for (element = decl; element->Stream != 0xff; ++element)
    {
        UINT type_size;

        if (element->Stream != stream_idx) continue;

        if (element->Type >= ARRAY_SIZE(d3dx_decltype_size))
        {
            FIXME("Unhandled element type %#x, size will be incorrect.\n", element->Type);
            continue;
        }

        type_size = d3dx_decltype_size[element->Type];
        if (element->Offset + type_size > size) size = element->Offset + type_size;
    }

    return size;
}

/*************************************************************************
 * D3DXGetDeclLength
 */
UINT WINAPI D3DXGetDeclLength(const D3DVERTEXELEMENT9 *decl)
{
    const D3DVERTEXELEMENT9 *element;

    TRACE("decl %p\n", decl);

    /* null decl results in exception on Windows XP */

    for (element = decl; element->Stream != 0xff; ++element);

    return element - decl;
}

BOOL WINAPI D3DXIntersectTri(const D3DXVECTOR3 *p0, const D3DXVECTOR3 *p1, const D3DXVECTOR3 *p2,
        const D3DXVECTOR3 *praypos, const D3DXVECTOR3 *praydir, float *pu, float *pv, float *pdist)
{
    D3DXMATRIX m;
    D3DXVECTOR4 vec;

    TRACE("p0 %p, p1 %p, p2 %p, praypos %p, praydir %p, pu %p, pv %p, pdist %p.\n",
            p0, p1, p2, praypos, praydir, pu, pv, pdist);

    m.m[0][0] = p1->x - p0->x;
    m.m[1][0] = p2->x - p0->x;
    m.m[2][0] = -praydir->x;
    m.m[3][0] = 0.0f;
    m.m[0][1] = p1->y - p0->y;
    m.m[1][1] = p2->y - p0->y;
    m.m[2][1] = -praydir->y;
    m.m[3][1] = 0.0f;
    m.m[0][2] = p1->z - p0->z;
    m.m[1][2] = p2->z - p0->z;
    m.m[2][2] = -praydir->z;
    m.m[3][2] = 0.0f;
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = 0.0f;
    m.m[3][3] = 1.0f;

    vec.x = praypos->x - p0->x;
    vec.y = praypos->y - p0->y;
    vec.z = praypos->z - p0->z;
    vec.w = 0.0f;

    if ( D3DXMatrixInverse(&m, NULL, &m) )
    {
        D3DXVec4Transform(&vec, &vec, &m);
        if ( (vec.x >= 0.0f) && (vec.y >= 0.0f) && (vec.x + vec.y <= 1.0f) && (vec.z >= 0.0f) )
        {
            if (pu) *pu = vec.x;
            if (pv) *pv = vec.y;
            if (pdist) *pdist = fabsf( vec.z );
            return TRUE;
        }
    }

    return FALSE;
}

BOOL WINAPI D3DXSphereBoundProbe(const D3DXVECTOR3 *center, float radius,
        const D3DXVECTOR3 *ray_position, const D3DXVECTOR3 *ray_direction)
{
    D3DXVECTOR3 difference = {0};
    float a, b, c, d;

    D3DXVec3Subtract(&difference, ray_position, center);
    c = D3DXVec3LengthSq(&difference) - radius * radius;
    if (c < 0.0f)
        return TRUE;
    a = D3DXVec3LengthSq(ray_direction);
    b = D3DXVec3Dot(&difference, ray_direction);
    d = b * b - a * c;

    return d >= 0.0f && (b <= 0.0f || d > b * b);
}

/*************************************************************************
 * D3DXCreateMesh
 */
HRESULT WINAPI D3DXCreateMesh(DWORD numfaces, DWORD numvertices, DWORD options,
        const D3DVERTEXELEMENT9 *declaration, struct IDirect3DDevice9 *device, struct ID3DXMesh **mesh)
{
    HRESULT hr;
    DWORD fvf;
    IDirect3DVertexDeclaration9 *vertex_declaration;
    UINT vertex_declaration_size;
    UINT num_elem;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    DWORD *attrib_buffer;
    struct d3dx9_mesh *object;
    DWORD index_usage = 0;
    D3DPOOL index_pool = D3DPOOL_DEFAULT;
    D3DFORMAT index_format = D3DFMT_INDEX16;
    DWORD vertex_usage = 0;
    D3DPOOL vertex_pool = D3DPOOL_DEFAULT;
    int i;

    TRACE("numfaces %lu, numvertices %lu, options %#lx, declaration %p, device %p, mesh %p.\n",
            numfaces, numvertices, options, declaration, device, mesh);

    if (numfaces == 0 || numvertices == 0 || declaration == NULL || device == NULL || mesh == NULL ||
        /* D3DXMESH_VB_SHARE is for cloning, and D3DXMESH_USEHWONLY is for ConvertToBlendedMesh */
        (options & (D3DXMESH_VB_SHARE | D3DXMESH_USEHWONLY | 0xfffe0000)))
    {
        return D3DERR_INVALIDCALL;
    }
    for (i = 0; declaration[i].Stream != 0xff; i++)
        if (declaration[i].Stream != 0)
            return D3DERR_INVALIDCALL;
    num_elem = i + 1;

    if (options & D3DXMESH_32BIT)
        index_format = D3DFMT_INDEX32;

    if (options & D3DXMESH_DONOTCLIP) {
        index_usage |= D3DUSAGE_DONOTCLIP;
        vertex_usage |= D3DUSAGE_DONOTCLIP;
    }
    if (options & D3DXMESH_POINTS) {
        index_usage |= D3DUSAGE_POINTS;
        vertex_usage |= D3DUSAGE_POINTS;
    }
    if (options & D3DXMESH_RTPATCHES) {
        index_usage |= D3DUSAGE_RTPATCHES;
        vertex_usage |= D3DUSAGE_RTPATCHES;
    }
    if (options & D3DXMESH_NPATCHES) {
        index_usage |= D3DUSAGE_NPATCHES;
        vertex_usage |= D3DUSAGE_NPATCHES;
    }

    if (options & D3DXMESH_VB_SYSTEMMEM)
        vertex_pool = D3DPOOL_SYSTEMMEM;
    else if (options & D3DXMESH_VB_MANAGED)
        vertex_pool = D3DPOOL_MANAGED;

    if (options & D3DXMESH_VB_WRITEONLY)
        vertex_usage |= D3DUSAGE_WRITEONLY;
    if (options & D3DXMESH_VB_DYNAMIC)
        vertex_usage |= D3DUSAGE_DYNAMIC;
    if (options & D3DXMESH_VB_SOFTWAREPROCESSING)
        vertex_usage |= D3DUSAGE_SOFTWAREPROCESSING;

    if (options & D3DXMESH_IB_SYSTEMMEM)
        index_pool = D3DPOOL_SYSTEMMEM;
    else if (options & D3DXMESH_IB_MANAGED)
        index_pool = D3DPOOL_MANAGED;

    if (options & D3DXMESH_IB_WRITEONLY)
        index_usage |= D3DUSAGE_WRITEONLY;
    if (options & D3DXMESH_IB_DYNAMIC)
        index_usage |= D3DUSAGE_DYNAMIC;
    if (options & D3DXMESH_IB_SOFTWAREPROCESSING)
        index_usage |= D3DUSAGE_SOFTWAREPROCESSING;

    hr = D3DXFVFFromDeclarator(declaration, &fvf);
    if (hr != D3D_OK)
    {
        fvf = 0;
    }

    if (FAILED(hr = IDirect3DDevice9_CreateVertexDeclaration(device, declaration, &vertex_declaration)))
    {
        WARN("Failed to create vertex declaration, hr %#lx.\n", hr);
        return hr;
    }
    vertex_declaration_size = D3DXGetDeclVertexSize(declaration, declaration[0].Stream);

    if (FAILED(hr = IDirect3DDevice9_CreateVertexBuffer(device, numvertices * vertex_declaration_size,
            vertex_usage, fvf, vertex_pool, &vertex_buffer, NULL)))
    {
        WARN("Failed to create vertex buffer, hr %#lx.\n", hr);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
        return hr;
    }

    if (FAILED(hr = IDirect3DDevice9_CreateIndexBuffer(device,
            numfaces * 3 * ((index_format == D3DFMT_INDEX16) ? 2 : 4), index_usage, index_format,
            index_pool, &index_buffer, NULL)))
    {
        WARN("Failed to create index buffer, hr %#lx.\n", hr);
        IDirect3DVertexBuffer9_Release(vertex_buffer);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
        return hr;
    }

    attrib_buffer = calloc(numfaces, sizeof(*attrib_buffer));
    object = calloc(1, sizeof(*object));
    if (object == NULL || attrib_buffer == NULL)
    {
        free(object);
        free(attrib_buffer);
        IDirect3DIndexBuffer9_Release(index_buffer);
        IDirect3DVertexBuffer9_Release(vertex_buffer);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
        *mesh = NULL;
        return E_OUTOFMEMORY;
    }
    object->ID3DXMesh_iface.lpVtbl = &D3DXMesh_Vtbl;
    object->ref = 1;

    object->numfaces = numfaces;
    object->numvertices = numvertices;
    object->options = options;
    object->fvf = fvf;
    object->device = device;
    IDirect3DDevice9_AddRef(device);

    copy_declaration(object->cached_declaration, declaration, num_elem);
    object->vertex_declaration = vertex_declaration;
    object->vertex_declaration_size = vertex_declaration_size;
    object->num_elem = num_elem;
    object->vertex_buffer = vertex_buffer;
    object->index_buffer = index_buffer;
    object->attrib_buffer = attrib_buffer;

    *mesh = &object->ID3DXMesh_iface;

    return D3D_OK;
}

/*************************************************************************
 * D3DXCreateMeshFVF
 */
HRESULT WINAPI D3DXCreateMeshFVF(DWORD face_count, DWORD vertex_count, DWORD options,
        DWORD fvf, struct IDirect3DDevice9 *device, struct ID3DXMesh **mesh)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];

    TRACE("face_count %lu, vertex_count %lu, options %#lx, fvf %#lx, device %p, mesh %p.\n",
            face_count, vertex_count, options, fvf, device, mesh);

    hr = D3DXDeclaratorFromFVF(fvf, declaration);
    if (FAILED(hr)) return hr;

    return D3DXCreateMesh(face_count, vertex_count, options, declaration, device, mesh);
}


struct mesh_data {
    unsigned int num_vertices;
    unsigned int num_poly_faces;
    unsigned int num_tri_faces;
    D3DXVECTOR3 *vertices;
    unsigned int *num_tri_per_face;
    DWORD *indices;

    DWORD fvf;

    /* optional mesh data */

    unsigned int num_normals;
    D3DXVECTOR3 *normals;
    DWORD *normal_indices;

    D3DXVECTOR2 *tex_coords;

    DWORD *vertex_colors;

    unsigned int num_materials;
    D3DXMATERIAL *materials;
    DWORD *material_indices;

    struct ID3DXSkinInfo *skin_info;
    unsigned int bone_count;
    unsigned int skin_weights_info_count;
};

static HRESULT parse_texture_filename(ID3DXFileData *filedata, char **filename_out)
{
    HRESULT hr;
    SIZE_T data_size;
    BYTE *data;
    char *filename_in;

    /* template TextureFilename {
     *     STRING filename;
     * }
     */

    free(*filename_out);
    *filename_out = NULL;

    hr = filedata->lpVtbl->Lock(filedata, &data_size, (const void**)&data);
    if (FAILED(hr)) return hr;

    /* FIXME: String must be retrieved directly instead of through a pointer once ID3DXFILE is fixed */
    if (data_size < sizeof(filename_in))
    {
        WARN("Truncated data (%Iu bytes).\n", data_size);
        filedata->lpVtbl->Unlock(filedata);
        return E_FAIL;
    }
    filename_in = *(char **)data;

    if (!(*filename_out = strdup(filename_in))) {
        filedata->lpVtbl->Unlock(filedata);
        return E_OUTOFMEMORY;
    }

    filedata->lpVtbl->Unlock(filedata);

    return D3D_OK;
}

static HRESULT parse_material(ID3DXFileData *filedata, D3DXMATERIAL *material)
{
    HRESULT hr;
    SIZE_T data_size;
    const BYTE *data;
    GUID type;
    ID3DXFileData *child;
    SIZE_T i, nb_children;

    material->pTextureFilename = NULL;

    hr = filedata->lpVtbl->Lock(filedata, &data_size, (const void**)&data);
    if (FAILED(hr)) return hr;

    /*
     * template ColorRGBA {
     *     FLOAT red;
     *     FLOAT green;
     *     FLOAT blue;
     *     FLOAT alpha;
     * }
     * template ColorRGB {
     *     FLOAT red;
     *     FLOAT green;
     *     FLOAT blue;
     * }
     * template Material {
     *     ColorRGBA faceColor;
     *     FLOAT power;
     *     ColorRGB specularColor;
     *     ColorRGB emissiveColor;
     *     [ ... ]
     * }
     */
    if (data_size != sizeof(float) * 11)
    {
        WARN("Incorrect data size (%Id bytes).\n", data_size);
        filedata->lpVtbl->Unlock(filedata);
        return E_FAIL;
    }

    memcpy(&material->MatD3D.Diffuse, data, sizeof(D3DCOLORVALUE));
    data += sizeof(D3DCOLORVALUE);
    material->MatD3D.Power = *(FLOAT*)data;
    data += sizeof(FLOAT);
    memcpy(&material->MatD3D.Specular, data, sizeof(FLOAT) * 3);
    material->MatD3D.Specular.a = 1.0f;
    data += 3 * sizeof(FLOAT);
    memcpy(&material->MatD3D.Emissive, data, sizeof(FLOAT) * 3);
    material->MatD3D.Emissive.a = 1.0f;
    material->MatD3D.Ambient.r = 0.0f;
    material->MatD3D.Ambient.g = 0.0f;
    material->MatD3D.Ambient.b = 0.0f;
    material->MatD3D.Ambient.a = 1.0f;

    filedata->lpVtbl->Unlock(filedata);

    hr = filedata->lpVtbl->GetChildren(filedata, &nb_children);
    if (FAILED(hr))
        return hr;

    for (i = 0; i < nb_children; i++)
    {
        hr = filedata->lpVtbl->GetChild(filedata, i, &child);
        if (FAILED(hr))
            return hr;
        hr = child->lpVtbl->GetType(child, &type);
        if (FAILED(hr))
            goto err;

        if (IsEqualGUID(&type, &TID_D3DRMTextureFilename)) {
            hr = parse_texture_filename(child, &material->pTextureFilename);
            if (FAILED(hr))
                goto err;
        }
        IUnknown_Release(child);
    }
    return D3D_OK;

err:
    IUnknown_Release(child);
    return hr;
}

static void destroy_materials(struct mesh_data *mesh)
{
    unsigned int i;

    for (i = 0; i < mesh->num_materials; ++i)
        free(mesh->materials[i].pTextureFilename);
    free(mesh->materials);
    free(mesh->material_indices);
    mesh->num_materials = 0;
    mesh->materials = NULL;
    mesh->material_indices = NULL;
}

static HRESULT parse_material_list(ID3DXFileData *filedata, struct mesh_data *mesh, DWORD flags)
{
    ID3DXFileData *child = NULL;
    unsigned int material_count;
    const uint32_t *in_ptr;
    SIZE_T nb_children;
    SIZE_T data_size;
    const void *data;
    unsigned int i;
    HRESULT hr;
    GUID type;

    if (!(flags & PROVIDE_MATERIALS))
        return S_OK;

    destroy_materials(mesh);

    hr = filedata->lpVtbl->Lock(filedata, &data_size, &data);
    if (FAILED(hr)) return hr;

    /* template MeshMaterialList {
     *     DWORD nMaterials;
     *     DWORD nFaceIndexes;
     *     array DWORD faceIndexes[nFaceIndexes];
     *     [ Material ]
     * }
     */

    in_ptr = data;
    hr = E_FAIL;

    if (data_size < sizeof(uint32_t))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }
    material_count = *in_ptr++;
    if (!material_count) {
        hr = D3D_OK;
        goto end;
    }

    if (data_size < 2 * sizeof(uint32_t))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }
    if (*in_ptr++ != mesh->num_poly_faces)
    {
        WARN("Number of material face indices (%u) doesn't match number of faces (%u).\n",
             *(in_ptr - 1), mesh->num_poly_faces);
        goto end;
    }
    if (data_size < 2 * sizeof(uint32_t) + mesh->num_poly_faces * sizeof(uint32_t))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }
    for (i = 0; i < mesh->num_poly_faces; ++i)
    {
        if (*in_ptr++ >= material_count)
        {
            WARN("Face %u: reference to undefined material %u (only %u materials).\n",
                    i, *(in_ptr - 1), material_count);
            goto end;
        }
    }

    mesh->materials = malloc(material_count * sizeof(*mesh->materials));
    mesh->material_indices = malloc(mesh->num_poly_faces * sizeof(*mesh->material_indices));
    if (!mesh->materials || !mesh->material_indices) {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    memcpy(mesh->material_indices, (const uint32_t *)data + 2, mesh->num_poly_faces * sizeof(uint32_t));

    hr = filedata->lpVtbl->GetChildren(filedata, &nb_children);
    if (FAILED(hr))
        goto end;

    for (i = 0; i < nb_children; i++)
    {
        hr = filedata->lpVtbl->GetChild(filedata, i, &child);
        if (FAILED(hr))
            goto end;
        hr = child->lpVtbl->GetType(child, &type);
        if (FAILED(hr))
            goto end;

        if (IsEqualGUID(&type, &TID_D3DRMMaterial))
        {
            if (mesh->num_materials >= material_count)
            {
                WARN("%u materials defined, only %u declared.\n", mesh->num_materials, material_count);
                hr = E_FAIL;
                goto end;
            }
            hr = parse_material(child, &mesh->materials[mesh->num_materials++]);
            if (FAILED(hr))
                goto end;
        }

        IUnknown_Release(child);
        child = NULL;
    }
    if (material_count != mesh->num_materials)
    {
        WARN("Only %u of %u materials defined.\n", material_count, mesh->num_materials);
        hr = E_FAIL;
    }

end:
    if (child)
        IUnknown_Release(child);
    filedata->lpVtbl->Unlock(filedata);
    return hr;
}

static HRESULT parse_texture_coords(ID3DXFileData *filedata, struct mesh_data *mesh, DWORD flags)
{
    const uint32_t *data;
    SIZE_T data_size;
    HRESULT hr;

    free(mesh->tex_coords);
    mesh->tex_coords = NULL;

    hr = filedata->lpVtbl->Lock(filedata, &data_size, (const void **)&data);
    if (FAILED(hr)) return hr;

    /* template Coords2d {
     *     FLOAT u;
     *     FLOAT v;
     * }
     * template MeshTextureCoords {
     *     DWORD nTextureCoords;
     *     array Coords2d textureCoords[nTextureCoords];
     * }
     */

    hr = E_FAIL;

    if (data_size < sizeof(uint32_t))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }
    if (*data != mesh->num_vertices)
    {
        WARN("Number of texture coordinates (%u) doesn't match number of vertices (%u).\n",
                *data, mesh->num_vertices);
        goto end;
    }
    ++data;
    if (data_size < sizeof(uint32_t) + mesh->num_vertices * sizeof(*mesh->tex_coords))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }

    mesh->tex_coords = malloc(mesh->num_vertices * sizeof(*mesh->tex_coords));
    if (!mesh->tex_coords) {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    memcpy(mesh->tex_coords, data, mesh->num_vertices * sizeof(*mesh->tex_coords));

    mesh->fvf |= D3DFVF_TEX1;

    hr = D3D_OK;

end:
    filedata->lpVtbl->Unlock(filedata);
    return hr;
}

static HRESULT parse_vertex_colors(ID3DXFileData *filedata, struct mesh_data *mesh, DWORD flags)
{
    unsigned int color_count, i;
    const uint32_t *data;
    SIZE_T data_size;
    HRESULT hr;

    free(mesh->vertex_colors);
    mesh->vertex_colors = NULL;

    hr = filedata->lpVtbl->Lock(filedata, &data_size, (const void **)&data);
    if (FAILED(hr)) return hr;

    /* template IndexedColor {
     *     DWORD index;
     *     ColorRGBA indexColor;
     * }
     * template MeshVertexColors {
     *     DWORD nVertexColors;
     *     array IndexedColor vertexColors[nVertexColors];
     * }
     */

    hr = E_FAIL;

    if (data_size < sizeof(uint32_t))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }
    color_count = *data;
    ++data;
    if (data_size < sizeof(uint32_t) + color_count * (sizeof(uint32_t) + sizeof(D3DCOLORVALUE)))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }

    mesh->vertex_colors = malloc(mesh->num_vertices * sizeof(*mesh->vertex_colors));
    if (!mesh->vertex_colors) {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    for (i = 0; i < mesh->num_vertices; i++)
        mesh->vertex_colors[i] = D3DCOLOR_ARGB(0, 0xff, 0xff, 0xff);
    for (i = 0; i < color_count; ++i)
    {
        D3DCOLORVALUE color;
        unsigned int index = *data;

        ++data;
        if (index >= mesh->num_vertices)
        {
            WARN("Vertex color %u references undefined vertex %u (only %u vertices).\n",
                    i, index, mesh->num_vertices);
            goto end;
        }
        memcpy(&color, data, sizeof(color));
        data += sizeof(color) / sizeof(*data);
        color.r = min(1.0f, max(0.0f, color.r));
        color.g = min(1.0f, max(0.0f, color.g));
        color.b = min(1.0f, max(0.0f, color.b));
        color.a = min(1.0f, max(0.0f, color.a));
        mesh->vertex_colors[index] = D3DCOLOR_ARGB((BYTE)(color.a * 255.0f + 0.5f),
                                                   (BYTE)(color.r * 255.0f + 0.5f),
                                                   (BYTE)(color.g * 255.0f + 0.5f),
                                                   (BYTE)(color.b * 255.0f + 0.5f));
    }

    mesh->fvf |= D3DFVF_DIFFUSE;

    hr = D3D_OK;

end:
    filedata->lpVtbl->Unlock(filedata);
    return hr;
}

static HRESULT parse_normals(ID3DXFileData *filedata, struct mesh_data *mesh, DWORD flags)
{
    unsigned int num_face_indices = mesh->num_poly_faces * 2 + mesh->num_tri_faces;
    DWORD *index_out_ptr;
    SIZE_T data_size;
    const BYTE *data;
    unsigned int i;
    HRESULT hr;

    free(mesh->normals);
    mesh->num_normals = 0;
    mesh->normals = NULL;
    mesh->normal_indices = NULL;
    mesh->fvf |= D3DFVF_NORMAL;

    hr = filedata->lpVtbl->Lock(filedata, &data_size, (const void **)&data);
    if (FAILED(hr)) return hr;

    /* template Vector {
     *     FLOAT x;
     *     FLOAT y;
     *     FLOAT z;
     * }
     * template MeshFace {
     *     DWORD nFaceVertexIndices;
     *     array DWORD faceVertexIndices[nFaceVertexIndices];
     * }
     * template MeshNormals {
     *     DWORD nNormals;
     *     array Vector normals[nNormals];
     *     DWORD nFaceNormals;
     *     array MeshFace faceNormals[nFaceNormals];
     * }
     */

    hr = E_FAIL;

    if (data_size < sizeof(uint32_t) * 2)
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }
    mesh->num_normals = *(uint32_t *)data;
    data += sizeof(uint32_t);
    if (data_size < sizeof(uint32_t) * 2 + mesh->num_normals * sizeof(D3DXVECTOR3) +
            num_face_indices * sizeof(uint32_t))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }

    mesh->normals = malloc(mesh->num_normals * sizeof(D3DXVECTOR3));
    mesh->normal_indices = malloc(num_face_indices * sizeof(*mesh->normal_indices));
    if (!mesh->normals || !mesh->normal_indices) {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    memcpy(mesh->normals, data, mesh->num_normals * sizeof(D3DXVECTOR3));
    data += mesh->num_normals * sizeof(D3DXVECTOR3);
    for (i = 0; i < mesh->num_normals; i++)
        D3DXVec3Normalize(&mesh->normals[i], &mesh->normals[i]);

    if (*(uint32_t *)data != mesh->num_poly_faces)
    {
        WARN("Number of face normals (%u) doesn't match number of faces (%u).\n",
             *(uint32_t *)data, mesh->num_poly_faces);
        goto end;
    }
    data += sizeof(uint32_t);
    index_out_ptr = mesh->normal_indices;
    for (i = 0; i < mesh->num_poly_faces; i++)
    {
        unsigned int count = *(uint32_t *)data;
        unsigned int j;

        if (count != mesh->num_tri_per_face[i] + 2)
        {
            WARN("Face %u: number of normals (%u) doesn't match number of vertices (%u).\n",
                 i, count, mesh->num_tri_per_face[i] + 2);
            goto end;
        }
        data += sizeof(uint32_t);

        for (j = 0; j < count; j++)
        {
            uint32_t normal_index = *(uint32_t *)data;

            if (normal_index >= mesh->num_normals)
            {
                WARN("Face %u, normal index %u: reference to undefined normal %u (only %u normals).\n",
                        i, j, normal_index, mesh->num_normals);
                goto end;
            }
            *index_out_ptr++ = normal_index;
            data += sizeof(uint32_t);
        }
    }

    hr =  D3D_OK;

end:
    filedata->lpVtbl->Unlock(filedata);
    return hr;
}

static HRESULT parse_skin_mesh_header(ID3DXFileData *filedata, struct mesh_data *mesh_data, DWORD flags)
{
    const BYTE *data;
    SIZE_T data_size;
    HRESULT hr;

    TRACE("filedata %p, mesh_data %p.\n", filedata, mesh_data);

    if (!(flags & PROVIDE_SKININFO))
        return S_OK;

    if (mesh_data->skin_info)
    {
        WARN("Skin mesh header already encountered\n");
        return E_FAIL;
    }

    if (FAILED(hr = filedata->lpVtbl->Lock(filedata, &data_size, (const void **)&data)))
        return hr;

    if (data_size < sizeof(WORD) * 3)
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        filedata->lpVtbl->Unlock(filedata);
        return E_FAIL;
    }
    /* Skip nMaxSkinWeightsPerVertex and nMaxSkinWeightsPerFace */
    data += 2 * sizeof(WORD);
    mesh_data->bone_count = *(WORD *)data;
    hr = D3DXCreateSkinInfoFVF(mesh_data->num_vertices, mesh_data->fvf, mesh_data->bone_count,
            &mesh_data->skin_info);

    return hr;
}

static HRESULT parse_skin_weights_info(ID3DXFileData *filedata, struct mesh_data *mesh_data, DWORD flags)
{
    unsigned int index = mesh_data->skin_weights_info_count;
    unsigned int influence_count;
    const char *name;
    const BYTE *data;
    SIZE_T data_size;
    HRESULT hr;

    TRACE("filedata %p, mesh_data %p, index %u.\n", filedata, mesh_data, index);

    if (!(flags & PROVIDE_SKININFO))
        return S_OK;

    if (!mesh_data->skin_info)
    {
        WARN("Skin weights found but skin mesh header not encountered yet.\n");
        return E_FAIL;
    }

    if (FAILED(hr = filedata->lpVtbl->Lock(filedata, &data_size, (const void **)&data)))
        return hr;

    /* FIXME: String will have to be retrieved directly instead of through a
     * pointer once our ID3DXFileData implementation is fixed. */
    name = *(const char **)data;
    data += sizeof(char *);

    influence_count = *(uint32_t *)data;
    data += sizeof(uint32_t);

    if (data_size < (sizeof(char *) + sizeof(uint32_t) + influence_count * (sizeof(uint32_t) + sizeof(float))
            + 16 * sizeof(float)))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        filedata->lpVtbl->Unlock(filedata);
        return E_FAIL;
    }

    hr = mesh_data->skin_info->lpVtbl->SetBoneName(mesh_data->skin_info, index, name);
    if (SUCCEEDED(hr))
        hr = mesh_data->skin_info->lpVtbl->SetBoneInfluence(mesh_data->skin_info, index, influence_count,
                (const DWORD *)data, (const float *)(data + influence_count * sizeof(uint32_t)));
    if (SUCCEEDED(hr))
        hr = mesh_data->skin_info->lpVtbl->SetBoneOffsetMatrix(mesh_data->skin_info, index,
                (const D3DMATRIX *)(data + influence_count * (sizeof(uint32_t) + sizeof(float))));

    if (SUCCEEDED(hr))
        ++mesh_data->skin_weights_info_count;
    return hr;
}

typedef HRESULT (*mesh_parse_func)(ID3DXFileData *, struct mesh_data *, DWORD);

static mesh_parse_func mesh_get_parse_func(const GUID *type)
{
    static const struct
    {
        const GUID *type;
        mesh_parse_func func;
    }
    funcs[] =
    {
        {&TID_D3DRMMeshNormals, parse_normals},
        {&TID_D3DRMMeshVertexColors, parse_vertex_colors},
        {&TID_D3DRMMeshTextureCoords, parse_texture_coords},
        {&TID_D3DRMMeshMaterialList, parse_material_list},
        {&DXFILEOBJ_XSkinMeshHeader, parse_skin_mesh_header},
        {&DXFILEOBJ_SkinWeights, parse_skin_weights_info},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(funcs); ++i)
        if (IsEqualGUID(type, funcs[i].type))
            return funcs[i].func;

    return NULL;
}

static HRESULT parse_mesh(ID3DXFileData *filedata, struct mesh_data *mesh_data, DWORD provide_flags)
{
    ID3DXFileData *child = NULL;
    mesh_parse_func parse_func;
    const BYTE *data, *in_ptr;
    DWORD *index_out_ptr;
    SIZE_T child_count;
    SIZE_T data_size;
    unsigned int i;
    HRESULT hr;
    GUID type;

    /*
     * template Mesh {
     *     DWORD nVertices;
     *     array Vector vertices[nVertices];
     *     DWORD nFaces;
     *     array MeshFace faces[nFaces];
     *     [ ... ]
     * }
     */

    mesh_data->skin_weights_info_count = 0;

    hr = filedata->lpVtbl->Lock(filedata, &data_size, (const void **)&data);
    if (FAILED(hr)) return hr;

    in_ptr = data;
    hr = E_FAIL;

    if (data_size < sizeof(uint32_t) * 2)
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }
    mesh_data->num_vertices = *(uint32_t *)in_ptr;
    if (data_size < sizeof(uint32_t) * 2 + mesh_data->num_vertices * sizeof(D3DXVECTOR3))
    {
        WARN("Truncated data (%Id bytes).\n", data_size);
        goto end;
    }
    in_ptr += sizeof(uint32_t) + mesh_data->num_vertices * sizeof(D3DXVECTOR3);

    mesh_data->num_poly_faces = *(uint32_t *)in_ptr;
    in_ptr += sizeof(uint32_t);

    mesh_data->num_tri_faces = 0;
    for (i = 0; i < mesh_data->num_poly_faces; i++)
    {
        unsigned int poly_vertices_count;
        unsigned int j;

        if (data_size - (in_ptr - data) < sizeof(uint32_t))
        {
            WARN("Truncated data (%Id bytes).\n", data_size);
            goto end;
        }
        poly_vertices_count = *(uint32_t *)in_ptr;
        in_ptr += sizeof(uint32_t);
        if (data_size - (in_ptr - data) < poly_vertices_count * sizeof(uint32_t))
        {
            WARN("Truncated data (%Id bytes).\n", data_size);
            goto end;
        }
        if (poly_vertices_count < 3)
        {
            WARN("Face %u has only %u vertices.\n", i, poly_vertices_count);
            goto end;
        }
        for (j = 0; j < poly_vertices_count; j++)
        {
            if (*(uint32_t *)in_ptr >= mesh_data->num_vertices)
            {
                WARN("Face %u, index %u: undefined vertex %u (only %u vertices).\n",
                     i, j, *(uint32_t *)in_ptr, mesh_data->num_vertices);
                goto end;
            }
            in_ptr += sizeof(uint32_t);
        }
        mesh_data->num_tri_faces += poly_vertices_count - 2;
    }

    mesh_data->fvf = D3DFVF_XYZ;

    mesh_data->vertices = malloc(mesh_data->num_vertices * sizeof(*mesh_data->vertices));
    mesh_data->num_tri_per_face = malloc(mesh_data->num_poly_faces * sizeof(*mesh_data->num_tri_per_face));
    mesh_data->indices = malloc((mesh_data->num_tri_faces + mesh_data->num_poly_faces * 2) * sizeof(*mesh_data->indices));
    if (!mesh_data->vertices || !mesh_data->num_tri_per_face || !mesh_data->indices) {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    in_ptr = data + sizeof(uint32_t);
    memcpy(mesh_data->vertices, in_ptr, mesh_data->num_vertices * sizeof(D3DXVECTOR3));
    in_ptr += mesh_data->num_vertices * sizeof(D3DXVECTOR3) + sizeof(uint32_t);

    index_out_ptr = mesh_data->indices;
    for (i = 0; i < mesh_data->num_poly_faces; i++)
    {
        unsigned int count;

        count = *(uint32_t *)in_ptr;
        in_ptr += sizeof(uint32_t);
        mesh_data->num_tri_per_face[i] = count - 2;

        while (count--)
        {
            *index_out_ptr++ = *(uint32_t *)in_ptr;
            in_ptr += sizeof(uint32_t);
        }
    }

    hr = filedata->lpVtbl->GetChildren(filedata, &child_count);
    if (FAILED(hr))
        goto end;

    for (i = 0; i < child_count; i++)
    {
        hr = filedata->lpVtbl->GetChild(filedata, i, &child);
        if (FAILED(hr))
            goto end;
        hr = child->lpVtbl->GetType(child, &type);
        if (FAILED(hr))
            goto end;

        if ((parse_func = mesh_get_parse_func(&type)))
            hr = parse_func(child, mesh_data, provide_flags);
        if (FAILED(hr))
            goto end;

        IUnknown_Release(child);
        child = NULL;
    }

    if (mesh_data->skin_info && (mesh_data->skin_weights_info_count != mesh_data->bone_count))
    {
        WARN("Mismatch between skin weights info count %u and bones count %u from skin mesh header.\n",
                mesh_data->skin_weights_info_count, mesh_data->bone_count);
        hr = E_FAIL;
        goto end;
    }

    if ((provide_flags & PROVIDE_SKININFO) && !mesh_data->skin_info)
    {
        if (FAILED(hr = D3DXCreateSkinInfoFVF(mesh_data->num_vertices, mesh_data->fvf,
                mesh_data->bone_count, &mesh_data->skin_info)))
            goto end;
    }

    hr = D3D_OK;

end:
    if (child)
        IUnknown_Release(child);
    filedata->lpVtbl->Unlock(filedata);
    return hr;
}

static HRESULT generate_effects(ID3DXBuffer *materials, DWORD num_materials,
                                ID3DXBuffer **effects)
{
    HRESULT hr;
    D3DXEFFECTINSTANCE *effect_ptr;
    BYTE *out_ptr;
    const D3DXMATERIAL *material_ptr = ID3DXBuffer_GetBufferPointer(materials);
    static const struct {
        const char *param_name;
        DWORD name_size;
        DWORD num_bytes;
        DWORD value_offset;
    } material_effects[] = {
#define EFFECT_TABLE_ENTRY(str, field) \
    {str, sizeof(str), sizeof(material_ptr->MatD3D.field), offsetof(D3DXMATERIAL, MatD3D.field)}
        EFFECT_TABLE_ENTRY("Diffuse", Diffuse),
        EFFECT_TABLE_ENTRY("Power", Power),
        EFFECT_TABLE_ENTRY("Specular", Specular),
        EFFECT_TABLE_ENTRY("Emissive", Emissive),
        EFFECT_TABLE_ENTRY("Ambient", Ambient),
#undef EFFECT_TABLE_ENTRY
    };
    static const char texture_paramname[] = "Texture0@Name";
    DWORD buffer_size;
    DWORD i;

    /* effects buffer layout:
     *
     * D3DXEFFECTINSTANCE effects[num_materials];
     * for (effect in effects)
     * {
     *     D3DXEFFECTDEFAULT defaults[effect.NumDefaults];
     *     for (default in defaults)
     *     {
     *         *default.pParamName;
     *         *default.pValue;
     *     }
     * }
     */
    buffer_size = sizeof(D3DXEFFECTINSTANCE);
    buffer_size += sizeof(D3DXEFFECTDEFAULT) * ARRAY_SIZE(material_effects);
    for (i = 0; i < ARRAY_SIZE(material_effects); i++) {
        buffer_size += material_effects[i].name_size;
        buffer_size += material_effects[i].num_bytes;
    }
    buffer_size *= num_materials;
    for (i = 0; i < num_materials; i++) {
        if (material_ptr[i].pTextureFilename) {
            buffer_size += sizeof(D3DXEFFECTDEFAULT);
            buffer_size += sizeof(texture_paramname);
            buffer_size += strlen(material_ptr[i].pTextureFilename) + 1;
        }
    }

    hr = D3DXCreateBuffer(buffer_size, effects);
    if (FAILED(hr)) return hr;
    effect_ptr = ID3DXBuffer_GetBufferPointer(*effects);
    out_ptr = (BYTE*)(effect_ptr + num_materials);

    for (i = 0; i < num_materials; i++)
    {
        DWORD j;
        D3DXEFFECTDEFAULT *defaults = (D3DXEFFECTDEFAULT*)out_ptr;

        effect_ptr->pDefaults = defaults;
        effect_ptr->NumDefaults = material_ptr->pTextureFilename ? 6 : 5;
        out_ptr = (BYTE*)(effect_ptr->pDefaults + effect_ptr->NumDefaults);

        for (j = 0; j < ARRAY_SIZE(material_effects); j++)
        {
            defaults->pParamName = (char *)out_ptr;
            strcpy(defaults->pParamName, material_effects[j].param_name);
            defaults->pValue = defaults->pParamName + material_effects[j].name_size;
            defaults->Type = D3DXEDT_FLOATS;
            defaults->NumBytes = material_effects[j].num_bytes;
            memcpy(defaults->pValue, (BYTE*)material_ptr + material_effects[j].value_offset, defaults->NumBytes);
            out_ptr = (BYTE*)defaults->pValue + defaults->NumBytes;
            defaults++;
        }

        if (material_ptr->pTextureFilename)
        {
            defaults->pParamName = (char *)out_ptr;
            strcpy(defaults->pParamName, texture_paramname);
            defaults->pValue = defaults->pParamName + sizeof(texture_paramname);
            defaults->Type = D3DXEDT_STRING;
            defaults->NumBytes = strlen(material_ptr->pTextureFilename) + 1;
            strcpy(defaults->pValue, material_ptr->pTextureFilename);
            out_ptr = (BYTE*)defaults->pValue + defaults->NumBytes;
        }
        material_ptr++;
        effect_ptr++;
    }
    assert(out_ptr - (BYTE*)ID3DXBuffer_GetBufferPointer(*effects) == buffer_size);

    return D3D_OK;
}

HRESULT WINAPI D3DXLoadSkinMeshFromXof(struct ID3DXFileData *filedata, DWORD options,
        struct IDirect3DDevice9 *device, struct ID3DXBuffer **adjacency_out, struct ID3DXBuffer **materials_out,
        struct ID3DXBuffer **effects_out, DWORD *num_materials_out, struct ID3DXSkinInfo **skin_info_out,
        struct ID3DXMesh **mesh_out)
{
    HRESULT hr;
    DWORD *index_in_ptr;
    struct mesh_data mesh_data;
    DWORD total_vertices;
    ID3DXMesh *d3dxmesh = NULL;
    ID3DXBuffer *adjacency = NULL;
    ID3DXBuffer *materials = NULL;
    ID3DXBuffer *effects = NULL;
    struct vertex_duplication {
        DWORD normal_index;
        struct list entry;
    } *duplications = NULL;
    DWORD i;
    void *vertices = NULL;
    void *indices = NULL;
    BYTE *out_ptr;
    DWORD provide_flags = 0;

    TRACE("filedata %p, options %#lx, device %p, adjacency_out %p, materials_out %p, "
            "effects_out %p, num_materials_out %p, skin_info_out %p, mesh_out %p.\n",
            filedata, options, device, adjacency_out, materials_out,
            effects_out, num_materials_out, skin_info_out, mesh_out);

    ZeroMemory(&mesh_data, sizeof(mesh_data));

    if (num_materials_out || materials_out || effects_out)
        provide_flags |= PROVIDE_MATERIALS;
    if (skin_info_out)
        provide_flags |= PROVIDE_SKININFO;

    hr = parse_mesh(filedata, &mesh_data, provide_flags);
    if (FAILED(hr)) goto cleanup;

    if (!mesh_data.num_vertices)
    {
        if (adjacency_out)
            *adjacency_out = NULL;
        if (materials_out)
            *materials_out = NULL;
        if (effects_out)
            *effects_out = NULL;
        *mesh_out = NULL;
        hr = D3D_OK;
        goto cleanup;
    }

    total_vertices = mesh_data.num_vertices;
    if (mesh_data.fvf & D3DFVF_NORMAL) {
        /* duplicate vertices with multiple normals */
        DWORD num_face_indices = mesh_data.num_poly_faces * 2 + mesh_data.num_tri_faces;
        duplications = malloc((mesh_data.num_vertices + num_face_indices) * sizeof(*duplications));
        if (!duplications) {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        for (i = 0; i < total_vertices; i++)
        {
            duplications[i].normal_index = -1;
            list_init(&duplications[i].entry);
        }
        for (i = 0; i < num_face_indices; i++) {
            DWORD vertex_index = mesh_data.indices[i];
            DWORD normal_index = mesh_data.normal_indices[i];
            struct vertex_duplication *dup_ptr = &duplications[vertex_index];

            if (dup_ptr->normal_index == -1) {
                dup_ptr->normal_index = normal_index;
            } else {
                D3DXVECTOR3 *new_normal = &mesh_data.normals[normal_index];
                struct list *dup_list = &dup_ptr->entry;
                while (TRUE) {
                    D3DXVECTOR3 *cur_normal = &mesh_data.normals[dup_ptr->normal_index];
                    if (new_normal->x == cur_normal->x &&
                        new_normal->y == cur_normal->y &&
                        new_normal->z == cur_normal->z)
                    {
                        mesh_data.indices[i] = dup_ptr - duplications;
                        break;
                    } else if (!list_next(dup_list, &dup_ptr->entry)) {
                        dup_ptr = &duplications[total_vertices++];
                        dup_ptr->normal_index = normal_index;
                        list_add_tail(dup_list, &dup_ptr->entry);
                        mesh_data.indices[i] = dup_ptr - duplications;
                        break;
                    } else {
                        dup_ptr = LIST_ENTRY(list_next(dup_list, &dup_ptr->entry),
                                             struct vertex_duplication, entry);
                    }
                }
            }
        }
    }

    hr = D3DXCreateMeshFVF(mesh_data.num_tri_faces, total_vertices, options, mesh_data.fvf, device, &d3dxmesh);
    if (FAILED(hr)) goto cleanup;

    hr = d3dxmesh->lpVtbl->LockVertexBuffer(d3dxmesh, 0, &vertices);
    if (FAILED(hr)) goto cleanup;

    out_ptr = vertices;
    for (i = 0; i < mesh_data.num_vertices; i++) {
        *(D3DXVECTOR3*)out_ptr = mesh_data.vertices[i];
        out_ptr += sizeof(D3DXVECTOR3);
        if (mesh_data.fvf & D3DFVF_NORMAL) {
            if (duplications[i].normal_index == -1)
                ZeroMemory(out_ptr, sizeof(D3DXVECTOR3));
            else
                *(D3DXVECTOR3*)out_ptr = mesh_data.normals[duplications[i].normal_index];
            out_ptr += sizeof(D3DXVECTOR3);
        }
        if (mesh_data.fvf & D3DFVF_DIFFUSE) {
            *(DWORD*)out_ptr = mesh_data.vertex_colors[i];
            out_ptr += sizeof(DWORD);
        }
        if (mesh_data.fvf & D3DFVF_TEX1) {
            *(D3DXVECTOR2*)out_ptr = mesh_data.tex_coords[i];
            out_ptr += sizeof(D3DXVECTOR2);
        }
    }
    if (mesh_data.fvf & D3DFVF_NORMAL) {
        DWORD vertex_size = D3DXGetFVFVertexSize(mesh_data.fvf);
        out_ptr = vertices;
        for (i = 0; i < mesh_data.num_vertices; i++) {
            struct vertex_duplication *dup_ptr;
            LIST_FOR_EACH_ENTRY(dup_ptr, &duplications[i].entry, struct vertex_duplication, entry)
            {
                int j = dup_ptr - duplications;
                BYTE *dest_vertex = (BYTE*)vertices + j * vertex_size;

                memcpy(dest_vertex, out_ptr, vertex_size);
                dest_vertex += sizeof(D3DXVECTOR3);
                *(D3DXVECTOR3*)dest_vertex = mesh_data.normals[dup_ptr->normal_index];
            }
            out_ptr += vertex_size;
        }
    }
    d3dxmesh->lpVtbl->UnlockVertexBuffer(d3dxmesh);

    hr = d3dxmesh->lpVtbl->LockIndexBuffer(d3dxmesh, 0, &indices);
    if (FAILED(hr)) goto cleanup;

    index_in_ptr = mesh_data.indices;
#define FILL_INDEX_BUFFER(indices_var) \
        for (i = 0; i < mesh_data.num_poly_faces; i++) \
        { \
            DWORD count = mesh_data.num_tri_per_face[i]; \
            WORD first_index = *index_in_ptr++; \
            while (count--) { \
                *indices_var++ = first_index; \
                *indices_var++ = *index_in_ptr; \
                index_in_ptr++; \
                *indices_var++ = *index_in_ptr; \
            } \
            index_in_ptr++; \
        }
    if (options & D3DXMESH_32BIT) {
        DWORD *dword_indices = indices;
        FILL_INDEX_BUFFER(dword_indices)
    } else {
        WORD *word_indices = indices;
        FILL_INDEX_BUFFER(word_indices)
    }
#undef FILL_INDEX_BUFFER
    d3dxmesh->lpVtbl->UnlockIndexBuffer(d3dxmesh);

    if (mesh_data.material_indices) {
        DWORD *attrib_buffer = NULL;
        hr = d3dxmesh->lpVtbl->LockAttributeBuffer(d3dxmesh, 0, &attrib_buffer);
        if (FAILED(hr)) goto cleanup;
        for (i = 0; i < mesh_data.num_poly_faces; i++)
        {
            DWORD count = mesh_data.num_tri_per_face[i];
            while (count--)
                *attrib_buffer++ = mesh_data.material_indices[i];
        }
        d3dxmesh->lpVtbl->UnlockAttributeBuffer(d3dxmesh);

        hr = d3dxmesh->lpVtbl->OptimizeInplace(d3dxmesh,
                D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_IGNOREVERTS | D3DXMESHOPT_DONOTSPLIT,
                NULL, NULL, NULL, NULL);
        if (FAILED(hr)) goto cleanup;
    }

    if (mesh_data.num_materials && (materials_out || effects_out)) {
        DWORD buffer_size = mesh_data.num_materials * sizeof(D3DXMATERIAL);
        char *strings_out_ptr;
        D3DXMATERIAL *materials_ptr;

        for (i = 0; i < mesh_data.num_materials; i++) {
            if (mesh_data.materials[i].pTextureFilename)
                buffer_size += strlen(mesh_data.materials[i].pTextureFilename) + 1;
        }

        hr = D3DXCreateBuffer(buffer_size, &materials);
        if (FAILED(hr)) goto cleanup;

        materials_ptr = ID3DXBuffer_GetBufferPointer(materials);
        memcpy(materials_ptr, mesh_data.materials, mesh_data.num_materials * sizeof(D3DXMATERIAL));
        strings_out_ptr = (char*)(materials_ptr + mesh_data.num_materials);
        for (i = 0; i < mesh_data.num_materials; i++) {
            if (materials_ptr[i].pTextureFilename) {
                strcpy(strings_out_ptr, mesh_data.materials[i].pTextureFilename);
                materials_ptr[i].pTextureFilename = strings_out_ptr;
                strings_out_ptr += strlen(mesh_data.materials[i].pTextureFilename) + 1;
            }
        }
    }

    if (mesh_data.num_materials && effects_out) {
        hr = generate_effects(materials, mesh_data.num_materials, &effects);
        if (FAILED(hr)) goto cleanup;

        if (!materials_out) {
            ID3DXBuffer_Release(materials);
            materials = NULL;
        }
    }

    if (adjacency_out) {
        hr = D3DXCreateBuffer(mesh_data.num_tri_faces * 3 * sizeof(DWORD), &adjacency);
        if (FAILED(hr)) goto cleanup;
        hr = d3dxmesh->lpVtbl->GenerateAdjacency(d3dxmesh, 0.0f, ID3DXBuffer_GetBufferPointer(adjacency));
        if (FAILED(hr)) goto cleanup;
    }

    *mesh_out = d3dxmesh;
    if (adjacency_out) *adjacency_out = adjacency;
    if (num_materials_out) *num_materials_out = mesh_data.num_materials;
    if (materials_out) *materials_out = materials;
    if (effects_out) *effects_out = effects;
    if (skin_info_out) *skin_info_out = mesh_data.skin_info;

    hr = D3D_OK;
cleanup:
    if (FAILED(hr)) {
        if (d3dxmesh) IUnknown_Release(d3dxmesh);
        if (adjacency) ID3DXBuffer_Release(adjacency);
        if (materials) ID3DXBuffer_Release(materials);
        if (effects) ID3DXBuffer_Release(effects);
        if (mesh_data.skin_info) mesh_data.skin_info->lpVtbl->Release(mesh_data.skin_info);
        if (skin_info_out) *skin_info_out = NULL;
    }
    free(mesh_data.vertices);
    free(mesh_data.num_tri_per_face);
    free(mesh_data.indices);
    free(mesh_data.normals);
    free(mesh_data.normal_indices);
    destroy_materials(&mesh_data);
    free(mesh_data.tex_coords);
    free(mesh_data.vertex_colors);
    free(duplications);
    return hr;
}

HRESULT WINAPI D3DXLoadMeshHierarchyFromXA(const char *filename, DWORD options, struct IDirect3DDevice9 *device,
        struct ID3DXAllocateHierarchy *alloc_hier, struct ID3DXLoadUserData *load_user_data,
        D3DXFRAME **frame_hierarchy, struct ID3DXAnimationController **anim_controller)
{
    WCHAR *filenameW;
    HRESULT hr;
    int len;

    TRACE("filename %s, options %#lx, device %p, alloc_hier %p, "
            "load_user_data %p, frame_hierarchy %p, anim_controller %p.\n",
            debugstr_a(filename), options, device, alloc_hier,
            load_user_data, frame_hierarchy, anim_controller);

    if (!filename)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filenameW = malloc(len * sizeof(WCHAR));
    if (!filenameW) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, len);

    hr = D3DXLoadMeshHierarchyFromXW(filenameW, options, device,
            alloc_hier, load_user_data, frame_hierarchy, anim_controller);
    free(filenameW);

    return hr;
}

HRESULT WINAPI D3DXLoadMeshHierarchyFromXW(const WCHAR *filename, DWORD options, struct IDirect3DDevice9 *device,
        struct ID3DXAllocateHierarchy *alloc_hier, struct ID3DXLoadUserData *load_user_data,
        D3DXFRAME **frame_hierarchy, struct ID3DXAnimationController **anim_controller)
{
    void *buffer;
    HRESULT hr;
    DWORD size;

    TRACE("filename %s, options %#lx, device %p, alloc_hier %p, "
            "load_user_data %p, frame_hierarchy %p, anim_controller %p.\n",
            debugstr_w(filename), options, device, alloc_hier,
            load_user_data, frame_hierarchy, anim_controller);

    if (!filename)
        return D3DERR_INVALIDCALL;

    hr = map_view_of_file(filename, &buffer, &size);
    if (FAILED(hr))
        return D3DXERR_INVALIDDATA;

    hr = D3DXLoadMeshHierarchyFromXInMemory(buffer, size, options, device,
            alloc_hier, load_user_data, frame_hierarchy, anim_controller);

    UnmapViewOfFile(buffer);

    return hr;
}

static HRESULT filedata_get_name(ID3DXFileData *filedata, char **name)
{
    HRESULT hr;
    SIZE_T name_len;

    hr = filedata->lpVtbl->GetName(filedata, NULL, &name_len);
    if (FAILED(hr)) return hr;

    if (!name_len)
        name_len++;
    *name = malloc(name_len);
    if (!*name) return E_OUTOFMEMORY;

    hr = filedata->lpVtbl->GetName(filedata, *name, &name_len);
    if (FAILED(hr))
        free(*name);
    else if (!name_len)
        (*name)[0] = 0;

    return hr;
}

static HRESULT load_mesh_container(struct ID3DXFileData *filedata, DWORD options, struct IDirect3DDevice9 *device,
        struct ID3DXAllocateHierarchy *alloc_hier, D3DXMESHCONTAINER **mesh_container,
        struct ID3DXLoadUserData *load_user_data)
{
    HRESULT hr;
    ID3DXBuffer *adjacency = NULL;
    ID3DXBuffer *materials = NULL;
    ID3DXBuffer *effects = NULL;
    ID3DXSkinInfo *skin_info = NULL;
    D3DXMESHDATA mesh_data;
    DWORD num_materials = 0;
    char *name = NULL;
    SIZE_T child_count;
    ID3DXFileData *child = NULL;
    GUID type;
    unsigned int i;

    mesh_data.Type = D3DXMESHTYPE_MESH;
    mesh_data.pMesh = NULL;

    hr = D3DXLoadSkinMeshFromXof(filedata, options, device,
            &adjacency, &materials, &effects, &num_materials,
            &skin_info, &mesh_data.pMesh);
    if (FAILED(hr)) return hr;

    hr = filedata_get_name(filedata, &name);
    if (FAILED(hr)) goto cleanup;

    if (!mesh_data.pMesh)
        goto cleanup;
    hr = alloc_hier->lpVtbl->CreateMeshContainer(alloc_hier, name, &mesh_data,
            materials ? ID3DXBuffer_GetBufferPointer(materials) : NULL,
            effects ? ID3DXBuffer_GetBufferPointer(effects) : NULL,
            num_materials,
            adjacency ? ID3DXBuffer_GetBufferPointer(adjacency) : NULL,
            skin_info, mesh_container);
    if (FAILED(hr) || !load_user_data)
        goto cleanup;

    hr = filedata->lpVtbl->GetChildren(filedata, &child_count);
    if (FAILED(hr))
        goto cleanup;

    for (i = 0; i < child_count; i++)
    {
        if (FAILED(hr = filedata->lpVtbl->GetChild(filedata, i, &child)))
            goto cleanup;
        if (FAILED(hr = child->lpVtbl->GetType(child, &type)))
            goto cleanup;

        if (!mesh_get_parse_func(&type)
                && FAILED(hr = load_user_data->lpVtbl->LoadMeshChildData(load_user_data, *mesh_container, child)))
            goto cleanup;

        IUnknown_Release(child);
        child = NULL;
    }

cleanup:
    if (child) IUnknown_Release(child);
    if (materials) ID3DXBuffer_Release(materials);
    if (effects) ID3DXBuffer_Release(effects);
    if (adjacency) ID3DXBuffer_Release(adjacency);
    if (skin_info) IUnknown_Release(skin_info);
    if (mesh_data.pMesh) IUnknown_Release(mesh_data.pMesh);
    free(name);
    return hr;
}

static HRESULT parse_transform_matrix(ID3DXFileData *filedata, D3DXMATRIX *transform)
{
    SIZE_T data_size;
    const BYTE *data;
    HRESULT hr;

    /* template Matrix4x4 {
     *     array FLOAT matrix[16];
     * }
     * template FrameTransformMatrix {
     *     Matrix4x4 frameMatrix;
     * }
     */

    hr = filedata->lpVtbl->Lock(filedata, &data_size, (const void **)&data);
    if (FAILED(hr))
        return hr;

    if (data_size != sizeof(D3DXMATRIX))
    {
        WARN("Incorrect data size (%Id bytes).\n", data_size);
        filedata->lpVtbl->Unlock(filedata);
        return E_FAIL;
    }

    memcpy(transform, data, sizeof(D3DXMATRIX));

    filedata->lpVtbl->Unlock(filedata);
    return D3D_OK;
}

static HRESULT load_frame(struct ID3DXFileData *filedata, DWORD options, struct IDirect3DDevice9 *device,
        struct ID3DXAllocateHierarchy *alloc_hier, D3DXFRAME **frame_out, struct ID3DXLoadUserData *load_user_data)
{
    HRESULT hr;
    GUID type;
    ID3DXFileData *child;
    char *name = NULL;
    D3DXFRAME *frame = NULL;
    D3DXMESHCONTAINER **next_container;
    D3DXFRAME **next_child;
    SIZE_T i, nb_children;

    hr = filedata_get_name(filedata, &name);
    if (FAILED(hr)) return hr;

    hr = alloc_hier->lpVtbl->CreateFrame(alloc_hier, name, frame_out);
    free(name);
    if (FAILED(hr)) return E_FAIL;

    frame = *frame_out;
    D3DXMatrixIdentity(&frame->TransformationMatrix);
    next_child = &frame->pFrameFirstChild;
    next_container = &frame->pMeshContainer;

    hr = filedata->lpVtbl->GetChildren(filedata, &nb_children);
    if (FAILED(hr))
        return hr;

    for (i = 0; i < nb_children; i++)
    {
        hr = filedata->lpVtbl->GetChild(filedata, i, &child);
        if (FAILED(hr))
            return hr;
        hr = child->lpVtbl->GetType(child, &type);
        if (FAILED(hr))
            goto err;

        if (IsEqualGUID(&type, &TID_D3DRMMesh)) {
            hr = load_mesh_container(child, options, device, alloc_hier, next_container, load_user_data);
            if (SUCCEEDED(hr))
                next_container = &(*next_container)->pNextMeshContainer;
        } else if (IsEqualGUID(&type, &TID_D3DRMFrameTransformMatrix)) {
            hr = parse_transform_matrix(child, &frame->TransformationMatrix);
        } else if (IsEqualGUID(&type, &TID_D3DRMFrame)) {
            hr = load_frame(child, options, device, alloc_hier, next_child, load_user_data);
            if (SUCCEEDED(hr))
                next_child = &(*next_child)->pFrameSibling;
        } else if (load_user_data) {
            TRACE("Loading %s as user data.\n", debugstr_guid(&type));
            hr = load_user_data->lpVtbl->LoadFrameChildData(load_user_data, frame, child);
        }
        if (FAILED(hr))
            goto err;

        IUnknown_Release(child);
    }
    return D3D_OK;

err:
    IUnknown_Release(child);
    return hr;
}

HRESULT WINAPI D3DXLoadMeshHierarchyFromXInMemory(const void *memory, DWORD memory_size, DWORD options,
        struct IDirect3DDevice9 *device, struct ID3DXAllocateHierarchy *alloc_hier,
        struct ID3DXLoadUserData *load_user_data, D3DXFRAME **frame_hierarchy,
        struct ID3DXAnimationController **anim_controller)
{
    HRESULT hr;
    ID3DXFile *d3dxfile = NULL;
    ID3DXFileEnumObject *enumobj = NULL;
    ID3DXFileData *filedata = NULL;
    D3DXF_FILELOADMEMORY source;
    D3DXFRAME *first_frame = NULL;
    D3DXFRAME **next_frame = &first_frame;
    SIZE_T i, nb_children;
    GUID guid;

    TRACE("memory %p, memory_size %lu, options %#lx, device %p, alloc_hier %p, load_user_data %p, "
            "frame_hierarchy %p, anim_controller %p.\n", memory, memory_size, options,
            device, alloc_hier, load_user_data, frame_hierarchy, anim_controller);

    if (!memory || !memory_size || !device || !frame_hierarchy || !alloc_hier)
        return D3DERR_INVALIDCALL;

    hr = D3DXFileCreate(&d3dxfile);
    if (FAILED(hr)) goto cleanup;

    hr = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES);
    if (FAILED(hr)) goto cleanup;

    source.lpMemory = (void*)memory;
    source.dSize = memory_size;
    hr = d3dxfile->lpVtbl->CreateEnumObject(d3dxfile, &source, D3DXF_FILELOAD_FROMMEMORY, &enumobj);
    if (FAILED(hr)) goto cleanup;

    hr = enumobj->lpVtbl->GetChildren(enumobj, &nb_children);
    if (FAILED(hr))
        goto cleanup;

    for (i = 0; i < nb_children; i++)
    {
        hr = enumobj->lpVtbl->GetChild(enumobj, i, &filedata);
        if (FAILED(hr))
            goto cleanup;

        hr = filedata->lpVtbl->GetType(filedata, &guid);
        if (SUCCEEDED(hr)) {
            if (IsEqualGUID(&guid, &TID_D3DRMMesh)) {
                hr = alloc_hier->lpVtbl->CreateFrame(alloc_hier, NULL, next_frame);
                if (FAILED(hr)) {
                    hr = E_FAIL;
                    goto cleanup;
                }

                D3DXMatrixIdentity(&(*next_frame)->TransformationMatrix);

                hr = load_mesh_container(filedata, options, device, alloc_hier, &(*next_frame)->pMeshContainer,
                        load_user_data);
                if (FAILED(hr)) goto cleanup;
            } else if (IsEqualGUID(&guid, &TID_D3DRMFrame)) {
                hr = load_frame(filedata, options, device, alloc_hier, next_frame, load_user_data);
                if (FAILED(hr)) goto cleanup;
            } else if (load_user_data) {
                TRACE("Loading %s as user data.\n", debugstr_guid(&guid));
                hr = load_user_data->lpVtbl->LoadTopLevelData(load_user_data, filedata);
            }
            while (*next_frame)
                next_frame = &(*next_frame)->pFrameSibling;
        }

        filedata->lpVtbl->Release(filedata);
        filedata = NULL;
        if (FAILED(hr))
            goto cleanup;
    }

    if (!first_frame) {
        hr = E_FAIL;
    } else if (first_frame->pFrameSibling) {
        D3DXFRAME *root_frame = NULL;
        hr = alloc_hier->lpVtbl->CreateFrame(alloc_hier, NULL, &root_frame);
        if (FAILED(hr)) {
            hr = E_FAIL;
            goto cleanup;
        }
        D3DXMatrixIdentity(&root_frame->TransformationMatrix);
        root_frame->pFrameFirstChild = first_frame;
        *frame_hierarchy = root_frame;
        hr = D3D_OK;
    } else {
        *frame_hierarchy = first_frame;
        hr = D3D_OK;
    }

    if (anim_controller)
    {
        *anim_controller = NULL;
        FIXME("Animation controller creation not implemented.\n");
    }

cleanup:
    if (FAILED(hr) && first_frame) D3DXFrameDestroy(first_frame, alloc_hier);
    if (filedata) filedata->lpVtbl->Release(filedata);
    if (enumobj) enumobj->lpVtbl->Release(enumobj);
    if (d3dxfile) d3dxfile->lpVtbl->Release(d3dxfile);
    return hr;
}

HRESULT WINAPI D3DXCleanMesh(D3DXCLEANTYPE clean_type, ID3DXMesh *mesh_in, const DWORD *adjacency_in,
        ID3DXMesh **mesh_out, DWORD *adjacency_out, ID3DXBuffer **errors_and_warnings)
{
    FIXME("(%u, %p, %p, %p, %p, %p)\n", clean_type, mesh_in, adjacency_in, mesh_out, adjacency_out, errors_and_warnings);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DXFrameDestroy(D3DXFRAME *frame, ID3DXAllocateHierarchy *alloc_hier)
{
    HRESULT hr;
    BOOL last = FALSE;

    TRACE("(%p, %p)\n", frame, alloc_hier);

    if (!frame || !alloc_hier)
        return D3DERR_INVALIDCALL;

    while (!last) {
        D3DXMESHCONTAINER *container;
        D3DXFRAME *current_frame;

        if (frame->pFrameSibling) {
            current_frame = frame->pFrameSibling;
            frame->pFrameSibling = current_frame->pFrameSibling;
            current_frame->pFrameSibling = NULL;
        } else {
            current_frame = frame;
            last = TRUE;
        }

        if (current_frame->pFrameFirstChild) {
            hr = D3DXFrameDestroy(current_frame->pFrameFirstChild, alloc_hier);
            if (FAILED(hr)) return hr;
            current_frame->pFrameFirstChild = NULL;
        }

        container = current_frame->pMeshContainer;
        while (container) {
            D3DXMESHCONTAINER *next_container = container->pNextMeshContainer;
            hr = alloc_hier->lpVtbl->DestroyMeshContainer(alloc_hier, container);
            if (FAILED(hr)) return hr;
            container = next_container;
        }
        hr = alloc_hier->lpVtbl->DestroyFrame(alloc_hier, current_frame);
        if (FAILED(hr)) return hr;
    }
    return D3D_OK;
}

HRESULT WINAPI D3DXLoadMeshFromXA(const char *filename, DWORD options, struct IDirect3DDevice9 *device,
        struct ID3DXBuffer **adjacency, struct ID3DXBuffer **materials, struct ID3DXBuffer **effect_instances,
        DWORD *num_materials, struct ID3DXMesh **mesh)
{
    WCHAR *filenameW;
    HRESULT hr;
    int len;

    TRACE("filename %s, options %#lx, device %p, adjacency %p, materials %p, "
            "effect_instances %p, num_materials %p, mesh %p.\n",
            debugstr_a(filename), options, device, adjacency, materials,
            effect_instances, num_materials, mesh);

    if (!filename)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filenameW = malloc(len * sizeof(WCHAR));
    if (!filenameW) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, len);

    hr = D3DXLoadMeshFromXW(filenameW, options, device, adjacency, materials,
                            effect_instances, num_materials, mesh);
    free(filenameW);

    return hr;
}

HRESULT WINAPI D3DXLoadMeshFromXW(const WCHAR *filename, DWORD options, struct IDirect3DDevice9 *device,
        struct ID3DXBuffer **adjacency, struct ID3DXBuffer **materials, struct ID3DXBuffer **effect_instances,
        DWORD *num_materials, struct ID3DXMesh **mesh)
{
    void *buffer;
    HRESULT hr;
    DWORD size;

    TRACE("filename %s, options %#lx, device %p, adjacency %p, materials %p, "
            "effect_instances %p, num_materials %p, mesh %p.\n",
            debugstr_w(filename), options, device, adjacency, materials,
            effect_instances, num_materials, mesh);

    if (!filename)
        return D3DERR_INVALIDCALL;

    hr = map_view_of_file(filename, &buffer, &size);
    if (FAILED(hr))
        return D3DXERR_INVALIDDATA;

    hr = D3DXLoadMeshFromXInMemory(buffer, size, options, device, adjacency,
            materials, effect_instances, num_materials, mesh);

    UnmapViewOfFile(buffer);

    return hr;
}

HRESULT WINAPI D3DXLoadMeshFromXResource(HMODULE module, const char *name, const char *type, DWORD options,
        struct IDirect3DDevice9 *device, struct ID3DXBuffer **adjacency, struct ID3DXBuffer **materials,
        struct ID3DXBuffer **effect_instances, DWORD *num_materials, struct ID3DXMesh **mesh)
{
    HRESULT hr;
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("module %p, name %s, type %s, options %#lx, device %p, adjacency %p, "
            "materials %p, effect_instances %p, num_materials %p, mesh %p.\n",
            module, debugstr_a(name), debugstr_a(type), options, device, adjacency,
            materials, effect_instances, num_materials, mesh);

    resinfo = FindResourceA(module, name, type);
    if (!resinfo) return D3DXERR_INVALIDDATA;

    hr = load_resource_into_memory(module, resinfo, &buffer, &size);
    if (FAILED(hr)) return D3DXERR_INVALIDDATA;

    return D3DXLoadMeshFromXInMemory(buffer, size, options, device, adjacency,
            materials, effect_instances, num_materials, mesh);
}

struct mesh_container
{
    struct list entry;
    ID3DXMesh *mesh;
    ID3DXBuffer *adjacency;
    ID3DXBuffer *materials;
    ID3DXBuffer *effects;
    DWORD num_materials;
    D3DXMATRIX transform;
};

static HRESULT parse_frame(struct ID3DXFileData *filedata, DWORD options, struct IDirect3DDevice9 *device,
        const D3DXMATRIX *parent_transform, struct list *container_list, DWORD provide_flags)
{
    HRESULT hr;
    D3DXMATRIX transform = *parent_transform;
    ID3DXFileData *child;
    GUID type;
    SIZE_T i, nb_children;

    hr = filedata->lpVtbl->GetChildren(filedata, &nb_children);
    if (FAILED(hr))
        return hr;

    for (i = 0; i < nb_children; i++)
    {
        hr = filedata->lpVtbl->GetChild(filedata, i, &child);
        if (FAILED(hr))
            return hr;
        hr = child->lpVtbl->GetType(child, &type);
        if (FAILED(hr))
            goto err;

        if (IsEqualGUID(&type, &TID_D3DRMMesh)) {
            struct mesh_container *container = calloc(1, sizeof(*container));
            if (!container)
            {
                hr = E_OUTOFMEMORY;
                goto err;
            }

            hr = D3DXLoadSkinMeshFromXof(child, options, device,
                    (provide_flags & PROVIDE_ADJACENCY) ? &container->adjacency : NULL,
                    (provide_flags & PROVIDE_MATERIALS) ? &container->materials : NULL,
                    NULL, &container->num_materials, NULL, &container->mesh);

            if (container->mesh)
            {
                list_add_tail(container_list, &container->entry);
                container->transform = transform;
            }
            else
            {
                free(container);
            }
        } else if (IsEqualGUID(&type, &TID_D3DRMFrameTransformMatrix)) {
            D3DXMATRIX new_transform;
            hr = parse_transform_matrix(child, &new_transform);
            D3DXMatrixMultiply(&transform, &transform, &new_transform);
        } else if (IsEqualGUID(&type, &TID_D3DRMFrame)) {
            hr = parse_frame(child, options, device, &transform, container_list, provide_flags);
        }
        if (FAILED(hr))
            goto err;

        IUnknown_Release(child);
    }
    return D3D_OK;

err:
    IUnknown_Release(child);
    return hr;
}

HRESULT WINAPI D3DXLoadMeshFromXInMemory(const void *memory, DWORD memory_size, DWORD options,
        struct IDirect3DDevice9 *device, struct ID3DXBuffer **adjacency_out, struct ID3DXBuffer **materials_out,
        struct ID3DXBuffer **effects_out, DWORD *num_materials_out, struct ID3DXMesh **mesh_out)
{
    HRESULT hr;
    ID3DXFile *d3dxfile = NULL;
    ID3DXFileEnumObject *enumobj = NULL;
    ID3DXFileData *filedata = NULL;
    D3DXF_FILELOADMEMORY source;
    ID3DXBuffer *materials = NULL;
    ID3DXBuffer *effects = NULL;
    ID3DXBuffer *adjacency = NULL;
    struct list container_list = LIST_INIT(container_list);
    struct mesh_container *container_ptr, *next_container_ptr;
    DWORD num_materials;
    DWORD num_faces, num_vertices;
    D3DXMATRIX identity;
    DWORD provide_flags = 0;
    DWORD fvf;
    ID3DXMesh *concat_mesh = NULL;
    D3DVERTEXELEMENT9 concat_decl[MAX_FVF_DECL_SIZE];
    BYTE *concat_vertices = NULL;
    void *concat_indices = NULL;
    DWORD index_offset;
    DWORD concat_vertex_size;
    SIZE_T i, nb_children;
    GUID guid;

    TRACE("memory %p, memory_size %lu, options %#lx, device %p, adjacency_out %p, materials_out %p, "
            "effects_out %p, num_materials_out %p, mesh_out %p.\n", memory, memory_size, options,
            device, adjacency_out, materials_out, effects_out, num_materials_out, mesh_out);

    if (!memory || !memory_size || !device || !mesh_out)
        return D3DERR_INVALIDCALL;

    hr = D3DXFileCreate(&d3dxfile);
    if (FAILED(hr)) goto cleanup;

    hr = d3dxfile->lpVtbl->RegisterTemplates(d3dxfile, D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES);
    if (FAILED(hr)) goto cleanup;

    source.lpMemory = (void*)memory;
    source.dSize = memory_size;
    hr = d3dxfile->lpVtbl->CreateEnumObject(d3dxfile, &source, D3DXF_FILELOAD_FROMMEMORY, &enumobj);
    if (FAILED(hr)) goto cleanup;

    D3DXMatrixIdentity(&identity);
    if (adjacency_out) provide_flags |= PROVIDE_ADJACENCY;
    if (materials_out || effects_out) provide_flags |= PROVIDE_MATERIALS;

    hr = enumobj->lpVtbl->GetChildren(enumobj, &nb_children);
    if (FAILED(hr))
        goto cleanup;

    for (i = 0; i < nb_children; i++)
    {
        hr = enumobj->lpVtbl->GetChild(enumobj, i, &filedata);
        if (FAILED(hr))
            goto cleanup;

        hr = filedata->lpVtbl->GetType(filedata, &guid);
        if (SUCCEEDED(hr)) {
            if (IsEqualGUID(&guid, &TID_D3DRMMesh)) {
                container_ptr = calloc(1, sizeof(*container_ptr));
                if (!container_ptr) {
                    hr = E_OUTOFMEMORY;
                    goto cleanup;
                }

                hr = D3DXLoadSkinMeshFromXof(filedata, options, device,
                        (provide_flags & PROVIDE_ADJACENCY) ? &container_ptr->adjacency : NULL,
                        (provide_flags & PROVIDE_MATERIALS) ? &container_ptr->materials : NULL,
                        NULL, &container_ptr->num_materials, NULL, &container_ptr->mesh);
                if (container_ptr->mesh)
                {
                    list_add_tail(&container_list, &container_ptr->entry);
                    D3DXMatrixIdentity(&container_ptr->transform);
                }
                else
                {
                    free(container_ptr);
                }
            } else if (IsEqualGUID(&guid, &TID_D3DRMFrame)) {
                hr = parse_frame(filedata, options, device, &identity, &container_list, provide_flags);
            }
            if (FAILED(hr)) goto cleanup;
        }
        filedata->lpVtbl->Release(filedata);
        filedata = NULL;
        if (FAILED(hr))
            goto cleanup;
    }

    enumobj->lpVtbl->Release(enumobj);
    enumobj = NULL;
    d3dxfile->lpVtbl->Release(d3dxfile);
    d3dxfile = NULL;

    if (list_empty(&container_list)) {
        hr = E_FAIL;
        goto cleanup;
    }

    fvf = D3DFVF_XYZ;
    num_faces = 0;
    num_vertices = 0;
    num_materials = 0;
    LIST_FOR_EACH_ENTRY(container_ptr, &container_list, struct mesh_container, entry)
    {
        ID3DXMesh *mesh = container_ptr->mesh;
        fvf |= mesh->lpVtbl->GetFVF(mesh);
        num_faces += mesh->lpVtbl->GetNumFaces(mesh);
        num_vertices += mesh->lpVtbl->GetNumVertices(mesh);
        num_materials += container_ptr->num_materials;
    }

    hr = D3DXCreateMeshFVF(num_faces, num_vertices, options, fvf, device, &concat_mesh);
    if (FAILED(hr)) goto cleanup;

    hr = concat_mesh->lpVtbl->GetDeclaration(concat_mesh, concat_decl);
    if (FAILED(hr)) goto cleanup;

    concat_vertex_size = D3DXGetDeclVertexSize(concat_decl, 0);

    hr = concat_mesh->lpVtbl->LockVertexBuffer(concat_mesh, 0, (void**)&concat_vertices);
    if (FAILED(hr)) goto cleanup;

    LIST_FOR_EACH_ENTRY(container_ptr, &container_list, struct mesh_container, entry)
    {
        D3DVERTEXELEMENT9 mesh_decl[MAX_FVF_DECL_SIZE];
        ID3DXMesh *mesh = container_ptr->mesh;
        DWORD num_mesh_vertices = mesh->lpVtbl->GetNumVertices(mesh);
        DWORD mesh_vertex_size;
        const BYTE *mesh_vertices;
        DWORD i;

        hr = mesh->lpVtbl->GetDeclaration(mesh, mesh_decl);
        if (FAILED(hr)) goto cleanup;

        mesh_vertex_size = D3DXGetDeclVertexSize(mesh_decl, 0);

        hr = mesh->lpVtbl->LockVertexBuffer(mesh, D3DLOCK_READONLY, (void**)&mesh_vertices);
        if (FAILED(hr)) goto cleanup;

        for (i = 0; i < num_mesh_vertices; i++) {
            int j;
            int k = 1;

            D3DXVec3TransformCoord((D3DXVECTOR3*)concat_vertices,
                                   (D3DXVECTOR3*)mesh_vertices,
                                   &container_ptr->transform);
            for (j = 1; concat_decl[j].Stream != 0xff; j++)
            {
                if (concat_decl[j].Usage == mesh_decl[k].Usage &&
                    concat_decl[j].UsageIndex == mesh_decl[k].UsageIndex)
                {
                    if (concat_decl[j].Usage == D3DDECLUSAGE_NORMAL) {
                        D3DXVec3TransformCoord((D3DXVECTOR3*)(concat_vertices + concat_decl[j].Offset),
                                               (D3DXVECTOR3*)(mesh_vertices + mesh_decl[k].Offset),
                                               &container_ptr->transform);
                    } else {
                        memcpy(concat_vertices + concat_decl[j].Offset,
                               mesh_vertices + mesh_decl[k].Offset,
                               d3dx_decltype_size[mesh_decl[k].Type]);
                    }
                    k++;
                }
            }
            mesh_vertices += mesh_vertex_size;
            concat_vertices += concat_vertex_size;
        }

        mesh->lpVtbl->UnlockVertexBuffer(mesh);
    }

    concat_mesh->lpVtbl->UnlockVertexBuffer(concat_mesh);
    concat_vertices = NULL;

    hr = concat_mesh->lpVtbl->LockIndexBuffer(concat_mesh, 0, &concat_indices);
    if (FAILED(hr)) goto cleanup;

    index_offset = 0;
    LIST_FOR_EACH_ENTRY(container_ptr, &container_list, struct mesh_container, entry)
    {
        ID3DXMesh *mesh = container_ptr->mesh;
        const void *mesh_indices;
        DWORD num_mesh_faces = mesh->lpVtbl->GetNumFaces(mesh);
        DWORD i;

        hr = mesh->lpVtbl->LockIndexBuffer(mesh, D3DLOCK_READONLY, (void**)&mesh_indices);
        if (FAILED(hr)) goto cleanup;

        if (options & D3DXMESH_32BIT) {
            DWORD *dest = concat_indices;
            const DWORD *src = mesh_indices;
            for (i = 0; i < num_mesh_faces * 3; i++)
                *dest++ = index_offset + *src++;
            concat_indices = dest;
        } else {
            WORD *dest = concat_indices;
            const WORD *src = mesh_indices;
            for (i = 0; i < num_mesh_faces * 3; i++)
                *dest++ = index_offset + *src++;
            concat_indices = dest;
        }
        mesh->lpVtbl->UnlockIndexBuffer(mesh);

        index_offset += num_mesh_faces * 3;
    }

    concat_mesh->lpVtbl->UnlockIndexBuffer(concat_mesh);
    concat_indices = NULL;

    if (num_materials) {
        DWORD *concat_attrib_buffer = NULL;
        DWORD offset = 0;

        hr = concat_mesh->lpVtbl->LockAttributeBuffer(concat_mesh, 0, &concat_attrib_buffer);
        if (FAILED(hr)) goto cleanup;

        LIST_FOR_EACH_ENTRY(container_ptr, &container_list, struct mesh_container, entry)
        {
            ID3DXMesh *mesh = container_ptr->mesh;
            const DWORD *mesh_attrib_buffer = NULL;
            DWORD count = mesh->lpVtbl->GetNumFaces(mesh);

            hr = mesh->lpVtbl->LockAttributeBuffer(mesh, D3DLOCK_READONLY, (DWORD**)&mesh_attrib_buffer);
            if (FAILED(hr)) {
                concat_mesh->lpVtbl->UnlockAttributeBuffer(concat_mesh);
                goto cleanup;
            }

            while (count--)
                *concat_attrib_buffer++ = offset + *mesh_attrib_buffer++;

            mesh->lpVtbl->UnlockAttributeBuffer(mesh);
            offset += container_ptr->num_materials;
        }
        concat_mesh->lpVtbl->UnlockAttributeBuffer(concat_mesh);
    }

    if (materials_out || effects_out) {
        D3DXMATERIAL *out_ptr;
        if (!num_materials) {
            /* create default material */
            hr = D3DXCreateBuffer(sizeof(D3DXMATERIAL), &materials);
            if (FAILED(hr)) goto cleanup;

            out_ptr = ID3DXBuffer_GetBufferPointer(materials);
            out_ptr->MatD3D.Diffuse.r = 0.5f;
            out_ptr->MatD3D.Diffuse.g = 0.5f;
            out_ptr->MatD3D.Diffuse.b = 0.5f;
            out_ptr->MatD3D.Specular.r = 0.5f;
            out_ptr->MatD3D.Specular.g = 0.5f;
            out_ptr->MatD3D.Specular.b = 0.5f;
            /* D3DXCreateBuffer initializes the rest to zero */
        } else {
            DWORD buffer_size = num_materials * sizeof(D3DXMATERIAL);
            char *strings_out_ptr;

            LIST_FOR_EACH_ENTRY(container_ptr, &container_list, struct mesh_container, entry)
            {
                if (container_ptr->materials) {
                    DWORD i;
                    const D3DXMATERIAL *in_ptr = ID3DXBuffer_GetBufferPointer(container_ptr->materials);
                    for (i = 0; i < container_ptr->num_materials; i++)
                    {
                        if (in_ptr->pTextureFilename)
                            buffer_size += strlen(in_ptr->pTextureFilename) + 1;
                        in_ptr++;
                    }
                }
            }

            hr = D3DXCreateBuffer(buffer_size, &materials);
            if (FAILED(hr)) goto cleanup;
            out_ptr = ID3DXBuffer_GetBufferPointer(materials);
            strings_out_ptr = (char*)(out_ptr + num_materials);

            LIST_FOR_EACH_ENTRY(container_ptr, &container_list, struct mesh_container, entry)
            {
                if (container_ptr->materials) {
                    DWORD i;
                    const D3DXMATERIAL *in_ptr = ID3DXBuffer_GetBufferPointer(container_ptr->materials);
                    for (i = 0; i < container_ptr->num_materials; i++)
                    {
                        out_ptr->MatD3D = in_ptr->MatD3D;
                        if (in_ptr->pTextureFilename) {
                            out_ptr->pTextureFilename = strings_out_ptr;
                            strcpy(out_ptr->pTextureFilename, in_ptr->pTextureFilename);
                            strings_out_ptr += strlen(in_ptr->pTextureFilename) + 1;
                        }
                        in_ptr++;
                        out_ptr++;
                    }
                }
            }
        }
    }
    if (!num_materials)
        num_materials = 1;

    if (effects_out) {
        generate_effects(materials, num_materials, &effects);
        if (!materials_out) {
            ID3DXBuffer_Release(materials);
            materials = NULL;
        }
    }

    if (adjacency_out) {
        if (!list_next(&container_list, list_head(&container_list))) {
            container_ptr = LIST_ENTRY(list_head(&container_list), struct mesh_container, entry);
            adjacency = container_ptr->adjacency;
            container_ptr->adjacency = NULL;
        } else {
            DWORD offset = 0;
            DWORD *out_ptr;

            hr = D3DXCreateBuffer(num_faces * 3 * sizeof(DWORD), &adjacency);
            if (FAILED(hr)) goto cleanup;

            out_ptr = ID3DXBuffer_GetBufferPointer(adjacency);
            LIST_FOR_EACH_ENTRY(container_ptr, &container_list, struct mesh_container, entry)
            {
                DWORD i;
                DWORD count = 3 * container_ptr->mesh->lpVtbl->GetNumFaces(container_ptr->mesh);
                DWORD *in_ptr = ID3DXBuffer_GetBufferPointer(container_ptr->adjacency);

                for (i = 0; i < count; i++)
                    *out_ptr++ = offset + *in_ptr++;

                offset += count;
            }
        }
    }

    *mesh_out = concat_mesh;
    if (adjacency_out) *adjacency_out = adjacency;
    if (materials_out) *materials_out = materials;
    if (effects_out) *effects_out = effects;
    if (num_materials_out) *num_materials_out = num_materials;

    hr = D3D_OK;
cleanup:
    if (concat_indices) concat_mesh->lpVtbl->UnlockIndexBuffer(concat_mesh);
    if (concat_vertices) concat_mesh->lpVtbl->UnlockVertexBuffer(concat_mesh);
    if (filedata) filedata->lpVtbl->Release(filedata);
    if (enumobj) enumobj->lpVtbl->Release(enumobj);
    if (d3dxfile) d3dxfile->lpVtbl->Release(d3dxfile);
    if (FAILED(hr)) {
        if (concat_mesh) IUnknown_Release(concat_mesh);
        if (materials) ID3DXBuffer_Release(materials);
        if (effects) ID3DXBuffer_Release(effects);
        if (adjacency) ID3DXBuffer_Release(adjacency);
    }
    LIST_FOR_EACH_ENTRY_SAFE(container_ptr, next_container_ptr, &container_list, struct mesh_container, entry)
    {
        if (container_ptr->mesh) IUnknown_Release(container_ptr->mesh);
        if (container_ptr->adjacency) ID3DXBuffer_Release(container_ptr->adjacency);
        if (container_ptr->materials) ID3DXBuffer_Release(container_ptr->materials);
        if (container_ptr->effects) ID3DXBuffer_Release(container_ptr->effects);
        free(container_ptr);
    }
    return hr;
}

struct vertex
{
    D3DXVECTOR3 position;
    D3DXVECTOR3 normal;
};

HRESULT WINAPI D3DXCreatePolygon(struct IDirect3DDevice9 *device, float length, UINT sides,
        struct ID3DXMesh **mesh, struct ID3DXBuffer **adjacency)
{
    HRESULT hr;
    ID3DXMesh *polygon;
    struct vertex *vertices;
    WORD (*faces)[3];
    DWORD (*adjacency_buf)[3];
    float angle, scale;
    unsigned int i;

    TRACE("device %p, length %f, sides %u, mesh %p, adjacency %p.\n",
            device, length, sides, mesh, adjacency);

    if (!device || length < 0.0f || sides < 3 || !mesh)
        return D3DERR_INVALIDCALL;

    if (FAILED(hr = D3DXCreateMeshFVF(sides, sides + 1, D3DXMESH_MANAGED,
            D3DFVF_XYZ | D3DFVF_NORMAL, device, &polygon)))
    {
        return hr;
    }

    if (FAILED(hr = polygon->lpVtbl->LockVertexBuffer(polygon, 0, (void **)&vertices)))
    {
        polygon->lpVtbl->Release(polygon);
        return hr;
    }

    if (FAILED(hr = polygon->lpVtbl->LockIndexBuffer(polygon, 0, (void **)&faces)))
    {
        polygon->lpVtbl->UnlockVertexBuffer(polygon);
        polygon->lpVtbl->Release(polygon);
        return hr;
    }

    angle = D3DX_PI / sides;
    scale = 0.5f * length / sinf(angle);
    angle *= 2.0f;

    vertices[0].position.x = 0.0f;
    vertices[0].position.y = 0.0f;
    vertices[0].position.z = 0.0f;
    vertices[0].normal.x = 0.0f;
    vertices[0].normal.y = 0.0f;
    vertices[0].normal.z = 1.0f;

    for (i = 0; i < sides; ++i)
    {
        vertices[i + 1].position.x = cosf(angle * i) * scale;
        vertices[i + 1].position.y = sinf(angle * i) * scale;
        vertices[i + 1].position.z = 0.0f;
        vertices[i + 1].normal.x = 0.0f;
        vertices[i + 1].normal.y = 0.0f;
        vertices[i + 1].normal.z = 1.0f;

        faces[i][0] = 0;
        faces[i][1] = i + 1;
        faces[i][2] = i + 2;
    }

    faces[sides - 1][2] = 1;

    polygon->lpVtbl->UnlockVertexBuffer(polygon);
    polygon->lpVtbl->UnlockIndexBuffer(polygon);

    if (adjacency)
    {
        if (FAILED(hr = D3DXCreateBuffer(sides * sizeof(DWORD) * 3, adjacency)))
        {
            polygon->lpVtbl->Release(polygon);
            return hr;
        }

        adjacency_buf = ID3DXBuffer_GetBufferPointer(*adjacency);
        for (i = 0; i < sides; ++i)
        {
            adjacency_buf[i][0] = i - 1;
            adjacency_buf[i][1] = ~0U;
            adjacency_buf[i][2] = i + 1;
        }
        adjacency_buf[0][0] = sides - 1;
        adjacency_buf[sides - 1][2] = 0;
    }

    *mesh = polygon;

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateBox(struct IDirect3DDevice9 *device, float width, float height,
        float depth, struct ID3DXMesh **mesh, struct ID3DXBuffer **adjacency)
{
    HRESULT hr;
    ID3DXMesh *box;
    struct vertex *vertices;
    WORD (*faces)[3];
    DWORD *adjacency_buf;
    unsigned int i, face;
    static const D3DXVECTOR3 unit_box[] =
    {
        {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f},
        {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
        {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}
    };
    static const D3DXVECTOR3 normals[] =
    {
        {-1.0f,  0.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 1.0f, 0.0f,  0.0f},
        { 0.0f, -1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, -1.0f}
    };
    static const DWORD adjacency_table[] =
    {
        6, 9, 1, 2, 10, 0, 1,  9,  3, 4, 10,  2,
        3, 8, 5, 7, 11, 4, 0, 11,  7, 5,  8,  6,
        7, 4, 9, 2,  0, 8, 1,  3, 11, 5,  6, 10
    };

    TRACE("device %p, width %f, height %f, depth %f, mesh %p, adjacency %p\n",
                    device, width, height, depth, mesh, adjacency);

    if (!device || width < 0.0f || height < 0.0f || depth < 0.0f || !mesh)
    {
        return D3DERR_INVALIDCALL;
    }

    if (FAILED(hr = D3DXCreateMeshFVF(12, 24, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, &box)))
    {
        return hr;
    }

    if (FAILED(hr = box->lpVtbl->LockVertexBuffer(box, 0, (void **)&vertices)))
    {
        box->lpVtbl->Release(box);
        return hr;
    }

    if (FAILED(hr = box->lpVtbl->LockIndexBuffer(box, 0, (void **)&faces)))
    {
        box->lpVtbl->UnlockVertexBuffer(box);
        box->lpVtbl->Release(box);
        return hr;
    }

    for (i = 0; i < 24; i++)
    {
        vertices[i].position.x = width * unit_box[i].x;
        vertices[i].position.y = height * unit_box[i].y;
        vertices[i].position.z = depth * unit_box[i].z;
        vertices[i].normal.x = normals[i / 4].x;
        vertices[i].normal.y = normals[i / 4].y;
        vertices[i].normal.z = normals[i / 4].z;
    }

    face = 0;
    for (i = 0; i < 12; i++)
    {
        faces[i][0] = face++;
        faces[i][1] = face++;
        faces[i][2] = (i % 2) ? face - 4 : face;
    }

    box->lpVtbl->UnlockIndexBuffer(box);
    box->lpVtbl->UnlockVertexBuffer(box);

    if (adjacency)
    {
        if (FAILED(hr = D3DXCreateBuffer(sizeof(adjacency_table), adjacency)))
        {
            box->lpVtbl->Release(box);
            return hr;
        }

        adjacency_buf = ID3DXBuffer_GetBufferPointer(*adjacency);
        memcpy(adjacency_buf, adjacency_table, sizeof(adjacency_table));
    }

    *mesh = box;

    return D3D_OK;
}

typedef WORD face[3];

struct sincos_table
{
    float *sin;
    float *cos;
};

static void free_sincos_table(struct sincos_table *sincos_table)
{
    free(sincos_table->cos);
    free(sincos_table->sin);
}

/* pre compute sine and cosine tables; caller must free */
static BOOL compute_sincos_table(struct sincos_table *sincos_table, float angle_start, float angle_step, int n)
{
    float angle;
    int i;

    sincos_table->sin = malloc(n * sizeof(*sincos_table->sin));
    if (!sincos_table->sin)
    {
        return FALSE;
    }
    sincos_table->cos = malloc(n * sizeof(*sincos_table->cos));
    if (!sincos_table->cos)
    {
        free(sincos_table->sin);
        return FALSE;
    }

    angle = angle_start;
    for (i = 0; i < n; i++)
    {
        sincos_table->sin[i] = sinf(angle);
        sincos_table->cos[i] = cosf(angle);
        angle += angle_step;
    }

    return TRUE;
}

static WORD vertex_index(UINT slices, int slice, int stack)
{
    return stack*slices+slice+1;
}

HRESULT WINAPI D3DXCreateSphere(struct IDirect3DDevice9 *device, float radius, UINT slices,
        UINT stacks, struct ID3DXMesh **mesh, struct ID3DXBuffer **adjacency)
{
    DWORD number_of_vertices, number_of_faces;
    HRESULT hr;
    ID3DXMesh *sphere;
    struct vertex *vertices;
    face *faces;
    float phi_step, phi_start;
    struct sincos_table phi;
    float theta_step, theta, sin_theta, cos_theta;
    DWORD vertex, face, stack, slice;

    TRACE("(%p, %f, %u, %u, %p, %p)\n", device, radius, slices, stacks, mesh, adjacency);

    if (!device || radius < 0.0f || slices < 2 || stacks < 2 || !mesh)
    {
        return D3DERR_INVALIDCALL;
    }

    number_of_vertices = 2 + slices * (stacks-1);
    number_of_faces = 2 * slices + (stacks - 2) * (2 * slices);

    hr = D3DXCreateMeshFVF(number_of_faces, number_of_vertices, D3DXMESH_MANAGED,
                           D3DFVF_XYZ | D3DFVF_NORMAL, device, &sphere);
    if (FAILED(hr))
    {
        return hr;
    }

    if (FAILED(hr = sphere->lpVtbl->LockVertexBuffer(sphere, 0, (void **)&vertices)))
    {
        sphere->lpVtbl->Release(sphere);
        return hr;
    }

    if (FAILED(hr = sphere->lpVtbl->LockIndexBuffer(sphere, 0, (void **)&faces)))
    {
        sphere->lpVtbl->UnlockVertexBuffer(sphere);
        sphere->lpVtbl->Release(sphere);
        return hr;
    }

    /* phi = angle on xz plane wrt z axis */
    phi_step = -2.0f * D3DX_PI / slices;
    phi_start = D3DX_PI / 2.0f;

    if (!compute_sincos_table(&phi, phi_start, phi_step, slices))
    {
        sphere->lpVtbl->UnlockIndexBuffer(sphere);
        sphere->lpVtbl->UnlockVertexBuffer(sphere);
        sphere->lpVtbl->Release(sphere);
        return E_OUTOFMEMORY;
    }

    /* theta = angle on xy plane wrt x axis */
    theta_step = D3DX_PI / stacks;
    theta = theta_step;

    vertex = 0;
    face = 0;

    vertices[vertex].normal.x = 0.0f;
    vertices[vertex].normal.y = 0.0f;
    vertices[vertex].normal.z = 1.0f;
    vertices[vertex].position.x = 0.0f;
    vertices[vertex].position.y = 0.0f;
    vertices[vertex].position.z = radius;
    vertex++;

    for (stack = 0; stack < stacks - 1; stack++)
    {
        sin_theta = sinf(theta);
        cos_theta = cosf(theta);

        for (slice = 0; slice < slices; slice++)
        {
            vertices[vertex].normal.x = sin_theta * phi.cos[slice];
            vertices[vertex].normal.y = sin_theta * phi.sin[slice];
            vertices[vertex].normal.z = cos_theta;
            vertices[vertex].position.x = radius * sin_theta * phi.cos[slice];
            vertices[vertex].position.y = radius * sin_theta * phi.sin[slice];
            vertices[vertex].position.z = radius * cos_theta;
            vertex++;

            if (slice > 0)
            {
                if (stack == 0)
                {
                    /* top stack is triangle fan */
                    faces[face][0] = 0;
                    faces[face][1] = slice + 1;
                    faces[face][2] = slice;
                    face++;
                }
                else
                {
                    /* stacks in between top and bottom are quad strips */
                    faces[face][0] = vertex_index(slices, slice-1, stack-1);
                    faces[face][1] = vertex_index(slices, slice, stack-1);
                    faces[face][2] = vertex_index(slices, slice-1, stack);
                    face++;

                    faces[face][0] = vertex_index(slices, slice, stack-1);
                    faces[face][1] = vertex_index(slices, slice, stack);
                    faces[face][2] = vertex_index(slices, slice-1, stack);
                    face++;
                }
            }
        }

        theta += theta_step;

        if (stack == 0)
        {
            faces[face][0] = 0;
            faces[face][1] = 1;
            faces[face][2] = slice;
            face++;
        }
        else
        {
            faces[face][0] = vertex_index(slices, slice-1, stack-1);
            faces[face][1] = vertex_index(slices, 0, stack-1);
            faces[face][2] = vertex_index(slices, slice-1, stack);
            face++;

            faces[face][0] = vertex_index(slices, 0, stack-1);
            faces[face][1] = vertex_index(slices, 0, stack);
            faces[face][2] = vertex_index(slices, slice-1, stack);
            face++;
        }
    }

    vertices[vertex].position.x = 0.0f;
    vertices[vertex].position.y = 0.0f;
    vertices[vertex].position.z = -radius;
    vertices[vertex].normal.x = 0.0f;
    vertices[vertex].normal.y = 0.0f;
    vertices[vertex].normal.z = -1.0f;

    /* bottom stack is triangle fan */
    for (slice = 1; slice < slices; slice++)
    {
        faces[face][0] = vertex_index(slices, slice-1, stack-1);
        faces[face][1] = vertex_index(slices, slice, stack-1);
        faces[face][2] = vertex;
        face++;
    }

    faces[face][0] = vertex_index(slices, slice-1, stack-1);
    faces[face][1] = vertex_index(slices, 0, stack-1);
    faces[face][2] = vertex;

    free_sincos_table(&phi);
    sphere->lpVtbl->UnlockIndexBuffer(sphere);
    sphere->lpVtbl->UnlockVertexBuffer(sphere);


    if (adjacency)
    {
        if (FAILED(hr = D3DXCreateBuffer(number_of_faces * sizeof(DWORD) * 3, adjacency)))
        {
            sphere->lpVtbl->Release(sphere);
            return hr;
        }

        if (FAILED(hr = sphere->lpVtbl->GenerateAdjacency(sphere, 0.0f, (*adjacency)->lpVtbl->GetBufferPointer(*adjacency))))
        {
            (*adjacency)->lpVtbl->Release(*adjacency);
            sphere->lpVtbl->Release(sphere);
            return hr;
        }
    }

    *mesh = sphere;

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateCylinder(struct IDirect3DDevice9 *device, float radius1, float radius2,
        float length, UINT slices, UINT stacks, struct ID3DXMesh **mesh, struct ID3DXBuffer **adjacency)
{
    DWORD number_of_vertices, number_of_faces;
    HRESULT hr;
    ID3DXMesh *cylinder;
    struct vertex *vertices;
    face *faces;
    float theta_step, theta_start;
    struct sincos_table theta;
    float delta_radius, radius, radius_step;
    float z, z_step, z_normal;
    DWORD vertex, face, slice, stack;

    TRACE("(%p, %f, %f, %f, %u, %u, %p, %p)\n", device, radius1, radius2, length, slices, stacks, mesh, adjacency);

    if (device == NULL || radius1 < 0.0f || radius2 < 0.0f || length < 0.0f || slices < 2 || stacks < 1 || mesh == NULL)
    {
        return D3DERR_INVALIDCALL;
    }

    number_of_vertices = 2 + (slices * (3 + stacks));
    number_of_faces = 2 * slices + stacks * (2 * slices);

    hr = D3DXCreateMeshFVF(number_of_faces, number_of_vertices, D3DXMESH_MANAGED,
                           D3DFVF_XYZ | D3DFVF_NORMAL, device, &cylinder);
    if (FAILED(hr))
    {
        return hr;
    }

    if (FAILED(hr = cylinder->lpVtbl->LockVertexBuffer(cylinder, 0, (void **)&vertices)))
    {
        cylinder->lpVtbl->Release(cylinder);
        return hr;
    }

    if (FAILED(hr = cylinder->lpVtbl->LockIndexBuffer(cylinder, 0, (void **)&faces)))
    {
        cylinder->lpVtbl->UnlockVertexBuffer(cylinder);
        cylinder->lpVtbl->Release(cylinder);
        return hr;
    }

    /* theta = angle on xy plane wrt x axis */
    theta_step = -2.0f * D3DX_PI / slices;
    theta_start = D3DX_PI / 2.0f;

    if (!compute_sincos_table(&theta, theta_start, theta_step, slices))
    {
        cylinder->lpVtbl->UnlockIndexBuffer(cylinder);
        cylinder->lpVtbl->UnlockVertexBuffer(cylinder);
        cylinder->lpVtbl->Release(cylinder);
        return E_OUTOFMEMORY;
    }

    vertex = 0;
    face = 0;

    delta_radius = radius1 - radius2;
    radius = radius1;
    radius_step = delta_radius / stacks;

    z = -length / 2;
    z_step = length / stacks;
    z_normal = delta_radius / length;
    if (isnan(z_normal))
    {
        z_normal = 0.0f;
    }

    vertices[vertex].normal.x = 0.0f;
    vertices[vertex].normal.y = 0.0f;
    vertices[vertex].normal.z = -1.0f;
    vertices[vertex].position.x = 0.0f;
    vertices[vertex].position.y = 0.0f;
    vertices[vertex++].position.z = z;

    for (slice = 0; slice < slices; slice++, vertex++)
    {
        vertices[vertex].normal.x = 0.0f;
        vertices[vertex].normal.y = 0.0f;
        vertices[vertex].normal.z = -1.0f;
        vertices[vertex].position.x = radius * theta.cos[slice];
        vertices[vertex].position.y = radius * theta.sin[slice];
        vertices[vertex].position.z = z;

        if (slice > 0)
        {
            faces[face][0] = 0;
            faces[face][1] = slice;
            faces[face++][2] = slice + 1;
        }
    }

    faces[face][0] = 0;
    faces[face][1] = slice;
    faces[face++][2] = 1;

    for (stack = 1; stack <= stacks+1; stack++)
    {
        for (slice = 0; slice < slices; slice++, vertex++)
        {
            vertices[vertex].normal.x = theta.cos[slice];
            vertices[vertex].normal.y = theta.sin[slice];
            vertices[vertex].normal.z = z_normal;
            D3DXVec3Normalize(&vertices[vertex].normal, &vertices[vertex].normal);
            vertices[vertex].position.x = radius * theta.cos[slice];
            vertices[vertex].position.y = radius * theta.sin[slice];
            vertices[vertex].position.z = z;

            if (stack > 1 && slice > 0)
            {
                faces[face][0] = vertex_index(slices, slice-1, stack-1);
                faces[face][1] = vertex_index(slices, slice-1, stack);
                faces[face++][2] = vertex_index(slices, slice, stack-1);

                faces[face][0] = vertex_index(slices, slice, stack-1);
                faces[face][1] = vertex_index(slices, slice-1, stack);
                faces[face++][2] = vertex_index(slices, slice, stack);
            }
        }

        if (stack > 1)
        {
            faces[face][0] = vertex_index(slices, slice-1, stack-1);
            faces[face][1] = vertex_index(slices, slice-1, stack);
            faces[face++][2] = vertex_index(slices, 0, stack-1);

            faces[face][0] = vertex_index(slices, 0, stack-1);
            faces[face][1] = vertex_index(slices, slice-1, stack);
            faces[face++][2] = vertex_index(slices, 0, stack);
        }

        if (stack < stacks + 1)
        {
            z += z_step;
            radius -= radius_step;
        }
    }

    for (slice = 0; slice < slices; slice++, vertex++)
    {
        vertices[vertex].normal.x = 0.0f;
        vertices[vertex].normal.y = 0.0f;
        vertices[vertex].normal.z = 1.0f;
        vertices[vertex].position.x = radius * theta.cos[slice];
        vertices[vertex].position.y = radius * theta.sin[slice];
        vertices[vertex].position.z = z;

        if (slice > 0)
        {
            faces[face][0] = vertex_index(slices, slice-1, stack);
            faces[face][1] = number_of_vertices - 1;
            faces[face++][2] = vertex_index(slices, slice, stack);
        }
    }

    vertices[vertex].position.x = 0.0f;
    vertices[vertex].position.y = 0.0f;
    vertices[vertex].position.z = z;
    vertices[vertex].normal.x = 0.0f;
    vertices[vertex].normal.y = 0.0f;
    vertices[vertex].normal.z = 1.0f;

    faces[face][0] = vertex_index(slices, slice-1, stack);
    faces[face][1] = number_of_vertices - 1;
    faces[face][2] = vertex_index(slices, 0, stack);

    free_sincos_table(&theta);
    cylinder->lpVtbl->UnlockIndexBuffer(cylinder);
    cylinder->lpVtbl->UnlockVertexBuffer(cylinder);

    if (adjacency)
    {
        if (FAILED(hr = D3DXCreateBuffer(number_of_faces * sizeof(DWORD) * 3, adjacency)))
        {
            cylinder->lpVtbl->Release(cylinder);
            return hr;
        }

        if (FAILED(hr = cylinder->lpVtbl->GenerateAdjacency(cylinder, 0.0f, (*adjacency)->lpVtbl->GetBufferPointer(*adjacency))))
        {
            (*adjacency)->lpVtbl->Release(*adjacency);
            cylinder->lpVtbl->Release(cylinder);
            return hr;
        }
    }

    *mesh = cylinder;

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateTeapot(struct IDirect3DDevice9 *device,
        struct ID3DXMesh **mesh, struct ID3DXBuffer **adjacency)
{
    FIXME("device %p, mesh %p, adjacency %p semi-stub.\n", device, mesh, adjacency);

    return D3DXCreateSphere(device, 1.0f, 4, 4, mesh, adjacency);
}

HRESULT WINAPI D3DXCreateTextA(struct IDirect3DDevice9 *device, HDC hdc, const char *text, float deviation,
        float extrusion, struct ID3DXMesh **mesh, struct ID3DXBuffer **adjacency, GLYPHMETRICSFLOAT *glyphmetrics)
{
    WCHAR *textW;
    HRESULT hr;
    int len;

    TRACE("device %p, hdc %p, text %s, deviation %.8e, extrusion %.8e, mesh %p, adjacency %p, glyphmetrics %p.\n",
            device, hdc, debugstr_a(text), deviation, extrusion, mesh, adjacency, glyphmetrics);

    if (!text)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    textW = malloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, text, -1, textW, len);

    hr = D3DXCreateTextW(device, hdc, textW, deviation, extrusion,
                         mesh, adjacency, glyphmetrics);
    free(textW);

    return hr;
}

HRESULT WINAPI D3DXCreateTorus(struct IDirect3DDevice9 *device,
        float innerradius, float outerradius, UINT sides, UINT rings, struct ID3DXMesh **mesh, ID3DXBuffer **adjacency)
{
    HRESULT hr;
    ID3DXMesh *torus;
    WORD (*faces)[3];
    struct vertex *vertices;
    float phi, phi_step, sin_phi, cos_phi;
    float theta, theta_step, sin_theta, cos_theta;
    unsigned int i, j, numvert, numfaces;

    TRACE("device %p, innerradius %.8e, outerradius %.8e, sides %u, rings %u, mesh %p, adjacency %p.\n",
            device, innerradius, outerradius, sides, rings, mesh, adjacency);

    numvert = sides * rings;
    numfaces = numvert * 2;

    if (!device || innerradius < 0.0f || outerradius < 0.0f || sides < 3 || rings < 3 || !mesh)
    {
        WARN("Invalid arguments.\n");
        return D3DERR_INVALIDCALL;
    }

    if (FAILED(hr = D3DXCreateMeshFVF(numfaces, numvert, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, &torus)))
        return hr;

    if (FAILED(hr = torus->lpVtbl->LockVertexBuffer(torus, 0, (void **)&vertices)))
    {
        torus->lpVtbl->Release(torus);
        return hr;
    }

    if (FAILED(hr = torus->lpVtbl->LockIndexBuffer(torus, 0, (void **)&faces)))
    {
        torus->lpVtbl->UnlockVertexBuffer(torus);
        torus->lpVtbl->Release(torus);
        return hr;
    }

    phi_step = D3DX_PI / sides * 2.0f;
    theta_step = D3DX_PI / rings * -2.0f;

    theta = 0.0f;

    for (i = 0; i < rings; ++i)
    {
        phi = 0.0f;

        sin_theta = sinf(theta);
        cos_theta = cosf(theta);

        for (j = 0; j < sides; ++j)
        {
            sin_phi = sinf(phi);
            cos_phi = cosf(phi);

            vertices[i * sides + j].position.x = (innerradius * cos_phi + outerradius) * cos_theta;
            vertices[i * sides + j].position.y = (innerradius * cos_phi + outerradius) * sin_theta;
            vertices[i * sides + j].position.z = innerradius * sin_phi;
            vertices[i * sides + j].normal.x = cos_phi * cos_theta;
            vertices[i * sides + j].normal.y = cos_phi * sin_theta;
            vertices[i * sides + j].normal.z = sin_phi;

            phi += phi_step;
        }

        theta += theta_step;
    }

    for (i = 0; i < numfaces - sides * 2; ++i)
    {
        faces[i][0] = i % 2 ? i / 2 + sides : i / 2;
        faces[i][1] = (i / 2 + 1) % sides ? i / 2 + 1 : i / 2 + 1 - sides;
        faces[i][2] = (i + 1) % (sides * 2) ? (i + 1) / 2 + sides : (i + 1) / 2;
    }

    for (j = 0; i < numfaces; ++i, ++j)
    {
        faces[i][0] = i % 2 ? j / 2 : i / 2;
        faces[i][1] = (i / 2 + 1) % sides ? i / 2 + 1 : i / 2 + 1 - sides;
        faces[i][2] = i == numfaces - 1 ? 0 : (j + 1) / 2;
    }

    torus->lpVtbl->UnlockIndexBuffer(torus);
    torus->lpVtbl->UnlockVertexBuffer(torus);

    if (adjacency)
    {
        if (FAILED(hr = D3DXCreateBuffer(numfaces * sizeof(DWORD) * 3, adjacency)))
        {
            torus->lpVtbl->Release(torus);
            return hr;
        }

        if (FAILED(hr = torus->lpVtbl->GenerateAdjacency(torus, 0.0f, (*adjacency)->lpVtbl->GetBufferPointer(*adjacency))))
        {
            (*adjacency)->lpVtbl->Release(*adjacency);
            torus->lpVtbl->Release(torus);
            return hr;
        }
    }

    *mesh = torus;

    return D3D_OK;
}

enum pointtype {
    POINTTYPE_CURVE = 0,
    POINTTYPE_CORNER,
    POINTTYPE_CURVE_START,
    POINTTYPE_CURVE_END,
    POINTTYPE_CURVE_MIDDLE,
};

struct point2d
{
    D3DXVECTOR2 pos;
    enum pointtype corner;
};

struct dynamic_array
{
    int count, capacity;
    void *items;
};

/* is a dynamic_array */
struct outline
{
    int count, capacity;
    struct point2d *items;
};

/* is a dynamic_array */
struct outline_array
{
    int count, capacity;
    struct outline *items;
};

struct face_array
{
    int count;
    face *items;
};

struct point2d_index
{
    struct outline *outline;
    int vertex;
};

struct point2d_index_array
{
    int count;
    struct point2d_index *items;
};

struct glyphinfo
{
    struct outline_array outlines;
    struct face_array faces;
    struct point2d_index_array ordered_vertices;
    float offset_x;
};

/* is an dynamic_array */
struct word_array
{
    int count, capacity;
    WORD *items;
};

/* complex polygons are split into monotone polygons, which have
 * at most 2 intersections with the vertical sweep line */
struct triangulation
{
    struct word_array vertex_stack;
    BOOL last_on_top, merging;
};

/* is an dynamic_array */
struct triangulation_array
{
    int count, capacity;
    struct triangulation *items;

    struct glyphinfo *glyph;
};

static BOOL reserve(struct dynamic_array *array, int count, int itemsize)
{
    if (count > array->capacity) {
        void *new_buffer;
        int new_capacity = max(array->capacity ? array->capacity * 2 : 16, count);
        new_buffer = realloc(array->items, new_capacity * itemsize);
        if (!new_buffer)
            return FALSE;
        array->items = new_buffer;
        array->capacity = new_capacity;
    }
    return TRUE;
}

static struct point2d *add_points(struct outline *array, int num)
{
    struct point2d *item;

    if (!reserve((struct dynamic_array *)array, array->count + num, sizeof(array->items[0])))
        return NULL;

    item = &array->items[array->count];
    array->count += num;
    return item;
}

static struct outline *add_outline(struct outline_array *array)
{
    struct outline *item;

    if (!reserve((struct dynamic_array *)array, array->count + 1, sizeof(array->items[0])))
        return NULL;

    item = &array->items[array->count++];
    ZeroMemory(item, sizeof(*item));
    return item;
}

static inline face *add_face(struct face_array *array)
{
    return &array->items[array->count++];
}

static struct triangulation *add_triangulation(struct triangulation_array *array)
{
    struct triangulation *item;

    if (!reserve((struct dynamic_array *)array, array->count + 1, sizeof(array->items[0])))
        return NULL;

    item = &array->items[array->count++];
    ZeroMemory(item, sizeof(*item));
    return item;
}

static HRESULT add_vertex_index(struct word_array *array, WORD vertex_index)
{
    if (!reserve((struct dynamic_array *)array, array->count + 1, sizeof(array->items[0])))
        return E_OUTOFMEMORY;

    array->items[array->count++] = vertex_index;
    return S_OK;
}

/* assume fixed point numbers can be converted to float point in place */
C_ASSERT(sizeof(FIXED) == sizeof(float));
C_ASSERT(sizeof(POINTFX) == sizeof(D3DXVECTOR2));

static inline D3DXVECTOR2 *convert_fixed_to_float(POINTFX *pt, int count, unsigned int emsquare)
{
    D3DXVECTOR2 *ret = (D3DXVECTOR2*)pt;
    while (count--) {
        D3DXVECTOR2 *pt_flt = (D3DXVECTOR2*)pt;
        pt_flt->x = (pt->x.value + pt->x.fract / (float)0x10000) / emsquare;
        pt_flt->y = (pt->y.value + pt->y.fract / (float)0x10000) / emsquare;
        pt++;
    }
    return ret;
}

static HRESULT add_bezier_points(struct outline *outline, const D3DXVECTOR2 *p1,
                                 const D3DXVECTOR2 *p2, const D3DXVECTOR2 *p3,
                                 float max_deviation_sq)
{
    D3DXVECTOR2 split1 = {0, 0}, split2 = {0, 0}, middle, vec;
    float deviation_sq;

    D3DXVec2Scale(&split1, D3DXVec2Add(&split1, p1, p2), 0.5f);
    D3DXVec2Scale(&split2, D3DXVec2Add(&split2, p2, p3), 0.5f);
    D3DXVec2Scale(&middle, D3DXVec2Add(&middle, &split1, &split2), 0.5f);

    deviation_sq = D3DXVec2LengthSq(D3DXVec2Subtract(&vec, &middle, p2));
    if (deviation_sq < max_deviation_sq) {
        struct point2d *pt = add_points(outline, 1);
        if (!pt) return E_OUTOFMEMORY;
        pt->pos = *p2;
        pt->corner = POINTTYPE_CURVE;
        /* the end point is omitted because the end line merges into the next segment of
         * the split bezier curve, and the end of the split bezier curve is added outside
         * this recursive function. */
    } else {
        HRESULT hr = add_bezier_points(outline, p1, &split1, &middle, max_deviation_sq);
        if (hr != S_OK) return hr;
        hr = add_bezier_points(outline, &middle, &split2, p3, max_deviation_sq);
        if (hr != S_OK) return hr;
    }

    return S_OK;
}

static inline BOOL is_direction_similar(D3DXVECTOR2 *dir1, D3DXVECTOR2 *dir2, float cos_theta)
{
    /* dot product = cos(theta) */
    return D3DXVec2Dot(dir1, dir2) > cos_theta;
}

static inline D3DXVECTOR2 *unit_vec2(D3DXVECTOR2 *dir, const D3DXVECTOR2 *pt1, const D3DXVECTOR2 *pt2)
{
    return D3DXVec2Normalize(dir, D3DXVec2Subtract(dir, pt2, pt1));
}

struct cos_table
{
    float cos_half;
    float cos_45;
    float cos_90;
};

static BOOL attempt_line_merge(struct outline *outline,
                               int pt_index,
                               const D3DXVECTOR2 *nextpt,
                               BOOL to_curve,
                               const struct cos_table *table)
{
    D3DXVECTOR2 curdir, lastdir;
    struct point2d *prevpt, *pt;
    BOOL ret = FALSE;

    pt = &outline->items[pt_index];
    pt_index = (pt_index - 1 + outline->count) % outline->count;
    prevpt = &outline->items[pt_index];

    if (to_curve)
        pt->corner = pt->corner != POINTTYPE_CORNER ? POINTTYPE_CURVE_MIDDLE : POINTTYPE_CURVE_START;

    if (outline->count < 2)
        return FALSE;

    /* remove last point if the next line continues the last line */
    unit_vec2(&lastdir, &prevpt->pos, &pt->pos);
    unit_vec2(&curdir, &pt->pos, nextpt);
    if (is_direction_similar(&lastdir, &curdir, table->cos_half))
    {
        outline->count--;
        if (pt->corner == POINTTYPE_CURVE_END)
            prevpt->corner = pt->corner;
        if (prevpt->corner == POINTTYPE_CURVE_END && to_curve)
            prevpt->corner = POINTTYPE_CURVE_MIDDLE;
        pt = prevpt;

        ret = TRUE;
        if (outline->count < 2)
            return ret;

        pt_index = (pt_index - 1 + outline->count) % outline->count;
        prevpt = &outline->items[pt_index];
        unit_vec2(&lastdir, &prevpt->pos, &pt->pos);
        unit_vec2(&curdir, &pt->pos, nextpt);
    }
    return ret;
}

static HRESULT create_outline(struct glyphinfo *glyph, void *raw_outline, int datasize,
                              float max_deviation_sq, unsigned int emsquare,
                              const struct cos_table *cos_table)
{
    TTPOLYGONHEADER *header = (TTPOLYGONHEADER *)raw_outline;

    while ((char *)header < (char *)raw_outline + datasize)
    {
        TTPOLYCURVE *curve = (TTPOLYCURVE *)(header + 1);
        struct point2d *lastpt, *pt;
        D3DXVECTOR2 lastdir;
        D3DXVECTOR2 *pt_flt;
        int j;
        struct outline *outline = add_outline(&glyph->outlines);

        if (!outline)
            return E_OUTOFMEMORY;

        pt = add_points(outline, 1);
        if (!pt)
            return E_OUTOFMEMORY;
        pt_flt = convert_fixed_to_float(&header->pfxStart, 1, emsquare);
        pt->pos = *pt_flt;
        pt->corner = POINTTYPE_CORNER;

        if (header->dwType != TT_POLYGON_TYPE)
            FIXME("Unknown header type %lu.\n", header->dwType);

        while ((char *)curve < (char *)header + header->cb)
        {
            D3DXVECTOR2 bezier_start = outline->items[outline->count - 1].pos;
            BOOL to_curve = curve->wType != TT_PRIM_LINE && curve->cpfx > 1;
            unsigned int j2 = 0;

            if (!curve->cpfx) {
                curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
                continue;
            }

            pt_flt = convert_fixed_to_float(curve->apfx, curve->cpfx, emsquare);

            attempt_line_merge(outline, outline->count - 1, &pt_flt[0], to_curve, cos_table);

            if (to_curve)
            {
                HRESULT hr;
                int count = curve->cpfx;

                while (count > 2)
                {
                    D3DXVECTOR2 bezier_end;

                    D3DXVec2Scale(&bezier_end, D3DXVec2Add(&bezier_end, &pt_flt[j2], &pt_flt[j2+1]), 0.5f);
                    hr = add_bezier_points(outline, &bezier_start, &pt_flt[j2], &bezier_end, max_deviation_sq);
                    if (hr != S_OK)
                        return hr;
                    bezier_start = bezier_end;
                    count--;
                    j2++;
                }
                hr = add_bezier_points(outline, &bezier_start, &pt_flt[j2], &pt_flt[j2+1], max_deviation_sq);
                if (hr != S_OK)
                    return hr;

                pt = add_points(outline, 1);
                if (!pt)
                    return E_OUTOFMEMORY;
                j2++;
                pt->pos = pt_flt[j2];
                pt->corner = POINTTYPE_CURVE_END;
            } else {
                pt = add_points(outline, curve->cpfx);
                if (!pt)
                    return E_OUTOFMEMORY;
                for (j2 = 0; j2 < curve->cpfx; j2++)
                {
                    pt->pos = pt_flt[j2];
                    pt->corner = POINTTYPE_CORNER;
                    pt++;
                }
            }

            curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
        }

        /* remove last point if the next line continues the last line */
        if (outline->count >= 3) {
            BOOL to_curve;

            lastpt = &outline->items[outline->count - 1];
            pt = &outline->items[0];
            if (pt->pos.x == lastpt->pos.x && pt->pos.y == lastpt->pos.y) {
                if (lastpt->corner == POINTTYPE_CURVE_END)
                {
                    if (pt->corner == POINTTYPE_CURVE_START)
                        pt->corner = POINTTYPE_CURVE_MIDDLE;
                    else
                        pt->corner = POINTTYPE_CURVE_END;
                }
                outline->count--;
            } else {
                /* outline closed with a line from end to start point */
                attempt_line_merge(outline, outline->count - 1, &pt->pos, FALSE, cos_table);
            }
            lastpt = &outline->items[0];
            to_curve = lastpt->corner != POINTTYPE_CORNER && lastpt->corner != POINTTYPE_CURVE_END;
            if (lastpt->corner == POINTTYPE_CURVE_START)
                lastpt->corner = POINTTYPE_CORNER;
            pt = &outline->items[1];
            if (attempt_line_merge(outline, 0, &pt->pos, to_curve, cos_table))
                *lastpt = outline->items[outline->count];
        }

        lastpt = &outline->items[outline->count - 1];
        pt = &outline->items[0];
        unit_vec2(&lastdir, &lastpt->pos, &pt->pos);
        for (j = 0; j < outline->count; j++)
        {
            D3DXVECTOR2 curdir;

            lastpt = pt;
            pt = &outline->items[(j + 1) % outline->count];
            unit_vec2(&curdir, &lastpt->pos, &pt->pos);

            switch (lastpt->corner)
            {
                case POINTTYPE_CURVE_START:
                case POINTTYPE_CURVE_END:
                    if (!is_direction_similar(&lastdir, &curdir, cos_table->cos_45))
                        lastpt->corner = POINTTYPE_CORNER;
                    break;
                case POINTTYPE_CURVE_MIDDLE:
                    if (!is_direction_similar(&lastdir, &curdir, cos_table->cos_90))
                        lastpt->corner = POINTTYPE_CORNER;
                    else
                        lastpt->corner = POINTTYPE_CURVE;
                    break;
                default:
                    break;
            }
            lastdir = curdir;
        }

        header = (TTPOLYGONHEADER *)((char *)header + header->cb);
    }
    return S_OK;
}

/* Get the y-distance from a line to a point */
static float get_line_to_point_y_distance(D3DXVECTOR2 *line_pt1,
                                          D3DXVECTOR2 *line_pt2,
                                          D3DXVECTOR2 *point)
{
    D3DXVECTOR2 line_vec = {0, 0};
    float line_pt_dx;
    float line_y;

    D3DXVec2Subtract(&line_vec, line_pt2, line_pt1);
    line_pt_dx = point->x - line_pt1->x;
    line_y = line_pt1->y + (line_vec.y * line_pt_dx) / line_vec.x;
    return point->y - line_y;
}

static D3DXVECTOR2 *get_indexed_point(struct point2d_index *pt_idx)
{
    return &pt_idx->outline->items[pt_idx->vertex].pos;
}

static D3DXVECTOR2 *get_ordered_vertex(struct glyphinfo *glyph, WORD index)
{
    return get_indexed_point(&glyph->ordered_vertices.items[index]);
}

static void remove_triangulation(struct triangulation_array *array, struct triangulation *item)
{
    free(item->vertex_stack.items);
    MoveMemory(item, item + 1, (char*)&array->items[array->count] - (char*)(item + 1));
    array->count--;
}

static HRESULT triangulation_add_point(struct triangulation **t_ptr,
                                       struct triangulation_array *triangulations,
                                       WORD vtx_idx,
                                       BOOL to_top)
{
    struct glyphinfo *glyph = triangulations->glyph;
    struct triangulation *t = *t_ptr;
    HRESULT hr;
    face *face;
    int f1, f2;

    if (t->last_on_top) {
        f1 = 1;
        f2 = 2;
    } else {
        f1 = 2;
        f2 = 1;
    }

    if (t->last_on_top != to_top && t->vertex_stack.count > 1) {
        /* consume all vertices on the stack */
        WORD last_pt = t->vertex_stack.items[0];
        int i;
        for (i = 1; i < t->vertex_stack.count; i++)
        {
            face = add_face(&glyph->faces);
            if (!face) return E_OUTOFMEMORY;
            (*face)[0] = vtx_idx;
            (*face)[f1] = last_pt;
            (*face)[f2] = last_pt = t->vertex_stack.items[i];
        }
        t->vertex_stack.items[0] = last_pt;
        t->vertex_stack.count = 1;
    } else if (t->vertex_stack.count > 1) {
        int i = t->vertex_stack.count - 1;
        D3DXVECTOR2 *point = get_ordered_vertex(glyph, vtx_idx);
        WORD top_idx = t->vertex_stack.items[i--];
        D3DXVECTOR2 *top_pt = get_ordered_vertex(glyph, top_idx);

        while (i >= 0)
        {
            WORD prev_idx = t->vertex_stack.items[i--];
            D3DXVECTOR2 *prev_pt = get_ordered_vertex(glyph, prev_idx);

            if (prev_pt->x != top_pt->x &&
                ((to_top && get_line_to_point_y_distance(prev_pt, top_pt, point) > 0) ||
                 (!to_top && get_line_to_point_y_distance(prev_pt, top_pt, point) < 0)))
                break;

            face = add_face(&glyph->faces);
            if (!face) return E_OUTOFMEMORY;
            (*face)[0] = vtx_idx;
            (*face)[f1] = prev_idx;
            (*face)[f2] = top_idx;

            top_pt = prev_pt;
            top_idx = prev_idx;
            t->vertex_stack.count--;
        }
    }
    t->last_on_top = to_top;

    hr = add_vertex_index(&t->vertex_stack, vtx_idx);

    if (hr == S_OK && t->merging) {
        struct triangulation *t2;

        t2 = to_top ? t - 1 : t + 1;
        t2->merging = FALSE;
        hr = triangulation_add_point(&t2, triangulations, vtx_idx, to_top);
        if (hr != S_OK) return hr;
        remove_triangulation(triangulations, t);
        if (t2 > t)
            t2--;
        *t_ptr = t2;
    }
    return hr;
}

/* check if the point is next on the outline for either the top or bottom */
static D3DXVECTOR2 *triangulation_get_next_point(struct triangulation *t, struct glyphinfo *glyph, BOOL on_top)
{
    int i = t->last_on_top == on_top ? t->vertex_stack.count - 1 : 0;
    WORD idx = t->vertex_stack.items[i];
    struct point2d_index *pt_idx = &glyph->ordered_vertices.items[idx];
    struct outline *outline = pt_idx->outline;

    if (on_top)
        i = (pt_idx->vertex + outline->count - 1) % outline->count;
    else
        i = (pt_idx->vertex + 1) % outline->count;

    return &outline->items[i].pos;
}

static int __cdecl compare_vertex_indices(const void *a, const void *b)
{
    const struct point2d_index *idx1 = a, *idx2 = b;
    const D3DXVECTOR2 *p1 = &idx1->outline->items[idx1->vertex].pos;
    const D3DXVECTOR2 *p2 = &idx2->outline->items[idx2->vertex].pos;
    float diff = p1->x - p2->x;

    if (diff == 0.0f)
        diff = p1->y - p2->y;

    return diff == 0.0f ? 0 : (diff > 0.0f ? -1 : 1);
}

static HRESULT triangulate(struct triangulation_array *triangulations)
{
    int sweep_idx;
    HRESULT hr;
    struct glyphinfo *glyph = triangulations->glyph;
    int nb_vertices = 0;
    int i;
    struct point2d_index *idx_ptr;

    /* Glyphs without outlines do not generate any vertices. */
    if (!glyph->outlines.count)
        return D3D_OK;

    for (i = 0; i < glyph->outlines.count; i++)
        nb_vertices += glyph->outlines.items[i].count;

    glyph->ordered_vertices.items = malloc(nb_vertices * sizeof(*glyph->ordered_vertices.items));
    if (!glyph->ordered_vertices.items)
        return E_OUTOFMEMORY;

    idx_ptr = glyph->ordered_vertices.items;
    for (i = 0; i < glyph->outlines.count; i++)
    {
        struct outline *outline = &glyph->outlines.items[i];
        int j;

        idx_ptr->outline = outline;
        idx_ptr->vertex = 0;
        idx_ptr++;
        for (j = outline->count - 1; j > 0; j--)
        {
            idx_ptr->outline = outline;
            idx_ptr->vertex = j;
            idx_ptr++;
        }
    }
    glyph->ordered_vertices.count = nb_vertices;

    /* Native implementation seems to try to create a triangle fan from
     * the first outline point if the glyph only has one outline. */
    if (glyph->outlines.count == 1)
    {
        struct outline *outline = glyph->outlines.items;
        D3DXVECTOR2 *base = &outline->items[0].pos;
        D3DXVECTOR2 *last = &outline->items[1].pos;
        float ccw = 0;

        for (i = 2; i < outline->count; i++)
        {
            D3DXVECTOR2 *next = &outline->items[i].pos;
            D3DXVECTOR2 v1 = {0.0f, 0.0f};
            D3DXVECTOR2 v2 = {0.0f, 0.0f};

            D3DXVec2Subtract(&v1, base, last);
            D3DXVec2Subtract(&v2, last, next);
            ccw = D3DXVec2CCW(&v1, &v2);
            if (ccw > 0.0f)
                break;

            last = next;
        }
        if (ccw <= 0)
        {
            glyph->faces.items = malloc((outline->count - 2) * sizeof(glyph->faces.items[0]));
            if (!glyph->faces.items)
                return E_OUTOFMEMORY;

            glyph->faces.count = outline->count - 2;
            for (i = 0; i < glyph->faces.count; i++)
            {
                glyph->faces.items[i][0] = 0;
                glyph->faces.items[i][1] = i + 1;
                glyph->faces.items[i][2] = i + 2;
            }
            return S_OK;
        }
    }

    /* Perform 2D polygon triangulation for complex glyphs.
     * Triangulation is performed using a sweep line concept, from right to left,
     * by processing vertices in sorted order. Complex polygons are split into
     * monotone polygons which are triangulated separately. */
    /* FIXME: The order of the faces is not consistent with the native implementation. */

    /* Reserve space for maximum possible faces from triangulation.
     * # faces for outer outlines = outline->count - 2
     * # faces for inner outlines = outline->count + 2
     * There must be at least 1 outer outline. */
    glyph->faces.items = malloc((nb_vertices + glyph->outlines.count * 2 - 4) * sizeof(glyph->faces.items[0]));
    if (!glyph->faces.items)
        return E_OUTOFMEMORY;

    qsort(glyph->ordered_vertices.items, nb_vertices,
          sizeof(glyph->ordered_vertices.items[0]), compare_vertex_indices);
    for (sweep_idx = 0; sweep_idx < glyph->ordered_vertices.count; sweep_idx++)
    {
        int start = 0;
        int end = triangulations->count;

        while (start < end)
        {
            D3DXVECTOR2 *sweep_vtx = get_ordered_vertex(glyph, sweep_idx);
            int current = (start + end) / 2;
            struct triangulation *t = &triangulations->items[current];
            BOOL on_top_outline = FALSE;
            D3DXVECTOR2 *top_next, *bottom_next;
            WORD top_idx, bottom_idx;

            if (t->merging && t->last_on_top)
                top_next = triangulation_get_next_point(t + 1, glyph, TRUE);
            else
                top_next = triangulation_get_next_point(t, glyph, TRUE);
            if (sweep_vtx == top_next)
            {
                if (t->merging && t->last_on_top)
                    t++;
                hr = triangulation_add_point(&t, triangulations, sweep_idx, TRUE);
                if (hr != S_OK) return hr;

                if (t + 1 < &triangulations->items[triangulations->count] &&
                    triangulation_get_next_point(t + 1, glyph, FALSE) == sweep_vtx)
                {
                    /* point also on bottom outline of higher triangulation */
                    struct triangulation *t2 = t + 1;
                    hr = triangulation_add_point(&t2, triangulations, sweep_idx, FALSE);
                    if (hr != S_OK) return hr;

                    t->merging = TRUE;
                    t2->merging = TRUE;
                }
                on_top_outline = TRUE;
            }

            if (t->merging && !t->last_on_top)
                bottom_next = triangulation_get_next_point(t - 1, glyph, FALSE);
            else
                bottom_next = triangulation_get_next_point(t, glyph, FALSE);
            if (sweep_vtx == bottom_next)
            {
                if (t->merging && !t->last_on_top)
                    t--;
                if (on_top_outline) {
                    /* outline finished */
                    remove_triangulation(triangulations, t);
                    break;
                }

                hr = triangulation_add_point(&t, triangulations, sweep_idx, FALSE);
                if (hr != S_OK) return hr;

                if (t > triangulations->items &&
                    triangulation_get_next_point(t - 1, glyph, TRUE) == sweep_vtx)
                {
                    struct triangulation *t2 = t - 1;
                    /* point also on top outline of lower triangulation */
                    hr = triangulation_add_point(&t2, triangulations, sweep_idx, TRUE);
                    if (hr != S_OK) return hr;
                    t = t2 + 1; /* t may be invalidated by triangulation merging */

                    t->merging = TRUE;
                    t2->merging = TRUE;
                }
                break;
            }
            if (on_top_outline)
                break;

            if (t->last_on_top) {
                top_idx = t->vertex_stack.items[t->vertex_stack.count - 1];
                bottom_idx = t->vertex_stack.items[0];
            } else {
                top_idx = t->vertex_stack.items[0];
                bottom_idx = t->vertex_stack.items[t->vertex_stack.count - 1];
            }

            /* check if the point is inside or outside this polygon */
            if (get_line_to_point_y_distance(get_ordered_vertex(glyph, top_idx),
                                             top_next, sweep_vtx) > 0)
            { /* above */
                start = current + 1;
            } else if (get_line_to_point_y_distance(get_ordered_vertex(glyph, bottom_idx),
                                                    bottom_next, sweep_vtx) < 0)
            { /* below */
                end = current;
            } else if (t->merging) {
                /* inside, so cancel merging */
                struct triangulation *t2 = t->last_on_top ? t + 1 : t - 1;
                t->merging = FALSE;
                t2->merging = FALSE;
                hr = triangulation_add_point(&t, triangulations, sweep_idx, t->last_on_top);
                if (hr != S_OK) return hr;
                hr = triangulation_add_point(&t2, triangulations, sweep_idx, t2->last_on_top);
                if (hr != S_OK) return hr;
                break;
            } else {
                /* inside, so split polygon into two monotone parts */
                struct triangulation *t2 = add_triangulation(triangulations);
                if (!t2) return E_OUTOFMEMORY;
                MoveMemory(t + 1, t, (char*)(t2 + 1) - (char*)t);
                if (t->last_on_top) {
                    t2 = t + 1;
                } else {
                    t2 = t;
                    t++;
                }

                ZeroMemory(&t2->vertex_stack, sizeof(t2->vertex_stack));
                hr = add_vertex_index(&t2->vertex_stack, t->vertex_stack.items[t->vertex_stack.count - 1]);
                if (hr != S_OK) return hr;
                hr = add_vertex_index(&t2->vertex_stack, sweep_idx);
                if (hr != S_OK) return hr;
                t2->last_on_top = !t->last_on_top;

                hr = triangulation_add_point(&t, triangulations, sweep_idx, t->last_on_top);
                if (hr != S_OK) return hr;
                break;
            }
        }
        if (start >= end)
        {
            struct triangulation *t;
            struct triangulation *t2 = add_triangulation(triangulations);
            if (!t2) return E_OUTOFMEMORY;
            t = &triangulations->items[start];
            MoveMemory(t + 1, t, (char*)(t2 + 1) - (char*)t);
            ZeroMemory(t, sizeof(*t));
            hr = add_vertex_index(&t->vertex_stack, sweep_idx);
            if (hr != S_OK) return hr;
        }
    }
    return S_OK;
}

HRESULT WINAPI D3DXCreateTextW(struct IDirect3DDevice9 *device, HDC hdc, const WCHAR *text, float deviation,
        float extrusion, struct ID3DXMesh **mesh_ptr, struct ID3DXBuffer **adjacency, GLYPHMETRICSFLOAT *glyphmetrics)
{
    HRESULT hr;
    ID3DXMesh *mesh = NULL;
    DWORD nb_vertices, nb_faces;
    DWORD nb_front_faces, nb_corners, nb_outline_points;
    struct vertex *vertices = NULL;
    face *faces = NULL;
    int textlen = 0;
    float offset_x;
    LOGFONTW lf;
    OUTLINETEXTMETRICW otm;
    HFONT font = NULL, oldfont = NULL;
    const MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
    void *raw_outline = NULL;
    int bufsize = 0;
    struct glyphinfo *glyphs = NULL;
    GLYPHMETRICS gm;
    struct triangulation_array triangulations = {0, 0, NULL};
    int i;
    struct vertex *vertex_ptr;
    face *face_ptr;
    float max_deviation_sq;
    const struct cos_table cos_table = {
        cosf(D3DXToRadian(0.5f)),
        cosf(D3DXToRadian(45.0f)),
        cosf(D3DXToRadian(90.0f)),
    };
    int f1, f2;

    TRACE("(%p, %p, %s, %f, %f, %p, %p, %p)\n", device, hdc,
          debugstr_w(text), deviation, extrusion, mesh_ptr, adjacency, glyphmetrics);

    if (!device || !hdc || !text || !*text || deviation < 0.0f || extrusion < 0.0f || !mesh_ptr)
        return D3DERR_INVALIDCALL;

    if (adjacency)
    {
        FIXME("Case of adjacency != NULL not implemented.\n");
        return E_NOTIMPL;
    }

    if (!GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf) ||
        !GetOutlineTextMetricsW(hdc, sizeof(otm), &otm))
    {
        return D3DERR_INVALIDCALL;
    }

    if (deviation == 0.0f)
        deviation = 1.0f / otm.otmEMSquare;
    max_deviation_sq = deviation * deviation;

    lf.lfHeight = otm.otmEMSquare;
    lf.lfWidth = 0;
    font = CreateFontIndirectW(&lf);
    if (!font) {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    oldfont = SelectObject(hdc, font);

    textlen = lstrlenW(text);
    for (i = 0; i < textlen; i++)
    {
        int datasize = GetGlyphOutlineW(hdc, text[i], GGO_NATIVE, &gm, 0, NULL, &identity);
        if (datasize < 0)
            return D3DERR_INVALIDCALL;
        if (bufsize < datasize)
            bufsize = datasize;
    }
    if (!bufsize) { /* e.g. text == " " */
        hr = D3DERR_INVALIDCALL;
        goto error;
    }

    glyphs = calloc(textlen, sizeof(*glyphs));
    raw_outline = malloc(bufsize);
    if (!glyphs || !raw_outline) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    offset_x = 0.0f;
    for (i = 0; i < textlen; i++)
    {
        /* get outline points from data returned from GetGlyphOutline */
        int datasize;

        glyphs[i].offset_x = offset_x;

        datasize = GetGlyphOutlineW(hdc, text[i], GGO_NATIVE, &gm, bufsize, raw_outline, &identity);
        hr = create_outline(&glyphs[i], raw_outline, datasize,
                            max_deviation_sq, otm.otmEMSquare, &cos_table);
        if (hr != S_OK) goto error;

        triangulations.glyph = &glyphs[i];
        hr = triangulate(&triangulations);
        if (hr != S_OK) goto error;
        if (triangulations.count) {
            ERR("%d incomplete triangulations of glyph (%u).\n", triangulations.count, text[i]);
            triangulations.count = 0;
        }

        if (glyphmetrics)
        {
            glyphmetrics[i].gmfBlackBoxX = gm.gmBlackBoxX / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfBlackBoxY = gm.gmBlackBoxY / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfptGlyphOrigin.x = gm.gmptGlyphOrigin.x / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfptGlyphOrigin.y = gm.gmptGlyphOrigin.y / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfCellIncX = gm.gmCellIncX / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfCellIncY = gm.gmCellIncY / (float)otm.otmEMSquare;
        }
        offset_x += gm.gmCellIncX / (float)otm.otmEMSquare;
    }

    /* corner points need an extra vertex for the different side faces normals */
    nb_corners = 0;
    nb_outline_points = 0;
    nb_front_faces = 0;
    for (i = 0; i < textlen; i++)
    {
        int j;
        nb_outline_points += glyphs[i].ordered_vertices.count;
        nb_front_faces += glyphs[i].faces.count;
        for (j = 0; j < glyphs[i].outlines.count; j++)
        {
            int k;
            struct outline *outline = &glyphs[i].outlines.items[j];
            nb_corners++; /* first outline point always repeated as a corner */
            for (k = 1; k < outline->count; k++)
                if (outline->items[k].corner)
                    nb_corners++;
        }
    }

    nb_vertices = (nb_outline_points + nb_corners) * 2 + nb_outline_points * 2;
    nb_faces = nb_outline_points * 2 + nb_front_faces * 2;


    hr = D3DXCreateMeshFVF(nb_faces, nb_vertices, D3DXMESH_MANAGED,
                           D3DFVF_XYZ | D3DFVF_NORMAL, device, &mesh);
    if (FAILED(hr))
        goto error;

    if (FAILED(hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, (void **)&vertices)))
        goto error;

    if (FAILED(hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, (void **)&faces)))
        goto error;

    /* convert 2D vertices and faces into 3D mesh */
    vertex_ptr = vertices;
    face_ptr = faces;
    if (extrusion == 0.0f) {
        f1 = 1;
        f2 = 2;
    } else {
        f1 = 2;
        f2 = 1;
    }
    for (i = 0; i < textlen; i++)
    {
        int j;
        int count;
        struct vertex *back_vertices;
        face *back_faces;

        /* side vertices and faces */
        for (j = 0; j < glyphs[i].outlines.count; j++)
        {
            struct vertex *outline_vertices = vertex_ptr;
            struct outline *outline = &glyphs[i].outlines.items[j];
            int k;
            struct point2d *prevpt = &outline->items[outline->count - 1];
            struct point2d *pt = &outline->items[0];

            for (k = 1; k <= outline->count; k++)
            {
                struct vertex vtx;
                struct point2d *nextpt = &outline->items[k % outline->count];
                WORD vtx_idx = vertex_ptr - vertices;
                D3DXVECTOR2 vec;

                if (pt->corner == POINTTYPE_CURVE_START)
                    D3DXVec2Subtract(&vec, &pt->pos, &prevpt->pos);
                else if (pt->corner)
                    D3DXVec2Subtract(&vec, &nextpt->pos, &pt->pos);
                else
                    D3DXVec2Subtract(&vec, &nextpt->pos, &prevpt->pos);
                D3DXVec2Normalize(&vec, &vec);
                vtx.normal.x = -vec.y;
                vtx.normal.y = vec.x;
                vtx.normal.z = 0;

                vtx.position.x = pt->pos.x + glyphs[i].offset_x;
                vtx.position.y = pt->pos.y;
                vtx.position.z = 0;
                *vertex_ptr++ = vtx;

                vtx.position.z = -extrusion;
                *vertex_ptr++ = vtx;

                vtx.position.x = nextpt->pos.x + glyphs[i].offset_x;
                vtx.position.y = nextpt->pos.y;
                if (pt->corner && nextpt->corner && nextpt->corner != POINTTYPE_CURVE_END) {
                    vtx.position.z = -extrusion;
                    *vertex_ptr++ = vtx;
                    vtx.position.z = 0;
                    *vertex_ptr++ = vtx;

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 2;
                    (*face_ptr)[2] = vtx_idx + 1;
                    face_ptr++;

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 3;
                    (*face_ptr)[2] = vtx_idx + 2;
                    face_ptr++;
                } else {
                    if (nextpt->corner) {
                        if (nextpt->corner == POINTTYPE_CURVE_END) {
                            D3DXVECTOR2 *nextpt2 = &outline->items[(k + 1) % outline->count].pos;
                            D3DXVec2Subtract(&vec, nextpt2, &nextpt->pos);
                        } else {
                            D3DXVec2Subtract(&vec, &nextpt->pos, &pt->pos);
                        }
                        D3DXVec2Normalize(&vec, &vec);
                        vtx.normal.x = -vec.y;
                        vtx.normal.y = vec.x;

                        vtx.position.z = 0;
                        *vertex_ptr++ = vtx;
                        vtx.position.z = -extrusion;
                        *vertex_ptr++ = vtx;
                    }

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 3;
                    (*face_ptr)[2] = vtx_idx + 1;
                    face_ptr++;

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 2;
                    (*face_ptr)[2] = vtx_idx + 3;
                    face_ptr++;
                }

                prevpt = pt;
                pt = nextpt;
            }
            if (!pt->corner) {
                *vertex_ptr++ = *outline_vertices++;
                *vertex_ptr++ = *outline_vertices++;
            }
        }

        /* back vertices and faces */
        back_faces = face_ptr;
        back_vertices = vertex_ptr;
        for (j = 0; j < glyphs[i].ordered_vertices.count; j++)
        {
            D3DXVECTOR2 *pt = get_ordered_vertex(&glyphs[i], j);
            vertex_ptr->position.x = pt->x + glyphs[i].offset_x;
            vertex_ptr->position.y = pt->y;
            vertex_ptr->position.z = 0;
            vertex_ptr->normal.x = 0;
            vertex_ptr->normal.y = 0;
            vertex_ptr->normal.z = 1;
            vertex_ptr++;
        }
        count = back_vertices - vertices;
        for (j = 0; j < glyphs[i].faces.count; j++)
        {
            face *f = &glyphs[i].faces.items[j];
            (*face_ptr)[0] = (*f)[0] + count;
            (*face_ptr)[1] = (*f)[1] + count;
            (*face_ptr)[2] = (*f)[2] + count;
            face_ptr++;
        }

        /* front vertices and faces */
        j = count = vertex_ptr - back_vertices;
        while (j--)
        {
            vertex_ptr->position.x = back_vertices->position.x;
            vertex_ptr->position.y = back_vertices->position.y;
            vertex_ptr->position.z = -extrusion;
            vertex_ptr->normal.x = 0;
            vertex_ptr->normal.y = 0;
            vertex_ptr->normal.z = extrusion == 0.0f ? 1.0f : -1.0f;
            vertex_ptr++;
            back_vertices++;
        }
        j = face_ptr - back_faces;
        while (j--)
        {
            (*face_ptr)[0] = (*back_faces)[0] + count;
            (*face_ptr)[1] = (*back_faces)[f1] + count;
            (*face_ptr)[2] = (*back_faces)[f2] + count;
            face_ptr++;
            back_faces++;
        }
    }

    *mesh_ptr = mesh;
    hr = D3D_OK;
error:
    if (mesh) {
        if (faces) mesh->lpVtbl->UnlockIndexBuffer(mesh);
        if (vertices) mesh->lpVtbl->UnlockVertexBuffer(mesh);
        if (hr != D3D_OK) mesh->lpVtbl->Release(mesh);
    }
    if (glyphs) {
        for (i = 0; i < textlen; i++)
        {
            int j;
            for (j = 0; j < glyphs[i].outlines.count; j++)
                free(glyphs[i].outlines.items[j].items);
            free(glyphs[i].outlines.items);
            free(glyphs[i].faces.items);
            free(glyphs[i].ordered_vertices.items);
        }
        free(glyphs);
    }
    if (triangulations.items) {
        int i;
        for (i = 0; i < triangulations.count; i++)
            free(triangulations.items[i].vertex_stack.items);
        free(triangulations.items);
    }
    free(raw_outline);
    if (oldfont) SelectObject(hdc, oldfont);
    if (font) DeleteObject(font);

    return hr;
}

HRESULT WINAPI D3DXValidMesh(ID3DXMesh *mesh, const DWORD *adjacency, ID3DXBuffer **errors_and_warnings)
{
    FIXME("mesh %p, adjacency %p, errors_and_warnings %p stub.\n", mesh, adjacency, errors_and_warnings);

    return E_NOTIMPL;
}

static BOOL weld_float1(void *to, void *from, FLOAT epsilon)
{
    FLOAT *v1 = to;
    FLOAT *v2 = from;

    if (fabsf(*v1 - *v2) <= epsilon)
    {
        *v1 = *v2;

        return TRUE;
    }

    return FALSE;
}

static BOOL weld_float2(void *to, void *from, FLOAT epsilon)
{
    D3DXVECTOR2 *v1 = to;
    D3DXVECTOR2 *v2 = from;
    FLOAT diff_x = fabsf(v1->x - v2->x);
    FLOAT diff_y = fabsf(v1->y - v2->y);
    FLOAT max_abs_diff = max(diff_x, diff_y);

    if (max_abs_diff <= epsilon)
    {
        memcpy(to, from, sizeof(D3DXVECTOR2));

        return TRUE;
    }

    return FALSE;
}

static BOOL weld_float3(void *to, void *from, FLOAT epsilon)
{
    D3DXVECTOR3 *v1 = to;
    D3DXVECTOR3 *v2 = from;
    FLOAT diff_x = fabsf(v1->x - v2->x);
    FLOAT diff_y = fabsf(v1->y - v2->y);
    FLOAT diff_z = fabsf(v1->z - v2->z);
    FLOAT max_abs_diff = max(diff_x, diff_y);
    max_abs_diff = max(diff_z, max_abs_diff);

    if (max_abs_diff <= epsilon)
    {
        memcpy(to, from, sizeof(D3DXVECTOR3));

        return TRUE;
    }

    return FALSE;
}

static BOOL weld_float4(void *to, void *from, FLOAT epsilon)
{
    D3DXVECTOR4 *v1 = to;
    D3DXVECTOR4 *v2 = from;
    FLOAT diff_x = fabsf(v1->x - v2->x);
    FLOAT diff_y = fabsf(v1->y - v2->y);
    FLOAT diff_z = fabsf(v1->z - v2->z);
    FLOAT diff_w = fabsf(v1->w - v2->w);
    FLOAT max_abs_diff = max(diff_x, diff_y);
    max_abs_diff = max(diff_z, max_abs_diff);
    max_abs_diff = max(diff_w, max_abs_diff);

    if (max_abs_diff <= epsilon)
    {
        memcpy(to, from, sizeof(D3DXVECTOR4));

        return TRUE;
    }

    return FALSE;
}

static BOOL weld_ubyte4(void *to, void *from, FLOAT epsilon)
{
    BYTE *b1 = to;
    BYTE *b2 = from;
    BYTE truncated_epsilon = (BYTE)epsilon;
    BYTE diff_x = b1[0] > b2[0] ? b1[0] - b2[0] : b2[0] - b1[0];
    BYTE diff_y = b1[1] > b2[1] ? b1[1] - b2[1] : b2[1] - b1[1];
    BYTE diff_z = b1[2] > b2[2] ? b1[2] - b2[2] : b2[2] - b1[2];
    BYTE diff_w = b1[3] > b2[3] ? b1[3] - b2[3] : b2[3] - b1[3];
    BYTE max_diff = max(diff_x, diff_y);
    max_diff = max(diff_z, max_diff);
    max_diff = max(diff_w, max_diff);

    if (max_diff <= truncated_epsilon)
    {
        memcpy(to, from, 4 * sizeof(BYTE));

        return TRUE;
    }

    return FALSE;
}

static BOOL weld_ubyte4n(void *to, void *from, FLOAT epsilon)
{
    return weld_ubyte4(to, from, epsilon * UCHAR_MAX);
}

static BOOL weld_d3dcolor(void *to, void *from, FLOAT epsilon)
{
    return weld_ubyte4n(to, from, epsilon);
}

static BOOL weld_short2(void *to, void *from, FLOAT epsilon)
{
    SHORT *s1 = to;
    SHORT *s2 = from;
    SHORT truncated_epsilon = (SHORT)epsilon;
    SHORT diff_x = abs(s1[0] - s2[0]);
    SHORT diff_y = abs(s1[1] - s2[1]);
    SHORT max_abs_diff = max(diff_x, diff_y);

    if (max_abs_diff <= truncated_epsilon)
    {
        memcpy(to, from, 2 * sizeof(SHORT));

        return TRUE;
    }

    return FALSE;
}

static BOOL weld_short2n(void *to, void *from, FLOAT epsilon)
{
    return weld_short2(to, from, epsilon * SHRT_MAX);
}

static BOOL weld_short4(void *to, void *from, FLOAT epsilon)
{
    SHORT *s1 = to;
    SHORT *s2 = from;
    SHORT truncated_epsilon = (SHORT)epsilon;
    SHORT diff_x = abs(s1[0] - s2[0]);
    SHORT diff_y = abs(s1[1] - s2[1]);
    SHORT diff_z = abs(s1[2] - s2[2]);
    SHORT diff_w = abs(s1[3] - s2[3]);
    SHORT max_abs_diff = max(diff_x, diff_y);
    max_abs_diff = max(diff_z, max_abs_diff);
    max_abs_diff = max(diff_w, max_abs_diff);

    if (max_abs_diff <= truncated_epsilon)
    {
        memcpy(to, from, 4 * sizeof(SHORT));

        return TRUE;
    }

    return FALSE;
}

static BOOL weld_short4n(void *to, void *from, FLOAT epsilon)
{
    return weld_short4(to, from, epsilon * SHRT_MAX);
}

static BOOL weld_ushort2n(void *to, void *from, FLOAT epsilon)
{
    USHORT *s1 = to;
    USHORT *s2 = from;
    USHORT scaled_epsilon = (USHORT)(epsilon * USHRT_MAX);
    USHORT diff_x = s1[0] > s2[0] ? s1[0] - s2[0] : s2[0] - s1[0];
    USHORT diff_y = s1[1] > s2[1] ? s1[1] - s2[1] : s2[1] - s1[1];
    USHORT max_diff = max(diff_x, diff_y);

    if (max_diff <= scaled_epsilon)
    {
        memcpy(to, from, 2 * sizeof(USHORT));

        return TRUE;
    }

    return FALSE;
}

static BOOL weld_ushort4n(void *to, void *from, FLOAT epsilon)
{
    USHORT *s1 = to;
    USHORT *s2 = from;
    USHORT scaled_epsilon = (USHORT)(epsilon * USHRT_MAX);
    USHORT diff_x = s1[0] > s2[0] ? s1[0] - s2[0] : s2[0] - s1[0];
    USHORT diff_y = s1[1] > s2[1] ? s1[1] - s2[1] : s2[1] - s1[1];
    USHORT diff_z = s1[2] > s2[2] ? s1[2] - s2[2] : s2[2] - s1[2];
    USHORT diff_w = s1[3] > s2[3] ? s1[3] - s2[3] : s2[3] - s1[3];
    USHORT max_diff = max(diff_x, diff_y);
    max_diff = max(diff_z, max_diff);
    max_diff = max(diff_w, max_diff);

    if (max_diff <= scaled_epsilon)
    {
        memcpy(to, from, 4 * sizeof(USHORT));

        return TRUE;
    }

    return FALSE;
}

struct udec3
{
    UINT x;
    UINT y;
    UINT z;
    UINT w;
};

static struct udec3 dword_to_udec3(DWORD d)
{
    struct udec3 v;

    v.x = d & 0x3ff;
    v.y = (d & 0xffc00) >> 10;
    v.z = (d & 0x3ff00000) >> 20;
    v.w = (d & 0xc0000000) >> 30;

    return v;
}

static BOOL weld_udec3(void *to, void *from, FLOAT epsilon)
{
    DWORD *d1 = to;
    DWORD *d2 = from;
    struct udec3 v1 = dword_to_udec3(*d1);
    struct udec3 v2 = dword_to_udec3(*d2);
    UINT truncated_epsilon = (UINT)epsilon;
    UINT diff_x = v1.x > v2.x ? v1.x - v2.x : v2.x - v1.x;
    UINT diff_y = v1.y > v2.y ? v1.y - v2.y : v2.y - v1.y;
    UINT diff_z = v1.z > v2.z ? v1.z - v2.z : v2.z - v1.z;
    UINT diff_w = v1.w > v2.w ? v1.w - v2.w : v2.w - v1.w;
    UINT max_diff = max(diff_x, diff_y);
    max_diff = max(diff_z, max_diff);
    max_diff = max(diff_w, max_diff);

    if (max_diff <= truncated_epsilon)
    {
        memcpy(to, from, sizeof(DWORD));

        return TRUE;
    }

    return FALSE;
}

struct dec3n
{
    INT x;
    INT y;
    INT z;
    INT w;
};

static struct dec3n dword_to_dec3n(DWORD d)
{
    struct dec3n v;

    v.x = d & 0x3ff;
    v.y = (d & 0xffc00) >> 10;
    v.z = (d & 0x3ff00000) >> 20;
    v.w = (d & 0xc0000000) >> 30;

    return v;
}

static BOOL weld_dec3n(void *to, void *from, FLOAT epsilon)
{
    const UINT MAX_DEC3N = 511;
    DWORD *d1 = to;
    DWORD *d2 = from;
    struct dec3n v1 = dword_to_dec3n(*d1);
    struct dec3n v2 = dword_to_dec3n(*d2);
    INT scaled_epsilon = (INT)(epsilon * MAX_DEC3N);
    INT diff_x = abs(v1.x - v2.x);
    INT diff_y = abs(v1.y - v2.y);
    INT diff_z = abs(v1.z - v2.z);
    INT diff_w = abs(v1.w - v2.w);
    INT max_abs_diff = max(diff_x, diff_y);
    max_abs_diff = max(diff_z, max_abs_diff);
    max_abs_diff = max(diff_w, max_abs_diff);

    if (max_abs_diff <= scaled_epsilon)
    {
        memcpy(to, from, sizeof(DWORD));

        return TRUE;
    }

    return FALSE;
}

static BOOL weld_float16_2(void *to, void *from, FLOAT epsilon)
{
    D3DXFLOAT16 *v1_float16 = to;
    D3DXFLOAT16 *v2_float16 = from;
    FLOAT diff_x;
    FLOAT diff_y;
    FLOAT max_abs_diff;
#define NUM_ELEM 2
    FLOAT v1[NUM_ELEM];
    FLOAT v2[NUM_ELEM];

    D3DXFloat16To32Array(v1, v1_float16, NUM_ELEM);
    D3DXFloat16To32Array(v2, v2_float16, NUM_ELEM);

    diff_x = fabsf(v1[0] - v2[0]);
    diff_y = fabsf(v1[1] - v2[1]);
    max_abs_diff = max(diff_x, diff_y);

    if (max_abs_diff <= epsilon)
    {
        memcpy(to, from, NUM_ELEM * sizeof(D3DXFLOAT16));

        return TRUE;
    }

    return FALSE;
#undef NUM_ELEM
}

static BOOL weld_float16_4(void *to, void *from, FLOAT epsilon)
{
    D3DXFLOAT16 *v1_float16 = to;
    D3DXFLOAT16 *v2_float16 = from;
    FLOAT diff_x;
    FLOAT diff_y;
    FLOAT diff_z;
    FLOAT diff_w;
    FLOAT max_abs_diff;
#define NUM_ELEM 4
    FLOAT v1[NUM_ELEM];
    FLOAT v2[NUM_ELEM];

    D3DXFloat16To32Array(v1, v1_float16, NUM_ELEM);
    D3DXFloat16To32Array(v2, v2_float16, NUM_ELEM);

    diff_x = fabsf(v1[0] - v2[0]);
    diff_y = fabsf(v1[1] - v2[1]);
    diff_z = fabsf(v1[2] - v2[2]);
    diff_w = fabsf(v1[3] - v2[3]);
    max_abs_diff = max(diff_x, diff_y);
    max_abs_diff = max(diff_z, max_abs_diff);
    max_abs_diff = max(diff_w, max_abs_diff);

    if (max_abs_diff <= epsilon)
    {
        memcpy(to, from, NUM_ELEM * sizeof(D3DXFLOAT16));

        return TRUE;
    }

    return FALSE;
#undef NUM_ELEM
}

/* Sets the vertex components to the same value if they are within epsilon. */
static BOOL weld_component(void *to, void *from, D3DDECLTYPE type, FLOAT epsilon)
{
    /* Quiet FIXMEs as this is in a loop with potentially thousand of iterations. */
    BOOL fixme_once_unused = FALSE;
    BOOL fixme_once_unknown = FALSE;

    switch (type)
    {
        case D3DDECLTYPE_FLOAT1:
            return weld_float1(to, from, epsilon);

        case D3DDECLTYPE_FLOAT2:
            return weld_float2(to, from, epsilon);

        case D3DDECLTYPE_FLOAT3:
            return weld_float3(to, from, epsilon);

        case D3DDECLTYPE_FLOAT4:
            return weld_float4(to, from, epsilon);

        case D3DDECLTYPE_D3DCOLOR:
            return weld_d3dcolor(to, from, epsilon);

        case D3DDECLTYPE_UBYTE4:
            return weld_ubyte4(to, from, epsilon);

        case D3DDECLTYPE_SHORT2:
            return weld_short2(to, from, epsilon);

        case D3DDECLTYPE_SHORT4:
            return weld_short4(to, from, epsilon);

        case D3DDECLTYPE_UBYTE4N:
            return weld_ubyte4n(to, from, epsilon);

        case D3DDECLTYPE_SHORT2N:
            return weld_short2n(to, from, epsilon);

        case D3DDECLTYPE_SHORT4N:
            return weld_short4n(to, from, epsilon);

        case D3DDECLTYPE_USHORT2N:
            return weld_ushort2n(to, from, epsilon);

        case D3DDECLTYPE_USHORT4N:
            return weld_ushort4n(to, from, epsilon);

        case D3DDECLTYPE_UDEC3:
            return weld_udec3(to, from, epsilon);

        case D3DDECLTYPE_DEC3N:
            return weld_dec3n(to, from, epsilon);

        case D3DDECLTYPE_FLOAT16_2:
            return weld_float16_2(to, from, epsilon);

        case D3DDECLTYPE_FLOAT16_4:
            return weld_float16_4(to, from, epsilon);

        case D3DDECLTYPE_UNUSED:
            if (!fixme_once_unused++)
                FIXME("D3DDECLTYPE_UNUSED welding not implemented.\n");
            break;

        default:
            if (!fixme_once_unknown++)
                FIXME("Welding of unknown declaration type %d is not implemented.\n", type);
            break;
    }

    return FALSE;
}

static FLOAT get_component_epsilon(const D3DVERTEXELEMENT9 *decl_ptr, const D3DXWELDEPSILONS *epsilons)
{
    FLOAT epsilon = 0.0f;
    /* Quiet FIXMEs as this is in a loop with potentially thousand of iterations. */
    static BOOL fixme_once_blendindices = FALSE;
    static BOOL fixme_once_positiont = FALSE;
    static BOOL fixme_once_fog = FALSE;
    static BOOL fixme_once_depth = FALSE;
    static BOOL fixme_once_sample = FALSE;
    static BOOL fixme_once_unknown = FALSE;

    switch (decl_ptr->Usage)
    {
        case D3DDECLUSAGE_POSITION:
            epsilon = epsilons->Position;
            break;
        case D3DDECLUSAGE_BLENDWEIGHT:
            epsilon = epsilons->BlendWeights;
            break;
        case D3DDECLUSAGE_NORMAL:
            epsilon = epsilons->Normals;
            break;
        case D3DDECLUSAGE_PSIZE:
            epsilon = epsilons->PSize;
            break;
        case D3DDECLUSAGE_TEXCOORD:
        {
            BYTE usage_index = decl_ptr->UsageIndex;
            if (usage_index > 7)
                usage_index = 7;
            epsilon = epsilons->Texcoords[usage_index];
            break;
        }
        case D3DDECLUSAGE_TANGENT:
            epsilon = epsilons->Tangent;
            break;
        case D3DDECLUSAGE_BINORMAL:
            epsilon = epsilons->Binormal;
            break;
        case D3DDECLUSAGE_TESSFACTOR:
            epsilon = epsilons->TessFactor;
            break;
        case D3DDECLUSAGE_COLOR:
            if (decl_ptr->UsageIndex == 0)
                epsilon = epsilons->Diffuse;
            else if (decl_ptr->UsageIndex == 1)
                epsilon = epsilons->Specular;
            else
                epsilon = 1e-6f;
            break;
        case D3DDECLUSAGE_BLENDINDICES:
            if (!fixme_once_blendindices++)
                FIXME("D3DDECLUSAGE_BLENDINDICES welding not implemented.\n");
            break;
        case D3DDECLUSAGE_POSITIONT:
            if (!fixme_once_positiont++)
                FIXME("D3DDECLUSAGE_POSITIONT welding not implemented.\n");
            break;
        case D3DDECLUSAGE_FOG:
            if (!fixme_once_fog++)
                FIXME("D3DDECLUSAGE_FOG welding not implemented.\n");
            break;
        case D3DDECLUSAGE_DEPTH:
            if (!fixme_once_depth++)
                FIXME("D3DDECLUSAGE_DEPTH welding not implemented.\n");
            break;
        case D3DDECLUSAGE_SAMPLE:
            if (!fixme_once_sample++)
                FIXME("D3DDECLUSAGE_SAMPLE welding not implemented.\n");
            break;
        default:
            if (!fixme_once_unknown++)
                FIXME("Unknown usage %x\n", decl_ptr->Usage);
            break;
    }

    return epsilon;
}

/* Helper function for reading a 32-bit index buffer. */
static inline DWORD read_ib(void *index_buffer, BOOL indices_are_32bit,
                            DWORD index)
{
    if (indices_are_32bit)
    {
        DWORD *indices = index_buffer;
        return indices[index];
    }
    else
    {
        WORD *indices = index_buffer;
        return indices[index];
    }
}

/* Helper function for writing to a 32-bit index buffer. */
static inline void write_ib(void *index_buffer, BOOL indices_are_32bit,
                            DWORD index, DWORD value)
{
    if (indices_are_32bit)
    {
        DWORD *indices = index_buffer;
        indices[index] = value;
    }
    else
    {
        WORD *indices = index_buffer;
        indices[index] = value;
    }
}

/*************************************************************************
 * D3DXWeldVertices    (D3DX9_36.@)
 *
 * Welds together similar vertices. The similarity between vert-
 * ices can be the position and other components such as
 * normal and color.
 *
 * PARAMS
 *   mesh             [I] Mesh which vertices will be welded together.
 *   flags            [I] D3DXWELDEPSILONSFLAGS specifying how to weld.
 *   epsilons         [I] How similar a component needs to be for welding.
 *   adjacency        [I] Which faces are adjacent to other faces.
 *   adjacency_out    [O] Updated adjacency after welding.
 *   face_remap_out   [O] Which faces the old faces have been mapped to.
 *   vertex_remap_out [O] Which vertices the old vertices have been mapped to.
 *
 * RETURNS
 *   Success: D3D_OK.
 *   Failure: D3DERR_INVALIDCALL, E_OUTOFMEMORY.
 *
 * BUGS
 *   Attribute sorting not implemented.
 *
 */
HRESULT WINAPI D3DXWeldVertices(ID3DXMesh *mesh, DWORD flags, const D3DXWELDEPSILONS *epsilons,
        const DWORD *adjacency, DWORD *adjacency_out, DWORD *face_remap_out, ID3DXBuffer **vertex_remap_out)
{
    DWORD *adjacency_generated = NULL;
    const DWORD *adjacency_ptr;
    DWORD *attributes = NULL;
    const FLOAT DEFAULT_EPSILON = 1.0e-6f;
    HRESULT hr;
    DWORD i;
    void *indices = NULL;
    BOOL indices_are_32bit = mesh->lpVtbl->GetOptions(mesh) & D3DXMESH_32BIT;
    DWORD optimize_flags;
    DWORD *point_reps = NULL;
    struct d3dx9_mesh *This = impl_from_ID3DXMesh(mesh);
    DWORD *vertex_face_map = NULL;
    BYTE *vertices = NULL;

    TRACE("mesh %p, flags %#lx, epsilons %p, adjacency %p, adjacency_out %p, face_remap_out %p, "
            "vertex_remap_out %p.\n",
            mesh, flags, epsilons, adjacency, adjacency_out, face_remap_out, vertex_remap_out);

    if (flags == 0)
    {
        WARN("No flags are undefined. Using D3DXWELDEPSILONS_WELDPARTIALMATCHES instead.\n");
        flags = D3DXWELDEPSILONS_WELDPARTIALMATCHES;
    }

    if (adjacency) /* Use supplied adjacency. */
    {
        adjacency_ptr = adjacency;
    }
    else /* Adjacency has to be generated. */
    {
        adjacency_generated = malloc(3 * This->numfaces * sizeof(*adjacency_generated));
        if (!adjacency_generated)
        {
            ERR("Couldn't allocate memory for adjacency_generated.\n");
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        hr = mesh->lpVtbl->GenerateAdjacency(mesh, DEFAULT_EPSILON, adjacency_generated);
        if (FAILED(hr))
        {
            ERR("Couldn't generate adjacency.\n");
            goto cleanup;
        }
        adjacency_ptr = adjacency_generated;
    }

    /* Point representation says which vertices can be replaced. */
    point_reps = malloc(This->numvertices * sizeof(*point_reps));
    if (!point_reps)
    {
        hr = E_OUTOFMEMORY;
        ERR("Couldn't allocate memory for point_reps.\n");
        goto cleanup;
    }
    hr = mesh->lpVtbl->ConvertAdjacencyToPointReps(mesh, adjacency_ptr, point_reps);
    if (FAILED(hr))
    {
        ERR("ConvertAdjacencyToPointReps failed.\n");
        goto cleanup;
    }

    hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, &indices);
    if (FAILED(hr))
    {
        ERR("Couldn't lock index buffer.\n");
        goto cleanup;
    }

    hr = mesh->lpVtbl->LockAttributeBuffer(mesh, 0, &attributes);
    if (FAILED(hr))
    {
        ERR("Couldn't lock attribute buffer.\n");
        goto cleanup;
    }
    vertex_face_map = malloc(This->numvertices * sizeof(*vertex_face_map));
    if (!vertex_face_map)
    {
        hr = E_OUTOFMEMORY;
        ERR("Couldn't allocate memory for vertex_face_map.\n");
        goto cleanup;
    }
    /* Build vertex face map, so that a vertex's face can be looked up. */
    for (i = 0; i < This->numfaces; i++)
    {
        DWORD j;
        for (j = 0; j < 3; j++)
        {
            DWORD index = read_ib(indices, indices_are_32bit, 3*i + j);
            vertex_face_map[index] = i;
        }
    }

    if (flags & D3DXWELDEPSILONS_WELDPARTIALMATCHES)
    {
        hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, (void**)&vertices);
        if (FAILED(hr))
        {
            ERR("Couldn't lock vertex buffer.\n");
            goto cleanup;
        }
        /* For each vertex that can be removed, compare its vertex components
         * with the vertex components from the vertex that can replace it. A
         * vertex is only fully replaced if all the components match and the
         * flag D3DXWELDEPSILONS_DONOTREMOVEVERTICES is not set, and they
         * belong to the same attribute group. Otherwise the vertex components
         * that are within epsilon are set to the same value.
         */
        for (i = 0; i < 3 * This->numfaces; i++)
        {
            D3DVERTEXELEMENT9 *decl_ptr;
            DWORD vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
            DWORD num_vertex_components;
            INT matches = 0;
            BOOL all_match;
            DWORD index = read_ib(indices, indices_are_32bit, i);

            for (decl_ptr = This->cached_declaration, num_vertex_components = 0; decl_ptr->Stream != 0xFF; decl_ptr++, num_vertex_components++)
            {
                BYTE *to = &vertices[vertex_size*index + decl_ptr->Offset];
                BYTE *from = &vertices[vertex_size*point_reps[index] + decl_ptr->Offset];
                FLOAT epsilon = get_component_epsilon(decl_ptr, epsilons);

                /* Don't weld self */
                if (index == point_reps[index])
                {
                    matches++;
                    continue;
                }

                if (weld_component(to, from, decl_ptr->Type, epsilon))
                    matches++;
            }

            all_match = (num_vertex_components == matches);
            if (all_match && !(flags & D3DXWELDEPSILONS_DONOTREMOVEVERTICES))
            {
                DWORD to_face = vertex_face_map[index];
                DWORD from_face = vertex_face_map[point_reps[index]];
                if(attributes[to_face] != attributes[from_face] && !(flags & D3DXWELDEPSILONS_DONOTSPLIT))
                    continue;
                write_ib(indices, indices_are_32bit, i, point_reps[index]);
            }
        }
        mesh->lpVtbl->UnlockVertexBuffer(mesh);
        vertices = NULL;
    }
    else if (flags & D3DXWELDEPSILONS_WELDALL)
    {
        for (i = 0; i < 3 * This->numfaces; i++)
        {
            DWORD index = read_ib(indices, indices_are_32bit, i);
            DWORD to_face = vertex_face_map[index];
            DWORD from_face = vertex_face_map[point_reps[index]];
            if(attributes[to_face] != attributes[from_face] && !(flags & D3DXWELDEPSILONS_DONOTSPLIT))
                continue;
            write_ib(indices, indices_are_32bit, i, point_reps[index]);
        }
    }
    mesh->lpVtbl->UnlockAttributeBuffer(mesh);
    attributes = NULL;
    mesh->lpVtbl->UnlockIndexBuffer(mesh);
    indices = NULL;

    /* Compact mesh using OptimizeInplace */
    optimize_flags = D3DXMESHOPT_COMPACT;
    hr = mesh->lpVtbl->OptimizeInplace(mesh, optimize_flags, adjacency_ptr, adjacency_out, face_remap_out, vertex_remap_out);
    if (FAILED(hr))
    {
        ERR("Couldn't compact mesh.\n");
        goto cleanup;
    }

    hr = D3D_OK;
cleanup:
    free(adjacency_generated);
    free(point_reps);
    free(vertex_face_map);
    if (attributes) mesh->lpVtbl->UnlockAttributeBuffer(mesh);
    if (indices) mesh->lpVtbl->UnlockIndexBuffer(mesh);
    if (vertices) mesh->lpVtbl->UnlockVertexBuffer(mesh);

    return hr;
}

/*************************************************************************
 * D3DXOptimizeFaces    (D3DX9_36.@)
 *
 * Re-orders the faces so the vertex cache is used optimally.
 *
 * PARAMS
 *   indices           [I] Pointer to an index buffer belonging to a mesh.
 *   num_faces         [I] Number of faces in the mesh.
 *   num_vertices      [I] Number of vertices in the mesh.
 *   indices_are_32bit [I] Specifies whether indices are 32- or 16-bit.
 *   face_remap        [I/O] The new order the faces should be drawn in.
 *
 * RETURNS
 *   Success: D3D_OK.
 *   Failure: D3DERR_INVALIDCALL.
 *
 * BUGS
 *   The face re-ordering does not use the vertex cache optimally.
 *
 */
HRESULT WINAPI D3DXOptimizeFaces(const void *indices, UINT num_faces,
        UINT num_vertices, BOOL indices_are_32bit, DWORD *face_remap)
{
    UINT i;
    UINT j = num_faces - 1;
    UINT limit_16_bit = 2 << 15; /* According to MSDN */
    HRESULT hr = D3D_OK;

    FIXME("indices %p, num_faces %u, num_vertices %u, indices_are_32bit %#x, face_remap %p semi-stub. "
            "Face order will not be optimal.\n",
            indices, num_faces, num_vertices, indices_are_32bit, face_remap);

    if (!indices_are_32bit && num_faces >= limit_16_bit)
    {
        WARN("Number of faces must be less than %d when using 16-bit indices.\n",
             limit_16_bit);
        hr = D3DERR_INVALIDCALL;
        goto error;
    }

    if (!face_remap)
    {
        WARN("Face remap pointer is NULL.\n");
        hr = D3DERR_INVALIDCALL;
        goto error;
    }

    /* The faces are drawn in reverse order for simple meshes. This ordering
     * is not optimal for complicated meshes, but will not break anything
     * either. The ordering should be changed to take advantage of the vertex
     * cache on the graphics card.
     *
     * TODO Re-order to take advantage of vertex cache.
     */
    for (i = 0; i < num_faces; i++)
    {
        face_remap[i] = j--;
    }

    return D3D_OK;

error:
    return hr;
}

HRESULT WINAPI D3DXOptimizeVertices(const void *indices, UINT face_count, UINT vertex_count,
        BOOL indices_are_32bit, DWORD *vertex_remap)
{
    unsigned int i;

    FIXME("indices %p, face_count %u, vertex_count %u, indices_are_32bit %#x, vertex_remap %p semi-stub.\n",
            indices, face_count, vertex_count, indices_are_32bit, vertex_remap);

    if (!indices || !face_count || !vertex_count || !vertex_remap)
        return D3DERR_INVALIDCALL;

    for (i = 0; i < vertex_count; ++i)
        vertex_remap[i] = i;

    return D3D_OK;
}

static D3DXVECTOR3 *vertex_element_vec3(BYTE *vertices, const D3DVERTEXELEMENT9 *declaration,
        DWORD vertex_stride, DWORD index)
{
    return (D3DXVECTOR3 *)(vertices + declaration->Offset + index * vertex_stride);
}

static D3DXVECTOR3 read_vec3(BYTE *vertices, const D3DVERTEXELEMENT9 *declaration,
        DWORD vertex_stride, DWORD index)
{
    D3DXVECTOR3 vec3 = {0};
    const D3DXVECTOR3 *src = vertex_element_vec3(vertices, declaration, vertex_stride, index);

    switch (declaration->Type)
    {
        case D3DDECLTYPE_FLOAT1:
            vec3.x = src->x;
            break;
        case D3DDECLTYPE_FLOAT2:
            vec3.x = src->x;
            vec3.y = src->y;
            break;
        case D3DDECLTYPE_FLOAT3:
        case D3DDECLTYPE_FLOAT4:
            vec3 = *src;
            break;
        default:
            ERR("Cannot read vec3\n");
            break;
    }

    return vec3;
}

/*************************************************************************
 * D3DXComputeTangentFrameEx    (D3DX9_36.@)
 */
HRESULT WINAPI D3DXComputeTangentFrameEx(ID3DXMesh *mesh, DWORD texture_in_semantic, DWORD texture_in_index,
        DWORD u_partial_out_semantic, DWORD u_partial_out_index, DWORD v_partial_out_semantic,
        DWORD v_partial_out_index, DWORD normal_out_semantic, DWORD normal_out_index, DWORD options,
        const DWORD *adjacency, float partial_edge_threshold, float singular_point_threshold,
        float normal_edge_threshold, ID3DXMesh **mesh_out, ID3DXBuffer **vertex_mapping)
{
    HRESULT hr;
    void *indices = NULL;
    BYTE *vertices = NULL;
    DWORD *point_reps = NULL;
    size_t normal_size;
    BOOL indices_are_32bit;
    DWORD i, j, num_faces, num_vertices, vertex_stride;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE] = {D3DDECL_END()};
    D3DVERTEXELEMENT9 *position_declaration = NULL, *normal_declaration = NULL;
    DWORD weighting_method = options & (D3DXTANGENT_WEIGHT_EQUAL | D3DXTANGENT_WEIGHT_BY_AREA);

    TRACE("mesh %p, texture_in_semantic %lu, texture_in_index %lu, u_partial_out_semantic %lu, "
            "u_partial_out_index %lu, v_partial_out_semantic %lu, v_partial_out_index %lu, "
            "normal_out_semantic %lu, normal_out_index %lu, options %#lx, adjacency %p, "
            "partial_edge_threshold %.8e, singular_point_threshold %.8e, normal_edge_threshold %.8e, "
            "mesh_out %p, vertex_mapping %p.\n",
            mesh, texture_in_semantic, texture_in_index, u_partial_out_semantic, u_partial_out_index,
            v_partial_out_semantic, v_partial_out_index, normal_out_semantic, normal_out_index,
            options, adjacency, partial_edge_threshold, singular_point_threshold,
            normal_edge_threshold, mesh_out, vertex_mapping);

    if (!mesh)
    {
        WARN("mesh is NULL\n");
        return D3DERR_INVALIDCALL;
    }

    if (weighting_method == (D3DXTANGENT_WEIGHT_EQUAL | D3DXTANGENT_WEIGHT_BY_AREA))
    {
        WARN("D3DXTANGENT_WEIGHT_BY_AREA and D3DXTANGENT_WEIGHT_EQUAL are mutally exclusive\n");
        return D3DERR_INVALIDCALL;
    }

    if (u_partial_out_semantic != D3DX_DEFAULT)
    {
        FIXME("tangent vectors computation is not supported\n");
        return E_NOTIMPL;
    }

    if (v_partial_out_semantic != D3DX_DEFAULT)
    {
        FIXME("binormal vectors computation is not supported\n");
        return E_NOTIMPL;
    }

    if (options & ~(D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS
            | D3DXTANGENT_WEIGHT_EQUAL | D3DXTANGENT_WEIGHT_BY_AREA))
    {
        FIXME("Unsupported options %#lx.\n", options);
        return E_NOTIMPL;
    }

    if (!(options & D3DXTANGENT_CALCULATE_NORMALS))
    {
        FIXME("only normals computation is supported\n");
        return E_NOTIMPL;
    }

    if (!(options & D3DXTANGENT_GENERATE_IN_PLACE) || mesh_out || vertex_mapping)
    {
        FIXME("only D3DXTANGENT_GENERATE_IN_PLACE is supported\n");
        return E_NOTIMPL;
    }

    if (FAILED(hr = mesh->lpVtbl->GetDeclaration(mesh, declaration)))
        return hr;

    for (i = 0; declaration[i].Stream != 0xff; i++)
    {
        if (declaration[i].Usage == D3DDECLUSAGE_POSITION && !declaration[i].UsageIndex)
            position_declaration = &declaration[i];
        if (declaration[i].Usage == normal_out_semantic && declaration[i].UsageIndex == normal_out_index)
            normal_declaration = &declaration[i];
    }

    if (!position_declaration || !normal_declaration)
        return D3DERR_INVALIDCALL;

    if (normal_declaration->Type == D3DDECLTYPE_FLOAT3)
    {
        normal_size = sizeof(D3DXVECTOR3);
    }
    else if (normal_declaration->Type == D3DDECLTYPE_FLOAT4)
    {
        normal_size = sizeof(D3DXVECTOR4);
    }
    else
    {
        WARN("unsupported normals type %u\n", normal_declaration->Type);
        return D3DERR_INVALIDCALL;
    }

    num_faces = mesh->lpVtbl->GetNumFaces(mesh);
    num_vertices = mesh->lpVtbl->GetNumVertices(mesh);
    vertex_stride = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    indices_are_32bit = mesh->lpVtbl->GetOptions(mesh) & D3DXMESH_32BIT;

    point_reps = malloc(num_vertices * sizeof(*point_reps));
    if (!point_reps)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (adjacency)
    {
        if (FAILED(hr = mesh->lpVtbl->ConvertAdjacencyToPointReps(mesh, adjacency, point_reps)))
            goto done;
    }
    else
    {
        for (i = 0; i < num_vertices; i++)
            point_reps[i] = i;
    }

    if (FAILED(hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, &indices)))
        goto done;

    if (FAILED(hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, (void **)&vertices)))
        goto done;

    for (i = 0; i < num_vertices; i++)
    {
        static const D3DXVECTOR4 default_vector = {0.0f, 0.0f, 0.0f, 1.0f};
        void *normal = vertices + normal_declaration->Offset + i * vertex_stride;

        memcpy(normal, &default_vector, normal_size);
    }

    for (i = 0; i < num_faces; i++)
    {
        float denominator, weights[3];
        D3DXVECTOR3 a, b, cross, face_normal;
        const DWORD face_indices[3] =
        {
            read_ib(indices, indices_are_32bit, 3 * i + 0),
            read_ib(indices, indices_are_32bit, 3 * i + 1),
            read_ib(indices, indices_are_32bit, 3 * i + 2)
        };
        const D3DXVECTOR3 v0 = read_vec3(vertices, position_declaration, vertex_stride, face_indices[0]);
        const D3DXVECTOR3 v1 = read_vec3(vertices, position_declaration, vertex_stride, face_indices[1]);
        const D3DXVECTOR3 v2 = read_vec3(vertices, position_declaration, vertex_stride, face_indices[2]);

        D3DXVec3Cross(&cross, D3DXVec3Subtract(&a, &v0, &v1), D3DXVec3Subtract(&b, &v0, &v2));

        switch (weighting_method)
        {
            case D3DXTANGENT_WEIGHT_EQUAL:
                weights[0] = weights[1] = weights[2] = 1.0f;
                break;
            case D3DXTANGENT_WEIGHT_BY_AREA:
                weights[0] = weights[1] = weights[2] = D3DXVec3Length(&cross);
                break;
            default:
                /* weight by angle */
                denominator = D3DXVec3Length(&a) * D3DXVec3Length(&b);
                if (!denominator)
                    weights[0] = 0.0f;
                else
                    weights[0] = acosf(D3DXVec3Dot(&a, &b) / denominator);

                D3DXVec3Subtract(&a, &v1, &v0);
                D3DXVec3Subtract(&b, &v1, &v2);
                denominator = D3DXVec3Length(&a) * D3DXVec3Length(&b);
                if (!denominator)
                    weights[1] = 0.0f;
                else
                    weights[1] = acosf(D3DXVec3Dot(&a, &b) / denominator);

                D3DXVec3Subtract(&a, &v2, &v0);
                D3DXVec3Subtract(&b, &v2, &v1);
                denominator = D3DXVec3Length(&a) * D3DXVec3Length(&b);
                if (!denominator)
                    weights[2] = 0.0f;
                else
                    weights[2] = acosf(D3DXVec3Dot(&a, &b) / denominator);

                break;
        }

        D3DXVec3Normalize(&face_normal, &cross);

        for (j = 0; j < 3; j++)
        {
            D3DXVECTOR3 normal;
            DWORD rep_index = point_reps[face_indices[j]];
            D3DXVECTOR3 *rep_normal = vertex_element_vec3(vertices, normal_declaration, vertex_stride, rep_index);

            D3DXVec3Scale(&normal, &face_normal, weights[j]);
            D3DXVec3Add(rep_normal, rep_normal, &normal);
        }
    }

    for (i = 0; i < num_vertices; i++)
    {
        DWORD rep_index = point_reps[i];
        D3DXVECTOR3 *normal = vertex_element_vec3(vertices, normal_declaration, vertex_stride, i);
        D3DXVECTOR3 *rep_normal = vertex_element_vec3(vertices, normal_declaration, vertex_stride, rep_index);

        if (i == rep_index)
            D3DXVec3Normalize(rep_normal, rep_normal);
        else
            *normal = *rep_normal;
    }

    hr = D3D_OK;

done:
    if (vertices)
        mesh->lpVtbl->UnlockVertexBuffer(mesh);

    if (indices)
        mesh->lpVtbl->UnlockIndexBuffer(mesh);

    free(point_reps);

    return hr;
}

/*************************************************************************
 * D3DXComputeNormals    (D3DX9_36.@)
 */
HRESULT WINAPI D3DXComputeNormals(struct ID3DXBaseMesh *mesh, const DWORD *adjacency)
{
    TRACE("mesh %p, adjacency %p\n", mesh, adjacency);

    if (mesh && (ID3DXMeshVtbl *)mesh->lpVtbl != &D3DXMesh_Vtbl)
    {
        ERR("Invalid virtual table\n");
        return D3DERR_INVALIDCALL;
    }

    return D3DXComputeTangentFrameEx((ID3DXMesh *)mesh, D3DX_DEFAULT, 0,
            D3DX_DEFAULT, 0, D3DX_DEFAULT, 0, D3DDECLUSAGE_NORMAL, 0,
            D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS,
            adjacency, -1.01f, -0.01f, -1.01f, NULL, NULL);
}

/*************************************************************************
 * D3DXIntersect    (D3DX9_36.@)
 */
HRESULT WINAPI D3DXIntersect(ID3DXBaseMesh *mesh, const D3DXVECTOR3 *ray_pos, const D3DXVECTOR3 *ray_dir,
        BOOL *hit, DWORD *face_index, float *u, float *v, float *distance, ID3DXBuffer **all_hits, DWORD *count_of_hits)
{
    FIXME("mesh %p, ray_pos %p, ray_dir %p, hit %p, face_index %p, u %p, v %p, distance %p, all_hits %p, "
            "count_of_hits %p stub!\n", mesh, ray_pos, ray_dir, hit, face_index, u, v, distance, all_hits, count_of_hits);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DXTessellateNPatches(ID3DXMesh *mesh, const DWORD *adjacency_in, float num_segs,
        BOOL quadratic_normals, ID3DXMesh **mesh_out, ID3DXBuffer **adjacency_out)
{
    FIXME("mesh %p, adjacency_in %p, num_segs %f, quadratic_normals %d, mesh_out %p, adjacency_out %p stub.\n",
            mesh, adjacency_in, num_segs, quadratic_normals, mesh_out, adjacency_out);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DXConvertMeshSubsetToSingleStrip(struct ID3DXBaseMesh *mesh_in, DWORD attribute_id,
        DWORD ib_flags, struct IDirect3DIndexBuffer9 **index_buffer, DWORD *index_count)
{
    FIXME("mesh_in %p, attribute_id %lu, ib_flags %#lx, index_buffer %p, index_count %p stub.\n",
            mesh_in, attribute_id, ib_flags, index_buffer, index_count);

    return E_NOTIMPL;
}

struct frame_node
{
    struct list entry;
    D3DXFRAME *frame;
};

static BOOL queue_frame_node(struct list *queue, D3DXFRAME *frame)
{
    struct frame_node *node;

    if (!frame->pFrameFirstChild)
        return TRUE;

    node = malloc(sizeof(*node));
    if (!node)
        return FALSE;

    node->frame = frame;
    list_add_tail(queue, &node->entry);

    return TRUE;
}

static void empty_frame_queue(struct list *queue)
{
    struct frame_node *cur, *cur2;
    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, queue, struct frame_node, entry)
    {
        list_remove(&cur->entry);
        free(cur);
    }
}

D3DXFRAME * WINAPI D3DXFrameFind(const D3DXFRAME *root, const char *name)
{
    D3DXFRAME *found = NULL, *frame;
    struct list queue;

    TRACE("root frame %p, name %s.\n", root, debugstr_a(name));

    if (!root)
        return NULL;

    list_init(&queue);

    frame = (D3DXFRAME *)root;

    for (;;)
    {
        struct frame_node *node;

        while (frame)
        {
            if ((name && frame->Name && !strcmp(frame->Name, name)) || (!name && !frame->Name))
            {
                found = frame;
                goto cleanup;
            }

            if (!queue_frame_node(&queue, frame))
                goto cleanup;

            frame = frame->pFrameSibling;
        }

        if (list_empty(&queue))
            break;

        node = LIST_ENTRY(list_head(&queue), struct frame_node, entry);
        list_remove(&node->entry);
        frame = node->frame->pFrameFirstChild;
        free(node);
    }

cleanup:
    empty_frame_queue(&queue);

    return found;
}
