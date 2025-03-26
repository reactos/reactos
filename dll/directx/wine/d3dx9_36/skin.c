#ifdef __REACTOS__
#include "precomp.h"
#else
/*
 * Skin Info operations specific to D3DX9.
 *
 * Copyright (C) 2011 Dylan Smith
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


#include "d3dx9_private.h"
#endif /* __REACTOS__ */

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

    TRACE("%p increasing refcount to %u.\n", skin, refcount);

    return refcount;
}

static ULONG WINAPI d3dx9_skin_info_Release(ID3DXSkinInfo *iface)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    ULONG refcount = InterlockedDecrement(&skin->ref);

    TRACE("%p decreasing refcount to %u.\n", skin, refcount);

    if (!refcount)
    {
        DWORD i;

        for (i = 0; i < skin->num_bones; ++i)
        {
            HeapFree(GetProcessHeap(), 0, skin->bones[i].name);
            HeapFree(GetProcessHeap(), 0, skin->bones[i].vertices);
            HeapFree(GetProcessHeap(), 0, skin->bones[i].weights);
        }
        HeapFree(GetProcessHeap(), 0, skin->bones);
        HeapFree(GetProcessHeap(), 0, skin);
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

    TRACE("iface %p, bone_num %u, num_influences %u, vertices %p, weights %p.\n",
            iface, bone_num, num_influences, vertices, weights);

    if (bone_num >= skin->num_bones || !vertices || !weights)
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
    bone = &skin->bones[bone_num];
    bone->num_influences = num_influences;
    HeapFree(GetProcessHeap(), 0, bone->vertices);
    HeapFree(GetProcessHeap(), 0, bone->weights);
    bone->vertices = new_vertices;
    bone->weights = new_weights;

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_skin_info_SetBoneVertexInfluence(ID3DXSkinInfo *iface,
        DWORD bone_num, DWORD influence_num, float weight)
{
    FIXME("iface %p, bone_num %u, influence_num %u, weight %.8e stub!\n",
            iface, bone_num, influence_num, weight);

    return E_NOTIMPL;
}

static DWORD WINAPI d3dx9_skin_info_GetNumBoneInfluences(ID3DXSkinInfo *iface, DWORD bone_num)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p, bone_num %u.\n", iface, bone_num);

    if (bone_num >= skin->num_bones)
        return 0;

    return skin->bones[bone_num].num_influences;
}

static HRESULT WINAPI d3dx9_skin_info_GetBoneInfluence(ID3DXSkinInfo *iface,
        DWORD bone_num, DWORD *vertices, float *weights)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    struct bone *bone;

    TRACE("iface %p, bone_num %u, vertices %p, weights %p.\n",
            iface, bone_num, vertices, weights);

    if (bone_num >= skin->num_bones || !vertices)
        return D3DERR_INVALIDCALL;

    bone = &skin->bones[bone_num];
    if (!bone->num_influences)
        return D3D_OK;

    memcpy(vertices, bone->vertices, bone->num_influences * sizeof(*vertices));
    if (weights)
        memcpy(weights, bone->weights, bone->num_influences * sizeof(*weights));

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_skin_info_GetBoneVertexInfluence(ID3DXSkinInfo *iface,
        DWORD bone_num, DWORD influence_num, float *weight, DWORD *vertex_num)
{
    FIXME("iface %p, bone_num %u, influence_num %u, weight %p, vertex_num %p stub!\n",
            iface, bone_num, influence_num, weight, vertex_num);

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
        DWORD bone_num, DWORD vertex_num, DWORD *influence_index)
{
    FIXME("iface %p, bone_num %u, vertex_num %u, influence_index %p stub!\n",
            iface, bone_num, vertex_num, influence_index);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_skin_info_GetMaxFaceInfluences(struct ID3DXSkinInfo *iface,
        struct IDirect3DIndexBuffer9 *index_buffer, DWORD num_faces, DWORD *max_face_influences)
{
    FIXME("iface %p, index_buffer %p, num_faces %u, max_face_influences %p stub!\n",
            iface, index_buffer, num_faces, max_face_influences);

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
    size_t size;

    TRACE("iface %p, bone_idx %u, name %s.\n", iface, bone_idx, debugstr_a(name));

    if (bone_idx >= skin->num_bones || !name)
        return D3DERR_INVALIDCALL;

    size = strlen(name) + 1;
    new_name = HeapAlloc(GetProcessHeap(), 0, size);
    if (!new_name)
        return E_OUTOFMEMORY;
    memcpy(new_name, name, size);
    HeapFree(GetProcessHeap(), 0, skin->bones[bone_idx].name);
    skin->bones[bone_idx].name = new_name;

    return D3D_OK;
}

static const char * WINAPI d3dx9_skin_info_GetBoneName(ID3DXSkinInfo *iface, DWORD bone_idx)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p, bone_idx %u.\n", iface, bone_idx);

    if (bone_idx >= skin->num_bones)
        return NULL;

    return skin->bones[bone_idx].name;
}

static HRESULT WINAPI d3dx9_skin_info_SetBoneOffsetMatrix(ID3DXSkinInfo *iface,
        DWORD bone_num, const D3DXMATRIX *bone_transform)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p, bone_num %u, bone_transform %p.\n", iface, bone_num, bone_transform);

    if (bone_num >= skin->num_bones || !bone_transform)
        return D3DERR_INVALIDCALL;

    skin->bones[bone_num].transform = *bone_transform;
    return D3D_OK;
}

static D3DXMATRIX * WINAPI d3dx9_skin_info_GetBoneOffsetMatrix(ID3DXSkinInfo *iface, DWORD bone_num)
{
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);

    TRACE("iface %p, bone_num %u.\n", iface, bone_num);

    if (bone_num >= skin->num_bones)
        return NULL;

    return &skin->bones[bone_num].transform;
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

static HRESULT WINAPI d3dx9_skin_info_Remap(ID3DXSkinInfo *iface, DWORD num_vertices, DWORD *vertex_remap)
{
    FIXME("iface %p, num_vertices %u, vertex_remap %p stub!\n", iface, num_vertices, vertex_remap);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_skin_info_SetFVF(ID3DXSkinInfo *iface, DWORD fvf)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];

    TRACE("iface %p, fvf %#x.\n", iface, fvf);

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
    struct d3dx9_skin_info *skin = impl_from_ID3DXSkinInfo(iface);
    DWORD size = D3DXGetFVFVertexSize(skin->fvf);
    DWORD i, j;

    TRACE("iface %p, bone_transforms %p, bone_inv_transpose_transforms %p, src_vertices %p, dst_vertices %p\n",
            skin, bone_transforms, bone_inv_transpose_transforms, src_vertices, dst_vertices);

    if (bone_inv_transpose_transforms)
        FIXME("Skinning vertices with two position elements not supported\n");

    if ((skin->fvf & D3DFVF_POSITION_MASK) != D3DFVF_XYZ) {
        FIXME("Vertex type %#x not supported\n", skin->fvf & D3DFVF_POSITION_MASK);
        return E_FAIL;
    }

    /* Reset all positions */
    for (i = 0; i < skin->num_vertices; i++) {
        D3DXVECTOR3 *position = (D3DXVECTOR3*)((BYTE*)dst_vertices + size * i);
        position->x = 0.0f;
        position->y = 0.0f;
        position->z = 0.0f;
    }

    /* Update positions that are influenced by bones */
    for (i = 0; i < skin->num_bones; i++) {
        D3DXMATRIX bone_inverse, matrix;

        D3DXMatrixInverse(&bone_inverse, NULL, &skin->bones[i].transform);
        D3DXMatrixMultiply(&matrix, &bone_transforms[i], &bone_inverse);
        D3DXMatrixMultiply(&matrix, &matrix, &skin->bones[i].transform);

        for (j = 0; j < skin->bones[i].num_influences; j++) {
            D3DXVECTOR3 position;
            D3DXVECTOR3 *position_src = (D3DXVECTOR3*)((BYTE*)src_vertices + size * skin->bones[i].vertices[j]);
            D3DXVECTOR3 *position_dest = (D3DXVECTOR3*)((BYTE*)dst_vertices + size * skin->bones[i].vertices[j]);
            FLOAT weight = skin->bones[i].weights[j];

            D3DXVec3TransformCoord(&position, position_src, &matrix);
            position_dest->x += weight * position.x;
            position_dest->y += weight * position.y;
            position_dest->z += weight * position.z;
        }
    }

    if (skin->fvf & D3DFVF_NORMAL) {
        /* Reset all normals */
        for (i = 0; i < skin->num_vertices; i++) {
            D3DXVECTOR3 *normal = (D3DXVECTOR3*)((BYTE*)dst_vertices + size * i + sizeof(D3DXVECTOR3));
            normal->x = 0.0f;
            normal->y = 0.0f;
            normal->z = 0.0f;
        }

        /* Update normals that are influenced by bones */
        for (i = 0; i < skin->num_bones; i++) {
            D3DXMATRIX bone_inverse, matrix;

            D3DXMatrixInverse(&bone_inverse, NULL, &skin->bones[i].transform);
            D3DXMatrixMultiply(&matrix, &skin->bones[i].transform, &bone_transforms[i]);

            for (j = 0; j < skin->bones[i].num_influences; j++) {
                D3DXVECTOR3 normal;
                D3DXVECTOR3 *normal_src = (D3DXVECTOR3*)((BYTE*)src_vertices + size * skin->bones[i].vertices[j] + sizeof(D3DXVECTOR3));
                D3DXVECTOR3 *normal_dest = (D3DXVECTOR3*)((BYTE*)dst_vertices + size * skin->bones[i].vertices[j] + sizeof(D3DXVECTOR3));
                FLOAT weight = skin->bones[i].weights[j];

                D3DXVec3TransformNormal(&normal, normal_src, &bone_inverse);
                D3DXVec3TransformNormal(&normal, &normal, &matrix);
                normal_dest->x += weight * normal.x;
                normal_dest->y += weight * normal.y;
                normal_dest->z += weight * normal.z;
            }
        }

        /* Normalize all normals that are influenced by bones*/
        for (i = 0; i < skin->num_vertices; i++) {
            D3DXVECTOR3 *normal_dest = (D3DXVECTOR3*)((BYTE*)dst_vertices + (i * size) + sizeof(D3DXVECTOR3));
            if ((normal_dest->x != 0.0f) && (normal_dest->y != 0.0f) && (normal_dest->z != 0.0f))
                D3DXVec3Normalize(normal_dest, normal_dest);
        }
    }

    return D3D_OK;
}

static HRESULT WINAPI d3dx9_skin_info_ConvertToBlendedMesh(ID3DXSkinInfo *iface, ID3DXMesh *mesh_in,
        DWORD options, const DWORD *adjacency_in, DWORD *adjacency_out, DWORD *face_remap,
        ID3DXBuffer **vertex_remap, DWORD *max_face_infl, DWORD *num_bone_combinations,
        ID3DXBuffer **bone_combination_table, ID3DXMesh **mesh_out)
{
    FIXME("iface %p, mesh_in %p, options %#x, adjacency_in %p, adjacency_out %p, face_remap %p, vertex_remap %p, "
            "max_face_infl %p, num_bone_combinations %p, bone_combination_table %p, mesh_out %p stub!\n",
            iface, mesh_in, options, adjacency_in, adjacency_out, face_remap, vertex_remap,
            max_face_infl, num_bone_combinations, bone_combination_table, mesh_out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3dx9_skin_info_ConvertToIndexedBlendedMesh(ID3DXSkinInfo *iface, ID3DXMesh *mesh_in,
        DWORD options, DWORD palette_size, const DWORD *adjacency_in, DWORD *adjacency_out, DWORD *face_remap,
        ID3DXBuffer **vertex_remap, DWORD *max_face_infl, DWORD *num_bone_combinations,
        ID3DXBuffer **bone_combination_table, ID3DXMesh **mesh_out)
{
    FIXME("iface %p, mesh_in %p, options %#x, palette_size %u, adjacency_in %p, adjacency_out %p, face_remap %p, vertex_remap %p, "
            "max_face_infl %p, num_bone_combinations %p, bone_combination_table %p, mesh_out %p stub!\n",
            iface, mesh_in, options, palette_size, adjacency_in, adjacency_out, face_remap, vertex_remap,
            max_face_infl, num_bone_combinations, bone_combination_table, mesh_out);

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

HRESULT WINAPI D3DXCreateSkinInfo(DWORD num_vertices, const D3DVERTEXELEMENT9 *declaration,
        DWORD num_bones, ID3DXSkinInfo **skin_info)
{
    HRESULT hr;
    static const D3DVERTEXELEMENT9 empty_declaration = D3DDECL_END();
    struct d3dx9_skin_info *object = NULL;

    TRACE("num_vertices %u, declaration %p, num_bones %u, skin_info %p.\n",
            num_vertices, declaration, num_bones, skin_info);

    if (!skin_info || !declaration)
        return D3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXSkinInfo_iface.lpVtbl = &d3dx9_skin_info_vtbl;
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

    if (FAILED(hr = d3dx9_skin_info_SetDeclaration(&object->ID3DXSkinInfo_iface, declaration)))
        goto error;

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
