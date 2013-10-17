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

#include "config.h"
#include "wine/port.h"

#define DXGI_INIT_GUID
#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

static CRITICAL_SECTION_DEBUG dxgi_cs_debug =
{
    0, 0, &dxgi_cs,
    {&dxgi_cs_debug.ProcessLocksList,
    &dxgi_cs_debug.ProcessLocksList},
    0, 0, {(DWORD_PTR)(__FILE__ ": dxgi_cs")}
};
CRITICAL_SECTION dxgi_cs = {&dxgi_cs_debug, -1, 0, 0, 0, 0};

struct dxgi_main
{
    HMODULE d3d10core;
    struct dxgi_device_layer *device_layers;
    UINT layer_count;
};
static struct dxgi_main dxgi_main;

static void dxgi_main_cleanup(void)
{
    HeapFree(GetProcessHeap(), 0, dxgi_main.device_layers);
    FreeLibrary(dxgi_main.d3d10core);
    DeleteCriticalSection(&dxgi_cs);
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("fdwReason %u\n", fdwReason);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            break;

        case DLL_PROCESS_DETACH:
            if (lpv) break;
            dxgi_main_cleanup();
            break;
    }

    return TRUE;
}

HRESULT WINAPI CreateDXGIFactory(REFIID riid, void **factory)
{
    struct dxgi_factory *object;
    HRESULT hr;

    TRACE("riid %s, factory %p\n", debugstr_guid(riid), factory);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate DXGI factory object memory\n");
        *factory = NULL;
        return E_OUTOFMEMORY;
    }

    hr = dxgi_factory_init(object);
    if (FAILED(hr))
    {
        WARN("Failed to initialize swapchain, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        *factory = NULL;
        return hr;
    }

    TRACE("Created IDXGIFactory %p\n", object);

    hr = IDXGIFactory_QueryInterface((IDXGIFactory *)object, riid, factory);
    IDXGIFactory_Release((IDXGIFactory *)object);

    return hr;
}

static BOOL get_layer(enum dxgi_device_layer_id id, struct dxgi_device_layer *layer)
{
    UINT i;

    EnterCriticalSection(&dxgi_cs);

    for (i = 0; i < dxgi_main.layer_count; ++i)
    {
        if (dxgi_main.device_layers[i].id == id)
        {
            *layer = dxgi_main.device_layers[i];
            LeaveCriticalSection(&dxgi_cs);
            return TRUE;
        }
    }

    LeaveCriticalSection(&dxgi_cs);
    return FALSE;
}

static HRESULT register_d3d10core_layers(HMODULE d3d10core)
{
    EnterCriticalSection(&dxgi_cs);

    if (!dxgi_main.d3d10core)
    {
        HRESULT hr;
        HRESULT (WINAPI *d3d10core_register_layers)(void);
        HMODULE mod;
        BOOL ret;

        ret = GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)d3d10core, &mod);
        if (!ret)
        {
            LeaveCriticalSection(&dxgi_cs);
            return E_FAIL;
        }

        d3d10core_register_layers = (void *)GetProcAddress(mod, "D3D10CoreRegisterLayers");
        hr = d3d10core_register_layers();
        if (FAILED(hr))
        {
            ERR("Failed to register d3d10core layers, returning %#x\n", hr);
            LeaveCriticalSection(&dxgi_cs);
            return hr;
        }

        dxgi_main.d3d10core = mod;
    }

    LeaveCriticalSection(&dxgi_cs);

    return S_OK;
}

HRESULT WINAPI DXGID3D10CreateDevice(HMODULE d3d10core, IDXGIFactory *factory, IDXGIAdapter *adapter,
        UINT flags, void *unknown0, void **device)
{
    struct layer_get_size_args get_size_args;
    struct dxgi_device *dxgi_device;
    struct dxgi_device_layer d3d10_layer;
    UINT device_size;
    DWORD count;
    HRESULT hr;

    TRACE("d3d10core %p, factory %p, adapter %p, flags %#x, unknown0 %p, device %p.\n",
            d3d10core, factory, adapter, flags, unknown0, device);

    hr = register_d3d10core_layers(d3d10core);
    if (FAILED(hr))
    {
        ERR("Failed to register d3d10core layers, returning %#x\n", hr);
        return hr;
    }

    if (!get_layer(DXGI_DEVICE_LAYER_D3D10_DEVICE, &d3d10_layer))
    {
        ERR("Failed to get D3D10 device layer\n");
        return E_FAIL;
    }

    count = 0;
    hr = d3d10_layer.init(d3d10_layer.id, &count, NULL);
    if (FAILED(hr))
    {
        WARN("Failed to initialize D3D10 device layer\n");
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

    dxgi_device = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, device_size);
    if (!dxgi_device)
    {
        ERR("Failed to allocate device memory\n");
        return E_OUTOFMEMORY;
    }

    hr = dxgi_device_init(dxgi_device, &d3d10_layer, factory, adapter);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, dxgi_device);
        *device = NULL;
        return hr;
    }

    TRACE("Created device %p.\n", dxgi_device);
    *device = dxgi_device;

    return S_OK;
}

HRESULT WINAPI DXGID3D10RegisterLayers(const struct dxgi_device_layer *layers, UINT layer_count)
{
    UINT i;
    struct dxgi_device_layer *new_layers;

    TRACE("layers %p, layer_count %u\n", layers, layer_count);

    EnterCriticalSection(&dxgi_cs);

    if (!dxgi_main.layer_count)
        new_layers = HeapAlloc(GetProcessHeap(), 0, layer_count * sizeof(*new_layers));
    else
        new_layers = HeapReAlloc(GetProcessHeap(), 0, dxgi_main.device_layers,
                (dxgi_main.layer_count + layer_count) * sizeof(*new_layers));

    if (!new_layers)
    {
        LeaveCriticalSection(&dxgi_cs);
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

    LeaveCriticalSection(&dxgi_cs);

    return S_OK;
}
