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

struct d3dx10_mesh
{
    ID3DX10Mesh ID3DX10Mesh_iface;
    LONG refcount;
};

static inline struct d3dx10_mesh *impl_from_ID3DX10Mesh(ID3DX10Mesh *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx10_mesh, ID3DX10Mesh_iface);
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_QueryInterface(ID3DX10Mesh *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DX10Mesh)
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

static ULONG STDMETHODCALLTYPE d3dx10_mesh_AddRef(ID3DX10Mesh *iface)
{
    struct d3dx10_mesh *mesh = impl_from_ID3DX10Mesh(iface);
    ULONG refcount = InterlockedIncrement(&mesh->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3dx10_mesh_Release(ID3DX10Mesh *iface)
{
    struct d3dx10_mesh *mesh = impl_from_ID3DX10Mesh(iface);
    ULONG refcount = InterlockedDecrement(&mesh->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(mesh);

    return refcount;
}

static UINT STDMETHODCALLTYPE d3dx10_mesh_GetFaceCount(ID3DX10Mesh *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static UINT STDMETHODCALLTYPE d3dx10_mesh_GetVertexCount(ID3DX10Mesh *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static UINT STDMETHODCALLTYPE d3dx10_mesh_GetVertexBufferCount(ID3DX10Mesh *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static UINT STDMETHODCALLTYPE d3dx10_mesh_GetFlags(ID3DX10Mesh *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GetVertexDescription(ID3DX10Mesh *iface,
        const D3D10_INPUT_ELEMENT_DESC **desc, UINT *count)
{
    FIXME("iface %p, desc %p, count %p stub!\n", iface, desc, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_SetVertexData(ID3DX10Mesh *iface, UINT index,
        const void *data)
{
    FIXME("iface %p, index %u, data %p stub!\n", iface, index, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GetVertexBuffer(ID3DX10Mesh *iface, UINT index,
        ID3DX10MeshBuffer **buffer)
{
    FIXME("iface %p, index %u, buffer %p stub!\n", iface, index, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_SetIndexData(ID3DX10Mesh *iface, const void *data,
        UINT count)
{
    FIXME("iface %p, data %p, count %u stub!\n", iface, data, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GetIndexBuffer(ID3DX10Mesh *iface,
        ID3DX10MeshBuffer **buffer)
{
    FIXME("iface %p, buffer %p stub!\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_SetAttributeData(ID3DX10Mesh *iface,
        const UINT *data)
{
    FIXME("iface %p, data %p stub!\n", iface, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GetAttributeBuffer(ID3DX10Mesh *iface,
        ID3DX10MeshBuffer **buffer)
{
    FIXME("iface %p, buffer %p stub!\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_SetAttributeTable(ID3DX10Mesh *iface,
        const D3DX10_ATTRIBUTE_RANGE *table, UINT count)
{
    FIXME("iface %p, table %p, count %u stub!\n", iface, table, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GetAttributeTable(ID3DX10Mesh *iface,
        D3DX10_ATTRIBUTE_RANGE *table, UINT *count)
{
    FIXME("iface %p, table %p, count %p stub!\n", iface, table, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GenerateAdjacencyAndPointReps(ID3DX10Mesh *iface,
        float epsilon)
{
    FIXME("iface %p, epsilon %.8e stub!\n", iface, epsilon);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GenerateGSAdjacency(ID3DX10Mesh *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_SetAdjacencyData(ID3DX10Mesh *iface,
        const UINT *adjacency)
{
    FIXME("iface %p, adjacency %p stub!\n", iface, adjacency);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GetAdjacencyBuffer(ID3DX10Mesh *iface,
        ID3DX10MeshBuffer **buffer)
{
    FIXME("iface %p, buffer %p stub!\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_SetPointRepData(ID3DX10Mesh *iface,
        const UINT *pointreps)
{
    FIXME("iface %p, pointreps %p stub!\n", iface, pointreps);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GetPointRepBuffer(ID3DX10Mesh *iface,
        ID3DX10MeshBuffer **buffer)
{
    FIXME("iface %p, buffer %p stub!\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_Discard(ID3DX10Mesh *iface,
        D3DX10_MESH_DISCARD_FLAGS flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_CloneMesh(ID3DX10Mesh *iface, UINT flags,
        const char *pos_semantic, const D3D10_INPUT_ELEMENT_DESC *desc, UINT decl_count,
        ID3DX10Mesh **cloned_mesh)
{
    FIXME("iface %p, flags %#x, pos_semantic %s, desc %p, decl_count %u, cloned_mesh %p stub!\n",
            iface, flags, debugstr_a(pos_semantic), desc, decl_count, cloned_mesh);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_Optimize(ID3DX10Mesh *iface, UINT flags,
        UINT *face_remap, ID3D10Blob **vertex_remap)
{
    FIXME("iface %p, flags %#x, face_remap %p, vertex_remap %p stub!\n", iface, flags,
            face_remap, vertex_remap);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GenerateAttributeBufferFromTable(ID3DX10Mesh *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_Intersect(ID3DX10Mesh *iface, D3DXVECTOR3 *ray_pos,
        D3DXVECTOR3 *ray_dir, UINT *hit_count, UINT *face_index, float *u, float *v, float *dist,
        ID3D10Blob **all_hits)
{
    FIXME("iface %p, ray_pos %p, ray_dir %p, hit_count %p, u %p, v %p, dist %p, all_hits %p stub!\n",
            iface, ray_pos, ray_dir, hit_count, u, v, dist, all_hits);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_IntersectSubset(ID3DX10Mesh *iface, UINT attr_id,
        D3DXVECTOR3 *ray_pos, D3DXVECTOR3 *ray_dir, UINT *hit_count, float *u, float *v,
        float *dist, ID3D10Blob **all_hits)
{
    FIXME("iface %p, attr_id %u, ray_pos %p, ray_dir %p, hit_count %p, u %p, v %p, dist %p, all_hits %p stub!\n",
            iface, attr_id, ray_pos, ray_dir, hit_count, u, v, dist, all_hits);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_CommitToDevice(ID3DX10Mesh *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_DrawSubset(ID3DX10Mesh *iface, UINT attr_id)
{
    FIXME("iface %p, attr_id %u stub!\n", iface, attr_id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_DrawSubsetInstanced(ID3DX10Mesh *iface, UINT attr_id,
        UINT instance_count, UINT start_instance)
{
    FIXME("iface %p, attr_id %u, instance_count %u, start_instance %u stub!\n", iface, attr_id,
            instance_count, start_instance);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GetDeviceVertexBuffer(ID3DX10Mesh *iface, UINT index,
        ID3D10Buffer **buffer)
{
    FIXME("iface %p, index %u, buffer %p stub!\n", iface, index, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dx10_mesh_GetDeviceIndexBuffer(ID3DX10Mesh *iface,
        ID3D10Buffer **buffer)
{
    FIXME("iface %p, buffer %p stub!\n", iface, buffer);

    return E_NOTIMPL;
}

static const ID3DX10MeshVtbl d3dx10_mesh_vtbl =
{
    d3dx10_mesh_QueryInterface,
    d3dx10_mesh_AddRef,
    d3dx10_mesh_Release,
    d3dx10_mesh_GetFaceCount,
    d3dx10_mesh_GetVertexCount,
    d3dx10_mesh_GetVertexBufferCount,
    d3dx10_mesh_GetFlags,
    d3dx10_mesh_GetVertexDescription,
    d3dx10_mesh_SetVertexData,
    d3dx10_mesh_GetVertexBuffer,
    d3dx10_mesh_SetIndexData,
    d3dx10_mesh_GetIndexBuffer,
    d3dx10_mesh_SetAttributeData,
    d3dx10_mesh_GetAttributeBuffer,
    d3dx10_mesh_SetAttributeTable,
    d3dx10_mesh_GetAttributeTable,
    d3dx10_mesh_GenerateAdjacencyAndPointReps,
    d3dx10_mesh_GenerateGSAdjacency,
    d3dx10_mesh_SetAdjacencyData,
    d3dx10_mesh_GetAdjacencyBuffer,
    d3dx10_mesh_SetPointRepData,
    d3dx10_mesh_GetPointRepBuffer,
    d3dx10_mesh_Discard,
    d3dx10_mesh_CloneMesh,
    d3dx10_mesh_Optimize,
    d3dx10_mesh_GenerateAttributeBufferFromTable,
    d3dx10_mesh_Intersect,
    d3dx10_mesh_IntersectSubset,
    d3dx10_mesh_CommitToDevice,
    d3dx10_mesh_DrawSubset,
    d3dx10_mesh_DrawSubsetInstanced,
    d3dx10_mesh_GetDeviceVertexBuffer,
    d3dx10_mesh_GetDeviceIndexBuffer,
};

HRESULT WINAPI D3DX10CreateMesh(ID3D10Device *device, const D3D10_INPUT_ELEMENT_DESC *decl,
        UINT decl_count, const char *position_semantic, UINT vertex_count, UINT face_count,
        UINT options, ID3DX10Mesh **mesh)
{
    struct d3dx10_mesh *object;

    FIXME("device %p, decl %p, decl_count %u, position_semantic %s, vertex_count %u, face_count %u, "
            "options %#x, mesh %p semi-stub.\n", device, decl, decl_count, debugstr_a(position_semantic), vertex_count,
            face_count, options, mesh);

    *mesh = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID3DX10Mesh_iface.lpVtbl = &d3dx10_mesh_vtbl;
    object->refcount = 1;

    *mesh = &object->ID3DX10Mesh_iface;

    return S_OK;
}
