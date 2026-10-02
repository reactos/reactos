// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------
//

//
//  Abstract:
//     This file contains rendering constants common amoungst milcore.
//
//----------------------------------------------------------------------------

#pragma once

// Because SRGB uses a single byte per channel, any pixel contribution that is 
// less than 1/2 of (1 / 256)  will be rounded down to 0.  If a shape covers
// less than INSIGNIFICANT_PIXEL_COVERAGE_SRGB, we can ignore this contribution.
const float INSIGNIFICANT_PIXEL_COVERAGE_SRGB = 1.0 / 512.0;     

// The corner coordinates of a square that is a safe rendering region
// in that:
//   1. All device pixels will be (well) within the box
//   2. Points are representable in the fixed-point representations
//      used by some of our rasterizers.
//
//  The AA hardware rasterizer enforces the most restrictive coordinate bounds,
//  so we use those (32 bits - (4 bits of scale + 1 sign bit + 5 bits of
//  rasterizer working range + 3 bits of anti-aliasing) = 19 bits). See
//  comments in TransformRasterizerPointsTo28_4 for details.

const float SAFE_RENDER_MAX = static_cast<float>((1 << 19)-1);

