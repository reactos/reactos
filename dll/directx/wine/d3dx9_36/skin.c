/*
 * Skin Info operations specific to D3DX9.
 *
 * Copyright (C) 2011 Dylan Smith
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

#include "wine/debug.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct bone
{
    char *name;
    D3DXMATRIX transform;
    DWORD num_influences;
    DWORD *vertices;
    FLOAT *weights;
};

typedef struct ID3DXSkinInfoImpl
{
    ID3DXSkinInfo ID3DXSkinInfo_iface;
    LONG ref;

    DWORD fvf;
    D3DVERTEXELEMENT9 vertex_declaration[MAX_FVF_DECL_SIZE];
    DWORD num_vertices;
    DWORD num_bones;
    struct bone *bones;
} ID3DXSkinInfoImpl;

static inline struct ID3DXSkinInfoImpl *impl_from_ID3DXSkinInfo(ID3DXSkinInfo *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXSkinInfoImpl, ID3DXSkinInfo_iface);
}

static HRESULT WINAPI ID3DXSkinInfoImpl_QueryInterface(ID3DXSkinInfo *iface, REFIID riid, void **ppobj)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ID3DXSkinInfo))
    {
        IUnknown_AddRef(iface);
        *ppobj = iface;
        return D3D_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXSkinInfoImpl_AddRef(ID3DXSkinInfo *iface)
{
    struct ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u\n", This, ref);

    return ref;
}

static ULONG WINAPI ID3DXSkinInfoImpl_Release(ID3DXSkinInfo *iface)
{
    struct ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u\n", This, ref);

    if (ref == 0) {
        DWORD i;
        for (i = 0; i < This->num_bones; i++) {
            HeapFree(GetProcessHeap(), 0, This->bones[i].name);
            HeapFree(GetProcessHeap(), 0, This->bones[i].vertices);
            HeapFree(GetProcessHeap(), 0, This->bones[i].weights);
        }
        HeapFree(GetProcessHeap(), 0, This->bones);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_SetBoneInfluence(ID3DXSkinInfo *iface, DWORD bone_num, DWORD num_influences, CONST DWORD *vertices, CONST FLOAT *weights)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);
    struct bone *bone;
    DWORD *new_vertices = NULL;
    FLOAT *new_weights = NULL;

    TRACE("(%p, %u, %u, %p, %p)\n", This, bone_num, num_influences, vertices, weights);

    if (bone_num >= This->num_bones || !vertices || !weights)
        return D3DERR_INVALIDCALL;

    if (num_influences) {
        new_vertices = HeapAlloc(GetProcessHeap(), 0, num_influences * sizeof(*vertices));
        if (!new_vertices)
            return E_OUTOFMEMORY;
        new_weights = HeapAlloc(GetProcessHeap(), 0, num_influences * sizeof(*weights));
        if (!new_weights) {
            HeapFree(GetProcessHeap(), 0, new_vertices);
            return E_OUTOFMEMORY;
        }
        memcpy(new_vertices, vertices, num_influences * sizeof(*vertices));
        memcpy(new_weights, weights, num_influences * sizeof(*weights));
    }
    bone = &This->bones[bone_num];
    bone->num_influences = num_influences;
    HeapFree(GetProcessHeap(), 0, bone->vertices);
    HeapFree(GetProcessHeap(), 0, bone->weights);
    bone->vertices = new_vertices;
    bone->weights = new_weights;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_SetBoneVertexInfluence(ID3DXSkinInfo *iface, DWORD bone_num, DWORD influence_num, float weight)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %u, %u, %g): stub\n", This, bone_num, influence_num, weight);

    return E_NOTIMPL;
}

static DWORD WINAPI ID3DXSkinInfoImpl_GetNumBoneInfluences(ID3DXSkinInfo *iface, DWORD bone_num)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    TRACE("(%p, %u)\n", This, bone_num);

    if (bone_num >= This->num_bones)
        return 0;

    return This->bones[bone_num].num_influences;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_GetBoneInfluence(ID3DXSkinInfo *iface, DWORD bone_num, DWORD *vertices, FLOAT *weights)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);
    struct bone *bone;

    TRACE("(%p, %u, %p, %p)\n", This, bone_num, vertices, weights);

    if (bone_num >= This->num_bones || !vertices)
        return D3DERR_INVALIDCALL;

    bone = &This->bones[bone_num];
    if (!bone->num_influences)
        return D3D_OK;

    memcpy(vertices, bone->vertices, bone->num_influences * sizeof(*vertices));
    if (weights)
        memcpy(weights, bone->weights, bone->num_influences * sizeof(*weights));

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_GetBoneVertexInfluence(ID3DXSkinInfo *iface, DWORD bone_num,
        DWORD influence_num, float *weight, DWORD *vertex_num)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %u, %u, %p, %p): stub\n", This, bone_num, influence_num, weight, vertex_num);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_GetMaxVertexInfluences(ID3DXSkinInfo *iface, DWORD *max_vertex_influences)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %p): stub\n", This, max_vertex_influences);

    return E_NOTIMPL;
}

static DWORD WINAPI ID3DXSkinInfoImpl_GetNumBones(ID3DXSkinInfo *iface)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    TRACE("(%p)\n", This);

    return This->num_bones;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_FindBoneVertexInfluenceIndex(ID3DXSkinInfo *iface, DWORD bone_num,
        DWORD vertex_num, DWORD *influence_index)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %u, %u, %p): stub\n", This, bone_num, vertex_num, influence_index);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_GetMaxFaceInfluences(struct ID3DXSkinInfo *iface,
        struct IDirect3DIndexBuffer9 *index_buffer, DWORD num_faces, DWORD *max_face_influences)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %p, %u, %p): stub\n", This, index_buffer, num_faces, max_face_influences);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_SetMinBoneInfluence(ID3DXSkinInfo *iface, FLOAT min_influence)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %g): stub\n", This, min_influence);

    return E_NOTIMPL;
}

static FLOAT WINAPI ID3DXSkinInfoImpl_GetMinBoneInfluence(ID3DXSkinInfo *iface)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p): stub\n", This);

    return 0.0f;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_SetBoneName(ID3DXSkinInfo *iface, DWORD bone_num, LPCSTR name)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);
    char *new_name;
    size_t size;

    TRACE("(%p, %u, %s)\n", This, bone_num, debugstr_a(name));

    if (bone_num >= This->num_bones || !name)
        return D3DERR_INVALIDCALL;

    size = strlen(name) + 1;
    new_name = HeapAlloc(GetProcessHeap(), 0, size);
    if (!new_name)
        return E_OUTOFMEMORY;
    memcpy(new_name, name, size);
    HeapFree(GetProcessHeap(), 0, This->bones[bone_num].name);
    This->bones[bone_num].name = new_name;

    return D3D_OK;
}

static LPCSTR WINAPI ID3DXSkinInfoImpl_GetBoneName(ID3DXSkinInfo *iface, DWORD bone_num)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    TRACE("(%p, %u)\n", This, bone_num);

    if (bone_num >= This->num_bones)
        return NULL;

    return This->bones[bone_num].name;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_SetBoneOffsetMatrix(ID3DXSkinInfo *iface, DWORD bone_num, CONST D3DXMATRIX *bone_transform)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    TRACE("(%p, %u, %p)\n", This, bone_num, bone_transform);

    if (bone_num >= This->num_bones || !bone_transform)
        return D3DERR_INVALIDCALL;

    This->bones[bone_num].transform = *bone_transform;
    return D3D_OK;
}

static D3DXMATRIX * WINAPI ID3DXSkinInfoImpl_GetBoneOffsetMatrix(ID3DXSkinInfo *iface, DWORD bone_num)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    TRACE("(%p, %u)\n", This, bone_num);

    if (bone_num >= This->num_bones)
        return NULL;

    return &This->bones[bone_num].transform;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_Clone(ID3DXSkinInfo *iface, ID3DXSkinInfo **skin_info)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %p): stub\n", This, skin_info);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_Remap(ID3DXSkinInfo *iface, DWORD num_vertices, DWORD *vertex_remap)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %u, %p): stub\n", This, num_vertices, vertex_remap);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_SetFVF(ID3DXSkinInfo *iface, DWORD fvf)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];

    TRACE("(%p, %x)\n", This, fvf);

    hr = D3DXDeclaratorFromFVF(fvf, declaration);
    if (FAILED(hr)) return hr;

    return iface->lpVtbl->SetDeclaration(iface, declaration);
}

static HRESULT WINAPI ID3DXSkinInfoImpl_SetDeclaration(ID3DXSkinInfo *iface, CONST D3DVERTEXELEMENT9 *declaration)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);
    HRESULT hr;
    int count;

    TRACE("(%p, %p)\n", This, declaration);

    if (!declaration)
        return D3DERR_INVALIDCALL;
    for (count = 0; declaration[count].Stream != 0xff; count++) {
        if (declaration[count].Stream != 0) {
            WARN("Invalid vertex element %u; contains non-zero stream %u\n",
                 count, declaration[count].Stream);
            return D3DERR_INVALIDCALL;
        }
    }
    count++;

    memcpy(This->vertex_declaration, declaration, count * sizeof(*declaration));

    hr = D3DXFVFFromDeclarator(This->vertex_declaration, &This->fvf);
    if (FAILED(hr))
        This->fvf = 0;

    return D3D_OK;
}

static DWORD WINAPI ID3DXSkinInfoImpl_GetFVF(ID3DXSkinInfo *iface)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    TRACE("(%p)\n", This);

    return This->fvf;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_GetDeclaration(ID3DXSkinInfo *iface, D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);
    UINT count = 0;

    TRACE("(%p)\n", This);

    while (This->vertex_declaration[count++].Stream != 0xff);
    memcpy(declaration, This->vertex_declaration, count * sizeof(declaration[0]));
    return D3D_OK;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_UpdateSkinnedMesh(ID3DXSkinInfo *iface, CONST D3DXMATRIX *bone_transforms,
        CONST D3DXMATRIX *bone_inv_transpose_transforms, LPCVOID vertices_src, PVOID vertices_dest)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %p, %p, %p, %p): stub\n", This, bone_transforms, bone_inv_transpose_transforms, vertices_src, vertices_dest);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_ConvertToBlendedMesh(ID3DXSkinInfo *iface, ID3DXMesh *mesh_in,
        DWORD options, const DWORD *adjacency_in, DWORD *adjacency_out, DWORD *face_remap,
        ID3DXBuffer **vertex_remap, DWORD *max_face_infl, DWORD *num_bone_combinations,
        ID3DXBuffer **bone_combination_table, ID3DXMesh **mesh_out)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %p, %x, %p, %p, %p, %p, %p, %p, %p, %p): stub\n",
          This, mesh_in, options, adjacency_in, adjacency_out, face_remap, vertex_remap,
          max_face_infl, num_bone_combinations, bone_combination_table, mesh_out);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSkinInfoImpl_ConvertToIndexedBlendedMesh(ID3DXSkinInfo *iface, ID3DXMesh *mesh_in,
        DWORD options, const DWORD *adjacency_in, DWORD *adjacency_out, DWORD *face_remap,
        ID3DXBuffer **vertex_remap, DWORD *max_face_infl, DWORD *num_bone_combinations,
        ID3DXBuffer **bone_combination_table, ID3DXMesh **mesh_out)
{
    ID3DXSkinInfoImpl *This = impl_from_ID3DXSkinInfo(iface);

    FIXME("(%p, %p, %x, %p, %p, %p, %p, %p, %p, %p, %p): stub\n",
          This, mesh_in, options, adjacency_in, adjacency_out, face_remap, vertex_remap,
          max_face_infl, num_bone_combinations, bone_combination_table, mesh_out);

    return E_NOTIMPL;
}

static const struct ID3DXSkinInfoVtbl ID3DXSkinInfoImpl_Vtbl =
{
    /* IUnknown methods */
    ID3DXSkinInfoImpl_QueryInterface,
    ID3DXSkinInfoImpl_AddRef,
    ID3DXSkinInfoImpl_Release,
    /* ID3DXSkinInfo */
    ID3DXSkinInfoImpl_SetBoneInfluence,
    ID3DXSkinInfoImpl_SetBoneVertexInfluence,
    ID3DXSkinInfoImpl_GetNumBoneInfluences,
    ID3DXSkinInfoImpl_GetBoneInfluence,
    ID3DXSkinInfoImpl_GetBoneVertexInfluence,
    ID3DXSkinInfoImpl_GetMaxVertexInfluences,
    ID3DXSkinInfoImpl_GetNumBones,
    ID3DXSkinInfoImpl_FindBoneVertexInfluenceIndex,
    ID3DXSkinInfoImpl_GetMaxFaceInfluences,
    ID3DXSkinInfoImpl_SetMinBoneInfluence,
    ID3DXSkinInfoImpl_GetMinBoneInfluence,
    ID3DXSkinInfoImpl_SetBoneName,
    ID3DXSkinInfoImpl_GetBoneName,
    ID3DXSkinInfoImpl_SetBoneOffsetMatrix,
    ID3DXSkinInfoImpl_GetBoneOffsetMatrix,
    ID3DXSkinInfoImpl_Clone,
    ID3DXSkinInfoImpl_Remap,
    ID3DXSkinInfoImpl_SetFVF,
    ID3DXSkinInfoImpl_SetDeclaration,
    ID3DXSkinInfoImpl_GetFVF,
    ID3DXSkinInfoImpl_GetDeclaration,
    ID3DXSkinInfoImpl_UpdateSkinnedMesh,
    ID3DXSkinInfoImpl_ConvertToBlendedMesh,
    ID3DXSkinInfoImpl_ConvertToIndexedBlendedMesh
};

HRESULT WINAPI D3DXCreateSkinInfo(DWORD num_vertices, const D3DVERTEXELEMENT9 *declaration,
        DWORD num_bones, ID3DXSkinInfo **skin_info)
{
    HRESULT hr;
    ID3DXSkinInfoImpl *object = NULL;
    static const D3DVERTEXELEMENT9 empty_declaration = D3DDECL_END();

    TRACE("(%u, %p, %u, %p)\n", num_vertices, declaration, num_bones, skin_info);

    if (!skin_info || !declaration)
        return D3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXSkinInfo_iface.lpVtbl = &ID3DXSkinInfoImpl_Vtbl;
    object->ref = 1;
    object->num_vertices = num_vertices;
    object->num_bones = num_bones;
    object->vertex_declaration[0] = empty_declaration;
    object->fvf = 0;

    object->bones = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, num_bones * sizeof(*object->bones));
    if (!object->bones) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = ID3DXSkinInfoImpl_SetDeclaration(&object->ID3DXSkinInfo_iface, declaration);
    if (FAILED(hr)) goto error;

    *skin_info = &object->ID3DXSkinInfo_iface;

    return D3D_OK;
error:
    HeapFree(GetProcessHeap(), 0, object->bones);
    HeapFree(GetProcessHeap(), 0, object);
    return hr;
}

HRESULT WINAPI D3DXCreateSkinInfoFVF(DWORD num_vertices, DWORD fvf, DWORD num_bones, ID3DXSkinInfo **skin_info)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];

    TRACE("(%u, %x, %u, %p)\n", num_vertices, fvf, num_bones, skin_info);

    hr = D3DXDeclaratorFromFVF(fvf, declaration);
    if (FAILED(hr))
        return hr;

    return D3DXCreateSkinInfo(num_vertices, declaration, num_bones, skin_info);
}
