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
//      Builder for CScanPipelineRendering
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
class RenderingBuilder : public ScanPipelineBuilder
{
public:

    inline RenderingBuilder(
        __in_ecount(1) CScanPipelineRendering *pSP,
        __inout_ecount(1) CSPIntermediateBuffers *pIntermediateBuffers,
        BuilderMode eBuilderMode
        ) : ScanPipelineBuilder(pSP, pIntermediateBuffers, eBuilderMode) {}

    HRESULT Append_EffectList(
        __in_ecount(1) IMILEffectList *pIEffectList,
        __in_ecount(1) const CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device> *pmatEffectToDevice,
        __in_ecount(1) const CContextState *pContextState,
        UINT uClipBoundsWidth,
        MilPixelFormat::Enum fmtBlendSource,
        __out_ecount(1) MilPixelFormat::Enum *pFmtBlendOutput
        );

    HRESULT AddOp_ScalePPAACoverage(
        MilPixelFormat::Enum fmtBlendSource,
        bool fComplementAlpha,
        __out_ecount(1) MilPixelFormat::Enum *pFmtBlendOutput
        );

protected:

    HRESULT Append_AlphaMask(
        __in_ecount(1) IWGXBitmapSource *pIMask,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatMaskToDevice,
        __in_ecount(1) const CContextState *pContextState,
        UINT uClipBoundsWidth,
        MilPixelFormat::Enum fmtBlendSource,
        __out_ecount(1) MilPixelFormat::Enum *pFmtBlendOutput
        );

    HRESULT Append_AlphaScale(
        FLOAT flAlpha,
        MilPixelFormat::Enum fmtBlendSource,
        __out_ecount(1) MilPixelFormat::Enum *pFmtBlendOutput
        );

private:
    inline CScanPipelineRendering *GetPipelineRendering()
    {
        Assert(m_pSP != NULL);
        return DYNCAST(CScanPipelineRendering, m_pSP);
    }
};



