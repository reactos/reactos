// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MILRender

    Created:

        10/09/2001 mbyrd

\*=========================================================================*/

#pragma once

/*=========================================================================*\
    Forward D3D type declarations

      To use these D3D types include the appropriate d3d header file(s).

\*=========================================================================*/

#define MIL_FORCE_DWORD 0x7FFFFFFF

typedef RECT MilRectL;

/*=========================================================================*\
    Color types
\*=========================================================================*/

const INT MIL_ALPHA_SHIFT = 24;
const INT MIL_RED_SHIFT   = 16;
const INT MIL_GREEN_SHIFT =  8;
const INT MIL_BLUE_SHIFT  =  0;

#define MIL_ALPHA_MASK  ((MilColorB) 0xff << MIL_ALPHA_SHIFT)
#define MIL_RED_MASK    ((MilColorB) 0xff << MIL_RED_SHIFT)
#define MIL_GREEN_MASK  ((MilColorB) 0xff << MIL_GREEN_SHIFT)
#define MIL_BLUE_MASK   ((MilColorB) 0xff << MIL_BLUE_SHIFT)

#define MIL_COLOR(a, r, g, b) \
    (MilColorB)(((a) & 0xFF) << MIL_ALPHA_SHIFT | \
     ((r) & 0xFF) << MIL_RED_SHIFT | \
     ((g) & 0xFF) << MIL_GREEN_SHIFT | \
     ((b) & 0xFF) << MIL_BLUE_SHIFT)

#define MIL_COLOR_GET_ALPHA(c) (((c) & MIL_ALPHA_MASK) >> MIL_ALPHA_SHIFT)
#define MIL_COLOR_GET_RED(c)   (((c) & MIL_RED_MASK)   >> MIL_RED_SHIFT)
#define MIL_COLOR_GET_GREEN(c) (((c) & MIL_GREEN_MASK) >> MIL_GREEN_SHIFT)
#define MIL_COLOR_GET_BLUE(c)  (((c) & MIL_BLUE_MASK)  >> MIL_BLUE_SHIFT)

/*=========================================================================*\
    Vertex types
\*=========================================================================*/

/* Type definition and flags for MIL Vertex Formats */

// These are different from D3DFVF flags because we use different types for
// color data.

typedef DWORD MilVertexFormat;

#define MIL_TEXTURESTAGE_TO_MILVFATTR(stage) static_cast<MilVertexFormatAttribute>(((MILVFAttrUV1 << (stage+1)) - 1) & MILVFAttrUV8)

/*=========================================================================*\
    Shader Types
\*=========================================================================*/

#define MILSP_INVALID_HANDLE 0xffffffff
#define MILSP_MAX_HANDLE (MILSP_INVALID_HANDLE-1)

typedef __range(0,MILSP_MAX_HANDLE) UINT MILSPHandle;

#include "Generated\wgx_render_types_generated.h"




