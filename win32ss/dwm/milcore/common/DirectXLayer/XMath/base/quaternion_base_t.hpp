// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "../xmcommon.hpp"

namespace dxlayer
{
    // Describes a quaternion that is 
    // compatible in size and layout with D3DXQUATERNION
    struct basetypes<dxapi::xmath>::quaternion_base_t
    {
    public:
        float x, y, z, w;

#pragma region constructors

        inline quaternion_base_t() : x(0), y(0), z(0), w(0) {}
        inline quaternion_base_t(const float* values)
            : x(values[0]), y(values[1]), z(values[2]), w(values[3]) {}
        inline quaternion_base_t(float x, float y, float z, float w)
            : x(x), y(y), z(z), w(w) {}

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

        inline quaternion_base_t& operator=(const quaternion_base_t& q)
        {
            x = q.x;
            y = q.y;
            z = q.z;
            w = q.w;

            return *this;
        }

        inline quaternion_base_t& operator+=(const quaternion_base_t& q)
        {
            x += q.x;
            y += q.y;
            z += q.z;
            w += q.w;

            return *this;
        }

        inline quaternion_base_t& operator-=(const quaternion_base_t& q)
        {
            x -= q.x;
            y -= q.y;
            z -= q.z;
            w -= q.w;

            return *this;
        }

        inline quaternion_base_t& operator*=(float f)
        {
            x *= f;
            y *= f;
            z *= f;
            w *= f;

            return *this;
        }

        inline quaternion_base_t& operator/=(float f)
        {
            x /= f;
            y /= f;
            z /= f;
            w /= f;

            return *this;
        }

#pragma endregion

#pragma region unary operators

        inline quaternion_base_t operator+() const
        {
            return{ x, y, z, w };
        }

        inline quaternion_base_t operator-() const
        {
            return{ -x, -y, -z, -w };
        }

#pragma endregion

#pragma region binary operators

        // NOTE:vector multiplication is not defined here. 
        // if needed, it should be handled by derived types.

        inline quaternion_base_t operator+(const quaternion_base_t& q) const
        {
            return (quaternion_base_t(*this) += q);
        }

        inline quaternion_base_t operator-(const quaternion_base_t& q) const
        {
            return (quaternion_base_t(*this) -= q);
        }

        inline quaternion_base_t operator*(float f) const
        {
            return (quaternion_base_t(*this) *= f);
        }

        inline quaternion_base_t operator/(float f) const
        {
            return (quaternion_base_t(*this) /= f);
        }

        inline friend quaternion_base_t operator*(float f, const quaternion_base_t& q)
        {
            return q * f;
        }

        inline bool operator==(const quaternion_base_t& q) const
        {
            return
                comparer<float>::is_essentially_equal_to(x, q.x) &&
                comparer<float>::is_essentially_equal_to(y, q.y) &&
                comparer<float>::is_essentially_equal_to(z, q.z) &&
                comparer<float>::is_essentially_equal_to(w, q.w);
        }

        inline bool operator!=(const quaternion_base_t& q) const
        {
            return !(*this == q);
        }

#pragma endregion
    };
}


