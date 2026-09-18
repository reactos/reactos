// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "vector4_t.hpp"
#include "dx9layercommon.hpp"

#include <d3dx9math.h>
#include <memory>
#include <exception>
#include <array>

namespace dxlayer
{
    // Represents a 4D vector based on DX9
    template<>
    class vector4_t<dxapi::d3dx9> : public basetypes<dxapi::d3dx9>::vector4_base_t
    {
        typedef basetypes<dxapi::d3dx9>::vector4_base_t base_t;
    public:
        template<typename... Args>
        vector4_t(Args&&...args) : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        inline vector4_t& operator=(const vector4_t& v)
        {
            base_t::operator=(v);
            return *this;
        }

        // Accessors & Setters for X, Y, Z, W values
        inline float& operator[](axis_4d axis)
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

        // get members as a float array
        operator const std::array<float, 4>() const
        {
            return { x, y, z, w };
        }
    };
}


