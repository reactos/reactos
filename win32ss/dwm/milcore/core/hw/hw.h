// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      All the includes that are exported from the hw rendering engine
//      directory.
//

// HW classes

#include "HwStateTable.h"
#include "HwRenderStateManager.h"
#include "d3drenderstate.h"

#include "d3dresource.h"
#include "d3dtexture.h"         // needs d3dresource.h
#include "d3dsurface.h"         // needs d3dresource.h
#include "d3dlockabletexture.h" // needs d3dtexture.h
#include "d3dvidmemonlytexture.h" // needs d3dtexture.h

#include "swfallback.h"         // needs d3dlockabletexture.h

#include "d3dstats.h"
#include "d3dlog.h"
#include "d3dglyphbank.h"       // needs d3dresource.h, d3dlog.h

#include "d3dvertex.h"
#include "d3dgeometry.h"        // needs d3dvertex.h

#include "Waffler.h"

#include "DelayComputedBounds.h"
#include "HwBrushContext.h"     // needs DelayComputedBounds.h

#include "HwVertexBuffer.h"     // needs Waffler.h
#include "Hw3DGeometryStreamBuffer.h"
#include "hwshader.h"                       // needs ResourceCache.h
#include "HwColorSource.h"                  // needs HwVertexBuffer.h
#include "hwffshaders.h"                    // needs hwshader.h, hwcolorsource.h
#include "HwConstantColorSource.h"          // needs HwColorSource.h
#include "HwTexturedColorSource.h"          // needs HwColorSource.h
#include "hwsolidcolortexturesource.h"      // needs HwTexturedColorSource.h
#include "hwsolidcolortexturesourcepool.h"  // needs HwSolidColorTextureSource.h
#include "HwBitmapColorSource.h"            // needs HwTexturedColorSource.h, HwBrushContext.h
#include "HwVidMemTextureManager.h"
#include "HwLinearGradientColorSource.h"    // needs HwTexturedColorSource.h, HwVidMemTextureManager.h
#include "HwBoxColorSource.h"               // needs HwTexturedColorSource.h, HwVidMemTextureManager.h
#include "HwColorComponentColorSource.h"
#include "HwLightingColorSource.h"
#include "HwDestinationTexture.h"           // needs HwTexturedColorSource.h
#include "HwDestinationTexturePool.h"       // needs HwDestinationTexture.h

#include "HwHLSLShaderFragments.h"
#include "HwPipeline.h"             // needs HwColorSource.h
#include "HwPipelineBuilder.h"      // needs HwPipeline.h
#include "HwShaderPipeline.h"
#include "HwShaderBuilder.h"        // needs HwPipeline.h, HwHLSLShaderFragments.h
#include "HwShaderCache.h"          // needs HwShaderBuilder.h
#include "HwPrimaryColorSource.h"   // needs HwPipeline.h

#include "HwShaderFragmentToHLSLConverter.h"

#include "HwBrush.h"                // needs HwPrimaryColorSource.h
#include "HwSolidBrush.h"           // needs HwBrush.h, HwConstantColorSource.h
#include "HwBitmapBrush.h"          // needs HwBrush.h, HwBitmapColorSource.h

#include "HwBrushPool.h"            // needs HwBrush.h
#include "HwLinearGradientBrush.h"  // needs HwBrushPool.h, HwLinearGradientColorSource.h

#include "HwSurfRTData.h"           // needs HwBrushPool.h

#include "gpumarker.h"           

#include "HwCaps.h"

#include "d3ddevice.h"          // needs d3dresource.h, dxbrush.h, d3dstats.h, d3dlog.h
                                // d3dglyphbank.h, hwsurfrtdata.h, d3dglyphrun.h, HwCaps.h

#include "d3dswapchain.h"       // needs d3dresource.h, d3ddevice.h


#include "d3dglyphrun.h"
#include "d3dglyphpainter.h"    // needs d3dglyphrun.h, d3ddevice.h

#include "hwrasterizer.h"
#include "hwsurfrt.h"
#include "hwtexturert.h"
#include "hwdisplayrt.h"
#include "hwhwndrt.h"

#include "d3ddevicemanager.h"

#include "hwinit.h"
#include "hwsw3dfallback.h"

#include "BitmapOfDeviceBitmaps.h"
#include "InteropDeviceBitmap.h"

#include "HwUtils.h"

#include "HwShaderEffect.h"
#include "ShaderAssemblies\Shaders.h"


