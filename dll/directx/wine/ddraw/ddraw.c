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

#include "config.h"
#include "wine/port.h"

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

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

static void STDMETHODCALLTYPE ddraw_null_wined3d_object_destroyed(void *parent) {}

const struct wined3d_parent_ops ddraw_null_wined3d_parent_ops =
{
    ddraw_null_wined3d_object_destroyed,
};

static inline IDirectDrawImpl *impl_from_IDirectDraw(IDirectDraw *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirectDraw_iface);
}

static inline IDirectDrawImpl *impl_from_IDirectDraw2(IDirectDraw2 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirectDraw2_iface);
}

static inline IDirectDrawImpl *impl_from_IDirectDraw3(IDirectDraw3 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirectDraw3_iface);
}

static inline IDirectDrawImpl *impl_from_IDirectDraw4(IDirectDraw4 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirectDraw4_iface);
}

static inline IDirectDrawImpl *impl_from_IDirectDraw7(IDirectDraw7 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirectDraw7_iface);
}

static inline IDirectDrawImpl *impl_from_IDirect3D(IDirect3D *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirect3D_iface);
}

static inline IDirectDrawImpl *impl_from_IDirect3D2(IDirect3D2 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirect3D2_iface);
}

static inline IDirectDrawImpl *impl_from_IDirect3D3(IDirect3D3 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirect3D3_iface);
}

static inline IDirectDrawImpl *impl_from_IDirect3D7(IDirect3D7 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirect3D7_iface);
}

/*****************************************************************************
 * IUnknown Methods
 *****************************************************************************/

/*****************************************************************************
 * IDirectDraw7::QueryInterface
 *
 * Queries different interfaces of the DirectDraw object. It can return
 * IDirectDraw interfaces in version 1, 2, 4 and 7, and IDirect3D interfaces
 * in version 1, 2, 3 and 7. An IDirect3DDevice can be created with this
 * method.
 * The returned interface is AddRef()-ed before it's returned
 *
 * Used for version 1, 2, 4 and 7
 *
 * Params:
 *  refiid: Interface ID asked for
 *  obj: Used to return the interface pointer
 *
 * Returns:
 *  S_OK if an interface was found
 *  E_NOINTERFACE if the requested interface wasn't found
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_QueryInterface(IDirectDraw7 *iface, REFIID refiid, void **obj)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(refiid), obj);

    /* Can change surface impl type */
    EnterCriticalSection(&ddraw_cs);

    /* According to COM docs, if the QueryInterface fails, obj should be set to NULL */
    *obj = NULL;

    if(!refiid)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* Check DirectDraw Interfaces */
    if ( IsEqualGUID( &IID_IUnknown, refiid ) ||
         IsEqualGUID( &IID_IDirectDraw7, refiid ) )
    {
        *obj = This;
        TRACE("(%p) Returning IDirectDraw7 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw4, refiid ) )
    {
        *obj = &This->IDirectDraw4_iface;
        TRACE("(%p) Returning IDirectDraw4 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw3, refiid ) )
    {
        /* This Interface exists in ddrawex.dll, it is implemented in a wrapper */
        WARN("IDirectDraw3 is not valid in ddraw.dll\n");
        *obj = NULL;
        LeaveCriticalSection(&ddraw_cs);
        return E_NOINTERFACE;
    }
    else if ( IsEqualGUID( &IID_IDirectDraw2, refiid ) )
    {
        *obj = &This->IDirectDraw2_iface;
        TRACE("(%p) Returning IDirectDraw2 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw, refiid ) )
    {
        *obj = &This->IDirectDraw_iface;
        TRACE("(%p) Returning IDirectDraw interface at %p\n", This, *obj);
    }

    /* Direct3D
     * The refcount unit test revealed that an IDirect3D7 interface can only be queried
     * from a DirectDraw object that was created as an IDirectDraw7 interface. No idea
     * who had this idea and why. The older interfaces can query and IDirect3D version
     * because they are all created as IDirectDraw(1). This isn't really crucial behavior,
     * and messy to implement with the common creation function, so it has been left out here.
     */
    else if ( IsEqualGUID( &IID_IDirect3D  , refiid ) ||
              IsEqualGUID( &IID_IDirect3D2 , refiid ) ||
              IsEqualGUID( &IID_IDirect3D3 , refiid ) ||
              IsEqualGUID( &IID_IDirect3D7 , refiid ) )
    {
        /* Check the surface implementation */
        if(This->ImplType == SURFACE_UNKNOWN)
        {
            /* Apps may create the IDirect3D Interface before the primary surface.
             * set the surface implementation */
            This->ImplType = SURFACE_OPENGL;
            TRACE("(%p) Choosing OpenGL surfaces because a Direct3D interface was requested\n", This);
        }
        else if(This->ImplType != SURFACE_OPENGL && DefaultSurfaceType == SURFACE_UNKNOWN)
        {
            ERR("(%p) The App is requesting a D3D device, but a non-OpenGL surface type was choosen. Prepare for trouble!\n", This);
            ERR(" (%p) You may want to contact wine-devel for help\n", This);
            /* Should I assert(0) here??? */
        }
        else if(This->ImplType != SURFACE_OPENGL)
        {
            WARN("The app requests a Direct3D interface, but non-opengl surfaces where set in winecfg\n");
            /* Do not abort here, only reject 3D Device creation */
        }

        if ( IsEqualGUID( &IID_IDirect3D  , refiid ) )
        {
            This->d3dversion = 1;
            *obj = &This->IDirect3D_iface;
            TRACE(" returning Direct3D interface at %p.\n", *obj);
        }
        else if ( IsEqualGUID( &IID_IDirect3D2  , refiid ) )
        {
            This->d3dversion = 2;
            *obj = &This->IDirect3D2_iface;
            TRACE(" returning Direct3D2 interface at %p.\n", *obj);
        }
        else if ( IsEqualGUID( &IID_IDirect3D3  , refiid ) )
        {
            This->d3dversion = 3;
            *obj = &This->IDirect3D3_iface;
            TRACE(" returning Direct3D3 interface at %p.\n", *obj);
        }
        else if(IsEqualGUID( &IID_IDirect3D7  , refiid ))
        {
            This->d3dversion = 7;
            *obj = &This->IDirect3D7_iface;
            TRACE(" returning Direct3D7 interface at %p.\n", *obj);
        }
    }
    /* Unknown interface */
    else
    {
        ERR("(%p)->(%s, %p): No interface found\n", This, debugstr_guid(refiid), obj);
        LeaveCriticalSection(&ddraw_cs);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown *) *obj );
    LeaveCriticalSection(&ddraw_cs);
    return S_OK;
}

static HRESULT WINAPI ddraw4_QueryInterface(IDirectDraw4 *iface, REFIID riid, void **object)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&This->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI ddraw3_QueryInterface(IDirectDraw3 *iface, REFIID riid, void **object)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&This->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI ddraw2_QueryInterface(IDirectDraw2 *iface, REFIID riid, void **object)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&This->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI ddraw1_QueryInterface(IDirectDraw *iface, REFIID riid, void **object)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&This->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI d3d7_QueryInterface(IDirect3D7 *iface, REFIID riid, void **object)
{
    IDirectDrawImpl *This = impl_from_IDirect3D7(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&This->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI d3d3_QueryInterface(IDirect3D3 *iface, REFIID riid, void **object)
{
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&This->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI d3d2_QueryInterface(IDirect3D2 *iface, REFIID riid, void **object)
{
    IDirectDrawImpl *This = impl_from_IDirect3D2(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&This->IDirectDraw7_iface, riid, object);
}

static HRESULT WINAPI d3d1_QueryInterface(IDirect3D *iface, REFIID riid, void **object)
{
    IDirectDrawImpl *This = impl_from_IDirect3D(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return ddraw7_QueryInterface(&This->IDirectDraw7_iface, riid, object);
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
 * IDirectDrawImpl object.
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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    ULONG ref = InterlockedIncrement(&This->ref7);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    if(ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI ddraw4_AddRef(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    ULONG ref = InterlockedIncrement(&This->ref4);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    if (ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI ddraw3_AddRef(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    ULONG ref = InterlockedIncrement(&This->ref3);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    if (ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI ddraw2_AddRef(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    ULONG ref = InterlockedIncrement(&This->ref2);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    if (ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI ddraw1_AddRef(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    ULONG ref = InterlockedIncrement(&This->ref1);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    if (ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI d3d7_AddRef(IDirect3D7 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirect3D7(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_AddRef(&This->IDirectDraw7_iface);
}

static ULONG WINAPI d3d3_AddRef(IDirect3D3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_AddRef(&This->IDirectDraw_iface);
}

static ULONG WINAPI d3d2_AddRef(IDirect3D2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirect3D2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_AddRef(&This->IDirectDraw_iface);
}

static ULONG WINAPI d3d1_AddRef(IDirect3D *iface)
{
    IDirectDrawImpl *This = impl_from_IDirect3D(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_AddRef(&This->IDirectDraw_iface);
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
static void ddraw_destroy(IDirectDrawImpl *This)
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

    EnterCriticalSection(&ddraw_cs);
    list_remove(&This->ddraw_list_entry);
    LeaveCriticalSection(&ddraw_cs);

    /* Release the attached WineD3D stuff */
    wined3d_device_decref(This->wined3d_device);
    wined3d_decref(This->wineD3D);

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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    ULONG ref = InterlockedDecrement(&This->ref7);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        ddraw_destroy(This);

    return ref;
}

static ULONG WINAPI ddraw4_Release(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    ULONG ref = InterlockedDecrement(&This->ref4);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        ddraw_destroy(This);

    return ref;
}

static ULONG WINAPI ddraw3_Release(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    ULONG ref = InterlockedDecrement(&This->ref3);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        ddraw_destroy(This);

    return ref;
}

static ULONG WINAPI ddraw2_Release(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    ULONG ref = InterlockedDecrement(&This->ref2);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        ddraw_destroy(This);

    return ref;
}

static ULONG WINAPI ddraw1_Release(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    ULONG ref = InterlockedDecrement(&This->ref1);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        ddraw_destroy(This);

    return ref;
}

static ULONG WINAPI d3d7_Release(IDirect3D7 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirect3D7(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_Release(&This->IDirectDraw7_iface);
}

static ULONG WINAPI d3d3_Release(IDirect3D3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_Release(&This->IDirectDraw_iface);
}

static ULONG WINAPI d3d2_Release(IDirect3D2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirect3D2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_Release(&This->IDirectDraw_iface);
}

static ULONG WINAPI d3d1_Release(IDirect3D *iface)
{
    IDirectDrawImpl *This = impl_from_IDirect3D(iface);

    TRACE("iface %p.\n", iface);

    return ddraw1_Release(&This->IDirectDraw_iface);
}

/*****************************************************************************
 * IDirectDraw methods
 *****************************************************************************/

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
static HRESULT WINAPI ddraw7_SetCooperativeLevel(IDirectDraw7 *iface, HWND hwnd, DWORD cooplevel)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    HWND window;

    TRACE("iface %p, window %p, flags %#x.\n", iface, hwnd, cooplevel);
    DDRAW_dump_cooperativelevel(cooplevel);

    EnterCriticalSection(&ddraw_cs);

    /* Get the old window */
    window = This->dest_window;

    /* Tests suggest that we need one of them: */
    if(!(cooplevel & (DDSCL_SETFOCUSWINDOW |
                      DDSCL_NORMAL         |
                      DDSCL_EXCLUSIVE      )))
    {
        TRACE("Incorrect cooplevel flags, returning DDERR_INVALIDPARAMS\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* Handle those levels first which set various hwnds */
    if(cooplevel & DDSCL_SETFOCUSWINDOW)
    {
        /* This isn't compatible with a lot of flags */
        if(cooplevel & ( DDSCL_MULTITHREADED      |
                         DDSCL_CREATEDEVICEWINDOW |
                         DDSCL_FPUSETUP           |
                         DDSCL_FPUPRESERVE        |
                         DDSCL_ALLOWREBOOT        |
                         DDSCL_ALLOWMODEX         |
                         DDSCL_SETDEVICEWINDOW    |
                         DDSCL_NORMAL             |
                         DDSCL_EXCLUSIVE          |
                         DDSCL_FULLSCREEN         ) )
        {
            TRACE("Called with incompatible flags, returning DDERR_INVALIDPARAMS\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }

        if( (This->cooperative_level & DDSCL_EXCLUSIVE) && window )
        {
            TRACE("Setting DDSCL_SETFOCUSWINDOW with an already set window, returning DDERR_HWNDALREADYSET\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_HWNDALREADYSET;
        }

        This->focuswindow = hwnd;
        /* Won't use the hwnd param for anything else */
        hwnd = NULL;

        /* Use the focus window for drawing too */
        This->dest_window = This->focuswindow;

        /* Destroy the device window, if we have one */
        if(This->devicewindow)
        {
            DestroyWindow(This->devicewindow);
            This->devicewindow = NULL;
        }

        LeaveCriticalSection(&ddraw_cs);
        return DD_OK;
    }

    if(cooplevel & DDSCL_EXCLUSIVE)
    {
        if( !(cooplevel & DDSCL_FULLSCREEN) || !hwnd )
        {
            TRACE("(%p) DDSCL_EXCLUSIVE needs DDSCL_FULLSCREEN and a window\n", This);
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }
    }
    else if( !(cooplevel & DDSCL_NORMAL) )
    {
        TRACE("(%p) SetCooperativeLevel needs at least SetFocusWindow or Exclusive or Normal mode\n", This);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    if ((This->cooperative_level & DDSCL_EXCLUSIVE)
            && (hwnd != window || !(cooplevel & DDSCL_EXCLUSIVE)))
        wined3d_device_release_focus_window(This->wined3d_device);

    if ((cooplevel & DDSCL_FULLSCREEN) != (This->cooperative_level & DDSCL_FULLSCREEN) || hwnd != window)
    {
        if (This->cooperative_level & DDSCL_FULLSCREEN)
            wined3d_device_restore_fullscreen_window(This->wined3d_device, window);

        if (cooplevel & DDSCL_FULLSCREEN)
        {
            WINED3DDISPLAYMODE display_mode;

            wined3d_get_adapter_display_mode(This->wineD3D, WINED3DADAPTER_DEFAULT, &display_mode);
            wined3d_device_setup_fullscreen_window(This->wined3d_device, hwnd,
                    display_mode.Width, display_mode.Height);
        }
    }

    if ((cooplevel & DDSCL_EXCLUSIVE)
            && (hwnd != window || !(This->cooperative_level & DDSCL_EXCLUSIVE)))
    {
        HRESULT hr = wined3d_device_acquire_focus_window(This->wined3d_device, hwnd);
        if (FAILED(hr))
        {
            ERR("Failed to acquire focus window, hr %#x.\n", hr);
            LeaveCriticalSection(&ddraw_cs);
            return hr;
        }
    }

    /* Don't override focus windows or private device windows */
    if (hwnd && !This->focuswindow && !This->devicewindow && (hwnd != window))
        This->dest_window = hwnd;

    if(cooplevel & DDSCL_CREATEDEVICEWINDOW)
    {
        /* Don't create a device window if a focus window is set */
        if( !(This->focuswindow) )
        {
            HWND devicewindow = CreateWindowExA(0, DDRAW_WINDOW_CLASS_NAME, "DDraw device window",
                    WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                    NULL, NULL, NULL, NULL);
            if (!devicewindow)
            {
                ERR("Failed to create window, last error %#x.\n", GetLastError());
                LeaveCriticalSection(&ddraw_cs);
                return E_FAIL;
            }

            ShowWindow(devicewindow, SW_SHOW);   /* Just to be sure */
            TRACE("(%p) Created a DDraw device window. HWND=%p\n", This, devicewindow);

            This->devicewindow = devicewindow;
            This->dest_window = devicewindow;
        }
    }

    if (cooplevel & DDSCL_MULTITHREADED && !(This->cooperative_level & DDSCL_MULTITHREADED))
        wined3d_device_set_multithreaded(This->wined3d_device);

    /* Unhandled flags */
    if(cooplevel & DDSCL_ALLOWREBOOT)
        WARN("(%p) Unhandled flag DDSCL_ALLOWREBOOT, harmless\n", This);
    if(cooplevel & DDSCL_ALLOWMODEX)
        WARN("(%p) Unhandled flag DDSCL_ALLOWMODEX, harmless\n", This);
    if(cooplevel & DDSCL_FPUSETUP)
        WARN("(%p) Unhandled flag DDSCL_FPUSETUP, harmless\n", This);

    /* Store the cooperative_level */
    This->cooperative_level = cooplevel;
    TRACE("SetCooperativeLevel retuning DD_OK\n");
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_SetCooperativeLevel(IDirectDraw4 *iface, HWND window, DWORD flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, window %p, flags %#x.\n", iface, window, flags);

    return ddraw7_SetCooperativeLevel(&This->IDirectDraw7_iface, window, flags);
}

static HRESULT WINAPI ddraw3_SetCooperativeLevel(IDirectDraw3 *iface, HWND window, DWORD flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, window %p, flags %#x.\n", iface, window, flags);

    return ddraw7_SetCooperativeLevel(&This->IDirectDraw7_iface, window, flags);
}

static HRESULT WINAPI ddraw2_SetCooperativeLevel(IDirectDraw2 *iface, HWND window, DWORD flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, window %p, flags %#x.\n", iface, window, flags);

    return ddraw7_SetCooperativeLevel(&This->IDirectDraw7_iface, window, flags);
}

static HRESULT WINAPI ddraw1_SetCooperativeLevel(IDirectDraw *iface, HWND window, DWORD flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, window %p, flags %#x.\n", iface, window, flags);

    return ddraw7_SetCooperativeLevel(&This->IDirectDraw7_iface, window, flags);
}

/*****************************************************************************
 *
 * Helper function for SetDisplayMode and RestoreDisplayMode
 *
 * Implements DirectDraw's SetDisplayMode, but ignores the value of
 * ForceRefreshRate, since it is already handled by
 * ddraw7_SetDisplayMode.  RestoreDisplayMode can use this function
 * without worrying that ForceRefreshRate will override the refresh rate.  For
 * argument and return value documentation, see
 * ddraw7_SetDisplayMode.
 *
 *****************************************************************************/
static HRESULT ddraw_set_display_mode(IDirectDrawImpl *ddraw, DWORD Width, DWORD Height,
        DWORD BPP, DWORD RefreshRate, DWORD Flags)
{
    enum wined3d_format_id format;
    WINED3DDISPLAYMODE Mode;
    HRESULT hr;

    TRACE("ddraw %p, width %u, height %u, bpp %u, refresh_rate %u, flags %#x.\n", ddraw, Width,
            Height, BPP, RefreshRate, Flags);

    EnterCriticalSection(&ddraw_cs);
    if( !Width || !Height )
    {
        ERR("Width %u, Height %u, what to do?\n", Width, Height);
        /* It looks like Need for Speed Porsche Unleashed expects DD_OK here */
        LeaveCriticalSection(&ddraw_cs);
        return DD_OK;
    }

    switch(BPP)
    {
        case 8:  format = WINED3DFMT_P8_UINT;          break;
        case 15: format = WINED3DFMT_B5G5R5X1_UNORM;   break;
        case 16: format = WINED3DFMT_B5G6R5_UNORM;     break;
        case 24: format = WINED3DFMT_B8G8R8_UNORM;     break;
        case 32: format = WINED3DFMT_B8G8R8X8_UNORM;   break;
        default: format = WINED3DFMT_UNKNOWN;          break;
    }

    if (FAILED(hr = wined3d_device_get_display_mode(ddraw->wined3d_device, 0, &Mode)))
    {
        ERR("Failed to get current display mode, hr %#x.\n", hr);
    }
    else if (Mode.Width == Width
            && Mode.Height == Height
            && Mode.Format == format
            && Mode.RefreshRate == RefreshRate)
    {
        TRACE("Skipping redundant mode setting call.\n");
        LeaveCriticalSection(&ddraw_cs);
        return DD_OK;
    }

    /* Check the exclusive mode
    if(!(ddraw->cooperative_level & DDSCL_EXCLUSIVE))
        return DDERR_NOEXCLUSIVEMODE;
     * This is WRONG. Don't know if the SDK is completely
     * wrong and if there are any conditions when DDERR_NOEXCLUSIVE
     * is returned, but Half-Life 1.1.1.1 (Steam version)
     * depends on this
     */

    Mode.Width = Width;
    Mode.Height = Height;
    Mode.RefreshRate = RefreshRate;
    Mode.Format = format;

    /* TODO: The possible return values from msdn suggest that
     * the screen mode can't be changed if a surface is locked
     * or some drawing is in progress
     */

    /* TODO: Lose the primary surface */
    hr = wined3d_device_set_display_mode(ddraw->wined3d_device, 0, &Mode);

    if (ddraw->cooperative_level & DDSCL_EXCLUSIVE)
    {
        wined3d_device_restore_fullscreen_window(ddraw->wined3d_device, ddraw->dest_window);
        wined3d_device_setup_fullscreen_window(ddraw->wined3d_device, ddraw->dest_window, Width, Height);
    }

    LeaveCriticalSection(&ddraw_cs);
    switch(hr)
    {
        case WINED3DERR_NOTAVAILABLE:       return DDERR_UNSUPPORTED;
        default:                            return hr;
    }
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
static HRESULT WINAPI ddraw7_SetDisplayMode(IDirectDraw7 *iface, DWORD Width, DWORD Height,
        DWORD BPP, DWORD RefreshRate, DWORD Flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);

    TRACE("iface %p, width %u, height %u, bpp %u, refresh_rate %u, flags %#x.\n",
            iface, Width, Height, BPP, RefreshRate, Flags);

    if (force_refresh_rate != 0)
    {
        TRACE("ForceRefreshRate overriding passed-in refresh rate (%u Hz) to %u Hz\n",
                RefreshRate, force_refresh_rate);
        RefreshRate = force_refresh_rate;
    }

    return ddraw_set_display_mode(This, Width, Height, BPP, RefreshRate, Flags);
}

static HRESULT WINAPI ddraw4_SetDisplayMode(IDirectDraw4 *iface, DWORD width, DWORD height,
        DWORD bpp, DWORD refresh_rate, DWORD flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, width %u, height %u, bpp %u, refresh_rate %u, flags %#x.\n",
            iface, width, height, bpp, refresh_rate, flags);

    return ddraw7_SetDisplayMode(&This->IDirectDraw7_iface, width, height, bpp, refresh_rate, flags);
}

static HRESULT WINAPI ddraw3_SetDisplayMode(IDirectDraw3 *iface, DWORD width, DWORD height,
        DWORD bpp, DWORD refresh_rate, DWORD flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, width %u, height %u, bpp %u, refresh_rate %u, flags %#x.\n",
            iface, width, height, bpp, refresh_rate, flags);

    return ddraw7_SetDisplayMode(&This->IDirectDraw7_iface, width, height, bpp, refresh_rate, flags);
}

static HRESULT WINAPI ddraw2_SetDisplayMode(IDirectDraw2 *iface,
        DWORD width, DWORD height, DWORD bpp, DWORD refresh_rate, DWORD flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, width %u, height %u, bpp %u, refresh_rate %u, flags %#x.\n",
            iface, width, height, bpp, refresh_rate, flags);

    return ddraw7_SetDisplayMode(&This->IDirectDraw7_iface, width, height, bpp, refresh_rate, flags);
}

static HRESULT WINAPI ddraw1_SetDisplayMode(IDirectDraw *iface, DWORD width, DWORD height, DWORD bpp)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, width %u, height %u, bpp %u.\n", iface, width, height, bpp);

    return ddraw7_SetDisplayMode(&This->IDirectDraw7_iface, width, height, bpp, 0, 0);
}

/*****************************************************************************
 * IDirectDraw7::RestoreDisplayMode
 *
 * Restores the display mode to what it was at creation time. Basically.
 *
 * A problem arises when there are 2 DirectDraw objects using the same hwnd:
 *  -> DD_1 finds the screen at 1400x1050x32 when created, sets it to 640x480x16
 *  -> DD_2 is created, finds the screen at 640x480x16, sets it to 1024x768x32
 *  -> DD_1 is released. The screen should be left at 1024x768x32.
 *  -> DD_2 is released. The screen should be set to 1400x1050x32
 * This case is unhandled right now, but Empire Earth does it this way.
 * (But perhaps there is something in SetCooperativeLevel to prevent this)
 *
 * The msdn says that this method resets the display mode to what it was before
 * SetDisplayMode was called. What if SetDisplayModes is called 2 times??
 *
 * Returns
 *  DD_OK on success
 *  DDERR_NOEXCLUSIVE mode if the device isn't in fullscreen mode
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_RestoreDisplayMode(IDirectDraw7 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);

    TRACE("iface %p.\n", iface);

    return ddraw_set_display_mode(This, This->orig_width, This->orig_height, This->orig_bpp, 0, 0);
}

static HRESULT WINAPI ddraw4_RestoreDisplayMode(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_RestoreDisplayMode(&This->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw3_RestoreDisplayMode(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_RestoreDisplayMode(&This->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw2_RestoreDisplayMode(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_RestoreDisplayMode(&This->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw1_RestoreDisplayMode(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_RestoreDisplayMode(&This->IDirectDraw7_iface);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    DDCAPS caps;
    WINED3DCAPS winecaps;
    HRESULT hr;
    DDSCAPS2 ddscaps = {0, 0, 0, {0}};

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, DriverCaps, HELCaps);

    /* One structure must be != NULL */
    if( (!DriverCaps) && (!HELCaps) )
    {
        ERR("(%p) Invalid params to ddraw7_GetCaps\n", This);
        return DDERR_INVALIDPARAMS;
    }

    memset(&caps, 0, sizeof(caps));
    memset(&winecaps, 0, sizeof(winecaps));
    caps.dwSize = sizeof(caps);
    EnterCriticalSection(&ddraw_cs);
    hr = wined3d_device_get_device_caps(This->wined3d_device, &winecaps);
    if (FAILED(hr))
    {
        WARN("IWineD3DDevice::GetDeviceCaps failed\n");
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    hr = IDirectDraw7_GetAvailableVidMem(iface, &ddscaps, &caps.dwVidMemTotal, &caps.dwVidMemFree);
    LeaveCriticalSection(&ddraw_cs);
    if(FAILED(hr)) {
        WARN("IDirectDraw7::GetAvailableVidMem failed\n");
        return hr;
    }

    caps.dwCaps = winecaps.DirectDrawCaps.Caps;
    caps.dwCaps2 = winecaps.DirectDrawCaps.Caps2;
    caps.dwCKeyCaps = winecaps.DirectDrawCaps.CKeyCaps;
    caps.dwFXCaps = winecaps.DirectDrawCaps.FXCaps;
    caps.dwPalCaps = winecaps.DirectDrawCaps.PalCaps;
    caps.ddsCaps.dwCaps = winecaps.DirectDrawCaps.ddsCaps;
    caps.dwSVBCaps = winecaps.DirectDrawCaps.SVBCaps;
    caps.dwSVBCKeyCaps = winecaps.DirectDrawCaps.SVBCKeyCaps;
    caps.dwSVBFXCaps = winecaps.DirectDrawCaps.SVBFXCaps;
    caps.dwVSBCaps = winecaps.DirectDrawCaps.VSBCaps;
    caps.dwVSBCKeyCaps = winecaps.DirectDrawCaps.VSBCKeyCaps;
    caps.dwVSBFXCaps = winecaps.DirectDrawCaps.VSBFXCaps;
    caps.dwSSBCaps = winecaps.DirectDrawCaps.SSBCaps;
    caps.dwSSBCKeyCaps = winecaps.DirectDrawCaps.SSBCKeyCaps;
    caps.dwSSBFXCaps = winecaps.DirectDrawCaps.SSBFXCaps;

    /* Even if WineD3D supports 3D rendering, remove the cap if ddraw is configured
     * not to use it
     */
    if(DefaultSurfaceType == SURFACE_GDI) {
        caps.dwCaps &= ~DDCAPS_3D;
        caps.ddsCaps.dwCaps &= ~(DDSCAPS_3DDEVICE | DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER);
    }
    if(winecaps.DirectDrawCaps.StrideAlign != 0) {
        caps.dwCaps |= DDCAPS_ALIGNSTRIDE;
        caps.dwAlignStrideAlign = winecaps.DirectDrawCaps.StrideAlign;
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
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddraw7_GetCaps(&This->IDirectDraw7_iface, driver_caps, hel_caps);
}

static HRESULT WINAPI ddraw3_GetCaps(IDirectDraw3 *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddraw7_GetCaps(&This->IDirectDraw7_iface, driver_caps, hel_caps);
}

static HRESULT WINAPI ddraw2_GetCaps(IDirectDraw2 *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddraw7_GetCaps(&This->IDirectDraw7_iface, driver_caps, hel_caps);
}

static HRESULT WINAPI ddraw1_GetCaps(IDirectDraw *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddraw7_GetCaps(&This->IDirectDraw7_iface, driver_caps, hel_caps);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_Compact(&This->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw3_Compact(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_Compact(&This->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw2_Compact(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_Compact(&This->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw1_Compact(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_Compact(&This->IDirectDraw7_iface);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    HRESULT hr;
    WINED3DDISPLAYMODE Mode;
    DWORD Size;

    TRACE("iface %p, surface_desc %p.\n", iface, DDSD);

    EnterCriticalSection(&ddraw_cs);
    /* This seems sane */
    if (!DDSD)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* The necessary members of LPDDSURFACEDESC and LPDDSURFACEDESC2 are equal,
     * so one method can be used for all versions (Hopefully) */
    hr = wined3d_device_get_display_mode(This->wined3d_device, 0, &Mode);
    if (FAILED(hr))
    {
        ERR(" (%p) IWineD3DDevice::GetDisplayMode returned %08x\n", This, hr);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    Size = DDSD->dwSize;
    memset(DDSD, 0, Size);

    DDSD->dwSize = Size;
    DDSD->dwFlags |= DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;
    DDSD->dwWidth = Mode.Width;
    DDSD->dwHeight = Mode.Height;
    DDSD->u2.dwRefreshRate = 60;
    DDSD->ddsCaps.dwCaps = 0;
    DDSD->u4.ddpfPixelFormat.dwSize = sizeof(DDSD->u4.ddpfPixelFormat);
    PixelFormat_WineD3DtoDD(&DDSD->u4.ddpfPixelFormat, Mode.Format);
    DDSD->u1.lPitch = Mode.Width * DDSD->u4.ddpfPixelFormat.u1.dwRGBBitCount / 8;

    if(TRACE_ON(ddraw))
    {
        TRACE("Returning surface desc :\n");
        DDRAW_dump_surface_desc(DDSD);
    }

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetDisplayMode(IDirectDraw4 *iface, DDSURFACEDESC2 *surface_desc)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, surface_desc);

    return ddraw7_GetDisplayMode(&This->IDirectDraw7_iface, surface_desc);
}

static HRESULT WINAPI ddraw3_GetDisplayMode(IDirectDraw3 *iface, DDSURFACEDESC *surface_desc)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, surface_desc);

    return ddraw7_GetDisplayMode(&This->IDirectDraw7_iface, (DDSURFACEDESC2 *)surface_desc);
}

static HRESULT WINAPI ddraw2_GetDisplayMode(IDirectDraw2 *iface, DDSURFACEDESC *surface_desc)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, surface_desc);

    return ddraw7_GetDisplayMode(&This->IDirectDraw7_iface, (DDSURFACEDESC2 *)surface_desc);
}

static HRESULT WINAPI ddraw1_GetDisplayMode(IDirectDraw *iface, DDSURFACEDESC *surface_desc)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, surface_desc %p.\n", iface, surface_desc);

    return ddraw7_GetDisplayMode(&This->IDirectDraw7_iface, (DDSURFACEDESC2 *)surface_desc);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    static const enum wined3d_format_id formats[] =
    {
        WINED3DFMT_YUY2, WINED3DFMT_UYVY, WINED3DFMT_YV12,
        WINED3DFMT_DXT1, WINED3DFMT_DXT2, WINED3DFMT_DXT3, WINED3DFMT_DXT4, WINED3DFMT_DXT5,
        WINED3DFMT_ATI2N, WINED3DFMT_NVHU, WINED3DFMT_NVHS
    };
    DWORD count = 0, i, outsize;
    HRESULT hr;
    WINED3DDISPLAYMODE d3ddm;
    WINED3DSURFTYPE type = This->ImplType;

    TRACE("iface %p, codes_count %p, codes %p.\n", iface, NumCodes, Codes);

    wined3d_device_get_display_mode(This->wined3d_device, 0, &d3ddm);

    outsize = NumCodes && Codes ? *NumCodes : 0;

    if(type == SURFACE_UNKNOWN) type = SURFACE_GDI;

    for (i = 0; i < (sizeof(formats) / sizeof(formats[0])); ++i)
    {
        hr = wined3d_check_device_format(This->wineD3D, WINED3DADAPTER_DEFAULT, WINED3DDEVTYPE_HAL,
                d3ddm.Format, 0, WINED3DRTYPE_SURFACE, formats[i], type);
        if (SUCCEEDED(hr))
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
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, codes_count %p, codes %p.\n", iface, codes_count, codes);

    return ddraw7_GetFourCCCodes(&This->IDirectDraw7_iface, codes_count, codes);
}

static HRESULT WINAPI ddraw3_GetFourCCCodes(IDirectDraw3 *iface, DWORD *codes_count, DWORD *codes)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, codes_count %p, codes %p.\n", iface, codes_count, codes);

    return ddraw7_GetFourCCCodes(&This->IDirectDraw7_iface, codes_count, codes);
}

static HRESULT WINAPI ddraw2_GetFourCCCodes(IDirectDraw2 *iface, DWORD *codes_count, DWORD *codes)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, codes_count %p, codes %p.\n", iface, codes_count, codes);

    return ddraw7_GetFourCCCodes(&This->IDirectDraw7_iface, codes_count, codes);
}

static HRESULT WINAPI ddraw1_GetFourCCCodes(IDirectDraw *iface, DWORD *codes_count, DWORD *codes)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, codes_count %p, codes %p.\n", iface, codes_count, codes);

    return ddraw7_GetFourCCCodes(&This->IDirectDraw7_iface, codes_count, codes);
}

/*****************************************************************************
 * IDirectDraw7::GetMonitorFrequency
 *
 * Returns the monitor's frequency
 *
 * Exists in Version 1, 2, 4 and 7
 *
 * Params:
 *  Freq: Pointer to a DWORD to write the frequency to
 *
 * Returns
 *  Always returns DD_OK
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetMonitorFrequency(IDirectDraw7 *iface, DWORD *Freq)
{
    FIXME("iface %p, frequency %p stub!\n", iface, Freq);

    /* Ideally this should be in WineD3D, as it concerns the screen setup,
     * but for now this should make the games happy
     */
    *Freq = 60;
    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetMonitorFrequency(IDirectDraw4 *iface, DWORD *frequency)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddraw7_GetMonitorFrequency(&This->IDirectDraw7_iface, frequency);
}

static HRESULT WINAPI ddraw3_GetMonitorFrequency(IDirectDraw3 *iface, DWORD *frequency)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddraw7_GetMonitorFrequency(&This->IDirectDraw7_iface, frequency);
}

static HRESULT WINAPI ddraw2_GetMonitorFrequency(IDirectDraw2 *iface, DWORD *frequency)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddraw7_GetMonitorFrequency(&This->IDirectDraw7_iface, frequency);
}

static HRESULT WINAPI ddraw1_GetMonitorFrequency(IDirectDraw *iface, DWORD *frequency)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddraw7_GetMonitorFrequency(&This->IDirectDraw7_iface, frequency);
}

/*****************************************************************************
 * IDirectDraw7::GetVerticalBlankStatus
 *
 * Returns the Vertical blank status of the monitor. This should be in WineD3D
 * too basically, but as it's a semi stub, I didn't create a function there
 *
 * Params:
 *  status: Pointer to a BOOL to be filled with the vertical blank status
 *
 * Returns
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if status is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetVerticalBlankStatus(IDirectDraw7 *iface, BOOL *status)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    /* This looks sane, the MSDN suggests it too */
    EnterCriticalSection(&ddraw_cs);
    if(!status)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    *status = This->fake_vblank;
    This->fake_vblank = !This->fake_vblank;
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetVerticalBlankStatus(IDirectDraw4 *iface, BOOL *status)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddraw7_GetVerticalBlankStatus(&This->IDirectDraw7_iface, status);
}

static HRESULT WINAPI ddraw3_GetVerticalBlankStatus(IDirectDraw3 *iface, BOOL *status)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddraw7_GetVerticalBlankStatus(&This->IDirectDraw7_iface, status);
}

static HRESULT WINAPI ddraw2_GetVerticalBlankStatus(IDirectDraw2 *iface, BOOL *status)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddraw7_GetVerticalBlankStatus(&This->IDirectDraw7_iface, status);
}

static HRESULT WINAPI ddraw1_GetVerticalBlankStatus(IDirectDraw *iface, BOOL *status)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddraw7_GetVerticalBlankStatus(&This->IDirectDraw7_iface, status);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, Caps, total, free);

    if(TRACE_ON(ddraw))
    {
        TRACE("(%p) Asked for memory with description: ", This);
        DDRAW_dump_DDSCAPS2(Caps);
    }
    EnterCriticalSection(&ddraw_cs);

    /* Todo: System memory vs local video memory vs non-local video memory
     * The MSDN also mentions differences between texture memory and other
     * resources, but that's not important
     */

    if( (!total) && (!free) )
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    if (total)
        *total = This->total_vidmem;
    if (free)
        *free = wined3d_device_get_available_texture_mem(This->wined3d_device);

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetAvailableVidMem(IDirectDraw4 *iface,
        DDSCAPS2 *caps, DWORD *total, DWORD *free)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, caps, total, free);

    return ddraw7_GetAvailableVidMem(&This->IDirectDraw7_iface, caps, total, free);
}

static HRESULT WINAPI ddraw3_GetAvailableVidMem(IDirectDraw3 *iface, DDSCAPS *caps, DWORD *total,
        DWORD *free)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    DDSCAPS2 caps2;

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, caps, total, free);

    DDRAW_Convert_DDSCAPS_1_To_2(caps, &caps2);
    return ddraw7_GetAvailableVidMem(&This->IDirectDraw7_iface, &caps2, total, free);
}

static HRESULT WINAPI ddraw2_GetAvailableVidMem(IDirectDraw2 *iface,
        DDSCAPS *caps, DWORD *total, DWORD *free)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    DDSCAPS2 caps2;

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, caps, total, free);

    DDRAW_Convert_DDSCAPS_1_To_2(caps, &caps2);
    return ddraw7_GetAvailableVidMem(&This->IDirectDraw7_iface, &caps2, total, free);
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
static HRESULT WINAPI ddraw7_Initialize(IDirectDraw7 *iface, GUID *Guid)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(Guid));

    if(This->initialized)
    {
        return DDERR_ALREADYINITIALIZED;
    }
    else
    {
        return DD_OK;
    }
}

static HRESULT WINAPI ddraw4_Initialize(IDirectDraw4 *iface, GUID *guid)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddraw7_Initialize(&This->IDirectDraw7_iface, guid);
}

static HRESULT WINAPI ddraw3_Initialize(IDirectDraw3 *iface, GUID *guid)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddraw7_Initialize(&This->IDirectDraw7_iface, guid);
}

static HRESULT WINAPI ddraw2_Initialize(IDirectDraw2 *iface, GUID *guid)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddraw7_Initialize(&This->IDirectDraw7_iface, guid);
}

static HRESULT WINAPI ddraw1_Initialize(IDirectDraw *iface, GUID *guid)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddraw7_Initialize(&This->IDirectDraw7_iface, guid);
}

static HRESULT WINAPI d3d1_Initialize(IDirect3D *iface, REFIID riid)
{
    TRACE("iface %p, riid %s.\n", iface, debugstr_guid(riid));

    return D3D_OK;
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
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_FlipToGDISurface(&This->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw3_FlipToGDISurface(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_FlipToGDISurface(&This->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw2_FlipToGDISurface(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_FlipToGDISurface(&This->IDirectDraw7_iface);
}

static HRESULT WINAPI ddraw1_FlipToGDISurface(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_FlipToGDISurface(&This->IDirectDraw7_iface);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#x, event %p.\n", iface, flags, event);

    return ddraw7_WaitForVerticalBlank(&This->IDirectDraw7_iface, flags, event);
}

static HRESULT WINAPI ddraw3_WaitForVerticalBlank(IDirectDraw3 *iface, DWORD flags, HANDLE event)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, flags %#x, event %p.\n", iface, flags, event);

    return ddraw7_WaitForVerticalBlank(&This->IDirectDraw7_iface, flags, event);
}

static HRESULT WINAPI ddraw2_WaitForVerticalBlank(IDirectDraw2 *iface, DWORD flags, HANDLE event)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, flags %#x, event %p.\n", iface, flags, event);

    return ddraw7_WaitForVerticalBlank(&This->IDirectDraw7_iface, flags, event);
}

static HRESULT WINAPI ddraw1_WaitForVerticalBlank(IDirectDraw *iface, DWORD flags, HANDLE event)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, flags %#x, event %p.\n", iface, flags, event);

    return ddraw7_WaitForVerticalBlank(&This->IDirectDraw7_iface, flags, event);
}

/*****************************************************************************
 * IDirectDraw7::GetScanLine
 *
 * Returns the scan line that is being drawn on the monitor
 *
 * Parameters:
 *  Scanline: Address to write the scan line value to
 *
 * Returns:
 *  Always returns DD_OK
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw7_GetScanLine(IDirectDraw7 *iface, DWORD *Scanline)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    static BOOL hide = FALSE;
    WINED3DDISPLAYMODE Mode;

    TRACE("iface %p, line %p.\n", iface, Scanline);

    /* This function is called often, so print the fixme only once */
    EnterCriticalSection(&ddraw_cs);
    if(!hide)
    {
        FIXME("iface %p, line %p partial stub!\n", iface, Scanline);
        hide = TRUE;
    }

    wined3d_device_get_display_mode(This->wined3d_device, 0, &Mode);

    /* Fake the line sweeping of the monitor */
    /* FIXME: We should synchronize with a source to keep the refresh rate */
    *Scanline = This->cur_scanline++;
    /* Assume 20 scan lines in the vertical blank */
    if (This->cur_scanline >= Mode.Height + 20)
        This->cur_scanline = 0;

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetScanLine(IDirectDraw4 *iface, DWORD *line)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddraw7_GetScanLine(&This->IDirectDraw7_iface, line);
}

static HRESULT WINAPI ddraw3_GetScanLine(IDirectDraw3 *iface, DWORD *line)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddraw7_GetScanLine(&This->IDirectDraw7_iface, line);
}

static HRESULT WINAPI ddraw2_GetScanLine(IDirectDraw2 *iface, DWORD *line)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddraw7_GetScanLine(&This->IDirectDraw7_iface, line);
}

static HRESULT WINAPI ddraw1_GetScanLine(IDirectDraw *iface, DWORD *line)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddraw7_GetScanLine(&This->IDirectDraw7_iface, line);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_TestCooperativeLevel(&This->IDirectDraw7_iface);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    struct wined3d_surface *wined3d_surface;
    IDirectDrawSurface7 *ddsurf;
    HRESULT hr;
    DDSCAPS2 ddsCaps;

    TRACE("iface %p, surface %p.\n", iface, GDISurface);

    /* Get the back buffer from the wineD3DDevice and search its
     * attached surfaces for the front buffer. */
    EnterCriticalSection(&ddraw_cs);
    hr = wined3d_device_get_back_buffer(This->wined3d_device,
            0, 0, WINED3DBACKBUFFER_TYPE_MONO, &wined3d_surface);
    if (FAILED(hr) || !wined3d_surface)
    {
        ERR("IWineD3DDevice::GetBackBuffer failed\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_NOTFOUND;
    }

    ddsurf = wined3d_surface_get_parent(wined3d_surface);
    wined3d_surface_decref(wined3d_surface);

    /* Find the front buffer */
    ddsCaps.dwCaps = DDSCAPS_FRONTBUFFER;
    hr = IDirectDrawSurface7_GetAttachedSurface(ddsurf,
                                                &ddsCaps,
                                                GDISurface);
    if(hr != DD_OK)
    {
        ERR("IDirectDrawSurface7::GetAttachedSurface failed, hr = %x\n", hr);
    }

    /* The AddRef is OK this time */
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI ddraw4_GetGDISurface(IDirectDraw4 *iface, IDirectDrawSurface4 **surface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, surface %p.\n", iface, surface);

    return ddraw7_GetGDISurface(&This->IDirectDraw7_iface, (IDirectDrawSurface7 **)surface);
}

static HRESULT WINAPI ddraw3_GetGDISurface(IDirectDraw3 *iface, IDirectDrawSurface **surface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    IDirectDrawSurface7 *surface7;
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, surface);

    hr = ddraw7_GetGDISurface(&This->IDirectDraw7_iface, &surface7);
    *surface = surface7 ? (IDirectDrawSurface *)&((IDirectDrawSurfaceImpl *)surface7)->IDirectDrawSurface3_vtbl : NULL;

    return hr;
}

static HRESULT WINAPI ddraw2_GetGDISurface(IDirectDraw2 *iface, IDirectDrawSurface **surface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    IDirectDrawSurface7 *surface7;
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, surface);

    hr = ddraw7_GetGDISurface(&This->IDirectDraw7_iface, &surface7);
    *surface = surface7 ? (IDirectDrawSurface *)&((IDirectDrawSurfaceImpl *)surface7)->IDirectDrawSurface3_vtbl : NULL;

    return hr;
}

static HRESULT WINAPI ddraw1_GetGDISurface(IDirectDraw *iface, IDirectDrawSurface **surface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    IDirectDrawSurface7 *surface7;
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, surface);

    hr = ddraw7_GetGDISurface(&This->IDirectDraw7_iface, &surface7);
    *surface = surface7 ? (IDirectDrawSurface *)&((IDirectDrawSurfaceImpl *)surface7)->IDirectDrawSurface3_vtbl : NULL;

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

    memcpy(&desc, surface_desc, sizeof(desc));
    desc.dwSize = sizeof(desc);

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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    unsigned int modenum, fmt;
    WINED3DDISPLAYMODE mode;
    DDSURFACEDESC2 callback_sd;
    WINED3DDISPLAYMODE *enum_modes = NULL;
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

    EnterCriticalSection(&ddraw_cs);
    /* This looks sane */
    if(!cb)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    if(!(Flags & DDEDM_REFRESHRATES))
    {
        enum_mode_array_size = 16;
        enum_modes = HeapAlloc(GetProcessHeap(), 0, sizeof(WINED3DDISPLAYMODE) * enum_mode_array_size);
        if (!enum_modes)
        {
            ERR("Out of memory\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_OUTOFMEMORY;
        }
    }

    pixelformat.dwSize = sizeof(pixelformat);
    for(fmt = 0; fmt < (sizeof(checkFormatList) / sizeof(checkFormatList[0])); fmt++)
    {
        modenum = 0;
        while (wined3d_enum_adapter_modes(This->wineD3D, WINED3DADAPTER_DEFAULT,
                checkFormatList[fmt], modenum++, &mode) == WINED3D_OK)
        {
            PixelFormat_WineD3DtoDD(&pixelformat, mode.Format);
            if(DDSD)
            {
                if(DDSD->dwFlags & DDSD_WIDTH && mode.Width != DDSD->dwWidth) continue;
                if(DDSD->dwFlags & DDSD_HEIGHT && mode.Height != DDSD->dwHeight) continue;
                if(DDSD->dwFlags & DDSD_REFRESHRATE && mode.RefreshRate != DDSD->u2.dwRefreshRate) continue;
                if (DDSD->dwFlags & DDSD_PIXELFORMAT &&
                        pixelformat.u1.dwRGBBitCount != DDSD->u4.ddpfPixelFormat.u1.dwRGBBitCount) continue;
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
                    if(enum_modes[i].Width == mode.Width && enum_modes[i].Height == mode.Height &&
                       enum_modes[i].Format == mode.Format)
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
            if(Flags & DDEDM_REFRESHRATES)
            {
                callback_sd.u2.dwRefreshRate = mode.RefreshRate;
            }

            callback_sd.dwWidth = mode.Width;
            callback_sd.dwHeight = mode.Height;

            callback_sd.u4.ddpfPixelFormat=pixelformat;

            /* Calc pitch and DWORD align like MSDN says */
            callback_sd.u1.lPitch = (callback_sd.u4.ddpfPixelFormat.u1.dwRGBBitCount / 8) * mode.Width;
            callback_sd.u1.lPitch = (callback_sd.u1.lPitch + 3) & ~3;

            TRACE("Enumerating %dx%dx%d @%d\n", callback_sd.dwWidth, callback_sd.dwHeight, callback_sd.u4.ddpfPixelFormat.u1.dwRGBBitCount,
              callback_sd.u2.dwRefreshRate);

            if(cb(&callback_sd, Context) == DDENUMRET_CANCEL)
            {
                TRACE("Application asked to terminate the enumeration\n");
                HeapFree(GetProcessHeap(), 0, enum_modes);
                LeaveCriticalSection(&ddraw_cs);
                return DD_OK;
            }

            if(!(Flags & DDEDM_REFRESHRATES))
            {
                if (enum_mode_count == enum_mode_array_size)
                {
                    WINED3DDISPLAYMODE *new_enum_modes;

                    enum_mode_array_size *= 2;
                    new_enum_modes = HeapReAlloc(GetProcessHeap(), 0, enum_modes, sizeof(WINED3DDISPLAYMODE) * enum_mode_array_size);

                    if (!new_enum_modes)
                    {
                        ERR("Out of memory\n");
                        HeapFree(GetProcessHeap(), 0, enum_modes);
                        LeaveCriticalSection(&ddraw_cs);
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
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_EnumDisplayModes(IDirectDraw4 *iface, DWORD flags,
        DDSURFACEDESC2 *surface_desc, void *context, LPDDENUMMODESCALLBACK2 callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    return ddraw7_EnumDisplayModes(&This->IDirectDraw7_iface, flags, surface_desc, context, callback);
}

static HRESULT WINAPI ddraw3_EnumDisplayModes(IDirectDraw3 *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMMODESCALLBACK callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    struct displaymodescallback_context cbcontext;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    return ddraw7_EnumDisplayModes(&This->IDirectDraw7_iface, flags, (DDSURFACEDESC2 *)surface_desc,
            &cbcontext, EnumDisplayModesCallbackThunk);
}

static HRESULT WINAPI ddraw2_EnumDisplayModes(IDirectDraw2 *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMMODESCALLBACK callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    struct displaymodescallback_context cbcontext;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    return ddraw7_EnumDisplayModes(&This->IDirectDraw7_iface, flags, (DDSURFACEDESC2 *)surface_desc,
            &cbcontext, EnumDisplayModesCallbackThunk);
}

static HRESULT WINAPI ddraw1_EnumDisplayModes(IDirectDraw *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMMODESCALLBACK callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    struct displaymodescallback_context cbcontext;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    return ddraw7_EnumDisplayModes(&This->IDirectDraw7_iface, flags, (DDSURFACEDESC2 *)surface_desc,
            &cbcontext, EnumDisplayModesCallbackThunk);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    DDDEVICEIDENTIFIER2 identifier2;
    HRESULT hr;

    TRACE("iface %p, identifier %p, flags %#x.\n", iface, identifier, flags);

    hr = ddraw7_GetDeviceIdentifier(&This->IDirectDraw7_iface, &identifier2, flags);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    struct wined3d_surface *wined3d_surface;
    HRESULT hr;

    TRACE("iface %p, dc %p, surface %p.\n", iface, hdc, Surface);

    if (!Surface) return E_INVALIDARG;

    hr = wined3d_device_get_surface_from_dc(This->wined3d_device, hdc, &wined3d_surface);
    if (FAILED(hr))
    {
        TRACE("No surface found for dc %p.\n", hdc);
        *Surface = NULL;
        return DDERR_NOTFOUND;
    }

    *Surface = wined3d_surface_get_parent(wined3d_surface);
    IDirectDrawSurface7_AddRef(*Surface);
    TRACE("Returning surface %p.\n", Surface);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_GetSurfaceFromDC(IDirectDraw4 *iface, HDC dc,
        IDirectDrawSurface4 **surface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    IDirectDrawSurface7 *surface7;
    HRESULT hr;

    TRACE("iface %p, dc %p, surface %p.\n", iface, dc, surface);

    if (!surface) return E_INVALIDARG;

    hr = ddraw7_GetSurfaceFromDC(&This->IDirectDraw7_iface, dc, &surface7);
    *surface = surface7 ? (IDirectDrawSurface4 *)&((IDirectDrawSurfaceImpl *)surface7)->IDirectDrawSurface3_vtbl : NULL;

    return hr;
}

static HRESULT WINAPI ddraw3_GetSurfaceFromDC(IDirectDraw3 *iface, HDC dc,
        IDirectDrawSurface **surface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, dc %p, surface %p.\n", iface, dc, surface);

    return ddraw7_GetSurfaceFromDC(&This->IDirectDraw7_iface, dc, (IDirectDrawSurface7 **)surface);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return ddraw7_RestoreAllSurfaces(&This->IDirectDraw7_iface);
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
 * ddraw_recreate_surfaces_cb
 *
 * Enumeration callback for ddraw_recreate_surface.
 * It re-recreates the WineD3DSurface. It's pretty straightforward
 *
 *****************************************************************************/
HRESULT WINAPI ddraw_recreate_surfaces_cb(IDirectDrawSurface7 *surf, DDSURFACEDESC2 *desc, void *Context)
{
    IDirectDrawSurfaceImpl *surfImpl = (IDirectDrawSurfaceImpl *)surf;
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;
    IDirectDrawImpl *This = surfImpl->ddraw;
    struct wined3d_surface *wined3d_surface;
    struct wined3d_swapchain *swapchain;
    struct wined3d_clipper *clipper;
    void *parent;
    HRESULT hr;

    TRACE("surface %p, surface_desc %p, context %p.\n",
            surf, desc, Context);

    /* For the enumeration */
    IDirectDrawSurface7_Release(surf);

    if(surfImpl->ImplType == This->ImplType) return DDENUMRET_OK; /* Continue */

    /* Get the objects */
    swapchain = surfImpl->wined3d_swapchain;
    surfImpl->wined3d_swapchain = NULL;
    wined3d_surface = surfImpl->wined3d_surface;

    /* get the clipper */
    clipper = wined3d_surface_get_clipper(wined3d_surface);

    /* Get the surface properties */
    wined3d_resource = wined3d_surface_get_resource(wined3d_surface);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);

    parent = wined3d_surface_get_parent(wined3d_surface);
    hr = wined3d_surface_create(This->wined3d_device, wined3d_desc.width, wined3d_desc.height,
            wined3d_desc.format, TRUE, FALSE, surfImpl->mipmap_level, wined3d_desc.usage, wined3d_desc.pool,
            wined3d_desc.multisample_type, wined3d_desc.multisample_quality, This->ImplType,
            parent, &ddraw_surface_wined3d_parent_ops, &surfImpl->wined3d_surface);
    if (FAILED(hr))
    {
        surfImpl->wined3d_surface = wined3d_surface;
        return hr;
    }

    wined3d_surface_set_clipper(surfImpl->wined3d_surface, clipper);

    /* TODO: Copy the surface content, except for render targets */

    /* If there's a swapchain, it owns the wined3d surfaces. So Destroy
     * the swapchain
     */
    if (swapchain)
    {
        /* The backbuffers have the swapchain set as well, but the primary
         * owns it and destroys it. */
        if (surfImpl->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            wined3d_device_uninit_gdi(This->wined3d_device);
        surfImpl->isRenderTarget = FALSE;
    }
    else
    {
        if (!wined3d_surface_decref(wined3d_surface))
            TRACE("Surface released successful, next surface\n");
        else
            ERR("Something's still holding the old WineD3DSurface\n");
    }

    surfImpl->ImplType = This->ImplType;

    return DDENUMRET_OK;
}

/*****************************************************************************
 * ddraw_recreate_surfaces
 *
 * A function, that converts all wineD3DSurfaces to the new implementation type
 * It enumerates all surfaces with IWineD3DDevice::EnumSurfaces, creates a
 * new WineD3DSurface, copies the content and releases the old surface
 *
 *****************************************************************************/
static HRESULT ddraw_recreate_surfaces(IDirectDrawImpl *This)
{
    DDSURFACEDESC2 desc;
    TRACE("(%p): Switch to implementation %d\n", This, This->ImplType);

    if(This->ImplType != SURFACE_OPENGL && This->d3d_initialized)
    {
        /* Should happen almost never */
        FIXME("(%p) Switching to non-opengl surfaces with d3d started. Is this a bug?\n", This);
        /* Shutdown d3d */
        wined3d_device_uninit_3d(This->wined3d_device);
    }
    /* Contrary: D3D starting is handled by the caller, because it knows the render target */

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    return IDirectDraw7_EnumSurfaces(&This->IDirectDraw7_iface, 0, &desc, This,
            ddraw_recreate_surfaces_cb);
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
static HRESULT ddraw_create_surface(IDirectDrawImpl *This, DDSURFACEDESC2 *pDDSD,
        IDirectDrawSurfaceImpl **ppSurf, UINT level)
{
    WINED3DSURFTYPE ImplType = This->ImplType;
    HRESULT hr;

    TRACE("ddraw %p, surface_desc %p, surface %p, level %u.\n",
            This, pDDSD, ppSurf, level);

    if (TRACE_ON(ddraw))
    {
        TRACE(" (%p) Requesting surface desc :\n", This);
        DDRAW_dump_surface_desc(pDDSD);
    }

    /* Select the surface type, if it wasn't choosen yet */
    if(ImplType == SURFACE_UNKNOWN)
    {
        /* Use GL Surfaces if a D3DDEVICE Surface is requested */
        if(pDDSD->ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
        {
            TRACE("(%p) Choosing GL surfaces because a 3DDEVICE Surface was requested\n", This);
            ImplType = SURFACE_OPENGL;
        }

        /* Otherwise use GDI surfaces for now */
        else
        {
            TRACE("(%p) Choosing GDI surfaces for 2D rendering\n", This);
            ImplType = SURFACE_GDI;
        }

        /* Policy if all surface implementations are available:
         * First, check if a default type was set with winecfg. If not,
         * try Xrender surfaces, and use them if they work. Next, check if
         * accelerated OpenGL is available, and use GL surfaces in this
         * case. If all else fails, use GDI surfaces. If a 3DDEVICE surface
         * was created, always use OpenGL surfaces.
         *
         * (Note: Xrender surfaces are not implemented for now, the
         * unaccelerated implementation uses GDI to render in Software)
         */

        /* Store the type. If it needs to be changed, all WineD3DSurfaces have to
         * be re-created. This could be done with IDirectDrawSurface7::Restore
         */
        This->ImplType = ImplType;
    }
    else
    {
        if ((pDDSD->ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
                && (This->ImplType != SURFACE_OPENGL)
                && DefaultSurfaceType == SURFACE_UNKNOWN)
        {
            /* We have to change to OpenGL,
             * and re-create all WineD3DSurfaces
             */
            ImplType = SURFACE_OPENGL;
            This->ImplType = ImplType;
            TRACE("(%p) Re-creating all surfaces\n", This);
            ddraw_recreate_surfaces(This);
            TRACE("(%p) Done recreating all surfaces\n", This);
        }
        else if(This->ImplType != SURFACE_OPENGL && pDDSD->ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
        {
            WARN("The application requests a 3D capable surface, but a non-opengl surface was set in the registry\n");
            /* Do not fail surface creation, only fail 3D device creation */
        }
    }

    /* Create the Surface object */
    *ppSurf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawSurfaceImpl));
    if(!*ppSurf)
    {
        ERR("(%p) Error allocating memory for a surface\n", This);
        return DDERR_OUTOFVIDEOMEMORY;
    }

    hr = ddraw_surface_init(*ppSurf, This, pDDSD, level, ImplType);
    if (FAILED(hr))
    {
        WARN("Failed to initialize surface, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, *ppSurf);
        return hr;
    }

    /* Increase the surface counter, and attach the surface */
    InterlockedIncrement(&This->surfaces);
    list_add_head(&This->surface_list, &(*ppSurf)->surface_list_entry);

    TRACE("Created surface %p.\n", *ppSurf);

    return DD_OK;
}
/*****************************************************************************
 * CreateAdditionalSurfaces
 *
 * Creates a new mipmap chain.
 *
 * Params:
 *  root: Root surface to attach the newly created chain to
 *  count: number of surfaces to create
 *  DDSD: Description of the surface. Intentionally not a pointer to avoid side
 *        effects on the caller
 *  CubeFaceRoot: Whether the new surface is a root of a cube map face. This
 *                creates an additional surface without the mipmapping flags
 *
 *****************************************************************************/
static HRESULT
CreateAdditionalSurfaces(IDirectDrawImpl *This,
                         IDirectDrawSurfaceImpl *root,
                         UINT count,
                         DDSURFACEDESC2 DDSD,
                         BOOL CubeFaceRoot)
{
    UINT i, j, level = 0;
    HRESULT hr;
    IDirectDrawSurfaceImpl *last = root;

    for(i = 0; i < count; i++)
    {
        IDirectDrawSurfaceImpl *object2 = NULL;

        /* increase the mipmap level, but only if a mipmap is created
         * In this case, also halve the size
         */
        if(DDSD.ddsCaps.dwCaps & DDSCAPS_MIPMAP && !CubeFaceRoot)
        {
            level++;
            if(DDSD.dwWidth > 1) DDSD.dwWidth /= 2;
            if(DDSD.dwHeight > 1) DDSD.dwHeight /= 2;
            /* Set the mipmap sublevel flag according to msdn */
            DDSD.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
        }
        else
        {
            DDSD.ddsCaps.dwCaps2 &= ~DDSCAPS2_MIPMAPSUBLEVEL;
        }
        CubeFaceRoot = FALSE;

        hr = ddraw_create_surface(This, &DDSD, &object2, level);
        if(hr != DD_OK)
        {
            return hr;
        }

        /* Add the new surface to the complex attachment array */
        for(j = 0; j < MAX_COMPLEX_ATTACHED; j++)
        {
            if(last->complex_array[j]) continue;
            last->complex_array[j] = object2;
            break;
        }
        last = object2;

        /* Remove the (possible) back buffer cap from the new surface description,
         * because only one surface in the flipping chain is a back buffer, one
         * is a front buffer, the others are just primary surfaces.
         */
        DDSD.ddsCaps.dwCaps &= ~DDSCAPS_BACKBUFFER;
    }
    return DD_OK;
}

/* Must set all attached surfaces (e.g. mipmaps) versions as well */
static void ddraw_set_surface_version(IDirectDrawSurfaceImpl *surface, UINT version)
{
    unsigned int i;

    TRACE("surface %p, version %u -> %u.\n", surface, surface->version, version);

    surface->version = version;
    for (i = 0; i < MAX_COMPLEX_ATTACHED; ++i)
    {
        if (!surface->complex_array[i]) break;
        ddraw_set_surface_version(surface->complex_array[i], version);
    }
    while ((surface = surface->next_attached))
    {
        ddraw_set_surface_version(surface, version);
    }
}

/*****************************************************************************
 * ddraw_attach_d3d_device
 *
 * Initializes the D3D capabilities of WineD3D
 *
 * Params:
 *  primary: The primary surface for D3D
 *
 * Returns
 *  DD_OK on success,
 *  DDERR_* otherwise
 *
 *****************************************************************************/
static HRESULT ddraw_attach_d3d_device(IDirectDrawImpl *ddraw, IDirectDrawSurfaceImpl *primary)
{
    WINED3DPRESENT_PARAMETERS localParameters;
    HWND window = ddraw->dest_window;
    HRESULT hr;

    TRACE("ddraw %p, primary %p.\n", ddraw, primary);

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
    }
    else
    {
        TRACE("Using existing window %p for Direct3D rendering.\n", window);
    }
    ddraw->d3d_window = window;

    /* Store the future Render Target surface */
    ddraw->d3d_target = primary;

    /* Use the surface description for the device parameters, not the device
     * settings. The application might render to an offscreen surface. */
    localParameters.BackBufferWidth = primary->surface_desc.dwWidth;
    localParameters.BackBufferHeight = primary->surface_desc.dwHeight;
    localParameters.BackBufferFormat = PixelFormat_DD2WineD3D(&primary->surface_desc.u4.ddpfPixelFormat);
    localParameters.BackBufferCount = (primary->surface_desc.dwFlags & DDSD_BACKBUFFERCOUNT)
            ? primary->surface_desc.u5.dwBackBufferCount : 0;
    localParameters.MultiSampleType = WINED3DMULTISAMPLE_NONE;
    localParameters.MultiSampleQuality = 0;
    localParameters.SwapEffect = WINED3DSWAPEFFECT_COPY;
    localParameters.hDeviceWindow = window;
    localParameters.Windowed = !(ddraw->cooperative_level & DDSCL_FULLSCREEN);
    localParameters.EnableAutoDepthStencil = TRUE;
    localParameters.AutoDepthStencilFormat = WINED3DFMT_D16_UNORM;
    localParameters.Flags = 0;
    localParameters.FullScreen_RefreshRateInHz = WINED3DPRESENT_RATE_DEFAULT;
    localParameters.PresentationInterval = WINED3DPRESENT_INTERVAL_DEFAULT;
    localParameters.AutoRestoreDisplayMode = FALSE;

    /* Set this NOW, otherwise creating the depth stencil surface will cause a
     * recursive loop until ram or emulated video memory is full. */
    ddraw->d3d_initialized = TRUE;
    hr = wined3d_device_init_3d(ddraw->wined3d_device, &localParameters);
    if (FAILED(hr))
    {
        ddraw->d3d_target = NULL;
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

static HRESULT ddraw_create_gdi_swapchain(IDirectDrawImpl *ddraw, IDirectDrawSurfaceImpl *primary)
{
    WINED3DPRESENT_PARAMETERS presentation_parameters;
    HWND window;
    HRESULT hr;

    window = ddraw->dest_window;

    memset(&presentation_parameters, 0, sizeof(presentation_parameters));

    /* Use the surface description for the device parameters, not the device
     * settings. The application might render to an offscreen surface. */
    presentation_parameters.BackBufferWidth = primary->surface_desc.dwWidth;
    presentation_parameters.BackBufferHeight = primary->surface_desc.dwHeight;
    presentation_parameters.BackBufferFormat = PixelFormat_DD2WineD3D(&primary->surface_desc.u4.ddpfPixelFormat);
    presentation_parameters.BackBufferCount = (primary->surface_desc.dwFlags & DDSD_BACKBUFFERCOUNT)
            ? primary->surface_desc.u5.dwBackBufferCount : 0;
    presentation_parameters.MultiSampleType = WINED3DMULTISAMPLE_NONE;
    presentation_parameters.MultiSampleQuality = 0;
    presentation_parameters.SwapEffect = WINED3DSWAPEFFECT_FLIP;
    presentation_parameters.hDeviceWindow = window;
    presentation_parameters.Windowed = !(ddraw->cooperative_level & DDSCL_FULLSCREEN);
    presentation_parameters.EnableAutoDepthStencil = FALSE; /* Not on GDI swapchains */
    presentation_parameters.AutoDepthStencilFormat = 0;
    presentation_parameters.Flags = 0;
    presentation_parameters.FullScreen_RefreshRateInHz = WINED3DPRESENT_RATE_DEFAULT;
    presentation_parameters.PresentationInterval = WINED3DPRESENT_INTERVAL_DEFAULT;
    presentation_parameters.AutoRestoreDisplayMode = FALSE;

    ddraw->d3d_target = primary;
    hr = wined3d_device_init_gdi(ddraw->wined3d_device, &presentation_parameters);
    ddraw->d3d_target = NULL;
    if (FAILED(hr))
    {
        WARN("Failed to initialize GDI ddraw implementation, hr %#x.\n", hr);
        primary->wined3d_swapchain = NULL;
    }

    return hr;
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
static HRESULT CreateSurface(IDirectDrawImpl *ddraw, DDSURFACEDESC2 *DDSD,
        IDirectDrawSurface7 **Surf, IUnknown *UnkOuter)
{
    IDirectDrawSurfaceImpl *object = NULL;
    HRESULT hr;
    LONG extra_surfaces = 0;
    DDSURFACEDESC2 desc2;
    WINED3DDISPLAYMODE Mode;
    const DWORD sysvidmem = DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;

    TRACE("ddraw %p, surface_desc %p, surface %p, outer_unknown %p.\n", ddraw, DDSD, Surf, UnkOuter);

    /* Some checks before we start */
    if (TRACE_ON(ddraw))
    {
        TRACE(" (%p) Requesting surface desc :\n", ddraw);
        DDRAW_dump_surface_desc(DDSD);
    }
    EnterCriticalSection(&ddraw_cs);

    if (UnkOuter != NULL)
    {
        FIXME("(%p) : outer != NULL?\n", ddraw);
        LeaveCriticalSection(&ddraw_cs);
        return CLASS_E_NOAGGREGATION; /* unchecked */
    }

    if (Surf == NULL)
    {
        FIXME("(%p) You want to get back a surface? Don't give NULL ptrs!\n", ddraw);
        LeaveCriticalSection(&ddraw_cs);
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
        *Surf = NULL;
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_NOEXCLUSIVEMODE;
    }

    if((DDSD->ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER | DDSCAPS_PRIMARYSURFACE)) == (DDSCAPS_BACKBUFFER | DDSCAPS_PRIMARYSURFACE))
    {
        WARN("Application wanted to create back buffer primary surface\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDCAPS;
    }

    if((DDSD->ddsCaps.dwCaps & sysvidmem) == sysvidmem)
    {
        /* This is a special switch in ddrawex.dll, but not allowed in ddraw.dll */
        WARN("Application tries to put the surface in both system and video memory\n");
        LeaveCriticalSection(&ddraw_cs);
        *Surf = NULL;
        return DDERR_INVALIDCAPS;
    }

    /* Check cube maps but only if the size includes them */
    if (DDSD->dwSize >= sizeof(DDSURFACEDESC2))
    {
        if(DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES &&
           !(DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP))
        {
            WARN("Cube map faces requested without cube map flag\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDCAPS;
        }
        if(DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP &&
           (DDSD->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES) == 0)
        {
            WARN("Cube map without faces requested\n");
            LeaveCriticalSection(&ddraw_cs);
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

    /* Get the video mode from WineD3D - we will need it */
    hr = wined3d_device_get_display_mode(ddraw->wined3d_device, 0, &Mode);
    if (FAILED(hr))
    {
        ERR("Failed to read display mode from wined3d\n");
        switch(ddraw->orig_bpp)
        {
            case 8:
                Mode.Format = WINED3DFMT_P8_UINT;
                break;

            case 15:
                Mode.Format = WINED3DFMT_B5G5R5X1_UNORM;
                break;

            case 16:
                Mode.Format = WINED3DFMT_B5G6R5_UNORM;
                break;

            case 24:
                Mode.Format = WINED3DFMT_B8G8R8_UNORM;
                break;

            case 32:
                Mode.Format = WINED3DFMT_B8G8R8X8_UNORM;
                break;
        }
        Mode.Width = ddraw->orig_width;
        Mode.Height = ddraw->orig_height;
    }

    /* No pixelformat given? Use the current screen format */
    if(!(desc2.dwFlags & DDSD_PIXELFORMAT))
    {
        desc2.dwFlags |= DDSD_PIXELFORMAT;
        desc2.u4.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);

        /* Wait: It could be a Z buffer */
        if(desc2.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        {
            switch(desc2.u2.dwMipMapCount) /* Who had this glorious idea? */
            {
                case 15:
                    PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, WINED3DFMT_S1_UINT_D15_UNORM);
                    break;
                case 16:
                    PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, WINED3DFMT_D16_UNORM);
                    break;
                case 24:
                    PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, WINED3DFMT_X8D24_UNORM);
                    break;
                case 32:
                    PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, WINED3DFMT_D32_UNORM);
                    break;
                default:
                    ERR("Unknown Z buffer bit depth\n");
            }
        }
        else
        {
            PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, Mode.Format);
        }
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
            *Surf = NULL;
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }

        desc2.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
        desc2.dwWidth = Mode.Width;
        desc2.dwHeight = Mode.Height;
    }

    if (!desc2.dwWidth || !desc2.dwHeight)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* Mipmap count fixes */
    if(desc2.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        if(desc2.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
        {
            if(desc2.dwFlags & DDSD_MIPMAPCOUNT)
            {
                /* Mipmap count is given, should not be 0 */
                if( desc2.u2.dwMipMapCount == 0 )
                {
                    LeaveCriticalSection(&ddraw_cs);
                    return DDERR_INVALIDPARAMS;
                }
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
        extra_surfaces = desc2.u2.dwMipMapCount - 1;

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

    /* Create the first surface */
    hr = ddraw_create_surface(ddraw, &desc2, &object, 0);
    if (FAILED(hr))
    {
        WARN("ddraw_create_surface failed, hr %#x.\n", hr);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }
    object->is_complex_root = TRUE;

    *Surf = (IDirectDrawSurface7 *)object;

    /* Create Additional surfaces if necessary
     * This applies to Primary surfaces which have a back buffer count
     * set, but not to mipmap textures. In case of Mipmap textures,
     * wineD3D takes care of the creation of additional surfaces
     */
    if(DDSD->dwFlags & DDSD_BACKBUFFERCOUNT)
    {
        extra_surfaces = DDSD->u5.dwBackBufferCount;
        desc2.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER; /* It's not a front buffer */
        desc2.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
        desc2.u5.dwBackBufferCount = 0;
    }

    hr = DD_OK;
    if(desc2.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
    {
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_ALLFACES;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_NEGATIVEZ;
        hr |= CreateAdditionalSurfaces(ddraw, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_NEGATIVEZ;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_POSITIVEZ;
        hr |= CreateAdditionalSurfaces(ddraw, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_POSITIVEZ;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_NEGATIVEY;
        hr |= CreateAdditionalSurfaces(ddraw, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_NEGATIVEY;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_POSITIVEY;
        hr |= CreateAdditionalSurfaces(ddraw, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_POSITIVEY;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_NEGATIVEX;
        hr |= CreateAdditionalSurfaces(ddraw, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_NEGATIVEX;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_POSITIVEX;
    }

    hr |= CreateAdditionalSurfaces(ddraw, object, extra_surfaces, desc2, FALSE);
    if(hr != DD_OK)
    {
        /* This destroys and possibly created surfaces too */
        IDirectDrawSurface_Release((IDirectDrawSurface7 *)object);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    /* If the implementation is OpenGL and there's no d3ddevice, attach a d3ddevice
     * But attach the d3ddevice only if the currently created surface was
     * a primary surface (2D app in 3D mode) or a 3DDEVICE surface (3D app)
     * The only case I can think of where this doesn't apply is when a
     * 2D app was configured by the user to run with OpenGL and it didn't create
     * the render target as first surface. In this case the render target creation
     * will cause the 3D init.
     */
    if( (ddraw->ImplType == SURFACE_OPENGL) && !(ddraw->d3d_initialized) &&
        desc2.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE) )
    {
        IDirectDrawSurfaceImpl *target = object, *surface;
        struct list *entry;

        /* Search for the primary to use as render target */
        LIST_FOR_EACH(entry, &ddraw->surface_list)
        {
            surface = LIST_ENTRY(entry, IDirectDrawSurfaceImpl, surface_list_entry);
            if((surface->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER)) ==
               (DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER))
            {
                /* found */
                target = surface;
                TRACE("Using primary %p as render target\n", target);
                break;
            }
        }

        TRACE("(%p) Attaching a D3DDevice, rendertarget = %p\n", ddraw, target);
        hr = ddraw_attach_d3d_device(ddraw, target);
        if (hr != D3D_OK)
        {
            IDirectDrawSurfaceImpl *release_surf;
            ERR("ddraw_attach_d3d_device failed, hr %#x\n", hr);
            *Surf = NULL;

            /* The before created surface structures are in an incomplete state here.
             * WineD3D holds the reference on the IParents, and it released them on the failure
             * already. So the regular release method implementation would fail on the attempt
             * to destroy either the IParents or the swapchain. So free the surface here.
             * The surface structure here is a list, not a tree, because onscreen targets
             * cannot be cube textures
             */
            while(object)
            {
                release_surf = object;
                object = object->complex_array[0];
                ddraw_surface_destroy(release_surf);
            }
            LeaveCriticalSection(&ddraw_cs);
            return hr;
        }
    }
    else if(!(ddraw->d3d_initialized) && desc2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        ddraw_create_gdi_swapchain(ddraw, object);
    }

    /* Addref the ddraw interface to keep an reference for each surface */
    IDirectDraw7_AddRef(&ddraw->IDirectDraw7_iface);
    object->ifaceToRelease = (IUnknown *)&ddraw->IDirectDraw7_iface;

    /* Create a WineD3DTexture if a texture was requested */
    if (desc2.ddsCaps.dwCaps & DDSCAPS_TEXTURE)
    {
        ddraw->tex_root = object;
        ddraw_surface_create_texture(object);
        ddraw->tex_root = NULL;
    }

    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI ddraw7_CreateSurface(IDirectDraw7 *iface, DDSURFACEDESC2 *surface_desc,
        IDirectDrawSurface7 **surface, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);

    TRACE("iface %p, surface_desc %p, surface %p, outer_unknown %p.\n",
            iface, surface_desc, surface, outer_unknown);

    if(surface_desc == NULL || surface_desc->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Application supplied invalid surface descriptor\n");
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
        return DDERR_INVALIDCAPS;
    }

    return CreateSurface(This, surface_desc, surface, outer_unknown);
}

static HRESULT WINAPI ddraw4_CreateSurface(IDirectDraw4 *iface,
        DDSURFACEDESC2 *surface_desc, IDirectDrawSurface4 **surface, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    IDirectDrawSurfaceImpl *impl;
    HRESULT hr;

    TRACE("iface %p, surface_desc %p, surface %p, outer_unknown %p.\n",
            iface, surface_desc, surface, outer_unknown);

    if(surface_desc == NULL || surface_desc->dwSize != sizeof(DDSURFACEDESC2))
    {
        WARN("Application supplied invalid surface descriptor\n");
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
        return DDERR_INVALIDCAPS;
    }

    hr = CreateSurface(This, surface_desc, (IDirectDrawSurface7 **)surface, outer_unknown);
    impl = (IDirectDrawSurfaceImpl *)*surface;
    if (SUCCEEDED(hr) && impl)
    {
        ddraw_set_surface_version(impl, 4);
        IDirectDraw7_Release(&This->IDirectDraw7_iface);
        IDirectDraw4_AddRef(iface);
        impl->ifaceToRelease = (IUnknown *)iface;
    }

    return hr;
}

static HRESULT WINAPI ddraw3_CreateSurface(IDirectDraw3 *iface, DDSURFACEDESC *surface_desc,
        IDirectDrawSurface **surface, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    IDirectDrawSurface7 *surface7;
    IDirectDrawSurfaceImpl *impl;
    HRESULT hr;

    TRACE("iface %p, surface_desc %p, surface %p, outer_unknown %p.\n",
            iface, surface_desc, surface, outer_unknown);

    if(surface_desc == NULL || surface_desc->dwSize != sizeof(DDSURFACEDESC))
    {
        WARN("Application supplied invalid surface descriptor\n");
        return DDERR_INVALIDPARAMS;
    }

    if(surface_desc->ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER))
    {
        if (TRACE_ON(ddraw))
        {
            TRACE(" (%p) Requesting surface desc :\n", iface);
            DDRAW_dump_surface_desc((LPDDSURFACEDESC2)surface_desc);
        }

        WARN("Application tried to create an explicit front or back buffer\n");
        return DDERR_INVALIDCAPS;
    }

    hr = CreateSurface(This, (DDSURFACEDESC2 *)surface_desc, &surface7, outer_unknown);
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }

    impl = (IDirectDrawSurfaceImpl *)surface7;
    *surface = (IDirectDrawSurface *)&impl->IDirectDrawSurface3_vtbl;
    ddraw_set_surface_version(impl, 3);
    IDirectDraw7_Release(&This->IDirectDraw7_iface);
    IDirectDraw3_AddRef(iface);
    impl->ifaceToRelease = (IUnknown *)iface;

    return hr;
}

static HRESULT WINAPI ddraw2_CreateSurface(IDirectDraw2 *iface,
        DDSURFACEDESC *surface_desc, IDirectDrawSurface **surface, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    IDirectDrawSurface7 *surface7;
    IDirectDrawSurfaceImpl *impl;
    HRESULT hr;

    TRACE("iface %p, surface_desc %p, surface %p, outer_unknown %p.\n",
            iface, surface_desc, surface, outer_unknown);

    if(surface_desc == NULL || surface_desc->dwSize != sizeof(DDSURFACEDESC))
    {
        WARN("Application supplied invalid surface descriptor\n");
        return DDERR_INVALIDPARAMS;
    }

    if(surface_desc->ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER))
    {
        if (TRACE_ON(ddraw))
        {
            TRACE(" (%p) Requesting surface desc :\n", iface);
            DDRAW_dump_surface_desc((LPDDSURFACEDESC2)surface_desc);
        }

        WARN("Application tried to create an explicit front or back buffer\n");
        return DDERR_INVALIDCAPS;
    }

    hr = CreateSurface(This, (DDSURFACEDESC2 *)surface_desc, &surface7, outer_unknown);
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }

    impl = (IDirectDrawSurfaceImpl *)surface7;
    *surface = (IDirectDrawSurface *)&impl->IDirectDrawSurface3_vtbl;
    ddraw_set_surface_version(impl, 2);
    IDirectDraw7_Release(&This->IDirectDraw7_iface);
    impl->ifaceToRelease = NULL;

    return hr;
}

static HRESULT WINAPI ddraw1_CreateSurface(IDirectDraw *iface,
        DDSURFACEDESC *surface_desc, IDirectDrawSurface **surface, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    IDirectDrawSurface7 *surface7;
    IDirectDrawSurfaceImpl *impl;
    HRESULT hr;

    TRACE("iface %p, surface_desc %p, surface %p, outer_unknown %p.\n",
            iface, surface_desc, surface, outer_unknown);

    if(surface_desc == NULL || surface_desc->dwSize != sizeof(DDSURFACEDESC))
    {
        WARN("Application supplied invalid surface descriptor\n");
        return DDERR_INVALIDPARAMS;
    }

    /* Remove front buffer flag, this causes failure in v7, and its added to normal
     * primaries anyway. */
    surface_desc->ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
    hr = CreateSurface(This, (DDSURFACEDESC2 *)surface_desc, &surface7, outer_unknown);
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }

    impl = (IDirectDrawSurfaceImpl *)surface7;
    *surface = (IDirectDrawSurface *)&impl->IDirectDrawSurface3_vtbl;
    ddraw_set_surface_version(impl, 1);
    IDirectDraw7_Release(&This->IDirectDraw7_iface);
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

struct surfacescallback_context
{
    LPDDENUMSURFACESCALLBACK func;
    void *context;
};

static HRESULT CALLBACK EnumSurfacesCallbackThunk(IDirectDrawSurface7 *surface,
        DDSURFACEDESC2 *surface_desc, void *context)
{
    struct surfacescallback_context *cbcontext = context;

    return cbcontext->func((IDirectDrawSurface *)&((IDirectDrawSurfaceImpl *)surface)->IDirectDrawSurface3_vtbl,
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
    /* The surface enumeration is handled by WineDDraw,
     * because it keeps track of all surfaces attached to
     * it. The filtering is done by our callback function,
     * because WineDDraw doesn't handle ddraw-like surface
     * caps structures
     */
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    IDirectDrawSurfaceImpl *surf;
    BOOL all, nomatch;
    DDSURFACEDESC2 desc;
    struct list *entry, *entry2;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, Flags, DDSD, Context, Callback);

    all = Flags & DDENUMSURFACES_ALL;
    nomatch = Flags & DDENUMSURFACES_NOMATCH;

    EnterCriticalSection(&ddraw_cs);

    if(!Callback)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* Use the _SAFE enumeration, the app may destroy enumerated surfaces */
    LIST_FOR_EACH_SAFE(entry, entry2, &This->surface_list)
    {
        surf = LIST_ENTRY(entry, IDirectDrawSurfaceImpl, surface_list_entry);
        if (all || (nomatch != ddraw_match_surface_desc(DDSD, &surf->surface_desc)))
        {
            desc = surf->surface_desc;
            IDirectDrawSurface7_AddRef((IDirectDrawSurface7 *)surf);
            if (Callback((IDirectDrawSurface7 *)surf, &desc, Context) != DDENUMRET_OK)
            {
                LeaveCriticalSection(&ddraw_cs);
                return DD_OK;
            }
        }
    }
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_EnumSurfaces(IDirectDraw4 *iface, DWORD flags,
        DDSURFACEDESC2 *surface_desc, void *context, LPDDENUMSURFACESCALLBACK2 callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    return ddraw7_EnumSurfaces(&This->IDirectDraw7_iface, flags, surface_desc, context,
            (LPDDENUMSURFACESCALLBACK7)callback);
}

static HRESULT WINAPI ddraw3_EnumSurfaces(IDirectDraw3 *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    struct surfacescallback_context cbcontext;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    return ddraw7_EnumSurfaces(&This->IDirectDraw7_iface, flags, (DDSURFACEDESC2 *)surface_desc,
            &cbcontext, EnumSurfacesCallbackThunk);
}

static HRESULT WINAPI ddraw2_EnumSurfaces(IDirectDraw2 *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    struct surfacescallback_context cbcontext;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    return ddraw7_EnumSurfaces(&This->IDirectDraw7_iface, flags, (DDSURFACEDESC2 *)surface_desc,
            &cbcontext, EnumSurfacesCallbackThunk);
}

static HRESULT WINAPI ddraw1_EnumSurfaces(IDirectDraw *iface, DWORD flags,
        DDSURFACEDESC *surface_desc, void *context, LPDDENUMSURFACESCALLBACK callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    struct surfacescallback_context cbcontext;

    TRACE("iface %p, flags %#x, surface_desc %p, context %p, callback %p.\n",
            iface, flags, surface_desc, context, callback);

    cbcontext.func = callback;
    cbcontext.context = context;

    return ddraw7_EnumSurfaces(&This->IDirectDraw7_iface, flags, (DDSURFACEDESC2 *)surface_desc,
            &cbcontext, EnumSurfacesCallbackThunk);
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
HRESULT WINAPI
DirectDrawCreateClipper(DWORD Flags,
                        LPDIRECTDRAWCLIPPER *Clipper,
                        IUnknown *UnkOuter)
{
    IDirectDrawClipperImpl* object;
    HRESULT hr;

    TRACE("flags %#x, clipper %p, outer_unknown %p.\n",
            Flags, Clipper, UnkOuter);

    EnterCriticalSection(&ddraw_cs);
    if (UnkOuter != NULL)
    {
        LeaveCriticalSection(&ddraw_cs);
        return CLASS_E_NOAGGREGATION;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                     sizeof(IDirectDrawClipperImpl));
    if (object == NULL)
    {
        LeaveCriticalSection(&ddraw_cs);
        return E_OUTOFMEMORY;
    }

    hr = ddraw_clipper_init(object);
    if (FAILED(hr))
    {
        WARN("Failed to initialize clipper, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    TRACE("Created clipper %p.\n", object);
    *Clipper = (IDirectDrawClipper *) object;
    LeaveCriticalSection(&ddraw_cs);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#x, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddraw7_CreateClipper(&This->IDirectDraw7_iface, flags, clipper, outer_unknown);
}

static HRESULT WINAPI ddraw3_CreateClipper(IDirectDraw3 *iface, DWORD flags,
        IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, flags %#x, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddraw7_CreateClipper(&This->IDirectDraw7_iface, flags, clipper, outer_unknown);
}

static HRESULT WINAPI ddraw2_CreateClipper(IDirectDraw2 *iface,
        DWORD flags, IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, flags %#x, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddraw7_CreateClipper(&This->IDirectDraw7_iface, flags, clipper, outer_unknown);
}

static HRESULT WINAPI ddraw1_CreateClipper(IDirectDraw *iface,
        DWORD flags, IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);

    TRACE("iface %p, flags %#x, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddraw7_CreateClipper(&This->IDirectDraw7_iface, flags, clipper, outer_unknown);
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
    IDirectDrawImpl *This = impl_from_IDirectDraw7(iface);
    IDirectDrawPaletteImpl *object;
    HRESULT hr;

    TRACE("iface %p, flags %#x, color_table %p, palette %p, outer_unknown %p.\n",
            iface, Flags, ColorTable, Palette, pUnkOuter);

    EnterCriticalSection(&ddraw_cs);
    if(pUnkOuter != NULL)
    {
        WARN("pUnkOuter is %p, returning CLASS_E_NOAGGREGATION\n", pUnkOuter);
        LeaveCriticalSection(&ddraw_cs);
        return CLASS_E_NOAGGREGATION;
    }

    /* The refcount test shows that a cooplevel is required for this */
    if(!This->cooperative_level)
    {
        WARN("No cooperative level set, returning DDERR_NOCOOPERATIVELEVELSET\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectDrawPaletteImpl));
    if(!object)
    {
        ERR("Out of memory when allocating memory for a palette implementation\n");
        LeaveCriticalSection(&ddraw_cs);
        return E_OUTOFMEMORY;
    }

    hr = ddraw_palette_init(object, This, Flags, ColorTable);
    if (FAILED(hr))
    {
        WARN("Failed to initialize palette, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    TRACE("Created palette %p.\n", object);
    *Palette = (IDirectDrawPalette *)object;
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI ddraw4_CreatePalette(IDirectDraw4 *iface, DWORD flags, PALETTEENTRY *entries,
        IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    hr = ddraw7_CreatePalette(&This->IDirectDraw7_iface, flags, entries, palette, outer_unknown);
    if (SUCCEEDED(hr) && *palette)
    {
        IDirectDrawPaletteImpl *impl = (IDirectDrawPaletteImpl *)*palette;
        IDirectDraw7_Release(&This->IDirectDraw7_iface);
        IDirectDraw4_AddRef(iface);
        impl->ifaceToRelease = (IUnknown *)iface;
    }
    return hr;
}

static HRESULT WINAPI ddraw3_CreatePalette(IDirectDraw3 *iface, DWORD flags,
        PALETTEENTRY *entries, IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    hr = ddraw7_CreatePalette(&This->IDirectDraw7_iface, flags, entries, palette, outer_unknown);
    if (SUCCEEDED(hr) && *palette)
    {
        IDirectDrawPaletteImpl *impl = (IDirectDrawPaletteImpl *)*palette;
        IDirectDraw7_Release(&This->IDirectDraw7_iface);
        IDirectDraw4_AddRef(iface);
        impl->ifaceToRelease = (IUnknown *)iface;
    }

    return hr;
}

static HRESULT WINAPI ddraw2_CreatePalette(IDirectDraw2 *iface, DWORD flags,
        PALETTEENTRY *entries, IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    hr = ddraw7_CreatePalette(&This->IDirectDraw7_iface, flags, entries, palette, outer_unknown);
    if (SUCCEEDED(hr) && *palette)
    {
        IDirectDrawPaletteImpl *impl = (IDirectDrawPaletteImpl *)*palette;
        IDirectDraw7_Release(&This->IDirectDraw7_iface);
        impl->ifaceToRelease = NULL;
    }

    return hr;
}

static HRESULT WINAPI ddraw1_CreatePalette(IDirectDraw *iface, DWORD flags,
        PALETTEENTRY *entries, IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    hr = ddraw7_CreatePalette(&This->IDirectDraw7_iface, flags, entries, palette, outer_unknown);
    if (SUCCEEDED(hr) && *palette)
    {
        IDirectDrawPaletteImpl *impl = (IDirectDrawPaletteImpl *)*palette;
        IDirectDraw7_Release(&This->IDirectDraw7_iface);
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
    IDirectDrawSurfaceImpl *Surf = (IDirectDrawSurfaceImpl *)Src;

    FIXME("iface %p, src %p, dst %p partial stub!\n", iface, Src, Dest);

    /* For now, simply create a new, independent surface */
    return IDirectDraw7_CreateSurface(iface,
                                      &Surf->surface_desc,
                                      Dest,
                                      NULL);
}

static HRESULT WINAPI ddraw4_DuplicateSurface(IDirectDraw4 *iface, IDirectDrawSurface4 *src,
        IDirectDrawSurface4 **dst)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);

    return ddraw7_DuplicateSurface(&This->IDirectDraw7_iface, (IDirectDrawSurface7 *)src,
            (IDirectDrawSurface7 **)dst);
}

static HRESULT WINAPI ddraw3_DuplicateSurface(IDirectDraw3 *iface, IDirectDrawSurface *src,
        IDirectDrawSurface **dst)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    IDirectDrawSurface7 *src7, *dst7;
    HRESULT hr;

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);
    src7 = (IDirectDrawSurface7 *)surface_from_surface3((IDirectDrawSurface3 *)src);
    hr = ddraw7_DuplicateSurface(&This->IDirectDraw7_iface, src7, &dst7);
    if (FAILED(hr))
        return hr;
    *dst = (IDirectDrawSurface *)&((IDirectDrawSurfaceImpl *)dst7)->IDirectDrawSurface3_vtbl;
    return hr;
}

static HRESULT WINAPI ddraw2_DuplicateSurface(IDirectDraw2 *iface,
        IDirectDrawSurface *src, IDirectDrawSurface **dst)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    IDirectDrawSurface7 *src7, *dst7;
    HRESULT hr;

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);
    src7 = (IDirectDrawSurface7 *)surface_from_surface3((IDirectDrawSurface3 *)src);
    hr = ddraw7_DuplicateSurface(&This->IDirectDraw7_iface, src7, &dst7);
    if (FAILED(hr))
        return hr;
    *dst = (IDirectDrawSurface *)&((IDirectDrawSurfaceImpl *)dst7)->IDirectDrawSurface3_vtbl;
    return hr;
}

static HRESULT WINAPI ddraw1_DuplicateSurface(IDirectDraw *iface, IDirectDrawSurface *src,
        IDirectDrawSurface **dst)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    IDirectDrawSurface7 *src7, *dst7;
    HRESULT hr;

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);
    src7 = (IDirectDrawSurface7 *)surface_from_surface3((IDirectDrawSurface3 *)src);
    hr = ddraw7_DuplicateSurface(&This->IDirectDraw7_iface, src7, &dst7);
    if (FAILED(hr))
        return hr;
    *dst = (IDirectDrawSurface *)&((IDirectDrawSurfaceImpl *)dst7)->IDirectDrawSurface3_vtbl;
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
    char interface_name_tnl[] = "WINE Direct3D7 Hardware Transform and Lighting acceleration using WineD3D";
    char device_name_tnl[] = "Wine D3D7 T&L HAL";
    char interface_name_hal[] = "WINE Direct3D7 Hardware acceleration using WineD3D";
    char device_name_hal[] = "Wine D3D7 HAL";
    char interface_name_rgb[] = "WINE Direct3D7 RGB Software Emulation using WineD3D";
    char device_name_rgb[] = "Wine D3D7 RGB";

    IDirectDrawImpl *This = impl_from_IDirect3D7(iface);
    D3DDEVICEDESC7 device_desc7;
    D3DDEVICEDESC device_desc1;
    HRESULT hr;

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    EnterCriticalSection(&ddraw_cs);

    hr = IDirect3DImpl_GetCaps(This->wineD3D, &device_desc1, &device_desc7);
    if (hr != D3D_OK)
    {
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }
    callback(interface_name_tnl, device_name_tnl, &device_desc7, context);

    device_desc7.deviceGUID = IID_IDirect3DHALDevice;
    callback(interface_name_hal, device_name_hal, &device_desc7, context);

    device_desc7.deviceGUID = IID_IDirect3DRGBDevice;
    callback(interface_name_rgb, device_name_rgb, &device_desc7, context);

    TRACE("End of enumeration.\n");

    LeaveCriticalSection(&ddraw_cs);

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

    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);
    D3DDEVICEDESC device_desc1, hal_desc, hel_desc;
    D3DDEVICEDESC7 device_desc7;
    HRESULT hr;

    /* Some games (Motoracer 2 demo) have the bad idea to modify the device
     * name string. Let's put the string in a sufficiently sized array in
     * writable memory. */
    char device_name[50];
    strcpy(device_name,"Direct3D HEL");

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    EnterCriticalSection(&ddraw_cs);

    hr = IDirect3DImpl_GetCaps(This->wineD3D, &device_desc1, &device_desc7);
    if (hr != D3D_OK)
    {
        LeaveCriticalSection(&ddraw_cs);
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
    if (This->d3dversion != 1)
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
        hr = callback((GUID *)&IID_IDirect3DRGBDevice, reference_description,
                device_name, &hal_desc, &hel_desc, context);
        if (hr != D3DENUMRET_OK)
        {
            TRACE("Application cancelled the enumeration.\n");
            LeaveCriticalSection(&ddraw_cs);
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
    hr = callback((GUID *)&IID_IDirect3DHALDevice, wined3d_description,
            device_name, &hal_desc, &hel_desc, context);
    if (hr != D3DENUMRET_OK)
    {
        TRACE("Application cancelled the enumeration.\n");
        LeaveCriticalSection(&ddraw_cs);
        return D3D_OK;
    }

    TRACE("End of enumeration.\n");

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI d3d2_EnumDevices(IDirect3D2 *iface, LPD3DENUMDEVICESCALLBACK callback, void *context)
{
    IDirectDrawImpl *This = impl_from_IDirect3D2(iface);

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    return d3d3_EnumDevices(&This->IDirect3D3_iface, callback, context);
}

static HRESULT WINAPI d3d1_EnumDevices(IDirect3D *iface, LPD3DENUMDEVICESCALLBACK callback, void *context)
{
    IDirectDrawImpl *This = impl_from_IDirect3D(iface);

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    return d3d3_EnumDevices(&This->IDirect3D3_iface, callback, context);
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
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);
    IDirect3DLightImpl *object;

    TRACE("iface %p, light %p, outer_unknown %p.\n", iface, light, outer_unknown);

    if (outer_unknown) return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate light memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    d3d_light_init(object, This);

    TRACE("Created light %p.\n", object);
    *light = (IDirect3DLight *)object;

    return D3D_OK;
}

static HRESULT WINAPI d3d2_CreateLight(IDirect3D2 *iface, IDirect3DLight **light, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirect3D2(iface);

    TRACE("iface %p, light %p, outer_unknown %p.\n", iface, light, outer_unknown);

    return d3d3_CreateLight(&This->IDirect3D3_iface, light, outer_unknown);
}

static HRESULT WINAPI d3d1_CreateLight(IDirect3D *iface, IDirect3DLight **light, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirect3D(iface);

    TRACE("iface %p, light %p, outer_unknown %p.\n", iface, light, outer_unknown);

    return d3d3_CreateLight(&This->IDirect3D3_iface, light, outer_unknown);
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
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);
    IDirect3DMaterialImpl *object;

    TRACE("iface %p, material %p, outer_unknown %p.\n", iface, material, outer_unknown);

    if (outer_unknown) return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate material memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    d3d_material_init(object, This);

    TRACE("Created material %p.\n", object);
    *material = (IDirect3DMaterial3 *)object;

    return D3D_OK;
}

static HRESULT WINAPI d3d2_CreateMaterial(IDirect3D2 *iface, IDirect3DMaterial2 **material, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirect3D2(iface);
    IDirect3DMaterial3 *material3;
    HRESULT hr;

    TRACE("iface %p, material %p, outer_unknown %p.\n", iface, material, outer_unknown);

    hr = d3d3_CreateMaterial(&This->IDirect3D3_iface, &material3, outer_unknown);
    *material = material3 ? (IDirect3DMaterial2 *)&((IDirect3DMaterialImpl *)material3)->IDirect3DMaterial2_vtbl : NULL;

    TRACE("Returning material %p.\n", *material);

    return hr;
}

static HRESULT WINAPI d3d1_CreateMaterial(IDirect3D *iface, IDirect3DMaterial **material, IUnknown *outer_unknown)
{
    IDirect3DMaterial3 *material3;
    HRESULT hr;

    IDirectDrawImpl *This = impl_from_IDirect3D(iface);

    TRACE("iface %p, material %p, outer_unknown %p.\n", iface, material, outer_unknown);

    hr = d3d3_CreateMaterial(&This->IDirect3D3_iface, &material3, outer_unknown);
    *material = material3 ? (IDirect3DMaterial *)&((IDirect3DMaterialImpl *)material3)->IDirect3DMaterial_vtbl : NULL;

    TRACE("Returning material %p.\n", *material);

    return hr;
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
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);
    IDirect3DViewportImpl *object;

    TRACE("iface %p, viewport %p, outer_unknown %p.\n", iface, viewport, outer_unknown);

    if (outer_unknown) return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate viewport memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    d3d_viewport_init(object, This);

    TRACE("Created viewport %p.\n", object);
    *viewport = (IDirect3DViewport3 *)object;

    return D3D_OK;
}

static HRESULT WINAPI d3d2_CreateViewport(IDirect3D2 *iface, IDirect3DViewport2 **viewport, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirect3D2(iface);

    TRACE("iface %p, viewport %p, outer_unknown %p.\n", iface, viewport, outer_unknown);

    return d3d3_CreateViewport(&This->IDirect3D3_iface, (IDirect3DViewport3 **)viewport,
            outer_unknown);
}

static HRESULT WINAPI d3d1_CreateViewport(IDirect3D *iface, IDirect3DViewport **viewport, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirect3D(iface);

    TRACE("iface %p, viewport %p, outer_unknown %p.\n", iface, viewport, outer_unknown);

    return d3d3_CreateViewport(&This->IDirect3D3_iface, (IDirect3DViewport3 **)viewport,
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
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);
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
    hr = IDirect3DImpl_GetCaps(This->wineD3D, &desc1, &desc7);
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
    IDirectDrawImpl *This = impl_from_IDirect3D2(iface);

    TRACE("iface %p, fds %p, fdr %p.\n", iface, fds, fdr);

    return d3d3_FindDevice(&This->IDirect3D3_iface, fds, fdr);
}

static HRESULT WINAPI d3d1_FindDevice(IDirect3D *iface, D3DFINDDEVICESEARCH *fds, D3DFINDDEVICERESULT *fdr)
{
    IDirectDrawImpl *This = impl_from_IDirect3D(iface);

    TRACE("iface %p, fds %p, fdr %p.\n", iface, fds, fdr);

    return d3d3_FindDevice(&This->IDirect3D3_iface, fds, fdr);
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
    IDirectDrawSurfaceImpl *target = (IDirectDrawSurfaceImpl *)surface;
    IDirectDrawImpl *This = impl_from_IDirect3D7(iface);
    IDirect3DDeviceImpl *object;
    HRESULT hr;

    TRACE("iface %p, riid %s, surface %p, device %p.\n", iface, debugstr_guid(riid), surface, device);

    EnterCriticalSection(&ddraw_cs);
    *device = NULL;

    /* Fail device creation if non-opengl surfaces are used. */
    if (This->ImplType != SURFACE_OPENGL)
    {
        ERR("The application wants to create a Direct3D device, but non-opengl surfaces are set in the registry.\n");
        ERR("Please set the surface implementation to opengl or autodetection to allow 3D rendering.\n");

        /* We only hit this path if a default surface is set in the registry. Incorrect autodetection
         * is caught in CreateSurface or QueryInterface. */
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_NO3D;
    }

    if (This->d3ddevice)
    {
        FIXME("Only one Direct3D device per DirectDraw object supported.\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate device memory.\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_OUTOFMEMORY;
    }

    hr = d3d_device_init(object, This, target);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    TRACE("Created device %p.\n", object);
    *device = (IDirect3DDevice7 *)object;

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI d3d3_CreateDevice(IDirect3D3 *iface, REFCLSID riid,
        IDirectDrawSurface4 *surface, IDirect3DDevice3 **device, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);
    HRESULT hr;

    TRACE("iface %p, riid %s, surface %p, device %p, outer_unknown %p.\n",
            iface, debugstr_guid(riid), surface, device, outer_unknown);

    if (outer_unknown) return CLASS_E_NOAGGREGATION;

    hr = d3d7_CreateDevice(&This->IDirect3D7_iface, riid, (IDirectDrawSurface7 *)surface,
            (IDirect3DDevice7 **)device);
    if (*device) *device = (IDirect3DDevice3 *)&((IDirect3DDeviceImpl *)*device)->IDirect3DDevice3_vtbl;

    return hr;
}

static HRESULT WINAPI d3d2_CreateDevice(IDirect3D2 *iface, REFCLSID riid,
        IDirectDrawSurface *surface, IDirect3DDevice2 **device)
{
    IDirectDrawImpl *This = impl_from_IDirect3D2(iface);
    HRESULT hr;

    TRACE("iface %p, riid %s, surface %p, device %p.\n",
            iface, debugstr_guid(riid), surface, device);

    hr = d3d7_CreateDevice(&This->IDirect3D7_iface, riid,
            surface ? (IDirectDrawSurface7 *)surface_from_surface3((IDirectDrawSurface3 *)surface) : NULL,
            (IDirect3DDevice7 **)device);
    if (*device) *device = (IDirect3DDevice2 *)&((IDirect3DDeviceImpl *)*device)->IDirect3DDevice2_vtbl;

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
    IDirectDrawImpl *This = impl_from_IDirect3D7(iface);
    IDirect3DVertexBufferImpl *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, vertex_buffer %p, flags %#x.\n",
            iface, desc, vertex_buffer, flags);

    if (!vertex_buffer || !desc) return DDERR_INVALIDPARAMS;

    TRACE("Vertex buffer description:\n");
    TRACE("    dwSize %u\n", desc->dwSize);
    TRACE("    dwCaps %#x\n", desc->dwCaps);
    TRACE("    FVF %#x\n", desc->dwFVF);
    TRACE("    dwNumVertices %u\n", desc->dwNumVertices);

    /* Now create the vertex buffer */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate vertex buffer memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    hr = d3d_vertex_buffer_init(object, This, desc);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex buffer, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created vertex buffer %p.\n", object);
    *vertex_buffer = (IDirect3DVertexBuffer7 *)object;

    return D3D_OK;
}

static HRESULT WINAPI d3d3_CreateVertexBuffer(IDirect3D3 *iface, D3DVERTEXBUFFERDESC *desc,
        IDirect3DVertexBuffer **vertex_buffer, DWORD flags, IUnknown *outer_unknown)
{
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);
    HRESULT hr;

    TRACE("iface %p, desc %p, vertex_buffer %p, flags %#x, outer_unknown %p.\n",
            iface, desc, vertex_buffer, flags, outer_unknown);

    if (outer_unknown) return CLASS_E_NOAGGREGATION;

    hr = d3d7_CreateVertexBuffer(&This->IDirect3D7_iface, desc,
            (IDirect3DVertexBuffer7 **)vertex_buffer, flags);
    if (*vertex_buffer)
        *vertex_buffer = (IDirect3DVertexBuffer *)&((IDirect3DVertexBufferImpl *)*vertex_buffer)->IDirect3DVertexBuffer_vtbl;

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
    IDirectDrawImpl *This = impl_from_IDirect3D7(iface);
    WINED3DDISPLAYMODE d3ddm;
    WINED3DDEVTYPE type;
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
        type = WINED3DDEVTYPE_HAL;
    }
    else if (IsEqualGUID(device_iid, &IID_IDirect3DRGBDevice)
            || IsEqualGUID(device_iid, &IID_IDirect3DMMXDevice))
    {
        TRACE("Asked for SW device.\n");
        type = WINED3DDEVTYPE_SW;
    }
    else if (IsEqualGUID(device_iid, &IID_IDirect3DRefDevice))
    {
        TRACE("Asked for REF device.\n");
        type = WINED3DDEVTYPE_REF;
    }
    else if (IsEqualGUID(device_iid, &IID_IDirect3DNullDevice))
    {
        TRACE("Asked for NULLREF device.\n");
        type = WINED3DDEVTYPE_NULLREF;
    }
    else
    {
        FIXME("Unexpected device GUID %s.\n", debugstr_guid(device_iid));
        type = WINED3DDEVTYPE_HAL;
    }

    EnterCriticalSection(&ddraw_cs);
    /* We need an adapter format from somewhere to please wined3d and WGL.
     * Use the current display mode. So far all cards offer the same depth
     * stencil format for all modes, but if some do not and applications do
     * not like that we'll have to find some workaround, like iterating over
     * all imaginable formats and collecting all the depth stencil formats we
     * can get. */
    hr = wined3d_device_get_display_mode(This->wined3d_device, 0, &d3ddm);

    for (i = 0; i < (sizeof(formats) / sizeof(*formats)); ++i)
    {
        hr = wined3d_check_device_format(This->wineD3D, WINED3DADAPTER_DEFAULT, type, d3ddm.Format,
                WINED3DUSAGE_DEPTHSTENCIL, WINED3DRTYPE_SURFACE, formats[i], SURFACE_OPENGL);
        if (SUCCEEDED(hr))
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
                LeaveCriticalSection(&ddraw_cs);
                return D3D_OK;
            }
        }
    }
    TRACE("End of enumeration.\n");

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI d3d3_EnumZBufferFormats(IDirect3D3 *iface, REFCLSID device_iid,
        LPD3DENUMPIXELFORMATSCALLBACK callback, void *context)
{
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);

    TRACE("iface %p, device_iid %s, callback %p, context %p.\n",
            iface, debugstr_guid(device_iid), callback, context);

    return d3d7_EnumZBufferFormats(&This->IDirect3D7_iface, device_iid, callback, context);
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
    FIXME("iface %p stub!\n", iface);

    /* TODO: Just enumerate resources using IWineD3DDevice_EnumResources(),
     * then unload surfaces / textures. */

    return D3D_OK;
}

static HRESULT WINAPI d3d3_EvictManagedTextures(IDirect3D3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirect3D3(iface);

    TRACE("iface %p.\n", iface);

    return d3d7_EvictManagedTextures(&This->IDirect3D7_iface);
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

    EnterCriticalSection(&ddraw_cs);
    hr = wined3d_get_device_caps(wined3d, 0, WINED3DDEVTYPE_HAL, &wined3d_caps);
    LeaveCriticalSection(&ddraw_cs);
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

    desc7->dpcLineCaps.dwTextureBlendCaps &= (
        D3DPTBLENDCAPS_DECAL             | D3DPTBLENDCAPS_MODULATE             | D3DPTBLENDCAPS_DECALALPHA           |
        D3DPTBLENDCAPS_MODULATEALPHA     | D3DPTBLENDCAPS_DECALMASK            | D3DPTBLENDCAPS_MODULATEMASK         |
        D3DPTBLENDCAPS_COPY              | D3DPTBLENDCAPS_ADD);

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

static const struct IDirectDraw3Vtbl ddraw3_vtbl =
{
    /* IUnknown */
    ddraw3_QueryInterface,
    ddraw3_AddRef,
    ddraw3_Release,
    /* IDirectDraw */
    ddraw3_Compact,
    ddraw3_CreateClipper,
    ddraw3_CreatePalette,
    ddraw3_CreateSurface,
    ddraw3_DuplicateSurface,
    ddraw3_EnumDisplayModes,
    ddraw3_EnumSurfaces,
    ddraw3_FlipToGDISurface,
    ddraw3_GetCaps,
    ddraw3_GetDisplayMode,
    ddraw3_GetFourCCCodes,
    ddraw3_GetGDISurface,
    ddraw3_GetMonitorFrequency,
    ddraw3_GetScanLine,
    ddraw3_GetVerticalBlankStatus,
    ddraw3_Initialize,
    ddraw3_RestoreDisplayMode,
    ddraw3_SetCooperativeLevel,
    ddraw3_SetDisplayMode,
    ddraw3_WaitForVerticalBlank,
    /* IDirectDraw2 */
    ddraw3_GetAvailableVidMem,
    /* IDirectDraw3 */
    ddraw3_GetSurfaceFromDC,
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
struct wined3d_vertex_declaration *ddraw_find_decl(IDirectDrawImpl *This, DWORD fvf)
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

static inline struct IDirectDrawImpl *ddraw_from_device_parent(struct wined3d_device_parent *device_parent)
{
    return CONTAINING_RECORD(device_parent, struct IDirectDrawImpl, device_parent);
}

static void CDECL device_parent_wined3d_device_created(struct wined3d_device_parent *device_parent,
        struct wined3d_device *device)
{
    TRACE("device_parent %p, device %p.\n", device_parent, device);
}

static HRESULT CDECL device_parent_create_surface(struct wined3d_device_parent *device_parent,
        void *container_parent, UINT width, UINT height, enum wined3d_format_id format, DWORD usage,
        WINED3DPOOL pool, UINT level, WINED3DCUBEMAP_FACES face, struct wined3d_surface **surface)
{
    struct IDirectDrawImpl *ddraw = ddraw_from_device_parent(device_parent);
    IDirectDrawSurfaceImpl *surf = NULL;
    UINT i = 0;
    DDSCAPS2 searchcaps = ddraw->tex_root->surface_desc.ddsCaps;

    TRACE("device_parent %p, container_parent %p, width %u, height %u, format %#x, usage %#x,\n"
            "\tpool %#x, level %u, face %u, surface %p.\n",
            device_parent, container_parent, width, height, format, usage, pool, level, face, surface);

    searchcaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_ALLFACES;
    switch(face)
    {
        case WINED3DCUBEMAP_FACE_POSITIVE_X:
            TRACE("Asked for positive x\n");
            if (searchcaps.dwCaps2 & DDSCAPS2_CUBEMAP)
            {
                searchcaps.dwCaps2 |= DDSCAPS2_CUBEMAP_POSITIVEX;
            }
            surf = ddraw->tex_root; break;
        case WINED3DCUBEMAP_FACE_NEGATIVE_X:
            TRACE("Asked for negative x\n");
            searchcaps.dwCaps2 |= DDSCAPS2_CUBEMAP_NEGATIVEX; break;
        case WINED3DCUBEMAP_FACE_POSITIVE_Y:
            TRACE("Asked for positive y\n");
            searchcaps.dwCaps2 |= DDSCAPS2_CUBEMAP_POSITIVEY; break;
        case WINED3DCUBEMAP_FACE_NEGATIVE_Y:
            TRACE("Asked for negative y\n");
            searchcaps.dwCaps2 |= DDSCAPS2_CUBEMAP_NEGATIVEY; break;
        case WINED3DCUBEMAP_FACE_POSITIVE_Z:
            TRACE("Asked for positive z\n");
            searchcaps.dwCaps2 |= DDSCAPS2_CUBEMAP_POSITIVEZ; break;
        case WINED3DCUBEMAP_FACE_NEGATIVE_Z:
            TRACE("Asked for negative z\n");
            searchcaps.dwCaps2 |= DDSCAPS2_CUBEMAP_NEGATIVEZ; break;
        default: {ERR("Unexpected cube face\n");} /* Stupid compiler */
    }

    if (!surf)
    {
        IDirectDrawSurface7 *attached;
        IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)ddraw->tex_root, &searchcaps, &attached);
        surf = (IDirectDrawSurfaceImpl *)attached;
        IDirectDrawSurface7_Release(attached);
    }
    if (!surf) ERR("root search surface not found\n");

    /* Find the wanted mipmap. There are enough mipmaps in the chain */
    while (i < level)
    {
        IDirectDrawSurface7 *attached;
        IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)surf, &searchcaps, &attached);
        if(!attached) ERR("Surface not found\n");
        surf = (IDirectDrawSurfaceImpl *)attached;
        IDirectDrawSurface7_Release(attached);
        ++i;
    }

    /* Return the surface */
    *surface = surf->wined3d_surface;
    wined3d_surface_incref(*surface);

    TRACE("Returning wineD3DSurface %p, it belongs to surface %p\n", *surface, surf);

    return D3D_OK;
}

static HRESULT WINAPI findRenderTarget(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *surface_desc, void *ctx)
{
    IDirectDrawSurfaceImpl *s = (IDirectDrawSurfaceImpl *)surface;
    IDirectDrawSurfaceImpl **target = ctx;

    if (!s->isRenderTarget)
    {
        *target = s;
        IDirectDrawSurface7_Release(surface);
        return DDENUMRET_CANCEL;
    }

    /* Recurse into the surface tree */
    IDirectDrawSurface7_EnumAttachedSurfaces(surface, ctx, findRenderTarget);

    IDirectDrawSurface7_Release(surface);
    if (*target) return DDENUMRET_CANCEL;

    return DDENUMRET_OK;
}

static HRESULT CDECL device_parent_create_rendertarget(struct wined3d_device_parent *device_parent,
        void *container_parent, UINT width, UINT height, enum wined3d_format_id format,
        WINED3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality, BOOL lockable,
        struct wined3d_surface **surface)
{
    struct IDirectDrawImpl *ddraw = ddraw_from_device_parent(device_parent);
    IDirectDrawSurfaceImpl *d3d_surface = ddraw->d3d_target;
    IDirectDrawSurfaceImpl *target = NULL;

    TRACE("device_parent %p, container_parent %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, lockable %u, surface %p.\n",
            device_parent, container_parent, width, height, format, multisample_type,
            multisample_quality, lockable, surface);

    if (d3d_surface->isRenderTarget)
    {
        IDirectDrawSurface7_EnumAttachedSurfaces((IDirectDrawSurface7 *)d3d_surface, &target, findRenderTarget);
    }
    else
    {
        target = d3d_surface;
    }

    if (!target)
    {
        target = ddraw->d3d_target;
        ERR(" (%p) : No DirectDrawSurface found to create the back buffer. Using the front buffer as back buffer. Uncertain consequences\n", ddraw);
    }

    /* TODO: Return failure if the dimensions do not match, but this shouldn't happen */

    *surface = target->wined3d_surface;
    wined3d_surface_incref(*surface);
    target->isRenderTarget = TRUE;

    TRACE("Returning wineD3DSurface %p, it belongs to surface %p\n", *surface, d3d_surface);

    return D3D_OK;
}

static HRESULT CDECL device_parent_create_depth_stencil(struct wined3d_device_parent *device_parent,
        UINT width, UINT height, enum wined3d_format_id format, WINED3DMULTISAMPLE_TYPE multisample_type,
        DWORD multisample_quality, BOOL discard, struct wined3d_surface **surface)
{
    struct IDirectDrawImpl *ddraw = ddraw_from_device_parent(device_parent);
    IDirectDrawSurfaceImpl *ddraw_surface;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;

    TRACE("device_parent %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, discard %u, surface %p.\n",
            device_parent, width, height, format, multisample_type, multisample_quality, discard, surface);

    *surface = NULL;

    /* Create a DirectDraw surface */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.u4.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwHeight = height;
    ddsd.dwWidth = width;
    if (format)
    {
        PixelFormat_WineD3DtoDD(&ddsd.u4.ddpfPixelFormat, format);
    }
    else
    {
        ddsd.dwFlags ^= DDSD_PIXELFORMAT;
    }

    ddraw->depthstencil = TRUE;
    hr = IDirectDraw7_CreateSurface(&ddraw->IDirectDraw7_iface, &ddsd,
            (IDirectDrawSurface7 **)&ddraw_surface, NULL);
    ddraw->depthstencil = FALSE;
    if (FAILED(hr))
    {
        WARN("Failed to create depth/stencil surface, hr %#x.\n", hr);
        return hr;
    }

    *surface = ddraw_surface->wined3d_surface;
    wined3d_surface_incref(*surface);
    IDirectDrawSurface7_Release((IDirectDrawSurface7 *)ddraw_surface);

    return D3D_OK;
}

static HRESULT CDECL device_parent_create_volume(struct wined3d_device_parent *device_parent,
        void *container_parent, UINT width, UINT height, UINT depth, enum wined3d_format_id format,
        WINED3DPOOL pool, DWORD usage, struct wined3d_volume **volume)
{
    TRACE("device_parent %p, container_parent %p, width %u, height %u, depth %u, "
            "format %#x, pool %#x, usage %#x, volume %p.\n",
            device_parent, container_parent, width, height, depth,
            format, pool, usage, volume);

    ERR("Not implemented!\n");

    return E_NOTIMPL;
}

static HRESULT CDECL device_parent_create_swapchain(struct wined3d_device_parent *device_parent,
        WINED3DPRESENT_PARAMETERS *present_parameters, struct wined3d_swapchain **swapchain)
{
    struct IDirectDrawImpl *ddraw = ddraw_from_device_parent(device_parent);
    IDirectDrawSurfaceImpl *iterator;
    HRESULT hr;

    TRACE("device_parent %p, present_parameters %p, swapchain %p.\n", device_parent, present_parameters, swapchain);

    hr = wined3d_swapchain_create(ddraw->wined3d_device, present_parameters,
            ddraw->ImplType, NULL, &ddraw_null_wined3d_parent_ops, swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to create swapchain, hr %#x.\n", hr);
        *swapchain = NULL;
        return hr;
    }

    ddraw->d3d_target->wined3d_swapchain = *swapchain;
    iterator = ddraw->d3d_target->complex_array[0];
    while (iterator)
    {
        iterator->wined3d_swapchain = *swapchain;
        iterator = iterator->complex_array[0];
    }

    return hr;
}

static const struct wined3d_device_parent_ops ddraw_wined3d_device_parent_ops =
{
    device_parent_wined3d_device_created,
    device_parent_create_surface,
    device_parent_create_rendertarget,
    device_parent_create_depth_stencil,
    device_parent_create_volume,
    device_parent_create_swapchain,
};

HRESULT ddraw_init(IDirectDrawImpl *ddraw, WINED3DDEVTYPE device_type)
{
    HRESULT hr;
    HDC hDC;

    ddraw->IDirectDraw7_iface.lpVtbl = &ddraw7_vtbl;
    ddraw->IDirectDraw_iface.lpVtbl = &ddraw1_vtbl;
    ddraw->IDirectDraw2_iface.lpVtbl = &ddraw2_vtbl;
    ddraw->IDirectDraw3_iface.lpVtbl = &ddraw3_vtbl;
    ddraw->IDirectDraw4_iface.lpVtbl = &ddraw4_vtbl;
    ddraw->IDirect3D_iface.lpVtbl = &d3d1_vtbl;
    ddraw->IDirect3D2_iface.lpVtbl = &d3d2_vtbl;
    ddraw->IDirect3D3_iface.lpVtbl = &d3d3_vtbl;
    ddraw->IDirect3D7_iface.lpVtbl = &d3d7_vtbl;
    ddraw->device_parent.ops = &ddraw_wined3d_device_parent_ops;
    ddraw->numIfaces = 1;
    ddraw->ref7 = 1;

    /* See comments in IDirectDrawImpl_CreateNewSurface for a description of
     * this field. */
    ddraw->ImplType = DefaultSurfaceType;

    /* Get the current screen settings. */
    hDC = GetDC(0);
    ddraw->orig_bpp = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
    ReleaseDC(0, hDC);
    ddraw->orig_width = GetSystemMetrics(SM_CXSCREEN);
    ddraw->orig_height = GetSystemMetrics(SM_CYSCREEN);

    ddraw->wineD3D = wined3d_create(7, &ddraw->IDirectDraw7_iface);
    if (!ddraw->wineD3D)
    {
        WARN("Failed to create a wined3d object.\n");
        return E_OUTOFMEMORY;
    }

    hr = wined3d_device_create(ddraw->wineD3D, WINED3DADAPTER_DEFAULT, device_type,
            NULL, 0, &ddraw->device_parent, &ddraw->wined3d_device);
    if (FAILED(hr))
    {
        WARN("Failed to create a wined3d device, hr %#x.\n", hr);
        wined3d_decref(ddraw->wineD3D);
        return hr;
    }

    /* Get the amount of video memory */
    ddraw->total_vidmem = wined3d_device_get_available_texture_mem(ddraw->wined3d_device);

    list_init(&ddraw->surface_list);

    return DD_OK;
}
