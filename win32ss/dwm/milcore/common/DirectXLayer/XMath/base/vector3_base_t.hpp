// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "../xmcommon.hpp"

namespace dxlayer
{
    // Describes a three-component vector, and includes 
    // constructors and operators compatible with the 
    // D3DXVECTOR3 type. 
    struct basetypes<dxapi::xmath>::vector3_base_t
    {
    public:
        float x;
        float y;
        float z;

#pragma region contructors

        inline vector3_base_t() : x(0), y(0), z(0) {}
        inline vector3_base_t(const FLOAT* values)
            : x(values[0]), y(values[1]), z(values[2]) {}
        inline vector3_base_t(FLOAT x, FLOAT y, FLOAT z)
            : x(x), y(y), z(z) {}
        inline vector3_base_t(const vector3_base_t& v)
            : vector3_base_t(v.x, v.y, v.z) {}
#pragma endregion

#pragma region casting

        inline operator float*()
        {
            return &x;
        }

        inline operator float const*() const
        {
            return &x;
        }

#pragma endregion

#pragma region assignment operators

        inline vector3_base_t& operator += (const vector3_base_t& v)
        {
            x += v.x;
            y += v.y;
            z += v.z;

            return *this;
        }

        inline vector3_base_t& operator -= (const vector3_base_t& v)
        {
            x -= v.x;
            y -= v.y;
            z -= v.z;

            return *this;
        }

        inline vector3_base_t& operator *= (float f)
        {
            x *= f;
            y *= f;
            z *= f;

            return *this;
        }

        inline vector3_base_t& operator /= (float f)
        {
            x /= f;
            y /= f;
            z /= f;

            return *this;
        }

        inline vector3_base_t& operator=(const vector3_base_t& v)
        {
            x = v.x;
            y = v.y;
            z = v.z;

            return *this;
        }

#pragma endregion


#pragma region unary operators

        inline vector3_base_t operator + () const
        {
            return *this;
        }

        inline vector3_base_t operator - () const
        {
            return{ -x, -y, -z };
        }

#pragma endregion

#pragma region binary operators

        inline vector3_base_t operator + (const vector3_base_t& v) const
        {
            return{ x + v.x, y + v.y, z + v.z };
        }
        
        inline vector3_base_t operator - (const vector3_base_t& v) const
        {
            return{ x - v.x, y - v.y, z - v.z };
        }

        inline vector3_base_t operator * (float f) const
        {
            return{ x*f, y*f, z*f };
        }

        inline vector3_base_t operator / (float f) const
        {
            return{ x / f, y / f, z / f };
        }


        friend inline vector3_base_t operator*(float f, const vector3_base_t& v)
        {
            return v * f;
        }

        bool operator==(const vector3_base_t& v) const
        {
            return
                comparer<float>::is_essentially_equal_to(x, v.x) &&
                comparer<float>::is_essentially_equal_to(y, v.y) &&
                comparer<float>::is_essentially_equal_to(z, v.z);

        }

        bool operator!=(const vector3_base_t& v) const
        {
            return !(*this == v);
        }

#pragma endregion

    };
}
