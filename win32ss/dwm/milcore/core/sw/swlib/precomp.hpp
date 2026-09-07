// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Module Name:
*
*   precompiled header files.
*
* Abstract:
*
*   This file contains headers that are common to all effects
*   directories and is included into the precomp.hpp files in
*   each subdirectory.
*   Directory paths are relative to these subdirectories,
*
* Created:
*
*   06/16/2001 asecchia
*      Created it.
*
**************************************************************************/

#include <wpfsdl.h>
// system includes

#include "std.h"
#include "d2d1.h"

#include "strsafe.h"

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

// common render target classes and methods

#include "targets\targets.h"

// get the primitive list from here.

#include "meta\meta.h"

#include "sw.h"

// Audio/Video classes

#include "av\av.h"

#include "hw\hw.h"
#include "resources\resources.h"

// local includes

#include "swpresentgdi.h"

#include "control\util\control.h"



