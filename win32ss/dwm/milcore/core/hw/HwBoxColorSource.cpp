// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwBoxColorSource implementation
//

#include "precomp.hpp"

MtDefine(CHwBoxColorSource, MILRender, "CHwBoxColorSource");

//+----------------------------------------------------------------------------
//
//  Function:  CHwBoxColorSource::ctor
//
//  Synopsis:  Initializes the device and texture.
//
//-----------------------------------------------------------------------------

CHwBoxColorSource::CHwBoxColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
    )
    : CHwTexturedColorSource(pD3DDevice)
{
    SetFilterAndWrapModes(
        MilBitmapInterpolationMode::NearestNeighbor,
        D3DTADDRESS_CLAMP,
        D3DTADDRESS_CLAMP
        );
    
    m_alphaScale = 1.0f;
    m_alphaScaleRealized = -2.0f; // Start with an unreasonable alpha scale
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwBoxColorSource::dtor
//
//  Synopsis:  Releases the texture.
//
//-----------------------------------------------------------------------------

CHwBoxColorSource::~CHwBoxColorSource()
{
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwBoxColorSource::Create
//
//  Synopsis:  Creates a CHwBoxColorSource given a device.
//
//-----------------------------------------------------------------------------

__checkReturn HRESULT
CHwBoxColorSource::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    __deref_out_ecount(1) CHwBoxColorSource ** const ppTextureSource
    )
{
    HRESULT hr = S_OK;
    CHwBoxColorSource *pNewTextureSource = NULL;

    *ppTextureSource = NULL;

    pNewTextureSource = new CHwBoxColorSource(pD3DDevice);

    IFCOOM(pNewTextureSource);

    pNewTextureSource->AddRef();

    *ppTextureSource = pNewTextureSource;

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterfaceNoNULL(pNewTextureSource);
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBoxColorSource::AlphaScale
//
//  Synopsis:
//      Accumulate alpha scale factor.
//
//-----------------------------------------------------------------------------

void CHwBoxColorSource::AlphaScale(
    FLOAT alphaScale
    )
{
    m_alphaScale *= alphaScale;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBoxColorSource::SendDeviceStates
//
//  Synopsis:  Send the device states to render the solid color texture
//
//-----------------------------------------------------------------------------

HRESULT
CHwBoxColorSource::SendDeviceStates(
    DWORD dwStage,
    DWORD dwSampler
    )
{
    HRESULT hr = S_OK;

    Assert(m_alphaScaleRealized == m_alphaScale);

    IFC(CHwTexturedColorSource::SendDeviceStates(
        dwStage,
        dwSampler,
        0 /* Tex Coord Index */
        ));
    
    IFC(m_pDevice->SetTexture(dwSampler, m_vidMemManager.GetVidMemTextureNoRef()));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwBoxColorSource::Realize
//
//  Synopsis:  Should Realize any data necessary which includes filling the texture.
//
//-----------------------------------------------------------------------------

HRESULT
CHwBoxColorSource::Realize()
{
    HRESULT hr = S_OK;

    if (m_vidMemManager.GetVidMemTextureNoRef() == NULL)
    {
        // Mark any existing realization as invalid
        m_alphaScaleRealized = m_alphaScale-1.0f;
        Assert(m_alphaScaleRealized != m_alphaScale);

        if (!m_vidMemManager.HasRealizationParameters())
        {
            m_vidMemManager.SetRealizationParameters(
                m_pDevice,
                D3DFMT_A8R8G8B8,
                4,
                4,
                TMML_One
                DBG_COMMA_PARAM(true) // Conditional Non Pow 2 OK
                );
        }
    }

    if (m_alphaScaleRealized != m_alphaScale)
    {
        IFC(FillTexture());
        
        IFC(m_vidMemManager.PushBitsToVidMemTexture());

        // Mark realization as valid
        m_alphaScaleRealized = m_alphaScale;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwBoxColorSource::SetContext
//
//  Synopsis:  Sets the transformation matrix and intializes the box color
//             source for use.
//
//-----------------------------------------------------------------------------

void
CHwBoxColorSource::SetContext(
    __in_ecount(1) const MILMatrix3x2 *pMatXSpaceToSourceClip
    )
{
    m_matXSpaceToTextureUV = *pMatXSpaceToSourceClip;

    // So far matrix takes x space clip parallelogram to unit square.
    // We need it to go to the quarter-size square in the center
    // because that's how our texture is set up.
    m_matXSpaceToTextureUV.m_00 *= 0.5;
    m_matXSpaceToTextureUV.m_01 *= 0.5;
    m_matXSpaceToTextureUV.m_10 *= 0.5;
    m_matXSpaceToTextureUV.m_11 *= 0.5;
    m_matXSpaceToTextureUV.m_20 *= 0.5;
    m_matXSpaceToTextureUV.m_21 *= 0.5;
    m_matXSpaceToTextureUV.m_20 += 0.25;
    m_matXSpaceToTextureUV.m_21 += 0.25;

#if DBG
    DbgMarkXSpaceToTextureUVAsSet(XSpaceIsIrrelevant);
#endif    

    ResetAlphaScaleFactor();
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwBoxColorSource::FillTexture
//
//  Synopsis:  Locks the texture and populates it.
//
//-----------------------------------------------------------------------------

HRESULT
CHwBoxColorSource::FillTexture()
{
    HRESULT hr = S_OK;
    D3DLOCKED_RECT d3dRect;
    bool fLockedTexture = false;

    IFC(m_vidMemManager.ReCreateAndLockSysMemSurface(
        &d3dRect
        ));

    fLockedTexture = true;

    DWORD *pdwTexel = reinterpret_cast<DWORD *>(d3dRect.pBits);

    const DWORD c_0 = 0x00000000;

    Assert(0 <= m_alphaScale);
    Assert(m_alphaScale <= 1.0f);
    const UINT uChannelScale = CFloatFPU::SmallRound(m_alphaScale*255.0f);

    const DWORD c_1 = MIL_COLOR(uChannelScale, uChannelScale, uChannelScale, uChannelScale);

    pdwTexel[0x0] = c_0;
    pdwTexel[0x1] = c_0;
    pdwTexel[0x2] = c_0;
    pdwTexel[0x3] = c_0;

    pdwTexel[0x4] = c_0;
    pdwTexel[0x5] = c_1;
    pdwTexel[0x6] = c_1;
    pdwTexel[0x7] = c_0;
    
    pdwTexel[0x8] = c_0;
    pdwTexel[0x9] = c_1;
    pdwTexel[0xa] = c_1;
    pdwTexel[0xb] = c_0;

    pdwTexel[0xc] = c_0;
    pdwTexel[0xd] = c_0;
    pdwTexel[0xe] = c_0;
    pdwTexel[0xf] = c_0;
    
Cleanup:
    if (fLockedTexture)
    {
        MIL_THR_SECONDARY(m_vidMemManager.UnlockSysMemSurface());
    }
    
    RRETURN(hr);
}


