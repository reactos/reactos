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
#include <strsafe.h>
#include <psapi.h>

#define NO_WINGDIP_PATH_SEPARATOR
#if 0
#include <wingdip.h>
#endif

#include "wgx_core_types.h"
#include "wgx_render_types.h"
#include "wgx_core_dllname.h"

#include "common\common.h"
#include "scanop\scanop.h"

//
// Somebody decided to make this a macro. Undefine it so we can compile our
// functions which have the same name.
//

#undef UnlockResource
#undef UpdateResource
#undef CreateWindow

//
// The files from below are needed to get hold of the
// Render Target (PDEV) layer. All we need for now is:
//    CContextState
//    CRenderState
//    IRenderTargetInternal
//

#include "glyph\glyph.h"
#include "geometry\geometry.h"
#include "api\api_include.h"
#include "meta\meta.h"

//
// End of compatibility files.
//


//
// Rendering resources.
//

#include "targets\targets.h"
#include "sw\sw.h"
#include "hw\hw.h"
#include "av\av.h"

#include "control\util\control.h"
#include "effects\effectlist.h"

// GUID-related
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include "objbase.h"

#define INITGUID
#include "guiddef.h"

#include "uce\uce.h"
#include "resources\resources.h"

#include "DynamicCall\DelayCall.h"
#include "DpiProvider.h"

