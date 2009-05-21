/*
 * Copyright (c) 2006 Stefan Dösinger
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
#include "wine/debug.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "wine/exception.h"

#include "ddraw.h"
#include "d3d.h"

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d7);

/*****************************************************************************
 * IDirect3D7::QueryInterface
 *
 * QueryInterface implementation with thunks to IDirectDraw7
 *
 *****************************************************************************/
static HRESULT WINAPI
Thunk_IDirect3DImpl_7_QueryInterface(IDirect3D7 *iface,
                                    REFIID refiid,
                                    void **obj)
{
    IDirectDrawImpl *This = ddraw_from_d3d7(iface);
    TRACE("(%p)->(%s,%p): Thunking to IDirectDraw7\n", This, debugstr_guid(refiid), obj);

    return IDirectDraw7_QueryInterface((IDirectDraw7 *)This, refiid, obj);
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_3_QueryInterface(IDirect3D3 *iface,
                                    REFIID refiid,
                                    void **obj)
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    TRACE("(%p)->(%s,%p): Thunking to IDirectDraw7\n", This, debugstr_guid(refiid), obj);

    return IDirectDraw7_QueryInterface((IDirectDraw7 *)This, refiid, obj);
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_2_QueryInterface(IDirect3D2 *iface,
                                    REFIID refiid,
                                    void **obj)
{
    IDirectDrawImpl *This = ddraw_from_d3d2(iface);
    TRACE("(%p)->(%s,%p): Thunking to IDirectDraw7\n", This, debugstr_guid(refiid), obj);

    return IDirectDraw7_QueryInterface((IDirectDraw7 *)This, refiid, obj);
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_1_QueryInterface(IDirect3D *iface,
                                    REFIID refiid,
                                    void **obj)
{
    IDirectDrawImpl *This = ddraw_from_d3d1(iface);
    TRACE("(%p)->(%s,%p): Thunking to IDirectDraw7\n", This, debugstr_guid(refiid), obj);

    return IDirectDraw7_QueryInterface((IDirectDraw7 *)This, refiid, obj);
}

/*****************************************************************************
 * IDirect3D7::AddRef
 *
 * DirectDraw refcounting is a bit odd. Every version of the ddraw interface
 * has its own refcount, but IDirect3D 1/2/3 refcounts are linked to
 * IDirectDraw, and IDirect3D7 is linked to IDirectDraw7
 *
 * IDirect3D7 -> IDirectDraw7
 * IDirect3D3 -> IDirectDraw
 * IDirect3D2 -> IDirectDraw
 * IDirect3D  -> IDirectDraw
 *
 * So every AddRef implementation thunks to a different interface, and the
 * IDirectDrawX::AddRef implementations have different counters...
 *
 * Returns
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
Thunk_IDirect3DImpl_7_AddRef(IDirect3D7 *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d7(iface);
    TRACE("(%p) : Thunking to IDirectDraw7.\n", This);

    return IDirectDraw7_AddRef((IDirectDraw7 *)This);
}

static ULONG WINAPI
Thunk_IDirect3DImpl_3_AddRef(IDirect3D3 *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    TRACE("(%p) : Thunking to IDirectDraw.\n", This);

    return IDirectDraw_AddRef((IDirectDraw *)&This->IDirectDraw_vtbl);
}

static ULONG WINAPI
Thunk_IDirect3DImpl_2_AddRef(IDirect3D2 *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d2(iface);
    TRACE("(%p) : Thunking to IDirectDraw.\n", This);

    return IDirectDraw_AddRef((IDirectDraw *)&This->IDirectDraw_vtbl);
}

static ULONG WINAPI
Thunk_IDirect3DImpl_1_AddRef(IDirect3D *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d1(iface);
    TRACE("(%p) : Thunking to IDirectDraw.\n", This);

    return IDirectDraw_AddRef((IDirectDraw *)&This->IDirectDraw_vtbl);
}

/*****************************************************************************
 * IDirect3D7::Release
 *
 * Same story as IDirect3D7::AddRef
 *
 * Returns: The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
Thunk_IDirect3DImpl_7_Release(IDirect3D7 *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d7(iface);
    TRACE("(%p) : Thunking to IDirectDraw7.\n", This);

    return IDirectDraw7_Release((IDirectDraw7 *)This);
}

static ULONG WINAPI
Thunk_IDirect3DImpl_3_Release(IDirect3D3 *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    TRACE("(%p) : Thunking to IDirectDraw.\n", This);

    return IDirectDraw_Release((IDirectDraw *)&This->IDirectDraw_vtbl);
}

static ULONG WINAPI
Thunk_IDirect3DImpl_2_Release(IDirect3D2 *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d2(iface);
    TRACE("(%p) : Thunking to IDirectDraw.\n", This);

    return IDirectDraw_Release((IDirectDraw *)&This->IDirectDraw_vtbl);
}

static ULONG WINAPI
Thunk_IDirect3DImpl_1_Release(IDirect3D *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d1(iface);
    TRACE("(%p) : Thunking to IDirectDraw.\n", This);

    return IDirectDraw_Release((IDirectDraw *)&This->IDirectDraw_vtbl);
}

/*****************************************************************************
 * IDirect3D Methods
 *****************************************************************************/

/*****************************************************************************
 * IDirect3D::Initialize
 *
 * Initializes the IDirect3D interface. This is a no-op implementation,
 * as all initialization is done at create time.
 *
 * Version 1
 *
 * Params:
 *  refiid: ?
 *
 * Returns:
 *  D3D_OK, because it's a no-op
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DImpl_1_Initialize(IDirect3D *iface,
                           REFIID refiid)
{
    IDirectDrawImpl *This = ddraw_from_d3d1(iface);

    TRACE("(%p)->(%s) no-op...\n", This, debugstr_guid(refiid));
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3D7::EnumDevices
 *
 * The EnumDevices method for IDirect3D7. It enumerates all supported
 * D3D7 devices. Currently the T&L, HAL and RGB devices are enumerated.
 *
 * Params:
 *  Callback: Function to call for each enumerated device
 *  Context: Pointer to pass back to the app
 *
 * Returns:
 *  D3D_OK, or the return value of the GetCaps call
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DImpl_7_EnumDevices(IDirect3D7 *iface,
                          LPD3DENUMDEVICESCALLBACK7 Callback,
                          void *Context)
{
    IDirectDrawImpl *This = ddraw_from_d3d7(iface);
    char interface_name_tnl[] = "WINE Direct3D7 Hardware Transform and Lighting acceleration using WineD3D";
    char device_name_tnl[] = "Wine D3D7 T&L HAL";
    char interface_name_hal[] = "WINE Direct3D7 Hardware acceleration using WineD3D";
    char device_name_hal[] = "Wine D3D7 HAL";
    char interface_name_rgb[] = "WINE Direct3D7 RGB Software Emulation using WineD3D";
    char device_name_rgb[] = "Wine D3D7 RGB";
    D3DDEVICEDESC7 ddesc;
    D3DDEVICEDESC oldDesc;
    HRESULT hr;

    TRACE("(%p)->(%p,%p)\n", This, Callback, Context);
    EnterCriticalSection(&ddraw_cs);

    TRACE("(%p) Enumerating WineD3D D3Device7 interface\n", This);
    hr = IDirect3DImpl_GetCaps(This->wineD3D, &oldDesc, &ddesc);
    if(hr != D3D_OK)
    {
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }
    Callback(interface_name_tnl, device_name_tnl, &ddesc, Context);

    ddesc.deviceGUID = IID_IDirect3DHALDevice;
    Callback(interface_name_hal, device_name_hal, &ddesc, Context);

    ddesc.deviceGUID = IID_IDirect3DRGBDevice;
    Callback(interface_name_rgb, device_name_rgb, &ddesc, Context);

    TRACE("(%p) End of enumeration\n", This);
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
 *  Callback: Application-provided routine to call for each enumerated device
 *  Context: Pointer to pass to the callback
 *
 * Returns:
 *  D3D_OK on success,
 *  The result of IDirect3DImpl_GetCaps if it failed
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DImpl_3_EnumDevices(IDirect3D3 *iface,
                            LPD3DENUMDEVICESCALLBACK Callback,
                            void *Context)
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    D3DDEVICEDESC dref, d1, d2;
    D3DDEVICEDESC7 newDesc;
    static CHAR wined3d_description[] = "Wine D3DDevice using WineD3D and OpenGL";
    HRESULT hr;

    /* Some games (Motoracer 2 demo) have the bad idea to modify the device name string.
       Let's put the string in a sufficiently sized array in writable memory. */
    char device_name[50];
    strcpy(device_name,"Direct3D HEL");

    TRACE("(%p)->(%p,%p)\n", This, Callback, Context);
    EnterCriticalSection(&ddraw_cs);

    hr = IDirect3DImpl_GetCaps(This->wineD3D, &dref, &newDesc);
    if(hr != D3D_OK)
    {
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    /* Do I have to enumerate the reference id? Note from old d3d7:
     * "It seems that enumerating the reference IID on Direct3D 1 games
     * (AvP / Motoracer2) breaks them". So do not enumerate this iid in V1
     *
     * There's a registry key HKLM\Software\Microsoft\Direct3D\Drivers, EnumReference
     * which enables / disables enumerating the reference rasterizer. It's a DWORD,
     * 0 means disabled, 2 means enabled. The enablerefrast.reg and disablerefrast.reg
     * files in the DirectX 7.0 sdk demo directory suggest this.
     *
     * Some games(GTA 2) seem to use the second enumerated device, so I have to enumerate
     * at least 2 devices. So enumerate the reference device to have 2 devices.
     *
     * Other games(Rollcage) tell emulation and hal device apart by certain flags.
     * Rollcage expects D3DPTEXTURECAPS_POW2 to be set(yeah, it is a limitation flag),
     * and it refuses all devices that have the perspective flag set. This way it refuses
     * the emulation device, and HAL devices never have POW2 unset in d3d7 on windows.
     */

    if(This->d3dversion != 1)
    {
        static CHAR reference_description[] = "RGB Direct3D emulation";

        TRACE("(%p) Enumerating WineD3D D3DDevice interface\n", This);
        d1 = dref;
        d2 = dref;
        /* The rgb device has the pow2 flag set in the hel caps, but not in the hal caps */
        d1.dpcLineCaps.dwTextureCaps &= ~(D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PERSPECTIVE);
        d1.dpcTriCaps.dwTextureCaps &= ~(D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PERSPECTIVE);
        hr = Callback( (LPIID) &IID_IDirect3DRGBDevice, reference_description, device_name, &d1, &d2, Context);
        if(hr != D3DENUMRET_OK)
        {
            TRACE("Application cancelled the enumeration\n");
            LeaveCriticalSection(&ddraw_cs);
            return D3D_OK;
        }
    }

    strcpy(device_name,"Direct3D HAL");

    TRACE("(%p) Enumerating HAL Direct3D device\n", This);
    d1 = dref;
    d2 = dref;
    /* The hal device does not have the pow2 flag set in hel, but in hal */
    d2.dpcLineCaps.dwTextureCaps &= ~(D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PERSPECTIVE);
    d2.dpcTriCaps.dwTextureCaps &= ~(D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PERSPECTIVE);
    hr = Callback( (LPIID) &IID_IDirect3DHALDevice, wined3d_description, device_name, &d1, &d2, Context);
    if(hr != D3DENUMRET_OK)
    {
        TRACE("Application cancelled the enumeration\n");
        LeaveCriticalSection(&ddraw_cs);
        return D3D_OK;
    }
    TRACE("(%p) End of enumeration\n", This);

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_2_EnumDevices(IDirect3D2 *iface,
                                  LPD3DENUMDEVICESCALLBACK Callback,
                                  void *Context)
{
    IDirectDrawImpl *This = ddraw_from_d3d2(iface);
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", This, Callback, Context);
    return IDirect3D3_EnumDevices((IDirect3D3 *)&This->IDirect3D3_vtbl, Callback, Context);
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_1_EnumDevices(IDirect3D *iface,
                                  LPD3DENUMDEVICESCALLBACK Callback,
                                  void *Context)
{
    IDirectDrawImpl *This = ddraw_from_d3d1(iface);
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", This, Callback, Context);
    return IDirect3D3_EnumDevices((IDirect3D3 *)&This->IDirect3D3_vtbl, Callback, Context);
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
 *  Light: Address to store the new interface pointer
 *  UnkOuter: Basically for aggregation, but ddraw doesn't support it.
 *            Must be NULL
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if memory allocation failed
 *  CLASS_E_NOAGGREGATION if UnkOuter != NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DImpl_3_CreateLight(IDirect3D3 *iface,
                            IDirect3DLight **Light,
                            IUnknown *UnkOuter )
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    IDirect3DLightImpl *object;

    TRACE("(%p)->(%p,%p)\n", This, Light, UnkOuter);

    if(UnkOuter)
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DLightImpl));
    if (object == NULL)
        return DDERR_OUTOFMEMORY;

    object->lpVtbl = &IDirect3DLight_Vtbl;
    object->ref = 1;
    object->ddraw = This;
    object->next = NULL;
    object->active_viewport = NULL;

    /* Update functions */
    object->activate = light_update;
    object->desactivate = light_activate;
    object->update = light_desactivate;
    object->active_viewport = NULL;

    *Light = (IDirect3DLight *)object;

    TRACE("(%p) creating implementation at %p.\n", This, object);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateLight(IDirect3D2 *iface,
                                  IDirect3DLight **Direct3DLight,
                                  IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = ddraw_from_d3d2(iface);
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", This, Direct3DLight, UnkOuter);
    return IDirect3D3_CreateLight((IDirect3D3 *)&This->IDirect3D3_vtbl, Direct3DLight, UnkOuter);
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateLight(IDirect3D *iface,
                                  IDirect3DLight **Direct3DLight,
                                  IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = ddraw_from_d3d1(iface);
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", This, Direct3DLight, UnkOuter);
    return IDirect3D3_CreateLight((IDirect3D3 *)&This->IDirect3D3_vtbl, Direct3DLight, UnkOuter);
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
 *  Material: Address to store the new interface's pointer to
 *  UnkOuter: Basically for aggregation, but ddraw doesn't support it.
 *            Must be NULL
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if memory allocation failed
 *  CLASS_E_NOAGGREGATION if UnkOuter != NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DImpl_3_CreateMaterial(IDirect3D3 *iface,
                               IDirect3DMaterial3 **Material,
                               IUnknown *UnkOuter )
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    IDirect3DMaterialImpl *object;

    TRACE("(%p)->(%p,%p)\n", This, Material, UnkOuter);

    if(UnkOuter)
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DMaterialImpl));
    if (object == NULL)
        return DDERR_OUTOFMEMORY;

    object->lpVtbl = &IDirect3DMaterial3_Vtbl;
    object->IDirect3DMaterial2_vtbl = &IDirect3DMaterial2_Vtbl;
    object->IDirect3DMaterial_vtbl = &IDirect3DMaterial_Vtbl;
    object->ref = 1;
    object->ddraw = This;
    object->activate = material_activate;

    *Material = (IDirect3DMaterial3 *)object;

    TRACE("(%p) creating implementation at %p.\n", This, object);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateMaterial(IDirect3D2 *iface,
                                     IDirect3DMaterial2 **Direct3DMaterial,
                                     IUnknown* UnkOuter)
{
    IDirectDrawImpl *This = ddraw_from_d3d2(iface);
    HRESULT ret;
    IDirect3DMaterial3 *ret_val;

    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", This, Direct3DMaterial, UnkOuter);
    ret = IDirect3D3_CreateMaterial((IDirect3D3 *)&This->IDirect3D3_vtbl, &ret_val, UnkOuter);

    *Direct3DMaterial = ret_val ?
            (IDirect3DMaterial2 *)&((IDirect3DMaterialImpl *)ret_val)->IDirect3DMaterial2_vtbl : NULL;

    TRACE(" returning interface %p.\n", *Direct3DMaterial);

    return ret;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateMaterial(IDirect3D *iface,
                                     IDirect3DMaterial **Direct3DMaterial,
                                     IUnknown* UnkOuter)
{
    IDirectDrawImpl *This = ddraw_from_d3d1(iface);
    HRESULT ret;
    LPDIRECT3DMATERIAL3 ret_val;

    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", This, Direct3DMaterial, UnkOuter);
    ret = IDirect3D3_CreateMaterial((IDirect3D3 *)&This->IDirect3D3_vtbl, &ret_val, UnkOuter);

    *Direct3DMaterial = ret_val ?
            (IDirect3DMaterial *)&((IDirect3DMaterialImpl *)ret_val)->IDirect3DMaterial_vtbl : NULL;

    TRACE(" returning interface %p.\n", *Direct3DMaterial);

    return ret;
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
 *  UnkOuter: Basically for aggregation, but ddraw doesn't support it.
 *            Must be NULL
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if memory allocation failed
 *  CLASS_E_NOAGGREGATION if UnkOuter != NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DImpl_3_CreateViewport(IDirect3D3 *iface,
                              IDirect3DViewport3 **Viewport,
                              IUnknown *UnkOuter )
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    IDirect3DViewportImpl *object;

    if(UnkOuter)
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DViewportImpl));
    if (object == NULL)
        return DDERR_OUTOFMEMORY;

    object->lpVtbl = &IDirect3DViewport3_Vtbl;
    object->ref = 1;
    object->ddraw = This;
    object->activate = viewport_activate;
    object->use_vp2 = 0xFF;
    object->next = NULL;
    object->lights = NULL;
    object->num_lights = 0;
    object->map_lights = 0;

    *Viewport = (IDirect3DViewport3 *)object;

    TRACE("(%p) creating implementation at %p.\n",This, object);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateViewport(IDirect3D2 *iface,
                                     IDirect3DViewport2 **D3DViewport2,
                                     IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = ddraw_from_d3d2(iface);
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", This, D3DViewport2, UnkOuter);

    return IDirect3D3_CreateViewport((IDirect3D3 *)&This->IDirect3D3_vtbl,
                                     (IDirect3DViewport3 **) D3DViewport2 /* No need to cast here */,
                                     UnkOuter);
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateViewport(IDirect3D *iface,
                                     IDirect3DViewport **D3DViewport,
                                     IUnknown* UnkOuter)
{
    IDirectDrawImpl *This = ddraw_from_d3d1(iface);
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", This, D3DViewport, UnkOuter);

    return IDirect3D3_CreateViewport((IDirect3D3 *)&This->IDirect3D3_vtbl,
                                     (IDirect3DViewport3 **) D3DViewport /* No need to cast here */,
                                     UnkOuter);
}

/*****************************************************************************
 * IDirect3D3::FindDevice
 *
 * This method finds a device with the requested properties and returns a
 * device description
 *
 * Verion 1, 2 and 3
 * Params:
 *  D3DDFS: Describes the requested device characteristics
 *  D3DFDR: Returns the device description
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if no device was found
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DImpl_3_FindDevice(IDirect3D3 *iface,
                           D3DFINDDEVICESEARCH *D3DDFS,
                           D3DFINDDEVICERESULT *D3DFDR)
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    D3DDEVICEDESC desc;
    D3DDEVICEDESC7 newDesc;
    HRESULT hr;

    TRACE("(%p)->(%p,%p)\n", This, D3DDFS, D3DFDR);

    if ((D3DDFS->dwFlags & D3DFDS_COLORMODEL) &&
        (D3DDFS->dcmColorModel != D3DCOLOR_RGB))
    {
        TRACE(" trying to request a non-RGB D3D color model. Not supported.\n");
        return DDERR_INVALIDPARAMS; /* No real idea what to return here :-) */
    }
    if (D3DDFS->dwFlags & D3DFDS_GUID)
    {
        TRACE(" trying to match guid %s.\n", debugstr_guid(&(D3DDFS->guid)));
        if ((IsEqualGUID(&IID_D3DDEVICE_WineD3D, &(D3DDFS->guid)) == 0) &&
            (IsEqualGUID(&IID_IDirect3DHALDevice, &(D3DDFS->guid)) == 0) &&
            (IsEqualGUID(&IID_IDirect3DRefDevice, &(D3DDFS->guid)) == 0))
        {
            TRACE(" no match for this GUID.\n");
            return DDERR_INVALIDPARAMS;
        }
    }

    /* Get the caps */
    hr = IDirect3DImpl_GetCaps(This->wineD3D, &desc, &newDesc);
    if(hr != D3D_OK) return hr;

    /* Now return our own GUID */
    D3DFDR->guid = IID_D3DDEVICE_WineD3D;
    D3DFDR->ddHwDesc = desc;
    D3DFDR->ddSwDesc = desc;

    TRACE(" returning Wine's WineD3D device with (undumped) capabilities\n");

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_2_FindDevice(IDirect3D2 *iface,
                                 D3DFINDDEVICESEARCH *D3DDFS,
                                 D3DFINDDEVICERESULT *D3DFDR)
{
    IDirectDrawImpl *This = ddraw_from_d3d2(iface);
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", iface, D3DDFS, D3DFDR);
    return IDirect3D3_FindDevice((IDirect3D3 *)&This->IDirect3D3_vtbl, D3DDFS, D3DFDR);
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_1_FindDevice(IDirect3D *iface,
                                D3DFINDDEVICESEARCH *D3DDFS,
                                D3DFINDDEVICERESULT *D3DDevice)
{
    IDirectDrawImpl *This = ddraw_from_d3d1(iface);
    TRACE("(%p)->(%p,%p) thunking to IDirect3D3 interface.\n", This, D3DDFS, D3DDevice);
    return IDirect3D3_FindDevice((IDirect3D3 *)&This->IDirect3D3_vtbl, D3DDFS, D3DDevice);
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
static HRESULT WINAPI
IDirect3DImpl_7_CreateDevice(IDirect3D7 *iface,
                             REFCLSID refiid,
                             IDirectDrawSurface7 *Surface,
                             IDirect3DDevice7 **Device)
{
    IDirectDrawImpl *This = ddraw_from_d3d7(iface);
    IDirect3DDeviceImpl *object;
    IParentImpl *IndexBufferParent;
    HRESULT hr;
    IDirectDrawSurfaceImpl *target = (IDirectDrawSurfaceImpl *)Surface;
    TRACE("(%p)->(%s,%p,%p)\n", iface, debugstr_guid(refiid), Surface, Device);

    EnterCriticalSection(&ddraw_cs);
    *Device = NULL;

    /* Fail device creation if non-opengl surfaces are used */
    if(This->ImplType != SURFACE_OPENGL)
    {
        ERR("The application wants to create a Direct3D device, but non-opengl surfaces are set in the registry. Please set the surface implementation to opengl or autodetection to allow 3D rendering\n");

        /* We only hit this path if a default surface is set in the registry. Incorrect autodetection
         * is caught in CreateSurface or QueryInterface
         */
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_NO3D;
    }

    /* So far we can only create one device per ddraw object */
    if(This->d3ddevice)
    {
        FIXME("(%p): Only one Direct3D device per DirectDraw object supported\n", This);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(IDirect3DDeviceImpl));
    if(!object)
    {
        ERR("Out of memory when allocating a IDirect3DDevice implementation\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_OUTOFMEMORY;
    }

    if (This->cooperative_level & DDSCL_FPUPRESERVE)
        object->lpVtbl = &IDirect3DDevice7_FPUPreserve_Vtbl;
    else
        object->lpVtbl = &IDirect3DDevice7_FPUSetup_Vtbl;

    object->IDirect3DDevice3_vtbl = &IDirect3DDevice3_Vtbl;
    object->IDirect3DDevice2_vtbl = &IDirect3DDevice2_Vtbl;
    object->IDirect3DDevice_vtbl = &IDirect3DDevice1_Vtbl;
    object->ref = 1;
    object->ddraw = This;
    object->viewport_list = NULL;
    object->current_viewport = NULL;
    object->material = 0;
    object->target = target;

    object->Handles = NULL;
    object->numHandles = 0;

    object->legacyTextureBlending = FALSE;

    /* This is for convenience */
    object->wineD3DDevice = This->wineD3DDevice;

    /* Create an index buffer, it's needed for indexed drawing */
    IndexBufferParent = HeapAlloc(GetProcessHeap(), 0, sizeof(IParentImpl));
    if(!IndexBufferParent)
    {
        ERR("Allocating memory for an index buffer parent failed\n");
        HeapFree(GetProcessHeap(), 0, object);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_OUTOFMEMORY;
    }
    IndexBufferParent->lpVtbl = &IParent_Vtbl;
    IndexBufferParent->ref = 1;

    /* Create an Index Buffer. WineD3D needs one for Drawing indexed primitives
     * Create a (hopefully) long enough buffer, and copy the indices into it
     * Ideally, a IWineD3DBuffer::SetData method could be created, which
     * takes the pointer and avoids the memcpy
     */
    hr = IWineD3DDevice_CreateIndexBuffer(This->wineD3DDevice, 0x40000 /* Length. Don't know how long it should be */,
            WINED3DUSAGE_DYNAMIC /* Usage */, WINED3DPOOL_DEFAULT, &object->indexbuffer, (IUnknown *)IndexBufferParent);

    if(FAILED(hr))
    {
        ERR("Failed to create an index buffer\n");
        HeapFree(GetProcessHeap(), 0, object);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }
    IndexBufferParent->child = (IUnknown *) object->indexbuffer;

    /* No need to set the indices, it's done when necessary */

    /* AddRef the WineD3D Device */
    IWineD3DDevice_AddRef(This->wineD3DDevice);

    /* Don't forget to return the interface ;) */
    *Device = (IDirect3DDevice7 *)object;

    TRACE(" (%p) Created an IDirect3DDeviceImpl object at %p\n", This, object);

    /* This is for apps which create a non-flip, non-d3d primary surface
     * and an offscreen D3DDEVICE surface, then render to the offscreen surface
     * and do a Blt from the offscreen to the primary surface.
     *
     * Set the offscreen D3DDDEVICE surface(=target) as the back buffer,
     * and the primary surface(=This->d3d_target) as the front buffer.
     *
     * This way the app will render to the D3DDEVICE surface and WineD3D
     * will catch the Blt was Back Buffer -> Front buffer blt and perform
     * a flip instead. This way we don't have to deal with a mixed GL / GDI
     * environment.
     *
     * This should be checked against windowed apps. The only app tested with
     * this is moto racer 2 during the loading screen.
     */
    TRACE("Isrendertarget: %s, d3d_target=%p\n", target->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ? "true" : "false", This->d3d_target);
    if(!(target->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
       (This->d3d_target != target))
    {
        WINED3DVIEWPORT vp;
        TRACE("(%p) Using %p as front buffer, %p as back buffer\n", This, This->d3d_target, target);
        hr = IWineD3DDevice_SetFrontBackBuffers(This->wineD3DDevice,
                                                This->d3d_target->WineD3DSurface,
                                                target->WineD3DSurface);
        if(hr != D3D_OK)
            ERR("(%p) Error %08x setting the front and back buffer\n", This, hr);

        /* Render to the back buffer */
        IWineD3DDevice_SetRenderTarget(This->wineD3DDevice, 0,
                                       target->WineD3DSurface);

        vp.X = 0;
        vp.Y = 0;
        vp.Width = target->surface_desc.dwWidth;
        vp.Height = target->surface_desc.dwHeight;
        vp.MinZ = 0.0;
        vp.MaxZ = 1.0;
        IWineD3DDevice_SetViewport(This->wineD3DDevice,
                                   &vp);

        object->OffScreenTarget = TRUE;
    }
    else
    {
        object->OffScreenTarget = FALSE;
    }

    /* AddRef the render target. Also AddRef the render target from ddraw,
     * because if it is released before the app releases the D3D device, the D3D capabilities
     * of WineD3D will be uninitialized, which has bad effects.
     *
     * In most cases, those surfaces are the surfaces are the same anyway, but this will simply
     * add another ref which is released when the device is destroyed.
     */
    IDirectDrawSurface7_AddRef(Surface);
    IDirectDrawSurface7_AddRef((IDirectDrawSurface7 *)This->d3d_target);

    This->d3ddevice = object;

    IWineD3DDevice_SetRenderState(This->wineD3DDevice,
                                  WINED3DRS_ZENABLE,
                                  IDirect3DDeviceImpl_UpdateDepthStencil(object));
    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_3_CreateDevice(IDirect3D3 *iface,
                                   REFCLSID refiid,
                                   IDirectDrawSurface4 *Surface,
                                   IDirect3DDevice3 **Device,
                                   IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    HRESULT hr;
    TRACE("(%p)->(%s,%p,%p,%p): Thunking to IDirect3D7\n", This, debugstr_guid(refiid), Surface, Device, UnkOuter);

    if(UnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    hr =  IDirect3D7_CreateDevice((IDirect3D7 *)&This->IDirect3D7_vtbl, refiid,
            (IDirectDrawSurface7 *)Surface /* Same VTables */, (IDirect3DDevice7 **)Device);

    *Device = *Device ? (IDirect3DDevice3 *)&((IDirect3DDeviceImpl *)*Device)->IDirect3DDevice3_vtbl : NULL;
    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateDevice(IDirect3D2 *iface,
                                   REFCLSID refiid,
                                   IDirectDrawSurface *Surface,
                                   IDirect3DDevice2 **Device)
{
    IDirectDrawImpl *This = ddraw_from_d3d2(iface);
    HRESULT hr;
    TRACE("(%p)->(%s,%p,%p): Thunking to IDirect3D7\n", This, debugstr_guid(refiid), Surface, Device);

    hr =  IDirect3D7_CreateDevice((IDirect3D7 *)&This->IDirect3D7_vtbl, refiid,
            Surface ? (IDirectDrawSurface7 *)surface_from_surface3((IDirectDrawSurface3 *)Surface) : NULL,
            (IDirect3DDevice7 **)Device);

    *Device = *Device ? (IDirect3DDevice2 *)&((IDirect3DDeviceImpl *)*Device)->IDirect3DDevice2_vtbl : NULL;
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
 *  Desc: Requested Vertex buffer properties
 *  VertexBuffer: Address to return the interface pointer at
 *  Flags: Some flags, must be 0
 *
 * Returns
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if memory allocation failed
 *  The return value of IWineD3DDevice::CreateVertexBuffer if this call fails
 *  DDERR_INVALIDPARAMS if Desc or VertexBuffer are NULL, or Flags != 0
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DImpl_7_CreateVertexBuffer(IDirect3D7 *iface,
                                   D3DVERTEXBUFFERDESC *Desc,
                                   IDirect3DVertexBuffer7 **VertexBuffer,
                                   DWORD Flags)
{
    IDirectDrawImpl *This = ddraw_from_d3d7(iface);
    IDirect3DVertexBufferImpl *object;
    HRESULT hr;
    TRACE("(%p)->(%p,%p,%08x)\n", This, Desc, VertexBuffer, Flags);

    TRACE("(%p) Vertex buffer description:\n", This);
    TRACE("(%p)  dwSize=%d\n", This, Desc->dwSize);
    TRACE("(%p)  dwCaps=%08x\n", This, Desc->dwCaps);
    TRACE("(%p)  FVF=%08x\n", This, Desc->dwFVF);
    TRACE("(%p)  dwNumVertices=%d\n", This, Desc->dwNumVertices);

    /* D3D7 SDK: "No Flags are currently defined for this method. This
     * parameter must be 0"
     *
     * Never trust the documentation - this is wrong
    if(Flags != 0)
    {
        ERR("(%p) Flags is %08lx, returning DDERR_INVALIDPARAMS\n", This, Flags);
        return DDERR_INVALIDPARAMS;
    }
     */

    /* Well, this sounds sane */
    if( (!VertexBuffer) || (!Desc) )
        return DDERR_INVALIDPARAMS;

    /* Now create the vertex buffer */
    object = HeapAlloc(GetProcessHeap(), 0, sizeof(IDirect3DVertexBufferImpl));
    if(!object)
    {
        ERR("(%p) Out of memory when allocating a IDirect3DVertexBufferImpl structure\n", This);
        return DDERR_OUTOFMEMORY;
    }

    object->ref = 1;
    object->lpVtbl = &IDirect3DVertexBuffer7_Vtbl;
    object->IDirect3DVertexBuffer_vtbl = &IDirect3DVertexBuffer1_Vtbl;

    object->Caps = Desc->dwCaps;
    object->ddraw = This;
    object->fvf = Desc->dwFVF;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_CreateVertexBuffer(This->wineD3DDevice,
            get_flexible_vertex_size(Desc->dwFVF) * Desc->dwNumVertices,
            Desc->dwCaps & D3DVBCAPS_WRITEONLY ? WINED3DUSAGE_WRITEONLY : 0, Desc->dwFVF,
            Desc->dwCaps & D3DVBCAPS_SYSTEMMEMORY ? WINED3DPOOL_SYSTEMMEM : WINED3DPOOL_DEFAULT,
            &object->wineD3DVertexBuffer, (IUnknown *)object);
    if(hr != D3D_OK)
    {
        ERR("(%p) IWineD3DDevice::CreateVertexBuffer failed with hr=%08x\n", This, hr);
        HeapFree(GetProcessHeap(), 0, object);
        LeaveCriticalSection(&ddraw_cs);
        if (hr == WINED3DERR_INVALIDCALL)
            return DDERR_INVALIDPARAMS;
        else
            return hr;
    }

    object->wineD3DVertexDeclaration = IDirectDrawImpl_FindDecl(This,
                                                                Desc->dwFVF);
    if(!object->wineD3DVertexDeclaration)
    {
        ERR("Cannot find the vertex declaration for fvf %08x\n", Desc->dwFVF);
        IWineD3DBuffer_Release(object->wineD3DVertexBuffer);
        HeapFree(GetProcessHeap(), 0, object);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }
    IWineD3DVertexDeclaration_AddRef(object->wineD3DVertexDeclaration);

    /* Return the interface */
    *VertexBuffer = (IDirect3DVertexBuffer7 *)object;

    TRACE("(%p) Created new vertex buffer implementation at %p, returning interface at %p\n", This, object, *VertexBuffer);
    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_3_CreateVertexBuffer(IDirect3D3 *iface,
                                         D3DVERTEXBUFFERDESC *Desc,
                                         IDirect3DVertexBuffer **VertexBuffer,
                                         DWORD Flags,
                                         IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    HRESULT hr;
    TRACE("(%p)->(%p,%p,%08x,%p): Relaying to IDirect3D7\n", This, Desc, VertexBuffer, Flags, UnkOuter);

    if(UnkOuter != NULL) return CLASS_E_NOAGGREGATION;

    hr = IDirect3D7_CreateVertexBuffer((IDirect3D7 *)&This->IDirect3D7_vtbl,
            Desc, (IDirect3DVertexBuffer7 **)VertexBuffer, Flags);

    *VertexBuffer = *VertexBuffer ?
            (IDirect3DVertexBuffer *)&((IDirect3DVertexBufferImpl *)*VertexBuffer)->IDirect3DVertexBuffer_vtbl : NULL;

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
 *  refiidDevice:
 *  Callback: Callback to call for each pixel format
 *  Context: Pointer to pass back to the callback
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Callback is NULL
 *  For details, see IWineD3DDevice::EnumZBufferFormats
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DImpl_7_EnumZBufferFormats(IDirect3D7 *iface,
                                   REFCLSID refiidDevice,
                                   LPD3DENUMPIXELFORMATSCALLBACK Callback,
                                   void *Context)
{
    IDirectDrawImpl *This = ddraw_from_d3d7(iface);
    HRESULT hr;
    unsigned int i;
    WINED3DDISPLAYMODE d3ddm;
    WINED3DDEVTYPE type;

    /* Order matters. Specifically, BattleZone II (full version) expects the
     * 16-bit depth formats to be listed before the 24 and 32 ones. */
    WINED3DFORMAT FormatList[] = {
        WINED3DFMT_D15S1,
        WINED3DFMT_D16_UNORM,
        WINED3DFMT_D24X8,
        WINED3DFMT_D24X4S4,
        WINED3DFMT_D24S8,
        WINED3DFMT_D32
    };

    TRACE("(%p)->(%s,%p,%p): Relay\n", iface, debugstr_guid(refiidDevice), Callback, Context);

    if(!Callback)
        return DDERR_INVALIDPARAMS;

    if(IsEqualGUID(refiidDevice, &IID_IDirect3DHALDevice)    ||
       IsEqualGUID(refiidDevice, &IID_IDirect3DTnLHalDevice) ||
       IsEqualGUID(refiidDevice, &IID_D3DDEVICE_WineD3D))
    {
        TRACE("Asked for HAL device\n");
        type = WINED3DDEVTYPE_HAL;
    }
    else if(IsEqualGUID(refiidDevice, &IID_IDirect3DRGBDevice) ||
            IsEqualGUID(refiidDevice, &IID_IDirect3DMMXDevice))
    {
        TRACE("Asked for SW device\n");
        type = WINED3DDEVTYPE_SW;
    }
    else if(IsEqualGUID(refiidDevice, &IID_IDirect3DRefDevice))
    {
        TRACE("Asked for REF device\n");
        type = WINED3DDEVTYPE_REF;
    }
    else if(IsEqualGUID(refiidDevice, &IID_IDirect3DNullDevice))
    {
        TRACE("Asked for NULLREF device\n");
        type = WINED3DDEVTYPE_NULLREF;
    }
    else
    {
        FIXME("Unexpected device GUID %s\n", debugstr_guid(refiidDevice));
        type = WINED3DDEVTYPE_HAL;
    }

    EnterCriticalSection(&ddraw_cs);
    /* We need an adapter format from somewhere to please wined3d and WGL. Use the current display mode.
     * So far all cards offer the same depth stencil format for all modes, but if some do not and apps
     * do not like that we'll have to find some workaround, like iterating over all imaginable formats
     * and collecting all the depth stencil formats we can get
     */
    hr = IWineD3DDevice_GetDisplayMode(This->wineD3DDevice,
                                       0 /* swapchain 0 */,
                                       &d3ddm);

    for(i = 0; i < (sizeof(FormatList) / sizeof(FormatList[0])); i++)
    {
        hr = IWineD3D_CheckDeviceFormat(This->wineD3D,
                                        WINED3DADAPTER_DEFAULT /* Adapter */,
                                        type /* DeviceType */,
                                        d3ddm.Format /* AdapterFormat */,
                                        WINED3DUSAGE_DEPTHSTENCIL /* Usage */,
                                        WINED3DRTYPE_SURFACE,
                                        FormatList[i],
                                        SURFACE_OPENGL);
        if(hr == D3D_OK)
        {
            DDPIXELFORMAT pformat;

            memset(&pformat, 0, sizeof(pformat));
            pformat.dwSize = sizeof(pformat);
            PixelFormat_WineD3DtoDD(&pformat, FormatList[i]);

            TRACE("Enumerating WineD3DFormat %d\n", FormatList[i]);
            hr = Callback(&pformat, Context);
            if(hr != DDENUMRET_OK)
            {
                TRACE("Format enumeration cancelled by application\n");
                LeaveCriticalSection(&ddraw_cs);
                return D3D_OK;
            }
        }
    }
    TRACE("End of enumeration\n");
    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_3_EnumZBufferFormats(IDirect3D3 *iface,
                                         REFCLSID riidDevice,
                                         LPD3DENUMPIXELFORMATSCALLBACK Callback,
                                         void *Context)
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    TRACE("(%p)->(%s,%p,%p) thunking to IDirect3D7 interface.\n", This, debugstr_guid(riidDevice), Callback, Context);
    return IDirect3D7_EnumZBufferFormats((IDirect3D7 *)&This->IDirect3D7_vtbl, riidDevice, Callback, Context);
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
static HRESULT WINAPI
IDirect3DImpl_7_EvictManagedTextures(IDirect3D7 *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d7(iface);
    FIXME("(%p): Stub!\n", This);

    /* Implementation idea:
     * Add an IWineD3DSurface method which sets the opengl texture
     * priority low or even removes the opengl texture.
     */

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DImpl_3_EvictManagedTextures(IDirect3D3 *iface)
{
    IDirectDrawImpl *This = ddraw_from_d3d3(iface);
    TRACE("(%p)->() thunking to IDirect3D7 interface.\n", This);
    return IDirect3D7_EvictManagedTextures((IDirect3D7 *)&This->IDirect3D7_vtbl);
}

/*****************************************************************************
 * IDirect3DImpl_GetCaps
 *
 * This function retrieves the device caps from wined3d
 * and converts it into a D3D7 and D3D - D3D3 structure
 * This is a helper function called from various places in ddraw
 *
 * Params:
 *  WineD3D: The interface to get the caps from
 *  Desc123: Old D3D <3 structure to fill (needed)
 *  Desc7: D3D7 device desc structure to fill (needed)
 *
 * Returns
 *  D3D_OK on success, or the return value of IWineD3D::GetCaps
 *
 *****************************************************************************/
HRESULT
IDirect3DImpl_GetCaps(IWineD3D *WineD3D,
                      D3DDEVICEDESC *Desc123,
                      D3DDEVICEDESC7 *Desc7)
{
    WINED3DCAPS WCaps;
    HRESULT hr;

    /* Some variables to assign to the pointers in WCaps */
    TRACE("()->(%p,%p,%p\n", WineD3D, Desc123, Desc7);

    memset(&WCaps, 0, sizeof(WCaps));
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3D_GetDeviceCaps(WineD3D, 0, WINED3DDEVTYPE_HAL, &WCaps);
    LeaveCriticalSection(&ddraw_cs);
    if(hr != D3D_OK)
    {
        return hr;
    }

    /* Copy the results into the d3d7 and d3d3 structures */
    Desc7->dwDevCaps = WCaps.DevCaps;
    Desc7->dpcLineCaps.dwRasterCaps = WCaps.RasterCaps;
    Desc7->dpcLineCaps.dwZCmpCaps = WCaps.ZCmpCaps;
    Desc7->dpcLineCaps.dwSrcBlendCaps = WCaps.SrcBlendCaps;
    Desc7->dpcLineCaps.dwDestBlendCaps = WCaps.DestBlendCaps;
    Desc7->dpcLineCaps.dwAlphaCmpCaps = WCaps.AlphaCmpCaps;
    Desc7->dpcLineCaps.dwShadeCaps = WCaps.ShadeCaps;
    Desc7->dpcLineCaps.dwTextureCaps = WCaps.TextureCaps;
    Desc7->dpcLineCaps.dwTextureFilterCaps = WCaps.TextureFilterCaps;
    Desc7->dpcLineCaps.dwTextureAddressCaps = WCaps.TextureAddressCaps;

    Desc7->dwMaxTextureWidth = WCaps.MaxTextureWidth;
    Desc7->dwMaxTextureHeight = WCaps.MaxTextureHeight;

    Desc7->dwMaxTextureRepeat = WCaps.MaxTextureRepeat;
    Desc7->dwMaxTextureAspectRatio = WCaps.MaxTextureAspectRatio;
    Desc7->dwMaxAnisotropy = WCaps.MaxAnisotropy;
    Desc7->dvMaxVertexW = WCaps.MaxVertexW;

    Desc7->dvGuardBandLeft = WCaps.GuardBandLeft;
    Desc7->dvGuardBandTop = WCaps.GuardBandTop;
    Desc7->dvGuardBandRight = WCaps.GuardBandRight;
    Desc7->dvGuardBandBottom = WCaps.GuardBandBottom;

    Desc7->dvExtentsAdjust = WCaps.ExtentsAdjust;
    Desc7->dwStencilCaps = WCaps.StencilCaps;

    Desc7->dwFVFCaps = WCaps.FVFCaps;
    Desc7->dwTextureOpCaps = WCaps.TextureOpCaps;

    Desc7->dwVertexProcessingCaps = WCaps.VertexProcessingCaps;
    Desc7->dwMaxActiveLights = WCaps.MaxActiveLights;

    /* Remove all non-d3d7 caps */
    Desc7->dwDevCaps &= (
        D3DDEVCAPS_FLOATTLVERTEX         | D3DDEVCAPS_SORTINCREASINGZ          | D3DDEVCAPS_SORTDECREASINGZ          |
        D3DDEVCAPS_SORTEXACT             | D3DDEVCAPS_EXECUTESYSTEMMEMORY      | D3DDEVCAPS_EXECUTEVIDEOMEMORY       |
        D3DDEVCAPS_TLVERTEXSYSTEMMEMORY  | D3DDEVCAPS_TLVERTEXVIDEOMEMORY      | D3DDEVCAPS_TEXTURESYSTEMMEMORY      |
        D3DDEVCAPS_TEXTUREVIDEOMEMORY    | D3DDEVCAPS_DRAWPRIMTLVERTEX         | D3DDEVCAPS_CANRENDERAFTERFLIP       |
        D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_DRAWPRIMITIVES2          | D3DDEVCAPS_SEPARATETEXTUREMEMORIES  |
        D3DDEVCAPS_DRAWPRIMITIVES2EX     | D3DDEVCAPS_HWTRANSFORMANDLIGHT      | D3DDEVCAPS_CANBLTSYSTONONLOCAL      |
        D3DDEVCAPS_HWRASTERIZATION);

    Desc7->dwStencilCaps &= (
        D3DSTENCILCAPS_KEEP              | D3DSTENCILCAPS_ZERO                 | D3DSTENCILCAPS_REPLACE              |
        D3DSTENCILCAPS_INCRSAT           | D3DSTENCILCAPS_DECRSAT              | D3DSTENCILCAPS_INVERT               |
        D3DSTENCILCAPS_INCR              | D3DSTENCILCAPS_DECR                 | D3DSTENCILCAPS_TWOSIDED);

    /* FVF caps ?*/

    Desc7->dwTextureOpCaps &= (
        D3DTEXOPCAPS_DISABLE             | D3DTEXOPCAPS_SELECTARG1             | D3DTEXOPCAPS_SELECTARG2             |
        D3DTEXOPCAPS_MODULATE            | D3DTEXOPCAPS_MODULATE2X             | D3DTEXOPCAPS_MODULATE4X             |
        D3DTEXOPCAPS_ADD                 | D3DTEXOPCAPS_ADDSIGNED              | D3DTEXOPCAPS_ADDSIGNED2X            |
        D3DTEXOPCAPS_SUBTRACT            | D3DTEXOPCAPS_ADDSMOOTH              | D3DTEXOPCAPS_BLENDTEXTUREALPHA      |
        D3DTEXOPCAPS_BLENDFACTORALPHA    | D3DTEXOPCAPS_BLENDTEXTUREALPHAPM    | D3DTEXOPCAPS_BLENDCURRENTALPHA      |
        D3DTEXOPCAPS_PREMODULATE         | D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR | D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA |
        D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR | D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA | D3DTEXOPCAPS_BUMPENVMAP    |
        D3DTEXOPCAPS_BUMPENVMAPLUMINANCE | D3DTEXOPCAPS_DOTPRODUCT3);

    Desc7->dwVertexProcessingCaps &= (
        D3DVTXPCAPS_TEXGEN               | D3DVTXPCAPS_MATERIALSOURCE7         | D3DVTXPCAPS_VERTEXFOG               |
        D3DVTXPCAPS_DIRECTIONALLIGHTS    | D3DVTXPCAPS_POSITIONALLIGHTS        | D3DVTXPCAPS_LOCALVIEWER);

    Desc7->dpcLineCaps.dwMiscCaps &= (
        D3DPMISCCAPS_MASKPLANES          | D3DPMISCCAPS_MASKZ                  | D3DPMISCCAPS_LINEPATTERNREP         |
        D3DPMISCCAPS_CONFORMANT          | D3DPMISCCAPS_CULLNONE               | D3DPMISCCAPS_CULLCW                 |
        D3DPMISCCAPS_CULLCCW);

    Desc7->dpcLineCaps.dwRasterCaps &= (
        D3DPRASTERCAPS_DITHER            | D3DPRASTERCAPS_ROP2                 | D3DPRASTERCAPS_XOR                  |
        D3DPRASTERCAPS_PAT               | D3DPRASTERCAPS_ZTEST                | D3DPRASTERCAPS_SUBPIXEL             |
        D3DPRASTERCAPS_SUBPIXELX         | D3DPRASTERCAPS_FOGVERTEX            | D3DPRASTERCAPS_FOGTABLE             |
        D3DPRASTERCAPS_STIPPLE           | D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT | D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT |
        D3DPRASTERCAPS_ANTIALIASEDGES    | D3DPRASTERCAPS_MIPMAPLODBIAS        | D3DPRASTERCAPS_ZBIAS                |
        D3DPRASTERCAPS_ZBUFFERLESSHSR    | D3DPRASTERCAPS_FOGRANGE             | D3DPRASTERCAPS_ANISOTROPY           |
        D3DPRASTERCAPS_WBUFFER           | D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT | D3DPRASTERCAPS_WFOG           |
        D3DPRASTERCAPS_ZFOG);

    Desc7->dpcLineCaps.dwZCmpCaps &= (
        D3DPCMPCAPS_NEVER                | D3DPCMPCAPS_LESS                    | D3DPCMPCAPS_EQUAL                   |
        D3DPCMPCAPS_LESSEQUAL            | D3DPCMPCAPS_GREATER                 | D3DPCMPCAPS_NOTEQUAL                |
        D3DPCMPCAPS_GREATEREQUAL         | D3DPCMPCAPS_ALWAYS);

    Desc7->dpcLineCaps.dwSrcBlendCaps &= (
        D3DPBLENDCAPS_ZERO               | D3DPBLENDCAPS_ONE                   | D3DPBLENDCAPS_SRCCOLOR              |
        D3DPBLENDCAPS_INVSRCCOLOR        | D3DPBLENDCAPS_SRCALPHA              | D3DPBLENDCAPS_INVSRCALPHA           |
        D3DPBLENDCAPS_DESTALPHA          | D3DPBLENDCAPS_INVDESTALPHA          | D3DPBLENDCAPS_DESTCOLOR             |
        D3DPBLENDCAPS_INVDESTCOLOR       | D3DPBLENDCAPS_SRCALPHASAT           | D3DPBLENDCAPS_BOTHSRCALPHA          |
        D3DPBLENDCAPS_BOTHINVSRCALPHA);

    Desc7->dpcLineCaps.dwDestBlendCaps &= (
        D3DPBLENDCAPS_ZERO               | D3DPBLENDCAPS_ONE                   | D3DPBLENDCAPS_SRCCOLOR              |
        D3DPBLENDCAPS_INVSRCCOLOR        | D3DPBLENDCAPS_SRCALPHA              | D3DPBLENDCAPS_INVSRCALPHA           |
        D3DPBLENDCAPS_DESTALPHA          | D3DPBLENDCAPS_INVDESTALPHA          | D3DPBLENDCAPS_DESTCOLOR             |
        D3DPBLENDCAPS_INVDESTCOLOR       | D3DPBLENDCAPS_SRCALPHASAT           | D3DPBLENDCAPS_BOTHSRCALPHA          |
        D3DPBLENDCAPS_BOTHINVSRCALPHA);

    Desc7->dpcLineCaps.dwAlphaCmpCaps &= (
        D3DPCMPCAPS_NEVER                | D3DPCMPCAPS_LESS                    | D3DPCMPCAPS_EQUAL                   |
        D3DPCMPCAPS_LESSEQUAL            | D3DPCMPCAPS_GREATER                 | D3DPCMPCAPS_NOTEQUAL                |
        D3DPCMPCAPS_GREATEREQUAL         | D3DPCMPCAPS_ALWAYS);

    Desc7->dpcLineCaps.dwShadeCaps &= (
        D3DPSHADECAPS_COLORFLATMONO      | D3DPSHADECAPS_COLORFLATRGB          | D3DPSHADECAPS_COLORGOURAUDMONO      |
        D3DPSHADECAPS_COLORGOURAUDRGB    | D3DPSHADECAPS_COLORPHONGMONO        | D3DPSHADECAPS_COLORPHONGRGB         |
        D3DPSHADECAPS_SPECULARFLATMONO   | D3DPSHADECAPS_SPECULARFLATRGB       | D3DPSHADECAPS_SPECULARGOURAUDMONO   |
        D3DPSHADECAPS_SPECULARGOURAUDRGB | D3DPSHADECAPS_SPECULARPHONGMONO     | D3DPSHADECAPS_SPECULARPHONGRGB      |
        D3DPSHADECAPS_ALPHAFLATBLEND     | D3DPSHADECAPS_ALPHAFLATSTIPPLED     | D3DPSHADECAPS_ALPHAGOURAUDBLEND     |
        D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED | D3DPSHADECAPS_ALPHAPHONGBLEND     | D3DPSHADECAPS_ALPHAPHONGSTIPPLED    |
        D3DPSHADECAPS_FOGFLAT            | D3DPSHADECAPS_FOGGOURAUD            | D3DPSHADECAPS_FOGPHONG);

    Desc7->dpcLineCaps.dwTextureCaps &= (
        D3DPTEXTURECAPS_PERSPECTIVE      | D3DPTEXTURECAPS_POW2                | D3DPTEXTURECAPS_ALPHA               |
        D3DPTEXTURECAPS_TRANSPARENCY     | D3DPTEXTURECAPS_BORDER              | D3DPTEXTURECAPS_SQUAREONLY          |
        D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE | D3DPTEXTURECAPS_ALPHAPALETTE| D3DPTEXTURECAPS_NONPOW2CONDITIONAL  |
        D3DPTEXTURECAPS_PROJECTED        | D3DPTEXTURECAPS_CUBEMAP             | D3DPTEXTURECAPS_COLORKEYBLEND);

    Desc7->dpcLineCaps.dwTextureFilterCaps &= (
        D3DPTFILTERCAPS_NEAREST          | D3DPTFILTERCAPS_LINEAR              | D3DPTFILTERCAPS_MIPNEAREST          |
        D3DPTFILTERCAPS_MIPLINEAR        | D3DPTFILTERCAPS_LINEARMIPNEAREST    | D3DPTFILTERCAPS_LINEARMIPLINEAR     |
        D3DPTFILTERCAPS_MINFPOINT        | D3DPTFILTERCAPS_MINFLINEAR          | D3DPTFILTERCAPS_MINFANISOTROPIC     |
        D3DPTFILTERCAPS_MIPFPOINT        | D3DPTFILTERCAPS_MIPFLINEAR          | D3DPTFILTERCAPS_MAGFPOINT           |
        D3DPTFILTERCAPS_MAGFLINEAR       | D3DPTFILTERCAPS_MAGFANISOTROPIC     | D3DPTFILTERCAPS_MAGFAFLATCUBIC      |
        D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC);

    Desc7->dpcLineCaps.dwTextureBlendCaps &= (
        D3DPTBLENDCAPS_DECAL             | D3DPTBLENDCAPS_MODULATE             | D3DPTBLENDCAPS_DECALALPHA           |
        D3DPTBLENDCAPS_MODULATEALPHA     | D3DPTBLENDCAPS_DECALMASK            | D3DPTBLENDCAPS_MODULATEMASK         |
        D3DPTBLENDCAPS_COPY              | D3DPTBLENDCAPS_ADD);

    Desc7->dpcLineCaps.dwTextureAddressCaps &= (
        D3DPTADDRESSCAPS_WRAP            | D3DPTADDRESSCAPS_MIRROR             | D3DPTADDRESSCAPS_CLAMP              |
        D3DPTADDRESSCAPS_BORDER          | D3DPTADDRESSCAPS_INDEPENDENTUV);

    if(!(Desc7->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2)) {
        /* DirectX7 always has the np2 flag set, no matter what the card supports. Some old games(rollcage)
         * check the caps incorrectly. If wined3d supports nonpow2 textures it also has np2 conditional support
         */
        Desc7->dpcLineCaps.dwTextureCaps |= D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL;
    }
    /* Fill the missing members, and do some fixup */
    Desc7->dpcLineCaps.dwSize = sizeof(Desc7->dpcLineCaps);
    Desc7->dpcLineCaps.dwTextureBlendCaps = D3DPTBLENDCAPS_ADD | D3DPTBLENDCAPS_MODULATEMASK |
                                            D3DPTBLENDCAPS_COPY | D3DPTBLENDCAPS_DECAL |
                                            D3DPTBLENDCAPS_DECALALPHA | D3DPTBLENDCAPS_DECALMASK |
                                            D3DPTBLENDCAPS_MODULATE | D3DPTBLENDCAPS_MODULATEALPHA;
    Desc7->dpcLineCaps.dwStippleWidth = 32;
    Desc7->dpcLineCaps.dwStippleHeight = 32;
    /* Use the same for the TriCaps */
    Desc7->dpcTriCaps = Desc7->dpcLineCaps;

    Desc7->dwDeviceRenderBitDepth = DDBD_16 | DDBD_24 | DDBD_32;
    Desc7->dwDeviceZBufferBitDepth = DDBD_16 | DDBD_24;
    Desc7->dwMinTextureWidth = 1;
    Desc7->dwMinTextureHeight = 1;

    /* Convert DWORDs safely to WORDs */
    if(WCaps.MaxTextureBlendStages > 65535) Desc7->wMaxTextureBlendStages = 65535;
    else Desc7->wMaxTextureBlendStages = (WORD) WCaps.MaxTextureBlendStages;
    if(WCaps.MaxSimultaneousTextures > 65535) Desc7->wMaxSimultaneousTextures = 65535;
    else Desc7->wMaxSimultaneousTextures = (WORD) WCaps.MaxSimultaneousTextures;

    if(WCaps.MaxUserClipPlanes > 65535) Desc7->wMaxUserClipPlanes = 65535;
    else Desc7->wMaxUserClipPlanes = (WORD) WCaps.MaxUserClipPlanes;
    if(WCaps.MaxVertexBlendMatrices > 65535) Desc7->wMaxVertexBlendMatrices = 65535;
    else Desc7->wMaxVertexBlendMatrices = (WORD) WCaps.MaxVertexBlendMatrices;

    Desc7->deviceGUID = IID_IDirect3DTnLHalDevice;

    Desc7->dwReserved1 = 0;
    Desc7->dwReserved2 = 0;
    Desc7->dwReserved3 = 0;
    Desc7->dwReserved4 = 0;

    /* Fill the old structure */
    memset(Desc123, 0x0, sizeof(D3DDEVICEDESC));
    Desc123->dwSize = sizeof(D3DDEVICEDESC);
    Desc123->dwFlags = D3DDD_COLORMODEL            |
                       D3DDD_DEVCAPS               |
                       D3DDD_TRANSFORMCAPS         |
                       D3DDD_BCLIPPING             |
                       D3DDD_LIGHTINGCAPS          |
                       D3DDD_LINECAPS              |
                       D3DDD_TRICAPS               |
                       D3DDD_DEVICERENDERBITDEPTH  |
                       D3DDD_DEVICEZBUFFERBITDEPTH |
                       D3DDD_MAXBUFFERSIZE         |
                       D3DDD_MAXVERTEXCOUNT;
    Desc123->dcmColorModel = D3DCOLOR_RGB;
    Desc123->dwDevCaps = Desc7->dwDevCaps;
    Desc123->dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
    Desc123->dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP;
    Desc123->bClipping = TRUE;
    Desc123->dlcLightingCaps.dwSize = sizeof(D3DLIGHTINGCAPS);
    Desc123->dlcLightingCaps.dwCaps = D3DLIGHTCAPS_DIRECTIONAL | D3DLIGHTCAPS_PARALLELPOINT | D3DLIGHTCAPS_POINT | D3DLIGHTCAPS_SPOT;
    Desc123->dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
    Desc123->dlcLightingCaps.dwNumLights = Desc7->dwMaxActiveLights;

    Desc123->dpcLineCaps.dwSize = sizeof(D3DPRIMCAPS);
    Desc123->dpcLineCaps.dwMiscCaps = Desc7->dpcLineCaps.dwMiscCaps;
    Desc123->dpcLineCaps.dwRasterCaps = Desc7->dpcLineCaps.dwRasterCaps;
    Desc123->dpcLineCaps.dwZCmpCaps = Desc7->dpcLineCaps.dwZCmpCaps;
    Desc123->dpcLineCaps.dwSrcBlendCaps = Desc7->dpcLineCaps.dwSrcBlendCaps;
    Desc123->dpcLineCaps.dwDestBlendCaps = Desc7->dpcLineCaps.dwDestBlendCaps;
    Desc123->dpcLineCaps.dwShadeCaps = Desc7->dpcLineCaps.dwShadeCaps;
    Desc123->dpcLineCaps.dwTextureCaps = Desc7->dpcLineCaps.dwTextureCaps;
    Desc123->dpcLineCaps.dwTextureFilterCaps = Desc7->dpcLineCaps.dwTextureFilterCaps;
    Desc123->dpcLineCaps.dwTextureBlendCaps = Desc7->dpcLineCaps.dwTextureBlendCaps;
    Desc123->dpcLineCaps.dwTextureAddressCaps = Desc7->dpcLineCaps.dwTextureAddressCaps;
    Desc123->dpcLineCaps.dwStippleWidth = Desc7->dpcLineCaps.dwStippleWidth;
    Desc123->dpcLineCaps.dwAlphaCmpCaps = Desc7->dpcLineCaps.dwAlphaCmpCaps;

    Desc123->dpcTriCaps.dwSize = sizeof(D3DPRIMCAPS);
    Desc123->dpcTriCaps.dwMiscCaps = Desc7->dpcTriCaps.dwMiscCaps;
    Desc123->dpcTriCaps.dwRasterCaps = Desc7->dpcTriCaps.dwRasterCaps;
    Desc123->dpcTriCaps.dwZCmpCaps = Desc7->dpcTriCaps.dwZCmpCaps;
    Desc123->dpcTriCaps.dwSrcBlendCaps = Desc7->dpcTriCaps.dwSrcBlendCaps;
    Desc123->dpcTriCaps.dwDestBlendCaps = Desc7->dpcTriCaps.dwDestBlendCaps;
    Desc123->dpcTriCaps.dwShadeCaps = Desc7->dpcTriCaps.dwShadeCaps;
    Desc123->dpcTriCaps.dwTextureCaps = Desc7->dpcTriCaps.dwTextureCaps;
    Desc123->dpcTriCaps.dwTextureFilterCaps = Desc7->dpcTriCaps.dwTextureFilterCaps;
    Desc123->dpcTriCaps.dwTextureBlendCaps = Desc7->dpcTriCaps.dwTextureBlendCaps;
    Desc123->dpcTriCaps.dwTextureAddressCaps = Desc7->dpcTriCaps.dwTextureAddressCaps;
    Desc123->dpcTriCaps.dwStippleWidth = Desc7->dpcTriCaps.dwStippleWidth;
    Desc123->dpcTriCaps.dwAlphaCmpCaps = Desc7->dpcTriCaps.dwAlphaCmpCaps;

    Desc123->dwDeviceRenderBitDepth = Desc7->dwDeviceRenderBitDepth;
    Desc123->dwDeviceZBufferBitDepth = Desc7->dwDeviceZBufferBitDepth;
    Desc123->dwMaxBufferSize = 0;
    Desc123->dwMaxVertexCount = 65536;
    Desc123->dwMinTextureWidth  = Desc7->dwMinTextureWidth;
    Desc123->dwMinTextureHeight = Desc7->dwMinTextureHeight;
    Desc123->dwMaxTextureWidth  = Desc7->dwMaxTextureWidth;
    Desc123->dwMaxTextureHeight = Desc7->dwMaxTextureHeight;
    Desc123->dwMinStippleWidth  = 1;
    Desc123->dwMinStippleHeight = 1;
    Desc123->dwMaxStippleWidth  = 32;
    Desc123->dwMaxStippleHeight = 32;
    Desc123->dwMaxTextureRepeat = Desc7->dwMaxTextureRepeat;
    Desc123->dwMaxTextureAspectRatio = Desc7->dwMaxTextureAspectRatio;
    Desc123->dwMaxAnisotropy = Desc7->dwMaxAnisotropy;
    Desc123->dvGuardBandLeft = Desc7->dvGuardBandLeft;
    Desc123->dvGuardBandRight = Desc7->dvGuardBandRight;
    Desc123->dvGuardBandTop = Desc7->dvGuardBandTop;
    Desc123->dvGuardBandBottom = Desc7->dvGuardBandBottom;
    Desc123->dvExtentsAdjust = Desc7->dvExtentsAdjust;
    Desc123->dwStencilCaps = Desc7->dwStencilCaps;
    Desc123->dwFVFCaps = Desc7->dwFVFCaps;
    Desc123->dwTextureOpCaps = Desc7->dwTextureOpCaps;
    Desc123->wMaxTextureBlendStages = Desc7->wMaxTextureBlendStages;
    Desc123->wMaxSimultaneousTextures = Desc7->wMaxSimultaneousTextures;

    return DD_OK;
}
/*****************************************************************************
 * IDirect3D vtables in various versions
 *****************************************************************************/

const IDirect3DVtbl IDirect3D1_Vtbl =
{
    /*** IUnknown methods ***/
    Thunk_IDirect3DImpl_1_QueryInterface,
    Thunk_IDirect3DImpl_1_AddRef,
    Thunk_IDirect3DImpl_1_Release,
    /*** IDirect3D methods ***/
    IDirect3DImpl_1_Initialize,
    Thunk_IDirect3DImpl_1_EnumDevices,
    Thunk_IDirect3DImpl_1_CreateLight,
    Thunk_IDirect3DImpl_1_CreateMaterial,
    Thunk_IDirect3DImpl_1_CreateViewport,
    Thunk_IDirect3DImpl_1_FindDevice
};

const IDirect3D2Vtbl IDirect3D2_Vtbl =
{
    /*** IUnknown methods ***/
    Thunk_IDirect3DImpl_2_QueryInterface,
    Thunk_IDirect3DImpl_2_AddRef,
    Thunk_IDirect3DImpl_2_Release,
    /*** IDirect3D2 methods ***/
    Thunk_IDirect3DImpl_2_EnumDevices,
    Thunk_IDirect3DImpl_2_CreateLight,
    Thunk_IDirect3DImpl_2_CreateMaterial,
    Thunk_IDirect3DImpl_2_CreateViewport,
    Thunk_IDirect3DImpl_2_FindDevice,
    Thunk_IDirect3DImpl_2_CreateDevice
};

const IDirect3D3Vtbl IDirect3D3_Vtbl =
{
    /*** IUnknown methods ***/
    Thunk_IDirect3DImpl_3_QueryInterface,
    Thunk_IDirect3DImpl_3_AddRef,
    Thunk_IDirect3DImpl_3_Release,
    /*** IDirect3D3 methods ***/
    IDirect3DImpl_3_EnumDevices,
    IDirect3DImpl_3_CreateLight,
    IDirect3DImpl_3_CreateMaterial,
    IDirect3DImpl_3_CreateViewport,
    IDirect3DImpl_3_FindDevice,
    Thunk_IDirect3DImpl_3_CreateDevice,
    Thunk_IDirect3DImpl_3_CreateVertexBuffer,
    Thunk_IDirect3DImpl_3_EnumZBufferFormats,
    Thunk_IDirect3DImpl_3_EvictManagedTextures
};

const IDirect3D7Vtbl IDirect3D7_Vtbl =
{
    /*** IUnknown methods ***/
    Thunk_IDirect3DImpl_7_QueryInterface,
    Thunk_IDirect3DImpl_7_AddRef,
    Thunk_IDirect3DImpl_7_Release,
    /*** IDirect3D7 methods ***/
    IDirect3DImpl_7_EnumDevices,
    IDirect3DImpl_7_CreateDevice,
    IDirect3DImpl_7_CreateVertexBuffer,
    IDirect3DImpl_7_EnumZBufferFormats,
    IDirect3DImpl_7_EvictManagedTextures
};
