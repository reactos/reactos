/*
 * Copyright 2018 Henri Verbeet for CodeWeavers
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

#include "config.h"
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

static void adapter_vk_destroy(struct wined3d_adapter *adapter)
{
    wined3d_adapter_cleanup(adapter);
    heap_free(adapter);
}

static BOOL adapter_vk_create_context(struct wined3d_context *context,
        struct wined3d_texture *target, const struct wined3d_format *ds_format)
{
    return TRUE;
}

static void adapter_vk_get_wined3d_caps(const struct wined3d_adapter *adapter, struct wined3d_caps *caps)
{
}

static BOOL adapter_vk_check_format(const struct wined3d_adapter *adapter,
        const struct wined3d_format *adapter_format, const struct wined3d_format *rt_format,
        const struct wined3d_format *ds_format)
{
    return TRUE;
}

static const struct wined3d_adapter_ops wined3d_adapter_vk_ops =
{
    adapter_vk_destroy,
    adapter_vk_create_context,
    adapter_vk_get_wined3d_caps,
    adapter_vk_check_format,
};

static BOOL wined3d_adapter_vk_init(struct wined3d_adapter_vk *adapter_vk,
        unsigned int ordinal, unsigned int wined3d_creation_flags)
{
    const struct wined3d_gpu_description *gpu_description;
    struct wined3d_adapter *adapter = &adapter_vk->a;

    TRACE("adapter_vk %p, ordinal %u, wined3d_creation_flags %#x.\n",
            adapter_vk, ordinal, wined3d_creation_flags);

    if (!wined3d_adapter_init(adapter, ordinal))
        return FALSE;

    if (!(gpu_description = wined3d_get_gpu_description(HW_VENDOR_AMD, CARD_AMD_RADEON_RX_VEGA)))
    {
        ERR("Failed to get GPU description.\n");
        return FALSE;
    }
    wined3d_driver_info_init(&adapter->driver_info, gpu_description, wined3d_settings.emulated_textureram);

    if (!wined3d_adapter_vk_init_format_info(adapter))
        return FALSE;

    adapter->vertex_pipe = &none_vertex_pipe;
    adapter->fragment_pipe = &none_fragment_pipe;
    adapter->shader_backend = &none_shader_backend;
    adapter->adapter_ops = &wined3d_adapter_vk_ops;

    adapter->d3d_info.wined3d_creation_flags = wined3d_creation_flags;

    return TRUE;
}

struct wined3d_adapter *wined3d_adapter_vk_create(unsigned int ordinal,
        unsigned int wined3d_creation_flags)
{
    struct wined3d_adapter_vk *adapter_vk;

    if (!(adapter_vk = heap_alloc_zero(sizeof(*adapter_vk))))
        return NULL;

    if (!wined3d_adapter_vk_init(adapter_vk, ordinal, wined3d_creation_flags))
    {
        heap_free(adapter_vk);
        return NULL;
    }

    TRACE("Created adapter %p.\n", adapter_vk);

    return &adapter_vk->a;
}
