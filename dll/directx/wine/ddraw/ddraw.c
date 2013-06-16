/*
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2006 Stefan Dösinger
 * Copyright 2008 Denver Gingerich
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

#include <config.h>
//#include "wine/port.h"

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static struct wined3d_display_mode original_mode;
static const struct ddraw *exclusive_ddraw;
static BOOL restore_mode;

/* Device identifier. Don't relay it to WineD3D */
static const DDDEVICEIDENTIFIER2 deviceidentifier =
{
    "display",
    "DirectDraw HAL",
    { { 0x00010001, 0x00010001 } },
    0, 0, 0, 0,
    /* a8373c10-7ac4-4deb-849a-009844d08b2d */
    {0xa8373c10,0x7ac4,0x4deb, {0x84,0x9a,0x00,0x98,0x44,0xd0,0x8b,0x2d}},
    0
};

static struct enum_device_entry
{
    char interface_name[100];
    char device_name[100];
    const GUID *device_guid;
} device_list7[] =
{
    /* T&L HAL device */
    {
        "WINE Direct3D7 Hardware Transform and Lighting acceleration using WineD3D",
        "Wine D3D7 T&L HAL",
        &IID_IDirect3DTnLHalDevice,
    },

    /* HAL device */
    {
        "WINE Direct3D7 Hardware acceleration using WineD3D",
        "Direct3D HAL",
        &IID_IDirect3DHALDevice,
    },

    /* RGB device */
    {
        "WINE Direct3D7 RGB Software Emulation using WineD3D",
        "Wine D3D7 RGB",
        &IID_IDirect3DRGBDevice,
    },
};

static void STDMETHODCALLTYPE ddraw_null_wined3d_object_destroyed(void *parent) {}

const struct wined3d_parent_ops ddraw_null_wined3d_parent_ops =
{
    ddraw_null_wined3d_object_destroyed,
};

static inline struct ddraw *impl_from_IDirectDraw(IDirectDraw *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw, IDirectDraw_iface);
}

static inline struct ddraw *impl_from_IDirectDraw2(IDirectDraw2 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw, IDirectDraw2_iface);
}

static inline struct ddraw *impl_from_IDirectDraw4(IDirectDraw4 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw, IDirectDraw4_iface);
}

static inline struct ddraw *impl_from_IDirectDraw7(IDirectDraw7 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw, IDirectDraw7_iface);
}

static inline struct ddraw *impl_from_IDirect3D(IDirect3D *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw, IDirect3D_iface);
}

static inline struct ddraw *impl_from_IDirect3D2(IDirect3D2 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw, IDirect3D2_iface);
}

static inline struct ddraw *impl_from_IDirect3D3(IDirect3D3 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw, IDirect3D3_iface);
}

static inline struct ddraw *impl_from_IDirect3D7(IDirect3D7 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw, IDirect3D7_iface);
}

static HRESULT WINAPI ddraw7_QueryInterface(IDirectDraw7 *iface, REFIID riid, void **out)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (!riid)
    {
        *out = NULL;
        return DDERR_INVALIDPARAMS;
    }

    /* The refcount unit test revealed that an IDirect3D7 interface can only
     * be queried from a DirectDraw object that was created as an IDirectDraw7
     * interface. The older interfaces can query any IDirect3D version except
     * 7, because they are all initially created as IDirectDraw. This isn't
     * really crucial behavior, and messy to implement with the common
     * creation function, so it has been left out here. */
    if (IsEqualGUID(&IID_IDirectDraw7, riid)
            || IsEqualGUID(&IID_IUnknown, riid))
    {
        *out = &ddraw->IDirectDraw7_iface;
        TRACE("Returning IDirectDraw7 interface %p.\n", *out);
    }
    else if (IsEqualGUID(&IID_IDirectDraw4, riid))
    {
        *out = &ddraw->IDirectDraw4_iface;
        TRACE("Returning IDirectDraw4 interface %p.\n", *out);
    }
    else if (IsEqualGUID(&IID_IDirectDraw2, riid))
    {
        *out = &ddraw->IDirectDraw2_iface;
        TRACE("Returning IDirectDraw2 interface %p.\n", *out);
    }
    else if (IsEqualGUID(&IID_IDirectDraw, riid))
    {
        *out = &ddraw->IDirectDraw_iface;
        TRACE("Returning IDirectDraw interface %p.\n", *out);
    }
    else if (IsEqualGUID(&IID_IDirect3D7, riid))
    {
        ddraw->d3dversion = 7;
        *out = &ddraw->IDirect3D7_iface;
        TRACE("Returning Direct3D7 interface %p.\n", *out);
    }
    else if (IsEqualGUID(&IID_IDirect3D3, riid))
    {
        ddraw->d3dversion = 3;
        *out = &ddraw->IDirect3D3_iface;
        TRACE("Returning Direct3D3 interface %p.\n", *out);
    }
    else if (IsEqualGUID(&IID_IDirect3D2, riid))
    {
        ddraw->d3dversion = 2;
        *out = &ddraw->IDirect3D2_iface;
        TRACE("Returning Direct3D2 interface %p.\n", *out);
    }
    else if (IsEqualGUID(&IID_IDirect3D, riid))
    {
        ddraw->d3dversion = 1;
        *out = &ddraw->IDirect3D_iface;
        TRACE("Returning Direct3D interface %p.\n", *out);
    }
    /* Unknown interface */
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI ddraw4_QueryInterface(IDirectDraw4 *iface, REFIID riid, void **object)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&ddraw->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI ddraw2_QueryInterface(IDirectDraw2 *iface, REFIID riid, void **object)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&ddraw->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI ddraw1_QueryInterface(IDirectDraw *iface, REFIID riid, void **object)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&ddraw->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI d3d7_QueryInterface(IDirect3D7 *iface, REFIID riid, void **object)
{
    struct ddraw *ddraw = impl_from_IDirect3D7(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&ddraw->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI d3d3_QueryInterface(IDirect3D3 *iface, REFIID riid, void **object)
{
    struct ddraw *ddraw = impl_from_IDirect3D3(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&ddraw->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI d3d2_QueryInterface(IDirect3D2 *iface, REFIID riid, void **object)
{
    struct ddraw *ddraw = impl_from_IDirect3D2(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&ddraw->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI d3d1_QueryInterface(IDirect3D *iface, REFIID riid, void **object)
{
    struct ddraw *ddraw = impl_from_IDirect3D(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&ddraw->IDirectDraw7_iface, riid, object);
}

/*****************************************************************************
 * IDirectDraw7::AddRef
 *
 * Increases the interfaces refcount, basically
 *
 * DDraw refcounting is a bit tricky. The different DirectDraw interface
 * versions have individual refcounts, but the IDirect3D interfaces do not.
 * All interfaces are from one object, that means calling QueryInterface on an
 * IDirectDraw7 interface for an IDirectDraw4 interface does not create a new
 * ddraw object.
 *
 * That means all AddRef and Release implementations of IDirectDrawX work
 * with their own counter, and IDirect3DX::AddRef thunk to IDirectDraw (1),
 * except of IDirect3D7 which thunks to IDirectDraw7
 *
 * Returns: The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI ddraw7_AddRef(IDirectDraw7 *iface)
{
    struct ddraw *This = impl_from_IDirectDraw7(iface);
    ULONG ref = InterlockedIncrement(&This->ref7);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    if(ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI ddraw4_AddRef(IDirectDraw4 *iface)
{
    struct ddraw *This = impl_from_IDirectDraw4(iface);
    ULONG ref = InterlockedIncrement(&This->ref4);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    if (ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI ddraw2_AddRef(IDirectDraw2 *iface)
{
    struct ddraw *This = impl_from_IDirectDraw2(iface);
    ULONG ref = InterlockedIncrement(&This->ref2);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    if (ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI ddraw1_AddRef(IDirectDraw *iface)
{
    struct ddraw *This = impl_from_IDirectDraw(iface);
    ULONG ref = InterlockedIncrement(&This->ref1);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    if (ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI d3d7_AddRef(IDirect3D7 *iface)
{
    struct ddraw *This = impl_from_IDirect3D7(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_AddRef(&This->IDirectDraw7_iface);
}

static ULONG WINAPI d3d3_AddRef(IDirect3D3 *iface)
{
    struct ddraw *This = impl_from_IDirect3D3(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_AddRef(&This->IDirectDraw_iface);
}

static ULONG WINAPI d3d2_AddRef(IDirect3D2 *iface)
{
    struct ddraw *This = impl_from_IDirect3D2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_AddRef(&This->IDirectDraw_iface);
}

static ULONG WINAPI d3d1_AddRef(IDirect3D *iface)
{
    struct ddraw *This = impl_from_IDirect3D(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_AddRef(&This->IDirectDraw_iface);
}

void ddraw_destroy_swapchain(struct ddraw *ddraw)
{
    TRACE("Destroying the swapchain.\n");

    wined3d_swapchain_decref(ddraw->wined3d_swapchain);
    ddraw->wined3d_swapchain = NULL;

    if (DefaultSurfaceType == DDRAW_SURFACE_TYPE_OPENGL)
    {
        UINT i;

        for (i = 0; i < ddraw->numConvertedDecls; ++i)
        {
            wined3d_vertex_declaration_decref(ddraw->decls[i].decl);
        }
        HeapFree(GetProcessHeap(), 0, ddraw->decls);
        ddraw->numConvertedDecls = 0;

        if (FAILED(wined3d_device_uninit_3d(ddraw->wined3d_device)))
        {
            ERR("Failed to uninit 3D.\n");
        }
        else
        {
            /* Free the d3d window if one was created. */
            if (ddraw->d3d_window && ddraw->d3d_window != ddraw->dest_window)
            {
                TRACE("Destroying the hidden render window %p.\n", ddraw->d3d_window);
                DestroyWindow(ddraw->d3d_window);
                ddraw->d3d_window = 0;
            }
        }

        ddraw->d3d_initialized = FALSE;
    }
    else
    {
        wined3d_device_uninit_gdi(ddraw->wined3d_device);
    }

    ddraw_set_swapchain_window(ddraw, NULL);

    TRACE("Swapchain destroyed.\n");
}

/*****************************************************************************
 * ddraw_destroy
 *
 * Destroys a ddraw object if all refcounts are 0. This is to share code
 * between the IDirectDrawX::Release functions
 *
 * Params:
 *  This: DirectDraw object to destroy
 *
 *****************************************************************************/
static void ddraw_destroy(struct ddraw *This)
{
    IDirectDraw7_SetCooperativeLevel(&This->IDirectDraw7_iface, NULL, DDSCL_NORMAL);
    IDirectDraw7_RestoreDisplayMode(&This->IDirectDraw7_iface);

    /* Destroy the device window if we created one */
    if(This->devicewindow != 0)
    {
        TRACE(" (%p) Destroying the device window %p\n", This, This->devicewindow);
        DestroyWindow(This->devicewindow);
        This->devicewindow = 0;
    }

    wined3d_mutex_lock();
    list_remove(&This->ddraw_list_entry);
    wined3d_mutex_unlock();

    if (This->wined3d_swapchain)
        ddraw_destroy_swapchain(This);
    wined3d_device_decref(This->wined3d_device);
    wined3d_decref(This->wined3d);

    /* Now free the object */
    HeapFree(GetProcessHeap(), 0, This);
}

/*****************************************************************************
 * IDirectDraw7::Release
 *
 * Decreases the refcount. If the refcount falls to 0, the object is destroyed
 *
 * Returns: The new refcount
 *****************************************************************************/
static ULONG WINAPI ddraw7_Release(IDirectDraw7 *iface)
{
    struct ddraw *This = impl_from_IDirectDraw7(iface);
    ULONG ref = InterlockedDecrement(&This->ref7);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        ddraw_destroy(This);

    return ref;
}

static ULONG WINAPI ddraw4_Release(IDirectDraw4 *iface)
{
    struct ddraw *This = impl_from_IDirectDraw4(iface);
    ULONG ref = InterlockedDecrement(&This->ref4);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        ddraw_destroy(This);

    return ref;
}

static ULONG WINAPI ddraw2_Release(IDirectDraw2 *iface)
{
    struct ddraw *This = impl_from_IDirectDraw2(iface);
    ULONG ref = InterlockedDecrement(&This->ref2);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        ddraw_destroy(This);

    return ref;
}

static ULONG WINAPI ddraw1_Release(IDirectDraw *iface)
{
    struct ddraw *This = impl_from_IDirectDraw(iface);
    ULONG ref = InterlockedDecrement(&This->ref1);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        ddraw_destroy(This);

    return ref;
}

static ULONG WINAPI d3d7_Release(IDirect3D7 *iface)
{
    struct ddraw *This = impl_from_IDirect3D7(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_Release(&This->IDirectDraw7_iface);
}

static ULONG WINAPI d3d3_Release(IDirect3D3 *iface)
{
    struct ddraw *This = impl_from_IDirect3D3(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_Release(&This->IDirectDraw_iface);
}

static ULONG WINAPI d3d2_Release(IDirect3D2 *iface)
{
    struct ddraw *This = impl_from_IDirect3D2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_Release(&This->IDirectDraw_iface);
}

static ULONG WINAPI d3d1_Release(IDirect3D *iface)
{
    struct ddraw *This = impl_from_IDirect3D(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_Release(&This->IDirectDraw_iface);
}

/*****************************************************************************
 * IDirectDraw methods
 *****************************************************************************/

static HRESULT ddraw_set_focus_window(struct ddraw *ddraw, HWND window)
{
    /* FIXME: This looks wrong, exclusive mode should imply a destination
     * window. */
    if ((ddraw->cooperative_level & DDSCL_EXCLUSIVE) && ddraw->dest_window)
    {
        TRACE("Setting DDSCL_SETFOCUSWINDOW with an already set window, returning DDERR_HWNDALREADYSET.\n");
        return DDERR_HWNDALREADYSET;
    }

    ddraw->focuswindow = window;

    return DD_OK;
}

static HRESULT ddraw_attach_d3d_device(struct ddraw *ddraw,
        struct wined3d_swapchain_desc *swapchain_desc)
{
    HWND window = swapchain_desc->device_window;
    HRESULT hr;

    TRACE("ddraw %p.\n", ddraw);

    if (!window || window == GetDesktopWindow())
    {
        window = CreateWindowExA(0, DDRAW_WINDOW_CLASS_NAME, "Hidden D3D Window",
                WS_DISABLED, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                NULL, NULL, NULL, NULL);
        if (!window)
        {
            ERR("Failed to create window, last error %#x.\n", GetLastError());
            return E_FAIL;
        }

        ShowWindow(window, SW_HIDE);   /* Just to be sure */
        WARN("No window for the Direct3DDevice, created hidden window %p.\n", window);

        swapchain_desc->device_window = window;
    }
    else
    {
        TRACE("Using existing window %p for Direct3D rendering.\n", window);
    }
    ddraw->d3d_window = window;

    /* Set this NOW, otherwise creating the depth stencil surface will cause a
     * recursive loop until ram or emulated video memory is full. */
    ddraw->d3d_initialized = TRUE;
    hr = wined3d_device_init_3d(ddraw->wined3d_device, swapchain_desc);
    if (FAILED(hr))
    {
        ddraw->d3d_initialized = FALSE;
        return hr;
    }

    ddraw->declArraySize = 2;
    ddraw->decls = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ddraw->decls) * ddraw->declArraySize);
    if (!ddraw->decls)
    {
        ERR("Error allocating an array for the converted vertex decls.\n");
        ddraw->declArraySize = 0;
        hr = wined3d_device_uninit_3d(ddraw->wined3d_device);
        return E_OUTOFMEMORY;
    }

    TRACE("Successfully initialized 3D.\n");

    return DD_OK;
}

static HRESULT ddraw_create_swapchain(struct ddraw *ddraw, HWND window, BOOL windowed)
{
    struct wined3d_swapchain_desc swapchain_desc;
    struct wined3d_display_mode mode;
    HRESULT hr = WINED3D_OK;

    if (FAILED(hr = wined3d_get_adapter_display_mode(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &mode, NULL)))
    {
        ERR("Failed to get display mode.\n");
        return hr;
    }

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    swapchain_desc.backbuffer_width = mode.width;
    swapchain_desc.backbuffer_height = mode.height;
    swapchain_desc.backbuffer_format = mode.format_id;
    swapchain_desc.swap_effect = WINED3D_SWAP_EFFECT_COPY;
    swapchain_desc.device_window = window;
    swapchain_desc.windowed = windowed;

    if (DefaultSurfaceType == DDRAW_SURFACE_TYPE_OPENGL)
        hr = ddraw_attach_d3d_device(ddraw, &swapchain_desc);
    else
        hr = wined3d_device_init_gdi(ddraw->wined3d_device, &swapchain_desc);

    if (FAILED(hr))
    {
        ERR("Failed to create swapchain, hr %#x.\n", hr);
        return hr;
    }

    if (!(ddraw->wined3d_swapchain = wined3d_device_get_swapchain(ddraw->wined3d_device, 0)))
    {
        ERR("Failed to get swapchain.\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_swapchain_incref(ddraw->wined3d_swapchain);
    ddraw_set_swapchain_window(ddraw, window);

    return DD_OK;
}

/*****************************************************************************
 * IDirectDraw7::SetCooperativeLevel
 *
 * Sets the cooperative level for the DirectDraw object, and the window
 * assigned to it. The cooperative level determines the general behavior
 * of the DirectDraw application
 *
 * Warning: This is quite tricky, as it's not really documented which
 * cooperative levels can be combined with each other. If a game fails
 * after this function, try to check the cooperative levels passed on
 * Windows, and if it returns something different.
 *
 * If you think that this function caused the failure because it writes a
 * fixme, be sure to run again with a +ddraw trace.
 *
 * What is known about cooperative levels (See the ddraw modes test):
 * DDSCL_EXCLUSIVE and DDSCL_FULLSCREEN must be used with each other
 * DDSCL_NORMAL is not compatible with DDSCL_EXCLUSIVE or DDSCL_FULLSCREEN
 * DDSCL_SETFOCUSWINDOW can be passed only in DDSCL_NORMAL mode, but after that
 * DDSCL_FULLSCREEN can be activated
 * DDSCL_SETFOCUSWINDOW may only be used with DDSCL_NOWINDOWCHANGES
 *
 * Handled flags: DDSCL_NORMAL, DDSCL_FULLSCREEN, DDSCL_EXCLUSIVE,
 *                DDSCL_SETFOCUSWINDOW (partially),
 *                DDSCL_MULTITHREADED (work in progress)
 *
 * Unhandled flags, which should be implemented
 *  DDSCL_SETDEVICEWINDOW: Sets a window specially used for rendering (I don't
 *  expect any difference to a normal window for wine)
 *  DDSCL_CREATEDEVICEWINDOW: Tells ddraw to create its own window for
 *  rendering (Possible test case: Half-Life)
 *
 * Unsure about these: DDSCL_FPUSETUP DDSCL_FPURESERVE
 *
 * These don't seem very important for wine:
 *  DDSCL_ALLOWREBOOT, DDSCL_NOWINDOWCHANGES, DDSCL_ALLOWMODEX
 *
 * Returns:
 *  DD_OK if the cooperative level was set successfully
 *  DDERR_INVALIDPARAMS if the passed cooperative level combination is invalid
 *  DDERR_HWNDALREADYSET if DDSCL_SETFOCUSWINDOW is passed in exclusive mode
 *   (Probably others too, have to investigate)
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_SetCooperativeLevel(IDirectDraw7 *iface, HWND window, DWORD cooplevel)
{
    struct ddraw *This = impl_from_IDirectDraw7(iface);
    struct wined3d_surface *rt = NULL, *ds = NULL;
    struct wined3d_stateblock *stateblock;
    BOOL restore_state = FALSE;
    HRESULT hr;

    TRACE("iface %p, window %p, flags %#x.\n", iface, window, cooplevel);
    DDRAW_dump_cooperativelevel(cooplevel);

    wined3d_mutex_lock();

    /* Tests suggest that we need one of them: */
    if(!(cooplevel & (DDSCL_SETFOCUSWINDOW |
                      DDSCL_NORMAL         |
                      DDSCL_EXCLUSIVE      )))
    {
        TRACE("Incorrect cooplevel flags, returning DDERR_INVALIDPARAMS\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if ((cooplevel & DDSCL_CREATEDEVICEWINDOW) && !(cooplevel & DDSCL_EXCLUSIVE))
    {
        WARN("DDSCL_CREATEDEVICEWINDOW requires DDSCL_EXCLUSIVE.\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    /* Handle those levels first which set various hwnds */
    if ((cooplevel & DDSCL_SETFOCUSWINDOW) && !(cooplevel & DDSCL_CREATEDEVICEWINDOW))
    {
        /* This isn't compatible with a lot of flags */
        if (cooplevel & (DDSCL_MULTITHREADED
                | DDSCL_FPUSETUP
                | DDSCL_FPUPRESERVE
                | DDSCL_ALLOWREBOOT
                | DDSCL_ALLOWMODEX
                | DDSCL_SETDEVICEWINDOW
                | DDSCL_NORMAL
                | DDSCL_EXCLUSIVE
                | DDSCL_FULLSCREEN))
        {
            WARN("Called with incompatible flags, returning DDERR_INVALIDPARAMS.\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }

        hr = ddraw_set_focus_window(This, window);
        wined3d_mutex_unlock();
        return hr;
    }

    if (cooplevel & DDSCL_EXCLUSIVE)
    {
        if (!(cooplevel & DDSCL_FULLSCREEN) || !(window || (cooplevel & DDSCL_CREATEDEVICEWINDOW)))
        {
            WARN("DDSCL_EXCLUSIVE requires DDSCL_FULLSCREEN and a window.\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }

        if (cooplevel & DDSCL_CREATEDEVICEWINDOW)
        {
            HWND device_window;

            if (!This->focuswindow && !(cooplevel & DDSCL_SETFOCUSWINDOW))
            {
                WARN("No focus window set.\n");
                wined3d_mutex_unlock();
                return DDERR_NOFOCUSWINDOW;
            }

            device_window = CreateWindowExA(0, DDRAW_WINDOW_CLASS_NAME, "DirectDrawDeviceWnd",
                    WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                    NULL, NULL, NULL, NULL);
            if (!device_window)
            {
                ERR("Failed to create window, last error %#x.\n", GetLastError());
                wined3d_mutex_unlock();
                return E_FAIL;
            }

            ShowWindow(device_window, SW_SHOW);
            TRACE("Created a device window %p.\n", device_window);

            /* Native apparently leaks the created device window if setting the
             * focus window below fails. */
            This->cooperative_level |= DDSCL_CREATEDEVICEWINDOW;
            This->devicewindow = device_window;

            if (cooplevel & DDSCL_SETFOCUSWINDOW)
            {
                if (!window)
                {
                    wined3d_mutex_unlock();
                    return DDERR_NOHWND;
                }

                if (FAILED(hr = ddraw_set_focus_window(This, window)))
                {
                    wined3d_mutex_unlock();
                    return hr;
                }
            }

            window = device_window;
        }
    }
    else
    {
        if (This->cooperative_level & DDSCL_CREATEDEVICEWINDOW)
            DestroyWindow(This->devicewindow);
        This->devicewindow = NULL;
        This->focuswindow = NULL;
    }

    if ((This->cooperative_level & DDSCL_EXCLUSIVE)
            && (window != This->dest_window || !(cooplevel & DDSCL_EXCLUSIVE)))
        wined3d_device_release_focus_window(This->wined3d_device);

    if ((cooplevel & DDSCL_FULLSCREEN) != (This->cooperative_level & DDSCL_FULLSCREEN) || window != This->dest_window)
    {
        if (This->cooperative_level & DDSCL_FULLSCREEN)
            wined3d_device_restore_fullscreen_window(This->wined3d_device, This->dest_window);

        if (cooplevel & DDSCL_FULLSCREEN)
        {
            struct wined3d_display_mode display_mode;

            wined3d_get_adapter_display_mode(This->wined3d, WINED3DADAPTER_DEFAULT, &display_mode, NULL);
            wined3d_device_setup_fullscreen_window(This->wined3d_device, window,
                    display_mode.width, display_mode.height);
        }
    }

    if ((cooplevel & DDSCL_EXCLUSIVE)
            && (window != This->dest_window || !(This->cooperative_level & DDSCL_EXCLUSIVE)))
    {
        hr = wined3d_device_acquire_focus_window(This->wined3d_device, window);
        if (FAILED(hr))
        {
            ERR("Failed to acquire focus window, hr %#x.\n", hr);
            wined3d_mutex_unlock();
            return hr;
        }
    }

    if (cooplevel & DDSCL_MULTITHREADED && !(This->cooperative_level & DDSCL_MULTITHREADED))
        wined3d_device_set_multithreaded(This->wined3d_device);

    if (This->wined3d_swapchain)
    {
        if (DefaultSurfaceType != DDRAW_SURFACE_TYPE_GDI)
        {
            restore_state = TRUE;

            if (FAILED(hr = wined3d_stateblock_create(This->wined3d_device, WINED3D_SBT_ALL, &stateblock)))
            {
                ERR("Failed to create stateblock, hr %#x.\n", hr);
                wined3d_mutex_unlock();
                return hr;
            }

            wined3d_stateblock_capture(stateblock);
            rt = wined3d_device_get_render_target(This->wined3d_device, 0);
            if (rt == This->wined3d_frontbuffer)
                rt = NULL;
            else if (rt)
                wined3d_surface_incref(rt);

            if ((ds = wined3d_device_get_depth_stencil(This->wined3d_device)))
                wined3d_surface_incref(ds);
        }

        ddraw_destroy_swapchain(This);
    }

    if (FAILED(hr = ddraw_create_swapchain(This, window, !(cooplevel & DDSCL_FULLSCREEN))))
        ERR("Failed to create swapchain, hr %#x.\n", hr);

    if (restore_state)
    {
        if (ds)
        {
            wined3d_device_set_depth_stencil(This->wined3d_device, ds);
            wined3d_surface_decref(ds);
        }

        if (rt)
        {
            wined3d_device_set_render_target(This->wined3d_device, 0, rt, FALSE);
            wined3d_surface_decref(rt);
        }

        wined3d_stateblock_apply(stateblock);
        wined3d_stateblock_decref(stateblock);
    }

    /* Unhandled flags */
    if(cooplevel & DDSCL_ALLOWREBOOT)
        WARN("(%p) Unhandled flag DDSCL_ALLOWREBOOT, harmless\n", This);
    if(cooplevel & DDSCL_ALLOWMODEX)
        WARN("(%p) Unhandled flag DDSCL_ALLOWMODEX, harmless\n", This);
    if(cooplevel & DDSCL_FPUSETUP)
        WARN("(%p) Unhandled flag DDSCL_FPUSETUP, harmless\n", This);

    if (cooplevel & DDSCL_EXCLUSIVE)
        exclusive_ddraw = This;
    else if (exclusive_ddraw == This)
        exclusive_ddraw = NULL;

    /* Store the cooperative_level */
    This->cooperative_level = cooplevel;
    This->dest_window = window;
    TRACE("SetCooperativeLevel retuning DD_OK\n");
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw4_SetCooperativeLevel(IDirectDraw4 *iface, HWND window, DWORD flags)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, window %p, flags %#x.\n", iface, window, flags);

    return ddraw7_SetCooperativeLevel(&ddraw->IDirectDraw7_iface, window, flags);
}

static HRESULT WINAPI ddraw2_SetCooperativeLevel(IDirectDraw2 *iface, HWND window, DWORD flags)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, window %p, flags %#x.\n", iface, window, flags);

    return ddraw7_SetCooperativeLevel(&ddraw->IDirectDraw7_iface, window, flags);
}

static HRESULT WINAPI ddraw1_SetCooperativeLevel(IDirectDraw *iface, HWND window, DWORD flags)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, window %p, flags %#x.\n", iface, window, flags);

    return ddraw7_SetCooperativeLevel(&ddraw->IDirectDraw7_iface, window, flags);
}

/*****************************************************************************
 * IDirectDraw7::SetDisplayMode
 *
 * Sets the display screen resolution, color depth and refresh frequency
 * when in fullscreen mode (in theory).
 * Possible return values listed in the SDK suggest that this method fails
 * when not in fullscreen mode, but this is wrong. Windows 2000 happily sets
 * the display mode in DDSCL_NORMAL mode without an hwnd specified.
 * It seems to be valid to pass 0 for With and Height, this has to be tested
 * It could mean that the current video mode should be left as-is. (But why
 * call it then?)
 *
 * Params:
 *  Height, Width: Screen dimension
 *  BPP: Color depth in Bits per pixel
 *  Refreshrate: Screen refresh rate
 *  Flags: Other stuff
 *
 * Returns
 *  DD_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_SetDisplayMode(IDirectDraw7 *iface, DWORD width, DWORD height,
        DWORD bpp, DWORD refresh_rate, DWORD flags)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct wined3d_display_mode mode;
    enum wined3d_format_id format;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, bpp %u, refresh_rate %u, flags %#x.\n",
            iface, width, height, bpp, refresh_rate, flags);

    if (force_refresh_rate != 0)
    {
        TRACE("ForceRefreshRate overriding passed-in refresh rate (%u Hz) to %u Hz\n",
                refresh_rate, force_refresh_rate);
        refresh_rate = force_refresh_rate;
    }

    wined3d_mutex_lock();

    if (exclusive_ddraw && exclusive_ddraw != ddraw)
    {
        wined3d_mutex_unlock();
        return DDERR_NOEXCLUSIVEMODE;
    }

    if (!width || !height)
    {
        /* It looks like Need for Speed Porsche Unleashed expects DD_OK here. */
        wined3d_mutex_unlock();
        return DD_OK;
    }

    if (!restore_mode && FAILED(hr = wined3d_get_adapter_display_mode(ddraw->wined3d,
            WINED3DADAPTER_DEFAULT, &original_mode, NULL)))
        ERR("Failed to get current display mode, hr %#x.\n", hr);

    switch (bpp)
    {
        case 8:  format = WINED3DFMT_P8_UINT;        break;
        case 15: format = WINED3DFMT_B5G5R5X1_UNORM; break;
        case 16: format = WINED3DFMT_B5G6R5_UNORM;   break;
        case 24: format = WINED3DFMT_B8G8R8_UNORM;   break;
        case 32: format = WINED3DFMT_B8G8R8X8_UNORM; break;
        default: format = WINED3DFMT_UNKNOWN;        break;
    }

    mode.width = width;
    mode.height = height;
    mode.refresh_rate = refresh_rate;
    mode.format_id = format;
    mode.scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;

    /* TODO: The possible return values from msdn suggest that the screen mode
     * can't be changed if a surface is locked or some drawing is in progress. */
    /* TODO: Lose the primary surface. */
    if (SUCCEEDED(hr = wined3d_set_adapter_display_mode(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &mode)))
    {
        ddraw->restore_mode = TRUE;
        restore_mode = TRUE;
    }

    wined3d_mutex_unlock();

    switch (hr)
    {
        case WINED3DERR_NOTAVAILABLE: return DDERR_UNSUPPORTED;
        default:                      return hr;
    }
}

static HRESULT WINAPI ddraw4_SetDisplayMode(IDirectDraw4 *iface, DWORD width, DWORD height,
        DWORD bpp, DWORD refresh_rate, DWORD flags)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, width %u, height %u, bpp %u, refresh_rate %u, flags %#x.\n",
            iface, width, height, bpp, refresh_rate, flags);

    return ddraw7_SetDisplayMode(&ddraw->IDirectDraw7_iface, width, height, bpp, refresh_rate, flags);
}

static HRESULT WINAPI ddraw2_SetDisplayMode(IDirectDraw2 *iface,
        DWORD width, DWORD height, DWORD bpp, DWORD refresh_rate, DWORD flags)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, width %u, height %u, bpp %u, refresh_rate %u, flags %#x.\n",
            iface, width, height, bpp, refresh_rate, flags);

    return ddraw7_SetDisplayMode(&ddraw->IDirectDraw7_iface, width, height, bpp, refresh_rate, flags);
}

static HRESULT WINAPI ddraw1_SetDisplayMode(IDirectDraw *iface, DWORD width, DWORD height, DWORD bpp)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, width %u, height %u, bpp %u.\n", iface, width, height, bpp);

    return ddraw7_SetDisplayMode(&ddraw->IDirectDraw7_iface, width, height, bpp, 0, 0);
}

/*****************************************************************************
 * IDirectDraw7::RestoreDisplayMode
 *
 * Restores the display mode to what it was at creation time. Basically.
 *
 * Returns
 *  DD_OK on success
 *  DDERR_NOEXCLUSIVE mode if the device isn't in fullscreen mode
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_RestoreDisplayMode(IDirectDraw7 *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();

    if (!ddraw->restore_mode)
    {
        wined3d_mutex_unlock();
        return DD_OK;
    }

    if (exclusive_ddraw && exclusive_ddraw != ddraw)
    {
        wined3d_mutex_unlock();
        return DDERR_NOEXCLUSIVEMODE;
    }

    if (SUCCEEDED(hr = wined3d_set_adapter_display_mode(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &original_mode)))
    {
        ddraw->restore_mode = FALSE;
        restore_mode = FALSE;
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw4_RestoreDisplayMode(IDirectDraw4 *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_RestoreDisplayMode(&ddraw->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw2_RestoreDisplayMode(IDirectDraw2 *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_RestoreDisplayMode(&ddraw->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw1_RestoreDisplayMode(IDirectDraw *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_RestoreDisplayMode(&ddraw->IDirectDraw7_iface);
}

/*****************************************************************************
 * IDirectDraw7::GetCaps
 *
 * Returns the drives capabilities
 *
 * Used for version 1, 2, 4 and 7
 *
 * Params:
 *  DriverCaps: Structure to write the Hardware accelerated caps to
 *  HelCaps: Structure to write the emulation caps to
 *
 * Returns
 *  This implementation returns DD_OK only
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetCaps(IDirectDraw7 *iface, DDCAPS *DriverCaps, DDCAPS *HELCaps)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    DDCAPS caps;
    WINED3DCAPS winecaps;
    HRESULT hr;
    DDSCAPS2 ddscaps = {0, 0, 0, {0}};

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, DriverCaps, HELCaps);

    /* One structure must be != NULL */
    if (!DriverCaps && !HELCaps)
    {
        WARN("Invalid parameters.\n");
        return DDERR_INVALIDPARAMS;
    }

    memset(&caps, 0, sizeof(caps));
    memset(&winecaps, 0, sizeof(winecaps));
    caps.dwSize = sizeof(caps);

    wined3d_mutex_lock();
    hr = wined3d_device_get_device_caps(ddraw->wined3d_device, &winecaps);
    if (FAILED(hr))
    {
        WARN("IWineD3DDevice::GetDeviceCaps failed\n");
        wined3d_mutex_unlock();
        return hr;
    }

    hr = IDirectDraw7_GetAvailableVidMem(iface, &ddscaps, &caps.dwVidMemTotal, &caps.dwVidMemFree);
    wined3d_mutex_unlock();
    if(FAILED(hr)) {
        WARN("IDirectDraw7::GetAvailableVidMem failed\n");
        return hr;
    }

    caps.dwCaps = winecaps.ddraw_caps.caps;
    caps.dwCaps2 = winecaps.ddraw_caps.caps2;
    caps.dwCKeyCaps = winecaps.ddraw_caps.color_key_caps;
    caps.dwFXCaps = winecaps.ddraw_caps.fx_caps;
    caps.dwPalCaps = winecaps.ddraw_caps.pal_caps;
    caps.ddsCaps.dwCaps = winecaps.ddraw_caps.dds_caps;
    caps.dwSVBCaps = winecaps.ddraw_caps.svb_caps;
    caps.dwSVBCKeyCaps = winecaps.ddraw_caps.svb_color_key_caps;
    caps.dwSVBFXCaps = winecaps.ddraw_caps.svb_fx_caps;
    caps.dwVSBCaps = winecaps.ddraw_caps.vsb_caps;
    caps.dwVSBCKeyCaps = winecaps.ddraw_caps.vsb_color_key_caps;
    caps.dwVSBFXCaps = winecaps.ddraw_caps.vsb_fx_caps;
    caps.dwSSBCaps = winecaps.ddraw_caps.ssb_caps;
    caps.dwSSBCKeyCaps = winecaps.ddraw_caps.ssb_color_key_caps;
    caps.dwSSBFXCaps = winecaps.ddraw_caps.ssb_fx_caps;

    /* Even if wined3d supports 3D rendering, remove the cap if ddraw is
     * configured not to use it. */
    if (DefaultSurfaceType == DDRAW_SURFACE_TYPE_GDI)
    {
        caps.dwCaps &= ~DDCAPS_3D;
        caps.ddsCaps.dwCaps &= ~(DDSCAPS_3DDEVICE | DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER);
    }
    if (winecaps.ddraw_caps.stride_align)
    {
        caps.dwCaps |= DDCAPS_ALIGNSTRIDE;
        caps.dwAlignStrideAlign = winecaps.ddraw_caps.stride_align;
    }

    if(DriverCaps)
    {
        DD_STRUCT_COPY_BYSIZE(DriverCaps, &caps);
        if (TRACE_ON(ddraw))
        {
            TRACE("Driver Caps :\n");
            DDRAW_dump_DDCAPS(DriverCaps);
        }

    }
    if(HELCaps)
    {
        DD_STRUCT_COPY_BYSIZE(HELCaps, &caps);
        if (TRACE_ON(ddraw))
        {
            TRACE("HEL Caps :\n");
            DDRAW_dump_DDCAPS(HELCaps);
        }
    }

    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetCaps(IDirectDraw4 *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddraw7_GetCaps(&ddraw->IDirectDraw7_iface, driver_caps, hel_caps);
}

static HRESULT WINAPI ddraw2_GetCaps(IDirectDraw2 *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddraw7_GetCaps(&ddraw->IDirectDraw7_iface, driver_caps, hel_caps);
}

static HRESULT WINAPI ddraw1_GetCaps(IDirectDraw *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddraw7_GetCaps(&ddraw->IDirectDraw7_iface, driver_caps, hel_caps);
}

/*****************************************************************************
 * IDirectDraw7::Compact
 *
 * No idea what it does, MSDN says it's not implemented.
 *
 * Returns
 *  DD_OK, but this is unchecked
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_Compact(IDirectDraw7 *iface)
{
    TRACE("iface %p.\n", iface);

    return DD_OK;
}

static HRESULT WINAPI ddraw4_Compact(IDirectDraw4 *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_Compact(&ddraw->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw2_Compact(IDirectDraw2 *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_Compact(&ddraw->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw1_Compact(IDirectDraw *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_Compact(&ddraw->IDirectDraw7_iface);
}

/*****************************************************************************
 * IDirectDraw7::GetDisplayMode
 *
 * Returns information about the current display mode
 *
 * Exists in Version 1, 2, 4 and 7
 *
 * Params:
 *  DDSD: Address of a surface description structure to write the info to
 *
 * Returns
 *  DD_OK
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetDisplayMode(IDirectDraw7 *iface, DDSURFACEDESC2 *DDSD)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct wined3d_display_mode mode;
    HRESULT hr;
    DWORD Size;

    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    wined3d_mutex_lock();
    /* This seems sane */
    if (!DDSD)
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if (FAILED(hr = wined3d_get_adapter_display_mode(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &mode, NULL)))
    {
        ERR("Failed to get display mode, hr %#x.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }

    Size = DDSD->dwSize;
    memset(DDSD, 0, Size);

    DDSD->dwSize = Size;
    DDSD->dwFlags |= DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;
    DDSD->dwWidth = mode.width;
    DDSD->dwHeight = mode.height;
    DDSD->u2.dwRefreshRate = 60;
    DDSD->ddsCaps.dwCaps = 0;
    DDSD->u4.ddpfPixelFormat.dwSize = sizeof(DDSD->u4.ddpfPixelFormat);
    PixelFormat_WineD3DtoDD(&DDSD->u4.ddpfPixelFormat, mode.format_id);
    DDSD->u1.lPitch = mode.width * DDSD->u4.ddpfPixelFormat.u1.dwRGBBitCount / 8;

    if(TRACE_ON(ddraw))
    {
        TRACE("Returning surface desc :\n");
        DDRAW_dump_surface_desc(DDSD);
    }

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetDisplayMode(IDirectDraw4 *iface, DDSURFACEDESC2 *surface_desc)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, surface_desc);

    return ddraw7_GetDisplayMode(&ddraw->IDirectDraw7_iface, surface_desc);
}

static HRESULT WINAPI ddraw2_GetDisplayMode(IDirectDraw2 *iface, DDSURFACEDESC *surface_desc)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, surface_desc);

    /* FIXME: Test sizes, properly convert surface_desc */
    return ddraw7_GetDisplayMode(&ddraw->IDirectDraw7_iface, (DDSURFACEDESC2 *)surface_desc);
}

static HRESULT WINAPI ddraw1_GetDisplayMode(IDirectDraw *iface, DDSURFACEDESC *surface_desc)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, surface_desc);

    /* FIXME: Test sizes, properly convert surface_desc */
    return ddraw7_GetDisplayMode(&ddraw->IDirectDraw7_iface, (DDSURFACEDESC2 *)surface_desc);
}

/*****************************************************************************
 * IDirectDraw7::GetFourCCCodes
 *
 * Returns an array of supported FourCC codes.
 *
 * Exists in Version 1, 2, 4 and 7
 *
 * Params:
 *  NumCodes: Contains the number of Codes that Codes can carry. Returns the number
 *            of enumerated codes
 *  Codes: Pointer to an array of DWORDs where the supported codes are written
 *         to
 *
 * Returns
 *  Always returns DD_OK, as it's a stub for now
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetFourCCCodes(IDirectDraw7 *iface, DWORD *NumCodes, DWORD *Codes)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    static const enum wined3d_format_id formats[] =
    {
        WINED3DFMT_YUY2, WINED3DFMT_UYVY, WINED3DFMT_YV12,
        WINED3DFMT_DXT1, WINED3DFMT_DXT2, WINED3DFMT_DXT3, WINED3DFMT_DXT4, WINED3DFMT_DXT5,
        WINED3DFMT_ATI2N, WINED3DFMT_NVHU, WINED3DFMT_NVHS
    };
    struct wined3d_display_mode mode;
    DWORD count = 0, i, outsize;
    HRESULT hr;

    TRACE("iface %p, codes_count %p, codes %p.\n", iface, NumCodes, Codes);

    if (FAILED(hr = wined3d_get_adapter_display_mode(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &mode, NULL)))
    {
        ERR("Failed to get display mode, hr %#x.\n", hr);
        return hr;
    }

    outsize = NumCodes && Codes ? *NumCodes : 0;

    for (i = 0; i < (sizeof(formats) / sizeof(formats[0])); ++i)
    {
        if (SUCCEEDED(wined3d_check_device_format(ddraw->wined3d, WINED3DADAPTER_DEFAULT, WINED3D_DEVICE_TYPE_HAL,
                mode.format_id, 0, WINED3D_RTYPE_SURFACE, formats[i])))
        {
            if (count < outsize)
                Codes[count] = formats[i];
            ++count;
        }
    }
    if(NumCodes) {
        TRACE("Returning %u FourCC codes\n", count);
        *NumCodes = count;
    }

    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetFourCCCodes(IDirectDraw4 *iface, DWORD *codes_count, DWORD *codes)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, codes_count %p, codes %p.\n", iface, codes_count, codes);

    return ddraw7_GetFourCCCodes(&ddraw->IDirectDraw7_iface, codes_count, codes);
}

static HRESULT WINAPI ddraw2_GetFourCCCodes(IDirectDraw2 *iface, DWORD *codes_count, DWORD *codes)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, codes_count %p, codes %p.\n", iface, codes_count, codes);

    return ddraw7_GetFourCCCodes(&ddraw->IDirectDraw7_iface, codes_count, codes);
}

static HRESULT WINAPI ddraw1_GetFourCCCodes(IDirectDraw *iface, DWORD *codes_count, DWORD *codes)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, codes_count %p, codes %p.\n", iface, codes_count, codes);

    return ddraw7_GetFourCCCodes(&ddraw->IDirectDraw7_iface, codes_count, codes);
}

static HRESULT WINAPI ddraw7_GetMonitorFrequency(IDirectDraw7 *iface, DWORD *frequency)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct wined3d_display_mode mode;
    HRESULT hr;

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_display_mode(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &mode, NULL);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to get display mode, hr %#x.\n", hr);
        return hr;
    }

    *frequency = mode.refresh_rate;

    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetMonitorFrequency(IDirectDraw4 *iface, DWORD *frequency)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddraw7_GetMonitorFrequency(&ddraw->IDirectDraw7_iface, frequency);
}

static HRESULT WINAPI ddraw2_GetMonitorFrequency(IDirectDraw2 *iface, DWORD *frequency)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddraw7_GetMonitorFrequency(&ddraw->IDirectDraw7_iface, frequency);
}

static HRESULT WINAPI ddraw1_GetMonitorFrequency(IDirectDraw *iface, DWORD *frequency)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddraw7_GetMonitorFrequency(&ddraw->IDirectDraw7_iface, frequency);
}

static HRESULT WINAPI ddraw7_GetVerticalBlankStatus(IDirectDraw7 *iface, BOOL *status)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct wined3d_raster_status raster_status;
    HRESULT hr;

    TRACE("iface %p, status %p.\n", iface, status);

    if(!status)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_raster_status(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &raster_status);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to get raster status, hr %#x.\n", hr);
        return hr;
    }

    *status = raster_status.in_vblank;

    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetVerticalBlankStatus(IDirectDraw4 *iface, BOOL *status)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddraw7_GetVerticalBlankStatus(&ddraw->IDirectDraw7_iface, status);
}

static HRESULT WINAPI ddraw2_GetVerticalBlankStatus(IDirectDraw2 *iface, BOOL *status)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddraw7_GetVerticalBlankStatus(&ddraw->IDirectDraw7_iface, status);
}

static HRESULT WINAPI ddraw1_GetVerticalBlankStatus(IDirectDraw *iface, BOOL *status)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddraw7_GetVerticalBlankStatus(&ddraw->IDirectDraw7_iface, status);
}

/*****************************************************************************
 * IDirectDraw7::GetAvailableVidMem
 *
 * Returns the total and free video memory
 *
 * Params:
 *  Caps: Specifies the memory type asked for
 *  total: Pointer to a DWORD to be filled with the total memory
 *  free: Pointer to a DWORD to be filled with the free memory
 *
 * Returns
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS of free and total are NULL
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetAvailableVidMem(IDirectDraw7 *iface, DDSCAPS2 *Caps, DWORD *total,
        DWORD *free)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    HRESULT hr = DD_OK;

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, Caps, total, free);

    if (TRACE_ON(ddraw))
    {
        TRACE("Asked for memory with description: ");
        DDRAW_dump_DDSCAPS2(Caps);
    }
    wined3d_mutex_lock();

    /* Todo: System memory vs local video memory vs non-local video memory
     * The MSDN also mentions differences between texture memory and other
     * resources, but that's not important
     */

    if( (!total) && (!free) )
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if (free)
        *free = wined3d_device_get_available_texture_mem(ddraw->wined3d_device);
    if (total)
    {
        struct wined3d_adapter_identifier desc = {0};

        hr = wined3d_get_adapter_identifier(ddraw->wined3d, WINED3DADAPTER_DEFAULT, 0, &desc);
        *total = desc.video_memory;
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI ddraw4_GetAvailableVidMem(IDirectDraw4 *iface,
        DDSCAPS2 *caps, DWORD *total, DWORD *free)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, caps, total, free);

    return ddraw7_GetAvailableVidMem(&ddraw->IDirectDraw7_iface, caps, total, free);
}

static HRESULT WINAPI ddraw2_GetAvailableVidMem(IDirectDraw2 *iface,
        DDSCAPS *caps, DWORD *total, DWORD *free)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);
    DDSCAPS2 caps2;

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, caps, total, free);

    DDRAW_Convert_DDSCAPS_1_To_2(caps, &caps2);
    return ddraw7_GetAvailableVidMem(&ddraw->IDirectDraw7_iface, &caps2, total, free);
}

/*****************************************************************************
 * IDirectDraw7::Initialize
 *
 * Initializes a DirectDraw interface.
 *
 * Params:
 *  GUID: Interface identifier. Well, don't know what this is really good
 *   for
 *
 * Returns
 *  Returns DD_OK on the first call,
 *  DDERR_ALREADYINITIALIZED on repeated calls
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_Initialize(IDirectDraw7 *iface, GUID *guid)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    if (ddraw->initialized)
        return DDERR_ALREADYINITIALIZED;

    /* FIXME: To properly take the GUID into account we should call
     * ddraw_init() here instead of in DDRAW_Create(). */
    if (guid)
        FIXME("Ignoring guid %s.\n", debugstr_guid(guid));

    ddraw->initialized = TRUE;
    return DD_OK;
}

static HRESULT WINAPI ddraw4_Initialize(IDirectDraw4 *iface, GUID *guid)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddraw7_Initialize(&ddraw->IDirectDraw7_iface, guid);
}

static HRESULT WINAPI ddraw2_Initialize(IDirectDraw2 *iface, GUID *guid)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddraw7_Initialize(&ddraw->IDirectDraw7_iface, guid);
}

static HRESULT WINAPI ddraw1_Initialize(IDirectDraw *iface, GUID *guid)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddraw7_Initialize(&ddraw->IDirectDraw7_iface, guid);
}

static HRESULT WINAPI d3d1_Initialize(IDirect3D *iface, REFIID riid)
{
    TRACE("iface %p, riid %s.\n", iface, debugstr_guid(riid));

    return DDERR_ALREADYINITIALIZED;
}

/*****************************************************************************
 * IDirectDraw7::FlipToGDISurface
 *
 * "Makes the surface that the GDI writes to the primary surface"
 * Looks like some windows specific thing we don't have to care about.
 * According to MSDN it permits GDI dialog boxes in FULLSCREEN mode. Good to
 * show error boxes ;)
 * Well, just return DD_OK.
 *
 * Returns:
 *  Always returns DD_OK
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_FlipToGDISurface(IDirectDraw7 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return DD_OK;
}

static HRESULT WINAPI ddraw4_FlipToGDISurface(IDirectDraw4 *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_FlipToGDISurface(&ddraw->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw2_FlipToGDISurface(IDirectDraw2 *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_FlipToGDISurface(&ddraw->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw1_FlipToGDISurface(IDirectDraw *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_FlipToGDISurface(&ddraw->IDirectDraw7_iface);
}

/*****************************************************************************
 * IDirectDraw7::WaitForVerticalBlank
 *
 * This method allows applications to get in sync with the vertical blank
 * interval.
 * The wormhole demo in the DirectX 7 sdk uses this call, and it doesn't
 * redraw the screen, most likely because of this stub
 *
 * Parameters:
 *  Flags: one of DDWAITVB_BLOCKBEGIN, DDWAITVB_BLOCKBEGINEVENT
 *         or DDWAITVB_BLOCKEND
 *  h: Not used, according to MSDN
 *
 * Returns:
 *  Always returns DD_OK
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_WaitForVerticalBlank(IDirectDraw7 *iface, DWORD Flags, HANDLE event)
{
    static BOOL hide;

    TRACE("iface %p, flags %#x, event %p.\n", iface, Flags, event);

    /* This function is called often, so print the fixme only once */
    if(!hide)
    {
        FIXME("iface %p, flags %#x, event %p stub!\n", iface, Flags, event);
        hide = TRUE;
    }

    /* MSDN says DDWAITVB_BLOCKBEGINEVENT is not supported */
    if(Flags & DDWAITVB_BLOCKBEGINEVENT)
        return DDERR_UNSUPPORTED; /* unchecked */

    return DD_OK;
}

static HRESULT WINAPI ddraw4_WaitForVerticalBlank(IDirectDraw4 *iface, DWORD flags, HANDLE event)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#x, event %p.\n", iface, flags, event);

    return ddraw7_WaitForVerticalBlank(&ddraw->IDirectDraw7_iface, flags, event);
}

static HRESULT WINAPI ddraw2_WaitForVerticalBlank(IDirectDraw2 *iface, DWORD flags, HANDLE event)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, flags %#x, event %p.\n", iface, flags, event);

    return ddraw7_WaitForVerticalBlank(&ddraw->IDirectDraw7_iface, flags, event);
}

static HRESULT WINAPI ddraw1_WaitForVerticalBlank(IDirectDraw *iface, DWORD flags, HANDLE event)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, flags %#x, event %p.\n", iface, flags, event);

    return ddraw7_WaitForVerticalBlank(&ddraw->IDirectDraw7_iface, flags, event);
}

static HRESULT WINAPI ddraw7_GetScanLine(IDirectDraw7 *iface, DWORD *Scanline)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct wined3d_raster_status raster_status;
    HRESULT hr;

    TRACE("iface %p, line %p.\n", iface, Scanline);

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_raster_status(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &raster_status);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to get raster status, hr %#x.\n", hr);
        return hr;
    }

    *Scanline = raster_status.scan_line;

    if (raster_status.in_vblank)
        return DDERR_VERTICALBLANKINPROGRESS;

    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetScanLine(IDirectDraw4 *iface, DWORD *line)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddraw7_GetScanLine(&ddraw->IDirectDraw7_iface, line);
}

static HRESULT WINAPI ddraw2_GetScanLine(IDirectDraw2 *iface, DWORD *line)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddraw7_GetScanLine(&ddraw->IDirectDraw7_iface, line);
}

static HRESULT WINAPI ddraw1_GetScanLine(IDirectDraw *iface, DWORD *line)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddraw7_GetScanLine(&ddraw->IDirectDraw7_iface, line);
}

/*****************************************************************************
 * IDirectDraw7::TestCooperativeLevel
 *
 * Informs the application about the state of the video adapter, depending
 * on the cooperative level
 *
 * Returns:
 *  DD_OK if the device is in a sane state
 *  DDERR_NOEXCLUSIVEMODE or DDERR_EXCLUSIVEMODEALREADYSET
 *  if the state is not correct(See below)
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_TestCooperativeLevel(IDirectDraw7 *iface)
{
    TRACE("iface %p.\n", iface);

    return DD_OK;
}

static HRESULT WINAPI ddraw4_TestCooperativeLevel(IDirectDraw4 *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_TestCooperativeLevel(&ddraw->IDirectDraw7_iface);
}

/*****************************************************************************
 * IDirectDraw7::GetGDISurface
 *
 * Returns the surface that GDI is treating as the primary surface.
 * For Wine this is the front buffer
 *
 * Params:
 *  GDISurface: Address to write the surface pointer to
 *
 * Returns:
 *  DD_OK if the surface was found
 *  DDERR_NOTFOUND if the GDI surface wasn't found
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetGDISurface(IDirectDraw7 *iface, IDirectDrawSurface7 **GDISurface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);

    TRACE("iface %p, surface %p.\n", iface, GDISurface);

    wined3d_mutex_lock();

    if (!(*GDISurface = &ddraw->primary->IDirectDrawSurface7_iface))
    {
        WARN("Primary not created yet.\n");
        wined3d_mutex_unlock();
        return DDERR_NOTFOUND;
    }
    IDirectDrawSurface7_AddRef(*GDISurface);

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetGDISurface(IDirectDraw4 *iface, IDirectDrawSurface4 **surface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);
    struct ddraw_surface *surface_impl;
    IDirectDrawSurface7 *surface7;
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, surface);

    hr = ddraw7_GetGDISurface(&ddraw->IDirectDraw7_iface, &surface7);
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }
    surface_impl = impl_from_IDirectDrawSurface7(surface7);
    *surface = &surface_impl->IDirectDrawSurface4_iface;
    IDirectDrawSurface4_AddRef(*surface);
    IDirectDrawSurface7_Release(surface7);

    return hr;
}

static HRESULT WINAPI ddraw2_GetGDISurface(IDirectDraw2 *iface, IDirectDrawSurface **surface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);
    struct ddraw_surface *surface_impl;
    IDirectDrawSurface7 *surface7;
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, surface);

    hr = ddraw7_GetGDISurface(&ddraw->IDirectDraw7_iface, &surface7);
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }
    surface_impl = impl_from_IDirectDrawSurface7(surface7);
    *surface = &surface_impl->IDirectDrawSurface_iface;
    IDirectDrawSurface_AddRef(*surface);
    IDirectDrawSurface7_Release(surface7);

    return hr;
}

static HRESULT WINAPI ddraw1_GetGDISurface(IDirectDraw *iface, IDirectDrawSurface **surface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);
    struct ddraw_surface *surface_impl;
    IDirectDrawSurface7 *surface7;
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, surface);

    hr = ddraw7_GetGDISurface(&ddraw->IDirectDraw7_iface, &surface7);
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }
    surface_impl = impl_from_IDirectDrawSurface7(surface7);
    *surface = &surface_impl->IDirectDrawSurface_iface;
    IDirectDrawSurface_AddRef(*surface);
    IDirectDrawSurface7_Release(surface7);

    return hr;
}

struct displaymodescallback_context
{
    LPDDENUMMODESCALLBACK func;
    void *context;
};

static HRESULT CALLBACK EnumDisplayModesCallbackThunk(DDSURFACEDESC2 *surface_desc, void *context)
{
    struct displaymodescallback_context *cbcontext = context;
    DDSURFACEDESC desc;

    DDSD2_to_DDSD(surface_desc, &desc);
    return cbcontext->func(&desc, cbcontext->context);
}

/*****************************************************************************
 * IDirectDraw7::EnumDisplayModes
 *
 * Enumerates the supported Display modes. The modes can be filtered with
 * the DDSD parameter.
 *
 * Params:
 *  Flags: can be DDEDM_REFRESHRATES and DDEDM_STANDARDVGAMODES. For old ddraw
 *         versions (3 and older?) this is reserved and must be 0.
 *  DDSD: Surface description to filter the modes
 *  Context: Pointer passed back to the callback function
 *  cb: Application-provided callback function
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if the callback wasn't set
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_EnumDisplayModes(IDirectDraw7 *iface, DWORD Flags,
        DDSURFACEDESC2 *DDSD, void *Context, LPDDENUMMODESCALLBACK2 cb)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct wined3d_display_mode *enum_modes = NULL;
    struct wined3d_display_mode mode;
    unsigned int modenum, fmt;
    DDSURFACEDESC2 callback_sd;
    unsigned enum_mode_count = 0, enum_mode_array_size = 0;
    DDPIXELFORMAT pixelformat;

    static const enum wined3d_format_id checkFormatList[] =
    {
        WINED3DFMT_B8G8R8X8_UNORM,
        WINED3DFMT_B5G6R5_UNORM,
        WINED3DFMT_P8_UINT,
    };

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, Flags, DDSD, Context, cb);

    if (!cb)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if(!(Flags & DDEDM_REFRESHRATES))
    {
        enum_mode_array_size = 16;
        enum_modes = HeapAlloc(GetProcessHeap(), 0, sizeof(*enum_modes) * enum_mode_array_size);
        if (!enum_modes)
        {
            ERR("Out of memory\n");
            wined3d_mutex_unlock();
            return DDERR_OUTOFMEMORY;
        }
    }

    pixelformat.dwSize = sizeof(pixelformat);
    for(fmt = 0; fmt < (sizeof(checkFormatList) / sizeof(checkFormatList[0])); fmt++)
    {
        modenum = 0;
        while (wined3d_enum_adapter_modes(ddraw->wined3d, WINED3DADAPTER_DEFAULT, checkFormatList[fmt],
                WINED3D_SCANLINE_ORDERING_UNKNOWN, modenum++, &mode) == WINED3D_OK)
        {
            PixelFormat_WineD3DtoDD(&pixelformat, mode.format_id);
            if (DDSD)
            {
                if (DDSD->dwFlags & DDSD_WIDTH && mode.width != DDSD->dwWidth)
                    continue;
                if (DDSD->dwFlags & DDSD_HEIGHT && mode.height != DDSD->dwHeight)
                    continue;
                if (DDSD->dwFlags & DDSD_REFRESHRATE && mode.refresh_rate != DDSD->u2.dwRefreshRate)
                    continue;
                if (DDSD->dwFlags & DDSD_PIXELFORMAT
                        && pixelformat.u1.dwRGBBitCount != DDSD->u4.ddpfPixelFormat.u1.dwRGBBitCount)
                    continue;
            }

            if(!(Flags & DDEDM_REFRESHRATES))
            {
                /* DX docs state EnumDisplayMode should return only unique modes. If DDEDM_REFRESHRATES is not set, refresh
                 * rate doesn't matter when determining if the mode is unique. So modes only differing in refresh rate have
                 * to be reduced to a single unique result in such case.
                 */
                BOOL found = FALSE;
                unsigned i;

                for (i = 0; i < enum_mode_count; i++)
                {
                    if (enum_modes[i].width == mode.width && enum_modes[i].height == mode.height
                            && enum_modes[i].format_id == mode.format_id)
                    {
                        found = TRUE;
                        break;
                    }
                }

                if(found) continue;
            }

            memset(&callback_sd, 0, sizeof(callback_sd));
            callback_sd.dwSize = sizeof(callback_sd);
            callback_sd.u4.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

            callback_sd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_PITCH|DDSD_REFRESHRATE;
            if (Flags & DDEDM_REFRESHRATES)
                callback_sd.u2.dwRefreshRate = mode.refresh_rate;

            callback_sd.dwWidth = mode.width;
            callback_sd.dwHeight = mode.height;

            callback_sd.u4.ddpfPixelFormat=pixelformat;

            /* Calc pitch and DWORD align like MSDN says */
            callback_sd.u1.lPitch = (callback_sd.u4.ddpfPixelFormat.u1.dwRGBBitCount / 8) * mode.width;
            callback_sd.u1.lPitch = (callback_sd.u1.lPitch + 3) & ~3;

            TRACE("Enumerating %dx%dx%d @%d\n", callback_sd.dwWidth, callback_sd.dwHeight, callback_sd.u4.ddpfPixelFormat.u1.dwRGBBitCount,
              callback_sd.u2.dwRefreshRate);

            if(cb(&callback_sd, Context) == DDENUMRET_CANCEL)
            {
                TRACE("Application asked to terminate the enumeration\n");
                HeapFree(GetProcessHeap(), 0, enum_modes);
                wined3d_mutex_unlock();
                return DD_OK;
            }

            if(!(Flags & DDEDM_REFRESHRATES))
            {
                if (enum_mode_count == enum_mode_array_size)
                {
                    struct wined3d_display_mode *new_enum_modes;

                    enum_mode_array_size *= 2;
                    new_enum_modes = HeapReAlloc(GetProcessHeap(), 0, enum_modes,
                            sizeof(*new_enum_modes) * enum_mode_array_size);
                    if (!new_enum_modes)
                    {
                        ERR("Out of memory\n");
                        HeapFree(GetProcessHeap(), 0, enum_modes);
                        wined3d_mutex_unlock();
                        return DDERR_OUTOFMEMORY;
                    }

                    enum_modes = new_enum_modes;
                }

                enum_modes[enum_mode_count++] = mode;
            }
        }
    }

    TRACE("End of enumeration\n");
    HeapFree(GetProcessHeap(), 0, enum_modes);
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw4_EnumDisplayModes(IDirectDraw4 *iface, DWORD flags,
        DDSURFACEDESC2 *surface_desc, void *context, LPDDENUMMODESCALLBACK2 callback)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    return ddraw7_EnumDisplayModes(&ddraw->IDirectDraw7_iface, flags, surface_desc, context, callback);
}

static HRESULT WINAPI ddraw2_EnumDisplayModes(IDirectDraw2 *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMMODESCALLBACK callback)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);
    struct displaymodescallback_context cbcontext;
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw7_EnumDisplayModes(&ddraw->IDirectDraw7_iface, flags,
            surface_desc ? &surface_desc2 : NULL, &cbcontext, EnumDisplayModesCallbackThunk);
}

static HRESULT WINAPI ddraw1_EnumDisplayModes(IDirectDraw *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMMODESCALLBACK callback)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);
    struct displaymodescallback_context cbcontext;
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw7_EnumDisplayModes(&ddraw->IDirectDraw7_iface, flags,
            surface_desc ? &surface_desc2 : NULL, &cbcontext, EnumDisplayModesCallbackThunk);
}

/*****************************************************************************
 * IDirectDraw7::EvaluateMode
 *
 * Used with IDirectDraw7::StartModeTest to test video modes.
 * EvaluateMode is used to pass or fail a mode, and continue with the next
 * mode
 *
 * Params:
 *  Flags: DDEM_MODEPASSED or DDEM_MODEFAILED
 *  Timeout: Returns the amount of seconds left before the mode would have
 *           been failed automatically
 *
 * Returns:
 *  This implementation always DD_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_EvaluateMode(IDirectDraw7 *iface, DWORD Flags, DWORD *Timeout)
{
    FIXME("iface %p, flags %#x, timeout %p stub!\n", iface, Flags, Timeout);

    /* When implementing this, implement it in WineD3D */

    return DD_OK;
}

/*****************************************************************************
 * IDirectDraw7::GetDeviceIdentifier
 *
 * Returns the device identifier, which gives information about the driver
 * Our device identifier is defined at the beginning of this file.
 *
 * Params:
 *  DDDI: Address for the returned structure
 *  Flags: Can be DDGDI_GETHOSTIDENTIFIER
 *
 * Returns:
 *  On success it returns DD_OK
 *  DDERR_INVALIDPARAMS if DDDI is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetDeviceIdentifier(IDirectDraw7 *iface,
        DDDEVICEIDENTIFIER2 *DDDI, DWORD Flags)
{
    TRACE("iface %p, device_identifier %p, flags %#x.\n", iface, DDDI, Flags);

    if(!DDDI)
        return DDERR_INVALIDPARAMS;

    /* The DDGDI_GETHOSTIDENTIFIER returns the information about the 2D
     * host adapter, if there's a secondary 3D adapter. This doesn't apply
     * to any modern hardware, nor is it interesting for Wine, so ignore it.
     * Size of DDDEVICEIDENTIFIER2 may be aligned to 8 bytes and thus 4
     * bytes too long. So only copy the relevant part of the structure
     */

    memcpy(DDDI, &deviceidentifier, FIELD_OFFSET(DDDEVICEIDENTIFIER2, dwWHQLLevel) + sizeof(DWORD));
    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetDeviceIdentifier(IDirectDraw4 *iface,
        DDDEVICEIDENTIFIER *identifier, DWORD flags)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);
    DDDEVICEIDENTIFIER2 identifier2;
    HRESULT hr;

    TRACE("iface %p, identifier %p, flags %#x.\n", iface, identifier, flags);

    hr = ddraw7_GetDeviceIdentifier(&ddraw->IDirectDraw7_iface, &identifier2, flags);
    DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(&identifier2, identifier);

    return hr;
}

/*****************************************************************************
 * IDirectDraw7::GetSurfaceFromDC
 *
 * Returns the Surface for a GDI device context handle.
 * Is this related to IDirectDrawSurface::GetDC ???
 *
 * Params:
 *  hdc: hdc to return the surface for
 *  Surface: Address to write the surface pointer to
 *
 * Returns:
 *  Always returns DD_OK because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetSurfaceFromDC(IDirectDraw7 *iface, HDC hdc,
        IDirectDrawSurface7 **Surface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct wined3d_surface *wined3d_surface;
    struct ddraw_surface *surface_impl;

    TRACE("iface %p, dc %p, surface %p.\n", iface, hdc, Surface);

    if (!Surface) return E_INVALIDARG;

    if (!(wined3d_surface = wined3d_device_get_surface_from_dc(ddraw->wined3d_device, hdc)))
    {
        TRACE("No surface found for dc %p.\n", hdc);
        *Surface = NULL;
        return DDERR_NOTFOUND;
    }

    surface_impl = wined3d_surface_get_parent(wined3d_surface);
    *Surface = &surface_impl->IDirectDrawSurface7_iface;
    IDirectDrawSurface7_AddRef(*Surface);
    TRACE("Returning surface %p.\n", Surface);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetSurfaceFromDC(IDirectDraw4 *iface, HDC dc,
        IDirectDrawSurface4 **surface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);
    struct ddraw_surface *surface_impl;
    IDirectDrawSurface7 *surface7;
    HRESULT hr;

    TRACE("iface %p, dc %p, surface %p.\n", iface, dc, surface);

    if (!surface) return E_INVALIDARG;

    hr = ddraw7_GetSurfaceFromDC(&ddraw->IDirectDraw7_iface, dc, &surface7);
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }
    surface_impl = impl_from_IDirectDrawSurface7(surface7);
    /* Tests say this is true */
    *surface = (IDirectDrawSurface4 *)&surface_impl->IDirectDrawSurface_iface;
    IDirectDrawSurface_AddRef(&surface_impl->IDirectDrawSurface_iface);
    IDirectDrawSurface7_Release(surface7);

    return hr;
}

/*****************************************************************************
 * IDirectDraw7::RestoreAllSurfaces
 *
 * Calls the restore method of all surfaces
 *
 * Params:
 *
 * Returns:
 *  Always returns DD_OK because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_RestoreAllSurfaces(IDirectDraw7 *iface)
{
    FIXME("iface %p stub!\n", iface);

    /* This isn't hard to implement: Enumerate all WineD3D surfaces,
     * get their parent and call their restore method. Do not implement
     * it in WineD3D, as restoring a surface means re-creating the
     * WineD3DDSurface
     */
    return DD_OK;
}

static HRESULT WINAPI ddraw4_RestoreAllSurfaces(IDirectDraw4 *iface)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_RestoreAllSurfaces(&ddraw->IDirectDraw7_iface);
}

/*****************************************************************************
 * IDirectDraw7::StartModeTest
 *
 * Tests the specified video modes to update the system registry with
 * refresh rate information. StartModeTest starts the mode test,
 * EvaluateMode is used to fail or pass a mode. If EvaluateMode
 * isn't called within 15 seconds, the mode is failed automatically
 *
 * As refresh rates are handled by the X server, I don't think this
 * Method is important
 *
 * Params:
 *  Modes: An array of mode specifications
 *  NumModes: The number of modes in Modes
 *  Flags: Some flags...
 *
 * Returns:
 *  Returns DDERR_TESTFINISHED if flags contains DDSMT_ISTESTREQUIRED,
 *  if no modes are passed, DDERR_INVALIDPARAMS is returned,
 *  otherwise DD_OK
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_StartModeTest(IDirectDraw7 *iface, SIZE *Modes, DWORD NumModes, DWORD Flags)
{
    FIXME("iface %p, modes %p, mode_count %u, flags %#x partial stub!\n",
            iface, Modes, NumModes, Flags);

    /* This looks sane */
    if( (!Modes) || (NumModes == 0) ) return DDERR_INVALIDPARAMS;

    /* DDSMT_ISTESTREQUIRED asks if a mode test is necessary.
     * As it is not, DDERR_TESTFINISHED is returned
     * (hopefully that's correct
     *
    if(Flags & DDSMT_ISTESTREQUIRED) return DDERR_TESTFINISHED;
     * well, that value doesn't (yet) exist in the wine headers, so ignore it
     */

    return DD_OK;
}

/*****************************************************************************
 * ddraw_create_surface
 *
 * A helper function for IDirectDraw7::CreateSurface. It creates a new surface
 * with the passed parameters.
 *
 * Params:
 *  DDSD: Description of the surface to create
 *  Surf: Address to store the interface pointer at
 *
 * Returns:
 *  DD_OK on success
 *
 *****************************************************************************/
static HRESULT ddraw_create_surface(struct ddraw *ddraw, DDSURFACEDESC2 *pDDSD,
        struct ddraw_surface **surface, UINT version)
{
    HRESULT hr;

    TRACE("ddraw %p, surface_desc %p, surface %p.\n", ddraw, pDDSD, surface);

    if (TRACE_ON(ddraw))
    {
        TRACE("Requesting surface desc:\n");
        DDRAW_dump_surface_desc(pDDSD);
    }

    if ((pDDSD->ddsCaps.dwCaps & DDSCAPS_3DDEVICE) && DefaultSurfaceType != DDRAW_SURFACE_TYPE_OPENGL)
    {
        WARN("The application requests a 3D capable surface, but a non-OpenGL surface type was set in the registry.\n");
        /* Do not fail surface creation, only fail 3D device creation. */
    }

    /* Create the Surface object */
    *surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(**surface));
    if (!*surface)
    {
        ERR("Failed to allocate surface memory.\n");
        return DDERR_OUTOFVIDEOMEMORY;
    }

    if (FAILED(hr = ddraw_surface_init(*surface, ddraw, pDDSD, version)))
    {
        WARN("Failed to initialize surface, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, *surface);
        return hr;
    }

    /* Increase the surface counter, and attach the surface */
    list_add_head(&ddraw->surface_list, &(*surface)->surface_list_entry);

    TRACE("Created surface %p.\n", *surface);

    return DD_OK;
}

static HRESULT CDECL ddraw_reset_enum_callback(struct wined3d_resource *resource)
{
    return DD_OK;
}

/*****************************************************************************
 * IDirectDraw7::CreateSurface
 *
 * Creates a new IDirectDrawSurface object and returns its interface.
 *
 * The surface connections with wined3d are a bit tricky. Basically it works
 * like this:
 *
 * |------------------------|               |-----------------|
 * | DDraw surface          |               | WineD3DSurface  |
 * |                        |               |                 |
 * |        WineD3DSurface  |-------------->|                 |
 * |        Child           |<------------->| Parent          |
 * |------------------------|               |-----------------|
 *
 * The DDraw surface is the parent of the wined3d surface, and it releases
 * the WineD3DSurface when the ddraw surface is destroyed.
 *
 * However, for all surfaces which can be in a container in WineD3D,
 * we have to do this. These surfaces are usually complex surfaces,
 * so this concerns primary surfaces with a front and a back buffer,
 * and textures.
 *
 * |------------------------|               |-----------------|
 * | DDraw surface          |               | Container       |
 * |                        |               |                 |
 * |                  Child |<------------->| Parent          |
 * |                Texture |<------------->|                 |
 * |         WineD3DSurface |<----|         |          Levels |<--|
 * | Complex connection     |     |         |                 |   |
 * |------------------------|     |         |-----------------|   |
 *  ^                             |                               |
 *  |                             |                               |
 *  |                             |                               |
 *  |    |------------------|     |         |-----------------|   |
 *  |    | IParent          |     |-------->| WineD3DSurface  |   |
 *  |    |                  |               |                 |   |
 *  |    |            Child |<------------->| Parent          |   |
 *  |    |                  |               |       Container |<--|
 *  |    |------------------|               |-----------------|   |
 *  |                                                             |
 *  |   |----------------------|                                  |
 *  |   | DDraw surface 2      |                                  |
 *  |   |                      |                                  |
 *  |<->| Complex root   Child |                                  |
 *  |   |              Texture |                                  |
 *  |   |       WineD3DSurface |<----|                            |
 *  |   |----------------------|     |                            |
 *  |                                |                            |
 *  |    |---------------------|     |      |-----------------|   |
 *  |    | IParent             |     |----->| WineD3DSurface  |   |
 *  |    |                     |            |                 |   |
 *  |    |               Child |<---------->| Parent          |   |
 *  |    |---------------------|            |       Container |<--|
 *  |                                       |-----------------|   |
 *  |                                                             |
 *  |             ---More surfaces can follow---                  |
 *
 * The reason is that the IWineD3DSwapchain(render target container)
 * and the IWineD3DTexure(Texture container) release the parents
 * of their surface's children, but by releasing the complex root
 * the surfaces which are complexly attached to it are destroyed
 * too. See IDirectDrawSurface::Release for a more detailed
 * explanation.
 *
 * Params:
 *  DDSD: Description of the surface to create
 *  Surf: Address to store the interface pointer at
 *  UnkOuter: Basically for aggregation support, but ddraw doesn't support
 *            aggregation, so it has to be NULL
 *
 * Returns:
 *  DD_OK on success
 *  CLASS_E_NOAGGREGATION if UnkOuter != NULL
 *  DDERR_* if an error occurs
 *
 *****************************************************************************/
static HRESULT CreateSurface(struct ddraw *ddraw, DDSURFACEDESC2 *DDSD,
        struct ddraw_surface **surface, IUnknown *UnkOuter, UINT version)
{
    struct ddraw_surface *object = NULL;
    struct wined3d_display_mode mode;
    HRESULT hr;
    DDSURFACEDESC2 desc2;
    const DWORD sysvidmem = DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;

    TRACE("ddraw %p, surface_desc %p, surface %p, outer_unknown %p.\n", ddraw, DDSD, surface, UnkOuter);

    /* Some checks before we start */
    if (TRACE_ON(ddraw))
    {
        TRACE(" (%p) Requesting surface desc :\n", ddraw);
        DDRAW_dump_surface_desc(DDSD);
    }

    if (UnkOuter != NULL)
    {
        FIXME("(%p) : outer != NULL?\n", ddraw);
        return CLASS_E_NOAGGREGATION; /* unchecked */
    }

    if (!surface)
    {
        FIXME("(%p) You want to get back a surface? Don't give NULL ptrs!\n", ddraw);
        return E_POINTER; /* unchecked */
    }

    if (!(DDSD->dwFlags & DDSD_CAPS))
    {
        /* DVIDEO.DLL does forget the DDSD_CAPS flag ... *sigh* */
        DDSD->dwFlags |= DDSD_CAPS;
    }

    if (DDSD->ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD)
    {
        /* If the surface is of the 'alloconload' type, ignore the LPSURFACE field */
        DDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

    if ((DDSD->dwFlags & DDSD_LPSURFACE) && (DDSD->lpSurface == NULL))
    {
        /* Frank Herbert's Dune specifies a null pointer for the surface, ignore the LPSURFACE field */
        WARN("(%p) Null surface pointer specified, ignore it!\n", ddraw);
        DDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

    if((DDSD->ddsCaps.dwCaps & (DDSCAPS_FLIP | DDSCAPS_PRIMARYSURFACE)) == (DDSCAPS_FLIP | DDSCAPS_PRIMARYSURFACE) &&
       !(ddraw->cooperative_level & DDSCL_EXCLUSIVE))
    {
        TRACE("(%p): Attempt to create a flipable primary surface without DDSCL_EXCLUSIVE set\n",
                ddraw);
        *surface = NULL;
        return DDERR_NOEXCLUSIVEMODE;
    }

    if((DDSD->ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER | DDSCAPS_PRIMARYSURFACE)) == (DDSCAPS_BACKBUFFER | DDSCAPS_PRIMARYSURFACE))
    {
        WARN("Application wanted to create back buffer primary surface\n");
        return DDERR_INVALIDCAPS;
    }

    if((DDSD->ddsCaps.dwCaps & sysvidmem) == sysvidmem)
    {
        /* This is a special switch in ddrawex.dll, but not allowed in ddraw.dll */
        WARN("Application tries to put the surface in both system and video memory\n");
        *surface = NULL;
        return DDERR_INVALIDCAPS;
    }

    /* Check cube maps but only if the size includes them */
    if (DDSD->dwSize >= sizeof(DDSURFACEDESC2))
    {
        if(DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES &&
           !(DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP))
        {
            WARN("Cube map faces requested without cube map flag\n");
            return DDERR_INVALIDCAPS;
        }
        if(DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP &&
           (DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES) == 0)
        {
            WARN("Cube map without faces requested\n");
            return DDERR_INVALIDPARAMS;
        }

        /* Quick tests confirm those can be created, but we don't do that yet */
        if(DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP &&
           (DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES) != DDSCAPS2_CUBEMAP_ALLFACES)
        {
            FIXME("Partial cube maps not supported yet\n");
        }
    }

    /* According to the msdn this flag is ignored by CreateSurface */
    if (DDSD->dwSize >= sizeof(DDSURFACEDESC2))
        DDSD->ddsCaps.dwCaps2 &= ~DDSCAPS2_MIPMAPSUBLEVEL;

    /* Modify some flags */
    copy_to_surfacedesc2(&desc2, DDSD);
    desc2.u4.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT); /* Just to be sure */

    if (FAILED(hr = wined3d_get_adapter_display_mode(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &mode, NULL)))
    {
        ERR("Failed to get display mode, hr %#x.\n", hr);
        return hr;
    }

    /* No pixelformat given? Use the current screen format */
    if(!(desc2.dwFlags & DDSD_PIXELFORMAT))
    {
        desc2.dwFlags |= DDSD_PIXELFORMAT;
        desc2.u4.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);

        PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, mode.format_id);
    }

    /* No Width or no Height? Use the original screen size
     */
    if(!(desc2.dwFlags & DDSD_WIDTH) ||
       !(desc2.dwFlags & DDSD_HEIGHT) )
    {
        /* Invalid for non-render targets */
        if(!(desc2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
        {
            WARN("Creating a non-Primary surface without Width or Height info, returning DDERR_INVALIDPARAMS\n");
            *surface = NULL;
            return DDERR_INVALIDPARAMS;
        }

        desc2.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
        desc2.dwWidth = mode.width;
        desc2.dwHeight = mode.height;
    }

    if (!desc2.dwWidth || !desc2.dwHeight)
        return DDERR_INVALIDPARAMS;

    /* Mipmap count fixes */
    if(desc2.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        if(desc2.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
        {
            if(desc2.dwFlags & DDSD_MIPMAPCOUNT)
            {
                /* Mipmap count is given, should not be 0 */
                if( desc2.u2.dwMipMapCount == 0 )
                    return DDERR_INVALIDPARAMS;
            }
            else
            {
                /* Undocumented feature: Create sublevels until
                 * either the width or the height is 1
                 */
                DWORD min = desc2.dwWidth < desc2.dwHeight ?
                            desc2.dwWidth : desc2.dwHeight;
                desc2.u2.dwMipMapCount = 0;
                while( min )
                {
                    desc2.u2.dwMipMapCount += 1;
                    min >>= 1;
                }
            }
        }
        else
        {
            /* Not-complex mipmap -> Mipmapcount = 1 */
            desc2.u2.dwMipMapCount = 1;
        }

        /* There's a mipmap count in the created surface in any case */
        desc2.dwFlags |= DDSD_MIPMAPCOUNT;
    }
    /* If no mipmap is given, the texture has only one level */

    /* The first surface is a front buffer, the back buffer is created afterwards */
    if( (desc2.dwFlags & DDSD_CAPS) && (desc2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) )
    {
        desc2.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
    }

    /* The root surface in a cube map is positive x */
    if(desc2.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
    {
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_ALLFACES;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_POSITIVEX;
    }

    if ((desc2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) && (ddraw->cooperative_level & DDSCL_EXCLUSIVE))
    {
        struct wined3d_swapchain_desc swapchain_desc;

        wined3d_swapchain_get_desc(ddraw->wined3d_swapchain, &swapchain_desc);
        swapchain_desc.backbuffer_width = mode.width;
        swapchain_desc.backbuffer_height = mode.height;
        swapchain_desc.backbuffer_format = mode.format_id;

        hr = wined3d_device_reset(ddraw->wined3d_device,
                &swapchain_desc, NULL, ddraw_reset_enum_callback, TRUE);
        if (FAILED(hr))
        {
            ERR("Failed to reset device.\n");
            return hr;
        }
    }

    /* Create the first surface */
    if (FAILED(hr = ddraw_create_surface(ddraw, &desc2, &object, version)))
    {
        WARN("ddraw_create_surface failed, hr %#x.\n", hr);
        return hr;
    }
    object->is_complex_root = TRUE;

    *surface = object;

    /* Create Additional surfaces if necessary
     * This applies to Primary surfaces which have a back buffer count
     * set, but not to mipmap textures. In case of Mipmap textures,
     * wineD3D takes care of the creation of additional surfaces
     */
    if(DDSD->dwFlags & DDSD_BACKBUFFERCOUNT)
    {
        struct ddraw_surface *last = object;
        UINT i;

        desc2.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER; /* It's not a front buffer */
        desc2.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
        desc2.u5.dwBackBufferCount = 0;

        for (i = 0; i < DDSD->u5.dwBackBufferCount; ++i)
        {
            struct ddraw_surface *object2 = NULL;

            if (FAILED(hr = ddraw_create_surface(ddraw, &desc2, &object2, version)))
            {
                if (version == 7)
                    IDirectDrawSurface7_Release(&object->IDirectDrawSurface7_iface);
                else if (version == 4)
                    IDirectDrawSurface4_Release(&object->IDirectDrawSurface4_iface);
                else
                    IDirectDrawSurface_Release(&object->IDirectDrawSurface_iface);

                return hr;
            }

            /* Add the new surface to the complex attachment array. */
            last->complex_array[0] = object2;
            last = object2;

            /* Remove the (possible) back buffer cap from the new surface
             * description, because only one surface in the flipping chain is a
             * back buffer, one is a front buffer, the others are just primary
             * surfaces. */
            desc2.ddsCaps.dwCaps &= ~DDSCAPS_BACKBUFFER;
        }
    }

    if (desc2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        ddraw->primary = object;

    /* Create a WineD3DTexture if a texture was requested */
    if (desc2.ddsCaps.dwCaps & DDSCAPS_TEXTURE)
        ddraw_surface_create_texture(object);

    return hr;
}

static HRESULT WINAPI ddraw7_CreateSurface(IDirectDraw7 *iface, DDSURFACEDESC2 *surface_desc,
        IDirectDrawSurface7 **surface, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct ddraw_surface *impl;
    HRESULT hr;

    TRACE("iface %p, surface_desc %p, surface %p, outer_unknown %p.\n",
            iface, surface_desc, surface, outer_unknown);

    wined3d_mutex_lock();

    if (!(ddraw->cooperative_level & (DDSCL_NORMAL | DDSCL_EXCLUSIVE)))
    {
        WARN("Cooperative level not set.\n");
        wined3d_mutex_unlock();
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    if(surface_desc == NULL || surface_desc->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Application supplied invalid surface descriptor\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if(surface_desc->ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER))
    {
        if (TRACE_ON(ddraw))
        {
            TRACE(" (%p) Requesting surface desc :\n", iface);
            DDRAW_dump_surface_desc(surface_desc);
        }

        WARN("Application tried to create an explicit front or back buffer\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }

    hr = CreateSurface(ddraw, surface_desc, &impl, outer_unknown, 7);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }

    *surface = &impl->IDirectDrawSurface7_iface;
    IDirectDraw7_AddRef(iface);
    impl->ifaceToRelease = (IUnknown *)iface;

    return hr;
}

static HRESULT WINAPI ddraw4_CreateSurface(IDirectDraw4 *iface,
        DDSURFACEDESC2 *surface_desc, IDirectDrawSurface4 **surface, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);
    struct ddraw_surface *impl;
    HRESULT hr;

    TRACE("iface %p, surface_desc %p, surface %p, outer_unknown %p.\n",
            iface, surface_desc, surface, outer_unknown);

    wined3d_mutex_lock();

    if (!(ddraw->cooperative_level & (DDSCL_NORMAL | DDSCL_EXCLUSIVE)))
    {
        WARN("Cooperative level not set.\n");
        wined3d_mutex_unlock();
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    if(surface_desc == NULL || surface_desc->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Application supplied invalid surface descriptor\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if(surface_desc->ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER))
    {
        if (TRACE_ON(ddraw))
        {
            TRACE(" (%p) Requesting surface desc :\n", iface);
            DDRAW_dump_surface_desc(surface_desc);
        }

        WARN("Application tried to create an explicit front or back buffer\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }

    hr = CreateSurface(ddraw, surface_desc, &impl, outer_unknown, 4);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }

    *surface = &impl->IDirectDrawSurface4_iface;
    IDirectDraw4_AddRef(iface);
    impl->ifaceToRelease = (IUnknown *)iface;

    return hr;
}

static HRESULT WINAPI ddraw2_CreateSurface(IDirectDraw2 *iface,
        DDSURFACEDESC *surface_desc, IDirectDrawSurface **surface, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);
    struct ddraw_surface *impl;
    HRESULT hr;
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, surface_desc %p, surface %p, outer_unknown %p.\n",
            iface, surface_desc, surface, outer_unknown);

    wined3d_mutex_lock();

    if (!(ddraw->cooperative_level & (DDSCL_NORMAL | DDSCL_EXCLUSIVE)))
    {
        WARN("Cooperative level not set.\n");
        wined3d_mutex_unlock();
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    if(surface_desc == NULL || surface_desc->dwSize != sizeof(DDSURFACEDESC))
    {
        WARN("Application supplied invalid surface descriptor\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    DDSD_to_DDSD2(surface_desc, &surface_desc2);
    if(surface_desc->ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER))
    {
        if (TRACE_ON(ddraw))
        {
            TRACE(" (%p) Requesting surface desc :\n", iface);
            DDRAW_dump_surface_desc((DDSURFACEDESC2 *)surface_desc);
        }

        WARN("Application tried to create an explicit front or back buffer\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }

    hr = CreateSurface(ddraw, &surface_desc2, &impl, outer_unknown, 2);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }

    *surface = &impl->IDirectDrawSurface_iface;
    impl->ifaceToRelease = NULL;

    return hr;
}

static HRESULT WINAPI ddraw1_CreateSurface(IDirectDraw *iface,
        DDSURFACEDESC *surface_desc, IDirectDrawSurface **surface, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);
    struct ddraw_surface *impl;
    HRESULT hr;
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, surface_desc %p, surface %p, outer_unknown %p.\n",
            iface, surface_desc, surface, outer_unknown);

    wined3d_mutex_lock();

    if (!(ddraw->cooperative_level & (DDSCL_NORMAL | DDSCL_EXCLUSIVE)))
    {
        WARN("Cooperative level not set.\n");
        wined3d_mutex_unlock();
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    if(surface_desc == NULL || surface_desc->dwSize != sizeof(DDSURFACEDESC))
    {
        WARN("Application supplied invalid surface descriptor\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    /* Remove front buffer flag, this causes failure in v7, and its added to normal
     * primaries anyway. */
    surface_desc->ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
    DDSD_to_DDSD2(surface_desc, &surface_desc2);
    hr = CreateSurface(ddraw, &surface_desc2, &impl, outer_unknown, 1);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }

    *surface = &impl->IDirectDrawSurface_iface;
    impl->ifaceToRelease = NULL;

    return hr;
}

#define DDENUMSURFACES_SEARCHTYPE (DDENUMSURFACES_CANBECREATED|DDENUMSURFACES_DOESEXIST)
#define DDENUMSURFACES_MATCHTYPE (DDENUMSURFACES_ALL|DDENUMSURFACES_MATCH|DDENUMSURFACES_NOMATCH)

static BOOL
Main_DirectDraw_DDPIXELFORMAT_Match(const DDPIXELFORMAT *requested,
                                    const DDPIXELFORMAT *provided)
{
    /* Some flags must be present in both or neither for a match. */
    static const DWORD must_match = DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2
        | DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8 | DDPF_FOURCC
        | DDPF_ZBUFFER | DDPF_STENCILBUFFER;

    if ((requested->dwFlags & provided->dwFlags) != requested->dwFlags)
        return FALSE;

    if ((requested->dwFlags & must_match) != (provided->dwFlags & must_match))
        return FALSE;

    if (requested->dwFlags & DDPF_FOURCC)
        if (requested->dwFourCC != provided->dwFourCC)
            return FALSE;

    if (requested->dwFlags & (DDPF_RGB|DDPF_YUV|DDPF_ZBUFFER|DDPF_ALPHA
                              |DDPF_LUMINANCE|DDPF_BUMPDUDV))
        if (requested->u1.dwRGBBitCount != provided->u1.dwRGBBitCount)
            return FALSE;

    if (requested->dwFlags & (DDPF_RGB|DDPF_YUV|DDPF_STENCILBUFFER
                              |DDPF_LUMINANCE|DDPF_BUMPDUDV))
        if (requested->u2.dwRBitMask != provided->u2.dwRBitMask)
            return FALSE;

    if (requested->dwFlags & (DDPF_RGB|DDPF_YUV|DDPF_ZBUFFER|DDPF_BUMPDUDV))
        if (requested->u3.dwGBitMask != provided->u3.dwGBitMask)
            return FALSE;

    /* I could be wrong about the bumpmapping. MSDN docs are vague. */
    if (requested->dwFlags & (DDPF_RGB|DDPF_YUV|DDPF_STENCILBUFFER
                              |DDPF_BUMPDUDV))
        if (requested->u4.dwBBitMask != provided->u4.dwBBitMask)
            return FALSE;

    if (requested->dwFlags & (DDPF_ALPHAPIXELS|DDPF_ZPIXELS))
        if (requested->u5.dwRGBAlphaBitMask != provided->u5.dwRGBAlphaBitMask)
            return FALSE;

    return TRUE;
}

static BOOL ddraw_match_surface_desc(const DDSURFACEDESC2 *requested, const DDSURFACEDESC2 *provided)
{
    struct compare_info
    {
        DWORD flag;
        ptrdiff_t offset;
        size_t size;
    };

#define CMP(FLAG, FIELD)                                \
        { DDSD_##FLAG, offsetof(DDSURFACEDESC2, FIELD), \
          sizeof(((DDSURFACEDESC2 *)(NULL))->FIELD) }

    static const struct compare_info compare[] =
    {
        CMP(ALPHABITDEPTH, dwAlphaBitDepth),
        CMP(BACKBUFFERCOUNT, u5.dwBackBufferCount),
        CMP(CAPS, ddsCaps),
        CMP(CKDESTBLT, ddckCKDestBlt),
        CMP(CKDESTOVERLAY, u3 /* ddckCKDestOverlay */),
        CMP(CKSRCBLT, ddckCKSrcBlt),
        CMP(CKSRCOVERLAY, ddckCKSrcOverlay),
        CMP(HEIGHT, dwHeight),
        CMP(LINEARSIZE, u1 /* dwLinearSize */),
        CMP(LPSURFACE, lpSurface),
        CMP(MIPMAPCOUNT, u2 /* dwMipMapCount */),
        CMP(PITCH, u1 /* lPitch */),
        /* PIXELFORMAT: manual */
        CMP(REFRESHRATE, u2 /* dwRefreshRate */),
        CMP(TEXTURESTAGE, dwTextureStage),
        CMP(WIDTH, dwWidth),
        /* ZBUFFERBITDEPTH: "obsolete" */
    };

#undef CMP

    unsigned int i;

    if ((requested->dwFlags & provided->dwFlags) != requested->dwFlags)
        return FALSE;

    for (i=0; i < sizeof(compare)/sizeof(compare[0]); i++)
    {
        if (requested->dwFlags & compare[i].flag
            && memcmp((const char *)provided + compare[i].offset,
                      (const char *)requested + compare[i].offset,
                      compare[i].size) != 0)
            return FALSE;
    }

    if (requested->dwFlags & DDSD_PIXELFORMAT)
    {
        if (!Main_DirectDraw_DDPIXELFORMAT_Match(&requested->u4.ddpfPixelFormat,
                                                &provided->u4.ddpfPixelFormat))
            return FALSE;
    }

    return TRUE;
}

#undef DDENUMSURFACES_SEARCHTYPE
#undef DDENUMSURFACES_MATCHTYPE

struct surfacescallback2_context
{
    LPDDENUMSURFACESCALLBACK2 func;
    void *context;
};

struct surfacescallback_context
{
    LPDDENUMSURFACESCALLBACK func;
    void *context;
};

static HRESULT CALLBACK EnumSurfacesCallback2Thunk(IDirectDrawSurface7 *surface,
        DDSURFACEDESC2 *surface_desc, void *context)
{
    struct ddraw_surface *surface_impl = impl_from_IDirectDrawSurface7(surface);
    struct surfacescallback2_context *cbcontext = context;

    IDirectDrawSurface4_AddRef(&surface_impl->IDirectDrawSurface4_iface);
    IDirectDrawSurface7_Release(surface);

    return cbcontext->func(&surface_impl->IDirectDrawSurface4_iface,
            surface_desc, cbcontext->context);
}

static HRESULT CALLBACK EnumSurfacesCallbackThunk(IDirectDrawSurface7 *surface,
        DDSURFACEDESC2 *surface_desc, void *context)
{
    struct ddraw_surface *surface_impl = impl_from_IDirectDrawSurface7(surface);
    struct surfacescallback_context *cbcontext = context;

    IDirectDrawSurface_AddRef(&surface_impl->IDirectDrawSurface_iface);
    IDirectDrawSurface7_Release(surface);

    return cbcontext->func(&surface_impl->IDirectDrawSurface_iface,
            (DDSURFACEDESC *)surface_desc, cbcontext->context);
}

/*****************************************************************************
 * IDirectDraw7::EnumSurfaces
 *
 * Loops through all surfaces attached to this device and calls the
 * application callback. This can't be relayed to WineD3DDevice,
 * because some WineD3DSurfaces' parents are IParent objects
 *
 * Params:
 *  Flags: Some filtering flags. See IDirectDrawImpl_EnumSurfacesCallback
 *  DDSD: Description to filter for
 *  Context: Application-provided pointer, it's passed unmodified to the
 *           Callback function
 *  Callback: Address to call for each surface
 *
 * Returns:
 *  DDERR_INVALIDPARAMS if the callback is NULL
 *  DD_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_EnumSurfaces(IDirectDraw7 *iface, DWORD Flags,
        DDSURFACEDESC2 *DDSD, void *Context, LPDDENUMSURFACESCALLBACK7 Callback)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct ddraw_surface *surf;
    BOOL all, nomatch;
    DDSURFACEDESC2 desc;
    struct list *entry, *entry2;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, Flags, DDSD, Context, Callback);

    all = Flags & DDENUMSURFACES_ALL;
    nomatch = Flags & DDENUMSURFACES_NOMATCH;

    if (!Callback)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    /* Use the _SAFE enumeration, the app may destroy enumerated surfaces */
    LIST_FOR_EACH_SAFE(entry, entry2, &ddraw->surface_list)
    {
        surf = LIST_ENTRY(entry, struct ddraw_surface, surface_list_entry);

        if (!surf->iface_count)
        {
            WARN("Not enumerating surface %p because it doesn't have any references.\n", surf);
            continue;
        }

        if (all || (nomatch != ddraw_match_surface_desc(DDSD, &surf->surface_desc)))
        {
            TRACE("Enumerating surface %p.\n", surf);
            desc = surf->surface_desc;
            IDirectDrawSurface7_AddRef(&surf->IDirectDrawSurface7_iface);
            if (Callback(&surf->IDirectDrawSurface7_iface, &desc, Context) != DDENUMRET_OK)
            {
                wined3d_mutex_unlock();
                return DD_OK;
            }
        }
    }

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw4_EnumSurfaces(IDirectDraw4 *iface, DWORD flags,
        DDSURFACEDESC2 *surface_desc, void *context, LPDDENUMSURFACESCALLBACK2 callback)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);
    struct surfacescallback2_context cbcontext;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    return ddraw7_EnumSurfaces(&ddraw->IDirectDraw7_iface, flags, surface_desc,
            &cbcontext, EnumSurfacesCallback2Thunk);
}

static HRESULT WINAPI ddraw2_EnumSurfaces(IDirectDraw2 *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);
    struct surfacescallback_context cbcontext;
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw7_EnumSurfaces(&ddraw->IDirectDraw7_iface, flags,
            surface_desc ? &surface_desc2 : NULL, &cbcontext, EnumSurfacesCallbackThunk);
}

static HRESULT WINAPI ddraw1_EnumSurfaces(IDirectDraw *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);
    struct surfacescallback_context cbcontext;
    DDSURFACEDESC2 surface_desc2;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    if (surface_desc) DDSD_to_DDSD2(surface_desc, &surface_desc2);
    return ddraw7_EnumSurfaces(&ddraw->IDirectDraw7_iface, flags,
            surface_desc ? &surface_desc2 : NULL, &cbcontext, EnumSurfacesCallbackThunk);
}

/*****************************************************************************
 * DirectDrawCreateClipper (DDRAW.@)
 *
 * Creates a new IDirectDrawClipper object.
 *
 * Params:
 *  Clipper: Address to write the interface pointer to
 *  UnkOuter: For aggregation support, which ddraw doesn't have. Has to be
 *            NULL
 *
 * Returns:
 *  CLASS_E_NOAGGREGATION if UnkOuter != NULL
 *  E_OUTOFMEMORY if allocating the object failed
 *
 *****************************************************************************/
HRESULT WINAPI DirectDrawCreateClipper(DWORD flags, IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    struct ddraw_clipper *object;
    HRESULT hr;

    TRACE("flags %#x, clipper %p, outer_unknown %p.\n",
            flags, clipper, outer_unknown);

    if (outer_unknown)
        return CLASS_E_NOAGGREGATION;

    wined3d_mutex_lock();

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    hr = ddraw_clipper_init(object);
    if (FAILED(hr))
    {
        WARN("Failed to initialize clipper, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        wined3d_mutex_unlock();
        return hr;
    }

    TRACE("Created clipper %p.\n", object);
    *clipper = &object->IDirectDrawClipper_iface;
    wined3d_mutex_unlock();

    return DD_OK;
}

/*****************************************************************************
 * IDirectDraw7::CreateClipper
 *
 * Creates a DDraw clipper. See DirectDrawCreateClipper for details
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_CreateClipper(IDirectDraw7 *iface, DWORD Flags,
        IDirectDrawClipper **Clipper, IUnknown *UnkOuter)
{
    TRACE("iface %p, flags %#x, clipper %p, outer_unknown %p.\n",
            iface, Flags, Clipper, UnkOuter);

    return DirectDrawCreateClipper(Flags, Clipper, UnkOuter);
}

static HRESULT WINAPI ddraw4_CreateClipper(IDirectDraw4 *iface, DWORD flags,
        IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#x, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddraw7_CreateClipper(&ddraw->IDirectDraw7_iface, flags, clipper, outer_unknown);
}

static HRESULT WINAPI ddraw2_CreateClipper(IDirectDraw2 *iface,
        DWORD flags, IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, flags %#x, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddraw7_CreateClipper(&ddraw->IDirectDraw7_iface, flags, clipper, outer_unknown);
}

static HRESULT WINAPI ddraw1_CreateClipper(IDirectDraw *iface,
        DWORD flags, IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);

    TRACE("iface %p, flags %#x, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddraw7_CreateClipper(&ddraw->IDirectDraw7_iface, flags, clipper, outer_unknown);
}

/*****************************************************************************
 * IDirectDraw7::CreatePalette
 *
 * Creates a new IDirectDrawPalette object
 *
 * Params:
 *  Flags: The flags for the new clipper
 *  ColorTable: Color table to assign to the new clipper
 *  Palette: Address to write the interface pointer to
 *  UnkOuter: For aggregation support, which ddraw doesn't have. Has to be
 *            NULL
 *
 * Returns:
 *  CLASS_E_NOAGGREGATION if UnkOuter != NULL
 *  E_OUTOFMEMORY if allocating the object failed
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_CreatePalette(IDirectDraw7 *iface, DWORD Flags,
        PALETTEENTRY *ColorTable, IDirectDrawPalette **Palette, IUnknown *pUnkOuter)
{
    struct ddraw *ddraw = impl_from_IDirectDraw7(iface);
    struct ddraw_palette *object;
    HRESULT hr;

    TRACE("iface %p, flags %#x, color_table %p, palette %p, outer_unknown %p.\n",
            iface, Flags, ColorTable, Palette, pUnkOuter);

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    wined3d_mutex_lock();

    /* The refcount test shows that a cooplevel is required for this */
    if (!ddraw->cooperative_level)
    {
        WARN("No cooperative level set, returning DDERR_NOCOOPERATIVELEVELSET\n");
        wined3d_mutex_unlock();
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory when allocating memory for a palette implementation\n");
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    hr = ddraw_palette_init(object, ddraw, Flags, ColorTable);
    if (FAILED(hr))
    {
        WARN("Failed to initialize palette, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        wined3d_mutex_unlock();
        return hr;
    }

    TRACE("Created palette %p.\n", object);
    *Palette = &object->IDirectDrawPalette_iface;
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw4_CreatePalette(IDirectDraw4 *iface, DWORD flags, PALETTEENTRY *entries,
        IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    hr = ddraw7_CreatePalette(&ddraw->IDirectDraw7_iface, flags, entries, palette, outer_unknown);
    if (SUCCEEDED(hr) && *palette)
    {
        struct ddraw_palette *impl = impl_from_IDirectDrawPalette(*palette);
        IDirectDraw7_Release(&ddraw->IDirectDraw7_iface);
        IDirectDraw4_AddRef(iface);
        impl->ifaceToRelease = (IUnknown *)iface;
    }
    return hr;
}

static HRESULT WINAPI ddraw2_CreatePalette(IDirectDraw2 *iface, DWORD flags,
        PALETTEENTRY *entries, IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    hr = ddraw7_CreatePalette(&ddraw->IDirectDraw7_iface, flags, entries, palette, outer_unknown);
    if (SUCCEEDED(hr) && *palette)
    {
        struct ddraw_palette *impl = impl_from_IDirectDrawPalette(*palette);
        IDirectDraw7_Release(&ddraw->IDirectDraw7_iface);
        impl->ifaceToRelease = NULL;
    }

    return hr;
}

static HRESULT WINAPI ddraw1_CreatePalette(IDirectDraw *iface, DWORD flags,
        PALETTEENTRY *entries, IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    hr = ddraw7_CreatePalette(&ddraw->IDirectDraw7_iface, flags, entries, palette, outer_unknown);
    if (SUCCEEDED(hr) && *palette)
    {
        struct ddraw_palette *impl = impl_from_IDirectDrawPalette(*palette);
        IDirectDraw7_Release(&ddraw->IDirectDraw7_iface);
        impl->ifaceToRelease = NULL;
    }

    return hr;
}

/*****************************************************************************
 * IDirectDraw7::DuplicateSurface
 *
 * Duplicates a surface. The surface memory points to the same memory as
 * the original surface, and it's released when the last surface referencing
 * it is released. I guess that's beyond Wine's surface management right now
 * (Idea: create a new DDraw surface with the same WineD3DSurface. I need a
 * test application to implement this)
 *
 * Params:
 *  Src: Address of the source surface
 *  Dest: Address to write the new surface pointer to
 *
 * Returns:
 *  See IDirectDraw7::CreateSurface
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_DuplicateSurface(IDirectDraw7 *iface,
        IDirectDrawSurface7 *Src, IDirectDrawSurface7 **Dest)
{
    struct ddraw_surface *src_surface = unsafe_impl_from_IDirectDrawSurface7(Src);

    FIXME("iface %p, src %p, dst %p partial stub!\n", iface, Src, Dest);

    /* For now, simply create a new, independent surface */
    return IDirectDraw7_CreateSurface(iface, &src_surface->surface_desc, Dest, NULL);
}

static HRESULT WINAPI ddraw4_DuplicateSurface(IDirectDraw4 *iface, IDirectDrawSurface4 *src,
        IDirectDrawSurface4 **dst)
{
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface4(src);
    struct ddraw *ddraw = impl_from_IDirectDraw4(iface);
    struct ddraw_surface *dst_impl;
    IDirectDrawSurface7 *dst7;
    HRESULT hr;

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);

    hr = ddraw7_DuplicateSurface(&ddraw->IDirectDraw7_iface,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, &dst7);
    if (FAILED(hr))
    {
        *dst = NULL;
        return hr;
    }
    dst_impl = impl_from_IDirectDrawSurface7(dst7);
    *dst = &dst_impl->IDirectDrawSurface4_iface;
    IDirectDrawSurface4_AddRef(*dst);
    IDirectDrawSurface7_Release(dst7);

    return hr;
}

static HRESULT WINAPI ddraw2_DuplicateSurface(IDirectDraw2 *iface,
        IDirectDrawSurface *src, IDirectDrawSurface **dst)
{
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface(src);
    struct ddraw *ddraw = impl_from_IDirectDraw2(iface);
    struct ddraw_surface *dst_impl;
    IDirectDrawSurface7 *dst7;
    HRESULT hr;

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);

    hr = ddraw7_DuplicateSurface(&ddraw->IDirectDraw7_iface,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, &dst7);
    if (FAILED(hr))
        return hr;
    dst_impl = impl_from_IDirectDrawSurface7(dst7);
    *dst = &dst_impl->IDirectDrawSurface_iface;
    IDirectDrawSurface_AddRef(*dst);
    IDirectDrawSurface7_Release(dst7);

    return hr;
}

static HRESULT WINAPI ddraw1_DuplicateSurface(IDirectDraw *iface, IDirectDrawSurface *src,
        IDirectDrawSurface **dst)
{
    struct ddraw_surface *src_impl = unsafe_impl_from_IDirectDrawSurface(src);
    struct ddraw *ddraw = impl_from_IDirectDraw(iface);
    struct ddraw_surface *dst_impl;
    IDirectDrawSurface7 *dst7;
    HRESULT hr;

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);

    hr = ddraw7_DuplicateSurface(&ddraw->IDirectDraw7_iface,
            src_impl ? &src_impl->IDirectDrawSurface7_iface : NULL, &dst7);
    if (FAILED(hr))
        return hr;
    dst_impl = impl_from_IDirectDrawSurface7(dst7);
    *dst = &dst_impl->IDirectDrawSurface_iface;
    IDirectDrawSurface_AddRef(*dst);
    IDirectDrawSurface7_Release(dst7);

    return hr;
}

/*****************************************************************************
 * IDirect3D7::EnumDevices
 *
 * The EnumDevices method for IDirect3D7. It enumerates all supported
 * D3D7 devices. Currently the T&L, HAL and RGB devices are enumerated.
 *
 * Params:
 *  callback: Function to call for each enumerated device
 *  context: Pointer to pass back to the app
 *
 * Returns:
 *  D3D_OK, or the return value of the GetCaps call
 *
 *****************************************************************************/
static HRESULT WINAPI d3d7_EnumDevices(IDirect3D7 *iface, LPD3DENUMDEVICESCALLBACK7 callback, void *context)
{
    struct ddraw *ddraw = impl_from_IDirect3D7(iface);
    D3DDEVICEDESC7 device_desc7;
    D3DDEVICEDESC device_desc1;
    HRESULT hr;
    size_t i;

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    if (!callback)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    hr = IDirect3DImpl_GetCaps(ddraw->wined3d, &device_desc1, &device_desc7);
    if (hr != D3D_OK)
    {
        wined3d_mutex_unlock();
        return hr;
    }

    for (i = 0; i < sizeof(device_list7)/sizeof(device_list7[0]); i++)
    {
        HRESULT ret;

        device_desc7.deviceGUID = *device_list7[i].device_guid;
        ret = callback(device_list7[i].interface_name, device_list7[i].device_name, &device_desc7, context);
        if (ret != DDENUMRET_OK)
        {
            TRACE("Application cancelled the enumeration.\n");
            wined3d_mutex_unlock();
            return D3D_OK;
        }
    }

    TRACE("End of enumeration.\n");

    wined3d_mutex_unlock();

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3D3::EnumDevices
 *
 * Enumerates all supported Direct3DDevice interfaces. This is the
 * implementation for Direct3D 1 to Direc3D 3, Version 7 has its own.
 *
 * Version 1, 2 and 3
 *
 * Params:
 *  callback: Application-provided routine to call for each enumerated device
 *  Context: Pointer to pass to the callback
 *
 * Returns:
 *  D3D_OK on success,
 *  The result of IDirect3DImpl_GetCaps if it failed
 *
 *****************************************************************************/
static HRESULT WINAPI d3d3_EnumDevices(IDirect3D3 *iface, LPD3DENUMDEVICESCALLBACK callback, void *context)
{
    static CHAR wined3d_description[] = "Wine D3DDevice using WineD3D and OpenGL";

    struct ddraw *ddraw = impl_from_IDirect3D3(iface);
    D3DDEVICEDESC device_desc1, hal_desc, hel_desc;
    D3DDEVICEDESC7 device_desc7;
    HRESULT hr;

    /* Some games (Motoracer 2 demo) have the bad idea to modify the device
     * name string. Let's put the string in a sufficiently sized array in
     * writable memory. */
    char device_name[50];
    strcpy(device_name,"Direct3D HEL");

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    if (!callback)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    hr = IDirect3DImpl_GetCaps(ddraw->wined3d, &device_desc1, &device_desc7);
    if (hr != D3D_OK)
    {
        wined3d_mutex_unlock();
        return hr;
    }

    /* Do I have to enumerate the reference id? Note from old d3d7:
     * "It seems that enumerating the reference IID on Direct3D 1 games
     * (AvP / Motoracer2) breaks them". So do not enumerate this iid in V1
     *
     * There's a registry key HKLM\Software\Microsoft\Direct3D\Drivers,
     * EnumReference which enables / disables enumerating the reference
     * rasterizer. It's a DWORD, 0 means disabled, 2 means enabled. The
     * enablerefrast.reg and disablerefrast.reg files in the DirectX 7.0 sdk
     * demo directory suggest this.
     *
     * Some games(GTA 2) seem to use the second enumerated device, so I have
     * to enumerate at least 2 devices. So enumerate the reference device to
     * have 2 devices.
     *
     * Other games (Rollcage) tell emulation and hal device apart by certain
     * flags. Rollcage expects D3DPTEXTURECAPS_POW2 to be set (yeah, it is a
     * limitation flag), and it refuses all devices that have the perspective
     * flag set. This way it refuses the emulation device, and HAL devices
     * never have POW2 unset in d3d7 on windows. */
    if (ddraw->d3dversion != 1)
    {
        static CHAR reference_description[] = "RGB Direct3D emulation";

        TRACE("Enumerating WineD3D D3DDevice interface.\n");
        hal_desc = device_desc1;
        hel_desc = device_desc1;
        /* The rgb device has the pow2 flag set in the hel caps, but not in the hal caps. */
        hal_desc.dpcLineCaps.dwTextureCaps &= ~(D3DPTEXTURECAPS_POW2
                | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PERSPECTIVE);
        hal_desc.dpcTriCaps.dwTextureCaps &= ~(D3DPTEXTURECAPS_POW2
                | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PERSPECTIVE);
        /* RGB, RAMP and MMX devices have a HAL dcmColorModel of 0 */
        hal_desc.dcmColorModel = 0;

        hr = callback((GUID *)&IID_IDirect3DRGBDevice, reference_description,
                device_name, &hal_desc, &hel_desc, context);
        if (hr != D3DENUMRET_OK)
        {
            TRACE("Application cancelled the enumeration.\n");
            wined3d_mutex_unlock();
            return D3D_OK;
        }
    }

    strcpy(device_name,"Direct3D HAL");

    TRACE("Enumerating HAL Direct3D device.\n");
    hal_desc = device_desc1;
    hel_desc = device_desc1;

    /* The hal device does not have the pow2 flag set in hel, but in hal. */
    hel_desc.dpcLineCaps.dwTextureCaps &= ~(D3DPTEXTURECAPS_POW2
            | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PERSPECTIVE);
    hel_desc.dpcTriCaps.dwTextureCaps &= ~(D3DPTEXTURECAPS_POW2
            | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PERSPECTIVE);
    /* HAL devices have a HEL dcmColorModel of 0 */
    hel_desc.dcmColorModel = 0;

    hr = callback((GUID *)&IID_IDirect3DHALDevice, wined3d_description,
            device_name, &hal_desc, &hel_desc, context);
    if (hr != D3DENUMRET_OK)
    {
        TRACE("Application cancelled the enumeration.\n");
        wined3d_mutex_unlock();
        return D3D_OK;
    }

    TRACE("End of enumeration.\n");

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d2_EnumDevices(IDirect3D2 *iface, LPD3DENUMDEVICESCALLBACK callback, void *context)
{
    struct ddraw *ddraw = impl_from_IDirect3D2(iface);

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    return d3d3_EnumDevices(&ddraw->IDirect3D3_iface, callback, context);
}

static HRESULT WINAPI d3d1_EnumDevices(IDirect3D *iface, LPD3DENUMDEVICESCALLBACK callback, void *context)
{
    struct ddraw *ddraw = impl_from_IDirect3D(iface);

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    return d3d3_EnumDevices(&ddraw->IDirect3D3_iface, callback, context);
}

/*****************************************************************************
 * IDirect3D3::CreateLight
 *
 * Creates an IDirect3DLight interface. This interface is used in
 * Direct3D3 or earlier for lighting. In Direct3D7 it has been replaced
 * by the DIRECT3DLIGHT7 structure. Wine's Direct3DLight implementation
 * uses the IDirect3DDevice7 interface with D3D7 lights.
 *
 * Version 1, 2 and 3
 *
 * Params:
 *  light: Address to store the new interface pointer
 *  outer_unknown: Basically for aggregation, but ddraw doesn't support it.
 *                 Must be NULL
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if memory allocation failed
 *  CLASS_E_NOAGGREGATION if outer_unknown != NULL
 *
 *****************************************************************************/
static HRESULT WINAPI d3d3_CreateLight(IDirect3D3 *iface, IDirect3DLight **light,
        IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D3(iface);
    struct d3d_light *object;

    TRACE("iface %p, light %p, outer_unknown %p.\n", iface, light, outer_unknown);

    if (outer_unknown) return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate light memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    d3d_light_init(object, ddraw);

    TRACE("Created light %p.\n", object);
    *light = &object->IDirect3DLight_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d2_CreateLight(IDirect3D2 *iface, IDirect3DLight **light, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D2(iface);

    TRACE("iface %p, light %p, outer_unknown %p.\n", iface, light, outer_unknown);

    return d3d3_CreateLight(&ddraw->IDirect3D3_iface, light, outer_unknown);
}

static HRESULT WINAPI d3d1_CreateLight(IDirect3D *iface, IDirect3DLight **light, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D(iface);

    TRACE("iface %p, light %p, outer_unknown %p.\n", iface, light, outer_unknown);

    return d3d3_CreateLight(&ddraw->IDirect3D3_iface, light, outer_unknown);
}

/*****************************************************************************
 * IDirect3D3::CreateMaterial
 *
 * Creates an IDirect3DMaterial interface. This interface is used by Direct3D3
 * and older versions. The IDirect3DMaterial implementation wraps its
 * functionality to IDirect3DDevice7::SetMaterial and friends.
 *
 * Version 1, 2 and 3
 *
 * Params:
 *  material: Address to store the new interface's pointer to
 *  outer_unknown: Basically for aggregation, but ddraw doesn't support it.
 *                 Must be NULL
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if memory allocation failed
 *  CLASS_E_NOAGGREGATION if outer_unknown != NULL
 *
 *****************************************************************************/
static HRESULT WINAPI d3d3_CreateMaterial(IDirect3D3 *iface, IDirect3DMaterial3 **material,
        IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D3(iface);
    struct d3d_material *object;

    TRACE("iface %p, material %p, outer_unknown %p.\n", iface, material, outer_unknown);

    if (outer_unknown) return CLASS_E_NOAGGREGATION;

    object = d3d_material_create(ddraw);
    if (!object)
    {
        ERR("Failed to allocate material memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    TRACE("Created material %p.\n", object);
    *material = &object->IDirect3DMaterial3_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d2_CreateMaterial(IDirect3D2 *iface, IDirect3DMaterial2 **material,
        IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D2(iface);
    struct d3d_material *object;

    TRACE("iface %p, material %p, outer_unknown %p.\n", iface, material, outer_unknown);

    object = d3d_material_create(ddraw);
    if (!object)
    {
        ERR("Failed to allocate material memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    TRACE("Created material %p.\n", object);
    *material = &object->IDirect3DMaterial2_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d1_CreateMaterial(IDirect3D *iface, IDirect3DMaterial **material,
        IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D(iface);
    struct d3d_material *object;

    TRACE("iface %p, material %p, outer_unknown %p.\n", iface, material, outer_unknown);

    object = d3d_material_create(ddraw);
    if (!object)
    {
        ERR("Failed to allocate material memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    TRACE("Created material %p.\n", object);
    *material = &object->IDirect3DMaterial_iface;

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3D3::CreateViewport
 *
 * Creates an IDirect3DViewport interface. This interface is used
 * by Direct3D and earlier versions for Viewport management. In Direct3D7
 * it has been replaced by a viewport structure and
 * IDirect3DDevice7::*Viewport. Wine's IDirect3DViewport implementation
 * uses the IDirect3DDevice7 methods for its functionality
 *
 * Params:
 *  Viewport: Address to store the new interface pointer
 *  outer_unknown: Basically for aggregation, but ddraw doesn't support it.
 *                 Must be NULL
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if memory allocation failed
 *  CLASS_E_NOAGGREGATION if outer_unknown != NULL
 *
 *****************************************************************************/
static HRESULT WINAPI d3d3_CreateViewport(IDirect3D3 *iface, IDirect3DViewport3 **viewport,
        IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D3(iface);
    struct d3d_viewport *object;

    TRACE("iface %p, viewport %p, outer_unknown %p.\n", iface, viewport, outer_unknown);

    if (outer_unknown) return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate viewport memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    d3d_viewport_init(object, ddraw);

    TRACE("Created viewport %p.\n", object);
    *viewport = &object->IDirect3DViewport3_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d2_CreateViewport(IDirect3D2 *iface, IDirect3DViewport2 **viewport, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D2(iface);

    TRACE("iface %p, viewport %p, outer_unknown %p.\n", iface, viewport, outer_unknown);

    return d3d3_CreateViewport(&ddraw->IDirect3D3_iface, (IDirect3DViewport3 **)viewport,
            outer_unknown);
}

static HRESULT WINAPI d3d1_CreateViewport(IDirect3D *iface, IDirect3DViewport **viewport, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D(iface);

    TRACE("iface %p, viewport %p, outer_unknown %p.\n", iface, viewport, outer_unknown);

    return d3d3_CreateViewport(&ddraw->IDirect3D3_iface, (IDirect3DViewport3 **)viewport,
            outer_unknown);
}

/*****************************************************************************
 * IDirect3D3::FindDevice
 *
 * This method finds a device with the requested properties and returns a
 * device description
 *
 * Verion 1, 2 and 3
 * Params:
 *  fds: Describes the requested device characteristics
 *  fdr: Returns the device description
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if no device was found
 *
 *****************************************************************************/
static HRESULT WINAPI d3d3_FindDevice(IDirect3D3 *iface, D3DFINDDEVICESEARCH *fds, D3DFINDDEVICERESULT *fdr)
{
    struct ddraw *ddraw = impl_from_IDirect3D3(iface);
    D3DDEVICEDESC7 desc7;
    D3DDEVICEDESC desc1;
    HRESULT hr;

    TRACE("iface %p, fds %p, fdr %p.\n", iface, fds, fdr);

    if (!fds || !fdr) return DDERR_INVALIDPARAMS;

    if (fds->dwSize != sizeof(D3DFINDDEVICESEARCH)
            || fdr->dwSize != sizeof(D3DFINDDEVICERESULT))
        return DDERR_INVALIDPARAMS;

    if ((fds->dwFlags & D3DFDS_COLORMODEL)
            && fds->dcmColorModel != D3DCOLOR_RGB)
    {
        WARN("Trying to request a non-RGB D3D color model. Not supported.\n");
        return DDERR_INVALIDPARAMS; /* No real idea what to return here :-) */
    }

    if (fds->dwFlags & D3DFDS_GUID)
    {
        TRACE("Trying to match guid %s.\n", debugstr_guid(&(fds->guid)));
        if (!IsEqualGUID(&IID_D3DDEVICE_WineD3D, &fds->guid)
                && !IsEqualGUID(&IID_IDirect3DHALDevice, &fds->guid)
                && !IsEqualGUID(&IID_IDirect3DRGBDevice, &fds->guid))
        {
            WARN("No match for this GUID.\n");
            return DDERR_NOTFOUND;
        }
    }

    /* Get the caps */
    hr = IDirect3DImpl_GetCaps(ddraw->wined3d, &desc1, &desc7);
    if (hr != D3D_OK) return hr;

    /* Now return our own GUID */
    fdr->guid = IID_D3DDEVICE_WineD3D;
    fdr->ddHwDesc = desc1;
    fdr->ddSwDesc = desc1;

    TRACE("Returning Wine's wined3d device with (undumped) capabilities.\n");

    return D3D_OK;
}

static HRESULT WINAPI d3d2_FindDevice(IDirect3D2 *iface, D3DFINDDEVICESEARCH *fds, D3DFINDDEVICERESULT *fdr)
{
    struct ddraw *ddraw = impl_from_IDirect3D2(iface);

    TRACE("iface %p, fds %p, fdr %p.\n", iface, fds, fdr);

    return d3d3_FindDevice(&ddraw->IDirect3D3_iface, fds, fdr);
}

static HRESULT WINAPI d3d1_FindDevice(IDirect3D *iface, D3DFINDDEVICESEARCH *fds, D3DFINDDEVICERESULT *fdr)
{
    struct ddraw *ddraw = impl_from_IDirect3D(iface);

    TRACE("iface %p, fds %p, fdr %p.\n", iface, fds, fdr);

    return d3d3_FindDevice(&ddraw->IDirect3D3_iface, fds, fdr);
}

/*****************************************************************************
 * IDirect3D7::CreateDevice
 *
 * Creates an IDirect3DDevice7 interface.
 *
 * Version 2, 3 and 7. IDirect3DDevice 1 interfaces are interfaces to
 * DirectDraw surfaces and are created with
 * IDirectDrawSurface::QueryInterface. This method uses CreateDevice to
 * create the device object and QueryInterfaces for IDirect3DDevice
 *
 * Params:
 *  refiid: IID of the device to create
 *  Surface: Initial rendertarget
 *  Device: Address to return the interface pointer
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if memory allocation failed
 *  DDERR_INVALIDPARAMS if a device exists already
 *
 *****************************************************************************/
static HRESULT WINAPI d3d7_CreateDevice(IDirect3D7 *iface, REFCLSID riid,
        IDirectDrawSurface7 *surface, IDirect3DDevice7 **device)
{
    struct ddraw_surface *target = unsafe_impl_from_IDirectDrawSurface7(surface);
    struct ddraw *ddraw = impl_from_IDirect3D7(iface);
    struct d3d_device *object;
    HRESULT hr;

    TRACE("iface %p, riid %s, surface %p, device %p.\n", iface, debugstr_guid(riid), surface, device);

    wined3d_mutex_lock();
    hr = d3d_device_create(ddraw, target, 7, &object, NULL);
    if (SUCCEEDED(hr))
        *device = &object->IDirect3DDevice7_iface;
    else
    {
        WARN("Failed to create device, hr %#x.\n", hr);
        *device = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d3_CreateDevice(IDirect3D3 *iface, REFCLSID riid,
        IDirectDrawSurface4 *surface, IDirect3DDevice3 **device, IUnknown *outer_unknown)
{
    struct ddraw_surface *surface_impl = unsafe_impl_from_IDirectDrawSurface4(surface);
    struct ddraw *ddraw = impl_from_IDirect3D3(iface);
    struct d3d_device *device_impl;
    HRESULT hr;

    TRACE("iface %p, riid %s, surface %p, device %p, outer_unknown %p.\n",
            iface, debugstr_guid(riid), surface, device, outer_unknown);

    if (outer_unknown)
        return CLASS_E_NOAGGREGATION;

    wined3d_mutex_lock();
    hr = d3d_device_create(ddraw, surface_impl, 3, &device_impl, NULL);
    if (SUCCEEDED(hr))
        *device = &device_impl->IDirect3DDevice3_iface;
    else
    {
        WARN("Failed to create device, hr %#x.\n", hr);
        *device = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d2_CreateDevice(IDirect3D2 *iface, REFCLSID riid,
        IDirectDrawSurface *surface, IDirect3DDevice2 **device)
{
    struct ddraw_surface *surface_impl = unsafe_impl_from_IDirectDrawSurface(surface);
    struct ddraw *ddraw = impl_from_IDirect3D2(iface);
    struct d3d_device *device_impl;
    HRESULT hr;

    TRACE("iface %p, riid %s, surface %p, device %p.\n",
            iface, debugstr_guid(riid), surface, device);

    wined3d_mutex_lock();
    hr = d3d_device_create(ddraw, surface_impl, 2, &device_impl, NULL);
    if (SUCCEEDED(hr))
        *device = &device_impl->IDirect3DDevice2_iface;
    else
    {
        WARN("Failed to create device, hr %#x.\n", hr);
        *device = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

/*****************************************************************************
 * IDirect3D7::CreateVertexBuffer
 *
 * Creates a new vertex buffer object and returns a IDirect3DVertexBuffer7
 * interface.
 *
 * Version 3 and 7
 *
 * Params:
 *  desc: Requested Vertex buffer properties
 *  vertex_buffer: Address to return the interface pointer at
 *  flags: Some flags, should be 0
 *
 * Returns
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if memory allocation failed
 *  The return value of IWineD3DDevice::CreateVertexBuffer if this call fails
 *  DDERR_INVALIDPARAMS if desc or vertex_buffer are NULL
 *
 *****************************************************************************/
static HRESULT WINAPI d3d7_CreateVertexBuffer(IDirect3D7 *iface, D3DVERTEXBUFFERDESC *desc,
        IDirect3DVertexBuffer7 **vertex_buffer, DWORD flags)
{
    struct ddraw *ddraw = impl_from_IDirect3D7(iface);
    struct d3d_vertex_buffer *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, vertex_buffer %p, flags %#x.\n",
            iface, desc, vertex_buffer, flags);

    if (!vertex_buffer || !desc) return DDERR_INVALIDPARAMS;

    hr = d3d_vertex_buffer_create(&object, ddraw, desc);
    if (hr == D3D_OK)
    {
        TRACE("Created vertex buffer %p.\n", object);
        *vertex_buffer = &object->IDirect3DVertexBuffer7_iface;
    }
    else
        WARN("Failed to create vertex buffer, hr %#x.\n", hr);

    return hr;
}

static HRESULT WINAPI d3d3_CreateVertexBuffer(IDirect3D3 *iface, D3DVERTEXBUFFERDESC *desc,
        IDirect3DVertexBuffer **vertex_buffer, DWORD flags, IUnknown *outer_unknown)
{
    struct ddraw *ddraw = impl_from_IDirect3D3(iface);
    struct d3d_vertex_buffer *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, vertex_buffer %p, flags %#x, outer_unknown %p.\n",
            iface, desc, vertex_buffer, flags, outer_unknown);

    if (outer_unknown)
        return CLASS_E_NOAGGREGATION;
    if (!vertex_buffer || !desc)
        return DDERR_INVALIDPARAMS;

    hr = d3d_vertex_buffer_create(&object, ddraw, desc);
    if (hr == D3D_OK)
    {
        TRACE("Created vertex buffer %p.\n", object);
        *vertex_buffer = &object->IDirect3DVertexBuffer_iface;
    }
    else
        WARN("Failed to create vertex buffer, hr %#x.\n", hr);

    return hr;
}

/*****************************************************************************
 * IDirect3D7::EnumZBufferFormats
 *
 * Enumerates all supported Z buffer pixel formats
 *
 * Version 3 and 7
 *
 * Params:
 *  device_iid:
 *  callback: callback to call for each pixel format
 *  context: Pointer to pass back to the callback
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if callback is NULL
 *  For details, see IWineD3DDevice::EnumZBufferFormats
 *
 *****************************************************************************/
static HRESULT WINAPI d3d7_EnumZBufferFormats(IDirect3D7 *iface, REFCLSID device_iid,
        LPD3DENUMPIXELFORMATSCALLBACK callback, void *context)
{
    struct ddraw *ddraw = impl_from_IDirect3D7(iface);
    struct wined3d_display_mode mode;
    enum wined3d_device_type type;
    unsigned int i;
    HRESULT hr;

    /* Order matters. Specifically, BattleZone II (full version) expects the
     * 16-bit depth formats to be listed before the 24 and 32 ones. */
    static const enum wined3d_format_id formats[] =
    {
        WINED3DFMT_S1_UINT_D15_UNORM,
        WINED3DFMT_D16_UNORM,
        WINED3DFMT_X8D24_UNORM,
        WINED3DFMT_S4X4_UINT_D24_UNORM,
        WINED3DFMT_D24_UNORM_S8_UINT,
        WINED3DFMT_D32_UNORM,
    };

    TRACE("iface %p, device_iid %s, callback %p, context %p.\n",
            iface, debugstr_guid(device_iid), callback, context);

    if (!callback) return DDERR_INVALIDPARAMS;

    if (IsEqualGUID(device_iid, &IID_IDirect3DHALDevice)
            || IsEqualGUID(device_iid, &IID_IDirect3DTnLHalDevice)
            || IsEqualGUID(device_iid, &IID_D3DDEVICE_WineD3D))
    {
        TRACE("Asked for HAL device.\n");
        type = WINED3D_DEVICE_TYPE_HAL;
    }
    else if (IsEqualGUID(device_iid, &IID_IDirect3DRGBDevice)
            || IsEqualGUID(device_iid, &IID_IDirect3DMMXDevice))
    {
        TRACE("Asked for SW device.\n");
        type = WINED3D_DEVICE_TYPE_SW;
    }
    else if (IsEqualGUID(device_iid, &IID_IDirect3DRefDevice))
    {
        TRACE("Asked for REF device.\n");
        type = WINED3D_DEVICE_TYPE_REF;
    }
    else if (IsEqualGUID(device_iid, &IID_IDirect3DNullDevice))
    {
        TRACE("Asked for NULLREF device.\n");
        type = WINED3D_DEVICE_TYPE_NULLREF;
    }
    else
    {
        FIXME("Unexpected device GUID %s.\n", debugstr_guid(device_iid));
        type = WINED3D_DEVICE_TYPE_HAL;
    }

    wined3d_mutex_lock();
    /* We need an adapter format from somewhere to please wined3d and WGL.
     * Use the current display mode. So far all cards offer the same depth
     * stencil format for all modes, but if some do not and applications do
     * not like that we'll have to find some workaround, like iterating over
     * all imaginable formats and collecting all the depth stencil formats we
     * can get. */
    if (FAILED(hr = wined3d_get_adapter_display_mode(ddraw->wined3d, WINED3DADAPTER_DEFAULT, &mode, NULL)))
    {
        ERR("Failed to get display mode, hr %#x.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }

    for (i = 0; i < (sizeof(formats) / sizeof(*formats)); ++i)
    {
        if (SUCCEEDED(wined3d_check_device_format(ddraw->wined3d, WINED3DADAPTER_DEFAULT, type, mode.format_id,
                WINED3DUSAGE_DEPTHSTENCIL, WINED3D_RTYPE_SURFACE, formats[i])))
        {
            DDPIXELFORMAT pformat;

            memset(&pformat, 0, sizeof(pformat));
            pformat.dwSize = sizeof(pformat);
            PixelFormat_WineD3DtoDD(&pformat, formats[i]);

            TRACE("Enumerating wined3d format %#x.\n", formats[i]);
            hr = callback(&pformat, context);
            if (hr != DDENUMRET_OK)
            {
                TRACE("Format enumeration cancelled by application.\n");
                wined3d_mutex_unlock();
                return D3D_OK;
            }
        }
    }

    /* Historically some windows drivers used dwZBufferBitDepth=24 for WINED3DFMT_X8D24_UNORM,
     * while others used dwZBufferBitDepth=32. In either case the pitch matches a 32 bits per
     * pixel format, so we use dwZBufferBitDepth=32. Some games expect 24. Windows Vista and
     * newer enumerate both versions, so we do the same(bug 22434) */
    if (SUCCEEDED(wined3d_check_device_format(ddraw->wined3d, WINED3DADAPTER_DEFAULT, type, mode.format_id,
            WINED3DUSAGE_DEPTHSTENCIL, WINED3D_RTYPE_SURFACE, WINED3DFMT_X8D24_UNORM)))
    {
        DDPIXELFORMAT x8d24 =
        {
            sizeof(x8d24), DDPF_ZBUFFER, 0,
            {24}, {0x00000000}, {0x00ffffff}, {0x00000000}
        };
        TRACE("Enumerating WINED3DFMT_X8D24_UNORM, dwZBufferBitDepth=24 version\n");
        callback(&x8d24, context);
    }

    TRACE("End of enumeration.\n");

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d3_EnumZBufferFormats(IDirect3D3 *iface, REFCLSID device_iid,
        LPD3DENUMPIXELFORMATSCALLBACK callback, void *context)
{
    struct ddraw *ddraw = impl_from_IDirect3D3(iface);

    TRACE("iface %p, device_iid %s, callback %p, context %p.\n",
            iface, debugstr_guid(device_iid), callback, context);

    return d3d7_EnumZBufferFormats(&ddraw->IDirect3D7_iface, device_iid, callback, context);
}

/*****************************************************************************
 * IDirect3D7::EvictManagedTextures
 *
 * Removes all managed textures (=surfaces with DDSCAPS2_TEXTUREMANAGE or
 * DDSCAPS2_D3DTEXTUREMANAGE caps) to be removed from video memory.
 *
 * Version 3 and 7
 *
 * Returns:
 *  D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI d3d7_EvictManagedTextures(IDirect3D7 *iface)
{
    struct ddraw *ddraw = impl_from_IDirect3D7(iface);

    TRACE("iface %p!\n", iface);

    wined3d_mutex_lock();
    if (ddraw->d3d_initialized)
        wined3d_device_evict_managed_resources(ddraw->wined3d_device);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d3_EvictManagedTextures(IDirect3D3 *iface)
{
    struct ddraw *ddraw = impl_from_IDirect3D3(iface);

    TRACE("iface %p.\n", iface);

    return d3d7_EvictManagedTextures(&ddraw->IDirect3D7_iface);
}

/*****************************************************************************
 * IDirect3DImpl_GetCaps
 *
 * This function retrieves the device caps from wined3d
 * and converts it into a D3D7 and D3D - D3D3 structure
 * This is a helper function called from various places in ddraw
 *
 * Params:
 *  wined3d: The interface to get the caps from
 *  desc1: Old D3D <3 structure to fill (needed)
 *  desc7: D3D7 device desc structure to fill (needed)
 *
 * Returns
 *  D3D_OK on success, or the return value of IWineD3D::GetCaps
 *
 *****************************************************************************/
HRESULT IDirect3DImpl_GetCaps(const struct wined3d *wined3d, D3DDEVICEDESC *desc1, D3DDEVICEDESC7 *desc7)
{
    WINED3DCAPS wined3d_caps;
    HRESULT hr;

    TRACE("wined3d %p, desc1 %p, desc7 %p.\n", wined3d, desc1, desc7);

    memset(&wined3d_caps, 0, sizeof(wined3d_caps));

    wined3d_mutex_lock();
    hr = wined3d_get_device_caps(wined3d, 0, WINED3D_DEVICE_TYPE_HAL, &wined3d_caps);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to get device caps, hr %#x.\n", hr);
        return hr;
    }

    /* Copy the results into the d3d7 and d3d3 structures */
    desc7->dwDevCaps = wined3d_caps.DevCaps;
    desc7->dpcLineCaps.dwMiscCaps = wined3d_caps.PrimitiveMiscCaps;
    desc7->dpcLineCaps.dwRasterCaps = wined3d_caps.RasterCaps;
    desc7->dpcLineCaps.dwZCmpCaps = wined3d_caps.ZCmpCaps;
    desc7->dpcLineCaps.dwSrcBlendCaps = wined3d_caps.SrcBlendCaps;
    desc7->dpcLineCaps.dwDestBlendCaps = wined3d_caps.DestBlendCaps;
    desc7->dpcLineCaps.dwAlphaCmpCaps = wined3d_caps.AlphaCmpCaps;
    desc7->dpcLineCaps.dwShadeCaps = wined3d_caps.ShadeCaps;
    desc7->dpcLineCaps.dwTextureCaps = wined3d_caps.TextureCaps;
    desc7->dpcLineCaps.dwTextureFilterCaps = wined3d_caps.TextureFilterCaps;
    desc7->dpcLineCaps.dwTextureAddressCaps = wined3d_caps.TextureAddressCaps;

    desc7->dwMaxTextureWidth = wined3d_caps.MaxTextureWidth;
    desc7->dwMaxTextureHeight = wined3d_caps.MaxTextureHeight;

    desc7->dwMaxTextureRepeat = wined3d_caps.MaxTextureRepeat;
    desc7->dwMaxTextureAspectRatio = wined3d_caps.MaxTextureAspectRatio;
    desc7->dwMaxAnisotropy = wined3d_caps.MaxAnisotropy;
    desc7->dvMaxVertexW = wined3d_caps.MaxVertexW;

    desc7->dvGuardBandLeft = wined3d_caps.GuardBandLeft;
    desc7->dvGuardBandTop = wined3d_caps.GuardBandTop;
    desc7->dvGuardBandRight = wined3d_caps.GuardBandRight;
    desc7->dvGuardBandBottom = wined3d_caps.GuardBandBottom;

    desc7->dvExtentsAdjust = wined3d_caps.ExtentsAdjust;
    desc7->dwStencilCaps = wined3d_caps.StencilCaps;

    desc7->dwFVFCaps = wined3d_caps.FVFCaps;
    desc7->dwTextureOpCaps = wined3d_caps.TextureOpCaps;

    desc7->dwVertexProcessingCaps = wined3d_caps.VertexProcessingCaps;
    desc7->dwMaxActiveLights = wined3d_caps.MaxActiveLights;

    /* Remove all non-d3d7 caps */
    desc7->dwDevCaps &= (
        D3DDEVCAPS_FLOATTLVERTEX         | D3DDEVCAPS_SORTINCREASINGZ          | D3DDEVCAPS_SORTDECREASINGZ          |
        D3DDEVCAPS_SORTEXACT             | D3DDEVCAPS_EXECUTESYSTEMMEMORY      | D3DDEVCAPS_EXECUTEVIDEOMEMORY       |
        D3DDEVCAPS_TLVERTEXSYSTEMMEMORY  | D3DDEVCAPS_TLVERTEXVIDEOMEMORY      | D3DDEVCAPS_TEXTURESYSTEMMEMORY      |
        D3DDEVCAPS_TEXTUREVIDEOMEMORY    | D3DDEVCAPS_DRAWPRIMTLVERTEX         | D3DDEVCAPS_CANRENDERAFTERFLIP       |
        D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_DRAWPRIMITIVES2          | D3DDEVCAPS_SEPARATETEXTUREMEMORIES  |
        D3DDEVCAPS_DRAWPRIMITIVES2EX     | D3DDEVCAPS_HWTRANSFORMANDLIGHT      | D3DDEVCAPS_CANBLTSYSTONONLOCAL      |
        D3DDEVCAPS_HWRASTERIZATION);

    desc7->dwStencilCaps &= (
        D3DSTENCILCAPS_KEEP              | D3DSTENCILCAPS_ZERO                 | D3DSTENCILCAPS_REPLACE              |
        D3DSTENCILCAPS_INCRSAT           | D3DSTENCILCAPS_DECRSAT              | D3DSTENCILCAPS_INVERT               |
        D3DSTENCILCAPS_INCR              | D3DSTENCILCAPS_DECR);

    /* FVF caps ?*/

    desc7->dwTextureOpCaps &= (
        D3DTEXOPCAPS_DISABLE             | D3DTEXOPCAPS_SELECTARG1             | D3DTEXOPCAPS_SELECTARG2             |
        D3DTEXOPCAPS_MODULATE            | D3DTEXOPCAPS_MODULATE2X             | D3DTEXOPCAPS_MODULATE4X             |
        D3DTEXOPCAPS_ADD                 | D3DTEXOPCAPS_ADDSIGNED              | D3DTEXOPCAPS_ADDSIGNED2X            |
        D3DTEXOPCAPS_SUBTRACT            | D3DTEXOPCAPS_ADDSMOOTH              | D3DTEXOPCAPS_BLENDTEXTUREALPHA      |
        D3DTEXOPCAPS_BLENDFACTORALPHA    | D3DTEXOPCAPS_BLENDTEXTUREALPHAPM    | D3DTEXOPCAPS_BLENDCURRENTALPHA      |
        D3DTEXOPCAPS_PREMODULATE         | D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR | D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA |
        D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR | D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA | D3DTEXOPCAPS_BUMPENVMAP    |
        D3DTEXOPCAPS_BUMPENVMAPLUMINANCE | D3DTEXOPCAPS_DOTPRODUCT3);

    desc7->dwVertexProcessingCaps &= (
        D3DVTXPCAPS_TEXGEN               | D3DVTXPCAPS_MATERIALSOURCE7         | D3DVTXPCAPS_VERTEXFOG               |
        D3DVTXPCAPS_DIRECTIONALLIGHTS    | D3DVTXPCAPS_POSITIONALLIGHTS        | D3DVTXPCAPS_LOCALVIEWER);

    desc7->dpcLineCaps.dwMiscCaps &= (
        D3DPMISCCAPS_MASKPLANES          | D3DPMISCCAPS_MASKZ                  | D3DPMISCCAPS_LINEPATTERNREP         |
        D3DPMISCCAPS_CONFORMANT          | D3DPMISCCAPS_CULLNONE               | D3DPMISCCAPS_CULLCW                 |
        D3DPMISCCAPS_CULLCCW);

    desc7->dpcLineCaps.dwRasterCaps &= (
        D3DPRASTERCAPS_DITHER            | D3DPRASTERCAPS_ROP2                 | D3DPRASTERCAPS_XOR                  |
        D3DPRASTERCAPS_PAT               | D3DPRASTERCAPS_ZTEST                | D3DPRASTERCAPS_SUBPIXEL             |
        D3DPRASTERCAPS_SUBPIXELX         | D3DPRASTERCAPS_FOGVERTEX            | D3DPRASTERCAPS_FOGTABLE             |
        D3DPRASTERCAPS_STIPPLE           | D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT | D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT |
        D3DPRASTERCAPS_ANTIALIASEDGES    | D3DPRASTERCAPS_MIPMAPLODBIAS        | D3DPRASTERCAPS_ZBIAS                |
        D3DPRASTERCAPS_ZBUFFERLESSHSR    | D3DPRASTERCAPS_FOGRANGE             | D3DPRASTERCAPS_ANISOTROPY           |
        D3DPRASTERCAPS_WBUFFER           | D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT | D3DPRASTERCAPS_WFOG           |
        D3DPRASTERCAPS_ZFOG);

    desc7->dpcLineCaps.dwZCmpCaps &= (
        D3DPCMPCAPS_NEVER                | D3DPCMPCAPS_LESS                    | D3DPCMPCAPS_EQUAL                   |
        D3DPCMPCAPS_LESSEQUAL            | D3DPCMPCAPS_GREATER                 | D3DPCMPCAPS_NOTEQUAL                |
        D3DPCMPCAPS_GREATEREQUAL         | D3DPCMPCAPS_ALWAYS);

    desc7->dpcLineCaps.dwSrcBlendCaps &= (
        D3DPBLENDCAPS_ZERO               | D3DPBLENDCAPS_ONE                   | D3DPBLENDCAPS_SRCCOLOR              |
        D3DPBLENDCAPS_INVSRCCOLOR        | D3DPBLENDCAPS_SRCALPHA              | D3DPBLENDCAPS_INVSRCALPHA           |
        D3DPBLENDCAPS_DESTALPHA          | D3DPBLENDCAPS_INVDESTALPHA          | D3DPBLENDCAPS_DESTCOLOR             |
        D3DPBLENDCAPS_INVDESTCOLOR       | D3DPBLENDCAPS_SRCALPHASAT           | D3DPBLENDCAPS_BOTHSRCALPHA          |
        D3DPBLENDCAPS_BOTHINVSRCALPHA);

    desc7->dpcLineCaps.dwDestBlendCaps &= (
        D3DPBLENDCAPS_ZERO               | D3DPBLENDCAPS_ONE                   | D3DPBLENDCAPS_SRCCOLOR              |
        D3DPBLENDCAPS_INVSRCCOLOR        | D3DPBLENDCAPS_SRCALPHA              | D3DPBLENDCAPS_INVSRCALPHA           |
        D3DPBLENDCAPS_DESTALPHA          | D3DPBLENDCAPS_INVDESTALPHA          | D3DPBLENDCAPS_DESTCOLOR             |
        D3DPBLENDCAPS_INVDESTCOLOR       | D3DPBLENDCAPS_SRCALPHASAT           | D3DPBLENDCAPS_BOTHSRCALPHA          |
        D3DPBLENDCAPS_BOTHINVSRCALPHA);

    desc7->dpcLineCaps.dwAlphaCmpCaps &= (
        D3DPCMPCAPS_NEVER                | D3DPCMPCAPS_LESS                    | D3DPCMPCAPS_EQUAL                   |
        D3DPCMPCAPS_LESSEQUAL            | D3DPCMPCAPS_GREATER                 | D3DPCMPCAPS_NOTEQUAL                |
        D3DPCMPCAPS_GREATEREQUAL         | D3DPCMPCAPS_ALWAYS);

    desc7->dpcLineCaps.dwShadeCaps &= (
        D3DPSHADECAPS_COLORFLATMONO      | D3DPSHADECAPS_COLORFLATRGB          | D3DPSHADECAPS_COLORGOURAUDMONO      |
        D3DPSHADECAPS_COLORGOURAUDRGB    | D3DPSHADECAPS_COLORPHONGMONO        | D3DPSHADECAPS_COLORPHONGRGB         |
        D3DPSHADECAPS_SPECULARFLATMONO   | D3DPSHADECAPS_SPECULARFLATRGB       | D3DPSHADECAPS_SPECULARGOURAUDMONO   |
        D3DPSHADECAPS_SPECULARGOURAUDRGB | D3DPSHADECAPS_SPECULARPHONGMONO     | D3DPSHADECAPS_SPECULARPHONGRGB      |
        D3DPSHADECAPS_ALPHAFLATBLEND     | D3DPSHADECAPS_ALPHAFLATSTIPPLED     | D3DPSHADECAPS_ALPHAGOURAUDBLEND     |
        D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED | D3DPSHADECAPS_ALPHAPHONGBLEND     | D3DPSHADECAPS_ALPHAPHONGSTIPPLED    |
        D3DPSHADECAPS_FOGFLAT            | D3DPSHADECAPS_FOGGOURAUD            | D3DPSHADECAPS_FOGPHONG);

    desc7->dpcLineCaps.dwTextureCaps &= (
        D3DPTEXTURECAPS_PERSPECTIVE      | D3DPTEXTURECAPS_POW2                | D3DPTEXTURECAPS_ALPHA               |
        D3DPTEXTURECAPS_TRANSPARENCY     | D3DPTEXTURECAPS_BORDER              | D3DPTEXTURECAPS_SQUAREONLY          |
        D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE | D3DPTEXTURECAPS_ALPHAPALETTE| D3DPTEXTURECAPS_NONPOW2CONDITIONAL  |
        D3DPTEXTURECAPS_PROJECTED        | D3DPTEXTURECAPS_CUBEMAP             | D3DPTEXTURECAPS_COLORKEYBLEND);

    desc7->dpcLineCaps.dwTextureFilterCaps &= (
        D3DPTFILTERCAPS_NEAREST          | D3DPTFILTERCAPS_LINEAR              | D3DPTFILTERCAPS_MIPNEAREST          |
        D3DPTFILTERCAPS_MIPLINEAR        | D3DPTFILTERCAPS_LINEARMIPNEAREST    | D3DPTFILTERCAPS_LINEARMIPLINEAR     |
        D3DPTFILTERCAPS_MINFPOINT        | D3DPTFILTERCAPS_MINFLINEAR          | D3DPTFILTERCAPS_MINFANISOTROPIC     |
        D3DPTFILTERCAPS_MIPFPOINT        | D3DPTFILTERCAPS_MIPFLINEAR          | D3DPTFILTERCAPS_MAGFPOINT           |
        D3DPTFILTERCAPS_MAGFLINEAR       | D3DPTFILTERCAPS_MAGFANISOTROPIC     | D3DPTFILTERCAPS_MAGFAFLATCUBIC      |
        D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC);

    desc7->dpcLineCaps.dwTextureAddressCaps &= (
        D3DPTADDRESSCAPS_WRAP            | D3DPTADDRESSCAPS_MIRROR             | D3DPTADDRESSCAPS_CLAMP              |
        D3DPTADDRESSCAPS_BORDER          | D3DPTADDRESSCAPS_INDEPENDENTUV);

    if (!(desc7->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2))
    {
        /* DirectX7 always has the np2 flag set, no matter what the card
         * supports. Some old games (Rollcage) check the caps incorrectly.
         * If wined3d supports nonpow2 textures it also has np2 conditional
         * support. */
        desc7->dpcLineCaps.dwTextureCaps |= D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL;
    }

    /* Fill the missing members, and do some fixup */
    desc7->dpcLineCaps.dwSize = sizeof(desc7->dpcLineCaps);
    desc7->dpcLineCaps.dwTextureBlendCaps = D3DPTBLENDCAPS_ADD | D3DPTBLENDCAPS_MODULATEMASK |
                                            D3DPTBLENDCAPS_COPY | D3DPTBLENDCAPS_DECAL |
                                            D3DPTBLENDCAPS_DECALALPHA | D3DPTBLENDCAPS_DECALMASK |
                                            D3DPTBLENDCAPS_MODULATE | D3DPTBLENDCAPS_MODULATEALPHA;
    desc7->dpcLineCaps.dwStippleWidth = 32;
    desc7->dpcLineCaps.dwStippleHeight = 32;
    /* Use the same for the TriCaps */
    desc7->dpcTriCaps = desc7->dpcLineCaps;

    desc7->dwDeviceRenderBitDepth = DDBD_16 | DDBD_24 | DDBD_32;
    desc7->dwDeviceZBufferBitDepth = DDBD_16 | DDBD_24;
    desc7->dwMinTextureWidth = 1;
    desc7->dwMinTextureHeight = 1;

    /* Convert DWORDs safely to WORDs */
    if (wined3d_caps.MaxTextureBlendStages > 0xffff) desc7->wMaxTextureBlendStages = 0xffff;
    else desc7->wMaxTextureBlendStages = (WORD)wined3d_caps.MaxTextureBlendStages;
    if (wined3d_caps.MaxSimultaneousTextures > 0xffff) desc7->wMaxSimultaneousTextures = 0xffff;
    else desc7->wMaxSimultaneousTextures = (WORD)wined3d_caps.MaxSimultaneousTextures;

    if (wined3d_caps.MaxUserClipPlanes > 0xffff) desc7->wMaxUserClipPlanes = 0xffff;
    else desc7->wMaxUserClipPlanes = (WORD)wined3d_caps.MaxUserClipPlanes;
    if (wined3d_caps.MaxVertexBlendMatrices > 0xffff) desc7->wMaxVertexBlendMatrices = 0xffff;
    else desc7->wMaxVertexBlendMatrices = (WORD)wined3d_caps.MaxVertexBlendMatrices;

    desc7->deviceGUID = IID_IDirect3DTnLHalDevice;

    desc7->dwReserved1 = 0;
    desc7->dwReserved2 = 0;
    desc7->dwReserved3 = 0;
    desc7->dwReserved4 = 0;

    /* Fill the old structure */
    memset(desc1, 0, sizeof(*desc1));
    desc1->dwSize = sizeof(D3DDEVICEDESC);
    desc1->dwFlags = D3DDD_COLORMODEL
            | D3DDD_DEVCAPS
            | D3DDD_TRANSFORMCAPS
            | D3DDD_BCLIPPING
            | D3DDD_LIGHTINGCAPS
            | D3DDD_LINECAPS
            | D3DDD_TRICAPS
            | D3DDD_DEVICERENDERBITDEPTH
            | D3DDD_DEVICEZBUFFERBITDEPTH
            | D3DDD_MAXBUFFERSIZE
            | D3DDD_MAXVERTEXCOUNT;

    desc1->dcmColorModel = D3DCOLOR_RGB;
    desc1->dwDevCaps = desc7->dwDevCaps;
    desc1->dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
    desc1->dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP;
    desc1->bClipping = TRUE;
    desc1->dlcLightingCaps.dwSize = sizeof(D3DLIGHTINGCAPS);
    desc1->dlcLightingCaps.dwCaps = D3DLIGHTCAPS_DIRECTIONAL
            | D3DLIGHTCAPS_PARALLELPOINT
            | D3DLIGHTCAPS_POINT
            | D3DLIGHTCAPS_SPOT;

    desc1->dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
    desc1->dlcLightingCaps.dwNumLights = desc7->dwMaxActiveLights;

    desc1->dpcLineCaps.dwSize = sizeof(D3DPRIMCAPS);
    desc1->dpcLineCaps.dwMiscCaps = desc7->dpcLineCaps.dwMiscCaps;
    desc1->dpcLineCaps.dwRasterCaps = desc7->dpcLineCaps.dwRasterCaps;
    desc1->dpcLineCaps.dwZCmpCaps = desc7->dpcLineCaps.dwZCmpCaps;
    desc1->dpcLineCaps.dwSrcBlendCaps = desc7->dpcLineCaps.dwSrcBlendCaps;
    desc1->dpcLineCaps.dwDestBlendCaps = desc7->dpcLineCaps.dwDestBlendCaps;
    desc1->dpcLineCaps.dwShadeCaps = desc7->dpcLineCaps.dwShadeCaps;
    desc1->dpcLineCaps.dwTextureCaps = desc7->dpcLineCaps.dwTextureCaps;
    desc1->dpcLineCaps.dwTextureFilterCaps = desc7->dpcLineCaps.dwTextureFilterCaps;
    desc1->dpcLineCaps.dwTextureBlendCaps = desc7->dpcLineCaps.dwTextureBlendCaps;
    desc1->dpcLineCaps.dwTextureAddressCaps = desc7->dpcLineCaps.dwTextureAddressCaps;
    desc1->dpcLineCaps.dwStippleWidth = desc7->dpcLineCaps.dwStippleWidth;
    desc1->dpcLineCaps.dwAlphaCmpCaps = desc7->dpcLineCaps.dwAlphaCmpCaps;

    desc1->dpcTriCaps.dwSize = sizeof(D3DPRIMCAPS);
    desc1->dpcTriCaps.dwMiscCaps = desc7->dpcTriCaps.dwMiscCaps;
    desc1->dpcTriCaps.dwRasterCaps = desc7->dpcTriCaps.dwRasterCaps;
    desc1->dpcTriCaps.dwZCmpCaps = desc7->dpcTriCaps.dwZCmpCaps;
    desc1->dpcTriCaps.dwSrcBlendCaps = desc7->dpcTriCaps.dwSrcBlendCaps;
    desc1->dpcTriCaps.dwDestBlendCaps = desc7->dpcTriCaps.dwDestBlendCaps;
    desc1->dpcTriCaps.dwShadeCaps = desc7->dpcTriCaps.dwShadeCaps;
    desc1->dpcTriCaps.dwTextureCaps = desc7->dpcTriCaps.dwTextureCaps;
    desc1->dpcTriCaps.dwTextureFilterCaps = desc7->dpcTriCaps.dwTextureFilterCaps;
    desc1->dpcTriCaps.dwTextureBlendCaps = desc7->dpcTriCaps.dwTextureBlendCaps;
    desc1->dpcTriCaps.dwTextureAddressCaps = desc7->dpcTriCaps.dwTextureAddressCaps;
    desc1->dpcTriCaps.dwStippleWidth = desc7->dpcTriCaps.dwStippleWidth;
    desc1->dpcTriCaps.dwAlphaCmpCaps = desc7->dpcTriCaps.dwAlphaCmpCaps;

    desc1->dwDeviceRenderBitDepth = desc7->dwDeviceRenderBitDepth;
    desc1->dwDeviceZBufferBitDepth = desc7->dwDeviceZBufferBitDepth;
    desc1->dwMaxBufferSize = 0;
    desc1->dwMaxVertexCount = 65536;
    desc1->dwMinTextureWidth  = desc7->dwMinTextureWidth;
    desc1->dwMinTextureHeight = desc7->dwMinTextureHeight;
    desc1->dwMaxTextureWidth  = desc7->dwMaxTextureWidth;
    desc1->dwMaxTextureHeight = desc7->dwMaxTextureHeight;
    desc1->dwMinStippleWidth  = 1;
    desc1->dwMinStippleHeight = 1;
    desc1->dwMaxStippleWidth  = 32;
    desc1->dwMaxStippleHeight = 32;
    desc1->dwMaxTextureRepeat = desc7->dwMaxTextureRepeat;
    desc1->dwMaxTextureAspectRatio = desc7->dwMaxTextureAspectRatio;
    desc1->dwMaxAnisotropy = desc7->dwMaxAnisotropy;
    desc1->dvGuardBandLeft = desc7->dvGuardBandLeft;
    desc1->dvGuardBandRight = desc7->dvGuardBandRight;
    desc1->dvGuardBandTop = desc7->dvGuardBandTop;
    desc1->dvGuardBandBottom = desc7->dvGuardBandBottom;
    desc1->dvExtentsAdjust = desc7->dvExtentsAdjust;
    desc1->dwStencilCaps = desc7->dwStencilCaps;
    desc1->dwFVFCaps = desc7->dwFVFCaps;
    desc1->dwTextureOpCaps = desc7->dwTextureOpCaps;
    desc1->wMaxTextureBlendStages = desc7->wMaxTextureBlendStages;
    desc1->wMaxSimultaneousTextures = desc7->wMaxSimultaneousTextures;

    return DD_OK;
}

/*****************************************************************************
 * IDirectDraw7 VTable
 *****************************************************************************/
static const struct IDirectDraw7Vtbl ddraw7_vtbl =
{
    /* IUnknown */
    ddraw7_QueryInterface,
    ddraw7_AddRef,
    ddraw7_Release,
    /* IDirectDraw */
    ddraw7_Compact,
    ddraw7_CreateClipper,
    ddraw7_CreatePalette,
    ddraw7_CreateSurface,
    ddraw7_DuplicateSurface,
    ddraw7_EnumDisplayModes,
    ddraw7_EnumSurfaces,
    ddraw7_FlipToGDISurface,
    ddraw7_GetCaps,
    ddraw7_GetDisplayMode,
    ddraw7_GetFourCCCodes,
    ddraw7_GetGDISurface,
    ddraw7_GetMonitorFrequency,
    ddraw7_GetScanLine,
    ddraw7_GetVerticalBlankStatus,
    ddraw7_Initialize,
    ddraw7_RestoreDisplayMode,
    ddraw7_SetCooperativeLevel,
    ddraw7_SetDisplayMode,
    ddraw7_WaitForVerticalBlank,
    /* IDirectDraw2 */
    ddraw7_GetAvailableVidMem,
    /* IDirectDraw3 */
    ddraw7_GetSurfaceFromDC,
    /* IDirectDraw4 */
    ddraw7_RestoreAllSurfaces,
    ddraw7_TestCooperativeLevel,
    ddraw7_GetDeviceIdentifier,
    /* IDirectDraw7 */
    ddraw7_StartModeTest,
    ddraw7_EvaluateMode
};

static const struct IDirectDraw4Vtbl ddraw4_vtbl =
{
    /* IUnknown */
    ddraw4_QueryInterface,
    ddraw4_AddRef,
    ddraw4_Release,
    /* IDirectDraw */
    ddraw4_Compact,
    ddraw4_CreateClipper,
    ddraw4_CreatePalette,
    ddraw4_CreateSurface,
    ddraw4_DuplicateSurface,
    ddraw4_EnumDisplayModes,
    ddraw4_EnumSurfaces,
    ddraw4_FlipToGDISurface,
    ddraw4_GetCaps,
    ddraw4_GetDisplayMode,
    ddraw4_GetFourCCCodes,
    ddraw4_GetGDISurface,
    ddraw4_GetMonitorFrequency,
    ddraw4_GetScanLine,
    ddraw4_GetVerticalBlankStatus,
    ddraw4_Initialize,
    ddraw4_RestoreDisplayMode,
    ddraw4_SetCooperativeLevel,
    ddraw4_SetDisplayMode,
    ddraw4_WaitForVerticalBlank,
    /* IDirectDraw2 */
    ddraw4_GetAvailableVidMem,
    /* IDirectDraw3 */
    ddraw4_GetSurfaceFromDC,
    /* IDirectDraw4 */
    ddraw4_RestoreAllSurfaces,
    ddraw4_TestCooperativeLevel,
    ddraw4_GetDeviceIdentifier,
};

static const struct IDirectDraw2Vtbl ddraw2_vtbl =
{
    /* IUnknown */
    ddraw2_QueryInterface,
    ddraw2_AddRef,
    ddraw2_Release,
    /* IDirectDraw */
    ddraw2_Compact,
    ddraw2_CreateClipper,
    ddraw2_CreatePalette,
    ddraw2_CreateSurface,
    ddraw2_DuplicateSurface,
    ddraw2_EnumDisplayModes,
    ddraw2_EnumSurfaces,
    ddraw2_FlipToGDISurface,
    ddraw2_GetCaps,
    ddraw2_GetDisplayMode,
    ddraw2_GetFourCCCodes,
    ddraw2_GetGDISurface,
    ddraw2_GetMonitorFrequency,
    ddraw2_GetScanLine,
    ddraw2_GetVerticalBlankStatus,
    ddraw2_Initialize,
    ddraw2_RestoreDisplayMode,
    ddraw2_SetCooperativeLevel,
    ddraw2_SetDisplayMode,
    ddraw2_WaitForVerticalBlank,
    /* IDirectDraw2 */
    ddraw2_GetAvailableVidMem,
};

static const struct IDirectDrawVtbl ddraw1_vtbl =
{
    /* IUnknown */
    ddraw1_QueryInterface,
    ddraw1_AddRef,
    ddraw1_Release,
    /* IDirectDraw */
    ddraw1_Compact,
    ddraw1_CreateClipper,
    ddraw1_CreatePalette,
    ddraw1_CreateSurface,
    ddraw1_DuplicateSurface,
    ddraw1_EnumDisplayModes,
    ddraw1_EnumSurfaces,
    ddraw1_FlipToGDISurface,
    ddraw1_GetCaps,
    ddraw1_GetDisplayMode,
    ddraw1_GetFourCCCodes,
    ddraw1_GetGDISurface,
    ddraw1_GetMonitorFrequency,
    ddraw1_GetScanLine,
    ddraw1_GetVerticalBlankStatus,
    ddraw1_Initialize,
    ddraw1_RestoreDisplayMode,
    ddraw1_SetCooperativeLevel,
    ddraw1_SetDisplayMode,
    ddraw1_WaitForVerticalBlank,
};

static const struct IDirect3D7Vtbl d3d7_vtbl =
{
    /* IUnknown methods */
    d3d7_QueryInterface,
    d3d7_AddRef,
    d3d7_Release,
    /* IDirect3D7 methods */
    d3d7_EnumDevices,
    d3d7_CreateDevice,
    d3d7_CreateVertexBuffer,
    d3d7_EnumZBufferFormats,
    d3d7_EvictManagedTextures
};

static const struct IDirect3D3Vtbl d3d3_vtbl =
{
    /* IUnknown methods */
    d3d3_QueryInterface,
    d3d3_AddRef,
    d3d3_Release,
    /* IDirect3D3 methods */
    d3d3_EnumDevices,
    d3d3_CreateLight,
    d3d3_CreateMaterial,
    d3d3_CreateViewport,
    d3d3_FindDevice,
    d3d3_CreateDevice,
    d3d3_CreateVertexBuffer,
    d3d3_EnumZBufferFormats,
    d3d3_EvictManagedTextures
};

static const struct IDirect3D2Vtbl d3d2_vtbl =
{
    /* IUnknown methods */
    d3d2_QueryInterface,
    d3d2_AddRef,
    d3d2_Release,
    /* IDirect3D2 methods */
    d3d2_EnumDevices,
    d3d2_CreateLight,
    d3d2_CreateMaterial,
    d3d2_CreateViewport,
    d3d2_FindDevice,
    d3d2_CreateDevice
};

static const struct IDirect3DVtbl d3d1_vtbl =
{
    /* IUnknown methods */
    d3d1_QueryInterface,
    d3d1_AddRef,
    d3d1_Release,
    /* IDirect3D methods */
    d3d1_Initialize,
    d3d1_EnumDevices,
    d3d1_CreateLight,
    d3d1_CreateMaterial,
    d3d1_CreateViewport,
    d3d1_FindDevice
};

/*****************************************************************************
 * ddraw_find_decl
 *
 * Finds the WineD3D vertex declaration for a specific fvf, and creates one
 * if none was found.
 *
 * This function is in ddraw.c and the DDraw object space because D3D7
 * vertex buffers are created using the IDirect3D interface to the ddraw
 * object, so they can be valid across D3D devices(theoretically. The ddraw
 * object also owns the wined3d device
 *
 * Parameters:
 *  This: Device
 *  fvf: Fvf to find the decl for
 *
 * Returns:
 *  NULL in case of an error, the vertex declaration for the FVF otherwise.
 *
 *****************************************************************************/
struct wined3d_vertex_declaration *ddraw_find_decl(struct ddraw *This, DWORD fvf)
{
    struct wined3d_vertex_declaration *pDecl = NULL;
    HRESULT hr;
    int p, low, high; /* deliberately signed */
    struct FvfToDecl *convertedDecls = This->decls;

    TRACE("Searching for declaration for fvf %08x... ", fvf);

    low = 0;
    high = This->numConvertedDecls - 1;
    while(low <= high) {
        p = (low + high) >> 1;
        TRACE("%d ", p);
        if(convertedDecls[p].fvf == fvf) {
            TRACE("found %p\n", convertedDecls[p].decl);
            return convertedDecls[p].decl;
        } else if(convertedDecls[p].fvf < fvf) {
            low = p + 1;
        } else {
            high = p - 1;
        }
    }
    TRACE("not found. Creating and inserting at position %d.\n", low);

    hr = wined3d_vertex_declaration_create_from_fvf(This->wined3d_device,
            fvf, This, &ddraw_null_wined3d_parent_ops, &pDecl);
    if (hr != S_OK) return NULL;

    if(This->declArraySize == This->numConvertedDecls) {
        int grow = max(This->declArraySize / 2, 8);
        convertedDecls = HeapReAlloc(GetProcessHeap(), 0, convertedDecls,
                                     sizeof(convertedDecls[0]) * (This->numConvertedDecls + grow));
        if (!convertedDecls)
        {
            wined3d_vertex_declaration_decref(pDecl);
            return NULL;
        }
        This->decls = convertedDecls;
        This->declArraySize += grow;
    }

    memmove(convertedDecls + low + 1, convertedDecls + low, sizeof(convertedDecls[0]) * (This->numConvertedDecls - low));
    convertedDecls[low].decl = pDecl;
    convertedDecls[low].fvf = fvf;
    This->numConvertedDecls++;

    TRACE("Returning %p. %d decls in array\n", pDecl, This->numConvertedDecls);
    return pDecl;
}

static inline struct ddraw *ddraw_from_device_parent(struct wined3d_device_parent *device_parent)
{
    return CONTAINING_RECORD(device_parent, struct ddraw, device_parent);
}

static void CDECL device_parent_wined3d_device_created(struct wined3d_device_parent *device_parent,
        struct wined3d_device *device)
{
    TRACE("device_parent %p, device %p.\n", device_parent, device);
}

/* This is run from device_process_message() in wined3d, we can't take the
 * wined3d mutex. */
static void CDECL device_parent_mode_changed(struct wined3d_device_parent *device_parent)
{
    struct ddraw *ddraw = ddraw_from_device_parent(device_parent);
    MONITORINFO monitor_info;
    HMONITOR monitor;
    RECT *r;

    TRACE("device_parent %p.\n", device_parent);

    if (!(ddraw->cooperative_level & DDSCL_EXCLUSIVE) || !ddraw->swapchain_window)
    {
        TRACE("Nothing to resize.\n");
        return;
    }

    monitor = MonitorFromWindow(ddraw->swapchain_window, MONITOR_DEFAULTTOPRIMARY);
    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfoW(monitor, &monitor_info))
    {
        ERR("Failed to get monitor info.\n");
        return;
    }

    r = &monitor_info.rcMonitor;
    TRACE("Resizing window %p to %s.\n", ddraw->swapchain_window, wine_dbgstr_rect(r));

    if (!SetWindowPos(ddraw->swapchain_window, HWND_TOP, r->left, r->top,
                      r->right - r->left, r->bottom - r->top, SWP_SHOWWINDOW | SWP_NOACTIVATE))
        ERR("Failed to resize window.\n");
}

static HRESULT CDECL device_parent_create_texture_surface(struct wined3d_device_parent *device_parent,
        void *container_parent, UINT width, UINT height, enum wined3d_format_id format, DWORD usage,
        enum wined3d_pool pool, UINT sub_resource_idx, struct wined3d_surface **surface)
{
    struct ddraw *ddraw = ddraw_from_device_parent(device_parent);
    struct ddraw_surface *tex_root = container_parent;
    DDSURFACEDESC2 desc = tex_root->surface_desc;
    struct ddraw_surface *ddraw_surface;
    HRESULT hr;

    TRACE("device_parent %p, container_parent %p, width %u, height %u, format %#x, usage %#x,\n"
            "\tpool %#x, sub_resource_idx %u, surface %p.\n",
            device_parent, container_parent, width, height, format, usage, pool, sub_resource_idx, surface);

    /* The ddraw root surface is created before the wined3d texture. */
    if (!sub_resource_idx)
    {
        ddraw_surface = tex_root;
        goto done;
    }

    desc.dwWidth = width;
    desc.dwHeight = height;

    /* FIXME: Validate that format, usage, pool, etc. really make sense. */
    if (FAILED(hr = ddraw_create_surface(ddraw, &desc, &ddraw_surface, tex_root->version)))
        return hr;

done:
    *surface = ddraw_surface->wined3d_surface;
    wined3d_surface_incref(*surface);

    return DD_OK;
}

static void STDMETHODCALLTYPE ddraw_frontbuffer_destroyed(void *parent)
{
    struct ddraw *ddraw = parent;
    ddraw->wined3d_frontbuffer = NULL;
}

static const struct wined3d_parent_ops ddraw_frontbuffer_parent_ops =
{
    ddraw_frontbuffer_destroyed,
};

static HRESULT CDECL device_parent_create_swapchain_surface(struct wined3d_device_parent *device_parent,
        void *container_parent, UINT width, UINT height, enum wined3d_format_id format_id, DWORD usage,
        enum wined3d_multisample_type multisample_type, DWORD multisample_quality, struct wined3d_surface **surface)
{
    struct ddraw *ddraw = ddraw_from_device_parent(device_parent);
    HRESULT hr;

    TRACE("device_parent %p, container_parent %p, width %u, height %u, format_id %#x, usage %#x,\n"
            "\tmultisample_type %#x, multisample_quality %u, surface %p.\n",
            device_parent, container_parent, width, height, format_id, usage,
            multisample_type, multisample_quality, surface);

    if (ddraw->wined3d_frontbuffer)
    {
        ERR("Frontbuffer already created.\n");
        return E_FAIL;
    }

    if (SUCCEEDED(hr = wined3d_surface_create(ddraw->wined3d_device, width, height, format_id, usage,
            WINED3D_POOL_DEFAULT, multisample_type, multisample_quality, WINED3D_SURFACE_MAPPABLE,
            ddraw, &ddraw_frontbuffer_parent_ops, surface)))
        ddraw->wined3d_frontbuffer = *surface;

    return hr;
}

static HRESULT CDECL device_parent_create_volume(struct wined3d_device_parent *device_parent,
        void *container_parent, UINT width, UINT height, UINT depth, enum wined3d_format_id format,
        enum wined3d_pool pool, DWORD usage, struct wined3d_volume **volume)
{
    TRACE("device_parent %p, container_parent %p, width %u, height %u, depth %u, "
            "format %#x, pool %#x, usage %#x, volume %p.\n",
            device_parent, container_parent, width, height, depth,
            format, pool, usage, volume);

    ERR("Not implemented!\n");

    return E_NOTIMPL;
}

static HRESULT CDECL device_parent_create_swapchain(struct wined3d_device_parent *device_parent,
        struct wined3d_swapchain_desc *desc, struct wined3d_swapchain **swapchain)
{
    struct ddraw *ddraw = ddraw_from_device_parent(device_parent);
    HRESULT hr;

    TRACE("device_parent %p, desc %p, swapchain %p.\n", device_parent, desc, swapchain);

    if (ddraw->wined3d_swapchain)
    {
        ERR("Swapchain already created.\n");
        return E_FAIL;
    }

    if (FAILED(hr = wined3d_swapchain_create(ddraw->wined3d_device, desc, NULL,
            &ddraw_null_wined3d_parent_ops, swapchain)))
        WARN("Failed to create swapchain, hr %#x.\n", hr);

    return hr;
}

static const struct wined3d_device_parent_ops ddraw_wined3d_device_parent_ops =
{
    device_parent_wined3d_device_created,
    device_parent_mode_changed,
    device_parent_create_swapchain_surface,
    device_parent_create_texture_surface,
    device_parent_create_volume,
    device_parent_create_swapchain,
};

HRESULT ddraw_init(struct ddraw *ddraw, enum wined3d_device_type device_type)
{
    DWORD flags;
    HRESULT hr;

    ddraw->IDirectDraw7_iface.lpVtbl = &ddraw7_vtbl;
    ddraw->IDirectDraw_iface.lpVtbl = &ddraw1_vtbl;
    ddraw->IDirectDraw2_iface.lpVtbl = &ddraw2_vtbl;
    ddraw->IDirectDraw4_iface.lpVtbl = &ddraw4_vtbl;
    ddraw->IDirect3D_iface.lpVtbl = &d3d1_vtbl;
    ddraw->IDirect3D2_iface.lpVtbl = &d3d2_vtbl;
    ddraw->IDirect3D3_iface.lpVtbl = &d3d3_vtbl;
    ddraw->IDirect3D7_iface.lpVtbl = &d3d7_vtbl;
    ddraw->device_parent.ops = &ddraw_wined3d_device_parent_ops;
    ddraw->numIfaces = 1;
    ddraw->ref7 = 1;

    flags = WINED3D_LEGACY_DEPTH_BIAS;
    if (DefaultSurfaceType != DDRAW_SURFACE_TYPE_OPENGL)
        flags |= WINED3D_NO3D;

    if (!(ddraw->wined3d = wined3d_create(7, flags)))
    {
        if ((flags & WINED3D_NO3D) || !(ddraw->wined3d = wined3d_create(7, flags | WINED3D_NO3D)))
        {
            WARN("Failed to create a wined3d object.\n");
            return E_FAIL;
        }

        WARN("Created a wined3d object without 3D support.\n");
        DefaultSurfaceType = DDRAW_SURFACE_TYPE_GDI;
    }

    hr = wined3d_device_create(ddraw->wined3d, WINED3DADAPTER_DEFAULT, device_type,
            NULL, 0, 8, &ddraw->device_parent, &ddraw->wined3d_device);
    if (FAILED(hr))
    {
        WARN("Failed to create a wined3d device, hr %#x.\n", hr);
        wined3d_decref(ddraw->wined3d);
        return hr;
    }

    list_init(&ddraw->surface_list);

    return DD_OK;
}
