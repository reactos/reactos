// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"
#include <array>
namespace dxlayer
{
    // Represents the interfaces present in a 4D vector
    // All method implementations are deferred to 
    // template specializations
    template<dxapi apiset>
    class vector4_t: public basetypes<apiset>::vector4_base_t
    {
        typedef typename basetypes<apiset>::vector4_base_t base_t;
    public:
        template<typename... Args>
        vector4_t(Args&&...args) : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        // Accessors & Setters for X, Y, Z, W values
        float& operator[](axis_4d axis);

        // Const accessors for X, Y, Z, W values
        float operator[](axis_4d axis) const;

        // Accessors and setters for X (index = 0), Y (index = 1), Z (index = 2), W (index = 3) values 
        float& operator[](int index);

        // Const accessors for X (index = 0), Y (index = 1), Z (index = 2), W (index = 3) values 
        float operator[](int index) const;

        // get members as a float array
        operator const std::array<float, 4>() const;

    };
}


