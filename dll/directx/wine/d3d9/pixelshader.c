/*
 * IDirect3DPixelShader9 implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
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
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

/* IDirect3DPixelShader9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DPixelShader9Impl_QueryInterface(LPDIRECT3DPIXELSHADER9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DPixelShader9)) {
        IDirect3DPixelShader9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DPixelShader9Impl_AddRef(LPDIRECT3DPIXELSHADER9 iface) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice9Ex_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        IWineD3DPixelShader_AddRef(This->wineD3DPixelShader);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DPixelShader9Impl_Release(LPDIRECT3DPIXELSHADER9 iface) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        IWineD3DPixelShader_Release(This->wineD3DPixelShader);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DPixelShader9 Interface follow: */
static HRESULT WINAPI IDirect3DPixelShader9Impl_GetDevice(IDirect3DPixelShader9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DPixelShader9Impl_GetFunction(LPDIRECT3DPIXELSHADER9 iface, VOID* pData, UINT* pSizeOfData) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, data %p, data_size %p.\n", iface, pData, pSizeOfData);

    wined3d_mutex_lock();
    hr = IWineD3DPixelShader_GetFunction(This->wineD3DPixelShader, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}


static const IDirect3DPixelShader9Vtbl Direct3DPixelShader9_Vtbl =
{
    /* IUnknown */
    IDirect3DPixelShader9Impl_QueryInterface,
    IDirect3DPixelShader9Impl_AddRef,
    IDirect3DPixelShader9Impl_Release,
    /* IDirect3DPixelShader9 */
    IDirect3DPixelShader9Impl_GetDevice,
    IDirect3DPixelShader9Impl_GetFunction
};

static void STDMETHODCALLTYPE d3d9_pixelshader_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_pixelshader_wined3d_parent_ops =
{
    d3d9_pixelshader_wined3d_object_destroyed,
};

HRESULT pixelshader_init(IDirect3DPixelShader9Impl *shader, IDirect3DDevice9Impl *device, const DWORD *byte_code)
{
    HRESULT hr;

    shader->ref = 1;
    shader->lpVtbl = &Direct3DPixelShader9_Vtbl;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreatePixelShader(device->WineD3DDevice, byte_code,
            NULL, &shader->wineD3DPixelShader, (IUnknown *)shader,
            &d3d9_pixelshader_wined3d_parent_ops);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to created wined3d pixel shader, hr %#x.\n", hr);
        return hr;
    }

    shader->parentDevice = (IDirect3DDevice9Ex *)device;
    IDirect3DDevice9Ex_AddRef(shader->parentDevice);

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShader(LPDIRECT3DDEVICE9EX iface, IDirect3DPixelShader9* pShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DPixelShader9Impl *shader = (IDirect3DPixelShader9Impl *)pShader;

    TRACE("iface %p, shader %p.\n", iface, shader);

    wined3d_mutex_lock();
    IWineD3DDevice_SetPixelShader(This->WineD3DDevice, shader == NULL ? NULL :shader->wineD3DPixelShader);
    wined3d_mutex_unlock();

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShader(LPDIRECT3DDEVICE9EX iface, IDirect3DPixelShader9** ppShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DPixelShader *object;
    HRESULT hrc;

    TRACE("iface %p, shader %p.\n", iface, ppShader);

    if (ppShader == NULL) {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hrc = IWineD3DDevice_GetPixelShader(This->WineD3DDevice, &object);
    if (SUCCEEDED(hrc))
    {
        if (object)
        {
            hrc = IWineD3DPixelShader_GetParent(object, (IUnknown **)ppShader);
            IWineD3DPixelShader_Release(object);
        }
        else
        {
            *ppShader = NULL;
        }
    }
    else
    {
        WARN("(%p) : Call to IWineD3DDevice_GetPixelShader failed %u (device %p)\n", This, hrc, This->WineD3DDevice);
    }
    wined3d_mutex_unlock();

    TRACE("(%p) : returning %p\n", This, *ppShader);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT Register, CONST float* pConstantData, UINT Vector4fCount) {
   IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, Vector4fCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetPixelShaderConstantF(This->WineD3DDevice, Register, pConstantData, Vector4fCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT Register, float* pConstantData, UINT Vector4fCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, Vector4fCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetPixelShaderConstantF(This->WineD3DDevice, Register, pConstantData, Vector4fCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT Register, CONST int* pConstantData, UINT Vector4iCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, Vector4iCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetPixelShaderConstantI(This->WineD3DDevice, Register, pConstantData, Vector4iCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT Register, int* pConstantData, UINT Vector4iCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, Vector4iCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetPixelShaderConstantI(This->WineD3DDevice, Register, pConstantData, Vector4iCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT Register, CONST BOOL* pConstantData, UINT BoolCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, BoolCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetPixelShaderConstantB(This->WineD3DDevice, Register, pConstantData, BoolCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT Register, BOOL* pConstantData, UINT BoolCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, BoolCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetPixelShaderConstantB(This->WineD3DDevice, Register, pConstantData, BoolCount);
    wined3d_mutex_unlock();

    return hr;
}
