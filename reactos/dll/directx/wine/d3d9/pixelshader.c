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

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DPixelShader9Impl_Release(LPDIRECT3DPIXELSHADER9 iface) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        EnterCriticalSection(&d3d9_cs);
        IWineD3DPixelShader_Release(This->wineD3DPixelShader);
        LeaveCriticalSection(&d3d9_cs);
        IDirect3DDevice9Ex_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DPixelShader9 Interface follow: */
static HRESULT WINAPI IDirect3DPixelShader9Impl_GetDevice(LPDIRECT3DPIXELSHADER9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    IWineD3DDevice *myDevice = NULL;

    TRACE("(%p) : Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    IWineD3DPixelShader_GetDevice(This->wineD3DPixelShader, &myDevice);
    IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
    IWineD3DDevice_Release(myDevice);
    LeaveCriticalSection(&d3d9_cs);
    TRACE("(%p) returning (%p)\n", This, *ppDevice);
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DPixelShader9Impl_GetFunction(LPDIRECT3DPIXELSHADER9 iface, VOID* pData, UINT* pSizeOfData) {
    IDirect3DPixelShader9Impl *This = (IDirect3DPixelShader9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DPixelShader_GetFunction(This->wineD3DPixelShader, pData, pSizeOfData);
    LeaveCriticalSection(&d3d9_cs);
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


/* IDirect3DDevice9 IDirect3DPixelShader9 Methods follow:  */
HRESULT WINAPI IDirect3DDevice9Impl_CreatePixelShader(LPDIRECT3DDEVICE9EX iface, CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DPixelShader9Impl *object;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    if (ppShader == NULL) {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));

    if (NULL == object) {
        FIXME("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return E_OUTOFMEMORY;
    }

    object->ref    = 1;
    object->lpVtbl = &Direct3DPixelShader9_Vtbl;
    EnterCriticalSection(&d3d9_cs);
    hrc = IWineD3DDevice_CreatePixelShader(This->WineD3DDevice, pFunction, NULL,
            &object->wineD3DPixelShader, (IUnknown *)object);
    LeaveCriticalSection(&d3d9_cs);
    if (hrc != D3D_OK) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreatePixelShader failed\n", This);
        HeapFree(GetProcessHeap(), 0 , object);
    } else {
        IDirect3DDevice9Ex_AddRef(iface);
        object->parentDevice = iface;
        *ppShader = (IDirect3DPixelShader9*) object;
        TRACE("(%p) : Created pixel shader %p\n", This, object);
    }

    TRACE("(%p) : returning %p\n", This, *ppShader);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShader(LPDIRECT3DDEVICE9EX iface, IDirect3DPixelShader9* pShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DPixelShader9Impl *shader = (IDirect3DPixelShader9Impl *)pShader;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    IWineD3DDevice_SetPixelShader(This->WineD3DDevice, shader == NULL ? NULL :shader->wineD3DPixelShader);
    LeaveCriticalSection(&d3d9_cs);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShader(LPDIRECT3DDEVICE9EX iface, IDirect3DPixelShader9** ppShader) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DPixelShader *object;

    HRESULT hrc = D3D_OK;
    TRACE("(%p) Relay\n", This);
    if (ppShader == NULL) {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d9_cs);
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
    LeaveCriticalSection(&d3d9_cs);

    TRACE("(%p) : returning %p\n", This, *ppShader);
    return hrc;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT Register, CONST float* pConstantData, UINT Vector4fCount) {
   IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);   

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetPixelShaderConstantF(This->WineD3DDevice, Register, pConstantData, Vector4fCount);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT Register, float* pConstantData, UINT Vector4fCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetPixelShaderConstantF(This->WineD3DDevice, Register, pConstantData, Vector4fCount);
    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT Register, CONST int* pConstantData, UINT Vector4iCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetPixelShaderConstantI(This->WineD3DDevice, Register, pConstantData, Vector4iCount);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT Register, int* pConstantData, UINT Vector4iCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetPixelShaderConstantI(This->WineD3DDevice, Register, pConstantData, Vector4iCount);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT Register, CONST BOOL* pConstantData, UINT BoolCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetPixelShaderConstantB(This->WineD3DDevice, Register, pConstantData, BoolCount);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT Register, BOOL* pConstantData, UINT BoolCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetPixelShaderConstantB(This->WineD3DDevice, Register, pConstantData, BoolCount);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}
