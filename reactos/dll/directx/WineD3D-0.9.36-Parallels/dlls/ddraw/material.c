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

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS
#define NONAMELESSUNION

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
WINE_DECLARE_DEBUG_CHANNEL(ddraw_thunk);

static void dump_material(const D3DMATERIAL *mat)
{
    DPRINTF("  dwSize : %d\n", mat->dwSize);
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
static HRESULT WINAPI
IDirect3DMaterialImpl_QueryInterface(IDirect3DMaterial3 *iface,
                                     REFIID riid,
                                     LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), obp);

    *obp = NULL;

    if ( IsEqualGUID( &IID_IUnknown,  riid ) ) {
        IDirect3DMaterial_AddRef(ICOM_INTERFACE(This, IDirect3DMaterial));
	*obp = iface;
	TRACE("  Creating IUnknown interface at %p.\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial, riid ) ) {
        IDirect3DMaterial_AddRef(ICOM_INTERFACE(This, IDirect3DMaterial));
        *obp = ICOM_INTERFACE(This, IDirect3DMaterial);
	TRACE("  Creating IDirect3DMaterial interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial2, riid ) ) {
        IDirect3DMaterial_AddRef(ICOM_INTERFACE(This, IDirect3DMaterial));
        *obp = ICOM_INTERFACE(This, IDirect3DMaterial2);
	TRACE("  Creating IDirect3DMaterial2 interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial3, riid ) ) {
        IDirect3DMaterial_AddRef(ICOM_INTERFACE(This, IDirect3DMaterial));
        *obp = ICOM_INTERFACE(This, IDirect3DMaterial3);
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
static ULONG WINAPI
IDirect3DMaterialImpl_AddRef(IDirect3DMaterial3 *iface)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %u.\n", This, ref - 1);

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
static ULONG WINAPI
IDirect3DMaterialImpl_Release(IDirect3DMaterial3 *iface)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %u.\n", This, ref + 1);

    if (!ref)
    {
        if(This->Handle)
        {
            This->ddraw->d3ddevice->Handles[This->Handle - 1].ptr = NULL;
            This->ddraw->d3ddevice->Handles[This->Handle - 1].type = DDrawHandle_Unknown;
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
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial, iface);

    TRACE("(%p)->(%p) no-op...!\n", This, Direct3D);

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
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial, iface);
    TRACE("(%p)->() not implemented\n", This);

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
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial, iface);
    TRACE("(%p)->() not implemented.\n", This);

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
static HRESULT WINAPI
IDirect3DMaterialImpl_SetMaterial(IDirect3DMaterial3 *iface,
                                  D3DMATERIAL *lpMat)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    TRACE("(%p)->(%p)\n", This, lpMat);
    if (TRACE_ON(d3d7))
        dump_material(lpMat);

    /* Stores the material */
    memset(&This->mat, 0, sizeof(This->mat));
    memcpy(&This->mat, lpMat, lpMat->dwSize);
    
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
static HRESULT WINAPI
IDirect3DMaterialImpl_GetMaterial(IDirect3DMaterial3 *iface,
                                  D3DMATERIAL *lpMat)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    DWORD dwSize;
    TRACE("(%p)->(%p)\n", This, lpMat);
    if (TRACE_ON(d3d7)) {
        TRACE("  Returning material : ");
        dump_material(&This->mat);
    }

    /* Copies the material structure */
    dwSize = lpMat->dwSize;
    memset(lpMat, 0, dwSize);
    memcpy(lpMat, &This->mat, dwSize);

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
static HRESULT WINAPI
IDirect3DMaterialImpl_GetHandle(IDirect3DMaterial3 *iface,
                                IDirect3DDevice3 *lpDirect3DDevice3,
                                D3DMATERIALHANDLE *lpHandle)
{
    ICOM_THIS_FROM(IDirect3DMaterialImpl, IDirect3DMaterial3, iface);
    IDirect3DDeviceImpl *device = ICOM_OBJECT(IDirect3DDeviceImpl, IDirect3DDevice3, lpDirect3DDevice3);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, device, lpHandle);

    This->active_device = device;
    if(!This->Handle)
    {
        This->Handle = IDirect3DDeviceImpl_CreateHandle(device);
        if(!This->Handle)
        {
            ERR("Error creating a handle\n");
            return DDERR_INVALIDPARAMS;   /* Unchecked */
        }
        device->Handles[This->Handle - 1].ptr = This;
        device->Handles[This->Handle - 1].type = DDrawHandle_Material;
    }
    *lpHandle = This->Handle;
    TRACE(" returning handle %08x.\n", *lpHandle);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_2_GetHandle(LPDIRECT3DMATERIAL2 iface,
					LPDIRECT3DDEVICE2 lpDirect3DDevice2,
					LPD3DMATERIALHANDLE lpHandle)
{
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpDirect3DDevice2, lpHandle);
    return IDirect3DMaterial3_GetHandle(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface),
					COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice2, IDirect3DDevice3, lpDirect3DDevice2),
					lpHandle);
}

static HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_1_GetHandle(LPDIRECT3DMATERIAL iface,
					LPDIRECT3DDEVICE lpDirect3DDevice,
					LPD3DMATERIALHANDLE lpHandle)
{
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpDirect3DDevice, lpHandle);
    return IDirect3DMaterial3_GetHandle(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface),
					COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice3, lpDirect3DDevice),
					lpHandle);
}

static HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_2_QueryInterface(LPDIRECT3DMATERIAL2 iface,
                                             REFIID riid,
                                             LPVOID* obp)
{
    TRACE_(ddraw_thunk)("(%p)->(%s,%p) thunking to IDirect3DMaterial3 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirect3DMaterial3_QueryInterface(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface),
                                             riid,
                                             obp);
}

static HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_1_QueryInterface(LPDIRECT3DMATERIAL iface,
                                             REFIID riid,
                                             LPVOID* obp)
{
    TRACE_(ddraw_thunk)("(%p)->(%s,%p) thunking to IDirect3DMaterial3 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirect3DMaterial3_QueryInterface(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface),
                                             riid,
                                             obp);
}

static ULONG WINAPI
Thunk_IDirect3DMaterialImpl_2_AddRef(LPDIRECT3DMATERIAL2 iface)
{
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DMaterial3 interface.\n", iface);
    return IDirect3DMaterial3_AddRef(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface));
}

static ULONG WINAPI
Thunk_IDirect3DMaterialImpl_1_AddRef(LPDIRECT3DMATERIAL iface)
{
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DMaterial3 interface.\n", iface);
    return IDirect3DMaterial3_AddRef(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface));
}

static ULONG WINAPI
Thunk_IDirect3DMaterialImpl_2_Release(LPDIRECT3DMATERIAL2 iface)
{
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DMaterial3 interface.\n", iface);
    return IDirect3DMaterial3_Release(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface));
}

static ULONG WINAPI
Thunk_IDirect3DMaterialImpl_1_Release(LPDIRECT3DMATERIAL iface)
{
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DMaterial3 interface.\n", iface);
    return IDirect3DMaterial3_Release(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface));
}

static HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_2_SetMaterial(LPDIRECT3DMATERIAL2 iface,
                                          LPD3DMATERIAL lpMat)
{
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpMat);
    return IDirect3DMaterial3_SetMaterial(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface),
                                          lpMat);
}

static HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_1_SetMaterial(LPDIRECT3DMATERIAL iface,
                                          LPD3DMATERIAL lpMat)
{
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpMat);
    return IDirect3DMaterial3_SetMaterial(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface),
                                          lpMat);
}

static HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_2_GetMaterial(LPDIRECT3DMATERIAL2 iface,
                                          LPD3DMATERIAL lpMat)
{
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpMat);
    return IDirect3DMaterial3_GetMaterial(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial2, IDirect3DMaterial3, iface),
                                          lpMat);
}

static HRESULT WINAPI
Thunk_IDirect3DMaterialImpl_1_GetMaterial(LPDIRECT3DMATERIAL iface,
                                          LPD3DMATERIAL lpMat)
{
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DMaterial3 interface.\n", iface, lpMat);
    return IDirect3DMaterial3_GetMaterial(COM_INTERFACE_CAST(IDirect3DMaterialImpl, IDirect3DMaterial, IDirect3DMaterial3, iface),
                                          lpMat);
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
#ifndef WINE_NATIVEWIN32
    d3d7mat.u.diffuse = This->mat.u.diffuse;
    d3d7mat.u1.ambient = This->mat.u1.ambient;
    d3d7mat.u2.specular = This->mat.u2.specular;
    d3d7mat.u3.emissive = This->mat.u3.emissive;
    d3d7mat.u4.power = This->mat.u4.power;
#else
    d3d7mat.diffuse = This->mat.diffuse;
    d3d7mat.ambient = This->mat.ambient;
    d3d7mat.specular = This->mat.specular;
    d3d7mat.emissive = This->mat.emissive;
    d3d7mat.power = This->mat.power;
#endif

    IDirect3DDevice7_SetMaterial(ICOM_INTERFACE(This->active_device, IDirect3DDevice7),
                                 &d3d7mat);
}

const IDirect3DMaterial3Vtbl IDirect3DMaterial3_Vtbl =
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

const IDirect3DMaterial2Vtbl IDirect3DMaterial2_Vtbl =
{
    /*** IUnknown Methods ***/
    Thunk_IDirect3DMaterialImpl_2_QueryInterface,
    Thunk_IDirect3DMaterialImpl_2_AddRef,
    Thunk_IDirect3DMaterialImpl_2_Release,
    /*** IDirect3DMaterial2 Methods ***/
    Thunk_IDirect3DMaterialImpl_2_SetMaterial,
    Thunk_IDirect3DMaterialImpl_2_GetMaterial,
    Thunk_IDirect3DMaterialImpl_2_GetHandle,
};

const IDirect3DMaterialVtbl IDirect3DMaterial_Vtbl =
{
    /*** IUnknown Methods ***/
    Thunk_IDirect3DMaterialImpl_1_QueryInterface,
    Thunk_IDirect3DMaterialImpl_1_AddRef,
    Thunk_IDirect3DMaterialImpl_1_Release,
    /*** IDirect3DMaterial1 Methods ***/
    IDirect3DMaterialImpl_Initialize,
    Thunk_IDirect3DMaterialImpl_1_SetMaterial,
    Thunk_IDirect3DMaterialImpl_1_GetMaterial,
    Thunk_IDirect3DMaterialImpl_1_GetHandle,
    IDirect3DMaterialImpl_Reserve,
    IDirect3DMaterialImpl_Unreserve
};
