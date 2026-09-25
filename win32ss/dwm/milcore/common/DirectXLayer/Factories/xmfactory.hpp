// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#if defined (DIRECTXMATH)

#include "xmath/xmcommon.hpp"
#include "xmath/base/vector2_base_t.hpp"
#include "xmath/base/vector3_base_t.hpp"
#include "xmath/base/vector4_base_t.hpp"
#include "xmath/base/matrix_base_t.hpp"
#include "xmath/base/color_base_t.hpp"

#include "xmath/vector2_xm.hpp"
#include "xmath/vector3_xm.hpp"
#include "xmath/vector4_xm.hpp"
#include "xmath/quaternion_xm.hpp"
#include "xmath/matrix_xm.hpp"
#include "xmath/extensions_xm.hpp"
#include "xmath/color_xm.hpp"
#include "xmath/shader_compiler_xm.hpp"

namespace dxlayer
{
    const dxapi dx_apiset = dxapi::xmath;

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
