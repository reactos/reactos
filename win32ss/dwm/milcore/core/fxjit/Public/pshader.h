// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Pixel shader compiler
//

#pragma once

// Custom HRESULT included here for WPF/SL interop.  SL uses XRESULT elsewhere
// and WPF uses HRESULT.
typedef long PS_HRESULT;

struct CTextureVariables;
struct CInstructionVariables;
struct PSTRINST_BASE_PARAMS;
class CPixelShaderRegisters;
class RDPSTrans;
class P_u8;
class C_u32;
class C_f32x4;

//-------------------------------------------------------------------------
//
//  Class:   CPixelShaderCompiler
//
//  Synopsis:
//     Pixel shader compiler
//
//-------------------------------------------------------------------------
class CPixelShaderCompiler
{
public:
    static PS_HRESULT Create(
        __in void* pCode,
        __in unsigned uByteCodeSize,
        __out CPixelShaderCompiler **ppPixelShaderCompiler
        );

    unsigned AddRef();
    unsigned Release();

    GenerateColorsEffect *GetGenerateColorsFunction()
    {
        return m_pfn;
    }

private:
    CPixelShaderCompiler();
    ~CPixelShaderCompiler();

    PS_HRESULT Init(
        __in void *pCode,
        __in unsigned uByteCodeSize
        );

    PS_HRESULT Compile(
        __out GenerateColorsEffect **ppfn
        );

    PS_HRESULT LoadTextureVariables(__in P_u8 *pPixelShaderState);

    PS_HRESULT LoadShaderConstants(__in int nChannel, __inout CPixelShaderRegisters *pShaderRegisters);

    PS_HRESULT ComputeEval(
        __in  const P_u8 *pPixelShaderState,
        __in  const C_u32 *puX,
        __in  const C_u32 *puY,
        __out C_f32x4  *pEvalRight,
        __out C_f32x4  *pEvalDeltaRight,
        __out C_f32x4  *pEvalDown,
        __out C_f32x4  *pEvalDeltaDown
        );

    PS_HRESULT CompileInstruction(
        __in int i,                                         // channel
        __in PSTRINST_BASE_PARAMS* pBaseInstr,              // instruction
        __inout CInstructionVariables *pInstructionVars     // variables used by instruction compiler
        );

    PS_HRESULT CompileDependentInstruction(
        __in PSTRINST_BASE_PARAMS* pBaseInstr,              // instruction
        __inout CInstructionVariables *pInstructionVars     // variables used by instruction compiler
        );

    PS_HRESULT PreloadConstant(
        __in int i,                                         // channel
        __in PSTRINST_BASE_PARAMS* pBaseInstr,              // instruction
        __inout CInstructionVariables *pInstructionVars     // variables used by instruction compiler
        );

private:
    unsigned              m_cRefs;
    RDPSTrans            *m_pTranslated;
    CTextureVariables    *m_pTextureVariables;
    GenerateColorsEffect *m_pfn;
};




