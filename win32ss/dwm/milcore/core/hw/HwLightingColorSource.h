// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_lighting
//      $Keywords:
//
//  $Description:
//      Contains definition for CHwLightingColorSource
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHwLightingColorSource);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwLightingColorSource
//
//  Synopsis:
//      A Color source that supplies lighing information either in a precomputed
//      color form or through instructions given to a shader.
//
//  Future Consideration:  Make class supply lighting to FF Pipeline.
//
//------------------------------------------------------------------------------
class CHwLightingColorSource :
    public CHwColorSource
{
    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetSourceType
    //
    //  Synopsis:
    //      This method returns the type of color source.
    //
    //--------------------------------------------------------------------------

public:
    static __checkReturn HRESULT Create(
        __in_ecount(1) CMILLightData const *pLightData,
        __deref_out_ecount(1) CHwLightingColorSource ** const ppHwLightingColorSource
        );

    override TypeFlags GetSourceType() const
    {
        return Programmatic;
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      IsOpaque
    //
    //  Synopsis:
    //      Does the source contain alpha?  This method tells you.
    //
    //--------------------------------------------------------------------------
    
    override bool IsOpaque() const
    {
        return !m_pLightDataNoRef->RequiresDestinationBlending();
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SendVertexMapping
    //
    //  Synopsis:
    //      This method sends the information needed by the vertex builder to
    //      generate vertex fields for this color source.
    //
    //--------------------------------------------------------------------------
    
    override HRESULT SendVertexMapping(
        __inout_ecount_opt(1) CHwVertexBuffer::Builder *pVertexBuilder, 
        MilVertexFormatAttribute mvfaLocation
        )
    {
        UNREFERENCED_PARAMETER(pVertexBuilder);
        UNREFERENCED_PARAMETER(mvfaLocation);

        return S_OK;
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      Realize
    //
    //  Synopsis:
    //      This method realizes the device consumable resources for this color
    //      source.
    //
    //--------------------------------------------------------------------------
    
    override HRESULT Realize()
    {
        return S_OK;
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SendDeviceStates
    //
    //  Synopsis:
    //      This method sends the render/stage/sampler states specific to this
    //      color source to the given device.
    //
    //--------------------------------------------------------------------------
    
    override HRESULT SendDeviceStates(
        DWORD dwStage, 
        DWORD dwSampler
        )
    {
        return S_OK;
    }

    override void ResetForPipelineReuse()
    {
        m_hParameter = MILSP_INVALID_HANDLE;
    }

    override HRESULT SendShaderData(
        __inout_ecount(1) CHwPipelineShader *pHwShader
        );

    float GetNormalScale() const
    {
        return m_pLightDataNoRef->GetNormalScale();
    }

    CHwShader::LightingValues GetLightingPass() const
    {
        return static_cast<CHwShader::LightingValues>(m_pLightDataNoRef->GetLightingPass());
    }

    UINT GetNumDirectionalLights() const
    {
        return m_pLightDataNoRef->GetNumDirectionalLights();
    }

    UINT GetNumPointLights() const
    {
        return m_pLightDataNoRef->GetNumPointLights();
    }

    UINT GetNumSpotLights() const
    {
        return m_pLightDataNoRef->GetNumSpotLights();
    }

    void SetFirstConstantParameter(MILSPHandle hParameter)
    {
        Assert(m_hParameter == MILSP_INVALID_HANDLE);

        m_hParameter = hParameter;
    }

private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwLightingColorSource));

    CHwLightingColorSource(
        __in_ecount(1) CMILLightData const *pLightData
        );

private:
    CMILLightData const * const m_pLightDataNoRef;

    MILSPHandle m_hParameter;
};


