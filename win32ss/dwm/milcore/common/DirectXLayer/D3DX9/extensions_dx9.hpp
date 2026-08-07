// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"

#include "vector3_dx9.hpp"
#include "vector4_dx9.hpp"
#include "quaternion_dx9.hpp"
#include "matrix_dx9.hpp"
#include "extensions.hpp"

#include <vector>
#include <array>
#include <cassert>

namespace dxlayer
{
    // Miscellaneous methods that either do not fit well as a member of a vector or matrix 
    // types, or cannot be implemented in one of those types directly due to circular 
    // (header file) inclusion problems arising form the fact that full definitions of 
    // some of these types would be needed at points where they wouldn't yet be available. 
    // These problems could well have been avoided by using pointers instead of objects, but 
    // we choose to take the approach of having a small utility class (this class) 
    // to work around the header-inclusion and definition-availability problems. 
    //
    // This implementation uses D3DX API's. 
    template<>
    class math_extensions_t<dxapi::d3dx9>
    {
    public:
        // D3DXQuaternionRotationMatrix
        inline static quaternion_t<dxapi::d3dx9> make_quaternion_from_rotation_matrix(const matrix_t<dxapi::d3dx9>& matrix)
        {
            quaternion_t<dxapi::d3dx9> q;
            D3DXQuaternionRotationMatrix(&q, &matrix);

            return q;
        }

        // D3DXVec3TransformCoord
        inline static vector3_t<dxapi::d3dx9> transform_coord(const vector3_t<dxapi::d3dx9>& vector, const matrix_t<dxapi::d3dx9>& matrix)
        {
            vector3_t<dxapi::d3dx9> v;
            D3DXVec3TransformCoord(&v, &vector, &matrix);

            return v;
        }

        // D3DXVec3TransformNormal
        inline static vector3_t<dxapi::d3dx9> transform_normal(const vector3_t<dxapi::d3dx9>& vector, const matrix_t<dxapi::d3dx9>& matrix)
        {
            vector3_t<dxapi::d3dx9> v;
            D3DXVec3TransformNormal(&v, &vector, &matrix);

            return v;
        }

        // D3DXVec4TransformArray
        inline static std::vector<vector4_t<dxapi::d3dx9>> transform_array(
            unsigned int out_stride, 
            const std::vector<vector4_t<dxapi::d3dx9>>& in,
            unsigned int in_stride, 
            const matrix_t<dxapi::d3dx9>& transformation, 
            unsigned int n)
        {
            dxlayer_assert::check(in.size() == n);

            std::vector<vector4_t<dxapi::d3dx9>> result(n);
            D3DXVec4TransformArray(result.data(), out_stride, in.data(), in_stride, &transformation, n);
            
            return result;
        }

        // D3DXToRadian
        template<typename T>
        static T to_radian(T degrees)
        {
            static_assert(std::is_arithmetic<T>::value, "to_radian is only usable with arithmetic types");
            return D3DXToRadian(degrees);
        }

        // D3DX_PI
        inline static float get_pi()
        {
            return D3DX_PI;
        }
    };
}
