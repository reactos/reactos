/* Direct3D Material
 * Copyright (c) 2002 Lionel ULMER
 * Copyright (c) 2006 Stefan DÖSINGER
 *
 * This file contains the implementation of Direct3DMaterial.
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

static void dump_material(const D3DMATERIAL *mat)
{
    TRACE("  dwSize : %d\n", mat->dwSize);
}

static inline IDirect3DMaterialImpl *impl_from_IDirect3DMaterial(IDirect3DMaterial *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DMaterialImpl, IDirect3DMaterial_iface);
}

static inline IDirect3DMaterialImpl *impl_from_IDirect3DMaterial2(IDirect3DMaterial2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DMaterialImpl, IDirect3DMaterial2_iface);
}

static inline IDirect3DMaterialImpl *impl_from_IDirect3DMaterial3(IDirect3DMaterial3 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DMaterialImpl, IDirect3DMaterial3_iface);
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
static HRESULT WINAPI IDirect3DMaterialImpl_QueryInterface(IDirect3DMaterial3 *iface, REFIID riid,
        void **obp)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial3(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obp);

    *obp = NULL;

    if ( IsEqualGUID( &IID_IUnknown,  riid ) ) {
        IUnknown_AddRef(iface);
        *obp = iface;
        TRACE("  Creating IUnknown interface at %p.\n", *obp);
        return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial, riid ) ) {
        IDirect3DMaterial_AddRef(&This->IDirect3DMaterial_iface);
        *obp = &This->IDirect3DMaterial_iface;
        TRACE("  Creating IDirect3DMaterial interface %p\n", *obp);
        return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial2, riid ) ) {
        IDirect3DMaterial_AddRef(&This->IDirect3DMaterial2_iface);
        *obp = &This->IDirect3DMaterial2_iface;
        TRACE("  Creating IDirect3DMaterial2 interface %p\n", *obp);
        return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial3, riid ) ) {
        IDirect3DMaterial3_AddRef(&This->IDirect3DMaterial3_iface);
        *obp = This;
        TRACE("  Creating IDirect3DMaterial3 interface %p\n", *obp);
        return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
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
static ULONG WINAPI IDirect3DMaterialImpl_AddRef(IDirect3DMaterial3 *iface)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial3(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", This, ref);

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
static ULONG WINAPI IDirect3DMaterialImpl_Release(IDirect3DMaterial3 *iface)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial3(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (!ref)
    {
        if(This->Handle)
        {
            wined3d_mutex_lock();
            ddraw_free_handle(&This->ddraw->d3ddevice->handle_table, This->Handle - 1, DDRAW_HANDLE_MATERIAL);
            wined3d_mutex_unlock();
        }

        HeapFree(GetProcessHeap(), 0, This);
        return 0;
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
static HRESULT WINAPI
IDirect3DMaterialImpl_Initialize(IDirect3DMaterial *iface,
                                  IDirect3D *Direct3D)
{
    TRACE("iface %p, d3d %p.\n", iface, Direct3D);

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
static HRESULT WINAPI
IDirect3DMaterialImpl_Reserve(IDirect3DMaterial *iface)
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
static HRESULT WINAPI
IDirect3DMaterialImpl_Unreserve(IDirect3DMaterial *iface)
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
static HRESULT WINAPI IDirect3DMaterialImpl_SetMaterial(IDirect3DMaterial3 *iface,
        D3DMATERIAL *lpMat)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial3(iface);

    TRACE("iface %p, material %p.\n", iface, lpMat);
    if (TRACE_ON(ddraw))
        dump_material(lpMat);

    /* Stores the material */
    wined3d_mutex_lock();
    memset(&This->mat, 0, sizeof(This->mat));
    memcpy(&This->mat, lpMat, lpMat->dwSize);
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
static HRESULT WINAPI IDirect3DMaterialImpl_GetMaterial(IDirect3DMaterial3 *iface,
        D3DMATERIAL *lpMat)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial3(iface);
    DWORD dwSize;

    TRACE("iface %p, material %p.\n", iface, lpMat);
    if (TRACE_ON(ddraw))
    {
        TRACE("  Returning material : ");
        dump_material(&This->mat);
    }

    /* Copies the material structure */
    wined3d_mutex_lock();
    dwSize = lpMat->dwSize;
    memcpy(lpMat, &This->mat, dwSize);
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
static HRESULT WINAPI IDirect3DMaterialImpl_GetHandle(IDirect3DMaterial3 *iface,
        IDirect3DDevice3 *device, D3DMATERIALHANDLE *handle)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial3(iface);
    IDirect3DDeviceImpl *device_impl = unsafe_impl_from_IDirect3DDevice3(device);

    TRACE("iface %p, device %p, handle %p.\n", iface, device, handle);

    wined3d_mutex_lock();
    This->active_device = device_impl;
    if(!This->Handle)
    {
        DWORD h = ddraw_allocate_handle(&device_impl->handle_table, This, DDRAW_HANDLE_MATERIAL);
        if (h == DDRAW_INVALID_HANDLE)
        {
            ERR("Failed to allocate a material handle.\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;   /* Unchecked */
        }

        This->Handle = h + 1;
    }
    *handle = This->Handle;
    TRACE(" returning handle %08x.\n", *handle);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DMaterialImpl_2_GetHandle(IDirect3DMaterial2 *iface,
        IDirect3DDevice2 *device, D3DMATERIALHANDLE *handle)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial2(iface);
    IDirect3DDeviceImpl *device_impl = unsafe_impl_from_IDirect3DDevice2(device);

    TRACE("iface %p, device %p, handle %p.\n", iface, device, handle);

    return IDirect3DMaterial3_GetHandle(&This->IDirect3DMaterial3_iface, device_impl ?
            &device_impl->IDirect3DDevice3_iface : NULL, handle);
}

static HRESULT WINAPI IDirect3DMaterialImpl_1_GetHandle(IDirect3DMaterial *iface,
        IDirect3DDevice *device, D3DMATERIALHANDLE *handle)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial(iface);
    IDirect3DDeviceImpl *device_impl = unsafe_impl_from_IDirect3DDevice(device);

    TRACE("iface %p, device %p, handle %p.\n", iface, device, handle);

    return IDirect3DMaterial3_GetHandle(&This->IDirect3DMaterial3_iface, device_impl ?
            &device_impl->IDirect3DDevice3_iface : NULL, handle);
}

static HRESULT WINAPI IDirect3DMaterialImpl_2_QueryInterface(IDirect3DMaterial2 *iface, REFIID riid,
        void **obp)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obp);

    return IDirect3DMaterial3_QueryInterface(&This->IDirect3DMaterial3_iface, riid, obp);
}

static HRESULT WINAPI IDirect3DMaterialImpl_1_QueryInterface(IDirect3DMaterial *iface, REFIID riid,
        void **obp)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obp);

    return IDirect3DMaterial3_QueryInterface(&This->IDirect3DMaterial3_iface, riid, obp);
}

static ULONG WINAPI IDirect3DMaterialImpl_2_AddRef(IDirect3DMaterial2 *iface)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DMaterial3_AddRef(&This->IDirect3DMaterial3_iface);
}

static ULONG WINAPI IDirect3DMaterialImpl_1_AddRef(IDirect3DMaterial *iface)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DMaterial3_AddRef(&This->IDirect3DMaterial3_iface);
}

static ULONG WINAPI IDirect3DMaterialImpl_2_Release(IDirect3DMaterial2 *iface)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DMaterial3_Release(&This->IDirect3DMaterial3_iface);
}

static ULONG WINAPI IDirect3DMaterialImpl_1_Release(IDirect3DMaterial *iface)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DMaterial3_Release(&This->IDirect3DMaterial3_iface);
}

static HRESULT WINAPI IDirect3DMaterialImpl_2_SetMaterial(IDirect3DMaterial2 *iface,
        LPD3DMATERIAL lpMat)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p, material %p.\n", iface, lpMat);

    return IDirect3DMaterial3_SetMaterial(&This->IDirect3DMaterial3_iface, lpMat);
}

static HRESULT WINAPI IDirect3DMaterialImpl_1_SetMaterial(IDirect3DMaterial *iface,
        LPD3DMATERIAL lpMat)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p, material %p.\n", iface, lpMat);

    return IDirect3DMaterial3_SetMaterial(&This->IDirect3DMaterial3_iface, lpMat);
}

static HRESULT WINAPI IDirect3DMaterialImpl_2_GetMaterial(IDirect3DMaterial2 *iface,
        LPD3DMATERIAL lpMat)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial2(iface);

    TRACE("iface %p, material %p.\n", iface, lpMat);

    return IDirect3DMaterial3_GetMaterial(&This->IDirect3DMaterial3_iface, lpMat);
}

static HRESULT WINAPI IDirect3DMaterialImpl_1_GetMaterial(IDirect3DMaterial *iface,
        LPD3DMATERIAL lpMat)
{
    IDirect3DMaterialImpl *This = impl_from_IDirect3DMaterial(iface);

    TRACE("iface %p, material %p.\n", iface, lpMat);

    return IDirect3DMaterial3_GetMaterial(&This->IDirect3DMaterial3_iface, lpMat);
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
void material_activate(IDirect3DMaterialImpl* This)
{
    D3DMATERIAL7 d3d7mat;

    TRACE("Activating material %p\n", This);
    d3d7mat.u.diffuse = This->mat.u.diffuse;
    d3d7mat.u1.ambient = This->mat.u1.ambient;
    d3d7mat.u2.specular = This->mat.u2.specular;
    d3d7mat.u3.emissive = This->mat.u3.emissive;
    d3d7mat.u4.power = This->mat.u4.power;

    IDirect3DDevice7_SetMaterial(&This->active_device->IDirect3DDevice7_iface, &d3d7mat);
}

static const struct IDirect3DMaterial3Vtbl d3d_material3_vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DMaterialImpl_QueryInterface,
    IDirect3DMaterialImpl_AddRef,
    IDirect3DMaterialImpl_Release,
    /*** IDirect3DMaterial3 Methods ***/
    IDirect3DMaterialImpl_SetMaterial,
    IDirect3DMaterialImpl_GetMaterial,
    IDirect3DMaterialImpl_GetHandle,
};

static const struct IDirect3DMaterial2Vtbl d3d_material2_vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DMaterialImpl_2_QueryInterface,
    IDirect3DMaterialImpl_2_AddRef,
    IDirect3DMaterialImpl_2_Release,
    /*** IDirect3DMaterial2 Methods ***/
    IDirect3DMaterialImpl_2_SetMaterial,
    IDirect3DMaterialImpl_2_GetMaterial,
    IDirect3DMaterialImpl_2_GetHandle,
};

static const struct IDirect3DMaterialVtbl d3d_material1_vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DMaterialImpl_1_QueryInterface,
    IDirect3DMaterialImpl_1_AddRef,
    IDirect3DMaterialImpl_1_Release,
    /*** IDirect3DMaterial1 Methods ***/
    IDirect3DMaterialImpl_Initialize,
    IDirect3DMaterialImpl_1_SetMaterial,
    IDirect3DMaterialImpl_1_GetMaterial,
    IDirect3DMaterialImpl_1_GetHandle,
    IDirect3DMaterialImpl_Reserve,
    IDirect3DMaterialImpl_Unreserve
};

IDirect3DMaterialImpl *d3d_material_create(IDirectDrawImpl *ddraw)
{
    IDirect3DMaterialImpl *material;

    material = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*material));
    if (!material)
        return NULL;

    material->IDirect3DMaterial3_iface.lpVtbl = &d3d_material3_vtbl;
    material->IDirect3DMaterial2_iface.lpVtbl = &d3d_material2_vtbl;
    material->IDirect3DMaterial_iface.lpVtbl = &d3d_material1_vtbl;
    material->ref = 1;
    material->ddraw = ddraw;

    return material;
}
