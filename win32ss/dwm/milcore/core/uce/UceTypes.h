// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Definition of types used by the UCE or it's resources.
//

class CContentBounder;
class CMilEffectDuce;

//+-----------------------------------------------------------------------------
//
//  Structure:  CLayer
//
//  Synopsis:   A temporary layer.
//              Currently designed only for PushOpacity, PushClip (geometric
//              mask), and PushOpacityMask, but it could generalize into
//              BeginLayer(rect,effect)...EndLayer.
//
//------------------------------------------------------------------------------

struct CLayer
{
    CLayer(
        FLOAT rAlphaIn = 1.0f,
        __in_ecount_opt(1) CShape *pGeometricMaskShapeIn = NULL,
        __in_ecount_opt(1) CMilSlaveResource *pAlphaMaskBrushIn = NULL,
        __in_ecount_opt(1) CMilEffectDuce *pEffectIn = NULL,
        __in_ecount_opt(1) CRectF<CoordinateSpace::LocalRendering> const *pBoundsIn = NULL
        )
    {
        prtTargetPrev = NULL;
        pbmOutput = NULL;
        prtbmOutput = NULL;
        fHasOffset = FALSE;
        rAlpha = rAlphaIn;
        pAlphaMaskBrush = pAlphaMaskBrushIn;
        pEffect = pEffectIn;
        if (pBoundsIn)
        {
            rcBounds = *pBoundsIn;
            fHasBounds = TRUE;
        }
        else
        {
            rcBounds.SetEmpty();
            fHasBounds = FALSE;
        }
        pGeometricMaskShape = pGeometricMaskShapeIn;
        ZeroMemory(&scaleMatrix, sizeof(CMILMatrix));
        ZeroMemory(&restMatrix, sizeof(CMILMatrix));
        surfaceScaleX = 1.0f;
        surfaceScaleY = 1.0f;
        uIntermediateWidth = 0;
        uIntermediateHeight = 0;
        isDummyEffectLayer = FALSE;
    }

    IRenderTargetInternal *prtTargetPrev;   // The previous render target

    IWGXBitmapSource *pbmOutput;            // The output of the layer rendering

    IMILRenderTargetBitmap *prtbmOutput;    // The render target we render our layer output into.
                                            // Only used by bitmap effects.
                                            
    MilPoint2L ptLayerPosition;            // Position of this layer relative
                                            // to the previous RT's origin.
    BOOL fHasOffset;                        // Whether or not this has an offset
                                            //  (in which case we need to pop both
                                            //   translation and clip whenever we
                                            //   are popping opacity)


    FLOAT rAlpha;                           // Constant alpha value to apply
                                            // when this layer ends.

    CMilSlaveResource *pAlphaMaskBrush;     // Pointer to the OpacityMask, if present

    CMilEffectDuce *pEffect;     // Pointer to the Bitmap Effect, if present

    CShape *pGeometricMaskShape;            // Pointer to the geometric mask, if present
    CRectF<CoordinateSpace::LocalRendering> rcBounds;   // Bounds of the PushOpacityMask
    BOOL fHasBounds;

    // Matrix decomposition of the world transform used if this is an image effect 
    // layer, instead of using the usual offset decomposition.
    CMILMatrix scaleMatrix;    
    CMILMatrix restMatrix;     
    
    // Scales for max texture size limitation.  Only used for effect layers.
    float surfaceScaleX;
    float surfaceScaleY;

    // Size of intermediates created for this layer for Effects.
    UINT uIntermediateWidth;
    UINT uIntermediateHeight; 
                              
    BOOL isDummyEffectLayer;    // Used when we need to push an extra software layer
                                // to render software shader effects when in fixed function (hw).
                                
#if DBG_ANALYSIS
    CoordinateSpaceId::Enum dbgTargetPrevCoordSpaceId;
#endif
};



