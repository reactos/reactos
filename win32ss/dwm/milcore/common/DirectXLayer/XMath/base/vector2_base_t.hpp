// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "../xmcommon.hpp"

namespace dxlayer
{
    // Describes a two-component vector, and includes 
    // constructors and operators compatible with the 
    // D3DXVECTOR2 type. 
    struct basetypes<dxapi::xmath>::vector2_base_t
    {
    public:
#pragma region Members
        FLOAT x, y;
#pragma endregion

#pragma region Constructors
        inline vector2_base_t() : x(0), y(0) {}
        inline vector2_base_t(const FLOAT* values)
            : x(values[0]), y(values[0]) {}
        inline vector2_base_t(FLOAT x, FLOAT y)
            : x(x), y(y) {}
        inline vector2_base_t(const vector2_base_t& v)
            : vector2_base_t(v.x, v.y) {}
#pragma endregion 

#pragma region Casting
        inline operator FLOAT const* () const
        {
            return &x;
        }

        inline operator FLOAT*()
        {
            return &x;
        }
#pragma endregion

#pragma region Assignment Operators
        inline vector2_base_t& operator+=(const vector2_base_t& v)
        {
            x += v.x;
            y += v.y;

            return *this;
        }

        inline vector2_base_t& operator-=(const vector2_base_t& v)
        {
            x -= v.x;
            y -= v.y;

            return *this;
        }

        inline vector2_base_t& operator*=(float r)
        {
            x *= r;
            y *= r;

            return *this;
        }

        inline vector2_base_t& operator/=(float r)
        {
            x /= r;
            y /= r;

            return *this;
        }

        inline vector2_base_t& operator=(const vector2_base_t& v)
        {
            x = v.x;
            y = v.y;

            return *this;
        }
#pragma endregion

#pragma region unary operators

        inline vector2_base_t operator+() const
        {
            return{ x,y };
        }

        inline vector2_base_t operator-() const
        {
            return{ -x, -y };
        }
#pragma endregion

#pragma region binary operators

        inline vector2_base_t operator+ (const vector2_base_t& v) const
        {
            auto copy = *this;
            copy += v;
            return copy;
        }

        inline vector2_base_t operator- (const vector2_base_t& v) const
        {
            auto copy = *this;
            copy -= v;
            return copy;
        }

        inline vector2_base_t operator* (float r) const
        {
            auto copy = *this;
            copy *= r;
            return copy;
        }

        inline vector2_base_t operator/ (float r) const
        {
            auto copy = *this;
            copy /= r;
            return copy;
        }

        inline friend vector2_base_t operator* (float r, const vector2_base_t& v)
        {
            return v * r;
        }

        inline bool operator== (const vector2_base_t& v) const
        {
            return
                comparer<float>::is_essentially_equal_to(x, v.x) &&
                comparer<float>::is_essentially_equal_to(y, v.y);
        }

        inline bool operator!= (const vector2_base_t& v) const
        {
            return !(*this == v);
        }

#pragma endregion
    };
}
