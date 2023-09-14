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

#define VK_NO_PROTOTYPES
#ifndef USE_WIN32_VULKAN
#define WINE_VK_HOST
#endif
#include "wine/vulkan.h"

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
    VK_INSTANCE_EXT_PFN(vkGetPhysicalDeviceProperties2)

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
    VK_DEVICE_PFN(vkWaitForFences)

#define DECLARE_VK_PFN(name) PFN_##name name;

struct vulkan_ops
{
#define VK_INSTANCE_PFN     DECLARE_VK_PFN
#define VK_INSTANCE_EXT_PFN DECLARE_VK_PFN
#define VK_DEVICE_PFN       DECLARE_VK_PFN
    VK_DEVICE_FUNCS()
    VK_INSTANCE_FUNCS()
#undef VK_INSTANCE_PFN
#undef VK_INSTANCE_EXT_PFN
#undef VK_DEVICE_PFN

    PFN_vkCreateInstance vkCreateInstance;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
};

struct wined3d_vk_info
{
    struct vulkan_ops vk_ops;

    VkInstance instance;
    unsigned int api_version;

#ifdef USE_WIN32_VULKAN
    HMODULE vulkan_lib;
#endif
};

#define VK_CALL(f) (vk_info->vk_ops.f)

#endif /* __WINE_WINED3D_VK */
