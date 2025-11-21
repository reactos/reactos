/*
 * Implementation of IDirect3DRMFace Interface
 *
 * Copyright 2013 André Hentschel
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

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

static inline struct d3drm_face *impl_from_IDirect3DRMFace(IDirect3DRMFace *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_face, IDirect3DRMFace_iface);
}

static inline struct d3drm_face *impl_from_IDirect3DRMFace2(IDirect3DRMFace2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_face, IDirect3DRMFace2_iface);
}

static HRESULT WINAPI d3drm_face1_QueryInterface(IDirect3DRMFace *iface, REFIID riid, void **out)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMFace)
            || IsEqualGUID(riid, &IID_IDirect3DRMObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &face->IDirect3DRMFace_iface;
    }
    else if(IsEqualGUID(riid, &IID_IDirect3DRMFace2))
    {
        *out = &face->IDirect3DRMFace2_iface;
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

static ULONG WINAPI d3drm_face1_AddRef(IDirect3DRMFace *iface)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);
    ULONG refcount = InterlockedIncrement(&face->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_face1_Release(IDirect3DRMFace *iface)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);
    ULONG refcount = InterlockedDecrement(&face->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d3drm_object_cleanup((IDirect3DRMObject *)iface, &face->obj);
        free(face);
    }

    return refcount;
}

static HRESULT WINAPI d3drm_face1_Clone(IDirect3DRMFace *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_AddDestroyCallback(IDirect3DRMFace *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return IDirect3DRMFace2_AddDestroyCallback(&face->IDirect3DRMFace2_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_face1_DeleteDestroyCallback(IDirect3DRMFace *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return IDirect3DRMFace2_DeleteDestroyCallback(&face->IDirect3DRMFace2_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_face2_SetAppData(IDirect3DRMFace2 *iface, DWORD data)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    face->obj.appdata = data;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_face1_SetAppData(IDirect3DRMFace *iface, DWORD data)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    return d3drm_face2_SetAppData(&face->IDirect3DRMFace2_iface, data);
}

static DWORD WINAPI d3drm_face2_GetAppData(IDirect3DRMFace2 *iface)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p.\n", iface);

    return face->obj.appdata;
}

static DWORD WINAPI d3drm_face1_GetAppData(IDirect3DRMFace *iface)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_face2_GetAppData(&face->IDirect3DRMFace2_iface);
}

static HRESULT WINAPI d3drm_face2_SetName(IDirect3DRMFace2 *iface, const char *name)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_object_set_name(&face->obj, name);
}

static HRESULT WINAPI d3drm_face1_SetName(IDirect3DRMFace *iface, const char *name)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_face2_SetName(&face->IDirect3DRMFace2_iface, name);
}

static HRESULT WINAPI d3drm_face2_GetName(IDirect3DRMFace2 *iface, DWORD *size, char *name)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_name(&face->obj, size, name);
}

static HRESULT WINAPI d3drm_face1_GetName(IDirect3DRMFace *iface, DWORD *size, char *name)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_face2_GetName(&face->IDirect3DRMFace2_iface, size, name);
}

static HRESULT WINAPI d3drm_face1_GetClassName(IDirect3DRMFace *iface, DWORD *size, char *name)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMFace2_GetClassName(&face->IDirect3DRMFace2_iface, size, name);
}

static HRESULT WINAPI d3drm_face1_AddVertex(IDirect3DRMFace *iface, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, x %.8e, y %.8e, z %.8e stub!\n", iface, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_AddVertexAndNormalIndexed(IDirect3DRMFace *iface,
        DWORD vertex, DWORD normal)
{
    FIXME("iface %p, vertex %lu, normal %lu stub!\n", iface, vertex, normal);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_SetColorRGB(IDirect3DRMFace2 *iface, D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e.\n", iface, red, green, blue);

    d3drm_set_color(&face->color, red, green, blue, 1.0f);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_face1_SetColorRGB(IDirect3DRMFace *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e.\n", iface, red, green, blue);

    return d3drm_face2_SetColorRGB(&face->IDirect3DRMFace2_iface, red, green, blue);
}

static HRESULT WINAPI d3drm_face2_SetColor(IDirect3DRMFace2 *iface, D3DCOLOR color)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p, color 0x%08lx.\n", iface, color);

    face->color = color;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_face1_SetColor(IDirect3DRMFace *iface, D3DCOLOR color)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p, color 0x%08lx.\n", iface, color);

    return d3drm_face2_SetColor(&face->IDirect3DRMFace2_iface, color);
}

static HRESULT WINAPI d3drm_face1_SetTexture(IDirect3DRMFace *iface, IDirect3DRMTexture *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_SetTextureCoordinates(IDirect3DRMFace *iface,
        DWORD vertex, D3DVALUE u, D3DVALUE v)
{
    FIXME("iface %p, vertex %lu, u %.8e, v %.8e stub!\n", iface, vertex, u, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_SetMaterial(IDirect3DRMFace *iface, IDirect3DRMMaterial *material)
{
    FIXME("iface %p, material %p stub!\n", iface, material);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_SetTextureTopology(IDirect3DRMFace *iface, BOOL wrap_u, BOOL wrap_v)
{
    FIXME("iface %p, wrap_u %#x, wrap_v %#x stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_GetVertex(IDirect3DRMFace *iface,
        DWORD index, D3DVECTOR *vertex, D3DVECTOR *normal)
{
    FIXME("iface %p, index %lu, vertex %p, normal %p stub!\n", iface, index, vertex, normal);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_GetVertices(IDirect3DRMFace *iface,
        DWORD *vertex_count, D3DVECTOR *coords, D3DVECTOR *normals)
{
    FIXME("iface %p, vertex_count %p, coords %p, normals %p stub!\n",
            iface, vertex_count, coords, normals);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_GetTextureCoordinates(IDirect3DRMFace *iface,
        DWORD vertex, D3DVALUE *u, D3DVALUE *v)
{
    FIXME("iface %p, vertex %lu, u %p, v %p stub!\n", iface, vertex, u, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_GetTextureTopology(IDirect3DRMFace *iface, BOOL *wrap_u, BOOL *wrap_v)
{
    FIXME("iface %p, wrap_u %p, wrap_v %p stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_GetNormal(IDirect3DRMFace *iface, D3DVECTOR *normal)
{
    FIXME("iface %p, normal %p stub!\n", iface, normal);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_GetTexture(IDirect3DRMFace *iface, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face1_GetMaterial(IDirect3DRMFace *iface, IDirect3DRMMaterial **material)
{
    FIXME("iface %p, material %p stub!\n", iface, material);

    return E_NOTIMPL;
}

static int WINAPI d3drm_face1_GetVertexCount(IDirect3DRMFace *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static int WINAPI d3drm_face1_GetVertexIndex(IDirect3DRMFace *iface, DWORD which)
{
    FIXME("iface %p, which %lu stub!\n", iface, which);

    return 0;
}

static int WINAPI d3drm_face1_GetTextureCoordinateIndex(IDirect3DRMFace *iface, DWORD which)
{
    FIXME("iface %p, which %lu stub!\n", iface, which);

    return 0;
}

static D3DCOLOR WINAPI d3drm_face2_GetColor(IDirect3DRMFace2 *iface)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p.\n", iface);

    return face->color;
}

static D3DCOLOR WINAPI d3drm_face1_GetColor(IDirect3DRMFace *iface)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_face2_GetColor(&face->IDirect3DRMFace2_iface);
}

static const struct IDirect3DRMFaceVtbl d3drm_face1_vtbl =
{
    d3drm_face1_QueryInterface,
    d3drm_face1_AddRef,
    d3drm_face1_Release,
    d3drm_face1_Clone,
    d3drm_face1_AddDestroyCallback,
    d3drm_face1_DeleteDestroyCallback,
    d3drm_face1_SetAppData,
    d3drm_face1_GetAppData,
    d3drm_face1_SetName,
    d3drm_face1_GetName,
    d3drm_face1_GetClassName,
    d3drm_face1_AddVertex,
    d3drm_face1_AddVertexAndNormalIndexed,
    d3drm_face1_SetColorRGB,
    d3drm_face1_SetColor,
    d3drm_face1_SetTexture,
    d3drm_face1_SetTextureCoordinates,
    d3drm_face1_SetMaterial,
    d3drm_face1_SetTextureTopology,
    d3drm_face1_GetVertex,
    d3drm_face1_GetVertices,
    d3drm_face1_GetTextureCoordinates,
    d3drm_face1_GetTextureTopology,
    d3drm_face1_GetNormal,
    d3drm_face1_GetTexture,
    d3drm_face1_GetMaterial,
    d3drm_face1_GetVertexCount,
    d3drm_face1_GetVertexIndex,
    d3drm_face1_GetTextureCoordinateIndex,
    d3drm_face1_GetColor,
};

static HRESULT WINAPI d3drm_face2_QueryInterface(IDirect3DRMFace2 *iface, REFIID riid, void **out)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    return d3drm_face1_QueryInterface(&face->IDirect3DRMFace_iface, riid, out);
}

static ULONG WINAPI d3drm_face2_AddRef(IDirect3DRMFace2 *iface)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    return d3drm_face1_AddRef(&face->IDirect3DRMFace_iface);
}

static ULONG WINAPI d3drm_face2_Release(IDirect3DRMFace2 *iface)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    return d3drm_face1_Release(&face->IDirect3DRMFace_iface);
}

static HRESULT WINAPI d3drm_face2_Clone(IDirect3DRMFace2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_AddDestroyCallback(IDirect3DRMFace2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_object_add_destroy_callback(&face->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_face2_DeleteDestroyCallback(IDirect3DRMFace2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_object_delete_destroy_callback(&face->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_face2_GetClassName(IDirect3DRMFace2 *iface, DWORD *size, char *name)
{
    struct d3drm_face *face = impl_from_IDirect3DRMFace2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_class_name(&face->obj, size, name);
}

static HRESULT WINAPI d3drm_face2_AddVertex(IDirect3DRMFace2 *iface, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, x %.8e, y %.8e, z %.8e stub!\n", iface, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_AddVertexAndNormalIndexed(IDirect3DRMFace2 *iface,
        DWORD vertex, DWORD normal)
{
    FIXME("iface %p, vertex %lu, normal %lu stub!\n", iface, vertex, normal);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_SetTexture(IDirect3DRMFace2 *iface, IDirect3DRMTexture3 *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_SetTextureCoordinates(IDirect3DRMFace2 *iface,
        DWORD vertex, D3DVALUE u, D3DVALUE v)
{
    FIXME("iface %p, vertex %lu, u %.8e, v %.8e stub!\n", iface, vertex, u, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_SetMaterial(IDirect3DRMFace2 *iface, IDirect3DRMMaterial2 *material)
{
    FIXME("iface %p, material %p stub!\n", iface, material);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_SetTextureTopology(IDirect3DRMFace2 *iface, BOOL wrap_u, BOOL wrap_v)
{
    FIXME("iface %p, wrap_u %#x, wrap_v %#x stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_GetVertex(IDirect3DRMFace2 *iface,
        DWORD index, D3DVECTOR *vertex, D3DVECTOR *normal)
{
    FIXME("iface %p, index %lu, vertex %p, normal %p stub!\n", iface, index, vertex, normal);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_GetVertices(IDirect3DRMFace2 *iface,
        DWORD *vertex_count, D3DVECTOR *coords, D3DVECTOR *normals)
{
    FIXME("iface %p, vertex_count %p, coords %p, normals %p stub!\n",
            iface, vertex_count, coords, normals);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_GetTextureCoordinates(IDirect3DRMFace2 *iface,
        DWORD vertex, D3DVALUE *u, D3DVALUE *v)
{
    FIXME("iface %p, vertex %lu, u %p, v %p stub!\n", iface, vertex, u, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_GetTextureTopology(IDirect3DRMFace2 *iface, BOOL *wrap_u, BOOL *wrap_v)
{
    FIXME("iface %p, wrap_u %p, wrap_v %p stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_GetNormal(IDirect3DRMFace2 *iface, D3DVECTOR *normal)
{
    FIXME("iface %p, normal %p stub!\n", iface, normal);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_GetTexture(IDirect3DRMFace2 *iface, IDirect3DRMTexture3 **texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_face2_GetMaterial(IDirect3DRMFace2 *iface, IDirect3DRMMaterial2 **material)
{
    FIXME("iface %p, material %p stub!\n", iface, material);

    return E_NOTIMPL;
}

static int WINAPI d3drm_face2_GetVertexCount(IDirect3DRMFace2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static int WINAPI d3drm_face2_GetVertexIndex(IDirect3DRMFace2 *iface, DWORD which)
{
    FIXME("iface %p, which %lu stub!\n", iface, which);

    return 0;
}

static int WINAPI d3drm_face2_GetTextureCoordinateIndex(IDirect3DRMFace2 *iface, DWORD which)
{
    FIXME("iface %p, which %lu stub!\n", iface, which);

    return 0;
}

static const struct IDirect3DRMFace2Vtbl d3drm_face2_vtbl =
{
    d3drm_face2_QueryInterface,
    d3drm_face2_AddRef,
    d3drm_face2_Release,
    d3drm_face2_Clone,
    d3drm_face2_AddDestroyCallback,
    d3drm_face2_DeleteDestroyCallback,
    d3drm_face2_SetAppData,
    d3drm_face2_GetAppData,
    d3drm_face2_SetName,
    d3drm_face2_GetName,
    d3drm_face2_GetClassName,
    d3drm_face2_AddVertex,
    d3drm_face2_AddVertexAndNormalIndexed,
    d3drm_face2_SetColorRGB,
    d3drm_face2_SetColor,
    d3drm_face2_SetTexture,
    d3drm_face2_SetTextureCoordinates,
    d3drm_face2_SetMaterial,
    d3drm_face2_SetTextureTopology,
    d3drm_face2_GetVertex,
    d3drm_face2_GetVertices,
    d3drm_face2_GetTextureCoordinates,
    d3drm_face2_GetTextureTopology,
    d3drm_face2_GetNormal,
    d3drm_face2_GetTexture,
    d3drm_face2_GetMaterial,
    d3drm_face2_GetVertexCount,
    d3drm_face2_GetVertexIndex,
    d3drm_face2_GetTextureCoordinateIndex,
    d3drm_face2_GetColor,
};

HRESULT d3drm_face_create(struct d3drm_face **face)
{
    static const char classname[] = "Face";
    struct d3drm_face *object;

    TRACE("face %p.\n", face);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMFace_iface.lpVtbl = &d3drm_face1_vtbl;
    object->IDirect3DRMFace2_iface.lpVtbl = &d3drm_face2_vtbl;
    object->ref = 1;

    d3drm_object_init(&object->obj, classname);

    *face = object;

    return S_OK;
}
