/*
 * Implementation of IDirect3DRMMeshBuilderX and IDirect3DRMMesh interfaces
 *
 * Copyright 2010, 2012 Christian Costa
 * Copyright 2011 André Hentschel
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

#include "d3drm_private.h"

struct mesh_group
{
    unsigned nb_vertices;
    D3DRMVERTEX* vertices;
    unsigned nb_faces;
    unsigned vertex_per_face;
    DWORD face_data_size;
    unsigned* face_data;
    D3DCOLOR color;
    IDirect3DRMMaterial2* material;
    IDirect3DRMTexture3* texture;
};

struct d3drm_mesh
{
    IDirect3DRMMesh IDirect3DRMMesh_iface;
    LONG ref;
    DWORD groups_capacity;
    DWORD nb_groups;
    struct mesh_group *groups;
};

struct coords_2d
{
    D3DVALUE u;
    D3DVALUE v;
};

struct mesh_material
{
    D3DCOLOR color;
    IDirect3DRMMaterial2 *material;
    IDirect3DRMTexture3 *texture;
};

struct d3drm_mesh_builder
{
    IDirect3DRMMeshBuilder2 IDirect3DRMMeshBuilder2_iface;
    IDirect3DRMMeshBuilder3 IDirect3DRMMeshBuilder3_iface;
    LONG ref;
    char* name;
    DWORD nb_vertices;
    D3DVECTOR* pVertices;
    DWORD nb_normals;
    D3DVECTOR* pNormals;
    DWORD nb_faces;
    DWORD face_data_size;
    void *pFaceData;
    DWORD nb_coords2d;
    struct coords_2d *pCoords2d;
    D3DCOLOR color;
    IDirect3DRMMaterial2 *material;
    IDirect3DRMTexture3 *texture;
    DWORD nb_materials;
    struct mesh_material *materials;
    DWORD *material_indices;
};

char templates[] = {
"xof 0302txt 0064"
"template Header"
"{"
"<3D82AB43-62DA-11CF-AB39-0020AF71E433>"
"WORD major;"
"WORD minor;"
"DWORD flags;"
"}"
"template Vector"
"{"
"<3D82AB5E-62DA-11CF-AB39-0020AF71E433>"
"FLOAT x;"
"FLOAT y;"
"FLOAT z;"
"}"
"template Coords2d"
"{"
"<F6F23F44-7686-11CF-8F52-0040333594A3>"
"FLOAT u;"
"FLOAT v;"
"}"
"template Matrix4x4"
"{"
"<F6F23F45-7686-11CF-8F52-0040333594A3>"
"array FLOAT matrix[16];"
"}"
"template ColorRGBA"
"{"
"<35FF44E0-6C7C-11CF-8F52-0040333594A3>"
"FLOAT red;"
"FLOAT green;"
"FLOAT blue;"
"FLOAT alpha;"
"}"
"template ColorRGB"
"{"
"<D3E16E81-7835-11CF-8F52-0040333594A3>"
"FLOAT red;"
"FLOAT green;"
"FLOAT blue;"
"}"
"template IndexedColor"
"{"
"<1630B820-7842-11CF-8F52-0040333594A3>"
"DWORD index;"
"ColorRGBA indexColor;"
"}"
"template Boolean"
"{"
"<537DA6A0-CA37-11D0-941C-0080C80CFA7B>"
"DWORD truefalse;"
"}"
"template Boolean2d"
"{"
"<4885AE63-78E8-11CF-8F52-0040333594A3>"
"Boolean u;"
"Boolean v;"
"}"
"template MaterialWrap"
"{"
"<4885AE60-78E8-11CF-8F52-0040333594A3>"
"Boolean u;"
"Boolean v;"
"}"
"template TextureFilename"
"{"
"<A42790E1-7810-11CF-8F52-0040333594A3>"
"STRING filename;"
"}"
"template Material"
"{"
"<3D82AB4D-62DA-11CF-AB39-0020AF71E433>"
"ColorRGBA faceColor;"
"FLOAT power;"
"ColorRGB specularColor;"
"ColorRGB emissiveColor;"
"[...]"
"}"
"template MeshFace"
"{"
"<3D82AB5F-62DA-11CF-AB39-0020AF71E433>"
"DWORD nFaceVertexIndices;"
"array DWORD faceVertexIndices[nFaceVertexIndices];"
"}"
"template MeshFaceWraps"
"{"
"<ED1EC5C0-C0A8-11D0-941C-0080C80CFA7B>"
"DWORD nFaceWrapValues;"
"array Boolean2d faceWrapValues[nFaceWrapValues];"
"}"
"template MeshTextureCoords"
"{"
"<F6F23F40-7686-11CF-8F52-0040333594A3>"
"DWORD nTextureCoords;"
"array Coords2d textureCoords[nTextureCoords];"
"}"
"template MeshMaterialList"
"{"
"<F6F23F42-7686-11CF-8F52-0040333594A3>"
"DWORD nMaterials;"
"DWORD nFaceIndexes;"
"array DWORD faceIndexes[nFaceIndexes];"
"[Material]"
"}"
"template MeshNormals"
"{"
"<F6F23F43-7686-11CF-8F52-0040333594A3>"
"DWORD nNormals;"
"array Vector normals[nNormals];"
"DWORD nFaceNormals;"
"array MeshFace faceNormals[nFaceNormals];"
"}"
"template MeshVertexColors"
"{"
"<1630B821-7842-11CF-8F52-0040333594A3>"
"DWORD nVertexColors;"
"array IndexedColor vertexColors[nVertexColors];"
"}"
"template Mesh"
"{"
"<3D82AB44-62DA-11CF-AB39-0020AF71E433>"
"DWORD nVertices;"
"array Vector vertices[nVertices];"
"DWORD nFaces;"
"array MeshFace faces[nFaces];"
"[...]"
"}"
"template FrameTransformMatrix"
"{"
"<F6F23F41-7686-11CF-8F52-0040333594A3>"
"Matrix4x4 frameMatrix;"
"}"
"template Frame"
"{"
"<3D82AB46-62DA-11CF-AB39-0020AF71E433>"
"[...]"
"}"
"template FloatKeys"
"{"
"<10DD46A9-775B-11CF-8F52-0040333594A3>"
"DWORD nValues;"
"array FLOAT values[nValues];"
"}"
"template TimedFloatKeys"
"{"
"<F406B180-7B3B-11CF-8F52-0040333594A3>"
"DWORD time;"
"FloatKeys tfkeys;"
"}"
"template AnimationKey"
"{"
"<10DD46A8-775B-11CF-8F52-0040333594A3>"
"DWORD keyType;"
"DWORD nKeys;"
"array TimedFloatKeys keys[nKeys];"
"}"
"template AnimationOptions"
"{"
"<E2BF56C0-840F-11CF-8F52-0040333594A3>"
"DWORD openclosed;"
"DWORD positionquality;"
"}"
"template Animation"
"{"
"<3D82AB4F-62DA-11CF-AB39-0020AF71E433>"
"[...]"
"}"
"template AnimationSet"
"{"
"<3D82AB50-62DA-11CF-AB39-0020AF71E433>"
"[Animation]"
"}"
"template InlineData"
"{"
"<3A23EEA0-94B1-11D0-AB39-0020AF71E433>"
"[BINARY]"
"}"
"template Url"
"{"
"<3A23EEA1-94B1-11D0-AB39-0020AF71E433>"
"DWORD nUrls;"
"array STRING urls[nUrls];"
"}"
"template ProgressiveMesh"
"{"
"<8A63C360-997D-11D0-941C-0080C80CFA7B>"
"[Url,InlineData]"
"}"
"template Guid"
"{"
"<A42790E0-7810-11CF-8F52-0040333594A3>"
"DWORD data1;"
"WORD data2;"
"WORD data3;"
"array UCHAR data4[8];"
"}"
"template StringProperty"
"{"
"<7F0F21E0-BFE1-11D1-82C0-00A0C9697271>"
"STRING key;"
"STRING value;"
"}"
"template PropertyBag"
"{"
"<7F0F21E1-BFE1-11D1-82C0-00A0C9697271>"
"[StringProperty]"
"}"
"template ExternalVisual"
"{"
"<98116AA0-BDBA-11D1-82C0-00A0C9697271>"
"Guid guidExternalVisual;"
"[...]"
"}"
"template RightHanded"
"{"
"<7F5D5EA0-D53A-11D1-82C0-00A0C9697271>"
"DWORD bRightHanded;"
"}"
};

static inline struct d3drm_mesh *impl_from_IDirect3DRMMesh(IDirect3DRMMesh *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_mesh, IDirect3DRMMesh_iface);
}

static inline struct d3drm_mesh_builder *impl_from_IDirect3DRMMeshBuilder2(IDirect3DRMMeshBuilder2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_mesh_builder, IDirect3DRMMeshBuilder2_iface);
}

static inline struct d3drm_mesh_builder *impl_from_IDirect3DRMMeshBuilder3(IDirect3DRMMeshBuilder3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_mesh_builder, IDirect3DRMMeshBuilder3_iface);
}

static void clean_mesh_builder_data(struct d3drm_mesh_builder *mesh_builder)
{
    DWORD i;

    HeapFree(GetProcessHeap(), 0, mesh_builder->name);
    mesh_builder->name = NULL;
    HeapFree(GetProcessHeap(), 0, mesh_builder->pVertices);
    mesh_builder->pVertices = NULL;
    mesh_builder->nb_vertices = 0;
    HeapFree(GetProcessHeap(), 0, mesh_builder->pNormals);
    mesh_builder->pNormals = NULL;
    mesh_builder->nb_normals = 0;
    HeapFree(GetProcessHeap(), 0, mesh_builder->pFaceData);
    mesh_builder->pFaceData = NULL;
    mesh_builder->face_data_size = 0;
    mesh_builder->nb_faces = 0;
    HeapFree(GetProcessHeap(), 0, mesh_builder->pCoords2d);
    mesh_builder->pCoords2d = NULL;
    mesh_builder->nb_coords2d = 0;
    for (i = 0; i < mesh_builder->nb_materials; i++)
    {
        if (mesh_builder->materials[i].material)
            IDirect3DRMMaterial2_Release(mesh_builder->materials[i].material);
        if (mesh_builder->materials[i].texture)
            IDirect3DRMTexture3_Release(mesh_builder->materials[i].texture);
    }
    mesh_builder->nb_materials = 0;
    HeapFree(GetProcessHeap(), 0, mesh_builder->materials);
    HeapFree(GetProcessHeap(), 0, mesh_builder->material_indices);
}

static HRESULT WINAPI d3drm_mesh_builder2_QueryInterface(IDirect3DRMMeshBuilder2 *iface, REFIID riid, void **out)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder2)
            || IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &mesh_builder->IDirect3DRMMeshBuilder2_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder3))
    {
        *out = &mesh_builder->IDirect3DRMMeshBuilder3_iface;
    }
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI d3drm_mesh_builder2_AddRef(IDirect3DRMMeshBuilder2 *iface)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);
    ULONG refcount = InterlockedIncrement(&mesh_builder->ref);

    TRACE("%p increasing refcount to %u.\n", mesh_builder, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_mesh_builder2_Release(IDirect3DRMMeshBuilder2 *iface)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);
    ULONG refcount = InterlockedDecrement(&mesh_builder->ref);

    TRACE("%p decreasing refcount to %u.\n", mesh_builder, refcount);

    if (!refcount)
    {
        clean_mesh_builder_data(mesh_builder);
        if (mesh_builder->material)
            IDirect3DRMMaterial2_Release(mesh_builder->material);
        if (mesh_builder->texture)
            IDirect3DRMTexture3_Release(mesh_builder->texture);
        HeapFree(GetProcessHeap(), 0, mesh_builder);
    }

    return refcount;
}

static HRESULT WINAPI d3drm_mesh_builder2_Clone(IDirect3DRMMeshBuilder2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_AddDestroyCallback(IDirect3DRMMeshBuilder2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_DeleteDestroyCallback(IDirect3DRMMeshBuilder2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetAppData(IDirect3DRMMeshBuilder2 *iface, DWORD data)
{
    FIXME("iface %p, data %#x stub!\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_mesh_builder2_GetAppData(IDirect3DRMMeshBuilder2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetName(IDirect3DRMMeshBuilder2 *iface, const char *name)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return IDirect3DRMMeshBuilder3_SetName(&mesh_builder->IDirect3DRMMeshBuilder3_iface, name);
}

static HRESULT WINAPI d3drm_mesh_builder2_GetName(IDirect3DRMMeshBuilder2 *iface, DWORD *size, char *name)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMMeshBuilder3_GetName(&mesh_builder->IDirect3DRMMeshBuilder3_iface, size, name);
}

static HRESULT WINAPI d3drm_mesh_builder2_GetClassName(IDirect3DRMMeshBuilder2 *iface, DWORD *size, char *name)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMMeshBuilder3_GetClassName(&mesh_builder->IDirect3DRMMeshBuilder3_iface, size, name);
}

static HRESULT WINAPI d3drm_mesh_builder2_Load(IDirect3DRMMeshBuilder2 *iface, void *filename,
        void *name, D3DRMLOADOPTIONS flags, D3DRMLOADTEXTURECALLBACK cb, void *ctx)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, filename %p, name %p, flags %#x, cb %p, ctx %p.\n",
            iface, filename, name, flags, cb, ctx);

    if (cb)
        FIXME("Texture callback is not yet supported\n");

    return IDirect3DRMMeshBuilder3_Load(&mesh_builder->IDirect3DRMMeshBuilder3_iface,
            filename, name, flags, NULL, ctx);
}

static HRESULT WINAPI d3drm_mesh_builder2_Save(IDirect3DRMMeshBuilder2 *iface,
        const char *filename, D3DRMXOFFORMAT format, D3DRMSAVEOPTIONS flags)
{
    FIXME("iface %p, filename %s, format %#x, flags %#x stub!\n",
            iface, debugstr_a(filename), format, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_Scale(IDirect3DRMMeshBuilder2 *iface,
        D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, sx %.8e, sy %.8e, sz %.8e.\n", iface, sx, sy, sz);

    return IDirect3DRMMeshBuilder3_Scale(&mesh_builder->IDirect3DRMMeshBuilder3_iface, sx, sy, sz);
}

static HRESULT WINAPI d3drm_mesh_builder2_Translate(IDirect3DRMMeshBuilder2 *iface,
        D3DVALUE tx, D3DVALUE ty, D3DVALUE tz)
{
    FIXME("iface %p, tx %.8e, ty %.8e, tz %.8e stub!\n", iface, tx, ty, tz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetColorSource(IDirect3DRMMeshBuilder2 *iface, D3DRMCOLORSOURCE source)
{
    FIXME("iface %p, source %#x stub!\n", iface, source);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_GetBox(IDirect3DRMMeshBuilder2 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_GenerateNormals(IDirect3DRMMeshBuilder2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static D3DRMCOLORSOURCE WINAPI d3drm_mesh_builder2_GetColorSource(IDirect3DRMMeshBuilder2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_AddMesh(IDirect3DRMMeshBuilder2 *iface, IDirect3DRMMesh *mesh)
{
    FIXME("iface %p, mesh %p stub!\n", iface, mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_AddMeshBuilder(IDirect3DRMMeshBuilder2 *iface,
        IDirect3DRMMeshBuilder *mesh_builder)
{
    FIXME("iface %p, mesh_builder %p stub!\n", iface, mesh_builder);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_AddFrame(IDirect3DRMMeshBuilder2 *iface, IDirect3DRMFrame *frame)
{
    FIXME("iface %p, frame %p stub!\n", iface, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_AddFace(IDirect3DRMMeshBuilder2 *iface, IDirect3DRMFace *face)
{
    FIXME("iface %p, face %p stub!\n", iface, face);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_AddFaces(IDirect3DRMMeshBuilder2 *iface,
        DWORD vertex_count, D3DVECTOR *vertices, DWORD normal_count, D3DVECTOR *normals,
        DWORD *face_data, IDirect3DRMFaceArray **array)
{
    FIXME("iface %p, vertex_count %u, vertices %p, normal_count %u, normals %p, face_data %p, array %p stub!\n",
            iface, vertex_count, vertices, normal_count, normals, face_data, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_ReserveSpace(IDirect3DRMMeshBuilder2 *iface,
        DWORD vertex_count, DWORD normal_count, DWORD face_count)
{
    FIXME("iface %p, vertex_count %u, normal_count %u, face_count %u stub!\n",
            iface, vertex_count, normal_count, face_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetColorRGB(IDirect3DRMMeshBuilder2 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e.\n", iface, red, green, blue);

    return IDirect3DRMMeshBuilder3_SetColorRGB(&mesh_builder->IDirect3DRMMeshBuilder3_iface, red, green, blue);
}

static HRESULT WINAPI d3drm_mesh_builder2_SetColor(IDirect3DRMMeshBuilder2 *iface, D3DCOLOR color)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, color 0x%08x.\n", iface, color);

    return IDirect3DRMMeshBuilder3_SetColor(&mesh_builder->IDirect3DRMMeshBuilder3_iface, color);
}

static HRESULT WINAPI d3drm_mesh_builder2_SetTexture(IDirect3DRMMeshBuilder2 *iface,
        IDirect3DRMTexture *texture)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);
    IDirect3DRMTexture3 *texture3 = NULL;
    HRESULT hr = D3DRM_OK;

    TRACE("iface %p, texture %p.\n", iface, texture);

    if (texture)
        hr = IDirect3DRMTexture_QueryInterface(texture, &IID_IDirect3DRMTexture3, (void **)&texture3);
    if (SUCCEEDED(hr))
        hr = IDirect3DRMMeshBuilder3_SetTexture(&mesh_builder->IDirect3DRMMeshBuilder3_iface, texture3);
    if (texture3)
        IDirect3DRMTexture3_Release(texture3);

    return hr;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetMaterial(IDirect3DRMMeshBuilder2 *iface,
        IDirect3DRMMaterial *material)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, material %p.\n", iface, material);

    return IDirect3DRMMeshBuilder3_SetMaterial(&mesh_builder->IDirect3DRMMeshBuilder3_iface,
            (IDirect3DRMMaterial2 *)material);
}

static HRESULT WINAPI d3drm_mesh_builder2_SetTextureTopology(IDirect3DRMMeshBuilder2 *iface,
        BOOL wrap_u, BOOL wrap_v)
{
    FIXME("iface %p, wrap_u %#x, wrap_v %#x stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetQuality(IDirect3DRMMeshBuilder2 *iface,
        D3DRMRENDERQUALITY quality)
{
    FIXME("iface %p, quality %#x stub!\n", iface, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetPerspective(IDirect3DRMMeshBuilder2 *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetVertex(IDirect3DRMMeshBuilder2 *iface,
        DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, index %u, x %.8e, y %.8e, z %.8e stub!\n", iface, index, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetNormal(IDirect3DRMMeshBuilder2 *iface,
        DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, index %u, x %.8e, y %.8e, z %.8e stub!\n", iface, index, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetTextureCoordinates(IDirect3DRMMeshBuilder2 *iface,
        DWORD index, D3DVALUE u, D3DVALUE v)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, index %u, u %.8e, v %.8e.\n", iface, index, u, v);

    return IDirect3DRMMeshBuilder3_SetTextureCoordinates(&mesh_builder->IDirect3DRMMeshBuilder3_iface,
            index, u, v);
}

static HRESULT WINAPI d3drm_mesh_builder2_SetVertexColor(IDirect3DRMMeshBuilder2 *iface,
        DWORD index, D3DCOLOR color)
{
    FIXME("iface %p, index %u, color 0x%08x stub!\n", iface, index, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_SetVertexColorRGB(IDirect3DRMMeshBuilder2 *iface,
        DWORD index, D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    FIXME("iface %p, index %u, red %.8e, green %.8e, blue %.8e stub!\n",
            iface, index, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_GetFaces(IDirect3DRMMeshBuilder2 *iface,
        IDirect3DRMFaceArray **array)
{
    FIXME("iface %p, array %p stub!\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_GetVertices(IDirect3DRMMeshBuilder2 *iface,
        DWORD *vertex_count, D3DVECTOR *vertices, DWORD *normal_count, D3DVECTOR *normals,
        DWORD *face_data_size, DWORD *face_data)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, vertex_count %p, vertices %p, normal_count %p, normals %p, face_data_size %p, face_data %p.\n",
            iface, vertex_count, vertices, normal_count, normals, face_data_size, face_data);

    if (vertices && (!vertex_count || (*vertex_count < mesh_builder->nb_vertices)))
        return D3DRMERR_BADVALUE;
    if (vertex_count)
        *vertex_count = mesh_builder->nb_vertices;
    if (vertices && mesh_builder->nb_vertices)
        memcpy(vertices, mesh_builder->pVertices, mesh_builder->nb_vertices * sizeof(*vertices));

    if (normals && (!normal_count || (*normal_count < mesh_builder->nb_normals)))
        return D3DRMERR_BADVALUE;
    if (normal_count)
        *normal_count = mesh_builder->nb_normals;
    if (normals && mesh_builder->nb_normals)
        memcpy(normals, mesh_builder->pNormals, mesh_builder->nb_normals * sizeof(*normals));

    if (face_data && (!face_data_size || (*face_data_size < mesh_builder->face_data_size)))
        return D3DRMERR_BADVALUE;
    if (face_data_size)
        *face_data_size = mesh_builder->face_data_size;
    if (face_data && mesh_builder->face_data_size)
        memcpy(face_data, mesh_builder->pFaceData, mesh_builder->face_data_size * sizeof(*face_data));

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder2_GetTextureCoordinates(IDirect3DRMMeshBuilder2 *iface,
        DWORD index, D3DVALUE *u, D3DVALUE *v)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, index %u, u %p, v %p.\n", iface, index, u, v);

    return IDirect3DRMMeshBuilder3_GetTextureCoordinates(&mesh_builder->IDirect3DRMMeshBuilder3_iface,
            index, u, v);
}

static int WINAPI d3drm_mesh_builder2_AddVertex(IDirect3DRMMeshBuilder2 *iface,
        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, x %.8e, y %.8e, z %.8e stub!\n", iface, x, y, z);

    return 0;
}

static int WINAPI d3drm_mesh_builder2_AddNormal(IDirect3DRMMeshBuilder2 *iface,
        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, x %.8e, y %.8e, z %.8e stub!\n", iface, x, y, z);

    return 0;
}

static HRESULT WINAPI d3drm_mesh_builder2_CreateFace(IDirect3DRMMeshBuilder2 *iface, IDirect3DRMFace **face)
{
    TRACE("iface %p, face %p.\n", iface, face);

    return Direct3DRMFace_create(&IID_IDirect3DRMFace, (IUnknown **)face);
}

static D3DRMRENDERQUALITY WINAPI d3drm_mesh_builder2_GetQuality(IDirect3DRMMeshBuilder2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL WINAPI d3drm_mesh_builder2_GetPerspective(IDirect3DRMMeshBuilder2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static int WINAPI d3drm_mesh_builder2_GetFaceCount(IDirect3DRMMeshBuilder2 *iface)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p.\n", iface);

    return mesh_builder->nb_faces;
}

static int WINAPI d3drm_mesh_builder2_GetVertexCount(IDirect3DRMMeshBuilder2 *iface)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p.\n", iface);

    return mesh_builder->nb_vertices;
}

static D3DCOLOR WINAPI d3drm_mesh_builder2_GetVertexColor(IDirect3DRMMeshBuilder2 *iface, DWORD index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return 0;
}

static HRESULT WINAPI d3drm_mesh_builder2_CreateMesh(IDirect3DRMMeshBuilder2 *iface, IDirect3DRMMesh **mesh)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return IDirect3DRMMeshBuilder3_CreateMesh(&mesh_builder->IDirect3DRMMeshBuilder3_iface, mesh);
}

static HRESULT WINAPI d3drm_mesh_builder2_GenerateNormals2(IDirect3DRMMeshBuilder2 *iface,
        D3DVALUE crease, DWORD flags)
{
    FIXME("iface %p, crease %.8e, flags %#x stub!\n", iface, crease, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder2_GetFace(IDirect3DRMMeshBuilder2 *iface,
        DWORD index, IDirect3DRMFace **face)
{
    FIXME("iface %p, index %u, face %p stub!\n", iface, index, face);

    return E_NOTIMPL;
}

static const struct IDirect3DRMMeshBuilder2Vtbl d3drm_mesh_builder2_vtbl =
{
    d3drm_mesh_builder2_QueryInterface,
    d3drm_mesh_builder2_AddRef,
    d3drm_mesh_builder2_Release,
    d3drm_mesh_builder2_Clone,
    d3drm_mesh_builder2_AddDestroyCallback,
    d3drm_mesh_builder2_DeleteDestroyCallback,
    d3drm_mesh_builder2_SetAppData,
    d3drm_mesh_builder2_GetAppData,
    d3drm_mesh_builder2_SetName,
    d3drm_mesh_builder2_GetName,
    d3drm_mesh_builder2_GetClassName,
    d3drm_mesh_builder2_Load,
    d3drm_mesh_builder2_Save,
    d3drm_mesh_builder2_Scale,
    d3drm_mesh_builder2_Translate,
    d3drm_mesh_builder2_SetColorSource,
    d3drm_mesh_builder2_GetBox,
    d3drm_mesh_builder2_GenerateNormals,
    d3drm_mesh_builder2_GetColorSource,
    d3drm_mesh_builder2_AddMesh,
    d3drm_mesh_builder2_AddMeshBuilder,
    d3drm_mesh_builder2_AddFrame,
    d3drm_mesh_builder2_AddFace,
    d3drm_mesh_builder2_AddFaces,
    d3drm_mesh_builder2_ReserveSpace,
    d3drm_mesh_builder2_SetColorRGB,
    d3drm_mesh_builder2_SetColor,
    d3drm_mesh_builder2_SetTexture,
    d3drm_mesh_builder2_SetMaterial,
    d3drm_mesh_builder2_SetTextureTopology,
    d3drm_mesh_builder2_SetQuality,
    d3drm_mesh_builder2_SetPerspective,
    d3drm_mesh_builder2_SetVertex,
    d3drm_mesh_builder2_SetNormal,
    d3drm_mesh_builder2_SetTextureCoordinates,
    d3drm_mesh_builder2_SetVertexColor,
    d3drm_mesh_builder2_SetVertexColorRGB,
    d3drm_mesh_builder2_GetFaces,
    d3drm_mesh_builder2_GetVertices,
    d3drm_mesh_builder2_GetTextureCoordinates,
    d3drm_mesh_builder2_AddVertex,
    d3drm_mesh_builder2_AddNormal,
    d3drm_mesh_builder2_CreateFace,
    d3drm_mesh_builder2_GetQuality,
    d3drm_mesh_builder2_GetPerspective,
    d3drm_mesh_builder2_GetFaceCount,
    d3drm_mesh_builder2_GetVertexCount,
    d3drm_mesh_builder2_GetVertexColor,
    d3drm_mesh_builder2_CreateMesh,
    d3drm_mesh_builder2_GenerateNormals2,
    d3drm_mesh_builder2_GetFace,
};

static HRESULT WINAPI d3drm_mesh_builder3_QueryInterface(IDirect3DRMMeshBuilder3 *iface, REFIID riid, void **out)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return d3drm_mesh_builder2_QueryInterface(&mesh_builder->IDirect3DRMMeshBuilder2_iface, riid, out);
}

static ULONG WINAPI d3drm_mesh_builder3_AddRef(IDirect3DRMMeshBuilder3 *iface)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_mesh_builder2_AddRef(&mesh_builder->IDirect3DRMMeshBuilder2_iface);
}

static ULONG WINAPI d3drm_mesh_builder3_Release(IDirect3DRMMeshBuilder3 *iface)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_mesh_builder2_Release(&mesh_builder->IDirect3DRMMeshBuilder2_iface);
}

static HRESULT WINAPI d3drm_mesh_builder3_Clone(IDirect3DRMMeshBuilder3 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_AddDestroyCallback(IDirect3DRMMeshBuilder3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_DeleteDestroyCallback(IDirect3DRMMeshBuilder3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetAppData(IDirect3DRMMeshBuilder3 *iface, DWORD data)
{
    FIXME("iface %p, data %#x stub!\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_mesh_builder3_GetAppData(IDirect3DRMMeshBuilder3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetName(IDirect3DRMMeshBuilder3 *iface, const char *name)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);
    char *string = NULL;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    if (name)
    {
        string = HeapAlloc(GetProcessHeap(), 0, strlen(name) + 1);
        if (!string) return E_OUTOFMEMORY;
        strcpy(string, name);
    }
    HeapFree(GetProcessHeap(), 0, mesh_builder->name);
    mesh_builder->name = string;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetName(IDirect3DRMMeshBuilder3 *iface,
        DWORD *size, char *name)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    if (!size)
        return E_POINTER;

    if (!mesh_builder->name)
    {
        *size = 0;
        return D3DRM_OK;
    }

    if (*size < (strlen(mesh_builder->name) + 1))
        return E_INVALIDARG;

    strcpy(name, mesh_builder->name);
    *size = strlen(mesh_builder->name) + 1;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetClassName(IDirect3DRMMeshBuilder3 *iface,
        DWORD *size, char *name)
{
    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    if (!size || *size < strlen("Builder") || !name)
        return E_INVALIDARG;

    strcpy(name, "Builder");
    *size = sizeof("Builder");

    return D3DRM_OK;
}

HRESULT load_mesh_data(IDirect3DRMMeshBuilder3 *iface, IDirectXFileData *pData,
        D3DRMLOADTEXTURECALLBACK load_texture_proc, void *arg)
{
    struct d3drm_mesh_builder *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    IDirectXFileData *pData2 = NULL;
    const GUID* guid;
    DWORD size;
    BYTE *ptr;
    HRESULT hr;
    HRESULT ret = D3DRMERR_BADOBJECT;
    DWORD* faces_vertex_idx_data = NULL;
    DWORD* faces_vertex_idx_ptr;
    DWORD faces_vertex_idx_size;
    DWORD* faces_normal_idx_data = NULL;
    DWORD* faces_normal_idx_ptr = NULL;
    DWORD* faces_data_ptr;
    DWORD faces_data_size = 0;
    DWORD i;

    TRACE("(%p)->(%p)\n", This, pData);

    hr = IDirectXFileData_GetName(pData, NULL, &size);
    if (hr != DXFILE_OK)
        return hr;
    if (size)
    {
        This->name = HeapAlloc(GetProcessHeap(), 0, size);
        if (!This->name)
            return E_OUTOFMEMORY;

        hr = IDirectXFileData_GetName(pData, This->name, &size);
        if (hr != DXFILE_OK)
            return hr;
    }

    TRACE("Mesh name is '%s'\n", This->name ? This->name : "");

    This->nb_normals = 0;

    hr = IDirectXFileData_GetData(pData, NULL, &size, (void**)&ptr);
    if (hr != DXFILE_OK)
        goto end;

    This->nb_vertices = *(DWORD*)ptr;
    This->nb_faces = *(DWORD*)(ptr + sizeof(DWORD) + This->nb_vertices * sizeof(D3DVECTOR));
    faces_vertex_idx_size = size - sizeof(DWORD) - This->nb_vertices * sizeof(D3DVECTOR) - sizeof(DWORD);

    TRACE("Mesh: nb_vertices = %d, nb_faces = %d, faces_vertex_idx_size = %d\n", This->nb_vertices, This->nb_faces, faces_vertex_idx_size);

    This->pVertices = HeapAlloc(GetProcessHeap(), 0, This->nb_vertices * sizeof(D3DVECTOR));
    memcpy(This->pVertices, ptr + sizeof(DWORD), This->nb_vertices * sizeof(D3DVECTOR));

    faces_vertex_idx_ptr = faces_vertex_idx_data = HeapAlloc(GetProcessHeap(), 0, faces_vertex_idx_size);
    memcpy(faces_vertex_idx_data, ptr + sizeof(DWORD) + This->nb_vertices * sizeof(D3DVECTOR) + sizeof(DWORD), faces_vertex_idx_size);

    /* Each vertex index will have its normal index counterpart so just allocate twice the size */
    This->pFaceData = HeapAlloc(GetProcessHeap(), 0, faces_vertex_idx_size * 2);
    faces_data_ptr = (DWORD*)This->pFaceData;

    while (1)
    {
        IDirectXFileObject *object;

        hr =  IDirectXFileData_GetNextObject(pData, &object);
        if (hr == DXFILEERR_NOMOREOBJECTS)
        {
            TRACE("No more object\n");
            break;
        }
        if (hr != DXFILE_OK)
           goto end;

        hr = IDirectXFileObject_QueryInterface(object, &IID_IDirectXFileData, (void**)&pData2);
        IDirectXFileObject_Release(object);
        if (hr != DXFILE_OK)
            goto end;

        hr = IDirectXFileData_GetType(pData2, &guid);
        if (hr != DXFILE_OK)
            goto end;

        TRACE("Found object type whose GUID = %s\n", debugstr_guid(guid));

        if (IsEqualGUID(guid, &TID_D3DRMMeshNormals))
        {
            DWORD nb_faces_normals;
            DWORD faces_normal_idx_size;

            hr = IDirectXFileData_GetData(pData2, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            This->nb_normals = *(DWORD*)ptr;
            nb_faces_normals = *(DWORD*)(ptr + sizeof(DWORD) + This->nb_normals * sizeof(D3DVECTOR));

            TRACE("MeshNormals: nb_normals = %d, nb_faces_normals = %d\n", This->nb_normals, nb_faces_normals);
            if (nb_faces_normals != This->nb_faces)
                WARN("nb_face_normals (%d) != nb_faces (%d)\n", nb_faces_normals, This->nb_normals);

            This->pNormals = HeapAlloc(GetProcessHeap(), 0, This->nb_normals * sizeof(D3DVECTOR));
            memcpy(This->pNormals, ptr + sizeof(DWORD), This->nb_normals * sizeof(D3DVECTOR));

            faces_normal_idx_size = size - (2 * sizeof(DWORD) + This->nb_normals * sizeof(D3DVECTOR));
            faces_normal_idx_ptr = faces_normal_idx_data = HeapAlloc(GetProcessHeap(), 0, faces_normal_idx_size);
            memcpy(faces_normal_idx_data, ptr + sizeof(DWORD) + This->nb_normals * sizeof(D3DVECTOR) + sizeof(DWORD), faces_normal_idx_size);
        }
        else if (IsEqualGUID(guid, &TID_D3DRMMeshTextureCoords))
        {
            hr = IDirectXFileData_GetData(pData2, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            This->nb_coords2d = *(DWORD*)ptr;

            TRACE("MeshTextureCoords: nb_coords2d = %d\n", This->nb_coords2d);

            This->pCoords2d = HeapAlloc(GetProcessHeap(), 0, This->nb_coords2d * sizeof(*This->pCoords2d));
            memcpy(This->pCoords2d, ptr + sizeof(DWORD), This->nb_coords2d * sizeof(*This->pCoords2d));

        }
        else if (IsEqualGUID(guid, &TID_D3DRMMeshMaterialList))
        {
            DWORD nb_materials;
            DWORD nb_face_indices;
            DWORD data_size;
            IDirectXFileObject *child;
            DWORD i = 0;
            float* values;

            TRACE("Process MeshMaterialList\n");

            hr = IDirectXFileData_GetData(pData2, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            nb_materials = *(DWORD*)ptr;
            nb_face_indices = *(DWORD*)(ptr + sizeof(DWORD));
            data_size = 2 * sizeof(DWORD) + nb_face_indices * sizeof(DWORD);

            TRACE("nMaterials = %u, nFaceIndexes = %u\n", nb_materials, nb_face_indices);

            if (size != data_size)
                WARN("Returned size %u does not match expected one %u\n", size, data_size);

            This->material_indices = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->material_indices) * nb_face_indices);
            if (!This->material_indices)
                goto end;
            memcpy(This->material_indices, ptr + 2 * sizeof(DWORD), sizeof(*This->material_indices) * nb_face_indices),

            This->materials = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->materials) * nb_materials);
            if (!This->materials)
            {
                HeapFree(GetProcessHeap(), 0, This->material_indices);
                goto end;
            }
            This->nb_materials = nb_materials;

            while (SUCCEEDED(hr = IDirectXFileData_GetNextObject(pData2, &child)) && (i < nb_materials))
            {
                IDirectXFileData *data;
                IDirectXFileDataReference *reference;
                IDirectXFileObject *material_child;

                hr = IDirectXFileObject_QueryInterface(child, &IID_IDirectXFileData, (void **)&data);
                if (FAILED(hr))
                {
                    hr = IDirectXFileObject_QueryInterface(child, &IID_IDirectXFileDataReference, (void **)&reference);
                    IDirectXFileObject_Release(child);
                    if (FAILED(hr))
                        goto end;

                    hr = IDirectXFileDataReference_Resolve(reference, &data);
                    IDirectXFileDataReference_Release(reference);
                    if (FAILED(hr))
                        goto end;
                }
                else
                {
                    IDirectXFileObject_Release(child);
                }

                hr = Direct3DRMMaterial_create(&This->materials[i].material);
                if (FAILED(hr))
                {
                    IDirectXFileData_Release(data);
                    goto end;
                }

                hr = IDirectXFileData_GetData(data, NULL, &size, (void**)&ptr);
                if (hr != DXFILE_OK)
                {
                    IDirectXFileData_Release(data);
                    goto end;
                }

                if (size != 44)
                    WARN("Material size %u does not match expected one %u\n", size, 44);

                values = (float*)ptr;

                This->materials[i].color = RGBA_MAKE((BYTE)(values[0] * 255.0f), (BYTE)(values[1] * 255.0f),
                        (BYTE)(values[2] * 255.0f), (BYTE)(values[3] * 255.0f));

                IDirect3DRMMaterial2_SetAmbient(This->materials[i].material, values[0], values [1], values[2]); /* Alpha ignored */
                IDirect3DRMMaterial2_SetPower(This->materials[i].material, values[4]);
                IDirect3DRMMaterial2_SetSpecular(This->materials[i].material, values[5], values[6], values[7]);
                IDirect3DRMMaterial2_SetEmissive(This->materials[i].material, values[8], values[9], values[10]);

                This->materials[i].texture = NULL;

                hr = IDirectXFileData_GetNextObject(data, &material_child);
                if (hr == S_OK)
                {
                    IDirectXFileData *data;
                    char **filename;

                    hr = IDirectXFileObject_QueryInterface(material_child, &IID_IDirectXFileData, (void **)&data);
                    if (FAILED(hr))
                    {
                        IDirectXFileDataReference *reference;

                        hr = IDirectXFileObject_QueryInterface(material_child, &IID_IDirectXFileDataReference, (void **)&reference);
                        if (FAILED(hr))
                            goto end;

                        hr = IDirectXFileDataReference_Resolve(reference, &data);
                        IDirectXFileDataReference_Release(reference);
                        if (FAILED(hr))
                            goto end;
                    }

                    hr = IDirectXFileData_GetType(data, &guid);
                    if (hr != DXFILE_OK)
                        goto end;
                    if (!IsEqualGUID(guid, &TID_D3DRMTextureFilename))
                    {
                         WARN("Not a texture filename\n");
                         goto end;
                    }

                    size = 4;
                    hr = IDirectXFileData_GetData(data, NULL, &size, (void**)&filename);
                    if (SUCCEEDED(hr))
                    {
                        if (load_texture_proc)
                        {
                            IDirect3DRMTexture *texture;

                            hr = load_texture_proc(*filename, arg, &texture);
                            if (SUCCEEDED(hr))
                            {
                                hr = IDirect3DTexture_QueryInterface(texture, &IID_IDirect3DRMTexture3, (void**)&This->materials[i].texture);
                                IDirect3DTexture_Release(texture);
                            }
                        }
                        else
                        {
                            HANDLE file;

                            /* If the texture file is not found, no texture is associated with the material */
                            file = CreateFileA(*filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                            if (file != INVALID_HANDLE_VALUE)
                            {
                                CloseHandle(file);

                                hr = Direct3DRMTexture_create(&IID_IDirect3DRMTexture3, (IUnknown**)&This->materials[i].texture);
                                if (FAILED(hr))
                                {
                                    IDirectXFileData_Release(data);
                                    goto end;
                                }
                            }
                        }
                    }
                }
                else if (hr != DXFILEERR_NOMOREOBJECTS)
                {
                    goto end;
                }
                hr = S_OK;

                IDirectXFileData_Release(data);
                i++;
            }
            if (hr == S_OK)
            {
                IDirectXFileObject_Release(child);
                WARN("Found more sub-objects than expected\n");
            }
            else if (hr != DXFILEERR_NOMOREOBJECTS)
            {
                goto end;
            }
            hr = S_OK;
        }
        else
        {
            FIXME("Unknown GUID %s, ignoring...\n", debugstr_guid(guid));
        }

        IDirectXFileData_Release(pData2);
        pData2 = NULL;
    }

    if (!This->nb_normals)
    {
        /* Allocate normals, one per vertex */
        This->pNormals = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->nb_vertices * sizeof(D3DVECTOR));
        if (!This->pNormals)
            goto end;
    }

    for (i = 0; i < This->nb_faces; i++)
    {
        DWORD j;
        DWORD nb_face_indexes;
        D3DVECTOR face_normal;

        if (faces_vertex_idx_size < sizeof(DWORD))
            WARN("Not enough data to read number of indices of face %d\n", i);

        nb_face_indexes  = *(faces_data_ptr + faces_data_size++) = *(faces_vertex_idx_ptr++);
        faces_vertex_idx_size--;
        if (faces_normal_idx_data && (*(faces_normal_idx_ptr++) != nb_face_indexes))
            WARN("Faces indices number mismatch\n");

        if (faces_vertex_idx_size < (nb_face_indexes * sizeof(DWORD)))
            WARN("Not enough data to read all indices of face %d\n", i);

        if (!This->nb_normals)
        {
            /* Compute face normal */
            if (nb_face_indexes > 2)
            {
                D3DVECTOR a, b;

                D3DRMVectorSubtract(&a, &This->pVertices[faces_vertex_idx_ptr[2]], &This->pVertices[faces_vertex_idx_ptr[1]]);
                D3DRMVectorSubtract(&b, &This->pVertices[faces_vertex_idx_ptr[0]], &This->pVertices[faces_vertex_idx_ptr[1]]);
                D3DRMVectorCrossProduct(&face_normal, &a, &b);
                D3DRMVectorNormalize(&face_normal);
            }
            else
            {
                face_normal.u1.x = 0.0f;
                face_normal.u2.y = 0.0f;
                face_normal.u3.z = 0.0f;
            }
        }

        for (j = 0; j < nb_face_indexes; j++)
        {
            /* Copy vertex index */
            *(faces_data_ptr + faces_data_size++) = *faces_vertex_idx_ptr;
            /* Copy normal index */
            if (This->nb_normals)
            {
                /* Read from x file */
                *(faces_data_ptr + faces_data_size++) = *(faces_normal_idx_ptr++);
            }
            else
            {
                DWORD vertex_idx = *faces_vertex_idx_ptr;
                if (vertex_idx >= This->nb_vertices)
                {
                    WARN("Found vertex index %u but only %u vertices available => use index 0\n", vertex_idx, This->nb_vertices);
                    vertex_idx = 0;
                }
                *(faces_data_ptr + faces_data_size++) = vertex_idx;
                /* Add face normal to vertex normal */
                D3DRMVectorAdd(&This->pNormals[vertex_idx], &This->pNormals[vertex_idx], &face_normal);
            }
            faces_vertex_idx_ptr++;
        }
        faces_vertex_idx_size -= nb_face_indexes;
    }

    /* Last DWORD must be 0 */
    *(faces_data_ptr + faces_data_size++) = 0;

    /* Set size (in number of DWORD) of all faces data */
    This->face_data_size = faces_data_size;

    if (!This->nb_normals)
    {
        /* Normalize all normals */
        for (i = 0; i < This->nb_vertices; i++)
        {
            D3DRMVectorNormalize(&This->pNormals[i]);
        }
        This->nb_normals = This->nb_vertices;
    }

    /* If there is no texture coordinates, generate default texture coordinates (0.0f, 0.0f) for each vertex */
    if (!This->pCoords2d)
    {
        This->nb_coords2d = This->nb_vertices;
        This->pCoords2d = HeapAlloc(GetProcessHeap(), 0, This->nb_coords2d * sizeof(*This->pCoords2d));
        for (i = 0; i < This->nb_coords2d; i++)
        {
            This->pCoords2d[i].u = 0.0f;
            This->pCoords2d[i].v = 0.0f;
        }
    }

    TRACE("Mesh data loaded successfully\n");

    ret = D3DRM_OK;

end:

    HeapFree(GetProcessHeap(), 0, faces_normal_idx_data);
    HeapFree(GetProcessHeap(), 0, faces_vertex_idx_data);

    return ret;
}

static HRESULT WINAPI d3drm_mesh_builder3_Load(IDirect3DRMMeshBuilder3 *iface, void *filename,
        void *name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURE3CALLBACK cb, void *arg)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);
    DXFILELOADOPTIONS load_options;
    IDirectXFile *dxfile = NULL;
    IDirectXFileEnumObject *enum_object = NULL;
    IDirectXFileData *data = NULL;
    const GUID* guid;
    DWORD size;
    struct d3drm_file_header *header;
    HRESULT hr;
    HRESULT ret = D3DRMERR_BADOBJECT;

    TRACE("iface %p, filename %p, name %p, loadflags %#x, cb %p, arg %p.\n",
            iface, filename, name, loadflags, cb, arg);

    clean_mesh_builder_data(mesh_builder);

    if (loadflags == D3DRMLOAD_FROMMEMORY)
    {
        load_options = DXFILELOAD_FROMMEMORY;
    }
    else if (loadflags == D3DRMLOAD_FROMFILE)
    {
        load_options = DXFILELOAD_FROMFILE;
        TRACE("Loading from file %s\n", debugstr_a(filename));
    }
    else
    {
        FIXME("Load options %d not supported yet\n", loadflags);
        return E_NOTIMPL;
    }

    hr = DirectXFileCreate(&dxfile);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFile_RegisterTemplates(dxfile, templates, strlen(templates));
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFile_CreateEnumObject(dxfile, filename, load_options, &enum_object);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFileEnumObject_GetNextDataObject(enum_object, &data);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFileData_GetType(data, &guid);
    if (hr != DXFILE_OK)
        goto end;

    TRACE("Found object type whose GUID = %s\n", debugstr_guid(guid));

    if (!IsEqualGUID(guid, &TID_DXFILEHeader))
    {
        ret = D3DRMERR_BADFILE;
        goto end;
    }

    hr = IDirectXFileData_GetData(data, NULL, &size, (void**)&header);
    if ((hr != DXFILE_OK) || (size != sizeof(*header)))
        goto end;

    TRACE("Version is %u.%u, flags %#x.\n", header->major, header->minor, header->flags);

    /* Version must be 1.0.x */
    if ((header->major != 1) || (header->minor != 0))
    {
        ret = D3DRMERR_BADFILE;
        goto end;
    }

    IDirectXFileData_Release(data);
    data = NULL;

    hr = IDirectXFileEnumObject_GetNextDataObject(enum_object, &data);
    if (hr != DXFILE_OK)
    {
        ret = D3DRMERR_NOTFOUND;
        goto end;
    }

    hr = IDirectXFileData_GetType(data, &guid);
    if (hr != DXFILE_OK)
        goto end;

    TRACE("Found object type whose GUID = %s\n", debugstr_guid(guid));

    if (!IsEqualGUID(guid, &TID_D3DRMMesh))
    {
        ret = D3DRMERR_NOTFOUND;
        goto end;
    }

    /* We don't care about the texture interface version since we rely on QueryInterface */
    hr = load_mesh_data(iface, data, (D3DRMLOADTEXTURECALLBACK)cb, arg);
    if (hr == S_OK)
        ret = D3DRM_OK;

end:

    if (data)
        IDirectXFileData_Release(data);
    if (enum_object)
        IDirectXFileEnumObject_Release(enum_object);
    if (dxfile)
        IDirectXFile_Release(dxfile);

    if (ret != D3DRM_OK)
        clean_mesh_builder_data(mesh_builder);

    return ret;
}

static HRESULT WINAPI d3drm_mesh_builder3_Save(IDirect3DRMMeshBuilder3 *iface,
        const char *filename, D3DRMXOFFORMAT format, D3DRMSAVEOPTIONS flags)
{
    FIXME("iface %p, filename %s, format %#x, flags %#x stub!\n",
            iface, debugstr_a(filename), format, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_Scale(IDirect3DRMMeshBuilder3 *iface,
        D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);
    DWORD i;

    TRACE("iface %p, sx %.8e, sy %.8e, sz %.8e.\n", iface, sx, sy, sz);

    for (i = 0; i < mesh_builder->nb_vertices; ++i)
    {
        mesh_builder->pVertices[i].u1.x *= sx;
        mesh_builder->pVertices[i].u2.y *= sy;
        mesh_builder->pVertices[i].u3.z *= sz;
    }

    /* Normals are not affected by Scale */

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_Translate(IDirect3DRMMeshBuilder3 *iface,
        D3DVALUE tx, D3DVALUE ty, D3DVALUE tz)
{
    FIXME("iface %p, tx %.8e, ty %.8e, tz %.8e stub!\n", iface, tx, ty, tz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetColorSource(IDirect3DRMMeshBuilder3 *iface,
        D3DRMCOLORSOURCE source)
{
    FIXME("iface %p, source %#x stub!\n", iface, source);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetBox(IDirect3DRMMeshBuilder3 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GenerateNormals(IDirect3DRMMeshBuilder3 *iface,
        D3DVALUE crease, DWORD flags)
{
    FIXME("iface %p, crease %.8e, flags %#x stub!\n", iface, crease, flags);

    return E_NOTIMPL;
}

static D3DRMCOLORSOURCE WINAPI d3drm_mesh_builder3_GetColorSource(IDirect3DRMMeshBuilder3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_AddMesh(IDirect3DRMMeshBuilder3 *iface, IDirect3DRMMesh *mesh)
{
    FIXME("iface %p, mesh %p stub!\n", iface, mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_AddMeshBuilder(IDirect3DRMMeshBuilder3 *iface,
        IDirect3DRMMeshBuilder3 *mesh_builder, DWORD flags)
{
    FIXME("iface %p, mesh_builder %p, flags %#x stub!\n", iface, mesh_builder, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_AddFrame(IDirect3DRMMeshBuilder3 *iface, IDirect3DRMFrame3 *frame)
{
    FIXME("iface %p, frame %p stub!\n", iface, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_AddFace(IDirect3DRMMeshBuilder3 *iface, IDirect3DRMFace2 *face)
{
    FIXME("iface %p, face %p stub!\n", iface, face);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_AddFaces(IDirect3DRMMeshBuilder3 *iface,
        DWORD vertex_count, D3DVECTOR *vertices, DWORD normal_count, D3DVECTOR *normals,
        DWORD *face_data, IDirect3DRMFaceArray **array)
{
    FIXME("iface %p, vertex_count %u, vertices %p, normal_count %u, normals %p, face_data %p array %p stub!\n",
            iface, vertex_count, vertices, normal_count, normals, face_data, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_ReserveSpace(IDirect3DRMMeshBuilder3 *iface,
        DWORD vertex_count, DWORD normal_count, DWORD face_count)
{
    FIXME("iface %p, vertex_count %u, normal_count %u, face_count %u stub!\n",
            iface, vertex_count, normal_count, face_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetColorRGB(IDirect3DRMMeshBuilder3 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e.\n", iface, red, green, blue);

    mesh_builder->color = RGBA_MAKE((BYTE)(red * 255.0f), (BYTE)(green * 255.0f), (BYTE)(blue * 255.0f), 0xff);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetColor(IDirect3DRMMeshBuilder3 *iface, D3DCOLOR color)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p, color 0x%08x.\n", iface, color);

    mesh_builder->color = color;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetTexture(IDirect3DRMMeshBuilder3 *iface,
        IDirect3DRMTexture3 *texture)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p, texture %p.\n", iface, texture);

    if (texture)
        IDirect3DRMTexture3_AddRef(texture);
    if (mesh_builder->texture)
        IDirect3DRMTexture3_Release(mesh_builder->texture);
    mesh_builder->texture = texture;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetMaterial(IDirect3DRMMeshBuilder3 *iface,
        IDirect3DRMMaterial2 *material)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p, material %p.\n", iface, material);

    if (material)
        IDirect3DRMTexture2_AddRef(material);
    if (mesh_builder->material)
        IDirect3DRMTexture2_Release(mesh_builder->material);
    mesh_builder->material = material;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetTextureTopology(IDirect3DRMMeshBuilder3 *iface,
        BOOL wrap_u, BOOL wrap_v)
{
    FIXME("iface %p, wrap_u %#x, wrap_v %#x stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetQuality(IDirect3DRMMeshBuilder3 *iface,
        D3DRMRENDERQUALITY quality)
{
    FIXME("iface %p, quality %#x stub!\n", iface, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetPerspective(IDirect3DRMMeshBuilder3 *iface,
        BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetVertex(IDirect3DRMMeshBuilder3 *iface,
        DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, index %u, x %.8e, y %.8e, z %.8e stub!\n", iface, index, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetNormal(IDirect3DRMMeshBuilder3 *iface,
        DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, index %u, x %.8e, y %.8e, z %.8e stub!\n", iface, index, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetTextureCoordinates(IDirect3DRMMeshBuilder3 *iface,
        DWORD index, D3DVALUE u, D3DVALUE v)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p, index %u, u %.8e, v %.8e.\n", iface, index, u, v);

    if (index >= mesh_builder->nb_coords2d)
        return D3DRMERR_BADVALUE;

    mesh_builder->pCoords2d[index].u = u;
    mesh_builder->pCoords2d[index].v = v;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetVertexColor(IDirect3DRMMeshBuilder3 *iface,
        DWORD index, D3DCOLOR color)
{
    FIXME("iface %p, index %u, color 0x%08x stub!\n", iface, index, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetVertexColorRGB(IDirect3DRMMeshBuilder3 *iface,
        DWORD index, D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    FIXME("iface %p, index %u, red %.8e, green %.8e, blue %.8e stub!\n",
            iface, index, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetFaces(IDirect3DRMMeshBuilder3 *iface,
        IDirect3DRMFaceArray **array)
{
    FIXME("iface %p, array %p stub!\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetGeometry(IDirect3DRMMeshBuilder3 *iface,
        DWORD *vertex_count, D3DVECTOR *vertices, DWORD *normal_count, D3DVECTOR *normals,
        DWORD *face_data_size, DWORD *face_data)
{
    FIXME("iface %p, vertex_count %p, vertices %p, normal_count %p, normals %p, "
            "face_data_size %p, face_data %p stub!\n",
            iface, vertex_count, vertices, normal_count, normals, face_data_size, face_data);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetTextureCoordinates(IDirect3DRMMeshBuilder3 *iface,
        DWORD index, D3DVALUE *u, D3DVALUE *v)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p, index %u, u %p, v %p.\n", iface, index, u, v);

    if (index >= mesh_builder->nb_coords2d)
        return D3DRMERR_BADVALUE;

    *u = mesh_builder->pCoords2d[index].u;
    *v = mesh_builder->pCoords2d[index].v;

    return D3DRM_OK;
}

static int WINAPI d3drm_mesh_builder3_AddVertex(IDirect3DRMMeshBuilder3 *iface,
        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, x %.8e, y %.8e, z %.8e stub!\n", iface, x, y, z);

    return 0;
}

static int WINAPI d3drm_mesh_builder3_AddNormal(IDirect3DRMMeshBuilder3 *iface,
        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, x %.8e, y %.8e, z %.8e stub!\n", iface, x, y, z);

    return 0;
}

static HRESULT WINAPI d3drm_mesh_builder3_CreateFace(IDirect3DRMMeshBuilder3 *iface, IDirect3DRMFace2 **face)
{
    TRACE("iface %p, face %p.\n", iface, face);

    return Direct3DRMFace_create(&IID_IDirect3DRMFace2, (IUnknown **)face);
}

static D3DRMRENDERQUALITY WINAPI d3drm_mesh_builder3_GetQuality(IDirect3DRMMeshBuilder3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL WINAPI d3drm_mesh_builder3_GetPerspective(IDirect3DRMMeshBuilder3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static int WINAPI d3drm_mesh_builder3_GetFaceCount(IDirect3DRMMeshBuilder3 *iface)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p.\n", iface);

    return mesh_builder->nb_faces;
}

static int WINAPI d3drm_mesh_builder3_GetVertexCount(IDirect3DRMMeshBuilder3 *iface)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p.\n", iface);

    return mesh_builder->nb_vertices;
}

static D3DCOLOR WINAPI d3drm_mesh_builder3_GetVertexColor(IDirect3DRMMeshBuilder3 *iface,
        DWORD index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return 0;
}

static HRESULT WINAPI d3drm_mesh_builder3_CreateMesh(IDirect3DRMMeshBuilder3 *iface, IDirect3DRMMesh **mesh)
{
    struct d3drm_mesh_builder *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    HRESULT hr;
    D3DRMGROUPINDEX group;

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    if (!mesh)
        return E_POINTER;

    hr = Direct3DRMMesh_create(mesh);
    if (FAILED(hr))
        return hr;

    /* If there is mesh data, create a group and put data inside */
    if (This->nb_vertices)
    {
        DWORD i, j;
        int k;
        D3DRMVERTEX* vertices;

        vertices = HeapAlloc(GetProcessHeap(), 0, This->nb_vertices * sizeof(D3DRMVERTEX));
        if (!vertices)
        {
            IDirect3DRMMesh_Release(*mesh);
            return E_OUTOFMEMORY;
        }
        for (i = 0; i < This->nb_vertices; i++)
            vertices[i].position = This->pVertices[i];
        hr = IDirect3DRMMesh_SetVertices(*mesh, 0, 0, This->nb_vertices, vertices);
        HeapFree(GetProcessHeap(), 0, vertices);

        /* Groups are in reverse order compared to materials list in X file */
        for (k = This->nb_materials - 1; k >= 0; k--)
        {
            unsigned* face_data;
            unsigned* out_ptr;
            DWORD* in_ptr = This->pFaceData;
            ULONG vertex_per_face = 0;
            BOOL* used_vertices;
            unsigned nb_vertices = 0;
            unsigned nb_faces = 0;

            used_vertices = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->face_data_size * sizeof(*used_vertices));
            if (!used_vertices)
            {
                IDirect3DRMMesh_Release(*mesh);
                return E_OUTOFMEMORY;
            }

            face_data = HeapAlloc(GetProcessHeap(), 0, This->face_data_size * sizeof(*face_data));
            if (!face_data)
            {
                HeapFree(GetProcessHeap(), 0, used_vertices);
                IDirect3DRMMesh_Release(*mesh);
                return E_OUTOFMEMORY;
            }
            out_ptr = face_data;

            /* If all faces have the same number of vertex, set vertex_per_face */
            for (i = 0; i < This->nb_faces; i++)
            {
                /* Process only faces belonging to the group */
                if (This->material_indices[i] == k)
                {
                    if (vertex_per_face && (vertex_per_face != *in_ptr))
                        break;
                    vertex_per_face = *in_ptr;
                }
                in_ptr += 1 + *in_ptr * 2;
            }
            if (i != This->nb_faces)
                vertex_per_face = 0;

            /* Put only vertex indices */
            in_ptr = This->pFaceData;
            for (i = 0; i < This->nb_faces; i++)
            {
                DWORD nb_indices = *in_ptr++;

                /* Skip faces not belonging to the group */
                if (This->material_indices[i] != k)
                {
                    in_ptr += 2 * nb_indices;
                    continue;
                }

                /* Don't put nb indices when vertex_per_face is set */
                if (vertex_per_face)
                    *out_ptr++ = nb_indices;

                for (j = 0; j < nb_indices; j++)
                {
                    *out_ptr = *in_ptr++;
                    used_vertices[*out_ptr++] = TRUE;
                    /* Skip normal index */
                    in_ptr++;
                }

                nb_faces++;
            }

            for (i = 0; i < This->nb_vertices; i++)
                if (used_vertices[i])
                    nb_vertices++;

            hr = IDirect3DRMMesh_AddGroup(*mesh, nb_vertices, nb_faces, vertex_per_face, face_data, &group);
            HeapFree(GetProcessHeap(), 0, used_vertices);
            HeapFree(GetProcessHeap(), 0, face_data);
            if (SUCCEEDED(hr))
                hr = IDirect3DRMMesh_SetGroupColor(*mesh, group, This->materials[k].color);
            if (SUCCEEDED(hr))
                hr = IDirect3DRMMesh_SetGroupMaterial(*mesh, group,
                        (IDirect3DRMMaterial *)This->materials[k].material);
            if (SUCCEEDED(hr) && This->materials[k].texture)
            {
                IDirect3DRMTexture *texture;

                IDirect3DRMTexture3_QueryInterface(This->materials[k].texture,
                        &IID_IDirect3DRMTexture, (void **)&texture);
                hr = IDirect3DRMMesh_SetGroupTexture(*mesh, group, texture);
                IDirect3DRMTexture_Release(texture);
            }
            if (FAILED(hr))
            {
                IDirect3DRMMesh_Release(*mesh);
                return hr;
            }
        }
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetFace(IDirect3DRMMeshBuilder3 *iface,
        DWORD index, IDirect3DRMFace2 **face)
{
    FIXME("iface %p, index %u, face %p stub!\n", iface, index, face);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetVertex(IDirect3DRMMeshBuilder3 *iface,
        DWORD index, D3DVECTOR *vector)
{
    FIXME("iface %p, index %u, vector %p stub!\n", iface, index, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetNormal(IDirect3DRMMeshBuilder3 *iface,
        DWORD index, D3DVECTOR *vector)
{
    FIXME("iface %p, index %u, vector %p stub!\n", iface, index, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_DeleteVertices(IDirect3DRMMeshBuilder3 *iface,
        DWORD start_idx, DWORD count)
{
    FIXME("iface %p, start_idx %u, count %u stub!\n", iface, start_idx, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_DeleteNormals(IDirect3DRMMeshBuilder3 *iface,
        DWORD start_idx, DWORD count)
{
    FIXME("iface %p, start_idx %u, count %u stub!\n", iface, start_idx, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_DeleteFace(IDirect3DRMMeshBuilder3 *iface, IDirect3DRMFace2 *face)
{
    FIXME("iface %p, face %p stub!\n", iface, face);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_Empty(IDirect3DRMMeshBuilder3 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_Optimize(IDirect3DRMMeshBuilder3 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_AddFacesIndexed(IDirect3DRMMeshBuilder3 *iface,
        DWORD flags, DWORD *indices, DWORD *start_idx, DWORD *count)
{
    FIXME("iface %p, flags %#x, indices %p, start_idx %p, count %p stub!\n",
            iface, flags, indices, start_idx, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_CreateSubMesh(IDirect3DRMMeshBuilder3 *iface, IUnknown **mesh)
{
    FIXME("iface %p, mesh %p stub!\n", iface, mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetParentMesh(IDirect3DRMMeshBuilder3 *iface,
        DWORD flags, IUnknown **parent)
{
    FIXME("iface %p, flags %#x, parent %p stub!\n", iface, flags, parent);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetSubMeshes(IDirect3DRMMeshBuilder3 *iface,
        DWORD *count, IUnknown **meshes)
{
    FIXME("iface %p, count %p, meshes %p stub!\n", iface, count, meshes);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_DeleteSubMesh(IDirect3DRMMeshBuilder3 *iface, IUnknown *mesh)
{
    FIXME("iface %p, mesh %p stub!\n", iface, mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_Enable(IDirect3DRMMeshBuilder3 *iface, DWORD index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetEnable(IDirect3DRMMeshBuilder3 *iface, DWORD *indices)
{
    FIXME("iface %p, indices %p stub!\n", iface, indices);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_AddTriangles(IDirect3DRMMeshBuilder3 *iface,
        DWORD flags, DWORD format, DWORD vertex_count, void *data)
{
    FIXME("iface %p, flags %#x, format %#x, vertex_count %u, data %p stub!\n",
            iface, flags, format, vertex_count, data);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetVertices(IDirect3DRMMeshBuilder3 *iface,
        DWORD start_idx, DWORD count, D3DVECTOR *vector)
{
    FIXME("iface %p, start_idx %u, count %u, vector %p stub!\n", iface, start_idx, count, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetVertices(IDirect3DRMMeshBuilder3 *iface,
        DWORD start_idx, DWORD *vertex_count, D3DVECTOR *vertices)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);
    DWORD count = mesh_builder->nb_vertices - start_idx;

    TRACE("iface %p, start_idx %u, vertex_count %p, vertices %p.\n",
            iface, start_idx, vertex_count, vertices);

    if (vertex_count)
        *vertex_count = count;
    if (vertices && mesh_builder->nb_vertices)
        memcpy(vertices, mesh_builder->pVertices + start_idx, count * sizeof(*vertices));

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_builder3_SetNormals(IDirect3DRMMeshBuilder3 *iface,
        DWORD start_idx, DWORD count, D3DVECTOR *vector)
{
    FIXME("iface %p, start_idx %u, count %u, vector %p stub!\n",
            iface, start_idx, count, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_builder3_GetNormals(IDirect3DRMMeshBuilder3 *iface,
        DWORD start_idx, DWORD *normal_count, D3DVECTOR *normals)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);
    DWORD count = mesh_builder->nb_normals - start_idx;

    TRACE("iface %p, start_idx %u, normal_count %p, normals %p stub!\n",
            iface, start_idx, normal_count, normals);

    if (normal_count)
        *normal_count = count;
    if (normals && mesh_builder->nb_normals)
        memcpy(normals, mesh_builder->pNormals + start_idx, count * sizeof(*normals));

    return D3DRM_OK;
}

static int WINAPI d3drm_mesh_builder3_GetNormalCount(IDirect3DRMMeshBuilder3 *iface)
{
    struct d3drm_mesh_builder *mesh_builder = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("iface %p.\n", iface);

    return mesh_builder->nb_normals;
}

static const struct IDirect3DRMMeshBuilder3Vtbl d3drm_mesh_builder3_vtbl =
{
    d3drm_mesh_builder3_QueryInterface,
    d3drm_mesh_builder3_AddRef,
    d3drm_mesh_builder3_Release,
    d3drm_mesh_builder3_Clone,
    d3drm_mesh_builder3_AddDestroyCallback,
    d3drm_mesh_builder3_DeleteDestroyCallback,
    d3drm_mesh_builder3_SetAppData,
    d3drm_mesh_builder3_GetAppData,
    d3drm_mesh_builder3_SetName,
    d3drm_mesh_builder3_GetName,
    d3drm_mesh_builder3_GetClassName,
    d3drm_mesh_builder3_Load,
    d3drm_mesh_builder3_Save,
    d3drm_mesh_builder3_Scale,
    d3drm_mesh_builder3_Translate,
    d3drm_mesh_builder3_SetColorSource,
    d3drm_mesh_builder3_GetBox,
    d3drm_mesh_builder3_GenerateNormals,
    d3drm_mesh_builder3_GetColorSource,
    d3drm_mesh_builder3_AddMesh,
    d3drm_mesh_builder3_AddMeshBuilder,
    d3drm_mesh_builder3_AddFrame,
    d3drm_mesh_builder3_AddFace,
    d3drm_mesh_builder3_AddFaces,
    d3drm_mesh_builder3_ReserveSpace,
    d3drm_mesh_builder3_SetColorRGB,
    d3drm_mesh_builder3_SetColor,
    d3drm_mesh_builder3_SetTexture,
    d3drm_mesh_builder3_SetMaterial,
    d3drm_mesh_builder3_SetTextureTopology,
    d3drm_mesh_builder3_SetQuality,
    d3drm_mesh_builder3_SetPerspective,
    d3drm_mesh_builder3_SetVertex,
    d3drm_mesh_builder3_SetNormal,
    d3drm_mesh_builder3_SetTextureCoordinates,
    d3drm_mesh_builder3_SetVertexColor,
    d3drm_mesh_builder3_SetVertexColorRGB,
    d3drm_mesh_builder3_GetFaces,
    d3drm_mesh_builder3_GetGeometry,
    d3drm_mesh_builder3_GetTextureCoordinates,
    d3drm_mesh_builder3_AddVertex,
    d3drm_mesh_builder3_AddNormal,
    d3drm_mesh_builder3_CreateFace,
    d3drm_mesh_builder3_GetQuality,
    d3drm_mesh_builder3_GetPerspective,
    d3drm_mesh_builder3_GetFaceCount,
    d3drm_mesh_builder3_GetVertexCount,
    d3drm_mesh_builder3_GetVertexColor,
    d3drm_mesh_builder3_CreateMesh,
    d3drm_mesh_builder3_GetFace,
    d3drm_mesh_builder3_GetVertex,
    d3drm_mesh_builder3_GetNormal,
    d3drm_mesh_builder3_DeleteVertices,
    d3drm_mesh_builder3_DeleteNormals,
    d3drm_mesh_builder3_DeleteFace,
    d3drm_mesh_builder3_Empty,
    d3drm_mesh_builder3_Optimize,
    d3drm_mesh_builder3_AddFacesIndexed,
    d3drm_mesh_builder3_CreateSubMesh,
    d3drm_mesh_builder3_GetParentMesh,
    d3drm_mesh_builder3_GetSubMeshes,
    d3drm_mesh_builder3_DeleteSubMesh,
    d3drm_mesh_builder3_Enable,
    d3drm_mesh_builder3_GetEnable,
    d3drm_mesh_builder3_AddTriangles,
    d3drm_mesh_builder3_SetVertices,
    d3drm_mesh_builder3_GetVertices,
    d3drm_mesh_builder3_SetNormals,
    d3drm_mesh_builder3_GetNormals,
    d3drm_mesh_builder3_GetNormalCount,
};

HRESULT Direct3DRMMeshBuilder_create(REFIID riid, IUnknown **out)
{
    struct d3drm_mesh_builder *object;

    TRACE("riid %s, out %p.\n", debugstr_guid(riid), out);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMMeshBuilder2_iface.lpVtbl = &d3drm_mesh_builder2_vtbl;
    object->IDirect3DRMMeshBuilder3_iface.lpVtbl = &d3drm_mesh_builder3_vtbl;
    object->ref = 1;

    if (IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder3))
        *out = (IUnknown *)&object->IDirect3DRMMeshBuilder3_iface;
    else
        *out = (IUnknown *)&object->IDirect3DRMMeshBuilder2_iface;

    return S_OK;
}

static HRESULT WINAPI d3drm_mesh_QueryInterface(IDirect3DRMMesh *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMMesh)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DRMMesh_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3drm_mesh_AddRef(IDirect3DRMMesh *iface)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);
    ULONG refcount = InterlockedIncrement(&mesh->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_mesh_Release(IDirect3DRMMesh *iface)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);
    ULONG refcount = InterlockedDecrement(&mesh->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        DWORD i;

        for (i = 0; i < mesh->nb_groups; ++i)
        {
            HeapFree(GetProcessHeap(), 0, mesh->groups[i].vertices);
            HeapFree(GetProcessHeap(), 0, mesh->groups[i].face_data);
            if (mesh->groups[i].material)
                IDirect3DRMMaterial2_Release(mesh->groups[i].material);
            if (mesh->groups[i].texture)
                IDirect3DRMTexture3_Release(mesh->groups[i].texture);
        }
        HeapFree(GetProcessHeap(), 0, mesh->groups);
        HeapFree(GetProcessHeap(), 0, mesh);
    }

    return refcount;
}

static HRESULT WINAPI d3drm_mesh_Clone(IDirect3DRMMesh *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_AddDestroyCallback(IDirect3DRMMesh *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_DeleteDestroyCallback(IDirect3DRMMesh *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_SetAppData(IDirect3DRMMesh *iface, DWORD data)
{
    FIXME("iface %p, data %#x stub!\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_mesh_GetAppData(IDirect3DRMMesh *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_mesh_SetName(IDirect3DRMMesh *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_GetName(IDirect3DRMMesh *iface, DWORD *size, char *name)
{
    FIXME("iface %p, size %p, name %p stub!\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_GetClassName(IDirect3DRMMesh *iface, DWORD *size, char *name)
{
    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    if (!size || *size < strlen("Mesh") || !name)
        return E_INVALIDARG;

    strcpy(name, "Mesh");
    *size = sizeof("Mesh");

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_Scale(IDirect3DRMMesh *iface,
        D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    FIXME("iface %p, sx %.8e, sy %.8e, sz %.8e stub!\n", iface, sx, sy, sz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_Translate(IDirect3DRMMesh *iface,
        D3DVALUE tx, D3DVALUE ty, D3DVALUE tz)
{
    FIXME("iface %p, tx %.8e, ty %.8e, tz %.8e stub!\n", iface, tx, ty, tz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_GetBox(IDirect3DRMMesh *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_AddGroup(IDirect3DRMMesh *iface, unsigned vertex_count,
        unsigned face_count, unsigned vertex_per_face, unsigned *face_data, D3DRMGROUPINDEX *id)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);
    struct mesh_group *group;

    TRACE("iface %p, vertex_count %u, face_count %u, vertex_per_face %u, face_data %p, id %p.\n",
            iface, vertex_count, face_count, vertex_per_face, face_data, id);

    if (!face_data || !id)
        return E_POINTER;

    if ((mesh->nb_groups + 1) > mesh->groups_capacity)
    {
        struct mesh_group *groups;
        ULONG new_capacity;

        if (!mesh->groups_capacity)
        {
            new_capacity = 16;
            groups = HeapAlloc(GetProcessHeap(), 0, new_capacity * sizeof(*groups));
        }
        else
        {
            new_capacity = mesh->groups_capacity * 2;
            groups = HeapReAlloc(GetProcessHeap(), 0, mesh->groups, new_capacity * sizeof(*groups));
        }

        if (!groups)
            return E_OUTOFMEMORY;

        mesh->groups_capacity = new_capacity;
        mesh->groups = groups;
    }

    group = mesh->groups + mesh->nb_groups;

    group->vertices = HeapAlloc(GetProcessHeap(), 0, vertex_count * sizeof(D3DRMVERTEX));
    if (!group->vertices)
        return E_OUTOFMEMORY;
    group->nb_vertices = vertex_count;
    group->nb_faces = face_count;
    group->vertex_per_face = vertex_per_face;

    if (vertex_per_face)
    {
        group->face_data_size = face_count * vertex_per_face;
    }
    else
    {
        unsigned i;
        unsigned nb_indices;
        unsigned* face_data_ptr = face_data;
        group->face_data_size = 0;

        for (i = 0; i < face_count; i++)
        {
            nb_indices = *face_data_ptr;
            group->face_data_size += nb_indices + 1;
            face_data_ptr += nb_indices;
        }
    }

    group->face_data = HeapAlloc(GetProcessHeap(), 0, group->face_data_size * sizeof(unsigned));
    if (!group->face_data)
    {
        HeapFree(GetProcessHeap(), 0 , group->vertices);
        return E_OUTOFMEMORY;
    }

    memcpy(group->face_data, face_data, group->face_data_size * sizeof(unsigned));

    group->material = NULL;
    group->texture = NULL;

    *id = mesh->nb_groups++;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_SetVertices(IDirect3DRMMesh *iface, D3DRMGROUPINDEX group_id,
        unsigned int start_idx, unsigned int count, D3DRMVERTEX *values)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, group_id %#x, start_idx %u, count %u, values %p.\n",
            iface, group_id, start_idx, count, values);

    if (group_id >= mesh->nb_groups)
        return D3DRMERR_BADVALUE;

    if ((start_idx + count - 1) >= mesh->groups[group_id].nb_vertices)
        return D3DRMERR_BADVALUE;

    if (!values)
        return E_POINTER;

    memcpy(mesh->groups[group_id].vertices + start_idx, values, count * sizeof(*values));

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_SetGroupColor(IDirect3DRMMesh *iface, D3DRMGROUPINDEX id, D3DCOLOR color)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, id %#x, color 0x%08x.\n", iface, id, color);

    if (id >= mesh->nb_groups)
        return D3DRMERR_BADVALUE;

    mesh->groups[id].color = color;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_SetGroupColorRGB(IDirect3DRMMesh *iface,
        D3DRMGROUPINDEX id, D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, id %#x, red %.8e, green %.8e, blue %.8e.\n", iface, id, red, green, blue);

    if (id >= mesh->nb_groups)
        return D3DRMERR_BADVALUE;

    mesh->groups[id].color = RGBA_MAKE((BYTE)(red * 255.0f), (BYTE)(green * 255.0f), (BYTE)(blue * 255.0f), 0xff);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_SetGroupMapping(IDirect3DRMMesh *iface, D3DRMGROUPINDEX id, D3DRMMAPPING value)
{
    FIXME("iface %p, id %#x, value %#x stub!\n", iface, id, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_SetGroupQuality(IDirect3DRMMesh *iface, D3DRMGROUPINDEX id, D3DRMRENDERQUALITY value)
{
    FIXME("iface %p, id %#x, value %#x stub!\n", iface, id, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_mesh_SetGroupMaterial(IDirect3DRMMesh *iface,
        D3DRMGROUPINDEX id, IDirect3DRMMaterial *material)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, id %#x, material %p.\n", iface, id, material);

    if (id >= mesh->nb_groups)
        return D3DRMERR_BADVALUE;

    if (mesh->groups[id].material)
        IDirect3DRMMaterial2_Release(mesh->groups[id].material);

    mesh->groups[id].material = (IDirect3DRMMaterial2 *)material;

    if (material)
        IDirect3DRMMaterial2_AddRef(mesh->groups[id].material);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_SetGroupTexture(IDirect3DRMMesh *iface,
        D3DRMGROUPINDEX id, IDirect3DRMTexture *texture)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, id %#x, texture %p.\n", iface, id, texture);

    if (id >= mesh->nb_groups)
        return D3DRMERR_BADVALUE;

    if (mesh->groups[id].texture)
        IDirect3DRMTexture3_Release(mesh->groups[id].texture);

    if (!texture)
    {
        mesh->groups[id].texture = NULL;
        return D3DRM_OK;
    }

    return IDirect3DRMTexture3_QueryInterface(texture, &IID_IDirect3DRMTexture, (void **)&mesh->groups[id].texture);
}

static DWORD WINAPI d3drm_mesh_GetGroupCount(IDirect3DRMMesh *iface)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p.\n", iface);

    return mesh->nb_groups;
}

static HRESULT WINAPI d3drm_mesh_GetGroup(IDirect3DRMMesh *iface, D3DRMGROUPINDEX id, unsigned *vertex_count,
        unsigned *face_count, unsigned *vertex_per_face, DWORD *face_data_size, unsigned *face_data)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, id %#x, vertex_count %p, face_count %p, vertex_per_face %p, face_data_size %p, face_data %p.\n",
            iface, id, vertex_count, face_count, vertex_per_face, face_data_size,face_data);

    if (id >= mesh->nb_groups)
        return D3DRMERR_BADVALUE;

    if (vertex_count)
        *vertex_count = mesh->groups[id].nb_vertices;
    if (face_count)
        *face_count = mesh->groups[id].nb_faces;
    if (vertex_per_face)
        *vertex_per_face = mesh->groups[id].vertex_per_face;
    if (face_data_size)
        *face_data_size = mesh->groups[id].face_data_size;
    if (face_data)
        memcpy(face_data, mesh->groups[id].face_data, mesh->groups[id].face_data_size * sizeof(*face_data));

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_GetVertices(IDirect3DRMMesh *iface,
        D3DRMGROUPINDEX group_id, DWORD start_idx, DWORD count, D3DRMVERTEX *vertices)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, group_id %#x, start_idx %u, count %u, vertices %p.\n",
            iface, group_id, start_idx, count, vertices);

    if (group_id >= mesh->nb_groups)
        return D3DRMERR_BADVALUE;

    if ((start_idx + count - 1) >= mesh->groups[group_id].nb_vertices)
        return D3DRMERR_BADVALUE;

    if (!vertices)
        return E_POINTER;

    memcpy(vertices, mesh->groups[group_id].vertices + start_idx, count * sizeof(*vertices));

    return D3DRM_OK;
}

static D3DCOLOR WINAPI d3drm_mesh_GetGroupColor(IDirect3DRMMesh *iface, D3DRMGROUPINDEX id)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, id %#x.\n", iface, id);

    return mesh->groups[id].color;
}

static D3DRMMAPPING WINAPI d3drm_mesh_GetGroupMapping(IDirect3DRMMesh *iface, D3DRMGROUPINDEX id)
{
    FIXME("iface %p, id %#x stub!\n", iface, id);

    return 0;
}
static D3DRMRENDERQUALITY WINAPI d3drm_mesh_GetGroupQuality(IDirect3DRMMesh *iface, D3DRMGROUPINDEX id)
{
    FIXME("iface %p, id %#x stub!\n", iface, id);

    return 0;
}

static HRESULT WINAPI d3drm_mesh_GetGroupMaterial(IDirect3DRMMesh *iface,
        D3DRMGROUPINDEX id, IDirect3DRMMaterial **material)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, id %#x, material %p.\n", iface, id, material);

    if (id >= mesh->nb_groups)
        return D3DRMERR_BADVALUE;

    if (!material)
        return E_POINTER;

    if (mesh->groups[id].material)
        IDirect3DRMTexture_QueryInterface(mesh->groups[id].material, &IID_IDirect3DRMMaterial, (void **)material);
    else
        *material = NULL;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_mesh_GetGroupTexture(IDirect3DRMMesh *iface,
        D3DRMGROUPINDEX id, IDirect3DRMTexture **texture)
{
    struct d3drm_mesh *mesh = impl_from_IDirect3DRMMesh(iface);

    TRACE("iface %p, id %#x, texture %p.\n", iface, id, texture);

    if (id >= mesh->nb_groups)
        return D3DRMERR_BADVALUE;

    if (!texture)
        return E_POINTER;

    if (mesh->groups[id].texture)
        IDirect3DRMTexture_QueryInterface(mesh->groups[id].texture, &IID_IDirect3DRMTexture, (void **)texture);
    else
        *texture = NULL;

    return D3DRM_OK;
}

static const struct IDirect3DRMMeshVtbl d3drm_mesh_vtbl =
{
    d3drm_mesh_QueryInterface,
    d3drm_mesh_AddRef,
    d3drm_mesh_Release,
    d3drm_mesh_Clone,
    d3drm_mesh_AddDestroyCallback,
    d3drm_mesh_DeleteDestroyCallback,
    d3drm_mesh_SetAppData,
    d3drm_mesh_GetAppData,
    d3drm_mesh_SetName,
    d3drm_mesh_GetName,
    d3drm_mesh_GetClassName,
    d3drm_mesh_Scale,
    d3drm_mesh_Translate,
    d3drm_mesh_GetBox,
    d3drm_mesh_AddGroup,
    d3drm_mesh_SetVertices,
    d3drm_mesh_SetGroupColor,
    d3drm_mesh_SetGroupColorRGB,
    d3drm_mesh_SetGroupMapping,
    d3drm_mesh_SetGroupQuality,
    d3drm_mesh_SetGroupMaterial,
    d3drm_mesh_SetGroupTexture,
    d3drm_mesh_GetGroupCount,
    d3drm_mesh_GetGroup,
    d3drm_mesh_GetVertices,
    d3drm_mesh_GetGroupColor,
    d3drm_mesh_GetGroupMapping,
    d3drm_mesh_GetGroupQuality,
    d3drm_mesh_GetGroupMaterial,
    d3drm_mesh_GetGroupTexture,
};

HRESULT Direct3DRMMesh_create(IDirect3DRMMesh **mesh)
{
    struct d3drm_mesh *object;

    TRACE("mesh %p.\n", mesh);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMMesh_iface.lpVtbl = &d3drm_mesh_vtbl;
    object->ref = 1;

    *mesh = &object->IDirect3DRMMesh_iface;

    return S_OK;
}
