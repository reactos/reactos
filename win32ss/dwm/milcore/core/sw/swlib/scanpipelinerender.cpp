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
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

// Builder2
#include "scanpipelinebuilder.hpp"

//
// CScanPipelineRendering
//

CScanPipelineRendering::CScanPipelineRendering()
{
    m_idxosdAAFiller = -1;
}

CScanPipelineRendering::~CScanPipelineRendering()
{
    for (UINT i = 0; i < m_rgosdOwned.GetCount(); i++)
    {
        delete m_rgosdOwned[i];
    }
    m_rgosdOwned.Reset();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::InitializeForRendering
//
//  Synopsis:
//      Builds the pipeline for rendering to a non-indexed surface. Adds scan
//      operations needed for brush color generation, alpha-masking,
//      antialiasing, and alpha-blending.
//

HRESULT
CScanPipelineRendering::InitializeForRendering(
    __inout_ecount(1) CSPIntermediateBuffers &oIntermediateBuffers,
    MilPixelFormat::Enum fmtDest,
        // The (non-indexed) destination format
    __in_ecount(1) CColorSource *pColorSource,
    BOOL fPPAA,                               
        // True if a per-primitive antialiasing mode will be used.
    bool fComplementAlpha,                    
        // True if complement will be used.
    MilCompositingMode::Enum eCompositingMode,      
        // SourceCopy or SourceOver
    UINT uClipBoundsWidth,                    
        // Width of clipping bounds
    __in_ecount_opt(1) IMILEffectList *pIEffectList,
        // Effects to apply to source data.
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device> *pmatEffectToDevice,
        // Needed only when pIEffectList != NULL
    __in_ecount_opt(1) const CContextState *pContextState
        // Needed only when pIEffectList != NULL
    )
{
    MilPixelFormat::Enum fmtColorSource = pColorSource->GetPixelFormat();
    
    Assert(!IsIndexedPixelFormat(fmtDest));

    Assert(CSoftwareRasterizer::IsValidPixelFormat(fmtColorSource));
    Assert(   (eCompositingMode == MilCompositingMode::SourceCopy)
           || (eCompositingMode == MilCompositingMode::SourceOver));

    // Check that, if there was a previous call to Initialize*, which added to
    // m_rgosdOwned, then ReleaseExpensiveResources() was called afterwards as required.
    Assert(m_rgosdOwned.GetCount() == 0);

#if DBG_ANALYSIS
    Assert(uClipBoundsWidth <= oIntermediateBuffers.DbgAanalysisGetMaxAllowableWidth());
#endif

    // PERF-2002/11/27-agodfrey  Opaque brush optimization
    //  We could optimize for opaque brushes. This would be especially good for FSAA.
    //
    //  For PPAA, one idea is to encode run information generated from the antialiasing
    //  coverage, and then have the blend functions use that information instead of testing
    //  the alpha for each pixel.
    //
    //  For FSAA it would be much simpler.

    HRESULT hr = S_OK;

    // Begin building a new pipeline

    ResetPipeline();

    RenderingBuilder builder(
        this,
        &oIntermediateBuffers,
        ScanPipelineBuilder::BM_RENDERING
        );

    m_PipelineParams.m_fDither16bpp = TRUE;  // Always dither when converting
                                             // down to 16bpp.

    BOOL fDestOpaque = !HasAlphaChannel(fmtDest);

    BOOL fBrushDataChanged = false;  // True if we do something to the color
                                     // data, like masking it.

    //
    // Generate brush color data
    //

    IFC( builder.AddOp_Unary(
        pColorSource->GetScanOp(),
        pColorSource,
        RenderingBuilder::SP_BLENDSOURCE) );

    //
    // Apply effect list
    //

    if (pIEffectList)
    {
        Assert(pmatEffectToDevice);
        Assert(pContextState);

        IFC(builder.Append_EffectList(
            pIEffectList,
            pmatEffectToDevice,
            pContextState,
            uClipBoundsWidth,
            fmtColorSource,
            &fmtColorSource
            ));


        fBrushDataChanged = true;
    }

    //
    // Apply per-prim antialiasing coverage
    //

    if (fPPAA)
    {
        // The actual coverage data is supplied to us later, when the rasterizer
        // calls SetAntialiasedFiller().

        IFC( builder.AddOp_ScalePPAACoverage(fmtColorSource, fComplementAlpha, &fmtColorSource) );
        fBrushDataChanged = true;
    }

    //
    // Blend the color data to the destination, using either SrcCopy or
    // SrcOver.
    //

    if (eCompositingMode == MilCompositingMode::SourceCopy)
    {
        // PERF-2002/11/27-agodfrey  Optimize SrcCopy for opaque data
        //  We pass "NULL" here for fSrcOpaque. If we knew that the brush data
        //  was opaque (and there was no PPAA/effect list), we could pass that
        //  here, saving an AlphaDivide step in many cases.

        IFC( builder.Append_Convert_NonHalftoned(
                fmtDest,
                fmtColorSource,
                FALSE,
                RenderingBuilder::SP_BLENDSOURCE) );
    }
    else
    {
        Assert(eCompositingMode == MilCompositingMode::SourceOver);

        // Check for a special-case SrcOver/SrcOverAL operation.

        ScanOpFunc pfnSrcOver = GetOp_SrcOver_or_SrcOverAL(fmtDest, fmtColorSource);

        if (pfnSrcOver)
        {
            bool fDummy;

            // We can't combine special-case operations with prior
            // operations on the destination data, because the 16bpp MMX
            // SrcOverAL functions ignore m_pvSrc2.

            IFC( builder.AddOp_PTernary(
                pfnSrcOver,
                NULL,
                &fDummy
                ) );

            // It's OK to ignore fDummy because no other operations are
            // performed on the destination. So the last operation (this blend
            // operation) is always a WriteRMW.
        }
        else
        {
            //
            //  The RMW optimization was targeted at video memory, so it's
            //  currently removed because the old logic no longer applies. But
            //  it's probably worth re-evaluating. For mostly-opaque data, it's
            //  costly to read the entire scan from the back buffer; using
            //  ReadRMW would save a lot of uncached reads.
            //
            //  Reminder: SrcOverAL_32bppPARGB_555_MMX and
            //            SrcOverAL_32bppPARGB_565_MMX are not true WriteRMW
            //            operations, so we can't use ReadRMW for them (or
            //            otherwise must append a WriteRMW afterwards).
            //
            //            Also, the old RMW logic would not work with
            //            superluminous premultiplied colors. Check out the
            //            comment in SrcOverAL_32bppPARGB_32bppPARGB.
            //

            //
            // Build a SrcOverAL operation using multiple scan operations.
            //

            // The brush data format will be the blend format.

            ScanOpFunc pfnSrcOverAL = GetOp_SrcOver_or_SrcOverAL(fmtColorSource, fmtColorSource);
            Assert(pfnSrcOverAL);
            bool fNeedWriteRMW;
            bool fNoOperationsAfterBlend;

            // Convert destination to blend format, if necessary

            IFC( builder.Append_Convert_NonHalftoned(
                    fmtColorSource,
                    fmtDest,
                    fDestOpaque,
                    RenderingBuilder::SP_MAIN) );

            // Blend

            IFC( builder.AddOp_PTernary(pfnSrcOverAL, NULL, &fNeedWriteRMW) );

            // Convert to destination format, if necessary

            IFC( builder.Append_Convert_NonHalftoned_ReportNOP(
                    fmtDest,
                    fmtColorSource,
                    fDestOpaque,          // For SrcOver, if "DestIn" is opaque,
                                          // then "DestOut" is too.
                    RenderingBuilder::SP_MAIN,
                    &fNoOperationsAfterBlend
                ) );

            if (fNeedWriteRMW)
            {
                if (fNoOperationsAfterBlend)
                {
                    // We don't need to add a WriteRMW; the last operation is a blend,
                    // and blends are also WriteRMW operations.

                    // Make sure no-one adds operations after this one.

                    builder.EnforcePipelineDone();

                    //   Need a WriteRMW for correctness.
                    //
                    // The 16bpp conversion algorithms we use currently, don't
                    // preserve 16bpp values when you round-trip between 5/6
                    // bits and 8 bits. So we
                    // theoretically need a WriteRMW to avoid changing
                    // destination pixels when our source pixels are completely
                    // transparent. (This includes our MMX special-case blending
                    // functions since they sometimes write when alpha = 0).
                    //
                    // But it would be better to fix the 16bpp code.
                    //
                    // I *assume* that our gamma conversion functions don't have
                    // the same round trip problems.
                    //
                }
                else
                {
                    // Add a WriteRMW operation

                    Assert(FALSE);

                    // I claim that this case is impossible. It only happens if there was
                    // no conversion of the destination before the blend operation, and there
                    // was conversion after the blend operation.
                    //
                    // That should be impossible because there's no format where converting
                    // to the blend format is a NOP, but converting back is not.
                    //
                    // It takes some effort to verify this claim by reading the code.


                    // The reason we don't "just handle" this case is, we
                    // currently don't have WriteRMW operations implemented for
                    // all the allowed destination formats.
                    //
                    // Code-generation could help here.
                }
            }
        }
    }

    IFC( builder.End() );

Cleanup:
    if (FAILED(hr))
    {
        ReleaseExpensiveResources();
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::InitializeForTextRendering
//
//  Arguments:
//      oIntermediateBuffers         placeholder for intermediate results
//      fmtDest                      the (non-indexed) destination format
//      fmtColorSource               source format (either 32bppPARGB or 128bppPARGB)
//      pColorSource                 brush
//      eCompositingMode             SourceCopy or SourceOver
//      painter                      glyph run painter - supplies scan operations
//
//------------------------------------------------------------------------------

HRESULT
CScanPipelineRendering::InitializeForTextRendering(
    __inout_ecount(1) CSPIntermediateBuffers &oIntermediateBuffers,
    MilPixelFormat::Enum fmtDest,
    __in_ecount(1) CColorSource *pColorSource,
    MilCompositingMode::Enum eCompositingMode,
    __inout_ecount(1) CSWGlyphRunPainter &painter,
    bool fNeedsAA
    )
{
    HRESULT hr = S_OK;
    CConstantAlphaSpan *pSpan = NULL;

    Assert(pColorSource);
    MilPixelFormat::Enum fmtColorSource = pColorSource->GetPixelFormat();

    Assert(!IsIndexedPixelFormat(fmtDest));
    Assert(    (fmtDest == MilPixelFormat::BGR32bpp)
            || (fmtDest == MilPixelFormat::PBGRA32bpp));
    Assert(    (fmtColorSource == MilPixelFormat::PBGRA32bpp)
            || (fmtColorSource == MilPixelFormat::BGR32bpp)
           /*|| (fmtColorSource == MilPixelFormat::PRGBA128bppFloat)*/);
    Assert(   (eCompositingMode == MilCompositingMode::SourceCopy)
           || (eCompositingMode == MilCompositingMode::SourceOver));

    // Check that, if there was a previous call to Initialize*, which added to
    // m_rgosdOwned, then ReleaseExpensiveResources() was called afterwards as required.
    Assert(m_rgosdOwned.GetCount() == 0);

    // Begin building a new pipeline
    ResetPipeline();

    Builder2 builder(
        this,
        &oIntermediateBuffers
        );

    VBID vbid_brush = builder.GetBuffer();

    //
    // Generate brush color data
    //

    builder.AddOperation(
        pColorSource->GetScanOp(),
        pColorSource,
        VBID_NULL,
        VBID_NULL,
        vbid_brush
        );

    if (   (   fmtColorSource != MilPixelFormat::PBGRA32bpp
            && fmtColorSource != MilPixelFormat::BGR32bpp)
        || (   fmtDest    != MilPixelFormat::BGR32bpp
            && fmtDest    != MilPixelFormat::PBGRA32bpp)  )
    {
        // currently unsupported pixel format
        IFC( WGXERR_UNSUPPORTED_OPERATION );
    }

    if (painter.GetEffectAlpha() != 1)
    {
        pSpan = new CConstantAlphaSpan;
        IFCOOM(pSpan);
        IFC( pSpan->Initialize(painter.GetEffectAlpha()) );

        builder.AddOperation(
            fmtColorSource == MilPixelFormat::BGR32bpp ? ConstantAlpha_32bppRGB : ConstantAlpha_32bppPARGB,
            pSpan,
            VBID_NULL,
            VBID_NULL,
            vbid_brush
            );

        IFC( m_rgosdOwned.Add(pSpan) );
        pSpan = NULL;

        // after appying effect alpha pixel format of vbid_brush changes to PBGRA
        fmtColorSource = MilPixelFormat::PBGRA32bpp;
    }

    if (fNeedsAA)
    {
        ScanOpFunc pfnOp = GetOp_ScalePPAACoverage(fmtColorSource,
                                                   false /* Not using complement rendering */,
                                                   &fmtColorSource);
        builder.AddOperation(
            pfnOp,
            NULL,
            VBID_NULL,
            VBID_NULL,
            vbid_brush
            );

        // We only support 1 of these operations in the pipeline.
        // Check that we haven't added one already.

        Assert(m_idxosdAAFiller == -1);

        // Record the index of this operation so that m_posd can be updated later.
        m_idxosdAAFiller = builder.GetCount() - 1;
    }

    if (!painter.IsClearType())
    {   //grey scale handling

        if (eCompositingMode == MilCompositingMode::SourceCopy)
        {
            //
            // Apply glyph run transparency
            //

            VBID vbid_painted = builder.GetBuffer();

            builder.AddOperation(
                painter.GetScanOpCopy(fmtColorSource),
                &painter,
                vbid_brush,
                VBID_NULL,
                vbid_painted
                );

            // In theory, we may wish format conversion here.
            // In practice, we don't.
            // Just redirect output.

            builder.Redirect(vbid_painted, VBID_DEST);
        }
        else
        {
            Assert(eCompositingMode == MilCompositingMode::SourceOver);

            //
            // Apply glyph run transparency and blend in one step
            //

            builder.AddOperation(
                painter.GetScanOpOver(fmtColorSource),
                &painter,
                vbid_brush,
                VBID_NULL,
        VBID_DEST
        );
        }
    }
    else
    {   // clear type handling

        //
        // Blend the color data to the destination, using either SrcCopy or
        // SrcOver.
        //

        if (eCompositingMode == MilCompositingMode::SourceCopy)
        {
            VBID vbid_alpha = builder.GetBuffer();
            //
            // Apply glyph run transparency
            //

            builder.AddOperation(
                painter.GetScanOpCopy(fmtColorSource),
                &painter,
                vbid_brush,
                VBID_NULL,
                vbid_alpha
                );

             // vbid_brush serves as input and output, receives colors
            VBID vbid_color = vbid_brush;

            builder.Redirect(vbid_color, VBID_DEST);
            builder.Redirect(vbid_alpha, VBID_AUX);
        }
        else
        {
            Assert(eCompositingMode == MilCompositingMode::SourceOver);

            //
            // Apply glyph run transparency and blend in one step
            //

            builder.AddOperation(
                painter.GetScanOpOver(fmtColorSource),
                &painter,
                vbid_brush,
                VBID_NULL,
                VBID_DEST
                );

        }
    }

    IFC( builder.Finalize() );


Cleanup:
    delete pSpan;

    if (FAILED(hr))
    {
        ReleaseExpensiveResources();
    }

    RRETURN(hr);
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::AssertNoExpensiveResources
//
//  Synopsis:
//      Checks that all "expensive resources" have been released using
//      ReleaseExpensiveResources.
//
//------------------------------------------------------------------------------

VOID
CScanPipelineRendering::AssertNoExpensiveResources()
{
    Assert(m_rgosdOwned.GetCount() == 0);
    CScanPipeline::AssertNoExpensiveResources();
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::ReleaseExpensiveResources
//
//  Synopsis:
//      Release potentially-expensive resources.
//
//  Notes:
//      AssertNoExpensiveResources needs to be updated if this function is
//      changed.
//
//------------------------------------------------------------------------------

VOID
CScanPipelineRendering::ReleaseExpensiveResources()
{
    for (UINT i = 0; i < m_rgosdOwned.GetCount(); i++)
    {
        delete m_rgosdOwned[i];
    }
    m_rgosdOwned.Reset();

    CScanPipeline::ReleaseExpensiveResources();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::SetAntialiasedFiller
//
//  Synopsis:
//      If there is a ScalePPAACoverage operation in the pipeline, updates its
//      op-specific data to point to the given CAntialiasedFiller.
//

void CScanPipelineRendering::SetAntialiasedFiller(
    __in_ecount(1) CAntialiasedFiller *pFiller
    )
{
    if (m_idxosdAAFiller >= 0)
    {
        m_rgPipeline[m_idxosdAAFiller].m_Params.m_posd =
            DowncastFiller(pFiller);
    }
}






