// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Pixel shader compiler
//
//-----------------------------------------------------------------------------
#include "precomp.h"

//
// Macros
//

#define HEX(x) (((x) < 10) ? ((x)+'0') : ((x)-10+'a'))

#if DBG
#define JIT_TRACE(msg) WarpPlatform::TraceMessage(msg)
#else
#define JIT_TRACE(msg) 
#endif


//
// Texture JIT variables
//

struct CTextureVariables
{
    P_u32 m_pTextureSource;
    C_u32 m_uWidth;
    C_u32 m_uHeight;
    C_u32 m_useBilinear;
    C_f32x4 m_textureWidthHeight;
};

//
// Instruction compile variables
//

struct CInstructionVariables
{
    CPixelShaderRegisters m_shaderRegisters[4];
    P_u8                  m_pPixelShaderState; 
    C_f32x4               m_r255;
    C_f32x4               m_rZero;
    C_f32x4               m_rNegativeOne;
    C_f32x4               m_evalRight;
    C_f32x4               m_evalDown;
    C_f32x4               m_kill[4];
};

//-------------------------------------------------------------------------
//
//  Function:   OutputBreakpointTrace
//
//  Synopsis:
//     Output the address of the start of the function that can be 
//     used for setting a breakpoint
//
//-------------------------------------------------------------------------
#if DBG
void
OutputBreakpointTrace(const void *pCode)
{
    unsigned short szBP[100];
    UINT_PTR addr = (UINT_PTR)pCode;
    
    szBP[0] = '\n';
    szBP[1] = 'J';
    szBP[2] = 'I';
    szBP[3] = 'T';
    szBP[4] = ':';
    szBP[5] = ' ';
    szBP[6] = '0';
    szBP[7] = 'x';
    szBP[8] = HEX((addr >> 28) & 0xf);
    szBP[9] = HEX((addr >> 24) & 0xf);
    szBP[10] = HEX((addr >> 20) & 0xf);
    szBP[11] = HEX((addr >> 16) & 0xf);
    szBP[12] = HEX((addr >> 12) & 0xf);
    szBP[13] = HEX((addr >> 8) & 0xf);
    szBP[14] = HEX((addr >> 4) & 0xf);
    szBP[15] = HEX((addr >> 0) & 0xf);
    szBP[16] = '\n';
    szBP[17] = 0;

    JIT_TRACE(szBP);
}
#endif

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::Create
//
//  Synopsis:
//     Create a CPixelShaderCompiler
//
//-------------------------------------------------------------------------
HRESULT 
CPixelShaderCompiler::Create(
    __in void* pCode,
    __in unsigned uByteCodeSize,
    __out CPixelShaderCompiler **ppPixelShaderCompiler
    )
{
    HRESULT hr = S_OK;
    CPixelShaderCompiler *pPixelShaderCompiler = NULL;

    pPixelShaderCompiler = new CPixelShaderCompiler();
    IFCOOM(pPixelShaderCompiler);

    IFC(pPixelShaderCompiler->Init(pCode, uByteCodeSize));

    *ppPixelShaderCompiler = pPixelShaderCompiler;
    pPixelShaderCompiler = NULL;

Cleanup:
    delete pPixelShaderCompiler;

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::CPixelShaderCompiler
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
CPixelShaderCompiler::CPixelShaderCompiler()
{
    m_pTranslated = NULL;
    m_pTextureVariables = NULL;
    m_pfn = NULL;
    m_cRefs = 1;
}

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::~CPixelShaderCompiler
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
CPixelShaderCompiler::~CPixelShaderCompiler()
{
    delete m_pTranslated;
    delete [] m_pTextureVariables;

    if (m_pfn != NULL)
    {
        CJitterSupport::CodeFree(m_pfn);
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::Init
//
//  Synopsis:
//     Translate the shader to our risk instruction set for simpler compile
//     and faster recompiler after a constant change;
//
//-------------------------------------------------------------------------
HRESULT 
CPixelShaderCompiler::Init(
    __in void *pCode,
    __in unsigned uByteCodeSize
    )
{
    HRESULT hr = S_OK;

    m_pTranslated = new RDPSTrans((DWORD *)pCode, uByteCodeSize, 0);
    IFCOOM(m_pTranslated);

    IFC(m_pTranslated->GetStatus());

    IFC(Compile(&m_pfn));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:  CPixelShaderCompiler::Release
//
//  Synopsis:
//     Release implementation
//
//-------------------------------------------------------------------------
UINT32 
CPixelShaderCompiler::Release()
{
    UINT32 cRef = --m_cRefs;
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

//-------------------------------------------------------------------------
//
//  Function:  CPixelShaderCompiler::AddRef
//
//  Synopsis:
//     AddRef implementation
//
//-------------------------------------------------------------------------
UINT32 
CPixelShaderCompiler::AddRef()
{
    return ++m_cRefs;
}

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::LoadTextureVariables
//
//  Synopsis:
//     Load the variables needed to sample a texture from the
//     active samplers
//
//-------------------------------------------------------------------------
HRESULT 
CPixelShaderCompiler::LoadTextureVariables(__in P_u8 *pPixelShaderState)
{
    HRESULT hr = S_OK;

    delete [] m_pTextureVariables;
    m_pTextureVariables = NULL;

    if (m_pTranslated->GetActiveTextureStageCount() > 0)
    {
        m_pTextureVariables = new CTextureVariables[m_pTranslated->GetActiveTextureStageCount()];
        IFCOOM(m_pTextureVariables);

        for (UINT32 i = 0; i < m_pTranslated->GetActiveTextureStageCount(); i++)
        {
            if (m_pTranslated->GetSamplerRegDcl()[i] == D3DSTT_2D)
            {
                m_pTextureVariables[i].m_uWidth = pPixelShaderState->GetMemberUINT32(OFFSET_OF(CPixelShaderState, m_samplers[i].m_nWidth));
                m_pTextureVariables[i].m_uHeight = pPixelShaderState->GetMemberUINT32(OFFSET_OF(CPixelShaderState, m_samplers[i].m_nHeight));
                m_pTextureVariables[i].m_useBilinear = pPixelShaderState->GetMemberUINT32(OFFSET_OF(CPixelShaderState, m_samplers[i].m_nUseBilinear));
                m_pTextureVariables[i].m_pTextureSource = pPixelShaderState->GetMemberPtr(OFFSET_OF(CPixelShaderState, m_samplers[i].m_pargbSource));
            }
            else if (m_pTranslated->GetSamplerRegDcl()[i] != D3DSTT_UNKNOWN)
            {
                JIT_TRACE(L"Invalid texture type: only D3DSTT_2D supported");
                IFC(E_FAIL);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::ComputeEval
//
//  Synopsis:
//     Computes eval and eval update variables
//
//-------------------------------------------------------------------------
HRESULT 
CPixelShaderCompiler::ComputeEval(
    __in  const P_u8 *pPixelShaderState,
    __in  const C_u32 *puX,
    __in  const C_u32 *puY,
    __out C_f32x4  *pEvalRight,
    __out C_f32x4  *pEvalDeltaRight,
    __out C_f32x4  *pEvalDown,
    __out C_f32x4  *pEvalDeltaDown
    )
{
    HRESULT hr = S_OK;

    const f32x4 c_rPixelDeltas = {0.0f, 1.0f, 2.0f, 3.0f};
    C_f32x4 rPixelDeltas = c_rPixelDeltas;

    C_f32x4 rX = (*puX).Replicate().ToFloat4() + rPixelDeltas;
    C_f32x4 rY = (*puY).Replicate().ToFloat4();

    (*pEvalDeltaRight) = (*pPixelShaderState + OFFSET_OF(CPixelShaderState, m_rgDeltaUVDownRight[0])).GetMemberFloat1(0).Replicate();
    (*pEvalRight)      = (*pPixelShaderState + OFFSET_OF(CPixelShaderState, m_rgOffsetUV[0])).GetMemberFloat1(0).Replicate()
                         + rX * (*pEvalDeltaRight)
                         + rY * (*pPixelShaderState + OFFSET_OF(CPixelShaderState, m_rgDeltaUVDownRight[2])).GetMemberFloat1(0).Replicate();

    C_f32x1 rFour = 4.0f;
    (*pEvalDeltaRight) *= rFour.Replicate();

    (*pEvalDeltaDown) = (*pPixelShaderState + OFFSET_OF(CPixelShaderState, m_rgDeltaUVDownRight[1])).GetMemberFloat1(0).Replicate();
    (*pEvalDown)      = (*pPixelShaderState + OFFSET_OF(CPixelShaderState, m_rgOffsetUV[1])).GetMemberFloat1(0).Replicate()
                        + rX * (*pEvalDeltaDown)
                        + rY * (*pPixelShaderState + OFFSET_OF(CPixelShaderState, m_rgDeltaUVDownRight[3])).GetMemberFloat1(0).Replicate();

    (*pEvalDeltaDown) *= rFour.Replicate();


    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::LoadShaderConstants
//
//  Synopsis:
//     Loads the shader constants
//
//-------------------------------------------------------------------------
HRESULT
CPixelShaderCompiler::LoadShaderConstants(
    __in INT32 nChannel, 
    __inout CPixelShaderRegisters *pShaderRegisters
    )
{
    HRESULT hr = S_OK;

    for (UINT32 i = 0; i < m_pTranslated->GetNumConstDefsF(); i++)
    {
        pShaderRegisters->SetConstant(nChannel, m_pTranslated->GetConstDefsF()[i]);
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   SampleTexture
//
//  Synopsis:
//     Sample from a texture using specified sampling mode.
//
//-------------------------------------------------------------------------
HRESULT
SampleTexture(
    __in CInstructionVariables *pInstructionVars, // Instruction compile variables
    __in CTextureVariables *pTextureVars,         // Texture sampler info vars
    __in const PSTRRegister *pRegUV,              // texture coordinates to sample from
    __inout const PSTRRegister *pRegOutput,       // Desintation register
    __in BYTE writeMask,                          // Write mask
    __in bool useBilinear                         // Specifies bilinear or nearest neighbor
    )
{
    HRESULT hr = S_OK;
    C_f32x4 *pTexCoordinateU;
    C_f32x4 *pTexCoordinateV;
    C_f32x4 *pRegDest;
    P_u8 &pPixelShaderState = pInstructionVars->m_pPixelShaderState;
    const C_f32x4 &rZero = pInstructionVars->m_rZero;
    const C_f32x4 &r255 = pInstructionVars->m_r255;
    CPixelShaderRegisters *shaderRegisters = pInstructionVars->m_shaderRegisters;
    INT32 i;
    
    IFC(shaderRegisters[0].GetRegister(&pPixelShaderState, pRegUV, &pTexCoordinateU));
    IFC(shaderRegisters[1].GetRegister(&pPixelShaderState, pRegUV, &pTexCoordinateV));

    {
        // Load and convert texture coordinates to integer

        C_u32x4 uWidth = pTextureVars->m_uWidth.Replicate();
        C_u32x4 uHeight = pTextureVars->m_uHeight.Replicate();


        C_f32x4 rWidth = uWidth.ToFloat4();
        C_f32x4 rHeight = uHeight.ToFloat4();

        C_f32x4 rU = (*pTexCoordinateU) * rWidth;
        C_f32x4 rV = (*pTexCoordinateV) * rHeight;

        C_u32x4 uU;
        C_u32x4 uV;        

        // Clamp low side to zero
        rU = rU.Max(rZero);
        rV = rV.Max(rZero);

        // Used for bilinear only
        C_f32x4 rURatios;
        C_f32x4 rVRatios;
        C_f32x4 rUOpposites;
        C_f32x4 rVOpposites;        
        C_u32x4 uU1;
        C_u32x4 uV1;


        // Clamp low side to zero
        rU = rU.Max(rZero);
        rV = rV.Max(rZero);

        if (useBilinear) // compile time switch
        {
            //
            // Use bilinear sampling.  We set uU and uV to the first of our four sampling texels.
            // We will use (uU+1,uV), (uU,uV+1), and (uU+1,uV+1) as the others.
            uU = rU.IntFloor();
            uV = rV.IntFloor();

            u32x4 uOne = {1, 1, 1, 1};
            uU1 = uU + uOne;
            uV1 = uV + uOne;

            // Clamp high side to to width-1, height-1
            C_u32x4 uWidthBound = (pTextureVars->m_uWidth - 1).Replicate();
            C_u32x4 uHeightBound = (pTextureVars->m_uHeight - 1).Replicate();
            uU = uU.Min(uWidthBound);
            uV = uV.Min(uHeightBound);  
            uU1 = uU1.Min(uWidthBound);
            uV1 = uV1.Min(uHeightBound);
 

            C_f32x4 rUfloor = uU.ToFloat4();
            C_f32x4 rVfloor = uV.ToFloat4();
            
            // Calculate the weight of U texel in U direction, then same for V.
            // rURatio stores the weight for each of the four pixels we're sampling.
            rURatios = rU - rUfloor;
            rVRatios = rV - rVfloor;

            f32x4 fOne = {1.0f, 1.0f, 1.0f, 1.0f};
            C_f32x4 rOne = fOne;
            
            // Calculate the weight of U+1 texel in U direction, then same for V.
            rUOpposites = rOne - rURatios;
            rVOpposites = rOne - rVRatios;
        }
        else
        {            
            //
            // Use nearest neighbor sampling.  We clamp the float tex coords back to uints.        
            uU = rU.ToInt32x4();
            uV = rV.ToInt32x4();     
            
            // Clamp high side to to width-1, height-1        
            uU = uU.Min((pTextureVars->m_uWidth - 1).Replicate());
            uV = uV.Min((pTextureVars->m_uHeight - 1).Replicate());  
        }
            
        INT32 rgChannelOrder[4] = {2, 1, 0, 3};

        // We are taking samples for each of 4 pixels.
        for (int j = 0; j < 4; j++)
        {
            C_u32 uCoordinate = uU.GetElement(3-j);
            C_u32 vCoordinate = uV.GetElement(3-j);
                                                                      
            // Sample from memory - in argb format in a 32-bit integer

            // For nearest neighbor
            C_u32 uSample;
            // For bilinear
            C_f32x4 rSample;

            if (useBilinear) // compile time switch
            {
                //
                // Using bilinear, we generate a sample as the weighted sum of the four enclosing texels.
                
                C_u32 u1Coordinate = uU1.GetElement(3-j);
                C_u32 v1Coordinate = uV1.GetElement(3-j);

                C_u32 uWidth = pTextureVars->m_uWidth*4;
                C_u32 vOffset = vCoordinate*uWidth;
                C_u32 v1Offset = v1Coordinate*uWidth;

                // Store the sample 32-bit uint as a 4x32 integer vector: 0000 0000 0000 argb
                C_u32x4 uSampleUV = *((pTextureVars->m_pTextureSource.AsP_u8() + vOffset).AsP_u32() + uCoordinate);
                C_u32x4 uSampleU1V = *((pTextureVars->m_pTextureSource.AsP_u8() + vOffset).AsP_u32() + u1Coordinate);
                C_u32x4 uSampleUV1 = *((pTextureVars->m_pTextureSource.AsP_u8() + v1Offset).AsP_u32() + uCoordinate);
                C_u32x4 uSampleU1V1 = *((pTextureVars->m_pTextureSource.AsP_u8() + v1Offset).AsP_u32() + u1Coordinate);

                // Interleave to get 0000 0000 aarr ggbb
                uSampleUV = uSampleUV.AsC_u8x16().InterleaveLow(uSampleUV.AsC_u8x16());
                uSampleU1V = uSampleU1V.AsC_u8x16().InterleaveLow(uSampleU1V.AsC_u8x16());
                uSampleUV1 = uSampleUV1.AsC_u8x16().InterleaveLow(uSampleUV1.AsC_u8x16());
                uSampleU1V1 = uSampleU1V1.AsC_u8x16().InterleaveLow(uSampleU1V1.AsC_u8x16());

                // Interleave to get aaaa rrrr gggg bbbb
                uSampleUV = uSampleUV.AsC_u16x8().InterleaveLow(uSampleUV.AsC_u16x8());
                uSampleU1V = uSampleU1V.AsC_u16x8().InterleaveLow(uSampleU1V.AsC_u16x8());
                uSampleUV1 = uSampleUV1.AsC_u16x8().InterleaveLow(uSampleUV1.AsC_u16x8());
                uSampleU1V1 = uSampleU1V1.AsC_u16x8().InterleaveLow(uSampleU1V1.AsC_u16x8());
                
                // Shift right to get 000a 000r 000g 000b
                uSampleUV >>= 24;
                uSampleU1V >>= 24;
                uSampleUV1 >>= 24;
                uSampleU1V1 >>= 24;

                // Convert to 4x32 float vector.
                C_f32x4 rSampleUV = uSampleUV.ToFloat4();
                C_f32x4 rSampleU1V = uSampleU1V.ToFloat4();            
                C_f32x4 rSampleUV1 = uSampleUV1.ToFloat4();
                C_f32x4 rSampleU1V1 = uSampleU1V1.ToFloat4();
                
                // Get weights for this pixel.  We cast to int to select an element, but do not convert the floats.
                C_u32 uURatio = rURatios.AsInt32x4().GetElement(3-j);
                C_u32 uVRatio = rVRatios.AsInt32x4().GetElement(3-j);
                C_u32 uUOpposite = rUOpposites.AsInt32x4().GetElement(3-j);
                C_u32 uVOpposite = rVOpposites.AsInt32x4().GetElement(3-j);

                // Replicate to 4x32 float vector.  Again, we do not convert the ints to floats, just cast back.
                C_f32x4 rURatiox4 = uURatio.Replicate().As_f32x4();
                C_f32x4 rVRatiox4 = uVRatio.Replicate().As_f32x4();
                C_f32x4 rUOppositex4 = uUOpposite.Replicate().As_f32x4();
                C_f32x4 rVOppositex4 = uVOpposite.Replicate().As_f32x4();

                // Calculate the weighted color per channel as floats.
                rSample = rVOppositex4 * (rUOppositex4 * rSampleUV   +  rURatiox4 * rSampleU1V) +
                          rVRatiox4    * (rUOppositex4 * rSampleUV1  +  rURatiox4 * rSampleU1V1);
            }
            else
            {
                // Using nearest neighbor, we just sample at the tex coord we calculated earlier.
                uSample = *((pTextureVars->m_pTextureSource.AsP_u8() + vCoordinate*pTextureVars->m_uWidth * 4).AsP_u32() + uCoordinate);
            }
        
            INT32 rgChannelOrder[4] = {2, 1, 0, 3};

            // The inner loop samples each color channel for this pixel and packs it into the output register.
            // Output registers are packed transposed, so each holds 4 different pixels' values for the same color channel.
            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[rgChannelOrder[i]].GetRegister(&pPixelShaderState, pRegOutput, &pRegDest));

                C_f32x4 rSampleChannel;
                if (useBilinear)
                {
                    // For bilinear, we've already calculated all 4 channels in a 128-bit float.
                    C_u32x4 uSampleChannel = rSample.AsInt32x4().GetElement(i);
                    rSampleChannel = uSampleChannel.As_f32x4();
                }
                else
                {
                    // For nearest neighbor, we mask off one channel, replicate it to 128-bit and cast it to float.
                    C_u32x4 uSampleChannel = (uSample & 0xff);
                    rSampleChannel = uSampleChannel.ToFloat4();
                }

                if (j == 0)
                {
                    (*pRegDest) = rSampleChannel;
                }
                else
                {
                    (*pRegDest) = (*pRegDest).Shuffle((*pRegDest), ((2 << 6) | (1 << 4) | (0 << 2) | 3));
                    (*pRegDest) += rSampleChannel;
                }
        
                if (i != 3)
                {
                    if (!useBilinear)
                    {
                        uSample = uSample >> 8;
                    }
                }
            }
        }
        
        for (i = 0; i < 4; i++)
        {
            IFC(shaderRegisters[rgChannelOrder[i]].GetRegister(&pPixelShaderState, pRegOutput, &pRegDest));

            *pRegDest = (*pRegDest)/r255;
        }                
    }

Cleanup:
    return hr;
}

//-------------------------------------------------------------------------
//
//  Function:   ConditionalMultiply
//
//  Synopsis:
//     Multiply source (with a temporary) if fMultiply is true
//
//-------------------------------------------------------------------------
void
ConditionalMultiply(
    __inout C_f32x4 **ppRegSource,
    __in INT32 fMultiply,
    __inout C_f32x4 *pTempRegister,
    __in const C_f32x4 &multiplicand
    )
{
    if (fMultiply)
    {
        (*pTempRegister) = (**ppRegSource)*multiplicand;
        *ppRegSource = pTempRegister;
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::CompileInstruction
//
//  Synopsis:
//     Compile a pixel shader instruction
//
//-------------------------------------------------------------------------
HRESULT
CPixelShaderCompiler::CompileInstruction(
    __in INT32 i,                                         // channel
    __in PSTRINST_BASE_PARAMS* pBaseInstr,                // instruction
    __inout CInstructionVariables *pInstructionVariables // instruction variables
    )
{
    HRESULT hr = S_OK;
    PSTR_INSTRUCTION_OPCODE_TYPE opcode = pBaseInstr->Inst;

    P_u8                  &pPixelShaderState = pInstructionVariables->m_pPixelShaderState;
    const C_f32x4               &rZero             = pInstructionVariables->m_rZero;
    const C_f32x4               &rNegativeOne      = pInstructionVariables->m_rNegativeOne;
    CPixelShaderRegisters *shaderRegisters   = pInstructionVariables->m_shaderRegisters;

    // Instruction input and output registers

    C_f32x4 *pRegSrc0;
    C_f32x4 *pRegSrc1;
    C_f32x4 *pRegSrc2;
    C_f32x4 *pRegDest;

    // Negate temporaries

    C_f32x4 regSourceNegate0;
    C_f32x4 regSourceNegate1;
    C_f32x4 regSourceNegate2;

    switch (opcode)
    {
    case PSTRINST_TEXCOVERAGE:
    case PSTRINST_QUADLOOPBEGIN:
    case PSTRINST_QUADLOOPEND:
    case PSTRINST_NEXTD3DPSINST:
    case PSTRINST_END:
        // Nothing to do for these instructions
        break;

    case PSTRINST_EVAL:
        {
            const PSTRINST_EVAL_PARAMS* pEval = (PSTRINST_EVAL_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pEval->DstReg, &pRegDest));

            // Texture coordinates evaluations output the computed u,v coordinates.  

            // If the mask is containts both component 0 and 1, do a faster write than
            // a masked write

            JIT_TRACE(L"PSTRINST_EVAL");

            if (!IsMasked(i, pEval->WriteMask, NULL))
            {
                if (i == 0)
                {
                    (*pRegDest) = pInstructionVariables->m_evalRight;
                }
                else if (i == 1)
                {
                    (*pRegDest) = pInstructionVariables->m_evalDown;
                }
                else
                {
                    (*pRegDest) = rZero;
                }
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

    case PSTRINST_MUL:
        {
            const PSTRINST_MUL_PARAMS* pMul = (PSTRINST_MUL_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMul->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_MUL");

            if (!IsMasked(i, pMul->WriteMask, &pMul->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMul->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMul->SrcReg1, &pRegSrc1));

                ConditionalMultiply(&pRegSrc0, pMul->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);
                ConditionalMultiply(&pRegSrc1, pMul->bSrcReg1_Negate, &regSourceNegate1, rNegativeOne);

                (*pRegDest) = (*pRegSrc0) * (*pRegSrc1);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

    case PSTRINST_DSTMOD:
        {
            const PSTRINST_DSTMOD_PARAMS* pDstMod = (PSTRINST_DSTMOD_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDstMod->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_DSTMOD - tested with _sat modifier but scale functionality untested");

            if (!IsMasked(i, pDstMod->WriteMask, &pDstMod->Predication))
            {
                if ((*pRegDest).IsInitialized())
                {
                    C_f32x1 scalar = (C_f32x1)pDstMod->fScale;
                    C_f32x4 vector = scalar.Replicate();
                    (*pRegDest) *= vector;

                    scalar = (C_f32x1)pDstMod->fRangeMin;
                    vector = scalar.Replicate();
                    (*pRegDest) = (*pRegDest).Max(vector);

                    scalar = (C_f32x1)pDstMod->fRangeMax;
                    vector = scalar.Replicate();
                    (*pRegDest) = (*pRegDest).Min(vector);
                }
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

    case PSTRINST_MOV:
        {
            const PSTRINST_MOV_PARAMS* pMov = (PSTRINST_MOV_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMov->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_MOV");

            if (!IsMasked(i, pMov->WriteMask, &pMov->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMov->SrcReg0, &pRegSrc0));

                ConditionalMultiply(&pRegSrc0, pMov->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);

                (*pRegDest) = (*pRegSrc0);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;


    case PSTRINST_ADD:
        {
            const PSTRINST_ADD_PARAMS* pAdd = (PSTRINST_ADD_PARAMS*)pBaseInstr;
            
            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pAdd->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_ADD");

            if (!IsMasked(i, pAdd->WriteMask, &pAdd->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pAdd->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pAdd->SrcReg1, &pRegSrc1));

                ConditionalMultiply(&pRegSrc0, pAdd->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);
                ConditionalMultiply(&pRegSrc1, pAdd->bSrcReg1_Negate, &regSourceNegate1, rNegativeOne);

                (*pRegDest) = (*pRegSrc0) + (*pRegSrc1);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;


    case PSTRINST_MAD:
        {
            const PSTRINST_MAD_PARAMS* pMad = (PSTRINST_MAD_PARAMS*)pBaseInstr;
            
            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMad->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_MAD");

            if (!IsMasked(i, pMad->WriteMask, &pMad->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMad->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMad->SrcReg1, &pRegSrc1));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMad->SrcReg2, &pRegSrc2));

                ConditionalMultiply(&pRegSrc0, pMad->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);
                ConditionalMultiply(&pRegSrc1, pMad->bSrcReg1_Negate, &regSourceNegate1, rNegativeOne);
                ConditionalMultiply(&pRegSrc2, pMad->bSrcReg2_Negate, &regSourceNegate2, rNegativeOne);

                (*pRegDest) = (*pRegSrc0) * (*pRegSrc1) + (*pRegSrc2);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

    case PSTRINST_LRP:
        {
            const PSTRINST_LRP_PARAMS* pLrp = (PSTRINST_LRP_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pLrp->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_LRP - untested");

            if (!IsMasked(i, pLrp->WriteMask, &pLrp->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pLrp->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pLrp->SrcReg1, &pRegSrc1));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pLrp->SrcReg2, &pRegSrc2));

                ConditionalMultiply(&pRegSrc0, pLrp->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);
                ConditionalMultiply(&pRegSrc1, pLrp->bSrcReg1_Negate, &regSourceNegate1, rNegativeOne);
                ConditionalMultiply(&pRegSrc2, pLrp->bSrcReg2_Negate, &regSourceNegate2, rNegativeOne);

                (*pRegDest) = (*pRegSrc0)*((*pRegSrc1) - (*pRegSrc2)) + (*pRegSrc2);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

    case PSTRINST_FRC:
        {
            const PSTRINST_FRC_PARAMS* pFrc = (PSTRINST_FRC_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pFrc->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_FRC");

            if (!IsMasked(i, pFrc->WriteMask, &pFrc->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pFrc->SrcReg0, &pRegSrc0));

                ConditionalMultiply(&pRegSrc0, pFrc->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);

                //
                // The conversion to integer may overflow if the float is too large (either positively or
                // negatively). In this case, and actually for some large values before overflow occurs, 
                // the frc value will be 0 by definition because there is no precision after the decimal point. 
                //
                // To determine if this occurs, we select the exponent only by rotating and
                // masking, then we compare this to the sum of the bias specified for IEEE 32-bit floating
                // point (127) with the maximum exponent allowed (22). The maximum exponent is 22 because
                // there are 23 bits in the mantissa, meaning that the exponent can be up to 22 before
                // the mantissa no longer contains any information beyond the decimal point of the expanded
                // value.
                //
                // 32-bit floating point format (31 is MSB, 0 is LSB):
                // | 31   | 30              23 | 22                  0 |
                // | Sign | Biased Exponent    | Mantissa              |
                //
                u32x4 exponentMask = {0xff, 0xff, 0xff, 0xff};
                s32x4 biasPlusMaxExponent = {149, 149, 149, 149};
                C_u32x4 exponentWithBias = ((((*pRegSrc0).AsInt32x4()) >> 23) & exponentMask);
                C_u32x4 overflow = exponentWithBias.AsC_s32x4() > biasPlusMaxExponent;

                C_f32x4 floorValue = (*pRegSrc0).IntFloor().ToFloat4();
            
                C_f32x4 frcValue = (*pRegSrc0) - floorValue;

                //
                // FRC(x) must always be in [0, 1)
                // so if the result turns out to be one (because of round to nearest even mode)
                // then return 1-EPS (the largest float less than 1)
                //
                UINT oneMinusEPS = 0x3F7Fffff;
                f32x4 rOneMinusEPS = {*reinterpret_cast<float*>(&oneMinusEPS),
                                      *reinterpret_cast<float*>(&oneMinusEPS),
                                      *reinterpret_cast<float*>(&oneMinusEPS),
                                      *reinterpret_cast<float*>(&oneMinusEPS)};
                f32x4 rOne = {1.0f, 1.0f, 1.0f, 1.0f};

                frcValue = frcValue.Blend(rOneMinusEPS, frcValue == rOne);

                //
                // Select 0s for cases where floating point number is too large to have any 
                // fractional precision
                //
                (*pRegDest) = frcValue.Blend(rZero, overflow);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

    case PSTRINST_DEPTH:
        {
            JIT_TRACE(L"PSTRINST_DEPTH - not supported");
            IFC(E_INVALIDARG);
        }
        break;

    case PSTRINST_KILL:
        {
            const PSTRINST_KILL_PARAMS* pKill = (PSTRINST_KILL_PARAMS*)pBaseInstr;

            JIT_TRACE(L"PSTRINST_KILL");

            if (!IsMasked(i, pKill->WriteMask, &pKill->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pKill->SrcReg0, &pRegSrc0));

                if (pKill->bKillLZ[i])
                {
                    pInstructionVariables->m_kill[i] |= (*pRegSrc0) < rZero;
                }
                else
                {
                    pInstructionVariables->m_kill[i] |= (*pRegSrc0) >= rZero;
                }
            }
        }
        break;

    case PSTRINST_CMP:
        {
            const PSTRINST_CMP_PARAMS* pCmp = (PSTRINST_CMP_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCmp->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_CMP");

            if (!IsMasked(i, pCmp->WriteMask, &pCmp->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCmp->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCmp->SrcReg1, &pRegSrc1));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCmp->SrcReg2, &pRegSrc2));

                ConditionalMultiply(&pRegSrc0, pCmp->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);
                ConditionalMultiply(&pRegSrc1, pCmp->bSrcReg1_Negate, &regSourceNegate1, rNegativeOne);
                ConditionalMultiply(&pRegSrc2, pCmp->bSrcReg2_Negate, &regSourceNegate2, rNegativeOne);

                C_f32x4 comparison = (*pRegSrc0) >= rZero;

                (*pRegDest) = (*pRegSrc2).Blend(*pRegSrc1, comparison);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

     case PSTRINST_CND:
        {
            const PSTRINST_CND_PARAMS* pCnd = (PSTRINST_CND_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCnd->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_CND - untested");

            if (!IsMasked(i, pCnd->WriteMask, &pCnd->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCnd->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCnd->SrcReg1, &pRegSrc1));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCnd->SrcReg2, &pRegSrc2));

                ConditionalMultiply(&pRegSrc0, pCnd->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);
                ConditionalMultiply(&pRegSrc1, pCnd->bSrcReg1_Negate, &regSourceNegate1, rNegativeOne);
                ConditionalMultiply(&pRegSrc2, pCnd->bSrcReg2_Negate, &regSourceNegate2, rNegativeOne);

                C_f32x1 rHalf = 0.5f;
                C_f32x4 rHalfVector = rHalf.Replicate();
                C_f32x4 comparison = (*pRegSrc0) > rHalfVector;

                (*pRegDest) = (*pRegSrc2).Blend(*pRegSrc1, comparison);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

    case PSTRINST_LEGACYRCP:
        {
            JIT_TRACE(L"PSTRINST_LEGACYRCP - not supported (no proj texturing)");
            IFC(E_INVALIDARG);
        }
        break;

    case PSTRINST_BEM:
        {
            // Bump environment matrix

            JIT_TRACE(L"PSTRINST_BEM - not supported");
            IFC(E_INVALIDARG);
        }
        break;

    case PSTRINST_MAX:
        {
            const PSTRINST_MAX_PARAMS* pMax = (PSTRINST_MAX_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMax->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_MAX");

            if (!IsMasked(i, pMax->WriteMask, &pMax->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMax->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMax->SrcReg1, &pRegSrc1));

                ConditionalMultiply(&pRegSrc0, pMax->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);
                ConditionalMultiply(&pRegSrc1, pMax->bSrcReg1_Negate, &regSourceNegate1, rNegativeOne);

                (*pRegDest) = (*pRegSrc0).Max(*pRegSrc1);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

    case PSTRINST_MIN:
        {
            const PSTRINST_MIN_PARAMS* pMin = (PSTRINST_MIN_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMin->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_MIN");

            if (!IsMasked(i, pMin->WriteMask, &pMin->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMin->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMin->SrcReg1, &pRegSrc1));

                ConditionalMultiply(&pRegSrc0, pMin->bSrcReg0_Negate, &regSourceNegate0, rNegativeOne);
                ConditionalMultiply(&pRegSrc1, pMin->bSrcReg1_Negate, &regSourceNegate1, rNegativeOne);

                (*pRegDest) = (*pRegSrc0).Min(*pRegSrc1);
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }
        }
        break;

    case PSTRINST_ABS:
        {
            const PSTRINST_ABS_PARAMS* pABS = (PSTRINST_ABS_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pABS->DstReg, &pRegDest));

            JIT_TRACE(L"PSTRINST_ABS");

            if (!IsMasked(i, pABS->WriteMask, &pABS->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pABS->SrcReg0, &pRegSrc0));

                (*pRegDest) = (*pRegSrc0).Fabs();
            }
            else if (!pRegDest->IsInitialized())
            {
                (*pRegDest) = rZero;
            }

        }
        break;


    case PSTRINST_SETPRED:
        {
            JIT_TRACE(L"PSTRINST_SETPRED - not supported (ps3.0 feature)");
            IFC(E_INVALIDARG);
        }
        break;

    case PSTRINST_DSX:
        {
            const PSTRINST_DSX_PARAMS* pDSX = (PSTRINST_DSX_PARAMS*)pBaseInstr;

            // We do texture sampling a different way

            if (PSTRREG_XGRADIENT != pDSX->DstReg.GetRegType())
            {
                JIT_TRACE(L"PSTRINST_DSX - not supported (ps3.0 feature)");
                IFC(E_INVALIDARG);
            }
        }
        break;

    case PSTRINST_DSY:
        {
            const PSTRINST_DSY_PARAMS* pDSY = (PSTRINST_DSY_PARAMS*)pBaseInstr;

            // We do texture sampling a different way

            if (PSTRREG_YGRADIENT != pDSY->DstReg.GetRegType())
            {
                JIT_TRACE(L"PSTRINST_DSY - not supported (ps3.0 feature)");
                IFC(E_INVALIDARG);
            }
        }
        break;

    case PSTRINST_SRCMOD:
        {
            JIT_TRACE(L"PSTRINST_SRCMOD - 1.x instructions not supported");
            IFC(E_INVALIDARG);
        }
        break;

    case PSTRINST_LUMINANCE:
        {
            JIT_TRACE(L"PSTRINST_LUMINANCE - 1.x instructions not supported");
            IFC(E_INVALIDARG);
        }
        break;

    default:
        JIT_TRACE(L"PSTRINST_?? - unknown instruction");
        IFC(E_INVALIDARG);
    }

Cleanup:
    return hr;
}

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::PreloadConstant
//
//  Synopsis:
//     Calling GetRegister will cause constant input values to be pre-loaded.
//
//     We have a separate pass so that constants are loaded outside the pixel loop.
//
//-------------------------------------------------------------------------
HRESULT
CPixelShaderCompiler::PreloadConstant(
    __in INT32 i,                                         // channel
    __in PSTRINST_BASE_PARAMS* pBaseInstr,                // instruction
    __inout CInstructionVariables *pInstructionVariables // instruction variables
    )
{
    HRESULT hr = S_OK;
    PSTR_INSTRUCTION_OPCODE_TYPE opcode = pBaseInstr->Inst;
    CPixelShaderRegisters *shaderRegisters = pInstructionVariables->m_shaderRegisters;
    P_u8                  &pPixelShaderState = pInstructionVariables->m_pPixelShaderState;

    C_f32x4 *pRegSrc0;
    C_f32x4 *pRegSrc1;
    C_f32x4 *pRegSrc2;

    switch (opcode)
    {
    case PSTRINST_MUL:
        {
            const PSTRINST_MUL_PARAMS* pMul = (PSTRINST_MUL_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pMul->WriteMask, &pMul->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMul->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMul->SrcReg1, &pRegSrc1));
            }
        }
        break;

    case PSTRINST_MOV:
        {
            const PSTRINST_MOV_PARAMS* pMov = (PSTRINST_MOV_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pMov->WriteMask, &pMov->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMov->SrcReg0, &pRegSrc0));
            }
        }
        break;


    case PSTRINST_ADD:
        {
            const PSTRINST_ADD_PARAMS* pAdd = (PSTRINST_ADD_PARAMS*)pBaseInstr;
            
            if (!IsMasked(i, pAdd->WriteMask, &pAdd->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pAdd->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pAdd->SrcReg1, &pRegSrc1));
            }
        }
        break;


    case PSTRINST_MAD:
        {
            const PSTRINST_MAD_PARAMS* pMad = (PSTRINST_MAD_PARAMS*)pBaseInstr;
            
            if (!IsMasked(i, pMad->WriteMask, &pMad->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMad->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMad->SrcReg1, &pRegSrc1));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMad->SrcReg2, &pRegSrc2));
            }
        }
        break;

    case PSTRINST_LRP:
        {
            const PSTRINST_LRP_PARAMS* pLrp = (PSTRINST_LRP_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pLrp->WriteMask, &pLrp->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pLrp->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pLrp->SrcReg1, &pRegSrc1));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pLrp->SrcReg2, &pRegSrc2));
            }
        }
        break;

    case PSTRINST_FRC:
        {
            const PSTRINST_FRC_PARAMS* pFrc = (PSTRINST_FRC_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pFrc->WriteMask, &pFrc->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pFrc->SrcReg0, &pRegSrc0));
            }
        }
        break;

    case PSTRINST_RSQ:
        {
            const PSTRINST_RSQ_PARAMS* pRSQ = (PSTRINST_RSQ_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[pRSQ->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pRSQ->SrcReg0, &pRegSrc0));
        }
        break;

    case PSTRINST_KILL:
        {
            const PSTRINST_KILL_PARAMS* pKill = (PSTRINST_KILL_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pKill->WriteMask, &pKill->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pKill->SrcReg0, &pRegSrc0));
            }
        }
        break;

    case PSTRINST_CMP:
        {
            const PSTRINST_CMP_PARAMS* pCmp = (PSTRINST_CMP_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pCmp->WriteMask, &pCmp->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCmp->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCmp->SrcReg1, &pRegSrc1));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCmp->SrcReg2, &pRegSrc2));
            }
        }
        break;

     case PSTRINST_CND:
        {
            const PSTRINST_CND_PARAMS* pCnd = (PSTRINST_CND_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pCnd->WriteMask, &pCnd->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCnd->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCnd->SrcReg1, &pRegSrc1));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCnd->SrcReg2, &pRegSrc2));
            }
        }
        break;

    case PSTRINST_MAX:
        {
            const PSTRINST_MAX_PARAMS* pMax = (PSTRINST_MAX_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pMax->WriteMask, &pMax->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMax->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMax->SrcReg1, &pRegSrc1));
            }
        }
        break;

    case PSTRINST_MIN:
        {
            const PSTRINST_MIN_PARAMS* pMin = (PSTRINST_MIN_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pMin->WriteMask, &pMin->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMin->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pMin->SrcReg1, &pRegSrc1));
            }
        }
        break;

    case PSTRINST_ABS:
        {
            const PSTRINST_ABS_PARAMS* pABS = (PSTRINST_ABS_PARAMS*)pBaseInstr;

            if (!IsMasked(i, pABS->WriteMask, &pABS->Predication))
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pABS->SrcReg0, &pRegSrc0));
            }
        }
        break;

    case PSTRINST_SWIZZLE:
        {
            const PSTRINST_SWIZZLE_PARAMS* pSwizzle = (PSTRINST_SWIZZLE_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pSwizzle->SrcReg0, &pRegSrc0));
        }
        break;

    case PSTRINST_RCP:
        {
            const PSTRINST_RCP_PARAMS* pRcp = (PSTRINST_RCP_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[pRcp->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pRcp->SrcReg0, &pRegSrc0));
        }
        break;

    case PSTRINST_COS:
        {
            const PSTRINST_COS_PARAMS* pCos = (PSTRINST_COS_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[pCos->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pCos->SrcReg0, &pRegSrc0));
        }
        break;

    case PSTRINST_SIN:
        {
            const PSTRINST_SIN_PARAMS* pSin = (PSTRINST_SIN_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[pSin->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pSin->SrcReg0, &pRegSrc0));
        }
        break;

    case PSTRINST_LOG:
        {
            const PSTRINST_LOG_PARAMS* pLog = (PSTRINST_LOG_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[pLog->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pLog->SrcReg0, &pRegSrc0));
        }
        break;

    case PSTRINST_EXP:
        {
            const PSTRINST_EXP_PARAMS* pExp = (PSTRINST_EXP_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[pExp->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pExp->SrcReg0, &pRegSrc0));
        }
        break;
      
    case PSTRINST_DP2ADD:
        {
            const PSTRINST_DP2ADD_PARAMS* pDP2Add = (PSTRINST_DP2ADD_PARAMS*)pBaseInstr;

            if (i < 2)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP2Add->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP2Add->SrcReg1, &pRegSrc1));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP2Add->SrcReg2, &pRegSrc2));
            }
       }
       break;

    case PSTRINST_DP3:
        {
            const PSTRINST_DP3_PARAMS* pDP3 = (PSTRINST_DP3_PARAMS*)pBaseInstr;

            if (i < 3)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP3->SrcReg0, &pRegSrc0));
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP3->SrcReg0, &pRegSrc1));
            }
        }
        break;

    case PSTRINST_DP4:
        {
            const PSTRINST_DP4_PARAMS* pDP4 = (PSTRINST_DP4_PARAMS*)pBaseInstr;

            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP4->SrcReg0, &pRegSrc0));
            IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP4->SrcReg0, &pRegSrc1));
        }
        break;
    }

Cleanup:
    return hr;
}


//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::CompileDependentInstruction
//
//  Synopsis:
//     Compile a pixel shader instruction
//
//-------------------------------------------------------------------------
HRESULT
CPixelShaderCompiler::CompileDependentInstruction(
    __in PSTRINST_BASE_PARAMS* pBaseInstr,               // instruction
    __inout CInstructionVariables *pInstructionVariables // instruction variables
    )
{
    HRESULT hr = S_OK;
    PSTR_INSTRUCTION_OPCODE_TYPE opcode = pBaseInstr->Inst;

    P_u8                        &pPixelShaderState = pInstructionVariables->m_pPixelShaderState;
    const C_f32x4               &rZero             = pInstructionVariables->m_rZero;
    const C_f32x4               &rNegativeOne      = pInstructionVariables->m_rNegativeOne;
    CPixelShaderRegisters       *shaderRegisters   = pInstructionVariables->m_shaderRegisters;

    // Source/destination variables

    C_f32x4 *pRegSrc0;
    C_f32x4 *pRegSrc2;
    C_f32x4 *pRegSrc00;
    C_f32x4 *pRegSrc01;
    C_f32x4 *pRegSrc02;
    C_f32x4 *pRegSrc03;
    C_f32x4 *pRegSrc10;
    C_f32x4 *pRegSrc11;
    C_f32x4 *pRegSrc12;
    C_f32x4 *pRegSrc13;
    C_f32x4 *pRegDest;

    // Negate temporaries

    C_f32x4 regSourceNegate00;
    C_f32x4 regSourceNegate01;
    C_f32x4 regSourceNegate02;
    C_f32x4 regSourceNegate03;
    C_f32x4 regSourceNegate10;
    C_f32x4 regSourceNegate11;
    C_f32x4 regSourceNegate12;
    C_f32x4 regSourceNegate13;
    C_f32x4 regSourceNegate2;

    INT32 i;

    switch (opcode)
    {
    case PSTRINST_SAMPLE:
        {
            PSTRINST_SAMPLE_PARAMS* pSample = (PSTRINST_SAMPLE_PARAMS*)pBaseInstr;
            CTextureVariables *pVars;

            // Validate sampler register

            if (pSample->uiStage >= m_pTranslated->GetActiveTextureStageCount()
                || !m_pTextureVariables[pSample->uiStage].m_pTextureSource.IsInitialized())
            {
                JIT_TRACE(L"Invalid sample index");
                IFC(E_FAIL);
            }

            pVars = &m_pTextureVariables[pSample->uiStage];

            // Check for noop write

            if (!IsPredicateFalse(pSample->Predication) && (pSample->WriteMask & PSTR_COMPONENTMASK_ALL) != 0)
            {
                JIT_TRACE(L"PSTRINST_SAMPLE");

                // We clear the output registers to zero. 
                // This is technically unnecessary since we're going to overwrite them, but
                // since we have two branches below, the JIT compiler will complain that the
                // registers may not be initialized by failing at run-time.
                //
                // For now we work around the case where the destination and coordinate registers
                // are the same for checking for that condition explicitly
                //

                if (!((pSample->DstReg.GetRegType() == pSample->CoordReg.GetRegType())
                    && (pSample->DstReg.GetRegNum() == pSample->CoordReg.GetRegNum())))
                {
                    const PSTRRegister *pRegOutput = &pSample->DstReg;
                    const C_f32x4 &rZero = pInstructionVariables->m_rZero;
                    C_f32x4 *pRegDest;
                    P_u8 &pPixelShaderState = pInstructionVariables->m_pPixelShaderState;
                    
                    INT32 rgChannelOrder[4] = {2, 1, 0, 3};

                    for (int i = 0; i < 4; i+=1)
                    {                    
                        IFC(shaderRegisters[rgChannelOrder[i]].GetRegister(&pPixelShaderState, pRegOutput, &pRegDest));
                        *pRegDest = rZero;
                    }
                }

                C_u32 uUseBilinear = pVars->m_useBilinear;
                C_u32 uUseNearestNeighbor = uUseBilinear^1;
                
                C_Branch bilinearBranch;
                bilinearBranch.BranchOnZero(uUseBilinear);
                {
                    IFC(SampleTexture(pInstructionVariables, pVars, &pSample->CoordReg, &pSample->DstReg, pSample->WriteMask, true /* bilinear */));
                }
                bilinearBranch.BranchHere();
                
                C_Branch nearestNeighborBranch;
                nearestNeighborBranch.BranchOnZero(uUseNearestNeighbor);
                {
                    IFC(SampleTexture(pInstructionVariables, pVars, &pSample->CoordReg, &pSample->DstReg, pSample->WriteMask, false /* nearest neighbor */));
                }
                nearestNeighborBranch.BranchHere();        
                
            }
        }
        break;


    case PSTRINST_SWIZZLE:
        {
            const PSTRINST_SWIZZLE_PARAMS* pSwizzle = (PSTRINST_SWIZZLE_PARAMS*)pBaseInstr;

            C_f32x4 temps[4];
            INT32 fUseTemps = FALSE;

            if (pSwizzle->SrcReg0.GetRegNum() == pSwizzle->DstReg.GetRegNum()
                && pSwizzle->SrcReg0.GetRegType() == pSwizzle->DstReg.GetRegType())
            {
                JIT_TRACE(L"PSTRINST_SWIZZLE_INPLACE");

                for (i = 0; i < 4; i++)
                {
                    IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pSwizzle->SrcReg0, &pRegSrc0));
                    temps[i] = (*pRegSrc0);
                }
                fUseTemps = TRUE;

            }
            else
            {
                JIT_TRACE(L"PSTRINST_SWIZZLE");
            }

             // 2 bits each
            UINT selectors[4];

            selectors[0] = pSwizzle->Swizzle & 0x3;
            selectors[1] = (pSwizzle->Swizzle >> 2) & 0x3;
            selectors[2] = (pSwizzle->Swizzle >> 4) & 0x3;
            selectors[3] = (pSwizzle->Swizzle >> 6) & 0x3;

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pSwizzle->DstReg, &pRegDest));

                if (!IsMasked(i, pSwizzle->WriteMask, &pSwizzle->Predication))
                {
                    if (fUseTemps)
                    {
                        (*pRegDest) = temps[selectors[i]];
                    }
                    else
                    {
                        IFC(shaderRegisters[selectors[i]].GetRegister(&pPixelShaderState, &pSwizzle->SrcReg0, &pRegSrc0));
                        (*pRegDest) = (*pRegSrc0);
                    }
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }
            }
        }
        break;

    case PSTRINST_RCP:
        {
            const PSTRINST_RCP_PARAMS* pRcp = (PSTRINST_RCP_PARAMS*)pBaseInstr;
            JIT_TRACE(L"PSTRINST_RCP");

            IFC(shaderRegisters[pRcp->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pRcp->SrcReg0, &pRegSrc0));

            ConditionalMultiply(&pRegSrc0, pRcp->bSrcReg0_Negate, &regSourceNegate00, rNegativeOne);

            C_f32x4 regRcp = (*pRegSrc0).Reciprocal();

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pRcp->DstReg, &pRegDest));

                if (!IsMasked(i, pRcp->WriteMask, &pRcp->Predication))
                {
                    (*pRegDest) = regRcp;
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }
            }

        }
        break;

    case PSTRINST_RSQ:
        {
            const PSTRINST_RSQ_PARAMS* pRSQ = (PSTRINST_RSQ_PARAMS*)pBaseInstr;
            JIT_TRACE(L"PSTRINST_RSQ");

            IFC(shaderRegisters[pRSQ->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pRSQ->SrcReg0, &pRegSrc0));

            C_f32x4 regRsq = (*pRegSrc0).Fabs().Rsqrt();

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pRSQ->DstReg, &pRegDest));

                if (!IsMasked(i, pRSQ->WriteMask, &pRSQ->Predication))
                {
                    (*pRegDest) = regRsq;
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }                
            }
        }
        break;

    case PSTRINST_COS:
        {
            const PSTRINST_COS_PARAMS *pCos = (PSTRINST_COS_PARAMS *)pBaseInstr;
            JIT_TRACE(L"PSTRINST_COS");

            IFC(shaderRegisters[pCos->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pCos->SrcReg0, &pRegSrc0));

            ConditionalMultiply(&pRegSrc0, pCos->bSrcReg0_Negate, &regSourceNegate00, rNegativeOne);

            C_f32x1 rOne(1.0f);
            C_f32x4 regCos = rOne.Replicate();

            C_f32x4 regSourceSquared = (*pRegSrc0)*(*pRegSrc0);
            C_f32x1 rNegTwoFactRecip(-0.5);
            regCos += rNegTwoFactRecip.Replicate() * regSourceSquared;

            C_f32x1 rFourFactRecip(1.0f/24.0f);
            C_f32x4 regTerm = regSourceSquared * regSourceSquared;
            regCos += rFourFactRecip.Replicate() * regTerm;

            C_f32x1 rNegSixFactRecip(-1.0f/720.0f);
            regTerm = regTerm * regSourceSquared;
            regCos += rNegSixFactRecip.Replicate() * regTerm;

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pCos->DstReg, &pRegDest));

                if (!IsMasked(i, pCos->WriteMask, &pCos->Predication))
                {
                    (*pRegDest) = regCos;
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }
            }
        }
        break;

    case PSTRINST_SIN:
        {
            const PSTRINST_SIN_PARAMS *pSin = (PSTRINST_SIN_PARAMS *)pBaseInstr;
            JIT_TRACE(L"PSTRINST_SIN");

            IFC(shaderRegisters[pSin->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pSin->SrcReg0, &pRegSrc0));

            ConditionalMultiply(&pRegSrc0, pSin->bSrcReg0_Negate, &regSourceNegate00, rNegativeOne);

            C_f32x4 regTerm = (*pRegSrc0);
            C_f32x4 regSin = regTerm;

            C_f32x1 rNegThreeFactRecip(-1.0f/6.0f);
            C_f32x4 regSourceSquared = (*pRegSrc0)*(*pRegSrc0);
            regTerm *= regSourceSquared;
            regSin += rNegThreeFactRecip.Replicate() * regTerm;

            C_f32x1 rFiveFactRecip(1.0f/120.0f);
            regTerm *= regSourceSquared;
            regSin += rFiveFactRecip.Replicate() * regTerm;

            C_f32x1 rNegSevenFactRecip(1.0f/5040.0f);
            regTerm *= regSourceSquared;
            regSin += rNegSevenFactRecip.Replicate() * regTerm;

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pSin->DstReg, &pRegDest));

                if (!IsMasked(i, pSin->WriteMask, &pSin->Predication))
                {
                    (*pRegDest) = regSin;
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }
            }
        }
        break;

    case PSTRINST_LOG:
        {
            const PSTRINST_LOG_PARAMS* pLog = (PSTRINST_LOG_PARAMS*)pBaseInstr;

            JIT_TRACE(L"PSTRINST_LOG");

            IFC(shaderRegisters[pLog->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pLog->SrcReg0, &pRegSrc0));

            /*
              Explaination
              A floating point number X is represented as:

              X = (2^E) * (1 + f)

              So Log(x) = E + Log(1 + F)

              E is computed by simply extracting the approriate bits from the exponent portion of the number (all integer operations)

              There is a lookup table of size 4 (2 bit index) that contains Log(1.0), Log(1.25), Log(1.5) and Log(1.75)
              The lookup table is used to approximate a portion of Log(1+F).  An approimating polynomial is used to compute the remainder

              Call the floating point value that is used as input to the look up table: "Ts".  It is one of {1.0, 1.25, 1.5, 1.75}
              Call the resulting logarithm read from the table: "Tr"

              So (1 + F) is broken down in to 2 parts, one is used with the LUT, another is used with the approximating polynomial

              The breakdown is: (1 + F) = (1 + A)(Ts)

              Thus Log(1 + F) = Log(1 + A) + Log(Ts)

              Log(1 + A) is computed with the polynomial
              Log(Ts) comes from the table

              The polynomial comes from the following deduction:
                Log(1 + A) = Log((1 + F)/Ts) (simple algebra)

                (definition of Log)
                Log(Y) = for(i = 0; i < NumTerms; i++)
                         {
                             Result += [ [2 / (2i + 1)] / LN(2) ] * [((Y - 1) / (Y + 1)) ^ (2i + 1)]
                         }

                Put both of these statements together and you get:

                Log(1 + A) = for(i = 0; i < NumTerms; i++)
                             {
                                 Result += [ [2 / (2i + 1)] / LN(2) ] * [((1 + A - Ts) / (1 + A + Ts)) ^ (2i + 1)]
                             }

                In this approximation, 3 terms are used (NumTerms = 3)
            */

            // Take absolute value and ignore source negate operator.
            C_f32x4 rAbsSource = (*pRegSrc0).Fabs();

            C_u32 uBias = 127;
            C_f32x4 rTerm1 = ((rAbsSource.AsInt32x4() >> 23) - uBias.Replicate()).ToFloat4();

            //
            // Term2 = look up most significant 2 bits of mantissa in table
            //

            C_u32   uMask2Bits = 3;
            C_u32x4 uTableIndex = ((rAbsSource.AsInt32x4() >> (23 - 2)) & uMask2Bits.Replicate());

            C_f32x4 rTerm2 = rZero;

            const float rgTable[4] =
            {
                0.0f,         // log2(1)
                0.321928095f, // log2(1.25)
                0.584962501f, // log2(1.5)
                0.807354922f, // log2(1.75)
            };

            for (i = 0; i < 4; i++)
            {
                C_u32   uIndex = i;
                C_u32x4 foundValue = (uTableIndex == uIndex.Replicate());
                C_f32x1 rValue = rgTable[i];

                rTerm2 += (foundValue & rValue.Replicate().AsInt32x4()).As_f32x4();
            }

            //
            // Term3 = Approximate remaining portion with polynomial
            //
            C_u32 uMask = (1 << 23) - 1; // all mantissa bits;
            C_u32 uBiasedExponent = 127 << 23; // all mantissa bits;
            C_f32x4 rRemainder =  ((rAbsSource.AsInt32x4() & uMask.Replicate()) | uBiasedExponent.Replicate()).As_f32x4();
            C_f32x4 rTableSrc = ((uTableIndex << (23 - 2)) | uBiasedExponent.Replicate()).As_f32x4();

            C_f32x4 rX1 = (rRemainder - rTableSrc) / (rRemainder + rTableSrc);
            C_f32x4 rX2 = rX1*rX1;
            C_f32x4 rX3 = rX1*rX2;
            C_f32x4 rX5 = rX3*rX2;

            C_f32x1 rCoeff1 = 2.885390081777930f; // 2 / LN2
            C_f32x1 rCoeff2 = 0.961796693925976f; // 2 / (3 LN 2)
            C_f32x1 rCoeff3 = 0.577078016355585f; // 2 / (5 LN 2)

            C_f32x4 rTerm3 = (rX1 * rCoeff1.Replicate()) + (rX3 * rCoeff2.Replicate()) + (rX5 * rCoeff3.Replicate());

            C_f32x4 log = rTerm1 + rTerm2 + rTerm3;

            //
            // Ensure 0 produces -INF result
            //
            f32x4 rNegINF = {0xFF800000, 0xFF800000, 0xFF800000, 0xFF800000};
            log.Blend(rNegINF, (*pRegSrc0) == rZero);

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pLog->DstReg, &pRegDest));

                if (!IsMasked(i, pLog->WriteMask, &pLog->Predication))
                {
                    (*pRegDest) = log;
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }
            }

        }
        break;

    case PSTRINST_EXP:
        {
            const PSTRINST_EXP_PARAMS* pExp = (PSTRINST_EXP_PARAMS*)pBaseInstr;

            JIT_TRACE(L"PSTRINST_EXP");

            IFC(shaderRegisters[pExp->SrcReg0_Selector & 3].GetRegister(&pPixelShaderState, &pExp->SrcReg0, &pRegSrc0));

            ConditionalMultiply(&pRegSrc0, pExp->bSrcReg0_Negate, &regSourceNegate00, rNegativeOne);

            /*
                Explanation

                2^X = 2^(A + B + C) = 2^A * 2^B * 2^C
                
                D = FloatToNDot3(X)
                A = D >> 3 (integer floor)
                B = FixedToFloat(D & 0x7) (fractional part)
                C = X - (A + B) (whatever is left)

                2^A Can be directly formed with bitwise operations because A is an integer.  Just stick it in the mantisa of a floating point number

                2^B Is computed from a look up table (8 entries)

                2^C Is computed with an approximating polynomial (taylor series)
            */

            C_f32x4 rAbsSource = (*pRegSrc0).Fabs();
            C_f32x1 rMagicAdd = (float)(1 << (23 - 3));

            //
            // D = FloatToNDot3(X)
            // Magic is is a number that that makes the correct bits (N.3)
            // show up in the least significant bits of the mantissa
            //

            C_u32x4 uD = (rAbsSource + rMagicAdd.Replicate()).AsInt32x4();

            //
            // A = D >> 3 (integer floor)
            //

            C_u32   uMaskFF = 0xff;
            C_u32x4 uA = (uD >> 3) & uMaskFF.Replicate();

            //
            // FixedToFloat(D & 0x7) (fractional part - 3 bits)
            //

            C_f32x1 rFixedToFloat = 1.0f / (float)(1 << 3);
            C_u32   uMask = (1 << 3) - 1;

            C_f32x4 rB = (uD & uMask.Replicate()).ToFloat4() * rFixedToFloat.Replicate();

            //
            // C = X - (A + B) (whatever is left)
            //

            C_f32x4 rC = rAbsSource - (uA.ToFloat4() + rB);

            //
            // Term1 = 2^A (integer porition)
            // Formed directly from floating point layout
            //

            C_u32 uBias = 127;
            C_f32x4 rTerm1 = ((uA + uBias.Replicate()) << 23).As_f32x4();

            //
            // Term2 = look up most significant 2 bits of mantissa in table
            //

            C_f32x4 rTerm2 = rZero;

            const float rgTable[8] =
            {
                1.0f,           // 2^0.0
                1.090507733f,   // 2^.125
                1.189207115f,   // 2^.25
                1.296839555f,   // 2^.375
                1.414213562f,
                1.542210825f,
                1.681792831f,
                1.834008086f
           };

            for (i = 0; i < 8; i++)
            {
                C_f32x1 rIndexRecip = (float)i/8.0f;
                C_u32x4 foundValue = (rB == rIndexRecip.Replicate()).AsInt32x4();

                C_f32x1 rValue = rgTable[i];
                rTerm2 += (foundValue & rValue.Replicate().AsInt32x4()).As_f32x4();
            }

            //
            // Term3 = Approximate remaining portion with polynomial
            //

            C_f32x4 rC2 = rC*rC;
            C_f32x4 rC3 = rC2*rC;

            C_f32x1 rCoeff1 = 0.693147180559945f; // 2 / LN2
            C_f32x1 rCoeff2 = 0.240226506959101f; // 2 / (3 LN 2)
            C_f32x1 rCoeff3 = 0.0555041086648216f; // 2 / (5 LN 2)

            C_f32x1 rOne(1.0f);
            C_f32x4 rTerm3 = rOne.Replicate() + (rC * rCoeff1.Replicate()) + (rC2 * rCoeff2.Replicate()) + (rC3 * rCoeff3.Replicate());

            C_f32x4 exp = rTerm1 * rTerm2 * rTerm3;

            //
            // If source was negative, take the reciprocal
            //
            C_f32x4 lessThanZeroMask = ((*pRegSrc0) < rZero);            
            exp = exp.Blend(exp.Reciprocal(), lessThanZeroMask);

            //
            // Check for overflow
            // Generate +INF in this case (matches REF, CRT, and hardware)
            // This is not explicitly defined in the spec, but it seems good to match hardware
            //
            f32x4 rMaxInput = {128.0f, 128.0f, 128.0f, 128.0f};
            f32x4 rINF = {0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000};             // +INF replicate
            C_f32x4 overflowMask = rAbsSource > rMaxInput;
            exp = exp.Blend(rINF, overflowMask);

            //
            // If overflow occured, and the input was negative, return 0.0 (rather than NAN)
            //
            exp = exp.Blend(rZero, overflowMask & lessThanZeroMask);

            //
            // Ensure that NAN inputs generate NAN result. 
            // Number is NAN if number > INF is true.
            //
            exp = exp.Blend((*pRegSrc0), (*pRegSrc0) > rINF);

            //
            // Ensure that -INF generates 0.0
            //
            f32x4 rNegINF = {0xFF800000, 0xFF800000, 0xFF800000, 0xFF800000};
            exp = exp.Blend(rZero, ((*pRegSrc0) == rNegINF));

            //
            // Ensure that +INF generates +INF
            //
            exp = exp.Blend(rINF, ((*pRegSrc0) == rINF));           

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pExp->DstReg, &pRegDest));

                if (!IsMasked(i, pExp->WriteMask, &pExp->Predication))
                {
                    (*pRegDest) = exp;
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }
            }

        }
        break;

    case PSTRINST_DP2ADD:
        {
            const PSTRINST_DP2ADD_PARAMS* pDP2Add = (PSTRINST_DP2ADD_PARAMS*)pBaseInstr;

            JIT_TRACE(L"PSTRINST_DP2ADD");

            IFC(shaderRegisters[0].GetRegister(&pPixelShaderState, &pDP2Add->SrcReg0, &pRegSrc00));
            IFC(shaderRegisters[1].GetRegister(&pPixelShaderState, &pDP2Add->SrcReg0, &pRegSrc01));

            IFC(shaderRegisters[0].GetRegister(&pPixelShaderState, &pDP2Add->SrcReg1, &pRegSrc10));
            IFC(shaderRegisters[1].GetRegister(&pPixelShaderState, &pDP2Add->SrcReg1, &pRegSrc11));

            IFC(shaderRegisters[0].GetRegister(&pPixelShaderState, &pDP2Add->SrcReg2, &pRegSrc2));

            ConditionalMultiply(&pRegSrc00, pDP2Add->bSrcReg0_Negate, &regSourceNegate00, rNegativeOne);
            ConditionalMultiply(&pRegSrc01, pDP2Add->bSrcReg0_Negate, &regSourceNegate01, rNegativeOne);

            ConditionalMultiply(&pRegSrc10, pDP2Add->bSrcReg1_Negate, &regSourceNegate10, rNegativeOne);
            ConditionalMultiply(&pRegSrc11, pDP2Add->bSrcReg1_Negate, &regSourceNegate11, rNegativeOne);

            ConditionalMultiply(&pRegSrc2, pDP2Add->bSrcReg2_Negate, &regSourceNegate2, rNegativeOne);

            C_f32x4 dp2Add = (*pRegSrc00)*(*pRegSrc10) + (*pRegSrc01)*(*pRegSrc11) + (*pRegSrc2);

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP2Add->DstReg, &pRegDest));

                if (!IsMasked(i, pDP2Add->WriteMask, &pDP2Add->Predication))
                {
                    (*pRegDest) = dp2Add;
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }
            }

       }
       break;

    case PSTRINST_DP3:
        {
            const PSTRINST_DP3_PARAMS* pDP3 = (PSTRINST_DP3_PARAMS*)pBaseInstr;

            JIT_TRACE(L"PSTRINST_DP3");

            IFC(shaderRegisters[0].GetRegister(&pPixelShaderState, &pDP3->SrcReg0, &pRegSrc00));
            IFC(shaderRegisters[1].GetRegister(&pPixelShaderState, &pDP3->SrcReg0, &pRegSrc01));
            IFC(shaderRegisters[2].GetRegister(&pPixelShaderState, &pDP3->SrcReg0, &pRegSrc02));

            IFC(shaderRegisters[0].GetRegister(&pPixelShaderState, &pDP3->SrcReg1, &pRegSrc10));
            IFC(shaderRegisters[1].GetRegister(&pPixelShaderState, &pDP3->SrcReg1, &pRegSrc11));
            IFC(shaderRegisters[2].GetRegister(&pPixelShaderState, &pDP3->SrcReg1, &pRegSrc12));

            ConditionalMultiply(&pRegSrc00, pDP3->bSrcReg0_Negate, &regSourceNegate00, rNegativeOne);
            ConditionalMultiply(&pRegSrc01, pDP3->bSrcReg0_Negate, &regSourceNegate01, rNegativeOne);
            ConditionalMultiply(&pRegSrc02, pDP3->bSrcReg0_Negate, &regSourceNegate02, rNegativeOne);

            ConditionalMultiply(&pRegSrc10, pDP3->bSrcReg1_Negate, &regSourceNegate10, rNegativeOne);
            ConditionalMultiply(&pRegSrc11, pDP3->bSrcReg1_Negate, &regSourceNegate11, rNegativeOne);
            ConditionalMultiply(&pRegSrc12, pDP3->bSrcReg1_Negate, &regSourceNegate12, rNegativeOne);

            C_f32x4 dp3 = (*pRegSrc00)*(*pRegSrc10) + (*pRegSrc01)*(*pRegSrc11) + (*pRegSrc02)*(*pRegSrc12);

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP3->DstReg, &pRegDest));

                if (!IsMasked(i, pDP3->WriteMask, &pDP3->Predication))
                {
                    (*pRegDest) = dp3;
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }
            }
        }
        break;

    case PSTRINST_DP4:
        {
            const PSTRINST_DP4_PARAMS* pDP4 = (PSTRINST_DP4_PARAMS*)pBaseInstr;

            JIT_TRACE(L"PSTRINST_DP4");

            IFC(shaderRegisters[0].GetRegister(&pPixelShaderState, &pDP4->SrcReg0, &pRegSrc00));
            IFC(shaderRegisters[1].GetRegister(&pPixelShaderState, &pDP4->SrcReg0, &pRegSrc01));
            IFC(shaderRegisters[2].GetRegister(&pPixelShaderState, &pDP4->SrcReg0, &pRegSrc02));
            IFC(shaderRegisters[3].GetRegister(&pPixelShaderState, &pDP4->SrcReg0, &pRegSrc03));

            IFC(shaderRegisters[0].GetRegister(&pPixelShaderState, &pDP4->SrcReg1, &pRegSrc10));
            IFC(shaderRegisters[1].GetRegister(&pPixelShaderState, &pDP4->SrcReg1, &pRegSrc11));
            IFC(shaderRegisters[2].GetRegister(&pPixelShaderState, &pDP4->SrcReg1, &pRegSrc12));
            IFC(shaderRegisters[3].GetRegister(&pPixelShaderState, &pDP4->SrcReg1, &pRegSrc13));

            ConditionalMultiply(&pRegSrc00, pDP4->bSrcReg0_Negate, &regSourceNegate00, rNegativeOne);
            ConditionalMultiply(&pRegSrc01, pDP4->bSrcReg0_Negate, &regSourceNegate01, rNegativeOne);
            ConditionalMultiply(&pRegSrc02, pDP4->bSrcReg0_Negate, &regSourceNegate02, rNegativeOne);
            ConditionalMultiply(&pRegSrc03, pDP4->bSrcReg0_Negate, &regSourceNegate03, rNegativeOne);

            ConditionalMultiply(&pRegSrc10, pDP4->bSrcReg1_Negate, &regSourceNegate10, rNegativeOne);
            ConditionalMultiply(&pRegSrc11, pDP4->bSrcReg1_Negate, &regSourceNegate11, rNegativeOne);
            ConditionalMultiply(&pRegSrc12, pDP4->bSrcReg1_Negate, &regSourceNegate12, rNegativeOne);
            ConditionalMultiply(&pRegSrc13, pDP4->bSrcReg1_Negate, &regSourceNegate13, rNegativeOne);

            C_f32x4 dp4 = (*pRegSrc00)*(*pRegSrc10) 
                        + (*pRegSrc01)*(*pRegSrc11) 
                        + (*pRegSrc02)*(*pRegSrc12)
                        + (*pRegSrc03)*(*pRegSrc13);

            for (i = 0; i < 4; i++)
            {
                IFC(shaderRegisters[i].GetRegister(&pPixelShaderState, &pDP4->DstReg, &pRegDest));

                if (!IsMasked(i, pDP4->WriteMask, &pDP4->Predication))
                {
                    (*pRegDest) = dp4;
                }
                else if (!pRegDest->IsInitialized())
                {
                    (*pRegDest) = rZero;
                }
            }
        }
        break;

    default:
        JIT_TRACE(L"PSTRINST_?? - unknown instruction");
        IFC(E_INVALIDARG);
    }

Cleanup:
    return hr;
}

//-------------------------------------------------------------------------
//
//  Function:   CPixelShaderCompiler::CompilePixelShader
//
//  Synopsis:
//     Compile a pixel shader
//
//-------------------------------------------------------------------------
HRESULT
CPixelShaderCompiler::Compile(
    __out GenerateColorsEffect **ppfn
    )
{
    HRESULT  hr             = S_OK;
    BOOL     fEnteredJitter = FALSE;
    UINT8   *pInstructions  = m_pTranslated->GetOutputBuffer();
    UINT32   cInstructions  = m_pTranslated->GetPSTRInstCount();
    UINT8   *pBinaryCode    = NULL;
    UINT32   i;

    // Start the JIT'er

    IFC(CJitterAccess::Enter(sizeof(GenerateColorsEffectParams*)));
    fEnteredJitter = TRUE;

    // Disable the use of negative stack offsets.  This will likely increase generated code
    // size, but is more compatible with debugging and profiling.
    CJitterAccess::SetMode(CJitterAccess::sc_uidUseNegativeStackOffsets, 0);

    {
        CInstructionVariables instructionVars;
        CPixelShaderRegisters *shaderRegisters = instructionVars.m_shaderRegisters;

        // Set the constants

        for (i = 0; i < 4; i++)
        {
            shaderRegisters[i].SetIndex(i);
            IFC(LoadShaderConstants(i, &shaderRegisters[i]));
        }

        // Constants loaded and common temporaries

        const f32x4 c_rZero = {0.0f, 0.0f, 0.0f, 0.0f};
        const f32x4 c_rNegativeOne = {-1.0f, -1.0f, -1.0f, -1.0f};
        const f32x4 c_r255 = {255.0f, 255.0f, 255.0f, 255.0f};

        instructionVars.m_r255 = c_r255;
        instructionVars.m_rZero = c_rZero;
        instructionVars.m_rNegativeOne = c_rNegativeOne;

        const C_f32x4 &r255  = instructionVars.m_r255;
        const C_f32x4 &rZero = instructionVars.m_rZero;

        // Get call parameters
    
        C_pVoid pArguments = C_pVoid::GetpVoidArgument(0); // Get GenerateColorsEffectParams structure argument.
        
        instructionVars.m_pPixelShaderState = (pArguments.GetMemberPtr(OFFSET_OF(GenerateColorsEffectParams, pPixelShaderState))).AsP_u8();
        P_u8 &pPixelShaderState = instructionVars.m_pPixelShaderState;

        P_u32 pDst =  (pArguments.GetMemberPtr(OFFSET_OF(GenerateColorsEffectParams, pPargbBuffer))).AsP_u32();
        C_u32 uCount = (pArguments.GetMemberUINT32(OFFSET_OF(GenerateColorsEffectParams, nCount)));
        C_u32 uX = (pArguments.GetMemberUINT32(OFFSET_OF(GenerateColorsEffectParams, nX)));
        C_u32 uY = (pArguments.GetMemberUINT32(OFFSET_OF(GenerateColorsEffectParams, nY)));


        // Compute eval value, i.e., variables to do incremental tex coord evaluation

        C_f32x4 evalDeltaRight;
        C_f32x4 evalDeltaDown;

        IFC(ComputeEval(
            &pPixelShaderState, 
            &uX, 
            &uY, 
            &instructionVars.m_evalRight,
            &evalDeltaRight,
            &instructionVars.m_evalDown,
            &evalDeltaDown
            ));

        // Set up texture variables

        IFC(LoadTextureVariables(&pPixelShaderState));

        // Preload constants outside the pixel loop

        for (UINT32 uInstruction = 0; uInstruction < cInstructions; uInstruction++)
        {
            PSTRINST_BASE_PARAMS *pBaseInstr = (PSTRINST_BASE_PARAMS*)pInstructions;

            for (INT32 i = 0; i < 4; i++)
            {
                IFC(PreloadConstant(i, pBaseInstr, &instructionVars));
            }

            pInstructions += pBaseInstr->InstSize;
        }
        pInstructions = m_pTranslated->GetOutputBuffer();


        // The main pixel loop

        C_Loop loop;    // do while (uCount != 0)
        {
            // Init kill if needed

            if (m_pTranslated->HasTexKillInstructions())
            {
                JIT_TRACE(L"==> kill instructions present");
                for (i = 0; i < 4; i++)
                {
                    instructionVars.m_kill[i] = rZero;
                }
            }

            // Compile instructions

            for (UINT32 uInstruction = 0; uInstruction < cInstructions; uInstruction++)
            {
                PSTRINST_BASE_PARAMS *pBaseInstr = (PSTRINST_BASE_PARAMS*)pInstructions;
                PSTR_INSTRUCTION_OPCODE_TYPE opcode = pBaseInstr->Inst;

                switch (opcode)
                {
                case PSTRINST_SAMPLE:
                case PSTRINST_SWIZZLE:
                case PSTRINST_RCP:
                case PSTRINST_DP2ADD:
                case PSTRINST_DP3:
                case PSTRINST_DP4:
                case PSTRINST_SIN:
                case PSTRINST_COS:
                case PSTRINST_LOG:
                case PSTRINST_EXP:
                case PSTRINST_RSQ:
                    {
                        IFC(CompileDependentInstruction(pBaseInstr, &instructionVars));
                    }
                    break;

                default:
                    for (int i = 0; i < 4; i++)
                    {
                        IFC(CompileInstruction(
                            i,
                            pBaseInstr,
                            &instructionVars
                            ));
                    }
                }

                pInstructions += pBaseInstr->InstSize;
            }
    
            // Output the color

            INT32 rgChannelOrder[4] = {3, 0, 1, 2};

            C_u32x4 colorOutput;

            for (i = 0; i < 4; i++)
            {
                C_f32x4 rOutputColor = *shaderRegisters[rgChannelOrder[i]].GetColorOutput();

                // Clamp

                rOutputColor *= r255;
                rOutputColor = rOutputColor.Min(r255);
                rOutputColor = rOutputColor.Max(rZero);

                // Check kill

                if (m_pTranslated->HasTexKillInstructions())
                {
                    rOutputColor = rOutputColor.Blend(rZero, instructionVars.m_kill[i]);
                }

                // Add to output color
                if (i == 0)
                {
                    colorOutput = rOutputColor.ToInt32x4();
                }
                else
                {
                    colorOutput = (colorOutput << 8) | rOutputColor.ToInt32x4();
                }
            }

            // Write to our buffer

            pDst[0] = colorOutput.GetElement(0);
            --uCount;

            C_Branch branch1;
            branch1.BranchOnZero(uCount);
            {
                pDst[1] = colorOutput.GetElement(1);
                --uCount;
            }
            branch1.BranchHere();

            C_Branch branch2;
            branch2.BranchOnZero(uCount);
            {
                pDst[2] = colorOutput.GetElement(2);
                --uCount;
            }
            branch2.BranchHere();

            C_Branch branch3;
            branch3.BranchOnZero(uCount);
            {
                pDst[3] = colorOutput.GetElement(3);
                --uCount;
            }
            branch3.BranchHere();

            // Advance

            pDst += 4;
            instructionVars.m_evalRight += evalDeltaRight;
            instructionVars.m_evalDown += evalDeltaDown;
        }
        loop.RepeatIfNonZero(uCount);
    }

    IFC(CJitterAccess::Compile(&pBinaryCode));

#if DBG

    //
    // Output a breakpoint address for debugging
    //
    OutputBreakpointTrace(pBinaryCode);
#endif

    //
    // Set the output program
    //

    *ppfn = (GenerateColorsEffect *)pBinaryCode;

Cleanup:
    if (fEnteredJitter)
    {
        CJitterAccess::Leave();
    }
    RRETURN(hr);
}





