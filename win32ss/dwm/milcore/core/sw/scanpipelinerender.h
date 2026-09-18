// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      A pipeline of scan operations.
//
//      This class composes scan operations to form the back-end rasterizer
//      pipeline. It includes brush color generation, modification such as
//      alpha-masking, and alpha-blending to the destination.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

// Number of intermediate buffers needed. Count them:
//   1. Brush colors
//   2. Destination pixels (may need to read them in to convert them before the
//     blend).
//   3. One extra so that we can ping-pong between buffers.
//
//  (4. When DrawGlyphs is integrated: ClearType mask?)

class CAntialiasedFiller;
class CColorSource;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CScanPipelineRendering
//
//  Synopsis:
//      A set of scan operations that, once set up, can be run on a set of
//      spans. This class is used:
//
//      * to implement the pixel pipeline of the SW renderer
//      * when generating clipping masks
//
//------------------------------------------------------------------------------

class CScanPipelineRendering : public CScanPipeline
{

public:

    CScanPipelineRendering();
    virtual ~CScanPipelineRendering();

    // Initialize for rendering (FillPath, DrawBitmap).
    // ReleaseExpensiveResources() must be called in between Initialize* calls.

    HRESULT InitializeForRendering(
        __inout_ecount(1) CSPIntermediateBuffers &oIntermediateBuffers,
        MilPixelFormat::Enum fmtDest,
        __in_ecount(1) CColorSource *pColorSource,
        BOOL fPPAA,
        bool fNeedsComplement,
        MilCompositingMode::Enum eCompositingMode,
        UINT uClipBoundsWidth,
        __in_ecount_opt(1) IMILEffectList *pIEffectList,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device> *pmatEffectToDevice,
        __in_ecount_opt(1) const CContextState *pContextState
        );

    HRESULT InitializeForTextRendering(
        __inout_ecount(1) CSPIntermediateBuffers &oIntermediateBuffers,
        MilPixelFormat::Enum fmtDest,
        __in_ecount(1) CColorSource *pColorSource,
        MilCompositingMode::Enum eCompositingMode,
        __inout_ecount(1) CSWGlyphRunPainter &painter,
        bool fNeedsAA
        );

    void SetAntialiasedFiller(
        __in_ecount(1) CAntialiasedFiller *pFiller
        );

    // Release expensive resources. See CSpanSink::ReleaseExpensiveResources().
    // *Must* be called between calls to Initialize*.
    virtual VOID ReleaseExpensiveResources();

protected:

#if DBG
    virtual VOID AssertNoExpensiveResources();
#else
    virtual VOID AssertNoExpensiveResources() {}
#endif // DBG

    virtual VOID ResetPipeline()
    {
        CScanPipeline::ResetPipeline();
        m_idxosdAAFiller = -1;
    }

private:

    friend class RenderingBuilder;

    // An internal helper class, used only by Initialize().

    class Builder2;

    // When we use antialiasing, we need to set the pointer to the filler after
    // the pipeline is built.

    INT m_idxosdAAFiller;

    // Remembers OSD structs which we allocate while building the pipeline (so that we can delete
    // them later).
    // Used for CMaskAlphaSpan* and CConstantAlphaSpan*.

    DynArray<COwnedOSD *> m_rgosdOwned;
};


