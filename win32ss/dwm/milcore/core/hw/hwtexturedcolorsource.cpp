// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwTexturedColorSource implementation
//

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(CHwTexturedColorSource, MILRender, "CHwTexturedColorSource");

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::CHwTexturedColorSource
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------

CHwTexturedColorSource::CHwTexturedColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) : m_pDevice(pDevice)
{
    m_pFilterMode = &CD3DRenderState::sc_fmUnknown;
    m_taU = m_taV = static_cast<D3DTEXTUREADDRESS>(0);

    m_fMaskWithSourceClip = false;

    ResetForPipelineReuse();

#if DBG
    m_fDbgValidXSpaceToTextureUV = false;
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::~CHwTexturedColorSource
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------

CHwTexturedColorSource::~CHwTexturedColorSource()
{
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::GetSourceType
//
//  Synopsis:  Return Texture type
//
//-----------------------------------------------------------------------------

CHwColorSource::TypeFlags
CHwTexturedColorSource::GetSourceType(
    ) const
{
    return Texture;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::MVFAttrToCoordIndex
//
//  Synopsis:  Convert a vertex attribute to a texture coordinate index.
//

HRESULT
CHwTexturedColorSource::MVFAttrToCoordIndex(
    MilVertexFormatAttribute mvfaLocation,
    __out_ecount(1) DWORD * const pdwCoordIndex
    )
{
    HRESULT hr = S_OK;

    switch (static_cast<int>(mvfaLocation))
    {
    case MILVFAttrUV1:
        *pdwCoordIndex = 0;
        break;

    case (MILVFAttrUV2 & ~MILVFAttrUV1):
    case MILVFAttrUV2:
        *pdwCoordIndex = 1;
        break;

    case (MILVFAttrUV3 & ~MILVFAttrUV2):
    case MILVFAttrUV3:
        *pdwCoordIndex = 2;
        break;

    case (MILVFAttrUV4 & ~MILVFAttrUV3):
    case MILVFAttrUV4:
        *pdwCoordIndex = 3;
        break;

    default:
        IFC(E_NOTIMPL);
        break;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::SendVertexMapping
//
//  Synopsis:  Send the vertex mapping for this textured source to the vertex
//             builder
//

HRESULT
CHwTexturedColorSource::SendVertexMapping(
    __inout_ecount_opt(1) CHwVertexBuffer::Builder *pVertexBuilder,
    MilVertexFormatAttribute mvfaLocation
    )
{
    HRESULT hr = S_OK;

    Assert(mvfaLocation != MILVFAttrNone);

    m_fUseHwTransform = (pVertexBuilder == NULL);

    if (!m_fUseHwTransform)
    {
        //
        // Decode the coordinate index
        //

        DWORD dwCoordIndex;

        IFC(MVFAttrToCoordIndex(mvfaLocation, &dwCoordIndex));

        //
        // Send the mapping
        //

        IFC(pVertexBuilder->SetTextureMapping(
            dwCoordIndex,
            DWORD_MAX,  // Invalid index
            &GetDevicePointToTextureUV()
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::SetFilterAndWrapModes
//
//  Synopsis:  Sets the filter mode and wrap modes that will be used to
//             render the texture
//

void CHwTexturedColorSource::SetFilterAndWrapModes(
    MilBitmapInterpolationMode::Enum interpolationMode,
    D3DTEXTUREADDRESS taU,
    D3DTEXTUREADDRESS taV
    )
{
    SetFilterMode(interpolationMode);

    SetWrapModes(taU,taV);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwTexturedColorSource::SetFilterMode
//
//  Synopsis:
//      Sets the filter mode that will be used to render the texture
//
//-----------------------------------------------------------------------------

void
CHwTexturedColorSource::SetFilterMode(
    MilBitmapInterpolationMode::Enum interpolationMode
    )
{
    //
    // Determine render state filter from interpolation mode
    //

    if (interpolationMode == MilBitmapInterpolationMode::NearestNeighbor)
    {
        m_pFilterMode = &CD3DRenderState::sc_fmNearest;
    }
    else if (interpolationMode == MilBitmapInterpolationMode::TriLinear)
    {
        m_pFilterMode = &CD3DRenderState::sc_fmTriLinear;
    }
    else if (interpolationMode == MilBitmapInterpolationMode::Anisotropic)
    {
        m_pFilterMode = m_pDevice->GetSupportedAnistotropicFilterMode();
    }
    else
    {
        Assert(   (interpolationMode == MilBitmapInterpolationMode::Linear)
               || (interpolationMode == MilBitmapInterpolationMode::Cubic)
               );
        m_pFilterMode = &CD3DRenderState::sc_fmLinear;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwTexturedColorSource::SetWrapModes
//
//  Synopsis:
//      Sets the wrap modes that will be used to render the texture
//
//-----------------------------------------------------------------------------

void
CHwTexturedColorSource::SetWrapModes(
    D3DTEXTUREADDRESS taU,
    D3DTEXTUREADDRESS taV
    )
{
    // Set texture addressing/wrapping modes

    m_taU = taU;
    m_taV = taV;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::CalcTextureTransform
//
//  Synopsis:  Sets the matrix which transforms objects from device space
//             to source space.
//

HRESULT
CHwTexturedColorSource::CalcTextureTransform(
    __in_ecount(1) const BitmapToXSpaceTransform *pBitmapToXSpaceTransform,
    __range(>=, 1) UINT uTextureWidth,
    __range(>=, 1) UINT uTextureHeight
    )
{
    HRESULT hr = S_OK;

    Assert(uTextureWidth > 0);
    Assert(uTextureHeight > 0);

    //
    // Setup transform
    //
    // Compute device to texture transformation
    //  1) texture to source transform is scaled by width and height
    //  2) source to device transform is given
    //  3) multiply them then take the inverse
    //

    if (!m_matXSpaceToTextureUV.SetInverse(
            pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.m[0][0] * uTextureWidth,
            pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.m[0][1] * uTextureWidth,
            pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.m[1][0] * uTextureHeight,
            pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.m[1][1] * uTextureHeight,
            pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.GetDx(),
            pBitmapToXSpaceTransform->matBitmapSpaceToXSpace.GetDy()
            ))
    {
        IFC(WGXERR_NONINVERTIBLEMATRIX);
    }

    // Reset shader handle for this context use
    ResetShaderTextureTransformHandle();

#if DBG
    DbgMarkXSpaceToTextureUVAsSet(pBitmapToXSpaceTransform->dbgXSpaceDefinition);
#endif

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::SendDeviceStates
//
//  Synopsis:  Send related texture states to device
//

HRESULT
CHwTexturedColorSource::SendDeviceStates(
    DWORD dwStage,
    DWORD dwSampler
    )
{
    return SendDeviceStates(dwStage, dwSampler, dwStage);
}

HRESULT
CHwTexturedColorSource::SendDeviceStates(
    DWORD dwStage,
    DWORD dwSampler,
    DWORD dwTexCoordIndex    
    )
{
    HRESULT hr = S_OK;

    Assert(m_pDevice);

    IFC(m_pDevice->SetFilterMode(
        dwSampler,
        m_pFilterMode
        ));

    IFC(m_pDevice->SetSamplerState(
        dwSampler,
        D3DSAMP_ADDRESSU,
        m_taU
        ));

    IFC(m_pDevice->SetSamplerState(
        dwSampler,
        D3DSAMP_ADDRESSV,
        m_taV
        ));

    if (m_taU == D3DTADDRESS_BORDER)
    {
        IFC(m_pDevice->SetSamplerState(
            dwSampler,
            D3DSAMP_BORDERCOLOR,
            0
            ));
    }

    IFC(m_pDevice->SetTextureStageState(
        dwStage,
        D3DTSS_TEXCOORDINDEX,
        dwTexCoordIndex
        ));
    
    if (m_hTextureTransform == MILSP_INVALID_HANDLE)
    {
        //
        // If a transform is passed set the Hardware to transform the texture
        // coordinates.  Otherwise disable hardware transformation of texture
        // coordinates.
        //

        if (!m_fUseHwTransform)
        {
            IFC(m_pDevice->SetTextureStageState(
                dwStage,
                D3DTSS_TEXTURETRANSFORMFLAGS,
                D3DTTFF_DISABLE
                ));
        }
        else
        {
            const MILMatrix3x2& matBrushCoordToTextureUV = GetBrushCoordToTextureUV();
         
            CMILMatrix matTrans = matrix::get_identity();
            matTrans._11 = matBrushCoordToTextureUV.m_00;
            matTrans._12 = matBrushCoordToTextureUV.m_01;
            matTrans._21 = matBrushCoordToTextureUV.m_10;
            matTrans._22 = matBrushCoordToTextureUV.m_11;
            matTrans._31 = matBrushCoordToTextureUV.m_20;
            matTrans._32 = matBrushCoordToTextureUV.m_21;
    
            Assert(dwStage < 8);
    
            IFC(m_pDevice->SetTransform(
                static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0 + dwStage),
                &matTrans
                ));
    
            IFC(m_pDevice->SetTextureStageState(
                dwStage,
                D3DTSS_TEXTURETRANSFORMFLAGS,
                D3DTTFF_COUNT2
                ));
        }
    }
    // IFC(m_pDevice->SetTexture(dwSampler, ...)); done by subclass

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::SendShaderData
//
//  Synopsis:  Send the data down to the shader
//

HRESULT 
CHwTexturedColorSource::SendShaderData(
    __inout_ecount(1) CHwPipelineShader *pShader
    )
{
    HRESULT hr = S_OK;

    if (m_hTextureTransform != MILSP_INVALID_HANDLE)
    {
        const MILMatrix3x2& matBrushCoordToTextureUV = GetBrushCoordToTextureUV();

        IFC(pShader->SetMatrix3x2(
            m_hTextureTransform,
            &matBrushCoordToTextureUV
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwTexturedColorSource::ConvertWrapModeToTextureAddressModes
//
//  Synopsis:  
//      Converts a bitmap wrap mode into two dx texture addressing modes.
//

void
CHwTexturedColorSource::ConvertWrapModeToTextureAddressModes(
    MilBitmapWrapMode::Enum wrapMode,
    __out_ecount(1) D3DTEXTUREADDRESS *ptaU,
    __out_ecount(1) D3DTEXTUREADDRESS *ptaV
    )
{
    switch (wrapMode)
    {
    case MilBitmapWrapMode::Extend:
        *ptaU = D3DTADDRESS_CLAMP;
        *ptaV = D3DTADDRESS_CLAMP;
        break;

    case MilBitmapWrapMode::FlipX:
        *ptaU = D3DTADDRESS_MIRROR;
        *ptaV = D3DTADDRESS_WRAP;
        break;

    case MilBitmapWrapMode::FlipY:
        *ptaU = D3DTADDRESS_WRAP;
        *ptaV = D3DTADDRESS_MIRROR;
        break;

    case MilBitmapWrapMode::FlipXY:
        *ptaU = D3DTADDRESS_MIRROR;
        *ptaV = D3DTADDRESS_MIRROR;
        break;

    case MilBitmapWrapMode::Tile:
        *ptaU = D3DTADDRESS_WRAP;
        *ptaV = D3DTADDRESS_WRAP;
        break;

    case MilBitmapWrapMode::Border:
        *ptaU = D3DTADDRESS_BORDER;
        *ptaV = D3DTADDRESS_BORDER;
        break;

    default:
        RIPW(L"Unrecognized MILWrapMode");
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::SetMaskClipWorldSpace
//
//  Synopsis:  Set a parallelogram to which this color source should be
//             clipped using a mask color source.
//

HRESULT
CHwTexturedColorSource::SetMaskClipWorldSpace(
    __in_ecount_opt(1) const CParallelogram *pClipWorldSpace
    )
{
    HRESULT hr = S_OK;

    m_fMaskWithSourceClip = (pClipWorldSpace != NULL);
    
    if (m_fMaskWithSourceClip)
    {
        MilPoint2F rgPoints[4];
        pClipWorldSpace->GetFigure(0).GetParallelogramVertices(rgPoints);

        // Convert parallelogram into origin delta format, where the origin
        // is one corner two corners are formed by adding the two deltas respectively
        // and the third corner by adding both deltas at the same time.

        const MilPoint2F &ptOffset = rgPoints[0];
        MilPoint2F delta1, delta2;

        delta1.X = rgPoints[1].X - ptOffset.X;
        delta1.Y = rgPoints[1].Y - ptOffset.Y;
        // Remember rgPoints[2] is the opposite corner to [0]
        delta2.X = rgPoints[3].X - ptOffset.X;
        delta2.Y = rgPoints[3].Y - ptOffset.Y;
        
        m_matXSpaceToSourceClip.SetInverse(
              delta1.X,   delta1.Y,
              delta2.X,   delta2.Y,
            ptOffset.X, ptOffset.Y);

    }
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTexturedColorSource::GetMaskColorSource
//
//  Synopsis:  Returns an alpha mask color source (i.e. each pixel is
//             either opaque white or transparent black) that is opaque
//             within the mask clip that has been set on this color source.
//

HRESULT
CHwTexturedColorSource::GetMaskColorSource(
    __deref_out_ecount_opt(1) CHwBoxColorSource ** const ppColorSource
    ) const
{
    HRESULT hr = S_OK;

    *ppColorSource = NULL;
    CHwBoxColorSource *pBoxColorSource = NULL;

    if (m_fMaskWithSourceClip)
    {
        IFC(m_pDevice->GetScratchHwBoxColorSource(
            &m_matXSpaceToSourceClip,
            &pBoxColorSource
            ));

        // Steal reference.
        *ppColorSource = pBoxColorSource;
        pBoxColorSource = NULL;
    }

Cleanup:
    ReleaseInterfaceNoNULL(pBoxColorSource);
    
    RRETURN(hr);
}


