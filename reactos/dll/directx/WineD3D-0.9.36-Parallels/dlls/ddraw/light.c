/* Direct3D Light
 * Copyright (c) 1998 / 2002 Lionel ULMER
 * Copyright (c) 2006        Stefan DÖSINGER
 *
 * This file contains the implementation of Direct3DLight.
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

//#ifndef WINE_NATIVEWIN32
# include "windef.h"
# include "winbase.h"
# include "winnls.h"
# include "winerror.h"
# include "wingdi.h"
//#endif


#include "ddraw.h"
#include "d3d.h"

#include "ddraw_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d7);

/*****************************************************************************
 * IUnknown Methods.
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DLight::QueryInterface
 *
 * Queries the object for different interfaces. Unimplemented for this
 * object at the moment
 *
 * Params:
 *  riid: Interface id asked for
 *  obj: Address to return the resulting pointer at.
 *
 * Returns:
 *  E_NOINTERFACE, because it's a stub
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DLightImpl_QueryInterface(IDirect3DLight *iface,
                                  REFIID riid,
                                  void **obp)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    FIXME("(%p)->(%s,%p): stub!\n", This, debugstr_guid(riid), obp);
    *obp = NULL;
    return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirect3DLight::AddRef
 *
 * Increases the refcount by 1
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirect3DLightImpl_AddRef(IDirect3DLight *iface)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %u.\n", This, ref - 1);

    return ref;
}

/*****************************************************************************
 * IDirect3DLight::Release
 *
 * Reduces the refcount by one. If the refcount falls to 0, the object
 * is destroyed
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirect3DLightImpl_Release(IDirect3DLight *iface)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %u.\n", This, ref + 1);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

/*****************************************************************************
 * IDirect3DLight Methods.
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DLight::Initialize
 *
 * Initializes the interface. This implementation is a no-op, because
 * initialization takes place at creation time
 *
 * Params:
 *  Direct3D: Pointer to an IDirect3D interface.
 *
 * Returns:
 *  D3D_OK
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DLightImpl_Initialize(IDirect3DLight *iface,
                              IDirect3D *lpDirect3D)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    IDirectDrawImpl *d3d = ICOM_OBJECT(IDirectDrawImpl, IDirect3D, lpDirect3D);
    TRACE("(%p)->(%p) no-op...\n", This, d3d);
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DLight::SetLight
 *
 * Assigns a lighting value to this object
 *
 * Params:
 *  Light: Lighting parametes to set
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Light is NULL
 *
 *****************************************************************************/
static void dump_light(const D3DLIGHT2 *light)
{
    DPRINTF("    - dwSize : %d\n", light->dwSize);
}

static const float zero_value[] = {
    0.0, 0.0, 0.0, 0.0
};

static HRESULT WINAPI
IDirect3DLightImpl_SetLight(IDirect3DLight *iface,
                            D3DLIGHT *lpLight)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    LPD3DLIGHT7 light7 = &(This->light7);
    TRACE("(%p)->(%p)\n", This, lpLight);
    if (TRACE_ON(d3d7)) {
        TRACE("  Light definition :\n");
	dump_light((LPD3DLIGHT2) lpLight);
    }

    if ( (lpLight->dltType == 0) || (lpLight->dltType > D3DLIGHT_PARALLELPOINT) )
         return DDERR_INVALIDPARAMS;
    
    if ( lpLight->dltType == D3DLIGHT_PARALLELPOINT )
	 FIXME("D3DLIGHT_PARALLELPOINT no supported\n");
    
    /* Translate D3DLIGH2 structure to D3DLIGHT7 */
    light7->dltType        = lpLight->dltType;
    light7->dcvDiffuse     = lpLight->dcvColor;
    if ((((LPD3DLIGHT2)lpLight)->dwFlags & D3DLIGHT_NO_SPECULAR) != 0)	    
      light7->dcvSpecular    = lpLight->dcvColor;
    else
      light7->dcvSpecular    = *(const D3DCOLORVALUE*)zero_value;	    
    light7->dcvAmbient     = lpLight->dcvColor;
    light7->dvPosition     = lpLight->dvPosition;
    light7->dvDirection    = lpLight->dvDirection;
    light7->dvRange        = lpLight->dvRange;
    light7->dvFalloff      = lpLight->dvFalloff;
    light7->dvAttenuation0 = lpLight->dvAttenuation0;
    light7->dvAttenuation1 = lpLight->dvAttenuation1;
    light7->dvAttenuation2 = lpLight->dvAttenuation2;
    light7->dvTheta        = lpLight->dvTheta;
    light7->dvPhi          = lpLight->dvPhi;

    memcpy(&This->light, lpLight, lpLight->dwSize);
    if ((This->light.dwFlags & D3DLIGHT_ACTIVE) != 0) {
        This->update(This);        
    }
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DLight::GetLight
 *
 * Returns the parameters currently assigned to the IDirect3DLight object
 *
 * Params:
 *  Light: Pointer to an D3DLIGHT structure to store the parameters
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Light is NULL
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DLightImpl_GetLight(IDirect3DLight *iface,
                            D3DLIGHT *lpLight)
{
    ICOM_THIS_FROM(IDirect3DLightImpl, IDirect3DLight, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpLight);
    if (TRACE_ON(d3d7)) {
        TRACE("  Returning light definition :\n");
	dump_light(&This->light);
    }
    memcpy(lpLight, &This->light, lpLight->dwSize);
    return DD_OK;
}

/*****************************************************************************
 * light_update
 *
 * Updates the Direct3DDevice7 lighting parameters
 *
 *****************************************************************************/
void light_update(IDirect3DLightImpl* This)
{
    IDirect3DDeviceImpl* device;

    TRACE("(%p)\n", This);

    if (!This->active_viewport || !This->active_viewport->active_device)
        return;
    device =  This->active_viewport->active_device;

    IDirect3DDevice7_SetLight(ICOM_INTERFACE(device,IDirect3DDevice7), This->dwLightIndex, &(This->light7));
}

/*****************************************************************************
 * light_activate
 *
 * Uses the Direct3DDevice7::LightEnable method to active the light
 *
 *****************************************************************************/
void light_activate(IDirect3DLightImpl* This)
{
    IDirect3DDeviceImpl* device;

    TRACE("(%p)\n", This);

    if (!This->active_viewport || !This->active_viewport->active_device)
        return;
    device =  This->active_viewport->active_device;
    
    light_update(This);
    /* If was not active, activate it */
    if ((This->light.dwFlags & D3DLIGHT_ACTIVE) == 0) {
        IDirect3DDevice7_LightEnable(ICOM_INTERFACE(device,IDirect3DDevice7), This->dwLightIndex, TRUE);
	This->light.dwFlags |= D3DLIGHT_ACTIVE;
    }
}

/*****************************************************************************
 *
 * light_desactivate
 *
 * Uses the Direct3DDevice7::LightEnable method to deactivate the light
 *
 *****************************************************************************/
void light_desactivate(IDirect3DLightImpl* This)
{
    IDirect3DDeviceImpl* device;

    TRACE("(%p)\n", This);

    if (!This->active_viewport || !This->active_viewport->active_device)
        return;
    device =  This->active_viewport->active_device;
    
    /* If was not active, activate it */
    if ((This->light.dwFlags & D3DLIGHT_ACTIVE) != 0) {
        IDirect3DDevice7_LightEnable(ICOM_INTERFACE(device,IDirect3DDevice7), This->dwLightIndex, FALSE);
	This->light.dwFlags &= ~D3DLIGHT_ACTIVE;
    }
}

const IDirect3DLightVtbl IDirect3DLight_Vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DLightImpl_QueryInterface,
    IDirect3DLightImpl_AddRef,
    IDirect3DLightImpl_Release,
    /*** IDirect3DLight Methods ***/
    IDirect3DLightImpl_Initialize,
    IDirect3DLightImpl_SetLight,
    IDirect3DLightImpl_GetLight
};
