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
//      Declaration of CHwPipelineShader
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CHwPipelineShader, MILRender, "CHwPipelineShader");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::Create
//
//  Synopsis:
//      Create the shader.
//

HRESULT
CHwPipelineShader::Create(
    __in_ecount(uNumPipelineItems) const HwPipelineItem * rgShaderPipelineItems,
    UINT uNumPipelineItems,
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) IDirect3DVertexShader9 *pVertexShader,
    __in_ecount(1) IDirect3DPixelShader9  *pPixelShader,
    __deref_out_ecount(1) CHwPipelineShader **ppHwShader
    DBG_COMMA_PARAM(__inout_ecount_opt(1) PCSTR &pDbgHLSLSource)
    )
{
    HRESULT hr = S_OK;
    CHwPipelineShader *pNewShader = NULL;

    pNewShader = new CHwPipelineShader(
        pDevice
        );

    IFCOOM(pNewShader);

    pNewShader->AddRef();

    IFC(pNewShader->Init(
        rgShaderPipelineItems,
        uNumPipelineItems,
        pVertexShader,
        pPixelShader
        DBG_COMMA_PARAM(pDbgHLSLSource)
        ));

    *ppHwShader = pNewShader;
    pNewShader = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewShader);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::SetState
//
//  Synopsis:
//      Send all the shader state down to d3d.
//

HRESULT 
CHwPipelineShader::SetState(bool f2D)
{
    HRESULT hr = S_OK;

    if (f2D)
    {
        IFC(m_pDeviceNoRef->Set2DTransformForVertexShader(
            0
            ));
    }
    else
    {
        IFC(m_pDeviceNoRef->Set3DTransformForVertexShader(
            0
            ));
    }

    IFC(m_pDeviceNoRef->SetVertexShader(m_pVertexShader));
    IFC(m_pDeviceNoRef->SetPixelShader(m_pPixelShader));

    // Important: If the texture states are not default the vertex shader
    //            will not work.
    IFC(m_pDeviceNoRef->SetDefaultTexCoordIndices());

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::SetMatrix4x4
//
//  Synopsis:
//      Set a matrix parameter in the shader.
//

HRESULT 
CHwPipelineShader::SetMatrix4x4(
    MILSPHandle hMatrixParameter,
    __in_ecount(1) const CMILMatrix *pmatTransform
    )
{

#if DBG
    DbgVerifyParameter(
        hMatrixParameter,
        ShaderFunctionConstantData::Matrix4x4
        );
#endif

    HRESULT hr = S_OK;

    if (hMatrixParameter >= PIXEL_SHADER_TABLE_OFFSET)
    {
        hMatrixParameter -= PIXEL_SHADER_TABLE_OFFSET;

        IFC(m_pDeviceNoRef->SetPixelShaderConstantF(
            GetShaderConstantRegister(hMatrixParameter),
            *pmatTransform,
            ShaderConstantTraits<ShaderFunctionConstantData::Matrix4x4>::RegisterSize
            ));
    }
    else
    {
        IFC(m_pDeviceNoRef->SetVertexShaderConstantF(
            GetShaderConstantRegister(hMatrixParameter),
            *pmatTransform,
            ShaderConstantTraits<ShaderFunctionConstantData::Matrix4x4>::RegisterSize
            ));
    }

Cleanup:
    RRETURN(hr)
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::SetMatrix3x2
//
//  Synopsis:
//      Set a matrix 3x2 in the shader
//

HRESULT
CHwPipelineShader::SetMatrix3x2(
    MILSPHandle hMatrixParameter,
    __in_ecount(1) const MILMatrix3x2 *pmatTransform3x2
    )
{
#if DBG
    DbgVerifyParameter(
        hMatrixParameter,
        ShaderFunctionConstantData::Matrix3x2
        );
#endif

    //                                                                  
    // Matrix3x2 aren't supported yet in pixel shaders, no real reason, just
    // haven't written the code.
    //
    Assert(hMatrixParameter < PIXEL_SHADER_TABLE_OFFSET);

    C_ASSERT(ShaderConstantTraits<ShaderFunctionConstantData::Matrix3x2>::RegisterSize == 2);

    float flInternalMatrix[2][4];

    //
    // Future Consideration:  Pack the matrix differently
    //
    // We can get the most efficient multiply and storage by packing the
    // transform differently:
    //
    //  +-----+-----+-----+-----+
    //  |     |     |     |     |
    //  | 00  | 10  | 20  | 21  |
    //  |     |     |     |     |
    //  +-----+-----+-----+-----+
    //  |     |     |     |     |
    //  | 10  | 11  |     |     |
    //  |     |     |     |     |
    //  +-----+-----+-----+-----+
    //
    //  This will keep the const register usage down to 2 (2 * 4 floats),
    //  and keep the data aligned for efficient multiplication and add.
    //

    flInternalMatrix[0][0] = pmatTransform3x2->m_00;
    flInternalMatrix[0][1] = pmatTransform3x2->m_10;
    flInternalMatrix[0][2] = pmatTransform3x2->m_20;
    flInternalMatrix[0][3] = 0.0f;

    flInternalMatrix[1][0] = pmatTransform3x2->m_01;
    flInternalMatrix[1][1] = pmatTransform3x2->m_11;
    flInternalMatrix[1][2] = pmatTransform3x2->m_21;
    flInternalMatrix[1][3] = 0.0f;

    HRESULT hr = S_OK;

    IFC(m_pDeviceNoRef->SetVertexShaderConstantF(
        GetShaderConstantRegister(hMatrixParameter),
        &flInternalMatrix[0][0],
        ShaderConstantTraits<ShaderFunctionConstantData::Matrix3x2>::RegisterSize
        ));

Cleanup:
    RRETURN(hr)
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::SetFloat4
//
//  Synopsis:
//      Set an array of floats in the shader.
//

HRESULT 
CHwPipelineShader::SetFloat4(
    MILSPHandle hParameter,
    __in_ecount(4) const float *rgFloats
    )
{
    HRESULT hr = S_OK;

#if DBG
    DbgVerifyParameter(
        hParameter,
        ShaderFunctionConstantData::Float4
        );
#endif

    IFC(SetFloat4Internal(
        hParameter,
        rgFloats
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::SetFloat3
//
//  Synopsis:
//      Set an array of floats in the shader.
//

HRESULT 
CHwPipelineShader::SetFloat3(
    MILSPHandle hParameter,
    __in_ecount(3) const float *rgFloats,
    float fourthValue
    )
{
    HRESULT hr = S_OK;

#if DBG
    DbgVerifyParameter(
        hParameter,
        ShaderFunctionConstantData::Float3
        );
#endif

    float rgFourFloats[4];

    rgFourFloats[0] = rgFloats[0];
    rgFourFloats[1] = rgFloats[1];
    rgFourFloats[2] = rgFloats[2];
    rgFourFloats[3] = fourthValue;

    IFC(SetFloat4Internal(
        hParameter,
        rgFourFloats
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::SetFloat2
//
//  Synopsis:
//      Set an array of floats in the shader.
//

HRESULT 
CHwPipelineShader::SetFloat2(
    MILSPHandle hParameter,
    __in_ecount(2) const float *rgFloats
    )
{
    HRESULT hr = S_OK;

#if DBG
    DbgVerifyParameter(
        hParameter,
        ShaderFunctionConstantData::Float2
        );
#endif

    float rgFourFloats[4];

    rgFourFloats[0] = rgFloats[0];
    rgFourFloats[1] = rgFloats[1];
    rgFourFloats[2] = 0.0f;
    rgFourFloats[3] = 0.0f;

    IFC(SetFloat4Internal(
        hParameter,
        rgFourFloats
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::SetFloat
//
//  Synopsis:
//      Set a float in the shader.
//

HRESULT 
CHwPipelineShader::SetFloat(
    MILSPHandle hParameter,
    __in_ecount(1) float flValue
    )
{
    HRESULT hr = S_OK;

#if DBG
    DbgVerifyParameter(
        hParameter,
        ShaderFunctionConstantData::Float
        );
#endif

    float rgFourFloats[4];

    rgFourFloats[0] = flValue;
    rgFourFloats[1] = 0.0f;
    rgFourFloats[2] = 0.0f;
    rgFourFloats[3] = 0.0f;

    IFC(SetFloat4Internal(
        hParameter,
        rgFourFloats
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::CHwPipelineShader
//
//  Synopsis:
//      Initialize the members.
//

CHwPipelineShader::CHwPipelineShader(
    __in_ecount(1) CD3DDeviceLevel1 * const pDevice
    )
    : m_pDeviceNoRef(pDevice)
{
    m_pVertexShader = NULL;
    m_pPixelShader = NULL;

#if DBG
    m_pDbgHLSLSource = NULL;
#endif
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::~CHwPipelineShader
//
//  Synopsis:
//      Release the shaders and string.
//

CHwPipelineShader::~CHwPipelineShader()
{
    ReleaseInterfaceNoNULL(m_pVertexShader);
    ReleaseInterfaceNoNULL(m_pPixelShader);

#if DBG
    WPFFree(
        ProcessHeap, 
        const_cast<PSTR>(m_pDbgHLSLSource)
        );
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::Init
//
//  Synopsis:
//      Set the shaders, copy the string, and initialize the parameter table.
//

HRESULT
CHwPipelineShader::Init(
    __in_ecount(uNumPipelineItems) const HwPipelineItem * rgShaderPipelineItems,
    UINT uNumPipelineItems,
    __in_ecount(1) IDirect3DVertexShader9 *pVertexShader,
    __in_ecount(1) IDirect3DPixelShader9  *pPixelShader
    DBG_COMMA_PARAM(__inout_ecount_opt(1) PCSTR &pDbgHLSLSource)
    )
{
    HRESULT hr = S_OK;

    m_pVertexShader = pVertexShader;
    m_pPixelShader = pPixelShader;

#if DBG
    // Steal HLSL source ownership
    m_pDbgHLSLSource = pDbgHLSLSource;
    pDbgHLSLSource = NULL;
#endif

    if (m_pVertexShader)
    {
        m_pVertexShader->AddRef();
    }

    if (m_pPixelShader)
    {
        m_pPixelShader->AddRef();
    }

    IFC(InitParameterTable(
        rgShaderPipelineItems,
        uNumPipelineItems
        ));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::InitParameterTable
//
//  Synopsis:
//      Walk through the fragments accumulating data on everything that must be
//      set in the shader.
//

HRESULT
CHwPipelineShader::InitParameterTable(
    __in_ecount(uNumPipelineItems) const HwPipelineItem * rgShaderPipelineItems,
    UINT uNumPipelineItems
    )
{
    HRESULT hr = S_OK;

    for (UINT uFragNum = 0; uFragNum < uNumPipelineItems; uFragNum++)
    {
        const ShaderFunction *pFragment = rgShaderPipelineItems[uFragNum].pFragment;
    
        //
        // Vertex Shader First
        //
        for (UINT uVertexParam = 0; 
                  uVertexParam < pFragment->VertexShader.NumConstDataParameters; 
                  uVertexParam++
                  )
        {
            IFC(m_rgVertexShaderParameters.Add(
                pFragment->VertexShader.rgConstDataParameter[uVertexParam].Type
                ));
        }

        for (UINT uPixelParam = 0;
                  uPixelParam < pFragment->PixelShader.NumConstDataParameters;
                  uPixelParam++
                  )
        {
            IFC(m_rgPixelShaderParameters.Add(
                pFragment->PixelShader.rgConstDataParameter[uPixelParam].Type
                ));
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::SetFloat4Internal
//
//  Synopsis:
//      Set an array of floats in the shader.
//

HRESULT 
CHwPipelineShader::SetFloat4Internal(
    MILSPHandle hParameter,
    __in_ecount(4) const float *rgFloats
    )
{
    HRESULT hr = S_OK;

    if (hParameter >= PIXEL_SHADER_TABLE_OFFSET)
    {
        UINT uStartRegister = (hParameter - PIXEL_SHADER_TABLE_OFFSET);

        IFC(m_pDeviceNoRef->SetPixelShaderConstantF(
            GetShaderConstantRegister(uStartRegister),
            rgFloats,
            ShaderConstantTraits<ShaderFunctionConstantData::Float4>::RegisterSize
            ));
    }
    else
    {
        UINT uStartRegister = hParameter;

        IFC(m_pDeviceNoRef->SetVertexShaderConstantF(
            GetShaderConstantRegister(uStartRegister),
            rgFloats,
            ShaderConstantTraits<ShaderFunctionConstantData::Float4>::RegisterSize
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineShader::DbgVerifyParameter
//
//  Synopsis:
//      Verify that the parameter type passed in matches that which is expected.
//

#if DBG
void
CHwPipelineShader::DbgVerifyParameter(
    MILSPHandle hMatrixParameter,
    ShaderFunctionConstantData::Enum type
    )
{
    UINT uParamNum = 0;

    if (hMatrixParameter >= PIXEL_SHADER_TABLE_OFFSET)
    {
        UINT uCurrentPixelSlot = 0;

        Assert((hMatrixParameter - PIXEL_SHADER_TABLE_OFFSET) + GetShaderConstantRegisterSize(type) < NUM_ROWS_CONST_TABLES*4);

        while (uCurrentPixelSlot < hMatrixParameter - PIXEL_SHADER_TABLE_OFFSET)
        {
            uCurrentPixelSlot += GetShaderConstantRegisterSize(m_rgPixelShaderParameters[uParamNum]);

            uParamNum++;

            Assert(uParamNum < m_rgPixelShaderParameters.GetCount());
        }

        Assert(m_rgPixelShaderParameters[uParamNum] == type);
    }
    else
    {
        UINT uCurrentVertexSlot = 0;

        Assert(hMatrixParameter + GetShaderConstantRegisterSize(type) < NUM_ROWS_CONST_TABLES*4);

        while (uCurrentVertexSlot < hMatrixParameter)
        {
            uCurrentVertexSlot += GetShaderConstantRegisterSize(m_rgVertexShaderParameters[uParamNum]);

            uParamNum++;

            Assert(uParamNum < m_rgVertexShaderParameters.GetCount());
        }

        Assert(m_rgVertexShaderParameters[uParamNum] == type);
    }
}
#endif




