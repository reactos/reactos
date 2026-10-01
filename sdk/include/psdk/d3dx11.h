/*
 * Copyright 2016 Andrey Gusev
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

#ifndef __D3DX11_H__
#define __D3DX11_H__

#include <limits.h>
#include <float.h>

#define D3DX11_DEFAULT        (0xffffffffu)
#define D3DX11_FROM_FILE      (0xfffffffdu)
#define DXGI_FORMAT_FROM_FILE ((DXGI_FORMAT)0xfffffffdu)

#include "d3d11.h"
#include "d3dx11core.h"
#include "d3dx11async.h"
#include "d3dx11tex.h"

#define _FACDD 0x876
#define MAKE_DDHRESULT(code) MAKE_HRESULT(SEVERITY_ERROR, _FACDD, code)

enum _D3DX11_ERR
{
    D3DX11_ERR_CANNOT_MODIFY_INDEX_BUFFER = MAKE_DDHRESULT(2900),
    D3DX11_ERR_INVALID_MESH               = MAKE_DDHRESULT(2901),
    D3DX11_ERR_CANNOT_ATTR_SORT           = MAKE_DDHRESULT(2902),
    D3DX11_ERR_SKINNING_NOT_SUPPORTED     = MAKE_DDHRESULT(2903),
    D3DX11_ERR_TOO_MANY_INFLUENCES        = MAKE_DDHRESULT(2904),
    D3DX11_ERR_INVALID_DATA               = MAKE_DDHRESULT(2905),
    D3DX11_ERR_LOADED_MESH_HAS_NO_DATA    = MAKE_DDHRESULT(2906),
    D3DX11_ERR_DUPLICATE_NAMED_FRAGMENT   = MAKE_DDHRESULT(2907),
    D3DX11_ERR_CANNOT_REMOVE_LAST_ITEM    = MAKE_DDHRESULT(2908)
};

#endif
