// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Precompiled header for hw directory
//
//------------------------------------------------------------------------------

#include <wpfsdl.h>
// system includes

#include "std.h"
#include "d2d1.h"

#include <strsafe.h>

#include "wgx_core_dllname.h"

// debug output, allocator, etc.

#include "common\common.h"

// Format converter, palette etc.

#include "scanop\scanop.h"

// common glyph rendering classes

#include "glyph\glyph.h"

// geometry classes

#include "geometry\geometry.h"

// COMBase

#include "api\api_include.h"

// imaging and effects classes

#include "api\api_codecfactory.h"

// common render target classes and methods

#include "targets\targets.h"

// software classes

#include "sw\sw.h"

// externals used in common glyph rendering

// good driver database includes

#include "meta\meta.h"

#include "hw.h"
#include "av\av.h"

#include "resources\resources.h"

#include "d3dregistry.h"

#include "HwTexturedColorSourceBrush.h"
#include "D3DTextureSurface.h"

#include "D3DSwapChainWithSwDC.h"

#include "control\util\control.h"

#include "shaders.h"


#include "..\common\effects\effectlist.h"

#include "HwDeviceBitmapColorSource.h"
#include "HwBitBltDeviceBitmapColorSource.h"
#include "HwBitmapCache.h"              // needs HwBitmapColorSource.h
#include "HwRadialGradientBrush.h"          // needs HwLinearGradientBrush.h
#include "HwRadialGradientColorSource.h"    // needs HwLinearGradientColorSource.h


// ETW Tracing support
#include "WPFEventTrace.h"


