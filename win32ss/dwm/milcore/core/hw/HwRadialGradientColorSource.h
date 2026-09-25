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
//      Contains CHwRadialGradientColorSource declaration
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHwRadialGradientColorSource);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwRadialGradientColorSource
//
//  Synopsis:
//      Provides a radial gradient color source for a HW device
//
//------------------------------------------------------------------------------

class CHwRadialGradientColorSource : public CHwLinearGradientColorSource
{
public:

    static HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __deref_out_ecount(1) CHwRadialGradientColorSource **ppHwLinGradCS
        );

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwRadialGradientColorSource));

    CHwRadialGradientColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );
    ~CHwRadialGradientColorSource();

public:

    BOOL HasSeperateOriginFromCenter() const;

    void SetNonCenteredRadialGradientParamData(
        MILSPHandle hptGradientOrigin,
        MILSPHandle hptFirstTexelRegionCenter,
        MILSPHandle hflGradientSpanNormalized,
        MILSPHandle hflHalfTexelSizeNormalized
        )
    {
        m_hptGradientOrigin             = hptGradientOrigin;
        m_hptFirstTexelRegionCenter     = hptFirstTexelRegionCenter;
        m_hflGradientSpanNormalized     = hflGradientSpanNormalized;
        m_hflHalfTexelSizeNormalized    = hflHalfTexelSizeNormalized;
    }

    void SetCenteredRadialGradientParamData(
        MILSPHandle hflHalfTexelSizeNormalized
        )
    {
        m_hptGradientOrigin             = MILSP_INVALID_HANDLE;
        m_hptFirstTexelRegionCenter     = MILSP_INVALID_HANDLE;
        m_hflGradientSpanNormalized     = MILSP_INVALID_HANDLE;
        m_hflHalfTexelSizeNormalized    = hflHalfTexelSizeNormalized;
    }

    override HRESULT SendShaderData(
        __inout_ecount(1) CHwPipelineShader *pShader
        );

private:
    MILSPHandle m_hptGradientOrigin;
    MILSPHandle m_hptFirstTexelRegionCenter;
    MILSPHandle m_hflGradientSpanNormalized;
    MILSPHandle m_hflHalfTexelSizeNormalized;
};



