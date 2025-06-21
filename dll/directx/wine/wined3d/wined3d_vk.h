/*
 * Copyright 2018 Józef Kucia for CodeWeavers
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

#ifndef __WINE_WINED3D_VK_H
#define __WINE_WINED3D_VK_H

#include "stdint.h"
#include "wine/list.h"
#include "wine/wined3d.h"
#define VK_NO_PROTOTYPES
#include "wine/vulkan.h"
#include "wined3d_private.h"

struct wined3d_buffer_vk;
struct wined3d_context_vk;
struct wined3d_device_vk;

#define VK_INSTANCE_FUNCS() \
    VK_INSTANCE_PFN(vkCreateDevice) \
    VK_INSTANCE_PFN(vkDestroyInstance) \
    VK_INSTANCE_PFN(vkEnumerateDeviceExtensionProperties) \
    VK_INSTANCE_PFN(vkEnumerateDeviceLayerProperties) \
    VK_INSTANCE_PFN(vkEnumeratePhysicalDevices) \
    VK_INSTANCE_PFN(vkGetDeviceProcAddr) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceFeatures) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceFormatProperties) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceImageFormatProperties) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceMemoryProperties) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceProperties) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceQueueFamilyProperties) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceSparseImageFormatProperties) \
    /* Vulkan 1.1 */ \
    VK_INSTANCE_EXT_PFN(vkGetPhysicalDeviceFeatures2) \
    VK_INSTANCE_EXT_PFN(vkGetPhysicalDeviceProperties2) \
    /* VK_KHR_surface */ \
    VK_INSTANCE_PFN(vkDestroySurfaceKHR) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceSurfaceFormatsKHR) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    VK_INSTANCE_PFN(vkGetPhysicalDeviceSurfaceSupportKHR) \
    /* VK_KHR_win32_surface */ \
    VK_INSTANCE_PFN(vkCreateWin32SurfaceKHR) \
    /* VK_EXT_host_query_reset */ \
    VK_INSTANCE_EXT_PFN(vkResetQueryPoolEXT)

#define VK_DEVICE_FUNCS() \
    VK_DEVICE_PFN(vkAllocateCommandBuffers) \
    VK_DEVICE_PFN(vkAllocateDescriptorSets) \
    VK_DEVICE_PFN(vkAllocateMemory) \
    VK_DEVICE_PFN(vkBeginCommandBuffer) \
    VK_DEVICE_PFN(vkBindBufferMemory) \
    VK_DEVICE_PFN(vkBindImageMemory) \
    VK_DEVICE_PFN(vkCmdBeginQuery) \
    VK_DEVICE_PFN(vkCmdBeginRenderPass) \
    VK_DEVICE_PFN(vkCmdBindDescriptorSets) \
    VK_DEVICE_PFN(vkCmdBindIndexBuffer) \
    VK_DEVICE_PFN(vkCmdBindPipeline) \
    VK_DEVICE_PFN(vkCmdBindVertexBuffers) \
    VK_DEVICE_PFN(vkCmdBlitImage) \
    VK_DEVICE_PFN(vkCmdClearAttachments) \
    VK_DEVICE_PFN(vkCmdClearColorImage) \
    VK_DEVICE_PFN(vkCmdClearDepthStencilImage) \
    VK_DEVICE_PFN(vkCmdCopyBuffer) \
    VK_DEVICE_PFN(vkCmdCopyBufferToImage) \
    VK_DEVICE_PFN(vkCmdCopyImage) \
    VK_DEVICE_PFN(vkCmdCopyImageToBuffer) \
    VK_DEVICE_PFN(vkCmdCopyQueryPoolResults) \
    VK_DEVICE_PFN(vkCmdDispatch) \
    VK_DEVICE_PFN(vkCmdDispatchIndirect) \
    VK_DEVICE_PFN(vkCmdDraw) \
    VK_DEVICE_PFN(vkCmdDrawIndexed) \
    VK_DEVICE_PFN(vkCmdDrawIndexedIndirect) \
    VK_DEVICE_PFN(vkCmdDrawIndirect) \
    VK_DEVICE_PFN(vkCmdEndQuery) \
    VK_DEVICE_PFN(vkCmdEndRenderPass) \
    VK_DEVICE_PFN(vkCmdExecuteCommands) \
    VK_DEVICE_PFN(vkCmdFillBuffer) \
    VK_DEVICE_PFN(vkCmdNextSubpass) \
    VK_DEVICE_PFN(vkCmdPipelineBarrier) \
    VK_DEVICE_PFN(vkCmdPushConstants) \
    VK_DEVICE_PFN(vkCmdResetEvent) \
    VK_DEVICE_PFN(vkCmdResetQueryPool) \
    VK_DEVICE_PFN(vkCmdResolveImage) \
    VK_DEVICE_PFN(vkCmdSetBlendConstants) \
    VK_DEVICE_PFN(vkCmdSetDepthBias) \
    VK_DEVICE_PFN(vkCmdSetDepthBounds) \
    VK_DEVICE_PFN(vkCmdSetEvent) \
    VK_DEVICE_PFN(vkCmdSetLineWidth) \
    VK_DEVICE_PFN(vkCmdSetScissor) \
    VK_DEVICE_PFN(vkCmdSetStencilCompareMask) \
    VK_DEVICE_PFN(vkCmdSetStencilReference) \
    VK_DEVICE_PFN(vkCmdSetStencilWriteMask) \
    VK_DEVICE_PFN(vkCmdSetViewport) \
    VK_DEVICE_PFN(vkCmdUpdateBuffer) \
    VK_DEVICE_PFN(vkCmdWaitEvents) \
    VK_DEVICE_PFN(vkCmdWriteTimestamp) \
    VK_DEVICE_PFN(vkCreateBuffer) \
    VK_DEVICE_PFN(vkCreateBufferView) \
    VK_DEVICE_PFN(vkCreateCommandPool) \
    VK_DEVICE_PFN(vkCreateComputePipelines) \
    VK_DEVICE_PFN(vkCreateDescriptorPool) \
    VK_DEVICE_PFN(vkCreateDescriptorSetLayout) \
    VK_DEVICE_PFN(vkCreateEvent) \
    VK_DEVICE_PFN(vkCreateFence) \
    VK_DEVICE_PFN(vkCreateFramebuffer) \
    VK_DEVICE_PFN(vkCreateGraphicsPipelines) \
    VK_DEVICE_PFN(vkCreateImage) \
    VK_DEVICE_PFN(vkCreateImageView) \
    VK_DEVICE_PFN(vkCreatePipelineCache) \
    VK_DEVICE_PFN(vkCreatePipelineLayout) \
    VK_DEVICE_PFN(vkCreateQueryPool) \
    VK_DEVICE_PFN(vkCreateRenderPass) \
    VK_DEVICE_PFN(vkCreateSampler) \
    VK_DEVICE_PFN(vkCreateSemaphore) \
    VK_DEVICE_PFN(vkCreateShaderModule) \
    VK_DEVICE_PFN(vkDestroyBuffer) \
    VK_DEVICE_PFN(vkDestroyBufferView) \
    VK_DEVICE_PFN(vkDestroyCommandPool) \
    VK_DEVICE_PFN(vkDestroyDescriptorPool) \
    VK_DEVICE_PFN(vkDestroyDescriptorSetLayout) \
    VK_DEVICE_PFN(vkDestroyDevice) \
    VK_DEVICE_PFN(vkDestroyEvent) \
    VK_DEVICE_PFN(vkDestroyFence) \
    VK_DEVICE_PFN(vkDestroyFramebuffer) \
    VK_DEVICE_PFN(vkDestroyImage) \
    VK_DEVICE_PFN(vkDestroyImageView) \
    VK_DEVICE_PFN(vkDestroyPipeline) \
    VK_DEVICE_PFN(vkDestroyPipelineCache) \
    VK_DEVICE_PFN(vkDestroyPipelineLayout) \
    VK_DEVICE_PFN(vkDestroyQueryPool) \
    VK_DEVICE_PFN(vkDestroyRenderPass) \
    VK_DEVICE_PFN(vkDestroySampler) \
    VK_DEVICE_PFN(vkDestroySemaphore) \
    VK_DEVICE_PFN(vkDestroyShaderModule) \
    VK_DEVICE_PFN(vkDeviceWaitIdle) \
    VK_DEVICE_PFN(vkEndCommandBuffer) \
    VK_DEVICE_PFN(vkFlushMappedMemoryRanges) \
    VK_DEVICE_PFN(vkFreeCommandBuffers) \
    VK_DEVICE_PFN(vkFreeDescriptorSets) \
    VK_DEVICE_PFN(vkFreeMemory) \
    VK_DEVICE_PFN(vkGetBufferMemoryRequirements) \
    VK_DEVICE_PFN(vkGetDeviceMemoryCommitment) \
    VK_DEVICE_PFN(vkGetDeviceQueue) \
    VK_DEVICE_PFN(vkGetEventStatus) \
    VK_DEVICE_PFN(vkGetFenceStatus) \
    VK_DEVICE_PFN(vkGetImageMemoryRequirements) \
    VK_DEVICE_PFN(vkGetImageSparseMemoryRequirements) \
    VK_DEVICE_PFN(vkGetImageSubresourceLayout) \
    VK_DEVICE_PFN(vkGetPipelineCacheData) \
    VK_DEVICE_PFN(vkGetQueryPoolResults) \
    VK_DEVICE_PFN(vkGetRenderAreaGranularity) \
    VK_DEVICE_PFN(vkInvalidateMappedMemoryRanges) \
    VK_DEVICE_PFN(vkMapMemory) \
    VK_DEVICE_PFN(vkMergePipelineCaches) \
    VK_DEVICE_PFN(vkQueueBindSparse) \
    VK_DEVICE_PFN(vkQueueSubmit) \
    VK_DEVICE_PFN(vkQueueWaitIdle) \
    VK_DEVICE_PFN(vkResetCommandBuffer) \
    VK_DEVICE_PFN(vkResetCommandPool) \
    VK_DEVICE_PFN(vkResetDescriptorPool) \
    VK_DEVICE_PFN(vkResetEvent) \
    VK_DEVICE_PFN(vkResetFences) \
    VK_DEVICE_PFN(vkSetEvent) \
    VK_DEVICE_PFN(vkUnmapMemory) \
    VK_DEVICE_PFN(vkUpdateDescriptorSets) \
    VK_DEVICE_PFN(vkWaitForFences) \
    /* VK_EXT_extended_dynamic_state */ \
    VK_DEVICE_EXT_PFN(vkCmdSetDepthCompareOpEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetDepthTestEnableEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetDepthWriteEnableEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetPrimitiveTopologyEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetStencilOpEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetStencilTestEnableEXT) \
    /* VK_EXT_extended_dynamic_state2 */ \
    VK_DEVICE_EXT_PFN(vkCmdSetPatchControlPointsEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetPrimitiveRestartEnableEXT) \
    /* VK_EXT_extended_dynamic_state3 */ \
    VK_DEVICE_EXT_PFN(vkCmdSetAlphaToCoverageEnableEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetColorBlendEnableEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetColorBlendEquationEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetColorWriteMaskEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetCullModeEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetDepthBiasEnableEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetDepthClampEnableEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetFrontFaceEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetRasterizationSamplesEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetRasterizerDiscardEnableEXT) \
    VK_DEVICE_EXT_PFN(vkCmdSetSampleMaskEXT) \
    /* VK_EXT_transform_feedback */ \
    VK_DEVICE_EXT_PFN(vkCmdBeginQueryIndexedEXT) \
    VK_DEVICE_EXT_PFN(vkCmdBeginTransformFeedbackEXT) \
    VK_DEVICE_EXT_PFN(vkCmdBindTransformFeedbackBuffersEXT) \
    VK_DEVICE_EXT_PFN(vkCmdEndQueryIndexedEXT) \
    VK_DEVICE_EXT_PFN(vkCmdEndTransformFeedbackEXT) \
    /* VK_KHR_swapchain */ \
    VK_DEVICE_PFN(vkAcquireNextImageKHR) \
    VK_DEVICE_PFN(vkCreateSwapchainKHR) \
    VK_DEVICE_PFN(vkDestroySwapchainKHR) \
    VK_DEVICE_PFN(vkGetSwapchainImagesKHR) \
    VK_DEVICE_PFN(vkQueuePresentKHR)

#define DECLARE_VK_PFN(name) PFN_##name name;

struct vulkan_ops
{
#define VK_INSTANCE_PFN     DECLARE_VK_PFN
#define VK_INSTANCE_EXT_PFN DECLARE_VK_PFN
#define VK_DEVICE_PFN       DECLARE_VK_PFN
#define VK_DEVICE_EXT_PFN   DECLARE_VK_PFN
    VK_DEVICE_FUNCS()
    VK_INSTANCE_FUNCS()
#undef VK_INSTANCE_PFN
#undef VK_INSTANCE_EXT_PFN
#undef VK_DEVICE_PFN
#undef VK_DEVICE_EXT_PFN

    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
};

enum wined3d_vk_extension
{
    WINED3D_VK_EXT_NONE,

    WINED3D_VK_EXT_EXTENDED_DYNAMIC_STATE,
    WINED3D_VK_EXT_EXTENDED_DYNAMIC_STATE2,
    WINED3D_VK_EXT_EXTENDED_DYNAMIC_STATE3,
    WINED3D_VK_EXT_HOST_QUERY_RESET,
    WINED3D_VK_EXT_SHADER_STENCIL_EXPORT,
    WINED3D_VK_EXT_TRANSFORM_FEEDBACK,
    WINED3D_VK_EXT_VERTEX_ATTRIBUTE_DIVISOR,
    WINED3D_VK_KHR_MAINTENANCE2,
    WINED3D_VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE,
    WINED3D_VK_KHR_SHADER_DRAW_PARAMETERS,

    WINED3D_VK_EXT_COUNT,
};

struct wined3d_vk_info
{
    struct vulkan_ops vk_ops;

    VkInstance instance;
    unsigned int api_version;

    BOOL supported[WINED3D_VK_EXT_COUNT];
    HMODULE vulkan_lib;

    bool multiple_viewports;
    bool dynamic_state2;
    bool dynamic_patch_vertex_count;
    bool dynamic_multisample_state;
    bool dynamic_blend_state;
    bool dynamic_rasterizer_state;
};

#define VK_CALL(f) (vk_info->vk_ops.f)

static const VkAccessFlags WINED3D_READ_ONLY_ACCESS_FLAGS = VK_ACCESS_INDIRECT_COMMAND_READ_BIT
        | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT
        | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_READ_BIT
        | VK_ACCESS_MEMORY_READ_BIT;

VkAccessFlags vk_access_mask_from_bind_flags(uint32_t bind_flags);
VkCompareOp vk_compare_op_from_wined3d(enum wined3d_cmp_func op);
VkImageViewType vk_image_view_type_from_wined3d(enum wined3d_resource_type type, uint32_t flags);
VkPipelineStageFlags vk_pipeline_stage_mask_from_bind_flags(uint32_t bind_flags);
VkShaderStageFlagBits vk_shader_stage_from_wined3d(enum wined3d_shader_type shader_type);
VkAccessFlags vk_access_mask_from_buffer_usage(VkBufferUsageFlags usage);
VkPipelineStageFlags vk_pipeline_stage_mask_from_buffer_usage(VkBufferUsageFlags usage);
VkBufferUsageFlags vk_buffer_usage_from_bind_flags(uint32_t bind_flags);
VkMemoryPropertyFlags vk_memory_type_from_access_flags(uint32_t access, uint32_t usage);
void wined3d_format_colour_to_vk(const struct wined3d_format *format, const struct wined3d_color *c,
        VkClearColorValue *retval);
void wined3d_vk_swizzle_from_color_fixup(VkComponentMapping *mapping, struct color_fixup_desc fixup);
const char *wined3d_debug_vkresult(VkResult vr);

static inline VkImageAspectFlags vk_aspect_mask_from_format(const struct wined3d_format *format)
{
    VkImageAspectFlags mask = 0;

    if (format->depth_size)
        mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (format->stencil_size)
        mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    if (!mask || format->red_size || format->green_size || format->blue_size || format->alpha_size)
        mask |= VK_IMAGE_ASPECT_COLOR_BIT;

    return mask;
}

struct wined3d_bo_vk
{
    struct wined3d_bo b;

    VkBuffer vk_buffer;
    struct wined3d_allocator_block *memory;
    struct wined3d_bo_slab_vk *slab;

    VkDeviceMemory vk_memory;

    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory_type;

    uint64_t command_buffer_id;
    bool host_synced;
};

static inline struct wined3d_bo_vk *wined3d_bo_vk(struct wined3d_bo *bo)
{
    return CONTAINING_RECORD(bo, struct wined3d_bo_vk, b);
}

struct wined3d_bo_slab_vk_key
{
    VkMemoryPropertyFlags memory_type;
    VkBufferUsageFlags usage;
    VkDeviceSize size;
};

struct wined3d_bo_slab_vk
{
    struct wine_rb_entry entry;
    struct wined3d_bo_slab_vk *next;
    VkMemoryPropertyFlags requested_memory_type;
    struct wined3d_bo_vk bo;
    unsigned int map_count;
    void *map_ptr;
    uint32_t map;
};

void *wined3d_bo_slab_vk_map(struct wined3d_bo_slab_vk *slab_vk,
        struct wined3d_context_vk *context_vk);
void wined3d_bo_slab_vk_unmap(struct wined3d_bo_slab_vk *slab_vk,
        struct wined3d_context_vk *context_vk);

struct wined3d_image_vk
{
    VkImage vk_image;
    struct wined3d_allocator_block *memory;
    VkDeviceMemory vk_memory;
    uint64_t command_buffer_id;
};

struct wined3d_query_pool_vk
{
    struct list entry;
    struct list completed_entry;

    struct list *free_list;
    VkQueryPool vk_query_pool;
    VkEvent vk_event;

    uint32_t allocated[WINED3D_BITMAP_SIZE(WINED3D_QUERY_POOL_SIZE)];
    uint32_t completed[WINED3D_BITMAP_SIZE(WINED3D_QUERY_POOL_SIZE)];
};

bool wined3d_query_pool_vk_allocate_query(struct wined3d_query_pool_vk *pool_vk, size_t *idx);
void wined3d_query_pool_vk_mark_free(struct wined3d_context_vk *context_vk, struct wined3d_query_pool_vk *pool_vk,
        uint32_t start, uint32_t count);
void wined3d_query_pool_vk_cleanup(struct wined3d_query_pool_vk *pool_vk,
        struct wined3d_context_vk *context_vk);
bool wined3d_query_pool_vk_init(struct wined3d_query_pool_vk *pool_vk, struct wined3d_context_vk *context_vk,
        enum wined3d_query_type type, struct list *free_pools);

struct wined3d_query_pool_idx_vk
{
    struct wined3d_query_pool_vk *pool_vk;
    size_t idx;
};

#define WINED3D_QUERY_VK_FLAG_ACTIVE       0x00000001
#define WINED3D_QUERY_VK_FLAG_STARTED      0x00000002
#define WINED3D_QUERY_VK_FLAG_RENDER_PASS  0x00000004

struct wined3d_query_vk
{
    struct wined3d_query q;

    struct list entry;
    struct wined3d_query_pool_idx_vk pool_idx;
    uint8_t flags;
    uint64_t command_buffer_id;
    uint32_t control_flags;
    VkEvent vk_event;
    SIZE_T pending_count, pending_size;
    struct wined3d_query_pool_idx_vk *pending;
};

static inline struct wined3d_query_vk *wined3d_query_vk(struct wined3d_query *query)
{
    return CONTAINING_RECORD(query, struct wined3d_query_vk, q);
}

bool wined3d_query_vk_accumulate_data(struct wined3d_query_vk *query_vk, struct wined3d_device_vk *device_vk,
        const struct wined3d_query_pool_idx_vk *pool_idx);
HRESULT wined3d_query_vk_create(struct wined3d_device *device, enum wined3d_query_type type, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_query **query);
void wined3d_query_vk_resume(struct wined3d_query_vk *query_vk, struct wined3d_context_vk *context_vk);
void wined3d_query_vk_suspend(struct wined3d_query_vk *query_vk, struct wined3d_context_vk *context_vk);

struct wined3d_command_buffer_vk
{
    uint64_t id;
    VkCommandBuffer vk_command_buffer;
    VkFence vk_fence;
};

enum wined3d_retired_object_type_vk
{
    WINED3D_RETIRED_FREE_VK,
    WINED3D_RETIRED_FRAMEBUFFER_VK,
    WINED3D_RETIRED_DESCRIPTOR_POOL_VK,
    WINED3D_RETIRED_MEMORY_VK,
    WINED3D_RETIRED_ALLOCATOR_BLOCK_VK,
    WINED3D_RETIRED_BO_SLAB_SLICE_VK,
    WINED3D_RETIRED_BUFFER_VK,
    WINED3D_RETIRED_IMAGE_VK,
    WINED3D_RETIRED_BUFFER_VIEW_VK,
    WINED3D_RETIRED_IMAGE_VIEW_VK,
    WINED3D_RETIRED_SAMPLER_VK,
    WINED3D_RETIRED_QUERY_POOL_VK,
    WINED3D_RETIRED_EVENT_VK,
    WINED3D_RETIRED_PIPELINE_VK,
};

struct wined3d_retired_object_vk
{
    enum wined3d_retired_object_type_vk type;
    union
    {
        struct wined3d_retired_object_vk *next;
        VkFramebuffer vk_framebuffer;
        VkDescriptorPool vk_descriptor_pool;
        VkDeviceMemory vk_memory;
        struct wined3d_allocator_block *block;
        struct
        {
            struct wined3d_bo_slab_vk *slab;
            size_t idx;
        } slice;
        VkBuffer vk_buffer;
        VkImage vk_image;
        VkBufferView vk_buffer_view;
        VkImageView vk_image_view;
        VkSampler vk_sampler;
        VkEvent vk_event;
        VkPipeline vk_pipeline;
        struct
        {
            struct wined3d_query_pool_vk *pool_vk;
            uint32_t start;
            uint32_t count;
        } queries;
    } u;
    uint64_t command_buffer_id;
};

struct wined3d_retired_objects_vk
{
    struct wined3d_retired_object_vk *objects;
    struct wined3d_retired_object_vk *free;
    SIZE_T size;
    SIZE_T count;
};

#define WINED3D_FB_ATTACHMENT_FLAG_DISCARDED   1
#define WINED3D_FB_ATTACHMENT_FLAG_CLEAR_C     2
#define WINED3D_FB_ATTACHMENT_FLAG_CLEAR_S     4
#define WINED3D_FB_ATTACHMENT_FLAG_CLEAR_Z     8

struct wined3d_render_pass_attachment_vk
{
    VkFormat vk_format;
    VkSampleCountFlagBits vk_samples;
    VkImageLayout vk_layout;
    uint32_t flags;
};

struct wined3d_render_pass_key_vk
{
    struct wined3d_render_pass_attachment_vk rt[WINED3D_MAX_RENDER_TARGETS];
    struct wined3d_render_pass_attachment_vk ds;
    uint32_t rt_mask;
};

struct wined3d_render_pass_vk
{
    struct wine_rb_entry entry;
    struct wined3d_render_pass_key_vk key;
    VkRenderPass vk_render_pass;
};

struct wined3d_pipeline_layout_key_vk
{
    VkDescriptorSetLayoutBinding *bindings;
    SIZE_T binding_count;
};

struct wined3d_pipeline_layout_vk
{
    struct wine_rb_entry entry;
    struct wined3d_pipeline_layout_key_vk key;
    VkPipelineLayout vk_pipeline_layout;
    VkDescriptorSetLayout vk_set_layout;
};

struct wined3d_graphics_pipeline_key_vk
{
    VkPipelineShaderStageCreateInfo stages[WINED3D_SHADER_TYPE_GRAPHICS_COUNT];
    VkVertexInputBindingDivisorDescriptionEXT divisors[MAX_ATTRIBS];
    VkVertexInputAttributeDescription attributes[MAX_ATTRIBS];
    VkVertexInputBindingDescription bindings[MAX_ATTRIBS];
    VkSampleMask sample_mask;
    VkPipelineColorBlendAttachmentState blend_attachments[WINED3D_MAX_RENDER_TARGETS];

    VkPipelineVertexInputDivisorStateCreateInfoEXT divisor_desc;
    VkPipelineVertexInputStateCreateInfo input_desc;
    VkPipelineInputAssemblyStateCreateInfo ia_desc;
    VkPipelineTessellationStateCreateInfo ts_desc;
    VkPipelineViewportStateCreateInfo vp_desc;
    VkPipelineRasterizationStateCreateInfo rs_desc;
    VkPipelineMultisampleStateCreateInfo ms_desc;
    VkPipelineDepthStencilStateCreateInfo ds_desc;
    VkPipelineColorBlendStateCreateInfo blend_desc;
    VkPipelineDynamicStateCreateInfo dynamic_desc;

    VkGraphicsPipelineCreateInfo pipeline_desc;
};

struct wined3d_graphics_pipeline_vk
{
    struct wine_rb_entry entry;
    struct wined3d_graphics_pipeline_key_vk key;
    VkPipeline vk_pipeline;
};

enum wined3d_shader_descriptor_type
{
    WINED3D_SHADER_DESCRIPTOR_TYPE_CBV,
    WINED3D_SHADER_DESCRIPTOR_TYPE_SRV,
    WINED3D_SHADER_DESCRIPTOR_TYPE_UAV,
    WINED3D_SHADER_DESCRIPTOR_TYPE_UAV_COUNTER,
    WINED3D_SHADER_DESCRIPTOR_TYPE_SAMPLER,
};

struct wined3d_shader_resource_binding
{
    enum wined3d_shader_type shader_type;
    enum wined3d_shader_descriptor_type shader_descriptor_type;
    size_t resource_idx;
    enum wined3d_shader_resource_type resource_type;
    enum wined3d_data_type resource_data_type;
    size_t binding_idx;
};

struct wined3d_shader_resource_bindings
{
    struct wined3d_shader_resource_binding *bindings;
    SIZE_T size, count;
};

struct wined3d_shader_descriptor_writes_vk
{
    VkWriteDescriptorSet *writes;
    SIZE_T size, count;
};

struct wined3d_context_vk
{
    struct wined3d_context c;

    const struct wined3d_vk_info *vk_info;

    VkDynamicState dynamic_states[27];

    uint32_t update_compute_pipeline : 1;
    uint32_t update_stream_output : 1;
    uint32_t padding : 30;

    struct
    {
        VkShaderModule vk_modules[WINED3D_SHADER_TYPE_GRAPHICS_COUNT];
        struct wined3d_graphics_pipeline_key_vk pipeline_key_vk;
        VkPipeline vk_pipeline;
        VkPipelineLayout vk_pipeline_layout;
        VkDescriptorSetLayout vk_set_layout;
        struct wined3d_shader_resource_bindings bindings;
    } graphics;

    struct
    {
        VkPipeline vk_pipeline;
        VkPipelineLayout vk_pipeline_layout;
        VkDescriptorSetLayout vk_set_layout;
        struct wined3d_shader_resource_bindings bindings;
    } compute;

    VkCommandPool vk_command_pool;
    struct wined3d_command_buffer_vk current_command_buffer;
    uint64_t completed_command_buffer_id;
    VkDeviceSize retired_bo_size;
    /* Number of draw or dispatch calls that have been recorded into the
     * current command buffer. */
    unsigned int command_buffer_work_count;

    struct
    {
        struct wined3d_command_buffer_vk *buffers;
        SIZE_T buffers_size;
        SIZE_T buffer_count;
    } submitted, completed;

    struct wined3d_shader_descriptor_writes_vk descriptor_writes;

    VkFramebuffer vk_framebuffer;
    VkRenderPass vk_render_pass;

    SIZE_T vk_descriptor_pools_size;
    SIZE_T vk_descriptor_pool_count;
    VkDescriptorPool *vk_descriptor_pools;

    VkSampleCountFlagBits sample_count;
    unsigned int rt_count;

    VkBuffer vk_so_counters[WINED3D_MAX_STREAM_OUTPUT_BUFFERS];
    VkDeviceSize vk_so_offsets[WINED3D_MAX_STREAM_OUTPUT_BUFFERS];
    struct wined3d_bo_vk vk_so_counter_bo;

    struct list render_pass_queries;
    struct list active_queries;
    struct list completed_query_pools;
    struct list free_occlusion_query_pools;
    struct list free_timestamp_query_pools;
    struct list free_pipeline_statistics_query_pools;
    struct list free_stream_output_statistics_query_pools;

    struct wined3d_retired_objects_vk retired;
    struct wine_rb_tree render_passes;
    struct wine_rb_tree pipeline_layouts;
    struct wine_rb_tree graphics_pipelines;
    struct wine_rb_tree bo_slab_available;
};

static inline struct wined3d_context_vk *wined3d_context_vk(struct wined3d_context *context)
{
    return CONTAINING_RECORD(context, struct wined3d_context_vk, c);
}

bool wined3d_context_vk_allocate_query(struct wined3d_context_vk *context_vk,
        enum wined3d_query_type type, struct wined3d_query_pool_idx_vk *pool_idx);
VkDeviceMemory wined3d_context_vk_allocate_vram_chunk_memory(struct wined3d_context_vk *context_vk,
        unsigned int pool, size_t size);
VkCommandBuffer wined3d_context_vk_apply_compute_state(struct wined3d_context_vk *context_vk,
        const struct wined3d_state *state, struct wined3d_buffer_vk *indirect_vk);
VkCommandBuffer wined3d_context_vk_apply_draw_state(struct wined3d_context_vk *context_vk,
        const struct wined3d_state *state, struct wined3d_buffer_vk *indirect_vk, bool indexed);
void wined3d_context_vk_cleanup(struct wined3d_context_vk *context_vk);
BOOL wined3d_context_vk_create_bo(struct wined3d_context_vk *context_vk, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_type, struct wined3d_bo_vk *bo);
BOOL wined3d_context_vk_create_image(struct wined3d_context_vk *context_vk, VkImageType vk_image_type,
        VkImageUsageFlags usage, VkFormat vk_format, unsigned int width, unsigned int height, unsigned int depth,
        unsigned int sample_count, unsigned int mip_levels, unsigned int layer_count, unsigned int flags,
        struct wined3d_image_vk *image);
void wined3d_context_vk_destroy_allocator_block(struct wined3d_context_vk *context_vk,
        struct wined3d_allocator_block *block, uint64_t command_buffer_id);
void wined3d_context_vk_destroy_bo(struct wined3d_context_vk *context_vk,
        const struct wined3d_bo_vk *bo);
void wined3d_context_vk_destroy_image(struct wined3d_context_vk *context_vk,
        struct wined3d_image_vk *image_vk);
void wined3d_context_vk_destroy_vk_buffer_view(struct wined3d_context_vk *context_vk,
        VkBufferView vk_view, uint64_t command_buffer_id);
void wined3d_context_vk_destroy_vk_framebuffer(struct wined3d_context_vk *context_vk,
        VkFramebuffer vk_framebuffer, uint64_t command_buffer_id);
void wined3d_context_vk_destroy_vk_image(struct wined3d_context_vk *context_vk,
        VkImage vk_image, uint64_t command_buffer_id);
void wined3d_context_vk_destroy_vk_image_view(struct wined3d_context_vk *context_vk,
        VkImageView vk_view, uint64_t command_buffer_id);
void wined3d_context_vk_destroy_vk_memory(struct wined3d_context_vk *context_vk,
        VkDeviceMemory vk_memory, uint64_t command_buffer_id);
void wined3d_context_vk_destroy_vk_sampler(struct wined3d_context_vk *context_vk,
        VkSampler vk_sampler, uint64_t command_buffer_id);
void wined3d_context_vk_destroy_vk_event(struct wined3d_context_vk *context_vk,
        VkEvent vk_event, uint64_t command_buffer_id);
void wined3d_context_vk_destroy_vk_pipeline(struct wined3d_context_vk *context_vk,
        VkPipeline vk_pipeline, uint64_t command_buffer_id);
void wined3d_context_vk_end_current_render_pass(struct wined3d_context_vk *context_vk);
VkCommandBuffer wined3d_context_vk_get_command_buffer(struct wined3d_context_vk *context_vk);
struct wined3d_pipeline_layout_vk *wined3d_context_vk_get_pipeline_layout(struct wined3d_context_vk *context_vk,
        VkDescriptorSetLayoutBinding *bindings, SIZE_T binding_count);
VkRenderPass wined3d_context_vk_get_render_pass(struct wined3d_context_vk *context_vk,
        const struct wined3d_fb_state *fb, unsigned int rt_count,
        bool depth_stencil, uint32_t clear_flags);
void wined3d_context_vk_image_barrier(struct wined3d_context_vk *context_vk,
        VkCommandBuffer vk_command_buffer, VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
        VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkImageLayout old_layout,
        VkImageLayout new_layout, VkImage image, const VkImageSubresourceRange *range);
HRESULT wined3d_context_vk_init(struct wined3d_context_vk *context_vk,
        struct wined3d_swapchain *swapchain);
void wined3d_context_vk_submit_command_buffer(struct wined3d_context_vk *context_vk,
        unsigned int wait_semaphore_count, const VkSemaphore *wait_semaphores, const VkPipelineStageFlags *wait_stages,
        unsigned int signal_semaphore_count, const VkSemaphore *signal_semaphores);
void wined3d_context_vk_wait_command_buffer(struct wined3d_context_vk *context_vk, uint64_t id);
VkDescriptorSet wined3d_context_vk_create_vk_descriptor_set(struct wined3d_context_vk *context_vk,
        VkDescriptorSetLayout vk_set_layout);

struct wined3d_adapter_vk
{
    struct wined3d_adapter a;

    struct wined3d_vk_info vk_info;
    unsigned int device_extension_count;
    const char **device_extensions;
    VkPhysicalDevice physical_device;

    VkPhysicalDeviceLimits device_limits;
    VkPhysicalDeviceMemoryProperties memory_properties;
};

static inline struct wined3d_adapter_vk *wined3d_adapter_vk(struct wined3d_adapter *adapter)
{
    return CONTAINING_RECORD(adapter, struct wined3d_adapter_vk, a);
}

void adapter_vk_copy_bo_address(struct wined3d_context *context, const struct wined3d_bo_address *dst,
        const struct wined3d_bo_address *src,
        unsigned int range_count, const struct wined3d_range *ranges, uint32_t map_flags);
unsigned int wined3d_adapter_vk_get_memory_type_index(const struct wined3d_adapter_vk *adapter_vk,
        uint32_t memory_type_mask, VkMemoryPropertyFlags flags);
BOOL wined3d_adapter_vk_init_format_info(struct wined3d_adapter_vk *adapter_vk,
        const struct wined3d_vk_info *vk_info);

struct wined3d_null_resources_vk
{
    struct wined3d_bo_vk bo;
    VkDescriptorBufferInfo buffer_info;

    struct wined3d_image_vk image_1d;
    struct wined3d_image_vk image_2d;
    struct wined3d_image_vk image_2dms;
    struct wined3d_image_vk image_3d;
};

struct wined3d_null_views_vk
{
    VkBufferView vk_view_buffer_uint;
    VkBufferView vk_view_buffer_float;

    VkDescriptorImageInfo vk_info_1d;
    VkDescriptorImageInfo vk_info_2d;
    VkDescriptorImageInfo vk_info_2dms;
    VkDescriptorImageInfo vk_info_3d;
    VkDescriptorImageInfo vk_info_cube;
    VkDescriptorImageInfo vk_info_1d_array;
    VkDescriptorImageInfo vk_info_2d_array;
    VkDescriptorImageInfo vk_info_2dms_array;
    VkDescriptorImageInfo vk_info_cube_array;
};

struct wined3d_allocator_chunk_vk
{
    struct wined3d_allocator_chunk c;
    VkDeviceMemory vk_memory;
};

static inline struct wined3d_allocator_chunk_vk *wined3d_allocator_chunk_vk(struct wined3d_allocator_chunk *chunk)
{
    return CONTAINING_RECORD(chunk, struct wined3d_allocator_chunk_vk, c);
}

void *wined3d_allocator_chunk_vk_map(struct wined3d_allocator_chunk_vk *chunk_vk,
        struct wined3d_context_vk *context_vk);
void wined3d_allocator_chunk_vk_unmap(struct wined3d_allocator_chunk_vk *chunk_vk,
        struct wined3d_context_vk *context_vk);

struct wined3d_uav_clear_pipelines_vk
{
    VkPipeline buffer;
    VkPipeline image_1d;
    VkPipeline image_1d_array;
    VkPipeline image_2d;
    VkPipeline image_2d_array;
    VkPipeline image_3d;
};

struct wined3d_uav_clear_state_vk
{
    struct wined3d_uav_clear_pipelines_vk float_pipelines;
    struct wined3d_uav_clear_pipelines_vk uint_pipelines;

    struct wined3d_shader_thread_group_size buffer_group_size;
    struct wined3d_shader_thread_group_size image_1d_group_size;
    struct wined3d_shader_thread_group_size image_1d_array_group_size;
    struct wined3d_shader_thread_group_size image_2d_group_size;
    struct wined3d_shader_thread_group_size image_2d_array_group_size;
    struct wined3d_shader_thread_group_size image_3d_group_size;

    struct wined3d_pipeline_layout_vk *image_layout;
    struct wined3d_pipeline_layout_vk *buffer_layout;
};

struct wined3d_device_vk
{
    struct wined3d_device d;

    struct wined3d_context_vk context_vk;

    VkDevice vk_device;
    VkQueue vk_queue;
    uint32_t vk_queue_family_index;
    uint32_t timestamp_bits;

    struct wined3d_vk_info vk_info;

    struct wined3d_null_resources_vk null_resources_vk;
    struct wined3d_null_views_vk null_views_vk;

    CRITICAL_SECTION allocator_cs;
    struct wined3d_allocator allocator;

    struct wined3d_uav_clear_state_vk uav_clear_state;
};

static inline struct wined3d_device_vk *wined3d_device_vk(struct wined3d_device *device)
{
    return CONTAINING_RECORD(device, struct wined3d_device_vk, d);
}

static inline struct wined3d_device_vk *wined3d_device_vk_from_allocator(struct wined3d_allocator *allocator)
{
    return CONTAINING_RECORD(allocator, struct wined3d_device_vk, allocator);
}

static inline void wined3d_device_vk_allocator_lock(struct wined3d_device_vk *device_vk)
{
    EnterCriticalSection(&device_vk->allocator_cs);
}

static inline void wined3d_device_vk_allocator_unlock(struct wined3d_device_vk *device_vk)
{
    LeaveCriticalSection(&device_vk->allocator_cs);
}

bool wined3d_device_vk_create_null_resources(struct wined3d_device_vk *device_vk,
        struct wined3d_context_vk *context_vk);
bool wined3d_device_vk_create_null_views(struct wined3d_device_vk *device_vk,
        struct wined3d_context_vk *context_vk);
void wined3d_device_vk_destroy_null_resources(struct wined3d_device_vk *device_vk,
        struct wined3d_context_vk *context_vk);
void wined3d_device_vk_destroy_null_views(struct wined3d_device_vk *device_vk,
        struct wined3d_context_vk *context_vk);

void wined3d_device_vk_uav_clear_state_init(struct wined3d_device_vk *device_vk);
void wined3d_device_vk_uav_clear_state_cleanup(struct wined3d_device_vk *device_vk);

struct wined3d_texture_vk
{
    struct wined3d_texture t;

    struct wined3d_image_vk image;
    enum VkImageLayout layout;
    uint32_t bind_mask;

    VkDescriptorImageInfo default_image_info;
};

static inline struct wined3d_texture_vk *wined3d_texture_vk(struct wined3d_texture *texture)
{
    return CONTAINING_RECORD(texture, struct wined3d_texture_vk, t);
}

void wined3d_texture_vk_barrier(struct wined3d_texture_vk *texture_vk,
        struct wined3d_context_vk *context_vk, uint32_t bind_mask);
const VkDescriptorImageInfo *wined3d_texture_vk_get_default_image_info(struct wined3d_texture_vk *texture_vk,
        struct wined3d_context_vk *context_vk);
HRESULT wined3d_texture_vk_init(struct wined3d_texture_vk *texture_vk, struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops);
void wined3d_texture_vk_make_generic(struct wined3d_texture_vk *texture_vk,
        struct wined3d_context_vk *context_vk);
BOOL wined3d_texture_vk_prepare_texture(struct wined3d_texture_vk *texture_vk,
        struct wined3d_context_vk *context_vk);

struct wined3d_sampler_vk
{
    struct wined3d_sampler s;

    VkDescriptorImageInfo vk_image_info;
    uint64_t command_buffer_id;
};

static inline struct wined3d_sampler_vk *wined3d_sampler_vk(struct wined3d_sampler *sampler)
{
    return CONTAINING_RECORD(sampler, struct wined3d_sampler_vk, s);
}

void wined3d_sampler_vk_init(struct wined3d_sampler_vk *sampler_vk,
        struct wined3d_device *device, const struct wined3d_sampler_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops);

struct wined3d_buffer_vk
{
    struct wined3d_buffer b;

    VkDescriptorBufferInfo buffer_info;
    uint32_t bind_mask;
};

static inline struct wined3d_buffer_vk *wined3d_buffer_vk(struct wined3d_buffer *buffer)
{
    return CONTAINING_RECORD(buffer, struct wined3d_buffer_vk, b);
}

void wined3d_buffer_vk_barrier(struct wined3d_buffer_vk *buffer_vk,
        struct wined3d_context_vk *context_vk, uint32_t bind_mask);
const VkDescriptorBufferInfo *wined3d_buffer_vk_get_buffer_info(struct wined3d_buffer_vk *buffer_vk);
HRESULT wined3d_buffer_vk_init(struct wined3d_buffer_vk *buffer_vk, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops);

static inline void wined3d_resource_vk_barrier(struct wined3d_resource *resource,
        struct wined3d_context_vk *context_vk, uint32_t bind_mask)
{
    if (resource->type == WINED3D_RTYPE_BUFFER)
        wined3d_buffer_vk_barrier(wined3d_buffer_vk(buffer_from_resource(resource)), context_vk, bind_mask);
    else
        wined3d_texture_vk_barrier(wined3d_texture_vk(texture_from_resource(resource)), context_vk, bind_mask);
}

struct wined3d_rendertarget_view_vk
{
    struct wined3d_rendertarget_view v;

    VkImageView vk_image_view;
    uint64_t command_buffer_id;
};

static inline struct wined3d_rendertarget_view_vk *wined3d_rendertarget_view_vk(
        struct wined3d_rendertarget_view *view)
{
    return CONTAINING_RECORD(view, struct wined3d_rendertarget_view_vk, v);
}

static inline void wined3d_rendertarget_view_vk_barrier(struct wined3d_rendertarget_view_vk *rtv_vk,
        struct wined3d_context_vk *context_vk, uint32_t bind_mask)
{
    wined3d_resource_vk_barrier(rtv_vk->v.resource, context_vk, bind_mask);
}

static inline VkImageView wined3d_rendertarget_view_vk_get_image_view(struct wined3d_rendertarget_view_vk *rtv_vk,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_texture_vk *texture_vk;

    if (rtv_vk->vk_image_view)
        return rtv_vk->vk_image_view;

    texture_vk = wined3d_texture_vk(wined3d_texture_from_resource(rtv_vk->v.resource));
    return wined3d_texture_vk_get_default_image_info(texture_vk, context_vk)->imageView;
}

HRESULT wined3d_rendertarget_view_vk_init(struct wined3d_rendertarget_view_vk *view_vk,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops);

struct wined3d_view_vk
{
    struct wined3d_bo_user bo_user;
    union
    {
        VkBufferView vk_buffer_view;
        VkDescriptorImageInfo vk_image_info;
    } u;
    uint64_t command_buffer_id;
};

struct wined3d_shader_resource_view_vk
{
    struct wined3d_shader_resource_view v;
    struct wined3d_view_vk view_vk;
};

static inline struct wined3d_shader_resource_view_vk *wined3d_shader_resource_view_vk(
        struct wined3d_shader_resource_view *view)
{
    return CONTAINING_RECORD(view, struct wined3d_shader_resource_view_vk, v);
}

static inline void wined3d_shader_resource_view_vk_barrier(struct wined3d_shader_resource_view_vk *srv_vk,
        struct wined3d_context_vk *context_vk, uint32_t bind_mask)
{
    wined3d_resource_vk_barrier(srv_vk->v.resource, context_vk, bind_mask);
}

void wined3d_shader_resource_view_vk_generate_mipmap(struct wined3d_shader_resource_view_vk *srv_vk,
        struct wined3d_context_vk *context_vk);
HRESULT wined3d_shader_resource_view_vk_init(struct wined3d_shader_resource_view_vk *view_vk,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops);
void wined3d_shader_resource_view_vk_update_buffer(struct wined3d_shader_resource_view_vk *view_vk,
        struct wined3d_context_vk *context_vk);
void wined3d_shader_resource_view_vk_update_layout(struct wined3d_shader_resource_view_vk *srv_vk,
        VkImageLayout layout);

struct wined3d_unordered_access_view_vk
{
    struct wined3d_unordered_access_view v;
    struct wined3d_view_vk view_vk;

    VkBufferView vk_counter_view;
    struct wined3d_bo_vk counter_bo;
};

static inline struct wined3d_unordered_access_view_vk *wined3d_unordered_access_view_vk(
        struct wined3d_unordered_access_view *view)
{
    return CONTAINING_RECORD(view, struct wined3d_unordered_access_view_vk, v);
}

static inline void wined3d_unordered_access_view_vk_barrier(struct wined3d_unordered_access_view_vk *uav_vk,
        struct wined3d_context_vk *context_vk, uint32_t bind_mask)
{
    wined3d_resource_vk_barrier(uav_vk->v.resource, context_vk, bind_mask);
}

void wined3d_unordered_access_view_vk_clear(struct wined3d_unordered_access_view_vk *view_vk,
        const struct wined3d_uvec4 *clear_value, struct wined3d_context_vk *context_vk, bool fp);
HRESULT wined3d_unordered_access_view_vk_init(struct wined3d_unordered_access_view_vk *view_vk,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops);
void wined3d_unordered_access_view_vk_update(struct wined3d_unordered_access_view_vk *view_vk,
        struct wined3d_context_vk *context_vk);

struct wined3d_swapchain_vk
{
    struct wined3d_swapchain s;

    VkSwapchainKHR vk_swapchain;
    VkSurfaceKHR vk_surface;
    VkImage *vk_images;
    struct
    {
        VkSemaphore available;
        VkSemaphore presentable;
        uint64_t command_buffer_id;
    } *vk_semaphores;
    unsigned int current, image_count;
    unsigned int width, height;
};

static inline struct wined3d_swapchain_vk *wined3d_swapchain_vk(struct wined3d_swapchain *swapchain)
{
    return CONTAINING_RECORD(swapchain, struct wined3d_swapchain_vk, s);
}

void wined3d_swapchain_vk_cleanup(struct wined3d_swapchain_vk *swapchain_vk);
HRESULT wined3d_swapchain_vk_init(struct wined3d_swapchain_vk *swapchain_vk,
        struct wined3d_device *device, const struct wined3d_swapchain_desc *desc,
        struct wined3d_swapchain_state_parent *state_parent, void *parent,
        const struct wined3d_parent_ops *parent_ops);

struct wined3d_format_vk
{
    struct wined3d_format f;

    VkFormat vk_format;
};

static inline const struct wined3d_format_vk *wined3d_format_vk(const struct wined3d_format *format)
{
    return CONTAINING_RECORD(format, struct wined3d_format_vk, f);
}

static inline void wined3d_context_vk_reference_bo(const struct wined3d_context_vk *context_vk,
        struct wined3d_bo_vk *bo)
{
    bo->command_buffer_id = context_vk->current_command_buffer.id;
}

static inline void wined3d_context_vk_reference_image(const struct wined3d_context_vk *context_vk,
        struct wined3d_image_vk *image)
{
    image->command_buffer_id = context_vk->current_command_buffer.id;
}

static inline void wined3d_context_vk_reference_texture(const struct wined3d_context_vk *context_vk,
        struct wined3d_texture_vk *texture_vk)
{
    wined3d_context_vk_reference_image(context_vk, &texture_vk->image);
}

static inline void wined3d_context_vk_reference_resource(const struct wined3d_context_vk *context_vk,
        struct wined3d_resource *resource)
{
    if (resource->type == WINED3D_RTYPE_BUFFER)
        wined3d_context_vk_reference_bo(context_vk, wined3d_bo_vk(buffer_from_resource(resource)->buffer_object));
    else
        wined3d_context_vk_reference_texture(context_vk, wined3d_texture_vk(texture_from_resource(resource)));
}

static inline void wined3d_context_vk_reference_query(const struct wined3d_context_vk *context_vk,
        struct wined3d_query_vk *query_vk)
{
    query_vk->command_buffer_id = context_vk->current_command_buffer.id;
}

static inline void wined3d_context_vk_reference_sampler(const struct wined3d_context_vk *context_vk,
        struct wined3d_sampler_vk *sampler_vk)
{
    sampler_vk->command_buffer_id = context_vk->current_command_buffer.id;
}

static inline void wined3d_context_vk_reference_rendertarget_view(const struct wined3d_context_vk *context_vk,
        struct wined3d_rendertarget_view_vk *rtv_vk)
{
    wined3d_context_vk_reference_resource(context_vk, rtv_vk->v.resource);
    rtv_vk->command_buffer_id = context_vk->current_command_buffer.id;
}

static inline void wined3d_context_vk_reference_shader_resource_view(const struct wined3d_context_vk *context_vk,
        struct wined3d_shader_resource_view_vk *srv_vk)
{
    wined3d_context_vk_reference_resource(context_vk, srv_vk->v.resource);
    srv_vk->view_vk.command_buffer_id = context_vk->current_command_buffer.id;
}

static inline void wined3d_context_vk_reference_unordered_access_view(const struct wined3d_context_vk *context_vk,
        struct wined3d_unordered_access_view_vk *uav_vk)
{
    wined3d_context_vk_reference_resource(context_vk, uav_vk->v.resource);
    uav_vk->view_vk.command_buffer_id = context_vk->current_command_buffer.id;
}

#endif /* __WINE_WINED3D_VK */
