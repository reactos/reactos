/*
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
 *
 */

#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

/* ID3D11BlendState1 methods */

static HRESULT STDMETHODCALLTYPE d3d11_blend_state_QueryInterface(ID3D11BlendState1 *iface,
        REFIID riid, void **object)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState1(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11BlendState)
            || IsEqualGUID(riid, &IID_ID3D11BlendState1)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11BlendState1_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10BlendState1)
            || IsEqualGUID(riid, &IID_ID3D10BlendState)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10BlendState1_AddRef(&state->ID3D10BlendState1_iface);
        *object = &state->ID3D10BlendState1_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_blend_state_AddRef(ID3D11BlendState1 *iface)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState1(iface);
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %lu.\n", state, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(state->device);
        wined3d_blend_state_incref(state->wined3d_state);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_blend_state_Release(ID3D11BlendState1 *iface)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState1(iface);
    ULONG refcount = InterlockedDecrement(&state->refcount);

    TRACE("%p decreasing refcount to %lu.\n", state, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = state->device;
        wined3d_blend_state_decref(state->wined3d_state);
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_blend_state_GetDevice(ID3D11BlendState1 *iface,
        ID3D11Device **device)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState1(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)state->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_blend_state_GetPrivateData(ID3D11BlendState1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_blend_state_SetPrivateData(ID3D11BlendState1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_blend_state_SetPrivateDataInterface(ID3D11BlendState1 *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState1(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_blend_state_GetDesc(ID3D11BlendState1 *iface, D3D11_BLEND_DESC *desc)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState1(iface);
    const D3D11_BLEND_DESC1 *d3d11_desc = &state->desc;
    unsigned int i;

    TRACE("iface %p, desc %p.\n", iface, desc);

    desc->AlphaToCoverageEnable = d3d11_desc->AlphaToCoverageEnable;
    desc->IndependentBlendEnable = d3d11_desc->IndependentBlendEnable;
    for (i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        desc->RenderTarget[i].BlendEnable = d3d11_desc->RenderTarget[i].BlendEnable;
        desc->RenderTarget[i].SrcBlend = d3d11_desc->RenderTarget[i].SrcBlend;
        desc->RenderTarget[i].DestBlend = d3d11_desc->RenderTarget[i].DestBlend;
        desc->RenderTarget[i].BlendOp = d3d11_desc->RenderTarget[i].BlendOp;
        desc->RenderTarget[i].SrcBlendAlpha = d3d11_desc->RenderTarget[i].SrcBlendAlpha;
        desc->RenderTarget[i].DestBlendAlpha = d3d11_desc->RenderTarget[i].DestBlendAlpha;
        desc->RenderTarget[i].BlendOpAlpha = d3d11_desc->RenderTarget[i].BlendOpAlpha;
        desc->RenderTarget[i].RenderTargetWriteMask = d3d11_desc->RenderTarget[i].RenderTargetWriteMask;
    }
}

static void STDMETHODCALLTYPE d3d11_blend_state_GetDesc1(ID3D11BlendState1 *iface, D3D11_BLEND_DESC1 *desc)
{
    struct d3d_blend_state *state = impl_from_ID3D11BlendState1(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = state->desc;
}

static const struct ID3D11BlendState1Vtbl d3d11_blend_state_vtbl =
{
    /* IUnknown methods */
    d3d11_blend_state_QueryInterface,
    d3d11_blend_state_AddRef,
    d3d11_blend_state_Release,
    /* ID3D11DeviceChild methods */
    d3d11_blend_state_GetDevice,
    d3d11_blend_state_GetPrivateData,
    d3d11_blend_state_SetPrivateData,
    d3d11_blend_state_SetPrivateDataInterface,
    /* ID3D11BlendState methods */
    d3d11_blend_state_GetDesc,
    /* ID3D11BlendState1 methods */
    d3d11_blend_state_GetDesc1,
};

/* ID3D10BlendState methods */

static inline struct d3d_blend_state *impl_from_ID3D10BlendState(ID3D10BlendState1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_blend_state, ID3D10BlendState1_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_blend_state_QueryInterface(ID3D10BlendState1 *iface,
        REFIID riid, void **object)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_blend_state_QueryInterface(&state->ID3D11BlendState1_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_blend_state_AddRef(ID3D10BlendState1 *iface)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_blend_state_AddRef(&state->ID3D11BlendState1_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_blend_state_Release(ID3D10BlendState1 *iface)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_blend_state_Release(&state->ID3D11BlendState1_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_blend_state_GetDevice(ID3D10BlendState1 *iface, ID3D10Device **device)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(state->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_blend_state_GetPrivateData(ID3D10BlendState1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_blend_state_SetPrivateData(ID3D10BlendState1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_blend_state_SetPrivateDataInterface(ID3D10BlendState1 *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

/* ID3D10BlendState methods */

static D3D10_BLEND d3d10_blend_from_d3d11(D3D11_BLEND factor)
{
    return (D3D10_BLEND)factor;
}

static D3D10_BLEND_OP d3d10_blend_op_from_d3d11(D3D11_BLEND_OP op)
{
    return (D3D10_BLEND_OP)op;
}

static void STDMETHODCALLTYPE d3d10_blend_state_GetDesc(ID3D10BlendState1 *iface, D3D10_BLEND_DESC *desc)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);
    const D3D11_BLEND_DESC1 *d3d11_desc = &state->desc;
    unsigned int i;

    TRACE("iface %p, desc %p.\n", iface, desc);

    desc->AlphaToCoverageEnable = d3d11_desc->AlphaToCoverageEnable;
    desc->SrcBlend = d3d10_blend_from_d3d11(d3d11_desc->RenderTarget[0].SrcBlend);
    desc->DestBlend = d3d10_blend_from_d3d11(d3d11_desc->RenderTarget[0].DestBlend);
    desc->BlendOp = d3d10_blend_op_from_d3d11(d3d11_desc->RenderTarget[0].BlendOp);
    desc->SrcBlendAlpha = d3d10_blend_from_d3d11(d3d11_desc->RenderTarget[0].SrcBlendAlpha);
    desc->DestBlendAlpha = d3d10_blend_from_d3d11(d3d11_desc->RenderTarget[0].DestBlendAlpha);
    desc->BlendOpAlpha = d3d10_blend_op_from_d3d11(d3d11_desc->RenderTarget[0].BlendOpAlpha);
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        desc->BlendEnable[i] = d3d11_desc->RenderTarget[i].BlendEnable;
        desc->RenderTargetWriteMask[i] = d3d11_desc->RenderTarget[i].RenderTargetWriteMask;
    }
}

static void STDMETHODCALLTYPE d3d10_blend_state_GetDesc1(ID3D10BlendState1 *iface, D3D10_BLEND_DESC1 *desc)
{
    struct d3d_blend_state *state = impl_from_ID3D10BlendState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    d3d11_blend_state_GetDesc(&state->ID3D11BlendState1_iface, (D3D11_BLEND_DESC *)desc);
}

static const struct ID3D10BlendState1Vtbl d3d10_blend_state_vtbl =
{
    /* IUnknown methods */
    d3d10_blend_state_QueryInterface,
    d3d10_blend_state_AddRef,
    d3d10_blend_state_Release,
    /* ID3D10DeviceChild methods */
    d3d10_blend_state_GetDevice,
    d3d10_blend_state_GetPrivateData,
    d3d10_blend_state_SetPrivateData,
    d3d10_blend_state_SetPrivateDataInterface,
    /* ID3D10BlendState methods */
    d3d10_blend_state_GetDesc,
    /* ID3D10BlendState1 methods */
    d3d10_blend_state_GetDesc1,
};

static void STDMETHODCALLTYPE d3d_blend_state_wined3d_object_destroyed(void *parent)
{
    struct d3d_blend_state *state = parent;
    struct d3d_device *device = impl_from_ID3D11Device2(state->device);

    wine_rb_remove(&device->blend_states, &state->entry);
    wined3d_private_store_cleanup(&state->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d_blend_state_wined3d_parent_ops =
{
    d3d_blend_state_wined3d_object_destroyed,
};

static enum wined3d_blend wined3d_blend_from_d3d11(D3D11_BLEND factor)
{
    return (enum wined3d_blend)factor;
}

static enum wined3d_blend_op wined3d_blend_op_from_d3d11(D3D11_BLEND_OP op)
{
    return (enum wined3d_blend_op)op;
}

HRESULT d3d_blend_state_create(struct d3d_device *device, const D3D11_BLEND_DESC1 *desc,
        struct d3d_blend_state **state)
{
    struct wined3d_blend_state_desc wined3d_desc;
    struct d3d_blend_state *object;
    struct wine_rb_entry *entry;
    D3D11_BLEND_DESC1 tmp_desc;
    unsigned int i, j;
    HRESULT hr;

    if (!desc)
        return E_INVALIDARG;

    /* D3D11_RENDER_TARGET_BLEND_DESC1 has a hole, which is a problem because we use
     * D3D11_BLEND_DESC1 as a key in the rbtree. */
    memset(&tmp_desc, 0, sizeof(tmp_desc));
    tmp_desc.AlphaToCoverageEnable = desc->AlphaToCoverageEnable;
    tmp_desc.IndependentBlendEnable = desc->IndependentBlendEnable;
    for (i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        j = desc->IndependentBlendEnable ? i : 0;
        tmp_desc.RenderTarget[i].BlendEnable = desc->RenderTarget[j].BlendEnable;
        if (tmp_desc.RenderTarget[i].BlendEnable)
        {
            tmp_desc.RenderTarget[i].SrcBlend = desc->RenderTarget[j].SrcBlend;
            tmp_desc.RenderTarget[i].DestBlend = desc->RenderTarget[j].DestBlend;
            tmp_desc.RenderTarget[i].BlendOp = desc->RenderTarget[j].BlendOp;
            tmp_desc.RenderTarget[i].SrcBlendAlpha = desc->RenderTarget[j].SrcBlendAlpha;
            tmp_desc.RenderTarget[i].DestBlendAlpha = desc->RenderTarget[j].DestBlendAlpha;
            tmp_desc.RenderTarget[i].BlendOpAlpha = desc->RenderTarget[j].BlendOpAlpha;
        }
        else
        {
            tmp_desc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
            tmp_desc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
            tmp_desc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
            tmp_desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
            tmp_desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
            tmp_desc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        }

        tmp_desc.RenderTarget[i].LogicOpEnable = desc->RenderTarget[j].LogicOpEnable;
        if (tmp_desc.RenderTarget[i].LogicOpEnable)
            tmp_desc.RenderTarget[i].LogicOp = desc->RenderTarget[j].LogicOp;
        else
            tmp_desc.RenderTarget[i].LogicOp = D3D11_LOGIC_OP_NOOP;

        tmp_desc.RenderTarget[i].RenderTargetWriteMask = desc->RenderTarget[j].RenderTargetWriteMask;
    }

    wined3d_mutex_lock();
    if ((entry = wine_rb_get(&device->blend_states, &tmp_desc)))
    {
        object = WINE_RB_ENTRY_VALUE(entry, struct d3d_blend_state, entry);

        TRACE("Returning existing blend state %p.\n", object);
        ID3D11BlendState1_AddRef(&object->ID3D11BlendState1_iface);
        *state = object;
        wined3d_mutex_unlock();

        return S_OK;
    }

    if (!(object = calloc(1, sizeof(*object))))
    {
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    object->ID3D11BlendState1_iface.lpVtbl = &d3d11_blend_state_vtbl;
    object->ID3D10BlendState1_iface.lpVtbl = &d3d10_blend_state_vtbl;
    object->refcount = 1;
    wined3d_private_store_init(&object->private_store);
    object->desc = tmp_desc;

    if (wine_rb_put(&device->blend_states, &tmp_desc, &object->entry) == -1)
    {
        ERR("Failed to insert blend state entry.\n");
        wined3d_private_store_cleanup(&object->private_store);
        free(object);
        wined3d_mutex_unlock();
        return E_FAIL;
    }

    wined3d_desc.alpha_to_coverage = desc->AlphaToCoverageEnable;
    wined3d_desc.independent = desc->IndependentBlendEnable;
    for (i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        wined3d_desc.rt[i].enable = desc->RenderTarget[i].BlendEnable;
        wined3d_desc.rt[i].src = wined3d_blend_from_d3d11(desc->RenderTarget[i].SrcBlend);
        wined3d_desc.rt[i].dst = wined3d_blend_from_d3d11(desc->RenderTarget[i].DestBlend);
        wined3d_desc.rt[i].op = wined3d_blend_op_from_d3d11(desc->RenderTarget[i].BlendOp);
        wined3d_desc.rt[i].src_alpha = wined3d_blend_from_d3d11(desc->RenderTarget[i].SrcBlendAlpha);
        wined3d_desc.rt[i].dst_alpha = wined3d_blend_from_d3d11(desc->RenderTarget[i].DestBlendAlpha);
        wined3d_desc.rt[i].op_alpha = wined3d_blend_op_from_d3d11(desc->RenderTarget[i].BlendOpAlpha);
        wined3d_desc.rt[i].writemask = desc->RenderTarget[i].RenderTargetWriteMask;
    }

    if (desc->RenderTarget[0].LogicOpEnable && desc->RenderTarget[0].LogicOp != D3D11_LOGIC_OP_NOOP)
        FIXME("Ignoring logic op %#x.\n", desc->RenderTarget[0].LogicOp);

    /* We cannot fail after creating a wined3d_blend_state object. It
     * would lead to double free. */
    if (FAILED(hr = wined3d_blend_state_create(device->wined3d_device, &wined3d_desc,
            object, &d3d_blend_state_wined3d_parent_ops, &object->wined3d_state)))
    {
        WARN("Failed to create wined3d blend state, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&object->private_store);
        wine_rb_remove(&device->blend_states, &object->entry);
        free(object);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(object->device = &device->ID3D11Device2_iface);

    TRACE("Created blend state %p.\n", object);
    *state = object;

    return S_OK;
}

struct d3d_blend_state *unsafe_impl_from_ID3D11BlendState(ID3D11BlendState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (ID3D11BlendStateVtbl *)&d3d11_blend_state_vtbl);

    return impl_from_ID3D11BlendState1((ID3D11BlendState1 *)iface);
}

struct d3d_blend_state *unsafe_impl_from_ID3D10BlendState(ID3D10BlendState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (ID3D10BlendStateVtbl *)&d3d10_blend_state_vtbl);

    return impl_from_ID3D10BlendState((ID3D10BlendState1 *)iface);
}

/* ID3D11DepthStencilState methods */

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_state_QueryInterface(ID3D11DepthStencilState *iface,
        REFIID riid, void **object)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11DepthStencilState)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11DepthStencilState_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10DepthStencilState)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10DepthStencilState_AddRef(&state->ID3D10DepthStencilState_iface);
        *object = &state->ID3D10DepthStencilState_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_depthstencil_state_AddRef(ID3D11DepthStencilState *iface)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %lu.\n", state, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(state->device);
        wined3d_depth_stencil_state_incref(state->wined3d_state);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_depthstencil_state_Release(ID3D11DepthStencilState *iface)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);
    ULONG refcount = InterlockedDecrement(&state->refcount);

    TRACE("%p decreasing refcount to %lu.\n", state, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = state->device;

        wined3d_depth_stencil_state_decref(state->wined3d_state);
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_depthstencil_state_GetDevice(ID3D11DepthStencilState *iface,
        ID3D11Device **device)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)state->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_state_GetPrivateData(ID3D11DepthStencilState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_state_SetPrivateData(ID3D11DepthStencilState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_depthstencil_state_SetPrivateDataInterface(ID3D11DepthStencilState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_depthstencil_state_GetDesc(ID3D11DepthStencilState *iface,
        D3D11_DEPTH_STENCIL_DESC *desc)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D11DepthStencilState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = state->desc;
}

static const struct ID3D11DepthStencilStateVtbl d3d11_depthstencil_state_vtbl =
{
    /* IUnknown methods */
    d3d11_depthstencil_state_QueryInterface,
    d3d11_depthstencil_state_AddRef,
    d3d11_depthstencil_state_Release,
    /* ID3D11DeviceChild methods */
    d3d11_depthstencil_state_GetDevice,
    d3d11_depthstencil_state_GetPrivateData,
    d3d11_depthstencil_state_SetPrivateData,
    d3d11_depthstencil_state_SetPrivateDataInterface,
    /* ID3D11DepthStencilState methods */
    d3d11_depthstencil_state_GetDesc,
};

/* ID3D10DepthStencilState methods */

static inline struct d3d_depthstencil_state *impl_from_ID3D10DepthStencilState(ID3D10DepthStencilState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_depthstencil_state, ID3D10DepthStencilState_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_state_QueryInterface(ID3D10DepthStencilState *iface,
        REFIID riid, void **object)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_depthstencil_state_QueryInterface(&state->ID3D11DepthStencilState_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_depthstencil_state_AddRef(ID3D10DepthStencilState *iface)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_depthstencil_state_AddRef(&state->ID3D11DepthStencilState_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_depthstencil_state_Release(ID3D10DepthStencilState *iface)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_depthstencil_state_Release(&state->ID3D11DepthStencilState_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_state_GetDevice(ID3D10DepthStencilState *iface, ID3D10Device **device)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(state->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_state_GetPrivateData(ID3D10DepthStencilState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_state_SetPrivateData(ID3D10DepthStencilState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_depthstencil_state_SetPrivateDataInterface(ID3D10DepthStencilState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

/* ID3D10DepthStencilState methods */

static void STDMETHODCALLTYPE d3d10_depthstencil_state_GetDesc(ID3D10DepthStencilState *iface,
        D3D10_DEPTH_STENCIL_DESC *desc)
{
    struct d3d_depthstencil_state *state = impl_from_ID3D10DepthStencilState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(desc, &state->desc, sizeof(*desc));
}

static const struct ID3D10DepthStencilStateVtbl d3d10_depthstencil_state_vtbl =
{
    /* IUnknown methods */
    d3d10_depthstencil_state_QueryInterface,
    d3d10_depthstencil_state_AddRef,
    d3d10_depthstencil_state_Release,
    /* ID3D10DeviceChild methods */
    d3d10_depthstencil_state_GetDevice,
    d3d10_depthstencil_state_GetPrivateData,
    d3d10_depthstencil_state_SetPrivateData,
    d3d10_depthstencil_state_SetPrivateDataInterface,
    /* ID3D10DepthStencilState methods */
    d3d10_depthstencil_state_GetDesc,
};

static void STDMETHODCALLTYPE d3d_depthstencil_state_wined3d_object_destroyed(void *parent)
{
    struct d3d_depthstencil_state *state = parent;
    struct d3d_device *device = impl_from_ID3D11Device2(state->device);

    wine_rb_remove(&device->depthstencil_states, &state->entry);
    wined3d_private_store_cleanup(&state->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d_depthstencil_state_wined3d_parent_ops =
{
    d3d_depthstencil_state_wined3d_object_destroyed,
};

static enum wined3d_cmp_func wined3d_cmp_func_from_d3d11(D3D11_COMPARISON_FUNC func)
{
    return (enum wined3d_cmp_func)func;
}

static enum wined3d_stencil_op wined3d_stencil_op_from_d3d11(D3D11_STENCIL_OP stencil_op)
{
    return (enum wined3d_stencil_op)stencil_op;
}

HRESULT d3d_depthstencil_state_create(struct d3d_device *device, const D3D11_DEPTH_STENCIL_DESC *desc,
        struct d3d_depthstencil_state **state)
{
    struct wined3d_depth_stencil_state_desc wined3d_desc;
    struct d3d_depthstencil_state *object;
    D3D11_DEPTH_STENCIL_DESC tmp_desc;
    struct wine_rb_entry *entry;
    HRESULT hr;

    if (!desc)
        return E_INVALIDARG;

    /* D3D11_DEPTH_STENCIL_DESC has a hole, which is a problem because we use
     * it as a key in the rbtree. */
    memset(&tmp_desc, 0, sizeof(tmp_desc));
    tmp_desc.DepthEnable = desc->DepthEnable;
    if (desc->DepthEnable)
    {
        tmp_desc.DepthWriteMask = desc->DepthWriteMask;
        tmp_desc.DepthFunc = desc->DepthFunc;
    }
    else
    {
        tmp_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        tmp_desc.DepthFunc = D3D11_COMPARISON_LESS;
    }
    tmp_desc.StencilEnable = desc->StencilEnable;
    if (desc->StencilEnable)
    {
        tmp_desc.StencilReadMask = desc->StencilReadMask;
        tmp_desc.StencilWriteMask = desc->StencilWriteMask;
        tmp_desc.FrontFace = desc->FrontFace;
        tmp_desc.BackFace = desc->BackFace;
    }
    else
    {
        tmp_desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        tmp_desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        tmp_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        tmp_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        tmp_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        tmp_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        tmp_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        tmp_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        tmp_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        tmp_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    }

    wined3d_mutex_lock();
    if ((entry = wine_rb_get(&device->depthstencil_states, &tmp_desc)))
    {
        object = WINE_RB_ENTRY_VALUE(entry, struct d3d_depthstencil_state, entry);

        TRACE("Returning existing depthstencil state %p.\n", object);
        ID3D11DepthStencilState_AddRef(&object->ID3D11DepthStencilState_iface);
        *state = object;
        wined3d_mutex_unlock();

        return S_OK;
    }

    if (!(object = calloc(1, sizeof(*object))))
    {
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    object->ID3D11DepthStencilState_iface.lpVtbl = &d3d11_depthstencil_state_vtbl;
    object->ID3D10DepthStencilState_iface.lpVtbl = &d3d10_depthstencil_state_vtbl;
    object->refcount = 1;
    wined3d_private_store_init(&object->private_store);
    object->desc = tmp_desc;

    if (wine_rb_put(&device->depthstencil_states, &tmp_desc, &object->entry) == -1)
    {
        ERR("Failed to insert depth/stencil state entry.\n");
        wined3d_private_store_cleanup(&object->private_store);
        free(object);
        wined3d_mutex_unlock();
        return E_FAIL;
    }

    wined3d_desc.depth = desc->DepthEnable;
    wined3d_desc.depth_write = desc->DepthWriteMask;
    wined3d_desc.depth_func = wined3d_cmp_func_from_d3d11(desc->DepthFunc);
    wined3d_desc.stencil = desc->StencilEnable;
    wined3d_desc.stencil_read_mask = desc->StencilReadMask;
    wined3d_desc.stencil_write_mask = desc->StencilWriteMask;
    wined3d_desc.front.fail_op = wined3d_stencil_op_from_d3d11(desc->FrontFace.StencilFailOp);
    wined3d_desc.front.depth_fail_op = wined3d_stencil_op_from_d3d11(desc->FrontFace.StencilDepthFailOp);
    wined3d_desc.front.pass_op = wined3d_stencil_op_from_d3d11(desc->FrontFace.StencilPassOp);
    wined3d_desc.front.func = wined3d_cmp_func_from_d3d11(desc->FrontFace.StencilFunc);
    wined3d_desc.back.fail_op = wined3d_stencil_op_from_d3d11(desc->BackFace.StencilFailOp);
    wined3d_desc.back.depth_fail_op = wined3d_stencil_op_from_d3d11(desc->BackFace.StencilDepthFailOp);
    wined3d_desc.back.pass_op = wined3d_stencil_op_from_d3d11(desc->BackFace.StencilPassOp);
    wined3d_desc.back.func = wined3d_cmp_func_from_d3d11(desc->BackFace.StencilFunc);

    /* We cannot fail after creating a wined3d_depth_stencil_state object. It
     * would lead to double free. */
    if (FAILED(hr = wined3d_depth_stencil_state_create(device->wined3d_device, &wined3d_desc,
            object, &d3d_depthstencil_state_wined3d_parent_ops, &object->wined3d_state)))
    {
        WARN("Failed to create wined3d depth/stencil state, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&object->private_store);
        wine_rb_remove(&device->depthstencil_states, &object->entry);
        free(object);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(object->device = &device->ID3D11Device2_iface);

    TRACE("Created depth/stencil state %p.\n", object);
    *state = object;

    return S_OK;
}

struct d3d_depthstencil_state *unsafe_impl_from_ID3D11DepthStencilState(ID3D11DepthStencilState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_depthstencil_state_vtbl);

    return impl_from_ID3D11DepthStencilState(iface);
}

struct d3d_depthstencil_state *unsafe_impl_from_ID3D10DepthStencilState(ID3D10DepthStencilState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_depthstencil_state_vtbl);

    return impl_from_ID3D10DepthStencilState(iface);
}

/* ID3D11RasterizerState methods */

static inline struct d3d_rasterizer_state *impl_from_ID3D11RasterizerState1(ID3D11RasterizerState1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_rasterizer_state, ID3D11RasterizerState1_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_rasterizer_state_QueryInterface(ID3D11RasterizerState1 *iface,
        REFIID riid, void **object)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState1(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11RasterizerState)
            || IsEqualGUID(riid, &IID_ID3D11RasterizerState1)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11RasterizerState1_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10RasterizerState)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10RasterizerState_AddRef(&state->ID3D10RasterizerState_iface);
        *object = &state->ID3D10RasterizerState_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_rasterizer_state_AddRef(ID3D11RasterizerState1 *iface)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState1(iface);
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %lu.\n", state, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(state->device);
        wined3d_rasterizer_state_incref(state->wined3d_state);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_rasterizer_state_Release(ID3D11RasterizerState1 *iface)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState1(iface);
    ULONG refcount = InterlockedDecrement(&state->refcount);

    TRACE("%p decreasing refcount to %lu.\n", state, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = state->device;
        wined3d_rasterizer_state_decref(state->wined3d_state);
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_rasterizer_state_GetDevice(ID3D11RasterizerState1 *iface,
        ID3D11Device **device)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState1(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)state->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_rasterizer_state_GetPrivateData(ID3D11RasterizerState1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_rasterizer_state_SetPrivateData(ID3D11RasterizerState1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_rasterizer_state_SetPrivateDataInterface(ID3D11RasterizerState1 *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState1(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_rasterizer_state_GetDesc(ID3D11RasterizerState1 *iface,
        D3D11_RASTERIZER_DESC *desc)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState1(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(desc, &state->desc, sizeof(*desc));
}

static void STDMETHODCALLTYPE d3d11_rasterizer_state_GetDesc1(ID3D11RasterizerState1 *iface,
        D3D11_RASTERIZER_DESC1 *desc)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D11RasterizerState1(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = state->desc;
}

static const struct ID3D11RasterizerState1Vtbl d3d11_rasterizer_state_vtbl =
{
    /* IUnknown methods */
    d3d11_rasterizer_state_QueryInterface,
    d3d11_rasterizer_state_AddRef,
    d3d11_rasterizer_state_Release,
    /* ID3D11DeviceChild methods */
    d3d11_rasterizer_state_GetDevice,
    d3d11_rasterizer_state_GetPrivateData,
    d3d11_rasterizer_state_SetPrivateData,
    d3d11_rasterizer_state_SetPrivateDataInterface,
    /* ID3D11RasterizerState methods */
    d3d11_rasterizer_state_GetDesc,
    /* ID3D11RasterizerState1 methods */
    d3d11_rasterizer_state_GetDesc1,
};

/* ID3D10RasterizerState methods */

static inline struct d3d_rasterizer_state *impl_from_ID3D10RasterizerState(ID3D10RasterizerState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_rasterizer_state, ID3D10RasterizerState_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_rasterizer_state_QueryInterface(ID3D10RasterizerState *iface,
        REFIID riid, void **object)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_rasterizer_state_QueryInterface(&state->ID3D11RasterizerState1_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_rasterizer_state_AddRef(ID3D10RasterizerState *iface)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_rasterizer_state_AddRef(&state->ID3D11RasterizerState1_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_rasterizer_state_Release(ID3D10RasterizerState *iface)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p.\n", state);

    return d3d11_rasterizer_state_Release(&state->ID3D11RasterizerState1_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_rasterizer_state_GetDevice(ID3D10RasterizerState *iface, ID3D10Device **device)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(state->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_rasterizer_state_GetPrivateData(ID3D10RasterizerState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_rasterizer_state_SetPrivateData(ID3D10RasterizerState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_rasterizer_state_SetPrivateDataInterface(ID3D10RasterizerState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

/* ID3D10RasterizerState methods */

static void STDMETHODCALLTYPE d3d10_rasterizer_state_GetDesc(ID3D10RasterizerState *iface,
        D3D10_RASTERIZER_DESC *desc)
{
    struct d3d_rasterizer_state *state = impl_from_ID3D10RasterizerState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(desc, &state->desc, sizeof(*desc));
}

static const struct ID3D10RasterizerStateVtbl d3d10_rasterizer_state_vtbl =
{
    /* IUnknown methods */
    d3d10_rasterizer_state_QueryInterface,
    d3d10_rasterizer_state_AddRef,
    d3d10_rasterizer_state_Release,
    /* ID3D10DeviceChild methods */
    d3d10_rasterizer_state_GetDevice,
    d3d10_rasterizer_state_GetPrivateData,
    d3d10_rasterizer_state_SetPrivateData,
    d3d10_rasterizer_state_SetPrivateDataInterface,
    /* ID3D10RasterizerState methods */
    d3d10_rasterizer_state_GetDesc,
};

static void STDMETHODCALLTYPE d3d_rasterizer_state_wined3d_object_destroyed(void *parent)
{
    struct d3d_rasterizer_state *state = parent;
    struct d3d_device *device = impl_from_ID3D11Device2(state->device);

    wine_rb_remove(&device->rasterizer_states, &state->entry);
    wined3d_private_store_cleanup(&state->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d_rasterizer_state_wined3d_parent_ops =
{
    d3d_rasterizer_state_wined3d_object_destroyed,
};

static enum wined3d_fill_mode wined3d_fill_mode_from_d3d11(D3D11_FILL_MODE mode)
{
    return (enum wined3d_fill_mode)mode;
}

static enum wined3d_cull wined3d_cull_from_d3d11(D3D11_CULL_MODE mode)
{
    return (enum wined3d_cull)mode;
}

static HRESULT d3d_rasterizer_state_init(struct d3d_rasterizer_state *state, struct d3d_device *device,
        const D3D11_RASTERIZER_DESC1 *desc)
{
    struct wined3d_rasterizer_state_desc wined3d_desc;
    HRESULT hr;

    state->ID3D11RasterizerState1_iface.lpVtbl = &d3d11_rasterizer_state_vtbl;
    state->ID3D10RasterizerState_iface.lpVtbl = &d3d10_rasterizer_state_vtbl;
    state->refcount = 1;
    wined3d_private_store_init(&state->private_store);
    state->desc = *desc;

    if (wine_rb_put(&device->rasterizer_states, desc, &state->entry) == -1)
    {
        ERR("Failed to insert rasterizer state entry.\n");
        wined3d_private_store_cleanup(&state->private_store);
        return E_FAIL;
    }

    wined3d_desc.fill_mode = wined3d_fill_mode_from_d3d11(desc->FillMode);
    wined3d_desc.cull_mode = wined3d_cull_from_d3d11(desc->CullMode);
    wined3d_desc.front_ccw = desc->FrontCounterClockwise;
    wined3d_desc.depth_bias = desc->DepthBias;
    wined3d_desc.depth_bias_clamp = desc->DepthBiasClamp;
    wined3d_desc.scale_bias = desc->SlopeScaledDepthBias;
    wined3d_desc.depth_clip = desc->DepthClipEnable;
    wined3d_desc.scissor = desc->ScissorEnable;
    wined3d_desc.line_antialias = desc->AntialiasedLineEnable;

    if (desc->MultisampleEnable)
    {
        static unsigned int once;
        if (!once++)
            FIXME("Ignoring MultisampleEnable %#x.\n", desc->MultisampleEnable);
    }

    if (desc->ForcedSampleCount)
    {
        static unsigned int once;
        if (!once++)
            FIXME("Ignoring ForcedSampleCount %#x.\n", desc->ForcedSampleCount);
    }

    /* We cannot fail after creating a wined3d_rasterizer_state object. It
     * would lead to double free. */
    if (FAILED(hr = wined3d_rasterizer_state_create(device->wined3d_device, &wined3d_desc,
            state, &d3d_rasterizer_state_wined3d_parent_ops, &state->wined3d_state)))
    {
        WARN("Failed to create wined3d rasteriser state, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&state->private_store);
        wine_rb_remove(&device->rasterizer_states, &state->entry);
        return hr;
    }

    ID3D11Device2_AddRef(state->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d_rasterizer_state_create(struct d3d_device *device, const D3D11_RASTERIZER_DESC1 *desc,
        struct d3d_rasterizer_state **state)
{
    struct d3d_rasterizer_state *object;
    struct wine_rb_entry *entry;
    HRESULT hr;

    wined3d_mutex_lock();
    if ((entry = wine_rb_get(&device->rasterizer_states, desc)))
    {
        object = WINE_RB_ENTRY_VALUE(entry, struct d3d_rasterizer_state, entry);

        TRACE("Returning existing rasterizer state %p.\n", object);
        ID3D11RasterizerState1_AddRef(&object->ID3D11RasterizerState1_iface);
        *state = object;
        wined3d_mutex_unlock();

        return S_OK;
    }

    if (!(object = calloc(1, sizeof(*object))))
    {
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    hr = d3d_rasterizer_state_init(object, device, desc);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to initialise rasterizer state, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created rasterizer state %p.\n", object);
    *state = object;

    return S_OK;
}

struct d3d_rasterizer_state *unsafe_impl_from_ID3D11RasterizerState(ID3D11RasterizerState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (ID3D11RasterizerStateVtbl *)&d3d11_rasterizer_state_vtbl);

    return impl_from_ID3D11RasterizerState1((ID3D11RasterizerState1 *)iface);
}

struct d3d_rasterizer_state *unsafe_impl_from_ID3D10RasterizerState(ID3D10RasterizerState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_rasterizer_state_vtbl);

    return impl_from_ID3D10RasterizerState(iface);
}

/* ID3D11SampleState methods */

static inline struct d3d_sampler_state *impl_from_ID3D11SamplerState(ID3D11SamplerState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_sampler_state, ID3D11SamplerState_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_sampler_state_QueryInterface(ID3D11SamplerState *iface,
        REFIID riid, void **object)
{
    struct d3d_sampler_state *state = impl_from_ID3D11SamplerState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11SamplerState)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11SamplerState_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10SamplerState)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10SamplerState_AddRef(&state->ID3D10SamplerState_iface);
        *object = &state->ID3D10SamplerState_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_sampler_state_AddRef(ID3D11SamplerState *iface)
{
    struct d3d_sampler_state *state = impl_from_ID3D11SamplerState(iface);
    ULONG refcount = InterlockedIncrement(&state->refcount);

    TRACE("%p increasing refcount to %lu.\n", state, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(state->device);
        wined3d_sampler_incref(state->wined3d_sampler);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_sampler_state_Release(ID3D11SamplerState *iface)
{
    struct d3d_sampler_state *state = impl_from_ID3D11SamplerState(iface);
    ULONG refcount = InterlockedDecrement(&state->refcount);

    TRACE("%p decreasing refcount to %lu.\n", state, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = state->device;
        wined3d_sampler_decref(state->wined3d_sampler);
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_sampler_state_GetDevice(ID3D11SamplerState *iface,
        ID3D11Device **device)
{
    struct d3d_sampler_state *state = impl_from_ID3D11SamplerState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)state->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_sampler_state_GetPrivateData(ID3D11SamplerState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_sampler_state *state = impl_from_ID3D11SamplerState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_sampler_state_SetPrivateData(ID3D11SamplerState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_sampler_state *state = impl_from_ID3D11SamplerState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_sampler_state_SetPrivateDataInterface(ID3D11SamplerState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_sampler_state *state = impl_from_ID3D11SamplerState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

static void STDMETHODCALLTYPE d3d11_sampler_state_GetDesc(ID3D11SamplerState *iface,
        D3D11_SAMPLER_DESC *desc)
{
    struct d3d_sampler_state *state = impl_from_ID3D11SamplerState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = state->desc;
}

static const struct ID3D11SamplerStateVtbl d3d11_sampler_state_vtbl =
{
    /* IUnknown methods */
    d3d11_sampler_state_QueryInterface,
    d3d11_sampler_state_AddRef,
    d3d11_sampler_state_Release,
    /* ID3D11DeviceChild methods */
    d3d11_sampler_state_GetDevice,
    d3d11_sampler_state_GetPrivateData,
    d3d11_sampler_state_SetPrivateData,
    d3d11_sampler_state_SetPrivateDataInterface,
    /* ID3D11SamplerState methods */
    d3d11_sampler_state_GetDesc,
};

/* ID3D10SamplerState methods */

static inline struct d3d_sampler_state *impl_from_ID3D10SamplerState(ID3D10SamplerState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_sampler_state, ID3D10SamplerState_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_sampler_state_QueryInterface(ID3D10SamplerState *iface,
        REFIID riid, void **object)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_sampler_state_QueryInterface(&state->ID3D11SamplerState_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_sampler_state_AddRef(ID3D10SamplerState *iface)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_sampler_state_AddRef(&state->ID3D11SamplerState_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_sampler_state_Release(ID3D10SamplerState *iface)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_sampler_state_Release(&state->ID3D11SamplerState_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_sampler_state_GetDevice(ID3D10SamplerState *iface, ID3D10Device **device)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(state->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_sampler_state_GetPrivateData(ID3D10SamplerState *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_sampler_state_SetPrivateData(ID3D10SamplerState *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&state->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_sampler_state_SetPrivateDataInterface(ID3D10SamplerState *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&state->private_store, guid, data);
}

/* ID3D10SamplerState methods */

static void STDMETHODCALLTYPE d3d10_sampler_state_GetDesc(ID3D10SamplerState *iface,
        D3D10_SAMPLER_DESC *desc)
{
    struct d3d_sampler_state *state = impl_from_ID3D10SamplerState(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(desc, &state->desc, sizeof(*desc));
}

static const struct ID3D10SamplerStateVtbl d3d10_sampler_state_vtbl =
{
    /* IUnknown methods */
    d3d10_sampler_state_QueryInterface,
    d3d10_sampler_state_AddRef,
    d3d10_sampler_state_Release,
    /* ID3D10DeviceChild methods */
    d3d10_sampler_state_GetDevice,
    d3d10_sampler_state_GetPrivateData,
    d3d10_sampler_state_SetPrivateData,
    d3d10_sampler_state_SetPrivateDataInterface,
    /* ID3D10SamplerState methods */
    d3d10_sampler_state_GetDesc,
};

static void STDMETHODCALLTYPE d3d_sampler_wined3d_object_destroyed(void *parent)
{
    struct d3d_sampler_state *state = parent;
    struct d3d_device *device = impl_from_ID3D11Device2(state->device);

    wine_rb_remove(&device->sampler_states, &state->entry);
    wined3d_private_store_cleanup(&state->private_store);
    free(parent);
}

static const struct wined3d_parent_ops d3d_sampler_wined3d_parent_ops =
{
    d3d_sampler_wined3d_object_destroyed,
};

static enum wined3d_texture_address wined3d_texture_address_from_d3d11(enum D3D11_TEXTURE_ADDRESS_MODE t)
{
    return (enum wined3d_texture_address)t;
}

static enum wined3d_texture_filter_type wined3d_texture_filter_mip_from_d3d11(enum D3D11_FILTER f)
{
    if (D3D11_DECODE_MIP_FILTER(f) == D3D11_FILTER_TYPE_LINEAR)
        return WINED3D_TEXF_LINEAR;
    return WINED3D_TEXF_POINT;
}

static enum wined3d_texture_filter_type wined3d_texture_filter_mag_from_d3d11(enum D3D11_FILTER f)
{
    if (D3D11_DECODE_MAG_FILTER(f) == D3D11_FILTER_TYPE_LINEAR)
        return WINED3D_TEXF_LINEAR;
    return WINED3D_TEXF_POINT;
}

static enum wined3d_texture_filter_type wined3d_texture_filter_min_from_d3d11(enum D3D11_FILTER f)
{
    if (D3D11_DECODE_MIN_FILTER(f) == D3D11_FILTER_TYPE_LINEAR)
        return WINED3D_TEXF_LINEAR;
    return WINED3D_TEXF_POINT;
}

static BOOL wined3d_texture_compare_from_d3d11(enum D3D11_FILTER f)
{
    return D3D11_DECODE_IS_COMPARISON_FILTER(f);
}

static HRESULT d3d_sampler_state_init(struct d3d_sampler_state *state, struct d3d_device *device,
        const D3D11_SAMPLER_DESC *desc)
{
    struct wined3d_sampler_desc wined3d_desc;
    HRESULT hr;

    state->ID3D11SamplerState_iface.lpVtbl = &d3d11_sampler_state_vtbl;
    state->ID3D10SamplerState_iface.lpVtbl = &d3d10_sampler_state_vtbl;
    state->refcount = 1;
    wined3d_private_store_init(&state->private_store);
    state->desc = *desc;

    wined3d_desc.address_u = wined3d_texture_address_from_d3d11(desc->AddressU);
    wined3d_desc.address_v = wined3d_texture_address_from_d3d11(desc->AddressV);
    wined3d_desc.address_w = wined3d_texture_address_from_d3d11(desc->AddressW);
    memcpy(wined3d_desc.border_color, desc->BorderColor, sizeof(wined3d_desc.border_color));
    wined3d_desc.mag_filter = wined3d_texture_filter_mag_from_d3d11(desc->Filter);
    wined3d_desc.min_filter = wined3d_texture_filter_min_from_d3d11(desc->Filter);
    wined3d_desc.mip_filter = wined3d_texture_filter_mip_from_d3d11(desc->Filter);
    wined3d_desc.lod_bias = desc->MipLODBias;
    wined3d_desc.min_lod = desc->MinLOD;
    wined3d_desc.max_lod = max(desc->MinLOD, desc->MaxLOD);
    wined3d_desc.mip_base_level = 0;
    wined3d_desc.max_anisotropy = D3D11_DECODE_IS_ANISOTROPIC_FILTER(desc->Filter) ? desc->MaxAnisotropy : 1;
    wined3d_desc.compare = wined3d_texture_compare_from_d3d11(desc->Filter);
    wined3d_desc.comparison_func = wined3d_cmp_func_from_d3d11(desc->ComparisonFunc);
    wined3d_desc.srgb_decode = TRUE;

    if (wine_rb_put(&device->sampler_states, desc, &state->entry) == -1)
    {
        ERR("Failed to insert sampler state entry.\n");
        wined3d_private_store_cleanup(&state->private_store);
        return E_FAIL;
    }

    /* We cannot fail after creating a wined3d_sampler object. It would lead to
     * double free. */
    if (FAILED(hr = wined3d_sampler_create(device->wined3d_device, &wined3d_desc,
            state, &d3d_sampler_wined3d_parent_ops, &state->wined3d_sampler)))
    {
        WARN("Failed to create wined3d sampler, hr %#lx.\n", hr);
        wined3d_private_store_cleanup(&state->private_store);
        wine_rb_remove(&device->sampler_states, &state->entry);
        return hr;
    }

    ID3D11Device2_AddRef(state->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d_sampler_state_create(struct d3d_device *device, const D3D11_SAMPLER_DESC *desc,
        struct d3d_sampler_state **state)
{
    D3D11_SAMPLER_DESC normalized_desc;
    struct d3d_sampler_state *object;
    struct wine_rb_entry *entry;
    HRESULT hr;

    if (!desc)
        return E_INVALIDARG;

    normalized_desc = *desc;
    if (!D3D11_DECODE_IS_ANISOTROPIC_FILTER(normalized_desc.Filter))
        normalized_desc.MaxAnisotropy = 0;
    if (!D3D11_DECODE_IS_COMPARISON_FILTER(normalized_desc.Filter))
        normalized_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    if (normalized_desc.AddressU != D3D11_TEXTURE_ADDRESS_BORDER
            && normalized_desc.AddressV != D3D11_TEXTURE_ADDRESS_BORDER
            && normalized_desc.AddressW != D3D11_TEXTURE_ADDRESS_BORDER)
        memset(&normalized_desc.BorderColor, 0, sizeof(normalized_desc.BorderColor));

    wined3d_mutex_lock();
    if ((entry = wine_rb_get(&device->sampler_states, &normalized_desc)))
    {
        object = WINE_RB_ENTRY_VALUE(entry, struct d3d_sampler_state, entry);

        TRACE("Returning existing sampler state %p.\n", object);
        ID3D11SamplerState_AddRef(&object->ID3D11SamplerState_iface);
        *state = object;
        wined3d_mutex_unlock();

        return S_OK;
    }

    if (!(object = calloc(1, sizeof(*object))))
    {
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    hr = d3d_sampler_state_init(object, device, &normalized_desc);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to initialise sampler state, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created sampler state %p.\n", object);
    *state = object;

    return S_OK;
}

struct d3d_sampler_state *unsafe_impl_from_ID3D11SamplerState(ID3D11SamplerState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_sampler_state_vtbl);

    return impl_from_ID3D11SamplerState(iface);
}

struct d3d_sampler_state *unsafe_impl_from_ID3D10SamplerState(ID3D10SamplerState *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_sampler_state_vtbl);

    return impl_from_ID3D10SamplerState(iface);
}
