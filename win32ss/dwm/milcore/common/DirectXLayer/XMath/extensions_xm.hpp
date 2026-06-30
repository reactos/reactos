// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"

#include "vector3_xm.hpp"
#include "vector4_xm.hpp"
#include "quaternion_xm.hpp"
#include "matrix_xm.hpp"

#include "extensions.hpp"

#include <DirectXMath.h>
#include <vector>

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
    // The implementation uses DirectXMath, but returns types that are compatible in layout and 
    // type with D3DX (DirectX 9 Extension API) types
    template<>
    class math_extensions_t<dxapi::xmath>
    {
    public:
        // D3DXQuaternionRotationMatrix ~= XMQuaternionRotationMatrix
        inline static quaternion_t<dxapi::xmath> make_quaternion_from_rotation_matrix(const matrix_t<dxapi::xmath>& matrix)
        {
            return DirectX::XMQuaternionRotationMatrix(matrix);
        }

        // D3DXVec3TransformCoord ~= XMVector3TransformCoord
        inline static vector3_t<dxapi::xmath> transform_coord(const vector3_t<dxapi::xmath>& vector, const matrix_t<dxapi::xmath>& matrix)
        {
            return DirectX::XMVector3TransformCoord(vector, matrix);
        }

        // D3DXVec3TransformNormal ~= XMVector3TransformNormal
        inline static vector3_t<dxapi::xmath> transform_normal(const vector3_t<dxapi::xmath>& vector, const matrix_t<dxapi::xmath>& matrix)
        {
            return DirectX::XMVector3TransformNormal(vector, matrix);
        }

        // D3DXVec4TransformArray ~= XMVector4TransformStream
        inline static std::vector<vector4_t<dxapi::xmath>> transform_array(
            unsigned int out_stride,
            const std::vector<vector4_t<dxapi::xmath>>& in,
            unsigned int in_stride,
            const matrix_t<dxapi::xmath>& transformation,
            unsigned int n)
        {
            dxlayer_assert::check(in.size() == n);

            // Transform the input list of vector4_t instance into a sequence of XMFLOAT4 objects
            std::vector<DirectX::XMFLOAT4> in_xmFloat4s(in.begin(), in.end());

            // The result of XMVector4TransformStream will be held in this. 
            std::vector<DirectX::XMFLOAT4> xmFloat4s(n);
            DirectX::XMVector4TransformStream(xmFloat4s.data(), out_stride, in_xmFloat4s.data(), in_stride, n, transformation);

            // Convert the list of XMFLOAT4's back to a list of vector4_t objects
            std::vector<vector4_t<dxapi::xmath>> result(xmFloat4s.begin(), xmFloat4s.end());

            return result;
        }

        // D3DXToRadian ~= XMConvertToRadians
        static float to_radian(float degrees)
        {
            return DirectX::XMConvertToRadians(degrees);
        }

        // D3DX_PI ~= XM_PI
        inline static float get_pi()
        {
            return DirectX::XM_PI;
        }
    };

}
