// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      MILCore render state. Contains renderstate methods accessible to
//  product code
//

#pragma once

struct RenderStateOptions
{
    UINT SourceRectValid        : 1;    // BOOL
    UINT                        : 0;    // Pad to 32-bit boundry
};

/*=========================================================================*\
    CRenderState - Internal Render State Class
\*=========================================================================*/

class CRenderState
{
public:

    RenderStateOptions Options;

    // Local Transform kept by the render state (W)
    CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::RootRendering> LocalTransform;

    // Source rectangle of the main bitmap used during the call
    // the BitmapSource being drawn in the DrawBitmap calls
    // the Brush in all the other calls.
    MilPointAndSizeL SourceRect;

    // Bitmap filtering mode
    MilBitmapInterpolationMode::Enum InterpolationMode;

    // Bitmap prefiltering settings
    bool PrefilterEnable;
    FLOAT PrefilterThreshold;  // Threshold shrink factor above which
                               // prefiltering occurs.

    // Antialising mode
    MilAntiAliasMode::Enum AntiAliasMode;

    // Compositing mode and quality
    MilCompositingMode::Enum CompositingMode;

    // Text rendering options
    MilTextRenderingMode::Enum TextRenderingMode;
    MilTextHintingMode::Enum TextHintingMode;

    CRenderState()
    : LocalTransform(true)
    {
        Options.SourceRectValid = FALSE;

        PrefilterEnable = FALSE;
        PrefilterThreshold = REAL_SQRT_2;   // Seems like a good default for the
                                            // worst case of 45-degree rotation.

        CompositingMode    = MilCompositingMode::SourceOver;
        InterpolationMode  = MilBitmapInterpolationMode::Linear;

        // 8x8 is the only rendering mode that looks similar in hw and sw so
        // that's what our default is set to.
        AntiAliasMode      = MilAntiAliasMode::EightByEight;

        TextRenderingMode  = MilTextRenderingMode::Auto;
        TextHintingMode    = MilTextHintingMode::Auto;
    }
};

MtExtern(CMILRenderState);

/*=========================================================================*\
    CMILRenderState - Render State for MIL
\*=========================================================================*/

class CMILRenderState :
    public CMILObject,
    public CObjectUniqueness
{
public:

    static __checkReturn HRESULT Create(
        __in_ecount_opt(1) CMILFactory *pFactory,
        __deref_out_ecount(1) CMILRenderState ** const ppRenderState
        );

protected:
    CMILRenderState(__in_ecount_opt(1) CMILFactory *pFactory);
    virtual ~CMILRenderState();

private:

    // Create should be used to instantiate this object, not operator new.
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILRenderState));


public:

    DECLARE_MIL_OBJECT

    /* Local Transform */
    HRESULT SetLocalTransform(
        __in_ecount(1) const D3DMATRIX *pIMatrix
        );

    /* Source Rectangle */
    HRESULT SetSourceRectangle(
        __in_ecount_opt(1) const MilPointAndSizeL *prc
        );

    /* Compositing Mode */
    // SetCompositingMode- Moved to testdll until a use for this method is found

    /* Interpolation Mode */
    HRESULT SetInterpolationMode(
        MilBitmapInterpolationMode::Enum interpolationMode
        );

    /* Smoothing Mode */
    HRESULT SetAntiAliasMode(
        MilAntiAliasMode::Enum antiAliasMode
        );

    /* Bitmap prefilter settings */
    // SetPrefilterSettings- Moved to testdll until a use for this method is found

    __out_ecount(1) CRenderState *GetRenderStateInternal();

protected:
    CRenderState m_RenderState;
};



