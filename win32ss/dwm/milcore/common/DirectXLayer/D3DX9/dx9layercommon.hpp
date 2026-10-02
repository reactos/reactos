// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once;

#include "dxlcommon.hpp"
#include <d3dx9math.h>

// Platform specific DX9 types
// These are D3DX9 types that will act as the base
// types for further derived types in the dxlayer
// namespace.
template<>
struct dxlayer::basetypes<dxlayer::dxapi::d3dx9>
{
    typedef D3DXVECTOR2 vector2_base_t;
    typedef D3DXVECTOR3 vector3_base_t;
    typedef D3DXVECTOR4 vector4_base_t;
    typedef D3DXQUATERNION quaternion_base_t;
    typedef D3DXMATRIX matrix_base_t;
    typedef D3DXCOLOR color_base_t;
};



