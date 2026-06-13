// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "vector4_t.hpp"
#include "xmcommon.hpp"

namespace dxlayer
{
    // Represents a 4D vector that is compatible with D3DX9 D3DXVECTOR4 type
    // and uses DirectXMath for its internals. 
    template<>
    class vector4_t<dxapi::xmath> : public basetypes<dxapi::xmath>::vector4_base_t
    {
    private:
        typedef basetypes<dxapi::xmath>::vector4_base_t base_t;

    public:

        inline vector4_t() : base_t() {}
        inline vector4_t(const vector4_t& v): base_t(v) {}
        inline vector4_t(const FLOAT* values): base_t(values) {}
        inline vector4_t(const vector3_t<dxapi::xmath>& xyz, FLOAT w) :base_t(xyz, w) {}
        inline vector4_t(FLOAT x, FLOAT y, FLOAT z, FLOAT w)
            : base_t(x, y, z, w) {}
        inline vector4_t(const DirectX::XMFLOAT4& values)
            : base_t(values.x, values.y, values.z, values.w){}

        inline vector4_t& operator=(const vector4_t& v)
        {
            base_t::operator=(v);
            return *this;
        }

        // Accessors and Setters for X, Y, Z, W values
        inline float& operator[](axis_4d axis)
        {
            switch (axis)
            {
            case dxlayer::axis_4d::X:
                return x;
                break;
            case dxlayer::axis_4d::Y:
                return y;
                break;
            case dxlayer::axis_4d::Z:
                return z;
                break;
            case dxlayer::axis_4d::W:
                return w;
                break;
            default:
                invalid_index_assert::terminate();
                break;
            }
        }

        // Const accessors for X, Y, Z, W values
        inline float operator[](axis_4d axis) const
        {
            switch (axis)
            {
            case axis_4d::X:
                return x;
                break;
            case axis_4d::Y:
                return y;
                break;
            case axis_4d::Z:
                return z;
                break;
            case axis_4d::W:
                return w;
                break;

            default:
                invalid_index_assert::terminate();
            }
        }

        // Accessors and setters for X (index = 0), Y (index = 1), Z (index = 2), W (index = 3) values 
        float& operator[](int index)
        {
            auto &retVal = (*this)[static_cast<axis_4d>(index)];
            return retVal;
        }

        // Const accessors for X (index = 0), Y (index = 1), Z (index = 2), W (index = 3) values 
        float operator[](int index) const
        {
            auto retVal = (*this)[static_cast<axis_4d>(index)];
            return retVal;
        }

        inline bool operator==(const vector4_t& v) const
        {
            return 
                comparer<float>::is_essentially_equal_to(x, v.x) &&
                comparer<float>::is_essentially_equal_to(y, v.y) &&
                comparer<float>::is_essentially_equal_to(z, v.z) &&
                comparer<float>::is_essentially_equal_to(w, v.w);
        }

        // get members as a float array
        operator const std::array<float, 4>() const
        {
            return{ x, y, z, w };
        }

        operator const DirectX::XMFLOAT4() const
        {
            return{ x, y, z, w };
        }
    };
}
