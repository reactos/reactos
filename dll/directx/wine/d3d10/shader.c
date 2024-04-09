/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2010 Rico Schüller
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
 *
 */

#include "d3d10_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

/* IUnknown methods */

static inline struct d3d10_shader_reflection *impl_from_ID3D10ShaderReflection(ID3D10ShaderReflection *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_shader_reflection, ID3D10ShaderReflection_iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_reflection_QueryInterface(ID3D10ShaderReflection *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10ShaderReflection)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d10_shader_reflection_AddRef(ID3D10ShaderReflection *iface)
{
    struct d3d10_shader_reflection *This = impl_from_ID3D10ShaderReflection(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_shader_reflection_Release(ID3D10ShaderReflection *iface)
{
    struct d3d10_shader_reflection *This = impl_from_ID3D10ShaderReflection(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
        heap_free(This);

    return refcount;
}

/* ID3D10ShaderReflection methods */

static HRESULT STDMETHODCALLTYPE d3d10_shader_reflection_GetDesc(ID3D10ShaderReflection *iface, D3D10_SHADER_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return E_NOTIMPL;
}

static struct ID3D10ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3d10_shader_reflection_GetConstantBufferByIndex(
        ID3D10ShaderReflection *iface, UINT index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return NULL;
}

static struct ID3D10ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3d10_shader_reflection_GetConstantBufferByName(
        ID3D10ShaderReflection *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return NULL;
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_reflection_GetResourceBindingDesc(
        ID3D10ShaderReflection *iface, UINT index, D3D10_SHADER_INPUT_BIND_DESC *desc)
{
    FIXME("iface %p, index %u, desc %p stub!\n", iface, index, desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_reflection_GetInputParameterDesc(
        ID3D10ShaderReflection *iface, UINT index, D3D10_SIGNATURE_PARAMETER_DESC *desc)
{
    FIXME("iface %p, index %u, desc %p stub!\n", iface, index, desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_shader_reflection_GetOutputParameterDesc(
        ID3D10ShaderReflection *iface, UINT index, D3D10_SIGNATURE_PARAMETER_DESC *desc)
{
    FIXME("iface %p, index %u, desc %p stub!\n", iface, index, desc);

    return E_NOTIMPL;
}

const struct ID3D10ShaderReflectionVtbl d3d10_shader_reflection_vtbl =
{
    /* IUnknown methods */
    d3d10_shader_reflection_QueryInterface,
    d3d10_shader_reflection_AddRef,
    d3d10_shader_reflection_Release,
    /* ID3D10ShaderReflection methods */
    d3d10_shader_reflection_GetDesc,
    d3d10_shader_reflection_GetConstantBufferByIndex,
    d3d10_shader_reflection_GetConstantBufferByName,
    d3d10_shader_reflection_GetResourceBindingDesc,
    d3d10_shader_reflection_GetInputParameterDesc,
    d3d10_shader_reflection_GetOutputParameterDesc,
};

HRESULT WINAPI D3D10CompileShader(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *entrypoint,
        const char *profile, UINT flags, ID3D10Blob **shader, ID3D10Blob **error_messages)
{
    /* Forward to d3dcompiler */
    return D3DCompile(data, data_size, filename, defines, include,
            entrypoint, profile, flags, 0, shader, error_messages);
}

HRESULT WINAPI D3D10DisassembleShader(const void *data, SIZE_T data_size,
        BOOL color_code, const char *comments, ID3D10Blob **disassembly)
{
    TRACE("data %p, data_size %#lx, color_code %#x, comments %p, disassembly %p.\n",
            data, data_size, color_code, comments, disassembly);

    return D3DDisassemble(data, data_size, color_code ? D3D_DISASM_ENABLE_COLOR_CODE : 0, comments, disassembly);
}
