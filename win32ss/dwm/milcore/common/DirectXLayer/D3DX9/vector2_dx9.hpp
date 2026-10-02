// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "vector2_t.hpp"
#include "dx9layercommon.hpp"

#include <array>

namespace dxlayer
{
    // Represents a 2D vector based on DX9
    template <>
    class vector2_t<dxapi::d3dx9> : public basetypes<dxapi::d3dx9>::vector2_base_t
    {
    public:
        typedef basetypes<dxapi::d3dx9>::vector2_base_t base_t;

        template<typename... Args>
        vector2_t(Args&&...args)
            : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        inline vector2_t& operator=(const vector2_t& v)
        {
            base_t::operator=(v);
            return *this;
        }

        // Accessors & Setters for X, Y values
        inline float& operator[](axis_2d axis)
        {
            switch (axis)
            {
            case axis_2d::X:
                return x;
                break;
            case axis_2d::Y:
                return y;
                break;
            default:
                invalid_index_assert::terminate();
            }
        }

        // Const accessors for X, Y values
        inline float operator[](axis_2d axis) const
        {
            switch(axis)
            {
            case axis_2d::X:
                return x;
                break;
            case axis_2d::Y:
                return y;
                break;
            default:
                invalid_index_assert::terminate();
            }
        }

        // Accessors and setters for X (index = 0), Y (index = 1) values
        inline float& operator[](int index)
        {
            return (*this)[static_cast<axis_2d>(index)];
        }

        // Const accessors for X (index = 0) and Y (index = 1) values
        inline float operator[](int index) const
        {
            return (*this)[static_cast<axis_2d>(index)];
        }

        // Type-cast operator to convert to an array of floats
        inline operator const std::array<float, 2>() const
        {
            return{ x, y };
        }
    };
}

