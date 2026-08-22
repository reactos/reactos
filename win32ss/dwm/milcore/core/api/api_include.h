// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MIL

\*=========================================================================*/

#pragma once

// error C4995: 'matrix-type': name was marked as #pragma deprecated
//
// Ignore deprecation of non-CMILMatrix types in the api directory because
// API classes have to implement interfaces defined in windows/published,
// which must use D3DMATRIX instead of CMILMatrix
#pragma warning (push)
#pragma warning (disable : 4995)

/*=========================================================================*\
    Class forward declarations - The classes definitions should be listed
    in the same order here as they appear later in the file.
\*=========================================================================*/

class CMILCOMBase;
class CMILObject;
class CMILFactory;

class CMILRenderState;
class CMILRenderContext;

class CMILBrush;

class CMILMesh3D;

class CMILBrushSolid;
class CMILBrushBitmap;

interface IAVSurfaceRenderer;

class DpiProvider;

// Include our internal GUIDs so that the code in our API proxy classes can
// implement QI for these interfaces.

#include "InternalGUIDs.h"


// pixel jit

#include "fxjit\public\public.h"

// DpiProvider
#include "uce\DpiProvider.h"

/*=========================================================================*\
    Stub implementation header files
\*=========================================================================*/

#include "api_base.h"
#include "api_utils.h"
#include "api_factory.h"

#include "api_mesh3d.h"
#include "api_renderstate.h"
#include "api_lights.h"
#include "api_lightdata.h"
#include "api_rendercontext.h"

#include "api_basebrushes.h"
#include "api_brush.h" // needs api_basebrushes.h
#include "api_shader.h"

#include "api_codecfactory.h"

#pragma warning (pop)
/*=========================================================================*/




