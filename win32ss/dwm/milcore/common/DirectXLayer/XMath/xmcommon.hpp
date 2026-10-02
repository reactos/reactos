// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"
#include <d3d9types.h>
#include <DirectXMath.h>

namespace dxlayer
{

    // D3DX9 compatible base types. 
    //
    // These definitions will stand-in for platform-specific types
    // used in our implementation of wpfgfx.
    //
    // We use D3DX9 compatible (in terms of size and layout) types here 
    // rather than the types supplied by the DirectX Math library
    // because our codebase depends on such conformity. For e.g., 
    // there are places in our codebase where we pass around arrays of 
    // objects as a BYTE* and reinterpret_cast<> as needed, do pointer
    // arithmetic on such BYTE* pointers etc. 
    //
    // Note: 
    //    The individual types declared below are defined
    //    in base/<typename>.hpp, and are all included in 
    //    factories/xmfactory.hpp
    template<>
    struct basetypes<dxapi::xmath>
    {
        // Describes a two-component vector, and includes 
        // constructors and operators compatible with the 
        // D3DXVECTOR2 type. 
        struct vector2_base_t;

        // Describes a three-component vector, and includes 
        // constructors and operators compatible with the 
        // D3DXVECTOR3 type. 
        struct vector3_base_t;

        // Describes a three-component vector, and includes 
        // constructors and operators compatible with the 
        // D3DXVECTOR3 type. 
        struct vector4_base_t;

        // Describes a quaternion that is 
        // compatible in size and layout with D3DXQUATERNION
        struct quaternion_base_t;

        // Describes a 4x4 matrix 
        // This is based on the D3DMATRIX types
        struct matrix_base_t;

        // Describes color values in a manner compatible 
        // in size and layout with D3DCOLORVALUE struct and 
        // D3DXCOLOR extensions.
        struct color_base_t;
    };
}
