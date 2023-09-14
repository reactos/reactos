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

#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

static void adapter_vk_destroy(struct wined3d_adapter *adapter)
{
    struct wined3d_adapter_vk *adapter_vk = wined3d_adapter_vk(adapter);
    const struct wined3d_vk_info *vk_info = &adapter_vk->vk_info;

    VK_CALL(vkDestroyInstance(vk_info->instance, NULL));
    wined3d_adapter_cleanup(&adapter_vk->a);
    heap_free(adapter_vk);
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

static BOOL wined3d_init_vulkan(struct wined3d_vk_info *vk_info)
{
    struct vulkan_ops *vk_ops = &vk_info->vk_ops;
    const struct vulkan_funcs *vk_funcs;
    VkInstanceCreateInfo instance_info;
    VkInstance instance;
    VkResult vr;
    HDC dc;

    dc = GetDC(0);
    vk_funcs = __wine_get_vulkan_driver(dc, WINE_VULKAN_DRIVER_VERSION);
    ReleaseDC(0, dc);

    if (!vk_funcs)
        return FALSE;

    vk_ops->vkGetInstanceProcAddr = (void *)vk_funcs->p_vkGetInstanceProcAddr;

    if (!(vk_ops->vkCreateInstance = (void *)VK_CALL(vkGetInstanceProcAddr(NULL, "vkCreateInstance"))))
    {
        ERR("Could not get 'vkCreateInstance'.\n");
        return FALSE;
    }

    memset(&instance_info, 0, sizeof(instance_info));
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    if ((vr = VK_CALL(vkCreateInstance(&instance_info, NULL, &instance))) < 0)
    {
        WARN("Failed to create Vulkan instance, vr %d.\n", vr);
        return FALSE;
    }

    TRACE("Created Vulkan instance %p.\n", instance);

#define LOAD_INSTANCE_PFN(name) \
    if (!(vk_ops->name = (void *)VK_CALL(vkGetInstanceProcAddr(instance, #name)))) \
    { \
        WARN("Could not get instance proc addr for '" #name "'.\n"); \
        vk_funcs->p_vkDestroyInstance(instance, NULL); \
        return FALSE; \
    }
#define VK_INSTANCE_PFN     LOAD_INSTANCE_PFN
#define VK_DEVICE_PFN       LOAD_INSTANCE_PFN
    VK_INSTANCE_FUNCS()
    VK_DEVICE_FUNCS()
#undef VK_INSTANCE_PFN
#undef VK_DEVICE_PFN

    vk_info->instance = instance;

    return TRUE;
}

static BOOL wined3d_adapter_vk_init(struct wined3d_adapter_vk *adapter_vk,
        unsigned int ordinal, unsigned int wined3d_creation_flags)
{
    struct wined3d_vk_info *vk_info = &adapter_vk->vk_info;
    const struct wined3d_gpu_description *gpu_description;
    struct wined3d_adapter *adapter = &adapter_vk->a;

    TRACE("adapter_vk %p, ordinal %u, wined3d_creation_flags %#x.\n",
            adapter_vk, ordinal, wined3d_creation_flags);

    if (!wined3d_adapter_init(adapter, ordinal))
        return FALSE;

    if (!wined3d_init_vulkan(vk_info))
    {
        WARN("Failed to initialize Vulkan.\n");
        goto fail;
    }

    if (!(gpu_description = wined3d_get_gpu_description(HW_VENDOR_AMD, CARD_AMD_RADEON_RX_VEGA)))
    {
        ERR("Failed to get GPU description.\n");
        goto fail;
    }
    wined3d_driver_info_init(&adapter->driver_info, gpu_description, wined3d_settings.emulated_textureram);

    if (!wined3d_adapter_vk_init_format_info(adapter))
        goto fail;

    adapter->vertex_pipe = &none_vertex_pipe;
    adapter->fragment_pipe = &none_fragment_pipe;
    adapter->shader_backend = &none_shader_backend;
    adapter->adapter_ops = &wined3d_adapter_vk_ops;

    adapter->d3d_info.wined3d_creation_flags = wined3d_creation_flags;

    return TRUE;

fail:
    if (vk_info->vk_ops.vkDestroyInstance)
        VK_CALL(vkDestroyInstance(vk_info->instance, NULL));
    wined3d_adapter_cleanup(adapter);
    return FALSE;
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
