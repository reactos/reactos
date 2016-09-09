/*
 * Copyright 2010 Christian Costa
 * Copyright 2011 Rico Schüller
 * Copyright 2015 Sebastian Lackner
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
#define NONAMELESSUNION
#include "wine/debug.h"
#define INITGUID
#include "initguid.h"
#define COBJMACROS
#include "wingdi.h"
#include "d3dx9.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

DEFINE_GUID(IID_ID3DXEffect25, 0xd165ccb1, 0x62b0, 0x4a33, 0xb3, 0xfa, 0xa9, 0x23, 0x0, 0x30, 0x5a, 0x11);

#define INTERFACE ID3DXEffect25
DECLARE_INTERFACE_(ID3DXEffect25, ID3DXBaseEffect)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DXBaseEffect methods ***/
    STDMETHOD(GetDesc)(THIS_ D3DXEFFECT_DESC* desc) PURE;
    STDMETHOD(GetParameterDesc)(THIS_ D3DXHANDLE parameter, D3DXPARAMETER_DESC* desc) PURE;
    STDMETHOD(GetTechniqueDesc)(THIS_ D3DXHANDLE technique, D3DXTECHNIQUE_DESC* desc) PURE;
    STDMETHOD(GetPassDesc)(THIS_ D3DXHANDLE pass, D3DXPASS_DESC* desc) PURE;
    STDMETHOD(GetFunctionDesc)(THIS_ D3DXHANDLE shader, D3DXFUNCTION_DESC* desc) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameter)(THIS_ D3DXHANDLE parameter, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterByName)(THIS_ D3DXHANDLE parameter, const char *name) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterBySemantic)(THIS_ D3DXHANDLE parameter, const char *semantic) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterElement)(THIS_ D3DXHANDLE parameter, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechnique)(THIS_ UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechniqueByName)(THIS_ const char *name) PURE;
    STDMETHOD_(D3DXHANDLE, GetPass)(THIS_ D3DXHANDLE technique, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetPassByName)(THIS_ D3DXHANDLE technique, const char *name) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunction)(THIS_ UINT index);
    STDMETHOD_(D3DXHANDLE, GetFunctionByName)(THIS_ const char *name);
    STDMETHOD_(D3DXHANDLE, GetAnnotation)(THIS_ D3DXHANDLE object, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotationByName)(THIS_ D3DXHANDLE object, const char *name) PURE;
    STDMETHOD(SetValue)(THIS_ D3DXHANDLE parameter, const void *data, UINT bytes) PURE;
    STDMETHOD(GetValue)(THIS_ D3DXHANDLE parameter, void *data, UINT bytes) PURE;
    STDMETHOD(SetBool)(THIS_ D3DXHANDLE parameter, BOOL b) PURE;
    STDMETHOD(GetBool)(THIS_ D3DXHANDLE parameter, BOOL* b) PURE;
    STDMETHOD(SetBoolArray)(THIS_ D3DXHANDLE parameter, const BOOL *b, UINT count) PURE;
    STDMETHOD(GetBoolArray)(THIS_ D3DXHANDLE parameter, BOOL* b, UINT count) PURE;
    STDMETHOD(SetInt)(THIS_ D3DXHANDLE parameter, INT n) PURE;
    STDMETHOD(GetInt)(THIS_ D3DXHANDLE parameter, INT* n) PURE;
    STDMETHOD(SetIntArray)(THIS_ D3DXHANDLE parameter, const INT *n, UINT count) PURE;
    STDMETHOD(GetIntArray)(THIS_ D3DXHANDLE parameter, INT* n, UINT count) PURE;
    STDMETHOD(SetFloat)(THIS_ D3DXHANDLE parameter, FLOAT f) PURE;
    STDMETHOD(GetFloat)(THIS_ D3DXHANDLE parameter, FLOAT* f) PURE;
    STDMETHOD(SetFloatArray)(THIS_ D3DXHANDLE parameter, const FLOAT *f, UINT count) PURE;
    STDMETHOD(GetFloatArray)(THIS_ D3DXHANDLE parameter, FLOAT* f, UINT count) PURE;
    STDMETHOD(SetVector)(THIS_ D3DXHANDLE parameter, const D3DXVECTOR4 *vector) PURE;
    STDMETHOD(GetVector)(THIS_ D3DXHANDLE parameter, D3DXVECTOR4* vector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ D3DXHANDLE parameter, const D3DXVECTOR4 *vector, UINT count) PURE;
    STDMETHOD(GetVectorArray)(THIS_ D3DXHANDLE parameter, D3DXVECTOR4* vector, UINT count) PURE;
    STDMETHOD(SetMatrix)(THIS_ D3DXHANDLE parameter, const D3DXMATRIX *matrix) PURE;
    STDMETHOD(GetMatrix)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count) PURE;
    STDMETHOD(GetMatrixArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count) PURE;
    STDMETHOD(GetMatrixPointerArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ D3DXHANDLE parameter, const D3DXMATRIX *matrix) PURE;
    STDMETHOD(GetMatrixTranspose)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count) PURE;
    STDMETHOD(GetMatrixTransposeArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count) PURE;
    STDMETHOD(GetMatrixTransposePointerArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(SetString)(THIS_ D3DXHANDLE parameter, const char *string) PURE;
    STDMETHOD(GetString)(THIS_ D3DXHANDLE parameter, const char **string) PURE;
    STDMETHOD(SetTexture)(THIS_ D3DXHANDLE parameter, struct IDirect3DBaseTexture9 *texture) PURE;
    STDMETHOD(GetTexture)(THIS_ D3DXHANDLE parameter, struct IDirect3DBaseTexture9 **texture) PURE;
    STDMETHOD(GetPixelShader)(THIS_ D3DXHANDLE parameter, struct IDirect3DPixelShader9 **shader) PURE;
    STDMETHOD(GetVertexShader)(THIS_ D3DXHANDLE parameter, struct IDirect3DVertexShader9 **shader) PURE;
    STDMETHOD(SetArrayRange)(THIS_ D3DXHANDLE parameter, UINT start, UINT end) PURE;
    /*** ID3DXEffect methods ***/
    STDMETHOD(GetPool)(THIS_ ID3DXEffectPool **pool) PURE;
    STDMETHOD(SetTechnique)(THIS_ D3DXHANDLE technique) PURE;
    STDMETHOD_(D3DXHANDLE, GetCurrentTechnique)(THIS) PURE;
    STDMETHOD(ValidateTechnique)(THIS_ D3DXHANDLE technique) PURE;
    STDMETHOD(FindNextValidTechnique)(THIS_ D3DXHANDLE technique, D3DXHANDLE* next_technique) PURE;
    STDMETHOD_(BOOL, IsParameterUsed)(THIS_ D3DXHANDLE parameter, D3DXHANDLE technique) PURE;
    STDMETHOD(Begin)(THIS_ UINT *passes, DWORD flags) PURE;
    STDMETHOD(BeginPass)(THIS_ UINT pass) PURE;
    STDMETHOD(CommitChanges)(THIS) PURE;
    STDMETHOD(EndPass)(THIS) PURE;
    STDMETHOD(End)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ struct IDirect3DDevice9 **device) PURE;
    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
    STDMETHOD(SetStateManager)(THIS_ ID3DXEffectStateManager *manager) PURE;
    STDMETHOD(GetStateManager)(THIS_ ID3DXEffectStateManager **manager) PURE;
    STDMETHOD(BeginParameterBlock)(THIS) PURE;
    STDMETHOD_(D3DXHANDLE, EndParameterBlock)(THIS) PURE;
    STDMETHOD(ApplyParameterBlock)(THIS_ D3DXHANDLE parameter_block) PURE;
    /* DeleteParameterBlock missing */
    STDMETHOD(CloneEffect)(THIS_ struct IDirect3DDevice9 *device, struct ID3DXEffect25 **effect) PURE;
    /* SetRawValue missing */
};
#undef INTERFACE

struct ID3DXEffect25Impl
{
    ID3DXEffect25 ID3DXEffect25_iface;
    ID3DXEffect *effect;
    LONG ref;
};

static const struct ID3DXEffect25Vtbl ID3DXEffect25_Vtbl;

static inline struct ID3DXEffect25Impl *impl_from_ID3DXEffect25(ID3DXEffect25 *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXEffect25Impl, ID3DXEffect25_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXEffect25Impl_QueryInterface(ID3DXEffect25 *iface, REFIID riid, void **object)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);

    TRACE("iface %p, riid %s, object %p\n", This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXEffect25))
    {
        iface->lpVtbl->AddRef(iface);
        *object = This;
        return S_OK;
    }

    ERR("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXEffect25Impl_AddRef(ID3DXEffect25 *iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);

    TRACE("iface %p: AddRef from %u\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXEffect25Impl_Release(ID3DXEffect25 *iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("iface %p: Release from %u\n", This, ref + 1);

    if (!ref)
    {
        ID3DXEffect *effect = This->effect;

        effect->lpVtbl->Release(effect);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBaseEffect methods ***/
static HRESULT WINAPI ID3DXEffect25Impl_GetDesc(ID3DXEffect25 *iface, D3DXEFFECT_DESC *desc)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetDesc(effect, desc);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetParameterDesc(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXPARAMETER_DESC *desc)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetParameterDesc(effect, parameter, desc);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetTechniqueDesc(ID3DXEffect25 *iface, D3DXHANDLE technique, D3DXTECHNIQUE_DESC *desc)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetTechniqueDesc(effect, technique, desc);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetPassDesc(ID3DXEffect25 *iface, D3DXHANDLE pass, D3DXPASS_DESC *desc)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetPassDesc(effect, pass, desc);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetFunctionDesc(ID3DXEffect25 *iface, D3DXHANDLE shader, D3DXFUNCTION_DESC *desc)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetFunctionDesc(effect, shader, desc);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetParameter(ID3DXEffect25 *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetParameter(effect, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetParameterByName(ID3DXEffect25 *iface, D3DXHANDLE parameter, LPCSTR name)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetParameterByName(effect, parameter, name);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetParameterBySemantic(ID3DXEffect25 *iface, D3DXHANDLE parameter, LPCSTR semantic)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetParameterBySemantic(effect, parameter, semantic);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetParameterElement(ID3DXEffect25 *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetParameterElement(effect, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetTechnique(ID3DXEffect25 *iface, UINT index)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetTechnique(effect, index);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetTechniqueByName(ID3DXEffect25 *iface, LPCSTR name)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetTechniqueByName(effect, name);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetPass(ID3DXEffect25 *iface, D3DXHANDLE technique, UINT index)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetPass(effect, technique, index);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetPassByName(ID3DXEffect25 *iface, D3DXHANDLE technique, LPCSTR name)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetPassByName(effect, technique, name);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetFunction(ID3DXEffect25 *iface, UINT index)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetFunction(effect, index);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetFunctionByName(ID3DXEffect25 *iface, LPCSTR name)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetFunctionByName(effect, name);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetAnnotation(ID3DXEffect25 *iface, D3DXHANDLE object, UINT index)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetAnnotation(effect, object, index);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetAnnotationByName(ID3DXEffect25 *iface, D3DXHANDLE object, LPCSTR name)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetAnnotationByName(effect, object, name);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetValue(ID3DXEffect25 *iface, D3DXHANDLE parameter, LPCVOID data, UINT bytes)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetValue(effect, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetValue(ID3DXEffect25 *iface, D3DXHANDLE parameter, LPVOID data, UINT bytes)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetValue(effect, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetBool(ID3DXEffect25 *iface, D3DXHANDLE parameter, BOOL b)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetBool(effect, parameter, b);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetBool(ID3DXEffect25 *iface, D3DXHANDLE parameter, BOOL *b)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetBool(effect, parameter, b);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetBoolArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, const BOOL *b, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetBoolArray(effect, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetBoolArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, BOOL *b, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetBoolArray(effect, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetInt(ID3DXEffect25 *iface, D3DXHANDLE parameter, INT n)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetInt(effect, parameter, n);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetInt(ID3DXEffect25 *iface, D3DXHANDLE parameter, INT *n)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetInt(effect, parameter, n);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetIntArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, const INT *n, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetIntArray(effect, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetIntArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, INT *n, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetIntArray(effect, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetFloat(ID3DXEffect25 *iface, D3DXHANDLE parameter, FLOAT f)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetFloat(effect, parameter, f);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetFloat(ID3DXEffect25 *iface, D3DXHANDLE parameter, FLOAT *f)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetFloat(effect, parameter, f);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetFloatArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, const FLOAT *f, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetFloatArray(effect, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetFloatArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, FLOAT *f, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetFloatArray(effect, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetVector(ID3DXEffect25 *iface, D3DXHANDLE parameter, const D3DXVECTOR4 *vector)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetVector(effect, parameter, vector);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetVector(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXVECTOR4 *vector)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetVector(effect, parameter, vector);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetVectorArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, const D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetVectorArray(effect, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetVectorArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetVectorArray(effect, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetMatrix(ID3DXEffect25 *iface, D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetMatrix(effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetMatrix(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetMatrix(effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetMatrixArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetMatrixArray(effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetMatrixArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetMatrixArray(effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetMatrixPointerArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetMatrixPointerArray(effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetMatrixPointerArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetMatrixPointerArray(effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetMatrixTranspose(ID3DXEffect25 *iface, D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetMatrixTranspose(effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetMatrixTranspose(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetMatrixTranspose(effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetMatrixTransposeArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetMatrixTransposeArray(effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetMatrixTransposeArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetMatrixTransposeArray(effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetMatrixTransposePointerArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetMatrixTransposePointerArray(effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetMatrixTransposePointerArray(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetMatrixTransposePointerArray(effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetString(ID3DXEffect25 *iface, D3DXHANDLE parameter, LPCSTR string)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetString(effect, parameter, string);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetString(ID3DXEffect25 *iface, D3DXHANDLE parameter, LPCSTR *string)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetString(effect, parameter, string);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetTexture(ID3DXEffect25 *iface, D3DXHANDLE parameter, IDirect3DBaseTexture9 *texture)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetTexture(effect, parameter, texture);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetTexture(ID3DXEffect25 *iface, D3DXHANDLE parameter, IDirect3DBaseTexture9 **texture)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetTexture(effect, parameter, texture);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetPixelShader(ID3DXEffect25 *iface, D3DXHANDLE parameter, IDirect3DPixelShader9 **pshader)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetPixelShader(effect, parameter, pshader);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetVertexShader(ID3DXEffect25 *iface, D3DXHANDLE parameter, IDirect3DVertexShader9 **vshader)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetVertexShader(effect, parameter, vshader);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetArrayRange(ID3DXEffect25 *iface, D3DXHANDLE parameter, UINT start, UINT end)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetArrayRange(effect, parameter, start, end);
}

/*** ID3DXEffect25 methods ***/
static HRESULT WINAPI ID3DXEffect25Impl_GetPool(ID3DXEffect25 *iface, ID3DXEffectPool **pool)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetPool(effect, pool);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetTechnique(ID3DXEffect25 *iface, D3DXHANDLE technique)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetTechnique(effect, technique);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_GetCurrentTechnique(ID3DXEffect25 *iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetCurrentTechnique(effect);
}

static HRESULT WINAPI ID3DXEffect25Impl_ValidateTechnique(ID3DXEffect25 *iface, D3DXHANDLE technique)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->ValidateTechnique(effect, technique);
}

static HRESULT WINAPI ID3DXEffect25Impl_FindNextValidTechnique(ID3DXEffect25 *iface, D3DXHANDLE technique, D3DXHANDLE *next_technique)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->FindNextValidTechnique(effect, technique, next_technique);
}

static BOOL WINAPI ID3DXEffect25Impl_IsParameterUsed(ID3DXEffect25 *iface, D3DXHANDLE parameter, D3DXHANDLE technique)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->IsParameterUsed(effect, parameter, technique);
}

static HRESULT WINAPI ID3DXEffect25Impl_Begin(ID3DXEffect25 *iface, UINT *passes, DWORD flags)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->Begin(effect, passes, flags);
}

static HRESULT WINAPI ID3DXEffect25Impl_BeginPass(ID3DXEffect25 *iface, UINT pass)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->BeginPass(effect, pass);
}

static HRESULT WINAPI ID3DXEffect25Impl_CommitChanges(ID3DXEffect25 *iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->CommitChanges(effect);
}

static HRESULT WINAPI ID3DXEffect25Impl_EndPass(ID3DXEffect25 *iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->EndPass(effect);
}

static HRESULT WINAPI ID3DXEffect25Impl_End(ID3DXEffect25 *iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->End(effect);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetDevice(ID3DXEffect25 *iface, IDirect3DDevice9 **device)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetDevice(effect, device);
}

static HRESULT WINAPI ID3DXEffect25Impl_OnLostDevice(ID3DXEffect25* iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->OnLostDevice(effect);
}

static HRESULT WINAPI ID3DXEffect25Impl_OnResetDevice(ID3DXEffect25* iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->OnResetDevice(effect);
}

static HRESULT WINAPI ID3DXEffect25Impl_SetStateManager(ID3DXEffect25 *iface, ID3DXEffectStateManager *manager)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->SetStateManager(effect, manager);
}

static HRESULT WINAPI ID3DXEffect25Impl_GetStateManager(ID3DXEffect25 *iface, ID3DXEffectStateManager **manager)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->GetStateManager(effect, manager);
}

static HRESULT WINAPI ID3DXEffect25Impl_BeginParameterBlock(ID3DXEffect25 *iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->BeginParameterBlock(effect);
}

static D3DXHANDLE WINAPI ID3DXEffect25Impl_EndParameterBlock(ID3DXEffect25 *iface)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->EndParameterBlock(effect);
}

static HRESULT WINAPI ID3DXEffect25Impl_ApplyParameterBlock(ID3DXEffect25 *iface, D3DXHANDLE parameter_block)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    return effect->lpVtbl->ApplyParameterBlock(effect, parameter_block);
}

static HRESULT WINAPI ID3DXEffect25Impl_CloneEffect(ID3DXEffect25 *iface, IDirect3DDevice9 *device, ID3DXEffect25 **clone)
{
    struct ID3DXEffect25Impl *This = impl_from_ID3DXEffect25(iface);
    ID3DXEffect *effect = This->effect;
    struct ID3DXEffect25Impl *object;
    HRESULT hr;

    TRACE("Forward iface %p, effect %p\n", This, effect);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->ID3DXEffect25_iface.lpVtbl = &ID3DXEffect25_Vtbl;
    object->ref = 1;

    hr = effect->lpVtbl->CloneEffect(effect, device, &object->effect);
    if (FAILED(hr))
    {
        WARN("Failed to clone effect\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *clone = &object->ID3DXEffect25_iface;

    TRACE("Created ID3DXEffect %p\n", clone);

    return hr;
}

static const struct ID3DXEffect25Vtbl ID3DXEffect25_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXEffect25Impl_QueryInterface,
    ID3DXEffect25Impl_AddRef,
    ID3DXEffect25Impl_Release,
    /*** ID3DXBaseEffect methods ***/
    ID3DXEffect25Impl_GetDesc,
    ID3DXEffect25Impl_GetParameterDesc,
    ID3DXEffect25Impl_GetTechniqueDesc,
    ID3DXEffect25Impl_GetPassDesc,
    ID3DXEffect25Impl_GetFunctionDesc,
    ID3DXEffect25Impl_GetParameter,
    ID3DXEffect25Impl_GetParameterByName,
    ID3DXEffect25Impl_GetParameterBySemantic,
    ID3DXEffect25Impl_GetParameterElement,
    ID3DXEffect25Impl_GetTechnique,
    ID3DXEffect25Impl_GetTechniqueByName,
    ID3DXEffect25Impl_GetPass,
    ID3DXEffect25Impl_GetPassByName,
    ID3DXEffect25Impl_GetFunction,
    ID3DXEffect25Impl_GetFunctionByName,
    ID3DXEffect25Impl_GetAnnotation,
    ID3DXEffect25Impl_GetAnnotationByName,
    ID3DXEffect25Impl_SetValue,
    ID3DXEffect25Impl_GetValue,
    ID3DXEffect25Impl_SetBool,
    ID3DXEffect25Impl_GetBool,
    ID3DXEffect25Impl_SetBoolArray,
    ID3DXEffect25Impl_GetBoolArray,
    ID3DXEffect25Impl_SetInt,
    ID3DXEffect25Impl_GetInt,
    ID3DXEffect25Impl_SetIntArray,
    ID3DXEffect25Impl_GetIntArray,
    ID3DXEffect25Impl_SetFloat,
    ID3DXEffect25Impl_GetFloat,
    ID3DXEffect25Impl_SetFloatArray,
    ID3DXEffect25Impl_GetFloatArray,
    ID3DXEffect25Impl_SetVector,
    ID3DXEffect25Impl_GetVector,
    ID3DXEffect25Impl_SetVectorArray,
    ID3DXEffect25Impl_GetVectorArray,
    ID3DXEffect25Impl_SetMatrix,
    ID3DXEffect25Impl_GetMatrix,
    ID3DXEffect25Impl_SetMatrixArray,
    ID3DXEffect25Impl_GetMatrixArray,
    ID3DXEffect25Impl_SetMatrixPointerArray,
    ID3DXEffect25Impl_GetMatrixPointerArray,
    ID3DXEffect25Impl_SetMatrixTranspose,
    ID3DXEffect25Impl_GetMatrixTranspose,
    ID3DXEffect25Impl_SetMatrixTransposeArray,
    ID3DXEffect25Impl_GetMatrixTransposeArray,
    ID3DXEffect25Impl_SetMatrixTransposePointerArray,
    ID3DXEffect25Impl_GetMatrixTransposePointerArray,
    ID3DXEffect25Impl_SetString,
    ID3DXEffect25Impl_GetString,
    ID3DXEffect25Impl_SetTexture,
    ID3DXEffect25Impl_GetTexture,
    ID3DXEffect25Impl_GetPixelShader,
    ID3DXEffect25Impl_GetVertexShader,
    ID3DXEffect25Impl_SetArrayRange,
    /*** ID3DXEffect25 methods ***/
    ID3DXEffect25Impl_GetPool,
    ID3DXEffect25Impl_SetTechnique,
    ID3DXEffect25Impl_GetCurrentTechnique,
    ID3DXEffect25Impl_ValidateTechnique,
    ID3DXEffect25Impl_FindNextValidTechnique,
    ID3DXEffect25Impl_IsParameterUsed,
    ID3DXEffect25Impl_Begin,
    ID3DXEffect25Impl_BeginPass,
    ID3DXEffect25Impl_CommitChanges,
    ID3DXEffect25Impl_EndPass,
    ID3DXEffect25Impl_End,
    ID3DXEffect25Impl_GetDevice,
    ID3DXEffect25Impl_OnLostDevice,
    ID3DXEffect25Impl_OnResetDevice,
    ID3DXEffect25Impl_SetStateManager,
    ID3DXEffect25Impl_GetStateManager,
    ID3DXEffect25Impl_BeginParameterBlock,
    ID3DXEffect25Impl_EndParameterBlock,
    ID3DXEffect25Impl_ApplyParameterBlock,
    ID3DXEffect25Impl_CloneEffect,
};

HRESULT WINAPI D3DXCreateEffectEx25(struct IDirect3DDevice9 *device, const void *srcdata, UINT srcdatalen,
        const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skip_constants, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    struct ID3DXEffect25Impl *object;
    HRESULT hr;

    TRACE("device %p, srcdata %p, srcdatalen %u, defines %p, include %p, "
            "skip_constants %s, flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, srcdata, srcdatalen, defines, include,
            debugstr_a(skip_constants), flags, pool, effect, compilation_errors);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->ID3DXEffect25_iface.lpVtbl = &ID3DXEffect25_Vtbl;
    object->ref = 1;

    hr = D3DXCreateEffectEx(device, srcdata, srcdatalen, defines, include,
            skip_constants, flags, pool, &object->effect, compilation_errors);
    if (FAILED(hr))
    {
        WARN("Failed to create effect\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *effect = &object->ID3DXEffect25_iface;
    TRACE("Created ID3DXEffect25 %p\n", object);
    return hr;
}

HRESULT WINAPI D3DXCreateEffect25(struct IDirect3DDevice9 *device, const void *srcdata, UINT srcdatalen,
        const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    TRACE("device %p, srcdata %p, srcdatalen %u, defines %p, include %p, "
            "flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, srcdata, srcdatalen, defines, include, flags, pool, effect, compilation_errors);

    return D3DXCreateEffectEx25(device, srcdata, srcdatalen, defines, include, NULL, flags, pool, effect, compilation_errors);
}


HRESULT WINAPI D3DXCreateEffectFromFileExW25(struct IDirect3DDevice9 *device, const WCHAR *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skip_constants, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    struct ID3DXEffect25Impl *object;
    HRESULT hr;

    TRACE("device %p, srcfile %s, defines %p, include %p, skip_constants %s, "
            "flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, debugstr_w(srcfile), defines, include, debugstr_a(skip_constants),
            flags, pool, effect, compilation_errors);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->ID3DXEffect25_iface.lpVtbl = &ID3DXEffect25_Vtbl;
    object->ref = 1;

    hr = D3DXCreateEffectFromFileExW(device, srcfile, defines, include,
            skip_constants, flags, pool, &object->effect, compilation_errors);
    if (FAILED(hr))
    {
        WARN("Failed to create effect\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *effect = &object->ID3DXEffect25_iface;
    TRACE("Created ID3DXEffect25 %p\n", object);
    return hr;
}

HRESULT WINAPI D3DXCreateEffectFromFileExA25(struct IDirect3DDevice9 *device, const char *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skip_constants, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    struct ID3DXEffect25Impl *object;
    HRESULT hr;

    TRACE("device %p, srcfile %s, defines %p, include %p, skip_constants %s, "
            "flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, debugstr_a(srcfile), defines, include, debugstr_a(skip_constants),
            flags, pool, effect, compilation_errors);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->ID3DXEffect25_iface.lpVtbl = &ID3DXEffect25_Vtbl;
    object->ref = 1;

    hr = D3DXCreateEffectFromFileExA(device, srcfile, defines, include, skip_constants, flags,
            pool, &object->effect, compilation_errors);
    if (FAILED(hr))
    {
        WARN("Failed to create effect\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *effect = &object->ID3DXEffect25_iface;
    TRACE("Created ID3DXEffect25 %p\n", object);
    return hr;
}

HRESULT WINAPI D3DXCreateEffectFromFileW25(struct IDirect3DDevice9 *device, const WCHAR *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags, struct ID3DXEffectPool *pool,
        struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    TRACE("device %p, srcfile %s, defines %p, include %p, "
            "flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, debugstr_w(srcfile), defines, include, flags, pool, effect, compilation_errors);

    return D3DXCreateEffectFromFileExW25(device, srcfile, defines, include, NULL, flags, pool, effect, compilation_errors);
}

HRESULT WINAPI D3DXCreateEffectFromFileA25(struct IDirect3DDevice9 *device, const char *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags, struct ID3DXEffectPool *pool,
        struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    TRACE("device %p, srcfile %s, defines %p, include %p, "
            "flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, debugstr_a(srcfile), defines, include, flags, pool, effect, compilation_errors);

    return D3DXCreateEffectFromFileExA25(device, srcfile, defines, include, NULL, flags, pool, effect, compilation_errors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceExW25(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const WCHAR *srcresource, const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skip_constants,
        DWORD flags, struct ID3DXEffectPool *pool, struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    struct ID3DXEffect25Impl *object;
    HRESULT hr;

    TRACE("device %p, srcmodule %p, srcresource %s, defines %p, include %p, skip_constants %s, "
            "flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, srcmodule, debugstr_w(srcresource), defines, include, debugstr_a(skip_constants),
            flags, pool, effect, compilation_errors);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->ID3DXEffect25_iface.lpVtbl = &ID3DXEffect25_Vtbl;
    object->ref = 1;

    hr = D3DXCreateEffectFromResourceExW(device, srcmodule, srcresource, defines, include,
            skip_constants, flags, pool, &object->effect, compilation_errors);
    if (FAILED(hr))
    {
        WARN("Failed to create effect\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *effect = &object->ID3DXEffect25_iface;
    TRACE("Created ID3DXEffect25 %p\n", object);
    return hr;
}

HRESULT WINAPI D3DXCreateEffectFromResourceExA25(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const char *srcresource, const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skip_constants,
        DWORD flags, struct ID3DXEffectPool *pool, struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    struct ID3DXEffect25Impl *object;
    HRESULT hr;

    TRACE("device %p, srcmodule %p, srcresource %s, defines %p, include %p, skip_constants %s, "
            "flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, srcmodule, debugstr_a(srcresource), defines, include, debugstr_a(skip_constants),
            flags, pool, effect, compilation_errors);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->ID3DXEffect25_iface.lpVtbl = &ID3DXEffect25_Vtbl;
    object->ref = 1;

    hr = D3DXCreateEffectFromResourceExA(device, srcmodule, srcresource, defines, include,
            skip_constants, flags, pool, &object->effect, compilation_errors);
    if (FAILED(hr))
    {
        WARN("Failed to create effect\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *effect = &object->ID3DXEffect25_iface;
    TRACE("Created ID3DXEffect25 %p\n", object);
    return hr;
}

HRESULT WINAPI D3DXCreateEffectFromResourceW25(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const WCHAR *srcresource, const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    TRACE("device %p, srcmodule %p, srcresource %s, defines %p, include %p, "
            "flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, srcmodule, debugstr_w(srcresource), defines, include,
            flags, pool, effect, compilation_errors);

    return D3DXCreateEffectFromResourceExW25(device, srcmodule, srcresource, defines, include, NULL,
            flags, pool, effect, compilation_errors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceA25(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const char *srcresource, const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect25 **effect, struct ID3DXBuffer **compilation_errors)
{
    TRACE("device %p, srcmodule %p, srcresource %s, defines %p, include %p, "
            "flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, srcmodule, debugstr_a(srcresource), defines, include,
            flags, pool, effect, compilation_errors);

    return D3DXCreateEffectFromResourceExA25(device, srcmodule, srcresource, defines, include, NULL,
            flags, pool, effect, compilation_errors);
}

HRESULT WINAPI D3DXDisassembleEffect25(ID3DXEffect25 *effect, BOOL enable_color_code, ID3DXBuffer **disassembly)
{
    struct ID3DXEffect25Impl *object = impl_from_ID3DXEffect25(effect);

    TRACE("effect %p, enable_color_code %u, disassembly %p.\n",
            effect, enable_color_code, disassembly);

    return D3DXDisassembleEffect(object->effect, enable_color_code, disassembly);
}
