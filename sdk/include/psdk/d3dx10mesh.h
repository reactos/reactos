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
 */

#include "d3dx10.h"

DEFINE_GUID(IID_ID3DX10Mesh, 0x4020e5c2, 0x1403, 0x4929, 0x88, 0x3f, 0xe2, 0xe8, 0x49, 0xfa, 0xc1, 0x95);
DEFINE_GUID(IID_ID3DX10MeshBuffer, 0x4b0d117, 0x1041, 0x46b1, 0xaa, 0x8a, 0x39, 0x52, 0x84, 0x8b, 0xa2, 0x2e);

typedef enum _D3DX10_MESH_DISCARD_FLAGS
{
    D3DX10_MESH_DISCARD_ATTRIBUTE_BUFFER = 0x01,
    D3DX10_MESH_DISCARD_ATTRIBUTE_TABLE  = 0x02,
    D3DX10_MESH_DISCARD_POINTREPS        = 0x04,
    D3DX10_MESH_DISCARD_ADJACENCY        = 0x08,
    D3DX10_MESH_DISCARD_DEVICE_BUFFERS   = 0x10,
} D3DX10_MESH_DISCARD_FLAGS;

typedef struct _D3DX10_ATTRIBUTE_RANGE
{
    UINT AttribId;
    UINT FaceStart;
    UINT FaceCount;
    UINT VertexStart;
    UINT VertexCount;
} D3DX10_ATTRIBUTE_RANGE;
typedef D3DX10_ATTRIBUTE_RANGE* LPD3DX10_ATTRIBUTE_RANGE;

#define INTERFACE  ID3DX10MeshBuffer
DECLARE_INTERFACE_(ID3DX10MeshBuffer, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DX10MeshBuffer methods ***/
    STDMETHOD(Map)(THIS_ void **data, SIZE_T *size) PURE;
    STDMETHOD(Unmap)(THIS) PURE;
    STDMETHOD_(SIZE_T, GetSize)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE  ID3DX10Mesh
DECLARE_INTERFACE_(ID3DX10Mesh, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DX10Mesh methods ***/
    STDMETHOD_(UINT, GetFaceCount)(THIS) PURE;
    STDMETHOD_(UINT, GetVertexCount)(THIS) PURE;
    STDMETHOD_(UINT, GetVertexBufferCount)(THIS) PURE;
    STDMETHOD_(UINT, GetFlags)(THIS) PURE;
    STDMETHOD(GetVertexDescription)(THIS_ const D3D10_INPUT_ELEMENT_DESC **desc, UINT *count) PURE;
    STDMETHOD(SetVertexData)(THIS_ UINT index, const void *data) PURE;
    STDMETHOD(GetVertexBuffer)(THIS_ UINT index, ID3DX10MeshBuffer **buffer) PURE;
    STDMETHOD(SetIndexData)(THIS_ const void *data, UINT count) PURE;
    STDMETHOD(GetIndexBuffer)(THIS_ ID3DX10MeshBuffer **buffer) PURE;
    STDMETHOD(SetAttributeData)(THIS_ const UINT *data) PURE;
    STDMETHOD(GetAttributeBuffer)(THIS_ ID3DX10MeshBuffer **buffer) PURE;
    STDMETHOD(SetAttributeTable)(THIS_ const D3DX10_ATTRIBUTE_RANGE *table, UINT size) PURE;
    STDMETHOD(GetAttributeTable)(THIS_ D3DX10_ATTRIBUTE_RANGE *table, UINT *size) PURE;
    STDMETHOD(GenerateAdjacencyAndPointReps)(THIS_ float epsilon) PURE;
    STDMETHOD(GenerateGSAdjacency)(THIS) PURE;
    STDMETHOD(SetAdjacencyData)(THIS_ const UINT *adjacency) PURE;
    STDMETHOD(GetAdjacenctBuffer)(THIS_ ID3DX10MeshBuffer **buffer) PURE;
    STDMETHOD(SetPointRepData)(THIS_ const UINT *pointreps) PURE;
    STDMETHOD(GetPointRepBuffer)(THIS_ ID3DX10MeshBuffer **buffer) PURE;
    STDMETHOD(Discard)(THIS_ D3DX10_MESH_DISCARD_FLAGS flags) PURE;
    STDMETHOD(CloneMesh)(THIS_ UINT flags, const char *pos_semantic, const D3D10_INPUT_ELEMENT_DESC *desc,
            UINT decl_count, ID3DX10Mesh **cloned_mesh);
    STDMETHOD(Optimize)(THIS_ UINT flags, UINT *face_remap, ID3D10Blob **vertex_remap) PURE;
    STDMETHOD(GenerateAttributeBufferFromTable)(THIS) PURE;
    STDMETHOD(Intersect)(THIS_ D3DXVECTOR3 *ray_pos, D3DXVECTOR3 *ray_dir, UINT *hit_count, UINT *face_index,
            float *u, float *v, float *dist, ID3D10Blob **all_hits) PURE;
    STDMETHOD(IntersectSubset)(THIS_ UINT attr_id, D3DXVECTOR3 *ray_pos, D3DXVECTOR3 *ray_dir, UINT *hit_count,
            float *u, float *v, float *dist, ID3D10Blob **all_hits) PURE;
    STDMETHOD(CommitToDevice)(THIS) PURE;
    STDMETHOD(DrawSubset)(THIS_ UINT attr_id) PURE;
    STDMETHOD(DrawSubsetInstanced)(THIS_ UINT attr_id, UINT instance_count, UINT start_instance) PURE;
    STDMETHOD(GetDeviceVertexBuffer)(THIS_ UINT index, ID3D10Buffer **buffer) PURE;
    STDMETHOD(GetDeviceIndexBuffer)(THIS_ ID3D10Buffer **buffer) PURE;
};
#undef INTERFACE
