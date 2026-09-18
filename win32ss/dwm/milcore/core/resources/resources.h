// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    The resources directory contains all rendering/composition device  code. This
    file contains all the includes containing exported functionality for the
    rest of the engine, but none of the imported functionality from
    elsewhere - see precomp.hpp for those.

Environment:

    User mode only.

--*/

#include <UCE\ResSlave.h>
#include <UCE\GlyphCacheSlave.h>    // Should be in resources directory
#include <UCE\GraphWalker.h>

#ifndef OFFSET_OF
#define OFFSET_OF(s, m)    UINT32(UINT64(HANDLE(&(((s *) 0)->m))))
#endif

#ifndef JIT_HEADERS
#define JIT_HEADERS

//
// JIT  headers
//

#include "fxjit\public\warpplatform.h"

#include "fxjit\Public\SIMDJit.h"

#include "fxjit\Compiler\FlushMemory.h"
#include "fxjit\Compiler\Register.h"
#include "fxjit\Compiler\Operator.h"
#include "fxjit\Compiler\Locator.h"
#include "fxjit\Compiler\Program.h"

#endif

//
// Rendering resources.
//

class CMilSlaveBitmap;

#include "valueres.h"
#include "resources_generated.h"
#include "data_generated.h"

#include "transform.h"
#include "CurrentValue.h"

#include "brushcontext.h"
#include "BrushRealizer.h"

#include "PathGeometryWrapper.h"
#include "brushtypeutils.h"

#include "CacheMode.h"
#include "BitmapCacheMode.h"

#include "translate.h"
#include "scale.h"
#include "skew.h"
#include "rotate.h"
#include "matrix.h"
#include "transformgroup.h"
#include "geometry.h"
#include "linegeometry.h"
#include "rectanglegeometry.h"
#include "ellipsegeometry.h"
#include "CombinedGeometry.h"
#include "GeometryGroup.h"
#include "pathgeometry.h"
#include "brush.h"
#include "solidcolorbrush.h"
#include "gradientbrush.h"
#include "lineargradient.h"
#include "radialgradient.h"
#include "BrushIntermediateCache.h"
#include "tilebrush.h"
#include "imagebrush.h"
#include "drawingbrush.h"
#include "visualbrush.h"
#include "BitmapCacheBrush.h"
#include "DashStyle.h"
#include "pen.h"

#include "TileBrushUtils.h" // Needs TileBrush.h

#include "imagesource.h"
#include "drawingimage.h"

#include "camera.h"
#include "projectioncamera.h"
#include "perspectivecamera.h"
#include "orthographiccamera.h"
#include "matrixcamera.h"
#include "model3d.h"
#include "model3dgroup.h"
#include "light.h"
#include "ambientlight.h"
#include "directionallight.h"
#include "pointlight.h"
#include "spotlight.h"
#include "geometrymodel3d.h"
#include "geometry3d.h"
#include "meshgeometry3d.h"
#include "material.h"
#include "materialgroup.h"
#include "diffusematerial.h"
#include "emissivematerial.h"
#include "specularmaterial.h"
#include "transform3d.h"
#include "transform3dgroup.h"
#include "affinetransform3d.h"
#include "translatetransform3d.h"
#include "scaletransform3d.h"
#include "rotatetransform3d.h"
#include "matrixtransform3d.h"
#include "rotation3d.h"
#include "axisanglerotation3d.h"
#include "quaternionrotation3d.h"

#include "modelwalker.h"
#include "modelrenderwalker.h"
#include "prerenderwalker.h"

#include "Effect.h"
#include "ShaderEffect.h"
#include "PixelShader.h"
#include "ImplicitInputBrush.h"
#include "BlurEffect.h"
#include "DropShadowEffect.h"

#include "d3dimage.h"

//
// Other resources
//

#include "bitmapres.h"
#include "doublebufferedbitmapres.h"
#include "glyphrunslave.h"
#include "glyphrungeometrysink.h"
#include "node.h"
#include "viewport3dvisual.h"
#include "visual3d.h"

#include "GuidelineCollectionResource.h"

#include "VideoSlave.h"

//
// Rendering layer.
//

#include "renderdata.h"
#include "drawing.h"
#include "geometrydrawing.h"
#include "imagedrawing.h"
#include "glyphrundrawing.h"
#include "videodrawing.h"
#include "drawinggroup.h"

//
// ETW Performance Tracing resource
//

#include "etwresource.h"





