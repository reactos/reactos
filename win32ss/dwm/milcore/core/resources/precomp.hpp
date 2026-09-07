// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    Precompiled header.

Environment:

    User mode only.

--*/

#include <wpfsdl.h>
#include "std.h"
#include "d2d1.h"

#include "wgx_render_types.h"
#include "wgx_core_types.h"

#include "common\common.h"
#include "scanop\scanop.h"



//
// Somebody decided to make this a macro. Undefine it so we can compile our
// functions which have the same name.
//  Where are these macros defined?

#undef CreateWindow
#undef UnlockResource
#undef UpdateResource


//
// End of compatibility files.
//


#include "glyph\glyph.h"

// needed for geometry
#include "geometry\geometry.h"

#include "api\api_include.h"

// Needed for sw.h and hw.h
#include "targets\targets.h"

// Needed only for bitmap
#include "sw\sw.h"

// Needed only for glyph run
#include "hw\hw.h"

// Needed for Video
#include "av\av.h"

// Needed for CBrushRealizer
#include "meta\meta.h"

#include "effects\effectlist.h"
#include "uce\uce.h"
#include "resources\resources.h"
#include "BrushIntermediateRealizer.h"
#include "DeviceAlignedIntermediateRealizer.h"
#include "ViewportAlignedIntermediateRealizer.h"

#include "fxjit\public\effectparams.h"
#include "fxjit\public\pshader.h"


