// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"
#include <utility>

namespace dxlayer
{
    // Represents the interfaces present in a quaternion
    // All method implementations are deferred to 
    // template specializations
    template<dxapi apiset>
    class quaternion_t : public basetypes<apiset>::quaternion_base_t
    {
        typedef typename basetypes<apiset>::quaternion_base_t base_t;
    public:
        template<typename... Args>
        quaternion_t(Args&&...args) : base_t(std::forward<Args>(args)...)
        {
            // empty
        }

        // Accessors and setters for x, y, z, w
        float& operator[](axis_4d axis);

        // Const accessors for x, y, z, w
        float operator[](axis_4d axis) const;
    };
}

