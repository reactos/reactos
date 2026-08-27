/*
 * Copyright 2016 Józef Kucia for CodeWeavers
 * Copyright 2016 Henri Verbeet for CodeWeavers
 * Copyright 2021 Conor McCarthy for CodeWeavers
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

#include "vkd3d_private.h"
#include <math.h>

static void d3d12_fence_incref(struct d3d12_fence *fence);
static void d3d12_fence_decref(struct d3d12_fence *fence);
static HRESULT d3d12_fence_signal(struct d3d12_fence *fence, uint64_t value, VkFence vk_fence, bool on_cpu);
static void d3d12_fence_signal_timeline_semaphore(struct d3d12_fence *fence, uint64_t timeline_value);
static HRESULT d3d12_command_queue_signal(struct d3d12_command_queue *command_queue,
        struct d3d12_fence *fence, uint64_t value);
static void d3d12_command_queue_submit_locked(struct d3d12_command_queue *queue);
static HRESULT d3d12_command_queue_flush_ops(struct d3d12_command_queue *queue, bool *flushed_any);
static HRESULT d3d12_command_queue_flush_ops_locked(struct d3d12_command_queue *queue, bool *flushed_any);

HRESULT vkd3d_queue_create(struct d3d12_device *device,
        uint32_t family_index, const VkQueueFamilyProperties *properties, struct vkd3d_queue **queue)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct vkd3d_queue *object;

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    vkd3d_mutex_init(&object->mutex);

    object->completed_sequence_number = 0;
    object->submitted_sequence_number = 0;

    object->vk_family_index = family_index;
    object->vk_queue_flags = properties->queueFlags;
    object->timestamp_bits = properties->timestampValidBits;

    object->semaphores = NULL;
    object->semaphores_size = 0;
    object->semaphore_count = 0;

    memset(object->old_vk_semaphores, 0, sizeof(object->old_vk_semaphores));

    VK_CALL(vkGetDeviceQueue(device->vk_device, family_index, 0, &object->vk_queue));

    TRACE("Created queue %p for queue family index %u.\n", object, family_index);

    *queue = object;

    return S_OK;
}

void vkd3d_queue_destroy(struct vkd3d_queue *queue, struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    unsigned int i;

    vkd3d_mutex_lock(&queue->mutex);

    for (i = 0; i < queue->semaphore_count; ++i)
        VK_CALL(vkDestroySemaphore(device->vk_device, queue->semaphores[i].vk_semaphore, NULL));

    vkd3d_free(queue->semaphores);

    for (i = 0; i < ARRAY_SIZE(queue->old_vk_semaphores); ++i)
    {
        if (queue->old_vk_semaphores[i])
            VK_CALL(vkDestroySemaphore(device->vk_device, queue->old_vk_semaphores[i], NULL));
    }

    vkd3d_mutex_unlock(&queue->mutex);

    vkd3d_mutex_destroy(&queue->mutex);
    vkd3d_free(queue);
}

VkQueue vkd3d_queue_acquire(struct vkd3d_queue *queue)
{
    TRACE("queue %p.\n", queue);

    vkd3d_mutex_lock(&queue->mutex);

    VKD3D_ASSERT(queue->vk_queue);
    return queue->vk_queue;
}

void vkd3d_queue_release(struct vkd3d_queue *queue)
{
    TRACE("queue %p.\n", queue);

    vkd3d_mutex_unlock(&queue->mutex);
}

static VkResult vkd3d_queue_wait_idle(struct vkd3d_queue *queue,
        const struct vkd3d_vk_device_procs *vk_procs)
{
    VkQueue vk_queue;
    VkResult vr;

    if ((vk_queue = vkd3d_queue_acquire(queue)))
    {
        vr = VK_CALL(vkQueueWaitIdle(vk_queue));
        vkd3d_queue_release(queue);

        if (vr < 0)
            WARN("Failed to wait for queue, vr %d.\n", vr);
    }
    else
    {
        ERR("Failed to acquire queue %p.\n", queue);
        vr = VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    return vr;
}

static void vkd3d_queue_update_sequence_number(struct vkd3d_queue *queue,
        uint64_t sequence_number, struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    unsigned int destroyed_semaphore_count = 0;
    uint64_t completed_sequence_number;
    VkSemaphore vk_semaphore;
    unsigned int i, j;

    vkd3d_mutex_lock(&queue->mutex);

    completed_sequence_number = queue->completed_sequence_number;
    queue->completed_sequence_number = max(sequence_number, queue->completed_sequence_number);

    TRACE("Queue %p sequence number %"PRIu64" -> %"PRIu64".\n",
            queue, completed_sequence_number, queue->completed_sequence_number);

    for (i = 0; i < queue->semaphore_count; ++i)
    {
        if (queue->semaphores[i].sequence_number > queue->completed_sequence_number)
            break;

        vk_semaphore = queue->semaphores[i].vk_semaphore;

        /* Try to store the Vulkan semaphore for reuse. */
        for (j = 0; j < ARRAY_SIZE(queue->old_vk_semaphores); ++j)
        {
            if (queue->old_vk_semaphores[j] == VK_NULL_HANDLE)
            {
                queue->old_vk_semaphores[j] = vk_semaphore;
                vk_semaphore = VK_NULL_HANDLE;
                break;
            }
        }

        if (!vk_semaphore)
            continue;

        VK_CALL(vkDestroySemaphore(device->vk_device, vk_semaphore, NULL));
        ++destroyed_semaphore_count;
    }
    if (i > 0)
    {
        queue->semaphore_count -= i;
        memmove(queue->semaphores, &queue->semaphores[i], queue->semaphore_count * sizeof(*queue->semaphores));
    }

    if (destroyed_semaphore_count)
        TRACE("Destroyed %u Vulkan semaphores.\n", destroyed_semaphore_count);

    vkd3d_mutex_unlock(&queue->mutex);
}

static uint64_t vkd3d_queue_reset_sequence_number_locked(struct vkd3d_queue *queue)
{
    unsigned int i;

    WARN("Resetting sequence number for queue %p.\n", queue);

    queue->completed_sequence_number = 0;
    queue->submitted_sequence_number = 1;

    for (i = 0; i < queue->semaphore_count; ++i)
        queue->semaphores[i].sequence_number = queue->submitted_sequence_number;

    return queue->submitted_sequence_number;
}

static VkResult vkd3d_queue_create_vk_semaphore_locked(struct vkd3d_queue *queue,
        struct d3d12_device *device, VkSemaphore *vk_semaphore)
{
    const struct vkd3d_vk_device_procs *vk_procs;
    VkSemaphoreCreateInfo semaphore_info;
    unsigned int i;
    VkResult vr;

    *vk_semaphore = VK_NULL_HANDLE;

    for (i = 0; i < ARRAY_SIZE(queue->old_vk_semaphores); ++i)
    {
        if ((*vk_semaphore = queue->old_vk_semaphores[i]))
        {
            queue->old_vk_semaphores[i] = VK_NULL_HANDLE;
            break;
        }
    }

    if (*vk_semaphore)
        return VK_SUCCESS;

    vk_procs = &device->vk_procs;

    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = NULL;
    semaphore_info.flags = 0;
    if ((vr = VK_CALL(vkCreateSemaphore(device->vk_device, &semaphore_info, NULL, vk_semaphore))) < 0)
    {
        WARN("Failed to create Vulkan semaphore, vr %d.\n", vr);
        *vk_semaphore = VK_NULL_HANDLE;
    }

    return vr;
}

/* Fence worker thread */
static HRESULT vkd3d_enqueue_gpu_fence(struct vkd3d_fence_worker *worker,
        VkFence vk_fence, struct d3d12_fence *fence, uint64_t value,
        struct vkd3d_queue *queue, uint64_t queue_sequence_number)
{
    struct vkd3d_waiting_fence *waiting_fence;

    TRACE("worker %p, fence %p, value %#"PRIx64".\n", worker, fence, value);

    vkd3d_mutex_lock(&worker->mutex);

    if (!vkd3d_array_reserve((void **)&worker->fences, &worker->fences_size,
            worker->fence_count + 1, sizeof(*worker->fences)))
    {
        ERR("Failed to add GPU fence.\n");
        vkd3d_mutex_unlock(&worker->mutex);
        return E_OUTOFMEMORY;
    }

    waiting_fence = &worker->fences[worker->fence_count++];
    waiting_fence->fence = fence;
    waiting_fence->value = value;
    waiting_fence->u.vk_fence = vk_fence;
    waiting_fence->queue_sequence_number = queue_sequence_number;

    d3d12_fence_incref(fence);

    vkd3d_cond_signal(&worker->cond);
    vkd3d_mutex_unlock(&worker->mutex);

    return S_OK;
}

static void vkd3d_wait_for_gpu_timeline_semaphore(struct vkd3d_fence_worker *worker,
        const struct vkd3d_waiting_fence *waiting_fence)
{
    const struct d3d12_device *device = worker->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkSemaphoreWaitInfoKHR wait_info;
    VkResult vr;

    wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR;
    wait_info.pNext = NULL;
    wait_info.flags = 0;
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &waiting_fence->u.vk_semaphore;
    wait_info.pValues = &waiting_fence->value;

    vr = VK_CALL(vkWaitSemaphoresKHR(device->vk_device, &wait_info, ~(uint64_t)0));
    if (vr == VK_TIMEOUT)
        return;
    if (vr != VK_SUCCESS)
    {
        ERR("Failed to wait for Vulkan timeline semaphore, vr %d.\n", vr);
        return;
    }

    TRACE("Signaling fence %p value %#"PRIx64".\n", waiting_fence->fence, waiting_fence->value);
    d3d12_fence_signal_timeline_semaphore(waiting_fence->fence, waiting_fence->value);

    d3d12_fence_decref(waiting_fence->fence);
}

static void vkd3d_wait_for_gpu_fence(struct vkd3d_fence_worker *worker,
        const struct vkd3d_waiting_fence *waiting_fence)
{
    struct d3d12_device *device = worker->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    HRESULT hr;
    int vr;

    vr = VK_CALL(vkWaitForFences(device->vk_device, 1, &waiting_fence->u.vk_fence, VK_FALSE, ~(uint64_t)0));
    if (vr == VK_TIMEOUT)
        return;
    if (vr != VK_SUCCESS)
    {
        ERR("Failed to wait for Vulkan fence, vr %d.\n", vr);
        return;
    }

    TRACE("Signaling fence %p value %#"PRIx64".\n", waiting_fence->fence, waiting_fence->value);
    if (FAILED(hr = d3d12_fence_signal(waiting_fence->fence, waiting_fence->value, waiting_fence->u.vk_fence, false)))
        ERR("Failed to signal d3d12 fence, hr %s.\n", debugstr_hresult(hr));

    d3d12_fence_decref(waiting_fence->fence);

    vkd3d_queue_update_sequence_number(worker->queue, waiting_fence->queue_sequence_number, device);
}

static void *vkd3d_fence_worker_main(void *arg)
{
    size_t old_fences_size, cur_fences_size = 0, cur_fence_count = 0;
    struct vkd3d_waiting_fence *old_fences, *cur_fences = NULL;
    struct vkd3d_fence_worker *worker = arg;
    unsigned int i;

    vkd3d_set_thread_name("vkd3d_fence");

    for (;;)
    {
        vkd3d_mutex_lock(&worker->mutex);

        if (!worker->fence_count && !worker->should_exit)
            vkd3d_cond_wait(&worker->cond, &worker->mutex);

        if (worker->should_exit)
        {
            vkd3d_mutex_unlock(&worker->mutex);
            break;
        }

        old_fences_size = cur_fences_size;
        old_fences = cur_fences;

        cur_fence_count = worker->fence_count;
        cur_fences_size = worker->fences_size;
        cur_fences = worker->fences;

        worker->fence_count = 0;
        worker->fences_size = old_fences_size;
        worker->fences = old_fences;

        vkd3d_mutex_unlock(&worker->mutex);

        for (i = 0; i < cur_fence_count; ++i)
            worker->wait_for_gpu_fence(worker, &cur_fences[i]);
    }

    vkd3d_free(cur_fences);
    return NULL;
}

static HRESULT vkd3d_fence_worker_start(struct vkd3d_fence_worker *worker,
        struct vkd3d_queue *queue, struct d3d12_device *device)
{
    HRESULT hr;

    TRACE("worker %p.\n", worker);

    worker->should_exit = false;
    worker->queue = queue;
    worker->device = device;

    worker->fence_count = 0;
    worker->fences = NULL;
    worker->fences_size = 0;

    worker->wait_for_gpu_fence = device->vk_info.KHR_timeline_semaphore
            ? vkd3d_wait_for_gpu_timeline_semaphore : vkd3d_wait_for_gpu_fence;

    vkd3d_mutex_init(&worker->mutex);

    vkd3d_cond_init(&worker->cond);

    if (FAILED(hr = vkd3d_create_thread(device->vkd3d_instance,
            vkd3d_fence_worker_main, worker, &worker->thread)))
    {
        vkd3d_mutex_destroy(&worker->mutex);
        vkd3d_cond_destroy(&worker->cond);
    }

    return hr;
}

static HRESULT vkd3d_fence_worker_stop(struct vkd3d_fence_worker *worker,
        struct d3d12_device *device)
{
    HRESULT hr;

    TRACE("worker %p.\n", worker);

    vkd3d_mutex_lock(&worker->mutex);

    worker->should_exit = true;
    vkd3d_cond_signal(&worker->cond);

    vkd3d_mutex_unlock(&worker->mutex);

    if (FAILED(hr = vkd3d_join_thread(device->vkd3d_instance, &worker->thread)))
        return hr;

    vkd3d_mutex_destroy(&worker->mutex);
    vkd3d_cond_destroy(&worker->cond);

    vkd3d_free(worker->fences);

    return S_OK;
}

static const struct d3d12_root_parameter *root_signature_get_parameter(
        const struct d3d12_root_signature *root_signature, unsigned int index)
{
    VKD3D_ASSERT(index < root_signature->parameter_count);
    return &root_signature->parameters[index];
}

static const struct d3d12_root_descriptor_table *root_signature_get_descriptor_table(
        const struct d3d12_root_signature *root_signature, unsigned int index)
{
    const struct d3d12_root_parameter *p = root_signature_get_parameter(root_signature, index);
    VKD3D_ASSERT(p->parameter_type == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE);
    return &p->u.descriptor_table;
}

static const struct d3d12_root_constant *root_signature_get_32bit_constants(
        const struct d3d12_root_signature *root_signature, unsigned int index)
{
    const struct d3d12_root_parameter *p = root_signature_get_parameter(root_signature, index);
    VKD3D_ASSERT(p->parameter_type == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
    return &p->u.constant;
}

static const struct d3d12_root_parameter *root_signature_get_root_descriptor(
        const struct d3d12_root_signature *root_signature, unsigned int index)
{
    const struct d3d12_root_parameter *p = root_signature_get_parameter(root_signature, index);
    VKD3D_ASSERT(p->parameter_type == D3D12_ROOT_PARAMETER_TYPE_CBV
        || p->parameter_type == D3D12_ROOT_PARAMETER_TYPE_SRV
        || p->parameter_type == D3D12_ROOT_PARAMETER_TYPE_UAV);
    return p;
}

/* ID3D12Fence */
static struct d3d12_fence *impl_from_ID3D12Fence1(ID3D12Fence1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_fence, ID3D12Fence1_iface);
}

static VkResult d3d12_fence_create_vk_fence(struct d3d12_fence *fence, VkFence *vk_fence)
{
    const struct vkd3d_vk_device_procs *vk_procs;
    struct d3d12_device *device = fence->device;
    VkFenceCreateInfo fence_info;
    unsigned int i;
    VkResult vr;

    *vk_fence = VK_NULL_HANDLE;

    vkd3d_mutex_lock(&fence->mutex);

    for (i = 0; i < ARRAY_SIZE(fence->old_vk_fences); ++i)
    {
        if ((*vk_fence = fence->old_vk_fences[i]))
        {
            fence->old_vk_fences[i] = VK_NULL_HANDLE;
            break;
        }
    }

    vkd3d_mutex_unlock(&fence->mutex);

    if (*vk_fence)
        return VK_SUCCESS;

    vk_procs = &device->vk_procs;

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = NULL;
    fence_info.flags = 0;
    if ((vr = VK_CALL(vkCreateFence(device->vk_device, &fence_info, NULL, vk_fence))) < 0)
    {
        WARN("Failed to create Vulkan fence, vr %d.\n", vr);
        *vk_fence = VK_NULL_HANDLE;
    }

    return vr;
}

static void d3d12_fence_garbage_collect_vk_semaphores_locked(struct d3d12_fence *fence,
        bool destroy_all)
{
    struct d3d12_device *device = fence->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct vkd3d_signaled_semaphore *current;
    unsigned int i, semaphore_count;

    semaphore_count = fence->semaphore_count;
    if (!destroy_all && semaphore_count < VKD3D_MAX_VK_SYNC_OBJECTS)
        return;

    i = 0;
    while (i < fence->semaphore_count)
    {
        if (!destroy_all && fence->semaphore_count < VKD3D_MAX_VK_SYNC_OBJECTS)
            break;

        current = &fence->semaphores[i];
        /* The semaphore doesn't have a pending signal operation if the fence
         * was signaled. */
        if ((current->u.binary.vk_fence || current->u.binary.is_acquired) && !destroy_all)
        {
            ++i;
            continue;
        }

        if (current->u.binary.vk_fence)
            WARN("Destroying potentially pending semaphore.\n");
        VKD3D_ASSERT(!current->u.binary.is_acquired);

        VK_CALL(vkDestroySemaphore(device->vk_device, current->u.binary.vk_semaphore, NULL));
        fence->semaphores[i] = fence->semaphores[--fence->semaphore_count];
    }

    if (semaphore_count != fence->semaphore_count)
        TRACE("Destroyed %u Vulkan semaphores.\n", semaphore_count - fence->semaphore_count);
}

static void d3d12_fence_destroy_vk_objects(struct d3d12_fence *fence)
{
    const struct vkd3d_vk_device_procs *vk_procs;
    struct d3d12_device *device = fence->device;
    unsigned int i;

    vkd3d_mutex_lock(&fence->mutex);

    vk_procs = &device->vk_procs;

    for (i = 0; i < ARRAY_SIZE(fence->old_vk_fences); ++i)
    {
        if (fence->old_vk_fences[i])
            VK_CALL(vkDestroyFence(device->vk_device, fence->old_vk_fences[i], NULL));
        fence->old_vk_fences[i] = VK_NULL_HANDLE;
    }

    d3d12_fence_garbage_collect_vk_semaphores_locked(fence, true);
    VK_CALL(vkDestroySemaphore(device->vk_device, fence->timeline_semaphore, NULL));

    vkd3d_mutex_unlock(&fence->mutex);
}

static struct vkd3d_signaled_semaphore *d3d12_fence_acquire_vk_semaphore_locked(struct d3d12_fence *fence,
        uint64_t value, uint64_t *completed_value)
{
    struct vkd3d_signaled_semaphore *semaphore;
    struct vkd3d_signaled_semaphore *current;
    uint64_t semaphore_value;
    unsigned int i;

    TRACE("fence %p, value %#"PRIx64".\n", fence, value);

    semaphore = NULL;
    semaphore_value = ~(uint64_t)0;

    for (i = 0; i < fence->semaphore_count; ++i)
    {
        current = &fence->semaphores[i];
        /* Prefer a semaphore with the smallest value. */
        if (!current->u.binary.is_acquired && current->value >= value && semaphore_value >= current->value)
        {
            semaphore = current;
            semaphore_value = current->value;
        }
        if (semaphore_value == value)
            break;
    }

    if (semaphore)
        semaphore->u.binary.is_acquired = true;

    *completed_value = fence->value;

    return semaphore;
}

static void d3d12_fence_remove_vk_semaphore(struct d3d12_fence *fence, struct vkd3d_signaled_semaphore *semaphore)
{
    vkd3d_mutex_lock(&fence->mutex);

    VKD3D_ASSERT(semaphore->u.binary.is_acquired);

    *semaphore = fence->semaphores[--fence->semaphore_count];

    vkd3d_mutex_unlock(&fence->mutex);
}

static void d3d12_fence_release_vk_semaphore(struct d3d12_fence *fence, struct vkd3d_signaled_semaphore *semaphore)
{
    vkd3d_mutex_lock(&fence->mutex);

    VKD3D_ASSERT(semaphore->u.binary.is_acquired);
    semaphore->u.binary.is_acquired = false;

    vkd3d_mutex_unlock(&fence->mutex);
}

static void d3d12_fence_update_pending_value_locked(struct d3d12_fence *fence)
{
    uint64_t new_max_pending_value;
    unsigned int i;

    for (i = 0, new_max_pending_value = 0; i < fence->semaphore_count; ++i)
        new_max_pending_value = max(fence->semaphores[i].value, new_max_pending_value);

    fence->max_pending_value = max(fence->value, new_max_pending_value);
}

static HRESULT d3d12_fence_update_pending_value(struct d3d12_fence *fence)
{
    vkd3d_mutex_lock(&fence->mutex);

    d3d12_fence_update_pending_value_locked(fence);

    vkd3d_mutex_unlock(&fence->mutex);

    return S_OK;
}

static HRESULT d3d12_command_queue_record_as_blocked(struct d3d12_command_queue *command_queue)
{
    struct d3d12_device *device = command_queue->device;
    HRESULT hr = S_OK;

    vkd3d_mutex_lock(&device->blocked_queues_mutex);

    if (device->blocked_queue_count < ARRAY_SIZE(device->blocked_queues))
    {
        device->blocked_queues[device->blocked_queue_count++] = command_queue;
    }
    else
    {
        WARN("Failed to add blocked command queue %p to device %p.\n", command_queue, device);
        hr = E_FAIL;
    }

    vkd3d_mutex_unlock(&device->blocked_queues_mutex);
    return hr;
}

static HRESULT d3d12_device_flush_blocked_queues_once(struct d3d12_device *device, bool *flushed_any)
{
    struct d3d12_command_queue *blocked_queues[VKD3D_MAX_DEVICE_BLOCKED_QUEUES];
    unsigned int i, blocked_queue_count;
    HRESULT hr = S_OK;

    *flushed_any = false;

    vkd3d_mutex_lock(&device->blocked_queues_mutex);

    /* Flush any ops unblocked by a new pending value. These cannot be
     * flushed while holding blocked_queue_mutex, so move the queue
     * pointers to a local array. */
    blocked_queue_count = device->blocked_queue_count;
    memcpy(blocked_queues, device->blocked_queues, blocked_queue_count * sizeof(blocked_queues[0]));
    device->blocked_queue_count = 0;

    vkd3d_mutex_unlock(&device->blocked_queues_mutex);

    for (i = 0; i < blocked_queue_count; ++i)
    {
        HRESULT new_hr;

        new_hr = d3d12_command_queue_flush_ops(blocked_queues[i], flushed_any);

        if (SUCCEEDED(hr))
            hr = new_hr;
    }

    return hr;
}

static HRESULT d3d12_device_flush_blocked_queues(struct d3d12_device *device)
{
    bool flushed_any;
    HRESULT hr;

    /* Executing an op on one queue may unblock another, so repeat until nothing is flushed. */
    do
    {
        if (FAILED(hr = d3d12_device_flush_blocked_queues_once(device, &flushed_any)))
            return hr;
    }
    while (flushed_any);

    return S_OK;
}

static HRESULT d3d12_fence_add_vk_semaphore(struct d3d12_fence *fence, VkSemaphore vk_semaphore,
        VkFence vk_fence, uint64_t value, const struct vkd3d_queue *signalling_queue)
{
    struct vkd3d_signaled_semaphore *semaphore;

    TRACE("fence %p, value %#"PRIx64".\n", fence, value);

    vkd3d_mutex_lock(&fence->mutex);

    d3d12_fence_garbage_collect_vk_semaphores_locked(fence, false);

    if (!vkd3d_array_reserve((void**)&fence->semaphores, &fence->semaphores_size,
            fence->semaphore_count + 1, sizeof(*fence->semaphores)))
    {
        ERR("Failed to add semaphore.\n");
        vkd3d_mutex_unlock(&fence->mutex);
        return E_OUTOFMEMORY;
    }

    semaphore = &fence->semaphores[fence->semaphore_count++];
    semaphore->value = value;
    semaphore->u.binary.vk_semaphore = vk_semaphore;
    semaphore->u.binary.vk_fence = vk_fence;
    semaphore->u.binary.is_acquired = false;
    semaphore->signalling_queue = signalling_queue;

    d3d12_fence_update_pending_value_locked(fence);

    vkd3d_mutex_unlock(&fence->mutex);

    return d3d12_device_flush_blocked_queues(fence->device);
}

static void d3d12_fence_signal_external_events_locked(struct d3d12_fence *fence)
{
    struct d3d12_device *device = fence->device;
    bool signal_null_event_cond = false;
    unsigned int i, j;

    for (i = 0, j = 0; i < fence->event_count; ++i)
    {
        struct vkd3d_waiting_event *current = &fence->events[i];

        if (current->value <= fence->value)
        {
            if (current->event)
            {
                device->signal_event(current->event);
            }
            else
            {
                *current->latch = true;
                signal_null_event_cond = true;
            }
        }
        else
        {
            if (i != j)
                fence->events[j] = *current;
            ++j;
        }
    }

    fence->event_count = j;

    if (signal_null_event_cond)
        vkd3d_cond_broadcast(&fence->null_event_cond);
}

static HRESULT d3d12_fence_signal(struct d3d12_fence *fence, uint64_t value, VkFence vk_fence, bool on_cpu)
{
    struct d3d12_device *device = fence->device;
    struct vkd3d_signaled_semaphore *current;
    unsigned int i;

    vkd3d_mutex_lock(&fence->mutex);

    fence->value = value;

    d3d12_fence_signal_external_events_locked(fence);

    if (vk_fence)
    {
        const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

        for (i = 0; i < fence->semaphore_count; ++i)
        {
            current = &fence->semaphores[i];
            if (current->u.binary.vk_fence == vk_fence)
                current->u.binary.vk_fence = VK_NULL_HANDLE;
        }

        for (i = 0; i < ARRAY_SIZE(fence->old_vk_fences); ++i)
        {
            if (fence->old_vk_fences[i] == VK_NULL_HANDLE)
            {
                fence->old_vk_fences[i] = vk_fence;
                VK_CALL(vkResetFences(device->vk_device, 1, &vk_fence));
                vk_fence = VK_NULL_HANDLE;
                break;
            }
        }
        if (vk_fence)
            VK_CALL(vkDestroyFence(device->vk_device, vk_fence, NULL));
    }

    d3d12_fence_update_pending_value_locked(fence);

    vkd3d_mutex_unlock(&fence->mutex);

    return on_cpu ? d3d12_device_flush_blocked_queues(device) : S_OK;
}

static uint64_t d3d12_fence_add_pending_timeline_signal(struct d3d12_fence *fence, uint64_t virtual_value,
        const struct vkd3d_queue *signalling_queue)
{
    struct vkd3d_signaled_semaphore *semaphore;

    vkd3d_mutex_lock(&fence->mutex);

    if (!vkd3d_array_reserve((void **)&fence->semaphores, &fence->semaphores_size,
            fence->semaphore_count + 1, sizeof(*fence->semaphores)))
    {
        return 0;
    }

    semaphore = &fence->semaphores[fence->semaphore_count++];
    semaphore->value = virtual_value;
    semaphore->u.timeline_value = ++fence->pending_timeline_value;
    semaphore->signalling_queue = signalling_queue;

    vkd3d_mutex_unlock(&fence->mutex);

    return fence->pending_timeline_value;
}

static uint64_t d3d12_fence_get_timeline_wait_value_locked(struct d3d12_fence *fence, uint64_t virtual_value)
{
    uint64_t target_timeline_value = UINT64_MAX;
    unsigned int i;

    /* Find the smallest physical value which is at least the virtual value. */
    for (i = 0; i < fence->semaphore_count; ++i)
    {
        if (virtual_value <= fence->semaphores[i].value)
            target_timeline_value = min(target_timeline_value, fence->semaphores[i].u.timeline_value);
    }

    /* No timeline value will be found if it was already signaled on the GPU and handled in
     * the worker thread. A wait must still be emitted as a barrier against command re-ordering. */
    return (target_timeline_value == UINT64_MAX) ? 0 : target_timeline_value;
}

static void d3d12_fence_signal_timeline_semaphore(struct d3d12_fence *fence, uint64_t timeline_value)
{
    bool did_signal;
    unsigned int i;

    vkd3d_mutex_lock(&fence->mutex);

    /* With multiple fence workers, it is possible that signal calls are out of
     * order. The physical value itself is monotonic, but we need to make sure
     * that all signals happen in correct order if there are fence rewinds.
     * We don't expect the loop to run more than once, but there might be
     * extreme edge cases where we signal 2 or more. */
    while (fence->timeline_value < timeline_value)
    {
        ++fence->timeline_value;
        did_signal = false;

        for (i = 0; i < fence->semaphore_count; ++i)
        {
            if (fence->timeline_value == fence->semaphores[i].u.timeline_value)
            {
                fence->value = fence->semaphores[i].value;
                d3d12_fence_signal_external_events_locked(fence);
                fence->semaphores[i] = fence->semaphores[--fence->semaphore_count];
                did_signal = true;
                break;
            }
        }

        if (!did_signal)
            FIXME("Did not signal a virtual value.\n");
    }

    /* If a rewind remains queued, the virtual value deleted above may be
     * greater than any pending value, so update the max pending value. */
    d3d12_fence_update_pending_value_locked(fence);

    vkd3d_mutex_unlock(&fence->mutex);
}

static HRESULT STDMETHODCALLTYPE d3d12_fence_QueryInterface(ID3D12Fence1 *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D12Fence1)
            || IsEqualGUID(riid, &IID_ID3D12Fence)
            || IsEqualGUID(riid, &IID_ID3D12Pageable)
            || IsEqualGUID(riid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(riid, &IID_ID3D12Object)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D12Fence1_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_fence_AddRef(ID3D12Fence1 *iface)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&fence->refcount);

    TRACE("%p increasing refcount to %u.\n", fence, refcount);

    return refcount;
}

static void d3d12_fence_incref(struct d3d12_fence *fence)
{
    vkd3d_atomic_increment_u32(&fence->internal_refcount);
}

static ULONG STDMETHODCALLTYPE d3d12_fence_Release(ID3D12Fence1 *iface)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&fence->refcount);

    TRACE("%p decreasing refcount to %u.\n", fence, refcount);

    if (!refcount)
        d3d12_fence_decref(fence);

    return refcount;
}

static void d3d12_fence_decref(struct d3d12_fence *fence)
{
    struct d3d12_device *device;

    if (vkd3d_atomic_decrement_u32(&fence->internal_refcount))
        return;

    device = fence->device;

    vkd3d_private_store_destroy(&fence->private_store);

    d3d12_fence_destroy_vk_objects(fence);

    vkd3d_free(fence->events);
    vkd3d_free(fence->semaphores);
    vkd3d_mutex_destroy(&fence->mutex);
    vkd3d_cond_destroy(&fence->null_event_cond);
    vkd3d_free(fence);

    d3d12_device_release(device);
}

static HRESULT STDMETHODCALLTYPE d3d12_fence_GetPrivateData(ID3D12Fence1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&fence->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_fence_SetPrivateData(ID3D12Fence1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&fence->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_fence_SetPrivateDataInterface(ID3D12Fence1 *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&fence->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_fence_SetName(ID3D12Fence1 *iface, const WCHAR *name)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, fence->device->wchar_size));

    return name ? S_OK : E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE d3d12_fence_GetDevice(ID3D12Fence1 *iface, REFIID iid, void **device)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(fence->device, iid, device);
}

static UINT64 STDMETHODCALLTYPE d3d12_fence_GetCompletedValue(ID3D12Fence1 *iface)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);
    uint64_t completed_value;

    TRACE("iface %p.\n", iface);

    vkd3d_mutex_lock(&fence->mutex);
    completed_value = fence->value;
    vkd3d_mutex_unlock(&fence->mutex);
    return completed_value;
}

static HRESULT STDMETHODCALLTYPE d3d12_fence_SetEventOnCompletion(ID3D12Fence1 *iface,
        UINT64 value, HANDLE event)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);
    unsigned int i;
    bool latch = false;

    TRACE("iface %p, value %#"PRIx64", event %p.\n", iface, value, event);

    vkd3d_mutex_lock(&fence->mutex);

    if (value <= fence->value)
    {
        if (event)
            fence->device->signal_event(event);
        vkd3d_mutex_unlock(&fence->mutex);
        return S_OK;
    }

    for (i = 0; i < fence->event_count; ++i)
    {
        struct vkd3d_waiting_event *current = &fence->events[i];
        if (current->value == value && current->event == event)
        {
            WARN("Event completion for (%p, %#"PRIx64") is already in the list.\n",
                    event, value);
            vkd3d_mutex_unlock(&fence->mutex);
            return S_OK;
        }
    }

    if (!vkd3d_array_reserve((void **)&fence->events, &fence->events_size,
            fence->event_count + 1, sizeof(*fence->events)))
    {
        WARN("Failed to add event.\n");
        vkd3d_mutex_unlock(&fence->mutex);
        return E_OUTOFMEMORY;
    }

    fence->events[fence->event_count].value = value;
    fence->events[fence->event_count].event = event;
    fence->events[fence->event_count].latch = &latch;
    ++fence->event_count;

    /* If event is NULL, we need to block until the fence value completes.
     * Implement this in a uniform way where we pretend we have a dummy event.
     * A NULL fence->events[].event means that we should set latch to true
     * and signal a condition variable instead of calling external signal_event callback. */
    if (!event)
    {
        while (!latch)
            vkd3d_cond_wait(&fence->null_event_cond, &fence->mutex);
    }

    vkd3d_mutex_unlock(&fence->mutex);
    return S_OK;
}

static HRESULT d3d12_fence_signal_cpu_timeline_semaphore(struct d3d12_fence *fence, uint64_t value)
{
    vkd3d_mutex_lock(&fence->mutex);

    fence->value = value;
    d3d12_fence_signal_external_events_locked(fence);
    d3d12_fence_update_pending_value_locked(fence);

    vkd3d_mutex_unlock(&fence->mutex);

    return d3d12_device_flush_blocked_queues(fence->device);
}

static HRESULT STDMETHODCALLTYPE d3d12_fence_Signal(ID3D12Fence1 *iface, UINT64 value)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);

    TRACE("iface %p, value %#"PRIx64".\n", iface, value);

    if (fence->timeline_semaphore)
        return d3d12_fence_signal_cpu_timeline_semaphore(fence, value);
    return d3d12_fence_signal(fence, value, VK_NULL_HANDLE, true);
}

static D3D12_FENCE_FLAGS STDMETHODCALLTYPE d3d12_fence_GetCreationFlags(ID3D12Fence1 *iface)
{
    struct d3d12_fence *fence = impl_from_ID3D12Fence1(iface);

    TRACE("iface %p.\n", iface);

    return fence->flags;
}

static const struct ID3D12Fence1Vtbl d3d12_fence_vtbl =
{
    /* IUnknown methods */
    d3d12_fence_QueryInterface,
    d3d12_fence_AddRef,
    d3d12_fence_Release,
    /* ID3D12Object methods */
    d3d12_fence_GetPrivateData,
    d3d12_fence_SetPrivateData,
    d3d12_fence_SetPrivateDataInterface,
    d3d12_fence_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_fence_GetDevice,
    /* ID3D12Fence methods */
    d3d12_fence_GetCompletedValue,
    d3d12_fence_SetEventOnCompletion,
    d3d12_fence_Signal,
    /* ID3D12Fence1 methods */
    d3d12_fence_GetCreationFlags,
};

static struct d3d12_fence *unsafe_impl_from_ID3D12Fence(ID3D12Fence *iface)
{
    ID3D12Fence1 *iface1;

    if (!(iface1 = (ID3D12Fence1 *)iface))
        return NULL;
    VKD3D_ASSERT(iface1->lpVtbl == &d3d12_fence_vtbl);
    return impl_from_ID3D12Fence1(iface1);
}

static HRESULT d3d12_fence_init(struct d3d12_fence *fence, struct d3d12_device *device,
        UINT64 initial_value, D3D12_FENCE_FLAGS flags)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkResult vr;
    HRESULT hr;

    fence->ID3D12Fence1_iface.lpVtbl = &d3d12_fence_vtbl;
    fence->internal_refcount = 1;
    fence->refcount = 1;

    fence->value = initial_value;
    fence->max_pending_value = initial_value;

    vkd3d_mutex_init(&fence->mutex);

    vkd3d_cond_init(&fence->null_event_cond);

    if ((fence->flags = flags))
        FIXME("Ignoring flags %#x.\n", flags);

    fence->events = NULL;
    fence->events_size = 0;
    fence->event_count = 0;

    fence->timeline_semaphore = VK_NULL_HANDLE;
    fence->timeline_value = 0;
    fence->pending_timeline_value = 0;
    if (device->vk_info.KHR_timeline_semaphore && (vr = vkd3d_create_timeline_semaphore(device, 0,
            &fence->timeline_semaphore)) < 0)
    {
        WARN("Failed to create timeline semaphore, vr %d.\n", vr);
        hr = hresult_from_vk_result(vr);
        goto fail_destroy_null_cond;
    }

    fence->semaphores = NULL;
    fence->semaphores_size = 0;
    fence->semaphore_count = 0;

    memset(fence->old_vk_fences, 0, sizeof(fence->old_vk_fences));

    if (FAILED(hr = vkd3d_private_store_init(&fence->private_store)))
    {
        goto fail_destroy_timeline_semaphore;
    }

    d3d12_device_add_ref(fence->device = device);

    return S_OK;

fail_destroy_timeline_semaphore:
    VK_CALL(vkDestroySemaphore(device->vk_device, fence->timeline_semaphore, NULL));
fail_destroy_null_cond:
    vkd3d_cond_destroy(&fence->null_event_cond);
    vkd3d_mutex_destroy(&fence->mutex);

    return hr;
}

HRESULT d3d12_fence_create(struct d3d12_device *device,
        uint64_t initial_value, D3D12_FENCE_FLAGS flags, struct d3d12_fence **fence)
{
    struct d3d12_fence *object;

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    d3d12_fence_init(object, device, initial_value, flags);

    TRACE("Created fence %p.\n", object);

    *fence = object;

    return S_OK;
}

VkResult vkd3d_create_timeline_semaphore(const struct d3d12_device *device, uint64_t initial_value,
        VkSemaphore *timeline_semaphore)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkSemaphoreTypeCreateInfoKHR type_info;
    VkSemaphoreCreateInfo info;

    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = &type_info;
    info.flags = 0;

    type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR;
    type_info.pNext = NULL;
    type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
    type_info.initialValue = initial_value;

    return VK_CALL(vkCreateSemaphore(device->vk_device, &info, NULL, timeline_semaphore));
}

/* Command buffers */
static void d3d12_command_list_mark_as_invalid(struct d3d12_command_list *list,
        const char *message, ...)
{
    va_list args;

    va_start(args, message);
    WARN("Command list %p is invalid: \"%s\".\n", list, vkd3d_dbg_vsprintf(message, args));
    va_end(args);

    list->is_valid = false;
}

static HRESULT d3d12_command_list_begin_command_buffer(struct d3d12_command_list *list)
{
    struct d3d12_device *device = list->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkCommandBufferBeginInfo begin_info;
    VkResult vr;

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = NULL;

    if ((vr = VK_CALL(vkBeginCommandBuffer(list->vk_command_buffer, &begin_info))) < 0)
    {
        WARN("Failed to begin command buffer, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    list->is_recording = true;
    list->is_valid = true;

    return S_OK;
}

static HRESULT d3d12_command_allocator_allocate_command_buffer(struct d3d12_command_allocator *allocator,
        struct d3d12_command_list *list)
{
    struct d3d12_device *device = allocator->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkCommandBufferAllocateInfo command_buffer_info;
    VkResult vr;
    HRESULT hr;

    TRACE("allocator %p, list %p.\n", allocator, list);

    if (allocator->current_command_list)
    {
        WARN("Command allocator is already in use.\n");
        return E_INVALIDARG;
    }

    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.pNext = NULL;
    command_buffer_info.commandPool = allocator->vk_command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;

    if ((vr = VK_CALL(vkAllocateCommandBuffers(device->vk_device, &command_buffer_info,
            &list->vk_command_buffer))) < 0)
    {
        WARN("Failed to allocate Vulkan command buffer, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    list->vk_queue_flags = allocator->vk_queue_flags;

    if (FAILED(hr = d3d12_command_list_begin_command_buffer(list)))
    {
        VK_CALL(vkFreeCommandBuffers(device->vk_device, allocator->vk_command_pool,
                1, &list->vk_command_buffer));
        return hr;
    }

    if (!vkd3d_array_reserve((void **)&allocator->command_buffers, &allocator->command_buffers_size,
            allocator->command_buffer_count + 1, sizeof(*allocator->command_buffers)))
    {
        WARN("Failed to add command buffer.\n");
        VK_CALL(vkFreeCommandBuffers(device->vk_device, allocator->vk_command_pool,
                1, &list->vk_command_buffer));
        return E_OUTOFMEMORY;
    }
    allocator->command_buffers[allocator->command_buffer_count++] = list->vk_command_buffer;

    allocator->current_command_list = list;

    return S_OK;
}

static void d3d12_command_allocator_remove_command_list(struct d3d12_command_allocator *allocator,
        const struct d3d12_command_list *list)
{
    if (allocator->current_command_list == list)
        allocator->current_command_list = NULL;
}

static bool d3d12_command_allocator_add_render_pass(struct d3d12_command_allocator *allocator, VkRenderPass pass)
{
    if (!vkd3d_array_reserve((void **)&allocator->passes, &allocator->passes_size,
            allocator->pass_count + 1, sizeof(*allocator->passes)))
        return false;

    allocator->passes[allocator->pass_count++] = pass;

    return true;
}

static bool d3d12_command_allocator_add_framebuffer(struct d3d12_command_allocator *allocator,
        VkFramebuffer framebuffer)
{
    if (!vkd3d_array_reserve((void **)&allocator->framebuffers, &allocator->framebuffers_size,
            allocator->framebuffer_count + 1, sizeof(*allocator->framebuffers)))
        return false;

    allocator->framebuffers[allocator->framebuffer_count++] = framebuffer;

    return true;
}

static bool d3d12_command_allocator_add_descriptor_pool(struct d3d12_command_allocator *allocator,
        VkDescriptorPool pool)
{
    if (!vkd3d_array_reserve((void **)&allocator->descriptor_pools, &allocator->descriptor_pools_size,
            allocator->descriptor_pool_count + 1, sizeof(*allocator->descriptor_pools)))
        return false;

    allocator->descriptor_pools[allocator->descriptor_pool_count++] = pool;

    return true;
}

static bool d3d12_command_allocator_add_view(struct d3d12_command_allocator *allocator,
        struct vkd3d_view *view)
{
    if (!vkd3d_array_reserve((void **)&allocator->views, &allocator->views_size,
            allocator->view_count + 1, sizeof(*allocator->views)))
        return false;

    vkd3d_view_incref(view);
    allocator->views[allocator->view_count++] = view;

    return true;
}

static bool d3d12_command_allocator_add_buffer_view(struct d3d12_command_allocator *allocator,
        VkBufferView view)
{
    if (!vkd3d_array_reserve((void **)&allocator->buffer_views, &allocator->buffer_views_size,
            allocator->buffer_view_count + 1, sizeof(*allocator->buffer_views)))
        return false;

    allocator->buffer_views[allocator->buffer_view_count++] = view;

    return true;
}

static bool d3d12_command_allocator_add_transfer_buffer(struct d3d12_command_allocator *allocator,
        const struct vkd3d_buffer *buffer)
{
    if (!vkd3d_array_reserve((void **)&allocator->transfer_buffers, &allocator->transfer_buffers_size,
            allocator->transfer_buffer_count + 1, sizeof(*allocator->transfer_buffers)))
        return false;

    allocator->transfer_buffers[allocator->transfer_buffer_count++] = *buffer;

    return true;
}

static VkDescriptorPool d3d12_command_allocator_allocate_descriptor_pool(
        struct d3d12_command_allocator *allocator)
{
    struct d3d12_device *device = allocator->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct VkDescriptorPoolCreateInfo pool_desc;
    VkDevice vk_device = device->vk_device;
    VkDescriptorPool vk_pool;
    VkResult vr;

    if (allocator->free_descriptor_pool_count > 0)
    {
        vk_pool = allocator->free_descriptor_pools[allocator->free_descriptor_pool_count - 1];
        allocator->free_descriptor_pools[allocator->free_descriptor_pool_count - 1] = VK_NULL_HANDLE;
        --allocator->free_descriptor_pool_count;
    }
    else
    {
        pool_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_desc.pNext = NULL;
        pool_desc.flags = 0;
        pool_desc.maxSets = 512;
        pool_desc.poolSizeCount = device->vk_pool_count;
        pool_desc.pPoolSizes = device->vk_pool_sizes;
        if ((vr = VK_CALL(vkCreateDescriptorPool(vk_device, &pool_desc, NULL, &vk_pool))) < 0)
        {
            ERR("Failed to create descriptor pool, vr %d.\n", vr);
            return VK_NULL_HANDLE;
        }
    }

    if (!(d3d12_command_allocator_add_descriptor_pool(allocator, vk_pool)))
    {
        ERR("Failed to add descriptor pool.\n");
        VK_CALL(vkDestroyDescriptorPool(vk_device, vk_pool, NULL));
        return VK_NULL_HANDLE;
    }

    return vk_pool;
}

static VkDescriptorSet d3d12_command_allocator_allocate_descriptor_set(
        struct d3d12_command_allocator *allocator, VkDescriptorSetLayout vk_set_layout,
        unsigned int variable_binding_size, bool unbounded)
{
    struct d3d12_device *device = allocator->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT set_size;
    struct VkDescriptorSetAllocateInfo set_desc;
    VkDevice vk_device = device->vk_device;
    VkDescriptorSet vk_descriptor_set;
    VkResult vr;

    if (!allocator->vk_descriptor_pool)
        allocator->vk_descriptor_pool = d3d12_command_allocator_allocate_descriptor_pool(allocator);
    if (!allocator->vk_descriptor_pool)
        return VK_NULL_HANDLE;

    set_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_desc.pNext = NULL;
    set_desc.descriptorPool = allocator->vk_descriptor_pool;
    set_desc.descriptorSetCount = 1;
    set_desc.pSetLayouts = &vk_set_layout;
    if (unbounded)
    {
        set_desc.pNext = &set_size;
        set_size.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
        set_size.pNext = NULL;
        set_size.descriptorSetCount = 1;
        set_size.pDescriptorCounts = &variable_binding_size;
    }
    if ((vr = VK_CALL(vkAllocateDescriptorSets(vk_device, &set_desc, &vk_descriptor_set))) >= 0)
        return vk_descriptor_set;

    allocator->vk_descriptor_pool = VK_NULL_HANDLE;
    if (vr == VK_ERROR_FRAGMENTED_POOL || vr == VK_ERROR_OUT_OF_POOL_MEMORY_KHR)
        allocator->vk_descriptor_pool = d3d12_command_allocator_allocate_descriptor_pool(allocator);
    if (!allocator->vk_descriptor_pool)
    {
        ERR("Failed to allocate descriptor set, vr %d.\n", vr);
        return VK_NULL_HANDLE;
    }

    set_desc.descriptorPool = allocator->vk_descriptor_pool;
    if ((vr = VK_CALL(vkAllocateDescriptorSets(vk_device, &set_desc, &vk_descriptor_set))) < 0)
    {
        FIXME("Failed to allocate descriptor set from a new pool, vr %d.\n", vr);
        return VK_NULL_HANDLE;
    }

    return vk_descriptor_set;
}

static void d3d12_command_list_allocator_destroyed(struct d3d12_command_list *list)
{
    TRACE("list %p.\n", list);

    list->allocator = NULL;
    list->vk_command_buffer = VK_NULL_HANDLE;
}

static void vkd3d_buffer_destroy(struct vkd3d_buffer *buffer, struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    VK_CALL(vkFreeMemory(device->vk_device, buffer->vk_memory, NULL));
    VK_CALL(vkDestroyBuffer(device->vk_device, buffer->vk_buffer, NULL));
}

static void d3d12_command_allocator_free_resources(struct d3d12_command_allocator *allocator,
        bool keep_reusable_resources)
{
    struct d3d12_device *device = allocator->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    unsigned int i, j;

    allocator->vk_descriptor_pool = VK_NULL_HANDLE;

    if (keep_reusable_resources)
    {
        if (vkd3d_array_reserve((void **)&allocator->free_descriptor_pools,
                &allocator->free_descriptor_pools_size,
                allocator->free_descriptor_pool_count + allocator->descriptor_pool_count,
                sizeof(*allocator->free_descriptor_pools)))
        {
            for (i = 0, j = allocator->free_descriptor_pool_count; i < allocator->descriptor_pool_count; ++i, ++j)
            {
                VK_CALL(vkResetDescriptorPool(device->vk_device, allocator->descriptor_pools[i], 0));
                allocator->free_descriptor_pools[j] = allocator->descriptor_pools[i];
            }
            allocator->free_descriptor_pool_count += allocator->descriptor_pool_count;
            allocator->descriptor_pool_count = 0;
        }
    }
    else
    {
        for (i = 0; i < allocator->free_descriptor_pool_count; ++i)
        {
            VK_CALL(vkDestroyDescriptorPool(device->vk_device, allocator->free_descriptor_pools[i], NULL));
        }
        allocator->free_descriptor_pool_count = 0;
    }

    for (i = 0; i < allocator->transfer_buffer_count; ++i)
    {
        vkd3d_buffer_destroy(&allocator->transfer_buffers[i], device);
    }
    allocator->transfer_buffer_count = 0;

    for (i = 0; i < allocator->buffer_view_count; ++i)
    {
        VK_CALL(vkDestroyBufferView(device->vk_device, allocator->buffer_views[i], NULL));
    }
    allocator->buffer_view_count = 0;

    for (i = 0; i < allocator->view_count; ++i)
    {
        vkd3d_view_decref(allocator->views[i], device);
    }
    allocator->view_count = 0;

    for (i = 0; i < allocator->descriptor_pool_count; ++i)
    {
        VK_CALL(vkDestroyDescriptorPool(device->vk_device, allocator->descriptor_pools[i], NULL));
    }
    allocator->descriptor_pool_count = 0;

    for (i = 0; i < allocator->framebuffer_count; ++i)
    {
        VK_CALL(vkDestroyFramebuffer(device->vk_device, allocator->framebuffers[i], NULL));
    }
    allocator->framebuffer_count = 0;

    for (i = 0; i < allocator->pass_count; ++i)
    {
        VK_CALL(vkDestroyRenderPass(device->vk_device, allocator->passes[i], NULL));
    }
    allocator->pass_count = 0;
}

/* ID3D12CommandAllocator */
static inline struct d3d12_command_allocator *impl_from_ID3D12CommandAllocator(ID3D12CommandAllocator *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_command_allocator, ID3D12CommandAllocator_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_allocator_QueryInterface(ID3D12CommandAllocator *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D12CommandAllocator)
            || IsEqualGUID(riid, &IID_ID3D12Pageable)
            || IsEqualGUID(riid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(riid, &IID_ID3D12Object)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D12CommandAllocator_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_command_allocator_AddRef(ID3D12CommandAllocator *iface)
{
    struct d3d12_command_allocator *allocator = impl_from_ID3D12CommandAllocator(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&allocator->refcount);

    TRACE("%p increasing refcount to %u.\n", allocator, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d12_command_allocator_Release(ID3D12CommandAllocator *iface)
{
    struct d3d12_command_allocator *allocator = impl_from_ID3D12CommandAllocator(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&allocator->refcount);

    TRACE("%p decreasing refcount to %u.\n", allocator, refcount);

    if (!refcount)
    {
        struct d3d12_device *device = allocator->device;
        const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

        vkd3d_private_store_destroy(&allocator->private_store);

        if (allocator->current_command_list)
            d3d12_command_list_allocator_destroyed(allocator->current_command_list);

        d3d12_command_allocator_free_resources(allocator, false);
        vkd3d_free(allocator->transfer_buffers);
        vkd3d_free(allocator->buffer_views);
        vkd3d_free(allocator->views);
        vkd3d_free(allocator->descriptor_pools);
        vkd3d_free(allocator->free_descriptor_pools);
        vkd3d_free(allocator->framebuffers);
        vkd3d_free(allocator->passes);

        /* All command buffers are implicitly freed when a pool is destroyed. */
        vkd3d_free(allocator->command_buffers);
        VK_CALL(vkDestroyCommandPool(device->vk_device, allocator->vk_command_pool, NULL));

        vkd3d_free(allocator);

        d3d12_device_release(device);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_allocator_GetPrivateData(ID3D12CommandAllocator *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_command_allocator *allocator = impl_from_ID3D12CommandAllocator(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&allocator->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_allocator_SetPrivateData(ID3D12CommandAllocator *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_command_allocator *allocator = impl_from_ID3D12CommandAllocator(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&allocator->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_allocator_SetPrivateDataInterface(ID3D12CommandAllocator *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_command_allocator *allocator = impl_from_ID3D12CommandAllocator(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&allocator->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_allocator_SetName(ID3D12CommandAllocator *iface, const WCHAR *name)
{
    struct d3d12_command_allocator *allocator = impl_from_ID3D12CommandAllocator(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, allocator->device->wchar_size));

    return vkd3d_set_vk_object_name(allocator->device, (uint64_t)allocator->vk_command_pool,
            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, name);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_allocator_GetDevice(ID3D12CommandAllocator *iface, REFIID iid, void **device)
{
    struct d3d12_command_allocator *allocator = impl_from_ID3D12CommandAllocator(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(allocator->device, iid, device);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_allocator_Reset(ID3D12CommandAllocator *iface)
{
    struct d3d12_command_allocator *allocator = impl_from_ID3D12CommandAllocator(iface);
    const struct vkd3d_vk_device_procs *vk_procs;
    struct d3d12_command_list *list;
    struct d3d12_device *device;
    VkResult vr;

    TRACE("iface %p.\n", iface);

    if ((list = allocator->current_command_list))
    {
        if (list->is_recording)
        {
            WARN("A command list using this allocator is in the recording state.\n");
            return E_FAIL;
        }

        TRACE("Resetting command list %p.\n", list);
    }

    device = allocator->device;
    vk_procs = &device->vk_procs;

    d3d12_command_allocator_free_resources(allocator, true);
    if (allocator->command_buffer_count)
    {
        VK_CALL(vkFreeCommandBuffers(device->vk_device, allocator->vk_command_pool,
                allocator->command_buffer_count, allocator->command_buffers));
        allocator->command_buffer_count = 0;
    }

    /* The intent here is to recycle memory, so do not use RELEASE_RESOURCES_BIT here. */
    if ((vr = VK_CALL(vkResetCommandPool(device->vk_device, allocator->vk_command_pool, 0))))
    {
        WARN("Resetting command pool failed, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    return S_OK;
}

static const struct ID3D12CommandAllocatorVtbl d3d12_command_allocator_vtbl =
{
    /* IUnknown methods */
    d3d12_command_allocator_QueryInterface,
    d3d12_command_allocator_AddRef,
    d3d12_command_allocator_Release,
    /* ID3D12Object methods */
    d3d12_command_allocator_GetPrivateData,
    d3d12_command_allocator_SetPrivateData,
    d3d12_command_allocator_SetPrivateDataInterface,
    d3d12_command_allocator_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_command_allocator_GetDevice,
    /* ID3D12CommandAllocator methods */
    d3d12_command_allocator_Reset,
};

static struct d3d12_command_allocator *unsafe_impl_from_ID3D12CommandAllocator(ID3D12CommandAllocator *iface)
{
    if (!iface)
        return NULL;
    VKD3D_ASSERT(iface->lpVtbl == &d3d12_command_allocator_vtbl);
    return impl_from_ID3D12CommandAllocator(iface);
}

struct vkd3d_queue *d3d12_device_get_vkd3d_queue(struct d3d12_device *device,
        D3D12_COMMAND_LIST_TYPE type)
{
    switch (type)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            return device->direct_queue;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return device->compute_queue;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            return device->copy_queue;
        default:
            FIXME("Unhandled command list type %#x.\n", type);
            return NULL;
    }
}

static HRESULT d3d12_command_allocator_init(struct d3d12_command_allocator *allocator,
        struct d3d12_device *device, D3D12_COMMAND_LIST_TYPE type)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkCommandPoolCreateInfo command_pool_info;
    struct vkd3d_queue *queue;
    VkResult vr;
    HRESULT hr;

    if (FAILED(hr = vkd3d_private_store_init(&allocator->private_store)))
        return hr;

    if (!(queue = d3d12_device_get_vkd3d_queue(device, type)))
        queue = device->direct_queue;

    allocator->ID3D12CommandAllocator_iface.lpVtbl = &d3d12_command_allocator_vtbl;
    allocator->refcount = 1;

    allocator->type = type;
    allocator->vk_queue_flags = queue->vk_queue_flags;

    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = NULL;
    /* Do not use RESET_COMMAND_BUFFER_BIT. This allows the CommandPool to be a D3D12-style command pool.
     * Memory is owned by the pool and CommandBuffers become lightweight handles,
     * assuming a half-decent driver implementation. */
    command_pool_info.flags = 0;
    command_pool_info.queueFamilyIndex = queue->vk_family_index;

    if ((vr = VK_CALL(vkCreateCommandPool(device->vk_device, &command_pool_info, NULL,
            &allocator->vk_command_pool))) < 0)
    {
        WARN("Failed to create Vulkan command pool, vr %d.\n", vr);
        vkd3d_private_store_destroy(&allocator->private_store);
        return hresult_from_vk_result(vr);
    }

    allocator->vk_descriptor_pool = VK_NULL_HANDLE;

    allocator->free_descriptor_pools = NULL;
    allocator->free_descriptor_pools_size = 0;
    allocator->free_descriptor_pool_count = 0;

    allocator->passes = NULL;
    allocator->passes_size = 0;
    allocator->pass_count = 0;

    allocator->framebuffers = NULL;
    allocator->framebuffers_size = 0;
    allocator->framebuffer_count = 0;

    allocator->descriptor_pools = NULL;
    allocator->descriptor_pools_size = 0;
    allocator->descriptor_pool_count = 0;

    allocator->views = NULL;
    allocator->views_size = 0;
    allocator->view_count = 0;

    allocator->buffer_views = NULL;
    allocator->buffer_views_size = 0;
    allocator->buffer_view_count = 0;

    allocator->transfer_buffers = NULL;
    allocator->transfer_buffers_size = 0;
    allocator->transfer_buffer_count = 0;

    allocator->command_buffers = NULL;
    allocator->command_buffers_size = 0;
    allocator->command_buffer_count = 0;

    allocator->current_command_list = NULL;

    d3d12_device_add_ref(allocator->device = device);

    return S_OK;
}

HRESULT d3d12_command_allocator_create(struct d3d12_device *device,
        D3D12_COMMAND_LIST_TYPE type, struct d3d12_command_allocator **allocator)
{
    struct d3d12_command_allocator *object;
    HRESULT hr;

    if (!(D3D12_COMMAND_LIST_TYPE_DIRECT <= type && type <= D3D12_COMMAND_LIST_TYPE_COPY))
    {
        WARN("Invalid type %#x.\n", type);
        return E_INVALIDARG;
    }

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_command_allocator_init(object, device, type)))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created command allocator %p.\n", object);

    *allocator = object;

    return S_OK;
}

static void d3d12_command_signature_incref(struct d3d12_command_signature *signature)
{
    vkd3d_atomic_increment_u32(&signature->internal_refcount);
}

static void d3d12_command_signature_decref(struct d3d12_command_signature *signature)
{
    unsigned int refcount = vkd3d_atomic_decrement_u32(&signature->internal_refcount);

    if (!refcount)
    {
        struct d3d12_device *device = signature->device;

        vkd3d_private_store_destroy(&signature->private_store);

        vkd3d_free((void *)signature->desc.pArgumentDescs);
        vkd3d_free(signature);

        d3d12_device_release(device);
    }
}

/* ID3D12CommandList */
static inline struct d3d12_command_list *impl_from_ID3D12GraphicsCommandList6(ID3D12GraphicsCommandList6 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_command_list, ID3D12GraphicsCommandList6_iface);
}

static void d3d12_command_list_invalidate_current_framebuffer(struct d3d12_command_list *list)
{
    list->current_framebuffer = VK_NULL_HANDLE;
}

static void d3d12_command_list_invalidate_current_pipeline(struct d3d12_command_list *list)
{
    list->current_pipeline = VK_NULL_HANDLE;
}

static void d3d12_command_list_end_current_render_pass(struct d3d12_command_list *list)
{
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;

    if (list->xfb_enabled)
    {
        VK_CALL(vkCmdEndTransformFeedbackEXT(list->vk_command_buffer, 0, ARRAY_SIZE(list->so_counter_buffers),
                list->so_counter_buffers, list->so_counter_buffer_offsets));
    }

    if (list->current_render_pass)
        VK_CALL(vkCmdEndRenderPass(list->vk_command_buffer));

    list->current_render_pass = VK_NULL_HANDLE;

    if (list->xfb_enabled)
    {
        VkMemoryBarrier vk_barrier;

        /* We need a barrier between pause and resume. */
        vk_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        vk_barrier.pNext = NULL;
        vk_barrier.srcAccessMask = VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT;
        vk_barrier.dstAccessMask = VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT;
        VK_CALL(vkCmdPipelineBarrier(list->vk_command_buffer,
                VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0,
                1, &vk_barrier, 0, NULL, 0, NULL));

        list->xfb_enabled = false;
    }
}

static void d3d12_command_list_invalidate_current_render_pass(struct d3d12_command_list *list)
{
    d3d12_command_list_end_current_render_pass(list);
}

static void d3d12_command_list_invalidate_bindings(struct d3d12_command_list *list,
        struct d3d12_pipeline_state *state)
{
    if (state && state->uav_counters.binding_count)
    {
        enum vkd3d_pipeline_bind_point bind_point = (enum vkd3d_pipeline_bind_point)state->vk_bind_point;
        struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];

        vkd3d_array_reserve((void **)&bindings->vk_uav_counter_views, &bindings->vk_uav_counter_views_size,
                state->uav_counters.binding_count, sizeof(*bindings->vk_uav_counter_views));
        memset(bindings->vk_uav_counter_views, 0,
                state->uav_counters.binding_count * sizeof(*bindings->vk_uav_counter_views));
        bindings->uav_counters_dirty = true;
    }
}

static void d3d12_command_list_invalidate_root_parameters(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];

    if (!bindings->root_signature)
        return;

    bindings->descriptor_set_count = 0;
    bindings->descriptor_table_dirty_mask = bindings->descriptor_table_active_mask & bindings->root_signature->descriptor_table_mask;
    bindings->push_descriptor_dirty_mask = bindings->push_descriptor_active_mask & bindings->root_signature->push_descriptor_mask;
    bindings->cbv_srv_uav_heap_id = 0;
    bindings->sampler_heap_id = 0;
}

static bool vk_barrier_parameters_from_d3d12_resource_state(unsigned int state, unsigned int stencil_state,
        const struct d3d12_resource *resource, VkQueueFlags vk_queue_flags, const struct vkd3d_vulkan_info *vk_info,
        VkAccessFlags *access_mask, VkPipelineStageFlags *stage_flags, VkImageLayout *image_layout,
        struct d3d12_device *device)
{
    bool is_swapchain_image = resource && (resource->flags & VKD3D_RESOURCE_PRESENT_STATE_TRANSITION);
    VkPipelineStageFlags queue_shader_stages = 0;

    if (vk_queue_flags & VK_QUEUE_GRAPHICS_BIT)
    {
        queue_shader_stages |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        if (device->vk_info.geometry_shaders)
            queue_shader_stages |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
        if (device->vk_info.tessellation_shaders)
            queue_shader_stages |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
                    | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    }
    if (vk_queue_flags & VK_QUEUE_COMPUTE_BIT)
        queue_shader_stages |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    switch (state)
    {
        case D3D12_RESOURCE_STATE_COMMON: /* D3D12_RESOURCE_STATE_PRESENT */
            /* The COMMON state is used for ownership transfer between
             * DIRECT/COMPUTE and COPY queues. Additionally, a texture has to
             * be in the COMMON state to be accessed by CPU. Moreover,
             * resources can be implicitly promoted to other states out of the
             * COMMON state, and the resource state can decay to the COMMON
             * state when GPU finishes execution of a command list. */
            if (is_swapchain_image)
            {
                if (resource->present_state != D3D12_RESOURCE_STATE_PRESENT)
                    return vk_barrier_parameters_from_d3d12_resource_state(resource->present_state, 0,
                            resource, vk_queue_flags, vk_info, access_mask, stage_flags, image_layout, device);

                *access_mask = VK_ACCESS_MEMORY_READ_BIT;
                *stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                if (image_layout)
                    *image_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                return true;
            }

            *access_mask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
            *stage_flags = VK_PIPELINE_STAGE_HOST_BIT;
            if (image_layout)
                *image_layout = VK_IMAGE_LAYOUT_GENERAL;
            return true;

        /* Handle write states. */
        case D3D12_RESOURCE_STATE_RENDER_TARGET:
            *access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                    | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            *stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            if (image_layout)
                *image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            return true;

        case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
            *access_mask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            *stage_flags = queue_shader_stages;
            if (image_layout)
                *image_layout = VK_IMAGE_LAYOUT_GENERAL;
            return true;

        case D3D12_RESOURCE_STATE_DEPTH_WRITE:
            *access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            *stage_flags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                    | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            if (image_layout)
            {
                if (!stencil_state || (stencil_state & D3D12_RESOURCE_STATE_DEPTH_WRITE))
                    *image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                else
                    *image_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            }
            return true;

        case D3D12_RESOURCE_STATE_COPY_DEST:
        case D3D12_RESOURCE_STATE_RESOLVE_DEST:
            *access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
            *stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            if (image_layout)
                *image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            return true;

        case D3D12_RESOURCE_STATE_STREAM_OUT:
            *access_mask = VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT
                    | VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT
                    | VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT;
            *stage_flags = VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT
                    | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            if (image_layout)
                *image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            return true;

        /* Set the Vulkan image layout for read-only states. */
        case D3D12_RESOURCE_STATE_DEPTH_READ:
        case D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
            *access_mask = 0;
            *stage_flags = 0;
            if (image_layout)
            {
                if (stencil_state & D3D12_RESOURCE_STATE_DEPTH_WRITE)
                {
                    *image_layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
                    *access_mask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                }
                else
                {
                    *image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                }
            }
            break;

        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
        case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
            *access_mask = 0;
            *stage_flags = 0;
            if (image_layout)
                *image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;

        case D3D12_RESOURCE_STATE_COPY_SOURCE:
        case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:
            *access_mask = 0;
            *stage_flags = 0;
            if (image_layout)
                *image_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            break;

        default:
            *access_mask = 0;
            *stage_flags = 0;
            if (image_layout)
                *image_layout = VK_IMAGE_LAYOUT_GENERAL;
            break;
    }

    /* Handle read-only states. */
    VKD3D_ASSERT(!is_write_resource_state(state));

    if (state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
    {
        *access_mask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
                | VK_ACCESS_UNIFORM_READ_BIT;
        *stage_flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
                | queue_shader_stages;
        state &= ~D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }

    if (state & D3D12_RESOURCE_STATE_INDEX_BUFFER)
    {
        *access_mask |= VK_ACCESS_INDEX_READ_BIT;
        *stage_flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        state &= ~D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }

    if (state & D3D12_RESOURCE_STATE_DEPTH_READ)
    {
        *access_mask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        *stage_flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        state &= ~D3D12_RESOURCE_STATE_DEPTH_READ;
    }

    if (state & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
    {
        *access_mask |= VK_ACCESS_SHADER_READ_BIT;
        *stage_flags |= (queue_shader_stages & ~VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        state &= ~D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    if (state & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    {
        *access_mask |= VK_ACCESS_SHADER_READ_BIT;
        *stage_flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        state &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    if (state & D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT) /* D3D12_RESOURCE_STATE_PREDICATION */
    {
        *access_mask |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        *stage_flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        if (vk_info->EXT_conditional_rendering)
        {
            *access_mask |= VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
            *stage_flags |= VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
        }
        state &= ~D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    }

    if (state & (D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_RESOLVE_SOURCE))
    {
        *access_mask |= VK_ACCESS_TRANSFER_READ_BIT;
        *stage_flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
        state &= ~(D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
    }

    if (state)
    {
        WARN("Invalid resource state %#x.\n", state);
        return false;
    }
    return true;
}

static void d3d12_command_list_transition_resource_to_initial_state(struct d3d12_command_list *list,
        struct d3d12_resource *resource)
{
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct vkd3d_vulkan_info *vk_info = &list->device->vk_info;
    VkPipelineStageFlags src_stage_mask, dst_stage_mask;
    VkImageMemoryBarrier barrier;

    VKD3D_ASSERT(d3d12_resource_is_texture(resource));

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = NULL;

    /* vkQueueSubmit() defines a memory dependency with prior host writes. */
    src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    barrier.srcAccessMask = 0;
    barrier.oldLayout = d3d12_resource_is_cpu_accessible(resource) ?
            VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

    if (!vk_barrier_parameters_from_d3d12_resource_state(resource->initial_state, 0,
            resource, list->vk_queue_flags, vk_info, &barrier.dstAccessMask,
            &dst_stage_mask, &barrier.newLayout, list->device))
    {
        FIXME("Unhandled state %#x.\n", resource->initial_state);
        return;
    }

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = resource->u.vk_image;
    barrier.subresourceRange.aspectMask = resource->format->vk_aspect_mask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    TRACE("Initial state %#x transition for resource %p (old layout %#x, new layout %#x).\n",
            resource->initial_state, resource, barrier.oldLayout, barrier.newLayout);

    VK_CALL(vkCmdPipelineBarrier(list->vk_command_buffer, src_stage_mask, dst_stage_mask, 0,
            0, NULL, 0, NULL, 1, &barrier));
}

static void d3d12_command_list_track_resource_usage(struct d3d12_command_list *list,
        struct d3d12_resource *resource)
{
    if (resource->flags & VKD3D_RESOURCE_INITIAL_STATE_TRANSITION)
    {
        d3d12_command_list_end_current_render_pass(list);

        d3d12_command_list_transition_resource_to_initial_state(list, resource);
        resource->flags &= ~VKD3D_RESOURCE_INITIAL_STATE_TRANSITION;
    }
}

static HRESULT STDMETHODCALLTYPE d3d12_command_list_QueryInterface(ID3D12GraphicsCommandList6 *iface,
        REFIID iid, void **object)
{
    TRACE("iface %p, iid %s, object %p.\n", iface, debugstr_guid(iid), object);

    if (IsEqualGUID(iid, &IID_ID3D12GraphicsCommandList6)
            || IsEqualGUID(iid, &IID_ID3D12GraphicsCommandList5)
            || IsEqualGUID(iid, &IID_ID3D12GraphicsCommandList4)
            || IsEqualGUID(iid, &IID_ID3D12GraphicsCommandList3)
            || IsEqualGUID(iid, &IID_ID3D12GraphicsCommandList2)
            || IsEqualGUID(iid, &IID_ID3D12GraphicsCommandList1)
            || IsEqualGUID(iid, &IID_ID3D12GraphicsCommandList)
            || IsEqualGUID(iid, &IID_ID3D12CommandList)
            || IsEqualGUID(iid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(iid, &IID_ID3D12Object)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID3D12GraphicsCommandList6_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_command_list_AddRef(ID3D12GraphicsCommandList6 *iface)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&list->refcount);

    TRACE("%p increasing refcount to %u.\n", list, refcount);

    return refcount;
}

static void vkd3d_pipeline_bindings_cleanup(struct vkd3d_pipeline_bindings *bindings)
{
    vkd3d_free(bindings->vk_uav_counter_views);
}

static ULONG STDMETHODCALLTYPE d3d12_command_list_Release(ID3D12GraphicsCommandList6 *iface)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&list->refcount);

    TRACE("%p decreasing refcount to %u.\n", list, refcount);

    if (!refcount)
    {
        struct d3d12_device *device = list->device;

        vkd3d_private_store_destroy(&list->private_store);

        /* When command pool is destroyed, all command buffers are implicitly freed. */
        if (list->allocator)
            d3d12_command_allocator_remove_command_list(list->allocator, list);

        vkd3d_pipeline_bindings_cleanup(&list->pipeline_bindings[VKD3D_PIPELINE_BIND_POINT_COMPUTE]);
        vkd3d_pipeline_bindings_cleanup(&list->pipeline_bindings[VKD3D_PIPELINE_BIND_POINT_GRAPHICS]);

        vkd3d_free(list);

        d3d12_device_release(device);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_list_GetPrivateData(ID3D12GraphicsCommandList6 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&list->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_list_SetPrivateData(ID3D12GraphicsCommandList6 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&list->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_list_SetPrivateDataInterface(ID3D12GraphicsCommandList6 *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&list->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_list_SetName(ID3D12GraphicsCommandList6 *iface, const WCHAR *name)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, list->device->wchar_size));

    return name ? S_OK : E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_list_GetDevice(ID3D12GraphicsCommandList6 *iface,
        REFIID iid, void **device)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(list->device, iid, device);
}

static D3D12_COMMAND_LIST_TYPE STDMETHODCALLTYPE d3d12_command_list_GetType(ID3D12GraphicsCommandList6 *iface)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p.\n", iface);

    return list->type;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_list_Close(ID3D12GraphicsCommandList6 *iface)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_vk_device_procs *vk_procs;
    VkResult vr;

    TRACE("iface %p.\n", iface);

    if (!list->is_recording)
    {
        WARN("Command list is not in the recording state.\n");
        return E_FAIL;
    }

    vk_procs = &list->device->vk_procs;

    d3d12_command_list_end_current_render_pass(list);
    if (list->is_predicated)
        VK_CALL(vkCmdEndConditionalRenderingEXT(list->vk_command_buffer));

    if ((vr = VK_CALL(vkEndCommandBuffer(list->vk_command_buffer))) < 0)
    {
        WARN("Failed to end command buffer, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    if (list->allocator)
    {
        d3d12_command_allocator_remove_command_list(list->allocator, list);
        list->allocator = NULL;
    }

    list->is_recording = false;
    list->has_depth_bounds = false;

    if (!list->is_valid)
    {
        WARN("Error occurred during command list recording.\n");
        return E_INVALIDARG;
    }

    return S_OK;
}

static void d3d12_command_list_reset_state(struct d3d12_command_list *list,
        ID3D12PipelineState *initial_pipeline_state)
{
    ID3D12GraphicsCommandList6 *iface = &list->ID3D12GraphicsCommandList6_iface;

    memset(list->strides, 0, sizeof(list->strides));
    list->primitive_topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

    list->index_buffer_format = DXGI_FORMAT_UNKNOWN;

    memset(list->rtvs, 0, sizeof(list->rtvs));
    list->dsv = VK_NULL_HANDLE;
    list->dsv_format = VK_FORMAT_UNDEFINED;
    list->fb_width = 0;
    list->fb_height = 0;
    list->fb_layer_count = 0;

    list->xfb_enabled = false;
    list->has_depth_bounds = false;
    list->is_predicated = false;

    list->current_framebuffer = VK_NULL_HANDLE;
    list->current_pipeline = VK_NULL_HANDLE;
    list->pso_render_pass = VK_NULL_HANDLE;
    list->current_render_pass = VK_NULL_HANDLE;

    vkd3d_pipeline_bindings_cleanup(&list->pipeline_bindings[VKD3D_PIPELINE_BIND_POINT_COMPUTE]);
    vkd3d_pipeline_bindings_cleanup(&list->pipeline_bindings[VKD3D_PIPELINE_BIND_POINT_GRAPHICS]);
    memset(list->pipeline_bindings, 0, sizeof(list->pipeline_bindings));
    list->pipeline_bindings[VKD3D_PIPELINE_BIND_POINT_GRAPHICS].vk_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    list->pipeline_bindings[VKD3D_PIPELINE_BIND_POINT_COMPUTE].vk_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

    list->state = NULL;

    memset(list->so_counter_buffers, 0, sizeof(list->so_counter_buffers));
    memset(list->so_counter_buffer_offsets, 0, sizeof(list->so_counter_buffer_offsets));

    list->descriptor_heap_count = 0;

    ID3D12GraphicsCommandList6_SetPipelineState(iface, initial_pipeline_state);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_list_Reset(ID3D12GraphicsCommandList6 *iface,
        ID3D12CommandAllocator *allocator, ID3D12PipelineState *initial_pipeline_state)
{
    struct d3d12_command_allocator *allocator_impl = unsafe_impl_from_ID3D12CommandAllocator(allocator);
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    HRESULT hr;

    TRACE("iface %p, allocator %p, initial_pipeline_state %p.\n",
            iface, allocator, initial_pipeline_state);

    if (!allocator_impl)
    {
        WARN("Command allocator is NULL.\n");
        return E_INVALIDARG;
    }

    if (list->is_recording)
    {
        WARN("Command list is in the recording state.\n");
        return E_FAIL;
    }

    if (SUCCEEDED(hr = d3d12_command_allocator_allocate_command_buffer(allocator_impl, list)))
    {
        list->allocator = allocator_impl;
        d3d12_command_list_reset_state(list, initial_pipeline_state);
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d12_command_list_ClearState(ID3D12GraphicsCommandList6 *iface,
        ID3D12PipelineState *pipeline_state)
{
    FIXME("iface %p, pipeline_state %p stub!\n", iface, pipeline_state);
}

static bool d3d12_command_list_has_depth_stencil_view(struct d3d12_command_list *list)
{
    struct d3d12_graphics_pipeline_state *graphics;

    VKD3D_ASSERT(d3d12_pipeline_state_is_graphics(list->state));
    graphics = &list->state->u.graphics;

    return graphics->dsv_format || (d3d12_pipeline_state_has_unknown_dsv_format(list->state) && list->dsv_format);
}

static void d3d12_command_list_get_fb_extent(struct d3d12_command_list *list,
        uint32_t *width, uint32_t *height, uint32_t *layer_count)
{
    struct d3d12_graphics_pipeline_state *graphics = &list->state->u.graphics;
    struct d3d12_device *device = list->device;

    if (graphics->rt_count || d3d12_command_list_has_depth_stencil_view(list))
    {
        *width = list->fb_width;
        *height = list->fb_height;
        if (layer_count)
            *layer_count = list->fb_layer_count;
    }
    else
    {
        *width = device->vk_info.device_limits.maxFramebufferWidth;
        *height = device->vk_info.device_limits.maxFramebufferHeight;
        if (layer_count)
            *layer_count = 1;
    }
}

static bool d3d12_command_list_update_current_framebuffer(struct d3d12_command_list *list)
{
    struct d3d12_device *device = list->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkImageView views[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT + 1];
    struct d3d12_graphics_pipeline_state *graphics;
    struct VkFramebufferCreateInfo fb_desc;
    VkFramebuffer vk_framebuffer;
    unsigned int view_count;
    unsigned int i;
    VkResult vr;

    if (list->current_framebuffer != VK_NULL_HANDLE)
        return true;

    graphics = &list->state->u.graphics;

    for (i = 0, view_count = 0; i < graphics->rt_count; ++i)
    {
        if (graphics->null_attachment_mask & (1u << i))
        {
            if (list->rtvs[i])
                WARN("Expected NULL RTV for attachment %u.\n", i);
            continue;
        }

        if (!list->rtvs[i])
        {
            FIXME("Invalid RTV for attachment %u.\n", i);
            return false;
        }

        views[view_count++] = list->rtvs[i];
    }

    if (d3d12_command_list_has_depth_stencil_view(list))
    {
        if (!(views[view_count++] = list->dsv))
        {
            FIXME("Invalid DSV.\n");
            return false;
        }
    }

    fb_desc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_desc.pNext = NULL;
    fb_desc.flags = 0;
    fb_desc.renderPass = list->pso_render_pass;
    fb_desc.attachmentCount = view_count;
    fb_desc.pAttachments = views;
    d3d12_command_list_get_fb_extent(list, &fb_desc.width, &fb_desc.height, &fb_desc.layers);
    if ((vr = VK_CALL(vkCreateFramebuffer(device->vk_device, &fb_desc, NULL, &vk_framebuffer))) < 0)
    {
        WARN("Failed to create Vulkan framebuffer, vr %d.\n", vr);
        return false;
    }

    if (!d3d12_command_allocator_add_framebuffer(list->allocator, vk_framebuffer))
    {
        WARN("Failed to add framebuffer.\n");
        VK_CALL(vkDestroyFramebuffer(device->vk_device, vk_framebuffer, NULL));
        return false;
    }

    list->current_framebuffer = vk_framebuffer;

    return true;
}

static bool d3d12_command_list_update_compute_pipeline(struct d3d12_command_list *list)
{
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;

    vkd3d_cond_signal(&list->device->worker_cond);

    if (list->current_pipeline != VK_NULL_HANDLE)
        return true;

    if (!d3d12_pipeline_state_is_compute(list->state))
    {
        WARN("Pipeline state %p is not a compute pipeline.\n", list->state);
        return false;
    }

    VK_CALL(vkCmdBindPipeline(list->vk_command_buffer, list->state->vk_bind_point, list->state->u.compute.vk_pipeline));
    list->current_pipeline = list->state->u.compute.vk_pipeline;

    return true;
}

static bool d3d12_command_list_update_graphics_pipeline(struct d3d12_command_list *list)
{
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    VkRenderPass vk_render_pass;
    VkPipeline vk_pipeline;

    vkd3d_cond_signal(&list->device->worker_cond);

    if (list->current_pipeline != VK_NULL_HANDLE)
        return true;

    if (!d3d12_pipeline_state_is_graphics(list->state))
    {
        WARN("Pipeline state %p is not a graphics pipeline.\n", list->state);
        return false;
    }

    if (!(vk_pipeline = d3d12_pipeline_state_get_or_create_pipeline(list->state,
            list->primitive_topology, list->strides, list->dsv_format, &vk_render_pass)))
        return false;

    /* The render pass cache ensures that we use the same Vulkan render pass
     * object for compatible render passes. */
    if (list->pso_render_pass != vk_render_pass)
    {
        list->pso_render_pass = vk_render_pass;
        d3d12_command_list_invalidate_current_framebuffer(list);
        d3d12_command_list_invalidate_current_render_pass(list);
    }

    VK_CALL(vkCmdBindPipeline(list->vk_command_buffer, list->state->vk_bind_point, vk_pipeline));
    list->current_pipeline = vk_pipeline;

    return true;
}

static void d3d12_command_list_prepare_descriptors(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    unsigned int variable_binding_size, unbounded_offset, table_index, heap_size, i;
    const struct d3d12_root_signature *root_signature = bindings->root_signature;
    const struct d3d12_descriptor_set_layout *layout;
    const struct d3d12_desc *base_descriptor;
    VkDescriptorSet vk_descriptor_set;

    if (bindings->descriptor_set_count && !bindings->in_use)
        return;

    /* We cannot modify bound descriptor sets. We need a new descriptor set if
     * we are about to update resource bindings.
     *
     * The Vulkan spec says:
     *
     *   "The descriptor set contents bound by a call to
     *   vkCmdBindDescriptorSets may be consumed during host execution of the
     *   command, or during shader execution of the resulting draws, or any
     *   time in between. Thus, the contents must not be altered (overwritten
     *   by an update command, or freed) between when the command is recorded
     *   and when the command completes executing on the queue."
     */
    bindings->descriptor_set_count = 0;
    for (i = root_signature->main_set; i < root_signature->vk_set_count; ++i)
    {
        layout = &root_signature->descriptor_set_layouts[i];
        unbounded_offset = layout->unbounded_offset;
        table_index = layout->table_index;
        variable_binding_size = 0;

        if (unbounded_offset != UINT_MAX
                /* Descriptors may not be set, eg. WoW. */
                && (base_descriptor = bindings->descriptor_tables[table_index]))
        {
            heap_size = d3d12_desc_heap_range_size(base_descriptor);

            if (heap_size < unbounded_offset)
                WARN("Descriptor heap size %u is less than the offset %u of an unbounded range in table %u, "
                        "vk set %u.\n", heap_size, unbounded_offset, table_index, i);
            else
                variable_binding_size = heap_size - unbounded_offset;
        }

        vk_descriptor_set = d3d12_command_allocator_allocate_descriptor_set(list->allocator,
                layout->vk_layout, variable_binding_size, unbounded_offset != UINT_MAX);
        bindings->descriptor_sets[bindings->descriptor_set_count++] = vk_descriptor_set;
    }

    bindings->in_use = false;

    bindings->descriptor_table_dirty_mask |= bindings->descriptor_table_active_mask & root_signature->descriptor_table_mask;
    bindings->push_descriptor_dirty_mask |= bindings->push_descriptor_active_mask & root_signature->push_descriptor_mask;
}

static bool vk_write_descriptor_set_from_d3d12_desc(VkWriteDescriptorSet *vk_descriptor_write,
        VkDescriptorImageInfo *vk_image_info, const struct d3d12_desc *descriptor,
        const struct d3d12_root_descriptor_table_range *range, VkDescriptorSet *vk_descriptor_sets,
        unsigned int index, bool use_array)
{
    uint32_t descriptor_range_magic = range->descriptor_magic;
    union d3d12_desc_object u = descriptor->s.u;
    uint32_t vk_binding = range->binding;
    VkDescriptorType vk_descriptor_type;
    uint32_t set = range->set;

    if (!u.header || u.header->magic != descriptor_range_magic)
        return false;

    vk_descriptor_type = u.header->vk_descriptor_type;

    vk_descriptor_write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vk_descriptor_write->pNext = NULL;
    vk_descriptor_write->dstSet = vk_descriptor_sets[set];
    vk_descriptor_write->dstBinding = use_array ? vk_binding : vk_binding + index;
    vk_descriptor_write->dstArrayElement = use_array ? index : 0;
    vk_descriptor_write->descriptorCount = 1;
    vk_descriptor_write->descriptorType = vk_descriptor_type;
    vk_descriptor_write->pImageInfo = NULL;
    vk_descriptor_write->pBufferInfo = NULL;
    vk_descriptor_write->pTexelBufferView = NULL;

    switch (u.header->magic)
    {
        case VKD3D_DESCRIPTOR_MAGIC_CBV:
            vk_descriptor_write->pBufferInfo = &u.cb_desc->vk_cbv_info;
            break;

        case VKD3D_DESCRIPTOR_MAGIC_SRV:
        case VKD3D_DESCRIPTOR_MAGIC_UAV:
            /* We use separate bindings for buffer and texture SRVs/UAVs.
             * See d3d12_root_signature_init(). For unbounded ranges the
             * descriptors exist in two consecutive sets, otherwise they occur
             * as consecutive ranges within a set. */
            if (vk_descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
                    || vk_descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
            {
                vk_descriptor_write->pTexelBufferView = &u.view->v.u.vk_buffer_view;
                break;
            }

            if (range->descriptor_count == UINT_MAX)
            {
                vk_descriptor_write->dstSet = vk_descriptor_sets[set + 1];
                vk_descriptor_write->dstBinding = 0;
            }
            else
            {
                vk_descriptor_write->dstBinding += use_array ? 1 : range->descriptor_count;
            }

            vk_image_info->sampler = VK_NULL_HANDLE;
            vk_image_info->imageView = u.view->v.u.vk_image_view;
            vk_image_info->imageLayout = u.header->magic == VKD3D_DESCRIPTOR_MAGIC_SRV
                    ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

            vk_descriptor_write->pImageInfo = vk_image_info;
            break;

        case VKD3D_DESCRIPTOR_MAGIC_SAMPLER:
            vk_image_info->sampler = u.view->v.u.vk_sampler;
            vk_image_info->imageView = VK_NULL_HANDLE;
            vk_image_info->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            vk_descriptor_write->pImageInfo = vk_image_info;
            break;

        default:
            ERR("Invalid descriptor %#x.\n", u.header->magic);
            return false;
    }

    return true;
}

static void d3d12_command_list_update_descriptor_table(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point, unsigned int index, struct d3d12_desc *base_descriptor)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    struct VkWriteDescriptorSet descriptor_writes[24], *current_descriptor_write;
    const struct d3d12_root_signature *root_signature = bindings->root_signature;
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    struct VkDescriptorImageInfo image_infos[24], *current_image_info;
    const struct d3d12_root_descriptor_table *descriptor_table;
    const struct d3d12_pipeline_state *state = list->state;
    const struct d3d12_root_descriptor_table_range *range;
    VkDevice vk_device = list->device->vk_device;
    unsigned int i, j, k, descriptor_count;
    struct d3d12_desc *descriptor;
    unsigned int write_count = 0;
    bool unbounded = false;

    descriptor_table = root_signature_get_descriptor_table(root_signature, index);

    current_descriptor_write = descriptor_writes;
    current_image_info = image_infos;
    for (i = 0; i < descriptor_table->range_count; ++i)
    {
        range = &descriptor_table->ranges[i];

        /* The first unbounded range of each type is written until the heap end is reached. Do not repeat. */
        if (unbounded && i && range->type == descriptor_table->ranges[i - 1].type)
            continue;

        descriptor = base_descriptor + range->offset;

        descriptor_count = range->descriptor_count;
        if ((unbounded = descriptor_count == UINT_MAX))
        {
            descriptor_count = d3d12_desc_heap_range_size(descriptor);

            if (descriptor_count > range->vk_binding_count)
            {
                ERR("Heap descriptor count %u exceeds maximum Vulkan count %u. Reducing to the Vulkan maximum.\n",
                        descriptor_count, range->vk_binding_count);
                descriptor_count = range->vk_binding_count;
            }
        }

        for (j = 0; j < descriptor_count; ++j, ++descriptor)
        {
            unsigned int register_idx = range->base_register_idx + j;
            union d3d12_desc_object u = descriptor->s.u;
            VkBufferView vk_counter_view;

            vk_counter_view = (u.header && u.header->magic == VKD3D_DESCRIPTOR_MAGIC_UAV)
                    ? u.view->v.vk_counter_view : VK_NULL_HANDLE;

            /* Track UAV counters. */
            if (range->descriptor_magic == VKD3D_DESCRIPTOR_MAGIC_UAV)
            {
                for (k = 0; k < state->uav_counters.binding_count; ++k)
                {
                    if (state->uav_counters.bindings[k].register_space == range->register_space
                            && state->uav_counters.bindings[k].register_index == register_idx)
                    {
                        if (bindings->vk_uav_counter_views[k] != vk_counter_view)
                            bindings->uav_counters_dirty = true;
                        bindings->vk_uav_counter_views[k] = vk_counter_view;
                        break;
                    }
                }
            }

            /* Not all descriptors are necessarily populated if the range is unbounded. */
            if (!u.header)
                continue;

            if (!vk_write_descriptor_set_from_d3d12_desc(current_descriptor_write, current_image_info,
                    descriptor, range, bindings->descriptor_sets, j, root_signature->use_descriptor_arrays))
                continue;

            ++write_count;
            ++current_descriptor_write;
            ++current_image_info;

            if (write_count == ARRAY_SIZE(descriptor_writes))
            {
                VK_CALL(vkUpdateDescriptorSets(vk_device, write_count, descriptor_writes, 0, NULL));
                write_count = 0;
                current_descriptor_write = descriptor_writes;
                current_image_info = image_infos;
            }
        }
    }

    VK_CALL(vkUpdateDescriptorSets(vk_device, write_count, descriptor_writes, 0, NULL));
}

static bool vk_write_descriptor_set_from_root_descriptor(VkWriteDescriptorSet *vk_descriptor_write,
        const struct d3d12_root_parameter *root_parameter, VkDescriptorSet vk_descriptor_set,
        VkBufferView *vk_buffer_view, const VkDescriptorBufferInfo *vk_buffer_info)
{
    const struct d3d12_root_descriptor *root_descriptor;

    switch (root_parameter->parameter_type)
    {
        case D3D12_ROOT_PARAMETER_TYPE_CBV:
            vk_descriptor_write->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case D3D12_ROOT_PARAMETER_TYPE_SRV:
            vk_descriptor_write->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            break;
        case D3D12_ROOT_PARAMETER_TYPE_UAV:
            vk_descriptor_write->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            break;
        default:
            ERR("Invalid root descriptor %#x.\n", root_parameter->parameter_type);
            return false;
    }

    root_descriptor = &root_parameter->u.descriptor;

    vk_descriptor_write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vk_descriptor_write->pNext = NULL;
    vk_descriptor_write->dstSet = vk_descriptor_set;
    vk_descriptor_write->dstBinding = root_descriptor->binding;
    vk_descriptor_write->dstArrayElement = 0;
    vk_descriptor_write->descriptorCount = 1;
    vk_descriptor_write->pImageInfo = NULL;
    vk_descriptor_write->pBufferInfo = vk_buffer_info;
    vk_descriptor_write->pTexelBufferView = vk_buffer_view;

    return true;
}

static void d3d12_command_list_update_push_descriptors(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    VkWriteDescriptorSet descriptor_writes[ARRAY_SIZE(bindings->push_descriptors)] = {0};
    VkDescriptorBufferInfo buffer_infos[ARRAY_SIZE(bindings->push_descriptors)] = {0};
    const struct d3d12_root_signature *root_signature = bindings->root_signature;
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct d3d12_root_parameter *root_parameter;
    struct vkd3d_push_descriptor *push_descriptor;
    struct d3d12_device *device = list->device;
    VkDescriptorBufferInfo *vk_buffer_info;
    unsigned int i, descriptor_count = 0;
    VkBufferView *vk_buffer_view;

    if (!bindings->push_descriptor_dirty_mask)
        return;

    for (i = 0; i < ARRAY_SIZE(bindings->push_descriptors); ++i)
    {
        if (!(bindings->push_descriptor_dirty_mask & (1u << i)))
            continue;

        root_parameter = root_signature_get_root_descriptor(root_signature, i);
        push_descriptor = &bindings->push_descriptors[i];

        if (root_parameter->parameter_type == D3D12_ROOT_PARAMETER_TYPE_CBV)
        {
            vk_buffer_view = NULL;
            vk_buffer_info = &buffer_infos[descriptor_count];
            vk_buffer_info->buffer = push_descriptor->u.cbv.vk_buffer;
            vk_buffer_info->offset = push_descriptor->u.cbv.offset;
            vk_buffer_info->range = VK_WHOLE_SIZE;
        }
        else
        {
            vk_buffer_view = &push_descriptor->u.vk_buffer_view;
            vk_buffer_info = NULL;
        }

        if (!vk_write_descriptor_set_from_root_descriptor(&descriptor_writes[descriptor_count],
                root_parameter, bindings->descriptor_sets[0], vk_buffer_view, vk_buffer_info))
            continue;

        ++descriptor_count;
    }

    VK_CALL(vkUpdateDescriptorSets(device->vk_device, descriptor_count, descriptor_writes, 0, NULL));
    bindings->push_descriptor_dirty_mask = 0;
}

static void d3d12_command_list_update_uav_counter_descriptors(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct d3d12_pipeline_state *state = list->state;
    VkDevice vk_device = list->device->vk_device;
    VkWriteDescriptorSet *vk_descriptor_writes;
    VkDescriptorSet vk_descriptor_set;
    unsigned int uav_counter_count;
    unsigned int i;

    if (!state || !bindings->uav_counters_dirty)
        return;

    uav_counter_count = state->uav_counters.binding_count;
    if (!(vk_descriptor_writes = vkd3d_calloc(uav_counter_count, sizeof(*vk_descriptor_writes))))
        return;
    if (!(vk_descriptor_set = d3d12_command_allocator_allocate_descriptor_set(
            list->allocator, state->uav_counters.vk_set_layout, 0, false)))
        goto done;

    for (i = 0; i < uav_counter_count; ++i)
    {
        const struct vkd3d_shader_uav_counter_binding *uav_counter = &state->uav_counters.bindings[i];
        const VkBufferView *vk_uav_counter_views = bindings->vk_uav_counter_views;

        VKD3D_ASSERT(vk_uav_counter_views[i]);

        vk_descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vk_descriptor_writes[i].pNext = NULL;
        vk_descriptor_writes[i].dstSet = vk_descriptor_set;
        vk_descriptor_writes[i].dstBinding = uav_counter->binding.binding;
        vk_descriptor_writes[i].dstArrayElement = 0;
        vk_descriptor_writes[i].descriptorCount = 1;
        vk_descriptor_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        vk_descriptor_writes[i].pImageInfo = NULL;
        vk_descriptor_writes[i].pBufferInfo = NULL;
        vk_descriptor_writes[i].pTexelBufferView = &vk_uav_counter_views[i];
    }

    VK_CALL(vkUpdateDescriptorSets(vk_device, uav_counter_count, vk_descriptor_writes, 0, NULL));

    VK_CALL(vkCmdBindDescriptorSets(list->vk_command_buffer, bindings->vk_bind_point,
            state->uav_counters.vk_pipeline_layout, state->uav_counters.set_index, 1, &vk_descriptor_set, 0, NULL));

    bindings->uav_counters_dirty = false;

done:
    vkd3d_free(vk_descriptor_writes);
}

static void d3d12_command_list_update_virtual_descriptors(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct d3d12_root_signature *rs = bindings->root_signature;
    struct d3d12_desc *base_descriptor;
    unsigned int i;

    if (!rs || !rs->vk_set_count)
        return;

    if (bindings->descriptor_table_dirty_mask || bindings->push_descriptor_dirty_mask)
        d3d12_command_list_prepare_descriptors(list, bind_point);

    for (i = 0; i < ARRAY_SIZE(bindings->descriptor_tables); ++i)
    {
        if (bindings->descriptor_table_dirty_mask & ((uint64_t)1 << i))
        {
            if ((base_descriptor = bindings->descriptor_tables[i]))
                d3d12_command_list_update_descriptor_table(list, bind_point, i, base_descriptor);
            else
                WARN("Descriptor table %u is not set.\n", i);
        }
    }
    bindings->descriptor_table_dirty_mask = 0;

    d3d12_command_list_update_push_descriptors(list, bind_point);

    if (bindings->descriptor_set_count)
    {
        VK_CALL(vkCmdBindDescriptorSets(list->vk_command_buffer, bindings->vk_bind_point,
                rs->vk_pipeline_layout, rs->main_set, bindings->descriptor_set_count, bindings->descriptor_sets,
                0, NULL));
        bindings->in_use = true;
    }

    d3d12_command_list_update_uav_counter_descriptors(list, bind_point);
}

static unsigned int d3d12_command_list_bind_descriptor_table(struct d3d12_command_list *list,
        struct vkd3d_pipeline_bindings *bindings, unsigned int index,
        struct d3d12_descriptor_heap **cbv_srv_uav_heap, struct d3d12_descriptor_heap **sampler_heap)
{
    struct d3d12_descriptor_heap *heap;
    const struct d3d12_desc *desc;
    unsigned int offset;

    if (!(desc = bindings->descriptor_tables[index]))
        return 0;

    /* AMD, Nvidia and Intel drivers on Windows work if SetDescriptorHeaps()
     * is not called, so we bind heaps from the tables instead. No NULL check is
     * needed here because it's checked when descriptor tables are set. */
    heap = d3d12_desc_get_descriptor_heap(desc);
    offset = desc->index;

    if (heap->desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        if (*cbv_srv_uav_heap)
        {
            if (heap == *cbv_srv_uav_heap)
                return offset;
            /* This occurs occasionally in Rise of the Tomb Raider apparently due to a race
             * condition (one of several), but adding a mutex for table updates has no effect. */
            WARN("List %p uses descriptors from more than one CBV/SRV/UAV heap.\n", list);
        }
        *cbv_srv_uav_heap = heap;
    }
    else
    {
        if (*sampler_heap)
        {
            if (heap == *sampler_heap)
                return offset;
            WARN("List %p uses descriptors from more than one sampler heap.\n", list);
        }
        *sampler_heap = heap;
    }

    return offset;
}

static void d3d12_command_list_update_descriptor_tables(struct d3d12_command_list *list,
        struct vkd3d_pipeline_bindings *bindings, struct d3d12_descriptor_heap **cbv_srv_uav_heap,
        struct d3d12_descriptor_heap **sampler_heap)
{
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct d3d12_root_signature *rs = bindings->root_signature;
    unsigned int offsets[D3D12_MAX_ROOT_COST];
    unsigned int i, j;

    for (i = 0, j = 0; i < ARRAY_SIZE(bindings->descriptor_tables); ++i)
    {
        if (!(rs->descriptor_table_mask & ((uint64_t)1 << i)))
            continue;
        offsets[j++] = d3d12_command_list_bind_descriptor_table(list, bindings, i,
                cbv_srv_uav_heap, sampler_heap);
    }
    if (j)
    {
        VK_CALL(vkCmdPushConstants(list->vk_command_buffer, rs->vk_pipeline_layout, VK_SHADER_STAGE_ALL,
                rs->descriptor_table_offset, j * sizeof(uint32_t), offsets));
    }
}

static bool contains_heap(struct d3d12_descriptor_heap **heap_array, unsigned int count,
        const struct d3d12_descriptor_heap *query)
{
    unsigned int i;

    for (i = 0; i < count; ++i)
        if (heap_array[i] == query)
            return true;
    return false;
}

static void command_list_flush_vk_heap_updates(struct d3d12_command_list *list)
{
    struct d3d12_device *device = list->device;
    unsigned int i;

    for (i = 0; i < list->descriptor_heap_count; ++i)
    {
        vkd3d_mutex_lock(&list->descriptor_heaps[i]->vk_sets_mutex);
        d3d12_desc_flush_vk_heap_updates_locked(list->descriptor_heaps[i], device);
        vkd3d_mutex_unlock(&list->descriptor_heaps[i]->vk_sets_mutex);
    }
}

static void command_list_add_descriptor_heap(struct d3d12_command_list *list, struct d3d12_descriptor_heap *heap)
{
    if (!list->device->use_vk_heaps)
        return;

    if (!contains_heap(list->descriptor_heaps, list->descriptor_heap_count, heap))
    {
        if (list->descriptor_heap_count == ARRAY_SIZE(list->descriptor_heaps))
        {
            /* Descriptors can be written after binding. */
            FIXME("Flushing descriptor updates while list %p is not closed.\n", list);
            vkd3d_mutex_lock(&heap->vk_sets_mutex);
            d3d12_desc_flush_vk_heap_updates_locked(heap, list->device);
            vkd3d_mutex_unlock(&heap->vk_sets_mutex);
            return;
        }
        list->descriptor_heaps[list->descriptor_heap_count++] = heap;
    }
}

static void d3d12_command_list_bind_descriptor_heap(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point, struct d3d12_descriptor_heap *heap)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct d3d12_root_signature *rs = bindings->root_signature;
    enum vkd3d_vk_descriptor_set_index set;

    if (!heap)
        return;

    if (heap->desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        if (heap->serial_id == bindings->cbv_srv_uav_heap_id)
            return;
        bindings->cbv_srv_uav_heap_id = heap->serial_id;
    }
    else
    {
        if (heap->serial_id == bindings->sampler_heap_id)
            return;
        bindings->sampler_heap_id = heap->serial_id;
    }

    vkd3d_mutex_lock(&heap->vk_sets_mutex);

    for (set = 0; set < ARRAY_SIZE(heap->vk_descriptor_sets); ++set)
    {
        VkDescriptorSet vk_descriptor_set = heap->vk_descriptor_sets[set].vk_set;

        /* Null vk_set_layout means set 0 uses mutable descriptors, and this set is unused. */
        if (!vk_descriptor_set || !list->device->vk_descriptor_heap_layouts[set].vk_set_layout)
            continue;

        VK_CALL(vkCmdBindDescriptorSets(list->vk_command_buffer, bindings->vk_bind_point, rs->vk_pipeline_layout,
                rs->vk_set_count + set, 1, &vk_descriptor_set, 0, NULL));
    }

    vkd3d_mutex_unlock(&heap->vk_sets_mutex);
}

static void d3d12_command_list_update_heap_descriptors(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    struct d3d12_descriptor_heap *cbv_srv_uav_heap = NULL, *sampler_heap = NULL;
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct d3d12_root_signature *rs = bindings->root_signature;

    if (!rs)
        return;

    if (bindings->descriptor_table_dirty_mask || bindings->push_descriptor_dirty_mask)
        d3d12_command_list_prepare_descriptors(list, bind_point);
    if (bindings->descriptor_table_dirty_mask)
        d3d12_command_list_update_descriptor_tables(list, bindings, &cbv_srv_uav_heap, &sampler_heap);
    bindings->descriptor_table_dirty_mask = 0;

    d3d12_command_list_update_push_descriptors(list, bind_point);

    if (bindings->descriptor_set_count)
    {
        VK_CALL(vkCmdBindDescriptorSets(list->vk_command_buffer, bindings->vk_bind_point, rs->vk_pipeline_layout,
                rs->main_set, bindings->descriptor_set_count, bindings->descriptor_sets, 0, NULL));
        bindings->in_use = true;
    }

    d3d12_command_list_bind_descriptor_heap(list, bind_point, cbv_srv_uav_heap);
    d3d12_command_list_bind_descriptor_heap(list, bind_point, sampler_heap);
}

static void d3d12_command_list_update_descriptors(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point)
{
    if (list->device->use_vk_heaps)
        d3d12_command_list_update_heap_descriptors(list, bind_point);
    else
        d3d12_command_list_update_virtual_descriptors(list, bind_point);
}

static bool d3d12_command_list_update_compute_state(struct d3d12_command_list *list)
{
    d3d12_command_list_end_current_render_pass(list);

    if (!d3d12_command_list_update_compute_pipeline(list))
        return false;

    d3d12_command_list_update_descriptors(list, VKD3D_PIPELINE_BIND_POINT_COMPUTE);

    return true;
}

static bool d3d12_command_list_begin_render_pass(struct d3d12_command_list *list)
{
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    struct d3d12_graphics_pipeline_state *graphics;
    struct VkRenderPassBeginInfo begin_desc;
    VkRenderPass vk_render_pass;

    if (!d3d12_command_list_update_graphics_pipeline(list))
        return false;
    if (!d3d12_command_list_update_current_framebuffer(list))
        return false;

    d3d12_command_list_update_descriptors(list, VKD3D_PIPELINE_BIND_POINT_GRAPHICS);

    if (list->current_render_pass != VK_NULL_HANDLE)
        return true;

    vk_render_pass = list->pso_render_pass;
    VKD3D_ASSERT(vk_render_pass);

    begin_desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_desc.pNext = NULL;
    begin_desc.renderPass = vk_render_pass;
    begin_desc.framebuffer = list->current_framebuffer;
    begin_desc.renderArea.offset.x = 0;
    begin_desc.renderArea.offset.y = 0;
    d3d12_command_list_get_fb_extent(list,
            &begin_desc.renderArea.extent.width, &begin_desc.renderArea.extent.height, NULL);
    begin_desc.clearValueCount = 0;
    begin_desc.pClearValues = NULL;
    VK_CALL(vkCmdBeginRenderPass(list->vk_command_buffer, &begin_desc, VK_SUBPASS_CONTENTS_INLINE));

    list->current_render_pass = vk_render_pass;

    graphics = &list->state->u.graphics;
    if (graphics->xfb_enabled)
    {
        VK_CALL(vkCmdBeginTransformFeedbackEXT(list->vk_command_buffer, 0, ARRAY_SIZE(list->so_counter_buffers),
                list->so_counter_buffers, list->so_counter_buffer_offsets));

        list->xfb_enabled = true;
    }

    if (graphics->ds_desc.depthBoundsTestEnable && !list->has_depth_bounds)
    {
        list->has_depth_bounds = true;
        VK_CALL(vkCmdSetDepthBounds(list->vk_command_buffer, 0.0f, 1.0f));
    }

    return true;
}

static void d3d12_command_list_check_index_buffer_strip_cut_value(struct d3d12_command_list *list)
{
    struct d3d12_graphics_pipeline_state *graphics = &list->state->u.graphics;

    /* In Vulkan, the strip cut value is derived from the index buffer format. */
    switch (graphics->index_buffer_strip_cut_value)
    {
        case D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF:
            if (list->index_buffer_format != DXGI_FORMAT_R16_UINT)
            {
                FIXME_ONCE("Strip cut value 0xffff is not supported with index buffer format %#x.\n",
                        list->index_buffer_format);
            }
            break;

        case D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF:
            if (list->index_buffer_format != DXGI_FORMAT_R32_UINT)
            {
                FIXME_ONCE("Strip cut value 0xffffffff is not supported with index buffer format %#x.\n",
                        list->index_buffer_format);
            }
            break;

        default:
            break;
    }
}

static void STDMETHODCALLTYPE d3d12_command_list_DrawInstanced(ID3D12GraphicsCommandList6 *iface,
        UINT vertex_count_per_instance, UINT instance_count, UINT start_vertex_location,
        UINT start_instance_location)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_vk_device_procs *vk_procs;

    TRACE("iface %p, vertex_count_per_instance %u, instance_count %u, "
            "start_vertex_location %u, start_instance_location %u.\n",
            iface, vertex_count_per_instance, instance_count,
            start_vertex_location, start_instance_location);

    vk_procs = &list->device->vk_procs;

    if (!d3d12_command_list_begin_render_pass(list))
    {
        WARN("Failed to begin render pass, ignoring draw call.\n");
        return;
    }

    VK_CALL(vkCmdDraw(list->vk_command_buffer, vertex_count_per_instance,
            instance_count, start_vertex_location, start_instance_location));
}

static void STDMETHODCALLTYPE d3d12_command_list_DrawIndexedInstanced(ID3D12GraphicsCommandList6 *iface,
        UINT index_count_per_instance, UINT instance_count, UINT start_vertex_location,
        INT base_vertex_location, UINT start_instance_location)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_vk_device_procs *vk_procs;

    TRACE("iface %p, index_count_per_instance %u, instance_count %u, start_vertex_location %u, "
            "base_vertex_location %d, start_instance_location %u.\n",
            iface, index_count_per_instance, instance_count, start_vertex_location,
            base_vertex_location, start_instance_location);

    if (!d3d12_command_list_begin_render_pass(list))
    {
        WARN("Failed to begin render pass, ignoring draw call.\n");
        return;
    }

    vk_procs = &list->device->vk_procs;

    d3d12_command_list_check_index_buffer_strip_cut_value(list);

    VK_CALL(vkCmdDrawIndexed(list->vk_command_buffer, index_count_per_instance,
            instance_count, start_vertex_location, base_vertex_location, start_instance_location));
}

static void STDMETHODCALLTYPE d3d12_command_list_Dispatch(ID3D12GraphicsCommandList6 *iface,
        UINT x, UINT y, UINT z)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_vk_device_procs *vk_procs;

    TRACE("iface %p, x %u, y %u, z %u.\n", iface, x, y, z);

    if (!d3d12_command_list_update_compute_state(list))
    {
        WARN("Failed to update compute state, ignoring dispatch.\n");
        return;
    }

    vk_procs = &list->device->vk_procs;

    VK_CALL(vkCmdDispatch(list->vk_command_buffer, x, y, z));
}

static void STDMETHODCALLTYPE d3d12_command_list_CopyBufferRegion(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *dst, UINT64 dst_offset, ID3D12Resource *src, UINT64 src_offset, UINT64 byte_count)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct d3d12_resource *dst_resource, *src_resource;
    const struct vkd3d_vk_device_procs *vk_procs;
    VkBufferCopy buffer_copy;

    TRACE("iface %p, dst_resource %p, dst_offset %#"PRIx64", src_resource %p, "
            "src_offset %#"PRIx64", byte_count %#"PRIx64".\n",
            iface, dst, dst_offset, src, src_offset, byte_count);

    vk_procs = &list->device->vk_procs;

    dst_resource = unsafe_impl_from_ID3D12Resource(dst);
    VKD3D_ASSERT(d3d12_resource_is_buffer(dst_resource));
    src_resource = unsafe_impl_from_ID3D12Resource(src);
    VKD3D_ASSERT(d3d12_resource_is_buffer(src_resource));

    d3d12_command_list_track_resource_usage(list, dst_resource);
    d3d12_command_list_track_resource_usage(list, src_resource);

    d3d12_command_list_end_current_render_pass(list);

    buffer_copy.srcOffset = src_offset;
    buffer_copy.dstOffset = dst_offset;
    buffer_copy.size = byte_count;

    VK_CALL(vkCmdCopyBuffer(list->vk_command_buffer,
            src_resource->u.vk_buffer, dst_resource->u.vk_buffer, 1, &buffer_copy));
}

static void vk_image_subresource_layers_from_d3d12(VkImageSubresourceLayers *subresource,
        const struct vkd3d_format *format, unsigned int sub_resource_idx, unsigned int miplevel_count)
{
    subresource->aspectMask = format->vk_aspect_mask;
    subresource->mipLevel = sub_resource_idx % miplevel_count;
    subresource->baseArrayLayer = sub_resource_idx / miplevel_count;
    subresource->layerCount = 1;
}

static void vk_extent_3d_from_d3d12_miplevel(VkExtent3D *extent,
        const D3D12_RESOURCE_DESC1 *resource_desc, unsigned int miplevel_idx)
{
    extent->width = d3d12_resource_desc_get_width(resource_desc, miplevel_idx);
    extent->height = d3d12_resource_desc_get_height(resource_desc, miplevel_idx);
    extent->depth = d3d12_resource_desc_get_depth(resource_desc, miplevel_idx);
}

static void vk_buffer_image_copy_from_d3d12(VkBufferImageCopy *copy,
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprint, unsigned int sub_resource_idx,
        const D3D12_RESOURCE_DESC1 *image_desc, const struct vkd3d_format *format,
        const D3D12_BOX *src_box, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    copy->bufferOffset = footprint->Offset;
    if (src_box)
    {
        VkDeviceSize row_count = footprint->Footprint.Height / format->block_height;
        copy->bufferOffset += vkd3d_format_get_data_offset(format, footprint->Footprint.RowPitch,
                row_count * footprint->Footprint.RowPitch, src_box->left, src_box->top, src_box->front);
    }
    copy->bufferRowLength = footprint->Footprint.RowPitch /
            (format->byte_count * format->block_byte_count) * format->block_width;
    copy->bufferImageHeight = footprint->Footprint.Height;
    vk_image_subresource_layers_from_d3d12(&copy->imageSubresource,
            format, sub_resource_idx, image_desc->MipLevels);
    copy->imageOffset.x = dst_x;
    copy->imageOffset.y = dst_y;
    copy->imageOffset.z = dst_z;

    vk_extent_3d_from_d3d12_miplevel(&copy->imageExtent, image_desc,
            copy->imageSubresource.mipLevel);
    copy->imageExtent.width -= copy->imageOffset.x;
    copy->imageExtent.height -= copy->imageOffset.y;
    copy->imageExtent.depth -= copy->imageOffset.z;

    if (src_box)
    {
        copy->imageExtent.width = min(copy->imageExtent.width, src_box->right - src_box->left);
        copy->imageExtent.height = min(copy->imageExtent.height, src_box->bottom - src_box->top);
        copy->imageExtent.depth = min(copy->imageExtent.depth, src_box->back - src_box->front);
    }
    else
    {
        copy->imageExtent.width = min(copy->imageExtent.width, footprint->Footprint.Width);
        copy->imageExtent.height = min(copy->imageExtent.height, footprint->Footprint.Height);
        copy->imageExtent.depth = min(copy->imageExtent.depth, footprint->Footprint.Depth);
    }
}

static void vk_image_buffer_copy_from_d3d12(VkBufferImageCopy *copy,
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprint, unsigned int sub_resource_idx,
        const D3D12_RESOURCE_DESC1 *image_desc, const struct vkd3d_format *format,
        const D3D12_BOX *src_box, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    VkDeviceSize row_count = footprint->Footprint.Height / format->block_height;

    copy->bufferOffset = footprint->Offset + vkd3d_format_get_data_offset(format,
            footprint->Footprint.RowPitch, row_count * footprint->Footprint.RowPitch, dst_x, dst_y, dst_z);
    copy->bufferRowLength = footprint->Footprint.RowPitch /
            (format->byte_count * format->block_byte_count) * format->block_width;
    copy->bufferImageHeight = footprint->Footprint.Height;
    vk_image_subresource_layers_from_d3d12(&copy->imageSubresource,
            format, sub_resource_idx, image_desc->MipLevels);
    copy->imageOffset.x = src_box ? src_box->left : 0;
    copy->imageOffset.y = src_box ? src_box->top : 0;
    copy->imageOffset.z = src_box ? src_box->front : 0;
    if (src_box)
    {
        copy->imageExtent.width = src_box->right - src_box->left;
        copy->imageExtent.height = src_box->bottom - src_box->top;
        copy->imageExtent.depth = src_box->back - src_box->front;
    }
    else
    {
        unsigned int miplevel = copy->imageSubresource.mipLevel;
        vk_extent_3d_from_d3d12_miplevel(&copy->imageExtent, image_desc, miplevel);
    }
}

static void vk_image_copy_from_d3d12(VkImageCopy *image_copy,
        unsigned int src_sub_resource_idx, unsigned int dst_sub_resource_idx,
        const D3D12_RESOURCE_DESC1 *src_desc, const D3D12_RESOURCE_DESC1 *dst_desc,
        const struct vkd3d_format *src_format, const struct vkd3d_format *dst_format,
        const D3D12_BOX *src_box, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    vk_image_subresource_layers_from_d3d12(&image_copy->srcSubresource,
            src_format, src_sub_resource_idx, src_desc->MipLevels);
    image_copy->srcOffset.x = src_box ? src_box->left : 0;
    image_copy->srcOffset.y = src_box ? src_box->top : 0;
    image_copy->srcOffset.z = src_box ? src_box->front : 0;
    vk_image_subresource_layers_from_d3d12(&image_copy->dstSubresource,
            dst_format, dst_sub_resource_idx, dst_desc->MipLevels);
    image_copy->dstOffset.x = dst_x;
    image_copy->dstOffset.y = dst_y;
    image_copy->dstOffset.z = dst_z;
    if (src_box)
    {
        image_copy->extent.width = src_box->right - src_box->left;
        image_copy->extent.height = src_box->bottom - src_box->top;
        image_copy->extent.depth = src_box->back - src_box->front;
    }
    else
    {
        unsigned int miplevel = image_copy->srcSubresource.mipLevel;
        vk_extent_3d_from_d3d12_miplevel(&image_copy->extent, src_desc, miplevel);
    }
}

static HRESULT d3d12_command_list_allocate_transfer_buffer(struct d3d12_command_list *list,
        VkDeviceSize size, struct vkd3d_buffer *buffer)
{
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    struct d3d12_device *device = list->device;
    D3D12_HEAP_PROPERTIES heap_properties;
    D3D12_RESOURCE_DESC1 buffer_desc;
    HRESULT hr;

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Alignment = 0;
    buffer_desc.Width = size;
    buffer_desc.Height = 1;
    buffer_desc.DepthOrArraySize = 1;
    buffer_desc.MipLevels = 1;
    buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
    buffer_desc.SampleDesc.Count = 1;
    buffer_desc.SampleDesc.Quality = 0;
    buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffer_desc.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

    if (FAILED(hr = vkd3d_create_buffer(device, &heap_properties, D3D12_HEAP_FLAG_NONE,
            &buffer_desc, &buffer->vk_buffer)))
        return hr;
    if (FAILED(hr = vkd3d_allocate_buffer_memory(device, buffer->vk_buffer,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &buffer->vk_memory, NULL, NULL)))
    {
        VK_CALL(vkDestroyBuffer(device->vk_device, buffer->vk_buffer, NULL));
        return hr;
    }

    if (!d3d12_command_allocator_add_transfer_buffer(list->allocator, buffer))
    {
        ERR("Failed to add transfer buffer.\n");
        vkd3d_buffer_destroy(buffer, device);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

/* In Vulkan, each depth/stencil format is only compatible with itself.
 * This means that we are not allowed to copy texture regions directly between
 * depth/stencil and color formats.
 *
 * FIXME: Implement color <-> depth/stencil blits in shaders.
 */
static void d3d12_command_list_copy_incompatible_texture_region(struct d3d12_command_list *list,
        struct d3d12_resource *dst_resource, unsigned int dst_sub_resource_idx,
        const struct vkd3d_format *dst_format, struct d3d12_resource *src_resource,
        unsigned int src_sub_resource_idx, const struct vkd3d_format *src_format, unsigned int layer_count)
{
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const D3D12_RESOURCE_DESC1 *dst_desc = &dst_resource->desc;
    const D3D12_RESOURCE_DESC1 *src_desc = &src_resource->desc;
    unsigned int dst_miplevel_idx, src_miplevel_idx;
    struct vkd3d_buffer transfer_buffer;
    VkBufferImageCopy buffer_image_copy;
    VkBufferMemoryBarrier vk_barrier;
    VkDeviceSize buffer_size;
    HRESULT hr;

    WARN("Copying incompatible texture formats %#x, %#x -> %#x, %#x.\n",
            src_format->dxgi_format, src_format->vk_format,
            dst_format->dxgi_format, dst_format->vk_format);

    VKD3D_ASSERT(d3d12_resource_is_texture(dst_resource));
    VKD3D_ASSERT(d3d12_resource_is_texture(src_resource));
    VKD3D_ASSERT(!vkd3d_format_is_compressed(dst_format));
    VKD3D_ASSERT(!vkd3d_format_is_compressed(src_format));
    VKD3D_ASSERT(dst_format->byte_count == src_format->byte_count);

    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    vk_image_subresource_layers_from_d3d12(&buffer_image_copy.imageSubresource,
            src_format, src_sub_resource_idx, src_desc->MipLevels);
    buffer_image_copy.imageSubresource.layerCount = layer_count;
    src_miplevel_idx = buffer_image_copy.imageSubresource.mipLevel;
    buffer_image_copy.imageOffset.x = 0;
    buffer_image_copy.imageOffset.y = 0;
    buffer_image_copy.imageOffset.z = 0;
    vk_extent_3d_from_d3d12_miplevel(&buffer_image_copy.imageExtent, src_desc, src_miplevel_idx);

    buffer_size = src_format->byte_count * buffer_image_copy.imageExtent.width *
            buffer_image_copy.imageExtent.height * buffer_image_copy.imageExtent.depth * layer_count;
    if (FAILED(hr = d3d12_command_list_allocate_transfer_buffer(list, buffer_size, &transfer_buffer)))
    {
        ERR("Failed to allocate transfer buffer, hr %s.\n", debugstr_hresult(hr));
        return;
    }

    VK_CALL(vkCmdCopyImageToBuffer(list->vk_command_buffer,
            src_resource->u.vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            transfer_buffer.vk_buffer, 1, &buffer_image_copy));

    vk_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    vk_barrier.pNext = NULL;
    vk_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vk_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    vk_barrier.buffer = transfer_buffer.vk_buffer;
    vk_barrier.offset = 0;
    vk_barrier.size = VK_WHOLE_SIZE;
    VK_CALL(vkCmdPipelineBarrier(list->vk_command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, NULL, 1, &vk_barrier, 0, NULL));

    vk_image_subresource_layers_from_d3d12(&buffer_image_copy.imageSubresource,
            dst_format, dst_sub_resource_idx, dst_desc->MipLevels);
    buffer_image_copy.imageSubresource.layerCount = layer_count;
    dst_miplevel_idx = buffer_image_copy.imageSubresource.mipLevel;

    VKD3D_ASSERT(d3d12_resource_desc_get_width(src_desc, src_miplevel_idx) ==
            d3d12_resource_desc_get_width(dst_desc, dst_miplevel_idx));
    VKD3D_ASSERT(d3d12_resource_desc_get_height(src_desc, src_miplevel_idx) ==
            d3d12_resource_desc_get_height(dst_desc, dst_miplevel_idx));
    VKD3D_ASSERT(d3d12_resource_desc_get_depth(src_desc, src_miplevel_idx) ==
            d3d12_resource_desc_get_depth(dst_desc, dst_miplevel_idx));

    VK_CALL(vkCmdCopyBufferToImage(list->vk_command_buffer,
            transfer_buffer.vk_buffer, dst_resource->u.vk_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy));
}

static bool validate_d3d12_box(const D3D12_BOX *box)
{
    return box->right > box->left
            && box->bottom > box->top
            && box->back > box->front;
}

static void STDMETHODCALLTYPE d3d12_command_list_CopyTextureRegion(ID3D12GraphicsCommandList6 *iface,
        const D3D12_TEXTURE_COPY_LOCATION *dst, UINT dst_x, UINT dst_y, UINT dst_z,
        const D3D12_TEXTURE_COPY_LOCATION *src, const D3D12_BOX *src_box)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct d3d12_resource *dst_resource, *src_resource;
    const struct vkd3d_format *src_format, *dst_format;
    const struct vkd3d_vk_device_procs *vk_procs;
    VkBufferImageCopy buffer_image_copy;
    VkImageCopy image_copy;

    TRACE("iface %p, dst %p, dst_x %u, dst_y %u, dst_z %u, src %p, src_box %p.\n",
            iface, dst, dst_x, dst_y, dst_z, src, src_box);

    if (src_box && !validate_d3d12_box(src_box))
    {
        WARN("Empty box %s.\n", debug_d3d12_box(src_box));
        return;
    }

    vk_procs = &list->device->vk_procs;

    dst_resource = unsafe_impl_from_ID3D12Resource(dst->pResource);
    src_resource = unsafe_impl_from_ID3D12Resource(src->pResource);

    d3d12_command_list_track_resource_usage(list, dst_resource);
    d3d12_command_list_track_resource_usage(list, src_resource);

    d3d12_command_list_end_current_render_pass(list);

    if (src->Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
            && dst->Type == D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT)
    {
        VKD3D_ASSERT(d3d12_resource_is_buffer(dst_resource));
        VKD3D_ASSERT(d3d12_resource_is_texture(src_resource));

        if (!(dst_format = vkd3d_format_from_d3d12_resource_desc(list->device,
                &src_resource->desc, dst->u.PlacedFootprint.Footprint.Format)))
        {
            WARN("Invalid format %#x.\n", dst->u.PlacedFootprint.Footprint.Format);
            return;
        }

        if (dst_format->is_emulated)
        {
            FIXME("Format %#x is not supported yet.\n", dst_format->dxgi_format);
            return;
        }

        if ((dst_format->vk_aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT)
                && (dst_format->vk_aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT))
            FIXME("Depth-stencil format %#x not fully supported yet.\n", dst_format->dxgi_format);

        vk_image_buffer_copy_from_d3d12(&buffer_image_copy, &dst->u.PlacedFootprint,
                src->u.SubresourceIndex, &src_resource->desc, dst_format, src_box, dst_x, dst_y, dst_z);
        VK_CALL(vkCmdCopyImageToBuffer(list->vk_command_buffer,
                src_resource->u.vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                dst_resource->u.vk_buffer, 1, &buffer_image_copy));
    }
    else if (src->Type == D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT
            && dst->Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)
    {
        VKD3D_ASSERT(d3d12_resource_is_texture(dst_resource));
        VKD3D_ASSERT(d3d12_resource_is_buffer(src_resource));

        if (!(src_format = vkd3d_format_from_d3d12_resource_desc(list->device,
                &dst_resource->desc, src->u.PlacedFootprint.Footprint.Format)))
        {
            WARN("Invalid format %#x.\n", src->u.PlacedFootprint.Footprint.Format);
            return;
        }

        if (src_format->is_emulated)
        {
            FIXME("Format %#x is not supported yet.\n", src_format->dxgi_format);
            return;
        }

        if ((src_format->vk_aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT)
                && (src_format->vk_aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT))
            FIXME("Depth-stencil format %#x not fully supported yet.\n", src_format->dxgi_format);

        vk_buffer_image_copy_from_d3d12(&buffer_image_copy, &src->u.PlacedFootprint,
                dst->u.SubresourceIndex, &dst_resource->desc, src_format, src_box, dst_x, dst_y, dst_z);
        VK_CALL(vkCmdCopyBufferToImage(list->vk_command_buffer,
                src_resource->u.vk_buffer, dst_resource->u.vk_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy));
    }
    else if (src->Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX
            && dst->Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)
    {
        VKD3D_ASSERT(d3d12_resource_is_texture(dst_resource));
        VKD3D_ASSERT(d3d12_resource_is_texture(src_resource));

        dst_format = dst_resource->format;
        src_format = src_resource->format;

        if ((dst_format->vk_aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT)
                && (dst_format->vk_aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT))
            FIXME("Depth-stencil format %#x not fully supported yet.\n", dst_format->dxgi_format);
        if ((src_format->vk_aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT)
                && (src_format->vk_aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT))
            FIXME("Depth-stencil format %#x not fully supported yet.\n", src_format->dxgi_format);

        if (dst_format->vk_aspect_mask != src_format->vk_aspect_mask)
        {
            d3d12_command_list_copy_incompatible_texture_region(list,
                    dst_resource, dst->u.SubresourceIndex, dst_format,
                    src_resource, src->u.SubresourceIndex, src_format, 1);
            return;
        }

        vk_image_copy_from_d3d12(&image_copy, src->u.SubresourceIndex, dst->u.SubresourceIndex,
                 &src_resource->desc, &dst_resource->desc, src_format, dst_format,
                 src_box, dst_x, dst_y, dst_z);
        VK_CALL(vkCmdCopyImage(list->vk_command_buffer, src_resource->u.vk_image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_resource->u.vk_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy));
    }
    else
    {
        FIXME("Copy type %#x -> %#x not implemented.\n", src->Type, dst->Type);
    }
}

static void STDMETHODCALLTYPE d3d12_command_list_CopyResource(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *dst, ID3D12Resource *src)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct d3d12_resource *dst_resource, *src_resource;
    const struct vkd3d_format *dst_format, *src_format;
    const struct vkd3d_vk_device_procs *vk_procs;
    VkBufferCopy vk_buffer_copy;
    VkImageCopy vk_image_copy;
    unsigned int layer_count;
    unsigned int i;

    TRACE("iface %p, dst_resource %p, src_resource %p.\n", iface, dst, src);

    vk_procs = &list->device->vk_procs;

    dst_resource = unsafe_impl_from_ID3D12Resource(dst);
    src_resource = unsafe_impl_from_ID3D12Resource(src);

    d3d12_command_list_track_resource_usage(list, dst_resource);
    d3d12_command_list_track_resource_usage(list, src_resource);

    d3d12_command_list_end_current_render_pass(list);

    if (d3d12_resource_is_buffer(dst_resource))
    {
        VKD3D_ASSERT(d3d12_resource_is_buffer(src_resource));
        VKD3D_ASSERT(src_resource->desc.Width == dst_resource->desc.Width);

        vk_buffer_copy.srcOffset = 0;
        vk_buffer_copy.dstOffset = 0;
        vk_buffer_copy.size = dst_resource->desc.Width;
        VK_CALL(vkCmdCopyBuffer(list->vk_command_buffer,
                src_resource->u.vk_buffer, dst_resource->u.vk_buffer, 1, &vk_buffer_copy));
    }
    else
    {
        layer_count = d3d12_resource_desc_get_layer_count(&dst_resource->desc);
        dst_format = dst_resource->format;
        src_format = src_resource->format;

        VKD3D_ASSERT(d3d12_resource_is_texture(dst_resource));
        VKD3D_ASSERT(d3d12_resource_is_texture(src_resource));
        VKD3D_ASSERT(dst_resource->desc.MipLevels == src_resource->desc.MipLevels);
        VKD3D_ASSERT(layer_count == d3d12_resource_desc_get_layer_count(&src_resource->desc));

        if (src_format->vk_aspect_mask != dst_format->vk_aspect_mask)
        {
            for (i = 0; i < dst_resource->desc.MipLevels; ++i)
            {
                d3d12_command_list_copy_incompatible_texture_region(list,
                        dst_resource, i, dst_format,
                        src_resource, i, src_format, layer_count);
            }
            return;
        }

        for (i = 0; i < dst_resource->desc.MipLevels; ++i)
        {
            vk_image_copy_from_d3d12(&vk_image_copy, i, i, &src_resource->desc, &dst_resource->desc,
                    src_format, dst_format, NULL, 0, 0, 0);
            vk_image_copy.dstSubresource.layerCount = layer_count;
            vk_image_copy.srcSubresource.layerCount = layer_count;
            VK_CALL(vkCmdCopyImage(list->vk_command_buffer, src_resource->u.vk_image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_resource->u.vk_image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_image_copy));
        }
    }
}

static void STDMETHODCALLTYPE d3d12_command_list_CopyTiles(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *tiled_resource, const D3D12_TILED_RESOURCE_COORDINATE *tile_region_start_coordinate,
        const D3D12_TILE_REGION_SIZE *tile_region_size, ID3D12Resource *buffer, UINT64 buffer_offset,
        D3D12_TILE_COPY_FLAGS flags)
{
    FIXME("iface %p, tiled_resource %p, tile_region_start_coordinate %p, tile_region_size %p, "
            "buffer %p, buffer_offset %#"PRIx64", flags %#x stub!\n",
            iface, tiled_resource, tile_region_start_coordinate, tile_region_size,
            buffer, buffer_offset, flags);
}

static void STDMETHODCALLTYPE d3d12_command_list_ResolveSubresource(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *dst, UINT dst_sub_resource_idx,
        ID3D12Resource *src, UINT src_sub_resource_idx, DXGI_FORMAT format)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_format *src_format, *dst_format, *vk_format;
    struct d3d12_resource *dst_resource, *src_resource;
    const struct vkd3d_vk_device_procs *vk_procs;
    const struct d3d12_device *device;
    VkImageResolve vk_image_resolve;

    TRACE("iface %p, dst_resource %p, dst_sub_resource_idx %u, src_resource %p, src_sub_resource_idx %u, "
            "format %#x.\n", iface, dst, dst_sub_resource_idx, src, src_sub_resource_idx, format);

    device = list->device;
    vk_procs = &device->vk_procs;

    dst_resource = unsafe_impl_from_ID3D12Resource(dst);
    src_resource = unsafe_impl_from_ID3D12Resource(src);

    VKD3D_ASSERT(d3d12_resource_is_texture(dst_resource));
    VKD3D_ASSERT(d3d12_resource_is_texture(src_resource));

    d3d12_command_list_track_resource_usage(list, dst_resource);
    d3d12_command_list_track_resource_usage(list, src_resource);

    d3d12_command_list_end_current_render_pass(list);

    dst_format = dst_resource->format;
    src_format = src_resource->format;

    if (dst_format->type == VKD3D_FORMAT_TYPE_TYPELESS || src_format->type == VKD3D_FORMAT_TYPE_TYPELESS)
    {
        if (!(vk_format = vkd3d_format_from_d3d12_resource_desc(device, &dst_resource->desc, format)))
        {
            WARN("Invalid format %#x.\n", format);
            return;
        }
        if (dst_format->vk_format != src_format->vk_format || dst_format->vk_format != vk_format->vk_format)
        {
            FIXME("Not implemented for typeless resources.\n");
            return;
        }
    }

    /* Resolve of depth/stencil images is not supported in Vulkan. */
    if ((dst_format->vk_aspect_mask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
            || (src_format->vk_aspect_mask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)))
    {
        FIXME("Resolve of depth/stencil images is not implemented yet.\n");
        return;
    }

    vk_image_subresource_layers_from_d3d12(&vk_image_resolve.srcSubresource,
            src_format, src_sub_resource_idx, src_resource->desc.MipLevels);
    memset(&vk_image_resolve.srcOffset, 0, sizeof(vk_image_resolve.srcOffset));
    vk_image_subresource_layers_from_d3d12(&vk_image_resolve.dstSubresource,
            dst_format, dst_sub_resource_idx, dst_resource->desc.MipLevels);
    memset(&vk_image_resolve.dstOffset, 0, sizeof(vk_image_resolve.dstOffset));
    vk_extent_3d_from_d3d12_miplevel(&vk_image_resolve.extent,
            &dst_resource->desc, vk_image_resolve.dstSubresource.mipLevel);

    VK_CALL(vkCmdResolveImage(list->vk_command_buffer, src_resource->u.vk_image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_resource->u.vk_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_image_resolve));
}

static void STDMETHODCALLTYPE d3d12_command_list_IASetPrimitiveTopology(ID3D12GraphicsCommandList6 *iface,
        D3D12_PRIMITIVE_TOPOLOGY topology)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, topology %#x.\n", iface, topology);

    if (list->primitive_topology == topology)
        return;

    list->primitive_topology = topology;
    d3d12_command_list_invalidate_current_pipeline(list);
}

static void STDMETHODCALLTYPE d3d12_command_list_RSSetViewports(ID3D12GraphicsCommandList6 *iface,
        UINT viewport_count, const D3D12_VIEWPORT *viewports)
{
    VkViewport vk_viewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_vk_device_procs *vk_procs;
    unsigned int i;

    TRACE("iface %p, viewport_count %u, viewports %p.\n", iface, viewport_count, viewports);

    if (viewport_count > ARRAY_SIZE(vk_viewports))
    {
        FIXME("Viewport count %u > D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE.\n", viewport_count);
        viewport_count = ARRAY_SIZE(vk_viewports);
    }

    for (i = 0; i < viewport_count; ++i)
    {
        vk_viewports[i].x = viewports[i].TopLeftX;
        vk_viewports[i].y = viewports[i].TopLeftY + viewports[i].Height;
        vk_viewports[i].width = viewports[i].Width;
        vk_viewports[i].height = -viewports[i].Height;
        vk_viewports[i].minDepth = viewports[i].MinDepth;
        vk_viewports[i].maxDepth = viewports[i].MaxDepth;

        if (vk_viewports[i].width <= 0.0f)
        {
            /* Vulkan does not support width <= 0 */
            FIXME_ONCE("Setting invalid viewport %u to zero height.\n", i);
            vk_viewports[i].width = 1.0f;
            vk_viewports[i].height = 0.0f;
        }
    }

    vk_procs = &list->device->vk_procs;
    VK_CALL(vkCmdSetViewport(list->vk_command_buffer, 0, viewport_count, vk_viewports));
}

static void STDMETHODCALLTYPE d3d12_command_list_RSSetScissorRects(ID3D12GraphicsCommandList6 *iface,
        UINT rect_count, const D3D12_RECT *rects)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    VkRect2D vk_rects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    const struct vkd3d_vk_device_procs *vk_procs;
    unsigned int i;

    TRACE("iface %p, rect_count %u, rects %p.\n", iface, rect_count, rects);

    if (rect_count > ARRAY_SIZE(vk_rects))
    {
        FIXME("Rect count %u > D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE.\n", rect_count);
        rect_count = ARRAY_SIZE(vk_rects);
    }

    for (i = 0; i < rect_count; ++i)
    {
        vk_rects[i].offset.x = rects[i].left;
        vk_rects[i].offset.y = rects[i].top;
        vk_rects[i].extent.width = rects[i].right - rects[i].left;
        vk_rects[i].extent.height = rects[i].bottom - rects[i].top;
    }

    vk_procs = &list->device->vk_procs;
    VK_CALL(vkCmdSetScissor(list->vk_command_buffer, 0, rect_count, vk_rects));
}

static void STDMETHODCALLTYPE d3d12_command_list_OMSetBlendFactor(ID3D12GraphicsCommandList6 *iface,
        const FLOAT blend_factor[4])
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_vk_device_procs *vk_procs;

    TRACE("iface %p, blend_factor %p.\n", iface, blend_factor);

    vk_procs = &list->device->vk_procs;
    VK_CALL(vkCmdSetBlendConstants(list->vk_command_buffer, blend_factor));
}

static void STDMETHODCALLTYPE d3d12_command_list_OMSetStencilRef(ID3D12GraphicsCommandList6 *iface,
        UINT stencil_ref)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_vk_device_procs *vk_procs;

    TRACE("iface %p, stencil_ref %u.\n", iface, stencil_ref);

    vk_procs = &list->device->vk_procs;
    VK_CALL(vkCmdSetStencilReference(list->vk_command_buffer, VK_STENCIL_FRONT_AND_BACK, stencil_ref));
}

static void STDMETHODCALLTYPE d3d12_command_list_SetPipelineState(ID3D12GraphicsCommandList6 *iface,
        ID3D12PipelineState *pipeline_state)
{
    struct d3d12_pipeline_state *state = unsafe_impl_from_ID3D12PipelineState(pipeline_state);
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, pipeline_state %p.\n", iface, pipeline_state);

    if (list->state == state)
        return;

    d3d12_command_list_invalidate_bindings(list, state);
    d3d12_command_list_invalidate_current_pipeline(list);

    list->state = state;
}

static bool is_ds_multiplanar_resolvable(unsigned int first_state, unsigned int second_state)
{
    /* Only combinations of depth/stencil read/write are supported. */
    return first_state == second_state
            || ((first_state & (D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_DEPTH_WRITE))
            && (second_state & (D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_DEPTH_WRITE)));
}

static unsigned int d3d12_find_ds_multiplanar_transition(const D3D12_RESOURCE_BARRIER *barriers,
        unsigned int i, unsigned int barrier_count, unsigned int sub_resource_count)
{
    unsigned int sub_resource_idx = barriers[i].u.Transition.Subresource;
    unsigned int j;

    for (j = i + 1; j < barrier_count; ++j)
    {
        if (barriers[j].Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION
                && barriers[j].u.Transition.pResource == barriers[i].u.Transition.pResource
                && sub_resource_idx % sub_resource_count == barriers[j].u.Transition.Subresource % sub_resource_count)
        {
            /* Second barrier must be for a different plane. */
            if (barriers[j].u.Transition.Subresource == sub_resource_idx)
                return 0;

            /* Validate the second barrier and check if the combination of two states is supported. */
            if (!is_valid_resource_state(barriers[j].u.Transition.StateBefore)
                    || !is_ds_multiplanar_resolvable(barriers[i].u.Transition.StateBefore, barriers[j].u.Transition.StateBefore)
                    || !is_valid_resource_state(barriers[j].u.Transition.StateAfter)
                    || !is_ds_multiplanar_resolvable(barriers[i].u.Transition.StateAfter, barriers[j].u.Transition.StateAfter)
                    || barriers[j].u.Transition.Subresource >= sub_resource_count * 2u)
                return 0;

            return j;
        }
    }
    return 0;
}

static void STDMETHODCALLTYPE d3d12_command_list_ResourceBarrier(ID3D12GraphicsCommandList6 *iface,
        UINT barrier_count, const D3D12_RESOURCE_BARRIER *barriers)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    bool have_aliasing_barriers = false, have_split_barriers = false;
    const struct vkd3d_vk_device_procs *vk_procs;
    const struct vkd3d_vulkan_info *vk_info;
    bool *multiplanar_handled = NULL;
    unsigned int i;

    TRACE("iface %p, barrier_count %u, barriers %p.\n", iface, barrier_count, barriers);

    vk_procs = &list->device->vk_procs;
    vk_info = &list->device->vk_info;

    d3d12_command_list_end_current_render_pass(list);

    for (i = 0; i < barrier_count; ++i)
    {
        unsigned int sub_resource_idx = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        VkPipelineStageFlags src_stage_mask = 0, dst_stage_mask = 0;
        VkAccessFlags src_access_mask = 0, dst_access_mask = 0;
        const D3D12_RESOURCE_BARRIER *current = &barriers[i];
        VkImageLayout layout_before, layout_after;
        struct d3d12_resource *resource;

        have_split_barriers = have_split_barriers
                || (current->Flags & D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY)
                || (current->Flags & D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

        if (current->Flags & D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY)
            continue;

        switch (current->Type)
        {
            case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
            {
                unsigned int state_before, state_after, stencil_state_before = 0, stencil_state_after = 0;
                const D3D12_RESOURCE_TRANSITION_BARRIER *transition = &current->u.Transition;

                if (!is_valid_resource_state(transition->StateBefore))
                {
                    d3d12_command_list_mark_as_invalid(list,
                            "Invalid StateBefore %#x (barrier %u).", transition->StateBefore, i);
                    continue;
                }
                if (!is_valid_resource_state(transition->StateAfter))
                {
                    d3d12_command_list_mark_as_invalid(list,
                            "Invalid StateAfter %#x (barrier %u).", transition->StateAfter, i);
                    continue;
                }

                if (!(resource = unsafe_impl_from_ID3D12Resource(transition->pResource)))
                {
                    d3d12_command_list_mark_as_invalid(list, "A resource pointer is NULL.");
                    continue;
                }

                if (multiplanar_handled && multiplanar_handled[i])
                    continue;

                state_before = transition->StateBefore;
                state_after = transition->StateAfter;

                sub_resource_idx = transition->Subresource;

                if (sub_resource_idx != D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
                        && (resource->desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
                {
                    unsigned int sub_resource_count = d3d12_resource_desc_get_sub_resource_count(&resource->desc);
                    unsigned int j = d3d12_find_ds_multiplanar_transition(barriers, i, barrier_count, sub_resource_count);
                    if (j && (multiplanar_handled || (multiplanar_handled = vkd3d_calloc(barrier_count, sizeof(*multiplanar_handled)))))
                    {
                        multiplanar_handled[j] = true;
                        if (sub_resource_idx >= sub_resource_count)
                        {
                            sub_resource_idx -= sub_resource_count;
                            /* The stencil barrier is at i, depth at j. */
                            state_before = barriers[j].u.Transition.StateBefore;
                            state_after = barriers[j].u.Transition.StateAfter;
                            stencil_state_before = transition->StateBefore;
                            stencil_state_after = transition->StateAfter;
                        }
                        else
                        {
                            /* Depth at i, stencil at j. */
                            stencil_state_before = barriers[j].u.Transition.StateBefore;
                            stencil_state_after = barriers[j].u.Transition.StateAfter;
                        }
                    }
                    else if (sub_resource_idx >= sub_resource_count)
                    {
                        FIXME_ONCE("Unhandled sub-resource idx %u.\n", sub_resource_idx);
                        continue;
                    }
                }

                if (!vk_barrier_parameters_from_d3d12_resource_state(state_before, stencil_state_before,
                        resource, list->vk_queue_flags, vk_info, &src_access_mask,
                        &src_stage_mask, &layout_before, list->device))
                {
                    FIXME("Unhandled state %#x.\n", state_before);
                    continue;
                }
                if (!vk_barrier_parameters_from_d3d12_resource_state(state_after, stencil_state_after,
                        resource, list->vk_queue_flags, vk_info, &dst_access_mask,
                        &dst_stage_mask, &layout_after, list->device))
                {
                    FIXME("Unhandled state %#x.\n", state_after);
                    continue;
                }

                TRACE("Transition barrier (resource %p, subresource %#x, before %#x, after %#x).\n",
                        resource, transition->Subresource, transition->StateBefore, transition->StateAfter);
                break;
            }

            case D3D12_RESOURCE_BARRIER_TYPE_UAV:
            {
                const D3D12_RESOURCE_UAV_BARRIER *uav = &current->u.UAV;
                VkPipelineStageFlags stage_mask;
                VkImageLayout image_layout;
                VkAccessFlags access_mask;

                resource = unsafe_impl_from_ID3D12Resource(uav->pResource);
                vk_barrier_parameters_from_d3d12_resource_state(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0,
                        resource, list->vk_queue_flags, vk_info, &access_mask,
                        &stage_mask, &image_layout, list->device);
                src_access_mask = dst_access_mask = access_mask;
                src_stage_mask = dst_stage_mask = stage_mask;
                layout_before = layout_after = image_layout;

                TRACE("UAV barrier (resource %p).\n", resource);
                break;
            }

            case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
                have_aliasing_barriers = true;
                continue;
            default:
                WARN("Invalid barrier type %#x.\n", current->Type);
                continue;
        }

        if (resource)
            d3d12_command_list_track_resource_usage(list, resource);

        if (!resource)
        {
            VkMemoryBarrier vk_barrier;

            vk_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            vk_barrier.pNext = NULL;
            vk_barrier.srcAccessMask = src_access_mask;
            vk_barrier.dstAccessMask = dst_access_mask;

            VK_CALL(vkCmdPipelineBarrier(list->vk_command_buffer, src_stage_mask, dst_stage_mask, 0,
                    1, &vk_barrier, 0, NULL, 0, NULL));
        }
        else if (d3d12_resource_is_buffer(resource))
        {
            VkBufferMemoryBarrier vk_barrier;

            vk_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            vk_barrier.pNext = NULL;
            vk_barrier.srcAccessMask = src_access_mask;
            vk_barrier.dstAccessMask = dst_access_mask;
            vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.buffer = resource->u.vk_buffer;
            vk_barrier.offset = 0;
            vk_barrier.size = VK_WHOLE_SIZE;

            VK_CALL(vkCmdPipelineBarrier(list->vk_command_buffer, src_stage_mask, dst_stage_mask, 0,
                    0, NULL, 1, &vk_barrier, 0, NULL));
        }
        else
        {
            VkImageMemoryBarrier vk_barrier;

            vk_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vk_barrier.pNext = NULL;
            vk_barrier.srcAccessMask = src_access_mask;
            vk_barrier.dstAccessMask = dst_access_mask;
            vk_barrier.oldLayout = layout_before;
            vk_barrier.newLayout = layout_after;
            vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_barrier.image = resource->u.vk_image;

            vk_barrier.subresourceRange.aspectMask = resource->format->vk_aspect_mask;
            if (sub_resource_idx == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
            {
                vk_barrier.subresourceRange.baseMipLevel = 0;
                vk_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                vk_barrier.subresourceRange.baseArrayLayer = 0;
                vk_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            }
            else
            {
                /* FIXME: Some formats in D3D12 are planar. Each plane is a separate sub-resource. */
                if (sub_resource_idx >= d3d12_resource_desc_get_sub_resource_count(&resource->desc))
                {
                    FIXME_ONCE("Unhandled sub-resource idx %u.\n", sub_resource_idx);
                    continue;
                }

                vk_barrier.subresourceRange.baseMipLevel = sub_resource_idx % resource->desc.MipLevels;
                vk_barrier.subresourceRange.levelCount = 1;
                vk_barrier.subresourceRange.baseArrayLayer = sub_resource_idx / resource->desc.MipLevels;
                vk_barrier.subresourceRange.layerCount = 1;
            }

            VK_CALL(vkCmdPipelineBarrier(list->vk_command_buffer, src_stage_mask, dst_stage_mask, 0,
                    0, NULL, 0, NULL, 1, &vk_barrier));
        }
    }

    vkd3d_free(multiplanar_handled);

    if (have_aliasing_barriers)
        FIXME_ONCE("Aliasing barriers not implemented yet.\n");

    /* Vulkan doesn't support split barriers. */
    if (have_split_barriers)
        WARN("Issuing split barrier(s) on D3D12_RESOURCE_BARRIER_FLAG_END_ONLY.\n");
}

static void STDMETHODCALLTYPE d3d12_command_list_ExecuteBundle(ID3D12GraphicsCommandList6 *iface,
        ID3D12GraphicsCommandList *command_list)
{
    FIXME("iface %p, command_list %p stub!\n", iface, command_list);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetDescriptorHeaps(ID3D12GraphicsCommandList6 *iface,
        UINT heap_count, ID3D12DescriptorHeap *const *heaps)
{
    TRACE("iface %p, heap_count %u, heaps %p.\n", iface, heap_count, heaps);

    /* Our current implementation does not need this method.
     * In Windows it doesn't need to be called at all for correct operation, and
     * at least on AMD the wrong heaps can be set here and tests still succeed.
     *
     * It could be used to validate descriptor tables but we do not have an
     * equivalent of the D3D12 Debug Layer. */
}

static void d3d12_command_list_set_root_signature(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point, const struct d3d12_root_signature *root_signature)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];

    if (bindings->root_signature == root_signature)
        return;

    bindings->root_signature = root_signature;

    d3d12_command_list_invalidate_root_parameters(list, bind_point);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetComputeRootSignature(ID3D12GraphicsCommandList6 *iface,
        ID3D12RootSignature *root_signature)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_signature %p.\n", iface, root_signature);

    d3d12_command_list_set_root_signature(list, VKD3D_PIPELINE_BIND_POINT_COMPUTE,
            unsafe_impl_from_ID3D12RootSignature(root_signature));
}

static void STDMETHODCALLTYPE d3d12_command_list_SetGraphicsRootSignature(ID3D12GraphicsCommandList6 *iface,
        ID3D12RootSignature *root_signature)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_signature %p.\n", iface, root_signature);

    d3d12_command_list_set_root_signature(list, VKD3D_PIPELINE_BIND_POINT_GRAPHICS,
            unsafe_impl_from_ID3D12RootSignature(root_signature));
}

static void d3d12_command_list_set_descriptor_table(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point, unsigned int index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    const struct d3d12_root_signature *root_signature = bindings->root_signature;
    struct d3d12_descriptor_heap *descriptor_heap;
    struct d3d12_desc *desc;

    VKD3D_ASSERT(root_signature_get_descriptor_table(root_signature, index));

    VKD3D_ASSERT(index < ARRAY_SIZE(bindings->descriptor_tables));
    desc = d3d12_desc_from_gpu_handle(base_descriptor);

    if (bindings->descriptor_tables[index] == desc)
        return;

    descriptor_heap = d3d12_desc_get_descriptor_heap(desc);
    if (!(descriptor_heap->desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
    {
        /* GetGPUDescriptorHandleForHeapStart() returns a null handle in this case,
         * but a CPU handle could be passed. */
        WARN("Descriptor heap %p is not shader visible.\n", descriptor_heap);
        return;
    }
    command_list_add_descriptor_heap(list, descriptor_heap);

    bindings->descriptor_tables[index] = desc;
    bindings->descriptor_table_dirty_mask |= (uint64_t)1 << index;
    bindings->descriptor_table_active_mask |= (uint64_t)1 << index;
}

static void STDMETHODCALLTYPE d3d12_command_list_SetComputeRootDescriptorTable(ID3D12GraphicsCommandList6 *iface,
        UINT root_parameter_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, base_descriptor %s.\n",
            iface, root_parameter_index, debug_gpu_handle(base_descriptor));

    d3d12_command_list_set_descriptor_table(list, VKD3D_PIPELINE_BIND_POINT_COMPUTE,
            root_parameter_index, base_descriptor);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList6 *iface,
        UINT root_parameter_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, base_descriptor %s.\n",
            iface, root_parameter_index, debug_gpu_handle(base_descriptor));

    d3d12_command_list_set_descriptor_table(list, VKD3D_PIPELINE_BIND_POINT_GRAPHICS,
            root_parameter_index, base_descriptor);
}

static void d3d12_command_list_set_root_constants(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point, unsigned int index, unsigned int offset,
        unsigned int count, const void *data)
{
    const struct d3d12_root_signature *root_signature = list->pipeline_bindings[bind_point].root_signature;
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct d3d12_root_constant *c;

    c = root_signature_get_32bit_constants(root_signature, index);
    VK_CALL(vkCmdPushConstants(list->vk_command_buffer, root_signature->vk_pipeline_layout,
            c->stage_flags, c->offset + offset * sizeof(uint32_t), count * sizeof(uint32_t), data));
}

static void STDMETHODCALLTYPE d3d12_command_list_SetComputeRoot32BitConstant(ID3D12GraphicsCommandList6 *iface,
        UINT root_parameter_index, UINT data, UINT dst_offset)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, data 0x%08x, dst_offset %u.\n",
            iface, root_parameter_index, data, dst_offset);

    d3d12_command_list_set_root_constants(list, VKD3D_PIPELINE_BIND_POINT_COMPUTE,
            root_parameter_index, dst_offset, 1, &data);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetGraphicsRoot32BitConstant(ID3D12GraphicsCommandList6 *iface,
        UINT root_parameter_index, UINT data, UINT dst_offset)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, data 0x%08x, dst_offset %u.\n",
            iface, root_parameter_index, data, dst_offset);

    d3d12_command_list_set_root_constants(list, VKD3D_PIPELINE_BIND_POINT_GRAPHICS,
            root_parameter_index, dst_offset, 1, &data);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetComputeRoot32BitConstants(ID3D12GraphicsCommandList6 *iface,
        UINT root_parameter_index, UINT constant_count, const void *data, UINT dst_offset)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, constant_count %u, data %p, dst_offset %u.\n",
            iface, root_parameter_index, constant_count, data, dst_offset);

    d3d12_command_list_set_root_constants(list, VKD3D_PIPELINE_BIND_POINT_COMPUTE,
            root_parameter_index, dst_offset, constant_count, data);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetGraphicsRoot32BitConstants(ID3D12GraphicsCommandList6 *iface,
        UINT root_parameter_index, UINT constant_count, const void *data, UINT dst_offset)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, constant_count %u, data %p, dst_offset %u.\n",
            iface, root_parameter_index, constant_count, data, dst_offset);

    d3d12_command_list_set_root_constants(list, VKD3D_PIPELINE_BIND_POINT_GRAPHICS,
            root_parameter_index, dst_offset, constant_count, data);
}

static void d3d12_command_list_set_root_cbv(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point, unsigned int index, D3D12_GPU_VIRTUAL_ADDRESS gpu_address)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    const struct d3d12_root_signature *root_signature = bindings->root_signature;
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct vkd3d_vulkan_info *vk_info = &list->device->vk_info;
    const struct d3d12_root_parameter *root_parameter;
    struct VkWriteDescriptorSet descriptor_write;
    struct VkDescriptorBufferInfo buffer_info;
    struct d3d12_resource *resource;

    root_parameter = root_signature_get_root_descriptor(root_signature, index);
    VKD3D_ASSERT(root_parameter->parameter_type == D3D12_ROOT_PARAMETER_TYPE_CBV);

    if (gpu_address)
    {
        resource = vkd3d_gpu_va_allocator_dereference(&list->device->gpu_va_allocator, gpu_address);
        buffer_info.buffer = resource->u.vk_buffer;
        buffer_info.offset = gpu_address - resource->gpu_address;
        buffer_info.range = resource->desc.Width - buffer_info.offset;
        buffer_info.range = min(buffer_info.range, vk_info->device_limits.maxUniformBufferRange);
    }
    else
    {
        buffer_info.buffer = list->device->null_resources.vk_buffer;
        buffer_info.offset = 0;
        buffer_info.range = VK_WHOLE_SIZE;
    }

    if (vk_info->KHR_push_descriptor)
    {
        vk_write_descriptor_set_from_root_descriptor(&descriptor_write,
                root_parameter, VK_NULL_HANDLE, NULL, &buffer_info);
        VK_CALL(vkCmdPushDescriptorSetKHR(list->vk_command_buffer, bindings->vk_bind_point,
                root_signature->vk_pipeline_layout, 0, 1, &descriptor_write));
    }
    else
    {
        d3d12_command_list_prepare_descriptors(list, bind_point);
        vk_write_descriptor_set_from_root_descriptor(&descriptor_write,
                root_parameter, bindings->descriptor_sets[0], NULL, &buffer_info);
        VK_CALL(vkUpdateDescriptorSets(list->device->vk_device, 1, &descriptor_write, 0, NULL));

        VKD3D_ASSERT(index < ARRAY_SIZE(bindings->push_descriptors));
        bindings->push_descriptors[index].u.cbv.vk_buffer = buffer_info.buffer;
        bindings->push_descriptors[index].u.cbv.offset = buffer_info.offset;
        bindings->push_descriptor_dirty_mask |= 1u << index;
        bindings->push_descriptor_active_mask |= 1u << index;
    }
}

static void STDMETHODCALLTYPE d3d12_command_list_SetComputeRootConstantBufferView(
        ID3D12GraphicsCommandList6 *iface, UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, address %#"PRIx64".\n",
            iface, root_parameter_index, address);

    d3d12_command_list_set_root_cbv(list, VKD3D_PIPELINE_BIND_POINT_COMPUTE, root_parameter_index, address);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetGraphicsRootConstantBufferView(
        ID3D12GraphicsCommandList6 *iface, UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, address %#"PRIx64".\n",
            iface, root_parameter_index, address);

    d3d12_command_list_set_root_cbv(list, VKD3D_PIPELINE_BIND_POINT_GRAPHICS, root_parameter_index, address);
}

static void d3d12_command_list_set_root_descriptor(struct d3d12_command_list *list,
        enum vkd3d_pipeline_bind_point bind_point, unsigned int index, D3D12_GPU_VIRTUAL_ADDRESS gpu_address)
{
    struct vkd3d_pipeline_bindings *bindings = &list->pipeline_bindings[bind_point];
    const struct d3d12_root_signature *root_signature = bindings->root_signature;
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    const struct vkd3d_vulkan_info *vk_info = &list->device->vk_info;
    const struct d3d12_root_parameter *root_parameter;
    struct VkWriteDescriptorSet descriptor_write;
    VkDevice vk_device = list->device->vk_device;
    VkBufferView vk_buffer_view;

    root_parameter = root_signature_get_root_descriptor(root_signature, index);
    VKD3D_ASSERT(root_parameter->parameter_type != D3D12_ROOT_PARAMETER_TYPE_CBV);

    /* FIXME: Re-use buffer views. */
    if (!vkd3d_create_raw_buffer_view(list->device, gpu_address, root_parameter->parameter_type, &vk_buffer_view))
    {
        ERR("Failed to create buffer view.\n");
        return;
    }

    if (vk_buffer_view && !(d3d12_command_allocator_add_buffer_view(list->allocator, vk_buffer_view)))
    {
        ERR("Failed to add buffer view.\n");
        VK_CALL(vkDestroyBufferView(vk_device, vk_buffer_view, NULL));
        return;
    }

    if (vk_info->KHR_push_descriptor)
    {
        vk_write_descriptor_set_from_root_descriptor(&descriptor_write,
                root_parameter, VK_NULL_HANDLE, &vk_buffer_view, NULL);
        VK_CALL(vkCmdPushDescriptorSetKHR(list->vk_command_buffer, bindings->vk_bind_point,
                root_signature->vk_pipeline_layout, 0, 1, &descriptor_write));
    }
    else
    {
        d3d12_command_list_prepare_descriptors(list, bind_point);
        vk_write_descriptor_set_from_root_descriptor(&descriptor_write,
                root_parameter, bindings->descriptor_sets[0], &vk_buffer_view,  NULL);
        VK_CALL(vkUpdateDescriptorSets(list->device->vk_device, 1, &descriptor_write, 0, NULL));

        VKD3D_ASSERT(index < ARRAY_SIZE(bindings->push_descriptors));
        bindings->push_descriptors[index].u.vk_buffer_view = vk_buffer_view;
        bindings->push_descriptor_dirty_mask |= 1u << index;
        bindings->push_descriptor_active_mask |= 1u << index;
    }
}

static void STDMETHODCALLTYPE d3d12_command_list_SetComputeRootShaderResourceView(
        ID3D12GraphicsCommandList6 *iface, UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, address %#"PRIx64".\n",
            iface, root_parameter_index, address);

    d3d12_command_list_set_root_descriptor(list, VKD3D_PIPELINE_BIND_POINT_COMPUTE,
            root_parameter_index, address);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetGraphicsRootShaderResourceView(
        ID3D12GraphicsCommandList6 *iface, UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, address %#"PRIx64".\n",
            iface, root_parameter_index, address);

    d3d12_command_list_set_root_descriptor(list, VKD3D_PIPELINE_BIND_POINT_GRAPHICS,
            root_parameter_index, address);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetComputeRootUnorderedAccessView(
        ID3D12GraphicsCommandList6 *iface, UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, address %#"PRIx64".\n",
            iface, root_parameter_index, address);

    d3d12_command_list_set_root_descriptor(list, VKD3D_PIPELINE_BIND_POINT_COMPUTE,
            root_parameter_index, address);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetGraphicsRootUnorderedAccessView(
        ID3D12GraphicsCommandList6 *iface, UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);

    TRACE("iface %p, root_parameter_index %u, address %#"PRIx64".\n",
            iface, root_parameter_index, address);

    d3d12_command_list_set_root_descriptor(list, VKD3D_PIPELINE_BIND_POINT_GRAPHICS,
            root_parameter_index, address);
}

static void STDMETHODCALLTYPE d3d12_command_list_IASetIndexBuffer(ID3D12GraphicsCommandList6 *iface,
        const D3D12_INDEX_BUFFER_VIEW *view)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_vk_device_procs *vk_procs;
    struct d3d12_resource *resource;
    enum VkIndexType index_type;

    TRACE("iface %p, view %p.\n", iface, view);

    if (!view)
    {
        WARN("Ignoring NULL index buffer view.\n");
        return;
    }
    if (!view->BufferLocation)
    {
        WARN("Ignoring index buffer location 0.\n");
        return;
    }

    vk_procs = &list->device->vk_procs;

    switch (view->Format)
    {
        case DXGI_FORMAT_R16_UINT:
            index_type = VK_INDEX_TYPE_UINT16;
            break;
        case DXGI_FORMAT_R32_UINT:
            index_type = VK_INDEX_TYPE_UINT32;
            break;
        default:
            WARN("Invalid index format %#x.\n", view->Format);
            return;
    }

    list->index_buffer_format = view->Format;

    resource = vkd3d_gpu_va_allocator_dereference(&list->device->gpu_va_allocator, view->BufferLocation);
    VK_CALL(vkCmdBindIndexBuffer(list->vk_command_buffer, resource->u.vk_buffer,
            view->BufferLocation - resource->gpu_address, index_type));
}

static void STDMETHODCALLTYPE d3d12_command_list_IASetVertexBuffers(ID3D12GraphicsCommandList6 *iface,
        UINT start_slot, UINT view_count, const D3D12_VERTEX_BUFFER_VIEW *views)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_null_resources *null_resources;
    struct vkd3d_gpu_va_allocator *gpu_va_allocator;
    VkDeviceSize offsets[ARRAY_SIZE(list->strides)];
    const struct vkd3d_vk_device_procs *vk_procs;
    VkBuffer buffers[ARRAY_SIZE(list->strides)];
    struct d3d12_device *device = list->device;
    unsigned int i, stride, max_view_count;
    struct d3d12_resource *resource;
    bool invalidate = false;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    vk_procs = &device->vk_procs;
    null_resources = &device->null_resources;
    gpu_va_allocator = &device->gpu_va_allocator;

    if (!vkd3d_bound_range(start_slot, view_count, ARRAY_SIZE(list->strides)))
    {
        WARN("Invalid start slot %u / view count %u.\n", start_slot, view_count);
        return;
    }

    max_view_count = device->vk_info.device_limits.maxVertexInputBindings;
    if (start_slot < max_view_count)
        max_view_count -= start_slot;
    else
        max_view_count = 0;

    /* Although simply skipping unsupported binding slots isn't especially
     * likely to work well in the general case, applications sometimes
     * explicitly set all 32 vertex buffer bindings slots supported by
     * Direct3D 12, with unused slots set to NULL. "Spider-Man Remastered" is
     * an example of such an application. */
    if (view_count > max_view_count)
    {
        for (i = max_view_count; i < view_count; ++i)
        {
            if (views && views[i].BufferLocation)
                WARN("Ignoring unsupported vertex buffer slot %u.\n", start_slot + i);
        }
        view_count = max_view_count;
    }

    for (i = 0; i < view_count; ++i)
    {
        if (views && views[i].BufferLocation)
        {
            resource = vkd3d_gpu_va_allocator_dereference(gpu_va_allocator, views[i].BufferLocation);
            buffers[i] = resource->u.vk_buffer;
            offsets[i] = views[i].BufferLocation - resource->gpu_address;
            stride = views[i].StrideInBytes;
        }
        else
        {
            buffers[i] = null_resources->vk_buffer;
            offsets[i] = 0;
            stride = 0;
        }

        invalidate |= list->strides[start_slot + i] != stride;
        list->strides[start_slot + i] = stride;
    }

    if (view_count)
        VK_CALL(vkCmdBindVertexBuffers(list->vk_command_buffer, start_slot, view_count, buffers, offsets));

    if (invalidate)
        d3d12_command_list_invalidate_current_pipeline(list);
}

static void STDMETHODCALLTYPE d3d12_command_list_SOSetTargets(ID3D12GraphicsCommandList6 *iface,
        UINT start_slot, UINT view_count, const D3D12_STREAM_OUTPUT_BUFFER_VIEW *views)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    VkDeviceSize offsets[ARRAY_SIZE(list->so_counter_buffers)];
    VkDeviceSize sizes[ARRAY_SIZE(list->so_counter_buffers)];
    VkBuffer buffers[ARRAY_SIZE(list->so_counter_buffers)];
    struct vkd3d_gpu_va_allocator *gpu_va_allocator;
    const struct vkd3d_vk_device_procs *vk_procs;
    struct d3d12_resource *resource;
    unsigned int i, first, count;

    TRACE("iface %p, start_slot %u, view_count %u, views %p.\n", iface, start_slot, view_count, views);

    d3d12_command_list_end_current_render_pass(list);

    if (!list->device->vk_info.EXT_transform_feedback)
    {
        FIXME("Transform feedback is not supported by Vulkan implementation.\n");
        return;
    }

    if (!vkd3d_bound_range(start_slot, view_count, ARRAY_SIZE(buffers)))
    {
        WARN("Invalid start slot %u / view count %u.\n", start_slot, view_count);
        return;
    }

    vk_procs = &list->device->vk_procs;
    gpu_va_allocator = &list->device->gpu_va_allocator;

    count = 0;
    first = start_slot;
    for (i = 0; i < view_count; ++i)
    {
        if (views[i].BufferLocation && views[i].SizeInBytes)
        {
            resource = vkd3d_gpu_va_allocator_dereference(gpu_va_allocator, views[i].BufferLocation);
            buffers[count] = resource->u.vk_buffer;
            offsets[count] = views[i].BufferLocation - resource->gpu_address;
            sizes[count] = views[i].SizeInBytes;

            resource = vkd3d_gpu_va_allocator_dereference(gpu_va_allocator, views[i].BufferFilledSizeLocation);
            list->so_counter_buffers[start_slot + i] = resource->u.vk_buffer;
            list->so_counter_buffer_offsets[start_slot + i] = views[i].BufferFilledSizeLocation - resource->gpu_address;
            ++count;
        }
        else
        {
            if (count)
                VK_CALL(vkCmdBindTransformFeedbackBuffersEXT(list->vk_command_buffer, first, count, buffers, offsets, sizes));
            count = 0;
            first = start_slot + i + 1;

            list->so_counter_buffers[start_slot + i] = VK_NULL_HANDLE;
            list->so_counter_buffer_offsets[start_slot + i] = 0;

            WARN("Trying to unbind transform feedback buffer %u. Ignoring.\n", start_slot + i);
        }
    }

    if (count)
        VK_CALL(vkCmdBindTransformFeedbackBuffersEXT(list->vk_command_buffer, first, count, buffers, offsets, sizes));
}

static void STDMETHODCALLTYPE d3d12_command_list_OMSetRenderTargets(ID3D12GraphicsCommandList6 *iface,
        UINT render_target_descriptor_count, const D3D12_CPU_DESCRIPTOR_HANDLE *render_target_descriptors,
        BOOL single_descriptor_handle, const D3D12_CPU_DESCRIPTOR_HANDLE *depth_stencil_descriptor)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct d3d12_rtv_desc *rtv_desc;
    const struct d3d12_dsv_desc *dsv_desc;
    VkFormat prev_dsv_format;
    struct vkd3d_view *view;
    unsigned int i;

    TRACE("iface %p, render_target_descriptor_count %u, render_target_descriptors %p, "
            "single_descriptor_handle %#x, depth_stencil_descriptor %p.\n",
            iface, render_target_descriptor_count, render_target_descriptors,
            single_descriptor_handle, depth_stencil_descriptor);

    if (render_target_descriptor_count > ARRAY_SIZE(list->rtvs))
    {
        WARN("Descriptor count %u > %zu, ignoring extra descriptors.\n",
                render_target_descriptor_count, ARRAY_SIZE(list->rtvs));
        render_target_descriptor_count = ARRAY_SIZE(list->rtvs);
    }

    list->fb_width = 0;
    list->fb_height = 0;
    list->fb_layer_count = 0;
    for (i = 0; i < render_target_descriptor_count; ++i)
    {
        if (single_descriptor_handle)
        {
            if ((rtv_desc = d3d12_rtv_desc_from_cpu_handle(*render_target_descriptors)))
                rtv_desc += i;
        }
        else
        {
            rtv_desc = d3d12_rtv_desc_from_cpu_handle(render_target_descriptors[i]);
        }

        if (!rtv_desc || !rtv_desc->resource)
        {
            WARN("RTV descriptor %u is not initialized.\n", i);
            list->rtvs[i] = VK_NULL_HANDLE;
            continue;
        }

        d3d12_command_list_track_resource_usage(list, rtv_desc->resource);

        /* In D3D12 CPU descriptors are consumed when a command is recorded. */
        view = rtv_desc->view;
        if (!d3d12_command_allocator_add_view(list->allocator, view))
        {
            WARN("Failed to add view.\n");
        }

        list->rtvs[i] = view->v.u.vk_image_view;
        list->fb_width = max(list->fb_width, rtv_desc->width);
        list->fb_height = max(list->fb_height, rtv_desc->height);
        list->fb_layer_count = max(list->fb_layer_count, rtv_desc->layer_count);
    }

    prev_dsv_format = list->dsv_format;
    list->dsv = VK_NULL_HANDLE;
    list->dsv_format = VK_FORMAT_UNDEFINED;
    if (depth_stencil_descriptor)
    {
        if ((dsv_desc = d3d12_dsv_desc_from_cpu_handle(*depth_stencil_descriptor))
                && dsv_desc->resource)
        {
            d3d12_command_list_track_resource_usage(list, dsv_desc->resource);

            /* In D3D12 CPU descriptors are consumed when a command is recorded. */
            view = dsv_desc->view;
            if (!d3d12_command_allocator_add_view(list->allocator, view))
            {
                WARN("Failed to add view.\n");
                list->dsv = VK_NULL_HANDLE;
            }

            list->dsv = view->v.u.vk_image_view;
            list->fb_width = max(list->fb_width, dsv_desc->width);
            list->fb_height = max(list->fb_height, dsv_desc->height);
            list->fb_layer_count = max(list->fb_layer_count, dsv_desc->layer_count);
            list->dsv_format = dsv_desc->format->vk_format;
        }
        else
        {
            WARN("DSV descriptor is not initialized.\n");
        }
    }

    if (prev_dsv_format != list->dsv_format && d3d12_pipeline_state_has_unknown_dsv_format(list->state))
        d3d12_command_list_invalidate_current_pipeline(list);

    d3d12_command_list_invalidate_current_framebuffer(list);
    d3d12_command_list_invalidate_current_render_pass(list);
}

static void d3d12_command_list_clear(struct d3d12_command_list *list,
        const struct VkAttachmentDescription *attachment_desc,
        const struct VkAttachmentReference *color_reference, const struct VkAttachmentReference *ds_reference,
        struct vkd3d_view *view, size_t width, size_t height, unsigned int layer_count,
        const union VkClearValue *clear_value, unsigned int rect_count, const D3D12_RECT *rects)
{
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    struct VkSubpassDescription sub_pass_desc;
    struct VkRenderPassCreateInfo pass_desc;
    struct VkRenderPassBeginInfo begin_desc;
    struct VkFramebufferCreateInfo fb_desc;
    VkFramebuffer vk_framebuffer;
    VkRenderPass vk_render_pass;
    D3D12_RECT full_rect;
    unsigned int i;
    VkResult vr;

    d3d12_command_list_end_current_render_pass(list);

    if (!rect_count)
    {
        full_rect.top = 0;
        full_rect.left = 0;
        full_rect.bottom = height;
        full_rect.right = width;

        rect_count = 1;
        rects = &full_rect;
    }

    sub_pass_desc.flags = 0;
    sub_pass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub_pass_desc.inputAttachmentCount = 0;
    sub_pass_desc.pInputAttachments = NULL;
    sub_pass_desc.colorAttachmentCount = !!color_reference;
    sub_pass_desc.pColorAttachments = color_reference;
    sub_pass_desc.pResolveAttachments = NULL;
    sub_pass_desc.pDepthStencilAttachment = ds_reference;
    sub_pass_desc.preserveAttachmentCount = 0;
    sub_pass_desc.pPreserveAttachments = NULL;

    pass_desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass_desc.pNext = NULL;
    pass_desc.flags = 0;
    pass_desc.attachmentCount = 1;
    pass_desc.pAttachments = attachment_desc;
    pass_desc.subpassCount = 1;
    pass_desc.pSubpasses = &sub_pass_desc;
    pass_desc.dependencyCount = 0;
    pass_desc.pDependencies = NULL;
    if ((vr = VK_CALL(vkCreateRenderPass(list->device->vk_device, &pass_desc, NULL, &vk_render_pass))) < 0)
    {
        WARN("Failed to create Vulkan render pass, vr %d.\n", vr);
        return;
    }

    if (!d3d12_command_allocator_add_render_pass(list->allocator, vk_render_pass))
    {
        WARN("Failed to add render pass.\n");
        VK_CALL(vkDestroyRenderPass(list->device->vk_device, vk_render_pass, NULL));
        return;
    }

    if (!d3d12_command_allocator_add_view(list->allocator, view))
    {
        WARN("Failed to add view.\n");
    }

    fb_desc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_desc.pNext = NULL;
    fb_desc.flags = 0;
    fb_desc.renderPass = vk_render_pass;
    fb_desc.attachmentCount = 1;
    fb_desc.pAttachments = &view->v.u.vk_image_view;
    fb_desc.width = width;
    fb_desc.height = height;
    fb_desc.layers = layer_count;
    if ((vr = VK_CALL(vkCreateFramebuffer(list->device->vk_device, &fb_desc, NULL, &vk_framebuffer))) < 0)
    {
        WARN("Failed to create Vulkan framebuffer, vr %d.\n", vr);
        return;
    }

    if (!d3d12_command_allocator_add_framebuffer(list->allocator, vk_framebuffer))
    {
        WARN("Failed to add framebuffer.\n");
        VK_CALL(vkDestroyFramebuffer(list->device->vk_device, vk_framebuffer, NULL));
        return;
    }

    begin_desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_desc.pNext = NULL;
    begin_desc.renderPass = vk_render_pass;
    begin_desc.framebuffer = vk_framebuffer;
    begin_desc.clearValueCount = 1;
    begin_desc.pClearValues = clear_value;

    for (i = 0; i < rect_count; ++i)
    {
        begin_desc.renderArea.offset.x = rects[i].left;
        begin_desc.renderArea.offset.y = rects[i].top;
        begin_desc.renderArea.extent.width = rects[i].right - rects[i].left;
        begin_desc.renderArea.extent.height = rects[i].bottom - rects[i].top;
        VK_CALL(vkCmdBeginRenderPass(list->vk_command_buffer, &begin_desc, VK_SUBPASS_CONTENTS_INLINE));
        VK_CALL(vkCmdEndRenderPass(list->vk_command_buffer));
    }
}

static void STDMETHODCALLTYPE d3d12_command_list_ClearDepthStencilView(ID3D12GraphicsCommandList6 *iface,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv, D3D12_CLEAR_FLAGS flags, float depth, UINT8 stencil,
        UINT rect_count, const D3D12_RECT *rects)
{
    const union VkClearValue clear_value = {.depthStencil = {depth, stencil}};
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct d3d12_dsv_desc *dsv_desc = d3d12_dsv_desc_from_cpu_handle(dsv);
    struct VkAttachmentDescription attachment_desc;
    struct VkAttachmentReference ds_reference;

    TRACE("iface %p, dsv %s, flags %#x, depth %.8e, stencil 0x%02x, rect_count %u, rects %p.\n",
            iface, debug_cpu_handle(dsv), flags, depth, stencil, rect_count, rects);

    d3d12_command_list_track_resource_usage(list, dsv_desc->resource);

    attachment_desc.flags = 0;
    attachment_desc.format = dsv_desc->format->vk_format;
    attachment_desc.samples = dsv_desc->sample_count;
    if (flags & D3D12_CLEAR_FLAG_DEPTH)
    {
        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
    else
    {
        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    if (flags & D3D12_CLEAR_FLAG_STENCIL)
    {
        attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
    else
    {
        attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    attachment_desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    ds_reference.attachment = 0;
    ds_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    d3d12_command_list_clear(list, &attachment_desc, NULL, &ds_reference,
            dsv_desc->view, dsv_desc->width, dsv_desc->height, dsv_desc->layer_count,
            &clear_value, rect_count, rects);
}

static void STDMETHODCALLTYPE d3d12_command_list_ClearRenderTargetView(ID3D12GraphicsCommandList6 *iface,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv, const FLOAT color[4], UINT rect_count, const D3D12_RECT *rects)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct d3d12_rtv_desc *rtv_desc = d3d12_rtv_desc_from_cpu_handle(rtv);
    struct VkAttachmentDescription attachment_desc;
    struct VkAttachmentReference color_reference;
    VkClearValue clear_value;

    TRACE("iface %p, rtv %s, color %p, rect_count %u, rects %p.\n",
            iface, debug_cpu_handle(rtv), color, rect_count, rects);

    d3d12_command_list_track_resource_usage(list, rtv_desc->resource);

    attachment_desc.flags = 0;
    attachment_desc.format = rtv_desc->format->vk_format;
    attachment_desc.samples = rtv_desc->sample_count;
    attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if (rtv_desc->format->type == VKD3D_FORMAT_TYPE_UINT)
    {
        clear_value.color.uint32[0] = max(0, color[0]);
        clear_value.color.uint32[1] = max(0, color[1]);
        clear_value.color.uint32[2] = max(0, color[2]);
        clear_value.color.uint32[3] = max(0, color[3]);
    }
    else if (rtv_desc->format->type == VKD3D_FORMAT_TYPE_SINT)
    {
        clear_value.color.int32[0] = color[0];
        clear_value.color.int32[1] = color[1];
        clear_value.color.int32[2] = color[2];
        clear_value.color.int32[3] = color[3];
    }
    else
    {
        clear_value.color.float32[0] = color[0];
        clear_value.color.float32[1] = color[1];
        clear_value.color.float32[2] = color[2];
        clear_value.color.float32[3] = color[3];
    }

    d3d12_command_list_clear(list, &attachment_desc, &color_reference, NULL,
            rtv_desc->view, rtv_desc->width, rtv_desc->height, rtv_desc->layer_count,
            &clear_value, rect_count, rects);
}

struct vkd3d_uav_clear_pipeline
{
    VkDescriptorSetLayout vk_set_layout;
    VkPipelineLayout vk_pipeline_layout;
    VkPipeline vk_pipeline;
    VkExtent3D group_size;
};

static void vkd3d_uav_clear_state_get_buffer_pipeline(const struct vkd3d_uav_clear_state *state,
        enum vkd3d_format_type format_type, struct vkd3d_uav_clear_pipeline *info)
{
    const struct vkd3d_uav_clear_pipelines *pipelines;

    pipelines = format_type == VKD3D_FORMAT_TYPE_UINT ? &state->pipelines_uint : &state->pipelines_float;
    info->vk_set_layout = state->vk_set_layout_buffer;
    info->vk_pipeline_layout = state->vk_pipeline_layout_buffer;
    info->vk_pipeline = pipelines->buffer;
    info->group_size = (VkExtent3D){128, 1, 1};
}

static void vkd3d_uav_clear_state_get_image_pipeline(const struct vkd3d_uav_clear_state *state,
        VkImageViewType image_view_type, enum vkd3d_format_type format_type, struct vkd3d_uav_clear_pipeline *info)
{
    const struct vkd3d_uav_clear_pipelines *pipelines;

    pipelines = format_type == VKD3D_FORMAT_TYPE_UINT ? &state->pipelines_uint : &state->pipelines_float;
    info->vk_set_layout = state->vk_set_layout_image;
    info->vk_pipeline_layout = state->vk_pipeline_layout_image;

    switch (image_view_type)
    {
        case VK_IMAGE_VIEW_TYPE_1D:
            info->vk_pipeline = pipelines->image_1d;
            info->group_size = (VkExtent3D){64, 1, 1};
            break;

        case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
            info->vk_pipeline = pipelines->image_1d_array;
            info->group_size = (VkExtent3D){64, 1, 1};
            break;

        case VK_IMAGE_VIEW_TYPE_2D:
            info->vk_pipeline = pipelines->image_2d;
            info->group_size = (VkExtent3D){8, 8, 1};
            break;

        case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
            info->vk_pipeline = pipelines->image_2d_array;
            info->group_size = (VkExtent3D){8, 8, 1};
            break;

        case VK_IMAGE_VIEW_TYPE_3D:
            info->vk_pipeline = pipelines->image_3d;
            info->group_size = (VkExtent3D){8, 8, 1};
            break;

        default:
            ERR("Unhandled view type %#x.\n", image_view_type);
            info->vk_pipeline = VK_NULL_HANDLE;
            info->group_size = (VkExtent3D){0, 0, 0};
            break;
    }
}

static void d3d12_command_list_clear_uav(struct d3d12_command_list *list,
        struct d3d12_resource *resource, struct vkd3d_view *descriptor, const VkClearColorValue *clear_colour,
        unsigned int rect_count, const D3D12_RECT *rects)
{
    const VkPhysicalDeviceLimits *device_limits = &list->device->vk_info.device_limits;
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;
    unsigned int i, miplevel_idx, layer_count;
    struct vkd3d_uav_clear_pipeline pipeline;
    struct vkd3d_uav_clear_args clear_args;
    const struct vkd3d_resource_view *view;
    uint32_t count_x, count_y, count_z;
    VkDescriptorImageInfo image_info;
    D3D12_RECT full_rect, curr_rect;
    VkWriteDescriptorSet write_set;

    d3d12_command_list_track_resource_usage(list, resource);
    d3d12_command_list_end_current_render_pass(list);

    d3d12_command_list_invalidate_current_pipeline(list);
    d3d12_command_list_invalidate_bindings(list, list->state);
    d3d12_command_list_invalidate_root_parameters(list, VKD3D_PIPELINE_BIND_POINT_COMPUTE);

    if (!d3d12_command_allocator_add_view(list->allocator, descriptor))
        WARN("Failed to add view.\n");
    view = &descriptor->v;

    clear_args.colour = *clear_colour;

    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.pNext = NULL;
    write_set.dstBinding = 0;
    write_set.dstArrayElement = 0;
    write_set.descriptorCount = 1;

    if (d3d12_resource_is_buffer(resource))
    {
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        write_set.pImageInfo = NULL;
        write_set.pBufferInfo = NULL;
        write_set.pTexelBufferView = &view->u.vk_buffer_view;

        miplevel_idx = 0;
        layer_count = 1;
        vkd3d_uav_clear_state_get_buffer_pipeline(&list->device->uav_clear_state,
                view->format->type, &pipeline);
    }
    else
    {
        image_info.sampler = VK_NULL_HANDLE;
        image_info.imageView = view->u.vk_image_view;
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        write_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write_set.pImageInfo = &image_info;
        write_set.pBufferInfo = NULL;
        write_set.pTexelBufferView = NULL;

        miplevel_idx = view->info.texture.miplevel_idx;
        layer_count = view->info.texture.vk_view_type == VK_IMAGE_VIEW_TYPE_3D
                ? d3d12_resource_desc_get_depth(&resource->desc, miplevel_idx)
                : view->info.texture.layer_count;
        vkd3d_uav_clear_state_get_image_pipeline(&list->device->uav_clear_state,
                view->info.texture.vk_view_type, view->format->type, &pipeline);
    }

    if (!(write_set.dstSet = d3d12_command_allocator_allocate_descriptor_set(
            list->allocator, pipeline.vk_set_layout, 0, false)))
    {
        ERR("Failed to allocate descriptor set.\n");
        return;
    }

    VK_CALL(vkUpdateDescriptorSets(list->device->vk_device, 1, &write_set, 0, NULL));

    full_rect.left = 0;
    full_rect.right = d3d12_resource_desc_get_width(&resource->desc, miplevel_idx);
    full_rect.top = 0;
    full_rect.bottom = d3d12_resource_desc_get_height(&resource->desc, miplevel_idx);

    if (!rect_count)
    {
        rects = &full_rect;
        rect_count = 1;
    }

    VK_CALL(vkCmdBindPipeline(list->vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.vk_pipeline));

    VK_CALL(vkCmdBindDescriptorSets(list->vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            pipeline.vk_pipeline_layout, 0, 1, &write_set.dstSet, 0, NULL));

    for (i = 0; i < rect_count; ++i)
    {
        /* Clamp to the actual resource region and skip empty rectangles. */
        curr_rect.left = max(rects[i].left, full_rect.left);
        curr_rect.top = max(rects[i].top, full_rect.top);
        curr_rect.right = min(rects[i].right, full_rect.right);
        curr_rect.bottom = min(rects[i].bottom, full_rect.bottom);

        if (curr_rect.left >= curr_rect.right || curr_rect.top >= curr_rect.bottom)
            continue;

        clear_args.offset.y = curr_rect.top;
        clear_args.extent.height = curr_rect.bottom - curr_rect.top;

        count_y = vkd3d_compute_workgroup_count(clear_args.extent.height, pipeline.group_size.height);
        count_z = vkd3d_compute_workgroup_count(layer_count, pipeline.group_size.depth);
        if (count_y > device_limits->maxComputeWorkGroupCount[1])
            FIXME("Group Y count %u exceeds max %u.\n", count_y, device_limits->maxComputeWorkGroupCount[1]);
        if (count_z > device_limits->maxComputeWorkGroupCount[2])
            FIXME("Group Z count %u exceeds max %u.\n", count_z, device_limits->maxComputeWorkGroupCount[2]);

        do
        {
            clear_args.offset.x = curr_rect.left;
            clear_args.extent.width = curr_rect.right - curr_rect.left;

            count_x = vkd3d_compute_workgroup_count(clear_args.extent.width, pipeline.group_size.width);
            count_x = min(count_x, device_limits->maxComputeWorkGroupCount[0]);

            VK_CALL(vkCmdPushConstants(list->vk_command_buffer, pipeline.vk_pipeline_layout,
                    VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(clear_args), &clear_args));

            VK_CALL(vkCmdDispatch(list->vk_command_buffer, count_x, count_y, count_z));

            curr_rect.left += count_x * pipeline.group_size.width;
        }
        while (curr_rect.right > curr_rect.left);
    }
}

static const struct vkd3d_format *vkd3d_fixup_clear_uav_uint_colour(struct d3d12_device *device,
        DXGI_FORMAT dxgi_format, VkClearColorValue *colour)
{
    switch (dxgi_format)
    {
        case DXGI_FORMAT_R11G11B10_FLOAT:
            colour->uint32[0] = (colour->uint32[0] & 0x7ff)
                    | ((colour->uint32[1] & 0x7ff) << 11)
                    | ((colour->uint32[2] & 0x3ff) << 22);
            return vkd3d_get_format(device, DXGI_FORMAT_R32_UINT, false);

        case DXGI_FORMAT_B5G6R5_UNORM:
            colour->uint32[0] = (colour->uint32[2] & 0x1f)
                    | ((colour->uint32[1] & 0x3f) << 5)
                    | ((colour->uint32[0] & 0x1f) << 11);
            return vkd3d_get_format(device, DXGI_FORMAT_R16_UINT, false);

        case DXGI_FORMAT_B5G5R5A1_UNORM:
            colour->uint32[0] = (colour->uint32[2] & 0x1f)
                    | ((colour->uint32[1] & 0x1f) << 5)
                    | ((colour->uint32[0] & 0x1f) << 10)
                    | ((colour->uint32[3] & 0x1) << 15);
            return vkd3d_get_format(device, DXGI_FORMAT_R16_UINT, false);

        case DXGI_FORMAT_B4G4R4A4_UNORM:
            colour->uint32[0] = (colour->uint32[2] & 0xf)
                    | ((colour->uint32[1] & 0xf) << 4)
                    | ((colour->uint32[0] & 0xf) << 8)
                    | ((colour->uint32[3] & 0xf) << 12);
            return vkd3d_get_format(device, DXGI_FORMAT_R16_UINT, false);

        default:
            return NULL;
    }
}

static struct vkd3d_view *create_uint_view(struct d3d12_device *device, const struct vkd3d_resource_view *view,
        struct d3d12_resource *resource, VkClearColorValue *colour)
{
    struct vkd3d_texture_view_desc view_desc;
    const struct vkd3d_format *uint_format;
    struct vkd3d_view *uint_view;

    if (!(uint_format = vkd3d_find_uint_format(device, view->format->dxgi_format))
            && !(uint_format = vkd3d_fixup_clear_uav_uint_colour(device, view->format->dxgi_format, colour)))
    {
        ERR("Unhandled format %#x.\n", view->format->dxgi_format);
        return NULL;
    }

    if (d3d12_resource_is_buffer(resource))
    {
        if (!vkd3d_create_buffer_view(device, VKD3D_DESCRIPTOR_MAGIC_UAV, resource->u.vk_buffer,
                    uint_format, view->info.buffer.offset, view->info.buffer.size, &uint_view))
        {
            ERR("Failed to create buffer view.\n");
            return NULL;
        }

        return uint_view;
    }

    memset(&view_desc, 0, sizeof(view_desc));
    view_desc.view_type = view->info.texture.vk_view_type;
    view_desc.format = uint_format;
    view_desc.miplevel_idx = view->info.texture.miplevel_idx;
    view_desc.miplevel_count = 1;
    view_desc.layer_idx = view->info.texture.layer_idx;
    view_desc.layer_count = view->info.texture.layer_count;
    view_desc.vk_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    view_desc.usage = VK_IMAGE_USAGE_STORAGE_BIT;

    if (!vkd3d_create_texture_view(device, VKD3D_DESCRIPTOR_MAGIC_UAV,
            resource->u.vk_image, &view_desc, &uint_view))
    {
        ERR("Failed to create image view.\n");
        return NULL;
    }

    return uint_view;
}

static void STDMETHODCALLTYPE d3d12_command_list_ClearUnorderedAccessViewUint(ID3D12GraphicsCommandList6 *iface,
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, ID3D12Resource *resource,
        const UINT values[4], UINT rect_count, const D3D12_RECT *rects)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct vkd3d_view *descriptor, *uint_view = NULL;
    struct d3d12_device *device = list->device;
    const struct vkd3d_resource_view *view;
    struct d3d12_resource *resource_impl;
    VkClearColorValue colour;

    TRACE("iface %p, gpu_handle %s, cpu_handle %s, resource %p, values %p, rect_count %u, rects %p.\n",
            iface, debug_gpu_handle(gpu_handle), debug_cpu_handle(cpu_handle), resource, values, rect_count, rects);

    resource_impl = unsafe_impl_from_ID3D12Resource(resource);
    if (!(descriptor = d3d12_desc_from_cpu_handle(cpu_handle)->s.u.view))
        return;
    view = &descriptor->v;
    memcpy(colour.uint32, values, sizeof(colour.uint32));

    if (view->format->type != VKD3D_FORMAT_TYPE_UINT
            && !(descriptor = uint_view = create_uint_view(device, view, resource_impl, &colour)))
    {
        ERR("Failed to create UINT view.\n");
        return;
    }

    d3d12_command_list_clear_uav(list, resource_impl, descriptor, &colour, rect_count, rects);

    if (uint_view)
        vkd3d_view_decref(uint_view, device);
}

static void STDMETHODCALLTYPE d3d12_command_list_ClearUnorderedAccessViewFloat(ID3D12GraphicsCommandList6 *iface,
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, ID3D12Resource *resource,
        const float values[4], UINT rect_count, const D3D12_RECT *rects)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct vkd3d_view *descriptor, *uint_view = NULL;
    struct d3d12_device *device = list->device;
    const struct vkd3d_resource_view *view;
    struct d3d12_resource *resource_impl;
    VkClearColorValue colour;

    TRACE("iface %p, gpu_handle %s, cpu_handle %s, resource %p, values %p, rect_count %u, rects %p.\n",
            iface, debug_gpu_handle(gpu_handle), debug_cpu_handle(cpu_handle), resource, values, rect_count, rects);

    resource_impl = unsafe_impl_from_ID3D12Resource(resource);
    if (!(descriptor = d3d12_desc_from_cpu_handle(cpu_handle)->s.u.view))
        return;
    view = &descriptor->v;
    memcpy(colour.float32, values, sizeof(colour.float32));

    if (view->format->type == VKD3D_FORMAT_TYPE_SINT
            && !(descriptor = uint_view = create_uint_view(device, view, resource_impl, &colour)))
    {
        ERR("Failed to create UINT view.\n");
        return;
    }

    d3d12_command_list_clear_uav(list, resource_impl, descriptor, &colour, rect_count, rects);

    if (uint_view)
        vkd3d_view_decref(uint_view, device);
}

static void STDMETHODCALLTYPE d3d12_command_list_DiscardResource(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *resource, const D3D12_DISCARD_REGION *region)
{
    FIXME_ONCE("iface %p, resource %p, region %p stub!\n", iface, resource, region);
}

static void STDMETHODCALLTYPE d3d12_command_list_BeginQuery(ID3D12GraphicsCommandList6 *iface,
        ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, UINT index)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct d3d12_query_heap *query_heap = unsafe_impl_from_ID3D12QueryHeap(heap);
    const struct vkd3d_vk_device_procs *vk_procs;
    VkQueryControlFlags flags = 0;

    TRACE("iface %p, heap %p, type %#x, index %u.\n", iface, heap, type, index);

    vk_procs = &list->device->vk_procs;

    d3d12_command_list_end_current_render_pass(list);

    VK_CALL(vkCmdResetQueryPool(list->vk_command_buffer, query_heap->vk_query_pool, index, 1));

    if (type == D3D12_QUERY_TYPE_OCCLUSION)
        flags = VK_QUERY_CONTROL_PRECISE_BIT;

    if (D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 <= type && type <= D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3)
    {
        unsigned int stream_index = type - D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0;
        VK_CALL(vkCmdBeginQueryIndexedEXT(list->vk_command_buffer,
                query_heap->vk_query_pool, index, flags, stream_index));
        return;
    }

    VK_CALL(vkCmdBeginQuery(list->vk_command_buffer, query_heap->vk_query_pool, index, flags));
}

static void STDMETHODCALLTYPE d3d12_command_list_EndQuery(ID3D12GraphicsCommandList6 *iface,
        ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, UINT index)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct d3d12_query_heap *query_heap = unsafe_impl_from_ID3D12QueryHeap(heap);
    const struct vkd3d_vk_device_procs *vk_procs;

    TRACE("iface %p, heap %p, type %#x, index %u.\n", iface, heap, type, index);

    vk_procs = &list->device->vk_procs;

    d3d12_command_list_end_current_render_pass(list);

    d3d12_query_heap_mark_result_as_available(query_heap, index);

    if (type == D3D12_QUERY_TYPE_TIMESTAMP)
    {
        VK_CALL(vkCmdResetQueryPool(list->vk_command_buffer, query_heap->vk_query_pool, index, 1));
        VK_CALL(vkCmdWriteTimestamp(list->vk_command_buffer,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_heap->vk_query_pool, index));
        return;
    }

    if (D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 <= type && type <= D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3)
    {
        unsigned int stream_index = type - D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0;
        VK_CALL(vkCmdEndQueryIndexedEXT(list->vk_command_buffer,
                query_heap->vk_query_pool, index, stream_index));
        return;
    }

    VK_CALL(vkCmdEndQuery(list->vk_command_buffer, query_heap->vk_query_pool, index));
}

static size_t get_query_stride(D3D12_QUERY_TYPE type)
{
    if (type == D3D12_QUERY_TYPE_PIPELINE_STATISTICS)
        return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);

    if (D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 <= type && type <= D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3)
        return sizeof(D3D12_QUERY_DATA_SO_STATISTICS);

    return sizeof(uint64_t);
}

static void STDMETHODCALLTYPE d3d12_command_list_ResolveQueryData(ID3D12GraphicsCommandList6 *iface,
        ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, UINT start_index, UINT query_count,
        ID3D12Resource *dst_buffer, UINT64 aligned_dst_buffer_offset)
{
    const struct d3d12_query_heap *query_heap = unsafe_impl_from_ID3D12QueryHeap(heap);
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct d3d12_resource *buffer = unsafe_impl_from_ID3D12Resource(dst_buffer);
    const struct vkd3d_vk_device_procs *vk_procs;
    unsigned int i, first, count;
    VkDeviceSize offset, stride;

    TRACE("iface %p, heap %p, type %#x, start_index %u, query_count %u, "
            "dst_buffer %p, aligned_dst_buffer_offset %#"PRIx64".\n",
            iface, heap, type, start_index, query_count,
            dst_buffer, aligned_dst_buffer_offset);

    vk_procs = &list->device->vk_procs;

    /* Vulkan is less strict than D3D12 here. Vulkan implementations are free
     * to return any non-zero result for binary occlusion with at least one
     * sample passing, while D3D12 guarantees that the result is 1 then.
     *
     * For example, the Nvidia binary blob drivers on Linux seem to always
     * count precisely, even when it was signalled that non-precise is enough.
     */
    if (type == D3D12_QUERY_TYPE_BINARY_OCCLUSION)
        FIXME_ONCE("D3D12 guarantees binary occlusion queries result in only 0 and 1.\n");

    if (!d3d12_resource_is_buffer(buffer))
    {
        WARN("Destination resource is not a buffer.\n");
        return;
    }

    d3d12_command_list_end_current_render_pass(list);

    stride = get_query_stride(type);

    count = 0;
    first = start_index;
    offset = aligned_dst_buffer_offset;
    for (i = 0; i < query_count; ++i)
    {
        if (d3d12_query_heap_is_result_available(query_heap, start_index + i))
        {
            ++count;
        }
        else
        {
            if (count)
            {
                VK_CALL(vkCmdCopyQueryPoolResults(list->vk_command_buffer,
                        query_heap->vk_query_pool, first, count, buffer->u.vk_buffer,
                        offset, stride, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
            }
            count = 0;
            first = start_index + i;
            offset = aligned_dst_buffer_offset + i * stride;

            /* We cannot copy query results if a query was not issued:
             *
             *   "If the query does not become available in a finite amount of
             *   time (e.g. due to not issuing a query since the last reset),
             *   a VK_ERROR_DEVICE_LOST error may occur."
             */
            VK_CALL(vkCmdFillBuffer(list->vk_command_buffer,
                    buffer->u.vk_buffer, offset, stride, 0x00000000));

            ++first;
            offset += stride;
        }
    }

    if (count)
    {
        VK_CALL(vkCmdCopyQueryPoolResults(list->vk_command_buffer,
                query_heap->vk_query_pool, first, count, buffer->u.vk_buffer,
                offset, stride, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
    }
}

static void STDMETHODCALLTYPE d3d12_command_list_SetPredication(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *buffer, UINT64 aligned_buffer_offset, D3D12_PREDICATION_OP operation)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct d3d12_resource *resource = unsafe_impl_from_ID3D12Resource(buffer);
    const struct vkd3d_vulkan_info *vk_info = &list->device->vk_info;
    const struct vkd3d_vk_device_procs *vk_procs;

    TRACE("iface %p, buffer %p, aligned_buffer_offset %#"PRIx64", operation %#x.\n",
            iface, buffer, aligned_buffer_offset, operation);

    if (!vk_info->EXT_conditional_rendering)
    {
        FIXME("Vulkan conditional rendering extension not present. Conditional rendering not supported.\n");
        return;
    }

    vk_procs = &list->device->vk_procs;

    /* FIXME: Add support for conditional rendering in render passes. */
    d3d12_command_list_end_current_render_pass(list);

    if (resource)
    {
        VkConditionalRenderingBeginInfoEXT cond_info;

        if (aligned_buffer_offset & (sizeof(uint64_t) - 1))
        {
            WARN("Unaligned predicate argument buffer offset %#"PRIx64".\n", aligned_buffer_offset);
            return;
        }

        if (!d3d12_resource_is_buffer(resource))
        {
            WARN("Predicate arguments must be stored in a buffer resource.\n");
            return;
        }

        FIXME_ONCE("Predication doesn't support clear and copy commands, "
                "and predication values are treated as 32-bit values.\n");

        cond_info.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
        cond_info.pNext = NULL;
        cond_info.buffer = resource->u.vk_buffer;
        cond_info.offset = aligned_buffer_offset;
        switch (operation)
        {
            case D3D12_PREDICATION_OP_EQUAL_ZERO:
                cond_info.flags = 0;
                break;

            case D3D12_PREDICATION_OP_NOT_EQUAL_ZERO:
                cond_info.flags = VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT;
                break;

            default:
                FIXME("Unhandled predication operation %#x.\n", operation);
                return;
        }

        if (list->is_predicated)
            VK_CALL(vkCmdEndConditionalRenderingEXT(list->vk_command_buffer));
        VK_CALL(vkCmdBeginConditionalRenderingEXT(list->vk_command_buffer, &cond_info));
        list->is_predicated = true;
    }
    else if (list->is_predicated)
    {
        VK_CALL(vkCmdEndConditionalRenderingEXT(list->vk_command_buffer));
        list->is_predicated = false;
    }
}

static void STDMETHODCALLTYPE d3d12_command_list_SetMarker(ID3D12GraphicsCommandList6 *iface,
        UINT metadata, const void *data, UINT size)
{
    FIXME("iface %p, metadata %#x, data %p, size %u stub!\n", iface, metadata, data, size);
}

static void STDMETHODCALLTYPE d3d12_command_list_BeginEvent(ID3D12GraphicsCommandList6 *iface,
        UINT metadata, const void *data, UINT size)
{
    FIXME("iface %p, metadata %#x, data %p, size %u stub!\n", iface, metadata, data, size);
}

static void STDMETHODCALLTYPE d3d12_command_list_EndEvent(ID3D12GraphicsCommandList6 *iface)
{
    FIXME("iface %p stub!\n", iface);
}

STATIC_ASSERT(sizeof(VkDispatchIndirectCommand) == sizeof(D3D12_DISPATCH_ARGUMENTS));
STATIC_ASSERT(sizeof(VkDrawIndexedIndirectCommand) == sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
STATIC_ASSERT(sizeof(VkDrawIndirectCommand) == sizeof(D3D12_DRAW_ARGUMENTS));

static void STDMETHODCALLTYPE d3d12_command_list_ExecuteIndirect(ID3D12GraphicsCommandList6 *iface,
        ID3D12CommandSignature *command_signature, UINT max_command_count, ID3D12Resource *arg_buffer,
        UINT64 arg_buffer_offset, ID3D12Resource *count_buffer, UINT64 count_buffer_offset)
{
    struct d3d12_command_signature *sig_impl = unsafe_impl_from_ID3D12CommandSignature(command_signature);
    struct d3d12_resource *count_impl = unsafe_impl_from_ID3D12Resource(count_buffer);
    struct d3d12_resource *arg_impl = unsafe_impl_from_ID3D12Resource(arg_buffer);
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const D3D12_COMMAND_SIGNATURE_DESC *signature_desc;
    const struct vkd3d_vk_device_procs *vk_procs;
    unsigned int i;

    TRACE("iface %p, command_signature %p, max_command_count %u, arg_buffer %p, "
            "arg_buffer_offset %#"PRIx64", count_buffer %p, count_buffer_offset %#"PRIx64".\n",
            iface, command_signature, max_command_count, arg_buffer, arg_buffer_offset,
            count_buffer, count_buffer_offset);

    vk_procs = &list->device->vk_procs;

    if (count_buffer && !list->device->vk_info.KHR_draw_indirect_count)
    {
        FIXME("Count buffers not supported by Vulkan implementation.\n");
        return;
    }

    d3d12_command_signature_incref(sig_impl);

    signature_desc = &sig_impl->desc;
    for (i = 0; i < signature_desc->NumArgumentDescs; ++i)
    {
        const D3D12_INDIRECT_ARGUMENT_DESC *arg_desc = &signature_desc->pArgumentDescs[i];

        switch (arg_desc->Type)
        {
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
                if (!d3d12_command_list_begin_render_pass(list))
                {
                    WARN("Failed to begin render pass, ignoring draw.\n");
                    break;
                }

                if (count_buffer)
                {
                    VK_CALL(vkCmdDrawIndirectCountKHR(list->vk_command_buffer, arg_impl->u.vk_buffer,
                            arg_buffer_offset, count_impl->u.vk_buffer, count_buffer_offset,
                            max_command_count, signature_desc->ByteStride));
                }
                else
                {
                    VK_CALL(vkCmdDrawIndirect(list->vk_command_buffer, arg_impl->u.vk_buffer,
                            arg_buffer_offset, max_command_count, signature_desc->ByteStride));
                }
                break;

            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
                if (!d3d12_command_list_begin_render_pass(list))
                {
                    WARN("Failed to begin render pass, ignoring draw.\n");
                    break;
                }

                d3d12_command_list_check_index_buffer_strip_cut_value(list);

                if (count_buffer)
                {
                    VK_CALL(vkCmdDrawIndexedIndirectCountKHR(list->vk_command_buffer, arg_impl->u.vk_buffer,
                            arg_buffer_offset, count_impl->u.vk_buffer, count_buffer_offset,
                            max_command_count, signature_desc->ByteStride));
                }
                else
                {
                    VK_CALL(vkCmdDrawIndexedIndirect(list->vk_command_buffer, arg_impl->u.vk_buffer,
                            arg_buffer_offset, max_command_count, signature_desc->ByteStride));
                }
                break;

            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
                if (max_command_count != 1)
                    FIXME("Ignoring command count %u.\n", max_command_count);

                if (count_buffer)
                {
                    FIXME("Count buffers not supported for indirect dispatch.\n");
                    break;
                }

                if (!d3d12_command_list_update_compute_state(list))
                {
                    WARN("Failed to update compute state, ignoring dispatch.\n");
                    d3d12_command_signature_decref(sig_impl);
                    return;
                }

                VK_CALL(vkCmdDispatchIndirect(list->vk_command_buffer,
                        arg_impl->u.vk_buffer, arg_buffer_offset));
                break;

            default:
                FIXME("Ignoring unhandled argument type %#x.\n", arg_desc->Type);
                break;
        }
    }

    d3d12_command_signature_decref(sig_impl);
}

static void STDMETHODCALLTYPE d3d12_command_list_AtomicCopyBufferUINT(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *dst_buffer, UINT64 dst_offset,
        ID3D12Resource *src_buffer, UINT64 src_offset,
        UINT dependent_resource_count, ID3D12Resource * const *dependent_resources,
        const D3D12_SUBRESOURCE_RANGE_UINT64 *dependent_sub_resource_ranges)
{
    FIXME("iface %p, dst_resource %p, dst_offset %#"PRIx64", src_resource %p, "
            "src_offset %#"PRIx64", dependent_resource_count %u, "
            "dependent_resources %p, dependent_sub_resource_ranges %p stub!\n",
            iface, dst_buffer, dst_offset, src_buffer, src_offset,
            dependent_resource_count, dependent_resources, dependent_sub_resource_ranges);
}

static void STDMETHODCALLTYPE d3d12_command_list_AtomicCopyBufferUINT64(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *dst_buffer, UINT64 dst_offset,
        ID3D12Resource *src_buffer, UINT64 src_offset,
        UINT dependent_resource_count, ID3D12Resource * const *dependent_resources,
        const D3D12_SUBRESOURCE_RANGE_UINT64 *dependent_sub_resource_ranges)
{
    FIXME("iface %p, dst_resource %p, dst_offset %#"PRIx64", src_resource %p, "
            "src_offset %#"PRIx64", dependent_resource_count %u, "
            "dependent_resources %p, dependent_sub_resource_ranges %p stub!\n",
            iface, dst_buffer, dst_offset, src_buffer, src_offset,
            dependent_resource_count, dependent_resources, dependent_sub_resource_ranges);
}

static void STDMETHODCALLTYPE d3d12_command_list_OMSetDepthBounds(ID3D12GraphicsCommandList6 *iface,
        FLOAT min, FLOAT max)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    const struct vkd3d_vk_device_procs *vk_procs = &list->device->vk_procs;

    TRACE("iface %p, min %.8e, max %.8e.\n", iface, min, max);

    if (isnan(max))
        max = 0.0f;
    if (isnan(min))
        min = 0.0f;

    if (!list->device->vk_info.EXT_depth_range_unrestricted && (min < 0.0f || min > 1.0f || max < 0.0f || max > 1.0f))
    {
        WARN("VK_EXT_depth_range_unrestricted was not found, clamping depth bounds to 0.0 and 1.0.\n");
        max = vkd3d_clamp(max, 0.0f, 1.0f);
        min = vkd3d_clamp(min, 0.0f, 1.0f);
    }

    list->has_depth_bounds = true;
    VK_CALL(vkCmdSetDepthBounds(list->vk_command_buffer, min, max));
}

static void STDMETHODCALLTYPE d3d12_command_list_SetSamplePositions(ID3D12GraphicsCommandList6 *iface,
        UINT sample_count, UINT pixel_count, D3D12_SAMPLE_POSITION *sample_positions)
{
    FIXME("iface %p, sample_count %u, pixel_count %u, sample_positions %p stub!\n",
            iface, sample_count, pixel_count, sample_positions);
}

static void STDMETHODCALLTYPE d3d12_command_list_ResolveSubresourceRegion(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *dst_resource, UINT dst_sub_resource_idx, UINT dst_x, UINT dst_y,
        ID3D12Resource *src_resource, UINT src_sub_resource_idx,
        D3D12_RECT *src_rect, DXGI_FORMAT format, D3D12_RESOLVE_MODE mode)
{
    FIXME("iface %p, dst_resource %p, dst_sub_resource_idx %u, "
            "dst_x %u, dst_y %u, src_resource %p, src_sub_resource_idx %u, "
            "src_rect %p, format %#x, mode %#x stub!\n",
            iface, dst_resource, dst_sub_resource_idx, dst_x, dst_y,
            src_resource, src_sub_resource_idx, src_rect, format, mode);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetViewInstanceMask(ID3D12GraphicsCommandList6 *iface, UINT mask)
{
    FIXME("iface %p, mask %#x stub!\n", iface, mask);
}

static void STDMETHODCALLTYPE d3d12_command_list_WriteBufferImmediate(ID3D12GraphicsCommandList6 *iface,
        UINT count, const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER *parameters,
        const D3D12_WRITEBUFFERIMMEDIATE_MODE *modes)
{
    struct d3d12_command_list *list = impl_from_ID3D12GraphicsCommandList6(iface);
    struct d3d12_resource *resource;
    unsigned int i;

    FIXME("iface %p, count %u, parameters %p, modes %p stub!\n", iface, count, parameters, modes);

    for (i = 0; i < count; ++i)
    {
        resource = vkd3d_gpu_va_allocator_dereference(&list->device->gpu_va_allocator, parameters[i].Dest);
        d3d12_command_list_track_resource_usage(list, resource);
    }
}

static void STDMETHODCALLTYPE d3d12_command_list_SetProtectedResourceSession(ID3D12GraphicsCommandList6 *iface,
        ID3D12ProtectedResourceSession *protected_session)
{
    FIXME("iface %p, protected_session %p stub!\n", iface, protected_session);
}

static void STDMETHODCALLTYPE d3d12_command_list_BeginRenderPass(ID3D12GraphicsCommandList6 *iface,
        UINT count, const D3D12_RENDER_PASS_RENDER_TARGET_DESC *render_targets,
        const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC *depth_stencil, D3D12_RENDER_PASS_FLAGS flags)
{
    FIXME("iface %p, count %u, render_targets %p, depth_stencil %p, flags %#x stub!\n", iface,
            count, render_targets, depth_stencil, flags);
}

static void STDMETHODCALLTYPE d3d12_command_list_EndRenderPass(ID3D12GraphicsCommandList6 *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static void STDMETHODCALLTYPE d3d12_command_list_InitializeMetaCommand(ID3D12GraphicsCommandList6 *iface,
        ID3D12MetaCommand *meta_command, const void *parameters_data, SIZE_T data_size_in_bytes)
{
    FIXME("iface %p, meta_command %p, parameters_data %p, data_size_in_bytes %"PRIuPTR" stub!\n", iface,
            meta_command, parameters_data, (uintptr_t)data_size_in_bytes);
}

static void STDMETHODCALLTYPE d3d12_command_list_ExecuteMetaCommand(ID3D12GraphicsCommandList6 *iface,
        ID3D12MetaCommand *meta_command, const void *parameters_data, SIZE_T data_size_in_bytes)
{
    FIXME("iface %p, meta_command %p, parameters_data %p, data_size_in_bytes %"PRIuPTR" stub!\n", iface,
            meta_command, parameters_data, (uintptr_t)data_size_in_bytes);
}

static void STDMETHODCALLTYPE d3d12_command_list_BuildRaytracingAccelerationStructure(ID3D12GraphicsCommandList6 *iface,
        const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC *desc, UINT count,
        const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC *postbuild_info_descs)
{
    FIXME("iface %p, desc %p, count %u, postbuild_info_descs %p stub!\n", iface, desc, count, postbuild_info_descs);
}

static void STDMETHODCALLTYPE d3d12_command_list_EmitRaytracingAccelerationStructurePostbuildInfo(
        ID3D12GraphicsCommandList6 *iface, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC *desc,
        UINT structures_count, const D3D12_GPU_VIRTUAL_ADDRESS *src_structure_data)
{
    FIXME("iface %p, desc %p, structures_count %u, src_structure_data %p stub!\n",
            iface, desc, structures_count, src_structure_data);
}

static void STDMETHODCALLTYPE d3d12_command_list_CopyRaytracingAccelerationStructure(ID3D12GraphicsCommandList6 *iface,
        D3D12_GPU_VIRTUAL_ADDRESS dst_structure_data, D3D12_GPU_VIRTUAL_ADDRESS src_structure_data,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode)
{
    FIXME("iface %p, dst_structure_data %#"PRIx64", src_structure_data %#"PRIx64", mode %u stub!\n",
            iface, dst_structure_data, src_structure_data, mode);
}

static void STDMETHODCALLTYPE d3d12_command_list_SetPipelineState1(ID3D12GraphicsCommandList6 *iface,
        ID3D12StateObject *state_object)
{
    FIXME("iface %p, state_object %p stub!\n", iface, state_object);
}

static void STDMETHODCALLTYPE d3d12_command_list_DispatchRays(ID3D12GraphicsCommandList6 *iface,
        const D3D12_DISPATCH_RAYS_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);
}

static void STDMETHODCALLTYPE d3d12_command_list_RSSetShadingRate(ID3D12GraphicsCommandList6 *iface,
        D3D12_SHADING_RATE rate, const D3D12_SHADING_RATE_COMBINER *combiners)
{
    FIXME("iface %p, rate %#x, combiners %p stub!\n", iface, rate, combiners);
}

static void STDMETHODCALLTYPE d3d12_command_list_RSSetShadingRateImage(ID3D12GraphicsCommandList6 *iface,
        ID3D12Resource *rate_image)
{
    FIXME("iface %p, rate_image %p stub!\n", iface, rate_image);
}

static void STDMETHODCALLTYPE d3d12_command_list_DispatchMesh(ID3D12GraphicsCommandList6 *iface, UINT x, UINT y, UINT z)
{
    FIXME("iface %p, x %u, y %u, z %u stub!\n", iface, x, y, z);
}

static const struct ID3D12GraphicsCommandList6Vtbl d3d12_command_list_vtbl =
{
    /* IUnknown methods */
    d3d12_command_list_QueryInterface,
    d3d12_command_list_AddRef,
    d3d12_command_list_Release,
    /* ID3D12Object methods */
    d3d12_command_list_GetPrivateData,
    d3d12_command_list_SetPrivateData,
    d3d12_command_list_SetPrivateDataInterface,
    d3d12_command_list_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_command_list_GetDevice,
    /* ID3D12CommandList methods */
    d3d12_command_list_GetType,
    /* ID3D12GraphicsCommandList methods */
    d3d12_command_list_Close,
    d3d12_command_list_Reset,
    d3d12_command_list_ClearState,
    d3d12_command_list_DrawInstanced,
    d3d12_command_list_DrawIndexedInstanced,
    d3d12_command_list_Dispatch,
    d3d12_command_list_CopyBufferRegion,
    d3d12_command_list_CopyTextureRegion,
    d3d12_command_list_CopyResource,
    d3d12_command_list_CopyTiles,
    d3d12_command_list_ResolveSubresource,
    d3d12_command_list_IASetPrimitiveTopology,
    d3d12_command_list_RSSetViewports,
    d3d12_command_list_RSSetScissorRects,
    d3d12_command_list_OMSetBlendFactor,
    d3d12_command_list_OMSetStencilRef,
    d3d12_command_list_SetPipelineState,
    d3d12_command_list_ResourceBarrier,
    d3d12_command_list_ExecuteBundle,
    d3d12_command_list_SetDescriptorHeaps,
    d3d12_command_list_SetComputeRootSignature,
    d3d12_command_list_SetGraphicsRootSignature,
    d3d12_command_list_SetComputeRootDescriptorTable,
    d3d12_command_list_SetGraphicsRootDescriptorTable,
    d3d12_command_list_SetComputeRoot32BitConstant,
    d3d12_command_list_SetGraphicsRoot32BitConstant,
    d3d12_command_list_SetComputeRoot32BitConstants,
    d3d12_command_list_SetGraphicsRoot32BitConstants,
    d3d12_command_list_SetComputeRootConstantBufferView,
    d3d12_command_list_SetGraphicsRootConstantBufferView,
    d3d12_command_list_SetComputeRootShaderResourceView,
    d3d12_command_list_SetGraphicsRootShaderResourceView,
    d3d12_command_list_SetComputeRootUnorderedAccessView,
    d3d12_command_list_SetGraphicsRootUnorderedAccessView,
    d3d12_command_list_IASetIndexBuffer,
    d3d12_command_list_IASetVertexBuffers,
    d3d12_command_list_SOSetTargets,
    d3d12_command_list_OMSetRenderTargets,
    d3d12_command_list_ClearDepthStencilView,
    d3d12_command_list_ClearRenderTargetView,
    d3d12_command_list_ClearUnorderedAccessViewUint,
    d3d12_command_list_ClearUnorderedAccessViewFloat,
    d3d12_command_list_DiscardResource,
    d3d12_command_list_BeginQuery,
    d3d12_command_list_EndQuery,
    d3d12_command_list_ResolveQueryData,
    d3d12_command_list_SetPredication,
    d3d12_command_list_SetMarker,
    d3d12_command_list_BeginEvent,
    d3d12_command_list_EndEvent,
    d3d12_command_list_ExecuteIndirect,
    /* ID3D12GraphicsCommandList1 methods */
    d3d12_command_list_AtomicCopyBufferUINT,
    d3d12_command_list_AtomicCopyBufferUINT64,
    d3d12_command_list_OMSetDepthBounds,
    d3d12_command_list_SetSamplePositions,
    d3d12_command_list_ResolveSubresourceRegion,
    d3d12_command_list_SetViewInstanceMask,
    /* ID3D12GraphicsCommandList2 methods */
    d3d12_command_list_WriteBufferImmediate,
    /* ID3D12GraphicsCommandList3 methods */
    d3d12_command_list_SetProtectedResourceSession,
    /* ID3D12GraphicsCommandList4 methods */
    d3d12_command_list_BeginRenderPass,
    d3d12_command_list_EndRenderPass,
    d3d12_command_list_InitializeMetaCommand,
    d3d12_command_list_ExecuteMetaCommand,
    d3d12_command_list_BuildRaytracingAccelerationStructure,
    d3d12_command_list_EmitRaytracingAccelerationStructurePostbuildInfo,
    d3d12_command_list_CopyRaytracingAccelerationStructure,
    d3d12_command_list_SetPipelineState1,
    d3d12_command_list_DispatchRays,
    /* ID3D12GraphicsCommandList5 methods */
    d3d12_command_list_RSSetShadingRate,
    d3d12_command_list_RSSetShadingRateImage,
    /* ID3D12GraphicsCommandList6 methods */
    d3d12_command_list_DispatchMesh,
};

static struct d3d12_command_list *unsafe_impl_from_ID3D12CommandList(ID3D12CommandList *iface)
{
    if (!iface)
        return NULL;
    VKD3D_ASSERT(iface->lpVtbl == (struct ID3D12CommandListVtbl *)&d3d12_command_list_vtbl);
    return CONTAINING_RECORD(iface, struct d3d12_command_list, ID3D12GraphicsCommandList6_iface);
}

static HRESULT d3d12_command_list_init(struct d3d12_command_list *list, struct d3d12_device *device,
        D3D12_COMMAND_LIST_TYPE type, struct d3d12_command_allocator *allocator,
        ID3D12PipelineState *initial_pipeline_state)
{
    HRESULT hr;

    list->ID3D12GraphicsCommandList6_iface.lpVtbl = &d3d12_command_list_vtbl;
    list->refcount = 1;

    list->type = type;

    if (FAILED(hr = vkd3d_private_store_init(&list->private_store)))
        return hr;

    d3d12_device_add_ref(list->device = device);

    list->allocator = allocator;

    list->descriptor_heap_count = 0;

    if (SUCCEEDED(hr = d3d12_command_allocator_allocate_command_buffer(allocator, list)))
    {
        list->pipeline_bindings[VKD3D_PIPELINE_BIND_POINT_GRAPHICS].vk_uav_counter_views = NULL;
        list->pipeline_bindings[VKD3D_PIPELINE_BIND_POINT_COMPUTE].vk_uav_counter_views = NULL;
        d3d12_command_list_reset_state(list, initial_pipeline_state);
    }
    else
    {
        vkd3d_private_store_destroy(&list->private_store);
        d3d12_device_release(device);
    }

    return hr;
}

HRESULT d3d12_command_list_create(struct d3d12_device *device,
        UINT node_mask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *allocator_iface,
        ID3D12PipelineState *initial_pipeline_state, struct d3d12_command_list **list)
{
    struct d3d12_command_allocator *allocator;
    struct d3d12_command_list *object;
    HRESULT hr;

    if (!(allocator = unsafe_impl_from_ID3D12CommandAllocator(allocator_iface)))
    {
        WARN("Command allocator is NULL.\n");
        return E_INVALIDARG;
    }

    if (allocator->type != type)
    {
        WARN("Command list types do not match (allocator %#x, list %#x).\n",
                allocator->type, type);
        return E_INVALIDARG;
    }

    debug_ignored_node_mask(node_mask);

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_command_list_init(object, device, type, allocator, initial_pipeline_state)))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created command list %p.\n", object);

    *list = object;

    return S_OK;
}

/* ID3D12CommandQueue */
static inline struct d3d12_command_queue *impl_from_ID3D12CommandQueue(ID3D12CommandQueue *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_command_queue, ID3D12CommandQueue_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_QueryInterface(ID3D12CommandQueue *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D12CommandQueue)
            || IsEqualGUID(riid, &IID_ID3D12Pageable)
            || IsEqualGUID(riid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(riid, &IID_ID3D12Object)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D12CommandQueue_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_command_queue_AddRef(ID3D12CommandQueue *iface)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&command_queue->refcount);

    TRACE("%p increasing refcount to %u.\n", command_queue, refcount);

    return refcount;
}

static void d3d12_command_queue_destroy_op(struct vkd3d_cs_op_data *op)
{
    switch (op->opcode)
    {
        case VKD3D_CS_OP_WAIT:
            d3d12_fence_decref(op->u.wait.fence);
            break;

        case VKD3D_CS_OP_SIGNAL:
            d3d12_fence_decref(op->u.signal.fence);
            break;

        case VKD3D_CS_OP_EXECUTE:
            vkd3d_free(op->u.execute.buffers);
            break;

        case VKD3D_CS_OP_UPDATE_MAPPINGS:
        case VKD3D_CS_OP_COPY_MAPPINGS:
            break;
    }
}

static void d3d12_command_queue_op_array_destroy(struct d3d12_command_queue_op_array *array)
{
    unsigned int i;

    for (i = 0; i < array->count; ++i)
        d3d12_command_queue_destroy_op(&array->ops[i]);

    vkd3d_free(array->ops);
}

static ULONG STDMETHODCALLTYPE d3d12_command_queue_Release(ID3D12CommandQueue *iface)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&command_queue->refcount);

    TRACE("%p decreasing refcount to %u.\n", command_queue, refcount);

    if (!refcount)
    {
        struct d3d12_device *device = command_queue->device;

        vkd3d_fence_worker_stop(&command_queue->fence_worker, device);

        vkd3d_mutex_destroy(&command_queue->op_mutex);
        d3d12_command_queue_op_array_destroy(&command_queue->op_queue);
        d3d12_command_queue_op_array_destroy(&command_queue->aux_op_queue);

        vkd3d_private_store_destroy(&command_queue->private_store);

        vkd3d_free(command_queue);

        d3d12_device_release(device);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_GetPrivateData(ID3D12CommandQueue *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&command_queue->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_SetPrivateData(ID3D12CommandQueue *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&command_queue->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_SetPrivateDataInterface(ID3D12CommandQueue *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&command_queue->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_SetName(ID3D12CommandQueue *iface, const WCHAR *name)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    VkQueue vk_queue;
    HRESULT hr;

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, command_queue->device->wchar_size));

    if (!(vk_queue = vkd3d_queue_acquire(command_queue->vkd3d_queue)))
    {
        ERR("Failed to acquire queue %p.\n", command_queue->vkd3d_queue);
        return E_FAIL;
    }

    hr = vkd3d_set_vk_object_name(command_queue->device, (uint64_t)(uintptr_t)vk_queue,
            VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, name);
    vkd3d_queue_release(command_queue->vkd3d_queue);
    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_GetDevice(ID3D12CommandQueue *iface, REFIID iid, void **device)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(command_queue->device, iid, device);
}

static struct vkd3d_cs_op_data *d3d12_command_queue_op_array_require_space(struct d3d12_command_queue_op_array *array)
{
    if (!vkd3d_array_reserve((void **)&array->ops, &array->size, array->count + 1, sizeof(*array->ops)))
        return NULL;

    return &array->ops[array->count++];
}

static bool clone_array_parameter(void **dst, const void *src, size_t elem_size, unsigned int count)
{
    void *buffer;

    *dst = NULL;
    if (src)
    {
        if (!(buffer = vkd3d_calloc(count, elem_size)))
            return false;
        memcpy(buffer, src, count * elem_size);
        *dst = buffer;
    }
    return true;
}

static void update_mappings_cleanup(struct vkd3d_cs_update_mappings *update_mappings)
{
    vkd3d_free(update_mappings->region_start_coordinates);
    vkd3d_free(update_mappings->region_sizes);
    vkd3d_free(update_mappings->range_flags);
    vkd3d_free(update_mappings->heap_range_offsets);
    vkd3d_free(update_mappings->range_tile_counts);
}

static void STDMETHODCALLTYPE d3d12_command_queue_UpdateTileMappings(ID3D12CommandQueue *iface,
        ID3D12Resource *resource, UINT region_count,
        const D3D12_TILED_RESOURCE_COORDINATE *region_start_coordinates, const D3D12_TILE_REGION_SIZE *region_sizes,
        ID3D12Heap *heap, UINT range_count, const D3D12_TILE_RANGE_FLAGS *range_flags,
        const UINT *heap_range_offsets, const UINT *range_tile_counts, D3D12_TILE_MAPPING_FLAGS flags)
{
    struct d3d12_resource *resource_impl = unsafe_impl_from_ID3D12Resource(resource);
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    struct d3d12_heap *heap_impl = unsafe_impl_from_ID3D12Heap(heap);
    struct vkd3d_cs_update_mappings update_mappings = {0};
    struct vkd3d_cs_op_data *op;

    TRACE("iface %p, resource %p, region_count %u, region_start_coordinates %p, "
            "region_sizes %p, heap %p, range_count %u, range_flags %p, heap_range_offsets %p, "
            "range_tile_counts %p, flags %#x.\n",
            iface, resource, region_count, region_start_coordinates, region_sizes, heap, range_count,
            range_flags, heap_range_offsets, range_tile_counts, flags);

    if (!region_count || !range_count)
        return;

    if (!command_queue->supports_sparse_binding)
    {
        FIXME("Command queue %p does not support sparse binding.\n", command_queue);
        return;
    }

    if (!resource_impl->tiles.subresource_count)
    {
        WARN("Resource %p is not a tiled resource.\n", resource_impl);
        return;
    }

    if (region_count > 1 && !region_start_coordinates)
    {
        WARN("Region start coordinates must not be NULL when region count is > 1.\n");
        return;
    }

    if (range_count > 1 && !range_tile_counts)
    {
        WARN("Range tile counts must not be NULL when range count is > 1.\n");
        return;
    }

    update_mappings.resource = resource_impl;
    update_mappings.heap = heap_impl;
    if (!clone_array_parameter((void **)&update_mappings.region_start_coordinates,
            region_start_coordinates, sizeof(*region_start_coordinates), region_count))
    {
        ERR("Failed to allocate region start coordinates.\n");
        return;
    }
    if (!clone_array_parameter((void **)&update_mappings.region_sizes,
            region_sizes, sizeof(*region_sizes), region_count))
    {
        ERR("Failed to allocate region sizes.\n");
        goto free_clones;
    }
    if (!clone_array_parameter((void **)&update_mappings.range_flags,
            range_flags, sizeof(*range_flags), range_count))
    {
        ERR("Failed to allocate range flags.\n");
        goto free_clones;
    }
    if (!clone_array_parameter((void **)&update_mappings.heap_range_offsets,
            heap_range_offsets, sizeof(*heap_range_offsets), range_count))
    {
        ERR("Failed to allocate heap range offsets.\n");
        goto free_clones;
    }
    if (!clone_array_parameter((void **)&update_mappings.range_tile_counts,
            range_tile_counts, sizeof(*range_tile_counts), range_count))
    {
        ERR("Failed to allocate range tile counts.\n");
        goto free_clones;
    }
    update_mappings.region_count = region_count;
    update_mappings.range_count = range_count;
    update_mappings.flags = flags;

    vkd3d_mutex_lock(&command_queue->op_mutex);

    if (!(op = d3d12_command_queue_op_array_require_space(&command_queue->op_queue)))
    {
        ERR("Failed to add op.\n");
        goto unlock_mutex;
    }

    op->opcode = VKD3D_CS_OP_UPDATE_MAPPINGS;
    op->u.update_mappings = update_mappings;

    d3d12_command_queue_submit_locked(command_queue);

    vkd3d_mutex_unlock(&command_queue->op_mutex);
    return;

unlock_mutex:
    vkd3d_mutex_unlock(&command_queue->op_mutex);
free_clones:
    update_mappings_cleanup(&update_mappings);
}

static void STDMETHODCALLTYPE d3d12_command_queue_CopyTileMappings(ID3D12CommandQueue *iface,
        ID3D12Resource *dst_resource,
        const D3D12_TILED_RESOURCE_COORDINATE *dst_region_start_coordinate,
        ID3D12Resource *src_resource,
        const D3D12_TILED_RESOURCE_COORDINATE *src_region_start_coordinate,
        const D3D12_TILE_REGION_SIZE *region_size,
        D3D12_TILE_MAPPING_FLAGS flags)
{
    struct d3d12_resource *dst_resource_impl = impl_from_ID3D12Resource(dst_resource);
    struct d3d12_resource *src_resource_impl = impl_from_ID3D12Resource(src_resource);
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    struct vkd3d_cs_op_data *op;

    TRACE("iface %p, dst_resource %p, dst_region_start_coordinate %p, "
            "src_resource %p, src_region_start_coordinate %p, region_size %p, flags %#x.\n",
            iface, dst_resource, dst_region_start_coordinate, src_resource,
            src_region_start_coordinate, region_size, flags);

    vkd3d_mutex_lock(&command_queue->op_mutex);

    if (!(op = d3d12_command_queue_op_array_require_space(&command_queue->op_queue)))
    {
        ERR("Failed to add op.\n");
        goto unlock_mutex;
    }
    op->opcode = VKD3D_CS_OP_COPY_MAPPINGS;
    op->u.copy_mappings.dst_resource = dst_resource_impl;
    op->u.copy_mappings.src_resource = src_resource_impl;
    op->u.copy_mappings.dst_region_start_coordinate = *dst_region_start_coordinate;
    op->u.copy_mappings.src_region_start_coordinate = *src_region_start_coordinate;
    op->u.copy_mappings.region_size = *region_size;
    op->u.copy_mappings.flags = flags;

    d3d12_command_queue_submit_locked(command_queue);

unlock_mutex:
    vkd3d_mutex_unlock(&command_queue->op_mutex);
}

static void d3d12_command_queue_execute(struct d3d12_command_queue *command_queue,
        VkCommandBuffer *buffers, unsigned int count)
{
    const struct vkd3d_vk_device_procs *vk_procs = &command_queue->device->vk_procs;
    struct vkd3d_queue *vkd3d_queue = command_queue->vkd3d_queue;
    VkSubmitInfo submit_desc;
    VkQueue vk_queue;
    VkResult vr;

    memset(&submit_desc, 0, sizeof(submit_desc));

    if (!(vk_queue = vkd3d_queue_acquire(vkd3d_queue)))
    {
        ERR("Failed to acquire queue %p.\n", vkd3d_queue);
        return;
    }

    submit_desc.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_desc.commandBufferCount = count;
    submit_desc.pCommandBuffers = buffers;

    if ((vr = VK_CALL(vkQueueSubmit(vk_queue, 1, &submit_desc, VK_NULL_HANDLE))) < 0)
        ERR("Failed to submit queue(s), vr %d.\n", vr);

    vkd3d_queue_release(vkd3d_queue);
}

static void d3d12_command_queue_submit_locked(struct d3d12_command_queue *queue)
{
    bool flushed_any = false;
    HRESULT hr;

    if (queue->op_queue.count == 1 && !queue->is_flushing)
    {
        if (FAILED(hr = d3d12_command_queue_flush_ops_locked(queue, &flushed_any)))
            ERR("Failed to flush queue, hr %s.\n", debugstr_hresult(hr));
    }
}

static void STDMETHODCALLTYPE d3d12_command_queue_ExecuteCommandLists(ID3D12CommandQueue *iface,
        UINT command_list_count, ID3D12CommandList * const *command_lists)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    struct d3d12_command_list *cmd_list;
    struct vkd3d_cs_op_data *op;
    VkCommandBuffer *buffers;
    unsigned int i;

    TRACE("iface %p, command_list_count %u, command_lists %p.\n",
            iface, command_list_count, command_lists);

    if (!command_list_count)
        return;

    if (!(buffers = vkd3d_calloc(command_list_count, sizeof(*buffers))))
    {
        ERR("Failed to allocate command buffer array.\n");
        return;
    }

    for (i = 0; i < command_list_count; ++i)
    {
        cmd_list = unsafe_impl_from_ID3D12CommandList(command_lists[i]);

        if (cmd_list->is_recording)
        {
            d3d12_device_mark_as_removed(command_queue->device, DXGI_ERROR_INVALID_CALL,
                    "Command list %p is in recording state.", command_lists[i]);
            vkd3d_free(buffers);
            return;
        }

        command_list_flush_vk_heap_updates(cmd_list);

        buffers[i] = cmd_list->vk_command_buffer;
    }

    vkd3d_mutex_lock(&command_queue->op_mutex);

    if (!(op = d3d12_command_queue_op_array_require_space(&command_queue->op_queue)))
    {
        ERR("Failed to add op.\n");
        goto done;
    }
    op->opcode = VKD3D_CS_OP_EXECUTE;
    op->u.execute.buffers = buffers;
    op->u.execute.buffer_count = command_list_count;

    d3d12_command_queue_submit_locked(command_queue);

done:
    vkd3d_mutex_unlock(&command_queue->op_mutex);
    return;
}

static void STDMETHODCALLTYPE d3d12_command_queue_SetMarker(ID3D12CommandQueue *iface,
        UINT metadata, const void *data, UINT size)
{
    FIXME("iface %p, metadata %#x, data %p, size %u stub!\n",
            iface, metadata, data, size);
}

static void STDMETHODCALLTYPE d3d12_command_queue_BeginEvent(ID3D12CommandQueue *iface,
        UINT metadata, const void *data, UINT size)
{
    FIXME("iface %p, metadata %#x, data %p, size %u stub!\n",
            iface, metadata, data, size);
}

static void STDMETHODCALLTYPE d3d12_command_queue_EndEvent(ID3D12CommandQueue *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static HRESULT vkd3d_enqueue_timeline_semaphore(struct vkd3d_fence_worker *worker, VkSemaphore vk_semaphore,
        struct d3d12_fence *fence, uint64_t value, struct vkd3d_queue *queue)
{
    struct vkd3d_waiting_fence *waiting_fence;

    TRACE("worker %p, fence %p, value %#"PRIx64".\n", worker, fence, value);

    vkd3d_mutex_lock(&worker->mutex);

    if (!vkd3d_array_reserve((void **)&worker->fences, &worker->fences_size,
            worker->fence_count + 1, sizeof(*worker->fences)))
    {
        ERR("Failed to add GPU timeline semaphore.\n");
        vkd3d_mutex_unlock(&worker->mutex);
        return E_OUTOFMEMORY;
    }

    waiting_fence = &worker->fences[worker->fence_count++];
    waiting_fence->fence = fence;
    waiting_fence->value = value;
    waiting_fence->u.vk_semaphore = vk_semaphore;

    d3d12_fence_incref(fence);

    vkd3d_cond_signal(&worker->cond);
    vkd3d_mutex_unlock(&worker->mutex);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_Signal(ID3D12CommandQueue *iface,
        ID3D12Fence *fence_iface, UINT64 value)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    struct d3d12_fence *fence = unsafe_impl_from_ID3D12Fence(fence_iface);
    struct vkd3d_cs_op_data *op;
    HRESULT hr = S_OK;

    TRACE("iface %p, fence %p, value %#"PRIx64".\n", iface, fence_iface, value);

    vkd3d_mutex_lock(&command_queue->op_mutex);

    if (!(op = d3d12_command_queue_op_array_require_space(&command_queue->op_queue)))
    {
        ERR("Failed to add op.\n");
        hr = E_OUTOFMEMORY;
        goto done;
    }
    op->opcode = VKD3D_CS_OP_SIGNAL;
    op->u.signal.fence = fence;
    op->u.signal.value = value;

    d3d12_fence_incref(fence);

    d3d12_command_queue_submit_locked(command_queue);

done:
    vkd3d_mutex_unlock(&command_queue->op_mutex);
    return hr;
}

static HRESULT d3d12_command_queue_signal(struct d3d12_command_queue *command_queue,
        struct d3d12_fence *fence, uint64_t value)
{
    VkTimelineSemaphoreSubmitInfoKHR timeline_submit_info;
    const struct vkd3d_vk_device_procs *vk_procs;
    VkSemaphore vk_semaphore = VK_NULL_HANDLE;
    VkFence vk_fence = VK_NULL_HANDLE;
    struct vkd3d_queue *vkd3d_queue;
    uint64_t sequence_number = 0;
    uint64_t timeline_value = 0;
    struct d3d12_device *device;
    VkSubmitInfo submit_info;
    VkQueue vk_queue;
    VkResult vr;
    HRESULT hr;

    device = command_queue->device;
    vk_procs = &device->vk_procs;
    vkd3d_queue = command_queue->vkd3d_queue;

    if (device->vk_info.KHR_timeline_semaphore)
    {
        if (!(timeline_value = d3d12_fence_add_pending_timeline_signal(fence, value, vkd3d_queue)))
        {
            ERR("Failed to add pending signal.\n");
            return E_OUTOFMEMORY;
        }

        vk_semaphore = fence->timeline_semaphore;
        VKD3D_ASSERT(vk_semaphore);
    }
    else
    {
        if ((vr = d3d12_fence_create_vk_fence(fence, &vk_fence)) < 0)
        {
            WARN("Failed to create Vulkan fence, vr %d.\n", vr);
            goto fail_vkresult;
        }
    }

    if (!(vk_queue = vkd3d_queue_acquire(vkd3d_queue)))
    {
        ERR("Failed to acquire queue %p.\n", vkd3d_queue);
        hr = E_FAIL;
        goto fail;
    }

    if (!device->vk_info.KHR_timeline_semaphore && (vr = vkd3d_queue_create_vk_semaphore_locked(vkd3d_queue,
            device, &vk_semaphore)) < 0)
    {
        ERR("Failed to create Vulkan semaphore, vr %d.\n", vr);
        vk_semaphore = VK_NULL_HANDLE;
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 0;
    submit_info.pCommandBuffers = NULL;
    submit_info.signalSemaphoreCount = vk_semaphore ? 1 : 0;
    submit_info.pSignalSemaphores = &vk_semaphore;

    if (device->vk_info.KHR_timeline_semaphore)
    {
        timeline_submit_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR;
        timeline_submit_info.pNext = NULL;
        timeline_submit_info.pSignalSemaphoreValues = &timeline_value;
        timeline_submit_info.signalSemaphoreValueCount = submit_info.signalSemaphoreCount;
        timeline_submit_info.waitSemaphoreValueCount = 0;
        timeline_submit_info.pWaitSemaphoreValues = NULL;
        submit_info.pNext = &timeline_submit_info;
    }

    vr = VK_CALL(vkQueueSubmit(vk_queue, 1, &submit_info, vk_fence));
    if (!device->vk_info.KHR_timeline_semaphore && vr >= 0)
    {
        sequence_number = ++vkd3d_queue->submitted_sequence_number;

        /* We don't expect to overrun the 64-bit counter, but we handle it gracefully anyway. */
        if (!sequence_number)
            sequence_number = vkd3d_queue_reset_sequence_number_locked(vkd3d_queue);
    }

    vkd3d_queue_release(vkd3d_queue);

    if (vr < 0)
    {
        WARN("Failed to submit signal operation, vr %d.\n", vr);
        goto fail_vkresult;
    }

    if (device->vk_info.KHR_timeline_semaphore)
    {
        if (FAILED(hr = d3d12_fence_update_pending_value(fence)))
            return hr;

        if (FAILED(hr = d3d12_device_flush_blocked_queues(device)))
            return hr;

        vk_semaphore = fence->timeline_semaphore;
        VKD3D_ASSERT(vk_semaphore);

        return vkd3d_enqueue_timeline_semaphore(&command_queue->fence_worker,
                vk_semaphore, fence, timeline_value, vkd3d_queue);
    }

    if (vk_semaphore && SUCCEEDED(hr = d3d12_fence_add_vk_semaphore(fence, vk_semaphore, vk_fence, value, vkd3d_queue)))
        vk_semaphore = VK_NULL_HANDLE;

    vr = VK_CALL(vkGetFenceStatus(device->vk_device, vk_fence));
    if (vr == VK_NOT_READY)
    {
        if (SUCCEEDED(hr = vkd3d_enqueue_gpu_fence(&command_queue->fence_worker,
                vk_fence, fence, value, vkd3d_queue, sequence_number)))
        {
            vk_fence = VK_NULL_HANDLE;
        }
    }
    else if (vr == VK_SUCCESS)
    {
        TRACE("Already signaled %p, value %#"PRIx64".\n", fence, value);
        hr = d3d12_fence_signal(fence, value, vk_fence, false);
        vk_fence = VK_NULL_HANDLE;
        vkd3d_queue_update_sequence_number(vkd3d_queue, sequence_number, device);
    }
    else
    {
        FIXME("Failed to get fence status, vr %d.\n", vr);
        hr = hresult_from_vk_result(vr);
    }

    if (vk_fence || vk_semaphore)
    {
        /* In case of an unexpected failure, try to safely destroy Vulkan objects. */
        vkd3d_queue_wait_idle(vkd3d_queue, vk_procs);
        goto fail;
    }

    return hr;

fail_vkresult:
    hr = hresult_from_vk_result(vr);
fail:
    VK_CALL(vkDestroyFence(device->vk_device, vk_fence, NULL));
    if (!device->vk_info.KHR_timeline_semaphore)
        VK_CALL(vkDestroySemaphore(device->vk_device, vk_semaphore, NULL));
    return hr;
}

static HRESULT d3d12_command_queue_wait_binary_semaphore_locked(struct d3d12_command_queue *command_queue,
        struct d3d12_fence *fence, uint64_t value)
{
    static const VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    const struct vkd3d_vk_device_procs *vk_procs;
    struct vkd3d_signaled_semaphore *semaphore;
    uint64_t completed_value = 0;
    struct vkd3d_queue *queue;
    VkSubmitInfo submit_info;
    VkQueue vk_queue;
    VkResult vr;
    HRESULT hr;

    vk_procs = &command_queue->device->vk_procs;
    queue = command_queue->vkd3d_queue;

    semaphore = d3d12_fence_acquire_vk_semaphore_locked(fence, value, &completed_value);

    vkd3d_mutex_unlock(&fence->mutex);

    if (!semaphore && completed_value >= value)
    {
        /* We don't get a Vulkan semaphore if the fence was signaled on CPU. */
        TRACE("Already signaled %p, value %#"PRIx64".\n", fence, completed_value);
        return S_OK;
    }

    if (!(vk_queue = vkd3d_queue_acquire(queue)))
    {
        ERR("Failed to acquire queue %p.\n", queue);
        hr = E_FAIL;
        goto fail;
    }

    if (!semaphore)
    {
        if (command_queue->last_waited_fence == fence && command_queue->last_waited_fence_value >= value)
        {
            WARN("Already waited on fence %p, value %#"PRIx64".\n", fence, value);
        }
        else
        {
            WARN("Failed to acquire Vulkan semaphore for fence %p, value %#"PRIx64
                    ", completed value %#"PRIx64".\n", fence, value, completed_value);
        }

        vkd3d_queue_release(queue);
        return S_OK;
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &semaphore->u.binary.vk_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage_mask;
    submit_info.commandBufferCount = 0;
    submit_info.pCommandBuffers = NULL;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    if (!vkd3d_array_reserve((void **)&queue->semaphores, &queue->semaphores_size,
            queue->semaphore_count + 1, sizeof(*queue->semaphores)))
    {
        ERR("Failed to allocate memory for semaphore.\n");
        vkd3d_queue_release(queue);
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    if ((vr = VK_CALL(vkQueueSubmit(vk_queue, 1, &submit_info, VK_NULL_HANDLE))) >= 0)
    {
        queue->semaphores[queue->semaphore_count].vk_semaphore = semaphore->u.binary.vk_semaphore;
        queue->semaphores[queue->semaphore_count].sequence_number = queue->submitted_sequence_number + 1;
        ++queue->semaphore_count;

        command_queue->last_waited_fence = fence;
        command_queue->last_waited_fence_value = value;
    }

    vkd3d_queue_release(queue);

    if (vr < 0)
    {
        WARN("Failed to submit wait operation, vr %d.\n", vr);
        hr = hresult_from_vk_result(vr);
        goto fail;
    }

    d3d12_fence_remove_vk_semaphore(fence, semaphore);
    return S_OK;

fail:
    d3d12_fence_release_vk_semaphore(fence, semaphore);
    return hr;
}

static HRESULT d3d12_command_queue_wait_locked(struct d3d12_command_queue *command_queue,
        struct d3d12_fence *fence, uint64_t value)
{
    static const VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkTimelineSemaphoreSubmitInfoKHR timeline_submit_info;
    const struct vkd3d_vk_device_procs *vk_procs;
    struct vkd3d_queue *queue;
    VkSubmitInfo submit_info;
    uint64_t wait_value;
    VkQueue vk_queue;
    VkResult vr;

    vk_procs = &command_queue->device->vk_procs;
    queue = command_queue->vkd3d_queue;

    if (!command_queue->device->vk_info.KHR_timeline_semaphore)
        return d3d12_command_queue_wait_binary_semaphore_locked(command_queue, fence, value);

    wait_value = d3d12_fence_get_timeline_wait_value_locked(fence, value);

    /* We can unlock the fence here. The queue semaphore will not be signalled to signal_value
     * until we have submitted, so the semaphore cannot be destroyed before the call to vkQueueSubmit. */
    vkd3d_mutex_unlock(&fence->mutex);

    VKD3D_ASSERT(fence->timeline_semaphore);
    timeline_submit_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR;
    timeline_submit_info.pNext = NULL;
    timeline_submit_info.waitSemaphoreValueCount = 1;
    timeline_submit_info.pWaitSemaphoreValues = &wait_value;
    timeline_submit_info.signalSemaphoreValueCount = 0;
    timeline_submit_info.pSignalSemaphoreValues = NULL;

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = &timeline_submit_info;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &fence->timeline_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage_mask;
    submit_info.commandBufferCount = 0;
    submit_info.pCommandBuffers = NULL;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    if (!(vk_queue = vkd3d_queue_acquire(queue)))
    {
        ERR("Failed to acquire queue %p.\n", queue);
        return E_FAIL;
    }

    vr = VK_CALL(vkQueueSubmit(vk_queue, 1, &submit_info, VK_NULL_HANDLE));

    vkd3d_queue_release(queue);

    if (vr < 0)
    {
        WARN("Failed to submit wait operation, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_Wait(ID3D12CommandQueue *iface,
        ID3D12Fence *fence_iface, UINT64 value)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    struct d3d12_fence *fence = unsafe_impl_from_ID3D12Fence(fence_iface);
    struct vkd3d_cs_op_data *op;
    HRESULT hr = S_OK;

    TRACE("iface %p, fence %p, value %#"PRIx64".\n", iface, fence_iface, value);

    vkd3d_mutex_lock(&command_queue->op_mutex);

    if (!(op = d3d12_command_queue_op_array_require_space(&command_queue->op_queue)))
    {
        ERR("Failed to add op.\n");
        hr = E_OUTOFMEMORY;
        goto done;
    }
    op->opcode = VKD3D_CS_OP_WAIT;
    op->u.wait.fence = fence;
    op->u.wait.value = value;

    d3d12_fence_incref(fence);

    d3d12_command_queue_submit_locked(command_queue);

done:
    vkd3d_mutex_unlock(&command_queue->op_mutex);
    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_GetTimestampFrequency(ID3D12CommandQueue *iface,
        UINT64 *frequency)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    struct d3d12_device *device = command_queue->device;

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    if (!command_queue->vkd3d_queue->timestamp_bits)
    {
        WARN("Timestamp queries not supported.\n");
        return E_FAIL;
    }

    *frequency = 1000000000 / device->vk_info.device_limits.timestampPeriod;

    return S_OK;
}

#define NANOSECONDS_IN_A_SECOND 1000000000

static HRESULT STDMETHODCALLTYPE d3d12_command_queue_GetClockCalibration(ID3D12CommandQueue *iface,
        UINT64 *gpu_timestamp, UINT64 *cpu_timestamp)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);
    struct d3d12_device *device = command_queue->device;
    const struct vkd3d_vk_device_procs *vk_procs;
    VkCalibratedTimestampInfoEXT infos[2];
    uint64_t timestamps[2];
    uint64_t deviations[2];
    VkResult vr;

    TRACE("iface %p, gpu_timestamp %p, cpu_timestamp %p.\n",
            iface, gpu_timestamp, cpu_timestamp);

    if (!command_queue->vkd3d_queue->timestamp_bits)
    {
        WARN("Timestamp queries not supported.\n");
        return E_FAIL;
    }

    if (!gpu_timestamp || !cpu_timestamp)
        return E_INVALIDARG;

    if (!device->vk_info.EXT_calibrated_timestamps || device->vk_host_time_domain == -1)
    {
        WARN(!device->vk_info.EXT_calibrated_timestamps
                ? "VK_EXT_calibrated_timestamps was not found. Setting timestamps to zero.\n"
                : "Device and/or host time domain is not available. Setting timestamps to zero.\n");
        *gpu_timestamp = 0;
        *cpu_timestamp = 0;
        return S_OK;
    }

    vk_procs = &device->vk_procs;

    infos[0].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
    infos[0].pNext = NULL;
    infos[0].timeDomain = VK_TIME_DOMAIN_DEVICE_EXT;
    infos[1].sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
    infos[1].pNext = NULL;
    infos[1].timeDomain = device->vk_host_time_domain;

    if ((vr = VK_CALL(vkGetCalibratedTimestampsEXT(command_queue->device->vk_device,
            ARRAY_SIZE(infos), infos, timestamps, deviations))) < 0)
    {
        WARN("Failed to get calibrated timestamps, vr %d.\n", vr);
        return E_FAIL;
    }

    if (infos[1].timeDomain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT
            || infos[1].timeDomain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT)
    {
        /* Convert monotonic clock to match Wine's RtlQueryPerformanceFrequency(). */
        timestamps[1] /= NANOSECONDS_IN_A_SECOND / device->vkd3d_instance->host_ticks_per_second;
    }

    *gpu_timestamp = timestamps[0];
    *cpu_timestamp = timestamps[1];

    return S_OK;
}

static D3D12_COMMAND_QUEUE_DESC * STDMETHODCALLTYPE d3d12_command_queue_GetDesc(ID3D12CommandQueue *iface,
        D3D12_COMMAND_QUEUE_DESC *desc)
{
    struct d3d12_command_queue *command_queue = impl_from_ID3D12CommandQueue(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = command_queue->desc;
    return desc;
}

static const struct ID3D12CommandQueueVtbl d3d12_command_queue_vtbl =
{
    /* IUnknown methods */
    d3d12_command_queue_QueryInterface,
    d3d12_command_queue_AddRef,
    d3d12_command_queue_Release,
    /* ID3D12Object methods */
    d3d12_command_queue_GetPrivateData,
    d3d12_command_queue_SetPrivateData,
    d3d12_command_queue_SetPrivateDataInterface,
    d3d12_command_queue_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_command_queue_GetDevice,
    /* ID3D12CommandQueue methods */
    d3d12_command_queue_UpdateTileMappings,
    d3d12_command_queue_CopyTileMappings,
    d3d12_command_queue_ExecuteCommandLists,
    d3d12_command_queue_SetMarker,
    d3d12_command_queue_BeginEvent,
    d3d12_command_queue_EndEvent,
    d3d12_command_queue_Signal,
    d3d12_command_queue_Wait,
    d3d12_command_queue_GetTimestampFrequency,
    d3d12_command_queue_GetClockCalibration,
    d3d12_command_queue_GetDesc,
};

static void d3d12_command_queue_swap_queues(struct d3d12_command_queue *queue)
{
    struct d3d12_command_queue_op_array array;

    array = queue->op_queue;
    queue->op_queue = queue->aux_op_queue;
    queue->aux_op_queue = array;
}

static bool d3d12_command_queue_op_array_append(struct d3d12_command_queue_op_array *array,
        size_t count, const struct vkd3d_cs_op_data *new_ops)
{
    if (!vkd3d_array_reserve((void **)&array->ops, &array->size, array->count + count, sizeof(*array->ops)))
    {
        ERR("Cannot reserve memory for %zu new ops.\n", count);
        return false;
    }

    memcpy(&array->ops[array->count], new_ops, count * sizeof(*array->ops));
    array->count += count;

    return true;
}

static void d3d12_command_queue_delete_aux_ops(struct d3d12_command_queue *queue,
        unsigned int done_count)
{
    queue->aux_op_queue.count -= done_count;
    memmove(queue->aux_op_queue.ops, &queue->aux_op_queue.ops[done_count],
            queue->aux_op_queue.count * sizeof(*queue->aux_op_queue.ops));
}

static HRESULT d3d12_command_queue_fixup_after_flush_locked(struct d3d12_command_queue *queue)
{
    d3d12_command_queue_swap_queues(queue);

    d3d12_command_queue_op_array_append(&queue->op_queue, queue->aux_op_queue.count, queue->aux_op_queue.ops);

    queue->aux_op_queue.count = 0;
    queue->is_flushing = false;

    return d3d12_command_queue_record_as_blocked(queue);
}

static HRESULT d3d12_command_queue_flush_ops(struct d3d12_command_queue *queue, bool *flushed_any)
{
    HRESULT hr;

    vkd3d_mutex_lock(&queue->op_mutex);

    /* This function may be re-entered when invoking
     * d3d12_command_queue_signal().  The first call is responsible
     * for re-adding the queue to the flush list. */
    if (queue->is_flushing)
    {
        vkd3d_mutex_unlock(&queue->op_mutex);
        return S_OK;
    }

    hr = d3d12_command_queue_flush_ops_locked(queue, flushed_any);

    vkd3d_mutex_unlock(&queue->op_mutex);

    return hr;
}

/* flushed_any is initialised by the caller. */
static HRESULT d3d12_command_queue_flush_ops_locked(struct d3d12_command_queue *queue, bool *flushed_any)
{
    struct vkd3d_cs_op_data *op;
    struct d3d12_fence *fence;
    unsigned int i;

    queue->is_flushing = true;

    VKD3D_ASSERT(queue->aux_op_queue.count == 0);

    while (queue->op_queue.count != 0)
    {
        d3d12_command_queue_swap_queues(queue);

        vkd3d_mutex_unlock(&queue->op_mutex);

        for (i = 0; i < queue->aux_op_queue.count; ++i)
        {
            op = &queue->aux_op_queue.ops[i];
            switch (op->opcode)
            {
                case VKD3D_CS_OP_WAIT:
                    fence = op->u.wait.fence;
                    vkd3d_mutex_lock(&fence->mutex);
                    if (op->u.wait.value > fence->max_pending_value)
                    {
                        vkd3d_mutex_unlock(&fence->mutex);
                        d3d12_command_queue_delete_aux_ops(queue, i);
                        vkd3d_mutex_lock(&queue->op_mutex);
                        return d3d12_command_queue_fixup_after_flush_locked(queue);
                    }
                    d3d12_command_queue_wait_locked(queue, fence, op->u.wait.value);
                    break;

                case VKD3D_CS_OP_SIGNAL:
                    d3d12_command_queue_signal(queue, op->u.signal.fence, op->u.signal.value);
                    break;

                case VKD3D_CS_OP_EXECUTE:
                    d3d12_command_queue_execute(queue, op->u.execute.buffers, op->u.execute.buffer_count);
                    break;

                case VKD3D_CS_OP_UPDATE_MAPPINGS:
                    FIXME("Tiled resource binding is not supported yet.\n");
                    update_mappings_cleanup(&op->u.update_mappings);
                    break;

                case VKD3D_CS_OP_COPY_MAPPINGS:
                    FIXME("Tiled resource mapping copying is not supported yet.\n");
                    break;

                default:
                    vkd3d_unreachable();
            }

            d3d12_command_queue_destroy_op(op);

            *flushed_any |= true;
        }

        queue->aux_op_queue.count = 0;

        vkd3d_mutex_lock(&queue->op_mutex);
    }

    queue->is_flushing = false;

    return S_OK;
}

static void d3d12_command_queue_op_array_init(struct d3d12_command_queue_op_array *array)
{
    array->ops = NULL;
    array->count = 0;
    array->size = 0;
}

static HRESULT d3d12_command_queue_init(struct d3d12_command_queue *queue,
        struct d3d12_device *device, const D3D12_COMMAND_QUEUE_DESC *desc)
{
    HRESULT hr;

    queue->ID3D12CommandQueue_iface.lpVtbl = &d3d12_command_queue_vtbl;
    queue->refcount = 1;

    queue->desc = *desc;
    if (!queue->desc.NodeMask)
        queue->desc.NodeMask = 0x1;

    if (!(queue->vkd3d_queue = d3d12_device_get_vkd3d_queue(device, desc->Type)))
        return E_NOTIMPL;

    queue->last_waited_fence = NULL;
    queue->last_waited_fence_value = 0;

    d3d12_command_queue_op_array_init(&queue->op_queue);
    queue->is_flushing = false;

    d3d12_command_queue_op_array_init(&queue->aux_op_queue);

    if (desc->Priority == D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME)
    {
        FIXME("Global realtime priority is not implemented.\n");
        return E_NOTIMPL;
    }

    if (desc->Priority)
        FIXME("Ignoring priority %#x.\n", desc->Priority);
    if (desc->Flags)
        FIXME("Ignoring flags %#x.\n", desc->Flags);

    if (FAILED(hr = vkd3d_private_store_init(&queue->private_store)))
        return hr;

    vkd3d_mutex_init(&queue->op_mutex);

    if (FAILED(hr = vkd3d_fence_worker_start(&queue->fence_worker, queue->vkd3d_queue, device)))
        goto fail_destroy_op_mutex;

    queue->supports_sparse_binding = !!(queue->vkd3d_queue->vk_queue_flags & VK_QUEUE_SPARSE_BINDING_BIT);

    d3d12_device_add_ref(queue->device = device);

    return S_OK;

fail_destroy_op_mutex:
    vkd3d_mutex_destroy(&queue->op_mutex);
    vkd3d_private_store_destroy(&queue->private_store);
    return hr;
}

HRESULT d3d12_command_queue_create(struct d3d12_device *device,
        const D3D12_COMMAND_QUEUE_DESC *desc, struct d3d12_command_queue **queue)
{
    struct d3d12_command_queue *object;
    HRESULT hr;

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_command_queue_init(object, device, desc)))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created command queue %p.\n", object);

    *queue = object;

    return S_OK;
}

uint32_t vkd3d_get_vk_queue_family_index(ID3D12CommandQueue *queue)
{
    struct d3d12_command_queue *d3d12_queue = impl_from_ID3D12CommandQueue(queue);

    return d3d12_queue->vkd3d_queue->vk_family_index;
}

VkQueue vkd3d_acquire_vk_queue(ID3D12CommandQueue *queue)
{
    struct d3d12_command_queue *d3d12_queue = impl_from_ID3D12CommandQueue(queue);
    VkQueue vk_queue = vkd3d_queue_acquire(d3d12_queue->vkd3d_queue);

    if (d3d12_queue->op_queue.count)
        WARN("Acquired command queue %p with %zu remaining ops.\n", d3d12_queue, d3d12_queue->op_queue.count);
    else if (d3d12_queue->is_flushing)
        WARN("Acquired command queue %p which is flushing.\n", d3d12_queue);

    return vk_queue;
}

void vkd3d_release_vk_queue(ID3D12CommandQueue *queue)
{
    struct d3d12_command_queue *d3d12_queue = impl_from_ID3D12CommandQueue(queue);

    return vkd3d_queue_release(d3d12_queue->vkd3d_queue);
}

/* ID3D12CommandSignature */
static inline struct d3d12_command_signature *impl_from_ID3D12CommandSignature(ID3D12CommandSignature *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_command_signature, ID3D12CommandSignature_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_signature_QueryInterface(ID3D12CommandSignature *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID3D12CommandSignature)
            || IsEqualGUID(iid, &IID_ID3D12Pageable)
            || IsEqualGUID(iid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(iid, &IID_ID3D12Object)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID3D12CommandSignature_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_command_signature_AddRef(ID3D12CommandSignature *iface)
{
    struct d3d12_command_signature *signature = impl_from_ID3D12CommandSignature(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&signature->refcount);

    TRACE("%p increasing refcount to %u.\n", signature, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d12_command_signature_Release(ID3D12CommandSignature *iface)
{
    struct d3d12_command_signature *signature = impl_from_ID3D12CommandSignature(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&signature->refcount);

    TRACE("%p decreasing refcount to %u.\n", signature, refcount);

    if (!refcount)
        d3d12_command_signature_decref(signature);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_signature_GetPrivateData(ID3D12CommandSignature *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_command_signature *signature = impl_from_ID3D12CommandSignature(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&signature->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_signature_SetPrivateData(ID3D12CommandSignature *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_command_signature *signature = impl_from_ID3D12CommandSignature(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&signature->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_signature_SetPrivateDataInterface(ID3D12CommandSignature *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_command_signature *signature = impl_from_ID3D12CommandSignature(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&signature->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_command_signature_SetName(ID3D12CommandSignature *iface, const WCHAR *name)
{
    struct d3d12_command_signature *signature = impl_from_ID3D12CommandSignature(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, signature->device->wchar_size));

    return name ? S_OK : E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE d3d12_command_signature_GetDevice(ID3D12CommandSignature *iface, REFIID iid, void **device)
{
    struct d3d12_command_signature *signature = impl_from_ID3D12CommandSignature(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(signature->device, iid, device);
}

static const struct ID3D12CommandSignatureVtbl d3d12_command_signature_vtbl =
{
    /* IUnknown methods */
    d3d12_command_signature_QueryInterface,
    d3d12_command_signature_AddRef,
    d3d12_command_signature_Release,
    /* ID3D12Object methods */
    d3d12_command_signature_GetPrivateData,
    d3d12_command_signature_SetPrivateData,
    d3d12_command_signature_SetPrivateDataInterface,
    d3d12_command_signature_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_command_signature_GetDevice,
};

struct d3d12_command_signature *unsafe_impl_from_ID3D12CommandSignature(ID3D12CommandSignature *iface)
{
    if (!iface)
        return NULL;
    VKD3D_ASSERT(iface->lpVtbl == &d3d12_command_signature_vtbl);
    return CONTAINING_RECORD(iface, struct d3d12_command_signature, ID3D12CommandSignature_iface);
}

HRESULT d3d12_command_signature_create(struct d3d12_device *device, const D3D12_COMMAND_SIGNATURE_DESC *desc,
        struct d3d12_command_signature **signature)
{
    struct d3d12_command_signature *object;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < desc->NumArgumentDescs; ++i)
    {
        const D3D12_INDIRECT_ARGUMENT_DESC *argument_desc = &desc->pArgumentDescs[i];
        switch (argument_desc->Type)
        {
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
                if (i != desc->NumArgumentDescs - 1)
                {
                    WARN("Draw/dispatch must be the last element of a command signature.\n");
                    return E_INVALIDARG;
                }
                break;
            default:
                break;
        }
    }

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ID3D12CommandSignature_iface.lpVtbl = &d3d12_command_signature_vtbl;
    object->refcount = 1;
    object->internal_refcount = 1;

    object->desc = *desc;
    if (!(object->desc.pArgumentDescs = vkd3d_calloc(desc->NumArgumentDescs, sizeof(*desc->pArgumentDescs))))
    {
        vkd3d_free(object);
        return E_OUTOFMEMORY;
    }
    memcpy((void *)object->desc.pArgumentDescs, desc->pArgumentDescs,
            desc->NumArgumentDescs * sizeof(*desc->pArgumentDescs));

    if (FAILED(hr = vkd3d_private_store_init(&object->private_store)))
    {
        vkd3d_free((void *)object->desc.pArgumentDescs);
        vkd3d_free(object);
        return hr;
    }

    d3d12_device_add_ref(object->device = device);

    TRACE("Created command signature %p.\n", object);

    *signature = object;

    return S_OK;
}
