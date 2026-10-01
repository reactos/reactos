/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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

#define DXGI_INIT_GUID
#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

struct dxgi_main
{
    HMODULE d3d10core;
    struct dxgi_device_layer *device_layers;
    UINT layer_count;
};
static struct dxgi_main dxgi_main;

static void dxgi_main_cleanup(void)
{
    free(dxgi_main.device_layers);
    FreeLibrary(dxgi_main.d3d10core);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(inst);
            break;

        case DLL_PROCESS_DETACH:
            if (!reserved)
                dxgi_main_cleanup();
            break;
    }

    return TRUE;
}

HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID iid, void **factory)
{
    TRACE("flags %#x, iid %s, factory %p.\n", flags, debugstr_guid(iid), factory);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    return dxgi_factory_create(iid, factory, TRUE);
}

HRESULT WINAPI CreateDXGIFactory1(REFIID iid, void **factory)
{
    TRACE("iid %s, factory %p.\n", debugstr_guid(iid), factory);

    return dxgi_factory_create(iid, factory, TRUE);
}

HRESULT WINAPI CreateDXGIFactory(REFIID iid, void **factory)
{
    TRACE("iid %s, factory %p.\n", debugstr_guid(iid), factory);

    return dxgi_factory_create(iid, factory, FALSE);
}

static BOOL get_layer(enum dxgi_device_layer_id id, struct dxgi_device_layer *layer)
{
    UINT i;

    wined3d_mutex_lock();

    for (i = 0; i < dxgi_main.layer_count; ++i)
    {
        if (dxgi_main.device_layers[i].id == id)
        {
            *layer = dxgi_main.device_layers[i];
            wined3d_mutex_unlock();
            return TRUE;
        }
    }

    wined3d_mutex_unlock();
    return FALSE;
}

static HRESULT register_d3d10core_layers(HMODULE d3d10core)
{
    wined3d_mutex_lock();

    if (!dxgi_main.d3d10core)
    {
        HRESULT hr;
        HRESULT (WINAPI *d3d11core_register_layers)(void);
        HMODULE mod;
        BOOL ret;

        if (!(ret = GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (const char *)d3d10core, &mod)))
        {
            wined3d_mutex_unlock();
            return E_FAIL;
        }

        d3d11core_register_layers = (void *)GetProcAddress(mod, "D3D11CoreRegisterLayers");
        hr = d3d11core_register_layers();
        if (FAILED(hr))
        {
            ERR("Failed to register d3d11 layers, returning %#lx.\n", hr);
            FreeLibrary(mod);
            wined3d_mutex_unlock();
            return hr;
        }

        dxgi_main.d3d10core = mod;
    }

    wined3d_mutex_unlock();

    return S_OK;
}

HRESULT WINAPI DXGID3D10CreateDevice(HMODULE d3d10core, IDXGIFactory *factory, IDXGIAdapter *adapter,
        unsigned int flags, const D3D_FEATURE_LEVEL *feature_levels, unsigned int level_count, void **device)
{
    struct layer_get_size_args get_size_args;
    struct dxgi_device_layer d3d10_layer;
    struct dxgi_device *dxgi_device;
    UINT device_size;
    DWORD count;
    HRESULT hr;

    TRACE("d3d10core %p, factory %p, adapter %p, flags %#x, feature_levels %p, level_count %u, device %p.\n",
            d3d10core, factory, adapter, flags, feature_levels, level_count, device);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    if (TRACE_ON(dxgi))
        dump_feature_levels(feature_levels, level_count);

    hr = register_d3d10core_layers(d3d10core);
    if (FAILED(hr))
    {
        ERR("Failed to register d3d10core layers, returning %#lx.\n", hr);
        return hr;
    }

    if (!get_layer(DXGI_DEVICE_LAYER_D3D10_DEVICE, &d3d10_layer))
    {
        ERR("Failed to get D3D10 device layer.\n");
        return E_FAIL;
    }

    count = 0;
    hr = d3d10_layer.init(d3d10_layer.id, &count, NULL);
    if (FAILED(hr))
    {
        WARN("Failed to initialize D3D10 device layer.\n");
        return E_FAIL;
    }

    get_size_args.unknown0 = 0;
    get_size_args.unknown1 = 0;
    get_size_args.unknown2 = NULL;
    get_size_args.unknown3 = NULL;
    get_size_args.adapter = adapter;
    get_size_args.interface_major = 10;
    get_size_args.interface_minor = 1;
    get_size_args.version_build = 4;
    get_size_args.version_revision = 6000;

    device_size = d3d10_layer.get_size(d3d10_layer.id, &get_size_args, 0);
    device_size += sizeof(*dxgi_device);

    if (!(dxgi_device = calloc(1, device_size)))
    {
        ERR("Failed to allocate device memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = dxgi_device_init(dxgi_device, &d3d10_layer, factory, adapter, feature_levels, level_count);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#lx.\n", hr);
        free(dxgi_device);
        *device = NULL;
        return hr;
    }

    TRACE("Created device %p.\n", dxgi_device);
    *device = &dxgi_device->IWineDXGIDevice_iface;

    return S_OK;
}

HRESULT WINAPI DXGID3D10RegisterLayers(const struct dxgi_device_layer *layers, UINT layer_count)
{
    UINT i;
    struct dxgi_device_layer *new_layers;

    TRACE("layers %p, layer_count %u\n", layers, layer_count);

    wined3d_mutex_lock();

    new_layers = realloc(dxgi_main.device_layers,
            (dxgi_main.layer_count + layer_count) * sizeof(*new_layers));

    if (!new_layers)
    {
        wined3d_mutex_unlock();
        ERR("Failed to allocate layer memory\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < layer_count; ++i)
    {
        const struct dxgi_device_layer *layer = &layers[i];

        TRACE("layer %d: id %#x, init %p, get_size %p, create %p\n",
                i, layer->id, layer->init, layer->get_size, layer->create);

        new_layers[dxgi_main.layer_count + i] = *layer;
    }

    dxgi_main.device_layers = new_layers;
    dxgi_main.layer_count += layer_count;

    wined3d_mutex_unlock();

    return S_OK;
}

HRESULT WINAPI DXGIGetDebugInterface1(UINT flags, REFIID iid, void **debug)
{
    TRACE("flags %#x, iid %s, debug %p.\n", flags, debugstr_guid(iid), debug);

    WARN("Returning DXGI_ERROR_SDK_COMPONENT_MISSING.\n");
    return DXGI_ERROR_SDK_COMPONENT_MISSING;
}
