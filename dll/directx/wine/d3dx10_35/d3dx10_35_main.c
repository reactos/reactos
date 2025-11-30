/*
 * D3DX10 main file
 *
 * Copyright (c) 2010 Owen Rudge for CodeWeavers
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

#include "d3d10.h"

/***********************************************************************
 * D3DX10CheckVersion
 *
 * Checks whether we are compiling against the correct d3d and d3dx library.
 */
BOOL WINAPI D3DX10CheckVersion(UINT d3dsdkvers, UINT d3dxsdkvers)
{
    if ((d3dsdkvers == D3D10_SDK_VERSION) && (d3dxsdkvers == 35))
        return TRUE;

    return FALSE;
}
