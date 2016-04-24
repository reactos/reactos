/*
 * Implementation of IDirect3DRMFrame Interface
 *
 * Copyright 2011, 2012 André Hentschel
 * Copyright 2012 Christian Costa
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

#include <assert.h>

static D3DRMMATRIX4D identity = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

struct d3drm_frame
{
    IDirect3DRMFrame2 IDirect3DRMFrame2_iface;
    IDirect3DRMFrame3 IDirect3DRMFrame3_iface;
    LONG ref;
    struct d3drm_frame *parent;
    ULONG nb_children;
    ULONG children_capacity;
    IDirect3DRMFrame3** children;
    ULONG nb_visuals;
    ULONG visuals_capacity;
    IDirect3DRMVisual** visuals;
    ULONG nb_lights;
    ULONG lights_capacity;
    IDirect3DRMLight** lights;
    D3DRMMATRIX4D transform;
    D3DCOLOR scenebackground;
};

struct d3drm_frame_array
{
    IDirect3DRMFrameArray IDirect3DRMFrameArray_iface;
    LONG ref;
    ULONG size;
    IDirect3DRMFrame **frames;
};

struct d3drm_visual_array
{
    IDirect3DRMVisualArray IDirect3DRMVisualArray_iface;
    LONG ref;
    ULONG size;
    IDirect3DRMVisual **visuals;
};

struct d3drm_light_array
{
    IDirect3DRMLightArray IDirect3DRMLightArray_iface;
    LONG ref;
    ULONG size;
    IDirect3DRMLight **lights;
};

static inline struct d3drm_frame *impl_from_IDirect3DRMFrame2(IDirect3DRMFrame2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_frame, IDirect3DRMFrame2_iface);
}

static inline struct d3drm_frame *impl_from_IDirect3DRMFrame3(IDirect3DRMFrame3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_frame, IDirect3DRMFrame3_iface);
}

static inline struct d3drm_frame *unsafe_impl_from_IDirect3DRMFrame3(IDirect3DRMFrame3 *iface);

static inline struct d3drm_frame_array *impl_from_IDirect3DRMFrameArray(IDirect3DRMFrameArray *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_frame_array, IDirect3DRMFrameArray_iface);
}

static inline struct d3drm_visual_array *impl_from_IDirect3DRMVisualArray(IDirect3DRMVisualArray *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_visual_array, IDirect3DRMVisualArray_iface);
}

static inline struct d3drm_light_array *impl_from_IDirect3DRMLightArray(IDirect3DRMLightArray *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_light_array, IDirect3DRMLightArray_iface);
}

static HRESULT WINAPI d3drm_frame_array_QueryInterface(IDirect3DRMFrameArray *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMFrameArray)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DRMFrameArray_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3drm_frame_array_AddRef(IDirect3DRMFrameArray *iface)
{
    struct d3drm_frame_array *array = impl_from_IDirect3DRMFrameArray(iface);
    ULONG refcount = InterlockedIncrement(&array->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_frame_array_Release(IDirect3DRMFrameArray *iface)
{
    struct d3drm_frame_array *array = impl_from_IDirect3DRMFrameArray(iface);
    ULONG refcount = InterlockedDecrement(&array->ref);
    ULONG i;

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < array->size; ++i)
        {
            IDirect3DRMFrame_Release(array->frames[i]);
        }
        HeapFree(GetProcessHeap(), 0, array->frames);
        HeapFree(GetProcessHeap(), 0, array);
    }

    return refcount;
}

static DWORD WINAPI d3drm_frame_array_GetSize(IDirect3DRMFrameArray *iface)
{
    struct d3drm_frame_array *array = impl_from_IDirect3DRMFrameArray(iface);

    TRACE("iface %p.\n", iface);

    return array->size;
}

static HRESULT WINAPI d3drm_frame_array_GetElement(IDirect3DRMFrameArray *iface,
        DWORD index, IDirect3DRMFrame **frame)
{
    struct d3drm_frame_array *array = impl_from_IDirect3DRMFrameArray(iface);

    TRACE("iface %p, index %u, frame %p.\n", iface, index, frame);

    if (!frame)
        return D3DRMERR_BADVALUE;

    if (index >= array->size)
    {
        *frame = NULL;
        return D3DRMERR_BADVALUE;
    }

    IDirect3DRMFrame_AddRef(array->frames[index]);
    *frame = array->frames[index];

    return D3DRM_OK;
}

static const struct IDirect3DRMFrameArrayVtbl d3drm_frame_array_vtbl =
{
    d3drm_frame_array_QueryInterface,
    d3drm_frame_array_AddRef,
    d3drm_frame_array_Release,
    d3drm_frame_array_GetSize,
    d3drm_frame_array_GetElement,
};

static struct d3drm_frame_array *d3drm_frame_array_create(unsigned int frame_count, IDirect3DRMFrame3 **frames)
{
    struct d3drm_frame_array *array;
    unsigned int i;

    if (!(array = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*array))))
        return NULL;

    array->IDirect3DRMFrameArray_iface.lpVtbl = &d3drm_frame_array_vtbl;
    array->ref = 1;
    array->size = frame_count;

    if (frame_count)
    {
        if (!(array->frames = HeapAlloc(GetProcessHeap(), 0, frame_count * sizeof(*array->frames))))
        {
            HeapFree(GetProcessHeap(), 0, array);
            return NULL;
        }

        for (i = 0; i < frame_count; ++i)
        {
            IDirect3DRMFrame3_QueryInterface(frames[i], &IID_IDirect3DRMFrame, (void **)&array->frames[i]);
        }
    }

    return array;
}

static HRESULT WINAPI d3drm_visual_array_QueryInterface(IDirect3DRMVisualArray *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMVisualArray)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DRMVisualArray_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3drm_visual_array_AddRef(IDirect3DRMVisualArray *iface)
{
    struct d3drm_visual_array *array = impl_from_IDirect3DRMVisualArray(iface);
    ULONG refcount = InterlockedIncrement(&array->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_visual_array_Release(IDirect3DRMVisualArray *iface)
{
    struct d3drm_visual_array *array = impl_from_IDirect3DRMVisualArray(iface);
    ULONG refcount = InterlockedDecrement(&array->ref);
    ULONG i;

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < array->size; ++i)
        {
            IDirect3DRMVisual_Release(array->visuals[i]);
        }
        HeapFree(GetProcessHeap(), 0, array->visuals);
        HeapFree(GetProcessHeap(), 0, array);
    }

    return refcount;
}

static DWORD WINAPI d3drm_visual_array_GetSize(IDirect3DRMVisualArray *iface)
{
    struct d3drm_visual_array *array = impl_from_IDirect3DRMVisualArray(iface);

    TRACE("iface %p.\n", iface);

    return array->size;
}

static HRESULT WINAPI d3drm_visual_array_GetElement(IDirect3DRMVisualArray *iface,
        DWORD index, IDirect3DRMVisual **visual)
{
    struct d3drm_visual_array *array = impl_from_IDirect3DRMVisualArray(iface);

    TRACE("iface %p, index %u, visual %p.\n", iface, index, visual);

    if (!visual)
        return D3DRMERR_BADVALUE;

    if (index >= array->size)
    {
        *visual = NULL;
        return D3DRMERR_BADVALUE;
    }

    IDirect3DRMVisual_AddRef(array->visuals[index]);
    *visual = array->visuals[index];

    return D3DRM_OK;
}

static const struct IDirect3DRMVisualArrayVtbl d3drm_visual_array_vtbl =
{
    d3drm_visual_array_QueryInterface,
    d3drm_visual_array_AddRef,
    d3drm_visual_array_Release,
    d3drm_visual_array_GetSize,
    d3drm_visual_array_GetElement,
};

static struct d3drm_visual_array *d3drm_visual_array_create(unsigned int visual_count, IDirect3DRMVisual **visuals)
{
    struct d3drm_visual_array *array;
    unsigned int i;

    if (!(array = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*array))))
        return NULL;

    array->IDirect3DRMVisualArray_iface.lpVtbl = &d3drm_visual_array_vtbl;
    array->ref = 1;
    array->size = visual_count;

    if (visual_count)
    {
        if (!(array->visuals = HeapAlloc(GetProcessHeap(), 0, visual_count * sizeof(*array->visuals))))
        {
            HeapFree(GetProcessHeap(), 0, array);
            return NULL;
        }

        for (i = 0; i < visual_count; ++i)
        {
            array->visuals[i] = visuals[i];
            IDirect3DRMVisual_AddRef(array->visuals[i]);
        }
    }

    return array;
}

static HRESULT WINAPI d3drm_light_array_QueryInterface(IDirect3DRMLightArray *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMLightArray)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DRMLightArray_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3drm_light_array_AddRef(IDirect3DRMLightArray *iface)
{
    struct d3drm_light_array *array = impl_from_IDirect3DRMLightArray(iface);
    ULONG refcount = InterlockedIncrement(&array->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_light_array_Release(IDirect3DRMLightArray *iface)
{
    struct d3drm_light_array *array = impl_from_IDirect3DRMLightArray(iface);
    ULONG refcount = InterlockedDecrement(&array->ref);
    ULONG i;

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < array->size; ++i)
        {
            IDirect3DRMLight_Release(array->lights[i]);
        }
        HeapFree(GetProcessHeap(), 0, array->lights);
        HeapFree(GetProcessHeap(), 0, array);
    }

    return refcount;
}

static DWORD WINAPI d3drm_light_array_GetSize(IDirect3DRMLightArray *iface)
{
    struct d3drm_light_array *array = impl_from_IDirect3DRMLightArray(iface);

    TRACE("iface %p.\n", iface);

    return array->size;
}

static HRESULT WINAPI d3drm_light_array_GetElement(IDirect3DRMLightArray *iface,
        DWORD index, IDirect3DRMLight **light)
{
    struct d3drm_light_array *array = impl_from_IDirect3DRMLightArray(iface);

    TRACE("iface %p, index %u, light %p.\n", iface, index, light);

    if (!light)
        return D3DRMERR_BADVALUE;

    if (index >= array->size)
    {
        *light = NULL;
        return D3DRMERR_BADVALUE;
    }

    IDirect3DRMLight_AddRef(array->lights[index]);
    *light = array->lights[index];

    return D3DRM_OK;
}

static const struct IDirect3DRMLightArrayVtbl d3drm_light_array_vtbl =
{
    d3drm_light_array_QueryInterface,
    d3drm_light_array_AddRef,
    d3drm_light_array_Release,
    d3drm_light_array_GetSize,
    d3drm_light_array_GetElement,
};

static struct d3drm_light_array *d3drm_light_array_create(unsigned int light_count, IDirect3DRMLight **lights)
{
    struct d3drm_light_array *array;
    unsigned int i;

    if (!(array = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*array))))
        return NULL;

    array->IDirect3DRMLightArray_iface.lpVtbl = &d3drm_light_array_vtbl;
    array->ref = 1;
    array->size = light_count;

    if (light_count)
    {
        if (!(array->lights = HeapAlloc(GetProcessHeap(), 0, light_count * sizeof(*array->lights))))
        {
            HeapFree(GetProcessHeap(), 0, array);
            return NULL;
        }

        for (i = 0; i < light_count; ++i)
        {
            array->lights[i] = lights[i];
            IDirect3DRMLight_AddRef(array->lights[i]);
        }
    }

    return array;
}

static HRESULT WINAPI d3drm_frame2_QueryInterface(IDirect3DRMFrame2 *iface, REFIID riid, void **out)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return IDirect3DRMFrame3_QueryInterface(&frame->IDirect3DRMFrame3_iface, riid, out);
}

static ULONG WINAPI d3drm_frame2_AddRef(IDirect3DRMFrame2 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMFrame3_AddRef(&frame->IDirect3DRMFrame3_iface);
}

static ULONG WINAPI d3drm_frame2_Release(IDirect3DRMFrame2 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMFrame3_Release(&frame->IDirect3DRMFrame3_iface);
}

static HRESULT WINAPI d3drm_frame2_Clone(IDirect3DRMFrame2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_AddDestroyCallback(IDirect3DRMFrame2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_DeleteDestroyCallback(IDirect3DRMFrame2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetAppData(IDirect3DRMFrame2 *iface, DWORD data)
{
    FIXME("iface %p, data %#x stub!\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_frame2_GetAppData(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_frame2_SetName(IDirect3DRMFrame2 *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetName(IDirect3DRMFrame2 *iface, DWORD *size, char *name)
{
    FIXME("iface %p, size %p, name %p stub!\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetClassName(IDirect3DRMFrame2 *iface, DWORD *size, char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMFrame3_GetClassName(&frame->IDirect3DRMFrame3_iface, size, name);
}

static HRESULT WINAPI d3drm_frame2_AddChild(IDirect3DRMFrame2 *iface, IDirect3DRMFrame *child)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);
    IDirect3DRMFrame3 *child3;
    HRESULT hr;

    TRACE("iface %p, child %p.\n", iface, child);

    if (!child)
        return D3DRMERR_BADOBJECT;
    hr = IDirect3DRMFrame_QueryInterface(child, &IID_IDirect3DRMFrame3, (void **)&child3);
    if (hr != S_OK)
        return D3DRMERR_BADOBJECT;
    IDirect3DRMFrame_Release(child);

    return IDirect3DRMFrame3_AddChild(&frame->IDirect3DRMFrame3_iface, child3);
}

static HRESULT WINAPI d3drm_frame2_AddLight(IDirect3DRMFrame2 *iface, IDirect3DRMLight *light)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, light %p.\n", iface, light);

    return IDirect3DRMFrame3_AddLight(&frame->IDirect3DRMFrame3_iface, light);
}

static HRESULT WINAPI d3drm_frame2_AddMoveCallback(IDirect3DRMFrame2 *iface,
        D3DRMFRAMEMOVECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_AddTransform(IDirect3DRMFrame2 *iface, D3DRMCOMBINETYPE type, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, type %#x, matrix %p.\n", iface, type, matrix);

    return IDirect3DRMFrame3_AddTransform(&frame->IDirect3DRMFrame3_iface, type, matrix);
}

static HRESULT WINAPI d3drm_frame2_AddTranslation(IDirect3DRMFrame2 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, type %#x, x %.8e, y %.8e, z %.8e stub!\n", iface, type, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_AddScale(IDirect3DRMFrame2 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    FIXME("iface %p, type %#x, sx %.8e, sy %.8e, sz %.8e stub!\n", iface, type, sx, sy, sz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_AddRotation(IDirect3DRMFrame2 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
    FIXME("iface %p, type %#x, x %.8e, y %.8e, z %.8e, theta %.8e stub!\n", iface, type, x, y, z, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_AddVisual(IDirect3DRMFrame2 *iface, IDirect3DRMVisual *visual)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, visual %p.\n", iface, visual);

    return IDirect3DRMFrame3_AddVisual(&frame->IDirect3DRMFrame3_iface, (IUnknown *)visual);
}

static HRESULT WINAPI d3drm_frame2_GetChildren(IDirect3DRMFrame2 *iface, IDirect3DRMFrameArray **children)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, children %p.\n", iface, children);

    return IDirect3DRMFrame3_GetChildren(&frame->IDirect3DRMFrame3_iface, children);
}

static D3DCOLOR WINAPI d3drm_frame2_GetColor(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_frame2_GetLights(IDirect3DRMFrame2 *iface, IDirect3DRMLightArray **lights)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, lights %p.\n", iface, lights);

    return IDirect3DRMFrame3_GetLights(&frame->IDirect3DRMFrame3_iface, lights);
}

static D3DRMMATERIALMODE WINAPI d3drm_frame2_GetMaterialMode(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMMATERIAL_FROMPARENT;
}

static HRESULT WINAPI d3drm_frame2_GetParent(IDirect3DRMFrame2 *iface, IDirect3DRMFrame **parent)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, parent %p.\n", iface, parent);

    if (!parent)
        return D3DRMERR_BADVALUE;

    if (frame->parent)
    {
        *parent = (IDirect3DRMFrame *)&frame->parent->IDirect3DRMFrame2_iface;
        IDirect3DRMFrame_AddRef(*parent);
    }
    else
    {
        *parent = NULL;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_GetPosition(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *position)
{
    FIXME("iface %p, reference %p, position %p stub!\n", iface, reference, position);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetRotation(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *axis, D3DVALUE *theta)
{
    FIXME("iface %p, reference %p, axis %p, theta %p stub!\n", iface, reference, axis, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetScene(IDirect3DRMFrame2 *iface, IDirect3DRMFrame **scene)
{
    FIXME("iface %p, scene %p stub!\n", iface, scene);

    return E_NOTIMPL;
}

static D3DRMSORTMODE WINAPI d3drm_frame2_GetSortMode(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMSORT_FROMPARENT;
}

static HRESULT WINAPI d3drm_frame2_GetTexture(IDirect3DRMFrame2 *iface, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetTransform(IDirect3DRMFrame2 *iface, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, matrix %p.\n", iface, matrix);

    memcpy(matrix, frame->transform, sizeof(D3DRMMATRIX4D));

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_GetVelocity(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *velocity, BOOL with_rotation)
{
    FIXME("iface %p, reference %p, velocity %p, with_rotation %#x stub!\n",
            iface, reference, velocity, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetOrientation(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *dir, D3DVECTOR *up)
{
    FIXME("iface %p, reference %p, dir %p, up %p stub!\n", iface, reference, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetVisuals(IDirect3DRMFrame2 *iface, IDirect3DRMVisualArray **visuals)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);
    struct d3drm_visual_array *array;

    TRACE("iface %p, visuals %p.\n", iface, visuals);

    if (!visuals)
        return D3DRMERR_BADVALUE;

    if (!(array = d3drm_visual_array_create(frame->nb_visuals, frame->visuals)))
        return E_OUTOFMEMORY;

    *visuals = &array->IDirect3DRMVisualArray_iface;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_GetTextureTopology(IDirect3DRMFrame2 *iface, BOOL *wrap_u, BOOL *wrap_v)
{
    FIXME("iface %p, wrap_u %p, wrap_v %p stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_InverseTransform(IDirect3DRMFrame2 *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_Load(IDirect3DRMFrame2 *iface, void *filename,
        void *name, D3DRMLOADOPTIONS flags, D3DRMLOADTEXTURECALLBACK cb, void *ctx)
{
    FIXME("iface %p, filename %p, name %p, flags %#x, cb %p, ctx %p stub!\n",
            iface, filename, name, flags, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_LookAt(IDirect3DRMFrame2 *iface, IDirect3DRMFrame *target,
        IDirect3DRMFrame *reference, D3DRMFRAMECONSTRAINT constraint)
{
    FIXME("iface %p, target %p, reference %p, constraint %#x stub!\n", iface, target, reference, constraint);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_Move(IDirect3DRMFrame2 *iface, D3DVALUE delta)
{
    FIXME("iface %p, delta %.8e stub!\n", iface, delta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_DeleteChild(IDirect3DRMFrame2 *iface, IDirect3DRMFrame *child)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);
    IDirect3DRMFrame3 *child3;
    HRESULT hr;

    TRACE("iface %p, child %p.\n", iface, child);

    if (!child)
        return D3DRMERR_BADOBJECT;
    if (FAILED(hr = IDirect3DRMFrame_QueryInterface(child, &IID_IDirect3DRMFrame3, (void **)&child3)))
        return D3DRMERR_BADOBJECT;
    IDirect3DRMFrame_Release(child);

    return IDirect3DRMFrame3_DeleteChild(&frame->IDirect3DRMFrame3_iface, child3);
}

static HRESULT WINAPI d3drm_frame2_DeleteLight(IDirect3DRMFrame2 *iface, IDirect3DRMLight *light)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, light %p.\n", iface, light);

    return IDirect3DRMFrame3_DeleteLight(&frame->IDirect3DRMFrame3_iface, light);
}

static HRESULT WINAPI d3drm_frame2_DeleteMoveCallback(IDirect3DRMFrame2 *iface,
        D3DRMFRAMEMOVECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_DeleteVisual(IDirect3DRMFrame2 *iface, IDirect3DRMVisual *visual)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, visual %p.\n", iface, visual);

    return IDirect3DRMFrame3_DeleteVisual(&frame->IDirect3DRMFrame3_iface, (IUnknown *)visual);
}

static D3DCOLOR WINAPI d3drm_frame2_GetSceneBackground(IDirect3DRMFrame2 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMFrame3_GetSceneBackground(&frame->IDirect3DRMFrame3_iface);
}

static HRESULT WINAPI d3drm_frame2_GetSceneBackgroundDepth(IDirect3DRMFrame2 *iface,
        IDirectDrawSurface **surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static D3DCOLOR WINAPI d3drm_frame2_GetSceneFogColor(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL WINAPI d3drm_frame2_GetSceneFogEnable(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static D3DRMFOGMODE WINAPI d3drm_frame2_GetSceneFogMode(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMFOG_LINEAR;
}

static HRESULT WINAPI d3drm_frame2_GetSceneFogParams(IDirect3DRMFrame2 *iface,
        D3DVALUE *start, D3DVALUE *end, D3DVALUE *density)
{
    FIXME("iface %p, start %p, end %p, density %p stub!\n", iface, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneBackground(IDirect3DRMFrame2 *iface, D3DCOLOR color)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, color 0x%08x.\n", iface, color);

    return IDirect3DRMFrame3_SetSceneBackground(&frame->IDirect3DRMFrame3_iface, color);
}

static HRESULT WINAPI d3drm_frame2_SetSceneBackgroundRGB(IDirect3DRMFrame2 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e.\n", iface, red, green, blue);

    return IDirect3DRMFrame3_SetSceneBackgroundRGB(&frame->IDirect3DRMFrame3_iface, red, green, blue);
}

static HRESULT WINAPI d3drm_frame2_SetSceneBackgroundDepth(IDirect3DRMFrame2 *iface, IDirectDrawSurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneBackgroundImage(IDirect3DRMFrame2 *iface, IDirect3DRMTexture *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneFogEnable(IDirect3DRMFrame2 *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneFogColor(IDirect3DRMFrame2 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08x stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneFogMode(IDirect3DRMFrame2 *iface, D3DRMFOGMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneFogParams(IDirect3DRMFrame2 *iface,
        D3DVALUE start, D3DVALUE end, D3DVALUE density)
{
    FIXME("iface %p, start %.8e, end %.8e, density %.8e stub!\n", iface, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetColor(IDirect3DRMFrame2 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08x stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetColorRGB(IDirect3DRMFrame2 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    FIXME("iface %p, red %.8e, green %.8e, blue %.8e stub!\n", iface, red, green, blue);

    return E_NOTIMPL;
}

static D3DRMZBUFFERMODE WINAPI d3drm_frame2_GetZbufferMode(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMZBUFFER_FROMPARENT;
}

static HRESULT WINAPI d3drm_frame2_SetMaterialMode(IDirect3DRMFrame2 *iface, D3DRMMATERIALMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetOrientation(IDirect3DRMFrame2 *iface, IDirect3DRMFrame *reference,
        D3DVALUE dx, D3DVALUE dy, D3DVALUE dz, D3DVALUE ux, D3DVALUE uy, D3DVALUE uz)
{
    FIXME("iface %p, reference %p, dx %.8e, dy %.8e, dz %.8e, ux %.8e, uy %.8e, uz %.8e stub!\n",
            iface, reference, dx, dy, dz, ux, uy, uz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetPosition(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e stub!\n", iface, reference, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetRotation(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e, theta %.8e stub!\n",
            iface, reference, x, y, z, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSortMode(IDirect3DRMFrame2 *iface, D3DRMSORTMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetTexture(IDirect3DRMFrame2 *iface, IDirect3DRMTexture *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetTextureTopology(IDirect3DRMFrame2 *iface, BOOL wrap_u, BOOL wrap_v)
{
    FIXME("iface %p, wrap_u %#x, wrap_v %#x stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetVelocity(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e, with_rotation %#x stub!\n",
            iface, reference, x, y, z, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetZbufferMode(IDirect3DRMFrame2 *iface, D3DRMZBUFFERMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_Transform(IDirect3DRMFrame2 *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_AddMoveCallback2(IDirect3DRMFrame2 *iface,
        D3DRMFRAMEMOVECALLBACK cb, void *ctx, DWORD flags)
{
    FIXME("iface %p, cb %p, ctx %p, flags %#x stub!\n", iface, cb, ctx, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetBox(IDirect3DRMFrame2 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static BOOL WINAPI d3drm_frame2_GetBoxEnable(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT WINAPI d3drm_frame2_GetAxes(IDirect3DRMFrame2 *iface, D3DVECTOR *dir, D3DVECTOR *up)
{
    FIXME("iface %p, dir %p, up %p stub!\n", iface, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetMaterial(IDirect3DRMFrame2 *iface, IDirect3DRMMaterial **material)
{
    FIXME("iface %p, material %p stub!\n", iface, material);

    return E_NOTIMPL;
}

static BOOL WINAPI d3drm_frame2_GetInheritAxes(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT WINAPI d3drm_frame2_GetHierarchyBox(IDirect3DRMFrame2 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static const struct IDirect3DRMFrame2Vtbl d3drm_frame2_vtbl =
{
    d3drm_frame2_QueryInterface,
    d3drm_frame2_AddRef,
    d3drm_frame2_Release,
    d3drm_frame2_Clone,
    d3drm_frame2_AddDestroyCallback,
    d3drm_frame2_DeleteDestroyCallback,
    d3drm_frame2_SetAppData,
    d3drm_frame2_GetAppData,
    d3drm_frame2_SetName,
    d3drm_frame2_GetName,
    d3drm_frame2_GetClassName,
    d3drm_frame2_AddChild,
    d3drm_frame2_AddLight,
    d3drm_frame2_AddMoveCallback,
    d3drm_frame2_AddTransform,
    d3drm_frame2_AddTranslation,
    d3drm_frame2_AddScale,
    d3drm_frame2_AddRotation,
    d3drm_frame2_AddVisual,
    d3drm_frame2_GetChildren,
    d3drm_frame2_GetColor,
    d3drm_frame2_GetLights,
    d3drm_frame2_GetMaterialMode,
    d3drm_frame2_GetParent,
    d3drm_frame2_GetPosition,
    d3drm_frame2_GetRotation,
    d3drm_frame2_GetScene,
    d3drm_frame2_GetSortMode,
    d3drm_frame2_GetTexture,
    d3drm_frame2_GetTransform,
    d3drm_frame2_GetVelocity,
    d3drm_frame2_GetOrientation,
    d3drm_frame2_GetVisuals,
    d3drm_frame2_GetTextureTopology,
    d3drm_frame2_InverseTransform,
    d3drm_frame2_Load,
    d3drm_frame2_LookAt,
    d3drm_frame2_Move,
    d3drm_frame2_DeleteChild,
    d3drm_frame2_DeleteLight,
    d3drm_frame2_DeleteMoveCallback,
    d3drm_frame2_DeleteVisual,
    d3drm_frame2_GetSceneBackground,
    d3drm_frame2_GetSceneBackgroundDepth,
    d3drm_frame2_GetSceneFogColor,
    d3drm_frame2_GetSceneFogEnable,
    d3drm_frame2_GetSceneFogMode,
    d3drm_frame2_GetSceneFogParams,
    d3drm_frame2_SetSceneBackground,
    d3drm_frame2_SetSceneBackgroundRGB,
    d3drm_frame2_SetSceneBackgroundDepth,
    d3drm_frame2_SetSceneBackgroundImage,
    d3drm_frame2_SetSceneFogEnable,
    d3drm_frame2_SetSceneFogColor,
    d3drm_frame2_SetSceneFogMode,
    d3drm_frame2_SetSceneFogParams,
    d3drm_frame2_SetColor,
    d3drm_frame2_SetColorRGB,
    d3drm_frame2_GetZbufferMode,
    d3drm_frame2_SetMaterialMode,
    d3drm_frame2_SetOrientation,
    d3drm_frame2_SetPosition,
    d3drm_frame2_SetRotation,
    d3drm_frame2_SetSortMode,
    d3drm_frame2_SetTexture,
    d3drm_frame2_SetTextureTopology,
    d3drm_frame2_SetVelocity,
    d3drm_frame2_SetZbufferMode,
    d3drm_frame2_Transform,
    d3drm_frame2_AddMoveCallback2,
    d3drm_frame2_GetBox,
    d3drm_frame2_GetBoxEnable,
    d3drm_frame2_GetAxes,
    d3drm_frame2_GetMaterial,
    d3drm_frame2_GetInheritAxes,
    d3drm_frame2_GetHierarchyBox,
};

static HRESULT WINAPI d3drm_frame3_QueryInterface(IDirect3DRMFrame3 *iface, REFIID riid, void **out)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMFrame2)
            || IsEqualGUID(riid, &IID_IDirect3DRMFrame)
            || IsEqualGUID(riid, &IID_IDirect3DRMObject)
            || IsEqualGUID(riid, &IID_IDirect3DRMVisual)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &frame->IDirect3DRMFrame2_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMFrame3))
    {
        *out = &frame->IDirect3DRMFrame3_iface;
    }
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(riid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI d3drm_frame3_AddRef(IDirect3DRMFrame3 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    ULONG refcount = InterlockedIncrement(&frame->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_frame3_Release(IDirect3DRMFrame3 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    ULONG refcount = InterlockedDecrement(&frame->ref);
    ULONG i;

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < frame->nb_children; ++i)
        {
            IDirect3DRMFrame3_Release(frame->children[i]);
        }
        HeapFree(GetProcessHeap(), 0, frame->children);
        for (i = 0; i < frame->nb_visuals; ++i)
        {
            IDirect3DRMVisual_Release(frame->visuals[i]);
        }
        HeapFree(GetProcessHeap(), 0, frame->visuals);
        for (i = 0; i < frame->nb_lights; ++i)
        {
            IDirect3DRMLight_Release(frame->lights[i]);
        }
        HeapFree(GetProcessHeap(), 0, frame->lights);
        HeapFree(GetProcessHeap(), 0, frame);
    }

    return refcount;
}

static HRESULT WINAPI d3drm_frame3_Clone(IDirect3DRMFrame3 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_AddDestroyCallback(IDirect3DRMFrame3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_DeleteDestroyCallback(IDirect3DRMFrame3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetAppData(IDirect3DRMFrame3 *iface, DWORD data)
{
    FIXME("iface %p, data %#x stub!\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI d3drm_frame3_GetAppData(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_frame3_SetName(IDirect3DRMFrame3 *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetName(IDirect3DRMFrame3 *iface, DWORD *size, char *name)
{
    FIXME("iface %p, size %p, name %p stub!\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetClassName(IDirect3DRMFrame3 *iface, DWORD *size, char *name)
{
    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    if (!size || *size < strlen("Frame") || !name)
        return E_INVALIDARG;

    strcpy(name, "Frame");
    *size = sizeof("Frame");

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_AddChild(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 *child)
{
    struct d3drm_frame *This = impl_from_IDirect3DRMFrame3(iface);
    struct d3drm_frame *child_obj = unsafe_impl_from_IDirect3DRMFrame3(child);

    TRACE("iface %p, child %p.\n", iface, child);

    if (!child_obj)
        return D3DRMERR_BADOBJECT;

    if (child_obj->parent)
    {
        IDirect3DRMFrame3* parent = &child_obj->parent->IDirect3DRMFrame3_iface;

        if (parent == iface)
        {
            /* Passed frame is already a child so return success */
            return D3DRM_OK;
        }
        else
        {
            /* Remove parent and continue */
            IDirect3DRMFrame3_DeleteChild(parent, child);
        }
    }

    if ((This->nb_children + 1) > This->children_capacity)
    {
        ULONG new_capacity;
        IDirect3DRMFrame3** children;

        if (!This->children_capacity)
        {
            new_capacity = 16;
            children = HeapAlloc(GetProcessHeap(), 0, new_capacity * sizeof(IDirect3DRMFrame3*));
        }
        else
        {
            new_capacity = This->children_capacity * 2;
            children = HeapReAlloc(GetProcessHeap(), 0, This->children, new_capacity * sizeof(IDirect3DRMFrame3*));
        }

        if (!children)
            return E_OUTOFMEMORY;

        This->children_capacity = new_capacity;
        This->children = children;
    }

    This->children[This->nb_children++] = child;
    IDirect3DRMFrame3_AddRef(child);
    child_obj->parent = This;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_AddLight(IDirect3DRMFrame3 *iface, IDirect3DRMLight *light)
{
    struct d3drm_frame *This = impl_from_IDirect3DRMFrame3(iface);
    ULONG i;
    IDirect3DRMLight** lights;

    TRACE("iface %p, light %p.\n", iface, light);

    if (!light)
        return D3DRMERR_BADOBJECT;

    /* Check if already existing and return gracefully without increasing ref count */
    for (i = 0; i < This->nb_lights; i++)
        if (This->lights[i] == light)
            return D3DRM_OK;

    if ((This->nb_lights + 1) > This->lights_capacity)
    {
        ULONG new_capacity;

        if (!This->lights_capacity)
        {
            new_capacity = 16;
            lights = HeapAlloc(GetProcessHeap(), 0, new_capacity * sizeof(IDirect3DRMLight*));
        }
        else
        {
            new_capacity = This->lights_capacity * 2;
            lights = HeapReAlloc(GetProcessHeap(), 0, This->lights, new_capacity * sizeof(IDirect3DRMLight*));
        }

        if (!lights)
            return E_OUTOFMEMORY;

        This->lights_capacity = new_capacity;
        This->lights = lights;
    }

    This->lights[This->nb_lights++] = light;
    IDirect3DRMLight_AddRef(light);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_AddMoveCallback(IDirect3DRMFrame3 *iface,
        D3DRMFRAME3MOVECALLBACK cb, void *ctx, DWORD flags)
{
    FIXME("iface %p, cb %p, ctx %p flags %#x stub!\n", iface, cb, ctx, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_AddTransform(IDirect3DRMFrame3 *iface,
        D3DRMCOMBINETYPE type, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, type %#x, matrix %p.\n", iface, type, matrix);

    switch (type)
    {
        case D3DRMCOMBINE_REPLACE:
            memcpy(frame->transform, matrix, sizeof(D3DRMMATRIX4D));
            break;

        case D3DRMCOMBINE_BEFORE:
            FIXME("D3DRMCOMBINE_BEFORE not supported yet\n");
            break;

        case D3DRMCOMBINE_AFTER:
            FIXME("D3DRMCOMBINE_AFTER not supported yet\n");
            break;

        default:
            WARN("Unknown Combine Type %u\n", type);
            return D3DRMERR_BADVALUE;
    }

    return S_OK;
}

static HRESULT WINAPI d3drm_frame3_AddTranslation(IDirect3DRMFrame3 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, type %#x, x %.8e, y %.8e, z %.8e stub!\n", iface, type, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_AddScale(IDirect3DRMFrame3 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    FIXME("iface %p, type %#x, sx %.8e, sy %.8e, sz %.8e stub!\n", iface, type, sx, sy, sz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_AddRotation(IDirect3DRMFrame3 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
    FIXME("iface %p, type %#x, x %.8e, y %.8e, z %.8e, theta %.8e stub!\n",
            iface, type, x, y, z, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_AddVisual(IDirect3DRMFrame3 *iface, IUnknown *visual)
{
    struct d3drm_frame *This = impl_from_IDirect3DRMFrame3(iface);
    ULONG i;
    IDirect3DRMVisual** visuals;

    TRACE("iface %p, visual %p.\n", iface, visual);

    if (!visual)
        return D3DRMERR_BADOBJECT;

    /* Check if already existing and return gracefully without increasing ref count */
    for (i = 0; i < This->nb_visuals; i++)
        if (This->visuals[i] == (IDirect3DRMVisual *)visual)
            return D3DRM_OK;

    if ((This->nb_visuals + 1) > This->visuals_capacity)
    {
        ULONG new_capacity;

        if (!This->visuals_capacity)
        {
            new_capacity = 16;
            visuals = HeapAlloc(GetProcessHeap(), 0, new_capacity * sizeof(IDirect3DRMVisual*));
        }
        else
        {
            new_capacity = This->visuals_capacity * 2;
            visuals = HeapReAlloc(GetProcessHeap(), 0, This->visuals, new_capacity * sizeof(IDirect3DRMVisual*));
        }

        if (!visuals)
            return E_OUTOFMEMORY;

        This->visuals_capacity = new_capacity;
        This->visuals = visuals;
    }

    This->visuals[This->nb_visuals++] = (IDirect3DRMVisual *)visual;
    IDirect3DRMVisual_AddRef(visual);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_GetChildren(IDirect3DRMFrame3 *iface, IDirect3DRMFrameArray **children)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    struct d3drm_frame_array *array;

    TRACE("iface %p, children %p.\n", iface, children);

    if (!children)
        return D3DRMERR_BADVALUE;

    if (!(array = d3drm_frame_array_create(frame->nb_children, frame->children)))
        return E_OUTOFMEMORY;

    *children = &array->IDirect3DRMFrameArray_iface;

    return D3DRM_OK;
}

static D3DCOLOR WINAPI d3drm_frame3_GetColor(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_frame3_GetLights(IDirect3DRMFrame3 *iface, IDirect3DRMLightArray **lights)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    struct d3drm_light_array *array;

    TRACE("iface %p, lights %p.\n", iface, lights);

    if (!lights)
        return D3DRMERR_BADVALUE;

    if (!(array = d3drm_light_array_create(frame->nb_lights, frame->lights)))
        return E_OUTOFMEMORY;

    *lights = &array->IDirect3DRMLightArray_iface;

    return D3DRM_OK;
}

static D3DRMMATERIALMODE WINAPI d3drm_frame3_GetMaterialMode(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMMATERIAL_FROMPARENT;
}

static HRESULT WINAPI d3drm_frame3_GetParent(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 **parent)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, parent %p.\n", iface, parent);

    if (!parent)
        return D3DRMERR_BADVALUE;

    if (frame->parent)
    {
        *parent = &frame->parent->IDirect3DRMFrame3_iface;
        IDirect3DRMFrame_AddRef(*parent);
    }
    else
    {
        *parent = NULL;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_GetPosition(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVECTOR *position)
{
    FIXME("iface %p, reference %p, position %p stub!\n", iface, reference, position);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetRotation(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVECTOR *axis, D3DVALUE *theta)
{
    FIXME("iface %p, reference %p, axis %p, theta %p stub!\n", iface, reference, axis, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetScene(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 **scene)
{
    FIXME("iface %p, scene %p stub!\n", iface, scene);

    return E_NOTIMPL;
}

static D3DRMSORTMODE WINAPI d3drm_frame3_GetSortMode(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMSORT_FROMPARENT;
}

static HRESULT WINAPI d3drm_frame3_GetTexture(IDirect3DRMFrame3 *iface, IDirect3DRMTexture3 **texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetTransform(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, reference %p, matrix %p.\n", iface, reference, matrix);

    if (reference)
        FIXME("Specifying a frame as the root of the scene different from the current root frame is not supported yet\n");

    memcpy(matrix, frame->transform, sizeof(D3DRMMATRIX4D));

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_GetVelocity(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVECTOR *velocity, BOOL with_rotation)
{
    FIXME("iface %p, reference %p, velocity %p, with_rotation %#x stub!\n",
            iface, reference, velocity, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetOrientation(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVECTOR *dir, D3DVECTOR *up)
{
    FIXME("iface %p, reference %p, dir %p, up %p stub!\n", iface, reference, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetVisuals(IDirect3DRMFrame3 *iface,
        DWORD *count, IUnknown **visuals)
{
    FIXME("iface %p, count %p, visuals %p stub!\n", iface, count, visuals);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_InverseTransform(IDirect3DRMFrame3 *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_Load(IDirect3DRMFrame3 *iface, void *filename,
        void *name, D3DRMLOADOPTIONS flags, D3DRMLOADTEXTURE3CALLBACK cb, void *ctx)
{
    FIXME("iface %p, filename %p, name %p, flags %#x, cb %p, ctx %p stub!\n",
            iface, filename, name, flags, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_LookAt(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 *target,
        IDirect3DRMFrame3 *reference, D3DRMFRAMECONSTRAINT constraint)
{
    FIXME("iface %p, target %p, reference %p, constraint %#x stub!\n", iface, target, reference, constraint);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_Move(IDirect3DRMFrame3 *iface, D3DVALUE delta)
{
    FIXME("iface %p, delta %.8e stub!\n", iface, delta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_DeleteChild(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 *child)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    struct d3drm_frame *child_impl = unsafe_impl_from_IDirect3DRMFrame3(child);
    ULONG i;

    TRACE("iface %p, child %p.\n", iface, child);

    if (!child_impl)
        return D3DRMERR_BADOBJECT;

    /* Check if child exists */
    for (i = 0; i < frame->nb_children; ++i)
    {
        if (frame->children[i] == child)
            break;
    }

    if (i == frame->nb_children)
        return D3DRMERR_BADVALUE;

    memmove(frame->children + i, frame->children + i + 1, sizeof(*frame->children) * (frame->nb_children - 1 - i));
    IDirect3DRMFrame3_Release(child);
    child_impl->parent = NULL;
    --frame->nb_children;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_DeleteLight(IDirect3DRMFrame3 *iface, IDirect3DRMLight *light)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    ULONG i;

    TRACE("iface %p, light %p.\n", iface, light);

    if (!light)
        return D3DRMERR_BADOBJECT;

    /* Check if visual exists */
    for (i = 0; i < frame->nb_lights; ++i)
    {
        if (frame->lights[i] == light)
            break;
    }

    if (i == frame->nb_lights)
        return D3DRMERR_BADVALUE;

    memmove(frame->lights + i, frame->lights + i + 1, sizeof(*frame->lights) * (frame->nb_lights - 1 - i));
    IDirect3DRMLight_Release(light);
    --frame->nb_lights;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_DeleteMoveCallback(IDirect3DRMFrame3 *iface,
        D3DRMFRAME3MOVECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_DeleteVisual(IDirect3DRMFrame3 *iface, IUnknown *visual)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    ULONG i;

    TRACE("iface %p, visual %p.\n", iface, visual);

    if (!visual)
        return D3DRMERR_BADOBJECT;

    /* Check if visual exists */
    for (i = 0; i < frame->nb_visuals; ++i)
    {
        if (frame->visuals[i] == (IDirect3DRMVisual *)visual)
            break;
    }

    if (i == frame->nb_visuals)
        return D3DRMERR_BADVALUE;

    memmove(frame->visuals + i, frame->visuals + i + 1, sizeof(*frame->visuals) * (frame->nb_visuals - 1 - i));
    IDirect3DRMVisual_Release(visual);
    --frame->nb_visuals;

    return D3DRM_OK;
}

static D3DCOLOR WINAPI d3drm_frame3_GetSceneBackground(IDirect3DRMFrame3 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p.\n", iface);

    return frame->scenebackground;
}

static HRESULT WINAPI d3drm_frame3_GetSceneBackgroundDepth(IDirect3DRMFrame3 *iface,
        IDirectDrawSurface **surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static D3DCOLOR WINAPI d3drm_frame3_GetSceneFogColor(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL WINAPI d3drm_frame3_GetSceneFogEnable(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static D3DRMFOGMODE WINAPI d3drm_frame3_GetSceneFogMode(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMFOG_LINEAR;
}

static HRESULT WINAPI d3drm_frame3_GetSceneFogParams(IDirect3DRMFrame3 *iface,
        D3DVALUE *start, D3DVALUE *end, D3DVALUE *density)
{
    FIXME("iface %p, start %p, end %p, density %p stub!\n", iface, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneBackground(IDirect3DRMFrame3 *iface, D3DCOLOR color)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, color 0x%08x.\n", iface, color);

    frame->scenebackground = color;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_SetSceneBackgroundRGB(IDirect3DRMFrame3 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e stub!\n", iface, red, green, blue);

    frame->scenebackground = RGBA_MAKE((BYTE)(red * 255.0f),
            (BYTE)(green * 255.0f), (BYTE)(blue * 255.0f), 0xff);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_SetSceneBackgroundDepth(IDirect3DRMFrame3 *iface,
        IDirectDrawSurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneBackgroundImage(IDirect3DRMFrame3 *iface,
        IDirect3DRMTexture3 *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneFogEnable(IDirect3DRMFrame3 *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneFogColor(IDirect3DRMFrame3 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08x stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneFogMode(IDirect3DRMFrame3 *iface, D3DRMFOGMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneFogParams(IDirect3DRMFrame3 *iface,
        D3DVALUE start, D3DVALUE end, D3DVALUE density)
{
    FIXME("iface %p, start %.8e, end %.8e, density %.8e stub!\n", iface, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetColor(IDirect3DRMFrame3 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08x stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetColorRGB(IDirect3DRMFrame3 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    FIXME("iface %p, red %.8e, green %.8e, blue %.8e stub!\n", iface, red, green, blue);

    return E_NOTIMPL;
}

static D3DRMZBUFFERMODE WINAPI d3drm_frame3_GetZbufferMode(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMZBUFFER_FROMPARENT;
}

static HRESULT WINAPI d3drm_frame3_SetMaterialMode(IDirect3DRMFrame3 *iface, D3DRMMATERIALMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetOrientation(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 *reference,
        D3DVALUE dx, D3DVALUE dy, D3DVALUE dz, D3DVALUE ux, D3DVALUE uy, D3DVALUE uz)
{
    FIXME("iface %p, reference %p, dx %.8e, dy %.8e, dz %.8e, ux %.8e, uy %.8e, uz %.8e stub!\n",
            iface, reference, dx, dy, dz, ux, uy, uz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetPosition(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e stub!\n", iface, reference, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetRotation(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e, theta %.8e stub!\n",
            iface, reference, x, y, z, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSortMode(IDirect3DRMFrame3 *iface, D3DRMSORTMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetTexture(IDirect3DRMFrame3 *iface, IDirect3DRMTexture3 *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetVelocity(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e, with_rotation %#x.\n",
            iface, reference, x, y, z, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetZbufferMode(IDirect3DRMFrame3 *iface, D3DRMZBUFFERMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_Transform(IDirect3DRMFrame3 *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetBox(IDirect3DRMFrame3 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static BOOL WINAPI d3drm_frame3_GetBoxEnable(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT WINAPI d3drm_frame3_GetAxes(IDirect3DRMFrame3 *iface, D3DVECTOR *dir, D3DVECTOR *up)
{
    FIXME("iface %p, dir %p, up %p stub!\n", iface, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetMaterial(IDirect3DRMFrame3 *iface, IDirect3DRMMaterial2 **material)
{
    FIXME("iface %p, material %p stub!\n", iface, material);

    return E_NOTIMPL;
}

static BOOL WINAPI d3drm_frame3_GetInheritAxes(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT WINAPI d3drm_frame3_GetHierarchyBox(IDirect3DRMFrame3 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetBox(IDirect3DRMFrame3 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetBoxEnable(IDirect3DRMFrame3 *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetAxes(IDirect3DRMFrame3 *iface,
        D3DVALUE dx, D3DVALUE dy, D3DVALUE dz, D3DVALUE ux, D3DVALUE uy, D3DVALUE uz)
{
    FIXME("iface %p, dx %.8e, dy %.8e, dz %.8e, ux %.8e, uy %.8e, uz %.8e stub!\n",
            iface, dx, dy, dz, ux, uy, uz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetInheritAxes(IDirect3DRMFrame3 *iface, BOOL inherit)
{
    FIXME("iface %p, inherit %#x stub!\n", iface, inherit);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetMaterial(IDirect3DRMFrame3 *iface, IDirect3DRMMaterial2 *material)
{
    FIXME("iface %p, material %p stub!\n", iface, material);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetQuaternion(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DRMQUATERNION *q)
{
    FIXME("iface %p, reference %p, q %p stub!\n", iface, reference, q);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_RayPick(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 *reference,
        D3DRMRAY *ray, DWORD flags, IDirect3DRMPicked2Array **visuals)
{
    FIXME("iface %p, reference %p, ray %p, flags %#x, visuals %p stub!\n",
            iface, reference, ray, flags, visuals);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_Save(IDirect3DRMFrame3 *iface,
        const char *filename, D3DRMXOFFORMAT format, D3DRMSAVEOPTIONS flags)
{
    FIXME("iface %p, filename %s, format %#x, flags %#x stub!\n",
            iface, debugstr_a(filename), format, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_TransformVectors(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, DWORD num, D3DVECTOR *dst, D3DVECTOR *src)
{
    FIXME("iface %p, reference %p, num %u, dst %p, src %p stub!\n", iface, reference, num, dst, src);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_InverseTransformVectors(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, DWORD num, D3DVECTOR *dst, D3DVECTOR *src)
{
    FIXME("iface %p, reference %p, num %u, dst %p, src %p stub!\n", iface, reference, num, dst, src);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetTraversalOptions(IDirect3DRMFrame3 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetTraversalOptions(IDirect3DRMFrame3 *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneFogMethod(IDirect3DRMFrame3 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetSceneFogMethod(IDirect3DRMFrame3 *iface, DWORD *fog_mode)
{
    FIXME("iface %p, fog_mode %p stub!\n", iface, fog_mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetMaterialOverride(IDirect3DRMFrame3 *iface,
        D3DRMMATERIALOVERRIDE *override)
{
    FIXME("iface %p, override %p stub!\n", iface, override);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetMaterialOverride(IDirect3DRMFrame3 *iface,
        D3DRMMATERIALOVERRIDE *override)
{
    FIXME("iface %p, override %p stub!\n", iface, override);

    return E_NOTIMPL;
}

static const struct IDirect3DRMFrame3Vtbl d3drm_frame3_vtbl =
{
    d3drm_frame3_QueryInterface,
    d3drm_frame3_AddRef,
    d3drm_frame3_Release,
    d3drm_frame3_Clone,
    d3drm_frame3_AddDestroyCallback,
    d3drm_frame3_DeleteDestroyCallback,
    d3drm_frame3_SetAppData,
    d3drm_frame3_GetAppData,
    d3drm_frame3_SetName,
    d3drm_frame3_GetName,
    d3drm_frame3_GetClassName,
    d3drm_frame3_AddChild,
    d3drm_frame3_AddLight,
    d3drm_frame3_AddMoveCallback,
    d3drm_frame3_AddTransform,
    d3drm_frame3_AddTranslation,
    d3drm_frame3_AddScale,
    d3drm_frame3_AddRotation,
    d3drm_frame3_AddVisual,
    d3drm_frame3_GetChildren,
    d3drm_frame3_GetColor,
    d3drm_frame3_GetLights,
    d3drm_frame3_GetMaterialMode,
    d3drm_frame3_GetParent,
    d3drm_frame3_GetPosition,
    d3drm_frame3_GetRotation,
    d3drm_frame3_GetScene,
    d3drm_frame3_GetSortMode,
    d3drm_frame3_GetTexture,
    d3drm_frame3_GetTransform,
    d3drm_frame3_GetVelocity,
    d3drm_frame3_GetOrientation,
    d3drm_frame3_GetVisuals,
    d3drm_frame3_InverseTransform,
    d3drm_frame3_Load,
    d3drm_frame3_LookAt,
    d3drm_frame3_Move,
    d3drm_frame3_DeleteChild,
    d3drm_frame3_DeleteLight,
    d3drm_frame3_DeleteMoveCallback,
    d3drm_frame3_DeleteVisual,
    d3drm_frame3_GetSceneBackground,
    d3drm_frame3_GetSceneBackgroundDepth,
    d3drm_frame3_GetSceneFogColor,
    d3drm_frame3_GetSceneFogEnable,
    d3drm_frame3_GetSceneFogMode,
    d3drm_frame3_GetSceneFogParams,
    d3drm_frame3_SetSceneBackground,
    d3drm_frame3_SetSceneBackgroundRGB,
    d3drm_frame3_SetSceneBackgroundDepth,
    d3drm_frame3_SetSceneBackgroundImage,
    d3drm_frame3_SetSceneFogEnable,
    d3drm_frame3_SetSceneFogColor,
    d3drm_frame3_SetSceneFogMode,
    d3drm_frame3_SetSceneFogParams,
    d3drm_frame3_SetColor,
    d3drm_frame3_SetColorRGB,
    d3drm_frame3_GetZbufferMode,
    d3drm_frame3_SetMaterialMode,
    d3drm_frame3_SetOrientation,
    d3drm_frame3_SetPosition,
    d3drm_frame3_SetRotation,
    d3drm_frame3_SetSortMode,
    d3drm_frame3_SetTexture,
    d3drm_frame3_SetVelocity,
    d3drm_frame3_SetZbufferMode,
    d3drm_frame3_Transform,
    d3drm_frame3_GetBox,
    d3drm_frame3_GetBoxEnable,
    d3drm_frame3_GetAxes,
    d3drm_frame3_GetMaterial,
    d3drm_frame3_GetInheritAxes,
    d3drm_frame3_GetHierarchyBox,
    d3drm_frame3_SetBox,
    d3drm_frame3_SetBoxEnable,
    d3drm_frame3_SetAxes,
    d3drm_frame3_SetInheritAxes,
    d3drm_frame3_SetMaterial,
    d3drm_frame3_SetQuaternion,
    d3drm_frame3_RayPick,
    d3drm_frame3_Save,
    d3drm_frame3_TransformVectors,
    d3drm_frame3_InverseTransformVectors,
    d3drm_frame3_SetTraversalOptions,
    d3drm_frame3_GetTraversalOptions,
    d3drm_frame3_SetSceneFogMethod,
    d3drm_frame3_GetSceneFogMethod,
    d3drm_frame3_SetMaterialOverride,
    d3drm_frame3_GetMaterialOverride,
};

static inline struct d3drm_frame *unsafe_impl_from_IDirect3DRMFrame3(IDirect3DRMFrame3 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3drm_frame3_vtbl);

    return impl_from_IDirect3DRMFrame3(iface);
}

HRESULT Direct3DRMFrame_create(REFIID riid, IUnknown *parent, IUnknown **out)
{
    struct d3drm_frame *object;
    HRESULT hr;

    TRACE("riid %s, parent %p, out %p.\n", debugstr_guid(riid), parent, out);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMFrame2_iface.lpVtbl = &d3drm_frame2_vtbl;
    object->IDirect3DRMFrame3_iface.lpVtbl = &d3drm_frame3_vtbl;
    object->ref = 1;
    object->scenebackground = RGBA_MAKE(0, 0, 0, 0xff);

    memcpy(object->transform, identity, sizeof(D3DRMMATRIX4D));

    if (parent)
    {
        IDirect3DRMFrame3 *p;

        hr = IDirect3DRMFrame_QueryInterface(parent, &IID_IDirect3DRMFrame3, (void**)&p);
        if (hr != S_OK)
        {
            HeapFree(GetProcessHeap(), 0, object);
            return hr;
        }
        IDirect3DRMFrame_Release(parent);
        IDirect3DRMFrame3_AddChild(p, &object->IDirect3DRMFrame3_iface);
    }

    hr = IDirect3DRMFrame3_QueryInterface(&object->IDirect3DRMFrame3_iface, riid, (void **)out);
    IDirect3DRMFrame3_Release(&object->IDirect3DRMFrame3_iface);
    return hr;
}
