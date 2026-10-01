/*
 * Copyright 2013 Detlef Riekenberg
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"

#include "d3dx11.h"
#include "d3dx11core.h"
#include "d3dx11tex.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

BOOL WINAPI D3DX11CheckVersion(UINT d3d_sdk_ver, UINT d3dx_sdk_ver)
{
    return d3d_sdk_ver == D3D11_SDK_VERSION && d3dx_sdk_ver == D3DX11_SDK_VERSION;
}

HRESULT WINAPI D3DX11FilterTexture(ID3D11DeviceContext *context, ID3D11Resource *texture, UINT src_level, UINT filter)
{
    FIXME("context %p, texture %p, src_level %u, filter %#x stub!\n", context, texture, src_level, filter);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX11GetImageInfoFromFileA(const char *filename, ID3DX11ThreadPump *pump, D3DX11_IMAGE_INFO *img_info,
        HRESULT *hresult)
{
    FIXME("filename %s, pump %p, img_info %p, hresult %p stub!\n", debugstr_a(filename), pump, img_info, hresult);

    if (!filename)
        return E_FAIL;

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX11GetImageInfoFromFileW(const WCHAR *filename, ID3DX11ThreadPump *pump, D3DX11_IMAGE_INFO *img_info,
        HRESULT *hresult)
{
    FIXME("filename %s, pump %p, img_info %p, hresult %p stub!\n", debugstr_w(filename), pump, img_info, hresult);

    if (!filename)
        return E_FAIL;

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX11GetImageInfoFromMemory(const void *src_data, SIZE_T src_data_size, ID3DX11ThreadPump *pump,
        D3DX11_IMAGE_INFO *img_info, HRESULT *hresult)
{
    FIXME("src_data %p, src_data_size %Iu, pump %p, img_info %p, hresult %p stub!\n",
            src_data, src_data_size, pump, img_info, hresult);

    return E_NOTIMPL;
}
