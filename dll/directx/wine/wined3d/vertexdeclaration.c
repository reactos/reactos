/*
 * vertex declaration implementation
 *
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_decl);

static void dump_wined3d_vertex_element(const struct wined3d_vertex_element *element)
{
    TRACE("                 format: %s (%#x)\n", debug_d3dformat(element->format), element->format);
    TRACE("             input_slot: %u\n", element->input_slot);
    TRACE("                 offset: %u\n", element->offset);
    TRACE("            output_slot: %u\n", element->output_slot);
    TRACE("       input slot class: %s\n", debug_d3dinput_classification(element->input_slot_class));
    TRACE("instance data step rate: %u\n", element->instance_data_step_rate);
    TRACE("                 method: %s (%#x)\n", debug_d3ddeclmethod(element->method), element->method);
    TRACE("                  usage: %s (%#x)\n", debug_d3ddeclusage(element->usage), element->usage);
    TRACE("              usage_idx: %u\n", element->usage_idx);
}

ULONG CDECL wined3d_vertex_declaration_incref(struct wined3d_vertex_declaration *declaration)
{
    unsigned int refcount = InterlockedIncrement(&declaration->ref);

    TRACE("%p increasing refcount to %u.\n", declaration, refcount);

    return refcount;
}

static void wined3d_vertex_declaration_destroy_object(void *object)
{
    struct wined3d_vertex_declaration *declaration = object;

    TRACE("declaration %p.\n", declaration);

    free(declaration->elements);
    free(declaration);
}

ULONG CDECL wined3d_vertex_declaration_decref(struct wined3d_vertex_declaration *declaration)
{
    unsigned int refcount = InterlockedDecrement(&declaration->ref);

    TRACE("%p decreasing refcount to %u.\n", declaration, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        declaration->parent_ops->wined3d_object_destroyed(declaration->parent);
        wined3d_cs_destroy_object(declaration->device->cs,
                wined3d_vertex_declaration_destroy_object, declaration);
        wined3d_mutex_unlock();
    }

    return refcount;
}

void * CDECL wined3d_vertex_declaration_get_parent(const struct wined3d_vertex_declaration *declaration)
{
    TRACE("declaration %p.\n", declaration);

    return declaration->parent;
}

static BOOL declaration_element_valid_ffp(const struct wined3d_vertex_element *element)
{
    switch(element->usage)
    {
        case WINED3D_DECL_USAGE_POSITION:
        case WINED3D_DECL_USAGE_POSITIONT:
            switch(element->format)
            {
                case WINED3DFMT_R32G32_FLOAT:
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_R16G16_SINT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R16G16_FLOAT:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3D_DECL_USAGE_BLEND_WEIGHT:
            switch(element->format)
            {
                case WINED3DFMT_R32_FLOAT:
                case WINED3DFMT_R32G32_FLOAT:
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_B8G8R8A8_UNORM:
                case WINED3DFMT_R8G8B8A8_UINT:
                case WINED3DFMT_R16G16_SINT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R16G16_FLOAT:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3D_DECL_USAGE_NORMAL:
            switch(element->format)
            {
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3D_DECL_USAGE_TEXCOORD:
            switch(element->format)
            {
                case WINED3DFMT_R32_FLOAT:
                case WINED3DFMT_R32G32_FLOAT:
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_R16G16_SINT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R16G16_FLOAT:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3D_DECL_USAGE_COLOR:
            switch(element->format)
            {
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_B8G8R8A8_UNORM:
                case WINED3DFMT_R8G8B8A8_UINT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R8G8B8A8_UNORM:
                case WINED3DFMT_R16G16B16A16_SNORM:
                case WINED3DFMT_R16G16B16A16_UNORM:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}

static HRESULT vertexdeclaration_init(struct wined3d_vertex_declaration *declaration,
        struct wined3d_device *device, const struct wined3d_vertex_element *elements, UINT element_count,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_adapter *adapter = device->adapter;
    unsigned int i;

    if (TRACE_ON(d3d_decl))
    {
        for (i = 0; i < element_count; ++i)
        {
            dump_wined3d_vertex_element(elements + i);
        }
    }

    declaration->ref = 1;
    declaration->parent = parent;
    declaration->parent_ops = parent_ops;
    declaration->device = device;
    if (!(declaration->elements = calloc(element_count, sizeof(*declaration->elements))))
    {
        ERR("Failed to allocate elements memory.\n");
        return E_OUTOFMEMORY;
    }
    declaration->element_count = element_count;

    /* Do some static analysis on the elements to make reading the
     * declaration more comfortable for the drawing code. */
    for (i = 0; i < element_count; ++i)
    {
        struct wined3d_vertex_declaration_element *e = &declaration->elements[i];
        unsigned int alignment;

        e->format = wined3d_get_format(adapter, elements[i].format, 0);
        e->ffp_valid = declaration_element_valid_ffp(&elements[i]);
        e->input_slot = elements[i].input_slot;
        e->offset = elements[i].offset;
        e->output_slot = elements[i].output_slot;
        e->input_slot_class = elements[i].input_slot_class;
        e->instance_data_step_rate = elements[i].instance_data_step_rate;
        e->method = elements[i].method;
        e->usage = elements[i].usage;
        e->usage_idx = elements[i].usage_idx;

        if ((alignment = e->format->byte_count) > 4)
            alignment = 4;

        if (e->usage == WINED3D_DECL_USAGE_POSITIONT)
            declaration->position_transformed = true;
        else if (e->usage == WINED3D_DECL_USAGE_PSIZE)
            declaration->point_size = true;
        else if (e->usage == WINED3D_DECL_USAGE_COLOR && !e->usage_idx)
            declaration->diffuse = true;
        else if (e->usage == WINED3D_DECL_USAGE_COLOR && e->usage_idx == 1)
            declaration->specular = true;
        else if (e->usage == WINED3D_DECL_USAGE_NORMAL)
            declaration->normal = true;
        else if (e->usage == WINED3D_DECL_USAGE_TEXCOORD)
            declaration->texcoords |= (1u << e->usage_idx);

        /* Find the streams used in the declaration. The vertex buffers have
         * to be loaded when drawing, but filter tessellation pseudo streams. */
        if (e->input_slot >= WINED3D_MAX_STREAMS)
            continue;

        if (!(e->format->caps[WINED3D_GL_RES_TYPE_BUFFER] & WINED3D_FORMAT_CAP_VERTEX_ATTRIBUTE))
        {
            FIXME("The application tries to use an unsupported format (%s).\n",
                    debug_d3dformat(elements[i].format));
            free(declaration->elements);
            return E_INVALIDARG;
        }

        if (e->offset == WINED3D_APPEND_ALIGNED_ELEMENT)
        {
            const struct wined3d_vertex_declaration_element *prev;
            unsigned int j;

            e->offset = 0;
            for (j = 1; j <= i; ++j)
            {
                prev = &declaration->elements[i - j];
                if (prev->input_slot == e->input_slot)
                {
                    e->offset = (prev->offset + prev->format->byte_count + alignment - 1) & ~(alignment - 1);
                    break;
                }
            }
        }

        if (e->offset & (alignment - 1))
        {
            WARN("Declaration element %u with format %s and offset %u is not %u byte aligned.\n",
                    i, debug_d3dformat(elements[i].format), e->offset, alignment);
            free(declaration->elements);
            return E_INVALIDARG;
        }
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_vertex_declaration_create(struct wined3d_device *device,
        const struct wined3d_vertex_element *elements, UINT element_count, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_vertex_declaration **declaration)
{
    struct wined3d_vertex_declaration *object;
    HRESULT hr;

    TRACE("device %p, elements %p, element_count %u, parent %p, parent_ops %p, declaration %p.\n",
            device, elements, element_count, parent, parent_ops, declaration);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = vertexdeclaration_init(object, device, elements, element_count, parent, parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex declaration, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created vertex declaration %p.\n", object);
    *declaration = object;

    return WINED3D_OK;
}

struct wined3d_fvf_convert_state
{
    const struct wined3d_adapter *adapter;
    struct wined3d_vertex_element *elements;
    unsigned int offset;
    unsigned int idx;
};

static void append_decl_element(struct wined3d_fvf_convert_state *state,
        enum wined3d_format_id format_id, enum wined3d_decl_usage usage, UINT usage_idx)
{
    struct wined3d_vertex_element *elements = state->elements;
    const struct wined3d_format *format;
    UINT offset = state->offset;
    UINT idx = state->idx;

    elements[idx].format = format_id;
    elements[idx].input_slot = 0;
    elements[idx].offset = offset;
    elements[idx].output_slot = WINED3D_OUTPUT_SLOT_SEMANTIC;
    elements[idx].input_slot_class = WINED3D_INPUT_PER_VERTEX_DATA;
    elements[idx].instance_data_step_rate = 0;
    elements[idx].method = WINED3D_DECL_METHOD_DEFAULT;
    elements[idx].usage = usage;
    elements[idx].usage_idx = usage_idx;

    format = wined3d_get_format(state->adapter, format_id, 0);
    state->offset += format->byte_count;
    ++state->idx;
}

static unsigned int convert_fvf_to_declaration(const struct wined3d_adapter *adapter,
        uint32_t fvf, struct wined3d_vertex_element **elements)
{
    BOOL has_pos = !!(fvf & WINED3DFVF_POSITION_MASK);
    BOOL has_blend = (fvf & WINED3DFVF_XYZB5) > WINED3DFVF_XYZRHW;
    BOOL has_blend_idx = has_blend &&
       (((fvf & WINED3DFVF_XYZB5) == WINED3DFVF_XYZB5) ||
        (fvf & WINED3DFVF_LASTBETA_D3DCOLOR) ||
        (fvf & WINED3DFVF_LASTBETA_UBYTE4));
    BOOL has_normal = !!(fvf & WINED3DFVF_NORMAL);
    BOOL has_psize = !!(fvf & WINED3DFVF_PSIZE);
    BOOL has_diffuse = !!(fvf & WINED3DFVF_DIFFUSE);
    BOOL has_specular = !!(fvf & WINED3DFVF_SPECULAR);

    unsigned int num_textures = (fvf & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;
    unsigned int texcoords = (fvf & 0xffff0000) >> 16;
    struct wined3d_fvf_convert_state state;
    unsigned int size;
    unsigned int idx;
    unsigned int num_blends = 1 + (((fvf & WINED3DFVF_XYZB5) - WINED3DFVF_XYZB1) >> 1);
    if (has_blend_idx) num_blends--;

    /* Compute declaration size */
    size = has_pos + (has_blend && num_blends > 0) + has_blend_idx + has_normal +
           has_psize + has_diffuse + has_specular + num_textures;

    state.adapter = adapter;
    if (!(state.elements = calloc(size, sizeof(*state.elements))))
        return ~0u;
    state.offset = 0;
    state.idx = 0;

    if (has_pos)
    {
        if (!has_blend && (fvf & WINED3DFVF_XYZRHW))
            append_decl_element(&state, WINED3DFMT_R32G32B32A32_FLOAT, WINED3D_DECL_USAGE_POSITIONT, 0);
        else if ((fvf & WINED3DFVF_XYZW) == WINED3DFVF_XYZW)
            append_decl_element(&state, WINED3DFMT_R32G32B32A32_FLOAT, WINED3D_DECL_USAGE_POSITION, 0);
        else
            append_decl_element(&state, WINED3DFMT_R32G32B32_FLOAT, WINED3D_DECL_USAGE_POSITION, 0);
    }

    if (has_blend && (num_blends > 0))
    {
        if ((fvf & WINED3DFVF_XYZB5) == WINED3DFVF_XYZB2 && (fvf & WINED3DFVF_LASTBETA_D3DCOLOR))
            append_decl_element(&state, WINED3DFMT_B8G8R8A8_UNORM, WINED3D_DECL_USAGE_BLEND_WEIGHT, 0);
        else
        {
            switch (num_blends)
            {
                case 1:
                    append_decl_element(&state, WINED3DFMT_R32_FLOAT, WINED3D_DECL_USAGE_BLEND_WEIGHT, 0);
                    break;
                case 2:
                    append_decl_element(&state, WINED3DFMT_R32G32_FLOAT, WINED3D_DECL_USAGE_BLEND_WEIGHT, 0);
                    break;
                case 3:
                    append_decl_element(&state, WINED3DFMT_R32G32B32_FLOAT, WINED3D_DECL_USAGE_BLEND_WEIGHT, 0);
                    break;
                case 4:
                    append_decl_element(&state, WINED3DFMT_R32G32B32A32_FLOAT, WINED3D_DECL_USAGE_BLEND_WEIGHT, 0);
                    break;
                default:
                    ERR("Unexpected amount of blend values: %u\n", num_blends);
            }
        }
    }

    if (has_blend_idx)
    {
        if ((fvf & WINED3DFVF_LASTBETA_UBYTE4)
                || ((fvf & WINED3DFVF_XYZB5) == WINED3DFVF_XYZB2 && (fvf & WINED3DFVF_LASTBETA_D3DCOLOR)))
            append_decl_element(&state, WINED3DFMT_R8G8B8A8_UINT, WINED3D_DECL_USAGE_BLEND_INDICES, 0);
        else if (fvf & WINED3DFVF_LASTBETA_D3DCOLOR)
            append_decl_element(&state, WINED3DFMT_B8G8R8A8_UNORM, WINED3D_DECL_USAGE_BLEND_INDICES, 0);
        else
            append_decl_element(&state, WINED3DFMT_R32_FLOAT, WINED3D_DECL_USAGE_BLEND_INDICES, 0);
    }

    if (has_normal)
        append_decl_element(&state, WINED3DFMT_R32G32B32_FLOAT, WINED3D_DECL_USAGE_NORMAL, 0);
    if (has_psize)
        append_decl_element(&state, WINED3DFMT_R32_FLOAT, WINED3D_DECL_USAGE_PSIZE, 0);
    if (has_diffuse)
        append_decl_element(&state, WINED3DFMT_B8G8R8A8_UNORM, WINED3D_DECL_USAGE_COLOR, 0);
    if (has_specular)
        append_decl_element(&state, WINED3DFMT_B8G8R8A8_UNORM, WINED3D_DECL_USAGE_COLOR, 1);

    for (idx = 0; idx < num_textures; ++idx)
    {
        switch ((texcoords >> (idx * 2)) & 0x03)
        {
            case WINED3DFVF_TEXTUREFORMAT1:
                append_decl_element(&state, WINED3DFMT_R32_FLOAT, WINED3D_DECL_USAGE_TEXCOORD, idx);
                break;
            case WINED3DFVF_TEXTUREFORMAT2:
                append_decl_element(&state, WINED3DFMT_R32G32_FLOAT, WINED3D_DECL_USAGE_TEXCOORD, idx);
                break;
            case WINED3DFVF_TEXTUREFORMAT3:
                append_decl_element(&state, WINED3DFMT_R32G32B32_FLOAT, WINED3D_DECL_USAGE_TEXCOORD, idx);
                break;
            case WINED3DFVF_TEXTUREFORMAT4:
                append_decl_element(&state, WINED3DFMT_R32G32B32A32_FLOAT, WINED3D_DECL_USAGE_TEXCOORD, idx);
                break;
        }
    }

    *elements = state.elements;
    return size;
}

HRESULT CDECL wined3d_vertex_declaration_create_from_fvf(struct wined3d_device *device,
        uint32_t fvf, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_vertex_declaration **declaration)
{
    struct wined3d_vertex_element *elements;
    unsigned int size;
    DWORD hr;

    TRACE("device %p, fvf %#x, parent %p, parent_ops %p, declaration %p.\n",
            device, fvf, parent, parent_ops, declaration);

    size = convert_fvf_to_declaration(device->adapter, fvf, &elements);
    if (size == ~0u)
        return E_OUTOFMEMORY;

    hr = wined3d_vertex_declaration_create(device, elements, size, parent, parent_ops, declaration);
    free(elements);
    return hr;
}
