// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      HLSL pixel shader register class
//
//-----------------------------------------------------------------------------
#include "precomp.h"

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderRegisters::SetConstant
//
//  Synopsis:
//     Set a user defined shader constant
//
//-------------------------------------------------------------------------
HRESULT 
CPixelShaderRegisters::SetConstant(
    __in INT32 nChannel,
    __in ConstDef_F constant
    )
{
    HRESULT hr = S_OK;
    C_f32x1 rValue(constant.f[nChannel]);

    if (constant.RegNum >= PIXELSHADER_CONSTS_MAX)
    {
        IFC(E_FAIL);
    }

    m_constants[constant.RegNum] = rValue.Replicate();

Cleanup:
    RRETURN(hr);
}
                 
//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderRegisters::GetRegister
//
//  Synopsis:
//     Get a JIT variable for a HLSL register
//
//-------------------------------------------------------------------------
HRESULT 
CPixelShaderRegisters::GetRegister(
    __in P_u8 *pPixelShaderState, 
    __in const PSTRRegister *pRegister, 
    __out C_f32x4 **ppRegisterOut
    )
{
    HRESULT hr = S_OK;

    if (pRegister->GetIsRelAddr())
    {
        AssertMsg(0, "Relative addresses not supported");
        IFC(E_FAIL);
    }

    if (pRegister->GetRegNum() >= PIXELSHADER_TEMPS_MAX)
    {
        AssertMsg(0, "Too many registers used in pixel shader");
        IFC(E_FAIL);
    }

    switch (pRegister->GetRegType())
    {
    case PSTRREG_CONST:
        *ppRegisterOut = &m_constants[pRegister->GetRegNum()];
        if (!(**ppRegisterOut).IsInitialized())
        {
            C_f32x4 &reg = m_constants[pRegister->GetRegNum()];

            // We don't have a set constants, read it out of the shader state register file

            reg.LoadUnaligned(
                (*pPixelShaderState + OFFSET_OF(CPixelShaderState, m_rgShaderConstants[pRegister->GetRegNum()][0])).AsP_f32x4()
                );

            reg = reg.AsInt32x4().GetElement(m_nIndex).Replicate().As_f32x4();
        }
        break;

    case PSTRREG_COLOROUT:
        if (pRegister->GetRegNum() >= PIXELSHADER_COLOROUT_MAX)
        {
            AssertMsg(0, "Too many color output registers used");
            IFC(E_FAIL);
        }
        *ppRegisterOut = &(m_colorOutput[pRegister->GetRegNum()]);
        break;

    case PSTRREG_TEMP:
        *ppRegisterOut = &m_temps[pRegister->GetRegNum()];
        break;

    case PSTRREG_SCRATCH:
        *ppRegisterOut = &m_scratch[pRegister->GetRegNum()];
        break;

    case PSTRREG_POSTMODSRC:
        *ppRegisterOut = &m_postModSrc[pRegister->GetRegNum()];
        break;

    case PSTRREG_TEXTURE:
        if (pRegister->GetRegNum() >= PIXELSHADER_SAMPLERS_MAX)
        {
            AssertMsg(0, "Too many samplers used");
            IFC(E_FAIL);
        }
        *ppRegisterOut = &m_textureSampler[pRegister->GetRegNum()];
        break;

    default:
        AssertMsg(0, "Unsupported register type");
        IFC(E_FAIL);
    }

Cleanup:
    RRETURN(hr);
}





