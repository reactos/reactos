/* Direct3D Light
 * Copyright (c) 1998 / 2002 Lionel ULMER
 * Copyright (c) 2006        Stefan DÖSINGER
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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/*****************************************************************************
 * light_update
 *
 * Updates the Direct3DDevice7 lighting parameters
 *
 *****************************************************************************/
static void light_update(struct d3d_light *light)
{
    struct d3d_device *device;

    TRACE("light %p.\n", light);

    if (!light->active_viewport || !light->active_viewport->active_device) return;
    device = light->active_viewport->active_device;

    IDirect3DDevice7_SetLight(&device->IDirect3DDevice7_iface, light->active_light_index, &light->light7);
}

/*****************************************************************************
 * light_activate
 *
 * Uses the Direct3DDevice7::LightEnable method to active the light
 *
 *****************************************************************************/
void light_activate(struct d3d_light *light)
{
    struct d3d_device *device;

    TRACE("light %p.\n", light);

    if (!light->active_viewport || !light->active_viewport->active_device
            || light->active_viewport->active_device->current_viewport != light->active_viewport)
        return;
    device = light->active_viewport->active_device;

    if (light->light.dwFlags & D3DLIGHT_ACTIVE)
    {
        viewport_alloc_active_light_index(light);
        light_update(light);
        IDirect3DDevice7_LightEnable(&device->IDirect3DDevice7_iface, light->active_light_index, TRUE);
    }
}

/*****************************************************************************
 *
 * light_deactivate
 *
 * Uses the Direct3DDevice7::LightEnable method to deactivate the light
 *
 *****************************************************************************/
void light_deactivate(struct d3d_light *light)
{
    struct d3d_device *device;

    TRACE("light %p.\n", light);

    if (!light->active_viewport || !light->active_viewport->active_device
            || light->active_viewport->active_device->current_viewport != light->active_viewport)
    {
        assert(!light->active_light_index);
        return;
    }

    device = light->active_viewport->active_device;
    if (light->active_light_index)
    {
        IDirect3DDevice7_LightEnable(&device->IDirect3DDevice7_iface, light->active_light_index, FALSE);
        viewport_free_active_light_index(light);
    }
}

static inline struct d3d_light *impl_from_IDirect3DLight(IDirect3DLight *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_light, IDirect3DLight_iface);
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
static HRESULT WINAPI d3d_light_QueryInterface(IDirect3DLight *iface, REFIID riid, void **object)
{
    FIXME("iface %p, riid %s, object %p stub!\n", iface, debugstr_guid(riid), object);

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d_light_AddRef(IDirect3DLight *iface)
{
    struct d3d_light *light = impl_from_IDirect3DLight(iface);
    ULONG ref = InterlockedIncrement(&light->ref);

    TRACE("%p increasing refcount to %lu.\n", light, ref);

    return ref;
}

static ULONG WINAPI d3d_light_Release(IDirect3DLight *iface)
{
    struct d3d_light *light = impl_from_IDirect3DLight(iface);
    ULONG ref = InterlockedDecrement(&light->ref);

    TRACE("%p decreasing refcount to %lu.\n", light, ref);

    if (!ref)
    {
        free(light);
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
static HRESULT WINAPI d3d_light_Initialize(IDirect3DLight *iface, IDirect3D *d3d)
{
    TRACE("iface %p, d3d %p.\n", iface, d3d);

    return D3D_OK;
}

static HRESULT WINAPI d3d_light_SetLight(IDirect3DLight *iface, D3DLIGHT *data)
{
    static const D3DCOLORVALUE zero_value = {{0.0f}, {0.0f}, {0.0f}, {0.0f}};
    struct d3d_light *light = impl_from_IDirect3DLight(iface);
    DWORD flags = data->dwSize >= sizeof(D3DLIGHT2) ? ((D3DLIGHT2 *)data)->dwFlags : D3DLIGHT_ACTIVE;
    D3DLIGHT7 *light7 = &light->light7;

    TRACE("iface %p, data %p.\n", iface, data);

    if ((!data->dltType) || (data->dltType > D3DLIGHT_PARALLELPOINT))
         return DDERR_INVALIDPARAMS;

    /* Translate D3DLIGHT2 structure to D3DLIGHT7. */
    light7->dltType = data->dltType;
    light7->dcvDiffuse = data->dcvColor;
    if (flags & D3DLIGHT_NO_SPECULAR)
        light7->dcvSpecular = zero_value;
    else
        light7->dcvSpecular = data->dcvColor;
    light7->dcvAmbient = zero_value;
    light7->dvPosition = data->dvPosition;
    light7->dvDirection = data->dvDirection;
    light7->dvRange = data->dvRange;
    light7->dvFalloff = data->dvFalloff;
    light7->dvAttenuation0 = data->dvAttenuation0;
    light7->dvAttenuation1 = data->dvAttenuation1;
    light7->dvAttenuation2 = data->dvAttenuation2;
    light7->dvTheta = data->dvTheta;
    light7->dvPhi = data->dvPhi;

    wined3d_mutex_lock();
    memcpy(&light->light, data, sizeof(*data));

    if (!(flags & D3DLIGHT_ACTIVE))
        light_deactivate(light);

    light->light.dwFlags = flags;
    light_activate(light);
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
static HRESULT WINAPI d3d_light_GetLight(IDirect3DLight *iface, D3DLIGHT *lpLight)
{
    struct d3d_light *light = impl_from_IDirect3DLight(iface);

    TRACE("iface %p, light %p.\n", iface, lpLight);

    wined3d_mutex_lock();
    memcpy(lpLight, &light->light, lpLight->dwSize);
    wined3d_mutex_unlock();

    return DD_OK;
}

static const struct IDirect3DLightVtbl d3d_light_vtbl =
{
    /*** IUnknown Methods ***/
    d3d_light_QueryInterface,
    d3d_light_AddRef,
    d3d_light_Release,
    /*** IDirect3DLight Methods ***/
    d3d_light_Initialize,
    d3d_light_SetLight,
    d3d_light_GetLight
};

void d3d_light_init(struct d3d_light *light, struct ddraw *ddraw)
{
    light->IDirect3DLight_iface.lpVtbl = &d3d_light_vtbl;
    light->ref = 1;
    light->ddraw = ddraw;
}

struct d3d_light *unsafe_impl_from_IDirect3DLight(IDirect3DLight *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d_light_vtbl);

    return impl_from_IDirect3DLight(iface);
}
