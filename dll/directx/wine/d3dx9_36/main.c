/*
 * Direct3D X 9 main file
 *
 * Copyright (C) 2007 David Adam
 * Copyright (C) 2008 Tony Wasserka
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


#include "initguid.h"
#include "d3dx9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

BOOL WINAPI D3DXCheckVersion(UINT d3d_sdk_ver, UINT d3dx_sdk_ver)
{
    return d3d_sdk_ver == D3D_SDK_VERSION && d3dx_sdk_ver == D3DX_SDK_VERSION;
}

DWORD WINAPI D3DXCpuOptimizations(BOOL enable)
{
    FIXME("%#x - stub\n", enable);
    return 0;
}
