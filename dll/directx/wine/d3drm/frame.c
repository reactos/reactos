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

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

static const struct d3drm_matrix identity =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
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

static inline struct d3drm_frame *impl_from_IDirect3DRMFrame(IDirect3DRMFrame *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_frame, IDirect3DRMFrame_iface);
}

static inline struct d3drm_frame *impl_from_IDirect3DRMFrame2(IDirect3DRMFrame2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_frame, IDirect3DRMFrame2_iface);
}

static inline struct d3drm_frame *impl_from_IDirect3DRMFrame3(IDirect3DRMFrame3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_frame, IDirect3DRMFrame3_iface);
}

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

static inline struct d3drm_animation *impl_from_IDirect3DRMAnimation(IDirect3DRMAnimation *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_animation, IDirect3DRMAnimation_iface);
}

static inline struct d3drm_animation *impl_from_IDirect3DRMAnimation2(IDirect3DRMAnimation2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_animation, IDirect3DRMAnimation2_iface);
}

static void d3drm_matrix_multiply_affine(struct d3drm_matrix *dst,
        const struct d3drm_matrix *src1, const struct d3drm_matrix *src2)
{
    struct d3drm_matrix tmp;

    tmp._11 = src1->_11 * src2->_11 + src1->_12 * src2->_21 + src1->_13 * src2->_31;
    tmp._12 = src1->_11 * src2->_12 + src1->_12 * src2->_22 + src1->_13 * src2->_32;
    tmp._13 = src1->_11 * src2->_13 + src1->_12 * src2->_23 + src1->_13 * src2->_33;
    tmp._14 = 0.0f;

    tmp._21 = src1->_21 * src2->_11 + src1->_22 * src2->_21 + src1->_23 * src2->_31;
    tmp._22 = src1->_21 * src2->_12 + src1->_22 * src2->_22 + src1->_23 * src2->_32;
    tmp._23 = src1->_21 * src2->_13 + src1->_22 * src2->_23 + src1->_23 * src2->_33;
    tmp._24 = 0.0f;

    tmp._31 = src1->_31 * src2->_11 + src1->_32 * src2->_21 + src1->_33 * src2->_31;
    tmp._32 = src1->_31 * src2->_12 + src1->_32 * src2->_22 + src1->_33 * src2->_32;
    tmp._33 = src1->_31 * src2->_13 + src1->_32 * src2->_23 + src1->_33 * src2->_33;
    tmp._34 = 0.0f;

    tmp._41 = src1->_41 * src2->_11 + src1->_42 * src2->_21 + src1->_43 * src2->_31 + src2->_41;
    tmp._42 = src1->_41 * src2->_12 + src1->_42 * src2->_22 + src1->_43 * src2->_32 + src2->_42;
    tmp._43 = src1->_41 * src2->_13 + src1->_42 * src2->_23 + src1->_43 * src2->_33 + src2->_43;
    tmp._44 = 1.0f;

    *dst = tmp;
}

static void d3drm_matrix_set_rotation(struct d3drm_matrix *matrix, D3DVECTOR *axis, float theta)
{
    float sin_theta, cos_theta, vers_theta;

    D3DRMVectorNormalize(axis);
    sin_theta = sinf(theta);
    cos_theta = cosf(theta);
    vers_theta = 1.0f - cos_theta;

    matrix->_11 = vers_theta * axis->x * axis->x + cos_theta;
    matrix->_21 = vers_theta * axis->x * axis->y - sin_theta * axis->z;
    matrix->_31 = vers_theta * axis->x * axis->z + sin_theta * axis->y;
    matrix->_41 = 0.0f;

    matrix->_12 = vers_theta * axis->y * axis->x + sin_theta * axis->z;
    matrix->_22 = vers_theta * axis->y * axis->y + cos_theta;
    matrix->_32 = vers_theta * axis->y * axis->z - sin_theta * axis->x;
    matrix->_42 = 0.0f;

    matrix->_13 = vers_theta * axis->z * axis->x - sin_theta * axis->y;
    matrix->_23 = vers_theta * axis->z * axis->y + sin_theta * axis->x;
    matrix->_33 = vers_theta * axis->z * axis->z + cos_theta;
    matrix->_43 = 0.0f;

    matrix->_14 = 0.0f;
    matrix->_24 = 0.0f;
    matrix->_34 = 0.0f;
    matrix->_44 = 1.0f;
}

static void d3drm_vector_transform_affine(D3DVECTOR *dst, const D3DVECTOR *v, const struct d3drm_matrix *m)
{
    D3DVECTOR tmp;

    tmp.x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31 + m->_41;
    tmp.y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32 + m->_42;
    tmp.z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33 + m->_43;

    *dst = tmp;
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

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_frame_array_Release(IDirect3DRMFrameArray *iface)
{
    struct d3drm_frame_array *array = impl_from_IDirect3DRMFrameArray(iface);
    ULONG refcount = InterlockedDecrement(&array->ref);
    ULONG i;

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < array->size; ++i)
        {
            IDirect3DRMFrame_Release(array->frames[i]);
        }
        free(array->frames);
        free(array);
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

    TRACE("iface %p, index %lu, frame %p.\n", iface, index, frame);

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

    if (!(array = calloc(1, sizeof(*array))))
        return NULL;

    array->IDirect3DRMFrameArray_iface.lpVtbl = &d3drm_frame_array_vtbl;
    array->ref = 1;
    array->size = frame_count;

    if (frame_count)
    {
        if (!(array->frames = calloc(frame_count, sizeof(*array->frames))))
        {
            free(array);
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

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_visual_array_Release(IDirect3DRMVisualArray *iface)
{
    struct d3drm_visual_array *array = impl_from_IDirect3DRMVisualArray(iface);
    ULONG refcount = InterlockedDecrement(&array->ref);
    ULONG i;

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < array->size; ++i)
        {
            IDirect3DRMVisual_Release(array->visuals[i]);
        }
        free(array->visuals);
        free(array);
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

    TRACE("iface %p, index %lu, visual %p.\n", iface, index, visual);

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

    if (!(array = calloc(1, sizeof(*array))))
        return NULL;

    array->IDirect3DRMVisualArray_iface.lpVtbl = &d3drm_visual_array_vtbl;
    array->ref = 1;
    array->size = visual_count;

    if (visual_count)
    {
        if (!(array->visuals = calloc(visual_count, sizeof(*array->visuals))))
        {
            free(array);
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

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_light_array_Release(IDirect3DRMLightArray *iface)
{
    struct d3drm_light_array *array = impl_from_IDirect3DRMLightArray(iface);
    ULONG refcount = InterlockedDecrement(&array->ref);
    ULONG i;

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        for (i = 0; i < array->size; ++i)
        {
            IDirect3DRMLight_Release(array->lights[i]);
        }
        free(array->lights);
        free(array);
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

    TRACE("iface %p, index %lu, light %p.\n", iface, index, light);

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

    if (!(array = calloc(1, sizeof(*array))))
        return NULL;

    array->IDirect3DRMLightArray_iface.lpVtbl = &d3drm_light_array_vtbl;
    array->ref = 1;
    array->size = light_count;

    if (light_count)
    {
        if (!(array->lights = calloc(light_count, sizeof(*array->lights))))
        {
            free(array);
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

static HRESULT WINAPI d3drm_frame3_QueryInterface(IDirect3DRMFrame3 *iface, REFIID riid, void **out)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMFrame)
            || IsEqualGUID(riid, &IID_IDirect3DRMObject)
            || IsEqualGUID(riid, &IID_IDirect3DRMVisual)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &frame->IDirect3DRMFrame_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMFrame2))
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

static HRESULT WINAPI d3drm_frame2_QueryInterface(IDirect3DRMFrame2 *iface, REFIID riid, void **out)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return d3drm_frame3_QueryInterface(&frame->IDirect3DRMFrame3_iface, riid, out);
}

static HRESULT WINAPI d3drm_frame1_QueryInterface(IDirect3DRMFrame *iface, REFIID riid, void **out)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return d3drm_frame3_QueryInterface(&frame->IDirect3DRMFrame3_iface, riid, out);
}

static ULONG WINAPI d3drm_frame3_AddRef(IDirect3DRMFrame3 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    ULONG refcount = InterlockedIncrement(&frame->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_frame2_AddRef(IDirect3DRMFrame2 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_frame3_AddRef(&frame->IDirect3DRMFrame3_iface);
}

static ULONG WINAPI d3drm_frame1_AddRef(IDirect3DRMFrame *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_frame3_AddRef(&frame->IDirect3DRMFrame3_iface);
}

static ULONG WINAPI d3drm_frame3_Release(IDirect3DRMFrame3 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    ULONG refcount = InterlockedDecrement(&frame->ref);
    ULONG i;

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d3drm_object_cleanup((IDirect3DRMObject *)&frame->IDirect3DRMFrame_iface, &frame->obj);
        for (i = 0; i < frame->nb_children; ++i)
        {
            IDirect3DRMFrame3_Release(frame->children[i]);
        }
        free(frame->children);
        for (i = 0; i < frame->nb_visuals; ++i)
        {
            IDirect3DRMVisual_Release(frame->visuals[i]);
        }
        free(frame->visuals);
        for (i = 0; i < frame->nb_lights; ++i)
        {
            IDirect3DRMLight_Release(frame->lights[i]);
        }
        free(frame->lights);
        IDirect3DRM_Release(frame->d3drm);
        free(frame);
    }

    return refcount;
}

static ULONG WINAPI d3drm_frame2_Release(IDirect3DRMFrame2 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_frame3_Release(&frame->IDirect3DRMFrame3_iface);
}

static ULONG WINAPI d3drm_frame1_Release(IDirect3DRMFrame *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_frame3_Release(&frame->IDirect3DRMFrame3_iface);
}

static HRESULT WINAPI d3drm_frame3_Clone(IDirect3DRMFrame3 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_Clone(IDirect3DRMFrame2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_Clone(IDirect3DRMFrame *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_AddDestroyCallback(IDirect3DRMFrame3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_object_add_destroy_callback(&frame->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_frame2_AddDestroyCallback(IDirect3DRMFrame2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return IDirect3DRMFrame3_AddDestroyCallback(&frame->IDirect3DRMFrame3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_frame1_AddDestroyCallback(IDirect3DRMFrame *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return IDirect3DRMFrame3_AddDestroyCallback(&frame->IDirect3DRMFrame3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_frame3_DeleteDestroyCallback(IDirect3DRMFrame3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_object_delete_destroy_callback(&frame->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_frame2_DeleteDestroyCallback(IDirect3DRMFrame2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return IDirect3DRMFrame3_DeleteDestroyCallback(&frame->IDirect3DRMFrame3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_frame1_DeleteDestroyCallback(IDirect3DRMFrame *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return IDirect3DRMFrame3_DeleteDestroyCallback(&frame->IDirect3DRMFrame3_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_frame3_SetAppData(IDirect3DRMFrame3 *iface, DWORD data)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    frame->obj.appdata = data;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_SetAppData(IDirect3DRMFrame2 *iface, DWORD data)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    return d3drm_frame3_SetAppData(&frame->IDirect3DRMFrame3_iface, data);
}

static HRESULT WINAPI d3drm_frame1_SetAppData(IDirect3DRMFrame *iface, DWORD data)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    return d3drm_frame3_SetAppData(&frame->IDirect3DRMFrame3_iface, data);
}

static DWORD WINAPI d3drm_frame3_GetAppData(IDirect3DRMFrame3 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p.\n", iface);

    return frame->obj.appdata;
}

static DWORD WINAPI d3drm_frame2_GetAppData(IDirect3DRMFrame2 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_frame3_GetAppData(&frame->IDirect3DRMFrame3_iface);
}

static DWORD WINAPI d3drm_frame1_GetAppData(IDirect3DRMFrame *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_frame3_GetAppData(&frame->IDirect3DRMFrame3_iface);
}

static HRESULT WINAPI d3drm_frame3_SetName(IDirect3DRMFrame3 *iface, const char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_object_set_name(&frame->obj, name);
}

static HRESULT WINAPI d3drm_frame2_SetName(IDirect3DRMFrame2 *iface, const char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_frame3_SetName(&frame->IDirect3DRMFrame3_iface, name);
}

static HRESULT WINAPI d3drm_frame1_SetName(IDirect3DRMFrame *iface, const char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_frame3_SetName(&frame->IDirect3DRMFrame3_iface, name);
}

static HRESULT WINAPI d3drm_frame3_GetName(IDirect3DRMFrame3 *iface, DWORD *size, char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_name(&frame->obj, size, name);
}

static HRESULT WINAPI d3drm_frame2_GetName(IDirect3DRMFrame2 *iface, DWORD *size, char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_frame3_GetName(&frame->IDirect3DRMFrame3_iface, size, name);
}

static HRESULT WINAPI d3drm_frame1_GetName(IDirect3DRMFrame *iface, DWORD *size, char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_frame3_GetName(&frame->IDirect3DRMFrame3_iface, size, name);
}

static HRESULT WINAPI d3drm_frame3_GetClassName(IDirect3DRMFrame3 *iface, DWORD *size, char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_class_name(&frame->obj, size, name);
}

static HRESULT WINAPI d3drm_frame2_GetClassName(IDirect3DRMFrame2 *iface, DWORD *size, char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_frame3_GetClassName(&frame->IDirect3DRMFrame3_iface, size, name);
}

static HRESULT WINAPI d3drm_frame1_GetClassName(IDirect3DRMFrame *iface, DWORD *size, char *name)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_frame3_GetClassName(&frame->IDirect3DRMFrame3_iface, size, name);
}

static HRESULT WINAPI d3drm_frame3_AddChild(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 *child)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
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

    if (!d3drm_array_reserve((void **)&frame->children, &frame->children_size,
            frame->nb_children + 1, sizeof(*frame->children)))
        return E_OUTOFMEMORY;

    frame->children[frame->nb_children++] = child;
    IDirect3DRMFrame3_AddRef(child);
    child_obj->parent = frame;

    return D3DRM_OK;
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

    return d3drm_frame3_AddChild(&frame->IDirect3DRMFrame3_iface, child3);
}

static HRESULT WINAPI d3drm_frame1_AddChild(IDirect3DRMFrame *iface, IDirect3DRMFrame *child)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);
    struct d3drm_frame *child_frame = unsafe_impl_from_IDirect3DRMFrame(child);

    TRACE("iface %p, child %p.\n", iface, child);

    if (!child_frame)
        return D3DRMERR_BADOBJECT;

    return d3drm_frame3_AddChild(&frame->IDirect3DRMFrame3_iface, &child_frame->IDirect3DRMFrame3_iface);
}

static HRESULT WINAPI d3drm_frame3_AddLight(IDirect3DRMFrame3 *iface, IDirect3DRMLight *light)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    ULONG i;

    TRACE("iface %p, light %p.\n", iface, light);

    if (!light)
        return D3DRMERR_BADOBJECT;

    /* Check if already existing and return gracefully without increasing ref count */
    for (i = 0; i < frame->nb_lights; i++)
        if (frame->lights[i] == light)
            return D3DRM_OK;

    if (!d3drm_array_reserve((void **)&frame->lights, &frame->lights_size,
            frame->nb_lights + 1, sizeof(*frame->lights)))
        return E_OUTOFMEMORY;

    frame->lights[frame->nb_lights++] = light;
    IDirect3DRMLight_AddRef(light);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_AddLight(IDirect3DRMFrame2 *iface, IDirect3DRMLight *light)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, light %p.\n", iface, light);

    return d3drm_frame3_AddLight(&frame->IDirect3DRMFrame3_iface, light);
}

static HRESULT WINAPI d3drm_frame1_AddLight(IDirect3DRMFrame *iface, IDirect3DRMLight *light)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, light %p.\n", iface, light);

    return d3drm_frame3_AddLight(&frame->IDirect3DRMFrame3_iface, light);
}

static HRESULT WINAPI d3drm_frame3_AddMoveCallback(IDirect3DRMFrame3 *iface,
        D3DRMFRAME3MOVECALLBACK cb, void *ctx, DWORD flags)
{
    FIXME("iface %p, cb %p, ctx %p flags %#lx stub!\n", iface, cb, ctx, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_AddMoveCallback(IDirect3DRMFrame2 *iface,
        D3DRMFRAMEMOVECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_AddMoveCallback(IDirect3DRMFrame *iface,
        D3DRMFRAMEMOVECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_AddTransform(IDirect3DRMFrame3 *iface,
        D3DRMCOMBINETYPE type, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    const struct d3drm_matrix *m = d3drm_matrix(matrix);

    TRACE("iface %p, type %#x, matrix %p.\n", iface, type, matrix);

    if (m->_14 != 0.0f || m->_24 != 0.0f || m->_34 != 0.0f || m->_44 != 1.0f)
        return D3DRMERR_BADVALUE;

    switch (type)
    {
        case D3DRMCOMBINE_REPLACE:
            frame->transform = *m;
            break;

        case D3DRMCOMBINE_BEFORE:
            d3drm_matrix_multiply_affine(&frame->transform, m, &frame->transform);
            break;

        case D3DRMCOMBINE_AFTER:
            d3drm_matrix_multiply_affine(&frame->transform, &frame->transform, m);
            break;

        default:
            FIXME("Unhandled type %#x.\n", type);
            return D3DRMERR_BADVALUE;
    }

    return S_OK;
}

static HRESULT WINAPI d3drm_frame2_AddTransform(IDirect3DRMFrame2 *iface,
        D3DRMCOMBINETYPE type, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, type %#x, matrix %p.\n", iface, type, matrix);

    return d3drm_frame3_AddTransform(&frame->IDirect3DRMFrame3_iface, type, matrix);
}

static HRESULT WINAPI d3drm_frame1_AddTransform(IDirect3DRMFrame *iface,
        D3DRMCOMBINETYPE type, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, type %#x, matrix %p.\n", iface, type, matrix);

    return d3drm_frame3_AddTransform(&frame->IDirect3DRMFrame3_iface, type, matrix);
}

static HRESULT WINAPI d3drm_frame3_AddTranslation(IDirect3DRMFrame3 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, type %#x, x %.8e, y %.8e, z %.8e.\n", iface, type, x, y, z);

    switch (type)
    {
        case D3DRMCOMBINE_REPLACE:
            frame->transform = identity;
            frame->transform._41 = x;
            frame->transform._42 = y;
            frame->transform._43 = z;
            break;

        case D3DRMCOMBINE_BEFORE:
            frame->transform._41 += x * frame->transform._11 + y * frame->transform._21 + z * frame->transform._31;
            frame->transform._42 += x * frame->transform._12 + y * frame->transform._22 + z * frame->transform._32;
            frame->transform._43 += x * frame->transform._13 + y * frame->transform._23 + z * frame->transform._33;
            break;

        case D3DRMCOMBINE_AFTER:
            frame->transform._41 += x;
            frame->transform._42 += y;
            frame->transform._43 += z;
            break;

        default:
            FIXME("Unhandled type %#x.\n", type);
            return D3DRMERR_BADVALUE;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_AddTranslation(IDirect3DRMFrame2 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, type %#x, x %.8e, y %.8e, z %.8e.\n", iface, type, x, y, z);

    return d3drm_frame3_AddTranslation(&frame->IDirect3DRMFrame3_iface, type, x, y, z);
}

static HRESULT WINAPI d3drm_frame1_AddTranslation(IDirect3DRMFrame *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, type %#x, x %.8e, y %.8e, z %.8e.\n", iface, type, x, y, z);

    return d3drm_frame3_AddTranslation(&frame->IDirect3DRMFrame3_iface, type, x, y, z);
}

static HRESULT WINAPI d3drm_frame3_AddScale(IDirect3DRMFrame3 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, type %#x, sx %.8e, sy %.8e, sz %.8e.\n", iface, type, sx, sy, sz);

    switch (type)
    {
        case D3DRMCOMBINE_REPLACE:
            frame->transform = identity;
            frame->transform._11 = sx;
            frame->transform._22 = sy;
            frame->transform._33 = sz;
            break;

        case D3DRMCOMBINE_BEFORE:
            frame->transform._11 *= sx;
            frame->transform._12 *= sx;
            frame->transform._13 *= sx;
            frame->transform._21 *= sy;
            frame->transform._22 *= sy;
            frame->transform._23 *= sy;
            frame->transform._31 *= sz;
            frame->transform._32 *= sz;
            frame->transform._33 *= sz;
            break;

        case D3DRMCOMBINE_AFTER:
            frame->transform._11 *= sx;
            frame->transform._12 *= sy;
            frame->transform._13 *= sz;
            frame->transform._21 *= sx;
            frame->transform._22 *= sy;
            frame->transform._23 *= sz;
            frame->transform._31 *= sx;
            frame->transform._32 *= sy;
            frame->transform._33 *= sz;
            frame->transform._41 *= sx;
            frame->transform._42 *= sy;
            frame->transform._43 *= sz;
            break;

        default:
            FIXME("Unhandled type %#x.\n", type);
            return D3DRMERR_BADVALUE;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_AddScale(IDirect3DRMFrame2 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, type %#x, sx %.8e, sy %.8e, sz %.8e.\n", iface, type, sx, sy, sz);

    return d3drm_frame3_AddScale(&frame->IDirect3DRMFrame3_iface, type, sx, sy, sz);
}

static HRESULT WINAPI d3drm_frame1_AddScale(IDirect3DRMFrame *iface,
        D3DRMCOMBINETYPE type, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, type %#x, sx %.8e, sy %.8e, sz %.8e.\n", iface, type, sx, sy, sz);

    return d3drm_frame3_AddScale(&frame->IDirect3DRMFrame3_iface, type, sx, sy, sz);
}

static HRESULT WINAPI d3drm_frame3_AddRotation(IDirect3DRMFrame3 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    struct d3drm_matrix m;
    D3DVECTOR axis;

    TRACE("iface %p, type %#x, x %.8e, y %.8e, z %.8e, theta %.8e.\n", iface, type, x, y, z, theta);

    axis.x = x;
    axis.y = y;
    axis.z = z;

    switch (type)
    {
        case D3DRMCOMBINE_REPLACE:
            d3drm_matrix_set_rotation(&frame->transform, &axis, theta);
            break;

        case D3DRMCOMBINE_BEFORE:
            d3drm_matrix_set_rotation(&m, &axis, theta);
            d3drm_matrix_multiply_affine(&frame->transform, &m, &frame->transform);
            break;

        case D3DRMCOMBINE_AFTER:
            d3drm_matrix_set_rotation(&m, &axis, theta);
            d3drm_matrix_multiply_affine(&frame->transform, &frame->transform, &m);
            break;

        default:
            FIXME("Unhandled type %#x.\n", type);
            return D3DRMERR_BADVALUE;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_AddRotation(IDirect3DRMFrame2 *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, type %#x, x %.8e, y %.8e, z %.8e, theta %.8e.\n", iface, type, x, y, z, theta);

    return d3drm_frame3_AddRotation(&frame->IDirect3DRMFrame3_iface, type, x, y, z, theta);
}

static HRESULT WINAPI d3drm_frame1_AddRotation(IDirect3DRMFrame *iface,
        D3DRMCOMBINETYPE type, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, type %#x, x %.8e, y %.8e, z %.8e, theta %.8e.\n", iface, type, x, y, z, theta);

    return d3drm_frame3_AddRotation(&frame->IDirect3DRMFrame3_iface, type, x, y, z, theta);
}

static HRESULT WINAPI d3drm_frame3_AddVisual(IDirect3DRMFrame3 *iface, IUnknown *visual)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    ULONG i;

    TRACE("iface %p, visual %p.\n", iface, visual);

    if (!visual)
        return D3DRMERR_BADOBJECT;

    /* Check if already existing and return gracefully without increasing ref count */
    for (i = 0; i < frame->nb_visuals; i++)
        if (frame->visuals[i] == (IDirect3DRMVisual *)visual)
            return D3DRM_OK;

    if (!d3drm_array_reserve((void **)&frame->visuals, &frame->visuals_size,
            frame->nb_visuals + 1, sizeof(*frame->visuals)))
        return E_OUTOFMEMORY;

    frame->visuals[frame->nb_visuals++] = (IDirect3DRMVisual *)visual;
    IDirect3DRMVisual_AddRef(visual);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_AddVisual(IDirect3DRMFrame2 *iface, IDirect3DRMVisual *visual)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, visual %p.\n", iface, visual);

    return d3drm_frame3_AddVisual(&frame->IDirect3DRMFrame3_iface, (IUnknown *)visual);
}

static HRESULT WINAPI d3drm_frame1_AddVisual(IDirect3DRMFrame *iface, IDirect3DRMVisual *visual)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, visual %p.\n", iface, visual);

    return d3drm_frame3_AddVisual(&frame->IDirect3DRMFrame3_iface, (IUnknown *)visual);
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

static HRESULT WINAPI d3drm_frame2_GetChildren(IDirect3DRMFrame2 *iface, IDirect3DRMFrameArray **children)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, children %p.\n", iface, children);

    return d3drm_frame3_GetChildren(&frame->IDirect3DRMFrame3_iface, children);
}

static HRESULT WINAPI d3drm_frame1_GetChildren(IDirect3DRMFrame *iface, IDirect3DRMFrameArray **children)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, children %p.\n", iface, children);

    return d3drm_frame3_GetChildren(&frame->IDirect3DRMFrame3_iface, children);
}

static D3DCOLOR WINAPI d3drm_frame3_GetColor(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static D3DCOLOR WINAPI d3drm_frame2_GetColor(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static D3DCOLOR WINAPI d3drm_frame1_GetColor(IDirect3DRMFrame *iface)
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

static HRESULT WINAPI d3drm_frame2_GetLights(IDirect3DRMFrame2 *iface, IDirect3DRMLightArray **lights)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, lights %p.\n", iface, lights);

    return d3drm_frame3_GetLights(&frame->IDirect3DRMFrame3_iface, lights);
}

static HRESULT WINAPI d3drm_frame1_GetLights(IDirect3DRMFrame *iface, IDirect3DRMLightArray **lights)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, lights %p.\n", iface, lights);

    return d3drm_frame3_GetLights(&frame->IDirect3DRMFrame3_iface, lights);
}

static D3DRMMATERIALMODE WINAPI d3drm_frame3_GetMaterialMode(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMMATERIAL_FROMPARENT;
}

static D3DRMMATERIALMODE WINAPI d3drm_frame2_GetMaterialMode(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMMATERIAL_FROMPARENT;
}

static D3DRMMATERIALMODE WINAPI d3drm_frame1_GetMaterialMode(IDirect3DRMFrame *iface)
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

static HRESULT WINAPI d3drm_frame2_GetParent(IDirect3DRMFrame2 *iface, IDirect3DRMFrame **parent)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, parent %p.\n", iface, parent);

    if (!parent)
        return D3DRMERR_BADVALUE;

    if (frame->parent)
    {
        *parent = &frame->parent->IDirect3DRMFrame_iface;
        IDirect3DRMFrame_AddRef(*parent);
    }
    else
    {
        *parent = NULL;
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame1_GetParent(IDirect3DRMFrame *iface, IDirect3DRMFrame **parent)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, parent %p.\n", iface, parent);

    return d3drm_frame2_GetParent(&frame->IDirect3DRMFrame2_iface, parent);
}

static HRESULT WINAPI d3drm_frame3_GetPosition(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVECTOR *position)
{
    FIXME("iface %p, reference %p, position %p stub!\n", iface, reference, position);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetPosition(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *position)
{
    FIXME("iface %p, reference %p, position %p stub!\n", iface, reference, position);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_GetPosition(IDirect3DRMFrame *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *position)
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

static HRESULT WINAPI d3drm_frame2_GetRotation(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *axis, D3DVALUE *theta)
{
    FIXME("iface %p, reference %p, axis %p, theta %p stub!\n", iface, reference, axis, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_GetRotation(IDirect3DRMFrame *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *axis, D3DVALUE *theta)
{
    FIXME("iface %p, reference %p, axis %p, theta %p stub!\n", iface, reference, axis, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetScene(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 **scene)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, scene %p.\n", iface, scene);

    if (!scene)
        return D3DRMERR_BADVALUE;

    while (frame->parent)
        frame = frame->parent;

    *scene = &frame->IDirect3DRMFrame3_iface;
    IDirect3DRMFrame3_AddRef(*scene);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_GetScene(IDirect3DRMFrame2 *iface, IDirect3DRMFrame **scene)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);
    IDirect3DRMFrame3 *frame3;
    HRESULT hr;

    TRACE("iface %p, scene %p.\n", iface, scene);

    if (!scene)
        return D3DRMERR_BADVALUE;

    hr = IDirect3DRMFrame3_GetScene(&frame->IDirect3DRMFrame3_iface, &frame3);
    if (FAILED(hr) || !frame3)
    {
        *scene = NULL;
        return hr;
    }

    hr = IDirect3DRMFrame3_QueryInterface(frame3, &IID_IDirect3DRMFrame, (void **)scene);
    IDirect3DRMFrame3_Release(frame3);

    return hr;
}

static HRESULT WINAPI d3drm_frame1_GetScene(IDirect3DRMFrame *iface, IDirect3DRMFrame **scene)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, scene %p.\n", iface, scene);

    return d3drm_frame2_GetScene(&frame->IDirect3DRMFrame2_iface, scene);
}

static D3DRMSORTMODE WINAPI d3drm_frame3_GetSortMode(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMSORT_FROMPARENT;
}

static D3DRMSORTMODE WINAPI d3drm_frame2_GetSortMode(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMSORT_FROMPARENT;
}

static D3DRMSORTMODE WINAPI d3drm_frame1_GetSortMode(IDirect3DRMFrame *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMSORT_FROMPARENT;
}

static HRESULT WINAPI d3drm_frame3_GetTexture(IDirect3DRMFrame3 *iface, IDirect3DRMTexture3 **texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetTexture(IDirect3DRMFrame2 *iface, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_GetTexture(IDirect3DRMFrame *iface, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetTransform(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);
    struct d3drm_matrix *m = d3drm_matrix(matrix);

    TRACE("iface %p, reference %p, matrix %p.\n", iface, reference, matrix);

    if (reference)
        FIXME("Ignoring reference frame %p.\n", reference);

    *m = frame->transform;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_GetTransform(IDirect3DRMFrame2 *iface, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);
    struct d3drm_matrix *m = d3drm_matrix(matrix);

    TRACE("iface %p, matrix %p.\n", iface, matrix);

    *m = frame->transform;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame1_GetTransform(IDirect3DRMFrame *iface, D3DRMMATRIX4D matrix)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, matrix %p.\n", iface, matrix);

    return d3drm_frame2_GetTransform(&frame->IDirect3DRMFrame2_iface, matrix);
}

static HRESULT WINAPI d3drm_frame3_GetVelocity(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVECTOR *velocity, BOOL with_rotation)
{
    FIXME("iface %p, reference %p, velocity %p, with_rotation %#x stub!\n",
            iface, reference, velocity, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetVelocity(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *velocity, BOOL with_rotation)
{
    FIXME("iface %p, reference %p, velocity %p, with_rotation %#x stub!\n",
            iface, reference, velocity, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_GetVelocity(IDirect3DRMFrame *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *velocity, BOOL with_rotation)
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

static HRESULT WINAPI d3drm_frame2_GetOrientation(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *dir, D3DVECTOR *up)
{
    FIXME("iface %p, reference %p, dir %p, up %p stub!\n", iface, reference, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_GetOrientation(IDirect3DRMFrame *iface,
        IDirect3DRMFrame *reference, D3DVECTOR *dir, D3DVECTOR *up)
{
    FIXME("iface %p, reference %p, dir %p, up %p stub!\n", iface, reference, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetVisuals(IDirect3DRMFrame3 *iface, DWORD *count, IUnknown **visuals)
{
    FIXME("iface %p, count %p, visuals %p stub!\n", iface, count, visuals);

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

static HRESULT WINAPI d3drm_frame1_GetVisuals(IDirect3DRMFrame *iface, IDirect3DRMVisualArray **visuals)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, visuals %p.\n", iface, visuals);

    return d3drm_frame2_GetVisuals(&frame->IDirect3DRMFrame2_iface, visuals);
}

static HRESULT WINAPI d3drm_frame2_GetTextureTopology(IDirect3DRMFrame2 *iface, BOOL *wrap_u, BOOL *wrap_v)
{
    FIXME("iface %p, wrap_u %p, wrap_v %p stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_GetTextureTopology(IDirect3DRMFrame *iface, BOOL *wrap_u, BOOL *wrap_v)
{
    FIXME("iface %p, wrap_u %p, wrap_v %p stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_InverseTransform(IDirect3DRMFrame3 *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_InverseTransform(IDirect3DRMFrame2 *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_InverseTransform(IDirect3DRMFrame *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    FIXME("iface %p, d %p, s %p stub!\n", iface, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_Load(IDirect3DRMFrame3 *iface, void *filename,
        void *name, D3DRMLOADOPTIONS flags, D3DRMLOADTEXTURE3CALLBACK cb, void *ctx)
{
    FIXME("iface %p, filename %p, name %p, flags %#lx, cb %p, ctx %p stub!\n",
            iface, filename, name, flags, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_Load(IDirect3DRMFrame2 *iface, void *filename,
        void *name, D3DRMLOADOPTIONS flags, D3DRMLOADTEXTURECALLBACK cb, void *ctx)
{
    FIXME("iface %p, filename %p, name %p, flags %#lx, cb %p, ctx %p stub!\n",
            iface, filename, name, flags, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_Load(IDirect3DRMFrame *iface, void *filename,
        void *name, D3DRMLOADOPTIONS flags, D3DRMLOADTEXTURECALLBACK cb, void *ctx)
{
    FIXME("iface %p, filename %p, name %p, flags %#lx, cb %p, ctx %p stub!\n",
            iface, filename, name, flags, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_LookAt(IDirect3DRMFrame3 *iface, IDirect3DRMFrame3 *target,
        IDirect3DRMFrame3 *reference, D3DRMFRAMECONSTRAINT constraint)
{
    FIXME("iface %p, target %p, reference %p, constraint %#x stub!\n", iface, target, reference, constraint);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_LookAt(IDirect3DRMFrame2 *iface, IDirect3DRMFrame *target,
        IDirect3DRMFrame *reference, D3DRMFRAMECONSTRAINT constraint)
{
    FIXME("iface %p, target %p, reference %p, constraint %#x stub!\n", iface, target, reference, constraint);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_LookAt(IDirect3DRMFrame *iface, IDirect3DRMFrame *target,
        IDirect3DRMFrame *reference, D3DRMFRAMECONSTRAINT constraint)
{
    FIXME("iface %p, target %p, reference %p, constraint %#x stub!\n", iface, target, reference, constraint);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_Move(IDirect3DRMFrame3 *iface, D3DVALUE delta)
{
    FIXME("iface %p, delta %.8e stub!\n", iface, delta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_Move(IDirect3DRMFrame2 *iface, D3DVALUE delta)
{
    FIXME("iface %p, delta %.8e stub!\n", iface, delta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_Move(IDirect3DRMFrame *iface, D3DVALUE delta)
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

    return d3drm_frame3_DeleteChild(&frame->IDirect3DRMFrame3_iface, child3);
}

static HRESULT WINAPI d3drm_frame1_DeleteChild(IDirect3DRMFrame *iface, IDirect3DRMFrame *child)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);
    struct d3drm_frame *child_frame = unsafe_impl_from_IDirect3DRMFrame(child);

    TRACE("iface %p, child %p.\n", iface, child);

    if (!child_frame)
        return D3DRMERR_BADOBJECT;

    return d3drm_frame3_DeleteChild(&frame->IDirect3DRMFrame3_iface, &child_frame->IDirect3DRMFrame3_iface);
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

static HRESULT WINAPI d3drm_frame2_DeleteLight(IDirect3DRMFrame2 *iface, IDirect3DRMLight *light)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, light %p.\n", iface, light);

    return d3drm_frame3_DeleteLight(&frame->IDirect3DRMFrame3_iface, light);
}

static HRESULT WINAPI d3drm_frame1_DeleteLight(IDirect3DRMFrame *iface, IDirect3DRMLight *light)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, light %p.\n", iface, light);

    return d3drm_frame3_DeleteLight(&frame->IDirect3DRMFrame3_iface, light);
}

static HRESULT WINAPI d3drm_frame3_DeleteMoveCallback(IDirect3DRMFrame3 *iface,
        D3DRMFRAME3MOVECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_DeleteMoveCallback(IDirect3DRMFrame2 *iface,
        D3DRMFRAMEMOVECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_DeleteMoveCallback(IDirect3DRMFrame *iface,
        D3DRMFRAMEMOVECALLBACK cb, void *ctx)
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

static HRESULT WINAPI d3drm_frame2_DeleteVisual(IDirect3DRMFrame2 *iface, IDirect3DRMVisual *visual)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, visual %p.\n", iface, visual);

    return d3drm_frame3_DeleteVisual(&frame->IDirect3DRMFrame3_iface, (IUnknown *)visual);
}

static HRESULT WINAPI d3drm_frame1_DeleteVisual(IDirect3DRMFrame *iface, IDirect3DRMVisual *visual)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, visual %p.\n", iface, visual);

    return d3drm_frame3_DeleteVisual(&frame->IDirect3DRMFrame3_iface, (IUnknown *)visual);
}

static D3DCOLOR WINAPI d3drm_frame3_GetSceneBackground(IDirect3DRMFrame3 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p.\n", iface);

    return frame->scenebackground;
}

static D3DCOLOR WINAPI d3drm_frame2_GetSceneBackground(IDirect3DRMFrame2 *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_frame3_GetSceneBackground(&frame->IDirect3DRMFrame3_iface);
}

static D3DCOLOR WINAPI d3drm_frame1_GetSceneBackground(IDirect3DRMFrame *iface)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_frame3_GetSceneBackground(&frame->IDirect3DRMFrame3_iface);
}

static HRESULT WINAPI d3drm_frame3_GetSceneBackgroundDepth(IDirect3DRMFrame3 *iface,
        IDirectDrawSurface **surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetSceneBackgroundDepth(IDirect3DRMFrame2 *iface,
        IDirectDrawSurface **surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_GetSceneBackgroundDepth(IDirect3DRMFrame *iface,
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

static D3DCOLOR WINAPI d3drm_frame2_GetSceneFogColor(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static D3DCOLOR WINAPI d3drm_frame1_GetSceneFogColor(IDirect3DRMFrame *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL WINAPI d3drm_frame3_GetSceneFogEnable(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static BOOL WINAPI d3drm_frame2_GetSceneFogEnable(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static BOOL WINAPI d3drm_frame1_GetSceneFogEnable(IDirect3DRMFrame *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static D3DRMFOGMODE WINAPI d3drm_frame3_GetSceneFogMode(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMFOG_LINEAR;
}

static D3DRMFOGMODE WINAPI d3drm_frame2_GetSceneFogMode(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMFOG_LINEAR;
}

static D3DRMFOGMODE WINAPI d3drm_frame1_GetSceneFogMode(IDirect3DRMFrame *iface)
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

static HRESULT WINAPI d3drm_frame2_GetSceneFogParams(IDirect3DRMFrame2 *iface,
        D3DVALUE *start, D3DVALUE *end, D3DVALUE *density)
{
    FIXME("iface %p, start %p, end %p, density %p stub!\n", iface, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_GetSceneFogParams(IDirect3DRMFrame *iface,
        D3DVALUE *start, D3DVALUE *end, D3DVALUE *density)
{
    FIXME("iface %p, start %p, end %p, density %p stub!\n", iface, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneBackground(IDirect3DRMFrame3 *iface, D3DCOLOR color)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, color 0x%08lx.\n", iface, color);

    frame->scenebackground = color;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_SetSceneBackground(IDirect3DRMFrame2 *iface, D3DCOLOR color)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, color 0x%08lx.\n", iface, color);

    return d3drm_frame3_SetSceneBackground(&frame->IDirect3DRMFrame3_iface, color);
}

static HRESULT WINAPI d3drm_frame1_SetSceneBackground(IDirect3DRMFrame *iface, D3DCOLOR color)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, color 0x%08lx.\n", iface, color);

    return d3drm_frame3_SetSceneBackground(&frame->IDirect3DRMFrame3_iface, color);
}

static HRESULT WINAPI d3drm_frame3_SetSceneBackgroundRGB(IDirect3DRMFrame3 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e.\n", iface, red, green, blue);

    d3drm_set_color(&frame->scenebackground, red, green, blue, 1.0f);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_SetSceneBackgroundRGB(IDirect3DRMFrame2 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e.\n", iface, red, green, blue);

    return d3drm_frame3_SetSceneBackgroundRGB(&frame->IDirect3DRMFrame3_iface, red, green, blue);
}

static HRESULT WINAPI d3drm_frame1_SetSceneBackgroundRGB(IDirect3DRMFrame *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, red %.8e, green %.8e, blue %.8e.\n", iface, red, green, blue);

    return d3drm_frame3_SetSceneBackgroundRGB(&frame->IDirect3DRMFrame3_iface, red, green, blue);
}

static HRESULT WINAPI d3drm_frame3_SetSceneBackgroundDepth(IDirect3DRMFrame3 *iface,
        IDirectDrawSurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneBackgroundDepth(IDirect3DRMFrame2 *iface,
        IDirectDrawSurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetSceneBackgroundDepth(IDirect3DRMFrame *iface,
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

static HRESULT WINAPI d3drm_frame2_SetSceneBackgroundImage(IDirect3DRMFrame2 *iface,
        IDirect3DRMTexture *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetSceneBackgroundImage(IDirect3DRMFrame *iface,
        IDirect3DRMTexture *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneFogEnable(IDirect3DRMFrame3 *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneFogEnable(IDirect3DRMFrame2 *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetSceneFogEnable(IDirect3DRMFrame *iface, BOOL enable)
{
    FIXME("iface %p, enable %#x stub!\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneFogColor(IDirect3DRMFrame3 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08lx stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneFogColor(IDirect3DRMFrame2 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08lx stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetSceneFogColor(IDirect3DRMFrame *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08lx stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetSceneFogMode(IDirect3DRMFrame3 *iface, D3DRMFOGMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetSceneFogMode(IDirect3DRMFrame2 *iface, D3DRMFOGMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetSceneFogMode(IDirect3DRMFrame *iface, D3DRMFOGMODE mode)
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

static HRESULT WINAPI d3drm_frame2_SetSceneFogParams(IDirect3DRMFrame2 *iface,
        D3DVALUE start, D3DVALUE end, D3DVALUE density)
{
    FIXME("iface %p, start %.8e, end %.8e, density %.8e stub!\n", iface, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetSceneFogParams(IDirect3DRMFrame *iface,
        D3DVALUE start, D3DVALUE end, D3DVALUE density)
{
    FIXME("iface %p, start %.8e, end %.8e, density %.8e stub!\n", iface, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetColor(IDirect3DRMFrame3 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08lx stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetColor(IDirect3DRMFrame2 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08lx stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetColor(IDirect3DRMFrame *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08lx stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetColorRGB(IDirect3DRMFrame3 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    FIXME("iface %p, red %.8e, green %.8e, blue %.8e stub!\n", iface, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetColorRGB(IDirect3DRMFrame2 *iface,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    FIXME("iface %p, red %.8e, green %.8e, blue %.8e stub!\n", iface, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetColorRGB(IDirect3DRMFrame *iface,
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

static D3DRMZBUFFERMODE WINAPI d3drm_frame2_GetZbufferMode(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMZBUFFER_FROMPARENT;
}

static D3DRMZBUFFERMODE WINAPI d3drm_frame1_GetZbufferMode(IDirect3DRMFrame *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D3DRMZBUFFER_FROMPARENT;
}

static HRESULT WINAPI d3drm_frame3_SetMaterialMode(IDirect3DRMFrame3 *iface, D3DRMMATERIALMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetMaterialMode(IDirect3DRMFrame2 *iface, D3DRMMATERIALMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetMaterialMode(IDirect3DRMFrame *iface, D3DRMMATERIALMODE mode)
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

static HRESULT WINAPI d3drm_frame2_SetOrientation(IDirect3DRMFrame2 *iface, IDirect3DRMFrame *reference,
        D3DVALUE dx, D3DVALUE dy, D3DVALUE dz, D3DVALUE ux, D3DVALUE uy, D3DVALUE uz)
{
    FIXME("iface %p, reference %p, dx %.8e, dy %.8e, dz %.8e, ux %.8e, uy %.8e, uz %.8e stub!\n",
            iface, reference, dx, dy, dz, ux, uy, uz);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetOrientation(IDirect3DRMFrame *iface, IDirect3DRMFrame *reference,
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

static HRESULT WINAPI d3drm_frame2_SetPosition(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e stub!\n", iface, reference, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetPosition(IDirect3DRMFrame *iface,
        IDirect3DRMFrame *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z)
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

static HRESULT WINAPI d3drm_frame2_SetRotation(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e, theta %.8e stub!\n",
            iface, reference, x, y, z, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetRotation(IDirect3DRMFrame *iface,
        IDirect3DRMFrame *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
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

static HRESULT WINAPI d3drm_frame2_SetSortMode(IDirect3DRMFrame2 *iface, D3DRMSORTMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetSortMode(IDirect3DRMFrame *iface, D3DRMSORTMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetTexture(IDirect3DRMFrame3 *iface, IDirect3DRMTexture3 *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetTexture(IDirect3DRMFrame2 *iface, IDirect3DRMTexture *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetTexture(IDirect3DRMFrame *iface, IDirect3DRMTexture *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetTextureTopology(IDirect3DRMFrame2 *iface, BOOL wrap_u, BOOL wrap_v)
{
    FIXME("iface %p, wrap_u %#x, wrap_v %#x stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetTextureTopology(IDirect3DRMFrame *iface, BOOL wrap_u, BOOL wrap_v)
{
    FIXME("iface %p, wrap_u %#x, wrap_v %#x stub!\n", iface, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetVelocity(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e, with_rotation %#x.\n",
            iface, reference, x, y, z, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetVelocity(IDirect3DRMFrame2 *iface,
        IDirect3DRMFrame *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e, with_rotation %#x stub!\n",
            iface, reference, x, y, z, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetVelocity(IDirect3DRMFrame *iface,
        IDirect3DRMFrame *reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation)
{
    FIXME("iface %p, reference %p, x %.8e, y %.8e, z %.8e, with_rotation %#x stub!\n",
            iface, reference, x, y, z, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetZbufferMode(IDirect3DRMFrame3 *iface, D3DRMZBUFFERMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_SetZbufferMode(IDirect3DRMFrame2 *iface, D3DRMZBUFFERMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame1_SetZbufferMode(IDirect3DRMFrame *iface, D3DRMZBUFFERMODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_Transform(IDirect3DRMFrame3 *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, d %p, s %p.\n", iface, d, s);

    d3drm_vector_transform_affine(d, s, &frame->transform);
    while ((frame = frame->parent))
    {
        d3drm_vector_transform_affine(d, d, &frame->transform);
    }

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame2_Transform(IDirect3DRMFrame2 *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame2(iface);

    TRACE("iface %p, d %p, s %p.\n", iface, d, s);

    return d3drm_frame3_Transform(&frame->IDirect3DRMFrame3_iface, d, s);
}

static HRESULT WINAPI d3drm_frame1_Transform(IDirect3DRMFrame *iface, D3DVECTOR *d, D3DVECTOR *s)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame(iface);

    TRACE("iface %p, d %p, s %p.\n", iface, d, s);

    return d3drm_frame3_Transform(&frame->IDirect3DRMFrame3_iface, d, s);
}

static HRESULT WINAPI d3drm_frame2_AddMoveCallback2(IDirect3DRMFrame2 *iface,
        D3DRMFRAMEMOVECALLBACK cb, void *ctx, DWORD flags)
{
    FIXME("iface %p, cb %p, ctx %p, flags %#lx stub!\n", iface, cb, ctx, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetBox(IDirect3DRMFrame3 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetBox(IDirect3DRMFrame2 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static BOOL WINAPI d3drm_frame3_GetBoxEnable(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static BOOL WINAPI d3drm_frame2_GetBoxEnable(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT WINAPI d3drm_frame3_GetAxes(IDirect3DRMFrame3 *iface, D3DVECTOR *dir, D3DVECTOR *up)
{
    FIXME("iface %p, dir %p, up %p stub!\n", iface, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetAxes(IDirect3DRMFrame2 *iface, D3DVECTOR *dir, D3DVECTOR *up)
{
    FIXME("iface %p, dir %p, up %p stub!\n", iface, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_GetMaterial(IDirect3DRMFrame3 *iface, IDirect3DRMMaterial2 **material)
{
    FIXME("iface %p, material %p stub!\n", iface, material);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetMaterial(IDirect3DRMFrame2 *iface, IDirect3DRMMaterial **material)
{
    FIXME("iface %p, material %p stub!\n", iface, material);

    return E_NOTIMPL;
}

static BOOL WINAPI d3drm_frame3_GetInheritAxes(IDirect3DRMFrame3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static BOOL WINAPI d3drm_frame2_GetInheritAxes(IDirect3DRMFrame2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT WINAPI d3drm_frame3_GetHierarchyBox(IDirect3DRMFrame3 *iface, D3DRMBOX *box)
{
    FIXME("iface %p, box %p stub!\n", iface, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame2_GetHierarchyBox(IDirect3DRMFrame2 *iface, D3DRMBOX *box)
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
    FIXME("iface %p, reference %p, ray %p, flags %#lx, visuals %p stub!\n",
            iface, reference, ray, flags, visuals);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_Save(IDirect3DRMFrame3 *iface,
        const char *filename, D3DRMXOFFORMAT format, D3DRMSAVEOPTIONS flags)
{
    FIXME("iface %p, filename %s, format %#x, flags %#lx stub!\n",
            iface, debugstr_a(filename), format, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_TransformVectors(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, DWORD num, D3DVECTOR *dst, D3DVECTOR *src)
{
    FIXME("iface %p, reference %p, num %lu, dst %p, src %p stub!\n", iface, reference, num, dst, src);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_InverseTransformVectors(IDirect3DRMFrame3 *iface,
        IDirect3DRMFrame3 *reference, DWORD num, D3DVECTOR *dst, D3DVECTOR *src)
{
    FIXME("iface %p, reference %p, num %lu, dst %p, src %p stub!\n", iface, reference, num, dst, src);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_frame3_SetTraversalOptions(IDirect3DRMFrame3 *iface, DWORD options)
{
    static const DWORD supported_options = D3DRMFRAME_RENDERENABLE | D3DRMFRAME_PICKENABLE;
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, options %#lx.\n", iface, options);

    if (options & ~supported_options)
        return D3DRMERR_BADVALUE;

    frame->traversal_options = options;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_GetTraversalOptions(IDirect3DRMFrame3 *iface, DWORD *options)
{
    struct d3drm_frame *frame = impl_from_IDirect3DRMFrame3(iface);

    TRACE("iface %p, options %p.\n", iface, options);

    if (!options)
        return D3DRMERR_BADVALUE;

    *options = frame->traversal_options;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_frame3_SetSceneFogMethod(IDirect3DRMFrame3 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#lx stub!\n", iface, flags);

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

static const struct IDirect3DRMFrameVtbl d3drm_frame1_vtbl =
{
    d3drm_frame1_QueryInterface,
    d3drm_frame1_AddRef,
    d3drm_frame1_Release,
    d3drm_frame1_Clone,
    d3drm_frame1_AddDestroyCallback,
    d3drm_frame1_DeleteDestroyCallback,
    d3drm_frame1_SetAppData,
    d3drm_frame1_GetAppData,
    d3drm_frame1_SetName,
    d3drm_frame1_GetName,
    d3drm_frame1_GetClassName,
    d3drm_frame1_AddChild,
    d3drm_frame1_AddLight,
    d3drm_frame1_AddMoveCallback,
    d3drm_frame1_AddTransform,
    d3drm_frame1_AddTranslation,
    d3drm_frame1_AddScale,
    d3drm_frame1_AddRotation,
    d3drm_frame1_AddVisual,
    d3drm_frame1_GetChildren,
    d3drm_frame1_GetColor,
    d3drm_frame1_GetLights,
    d3drm_frame1_GetMaterialMode,
    d3drm_frame1_GetParent,
    d3drm_frame1_GetPosition,
    d3drm_frame1_GetRotation,
    d3drm_frame1_GetScene,
    d3drm_frame1_GetSortMode,
    d3drm_frame1_GetTexture,
    d3drm_frame1_GetTransform,
    d3drm_frame1_GetVelocity,
    d3drm_frame1_GetOrientation,
    d3drm_frame1_GetVisuals,
    d3drm_frame1_GetTextureTopology,
    d3drm_frame1_InverseTransform,
    d3drm_frame1_Load,
    d3drm_frame1_LookAt,
    d3drm_frame1_Move,
    d3drm_frame1_DeleteChild,
    d3drm_frame1_DeleteLight,
    d3drm_frame1_DeleteMoveCallback,
    d3drm_frame1_DeleteVisual,
    d3drm_frame1_GetSceneBackground,
    d3drm_frame1_GetSceneBackgroundDepth,
    d3drm_frame1_GetSceneFogColor,
    d3drm_frame1_GetSceneFogEnable,
    d3drm_frame1_GetSceneFogMode,
    d3drm_frame1_GetSceneFogParams,
    d3drm_frame1_SetSceneBackground,
    d3drm_frame1_SetSceneBackgroundRGB,
    d3drm_frame1_SetSceneBackgroundDepth,
    d3drm_frame1_SetSceneBackgroundImage,
    d3drm_frame1_SetSceneFogEnable,
    d3drm_frame1_SetSceneFogColor,
    d3drm_frame1_SetSceneFogMode,
    d3drm_frame1_SetSceneFogParams,
    d3drm_frame1_SetColor,
    d3drm_frame1_SetColorRGB,
    d3drm_frame1_GetZbufferMode,
    d3drm_frame1_SetMaterialMode,
    d3drm_frame1_SetOrientation,
    d3drm_frame1_SetPosition,
    d3drm_frame1_SetRotation,
    d3drm_frame1_SetSortMode,
    d3drm_frame1_SetTexture,
    d3drm_frame1_SetTextureTopology,
    d3drm_frame1_SetVelocity,
    d3drm_frame1_SetZbufferMode,
    d3drm_frame1_Transform,
};

struct d3drm_frame *unsafe_impl_from_IDirect3DRMFrame3(IDirect3DRMFrame3 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3drm_frame3_vtbl);

    return impl_from_IDirect3DRMFrame3(iface);
}

struct d3drm_frame *unsafe_impl_from_IDirect3DRMFrame(IDirect3DRMFrame *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3drm_frame1_vtbl);

    return impl_from_IDirect3DRMFrame(iface);
}

HRESULT d3drm_frame_create(struct d3drm_frame **frame, IUnknown *parent_frame, IDirect3DRM *d3drm)
{
    static const char classname[] = "Frame";
    struct d3drm_frame *object;
    HRESULT hr = D3DRM_OK;

    TRACE("frame %p, parent_frame %p, d3drm %p.\n", frame, parent_frame, d3drm);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMFrame_iface.lpVtbl = &d3drm_frame1_vtbl;
    object->IDirect3DRMFrame2_iface.lpVtbl = &d3drm_frame2_vtbl;
    object->IDirect3DRMFrame3_iface.lpVtbl = &d3drm_frame3_vtbl;
    object->d3drm = d3drm;
    object->ref = 1;
    d3drm_set_color(&object->scenebackground, 0.0f, 0.0f, 0.0f, 1.0f);
    object->traversal_options = D3DRMFRAME_RENDERENABLE | D3DRMFRAME_PICKENABLE;

    d3drm_object_init(&object->obj, classname);

    object->transform = identity;

    if (parent_frame)
    {
        IDirect3DRMFrame3 *p;

        if (FAILED(hr = IDirect3DRMFrame_QueryInterface(parent_frame, &IID_IDirect3DRMFrame3, (void **)&p)))
        {
            free(object);
            return hr;
        }
        IDirect3DRMFrame_Release(parent_frame);
        IDirect3DRMFrame3_AddChild(p, &object->IDirect3DRMFrame3_iface);
    }

    IDirect3DRM_AddRef(object->d3drm);

    *frame = object;

    return hr;
}

static HRESULT WINAPI d3drm_animation2_QueryInterface(IDirect3DRMAnimation2 *iface, REFIID riid, void **out)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMAnimation)
            || IsEqualGUID(riid, &IID_IDirect3DRMObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &animation->IDirect3DRMAnimation_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMAnimation2))
    {
        *out = &animation->IDirect3DRMAnimation2_iface;
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

static HRESULT WINAPI d3drm_animation1_QueryInterface(IDirect3DRMAnimation *iface, REFIID riid, void **out)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return IDirect3DRMAnimation2_QueryInterface(&animation->IDirect3DRMAnimation2_iface, riid, out);
}

static ULONG WINAPI d3drm_animation2_AddRef(IDirect3DRMAnimation2 *iface)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);
    ULONG refcount = InterlockedIncrement(&animation->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_animation1_AddRef(IDirect3DRMAnimation *iface)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);
    return IDirect3DRMAnimation2_AddRef(&animation->IDirect3DRMAnimation2_iface);
}

static ULONG WINAPI d3drm_animation2_Release(IDirect3DRMAnimation2 *iface)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);
    ULONG refcount = InterlockedDecrement(&animation->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d3drm_object_cleanup((IDirect3DRMObject *)&animation->IDirect3DRMAnimation_iface, &animation->obj);
        IDirect3DRM_Release(animation->d3drm);
        free(animation->rotate.keys);
        free(animation->scale.keys);
        free(animation->position.keys);
        free(animation);
    }

    return refcount;
}

static ULONG WINAPI d3drm_animation1_Release(IDirect3DRMAnimation *iface)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    return IDirect3DRMAnimation2_Release(&animation->IDirect3DRMAnimation2_iface);
}

static HRESULT WINAPI d3drm_animation2_Clone(IDirect3DRMAnimation2 *iface, IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_animation1_Clone(IDirect3DRMAnimation *iface, IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_animation2_AddDestroyCallback(IDirect3DRMAnimation2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_object_add_destroy_callback(&animation->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_animation1_AddDestroyCallback(IDirect3DRMAnimation *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return IDirect3DRMAnimation2_AddDestroyCallback(&animation->IDirect3DRMAnimation2_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_animation2_DeleteDestroyCallback(IDirect3DRMAnimation2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return d3drm_object_delete_destroy_callback(&animation->obj, cb, ctx);
}

static HRESULT WINAPI d3drm_animation1_DeleteDestroyCallback(IDirect3DRMAnimation *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, cb %p, ctx %p.\n", iface, cb, ctx);

    return IDirect3DRMAnimation2_DeleteDestroyCallback(&animation->IDirect3DRMAnimation2_iface, cb, ctx);
}

static HRESULT WINAPI d3drm_animation2_SetAppData(IDirect3DRMAnimation2 *iface, DWORD data)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    animation->obj.appdata = data;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_animation1_SetAppData(IDirect3DRMAnimation *iface, DWORD data)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, data %#lx.\n", iface, data);

    return d3drm_animation2_SetAppData(&animation->IDirect3DRMAnimation2_iface, data);
}

static DWORD WINAPI d3drm_animation2_GetAppData(IDirect3DRMAnimation2 *iface)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p.\n", iface);

    return animation->obj.appdata;
}

static DWORD WINAPI d3drm_animation1_GetAppData(IDirect3DRMAnimation *iface)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_animation2_GetAppData(&animation->IDirect3DRMAnimation2_iface);
}

static HRESULT WINAPI d3drm_animation2_SetName(IDirect3DRMAnimation2 *iface, const char *name)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_object_set_name(&animation->obj, name);
}

static HRESULT WINAPI d3drm_animation1_SetName(IDirect3DRMAnimation *iface, const char *name)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3drm_animation2_SetName(&animation->IDirect3DRMAnimation2_iface, name);
}

static HRESULT WINAPI d3drm_animation2_GetName(IDirect3DRMAnimation2 *iface, DWORD *size, char *name)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_name(&animation->obj, size, name);
}

static HRESULT WINAPI d3drm_animation1_GetName(IDirect3DRMAnimation *iface, DWORD *size, char *name)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_animation2_GetName(&animation->IDirect3DRMAnimation2_iface, size, name);
}

static HRESULT WINAPI d3drm_animation2_GetClassName(IDirect3DRMAnimation2 *iface, DWORD *size, char *name)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_object_get_class_name(&animation->obj, size, name);
}

static HRESULT WINAPI d3drm_animation1_GetClassName(IDirect3DRMAnimation *iface, DWORD *size, char *name)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return d3drm_animation2_GetClassName(&animation->IDirect3DRMAnimation2_iface, size, name);
}

static HRESULT WINAPI d3drm_animation2_SetOptions(IDirect3DRMAnimation2 *iface, D3DRMANIMATIONOPTIONS options)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);
    static const DWORD supported_options = D3DRMANIMATION_OPEN | D3DRMANIMATION_CLOSED | D3DRMANIMATION_LINEARPOSITION
        | D3DRMANIMATION_SPLINEPOSITION | D3DRMANIMATION_SCALEANDROTATION | D3DRMANIMATION_POSITION;

    TRACE("iface %p, options %#lx.\n", iface, options);

    if (!(options & supported_options))
        return D3DRMERR_BADVALUE;

    if ((options & (D3DRMANIMATION_OPEN | D3DRMANIMATION_CLOSED)) == (D3DRMANIMATION_OPEN | D3DRMANIMATION_CLOSED) ||
            (options & (D3DRMANIMATION_LINEARPOSITION | D3DRMANIMATION_SPLINEPOSITION)) ==
            (D3DRMANIMATION_LINEARPOSITION | D3DRMANIMATION_SPLINEPOSITION) ||
            (options & (D3DRMANIMATION_SCALEANDROTATION | D3DRMANIMATION_POSITION)) ==
            (D3DRMANIMATION_SCALEANDROTATION | D3DRMANIMATION_POSITION))
    {
        return D3DRMERR_BADVALUE;
    }

    animation->options = options;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_animation1_SetOptions(IDirect3DRMAnimation *iface, D3DRMANIMATIONOPTIONS options)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, %#lx.\n", iface, options);

    return d3drm_animation2_SetOptions(&animation->IDirect3DRMAnimation2_iface, options);
}

static SIZE_T d3drm_animation_lookup_key(const struct d3drm_animation_key *keys,
        SIZE_T count, D3DVALUE time)
{
    SIZE_T start = 0, cur = 0, end = count;

    while (start < end)
    {
        cur = start + (end - start) / 2;

        if (time == keys[cur].time)
            return cur;

        if (time < keys[cur].time)
            end = cur;
        else
            start = cur + 1;
    }

    return cur;
}

static SIZE_T d3drm_animation_get_index_min(const struct d3drm_animation_key *keys, SIZE_T count, D3DVALUE time)
{
    SIZE_T i;

    i = d3drm_animation_lookup_key(keys, count, time);
    while (i > 0 && keys[i - 1].time == time)
        --i;

    return i;
}

static SIZE_T d3drm_animation_get_index_max(const struct d3drm_animation_key *keys, SIZE_T count, D3DVALUE time)
{
    SIZE_T i;

    i = d3drm_animation_lookup_key(keys, count, time);
    while (i < count - 1 && keys[i + 1].time == time)
        ++i;

    return i;
}

static SIZE_T d3drm_animation_get_insert_position(const struct d3drm_animation_keys *keys, D3DVALUE time)
{
    if (!keys->count || time < keys->keys[0].time)
        return 0;

    if (time >= keys->keys[keys->count - 1].time)
        return keys->count;

    return d3drm_animation_get_index_max(keys->keys, keys->count, time);
}

static const struct d3drm_animation_key *d3drm_animation_get_range(const struct d3drm_animation_keys *keys,
        D3DVALUE time_min, D3DVALUE time_max, SIZE_T *count)
{
    SIZE_T min;

    if (!keys->count || time_max < keys->keys[0].time
            || time_min > keys->keys[keys->count - 1].time)
        return NULL;

    min = d3drm_animation_get_index_min(keys->keys, keys->count, time_min);
    if (count)
        *count = d3drm_animation_get_index_max(&keys->keys[min], keys->count - min, time_max) - min + 1;

    return &keys->keys[min];
}

static HRESULT WINAPI d3drm_animation2_AddKey(IDirect3DRMAnimation2 *iface, D3DRMANIMATIONKEY *key)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);
    struct d3drm_animation_keys *keys;
    SIZE_T index;

    TRACE("iface %p, key %p.\n", iface, key);

    if (!key || key->dwSize != sizeof(*key))
        return E_INVALIDARG;

    switch (key->dwKeyType)
    {
        case D3DRMANIMATION_POSITIONKEY:
            keys = &animation->position;
            break;
        case D3DRMANIMATION_SCALEKEY:
            keys = &animation->scale;
            break;
        case D3DRMANIMATION_ROTATEKEY:
            keys = &animation->rotate;
            break;
        default:
            return E_INVALIDARG;
    }

    index = d3drm_animation_get_insert_position(keys, key->dvTime);

    if (!d3drm_array_reserve((void **)&keys->keys, &keys->size, keys->count + 1, sizeof(*keys->keys)))
        return E_OUTOFMEMORY;

    if (index < keys->count)
        memmove(&keys->keys[index + 1], &keys->keys[index], sizeof(*keys->keys) * (keys->count - index));
    keys->keys[index].time = key->dvTime;
    switch (key->dwKeyType)
    {
        case D3DRMANIMATION_POSITIONKEY:
            keys->keys[index].u.position = key->dvPositionKey;
            break;
        case D3DRMANIMATION_SCALEKEY:
            keys->keys[index].u.scale = key->dvScaleKey;
            break;
        case D3DRMANIMATION_ROTATEKEY:
            keys->keys[index].u.rotate = key->dqRotateKey;
            break;
    }
    ++keys->count;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_animation2_AddRotateKey(IDirect3DRMAnimation2 *iface, D3DVALUE time, D3DRMQUATERNION *q)
{
    D3DRMANIMATIONKEY key;

    TRACE("iface %p, time %.8e, q %p.\n", iface, time, q);

    key.dwSize = sizeof(key);
    key.dwKeyType = D3DRMANIMATION_ROTATEKEY;
    key.dvTime = time;
    key.dwID = 0;
    key.dqRotateKey = *q;

    return d3drm_animation2_AddKey(iface, &key);
}

static HRESULT WINAPI d3drm_animation1_AddRotateKey(IDirect3DRMAnimation *iface, D3DVALUE time, D3DRMQUATERNION *q)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, time %.8e, q %p.\n", iface, time, q);

    return d3drm_animation2_AddRotateKey(&animation->IDirect3DRMAnimation2_iface, time, q);
}

static HRESULT WINAPI d3drm_animation2_AddPositionKey(IDirect3DRMAnimation2 *iface, D3DVALUE time,
        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    D3DRMANIMATIONKEY key;

    TRACE("iface %p, time %.8e, x %.8e, y %.8e, z %.8e.\n", iface, time, x, y, z);

    key.dwSize = sizeof(key);
    key.dwKeyType = D3DRMANIMATION_POSITIONKEY;
    key.dvTime = time;
    key.dwID = 0;
    key.dvPositionKey.x = x;
    key.dvPositionKey.y = y;
    key.dvPositionKey.z = z;

    return d3drm_animation2_AddKey(iface, &key);
}

static HRESULT WINAPI d3drm_animation1_AddPositionKey(IDirect3DRMAnimation *iface, D3DVALUE time,
        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, time %.8e, x %.8e, y %.8e, z %.8e.\n", iface, time, x, y, z);

    return d3drm_animation2_AddPositionKey(&animation->IDirect3DRMAnimation2_iface, time, x, y, z);
}

static HRESULT WINAPI d3drm_animation2_AddScaleKey(IDirect3DRMAnimation2 *iface, D3DVALUE time,
        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    D3DRMANIMATIONKEY key;

    TRACE("iface %p, time %.8e, x %.8e, y %.8e, z %.8e.\n", iface, time, x, y, z);

    key.dwSize = sizeof(key);
    key.dwKeyType = D3DRMANIMATION_SCALEKEY;
    key.dvTime = time;
    key.dwID = 0;
    key.dvScaleKey.x = x;
    key.dvScaleKey.y = y;
    key.dvScaleKey.z = z;

    return d3drm_animation2_AddKey(iface, &key);
}

static HRESULT WINAPI d3drm_animation1_AddScaleKey(IDirect3DRMAnimation *iface, D3DVALUE time,
        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, time %.8e, x %.8e, y %.8e, z %.8e.\n", iface, time, x, y, z);

    return d3drm_animation2_AddScaleKey(&animation->IDirect3DRMAnimation2_iface, time, x, y, z);
}

static void d3drm_animation_delete_key(struct d3drm_animation_keys *keys, const struct d3drm_animation_key *key)
{
    SIZE_T index = key - keys->keys;

    if (index < keys->count - 1)
        memmove(&keys->keys[index], &keys->keys[index + 1], sizeof(*keys->keys) * (keys->count - index - 1));
    --keys->count;
}

static HRESULT WINAPI d3drm_animation2_DeleteKey(IDirect3DRMAnimation2 *iface, D3DVALUE time)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);
    const struct d3drm_animation_key *key;

    TRACE("iface %p, time %.8e.\n", iface, time);

    if ((key = d3drm_animation_get_range(&animation->rotate, time, time, NULL)))
        d3drm_animation_delete_key(&animation->rotate, key);

    if ((key = d3drm_animation_get_range(&animation->position, time, time, NULL)))
        d3drm_animation_delete_key(&animation->position, key);

    if ((key = d3drm_animation_get_range(&animation->scale, time, time, NULL)))
        d3drm_animation_delete_key(&animation->scale, key);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_animation1_DeleteKey(IDirect3DRMAnimation *iface, D3DVALUE time)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p, time %.8e.\n", iface, time);

    return d3drm_animation2_DeleteKey(&animation->IDirect3DRMAnimation2_iface, time);
}

static HRESULT WINAPI d3drm_animation1_SetFrame(IDirect3DRMAnimation *iface, IDirect3DRMFrame *frame)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);
    HRESULT hr = D3DRM_OK;

    TRACE("iface %p, frame %p.\n", iface, frame);

    if (frame)
    {
        hr = IDirect3DRMFrame_QueryInterface(frame, &IID_IDirect3DRMFrame3, (void **)&animation->frame);
        if (SUCCEEDED(hr))
            IDirect3DRMFrame3_Release(animation->frame);
    }
    else
        animation->frame = NULL;

    return hr;
}

static HRESULT WINAPI d3drm_animation1_SetTime(IDirect3DRMAnimation *iface, D3DVALUE time)
{
    FIXME("iface %p, time %.8e.\n", iface, time);

    return E_NOTIMPL;
}

static D3DRMANIMATIONOPTIONS WINAPI d3drm_animation2_GetOptions(IDirect3DRMAnimation2 *iface)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p.\n", iface);

    return animation->options;
}

static D3DRMANIMATIONOPTIONS WINAPI d3drm_animation1_GetOptions(IDirect3DRMAnimation *iface)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation(iface);

    TRACE("iface %p.\n", iface);

    return d3drm_animation2_GetOptions(&animation->IDirect3DRMAnimation2_iface);
}

static HRESULT WINAPI d3drm_animation2_SetFrame(IDirect3DRMAnimation2 *iface, IDirect3DRMFrame3 *frame)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p, frame %p.\n", iface, frame);

    animation->frame = frame;

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_animation2_SetTime(IDirect3DRMAnimation2 *iface, D3DVALUE time)
{
    FIXME("iface %p, time %.8e.\n", iface, time);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_animation2_GetFrame(IDirect3DRMAnimation2 *iface, IDirect3DRMFrame3 **frame)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);

    TRACE("iface %p, frame %p.\n", iface, frame);

    if (!frame)
        return D3DRMERR_BADVALUE;

    *frame = animation->frame;
    if (*frame)
        IDirect3DRMFrame3_AddRef(*frame);

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_animation2_DeleteKeyByID(IDirect3DRMAnimation2 *iface, DWORD id)
{
    FIXME("iface %p, id %#lx.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_animation2_ModifyKey(IDirect3DRMAnimation2 *iface, D3DRMANIMATIONKEY *key)
{
    FIXME("iface %p, key %p.\n", iface, key);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_animation2_GetKeys(IDirect3DRMAnimation2 *iface, D3DVALUE time_min, D3DVALUE time_max,
        DWORD *key_count, D3DRMANIMATIONKEY *keys)
{
    struct d3drm_animation *animation = impl_from_IDirect3DRMAnimation2(iface);
    const struct d3drm_animation_key *key;
    SIZE_T count, i;

    TRACE("iface %p, time min %.8e, time max %.8e, key_count %p, keys %p.\n",
            iface, time_min, time_max, key_count, keys);

    if (!key_count)
        return D3DRMERR_BADVALUE;

    *key_count = 0;

    if ((key = d3drm_animation_get_range(&animation->rotate, time_min, time_max, &count)))
    {
        if (keys)
        {
            for (i = 0; i < count; ++i)
            {
                keys[i].dwSize = sizeof(*keys);
                keys[i].dwKeyType = D3DRMANIMATION_ROTATEKEY;
                keys[i].dvTime = key[i].time;
                keys[i].dwID = 0; /* FIXME */
                keys[i].dqRotateKey = key[i].u.rotate;
            }
            keys += count;
        }
        *key_count += count;
    }

    if ((key = d3drm_animation_get_range(&animation->position, time_min, time_max, &count)))
    {
        if (keys)
        {
            for (i = 0; i < count; ++i)
            {
                keys[i].dwSize = sizeof(*keys);
                keys[i].dwKeyType = D3DRMANIMATION_POSITIONKEY;
                keys[i].dvTime = key[i].time;
                keys[i].dwID = 0; /* FIXME */
                keys[i].dvPositionKey = key[i].u.position;
            }
            keys += count;
        }
        *key_count += count;
    }

    if ((key = d3drm_animation_get_range(&animation->scale, time_min, time_max, &count)))
    {
        if (keys)
        {
            for (i = 0; keys && i < count; ++i)
            {
                keys[i].dwSize = sizeof(*keys);
                keys[i].dwKeyType = D3DRMANIMATION_SCALEKEY;
                keys[i].dvTime = key[i].time;
                keys[i].dwID = 0; /* FIXME */
                keys[i].dvScaleKey = key[i].u.scale;
            }
            keys += count;
        }
        *key_count += count;
    }

    return *key_count ? D3DRM_OK : D3DRMERR_NOSUCHKEY;
}

static const struct IDirect3DRMAnimationVtbl d3drm_animation1_vtbl =
{
    d3drm_animation1_QueryInterface,
    d3drm_animation1_AddRef,
    d3drm_animation1_Release,
    d3drm_animation1_Clone,
    d3drm_animation1_AddDestroyCallback,
    d3drm_animation1_DeleteDestroyCallback,
    d3drm_animation1_SetAppData,
    d3drm_animation1_GetAppData,
    d3drm_animation1_SetName,
    d3drm_animation1_GetName,
    d3drm_animation1_GetClassName,
    d3drm_animation1_SetOptions,
    d3drm_animation1_AddRotateKey,
    d3drm_animation1_AddPositionKey,
    d3drm_animation1_AddScaleKey,
    d3drm_animation1_DeleteKey,
    d3drm_animation1_SetFrame,
    d3drm_animation1_SetTime,
    d3drm_animation1_GetOptions,
};

static const struct IDirect3DRMAnimation2Vtbl d3drm_animation2_vtbl =
{
    d3drm_animation2_QueryInterface,
    d3drm_animation2_AddRef,
    d3drm_animation2_Release,
    d3drm_animation2_Clone,
    d3drm_animation2_AddDestroyCallback,
    d3drm_animation2_DeleteDestroyCallback,
    d3drm_animation2_SetAppData,
    d3drm_animation2_GetAppData,
    d3drm_animation2_SetName,
    d3drm_animation2_GetName,
    d3drm_animation2_GetClassName,
    d3drm_animation2_SetOptions,
    d3drm_animation2_AddRotateKey,
    d3drm_animation2_AddPositionKey,
    d3drm_animation2_AddScaleKey,
    d3drm_animation2_DeleteKey,
    d3drm_animation2_SetFrame,
    d3drm_animation2_SetTime,
    d3drm_animation2_GetOptions,
    d3drm_animation2_GetFrame,
    d3drm_animation2_DeleteKeyByID,
    d3drm_animation2_AddKey,
    d3drm_animation2_ModifyKey,
    d3drm_animation2_GetKeys,
};

HRESULT d3drm_animation_create(struct d3drm_animation **animation, IDirect3DRM *d3drm)
{
    static const char classname[] = "Animation";
    struct d3drm_animation *object;
    HRESULT hr = D3DRM_OK;

    TRACE("animation %p, d3drm %p.\n", animation, d3drm);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMAnimation_iface.lpVtbl = &d3drm_animation1_vtbl;
    object->IDirect3DRMAnimation2_iface.lpVtbl = &d3drm_animation2_vtbl;
    object->d3drm = d3drm;
    object->ref = 1;
    object->options = D3DRMANIMATION_CLOSED | D3DRMANIMATION_LINEARPOSITION;

    d3drm_object_init(&object->obj, classname);

    IDirect3DRM_AddRef(object->d3drm);

    *animation = object;

    return hr;
}
