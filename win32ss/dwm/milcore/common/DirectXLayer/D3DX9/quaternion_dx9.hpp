// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "quaternion_t.hpp"
#include "dx9layercommon.hpp"

#include <memory>
#include <utility>
#include <d3dx9math.h>


namespace dxlayer
{
    // Represents a quaternion based on DX9
    template<>
    class quaternion_t<dxapi::d3dx9> : public basetypes<dxapi::d3dx9>::quaternion_base_t
    {
        typedef basetypes<dxapi::d3dx9>::quaternion_base_t base_t;
    public:
        template<typename... Args>
        inline quaternion_t(Args&&...args) : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        inline quaternion_t& operator=(const quaternion_t& q)
        {
            base_t::operator=(q);
            return *this;
        }

        // Type-cast operator to convert to a 4D vector
        inline operator basetypes<dxapi::d3dx9>::vector4_base_t() const
        {
            return{ x, y, z, w };
        }

        // Accessors and setters for x, y, z, w
        inline virtual float& operator[](axis_4d axis)
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

        // Const accessors for x, y, z, w
        inline virtual float operator[](axis_4d axis) const
        {
            return (*this)[axis];
        }
    };
}
