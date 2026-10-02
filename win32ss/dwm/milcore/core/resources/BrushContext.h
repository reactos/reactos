// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Declaration of the brush context class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#pragma once

class CComposition;
class CContentBounder;
class CMilSlaveResource;

//+-----------------------------------------------------------------------------
//
//  Class:
//      BrushContext
//
//  Synopsis:
//      Contains all context-specific state needed to create brush realizations.
//
//------------------------------------------------------------------------------
struct BrushContext
{
    // This variable is used to determine if the CDrawingContext owns
    // setting matWorldToSampleSpace or if this job belongs to the brush
    // realizer. 
    // It is also used to cause TileMode.None brushes to leave room for
    // transparency.
    bool fBrushIsUsedFor3D;

    // Matrix that transforms user-specified brush properties into the space
    // that intermediate representations are sampled from. See comment for
    // fSampleSpaceIsDeviceSpace to see how these variables are related
    CMILMatrix matWorldToSampleSpace;
    
    // Rectangle that relative brush coordinates should be sized to.
    // Ideally typed as CRectD<CoordinateSpace::BaseSampling>
    MilPointAndSizeD rcWorldBrushSizingBounds;

    // Extents of the viewable region in world space. 
    //
    // In 2D, this is the widened bounds of the shape being filled, and is used to avoid
    // creating intermediate bitmaps that are larger than these bounds.  In 3D this
    // is always equivalent to rcWorldBrushSizingBounds, because 3D doesn't implement any
    // notion of a 'viewable region' which should be clipped to.
    CMilRectF rcWorldSpaceBounds;

    // Optional clip rectangle in sample space.  
    // Intermediate representations do not need to define content outside of this rect.
    //
    // In 2D, this clip is obtained from the top of the clip stack, but isn't
    // used in 3D. 3D will set this varible to infinite.
    CMilRectF rcSampleSpaceClip;

    // Composition device used to retrieve timing for animate renderdata.
    CComposition *pBrushDeviceNoRef;
    
    // CContentBounder to obtain content bounds with.  
    //
    // Note: This object cannot be in-use by another bounding operation.
    CContentBounder *pContentBounder;

    // Flag that says if we should realize procedural brushes immediately as an
    // intermediate rendertarget.  This is useful for Radial Gradient Brush,
    // which we can't easily render in HW in 3D.
    BOOL fRealizeProceduralBrushesAsIntermediates;

    // Compositing mode that the brush is realized for. (Optimizations can be made based
    // on the compositing mode.)
    // This member is managed by the brush realizer class
    MilCompositingMode::Enum compositingMode;

    // Render target used by brushes to create intermediate surfaces.
    // This member mangaed by the brush realizer class
    CIntermediateRTCreator *pRenderTargetCreator;

    //
    // Adapter index to obtain realization for
    //
    UINT uAdapterIndex;
};




