// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Contains implementation for Constant type HW Color Sources
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


MtDefine(CHwConstantMilColorFColorSource, MILRender, "CHwConstantMILColorColorSource");
MtDefine(CHwConstantAlphaColorSource, MILRender, "CHwConstantAlphaColorSource");
MtDefine(CHwConstantAlphaScalableColorSource, MILRender, "CHwConstantAlphaScalableColorSource");

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwConstantColorSource
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantColorSource::CHwConstantColorSource
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CHwConstantColorSource::CHwConstantColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) : m_pDevice(pDevice)
{
    m_pHwTexturedColorSource = NULL;

    ResetForPipelineReuse();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantColorSource::~CHwConstantColorSource
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------

CHwConstantColorSource::~CHwConstantColorSource()
{
    ReleaseInterfaceNoNULL(m_pHwTexturedColorSource);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantColorSource::GetSourceType
//
//  Synopsis:
//      Return color source type - always Constant, but sometimes also Textured
//      if SendVertexMapping requests a UV location.
//

CHwColorSource::TypeFlags
CHwConstantColorSource::GetSourceType() const
{
    return Constant | (m_pHwTexturedColorSource ? Texture : 0);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantColorSource::IsOpaque
//
//  Synopsis:
//      Does the source contain alpha?  This method tells you.
//
//------------------------------------------------------------------------------

bool
CHwConstantColorSource::IsOpaque(
    ) const
{
    MilColorF color;
    GetColor(OUT color);
    // Note this comparison is too restrictive for sRGB which has less
    // granularity and is considered opaque at values less than 1.
    return (color.a >= 1.0f);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantColorSource::SendVertexMapping
//
//  Synopsis:
//      Tell vertex builder this source is constant and should be filled into
//      the given vertex field
//

HRESULT
CHwConstantColorSource::SendVertexMapping(
    __inout_ecount_opt(1) CHwVertexBuffer::Builder *pVertexBuilder,
    MilVertexFormatAttribute mvfaLocation
    )
{
    HRESULT hr = S_OK;

    //Assert(pVertexBuilder);
    Assert(mvfaLocation != MILVFAttrNone);
    Assert(!(mvfaLocation & (MILVFAttrXYZ | MILVFAttrNormal)));

    if (mvfaLocation & MILVFAttrUV8)
    {
        MilColorF color;
        GetColor(OUT color);

        if (!m_pHwTexturedColorSource)
        {
            IFC(m_pDevice->GetSolidColorTexture(
                color,
                &m_pHwTexturedColorSource
                ));
        }
        else
        {
            m_pHwTexturedColorSource->SetColor(color);
        }

        IFC(m_pHwTexturedColorSource->SendVertexMapping(
            pVertexBuilder,
            mvfaLocation
            ));
    }
    else
    {
        Assert(pVertexBuilder);

        ReleaseInterface(m_pHwTexturedColorSource);

        IFC(pVertexBuilder->SetConstantMapping(
            mvfaLocation,
            this
            ));
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantColorSource::Realize
//
//  Synopsis:
//      There is nothing to be done to realize a constant color source, but if a
//      texture version has been requested (see SendVertexMapping) then delegate
//      to textured color source.
//

HRESULT
CHwConstantColorSource::Realize()
{
    HRESULT hr = S_OK;

    if (m_pHwTexturedColorSource)
    {
        hr = m_pHwTexturedColorSource->Realize();
    }

    // Using plain return to allow fast call when using textured color source
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantColorSource::SendDeviceStates
//
//  Synopsis:
//      There are no device states that need to be set for non-textured constant
//      color sources as the colors are specified in the vertex data.  For
//      textured versions delegate to textured color source.
//
//  Notes:
//      If the color data is to be sent as device state such as a material then
//      there may be some more work to do here.
//

HRESULT
CHwConstantColorSource::SendDeviceStates(
    DWORD dwStage,
    DWORD dwSampler
    )
{
    HRESULT hr = S_OK;

    if (dwSampler != INVALID_PIPELINE_SAMPLER)
    {
        Assert(m_pHwTexturedColorSource);

        hr = m_pHwTexturedColorSource->SendDeviceStates(
            dwStage,
            dwSampler
            );
    }

    // Using plain return to allow fast call when using textured color source
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantColorSource::SendShaderData
//
//  Synopsis:
//      Sends the color source data to the PipelineShader structure.
//

HRESULT 
CHwConstantColorSource::SendShaderData(
    __inout_ecount(1) CHwPipelineShader *pHwShader
    )
{
    HRESULT hr = S_OK;

    if (m_hShaderColorHandle != MILSP_INVALID_HANDLE)
    {
        MilColorF scRGBColor;
        MilColorF sRGBColor;

        GetColor(
            scRGBColor
            );
        
        sRGBColor = Convert_MilColorF_scRGB_To_MilColorF_sRGB(&scRGBColor);

        Premultiply(&sRGBColor);

        IFC(pHwShader->SetFloat4(
            m_hShaderColorHandle,
            reinterpret_cast<const float *>(&sRGBColor)
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwConstantMilColorFColorSource
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantMilColorFColorSource::Create
//
//  Synopsis:
//      Instantiate a HW Color Source for a constant scRGB color
//

__checkReturn HRESULT
CHwConstantMilColorFColorSource::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) const MilColorF &color,
    __deref_out_ecount(1) CHwConstantMilColorFColorSource ** const ppHwColorSource
    )
{
    HRESULT hr = S_OK;

    Assert(ppHwColorSource);

    *ppHwColorSource = new CHwConstantMilColorFColorSource(pDevice, color);
    IFCOOM(*ppHwColorSource);
    (*ppHwColorSource)->AddRef();

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppHwColorSource);
    }
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantMilColorFColorSource::CHwConstantMilColorFColorSource
//
//  Synopsis:
//      Public constructor
//
//------------------------------------------------------------------------------

CHwConstantMilColorFColorSource::CHwConstantMilColorFColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) const MilColorF &color
    ) : CHwConstantColorSource(pDevice)
{
    m_color = color;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantMilColorFColorSource::GetColor
//
//  Synopsis:
//      Return color
//

void
CHwConstantMilColorFColorSource::GetColor(
    __out_ecount(1) MilColorF &color
    ) const
{
    color = m_color;
}



//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwConstantAlphaColorSource
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaColorSource::CHwConstantAlphaColorSource
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CHwConstantAlphaColorSource::CHwConstantAlphaColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    FLOAT alpha
    ) : CHwConstantColorSource(pDevice)
{
    m_alpha = alpha;
    m_hShaderFloat = MILSP_INVALID_HANDLE;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaColorSource::~CHwConstantAlphaColorSource
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------

CHwConstantAlphaColorSource::~CHwConstantAlphaColorSource()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaColorSource::GetColor
//
//  Synopsis:
//      Return opaque, semi-, or fully-transparent white
//

void
CHwConstantAlphaColorSource::GetColor(
    __out_ecount(1) MilColorF &color
    ) const
{
    color.r = 1.0f;
    color.g = 1.0f;
    color.b = 1.0f;
    color.a = m_alpha;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaColorSource::SendShaderData
//
//  Synopsis:
//      Sends the Alpha multiplier data to the shader.
//

HRESULT 
CHwConstantAlphaColorSource::SendShaderData(
    __inout_ecount(1) CHwPipelineShader *pHwShader
    )
{
    HRESULT hr = S_OK;

    if (m_hShaderFloat != MILSP_INVALID_HANDLE)
    {
        MilColorF scRGBColor;

        GetColor(
            scRGBColor
            );

        IFC(pHwShader->SetFloat4(
            m_hShaderFloat,
            reinterpret_cast<float *>(&scRGBColor)
            ));
    }

Cleanup:
    RRETURN(hr);
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwConstantAlphaScalableColorSource
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaScalableColorSource::Create
//
//  Synopsis:
//      Instantiate a HW Color Source for an alpha scale time the given color
//
//      If not color is given opaque white is used.
//

__checkReturn HRESULT
CHwConstantAlphaScalableColorSource::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    FLOAT alpha,
    __in_ecount_opt(1) CHwConstantColorSource *pHwColorSource,
    __in_ecount(1) CBufferDispenser *pBufferDispenser,
    __deref_out_ecount(1) CHwConstantAlphaScalableColorSource ** const ppHwColorSource
    )
{
    HRESULT hr = S_OK;

    *ppHwColorSource = new(pBufferDispenser)
        CHwConstantAlphaScalableColorSource(pDevice, alpha, pHwColorSource);
    IFCOOM(*ppHwColorSource);
    (*ppHwColorSource)->AddRef();

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppHwColorSource);
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaScalableColorSource::CHwConstantAlphaScalableColorSource
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CHwConstantAlphaScalableColorSource::CHwConstantAlphaScalableColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    FLOAT alpha,
    __in_ecount_opt(1) CHwConstantColorSource *pHwColorSource
    )
    : CHwConstantAlphaColorSource(pDevice, alpha)
{
    m_pHwColorSource = pHwColorSource;

    ResetForPipelineReuse();

    if (m_pHwColorSource)
    {
        m_pHwColorSource->AddRef();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaScalableColorSource::~CHwConstantAlphaScalableColorSource
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------

CHwConstantAlphaScalableColorSource::~CHwConstantAlphaScalableColorSource()
{
    ReleaseInterfaceNoNULL(m_pHwColorSource);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaScalableColorSource::GetColor
//
//  Synopsis:
//      Return the orignally passed color scaled by the current alpha
//
//      If there was no original color, opaque white is scaled.
//

void
CHwConstantAlphaScalableColorSource::GetColor(
    __out_ecount(1) MilColorF &color
    ) const
{
    if (m_pHwColorSource)
    {
        m_pHwColorSource->GetColor(color);
        color.a *= m_alpha;
    }
    else
    {
        CHwConstantAlphaColorSource::GetColor(color);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaScalableColorSource::AlphaScale
//
//  Synopsis:
//      Scale (multiply) the current alpha value by the given scale
//

void
CHwConstantAlphaScalableColorSource::AlphaScale(
    FLOAT alphaScale
    )
{
    m_alpha *= alphaScale;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwConstantAlphaColorSource::SendShaderData
//
//  Synopsis:
//      Sends the Alpha multiplier data to the shader.
//

HRESULT 
CHwConstantAlphaScalableColorSource::SendShaderData(
    __inout_ecount(1) CHwPipelineShader *pHwShader
    )
{
    HRESULT hr = S_OK;
    MilColorF scRGBColor;
    MilColorF sRGBColor;
    MILSPHandle hParameter;

    //
    // This class may be used to multiply by alpha, or multiply an existing
    // color source by alpha.
    //
    // We need to check so we know which shader parameter to use when setting
    // the color.
    //

    if (m_pHwColorSource)
    {
        Assert(m_hShaderFloat == MILSP_INVALID_HANDLE);

        hParameter = m_pHwColorSource->GetShaderParameterHandle();
    }
    else
    {
        hParameter = m_hShaderFloat;
    }

    if (hParameter != MILSP_INVALID_HANDLE)
    {
        GetColor(
            scRGBColor
            );

        sRGBColor = Convert_MilColorF_scRGB_To_MilColorF_sRGB(&scRGBColor);

        Premultiply(&sRGBColor);

        IFC(pHwShader->SetFloat4(
            hParameter,
            reinterpret_cast<const float *>(&sRGBColor)
            ));
    }

Cleanup:
    RRETURN(hr);
};




