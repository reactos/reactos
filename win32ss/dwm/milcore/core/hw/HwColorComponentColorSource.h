// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_material
//      $Keywords:
//
//  $Description:
//      Contains definition for Color Component Color Source
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHwColorComponentSource);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwColorComponentSource
//
//  Synopsis:
//      Represents a stream of precomputed color values
//
//------------------------------------------------------------------------------
class CHwColorComponentSource :
    public CHwColorSource
{    
public:

    enum VertexComponent
    {
        Diffuse,
        Specular,
        Total
    };

    static __checkReturn HRESULT Create(
        CHwColorComponentSource::VertexComponent component,
        __deref_out_ecount(1) CHwColorComponentSource ** const ppHwColorSource
        );

public:

    CHwColorComponentSource::VertexComponent GetComponentLocation() const
    {
        return m_eSourceLocation;
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetSourceType
    //
    //  Synopsis:
    //      This method returns the type of color source.
    //
    //--------------------------------------------------------------------------
    
    override TypeFlags GetSourceType() const
    {
        return PrecomputedComponent;
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
        return false;
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
    }

    override HRESULT SendShaderData(
        __inout_ecount(1) CHwPipelineShader *pHwShader
        )
    {
        return S_OK;
    }

private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwColorComponentSource));

    CHwColorComponentSource();

    void SetComponentLocation(
        CHwColorComponentSource::VertexComponent eSourceLocation
        )
    {
        m_eSourceLocation = eSourceLocation;
    }

private:
    CHwColorComponentSource::VertexComponent m_eSourceLocation;
};



