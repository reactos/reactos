/* Direct3D Material
 * Copyright (c) 2002 Lionel ULMER
 * Copyright (c) 2006 Stefan DÖSINGER
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

static void dump_material(const D3DMATERIAL *mat)
{
    TRACE("  dwSize : %lu\n", mat->dwSize);
}

static inline struct d3d_material *impl_from_IDirect3DMaterial(IDirect3DMaterial *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_material, IDirect3DMaterial_iface);
}

static inline struct d3d_material *impl_from_IDirect3DMaterial2(IDirect3DMaterial2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_material, IDirect3DMaterial2_iface);
}

static inline struct d3d_material *impl_from_IDirect3DMaterial3(IDirect3DMaterial3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_material, IDirect3DMaterial3_iface);
}

/*****************************************************************************
 * IUnknown Methods.
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DMaterial3::QueryInterface
 *
 * QueryInterface for IDirect3DMaterial. Can query all IDirect3DMaterial
 * versions.
 *
 * Params:
 *  riid: Interface id queried for
 *  obj: Address to pass the interface pointer back
 *
 * Returns:
 *  S_OK on success
 *  E_NOINTERFACE if the requested interface wasn't found
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_material3_QueryInterface(IDirect3DMaterial3 *iface, REFIID riid, void **obp)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial3(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obp);

    *obp = NULL;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        IDirect3DMaterial3_AddRef(iface);
        *obp = iface;
        TRACE("  Creating IUnknown interface at %p.\n", *obp);
        return S_OK;
    }
    if (IsEqualGUID(&IID_IDirect3DMaterial, riid))
    {
        IDirect3DMaterial_AddRef(&material->IDirect3DMaterial_iface);
        *obp = &material->IDirect3DMaterial_iface;
        TRACE("  Creating IDirect3DMaterial interface %p\n", *obp);
        return S_OK;
    }
    if (IsEqualGUID(&IID_IDirect3DMaterial2, riid))
    {
        IDirect3DMaterial2_AddRef(&material->IDirect3DMaterial2_iface);
        *obp = &material->IDirect3DMaterial2_iface;
        TRACE("  Creating IDirect3DMaterial2 interface %p\n", *obp);
        return S_OK;
    }
    if (IsEqualGUID(&IID_IDirect3DMaterial3, riid))
    {
        IDirect3DMaterial3_AddRef(&material->IDirect3DMaterial3_iface);
        *obp = &material->IDirect3DMaterial3_iface;
        TRACE("  Creating IDirect3DMaterial3 interface %p\n", *obp);
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirect3DMaterial3::AddRef
 *
 * Increases the refcount.
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI d3d_material3_AddRef(IDirect3DMaterial3 *iface)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial3(iface);
    ULONG ref = InterlockedIncrement(&material->ref);

    TRACE("%p increasing refcount to %lu.\n", material, ref);

    return ref;
}

/*****************************************************************************
 * IDirect3DMaterial3::Release
 *
 * Reduces the refcount by one. If the refcount falls to 0, the object
 * is destroyed
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI d3d_material3_Release(IDirect3DMaterial3 *iface)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial3(iface);
    ULONG ref = InterlockedDecrement(&material->ref);

    TRACE("%p decreasing refcount to %lu.\n", material, ref);

    if (!ref)
    {
        if (material->Handle)
        {
            wined3d_mutex_lock();
            ddraw_free_handle(NULL, material->Handle - 1, DDRAW_HANDLE_MATERIAL);
            wined3d_mutex_unlock();
        }

        free(material);
    }

    return ref;
}

/*****************************************************************************
 * IDirect3DMaterial Methods
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DMaterial::Initialize
 *
 * A no-op initialization
 *
 * Params:
 *  Direct3D: Pointer to a Direct3D interface
 *
 * Returns:
 *  D3D_OK
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_material1_Initialize(IDirect3DMaterial *iface, IDirect3D *d3d)
{
    TRACE("iface %p, d3d %p.\n", iface, d3d);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DMaterial::Reserve
 *
 * DirectX 5 sdk: "The IDirect3DMaterial2::Reserve method is not implemented"
 * Odd. They seem to have mixed their interfaces.
 *
 * Returns:
 *  DDERR_UNSUPPORTED
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_material1_Reserve(IDirect3DMaterial *iface)
{
    TRACE("iface %p.\n", iface);

    return DDERR_UNSUPPORTED;
}

/*****************************************************************************
 * IDirect3DMaterial::Unreserve
 *
 * Not supported too
 *
 * Returns:
 *  DDERR_UNSUPPORTED
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_material1_Unreserve(IDirect3DMaterial *iface)
{
    TRACE("iface %p.\n", iface);

    return DDERR_UNSUPPORTED;
}

/*****************************************************************************
 * IDirect3DMaterial3::SetMaterial
 *
 * Sets the material description
 *
 * Params:
 *  Mat: Material to set
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Mat is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_material3_SetMaterial(IDirect3DMaterial3 *iface, D3DMATERIAL *mat)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial3(iface);

    TRACE("iface %p, mat %p.\n", iface, mat);
    if (TRACE_ON(ddraw))
        dump_material(mat);

    /* Stores the material */
    wined3d_mutex_lock();
    memset(&material->mat, 0, sizeof(material->mat));
    memcpy(&material->mat, mat, mat->dwSize);
    wined3d_mutex_unlock();

    return DD_OK;
}

/*****************************************************************************
 * IDirect3DMaterial3::GetMaterial
 *
 * Returns the material assigned to this interface
 *
 * Params:
 *  Mat: Pointer to a D3DMATERIAL structure to store the material description
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Mat is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_material3_GetMaterial(IDirect3DMaterial3 *iface, D3DMATERIAL *mat)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial3(iface);
    DWORD dwSize;

    TRACE("iface %p, mat %p.\n", iface, mat);
    if (TRACE_ON(ddraw))
    {
        TRACE("  Returning material : ");
        dump_material(&material->mat);
    }

    /* Copies the material structure */
    wined3d_mutex_lock();
    dwSize = mat->dwSize;
    memcpy(mat, &material->mat, dwSize);
    wined3d_mutex_unlock();

    return DD_OK;
}

/*****************************************************************************
 * IDirect3DMaterial3::GetHandle
 *
 * Returns a handle for the material interface. The handle is simply a
 * pointer to the material implementation
 *
 * Params:
 *  Direct3DDevice3: The device this handle is assigned to
 *  Handle: Address to write the handle to
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Handle is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_material3_GetHandle(IDirect3DMaterial3 *iface,
        IDirect3DDevice3 *device, D3DMATERIALHANDLE *handle)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial3(iface);
    struct d3d_device *device_impl = unsafe_impl_from_IDirect3DDevice3(device);

    TRACE("iface %p, device %p, handle %p.\n", iface, device, handle);

    wined3d_mutex_lock();
    material->active_device = device_impl;
    if (!material->Handle)
    {
        DWORD h = ddraw_allocate_handle(NULL, material, DDRAW_HANDLE_MATERIAL);
        if (h == DDRAW_INVALID_HANDLE)
        {
            ERR("Failed to allocate a material handle.\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;   /* Unchecked */
        }

        material->Handle = h + 1;
    }
    *handle = material->Handle;
    TRACE(" returning handle %#lx.\n", *handle);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_material2_GetHandle(IDirect3DMaterial2 *iface,
        IDirect3DDevice2 *device, D3DMATERIALHANDLE *handle)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial2(iface);
    struct d3d_device *device_impl = unsafe_impl_from_IDirect3DDevice2(device);

    TRACE("iface %p, device %p, handle %p.\n", iface, device, handle);

    return d3d_material3_GetHandle(&material->IDirect3DMaterial3_iface,
            device_impl ? &device_impl->IDirect3DDevice3_iface : NULL, handle);
}

static HRESULT WINAPI d3d_material1_GetHandle(IDirect3DMaterial *iface,
        IDirect3DDevice *device, D3DMATERIALHANDLE *handle)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial(iface);
    struct d3d_device *device_impl = unsafe_impl_from_IDirect3DDevice(device);

    TRACE("iface %p, device %p, handle %p.\n", iface, device, handle);

    return d3d_material3_GetHandle(&material->IDirect3DMaterial3_iface,
            device_impl ? &device_impl->IDirect3DDevice3_iface : NULL, handle);
}

static HRESULT WINAPI d3d_material2_QueryInterface(IDirect3DMaterial2 *iface, REFIID riid, void **object)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d_material3_QueryInterface(&material->IDirect3DMaterial3_iface, riid, object);
}

static HRESULT WINAPI d3d_material1_QueryInterface(IDirect3DMaterial *iface, REFIID riid, void **object)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d_material3_QueryInterface(&material->IDirect3DMaterial3_iface, riid, object);
}

static ULONG WINAPI d3d_material2_AddRef(IDirect3DMaterial2 *iface)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p.\n", iface);

    return d3d_material3_AddRef(&material->IDirect3DMaterial3_iface);
}

static ULONG WINAPI d3d_material1_AddRef(IDirect3DMaterial *iface)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p.\n", iface);

    return d3d_material3_AddRef(&material->IDirect3DMaterial3_iface);
}

static ULONG WINAPI d3d_material2_Release(IDirect3DMaterial2 *iface)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p.\n", iface);

    return d3d_material3_Release(&material->IDirect3DMaterial3_iface);
}

static ULONG WINAPI d3d_material1_Release(IDirect3DMaterial *iface)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p.\n", iface);

    return d3d_material3_Release(&material->IDirect3DMaterial3_iface);
}

static HRESULT WINAPI d3d_material2_SetMaterial(IDirect3DMaterial2 *iface, D3DMATERIAL *mat)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p, material %p.\n", iface, mat);

    return d3d_material3_SetMaterial(&material->IDirect3DMaterial3_iface, mat);
}

static HRESULT WINAPI d3d_material1_SetMaterial(IDirect3DMaterial *iface, D3DMATERIAL *mat)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p, material %p.\n", iface, mat);

    return d3d_material3_SetMaterial(&material->IDirect3DMaterial3_iface, mat);
}

static HRESULT WINAPI d3d_material2_GetMaterial(IDirect3DMaterial2 *iface, D3DMATERIAL *mat)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p, material %p.\n", iface, mat);

    return d3d_material3_GetMaterial(&material->IDirect3DMaterial3_iface, mat);
}

static HRESULT WINAPI d3d_material1_GetMaterial(IDirect3DMaterial *iface, D3DMATERIAL *mat)
{
    struct d3d_material *material = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p, material %p.\n", iface, mat);

    return d3d_material3_GetMaterial(&material->IDirect3DMaterial3_iface, mat);
}


/*****************************************************************************
 * material_activate
 *
 * Uses IDirect3DDevice7::SetMaterial to activate the material
 *
 * Params:
 *  This: Pointer to the material implementation to activate
 *
 *****************************************************************************/
void material_activate(struct d3d_material *material)
{
    D3DMATERIAL7 d3d7mat;

    TRACE("Activating material %p.\n", material);

    d3d7mat.diffuse = material->mat.diffuse;
    d3d7mat.ambient = material->mat.ambient;
    d3d7mat.specular = material->mat.specular;
    d3d7mat.emissive = material->mat.emissive;
    d3d7mat.power = material->mat.power;

    IDirect3DDevice7_SetMaterial(&material->active_device->IDirect3DDevice7_iface, &d3d7mat);
}

static const struct IDirect3DMaterial3Vtbl d3d_material3_vtbl =
{
    /*** IUnknown Methods ***/
    d3d_material3_QueryInterface,
    d3d_material3_AddRef,
    d3d_material3_Release,
    /*** IDirect3DMaterial3 Methods ***/
    d3d_material3_SetMaterial,
    d3d_material3_GetMaterial,
    d3d_material3_GetHandle,
};

static const struct IDirect3DMaterial2Vtbl d3d_material2_vtbl =
{
    /*** IUnknown Methods ***/
    d3d_material2_QueryInterface,
    d3d_material2_AddRef,
    d3d_material2_Release,
    /*** IDirect3DMaterial2 Methods ***/
    d3d_material2_SetMaterial,
    d3d_material2_GetMaterial,
    d3d_material2_GetHandle,
};

static const struct IDirect3DMaterialVtbl d3d_material1_vtbl =
{
    /*** IUnknown Methods ***/
    d3d_material1_QueryInterface,
    d3d_material1_AddRef,
    d3d_material1_Release,
    /*** IDirect3DMaterial1 Methods ***/
    d3d_material1_Initialize,
    d3d_material1_SetMaterial,
    d3d_material1_GetMaterial,
    d3d_material1_GetHandle,
    d3d_material1_Reserve,
    d3d_material1_Unreserve,
};

struct d3d_material *d3d_material_create(struct ddraw *ddraw)
{
    struct d3d_material *material;

    if (!(material = calloc(1, sizeof(*material))))
        return NULL;

    material->IDirect3DMaterial3_iface.lpVtbl = &d3d_material3_vtbl;
    material->IDirect3DMaterial2_iface.lpVtbl = &d3d_material2_vtbl;
    material->IDirect3DMaterial_iface.lpVtbl = &d3d_material1_vtbl;
    material->ref = 1;
    material->ddraw = ddraw;

    return material;
}
