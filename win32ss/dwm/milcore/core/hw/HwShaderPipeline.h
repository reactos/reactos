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
//      Contains the CHwPipelineShader Shader definition.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------



MtExtern(CHwPipelineShader);

#define NUM_ROWS_CONST_TABLES 64
#define PIXEL_SHADER_TABLE_OFFSET 0x80000000

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwPipelineShader
//
//  Synopsis:
//      A shader class that keeps track of a vertex shader, pixel shader, and
//      input parameters.  It allows the vertex and pixel shader constant data
//      to be set through some functions it exposes:
//
//        SetMatrix4x4
//        SetFloat4
//
//------------------------------------------------------------------------------

class CHwPipelineShader :
    public CMILRefCountBase
{
private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwPipelineShader));

public:
    static HRESULT Create(
        __in_ecount(uNumPipelineItems) const HwPipelineItem * rgShaderPipelineItems,
        UINT uNumPipelineItems,
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) IDirect3DVertexShader9 *pVertexShader,
        __in_ecount(1) IDirect3DPixelShader9  *pPixelShader,
        __deref_out_ecount(1) CHwPipelineShader **ppHwShader
        DBG_COMMA_PARAM(__inout_ecount_opt(1) PCSTR &pDbgHLSLSource)
        );

    HRESULT SetState(bool f2D);

    HRESULT SetMatrix4x4(
        MILSPHandle hMatrixParameter,
        __in_ecount(1) const CMILMatrix *pmatTransform
        );

    HRESULT SetMatrix3x2(
        MILSPHandle hMatrixParameter,
        __in_ecount(1) const MILMatrix3x2 *pmatTransform3x2
        );
    
    HRESULT SetFloat(
        MILSPHandle hParameter,
        __in_ecount(1) float flValue
        );

    HRESULT SetFloat2(
        MILSPHandle hParameter,
        __in_ecount(2) const float *rgFloats
        );

    HRESULT SetFloat3(
        MILSPHandle hParameter,
        __in_ecount(3) const float *rgFloats,
        float flFourthValue
        );

    HRESULT SetFloat4(
        MILSPHandle hMatrixParameter,
        __in_ecount(4) const float *rgFloats
        );

private:
    CHwPipelineShader(
        __in_ecount(1) CD3DDeviceLevel1 * const pDevice
        );

    ~CHwPipelineShader();

    HRESULT Init(         
        __in_ecount(uNumPipelineItems) const HwPipelineItem * rgShaderPipelineItems,
        UINT uNumPipelineItems,
        __in_ecount(1) IDirect3DVertexShader9 *pVertexShader,
        __in_ecount(1) IDirect3DPixelShader9  *pPixelShader
        DBG_COMMA_PARAM(__inout_ecount_opt(1) PCSTR &pDbgHLSLSource)
        );

    HRESULT InitParameterTable(
        __in_ecount(uNumPipelineItems) const HwPipelineItem * rgShaderPipelineItems,
        UINT uNumPipelineItems
        );

    HRESULT SetFloat4Internal(
        MILSPHandle hMatrixParameter,
        __in_ecount(4) const float *rgFloats
        );

#if DBG
    void DbgVerifyParameter(
        MILSPHandle hMatrixParameter,
        ShaderFunctionConstantData::Enum type
        );
#endif

private:
    CD3DDeviceLevel1 * const m_pDeviceNoRef;

    IDirect3DVertexShader9 *m_pVertexShader;
    IDirect3DPixelShader9  *m_pPixelShader;

#if DBG
    PCSTR m_pDbgHLSLSource;
#endif

    DynArray<ShaderFunctionConstantData::Enum> m_rgVertexShaderParameters;
    DynArray<ShaderFunctionConstantData::Enum> m_rgPixelShaderParameters;
};


