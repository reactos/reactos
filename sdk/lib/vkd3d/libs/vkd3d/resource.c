/*
 * Copyright 2016 Józef Kucia for CodeWeavers
 * Copyright 2019 Conor McCarthy for CodeWeavers
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

#define VKD3D_NULL_BUFFER_SIZE 16
#define VKD3D_NULL_VIEW_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM

uint64_t object_global_serial_id;

static inline bool is_cpu_accessible_heap(const D3D12_HEAP_PROPERTIES *properties)
{
    if (properties->Type == D3D12_HEAP_TYPE_DEFAULT)
        return false;
    if (properties->Type == D3D12_HEAP_TYPE_CUSTOM)
    {
        return properties->CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE
                || properties->CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    }
    return true;
}

static HRESULT vkd3d_select_memory_type(struct d3d12_device *device, uint32_t memory_type_mask,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags, unsigned int *type_index)
{
    const VkPhysicalDeviceMemoryProperties *memory_info = &device->memory_properties;
    VkMemoryPropertyFlags flags[3];
    unsigned int i, j, count = 0;

    switch (heap_properties->Type)
    {
        case D3D12_HEAP_TYPE_DEFAULT:
            flags[count++] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            break;

        case D3D12_HEAP_TYPE_UPLOAD:
            flags[count++] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;

        case D3D12_HEAP_TYPE_READBACK:
            flags[count++] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            flags[count++] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;

        case D3D12_HEAP_TYPE_CUSTOM:
            if (heap_properties->MemoryPoolPreference == D3D12_MEMORY_POOL_UNKNOWN
                    || (heap_properties->MemoryPoolPreference == D3D12_MEMORY_POOL_L1
                    && (is_cpu_accessible_heap(heap_properties) || d3d12_device_is_uma(device, NULL))))
            {
                WARN("Invalid memory pool preference.\n");
                return E_INVALIDARG;
            }

            switch (heap_properties->CPUPageProperty)
            {
                case D3D12_CPU_PAGE_PROPERTY_WRITE_BACK:
                    flags[count++] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                            | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                    /* Fall through. */
                case D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE:
                    flags[count++] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                    /* Fall through. */
                case D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE:
                    flags[count++] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                    break;
                case D3D12_CPU_PAGE_PROPERTY_UNKNOWN:
                default:
                    WARN("Invalid CPU page property.\n");
                    return E_INVALIDARG;
            }
            break;

        default:
            WARN("Invalid heap type %#x.\n", heap_properties->Type);
            return E_INVALIDARG;
    }

    for (j = 0; j < count; ++j)
    {
        VkMemoryPropertyFlags preferred_flags = flags[j];

        for (i = 0; i < memory_info->memoryTypeCount; ++i)
        {
            if (!(memory_type_mask & (1u << i)))
                continue;
            if ((memory_info->memoryTypes[i].propertyFlags & preferred_flags) == preferred_flags)
            {
                *type_index = i;
                return S_OK;
            }
        }
    }

    return E_FAIL;
}

static HRESULT vkd3d_allocate_device_memory(struct d3d12_device *device,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags,
        const VkMemoryRequirements *memory_requirements,
        const VkMemoryDedicatedAllocateInfo *dedicated_allocate_info,
        VkDeviceMemory *vk_memory, uint32_t *vk_memory_type)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkMemoryAllocateInfo allocate_info;
    VkResult vr;
    HRESULT hr;

    TRACE("Memory requirements: size %#"PRIx64", alignment %#"PRIx64".\n",
            memory_requirements->size, memory_requirements->alignment);

    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.pNext = dedicated_allocate_info;
    allocate_info.allocationSize = memory_requirements->size;
    if (FAILED(hr = vkd3d_select_memory_type(device, memory_requirements->memoryTypeBits,
            heap_properties, heap_flags, &allocate_info.memoryTypeIndex)))
    {
        if (hr != E_INVALIDARG)
            FIXME("Failed to find suitable memory type (allowed types %#x).\n", memory_requirements->memoryTypeBits);
        *vk_memory = VK_NULL_HANDLE;
        return hr;
    }

    TRACE("Allocating memory type %u.\n", allocate_info.memoryTypeIndex);

    if ((vr = VK_CALL(vkAllocateMemory(device->vk_device, &allocate_info, NULL, vk_memory))) < 0)
    {
        WARN("Failed to allocate device memory, vr %d.\n", vr);
        *vk_memory = VK_NULL_HANDLE;
        return hresult_from_vk_result(vr);
    }

    if (vk_memory_type)
        *vk_memory_type = allocate_info.memoryTypeIndex;

    return S_OK;
}

HRESULT vkd3d_allocate_buffer_memory(struct d3d12_device *device, VkBuffer vk_buffer,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags,
        VkDeviceMemory *vk_memory, uint32_t *vk_memory_type, VkDeviceSize *vk_memory_size)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkMemoryDedicatedAllocateInfo *dedicated_allocation = NULL;
    VkMemoryDedicatedRequirements dedicated_requirements;
    VkMemoryDedicatedAllocateInfo dedicated_info;
    VkMemoryRequirements2 memory_requirements2;
    VkMemoryRequirements *memory_requirements;
    VkBufferMemoryRequirementsInfo2 info;
    VkResult vr;
    HRESULT hr;

    memory_requirements = &memory_requirements2.memoryRequirements;

    if (device->vk_info.KHR_dedicated_allocation)
    {
        info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
        info.pNext = NULL;
        info.buffer = vk_buffer;

        dedicated_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
        dedicated_requirements.pNext = NULL;

        memory_requirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        memory_requirements2.pNext = &dedicated_requirements;

        VK_CALL(vkGetBufferMemoryRequirements2KHR(device->vk_device, &info, &memory_requirements2));

        if (dedicated_requirements.prefersDedicatedAllocation)
        {
            dedicated_allocation = &dedicated_info;

            dedicated_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
            dedicated_info.pNext = NULL;
            dedicated_info.image = VK_NULL_HANDLE;
            dedicated_info.buffer = vk_buffer;
        }
    }
    else
    {
        VK_CALL(vkGetBufferMemoryRequirements(device->vk_device, vk_buffer, memory_requirements));
    }

    if (FAILED(hr = vkd3d_allocate_device_memory(device, heap_properties, heap_flags,
            memory_requirements, dedicated_allocation, vk_memory, vk_memory_type)))
        return hr;

    if ((vr = VK_CALL(vkBindBufferMemory(device->vk_device, vk_buffer, *vk_memory, 0))) < 0)
    {
        WARN("Failed to bind memory, vr %d.\n", vr);
        VK_CALL(vkFreeMemory(device->vk_device, *vk_memory, NULL));
        *vk_memory = VK_NULL_HANDLE;
    }

    if (vk_memory_size)
        *vk_memory_size = memory_requirements->size;

    return hresult_from_vk_result(vr);
}

static HRESULT vkd3d_allocate_image_memory(struct d3d12_device *device, VkImage vk_image,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags,
        VkDeviceMemory *vk_memory, uint32_t *vk_memory_type, VkDeviceSize *vk_memory_size)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkMemoryDedicatedAllocateInfo *dedicated_allocation = NULL;
    VkMemoryDedicatedRequirements dedicated_requirements;
    VkMemoryDedicatedAllocateInfo dedicated_info;
    VkMemoryRequirements2 memory_requirements2;
    VkMemoryRequirements *memory_requirements;
    VkImageMemoryRequirementsInfo2 info;
    VkResult vr;
    HRESULT hr;

    memory_requirements = &memory_requirements2.memoryRequirements;

    if (device->vk_info.KHR_dedicated_allocation)
    {
        info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
        info.pNext = NULL;
        info.image = vk_image;

        dedicated_requirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
        dedicated_requirements.pNext = NULL;

        memory_requirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        memory_requirements2.pNext = &dedicated_requirements;

        VK_CALL(vkGetImageMemoryRequirements2KHR(device->vk_device, &info, &memory_requirements2));

        if (dedicated_requirements.prefersDedicatedAllocation)
        {
            dedicated_allocation = &dedicated_info;

            dedicated_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
            dedicated_info.pNext = NULL;
            dedicated_info.image = vk_image;
            dedicated_info.buffer = VK_NULL_HANDLE;
        }
    }
    else
    {
        VK_CALL(vkGetImageMemoryRequirements(device->vk_device, vk_image, memory_requirements));
    }

    if (FAILED(hr = vkd3d_allocate_device_memory(device, heap_properties, heap_flags,
            memory_requirements, dedicated_allocation, vk_memory, vk_memory_type)))
        return hr;

    if ((vr = VK_CALL(vkBindImageMemory(device->vk_device, vk_image, *vk_memory, 0))) < 0)
    {
        WARN("Failed to bind memory, vr %d.\n", vr);
        VK_CALL(vkFreeMemory(device->vk_device, *vk_memory, NULL));
        *vk_memory = VK_NULL_HANDLE;
        return hresult_from_vk_result(vr);
    }

    if (vk_memory_size)
        *vk_memory_size = memory_requirements->size;

    return S_OK;
}

/* ID3D12Heap */
static inline struct d3d12_heap *impl_from_ID3D12Heap(ID3D12Heap *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_heap, ID3D12Heap_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_heap_QueryInterface(ID3D12Heap *iface,
        REFIID iid, void **object)
{
    TRACE("iface %p, iid %s, object %p.\n", iface, debugstr_guid(iid), object);

    if (IsEqualGUID(iid, &IID_ID3D12Heap)
            || IsEqualGUID(iid, &IID_ID3D12Pageable)
            || IsEqualGUID(iid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(iid, &IID_ID3D12Object)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID3D12Heap_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_heap_AddRef(ID3D12Heap *iface)
{
    struct d3d12_heap *heap = impl_from_ID3D12Heap(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&heap->refcount);

    TRACE("%p increasing refcount to %u.\n", heap, refcount);

    VKD3D_ASSERT(!heap->is_private);

    return refcount;
}

static void d3d12_heap_destroy(struct d3d12_heap *heap)
{
    struct d3d12_device *device = heap->device;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    TRACE("Destroying heap %p.\n", heap);

    vkd3d_private_store_destroy(&heap->private_store);

    if (heap->map_ptr)
        VK_CALL(vkUnmapMemory(device->vk_device, heap->vk_memory));

    VK_CALL(vkFreeMemory(device->vk_device, heap->vk_memory, NULL));

    vkd3d_mutex_destroy(&heap->mutex);

    if (heap->is_private)
        device = NULL;

    vkd3d_free(heap);

    if (device)
        d3d12_device_release(device);
}

static ULONG STDMETHODCALLTYPE d3d12_heap_Release(ID3D12Heap *iface)
{
    struct d3d12_heap *heap = impl_from_ID3D12Heap(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&heap->refcount);

    TRACE("%p decreasing refcount to %u.\n", heap, refcount);

    /* A heap must not be destroyed until all contained resources are destroyed. */
    if (!refcount && !heap->resource_count)
        d3d12_heap_destroy(heap);

    return refcount;
}

static void d3d12_heap_resource_destroyed(struct d3d12_heap *heap)
{
    if (!vkd3d_atomic_decrement_u32(&heap->resource_count) && (!heap->refcount || heap->is_private))
        d3d12_heap_destroy(heap);
}

static HRESULT STDMETHODCALLTYPE d3d12_heap_GetPrivateData(ID3D12Heap *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_heap *heap = impl_from_ID3D12Heap(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&heap->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_heap_SetPrivateData(ID3D12Heap *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_heap *heap = impl_from_ID3D12Heap(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&heap->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_heap_SetPrivateDataInterface(ID3D12Heap *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_heap *heap = impl_from_ID3D12Heap(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&heap->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_heap_SetName(ID3D12Heap *iface, const WCHAR *name)
{
    struct d3d12_heap *heap = impl_from_ID3D12Heap(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, heap->device->wchar_size));

    return vkd3d_set_vk_object_name(heap->device, (uint64_t)heap->vk_memory,
            VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, name);
}

static HRESULT STDMETHODCALLTYPE d3d12_heap_GetDevice(ID3D12Heap *iface, REFIID iid, void **device)
{
    struct d3d12_heap *heap = impl_from_ID3D12Heap(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(heap->device, iid, device);
}

static D3D12_HEAP_DESC * STDMETHODCALLTYPE d3d12_heap_GetDesc(ID3D12Heap *iface,
        D3D12_HEAP_DESC *desc)
{
    struct d3d12_heap *heap = impl_from_ID3D12Heap(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = heap->desc;
    return desc;
}

static const struct ID3D12HeapVtbl d3d12_heap_vtbl =
{
    /* IUnknown methods */
    d3d12_heap_QueryInterface,
    d3d12_heap_AddRef,
    d3d12_heap_Release,
    /* ID3D12Object methods */
    d3d12_heap_GetPrivateData,
    d3d12_heap_SetPrivateData,
    d3d12_heap_SetPrivateDataInterface,
    d3d12_heap_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_heap_GetDevice,
    /* ID3D12Heap methods */
    d3d12_heap_GetDesc,
};

struct d3d12_heap *unsafe_impl_from_ID3D12Heap(ID3D12Heap *iface)
{
    if (!iface)
        return NULL;
    VKD3D_ASSERT(iface->lpVtbl == &d3d12_heap_vtbl);
    return impl_from_ID3D12Heap(iface);
}

static HRESULT validate_heap_desc(const D3D12_HEAP_DESC *desc, const struct d3d12_resource *resource)
{
    if (!resource && !desc->SizeInBytes)
    {
        WARN("Invalid size %"PRIu64".\n", desc->SizeInBytes);
        return E_INVALIDARG;
    }

    if (desc->Alignment != D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT
            && desc->Alignment != D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT)
    {
        WARN("Invalid alignment %"PRIu64".\n", desc->Alignment);
        return E_INVALIDARG;
    }

    if (!resource && desc->Flags & D3D12_HEAP_FLAG_ALLOW_DISPLAY)
    {
        WARN("D3D12_HEAP_FLAG_ALLOW_DISPLAY is only for committed resources.\n");
        return E_INVALIDARG;
    }

    return S_OK;
}

static VkMemoryPropertyFlags d3d12_heap_get_memory_property_flags(const struct d3d12_heap *heap)
{
    return heap->device->memory_properties.memoryTypes[heap->vk_memory_type].propertyFlags;
}

static HRESULT d3d12_heap_init(struct d3d12_heap *heap,
        struct d3d12_device *device, const D3D12_HEAP_DESC *desc, const struct d3d12_resource *resource)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkMemoryRequirements memory_requirements;
    VkDeviceSize vk_memory_size;
    VkResult vr;
    HRESULT hr;

    heap->ID3D12Heap_iface.lpVtbl = &d3d12_heap_vtbl;
    heap->refcount = 1;
    heap->resource_count = 0;

    heap->is_private = !!resource;

    heap->desc = *desc;

    heap->map_ptr = NULL;
    heap->map_count = 0;

    if (!heap->desc.Properties.CreationNodeMask)
        heap->desc.Properties.CreationNodeMask = 1;
    if (!heap->desc.Properties.VisibleNodeMask)
        heap->desc.Properties.VisibleNodeMask = 1;

    debug_ignored_node_mask(heap->desc.Properties.CreationNodeMask);
    debug_ignored_node_mask(heap->desc.Properties.VisibleNodeMask);

    if (!heap->desc.Alignment)
        heap->desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    if (FAILED(hr = validate_heap_desc(&heap->desc, resource)))
        return hr;

    vkd3d_mutex_init(&heap->mutex);

    if (FAILED(hr = vkd3d_private_store_init(&heap->private_store)))
    {
        vkd3d_mutex_destroy(&heap->mutex);
        return hr;
    }

    if (resource)
    {
        if (d3d12_resource_is_buffer(resource))
        {
            hr = vkd3d_allocate_buffer_memory(device, resource->u.vk_buffer,
                    &heap->desc.Properties, heap->desc.Flags,
                    &heap->vk_memory, &heap->vk_memory_type, &vk_memory_size);
        }
        else
        {
            hr = vkd3d_allocate_image_memory(device, resource->u.vk_image,
                    &heap->desc.Properties, heap->desc.Flags,
                    &heap->vk_memory, &heap->vk_memory_type, &vk_memory_size);
        }

        heap->desc.SizeInBytes = vk_memory_size;
    }
    else
    {
        memory_requirements.size = heap->desc.SizeInBytes;
        memory_requirements.alignment = heap->desc.Alignment;
        memory_requirements.memoryTypeBits = ~(uint32_t)0;

        hr = vkd3d_allocate_device_memory(device, &heap->desc.Properties,
                heap->desc.Flags, &memory_requirements, NULL,
                &heap->vk_memory, &heap->vk_memory_type);
    }
    if (FAILED(hr))
    {
        vkd3d_private_store_destroy(&heap->private_store);
        vkd3d_mutex_destroy(&heap->mutex);
        return hr;
    }

    heap->device = device;
    if (!heap->is_private)
        d3d12_device_add_ref(heap->device);
    else
        heap->resource_count = 1;

    if (d3d12_heap_get_memory_property_flags(heap) & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        if ((vr = VK_CALL(vkMapMemory(device->vk_device,
                heap->vk_memory, 0, VK_WHOLE_SIZE, 0, &heap->map_ptr))) < 0)
        {
            heap->map_ptr = NULL;
            ERR("Failed to map memory, vr %d.\n", vr);
            d3d12_heap_destroy(heap);
            return hresult_from_vk_result(hr);
        }
    }

    return S_OK;
}

HRESULT d3d12_heap_create(struct d3d12_device *device, const D3D12_HEAP_DESC *desc,
        const struct d3d12_resource *resource, ID3D12ProtectedResourceSession *protected_session,
        struct d3d12_heap **heap)
{
    struct d3d12_heap *object;
    HRESULT hr;

    if (protected_session)
        FIXME("Protected session is not supported.\n");

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_heap_init(object, device, desc, resource)))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created %s %p.\n", object->is_private ? "private heap" : "heap", object);

    *heap = object;

    return S_OK;
}

static VkImageType vk_image_type_from_d3d12_resource_dimension(D3D12_RESOURCE_DIMENSION dimension)
{
    switch (dimension)
    {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            return VK_IMAGE_TYPE_1D;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            return VK_IMAGE_TYPE_2D;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            return VK_IMAGE_TYPE_3D;
        default:
            ERR("Invalid resource dimension %#x.\n", dimension);
            return VK_IMAGE_TYPE_2D;
    }
}

VkSampleCountFlagBits vk_samples_from_sample_count(unsigned int sample_count)
{
    switch (sample_count)
    {
        case 1:
            return VK_SAMPLE_COUNT_1_BIT;
        case 2:
            return VK_SAMPLE_COUNT_2_BIT;
        case 4:
            return VK_SAMPLE_COUNT_4_BIT;
        case 8:
            return VK_SAMPLE_COUNT_8_BIT;
        case 16:
            return VK_SAMPLE_COUNT_16_BIT;
        case 32:
            return VK_SAMPLE_COUNT_32_BIT;
        case 64:
            return VK_SAMPLE_COUNT_64_BIT;
        default:
            return 0;
    }
}

VkSampleCountFlagBits vk_samples_from_dxgi_sample_desc(const DXGI_SAMPLE_DESC *desc)
{
    VkSampleCountFlagBits vk_samples;

    if ((vk_samples = vk_samples_from_sample_count(desc->Count)))
        return vk_samples;

    FIXME("Unhandled sample count %u.\n", desc->Count);
    return VK_SAMPLE_COUNT_1_BIT;
}

HRESULT vkd3d_create_buffer(struct d3d12_device *device,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags,
        const D3D12_RESOURCE_DESC1 *desc, VkBuffer *vk_buffer)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    const bool sparse_resource = !heap_properties;
    VkBufferCreateInfo buffer_info;
    D3D12_HEAP_TYPE heap_type;
    VkResult vr;

    heap_type = heap_properties ? heap_properties->Type : D3D12_HEAP_TYPE_DEFAULT;

    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = NULL;
    buffer_info.flags = 0;
    buffer_info.size = desc->Width;

    if (sparse_resource)
    {
        buffer_info.flags |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
        if (device->vk_info.sparse_properties.residencyNonResidentStrict)
            buffer_info.flags |= VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
    }

    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
            | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
            | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    if (device->vk_info.EXT_conditional_rendering)
        buffer_info.usage |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;

    if (heap_type == D3D12_HEAP_TYPE_DEFAULT && device->vk_info.EXT_transform_feedback)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT
                | VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT;
    }

    if (heap_type == D3D12_HEAP_TYPE_UPLOAD)
        buffer_info.usage &= ~VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    else if (heap_type == D3D12_HEAP_TYPE_READBACK)
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if (desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
        buffer_info.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    if (!(desc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
        buffer_info.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;

    /* Buffers always have properties of D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS. */
    if (desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
    {
        WARN("D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS cannot be set for buffers.\n");
        return E_INVALIDARG;
    }

    if (device->queue_family_count > 1)
    {
        buffer_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        buffer_info.queueFamilyIndexCount = device->queue_family_count;
        buffer_info.pQueueFamilyIndices = device->queue_family_indices;
    }
    else
    {
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_info.queueFamilyIndexCount = 0;
        buffer_info.pQueueFamilyIndices = NULL;
    }

    if (desc->Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
        FIXME("Unsupported resource flags %#x.\n", desc->Flags);

    if ((vr = VK_CALL(vkCreateBuffer(device->vk_device, &buffer_info, NULL, vk_buffer))) < 0)
    {
        WARN("Failed to create Vulkan buffer, vr %d.\n", vr);
        *vk_buffer = VK_NULL_HANDLE;
    }

    return hresult_from_vk_result(vr);
}

static unsigned int max_miplevel_count(const D3D12_RESOURCE_DESC1 *desc)
{
    unsigned int size = max(desc->Width, desc->Height);
    size = max(size, d3d12_resource_desc_get_depth(desc, 0));
    return vkd3d_log2i(size) + 1;
}

static const struct vkd3d_format_compatibility_list *vkd3d_get_format_compatibility_list(
        const struct d3d12_device *device, DXGI_FORMAT dxgi_format)
{
    unsigned int i;

    for (i = 0; i < device->format_compatibility_list_count; ++i)
    {
        if (device->format_compatibility_lists[i].typeless_format == dxgi_format)
            return &device->format_compatibility_lists[i];
    }

    return NULL;
}

static bool vkd3d_is_linear_tiling_supported(const struct d3d12_device *device, VkImageCreateInfo *image_info)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkImageFormatProperties properties;
    VkResult vr;

    if ((vr = VK_CALL(vkGetPhysicalDeviceImageFormatProperties(device->vk_physical_device, image_info->format,
            image_info->imageType, VK_IMAGE_TILING_LINEAR, image_info->usage, image_info->flags, &properties))) < 0)
    {
        if (vr != VK_ERROR_FORMAT_NOT_SUPPORTED)
            WARN("Failed to get device image format properties, vr %d.\n", vr);

        return false;
    }

    return image_info->extent.depth <= properties.maxExtent.depth
            && image_info->mipLevels <= properties.maxMipLevels
            && image_info->arrayLayers <= properties.maxArrayLayers
            && (image_info->samples & properties.sampleCounts);
}

static HRESULT vkd3d_create_image(struct d3d12_device *device,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags,
        const D3D12_RESOURCE_DESC1 *desc, struct d3d12_resource *resource, VkImage *vk_image)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    const struct vkd3d_format_compatibility_list *compat_list;
    const bool sparse_resource = !heap_properties;
    VkImageFormatListCreateInfoKHR format_list;
    const struct vkd3d_format *format;
    VkImageCreateInfo image_info;
    uint32_t count;
    VkResult vr;

    if (resource)
    {
        format = resource->format;
    }
    else if (!(format = vkd3d_format_from_d3d12_resource_desc(device, desc, 0)))
    {
        WARN("Invalid DXGI format %#x.\n", desc->Format);
        return E_INVALIDARG;
    }

    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.flags = 0;
    if (desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
        /* Format compatibility rules are more relaxed for UAVs. */
        if (format->type != VKD3D_FORMAT_TYPE_UINT)
            image_info.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }
    else if (!(desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) && format->type == VKD3D_FORMAT_TYPE_TYPELESS)
    {
        image_info.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

        if ((compat_list = vkd3d_get_format_compatibility_list(device, desc->Format)))
        {
            format_list.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
            format_list.pNext = NULL;
            format_list.viewFormatCount = compat_list->format_count;
            format_list.pViewFormats = compat_list->vk_formats;

            image_info.pNext = &format_list;
        }
    }
    if (desc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D
            && desc->Width == desc->Height && desc->DepthOrArraySize >= 6
            && desc->SampleDesc.Count == 1)
        image_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    if (desc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
        image_info.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;

    if (sparse_resource)
    {
        image_info.flags |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
        if (device->vk_info.sparse_properties.residencyNonResidentStrict)
            image_info.flags |= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
    }

    image_info.imageType = vk_image_type_from_d3d12_resource_dimension(desc->Dimension);
    image_info.format = format->vk_format;
    image_info.extent.width = desc->Width;
    image_info.extent.height = desc->Height;

    if (desc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        image_info.extent.depth = desc->DepthOrArraySize;
        image_info.arrayLayers = 1;
    }
    else
    {
        image_info.extent.depth = 1;
        image_info.arrayLayers = desc->DepthOrArraySize;
    }

    image_info.mipLevels = min(desc->MipLevels, max_miplevel_count(desc));
    image_info.samples = vk_samples_from_dxgi_sample_desc(&desc->SampleDesc);

    if (sparse_resource)
    {
        if (desc->Layout != D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE)
        {
            WARN("D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE must be used for reserved texture.\n");
            return E_INVALIDARG;
        }

        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    }
    else if (desc->Layout == D3D12_TEXTURE_LAYOUT_UNKNOWN)
    {
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    }
    else if (desc->Layout == D3D12_TEXTURE_LAYOUT_ROW_MAJOR)
    {
        image_info.tiling = VK_IMAGE_TILING_LINEAR;
    }
    else
    {
        FIXME("Unsupported layout %#x.\n", desc->Layout);
        return E_NOTIMPL;
    }

    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
        image_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
        image_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
        image_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (!(desc->Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
        image_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    if ((desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) && device->queue_family_count > 1)
    {
        TRACE("Creating image with VK_SHARING_MODE_CONCURRENT.\n");
        image_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        image_info.queueFamilyIndexCount = device->queue_family_count;
        image_info.pQueueFamilyIndices = device->queue_family_indices;
    }
    else
    {
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.queueFamilyIndexCount = 0;
        image_info.pQueueFamilyIndices = NULL;
    }

    if (heap_properties && is_cpu_accessible_heap(heap_properties))
    {
        image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

        if (vkd3d_is_linear_tiling_supported(device, &image_info))
        {
            /* Required for ReadFromSubresource(). */
            WARN("Forcing VK_IMAGE_TILING_LINEAR for CPU readable texture.\n");
            image_info.tiling = VK_IMAGE_TILING_LINEAR;
        }
    }
    else
    {
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    if (resource && image_info.tiling == VK_IMAGE_TILING_LINEAR)
        resource->flags |= VKD3D_RESOURCE_LINEAR_TILING;

    if (sparse_resource)
    {
        count = 0;
        VK_CALL(vkGetPhysicalDeviceSparseImageFormatProperties(device->vk_physical_device, image_info.format,
                image_info.imageType, image_info.samples, image_info.usage, image_info.tiling, &count, NULL));

        if (!count)
        {
            FIXME("Sparse images are not supported with format %u, type %u, samples %u, usage %#x.\n",
                    image_info.format, image_info.imageType, image_info.samples, image_info.usage);
            return E_INVALIDARG;
        }
    }

    if ((vr = VK_CALL(vkCreateImage(device->vk_device, &image_info, NULL, vk_image))) < 0)
        WARN("Failed to create Vulkan image, vr %d.\n", vr);

    return hresult_from_vk_result(vr);
}

HRESULT vkd3d_get_image_allocation_info(struct d3d12_device *device,
        const D3D12_RESOURCE_DESC1 *desc, struct vkd3d_resource_allocation_info *allocation_info)
{
    static const D3D12_HEAP_PROPERTIES heap_properties = {D3D12_HEAP_TYPE_DEFAULT};
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    D3D12_RESOURCE_DESC1 validated_desc;
    VkMemoryRequirements requirements;
    VkImage vk_image;
    bool tiled;
    HRESULT hr;

    VKD3D_ASSERT(desc->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER);
    VKD3D_ASSERT(d3d12_resource_validate_desc(desc, device) == S_OK);

    if (!desc->MipLevels)
    {
        validated_desc = *desc;
        validated_desc.MipLevels = max_miplevel_count(desc);
        desc = &validated_desc;
    }

    tiled = desc->Layout == D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

    /* XXX: We have to create an image to get its memory requirements. */
    if (SUCCEEDED(hr = vkd3d_create_image(device, tiled ? NULL : &heap_properties, 0, desc, NULL, &vk_image)))
    {
        VK_CALL(vkGetImageMemoryRequirements(device->vk_device, vk_image, &requirements));
        VK_CALL(vkDestroyImage(device->vk_device, vk_image, NULL));

        allocation_info->size_in_bytes = requirements.size;
        allocation_info->alignment = requirements.alignment;
    }

    return hr;
}

static void d3d12_resource_tile_info_cleanup(struct d3d12_resource *resource)
{
    vkd3d_free(resource->tiles.subresources);
}

static void d3d12_resource_destroy(struct d3d12_resource *resource, struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    if (resource->flags & VKD3D_RESOURCE_EXTERNAL)
        return;

    if (resource->gpu_address)
        vkd3d_gpu_va_allocator_free(&device->gpu_va_allocator, resource->gpu_address);

    if (d3d12_resource_is_buffer(resource))
        VK_CALL(vkDestroyBuffer(device->vk_device, resource->u.vk_buffer, NULL));
    else
        VK_CALL(vkDestroyImage(device->vk_device, resource->u.vk_image, NULL));

    d3d12_resource_tile_info_cleanup(resource);

    if (resource->heap)
        d3d12_heap_resource_destroyed(resource->heap);
}

static ULONG d3d12_resource_incref(struct d3d12_resource *resource)
{
    unsigned int refcount = vkd3d_atomic_increment_u32(&resource->internal_refcount);

    TRACE("%p increasing refcount to %u.\n", resource, refcount);

    return refcount;
}

static ULONG d3d12_resource_decref(struct d3d12_resource *resource)
{
    unsigned int refcount = vkd3d_atomic_decrement_u32(&resource->internal_refcount);

    TRACE("%p decreasing refcount to %u.\n", resource, refcount);

    if (!refcount)
    {
        vkd3d_private_store_destroy(&resource->private_store);
        d3d12_resource_destroy(resource, resource->device);
        vkd3d_free(resource);
    }

    return refcount;
}

bool d3d12_resource_is_cpu_accessible(const struct d3d12_resource *resource)
{
    return resource->heap && is_cpu_accessible_heap(&resource->heap->desc.Properties);
}

static bool d3d12_resource_validate_box(const struct d3d12_resource *resource,
        unsigned int sub_resource_idx, const D3D12_BOX *box)
{
    unsigned int mip_level = sub_resource_idx % resource->desc.MipLevels;
    const struct vkd3d_format *vkd3d_format;
    uint32_t width_mask, height_mask;
    uint64_t width, height, depth;

    width = d3d12_resource_desc_get_width(&resource->desc, mip_level);
    height = d3d12_resource_desc_get_height(&resource->desc, mip_level);
    depth = d3d12_resource_desc_get_depth(&resource->desc, mip_level);

    vkd3d_format = resource->format;
    VKD3D_ASSERT(vkd3d_format);
    width_mask = vkd3d_format->block_width - 1;
    height_mask = vkd3d_format->block_height - 1;

    return box->left <= width && box->right <= width
            && box->top <= height && box->bottom <= height
            && box->front <= depth && box->back <= depth
            && !(box->left & width_mask)
            && !(box->right & width_mask)
            && !(box->top & height_mask)
            && !(box->bottom & height_mask);
}

static void d3d12_resource_get_level_box(const struct d3d12_resource *resource,
        unsigned int level, D3D12_BOX *box)
{
    box->left = 0;
    box->top = 0;
    box->front = 0;
    box->right = d3d12_resource_desc_get_width(&resource->desc, level);
    box->bottom = d3d12_resource_desc_get_height(&resource->desc, level);
    box->back = d3d12_resource_desc_get_depth(&resource->desc, level);
}

static void compute_image_subresource_size_in_tiles(const VkExtent3D *tile_extent,
        const struct D3D12_RESOURCE_DESC1 *desc, unsigned int miplevel_idx,
        struct vkd3d_tiled_region_extent *size)
{
    unsigned int width, height, depth;

    width = d3d12_resource_desc_get_width(desc, miplevel_idx);
    height = d3d12_resource_desc_get_height(desc, miplevel_idx);
    depth = d3d12_resource_desc_get_depth(desc, miplevel_idx);
    size->width = (width + tile_extent->width - 1) / tile_extent->width;
    size->height = (height + tile_extent->height - 1) / tile_extent->height;
    size->depth = (depth + tile_extent->depth - 1) / tile_extent->depth;
}

void d3d12_resource_get_tiling(struct d3d12_device *device, const struct d3d12_resource *resource,
        UINT *total_tile_count, D3D12_PACKED_MIP_INFO *packed_mip_info, D3D12_TILE_SHAPE *standard_tile_shape,
        UINT *subresource_tiling_count, UINT first_subresource_tiling,
        D3D12_SUBRESOURCE_TILING *subresource_tilings)
{
    unsigned int i, subresource, subresource_count, miplevel_idx, count;
    const struct vkd3d_subresource_tile_info *tile_info;
    const VkExtent3D *tile_extent;

    tile_extent = &resource->tiles.tile_extent;

    if (packed_mip_info)
    {
        packed_mip_info->NumStandardMips = resource->tiles.standard_mip_count;
        packed_mip_info->NumPackedMips = resource->desc.MipLevels - packed_mip_info->NumStandardMips;
        packed_mip_info->NumTilesForPackedMips = !!resource->tiles.packed_mip_tile_count; /* non-zero dummy value */
        packed_mip_info->StartTileIndexInOverallResource = packed_mip_info->NumPackedMips
                ? resource->tiles.subresources[resource->tiles.standard_mip_count].offset : 0;
    }

    if (standard_tile_shape)
    {
        /* D3D12 docs say tile shape is cleared to zero if there is no standard mip, but drivers don't to do this. */
        standard_tile_shape->WidthInTexels = tile_extent->width;
        standard_tile_shape->HeightInTexels = tile_extent->height;
        standard_tile_shape->DepthInTexels = tile_extent->depth;
    }

    if (total_tile_count)
        *total_tile_count = resource->tiles.total_count;

    if (!subresource_tiling_count)
        return;

    subresource_count = resource->tiles.subresource_count;

    count = subresource_count - min(first_subresource_tiling, subresource_count);
    count = min(count, *subresource_tiling_count);

    for (i = 0; i < count; ++i)
    {
        subresource = i + first_subresource_tiling;
        miplevel_idx = subresource % resource->desc.MipLevels;
        if (miplevel_idx >= resource->tiles.standard_mip_count)
        {
            memset(&subresource_tilings[i], 0, sizeof(subresource_tilings[i]));
            subresource_tilings[i].StartTileIndexInOverallResource = D3D12_PACKED_TILE;
            continue;
        }

        tile_info = &resource->tiles.subresources[subresource];
        subresource_tilings[i].StartTileIndexInOverallResource = tile_info->offset;
        subresource_tilings[i].WidthInTiles = tile_info->extent.width;
        subresource_tilings[i].HeightInTiles = tile_info->extent.height;
        subresource_tilings[i].DepthInTiles = tile_info->extent.depth;
    }
    *subresource_tiling_count = i;
}

static bool d3d12_resource_init_tiles(struct d3d12_resource *resource, struct d3d12_device *device)
{
    unsigned int i, start_idx, subresource_count, tile_count, miplevel_idx;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkSparseImageMemoryRequirements *sparse_requirements_array;
    VkSparseImageMemoryRequirements sparse_requirements = {0};
    struct vkd3d_subresource_tile_info *tile_info;
    VkMemoryRequirements requirements;
    const VkExtent3D *tile_extent;
    uint32_t requirement_count;

    subresource_count = d3d12_resource_desc_get_sub_resource_count(&resource->desc);

    if (!(resource->tiles.subresources = vkd3d_calloc(subresource_count, sizeof(*resource->tiles.subresources))))
    {
        ERR("Failed to allocate subresource info array.\n");
        return false;
    }

    if (d3d12_resource_is_buffer(resource))
    {
        VKD3D_ASSERT(subresource_count == 1);

        VK_CALL(vkGetBufferMemoryRequirements(device->vk_device, resource->u.vk_buffer, &requirements));
        if (requirements.alignment > D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES)
            FIXME("Vulkan device tile size is greater than the standard D3D12 tile size.\n");

        tile_info = &resource->tiles.subresources[0];
        tile_info->offset = 0;
        tile_info->extent.width = align(resource->desc.Width, D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES)
                / D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        tile_info->extent.height = 1;
        tile_info->extent.depth = 1;
        tile_info->count = tile_info->extent.width;

        resource->tiles.tile_extent.width = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        resource->tiles.tile_extent.height = 1;
        resource->tiles.tile_extent.depth = 1;
        resource->tiles.total_count = tile_info->extent.width;
        resource->tiles.subresource_count = 1;
        resource->tiles.standard_mip_count = 1;
        resource->tiles.packed_mip_tile_count = 0;
    }
    else
    {
        VK_CALL(vkGetImageMemoryRequirements(device->vk_device, resource->u.vk_image, &requirements));
        if (requirements.alignment > D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES)
            FIXME("Vulkan device tile size is greater than the standard D3D12 tile size.\n");

        requirement_count = 0;
        VK_CALL(vkGetImageSparseMemoryRequirements(device->vk_device, resource->u.vk_image, &requirement_count, NULL));
        if (!(sparse_requirements_array = vkd3d_calloc(requirement_count, sizeof(*sparse_requirements_array))))
        {
            ERR("Failed to allocate sparse requirements array.\n");
            return false;
        }
        VK_CALL(vkGetImageSparseMemoryRequirements(device->vk_device, resource->u.vk_image,
                &requirement_count, sparse_requirements_array));

        for (i = 0; i < requirement_count; ++i)
        {
            if (sparse_requirements_array[i].formatProperties.aspectMask & resource->format->vk_aspect_mask)
            {
                if (sparse_requirements.formatProperties.aspectMask)
                {
                    WARN("Ignoring properties for aspect mask %#x.\n",
                            sparse_requirements_array[i].formatProperties.aspectMask);
                }
                else
                {
                    sparse_requirements = sparse_requirements_array[i];
                }
            }
        }
        vkd3d_free(sparse_requirements_array);
        if (!sparse_requirements.formatProperties.aspectMask)
        {
            WARN("Failed to get sparse requirements.\n");
            return false;
        }

        resource->tiles.tile_extent = sparse_requirements.formatProperties.imageGranularity;
        resource->tiles.subresource_count = subresource_count;
        resource->tiles.standard_mip_count = sparse_requirements.imageMipTailSize
                ? sparse_requirements.imageMipTailFirstLod : resource->desc.MipLevels;
        resource->tiles.packed_mip_tile_count = (resource->tiles.standard_mip_count < resource->desc.MipLevels)
                ? sparse_requirements.imageMipTailSize / requirements.alignment : 0;

        for (i = 0, start_idx = 0; i < subresource_count; ++i)
        {
            miplevel_idx = i % resource->desc.MipLevels;

            tile_extent = &sparse_requirements.formatProperties.imageGranularity;
            tile_info = &resource->tiles.subresources[i];
            compute_image_subresource_size_in_tiles(tile_extent, &resource->desc, miplevel_idx, &tile_info->extent);
            tile_info->offset = start_idx;
            tile_info->count = 0;

            if (miplevel_idx < resource->tiles.standard_mip_count)
            {
                tile_count = tile_info->extent.width * tile_info->extent.height * tile_info->extent.depth;
                start_idx += tile_count;
                tile_info->count = tile_count;
            }
            else if (miplevel_idx == resource->tiles.standard_mip_count)
            {
                tile_info->count = 1; /* Non-zero dummy value */
                start_idx += 1;
            }
        }
        resource->tiles.total_count = start_idx;
    }

    return true;
}

/* ID3D12Resource */
static HRESULT STDMETHODCALLTYPE d3d12_resource_QueryInterface(ID3D12Resource2 *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D12Resource2)
            || IsEqualGUID(riid, &IID_ID3D12Resource1)
            || IsEqualGUID(riid, &IID_ID3D12Resource)
            || IsEqualGUID(riid, &IID_ID3D12Pageable)
            || IsEqualGUID(riid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(riid, &IID_ID3D12Object)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D12Resource2_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_resource_AddRef(ID3D12Resource2 *iface)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&resource->refcount);

    TRACE("%p increasing refcount to %u.\n", resource, refcount);

    if (refcount == 1)
    {
        struct d3d12_device *device = resource->device;

        d3d12_device_add_ref(device);
        d3d12_resource_incref(resource);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d12_resource_Release(ID3D12Resource2 *iface)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&resource->refcount);

    TRACE("%p decreasing refcount to %u.\n", resource, refcount);

    if (!refcount)
    {
        struct d3d12_device *device = resource->device;

        d3d12_resource_decref(resource);

        d3d12_device_release(device);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_GetPrivateData(ID3D12Resource2 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&resource->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_SetPrivateData(ID3D12Resource2 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&resource->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_SetPrivateDataInterface(ID3D12Resource2 *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&resource->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_SetName(ID3D12Resource2 *iface, const WCHAR *name)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);
    HRESULT hr;

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, resource->device->wchar_size));

    if (resource->flags & VKD3D_RESOURCE_DEDICATED_HEAP)
    {
        if (FAILED(hr = d3d12_heap_SetName(&resource->heap->ID3D12Heap_iface, name)))
            return hr;
    }

    if (d3d12_resource_is_buffer(resource))
        return vkd3d_set_vk_object_name(resource->device, (uint64_t)resource->u.vk_buffer,
                VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name);
    else
        return vkd3d_set_vk_object_name(resource->device, (uint64_t)resource->u.vk_image,
                VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, name);
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_GetDevice(ID3D12Resource2 *iface, REFIID iid, void **device)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(resource->device, iid, device);
}

static void *d3d12_resource_get_map_ptr(struct d3d12_resource *resource)
{
    VKD3D_ASSERT(resource->heap->map_ptr);
    return (uint8_t *)resource->heap->map_ptr + resource->heap_offset;
}

static void d3d12_resource_get_vk_range(struct d3d12_resource *resource,
        uint64_t offset, uint64_t size, VkMappedMemoryRange *vk_range)
{
    vk_range->sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    vk_range->pNext = NULL;
    vk_range->memory = resource->heap->vk_memory;
    vk_range->offset = resource->heap_offset + offset;
    vk_range->size = size;
}

static void d3d12_resource_invalidate(struct d3d12_resource *resource, uint64_t offset, uint64_t size)
{
    const struct vkd3d_vk_device_procs *vk_procs = &resource->device->vk_procs;
    VkMappedMemoryRange vk_range;
    VkResult vr;

    if (d3d12_heap_get_memory_property_flags(resource->heap) & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        return;

    d3d12_resource_get_vk_range(resource, offset, size, &vk_range);
    if ((vr = VK_CALL(vkInvalidateMappedMemoryRanges(resource->device->vk_device, 1, &vk_range))) < 0)
        ERR("Failed to invalidate memory, vr %d.\n", vr);
}

static void d3d12_resource_flush(struct d3d12_resource *resource, uint64_t offset, uint64_t size)
{
    const struct vkd3d_vk_device_procs *vk_procs = &resource->device->vk_procs;
    VkMappedMemoryRange vk_range;
    VkResult vr;

    if (d3d12_heap_get_memory_property_flags(resource->heap) & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        return;

    d3d12_resource_get_vk_range(resource, offset, size, &vk_range);
    if ((vr = VK_CALL(vkFlushMappedMemoryRanges(resource->device->vk_device, 1, &vk_range))) < 0)
        ERR("Failed to flush memory, vr %d.\n", vr);
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_Map(ID3D12Resource2 *iface, UINT sub_resource,
        const D3D12_RANGE *read_range, void **data)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);
    unsigned int sub_resource_count;

    TRACE("iface %p, sub_resource %u, read_range %p, data %p.\n",
            iface, sub_resource, read_range, data);

    if (!d3d12_resource_is_cpu_accessible(resource))
    {
        WARN("Resource is not CPU accessible.\n");
        return E_INVALIDARG;
    }

    sub_resource_count = d3d12_resource_desc_get_sub_resource_count(&resource->desc);
    if (sub_resource >= sub_resource_count)
    {
        WARN("Sub-resource index %u is out of range (%u sub-resources).\n", sub_resource, sub_resource_count);
        return E_INVALIDARG;
    }

    if (d3d12_resource_is_texture(resource))
    {
        /* Textures seem to be mappable only on UMA adapters. */
        FIXME("Not implemented for textures.\n");
        return E_INVALIDARG;
    }

    if (!resource->heap)
    {
        FIXME("Not implemented for this resource type.\n");
        return E_NOTIMPL;
    }

    if (data)
    {
        *data = d3d12_resource_get_map_ptr(resource);
        TRACE("Returning pointer %p.\n", *data);
    }

    if (!read_range)
        d3d12_resource_invalidate(resource, 0, resource->desc.Width);
    else if (read_range->End > read_range->Begin)
        d3d12_resource_invalidate(resource, read_range->Begin, read_range->End - read_range->Begin);

    return S_OK;
}

static void STDMETHODCALLTYPE d3d12_resource_Unmap(ID3D12Resource2 *iface, UINT sub_resource,
        const D3D12_RANGE *written_range)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);
    unsigned int sub_resource_count;

    TRACE("iface %p, sub_resource %u, written_range %p.\n",
            iface, sub_resource, written_range);

    sub_resource_count = d3d12_resource_desc_get_sub_resource_count(&resource->desc);
    if (sub_resource >= sub_resource_count)
    {
        WARN("Sub-resource index %u is out of range (%u sub-resources).\n", sub_resource, sub_resource_count);
        return;
    }

    if (!written_range)
        d3d12_resource_flush(resource, 0, resource->desc.Width);
    else if (written_range->End > written_range->Begin)
        d3d12_resource_flush(resource, written_range->Begin, written_range->End - written_range->Begin);
}

static D3D12_RESOURCE_DESC * STDMETHODCALLTYPE d3d12_resource_GetDesc(ID3D12Resource2 *iface,
        D3D12_RESOURCE_DESC *resource_desc)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);

    TRACE("iface %p, resource_desc %p.\n", iface, resource_desc);

    memcpy(resource_desc, &resource->desc, sizeof(*resource_desc));
    return resource_desc;
}

static D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE d3d12_resource_GetGPUVirtualAddress(ID3D12Resource2 *iface)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);

    TRACE("iface %p.\n", iface);

    return resource->gpu_address;
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_WriteToSubresource(ID3D12Resource2 *iface,
        UINT dst_sub_resource, const D3D12_BOX *dst_box, const void *src_data,
        UINT src_row_pitch, UINT src_slice_pitch)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);
    const struct vkd3d_vk_device_procs *vk_procs;
    VkImageSubresource vk_sub_resource;
    const struct vkd3d_format *format;
    VkSubresourceLayout vk_layout;
    uint64_t dst_offset, dst_size;
    struct d3d12_device *device;
    uint8_t *dst_data;
    D3D12_BOX box;

    TRACE("iface %p, src_data %p, src_row_pitch %u, src_slice_pitch %u, "
            "dst_sub_resource %u, dst_box %s.\n",
            iface, src_data, src_row_pitch, src_slice_pitch, dst_sub_resource, debug_d3d12_box(dst_box));

    if (d3d12_resource_is_buffer(resource))
    {
        WARN("Buffers are not supported.\n");
        return E_INVALIDARG;
    }

    device = resource->device;
    vk_procs = &device->vk_procs;

    format = resource->format;
    if (format->vk_aspect_mask != VK_IMAGE_ASPECT_COLOR_BIT)
    {
        FIXME("Not supported for format %#x.\n", format->dxgi_format);
        return E_NOTIMPL;
    }

    vk_sub_resource.arrayLayer = dst_sub_resource / resource->desc.MipLevels;
    vk_sub_resource.mipLevel = dst_sub_resource % resource->desc.MipLevels;
    vk_sub_resource.aspectMask = format->vk_aspect_mask;

    if (!dst_box)
    {
        d3d12_resource_get_level_box(resource, vk_sub_resource.mipLevel, &box);
        dst_box = &box;
    }
    else if (!d3d12_resource_validate_box(resource, dst_sub_resource, dst_box))
    {
        WARN("Invalid box %s.\n", debug_d3d12_box(dst_box));
        return E_INVALIDARG;
    }

    if (d3d12_box_is_empty(dst_box))
    {
        WARN("Empty box %s.\n", debug_d3d12_box(dst_box));
        return S_OK;
    }

    if (!d3d12_resource_is_cpu_accessible(resource))
    {
        FIXME_ONCE("Not implemented for this resource type.\n");
        return E_NOTIMPL;
    }
    if (!(resource->flags & VKD3D_RESOURCE_LINEAR_TILING))
    {
        FIXME_ONCE("Not implemented for image tiling other than VK_IMAGE_TILING_LINEAR.\n");
        return E_NOTIMPL;
    }

    VK_CALL(vkGetImageSubresourceLayout(device->vk_device, resource->u.vk_image, &vk_sub_resource, &vk_layout));
    TRACE("Offset %#"PRIx64", size %#"PRIx64", row pitch %#"PRIx64", depth pitch %#"PRIx64".\n",
            vk_layout.offset, vk_layout.size, vk_layout.rowPitch, vk_layout.depthPitch);

    dst_data = d3d12_resource_get_map_ptr(resource);
    dst_offset = vk_layout.offset + vkd3d_format_get_data_offset(format, vk_layout.rowPitch,
            vk_layout.depthPitch, dst_box->left, dst_box->top, dst_box->front);
    dst_size = vk_layout.offset + vkd3d_format_get_data_offset(format, vk_layout.rowPitch,
            vk_layout.depthPitch, dst_box->right, dst_box->bottom - 1, dst_box->back - 1) - dst_offset;

    vkd3d_format_copy_data(format, src_data, src_row_pitch, src_slice_pitch,
            dst_data + dst_offset, vk_layout.rowPitch, vk_layout.depthPitch, dst_box->right - dst_box->left,
            dst_box->bottom - dst_box->top, dst_box->back - dst_box->front);

    d3d12_resource_flush(resource, dst_offset, dst_size);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_ReadFromSubresource(ID3D12Resource2 *iface,
        void *dst_data, UINT dst_row_pitch, UINT dst_slice_pitch,
        UINT src_sub_resource, const D3D12_BOX *src_box)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);
    const struct vkd3d_vk_device_procs *vk_procs;
    VkImageSubresource vk_sub_resource;
    const struct vkd3d_format *format;
    VkSubresourceLayout vk_layout;
    uint64_t src_offset, src_size;
    struct d3d12_device *device;
    uint8_t *src_data;
    D3D12_BOX box;

    TRACE("iface %p, dst_data %p, dst_row_pitch %u, dst_slice_pitch %u, "
            "src_sub_resource %u, src_box %s.\n",
            iface, dst_data, dst_row_pitch, dst_slice_pitch, src_sub_resource, debug_d3d12_box(src_box));

    if (d3d12_resource_is_buffer(resource))
    {
        WARN("Buffers are not supported.\n");
        return E_INVALIDARG;
    }

    device = resource->device;
    vk_procs = &device->vk_procs;

    format = resource->format;
    if (format->vk_aspect_mask != VK_IMAGE_ASPECT_COLOR_BIT)
    {
        FIXME("Not supported for format %#x.\n", format->dxgi_format);
        return E_NOTIMPL;
    }

    vk_sub_resource.arrayLayer = src_sub_resource / resource->desc.MipLevels;
    vk_sub_resource.mipLevel = src_sub_resource % resource->desc.MipLevels;
    vk_sub_resource.aspectMask = format->vk_aspect_mask;

    if (!src_box)
    {
        d3d12_resource_get_level_box(resource, vk_sub_resource.mipLevel, &box);
        src_box = &box;
    }
    else if (!d3d12_resource_validate_box(resource, src_sub_resource, src_box))
    {
        WARN("Invalid box %s.\n", debug_d3d12_box(src_box));
        return E_INVALIDARG;
    }

    if (d3d12_box_is_empty(src_box))
    {
        WARN("Empty box %s.\n", debug_d3d12_box(src_box));
        return S_OK;
    }

    if (!d3d12_resource_is_cpu_accessible(resource))
    {
        FIXME_ONCE("Not implemented for this resource type.\n");
        return E_NOTIMPL;
    }
    if (!(resource->flags & VKD3D_RESOURCE_LINEAR_TILING))
    {
        FIXME_ONCE("Not implemented for image tiling other than VK_IMAGE_TILING_LINEAR.\n");
        return E_NOTIMPL;
    }

    VK_CALL(vkGetImageSubresourceLayout(device->vk_device, resource->u.vk_image, &vk_sub_resource, &vk_layout));
    TRACE("Offset %#"PRIx64", size %#"PRIx64", row pitch %#"PRIx64", depth pitch %#"PRIx64".\n",
            vk_layout.offset, vk_layout.size, vk_layout.rowPitch, vk_layout.depthPitch);

    src_data = d3d12_resource_get_map_ptr(resource);
    src_offset = vk_layout.offset + vkd3d_format_get_data_offset(format, vk_layout.rowPitch,
            vk_layout.depthPitch, src_box->left, src_box->top, src_box->front);
    src_size = vk_layout.offset + vkd3d_format_get_data_offset(format, vk_layout.rowPitch,
            vk_layout.depthPitch, src_box->right, src_box->bottom - 1, src_box->back - 1) - src_offset;

    d3d12_resource_invalidate(resource, src_offset, src_size);

    vkd3d_format_copy_data(format, src_data + src_offset, vk_layout.rowPitch, vk_layout.depthPitch,
            dst_data, dst_row_pitch, dst_slice_pitch, src_box->right - src_box->left,
            src_box->bottom - src_box->top, src_box->back - src_box->front);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_GetHeapProperties(ID3D12Resource2 *iface,
        D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS *flags)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);
    struct d3d12_heap *heap;

    TRACE("iface %p, heap_properties %p, flags %p.\n",
            iface, heap_properties, flags);

    if (resource->flags & VKD3D_RESOURCE_EXTERNAL)
    {
        if (heap_properties)
        {
            memset(heap_properties, 0, sizeof(*heap_properties));
            heap_properties->Type = D3D12_HEAP_TYPE_DEFAULT;
            heap_properties->CreationNodeMask = 1;
            heap_properties->VisibleNodeMask = 1;
        }
        if (flags)
            *flags = D3D12_HEAP_FLAG_NONE;
        return S_OK;
    }

    if (!(heap = resource->heap))
    {
        WARN("Cannot get heap properties for reserved resources.\n");
        return E_INVALIDARG;
    }

    if (heap_properties)
        *heap_properties = heap->desc.Properties;
    if (flags)
        *flags = heap->desc.Flags;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_resource_GetProtectedResourceSession(ID3D12Resource2 *iface,
        REFIID iid, void **session)
{
    FIXME("iface %p, iid %s, session %p stub!\n", iface, debugstr_guid(iid), session);

    return DXGI_ERROR_NOT_FOUND;
}

static D3D12_RESOURCE_DESC1 * STDMETHODCALLTYPE d3d12_resource_GetDesc1(ID3D12Resource2 *iface,
        D3D12_RESOURCE_DESC1 *resource_desc)
{
    struct d3d12_resource *resource = impl_from_ID3D12Resource2(iface);

    TRACE("iface %p, resource_desc %p.\n", iface, resource_desc);

    *resource_desc = resource->desc;
    return resource_desc;
}

static const struct ID3D12Resource2Vtbl d3d12_resource_vtbl =
{
    /* IUnknown methods */
    d3d12_resource_QueryInterface,
    d3d12_resource_AddRef,
    d3d12_resource_Release,
    /* ID3D12Object methods */
    d3d12_resource_GetPrivateData,
    d3d12_resource_SetPrivateData,
    d3d12_resource_SetPrivateDataInterface,
    d3d12_resource_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_resource_GetDevice,
    /* ID3D12Resource methods */
    d3d12_resource_Map,
    d3d12_resource_Unmap,
    d3d12_resource_GetDesc,
    d3d12_resource_GetGPUVirtualAddress,
    d3d12_resource_WriteToSubresource,
    d3d12_resource_ReadFromSubresource,
    d3d12_resource_GetHeapProperties,
    /* ID3D12Resource1 methods */
    d3d12_resource_GetProtectedResourceSession,
    /* ID3D12Resource2 methods */
    d3d12_resource_GetDesc1,
};

struct d3d12_resource *unsafe_impl_from_ID3D12Resource(ID3D12Resource *iface)
{
    if (!iface)
        return NULL;
    VKD3D_ASSERT(iface->lpVtbl == (ID3D12ResourceVtbl *)&d3d12_resource_vtbl);
    return impl_from_ID3D12Resource(iface);
}

static void d3d12_validate_resource_flags(D3D12_RESOURCE_FLAGS flags)
{
    unsigned int unknown_flags = flags & ~(D3D12_RESOURCE_FLAG_NONE
            | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
            | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
            | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
            | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE
            | D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER
            | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);

    if (unknown_flags)
        FIXME("Unknown resource flags %#x.\n", unknown_flags);
    if (flags & D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER)
        FIXME("Ignoring D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER.\n");
}

static bool d3d12_resource_validate_texture_format(const D3D12_RESOURCE_DESC1 *desc,
        const struct vkd3d_format *format)
{
    if (desc->Format == DXGI_FORMAT_UNKNOWN)
    {
        WARN("DXGI_FORMAT_UNKNOWN is invalid for textures.\n");
        return false;
    }

    if (!vkd3d_format_is_compressed(format))
        return true;

    if (desc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D && format->block_height > 1)
    {
        WARN("1D texture with a format block height > 1.\n");
        return false;
    }

    return true;
}

static bool d3d12_resource_validate_texture_alignment(const D3D12_RESOURCE_DESC1 *desc,
        const struct vkd3d_format *format)
{
    uint64_t estimated_size;

    if (!desc->Alignment)
        return true;

    if (desc->Alignment != D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT
            && desc->Alignment != D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT
            && (desc->SampleDesc.Count == 1 || desc->Alignment != D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT))
    {
        WARN("Invalid resource alignment %#"PRIx64".\n", desc->Alignment);
        return false;
    }

    if (desc->Alignment < D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
    {
        /* Windows uses the slice size to determine small alignment eligibility. DepthOrArraySize is ignored. */
        estimated_size = desc->Width * desc->Height * format->byte_count * format->block_byte_count
                / (format->block_width * format->block_height);
        if (estimated_size > D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
        {
            WARN("Invalid resource alignment %#"PRIx64" (required %#x).\n",
                    desc->Alignment, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
            return false;
        }
    }

    /* The size check for MSAA textures with D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT is probably
     * not important. The 4MB requirement is no longer universal and Vulkan has no such requirement. */

    return true;
}

HRESULT d3d12_resource_validate_desc(const D3D12_RESOURCE_DESC1 *desc, struct d3d12_device *device)
{
    const D3D12_MIP_REGION *mip_region = &desc->SamplerFeedbackMipRegion;
    const struct vkd3d_format *format;

    switch (desc->Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            if (desc->MipLevels != 1)
            {
                WARN("Invalid miplevel count %u for buffer.\n", desc->MipLevels);
                return E_INVALIDARG;
            }

            if (desc->Format != DXGI_FORMAT_UNKNOWN || desc->Layout != D3D12_TEXTURE_LAYOUT_ROW_MAJOR
                    || desc->Height != 1 || desc->DepthOrArraySize != 1
                    || desc->SampleDesc.Count != 1 || desc->SampleDesc.Quality != 0
                    || (desc->Alignment != 0 && desc->Alignment != D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT))
            {
                WARN("Invalid parameters for a buffer resource.\n");
                return E_INVALIDARG;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (desc->Height != 1)
            {
                WARN("1D texture with a height of %u.\n", desc->Height);
                return E_INVALIDARG;
            }
            /* Fall through. */
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            if (!desc->SampleDesc.Count)
            {
                WARN("Invalid sample count 0.\n");
                return E_INVALIDARG;
            }
            if (desc->SampleDesc.Count > 1
                    && !(desc->Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)))
            {
                WARN("Sample count %u invalid without ALLOW_RENDER_TARGET or ALLOW_DEPTH_STENCIL.\n",
                        desc->SampleDesc.Count);
                return E_INVALIDARG;
            }

            if (!(format = vkd3d_format_from_d3d12_resource_desc(device, desc, 0)))
            {
                WARN("Invalid format %#x.\n", desc->Format);
                return E_INVALIDARG;
            }

            if (desc->Layout == D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE)
            {
                if (desc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D && !device->vk_info.sparse_residency_3d)
                {
                    WARN("The device does not support tiled 3D images.\n");
                    return E_INVALIDARG;
                }
                if (format->plane_count > 1)
                {
                    WARN("Invalid format %#x. D3D12 does not support multiplanar formats for tiled resources.\n",
                            format->dxgi_format);
                    return E_INVALIDARG;
                }
            }

            if (!d3d12_resource_validate_texture_format(desc, format)
                    || !d3d12_resource_validate_texture_alignment(desc, format))
                return E_INVALIDARG;
            break;

        default:
            WARN("Invalid resource dimension %#x.\n", desc->Dimension);
            return E_INVALIDARG;
    }

    d3d12_validate_resource_flags(desc->Flags);

    if (mip_region->Width && mip_region->Height && mip_region->Depth)
    {
        FIXME("Unhandled sampler feedback mip region size (%u, %u, %u).\n", mip_region->Width, mip_region->Height,
                mip_region->Depth);
    }

    return S_OK;
}

static bool d3d12_resource_validate_heap_properties(const struct d3d12_resource *resource,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_RESOURCE_STATES initial_state)
{
    if (heap_properties->Type == D3D12_HEAP_TYPE_UPLOAD
            || heap_properties->Type == D3D12_HEAP_TYPE_READBACK)
    {
        if (d3d12_resource_is_texture(resource))
        {
            WARN("Textures cannot be created on upload/readback heaps.\n");
            return false;
        }

        if (resource->desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
        {
            WARN("Render target and unordered access buffers cannot be created on upload/readback heaps.\n");
            return false;
        }
    }

    if (heap_properties->Type == D3D12_HEAP_TYPE_UPLOAD && initial_state != D3D12_RESOURCE_STATE_GENERIC_READ)
    {
        WARN("For D3D12_HEAP_TYPE_UPLOAD the state must be D3D12_RESOURCE_STATE_GENERIC_READ.\n");
        return false;
    }
    if (heap_properties->Type == D3D12_HEAP_TYPE_READBACK && initial_state != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        WARN("For D3D12_HEAP_TYPE_READBACK the state must be D3D12_RESOURCE_STATE_COPY_DEST.\n");
        return false;
    }

    return true;
}

static HRESULT d3d12_resource_init(struct d3d12_resource *resource, struct d3d12_device *device,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags,
        const D3D12_RESOURCE_DESC1 *desc, D3D12_RESOURCE_STATES initial_state,
        const D3D12_CLEAR_VALUE *optimized_clear_value)
{
    HRESULT hr;

    resource->ID3D12Resource2_iface.lpVtbl = &d3d12_resource_vtbl;
    resource->refcount = 1;
    resource->internal_refcount = 1;

    resource->desc = *desc;

    if (!heap_properties && !device->vk_info.sparse_binding)
    {
        WARN("The device does not support tiled images.\n");
        return E_INVALIDARG;
    }

    if (heap_properties && !d3d12_resource_validate_heap_properties(resource, heap_properties, initial_state))
        return E_INVALIDARG;

    if (!is_valid_resource_state(initial_state))
    {
        WARN("Invalid initial resource state %#x.\n", initial_state);
        return E_INVALIDARG;
    }
    if (initial_state == D3D12_RESOURCE_STATE_RENDER_TARGET && !(desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET))
    {
        WARN("Invalid initial resource state %#x for non-render-target.\n", initial_state);
        return E_INVALIDARG;
    }

    if (optimized_clear_value && d3d12_resource_is_buffer(resource))
    {
        WARN("Optimized clear value must be NULL for buffers.\n");
        return E_INVALIDARG;
    }

    if (optimized_clear_value)
        WARN("Ignoring optimized clear value.\n");

    resource->gpu_address = 0;
    resource->flags = 0;

    if (FAILED(hr = d3d12_resource_validate_desc(&resource->desc, device)))
        return hr;

    resource->format = vkd3d_format_from_d3d12_resource_desc(device, desc, 0);

    switch (desc->Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            if (FAILED(hr = vkd3d_create_buffer(device, heap_properties, heap_flags,
                    &resource->desc, &resource->u.vk_buffer)))
                return hr;
            if (!(resource->gpu_address = vkd3d_gpu_va_allocator_allocate(&device->gpu_va_allocator,
                    desc->Alignment ? desc->Alignment : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
                    desc->Width, resource)))
            {
                ERR("Failed to allocate GPU VA.\n");
                d3d12_resource_destroy(resource, device);
                return E_OUTOFMEMORY;
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            if (!resource->desc.MipLevels)
                resource->desc.MipLevels = max_miplevel_count(desc);
            resource->flags |= VKD3D_RESOURCE_INITIAL_STATE_TRANSITION;
            if (FAILED(hr = vkd3d_create_image(device, heap_properties, heap_flags,
                    &resource->desc, resource, &resource->u.vk_image)))
                return hr;
            break;

        default:
            WARN("Invalid resource dimension %#x.\n", resource->desc.Dimension);
            return E_INVALIDARG;
    }

    resource->map_count = 0;

    resource->initial_state = initial_state;

    resource->heap = NULL;
    resource->heap_offset = 0;

    memset(&resource->tiles, 0, sizeof(resource->tiles));

    if (FAILED(hr = vkd3d_private_store_init(&resource->private_store)))
    {
        d3d12_resource_destroy(resource, device);
        return hr;
    }

    d3d12_device_add_ref(resource->device = device);

    return S_OK;
}

static HRESULT d3d12_resource_create(struct d3d12_device *device,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags,
        const D3D12_RESOURCE_DESC1 *desc, D3D12_RESOURCE_STATES initial_state,
        const D3D12_CLEAR_VALUE *optimized_clear_value, struct d3d12_resource **resource)
{
    struct d3d12_resource *object;
    HRESULT hr;

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_resource_init(object, device, heap_properties, heap_flags,
            desc, initial_state, optimized_clear_value)))
    {
        vkd3d_free(object);
        return hr;
    }

    *resource = object;

    return hr;
}

static HRESULT vkd3d_allocate_resource_memory(
        struct d3d12_device *device, struct d3d12_resource *resource,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags)
{
    D3D12_HEAP_DESC heap_desc;
    HRESULT hr;

    heap_desc.SizeInBytes = 0;
    heap_desc.Properties = *heap_properties;
    heap_desc.Alignment = 0;
    heap_desc.Flags = heap_flags;
    if (SUCCEEDED(hr = d3d12_heap_create(device, &heap_desc, resource, NULL, &resource->heap)))
        resource->flags |= VKD3D_RESOURCE_DEDICATED_HEAP;
    return hr;
}

HRESULT d3d12_committed_resource_create(struct d3d12_device *device,
        const D3D12_HEAP_PROPERTIES *heap_properties, D3D12_HEAP_FLAGS heap_flags,
        const D3D12_RESOURCE_DESC1 *desc, D3D12_RESOURCE_STATES initial_state,
        const D3D12_CLEAR_VALUE *optimized_clear_value, ID3D12ProtectedResourceSession *protected_session,
        struct d3d12_resource **resource)
{
    struct d3d12_resource *object;
    HRESULT hr;

    if (!heap_properties)
    {
        WARN("Heap properties are NULL.\n");
        return E_INVALIDARG;
    }

    if (protected_session)
        FIXME("Protected session is not supported.\n");

    if (FAILED(hr = d3d12_resource_create(device, heap_properties, heap_flags,
            desc, initial_state, optimized_clear_value, &object)))
        return hr;

    if (FAILED(hr = vkd3d_allocate_resource_memory(device, object, heap_properties, heap_flags)))
    {
        d3d12_resource_Release(&object->ID3D12Resource2_iface);
        return hr;
    }

    TRACE("Created committed resource %p.\n", object);

    *resource = object;

    return S_OK;
}

static HRESULT vkd3d_bind_heap_memory(struct d3d12_device *device,
        struct d3d12_resource *resource, struct d3d12_heap *heap, uint64_t heap_offset)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkDevice vk_device = device->vk_device;
    VkMemoryRequirements requirements;
    VkResult vr;

    if (d3d12_resource_is_buffer(resource))
    {
        VK_CALL(vkGetBufferMemoryRequirements(vk_device, resource->u.vk_buffer, &requirements));
    }
    else
    {
        VK_CALL(vkGetImageMemoryRequirements(vk_device, resource->u.vk_image, &requirements));
        /* Padding in d3d12_device_GetResourceAllocationInfo() leaves room to align the offset. */
        heap_offset = align(heap_offset, requirements.alignment);
    }

    if (heap_offset > heap->desc.SizeInBytes || requirements.size > heap->desc.SizeInBytes - heap_offset)
    {
        WARN("Heap too small for the resource (offset %"PRIu64", resource size %"PRIu64", heap size %"PRIu64".\n",
                heap_offset, requirements.size, heap->desc.SizeInBytes);
        return E_INVALIDARG;
    }

    if (heap_offset % requirements.alignment)
    {
        FIXME("Invalid heap offset %#"PRIx64" (alignment %#"PRIx64").\n",
                heap_offset, requirements.alignment);
        goto allocate_memory;
    }

    if (!(requirements.memoryTypeBits & (1u << heap->vk_memory_type)))
    {
        FIXME("Memory type %u cannot be bound to resource %p (allowed types %#x).\n",
                heap->vk_memory_type, resource, requirements.memoryTypeBits);
        goto allocate_memory;
    }

    /* Synchronisation is not required for binding, but vkMapMemory() may be called
     * from another thread and it requires exclusive access. */
    vkd3d_mutex_lock(&heap->mutex);

    if (d3d12_resource_is_buffer(resource))
        vr = VK_CALL(vkBindBufferMemory(vk_device, resource->u.vk_buffer, heap->vk_memory, heap_offset));
    else
        vr = VK_CALL(vkBindImageMemory(vk_device, resource->u.vk_image, heap->vk_memory, heap_offset));

    vkd3d_mutex_unlock(&heap->mutex);

    if (vr == VK_SUCCESS)
    {
        resource->heap = heap;
        resource->heap_offset = heap_offset;
        vkd3d_atomic_increment_u32(&heap->resource_count);
    }
    else
    {
        WARN("Failed to bind memory, vr %d.\n", vr);
    }

    return hresult_from_vk_result(vr);

allocate_memory:
    FIXME("Allocating device memory.\n");
    return vkd3d_allocate_resource_memory(device, resource, &heap->desc.Properties, heap->desc.Flags);
}

HRESULT d3d12_placed_resource_create(struct d3d12_device *device, struct d3d12_heap *heap, uint64_t heap_offset,
        const D3D12_RESOURCE_DESC1 *desc, D3D12_RESOURCE_STATES initial_state,
        const D3D12_CLEAR_VALUE *optimized_clear_value, struct d3d12_resource **resource)
{
    struct d3d12_resource *object;
    HRESULT hr;

    if (FAILED(hr = d3d12_resource_create(device, &heap->desc.Properties, heap->desc.Flags,
            desc, initial_state, optimized_clear_value, &object)))
        return hr;

    if (FAILED(hr = vkd3d_bind_heap_memory(device, object, heap, heap_offset)))
    {
        d3d12_resource_Release(&object->ID3D12Resource2_iface);
        return hr;
    }

    TRACE("Created placed resource %p.\n", object);

    *resource = object;

    return S_OK;
}

HRESULT d3d12_reserved_resource_create(struct d3d12_device *device,
        const D3D12_RESOURCE_DESC1 *desc, D3D12_RESOURCE_STATES initial_state,
        const D3D12_CLEAR_VALUE *optimized_clear_value, struct d3d12_resource **resource)
{
    struct d3d12_resource *object;
    HRESULT hr;

    if (FAILED(hr = d3d12_resource_create(device, NULL, 0,
            desc, initial_state, optimized_clear_value, &object)))
        return hr;

    if (!d3d12_resource_init_tiles(object, device))
    {
        d3d12_resource_Release(&object->ID3D12Resource2_iface);
        return E_OUTOFMEMORY;
    }

    TRACE("Created reserved resource %p.\n", object);

    *resource = object;

    return S_OK;
}

HRESULT vkd3d_create_image_resource(ID3D12Device *device,
        const struct vkd3d_image_resource_create_info *create_info, ID3D12Resource **resource)
{
    struct d3d12_device *d3d12_device = unsafe_impl_from_ID3D12Device9((ID3D12Device9 *)device);
    struct d3d12_resource *object;
    HRESULT hr;

    TRACE("device %p, create_info %p, resource %p.\n", device, create_info, resource);

    if (!create_info || !resource)
        return E_INVALIDARG;
    if (create_info->type != VKD3D_STRUCTURE_TYPE_IMAGE_RESOURCE_CREATE_INFO)
    {
        WARN("Invalid structure type %#x.\n", create_info->type);
        return E_INVALIDARG;
    }
    if (create_info->next)
        WARN("Unhandled next %p.\n", create_info->next);

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    memset(object, 0, sizeof(*object));

    object->ID3D12Resource2_iface.lpVtbl = &d3d12_resource_vtbl;
    object->refcount = 1;
    object->internal_refcount = 1;
    d3d12_resource_desc1_from_desc(&object->desc, &create_info->desc);
    object->format = vkd3d_format_from_d3d12_resource_desc(d3d12_device, &object->desc, 0);
    object->u.vk_image = create_info->vk_image;
    object->flags = VKD3D_RESOURCE_EXTERNAL;
    object->flags |= create_info->flags & VKD3D_RESOURCE_PUBLIC_FLAGS;
    object->initial_state = D3D12_RESOURCE_STATE_COMMON;
    if (create_info->flags & VKD3D_RESOURCE_PRESENT_STATE_TRANSITION)
        object->present_state = create_info->present_state;
    else
        object->present_state = D3D12_RESOURCE_STATE_COMMON;

    if (FAILED(hr = vkd3d_private_store_init(&object->private_store)))
    {
        vkd3d_free(object);
        return hr;
    }

    d3d12_device_add_ref(object->device = d3d12_device);

    TRACE("Created resource %p.\n", object);

    *resource = (ID3D12Resource *)&object->ID3D12Resource2_iface;

    return S_OK;
}

ULONG vkd3d_resource_incref(ID3D12Resource *resource)
{
    TRACE("resource %p.\n", resource);
    return d3d12_resource_incref(impl_from_ID3D12Resource(resource));
}

ULONG vkd3d_resource_decref(ID3D12Resource *resource)
{
    TRACE("resource %p.\n", resource);
    return d3d12_resource_decref(impl_from_ID3D12Resource(resource));
}

#define HEAD_INDEX_MASK (ARRAY_SIZE(cache->heads) - 1)

/* Objects are cached so that vkd3d_view_incref() can safely check the refcount of an
 * object freed by another thread. This could be implemented as a single atomic linked
 * list, but it requires handling the ABA problem, which brings issues with cross-platform
 * support, compiler support, and non-universal x86-64 support for 128-bit CAS. */
static void *vkd3d_desc_object_cache_get(struct vkd3d_desc_object_cache *cache)
{
    union d3d12_desc_object u;
    unsigned int i;

    STATIC_ASSERT(!(ARRAY_SIZE(cache->heads) & HEAD_INDEX_MASK));

    i = vkd3d_atomic_increment_u32(&cache->next_index) & HEAD_INDEX_MASK;
    for (;;)
    {
        if (vkd3d_atomic_compare_exchange_u32(&cache->heads[i].spinlock, 0, 1))
        {
            if ((u.object = cache->heads[i].head))
            {
                vkd3d_atomic_decrement_u32(&cache->free_count);
                cache->heads[i].head = u.header->next;
                vkd3d_atomic_exchange_u32(&cache->heads[i].spinlock, 0);
                return u.object;
            }
            vkd3d_atomic_exchange_u32(&cache->heads[i].spinlock, 0);
        }
        /* Keeping a free count avoids uncertainty over when this loop should terminate,
         * which could result in excess allocations gradually increasing without limit. */
        if (cache->free_count < ARRAY_SIZE(cache->heads))
            return vkd3d_malloc(cache->size);

        i = (i + 1) & HEAD_INDEX_MASK;
    }
}

static void vkd3d_desc_object_cache_push(struct vkd3d_desc_object_cache *cache, void *object)
{
    union d3d12_desc_object u = {object};
    unsigned int i;
    void *head;

    /* Using the same index as above may result in a somewhat uneven distribution,
     * but the main objective is to avoid costly spinlock contention. */
    i = vkd3d_atomic_increment_u32(&cache->next_index) & HEAD_INDEX_MASK;
    for (;;)
    {
        if (vkd3d_atomic_compare_exchange_u32(&cache->heads[i].spinlock, 0, 1))
            break;
        i = (i + 1) & HEAD_INDEX_MASK;
    }

    head = cache->heads[i].head;
    u.header->next = head;
    cache->heads[i].head = u.object;
    vkd3d_atomic_exchange_u32(&cache->heads[i].spinlock, 0);
    vkd3d_atomic_increment_u32(&cache->free_count);
}

#undef HEAD_INDEX_MASK

static struct vkd3d_cbuffer_desc *vkd3d_cbuffer_desc_create(struct d3d12_device *device)
{
    struct vkd3d_cbuffer_desc *desc;

    if (!(desc = vkd3d_desc_object_cache_get(&device->cbuffer_desc_cache)))
        return NULL;

    desc->h.magic = VKD3D_DESCRIPTOR_MAGIC_CBV;
    desc->h.vk_descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc->h.refcount = 1;

    return desc;
}

static struct vkd3d_view *vkd3d_view_create(uint32_t magic, VkDescriptorType vk_descriptor_type,
        enum vkd3d_view_type type, struct d3d12_device *device)
{
    struct vkd3d_view *view;

    VKD3D_ASSERT(magic);

    if (!(view = vkd3d_desc_object_cache_get(&device->view_desc_cache)))
    {
        ERR("Failed to allocate descriptor object.\n");
        return NULL;
    }

    view->h.magic = magic;
    view->h.vk_descriptor_type = vk_descriptor_type;
    view->h.refcount = 1;
    view->v.type = type;
    view->v.vk_counter_view = VK_NULL_HANDLE;

    return view;
}

static void vkd3d_view_destroy(struct vkd3d_view *view, struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    TRACE("Destroying view %p.\n", view);

    switch (view->v.type)
    {
        case VKD3D_VIEW_TYPE_BUFFER:
            VK_CALL(vkDestroyBufferView(device->vk_device, view->v.u.vk_buffer_view, NULL));
            break;
        case VKD3D_VIEW_TYPE_IMAGE:
            VK_CALL(vkDestroyImageView(device->vk_device, view->v.u.vk_image_view, NULL));
            break;
        case VKD3D_VIEW_TYPE_SAMPLER:
            VK_CALL(vkDestroySampler(device->vk_device, view->v.u.vk_sampler, NULL));
            break;
        default:
            WARN("Unhandled view type %d.\n", view->v.type);
    }

    if (view->v.vk_counter_view)
        VK_CALL(vkDestroyBufferView(device->vk_device, view->v.vk_counter_view, NULL));

    vkd3d_desc_object_cache_push(&device->view_desc_cache, view);
}

void vkd3d_view_decref(void *view, struct d3d12_device *device)
{
    union d3d12_desc_object u = {view};

    if (vkd3d_atomic_decrement_u32(&u.header->refcount))
        return;

    if (u.header->magic != VKD3D_DESCRIPTOR_MAGIC_CBV)
        vkd3d_view_destroy(u.view, device);
    else
        vkd3d_desc_object_cache_push(&device->cbuffer_desc_cache, u.object);
}

static inline void d3d12_desc_replace(struct d3d12_desc *dst, void *view, struct d3d12_device *device)
{
    if ((view = vkd3d_atomic_exchange_ptr(&dst->s.u.object, view)))
        vkd3d_view_decref(view, device);
}

#define VKD3D_DESCRIPTOR_WRITE_BUFFER_SIZE 24

struct descriptor_writes
{
    VkDescriptorBufferInfo null_vk_cbv_info;
    VkBufferView null_vk_buffer_view;
    VkDescriptorImageInfo vk_image_infos[VKD3D_DESCRIPTOR_WRITE_BUFFER_SIZE];
    VkWriteDescriptorSet vk_descriptor_writes[VKD3D_DESCRIPTOR_WRITE_BUFFER_SIZE];
    void *held_refs[VKD3D_DESCRIPTOR_WRITE_BUFFER_SIZE];
    unsigned int count;
    unsigned int held_ref_count;
};

static void descriptor_writes_free_object_refs(struct descriptor_writes *writes, struct d3d12_device *device)
{
    unsigned int i;
    for (i = 0; i < writes->held_ref_count; ++i)
        vkd3d_view_decref(writes->held_refs[i], device);
    writes->held_ref_count = 0;
}

static void d3d12_desc_write_vk_heap_null_descriptor(struct d3d12_descriptor_heap *descriptor_heap,
        uint32_t dst_array_element, struct descriptor_writes *writes, struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct d3d12_descriptor_heap_vk_set *descriptor_set;
    enum vkd3d_vk_descriptor_set_index set, end;
    unsigned int i = writes->count;

    end = device->vk_info.EXT_mutable_descriptor_type ? VKD3D_SET_INDEX_MUTABLE
            : VKD3D_SET_INDEX_STORAGE_IMAGE;
    /* Binding a shader with the wrong null descriptor type works in Windows.
     * To support that here we must write one to all applicable Vulkan sets. */
    for (set = VKD3D_SET_INDEX_UNIFORM_BUFFER; set <= end; ++set)
    {
        descriptor_set = &descriptor_heap->vk_descriptor_sets[set];
        writes->vk_descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes->vk_descriptor_writes[i].pNext = NULL;
        writes->vk_descriptor_writes[i].dstSet = descriptor_set->vk_set;
        writes->vk_descriptor_writes[i].dstBinding = 0;
        writes->vk_descriptor_writes[i].dstArrayElement = dst_array_element;
        writes->vk_descriptor_writes[i].descriptorCount = 1;
        writes->vk_descriptor_writes[i].descriptorType = descriptor_set->vk_type;
        switch (set)
        {
            case VKD3D_SET_INDEX_UNIFORM_BUFFER:
                writes->vk_descriptor_writes[i].pImageInfo = NULL;
                writes->vk_descriptor_writes[i].pBufferInfo = &writes->null_vk_cbv_info;
                writes->vk_descriptor_writes[i].pTexelBufferView = NULL;
                break;
            case VKD3D_SET_INDEX_SAMPLED_IMAGE:
            case VKD3D_SET_INDEX_STORAGE_IMAGE:
                writes->vk_descriptor_writes[i].pImageInfo = &writes->vk_image_infos[i];
                writes->vk_descriptor_writes[i].pBufferInfo = NULL;
                writes->vk_descriptor_writes[i].pTexelBufferView = NULL;
                writes->vk_image_infos[i].sampler = VK_NULL_HANDLE;
                writes->vk_image_infos[i].imageView = VK_NULL_HANDLE;
                writes->vk_image_infos[i].imageLayout = (set == VKD3D_SET_INDEX_STORAGE_IMAGE)
                        ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;
            case VKD3D_SET_INDEX_UNIFORM_TEXEL_BUFFER:
            case VKD3D_SET_INDEX_STORAGE_TEXEL_BUFFER:
                writes->vk_descriptor_writes[i].pImageInfo = NULL;
                writes->vk_descriptor_writes[i].pBufferInfo = NULL;
                writes->vk_descriptor_writes[i].pTexelBufferView = &writes->null_vk_buffer_view;
                break;
            default:
                VKD3D_ASSERT(false);
                break;
        }
        if (++i < ARRAY_SIZE(writes->vk_descriptor_writes) - 1)
            continue;
        VK_CALL(vkUpdateDescriptorSets(device->vk_device, i, writes->vk_descriptor_writes, 0, NULL));
        descriptor_writes_free_object_refs(writes, device);
        i = 0;
    }

    writes->count = i;
}

static void d3d12_desc_write_vk_heap(struct d3d12_descriptor_heap *descriptor_heap, unsigned int dst_array_element,
        struct descriptor_writes *writes, void *object, struct d3d12_device *device)
{
    struct d3d12_descriptor_heap_vk_set *descriptor_set;
    const struct vkd3d_vk_device_procs *vk_procs;
    union d3d12_desc_object u = {object};
    unsigned int i = writes->count;
    VkDescriptorType type;
    bool is_null = false;

    type = u.header->vk_descriptor_type;
    descriptor_set = &descriptor_heap->vk_descriptor_sets[vkd3d_vk_descriptor_set_index_from_vk_descriptor_type(type)];
    vk_procs = &device->vk_procs;

    writes->vk_descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes->vk_descriptor_writes[i].pNext = NULL;
    writes->vk_descriptor_writes[i].dstSet = descriptor_set->vk_set;
    writes->vk_descriptor_writes[i].dstBinding = 0;
    writes->vk_descriptor_writes[i].dstArrayElement = dst_array_element;
    writes->vk_descriptor_writes[i].descriptorCount = 1;
    writes->vk_descriptor_writes[i].descriptorType = type;
    switch (type)
    {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            writes->vk_descriptor_writes[i].pImageInfo = NULL;
            writes->vk_descriptor_writes[i].pBufferInfo = &u.cb_desc->vk_cbv_info;
            writes->vk_descriptor_writes[i].pTexelBufferView = NULL;
            is_null = !u.cb_desc->vk_cbv_info.buffer;
            break;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            writes->vk_descriptor_writes[i].pImageInfo = &writes->vk_image_infos[i];
            writes->vk_descriptor_writes[i].pBufferInfo = NULL;
            writes->vk_descriptor_writes[i].pTexelBufferView = NULL;
            writes->vk_image_infos[i].sampler = VK_NULL_HANDLE;
            is_null = !(writes->vk_image_infos[i].imageView = u.view->v.u.vk_image_view);
            writes->vk_image_infos[i].imageLayout = (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                    ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            writes->vk_descriptor_writes[i].pImageInfo = NULL;
            writes->vk_descriptor_writes[i].pBufferInfo = NULL;
            writes->vk_descriptor_writes[i].pTexelBufferView = &u.view->v.u.vk_buffer_view;
            is_null = !u.view->v.u.vk_buffer_view;
            break;
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            writes->vk_descriptor_writes[i].pImageInfo = &writes->vk_image_infos[i];
            writes->vk_descriptor_writes[i].pBufferInfo = NULL;
            writes->vk_descriptor_writes[i].pTexelBufferView = NULL;
            writes->vk_image_infos[i].sampler = u.view->v.u.vk_sampler;
            writes->vk_image_infos[i].imageView = VK_NULL_HANDLE;
            writes->vk_image_infos[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            break;
        default:
            ERR("Unhandled descriptor type %#x.\n", type);
            break;
    }
    if (is_null && device->vk_info.EXT_robustness2)
        return d3d12_desc_write_vk_heap_null_descriptor(descriptor_heap, dst_array_element, writes, device);

    ++i;
    if (u.header->magic == VKD3D_DESCRIPTOR_MAGIC_UAV && u.view->v.vk_counter_view)
    {
        descriptor_set = &descriptor_heap->vk_descriptor_sets[VKD3D_SET_INDEX_UAV_COUNTER];
        writes->vk_descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes->vk_descriptor_writes[i].pNext = NULL;
        writes->vk_descriptor_writes[i].dstSet = descriptor_set->vk_set;
        writes->vk_descriptor_writes[i].dstBinding = 0;
        writes->vk_descriptor_writes[i].dstArrayElement = dst_array_element;
        writes->vk_descriptor_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        writes->vk_descriptor_writes[i].descriptorCount = 1;
        writes->vk_descriptor_writes[i].pImageInfo = NULL;
        writes->vk_descriptor_writes[i].pBufferInfo = NULL;
        writes->vk_descriptor_writes[i++].pTexelBufferView = &u.view->v.vk_counter_view;
    }

    if (i >= ARRAY_SIZE(writes->vk_descriptor_writes) - 1)
    {
        VK_CALL(vkUpdateDescriptorSets(device->vk_device, i, writes->vk_descriptor_writes, 0, NULL));
        descriptor_writes_free_object_refs(writes, device);
        i = 0;
    }

    writes->count = i;
}

void d3d12_desc_flush_vk_heap_updates_locked(struct d3d12_descriptor_heap *descriptor_heap, struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct d3d12_desc *descriptors, *src;
    struct descriptor_writes writes;
    union d3d12_desc_object u;
    unsigned int i, next;

    if ((i = vkd3d_atomic_exchange_u32(&descriptor_heap->dirty_list_head, UINT_MAX)) == UINT_MAX)
        return;

    writes.null_vk_cbv_info.buffer = VK_NULL_HANDLE;
    writes.null_vk_cbv_info.offset = 0;
    writes.null_vk_cbv_info.range = VK_WHOLE_SIZE;
    writes.null_vk_buffer_view = VK_NULL_HANDLE;
    writes.count = 0;
    writes.held_ref_count = 0;

    descriptors = (struct d3d12_desc *)descriptor_heap->descriptors;

    for (; i != UINT_MAX; i = next)
    {
        src = &descriptors[i];
        next = vkd3d_atomic_exchange_u32(&src->next, 0);
        next = (int)next >> 1;

        /* A race exists here between updating src->next and getting the current object. The best
         * we can do is get the object last, which may result in a harmless rewrite later. */
        u.object = d3d12_desc_get_object_ref(src, device);

        if (!u.object)
            continue;

        writes.held_refs[writes.held_ref_count++] = u.object;
        d3d12_desc_write_vk_heap(descriptor_heap, i, &writes, u.object, device);
    }

    /* Avoid thunk calls wherever possible. */
    if (writes.count)
        VK_CALL(vkUpdateDescriptorSets(device->vk_device, writes.count, writes.vk_descriptor_writes, 0, NULL));
    descriptor_writes_free_object_refs(&writes, device);
}

static void d3d12_desc_mark_as_modified(struct d3d12_desc *dst, struct d3d12_descriptor_heap *descriptor_heap)
{
    unsigned int i, head;

    i = dst->index;
    head = descriptor_heap->dirty_list_head;

    /* Only one thread can swap the value away from zero. */
    if (!vkd3d_atomic_compare_exchange_u32(&dst->next, 0, (head << 1) | 1))
        return;
    /* Now it is safe to modify 'next' to another nonzero value if necessary. */
    while (!vkd3d_atomic_compare_exchange_u32(&descriptor_heap->dirty_list_head, head, i))
    {
        head = descriptor_heap->dirty_list_head;
        vkd3d_atomic_exchange_u32(&dst->next, (head << 1) | 1);
    }
}

static inline void descriptor_heap_write_atomic(struct d3d12_descriptor_heap *descriptor_heap, struct d3d12_desc *dst,
        const struct d3d12_desc *src, struct d3d12_device *device)
{
    void *object = src->s.u.object;

    d3d12_desc_replace(dst, object, device);
    if (descriptor_heap->use_vk_heaps && object && !dst->next)
        d3d12_desc_mark_as_modified(dst, descriptor_heap);
}

void d3d12_desc_write_atomic(struct d3d12_desc *dst, const struct d3d12_desc *src,
        struct d3d12_device *device)
{
    descriptor_heap_write_atomic(d3d12_desc_get_descriptor_heap(dst), dst, src, device);
}

static void d3d12_desc_destroy(struct d3d12_desc *descriptor, struct d3d12_device *device)
{
    d3d12_desc_replace(descriptor, NULL, device);
}

/* This is a major performance bottleneck for some games, so do not load the device
 * pointer from dst_heap. In some cases device will not be used. */
void d3d12_desc_copy(struct d3d12_desc *dst, const struct d3d12_desc *src, struct d3d12_descriptor_heap *dst_heap,
        struct d3d12_device *device)
{
    struct d3d12_desc tmp;

    VKD3D_ASSERT(dst != src);

    tmp.s.u.object = d3d12_desc_get_object_ref(src, device);
    descriptor_heap_write_atomic(dst_heap, dst, &tmp, device);
}

static VkDeviceSize vkd3d_get_required_texel_buffer_alignment(const struct d3d12_device *device,
        const struct vkd3d_format *format)
{
    const VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT *properties;
    const struct vkd3d_vulkan_info *vk_info = &device->vk_info;
    VkDeviceSize alignment;

    if (vk_info->EXT_texel_buffer_alignment)
    {
        properties = &vk_info->texel_buffer_alignment_properties;

        alignment = max(properties->storageTexelBufferOffsetAlignmentBytes,
                properties->uniformTexelBufferOffsetAlignmentBytes);

        if (properties->storageTexelBufferOffsetSingleTexelAlignment
                && properties->uniformTexelBufferOffsetSingleTexelAlignment)
        {
            VKD3D_ASSERT(!vkd3d_format_is_compressed(format));
            return min(format->byte_count, alignment);
        }

        return alignment;
    }

    return vk_info->device_limits.minTexelBufferOffsetAlignment;
}

static bool vkd3d_create_vk_buffer_view(struct d3d12_device *device,
        VkBuffer vk_buffer, const struct vkd3d_format *format,
        VkDeviceSize offset, VkDeviceSize range, VkBufferView *vk_view)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct VkBufferViewCreateInfo view_desc;
    VkDeviceSize alignment;
    VkResult vr;

    if (vkd3d_format_is_compressed(format))
    {
        WARN("Invalid format for buffer view %#x.\n", format->dxgi_format);
        return false;
    }

    alignment = vkd3d_get_required_texel_buffer_alignment(device, format);
    if (offset % alignment)
        FIXME("Offset %#"PRIx64" violates the required alignment %#"PRIx64".\n", offset, alignment);

    view_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_desc.pNext = NULL;
    view_desc.flags = 0;
    view_desc.buffer = vk_buffer;
    view_desc.format = format->vk_format;
    view_desc.offset = offset;
    view_desc.range = range;
    if ((vr = VK_CALL(vkCreateBufferView(device->vk_device, &view_desc, NULL, vk_view))) < 0)
        WARN("Failed to create Vulkan buffer view, vr %d.\n", vr);
    return vr == VK_SUCCESS;
}

bool vkd3d_create_buffer_view(struct d3d12_device *device, uint32_t magic, VkBuffer vk_buffer,
        const struct vkd3d_format *format, VkDeviceSize offset, VkDeviceSize size,
        struct vkd3d_view **view)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkBufferView vk_view = VK_NULL_HANDLE;
    struct vkd3d_view *object;

    if (vk_buffer && !vkd3d_create_vk_buffer_view(device, vk_buffer, format, offset, size, &vk_view))
        return false;

    if (!(object = vkd3d_view_create(magic, magic == VKD3D_DESCRIPTOR_MAGIC_UAV
            ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
            VKD3D_VIEW_TYPE_BUFFER, device)))
    {
        VK_CALL(vkDestroyBufferView(device->vk_device, vk_view, NULL));
        return false;
    }

    object->v.u.vk_buffer_view = vk_view;
    object->v.format = format;
    object->v.info.buffer.offset = offset;
    object->v.info.buffer.size = size;
    *view = object;
    return true;
}

#define VKD3D_VIEW_RAW_BUFFER 0x1

static bool vkd3d_create_buffer_view_for_resource(struct d3d12_device *device,
        uint32_t magic, struct d3d12_resource *resource, DXGI_FORMAT view_format,
        unsigned int offset, unsigned int size, unsigned int structure_stride,
        unsigned int flags, struct vkd3d_view **view)
{
    const struct vkd3d_format *format;
    VkDeviceSize element_size;

    if (view_format == DXGI_FORMAT_R32_TYPELESS && (flags & VKD3D_VIEW_RAW_BUFFER))
    {
        format = vkd3d_get_format(device, DXGI_FORMAT_R32_UINT, false);
        element_size = format->byte_count;
    }
    else if (view_format == DXGI_FORMAT_UNKNOWN && structure_stride)
    {
        format = vkd3d_get_format(device, DXGI_FORMAT_R32_UINT, false);
        element_size = structure_stride;
    }
    else if ((format = vkd3d_format_from_d3d12_resource_desc(device, &resource->desc, view_format)))
    {
        /* TODO: if view_format is DXGI_FORMAT_UNKNOWN, this is always 1, which
         * may not match driver behaviour (return false?). */
        element_size = format->byte_count;
    }
    else
    {
        WARN("Failed to find format for %#x.\n", resource->desc.Format);
        return false;
    }

    VKD3D_ASSERT(d3d12_resource_is_buffer(resource));

    return vkd3d_create_buffer_view(device, magic, resource->u.vk_buffer,
            format, offset * element_size, size * element_size, view);
}

static void vkd3d_set_view_swizzle_for_format(VkComponentMapping *components,
        const struct vkd3d_format *format, bool allowed_swizzle)
{
    components->r = VK_COMPONENT_SWIZZLE_R;
    components->g = VK_COMPONENT_SWIZZLE_G;
    components->b = VK_COMPONENT_SWIZZLE_B;
    components->a = VK_COMPONENT_SWIZZLE_A;

    if (format->vk_aspect_mask == VK_IMAGE_ASPECT_STENCIL_BIT)
    {
        if (allowed_swizzle)
        {
            components->r = VK_COMPONENT_SWIZZLE_ZERO;
            components->g = VK_COMPONENT_SWIZZLE_R;
            components->b = VK_COMPONENT_SWIZZLE_ZERO;
            components->a = VK_COMPONENT_SWIZZLE_ZERO;
        }
        else
        {
            FIXME("Stencil swizzle is not supported for format %#x.\n",
                    format->dxgi_format);
        }
    }

    if (format->dxgi_format == DXGI_FORMAT_A8_UNORM)
    {
        if (allowed_swizzle)
        {
            components->r = VK_COMPONENT_SWIZZLE_ZERO;
            components->g = VK_COMPONENT_SWIZZLE_ZERO;
            components->b = VK_COMPONENT_SWIZZLE_ZERO;
            components->a = VK_COMPONENT_SWIZZLE_R;
        }
        else
        {
            FIXME("Alpha swizzle is not supported.\n");
        }
    }

    if (format->dxgi_format == DXGI_FORMAT_B8G8R8X8_UNORM
            || format->dxgi_format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB)
    {
        if (allowed_swizzle)
        {
            components->r = VK_COMPONENT_SWIZZLE_R;
            components->g = VK_COMPONENT_SWIZZLE_G;
            components->b = VK_COMPONENT_SWIZZLE_B;
            components->a = VK_COMPONENT_SWIZZLE_ONE;
        }
        else
        {
            FIXME("B8G8R8X8 swizzle is not supported.\n");
        }
    }
}

static VkComponentSwizzle vk_component_swizzle_from_d3d12(unsigned int component_mapping,
        unsigned int component_index)
{
    D3D12_SHADER_COMPONENT_MAPPING mapping
            = D3D12_DECODE_SHADER_4_COMPONENT_MAPPING(component_index, component_mapping);

    switch (mapping)
    {
        case D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0:
            return VK_COMPONENT_SWIZZLE_R;
        case D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1:
            return VK_COMPONENT_SWIZZLE_G;
        case D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2:
            return VK_COMPONENT_SWIZZLE_B;
        case D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3:
            return VK_COMPONENT_SWIZZLE_A;
        case D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0:
            return VK_COMPONENT_SWIZZLE_ZERO;
        case D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1:
            return VK_COMPONENT_SWIZZLE_ONE;
    }

    FIXME("Invalid component mapping %#x.\n", mapping);
    return VK_COMPONENT_SWIZZLE_IDENTITY;
}

static void vk_component_mapping_from_d3d12(VkComponentMapping *components,
        unsigned int component_mapping)
{
    components->r = vk_component_swizzle_from_d3d12(component_mapping, 0);
    components->g = vk_component_swizzle_from_d3d12(component_mapping, 1);
    components->b = vk_component_swizzle_from_d3d12(component_mapping, 2);
    components->a = vk_component_swizzle_from_d3d12(component_mapping, 3);
}

static VkComponentSwizzle swizzle_vk_component(const VkComponentMapping *components,
        VkComponentSwizzle component, VkComponentSwizzle swizzle)
{
    switch (swizzle)
    {
        case VK_COMPONENT_SWIZZLE_IDENTITY:
            break;

        case VK_COMPONENT_SWIZZLE_R:
            component = components->r;
            break;

        case VK_COMPONENT_SWIZZLE_G:
            component = components->g;
            break;

        case VK_COMPONENT_SWIZZLE_B:
            component = components->b;
            break;

        case VK_COMPONENT_SWIZZLE_A:
            component = components->a;
            break;

        case VK_COMPONENT_SWIZZLE_ONE:
        case VK_COMPONENT_SWIZZLE_ZERO:
            component = swizzle;
            break;

        default:
            FIXME("Invalid component swizzle %#x.\n", swizzle);
            break;
    }

    VKD3D_ASSERT(component != VK_COMPONENT_SWIZZLE_IDENTITY);
    return component;
}

static void vk_component_mapping_compose(VkComponentMapping *dst, const VkComponentMapping *b)
{
    const VkComponentMapping a = *dst;

    dst->r = swizzle_vk_component(&a, a.r, b->r);
    dst->g = swizzle_vk_component(&a, a.g, b->g);
    dst->b = swizzle_vk_component(&a, a.b, b->b);
    dst->a = swizzle_vk_component(&a, a.a, b->a);
}

static bool init_default_texture_view_desc(struct vkd3d_texture_view_desc *desc,
        struct d3d12_resource *resource, DXGI_FORMAT view_format)
{
    const struct d3d12_device *device = resource->device;

    if (view_format == resource->desc.Format)
    {
        desc->format = resource->format;
    }
    else if (!(desc->format = vkd3d_format_from_d3d12_resource_desc(device, &resource->desc, view_format)))
    {
        FIXME("Failed to find format (resource format %#x, view format %#x).\n",
                resource->desc.Format, view_format);
        return false;
    }

    desc->miplevel_idx = 0;
    desc->miplevel_count = 1;
    desc->layer_idx = 0;
    desc->layer_count = d3d12_resource_desc_get_layer_count(&resource->desc);
    desc->vk_image_aspect = desc->format->vk_aspect_mask;

    switch (resource->desc.Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            desc->view_type = resource->desc.DepthOrArraySize > 1
                    ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            desc->view_type = resource->desc.DepthOrArraySize > 1
                    ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc->view_type = VK_IMAGE_VIEW_TYPE_3D;
            desc->layer_count = 1;
            break;

        default:
            FIXME("Resource dimension %#x not implemented.\n", resource->desc.Dimension);
            return false;
    }

    desc->components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    desc->components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    desc->components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    desc->components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    desc->allowed_swizzle = false;
    desc->usage = 0;
    return true;
}

static void vkd3d_texture_view_desc_normalise(struct vkd3d_texture_view_desc *desc,
        const D3D12_RESOURCE_DESC1 *resource_desc)
{
    unsigned int max_layer_count;

    if (resource_desc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        if (desc->view_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
            max_layer_count = max(1, resource_desc->DepthOrArraySize >> desc->miplevel_idx);
        else
            max_layer_count = 1;
    }
    else
    {
        max_layer_count = resource_desc->DepthOrArraySize;
    }

    if (desc->layer_idx >= max_layer_count)
    {
        WARN("Layer index %u exceeds maximum available layer %u.\n", desc->layer_idx, max_layer_count - 1);
        desc->layer_count = 1;
        return;
    }

    max_layer_count -= desc->layer_idx;
    if (desc->layer_count <= max_layer_count)
        return;

    if (desc->layer_count != UINT_MAX)
        WARN("Layer count %u exceeds maximum %u.\n", desc->layer_count, max_layer_count);
    desc->layer_count = max_layer_count;
}

bool vkd3d_create_texture_view(struct d3d12_device *device, uint32_t magic, VkImage vk_image,
        const struct vkd3d_texture_view_desc *desc, struct vkd3d_view **view)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    const struct vkd3d_format *format = desc->format;
    VkImageViewUsageCreateInfoKHR usage_desc;
    struct VkImageViewCreateInfo view_desc;
    VkImageView vk_view = VK_NULL_HANDLE;
    struct vkd3d_view *object;
    VkResult vr;

    if (vk_image)
    {
        view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_desc.pNext = NULL;
        view_desc.flags = 0;
        view_desc.image = vk_image;
        view_desc.viewType = desc->view_type;
        view_desc.format = format->vk_format;
        vkd3d_set_view_swizzle_for_format(&view_desc.components, format, desc->allowed_swizzle);
        if (desc->allowed_swizzle)
            vk_component_mapping_compose(&view_desc.components, &desc->components);
        view_desc.subresourceRange.aspectMask = desc->vk_image_aspect;
        view_desc.subresourceRange.baseMipLevel = desc->miplevel_idx;
        view_desc.subresourceRange.levelCount = desc->miplevel_count;
        view_desc.subresourceRange.baseArrayLayer = desc->layer_idx;
        view_desc.subresourceRange.layerCount = desc->layer_count;
        if (device->vk_info.KHR_maintenance2)
        {
            usage_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
            usage_desc.pNext = NULL;
            usage_desc.usage = desc->usage;
            view_desc.pNext = &usage_desc;
        }
        if ((vr = VK_CALL(vkCreateImageView(device->vk_device, &view_desc, NULL, &vk_view))) < 0)
        {
            WARN("Failed to create Vulkan image view, vr %d.\n", vr);
            return false;
        }
    }

    if (!(object = vkd3d_view_create(magic, magic == VKD3D_DESCRIPTOR_MAGIC_UAV ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
            : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VKD3D_VIEW_TYPE_IMAGE, device)))
    {
        VK_CALL(vkDestroyImageView(device->vk_device, vk_view, NULL));
        return false;
    }

    object->v.u.vk_image_view = vk_view;
    object->v.format = format;
    object->v.info.texture.vk_view_type = desc->view_type;
    object->v.info.texture.miplevel_idx = desc->miplevel_idx;
    object->v.info.texture.layer_idx = desc->layer_idx;
    object->v.info.texture.layer_count = desc->layer_count;
    *view = object;
    return true;
}

void d3d12_desc_create_cbv(struct d3d12_desc *descriptor,
        struct d3d12_device *device, const D3D12_CONSTANT_BUFFER_VIEW_DESC *desc)
{
    struct VkDescriptorBufferInfo *buffer_info;
    struct vkd3d_cbuffer_desc *cb_desc;
    struct d3d12_resource *resource;

    if (!desc)
    {
        WARN("Constant buffer desc is NULL.\n");
        return;
    }

    if (!(cb_desc = vkd3d_cbuffer_desc_create(device)))
    {
        ERR("Failed to allocate descriptor object.\n");
        return;
    }

    if (desc->SizeInBytes & (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1))
    {
        WARN("Size is not %u bytes aligned.\n", D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        return;
    }

    buffer_info = &cb_desc->vk_cbv_info;
    if (desc->BufferLocation)
    {
        resource = vkd3d_gpu_va_allocator_dereference(&device->gpu_va_allocator, desc->BufferLocation);
        buffer_info->buffer = resource->u.vk_buffer;
        buffer_info->offset = desc->BufferLocation - resource->gpu_address;
        buffer_info->range = min(desc->SizeInBytes, resource->desc.Width - buffer_info->offset);
    }
    else
    {
        /* NULL descriptor */
        buffer_info->buffer = device->null_resources.vk_buffer;
        buffer_info->offset = 0;
        buffer_info->range = VK_WHOLE_SIZE;
    }

    descriptor->s.u.cb_desc = cb_desc;
}

static unsigned int vkd3d_view_flags_from_d3d12_buffer_srv_flags(D3D12_BUFFER_SRV_FLAGS flags)
{
    if (flags == D3D12_BUFFER_SRV_FLAG_RAW)
        return VKD3D_VIEW_RAW_BUFFER;
    if (flags)
        FIXME("Unhandled buffer SRV flags %#x.\n", flags);
    return 0;
}

static void vkd3d_create_null_srv(struct d3d12_desc *descriptor,
        struct d3d12_device *device, const D3D12_SHADER_RESOURCE_VIEW_DESC *desc)
{
    struct vkd3d_null_resources *null_resources = &device->null_resources;
    struct vkd3d_texture_view_desc vkd3d_desc;
    VkImage vk_image;

    if (!desc)
    {
        WARN("D3D12_SHADER_RESOURCE_VIEW_DESC is required for NULL view.\n");
        return;
    }

    switch (desc->ViewDimension)
    {
        case D3D12_SRV_DIMENSION_BUFFER:
            if (!device->vk_info.EXT_robustness2)
                WARN("Creating NULL buffer SRV %#x.\n", desc->Format);

            vkd3d_create_buffer_view(device, VKD3D_DESCRIPTOR_MAGIC_SRV, null_resources->vk_buffer,
                    vkd3d_get_format(device, DXGI_FORMAT_R32_UINT, false),
                    0, VKD3D_NULL_BUFFER_SIZE, &descriptor->s.u.view);
            return;

        case D3D12_SRV_DIMENSION_TEXTURE2D:
            vk_image = null_resources->vk_2d_image;
            vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
            vk_image = null_resources->vk_2d_image;
            vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;

        default:
            if (device->vk_info.EXT_robustness2)
            {
                vk_image = VK_NULL_HANDLE;
                /* view_type is not used for Vulkan null descriptors, but make it valid. */
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D;
                break;
            }
            FIXME("Unhandled view dimension %#x.\n", desc->ViewDimension);
            return;
    }

    if (!device->vk_info.EXT_robustness2)
        WARN("Creating NULL SRV %#x.\n", desc->ViewDimension);

    vkd3d_desc.format = vkd3d_get_format(device, VKD3D_NULL_VIEW_FORMAT, false);
    vkd3d_desc.miplevel_idx = 0;
    vkd3d_desc.miplevel_count = 1;
    vkd3d_desc.layer_idx = 0;
    vkd3d_desc.layer_count = 1;
    vkd3d_desc.vk_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    vkd3d_desc.components.r = VK_COMPONENT_SWIZZLE_ZERO;
    vkd3d_desc.components.g = VK_COMPONENT_SWIZZLE_ZERO;
    vkd3d_desc.components.b = VK_COMPONENT_SWIZZLE_ZERO;
    vkd3d_desc.components.a = VK_COMPONENT_SWIZZLE_ZERO;
    vkd3d_desc.allowed_swizzle = true;
    vkd3d_desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    vkd3d_create_texture_view(device, VKD3D_DESCRIPTOR_MAGIC_SRV, vk_image, &vkd3d_desc, &descriptor->s.u.view);
}

static void vkd3d_create_buffer_srv(struct d3d12_desc *descriptor,
        struct d3d12_device *device, struct d3d12_resource *resource,
        const D3D12_SHADER_RESOURCE_VIEW_DESC *desc)
{
    unsigned int flags;

    if (!desc)
    {
        FIXME("Default SRV views not supported.\n");
        return;
    }

    if (desc->ViewDimension != D3D12_SRV_DIMENSION_BUFFER)
    {
        WARN("Unexpected view dimension %#x.\n", desc->ViewDimension);
        return;
    }

    flags = vkd3d_view_flags_from_d3d12_buffer_srv_flags(desc->u.Buffer.Flags);
    vkd3d_create_buffer_view_for_resource(device, VKD3D_DESCRIPTOR_MAGIC_SRV, resource, desc->Format,
            desc->u.Buffer.FirstElement, desc->u.Buffer.NumElements,
            desc->u.Buffer.StructureByteStride, flags, &descriptor->s.u.view);
}

static VkImageAspectFlags vk_image_aspect_flags_from_d3d12_plane_slice(const struct vkd3d_format *format,
        unsigned int plane_slice)
{
    VkImageAspectFlags aspect_flags = format->vk_aspect_mask;
    unsigned int i;

    /* For all formats we currently handle, the n-th aspect bit in Vulkan, if the lowest bit set is
     * n = 0, corresponds to the n-th plane in D3D12, so clear the lowest bit for each slice skipped. */
    for (i = 0; i < plane_slice; i++)
        aspect_flags &= aspect_flags - 1;

    if (!aspect_flags)
    {
        WARN("Invalid plane slice %u for format %#x.\n", plane_slice, format->vk_format);
        aspect_flags = format->vk_aspect_mask;
    }

    /* The selected slice is now the lowest bit in the aspect flags, so clear the others. */
    return aspect_flags & -aspect_flags;
}

void d3d12_desc_create_srv(struct d3d12_desc *descriptor,
        struct d3d12_device *device, struct d3d12_resource *resource,
        const D3D12_SHADER_RESOURCE_VIEW_DESC *desc)
{
    struct vkd3d_texture_view_desc vkd3d_desc;

    if (!resource)
    {
        vkd3d_create_null_srv(descriptor, device, desc);
        return;
    }

    if (d3d12_resource_is_buffer(resource))
    {
        vkd3d_create_buffer_srv(descriptor, device, resource, desc);
        return;
    }

    if (!init_default_texture_view_desc(&vkd3d_desc, resource, desc ? desc->Format : 0))
        return;

    vkd3d_desc.miplevel_count = VK_REMAINING_MIP_LEVELS;
    vkd3d_desc.allowed_swizzle = true;
    vkd3d_desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    if (desc)
    {
        if (desc->Shader4ComponentMapping != D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
        {
            TRACE("Component mapping %s for format %#x.\n",
                    debug_d3d12_shader_component_mapping(desc->Shader4ComponentMapping), desc->Format);

            vk_component_mapping_from_d3d12(&vkd3d_desc.components, desc->Shader4ComponentMapping);
        }

        switch (desc->ViewDimension)
        {
            case D3D12_SRV_DIMENSION_TEXTURE1D:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_1D;
                vkd3d_desc.miplevel_idx = desc->u.Texture1D.MostDetailedMip;
                vkd3d_desc.miplevel_count = desc->u.Texture1D.MipLevels;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2D:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D;
                vkd3d_desc.miplevel_idx = desc->u.Texture2D.MostDetailedMip;
                vkd3d_desc.miplevel_count = desc->u.Texture2D.MipLevels;
                if (desc->u.Texture2D.PlaneSlice)
                    vkd3d_desc.vk_image_aspect = vk_image_aspect_flags_from_d3d12_plane_slice(resource->format,
                            desc->u.Texture2D.PlaneSlice);
                if (desc->u.Texture2D.ResourceMinLODClamp)
                    FIXME("Unhandled min LOD clamp %.8e.\n", desc->u.Texture2D.ResourceMinLODClamp);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                vkd3d_desc.miplevel_idx = desc->u.Texture2DArray.MostDetailedMip;
                vkd3d_desc.miplevel_count = desc->u.Texture2DArray.MipLevels;
                vkd3d_desc.layer_idx = desc->u.Texture2DArray.FirstArraySlice;
                vkd3d_desc.layer_count = desc->u.Texture2DArray.ArraySize;
                if (desc->u.Texture2DArray.PlaneSlice)
                    vkd3d_desc.vk_image_aspect = vk_image_aspect_flags_from_d3d12_plane_slice(resource->format,
                            desc->u.Texture2DArray.PlaneSlice);
                if (desc->u.Texture2DArray.ResourceMinLODClamp)
                    FIXME("Unhandled min LOD clamp %.8e.\n", desc->u.Texture2DArray.ResourceMinLODClamp);
                vkd3d_texture_view_desc_normalise(&vkd3d_desc, &resource->desc);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DMS:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                vkd3d_desc.layer_idx = desc->u.Texture2DMSArray.FirstArraySlice;
                vkd3d_desc.layer_count = desc->u.Texture2DMSArray.ArraySize;
                vkd3d_texture_view_desc_normalise(&vkd3d_desc, &resource->desc);
                break;
            case D3D12_SRV_DIMENSION_TEXTURE3D:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_3D;
                vkd3d_desc.miplevel_idx = desc->u.Texture3D.MostDetailedMip;
                vkd3d_desc.miplevel_count = desc->u.Texture3D.MipLevels;
                if (desc->u.Texture3D.ResourceMinLODClamp)
                    FIXME("Unhandled min LOD clamp %.8e.\n", desc->u.Texture2D.ResourceMinLODClamp);
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBE:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_CUBE;
                vkd3d_desc.miplevel_idx = desc->u.TextureCube.MostDetailedMip;
                vkd3d_desc.miplevel_count = desc->u.TextureCube.MipLevels;
                vkd3d_desc.layer_count = 6;
                if (desc->u.TextureCube.ResourceMinLODClamp)
                    FIXME("Unhandled min LOD clamp %.8e.\n", desc->u.TextureCube.ResourceMinLODClamp);
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                vkd3d_desc.miplevel_idx = desc->u.TextureCubeArray.MostDetailedMip;
                vkd3d_desc.miplevel_count = desc->u.TextureCubeArray.MipLevels;
                vkd3d_desc.layer_idx = desc->u.TextureCubeArray.First2DArrayFace;
                vkd3d_desc.layer_count = desc->u.TextureCubeArray.NumCubes;
                if (vkd3d_desc.layer_count != UINT_MAX)
                    vkd3d_desc.layer_count *= 6;
                if (desc->u.TextureCubeArray.ResourceMinLODClamp)
                    FIXME("Unhandled min LOD clamp %.8e.\n", desc->u.TextureCubeArray.ResourceMinLODClamp);
                vkd3d_texture_view_desc_normalise(&vkd3d_desc, &resource->desc);
                break;
            default:
                FIXME("Unhandled view dimension %#x.\n", desc->ViewDimension);
        }
    }

    vkd3d_create_texture_view(device, VKD3D_DESCRIPTOR_MAGIC_SRV, resource->u.vk_image, &vkd3d_desc,
            &descriptor->s.u.view);
}

static unsigned int vkd3d_view_flags_from_d3d12_buffer_uav_flags(D3D12_BUFFER_UAV_FLAGS flags)
{
    if (flags == D3D12_BUFFER_UAV_FLAG_RAW)
        return VKD3D_VIEW_RAW_BUFFER;
    if (flags)
        FIXME("Unhandled buffer UAV flags %#x.\n", flags);
    return 0;
}

static void vkd3d_create_null_uav(struct d3d12_desc *descriptor,
        struct d3d12_device *device, const D3D12_UNORDERED_ACCESS_VIEW_DESC *desc)
{
    struct vkd3d_null_resources *null_resources = &device->null_resources;
    struct vkd3d_texture_view_desc vkd3d_desc;
    VkImage vk_image;

    if (!desc)
    {
        WARN("View desc is required for NULL view.\n");
        return;
    }

    switch (desc->ViewDimension)
    {
        case D3D12_UAV_DIMENSION_BUFFER:
            if (!device->vk_info.EXT_robustness2)
                WARN("Creating NULL buffer UAV %#x.\n", desc->Format);

            vkd3d_create_buffer_view(device, VKD3D_DESCRIPTOR_MAGIC_UAV, null_resources->vk_storage_buffer,
                    vkd3d_get_format(device, DXGI_FORMAT_R32_UINT, false),
                    0, VKD3D_NULL_BUFFER_SIZE, &descriptor->s.u.view);
            return;

        case D3D12_UAV_DIMENSION_TEXTURE2D:
            vk_image = null_resources->vk_2d_storage_image;
            vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
            vk_image = null_resources->vk_2d_storage_image;
            vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;

        default:
            if (device->vk_info.EXT_robustness2)
            {
                vk_image = VK_NULL_HANDLE;
                /* view_type is not used for Vulkan null descriptors, but make it valid. */
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D;
                break;
            }
            FIXME("Unhandled view dimension %#x.\n", desc->ViewDimension);
            return;
    }

    if (!device->vk_info.EXT_robustness2)
        WARN("Creating NULL UAV %#x.\n", desc->ViewDimension);

    vkd3d_desc.format = vkd3d_get_format(device, VKD3D_NULL_VIEW_FORMAT, false);
    vkd3d_desc.miplevel_idx = 0;
    vkd3d_desc.miplevel_count = 1;
    vkd3d_desc.layer_idx = 0;
    vkd3d_desc.layer_count = 1;
    vkd3d_desc.vk_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    vkd3d_desc.components.r = VK_COMPONENT_SWIZZLE_R;
    vkd3d_desc.components.g = VK_COMPONENT_SWIZZLE_G;
    vkd3d_desc.components.b = VK_COMPONENT_SWIZZLE_B;
    vkd3d_desc.components.a = VK_COMPONENT_SWIZZLE_A;
    vkd3d_desc.allowed_swizzle = false;
    vkd3d_desc.usage = VK_IMAGE_USAGE_STORAGE_BIT;

    vkd3d_create_texture_view(device, VKD3D_DESCRIPTOR_MAGIC_UAV, vk_image, &vkd3d_desc, &descriptor->s.u.view);
}

static void vkd3d_create_buffer_uav(struct d3d12_desc *descriptor, struct d3d12_device *device,
        struct d3d12_resource *resource, struct d3d12_resource *counter_resource,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC *desc)
{
    struct vkd3d_view *view;
    unsigned int flags;

    if (!desc)
    {
        FIXME("Default UAV views not supported.\n");
        return;
    }

    if (desc->ViewDimension != D3D12_UAV_DIMENSION_BUFFER)
    {
        WARN("Unexpected view dimension %#x.\n", desc->ViewDimension);
        return;
    }

    flags = vkd3d_view_flags_from_d3d12_buffer_uav_flags(desc->u.Buffer.Flags);
    if (!vkd3d_create_buffer_view_for_resource(device, VKD3D_DESCRIPTOR_MAGIC_UAV, resource, desc->Format,
            desc->u.Buffer.FirstElement, desc->u.Buffer.NumElements,
            desc->u.Buffer.StructureByteStride, flags, &view))
        return;

    if (counter_resource)
    {
        const struct vkd3d_format *format;

        VKD3D_ASSERT(d3d12_resource_is_buffer(counter_resource));
        VKD3D_ASSERT(desc->u.Buffer.StructureByteStride);

        format = vkd3d_get_format(device, DXGI_FORMAT_R32_UINT, false);
        if (!vkd3d_create_vk_buffer_view(device, counter_resource->u.vk_buffer, format,
                desc->u.Buffer.CounterOffsetInBytes, sizeof(uint32_t), &view->v.vk_counter_view))
        {
            WARN("Failed to create counter buffer view.\n");
            view->v.vk_counter_view = VK_NULL_HANDLE;
            vkd3d_view_decref(view, device);
            return;
        }
    }

    descriptor->s.u.view = view;
}

static void vkd3d_create_texture_uav(struct d3d12_desc *descriptor,
        struct d3d12_device *device, struct d3d12_resource *resource,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC *desc)
{
    struct vkd3d_texture_view_desc vkd3d_desc;

    if (!init_default_texture_view_desc(&vkd3d_desc, resource, desc ? desc->Format : 0))
        return;

    vkd3d_desc.usage = VK_IMAGE_USAGE_STORAGE_BIT;

    if (vkd3d_format_is_compressed(vkd3d_desc.format))
    {
        WARN("UAVs cannot be created for compressed formats.\n");
        return;
    }

    if (desc)
    {
        switch (desc->ViewDimension)
        {
            case D3D12_UAV_DIMENSION_TEXTURE1D:
                vkd3d_desc.miplevel_idx = desc->u.Texture1D.MipSlice;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2D:
                vkd3d_desc.miplevel_idx = desc->u.Texture2D.MipSlice;
                if (desc->u.Texture2D.PlaneSlice)
                    vkd3d_desc.vk_image_aspect = vk_image_aspect_flags_from_d3d12_plane_slice(resource->format,
                            desc->u.Texture2D.PlaneSlice);
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                vkd3d_desc.miplevel_idx = desc->u.Texture2DArray.MipSlice;
                vkd3d_desc.layer_idx = desc->u.Texture2DArray.FirstArraySlice;
                vkd3d_desc.layer_count = desc->u.Texture2DArray.ArraySize;
                if (desc->u.Texture2DArray.PlaneSlice)
                    vkd3d_desc.vk_image_aspect = vk_image_aspect_flags_from_d3d12_plane_slice(resource->format,
                            desc->u.Texture2DArray.PlaneSlice);
                vkd3d_texture_view_desc_normalise(&vkd3d_desc, &resource->desc);
                break;
            case D3D12_UAV_DIMENSION_TEXTURE3D:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_3D;
                vkd3d_desc.miplevel_idx = desc->u.Texture3D.MipSlice;
                if (desc->u.Texture3D.FirstWSlice || (desc->u.Texture3D.WSize != UINT_MAX
                        && desc->u.Texture3D.WSize != max(1u,
                        resource->desc.DepthOrArraySize >> desc->u.Texture3D.MipSlice)))
                    FIXME("Unhandled depth view %u-%u.\n",
                            desc->u.Texture3D.FirstWSlice, desc->u.Texture3D.WSize);
                break;
            default:
                FIXME("Unhandled view dimension %#x.\n", desc->ViewDimension);
        }
    }

    vkd3d_create_texture_view(device, VKD3D_DESCRIPTOR_MAGIC_UAV, resource->u.vk_image, &vkd3d_desc,
            &descriptor->s.u.view);
}

void d3d12_desc_create_uav(struct d3d12_desc *descriptor, struct d3d12_device *device,
        struct d3d12_resource *resource, struct d3d12_resource *counter_resource,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC *desc)
{
    if (!resource)
    {
        if (counter_resource)
            FIXME("Ignoring counter resource %p.\n", counter_resource);
        vkd3d_create_null_uav(descriptor, device, desc);
        return;
    }

    if (d3d12_resource_is_buffer(resource))
    {
        vkd3d_create_buffer_uav(descriptor, device, resource, counter_resource, desc);
    }
    else
    {
        if (counter_resource)
            FIXME("Unexpected counter resource for texture view.\n");
        vkd3d_create_texture_uav(descriptor, device, resource, desc);
    }
}

bool vkd3d_create_raw_buffer_view(struct d3d12_device *device,
        D3D12_GPU_VIRTUAL_ADDRESS gpu_address, D3D12_ROOT_PARAMETER_TYPE parameter_type, VkBufferView *vk_buffer_view)
{
    const struct vkd3d_format *format;
    struct d3d12_resource *resource;

    format = vkd3d_get_format(device, DXGI_FORMAT_R32_UINT, false);

    if (!gpu_address)
    {
        if (device->vk_info.EXT_robustness2)
        {
            *vk_buffer_view = VK_NULL_HANDLE;
            return true;
        }
        WARN("Creating null buffer view.\n");
        return vkd3d_create_vk_buffer_view(device, parameter_type == D3D12_ROOT_PARAMETER_TYPE_UAV
                ? device->null_resources.vk_storage_buffer : device->null_resources.vk_buffer,
                format, 0, VK_WHOLE_SIZE, vk_buffer_view);
    }

    resource = vkd3d_gpu_va_allocator_dereference(&device->gpu_va_allocator, gpu_address);
    VKD3D_ASSERT(d3d12_resource_is_buffer(resource));
    return vkd3d_create_vk_buffer_view(device, resource->u.vk_buffer, format,
            gpu_address - resource->gpu_address, VK_WHOLE_SIZE, vk_buffer_view);
}

/* samplers */
static VkFilter vk_filter_from_d3d12(D3D12_FILTER_TYPE type)
{
    switch (type)
    {
        case D3D12_FILTER_TYPE_POINT:
            return VK_FILTER_NEAREST;
        case D3D12_FILTER_TYPE_LINEAR:
            return VK_FILTER_LINEAR;
        default:
            FIXME("Unhandled filter type %#x.\n", type);
            return VK_FILTER_NEAREST;
    }
}

static VkSamplerMipmapMode vk_mipmap_mode_from_d3d12(D3D12_FILTER_TYPE type)
{
    switch (type)
    {
        case D3D12_FILTER_TYPE_POINT:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case D3D12_FILTER_TYPE_LINEAR:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        default:
            FIXME("Unhandled filter type %#x.\n", type);
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
}

static VkSamplerAddressMode vk_address_mode_from_d3d12(const struct d3d12_device *device,
        D3D12_TEXTURE_ADDRESS_MODE mode)
{
    switch (mode)
    {
        case D3D12_TEXTURE_ADDRESS_MODE_WRAP:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case D3D12_TEXTURE_ADDRESS_MODE_CLAMP:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case D3D12_TEXTURE_ADDRESS_MODE_BORDER:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE:
            if (device->vk_info.KHR_sampler_mirror_clamp_to_edge)
                return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
            /* Fall through */
        default:
            FIXME("Unhandled address mode %#x.\n", mode);
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

static VkBorderColor vk_border_colour_from_d3d12(D3D12_STATIC_BORDER_COLOR colour)
{
    switch (colour)
    {
        case D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK:
            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        case D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        case D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        default:
            FIXME("Unhandled border colour %#x.\n", colour);
            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    }
}

static VkResult d3d12_create_sampler(struct d3d12_device *device, D3D12_FILTER filter,
        D3D12_TEXTURE_ADDRESS_MODE address_u, D3D12_TEXTURE_ADDRESS_MODE address_v,
        D3D12_TEXTURE_ADDRESS_MODE address_w, float mip_lod_bias, unsigned int max_anisotropy,
        D3D12_COMPARISON_FUNC comparison_func, D3D12_STATIC_BORDER_COLOR border_colour,
        float min_lod, float max_lod, VkSampler *vk_sampler)
{
    const struct vkd3d_vk_device_procs *vk_procs;
    struct VkSamplerCreateInfo sampler_desc;
    VkResult vr;

    vk_procs = &device->vk_procs;

    if (D3D12_DECODE_FILTER_REDUCTION(filter) == D3D12_FILTER_REDUCTION_TYPE_MINIMUM
            || D3D12_DECODE_FILTER_REDUCTION(filter) == D3D12_FILTER_REDUCTION_TYPE_MAXIMUM)
        FIXME("Min/max reduction mode not supported.\n");

    sampler_desc.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_desc.pNext = NULL;
    sampler_desc.flags = 0;
    sampler_desc.magFilter = vk_filter_from_d3d12(D3D12_DECODE_MAG_FILTER(filter));
    sampler_desc.minFilter = vk_filter_from_d3d12(D3D12_DECODE_MIN_FILTER(filter));
    sampler_desc.mipmapMode = vk_mipmap_mode_from_d3d12(D3D12_DECODE_MIP_FILTER(filter));
    sampler_desc.addressModeU = vk_address_mode_from_d3d12(device, address_u);
    sampler_desc.addressModeV = vk_address_mode_from_d3d12(device, address_v);
    sampler_desc.addressModeW = vk_address_mode_from_d3d12(device, address_w);
    sampler_desc.mipLodBias = mip_lod_bias;
    sampler_desc.anisotropyEnable = D3D12_DECODE_IS_ANISOTROPIC_FILTER(filter);
    sampler_desc.maxAnisotropy = max_anisotropy;
    sampler_desc.compareEnable = D3D12_DECODE_IS_COMPARISON_FILTER(filter);
    sampler_desc.compareOp = sampler_desc.compareEnable ? vk_compare_op_from_d3d12(comparison_func) : 0;
    sampler_desc.minLod = min_lod;
    sampler_desc.maxLod = max_lod;
    sampler_desc.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_desc.unnormalizedCoordinates = VK_FALSE;

    if (address_u == D3D12_TEXTURE_ADDRESS_MODE_BORDER || address_v == D3D12_TEXTURE_ADDRESS_MODE_BORDER
            || address_w == D3D12_TEXTURE_ADDRESS_MODE_BORDER)
        sampler_desc.borderColor = vk_border_colour_from_d3d12(border_colour);

    if ((vr = VK_CALL(vkCreateSampler(device->vk_device, &sampler_desc, NULL, vk_sampler))) < 0)
        WARN("Failed to create Vulkan sampler, vr %d.\n", vr);

    return vr;
}

static D3D12_STATIC_BORDER_COLOR d3d12_static_border_colour(const float *colour)
{
    unsigned int i;

    static const struct
    {
        float colour[4];
        D3D12_STATIC_BORDER_COLOR static_colour;
    }
    colours[] =
    {
        {{0.0f, 0.0f, 0.0f, 0.0f}, D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK},
        {{0.0f, 0.0f, 0.0f, 1.0f}, D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK},
        {{1.0f, 1.0f, 1.0f, 1.0f}, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE},
    };

    for (i = 0; i < ARRAY_SIZE(colours); ++i)
    {
        if (!memcmp(colour, colours[i].colour, sizeof(colours[i].colour)))
            return colours[i].static_colour;
    }

    FIXME("Unhandled border colour {%.8e, %.8e, %.8e, %.8e}.\n", colour[0], colour[1], colour[2], colour[3]);

    return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
}

void d3d12_desc_create_sampler(struct d3d12_desc *sampler,
        struct d3d12_device *device, const D3D12_SAMPLER_DESC *desc)
{
    D3D12_STATIC_BORDER_COLOR static_colour = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    struct vkd3d_view *view;

    if (!desc)
    {
        WARN("NULL sampler desc.\n");
        return;
    }

    if (desc->AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER
            || desc->AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER
            || desc->AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER)
        static_colour = d3d12_static_border_colour(desc->BorderColor);

    if (!(view = vkd3d_view_create(VKD3D_DESCRIPTOR_MAGIC_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLER,
            VKD3D_VIEW_TYPE_SAMPLER, device)))
        return;
    view->v.u.vk_sampler = VK_NULL_HANDLE;
    view->v.format = NULL;

    if (d3d12_create_sampler(device, desc->Filter, desc->AddressU, desc->AddressV,
            desc->AddressW, desc->MipLODBias, desc->MaxAnisotropy, desc->ComparisonFunc,
            static_colour, desc->MinLOD, desc->MaxLOD, &view->v.u.vk_sampler) < 0)
    {
        vkd3d_view_decref(view, device);
        return;
    }

    sampler->s.u.view = view;
}

HRESULT vkd3d_create_static_sampler(struct d3d12_device *device,
        const D3D12_STATIC_SAMPLER_DESC *desc, VkSampler *vk_sampler)
{
    VkResult vr;

    vr = d3d12_create_sampler(device, desc->Filter, desc->AddressU, desc->AddressV,
            desc->AddressW, desc->MipLODBias, desc->MaxAnisotropy, desc->ComparisonFunc,
            desc->BorderColor, desc->MinLOD, desc->MaxLOD, vk_sampler);
    return hresult_from_vk_result(vr);
}

/* RTVs */
static void d3d12_rtv_desc_destroy(struct d3d12_rtv_desc *rtv, struct d3d12_device *device)
{
    if (!rtv->view)
        return;

    vkd3d_view_decref(rtv->view, device);
    memset(rtv, 0, sizeof(*rtv));
}

void d3d12_rtv_desc_create_rtv(struct d3d12_rtv_desc *rtv_desc, struct d3d12_device *device,
        struct d3d12_resource *resource, const D3D12_RENDER_TARGET_VIEW_DESC *desc)
{
    struct vkd3d_texture_view_desc vkd3d_desc;
    struct vkd3d_view *view;

    d3d12_rtv_desc_destroy(rtv_desc, device);

    if (!resource)
    {
        FIXME("NULL resource RTV not implemented.\n");
        return;
    }

    if (!init_default_texture_view_desc(&vkd3d_desc, resource, desc ? desc->Format : 0))
        return;

    vkd3d_desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (vkd3d_desc.format->vk_aspect_mask != VK_IMAGE_ASPECT_COLOR_BIT)
    {
        WARN("Trying to create RTV for depth/stencil format %#x.\n", vkd3d_desc.format->dxgi_format);
        return;
    }

    if (desc)
    {
        switch (desc->ViewDimension)
        {
            case D3D12_RTV_DIMENSION_TEXTURE2D:
                vkd3d_desc.miplevel_idx = desc->u.Texture2D.MipSlice;
                if (desc->u.Texture2D.PlaneSlice)
                    vkd3d_desc.vk_image_aspect = vk_image_aspect_flags_from_d3d12_plane_slice(resource->format,
                            desc->u.Texture2D.PlaneSlice);
                break;
            case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                vkd3d_desc.miplevel_idx = desc->u.Texture2DArray.MipSlice;
                vkd3d_desc.layer_idx = desc->u.Texture2DArray.FirstArraySlice;
                vkd3d_desc.layer_count = desc->u.Texture2DArray.ArraySize;
                if (desc->u.Texture2DArray.PlaneSlice)
                    vkd3d_desc.vk_image_aspect = vk_image_aspect_flags_from_d3d12_plane_slice(resource->format,
                            desc->u.Texture2DArray.PlaneSlice);
                vkd3d_texture_view_desc_normalise(&vkd3d_desc, &resource->desc);
                break;
            case D3D12_RTV_DIMENSION_TEXTURE2DMS:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D;
                break;
            case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                vkd3d_desc.layer_idx = desc->u.Texture2DMSArray.FirstArraySlice;
                vkd3d_desc.layer_count = desc->u.Texture2DMSArray.ArraySize;
                vkd3d_texture_view_desc_normalise(&vkd3d_desc, &resource->desc);
                break;
            case D3D12_RTV_DIMENSION_TEXTURE3D:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                vkd3d_desc.miplevel_idx = desc->u.Texture3D.MipSlice;
                vkd3d_desc.layer_idx = desc->u.Texture3D.FirstWSlice;
                vkd3d_desc.layer_count = desc->u.Texture3D.WSize;
                vkd3d_texture_view_desc_normalise(&vkd3d_desc, &resource->desc);
                break;
            default:
                FIXME("Unhandled view dimension %#x.\n", desc->ViewDimension);
        }
    }
    else if (resource->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        vkd3d_desc.layer_idx = 0;
        vkd3d_desc.layer_count = resource->desc.DepthOrArraySize;
    }

    VKD3D_ASSERT(d3d12_resource_is_texture(resource));

    if (!vkd3d_create_texture_view(device, VKD3D_DESCRIPTOR_MAGIC_RTV, resource->u.vk_image, &vkd3d_desc, &view))
        return;

    rtv_desc->sample_count = vk_samples_from_dxgi_sample_desc(&resource->desc.SampleDesc);
    rtv_desc->format = vkd3d_desc.format;
    rtv_desc->width = d3d12_resource_desc_get_width(&resource->desc, vkd3d_desc.miplevel_idx);
    rtv_desc->height = d3d12_resource_desc_get_height(&resource->desc, vkd3d_desc.miplevel_idx);
    rtv_desc->layer_count = vkd3d_desc.layer_count;
    rtv_desc->view = view;
    rtv_desc->resource = resource;
}

/* DSVs */
static void d3d12_dsv_desc_destroy(struct d3d12_dsv_desc *dsv, struct d3d12_device *device)
{
    if (!dsv->view)
        return;

    vkd3d_view_decref(dsv->view, device);
    memset(dsv, 0, sizeof(*dsv));
}

void d3d12_dsv_desc_create_dsv(struct d3d12_dsv_desc *dsv_desc, struct d3d12_device *device,
        struct d3d12_resource *resource, const D3D12_DEPTH_STENCIL_VIEW_DESC *desc)
{
    struct vkd3d_texture_view_desc vkd3d_desc;
    struct vkd3d_view *view;

    d3d12_dsv_desc_destroy(dsv_desc, device);

    if (!resource)
    {
        FIXME("NULL resource DSV not implemented.\n");
        return;
    }

    if (resource->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        WARN("Cannot create DSV for 3D texture.\n");
        return;
    }

    if (!init_default_texture_view_desc(&vkd3d_desc, resource, desc ? desc->Format : 0))
        return;

    vkd3d_desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    if (!(vkd3d_desc.format->vk_aspect_mask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)))
    {
        WARN("Trying to create DSV for format %#x.\n", vkd3d_desc.format->dxgi_format);
        return;
    }

    if (desc)
    {
        if (desc->Flags)
            FIXME("Ignoring flags %#x.\n", desc->Flags);

        switch (desc->ViewDimension)
        {
            case D3D12_DSV_DIMENSION_TEXTURE2D:
                vkd3d_desc.miplevel_idx = desc->u.Texture2D.MipSlice;
                break;
            case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                vkd3d_desc.miplevel_idx = desc->u.Texture2DArray.MipSlice;
                vkd3d_desc.layer_idx = desc->u.Texture2DArray.FirstArraySlice;
                vkd3d_desc.layer_count = desc->u.Texture2DArray.ArraySize;
                vkd3d_texture_view_desc_normalise(&vkd3d_desc, &resource->desc);
                break;
            case D3D12_DSV_DIMENSION_TEXTURE2DMS:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D;
                break;
            case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
                vkd3d_desc.view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                vkd3d_desc.layer_idx = desc->u.Texture2DMSArray.FirstArraySlice;
                vkd3d_desc.layer_count = desc->u.Texture2DMSArray.ArraySize;
                vkd3d_texture_view_desc_normalise(&vkd3d_desc, &resource->desc);
                break;
            default:
                FIXME("Unhandled view dimension %#x.\n", desc->ViewDimension);
        }
    }

    VKD3D_ASSERT(d3d12_resource_is_texture(resource));

    if (!vkd3d_create_texture_view(device, VKD3D_DESCRIPTOR_MAGIC_DSV, resource->u.vk_image, &vkd3d_desc, &view))
        return;

    dsv_desc->sample_count = vk_samples_from_dxgi_sample_desc(&resource->desc.SampleDesc);
    dsv_desc->format = vkd3d_desc.format;
    dsv_desc->width = d3d12_resource_desc_get_width(&resource->desc, vkd3d_desc.miplevel_idx);
    dsv_desc->height = d3d12_resource_desc_get_height(&resource->desc, vkd3d_desc.miplevel_idx);
    dsv_desc->layer_count = vkd3d_desc.layer_count;
    dsv_desc->view = view;
    dsv_desc->resource = resource;
}

/* ID3D12DescriptorHeap */
static inline struct d3d12_descriptor_heap *impl_from_ID3D12DescriptorHeap(ID3D12DescriptorHeap *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_descriptor_heap, ID3D12DescriptorHeap_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_descriptor_heap_QueryInterface(ID3D12DescriptorHeap *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D12DescriptorHeap)
            || IsEqualGUID(riid, &IID_ID3D12Pageable)
            || IsEqualGUID(riid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(riid, &IID_ID3D12Object)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D12DescriptorHeap_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_descriptor_heap_AddRef(ID3D12DescriptorHeap *iface)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&heap->refcount);

    TRACE("%p increasing refcount to %u.\n", heap, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d12_descriptor_heap_Release(ID3D12DescriptorHeap *iface)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&heap->refcount);

    TRACE("%p decreasing refcount to %u.\n", heap, refcount);

    if (!refcount)
    {
        const struct vkd3d_vk_device_procs *vk_procs;
        struct d3d12_device *device = heap->device;
        unsigned int i;

        vk_procs = &device->vk_procs;

        vkd3d_private_store_destroy(&heap->private_store);

        switch (heap->desc.Type)
        {
            case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            {
                struct d3d12_desc *descriptors = (struct d3d12_desc *)heap->descriptors;

                if (heap->use_vk_heaps)
                    d3d12_device_remove_descriptor_heap(device, heap);

                for (i = 0; i < heap->desc.NumDescriptors; ++i)
                {
                    d3d12_desc_destroy(&descriptors[i], device);
                }

                break;
            }

            case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            {
                struct d3d12_rtv_desc *rtvs = (struct d3d12_rtv_desc *)heap->descriptors;

                for (i = 0; i < heap->desc.NumDescriptors; ++i)
                {
                    d3d12_rtv_desc_destroy(&rtvs[i], device);
                }
                break;
            }

            case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            {
                struct d3d12_dsv_desc *dsvs = (struct d3d12_dsv_desc *)heap->descriptors;

                for (i = 0; i < heap->desc.NumDescriptors; ++i)
                {
                    d3d12_dsv_desc_destroy(&dsvs[i], device);
                }
                break;
            }

            default:
                break;
        }

        VK_CALL(vkDestroyDescriptorPool(device->vk_device, heap->vk_descriptor_pool, NULL));
        vkd3d_mutex_destroy(&heap->vk_sets_mutex);

        vkd3d_free(heap);

        d3d12_device_release(device);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_descriptor_heap_GetPrivateData(ID3D12DescriptorHeap *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&heap->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_descriptor_heap_SetPrivateData(ID3D12DescriptorHeap *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&heap->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_descriptor_heap_SetPrivateDataInterface(ID3D12DescriptorHeap *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&heap->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_descriptor_heap_SetName(ID3D12DescriptorHeap *iface, const WCHAR *name)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, heap->device->wchar_size));

    return name ? S_OK : E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE d3d12_descriptor_heap_GetDevice(ID3D12DescriptorHeap *iface, REFIID iid, void **device)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(heap->device, iid, device);
}

static D3D12_DESCRIPTOR_HEAP_DESC * STDMETHODCALLTYPE d3d12_descriptor_heap_GetDesc(ID3D12DescriptorHeap *iface,
        D3D12_DESCRIPTOR_HEAP_DESC *desc)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = heap->desc;
    return desc;
}

static D3D12_CPU_DESCRIPTOR_HANDLE * STDMETHODCALLTYPE d3d12_descriptor_heap_GetCPUDescriptorHandleForHeapStart(
        ID3D12DescriptorHeap *iface, D3D12_CPU_DESCRIPTOR_HANDLE *descriptor)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);

    TRACE("iface %p, descriptor %p.\n", iface, descriptor);

    descriptor->ptr = (SIZE_T)heap->descriptors;

    return descriptor;
}

static D3D12_GPU_DESCRIPTOR_HANDLE * STDMETHODCALLTYPE d3d12_descriptor_heap_GetGPUDescriptorHandleForHeapStart(
        ID3D12DescriptorHeap *iface, D3D12_GPU_DESCRIPTOR_HANDLE *descriptor)
{
    struct d3d12_descriptor_heap *heap = impl_from_ID3D12DescriptorHeap(iface);

    TRACE("iface %p, descriptor %p.\n", iface, descriptor);

    if (heap->desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        descriptor->ptr = (uint64_t)(intptr_t)heap->descriptors;
    }
    else
    {
        WARN("Heap %p is not shader-visible.\n", iface);
        descriptor->ptr = 0;
    }

    return descriptor;
}

static const struct ID3D12DescriptorHeapVtbl d3d12_descriptor_heap_vtbl =
{
    /* IUnknown methods */
    d3d12_descriptor_heap_QueryInterface,
    d3d12_descriptor_heap_AddRef,
    d3d12_descriptor_heap_Release,
    /* ID3D12Object methods */
    d3d12_descriptor_heap_GetPrivateData,
    d3d12_descriptor_heap_SetPrivateData,
    d3d12_descriptor_heap_SetPrivateDataInterface,
    d3d12_descriptor_heap_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_descriptor_heap_GetDevice,
    /* ID3D12DescriptorHeap methods */
    d3d12_descriptor_heap_GetDesc,
    d3d12_descriptor_heap_GetCPUDescriptorHandleForHeapStart,
    d3d12_descriptor_heap_GetGPUDescriptorHandleForHeapStart,
};

const enum vkd3d_vk_descriptor_set_index vk_descriptor_set_index_table[] =
{
    VKD3D_SET_INDEX_SAMPLER,
    VKD3D_SET_INDEX_COUNT,
    VKD3D_SET_INDEX_SAMPLED_IMAGE,
    VKD3D_SET_INDEX_STORAGE_IMAGE,
    VKD3D_SET_INDEX_UNIFORM_TEXEL_BUFFER,
    VKD3D_SET_INDEX_STORAGE_TEXEL_BUFFER,
    VKD3D_SET_INDEX_UNIFORM_BUFFER,
};

static HRESULT d3d12_descriptor_heap_create_descriptor_pool(struct d3d12_descriptor_heap *descriptor_heap,
        struct d3d12_device *device, const D3D12_DESCRIPTOR_HEAP_DESC *desc)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkDescriptorPoolSize pool_sizes[VKD3D_SET_INDEX_COUNT];
    struct VkDescriptorPoolCreateInfo pool_desc;
    VkDevice vk_device = device->vk_device;
    enum vkd3d_vk_descriptor_set_index set;
    VkResult vr;

    for (set = 0, pool_desc.poolSizeCount = 0; set < ARRAY_SIZE(device->vk_descriptor_heap_layouts); ++set)
    {
        if (device->vk_descriptor_heap_layouts[set].applicable_heap_type == desc->Type
                && device->vk_descriptor_heap_layouts[set].vk_set_layout)
        {
            pool_sizes[pool_desc.poolSizeCount].type =
                    (device->vk_info.EXT_mutable_descriptor_type && set == VKD3D_SET_INDEX_MUTABLE)
                    ? VK_DESCRIPTOR_TYPE_MUTABLE_EXT : device->vk_descriptor_heap_layouts[set].type;
            pool_sizes[pool_desc.poolSizeCount++].descriptorCount = desc->NumDescriptors;
        }
    }

    pool_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_desc.pNext = NULL;
    pool_desc.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    pool_desc.maxSets = pool_desc.poolSizeCount;
    pool_desc.pPoolSizes = pool_sizes;
    if ((vr = VK_CALL(vkCreateDescriptorPool(vk_device, &pool_desc, NULL, &descriptor_heap->vk_descriptor_pool))) < 0)
        ERR("Failed to create descriptor pool, vr %d.\n", vr);

    return hresult_from_vk_result(vr);
}

static HRESULT d3d12_descriptor_heap_create_descriptor_set(struct d3d12_descriptor_heap *descriptor_heap,
        struct d3d12_device *device, unsigned int set)
{
    struct d3d12_descriptor_heap_vk_set *descriptor_set = &descriptor_heap->vk_descriptor_sets[set];
    uint32_t variable_binding_size = descriptor_heap->desc.NumDescriptors;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT set_size;
    VkDescriptorSetAllocateInfo set_desc;
    VkResult vr;
    HRESULT hr;

    if (!device->vk_descriptor_heap_layouts[set].vk_set_layout)
    {
        /* Mutable descriptors are in use, and this set is unused. */
        if (!descriptor_heap->vk_descriptor_sets[VKD3D_SET_INDEX_MUTABLE].vk_set
                && FAILED(hr = d3d12_descriptor_heap_create_descriptor_set(descriptor_heap,
                device, VKD3D_SET_INDEX_MUTABLE)))
            return hr;
        descriptor_set->vk_set = descriptor_heap->vk_descriptor_sets[VKD3D_SET_INDEX_MUTABLE].vk_set;
        descriptor_set->vk_type = device->vk_descriptor_heap_layouts[set].type;
        return S_OK;
    }

    set_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_desc.pNext = &set_size;
    set_desc.descriptorPool = descriptor_heap->vk_descriptor_pool;
    set_desc.descriptorSetCount = 1;
    set_desc.pSetLayouts = &device->vk_descriptor_heap_layouts[set].vk_set_layout;
    set_size.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
    set_size.pNext = NULL;
    set_size.descriptorSetCount = 1;
    set_size.pDescriptorCounts = &variable_binding_size;
    if ((vr = VK_CALL(vkAllocateDescriptorSets(device->vk_device, &set_desc, &descriptor_set->vk_set))) >= 0)
    {
        descriptor_set->vk_type = device->vk_descriptor_heap_layouts[set].type;
        return S_OK;
    }

    ERR("Failed to allocate descriptor set, vr %d.\n", vr);
    return hresult_from_vk_result(vr);
}

static HRESULT d3d12_descriptor_heap_vk_descriptor_sets_init(struct d3d12_descriptor_heap *descriptor_heap,
        struct d3d12_device *device, const D3D12_DESCRIPTOR_HEAP_DESC *desc)
{
    enum vkd3d_vk_descriptor_set_index set;
    HRESULT hr;

    descriptor_heap->vk_descriptor_pool = VK_NULL_HANDLE;
    memset(descriptor_heap->vk_descriptor_sets, 0, sizeof(descriptor_heap->vk_descriptor_sets));

    if (!descriptor_heap->use_vk_heaps || (desc->Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            && desc->Type != D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER))
        return S_OK;

    if (FAILED(hr = d3d12_descriptor_heap_create_descriptor_pool(descriptor_heap, device, desc)))
        return hr;

    for (set = 0; set < ARRAY_SIZE(descriptor_heap->vk_descriptor_sets); ++set)
    {
        if (device->vk_descriptor_heap_layouts[set].applicable_heap_type == desc->Type
                && FAILED(hr = d3d12_descriptor_heap_create_descriptor_set(descriptor_heap, device, set)))
            return hr;
    }

    return S_OK;
}

static HRESULT d3d12_descriptor_heap_init(struct d3d12_descriptor_heap *descriptor_heap,
        struct d3d12_device *device, const D3D12_DESCRIPTOR_HEAP_DESC *desc)
{
    HRESULT hr;

    descriptor_heap->ID3D12DescriptorHeap_iface.lpVtbl = &d3d12_descriptor_heap_vtbl;
    descriptor_heap->refcount = 1;
    descriptor_heap->serial_id = vkd3d_atomic_increment_u64(&object_global_serial_id);

    descriptor_heap->desc = *desc;

    if (FAILED(hr = vkd3d_private_store_init(&descriptor_heap->private_store)))
        return hr;

    descriptor_heap->use_vk_heaps = device->use_vk_heaps && (desc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    if (FAILED(hr = d3d12_descriptor_heap_vk_descriptor_sets_init(descriptor_heap, device, desc)))
    {
        vkd3d_private_store_destroy(&descriptor_heap->private_store);
        return hr;
    }
    vkd3d_mutex_init(&descriptor_heap->vk_sets_mutex);

    d3d12_device_add_ref(descriptor_heap->device = device);

    return S_OK;
}

HRESULT d3d12_descriptor_heap_create(struct d3d12_device *device,
        const D3D12_DESCRIPTOR_HEAP_DESC *desc, struct d3d12_descriptor_heap **descriptor_heap)
{
    size_t max_descriptor_count, descriptor_size;
    struct d3d12_descriptor_heap *object;
    struct d3d12_desc *dst;
    unsigned int i;
    HRESULT hr;

    if (!(descriptor_size = d3d12_device_get_descriptor_handle_increment_size(device, desc->Type)))
    {
        WARN("No descriptor size for descriptor type %#x.\n", desc->Type);
        return E_INVALIDARG;
    }

    if ((desc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
            && (desc->Type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || desc->Type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV))
    {
        WARN("RTV/DSV descriptor heaps cannot be shader visible.\n");
        return E_INVALIDARG;
    }

    max_descriptor_count = (~(size_t)0 - sizeof(*object)) / descriptor_size;
    if (desc->NumDescriptors > max_descriptor_count)
    {
        WARN("Invalid descriptor count %u (max %zu).\n", desc->NumDescriptors, max_descriptor_count);
        return E_OUTOFMEMORY;
    }

    if (!(object = vkd3d_malloc(offsetof(struct d3d12_descriptor_heap,
            descriptors[descriptor_size * desc->NumDescriptors]))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_descriptor_heap_init(object, device, desc)))
    {
        vkd3d_free(object);
        return hr;
    }

    if (desc->Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || desc->Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    {
        dst = (struct d3d12_desc *)object->descriptors;
        for (i = 0; i < desc->NumDescriptors; ++i)
        {
            memset(&dst[i].s, 0, sizeof(dst[i].s));
            dst[i].index = i;
            dst[i].next = 0;
        }
        object->dirty_list_head = UINT_MAX;

        if (object->use_vk_heaps && FAILED(hr = d3d12_device_add_descriptor_heap(device, object)))
        {
            vkd3d_free(object);
            return hr;
        }
    }
    else
    {
        memset(object->descriptors, 0, descriptor_size * desc->NumDescriptors);
    }

    TRACE("Created descriptor heap %p.\n", object);

    *descriptor_heap = object;

    return S_OK;
}

/* ID3D12QueryHeap */
static inline struct d3d12_query_heap *impl_from_ID3D12QueryHeap(ID3D12QueryHeap *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_query_heap, ID3D12QueryHeap_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_query_heap_QueryInterface(ID3D12QueryHeap *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID3D12QueryHeap)
            || IsEqualGUID(iid, &IID_ID3D12Pageable)
            || IsEqualGUID(iid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(iid, &IID_ID3D12Object)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID3D12QueryHeap_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_query_heap_AddRef(ID3D12QueryHeap *iface)
{
    struct d3d12_query_heap *heap = impl_from_ID3D12QueryHeap(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&heap->refcount);

    TRACE("%p increasing refcount to %u.\n", heap, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d12_query_heap_Release(ID3D12QueryHeap *iface)
{
    struct d3d12_query_heap *heap = impl_from_ID3D12QueryHeap(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&heap->refcount);

    TRACE("%p decreasing refcount to %u.\n", heap, refcount);

    if (!refcount)
    {
        struct d3d12_device *device = heap->device;
        const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

        vkd3d_private_store_destroy(&heap->private_store);

        VK_CALL(vkDestroyQueryPool(device->vk_device, heap->vk_query_pool, NULL));

        vkd3d_free(heap);

        d3d12_device_release(device);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_query_heap_GetPrivateData(ID3D12QueryHeap *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_query_heap *heap = impl_from_ID3D12QueryHeap(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&heap->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_query_heap_SetPrivateData(ID3D12QueryHeap *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_query_heap *heap = impl_from_ID3D12QueryHeap(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&heap->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_query_heap_SetPrivateDataInterface(ID3D12QueryHeap *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_query_heap *heap = impl_from_ID3D12QueryHeap(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&heap->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_query_heap_SetName(ID3D12QueryHeap *iface, const WCHAR *name)
{
    struct d3d12_query_heap *heap = impl_from_ID3D12QueryHeap(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, heap->device->wchar_size));

    return vkd3d_set_vk_object_name(heap->device, (uint64_t)heap->vk_query_pool,
            VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, name);
}

static HRESULT STDMETHODCALLTYPE d3d12_query_heap_GetDevice(ID3D12QueryHeap *iface, REFIID iid, void **device)
{
    struct d3d12_query_heap *heap = impl_from_ID3D12QueryHeap(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(heap->device, iid, device);
}

static const struct ID3D12QueryHeapVtbl d3d12_query_heap_vtbl =
{
    /* IUnknown methods */
    d3d12_query_heap_QueryInterface,
    d3d12_query_heap_AddRef,
    d3d12_query_heap_Release,
    /* ID3D12Object methods */
    d3d12_query_heap_GetPrivateData,
    d3d12_query_heap_SetPrivateData,
    d3d12_query_heap_SetPrivateDataInterface,
    d3d12_query_heap_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_query_heap_GetDevice,
};

struct d3d12_query_heap *unsafe_impl_from_ID3D12QueryHeap(ID3D12QueryHeap *iface)
{
    if (!iface)
        return NULL;
    VKD3D_ASSERT(iface->lpVtbl == &d3d12_query_heap_vtbl);
    return impl_from_ID3D12QueryHeap(iface);
}

HRESULT d3d12_query_heap_create(struct d3d12_device *device, const D3D12_QUERY_HEAP_DESC *desc,
        struct d3d12_query_heap **heap)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct d3d12_query_heap *object;
    VkQueryPoolCreateInfo pool_info;
    unsigned int element_count;
    VkResult vr;
    HRESULT hr;

    element_count = DIV_ROUND_UP(desc->Count, sizeof(*object->availability_mask) * CHAR_BIT);
    if (!(object = vkd3d_malloc(offsetof(struct d3d12_query_heap, availability_mask[element_count]))))
        return E_OUTOFMEMORY;

    object->ID3D12QueryHeap_iface.lpVtbl = &d3d12_query_heap_vtbl;
    object->refcount = 1;
    object->device = device;
    memset(object->availability_mask, 0, element_count * sizeof(*object->availability_mask));

    pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    pool_info.pNext = NULL;
    pool_info.flags = 0;
    pool_info.queryCount = desc->Count;

    switch (desc->Type)
    {
        case D3D12_QUERY_HEAP_TYPE_OCCLUSION:
            pool_info.queryType = VK_QUERY_TYPE_OCCLUSION;
            pool_info.pipelineStatistics = 0;
            break;

        case D3D12_QUERY_HEAP_TYPE_TIMESTAMP:
            pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
            pool_info.pipelineStatistics = 0;
            break;

        case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS:
            pool_info.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
            pool_info.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT
                    | VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
            break;

        case D3D12_QUERY_HEAP_TYPE_SO_STATISTICS:
            if (!device->vk_info.transform_feedback_queries)
            {
                FIXME("Transform feedback queries are not supported by Vulkan implementation.\n");
                vkd3d_free(object);
                return E_NOTIMPL;
            }

            pool_info.queryType = VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT;
            pool_info.pipelineStatistics = 0;
            break;

        default:
            WARN("Invalid query heap type %u.\n", desc->Type);
            vkd3d_free(object);
            return E_INVALIDARG;
    }

    if (FAILED(hr = vkd3d_private_store_init(&object->private_store)))
    {
        vkd3d_free(object);
        return hr;
    }

    if ((vr = VK_CALL(vkCreateQueryPool(device->vk_device, &pool_info, NULL, &object->vk_query_pool))) < 0)
    {
        WARN("Failed to create Vulkan query pool, vr %d.\n", vr);
        vkd3d_private_store_destroy(&object->private_store);
        vkd3d_free(object);
        return hresult_from_vk_result(vr);
    }

    d3d12_device_add_ref(device);

    TRACE("Created query heap %p.\n", object);

    *heap = object;

    return S_OK;
}

static HRESULT vkd3d_init_null_resources_data(struct vkd3d_null_resources *null_resource,
        struct d3d12_device *device)
{
    const bool use_sparse_resources = device->vk_info.sparse_properties.residencyNonResidentStrict;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    static const VkClearColorValue clear_color = {{0}};
    VkCommandBufferAllocateInfo command_buffer_info;
    VkCommandPool vk_command_pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo command_pool_info;
    VkDevice vk_device = device->vk_device;
    VkCommandBufferBeginInfo begin_info;
    VkCommandBuffer vk_command_buffer;
    VkFence vk_fence = VK_NULL_HANDLE;
    VkImageSubresourceRange range;
    VkImageMemoryBarrier barrier;
    VkFenceCreateInfo fence_info;
    struct vkd3d_queue *queue;
    VkSubmitInfo submit_info;
    VkQueue vk_queue;
    VkResult vr;

    queue = d3d12_device_get_vkd3d_queue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = NULL;
    command_pool_info.flags = 0;
    command_pool_info.queueFamilyIndex = queue->vk_family_index;

    if ((vr = VK_CALL(vkCreateCommandPool(vk_device, &command_pool_info, NULL, &vk_command_pool))) < 0)
    {
        WARN("Failed to create Vulkan command pool, vr %d.\n", vr);
        goto done;
    }

    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.pNext = NULL;
    command_buffer_info.commandPool = vk_command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;

    if ((vr = VK_CALL(vkAllocateCommandBuffers(vk_device, &command_buffer_info, &vk_command_buffer))) < 0)
    {
        WARN("Failed to allocate Vulkan command buffer, vr %d.\n", vr);
        goto done;
    }

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = NULL;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = NULL;

    if ((vr = VK_CALL(vkBeginCommandBuffer(vk_command_buffer, &begin_info))) < 0)
    {
        WARN("Failed to begin command buffer, vr %d.\n", vr);
        goto done;
    }

    /* fill buffer */
    VK_CALL(vkCmdFillBuffer(vk_command_buffer, null_resource->vk_buffer, 0, VK_WHOLE_SIZE, 0x00000000));

    if (use_sparse_resources)
    {
        /* transition 2D UAV image */
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = NULL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = null_resource->vk_2d_storage_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VK_CALL(vkCmdPipelineBarrier(vk_command_buffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                0, NULL, 0, NULL, 1, &barrier));
    }
    else
    {
        /* fill UAV buffer */
        VK_CALL(vkCmdFillBuffer(vk_command_buffer,
                null_resource->vk_storage_buffer, 0, VK_WHOLE_SIZE, 0x00000000));

        /* clear 2D UAV image */
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = NULL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = null_resource->vk_2d_storage_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VK_CALL(vkCmdPipelineBarrier(vk_command_buffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, NULL, 0, NULL, 1, &barrier));

        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VK_CALL(vkCmdClearColorImage(vk_command_buffer,
                null_resource->vk_2d_storage_image, VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range));
    }

    /* transition 2D SRV image */
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = NULL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = null_resource->vk_2d_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VK_CALL(vkCmdPipelineBarrier(vk_command_buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
            0, NULL, 0, NULL, 1, &barrier));

    if ((vr = VK_CALL(vkEndCommandBuffer(vk_command_buffer))) < 0)
    {
        WARN("Failed to end command buffer, vr %d.\n", vr);
        goto done;
    }

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = NULL;
    fence_info.flags = 0;

    if ((vr = VK_CALL(vkCreateFence(device->vk_device, &fence_info, NULL, &vk_fence))) < 0)
    {
        WARN("Failed to create Vulkan fence, vr %d.\n", vr);
        goto done;
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk_command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    if (!(vk_queue = vkd3d_queue_acquire(queue)))
    {
        WARN("Failed to acquire queue %p.\n", queue);
        goto done;
    }

    if ((vr = VK_CALL(vkQueueSubmit(vk_queue, 1, &submit_info, vk_fence))) < 0)
        ERR("Failed to submit, vr %d.\n", vr);

    vkd3d_queue_release(queue);

    vr = VK_CALL(vkWaitForFences(device->vk_device, 1, &vk_fence, VK_FALSE, ~(uint64_t)0));
    if (vr != VK_SUCCESS)
        WARN("Failed to wait for fence, vr %d.\n", vr);

done:
    VK_CALL(vkDestroyCommandPool(vk_device, vk_command_pool, NULL));
    VK_CALL(vkDestroyFence(vk_device, vk_fence, NULL));

    return hresult_from_vk_result(vr);
}

HRESULT vkd3d_init_null_resources(struct vkd3d_null_resources *null_resources,
        struct d3d12_device *device)
{
    const bool use_sparse_resources = device->vk_info.sparse_properties.residencyNonResidentStrict;
    D3D12_HEAP_PROPERTIES heap_properties;
    D3D12_RESOURCE_DESC1 resource_desc;
    HRESULT hr;

    TRACE("Creating resources for NULL views.\n");

    memset(null_resources, 0, sizeof(*null_resources));

    if (device->vk_info.EXT_robustness2)
        return S_OK;

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    /* buffer */
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment = 0;
    resource_desc.Width = VKD3D_NULL_BUFFER_SIZE;
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    memset(&resource_desc.SamplerFeedbackMipRegion, 0, sizeof(resource_desc.SamplerFeedbackMipRegion));

    if (FAILED(hr = vkd3d_create_buffer(device, &heap_properties, D3D12_HEAP_FLAG_NONE,
            &resource_desc, &null_resources->vk_buffer)))
        goto fail;
    if (FAILED(hr = vkd3d_allocate_buffer_memory(device, null_resources->vk_buffer,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &null_resources->vk_buffer_memory, NULL, NULL)))
        goto fail;

    /* buffer UAV */
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    if (FAILED(hr = vkd3d_create_buffer(device, use_sparse_resources ? NULL : &heap_properties, D3D12_HEAP_FLAG_NONE,
            &resource_desc, &null_resources->vk_storage_buffer)))
        goto fail;
    if (!use_sparse_resources && FAILED(hr = vkd3d_allocate_buffer_memory(device, null_resources->vk_storage_buffer,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &null_resources->vk_storage_buffer_memory, NULL, NULL)))
        goto fail;

    /* 2D SRV */
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Alignment = 0;
    resource_desc.Width = 1;
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = VKD3D_NULL_VIEW_FORMAT;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (FAILED(hr = vkd3d_create_image(device, &heap_properties, D3D12_HEAP_FLAG_NONE,
            &resource_desc, NULL, &null_resources->vk_2d_image)))
        goto fail;
    if (FAILED(hr = vkd3d_allocate_image_memory(device, null_resources->vk_2d_image,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &null_resources->vk_2d_image_memory, NULL, NULL)))
        goto fail;

    /* 2D UAV */
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Alignment = 0;
    resource_desc.Width = 1;
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = VKD3D_NULL_VIEW_FORMAT;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = use_sparse_resources
            ? D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE : D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    if (FAILED(hr = vkd3d_create_image(device, use_sparse_resources ? NULL : &heap_properties, D3D12_HEAP_FLAG_NONE,
            &resource_desc, NULL, &null_resources->vk_2d_storage_image)))
        goto fail;
    if (!use_sparse_resources && FAILED(hr = vkd3d_allocate_image_memory(device, null_resources->vk_2d_storage_image,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &null_resources->vk_2d_storage_image_memory, NULL, NULL)))
        goto fail;

    /* set Vulkan object names */
    vkd3d_set_vk_object_name_utf8(device, (uint64_t)null_resources->vk_buffer,
            VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "NULL buffer");
    vkd3d_set_vk_object_name_utf8(device, (uint64_t)null_resources->vk_buffer_memory,
            VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, "NULL memory");
    vkd3d_set_vk_object_name_utf8(device, (uint64_t)null_resources->vk_storage_buffer,
            VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "NULL UAV buffer");
    vkd3d_set_vk_object_name_utf8(device, (uint64_t)null_resources->vk_2d_image,
            VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "NULL 2D SRV image");
    vkd3d_set_vk_object_name_utf8(device, (uint64_t)null_resources->vk_2d_image_memory,
            VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, "NULL 2D SRV memory");
    vkd3d_set_vk_object_name_utf8(device, (uint64_t)null_resources->vk_2d_storage_image,
            VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "NULL 2D UAV image");
    if (!use_sparse_resources)
    {
        vkd3d_set_vk_object_name_utf8(device, (uint64_t)null_resources->vk_storage_buffer_memory,
                VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, "NULL UAV buffer memory");
        vkd3d_set_vk_object_name_utf8(device, (uint64_t)null_resources->vk_2d_storage_image_memory,
                VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, "NULL 2D UAV memory");
    }

    return vkd3d_init_null_resources_data(null_resources, device);

fail:
    ERR("Failed to initialise NULL resources, hr %s.\n", debugstr_hresult(hr));
    vkd3d_destroy_null_resources(null_resources, device);
    return hr;
}

void vkd3d_destroy_null_resources(struct vkd3d_null_resources *null_resources,
        struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    VK_CALL(vkDestroyBuffer(device->vk_device, null_resources->vk_buffer, NULL));
    VK_CALL(vkFreeMemory(device->vk_device, null_resources->vk_buffer_memory, NULL));

    VK_CALL(vkDestroyBuffer(device->vk_device, null_resources->vk_storage_buffer, NULL));
    VK_CALL(vkFreeMemory(device->vk_device, null_resources->vk_storage_buffer_memory, NULL));

    VK_CALL(vkDestroyImage(device->vk_device, null_resources->vk_2d_image, NULL));
    VK_CALL(vkFreeMemory(device->vk_device, null_resources->vk_2d_image_memory, NULL));

    VK_CALL(vkDestroyImage(device->vk_device, null_resources->vk_2d_storage_image, NULL));
    VK_CALL(vkFreeMemory(device->vk_device, null_resources->vk_2d_storage_image_memory, NULL));

    memset(null_resources, 0, sizeof(*null_resources));
}
