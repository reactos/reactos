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
#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Member:
//      RenderingBuilder::Append_EffectList
//
//  Synopsis:
//      Appends operations which implement a given effect list.
//
//  Notes:
//      IMILEffectList is too general an API for this functionality. More
//      complex imaging effects would not be suitable at this stage of the
//      pipeline.
//

HRESULT RenderingBuilder::Append_EffectList(
    __in_ecount(1) IMILEffectList *pIEffectList,
    __in_ecount(1) const CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device> *pmatEffectToDevice,
    __in_ecount(1) const CContextState *pContextState,
    UINT uClipBoundsWidth,
    MilPixelFormat::Enum fmtBlendSource, // Any legitimate pipeline format
    __out_ecount(1) MilPixelFormat::Enum *pFmtBlendOutput
    )
{
    HRESULT hr = S_OK;

    UINT cEntries = 0;

    // Get the count of the transform blocks in the effect object.
    IFC(pIEffectList->GetCount(&cEntries));

    for (UINT i = 0; i < cEntries; i++)
    {
        CLSID clsid;
        UINT cbSize;
        UINT cResources;

        IFC(pIEffectList->GetCLSID(i, &clsid));
        IFC(pIEffectList->GetParameterSize(i, &cbSize));
        IFC(pIEffectList->GetResourceCount(i, &cResources));

        if (clsid == CLSID_MILEffectAlphaScale)
        {
            AlphaScaleParams alphaScale;

            // check the parameter size
            if (cbSize != sizeof(alphaScale))
            {
                RIP("AlphaScale parameter has unexpected size.");
                IFC(WGXERR_UNSUPPORTED_OPERATION);
            }
            else if (cResources != 0)
            {
                RIP("AlphaScale has unexpected number of resources.");
                IFC(WGXERR_UNSUPPORTED_OPERATION);
            }
            else
            {
                IFC(pIEffectList->GetParameters(i, cbSize, &alphaScale));
            }

            IFC(Append_AlphaScale(alphaScale.scale, fmtBlendSource, pFmtBlendOutput));
            fmtBlendSource = *pFmtBlendOutput;
        }
        else if (clsid == CLSID_MILEffectAlphaMask)
        {
            AlphaMaskParams alphaMask;
            CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> matMaskToDevice;
            IUnknown *pIUnknown = NULL;
            IWGXBitmapSource *pIMaskBitmap = NULL;

            // check the parameter size
            if (cbSize != sizeof(alphaMask))
            {
                RIP("AlphaMask parameter has unexpected size.");
                IFC(WGXERR_UNSUPPORTED_OPERATION);
            }
            else if (cResources != 1)
            {
                RIP("AlphaMask has unexpected number of resources.");
                IFC(WGXERR_UNSUPPORTED_OPERATION);
            }
            else
            {
                IFC(pIEffectList->GetParameters(i, cbSize, &alphaMask));
            }

            matMaskToDevice.SetToMultiplyResult(
                CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Effect>::ReinterpretBase(alphaMask.matTransform),*pmatEffectToDevice);

            IFC(pIEffectList->GetResources(
                i,
                cResources,
                reinterpret_cast<IUnknown **>(&pIUnknown))
                );

            IFC(pIUnknown->QueryInterface(
                IID_IWGXBitmapSource,
                reinterpret_cast<void **>(&pIMaskBitmap))
                );

            IFC(Append_AlphaMask(
                pIMaskBitmap,
                &matMaskToDevice,
                pContextState,
                uClipBoundsWidth,
                fmtBlendSource,
                pFmtBlendOutput
                ));
            fmtBlendSource = *pFmtBlendOutput;

            if (pIUnknown != NULL)
            {
                pIUnknown->Release();
            }

            if (pIMaskBitmap != NULL)
            {
                pIMaskBitmap->Release();
            }
        }
        else
        {
            IFC(WGXERR_UNSUPPORTED_OPERATION);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      RenderingBuilder::Append_AlphaMask
//
//  Synopsis:
//      Appends an alpha-mask operation which operates in-place on the source
//      data.
//

HRESULT
RenderingBuilder::Append_AlphaMask(
    __in_ecount(1) IWGXBitmapSource *pIMask,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatMaskToDevice,
    __in_ecount(1) const CContextState *pContextState,
    UINT uClipBoundsWidth,
    MilPixelFormat::Enum fmtBlendSource, // Legitimate pipeline color
    __out_ecount(1) MilPixelFormat::Enum *pFmtBlendOutput
    )
{
    HRESULT hr = S_OK;

    COwnedOSD *pNewOSD = NULL;
    ScanOpFunc pfnOp = NULL;

    switch (fmtBlendSource)
    {
    case MilPixelFormat::PBGRA32bpp:
    case MilPixelFormat::BGR32bpp:
    {
        CMaskAlphaSpan *pSpan = new CMaskAlphaSpan;
        IFCOOM(pSpan);
        pNewOSD = pSpan;    // pNewOSD now owns the lifetime

        if (fmtBlendSource == MilPixelFormat::PBGRA32bpp)
        {
            pfnOp = MaskAlpha_32bppPARGB_32bppPARGB;
        }
        else
        {
            pfnOp = MaskAlpha_32bppRGB_32bppPARGB;
        }
        *pFmtBlendOutput = MilPixelFormat::PBGRA32bpp;

        IFC(pSpan->Initialize(
                pIMask,
                pmatMaskToDevice,
                pContextState->RenderState->InterpolationMode,
                pContextState->RenderState->PrefilterEnable,
                pContextState->RenderState->PrefilterThreshold,
                uClipBoundsWidth
                ));

        break;
    }
    case MilPixelFormat::PRGBA128bppFloat:
    {
        CMaskAlphaSpan_scRGB *pSpan = new CMaskAlphaSpan_scRGB;
        IFCOOM(pSpan);
        pNewOSD = pSpan;    // pNewOSD now owns the lifetime

        pfnOp = MaskAlpha_128bppPABGR_128bppPABGR;
        *pFmtBlendOutput = MilPixelFormat::PRGBA128bppFloat;

        IFC(pSpan->Initialize(
                pIMask,
                pmatMaskToDevice,
                pContextState->RenderState->InterpolationMode,
                pContextState->RenderState->PrefilterEnable,
                pContextState->RenderState->PrefilterThreshold,
                uClipBoundsWidth
                ));
        break;
    }
    default:
        NO_DEFAULT("Unsupported pixel format for alpha mask.");
    }

    IFC(AddOp_Unary(pfnOp, pNewOSD, SP_BLENDSOURCE));

    CScanPipelineRendering *pSPR = GetPipelineRendering();
    Assert(pSPR);

    IFC(pSPR->m_rgosdOwned.Add(pNewOSD));
    pNewOSD = NULL;   // Ownership transfered to pSPR->m_rgosdOwned.

Cleanup:
    delete pNewOSD;

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      RenderingBuilder::AddOp_ScalePPAACoverage
//
//  Synopsis:
//      Add a "ScalePPAACoverage" operation for the given pixel format.
//
//------------------------------------------------------------------------------

HRESULT
RenderingBuilder::AddOp_ScalePPAACoverage(
    MilPixelFormat::Enum fmtBlendSource,
    bool fComplementAlpha,
    __out_ecount(1) MilPixelFormat::Enum *pFmtBlendOutput
    )
{
    HRESULT hr = S_OK;

    ScanOpFunc pfnOp = GetOp_ScalePPAACoverage(fmtBlendSource, fComplementAlpha, pFmtBlendOutput);

    IFC(AddOp_Unary(pfnOp, NULL, SP_BLENDSOURCE));

    // We only support 1 of these operations in the pipeline.
    // Check that we haven't added one already.

    CScanPipelineRendering *pSPR = GetPipelineRendering();
    Assert(pSPR);
    Assert(pSPR->m_idxosdAAFiller == -1);

    // Record the index of this operation so that m_posd can be updated later.

    pSPR->m_idxosdAAFiller = pSPR->m_rgPipeline.GetCount() - 1;

Cleanup:

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      RenderingBuilder::Append_AlphaScale
//
//  Synopsis:
//      Appends an alpha-scale operation which operates in-place on the source
//      data.
//

HRESULT RenderingBuilder::Append_AlphaScale(
    FLOAT flAlpha,
    MilPixelFormat::Enum fmtBlendSource,    // Either 32bppPBGRA, 32bppBGR, or 128bppPRGBA
    __out_ecount(1) MilPixelFormat::Enum *pFmtBlendOutput    
    )
{
    HRESULT hr = S_OK;

    COwnedOSD *pNewOSD = NULL;
    ScanOpFunc pfnOp = NULL;

    switch (fmtBlendSource)
    {
    case MilPixelFormat::PBGRA32bpp:
    case MilPixelFormat::BGR32bpp:
    {
        *pFmtBlendOutput = MilPixelFormat::PBGRA32bpp;
        CConstantAlphaSpan *pSpan = new CConstantAlphaSpan;
        IFCOOM(pSpan);
        pNewOSD = pSpan;    // pNewOSD now owns the lifetime

        if (fmtBlendSource == MilPixelFormat::PBGRA32bpp)
        {
            pfnOp = ConstantAlpha_32bppPARGB;
        }
        else
        {
            pfnOp = ConstantAlpha_32bppRGB;
        }

        IFC(pSpan->Initialize(flAlpha));
        break;
    }
    case MilPixelFormat::PRGBA128bppFloat:
    {
        *pFmtBlendOutput = MilPixelFormat::PRGBA128bppFloat;

        CConstantAlphaSpan_scRGB *pSpan = new CConstantAlphaSpan_scRGB;
        IFCOOM(pSpan);
        pNewOSD = pSpan;    // pNewOSD now owns the lifetime

        pfnOp = ConstantAlpha_128bppPABGR;

        IFC(pSpan->Initialize(flAlpha));
        break;
    }
    default:
    {
        NO_DEFAULT("Bad pixel format in Append_AlphaScale");
        break;
    }
    }

    IFC(AddOp_Unary(pfnOp, pNewOSD, SP_BLENDSOURCE));

    CScanPipelineRendering *pSPR = GetPipelineRendering();
    Assert(pSPR);
    IFC(pSPR->m_rgosdOwned.Add(pNewOSD));
    pNewOSD = NULL;   // Ownership transfered to pSPR->m_rgosdOwned.

Cleanup:
    delete pNewOSD;

    RRETURN(hr);
}





