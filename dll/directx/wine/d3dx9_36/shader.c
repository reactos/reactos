/*
 * Copyright 2008 Luis Busquets
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
#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "wingdi.h"
#include "d3dx9.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

LPCSTR WINAPI D3DXGetPixelShaderProfile(LPDIRECT3DDEVICE9 device)
{
    D3DCAPS9 caps;

    TRACE("device %p\n", device);

    if (!device) return NULL;

    IDirect3DDevice9_GetDeviceCaps(device,&caps);

    switch (caps.PixelShaderVersion)
    {
    case D3DPS_VERSION(1, 1):
        return "ps_1_1";

    case D3DPS_VERSION(1, 2):
        return "ps_1_2";

    case D3DPS_VERSION(1, 3):
        return "ps_1_3";

    case D3DPS_VERSION(1, 4):
        return "ps_1_4";

    case D3DPS_VERSION(2, 0):
        if ((caps.PS20Caps.NumTemps>=22)                          &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_ARBITRARYSWIZZLE)     &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_GRADIENTINSTRUCTIONS) &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_PREDICATION)          &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_NODEPENDENTREADLIMIT) &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT))
        {
            return "ps_2_a";
        }
        if ((caps.PS20Caps.NumTemps>=32)                          &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT))
        {
            return "ps_2_b";
        }
        return "ps_2_0";

    case D3DPS_VERSION(3, 0):
        return "ps_3_0";
    }

    return NULL;
}

UINT WINAPI D3DXGetShaderSize(const DWORD *byte_code)
{
    const DWORD *ptr = byte_code;

    TRACE("byte_code %p\n", byte_code);

    if (!ptr) return 0;

    /* Look for the END token, skipping the VERSION token */
    while (*++ptr != D3DSIO_END)
    {
        /* Skip comments */
        if ((*ptr & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT)
        {
            ptr += ((*ptr & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT);
        }
    }
    ++ptr;

    /* Return the shader size in bytes */
    return (ptr - byte_code) * sizeof(*ptr);
}

DWORD WINAPI D3DXGetShaderVersion(const DWORD *byte_code)
{
    TRACE("byte_code %p\n", byte_code);

    return byte_code ? *byte_code : 0;
}

LPCSTR WINAPI D3DXGetVertexShaderProfile(LPDIRECT3DDEVICE9 device)
{
    D3DCAPS9 caps;

    TRACE("device %p\n", device);

    if (!device) return NULL;

    IDirect3DDevice9_GetDeviceCaps(device,&caps);

    switch (caps.VertexShaderVersion)
    {
    case D3DVS_VERSION(1, 1):
        return "vs_1_1";
    case D3DVS_VERSION(2, 0):
        if ((caps.VS20Caps.NumTemps>=13) &&
            (caps.VS20Caps.DynamicFlowControlDepth==24) &&
            (caps.VS20Caps.Caps&D3DPS20CAPS_PREDICATION))
        {
            return "vs_2_a";
        }
        return "vs_2_0";
    case D3DVS_VERSION(3, 0):
        return "vs_3_0";
    }

    return NULL;
}

HRESULT WINAPI D3DXAssembleShader(LPCSTR data,
                                  UINT data_len,
                                  CONST D3DXMACRO* defines,
                                  LPD3DXINCLUDE include,
                                  DWORD flags,
                                  LPD3DXBUFFER* shader,
                                  LPD3DXBUFFER* error_messages)
{
    FIXME("stub\n");
    return D3DERR_INVALIDCALL;
}

HRESULT WINAPI D3DXAssembleShaderFromFileA(LPCSTR filename,
                                           CONST D3DXMACRO* defines,
                                           LPD3DXINCLUDE include,
                                           DWORD flags,
                                           LPD3DXBUFFER* shader,
                                           LPD3DXBUFFER* error_messages)
{
    LPWSTR filename_w = NULL;
    DWORD len;
    HRESULT ret;

    if (!filename) return D3DXERR_INVALIDDATA;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filename_w = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename_w) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, len);

    ret = D3DXAssembleShaderFromFileW(filename_w, defines, include, flags, shader, error_messages);

    HeapFree(GetProcessHeap(), 0, filename_w);
    return ret;
}

HRESULT WINAPI D3DXAssembleShaderFromFileW(LPCWSTR filename,
                                           CONST D3DXMACRO* defines,
                                           LPD3DXINCLUDE include,
                                           DWORD flags,
                                           LPD3DXBUFFER* shader,
                                           LPD3DXBUFFER* error_messages)
{
    FIXME("stub\n");
    return D3DERR_INVALIDCALL;
}

HRESULT WINAPI D3DXAssembleShaderFromResourceA(HMODULE module,
                                               LPCSTR resource,
                                               CONST D3DXMACRO* defines,
                                               LPD3DXINCLUDE include,
                                               DWORD flags,
                                               LPD3DXBUFFER* shader,
                                               LPD3DXBUFFER* error_messages)
{
    HRSRC res;
    LPCSTR buffer;
    DWORD len;

    if (!(res = FindResourceA(module, resource, (LPCSTR)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, (LPVOID *)&buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXAssembleShader(buffer, len, defines, include, flags,
                              shader, error_messages);
}

HRESULT WINAPI D3DXAssembleShaderFromResourceW(HMODULE module,
                                               LPCWSTR resource,
                                               CONST D3DXMACRO* defines,
                                               LPD3DXINCLUDE include,
                                               DWORD flags,
                                               LPD3DXBUFFER* shader,
                                               LPD3DXBUFFER* error_messages)
{
    HRSRC res;
    LPCSTR buffer;
    DWORD len;

    if (!(res = FindResourceW(module, resource, (LPCWSTR)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, (LPVOID *)&buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXAssembleShader(buffer, len, defines, include, flags,
                              shader, error_messages);
}
