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

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "wine/exception.h"

#include "ddraw.h"
#include "d3d.h"

#include "ddraw_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static BOOL IDirectDrawImpl_DDSD_Match(const DDSURFACEDESC2* requested, const DDSURFACEDESC2* provided);
static HRESULT IDirectDrawImpl_AttachD3DDevice(IDirectDrawImpl *This, IDirectDrawSurfaceImpl *primary);
static HRESULT IDirectDrawImpl_CreateNewSurface(IDirectDrawImpl *This, DDSURFACEDESC2 *pDDSD, IDirectDrawSurfaceImpl **ppSurf, UINT level);
static HRESULT IDirectDrawImpl_CreateGDISwapChain(IDirectDrawImpl *This, IDirectDrawSurfaceImpl *primary);

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
static HRESULT WINAPI
IDirectDrawImpl_QueryInterface(IDirectDraw7 *iface,
                               REFIID refiid,
                               void **obj)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(refiid), obj);

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
        *obj = &This->IDirectDraw4_vtbl;
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
        *obj = &This->IDirectDraw2_vtbl;
        TRACE("(%p) Returning IDirectDraw2 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw, refiid ) )
    {
        *obj = &This->IDirectDraw_vtbl;
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
            *obj = &This->IDirect3D_vtbl;
            TRACE(" returning Direct3D interface at %p.\n", *obj);
        }
        else if ( IsEqualGUID( &IID_IDirect3D2  , refiid ) )
        {
            This->d3dversion = 2;
            *obj = &This->IDirect3D2_vtbl;
            TRACE(" returning Direct3D2 interface at %p.\n", *obj);
        }
        else if ( IsEqualGUID( &IID_IDirect3D3  , refiid ) )
        {
            This->d3dversion = 3;
            *obj = &This->IDirect3D3_vtbl;
            TRACE(" returning Direct3D3 interface at %p.\n", *obj);
        }
        else if(IsEqualGUID( &IID_IDirect3D7  , refiid ))
        {
            This->d3dversion = 7;
            *obj = &This->IDirect3D7_vtbl;
            TRACE(" returning Direct3D7 interface at %p.\n", *obj);
        }
    }
    else if (IsEqualGUID(refiid, &IID_IWineD3DDeviceParent))
    {
        *obj = &This->device_parent_vtbl;
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
static ULONG WINAPI
IDirectDrawImpl_AddRef(IDirectDraw7 *iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref7);

    TRACE("(%p) : incrementing IDirectDraw7 refcount from %u.\n", This, ref -1);

    if(ref == 1) InterlockedIncrement(&This->numIfaces);

    return ref;
}

/*****************************************************************************
 * IDirectDrawImpl_Destroy
 *
 * Destroys a ddraw object if all refcounts are 0. This is to share code
 * between the IDirectDrawX::Release functions
 *
 * Params:
 *  This: DirectDraw object to destroy
 *
 *****************************************************************************/
void
IDirectDrawImpl_Destroy(IDirectDrawImpl *This)
{
    /* Clear the cooplevel to restore window and display mode */
    IDirectDraw7_SetCooperativeLevel((IDirectDraw7 *)This, NULL, DDSCL_NORMAL);

    /* Destroy the device window if we created one */
    if(This->devicewindow != 0)
    {
        TRACE(" (%p) Destroying the device window %p\n", This, This->devicewindow);
        DestroyWindow(This->devicewindow);
        This->devicewindow = 0;
    }

    /* Unregister the window class */
    UnregisterClassA(This->classname, 0);

    EnterCriticalSection(&ddraw_cs);
    list_remove(&This->ddraw_list_entry);
    LeaveCriticalSection(&ddraw_cs);

    /* Release the attached WineD3D stuff */
    IWineD3DDevice_Release(This->wineD3DDevice);
    IWineD3D_Release(This->wineD3D);

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
static ULONG WINAPI
IDirectDrawImpl_Release(IDirectDraw7 *iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref7);

    TRACE("(%p)->() decrementing IDirectDraw7 refcount from %u.\n", This, ref +1);

    if(ref == 0)
    {
        ULONG ifacecount = InterlockedDecrement(&This->numIfaces);
        if(ifacecount == 0) IDirectDrawImpl_Destroy(This);
    }

    return ref;
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
 *  rendering (Possible test case: Half-life)
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
static HRESULT WINAPI
IDirectDrawImpl_SetCooperativeLevel(IDirectDraw7 *iface,
                                    HWND hwnd,
                                    DWORD cooplevel)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    HWND window;

    TRACE("(%p)->(%p,%08x)\n",This,hwnd,cooplevel);
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
        if(cooplevel & ( DDSCL_MULTITHREADED   |
                         DDSCL_FPUSETUP        |
                         DDSCL_FPUPRESERVE     |
                         DDSCL_ALLOWREBOOT     |
                         DDSCL_ALLOWMODEX      |
                         DDSCL_SETDEVICEWINDOW |
                         DDSCL_NORMAL          |
                         DDSCL_EXCLUSIVE       |
                         DDSCL_FULLSCREEN      ) )
        {
            TRACE("Called with incompatible flags, returning DDERR_INVALIDPARAMS\n");
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }
        else if( (This->cooperative_level & DDSCL_FULLSCREEN) && window)
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
    }
    /* DDSCL_NORMAL or DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE */
    if(cooplevel & DDSCL_NORMAL)
    {
        /* Can't coexist with fullscreen or exclusive */
        if(cooplevel & (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) )
        {
            TRACE("(%p) DDSCL_NORMAL is not compative with DDSCL_FULLSCREEN or DDSCL_EXCLUSIVE\n", This);
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }

        /* Switching from fullscreen? */
        if(This->cooperative_level & DDSCL_FULLSCREEN)
        {
            /* Restore the display mode */
            IDirectDraw7_RestoreDisplayMode(iface);

            This->cooperative_level &= ~DDSCL_FULLSCREEN;
            This->cooperative_level &= ~DDSCL_EXCLUSIVE;
            This->cooperative_level &= ~DDSCL_ALLOWMODEX;
        }

        /* Don't override focus windows or private device windows */
        if( hwnd &&
            !(This->focuswindow) &&
            !(This->devicewindow) &&
            (hwnd != window) )
        {
            This->dest_window = hwnd;
        }
    }
    else if(cooplevel & DDSCL_FULLSCREEN)
    {
        /* Needs DDSCL_EXCLUSIVE */
        if(!(cooplevel & DDSCL_EXCLUSIVE) )
        {
            TRACE("(%p) DDSCL_FULLSCREEN needs DDSCL_EXCLUSIVE\n", This);
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }
        /* Need a HWND
        if(hwnd == 0)
        {
            TRACE("(%p) DDSCL_FULLSCREEN needs a HWND\n", This);
            return DDERR_INVALIDPARAMS;
        }
        */

        This->cooperative_level &= ~DDSCL_NORMAL;

        /* Don't override focus windows or private device windows */
        if( hwnd &&
            !(This->focuswindow) &&
            !(This->devicewindow) &&
            (hwnd != window) )
        {
            This->dest_window = hwnd;
        }
    }
    else if(cooplevel & DDSCL_EXCLUSIVE)
    {
        TRACE("(%p) DDSCL_EXCLUSIVE needs DDSCL_FULLSCREEN\n", This);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    if(cooplevel & DDSCL_CREATEDEVICEWINDOW)
    {
        /* Don't create a device window if a focus window is set */
        if( !(This->focuswindow) )
        {
            HWND devicewindow = CreateWindowExA(0, This->classname, "DDraw device window",
                                                WS_POPUP, 0, 0,
                                                GetSystemMetrics(SM_CXSCREEN),
                                                GetSystemMetrics(SM_CYSCREEN),
                                                NULL, NULL, GetModuleHandleA(0), NULL);

            ShowWindow(devicewindow, SW_SHOW);   /* Just to be sure */
            TRACE("(%p) Created a DDraw device window. HWND=%p\n", This, devicewindow);

            This->devicewindow = devicewindow;
            This->dest_window = devicewindow;
        }
    }

    if(cooplevel & DDSCL_MULTITHREADED && !(This->cooperative_level & DDSCL_MULTITHREADED))
    {
        /* Enable thread safety in wined3d */
        IWineD3DDevice_SetMultithreaded(This->wineD3DDevice);
    }

    /* Unhandled flags */
    if(cooplevel & DDSCL_ALLOWREBOOT)
        WARN("(%p) Unhandled flag DDSCL_ALLOWREBOOT, harmless\n", This);
    if(cooplevel & DDSCL_ALLOWMODEX)
        WARN("(%p) Unhandled flag DDSCL_ALLOWMODEX, harmless\n", This);
    if(cooplevel & DDSCL_FPUSETUP)
        WARN("(%p) Unhandled flag DDSCL_FPUSETUP, harmless\n", This);

    /* Store the cooperative_level */
    This->cooperative_level |= cooplevel;
    TRACE("SetCooperativeLevel retuning DD_OK\n");
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 *
 * Helper function for SetDisplayMode and RestoreDisplayMode
 *
 * Implements DirectDraw's SetDisplayMode, but ignores the value of
 * ForceRefreshRate, since it is already handled by
 * IDirectDrawImpl_SetDisplayMode.  RestoreDisplayMode can use this function
 * without worrying that ForceRefreshRate will override the refresh rate.  For
 * argument and return value documentation, see
 * IDirectDrawImpl_SetDisplayMode.
 *
 *****************************************************************************/
static HRESULT
IDirectDrawImpl_SetDisplayModeNoOverride(IDirectDraw7 *iface,
                                         DWORD Width,
                                         DWORD Height,
                                         DWORD BPP,
                                         DWORD RefreshRate,
                                         DWORD Flags)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    WINED3DDISPLAYMODE Mode;
    HRESULT hr;
    TRACE("(%p)->(%d,%d,%d,%d,%x): Relay!\n", This, Width, Height, BPP, RefreshRate, Flags);

    EnterCriticalSection(&ddraw_cs);
    if( !Width || !Height )
    {
        ERR("Width=%d, Height=%d, what to do?\n", Width, Height);
        /* It looks like Need for Speed Porsche Unleashed expects DD_OK here */
        LeaveCriticalSection(&ddraw_cs);
        return DD_OK;
    }

    /* Check the exclusive mode
    if(!(This->cooperative_level & DDSCL_EXCLUSIVE))
        return DDERR_NOEXCLUSIVEMODE;
     * This is WRONG. Don't know if the SDK is completely
     * wrong and if there are any conditions when DDERR_NOEXCLUSIVE
     * is returned, but Half-Life 1.1.1.1 (Steam version)
     * depends on this
     */

    Mode.Width = Width;
    Mode.Height = Height;
    Mode.RefreshRate = RefreshRate;
    switch(BPP)
    {
        case 8:  Mode.Format = WINED3DFMT_P8;       break;
        case 15: Mode.Format = WINED3DFMT_X1R5G5B5; break;
        case 16: Mode.Format = WINED3DFMT_R5G6B5;   break;
        case 24: Mode.Format = WINED3DFMT_R8G8B8;   break;
        case 32: Mode.Format = WINED3DFMT_X8R8G8B8; break;
    }

    /* TODO: The possible return values from msdn suggest that
     * the screen mode can't be changed if a surface is locked
     * or some drawing is in progress
     */

    /* TODO: Lose the primary surface */
    hr = IWineD3DDevice_SetDisplayMode(This->wineD3DDevice,
                                       0, /* First swapchain */
                                       &Mode);
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
static HRESULT WINAPI
IDirectDrawImpl_SetDisplayMode(IDirectDraw7 *iface,
                               DWORD Width,
                               DWORD Height,
                               DWORD BPP,
                               DWORD RefreshRate,
                               DWORD Flags)
{
    if (force_refresh_rate != 0)
    {
        TRACE("ForceRefreshRate overriding passed-in refresh rate (%d Hz) to %d Hz\n", RefreshRate, force_refresh_rate);
        RefreshRate = force_refresh_rate;
    }

    return IDirectDrawImpl_SetDisplayModeNoOverride(iface, Width, Height, BPP,
                                                    RefreshRate, Flags);
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
static HRESULT WINAPI
IDirectDrawImpl_RestoreDisplayMode(IDirectDraw7 *iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)\n", This);

    return IDirectDrawImpl_SetDisplayModeNoOverride(iface,
            This->orig_width, This->orig_height, This->orig_bpp, 0, 0);
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
static HRESULT WINAPI
IDirectDrawImpl_GetCaps(IDirectDraw7 *iface,
                        DDCAPS *DriverCaps,
                        DDCAPS *HELCaps)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    DDCAPS caps;
    WINED3DCAPS winecaps;
    HRESULT hr;
    DDSCAPS2 ddscaps = {0, 0, 0, {0}};
    TRACE("(%p)->(%p,%p)\n", This, DriverCaps, HELCaps);

    /* One structure must be != NULL */
    if( (!DriverCaps) && (!HELCaps) )
    {
        ERR("(%p) Invalid params to IDirectDrawImpl_GetCaps\n", This);
        return DDERR_INVALIDPARAMS;
    }

    memset(&caps, 0, sizeof(caps));
    memset(&winecaps, 0, sizeof(winecaps));
    caps.dwSize = sizeof(caps);
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_GetDeviceCaps(This->wineD3DDevice, &winecaps);
    if(FAILED(hr)) {
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

/*****************************************************************************
 * IDirectDraw7::Compact
 *
 * No idea what it does, MSDN says it's not implemented.
 *
 * Returns
 *  DD_OK, but this is unchecked
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawImpl_Compact(IDirectDraw7 *iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)\n", This);

    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_GetDisplayMode(IDirectDraw7 *iface,
                               DDSURFACEDESC2 *DDSD)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    HRESULT hr;
    WINED3DDISPLAYMODE Mode;
    DWORD Size;
    TRACE("(%p)->(%p): Relay\n", This, DDSD);

    EnterCriticalSection(&ddraw_cs);
    /* This seems sane */
    if(!DDSD) 
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* The necessary members of LPDDSURFACEDESC and LPDDSURFACEDESC2 are equal,
     * so one method can be used for all versions (Hopefully)
     */
    hr = IWineD3DDevice_GetDisplayMode(This->wineD3DDevice,
                                      0 /* swapchain 0 */,
                                      &Mode);
    if( hr != D3D_OK )
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
static HRESULT WINAPI
IDirectDrawImpl_GetFourCCCodes(IDirectDraw7 *iface,
                               DWORD *NumCodes, DWORD *Codes)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    WINED3DFORMAT formats[] = {
        WINED3DFMT_YUY2, WINED3DFMT_UYVY, WINED3DFMT_YV12,
        WINED3DFMT_DXT1, WINED3DFMT_DXT2, WINED3DFMT_DXT3, WINED3DFMT_DXT4, WINED3DFMT_DXT5,
        WINED3DFMT_ATI2N, WINED3DFMT_NVHU, WINED3DFMT_NVHS
    };
    DWORD count = 0, i, outsize;
    HRESULT hr;
    WINED3DDISPLAYMODE d3ddm;
    WINED3DSURFTYPE type = This->ImplType;
    TRACE("(%p)->(%p, %p)\n", This, NumCodes, Codes);

    IWineD3DDevice_GetDisplayMode(This->wineD3DDevice,
                                  0 /* swapchain 0 */,
                                  &d3ddm);

    outsize = NumCodes && Codes ? *NumCodes : 0;

    if(type == SURFACE_UNKNOWN) type = SURFACE_GDI;

    for(i = 0; i < (sizeof(formats) / sizeof(formats[0])); i++) {
        hr = IWineD3D_CheckDeviceFormat(This->wineD3D,
                                        WINED3DADAPTER_DEFAULT,
                                        WINED3DDEVTYPE_HAL,
                                        d3ddm.Format /* AdapterFormat */,
                                        0 /* usage */,
                                        WINED3DRTYPE_SURFACE,
                                        formats[i],
                                        type);
        if(SUCCEEDED(hr)) {
            if(count < outsize) {
                Codes[count] = formats[i];
            }
            count++;
        }
    }
    if(NumCodes) {
        TRACE("Returning %u FourCC codes\n", count);
        *NumCodes = count;
    }

    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_GetMonitorFrequency(IDirectDraw7 *iface,
                                    DWORD *Freq)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%p)\n", This, Freq);

    /* Ideally this should be in WineD3D, as it concerns the screen setup,
     * but for now this should make the games happy
     */
    *Freq = 60;
    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_GetVerticalBlankStatus(IDirectDraw7 *iface,
                                       BOOL *status)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%p)\n", This, status);

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
static HRESULT WINAPI
IDirectDrawImpl_GetAvailableVidMem(IDirectDraw7 *iface, DDSCAPS2 *Caps, DWORD *total, DWORD *free)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%p, %p, %p)\n", This, Caps, total, free);

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

    if(total) *total = This->total_vidmem;
    if(free) *free = IWineD3DDevice_GetAvailableTextureMem(This->wineD3DDevice);

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_Initialize(IDirectDraw7 *iface,
                           GUID *Guid)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%s): No-op\n", This, debugstr_guid(Guid));

    if(This->initialized)
    {
        return DDERR_ALREADYINITIALIZED;
    }
    else
    {
        return DD_OK;
    }
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
static HRESULT WINAPI
IDirectDrawImpl_FlipToGDISurface(IDirectDraw7 *iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)\n", This);

    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_WaitForVerticalBlank(IDirectDraw7 *iface,
                                     DWORD Flags,
                                     HANDLE h)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    static BOOL hide = FALSE;

    /* This function is called often, so print the fixme only once */
    if(!hide)
    {
        FIXME("(%p)->(%x,%p): Stub\n", This, Flags, h);
        hide = TRUE;
    }

    /* MSDN says DDWAITVB_BLOCKBEGINEVENT is not supported */
    if(Flags & DDWAITVB_BLOCKBEGINEVENT)
        return DDERR_UNSUPPORTED; /* unchecked */

    return DD_OK;
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
static HRESULT WINAPI IDirectDrawImpl_GetScanLine(IDirectDraw7 *iface, DWORD *Scanline)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    static BOOL hide = FALSE;
    WINED3DDISPLAYMODE Mode;

    /* This function is called often, so print the fixme only once */
    EnterCriticalSection(&ddraw_cs);
    if(!hide)
    {
        FIXME("(%p)->(%p): Semi-Stub\n", This, Scanline);
        hide = TRUE;
    }

    IWineD3DDevice_GetDisplayMode(This->wineD3DDevice,
                                  0,
                                  &Mode);

    /* Fake the line sweeping of the monitor */
    /* FIXME: We should synchronize with a source to keep the refresh rate */ 
    *Scanline = This->cur_scanline++;
    /* Assume 20 scan lines in the vertical blank */
    if (This->cur_scanline >= Mode.Height + 20)
        This->cur_scanline = 0;

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_TestCooperativeLevel(IDirectDraw7 *iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    HRESULT hr;
    TRACE("(%p)\n", This);

    EnterCriticalSection(&ddraw_cs);
    /* Description from MSDN:
     * For fullscreen apps return DDERR_NOEXCLUSIVEMODE if the user switched
     * away from the app with e.g. alt-tab. Windowed apps receive 
     * DDERR_EXCLUSIVEMODEALREADYSET if another application created a 
     * DirectDraw object in exclusive mode. DDERR_WRONGMODE is returned,
     * when the video mode has changed
     */

    hr =  IWineD3DDevice_TestCooperativeLevel(This->wineD3DDevice);

    /* Fix the result value. These values are mapped from their
     * d3d9 counterpart.
     */
    switch(hr)
    {
        case WINED3DERR_DEVICELOST:
            if(This->cooperative_level & DDSCL_EXCLUSIVE)
            {
                LeaveCriticalSection(&ddraw_cs);
                return DDERR_NOEXCLUSIVEMODE;
            }
            else
            {
                LeaveCriticalSection(&ddraw_cs);
                return DDERR_EXCLUSIVEMODEALREADYSET;
            }

        case WINED3DERR_DEVICENOTRESET:
            LeaveCriticalSection(&ddraw_cs);
            return DD_OK;

        case WINED3D_OK:
            LeaveCriticalSection(&ddraw_cs);
            return DD_OK;

        case WINED3DERR_DRIVERINTERNALERROR:
        default:
            ERR("(%p) Unexpected return value %08x from wineD3D, "
                " returning DD_OK\n", This, hr);
    }

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_GetGDISurface(IDirectDraw7 *iface,
                              IDirectDrawSurface7 **GDISurface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    IWineD3DSurface *Surf;
    IDirectDrawSurface7 *ddsurf;
    HRESULT hr;
    DDSCAPS2 ddsCaps;
    TRACE("(%p)->(%p)\n", This, GDISurface);

    /* Get the back buffer from the wineD3DDevice and search its
     * attached surfaces for the front buffer
     */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_GetBackBuffer(This->wineD3DDevice,
                                      0, /* SwapChain */
                                      0, /* first back buffer*/
                                      WINED3DBACKBUFFER_TYPE_MONO,
                                      &Surf);

    if( (hr != D3D_OK) ||
        (!Surf) )
    {
        ERR("IWineD3DDevice::GetBackBuffer failed\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_NOTFOUND;
    }

    /* GetBackBuffer AddRef()ed the surface, release it */
    IWineD3DSurface_Release(Surf);

    IWineD3DSurface_GetParent(Surf,
                              (IUnknown **) &ddsurf);
    IDirectDrawSurface7_Release(ddsurf);  /* For the GetParent */

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

/*****************************************************************************
 * IDirectDraw7::EnumDisplayModes
 *
 * Enumerates the supported Display modes. The modes can be filtered with
 * the DDSD parameter.
 *
 * Params:
 *  Flags: can be DDEDM_REFRESHRATES and DDEDM_STANDARDVGAMODES
 *  DDSD: Surface description to filter the modes
 *  Context: Pointer passed back to the callback function
 *  cb: Application-provided callback function
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if the callback wasn't set
 *
 *****************************************************************************/ 
static HRESULT WINAPI
IDirectDrawImpl_EnumDisplayModes(IDirectDraw7 *iface,
                                 DWORD Flags,
                                 DDSURFACEDESC2 *DDSD,
                                 void *Context,
                                 LPDDENUMMODESCALLBACK2 cb)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    unsigned int modenum, fmt;
    WINED3DFORMAT pixelformat = WINED3DFMT_UNKNOWN;
    WINED3DDISPLAYMODE mode;
    DDSURFACEDESC2 callback_sd;
    WINED3DDISPLAYMODE *enum_modes = NULL;
    unsigned enum_mode_count = 0, enum_mode_array_size = 0;

    WINED3DFORMAT checkFormatList[] =
    {
        WINED3DFMT_R8G8B8,
        WINED3DFMT_A8R8G8B8,
        WINED3DFMT_X8R8G8B8,
        WINED3DFMT_R5G6B5,
        WINED3DFMT_X1R5G5B5,
        WINED3DFMT_A1R5G5B5,
        WINED3DFMT_A4R4G4B4,
        WINED3DFMT_R3G3B2,
        WINED3DFMT_A8R3G3B2,
        WINED3DFMT_X4R4G4B4,
        WINED3DFMT_R10G10B10A2_UNORM,
        WINED3DFMT_R8G8B8A8_UNORM,
        WINED3DFMT_X8B8G8R8,
        WINED3DFMT_A2R10G10B10,
        WINED3DFMT_A8P8,
        WINED3DFMT_P8
    };

    TRACE("(%p)->(%p,%p,%p): Relay\n", This, DDSD, Context, cb);

    EnterCriticalSection(&ddraw_cs);
    /* This looks sane */
    if(!cb)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    if(DDSD)
    {
        if ((DDSD->dwFlags & DDSD_PIXELFORMAT) && (DDSD->u4.ddpfPixelFormat.dwFlags & DDPF_RGB) )
            pixelformat = PixelFormat_DD2WineD3D(&DDSD->u4.ddpfPixelFormat);
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

    for(fmt = 0; fmt < (sizeof(checkFormatList) / sizeof(checkFormatList[0])); fmt++)
    {
        if(pixelformat != WINED3DFMT_UNKNOWN && checkFormatList[fmt] != pixelformat)
        {
            continue;
        }

        modenum = 0;
        while(IWineD3D_EnumAdapterModes(This->wineD3D,
                                        WINED3DADAPTER_DEFAULT,
                                        checkFormatList[fmt],
                                        modenum++,
                                        &mode) == WINED3D_OK)
        {
            if(DDSD)
            {
                if(DDSD->dwFlags & DDSD_WIDTH && mode.Width != DDSD->dwWidth) continue;
                if(DDSD->dwFlags & DDSD_HEIGHT && mode.Height != DDSD->dwHeight) continue;
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

            callback_sd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_PITCH;
            if(Flags & DDEDM_REFRESHRATES)
            {
                callback_sd.dwFlags |= DDSD_REFRESHRATE;
                callback_sd.u2.dwRefreshRate = mode.RefreshRate;
            }

            callback_sd.dwWidth = mode.Width;
            callback_sd.dwHeight = mode.Height;

            PixelFormat_WineD3DtoDD(&callback_sd.u4.ddpfPixelFormat, mode.Format);

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
static HRESULT WINAPI
IDirectDrawImpl_EvaluateMode(IDirectDraw7 *iface,
                             DWORD Flags,
                             DWORD *Timeout)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    FIXME("(%p)->(%d,%p): Stub!\n", This, Flags, Timeout);

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
static HRESULT WINAPI
IDirectDrawImpl_GetDeviceIdentifier(IDirectDraw7 *iface,
                                    DDDEVICEIDENTIFIER2 *DDDI,
                                    DWORD Flags)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%p,%08x)\n", This, DDDI, Flags);

    if(!DDDI)
        return DDERR_INVALIDPARAMS;

    /* The DDGDI_GETHOSTIDENTIFIER returns the information about the 2D
     * host adapter, if there's a secondary 3D adapter. This doesn't apply
     * to any modern hardware, nor is it interesting for Wine, so ignore it
     */

    *DDDI = deviceidentifier;
    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_GetSurfaceFromDC(IDirectDraw7 *iface,
                                 HDC hdc,
                                 IDirectDrawSurface7 **Surface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    FIXME("(%p)->(%p,%p): Stub!\n", This, hdc, Surface);

    /* Implementation idea if needed: Loop through all surfaces and compare
     * their hdc with hdc. Implement it in WineD3D! */
    return DDERR_NOTFOUND;
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
static HRESULT WINAPI
IDirectDrawImpl_RestoreAllSurfaces(IDirectDraw7 *iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    FIXME("(%p): Stub\n", This);

    /* This isn't hard to implement: Enumerate all WineD3D surfaces,
     * get their parent and call their restore method. Do not implement
     * it in WineD3D, as restoring a surface means re-creating the
     * WineD3DDSurface
     */
    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_StartModeTest(IDirectDraw7 *iface,
                              SIZE *Modes,
                              DWORD NumModes,
                              DWORD Flags)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    WARN("(%p)->(%p, %d, %x): Semi-Stub, most likely harmless\n", This, Modes, NumModes, Flags);

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
 * IDirectDrawImpl_RecreateSurfacesCallback
 *
 * Enumeration callback for IDirectDrawImpl_RecreateAllSurfaces.
 * It re-recreates the WineD3DSurface. It's pretty straightforward
 *
 *****************************************************************************/
HRESULT WINAPI
IDirectDrawImpl_RecreateSurfacesCallback(IDirectDrawSurface7 *surf,
                                         DDSURFACEDESC2 *desc,
                                         void *Context)
{
    IDirectDrawSurfaceImpl *surfImpl = (IDirectDrawSurfaceImpl *)surf;
    IDirectDrawImpl *This = surfImpl->ddraw;
    IUnknown *Parent;
    IParentImpl *parImpl = NULL;
    IWineD3DSurface *wineD3DSurface;
    IWineD3DSwapChain *swapchain;
    HRESULT hr;
    void *tmp;
    IWineD3DClipper *clipper = NULL;

    WINED3DSURFACE_DESC     Desc;
    WINED3DFORMAT           Format;
    WINED3DRESOURCETYPE     Type;
    DWORD                   Usage;
    WINED3DPOOL             Pool;
    UINT                    Size;

    WINED3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD                   MultiSampleQuality;
    UINT                    Width;
    UINT                    Height;

    TRACE("(%p): Enumerated Surface %p\n", This, surfImpl);

    /* For the enumeration */
    IDirectDrawSurface7_Release(surf);

    if(surfImpl->ImplType == This->ImplType) return DDENUMRET_OK; /* Continue */

    /* Get the objects */
    swapchain = surfImpl->wineD3DSwapChain;
    surfImpl->wineD3DSwapChain = NULL;
    wineD3DSurface = surfImpl->WineD3DSurface;
    IWineD3DSurface_GetParent(wineD3DSurface, &Parent);
    IUnknown_Release(Parent); /* For the getParent */

    /* Is the parent an IParent interface? */
    if(IUnknown_QueryInterface(Parent, &IID_IParent, &tmp) == S_OK)
    {
        /* It is a IParent interface! */
        IUnknown_Release(Parent); /* For the QueryInterface */
        parImpl = (IParentImpl *)Parent;
        /* Release the reference the parent interface is holding */
        IWineD3DSurface_Release(wineD3DSurface);
    }

    /* get the clipper */
    IWineD3DSurface_GetClipper(wineD3DSurface, &clipper);

    /* Get the surface properties */
    Desc.Format = &Format;
    Desc.Type = &Type;
    Desc.Usage = &Usage;
    Desc.Pool = &Pool;
    Desc.Size = &Size;
    Desc.MultiSampleType = &MultiSampleType;
    Desc.MultiSampleQuality = &MultiSampleQuality;
    Desc.Width = &Width;
    Desc.Height = &Height;

    hr = IWineD3DSurface_GetDesc(wineD3DSurface, &Desc);
    if(hr != D3D_OK) return hr;

    if(swapchain) {
        /* If there's a swapchain, it owns the IParent interface. Create a new one for the
         * new surface
         */
        parImpl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*parImpl));
        parImpl->lpVtbl = &IParent_Vtbl;
        parImpl->ref = 1;

        Parent = (IUnknown *) parImpl;
    }

    /* Create the new surface */
    hr = IWineD3DDevice_CreateSurface(This->wineD3DDevice, Width, Height, Format,
            TRUE /* Lockable */, FALSE /* Discard */, surfImpl->mipmap_level, &surfImpl->WineD3DSurface,
            Type, Usage, Pool, MultiSampleType, MultiSampleQuality, This->ImplType, Parent);

    if(hr != D3D_OK)
        return hr;

    IWineD3DSurface_SetClipper(surfImpl->WineD3DSurface, clipper);

    /* Update the IParent if it exists */
    if(parImpl)
    {
        parImpl->child = (IUnknown *) surfImpl->WineD3DSurface;
        /* Add a reference for the IParent */
        IWineD3DSurface_AddRef(surfImpl->WineD3DSurface);
    }
    /* TODO: Copy the surface content, except for render targets */

    /* If there's a swapchain, it owns the wined3d surfaces. So Destroy
     * the swapchain
     */
    if(swapchain) {
        /* The backbuffers have the swapchain set as well, but the primary
         * owns it and destroys it
         */
        if(surfImpl->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
            IWineD3DDevice_UninitGDI(This->wineD3DDevice, D3D7CB_DestroySwapChain);
        }
        surfImpl->isRenderTarget = FALSE;
    } else {
        if(IWineD3DSurface_Release(wineD3DSurface) == 0)
            TRACE("Surface released successful, next surface\n");
        else
            ERR("Something's still holding the old WineD3DSurface\n");
    }

    surfImpl->ImplType = This->ImplType;

    if(clipper)
    {
        IWineD3DClipper_Release(clipper);
    }
    return DDENUMRET_OK;
}

/*****************************************************************************
 * IDirectDrawImpl_RecreateAllSurfaces
 *
 * A function, that converts all wineD3DSurfaces to the new implementation type
 * It enumerates all surfaces with IWineD3DDevice::EnumSurfaces, creates a
 * new WineD3DSurface, copies the content and releases the old surface
 *
 *****************************************************************************/
static HRESULT
IDirectDrawImpl_RecreateAllSurfaces(IDirectDrawImpl *This)
{
    DDSURFACEDESC2 desc;
    TRACE("(%p): Switch to implementation %d\n", This, This->ImplType);

    if(This->ImplType != SURFACE_OPENGL && This->d3d_initialized)
    {
        /* Should happen almost never */
        FIXME("(%p) Switching to non-opengl surfaces with d3d started. Is this a bug?\n", This);
        /* Shutdown d3d */
        IWineD3DDevice_Uninit3D(This->wineD3DDevice, D3D7CB_DestroyDepthStencilSurface, D3D7CB_DestroySwapChain);
    }
    /* Contrary: D3D starting is handled by the caller, because it knows the render target */

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    return IDirectDraw7_EnumSurfaces((IDirectDraw7 *)This, 0, &desc, This, IDirectDrawImpl_RecreateSurfacesCallback);
}

ULONG WINAPI D3D7CB_DestroySwapChain(IWineD3DSwapChain *pSwapChain) {
    IUnknown* swapChainParent;
    TRACE("(%p) call back\n", pSwapChain);

    IWineD3DSwapChain_GetParent(pSwapChain, &swapChainParent);
    IUnknown_Release(swapChainParent);
    return IUnknown_Release(swapChainParent);
}

ULONG WINAPI D3D7CB_DestroyDepthStencilSurface(IWineD3DSurface *pSurface) {
    IUnknown* surfaceParent;
    TRACE("(%p) call back\n", pSurface);

    IWineD3DSurface_GetParent(pSurface, &surfaceParent);
    IUnknown_Release(surfaceParent);
    return IUnknown_Release(surfaceParent);
}

/*****************************************************************************
 * IDirectDrawImpl_CreateNewSurface
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
static HRESULT
IDirectDrawImpl_CreateNewSurface(IDirectDrawImpl *This,
                                 DDSURFACEDESC2 *pDDSD,
                                 IDirectDrawSurfaceImpl **ppSurf,
                                 UINT level)
{
    HRESULT hr;
    UINT Width = 0, Height = 0;
    WINED3DFORMAT Format = WINED3DFMT_UNKNOWN;
    WINED3DRESOURCETYPE ResType = WINED3DRTYPE_SURFACE;
    DWORD Usage = 0;
    WINED3DSURFTYPE ImplType = This->ImplType;
    WINED3DSURFACE_DESC Desc;
    IUnknown *Parent;
    IParentImpl *parImpl = NULL;
    WINED3DPOOL Pool = WINED3DPOOL_DEFAULT;

    /* Dummies for GetDesc */
    WINED3DPOOL dummy_d3dpool;
    WINED3DMULTISAMPLE_TYPE dummy_mst;
    UINT dummy_uint;
    DWORD dummy_dword;

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
         if((pDDSD->ddsCaps.dwCaps & DDSCAPS_3DDEVICE ) && 
            (This->ImplType != SURFACE_OPENGL ) && DefaultSurfaceType == SURFACE_UNKNOWN)
        {
            /* We have to change to OpenGL,
             * and re-create all WineD3DSurfaces
             */
            ImplType = SURFACE_OPENGL;
            This->ImplType = ImplType;
            TRACE("(%p) Re-creating all surfaces\n", This);
            IDirectDrawImpl_RecreateAllSurfaces(This);
            TRACE("(%p) Done recreating all surfaces\n", This);
        }
        else if(This->ImplType != SURFACE_OPENGL && pDDSD->ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
        {
            WARN("The application requests a 3D capable surface, but a non-opengl surface was set in the registry\n");
            /* Do not fail surface creation, only fail 3D device creation */
        }
    }

    if (!(pDDSD->ddsCaps.dwCaps & (DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY)) &&
        !((pDDSD->ddsCaps.dwCaps & DDSCAPS_TEXTURE) && (pDDSD->ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)) )
    {
        /* Tests show surfaces without memory flags get these flags added right after creation. */
        pDDSD->ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    }
    /* Get the correct wined3d usage */
    if (pDDSD->ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE |
                                 DDSCAPS_3DDEVICE       ) )
    {
        Usage |= WINED3DUSAGE_RENDERTARGET;

        pDDSD->ddsCaps.dwCaps |= DDSCAPS_VISIBLE;
    }
    if (pDDSD->ddsCaps.dwCaps & (DDSCAPS_OVERLAY))
    {
        Usage |= WINED3DUSAGE_OVERLAY;
    }
    if(This->depthstencil || (pDDSD->ddsCaps.dwCaps & DDSCAPS_ZBUFFER) )
    {
        /* The depth stencil creation callback sets this flag.
         * Set the WineD3D usage to let it know that it's a depth
         * Stencil surface.
         */
        Usage |= WINED3DUSAGE_DEPTHSTENCIL;
    }
    if(pDDSD->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
    {
        Pool = WINED3DPOOL_SYSTEMMEM;
    }
    else if(pDDSD->ddsCaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
    {
        Pool = WINED3DPOOL_MANAGED;
        /* Managed textures have the system memory flag set */
        pDDSD->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    }
    else if(pDDSD->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
    {
        /* Videomemory adds localvidmem, this is mutually exclusive with systemmemory
         * and texturemanage
         */
        pDDSD->ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM;
    }

    Format = PixelFormat_DD2WineD3D(&pDDSD->u4.ddpfPixelFormat);
    if(Format == WINED3DFMT_UNKNOWN)
    {
        ERR("Unsupported / Unknown pixelformat\n");
        return DDERR_INVALIDPIXELFORMAT;
    }

    /* Create the Surface object */
    *ppSurf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawSurfaceImpl));
    if(!*ppSurf)
    {
        ERR("(%p) Error allocating memory for a surface\n", This);
        return DDERR_OUTOFVIDEOMEMORY;
    }
    (*ppSurf)->lpVtbl = &IDirectDrawSurface7_Vtbl;
    (*ppSurf)->IDirectDrawSurface3_vtbl = &IDirectDrawSurface3_Vtbl;
    (*ppSurf)->IDirectDrawGammaControl_vtbl = &IDirectDrawGammaControl_Vtbl;
    (*ppSurf)->IDirect3DTexture2_vtbl = &IDirect3DTexture2_Vtbl;
    (*ppSurf)->IDirect3DTexture_vtbl = &IDirect3DTexture1_Vtbl;
    (*ppSurf)->ref = 1;
    (*ppSurf)->version = 7;
    TRACE("%p->version = %d\n", (*ppSurf), (*ppSurf)->version);
    (*ppSurf)->ddraw = This;
    (*ppSurf)->surface_desc.dwSize = sizeof(DDSURFACEDESC2);
    (*ppSurf)->surface_desc.u4.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    DD_STRUCT_COPY_BYSIZE(&(*ppSurf)->surface_desc, pDDSD);

    /* Surface attachments */
    (*ppSurf)->next_attached = NULL;
    (*ppSurf)->first_attached = *ppSurf;

    /* Needed to re-create the surface on an implementation change */
    (*ppSurf)->ImplType = ImplType;

    /* For D3DDevice creation */
    (*ppSurf)->isRenderTarget = FALSE;

    /* A trace message for debugging */
    TRACE("(%p) Created IDirectDrawSurface implementation structure at %p\n", This, *ppSurf);

    if(pDDSD->ddsCaps.dwCaps & ( DDSCAPS_PRIMARYSURFACE | DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE) )
    {
        /* Render targets and textures need a IParent interface,
         * because WineD3D will destroy them when the swapchain
         * is released
         */
        parImpl = HeapAlloc(GetProcessHeap(), 0, sizeof(IParentImpl));
        if(!parImpl)
        {
            ERR("Out of memory when allocating memory for a IParent implementation\n");
            return DDERR_OUTOFMEMORY;
        }
        parImpl->ref = 1;
        parImpl->lpVtbl = &IParent_Vtbl;
        Parent = (IUnknown *)parImpl;
        TRACE("Using IParent interface %p as parent\n", parImpl);
    }
    else
    {
        /* Use the surface as parent */
        Parent = (IUnknown *)*ppSurf;
        TRACE("Using Surface interface %p as parent\n", *ppSurf);
    }

    /* Now create the WineD3D Surface */
    hr = IWineD3DDevice_CreateSurface(This->wineD3DDevice, pDDSD->dwWidth, pDDSD->dwHeight, Format,
            TRUE /* Lockable */, FALSE /* Discard */, level, &(*ppSurf)->WineD3DSurface, ResType, Usage,
            Pool, WINED3DMULTISAMPLE_NONE, 0 /* MultiSampleQuality */, ImplType, Parent);

    if(hr != D3D_OK)
    {
        ERR("IWineD3DDevice::CreateSurface failed. hr = %08x\n", hr);
        return hr;
    }

    /* Set the child of the parent implementation if it exists */
    if(parImpl)
    {
        parImpl->child = (IUnknown *) (*ppSurf)->WineD3DSurface;
        /* The IParent releases the WineD3DSurface, and
         * the ddraw surface does that too. Hold a reference
         */
        IWineD3DSurface_AddRef((*ppSurf)->WineD3DSurface);
    }

    /* Increase the surface counter, and attach the surface */
    InterlockedIncrement(&This->surfaces);
    list_add_head(&This->surface_list, &(*ppSurf)->surface_list_entry);

    /* Here we could store all created surfaces in the DirectDrawImpl structure,
     * But this could also be delegated to WineDDraw, as it keeps track of all its
     * resources. Not implemented for now, as there are more important things ;)
     */

    /* Get the pixel format of the WineD3DSurface and store it.
     * Don't use the Format choosen above, WineD3D might have
     * changed it
     */
    Desc.Format = &Format;
    Desc.Type = &ResType;
    Desc.Usage = &Usage;
    Desc.Pool = &dummy_d3dpool;
    Desc.Size = &dummy_uint;
    Desc.MultiSampleType = &dummy_mst;
    Desc.MultiSampleQuality = &dummy_dword;
    Desc.Width = &Width;
    Desc.Height = &Height;

    (*ppSurf)->surface_desc.dwFlags |= DDSD_PIXELFORMAT;
    hr = IWineD3DSurface_GetDesc((*ppSurf)->WineD3DSurface, &Desc);
    if(hr != D3D_OK)
    {
        ERR("IWineD3DSurface::GetDesc failed\n");
        IDirectDrawSurface7_Release( (IDirectDrawSurface7 *) *ppSurf);
        return hr;
    }

    if(Format == WINED3DFMT_UNKNOWN)
    {
        FIXME("IWineD3DSurface::GetDesc returned WINED3DFMT_UNKNOWN\n");
    }
    PixelFormat_WineD3DtoDD( &(*ppSurf)->surface_desc.u4.ddpfPixelFormat, Format);

    /* Anno 1602 stores the pitch right after surface creation, so make sure it's there.
     * I can't LockRect() the surface here because if OpenGL surfaces are in use, the
     * WineD3DDevice might not be usable for 3D yet, so an extra method was created.
     * TODO: Test other fourcc formats
     */
    if(Format == WINED3DFMT_DXT1 || Format == WINED3DFMT_DXT2 || Format == WINED3DFMT_DXT3 ||
       Format == WINED3DFMT_DXT4 || Format == WINED3DFMT_DXT5)
    {
        (*ppSurf)->surface_desc.dwFlags |= DDSD_LINEARSIZE;
        if(Format == WINED3DFMT_DXT1)
        {
            (*ppSurf)->surface_desc.u1.dwLinearSize = max(4, Width) * max(4, Height) / 2;
        }
        else
        {
            (*ppSurf)->surface_desc.u1.dwLinearSize = max(4, Width) * max(4, Height);
        }
    }
    else
    {
        (*ppSurf)->surface_desc.dwFlags |= DDSD_PITCH;
        (*ppSurf)->surface_desc.u1.lPitch = IWineD3DSurface_GetPitch((*ppSurf)->WineD3DSurface);
    }

    /* Application passed a color key? Set it! */
    if(pDDSD->dwFlags & DDSD_CKDESTOVERLAY)
    {
        IWineD3DSurface_SetColorKey((*ppSurf)->WineD3DSurface,
                                    DDCKEY_DESTOVERLAY,
                                    (WINEDDCOLORKEY *) &pDDSD->u3.ddckCKDestOverlay);
    }
    if(pDDSD->dwFlags & DDSD_CKDESTBLT)
    {
        IWineD3DSurface_SetColorKey((*ppSurf)->WineD3DSurface,
                                    DDCKEY_DESTBLT,
                                    (WINEDDCOLORKEY *) &pDDSD->ddckCKDestBlt);
    }
    if(pDDSD->dwFlags & DDSD_CKSRCOVERLAY)
    {
        IWineD3DSurface_SetColorKey((*ppSurf)->WineD3DSurface,
                                    DDCKEY_SRCOVERLAY,
                                    (WINEDDCOLORKEY *) &pDDSD->ddckCKSrcOverlay);
    }
    if(pDDSD->dwFlags & DDSD_CKSRCBLT)
    {
        IWineD3DSurface_SetColorKey((*ppSurf)->WineD3DSurface,
                                    DDCKEY_SRCBLT,
                                    (WINEDDCOLORKEY *) &pDDSD->ddckCKSrcBlt);
    }
    if ( pDDSD->dwFlags & DDSD_LPSURFACE)
    {
        hr = IWineD3DSurface_SetMem((*ppSurf)->WineD3DSurface, pDDSD->lpSurface);
        if(hr != WINED3D_OK)
        {
            /* No need for a trace here, wined3d does that for us */
            IDirectDrawSurface7_Release((IDirectDrawSurface7 *)*ppSurf);
            return hr;
        }
    }

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

        hr = IDirectDrawImpl_CreateNewSurface(This,
                                              &DDSD,
                                              &object2,
                                              level);
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
static HRESULT WINAPI
IDirectDrawImpl_CreateSurface(IDirectDraw7 *iface,
                              DDSURFACEDESC2 *DDSD,
                              IDirectDrawSurface7 **Surf,
                              IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    IDirectDrawSurfaceImpl *object = NULL;
    HRESULT hr;
    LONG extra_surfaces = 0;
    DDSURFACEDESC2 desc2;
    WINED3DDISPLAYMODE Mode;
    const DWORD sysvidmem = DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;

    TRACE("(%p)->(%p,%p,%p)\n", This, DDSD, Surf, UnkOuter);

    /* Some checks before we start */
    if (TRACE_ON(ddraw))
    {
        TRACE(" (%p) Requesting surface desc :\n", This);
        DDRAW_dump_surface_desc(DDSD);
    }
    EnterCriticalSection(&ddraw_cs);

    if (UnkOuter != NULL)
    {
        FIXME("(%p) : outer != NULL?\n", This);
        LeaveCriticalSection(&ddraw_cs);
        return CLASS_E_NOAGGREGATION; /* unchecked */
    }

    if (Surf == NULL)
    {
        FIXME("(%p) You want to get back a surface? Don't give NULL ptrs!\n", This);
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
        WARN("(%p) Null surface pointer specified, ignore it!\n", This);
        DDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

    if((DDSD->ddsCaps.dwCaps & (DDSCAPS_FLIP | DDSCAPS_PRIMARYSURFACE)) == (DDSCAPS_FLIP | DDSCAPS_PRIMARYSURFACE) &&
       !(This->cooperative_level & DDSCL_EXCLUSIVE))
    {
        TRACE("(%p): Attempt to create a flipable primary surface without DDSCL_EXCLUSIVE set\n", This);
        *Surf = NULL;
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_NOEXCLUSIVEMODE;
    }

    if(DDSD->ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER)) {
        WARN("Application tried to create an explicit front or back buffer\n");
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
    memset(&desc2, 0, sizeof(desc2));
    desc2.dwSize = sizeof(desc2);   /* For the struct copy */
    DD_STRUCT_COPY_BYSIZE(&desc2, DDSD);
    desc2.dwSize = sizeof(desc2);   /* To override a possibly smaller size */
    desc2.u4.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT); /* Just to be sure */

    /* Get the video mode from WineD3D - we will need it */
    hr = IWineD3DDevice_GetDisplayMode(This->wineD3DDevice,
                                       0, /* Swapchain 0 */
                                       &Mode);
    if(FAILED(hr))
    {
        ERR("Failed to read display mode from wined3d\n");
        switch(This->orig_bpp)
        {
            case 8:
                Mode.Format = WINED3DFMT_P8;
                break;

            case 15:
                Mode.Format = WINED3DFMT_X1R5G5B5;
                break;

            case 16:
                Mode.Format = WINED3DFMT_R5G6B5;
                break;

            case 24:
                Mode.Format = WINED3DFMT_R8G8B8;
                break;

            case 32:
                Mode.Format = WINED3DFMT_X8R8G8B8;
                break;
        }
        Mode.Width = This->orig_width;
        Mode.Height = This->orig_height;
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
                    PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, WINED3DFMT_D15S1);
                    break;
                case 16:
                    PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, WINED3DFMT_D16_UNORM);
                    break;
                case 24:
                    PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, WINED3DFMT_D24X8);
                    break;
                case 32:
                    PixelFormat_WineD3DtoDD(&desc2.u4.ddpfPixelFormat, WINED3DFMT_D32);
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
    hr = IDirectDrawImpl_CreateNewSurface(This, &desc2, &object, 0);
    if( hr != DD_OK)
    {
        ERR("IDirectDrawImpl_CreateNewSurface failed with %08x\n", hr);
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
    }

    hr = DD_OK;
    if(desc2.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
    {
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_ALLFACES;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_NEGATIVEZ;
        hr |= CreateAdditionalSurfaces(This, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_NEGATIVEZ;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_POSITIVEZ;
        hr |= CreateAdditionalSurfaces(This, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_POSITIVEZ;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_NEGATIVEY;
        hr |= CreateAdditionalSurfaces(This, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_NEGATIVEY;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_POSITIVEY;
        hr |= CreateAdditionalSurfaces(This, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_POSITIVEY;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_NEGATIVEX;
        hr |= CreateAdditionalSurfaces(This, object, extra_surfaces + 1, desc2, TRUE);
        desc2.ddsCaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_NEGATIVEX;
        desc2.ddsCaps.dwCaps2 |=  DDSCAPS2_CUBEMAP_POSITIVEX;
    }

    hr |= CreateAdditionalSurfaces(This, object, extra_surfaces, desc2, FALSE);
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
    if( (This->ImplType == SURFACE_OPENGL) && !(This->d3d_initialized) &&
        desc2.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE) )
    {
        IDirectDrawSurfaceImpl *target = object, *surface;
        struct list *entry;

        /* Search for the primary to use as render target */
        LIST_FOR_EACH(entry, &This->surface_list)
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

        TRACE("(%p) Attaching a D3DDevice, rendertarget = %p\n", This, target);
        hr = IDirectDrawImpl_AttachD3DDevice(This, target);
        if(hr != D3D_OK)
        {
            IDirectDrawSurfaceImpl *release_surf;
            ERR("IDirectDrawImpl_AttachD3DDevice failed, hr = %x\n", hr);
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
                IDirectDrawSurfaceImpl_Destroy(release_surf);
            }
            LeaveCriticalSection(&ddraw_cs);
            return hr;
        }
    } else if(!(This->d3d_initialized) && desc2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
        IDirectDrawImpl_CreateGDISwapChain(This, object);
    }

    /* Addref the ddraw interface to keep an reference for each surface */
    IDirectDraw7_AddRef(iface);
    object->ifaceToRelease = (IUnknown *) iface;

    /* Create a WineD3DTexture if a texture was requested */
    if(desc2.ddsCaps.dwCaps & DDSCAPS_TEXTURE)
    {
        UINT levels;
        WINED3DFORMAT Format;
        WINED3DPOOL Pool = WINED3DPOOL_DEFAULT;

        This->tex_root = object;

        if(desc2.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
        {
            /* a mipmap is created, create enough levels */
            levels = desc2.u2.dwMipMapCount;
        }
        else
        {
            /* No mipmap is created, create one level */
            levels = 1;
        }

        /* DDSCAPS_SYSTEMMEMORY textures are in WINED3DPOOL_SYSTEMMEM */
        if(DDSD->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        {
            Pool = WINED3DPOOL_SYSTEMMEM;
        }
        /* Should I forward the MANAGED cap to the managed pool ? */

        /* Get the format. It's set already by CreateNewSurface */
        Format = PixelFormat_DD2WineD3D(&object->surface_desc.u4.ddpfPixelFormat);

        /* The surfaces are already created, the callback only
         * passes the IWineD3DSurface to WineD3D
         */
        if(desc2.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
        {
            hr = IWineD3DDevice_CreateCubeTexture(This->wineD3DDevice, DDSD->dwWidth /* Edgelength */, levels,
                    0 /* usage */, Format, Pool, (IWineD3DCubeTexture **)&object->wineD3DTexture, (IUnknown *)object);
        }
        else
        {
            hr = IWineD3DDevice_CreateTexture(This->wineD3DDevice, DDSD->dwWidth, DDSD->dwHeight, levels,
                    0 /* usage */, Format, Pool, (IWineD3DTexture **)&object->wineD3DTexture, (IUnknown *)object);
        }
        This->tex_root = NULL;
    }

    LeaveCriticalSection(&ddraw_cs);
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

static BOOL
IDirectDrawImpl_DDSD_Match(const DDSURFACEDESC2* requested,
                           const DDSURFACEDESC2* provided)
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
static HRESULT WINAPI
IDirectDrawImpl_EnumSurfaces(IDirectDraw7 *iface,
                             DWORD Flags,
                             DDSURFACEDESC2 *DDSD,
                             void *Context,
                             LPDDENUMSURFACESCALLBACK7 Callback)
{
    /* The surface enumeration is handled by WineDDraw,
     * because it keeps track of all surfaces attached to
     * it. The filtering is done by our callback function,
     * because WineDDraw doesn't handle ddraw-like surface
     * caps structures
     */
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    IDirectDrawSurfaceImpl *surf;
    BOOL all, nomatch;
    DDSURFACEDESC2 desc;
    struct list *entry, *entry2;

    all = Flags & DDENUMSURFACES_ALL;
    nomatch = Flags & DDENUMSURFACES_NOMATCH;

    TRACE("(%p)->(%x,%p,%p,%p)\n", This, Flags, DDSD, Context, Callback);
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
        if (all || (nomatch != IDirectDrawImpl_DDSD_Match(DDSD, &surf->surface_desc)))
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

static HRESULT WINAPI
findRenderTarget(IDirectDrawSurface7 *surface,
                 DDSURFACEDESC2 *desc,
                 void *ctx)
{
    IDirectDrawSurfaceImpl *surf = (IDirectDrawSurfaceImpl *)surface;
    IDirectDrawSurfaceImpl **target = ctx;

    if(!surf->isRenderTarget) {
        *target = surf;
        IDirectDrawSurface7_Release(surface);
        return DDENUMRET_CANCEL;
    }

    /* Recurse into the surface tree */
    IDirectDrawSurface7_EnumAttachedSurfaces(surface, ctx, findRenderTarget);

    IDirectDrawSurface7_Release(surface);
    if(*target) return DDENUMRET_CANCEL;
    else return DDENUMRET_OK; /* Continue with the next neighbor surface */
}

static HRESULT IDirectDrawImpl_CreateGDISwapChain(IDirectDrawImpl *This,
                                                         IDirectDrawSurfaceImpl *primary) {
    HRESULT hr;
    WINED3DPRESENT_PARAMETERS presentation_parameters;
    HWND window;

    window = This->dest_window;

    memset(&presentation_parameters, 0, sizeof(presentation_parameters));

    /* Use the surface description for the device parameters, not the
     * Device settings. The app might render to an offscreen surface
     */
    presentation_parameters.BackBufferWidth                 = primary->surface_desc.dwWidth;
    presentation_parameters.BackBufferHeight                = primary->surface_desc.dwHeight;
    presentation_parameters.BackBufferFormat                = PixelFormat_DD2WineD3D(&primary->surface_desc.u4.ddpfPixelFormat);
    presentation_parameters.BackBufferCount                 = (primary->surface_desc.dwFlags & DDSD_BACKBUFFERCOUNT) ? primary->surface_desc.u5.dwBackBufferCount : 0;
    presentation_parameters.MultiSampleType                 = WINED3DMULTISAMPLE_NONE;
    presentation_parameters.MultiSampleQuality              = 0;
    presentation_parameters.SwapEffect                      = WINED3DSWAPEFFECT_FLIP;
    presentation_parameters.hDeviceWindow                   = window;
    presentation_parameters.Windowed                        = !(This->cooperative_level & DDSCL_FULLSCREEN);
    presentation_parameters.EnableAutoDepthStencil          = FALSE; /* Not on GDI swapchains */
    presentation_parameters.AutoDepthStencilFormat          = 0;
    presentation_parameters.Flags                           = 0;
    presentation_parameters.FullScreen_RefreshRateInHz      = WINED3DPRESENT_RATE_DEFAULT; /* Default rate: It's already set */
    presentation_parameters.PresentationInterval            = WINED3DPRESENT_INTERVAL_DEFAULT;

    This->d3d_target = primary;
    hr = IWineD3DDevice_InitGDI(This->wineD3DDevice, &presentation_parameters);
    This->d3d_target = NULL;

    if (hr != D3D_OK)
    {
        FIXME("(%p) call to IWineD3DDevice_InitGDI failed\n", This);
        primary->wineD3DSwapChain = NULL;
    }
    return hr;
}

/*****************************************************************************
 * IDirectDrawImpl_AttachD3DDevice
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
static HRESULT
IDirectDrawImpl_AttachD3DDevice(IDirectDrawImpl *This,
                                IDirectDrawSurfaceImpl *primary)
{
    HRESULT hr;
    HWND                  window = This->dest_window;

    WINED3DPRESENT_PARAMETERS localParameters;

    TRACE("(%p)->(%p)\n", This, primary);

    /* If there's no window, create a hidden window. WineD3D needs it */
    if(window == 0 || window == GetDesktopWindow())
    {
        window = CreateWindowExA(0, This->classname, "Hidden D3D Window",
                                 WS_DISABLED, 0, 0,
                                 GetSystemMetrics(SM_CXSCREEN),
                                 GetSystemMetrics(SM_CYSCREEN),
                                 NULL, NULL, GetModuleHandleA(0), NULL);

        ShowWindow(window, SW_HIDE);   /* Just to be sure */
        WARN("(%p) No window for the Direct3DDevice, created a hidden window. HWND=%p\n", This, window);
    }
    else
    {
        TRACE("(%p) Using existing window %p for Direct3D rendering\n", This, window);
    }
    This->d3d_window = window;

    /* Store the future Render Target surface */
    This->d3d_target = primary;

    /* Use the surface description for the device parameters, not the
     * Device settings. The app might render to an offscreen surface
     */
    localParameters.BackBufferWidth                 = primary->surface_desc.dwWidth;
    localParameters.BackBufferHeight                = primary->surface_desc.dwHeight;
    localParameters.BackBufferFormat                = PixelFormat_DD2WineD3D(&primary->surface_desc.u4.ddpfPixelFormat);
    localParameters.BackBufferCount                 = (primary->surface_desc.dwFlags & DDSD_BACKBUFFERCOUNT) ? primary->surface_desc.u5.dwBackBufferCount : 0;
    localParameters.MultiSampleType                 = WINED3DMULTISAMPLE_NONE;
    localParameters.MultiSampleQuality              = 0;
    localParameters.SwapEffect                      = WINED3DSWAPEFFECT_COPY;
    localParameters.hDeviceWindow                   = window;
    localParameters.Windowed                        = !(This->cooperative_level & DDSCL_FULLSCREEN);
    localParameters.EnableAutoDepthStencil          = TRUE;
    localParameters.AutoDepthStencilFormat          = WINED3DFMT_D16_UNORM;
    localParameters.Flags                           = 0;
    localParameters.FullScreen_RefreshRateInHz      = WINED3DPRESENT_RATE_DEFAULT; /* Default rate: It's already set */
    localParameters.PresentationInterval            = WINED3DPRESENT_INTERVAL_DEFAULT;

    TRACE("Passing mode %d\n", localParameters.BackBufferFormat);

    /* Set this NOW, otherwise creating the depth stencil surface will cause a
     * recursive loop until ram or emulated video memory is full
     */
    This->d3d_initialized = TRUE;

    hr = IWineD3DDevice_Init3D(This->wineD3DDevice, &localParameters);
    if(FAILED(hr))
    {
        This->d3d_target = NULL;
        This->d3d_initialized = FALSE;
        return hr;
    }

    This->declArraySize = 2;
    This->decls = HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(*This->decls) * This->declArraySize);
    if(!This->decls)
    {
        ERR("Error allocating an array for the converted vertex decls\n");
        This->declArraySize = 0;
        hr = IWineD3DDevice_Uninit3D(This->wineD3DDevice,
                                     D3D7CB_DestroyDepthStencilSurface,
                                     D3D7CB_DestroySwapChain);
        return E_OUTOFMEMORY;
    }

    /* Create an Index Buffer parent */
    TRACE("(%p) Successfully initialized 3D\n", This);
    return DD_OK;
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
    TRACE("(%08x,%p,%p)\n", Flags, Clipper, UnkOuter);

    EnterCriticalSection(&ddraw_cs);
    if (UnkOuter != NULL)
    {
        LeaveCriticalSection(&ddraw_cs);
        return CLASS_E_NOAGGREGATION;
    }

    if (!LoadWineD3D())
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_NODIRECTDRAWSUPPORT;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                     sizeof(IDirectDrawClipperImpl));
    if (object == NULL)
    {
        LeaveCriticalSection(&ddraw_cs);
        return E_OUTOFMEMORY;
    }

    object->lpVtbl = &IDirectDrawClipper_Vtbl;
    object->ref = 1;
    object->wineD3DClipper = pWineDirect3DCreateClipper((IUnknown *) object);
    if(!object->wineD3DClipper)
    {
        HeapFree(GetProcessHeap(), 0, object);
        LeaveCriticalSection(&ddraw_cs);
        return E_OUTOFMEMORY;
    }

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
static HRESULT WINAPI
IDirectDrawImpl_CreateClipper(IDirectDraw7 *iface,
                              DWORD Flags,
                              IDirectDrawClipper **Clipper,
                              IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%x,%p,%p)\n", This, Flags, Clipper, UnkOuter);
    return DirectDrawCreateClipper(Flags, Clipper, UnkOuter);
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
static HRESULT WINAPI
IDirectDrawImpl_CreatePalette(IDirectDraw7 *iface,
                              DWORD Flags,
                              PALETTEENTRY *ColorTable,
                              IDirectDrawPalette **Palette,
                              IUnknown *pUnkOuter)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    IDirectDrawPaletteImpl *object;
    HRESULT hr = DDERR_GENERIC;
    TRACE("(%p)->(%x,%p,%p,%p)\n", This, Flags, ColorTable, Palette, pUnkOuter);

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

    object->lpVtbl = &IDirectDrawPalette_Vtbl;
    object->ref = 1;
    object->ddraw_owner = This;

    hr = IWineD3DDevice_CreatePalette(This->wineD3DDevice, Flags,
            ColorTable, &object->wineD3DPalette, (IUnknown *)object);
    if(hr != DD_OK)
    {
        HeapFree(GetProcessHeap(), 0, object);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    IDirectDraw7_AddRef(iface);
    object->ifaceToRelease = (IUnknown *) iface;
    *Palette = (IDirectDrawPalette *)object;
    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
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
static HRESULT WINAPI
IDirectDrawImpl_DuplicateSurface(IDirectDraw7 *iface,
                                 IDirectDrawSurface7 *Src,
                                 IDirectDrawSurface7 **Dest)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    IDirectDrawSurfaceImpl *Surf = (IDirectDrawSurfaceImpl *)Src;

    FIXME("(%p)->(%p,%p)\n", This, Surf, Dest);

    /* For now, simply create a new, independent surface */
    return IDirectDraw7_CreateSurface(iface,
                                      &Surf->surface_desc,
                                      Dest,
                                      NULL);
}

/*****************************************************************************
 * IDirectDraw7 VTable
 *****************************************************************************/
const IDirectDraw7Vtbl IDirectDraw7_Vtbl =
{
    /*** IUnknown ***/
    IDirectDrawImpl_QueryInterface,
    IDirectDrawImpl_AddRef,
    IDirectDrawImpl_Release,
    /*** IDirectDraw ***/
    IDirectDrawImpl_Compact,
    IDirectDrawImpl_CreateClipper,
    IDirectDrawImpl_CreatePalette,
    IDirectDrawImpl_CreateSurface,
    IDirectDrawImpl_DuplicateSurface,
    IDirectDrawImpl_EnumDisplayModes,
    IDirectDrawImpl_EnumSurfaces,
    IDirectDrawImpl_FlipToGDISurface,
    IDirectDrawImpl_GetCaps,
    IDirectDrawImpl_GetDisplayMode,
    IDirectDrawImpl_GetFourCCCodes,
    IDirectDrawImpl_GetGDISurface,
    IDirectDrawImpl_GetMonitorFrequency,
    IDirectDrawImpl_GetScanLine,
    IDirectDrawImpl_GetVerticalBlankStatus,
    IDirectDrawImpl_Initialize,
    IDirectDrawImpl_RestoreDisplayMode,
    IDirectDrawImpl_SetCooperativeLevel,
    IDirectDrawImpl_SetDisplayMode,
    IDirectDrawImpl_WaitForVerticalBlank,
    /*** IDirectDraw2 ***/
    IDirectDrawImpl_GetAvailableVidMem,
    /*** IDirectDraw3 ***/
    IDirectDrawImpl_GetSurfaceFromDC,
    /*** IDirectDraw4 ***/
    IDirectDrawImpl_RestoreAllSurfaces,
    IDirectDrawImpl_TestCooperativeLevel,
    IDirectDrawImpl_GetDeviceIdentifier,
    /*** IDirectDraw7 ***/
    IDirectDrawImpl_StartModeTest,
    IDirectDrawImpl_EvaluateMode
};

/*****************************************************************************
 * IDirectDrawImpl_FindDecl
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
 *  NULL in case of an error, the IWineD3DVertexDeclaration interface for the
 *  fvf otherwise.
 *
 *****************************************************************************/
IWineD3DVertexDeclaration *
IDirectDrawImpl_FindDecl(IDirectDrawImpl *This,
                         DWORD fvf)
{
    HRESULT hr;
    IWineD3DVertexDeclaration* pDecl = NULL;
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

    hr = IWineD3DDevice_CreateVertexDeclarationFromFVF(This->wineD3DDevice, &pDecl, (IUnknown *)This, fvf);
    if (hr != S_OK) return NULL;

    if(This->declArraySize == This->numConvertedDecls) {
        int grow = max(This->declArraySize / 2, 8);
        convertedDecls = HeapReAlloc(GetProcessHeap(), 0, convertedDecls,
                                     sizeof(convertedDecls[0]) * (This->numConvertedDecls + grow));
        if(!convertedDecls) {
            /* This will destroy it */
            IWineD3DVertexDeclaration_Release(pDecl);
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

/* IWineD3DDeviceParent IUnknown methods */

static inline struct IDirectDrawImpl *ddraw_from_device_parent(IWineD3DDeviceParent *iface)
{
    return (struct IDirectDrawImpl *)((char*)iface - FIELD_OFFSET(struct IDirectDrawImpl, device_parent_vtbl));
}

static HRESULT STDMETHODCALLTYPE device_parent_QueryInterface(IWineD3DDeviceParent *iface, REFIID riid, void **object)
{
    struct IDirectDrawImpl *This = ddraw_from_device_parent(iface);
    return IDirectDrawImpl_QueryInterface((IDirectDraw7 *)This, riid, object);
}

static ULONG STDMETHODCALLTYPE device_parent_AddRef(IWineD3DDeviceParent *iface)
{
    struct IDirectDrawImpl *This = ddraw_from_device_parent(iface);
    return IDirectDrawImpl_AddRef((IDirectDraw7 *)This);
}

static ULONG STDMETHODCALLTYPE device_parent_Release(IWineD3DDeviceParent *iface)
{
    struct IDirectDrawImpl *This = ddraw_from_device_parent(iface);
    return IDirectDrawImpl_Release((IDirectDraw7 *)This);
}

/* IWineD3DDeviceParent methods */

static void STDMETHODCALLTYPE device_parent_WineD3DDeviceCreated(IWineD3DDeviceParent *iface, IWineD3DDevice *device)
{
    TRACE("iface %p, device %p\n", iface, device);
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateSurface(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, WINED3DFORMAT format, DWORD usage,
        WINED3DPOOL pool, UINT level, WINED3DCUBEMAP_FACES face, IWineD3DSurface **surface)
{
    struct IDirectDrawImpl *This = ddraw_from_device_parent(iface);
    IDirectDrawSurfaceImpl *surf = NULL;
    UINT i = 0;
    DDSCAPS2 searchcaps = This->tex_root->surface_desc.ddsCaps;

    TRACE("iface %p, superior %p, width %u, height %u, format %#x, usage %#x,\n"
            "\tpool %#x, level %u, face %u, surface %p\n",
            iface, superior, width, height, format, usage, pool, level, face, surface);

    searchcaps.dwCaps2 &= ~DDSCAPS2_CUBEMAP_ALLFACES;
    switch(face)
    {
        case WINED3DCUBEMAP_FACE_POSITIVE_X:
            TRACE("Asked for positive x\n");
            if (searchcaps.dwCaps2 & DDSCAPS2_CUBEMAP)
            {
                searchcaps.dwCaps2 |= DDSCAPS2_CUBEMAP_POSITIVEX;
            }
            surf = This->tex_root; break;
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
        IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)This->tex_root, &searchcaps, &attached);
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
    *surface = surf->WineD3DSurface;

    TRACE("Returning wineD3DSurface %p, it belongs to surface %p\n", *surface, surf);

    return D3D_OK;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateRenderTarget(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, WINED3DFORMAT format, WINED3DMULTISAMPLE_TYPE multisample_type,
        DWORD multisample_quality, BOOL lockable, IWineD3DSurface **surface)
{
    struct IDirectDrawImpl *This = ddraw_from_device_parent(iface);
    IDirectDrawSurfaceImpl *d3d_surface = This->d3d_target;
    IDirectDrawSurfaceImpl *target = NULL;

    TRACE("iface %p, superior %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, lockable %u, surface %p\n",
            iface, superior, width, height, format, multisample_type, multisample_quality, lockable, surface);

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
        target = This->d3d_target;
        ERR(" (%p) : No DirectDrawSurface found to create the back buffer. Using the front buffer as back buffer. Uncertain consequences\n", This);
    }

    /* TODO: Return failure if the dimensions do not match, but this shouldn't happen */

    *surface = target->WineD3DSurface;
    target->isRenderTarget = TRUE;

    TRACE("Returning wineD3DSurface %p, it belongs to surface %p\n", *surface, d3d_surface);

    return D3D_OK;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateDepthStencilSurface(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, WINED3DFORMAT format, WINED3DMULTISAMPLE_TYPE multisample_type,
        DWORD multisample_quality, BOOL discard, IWineD3DSurface **surface)
{
    struct IDirectDrawImpl *This = ddraw_from_device_parent(iface);
    /* Create a Depth Stencil surface to make WineD3D happy */
    DDSURFACEDESC2 ddsd;
    HRESULT hr;

    TRACE("iface %p, superior %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, discard %u, surface %p\n",
            iface, superior, width, height, format, multisample_type, multisample_quality, discard, surface);

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

    This->depthstencil = TRUE;
    hr = IDirectDraw7_CreateSurface((IDirectDraw7 *)This,
            &ddsd, (IDirectDrawSurface7 **)&This->DepthStencilBuffer, NULL);
    This->depthstencil = FALSE;
    if(FAILED(hr))
    {
        ERR(" (%p) Creating a DepthStencil Surface failed, result = %x\n", This, hr);
        return hr;
    }

    *surface = This->DepthStencilBuffer->WineD3DSurface;

    return D3D_OK;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateVolume(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, UINT depth, WINED3DFORMAT format,
        WINED3DPOOL pool, DWORD usage, IWineD3DVolume **volume)
{
    TRACE("iface %p, superior %p, width %u, height %u, depth %u, format %#x, pool %#x, usage %#x, volume %p\n",
                iface, superior, width, height, depth, format, pool, usage, volume);

    ERR("Not implemented!\n");

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateSwapChain(IWineD3DDeviceParent *iface,
        WINED3DPRESENT_PARAMETERS *present_parameters, IWineD3DSwapChain **swapchain)
{
    struct IDirectDrawImpl *This = ddraw_from_device_parent(iface);
    IDirectDrawSurfaceImpl *iterator;
    IParentImpl *object;
    HRESULT hr;

    TRACE("iface %p, present_parameters %p, swapchain %p\n", iface, present_parameters, swapchain);

    object = HeapAlloc(GetProcessHeap(),  HEAP_ZERO_MEMORY, sizeof(IParentImpl));
    if (!object)
    {
        FIXME("Allocation of memory failed\n");
        *swapchain = NULL;
        return DDERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &IParent_Vtbl;
    object->ref = 1;

    hr = IWineD3DDevice_CreateSwapChain(This->wineD3DDevice, present_parameters,
            swapchain, (IUnknown *)object, This->ImplType);
    if (FAILED(hr))
    {
        FIXME("(%p) CreateSwapChain failed, returning %#x\n", iface, hr);
        HeapFree(GetProcessHeap(), 0 , object);
        *swapchain = NULL;
        return hr;
    }

    object->child = (IUnknown *)*swapchain;
    This->d3d_target->wineD3DSwapChain = *swapchain;
    iterator = This->d3d_target->complex_array[0];
    while (iterator)
    {
        iterator->wineD3DSwapChain = *swapchain;
        iterator = iterator->complex_array[0];
    }

    return hr;
}

const IWineD3DDeviceParentVtbl ddraw_wined3d_device_parent_vtbl =
{
    /* IUnknown methods */
    device_parent_QueryInterface,
    device_parent_AddRef,
    device_parent_Release,
    /* IWineD3DDeviceParent methods */
    device_parent_WineD3DDeviceCreated,
    device_parent_CreateSurface,
    device_parent_CreateRenderTarget,
    device_parent_CreateDepthStencilSurface,
    device_parent_CreateVolume,
    device_parent_CreateSwapChain,
};
