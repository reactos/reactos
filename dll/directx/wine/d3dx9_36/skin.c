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


#include "d3dx9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct bone
{
    char *name;
    D3DXMATRIX transform;
    DWORD num_influences;
    DWORD *vertices;
    FLOAT *weights;
};

struct d3dx9_skin_info
{
    ID3DXSkinInfo ID3DXSkinInfo_iface;
    LONG ref;

    DWORD fvf;
    D3DVERTEXELEMENT9 vertex_declaration[MAX_FVF_DECL_SIZE];
    DWORD num_vertices;
    DWORD num_bones;
    struct bone *bones;
};

static inline struct d3dx9_skin_info *impl_from_ID3DXSkinInfo(ID3DXSkinInfo *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx9_skin_info, ID3DXSkinInfo_iface);
}

static HRESULT WINAPI d3dx9_skin_info_QueryInterface(ID3DXSkinInfo *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ID3DXSkinInfo))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return D3D_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx9_skin_info_AddRef(ID3DXSkinInfo *iface)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    ULONG refcount = InterlockedIncrement(&skin->ref);

    TRACE("%p increasing refcount to %lu.\n", skin, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_skin_info_Release(ID3DXSkinInfo *iface)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    ULONG refcount = InterlockedDecrement(&skin->ref);

    TRACE("%p decreasing refcount to %lu.\n", skin, refcount);

    if (!refcount)
    {
        DWORD i;

        for (i = 0; i < skin->num_bones; ++i)
        {
            free(skin->bones[i].name);
            free(skin->bones[i].vertices);
            free(skin->bones[i].weights);
        }
        free(skin->bones);
        free(skin);
    }

    return refcount;
}

static HRESULT WINAPI d3dx9_skin_info_SetBoneInfluence(ID3DXSkinInfo *iface,
        DWORD bone_num, DWORD num_influences, const DWORD *vertices, const float *weights)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    struct bone *bone;
    DWORD *new_vertices = NULL;
    FLOAT *new_weights = NULL;

    TRACE("iface %p, bone_num %lu, num_influences %lu, vertices %p, weights %p.\n",
            iface, bone_num, num_influences, vertices, weights);

    if (bone_num >= skin->num_bones || !vertices || !weights)
        return D3DERR_INVALIDCALL;

    if (num_influences) {
        new_vertices = malloc(num_influences * sizeof(*vertices));
        if (!new_vertices)
            return E_OUTOFMEMORY;
        new_weights = malloc(num_influences * sizeof(*weights));
        if (!new_weights) {
            free(new_vertices);
            return E_OUTOFMEMORY;
        }
        memcpy(new_vertices, vertices, num_influences * sizeof(*vertices));
        memcpy(new_weights, weights, num_influences * sizeof(*weights));
    }
    bone = &skin->bones[bone_num];
    bone->num_influences = num_influences;
    free(bone->vertices);
    free(bone->weights);
    bone->vertices = new_vertices;
    bone->weights = new_weights;

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_skin_info_SetBoneVertexInfluence(ID3DXSkinInfo *iface,
        DWORD bone_idx, DWORD influence_idx, float weight)
{
    FIXME("iface %p, bone_idx %lu, influence_idx %lu, weight %.8e stub!\n",
            iface, bone_idx, influence_idx, weight);

    return E_NOTIMPL;
}

static DWORD WINAPI d3dx9_skin_info_GetNumBoneInfluences(ID3DXSkinInfo *iface, DWORD bone_idx)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p, bone_idx %lu.\n", iface, bone_idx);

    if (bone_idx >= skin->num_bones)
        return 0;

    return skin->bones[bone_idx].num_influences;
}

static HRESULT WINAPI d3dx9_skin_info_GetBoneInfluence(ID3DXSkinInfo *iface,
        DWORD bone_idx, DWORD *vertices, float *weights)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    struct bone *bone;

    TRACE("iface %p, bone_idx %lu, vertices %p, weights %p.\n",
            iface, bone_idx, vertices, weights);

    if (bone_idx >= skin->num_bones || !vertices)
        return D3DERR_INVALIDCALL;

    bone = &skin->bones[bone_idx];
    if (!bone->num_influences)
        return D3D_OK;

    memcpy(vertices, bone->vertices, bone->num_influences * sizeof(*vertices));
    if (weights)
        memcpy(weights, bone->weights, bone->num_influences * sizeof(*weights));

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_skin_info_GetBoneVertexInfluence(ID3DXSkinInfo *iface,
        DWORD bone_idx, DWORD influence_idx, float *weight, DWORD *vertex_idx)
{
    FIXME("iface %p, bone_idx %lu, influence_idx %lu, weight %p, vertex_idx %p stub!\n",
            iface, bone_idx, influence_idx, weight, vertex_idx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_skin_info_GetMaxVertexInfluences(ID3DXSkinInfo *iface, DWORD *max_vertex_influences)
{
    FIXME("iface %p, max_vertex_influences %p stub!\n", iface, max_vertex_influences);

    return E_NOTIMPL;
}

static DWORD WINAPI d3dx9_skin_info_GetNumBones(ID3DXSkinInfo *iface)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p.\n", iface);

    return skin->num_bones;
}

static HRESULT WINAPI d3dx9_skin_info_FindBoneVertexInfluenceIndex(ID3DXSkinInfo *iface,
        DWORD bone_idx, DWORD vertex_idx, DWORD *influence_idx)
{
    FIXME("iface %p, bone_idx %lu, vertex_idx %lu, influence_idx %p stub!\n",
            iface, bone_idx, vertex_idx, influence_idx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_skin_info_GetMaxFaceInfluences(struct ID3DXSkinInfo *iface,
        struct IDirect3DIndexBuffer9 *index_buffer, DWORD face_count, DWORD *max_face_influences)
{
    FIXME("iface %p, index_buffer %p, face_count %lu, max_face_influences %p stub!\n",
            iface, index_buffer, face_count, max_face_influences);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_skin_info_SetMinBoneInfluence(ID3DXSkinInfo *iface, float min_influence)
{
    FIXME("iface %p, min_influence %.8e stub!\n", iface, min_influence);

    return E_NOTIMPL;
}

static float WINAPI d3dx9_skin_info_GetMinBoneInfluence(ID3DXSkinInfo *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0.0f;
}

static HRESULT WINAPI d3dx9_skin_info_SetBoneName(ID3DXSkinInfo *iface, DWORD bone_idx, const char *name)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    char *new_name;

    TRACE("iface %p, bone_idx %lu, name %s.\n", iface, bone_idx, debugstr_a(name));

    if (bone_idx >= skin->num_bones || !name)
        return D3DERR_INVALIDCALL;

    new_name = strdup(name);
    if (!new_name)
        return E_OUTOFMEMORY;
    free(skin->bones[bone_idx].name);
    skin->bones[bone_idx].name = new_name;

    return D3D_OK;
}

static const char * WINAPI d3dx9_skin_info_GetBoneName(ID3DXSkinInfo *iface, DWORD bone_idx)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p, bone_idx %lu.\n", iface, bone_idx);

    if (bone_idx >= skin->num_bones)
        return NULL;

    return skin->bones[bone_idx].name;
}

static HRESULT WINAPI d3dx9_skin_info_SetBoneOffsetMatrix(ID3DXSkinInfo *iface,
        DWORD bone_idx, const D3DXMATRIX *bone_transform)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p, bone_idx %lu, bone_transform %p.\n", iface, bone_idx, bone_transform);

    if (bone_idx >= skin->num_bones || !bone_transform)
        return D3DERR_INVALIDCALL;

    skin->bones[bone_idx].transform = *bone_transform;
    return D3D_OK;
}

static D3DXMATRIX * WINAPI d3dx9_skin_info_GetBoneOffsetMatrix(ID3DXSkinInfo *iface, DWORD bone_idx)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p, bone_idx %lu.\n", iface, bone_idx);

    if (bone_idx >= skin->num_bones)
        return NULL;

    return &skin->bones[bone_idx].transform;
}

static HRESULT WINAPI d3dx9_skin_info_Clone(ID3DXSkinInfo *iface, ID3DXSkinInfo **skin_info)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, skin_info %p.\n", iface, skin_info);

    if (FAILED(hr = D3DXCreateSkinInfo(skin->num_vertices, skin->vertex_declaration, skin->num_bones, skin_info)))
        return hr;

    for (i = 0; i < skin->num_bones; ++i)
    {
        struct bone *current_bone = &skin->bones[i];

        if (current_bone->name && FAILED(hr = (*skin_info)->lpVtbl->SetBoneName(*skin_info, i, current_bone->name)))
            break;
        if (FAILED(hr = (*skin_info)->lpVtbl->SetBoneOffsetMatrix(*skin_info, i, &current_bone->transform)))
            break;
        if (current_bone->vertices && current_bone->weights
                && FAILED(hr = (*skin_info)->lpVtbl->SetBoneInfluence(*skin_info, i, current_bone->num_influences,
                current_bone->vertices, current_bone->weights)))
            break;
    }

    if (FAILED(hr))
        (*skin_info)->lpVtbl->Release(*skin_info);

    return hr;
}

static HRESULT WINAPI d3dx9_skin_info_Remap(ID3DXSkinInfo *iface, DWORD vertex_count, DWORD *vertex_remap)
{
    FIXME("iface %p, vertex_count %lu, vertex_remap %p stub!\n", iface, vertex_count, vertex_remap);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_skin_info_SetFVF(ID3DXSkinInfo *iface, DWORD fvf)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];

    TRACE("iface %p, fvf %#lx.\n", iface, fvf);

    hr = D3DXDeclaratorFromFVF(fvf, declaration);
    if (FAILED(hr)) return hr;

    return iface->lpVtbl->SetDeclaration(iface, declaration);
}

static HRESULT WINAPI d3dx9_skin_info_SetDeclaration(ID3DXSkinInfo *iface, const D3DVERTEXELEMENT9 *declaration)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    HRESULT hr;
    int count;

    TRACE("iface %p, declaration %p.\n", iface, declaration);

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

    memcpy(skin->vertex_declaration, declaration, count * sizeof(*declaration));

    if (FAILED(hr = D3DXFVFFromDeclarator(skin->vertex_declaration, &skin->fvf)))
        skin->fvf = 0;

    return D3D_OK;
}

static DWORD WINAPI d3dx9_skin_info_GetFVF(ID3DXSkinInfo *iface)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p.\n", iface);

    return skin->fvf;
}

static HRESULT WINAPI d3dx9_skin_info_GetDeclaration(ID3DXSkinInfo *iface,
        D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    UINT count = 0;

    TRACE("iface %p, declaration %p.\n", iface, declaration);

    while (skin->vertex_declaration[count++].Stream != 0xff);
    memcpy(declaration, skin->vertex_declaration, count * sizeof(declaration[0]));
    return D3D_OK;
}

static HRESULT WINAPI d3dx9_skin_info_UpdateSkinnedMesh(ID3DXSkinInfo *iface, const D3DXMATRIX *bone_transforms,
        const D3DXMATRIX *bone_inv_transpose_transforms, const void *src_vertices, void *dst_vertices)
{
    FIXME("iface %p, bone_transforms %p, bone_inv_transpose_transforms %p, src_vertices %p, dst_vertices %p stub!\n",
            iface, bone_transforms, bone_inv_transpose_transforms, src_vertices, dst_vertices);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_skin_info_ConvertToBlendedMesh(ID3DXSkinInfo *iface, ID3DXMesh *mesh_in,
        DWORD options, const DWORD *adjacency_in, DWORD *adjacency_out, DWORD *face_remap,
        ID3DXBuffer **vertex_remap, DWORD *max_face_influences, DWORD *bone_combination_count,
        ID3DXBuffer **bone_combination_table, ID3DXMesh **mesh_out)
{
    FIXME("iface %p, mesh_in %p, options %#lx, adjacency_in %p, adjacency_out %p, face_remap %p, vertex_remap %p, "
            "max_face_influences %p, bone_combination_count %p, bone_combination_table %p, mesh_out %p stub!\n",
            iface, mesh_in, options, adjacency_in, adjacency_out, face_remap, vertex_remap,
            max_face_influences, bone_combination_count, bone_combination_table, mesh_out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_skin_info_ConvertToIndexedBlendedMesh(ID3DXSkinInfo *iface, ID3DXMesh *mesh_in,
        DWORD options, DWORD palette_size, const DWORD *adjacency_in, DWORD *adjacency_out, DWORD *face_remap,
        ID3DXBuffer **vertex_remap, DWORD *max_vertex_influences, DWORD *bone_combination_count,
        ID3DXBuffer **bone_combination_table, ID3DXMesh **mesh_out)
{
    FIXME("iface %p, mesh_in %p, options %#lx, palette_size %lu, adjacency_in %p, adjacency_out %p, "
            "face_remap %p, vertex_remap %p, max_vertex_influences %p, bone_combination_count %p, "
            "bone_combination_table %p, mesh_out %p stub!\n",
            iface, mesh_in, options, palette_size, adjacency_in, adjacency_out, face_remap, vertex_remap,
            max_vertex_influences, bone_combination_count, bone_combination_table, mesh_out);

    return E_NOTIMPL;
}

static const struct ID3DXSkinInfoVtbl d3dx9_skin_info_vtbl =
{
    d3dx9_skin_info_QueryInterface,
    d3dx9_skin_info_AddRef,
    d3dx9_skin_info_Release,
    d3dx9_skin_info_SetBoneInfluence,
    d3dx9_skin_info_SetBoneVertexInfluence,
    d3dx9_skin_info_GetNumBoneInfluences,
    d3dx9_skin_info_GetBoneInfluence,
    d3dx9_skin_info_GetBoneVertexInfluence,
    d3dx9_skin_info_GetMaxVertexInfluences,
    d3dx9_skin_info_GetNumBones,
    d3dx9_skin_info_FindBoneVertexInfluenceIndex,
    d3dx9_skin_info_GetMaxFaceInfluences,
    d3dx9_skin_info_SetMinBoneInfluence,
    d3dx9_skin_info_GetMinBoneInfluence,
    d3dx9_skin_info_SetBoneName,
    d3dx9_skin_info_GetBoneName,
    d3dx9_skin_info_SetBoneOffsetMatrix,
    d3dx9_skin_info_GetBoneOffsetMatrix,
    d3dx9_skin_info_Clone,
    d3dx9_skin_info_Remap,
    d3dx9_skin_info_SetFVF,
    d3dx9_skin_info_SetDeclaration,
    d3dx9_skin_info_GetFVF,
    d3dx9_skin_info_GetDeclaration,
    d3dx9_skin_info_UpdateSkinnedMesh,
    d3dx9_skin_info_ConvertToBlendedMesh,
    d3dx9_skin_info_ConvertToIndexedBlendedMesh,
};

HRESULT WINAPI D3DXCreateSkinInfo(DWORD vertex_count, const D3DVERTEXELEMENT9 *declaration,
        DWORD bone_count, ID3DXSkinInfo **skin_info)
{
    HRESULT hr;
    static const D3DVERTEXELEMENT9 empty_declaration = D3DDECL_END();
    struct d3dx9_skin_info *object = NULL;

    TRACE("vertex_count %lu, declaration %p, bone_count %lu, skin_info %p.\n",
            vertex_count, declaration, bone_count, skin_info);

    if (!skin_info || !declaration)
        return D3DERR_INVALIDCALL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXSkinInfo_iface.lpVtbl = &d3dx9_skin_info_vtbl;
    object->ref = 1;
    object->num_vertices = vertex_count;
    object->num_bones = bone_count;
    object->vertex_declaration[0] = empty_declaration;
    object->fvf = 0;

    object->bones = calloc(bone_count, sizeof(*object->bones));
    if (!object->bones) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    if (FAILED(hr = d3dx9_skin_info_SetDeclaration(&object->ID3DXSkinInfo_iface, declaration)))
        goto error;

    *skin_info = &object->ID3DXSkinInfo_iface;

    return D3D_OK;
error:
    free(object->bones);
    free(object);
    return hr;
}

HRESULT WINAPI D3DXCreateSkinInfoFVF(DWORD vertex_count, DWORD fvf, DWORD bone_count, ID3DXSkinInfo **skin_info)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];

    TRACE("vertex_count %lu, fvf %#lx, bone_count %lu, skin_info %p.\n",
            vertex_count, fvf, bone_count, skin_info);

    hr = D3DXDeclaratorFromFVF(fvf, declaration);
    if (FAILED(hr))
        return hr;

    return D3DXCreateSkinInfo(vertex_count, declaration, bone_count, skin_info);
}
