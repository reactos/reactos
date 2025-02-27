/*
 * Copyright 2002 Lionel Ulmer
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2006-2008 Henri Verbeet
 * Copyright 2007 Andrew Riedi
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
 * Copyright 2016, 2018 Józef Kucia for CodeWeavers
 * Copyright 2020 Zebediah Figura
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
#include "wined3d_gl.h"
#include "wined3d_vk.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

struct light_transformed
{
    struct wined3d_color diffuse, specular, ambient;
    struct wined3d_vec4 position;
    struct wined3d_vec3 direction;
    float range, falloff, c_att, l_att, q_att, cos_htheta, cos_hphi;
};

struct lights_settings
{
    struct light_transformed lights[WINED3D_MAX_SOFTWARE_ACTIVE_LIGHTS];
    struct wined3d_color ambient_light;
    struct wined3d_matrix modelview_matrix;
    struct wined3d_matrix normal_matrix;
    struct wined3d_vec4 position_transformed;

    float fog_start, fog_end, fog_density;

    uint32_t point_light_count          : 8;
    uint32_t spot_light_count           : 8;
    uint32_t directional_light_count    : 8;
    uint32_t parallel_point_light_count : 8;
    uint32_t lighting                   : 1;
    uint32_t legacy_lighting            : 1;
    uint32_t normalise                  : 1;
    uint32_t localviewer                : 1;
    uint32_t fog_coord_mode             : 2;
    uint32_t fog_mode                   : 2;
    uint32_t padding                    : 24;
};

/* Define the default light parameters as specified by MSDN. */
const struct wined3d_light WINED3D_default_light =
{
    WINED3D_LIGHT_DIRECTIONAL,  /* Type */
    { 1.0f, 1.0f, 1.0f, 0.0f }, /* Diffuse r,g,b,a */
    { 0.0f, 0.0f, 0.0f, 0.0f }, /* Specular r,g,b,a */
    { 0.0f, 0.0f, 0.0f, 0.0f }, /* Ambient r,g,b,a, */
    { 0.0f, 0.0f, 0.0f },       /* Position x,y,z */
    { 0.0f, 0.0f, 1.0f },       /* Direction x,y,z */
    0.0f,                       /* Range */
    0.0f,                       /* Falloff */
    0.0f, 0.0f, 0.0f,           /* Attenuation 0,1,2 */
    0.0f,                       /* Theta */
    0.0f                        /* Phi */
};

BOOL device_context_add(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context **new_array;

    TRACE("Adding context %p.\n", context);

    if (!device->shader_backend->shader_allocate_context_data(context))
    {
        ERR("Failed to allocate shader backend context data.\n");
        return FALSE;
    }
    device->shader_backend->shader_init_context_state(context);

    if (!device->adapter->fragment_pipe->allocate_context_data(context))
    {
        ERR("Failed to allocate fragment pipeline context data.\n");
        device->shader_backend->shader_free_context_data(context);
        return FALSE;
    }

    if (!(new_array = realloc(device->contexts, sizeof(*new_array) * (device->context_count + 1))))
    {
        ERR("Failed to grow the context array.\n");
        device->adapter->fragment_pipe->free_context_data(context);
        device->shader_backend->shader_free_context_data(context);
        return FALSE;
    }

    new_array[device->context_count++] = context;
    device->contexts = new_array;

    return TRUE;
}

void device_context_remove(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context **new_array;
    BOOL found = FALSE;
    UINT i;

    TRACE("Removing context %p.\n", context);

    device->adapter->fragment_pipe->free_context_data(context);
    device->shader_backend->shader_free_context_data(context);

    for (i = 0; i < device->context_count; ++i)
    {
        if (device->contexts[i] == context)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        ERR("Context %p doesn't exist in context array.\n", context);
        return;
    }

    if (!--device->context_count)
    {
        free(device->contexts);
        device->contexts = NULL;
        return;
    }

    memmove(&device->contexts[i], &device->contexts[i + 1], (device->context_count - i) * sizeof(*device->contexts));
    if (!(new_array = realloc(device->contexts, device->context_count * sizeof(*device->contexts))))
    {
        ERR("Failed to shrink context array. Oh well.\n");
        return;
    }

    device->contexts = new_array;
}

ULONG CDECL wined3d_device_incref(struct wined3d_device *device)
{
    unsigned int refcount = InterlockedIncrement(&device->ref);

    TRACE("%p increasing refcount to %u.\n", device, refcount);

    return refcount;
}

static void device_free_so_desc(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_so_desc_entry *s = WINE_RB_ENTRY_VALUE(entry, struct wined3d_so_desc_entry, entry);

    free(s);
}

static void device_leftover_sampler(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_sampler *sampler = WINE_RB_ENTRY_VALUE(entry, struct wined3d_sampler, entry);

    ERR("Leftover sampler %p.\n", sampler);
}

static void device_leftover_rasterizer_state(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_rasterizer_state *state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_rasterizer_state, entry);

    ERR("Leftover rasterizer state %p.\n", state);
}

static void device_leftover_blend_state(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_blend_state *blend_state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_blend_state, entry);

    ERR("Leftover blend state %p.\n", blend_state);
}

static void device_leftover_depth_stencil_state(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_depth_stencil_state *state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_depth_stencil_state, entry);

    ERR("Leftover depth/stencil state %p.\n", state);
}

void wined3d_device_cleanup(struct wined3d_device *device)
{
    unsigned int i;

    if (device->swapchain_count)
        wined3d_device_uninit_3d(device);

    wined3d_cs_destroy(device->cs);

    for (i = 0; i < ARRAY_SIZE(device->multistate_funcs); ++i)
    {
        free(device->multistate_funcs[i]);
        device->multistate_funcs[i] = NULL;
    }

    if (!list_empty(&device->resources))
    {
        struct wined3d_resource *resource;

        ERR("Device released with resources still bound.\n");

        LIST_FOR_EACH_ENTRY(resource, &device->resources, struct wined3d_resource, resource_list_entry)
        {
            ERR("Leftover resource %p with type %s (%#x).\n",
                    resource, debug_d3dresourcetype(resource->type), resource->type);
        }
    }

    if (device->contexts)
        ERR("Context array not freed!\n");
    if (device->hardwareCursor)
        DestroyCursor(device->hardwareCursor);
    device->hardwareCursor = 0;

    wine_rb_destroy(&device->samplers, device_leftover_sampler, NULL);
    wine_rb_destroy(&device->rasterizer_states, device_leftover_rasterizer_state, NULL);
    wine_rb_destroy(&device->blend_states, device_leftover_blend_state, NULL);
    wine_rb_destroy(&device->depth_stencil_states, device_leftover_depth_stencil_state, NULL);
    wine_rb_destroy(&device->so_descs, device_free_so_desc, NULL);

    wined3d_lock_cleanup(&device->bo_map_lock);

    wined3d_decref(device->wined3d);
    device->wined3d = NULL;
}

ULONG CDECL wined3d_device_decref(struct wined3d_device *device)
{
    unsigned int refcount = InterlockedDecrement(&device->ref);

    TRACE("%p decreasing refcount to %u.\n", device, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        device->adapter->adapter_ops->adapter_destroy_device(device);
        TRACE("Destroyed device %p.\n", device);
        wined3d_mutex_unlock();
    }

    return refcount;
}

ULONG CDECL wined3d_blend_state_incref(struct wined3d_blend_state *state)
{
    unsigned int refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %u.\n", state, refcount);

    return refcount;
}

static void wined3d_blend_state_destroy_object(void *object)
{
    TRACE("object %p.\n", object);

    free(object);
}

ULONG CDECL wined3d_blend_state_decref(struct wined3d_blend_state *state)
{
    unsigned int refcount = wined3d_atomic_decrement_mutex_lock(&state->refcount);
    struct wined3d_device *device = state->device;

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        state->parent_ops->wined3d_object_destroyed(state->parent);
        wined3d_cs_destroy_object(device->cs, wined3d_blend_state_destroy_object, state);
        wined3d_mutex_unlock();
    }

    return refcount;
}

void * CDECL wined3d_blend_state_get_parent(const struct wined3d_blend_state *state)
{
    TRACE("state %p.\n", state);

    return state->parent;
}

static bool is_dual_source(enum wined3d_blend state)
{
    return state >= WINED3D_BLEND_SRC1COLOR && state <= WINED3D_BLEND_INVSRC1ALPHA;
}

HRESULT CDECL wined3d_blend_state_create(struct wined3d_device *device,
        const struct wined3d_blend_state_desc *desc, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_blend_state **state)
{
    struct wined3d_blend_state *object;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, state %p.\n",
            device, desc, parent, parent_ops, state);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->refcount = 1;
    object->desc = *desc;
    object->parent = parent;
    object->parent_ops = parent_ops;
    object->device = device;

    object->dual_source = desc->rt[0].enable
            && (is_dual_source(desc->rt[0].src)
            || is_dual_source(desc->rt[0].dst)
            || is_dual_source(desc->rt[0].src_alpha)
            || is_dual_source(desc->rt[0].dst_alpha));

    TRACE("Created blend state %p.\n", object);
    *state = object;

    return WINED3D_OK;
}

ULONG CDECL wined3d_depth_stencil_state_incref(struct wined3d_depth_stencil_state *state)
{
    unsigned int refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %u.\n", state, refcount);

    return refcount;
}

static void wined3d_depth_stencil_state_destroy_object(void *object)
{
    TRACE("object %p.\n", object);

    free(object);
}

ULONG CDECL wined3d_depth_stencil_state_decref(struct wined3d_depth_stencil_state *state)
{
    unsigned int refcount = wined3d_atomic_decrement_mutex_lock(&state->refcount);
    struct wined3d_device *device = state->device;

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        state->parent_ops->wined3d_object_destroyed(state->parent);
        wined3d_cs_destroy_object(device->cs, wined3d_depth_stencil_state_destroy_object, state);
        wined3d_mutex_unlock();
    }

    return refcount;
}

void * CDECL wined3d_depth_stencil_state_get_parent(const struct wined3d_depth_stencil_state *state)
{
    TRACE("state %p.\n", state);

    return state->parent;
}

static bool stencil_op_writes_ds(const struct wined3d_stencil_op_desc *desc)
{
    return desc->fail_op != WINED3D_STENCIL_OP_KEEP
            || desc->depth_fail_op != WINED3D_STENCIL_OP_KEEP
            || desc->pass_op != WINED3D_STENCIL_OP_KEEP;
}

static bool depth_stencil_state_desc_writes_ds(const struct wined3d_depth_stencil_state_desc *desc)
{
    if (desc->depth && desc->depth_write)
        return true;

    if (desc->stencil && desc->stencil_write_mask)
    {
        if (stencil_op_writes_ds(&desc->front) || stencil_op_writes_ds(&desc->back))
            return true;
    }

    return false;
}

HRESULT CDECL wined3d_depth_stencil_state_create(struct wined3d_device *device,
        const struct wined3d_depth_stencil_state_desc *desc, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_depth_stencil_state **state)
{
    struct wined3d_depth_stencil_state *object;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, state %p.\n",
            device, desc, parent, parent_ops, state);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->refcount = 1;
    object->desc = *desc;
    object->parent = parent;
    object->parent_ops = parent_ops;
    object->device = device;

    object->writes_ds = depth_stencil_state_desc_writes_ds(desc);

    TRACE("Created depth/stencil state %p.\n", object);
    *state = object;

    return WINED3D_OK;
}

ULONG CDECL wined3d_rasterizer_state_incref(struct wined3d_rasterizer_state *state)
{
    unsigned int refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %u.\n", state, refcount);

    return refcount;
}

static void wined3d_rasterizer_state_destroy_object(void *object)
{
    TRACE("object %p.\n", object);

    free(object);
}

ULONG CDECL wined3d_rasterizer_state_decref(struct wined3d_rasterizer_state *state)
{
    unsigned int refcount = wined3d_atomic_decrement_mutex_lock(&state->refcount);
    struct wined3d_device *device = state->device;

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        state->parent_ops->wined3d_object_destroyed(state->parent);
        wined3d_cs_destroy_object(device->cs, wined3d_rasterizer_state_destroy_object, state);
        wined3d_mutex_unlock();
    }

    return refcount;
}

void * CDECL wined3d_rasterizer_state_get_parent(const struct wined3d_rasterizer_state *state)
{
    TRACE("rasterizer_state %p.\n", state);

    return state->parent;
}

HRESULT CDECL wined3d_rasterizer_state_create(struct wined3d_device *device,
        const struct wined3d_rasterizer_state_desc *desc, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_rasterizer_state **state)
{
    struct wined3d_rasterizer_state *object;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, state %p.\n",
            device, desc, parent, parent_ops, state);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->refcount = 1;
    object->desc = *desc;
    object->parent = parent;
    object->parent_ops = parent_ops;
    object->device = device;

    TRACE("Created rasterizer state %p.\n", object);
    *state = object;

    return WINED3D_OK;
}

UINT CDECL wined3d_device_get_swapchain_count(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->swapchain_count;
}

struct wined3d_swapchain * CDECL wined3d_device_get_swapchain(const struct wined3d_device *device, UINT swapchain_idx)
{
    TRACE("device %p, swapchain_idx %u.\n", device, swapchain_idx);

    if (swapchain_idx >= device->swapchain_count)
    {
        WARN("swapchain_idx %u >= swapchain_count %u.\n",
                swapchain_idx, device->swapchain_count);
        return NULL;
    }

    return device->swapchains[swapchain_idx];
}

static void device_load_logo(struct wined3d_device *device, const char *filename)
{
    struct wined3d_color_key color_key;
    struct wined3d_resource_desc desc;
    HBITMAP hbm;
    BITMAP bm;
    HRESULT hr;
    HDC dcb = NULL, dcs = NULL;

    if (!(hbm = LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION)))
    {
        ERR_(winediag)("Failed to load logo %s.\n", wine_dbgstr_a(filename));
        return;
    }
    GetObjectA(hbm, sizeof(BITMAP), &bm);

    if (!(dcb = CreateCompatibleDC(NULL)))
        goto out;
    SelectObject(dcb, hbm);

    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = WINED3DFMT_B5G6R5_UNORM;
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = WINED3DUSAGE_DYNAMIC;
    desc.bind_flags = 0;
    desc.access = WINED3D_RESOURCE_ACCESS_GPU;
    desc.width = bm.bmWidth;
    desc.height = bm.bmHeight;
    desc.depth = 1;
    desc.size = 0;
    if (FAILED(hr = wined3d_texture_create(device, &desc, 1, 1, WINED3D_TEXTURE_CREATE_GET_DC,
            NULL, NULL, &wined3d_null_parent_ops, &device->logo_texture)))
    {
        ERR("Wine logo requested, but failed to create texture, hr %#lx.\n", hr);
        goto out;
    }

    if (FAILED(hr = wined3d_texture_get_dc(device->logo_texture, 0, &dcs)))
    {
        wined3d_texture_decref(device->logo_texture);
        device->logo_texture = NULL;
        goto out;
    }
    BitBlt(dcs, 0, 0, bm.bmWidth, bm.bmHeight, dcb, 0, 0, SRCCOPY);
    wined3d_texture_release_dc(device->logo_texture, 0, dcs);

    color_key.color_space_low_value = 0;
    color_key.color_space_high_value = 0;
    wined3d_texture_set_color_key(device->logo_texture, WINED3D_CKEY_SRC_BLT, &color_key);

out:
    if (dcb) DeleteDC(dcb);
    if (hbm) DeleteObject(hbm);
}

/* Context activation is done by the caller. */
static void wined3d_device_gl_create_dummy_textures(struct wined3d_device_gl *device_gl,
        struct wined3d_context_gl *context_gl)
{
    struct wined3d_dummy_textures *textures = &device_gl->dummy_textures;
    const struct wined3d_d3d_info *d3d_info = context_gl->c.d3d_info;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int i;
    DWORD color;

    if (d3d_info->wined3d_creation_flags & WINED3D_LEGACY_UNBOUND_RESOURCE_COLOR)
        color = 0x000000ff;
    else
        color = 0x00000000;

    /* Under DirectX you can sample even if no texture is bound, whereas
     * OpenGL will only allow that when a valid texture is bound.
     * We emulate this by creating dummy textures and binding them
     * to each texture stage when the currently set D3D texture is NULL. */
    wined3d_context_gl_active_texture(context_gl, gl_info, 0);

    gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_1d);
    TRACE("Dummy 1D texture given name %u.\n", textures->tex_1d);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, textures->tex_1d);
    gl_info->gl_ops.gl.p_glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 1, 0,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);

    gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_2d);
    TRACE("Dummy 2D texture given name %u.\n", textures->tex_2d);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, textures->tex_2d);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_3d);
        TRACE("Dummy 3D texture given name %u.\n", textures->tex_3d);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, textures->tex_3d);
        GL_EXTCALL(glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 1, 1, 1, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_cube);
        TRACE("Dummy cube texture given name %u.\n", textures->tex_cube);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, textures->tex_cube);
        for (i = GL_TEXTURE_CUBE_MAP_POSITIVE_X; i <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++i)
        {
            gl_info->gl_ops.gl.p_glTexImage2D(i, 0, GL_RGBA8, 1, 1, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
        }
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP_ARRAY])
    {
        DWORD cube_array_data[6];

        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_cube_array);
        TRACE("Dummy cube array texture given name %u.\n", textures->tex_cube_array);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures->tex_cube_array);
        for (i = 0; i < ARRAY_SIZE(cube_array_data); ++i)
            cube_array_data[i] = color;
        GL_EXTCALL(glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA8, 1, 1, 6, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, cube_array_data));
    }

    if (gl_info->supported[EXT_TEXTURE_ARRAY])
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_1d_array);
        TRACE("Dummy 1D array texture given name %u.\n", textures->tex_1d_array);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D_ARRAY, textures->tex_1d_array);
        gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, GL_RGBA8, 1, 1, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);

        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_2d_array);
        TRACE("Dummy 2D array texture given name %u.\n", textures->tex_2d_array);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_ARRAY, textures->tex_2d_array);
        GL_EXTCALL(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 1, 1, 1, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
    }

    if (gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
    {
        GLuint buffer;

        GL_EXTCALL(glGenBuffers(1, &buffer));
        GL_EXTCALL(glBindBuffer(GL_TEXTURE_BUFFER, buffer));
        GL_EXTCALL(glBufferData(GL_TEXTURE_BUFFER, sizeof(color), &color, GL_STATIC_DRAW));
        GL_EXTCALL(glBindBuffer(GL_TEXTURE_BUFFER, 0));

        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_buffer);
        TRACE("Dummy buffer texture given name %u.\n", textures->tex_buffer);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_BUFFER, textures->tex_buffer);
        GL_EXTCALL(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, buffer));
        GL_EXTCALL(glDeleteBuffers(1, &buffer));
    }

    if (gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_2d_ms);
        TRACE("Dummy multisample texture given name %u.\n", textures->tex_2d_ms);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures->tex_2d_ms);
        GL_EXTCALL(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, 1, 1, GL_TRUE));

        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_2d_ms_array);
        TRACE("Dummy multisample array texture given name %u.\n", textures->tex_2d_ms_array);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, textures->tex_2d_ms_array);
        GL_EXTCALL(glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 1, GL_RGBA8, 1, 1, 1, GL_TRUE));

        if (gl_info->supported[ARB_CLEAR_TEXTURE])
        {
            GL_EXTCALL(glClearTexImage(textures->tex_2d_ms, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
            GL_EXTCALL(glClearTexImage(textures->tex_2d_ms_array, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
        }
        else
        {
            WARN("ARB_clear_texture is currently required to clear dummy multisample textures.\n");
        }
    }

    checkGLcall("create dummy textures");

    wined3d_context_gl_bind_dummy_textures(context_gl);
}

/* Context activation is done by the caller. */
static void wined3d_device_gl_destroy_dummy_textures(struct wined3d_device_gl *device_gl,
        struct wined3d_context_gl *context_gl)
{
    struct wined3d_dummy_textures *dummy_textures = &device_gl->dummy_textures;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_2d_ms);
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_2d_ms_array);
    }

    if (gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_buffer);

    if (gl_info->supported[EXT_TEXTURE_ARRAY])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_2d_array);
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_1d_array);
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP_ARRAY])
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_cube_array);

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_cube);

    if (gl_info->supported[EXT_TEXTURE3D])
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_3d);

    gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_2d);
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_1d);

    checkGLcall("delete dummy textures");

    memset(dummy_textures, 0, sizeof(*dummy_textures));
}

/* Context activation is done by the caller. */
void wined3d_device_create_default_samplers(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_sampler_desc desc;
    HRESULT hr;

    desc.address_u = WINED3D_TADDRESS_WRAP;
    desc.address_v = WINED3D_TADDRESS_WRAP;
    desc.address_w = WINED3D_TADDRESS_WRAP;
    memset(desc.border_color, 0, sizeof(desc.border_color));
    desc.mag_filter = WINED3D_TEXF_POINT;
    desc.min_filter = WINED3D_TEXF_POINT;
    desc.mip_filter = WINED3D_TEXF_NONE;
    desc.lod_bias = 0.0f;
    desc.min_lod = -1000.0f;
    desc.max_lod =  1000.0f;
    desc.mip_base_level = 0;
    desc.max_anisotropy = 1;
    desc.compare = FALSE;
    desc.comparison_func = WINED3D_CMP_NEVER;
    desc.srgb_decode = TRUE;

    /* In SM4+ shaders there is a separation between resources and samplers. Some shader
     * instructions allow access to resources without using samplers.
     * In GLSL, resources are always accessed through sampler or image variables. The default
     * sampler object is used to emulate the direct resource access when there is no sampler state
     * to use.
     */
    if (FAILED(hr = wined3d_sampler_create(device, &desc, NULL, &wined3d_null_parent_ops, &device->default_sampler)))
    {
        ERR("Failed to create default sampler, hr %#lx.\n", hr);
        device->default_sampler = NULL;
    }

    /* In D3D10+, a NULL sampler maps to the default sampler state. */
    desc.address_u = WINED3D_TADDRESS_CLAMP;
    desc.address_v = WINED3D_TADDRESS_CLAMP;
    desc.address_w = WINED3D_TADDRESS_CLAMP;
    desc.mag_filter = WINED3D_TEXF_LINEAR;
    desc.min_filter = WINED3D_TEXF_LINEAR;
    desc.mip_filter = WINED3D_TEXF_LINEAR;
    if (FAILED(hr = wined3d_sampler_create(device, &desc, NULL, &wined3d_null_parent_ops, &device->null_sampler)))
    {
        ERR("Failed to create null sampler, hr %#lx.\n", hr);
        device->null_sampler = NULL;
    }
}

void wined3d_device_destroy_default_samplers(struct wined3d_device *device)
{
    wined3d_sampler_decref(device->default_sampler);
    device->default_sampler = NULL;
    wined3d_sampler_decref(device->null_sampler);
    device->null_sampler = NULL;
}

static bool wined3d_null_image_vk_init(struct wined3d_image_vk *image, struct wined3d_context_vk *context_vk,
        VkCommandBuffer vk_command_buffer, VkImageType type, unsigned int layer_count, unsigned int sample_count)
{
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkImageSubresourceRange range;
    uint32_t flags = 0;

    static const VkClearColorValue colour = {{0}};

    TRACE("image %p, context_vk %p, vk_command_buffer %p, type %#x, layer_count %u, sample_count %u.\n",
            image, context_vk, vk_command_buffer, type, layer_count, sample_count);

    if (type == VK_IMAGE_TYPE_2D && layer_count >= 6)
        flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (!wined3d_context_vk_create_image(context_vk, type,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_FORMAT_R8G8B8A8_UNORM,
            1, 1, 1, sample_count, 1, layer_count, flags, image))
    {
        return false;
    }

    wined3d_context_vk_reference_image(context_vk, image);

    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = layer_count;

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image->vk_image, &range);

    VK_CALL(vkCmdClearColorImage(vk_command_buffer, image->vk_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &colour, 1, &range));

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image->vk_image, &range);

    TRACE("Created NULL image 0x%s, memory 0x%s.\n",
            wine_dbgstr_longlong(image->vk_image), wine_dbgstr_longlong(image->vk_memory));

    return true;
}

bool wined3d_device_vk_create_null_resources(struct wined3d_device_vk *device_vk,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_null_resources_vk *r = &device_vk->null_resources_vk;
    const struct wined3d_vk_info *vk_info;
    const struct wined3d_format *format;
    VkMemoryPropertyFlags memory_type;
    VkCommandBuffer vk_command_buffer;
    unsigned int sample_count = 2;
    VkBufferUsageFlags usage;

    format = wined3d_get_format(device_vk->d.adapter, WINED3DFMT_R8G8B8A8_UNORM, WINED3D_BIND_SHADER_RESOURCE);
    while (sample_count && !(sample_count & format->multisample_types))
        sample_count <<= 1;

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        return false;
    }

    vk_info = context_vk->vk_info;

    usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
            | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    memory_type = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (!wined3d_context_vk_create_bo(context_vk, 16, usage, memory_type, &r->bo))
        return false;
    VK_CALL(vkCmdFillBuffer(vk_command_buffer, r->bo.vk_buffer, r->bo.b.buffer_offset, r->bo.size, 0x00000000u));
    r->buffer_info.buffer = r->bo.vk_buffer;
    r->buffer_info.offset = r->bo.b.buffer_offset;
    r->buffer_info.range = r->bo.size;

    if (!wined3d_null_image_vk_init(&r->image_1d, context_vk, vk_command_buffer, VK_IMAGE_TYPE_1D, 1, 1))
    {
        ERR("Failed to create 1D image.\n");
        goto fail;
    }

    if (!wined3d_null_image_vk_init(&r->image_2d, context_vk, vk_command_buffer, VK_IMAGE_TYPE_2D, 6, 1))
    {
        ERR("Failed to create 2D image.\n");
        goto fail;
    }

    if (!wined3d_null_image_vk_init(&r->image_2dms, context_vk, vk_command_buffer, VK_IMAGE_TYPE_2D, 1, sample_count))
    {
        ERR("Failed to create 2D MSAA image.\n");
        goto fail;
    }

    if (!wined3d_null_image_vk_init(&r->image_3d, context_vk, vk_command_buffer, VK_IMAGE_TYPE_3D, 1, 1))
    {
        ERR("Failed to create 3D image.\n");
        goto fail;
    }

    return true;

fail:
    if (r->image_2dms.vk_image)
        wined3d_context_vk_destroy_image(context_vk, &r->image_2dms);
    if (r->image_2d.vk_image)
        wined3d_context_vk_destroy_image(context_vk, &r->image_2d);
    if (r->image_1d.vk_image)
        wined3d_context_vk_destroy_image(context_vk, &r->image_1d);
    wined3d_context_vk_reference_bo(context_vk, &r->bo);
    wined3d_context_vk_destroy_bo(context_vk, &r->bo);
    return false;
}

void wined3d_device_vk_destroy_null_resources(struct wined3d_device_vk *device_vk,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_null_resources_vk *r = &device_vk->null_resources_vk;

    /* We don't track command buffer references to NULL resources. We easily
     * could, but it doesn't seem worth it. */
    wined3d_context_vk_reference_image(context_vk, &r->image_3d);
    wined3d_context_vk_destroy_image(context_vk, &r->image_3d);
    wined3d_context_vk_reference_image(context_vk, &r->image_2dms);
    wined3d_context_vk_destroy_image(context_vk, &r->image_2dms);
    wined3d_context_vk_reference_image(context_vk, &r->image_2d);
    wined3d_context_vk_destroy_image(context_vk, &r->image_2d);
    wined3d_context_vk_reference_image(context_vk, &r->image_1d);
    wined3d_context_vk_destroy_image(context_vk, &r->image_1d);
    wined3d_context_vk_reference_bo(context_vk, &r->bo);
    wined3d_context_vk_destroy_bo(context_vk, &r->bo);
}

bool wined3d_device_vk_create_null_views(struct wined3d_device_vk *device_vk, struct wined3d_context_vk *context_vk)
{
    struct wined3d_null_resources_vk *r = &device_vk->null_resources_vk;
    struct wined3d_null_views_vk *v = &device_vk->null_views_vk;
    VkBufferViewCreateInfo buffer_create_info;
    VkImageViewUsageCreateInfoKHR usage_desc;
    const struct wined3d_vk_info *vk_info;
    VkImageViewCreateInfo view_desc;
    VkResult vr;

    vk_info = context_vk->vk_info;

    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    buffer_create_info.pNext = NULL;
    buffer_create_info.flags = 0;
    buffer_create_info.buffer = r->bo.vk_buffer;
    buffer_create_info.format = VK_FORMAT_R32_UINT;
    buffer_create_info.offset = r->bo.b.buffer_offset;
    buffer_create_info.range = r->bo.size;

    if ((vr = VK_CALL(vkCreateBufferView(device_vk->vk_device,
            &buffer_create_info, NULL, &v->vk_view_buffer_uint))) < 0)
    {
        ERR("Failed to create buffer view, vr %s.\n", wined3d_debug_vkresult(vr));
        return false;
    }
    TRACE("Created buffer view 0x%s.\n", wine_dbgstr_longlong(v->vk_view_buffer_uint));

    buffer_create_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    if ((vr = VK_CALL(vkCreateBufferView(device_vk->vk_device,
            &buffer_create_info, NULL, &v->vk_view_buffer_float))) < 0)
    {
        ERR("Failed to create buffer view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    TRACE("Created buffer view 0x%s.\n", wine_dbgstr_longlong(v->vk_view_buffer_float));

    view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_desc.pNext = NULL;
    view_desc.flags = 0;
    view_desc.image = r->image_1d.vk_image;
    view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D;
    view_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
    view_desc.components.r = VK_COMPONENT_SWIZZLE_ZERO;
    view_desc.components.g = VK_COMPONENT_SWIZZLE_ZERO;
    view_desc.components.b = VK_COMPONENT_SWIZZLE_ZERO;
    view_desc.components.a = VK_COMPONENT_SWIZZLE_ZERO;
    view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_desc.subresourceRange.baseMipLevel = 0;
    view_desc.subresourceRange.levelCount = 1;
    view_desc.subresourceRange.baseArrayLayer = 0;
    view_desc.subresourceRange.layerCount = 1;

    if (vk_info->supported[WINED3D_VK_KHR_MAINTENANCE2] || vk_info->api_version >= VK_API_VERSION_1_1)
    {
        usage_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO_KHR;
        usage_desc.pNext = NULL;
        usage_desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

        view_desc.pNext = &usage_desc;
    }

    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &v->vk_info_1d.imageView))) < 0)
    {
        ERR("Failed to create 1D image view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    v->vk_info_1d.sampler = VK_NULL_HANDLE;
    v->vk_info_1d.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    TRACE("Created 1D image view 0x%s.\n", wine_dbgstr_longlong(v->vk_info_1d.imageView));

    view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &v->vk_info_1d_array.imageView))) < 0)
    {
        ERR("Failed to create 1D image view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    v->vk_info_1d_array.sampler = VK_NULL_HANDLE;
    v->vk_info_1d_array.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    TRACE("Created 1D array image view 0x%s.\n", wine_dbgstr_longlong(v->vk_info_1d_array.imageView));

    view_desc.image = r->image_2d.vk_image;
    view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &v->vk_info_2d.imageView))) < 0)
    {
        ERR("Failed to create 2D image view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    v->vk_info_2d.sampler = VK_NULL_HANDLE;
    v->vk_info_2d.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    TRACE("Created 2D image view 0x%s.\n", wine_dbgstr_longlong(v->vk_info_2d.imageView));

    view_desc.image = r->image_2dms.vk_image;
    view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &v->vk_info_2dms.imageView))) < 0)
    {
        ERR("Failed to create 2D MSAA image view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    v->vk_info_2dms.sampler = VK_NULL_HANDLE;
    v->vk_info_2dms.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    TRACE("Created 2D MSAA image view 0x%s.\n", wine_dbgstr_longlong(v->vk_info_2dms.imageView));

    view_desc.image = r->image_3d.vk_image;
    view_desc.viewType = VK_IMAGE_VIEW_TYPE_3D;
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &v->vk_info_3d.imageView))) < 0)
    {
        ERR("Failed to create 3D image view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    v->vk_info_3d.sampler = VK_NULL_HANDLE;
    v->vk_info_3d.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    TRACE("Created 3D image view 0x%s.\n", wine_dbgstr_longlong(v->vk_info_3d.imageView));

    view_desc.image = r->image_2d.vk_image;
    view_desc.subresourceRange.layerCount = 6;
    view_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &v->vk_info_cube.imageView))) < 0)
    {
        ERR("Failed to create cube image view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    v->vk_info_cube.sampler = VK_NULL_HANDLE;
    v->vk_info_cube.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    TRACE("Created cube image view 0x%s.\n", wine_dbgstr_longlong(v->vk_info_cube.imageView));

    view_desc.subresourceRange.layerCount = 1;
    view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &v->vk_info_2d_array.imageView))) < 0)
    {
        ERR("Failed to create 2D array image view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    v->vk_info_2d_array.sampler = VK_NULL_HANDLE;
    v->vk_info_2d_array.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    TRACE("Created 2D array image view 0x%s.\n", wine_dbgstr_longlong(v->vk_info_2d_array.imageView));

    view_desc.image = r->image_2dms.vk_image;
    view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &v->vk_info_2dms_array.imageView))) < 0)
    {
        ERR("Failed to create 2D MSAA array image view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    v->vk_info_2dms_array.sampler = VK_NULL_HANDLE;
    v->vk_info_2dms_array.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    TRACE("Created 2D MSAA array image view 0x%s.\n", wine_dbgstr_longlong(v->vk_info_2dms_array.imageView));

    view_desc.image = r->image_2d.vk_image;
    view_desc.subresourceRange.layerCount = 6;
    view_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &v->vk_info_cube_array.imageView))) < 0)
    {
        ERR("Failed to create cube array image view, vr %s.\n", wined3d_debug_vkresult(vr));
        goto fail;
    }
    v->vk_info_cube_array.sampler = VK_NULL_HANDLE;
    v->vk_info_cube_array.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    TRACE("Created cube array image view 0x%s.\n", wine_dbgstr_longlong(v->vk_info_cube_array.imageView));

    return true;

fail:
    if (v->vk_info_cube_array.imageView)
        VK_CALL(vkDestroyImageView(device_vk->vk_device, v->vk_info_cube_array.imageView, NULL));
    if (v->vk_info_2d_array.imageView)
        VK_CALL(vkDestroyImageView(device_vk->vk_device, v->vk_info_2d_array.imageView, NULL));
    if (v->vk_info_cube.imageView)
        VK_CALL(vkDestroyImageView(device_vk->vk_device, v->vk_info_cube.imageView, NULL));
    if (v->vk_info_3d.imageView)
        VK_CALL(vkDestroyImageView(device_vk->vk_device, v->vk_info_3d.imageView, NULL));
    if (v->vk_info_2dms.imageView)
        VK_CALL(vkDestroyImageView(device_vk->vk_device, v->vk_info_2dms.imageView, NULL));
    if (v->vk_info_2d.imageView)
        VK_CALL(vkDestroyImageView(device_vk->vk_device, v->vk_info_2d.imageView, NULL));
    if (v->vk_info_1d_array.imageView)
        VK_CALL(vkDestroyImageView(device_vk->vk_device, v->vk_info_1d_array.imageView, NULL));
    if (v->vk_info_1d.imageView)
        VK_CALL(vkDestroyImageView(device_vk->vk_device, v->vk_info_1d.imageView, NULL));
    if (v->vk_view_buffer_float)
        VK_CALL(vkDestroyBufferView(device_vk->vk_device, v->vk_view_buffer_float, NULL));
    VK_CALL(vkDestroyBufferView(device_vk->vk_device, v->vk_view_buffer_uint, NULL));
    return false;
}

void wined3d_device_vk_destroy_null_views(struct wined3d_device_vk *device_vk, struct wined3d_context_vk *context_vk)
{
    struct wined3d_null_views_vk *v = &device_vk->null_views_vk;
    uint64_t id = context_vk->current_command_buffer.id;

    wined3d_context_vk_destroy_vk_image_view(context_vk, v->vk_info_cube_array.imageView, id);
    wined3d_context_vk_destroy_vk_image_view(context_vk, v->vk_info_2dms_array.imageView, id);
    wined3d_context_vk_destroy_vk_image_view(context_vk, v->vk_info_2d_array.imageView, id);
    wined3d_context_vk_destroy_vk_image_view(context_vk, v->vk_info_cube.imageView, id);
    wined3d_context_vk_destroy_vk_image_view(context_vk, v->vk_info_3d.imageView, id);
    wined3d_context_vk_destroy_vk_image_view(context_vk, v->vk_info_2dms.imageView, id);
    wined3d_context_vk_destroy_vk_image_view(context_vk, v->vk_info_2d.imageView, id);
    wined3d_context_vk_destroy_vk_image_view(context_vk, v->vk_info_1d_array.imageView, id);
    wined3d_context_vk_destroy_vk_image_view(context_vk, v->vk_info_1d.imageView, id);

    wined3d_context_vk_destroy_vk_buffer_view(context_vk, v->vk_view_buffer_float, id);
    wined3d_context_vk_destroy_vk_buffer_view(context_vk, v->vk_view_buffer_uint, id);
}

HRESULT CDECL wined3d_device_acquire_focus_window(struct wined3d_device *device, HWND window)
{
    unsigned int screensaver_active;

    TRACE("device %p, window %p.\n", device, window);

    if (!wined3d_register_window(NULL, window, device, 0))
    {
        ERR("Failed to register window %p.\n", window);
        return E_FAIL;
    }

    InterlockedExchangePointer((void **)&device->focus_window, window);
    SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SystemParametersInfoW(SPI_GETSCREENSAVEACTIVE, 0, &screensaver_active, 0);
    if ((device->restore_screensaver = !!screensaver_active))
        SystemParametersInfoW(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);

    return WINED3D_OK;
}

void CDECL wined3d_device_release_focus_window(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    if (device->focus_window) wined3d_unregister_window(device->focus_window);
    InterlockedExchangePointer((void **)&device->focus_window, NULL);
    if (device->restore_screensaver)
    {
        SystemParametersInfoW(SPI_SETSCREENSAVEACTIVE, TRUE, NULL, 0);
        device->restore_screensaver = FALSE;
    }
}

static void device_init_swapchain_state(struct wined3d_device *device, struct wined3d_swapchain *swapchain)
{
    struct wined3d_rendertarget_view *views[WINED3D_MAX_RENDER_TARGETS] = {0};
    BOOL ds_enable = swapchain->state.desc.enable_auto_depth_stencil;
    struct wined3d_device_context *context = &device->cs->c;

    if (device->back_buffer_view)
        views[0] = device->back_buffer_view;
    wined3d_device_context_set_rendertarget_views(context, 0,
            device->adapter->d3d_info.limits.max_rt_count, views, !!device->back_buffer_view);

    wined3d_device_context_set_depth_stencil_view(context, ds_enable ? device->auto_depth_stencil_view : NULL);
}

static struct wined3d_allocator_chunk *wined3d_allocator_gl_create_chunk(struct wined3d_allocator *allocator,
        struct wined3d_context *context, unsigned int memory_type, size_t chunk_size)
{
    struct wined3d_allocator_chunk_gl *chunk_gl;
    struct wined3d_context_gl *context_gl;

    TRACE("allocator %p, context %p, memory_type %u, chunk_size %Iu.\n", allocator, context, memory_type, chunk_size);

    if (!context)
        return NULL;
    context_gl = wined3d_context_gl(context);

    if (!(chunk_gl = malloc(sizeof(*chunk_gl))))
        return NULL;

    if (!wined3d_allocator_chunk_init(&chunk_gl->c, allocator))
    {
        free(chunk_gl);
        return NULL;
    }

    chunk_gl->memory_type = memory_type;
    if (!(chunk_gl->gl_buffer = wined3d_context_gl_allocate_vram_chunk_buffer(context_gl, memory_type, chunk_size)))
    {
        wined3d_allocator_chunk_cleanup(&chunk_gl->c);
        free(chunk_gl);
        return NULL;
    }
    list_add_head(&allocator->pools[memory_type].chunks, &chunk_gl->c.entry);

    return &chunk_gl->c;
}

static void wined3d_allocator_gl_destroy_chunk(struct wined3d_allocator_chunk *chunk)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl_from_allocator(chunk->allocator);
    struct wined3d_allocator_chunk_gl *chunk_gl = wined3d_allocator_chunk_gl(chunk);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;

    TRACE("chunk %p.\n", chunk);

    context_gl = wined3d_context_gl(context_acquire(&device_gl->d, NULL, 0));
    gl_info = context_gl->gl_info;

    wined3d_context_gl_bind_bo(context_gl, GL_PIXEL_UNPACK_BUFFER, chunk_gl->gl_buffer);
    if (chunk_gl->c.map_ptr)
        GL_EXTCALL(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
    GL_EXTCALL(glDeleteBuffers(1, &chunk_gl->gl_buffer));
    TRACE("Freed buffer %u.\n", chunk_gl->gl_buffer);
    wined3d_allocator_chunk_cleanup(&chunk_gl->c);
    free(chunk_gl);

    context_release(&context_gl->c);
}

static const struct wined3d_allocator_ops wined3d_allocator_gl_ops =
{
    .allocator_create_chunk = wined3d_allocator_gl_create_chunk,
    .allocator_destroy_chunk = wined3d_allocator_gl_destroy_chunk,
};

static const struct
{
    GLbitfield flags;
}
gl_memory_types[] =
{
    {0},
    {GL_MAP_READ_BIT},
    {GL_MAP_WRITE_BIT},
    {GL_MAP_READ_BIT | GL_MAP_WRITE_BIT},

    {GL_CLIENT_STORAGE_BIT},
    {GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT},
    {GL_CLIENT_STORAGE_BIT | GL_MAP_WRITE_BIT},
    {GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT},
};

static unsigned int wined3d_device_gl_find_memory_type(GLbitfield flags)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(gl_memory_types); ++i)
    {
        if (gl_memory_types[i].flags == flags)
            return i;
    }

    assert(0);
    return 0;
}

GLbitfield wined3d_device_gl_get_memory_type_flags(unsigned int memory_type_idx)
{
    return gl_memory_types[memory_type_idx].flags;
}

static struct wined3d_allocator_block *wined3d_device_gl_allocate_memory(struct wined3d_device_gl *device_gl,
        struct wined3d_context_gl *context_gl, unsigned int memory_type, GLsizeiptr size, GLuint *id)
{
    struct wined3d_allocator *allocator = &device_gl->allocator;
    struct wined3d_allocator_block *block;

    wined3d_device_gl_allocator_lock(device_gl);

    if (size > WINED3D_ALLOCATOR_CHUNK_SIZE / 2)
    {
        if (context_gl)
            *id = wined3d_context_gl_allocate_vram_chunk_buffer(context_gl, memory_type, size);
        wined3d_device_gl_allocator_unlock(device_gl);
        return NULL;
    }

    if (!(block = wined3d_allocator_allocate(allocator, context_gl ? &context_gl->c : NULL, memory_type, size)))
    {
        wined3d_device_gl_allocator_unlock(device_gl);
        *id = 0;
        return NULL;
    }

    *id = wined3d_allocator_chunk_gl(block->chunk)->gl_buffer;

    wined3d_device_gl_allocator_unlock(device_gl);
    TRACE("Allocated offset %#Ix from chunk %p.\n", block->offset, block->chunk);
    return block;
}

static bool use_buffer_chunk_suballocation(struct wined3d_device_gl *device_gl,
        const struct wined3d_gl_info *gl_info, GLenum binding)
{
    switch (binding)
    {
        case GL_ARRAY_BUFFER:
        case GL_ATOMIC_COUNTER_BUFFER:
        case GL_DRAW_INDIRECT_BUFFER:
        case GL_PIXEL_UNPACK_BUFFER:
        case GL_UNIFORM_BUFFER:
            return true;

        case GL_ELEMENT_ARRAY_BUFFER:
            /* There is no way to specify an element array buffer offset for
             * indirect draws in OpenGL. */
            return device_gl->d.wined3d->flags & WINED3D_NO_DRAW_INDIRECT
                    || !gl_info->supported[ARB_DRAW_INDIRECT];

        case GL_TEXTURE_BUFFER:
            return gl_info->supported[ARB_TEXTURE_BUFFER_RANGE];

        default:
            return false;
    }
}

bool wined3d_device_gl_create_bo(struct wined3d_device_gl *device_gl, struct wined3d_context_gl *context_gl,
        GLsizeiptr size, GLenum binding, GLenum usage, bool coherent, GLbitfield flags, struct wined3d_bo_gl *bo)
{
    const struct wined3d_gl_info *gl_info = &wined3d_adapter_gl(device_gl->d.adapter)->gl_info;
    unsigned int memory_type_idx = wined3d_device_gl_find_memory_type(flags);
    struct wined3d_allocator_block *memory = NULL;
    GLsizeiptr buffer_offset = 0;
    GLuint id = 0;

    TRACE("device_gl %p, context_gl %p, size %Iu, binding %#x, usage %#x, coherent %#x, flags %#x, bo %p.\n",
            device_gl, context_gl, size, binding, usage, coherent, flags, bo);

    if (gl_info->supported[ARB_BUFFER_STORAGE])
    {
        /* Only suballocate dynamic buffers.
         *
         * We only need suballocation so that we can allocate GL buffers from
         * the client thread and thereby accelerate DISCARD maps.
         *
         * For other buffer types, suballocating means that a whole-buffer
         * upload won't be replacing the whole buffer anymore. If the driver
         * isn't smart enough to track individual buffer ranges then it'll
         * force synchronizing that BO with the GPU. Even using ARB_sync
         * ourselves won't help here, because glBufferSubData() is still
         * implicitly synchronized. */
        if (flags & GL_CLIENT_STORAGE_BIT)
        {
            if (use_buffer_chunk_suballocation(device_gl, gl_info, binding))
            {
                if ((memory = wined3d_device_gl_allocate_memory(device_gl, context_gl, memory_type_idx, size, &id)))
                    buffer_offset = memory->offset;
                else if (!context_gl)
                    WARN_(d3d_perf)("Failed to suballocate buffer from the client thread.\n");
            }
            else if (context_gl)
            {
                WARN_(d3d_perf)("Not allocating chunk memory for binding type %#x.\n", binding);
                id = wined3d_context_gl_allocate_vram_chunk_buffer(context_gl, memory_type_idx, size);
            }
        }
        else
        {
            id = wined3d_context_gl_allocate_vram_chunk_buffer(context_gl, memory_type_idx, size);
        }

        if (!id)
        {
            if (context_gl)
                WARN("Failed to allocate buffer.\n");
            return false;
        }
    }
    else
    {
        if (!context_gl)
            return false;

        GL_EXTCALL(glGenBuffers(1, &id));
        if (!id)
        {
            checkGLcall("buffer object creation");
            return false;
        }
        TRACE("Created buffer object %u.\n", id);
        wined3d_context_gl_bind_bo(context_gl, binding, id);

        if (!coherent && gl_info->supported[APPLE_FLUSH_BUFFER_RANGE])
        {
            GL_EXTCALL(glBufferParameteriAPPLE(binding, GL_BUFFER_FLUSHING_UNMAP_APPLE, GL_FALSE));
            GL_EXTCALL(glBufferParameteriAPPLE(binding, GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_FALSE));
        }

        GL_EXTCALL(glBufferData(binding, size, NULL, usage));

        wined3d_context_gl_bind_bo(context_gl, binding, 0);
        checkGLcall("buffer object creation");
    }

    bo->id = id;
    bo->memory = memory;
    bo->size = size;
    bo->binding = binding;
    bo->usage = usage;
    bo->flags = flags;
    bo->b.coherent = coherent;
    list_init(&bo->b.users);
    bo->command_fence_id = 0;
    bo->b.buffer_offset = buffer_offset;
    bo->b.memory_offset = bo->b.buffer_offset;
    bo->b.map_ptr = NULL;
    bo->b.client_map_count = 0;
    bo->b.refcount = 1;

    return true;
}

void wined3d_device_gl_delete_opengl_contexts_cs(void *object)
{
    struct wined3d_device_gl *device_gl = object;
    struct wined3d_context_gl *context_gl;
    struct wined3d_context *context;
    struct wined3d_device *device;
    struct wined3d_shader *shader;

    TRACE("device %p.\n", device_gl);

    device = &device_gl->d;

    LIST_FOR_EACH_ENTRY(shader, &device->shaders, struct wined3d_shader, shader_list_entry)
    {
        device->shader_backend->shader_destroy(shader);
    }

    context = context_acquire(device, NULL, 0);
    context_gl = wined3d_context_gl(context);
    device->blitter->ops->blitter_destroy(device->blitter, context);
    device->shader_backend->shader_free_private(device, context);
    wined3d_device_gl_destroy_dummy_textures(device_gl, context_gl);

    if (context_gl->c.d3d_info->fences)
    {
        wined3d_context_gl_submit_command_fence(context_gl);
        wined3d_context_gl_wait_command_fence(context_gl,
                wined3d_device_gl(context_gl->c.device)->current_fence_id - 1);
    }
    wined3d_allocator_cleanup(&device_gl->allocator);

    context_release(context);

    while (device->context_count)
        wined3d_context_gl_destroy(wined3d_context_gl(device->contexts[0]));

    if (device_gl->backup_dc)
    {
        TRACE("Destroying backup wined3d window %p, dc %p.\n", device_gl->backup_wnd, device_gl->backup_dc);

        wined3d_release_dc(device_gl->backup_wnd, device_gl->backup_dc);
        DestroyWindow(device_gl->backup_wnd);
    }
}

void wined3d_device_gl_create_primary_opengl_context_cs(void *object)
{
    struct wined3d_device_gl *device_gl = object;
    struct wined3d_context_gl *context_gl;
    struct wined3d_swapchain *swapchain;
    struct wined3d_context *context;
    struct wined3d_texture *target;
    struct wined3d_device *device;
    HRESULT hr;

    TRACE("device %p.\n", device_gl);

    device = &device_gl->d;
    swapchain = device->swapchains[0];
    target = swapchain->back_buffers ? swapchain->back_buffers[0] : swapchain->front_buffer;
    if (!(context = context_acquire(device, target, 0)))
    {
        WARN("Failed to acquire context.\n");
        return;
    }

    context_gl = wined3d_context_gl(context);

    if (!wined3d_allocator_init(&device_gl->allocator, ARRAY_SIZE(gl_memory_types), &wined3d_allocator_gl_ops))
    {
        WARN("Failed to initialise allocator.\n");
        context_release(context);
        wined3d_context_gl_destroy(wined3d_context_gl(device->contexts[0]));
        return;
    }

    if (FAILED(hr = device->shader_backend->shader_alloc_private(device,
            device->adapter->vertex_pipe, device->adapter->fragment_pipe)))
    {
        ERR("Failed to allocate shader private data, hr %#lx.\n", hr);
        wined3d_allocator_cleanup(&device_gl->allocator);
        context_release(context);
        wined3d_context_gl_destroy(wined3d_context_gl(device->contexts[0]));
        return;
    }

    if (!(device->blitter = wined3d_cpu_blitter_create()))
    {
        ERR("Failed to create CPU blitter.\n");
        device->shader_backend->shader_free_private(device, NULL);
        wined3d_allocator_cleanup(&device_gl->allocator);
        context_release(context);
        wined3d_context_gl_destroy(wined3d_context_gl(device->contexts[0]));
        return;
    }

    wined3d_ffp_blitter_create(&device->blitter, context_gl->gl_info);
    wined3d_glsl_blitter_create(&device->blitter, device);
    wined3d_fbo_blitter_create(&device->blitter, context_gl->gl_info);
    wined3d_raw_blitter_create(&device->blitter, context_gl->gl_info);

    wined3d_device_gl_create_dummy_textures(device_gl, context_gl);
    wined3d_device_create_default_samplers(device, context);
    context_release(context);
}

HRESULT wined3d_device_set_implicit_swapchain(struct wined3d_device *device, struct wined3d_swapchain *swapchain)
{
    static const struct wined3d_color black = {0.0f, 0.0f, 0.0f, 0.0f};
    const struct wined3d_swapchain_desc *swapchain_desc;
    struct wined3d_fb_state *fb = &device->cs->c.state->fb;
    DWORD clear_flags = 0;
    unsigned int i;
    HRESULT hr;

    TRACE("device %p, swapchain %p.\n", device, swapchain);

    if (device->d3d_initialized)
        return WINED3DERR_INVALIDCALL;

    device->swapchain_count = 1;
    if (!(device->swapchains = calloc(device->swapchain_count, sizeof(*device->swapchains))))
    {
        ERR("Failed to allocate swapchain array.\n");
        hr = E_OUTOFMEMORY;
        goto err_out;
    }
    device->swapchains[0] = swapchain;

    for (i = 0; i < ARRAY_SIZE(fb->render_targets); ++i)
    {
        if (fb->render_targets[i])
            wined3d_rtv_bind_count_dec(fb->render_targets[i]);
    }
    memset(fb->render_targets, 0, sizeof(fb->render_targets));

    if (FAILED(hr = device->adapter->adapter_ops->adapter_init_3d(device)))
        goto err_out;
    device->d3d_initialized = TRUE;

    swapchain_desc = &swapchain->state.desc;
    if (swapchain_desc->backbuffer_count && swapchain_desc->backbuffer_bind_flags & WINED3D_BIND_RENDER_TARGET)
    {
        struct wined3d_resource *back_buffer = &swapchain->back_buffers[0]->resource;
        struct wined3d_view_desc view_desc;

        view_desc.format_id = back_buffer->format->id;
        view_desc.flags = 0;
        view_desc.u.texture.level_idx = 0;
        view_desc.u.texture.level_count = 1;
        view_desc.u.texture.layer_idx = 0;
        view_desc.u.texture.layer_count = 1;
        if (FAILED(hr = wined3d_rendertarget_view_create(&view_desc, back_buffer,
                NULL, &wined3d_null_parent_ops, &device->back_buffer_view)))
        {
            ERR("Failed to create rendertarget view, hr %#lx.\n", hr);
            device->adapter->adapter_ops->adapter_uninit_3d(device);
            device->d3d_initialized = FALSE;
            goto err_out;
        }
    }

    device_init_swapchain_state(device, swapchain);

    TRACE("All defaults now set up.\n");

    /* Clear the screen. */
    if (device->back_buffer_view)
        clear_flags |= WINED3DCLEAR_TARGET;
    if (swapchain_desc->enable_auto_depth_stencil)
        clear_flags |= WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL;
    if (clear_flags)
        wined3d_device_clear(device, 0, NULL, clear_flags, &black, 1.0f, 0);

    if (wined3d_settings.logo)
        device_load_logo(device, wined3d_settings.logo);

    return WINED3D_OK;

err_out:
    free(device->swapchains);
    device->swapchains = NULL;
    device->swapchain_count = 0;

    return hr;
}

static void device_free_sampler(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_sampler *sampler = WINE_RB_ENTRY_VALUE(entry, struct wined3d_sampler, entry);

    wined3d_sampler_decref(sampler);
}

static void device_free_rasterizer_state(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_rasterizer_state *state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_rasterizer_state, entry);

    wined3d_rasterizer_state_decref(state);
}

static void device_free_blend_state(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_blend_state *blend_state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_blend_state, entry);

    wined3d_blend_state_decref(blend_state);
}

static void device_free_depth_stencil_state(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_depth_stencil_state *state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_depth_stencil_state, entry);

    wined3d_depth_stencil_state_decref(state);
}

static void device_free_ffp_vertex_shader(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_ffp_vs *vs = WINE_RB_ENTRY_VALUE(entry, struct wined3d_ffp_vs, entry);

    wined3d_shader_decref(vs->shader);
    free(vs);
}

static void device_free_ffp_pixel_shader(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_ffp_ps *ps = WINE_RB_ENTRY_VALUE(entry, struct wined3d_ffp_ps, entry);

    wined3d_shader_decref(ps->shader);
    free(ps);
}

void wined3d_device_uninit_3d(struct wined3d_device *device)
{
    struct wined3d_state *state = device->cs->c.state;
    struct wined3d_resource *resource, *cursor;
    struct wined3d_rendertarget_view *view;
    struct wined3d_texture *texture;
    struct wined3d_buffer *buffer;
    unsigned int i;

    TRACE("device %p.\n", device);

    if (!device->d3d_initialized)
    {
        ERR("Called while 3D support was not initialised.\n");
        return;
    }

    wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);

    device->swapchain_count = 0;

    if ((texture = device->logo_texture))
    {
        device->logo_texture = NULL;
        wined3d_texture_decref(texture);
    }

    if ((texture = device->cursor_texture))
    {
        device->cursor_texture = NULL;
        wined3d_texture_decref(texture);
    }

    for (i = 0; i < ARRAY_SIZE(device->push_constants); ++i)
    {
        if ((buffer = device->push_constants[i]))
            wined3d_buffer_decref(buffer);
    }
    memset(device->push_constants, 0, sizeof(device->push_constants));

    wined3d_device_context_emit_reset_state(&device->cs->c, true);
    state_cleanup(state);

    wine_rb_destroy(&device->samplers, device_free_sampler, NULL);
    wine_rb_destroy(&device->rasterizer_states, device_free_rasterizer_state, NULL);
    wine_rb_destroy(&device->blend_states, device_free_blend_state, NULL);
    wine_rb_destroy(&device->depth_stencil_states, device_free_depth_stencil_state, NULL);
    wine_rb_destroy(&device->ffp_vertex_shaders, device_free_ffp_vertex_shader, NULL);
    wine_rb_destroy(&device->ffp_pixel_shaders, device_free_ffp_pixel_shader, NULL);

    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Unloading resource %p.\n", resource);
        wined3d_cs_emit_unload_resource(device->cs, resource);
    }

    device->adapter->adapter_ops->adapter_uninit_3d(device);
    device->d3d_initialized = FALSE;

    if ((view = device->auto_depth_stencil_view))
    {
        device->auto_depth_stencil_view = NULL;
        if (wined3d_rendertarget_view_decref(view))
            ERR("Something's still holding the auto depth/stencil view (%p).\n", view);
    }

    if ((view = device->back_buffer_view))
    {
        device->back_buffer_view = NULL;
        wined3d_rendertarget_view_decref(view);
    }

    free(device->swapchains);
    device->swapchains = NULL;

    wined3d_state_reset(state, &device->adapter->d3d_info);
}

/* Enables thread safety in the wined3d device and its resources. Called by DirectDraw
 * from SetCooperativeLevel if DDSCL_MULTITHREADED is specified, and by d3d8/9 from
 * CreateDevice if D3DCREATE_MULTITHREADED is passed.
 *
 * There is no way to deactivate thread safety once it is enabled.
 */
void CDECL wined3d_device_set_multithreaded(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    /* For now just store the flag (needed in case of ddraw). */
    device->create_parms.flags |= WINED3DCREATE_MULTITHREADED;
}

UINT CDECL wined3d_device_get_available_texture_mem(const struct wined3d_device *device)
{
    const struct wined3d_driver_info *driver_info;

    TRACE("device %p.\n", device);

    driver_info = &device->adapter->driver_info;

    TRACE("Emulating 0x%s bytes. 0x%s used, returning 0x%s left.\n",
            wine_dbgstr_longlong(driver_info->vram_bytes),
            wine_dbgstr_longlong(device->adapter->vram_bytes_used),
            wine_dbgstr_longlong(driver_info->vram_bytes - device->adapter->vram_bytes_used));

    return min(UINT_MAX, driver_info->vram_bytes) - device->adapter->vram_bytes_used;
}

struct wined3d_buffer * CDECL wined3d_device_context_get_stream_output(struct wined3d_device_context *context,
        unsigned int idx, unsigned int *offset)
{
    TRACE("context %p, idx %u, offset %p.\n", context, idx, offset);

    if (idx >= WINED3D_MAX_STREAM_OUTPUT_BUFFERS)
    {
        WARN("Invalid stream output %u.\n", idx);
        return NULL;
    }

    if (offset)
        *offset = context->state->stream_output[idx].offset;
    return context->state->stream_output[idx].buffer;
}

HRESULT CDECL wined3d_device_context_get_stream_source(const struct wined3d_device_context *context,
        unsigned int stream_idx, struct wined3d_buffer **buffer, unsigned int *offset, unsigned int *stride)
{
    const struct wined3d_stream_state *stream;

    TRACE("context %p, stream_idx %u, buffer %p, offset %p, stride %p.\n",
            context, stream_idx, buffer, offset, stride);

    if (stream_idx >= WINED3D_MAX_STREAMS)
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return WINED3DERR_INVALIDCALL;
    }

    stream = &context->state->streams[stream_idx];
    *buffer = stream->buffer;
    if (offset)
        *offset = stream->offset;
    *stride = stream->stride;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_clip_status(struct wined3d_device *device,
        const struct wined3d_clip_status *clip_status)
{
    FIXME("device %p, clip_status %p stub!\n", device, clip_status);

    if (!clip_status)
        return WINED3DERR_INVALIDCALL;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_clip_status(const struct wined3d_device *device,
        struct wined3d_clip_status *clip_status)
{
    FIXME("device %p, clip_status %p stub!\n", device, clip_status);

    if (!clip_status)
        return WINED3DERR_INVALIDCALL;

    return WINED3D_OK;
}

struct wined3d_buffer * CDECL wined3d_device_context_get_index_buffer(const struct wined3d_device_context *context,
        enum wined3d_format_id *format, unsigned int *offset)
{
    const struct wined3d_state *state = context->state;

    TRACE("context %p, format %p, offset %p.\n", context, format, offset);

    *format = state->index_format;
    if (offset)
        *offset = state->index_offset;
    return state->index_buffer;
}

void CDECL wined3d_device_context_get_viewports(const struct wined3d_device_context *context,
        unsigned int *viewport_count, struct wined3d_viewport *viewports)
{
    const struct wined3d_state *state = context->state;
    unsigned int count;

    TRACE("context %p, viewport_count %p, viewports %p.\n", context, viewport_count, viewports);

    count = viewport_count ? min(*viewport_count, state->viewport_count) : 1;
    if (count && viewports)
        memcpy(viewports, state->viewports, count * sizeof(*viewports));
    if (viewport_count)
        *viewport_count = state->viewport_count;
}

struct wined3d_blend_state * CDECL wined3d_device_context_get_blend_state(const struct wined3d_device_context *context,
        struct wined3d_color *blend_factor, unsigned int *sample_mask)
{
    const struct wined3d_state *state = context->state;

    TRACE("context %p, blend_factor %p, sample_mask %p.\n", context, blend_factor, sample_mask);

    *blend_factor = state->blend_factor;
    *sample_mask = state->sample_mask;
    return state->blend_state;
}

struct wined3d_depth_stencil_state * CDECL wined3d_device_context_get_depth_stencil_state(
        const struct wined3d_device_context *context, unsigned int *stencil_ref)
{
    const struct wined3d_state *state = context->state;

    TRACE("context %p, stencil_ref %p.\n", context, stencil_ref);

    *stencil_ref = state->stencil_ref;
    return state->depth_stencil_state;
}

struct wined3d_rasterizer_state * CDECL wined3d_device_context_get_rasterizer_state(
        struct wined3d_device_context *context)
{
    TRACE("context %p.\n", context);

    return context->state->rasterizer_state;
}

void CDECL wined3d_device_context_get_scissor_rects(const struct wined3d_device_context *context,
        unsigned int *rect_count, RECT *rects)
{
    const struct wined3d_state *state = context->state;
    unsigned int count;

    TRACE("context %p, rect_count %p, rects %p.\n", context, rect_count, rects);

    if (rects && (count = rect_count ? min(*rect_count, state->scissor_rect_count) : 1))
        memcpy(rects, state->scissor_rects, count * sizeof(*rects));
    if (rect_count)
        *rect_count = state->scissor_rect_count;
}

void CDECL wined3d_device_context_reset_state(struct wined3d_device_context *context)
{
    TRACE("context %p.\n", context);

    wined3d_device_context_lock(context);
    state_cleanup(context->state);
    wined3d_state_reset(context->state, &context->device->adapter->d3d_info);
    wined3d_device_context_emit_reset_state(context, true);
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_state(struct wined3d_device_context *context, struct wined3d_state *state)
{
    unsigned int i;

    TRACE("context %p, state %p.\n", context, state);

    wined3d_device_context_lock(context);
    context->state = state;
    wined3d_device_context_emit_set_feature_level(context, state->feature_level);

    wined3d_device_context_emit_set_rendertarget_views(context, 0,
            ARRAY_SIZE(state->fb.render_targets), state->fb.render_targets);

    wined3d_device_context_emit_set_depth_stencil_view(context, state->fb.depth_stencil);
    wined3d_device_context_emit_set_vertex_declaration(context, state->vertex_declaration);

    wined3d_device_context_emit_set_stream_outputs(context, state->stream_output);

    wined3d_device_context_emit_set_stream_sources(context, 0, WINED3D_MAX_STREAMS, state->streams);

    wined3d_device_context_emit_set_index_buffer(context, state->index_buffer,
            state->index_format, state->index_offset);

    wined3d_device_context_emit_set_predication(context, state->predicate, state->predicate_value);

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        wined3d_device_context_emit_set_shader(context, i, state->shader[i]);
        wined3d_device_context_emit_set_constant_buffers(context, i, 0, MAX_CONSTANT_BUFFERS, state->cb[i]);
        wined3d_device_context_emit_set_samplers(context, i, 0, MAX_SAMPLER_OBJECTS, state->sampler[i]);
        wined3d_device_context_emit_set_shader_resource_views(context, i, 0,
                MAX_SHADER_RESOURCE_VIEWS, state->shader_resource_view[i]);
    }

    for (i = 0; i < WINED3D_PIPELINE_COUNT; ++i)
        wined3d_device_context_emit_set_unordered_access_views(context, i, 0, MAX_UNORDERED_ACCESS_VIEWS,
                state->unordered_access_view[i], NULL);

    wined3d_device_context_emit_set_viewports(context, state->viewport_count, state->viewports);
    wined3d_device_context_emit_set_scissor_rects(context, state->scissor_rect_count, state->scissor_rects);

    wined3d_device_context_emit_set_blend_state(context, state->blend_state, &state->blend_factor, state->sample_mask);
    wined3d_device_context_emit_set_depth_stencil_state(context, state->depth_stencil_state, state->stencil_ref);
    wined3d_device_context_emit_set_rasterizer_state(context, state->rasterizer_state);
    wined3d_device_context_unlock(context);
}

struct wined3d_state * CDECL wined3d_device_get_state(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->cs->c.state;
}

struct wined3d_device_context * CDECL wined3d_device_get_immediate_context(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return &device->cs->c;
}

struct wined3d_vertex_declaration * CDECL wined3d_device_context_get_vertex_declaration(
        const struct wined3d_device_context *context)
{
    TRACE("context %p.\n", context);

    return context->state->vertex_declaration;
}

void CDECL wined3d_device_context_set_shader(struct wined3d_device_context *context,
        enum wined3d_shader_type type, struct wined3d_shader *shader)
{
    struct wined3d_state *state = context->state;
    struct wined3d_shader *prev;

    TRACE("context %p, type %#x, shader %p.\n", context, type, shader);

    wined3d_device_context_lock(context);
    prev = state->shader[type];
    if (shader == prev)
        goto out;

    if (shader)
        wined3d_shader_incref(shader);
    state->shader[type] = shader;
    wined3d_device_context_emit_set_shader(context, type, shader);
    if (prev)
        wined3d_shader_decref(prev);
out:
    wined3d_device_context_unlock(context);
}

struct wined3d_shader * CDECL wined3d_device_context_get_shader(const struct wined3d_device_context *context,
        enum wined3d_shader_type type)
{
    TRACE("context %p, type %#x.\n", context, type);

    return context->state->shader[type];
}

void CDECL wined3d_device_context_set_constant_buffers(struct wined3d_device_context *context,
        enum wined3d_shader_type type, unsigned int start_idx, unsigned int count,
        const struct wined3d_constant_buffer_state *buffers)
{
    struct wined3d_state *state = context->state;
    unsigned int i;

    TRACE("context %p, type %#x, start_idx %u, count %u, buffers %p.\n", context, type, start_idx, count, buffers);

    if (!wined3d_bound_range(start_idx, count, MAX_CONSTANT_BUFFERS))
    {
        WARN("Invalid constant buffer index %u, count %u.\n", start_idx, count);
        return;
    }

    wined3d_device_context_lock(context);
    if (!memcmp(buffers, &state->cb[type][start_idx], count * sizeof(*buffers)))
        goto out;

    wined3d_device_context_emit_set_constant_buffers(context, type, start_idx, count, buffers);
    for (i = 0; i < count; ++i)
    {
        struct wined3d_buffer *prev = state->cb[type][start_idx + i].buffer;
        struct wined3d_buffer *buffer = buffers[i].buffer;

        if (buffer)
            wined3d_buffer_incref(buffer);
        state->cb[type][start_idx + i] = buffers[i];
        if (prev)
            wined3d_buffer_decref(prev);
    }
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_blend_state(struct wined3d_device_context *context,
        struct wined3d_blend_state *blend_state, const struct wined3d_color *blend_factor, unsigned int sample_mask)
{
    struct wined3d_state *state = context->state;
    struct wined3d_blend_state *prev;

    TRACE("context %p, blend_state %p, blend_factor %p, sample_mask %#x.\n",
            context, blend_state, blend_factor, sample_mask);

    wined3d_device_context_lock(context);
    prev = state->blend_state;
    if (prev == blend_state && !memcmp(blend_factor, &state->blend_factor, sizeof(*blend_factor))
            && sample_mask == state->sample_mask)
        goto out;

    if (blend_state)
        wined3d_blend_state_incref(blend_state);
    state->blend_state = blend_state;
    state->blend_factor = *blend_factor;
    state->sample_mask = sample_mask;
    wined3d_device_context_emit_set_blend_state(context, blend_state, blend_factor, sample_mask);
    if (prev)
        wined3d_blend_state_decref(prev);
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_depth_stencil_state(struct wined3d_device_context *context,
        struct wined3d_depth_stencil_state *depth_stencil_state, unsigned int stencil_ref)
{
    struct wined3d_state *state = context->state;
    struct wined3d_depth_stencil_state *prev;

    TRACE("context %p, depth_stencil_state %p, stencil_ref %u.\n", context, depth_stencil_state, stencil_ref);

    wined3d_device_context_lock(context);
    prev = state->depth_stencil_state;
    if (prev == depth_stencil_state && state->stencil_ref == stencil_ref)
        goto out;

    if (depth_stencil_state)
        wined3d_depth_stencil_state_incref(depth_stencil_state);
    state->depth_stencil_state = depth_stencil_state;
    state->stencil_ref = stencil_ref;
    wined3d_device_context_emit_set_depth_stencil_state(context, depth_stencil_state, stencil_ref);
    if (prev)
        wined3d_depth_stencil_state_decref(prev);
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_rasterizer_state(struct wined3d_device_context *context,
        struct wined3d_rasterizer_state *rasterizer_state)
{
    struct wined3d_state *state = context->state;
    struct wined3d_rasterizer_state *prev;

    TRACE("context %p, rasterizer_state %p.\n", context, rasterizer_state);

    wined3d_device_context_lock(context);
    prev = state->rasterizer_state;
    if (prev == rasterizer_state)
        goto out;

    if (rasterizer_state)
        wined3d_rasterizer_state_incref(rasterizer_state);
    state->rasterizer_state = rasterizer_state;
    wined3d_device_context_emit_set_rasterizer_state(context, rasterizer_state);
    if (prev)
        wined3d_rasterizer_state_decref(prev);
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_viewports(struct wined3d_device_context *context, unsigned int viewport_count,
        const struct wined3d_viewport *viewports)
{
    struct wined3d_state *state = context->state;
    unsigned int i;

    TRACE("context %p, viewport_count %u, viewports %p.\n", context, viewport_count, viewports);

    for (i = 0; i < viewport_count; ++i)
    {
        TRACE("%u: x %.8e, y %.8e, w %.8e, h %.8e, min_z %.8e, max_z %.8e.\n",  i, viewports[i].x, viewports[i].y,
                viewports[i].width, viewports[i].height, viewports[i].min_z, viewports[i].max_z);
    }

    wined3d_device_context_lock(context);
    if (viewport_count)
        memcpy(state->viewports, viewports, viewport_count * sizeof(*viewports));
    else
        memset(state->viewports, 0, sizeof(state->viewports));
    state->viewport_count = viewport_count;

    wined3d_device_context_emit_set_viewports(context, viewport_count, viewports);
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_scissor_rects(struct wined3d_device_context *context, unsigned int rect_count,
        const RECT *rects)
{
    struct wined3d_state *state = context->state;
    unsigned int i;

    TRACE("context %p, rect_count %u, rects %p.\n", context, rect_count, rects);

    for (i = 0; i < rect_count; ++i)
    {
        TRACE("%u: %s\n", i, wine_dbgstr_rect(&rects[i]));
    }

    wined3d_device_context_lock(context);
    if (state->scissor_rect_count == rect_count
            && !memcmp(state->scissor_rects, rects, rect_count * sizeof(*rects)))
    {
        TRACE("App is setting the old scissor rectangles over, nothing to do.\n");
        goto out;
    }

    if (rect_count)
        memcpy(state->scissor_rects, rects, rect_count * sizeof(*rects));
    else
        memset(state->scissor_rects, 0, sizeof(state->scissor_rects));
    state->scissor_rect_count = rect_count;

    wined3d_device_context_emit_set_scissor_rects(context, rect_count, rects);
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_shader_resource_views(struct wined3d_device_context *context,
        enum wined3d_shader_type type, unsigned int start_idx, unsigned int count,
        struct wined3d_shader_resource_view *const *const views)
{
    struct wined3d_shader_resource_view *real_views[MAX_SHADER_RESOURCE_VIEWS];
    struct wined3d_state *state = context->state;
    const struct wined3d_rendertarget_view *dsv = state->fb.depth_stencil;
    unsigned int i;

    TRACE("context %p, type %#x, start_idx %u, count %u, views %p.\n", context, type, start_idx, count, views);

    if (!wined3d_bound_range(start_idx, count, MAX_SHADER_RESOURCE_VIEWS))
    {
        WARN("Invalid view index %u, count %u.\n", start_idx, count);
        return;
    }

    wined3d_device_context_lock(context);
    if (!memcmp(views, &state->shader_resource_view[type][start_idx], count * sizeof(*views)))
        goto out;

    memcpy(real_views, views, count * sizeof(*views));

    for (i = 0; i < count; ++i)
    {
        struct wined3d_shader_resource_view *view = real_views[i];

        if (view && (wined3d_is_srv_rtv_bound(state, view)
                || (dsv && dsv->resource == view->resource && wined3d_dsv_srv_conflict(dsv, view->format))))
        {
            WARN("Application is trying to bind resource which is attached as render target.\n");
            real_views[i] = NULL;
        }
    }

    wined3d_device_context_emit_set_shader_resource_views(context, type, start_idx, count, real_views);
    for (i = 0; i < count; ++i)
    {
        struct wined3d_shader_resource_view *prev = state->shader_resource_view[type][start_idx + i];
        struct wined3d_shader_resource_view *view = real_views[i];

        if (view)
        {
            wined3d_shader_resource_view_incref(view);
            wined3d_srv_bind_count_inc(view);
        }

        state->shader_resource_view[type][start_idx + i] = view;
        if (prev)
        {
            wined3d_srv_bind_count_dec(prev);
            wined3d_shader_resource_view_decref(prev);
        }
    }
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_samplers(struct wined3d_device_context *context, enum wined3d_shader_type type,
        unsigned int start_idx, unsigned int count, struct wined3d_sampler *const *samplers)
{
    struct wined3d_state *state = context->state;
    unsigned int i;

    TRACE("context %p, type %#x, start_idx %u, count %u, samplers %p.\n", context, type, start_idx, count, samplers);

    if (!wined3d_bound_range(start_idx, count, MAX_SAMPLER_OBJECTS))
    {
        WARN("Invalid sampler index %u, count %u.\n", start_idx, count);
        return;
    }

    wined3d_device_context_lock(context);
    if (!memcmp(samplers, &state->sampler[type][start_idx], count * sizeof(*samplers)))
        goto out;

    wined3d_device_context_emit_set_samplers(context, type, start_idx, count, samplers);
    for (i = 0; i < count; ++i)
    {
        struct wined3d_sampler *prev = state->sampler[type][start_idx + i];
        struct wined3d_sampler *sampler = samplers[i];

        if (sampler)
            wined3d_sampler_incref(sampler);
        state->sampler[type][start_idx + i] = sampler;
        if (prev)
            wined3d_sampler_decref(prev);
    }
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_unordered_access_views(struct wined3d_device_context *context,
        enum wined3d_pipeline pipeline, unsigned int start_idx, unsigned int count,
        struct wined3d_unordered_access_view *const *uavs, const unsigned int *initial_counts)
{
    struct wined3d_state *state = context->state;
    unsigned int i;

    TRACE("context %p, pipeline %#x, start_idx %u, count %u, uavs %p, initial_counts %p.\n",
            context, pipeline, start_idx, count, uavs, initial_counts);

    if (!wined3d_bound_range(start_idx, count, MAX_UNORDERED_ACCESS_VIEWS))
    {
        WARN("Invalid UAV index %u, count %u.\n", start_idx, count);
        return;
    }

    wined3d_device_context_lock(context);
    if (!memcmp(uavs, &state->unordered_access_view[pipeline][start_idx], count * sizeof(*uavs)) && !initial_counts)
        goto out;

    wined3d_device_context_emit_set_unordered_access_views(context, pipeline, start_idx, count, uavs, initial_counts);
    for (i = 0; i < count; ++i)
    {
        struct wined3d_unordered_access_view *prev = state->unordered_access_view[pipeline][start_idx + i];
        struct wined3d_unordered_access_view *uav = uavs[i];

        if (uav)
            wined3d_unordered_access_view_incref(uav);
        state->unordered_access_view[pipeline][start_idx + i] = uav;
        if (prev)
            wined3d_unordered_access_view_decref(prev);
    }
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_render_targets_and_unordered_access_views(struct wined3d_device_context *context,
        unsigned int rtv_count, struct wined3d_rendertarget_view *const *render_target_views,
        struct wined3d_rendertarget_view *depth_stencil_view, UINT uav_count,
        struct wined3d_unordered_access_view *const *unordered_access_views, const unsigned int *initial_counts)
{
    wined3d_device_context_lock(context);
    if (rtv_count != ~0u)
    {
        if (depth_stencil_view && !(depth_stencil_view->resource->bind_flags & WINED3D_BIND_DEPTH_STENCIL))
        {
            WARN("View resource %p has incompatible %s bind flags.\n",
                    depth_stencil_view->resource, wined3d_debug_bind_flags(depth_stencil_view->resource->bind_flags));
            goto out;
        }

        if (FAILED(wined3d_device_context_set_rendertarget_views(context, 0, rtv_count,
                render_target_views, FALSE)))
            goto out;

        wined3d_device_context_set_depth_stencil_view(context, depth_stencil_view);
    }

    if (uav_count != ~0u)
    {
        wined3d_device_context_set_unordered_access_views(context, WINED3D_PIPELINE_GRAPHICS, 0, uav_count,
                unordered_access_views, initial_counts);
    }
out:
    wined3d_device_context_unlock(context);
}

static void wined3d_device_context_unbind_srv_for_rtv(struct wined3d_device_context *context,
        const struct wined3d_rendertarget_view *view, BOOL dsv)
{
    const struct wined3d_state *state = context->state;
    const struct wined3d_resource *resource;

    if (!view)
        return;
    resource = view->resource;

    if (resource->srv_bind_count_device)
    {
        const struct wined3d_shader_resource_view *srv;
        unsigned int i, j;

        for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
        {
            for (j = 0; j < MAX_SHADER_RESOURCE_VIEWS; ++j)
            {
                if ((srv = state->shader_resource_view[i][j]) && srv->resource == resource
                        && ((!dsv && wined3d_is_srv_rtv_bound(state, srv))
                        || (dsv && wined3d_dsv_srv_conflict(view, srv->format))))
                {
                    static struct wined3d_shader_resource_view *const null_srv;

                    WARN("Application sets bound resource as render target.\n");
                    wined3d_device_context_set_shader_resource_views(context, i, j, 1, &null_srv);
                }
            }
        }
    }
}

HRESULT CDECL wined3d_device_context_set_rendertarget_views(struct wined3d_device_context *context,
        unsigned int start_idx, unsigned int count, struct wined3d_rendertarget_view *const *views, BOOL set_viewport)
{
    struct wined3d_state *state = context->state;
    unsigned int i, max_rt_count;

    TRACE("context %p, start_idx %u, count %u, views %p, set_viewport %#x.\n",
            context, start_idx, count, views, set_viewport);

    max_rt_count = context->device->adapter->d3d_info.limits.max_rt_count;
    if (start_idx >= max_rt_count)
    {
        WARN("Only %u render targets are supported.\n", max_rt_count);
        return WINED3DERR_INVALIDCALL;
    }
    count = min(count, max_rt_count - start_idx);

    for (i = 0; i < count; ++i)
    {
        if (views[i] && !(views[i]->resource->bind_flags & WINED3D_BIND_RENDER_TARGET))
        {
            WARN("View resource %p doesn't have render target bind flags.\n", views[i]->resource);
            return WINED3DERR_INVALIDCALL;
        }
    }

    wined3d_device_context_lock(context);
    /* Set the viewport and scissor rectangles, if requested. Tests show that
     * stateblock recording is ignored, the change goes directly into the
     * primary stateblock. */
    if (!start_idx && set_viewport)
    {
        state->viewports[0].x = 0;
        state->viewports[0].y = 0;
        state->viewports[0].width = views[0]->width;
        state->viewports[0].height = views[0]->height;
        state->viewports[0].min_z = 0.0f;
        state->viewports[0].max_z = 1.0f;
        state->viewport_count = 1;
        wined3d_device_context_emit_set_viewports(context, 1, state->viewports);

        SetRect(&state->scissor_rects[0], 0, 0, views[0]->width, views[0]->height);
        state->scissor_rect_count = 1;
        wined3d_device_context_emit_set_scissor_rects(context, 1, state->scissor_rects);
    }

    if (!memcmp(views, &state->fb.render_targets[start_idx], count * sizeof(*views)))
        goto out;

    wined3d_device_context_emit_set_rendertarget_views(context, start_idx, count, views);
    for (i = 0; i < count; ++i)
    {
        struct wined3d_rendertarget_view *prev = state->fb.render_targets[start_idx + i];
        struct wined3d_rendertarget_view *view = views[i];

        if (view)
        {
            wined3d_rendertarget_view_incref(view);
            wined3d_rtv_bind_count_inc(view);
        }
        state->fb.render_targets[start_idx + i] = view;
        /* Release after the assignment, to prevent device_resource_released()
         * from seeing the resource as still in use. */
        if (prev)
        {
            wined3d_rtv_bind_count_dec(prev);
            wined3d_rendertarget_view_decref(prev);
        }

        wined3d_device_context_unbind_srv_for_rtv(context, view, FALSE);
    }
out:
    wined3d_device_context_unlock(context);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_context_set_depth_stencil_view(struct wined3d_device_context *context,
        struct wined3d_rendertarget_view *view)
{
    struct wined3d_fb_state *fb = &context->state->fb;
    struct wined3d_rendertarget_view *prev;

    TRACE("context %p, view %p.\n", context, view);

    if (view && !(view->resource->bind_flags & WINED3D_BIND_DEPTH_STENCIL))
    {
        WARN("View resource %p has incompatible %s bind flags.\n",
                view->resource, wined3d_debug_bind_flags(view->resource->bind_flags));
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_device_context_lock(context);
    prev = fb->depth_stencil;
    if (prev == view)
    {
        TRACE("Trying to do a NOP SetRenderTarget operation.\n");
        goto out;
    }

    if ((fb->depth_stencil = view))
        wined3d_rendertarget_view_incref(view);
    wined3d_device_context_emit_set_depth_stencil_view(context, view);
    if (prev)
        wined3d_rendertarget_view_decref(prev);
    wined3d_device_context_unbind_srv_for_rtv(context, view, TRUE);
out:
    wined3d_device_context_unlock(context);
    return WINED3D_OK;
}

void CDECL wined3d_device_context_set_predication(struct wined3d_device_context *context,
        struct wined3d_query *predicate, BOOL value)
{
    struct wined3d_state *state = context->state;
    struct wined3d_query *prev;

    TRACE("context %p, predicate %p, value %#x.\n", context, predicate, value);

    wined3d_device_context_lock(context);
    prev = state->predicate;
    if (predicate)
    {
        FIXME("Predicated rendering not implemented.\n");
        wined3d_query_incref(predicate);
    }
    state->predicate = predicate;
    state->predicate_value = value;
    wined3d_device_context_emit_set_predication(context, predicate, value);
    if (prev)
        wined3d_query_decref(prev);
    wined3d_device_context_unlock(context);
}

HRESULT CDECL wined3d_device_context_set_stream_sources(struct wined3d_device_context *context,
        unsigned int start_idx, unsigned int count, const struct wined3d_stream_state *streams)
{
    struct wined3d_state *state = context->state;
    unsigned int i;

    TRACE("context %p, start_idx %u, count %u, streams %p.\n", context, start_idx, count, streams);

    if (start_idx >= WINED3D_MAX_STREAMS)
    {
        WARN("Start index %u is out of range.\n", start_idx);
        return WINED3DERR_INVALIDCALL;
    }

    count = min(count, WINED3D_MAX_STREAMS - start_idx);

    for (i = 0; i < count; ++i)
    {
        if (streams[i].offset & 0x3)
        {
            WARN("Offset %u is not 4 byte aligned.\n", streams[i].offset);
            return WINED3DERR_INVALIDCALL;
        }
    }

    wined3d_device_context_lock(context);
    if (!memcmp(streams, &state->streams[start_idx], count * sizeof(*streams)))
        goto out;

    wined3d_device_context_emit_set_stream_sources(context, start_idx, count, streams);
    for (i = 0; i < count; ++i)
    {
        struct wined3d_buffer *prev = state->streams[start_idx + i].buffer;
        struct wined3d_buffer *buffer = streams[i].buffer;

        state->streams[start_idx + i] = streams[i];

        if (buffer)
            wined3d_buffer_incref(buffer);
        if (prev)
            wined3d_buffer_decref(prev);
    }
out:
    wined3d_device_context_unlock(context);
    return WINED3D_OK;
}

void CDECL wined3d_device_context_set_index_buffer(struct wined3d_device_context *context,
        struct wined3d_buffer *buffer, enum wined3d_format_id format_id, unsigned int offset)
{
    struct wined3d_state *state = context->state;
    enum wined3d_format_id prev_format;
    struct wined3d_buffer *prev_buffer;
    unsigned int prev_offset;

    TRACE("context %p, buffer %p, format %s, offset %u.\n",
            context, buffer, debug_d3dformat(format_id), offset);

    wined3d_device_context_lock(context);
    prev_buffer = state->index_buffer;
    prev_format = state->index_format;
    prev_offset = state->index_offset;

    if (prev_buffer == buffer && prev_format == format_id && prev_offset == offset)
        goto out;

    if (buffer)
        wined3d_buffer_incref(buffer);
    state->index_buffer = buffer;
    state->index_format = format_id;
    state->index_offset = offset;
    wined3d_device_context_emit_set_index_buffer(context, buffer, format_id, offset);
    if (prev_buffer)
        wined3d_buffer_decref(prev_buffer);
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_vertex_declaration(struct wined3d_device_context *context,
        struct wined3d_vertex_declaration *declaration)
{
    struct wined3d_state *state = context->state;
    struct wined3d_vertex_declaration *prev;

    TRACE("context %p, declaration %p.\n", context, declaration);

    wined3d_device_context_lock(context);
    prev = state->vertex_declaration;
    if (declaration == prev)
        goto out;

    if (declaration)
        wined3d_vertex_declaration_incref(declaration);
    state->vertex_declaration = declaration;
    wined3d_device_context_emit_set_vertex_declaration(context, declaration);
    if (prev)
        wined3d_vertex_declaration_decref(prev);
out:
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_set_stream_outputs(struct wined3d_device_context *context,
        const struct wined3d_stream_output outputs[WINED3D_MAX_STREAM_OUTPUT_BUFFERS])
{
    struct wined3d_state *state = context->state;
    unsigned int i;

    TRACE("context %p, outputs %p.\n", context, outputs);

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_set_stream_outputs(context, outputs);
    for (i = 0; i < WINED3D_MAX_STREAM_OUTPUT_BUFFERS; ++i)
    {
        struct wined3d_buffer *prev_buffer = state->stream_output[i].buffer;
        struct wined3d_buffer *buffer = outputs[i].buffer;

        if (buffer)
            wined3d_buffer_incref(buffer);
        state->stream_output[i] = outputs[i];
        if (prev_buffer)
            wined3d_buffer_decref(prev_buffer);
    }
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_draw(struct wined3d_device_context *context, unsigned int start_vertex,
        unsigned int vertex_count, unsigned int start_instance, unsigned int instance_count)
{
    struct wined3d_state *state = context->state;

    TRACE("context %p, start_vertex %u, vertex_count %u, start_instance %u, instance_count %u.\n",
            context, start_vertex, vertex_count, start_instance, instance_count);

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_draw(context, state->primitive_type, state->patch_vertex_count,
            0, start_vertex, vertex_count, start_instance, instance_count, false);
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_draw_indexed(struct wined3d_device_context *context, int base_vertex_index,
        unsigned int start_index, unsigned int index_count, unsigned int start_instance, unsigned int instance_count)
{
    struct wined3d_state *state = context->state;

    TRACE("context %p, base_vertex_index %d, start_index %u, index_count %u, start_instance %u, instance_count %u.\n",
            context, base_vertex_index, start_index, index_count, start_instance, instance_count);

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_draw(context, state->primitive_type, state->patch_vertex_count,
            base_vertex_index, start_index, index_count, start_instance, instance_count, true);
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_get_constant_buffer(const struct wined3d_device_context *context,
        enum wined3d_shader_type shader_type, unsigned int idx, struct wined3d_constant_buffer_state *state)
{
    TRACE("context %p, shader_type %#x, idx %u.\n", context, shader_type, idx);

    if (idx >= MAX_CONSTANT_BUFFERS)
    {
        WARN("Invalid constant buffer index %u.\n", idx);
        return;
    }

    *state = context->state->cb[shader_type][idx];
}

struct wined3d_shader_resource_view * CDECL wined3d_device_context_get_shader_resource_view(
        const struct wined3d_device_context *context, enum wined3d_shader_type shader_type, unsigned int idx)
{
    if (idx >= MAX_SHADER_RESOURCE_VIEWS)
    {
        WARN("Invalid view index %u.\n", idx);
        return NULL;
    }

    return context->state->shader_resource_view[shader_type][idx];
}

struct wined3d_sampler * CDECL wined3d_device_context_get_sampler(const struct wined3d_device_context *context,
        enum wined3d_shader_type shader_type, unsigned int idx)
{
    TRACE("context %p, shader_type %#x, idx %u.\n", context, shader_type, idx);

    if (idx >= MAX_SAMPLER_OBJECTS)
    {
        WARN("Invalid sampler index %u.\n", idx);
        return NULL;
    }

    return context->state->sampler[shader_type][idx];
}

struct wined3d_unordered_access_view * CDECL wined3d_device_context_get_unordered_access_view(
        const struct wined3d_device_context *context, enum wined3d_pipeline pipeline, unsigned int idx)
{
    TRACE("context %p, pipeline %#x, idx %u.\n", context, pipeline, idx);

    if (idx >= MAX_UNORDERED_ACCESS_VIEWS)
    {
        WARN("Invalid UAV index %u.\n", idx);
        return NULL;
    }

    return context->state->unordered_access_view[pipeline][idx];
}

void CDECL wined3d_device_set_max_frame_latency(struct wined3d_device *device, unsigned int latency)
{
    unsigned int i;

    if (!latency)
        latency = 3;

    device->max_frame_latency = latency;
    for (i = 0; i < device->swapchain_count; ++i)
        swapchain_set_max_frame_latency(device->swapchains[i], device);
}

unsigned int CDECL wined3d_device_get_max_frame_latency(const struct wined3d_device *device)
{
    return device->max_frame_latency;
}

static unsigned int wined3d_get_flexible_vertex_size(uint32_t fvf)
{
    unsigned int texcoord_count = (fvf & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;
    unsigned int i, size = 0;

    if (fvf & WINED3DFVF_NORMAL) size += 3 * sizeof(float);
    if (fvf & WINED3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (fvf & WINED3DFVF_SPECULAR) size += sizeof(DWORD);
    if (fvf & WINED3DFVF_PSIZE) size += sizeof(DWORD);
    switch (fvf & WINED3DFVF_POSITION_MASK)
    {
        case WINED3DFVF_XYZ:    size += 3 * sizeof(float); break;
        case WINED3DFVF_XYZRHW: size += 4 * sizeof(float); break;
        case WINED3DFVF_XYZB1:  size += 4 * sizeof(float); break;
        case WINED3DFVF_XYZB2:  size += 5 * sizeof(float); break;
        case WINED3DFVF_XYZB3:  size += 6 * sizeof(float); break;
        case WINED3DFVF_XYZB4:  size += 7 * sizeof(float); break;
        case WINED3DFVF_XYZB5:  size += 8 * sizeof(float); break;
        case WINED3DFVF_XYZW:   size += 4 * sizeof(float); break;
        default: FIXME("Unexpected position mask %#x.\n", fvf & WINED3DFVF_POSITION_MASK);
    }
    for (i = 0; i < texcoord_count; ++i)
    {
        size += GET_TEXCOORD_SIZE_FROM_FVF(fvf, i) * sizeof(float);
    }

    return size;
}

static void wined3d_format_get_colour(const struct wined3d_format *format,
        const void *data, struct wined3d_color *colour)
{
    float *output = &colour->r;
    const uint32_t *u32_data;
    const uint16_t *u16_data;
    const float *f32_data;
    unsigned int i;

    static const struct wined3d_color default_colour = {0.0f, 0.0f, 0.0f, 1.0f};
    static unsigned int warned;

    switch (format->id)
    {
        case WINED3DFMT_B8G8R8A8_UNORM:
            u32_data = data;
            wined3d_color_from_d3dcolor(colour, *u32_data);
            break;

        case WINED3DFMT_R8G8B8A8_UNORM:
            u32_data = data;
            colour->r = (*u32_data & 0xffu) / 255.0f;
            colour->g = ((*u32_data >> 8) & 0xffu) / 255.0f;
            colour->b = ((*u32_data >> 16) & 0xffu) / 255.0f;
            colour->a = ((*u32_data >> 24) & 0xffu) / 255.0f;
            break;

        case WINED3DFMT_R16G16_UNORM:
        case WINED3DFMT_R16G16B16A16_UNORM:
            u16_data = data;
            *colour = default_colour;
            for (i = 0; i < format->component_count; ++i)
                output[i] = u16_data[i] / 65535.0f;
            break;

        case WINED3DFMT_R32_FLOAT:
        case WINED3DFMT_R32G32_FLOAT:
        case WINED3DFMT_R32G32B32_FLOAT:
        case WINED3DFMT_R32G32B32A32_FLOAT:
            f32_data = data;
            *colour = default_colour;
            for (i = 0; i < format->component_count; ++i)
                output[i] = f32_data[i];
            break;

        default:
            *colour = default_colour;
            if (!warned++)
                FIXME("Unhandled colour format conversion, format %s.\n", debug_d3dformat(format->id));
            break;
    }
}

static void wined3d_colour_from_mcs(struct wined3d_color *colour, enum wined3d_material_color_source mcs,
        const struct wined3d_color *material_colour, unsigned int index,
        const struct wined3d_stream_info *stream_info)
{
    const struct wined3d_stream_info_element *element = NULL;

    switch (mcs)
    {
        case WINED3D_MCS_MATERIAL:
            *colour = *material_colour;
            return;

        case WINED3D_MCS_COLOR1:
            if (!(stream_info->use_map & (1u << WINED3D_FFP_DIFFUSE)))
            {
                colour->r = colour->g = colour->b = colour->a = 1.0f;
                return;
            }
            element = &stream_info->elements[WINED3D_FFP_DIFFUSE];
            break;

        case WINED3D_MCS_COLOR2:
            if (!(stream_info->use_map & (1u << WINED3D_FFP_SPECULAR)))
            {
                colour->r = colour->g = colour->b = colour->a = 0.0f;
                return;
            }
            element = &stream_info->elements[WINED3D_FFP_SPECULAR];
            break;

        default:
            colour->r = colour->g = colour->b = colour->a = 0.0f;
            ERR("Invalid material colour source %#x.\n", mcs);
            return;
    }

    wined3d_format_get_colour(element->format, &element->data.addr[index * element->stride], colour);
}

static float wined3d_clamp(float value, float min_value, float max_value)
{
    return value < min_value ? min_value : value > max_value ? max_value : value;
}

static float wined3d_vec3_dot(const struct wined3d_vec3 *v0, const struct wined3d_vec3 *v1)
{
    return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

static void wined3d_vec3_subtract(struct wined3d_vec3 *v0, const struct wined3d_vec3 *v1)
{
    v0->x -= v1->x;
    v0->y -= v1->y;
    v0->z -= v1->z;
}

static void wined3d_vec3_scale(struct wined3d_vec3 *v, float s)
{
    v->x *= s;
    v->y *= s;
    v->z *= s;
}

static void wined3d_vec3_normalise(struct wined3d_vec3 *v)
{
    float rnorm = 1.0f / sqrtf(wined3d_vec3_dot(v, v));

    if (isfinite(rnorm))
        wined3d_vec3_scale(v, rnorm);
}

static void wined3d_vec3_transform(struct wined3d_vec3 *dst,
        const struct wined3d_vec3 *v, const struct wined3d_matrix *m)
{
    struct wined3d_vec3 tmp;

    tmp.x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31;
    tmp.y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32;
    tmp.z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33;

    *dst = tmp;
}

static void wined3d_color_clamp(struct wined3d_color *dst, const struct wined3d_color *src,
        float min_value, float max_value)
{
    dst->r = wined3d_clamp(src->r, min_value, max_value);
    dst->g = wined3d_clamp(src->g, min_value, max_value);
    dst->b = wined3d_clamp(src->b, min_value, max_value);
    dst->a = wined3d_clamp(src->a, min_value, max_value);
}

static void wined3d_color_rgb_mul_add(struct wined3d_color *dst, const struct wined3d_color *src, float c)
{
    dst->r += src->r * c;
    dst->g += src->g * c;
    dst->b += src->b * c;
}

static void init_transformed_lights(struct lights_settings *ls,
        const struct wined3d_stateblock_state *state, BOOL legacy_lighting, BOOL compute_lighting)
{
    const struct wined3d_light_info *lights[WINED3D_MAX_SOFTWARE_ACTIVE_LIGHTS];
    const struct wined3d_light_info *light_info;
    struct wined3d_light_info *light_iter;
    struct light_transformed *light;
    struct wined3d_vec4 vec4;
    unsigned int light_count;
    unsigned int i, index;

    memset(ls, 0, sizeof(*ls));

    ls->lighting = !!compute_lighting;
    ls->fog_mode = state->rs[WINED3D_RS_FOGVERTEXMODE];
    ls->fog_coord_mode = state->rs[WINED3D_RS_RANGEFOGENABLE]
            ? WINED3D_FFP_VS_FOG_RANGE : WINED3D_FFP_VS_FOG_DEPTH;
    ls->fog_start = int_to_float(state->rs[WINED3D_RS_FOGSTART]);
    ls->fog_end = int_to_float(state->rs[WINED3D_RS_FOGEND]);
    ls->fog_density = int_to_float(state->rs[WINED3D_RS_FOGDENSITY]);

    if (ls->fog_mode == WINED3D_FOG_NONE && !compute_lighting)
        return;

    multiply_matrix(&ls->modelview_matrix, &state->transforms[WINED3D_TS_VIEW],
            &state->transforms[WINED3D_TS_WORLD_MATRIX(0)]);

    if (!compute_lighting)
        return;

    compute_normal_matrix(&ls->normal_matrix, legacy_lighting, &ls->modelview_matrix);

    wined3d_color_from_d3dcolor(&ls->ambient_light, state->rs[WINED3D_RS_AMBIENT]);
    ls->legacy_lighting = !!legacy_lighting;
    ls->normalise = !!state->rs[WINED3D_RS_NORMALIZENORMALS];
    ls->localviewer = !!state->rs[WINED3D_RS_LOCALVIEWER];

    index = 0;
    RB_FOR_EACH_ENTRY(light_iter, &state->light_state->lights_tree, struct wined3d_light_info, entry)
    {
        if (!light_iter->enabled)
            continue;

        switch (light_iter->OriginalParms.type)
        {
            case WINED3D_LIGHT_DIRECTIONAL:
                ++ls->directional_light_count;
                break;

            case WINED3D_LIGHT_POINT:
                ++ls->point_light_count;
                break;

            case WINED3D_LIGHT_SPOT:
                ++ls->spot_light_count;
                break;

            case WINED3D_LIGHT_PARALLELPOINT:
                ++ls->parallel_point_light_count;
                break;

            default:
                FIXME("Unhandled light type %#x.\n", light_iter->OriginalParms.type);
                continue;
        }
        lights[index++] = light_iter;
        if (index == WINED3D_MAX_SOFTWARE_ACTIVE_LIGHTS)
            break;
    }

    light_count = index;
    for (i = 0, index = 0; i < light_count; ++i)
    {
        light_info = lights[i];
        if (light_info->OriginalParms.type != WINED3D_LIGHT_DIRECTIONAL)
            continue;

        light = &ls->lights[index];
        wined3d_vec4_transform(&vec4, &light_info->constants.direction, &state->transforms[WINED3D_TS_VIEW]);
        light->direction = *(struct wined3d_vec3 *)&vec4;
        wined3d_vec3_normalise(&light->direction);

        light->diffuse = light_info->OriginalParms.diffuse;
        light->ambient = light_info->OriginalParms.ambient;
        light->specular = light_info->OriginalParms.specular;
        ++index;
    }

    for (i = 0; i < light_count; ++i)
    {
        light_info = lights[i];
        if (light_info->OriginalParms.type != WINED3D_LIGHT_POINT)
            continue;

        light = &ls->lights[index];

        wined3d_vec4_transform(&light->position, &light_info->constants.position, &state->transforms[WINED3D_TS_VIEW]);
        light->range = light_info->OriginalParms.range;
        light->c_att = light_info->OriginalParms.attenuation0;
        light->l_att = light_info->OriginalParms.attenuation1;
        light->q_att = light_info->OriginalParms.attenuation2;

        light->diffuse = light_info->OriginalParms.diffuse;
        light->ambient = light_info->OriginalParms.ambient;
        light->specular = light_info->OriginalParms.specular;
        ++index;
    }

    for (i = 0; i < light_count; ++i)
    {
        light_info = lights[i];
        if (light_info->OriginalParms.type != WINED3D_LIGHT_SPOT)
            continue;

        light = &ls->lights[index];

        wined3d_vec4_transform(&light->position, &light_info->constants.position, &state->transforms[WINED3D_TS_VIEW]);
        wined3d_vec4_transform(&vec4, &light_info->constants.direction, &state->transforms[WINED3D_TS_VIEW]);
        light->direction = *(struct wined3d_vec3 *)&vec4;
        wined3d_vec3_normalise(&light->direction);
        light->range = light_info->OriginalParms.range;
        light->falloff = light_info->OriginalParms.falloff;
        light->c_att = light_info->OriginalParms.attenuation0;
        light->l_att = light_info->OriginalParms.attenuation1;
        light->q_att = light_info->OriginalParms.attenuation2;
        light->cos_htheta = cosf(light_info->OriginalParms.theta / 2.0f);
        light->cos_hphi = cosf(light_info->OriginalParms.phi / 2.0f);

        light->diffuse = light_info->OriginalParms.diffuse;
        light->ambient = light_info->OriginalParms.ambient;
        light->specular = light_info->OriginalParms.specular;
        ++index;
    }

    for (i = 0; i < light_count; ++i)
    {
        light_info = lights[i];
        if (light_info->OriginalParms.type != WINED3D_LIGHT_PARALLELPOINT)
            continue;

        light = &ls->lights[index];

        wined3d_vec4_transform(&vec4, &light_info->constants.position, &state->transforms[WINED3D_TS_VIEW]);
        *(struct wined3d_vec3 *)&light->position = *(struct wined3d_vec3 *)&vec4;
        wined3d_vec3_normalise((struct wined3d_vec3 *)&light->position);
        light->diffuse = light_info->OriginalParms.diffuse;
        light->ambient = light_info->OriginalParms.ambient;
        light->specular = light_info->OriginalParms.specular;
        ++index;
    }
}

static void update_light_diffuse_specular(struct wined3d_color *diffuse, struct wined3d_color *specular,
        const struct wined3d_vec3 *dir, float att, float material_shininess,
        const struct wined3d_vec3 *normal_transformed,
        const struct wined3d_vec3 *position_transformed_normalised,
        const struct light_transformed *light, const struct lights_settings *ls)
{
    struct wined3d_vec3 vec3;
    float t, c;

    c = wined3d_clamp(wined3d_vec3_dot(dir, normal_transformed), 0.0f, 1.0f);
    wined3d_color_rgb_mul_add(diffuse, &light->diffuse, c * att);

    vec3 = *dir;
    if (ls->localviewer)
        wined3d_vec3_subtract(&vec3, position_transformed_normalised);
    else
        vec3.z -= 1.0f;
    wined3d_vec3_normalise(&vec3);
    t = wined3d_vec3_dot(normal_transformed, &vec3);
    if (t > 0.0f && (!ls->legacy_lighting || material_shininess > 0.0f)
            && wined3d_vec3_dot(dir, normal_transformed) > 0.0f)
        wined3d_color_rgb_mul_add(specular, &light->specular, att * powf(t, material_shininess));
}

static void light_set_vertex_data(struct lights_settings *ls,
        const struct wined3d_vec4 *position)
{
    if (ls->fog_mode == WINED3D_FOG_NONE && !ls->lighting)
        return;

    wined3d_vec4_transform(&ls->position_transformed, position, &ls->modelview_matrix);
    wined3d_vec3_scale((struct wined3d_vec3 *)&ls->position_transformed, 1.0f / ls->position_transformed.w);
}

static void compute_light(struct wined3d_color *ambient, struct wined3d_color *diffuse,
        struct wined3d_color *specular, struct lights_settings *ls, const struct wined3d_vec3 *normal,
        float material_shininess)
{
    struct wined3d_vec3 position_transformed_normalised;
    struct wined3d_vec3 normal_transformed = {0.0f};
    const struct light_transformed *light;
    struct wined3d_vec3 dir, dst;
    unsigned int i, index;
    float att;

    position_transformed_normalised = *(const struct wined3d_vec3 *)&ls->position_transformed;
    wined3d_vec3_normalise(&position_transformed_normalised);

    if (normal)
    {
        wined3d_vec3_transform(&normal_transformed, normal, &ls->normal_matrix);
        if (ls->normalise)
            wined3d_vec3_normalise(&normal_transformed);
    }

    diffuse->r = diffuse->g = diffuse->b = diffuse->a = 0.0f;
    *specular = *diffuse;
    *ambient = ls->ambient_light;

    index = 0;
    for (i = 0; i < ls->directional_light_count; ++i, ++index)
    {
        light = &ls->lights[index];

        wined3d_color_rgb_mul_add(ambient, &light->ambient, 1.0f);
        if (normal)
            update_light_diffuse_specular(diffuse, specular, &light->direction, 1.0f, material_shininess,
                    &normal_transformed, &position_transformed_normalised, light, ls);
    }

    for (i = 0; i < ls->point_light_count; ++i, ++index)
    {
        light = &ls->lights[index];
        dir.x = light->position.x - ls->position_transformed.x;
        dir.y = light->position.y - ls->position_transformed.y;
        dir.z = light->position.z - ls->position_transformed.z;

        dst.z = wined3d_vec3_dot(&dir, &dir);
        dst.y = sqrtf(dst.z);
        dst.x = 1.0f;
        if (ls->legacy_lighting)
        {
            dst.y = (light->range - dst.y) / light->range;
            if (!(dst.y > 0.0f))
                continue;
            dst.z = dst.y * dst.y;
        }
        else
        {
            if (!(dst.y <= light->range))
                continue;
        }
        att = dst.x * light->c_att + dst.y * light->l_att + dst.z * light->q_att;
        if (!ls->legacy_lighting)
            att = 1.0f / att;

        wined3d_color_rgb_mul_add(ambient, &light->ambient, att);
        if (normal)
        {
            wined3d_vec3_normalise(&dir);
            update_light_diffuse_specular(diffuse, specular, &dir, att, material_shininess,
                    &normal_transformed, &position_transformed_normalised, light, ls);
        }
    }

    for (i = 0; i < ls->spot_light_count; ++i, ++index)
    {
        float t;

        light = &ls->lights[index];

        dir.x = light->position.x - ls->position_transformed.x;
        dir.y = light->position.y - ls->position_transformed.y;
        dir.z = light->position.z - ls->position_transformed.z;

        dst.z = wined3d_vec3_dot(&dir, &dir);
        dst.y = sqrtf(dst.z);
        dst.x = 1.0f;

        if (ls->legacy_lighting)
        {
            dst.y = (light->range - dst.y) / light->range;
            if (!(dst.y > 0.0f))
                continue;
            dst.z = dst.y * dst.y;
        }
        else
        {
            if (!(dst.y <= light->range))
                continue;
        }
        wined3d_vec3_normalise(&dir);
        t = -wined3d_vec3_dot(&dir, &light->direction);
        if (t > light->cos_htheta)
            att = 1.0f;
        else if (t <= light->cos_hphi)
            att = 0.0f;
        else
            att = powf((t - light->cos_hphi) / (light->cos_htheta - light->cos_hphi), light->falloff);

        t = dst.x * light->c_att + dst.y * light->l_att + dst.z * light->q_att;
        if (ls->legacy_lighting)
            att *= t;
        else
            att /= t;

        wined3d_color_rgb_mul_add(ambient, &light->ambient, att);

        if (normal)
            update_light_diffuse_specular(diffuse, specular, &dir, att, material_shininess,
                    &normal_transformed, &position_transformed_normalised, light, ls);
    }

    for (i = 0; i < ls->parallel_point_light_count; ++i, ++index)
    {
        light = &ls->lights[index];

        wined3d_color_rgb_mul_add(ambient, &light->ambient, 1.0f);
        if (normal)
            update_light_diffuse_specular(diffuse, specular, (const struct wined3d_vec3 *)&light->position,
                    1.0f, material_shininess, &normal_transformed, &position_transformed_normalised, light, ls);
    }
}

static float wined3d_calculate_fog_factor(float fog_coord, const struct lights_settings *ls)
{
    switch (ls->fog_mode)
    {
        case WINED3D_FOG_NONE:
            return fog_coord;
        case WINED3D_FOG_LINEAR:
            return (ls->fog_end - fog_coord) / (ls->fog_end - ls->fog_start);
        case WINED3D_FOG_EXP:
            return expf(-fog_coord * ls->fog_density);
        case WINED3D_FOG_EXP2:
            return expf(-fog_coord * fog_coord * ls->fog_density * ls->fog_density);
        default:
            ERR("Unhandled fog mode %#x.\n", ls->fog_mode);
            return 0.0f;
    }
}

static void update_fog_factor(float *fog_factor, struct lights_settings *ls)
{
    float fog_coord;

    if (ls->fog_mode == WINED3D_FOG_NONE)
        return;

    switch (ls->fog_coord_mode)
    {
        case WINED3D_FFP_VS_FOG_RANGE:
            fog_coord = sqrtf(wined3d_vec3_dot((const struct wined3d_vec3 *)&ls->position_transformed,
                    (const struct wined3d_vec3 *)&ls->position_transformed));
            break;

        case WINED3D_FFP_VS_FOG_DEPTH:
            fog_coord = fabsf(ls->position_transformed.z);
            break;

        default:
            ERR("Unhandled fog coordinate mode %#x.\n", ls->fog_coord_mode);
            return;
    }
    *fog_factor = wined3d_calculate_fog_factor(fog_coord, ls);
}

/* Context activation is done by the caller. */
#define copy_and_next(dest, src, size) memcpy(dest, src, size); dest += (size)
static HRESULT process_vertices_strided(const struct wined3d_device *device,
        const struct wined3d_stateblock_state *state, DWORD dwDestIndex, DWORD dwCount,
        const struct wined3d_stream_info *stream_info, struct wined3d_buffer *dest, uint32_t flags, uint32_t dst_fvf)
{
    enum wined3d_material_color_source diffuse_source, specular_source, ambient_source, emissive_source;
    const struct wined3d_state *device_state = device->cs->c.state;
    const struct wined3d_matrix *proj_mat, *view_mat, *world_mat;
    const struct wined3d_color *material_specular_state_colour;
    const struct wined3d_format *output_colour_format;
    static const struct wined3d_color black;
    struct wined3d_map_desc map_desc;
    struct wined3d_box box = {0};
    struct wined3d_viewport vp;
    unsigned int texture_count;
    struct lights_settings ls;
    struct wined3d_matrix mat;
    unsigned int vertex_size;
    BOOL do_clip, lighting;
    float min_z, max_z;
    unsigned int i;
    BYTE *dest_ptr;
    HRESULT hr;

    if (!(stream_info->use_map & (1u << WINED3D_FFP_POSITION)))
    {
        ERR("Source has no position mask.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (state->rs[WINED3D_RS_CLIPPING])
    {
        static BOOL warned = FALSE;
        /*
         * The clipping code is not quite correct. Some things need
         * to be checked against IDirect3DDevice3 (!), d3d8 and d3d9,
         * so disable clipping for now.
         * (The graphics in Half-Life are broken, and my processvertices
         *  test crashes with IDirect3DDevice3)
        do_clip = TRUE;
         */
        do_clip = FALSE;
        if (!warned)
        {
           warned = TRUE;
           FIXME("Clipping is broken and disabled for now\n");
        }
    }
    else
        do_clip = FALSE;

    vertex_size = wined3d_get_flexible_vertex_size(dst_fvf);
    box.left = dwDestIndex * vertex_size;
    box.right = box.left + dwCount * vertex_size;
    if (FAILED(hr = wined3d_resource_map(&dest->resource, 0, &map_desc, &box, WINED3D_MAP_WRITE)))
    {
        WARN("Failed to map buffer, hr %#lx.\n", hr);
        return hr;
    }
    dest_ptr = map_desc.data;

    view_mat = &state->transforms[WINED3D_TS_VIEW];
    proj_mat = &state->transforms[WINED3D_TS_PROJECTION];
    world_mat = &state->transforms[WINED3D_TS_WORLD];

    TRACE("View mat:\n");
    TRACE("%.8e %.8e %.8e %.8e\n", view_mat->_11, view_mat->_12, view_mat->_13, view_mat->_14);
    TRACE("%.8e %.8e %.8e %.8e\n", view_mat->_21, view_mat->_22, view_mat->_23, view_mat->_24);
    TRACE("%.8e %.8e %.8e %.8e\n", view_mat->_31, view_mat->_32, view_mat->_33, view_mat->_34);
    TRACE("%.8e %.8e %.8e %.8e\n", view_mat->_41, view_mat->_42, view_mat->_43, view_mat->_44);

    TRACE("Proj mat:\n");
    TRACE("%.8e %.8e %.8e %.8e\n", proj_mat->_11, proj_mat->_12, proj_mat->_13, proj_mat->_14);
    TRACE("%.8e %.8e %.8e %.8e\n", proj_mat->_21, proj_mat->_22, proj_mat->_23, proj_mat->_24);
    TRACE("%.8e %.8e %.8e %.8e\n", proj_mat->_31, proj_mat->_32, proj_mat->_33, proj_mat->_34);
    TRACE("%.8e %.8e %.8e %.8e\n", proj_mat->_41, proj_mat->_42, proj_mat->_43, proj_mat->_44);

    TRACE("World mat:\n");
    TRACE("%.8e %.8e %.8e %.8e\n", world_mat->_11, world_mat->_12, world_mat->_13, world_mat->_14);
    TRACE("%.8e %.8e %.8e %.8e\n", world_mat->_21, world_mat->_22, world_mat->_23, world_mat->_24);
    TRACE("%.8e %.8e %.8e %.8e\n", world_mat->_31, world_mat->_32, world_mat->_33, world_mat->_34);
    TRACE("%.8e %.8e %.8e %.8e\n", world_mat->_41, world_mat->_42, world_mat->_43, world_mat->_44);

    /* Get the viewport */
    wined3d_device_context_get_viewports(&device->cs->c, NULL, &vp);
    TRACE("viewport x %.8e, y %.8e, width %.8e, height %.8e, min_z %.8e, max_z %.8e.\n",
          vp.x, vp.y, vp.width, vp.height, vp.min_z, vp.max_z);

    multiply_matrix(&mat, view_mat, world_mat);
    multiply_matrix(&mat, proj_mat, &mat);

    texture_count = (dst_fvf & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;

    lighting = state->rs[WINED3D_RS_LIGHTING]
            && (dst_fvf & (WINED3DFVF_DIFFUSE | WINED3DFVF_SPECULAR));
    wined3d_get_material_colour_source(&diffuse_source, &emissive_source,
            &ambient_source, &specular_source, device_state);
    output_colour_format = wined3d_get_format(device->adapter, WINED3DFMT_B8G8R8A8_UNORM, 0);
    material_specular_state_colour = state->rs[WINED3D_RS_SPECULARENABLE]
            ? &state->material.specular : &black;
    init_transformed_lights(&ls, state, device->adapter->d3d_info.wined3d_creation_flags
            & WINED3D_LEGACY_FFP_LIGHTING, lighting);

    wined3d_viewport_get_z_range(&vp, &min_z, &max_z);

    for (i = 0; i < dwCount; ++i)
    {
        const struct wined3d_stream_info_element *position_element = &stream_info->elements[WINED3D_FFP_POSITION];
        const float *p = (const float *)&position_element->data.addr[i * position_element->stride];
        struct wined3d_color ambient, diffuse, specular;
        struct wined3d_vec4 position;
        unsigned int tex_index;

        position.x = p[0];
        position.y = p[1];
        position.z = p[2];
        position.w = 1.0f;

        light_set_vertex_data(&ls, &position);

        if ( ((dst_fvf & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZ ) ||
             ((dst_fvf & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW ) ) {
            /* The position first */
            float x, y, z, rhw;
            TRACE("In: ( %06.2f %06.2f %06.2f )\n", p[0], p[1], p[2]);

            /* Multiplication with world, view and projection matrix. */
            x   = (p[0] * mat._11) + (p[1] * mat._21) + (p[2] * mat._31) + mat._41;
            y   = (p[0] * mat._12) + (p[1] * mat._22) + (p[2] * mat._32) + mat._42;
            z   = (p[0] * mat._13) + (p[1] * mat._23) + (p[2] * mat._33) + mat._43;
            rhw = (p[0] * mat._14) + (p[1] * mat._24) + (p[2] * mat._34) + mat._44;

            TRACE("x=%f y=%f z=%f rhw=%f\n", x, y, z, rhw);

            /* WARNING: The following things are taken from d3d7 and were not yet checked
             * against d3d8 or d3d9!
             */

            /* Clipping conditions: From msdn
             *
             * A vertex is clipped if it does not match the following requirements
             * -rhw < x <= rhw
             * -rhw < y <= rhw
             *    0 < z <= rhw
             *    0 < rhw ( Not in d3d7, but tested in d3d7)
             *
             * If clipping is on is determined by the D3DVOP_CLIP flag in D3D7, and
             * by the D3DRS_CLIPPING in D3D9(according to the msdn, not checked)
             *
             */

            if (!do_clip || (-rhw - eps < x && -rhw - eps < y && -eps < z && x <= rhw + eps
                    && y <= rhw + eps && z <= rhw + eps && rhw > eps))
            {
                /* "Normal" viewport transformation (not clipped)
                 * 1) The values are divided by rhw
                 * 2) The y axis is negative, so multiply it with -1
                 * 3) Screen coordinates go from -(Width/2) to +(Width/2) and
                 *    -(Height/2) to +(Height/2). The z range is MinZ to MaxZ
                 * 4) Multiply x with Width/2 and add Width/2
                 * 5) The same for the height
                 * 6) Add the viewpoint X and Y to the 2D coordinates and
                 *    The minimum Z value to z
                 * 7) rhw = 1 / rhw Reciprocal of Homogeneous W....
                 *
                 * Well, basically it's simply a linear transformation into viewport
                 * coordinates
                 */

                x /= rhw;
                y /= rhw;
                z /= rhw;

                y *= -1;

                x *= vp.width / 2;
                y *= vp.height / 2;
                z *= max_z - min_z;

                x += vp.width / 2 + vp.x;
                y += vp.height / 2 + vp.y;
                z += min_z;

                rhw = 1 / rhw;
            } else {
                /* That vertex got clipped
                 * Contrary to OpenGL it is not dropped completely, it just
                 * undergoes a different calculation.
                 */
                TRACE("Vertex got clipped\n");
                x += rhw;
                y += rhw;

                x  /= 2;
                y  /= 2;

                /* Msdn mentions that Direct3D9 keeps a list of clipped vertices
                 * outside of the main vertex buffer memory. That needs some more
                 * investigation...
                 */
            }

            TRACE("Writing (%f %f %f) %f\n", x, y, z, rhw);


            ( (float *) dest_ptr)[0] = x;
            ( (float *) dest_ptr)[1] = y;
            ( (float *) dest_ptr)[2] = z;
            ( (float *) dest_ptr)[3] = rhw; /* SIC, see ddraw test! */

            dest_ptr += 3 * sizeof(float);

            if ((dst_fvf & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW)
                dest_ptr += sizeof(float);
        }

        if (dst_fvf & WINED3DFVF_PSIZE)
            dest_ptr += sizeof(DWORD);

        if (dst_fvf & WINED3DFVF_NORMAL)
        {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_NORMAL];
            const float *normal = (const float *)(element->data.addr + i * element->stride);
            /* AFAIK this should go into the lighting information */
            FIXME("Didn't expect the destination to have a normal\n");
            copy_and_next(dest_ptr, normal, 3 * sizeof(float));
        }

        if (lighting)
        {
            const struct wined3d_stream_info_element *element;
            struct wined3d_vec3 *normal;

            if (stream_info->use_map & (1u << WINED3D_FFP_NORMAL))
            {
                element = &stream_info->elements[WINED3D_FFP_NORMAL];
                normal = (struct wined3d_vec3 *)&element->data.addr[i * element->stride];
            }
            else
            {
                normal = NULL;
            }
            compute_light(&ambient, &diffuse, &specular, &ls, normal,
                    state->rs[WINED3D_RS_SPECULARENABLE] ? state->material.power : 0.0f);
        }

        if (dst_fvf & WINED3DFVF_DIFFUSE)
        {
            struct wined3d_color material_diffuse, material_ambient, material_emissive, diffuse_colour;

            wined3d_colour_from_mcs(&material_diffuse, diffuse_source,
                    &state->material.diffuse, i, stream_info);

            if (lighting)
            {
                wined3d_colour_from_mcs(&material_ambient, ambient_source,
                        &state->material.ambient, i, stream_info);
                wined3d_colour_from_mcs(&material_emissive, emissive_source,
                        &state->material.emissive, i, stream_info);

                diffuse_colour.r = ambient.r * material_ambient.r
                        + diffuse.r * material_diffuse.r + material_emissive.r;
                diffuse_colour.g = ambient.g * material_ambient.g
                        + diffuse.g * material_diffuse.g + material_emissive.g;
                diffuse_colour.b = ambient.b * material_ambient.b
                        + diffuse.b * material_diffuse.b + material_emissive.b;
                diffuse_colour.a = material_diffuse.a;
            }
            else
            {
                diffuse_colour = material_diffuse;
            }
            wined3d_color_clamp(&diffuse_colour, &diffuse_colour, 0.0f, 1.0f);
            wined3d_format_convert_from_float(output_colour_format, &diffuse_colour, dest_ptr);
            dest_ptr += sizeof(DWORD);
        }

        if (dst_fvf & WINED3DFVF_SPECULAR)
        {
            struct wined3d_color material_specular, specular_colour;

            wined3d_colour_from_mcs(&material_specular, specular_source,
                    material_specular_state_colour, i, stream_info);

            if (lighting)
            {
                specular_colour.r = specular.r * material_specular.r;
                specular_colour.g = specular.g * material_specular.g;
                specular_colour.b = specular.b * material_specular.b;
                specular_colour.a = ls.legacy_lighting ? 0.0f : material_specular.a;
            }
            else
            {
                specular_colour = material_specular;
            }
            update_fog_factor(&specular_colour.a, &ls);
            wined3d_color_clamp(&specular_colour, &specular_colour, 0.0f, 1.0f);
            wined3d_format_convert_from_float(output_colour_format, &specular_colour, dest_ptr);
            dest_ptr += sizeof(DWORD);
        }

        for (tex_index = 0; tex_index < texture_count; ++tex_index)
        {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_TEXCOORD0 + tex_index];
            const float *tex_coord = (const float *)(element->data.addr + i * element->stride);
            if (!(stream_info->use_map & (1u << (WINED3D_FFP_TEXCOORD0 + tex_index))))
            {
                ERR("No source texture, but destination requests one\n");
                dest_ptr += GET_TEXCOORD_SIZE_FROM_FVF(dst_fvf, tex_index) * sizeof(float);
            }
            else
            {
                copy_and_next(dest_ptr, tex_coord, GET_TEXCOORD_SIZE_FROM_FVF(dst_fvf, tex_index) * sizeof(float));
            }
        }
    }

    wined3d_resource_unmap(&dest->resource, 0);

    return WINED3D_OK;
}
#undef copy_and_next

HRESULT CDECL wined3d_device_process_vertices(struct wined3d_device *device, struct wined3d_stateblock *stateblock,
        UINT src_start_idx, UINT dst_idx, UINT vertex_count, struct wined3d_buffer *dst_buffer,
        const struct wined3d_vertex_declaration *declaration, uint32_t flags, uint32_t dst_fvf)
{
    const struct wined3d_stateblock_state *state = wined3d_stateblock_get_state(stateblock);
    struct wined3d_state *device_state = device->cs->c.state;
    struct wined3d_stream_info stream_info;
    struct wined3d_resource *resource;
    struct wined3d_box box = {0};
    struct wined3d_shader *vs;
    unsigned int i, j;
    uint32_t map;
    HRESULT hr;

    TRACE("device %p, src_start_idx %u, dst_idx %u, vertex_count %u, "
            "dst_buffer %p, declaration %p, flags %#x, dst_fvf %#x.\n",
            device, src_start_idx, dst_idx, vertex_count,
            dst_buffer, declaration, flags, dst_fvf);

    if (declaration)
        FIXME("Output vertex declaration not implemented yet.\n");

    wined3d_device_apply_stateblock(device, stateblock);

    vs = device_state->shader[WINED3D_SHADER_TYPE_VERTEX];
    device_state->shader[WINED3D_SHADER_TYPE_VERTEX] = NULL;
    wined3d_stream_info_from_declaration(&stream_info, device_state, &device->adapter->d3d_info);
    device_state->shader[WINED3D_SHADER_TYPE_VERTEX] = vs;

    /* We can't convert FROM a VBO, and vertex buffers used to source into
     * process_vertices() are unlikely to ever be used for drawing. Release
     * VBOs in those buffers and fix up the stream_info structure.
     *
     * Also apply the start index. */
    map = stream_info.use_map;
    while (map)
    {
        struct wined3d_stream_info_element *e;
        struct wined3d_map_desc map_desc;

        i = wined3d_bit_scan(&map);
        e = &stream_info.elements[i];
        resource = &state->streams[e->stream_idx].buffer->resource;
        box.left = src_start_idx * e->stride;
        box.right = box.left + vertex_count * e->stride;
        if (FAILED(wined3d_resource_map(resource, 0, &map_desc, &box, WINED3D_MAP_READ)))
        {
            ERR("Failed to map resource.\n");
            map = stream_info.use_map;
            while (map)
            {
                j = wined3d_bit_scan(&map);
                if (j >= i)
                    break;

                e = &stream_info.elements[j];
                resource = &state->streams[e->stream_idx].buffer->resource;
                if (FAILED(wined3d_resource_unmap(resource, 0)))
                    ERR("Failed to unmap resource.\n");
            }
            return WINED3DERR_INVALIDCALL;
        }
        e->data.buffer_object = 0;
        e->data.addr += (ULONG_PTR)map_desc.data;
    }

    hr = process_vertices_strided(device, state, dst_idx, vertex_count,
            &stream_info, dst_buffer, flags, dst_fvf);

    map = stream_info.use_map;
    while (map)
    {
        i = wined3d_bit_scan(&map);
        resource = &state->streams[stream_info.elements[i].stream_idx].buffer->resource;
        if (FAILED(wined3d_resource_unmap(resource, 0)))
            ERR("Failed to unmap resource.\n");
    }

    return hr;
}

HRESULT CDECL wined3d_device_get_device_caps(const struct wined3d_device *device, struct wined3d_caps *caps)
{
    TRACE("device %p, caps %p.\n", device, caps);

    return wined3d_get_device_caps(device->adapter, device->create_parms.device_type, caps);
}

HRESULT CDECL wined3d_device_get_display_mode(const struct wined3d_device *device, UINT swapchain_idx,
        struct wined3d_display_mode *mode, enum wined3d_display_rotation *rotation)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, mode %p, rotation %p.\n",
            device, swapchain_idx, mode, rotation);

    if (!(swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        return WINED3DERR_INVALIDCALL;

    return wined3d_swapchain_get_display_mode(swapchain, mode, rotation);
}

HRESULT CDECL wined3d_device_begin_scene(struct wined3d_device *device)
{
    /* At the moment we have no need for any functionality at the beginning
     * of a scene. */
    TRACE("device %p.\n", device);

    if (device->inScene)
    {
        WARN("Already in scene, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    device->inScene = TRUE;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_end_scene(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    if (!device->inScene)
    {
        WARN("Not in scene, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    device->inScene = FALSE;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_clear(struct wined3d_device *device, unsigned int rect_count,
        const RECT *rects, uint32_t flags, const struct wined3d_color *color, float depth, unsigned int stencil)
{
    struct wined3d_fb_state *fb = &device->cs->c.state->fb;

    TRACE("device %p, rect_count %u, rects %p, flags %#x, color %s, depth %.8e, stencil %u.\n",
            device, rect_count, rects, flags, debug_color(color), depth, stencil);

    if (!rect_count && rects)
    {
        WARN("Rects is %p, but rect_count is 0, ignoring clear\n", rects);
        return WINED3D_OK;
    }

    if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
    {
        struct wined3d_rendertarget_view *ds = fb->depth_stencil;
        if (!ds)
        {
            WARN("Clearing depth and/or stencil without a depth stencil buffer attached, returning WINED3DERR_INVALIDCALL\n");
            /* TODO: What about depth stencil buffers without stencil bits? */
            return WINED3DERR_INVALIDCALL;
        }
        else if (flags & WINED3DCLEAR_TARGET)
        {
            if (ds->width < fb->render_targets[0]->width
                    || ds->height < fb->render_targets[0]->height)
            {
                WARN("Silently ignoring depth and target clear with mismatching sizes\n");
                return WINED3D_OK;
            }
        }
    }

    wined3d_cs_emit_clear(device->cs, rect_count, rects, flags, color, depth, stencil);

    return WINED3D_OK;
}

struct wined3d_query * CDECL wined3d_device_context_get_predication(struct wined3d_device_context *context, BOOL *value)
{
    struct wined3d_state *state = context->state;

    TRACE("context %p, value %p.\n", context, value);

    if (value)
        *value = state->predicate_value;
    return state->predicate;
}

void CDECL wined3d_device_context_set_primitive_type(struct wined3d_device_context *context,
        enum wined3d_primitive_type primitive_type, unsigned int patch_vertex_count)
{
    struct wined3d_state *state = context->state;

    TRACE("context %p, primitive_type %s, patch_vertex_count %u.\n",
            context, debug_d3dprimitivetype(primitive_type), patch_vertex_count);

    wined3d_device_context_lock(context);
    state->primitive_type = primitive_type;
    state->patch_vertex_count = patch_vertex_count;
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_get_primitive_type(const struct wined3d_device_context *context,
        enum wined3d_primitive_type *primitive_type, unsigned int *patch_vertex_count)
{
    const struct wined3d_state *state = context->state;

    TRACE("context %p, primitive_type %p, patch_vertex_count %p.\n",
            context, primitive_type, patch_vertex_count);

    *primitive_type = state->primitive_type;
    if (patch_vertex_count)
        *patch_vertex_count = state->patch_vertex_count;

    TRACE("Returning %s.\n", debug_d3dprimitivetype(*primitive_type));
}

HRESULT CDECL wined3d_device_update_texture(struct wined3d_device *device,
        struct wined3d_texture *src_texture, struct wined3d_texture *dst_texture)
{
    unsigned int src_size, dst_size, src_skip_levels = 0;
    unsigned int src_level_count, dst_level_count;
    const struct wined3d_dirty_regions *regions;
    unsigned int layer_count, level_count, i, j;
    enum wined3d_resource_type type;
    BOOL entire_texture = TRUE;
    struct wined3d_box box;

    TRACE("device %p, src_texture %p, dst_texture %p.\n", device, src_texture, dst_texture);

    /* Verify that the source and destination textures are non-NULL. */
    if (!src_texture || !dst_texture)
    {
        WARN("Source and destination textures must be non-NULL, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (src_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU
            || src_texture->resource.usage & WINED3DUSAGE_SCRATCH)
    {
        WARN("Source resource is GPU accessible or a scratch resource.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if (dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_CPU)
    {
        WARN("Destination resource is CPU accessible.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Verify that the source and destination textures are the same type. */
    type = src_texture->resource.type;
    if (dst_texture->resource.type != type)
    {
        WARN("Source and destination have different types, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    layer_count = src_texture->layer_count;
    if (layer_count != dst_texture->layer_count)
    {
        WARN("Source and destination have different layer counts.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (src_texture->resource.format != dst_texture->resource.format)
    {
        WARN("Source and destination formats do not match.\n");
        return WINED3DERR_INVALIDCALL;
    }

    src_level_count = src_texture->level_count;
    dst_level_count = dst_texture->level_count;
    level_count = min(src_level_count, dst_level_count);

    src_size = max(src_texture->resource.width, src_texture->resource.height);
    src_size = max(src_size, src_texture->resource.depth);
    dst_size = max(dst_texture->resource.width, dst_texture->resource.height);
    dst_size = max(dst_size, dst_texture->resource.depth);
    while (src_size > dst_size)
    {
        src_size >>= 1;
        ++src_skip_levels;
    }

    if (wined3d_texture_get_level_width(src_texture, src_skip_levels) != dst_texture->resource.width
            || wined3d_texture_get_level_height(src_texture, src_skip_levels) != dst_texture->resource.height
            || wined3d_texture_get_level_depth(src_texture, src_skip_levels) != dst_texture->resource.depth)
    {
        WARN("Source and destination dimensions do not match.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if ((regions = src_texture->dirty_regions))
    {
        for (i = 0; i < layer_count && entire_texture; ++i)
        {
            if (regions[i].box_count >= WINED3D_MAX_DIRTY_REGION_COUNT)
                continue;

            entire_texture = FALSE;
            break;
        }
    }

    /* Update every surface level of the texture. */
    if (entire_texture)
    {
        for (i = 0; i < level_count; ++i)
        {
            wined3d_texture_get_level_box(dst_texture, i, &box);
            for (j = 0; j < layer_count; ++j)
            {
                wined3d_device_context_emit_blt_sub_resource(&device->cs->c,
                        &dst_texture->resource, j * dst_level_count + i, &box,
                        &src_texture->resource, j * src_level_count + i + src_skip_levels, &box,
                        0, NULL, WINED3D_TEXF_POINT);
            }
        }
    }
    else
    {
        unsigned int src_level, box_count, k;
        const struct wined3d_box *boxes;
        struct wined3d_box b;

        for (i = 0; i < layer_count; ++i)
        {
            boxes = regions[i].boxes;
            box_count = regions[i].box_count;
            if (regions[i].box_count >= WINED3D_MAX_DIRTY_REGION_COUNT)
            {
                boxes = &b;
                box_count = 1;
                wined3d_texture_get_level_box(dst_texture, i, &b);
            }

            for (j = 0; j < level_count; ++j)
            {
                src_level = j + src_skip_levels;

                /* TODO: We could pass an array of boxes here to avoid
                 * multiple context acquisitions for the same resource. */
                for (k = 0; k < box_count; ++k)
                {
                    box = boxes[k];
                    if (src_level)
                    {
                        box.left >>= src_level;
                        box.top >>= src_level;
                        box.right = min((box.right + (1u << src_level) - 1) >> src_level,
                                wined3d_texture_get_level_width(src_texture, src_level));
                        box.bottom = min((box.bottom + (1u << src_level) - 1) >> src_level,
                                wined3d_texture_get_level_height(src_texture, src_level));
                        box.front >>= src_level;
                        box.back = min((box.back + (1u << src_level) - 1) >> src_level,
                                wined3d_texture_get_level_depth(src_texture, src_level));
                    }

                    wined3d_device_context_emit_blt_sub_resource(&device->cs->c,
                            &dst_texture->resource, i * dst_level_count + j, &box,
                            &src_texture->resource, i * src_level_count + src_level, &box,
                            0, NULL, WINED3D_TEXF_POINT);
                }
            }
        }
    }

    wined3d_texture_clear_dirty_regions(src_texture);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_validate_device(const struct wined3d_device *device, const struct wined3d_stateblock_state *state, DWORD *num_passes)
{
    const struct wined3d_state *device_state = device->cs->c.state;
    struct wined3d_texture *texture;
    unsigned i;

    TRACE("device %p, num_passes %p.\n", device, num_passes);

    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
    {
        if (state->sampler_states[i][WINED3D_SAMP_MIN_FILTER] == WINED3D_TEXF_NONE)
        {
            WARN("Sampler state %u has minfilter D3DTEXF_NONE, returning D3DERR_UNSUPPORTEDTEXTUREFILTER\n", i);
            return WINED3DERR_UNSUPPORTEDTEXTUREFILTER;
        }
        if (state->sampler_states[i][WINED3D_SAMP_MAG_FILTER] == WINED3D_TEXF_NONE)
        {
            WARN("Sampler state %u has magfilter D3DTEXF_NONE, returning D3DERR_UNSUPPORTEDTEXTUREFILTER\n", i);
            return WINED3DERR_UNSUPPORTEDTEXTUREFILTER;
        }

        texture = state->textures[i];
        if (!texture || texture->resource.format_caps & WINED3D_FORMAT_CAP_FILTERING)
            continue;

        if (state->sampler_states[i][WINED3D_SAMP_MAG_FILTER] != WINED3D_TEXF_POINT)
        {
            WARN("Non-filterable texture and mag filter enabled on sampler %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
        if (state->sampler_states[i][WINED3D_SAMP_MIN_FILTER] != WINED3D_TEXF_POINT)
        {
            WARN("Non-filterable texture and min filter enabled on sampler %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
        if (state->sampler_states[i][WINED3D_SAMP_MIP_FILTER] != WINED3D_TEXF_NONE
                && state->sampler_states[i][WINED3D_SAMP_MIP_FILTER] != WINED3D_TEXF_POINT)
        {
            WARN("Non-filterable texture and mip filter enabled on sampler %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
    }

    if (state->rs[WINED3D_RS_ZENABLE] || state->rs[WINED3D_RS_ZWRITEENABLE] || state->rs[WINED3D_RS_STENCILENABLE])
    {
        struct wined3d_rendertarget_view *rt = device_state->fb.render_targets[0];
        struct wined3d_rendertarget_view *ds = device_state->fb.depth_stencil;

        if (ds && rt && (ds->width < rt->width || ds->height < rt->height))
        {
            WARN("Depth stencil is smaller than the color buffer, returning D3DERR_CONFLICTINGRENDERSTATE\n");
            return WINED3DERR_CONFLICTINGRENDERSTATE;
        }
    }

    /* return a sensible default */
    *num_passes = 1;

    TRACE("returning D3D_OK\n");
    return WINED3D_OK;
}

void CDECL wined3d_device_set_software_vertex_processing(struct wined3d_device *device, BOOL software)
{
    static BOOL warned;

    TRACE("device %p, software %#x.\n", device, software);

    if (!warned)
    {
        FIXME("device %p, software %#x stub!\n", device, software);
        warned = TRUE;
    }

    device->softwareVertexProcessing = software;
}

BOOL CDECL wined3d_device_get_software_vertex_processing(const struct wined3d_device *device)
{
    static BOOL warned;

    TRACE("device %p.\n", device);

    if (!warned)
    {
        TRACE("device %p stub!\n", device);
        warned = TRUE;
    }

    return device->softwareVertexProcessing;
}

HRESULT CDECL wined3d_device_get_raster_status(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_raster_status *raster_status)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, raster_status %p.\n",
            device, swapchain_idx, raster_status);

    if (!(swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        return WINED3DERR_INVALIDCALL;

    return wined3d_swapchain_get_raster_status(swapchain, raster_status);
}

HRESULT CDECL wined3d_device_set_npatch_mode(struct wined3d_device *device, float segments)
{
    static BOOL warned;

    TRACE("device %p, segments %.8e.\n", device, segments);

    if (segments != 0.0f)
    {
        if (!warned)
        {
            FIXME("device %p, segments %.8e stub!\n", device, segments);
            warned = TRUE;
        }
    }

    return WINED3D_OK;
}

float CDECL wined3d_device_get_npatch_mode(const struct wined3d_device *device)
{
    static BOOL warned;

    TRACE("device %p.\n", device);

    if (!warned)
    {
        FIXME("device %p stub!\n", device);
        warned = TRUE;
    }

    return 0.0f;
}

void CDECL wined3d_device_context_copy_uav_counter(struct wined3d_device_context *context,
        struct wined3d_buffer *dst_buffer, unsigned int offset, struct wined3d_unordered_access_view *uav)
{
    TRACE("context %p, dst_buffer %p, offset %u, uav %p.\n",
            context, dst_buffer, offset, uav);

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_copy_uav_counter(context, dst_buffer, offset, uav);
    wined3d_device_context_unlock(context);
}

static bool resources_format_compatible(const struct wined3d_resource *src_resource,
        const struct wined3d_resource *dst_resource)
{
    if (src_resource->format->id == dst_resource->format->id)
        return true;
    if (src_resource->format->typeless_id && src_resource->format->typeless_id == dst_resource->format->typeless_id)
        return true;
    if (src_resource->device->cs->c.state->feature_level < WINED3D_FEATURE_LEVEL_10_1)
        return false;
    if ((src_resource->format_attrs & WINED3D_FORMAT_ATTR_BLOCKS)
            && (dst_resource->format_attrs & WINED3D_FORMAT_ATTR_CAST_TO_BLOCK))
        return src_resource->format->block_byte_count == dst_resource->format->byte_count;
    if ((src_resource->format_attrs & WINED3D_FORMAT_ATTR_CAST_TO_BLOCK)
            && (dst_resource->format_attrs & WINED3D_FORMAT_ATTR_BLOCKS))
        return src_resource->format->byte_count == dst_resource->format->block_byte_count;
    return false;
}

void CDECL wined3d_device_context_copy_resource(struct wined3d_device_context *context,
        struct wined3d_resource *dst_resource, struct wined3d_resource *src_resource)
{
    unsigned int src_row_block_count, dst_row_block_count;
    struct wined3d_texture *dst_texture, *src_texture;
    unsigned int src_row_count, dst_row_count;
    struct wined3d_box src_box, dst_box;
    unsigned int i, j;

    TRACE("context %p, dst_resource %p, src_resource %p.\n", context, dst_resource, src_resource);

    if (src_resource == dst_resource)
    {
        WARN("Source and destination are the same resource.\n");
        return;
    }

    if (src_resource->type != dst_resource->type)
    {
        WARN("Resource types (%s / %s) don't match.\n",
                debug_d3dresourcetype(dst_resource->type),
                debug_d3dresourcetype(src_resource->type));
        return;
    }

    if (!resources_format_compatible(src_resource, dst_resource))
    {
        WARN("Resource formats %s and %s are incompatible.\n",
                debug_d3dformat(dst_resource->format->id),
                debug_d3dformat(src_resource->format->id));
        return;
    }

    src_row_block_count = (src_resource->width + (src_resource->format->block_width - 1))
            / src_resource->format->block_width;
    dst_row_block_count = (dst_resource->width + (dst_resource->format->block_width - 1))
            / dst_resource->format->block_width;
    src_row_count = (src_resource->height + (src_resource->format->block_height - 1))
            / src_resource->format->block_height;
    dst_row_count = (dst_resource->height + (dst_resource->format->block_height - 1))
            / dst_resource->format->block_height;

    if (src_row_block_count != dst_row_block_count || src_row_count != dst_row_count
            || src_resource->depth != dst_resource->depth)
    {
        WARN("Resource block dimensions (%ux%ux%u / %ux%ux%u) don't match.\n",
                dst_row_block_count, dst_row_count, dst_resource->depth,
                src_row_block_count, src_row_count, src_resource->depth);
        return;
    }

    if (dst_resource->type == WINED3D_RTYPE_BUFFER)
    {
        wined3d_box_set(&src_box, 0, 0, src_resource->size, 1, 0, 1);
        wined3d_device_context_lock(context);
        wined3d_device_context_emit_blt_sub_resource(context, dst_resource, 0, &src_box,
                src_resource, 0, &src_box, WINED3D_BLT_RAW, NULL, WINED3D_TEXF_POINT);
        wined3d_device_context_unlock(context);
        return;
    }

    dst_texture = texture_from_resource(dst_resource);
    src_texture = texture_from_resource(src_resource);

    if (src_texture->layer_count != dst_texture->layer_count
            || src_texture->level_count != dst_texture->level_count)
    {
        WARN("Subresource layouts (%ux%u / %ux%u) don't match.\n",
                dst_texture->layer_count, dst_texture->level_count,
                src_texture->layer_count, src_texture->level_count);
        return;
    }

    wined3d_device_context_lock(context);
    for (i = 0; i < dst_texture->level_count; ++i)
    {
        wined3d_texture_get_level_box(src_texture, i, &src_box);
        wined3d_texture_get_level_box(dst_texture, i, &dst_box);
        for (j = 0; j < dst_texture->layer_count; ++j)
        {
            unsigned int idx = j * dst_texture->level_count + i;

            wined3d_device_context_emit_blt_sub_resource(context, dst_resource, idx, &dst_box,
                    src_resource, idx, &src_box, WINED3D_BLT_RAW, NULL, WINED3D_TEXF_POINT);
        }
    }
    wined3d_device_context_unlock(context);
}

HRESULT CDECL wined3d_device_context_copy_sub_resource_region(struct wined3d_device_context *context,
        struct wined3d_resource *dst_resource, unsigned int dst_sub_resource_idx, unsigned int dst_x,
        unsigned int dst_y, unsigned int dst_z, struct wined3d_resource *src_resource,
        unsigned int src_sub_resource_idx, const struct wined3d_box *src_box, unsigned int flags)
{
    struct wined3d_box dst_box, b;

    TRACE("context %p, dst_resource %p, dst_sub_resource_idx %u, dst_x %u, dst_y %u, dst_z %u, "
            "src_resource %p, src_sub_resource_idx %u, src_box %s, flags %#x.\n",
            context, dst_resource, dst_sub_resource_idx, dst_x, dst_y, dst_z,
            src_resource, src_sub_resource_idx, debug_box(src_box), flags);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    if (src_resource == dst_resource && src_sub_resource_idx == dst_sub_resource_idx)
    {
        WARN("Source and destination are the same sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!resources_format_compatible(src_resource, dst_resource))
    {
        WARN("Resource formats %s and %s are incompatible.\n",
                debug_d3dformat(dst_resource->format->id),
                debug_d3dformat(src_resource->format->id));
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_resource->type == WINED3D_RTYPE_BUFFER)
    {
        if (src_resource->type != WINED3D_RTYPE_BUFFER)
        {
            WARN("Resource types (%s / %s) don't match.\n",
                    debug_d3dresourcetype(dst_resource->type),
                    debug_d3dresourcetype(src_resource->type));
            return WINED3DERR_INVALIDCALL;
        }

        if (dst_sub_resource_idx)
        {
            WARN("Invalid dst_sub_resource_idx %u.\n", dst_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (src_sub_resource_idx)
        {
            WARN("Invalid src_sub_resource_idx %u.\n", src_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (!src_box)
        {
            unsigned int dst_w;

            dst_w = dst_resource->size - dst_x;
            wined3d_box_set(&b, 0, 0, min(src_resource->size, dst_w), 1, 0, 1);
            src_box = &b;
        }
        else if ((src_box->left >= src_box->right
                || src_box->top >= src_box->bottom
                || src_box->front >= src_box->back))
        {
            WARN("Invalid box %s specified.\n", debug_box(src_box));
            return WINED3DERR_INVALIDCALL;
        }

        if (src_box->right > src_resource->size || dst_x >= dst_resource->size
                || src_box->right - src_box->left > dst_resource->size - dst_x)
        {
            WARN("Invalid range specified, dst_offset %u, src_offset %u, size %u.\n",
                    dst_x, src_box->left, src_box->right - src_box->left);
            return WINED3DERR_INVALIDCALL;
        }

        wined3d_box_set(&dst_box, dst_x, 0, dst_x + (src_box->right - src_box->left), 1, 0, 1);
    }
    else
    {
        struct wined3d_texture *dst_texture = texture_from_resource(dst_resource);
        struct wined3d_texture *src_texture = texture_from_resource(src_resource);
        unsigned int src_level = src_sub_resource_idx % src_texture->level_count;
        unsigned int src_row_block_count, src_row_count;

        if (!wined3d_texture_validate_sub_resource_idx(dst_texture, dst_sub_resource_idx))
            return WINED3DERR_INVALIDCALL;

        if (!wined3d_texture_validate_sub_resource_idx(src_texture, src_sub_resource_idx))
            return WINED3DERR_INVALIDCALL;

        if (dst_texture->sub_resources[dst_sub_resource_idx].map_count)
        {
            WARN("Destination sub-resource %u is mapped.\n", dst_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (src_texture->sub_resources[src_sub_resource_idx].map_count)
        {
            WARN("Source sub-resource %u is mapped.\n", src_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (!src_box)
        {
            unsigned int src_w, src_h, src_d, dst_w, dst_h, dst_d, dst_level;

            src_w = wined3d_texture_get_level_width(src_texture, src_level);
            src_h = wined3d_texture_get_level_height(src_texture, src_level);
            src_d = wined3d_texture_get_level_depth(src_texture, src_level);

            dst_level = dst_sub_resource_idx % dst_texture->level_count;
            dst_w = wined3d_texture_get_level_width(dst_texture, dst_level) - dst_x;
            dst_h = wined3d_texture_get_level_height(dst_texture, dst_level) - dst_y;
            dst_d = wined3d_texture_get_level_depth(dst_texture, dst_level) - dst_z;

            wined3d_box_set(&b, 0, 0, min(src_w, dst_w), min(src_h, dst_h), 0, min(src_d, dst_d));
            src_box = &b;
        }
        else if (FAILED(wined3d_resource_check_box_dimensions(src_resource, src_sub_resource_idx, src_box)))
        {
            WARN("Invalid source box %s.\n", debug_box(src_box));
            return WINED3DERR_INVALIDCALL;
        }

        if (src_resource->format->block_width == dst_resource->format->block_width
                && src_resource->format->block_height == dst_resource->format->block_height)
        {
            wined3d_box_set(&dst_box, dst_x, dst_y, dst_x + (src_box->right - src_box->left),
                    dst_y + (src_box->bottom - src_box->top), dst_z, dst_z + (src_box->back - src_box->front));
        }
        else
        {
            src_row_block_count = (src_box->right - src_box->left + src_resource->format->block_width - 1)
                    / src_resource->format->block_width;
            src_row_count = (src_box->bottom - src_box->top + src_resource->format->block_height - 1)
                    / src_resource->format->block_height;
            wined3d_box_set(&dst_box, dst_x, dst_y,
                    dst_x + (src_row_block_count * dst_resource->format->block_width),
                    dst_y + (src_row_count * dst_resource->format->block_height),
                    dst_z, dst_z + (src_box->back - src_box->front));
        }
        if (FAILED(wined3d_resource_check_box_dimensions(dst_resource, dst_sub_resource_idx, &dst_box)))
        {
            WARN("Invalid destination box %s.\n", debug_box(&dst_box));
            return WINED3DERR_INVALIDCALL;
        }
    }

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_blt_sub_resource(context, dst_resource, dst_sub_resource_idx, &dst_box,
            src_resource, src_sub_resource_idx, src_box, WINED3D_BLT_RAW, NULL, WINED3D_TEXF_POINT);
    wined3d_device_context_unlock(context);

    return WINED3D_OK;
}

void CDECL wined3d_device_context_update_sub_resource(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx, const struct wined3d_box *box,
        const void *data, unsigned int row_pitch, unsigned int depth_pitch, unsigned int flags)
{
    struct wined3d_sub_resource_desc desc;
    struct wined3d_box b;

    TRACE("context %p, resource %p, sub_resource_idx %u, box %s, data %p, row_pitch %u, depth_pitch %u, flags %#x.\n",
            context, resource, sub_resource_idx, debug_box(box), data, row_pitch, depth_pitch, flags);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    if (!(resource->access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        WARN("Resource %p is not GPU accessible.\n", resource);
        return;
    }

    if (FAILED(wined3d_resource_get_sub_resource_desc(resource, sub_resource_idx, &desc)))
        return;

    if (!box)
    {
        wined3d_box_set(&b, 0, 0, desc.width, desc.height, 0, desc.depth);
        box = &b;
    }
    else if (box->left >= box->right || box->right > desc.width
            || box->top >= box->bottom || box->bottom > desc.height
            || box->front >= box->back || box->back > desc.depth)
    {
        WARN("Invalid box %s specified.\n", debug_box(box));
        return;
    }

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_update_sub_resource(context, resource,
            sub_resource_idx, box, data, row_pitch, depth_pitch);
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_resolve_sub_resource(struct wined3d_device_context *context,
        struct wined3d_resource *dst_resource, unsigned int dst_sub_resource_idx,
        struct wined3d_resource *src_resource, unsigned int src_sub_resource_idx,
        enum wined3d_format_id format_id)
{
    struct wined3d_texture *dst_texture, *src_texture;
    unsigned int dst_level, src_level;
    struct wined3d_blt_fx fx = {0};
    RECT dst_rect, src_rect;

    TRACE("context %p, dst_resource %p, dst_sub_resource_idx %u, "
            "src_resource %p, src_sub_resource_idx %u, format %s.\n",
            context, dst_resource, dst_sub_resource_idx,
            src_resource, src_sub_resource_idx, debug_d3dformat(format_id));

    if (wined3d_format_is_typeless(dst_resource->format)
            || wined3d_format_is_typeless(src_resource->format))
    {
        FIXME("Multisample resolve is not fully supported for typeless formats "
                "(dst_format %s, src_format %s, format %s).\n",
                debug_d3dformat(dst_resource->format->id), debug_d3dformat(src_resource->format->id),
                debug_d3dformat(format_id));
    }
    if (dst_resource->type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Invalid destination resource type %s.\n", debug_d3dresourcetype(dst_resource->type));
        return;
    }
    if (src_resource->type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Invalid source resource type %s.\n", debug_d3dresourcetype(src_resource->type));
        return;
    }

    wined3d_device_context_lock(context);
    fx.resolve_format_id = format_id;

    dst_texture = texture_from_resource(dst_resource);
    src_texture = texture_from_resource(src_resource);

    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    SetRect(&dst_rect, 0, 0, wined3d_texture_get_level_width(dst_texture, dst_level),
            wined3d_texture_get_level_height(dst_texture, dst_level));
    src_level = src_sub_resource_idx % src_texture->level_count;
    SetRect(&src_rect, 0, 0, wined3d_texture_get_level_width(src_texture, src_level),
            wined3d_texture_get_level_height(src_texture, src_level));
    wined3d_device_context_blt(context, dst_texture, dst_sub_resource_idx, &dst_rect,
            src_texture, src_sub_resource_idx, &src_rect, 0, &fx, WINED3D_TEXF_POINT);
    wined3d_device_context_unlock(context);
}

HRESULT CDECL wined3d_device_context_clear_rendertarget_view(struct wined3d_device_context *context,
        struct wined3d_rendertarget_view *view, const RECT *rect, unsigned int flags,
        const struct wined3d_color *color, float depth, unsigned int stencil)
{
    struct wined3d_resource *resource;
    RECT r;

    TRACE("context %p, view %p, rect %s, flags %#x, color %s, depth %.8e, stencil %u.\n",
            context, view, wine_dbgstr_rect(rect), flags, debug_color(color), depth, stencil);

    if (!flags)
        return WINED3D_OK;

    resource = view->resource;
    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(resource->type));
        return WINED3DERR_INVALIDCALL;
    }

    if (!rect)
    {
        SetRect(&r, 0, 0, view->width, view->height);
        rect = &r;
    }
    else
    {
        struct wined3d_box b = {rect->left, rect->top, rect->right, rect->bottom, 0, 1};
        HRESULT hr;

        if (FAILED(hr = wined3d_resource_check_box_dimensions(resource, view->sub_resource_idx, &b)))
            return hr;
    }

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_clear_rendertarget_view(context, view, rect, flags, color, depth, stencil);
    wined3d_device_context_unlock(context);

    return WINED3D_OK;
}

void CDECL wined3d_device_context_clear_uav_float(struct wined3d_device_context *context,
        struct wined3d_unordered_access_view *view, const struct wined3d_vec4 *clear_value)
{
    TRACE("context %p, view %p, clear_value %s.\n", context, view, debug_vec4(clear_value));

    if (!(view->format->attrs & (WINED3D_FORMAT_ATTR_FLOAT | WINED3D_FORMAT_ATTR_NORMALISED)))
    {
        WARN("Not supported for view format %s.\n", debug_d3dformat(view->format->id));
        return;
    }

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_clear_uav(context, view, (const struct wined3d_uvec4 *)clear_value, true);
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_clear_uav_uint(struct wined3d_device_context *context,
        struct wined3d_unordered_access_view *view, const struct wined3d_uvec4 *clear_value)
{
    TRACE("context %p, view %p, clear_value %s.\n", context, view, debug_uvec4(clear_value));

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_clear_uav(context, view, clear_value, false);
    wined3d_device_context_unlock(context);
}

static unsigned int sanitise_map_flags(const struct wined3d_resource *resource, unsigned int flags)
{
    /* Not all flags make sense together, but Windows never returns an error.
     * Catch the cases that could cause issues. */
    if (flags & WINED3D_MAP_READ)
    {
        if (flags & WINED3D_MAP_DISCARD)
        {
            WARN("WINED3D_MAP_READ combined with WINED3D_MAP_DISCARD, ignoring flags.\n");
            return flags & (WINED3D_MAP_READ | WINED3D_MAP_WRITE);
        }
        if (flags & WINED3D_MAP_NOOVERWRITE)
        {
            WARN("WINED3D_MAP_READ combined with WINED3D_MAP_NOOVERWRITE, ignoring flags.\n");
            return flags & (WINED3D_MAP_READ | WINED3D_MAP_WRITE);
        }
    }
    else if (flags & (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE))
    {
        if (!(resource->access & WINED3D_RESOURCE_ACCESS_GPU) || !(resource->usage & WINED3DUSAGE_DYNAMIC))
        {
            WARN("DISCARD or NOOVERWRITE map not on dynamic GPU-accessible buffer, ignoring.\n");
            return flags & (WINED3D_MAP_READ | WINED3D_MAP_WRITE);
        }
        if ((flags & (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE))
                == (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE))
        {
            WARN("WINED3D_MAP_NOOVERWRITE used with WINED3D_MAP_DISCARD, ignoring WINED3D_MAP_DISCARD.\n");
            flags &= ~WINED3D_MAP_DISCARD;
        }
    }

    return flags;
}

HRESULT CDECL wined3d_device_context_map(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, unsigned int flags)
{
    struct wined3d_sub_resource_desc desc;
    struct wined3d_box b;
    HRESULT hr;

    TRACE("context %p, resource %p, sub_resource_idx %u, map_desc %p, box %s, flags %#x.\n",
            context, resource, sub_resource_idx, map_desc, debug_box(box), flags);

    if (!(flags & (WINED3D_MAP_READ | WINED3D_MAP_WRITE)))
    {
        WARN("No read/write flags specified.\n");
        return E_INVALIDARG;
    }

    if ((flags & WINED3D_MAP_READ) && !(resource->access & WINED3D_RESOURCE_ACCESS_MAP_R))
    {
        WARN("Resource does not have MAP_R access.\n");
        return E_INVALIDARG;
    }

    if ((flags & WINED3D_MAP_WRITE) && !(resource->access & WINED3D_RESOURCE_ACCESS_MAP_W))
    {
        WARN("Resource does not have MAP_W access.\n");
        return E_INVALIDARG;
    }

    flags = sanitise_map_flags(resource, flags);

    if (FAILED(wined3d_resource_get_sub_resource_desc(resource, sub_resource_idx, &desc)))
        return E_INVALIDARG;

    if (!box)
    {
        wined3d_box_set(&b, 0, 0, desc.width, desc.height, 0, desc.depth);
        box = &b;
    }
    else if (FAILED(wined3d_resource_check_box_dimensions(resource, sub_resource_idx, box)))
    {
        WARN("Map box is invalid.\n");

        if (resource->type != WINED3D_RTYPE_BUFFER && resource->type != WINED3D_RTYPE_TEXTURE_2D)
            return WINED3DERR_INVALIDCALL;

        if ((resource->format_attrs & WINED3D_FORMAT_ATTR_BLOCKS) &&
                !(resource->access & WINED3D_RESOURCE_ACCESS_CPU))
            return WINED3DERR_INVALIDCALL;
    }

    wined3d_device_context_lock(context);
    hr = wined3d_device_context_emit_map(context, resource, sub_resource_idx, map_desc, box, flags);
    wined3d_device_context_unlock(context);
    return hr;
}

HRESULT CDECL wined3d_device_context_unmap(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    HRESULT hr;
    TRACE("context %p, resource %p, sub_resource_idx %u.\n", context, resource, sub_resource_idx);

    wined3d_device_context_lock(context);
    hr = wined3d_device_context_emit_unmap(context, resource, sub_resource_idx);
    wined3d_device_context_unlock(context);
    return hr;
}

void CDECL wined3d_device_context_issue_query(struct wined3d_device_context *context,
        struct wined3d_query *query, unsigned int flags)
{
    TRACE("context %p, query %p, flags %#x.\n", context, query, flags);

    wined3d_device_context_lock(context);
    context->ops->issue_query(context, query, flags);
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_execute_command_list(struct wined3d_device_context *context,
        struct wined3d_command_list *list, bool restore_state)
{
    TRACE("context %p, list %p, restore_state %d.\n", context, list, restore_state);

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_execute_command_list(context, list, restore_state);
    wined3d_device_context_unlock(context);
}

struct wined3d_rendertarget_view * CDECL wined3d_device_context_get_rendertarget_view(
        const struct wined3d_device_context *context, unsigned int view_idx)
{
    unsigned int max_rt_count;

    TRACE("context %p, view_idx %u.\n", context, view_idx);

    max_rt_count = context->device->adapter->d3d_info.limits.max_rt_count;
    if (view_idx >= max_rt_count)
    {
        WARN("Only %u render targets are supported.\n", max_rt_count);
        return NULL;
    }

    return context->state->fb.render_targets[view_idx];
}

struct wined3d_rendertarget_view * CDECL wined3d_device_context_get_depth_stencil_view(
        const struct wined3d_device_context *context)
{
    TRACE("context %p.\n", context);

    return context->state->fb.depth_stencil;
}

void CDECL wined3d_device_context_generate_mipmaps(struct wined3d_device_context *context,
        struct wined3d_shader_resource_view *view)
{
    struct wined3d_texture *texture;

    TRACE("context %p, view %p.\n", context, view);

    if (view->resource->type == WINED3D_RTYPE_BUFFER)
    {
        WARN("Called on buffer resource %p.\n", view->resource);
        return;
    }

    texture = texture_from_resource(view->resource);
    if (!(texture->flags & WINED3D_TEXTURE_GENERATE_MIPMAPS))
    {
        WARN("Texture without the WINED3D_TEXTURE_GENERATE_MIPMAPS flag, ignoring.\n");
        return;
    }

    wined3d_device_context_lock(context);
    wined3d_device_context_emit_generate_mipmaps(context, view);
    wined3d_device_context_unlock(context);
}

static struct wined3d_texture *wined3d_device_create_cursor_texture(struct wined3d_device *device,
        struct wined3d_texture *cursor_image, unsigned int sub_resource_idx)
{
    unsigned int texture_level = sub_resource_idx % cursor_image->level_count;
    struct wined3d_sub_resource_data data;
    struct wined3d_resource_desc desc;
    struct wined3d_map_desc map_desc;
    struct wined3d_texture *texture;
    HRESULT hr;

    if (FAILED(wined3d_resource_map(&cursor_image->resource, sub_resource_idx, &map_desc, NULL, WINED3D_MAP_READ)))
    {
        ERR("Failed to map source texture.\n");
        return NULL;
    }

    data.data = map_desc.data;
    data.row_pitch = map_desc.row_pitch;
    data.slice_pitch = map_desc.slice_pitch;

    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = WINED3DFMT_B8G8R8A8_UNORM;
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = WINED3DUSAGE_DYNAMIC;
    desc.bind_flags = 0;
    desc.access = WINED3D_RESOURCE_ACCESS_GPU;
    desc.width = wined3d_texture_get_level_width(cursor_image, texture_level);
    desc.height = wined3d_texture_get_level_height(cursor_image, texture_level);
    desc.depth = 1;
    desc.size = 0;

    hr = wined3d_texture_create(device, &desc, 1, 1, 0, &data, NULL, &wined3d_null_parent_ops, &texture);
    wined3d_resource_unmap(&cursor_image->resource, sub_resource_idx);
    if (FAILED(hr))
    {
        ERR("Failed to create cursor texture.\n");
        return NULL;
    }

    return texture;
}

HRESULT CDECL wined3d_device_set_cursor_properties(struct wined3d_device *device,
        UINT x_hotspot, UINT y_hotspot, struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    unsigned int texture_level = sub_resource_idx % texture->level_count;
    unsigned int cursor_width, cursor_height;
    struct wined3d_map_desc map_desc;

    TRACE("device %p, x_hotspot %u, y_hotspot %u, texture %p, sub_resource_idx %u.\n",
            device, x_hotspot, y_hotspot, texture, sub_resource_idx);

    if (!wined3d_texture_validate_sub_resource_idx(texture, sub_resource_idx)
            || texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
        return WINED3DERR_INVALIDCALL;

    if (device->cursor_texture)
    {
        wined3d_texture_decref(device->cursor_texture);
        device->cursor_texture = NULL;
    }

    if (texture->resource.format->id != WINED3DFMT_B8G8R8A8_UNORM)
    {
        WARN("Texture %p has invalid format %s.\n",
                texture, debug_d3dformat(texture->resource.format->id));
        return WINED3DERR_INVALIDCALL;
    }

    /* Cursor width and height must all be powers of two */
    cursor_width = wined3d_texture_get_level_width(texture, texture_level);
    cursor_height = wined3d_texture_get_level_height(texture, texture_level);
    if ((cursor_width & (cursor_width - 1)) || (cursor_height & (cursor_height - 1)))
    {
        WARN("Cursor size %ux%u are not all powers of two.\n", cursor_width, cursor_height);
        return WINED3DERR_INVALIDCALL;
    }

    /* Do not store the surface's pointer because the application may
     * release it after setting the cursor image. Windows doesn't
     * addref the set surface, so we can't do this either without
     * creating circular refcount dependencies. */
    if (!(device->cursor_texture = wined3d_device_create_cursor_texture(device, texture, sub_resource_idx)))
    {
        ERR("Failed to create cursor texture.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (cursor_width == 32 && cursor_height == 32)
    {
        UINT mask_size = cursor_width * cursor_height / 8;
        ICONINFO cursor_info;
        DWORD *mask_bits;
        HCURSOR cursor;

        /* 32-bit user32 cursors ignore the alpha channel if it's all
         * zeroes, and use the mask instead. Fill the mask with all ones
         * to ensure we still get a fully transparent cursor. */
        if (!(mask_bits = malloc(mask_size)))
            return E_OUTOFMEMORY;
        memset(mask_bits, 0xff, mask_size);

        wined3d_resource_map(&texture->resource, sub_resource_idx, &map_desc, NULL,
                WINED3D_MAP_NO_DIRTY_UPDATE | WINED3D_MAP_READ);
        cursor_info.fIcon = FALSE;
        cursor_info.xHotspot = x_hotspot;
        cursor_info.yHotspot = y_hotspot;
        cursor_info.hbmMask = CreateBitmap(cursor_width, cursor_height, 1, 1, mask_bits);
        cursor_info.hbmColor = CreateBitmap(cursor_width, cursor_height, 1, 32, map_desc.data);
        wined3d_resource_unmap(&texture->resource, sub_resource_idx);

        /* Create our cursor and clean up. */
        cursor = CreateIconIndirect(&cursor_info);
        if (cursor_info.hbmMask)
            DeleteObject(cursor_info.hbmMask);
        if (cursor_info.hbmColor)
            DeleteObject(cursor_info.hbmColor);
        if (device->hardwareCursor)
            DestroyCursor(device->hardwareCursor);
        device->hardwareCursor = cursor;
        if (device->bCursorVisible)
            SetCursor(cursor);

        free(mask_bits);
    }

    TRACE("New cursor dimensions are %ux%u.\n", cursor_width, cursor_height);
    device->cursorWidth = cursor_width;
    device->cursorHeight = cursor_height;
    device->xHotSpot = x_hotspot;
    device->yHotSpot = y_hotspot;

    return WINED3D_OK;
}

void CDECL wined3d_device_set_cursor_position(struct wined3d_device *device,
        int x_screen_space, int y_screen_space, uint32_t flags)
{
    TRACE("device %p, x %d, y %d, flags %#x.\n",
            device, x_screen_space, y_screen_space, flags);

    device->xScreenSpace = x_screen_space;
    device->yScreenSpace = y_screen_space;

    if (device->hardwareCursor)
    {
        POINT pt;

        GetCursorPos( &pt );
        if (x_screen_space == pt.x && y_screen_space == pt.y)
            return;
        SetCursorPos( x_screen_space, y_screen_space );

        /* Switch to the software cursor if position diverges from the hardware one. */
        GetCursorPos( &pt );
        if (x_screen_space != pt.x || y_screen_space != pt.y)
        {
            if (device->bCursorVisible) SetCursor( NULL );
            DestroyCursor( device->hardwareCursor );
            device->hardwareCursor = 0;
        }
    }
}

BOOL CDECL wined3d_device_show_cursor(struct wined3d_device *device, BOOL show)
{
    BOOL oldVisible = device->bCursorVisible;

    TRACE("device %p, show %#x.\n", device, show);

    /*
     * When ShowCursor is first called it should make the cursor appear at the OS's last
     * known cursor position.
     */
    if (show && !oldVisible)
    {
        POINT pt;
        GetCursorPos(&pt);
        device->xScreenSpace = pt.x;
        device->yScreenSpace = pt.y;
    }

    if (device->hardwareCursor)
    {
        device->bCursorVisible = show;
        if (show)
            SetCursor(device->hardwareCursor);
        else
            SetCursor(NULL);
    }
    else if (device->cursor_texture)
    {
        device->bCursorVisible = show;
    }

    return oldVisible;
}

static void mark_managed_resource_dirty(struct wined3d_resource *resource)
{
    if (resource->type != WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_texture *texture = texture_from_resource(resource);
        unsigned int i;

        if (texture->dirty_regions)
        {
            for (i = 0; i < texture->layer_count; ++i)
                wined3d_texture_add_dirty_region(texture, i, NULL);
        }
    }
}

void CDECL wined3d_device_evict_managed_resources(struct wined3d_device *device)
{
    struct wined3d_resource *resource, *cursor;

    TRACE("device %p.\n", device);

    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Checking resource %p for eviction.\n", resource);

        if ((resource->usage & WINED3DUSAGE_MANAGED) && !resource->map_count)
        {
            if (resource->access & WINED3D_RESOURCE_ACCESS_GPU)
            {
                TRACE("Evicting %p.\n", resource);
                wined3d_cs_emit_unload_resource(device->cs, resource);
            }

            mark_managed_resource_dirty(resource);
        }
    }
}

void CDECL wined3d_device_context_flush(struct wined3d_device_context *context)
{
    TRACE("context %p.\n", context);

    wined3d_device_context_lock(context);
    context->ops->flush(context);
    wined3d_device_context_unlock(context);
}

static void update_swapchain_flags(struct wined3d_texture *texture)
{
    unsigned int flags = texture->swapchain->state.desc.flags;

    if (flags & WINED3D_SWAPCHAIN_LOCKABLE_BACKBUFFER)
        texture->resource.access |= WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    else
        texture->resource.access &= ~(WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W);

    if (flags & WINED3D_SWAPCHAIN_GDI_COMPATIBLE)
        texture->flags |= WINED3D_TEXTURE_GET_DC;
    else
        texture->flags &= ~WINED3D_TEXTURE_GET_DC;
}

HRESULT CDECL wined3d_device_reset(struct wined3d_device *device,
        const struct wined3d_swapchain_desc *swapchain_desc, const struct wined3d_display_mode *mode,
        wined3d_device_reset_cb callback, BOOL reset_state)
{
    static struct wined3d_rendertarget_view *const views[WINED3D_MAX_RENDER_TARGETS];
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    struct wined3d_device_context *context = &device->cs->c;
    struct wined3d_swapchain_state *swapchain_state;
    struct wined3d_state *state = context->state;
    struct wined3d_swapchain_desc *current_desc;
    struct wined3d_resource *resource, *cursor;
    struct wined3d_rendertarget_view *view;
    struct wined3d_swapchain *swapchain;
    struct wined3d_view_desc view_desc;
    BOOL backbuffer_resized, windowed;
    HRESULT hr = WINED3D_OK;
    HWND device_window;
    unsigned int i;

    TRACE("device %p, swapchain_desc %p, mode %p, callback %p, reset_state %#x.\n",
            device, swapchain_desc, mode, callback, reset_state);

    wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);

    if (!(swapchain = wined3d_device_get_swapchain(device, 0)))
    {
        ERR("Failed to get the first implicit swapchain.\n");
        return WINED3DERR_INVALIDCALL;
    }
    swapchain_state = &swapchain->state;
    current_desc = &swapchain_state->desc;

    if (reset_state)
    {
        if (device->logo_texture)
        {
            wined3d_texture_decref(device->logo_texture);
            device->logo_texture = NULL;
        }
        if (device->cursor_texture)
        {
            wined3d_texture_decref(device->cursor_texture);
            device->cursor_texture = NULL;
        }
        for (unsigned int i = 0; i < ARRAY_SIZE(device->push_constants); ++i)
        {
            if (device->push_constants[i])
                wined3d_buffer_decref(device->push_constants[i]);
            device->push_constants[i] = NULL;
        }
        state_unbind_resources(state);
    }

    wined3d_device_context_set_rendertarget_views(context, 0, d3d_info->limits.max_rt_count, views, FALSE);
    wined3d_device_context_set_depth_stencil_view(context, NULL);

    if (reset_state)
    {
        LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
        {
            TRACE("Enumerating resource %p.\n", resource);
            if (FAILED(hr = callback(resource)))
                return hr;
        }
    }

    TRACE("New params:\n");
    TRACE("output %p\n", swapchain_desc->output);
    TRACE("backbuffer_width %u\n", swapchain_desc->backbuffer_width);
    TRACE("backbuffer_height %u\n", swapchain_desc->backbuffer_height);
    TRACE("backbuffer_format %s\n", debug_d3dformat(swapchain_desc->backbuffer_format));
    TRACE("backbuffer_count %u\n", swapchain_desc->backbuffer_count);
    TRACE("multisample_type %#x\n", swapchain_desc->multisample_type);
    TRACE("multisample_quality %u\n", swapchain_desc->multisample_quality);
    TRACE("swap_effect %#x\n", swapchain_desc->swap_effect);
    TRACE("device_window %p\n", swapchain_desc->device_window);
    TRACE("windowed %#x\n", swapchain_desc->windowed);
    TRACE("enable_auto_depth_stencil %#x\n", swapchain_desc->enable_auto_depth_stencil);
    if (swapchain_desc->enable_auto_depth_stencil)
        TRACE("auto_depth_stencil_format %s\n", debug_d3dformat(swapchain_desc->auto_depth_stencil_format));
    TRACE("flags %#x\n", swapchain_desc->flags);
    TRACE("refresh_rate %u\n", swapchain_desc->refresh_rate);
    TRACE("auto_restore_display_mode %#x\n", swapchain_desc->auto_restore_display_mode);

    if (swapchain_desc->backbuffer_bind_flags && swapchain_desc->backbuffer_bind_flags != WINED3D_BIND_RENDER_TARGET)
        FIXME("Got unexpected backbuffer bind flags %#x.\n", swapchain_desc->backbuffer_bind_flags);

    if (swapchain_desc->swap_effect != WINED3D_SWAP_EFFECT_DISCARD
            && swapchain_desc->swap_effect != WINED3D_SWAP_EFFECT_SEQUENTIAL
            && swapchain_desc->swap_effect != WINED3D_SWAP_EFFECT_COPY)
        FIXME("Unimplemented swap effect %#x.\n", swapchain_desc->swap_effect);

    /* No special treatment of these parameters. Just store them */
    current_desc->swap_effect = swapchain_desc->swap_effect;
    current_desc->enable_auto_depth_stencil = swapchain_desc->enable_auto_depth_stencil;
    current_desc->auto_depth_stencil_format = swapchain_desc->auto_depth_stencil_format;
    current_desc->refresh_rate = swapchain_desc->refresh_rate;
    current_desc->auto_restore_display_mode = swapchain_desc->auto_restore_display_mode;

    device_window = swapchain_desc->device_window ? swapchain_desc->device_window : device->create_parms.focus_window;
    if (device_window && device_window != current_desc->device_window)
    {
        TRACE("Changing the device window from %p to %p.\n",
                current_desc->device_window, device_window);
        current_desc->device_window = device_window;
        swapchain_state->device_window = device_window;
        wined3d_swapchain_set_window(swapchain, NULL);
    }

    backbuffer_resized = swapchain_desc->backbuffer_width != current_desc->backbuffer_width
            || swapchain_desc->backbuffer_height != current_desc->backbuffer_height;
    windowed = current_desc->windowed;

    if (!swapchain_desc->windowed != !windowed || swapchain->reapply_mode
            || mode || (!swapchain_desc->windowed && backbuffer_resized))
    {
        /* Switch from windowed to fullscreen. */
        if (windowed && !swapchain_desc->windowed)
        {
            HWND focus_window = device->create_parms.focus_window;

            if (!focus_window)
                focus_window = swapchain->state.device_window;
            if (FAILED(hr = wined3d_device_acquire_focus_window(device, focus_window)))
            {
                ERR("Failed to acquire focus window, hr %#lx.\n", hr);
                return hr;
            }
        }

        if (FAILED(hr = wined3d_swapchain_state_set_fullscreen(&swapchain->state,
                swapchain_desc, mode)))
            return hr;

        /* Switch from fullscreen to windowed. */
        if (!windowed && swapchain_desc->windowed)
            wined3d_device_release_focus_window(device);
    }
    else if (!swapchain_desc->windowed)
    {
        DWORD style = swapchain_state->style;
        DWORD exstyle = swapchain_state->exstyle;
        struct wined3d_output_desc output_desc;

        /* If we're in fullscreen, and the mode wasn't changed, we have to get
         * the window back into the right position. Some applications
         * (Battlefield 2, Guild Wars) move it and then call Reset() to clean
         * up their mess. Guild Wars also loses the device during that. */
        if (FAILED(hr = wined3d_output_get_desc(swapchain_desc->output, &output_desc)))
        {
            ERR("Failed to get output description, hr %#lx.\n", hr);
            return hr;
        }

        swapchain_state->style = 0;
        swapchain_state->exstyle = 0;
        wined3d_swapchain_state_setup_fullscreen(swapchain_state, swapchain_state->device_window,
                output_desc.desktop_rect.left, output_desc.desktop_rect.top,
                swapchain_desc->backbuffer_width, swapchain_desc->backbuffer_height);
        swapchain_state->style = style;
        swapchain_state->exstyle = exstyle;
    }

    if (FAILED(hr = wined3d_swapchain_resize_buffers(swapchain, swapchain_desc->backbuffer_count,
            swapchain_desc->backbuffer_width, swapchain_desc->backbuffer_height, swapchain_desc->backbuffer_format,
            swapchain_desc->multisample_type, swapchain_desc->multisample_quality)))
        return hr;

    if (swapchain_desc->flags != current_desc->flags)
    {
        current_desc->flags = swapchain_desc->flags;

        update_swapchain_flags(swapchain->front_buffer);
        for (i = 0; i < current_desc->backbuffer_count; ++i)
        {
            update_swapchain_flags(swapchain->back_buffers[i]);
        }
    }

    if ((view = device->auto_depth_stencil_view))
    {
        device->auto_depth_stencil_view = NULL;
        wined3d_rendertarget_view_decref(view);
    }
    if (current_desc->enable_auto_depth_stencil)
    {
        struct wined3d_resource_desc texture_desc;
        struct wined3d_texture *texture;

        TRACE("Creating the depth stencil buffer.\n");

        texture_desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
        texture_desc.format = current_desc->auto_depth_stencil_format;
        texture_desc.multisample_type = current_desc->multisample_type;
        texture_desc.multisample_quality = current_desc->multisample_quality;
        texture_desc.usage = 0;
        texture_desc.bind_flags = WINED3D_BIND_DEPTH_STENCIL;
        texture_desc.access = WINED3D_RESOURCE_ACCESS_GPU;
        texture_desc.width = current_desc->backbuffer_width;
        texture_desc.height = current_desc->backbuffer_height;
        texture_desc.depth = 1;
        texture_desc.size = 0;

        if (FAILED(hr = wined3d_texture_create(device, &texture_desc, 1, 1, 0,
                NULL, NULL, &wined3d_null_parent_ops, &texture)))
        {
            ERR("Failed to create the auto depth/stencil surface, hr %#lx.\n", hr);
            return WINED3DERR_INVALIDCALL;
        }

        view_desc.format_id = texture->resource.format->id;
        view_desc.flags = 0;
        view_desc.u.texture.level_idx = 0;
        view_desc.u.texture.level_count = 1;
        view_desc.u.texture.layer_idx = 0;
        view_desc.u.texture.layer_count = 1;
        hr = wined3d_rendertarget_view_create(&view_desc, &texture->resource,
                NULL, &wined3d_null_parent_ops, &device->auto_depth_stencil_view);
        wined3d_texture_decref(texture);
        if (FAILED(hr))
        {
            ERR("Failed to create rendertarget view, hr %#lx.\n", hr);
            return hr;
        }
    }

    if ((view = device->back_buffer_view))
    {
        device->back_buffer_view = NULL;
        wined3d_rendertarget_view_decref(view);
    }
    if (current_desc->backbuffer_count && current_desc->backbuffer_bind_flags & WINED3D_BIND_RENDER_TARGET)
    {
        struct wined3d_resource *back_buffer = &swapchain->back_buffers[0]->resource;

        view_desc.format_id = back_buffer->format->id;
        view_desc.flags = 0;
        view_desc.u.texture.level_idx = 0;
        view_desc.u.texture.level_count = 1;
        view_desc.u.texture.layer_idx = 0;
        view_desc.u.texture.layer_count = 1;
        if (FAILED(hr = wined3d_rendertarget_view_create(&view_desc, back_buffer,
                NULL, &wined3d_null_parent_ops, &device->back_buffer_view)))
        {
            ERR("Failed to create rendertarget view, hr %#lx.\n", hr);
            return hr;
        }
    }

    wine_rb_destroy(&device->samplers, device_free_sampler, NULL);
    wine_rb_destroy(&device->rasterizer_states, device_free_rasterizer_state, NULL);
    wine_rb_destroy(&device->blend_states, device_free_blend_state, NULL);
    wine_rb_destroy(&device->depth_stencil_states, device_free_depth_stencil_state, NULL);

    if (reset_state)
    {
        TRACE("Resetting state.\n");
        if (device->inScene)
            wined3d_device_end_scene(device);
        wined3d_device_context_emit_reset_state(&device->cs->c, false);
        state_cleanup(state);

        LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
        {
            TRACE("Unloading resource %p.\n", resource);
            wined3d_cs_emit_unload_resource(device->cs, resource);

            if (resource->usage & WINED3DUSAGE_MANAGED)
                mark_managed_resource_dirty(resource);
        }

        device->adapter->adapter_ops->adapter_uninit_3d(device);

        wined3d_state_reset(state, &device->adapter->d3d_info);

        device_init_swapchain_state(device, swapchain);
        if (wined3d_settings.logo)
            device_load_logo(device, wined3d_settings.logo);

        hr = device->adapter->adapter_ops->adapter_init_3d(device);
    }
    else
    {
        if ((view = device->back_buffer_view))
            wined3d_device_context_set_rendertarget_views(context, 0, 1, &view, FALSE);
        if ((view = device->auto_depth_stencil_view))
            wined3d_device_context_set_depth_stencil_view(context, view);
    }

    /* All done. There is no need to reload resources or shaders, this will
     * happen automatically on the first use. */
    return hr;
}

HRESULT CDECL wined3d_device_set_dialog_box_mode(struct wined3d_device *device, BOOL enable_dialogs)
{
    TRACE("device %p, enable_dialogs %#x.\n", device, enable_dialogs);

    if (!enable_dialogs) FIXME("Dialogs cannot be disabled yet.\n");

    return WINED3D_OK;
}


void CDECL wined3d_device_get_creation_parameters(const struct wined3d_device *device,
        struct wined3d_device_creation_parameters *parameters)
{
    TRACE("device %p, parameters %p.\n", device, parameters);

    *parameters = device->create_parms;
}

struct wined3d * CDECL wined3d_device_get_wined3d(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->wined3d;
}

void CDECL wined3d_device_set_gamma_ramp(const struct wined3d_device *device,
        UINT swapchain_idx, uint32_t flags, const struct wined3d_gamma_ramp *ramp)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, flags %#x, ramp %p.\n",
            device, swapchain_idx, flags, ramp);

    if ((swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        wined3d_swapchain_set_gamma_ramp(swapchain, flags, ramp);
}

void CDECL wined3d_device_get_gamma_ramp(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_gamma_ramp *ramp)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, ramp %p.\n",
            device, swapchain_idx, ramp);

    if ((swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        wined3d_swapchain_get_gamma_ramp(swapchain, ramp);
}

HDC wined3d_device_gl_get_backup_dc(struct wined3d_device_gl *device_gl)
{
    TRACE("device_gl %p.\n", device_gl);

    if (!device_gl->backup_dc)
    {
        TRACE("Creating the backup window for device %p.\n", device_gl);

        if (!(device_gl->backup_wnd = CreateWindowA(WINED3D_OPENGL_WINDOW_CLASS_NAME, "WineD3D fake window",
                WS_OVERLAPPEDWINDOW, 10, 10, 10, 10, NULL, NULL, NULL, NULL)))
        {
            ERR("Failed to create a window.\n");
            return NULL;
        }

        if (!(device_gl->backup_dc = GetDC(device_gl->backup_wnd)))
        {
            ERR("Failed to get a DC.\n");
            DestroyWindow(device_gl->backup_wnd);
            device_gl->backup_wnd = NULL;
            return NULL;
        }
    }

    return device_gl->backup_dc;
}

void device_resource_add(struct wined3d_device *device, struct wined3d_resource *resource)
{
    TRACE("device %p, resource %p.\n", device, resource);

    wined3d_not_from_cs(device->cs);

    list_add_head(&device->resources, &resource->resource_list_entry);
}

static void device_resource_remove(struct wined3d_device *device, struct wined3d_resource *resource)
{
    TRACE("device %p, resource %p.\n", device, resource);

    wined3d_not_from_cs(device->cs);

    list_remove(&resource->resource_list_entry);
}

void device_resource_released(struct wined3d_device *device, struct wined3d_resource *resource)
{
    enum wined3d_resource_type type = resource->type;
    struct wined3d_state *state = device->cs->c.state;
    struct wined3d_rendertarget_view *rtv;
    unsigned int i;

    TRACE("device %p, resource %p, type %s.\n", device, resource, debug_d3dresourcetype(type));

    for (i = 0; i < ARRAY_SIZE(state->fb.render_targets); ++i)
    {
        if ((rtv = state->fb.render_targets[i]) && rtv->resource == resource)
            ERR("Resource %p is still in use as render target %u.\n", resource, i);
    }

    if ((rtv = state->fb.depth_stencil) && rtv->resource == resource)
        ERR("Resource %p is still in use as depth/stencil buffer.\n", resource);

    switch (type)
    {
        case WINED3D_RTYPE_BUFFER:
            for (i = 0; i < WINED3D_MAX_STREAMS; ++i)
            {
                if (&state->streams[i].buffer->resource == resource)
                {
                    ERR("Buffer resource %p is still in use, stream %u.\n", resource, i);
                    state->streams[i].buffer = NULL;
                }
            }

            if (&state->index_buffer->resource == resource)
            {
                ERR("Buffer resource %p is still in use as index buffer.\n", resource);
                state->index_buffer =  NULL;
            }
            break;

        default:
            break;
    }

    /* Remove the resource from the resourceStore */
    device_resource_remove(device, resource);

    TRACE("Resource released.\n");
}

static int wined3d_so_desc_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_stream_output_desc *desc = &WINE_RB_ENTRY_VALUE(entry,
            struct wined3d_so_desc_entry, entry)->desc;
    const struct wined3d_stream_output_desc *k = key;
    unsigned int i;
    int ret;

    if ((ret = wined3d_uint32_compare(k->element_count, desc->element_count)))
        return ret;
    if ((ret = wined3d_uint32_compare(k->buffer_stride_count, desc->buffer_stride_count)))
        return ret;
    if ((ret = wined3d_uint32_compare(k->rasterizer_stream_idx, desc->rasterizer_stream_idx)))
        return ret;

    for (i = 0; i < k->element_count; ++i)
    {
        const struct wined3d_stream_output_element *b = &desc->elements[i];
        const struct wined3d_stream_output_element *a = &k->elements[i];

        if ((ret = wined3d_uint32_compare(a->stream_idx, b->stream_idx)))
            return ret;
        if ((ret = (!a->semantic_name - !b->semantic_name)))
            return ret;
        if (a->semantic_name && (ret = strcmp(a->semantic_name, b->semantic_name)))
            return ret;
        if ((ret = wined3d_uint32_compare(a->semantic_idx, b->semantic_idx)))
            return ret;
        if ((ret = wined3d_uint32_compare(a->component_idx, b->component_idx)))
            return ret;
        if ((ret = wined3d_uint32_compare(a->component_count, b->component_count)))
            return ret;
        if ((ret = wined3d_uint32_compare(a->output_slot, b->output_slot)))
            return ret;
    }

    for (i = 0; i < k->buffer_stride_count; ++i)
    {
        if ((ret = wined3d_uint32_compare(k->buffer_strides[i], desc->buffer_strides[i])))
            return ret;
    }

    return 0;
}

static int wined3d_sampler_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_sampler *sampler = WINE_RB_ENTRY_VALUE(entry, struct wined3d_sampler, entry);

    return memcmp(&sampler->desc, key, sizeof(sampler->desc));
}

static int wined3d_rasterizer_state_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_rasterizer_state *state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_rasterizer_state, entry);

    return memcmp(&state->desc, key, sizeof(state->desc));
}

static int wined3d_blend_state_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_blend_state *state = WINE_RB_ENTRY_VALUE(entry, struct wined3d_blend_state, entry);

    return memcmp(&state->desc, key, sizeof(state->desc));
}

static int wined3d_depth_stencil_state_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_depth_stencil_state *state
            = WINE_RB_ENTRY_VALUE(entry, struct wined3d_depth_stencil_state, entry);

    return memcmp(&state->desc, key, sizeof(state->desc));
}

HRESULT wined3d_device_init(struct wined3d_device *device, struct wined3d *wined3d,
        unsigned int adapter_idx, enum wined3d_device_type device_type, HWND focus_window, unsigned int flags,
        BYTE surface_alignment, const enum wined3d_feature_level *levels, unsigned int level_count,
        const BOOL *supported_extensions, struct wined3d_device_parent *device_parent)
{
    struct wined3d_adapter *adapter = wined3d->adapters[adapter_idx];
    const struct wined3d_fragment_pipe_ops *fragment_pipeline;
    const struct wined3d_vertex_pipe_ops *vertex_pipeline;
    unsigned int i;
    HRESULT hr;

    device->ref = 1;
    device->wined3d = wined3d;
    wined3d_incref(device->wined3d);
    device->adapter = adapter;
    device->device_parent = device_parent;
    list_init(&device->resources);
    list_init(&device->shaders);
    device->surface_alignment = surface_alignment;

    /* Save the creation parameters. */
    device->create_parms.adapter_idx = adapter_idx;
    device->create_parms.device_type = device_type;
    device->create_parms.focus_window = focus_window;
    device->create_parms.flags = flags;

    device->shader_backend = adapter->shader_backend;

    vertex_pipeline = adapter->vertex_pipe;

    fragment_pipeline = adapter->fragment_pipe;

    wine_rb_init(&device->so_descs, wined3d_so_desc_compare);
    wine_rb_init(&device->samplers, wined3d_sampler_compare);
    wine_rb_init(&device->rasterizer_states, wined3d_rasterizer_state_compare);
    wine_rb_init(&device->blend_states, wined3d_blend_state_compare);
    wine_rb_init(&device->depth_stencil_states, wined3d_depth_stencil_state_compare);
    wine_rb_init(&device->ffp_vertex_shaders, wined3d_ffp_vertex_program_key_compare);
    wine_rb_init(&device->ffp_pixel_shaders, wined3d_ffp_frag_program_key_compare);

    if (vertex_pipeline->vp_states && fragment_pipeline->states
            && FAILED(hr = compile_state_table(device->state_table, device->multistate_funcs,
            &adapter->d3d_info, supported_extensions, vertex_pipeline,
            fragment_pipeline, adapter->misc_state_template)))
    {
        ERR("Failed to compile state table, hr %#lx.\n", hr);
        wine_rb_destroy(&device->samplers, NULL, NULL);
        wine_rb_destroy(&device->rasterizer_states, NULL, NULL);
        wine_rb_destroy(&device->blend_states, NULL, NULL);
        wine_rb_destroy(&device->depth_stencil_states, NULL, NULL);
        wine_rb_destroy(&device->so_descs, NULL, NULL);
        wine_rb_destroy(&device->ffp_vertex_shaders, NULL, NULL);
        wine_rb_destroy(&device->ffp_pixel_shaders, NULL, NULL);
        wined3d_decref(device->wined3d);
        return hr;
    }

    device->max_frame_latency = 3;

    if (!(device->cs = wined3d_cs_create(device, levels, level_count)))
    {
        WARN("Failed to create command stream.\n");
        hr = E_FAIL;
        goto err;
    }

    wined3d_lock_init(&device->bo_map_lock, "wined3d_device.bo_map_lock");

    return WINED3D_OK;

err:
    for (i = 0; i < ARRAY_SIZE(device->multistate_funcs); ++i)
    {
        free(device->multistate_funcs[i]);
    }
    wine_rb_destroy(&device->samplers, NULL, NULL);
    wine_rb_destroy(&device->rasterizer_states, NULL, NULL);
    wine_rb_destroy(&device->blend_states, NULL, NULL);
    wine_rb_destroy(&device->depth_stencil_states, NULL, NULL);
    wine_rb_destroy(&device->so_descs, NULL, NULL);
    wine_rb_destroy(&device->ffp_vertex_shaders, NULL, NULL);
    wine_rb_destroy(&device->ffp_pixel_shaders, NULL, NULL);
    wined3d_decref(device->wined3d);
    return hr;
}

void device_invalidate_state(const struct wined3d_device *device, unsigned int state_id)
{
    unsigned int representative, i, idx, shift;
    struct wined3d_context *context;

    wined3d_from_cs(device->cs);

    if (STATE_IS_COMPUTE(state_id))
    {
        for (i = 0; i < device->context_count; ++i)
            context_invalidate_compute_state(device->contexts[i], state_id);
        return;
    }

    representative = device->state_table[state_id].representative;
    idx = representative / (sizeof(*context->dirty_graphics_states) * CHAR_BIT);
    shift = representative & ((sizeof(*context->dirty_graphics_states) * CHAR_BIT) - 1);
    for (i = 0; i < device->context_count; ++i)
    {
        device->contexts[i]->dirty_graphics_states[idx] |= (1u << shift);
    }
}

LRESULT device_process_message(struct wined3d_device *device, HWND window, BOOL unicode,
        UINT message, WPARAM wparam, LPARAM lparam, WNDPROC proc)
{
    if (message == WM_DESTROY)
    {
        TRACE("unregister window %p.\n", window);
        wined3d_unregister_window(window);

        if (InterlockedCompareExchangePointer((void **)&device->focus_window, NULL, window) != window)
            ERR("Window %p is not the focus window for device %p.\n", window, device);
    }
    else if (message == WM_DISPLAYCHANGE)
    {
        device->device_parent->ops->mode_changed(device->device_parent);
    }
    else if (message == WM_ACTIVATEAPP)
    {
        unsigned int i = device->swapchain_count;

        /* Deactivating the implicit swapchain may cause the application
         * (e.g. Deus Ex: GOTY) to destroy the device, so take care to
         * deactivate the implicit swapchain last, and to avoid accessing the
         * "device" pointer afterwards. */
        while (i--)
            wined3d_swapchain_activate(device->swapchains[i], wparam);
    }
    else if (message == WM_SYSCOMMAND)
    {
        if (wparam == SC_RESTORE && device->wined3d->flags & WINED3D_HANDLE_RESTORE)
        {
            if (unicode)
                DefWindowProcW(window, message, wparam, lparam);
            else
                DefWindowProcA(window, message, wparam, lparam);
        }
    }

    if (unicode)
        return CallWindowProcW(proc, window, message, wparam, lparam);
    else
        return CallWindowProcA(proc, window, message, wparam, lparam);
}
