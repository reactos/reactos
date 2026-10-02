// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    wim_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Contains CHwRadialGradientColorSource implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CHwRadialGradientColorSource, MILRender, "CHwRadialGradientColorSource");

//+-----------------------------------------------------------------------------
//
//  Structure:
//      NonCenteredGradInfoParamsType
//
//  Synopsis:
//      Additional structure for a non-centered radial gradient
//
//------------------------------------------------------------------------------
struct NonCenteredGradInfoParamsType
{
    // Gradient origin coordinates in unit circle space 
    float ptGradOriginX;
    float ptGradOriginY;

    // Center of inner circle with radius flHalfTexelSizeNormalized
    // where anything inside the circle should be given the color of the first texel.
    float ptFirstTexelRegionCenterX;
    float ptFirstTexelRegionCenterY;

    // number of texels covered by base portion of gradient, normalized to [0-1] space
    float flGradientSpanNormalized;

    // the center of the first texel of the gradient texture, in normalized [0-1] space
    float flHalfTexelSizeNormalized;
};

//+-----------------------------------------------------------------------------
//
//  Structure:
//      CenteredGradInfoParamsType
//
//  Synopsis:
//      Additional structure for a centered radial gradient
//
//------------------------------------------------------------------------------
struct CenteredGradInfoParamsType
{
    // the center of the first texel of the gradient texture, in normalized [0-1] space
    float flHalfTexelSizeNormalized;
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientColorSource::Create
//
//  Synopsis:
//      Creates a HW radial gradient color source
//
//------------------------------------------------------------------------------
HRESULT
CHwRadialGradientColorSource::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __deref_out_ecount(1) CHwRadialGradientColorSource **ppHwRadialGradCS
    )
{
    HRESULT hr = S_OK;

    *ppHwRadialGradCS = new CHwRadialGradientColorSource(pDevice);
    IFCOOM(*ppHwRadialGradCS);
    (*ppHwRadialGradCS)->AddRef();

Cleanup:
    if (FAILED(hr))
    {
        Assert(*ppHwRadialGradCS == NULL);
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientColorSource::CHwRadialGradientColorSource
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CHwRadialGradientColorSource::CHwRadialGradientColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) :
    CHwLinearGradientColorSource(pDevice)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientColorSource::~CHwRadialGradientColorSource
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CHwRadialGradientColorSource::~CHwRadialGradientColorSource()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientColorSource::HasSeperateOriginFromCenter
//
//  Synopsis:
//      Forwards the call to the CMILBrushRadialGradient to find out if this is
//      a centered/non centered radial gradient.
//
//------------------------------------------------------------------------------
BOOL
CHwRadialGradientColorSource::HasSeperateOriginFromCenter() const
{
    const CMILBrushRadialGradient *pRadialGradientBrushNoRef =
        DYNCAST(const CMILBrushRadialGradient, CHwLinearGradientColorSource::GetGradientBrushNoRef());
    Assert(pRadialGradientBrushNoRef);

    return pRadialGradientBrushNoRef->HasSeparateOriginFromCenter();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientColorSource::SendShaderData
//
//  Synopsis:
//      Sends the linear gradient shader data, then determines whether it's a
//      centered or non-centered radial gradient and sets the appropriate data
//      in the shader constants.
//
//------------------------------------------------------------------------------
HRESULT
CHwRadialGradientColorSource::SendShaderData(
    __inout_ecount(1) CHwPipelineShader *pShader
    )
{
    HRESULT hr = S_OK;

    IFC(CHwLinearGradientColorSource::SendShaderData(
        pShader
        ));

    const CMILBrushRadialGradient *pRadialGradientBrushNoRef =
        DYNCAST(const CMILBrushRadialGradient, this->GetGradientBrushNoRef());
    Assert(pRadialGradientBrushNoRef);

    if (pRadialGradientBrushNoRef->HasSeparateOriginFromCenter())
    {
        NonCenteredGradInfoParamsType gradInfo;

        CMilPoint2F ptGradOriginUnitCircleSpace;   

        Assert(m_hptGradientOrigin != MILSP_INVALID_HANDLE);
        Assert(m_hptFirstTexelRegionCenter != MILSP_INVALID_HANDLE);
        Assert(m_hflGradientSpanNormalized != MILSP_INVALID_HANDLE);
        Assert(m_hflHalfTexelSizeNormalized != MILSP_INVALID_HANDLE);

        GetWorld2DToTexture()->Transform(
            &pRadialGradientBrushNoRef->GetGradientOrigin(),
            OUT &ptGradOriginUnitCircleSpace
            );

        gradInfo.ptGradOriginX = ptGradOriginUnitCircleSpace.X;
        gradInfo.ptGradOriginY = ptGradOriginUnitCircleSpace.Y;

        gradInfo.flHalfTexelSizeNormalized = 0.5f / GetTexelCount();

        {
            FLOAT flCenterWeight = 0.5f / CFloatFPU::Ceiling(GetGradientSpanEnd());

            // See brushspan.cpp for a description of why we calculate this
            // region and how the math works. This is an interpolation between
            // the center (0, 0) and the gradient origin.
            CMilPoint2F ptFirstTexelRegionCenter =
                  ptGradOriginUnitCircleSpace * (1 - flCenterWeight);

            gradInfo.ptFirstTexelRegionCenterX = ptFirstTexelRegionCenter.X;
            gradInfo.ptFirstTexelRegionCenterY = ptFirstTexelRegionCenter.Y;
        }

        gradInfo.flGradientSpanNormalized = 
              GetGradientSpanEnd()
            / static_cast<float>(GetTexelCount());

        float ptGradientOrigin[2] = { gradInfo.ptGradOriginX, gradInfo.ptGradOriginY };

        IFC(pShader->SetFloat2(
            m_hptGradientOrigin,
            ptGradientOrigin
            ));

        float ptFirstTexelRegionCenter[2] = { gradInfo.ptFirstTexelRegionCenterX, gradInfo.ptFirstTexelRegionCenterY };

        IFC(pShader->SetFloat2(
            m_hptFirstTexelRegionCenter,
            ptFirstTexelRegionCenter
            ));

        IFC(pShader->SetFloat(
            m_hflGradientSpanNormalized,
            gradInfo.flGradientSpanNormalized
            ));

        IFC(pShader->SetFloat(
            m_hflHalfTexelSizeNormalized,
            gradInfo.flHalfTexelSizeNormalized
            ));
    }
    else
    {
        CenteredGradInfoParamsType gradInfo;

        //
        // If we're in the centered scenario, all the other handles should be invalid
        Assert(m_hptGradientOrigin == MILSP_INVALID_HANDLE);
        Assert(m_hptFirstTexelRegionCenter == MILSP_INVALID_HANDLE);
        Assert(m_hflGradientSpanNormalized == MILSP_INVALID_HANDLE);

        Assert(m_hflHalfTexelSizeNormalized != MILSP_INVALID_HANDLE);

        gradInfo.flHalfTexelSizeNormalized = 0.5f / GetTexelCount();

        IFC(pShader->SetFloat(
            m_hflHalfTexelSizeNormalized,
            gradInfo.flHalfTexelSizeNormalized
            ));
    }

Cleanup:
    RRETURN(hr);
}




