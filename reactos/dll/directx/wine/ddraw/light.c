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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/*****************************************************************************
 * light_update
 *
 * Updates the Direct3DDevice7 lighting parameters
 *
 *****************************************************************************/
static void light_update(IDirect3DLightImpl *light)
{
    IDirect3DDeviceImpl *device;

    TRACE("light %p.\n", light);

    if (!light->active_viewport || !light->active_viewport->active_device) return;
    device = light->active_viewport->active_device;

    IDirect3DDevice7_SetLight(&device->IDirect3DDevice7_iface, light->dwLightIndex, &light->light7);
}

/*****************************************************************************
 * light_activate
 *
 * Uses the Direct3DDevice7::LightEnable method to active the light
 *
 *****************************************************************************/
void light_activate(IDirect3DLightImpl *light)
{
    IDirect3DDeviceImpl *device;

    TRACE("light %p.\n", light);

    if (!light->active_viewport || !light->active_viewport->active_device) return;
    device = light->active_viewport->active_device;

    light_update(light);
    if (!(light->light.dwFlags & D3DLIGHT_ACTIVE))
    {
        IDirect3DDevice7_LightEnable(&device->IDirect3DDevice7_iface, light->dwLightIndex, TRUE);
        light->light.dwFlags |= D3DLIGHT_ACTIVE;
    }
}

/*****************************************************************************
 *
 * light_deactivate
 *
 * Uses the Direct3DDevice7::LightEnable method to deactivate the light
 *
 *****************************************************************************/
void light_deactivate(IDirect3DLightImpl *light)
{
    IDirect3DDeviceImpl *device;

    TRACE("light %p.\n", light);

    if (!light->active_viewport || !light->active_viewport->active_device) return;
    device = light->active_viewport->active_device;

    /* If was not active, activate it */
    if (light->light.dwFlags & D3DLIGHT_ACTIVE)
    {
        IDirect3DDevice7_LightEnable(&device->IDirect3DDevice7_iface, light->dwLightIndex, FALSE);
        light->light.dwFlags &= ~D3DLIGHT_ACTIVE;
    }
}

static inline IDirect3DLightImpl *impl_from_IDirect3DLight(IDirect3DLight *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DLightImpl, IDirect3DLight_iface);
}

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
static HRESULT WINAPI IDirect3DLightImpl_QueryInterface(IDirect3DLight *iface, REFIID riid, void **object)
{
    FIXME("iface %p, riid %s, object %p stub!\n", iface, debugstr_guid(riid), object);

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DLightImpl_AddRef(IDirect3DLight *iface)
{
    IDirect3DLightImpl *This = impl_from_IDirect3DLight(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirect3DLightImpl_Release(IDirect3DLight *iface)
{
    IDirect3DLightImpl *This = impl_from_IDirect3DLight(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

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
static HRESULT WINAPI IDirect3DLightImpl_Initialize(IDirect3DLight *iface, IDirect3D *d3d)
{
    TRACE("iface %p, d3d %p.\n", iface, d3d);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DLight::SetLight
 *
 * Assigns a lighting value to this object
 *
 * Params:
 *  Light: Lighting parameter to set
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Light is NULL
 *
 *****************************************************************************/
static void dump_light(const D3DLIGHT2 *light)
{
    TRACE("    - dwSize : %d\n", light->dwSize);
}

static const float zero_value[] = {
    0.0, 0.0, 0.0, 0.0
};

static HRESULT WINAPI IDirect3DLightImpl_SetLight(IDirect3DLight *iface, D3DLIGHT *lpLight)
{
    IDirect3DLightImpl *This = impl_from_IDirect3DLight(iface);
    LPD3DLIGHT7 light7 = &This->light7;

    TRACE("iface %p, light %p.\n", iface, lpLight);

    if (TRACE_ON(ddraw))
    {
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

    wined3d_mutex_lock();
    memcpy(&This->light, lpLight, lpLight->dwSize);
    if (This->light.dwFlags & D3DLIGHT_ACTIVE)
        light_update(This);
    wined3d_mutex_unlock();

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
static HRESULT WINAPI IDirect3DLightImpl_GetLight(IDirect3DLight *iface, D3DLIGHT *lpLight)
{
    IDirect3DLightImpl *This = impl_from_IDirect3DLight(iface);

    TRACE("iface %p, light %p.\n", iface, lpLight);

    if (TRACE_ON(ddraw))
    {
        TRACE("  Returning light definition :\n");
        dump_light(&This->light);
    }

    wined3d_mutex_lock();
    memcpy(lpLight, &This->light, lpLight->dwSize);
    wined3d_mutex_unlock();

    return DD_OK;
}

static const struct IDirect3DLightVtbl d3d_light_vtbl =
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

void d3d_light_init(IDirect3DLightImpl *light, IDirectDrawImpl *ddraw)
{
    light->IDirect3DLight_iface.lpVtbl = &d3d_light_vtbl;
    light->ref = 1;
    light->ddraw = ddraw;
}

IDirect3DLightImpl *unsafe_impl_from_IDirect3DLight(IDirect3DLight *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d_light_vtbl);

    return impl_from_IDirect3DLight(iface);
}
