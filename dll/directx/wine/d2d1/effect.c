/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

static inline struct d2d_transform *impl_from_ID2D1OffsetTransform(ID2D1OffsetTransform *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_transform, ID2D1TransformNode_iface);
}

static inline struct d2d_transform *impl_from_ID2D1BlendTransform(ID2D1BlendTransform *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_transform, ID2D1TransformNode_iface);
}

static inline struct d2d_transform *impl_from_ID2D1BorderTransform(ID2D1BorderTransform *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_transform, ID2D1TransformNode_iface);
}

static inline struct d2d_transform *impl_from_ID2D1BoundsAdjustmentTransform(
        ID2D1BoundsAdjustmentTransform *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_transform, ID2D1TransformNode_iface);
}

static inline struct d2d_vertex_buffer *impl_from_ID2D1VertexBuffer(ID2D1VertexBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_vertex_buffer, ID2D1VertexBuffer_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_vertex_buffer_QueryInterface(ID2D1VertexBuffer *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1VertexBuffer)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        ID2D1VertexBuffer_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_vertex_buffer_AddRef(ID2D1VertexBuffer *iface)
{
    struct d2d_vertex_buffer *buffer = impl_from_ID2D1VertexBuffer(iface);
    ULONG refcount = InterlockedIncrement(&buffer->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_vertex_buffer_Release(ID2D1VertexBuffer *iface)
{
    struct d2d_vertex_buffer *buffer = impl_from_ID2D1VertexBuffer(iface);
    ULONG refcount = InterlockedDecrement(&buffer->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(buffer);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d2d_vertex_buffer_Map(ID2D1VertexBuffer *iface, BYTE **data, UINT32 size)
{
    FIXME("iface %p, data %p, size %u.\n", iface, data, size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_vertex_buffer_Unmap(ID2D1VertexBuffer *iface)
{
    FIXME("iface %p.\n", iface);

    return E_NOTIMPL;
}

static const ID2D1VertexBufferVtbl d2d_vertex_buffer_vtbl =
{
    d2d_vertex_buffer_QueryInterface,
    d2d_vertex_buffer_AddRef,
    d2d_vertex_buffer_Release,
    d2d_vertex_buffer_Map,
    d2d_vertex_buffer_Unmap,
};

static HRESULT d2d_vertex_buffer_create(ID2D1VertexBuffer **buffer)
{
    struct d2d_vertex_buffer *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID2D1VertexBuffer_iface.lpVtbl = &d2d_vertex_buffer_vtbl;
    object->refcount = 1;

    *buffer = &object->ID2D1VertexBuffer_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_offset_transform_QueryInterface(ID2D1OffsetTransform *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1OffsetTransform)
            || IsEqualGUID(iid, &IID_ID2D1TransformNode)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        ID2D1OffsetTransform_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_offset_transform_AddRef(ID2D1OffsetTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1OffsetTransform(iface);
    ULONG refcount = InterlockedIncrement(&transform->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_offset_transform_Release(ID2D1OffsetTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1OffsetTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(transform);

    return refcount;
}

static UINT32 STDMETHODCALLTYPE d2d_offset_transform_GetInputCount(ID2D1OffsetTransform *iface)
{
    TRACE("iface %p.\n", iface);

    return 1;
}

static void STDMETHODCALLTYPE d2d_offset_transform_SetOffset(ID2D1OffsetTransform *iface,
        D2D1_POINT_2L offset)
{
    struct d2d_transform *transform = impl_from_ID2D1OffsetTransform(iface);

    TRACE("iface %p, offset %s.\n", iface, debug_d2d_point_2l(&offset));

    transform->offset = offset;
}

static D2D1_POINT_2L * STDMETHODCALLTYPE d2d_offset_transform_GetOffset(ID2D1OffsetTransform *iface,
        D2D1_POINT_2L *offset)
{
    struct d2d_transform *transform = impl_from_ID2D1OffsetTransform(iface);

    TRACE("iface %p.\n", iface);

    *offset = transform->offset;
    return offset;
}

static const ID2D1OffsetTransformVtbl d2d_offset_transform_vtbl =
{
    d2d_offset_transform_QueryInterface,
    d2d_offset_transform_AddRef,
    d2d_offset_transform_Release,
    d2d_offset_transform_GetInputCount,
    d2d_offset_transform_SetOffset,
    d2d_offset_transform_GetOffset,
};

static HRESULT d2d_offset_transform_create(D2D1_POINT_2L offset, ID2D1OffsetTransform **transform)
{
    struct d2d_transform *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID2D1TransformNode_iface.lpVtbl = (ID2D1TransformNodeVtbl *)&d2d_offset_transform_vtbl;
    object->refcount = 1;
    object->offset = offset;

    *transform = (ID2D1OffsetTransform *)&object->ID2D1TransformNode_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_blend_transform_QueryInterface(ID2D1BlendTransform *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1BlendTransform)
            || IsEqualGUID(iid, &IID_ID2D1ConcreteTransform)
            || IsEqualGUID(iid, &IID_ID2D1TransformNode)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        ID2D1BlendTransform_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_blend_transform_AddRef(ID2D1BlendTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1BlendTransform(iface);
    ULONG refcount = InterlockedIncrement(&transform->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_blend_transform_Release(ID2D1BlendTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1BlendTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(transform);

    return refcount;
}

static UINT32 STDMETHODCALLTYPE d2d_blend_transform_GetInputCount(ID2D1BlendTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1BlendTransform(iface);

    TRACE("iface %p.\n", iface);

    return transform->input_count;
}

static HRESULT STDMETHODCALLTYPE d2d_blend_transform_SetOutputBuffer(ID2D1BlendTransform *iface,
        D2D1_BUFFER_PRECISION precision, D2D1_CHANNEL_DEPTH depth)
{
    FIXME("iface %p, precision %u, depth %u stub.\n", iface, precision, depth);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_blend_transform_SetCached(ID2D1BlendTransform *iface,
        BOOL is_cached)
{
    FIXME("iface %p, is_cached %d stub.\n", iface, is_cached);
}

static void STDMETHODCALLTYPE d2d_blend_transform_SetDescription(ID2D1BlendTransform *iface,
        const D2D1_BLEND_DESCRIPTION *description)
{
    struct d2d_transform *transform = impl_from_ID2D1BlendTransform(iface);

    TRACE("iface %p, description %p.\n", iface, description);

    transform->blend_desc = *description;
}

static void STDMETHODCALLTYPE d2d_blend_transform_GetDescription(ID2D1BlendTransform *iface,
        D2D1_BLEND_DESCRIPTION *description)
{
    struct d2d_transform *transform = impl_from_ID2D1BlendTransform(iface);

    TRACE("iface %p, description %p.\n", iface, description);

    *description = transform->blend_desc;
}

static const ID2D1BlendTransformVtbl d2d_blend_transform_vtbl =
{
    d2d_blend_transform_QueryInterface,
    d2d_blend_transform_AddRef,
    d2d_blend_transform_Release,
    d2d_blend_transform_GetInputCount,
    d2d_blend_transform_SetOutputBuffer,
    d2d_blend_transform_SetCached,
    d2d_blend_transform_SetDescription,
    d2d_blend_transform_GetDescription,
};

static HRESULT d2d_blend_transform_create(UINT32 input_count, const D2D1_BLEND_DESCRIPTION *blend_desc,
        ID2D1BlendTransform **transform)
{
    struct d2d_transform *object;

    *transform = NULL;

    if (!input_count)
        return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID2D1TransformNode_iface.lpVtbl = (ID2D1TransformNodeVtbl *)&d2d_blend_transform_vtbl;
    object->refcount = 1;
    object->input_count = input_count;
    object->blend_desc = *blend_desc;

    *transform = (ID2D1BlendTransform *)&object->ID2D1TransformNode_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_border_transform_QueryInterface(ID2D1BorderTransform *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1BorderTransform)
            || IsEqualGUID(iid, &IID_ID2D1ConcreteTransform)
            || IsEqualGUID(iid, &IID_ID2D1TransformNode)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        ID2D1BorderTransform_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_border_transform_AddRef(ID2D1BorderTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1BorderTransform(iface);
    ULONG refcount = InterlockedIncrement(&transform->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_border_transform_Release(ID2D1BorderTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1BorderTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(transform);

    return refcount;
}

static UINT32 STDMETHODCALLTYPE d2d_border_transform_GetInputCount(ID2D1BorderTransform *iface)
{
    TRACE("iface %p.\n", iface);

    return 1;
}

static HRESULT STDMETHODCALLTYPE d2d_border_transform_SetOutputBuffer(
        ID2D1BorderTransform *iface, D2D1_BUFFER_PRECISION precision, D2D1_CHANNEL_DEPTH depth)
{
    FIXME("iface %p, precision %u, depth %u stub.\n", iface, precision, depth);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_border_transform_SetCached(
        ID2D1BorderTransform *iface, BOOL is_cached)
{
    FIXME("iface %p, is_cached %d stub.\n", iface, is_cached);
}

static void STDMETHODCALLTYPE d2d_border_transform_SetExtendModeX(
        ID2D1BorderTransform *iface, D2D1_EXTEND_MODE mode)
{
    struct d2d_transform *transform = impl_from_ID2D1BorderTransform(iface);

    TRACE("iface %p.\n", iface);

    switch (mode)
    {
        case D2D1_EXTEND_MODE_CLAMP:
        case D2D1_EXTEND_MODE_WRAP:
        case D2D1_EXTEND_MODE_MIRROR:
            transform->border.mode_x = mode;
            break;
        default:
            ;
    }
}

static void STDMETHODCALLTYPE d2d_border_transform_SetExtendModeY(
        ID2D1BorderTransform *iface, D2D1_EXTEND_MODE mode)
{
    struct d2d_transform *transform = impl_from_ID2D1BorderTransform(iface);

    TRACE("iface %p.\n", iface);

    switch (mode)
    {
        case D2D1_EXTEND_MODE_CLAMP:
        case D2D1_EXTEND_MODE_WRAP:
        case D2D1_EXTEND_MODE_MIRROR:
            transform->border.mode_y = mode;
            break;
        default:
            ;
    }
}

static D2D1_EXTEND_MODE STDMETHODCALLTYPE d2d_border_transform_GetExtendModeX(
        ID2D1BorderTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1BorderTransform(iface);

    TRACE("iface %p.\n", iface);

    return transform->border.mode_x;
}

static D2D1_EXTEND_MODE STDMETHODCALLTYPE d2d_border_transform_GetExtendModeY(
        ID2D1BorderTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1BorderTransform(iface);

    TRACE("iface %p.\n", iface);

    return transform->border.mode_y;
}

static const ID2D1BorderTransformVtbl d2d_border_transform_vtbl =
{
    d2d_border_transform_QueryInterface,
    d2d_border_transform_AddRef,
    d2d_border_transform_Release,
    d2d_border_transform_GetInputCount,
    d2d_border_transform_SetOutputBuffer,
    d2d_border_transform_SetCached,
    d2d_border_transform_SetExtendModeX,
    d2d_border_transform_SetExtendModeY,
    d2d_border_transform_GetExtendModeX,
    d2d_border_transform_GetExtendModeY,
};

static HRESULT d2d_border_transform_create(D2D1_EXTEND_MODE mode_x, D2D1_EXTEND_MODE mode_y,
        ID2D1BorderTransform **transform)
{
    struct d2d_transform *object;

    *transform = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID2D1TransformNode_iface.lpVtbl = (ID2D1TransformNodeVtbl *)&d2d_border_transform_vtbl;
    object->refcount = 1;
    object->border.mode_x = mode_x;
    object->border.mode_y = mode_y;

    *transform = (ID2D1BorderTransform *)&object->ID2D1TransformNode_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_bounds_adjustment_transform_QueryInterface(
        ID2D1BoundsAdjustmentTransform *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1BoundsAdjustmentTransform)
            || IsEqualGUID(iid, &IID_ID2D1TransformNode)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        ID2D1BoundsAdjustmentTransform_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_bounds_adjustment_transform_AddRef(
        ID2D1BoundsAdjustmentTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1BoundsAdjustmentTransform(iface);
    ULONG refcount = InterlockedIncrement(&transform->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_bounds_adjustment_transform_Release(
        ID2D1BoundsAdjustmentTransform *iface)
{
    struct d2d_transform *transform = impl_from_ID2D1BoundsAdjustmentTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(transform);

    return refcount;
}

static UINT32 STDMETHODCALLTYPE d2d_bounds_adjustment_transform_GetInputCount(
        ID2D1BoundsAdjustmentTransform *iface)
{
    TRACE("iface %p.\n", iface);

    return 1;
}

static void STDMETHODCALLTYPE d2d_bounds_adjustment_transform_SetOutputBounds(
        ID2D1BoundsAdjustmentTransform *iface, const D2D1_RECT_L *bounds)
{
    struct d2d_transform *transform = impl_from_ID2D1BoundsAdjustmentTransform(iface);

    TRACE("iface %p.\n", iface);

    transform->bounds = *bounds;
}

static void STDMETHODCALLTYPE d2d_bounds_adjustment_transform_GetOutputBounds(
        ID2D1BoundsAdjustmentTransform *iface, D2D1_RECT_L *bounds)
{
    struct d2d_transform *transform = impl_from_ID2D1BoundsAdjustmentTransform(iface);

    TRACE("iface %p.\n", iface);

    *bounds = transform->bounds;
}

static const ID2D1BoundsAdjustmentTransformVtbl d2d_bounds_adjustment_transform_vtbl =
{
    d2d_bounds_adjustment_transform_QueryInterface,
    d2d_bounds_adjustment_transform_AddRef,
    d2d_bounds_adjustment_transform_Release,
    d2d_bounds_adjustment_transform_GetInputCount,
    d2d_bounds_adjustment_transform_SetOutputBounds,
    d2d_bounds_adjustment_transform_GetOutputBounds,
};

static HRESULT d2d_bounds_adjustment_transform_create(const D2D1_RECT_L *rect,
        ID2D1BoundsAdjustmentTransform **transform)
{
    struct d2d_transform *object;

    *transform = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID2D1TransformNode_iface.lpVtbl = (ID2D1TransformNodeVtbl *)&d2d_bounds_adjustment_transform_vtbl;
    object->refcount = 1;
    object->bounds = *rect;

    *transform = (ID2D1BoundsAdjustmentTransform *)&object->ID2D1TransformNode_iface;

    return S_OK;
}

static struct d2d_transform_node * d2d_transform_graph_get_node(const struct d2d_transform_graph *graph,
        ID2D1TransformNode *object)
{
    struct d2d_transform_node *node;

    LIST_FOR_EACH_ENTRY(node, &graph->nodes, struct d2d_transform_node, entry)
    {
        if (node->object == object)
            return node;
    }

    return NULL;
}

static HRESULT d2d_transform_graph_add_node(struct d2d_transform_graph *graph,
        ID2D1TransformNode *object)
{
    struct d2d_transform_node *node;

    if (!(node = calloc(1, sizeof(*node))))
        return E_OUTOFMEMORY;
    node->input_count = ID2D1TransformNode_GetInputCount(object);
    if (!(node->inputs = calloc(node->input_count, sizeof(*node->inputs))))
    {
        free(node);
        return E_OUTOFMEMORY;
    }

    node->object = object;
    ID2D1TransformNode_AddRef(node->object);
    list_add_tail(&graph->nodes, &node->entry);

    return S_OK;
}

static void d2d_transform_node_disconnect(struct d2d_transform_node *node)
{
    struct d2d_transform_node *output = node->output;
    unsigned int i;

    if (!output)
        return;

    for (i = 0; i < output->input_count; ++i)
    {
        if (output->inputs[i] == node)
        {
            output->inputs[i] = NULL;
            break;
        }
    }
}

static void d2d_transform_graph_delete_node(struct d2d_transform_graph *graph,
        struct d2d_transform_node *node)
{
    unsigned int i;

    list_remove(&node->entry);
    ID2D1TransformNode_Release(node->object);

    for (i = 0; i < graph->input_count; ++i)
    {
        if (graph->inputs[i].node == node)
            memset(&graph->inputs[i].node, 0, sizeof(graph->inputs[i].node));
    }

    if (graph->output == node)
        graph->output = NULL;

    if (node->render_info)
        ID2D1DrawInfo_Release(&node->render_info->ID2D1DrawInfo_iface);

    d2d_transform_node_disconnect(node);

    free(node->inputs);
    free(node);
}

static void d2d_transform_graph_clear(struct d2d_transform_graph *graph)
{
    struct d2d_transform_node *node, *node_next;

    LIST_FOR_EACH_ENTRY_SAFE(node, node_next, &graph->nodes, struct d2d_transform_node, entry)
    {
        d2d_transform_graph_delete_node(graph, node);
    }
    graph->passthrough = false;
}

static inline struct d2d_transform_graph *impl_from_ID2D1TransformGraph(ID2D1TransformGraph *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_transform_graph, ID2D1TransformGraph_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_transform_graph_QueryInterface(ID2D1TransformGraph *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1TransformGraph)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1TransformGraph_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_transform_graph_AddRef(ID2D1TransformGraph *iface)
{
    struct d2d_transform_graph *graph =impl_from_ID2D1TransformGraph(iface);
    ULONG refcount = InterlockedIncrement(&graph->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_transform_graph_Release(ID2D1TransformGraph *iface)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);
    ULONG refcount = InterlockedDecrement(&graph->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d2d_transform_graph_clear(graph);
        free(graph->inputs);
        free(graph);
    }

    return refcount;
}

static UINT32 STDMETHODCALLTYPE d2d_transform_graph_GetInputCount(ID2D1TransformGraph *iface)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);

    TRACE("iface %p.\n", iface);

    return graph->input_count;
}

static HRESULT STDMETHODCALLTYPE d2d_transform_graph_SetSingleTransformNode(ID2D1TransformGraph *iface,
        ID2D1TransformNode *object)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);
    struct d2d_transform_node *node;
    unsigned int i, input_count;
    HRESULT hr;

    TRACE("iface %p, object %p.\n", iface, object);

    d2d_transform_graph_clear(graph);
    if (FAILED(hr = d2d_transform_graph_add_node(graph, object)))
        return hr;

    node = d2d_transform_graph_get_node(graph, object);
    graph->output = node;

    input_count = ID2D1TransformNode_GetInputCount(object);
    if (graph->input_count != input_count)
        return E_INVALIDARG;

    for (i = 0; i < graph->input_count; ++i)
    {
        graph->inputs[i].node = node;
        graph->inputs[i].index = i;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_transform_graph_AddNode(ID2D1TransformGraph *iface,
        ID2D1TransformNode *object)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);

    TRACE("iface %p, object %p.\n", iface, object);

    if (d2d_transform_graph_get_node(graph, object))
        return E_INVALIDARG;

    return d2d_transform_graph_add_node(graph, object);
}

static HRESULT STDMETHODCALLTYPE d2d_transform_graph_RemoveNode(ID2D1TransformGraph *iface,
        ID2D1TransformNode *object)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);
    struct d2d_transform_node *node;

    TRACE("iface %p, object %p.\n", iface, object);

    if (!(node = d2d_transform_graph_get_node(graph, object)))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    d2d_transform_graph_delete_node(graph, node);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_transform_graph_SetOutputNode(ID2D1TransformGraph *iface,
        ID2D1TransformNode *object)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);
    struct d2d_transform_node *node;

    TRACE("iface %p, object %p.\n", iface, object);

    if (!(node = d2d_transform_graph_get_node(graph, object)))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    graph->output = node;
    graph->passthrough = false;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_transform_graph_ConnectNode(ID2D1TransformGraph *iface,
        ID2D1TransformNode *from_node, ID2D1TransformNode *to_node, UINT32 index)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);
    struct d2d_transform_node *from, *to;

    TRACE("iface %p, from_node %p, to_node %p, index %u.\n", iface, from_node, to_node, index);

    if (!(from = d2d_transform_graph_get_node(graph, from_node)))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (!(to = d2d_transform_graph_get_node(graph, to_node)))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (index >= to->input_count)
        return E_INVALIDARG;

    d2d_transform_node_disconnect(from);
    to->inputs[index] = from;
    from->output = to;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_transform_graph_ConnectToEffectInput(ID2D1TransformGraph *iface,
        UINT32 input_index, ID2D1TransformNode *object, UINT32 node_index)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);
    struct d2d_transform_node *node;
    unsigned int count;

    TRACE("iface %p, input_index %u, object %p, node_index %u.\n", iface, input_index, object, node_index);

    if (!(node = d2d_transform_graph_get_node(graph, object)))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (input_index >= graph->input_count)
        return E_INVALIDARG;

    count = ID2D1TransformNode_GetInputCount(object);
    if (node_index >= count)
        return E_INVALIDARG;

    graph->inputs[input_index].node = node;
    graph->inputs[input_index].index = node_index;
    graph->passthrough = false;

    return S_OK;
}

static void STDMETHODCALLTYPE d2d_transform_graph_Clear(ID2D1TransformGraph *iface)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);

    TRACE("iface %p.\n", iface);

    d2d_transform_graph_clear(graph);
}

static HRESULT STDMETHODCALLTYPE d2d_transform_graph_SetPassthroughGraph(ID2D1TransformGraph *iface, UINT32 index)
{
    struct d2d_transform_graph *graph = impl_from_ID2D1TransformGraph(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    if (index >= graph->input_count)
        return E_INVALIDARG;

    d2d_transform_graph_clear(graph);

    graph->passthrough = true;
    graph->passthrough_input = index;

    return S_OK;
}

static const ID2D1TransformGraphVtbl d2d_transform_graph_vtbl =
{
    d2d_transform_graph_QueryInterface,
    d2d_transform_graph_AddRef,
    d2d_transform_graph_Release,
    d2d_transform_graph_GetInputCount,
    d2d_transform_graph_SetSingleTransformNode,
    d2d_transform_graph_AddNode,
    d2d_transform_graph_RemoveNode,
    d2d_transform_graph_SetOutputNode,
    d2d_transform_graph_ConnectNode,
    d2d_transform_graph_ConnectToEffectInput,
    d2d_transform_graph_Clear,
    d2d_transform_graph_SetPassthroughGraph,
};

static HRESULT d2d_transform_graph_create(UINT32 input_count, struct d2d_transform_graph **graph)
{
    struct d2d_transform_graph *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID2D1TransformGraph_iface.lpVtbl = &d2d_transform_graph_vtbl;
    object->refcount = 1;
    list_init(&object->nodes);

    if (!(object->inputs = calloc(input_count, sizeof(*object->inputs))))
    {
        free(object);
        return E_OUTOFMEMORY;
    }
    object->input_count = input_count;

    *graph = object;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_impl_QueryInterface(ID2D1EffectImpl *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1EffectImpl)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1EffectImpl_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_effect_impl_AddRef(ID2D1EffectImpl *iface)
{
    return 2;
}

static ULONG STDMETHODCALLTYPE d2d_effect_impl_Release(ID2D1EffectImpl *iface)
{
    return 1;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_impl_Initialize(ID2D1EffectImpl *iface,
        ID2D1EffectContext *context, ID2D1TransformGraph *graph)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_impl_PrepareForRender(ID2D1EffectImpl *iface, D2D1_CHANGE_TYPE type)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_impl_SetGraph(ID2D1EffectImpl *iface, ID2D1TransformGraph *graph)
{
    return S_OK;
}

static const ID2D1EffectImplVtbl d2d_effect_impl_vtbl =
{
    d2d_effect_impl_QueryInterface,
    d2d_effect_impl_AddRef,
    d2d_effect_impl_Release,
    d2d_effect_impl_Initialize,
    d2d_effect_impl_PrepareForRender,
    d2d_effect_impl_SetGraph,
};

static HRESULT STDMETHODCALLTYPE builtin_factory_stub(IUnknown **effect_impl)
{
    static ID2D1EffectImpl builtin_stub = { &d2d_effect_impl_vtbl };

    *effect_impl = (IUnknown *)&builtin_stub;

    return S_OK;
}

static const WCHAR _2d_affine_transform_description[] =
L"<?xml version='1.0'?>                                                      \
  <Effect>                                                                   \
    <Property name='DisplayName' type='string' value='2D Affine Transform'/> \
    <Property name='Author'      type='string' value='The Wine Project'/>    \
    <Property name='Category'    type='string' value='Stub'/>                \
    <Property name='Description' type='string' value='2D Affine Transform'/> \
    <Inputs>                                                                 \
      <Input name='Source'/>                                                 \
    </Inputs>                                                                \
  </Effect>";

static const WCHAR _3d_perspective_transform_description[] =
L"<?xml version='1.0'?>                                                           \
  <Effect>                                                                        \
    <Property name='DisplayName' type='string' value='3D Perspective Transform'/> \
    <Property name='Author'      type='string' value='The Wine Project'/>         \
    <Property name='Category'    type='string' value='Stub'/>                     \
    <Property name='Description' type='string' value='3D Perspective Transform'/> \
    <Inputs>                                                                      \
      <Input name='Source'/>                                                      \
    </Inputs>                                                                     \
  </Effect>";

static const WCHAR composite_description[] =
L"<?xml version='1.0'?>                                                   \
  <Effect>                                                                \
    <Property name='DisplayName' type='string' value='Composite'/>        \
    <Property name='Author'      type='string' value='The Wine Project'/> \
    <Property name='Category'    type='string' value='Stub'/>             \
    <Property name='Description' type='string' value='Composite'/>        \
    <Inputs minimum='1' maximum='0xffffffff' >                            \
      <Input name='Source1'/>                                             \
      <Input name='Source2'/>                                             \
    </Inputs>                                                             \
  </Effect>";

static const WCHAR crop_description[] =
L"<?xml version='1.0'?>                                                   \
  <Effect>                                                                \
    <Property name='DisplayName' type='string' value='Crop'/>             \
    <Property name='Author'      type='string' value='The Wine Project'/> \
    <Property name='Category'    type='string' value='Stub'/>             \
    <Property name='Description' type='string' value='Crop'/>             \
    <Inputs >                                                             \
      <Input name='Source'/>                                              \
    </Inputs>                                                             \
    <Property name='Rect' type='vector4' />                               \
  </Effect>";

static const WCHAR shadow_description[] =
L"<?xml version='1.0'?>                                                   \
  <Effect>                                                                \
    <Property name='DisplayName' type='string' value='Shadow'/>           \
    <Property name='Author'      type='string' value='The Wine Project'/> \
    <Property name='Category'    type='string' value='Stub'/>             \
    <Property name='Description' type='string' value='Shadow'/>           \
    <Inputs >                                                             \
      <Input name='Source'/>                                              \
    </Inputs>                                                             \
  </Effect>";

static const WCHAR grayscale_description[] =
L"<?xml version='1.0'?>                                                   \
  <Effect>                                                                \
    <Property name='DisplayName' type='string' value='Grayscale'/>        \
    <Property name='Author'      type='string' value='The Wine Project'/> \
    <Property name='Category'    type='string' value='Stub'/>             \
    <Property name='Description' type='string' value='Grayscale'/>        \
    <Inputs >                                                             \
      <Input name='Source'/>                                              \
    </Inputs>                                                             \
  </Effect>";

void d2d_effects_init_builtins(struct d2d_factory *factory)
{
    static const struct builtin_description
    {
        const CLSID *clsid;
        const WCHAR *description;
    }
    builtin_effects[] =
    {
        { &CLSID_D2D12DAffineTransform, _2d_affine_transform_description },
        { &CLSID_D2D13DPerspectiveTransform, _3d_perspective_transform_description},
        { &CLSID_D2D1Composite, composite_description },
        { &CLSID_D2D1Crop, crop_description },
        { &CLSID_D2D1Shadow, shadow_description },
        { &CLSID_D2D1Grayscale, grayscale_description },
    };
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(builtin_effects); ++i)
    {
        if (FAILED(hr = d2d_factory_register_builtin_effect(factory, builtin_effects[i].clsid, builtin_effects[i].description,
                NULL, 0, builtin_factory_stub)))
        {
            WARN("Failed to register the effect %s, hr %#lx.\n", wine_dbgstr_guid(builtin_effects[i].clsid), hr);
        }
    }
}

/* Same syntax is used for value and default values. */
static HRESULT d2d_effect_parse_float_array(D2D1_PROPERTY_TYPE type, const WCHAR *value,
        float *vec)
{
    unsigned int i, num_components;
    WCHAR *end_ptr;

    /* Type values are sequential. */
    switch (type)
    {
        case D2D1_PROPERTY_TYPE_VECTOR2:
        case D2D1_PROPERTY_TYPE_VECTOR3:
        case D2D1_PROPERTY_TYPE_VECTOR4:
            num_components = (type - D2D1_PROPERTY_TYPE_VECTOR2) + 2;
            break;
        case D2D1_PROPERTY_TYPE_MATRIX_3X2:
            num_components = 6;
            break;
        case D2D1_PROPERTY_TYPE_MATRIX_4X3:
        case D2D1_PROPERTY_TYPE_MATRIX_4X4:
        case D2D1_PROPERTY_TYPE_MATRIX_5X4:
            num_components = (type - D2D1_PROPERTY_TYPE_MATRIX_4X3) * 4 + 12;
            break;
        default:
            return E_UNEXPECTED;
    }

    if (*(value++) != '(') return E_INVALIDARG;

    for (i = 0; i < num_components; ++i)
    {
        vec[i] = wcstof(value, &end_ptr);
        if (value == end_ptr) return E_INVALIDARG;
        value = end_ptr;

        /* Trailing characters after last component are ignored. */
        if (i == num_components - 1) continue;
        if (*(value++) != ',') return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT d2d_effect_properties_internal_add(struct d2d_effect_properties *props,
        const WCHAR *name, UINT32 index, BOOL subprop, D2D1_PROPERTY_TYPE type, const WCHAR *value)
{
    static const UINT32 sizes[] =
    {
        [D2D1_PROPERTY_TYPE_UNKNOWN]       = 0,
        [D2D1_PROPERTY_TYPE_STRING]        = 0,
        [D2D1_PROPERTY_TYPE_BOOL]          = sizeof(BOOL),
        [D2D1_PROPERTY_TYPE_UINT32]        = sizeof(UINT32),
        [D2D1_PROPERTY_TYPE_INT32]         = sizeof(INT32),
        [D2D1_PROPERTY_TYPE_FLOAT]         = sizeof(float),
        [D2D1_PROPERTY_TYPE_VECTOR2]       = sizeof(D2D_VECTOR_2F),
        [D2D1_PROPERTY_TYPE_VECTOR3]       = sizeof(D2D_VECTOR_3F),
        [D2D1_PROPERTY_TYPE_VECTOR4]       = sizeof(D2D_VECTOR_4F),
        [D2D1_PROPERTY_TYPE_BLOB]          = 0,
        [D2D1_PROPERTY_TYPE_IUNKNOWN]      = sizeof(IUnknown *),
        [D2D1_PROPERTY_TYPE_ENUM]          = sizeof(UINT32),
        [D2D1_PROPERTY_TYPE_ARRAY]         = sizeof(UINT32),
        [D2D1_PROPERTY_TYPE_CLSID]         = sizeof(CLSID),
        [D2D1_PROPERTY_TYPE_MATRIX_3X2]    = sizeof(D2D_MATRIX_3X2_F),
        [D2D1_PROPERTY_TYPE_MATRIX_4X3]    = sizeof(D2D_MATRIX_4X3_F),
        [D2D1_PROPERTY_TYPE_MATRIX_4X4]    = sizeof(D2D_MATRIX_4X4_F),
        [D2D1_PROPERTY_TYPE_MATRIX_5X4]    = sizeof(D2D_MATRIX_5X4_F),
        [D2D1_PROPERTY_TYPE_COLOR_CONTEXT] = sizeof(ID2D1ColorContext *),
    };
    struct d2d_effect_property *p;
    HRESULT hr;

    assert(type >= D2D1_PROPERTY_TYPE_STRING && type <= D2D1_PROPERTY_TYPE_COLOR_CONTEXT);

    if (!d2d_array_reserve((void **)&props->properties, &props->size, props->count + 1,
            sizeof(*props->properties)))
    {
        return E_OUTOFMEMORY;
    }

    /* TODO: we could save some space for properties that have both getter and setter. */
    if (!d2d_array_reserve((void **)&props->data.ptr, &props->data.size,
            props->data.count + sizes[type], sizeof(*props->data.ptr)))
    {
        return E_OUTOFMEMORY;
    }
    props->data.count += sizes[type];

    p = &props->properties[props->count++];
    memset(p, 0, sizeof(*p));
    p->index = index;
    if (p->index < 0x80000000)
    {
        props->custom_count++;
        /* FIXME: this should probably be controlled by subproperty */
        p->readonly = FALSE;
    }
    else if (subprop)
        p->readonly = TRUE;
    else
        p->readonly = index != D2D1_PROPERTY_CACHED && index != D2D1_PROPERTY_PRECISION;
    p->name = wcsdup(name);
    p->type = type;
    if (p->type == D2D1_PROPERTY_TYPE_STRING)
    {
        p->data.ptr = wcsdup(value);
        p->size = value ? (wcslen(value) + 1) * sizeof(WCHAR) : sizeof(WCHAR);
    }
    else if (p->type == D2D1_PROPERTY_TYPE_BLOB)
    {
        p->data.ptr = NULL;
        p->size = 0;
    }
    else
    {
        void *src = NULL;
        UINT32 _uint32;
        float _vec[20];
        CLSID _clsid;
        BOOL _bool;

        p->data.offset = props->offset;
        p->size = sizes[type];
        props->offset += p->size;

        if (value)
        {
            switch (p->type)
            {
                case D2D1_PROPERTY_TYPE_UINT32:
                case D2D1_PROPERTY_TYPE_INT32:
                    _uint32 = wcstoul(value, NULL, 0);
                    src = &_uint32;
                    break;
                case D2D1_PROPERTY_TYPE_ENUM:
                    _uint32 = wcstoul(value, NULL, 10);
                    src = &_uint32;
                    break;
                case D2D1_PROPERTY_TYPE_BOOL:
                    if (!wcscmp(value, L"true")) _bool = TRUE;
                    else if (!wcscmp(value, L"false")) _bool = FALSE;
                    else return E_INVALIDARG;
                    src = &_bool;
                    break;
                case D2D1_PROPERTY_TYPE_CLSID:
                    CLSIDFromString(value, &_clsid);
                    src = &_clsid;
                    break;
                case D2D1_PROPERTY_TYPE_VECTOR2:
                case D2D1_PROPERTY_TYPE_VECTOR3:
                case D2D1_PROPERTY_TYPE_VECTOR4:
                case D2D1_PROPERTY_TYPE_MATRIX_3X2:
                case D2D1_PROPERTY_TYPE_MATRIX_4X3:
                case D2D1_PROPERTY_TYPE_MATRIX_4X4:
                case D2D1_PROPERTY_TYPE_MATRIX_5X4:
                    if (FAILED(hr = d2d_effect_parse_float_array(p->type, value, _vec)))
                    {
                        WARN("Failed to parse float array %s for type %u.\n",
                                wine_dbgstr_w(value), p->type);
                        return hr;
                    }
                    src = _vec;
                    break;
                case D2D1_PROPERTY_TYPE_IUNKNOWN:
                case D2D1_PROPERTY_TYPE_COLOR_CONTEXT:
                    break;
                default:
                    FIXME("Initial value for property type %u is not handled.\n", p->type);
            }

            if (src && p->size) memcpy(props->data.ptr + p->data.offset, src, p->size);
        }
        else if (p->size)
            memset(props->data.ptr + p->data.offset, 0, p->size);
    }

    return S_OK;
}

HRESULT d2d_effect_properties_add(struct d2d_effect_properties *props, const WCHAR *name,
        UINT32 index, D2D1_PROPERTY_TYPE type, const WCHAR *value)
{
    return d2d_effect_properties_internal_add(props, name, index, FALSE, type, value);
}

HRESULT d2d_effect_subproperties_add(struct d2d_effect_properties *props, const WCHAR *name,
        UINT32 index, D2D1_PROPERTY_TYPE type, const WCHAR *value)
{
    return d2d_effect_properties_internal_add(props, name, index, TRUE, type, value);
}

static HRESULT d2d_effect_duplicate_properties(struct d2d_effect *effect,
        struct d2d_effect_properties *dst, const struct d2d_effect_properties *src)
{
    HRESULT hr;
    size_t i;

    *dst = *src;
    dst->effect = effect;

    if (!(dst->data.ptr = malloc(dst->data.size)))
        return E_OUTOFMEMORY;
    memcpy(dst->data.ptr, src->data.ptr, dst->data.size);

    if (!(dst->properties = calloc(dst->size, sizeof(*dst->properties))))
        return E_OUTOFMEMORY;

    for (i = 0; i < dst->count; ++i)
    {
        struct d2d_effect_property *d = &dst->properties[i];
        const struct d2d_effect_property *s = &src->properties[i];

        *d = *s;
        d->name = wcsdup(s->name);
        if (d->type == D2D1_PROPERTY_TYPE_STRING)
            d->data.ptr = wcsdup((WCHAR *)s->data.ptr);

        if (s->subproperties)
        {
            if (!(d->subproperties = calloc(1, sizeof(*d->subproperties))))
                return E_OUTOFMEMORY;
            if (FAILED(hr = d2d_effect_duplicate_properties(effect, d->subproperties, s->subproperties)))
                return hr;
        }
    }

    return S_OK;
}

static struct d2d_effect_property * d2d_effect_properties_get_property_by_index(
        const struct d2d_effect_properties *properties, UINT32 index)
{
    unsigned int i;

    for (i = 0; i < properties->count; ++i)
    {
        if (properties->properties[i].index == index)
            return &properties->properties[i];
    }

    return NULL;
}

struct d2d_effect_property * d2d_effect_properties_get_property_by_name(
        const struct d2d_effect_properties *properties, const WCHAR *name)
{
    unsigned int i;

    for (i = 0; i < properties->count; ++i)
    {
        if (!wcscmp(properties->properties[i].name, name))
            return &properties->properties[i];
    }

    return NULL;
}

static UINT32 d2d_effect_properties_get_value_size(const struct d2d_effect_properties *properties,
        UINT32 index)
{
    struct d2d_effect *effect = properties->effect;
    struct d2d_effect_property *prop;
    UINT32 size;

    if (!(prop = d2d_effect_properties_get_property_by_index(properties, index)))
        return 0;

    if (prop->get_function)
    {
        if (FAILED(prop->get_function((IUnknown *)effect->impl, NULL, 0, &size))) return 0;
        return size;
    }

    return prop->size;
}

static HRESULT d2d_effect_return_string(const WCHAR *str, WCHAR *buffer, UINT32 buffer_size)
{
    UINT32 size = str ? wcslen(str) : 0;
    if (size >= buffer_size) return D2DERR_INSUFFICIENT_BUFFER;
    if (str) memcpy(buffer, str, (size + 1) * sizeof(*buffer));
    else *buffer = 0;
    return S_OK;
}

static HRESULT d2d_effect_property_get_value(const struct d2d_effect_properties *properties,
        const struct d2d_effect_property *prop, D2D1_PROPERTY_TYPE type, BYTE *value, UINT32 size)
{
    struct d2d_effect *effect = properties->effect;
    UINT32 actual_size;

    memset(value, 0, size);

    if (type != D2D1_PROPERTY_TYPE_UNKNOWN && prop->type != type) return E_INVALIDARG;
    /* Do not check sizes for variable-length properties. */
    if (prop->type != D2D1_PROPERTY_TYPE_STRING
            && prop->type != D2D1_PROPERTY_TYPE_BLOB
            && prop->size != size)
    {
        return E_INVALIDARG;
    }

    if (prop->get_function)
        return prop->get_function((IUnknown *)effect->impl, value, size, &actual_size);

    switch (prop->type)
    {
        case D2D1_PROPERTY_TYPE_BLOB:
            memset(value, 0, size);
            break;
        case D2D1_PROPERTY_TYPE_STRING:
            return d2d_effect_return_string(prop->data.ptr, (WCHAR *)value, size / sizeof(WCHAR));
        default:
            memcpy(value, properties->data.ptr + prop->data.offset, size);
            break;
    }

    return S_OK;
}

HRESULT d2d_effect_property_get_uint32_value(const struct d2d_effect_properties *properties,
        const struct d2d_effect_property *prop, UINT32 *value)
{
    return d2d_effect_property_get_value(properties, prop, D2D1_PROPERTY_TYPE_UINT32,
            (BYTE *)value, sizeof(*value));
}

static HRESULT d2d_effect_property_set_value(struct d2d_effect_properties *properties,
        struct d2d_effect_property *prop, D2D1_PROPERTY_TYPE type, const BYTE *value, UINT32 size)
{
    struct d2d_effect *effect = properties->effect;
    if (prop->readonly || !effect) return E_INVALIDARG;
    if (type != D2D1_PROPERTY_TYPE_UNKNOWN && prop->type != type) return E_INVALIDARG;
    if (prop->get_function && !prop->set_function) return E_INVALIDARG;
    if (prop->index < 0x80000000 && !prop->set_function) return E_INVALIDARG;

    if (prop->set_function)
        return prop->set_function((IUnknown *)effect->impl, value, size);

    if (prop->size != size) return E_INVALIDARG;

    switch (prop->type)
    {
        case D2D1_PROPERTY_TYPE_BOOL:
        case D2D1_PROPERTY_TYPE_UINT32:
        case D2D1_PROPERTY_TYPE_ENUM:
            memcpy(properties->data.ptr + prop->data.offset, value, size);
            break;
        default:
            FIXME("Unhandled type %u.\n", prop->type);
    }

    return S_OK;
}

void d2d_effect_properties_cleanup(struct d2d_effect_properties *props)
{
    struct d2d_effect_property *p;
    size_t i;

    for (i = 0; i < props->count; ++i)
    {
        p = &props->properties[i];
        free(p->name);
        if (p->type == D2D1_PROPERTY_TYPE_STRING)
            free(p->data.ptr);
        if (p->subproperties)
        {
            d2d_effect_properties_cleanup(p->subproperties);
            free(p->subproperties);
        }
    }
    free(props->properties);
    free(props->data.ptr);
}

static inline struct d2d_effect_context *impl_from_ID2D1EffectContext(ID2D1EffectContext *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_effect_context, ID2D1EffectContext_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_QueryInterface(ID2D1EffectContext *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1EffectContext)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1EffectContext_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_effect_context_AddRef(ID2D1EffectContext *iface)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    ULONG refcount = InterlockedIncrement(&effect_context->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_effect_context_Release(ID2D1EffectContext *iface)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    ULONG refcount = InterlockedDecrement(&effect_context->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1DeviceContext6_Release(&effect_context->device_context->ID2D1DeviceContext6_iface);
        free(effect_context);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_effect_context_GetDpi(ID2D1EffectContext *iface, float *dpi_x, float *dpi_y)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);

    TRACE("iface %p, dpi_x %p, dpi_y %p.\n", iface, dpi_x, dpi_y);

    ID2D1DeviceContext6_GetDpi(&effect_context->device_context->ID2D1DeviceContext6_iface, dpi_x, dpi_y);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateEffect(ID2D1EffectContext *iface,
        REFCLSID clsid, ID2D1Effect **effect)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);

    TRACE("iface %p, clsid %s, effect %p.\n", iface, debugstr_guid(clsid), effect);

    return ID2D1DeviceContext6_CreateEffect(&effect_context->device_context->ID2D1DeviceContext6_iface,
            clsid, effect);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_GetMaximumSupportedFeatureLevel(ID2D1EffectContext *iface,
        const D3D_FEATURE_LEVEL *levels, UINT32 level_count, D3D_FEATURE_LEVEL *max_level)
{
    FIXME("iface %p, levels %p, level_count %u, max_level %p stub!\n", iface, levels, level_count, max_level);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateTransformNodeFromEffect(ID2D1EffectContext *iface,
        ID2D1Effect *effect, ID2D1TransformNode **node)
{
    FIXME("iface %p, effect %p, node %p stub!\n", iface, effect, node);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateBlendTransform(ID2D1EffectContext *iface,
        UINT32 num_inputs, const D2D1_BLEND_DESCRIPTION *description, ID2D1BlendTransform **transform)
{
    TRACE("iface %p, num_inputs %u, description %p, transform %p,\n", iface, num_inputs, description, transform);

    return d2d_blend_transform_create(num_inputs, description, transform);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateBorderTransform(ID2D1EffectContext *iface,
        D2D1_EXTEND_MODE mode_x, D2D1_EXTEND_MODE mode_y, ID2D1BorderTransform **transform)
{
    TRACE("iface %p, mode_x %#x, mode_y %#x, transform %p.\n", iface, mode_x, mode_y, transform);

    return d2d_border_transform_create(mode_x, mode_y, transform);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateOffsetTransform(ID2D1EffectContext *iface,
        D2D1_POINT_2L offset, ID2D1OffsetTransform **transform)
{
    TRACE("iface %p, offset %s, transform %p.\n", iface, debug_d2d_point_2l(&offset), transform);

    return d2d_offset_transform_create(offset, transform);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateBoundsAdjustmentTransform(ID2D1EffectContext *iface,
        const D2D1_RECT_L *output_rect, ID2D1BoundsAdjustmentTransform **transform)
{
    TRACE("iface %p, output_rect %s, transform %p.\n", iface, debug_d2d_rect_l(output_rect), transform);

    return d2d_bounds_adjustment_transform_create(output_rect, transform);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_LoadPixelShader(ID2D1EffectContext *iface,
        REFGUID shader_id, const BYTE *buffer, UINT32 buffer_size)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    struct d2d_device *device = effect_context->device_context->device;
    ID3D11PixelShader *shader;
    HRESULT hr;

    TRACE("iface %p, shader_id %s, buffer %p, buffer_size %u.\n",
            iface, debugstr_guid(shader_id), buffer, buffer_size);

    if (d2d_device_get_indexed_object(&device->shaders, shader_id, NULL))
        return S_OK;

    if (FAILED(hr = ID3D11Device1_CreatePixelShader(effect_context->device_context->d3d_device,
            buffer, buffer_size, NULL, &shader)))
    {
        WARN("Failed to create a pixel shader, hr %#lx.\n", hr);
        return hr;
    }

    hr = d2d_device_add_indexed_object(&device->shaders, shader_id, (IUnknown *)shader);
    ID3D11PixelShader_Release(shader);

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_LoadVertexShader(ID2D1EffectContext *iface,
        REFGUID shader_id, const BYTE *buffer, UINT32 buffer_size)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    struct d2d_device *device = effect_context->device_context->device;
    ID3D11VertexShader *shader;
    HRESULT hr;

    TRACE("iface %p, shader_id %s, buffer %p, buffer_size %u.\n",
            iface, debugstr_guid(shader_id), buffer, buffer_size);

    if (d2d_device_get_indexed_object(&device->shaders, shader_id, NULL))
        return S_OK;

    if (FAILED(hr = ID3D11Device1_CreateVertexShader(effect_context->device_context->d3d_device,
            buffer, buffer_size, NULL, &shader)))
    {
        WARN("Failed to create vertex shader, hr %#lx.\n", hr);
        return hr;
    }

    hr = d2d_device_add_indexed_object(&device->shaders, shader_id, (IUnknown *)shader);
    ID3D11VertexShader_Release(shader);

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_LoadComputeShader(ID2D1EffectContext *iface,
        REFGUID shader_id, const BYTE *buffer, UINT32 buffer_size)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    struct d2d_device *device = effect_context->device_context->device;
    ID3D11ComputeShader *shader;
    HRESULT hr;

    TRACE("iface %p, shader_id %s, buffer %p, buffer_size %u.\n",
            iface, debugstr_guid(shader_id), buffer, buffer_size);

    if (d2d_device_get_indexed_object(&device->shaders, shader_id, NULL))
        return S_OK;

    if (FAILED(hr = ID3D11Device1_CreateComputeShader(effect_context->device_context->d3d_device,
            buffer, buffer_size, NULL, &shader)))
    {
        WARN("Failed to create a compute shader, hr %#lx.\n", hr);
        return hr;
    }

    hr = d2d_device_add_indexed_object(&device->shaders, shader_id, (IUnknown *)shader);
    ID3D11ComputeShader_Release(shader);

    return hr;
}

static BOOL STDMETHODCALLTYPE d2d_effect_context_IsShaderLoaded(ID2D1EffectContext *iface, REFGUID shader_id)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    struct d2d_device *device = effect_context->device_context->device;

    TRACE("iface %p, shader_id %s.\n", iface, debugstr_guid(shader_id));

    return d2d_device_get_indexed_object(&device->shaders, shader_id, NULL);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateResourceTexture(ID2D1EffectContext *iface,
        const GUID *id,  const D2D1_RESOURCE_TEXTURE_PROPERTIES *texture_properties,
        const BYTE *data, const UINT32 *strides, UINT32 data_size, ID2D1ResourceTexture **texture)
{
    FIXME("iface %p, id %s, texture_properties %p, data %p, strides %p, data_size %u, texture %p stub!\n",
            iface, debugstr_guid(id), texture_properties, data, strides, data_size, texture);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_FindResourceTexture(ID2D1EffectContext *iface,
        const GUID *id, ID2D1ResourceTexture **texture)
{
    FIXME("iface %p, id %s, texture %p stub!\n", iface, debugstr_guid(id), texture);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateVertexBuffer(ID2D1EffectContext *iface,
        const D2D1_VERTEX_BUFFER_PROPERTIES *buffer_properties, const GUID *id,
        const D2D1_CUSTOM_VERTEX_BUFFER_PROPERTIES *custom_buffer_properties,
        ID2D1VertexBuffer **buffer)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    struct d2d_device_context *context = effect_context->device_context;
    HRESULT hr;

    FIXME("iface %p, buffer_properties %p, id %s, custom_buffer_properties %p, buffer %p stub!\n",
            iface, buffer_properties, debugstr_guid(id), custom_buffer_properties, buffer);

    if (id && d2d_device_get_indexed_object(&context->vertex_buffers, id, (IUnknown **)buffer))
        return S_OK;

    if (SUCCEEDED(hr = d2d_vertex_buffer_create(buffer)))
    {
        if (id)
            hr = d2d_device_add_indexed_object(&context->vertex_buffers, id, (IUnknown *)*buffer);
    }

    if (FAILED(hr))
        *buffer = NULL;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_FindVertexBuffer(ID2D1EffectContext *iface,
        const GUID *id, ID2D1VertexBuffer **buffer)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    struct d2d_device_context *context = effect_context->device_context;

    TRACE("iface %p, id %s, buffer %p.\n", iface, debugstr_guid(id), buffer);

    if (!d2d_device_get_indexed_object(&context->vertex_buffers, id, (IUnknown **)buffer))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateColorContext(ID2D1EffectContext *iface,
        D2D1_COLOR_SPACE space, const BYTE *profile, UINT32 profile_size, ID2D1ColorContext **color_context)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);

    TRACE("iface %p, space %#x, profile %p, profile_size %u, color_context %p.\n",
            iface, space, profile, profile_size, color_context);

    return ID2D1DeviceContext6_CreateColorContext(&effect_context->device_context->ID2D1DeviceContext6_iface,
            space, profile, profile_size, color_context);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateColorContextFromFilename(ID2D1EffectContext *iface,
        const WCHAR *filename, ID2D1ColorContext **color_context)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);

    TRACE("iface %p, filename %s, color_context %p.\n", iface, debugstr_w(filename), color_context);

    return ID2D1DeviceContext6_CreateColorContextFromFilename(&effect_context->device_context->ID2D1DeviceContext6_iface,
            filename, color_context);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CreateColorContextFromWicColorContext(ID2D1EffectContext *iface,
        IWICColorContext *wic_color_context, ID2D1ColorContext **color_context)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);

    TRACE("iface %p, wic_color_context %p, color_context %p.\n", iface, wic_color_context, color_context);

    return ID2D1DeviceContext6_CreateColorContextFromWicColorContext(&effect_context->device_context->ID2D1DeviceContext6_iface,
            wic_color_context, color_context);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_context_CheckFeatureSupport(ID2D1EffectContext *iface,
        D2D1_FEATURE feature, void *data, UINT32 data_size)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);
    D3D11_FEATURE d3d11_feature;

    TRACE("iface %p, feature %#x, data %p, data_size %u.\n", iface, feature, data, data_size);

    /* Data structures are compatible. */
    switch (feature)
    {
        case D2D1_FEATURE_DOUBLES: d3d11_feature = D3D11_FEATURE_DOUBLES; break;
        case D2D1_FEATURE_D3D10_X_HARDWARE_OPTIONS: d3d11_feature = D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS; break;
        default:
            WARN("Unexpected feature index %d.\n", feature);
            return E_INVALIDARG;
    }

    return ID3D11Device1_CheckFeatureSupport(effect_context->device_context->d3d_device,
            d3d11_feature, data, data_size);
}

static BOOL STDMETHODCALLTYPE d2d_effect_context_IsBufferPrecisionSupported(ID2D1EffectContext *iface,
        D2D1_BUFFER_PRECISION precision)
{
    struct d2d_effect_context *effect_context = impl_from_ID2D1EffectContext(iface);

    TRACE("iface %p, precision %u.\n", iface, precision);

    return ID2D1DeviceContext6_IsBufferPrecisionSupported(&effect_context->device_context->ID2D1DeviceContext6_iface,
            precision);
}

static const ID2D1EffectContextVtbl d2d_effect_context_vtbl =
{
    d2d_effect_context_QueryInterface,
    d2d_effect_context_AddRef,
    d2d_effect_context_Release,
    d2d_effect_context_GetDpi,
    d2d_effect_context_CreateEffect,
    d2d_effect_context_GetMaximumSupportedFeatureLevel,
    d2d_effect_context_CreateTransformNodeFromEffect,
    d2d_effect_context_CreateBlendTransform,
    d2d_effect_context_CreateBorderTransform,
    d2d_effect_context_CreateOffsetTransform,
    d2d_effect_context_CreateBoundsAdjustmentTransform,
    d2d_effect_context_LoadPixelShader,
    d2d_effect_context_LoadVertexShader,
    d2d_effect_context_LoadComputeShader,
    d2d_effect_context_IsShaderLoaded,
    d2d_effect_context_CreateResourceTexture,
    d2d_effect_context_FindResourceTexture,
    d2d_effect_context_CreateVertexBuffer,
    d2d_effect_context_FindVertexBuffer,
    d2d_effect_context_CreateColorContext,
    d2d_effect_context_CreateColorContextFromFilename,
    d2d_effect_context_CreateColorContextFromWicColorContext,
    d2d_effect_context_CheckFeatureSupport,
    d2d_effect_context_IsBufferPrecisionSupported,
};

void d2d_effect_context_init(struct d2d_effect_context *effect_context, struct d2d_device_context *device_context)
{
    effect_context->ID2D1EffectContext_iface.lpVtbl = &d2d_effect_context_vtbl;
    effect_context->refcount = 1;
    effect_context->device_context = device_context;
    ID2D1DeviceContext6_AddRef(&device_context->ID2D1DeviceContext6_iface);
}

static inline struct d2d_effect *impl_from_ID2D1Effect(ID2D1Effect *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_effect, ID2D1Effect_iface);
}

static void d2d_effect_cleanup(struct d2d_effect *effect)
{
    unsigned int i;

    for (i = 0; i < effect->input_count; ++i)
    {
        if (effect->inputs[i])
            ID2D1Image_Release(effect->inputs[i]);
    }
    free(effect->inputs);
    ID2D1EffectContext_Release(&effect->effect_context->ID2D1EffectContext_iface);
    if (effect->graph)
        ID2D1TransformGraph_Release(&effect->graph->ID2D1TransformGraph_iface);
    d2d_effect_properties_cleanup(&effect->properties);
    if (effect->impl)
        ID2D1EffectImpl_Release(effect->impl);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_QueryInterface(ID2D1Effect *iface, REFIID iid, void **out)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1Effect)
            || IsEqualGUID(iid, &IID_ID2D1Properties)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1Effect_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ID2D1Image)
            || IsEqualGUID(iid, &IID_ID2D1Resource))
    {
        ID2D1Image_AddRef(&effect->ID2D1Image_iface);
        *out = &effect->ID2D1Image_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_effect_AddRef(ID2D1Effect *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    ULONG refcount = InterlockedIncrement(&effect->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_effect_Release(ID2D1Effect *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    ULONG refcount = InterlockedDecrement(&effect->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d2d_effect_cleanup(effect);
        free(effect);
    }

    return refcount;
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetPropertyCount(ID2D1Effect *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1Properties_GetPropertyCount(&effect->properties.ID2D1Properties_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_GetPropertyName(ID2D1Effect *iface, UINT32 index,
        WCHAR *name, UINT32 name_count)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %u, name %p, name_count %u.\n", iface, index, name, name_count);

    return ID2D1Properties_GetPropertyName(&effect->properties.ID2D1Properties_iface,
            index, name, name_count);
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetPropertyNameLength(ID2D1Effect *iface, UINT32 index)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    return ID2D1Properties_GetPropertyNameLength(&effect->properties.ID2D1Properties_iface, index);
}

static D2D1_PROPERTY_TYPE STDMETHODCALLTYPE d2d_effect_GetType(ID2D1Effect *iface, UINT32 index)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %#x.\n", iface, index);

    return ID2D1Properties_GetType(&effect->properties.ID2D1Properties_iface, index);
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetPropertyIndex(ID2D1Effect *iface, const WCHAR *name)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name));

    return ID2D1Properties_GetPropertyIndex(&effect->properties.ID2D1Properties_iface, name);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_SetValueByName(ID2D1Effect *iface, const WCHAR *name,
        D2D1_PROPERTY_TYPE type, const BYTE *value, UINT32 value_size)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, name %s, type %u, value %p, value_size %u.\n", iface, debugstr_w(name),
            type, value, value_size);

    return ID2D1Properties_SetValueByName(&effect->properties.ID2D1Properties_iface, name,
            type, value, value_size);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_SetValue(ID2D1Effect *iface, UINT32 index, D2D1_PROPERTY_TYPE type,
        const BYTE *value, UINT32 value_size)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %#x, type %u, value %p, value_size %u.\n", iface, index, type, value, value_size);

    return ID2D1Properties_SetValue(&effect->properties.ID2D1Properties_iface, index, type,
            value, value_size);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_GetValueByName(ID2D1Effect *iface, const WCHAR *name,
        D2D1_PROPERTY_TYPE type, BYTE *value, UINT32 value_size)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, name %s, type %#x, value %p, value_size %u.\n", iface, debugstr_w(name), type,
            value, value_size);

    return ID2D1Properties_GetValueByName(&effect->properties.ID2D1Properties_iface, name, type,
            value, value_size);
}

static HRESULT d2d_effect_get_value(struct d2d_effect *effect, UINT32 index, D2D1_PROPERTY_TYPE type,
        BYTE *value, UINT32 value_size)
{
    return ID2D1Properties_GetValue(&effect->properties.ID2D1Properties_iface, index, type, value, value_size);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_GetValue(ID2D1Effect *iface, UINT32 index, D2D1_PROPERTY_TYPE type,
        BYTE *value, UINT32 value_size)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %#x, type %u, value %p, value_size %u.\n", iface, index, type, value, value_size);

    return d2d_effect_get_value(effect, index, type, value, value_size);
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetValueSize(ID2D1Effect *iface, UINT32 index)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %#x.\n", iface, index);

    return ID2D1Properties_GetValueSize(&effect->properties.ID2D1Properties_iface, index);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_GetSubProperties(ID2D1Effect *iface, UINT32 index,
        ID2D1Properties **props)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %u, props %p.\n", iface, index, props);

    return ID2D1Properties_GetSubProperties(&effect->properties.ID2D1Properties_iface, index, props);
}

static void STDMETHODCALLTYPE d2d_effect_SetInput(ID2D1Effect *iface, UINT32 index, ID2D1Image *input, BOOL invalidate)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %u, input %p, invalidate %#x.\n", iface, index, input, invalidate);

    if (index >= effect->input_count)
        return;

    ID2D1Image_AddRef(input);
    if (effect->inputs[index])
        ID2D1Image_Release(effect->inputs[index]);
    effect->inputs[index] = input;
}

static HRESULT d2d_effect_set_input_count(struct d2d_effect *effect, UINT32 count)
{
    bool initialized = effect->inputs != NULL;
    HRESULT hr = S_OK;
    unsigned int i;

    if (count == effect->input_count)
        return S_OK;

    if (count < effect->input_count)
    {
        for (i = count; i < effect->input_count; ++i)
        {
            if (effect->inputs[i])
                ID2D1Image_Release(effect->inputs[i]);
        }
    }
    else
    {
        if (!d2d_array_reserve((void **)&effect->inputs, &effect->inputs_size,
                count, sizeof(*effect->inputs)))
        {
            ERR("Failed to resize inputs array.\n");
            return E_OUTOFMEMORY;
        }

        memset(&effect->inputs[effect->input_count], 0, sizeof(*effect->inputs) * (count - effect->input_count));
    }
    effect->input_count = count;

    if (initialized)
    {
        ID2D1TransformGraph_Release(&effect->graph->ID2D1TransformGraph_iface);
        effect->graph = NULL;

        if (SUCCEEDED(hr = d2d_transform_graph_create(count, &effect->graph)))
        {
            if (FAILED(hr = ID2D1EffectImpl_SetGraph(effect->impl, &effect->graph->ID2D1TransformGraph_iface)))
                WARN("Failed to set a new transform graph, hr %#lx.\n", hr);
        }
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_SetInputCount(ID2D1Effect *iface, UINT32 count)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);
    unsigned int min_inputs, max_inputs;

    TRACE("iface %p, count %u.\n", iface, count);

    d2d_effect_get_value(effect, D2D1_PROPERTY_MIN_INPUTS, D2D1_PROPERTY_TYPE_UINT32,
            (BYTE *)&min_inputs, sizeof(min_inputs));
    d2d_effect_get_value(effect, D2D1_PROPERTY_MAX_INPUTS, D2D1_PROPERTY_TYPE_UINT32,
            (BYTE *)&max_inputs, sizeof(max_inputs));

    if (count < min_inputs || count > max_inputs)
        return E_INVALIDARG;

    return d2d_effect_set_input_count(effect, count);
}

static void STDMETHODCALLTYPE d2d_effect_GetInput(ID2D1Effect *iface, UINT32 index, ID2D1Image **input)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, index %u, input %p.\n", iface, index, input);

    if (index < effect->input_count && effect->inputs[index])
        ID2D1Image_AddRef(*input = effect->inputs[index]);
    else
        *input = NULL;
}

static UINT32 STDMETHODCALLTYPE d2d_effect_GetInputCount(ID2D1Effect *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p.\n", iface);

    return effect->input_count;
}

static void STDMETHODCALLTYPE d2d_effect_GetOutput(ID2D1Effect *iface, ID2D1Image **output)
{
    struct d2d_effect *effect = impl_from_ID2D1Effect(iface);

    TRACE("iface %p, output %p.\n", iface, output);

    ID2D1Image_AddRef(*output = &effect->ID2D1Image_iface);
}

static const ID2D1EffectVtbl d2d_effect_vtbl =
{
    d2d_effect_QueryInterface,
    d2d_effect_AddRef,
    d2d_effect_Release,
    d2d_effect_GetPropertyCount,
    d2d_effect_GetPropertyName,
    d2d_effect_GetPropertyNameLength,
    d2d_effect_GetType,
    d2d_effect_GetPropertyIndex,
    d2d_effect_SetValueByName,
    d2d_effect_SetValue,
    d2d_effect_GetValueByName,
    d2d_effect_GetValue,
    d2d_effect_GetValueSize,
    d2d_effect_GetSubProperties,
    d2d_effect_SetInput,
    d2d_effect_SetInputCount,
    d2d_effect_GetInput,
    d2d_effect_GetInputCount,
    d2d_effect_GetOutput,
};

static inline struct d2d_effect *impl_from_ID2D1Image(ID2D1Image *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_effect, ID2D1Image_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_image_QueryInterface(ID2D1Image *iface, REFIID iid, void **out)
{
    struct d2d_effect *effect = impl_from_ID2D1Image(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return d2d_effect_QueryInterface(&effect->ID2D1Effect_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE d2d_effect_image_AddRef(ID2D1Image *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Image(iface);

    TRACE("iface %p.\n", iface);

    return d2d_effect_AddRef(&effect->ID2D1Effect_iface);
}

static ULONG STDMETHODCALLTYPE d2d_effect_image_Release(ID2D1Image *iface)
{
    struct d2d_effect *effect = impl_from_ID2D1Image(iface);

    TRACE("iface %p.\n", iface);

    return d2d_effect_Release(&effect->ID2D1Effect_iface);
}

static void STDMETHODCALLTYPE d2d_effect_image_GetFactory(ID2D1Image *iface, ID2D1Factory **factory)
{
    struct d2d_effect *effect = impl_from_ID2D1Image(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = effect->effect_context->device_context->factory);
}

static const ID2D1ImageVtbl d2d_effect_image_vtbl =
{
    d2d_effect_image_QueryInterface,
    d2d_effect_image_AddRef,
    d2d_effect_image_Release,
    d2d_effect_image_GetFactory,
};

static inline struct d2d_effect_properties *impl_from_ID2D1Properties(ID2D1Properties *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_effect_properties, ID2D1Properties_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_properties_QueryInterface(ID2D1Properties *iface,
        REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_ID2D1Properties) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        ID2D1Properties_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_effect_properties_AddRef(ID2D1Properties *iface)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);

    if (properties->effect)
        return ID2D1Effect_AddRef(&properties->effect->ID2D1Effect_iface);

    return InterlockedIncrement(&properties->refcount);
}

static ULONG STDMETHODCALLTYPE d2d_effect_properties_Release(ID2D1Properties *iface)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    ULONG refcount;

    if (properties->effect)
        return ID2D1Effect_Release(&properties->effect->ID2D1Effect_iface);

    refcount = InterlockedDecrement(&properties->refcount);

    if (!refcount)
    {
        d2d_effect_properties_cleanup(properties);
        free(properties);
    }

    return refcount;
}

static UINT32 STDMETHODCALLTYPE d2d_effect_properties_GetPropertyCount(ID2D1Properties *iface)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);

    TRACE("iface %p.\n", iface);

    return properties->custom_count;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_properties_GetPropertyName(ID2D1Properties *iface,
        UINT32 index, WCHAR *name, UINT32 name_count)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    struct d2d_effect_property *prop;

    TRACE("iface %p, index %u, name %p, name_count %u.\n", iface, index, name, name_count);

    if (!(prop = d2d_effect_properties_get_property_by_index(properties, index)))
        return D2DERR_INVALID_PROPERTY;

    return d2d_effect_return_string(prop->name, name, name_count);
}

static UINT32 STDMETHODCALLTYPE d2d_effect_properties_GetPropertyNameLength(ID2D1Properties *iface,
        UINT32 index)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    struct d2d_effect_property *prop;

    TRACE("iface %p, index %u.\n", iface, index);

    if (!(prop = d2d_effect_properties_get_property_by_index(properties, index)))
        return D2DERR_INVALID_PROPERTY;

    return wcslen(prop->name);
}

static D2D1_PROPERTY_TYPE STDMETHODCALLTYPE d2d_effect_properties_GetType(ID2D1Properties *iface,
        UINT32 index)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    struct d2d_effect_property *prop;

    TRACE("iface %p, index %#x.\n", iface, index);

    if (!(prop = d2d_effect_properties_get_property_by_index(properties, index)))
        return D2D1_PROPERTY_TYPE_UNKNOWN;

    return prop->type;
}

static UINT32 STDMETHODCALLTYPE d2d_effect_properties_GetPropertyIndex(ID2D1Properties *iface,
        const WCHAR *name)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    struct d2d_effect_property *prop;

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name));

    if (!(prop = d2d_effect_properties_get_property_by_name(properties, name)))
        return D2D1_INVALID_PROPERTY_INDEX;

    return prop->index;
}

static HRESULT STDMETHODCALLTYPE d2d_effect_properties_SetValueByName(ID2D1Properties *iface,
        const WCHAR *name, D2D1_PROPERTY_TYPE type, const BYTE *value, UINT32 value_size)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    struct d2d_effect_property *prop;

    TRACE("iface %p, name %s, type %u, value %p, value_size %u.\n", iface, debugstr_w(name),
            type, value, value_size);

    if (!(prop = d2d_effect_properties_get_property_by_name(properties, name)))
        return D2DERR_INVALID_PROPERTY;

    return d2d_effect_property_set_value(properties, prop, type, value, value_size);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_properties_SetValue(ID2D1Properties *iface,
        UINT32 index, D2D1_PROPERTY_TYPE type, const BYTE *value, UINT32 value_size)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    struct d2d_effect_property *prop;

    TRACE("iface %p, index %#x, type %u, value %p, value_size %u.\n", iface, index, type, value, value_size);

    if (!(prop = d2d_effect_properties_get_property_by_index(properties, index)))
        return D2DERR_INVALID_PROPERTY;

    return d2d_effect_property_set_value(properties, prop, type, value, value_size);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_properties_GetValueByName(ID2D1Properties *iface,
        const WCHAR *name, D2D1_PROPERTY_TYPE type, BYTE *value, UINT32 value_size)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    struct d2d_effect_property *prop;

    TRACE("iface %p, name %s, type %#x, value %p, value_size %u.\n", iface, debugstr_w(name), type,
            value, value_size);

    if (!(prop = d2d_effect_properties_get_property_by_name(properties, name)))
        return D2DERR_INVALID_PROPERTY;

    return d2d_effect_property_get_value(properties, prop, type, value, value_size);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_properties_GetValue(ID2D1Properties *iface,
        UINT32 index, D2D1_PROPERTY_TYPE type, BYTE *value, UINT32 value_size)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    struct d2d_effect_property *prop;

    TRACE("iface %p, index %#x, type %u, value %p, value_size %u.\n", iface, index, type, value, value_size);

    if (!(prop = d2d_effect_properties_get_property_by_index(properties, index)))
        return D2DERR_INVALID_PROPERTY;

    return d2d_effect_property_get_value(properties, prop, type, value, value_size);
}

static UINT32 STDMETHODCALLTYPE d2d_effect_properties_GetValueSize(ID2D1Properties *iface,
        UINT32 index)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);

    TRACE("iface %p, index %#x.\n", iface, index);

    return d2d_effect_properties_get_value_size(properties, index);
}

static HRESULT STDMETHODCALLTYPE d2d_effect_properties_GetSubProperties(ID2D1Properties *iface,
        UINT32 index, ID2D1Properties **props)
{
    struct d2d_effect_properties *properties = impl_from_ID2D1Properties(iface);
    struct d2d_effect_property *prop;

    TRACE("iface %p, index %u, props %p.\n", iface, index, props);

    if (!(prop = d2d_effect_properties_get_property_by_index(properties, index)))
        return D2DERR_INVALID_PROPERTY;

    if (!prop->subproperties) return D2DERR_NO_SUBPROPERTIES;

    *props = &prop->subproperties->ID2D1Properties_iface;
    ID2D1Properties_AddRef(*props);
    return S_OK;
}

static const ID2D1PropertiesVtbl d2d_effect_properties_vtbl =
{
    d2d_effect_properties_QueryInterface,
    d2d_effect_properties_AddRef,
    d2d_effect_properties_Release,
    d2d_effect_properties_GetPropertyCount,
    d2d_effect_properties_GetPropertyName,
    d2d_effect_properties_GetPropertyNameLength,
    d2d_effect_properties_GetType,
    d2d_effect_properties_GetPropertyIndex,
    d2d_effect_properties_SetValueByName,
    d2d_effect_properties_SetValue,
    d2d_effect_properties_GetValueByName,
    d2d_effect_properties_GetValue,
    d2d_effect_properties_GetValueSize,
    d2d_effect_properties_GetSubProperties,
};

void d2d_effect_init_properties(struct d2d_effect *effect,
        struct d2d_effect_properties *properties)
{
    properties->ID2D1Properties_iface.lpVtbl = &d2d_effect_properties_vtbl;
    properties->effect = effect;
    properties->refcount = 1;
}

static struct d2d_render_info *impl_from_ID2D1DrawInfo(ID2D1DrawInfo *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_render_info, ID2D1DrawInfo_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_draw_info_QueryInterface(ID2D1DrawInfo *iface, REFIID iid,
        void **obj)
{
    TRACE("iface %p, iid %s, obj %p.\n", iface, debugstr_guid(iid), obj);

    if (IsEqualGUID(iid, &IID_ID2D1DrawInfo)
            || IsEqualGUID(iid, &IID_ID2D1RenderInfo)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *obj = iface;
        ID2D1DrawInfo_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_draw_info_AddRef(ID2D1DrawInfo *iface)
{
    struct d2d_render_info *render_info = impl_from_ID2D1DrawInfo(iface);
    ULONG refcount = InterlockedIncrement(&render_info->refcount);

    TRACE("iface %p refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_draw_info_Release(ID2D1DrawInfo *iface)
{
    struct d2d_render_info *render_info = impl_from_ID2D1DrawInfo(iface);
    ULONG refcount = InterlockedDecrement(&render_info->refcount);

    TRACE("iface %p refcount %lu.\n", iface, refcount);

    if (!refcount)
        free(render_info);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d2d_draw_info_SetInputDescription(ID2D1DrawInfo *iface,
        UINT32 index, D2D1_INPUT_DESCRIPTION description)
{
    FIXME("iface %p, index %u stub.\n", iface, index);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_draw_info_SetOutputBuffer(ID2D1DrawInfo *iface,
        D2D1_BUFFER_PRECISION precision, D2D1_CHANNEL_DEPTH depth)
{
    FIXME("iface %p, precision %u, depth %u stub.\n", iface, precision, depth);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_draw_info_SetCached(ID2D1DrawInfo *iface, BOOL is_cached)
{
    FIXME("iface %p, is_cached %d stub.\n", iface, is_cached);
}

static void STDMETHODCALLTYPE d2d_draw_info_SetInstructionCountHint(ID2D1DrawInfo *iface,
        UINT32 count)
{
    FIXME("iface %p, count %u stub.\n", iface, count);
}

static HRESULT STDMETHODCALLTYPE d2d_draw_info_SetPixelShaderConstantBuffer(ID2D1DrawInfo *iface,
        const BYTE *buffer, UINT32 size)
{
    FIXME("iface %p, buffer %p, size %u stub.\n", iface, buffer, size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_draw_info_SetResourceTexture(ID2D1DrawInfo *iface,
        UINT32 index, ID2D1ResourceTexture *texture)
{
    FIXME("iface %p, index %u, texture %p stub.\n", iface, index, texture);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_draw_info_SetVertexShaderConstantBuffer(ID2D1DrawInfo *iface,
        const BYTE *buffer, UINT32 size)
{
    FIXME("iface %p, buffer %p, size %u stub.\n", iface, buffer, size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_draw_info_SetPixelShader(ID2D1DrawInfo *iface,
        REFGUID id, D2D1_PIXEL_OPTIONS options)
{
    struct d2d_render_info *render_info = impl_from_ID2D1DrawInfo(iface);

    TRACE("iface %p, id %s, options %u.\n", iface, debugstr_guid(id), options);

    render_info->mask |= D2D_RENDER_INFO_PIXEL_SHADER;
    render_info->pixel_shader = *id;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_draw_info_SetVertexProcessing(ID2D1DrawInfo *iface,
        ID2D1VertexBuffer *buffer, D2D1_VERTEX_OPTIONS options,
        const D2D1_BLEND_DESCRIPTION *description, const D2D1_VERTEX_RANGE *range,
        const GUID *shader)
{
    FIXME("iface %p, buffer %p, options %#x, description %p, range %p, shader %s stub.\n",
            iface, buffer, options, description, range, debugstr_guid(shader));

    return E_NOTIMPL;
}

static const ID2D1DrawInfoVtbl d2d_draw_info_vtbl =
{
    d2d_draw_info_QueryInterface,
    d2d_draw_info_AddRef,
    d2d_draw_info_Release,
    d2d_draw_info_SetInputDescription,
    d2d_draw_info_SetOutputBuffer,
    d2d_draw_info_SetCached,
    d2d_draw_info_SetInstructionCountHint,
    d2d_draw_info_SetPixelShaderConstantBuffer,
    d2d_draw_info_SetResourceTexture,
    d2d_draw_info_SetVertexShaderConstantBuffer,
    d2d_draw_info_SetPixelShader,
    d2d_draw_info_SetVertexProcessing,
};

static HRESULT d2d_effect_render_info_create(struct d2d_render_info **obj)
{
    struct d2d_render_info *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID2D1DrawInfo_iface.lpVtbl = &d2d_draw_info_vtbl;
    object->refcount = 1;

    *obj = object;

    return S_OK;
}

static bool d2d_transform_node_needs_render_info(const struct d2d_transform_node *node)
{
    static const GUID *iids[] =
    {
        &IID_ID2D1SourceTransform,
        &IID_ID2D1ComputeTransform,
        &IID_ID2D1DrawTransform,
    };
    unsigned int i;
    IUnknown *obj;

    for (i = 0; i < ARRAY_SIZE(iids); ++i)
    {
        if (SUCCEEDED(ID2D1TransformNode_QueryInterface(node->object, iids[i], (void **)&obj)))
        {
            IUnknown_Release(obj);
            return true;
        }
    }

    return false;
}

static HRESULT d2d_effect_transform_graph_initialize_nodes(struct d2d_transform_graph *graph)
{
    ID2D1DrawTransform *draw_transform;
    struct d2d_transform_node *node;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY(node, &graph->nodes, struct d2d_transform_node, entry)
    {
        if (d2d_transform_node_needs_render_info(node))
        {
            if (FAILED(hr = d2d_effect_render_info_create(&node->render_info)))
                return hr;
        }

        if (SUCCEEDED(ID2D1TransformNode_QueryInterface(node->object, &IID_ID2D1DrawTransform,
                (void **)&draw_transform)))
        {
            hr = ID2D1DrawTransform_SetDrawInfo(draw_transform, &node->render_info->ID2D1DrawInfo_iface);
            ID2D1DrawTransform_Release(draw_transform);
            if (FAILED(hr))
            {
                WARN("Failed to set draw info, hr %#lx.\n", hr);
                return hr;
            }
        }
        else
        {
            FIXME("Unsupported node %p.\n", node);
            return E_NOTIMPL;
        }
    }

    return S_OK;
}

HRESULT d2d_effect_create(struct d2d_device_context *context, const CLSID *effect_id,
        ID2D1Effect **effect)
{
    struct d2d_effect_context *effect_context;
    const struct d2d_effect_registration *reg;
    struct d2d_effect *object;
    UINT32 input_count;
    WCHAR clsidW[39];
    HRESULT hr;

    if (!(reg = d2d_factory_get_registered_effect(context->factory, effect_id)))
    {
        WARN("Effect id %s not found.\n", wine_dbgstr_guid(effect_id));
        return D2DERR_EFFECT_IS_NOT_REGISTERED;
    }

    if (!(effect_context = calloc(1, sizeof(*effect_context))))
        return E_OUTOFMEMORY;
    d2d_effect_context_init(effect_context, context);

    if (!(object = calloc(1, sizeof(*object))))
    {
        ID2D1EffectContext_Release(&effect_context->ID2D1EffectContext_iface);
        return E_OUTOFMEMORY;
    }

    object->ID2D1Effect_iface.lpVtbl = &d2d_effect_vtbl;
    object->ID2D1Image_iface.lpVtbl = &d2d_effect_image_vtbl;
    object->refcount = 1;
    object->effect_context = effect_context;

    /* Create properties */
    d2d_effect_duplicate_properties(object, &object->properties, reg->properties);

    StringFromGUID2(effect_id, clsidW, ARRAY_SIZE(clsidW));
    d2d_effect_properties_add(&object->properties, L"CLSID", D2D1_PROPERTY_CLSID, D2D1_PROPERTY_TYPE_CLSID, clsidW);
    d2d_effect_properties_add(&object->properties, L"Cached", D2D1_PROPERTY_CACHED, D2D1_PROPERTY_TYPE_BOOL, L"false");
    d2d_effect_properties_add(&object->properties, L"Precision", D2D1_PROPERTY_PRECISION, D2D1_PROPERTY_TYPE_ENUM, L"0");

    /* Sync instance input count with default input count from the description. */
    d2d_effect_get_value(object, D2D1_PROPERTY_INPUTS, D2D1_PROPERTY_TYPE_ARRAY, (BYTE *)&input_count, sizeof(input_count));
    d2d_effect_set_input_count(object, input_count);

    if (FAILED(hr = d2d_transform_graph_create(input_count, &object->graph)))
    {
        ID2D1EffectContext_Release(&effect_context->ID2D1EffectContext_iface);
        return hr;
    }

    if (FAILED(hr = reg->factory((IUnknown **)&object->impl)))
    {
        WARN("Failed to create implementation object, hr %#lx.\n", hr);
        ID2D1Effect_Release(&object->ID2D1Effect_iface);
        return hr;
    }

    if (FAILED(hr = ID2D1EffectImpl_Initialize(object->impl, &effect_context->ID2D1EffectContext_iface,
            &object->graph->ID2D1TransformGraph_iface)))
    {
        WARN("Failed to initialize effect, hr %#lx.\n", hr);
        ID2D1Effect_Release(&object->ID2D1Effect_iface);
        return hr;
    }

    if (FAILED(hr = d2d_effect_transform_graph_initialize_nodes(object->graph)))
    {
        WARN("Failed to initialize graph nodes, hr %#lx.\n", hr);
        ID2D1Effect_Release(&object->ID2D1Effect_iface);
        return hr;
    }

    *effect = &object->ID2D1Effect_iface;

    TRACE("Created effect %p.\n", *effect);

    return S_OK;
}
