// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dx9layercommon.hpp"

namespace dxlayer
{
    // Specialization is really unnecessary here since there is nothing 
    // being added into this type. This specialization exists for symmetry
    // with other base types' specializations.
    template<>
    struct color_t<dxapi::d3dx9> : public basetypes<dxapi::d3dx9>::color_base_t
    {

    };
}

