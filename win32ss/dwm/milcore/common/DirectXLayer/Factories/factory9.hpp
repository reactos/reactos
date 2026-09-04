// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#if defined (D3D9EXTENSIONS)

#include "D3DX9/dx9layercommon.hpp"
#include "D3DX9/vector2_dx9.hpp"
#include "D3DX9/vector3_dx9.hpp"
#include "D3DX9/vector4_dx9.hpp"
#include "D3DX9/quaternion_dx9.hpp"
#include "D3DX9/matrix_dx9.hpp"
#include "D3DX9/shader_compiler_dx9.hpp"
#include "D3DX9/extensions_dx9.hpp"
#include "D3DX9/color_dx9.hpp"
namespace dxlayer
{
    const dxapi dx_apiset = dxapi::d3dx9;

    typedef vector2_t<dx_apiset> vector2;
    typedef vector3_t<dx_apiset> vector3;
    typedef vector4_t<dx_apiset> vector4;
    typedef quaternion_t<dx_apiset> quaternion;
    typedef matrix_t<dx_apiset> matrix;
    typedef math_extensions_t<dx_apiset> math_extensions;
    
    typedef color_t<dx_apiset> color;

    typedef shader_t<dx_apiset> shader;
}

#endif 


