// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


// Note: Visual caching required a change to the stepped rendering utility
// to ref-count m_pParentDisplayRT.  Since ref-counts will differ with
// step-rendering enabled vs. disabled, we'll disable it by default even on
// chk builds.  If you want to debug with step-rendering you'll need to
// produce a build with the bit below enabled.
#if DBG && !defined(DBG_STEP_RENDERING)
#define DBG_STEP_RENDERING  0
#endif

#if DBG_STEP_RENDERING
    #define DBG_STEP_RENDERING_COMMA_PARAM(x)  , x
#else
    #define DBG_STEP_RENDERING_COMMA_PARAM(x)
#endif

#include "log.h"
#include "ptrarray.h"
#include "ptrset.h"

#include "shared\shared.h"
#include "basetypes.h"
#include "CoordinateSpace.h"
#include "fix.h"
#include "Rect.h"
#include "milboxf.h"
#include "utils.h"
#include "SortAlgorithms.h"
#include "shared\DpiScale.h"
#include "shared\DpiUtil.h"

#include "stackalign.h"
#include "BufferDispenser.h"

#include "BaseMatrix.h"
#include "MILMatrix.h"
#include "matrix.h"
#include "MatrixBuilder.h"
#include "MultiOutSpaceMatrix.h"
#include "matrix3x2.h"

#include "WatermarkStack.h"
#include "MatrixStack.h"    // Needs WatermarkStack.h and Matrix.h

#include "memblock.h"
#include "lazymem.h"
#include "engine.h"
#include "GradientTexture.h"
#include "D3DLoader.h"
#include "Gamma.h"
#include "aliasedclip.h"
#include "IntermediateRTCreator.h"
#include "RTUtils.h"
#include "ResourcePool.h"
#include "GuidelineCollection.h"
#include "Tier.h"
#include "EnhancedContrastTable.h"
#include "Display.h"    // needs RTUtils.h (for CMILSurfaceRect) and Tier.h
#include "internalRT.h" // needs RTUtils.h (for CMILSurfaceRect) and Display.h
#include "d3dutils.h"
#include "3dutils.h"

#include "RectUtils.h"

#include "AssertDllInUse.h"
#include "milcoredllentry.h"

#include "memwriter.h"
#include "memreader.h"
#include "memblockreader.h"

#include "slistutil.h"

#include "typeconvert.inl"
#include "dump.h"
#include "Config.h"

#include "OSCompat.h"

#include "RenderingConstants.h"
#include "MILDC.h"

#include "DelayLoadedModule.h"
#include "DWMInterop.h"

#include "dwritefactory.h"

#include "renderoptions.h"

//
// Logging support flags
//

#define LOG_ERROR 0x1
#define LOG_API 0x2
#define LOG_FRAMES 0x4
#define LOG_WIN32 0x8
#define LOG_HANDLES 0x10


