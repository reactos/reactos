// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Render exported header file
//
//  $Description:
//      The render directory contains all software rendering code. This file
//      contains all the includes containing exported functionality for the rest
//      of the engine, but none of the imported functionality from elsewhere -
//      see precomp.hpp for those.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


// pixel jit

#include "fxjit\public\effectparams.h"
#include "fxjit\public\pshader.h"

// Color format conversion and scanline blending operations.

#include "scanpipelinerender.h"
#include "renderingbuilder.h"

// Rasterization.

#include "swrast.h"

#include "bilinearspan.h"
#include "brushspan.h"

#include "aarasterizer.h"
#include "aacoverage.h"

// Text rasterization.

#include "swglyphrun.h"
#include "swglyphpainter.h"

// Utility functions.

#include "geometry\geometry.h"

// Clip

#include "swclip.h"

// Render targets.

#include "SwIntermediateRTCreator.h"
#include "swsurfrt.h"   // Needs SWClip.h
#include "swhwndrt.h"
#include "boundsrt.h"

// Realization

#include "SwBitmapColorSource.h"

// Caching

#include "SwBitmapCache.h"

// Common setting

#include "swinit.h"


#include "DoubleBufferedBitmap.h"


