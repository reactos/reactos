// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"
#include <array>

namespace dxlayer
{
    // Represents the interfaces present in a 2D vector
    // All method implementations are deferred to 
    // template specializations
    template <dxapi apiset>
    class vector2_t : public basetypes<apiset>::vector2_base_t
    {
    public:
        typedef typename basetypes<apiset>::vector2_base_t base_t;

        template<typename... Args>
        vector2_t(Args&&...args)
            : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        // Accessors & Setters for X, Y values
        float& operator[](axis_2d axis);

        // Const accessors for X, Y values
        float operator[](axis_2d axis) const;

        // Accessors and setters for X (index = 0), Y (index = 1) values
        float& operator[](int index);

        // Const accessors for X (index = 0) and Y (index = 1) values
        float operator[](int index) const;

        // Type-cast operator to convert to an array of floats
        operator const std::array<float, 2>() const;
    };
}

