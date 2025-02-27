/*
 * Copyright 2021 Nikolay Sivov for CodeWeavers
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

#include "wine/debug.h"

#define COBJMACROS

#include "d3d10_1.h"
#include "d3dx10.h"
#include "d3dcompiler.h"
#include "dxhelpers.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

#define D3DERR_INVALIDCALL 0x8876086c

static HRESULT create_effect(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile,
        UINT shader_flags, UINT effect_flags, ID3D10Device *device, ID3D10EffectPool *effect_pool,
        ID3D10Effect **effect, ID3D10Blob **errors)
{
    const char dxbc[] = {'D','X','B','C'};
    ID3D10Blob *code = NULL;
    void *buffer;
    SIZE_T size;
    HRESULT hr;

    if (!data || !device)
        return E_FAIL;

    if (errors)
        *errors = NULL;

    buffer = (void *)data;
    size = datasize;

    /* Effect is not compiled. */
    if (datasize < sizeof(dxbc) || memcmp(dxbc, data, sizeof(dxbc)))
    {
        if (!profile)
            return D3DERR_INVALIDCALL;
        if (FAILED(hr = D3DCompile(data, datasize, filename, defines, include, "main", profile,
                shader_flags, effect_flags, &code, errors)))
        {
            WARN("Effect compilation failed, hr %#lx.\n", hr);
            return hr;
        }
        buffer = ID3D10Blob_GetBufferPointer(code);
        size = ID3D10Blob_GetBufferSize(code);
    }

    hr = D3D10CreateEffectFromMemory(buffer, size, effect_flags, device, effect_pool, effect);

    if (code)
        ID3D10Blob_Release(code);
    return hr;
}

HRESULT WINAPI D3DX10CreateEffectFromMemory(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile,
        UINT shader_flags, UINT effect_flags, ID3D10Device *device, ID3D10EffectPool *effect_pool,
        ID3DX10ThreadPump *pump, ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult)
{
    TRACE("data %p, datasize %Iu, filename %s, defines %p, include %p, profile %s, shader_flags %#x,"
            "effect_flags %#x, device %p, effect_pool %p, pump %p, effect %p, errors %p, hresult %p.\n",
            data, datasize, debugstr_a(filename), defines, include, debugstr_a(profile),
            shader_flags, effect_flags, device, effect_pool, pump, effect, errors, hresult);

    if (pump)
        FIXME("Asynchronous mode is not supported.\n");

    if (!include)
        include = D3D_COMPILE_STANDARD_FILE_INCLUDE;

    return create_effect(data, datasize, filename, defines, include, profile,
            shader_flags, effect_flags, device, effect_pool, effect, errors);
}

HRESULT WINAPI D3DX10CreateEffectFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT shader_flags, UINT effect_flags,
        ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump,
        ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult)
{
    char filename_a[MAX_PATH];
    void *buffer = NULL;
    DWORD size = 0;
    HRESULT hr;

    TRACE("filename %s, defines %p, include %p, profile %s, shader_flags %#x, effect_flags %#x, "
            "device %p, effect_pool %p, pump %p, effect %p, errors %p, hresult %p.\n",
            debugstr_w(filename), defines, include, debugstr_a(profile), shader_flags, effect_flags,
            device, effect_pool, pump, effect, errors, hresult);

    if (!filename)
        return E_FAIL;

    if (pump)
        FIXME("Asynchronous mode is not supported.\n");

    if (!include)
        include = D3D_COMPILE_STANDARD_FILE_INCLUDE;

    if (SUCCEEDED((hr = load_file(filename, &buffer, &size))))
    {
        WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, sizeof(filename_a), NULL, NULL);
        hr = create_effect(buffer, size, filename_a, defines, include, profile,
                shader_flags, effect_flags, device, effect_pool, effect, errors);
        free(buffer);
    }

    return hr;
}

HRESULT WINAPI D3DX10CreateEffectFromFileA(const char *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT shader_flags, UINT effect_flags,
        ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump,
        ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult)
{
    WCHAR *filenameW;
    HRESULT hr;
    int len;

    TRACE("filename %s, defines %p, include %p, profile %s, shader_flags %#x, effect_flags %#x, "
            "device %p, effect_pool %p, pump %p, effect %p, errors %p, hresult %p.\n",
            debugstr_a(filename), defines, include, debugstr_a(profile), shader_flags, effect_flags,
            device, effect_pool, pump, effect, errors, hresult);

    if (!filename)
        return E_FAIL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    if (!(filenameW = malloc(len * sizeof(*filenameW))))
        return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, len);

    hr = D3DX10CreateEffectFromFileW(filenameW, defines, include, profile, shader_flags,
            effect_flags, device, effect_pool, pump, effect, errors, hresult);
    free(filenameW);

    return hr;
}

HRESULT WINAPI D3DX10CreateEffectFromResourceA(HMODULE module, const char *resource_name,
        const char *filename, const D3D10_SHADER_MACRO *defines, ID3D10Include *include,
        const char *profile, UINT shader_flags, UINT effect_flags, ID3D10Device *device,
        ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump, ID3D10Effect **effect,
        ID3D10Blob **errors, HRESULT *hresult)
{
    void *data;
    DWORD size;
    HRESULT hr;

    TRACE("module %p, resource_name %s, filename %s, defines %p, include %p, profile %s, "
            "shader_flags %#x, effect_flags %#x, device %p, effect_pool %p, pump %p, effect %p, "
            "errors %p, hresult %p.\n", module, debugstr_a(resource_name), debugstr_a(filename),
            defines, include, debugstr_a(profile), shader_flags, effect_flags,
            device, effect_pool, pump, effect, errors, hresult);

    hr = load_resourceA(module, resource_name, &data, &size);
    if (FAILED(hr))
        return hr;

    return create_effect(data, size, filename, defines, include, profile,
            shader_flags, effect_flags, device, effect_pool, effect, errors);
}

HRESULT WINAPI D3DX10CreateEffectFromResourceW(HMODULE module, const WCHAR *resource_name,
        const WCHAR *filenameW, const D3D10_SHADER_MACRO *defines, ID3D10Include *include,
        const char *profile, UINT shader_flags, UINT effect_flags, ID3D10Device *device,
        ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump, ID3D10Effect **effect,
        ID3D10Blob **errors, HRESULT *hresult)
{
    char *filename = NULL;
    void *data;
    DWORD size;
    HRESULT hr;
    int len;

    TRACE("module %p, resource_name %s, filename %s, defines %p, include %p, profile %s, "
            "shader_flags %#x, effect_flags %#x, device %p, effect_pool %p, pump %p, effect %p, "
            "errors %p, hresult %p.\n", module, debugstr_w(resource_name), debugstr_w(filenameW),
            defines, include, debugstr_a(profile), shader_flags, effect_flags,
            device, effect_pool, pump, effect, errors, hresult);

    hr = load_resourceW(module, resource_name, &data, &size);
    if (FAILED(hr))
        return hr;

    if (filenameW)
    {
        len = WideCharToMultiByte(CP_ACP, 0, filenameW, -1, NULL, 0, NULL, NULL);
        if (!(filename = malloc(len)))
            return E_OUTOFMEMORY;
        WideCharToMultiByte(CP_ACP, 0, filenameW, -1, filename, len, NULL, NULL);
    }

    hr = create_effect(data, size, filename, defines, include, profile,
            shader_flags, effect_flags, device, effect_pool, effect, errors);
    free(filename);
    return hr;
}

HRESULT WINAPI D3DX10PreprocessShaderFromMemory(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3DInclude *include, ID3DX10ThreadPump *pump, ID3D10Blob **shader_text,
        ID3D10Blob **errors, HRESULT *hresult)
{
    HRESULT hr;

    TRACE("data %s, data_size %Iu, filename %s, defines %p, include %p, pump %p, shader_text %p, "
            "errors %p, hresult %p.\n",
            debugstr_an(data, data_size), data_size, debugstr_a(filename), defines, include, pump,
            shader_text, errors, hresult);

    if (!data)
        return E_FAIL;

    if (pump)
        FIXME("Unimplemented ID3DX10ThreadPump handling.\n");

    hr = D3DPreprocess(data, data_size, filename, defines, include, shader_text, errors);
    if (hresult)
        *hresult = hr;
    return hr;
}
