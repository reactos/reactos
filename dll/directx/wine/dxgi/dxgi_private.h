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
 */

#ifndef __WINE_DXGI_PRIVATE_H
#define __WINE_DXGI_PRIVATE_H

#include <wine/config.h>
#include <wine/port.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <objbase.h>
#include <winnls.h>

#include <wine/debug.h>
#include <wine/wined3d.h>
#include <wine/winedxgi.h>

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

extern CRITICAL_SECTION dxgi_cs DECLSPEC_HIDDEN;

/* Layered device */
enum dxgi_device_layer_id
{
    DXGI_DEVICE_LAYER_DEBUG1        = 0x8,
    DXGI_DEVICE_LAYER_THREAD_SAFE   = 0x10,
    DXGI_DEVICE_LAYER_DEBUG2        = 0x20,
    DXGI_DEVICE_LAYER_SWITCH_TO_REF = 0x30,
    DXGI_DEVICE_LAYER_D3D10_DEVICE  = 0xffffffff,
};

struct layer_get_size_args
{
    DWORD unknown0;
    DWORD unknown1;
    DWORD *unknown2;
    DWORD *unknown3;
    IDXGIAdapter *adapter;
    WORD interface_major;
    WORD interface_minor;
    WORD version_build;
    WORD version_revision;
};

struct dxgi_device_layer
{
    enum dxgi_device_layer_id id;
    HRESULT (WINAPI *init)(enum dxgi_device_layer_id id, DWORD *count, DWORD *values);
    UINT (WINAPI *get_size)(enum dxgi_device_layer_id id, struct layer_get_size_args *args, DWORD unknown0);
    HRESULT (WINAPI *create)(enum dxgi_device_layer_id id, void **layer_base, DWORD unknown0,
            void *device_object, REFIID riid, void **device_layer);
};

/* TRACE helper functions */
const char *debug_dxgi_format(DXGI_FORMAT format) DECLSPEC_HIDDEN;

DXGI_FORMAT dxgi_format_from_wined3dformat(enum wined3d_format_id format) DECLSPEC_HIDDEN;
enum wined3d_format_id wined3dformat_from_dxgi_format(DXGI_FORMAT format) DECLSPEC_HIDDEN;
HRESULT dxgi_get_private_data(struct wined3d_private_store *store,
        REFGUID guid, UINT *data_size, void *data) DECLSPEC_HIDDEN;
HRESULT dxgi_set_private_data(struct wined3d_private_store *store,
        REFGUID guid, UINT data_size, const void *data) DECLSPEC_HIDDEN;
HRESULT dxgi_set_private_data_interface(struct wined3d_private_store *store,
        REFGUID guid, const IUnknown *object) DECLSPEC_HIDDEN;

/* IDXGIFactory */
struct dxgi_factory
{
    IDXGIFactory1 IDXGIFactory1_iface;
    LONG refcount;
    struct wined3d_private_store private_store;
    struct wined3d *wined3d;
    UINT adapter_count;
    IDXGIAdapter1 **adapters;
    BOOL extended;
    HWND device_window;
};

HRESULT dxgi_factory_create(REFIID riid, void **factory, BOOL extended) DECLSPEC_HIDDEN;
HWND dxgi_factory_get_device_window(struct dxgi_factory *factory) DECLSPEC_HIDDEN;
struct dxgi_factory *unsafe_impl_from_IDXGIFactory1(IDXGIFactory1 *iface) DECLSPEC_HIDDEN;

/* IDXGIDevice */
struct dxgi_device
{
    IWineDXGIDevice IWineDXGIDevice_iface;
    IUnknown *child_layer;
    LONG refcount;
    struct wined3d_private_store private_store;
    struct wined3d_device *wined3d_device;
    IDXGIFactory1 *factory;
};

HRESULT dxgi_device_init(struct dxgi_device *device, struct dxgi_device_layer *layer,
        IDXGIFactory *factory, IDXGIAdapter *adapter) DECLSPEC_HIDDEN;

/* IDXGIOutput */
struct dxgi_output
{
    IDXGIOutput IDXGIOutput_iface;
    LONG refcount;
    struct wined3d_private_store private_store;
    struct dxgi_adapter *adapter;
};

void dxgi_output_init(struct dxgi_output *output, struct dxgi_adapter *adapter) DECLSPEC_HIDDEN;

/* IDXGIAdapter */
struct dxgi_adapter
{
    IDXGIAdapter1 IDXGIAdapter1_iface;
    struct dxgi_factory *parent;
    LONG refcount;
    struct wined3d_private_store private_store;
    UINT ordinal;
    IDXGIOutput *output;
};

HRESULT dxgi_adapter_init(struct dxgi_adapter *adapter, struct dxgi_factory *parent, UINT ordinal) DECLSPEC_HIDDEN;
struct dxgi_adapter *unsafe_impl_from_IDXGIAdapter1(IDXGIAdapter1 *iface) DECLSPEC_HIDDEN;

/* IDXGISwapChain */
struct dxgi_swapchain
{
    IDXGISwapChain IDXGISwapChain_iface;
    LONG refcount;
    struct wined3d_private_store private_store;
    struct wined3d_swapchain *wined3d_swapchain;
};

HRESULT dxgi_swapchain_init(struct dxgi_swapchain *swapchain, struct dxgi_device *device,
        struct wined3d_swapchain_desc *desc) DECLSPEC_HIDDEN;

/* IDXGISurface */
struct dxgi_surface
{
    IDXGISurface IDXGISurface_iface;
    IUnknown IUnknown_iface;
    IUnknown *outer_unknown;
    LONG refcount;
    struct wined3d_private_store private_store;
    IDXGIDevice *device;

    DXGI_SURFACE_DESC desc;
};

HRESULT dxgi_surface_init(struct dxgi_surface *surface, IDXGIDevice *device,
        IUnknown *outer, const DXGI_SURFACE_DESC *desc) DECLSPEC_HIDDEN;

#endif /* __WINE_DXGI_PRIVATE_H */
