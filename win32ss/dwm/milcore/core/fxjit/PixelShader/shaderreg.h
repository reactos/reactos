// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      HLSL pixel shader register class
//

//
// Register max counts
//

#define PIXELSHADER_CONSTS_MAX     32
#define PIXELSHADER_TEMPS_MAX      32
#define PIXELSHADER_POSTMODSRC_MAX 32
#define PIXELSHADER_SAMPLERS_MAX   16
#define PIXELSHADER_COLOROUT_MAX    4

//-------------------------------------------------------------------------
//
//  Class:   CPixelShaderRegisters
//
//  Synopsis:
//     HLSL model pixel shader registers
//
//-------------------------------------------------------------------------
class CPixelShaderRegisters
{
public:
    //
    // Set the channel index for these registers
    //

    void SetIndex(INT32 nIndex)
    {
        m_nIndex = nIndex;
    }

    //
    // Set a constant register
    //

    XRESULT SetConstant(
        __in INT32 nChannel,
        __in ConstDef_F constant
        );
                 
    //
    // Get a JIT variable for the pixel shader register
    //

    XRESULT GetRegister(
        __in P_u8 *pPixelShaderState, 
        __in const PSTRRegister *pRegister, 
        __out C_f32x4 **ppRegisterOut
        );
        
    //
    // Get the JIT variable for the output color of the primary 
    // ouput register.
    //

    C_f32x4 *
    CPixelShaderRegisters::GetColorOutput()
    {
        return &(m_colorOutput[0]);
    }

private:
    //
    // HLSL model registers
    //

    C_f32x4 m_colorOutput[PIXELSHADER_COLOROUT_MAX];
    C_f32x4 m_constants[PIXELSHADER_CONSTS_MAX];
    C_f32x4 m_temps[PIXELSHADER_TEMPS_MAX];
    C_f32x4 m_postModSrc[PIXELSHADER_POSTMODSRC_MAX];
    C_f32x4 m_textureSampler[PIXELSHADER_SAMPLERS_MAX];
    C_f32x4 m_scratch[PIXELSHADER_TEMPS_MAX];

    INT32 m_nIndex;
};


//-------------------------------------------------------------------------
//
//  Function:   IsPredicateFalse
//
//  Synopsis:
//     Is the predicate false
//
//-------------------------------------------------------------------------
inline INT32 
IsPredicateFalse(__in const PSTRPredInfo &predicateInfo)
{
    return (predicateInfo.PredicateReg.GetRegType() == PSTRREG_PREDICATETRUE 
            && predicateInfo.bInvertPredicate == TRUE);
}

//-------------------------------------------------------------------------
//
//  Function:   IsSimpleWrite
//
//  Synopsis:
//     Is the predicate true and the write all mask set
//
//-------------------------------------------------------------------------
inline INT32
IsMasked(
    __in UINT32 i,
    __in UINT8 writeMask,
    __in_opt const PSTRPredInfo *pPredicateInfo
    )
{
    if (pPredicateInfo != NULL)
    {
        if (IsPredicateFalse(*pPredicateInfo))
        {
            return TRUE;
        }
    }

    switch (i)
    {
    case 0:
        return !(PSTR_COMPONENTMASK_0 & writeMask);
        break;

    case 1:
        return !(PSTR_COMPONENTMASK_1 & writeMask);
        break;

    case 2:
        return !(PSTR_COMPONENTMASK_2 & writeMask);
        break;

    case 3:
        return !(PSTR_COMPONENTMASK_3 & writeMask);
        break;
    }

    return TRUE;
}

//
// Offset macros
//

#ifdef __MACINTOSH__
#define OFFSET_OF(s, m)    __builtin_offsetof(s, m)
#else // __MACINTOSH__
#define OFFSET_OF(s, m)    UINT32(UINT64(HANDLE(&(((s *) 0)->m))))
#endif




