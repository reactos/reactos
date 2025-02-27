/*
 * Copyright 2015 Alistair Leslie-Hughes
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

#ifndef __D3DX10ASYNC_H__
#define __D3DX10ASYNC_H__

#include "d3dx10.h"

HRESULT WINAPI D3DX10CompileFromMemory(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *entry_point,
        const char *target, UINT sflags, UINT eflags, ID3DX10ThreadPump *pump, ID3D10Blob **shader,
        ID3D10Blob **error_messages, HRESULT *hresult);

HRESULT WINAPI D3DX10CreateEffectFromFileA(const char *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3D10EffectPool *effectpool, ID3DX10ThreadPump *pump, ID3D10Effect **effect, ID3D10Blob **errors,
        HRESULT *hresult);

HRESULT WINAPI D3DX10CreateEffectFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3D10EffectPool *effectpool, ID3DX10ThreadPump *pump, ID3D10Effect **effect, ID3D10Blob **errors,
        HRESULT *hresult);

HRESULT WINAPI D3DX10CreateEffectFromMemory(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile, UINT hlslflags,
        UINT fxflags, ID3D10Device *device, ID3D10EffectPool *effectpool, ID3DX10ThreadPump *pump,
        ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult);

HRESULT WINAPI D3DX10CreateEffectPoolFromFileA(const char *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3DX10ThreadPump *pump, ID3D10EffectPool **effectpool, ID3D10Blob **errors, HRESULT *hresult);

HRESULT WINAPI D3DX10CreateEffectPoolFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3DX10ThreadPump *pump, ID3D10EffectPool **effectpool, ID3D10Blob **errors, HRESULT *hresult);

HRESULT WINAPI D3DX10CreateEffectPoolFromMemory(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile, UINT hlslflags,
        UINT fxflags, ID3D10Device *device, ID3DX10ThreadPump *pump, ID3D10EffectPool **effectpool,
        ID3D10Blob **errors, HRESULT *hresult);

HRESULT WINAPI D3DX10PreprocessShaderFromMemory(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3DInclude *include, ID3DX10ThreadPump *pump, ID3D10Blob **shader_text,
        ID3D10Blob **errors, HRESULT *hresult);

HRESULT WINAPI D3DX10CreateEffectFromResourceA(HMODULE module, const char *resource_name,
        const char *filename, const D3D10_SHADER_MACRO *defines, ID3D10Include *include,
        const char *profile, UINT shader_flags, UINT effect_flags, ID3D10Device *device,
        ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump, ID3D10Effect **effect,
        ID3D10Blob **errors, HRESULT *hresult);

HRESULT WINAPI D3DX10CreateEffectFromResourceW(HMODULE module, const WCHAR *resource_name,
        const WCHAR *filename, const D3D10_SHADER_MACRO *defines, ID3D10Include *include,
        const char *profile, UINT shader_flags, UINT effect_flags, ID3D10Device *device,
        ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump, ID3D10Effect **effect,
        ID3D10Blob **errors, HRESULT *hresult);

HRESULT WINAPI D3DX10CreateAsyncFileLoaderW(const WCHAR *filename, ID3DX10DataLoader **loader);
HRESULT WINAPI D3DX10CreateAsyncFileLoaderA(const char *filename, ID3DX10DataLoader **loader);
HRESULT WINAPI D3DX10CreateAsyncMemoryLoader(const void *data, SIZE_T datasize, ID3DX10DataLoader **loader);
HRESULT WINAPI D3DX10CreateAsyncResourceLoaderA(HMODULE module, const char *resource, ID3DX10DataLoader **loader);
HRESULT WINAPI D3DX10CreateAsyncResourceLoaderW(HMODULE module, const WCHAR *resource, ID3DX10DataLoader **loader);

HRESULT WINAPI D3DX10CreateAsyncTextureProcessor(ID3D10Device *device,
        D3DX10_IMAGE_LOAD_INFO *info, ID3DX10DataProcessor **processor);
HRESULT WINAPI D3DX10CreateAsyncTextureInfoProcessor(D3DX10_IMAGE_INFO *info, ID3DX10DataProcessor **processor);

#endif
