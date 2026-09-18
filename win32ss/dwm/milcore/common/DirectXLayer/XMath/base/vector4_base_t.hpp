// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "../xmcommon.hpp"
#include "vector3_base_t.hpp"

namespace dxlayer
{
    // Describes a four-component vector, and includes 
    // constructors and operators compatible with the 
    // D3DXVECTOR4 type. 
    struct basetypes<dxapi::xmath>::vector4_base_t
    {
    public:
        FLOAT x, y, z, w;

#pragma region constructors

        inline vector4_base_t() : x(0), y(0), z(0), w(0) {}
        inline vector4_base_t(const vector4_base_t& v) 
            : vector4_base_t(v.x, v.y, v.z, v.w) {}
        inline vector4_base_t(const FLOAT* values)
            : x(values[0]), y(values[1]), z(values[2]), w(values[3]) {}
        inline vector4_base_t(const vector3_base_t& xyz, FLOAT w) :
            x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
        inline vector4_base_t(FLOAT x, FLOAT y, FLOAT z, FLOAT w)
            : x(x), y(y), z(z), w(w) {}

#pragma endregion

#pragma region casting

        inline operator float* ()
        {
            return &x;
        }

        inline operator float const*() const
        {
            return &x;
        }

#pragma endregion

#pragma region assignment operators

        inline vector4_base_t& operator +=(const vector4_base_t& v)
        {
            x += v.x;
            y += v.y;
            z += v.z;
            w += v.w;

            return *this;
        }

        inline vector4_base_t& operator -=(const vector4_base_t& v)
        {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            w -= v.w;

            return *this;
        }

        inline vector4_base_t& operator *=(float f)
        {
            x *= f;
            y *= f;
            z *= f;
            w *= f;

            return *this;
        }

        inline vector4_base_t& operator /=(float f)
        {
            x /= f;
            y /= f;
            z /= f;
            w /= f;

            return *this;
        }

        inline vector4_base_t& operator =(const vector4_base_t& v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
            w = v.w;

            return *this;
        }

#pragma endregion

#pragma region unary operators

        inline vector4_base_t operator+() const
        {
            return{ x, y, z, w };
        }

        inline vector4_base_t operator-() const
        {
            return{ -x, -y, -z, -w };
        }

#pragma endregion

#pragma region binary operators

        inline vector4_base_t operator+(const vector4_base_t& v) const
        {
            return (vector4_base_t(*this) += v);
        }

        inline vector4_base_t operator-(const vector4_base_t& v) const
        {
            return (vector4_base_t(*this) -= v);
        }

        inline vector4_base_t operator*(float f) const
        {
            return (vector4_base_t(*this) *= f);
        }

        inline vector4_base_t operator/(float f) const
        {
            return (vector4_base_t(*this) /= f);
        }

        inline friend vector4_base_t operator*(float f, const vector4_base_t& v)
        {
            return v * f;
        }

        inline bool operator ==(const vector4_base_t& v) const
        {
            return
                comparer<float>::is_essentially_equal_to(x, v.x) &&
                comparer<float>::is_essentially_equal_to(y, v.y) &&
                comparer<float>::is_essentially_equal_to(z, v.z) &&
                comparer<float>::is_essentially_equal_to(w, v.w);
        }

        inline bool operator !=(const vector4_base_t& v) const
        {
            return !(*this == v);
        }

#pragma endregion


    };
}
