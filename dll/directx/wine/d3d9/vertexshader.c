/*
 * IDirect3DVertexShader9 implementation
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

/* IDirect3DVertexShader9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVertexShader9Impl_QueryInterface(LPDIRECT3DVERTEXSHADER9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVertexShader9)) {
        IDirect3DVertexShader9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexShader9Impl_AddRef(LPDIRECT3DVERTEXSHADER9 iface) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice9Ex_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        IWineD3DVertexShader_AddRef(This->wineD3DVertexShader);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DVertexShader9Impl_Release(LPDIRECT3DVERTEXSHADER9 iface) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        IWineD3DVertexShader_Release(This->wineD3DVertexShader);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DVertexShader9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexShader9Impl_GetDevice(IDirect3DVertexShader9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVertexShader9Impl_GetFunction(LPDIRECT3DVERTEXSHADER9 iface, VOID* pData, UINT* pSizeOfData) {
    IDirect3DVertexShader9Impl *This = (IDirect3DVertexShader9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, data %p, data_size %p.\n", iface, pData, pSizeOfData);

    wined3d_mutex_lock();
    hr = IWineD3DVertexShader_GetFunction(This->wineD3DVertexShader, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}


static const IDirect3DVertexShader9Vtbl Direct3DVertexShader9_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexShader9Impl_QueryInterface,
    IDirect3DVertexShader9Impl_AddRef,
    IDirect3DVertexShader9Impl_Release,
    /* IDirect3DVertexShader9 */
    IDirect3DVertexShader9Impl_GetDevice,
    IDirect3DVertexShader9Impl_GetFunction
};

static void STDMETHODCALLTYPE d3d9_vertexshader_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_vertexshader_wined3d_parent_ops =
{
    d3d9_vertexshader_wined3d_object_destroyed,
};

HRESULT vertexshader_init(IDirect3DVertexShader9Impl *shader, IDirect3DDevice9Impl *device, const DWORD *byte_code)
{
    HRESULT hr;

    shader->ref = 1;
    shader->lpVtbl = &Direct3DVertexShader9_Vtbl;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateVertexShader(device->WineD3DDevice, byte_code,
            NULL /* output signature */, &shader->wineD3DVertexShader,
            (IUnknown *)shader, &d3d9_vertexshader_wined3d_parent_ops);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d vertex shader, hr %#x.\n", hr);
        return hr;
    }

    shader->parentDevice = (IDirect3DDevice9Ex *)device;
    IDirect3DDevice9Ex_AddRef(shader->parentDevice);

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShader(LPDIRECT3DDEVICE9EX iface, IDirect3DVertexShader9* pShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("iface %p, shader %p.\n", iface, pShader);

    wined3d_mutex_lock();
    hrc =  IWineD3DDevice_SetVertexShader(This->WineD3DDevice, pShader==NULL?NULL:((IDirect3DVertexShader9Impl *)pShader)->wineD3DVertexShader);
    wined3d_mutex_unlock();

    TRACE("(%p) : returning hr(%u)\n", This, hrc);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShader(LPDIRECT3DDEVICE9EX iface, IDirect3DVertexShader9** ppShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DVertexShader *pShader;
    HRESULT hrc = D3D_OK;

    TRACE("iface %p, shader %p.\n", iface, ppShader);

    wined3d_mutex_lock();
    hrc = IWineD3DDevice_GetVertexShader(This->WineD3DDevice, &pShader);
    if (SUCCEEDED(hrc))
    {
        if (pShader)
        {
            hrc = IWineD3DVertexShader_GetParent(pShader, (IUnknown **)ppShader);
            IWineD3DVertexShader_Release(pShader);
        }
        else
        {
            *ppShader = NULL;
        }
    }
    else
    {
        WARN("(%p) : Call to IWineD3DDevice_GetVertexShader failed %u (device %p)\n", This, hrc, This->WineD3DDevice);
    }
    wined3d_mutex_unlock();

    TRACE("(%p) : returning %p\n", This, *ppShader);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT Register, CONST float* pConstantData, UINT Vector4fCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, Vector4fCount);

    if(Register + Vector4fCount > D3D9_MAX_VERTEX_SHADER_CONSTANTF) {
        WARN("Trying to access %u constants, but d3d9 only supports %u\n",
             Register + Vector4fCount, D3D9_MAX_VERTEX_SHADER_CONSTANTF);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetVertexShaderConstantF(This->WineD3DDevice, Register, pConstantData, Vector4fCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT Register, float* pConstantData, UINT Vector4fCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, Vector4fCount);

    if(Register + Vector4fCount > D3D9_MAX_VERTEX_SHADER_CONSTANTF) {
        WARN("Trying to access %u constants, but d3d9 only supports %u\n",
             Register + Vector4fCount, D3D9_MAX_VERTEX_SHADER_CONSTANTF);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetVertexShaderConstantF(This->WineD3DDevice, Register, pConstantData, Vector4fCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT Register, CONST int* pConstantData, UINT Vector4iCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, Vector4iCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetVertexShaderConstantI(This->WineD3DDevice, Register, pConstantData, Vector4iCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT Register, int* pConstantData, UINT Vector4iCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, Vector4iCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetVertexShaderConstantI(This->WineD3DDevice, Register, pConstantData, Vector4iCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT Register, CONST BOOL* pConstantData, UINT BoolCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, BoolCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetVertexShaderConstantB(This->WineD3DDevice, Register, pConstantData, BoolCount);
    wined3d_mutex_unlock();

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT Register, BOOL* pConstantData, UINT BoolCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, BoolCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetVertexShaderConstantB(This->WineD3DDevice, Register, pConstantData, BoolCount);
    wined3d_mutex_unlock();

    return hr;
}
