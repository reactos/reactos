/*
 * Copyright 2016 Matteo Bruni for CodeWeavers
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

#ifndef __D3DX11ASYNC_H__
#define __D3DX11ASYNC_H__

#include "d3dx11.h"

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DX11CreateAsyncFileLoaderA(const char *file_name, ID3DX11DataLoader **loader);
HRESULT WINAPI D3DX11CreateAsyncFileLoaderW(const WCHAR *file_name, ID3DX11DataLoader **loader);
HRESULT WINAPI D3DX11CreateAsyncResourceLoaderA(HMODULE module, const char *resource, ID3DX11DataLoader **loader);
HRESULT WINAPI D3DX11CreateAsyncResourceLoaderW(HMODULE module, const WCHAR *resource, ID3DX11DataLoader **loader);
HRESULT WINAPI D3DX11CreateAsyncMemoryLoader(const void *data, SIZE_T data_size, ID3DX11DataLoader **loader);

HRESULT WINAPI D3DX11CompileFromMemory(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *entry_point,
        const char *target, UINT sflags, UINT eflags, ID3DX11ThreadPump *pump, ID3D10Blob **shader,
        ID3D10Blob **error_messages, HRESULT *hresult);

HRESULT WINAPI D3DX11CompileFromFileA(const char *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *entry_point, const char *target, UINT sflags, UINT eflags,
        ID3DX11ThreadPump *pump, ID3D10Blob **shader, ID3D10Blob **error_messages, HRESULT *hresult);

HRESULT WINAPI D3DX11CompileFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *entry_point, const char *target, UINT sflags, UINT eflags,
        ID3DX11ThreadPump *pump, ID3D10Blob **shader, ID3D10Blob **error_messages, HRESULT *hresult);

#ifdef __cplusplus
}
#endif

#endif
