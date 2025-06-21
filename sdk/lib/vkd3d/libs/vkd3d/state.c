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
#include "vkd3d_shaders.h"
#include "vkd3d_shader_utils.h"

/* ID3D12RootSignature */
static inline struct d3d12_root_signature *impl_from_ID3D12RootSignature(ID3D12RootSignature *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_root_signature, ID3D12RootSignature_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_root_signature_QueryInterface(ID3D12RootSignature *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D12RootSignature)
            || IsEqualGUID(riid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(riid, &IID_ID3D12Object)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D12RootSignature_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_root_signature_AddRef(ID3D12RootSignature *iface)
{
    struct d3d12_root_signature *root_signature = impl_from_ID3D12RootSignature(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&root_signature->refcount);

    TRACE("%p increasing refcount to %u.\n", root_signature, refcount);

    return refcount;
}

static void d3d12_descriptor_set_layout_cleanup(
        struct d3d12_descriptor_set_layout *layout, struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    VK_CALL(vkDestroyDescriptorSetLayout(device->vk_device, layout->vk_layout, NULL));
}

static void d3d12_root_signature_cleanup(struct d3d12_root_signature *root_signature,
        struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    unsigned int i;

    if (root_signature->vk_pipeline_layout)
        VK_CALL(vkDestroyPipelineLayout(device->vk_device, root_signature->vk_pipeline_layout, NULL));
    for (i = 0; i < root_signature->vk_set_count; ++i)
    {
        d3d12_descriptor_set_layout_cleanup(&root_signature->descriptor_set_layouts[i], device);
    }

    if (root_signature->parameters)
    {
        for (i = 0; i < root_signature->parameter_count; ++i)
        {
            if (root_signature->parameters[i].parameter_type == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
                vkd3d_free(root_signature->parameters[i].u.descriptor_table.ranges);
        }
        vkd3d_free(root_signature->parameters);
    }

    if (root_signature->descriptor_mapping)
        vkd3d_free(root_signature->descriptor_mapping);
    vkd3d_free(root_signature->descriptor_offsets);
    vkd3d_free(root_signature->uav_counter_mapping);
    vkd3d_free(root_signature->uav_counter_offsets);
    if (root_signature->root_constants)
        vkd3d_free(root_signature->root_constants);

    for (i = 0; i < root_signature->static_sampler_count; ++i)
    {
        if (root_signature->static_samplers[i])
            VK_CALL(vkDestroySampler(device->vk_device, root_signature->static_samplers[i], NULL));
    }
    if (root_signature->static_samplers)
        vkd3d_free(root_signature->static_samplers);
}

static ULONG STDMETHODCALLTYPE d3d12_root_signature_Release(ID3D12RootSignature *iface)
{
    struct d3d12_root_signature *root_signature = impl_from_ID3D12RootSignature(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&root_signature->refcount);

    TRACE("%p decreasing refcount to %u.\n", root_signature, refcount);

    if (!refcount)
    {
        struct d3d12_device *device = root_signature->device;
        vkd3d_private_store_destroy(&root_signature->private_store);
        d3d12_root_signature_cleanup(root_signature, device);
        vkd3d_free(root_signature);
        d3d12_device_release(device);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_root_signature_GetPrivateData(ID3D12RootSignature *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_root_signature *root_signature = impl_from_ID3D12RootSignature(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&root_signature->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_root_signature_SetPrivateData(ID3D12RootSignature *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_root_signature *root_signature = impl_from_ID3D12RootSignature(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&root_signature->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_root_signature_SetPrivateDataInterface(ID3D12RootSignature *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_root_signature *root_signature = impl_from_ID3D12RootSignature(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&root_signature->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_root_signature_SetName(ID3D12RootSignature *iface, const WCHAR *name)
{
    struct d3d12_root_signature *root_signature = impl_from_ID3D12RootSignature(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, root_signature->device->wchar_size));

    return name ? S_OK : E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE d3d12_root_signature_GetDevice(ID3D12RootSignature *iface,
        REFIID iid, void **device)
{
    struct d3d12_root_signature *root_signature = impl_from_ID3D12RootSignature(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(root_signature->device, iid, device);
}

static const struct ID3D12RootSignatureVtbl d3d12_root_signature_vtbl =
{
    /* IUnknown methods */
    d3d12_root_signature_QueryInterface,
    d3d12_root_signature_AddRef,
    d3d12_root_signature_Release,
    /* ID3D12Object methods */
    d3d12_root_signature_GetPrivateData,
    d3d12_root_signature_SetPrivateData,
    d3d12_root_signature_SetPrivateDataInterface,
    d3d12_root_signature_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_root_signature_GetDevice,
};

struct d3d12_root_signature *unsafe_impl_from_ID3D12RootSignature(ID3D12RootSignature *iface)
{
    if (!iface)
        return NULL;
    VKD3D_ASSERT(iface->lpVtbl == &d3d12_root_signature_vtbl);
    return impl_from_ID3D12RootSignature(iface);
}

static VkShaderStageFlags stage_flags_from_visibility(D3D12_SHADER_VISIBILITY visibility)
{
    switch (visibility)
    {
        case D3D12_SHADER_VISIBILITY_ALL:
            return VK_SHADER_STAGE_ALL;
        case D3D12_SHADER_VISIBILITY_VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case D3D12_SHADER_VISIBILITY_HULL:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case D3D12_SHADER_VISIBILITY_DOMAIN:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case D3D12_SHADER_VISIBILITY_GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case D3D12_SHADER_VISIBILITY_PIXEL:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        default:
            return 0;
    }
}

static VkShaderStageFlags stage_flags_from_vkd3d_shader_visibility(enum vkd3d_shader_visibility visibility)
{
    switch (visibility)
    {
        case VKD3D_SHADER_VISIBILITY_ALL:
            return VK_SHADER_STAGE_ALL;
        case VKD3D_SHADER_VISIBILITY_VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case VKD3D_SHADER_VISIBILITY_HULL:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case VKD3D_SHADER_VISIBILITY_DOMAIN:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case VKD3D_SHADER_VISIBILITY_GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case VKD3D_SHADER_VISIBILITY_PIXEL:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case VKD3D_SHADER_VISIBILITY_COMPUTE:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            FIXME("Unhandled visibility %#x.\n", visibility);
            return VKD3D_SHADER_VISIBILITY_ALL;
    }
}

static enum vkd3d_shader_visibility vkd3d_shader_visibility_from_d3d12(D3D12_SHADER_VISIBILITY visibility)
{
    switch (visibility)
    {
        case D3D12_SHADER_VISIBILITY_ALL:
            return VKD3D_SHADER_VISIBILITY_ALL;
        case D3D12_SHADER_VISIBILITY_VERTEX:
            return VKD3D_SHADER_VISIBILITY_VERTEX;
        case D3D12_SHADER_VISIBILITY_HULL:
            return VKD3D_SHADER_VISIBILITY_HULL;
        case D3D12_SHADER_VISIBILITY_DOMAIN:
            return VKD3D_SHADER_VISIBILITY_DOMAIN;
        case D3D12_SHADER_VISIBILITY_GEOMETRY:
            return VKD3D_SHADER_VISIBILITY_GEOMETRY;
        case D3D12_SHADER_VISIBILITY_PIXEL:
            return VKD3D_SHADER_VISIBILITY_PIXEL;
        default:
            FIXME("Unhandled visibility %#x.\n", visibility);
            return VKD3D_SHADER_VISIBILITY_ALL;
    }
}

static VkDescriptorType vk_descriptor_type_from_vkd3d_descriptor_type(enum vkd3d_shader_descriptor_type type,
        bool is_buffer)
{
    switch (type)
    {
        case VKD3D_SHADER_DESCRIPTOR_TYPE_SRV:
            return is_buffer ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case VKD3D_SHADER_DESCRIPTOR_TYPE_UAV:
            return is_buffer ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case VKD3D_SHADER_DESCRIPTOR_TYPE_CBV:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        default:
            FIXME("Unhandled descriptor range type type %#x.\n", type);
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }
}

static enum vkd3d_shader_descriptor_type vkd3d_descriptor_type_from_d3d12_range_type(
        D3D12_DESCRIPTOR_RANGE_TYPE type)
{
    switch (type)
    {
        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
            return VKD3D_SHADER_DESCRIPTOR_TYPE_SRV;
        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
            return VKD3D_SHADER_DESCRIPTOR_TYPE_UAV;
        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
            return VKD3D_SHADER_DESCRIPTOR_TYPE_CBV;
        case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
            return VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER;
        default:
            FIXME("Unhandled descriptor range type type %#x.\n", type);
            return VKD3D_SHADER_DESCRIPTOR_TYPE_SRV;
    }
}

static enum vkd3d_shader_descriptor_type vkd3d_descriptor_type_from_d3d12_root_parameter_type(
        D3D12_ROOT_PARAMETER_TYPE type)
{
    switch (type)
    {
        case D3D12_ROOT_PARAMETER_TYPE_SRV:
            return VKD3D_SHADER_DESCRIPTOR_TYPE_SRV;
        case D3D12_ROOT_PARAMETER_TYPE_UAV:
            return VKD3D_SHADER_DESCRIPTOR_TYPE_UAV;
        case D3D12_ROOT_PARAMETER_TYPE_CBV:
            return VKD3D_SHADER_DESCRIPTOR_TYPE_CBV;
        default:
            FIXME("Unhandled descriptor root parameter type %#x.\n", type);
            return VKD3D_SHADER_DESCRIPTOR_TYPE_SRV;
    }
}

struct d3d12_root_signature_info
{
    size_t binding_count;
    size_t uav_range_count;

    size_t root_constant_count;
    size_t root_descriptor_count;

    unsigned int cbv_count;
    unsigned int srv_count;
    unsigned int uav_count;
    unsigned int sampler_count;
    unsigned int cbv_unbounded_range_count;
    unsigned int srv_unbounded_range_count;
    unsigned int uav_unbounded_range_count;
    unsigned int sampler_unbounded_range_count;

    size_t cost;

    struct d3d12_root_signature_info_range
    {
        enum vkd3d_shader_descriptor_type type;
        unsigned int space;
        unsigned int base_idx;
        unsigned int count;
        D3D12_SHADER_VISIBILITY visibility;
    } *ranges;
    size_t range_count, range_capacity;
};

static HRESULT d3d12_root_signature_info_add_range(struct d3d12_root_signature_info *info,
        enum vkd3d_shader_descriptor_type type,  D3D12_SHADER_VISIBILITY visibility,
        unsigned int space, unsigned int base_idx, unsigned int count)
{
    struct d3d12_root_signature_info_range *range;

    if (!vkd3d_array_reserve((void **)&info->ranges, &info->range_capacity, info->range_count + 1,
            sizeof(*info->ranges)))
        return E_OUTOFMEMORY;

    range = &info->ranges[info->range_count++];
    range->type = type;
    range->space = space;
    range->base_idx = base_idx;
    range->count = count;
    range->visibility = visibility;

    return S_OK;
}

static int d3d12_root_signature_info_range_compare(const void *a, const void *b)
{
    const struct d3d12_root_signature_info_range *range_a = a, *range_b = b;
    int ret;

    if ((ret = vkd3d_u32_compare(range_a->type, range_b->type)))
        return ret;

    if ((ret = vkd3d_u32_compare(range_a->space, range_b->space)))
        return ret;

    return vkd3d_u32_compare(range_a->base_idx, range_b->base_idx);
}

static HRESULT d3d12_root_signature_info_range_validate(const struct d3d12_root_signature_info_range *ranges,
        unsigned int count, D3D12_SHADER_VISIBILITY visibility)
{
    const struct d3d12_root_signature_info_range *range, *next;
    unsigned int i = 0, j;

    while (i < count)
    {
        range = &ranges[i];

        for (j = i + 1; j < count; ++j)
        {
            next = &ranges[j];

            if (range->visibility != D3D12_SHADER_VISIBILITY_ALL
                    && next->visibility != D3D12_SHADER_VISIBILITY_ALL
                    && range->visibility != next->visibility)
                continue;

            if (range->type == next->type && range->space == next->space
                    && range->base_idx + range->count > next->base_idx)
                return E_INVALIDARG;

            break;
        }

        i = j;
    }

    return S_OK;
}

static HRESULT d3d12_root_signature_info_count_descriptors(struct d3d12_root_signature_info *info,
        const D3D12_ROOT_PARAMETER *param, bool use_array)
{
    bool cbv_unbounded_range = false, srv_unbounded_range = false, uav_unbounded_range = false;
    const D3D12_ROOT_DESCRIPTOR_TABLE *table = &param->u.DescriptorTable;
    bool sampler_unbounded_range = false;
    bool unbounded = false;
    unsigned int i, count;
    HRESULT hr;

    for (i = 0; i < table->NumDescriptorRanges; ++i)
    {
        const D3D12_DESCRIPTOR_RANGE *range = &table->pDescriptorRanges[i];
        unsigned int binding_count;

        if (!range->NumDescriptors)
        {
            WARN("A descriptor range is empty.\n");
            return E_INVALIDARG;
        }

        if (range->NumDescriptors != UINT_MAX && !vkd3d_bound_range(range->BaseShaderRegister,
                range->NumDescriptors, UINT_MAX))
        {
            WARN("A descriptor range overflows.\n");
            return E_INVALIDARG;
        }

        if (unbounded && range->OffsetInDescriptorsFromTableStart == D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
        {
            WARN("A range with offset D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND occurs after "
                    "an unbounded range.\n");
            return E_INVALIDARG;
        }

        count = range->NumDescriptors;

        if (FAILED(hr = d3d12_root_signature_info_add_range(info,
                vkd3d_descriptor_type_from_d3d12_range_type(range->RangeType),
                param->ShaderVisibility, range->RegisterSpace, range->BaseShaderRegister, count)))
            return hr;

        if (range->NumDescriptors == UINT_MAX)
        {
            unbounded = true;
            count = 0;
        }

        binding_count = use_array ? 1 : range->NumDescriptors;

        switch (range->RangeType)
        {
            case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                /* XXX: Vulkan buffer and image descriptors have different types. In order
                * to preserve compatibility between Vulkan resource bindings for the same
                * root signature, we create descriptor set layouts with two bindings for
                * each SRV and UAV. */
                info->binding_count += binding_count;
                info->srv_count += count * 2u;
                srv_unbounded_range |= unbounded;
                break;
            case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                /* As above. */
                info->binding_count += binding_count;
                info->uav_count += count * 2u;
                uav_unbounded_range |= unbounded;
                ++info->uav_range_count;
                break;
            case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                info->cbv_count += count;
                cbv_unbounded_range |= unbounded;
                break;
            case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                info->sampler_count += count;
                sampler_unbounded_range |= unbounded;
                break;
            default:
                FIXME("Unhandled descriptor type %#x.\n", range->RangeType);
                return E_NOTIMPL;
        }

        info->binding_count += binding_count;
    }

    if (unbounded && !use_array)
    {
        FIXME("The device does not support unbounded descriptor ranges.\n");
        return E_FAIL;
    }

    info->srv_unbounded_range_count += srv_unbounded_range * 2u;
    info->uav_unbounded_range_count += uav_unbounded_range * 2u;
    info->cbv_unbounded_range_count += cbv_unbounded_range;
    info->sampler_unbounded_range_count += sampler_unbounded_range;

    return S_OK;
}

static HRESULT d3d12_root_signature_info_from_desc(struct d3d12_root_signature_info *info,
        const D3D12_ROOT_SIGNATURE_DESC *desc, bool use_array)
{
    unsigned int i;
    HRESULT hr;

    memset(info, 0, sizeof(*info));

    for (i = 0; i < desc->NumParameters; ++i)
    {
        const D3D12_ROOT_PARAMETER *p = &desc->pParameters[i];

        switch (p->ParameterType)
        {
            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                if (FAILED(hr = d3d12_root_signature_info_count_descriptors(info,
                        p, use_array)))
                    goto done;
                ++info->cost;
                break;

            case D3D12_ROOT_PARAMETER_TYPE_CBV:
                ++info->root_descriptor_count;
                ++info->cbv_count;
                ++info->binding_count;
                info->cost += 2;
                if (FAILED(hr = d3d12_root_signature_info_add_range(info,
                        VKD3D_SHADER_DESCRIPTOR_TYPE_CBV, p->ShaderVisibility,
                        p->u.Descriptor.RegisterSpace, p->u.Descriptor.ShaderRegister, 1)))
                    goto done;
                break;

            case D3D12_ROOT_PARAMETER_TYPE_SRV:
                ++info->root_descriptor_count;
                ++info->srv_count;
                ++info->binding_count;
                info->cost += 2;
                if (FAILED(hr = d3d12_root_signature_info_add_range(info,
                        VKD3D_SHADER_DESCRIPTOR_TYPE_SRV, p->ShaderVisibility,
                        p->u.Descriptor.RegisterSpace, p->u.Descriptor.ShaderRegister, 1)))
                    goto done;
                break;

            case D3D12_ROOT_PARAMETER_TYPE_UAV:
                ++info->root_descriptor_count;
                ++info->uav_count;
                ++info->binding_count;
                info->cost += 2;
                if (FAILED(hr = d3d12_root_signature_info_add_range(info,
                        VKD3D_SHADER_DESCRIPTOR_TYPE_UAV, p->ShaderVisibility,
                        p->u.Descriptor.RegisterSpace, p->u.Descriptor.ShaderRegister, 1)))
                    goto done;
                break;

            case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                ++info->root_constant_count;
                info->cost += p->u.Constants.Num32BitValues;
                if (FAILED(hr = d3d12_root_signature_info_add_range(info,
                        VKD3D_SHADER_DESCRIPTOR_TYPE_CBV, p->ShaderVisibility,
                        p->u.Constants.RegisterSpace, p->u.Constants.ShaderRegister, 1)))
                    goto done;
                break;

            default:
                FIXME("Unhandled type %#x for parameter %u.\n", p->ParameterType, i);
                hr = E_NOTIMPL;
                goto done;
        }
    }

    info->binding_count += desc->NumStaticSamplers;
    info->sampler_count += desc->NumStaticSamplers;

    for (i = 0; i < desc->NumStaticSamplers; ++i)
    {
        const D3D12_STATIC_SAMPLER_DESC *s = &desc->pStaticSamplers[i];

        if (FAILED(hr = d3d12_root_signature_info_add_range(info,
                VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, s->ShaderVisibility,
                s->RegisterSpace, s->ShaderRegister, 1)))
            goto done;
    }

    qsort(info->ranges, info->range_count, sizeof(*info->ranges),
            d3d12_root_signature_info_range_compare);

    for (i = D3D12_SHADER_VISIBILITY_VERTEX; i <= D3D12_SHADER_VISIBILITY_MESH; ++i)
    {
        if (FAILED(hr = d3d12_root_signature_info_range_validate(info->ranges, info->range_count, i)))
            goto done;
    }

    hr = S_OK;
done:
    vkd3d_free(info->ranges);
    info->ranges = NULL;
    info->range_count = 0;
    info->range_capacity = 0;

    return hr;
}

static HRESULT d3d12_root_signature_init_push_constants(struct d3d12_root_signature *root_signature,
        const D3D12_ROOT_SIGNATURE_DESC *desc,
        struct VkPushConstantRange push_constants[D3D12_SHADER_VISIBILITY_PIXEL + 1],
        uint32_t *push_constant_range_count)
{
    uint32_t push_constants_offset[D3D12_SHADER_VISIBILITY_PIXEL + 1];
    bool use_vk_heaps = root_signature->device->use_vk_heaps;
    unsigned int i, j, push_constant_count;
    uint32_t offset;

    memset(push_constants, 0, (D3D12_SHADER_VISIBILITY_PIXEL + 1) * sizeof(*push_constants));
    memset(push_constants_offset, 0, sizeof(push_constants_offset));
    for (i = 0; i < desc->NumParameters; ++i)
    {
        const D3D12_ROOT_PARAMETER *p = &desc->pParameters[i];
        D3D12_SHADER_VISIBILITY visibility;

        if (p->ParameterType != D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
            continue;

        visibility = use_vk_heaps ? D3D12_SHADER_VISIBILITY_ALL : p->ShaderVisibility;
        VKD3D_ASSERT(visibility <= D3D12_SHADER_VISIBILITY_PIXEL);

        push_constants[visibility].stageFlags = stage_flags_from_visibility(visibility);
        push_constants[visibility].size += align(p->u.Constants.Num32BitValues, 4) * sizeof(uint32_t);
    }

    if (push_constants[D3D12_SHADER_VISIBILITY_ALL].size)
    {
        /* When D3D12_SHADER_VISIBILITY_ALL is used we use a single push
         * constants range because the Vulkan spec states:
         *
         *   "Any two elements of pPushConstantRanges must not include the same
         *   stage in stageFlags".
         */
        push_constant_count = 1;
        for (i = 0; i <= D3D12_SHADER_VISIBILITY_PIXEL; ++i)
        {
            if (i == D3D12_SHADER_VISIBILITY_ALL)
                continue;

            push_constants[D3D12_SHADER_VISIBILITY_ALL].size += push_constants[i].size;
            push_constants[i].size = 0;
        }
    }
    else
    {
        /* Move non-empty push constants ranges to front and compute offsets. */
        offset = 0;
        for (i = 0, j = 0; i <= D3D12_SHADER_VISIBILITY_PIXEL; ++i)
        {
            if (push_constants[i].size)
            {
                push_constants[j] = push_constants[i];
                push_constants[j].offset = offset;
                push_constants_offset[i] = offset;
                offset += push_constants[j].size;
                ++j;
            }
        }
        push_constant_count = j;
    }

    for (i = 0, j = 0; i < desc->NumParameters; ++i)
    {
        struct d3d12_root_constant *root_constant = &root_signature->parameters[i].u.constant;
        const D3D12_ROOT_PARAMETER *p = &desc->pParameters[i];
        unsigned int idx;

        if (p->ParameterType != D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
            continue;

        idx = push_constant_count == 1 ? 0 : p->ShaderVisibility;
        offset = push_constants_offset[idx];
        push_constants_offset[idx] += align(p->u.Constants.Num32BitValues, 4) * sizeof(uint32_t);

        root_signature->parameters[i].parameter_type = p->ParameterType;
        root_constant->stage_flags = push_constant_count == 1
                ? push_constants[0].stageFlags : stage_flags_from_visibility(p->ShaderVisibility);
        root_constant->offset = offset;

        root_signature->root_constants[j].register_space = p->u.Constants.RegisterSpace;
        root_signature->root_constants[j].register_index = p->u.Constants.ShaderRegister;
        root_signature->root_constants[j].shader_visibility
                = vkd3d_shader_visibility_from_d3d12(p->ShaderVisibility);
        root_signature->root_constants[j].offset = offset;
        root_signature->root_constants[j].size = p->u.Constants.Num32BitValues * sizeof(uint32_t);

        ++j;
    }

    *push_constant_range_count = push_constant_count;

    return S_OK;
}

struct vk_binding_array
{
    VkDescriptorSetLayoutBinding *bindings;
    size_t capacity, count;

    unsigned int table_index;
    unsigned int unbounded_offset;
    VkDescriptorSetLayoutCreateFlags flags;
};

static void vk_binding_array_cleanup(struct vk_binding_array *array)
{
    vkd3d_free(array->bindings);
    array->bindings = NULL;
}

static bool vk_binding_array_add_binding(struct vk_binding_array *array,
        VkDescriptorType descriptor_type, unsigned int descriptor_count,
        VkShaderStageFlags stage_flags, const VkSampler *immutable_sampler, unsigned int *binding_idx)
{
    unsigned int binding_count = array->count;
    VkDescriptorSetLayoutBinding *binding;

    if (!vkd3d_array_reserve((void **)&array->bindings, &array->capacity,
            array->count + 1, sizeof(*array->bindings)))
    {
        ERR("Failed to reallocate the Vulkan binding array.\n");
        return false;
    }

    *binding_idx = binding_count;
    binding = &array->bindings[binding_count];
    binding->binding = binding_count;
    binding->descriptorType = descriptor_type;
    binding->descriptorCount = descriptor_count;
    binding->stageFlags = stage_flags;
    binding->pImmutableSamplers = immutable_sampler;
    ++array->count;

    return true;
}

struct vkd3d_descriptor_set_context
{
    struct vk_binding_array vk_bindings[VKD3D_MAX_DESCRIPTOR_SETS];
    unsigned int table_index;
    unsigned int unbounded_offset;
    unsigned int descriptor_index;
    unsigned int uav_counter_index;
    unsigned int push_constant_index;
};

static void descriptor_set_context_cleanup(struct vkd3d_descriptor_set_context *context)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(context->vk_bindings); ++i)
        vk_binding_array_cleanup(&context->vk_bindings[i]);
}

static bool vkd3d_validate_descriptor_set_count(struct d3d12_device *device, unsigned int set_count)
{
    uint32_t max_count = min(VKD3D_MAX_DESCRIPTOR_SETS, device->vk_info.device_limits.maxBoundDescriptorSets);

    if (set_count > max_count)
    {
        /* NOTE: If maxBoundDescriptorSets is < 9, try VKD3D_CONFIG=virtual_heaps */
        WARN("Required descriptor set count exceeds maximum allowed count of %u.\n", max_count);
        return false;
    }

    return true;
}

static struct vk_binding_array *d3d12_root_signature_current_vk_binding_array(
        struct d3d12_root_signature *root_signature, struct vkd3d_descriptor_set_context *context)
{
    if (root_signature->vk_set_count >= ARRAY_SIZE(context->vk_bindings))
        return NULL;

    return &context->vk_bindings[root_signature->vk_set_count];
}

static void d3d12_root_signature_append_vk_binding_array(struct d3d12_root_signature *root_signature,
        VkDescriptorSetLayoutCreateFlags flags, struct vkd3d_descriptor_set_context *context)
{
    struct vk_binding_array *array;

    if (!(array = d3d12_root_signature_current_vk_binding_array(root_signature, context)) || !array->count)
        return;

    array->table_index = context->table_index;
    array->unbounded_offset = context->unbounded_offset;
    array->flags = flags;

    ++root_signature->vk_set_count;
}

static HRESULT d3d12_root_signature_append_vk_binding(struct d3d12_root_signature *root_signature,
        enum vkd3d_shader_descriptor_type descriptor_type, unsigned int register_space,
        unsigned int register_idx, bool buffer_descriptor, enum vkd3d_shader_visibility shader_visibility,
        unsigned int descriptor_count, struct vkd3d_descriptor_set_context *context,
        const VkSampler *immutable_sampler, unsigned int *binding_idx)
{
    struct vkd3d_shader_descriptor_offset *offset = root_signature->descriptor_offsets
            ? &root_signature->descriptor_offsets[context->descriptor_index] : NULL;
    struct vkd3d_shader_resource_binding *mapping;
    struct vk_binding_array *array;
    unsigned int idx;

    if (!(array = d3d12_root_signature_current_vk_binding_array(root_signature, context))
            || !(vk_binding_array_add_binding(&context->vk_bindings[root_signature->vk_set_count],
                    vk_descriptor_type_from_vkd3d_descriptor_type(descriptor_type, buffer_descriptor), descriptor_count,
                    stage_flags_from_vkd3d_shader_visibility(shader_visibility), immutable_sampler, &idx)))
        return E_OUTOFMEMORY;

    mapping = &root_signature->descriptor_mapping[context->descriptor_index++];
    mapping->type = descriptor_type;
    mapping->register_space = register_space;
    mapping->register_index = register_idx;
    mapping->shader_visibility = shader_visibility;
    mapping->flags = buffer_descriptor ? VKD3D_SHADER_BINDING_FLAG_BUFFER : VKD3D_SHADER_BINDING_FLAG_IMAGE;
    mapping->binding.set = root_signature->vk_set_count;
    mapping->binding.binding = idx;
    mapping->binding.count = descriptor_count;
    if (offset)
    {
        offset->static_offset = 0;
        offset->dynamic_offset_index = ~0u;
    }

    if (context->unbounded_offset != UINT_MAX)
        d3d12_root_signature_append_vk_binding_array(root_signature, 0, context);

    if (binding_idx)
        *binding_idx = idx;

    return S_OK;
}

static uint32_t vkd3d_descriptor_magic_from_d3d12(D3D12_DESCRIPTOR_RANGE_TYPE type)
{
    switch (type)
    {
        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
            return VKD3D_DESCRIPTOR_MAGIC_SRV;
        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
            return VKD3D_DESCRIPTOR_MAGIC_UAV;
        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
            return VKD3D_DESCRIPTOR_MAGIC_CBV;
        case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
            return VKD3D_DESCRIPTOR_MAGIC_SAMPLER;
        default:
            ERR("Invalid range type %#x.\n", type);
            return VKD3D_DESCRIPTOR_MAGIC_FREE;
    }
}

static unsigned int vk_binding_count_from_descriptor_range(const struct d3d12_root_descriptor_table_range *range,
        const struct d3d12_root_signature_info *info, const struct vkd3d_device_descriptor_limits *limits)
{
    unsigned int count, limit;

    if (range->descriptor_count != UINT_MAX)
        return range->descriptor_count;

    switch (range->type)
    {
        case VKD3D_SHADER_DESCRIPTOR_TYPE_CBV:
            limit = limits->uniform_buffer_max_descriptors;
            count = (limit - min(info->cbv_count, limit)) / info->cbv_unbounded_range_count;
            break;
        case VKD3D_SHADER_DESCRIPTOR_TYPE_SRV:
            limit = limits->sampled_image_max_descriptors;
            count = (limit - min(info->srv_count, limit)) / info->srv_unbounded_range_count;
            break;
        case VKD3D_SHADER_DESCRIPTOR_TYPE_UAV:
            limit = limits->storage_image_max_descriptors;
            count = (limit - min(info->uav_count, limit)) / info->uav_unbounded_range_count;
            break;
        case VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER:
            limit = limits->sampler_max_descriptors;
            count = (limit - min(info->sampler_count, limit)) / info->sampler_unbounded_range_count;
            break;
        default:
            ERR("Unhandled type %#x.\n", range->type);
            return 1;
    }

    if (!count)
    {
        WARN("Descriptor table exceeds type %#x limit of %u.\n", range->type, limit);
        count = 1;
    }

    return min(count, VKD3D_MAX_VIRTUAL_HEAP_DESCRIPTORS_PER_TYPE);
}

static HRESULT d3d12_root_signature_init_descriptor_table_binding(struct d3d12_root_signature *root_signature,
        const struct d3d12_root_descriptor_table_range *range, D3D12_SHADER_VISIBILITY visibility,
        unsigned int vk_binding_array_count, unsigned int bindings_per_range,
        struct vkd3d_descriptor_set_context *context)
{
    enum vkd3d_shader_visibility shader_visibility = vkd3d_shader_visibility_from_d3d12(visibility);
    bool is_buffer = range->type != VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER;
    enum vkd3d_shader_descriptor_type descriptor_type = range->type;
    unsigned int i, register_space = range->register_space;
    HRESULT hr;

    if (range->descriptor_count == UINT_MAX)
        context->unbounded_offset = range->offset;

    for (i = 0; i < bindings_per_range; ++i)
    {
        if (FAILED(hr = d3d12_root_signature_append_vk_binding(root_signature, descriptor_type,
                register_space, range->base_register_idx + i, is_buffer, shader_visibility,
                vk_binding_array_count, context, NULL, NULL)))
            return hr;
    }

    if (descriptor_type != VKD3D_SHADER_DESCRIPTOR_TYPE_SRV && descriptor_type != VKD3D_SHADER_DESCRIPTOR_TYPE_UAV)
    {
        context->unbounded_offset = UINT_MAX;
        return S_OK;
    }

    for (i = 0; i < bindings_per_range; ++i)
    {
        if (FAILED(hr = d3d12_root_signature_append_vk_binding(root_signature, descriptor_type,
                register_space, range->base_register_idx + i, false, shader_visibility,
                vk_binding_array_count, context, NULL, NULL)))
            return hr;
    }

    context->unbounded_offset = UINT_MAX;

    return S_OK;
}

static void d3d12_root_signature_map_vk_unbounded_binding(struct d3d12_root_signature *root_signature,
        const struct d3d12_root_descriptor_table_range *range, unsigned int descriptor_offset, bool buffer_descriptor,
        enum vkd3d_shader_visibility shader_visibility, struct vkd3d_descriptor_set_context *context)
{
    struct vkd3d_shader_resource_binding *mapping = &root_signature->descriptor_mapping[context->descriptor_index];
    struct vkd3d_shader_descriptor_offset *offset = &root_signature->descriptor_offsets[context->descriptor_index++];

    mapping->type = range->type;
    mapping->register_space = range->register_space;
    mapping->register_index = range->base_register_idx;
    mapping->shader_visibility = shader_visibility;
    mapping->flags = buffer_descriptor ? VKD3D_SHADER_BINDING_FLAG_BUFFER : VKD3D_SHADER_BINDING_FLAG_IMAGE;
    mapping->binding.set = root_signature->main_set + range->set + ((range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_SRV
            || range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV) && !buffer_descriptor);
    mapping->binding.binding = range->binding;
    mapping->binding.count = range->vk_binding_count;
    offset->static_offset = descriptor_offset;
    offset->dynamic_offset_index = ~0u;
}

static unsigned int vk_heap_binding_count_from_descriptor_range(const struct d3d12_root_descriptor_table_range *range,
        unsigned int descriptor_set_size)
{
    unsigned int max_count;

    if (descriptor_set_size <= range->offset)
    {
        ERR("Descriptor range offset %u exceeds maximum available offset %u.\n", range->offset, descriptor_set_size - 1);
        max_count = 0;
    }
    else
    {
        max_count = descriptor_set_size - range->offset;
    }

    if (range->descriptor_count != UINT_MAX)
    {
        if (range->descriptor_count > max_count)
            ERR("Range size %u exceeds available descriptor count %u.\n", range->descriptor_count, max_count);
        return range->descriptor_count;
    }
    else
    {
        /* Prefer an unsupported binding count vs a zero count, because shader compilation will fail
         * to match a declaration to a zero binding, resulting in failure of pipeline state creation. */
        return max_count + !max_count;
    }
}

static void vkd3d_descriptor_heap_binding_from_descriptor_range(const struct d3d12_root_descriptor_table_range *range,
        bool is_buffer, const struct d3d12_root_signature *root_signature,
        struct vkd3d_shader_descriptor_binding *binding)
{
    const struct vkd3d_device_descriptor_limits *descriptor_limits = &root_signature->device->vk_info.descriptor_limits;
    unsigned int descriptor_set_size;

    if (root_signature->device->vk_info.EXT_mutable_descriptor_type)
    {
        if (range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER)
        {
            binding->set = VKD3D_SET_INDEX_SAMPLER;
            descriptor_set_size = descriptor_limits->sampler_max_descriptors;
        }
        else
        {
            binding->set = VKD3D_SET_INDEX_MUTABLE;
            descriptor_set_size = descriptor_limits->sampled_image_max_descriptors;
        }
    }
    else switch (range->type)
    {
        case VKD3D_SHADER_DESCRIPTOR_TYPE_SRV:
            binding->set = is_buffer ? VKD3D_SET_INDEX_UNIFORM_TEXEL_BUFFER : VKD3D_SET_INDEX_SAMPLED_IMAGE;
            descriptor_set_size = descriptor_limits->sampled_image_max_descriptors;
            break;
        case VKD3D_SHADER_DESCRIPTOR_TYPE_UAV:
            binding->set = is_buffer ? VKD3D_SET_INDEX_STORAGE_TEXEL_BUFFER : VKD3D_SET_INDEX_STORAGE_IMAGE;
            descriptor_set_size = descriptor_limits->storage_image_max_descriptors;
            break;
        case VKD3D_SHADER_DESCRIPTOR_TYPE_CBV:
            binding->set = VKD3D_SET_INDEX_UNIFORM_BUFFER;
            descriptor_set_size = descriptor_limits->uniform_buffer_max_descriptors;
            break;
        case VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER:
            binding->set = VKD3D_SET_INDEX_SAMPLER;
            descriptor_set_size = descriptor_limits->sampler_max_descriptors;
            break;
        default:
            FIXME("Unhandled descriptor range type type %#x.\n", range->type);
            binding->set = VKD3D_SET_INDEX_SAMPLED_IMAGE;
            descriptor_set_size = descriptor_limits->sampled_image_max_descriptors;
            break;
    }
    binding->set += root_signature->vk_set_count;
    binding->binding = 0;
    binding->count = vk_heap_binding_count_from_descriptor_range(range, descriptor_set_size);
}

static void d3d12_root_signature_map_vk_heap_binding(struct d3d12_root_signature *root_signature,
        const struct d3d12_root_descriptor_table_range *range, bool buffer_descriptor,
        enum vkd3d_shader_visibility shader_visibility, struct vkd3d_descriptor_set_context *context)
{
    struct vkd3d_shader_resource_binding *mapping = &root_signature->descriptor_mapping[context->descriptor_index];
    struct vkd3d_shader_descriptor_offset *offset = &root_signature->descriptor_offsets[context->descriptor_index++];

    mapping->type = range->type;
    mapping->register_space = range->register_space;
    mapping->register_index = range->base_register_idx;
    mapping->shader_visibility = shader_visibility;
    mapping->flags = buffer_descriptor ? VKD3D_SHADER_BINDING_FLAG_BUFFER : VKD3D_SHADER_BINDING_FLAG_IMAGE;
    vkd3d_descriptor_heap_binding_from_descriptor_range(range, buffer_descriptor, root_signature, &mapping->binding);
    offset->static_offset = range->offset;
    offset->dynamic_offset_index = context->push_constant_index;
}

static void d3d12_root_signature_map_vk_heap_uav_counter(struct d3d12_root_signature *root_signature,
        const struct d3d12_root_descriptor_table_range *range, enum vkd3d_shader_visibility shader_visibility,
        struct vkd3d_descriptor_set_context *context)
{
    struct vkd3d_shader_uav_counter_binding *mapping = &root_signature->uav_counter_mapping[context->uav_counter_index];
    struct vkd3d_shader_descriptor_offset *offset = &root_signature->uav_counter_offsets[context->uav_counter_index++];

    mapping->register_space = range->register_space;
    mapping->register_index = range->base_register_idx;
    mapping->shader_visibility = shader_visibility;
    mapping->binding.set = root_signature->vk_set_count + VKD3D_SET_INDEX_UAV_COUNTER;
    mapping->binding.binding = 0;
    mapping->binding.count = vk_heap_binding_count_from_descriptor_range(range,
            root_signature->device->vk_info.descriptor_limits.storage_image_max_descriptors);
    offset->static_offset = range->offset;
    offset->dynamic_offset_index = context->push_constant_index;
}

static void d3d12_root_signature_map_descriptor_heap_binding(struct d3d12_root_signature *root_signature,
        const struct d3d12_root_descriptor_table_range *range, enum vkd3d_shader_visibility shader_visibility,
        struct vkd3d_descriptor_set_context *context)
{
    bool is_buffer = range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_CBV;

    if (range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_SRV || range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV)
    {
        d3d12_root_signature_map_vk_heap_binding(root_signature, range, true, shader_visibility, context);
        if (range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV)
            d3d12_root_signature_map_vk_heap_uav_counter(root_signature, range, shader_visibility, context);
    }

    d3d12_root_signature_map_vk_heap_binding(root_signature, range, is_buffer, shader_visibility, context);
}

static void d3d12_root_signature_map_descriptor_unbounded_binding(struct d3d12_root_signature *root_signature,
        const struct d3d12_root_descriptor_table_range *range, unsigned int descriptor_offset,
        enum vkd3d_shader_visibility shader_visibility, struct vkd3d_descriptor_set_context *context)
{
    bool is_buffer = range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_CBV;

    if (range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_SRV || range->type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV)
        d3d12_root_signature_map_vk_unbounded_binding(root_signature, range,
                descriptor_offset, true, shader_visibility, context);

    d3d12_root_signature_map_vk_unbounded_binding(root_signature, range,
            descriptor_offset, is_buffer, shader_visibility, context);
}

static int compare_descriptor_range(const void *a, const void *b)
{
    const struct d3d12_root_descriptor_table_range *range_a = a, *range_b = b;
    int ret;

    if ((ret = vkd3d_u32_compare(range_a->type, range_b->type)))
        return ret;

    if ((ret = vkd3d_u32_compare(range_a->offset, range_b->offset)))
        return ret;

    /* Place bounded ranges after unbounded ones of equal offset,
     * so the bounded range can be mapped to the unbounded one. */
    return (range_b->descriptor_count == UINT_MAX) - (range_a->descriptor_count == UINT_MAX);
}

static HRESULT d3d12_root_signature_init_root_descriptor_tables(struct d3d12_root_signature *root_signature,
        const D3D12_ROOT_SIGNATURE_DESC *desc, const struct d3d12_root_signature_info *info,
        struct vkd3d_descriptor_set_context *context)
{
    unsigned int i, j, range_count, bindings_per_range, vk_binding_array_count;
    const struct d3d12_device *device = root_signature->device;
    bool use_vk_heaps = root_signature->device->use_vk_heaps;
    struct d3d12_root_descriptor_table *table;
    HRESULT hr;

    root_signature->descriptor_table_mask = 0;

    for (i = 0; i < desc->NumParameters; ++i)
    {
        const struct d3d12_root_descriptor_table_range *base_range = NULL;
        const D3D12_ROOT_PARAMETER *p = &desc->pParameters[i];
        enum vkd3d_shader_visibility shader_visibility;
        unsigned int offset = 0;

        if (p->ParameterType != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            continue;

        root_signature->descriptor_table_mask |= 1ull << i;

        table = &root_signature->parameters[i].u.descriptor_table;
        range_count = p->u.DescriptorTable.NumDescriptorRanges;
        shader_visibility = vkd3d_shader_visibility_from_d3d12(p->ShaderVisibility);

        root_signature->parameters[i].parameter_type = p->ParameterType;
        table->range_count = range_count;
        if (!(table->ranges = vkd3d_calloc(table->range_count, sizeof(*table->ranges))))
            return E_OUTOFMEMORY;

        context->table_index = i;

        for (j = 0; j < range_count; ++j)
        {
            const D3D12_DESCRIPTOR_RANGE *range = &p->u.DescriptorTable.pDescriptorRanges[j];

            if (range->OffsetInDescriptorsFromTableStart != D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
                offset = range->OffsetInDescriptorsFromTableStart;

            if (range->NumDescriptors != UINT_MAX && !vkd3d_bound_range(offset, range->NumDescriptors, UINT_MAX))
                return E_INVALIDARG;

            table->ranges[j].offset = offset;
            table->ranges[j].descriptor_count = range->NumDescriptors;
            table->ranges[j].type = vkd3d_descriptor_type_from_d3d12_range_type(range->RangeType);
            table->ranges[j].descriptor_magic = vkd3d_descriptor_magic_from_d3d12(range->RangeType);
            table->ranges[j].register_space = range->RegisterSpace;
            table->ranges[j].base_register_idx = range->BaseShaderRegister;

            TRACE("Descriptor table %u, range %u, offset %u, type %#x, count %u.\n", i, j,
                    offset, range->RangeType, range->NumDescriptors);

            /* If NumDescriptors == UINT_MAX, validation during counting ensures this offset is not used. */
            offset += range->NumDescriptors;
        }

        qsort(table->ranges, range_count, sizeof(*table->ranges), compare_descriptor_range);

        for (j = 0; j < range_count; ++j)
        {
            struct d3d12_root_descriptor_table_range *range;

            range = &table->ranges[j];

            if (use_vk_heaps)
            {
                 /* set, binding and vk_binding_count are not used. */
                range->set = 0;
                range->binding = 0;
                range->vk_binding_count = 0;
                d3d12_root_signature_map_descriptor_heap_binding(root_signature, range, shader_visibility, context);
                continue;
            }

            range->set = root_signature->vk_set_count - root_signature->main_set;

            if (root_signature->use_descriptor_arrays)
            {
                if (j && range->type != table->ranges[j - 1].type)
                    base_range = NULL;

                /* Bounded and unbounded ranges can follow unbounded ones,
                 * so map them all into the first unbounded range. */
                if (base_range)
                {
                    unsigned int rel_offset = range->offset - base_range->offset;

                    if (rel_offset >= base_range->vk_binding_count)
                    {
                        ERR("Available binding size of %u is insufficient for an offset of %u.\n",
                                base_range->vk_binding_count, rel_offset);
                        continue;
                    }

                    range->set = base_range->set;
                    range->binding = base_range->binding;
                    range->vk_binding_count = base_range->vk_binding_count - rel_offset;
                    d3d12_root_signature_map_descriptor_unbounded_binding(root_signature, range,
                            rel_offset, shader_visibility, context);
                    continue;
                }
                else if (range->descriptor_count == UINT_MAX)
                {
                    base_range = range;
                }

                range->vk_binding_count = vk_binding_count_from_descriptor_range(range,
                        info, &device->vk_info.descriptor_limits);
                vk_binding_array_count = range->vk_binding_count;
                bindings_per_range = 1;
            }
            else
            {
                range->vk_binding_count = range->descriptor_count;
                vk_binding_array_count = 1;
                bindings_per_range = range->descriptor_count;
            }

            range->binding = context->vk_bindings[root_signature->vk_set_count].count;

            if (FAILED(hr = d3d12_root_signature_init_descriptor_table_binding(root_signature, range,
                    p->ShaderVisibility, vk_binding_array_count, bindings_per_range, context)))
                return hr;
        }
        ++context->push_constant_index;
    }

    return S_OK;
}

static HRESULT d3d12_root_signature_init_root_descriptors(struct d3d12_root_signature *root_signature,
        const D3D12_ROOT_SIGNATURE_DESC *desc, struct vkd3d_descriptor_set_context *context)
{
    unsigned int binding, i;
    HRESULT hr;

    root_signature->push_descriptor_mask = 0;

    for (i = 0; i < desc->NumParameters; ++i)
    {
        const D3D12_ROOT_PARAMETER *p = &desc->pParameters[i];
        if (p->ParameterType != D3D12_ROOT_PARAMETER_TYPE_CBV
                && p->ParameterType != D3D12_ROOT_PARAMETER_TYPE_SRV
                && p->ParameterType != D3D12_ROOT_PARAMETER_TYPE_UAV)
            continue;

        root_signature->push_descriptor_mask |= 1u << i;

        if (FAILED(hr = d3d12_root_signature_append_vk_binding(root_signature,
                vkd3d_descriptor_type_from_d3d12_root_parameter_type(p->ParameterType),
                p->u.Descriptor.RegisterSpace, p->u.Descriptor.ShaderRegister, true,
                vkd3d_shader_visibility_from_d3d12(p->ShaderVisibility), 1, context, NULL, &binding)))
            return hr;

        root_signature->parameters[i].parameter_type = p->ParameterType;
        root_signature->parameters[i].u.descriptor.binding = binding;
    }

    return S_OK;
}

static HRESULT d3d12_root_signature_init_static_samplers(struct d3d12_root_signature *root_signature,
        struct d3d12_device *device, const D3D12_ROOT_SIGNATURE_DESC *desc,
        struct vkd3d_descriptor_set_context *context)
{
    unsigned int i;
    HRESULT hr;

    VKD3D_ASSERT(root_signature->static_sampler_count == desc->NumStaticSamplers);
    for (i = 0; i < desc->NumStaticSamplers; ++i)
    {
        const D3D12_STATIC_SAMPLER_DESC *s = &desc->pStaticSamplers[i];

        if (FAILED(hr = vkd3d_create_static_sampler(device, s, &root_signature->static_samplers[i])))
            return hr;

        if (FAILED(hr = d3d12_root_signature_append_vk_binding(root_signature,
                VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, s->RegisterSpace, s->ShaderRegister, false,
                vkd3d_shader_visibility_from_d3d12(s->ShaderVisibility), 1, context,
                &root_signature->static_samplers[i], NULL)))
            return hr;
    }

    if (device->use_vk_heaps)
        d3d12_root_signature_append_vk_binding_array(root_signature, 0, context);

    return S_OK;
}

static void d3d12_root_signature_init_descriptor_table_push_constants(struct d3d12_root_signature *root_signature,
        const struct vkd3d_descriptor_set_context *context)
{
    root_signature->descriptor_table_offset = 0;
    if ((root_signature->descriptor_table_count = context->push_constant_index))
    {
        VkPushConstantRange *range = &root_signature->push_constant_ranges[D3D12_SHADER_VISIBILITY_ALL];

        root_signature->descriptor_table_offset = align(range->size, 16);
        range->size = root_signature->descriptor_table_offset
                + root_signature->descriptor_table_count * sizeof(uint32_t);

        if (range->size > root_signature->device->vk_info.device_limits.maxPushConstantsSize)
            FIXME("Push constants size %u exceeds maximum allowed size %u. Try VKD3D_CONFIG=virtual_heaps.\n",
                    range->size, root_signature->device->vk_info.device_limits.maxPushConstantsSize);

        if (!root_signature->push_constant_range_count)
        {
            root_signature->push_constant_range_count = 1;
            range->stageFlags = VK_SHADER_STAGE_ALL;
        }
    }
}

static bool vk_binding_uses_partial_binding(const VkDescriptorSetLayoutBinding *binding)
{
    if (binding->descriptorCount == 1)
        return false;

    switch (binding->descriptorType)
    {
        /* Types mapped in vk_descriptor_type_from_vkd3d_descriptor_type() from D3D12 SRV and UAV types,
         * i.e. those which can be a buffer or an image. */
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return true;
        default:
            return false;
    }
}

static HRESULT vkd3d_create_descriptor_set_layout(struct d3d12_device *device,
        VkDescriptorSetLayoutCreateFlags flags, unsigned int binding_count, bool unbounded,
        const VkDescriptorSetLayoutBinding *bindings, VkDescriptorSetLayout *set_layout)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flags_info;
    VkDescriptorBindingFlagsEXT *set_flags = NULL;
    VkDescriptorSetLayoutCreateInfo set_desc;
    VkResult vr;

    set_desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_desc.pNext = NULL;
    set_desc.flags = flags;
    set_desc.bindingCount = binding_count;
    set_desc.pBindings = bindings;
    if (device->vk_info.EXT_descriptor_indexing)
    {
        unsigned int i;

        for (i = 0; i < binding_count; ++i)
            if (unbounded || vk_binding_uses_partial_binding(&bindings[i]))
                break;

        if (i < binding_count)
        {
            if (!(set_flags = vkd3d_malloc(binding_count * sizeof(*set_flags))))
                return E_OUTOFMEMORY;

            for (i = 0; i < binding_count; ++i)
                set_flags[i] = vk_binding_uses_partial_binding(&bindings[i])
                        ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT : 0;

            if (unbounded)
                set_flags[binding_count - 1] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
                        | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;

            flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
            flags_info.pNext = NULL;
            flags_info.bindingCount = binding_count;
            flags_info.pBindingFlags = set_flags;

            set_desc.pNext = &flags_info;
        }
    }

    vr = VK_CALL(vkCreateDescriptorSetLayout(device->vk_device, &set_desc, NULL, set_layout));
    vkd3d_free(set_flags);
    if (vr < 0)
    {
        WARN("Failed to create Vulkan descriptor set layout, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    return S_OK;
}

static HRESULT vkd3d_create_pipeline_layout(struct d3d12_device *device,
        unsigned int set_layout_count, const VkDescriptorSetLayout *set_layouts,
        unsigned int push_constant_count, const VkPushConstantRange *push_constants,
        VkPipelineLayout *pipeline_layout)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct VkPipelineLayoutCreateInfo pipeline_layout_info;
    VkResult vr;

    if (!vkd3d_validate_descriptor_set_count(device, set_layout_count))
        return E_INVALIDARG;

    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pNext = NULL;
    pipeline_layout_info.flags = 0;
    pipeline_layout_info.setLayoutCount = set_layout_count;
    pipeline_layout_info.pSetLayouts = set_layouts;
    pipeline_layout_info.pushConstantRangeCount = push_constant_count;
    pipeline_layout_info.pPushConstantRanges = push_constants;
    if ((vr = VK_CALL(vkCreatePipelineLayout(device->vk_device,
            &pipeline_layout_info, NULL, pipeline_layout))) < 0)
    {
        WARN("Failed to create Vulkan pipeline layout, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    return S_OK;
}

static HRESULT d3d12_root_signature_create_descriptor_set_layouts(struct d3d12_root_signature *root_signature,
        struct vkd3d_descriptor_set_context *context)
{
    unsigned int i;
    HRESULT hr;

    d3d12_root_signature_append_vk_binding_array(root_signature, 0, context);

    if (!vkd3d_validate_descriptor_set_count(root_signature->device, root_signature->vk_set_count))
        return E_INVALIDARG;

    for (i = 0; i < root_signature->vk_set_count; ++i)
    {
        struct d3d12_descriptor_set_layout *layout = &root_signature->descriptor_set_layouts[i];
        struct vk_binding_array *array = &context->vk_bindings[i];

        VKD3D_ASSERT(array->count);

        if (FAILED(hr = vkd3d_create_descriptor_set_layout(root_signature->device, array->flags, array->count,
                array->unbounded_offset != UINT_MAX, array->bindings, &layout->vk_layout)))
            return hr;
        layout->unbounded_offset = array->unbounded_offset;
        layout->table_index = array->table_index;
    }

    return S_OK;
}

static unsigned int d3d12_root_signature_copy_descriptor_set_layouts(const struct d3d12_root_signature *root_signature,
        VkDescriptorSetLayout *vk_set_layouts)
{
    const struct d3d12_device *device = root_signature->device;
    enum vkd3d_vk_descriptor_set_index set;
    VkDescriptorSetLayout vk_set_layout;
    unsigned int i;

    for (i = 0; i < root_signature->vk_set_count; ++i)
        vk_set_layouts[i] = root_signature->descriptor_set_layouts[i].vk_layout;

    if (!device->use_vk_heaps)
        return i;

    for (set = 0; set < ARRAY_SIZE(device->vk_descriptor_heap_layouts); ++set)
    {
        vk_set_layout = device->vk_descriptor_heap_layouts[set].vk_set_layout;

        VKD3D_ASSERT(vk_set_layout);
        vk_set_layouts[i++] = vk_set_layout;

        if (device->vk_info.EXT_mutable_descriptor_type && set == VKD3D_SET_INDEX_MUTABLE)
            break;
    }

    return i;
}

static HRESULT d3d12_root_signature_init(struct d3d12_root_signature *root_signature,
        struct d3d12_device *device, const D3D12_ROOT_SIGNATURE_DESC *desc)
{
    VkDescriptorSetLayout vk_layouts[VKD3D_MAX_DESCRIPTOR_SETS];
    const struct vkd3d_vulkan_info *vk_info = &device->vk_info;
    struct vkd3d_descriptor_set_context context;
    struct d3d12_root_signature_info info;
    bool use_vk_heaps;
    unsigned int i;
    HRESULT hr;

    memset(&context, 0, sizeof(context));
    context.unbounded_offset = UINT_MAX;

    root_signature->ID3D12RootSignature_iface.lpVtbl = &d3d12_root_signature_vtbl;
    root_signature->refcount = 1;

    root_signature->vk_pipeline_layout = VK_NULL_HANDLE;
    root_signature->vk_set_count = 0;
    root_signature->parameters = NULL;
    root_signature->flags = desc->Flags;
    root_signature->descriptor_mapping = NULL;
    root_signature->descriptor_offsets = NULL;
    root_signature->uav_counter_mapping = NULL;
    root_signature->uav_counter_offsets = NULL;
    root_signature->static_sampler_count = 0;
    root_signature->static_samplers = NULL;
    root_signature->device = device;

    if (desc->Flags & ~(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT))
        FIXME("Ignoring root signature flags %#x.\n", desc->Flags);

    if (FAILED(hr = d3d12_root_signature_info_from_desc(&info, desc, device->vk_info.EXT_descriptor_indexing)))
        return hr;
    if (info.cost > D3D12_MAX_ROOT_COST)
    {
        WARN("Root signature cost %zu exceeds maximum allowed cost.\n", info.cost);
        return E_INVALIDARG;
    }

    root_signature->binding_count = info.binding_count;
    root_signature->uav_mapping_count = info.uav_range_count;
    root_signature->static_sampler_count = desc->NumStaticSamplers;
    root_signature->root_descriptor_count = info.root_descriptor_count;
    root_signature->use_descriptor_arrays = device->vk_info.EXT_descriptor_indexing;
    root_signature->descriptor_table_count = 0;

    use_vk_heaps = device->use_vk_heaps;

    hr = E_OUTOFMEMORY;
    root_signature->parameter_count = desc->NumParameters;
    if (!(root_signature->parameters = vkd3d_calloc(root_signature->parameter_count,
            sizeof(*root_signature->parameters))))
        goto fail;
    if (!(root_signature->descriptor_mapping = vkd3d_calloc(root_signature->binding_count,
            sizeof(*root_signature->descriptor_mapping))))
        goto fail;
    if (use_vk_heaps && (!(root_signature->uav_counter_mapping = vkd3d_calloc(root_signature->uav_mapping_count,
            sizeof(*root_signature->uav_counter_mapping)))
            || !(root_signature->uav_counter_offsets = vkd3d_calloc(root_signature->uav_mapping_count,
            sizeof(*root_signature->uav_counter_offsets)))))
        goto fail;
    if (root_signature->use_descriptor_arrays && !(root_signature->descriptor_offsets = vkd3d_calloc(
            root_signature->binding_count, sizeof(*root_signature->descriptor_offsets))))
        goto fail;
    root_signature->root_constant_count = info.root_constant_count;
    if (!(root_signature->root_constants = vkd3d_calloc(root_signature->root_constant_count,
            sizeof(*root_signature->root_constants))))
        goto fail;
    if (!(root_signature->static_samplers = vkd3d_calloc(root_signature->static_sampler_count,
            sizeof(*root_signature->static_samplers))))
        goto fail;

    if (FAILED(hr = d3d12_root_signature_init_root_descriptors(root_signature, desc, &context)))
        goto fail;

    /* We use KHR_push_descriptor for root descriptor parameters. */
    if (vk_info->KHR_push_descriptor)
    {
        d3d12_root_signature_append_vk_binding_array(root_signature,
                VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR, &context);
    }

    root_signature->main_set = root_signature->vk_set_count;

    if (FAILED(hr = d3d12_root_signature_init_push_constants(root_signature, desc,
            root_signature->push_constant_ranges, &root_signature->push_constant_range_count)))
        goto fail;
    if (FAILED(hr = d3d12_root_signature_init_static_samplers(root_signature, device, desc, &context)))
        goto fail;
    context.push_constant_index = 0;
    if (FAILED(hr = d3d12_root_signature_init_root_descriptor_tables(root_signature, desc, &info, &context)))
        goto fail;
    if (use_vk_heaps)
        d3d12_root_signature_init_descriptor_table_push_constants(root_signature, &context);

    if (FAILED(hr = d3d12_root_signature_create_descriptor_set_layouts(root_signature, &context)))
        goto fail;

    descriptor_set_context_cleanup(&context);

    i = d3d12_root_signature_copy_descriptor_set_layouts(root_signature, vk_layouts);
    if (FAILED(hr = vkd3d_create_pipeline_layout(device, i,
            vk_layouts, root_signature->push_constant_range_count,
            root_signature->push_constant_ranges, &root_signature->vk_pipeline_layout)))
        goto fail;

    if (FAILED(hr = vkd3d_private_store_init(&root_signature->private_store)))
        goto fail;

    d3d12_device_add_ref(device);

    return S_OK;

fail:
    descriptor_set_context_cleanup(&context);
    d3d12_root_signature_cleanup(root_signature, device);
    return hr;
}

HRESULT d3d12_root_signature_create(struct d3d12_device *device,
        const void *bytecode, size_t bytecode_length, struct d3d12_root_signature **root_signature)
{
    const struct vkd3d_shader_code dxbc = {bytecode, bytecode_length};
    union
    {
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3d12;
        struct vkd3d_shader_versioned_root_signature_desc vkd3d;
    } root_signature_desc;
    struct d3d12_root_signature *object;
    HRESULT hr;
    int ret;

    if ((ret = vkd3d_parse_root_signature_v_1_0(&dxbc, &root_signature_desc.vkd3d)) < 0)
    {
        WARN("Failed to parse root signature, vkd3d result %d.\n", ret);
        return hresult_from_vkd3d_result(ret);
    }

    if (!(object = vkd3d_malloc(sizeof(*object))))
    {
        vkd3d_shader_free_root_signature(&root_signature_desc.vkd3d);
        return E_OUTOFMEMORY;
    }

    hr = d3d12_root_signature_init(object, device, &root_signature_desc.d3d12.u.Desc_1_0);
    vkd3d_shader_free_root_signature(&root_signature_desc.vkd3d);
    if (FAILED(hr))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created root signature %p.\n", object);

    *root_signature = object;

    return S_OK;
}

/* vkd3d_render_pass_cache */
struct vkd3d_render_pass_entry
{
    struct vkd3d_render_pass_key key;
    VkRenderPass vk_render_pass;
};

STATIC_ASSERT(sizeof(struct vkd3d_render_pass_key) == 48);

static HRESULT vkd3d_render_pass_cache_create_pass_locked(struct vkd3d_render_pass_cache *cache,
        struct d3d12_device *device, const struct vkd3d_render_pass_key *key, VkRenderPass *vk_render_pass)
{
    VkAttachmentReference attachment_references[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT + 1];
    VkAttachmentDescription attachments[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT + 1];
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct vkd3d_render_pass_entry *entry;
    unsigned int index, attachment_index;
    VkSubpassDescription sub_pass_desc;
    VkRenderPassCreateInfo pass_info;
    bool have_depth_stencil;
    unsigned int rt_count;
    VkResult vr;

    if (!vkd3d_array_reserve((void **)&cache->render_passes, &cache->render_passes_size,
            cache->render_pass_count + 1, sizeof(*cache->render_passes)))
    {
        *vk_render_pass = VK_NULL_HANDLE;
        return E_OUTOFMEMORY;
    }

    entry = &cache->render_passes[cache->render_pass_count];

    entry->key = *key;

    have_depth_stencil = key->depth_enable || key->stencil_enable;
    rt_count = have_depth_stencil ? key->attachment_count - 1 : key->attachment_count;
    VKD3D_ASSERT(rt_count <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

    for (index = 0, attachment_index = 0; index < rt_count; ++index)
    {
        if (!key->vk_formats[index])
        {
            attachment_references[index].attachment = VK_ATTACHMENT_UNUSED;
            attachment_references[index].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            continue;
        }

        attachments[attachment_index].flags = 0;
        attachments[attachment_index].format = key->vk_formats[index];
        attachments[attachment_index].samples = key->sample_count;
        attachments[attachment_index].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[attachment_index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[attachment_index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[attachment_index].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[attachment_index].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachment_references[index].attachment = attachment_index;
        attachment_references[index].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        ++attachment_index;
    }

    if (have_depth_stencil)
    {
        VkImageLayout depth_layout = key->depth_stencil_write
                ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        attachments[attachment_index].flags = 0;
        attachments[attachment_index].format = key->vk_formats[index];
        attachments[attachment_index].samples = key->sample_count;

        if (key->depth_enable)
        {
            attachments[attachment_index].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            attachments[attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }
        else
        {
            attachments[attachment_index].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        if (key->stencil_enable)
        {
            attachments[attachment_index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            attachments[attachment_index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        }
        else
        {
            attachments[attachment_index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[attachment_index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        attachments[attachment_index].initialLayout = depth_layout;
        attachments[attachment_index].finalLayout = depth_layout;

        attachment_references[index].attachment = attachment_index;
        attachment_references[index].layout = depth_layout;

        attachment_index++;
    }

    sub_pass_desc.flags = 0;
    sub_pass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub_pass_desc.inputAttachmentCount = 0;
    sub_pass_desc.pInputAttachments = NULL;
    sub_pass_desc.colorAttachmentCount = rt_count;
    sub_pass_desc.pColorAttachments = attachment_references;
    sub_pass_desc.pResolveAttachments = NULL;
    if (have_depth_stencil)
        sub_pass_desc.pDepthStencilAttachment = &attachment_references[rt_count];
    else
        sub_pass_desc.pDepthStencilAttachment = NULL;
    sub_pass_desc.preserveAttachmentCount = 0;
    sub_pass_desc.pPreserveAttachments = NULL;

    pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass_info.pNext = NULL;
    pass_info.flags = 0;
    pass_info.attachmentCount = attachment_index;
    pass_info.pAttachments = attachments;
    pass_info.subpassCount = 1;
    pass_info.pSubpasses = &sub_pass_desc;
    pass_info.dependencyCount = 0;
    pass_info.pDependencies = NULL;
    if ((vr = VK_CALL(vkCreateRenderPass(device->vk_device, &pass_info, NULL, vk_render_pass))) >= 0)
    {
        entry->vk_render_pass = *vk_render_pass;
        ++cache->render_pass_count;
    }
    else
    {
        WARN("Failed to create Vulkan render pass, vr %d.\n", vr);
        *vk_render_pass = VK_NULL_HANDLE;
    }

    return hresult_from_vk_result(vr);
}

HRESULT vkd3d_render_pass_cache_find(struct vkd3d_render_pass_cache *cache,
        struct d3d12_device *device, const struct vkd3d_render_pass_key *key, VkRenderPass *vk_render_pass)
{
    bool found = false;
    HRESULT hr = S_OK;
    unsigned int i;

    vkd3d_mutex_lock(&device->pipeline_cache_mutex);

    for (i = 0; i < cache->render_pass_count; ++i)
    {
        struct vkd3d_render_pass_entry *current = &cache->render_passes[i];

        if (!memcmp(&current->key, key, sizeof(*key)))
        {
            *vk_render_pass = current->vk_render_pass;
            found = true;
            break;
        }
    }

    if (!found)
        hr = vkd3d_render_pass_cache_create_pass_locked(cache, device, key, vk_render_pass);

    vkd3d_mutex_unlock(&device->pipeline_cache_mutex);

    return hr;
}

void vkd3d_render_pass_cache_init(struct vkd3d_render_pass_cache *cache)
{
    cache->render_passes = NULL;
    cache->render_pass_count = 0;
    cache->render_passes_size = 0;
}

void vkd3d_render_pass_cache_cleanup(struct vkd3d_render_pass_cache *cache,
        struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    unsigned int i;

    for (i = 0; i < cache->render_pass_count; ++i)
    {
        struct vkd3d_render_pass_entry *current = &cache->render_passes[i];
        VK_CALL(vkDestroyRenderPass(device->vk_device, current->vk_render_pass, NULL));
    }

    vkd3d_free(cache->render_passes);
    cache->render_passes = NULL;
}

static void d3d12_init_pipeline_state_desc(struct d3d12_pipeline_state_desc *desc)
{
    D3D12_DEPTH_STENCIL_DESC1 *ds_state = &desc->depth_stencil_state;
    D3D12_RASTERIZER_DESC *rs_state = &desc->rasterizer_state;
    D3D12_BLEND_DESC *blend_state = &desc->blend_state;
    DXGI_SAMPLE_DESC *sample_desc = &desc->sample_desc;

    memset(desc, 0, sizeof(*desc));
    ds_state->DepthEnable = TRUE;
    ds_state->DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    ds_state->DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    ds_state->StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    ds_state->StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    ds_state->FrontFace.StencilFunc = ds_state->BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    ds_state->FrontFace.StencilDepthFailOp = ds_state->BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    ds_state->FrontFace.StencilPassOp = ds_state->BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    ds_state->FrontFace.StencilFailOp = ds_state->BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;

    rs_state->FillMode = D3D12_FILL_MODE_SOLID;
    rs_state->CullMode = D3D12_CULL_MODE_BACK;
    rs_state->DepthClipEnable = TRUE;
    rs_state->ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    blend_state->RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    sample_desc->Count = 1;
    sample_desc->Quality = 0;

    desc->sample_mask = D3D12_DEFAULT_SAMPLE_MASK;
}

static void pipeline_state_desc_from_d3d12_graphics_desc(struct d3d12_pipeline_state_desc *desc,
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC *d3d12_desc)
{
    memset(desc, 0, sizeof(*desc));
    desc->root_signature = d3d12_desc->pRootSignature;
    desc->vs = d3d12_desc->VS;
    desc->ps = d3d12_desc->PS;
    desc->ds = d3d12_desc->DS;
    desc->hs = d3d12_desc->HS;
    desc->gs = d3d12_desc->GS;
    desc->stream_output = d3d12_desc->StreamOutput;
    desc->blend_state = d3d12_desc->BlendState;
    desc->sample_mask = d3d12_desc->SampleMask;
    desc->rasterizer_state = d3d12_desc->RasterizerState;
    memcpy(&desc->depth_stencil_state, &d3d12_desc->DepthStencilState, sizeof(d3d12_desc->DepthStencilState));
    desc->input_layout = d3d12_desc->InputLayout;
    desc->strip_cut_value = d3d12_desc->IBStripCutValue;
    desc->primitive_topology_type = d3d12_desc->PrimitiveTopologyType;
    desc->rtv_formats.NumRenderTargets = d3d12_desc->NumRenderTargets;
    memcpy(desc->rtv_formats.RTFormats, d3d12_desc->RTVFormats, sizeof(desc->rtv_formats.RTFormats));
    desc->dsv_format = d3d12_desc->DSVFormat;
    desc->sample_desc = d3d12_desc->SampleDesc;
    desc->node_mask = d3d12_desc->NodeMask;
    desc->cached_pso = d3d12_desc->CachedPSO;
    desc->flags = d3d12_desc->Flags;
}

static void pipeline_state_desc_from_d3d12_compute_desc(struct d3d12_pipeline_state_desc *desc,
        const D3D12_COMPUTE_PIPELINE_STATE_DESC *d3d12_desc)
{
    memset(desc, 0, sizeof(*desc));
    desc->root_signature = d3d12_desc->pRootSignature;
    desc->cs = d3d12_desc->CS;
    desc->node_mask = d3d12_desc->NodeMask;
    desc->cached_pso = d3d12_desc->CachedPSO;
    desc->flags = d3d12_desc->Flags;
}

static HRESULT pipeline_state_desc_from_d3d12_stream_desc(struct d3d12_pipeline_state_desc *desc,
        const D3D12_PIPELINE_STATE_STREAM_DESC *d3d12_desc, VkPipelineBindPoint *vk_bind_point)
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobject_type;
    uint64_t defined_subobjects = 0;
    const uint8_t *stream_ptr;
    uint64_t subobject_bit;
    size_t start, size, i;
    uint8_t *desc_bytes;

    static const struct
    {
        size_t alignment;
        size_t size;
        size_t dst_offset;
    }
    subobject_info[] =
    {
#define DCL_SUBOBJECT_INFO(type, field) {__alignof__(type), sizeof(type), offsetof(struct d3d12_pipeline_state_desc, field)}
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE]  = DCL_SUBOBJECT_INFO(ID3D12RootSignature *, root_signature),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS]              = DCL_SUBOBJECT_INFO(D3D12_SHADER_BYTECODE, vs),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS]              = DCL_SUBOBJECT_INFO(D3D12_SHADER_BYTECODE, ps),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS]              = DCL_SUBOBJECT_INFO(D3D12_SHADER_BYTECODE, ds),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS]              = DCL_SUBOBJECT_INFO(D3D12_SHADER_BYTECODE, hs),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS]              = DCL_SUBOBJECT_INFO(D3D12_SHADER_BYTECODE, gs),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS]              = DCL_SUBOBJECT_INFO(D3D12_SHADER_BYTECODE, cs),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT]   = DCL_SUBOBJECT_INFO(D3D12_STREAM_OUTPUT_DESC, stream_output),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND]           = DCL_SUBOBJECT_INFO(D3D12_BLEND_DESC, blend_state),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK]     = DCL_SUBOBJECT_INFO(UINT, sample_mask),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER]      = DCL_SUBOBJECT_INFO(D3D12_RASTERIZER_DESC, rasterizer_state),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL]   = DCL_SUBOBJECT_INFO(D3D12_DEPTH_STENCIL_DESC, depth_stencil_state),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT]    = DCL_SUBOBJECT_INFO(D3D12_INPUT_LAYOUT_DESC, input_layout),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE] = DCL_SUBOBJECT_INFO(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE, strip_cut_value),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY] = DCL_SUBOBJECT_INFO(D3D12_PRIMITIVE_TOPOLOGY_TYPE, primitive_topology_type),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS] = DCL_SUBOBJECT_INFO(struct D3D12_RT_FORMAT_ARRAY, rtv_formats),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT]  = DCL_SUBOBJECT_INFO(DXGI_FORMAT, dsv_format),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC]     = DCL_SUBOBJECT_INFO(DXGI_SAMPLE_DESC, sample_desc),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK]       = DCL_SUBOBJECT_INFO(UINT, node_mask),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO]      = DCL_SUBOBJECT_INFO(D3D12_CACHED_PIPELINE_STATE, cached_pso),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS]           = DCL_SUBOBJECT_INFO(D3D12_PIPELINE_STATE_FLAGS, flags),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1]  = DCL_SUBOBJECT_INFO(D3D12_DEPTH_STENCIL_DESC1, depth_stencil_state),
        [D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING] = DCL_SUBOBJECT_INFO(D3D12_VIEW_INSTANCING_DESC, view_instancing_desc),
#undef DCL_SUBOBJECT_INFO
    };
    STATIC_ASSERT(ARRAY_SIZE(subobject_info) <= sizeof(defined_subobjects) * CHAR_BIT);

    /* Initialize defaults for undefined subobjects. */
    d3d12_init_pipeline_state_desc(desc);

    stream_ptr = d3d12_desc->pPipelineStateSubobjectStream;
    desc_bytes = (uint8_t *)desc;

    for (i = 0; i < d3d12_desc->SizeInBytes; )
    {
        if (!vkd3d_bound_range(0, sizeof(subobject_type), d3d12_desc->SizeInBytes - i))
        {
            WARN("Invalid pipeline state stream.\n");
            return E_INVALIDARG;
        }

        subobject_type = *(const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE *)&stream_ptr[i];
        if (subobject_type >= ARRAY_SIZE(subobject_info))
        {
            FIXME("Unhandled pipeline subobject type %#x.\n", subobject_type);
            return E_INVALIDARG;
        }

        subobject_bit = 1ull << subobject_type;
        if (defined_subobjects & subobject_bit)
        {
            WARN("Duplicate pipeline subobject type %u.\n", subobject_type);
            return E_INVALIDARG;
        }
        defined_subobjects |= subobject_bit;

        start = align(sizeof(subobject_type), subobject_info[subobject_type].alignment);
        size = subobject_info[subobject_type].size;

        if (!vkd3d_bound_range(start, size, d3d12_desc->SizeInBytes - i))
        {
            WARN("Invalid pipeline state stream.\n");
            return E_INVALIDARG;
        }

        memcpy(&desc_bytes[subobject_info[subobject_type].dst_offset], &stream_ptr[i + start], size);
        /* Stream packets are aligned to the size of pointers. */
        i += align(start + size, sizeof(void *));
    }

    /* Deduce pipeline type from specified shaders. */
    if (desc->vs.BytecodeLength && desc->vs.pShaderBytecode)
    {
        *vk_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
    else if (desc->cs.BytecodeLength && desc->cs.pShaderBytecode)
    {
        *vk_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    }
    else
    {
        WARN("Cannot deduce pipeline type from shader stages.\n");
        return E_INVALIDARG;
    }

    if (desc->vs.BytecodeLength && desc->vs.pShaderBytecode
            && desc->cs.BytecodeLength && desc->cs.pShaderBytecode)
    {
        WARN("Invalid combination of shader stages VS and CS.\n");
        return E_INVALIDARG;
    }

    return S_OK;
}

struct vkd3d_pipeline_key
{
    D3D12_PRIMITIVE_TOPOLOGY topology;
    uint32_t strides[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    VkFormat dsv_format;
};

struct vkd3d_compiled_pipeline
{
    struct list entry;
    struct vkd3d_pipeline_key key;
    VkPipeline vk_pipeline;
    VkRenderPass vk_render_pass;
};

/* ID3D12PipelineState */
static inline struct d3d12_pipeline_state *impl_from_ID3D12PipelineState(ID3D12PipelineState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_pipeline_state, ID3D12PipelineState_iface);
}

static HRESULT STDMETHODCALLTYPE d3d12_pipeline_state_QueryInterface(ID3D12PipelineState *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D12PipelineState)
            || IsEqualGUID(riid, &IID_ID3D12Pageable)
            || IsEqualGUID(riid, &IID_ID3D12DeviceChild)
            || IsEqualGUID(riid, &IID_ID3D12Object)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D12PipelineState_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_pipeline_state_AddRef(ID3D12PipelineState *iface)
{
    struct d3d12_pipeline_state *state = impl_from_ID3D12PipelineState(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&state->refcount);

    TRACE("%p increasing refcount to %u.\n", state, refcount);

    return refcount;
}

static void d3d12_pipeline_state_destroy_graphics(struct d3d12_pipeline_state *state,
        struct d3d12_device *device)
{
    struct d3d12_graphics_pipeline_state *graphics = &state->u.graphics;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct vkd3d_compiled_pipeline *current, *e;
    unsigned int i;

    for (i = 0; i < graphics->stage_count; ++i)
    {
        VK_CALL(vkDestroyShaderModule(device->vk_device, graphics->stages[i].module, NULL));
    }

    LIST_FOR_EACH_ENTRY_SAFE(current, e, &graphics->compiled_pipelines, struct vkd3d_compiled_pipeline, entry)
    {
        VK_CALL(vkDestroyPipeline(device->vk_device, current->vk_pipeline, NULL));
        vkd3d_free(current);
    }
}

static void d3d12_pipeline_uav_counter_state_cleanup(struct d3d12_pipeline_uav_counter_state *uav_counters,
        struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    if (uav_counters->vk_set_layout)
        VK_CALL(vkDestroyDescriptorSetLayout(device->vk_device, uav_counters->vk_set_layout, NULL));
    if (uav_counters->vk_pipeline_layout)
        VK_CALL(vkDestroyPipelineLayout(device->vk_device, uav_counters->vk_pipeline_layout, NULL));

    vkd3d_free(uav_counters->bindings);
}

static ULONG STDMETHODCALLTYPE d3d12_pipeline_state_Release(ID3D12PipelineState *iface)
{
    struct d3d12_pipeline_state *state = impl_from_ID3D12PipelineState(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&state->refcount);

    TRACE("%p decreasing refcount to %u.\n", state, refcount);

    if (!refcount)
    {
        struct d3d12_device *device = state->device;
        const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

        vkd3d_private_store_destroy(&state->private_store);

        if (d3d12_pipeline_state_is_graphics(state))
            d3d12_pipeline_state_destroy_graphics(state, device);
        else if (d3d12_pipeline_state_is_compute(state))
            VK_CALL(vkDestroyPipeline(device->vk_device, state->u.compute.vk_pipeline, NULL));

        d3d12_pipeline_uav_counter_state_cleanup(&state->uav_counters, device);

        if (state->implicit_root_signature)
            d3d12_root_signature_Release(state->implicit_root_signature);

        vkd3d_free(state);

        d3d12_device_release(device);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d3d12_pipeline_state_GetPrivateData(ID3D12PipelineState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_pipeline_state *state = impl_from_ID3D12PipelineState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_pipeline_state_SetPrivateData(ID3D12PipelineState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_pipeline_state *state = impl_from_ID3D12PipelineState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return vkd3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_pipeline_state_SetPrivateDataInterface(ID3D12PipelineState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d12_pipeline_state *state = impl_from_ID3D12PipelineState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return vkd3d_set_private_data_interface(&state->private_store, guid, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_pipeline_state_SetName(ID3D12PipelineState *iface, const WCHAR *name)
{
    struct d3d12_pipeline_state *state = impl_from_ID3D12PipelineState(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_w(name, state->device->wchar_size));

    if (d3d12_pipeline_state_is_compute(state))
    {
        return vkd3d_set_vk_object_name(state->device, (uint64_t)state->u.compute.vk_pipeline,
                VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, name);
    }

    return name ? S_OK : E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE d3d12_pipeline_state_GetDevice(ID3D12PipelineState *iface,
        REFIID iid, void **device)
{
    struct d3d12_pipeline_state *state = impl_from_ID3D12PipelineState(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return d3d12_device_query_interface(state->device, iid, device);
}

static HRESULT STDMETHODCALLTYPE d3d12_pipeline_state_GetCachedBlob(ID3D12PipelineState *iface,
        ID3DBlob **blob)
{
    FIXME("iface %p, blob %p stub!\n", iface, blob);

    return E_NOTIMPL;
}

static const struct ID3D12PipelineStateVtbl d3d12_pipeline_state_vtbl =
{
    /* IUnknown methods */
    d3d12_pipeline_state_QueryInterface,
    d3d12_pipeline_state_AddRef,
    d3d12_pipeline_state_Release,
    /* ID3D12Object methods */
    d3d12_pipeline_state_GetPrivateData,
    d3d12_pipeline_state_SetPrivateData,
    d3d12_pipeline_state_SetPrivateDataInterface,
    d3d12_pipeline_state_SetName,
    /* ID3D12DeviceChild methods */
    d3d12_pipeline_state_GetDevice,
    /* ID3D12PipelineState methods */
    d3d12_pipeline_state_GetCachedBlob,
};

struct d3d12_pipeline_state *unsafe_impl_from_ID3D12PipelineState(ID3D12PipelineState *iface)
{
    if (!iface)
        return NULL;
    VKD3D_ASSERT(iface->lpVtbl == &d3d12_pipeline_state_vtbl);
    return impl_from_ID3D12PipelineState(iface);
}

static inline unsigned int typed_uav_compile_option(const struct d3d12_device *device)
{
    return device->vk_info.uav_read_without_format
            ? VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV_READ_FORMAT_UNKNOWN
            : VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV_READ_FORMAT_R32;
}

static unsigned int feature_flags_compile_option(const struct d3d12_device *device)
{
    unsigned int flags = 0;

    if (device->feature_options1.Int64ShaderOps)
        flags |= VKD3D_SHADER_COMPILE_OPTION_FEATURE_INT64;
    if (device->feature_options.DoublePrecisionFloatShaderOps)
        flags |= VKD3D_SHADER_COMPILE_OPTION_FEATURE_FLOAT64;
    if (device->feature_options1.WaveOps)
        flags |= VKD3D_SHADER_COMPILE_OPTION_FEATURE_WAVE_OPS;

    return flags;
}

static HRESULT create_shader_stage(struct d3d12_device *device,
        struct VkPipelineShaderStageCreateInfo *stage_desc, enum VkShaderStageFlagBits stage,
        const D3D12_SHADER_BYTECODE *code, const struct vkd3d_shader_interface_info *shader_interface)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct vkd3d_shader_compile_info compile_info;
    struct VkShaderModuleCreateInfo shader_desc;
    struct vkd3d_shader_code spirv = {0};
    VkResult vr;
    int ret;

    const struct vkd3d_shader_compile_option options[] =
    {
        {VKD3D_SHADER_COMPILE_OPTION_API_VERSION, VKD3D_SHADER_API_VERSION_1_14},
        {VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV, typed_uav_compile_option(device)},
        {VKD3D_SHADER_COMPILE_OPTION_WRITE_TESS_GEOM_POINT_SIZE, 0},
        {VKD3D_SHADER_COMPILE_OPTION_FEATURE, feature_flags_compile_option(device)},
    };

    stage_desc->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_desc->pNext = NULL;
    stage_desc->flags = 0;
    stage_desc->stage = stage;
    stage_desc->pName = "main";
    stage_desc->pSpecializationInfo = NULL;

    shader_desc.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_desc.pNext = NULL;
    shader_desc.flags = 0;

    compile_info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    compile_info.next = shader_interface;
    compile_info.source.code = code->pShaderBytecode;
    compile_info.source.size = code->BytecodeLength;
    compile_info.target_type = VKD3D_SHADER_TARGET_SPIRV_BINARY;
    compile_info.options = options;
    compile_info.option_count = ARRAY_SIZE(options);
    compile_info.log_level = VKD3D_SHADER_LOG_NONE;
    compile_info.source_name = NULL;

    if ((ret = vkd3d_shader_parse_dxbc_source_type(&compile_info.source, &compile_info.source_type, NULL)) < 0
            || (ret = vkd3d_shader_compile(&compile_info, &spirv, NULL)) < 0)
    {
        WARN("Failed to compile shader, vkd3d result %d.\n", ret);
        return hresult_from_vkd3d_result(ret);
    }
    shader_desc.codeSize = spirv.size;
    shader_desc.pCode = spirv.code;

    vr = VK_CALL(vkCreateShaderModule(device->vk_device, &shader_desc, NULL, &stage_desc->module));
    vkd3d_shader_free_shader_code(&spirv);
    if (vr < 0)
    {
        WARN("Failed to create Vulkan shader module, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    return S_OK;
}

static int vkd3d_scan_dxbc(const struct d3d12_device *device, const D3D12_SHADER_BYTECODE *code,
        struct vkd3d_shader_scan_descriptor_info *descriptor_info)
{
    struct vkd3d_shader_compile_info compile_info;
    enum vkd3d_result ret;

    const struct vkd3d_shader_compile_option options[] =
    {
        {VKD3D_SHADER_COMPILE_OPTION_API_VERSION, VKD3D_SHADER_API_VERSION_1_14},
        {VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV, typed_uav_compile_option(device)},
    };

    compile_info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    compile_info.next = descriptor_info;
    compile_info.source.code = code->pShaderBytecode;
    compile_info.source.size = code->BytecodeLength;
    compile_info.target_type = VKD3D_SHADER_TARGET_SPIRV_BINARY;
    compile_info.options = options;
    compile_info.option_count = ARRAY_SIZE(options);
    compile_info.log_level = VKD3D_SHADER_LOG_NONE;
    compile_info.source_name = NULL;

    if ((ret = vkd3d_shader_parse_dxbc_source_type(&compile_info.source, &compile_info.source_type, NULL)) < 0)
        return ret;

    return vkd3d_shader_scan(&compile_info, NULL);
}

static HRESULT vkd3d_create_compute_pipeline(struct d3d12_device *device,
        const D3D12_SHADER_BYTECODE *code, const struct vkd3d_shader_interface_info *shader_interface,
        VkPipelineLayout vk_pipeline_layout, VkPipeline *vk_pipeline)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkComputePipelineCreateInfo pipeline_info;
    VkResult vr;
    HRESULT hr;

    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = NULL;
    pipeline_info.flags = 0;
    if (FAILED(hr = create_shader_stage(device, &pipeline_info.stage,
            VK_SHADER_STAGE_COMPUTE_BIT, code, shader_interface)))
        return hr;
    pipeline_info.layout = vk_pipeline_layout;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    vr = VK_CALL(vkCreateComputePipelines(device->vk_device,
            VK_NULL_HANDLE, 1, &pipeline_info, NULL, vk_pipeline));
    VK_CALL(vkDestroyShaderModule(device->vk_device, pipeline_info.stage.module, NULL));
    if (vr < 0)
    {
        WARN("Failed to create Vulkan compute pipeline, hr %s.\n", debugstr_hresult(hr));
        return hresult_from_vk_result(vr);
    }

    return S_OK;
}

static HRESULT d3d12_pipeline_state_init_uav_counters(struct d3d12_pipeline_state *state,
        struct d3d12_device *device, const struct d3d12_root_signature *root_signature,
        const struct vkd3d_shader_scan_descriptor_info *shader_info, VkShaderStageFlags stage_flags)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkDescriptorSetLayout set_layouts[VKD3D_MAX_DESCRIPTOR_SETS + 1];
    VkDescriptorSetLayoutBinding *binding_desc;
    uint32_t set_index, descriptor_binding;
    unsigned int uav_counter_count = 0;
    unsigned int i, j;
    HRESULT hr;

    VKD3D_ASSERT(vkd3d_popcount(stage_flags) == 1);

    for (i = 0; i < shader_info->descriptor_count; ++i)
    {
        const struct vkd3d_shader_descriptor_info *d = &shader_info->descriptors[i];

        if (d->type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV
                && (d->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_COUNTER))
            ++uav_counter_count;
    }

    if (!uav_counter_count)
        return S_OK;

    /* It should be possible to support other stages in Vulkan, but in a graphics pipeline
     * D3D12 currently only supports counters in pixel shaders, and handling multiple stages
     * would be more complex. */
    if (!(stage_flags & (VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)))
    {
        FIXME("Found a UAV counter for Vulkan shader stage %#x. UAV counters in a "
                "graphics pipeline are only supported in pixel shaders.\n", stage_flags);
        return E_INVALIDARG;
    }

    if (!(binding_desc = vkd3d_calloc(uav_counter_count, sizeof(*binding_desc))))
        return E_OUTOFMEMORY;
    if (!(state->uav_counters.bindings = vkd3d_calloc(uav_counter_count, sizeof(*state->uav_counters.bindings))))
    {
        vkd3d_free(binding_desc);
        return E_OUTOFMEMORY;
    }
    state->uav_counters.binding_count = uav_counter_count;

    descriptor_binding = 0;
    set_index = d3d12_root_signature_copy_descriptor_set_layouts(root_signature, set_layouts);

    for (i = 0, j = 0; i < shader_info->descriptor_count; ++i)
    {
        const struct vkd3d_shader_descriptor_info *d = &shader_info->descriptors[i];

        if (d->type != VKD3D_SHADER_DESCRIPTOR_TYPE_UAV
                || !(d->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_COUNTER))
            continue;

        state->uav_counters.bindings[j].register_space = d->register_space;
        state->uav_counters.bindings[j].register_index = d->register_index;
        state->uav_counters.bindings[j].shader_visibility = (stage_flags == VK_SHADER_STAGE_COMPUTE_BIT)
                ? VKD3D_SHADER_VISIBILITY_COMPUTE : VKD3D_SHADER_VISIBILITY_PIXEL;
        state->uav_counters.bindings[j].binding.set = set_index;
        state->uav_counters.bindings[j].binding.binding = descriptor_binding;
        state->uav_counters.bindings[j].binding.count = 1;

        binding_desc[j].binding = descriptor_binding;
        binding_desc[j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        binding_desc[j].descriptorCount = 1;
        binding_desc[j].stageFlags = stage_flags;
        binding_desc[j].pImmutableSamplers = NULL;

        ++descriptor_binding;
        ++j;
    }

    /* Create a descriptor set layout for UAV counters. */
    hr = vkd3d_create_descriptor_set_layout(device, 0, descriptor_binding,
            false, binding_desc, &state->uav_counters.vk_set_layout);
    vkd3d_free(binding_desc);
    if (FAILED(hr))
    {
        vkd3d_free(state->uav_counters.bindings);
        return hr;
    }

    /* Create a pipeline layout which is compatible for all other descriptor
     * sets with the root signature's pipeline layout.
     */
    state->uav_counters.set_index = set_index;
    set_layouts[set_index++] = state->uav_counters.vk_set_layout;
    if (FAILED(hr = vkd3d_create_pipeline_layout(device, set_index, set_layouts,
            root_signature->push_constant_range_count, root_signature->push_constant_ranges,
            &state->uav_counters.vk_pipeline_layout)))
    {
        VK_CALL(vkDestroyDescriptorSetLayout(device->vk_device, state->uav_counters.vk_set_layout, NULL));
        vkd3d_free(state->uav_counters.bindings);
        return hr;
    }

    return S_OK;
}

static HRESULT d3d12_pipeline_state_find_and_init_uav_counters(struct d3d12_pipeline_state *state,
        struct d3d12_device *device, const struct d3d12_root_signature *root_signature,
        const D3D12_SHADER_BYTECODE *code, VkShaderStageFlags stage_flags)
{
    struct vkd3d_shader_scan_descriptor_info shader_info;
    HRESULT hr;
    int ret;

    if (device->use_vk_heaps)
        return S_OK;

    shader_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SCAN_DESCRIPTOR_INFO;
    shader_info.next = NULL;
    if ((ret = vkd3d_scan_dxbc(device, code, &shader_info)) < 0)
    {
        WARN("Failed to scan shader bytecode, stage %#x, vkd3d result %d.\n", stage_flags, ret);
        return hresult_from_vkd3d_result(ret);
    }

    if (FAILED(hr = d3d12_pipeline_state_init_uav_counters(state, device, root_signature, &shader_info, stage_flags)))
        WARN("Failed to create descriptor set layout for UAV counters, hr %s.\n", debugstr_hresult(hr));

    vkd3d_shader_free_scan_descriptor_info(&shader_info);

    return hr;
}

static HRESULT d3d12_pipeline_state_init_compute(struct d3d12_pipeline_state *state,
        struct d3d12_device *device, const struct d3d12_pipeline_state_desc *desc)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    struct vkd3d_shader_interface_info shader_interface;
    struct vkd3d_shader_descriptor_offset_info offset_info;
    struct vkd3d_shader_spirv_target_info target_info;
    struct d3d12_root_signature *root_signature;
    VkPipelineLayout vk_pipeline_layout;
    HRESULT hr;

    state->ID3D12PipelineState_iface.lpVtbl = &d3d12_pipeline_state_vtbl;
    state->refcount = 1;

    memset(&state->uav_counters, 0, sizeof(state->uav_counters));

    if (!(root_signature = unsafe_impl_from_ID3D12RootSignature(desc->root_signature)))
    {
        TRACE("Root signature is NULL, looking for an embedded signature.\n");
        if (FAILED(hr = d3d12_root_signature_create(device,
                desc->cs.pShaderBytecode, desc->cs.BytecodeLength, &root_signature)))
        {
            WARN("Failed to find an embedded root signature, hr %s.\n", debugstr_hresult(hr));
            return hr;
        }
        state->implicit_root_signature = &root_signature->ID3D12RootSignature_iface;
    }
    else
    {
        state->implicit_root_signature = NULL;
    }

    if (FAILED(hr = d3d12_pipeline_state_find_and_init_uav_counters(state, device, root_signature,
            &desc->cs, VK_SHADER_STAGE_COMPUTE_BIT)))
    {
        if (state->implicit_root_signature)
            d3d12_root_signature_Release(state->implicit_root_signature);
        return hr;
    }

    memset(&target_info, 0, sizeof(target_info));
    target_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SPIRV_TARGET_INFO;
    target_info.environment = device->environment;
    target_info.extensions = device->vk_info.shader_extensions;
    target_info.extension_count = device->vk_info.shader_extension_count;

    if (root_signature->descriptor_offsets)
    {
        offset_info.type = VKD3D_SHADER_STRUCTURE_TYPE_DESCRIPTOR_OFFSET_INFO;
        offset_info.next = NULL;
        offset_info.descriptor_table_offset = root_signature->descriptor_table_offset;
        offset_info.descriptor_table_count = root_signature->descriptor_table_count;
        offset_info.binding_offsets = root_signature->descriptor_offsets;
        offset_info.uav_counter_offsets = root_signature->uav_counter_offsets;
        vkd3d_prepend_struct(&target_info, &offset_info);
    }

    shader_interface.type = VKD3D_SHADER_STRUCTURE_TYPE_INTERFACE_INFO;
    shader_interface.next = &target_info;
    shader_interface.bindings = root_signature->descriptor_mapping;
    shader_interface.binding_count = root_signature->binding_count;
    shader_interface.push_constant_buffers = root_signature->root_constants;
    shader_interface.push_constant_buffer_count = root_signature->root_constant_count;
    shader_interface.combined_samplers = NULL;
    shader_interface.combined_sampler_count = 0;
    if (root_signature->uav_counter_mapping)
    {
        shader_interface.uav_counters = root_signature->uav_counter_mapping;
        shader_interface.uav_counter_count = root_signature->uav_mapping_count;
    }
    else
    {
        shader_interface.uav_counters = state->uav_counters.bindings;
        shader_interface.uav_counter_count = state->uav_counters.binding_count;
    }

    vk_pipeline_layout = state->uav_counters.vk_pipeline_layout
            ? state->uav_counters.vk_pipeline_layout : root_signature->vk_pipeline_layout;
    if (FAILED(hr = vkd3d_create_compute_pipeline(device, &desc->cs, &shader_interface,
            vk_pipeline_layout, &state->u.compute.vk_pipeline)))
    {
        WARN("Failed to create Vulkan compute pipeline, hr %s.\n", debugstr_hresult(hr));
        d3d12_pipeline_uav_counter_state_cleanup(&state->uav_counters, device);
        if (state->implicit_root_signature)
            d3d12_root_signature_Release(state->implicit_root_signature);
        return hr;
    }

    if (FAILED(hr = vkd3d_private_store_init(&state->private_store)))
    {
        VK_CALL(vkDestroyPipeline(device->vk_device, state->u.compute.vk_pipeline, NULL));
        d3d12_pipeline_uav_counter_state_cleanup(&state->uav_counters, device);
        if (state->implicit_root_signature)
            d3d12_root_signature_Release(state->implicit_root_signature);
        return hr;
    }

    state->vk_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    d3d12_device_add_ref(state->device = device);

    return S_OK;
}

HRESULT d3d12_pipeline_state_create_compute(struct d3d12_device *device,
        const D3D12_COMPUTE_PIPELINE_STATE_DESC *desc, struct d3d12_pipeline_state **state)
{
    struct d3d12_pipeline_state_desc pipeline_desc;
    struct d3d12_pipeline_state *object;
    HRESULT hr;

    pipeline_state_desc_from_d3d12_compute_desc(&pipeline_desc, desc);

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_pipeline_state_init_compute(object, device, &pipeline_desc)))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created compute pipeline state %p.\n", object);

    *state = object;

    return S_OK;
}

static enum VkPolygonMode vk_polygon_mode_from_d3d12(D3D12_FILL_MODE mode)
{
    switch (mode)
    {
        case D3D12_FILL_MODE_WIREFRAME:
            return VK_POLYGON_MODE_LINE;
        case D3D12_FILL_MODE_SOLID:
            return VK_POLYGON_MODE_FILL;
        default:
            FIXME("Unhandled fill mode %#x.\n", mode);
            return VK_POLYGON_MODE_FILL;
    }
}

static enum VkCullModeFlagBits vk_cull_mode_from_d3d12(D3D12_CULL_MODE mode)
{
    switch (mode)
    {
        case D3D12_CULL_MODE_NONE:
            return VK_CULL_MODE_NONE;
        case D3D12_CULL_MODE_FRONT:
            return VK_CULL_MODE_FRONT_BIT;
        case D3D12_CULL_MODE_BACK:
            return VK_CULL_MODE_BACK_BIT;
        default:
            FIXME("Unhandled cull mode %#x.\n", mode);
            return VK_CULL_MODE_NONE;
    }
}

static void rs_desc_from_d3d12(VkPipelineRasterizationStateCreateInfo *vk_desc,
        const D3D12_RASTERIZER_DESC *d3d12_desc)
{
    vk_desc->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    vk_desc->pNext = NULL;
    vk_desc->flags = 0;
    vk_desc->depthClampEnable = !d3d12_desc->DepthClipEnable;
    vk_desc->rasterizerDiscardEnable = VK_FALSE;
    vk_desc->polygonMode = vk_polygon_mode_from_d3d12(d3d12_desc->FillMode);
    vk_desc->cullMode = vk_cull_mode_from_d3d12(d3d12_desc->CullMode);
    vk_desc->frontFace = d3d12_desc->FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    vk_desc->depthBiasEnable = d3d12_desc->DepthBias || d3d12_desc->SlopeScaledDepthBias;
    vk_desc->depthBiasConstantFactor = d3d12_desc->DepthBias;
    vk_desc->depthBiasClamp = d3d12_desc->DepthBiasClamp;
    vk_desc->depthBiasSlopeFactor = d3d12_desc->SlopeScaledDepthBias;
    vk_desc->lineWidth = 1.0f;

    if (d3d12_desc->MultisampleEnable)
        FIXME_ONCE("Ignoring MultisampleEnable %#x.\n", d3d12_desc->MultisampleEnable);
    if (d3d12_desc->AntialiasedLineEnable)
        FIXME_ONCE("Ignoring AntialiasedLineEnable %#x.\n", d3d12_desc->AntialiasedLineEnable);
    if (d3d12_desc->ForcedSampleCount)
        FIXME("Ignoring ForcedSampleCount %#x.\n", d3d12_desc->ForcedSampleCount);
    if (d3d12_desc->ConservativeRaster)
        FIXME("Ignoring ConservativeRaster %#x.\n", d3d12_desc->ConservativeRaster);
}

static void rs_depth_clip_info_from_d3d12(VkPipelineRasterizationDepthClipStateCreateInfoEXT *depth_clip_info,
        VkPipelineRasterizationStateCreateInfo *vk_rs_desc, const D3D12_RASTERIZER_DESC *d3d12_desc)
{
    vk_rs_desc->depthClampEnable = VK_TRUE;

    depth_clip_info->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
    depth_clip_info->pNext = NULL;
    depth_clip_info->flags = 0;
    depth_clip_info->depthClipEnable = d3d12_desc->DepthClipEnable;

    vk_prepend_struct(vk_rs_desc, depth_clip_info);
}

static void rs_stream_info_from_d3d12(VkPipelineRasterizationStateStreamCreateInfoEXT *stream_info,
        VkPipelineRasterizationStateCreateInfo *vk_rs_desc, const D3D12_STREAM_OUTPUT_DESC *so_desc,
        const struct vkd3d_vulkan_info *vk_info)
{
    if (!so_desc->NumEntries || !so_desc->RasterizedStream
            || so_desc->RasterizedStream == D3D12_SO_NO_RASTERIZED_STREAM)
        return;

    if (!vk_info->rasterization_stream)
    {
        FIXME("Rasterization stream select is not supported by Vulkan implementation.\n");
        return;
    }

    stream_info->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT;
    stream_info->pNext = NULL;
    stream_info->flags = 0;
    stream_info->rasterizationStream = so_desc->RasterizedStream;

    vk_prepend_struct(vk_rs_desc, stream_info);
}

static enum VkStencilOp vk_stencil_op_from_d3d12(D3D12_STENCIL_OP op)
{
    switch (op)
    {
        case D3D12_STENCIL_OP_KEEP:
            return VK_STENCIL_OP_KEEP;
        case D3D12_STENCIL_OP_ZERO:
            return VK_STENCIL_OP_ZERO;
        case D3D12_STENCIL_OP_REPLACE:
            return VK_STENCIL_OP_REPLACE;
        case D3D12_STENCIL_OP_INCR_SAT:
            return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case D3D12_STENCIL_OP_DECR_SAT:
            return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case D3D12_STENCIL_OP_INVERT:
            return VK_STENCIL_OP_INVERT;
        case D3D12_STENCIL_OP_INCR:
            return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case D3D12_STENCIL_OP_DECR:
            return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default:
            FIXME("Unhandled stencil op %#x.\n", op);
            return VK_STENCIL_OP_KEEP;
    }
}

enum VkCompareOp vk_compare_op_from_d3d12(D3D12_COMPARISON_FUNC op)
{
    switch (op)
    {
        case D3D12_COMPARISON_FUNC_NEVER:
            return VK_COMPARE_OP_NEVER;
        case D3D12_COMPARISON_FUNC_LESS:
            return VK_COMPARE_OP_LESS;
        case D3D12_COMPARISON_FUNC_EQUAL:
            return VK_COMPARE_OP_EQUAL;
        case D3D12_COMPARISON_FUNC_LESS_EQUAL:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case D3D12_COMPARISON_FUNC_GREATER:
            return VK_COMPARE_OP_GREATER;
        case D3D12_COMPARISON_FUNC_NOT_EQUAL:
            return VK_COMPARE_OP_NOT_EQUAL;
        case D3D12_COMPARISON_FUNC_GREATER_EQUAL:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case D3D12_COMPARISON_FUNC_ALWAYS:
            return VK_COMPARE_OP_ALWAYS;
        default:
            FIXME("Unhandled compare op %#x.\n", op);
            return VK_COMPARE_OP_NEVER;
    }
}

static void vk_stencil_op_state_from_d3d12(struct VkStencilOpState *vk_desc,
        const D3D12_DEPTH_STENCILOP_DESC *d3d12_desc, uint32_t compare_mask, uint32_t write_mask)
{
    vk_desc->failOp = vk_stencil_op_from_d3d12(d3d12_desc->StencilFailOp);
    vk_desc->passOp = vk_stencil_op_from_d3d12(d3d12_desc->StencilPassOp);
    vk_desc->depthFailOp = vk_stencil_op_from_d3d12(d3d12_desc->StencilDepthFailOp);
    vk_desc->compareOp = vk_compare_op_from_d3d12(d3d12_desc->StencilFunc);
    vk_desc->compareMask = compare_mask;
    vk_desc->writeMask = write_mask;
    /* The stencil reference value is a dynamic state. Set by OMSetStencilRef(). */
    vk_desc->reference = 0;
}

static void ds_desc_from_d3d12(struct VkPipelineDepthStencilStateCreateInfo *vk_desc,
        const D3D12_DEPTH_STENCIL_DESC1 *d3d12_desc)
{
    memset(vk_desc, 0, sizeof(*vk_desc));
    vk_desc->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    vk_desc->pNext = NULL;
    vk_desc->flags = 0;
    if ((vk_desc->depthTestEnable = d3d12_desc->DepthEnable))
    {
        vk_desc->depthWriteEnable = d3d12_desc->DepthWriteMask & D3D12_DEPTH_WRITE_MASK_ALL;
        vk_desc->depthCompareOp = vk_compare_op_from_d3d12(d3d12_desc->DepthFunc);
    }
    else
    {
        vk_desc->depthWriteEnable = VK_FALSE;
        vk_desc->depthCompareOp = VK_COMPARE_OP_NEVER;
    }
    vk_desc->depthBoundsTestEnable = d3d12_desc->DepthBoundsTestEnable;
    if ((vk_desc->stencilTestEnable = d3d12_desc->StencilEnable))
    {
        vk_stencil_op_state_from_d3d12(&vk_desc->front, &d3d12_desc->FrontFace,
                d3d12_desc->StencilReadMask, d3d12_desc->StencilWriteMask);
        vk_stencil_op_state_from_d3d12(&vk_desc->back, &d3d12_desc->BackFace,
                d3d12_desc->StencilReadMask, d3d12_desc->StencilWriteMask);
    }
    else
    {
        memset(&vk_desc->front, 0, sizeof(vk_desc->front));
        memset(&vk_desc->back, 0, sizeof(vk_desc->back));
    }
    vk_desc->minDepthBounds = 0.0f;
    vk_desc->maxDepthBounds = 1.0f;
}

static enum VkBlendFactor vk_blend_factor_from_d3d12(D3D12_BLEND blend, bool alpha)
{
    switch (blend)
    {
        case D3D12_BLEND_ZERO:
            return VK_BLEND_FACTOR_ZERO;
        case D3D12_BLEND_ONE:
            return VK_BLEND_FACTOR_ONE;
        case D3D12_BLEND_SRC_COLOR:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case D3D12_BLEND_INV_SRC_COLOR:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case D3D12_BLEND_SRC_ALPHA:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case D3D12_BLEND_INV_SRC_ALPHA:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case D3D12_BLEND_DEST_ALPHA:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case D3D12_BLEND_INV_DEST_ALPHA:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case D3D12_BLEND_DEST_COLOR:
            return VK_BLEND_FACTOR_DST_COLOR;
        case D3D12_BLEND_INV_DEST_COLOR:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case D3D12_BLEND_SRC_ALPHA_SAT:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case D3D12_BLEND_BLEND_FACTOR:
            if (alpha)
                return VK_BLEND_FACTOR_CONSTANT_ALPHA;
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case D3D12_BLEND_INV_BLEND_FACTOR:
            if (alpha)
                return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case D3D12_BLEND_SRC1_COLOR:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case D3D12_BLEND_INV_SRC1_COLOR:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case D3D12_BLEND_SRC1_ALPHA:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case D3D12_BLEND_INV_SRC1_ALPHA:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            FIXME("Unhandled blend %#x.\n", blend);
            return VK_BLEND_FACTOR_ZERO;
    }
}

static enum VkBlendOp vk_blend_op_from_d3d12(D3D12_BLEND_OP op)
{
    switch (op)
    {
        case D3D12_BLEND_OP_ADD:
            return VK_BLEND_OP_ADD;
        case D3D12_BLEND_OP_SUBTRACT:
            return VK_BLEND_OP_SUBTRACT;
        case D3D12_BLEND_OP_REV_SUBTRACT:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case D3D12_BLEND_OP_MIN:
            return VK_BLEND_OP_MIN;
        case D3D12_BLEND_OP_MAX:
            return VK_BLEND_OP_MAX;
        default:
            FIXME("Unhandled blend op %#x.\n", op);
            return VK_BLEND_OP_ADD;
    }
}

static void blend_attachment_from_d3d12(struct VkPipelineColorBlendAttachmentState *vk_desc,
        const D3D12_RENDER_TARGET_BLEND_DESC *d3d12_desc)
{
    if (d3d12_desc->BlendEnable)
    {
        vk_desc->blendEnable = VK_TRUE;
        vk_desc->srcColorBlendFactor = vk_blend_factor_from_d3d12(d3d12_desc->SrcBlend, false);
        vk_desc->dstColorBlendFactor = vk_blend_factor_from_d3d12(d3d12_desc->DestBlend, false);
        vk_desc->colorBlendOp = vk_blend_op_from_d3d12(d3d12_desc->BlendOp);
        vk_desc->srcAlphaBlendFactor = vk_blend_factor_from_d3d12(d3d12_desc->SrcBlendAlpha, true);
        vk_desc->dstAlphaBlendFactor = vk_blend_factor_from_d3d12(d3d12_desc->DestBlendAlpha, true);
        vk_desc->alphaBlendOp = vk_blend_op_from_d3d12(d3d12_desc->BlendOpAlpha);
    }
    else
    {
        memset(vk_desc, 0, sizeof(*vk_desc));
    }
    vk_desc->colorWriteMask = 0;
    if (d3d12_desc->RenderTargetWriteMask & D3D12_COLOR_WRITE_ENABLE_RED)
        vk_desc->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
    if (d3d12_desc->RenderTargetWriteMask & D3D12_COLOR_WRITE_ENABLE_GREEN)
        vk_desc->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
    if (d3d12_desc->RenderTargetWriteMask & D3D12_COLOR_WRITE_ENABLE_BLUE)
        vk_desc->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
    if (d3d12_desc->RenderTargetWriteMask & D3D12_COLOR_WRITE_ENABLE_ALPHA)
        vk_desc->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
}

static bool is_dual_source_blending_blend(D3D12_BLEND b)
{
    return b == D3D12_BLEND_SRC1_COLOR || b == D3D12_BLEND_INV_SRC1_COLOR
            || b == D3D12_BLEND_SRC1_ALPHA || b == D3D12_BLEND_INV_SRC1_ALPHA;
}

static bool is_dual_source_blending(const D3D12_RENDER_TARGET_BLEND_DESC *desc)
{
    return desc->BlendEnable
            && (is_dual_source_blending_blend(desc->SrcBlend)
            || is_dual_source_blending_blend(desc->DestBlend)
            || is_dual_source_blending_blend(desc->SrcBlendAlpha)
            || is_dual_source_blending_blend(desc->DestBlendAlpha));
}

static HRESULT compute_input_layout_offsets(const struct d3d12_device *device,
        const D3D12_INPUT_LAYOUT_DESC *input_layout_desc, uint32_t *offsets)
{
    uint32_t input_slot_offsets[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {0};
    const D3D12_INPUT_ELEMENT_DESC *e;
    const struct vkd3d_format *format;
    unsigned int i;

    if (input_layout_desc->NumElements > D3D12_VS_INPUT_REGISTER_COUNT)
    {
        FIXME("InputLayout.NumElements %u > %u, ignoring extra elements.\n",
                input_layout_desc->NumElements, D3D12_VS_INPUT_REGISTER_COUNT);
    }

    for (i = 0; i < min(input_layout_desc->NumElements, D3D12_VS_INPUT_REGISTER_COUNT); ++i)
    {
        e = &input_layout_desc->pInputElementDescs[i];

        if (e->InputSlot >= ARRAY_SIZE(input_slot_offsets))
        {
            WARN("Invalid input slot %#x.\n", e->InputSlot);
            return E_INVALIDARG;
        }

        /* TODO: DXGI_FORMAT_UNKNOWN will return a format with byte_count == 1,
         * which may not match driver behaviour (return E_INVALIDARG?). */
        if (!(format = vkd3d_get_format(device, e->Format, false)))
        {
            WARN("Invalid input element format %#x.\n", e->Format);
            return E_INVALIDARG;
        }

        if (e->AlignedByteOffset != D3D12_APPEND_ALIGNED_ELEMENT)
            offsets[i] = e->AlignedByteOffset;
        else
            offsets[i] = align(input_slot_offsets[e->InputSlot], min(4, format->byte_count));

        input_slot_offsets[e->InputSlot] = offsets[i] + format->byte_count;
    }

    return S_OK;
}

static unsigned int vkd3d_get_rt_format_swizzle(const struct vkd3d_format *format)
{
    if (format->dxgi_format == DXGI_FORMAT_A8_UNORM)
        return VKD3D_SHADER_SWIZZLE(W, X, Y, Z);

    return VKD3D_SHADER_NO_SWIZZLE;
}

STATIC_ASSERT(sizeof(struct vkd3d_shader_transform_feedback_element) == sizeof(D3D12_SO_DECLARATION_ENTRY));

static HRESULT d3d12_graphics_pipeline_state_create_render_pass(
        struct d3d12_graphics_pipeline_state *graphics, struct d3d12_device *device,
        VkFormat dynamic_dsv_format, VkRenderPass *vk_render_pass)
{
    struct vkd3d_render_pass_key key;
    VkFormat dsv_format;
    unsigned int i;

    memcpy(key.vk_formats, graphics->rtv_formats, sizeof(graphics->rtv_formats));
    key.attachment_count = graphics->rt_count;

    if (!(dsv_format = graphics->dsv_format) && (graphics->null_attachment_mask & dsv_attachment_mask(graphics)))
        dsv_format = dynamic_dsv_format;

    if (dsv_format)
    {
        VKD3D_ASSERT(graphics->ds_desc.front.writeMask == graphics->ds_desc.back.writeMask);
        key.depth_enable = graphics->ds_desc.depthTestEnable;
        key.stencil_enable = graphics->ds_desc.stencilTestEnable;
        key.depth_stencil_write = graphics->ds_desc.depthWriteEnable
                || graphics->ds_desc.front.writeMask;
        key.vk_formats[key.attachment_count++] = dsv_format;
    }
    else
    {
        key.depth_enable = false;
        key.stencil_enable = false;
        key.depth_stencil_write = false;
    }

    if (key.attachment_count != ARRAY_SIZE(key.vk_formats))
        key.vk_formats[ARRAY_SIZE(key.vk_formats) - 1] = VK_FORMAT_UNDEFINED;
    for (i = key.attachment_count; i < ARRAY_SIZE(key.vk_formats); ++i)
        VKD3D_ASSERT(key.vk_formats[i] == VK_FORMAT_UNDEFINED);

    key.padding = 0;
    key.sample_count = graphics->ms_desc.rasterizationSamples;

    return vkd3d_render_pass_cache_find(&device->render_pass_cache, device, &key, vk_render_pass);
}

static VkLogicOp vk_logic_op_from_d3d12(D3D12_LOGIC_OP op)
{
    switch (op)
    {
        case D3D12_LOGIC_OP_CLEAR:
            return VK_LOGIC_OP_CLEAR;
        case D3D12_LOGIC_OP_SET:
            return VK_LOGIC_OP_SET;
        case D3D12_LOGIC_OP_COPY:
            return VK_LOGIC_OP_COPY;
        case D3D12_LOGIC_OP_COPY_INVERTED:
            return VK_LOGIC_OP_COPY_INVERTED;
        case D3D12_LOGIC_OP_NOOP:
            return VK_LOGIC_OP_NO_OP;
        case D3D12_LOGIC_OP_INVERT:
            return VK_LOGIC_OP_INVERT;
        case D3D12_LOGIC_OP_AND:
            return VK_LOGIC_OP_AND;
        case D3D12_LOGIC_OP_NAND:
            return VK_LOGIC_OP_NAND;
        case D3D12_LOGIC_OP_OR:
            return VK_LOGIC_OP_OR;
        case D3D12_LOGIC_OP_NOR:
            return VK_LOGIC_OP_NOR;
        case D3D12_LOGIC_OP_XOR:
            return VK_LOGIC_OP_XOR;
        case D3D12_LOGIC_OP_EQUIV:
            return VK_LOGIC_OP_EQUIVALENT;
        case D3D12_LOGIC_OP_AND_REVERSE:
            return VK_LOGIC_OP_AND_REVERSE;
        case D3D12_LOGIC_OP_AND_INVERTED:
            return VK_LOGIC_OP_AND_INVERTED;
        case D3D12_LOGIC_OP_OR_REVERSE:
            return VK_LOGIC_OP_OR_REVERSE;
        case D3D12_LOGIC_OP_OR_INVERTED:
            return VK_LOGIC_OP_OR_INVERTED;
        default:
            FIXME("Unhandled logic op %#x.\n", op);
            return VK_LOGIC_OP_NO_OP;
    }
}

static HRESULT d3d12_pipeline_state_init_graphics(struct d3d12_pipeline_state *state,
        struct d3d12_device *device, const struct d3d12_pipeline_state_desc *desc)
{
    unsigned int ps_output_swizzle[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
    struct d3d12_graphics_pipeline_state *graphics = &state->u.graphics;
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    const D3D12_STREAM_OUTPUT_DESC *so_desc = &desc->stream_output;
    VkVertexInputBindingDivisorDescriptionEXT *binding_divisor;
    const struct vkd3d_vulkan_info *vk_info = &device->vk_info;
    uint32_t instance_divisors[D3D12_VS_INPUT_REGISTER_COUNT];
    struct vkd3d_shader_spirv_target_info *stage_target_info;
    uint32_t aligned_offsets[D3D12_VS_INPUT_REGISTER_COUNT];
    struct vkd3d_shader_descriptor_offset_info offset_info;
    struct vkd3d_shader_parameter ps_shader_parameters[1];
    struct vkd3d_shader_transform_feedback_info xfb_info;
    struct vkd3d_shader_spirv_target_info ps_target_info;
    struct vkd3d_shader_interface_info shader_interface;
    struct vkd3d_shader_spirv_target_info target_info;
    const struct d3d12_root_signature *root_signature;
    struct vkd3d_shader_signature input_signature;
    bool have_attachment, is_dsv_format_unknown;
    VkShaderStageFlagBits xfb_stage = 0;
    VkSampleCountFlagBits sample_count;
    const struct vkd3d_format *format;
    unsigned int instance_divisor;
    VkVertexInputRate input_rate;
    unsigned int i, j;
    size_t rt_count;
    uint32_t mask;
    HRESULT hr;
    int ret;

    static const DWORD default_ps_code[] =
    {
#if 0
        ps_4_0
        ret
#endif
        0x43425844, 0x19cbf606, 0x18f562b9, 0xdaeed4db, 0xc324aa46, 0x00000001, 0x00000060, 0x00000003,
        0x0000002c, 0x0000003c, 0x0000004c, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x00000008, 0x00000000, 0x00000008, 0x52444853, 0x0000000c, 0x00000040, 0x00000003, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE default_ps = {default_ps_code, sizeof(default_ps_code)};
    static const struct
    {
        enum VkShaderStageFlagBits stage;
        ptrdiff_t offset;
    }
    shader_stages[] =
    {
        {VK_SHADER_STAGE_VERTEX_BIT,                  offsetof(struct d3d12_pipeline_state_desc, vs)},
        {VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,    offsetof(struct d3d12_pipeline_state_desc, hs)},
        {VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, offsetof(struct d3d12_pipeline_state_desc, ds)},
        {VK_SHADER_STAGE_GEOMETRY_BIT,                offsetof(struct d3d12_pipeline_state_desc, gs)},
        {VK_SHADER_STAGE_FRAGMENT_BIT,                offsetof(struct d3d12_pipeline_state_desc, ps)},
    };

    state->ID3D12PipelineState_iface.lpVtbl = &d3d12_pipeline_state_vtbl;
    state->refcount = 1;

    memset(&state->uav_counters, 0, sizeof(state->uav_counters));
    graphics->stage_count = 0;

    memset(&input_signature, 0, sizeof(input_signature));

    for (i = desc->rtv_formats.NumRenderTargets; i < ARRAY_SIZE(desc->rtv_formats.RTFormats); ++i)
    {
        if (desc->rtv_formats.RTFormats[i] != DXGI_FORMAT_UNKNOWN)
        {
            WARN("Format must be set to DXGI_FORMAT_UNKNOWN for inactive render targets.\n");
            return E_INVALIDARG;
        }
    }

    if (!(root_signature = unsafe_impl_from_ID3D12RootSignature(desc->root_signature)))
    {
        WARN("Root signature is NULL.\n");
        return E_INVALIDARG;
    }

    sample_count = vk_samples_from_dxgi_sample_desc(&desc->sample_desc);
    if (desc->sample_desc.Count != 1 && desc->sample_desc.Quality)
        WARN("Ignoring sample quality %u.\n", desc->sample_desc.Quality);

    rt_count = desc->rtv_formats.NumRenderTargets;
    if (rt_count > ARRAY_SIZE(graphics->blend_attachments))
    {
        FIXME("NumRenderTargets %zu > %zu, ignoring extra formats.\n",
                rt_count, ARRAY_SIZE(graphics->blend_attachments));
        rt_count = ARRAY_SIZE(graphics->blend_attachments);
    }

    graphics->om_logic_op_enable = desc->blend_state.RenderTarget[0].LogicOpEnable
            && device->feature_options.OutputMergerLogicOp;
    graphics->om_logic_op = graphics->om_logic_op_enable
            ? vk_logic_op_from_d3d12(desc->blend_state.RenderTarget[0].LogicOp)
            : VK_LOGIC_OP_COPY;
    if (desc->blend_state.RenderTarget[0].LogicOpEnable && !graphics->om_logic_op_enable)
        WARN("The device does not support output merger logic ops. Ignoring logic op %#x.\n",
                desc->blend_state.RenderTarget[0].LogicOp);

    graphics->null_attachment_mask = 0;
    for (i = 0; i < rt_count; ++i)
    {
        const D3D12_RENDER_TARGET_BLEND_DESC *rt_desc;

        if (desc->rtv_formats.RTFormats[i] == DXGI_FORMAT_UNKNOWN)
        {
            graphics->null_attachment_mask |= 1u << i;
            ps_output_swizzle[i] = VKD3D_SHADER_NO_SWIZZLE;
            graphics->rtv_formats[i] = VK_FORMAT_UNDEFINED;
        }
        else if ((format = vkd3d_get_format(device, desc->rtv_formats.RTFormats[i], false)))
        {
            ps_output_swizzle[i] = vkd3d_get_rt_format_swizzle(format);
            graphics->rtv_formats[i] = format->vk_format;
        }
        else
        {
            WARN("Invalid RTV format %#x.\n", desc->rtv_formats.RTFormats[i]);
            hr = E_INVALIDARG;
            goto fail;
        }

        rt_desc = &desc->blend_state.RenderTarget[desc->blend_state.IndependentBlendEnable ? i : 0];
        if (desc->blend_state.IndependentBlendEnable && rt_desc->LogicOpEnable)
        {
            WARN("IndependentBlendEnable must be FALSE when logic operations are enabled.\n");
            hr = E_INVALIDARG;
            goto fail;
        }
        if (rt_desc->BlendEnable && rt_desc->LogicOpEnable)
        {
            WARN("Only one of BlendEnable or LogicOpEnable can be set to TRUE.\n");
            hr = E_INVALIDARG;
            goto fail;
        }

        blend_attachment_from_d3d12(&graphics->blend_attachments[i], rt_desc);
    }
    for (i = rt_count; i < ARRAY_SIZE(graphics->rtv_formats); ++i)
        graphics->rtv_formats[i] = VK_FORMAT_UNDEFINED;
    graphics->rt_count = rt_count;

    ds_desc_from_d3d12(&graphics->ds_desc, &desc->depth_stencil_state);
    if (graphics->ds_desc.depthBoundsTestEnable && !device->feature_options2.DepthBoundsTestSupported)
    {
        WARN("Depth bounds test not supported by device.\n");
        hr = E_INVALIDARG;
        goto fail;
    }
    if (desc->dsv_format == DXGI_FORMAT_UNKNOWN
            && graphics->ds_desc.depthTestEnable && !graphics->ds_desc.depthWriteEnable
            && graphics->ds_desc.depthCompareOp == VK_COMPARE_OP_ALWAYS && !graphics->ds_desc.stencilTestEnable)
    {
        TRACE("Disabling depth test.\n");
        graphics->ds_desc.depthTestEnable = VK_FALSE;
    }

    graphics->dsv_format = VK_FORMAT_UNDEFINED;
    if (graphics->ds_desc.depthTestEnable || graphics->ds_desc.stencilTestEnable
            || graphics->ds_desc.depthBoundsTestEnable)
    {
        if (desc->dsv_format == DXGI_FORMAT_UNKNOWN)
        {
            WARN("DSV format is DXGI_FORMAT_UNKNOWN.\n");
            graphics->dsv_format = VK_FORMAT_UNDEFINED;
            graphics->null_attachment_mask |= dsv_attachment_mask(graphics);
        }
        else if ((format = vkd3d_get_format(device, desc->dsv_format, true)))
        {
            if (!(format->vk_aspect_mask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)))
                FIXME("Format %#x is not depth/stencil format.\n", format->dxgi_format);

            graphics->dsv_format = format->vk_format;
        }
        else
        {
            WARN("Invalid DSV format %#x.\n", desc->dsv_format);
            hr = E_INVALIDARG;
            goto fail;
        }

        if (!desc->ps.pShaderBytecode)
        {
            if (FAILED(hr = create_shader_stage(device, &graphics->stages[graphics->stage_count],
                    VK_SHADER_STAGE_FRAGMENT_BIT, &default_ps, NULL)))
                goto fail;

            ++graphics->stage_count;
        }
    }

    ps_shader_parameters[0].name = VKD3D_SHADER_PARAMETER_NAME_RASTERIZER_SAMPLE_COUNT;
    ps_shader_parameters[0].type = VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT;
    ps_shader_parameters[0].data_type = VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32;
    ps_shader_parameters[0].u.immediate_constant.u.u32 = sample_count;

    ps_target_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SPIRV_TARGET_INFO;
    ps_target_info.next = NULL;
    ps_target_info.entry_point = "main";
    ps_target_info.environment = device->environment;
    ps_target_info.extensions = vk_info->shader_extensions;
    ps_target_info.extension_count = vk_info->shader_extension_count;
    ps_target_info.parameters = ps_shader_parameters;
    ps_target_info.parameter_count = ARRAY_SIZE(ps_shader_parameters);
    ps_target_info.dual_source_blending = is_dual_source_blending(&desc->blend_state.RenderTarget[0]);
    ps_target_info.output_swizzles = ps_output_swizzle;
    ps_target_info.output_swizzle_count = rt_count;

    if (ps_target_info.dual_source_blending && rt_count > 1)
    {
        WARN("Only one render target is allowed when dual source blending is used.\n");
        hr = E_INVALIDARG;
        goto fail;
    }
    if (ps_target_info.dual_source_blending && desc->blend_state.IndependentBlendEnable)
    {
        for (i = 1; i < ARRAY_SIZE(desc->blend_state.RenderTarget); ++i)
        {
            if (desc->blend_state.RenderTarget[i].BlendEnable)
            {
                WARN("Blend enable cannot be set for render target %u when dual source blending is used.\n", i);
                hr = E_INVALIDARG;
                goto fail;
            }
        }
    }

    memset(&target_info, 0, sizeof(target_info));
    target_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SPIRV_TARGET_INFO;
    target_info.environment = device->environment;
    target_info.extensions = vk_info->shader_extensions;
    target_info.extension_count = vk_info->shader_extension_count;

    graphics->xfb_enabled = false;
    if (so_desc->NumEntries)
    {
        if (!(root_signature->flags & D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT))
        {
            WARN("Stream output is used without D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT.\n");
            hr = E_INVALIDARG;
            goto fail;
        }

        if (!vk_info->EXT_transform_feedback)
        {
            FIXME("Transform feedback is not supported by Vulkan implementation.\n");
            hr = E_NOTIMPL;
            goto fail;
        }

        graphics->xfb_enabled = true;

        xfb_info.type = VKD3D_SHADER_STRUCTURE_TYPE_TRANSFORM_FEEDBACK_INFO;
        xfb_info.next = NULL;

        xfb_info.elements = (const struct vkd3d_shader_transform_feedback_element *)so_desc->pSODeclaration;
        xfb_info.element_count = so_desc->NumEntries;
        xfb_info.buffer_strides = so_desc->pBufferStrides;
        xfb_info.buffer_stride_count = so_desc->NumStrides;

        if (desc->gs.pShaderBytecode)
            xfb_stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        else if (desc->ds.pShaderBytecode)
            xfb_stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        else
            xfb_stage = VK_SHADER_STAGE_VERTEX_BIT;
    }

    shader_interface.type = VKD3D_SHADER_STRUCTURE_TYPE_INTERFACE_INFO;
    shader_interface.next = NULL;
    shader_interface.bindings = root_signature->descriptor_mapping;
    shader_interface.binding_count = root_signature->binding_count;
    shader_interface.push_constant_buffers = root_signature->root_constants;
    shader_interface.push_constant_buffer_count = root_signature->root_constant_count;
    shader_interface.combined_samplers = NULL;
    shader_interface.combined_sampler_count = 0;

    if (root_signature->descriptor_offsets)
    {
        offset_info.type = VKD3D_SHADER_STRUCTURE_TYPE_DESCRIPTOR_OFFSET_INFO;
        offset_info.next = NULL;
        offset_info.descriptor_table_offset = root_signature->descriptor_table_offset;
        offset_info.descriptor_table_count = root_signature->descriptor_table_count;
        offset_info.binding_offsets = root_signature->descriptor_offsets;
        offset_info.uav_counter_offsets = root_signature->uav_counter_offsets;
    }

    for (i = 0; i < ARRAY_SIZE(shader_stages); ++i)
    {
        const D3D12_SHADER_BYTECODE *b = (const void *)((uintptr_t)desc + shader_stages[i].offset);
        const struct vkd3d_shader_code dxbc = {b->pShaderBytecode, b->BytecodeLength};

        if (!b->pShaderBytecode)
            continue;

        if (FAILED(hr = d3d12_pipeline_state_find_and_init_uav_counters(state, device, root_signature,
                b, shader_stages[i].stage)))
            goto fail;

        shader_interface.uav_counters = NULL;
        shader_interface.uav_counter_count = 0;
        stage_target_info = &target_info;
        switch (shader_stages[i].stage)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:
                if ((ret = vkd3d_shader_parse_input_signature(&dxbc, &input_signature, NULL)) < 0)
                {
                    hr = hresult_from_vkd3d_result(ret);
                    goto fail;
                }
                break;

            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                if (desc->primitive_topology_type != D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH)
                {
                    WARN("D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH must be used with tessellation shaders.\n");
                    hr = E_INVALIDARG;
                    goto fail;
                }
                break;

            case VK_SHADER_STAGE_GEOMETRY_BIT:
                break;

            case VK_SHADER_STAGE_FRAGMENT_BIT:
                shader_interface.uav_counters = root_signature->uav_counter_mapping
                        ? root_signature->uav_counter_mapping : state->uav_counters.bindings;
                shader_interface.uav_counter_count = root_signature->uav_counter_mapping
                        ? root_signature->uav_mapping_count : state->uav_counters.binding_count;
                stage_target_info = &ps_target_info;
                break;

            default:
                hr = E_INVALIDARG;
                goto fail;
        }

        shader_interface.next = NULL;
        xfb_info.next = NULL;
        ps_target_info.next = NULL;
        target_info.next = NULL;
        offset_info.next = NULL;
        if (shader_stages[i].stage == xfb_stage)
            vkd3d_prepend_struct(&shader_interface, &xfb_info);
        vkd3d_prepend_struct(&shader_interface, stage_target_info);
        if (root_signature->descriptor_offsets)
            vkd3d_prepend_struct(&shader_interface, &offset_info);

        if (FAILED(hr = create_shader_stage(device, &graphics->stages[graphics->stage_count],
                shader_stages[i].stage, b, &shader_interface)))
            goto fail;

        ++graphics->stage_count;
    }

    graphics->attribute_count = desc->input_layout.NumElements;
    if (graphics->attribute_count > ARRAY_SIZE(graphics->attributes))
    {
        FIXME("InputLayout.NumElements %zu > %zu, ignoring extra elements.\n",
                graphics->attribute_count, ARRAY_SIZE(graphics->attributes));
        graphics->attribute_count = ARRAY_SIZE(graphics->attributes);
    }

    if (graphics->attribute_count
            && !(root_signature->flags & D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT))
    {
        WARN("Input layout is used without D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT.\n");
        hr = E_INVALIDARG;
        goto fail;
    }

    if (FAILED(hr = compute_input_layout_offsets(device, &desc->input_layout, aligned_offsets)))
        goto fail;

    graphics->instance_divisor_count = 0;
    for (i = 0, j = 0, mask = 0; i < graphics->attribute_count; ++i)
    {
        const D3D12_INPUT_ELEMENT_DESC *e = &desc->input_layout.pInputElementDescs[i];
        const struct vkd3d_shader_signature_element *signature_element;

        /* TODO: DXGI_FORMAT_UNKNOWN will succeed here, which may not match
         * driver behaviour (return E_INVALIDARG?). */
        if (!(format = vkd3d_get_format(device, e->Format, false)))
        {
            WARN("Invalid input element format %#x.\n", e->Format);
            hr = E_INVALIDARG;
            goto fail;
        }

        if (e->InputSlot >= ARRAY_SIZE(graphics->input_rates)
                || e->InputSlot >= ARRAY_SIZE(instance_divisors))
        {
            WARN("Invalid input slot %#x.\n", e->InputSlot);
            hr = E_INVALIDARG;
            goto fail;
        }

        if (!(signature_element = vkd3d_shader_find_signature_element(&input_signature,
                e->SemanticName, e->SemanticIndex, 0)))
        {
            WARN("Unused input element %u.\n", i);
            continue;
        }

        graphics->attributes[j].location = signature_element->register_index;
        graphics->attributes[j].binding = e->InputSlot;
        graphics->attributes[j].format = format->vk_format;
        if (e->AlignedByteOffset != D3D12_APPEND_ALIGNED_ELEMENT)
            graphics->attributes[j].offset = e->AlignedByteOffset;
        else
            graphics->attributes[j].offset = aligned_offsets[i];
        ++j;

        switch (e->InputSlotClass)
        {
            case D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA:
                input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
                instance_divisor = 1;
                break;

            case D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA:
                input_rate = VK_VERTEX_INPUT_RATE_INSTANCE;
                instance_divisor = e->InstanceDataStepRate;
                if (instance_divisor > vk_info->max_vertex_attrib_divisor
                        || (!instance_divisor && !vk_info->vertex_attrib_zero_divisor))
                {
                    FIXME("Instance divisor %u not supported by Vulkan implementation.\n", instance_divisor);
                    instance_divisor = 1;
                }
                break;

            default:
                FIXME("Unhandled input slot class %#x on input element %u.\n", e->InputSlotClass, i);
                hr = E_INVALIDARG;
                goto fail;
        }

        if (mask & (1u << e->InputSlot) && (graphics->input_rates[e->InputSlot] != input_rate
                || instance_divisors[e->InputSlot] != instance_divisor))
        {
            FIXME("Input slot rate %#x, instance divisor %u on input element %u conflicts "
                    "with earlier input slot rate %#x, instance divisor %u.\n",
                    input_rate, instance_divisor, e->InputSlot,
                    graphics->input_rates[e->InputSlot], instance_divisors[e->InputSlot]);
            hr = E_INVALIDARG;
            goto fail;
        }

        graphics->input_rates[e->InputSlot] = input_rate;
        instance_divisors[e->InputSlot] = instance_divisor;
        if (instance_divisor != 1 && !(mask & (1u << e->InputSlot)))
        {
            binding_divisor = &graphics->instance_divisors[graphics->instance_divisor_count++];
            binding_divisor->binding = e->InputSlot;
            binding_divisor->divisor = instance_divisor;
        }
        mask |= 1u << e->InputSlot;
    }
    graphics->attribute_count = j;
    vkd3d_shader_free_shader_signature(&input_signature);

    switch (desc->strip_cut_value)
    {
        case D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED:
        case D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF:
        case D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF:
            graphics->index_buffer_strip_cut_value = desc->strip_cut_value;
            break;
        default:
            WARN("Invalid index buffer strip cut value %#x.\n", desc->strip_cut_value);
            hr = E_INVALIDARG;
            goto fail;
    }

    is_dsv_format_unknown = graphics->null_attachment_mask & dsv_attachment_mask(graphics);

    rs_desc_from_d3d12(&graphics->rs_desc, &desc->rasterizer_state);
    have_attachment = graphics->rt_count || graphics->dsv_format || is_dsv_format_unknown;
    if ((!have_attachment && !(desc->ps.pShaderBytecode && desc->ps.BytecodeLength))
            || (graphics->xfb_enabled && so_desc->RasterizedStream == D3D12_SO_NO_RASTERIZED_STREAM))
        graphics->rs_desc.rasterizerDiscardEnable = VK_TRUE;

    rs_stream_info_from_d3d12(&graphics->rs_stream_info, &graphics->rs_desc, so_desc, vk_info);
    if (vk_info->EXT_depth_clip_enable)
        rs_depth_clip_info_from_d3d12(&graphics->rs_depth_clip_info, &graphics->rs_desc, &desc->rasterizer_state);

    graphics->ms_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    graphics->ms_desc.pNext = NULL;
    graphics->ms_desc.flags = 0;
    graphics->ms_desc.rasterizationSamples = sample_count;
    graphics->ms_desc.sampleShadingEnable = VK_FALSE;
    graphics->ms_desc.minSampleShading = 0.0f;
    graphics->ms_desc.pSampleMask = NULL;
    if (desc->sample_mask != ~0u)
    {
        VKD3D_ASSERT(DIV_ROUND_UP(sample_count, 32) <= ARRAY_SIZE(graphics->sample_mask));
        graphics->sample_mask[0] = desc->sample_mask;
        graphics->sample_mask[1] = 0xffffffffu;
        graphics->ms_desc.pSampleMask = graphics->sample_mask;
    }
    graphics->ms_desc.alphaToCoverageEnable = desc->blend_state.AlphaToCoverageEnable;
    graphics->ms_desc.alphaToOneEnable = VK_FALSE;

    if (desc->view_instancing_desc.ViewInstanceCount)
    {
        FIXME("View instancing is not supported yet.\n");
        hr = E_INVALIDARG;
        goto fail;
    }

    /* We defer creating the render pass for pipelines with DSVFormat equal to
     * DXGI_FORMAT_UNKNOWN. We take the actual DSV format from the bound DSV. */
    if (is_dsv_format_unknown)
        graphics->render_pass = VK_NULL_HANDLE;
    else if (FAILED(hr = d3d12_graphics_pipeline_state_create_render_pass(graphics,
            device, 0, &graphics->render_pass)))
        goto fail;

    graphics->root_signature = root_signature;

    list_init(&graphics->compiled_pipelines);

    if (FAILED(hr = vkd3d_private_store_init(&state->private_store)))
        goto fail;

    state->vk_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    state->implicit_root_signature = NULL;
    d3d12_device_add_ref(state->device = device);

    return S_OK;

fail:
    for (i = 0; i < graphics->stage_count; ++i)
    {
        VK_CALL(vkDestroyShaderModule(device->vk_device, state->u.graphics.stages[i].module, NULL));
    }
    vkd3d_shader_free_shader_signature(&input_signature);

    d3d12_pipeline_uav_counter_state_cleanup(&state->uav_counters, device);

    return hr;
}

HRESULT d3d12_pipeline_state_create_graphics(struct d3d12_device *device,
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc, struct d3d12_pipeline_state **state)
{
    struct d3d12_pipeline_state_desc pipeline_desc;
    struct d3d12_pipeline_state *object;
    HRESULT hr;

    pipeline_state_desc_from_d3d12_graphics_desc(&pipeline_desc, desc);

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d12_pipeline_state_init_graphics(object, device, &pipeline_desc)))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created graphics pipeline state %p.\n", object);

    *state = object;

    return S_OK;
}

HRESULT d3d12_pipeline_state_create(struct d3d12_device *device,
        const D3D12_PIPELINE_STATE_STREAM_DESC *desc, struct d3d12_pipeline_state **state)
{
    struct d3d12_pipeline_state_desc pipeline_desc;
    struct d3d12_pipeline_state *object;
    VkPipelineBindPoint bind_point;
    HRESULT hr;

    if (FAILED(hr = pipeline_state_desc_from_d3d12_stream_desc(&pipeline_desc, desc, &bind_point)))
        return hr;

    if (!(object = vkd3d_calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    switch (bind_point)
    {
        case VK_PIPELINE_BIND_POINT_COMPUTE:
            hr = d3d12_pipeline_state_init_compute(object, device, &pipeline_desc);
            break;

        case VK_PIPELINE_BIND_POINT_GRAPHICS:
            hr = d3d12_pipeline_state_init_graphics(object, device, &pipeline_desc);
            break;

        default:
            vkd3d_unreachable();
    }

    if (FAILED(hr))
    {
        vkd3d_free(object);
        return hr;
    }

    TRACE("Created pipeline state %p.\n", object);

    *state = object;
    return S_OK;
}

static enum VkPrimitiveTopology vk_topology_from_d3d12_topology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
    switch (topology)
    {
        case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST:
        case D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST:
            return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        case D3D_PRIMITIVE_TOPOLOGY_UNDEFINED:
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        default:
            FIXME("Unhandled primitive topology %#x.\n", topology);
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
}

static bool vk_topology_can_restart(VkPrimitiveTopology topology)
{
    switch (topology)
    {
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
        case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
        case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
            return false;

        default:
            return true;
    }
}

static VkPipeline d3d12_pipeline_state_find_compiled_pipeline(const struct d3d12_pipeline_state *state,
        const struct vkd3d_pipeline_key *key, VkRenderPass *vk_render_pass)
{
    const struct d3d12_graphics_pipeline_state *graphics = &state->u.graphics;
    struct d3d12_device *device = state->device;
    VkPipeline vk_pipeline = VK_NULL_HANDLE;
    struct vkd3d_compiled_pipeline *current;

    *vk_render_pass = VK_NULL_HANDLE;

    vkd3d_mutex_lock(&device->pipeline_cache_mutex);

    LIST_FOR_EACH_ENTRY(current, &graphics->compiled_pipelines, struct vkd3d_compiled_pipeline, entry)
    {
        if (!memcmp(&current->key, key, sizeof(*key)))
        {
            vk_pipeline = current->vk_pipeline;
            *vk_render_pass = current->vk_render_pass;
            break;
        }
    }

    vkd3d_mutex_unlock(&device->pipeline_cache_mutex);

    return vk_pipeline;
}

static bool d3d12_pipeline_state_put_pipeline_to_cache(struct d3d12_pipeline_state *state,
        const struct vkd3d_pipeline_key *key, VkPipeline vk_pipeline, VkRenderPass vk_render_pass)
{
    struct d3d12_graphics_pipeline_state *graphics = &state->u.graphics;
    struct vkd3d_compiled_pipeline *compiled_pipeline, *current;
    struct d3d12_device *device = state->device;

    if (!(compiled_pipeline = vkd3d_malloc(sizeof(*compiled_pipeline))))
        return false;

    compiled_pipeline->key = *key;
    compiled_pipeline->vk_pipeline = vk_pipeline;
    compiled_pipeline->vk_render_pass = vk_render_pass;

    vkd3d_mutex_lock(&device->pipeline_cache_mutex);

    LIST_FOR_EACH_ENTRY(current, &graphics->compiled_pipelines, struct vkd3d_compiled_pipeline, entry)
    {
        if (!memcmp(&current->key, key, sizeof(*key)))
        {
            vkd3d_free(compiled_pipeline);
            compiled_pipeline = NULL;
            break;
        }
    }

    if (compiled_pipeline)
        list_add_tail(&graphics->compiled_pipelines, &compiled_pipeline->entry);

    vkd3d_mutex_unlock(&device->pipeline_cache_mutex);
    return compiled_pipeline;
}

VkPipeline d3d12_pipeline_state_get_or_create_pipeline(struct d3d12_pipeline_state *state,
        D3D12_PRIMITIVE_TOPOLOGY topology, const uint32_t *strides, VkFormat dsv_format,
        VkRenderPass *vk_render_pass)
{
    VkVertexInputBindingDescription bindings[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    const struct vkd3d_vk_device_procs *vk_procs = &state->device->vk_procs;
    struct d3d12_graphics_pipeline_state *graphics = &state->u.graphics;
    VkPipelineVertexInputDivisorStateCreateInfoEXT input_divisor_info;
    VkPipelineTessellationStateCreateInfo tessellation_info;
    VkPipelineVertexInputStateCreateInfo input_desc;
    VkPipelineInputAssemblyStateCreateInfo ia_desc;
    VkPipelineColorBlendStateCreateInfo blend_desc;
    struct d3d12_device *device = state->device;
    VkGraphicsPipelineCreateInfo pipeline_desc;
    struct vkd3d_pipeline_key pipeline_key;
    size_t binding_count = 0;
    VkPipeline vk_pipeline;
    unsigned int i;
    uint32_t mask;
    VkResult vr;
    HRESULT hr;

    static const VkPipelineViewportStateCreateInfo vp_desc =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = NULL,
        .scissorCount = 1,
        .pScissors = NULL,
    };
    static const VkDynamicState dynamic_states[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS,
    };
    static const VkPipelineDynamicStateCreateInfo dynamic_desc =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .dynamicStateCount = ARRAY_SIZE(dynamic_states),
        .pDynamicStates = dynamic_states,
    };

    VKD3D_ASSERT(d3d12_pipeline_state_is_graphics(state));

    memset(&pipeline_key, 0, sizeof(pipeline_key));
    pipeline_key.topology = topology;

    for (i = 0, mask = 0; i < graphics->attribute_count; ++i)
    {
        struct VkVertexInputBindingDescription *b;
        uint32_t binding;

        binding = graphics->attributes[i].binding;
        if (mask & (1u << binding))
            continue;

        if (binding_count == ARRAY_SIZE(bindings))
        {
            FIXME("Maximum binding count exceeded.\n");
            break;
        }

        mask |= 1u << binding;
        b = &bindings[binding_count];
        b->binding = binding;
        b->stride = strides[binding];
        b->inputRate = graphics->input_rates[binding];

        pipeline_key.strides[binding_count] = strides[binding];

        ++binding_count;
    }

    pipeline_key.dsv_format = dsv_format;

    if ((vk_pipeline = d3d12_pipeline_state_find_compiled_pipeline(state, &pipeline_key, vk_render_pass)))
        return vk_pipeline;

    input_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input_desc.pNext = NULL;
    input_desc.flags = 0;
    input_desc.vertexBindingDescriptionCount = binding_count;
    input_desc.pVertexBindingDescriptions = bindings;
    input_desc.vertexAttributeDescriptionCount = graphics->attribute_count;
    input_desc.pVertexAttributeDescriptions = graphics->attributes;

    if (graphics->instance_divisor_count)
    {
        input_desc.pNext = &input_divisor_info;
        input_divisor_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;
        input_divisor_info.pNext = NULL;
        input_divisor_info.vertexBindingDivisorCount = graphics->instance_divisor_count;
        input_divisor_info.pVertexBindingDivisors = graphics->instance_divisors;
    }

    ia_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia_desc.pNext = NULL;
    ia_desc.flags = 0;
    ia_desc.topology = vk_topology_from_d3d12_topology(topology);
    ia_desc.primitiveRestartEnable = graphics->index_buffer_strip_cut_value
            && vk_topology_can_restart(ia_desc.topology);

    if (ia_desc.topology == VK_PRIMITIVE_TOPOLOGY_MAX_ENUM)
    {
        WARN("Primitive topology is undefined.\n");
        return VK_NULL_HANDLE;
    }

    tessellation_info.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellation_info.pNext = NULL;
    tessellation_info.flags = 0;
    tessellation_info.patchControlPoints
            = max(topology - D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + 1, 1);

    blend_desc.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_desc.pNext = NULL;
    blend_desc.flags = 0;
    blend_desc.logicOpEnable = graphics->om_logic_op_enable;
    blend_desc.logicOp = graphics->om_logic_op;
    blend_desc.attachmentCount = graphics->rt_count;
    blend_desc.pAttachments = graphics->blend_attachments;
    blend_desc.blendConstants[0] = D3D12_DEFAULT_BLEND_FACTOR_RED;
    blend_desc.blendConstants[1] = D3D12_DEFAULT_BLEND_FACTOR_GREEN;
    blend_desc.blendConstants[2] = D3D12_DEFAULT_BLEND_FACTOR_BLUE;
    blend_desc.blendConstants[3] = D3D12_DEFAULT_BLEND_FACTOR_ALPHA;

    pipeline_desc.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_desc.pNext = NULL;
    pipeline_desc.flags = 0;
    pipeline_desc.stageCount = graphics->stage_count;
    pipeline_desc.pStages = graphics->stages;
    pipeline_desc.pVertexInputState = &input_desc;
    pipeline_desc.pInputAssemblyState = &ia_desc;
    pipeline_desc.pTessellationState = &tessellation_info;
    pipeline_desc.pViewportState = &vp_desc;
    pipeline_desc.pRasterizationState = &graphics->rs_desc;
    pipeline_desc.pMultisampleState = &graphics->ms_desc;
    pipeline_desc.pDepthStencilState = &graphics->ds_desc;
    pipeline_desc.pColorBlendState = &blend_desc;
    pipeline_desc.pDynamicState = &dynamic_desc;
    pipeline_desc.layout = state->uav_counters.vk_pipeline_layout ? state->uav_counters.vk_pipeline_layout
            : graphics->root_signature->vk_pipeline_layout;
    pipeline_desc.subpass = 0;
    pipeline_desc.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_desc.basePipelineIndex = -1;

    /* Create a render pass for pipelines with DXGI_FORMAT_UNKNOWN. */
    if (!(pipeline_desc.renderPass = graphics->render_pass))
    {
        if (graphics->null_attachment_mask & dsv_attachment_mask(graphics))
            TRACE("Compiling %p with DSV format %#x.\n", state, dsv_format);

        if (FAILED(hr = d3d12_graphics_pipeline_state_create_render_pass(graphics, device, dsv_format,
                &pipeline_desc.renderPass)))
            return VK_NULL_HANDLE;
    }

    *vk_render_pass = pipeline_desc.renderPass;

    if ((vr = VK_CALL(vkCreateGraphicsPipelines(device->vk_device, device->vk_pipeline_cache,
            1, &pipeline_desc, NULL, &vk_pipeline))) < 0)
    {
        WARN("Failed to create Vulkan graphics pipeline, vr %d.\n", vr);
        return VK_NULL_HANDLE;
    }

    if (d3d12_pipeline_state_put_pipeline_to_cache(state, &pipeline_key, vk_pipeline, pipeline_desc.renderPass))
        return vk_pipeline;

    /* Other thread compiled the pipeline before us. */
    VK_CALL(vkDestroyPipeline(device->vk_device, vk_pipeline, NULL));
    vk_pipeline = d3d12_pipeline_state_find_compiled_pipeline(state, &pipeline_key, vk_render_pass);
    if (!vk_pipeline)
        ERR("Could not get the pipeline compiled by other thread from the cache.\n");
    return vk_pipeline;
}

static int compile_hlsl_cs(const struct vkd3d_shader_code *hlsl, struct vkd3d_shader_code *dxbc)
{
    struct vkd3d_shader_hlsl_source_info hlsl_info;
    struct vkd3d_shader_compile_info info;

    static const struct vkd3d_shader_compile_option options[] =
    {
        {VKD3D_SHADER_COMPILE_OPTION_API_VERSION, VKD3D_SHADER_API_VERSION_1_14},
    };

    info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.next = &hlsl_info;
    info.source = *hlsl;
    info.source_type = VKD3D_SHADER_SOURCE_HLSL;
    info.target_type = VKD3D_SHADER_TARGET_DXBC_TPF;
    info.options = options;
    info.option_count = ARRAY_SIZE(options);
    info.log_level = VKD3D_SHADER_LOG_NONE;
    info.source_name = NULL;

    hlsl_info.type = VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO;
    hlsl_info.next = NULL;
    hlsl_info.entry_point = "main";
    hlsl_info.secondary_code.code = NULL;
    hlsl_info.secondary_code.size = 0;
    hlsl_info.profile = "cs_5_0";

    return vkd3d_shader_compile(&info, dxbc, NULL);
}

static void vkd3d_uav_clear_pipelines_cleanup(struct vkd3d_uav_clear_pipelines *pipelines,
        struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    VK_CALL(vkDestroyPipeline(device->vk_device, pipelines->image_3d, NULL));
    VK_CALL(vkDestroyPipeline(device->vk_device, pipelines->image_2d_array, NULL));
    VK_CALL(vkDestroyPipeline(device->vk_device, pipelines->image_2d, NULL));
    VK_CALL(vkDestroyPipeline(device->vk_device, pipelines->image_1d_array, NULL));
    VK_CALL(vkDestroyPipeline(device->vk_device, pipelines->image_1d, NULL));
    VK_CALL(vkDestroyPipeline(device->vk_device, pipelines->buffer, NULL));
}

void vkd3d_uav_clear_state_cleanup(struct vkd3d_uav_clear_state *state, struct d3d12_device *device)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;

    vkd3d_uav_clear_pipelines_cleanup(&state->pipelines_uint, device);
    vkd3d_uav_clear_pipelines_cleanup(&state->pipelines_float, device);

    VK_CALL(vkDestroyPipelineLayout(device->vk_device, state->vk_pipeline_layout_image, NULL));
    VK_CALL(vkDestroyPipelineLayout(device->vk_device, state->vk_pipeline_layout_buffer, NULL));

    VK_CALL(vkDestroyDescriptorSetLayout(device->vk_device, state->vk_set_layout_image, NULL));
    VK_CALL(vkDestroyDescriptorSetLayout(device->vk_device, state->vk_set_layout_buffer, NULL));
}

HRESULT vkd3d_uav_clear_state_init(struct vkd3d_uav_clear_state *state, struct d3d12_device *device)
{
    struct vkd3d_shader_push_constant_buffer push_constant;
    struct vkd3d_shader_interface_info shader_interface;
    struct vkd3d_shader_resource_binding binding;
    VkDescriptorSetLayoutBinding set_binding;
    VkPushConstantRange push_constant_range;
    unsigned int i;
    HRESULT hr;

    const struct
    {
        VkDescriptorSetLayout *set_layout;
        VkPipelineLayout *pipeline_layout;
        VkDescriptorType descriptor_type;
    }
    set_layouts[] =
    {
        {&state->vk_set_layout_buffer, &state->vk_pipeline_layout_buffer, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER},
        {&state->vk_set_layout_image,  &state->vk_pipeline_layout_image, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
    };

    const struct
    {
        VkPipeline *pipeline;
        VkPipelineLayout *pipeline_layout;
        struct vkd3d_shader_code code;
    }
    pipelines[] =
    {
#define SHADER_CODE(name) {name, sizeof(name)}
        {&state->pipelines_float.buffer, &state->vk_pipeline_layout_buffer,
                SHADER_CODE(cs_uav_clear_buffer_float_code)},
        {&state->pipelines_float.image_1d, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_1d_float_code)},
        {&state->pipelines_float.image_1d_array, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_1d_array_float_code)},
        {&state->pipelines_float.image_2d, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_2d_float_code)},
        {&state->pipelines_float.image_2d_array, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_2d_array_float_code)},
        {&state->pipelines_float.image_3d, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_3d_float_code)},

        {&state->pipelines_uint.buffer, &state->vk_pipeline_layout_buffer,
                SHADER_CODE(cs_uav_clear_buffer_uint_code)},
        {&state->pipelines_uint.image_1d, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_1d_uint_code)},
        {&state->pipelines_uint.image_1d_array, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_1d_array_uint_code)},
        {&state->pipelines_uint.image_2d, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_2d_uint_code)},
        {&state->pipelines_uint.image_2d_array, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_2d_array_uint_code)},
        {&state->pipelines_uint.image_3d, &state->vk_pipeline_layout_image,
                SHADER_CODE(cs_uav_clear_3d_uint_code)},
#undef SHADER_CODE
    };

    memset(state, 0, sizeof(*state));

    set_binding.binding = 0;
    set_binding.descriptorCount = 1;
    set_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    set_binding.pImmutableSamplers = NULL;

    binding.type = VKD3D_SHADER_DESCRIPTOR_TYPE_UAV;
    binding.register_space = 0;
    binding.register_index = 0;
    binding.shader_visibility = VKD3D_SHADER_VISIBILITY_COMPUTE;
    binding.binding.set = 0;
    binding.binding.binding = 0;
    binding.binding.count = 1;

    push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(struct vkd3d_uav_clear_args);

    push_constant.register_space = 0;
    push_constant.register_index = 0;
    push_constant.shader_visibility = VKD3D_SHADER_VISIBILITY_COMPUTE;
    push_constant.offset = 0;
    push_constant.size = sizeof(struct vkd3d_uav_clear_args);

    for (i = 0; i < ARRAY_SIZE(set_layouts); ++i)
    {
        set_binding.descriptorType = set_layouts[i].descriptor_type;

        if (FAILED(hr = vkd3d_create_descriptor_set_layout(device, 0,
                1, false, &set_binding, set_layouts[i].set_layout)))
        {
            ERR("Failed to create descriptor set layout %u, hr %s.\n", i, debugstr_hresult(hr));
            goto fail;
        }

        if (FAILED(hr = vkd3d_create_pipeline_layout(device, 1, set_layouts[i].set_layout,
                1, &push_constant_range, set_layouts[i].pipeline_layout)))
        {
            ERR("Failed to create pipeline layout %u, hr %s.\n", i, debugstr_hresult(hr));
            goto fail;
        }
    }

    shader_interface.type = VKD3D_SHADER_STRUCTURE_TYPE_INTERFACE_INFO;
    shader_interface.next = NULL;
    shader_interface.bindings = &binding;
    shader_interface.binding_count = 1;
    shader_interface.push_constant_buffers = &push_constant;
    shader_interface.push_constant_buffer_count = 1;
    shader_interface.combined_samplers = NULL;
    shader_interface.combined_sampler_count = 0;
    shader_interface.uav_counters = NULL;
    shader_interface.uav_counter_count = 0;

    for (i = 0; i < ARRAY_SIZE(pipelines); ++i)
    {
        struct vkd3d_shader_code dxbc;
        int ret;

        if ((ret = compile_hlsl_cs(&pipelines[i].code, &dxbc)))
        {
            ERR("Failed to compile HLSL compute shader %u, ret %d.\n", i, ret);
            hr = hresult_from_vk_result(ret);
            goto fail;
        }

        if (pipelines[i].pipeline_layout == &state->vk_pipeline_layout_buffer)
            binding.flags = VKD3D_SHADER_BINDING_FLAG_BUFFER;
        else
            binding.flags = VKD3D_SHADER_BINDING_FLAG_IMAGE;

        hr = vkd3d_create_compute_pipeline(device, &(D3D12_SHADER_BYTECODE){dxbc.code, dxbc.size},
                &shader_interface, *pipelines[i].pipeline_layout, pipelines[i].pipeline);
        vkd3d_shader_free_shader_code(&dxbc);
        if (FAILED(hr))
        {
            ERR("Failed to create compute pipeline %u, hr %s.\n", i, debugstr_hresult(hr));
            goto fail;
        }
    }

    return S_OK;

fail:
    vkd3d_uav_clear_state_cleanup(state, device);
    return hr;
}
