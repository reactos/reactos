// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Abstract:
*
*     The precomp.hpp includes any external header dependencies reqired
*     to compile the files in this subdirectory.
*
*
**************************************************************************/

#include <wpfsdl.h>
// system includes

#include "std.h"
#include "wgx_core_types.h"

#include "strsafe.h"

// debug output, allocator, etc.

#include "common\common.h"
#include "scanop\scanop.h"

#include "glyph\glyph.h"

// geometry classes

#include "geometry\geometry.h"

// COMBase

#include "api\api_include.h"

// hardware and software render targets.
#include "..\targets\targets.h"
#include "..\sw\sw.h"
#include "..\hw\hw.h"

#include "..\av\av.h"

#include "meta.h"

#include "MetaAdjustBounds.h"

#include "metaadjustobject.h"
#include "metaadjustbitmapsource.h" // needs metaadjustobject.h
#include "metaadjustaliasedclip.h"  // needs metaadjustobject.h
#include "metaadjusttransforms.h"   // needs metaadjustobject.h

#include "metaiterator.h"

#include "control\util\control.h"




