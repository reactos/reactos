// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


///////////////////////////////////////////////////////////////////////////////

//
// pstrans.cpp
//
// Direct3D Pixel Shader Translator:
//
// Translate all pixel shader versions
// to a common instruction set.
//
// The combination of pstrans.hpp and pstrans.cpp are
// designed to be able to compile outside of Refrast.
// In other words, it should be easy to integrate this
// code into other projects.  The only small catch is that
// a project taking this code needs to have a pch.cpp.
//
///////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

#define _BAD_VALUE (-1)
#define _DWordCount() (pToken - pCode)
#define EXIT_WITH_STATUS(hr) {m_Status = (hr); goto ERROR_EXIT;}

#define DPFERR
#define UNREFERENCED_PARAMETER(x)   (x)

// attribute array offsets
#define RDATTR_TEXTURE0             0
#define RDATTR_TEXTURE1             4
#define RDATTR_TEXTURE2             8
#define RDATTR_TEXTURE3             12
#define RDATTR_TEXTURE4             16
#define RDATTR_TEXTURE5             20
#define RDATTR_TEXTURE6             24
#define RDATTR_TEXTURE7             28
#define RDATTR_DIFFUSE              32
#define RDATTR_SPECULAR             36
#define RDATTR_FOG                  40
#define RDATTR_DEPTH                (MAX(41,PSTR_MAX_NUMINPUTREG*PSTR_NUM_COMPONENTS_IN_REGISTER))
const UINT RDPRIM_MAX_ATTRIBUTES = (RDATTR_DEPTH+1);

//-----------------------------------------------------------------------------
//
// CPSTrans::GetInstructionSize()
//
// Retrieve PSTR* translated instruction structure size.
//
//-----------------------------------------------------------------------------
size_t CPSTrans::GetInstructionSize(PSTR_INSTRUCTION_OPCODE_TYPE Inst)
{
    size_t InstSize = 0;
    switch( Inst )
    {
    case PSTRINST_BEM:              InstSize = sizeof(PSTRINST_BEM_PARAMS); break;
    case PSTRINST_DEPTH:            InstSize = sizeof(PSTRINST_DEPTH_PARAMS); break;
    case PSTRINST_DSTMOD:           InstSize = sizeof(PSTRINST_DSTMOD_PARAMS); break;
    case PSTRINST_END:              InstSize = sizeof(PSTRINST_END_PARAMS); break;
    case PSTRINST_EVAL:             InstSize = sizeof(PSTRINST_EVAL_PARAMS); break;
    case PSTRINST_FORCELOD:         InstSize = sizeof(PSTRINST_FORCELOD_PARAMS); break;
    case PSTRINST_KILL:             InstSize = sizeof(PSTRINST_KILL_PARAMS); break;
    case PSTRINST_LUMINANCE:        InstSize = sizeof(PSTRINST_LUMINANCE_PARAMS); break;
    case PSTRINST_NEXTD3DPSINST:    InstSize = sizeof(PSTRINST_NEXTD3DPSINST_PARAMS); break;
    case PSTRINST_QUADLOOPBEGIN:    InstSize = sizeof(PSTRINST_QUADLOOPBEGIN_PARAMS); break;
    case PSTRINST_QUADLOOPEND:      InstSize = sizeof(PSTRINST_QUADLOOPEND_PARAMS); break;
    case PSTRINST_SAMPLE:           InstSize = sizeof(PSTRINST_SAMPLE_PARAMS); break;
    case PSTRINST_SRCMOD:           InstSize = sizeof(PSTRINST_SRCMOD_PARAMS); break;
    case PSTRINST_SWIZZLE:          InstSize = sizeof(PSTRINST_SWIZZLE_PARAMS); break;
    case PSTRINST_TEXCOVERAGE:      InstSize = sizeof(PSTRINST_TEXCOVERAGE_PARAMS); break;
    case PSTRINST_CALL:             InstSize = sizeof(PSTRINST_CALL_PARAMS); break;
    case PSTRINST_CALLNZ:           InstSize = sizeof(PSTRINST_CALLNZ_PARAMS); break;    
    case PSTRINST_JUMP:             InstSize = sizeof(PSTRINST_JUMP_PARAMS); break;
    case PSTRINST_PUSHREG:          InstSize = sizeof(PSTRINST_PUSHREG_PARAMS); break;
    case PSTRINST_POPREG:           InstSize = sizeof(PSTRINST_POPREG_PARAMS); break;
    case PSTRINST_RET:              InstSize = sizeof(PSTRINST_RET_PARAMS); break;
    case PSTRINST_ABS:              InstSize = sizeof(PSTRINST_ABS_PARAMS); break;
    case PSTRINST_ADD:              InstSize = sizeof(PSTRINST_ADD_PARAMS); break;
    case PSTRINST_CND:              InstSize = sizeof(PSTRINST_CND_PARAMS); break;
    case PSTRINST_CMP:              InstSize = sizeof(PSTRINST_CMP_PARAMS); break;
    case PSTRINST_COS:              InstSize = sizeof(PSTRINST_COS_PARAMS); break;
    case PSTRINST_DSX:              InstSize = sizeof(PSTRINST_DSX_PARAMS); break;
    case PSTRINST_DSY:              InstSize = sizeof(PSTRINST_DSY_PARAMS); break;
    case PSTRINST_DP2ADD:           InstSize = sizeof(PSTRINST_DP2ADD_PARAMS); break;
    case PSTRINST_DP3:              InstSize = sizeof(PSTRINST_DP3_PARAMS); break;
    case PSTRINST_DP4:              InstSize = sizeof(PSTRINST_DP4_PARAMS); break;
    case PSTRINST_EXP:              InstSize = sizeof(PSTRINST_EXP_PARAMS); break;
    case PSTRINST_FRC:              InstSize = sizeof(PSTRINST_FRC_PARAMS); break;
    case PSTRINST_LEGACYRCP:        InstSize = sizeof(PSTRINST_LEGACYRCP_PARAMS); break;
    case PSTRINST_LOG:              InstSize = sizeof(PSTRINST_LOG_PARAMS); break;
    case PSTRINST_LRP:              InstSize = sizeof(PSTRINST_LRP_PARAMS); break;
    case PSTRINST_MAD:              InstSize = sizeof(PSTRINST_MAD_PARAMS); break;
    case PSTRINST_MAX:              InstSize = sizeof(PSTRINST_MAX_PARAMS); break;
    case PSTRINST_MIN:              InstSize = sizeof(PSTRINST_MIN_PARAMS); break;
    case PSTRINST_MOV:              InstSize = sizeof(PSTRINST_MOV_PARAMS); break;
    case PSTRINST_MUL:              InstSize = sizeof(PSTRINST_MUL_PARAMS); break;
    case PSTRINST_RCP:              InstSize = sizeof(PSTRINST_RCP_PARAMS); break;
    case PSTRINST_RSQ:              InstSize = sizeof(PSTRINST_RSQ_PARAMS); break;
    case PSTRINST_SETPRED:          InstSize = sizeof(PSTRINST_SETPRED_PARAMS); break;
    case PSTRINST_SIN:              InstSize = sizeof(PSTRINST_SIN_PARAMS); break;
    case PSTRINST_BEGINLOOP:        InstSize = sizeof(PSTRINST_BEGINLOOP_PARAMS); break;
    case PSTRINST_ENDLOOP:          InstSize = sizeof(PSTRINST_ENDLOOP_PARAMS); break;
    case PSTRINST_BEGINREP:         InstSize = sizeof(PSTRINST_BEGINREP_PARAMS); break;
    case PSTRINST_ENDREP:           InstSize = sizeof(PSTRINST_ENDREP_PARAMS); break;
    case PSTRINST_BREAK:            InstSize = sizeof(PSTRINST_BREAK_PARAMS); break;
    case PSTRINST_IF:               InstSize = sizeof(PSTRINST_IF_PARAMS); break;
    case PSTRINST_ENDIF:            InstSize = sizeof(PSTRINST_ENDIF_PARAMS); break;
    case PSTRINST_ELSE:             InstSize = sizeof(PSTRINST_ELSE_PARAMS); break;
    case PSTRINST_LOADCONSTBOOL:    InstSize = sizeof(PSTRINST_LOADCONSTBOOL_PARAMS); break;
    case PSTRINST_DEFINESUB:        InstSize = sizeof(PSTRINST_DEFINESUB_PARAMS); break;
    default:
        ;     // CPSTrans::GetInstructionSize - Unrecognized instruction.
        break;
    }
    return InstSize;
}

//-----------------------------------------------------------------------------
//
// Helper functions used by CPSTrans::Initialize()'s third pass through shader,
// using shared worker data temporarily allocated in CPSTrans (m_pWorkerData)
//
//-----------------------------------------------------------------------------
#define _InstParam(__INST)     (*(__INST##_PARAMS*)m_pWorkerData->pPSTRInst)
#define _Set(RegType, RegNum)   Set(RegType,RegNum)
#define _SetRelAddr(RegType,RegNum,RelAddrRegType,RelAddrRegNum,RelAddrSrcSelector)   \
                    SetRelAddr(RegType,RegNum,RelAddrRegType,RelAddrRegNum,RelAddrSrcSelector)
#define _UpdateRegNum(RegNum)   UpdateRegNum(RegNum)

size_t  CPSTrans::_GetParamOffset(void* pParam) const
{
    return (size_t)((BYTE*)pParam - m_pWorkerData->pPSTRInstBuffer);
}
size_t  CPSTrans::_GetOffset() const
{
    return (size_t)((BYTE*)m_pWorkerData->pPSTRInst - m_pWorkerData->pPSTRInstBuffer);
}
size_t  CPSTrans::_GetNextInstOffset() const
{
    return (size_t)(m_pWorkerData->pPSTRInst - m_pWorkerData->pPSTRInstBuffer + m_pWorkerData->LastPSTRInstSize);
}
UINT    CPSTrans::_GetNextPSTRInstID() const
{
    return m_cPSTRInst;
}
void    CPSTrans::_NewPSInstImpl(PSTR_INSTRUCTION_OPCODE_TYPE Inst)
{
    size_t InstSize = GetInstructionSize(Inst);
    m_pWorkerData->PSTROffset = _GetNextInstOffset();
    SetOutputBufferGrowSize(MAX(512,(UINT)m_pWorkerData->PSTROffset));
    if( FAILED(GrowOutputBuffer((UINT)(m_pWorkerData->PSTROffset + InstSize))))
    {
        // throw E_OUTOFMEMORY;
    }
    m_pWorkerData->pPSTRInstBuffer = GetOutputBufferI();
    m_pWorkerData->pPSTRInst = m_pWorkerData->pPSTRInstBuffer + m_pWorkerData->PSTROffset;
    ((PSTRINST_BASE_PARAMS*)m_pWorkerData->pPSTRInst)->Inst = Inst;
    ((PSTRINST_BASE_PARAMS*)m_pWorkerData->pPSTRInst)->InstSize = InstSize;
    ((PSTRINST_BASE_PARAMS*)m_pWorkerData->pPSTRInst)->D3DInstID = UINT_MAX;
    ((PSTRINST_BASE_PARAMS*)m_pWorkerData->pPSTRInst)->PSTRInstID = m_cPSTRInst++;
    ((PSTRINST_BASE_PARAMS*)m_pWorkerData->pPSTRInst)->D3DInstByteOffset = UINT_MAX;
    m_pWorkerData->LastPSTRInstSize = InstSize;
}
void    CPSTrans::_EnterQuadPixelLoop()
{
    if(!m_pWorkerData->bInQuadPixelLoop)
    {
        m_pWorkerData->bQueuedEnterQuadPixelLoop = true;
    }
}

void    CPSTrans::_LeaveQuadPixelLoop()
{
    if(m_pWorkerData->bInQuadPixelLoop)
    {
        _NewPSInstImpl(PSTRINST_QUADLOOPEND);
        _InstParam(PSTRINST_QUADLOOPEND).JumpBackByOffset =
                                        m_pWorkerData->PSTROffset - m_pWorkerData->PSTRLoopOffset;
        m_pWorkerData->bInQuadPixelLoop = false;
    }
    m_pWorkerData->bQueuedEnterQuadPixelLoop = false;
}
void    CPSTrans::_NewPSInst(PSTR_INSTRUCTION_OPCODE_TYPE Inst)
{
    if(m_pWorkerData->bQueuedEnterQuadPixelLoop)
    {
        _NewPSInstImpl(PSTRINST_QUADLOOPBEGIN);
        m_pWorkerData->PSTRLoopOffset = m_pWorkerData->PSTROffset + sizeof(PSTRINST_QUADLOOPBEGIN_PARAMS);
        m_pWorkerData->bInQuadPixelLoop = true;
        m_pWorkerData->bQueuedEnterQuadPixelLoop = false;
    }
    _NewPSInstImpl(Inst);
}
void    CPSTrans::_NoteInstructionEvent()
{
    _NewPSInstImpl(PSTRINST_NEXTD3DPSINST);
    _InstParam(PSTRINST_NEXTD3DPSINST).pInst = m_pWorkerData->pInst;
    _InstParam(PSTRINST_NEXTD3DPSINST).D3DInstByteOffset = m_pWorkerData->pInst->ByteOffset;
    _InstParam(PSTRINST_NEXTD3DPSINST).D3DInstID = m_pWorkerData->D3DInstID;
}
void    CPSTrans::_NoteInstructionEventNOBREAK()
{
    _NewPSInstImpl(PSTRINST_NEXTD3DPSINST);
    _InstParam(PSTRINST_NEXTD3DPSINST).pInst = m_pWorkerData->pInst;
    _InstParam(PSTRINST_NEXTD3DPSINST).D3DInstByteOffset = (size_t)-1;
    _InstParam(PSTRINST_NEXTD3DPSINST).D3DInstID = m_pWorkerData->D3DInstID;
}
void    CPSTrans::_EmitDstMod(const PSTRRegister& DstReg, BYTE WriteMask)
{
    if( !((1.0f == m_pWorkerData->DstScale) &&
        (-FLT_MAX == m_pWorkerData->DstRange[0]) &&
        (FLT_MAX == m_pWorkerData->DstRange[1])))
    {
        _NewPSInst(PSTRINST_DSTMOD);
        _InstParam(PSTRINST_DSTMOD).DstReg    = DstReg;
        _InstParam(PSTRINST_DSTMOD).WriteMask = WriteMask;
        _InstParam(PSTRINST_DSTMOD).fScale    = m_pWorkerData->DstScale;
        _InstParam(PSTRINST_DSTMOD).fRangeMin = m_pWorkerData->DstRange[0];
        _InstParam(PSTRINST_DSTMOD).fRangeMax = m_pWorkerData->DstRange[1];
        _InstParam(PSTRINST_DSTMOD).Predication = m_pWorkerData->PredicateInfo;
    }
}
void CPSTrans::_EmitProj(const PSTRRegister& DstReg,
                      const PSTRRegister& SrcReg,
                      BYTE ProjComponentMask, FLOAT fRangeMax, BOOL bLegacyRCP)
{
    // The macro _EmitProj emits instructions to do the following:
    // - Put reciprocal of source (x,y,z,w) component __COMPONENT into scratch register 0, all components:
    //   If __COMPONENT is z, for example, that yields: [1/z,1/z,1/z,1/z] in the scratch register.
    // - Multiply source register register by scratch register ([1/z,1/z,1/z,1/z] example above) and put the result into the dest register.
    //   In z example you get [x/z, y/z, 1, (untouched)] in dest.
    if( bLegacyRCP )
    {
        _NewPSInst(PSTRINST_LEGACYRCP);
        _InstParam(PSTRINST_LEGACYRCP).DstReg._Set(PSTRREG_SCRATCH,0);
        _InstParam(PSTRINST_LEGACYRCP).SrcReg0 = SrcReg;
        _InstParam(PSTRINST_LEGACYRCP).bSrcReg0_Negate = FALSE;
        _InstParam(PSTRINST_LEGACYRCP).WriteMask  = PSTR_COMPONENTMASK_ALL;
        _InstParam(PSTRINST_LEGACYRCP).SrcReg0_Selector  =
            (PSTR_COMPONENTMASK_0 == ProjComponentMask) ? PSTR_SELECT_R :
            (PSTR_COMPONENTMASK_1 == ProjComponentMask) ? PSTR_SELECT_G :
            (PSTR_COMPONENTMASK_2 == ProjComponentMask) ? PSTR_SELECT_B : PSTR_SELECT_A;
        _InstParam(PSTRINST_LEGACYRCP).fRangeMax        = fRangeMax;
        _InstParam(PSTRINST_LEGACYRCP).Predication = m_pWorkerData->PredicateInfo;
    }
    else
    {
        _NewPSInst(PSTRINST_RCP);
        _InstParam(PSTRINST_RCP).DstReg._Set(PSTRREG_SCRATCH,0);
        _InstParam(PSTRINST_RCP).SrcReg0 = SrcReg;
        _InstParam(PSTRINST_RCP).bSrcReg0_Negate = FALSE;
        _InstParam(PSTRINST_RCP).WriteMask  = PSTR_COMPONENTMASK_ALL;
        _InstParam(PSTRINST_RCP).SrcReg0_Selector  =
            (PSTR_COMPONENTMASK_0 == ProjComponentMask) ? PSTR_SELECT_R :
            (PSTR_COMPONENTMASK_1 == ProjComponentMask) ? PSTR_SELECT_G :
            (PSTR_COMPONENTMASK_2 == ProjComponentMask) ? PSTR_SELECT_B : PSTR_SELECT_A;
        _InstParam(PSTRINST_RCP).Predication = m_pWorkerData->PredicateInfo;
    }

    _NewPSInst(PSTRINST_MUL);
    _InstParam(PSTRINST_MUL).DstReg = DstReg;
    _InstParam(PSTRINST_MUL).SrcReg0._Set(PSTRREG_SCRATCH,0);
    _InstParam(PSTRINST_MUL).SrcReg1 = SrcReg;
    _InstParam(PSTRINST_MUL).bSrcReg0_Negate = FALSE;
    _InstParam(PSTRINST_MUL).bSrcReg1_Negate = FALSE;
    _InstParam(PSTRINST_MUL).WriteMask  =
                        (PSTR_COMPONENTMASK_0 == ProjComponentMask) ? PSTR_COMPONENTMASK_0 :
                        (PSTR_COMPONENTMASK_1 == ProjComponentMask) ? PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 :
                        (PSTR_COMPONENTMASK_2 == ProjComponentMask) ? PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 |
                                                                      PSTR_COMPONENTMASK_2 : PSTR_COMPONENTMASK_ALL;
    _InstParam(PSTRINST_MUL).Predication = m_pWorkerData->PredicateInfo;
}

//-----------------------------------------------------------------------------
//
// CPSTrans::Initialize()
//
// Translate the incoming D3D pixel shader into a basic instruction set.
// The resulting instruction list is stored in a BYTE* array.
//
// - In addition, an array of D3DPixelShaderInstruction structures
//  can be obtained (useful for debug info).
// - Headings of the current D3D pixelshader instruction can be inserted
//   into the output translated shader (handy for debugging).
// - The translated shader may be disassembled to debug output if desired
//   (bug finder!).
//-----------------------------------------------------------------------------
void CPSTrans::Initialize(  const DWORD *pCode,
                           const DWORD ByteCodeSize,
                           const DWORD Flags
                   )
{
    Assert(pCode && ByteCodeSize);     // CPSTrans::Initialize - Invalid input parameters
    DWORD dwDwordCodeSize = ByteCodeSize/4;    // bytecount -> dword count
    DWORD TexCoordClamp1x = 0; // bitfield for which texcoords to clamp on eval
    DWORD IgnoreD3DTTFF_PROJECTED = UINT_MAX; // bitfield for which texcoords to ignore TTFFProjected on eval.
    DWORD  Version = *pCode;

    // Hardwired to only accept PixelShader 2.0.  If version suggests otherwise,
    // fail.
    const DWORD expectedPixelShader20Version = 0x0200;
    if ((Version & 0x0000ffff) != expectedPixelShader20Version)
    {
        EXIT_WITH_STATUS(E_FAIL);
    }

    FLOAT fMax = FLT_MAX;
    FLOAT fMin = -fMax;

    // Process flags
    BOOL  bKeepDebugInfo =              (0!=(PSTRANS_FLAGS_KEEP_DEBUGINFO_INSTRUCTION_LIST & Flags));
    BOOL  bInsertD3DPSInstMarkers =     (0!=(PSTRANS_FLAGS_INSERT_D3DPSINST_MARKERS & Flags));
    BOOL  bAllowLegacyApproximations =  (0!=(PSTRANS_FLAGS_ENABLE_LEGACY_APPROXIMATIONS & Flags));

    m_ColorOutPresentMask               = 0;

    // ------------------------------------------------------------------------
    //
    // First pass through shader to find the number of instructions,
    // figure out how many constants there are.
    // Also parse dcl instructions (ps_2_0+)
    //
    // ------------------------------------------------------------------------
    {
        const DWORD* pToken = pCode;
        pToken++;    // version token
        while (*pToken != D3DPS_END())
        {
            DWORD Inst = *pToken;
            m_cD3DInst++;
            if (*pToken++ & (1L<<31))    // instruction token
            {
                DPFERR("PixelShader Token #%d: instruction token error",_DWordCount());
                EXIT_WITH_STATUS(E_FAIL);
            }
            if ( (Inst & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT )
            {
                pToken += (Inst & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
            }
            else if( (Inst & D3DSI_OPCODE_MASK) == D3DSIO_DEF )
            {
                switch(D3DSI_GETREGTYPE_RESOLVING_CONSTANTS(*pToken))
                {
                case D3DSPR_CONST:
                    m_cConstDefsF++;
                    break;
                }
                pToken += 5;
            }
            else if( (Inst & D3DSI_OPCODE_MASK) == D3DSIO_DEFI )
            {
                switch(D3DSI_GETREGTYPE_RESOLVING_CONSTANTS(*pToken))
                {
                case D3DSPR_CONSTINT:
                    m_cConstDefsI++;
                    break;
                }
                pToken += 5;
            }
            else if( (Inst & D3DSI_OPCODE_MASK) == D3DSIO_DEFB )
            {
                switch(D3DSI_GETREGTYPE(*pToken))
                {
                case D3DSPR_CONSTBOOL:
                    m_cConstDefsB++;
                    break;
                }
                pToken += 2;
            }
            else if( (Inst & D3DSI_OPCODE_MASK) == D3DSIO_DCL )
            {
                DWORD DclLength = ((Inst & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT);
                DWORD DclDesc = 0;
                DWORD DclRegister = 0;
                if( 2 == DclLength )
                {
                    DclDesc = *pToken;
                    DclRegister = *(pToken+1);
                }
                else
                {
                    NO_DEFAULT;     // CPSTrans::Initialize - dcl is expected to have 2 tokens only.
                }
                UINT RegNum = D3DSI_GETREGNUM(DclRegister);
                D3DSHADER_PARAM_REGISTER_TYPE RegType = D3DSI_GETREGTYPE(DclRegister);
                switch( RegType )
                {
                case D3DSPR_SAMPLER:
                    {
                        Assert(RegNum < PSTR_MAX_TEXTURE_SAMPLERS);     // CPSTrans::Initialize - Sampler register number too high!
                        D3DSAMPLER_TEXTURE_TYPE TextureType =
                                (D3DSAMPLER_TEXTURE_TYPE)(DclDesc & D3DSP_TEXTURETYPE_MASK);
                        m_SamplerRegDcl[RegNum] = TextureType;
                    }
                    break;
                case D3DSPR_INPUT:
                    {
                    Assert(RegNum < PSTR_MAX_NUMINPUTREG);      // CPSTrans::Initialize - Input register number too high!
                    D3DDECLUSAGE Usage = D3DDECLUSAGE_POSITION;
                    BYTE Index = 0;
                        BOOL bDoCentroid = FALSE;
                    if( D3DPS_VERSION(3,0) <= Version )
                    {
                        Usage = (D3DDECLUSAGE)D3DSI_GETUSAGE(DclDesc);
                        Index = (BYTE)D3DSI_GETUSAGEINDEX(DclDesc);
                    }
                    else if( 0 == RegNum )
                    {
                        Usage = D3DDECLUSAGE_COLOR;
                        Index = 0;
                    }
                    else if( 1 == RegNum )
                    {
                        Usage = D3DDECLUSAGE_COLOR;
                        Index = 1;
                    }
                    else
                    {
                        NO_DEFAULT;     // CPSTrans::Initialize - Unexpected input register type.
                    }
                        bDoCentroid = ((D3DSPDM_MSAMPCENTROID & DclRegister) || ((D3DPS_VERSION(2,0) <= Version)&&(D3DDECLUSAGE_COLOR == Usage)));
                    if(FAILED(m_InputRegDclInfo.AddNewDcl(Usage,Index,PSTRREG_INPUT,RegNum,
                                                (BYTE)((DclRegister & D3DSP_WRITEMASK_ALL)>>PSTR_COMPONENTMASK_SHIFT),
                                                    bDoCentroid)))
                    {
                        DPFERR("CPSTrans::Initialize - Out of memory.");
                        EXIT_WITH_STATUS(E_FAIL);
                    }
                    }
                    break;
                case D3DSPR_TEXTURE:
                    {
                    Assert(RegNum < PSTR_MAX_NUMTEXTUREREG);        // CPSTrans::Initialize - Texture register number too high!
                    Assert(D3DPS_VERSION(3,0) > Version);           // CPSTrans::Initialize - t# registers not available above ps_2_x
                    BOOL bDoCentroid = (D3DSPDM_MSAMPCENTROID & DclRegister);
                    if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_TEXCOORD,RegNum,PSTRREG_TEXTURE,RegNum,
                                                    (BYTE)((DclRegister & D3DSP_WRITEMASK_ALL)>>PSTR_COMPONENTMASK_SHIFT),bDoCentroid)))
                    {
                        DPFERR("CPSTrans::Initialize - Out of memory.");
                        EXIT_WITH_STATUS(E_FAIL);
                    }
                    }
                    break;
                }
                pToken += DclLength;
            }
            else
            {
                while (*pToken & (1L<<31)) pToken++; // parameter tokens
            }
            if (_DWordCount() > (int)dwDwordCodeSize)
            {
                DPFERR("PixelShader(%d tokens, %d expected): count error",_DWordCount(),dwDwordCodeSize);
                EXIT_WITH_STATUS(E_FAIL);
            }
        }
        pToken++; // step over END token
        m_cD3DInst++; // count the END token as an instruction (so debugger can break on it)
        if (_DWordCount() != (int)dwDwordCodeSize)
        {
            DPFERR("PixelShader(%d tokens, %d expected): count error",_DWordCount(),dwDwordCodeSize);
            EXIT_WITH_STATUS(E_FAIL);
        }

        // make copy of original shader
        m_pCode = (DWORD*)WarpPlatform::AllocateMemory(sizeof(DWORD)*dwDwordCodeSize);
        if (NULL == m_pCode)
        {
            EXIT_WITH_STATUS(E_OUTOFMEMORY);
        }
        memcpy( m_pCode, pCode, ByteCodeSize );

        // allocate instruction array
        m_pD3DPixelShaderInstructionArray = (D3DPixelShaderInstruction*)WarpPlatform::AllocateMemory(sizeof(D3DPixelShaderInstruction)*m_cD3DInst);
        if (NULL == m_pD3DPixelShaderInstructionArray)
            EXIT_WITH_STATUS(E_OUTOFMEMORY);

        memset( m_pD3DPixelShaderInstructionArray, 0x0, sizeof(D3DPixelShaderInstruction)*m_cD3DInst );

        m_pConstDefsF = (ConstDef_F*)WarpPlatform::AllocateMemory(sizeof(ConstDef_F)*m_cConstDefsF);
        if (NULL == m_pConstDefsF)
            EXIT_WITH_STATUS(E_OUTOFMEMORY);

        m_pConstDefsI = (ConstDef_I*)WarpPlatform::AllocateMemory(sizeof(ConstDef_I)*m_cConstDefsI);
        if (NULL == m_pConstDefsI)
            EXIT_WITH_STATUS(E_OUTOFMEMORY);

        m_pConstDefsB = (ConstDef_B*)WarpPlatform::AllocateMemory(sizeof(ConstDef_B)*m_cConstDefsB);
        if (NULL == m_pConstDefsB)
            EXIT_WITH_STATUS(E_OUTOFMEMORY);
    }

    // ------------------------------------------------------------------------
    //
    // Second pass through shader to:
    //      - produce a list of instructions, each one including opcodes,
    //        comments, and disassembled text for access by shader debuggers.
    //      - figure out the TSS # used (if any) by each instruction
    //      - figure out the max texture stage # used
    //      - figure out when the ref. pixel shader executor should
    //        queue writes up and when to flush the queue, in order to
    //        simulate co-issue.
    //      - figure out which texture coordinate sets get used (1x shader models)
    //      - process constant DEF instructions into a list that can be
    //        executed whenever SetPixelShader is done.
    //
    // ------------------------------------------------------------------------
    {
        DWORD* pToken = m_pCode;
        D3DPixelShaderInstruction* pInst = m_pD3DPixelShaderInstructionArray;
        D3DPixelShaderInstruction* pPrevious_NonTrivial_Inst = NULL;
        pToken++; // skip over version

        BOOL    bMinimizeReferencedTexCoords;

        if( (D3DPS_VERSION(1,3) >= *pCode) ||
            (D3DPS_VERSION(254,254) == *pCode ) )//legacy
        {
            bMinimizeReferencedTexCoords    = FALSE;
        }
        else
        {
            bMinimizeReferencedTexCoords    = TRUE;
        }

        UINT    CurrConstDefF = 0;
        UINT    CurrConstDefI = 0;
        UINT    CurrConstDefB = 0;

        while (*pToken != D3DPS_END())
        {
            pInst->ByteOffset = (BYTE*)pToken - (BYTE*)m_pCode;
            pInst->bPredicated = FALSE;

            switch( (*pToken) & D3DSI_OPCODE_MASK )
            {
            case D3DSIO_COMMENT:
                pInst->Opcode = *pToken;
                if(bKeepDebugInfo)
                {
                    pInst->pComment = (pToken+1);
                }
                pInst->CommentSize = ((*pToken) & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                pToken += (pInst->CommentSize+1);
                pInst++;
                continue;
            case D3DSIO_DEF:
                {
                    pInst->Opcode = *(pToken++);
                    pInst->DstParam = *pToken++;
                    pInst->DstParamCount++;

                    switch(D3DSI_GETREGTYPE_RESOLVING_CONSTANTS(pInst->DstParam))
                    {
                    case D3DSPR_CONST:
                        m_pConstDefsF[CurrConstDefF].RegNum = D3DSI_GETREGNUM_RESOLVING_CONSTANTS(pInst->DstParam);

                        // clamp constants on input to range of values in pixel shaders
                        for( UINT i = 0; i < 4; i++ )
                        {
                            m_pConstDefsF[CurrConstDefF].f[i] = MAX( fMin, MIN( fMax, *(FLOAT*)pToken));
                            pInst->fDefValues[i] = m_pConstDefsF[CurrConstDefF].f[i]; // even show debuggers clamped def values.
                            pToken++;
                        }
                        CurrConstDefF++;
                        break;
                    }
                    pInst++;
                    continue;
                }
            case D3DSIO_DEFI:
                {
                    pInst->Opcode = *(pToken++);
                    pInst->DstParam = *pToken++;
                    pInst->DstParamCount++;
                    switch(D3DSI_GETREGTYPE_RESOLVING_CONSTANTS(pInst->DstParam))
                    {
                    case D3DSPR_CONSTINT:
                        m_pConstDefsI[CurrConstDefI].RegNum = D3DSI_GETREGNUM(pInst->DstParam);

                        for( UINT i = 0; i < 4; i++ )
                        {
                            m_pConstDefsI[CurrConstDefI].i[i] = *(INT*)pToken;
                            pInst->iDefValues[i] = m_pConstDefsI[CurrConstDefI].i[i];
                            pToken++;
                        }
                        CurrConstDefI++;
                        break;
                    }
                    pInst++;
                    continue;
                }
            case D3DSIO_DEFB:
                {
                    pInst->Opcode = *(pToken++);
                    pInst->DstParam = *pToken++;
                    pInst->DstParamCount++;
                    switch(D3DSI_GETREGTYPE(pInst->DstParam))
                    {
                    case D3DSPR_CONSTBOOL:
                        m_pConstDefsB[CurrConstDefB].RegNum = D3DSI_GETREGNUM(pInst->DstParam);
                        pInst->bDefValue = *(BOOL*)pToken;
                        m_pConstDefsB[CurrConstDefB].b = 0!=*(BOOL*)pToken;
                        pToken++;
                        CurrConstDefB++;
                        break;
                    }
                    pInst++;
                    continue;
                }
            case D3DSIO_NOP:
                pInst->Opcode = *pToken++;
                pInst++;
                continue;
            }

            // Is instruction predicated?
            pInst->bPredicated = D3DSHADER_INSTRUCTION_PREDICATED & *pToken;

            // get next instruction and parameters
            pInst->Opcode = *pToken++;

            switch(D3DSI_OPCODE_MASK & pInst->Opcode)
            {
            case D3DSIO_DCL:
                pInst->DclInfoToken = *pToken++;
                break;
            }
            if (*pToken & (1L<<31))
            {
                switch(D3DSI_OPCODE_MASK & pInst->Opcode)
                {
                case D3DSIO_CALL:
                case D3DSIO_REP:
                case D3DSIO_LOOP:
                case D3DSIO_CALLNZ:
                case D3DSIO_IF:
                case D3DSIO_IFC:
                case D3DSIO_BREAKC:
                case D3DSIO_BREAKP:
                case D3DSIO_LABEL:
                    // No dst param, only src params.
                    // Also, no predication:
                    Assert(!pInst->bPredicated);        // CPSTrans::Initialize - Flow control ops can't be predicated
                    break;
                default:
                    pInst->DstParam = *pToken++;
                    pInst->DstParamCount++;
                    break;
                }
            }
            if(pInst->bPredicated)
            {
                Assert(*pToken & (1L<<31));             // CPSTrans::Initialize - Expected source predicate token.
                pInst->SrcPredicateToken = *pToken++;
            }

            pInst->SrcParamCount = 0;
            while (*pToken & (1L<<31))
            {
                pInst->SrcParam[pInst->SrcParamCount] = *pToken++;
                if( D3DSHADER_ADDRMODE_RELATIVE == D3DSI_GETADDRESSMODE(pInst->SrcParam[pInst->SrcParamCount]) )
                {
                    Assert(*pToken & (1L<<31));         // CPSTrans::Initialize - Expected relative address token
                    pInst->SrcParamRelAddr[pInst->SrcParamCount] = *pToken++;
                }
                if( Version < D3DPS_VERSION(2,0) )
                {
                    // since ps_1_x doesn't have dcl's, figure track if diffuse/specular are being used
                    // by if they are ever a source parameter.
                    if( D3DSI_GETREGTYPE(pInst->SrcParam[pInst->SrcParamCount]) == D3DSPR_INPUT )
                    {
                        UINT RegNum = D3DSI_GETREGNUM(pInst->SrcParam[pInst->SrcParamCount]);
                        if(!m_InputRegDclInfo.IsRegDeclared(PSTRREG_INPUT,RegNum,PSTR_COMPONENTMASK_ALL))
                        {
                            if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_COLOR,RegNum,
                                                                  PSTRREG_INPUT,RegNum,
                                                                  PSTR_COMPONENTMASK_ALL,FALSE)))
                            {
                                DPFERR("CPSTrans::Initialize - Out of memory.");
                                EXIT_WITH_STATUS(E_FAIL);
                            }
                        }
                    }
                }
                pInst->SrcParamCount++;
            }

            // process TEX ops
            //
            BOOL bLegacyTexOp = FALSE;
            switch (pInst->Opcode & D3DSI_OPCODE_MASK)
            {
            default: break;
            case D3DSIO_TEXBEM_LEGACY:
            case D3DSIO_TEXBEML_LEGACY:
                bLegacyTexOp = TRUE;
            case D3DSIO_TEXCOORD:
            case D3DSIO_TEXKILL:
            case D3DSIO_TEX:
            case D3DSIO_TEXBEM:
            case D3DSIO_TEXBEML:
            case D3DSIO_TEXLDD:
            case D3DSIO_TEXLDL:
            case D3DSIO_TEXREG2AR:
            case D3DSIO_TEXREG2GB:
            case D3DSIO_TEXM3x2PAD:
            case D3DSIO_TEXM3x2TEX:
            case D3DSIO_TEXM3x3PAD:
            case D3DSIO_TEXM3x3TEX:
            case D3DSIO_TEXM3x3SPEC:
            case D3DSIO_TEXM3x3VSPEC:
            case D3DSIO_TEXM3x2DEPTH:
            case D3DSIO_TEXDP3:
            case D3DSIO_TEXREG2RGB:
            case D3DSIO_TEXDEPTH:
            case D3DSIO_TEXDP3TEX:
            case D3DSIO_TEXM3x3:
                pInst->bTexOp = TRUE;
                break;
            }
            if (pInst->bTexOp)
            {
                // update stage count and assign ptr to TSS for this op
                if (bLegacyTexOp)
                {
                    m_cActiveTextureStages =
                        max(m_cActiveTextureStages,D3DSI_GETREGNUM(pInst->DstParam)+1);
                    pInst->uiTSSNum = D3DSI_GETREGNUM(pInst->DstParam)-1;

                    UINT CoordSet = D3DSI_GETREGNUM(pInst->DstParam);
                    Assert(32 > CoordSet);     // CPSTrans::Initialize - Unexpectedly large texture stage number!
                    IgnoreD3DTTFF_PROJECTED &= ~(1<<CoordSet);
                    TexCoordClamp1x &= ~(1<<CoordSet);
                }
                else
                {
                    UINT Stage = 0;
                    BOOL bStageUsed = TRUE;

                    switch(pInst->Opcode & D3DSI_OPCODE_MASK)
                    {
                    case D3DSIO_TEXBEM:
                    case D3DSIO_TEXBEML:
                        {
                            Stage = D3DSI_GETREGNUM(pInst->DstParam);
                            UINT CoordSet = Stage;
                            Assert(32 > CoordSet);     // CPSTrans::Initialize - Unexpectedly large texture stage number!
                            if(!m_InputRegDclInfo.IsRegDeclared(PSTRREG_TEXTURE,CoordSet,PSTR_COMPONENTMASK_ALL))
                            {
                                if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_TEXCOORD,CoordSet,
                                          PSTRREG_TEXTURE,CoordSet,
                                          PSTR_COMPONENTMASK_ALL,FALSE)))
                                {
                                    DPFERR("CPSTrans::Initialize - Out of memory.");
                                    EXIT_WITH_STATUS(E_FAIL);
                                }
                                IgnoreD3DTTFF_PROJECTED &= ~(1<<CoordSet);
                                TexCoordClamp1x &= ~(1<<CoordSet);
                            }
                        }
                        break;
                    case D3DSIO_TEXDEPTH:
                        bStageUsed = FALSE; // texture not used
                        break;
                    case D3DSIO_TEXCOORD:
                        if( bMinimizeReferencedTexCoords )
                            bStageUsed = FALSE; // texture not used. (coordinates may be used though)
                        else
                            Stage = D3DSI_GETREGNUM(pInst->DstParam); // note Stage used after switch block

                        if( D3DSPR_TEXTURE == D3DSI_GETREGTYPE(pInst->DstParam) ) // ps_1_1-1_3 can do this
                        {
                            UINT CoordSet = D3DSI_GETREGNUM(pInst->DstParam);
                            Assert(32 > CoordSet);     // CPSTrans::Initialize - Unexpectedly large texture stage number!
                            if(!m_InputRegDclInfo.IsRegDeclared(PSTRREG_TEXTURE,CoordSet,PSTR_COMPONENTMASK_ALL))
                            {
                                if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_TEXCOORD,CoordSet,
                                          PSTRREG_TEXTURE,CoordSet,
                                          PSTR_COMPONENTMASK_ALL,FALSE)))
                                {
                                    DPFERR("CPSTrans::Initialize - Out of memory.");
                                    EXIT_WITH_STATUS(E_FAIL);
                                }
                                IgnoreD3DTTFF_PROJECTED |= (1<<CoordSet);
                                TexCoordClamp1x |= (1<<CoordSet);
                            }
                        }
                        else if( (pInst->SrcParamCount) &&
                                 (D3DSPR_TEXTURE == D3DSI_GETREGTYPE(pInst->SrcParam[0]))) // ps_1_4+
                        {
                            UINT CoordSet = D3DSI_GETREGNUM(pInst->SrcParam[0]);
                            Assert(32 > CoordSet);     // CPSTrans::Initialize - Unexpectedly large texture stage number!
                            if(!m_InputRegDclInfo.IsRegDeclared(PSTRREG_TEXTURE,CoordSet,PSTR_COMPONENTMASK_ALL))
                            {
                                if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_TEXCOORD,CoordSet,
                                          PSTRREG_TEXTURE,CoordSet,
                                          PSTR_COMPONENTMASK_ALL,FALSE)))
                                {
                                    DPFERR("CPSTrans::Initialize - Out of memory.");
                                    EXIT_WITH_STATUS(E_FAIL);
                                }
                                IgnoreD3DTTFF_PROJECTED &= ~(1<<CoordSet);
                                TexCoordClamp1x &= ~(1<<CoordSet);
                            }
                        }
                        break;
                    case D3DSIO_TEXKILL:
                        if( bMinimizeReferencedTexCoords )
                            bStageUsed = FALSE; // texture not used. (coordinates may be used though)
                        else
                            Stage = D3DSI_GETREGNUM(pInst->DstParam); // note Stage used after switch block

                        if( D3DPS_VERSION(2,0) > Version ) // in ps_2_0+, dcl indicates texcood use
                        {
                            if( D3DSPR_TEXTURE == D3DSI_GETREGTYPE(pInst->DstParam) )
                            {
                                UINT CoordSet = D3DSI_GETREGNUM(pInst->DstParam);
                                Assert(32 > CoordSet);     // CPSTrans::Initialize - Unexpectedly large texture stage number!
                                if(!m_InputRegDclInfo.IsRegDeclared(PSTRREG_TEXTURE,CoordSet,PSTR_COMPONENTMASK_ALL))
                                {
                                    if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_TEXCOORD,CoordSet,
                                            PSTRREG_TEXTURE,CoordSet,
                                            PSTR_COMPONENTMASK_ALL,FALSE)))
                                    {
                                        DPFERR("CPSTrans::Initialize - Out of memory.");
                                        EXIT_WITH_STATUS(E_FAIL);
                                    }
                                    IgnoreD3DTTFF_PROJECTED &= ~(1<<CoordSet);
                                    TexCoordClamp1x &= ~(1<<CoordSet);
                                }
                            }
                        }
                        break;
                    case D3DSIO_TEX:
                        {
                            Stage = (2 <= pInst->SrcParamCount) ? D3DSI_GETREGNUM(pInst->SrcParam[1]) : // ps_2_0+ "texld"
                                                                  D3DSI_GETREGNUM(pInst->DstParam);     // ps_1_x

                            if( 0 == pInst->SrcParamCount ) // ps_1_3 and lower, as well as 254_254 (legacy)
                            {
                                UINT CoordSet = Stage;
                                Assert(32 > CoordSet);     // CPSTrans::Initialize - Unexpectedly large texture stage number!
                                if(!m_InputRegDclInfo.IsRegDeclared(PSTRREG_TEXTURE,CoordSet,PSTR_COMPONENTMASK_ALL))
                                {
                                    if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_TEXCOORD,CoordSet,
                                            PSTRREG_TEXTURE,CoordSet,
                                            PSTR_COMPONENTMASK_ALL,FALSE)))
                                    {
                                        DPFERR("CPSTrans::Initialize - Out of memory.");
                                        EXIT_WITH_STATUS(E_FAIL);
                                    }
                                    IgnoreD3DTTFF_PROJECTED &= ~(1<<CoordSet);
                                    TexCoordClamp1x &= ~(1<<CoordSet);
                                }
                            }
                            else if( (D3DPS_VERSION(2,0)>Version) && // which texcoord.... (in ps_2_0 dcl indicates texcoord use)
                                     (D3DSPR_TEXTURE == D3DSI_GETREGTYPE(pInst->SrcParam[0])) )
                            {
                                UINT CoordSet = D3DSI_GETREGNUM(pInst->SrcParam[0]);
                                Assert(32 > CoordSet);      // CPSTrans::Initialize - Unexpectedly large texture stage number!
                                if(!m_InputRegDclInfo.IsRegDeclared(PSTRREG_TEXTURE,CoordSet,PSTR_COMPONENTMASK_ALL))
                                {
                                    if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_TEXCOORD,CoordSet,
                                            PSTRREG_TEXTURE,CoordSet,
                                            PSTR_COMPONENTMASK_ALL,FALSE)))
                                    {
                                        DPFERR("CPSTrans::Initialize - Out of memory.");
                                        EXIT_WITH_STATUS(E_FAIL);
                                    }
                                    IgnoreD3DTTFF_PROJECTED &= ~(1<<CoordSet);
                                    TexCoordClamp1x &= ~(1<<CoordSet);
                                }
                            }
                            // note it's possible to not reference any texture coordinates (temp register as input coord)
                        }
                        break;
                    case D3DSIO_TEXLDD:
                        Stage = D3DSI_GETREGNUM(pInst->SrcParam[1]);
                        break;
                    case D3DSIO_TEXLDL:
                        Stage = D3DSI_GETREGNUM(pInst->SrcParam[1]);
                        break;
                    default:
                        if( D3DPS_VERSION(2,0) > Version)
                        {   // other various ps_1_x tex ops... state and coordset are both == dst reg#
                            Stage = D3DSI_GETREGNUM(pInst->DstParam);
                            UINT CoordSet = Stage;
                            Assert(32 > CoordSet);          // CPSTrans::Initialize - Unexpectedly large texture stage number!
                            if(!m_InputRegDclInfo.IsRegDeclared(PSTRREG_TEXTURE,CoordSet,PSTR_COMPONENTMASK_ALL))
                            {
                                if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_TEXCOORD,CoordSet,
                                          PSTRREG_TEXTURE,CoordSet,
                                          PSTR_COMPONENTMASK_ALL,FALSE)))
                                {
                                    DPFERR("CPSTrans::Initialize - Out of memory.");
                                    EXIT_WITH_STATUS(E_FAIL);
                                }
                                IgnoreD3DTTFF_PROJECTED |= 1<<CoordSet;
                                TexCoordClamp1x &= ~(1<<CoordSet);
                            }
                        }
                        else
                        {
                            NO_DEFAULT;     // CPSTrans::Initialize - Unexpected tex op.
                        }
                        break;
                    }

                    if( bStageUsed )
                    {
                        if( ((D3DPS_VERSION(2,0) > Version) || (D3DPS_VERSION(254,254) == Version) ) || // legacy
                            (m_SamplerRegDcl[Stage] != D3DSTT_UNKNOWN))  // ps_2_0 texture sampler has a corresponding dcl statement

                        {
                            m_cActiveTextureStages = max(m_cActiveTextureStages,Stage+1);
                            pInst->uiTSSNum = Stage;
                        }
                        else
                        {
                            NO_DEFAULT;     // CPSTrans::Initialize - Sampler stage used without being declared!
                        }
                    }
                }
            }

            if( pPrevious_NonTrivial_Inst )
            {
                // Queue write of last instruction if the current instruction has the
                // COISSUE flag.
                if( pInst->Opcode & D3DSI_COISSUE )
                {
                    pPrevious_NonTrivial_Inst->bQueueWrite = TRUE;
                }

                // Flush writes after the previous instruction if it had the COISSUE
                // flag and the current instruction doesn't have it.
                if( !(pInst->Opcode & D3DSI_COISSUE) && (pPrevious_NonTrivial_Inst->Opcode & D3DSI_COISSUE) )
                {
                    pPrevious_NonTrivial_Inst->bFlushQueue = TRUE;
                }
            }

            pPrevious_NonTrivial_Inst = pInst;

            pInst++;
        }
        // Note the end token.
        pInst->Opcode = D3DPS_END();
        pInst->ByteOffset = pInst->ByteOffset = (BYTE*)pToken - (BYTE*)m_pCode;
        if(pPrevious_NonTrivial_Inst && (pPrevious_NonTrivial_Inst->Opcode & D3DSI_COISSUE))
        {
            pPrevious_NonTrivial_Inst->bFlushQueue = TRUE;
        }

        if( !bMinimizeReferencedTexCoords )
        {
           for(UINT CoordSet = 0; CoordSet < m_cActiveTextureStages; CoordSet++)
           {
                if(!m_InputRegDclInfo.IsRegDeclared(PSTRREG_TEXTURE,CoordSet,PSTR_COMPONENTMASK_ALL))
                {
                    if(FAILED(m_InputRegDclInfo.AddNewDcl(D3DDECLUSAGE_TEXCOORD,CoordSet,
                                PSTRREG_TEXTURE,CoordSet,
                                PSTR_COMPONENTMASK_ALL,FALSE)))
                    {
                        DPFERR("CPSTrans::Initialize - Out of memory.");
                        EXIT_WITH_STATUS(E_FAIL);
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    //
    // Third pass through the shader (through the list of instructions made
    // in the last pass) to translate instructions into a more basic ("RISC")
    // instruction set for the refrast executor.
    //
    // ------------------------------------------------------------------------
    // try
    {
        CWorkerData WorkerData;
        m_pWorkerData = &WorkerData;

        BYTE    ComponentReplicate[4] = {PSTR_REPLICATERED, PSTR_REPLICATEGREEN, PSTR_REPLICATEBLUE, PSTR_REPLICATEALPHA};
        BYTE    ComponentMask[4]    = {PSTR_COMPONENTMASK_0, PSTR_COMPONENTMASK_1, PSTR_COMPONENTMASK_2, PSTR_COMPONENTMASK_3};
        BOOL    bQueuedWrite        = FALSE; // write has been queued (for staging results when simulating coissue)
        PSTRRegister QueuedWriteDstReg;
        BYTE    QueuedWriteDstWriteMask = PSTR_COMPONENTMASK_ALL;
        UINT    i;
        BOOL    bDepthOutput     = FALSE;

        PSTRRegister ZeroReg; ZeroReg._Set(PSTRREG_ZERO,0);
        PSTRRegister OneReg;  OneReg._Set(PSTRREG_ONE,0);
        PSTRRegister InternalLoopCounterReg; InternalLoopCounterReg._Set(PSTRREG_INTERNALLOOPCOUNTER,0);
        PSTRRegister LoopCounterReg; LoopCounterReg._Set(PSTRREG_LOOPCOUNTER,0);

        // destination parameter controls
        PSTRRegister    DstReg;
        BYTE            DstWriteMask;       // per-component write mask

        // source parameter controls
        PSTRRegister SrcReg[PSTR_MAX_NUMSRCPARAMS];

        DWORD   Version = *pCode;

        _EnterQuadPixelLoop();

        // First emit instructions to evaluate necessary interpolated attributes
        CInputRegDclInfo::INPUT_DCL_NODE* pDclInfo = m_InputRegDclInfo.GetInputDclList();
        while(pDclInfo)
        {
            if( (D3DPS_VERSION(3,0) <= Version) &&
                (D3DPS_VERSION(254,254) != Version)) // Legacy
            {
                _NewPSInst(PSTRINST_EVAL);
                _InstParam(PSTRINST_EVAL).DstReg._Set(pDclInfo->PSTRRegType,pDclInfo->RegNum);
                _InstParam(PSTRINST_EVAL).RDAttrBaseIndex           = 4*pDclInfo->RegNum;
                _InstParam(PSTRINST_EVAL).bIgnoreD3DTTFF_PROJECTED  = TRUE;
                _InstParam(PSTRINST_EVAL).WriteMask                 = pDclInfo->WriteMask;
                _InstParam(PSTRINST_EVAL).bSampleAtCentroidWhenMultisampling = pDclInfo->bSampleAtCentroidWhenMultisampling;
                _InstParam(PSTRINST_EVAL).bClamp                    = FALSE;
                _InstParam(PSTRINST_EVAL).Usage                     = pDclInfo->Usage;
                _InstParam(PSTRINST_EVAL).UsageIndex                = pDclInfo->Index;
            }
            else 
            {
                UINT AttrBaseIndex = RDATTR_TEXTURE0;
                if(pDclInfo->PSTRRegType == PSTRREG_INPUT)
                {
                    AttrBaseIndex = RDATTR_DIFFUSE;
                    
                    if( pDclInfo->RegNum>2 )
                    {
                        C_ASSERT((RDATTR_DIFFUSE+4) == RDATTR_SPECULAR);
                        WarpError("CPSTrans::Initialize - Unexpected input register number");
                    }
                }
                
                BOOL bClamp;
                BOOL bIgnoreD3DTTFF_PROJECTED;
                if((D3DPS_VERSION(2,0)>Version)||(Version==D3DPS_VERSION(254,254)))
                    bClamp = (TexCoordClamp1x >> pDclInfo->RegNum) & 0x1;
                else
                    bClamp = FALSE;
                if(D3DPS_VERSION(1,4)>Version||(Version==D3DPS_VERSION(254,254)))
                    bIgnoreD3DTTFF_PROJECTED = (IgnoreD3DTTFF_PROJECTED >> pDclInfo->RegNum) & 0x1;
                else
                    bIgnoreD3DTTFF_PROJECTED = TRUE;

                _NewPSInst(PSTRINST_EVAL);
                _InstParam(PSTRINST_EVAL).DstReg._Set(pDclInfo->PSTRRegType,pDclInfo->RegNum);
                _InstParam(PSTRINST_EVAL).RDAttrBaseIndex           = AttrBaseIndex + 4*pDclInfo->RegNum;
                _InstParam(PSTRINST_EVAL).bIgnoreD3DTTFF_PROJECTED  = bIgnoreD3DTTFF_PROJECTED;
                _InstParam(PSTRINST_EVAL).WriteMask                 = pDclInfo->WriteMask;
                _InstParam(PSTRINST_EVAL).bSampleAtCentroidWhenMultisampling = pDclInfo->bSampleAtCentroidWhenMultisampling;
                _InstParam(PSTRINST_EVAL).bClamp                    = bClamp;
                _InstParam(PSTRINST_EVAL).Usage                     = pDclInfo->Usage;
                _InstParam(PSTRINST_EVAL).UsageIndex                = pDclInfo->Index;
            }

            pDclInfo = pDclInfo->pNext;
        }

        if( bInsertD3DPSInstMarkers )
            _LeaveQuadPixelLoop();

        for (m_pWorkerData->D3DInstID = 0; m_pWorkerData->D3DInstID < m_cD3DInst; m_pWorkerData->D3DInstID++)
        {
            m_pWorkerData->pInst = m_pD3DPixelShaderInstructionArray + m_pWorkerData->D3DInstID;
            DWORD   Opcode = m_pWorkerData->pInst->Opcode & D3DSI_OPCODE_MASK;
            DWORD   OpcodeSpecificControl = m_pWorkerData->pInst->Opcode & D3DSP_OPCODESPECIFICCONTROL_MASK;
            BYTE    SrcSwizzle[PSTR_MAX_NUMSRCPARAMS];
            BYTE    SourceReadMasks[PSTR_MAX_NUMSRCPARAMS];
            BYTE    SourceReadMasksAfterSwizzle[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bForceNeg1To1Clamp[PSTR_MAX_NUMSRCPARAMS];
            BYTE    ProjComponent[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bProjOnEval[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bSwizzleOnEval[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bEmitSrcMod[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bEmitAbs[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bEmitSwizzle[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bSrcNegate[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bSrcNOT[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bSrcBias[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bSrcTimes2[PSTR_MAX_NUMSRCPARAMS];
            BOOL    bSrcComplement[PSTR_MAX_NUMSRCPARAMS];
            memset( bForceNeg1To1Clamp,0,sizeof(bForceNeg1To1Clamp) );
            memset( ProjComponent,0,sizeof(ProjComponent) );
            memset( bProjOnEval,0,sizeof(bProjOnEval) );
            memset( bSwizzleOnEval,0,sizeof(bSwizzleOnEval) );
            memset( bEmitSrcMod,0,sizeof(bEmitSrcMod) );
            memset( bEmitAbs,0,sizeof(bEmitAbs) );
            memset( bEmitSwizzle,0,sizeof(bEmitSwizzle) );
            memset( bSrcNegate,0,sizeof(bSrcNegate) );
            memset( bSrcNOT,0,sizeof(bSrcNOT) );
            memset( bSrcBias,0,sizeof(bSrcBias) );
            memset( bSrcTimes2,0,sizeof(bSrcTimes2) );
            memset( bSrcComplement,0,sizeof(bSrcComplement) );

            // Figure out how instruction is predicated (if at all)
            m_pWorkerData->ForceNoPredication.bInvertPredicate = FALSE;
            m_pWorkerData->ForceNoPredication.PredicateSwizzle = PSTR_NOSWIZZLE;
            m_pWorkerData->ForceNoPredication.PredicateReg._Set(PSTRREG_PREDICATETRUE,0);

            if( m_pWorkerData->pInst->bPredicated )
            {
                m_pWorkerData->PredicateInfo.bInvertPredicate = (D3DSPSM_NOT == (D3DSP_SRCMOD_MASK & m_pWorkerData->pInst->SrcPredicateToken));
                m_pWorkerData->PredicateInfo.PredicateSwizzle = (BYTE)((D3DSP_SWIZZLE_MASK & m_pWorkerData->pInst->SrcPredicateToken) >> D3DSP_SWIZZLE_SHIFT);
                Assert(D3DSPR_PREDICATE == D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcPredicateToken));  // CPSTrans::Initialize - unexpected predicate register type.
                m_pWorkerData->PredicateInfo.PredicateReg._Set(PSTRREG_PREDICATE,D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcPredicateToken));
            }
            else
            {
                m_pWorkerData->PredicateInfo = m_pWorkerData->ForceNoPredication;
            }

            _EnterQuadPixelLoop();

            switch( Opcode )
            {
            /*case D3DSIO_LABEL:
                if( bInsertD3DPSInstMarkers )
                {
                    _NoteInstructionEventNOBREAK();
                }

                _LeaveQuadPixelLoop();
                // Flag the instruction after the label as the target.
                m_LabelTracker.AddLabel(D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[0]),
                                        _GetNextInstOffset(),
                                        _GetNextPSTRInstID(),
                                        m_pWorkerData->pPSTRInstBuffer);
                _EnterQuadPixelLoop();
                continue;*/ // blakep, let this get parsed
            case D3DSIO_NOP:
                if( bInsertD3DPSInstMarkers )
                {
                    _NoteInstructionEvent();
                }
                continue;
            case D3DSIO_DEF:
            case D3DSIO_DEFI:
            case D3DSIO_DEFB:
                // nothing to do -> DEF has already been processed out and is not an true instruction
                // falling through
            case D3DSIO_COMMENT:
            case D3DSIO_PHASE:
            case D3DSIO_DCL:
                if( bInsertD3DPSInstMarkers )
                {
                    _NoteInstructionEventNOBREAK();
                }
                continue;
            case D3DSIO_END:
                if( bInsertD3DPSInstMarkers )
                {
                    _NoteInstructionEvent();
                    m_EndOffset = _GetOffset();
                }
                continue;
            }

            if( bInsertD3DPSInstMarkers )
            {
                switch( Opcode )
                {
                case D3DSIO_ELSE:
                case D3DSIO_ENDIF:
                    _NoteInstructionEventNOBREAK();
                    break;
                default:
                    _NoteInstructionEvent();
                    break;
                }
            }

            // do some preliminary setup for this instruction

            UINT RegNum = D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);
            switch (D3DSI_GETREGTYPE(m_pWorkerData->pInst->DstParam))
            {
            case D3DSPR_TEXTURE:
                DstReg._Set(PSTRREG_TEXTURE, RegNum); break;
            case D3DSPR_TEMP:
                DstReg._Set(PSTRREG_TEMP, RegNum); break;
            case D3DSPR_COLOROUT:
                DstReg._Set(PSTRREG_COLOROUT, RegNum); 
                if( D3DSIO_TEXKILL != Opcode )
                {
                    m_ColorOutPresentMask |= 1<<RegNum;
                }
                break;
            case D3DSPR_PREDICATE:
                DstReg._Set(PSTRREG_PREDICATE, RegNum);
                Assert(0 == RegNum);       // CPSTrans::Initialize - Regnum must be 0 for predicate register.
                break;
            case D3DSPR_DEPTHOUT:
                DstReg._Set(PSTRREG_DEPTHOUT, RegNum);
                Assert(0 == RegNum);       // CPSTrans::Initialize - Regnum must be 0 for depth output.
                bDepthOutput = TRUE;
                break;
            case D3DSPR_INPUT:
                if( D3DSIO_TEXKILL == Opcode )
                {
                    DstReg._Set(PSTRREG_INPUT, RegNum); 
                    break;
                }
            default:
                NO_DEFAULT;     // CPSTrans::Initialize - Unexpected destination register type.
                break;
            }

            DstWriteMask = (BYTE)(m_pWorkerData->pInst->DstParam ? (m_pWorkerData->pInst->DstParam & D3DSP_WRITEMASK_ALL) >> PSTR_COMPONENTMASK_SHIFT 
                                           : PSTR_COMPONENTMASK_ALL); // for ops with no destination parameter (may be implied),
                                                                     // default write mask to FULL. i.e. D3DSIO_IFC, D3DSIO_BREAKC

            if( m_pWorkerData->pInst->bQueueWrite )
            {
                bQueuedWrite = TRUE;
                QueuedWriteDstReg = DstReg;
                QueuedWriteDstWriteMask = DstWriteMask;
                DstReg._Set(PSTRREG_QUEUEDWRITE,0);
            }

            CalculateSourceReadMasks(m_pWorkerData->pInst, SourceReadMasks, FALSE, m_SamplerRegDcl, Version);
            CalculateSourceReadMasks(m_pWorkerData->pInst, SourceReadMasksAfterSwizzle, TRUE, m_SamplerRegDcl, Version);
            for (i=0; i < m_pWorkerData->pInst->SrcParamCount; i++)
            {
                RegNum = D3DSI_GETREGNUM_RESOLVING_CONSTANTS(m_pWorkerData->pInst->SrcParam[i]);
                switch (D3DSI_GETREGTYPE_RESOLVING_CONSTANTS(m_pWorkerData->pInst->SrcParam[i]))
                {
                case D3DSPR_TEMP:
                    SrcReg[i]._Set(PSTRREG_TEMP, RegNum); break;
                case D3DSPR_TEXTURE:
                    SrcReg[i]._Set(PSTRREG_TEXTURE, RegNum);
                    Assert((((RegNum < PSTR_MAX_NUMTEXTUREREG) &&
                            m_InputRegDclInfo.IsRegDeclared(PSTRREG_TEXTURE,RegNum,SourceReadMasks[i]))
                            ||
                            (D3DPS_VERSION(2,0) > Version)
                            ||
                            (D3DPS_VERSION(254,254) == Version)));      // CPSTrans::Initialize - Component(s) of t# register read without being declared!
                    break;
                case D3DSPR_INPUT:
                    if( D3DSHADER_ADDRMODE_RELATIVE == D3DSI_GETADDRESSMODE(m_pWorkerData->pInst->SrcParam[i]) )
                    {
                        PSTR_REGISTER_TYPE RelAddrRegType = PSTRREG_UNINITIALIZED_TYPE;
                        UINT RelAddrRegNum = UINT_MAX;
                        switch(D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcParamRelAddr[i]))
                        {
                        case D3DSPR_LOOP:
                            RelAddrRegType = PSTRREG_LOOPCOUNTER;
                            RelAddrRegNum = 0;
                            Assert(0 == D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParamRelAddr[i]));     // CPSTrans::Initialize - Unexpected relative address register #
                            break;
                        default:
                            NO_DEFAULT;     // CPSTrans::Initialize - Unexpected relative addressing register type.
                            break;
                        }
                        SrcReg[i]._SetRelAddr(PSTRREG_INPUT, RegNum,RelAddrRegType,RelAddrRegNum,
                                              (BYTE)_SelectorFromSwizzle(D3DSI_GETSWIZZLE(m_pWorkerData->pInst->SrcParamRelAddr[i])>>D3DSP_SWIZZLE_SHIFT));
                    }
                    else
                    {
                        SrcReg[i]._Set(PSTRREG_INPUT, RegNum);
                        Assert((D3DPS_VERSION(2,0) > Version) ||
                                ((RegNum < PSTR_MAX_NUMINPUTREG) &&
                                m_InputRegDclInfo.IsRegDeclared(PSTRREG_INPUT,RegNum,SourceReadMasks[i])));     // CPSTrans::Initialize - Component(s) of v# register read without being declared!
                    }
                    break;
                case D3DSPR_CONST:
                    SrcReg[i]._Set(PSTRREG_CONST, RegNum);
                    if( D3DPS_VERSION(2,0) > Version )
                    {
                        // Force a [-1,1] clamp after applying modifier (for constants only)
                        // This overrides the the standard [-PixelShader1xMaxValue,PixelShader1xMaxValue] clamp.
                        // An IHV that supports PixelShader1xMaxValue > 1 forgot to do this for constants.
                        bForceNeg1To1Clamp[i] = TRUE;
                    }
                    break;
                case D3DSPR_CONSTINT:
                    SrcReg[i]._Set(PSTRREG_CONSTINT, RegNum);
                    break;
                case D3DSPR_CONSTBOOL:
                    SrcReg[i]._Set(PSTRREG_CONSTBOOL, RegNum);
                    break;
                case D3DSPR_MISCTYPE:
                    switch(RegNum)
                    {
                    case D3DSMO_POSITION:
                        SrcReg[i]._Set(PSTRREG_POSITION, 0);
                        break;
                    case D3DSMO_FACE:
                        SrcReg[i]._Set(PSTRREG_FACE, 0);
                        break;
                    }
                    break;
                case D3DSPR_SAMPLER:
                    // do nothing.  this parameter merely provides a sampler stage # (register number),
                    //              as well as swizzle (although the swizzle is different from usual in that it
                    //              occurs on the data AFTER the texture lookup result.)
                    break;
                case D3DSPR_LOOP:
                case D3DSPR_LABEL:
                    break;
                case D3DSPR_PREDICATE:
                    SrcReg[i]._Set(PSTRREG_PREDICATE,RegNum);
                    break;
                default:
                    NO_DEFAULT;     // CPSTrans::Initialize - Unexpected source register type.
                    break;
                }

                if( (D3DSPSM_DZ == (m_pWorkerData->pInst->SrcParam[i] & D3DSP_SRCMOD_MASK)) ||
                    (D3DSPSM_DW == (m_pWorkerData->pInst->SrcParam[i] & D3DSP_SRCMOD_MASK)) )
                {
                    Assert( D3DPS_VERSION(1,4) == Version);     // CPSTrans::Initialize - _dz/_dw can only be used on ps_1_4.

                    // Note that both _dz and _dw are dividing by (0 based) 2nd component.  Runtime validator only lets through
                    // _dw with a .xyw source swizzle, so _dw ends up being the same as _dz (for now).

                    ProjComponent[i] = PSTR_COMPONENTMASK_2;
                    bProjOnEval[i] = TRUE;
                }
                else
                {
                    bEmitSrcMod[i] = TRUE;

                    switch (m_pWorkerData->pInst->SrcParam[i] & D3DSP_SRCMOD_MASK)
                    {
                    default:
                    case D3DSPSM_NONE:
                        if( !bForceNeg1To1Clamp[i] )
                            bEmitSrcMod[i] = FALSE;
                        break;
                    case D3DSPSM_NEG:
                        bSrcNegate[i]   = TRUE; // negate is not part of source modifier
                        if( !bForceNeg1To1Clamp[i] )
                            bEmitSrcMod[i] = FALSE;
                        break;
                    case D3DSPSM_NOT:
                        bSrcNOT[i]   = TRUE; // negate is not part of source modifier
                        bEmitSrcMod[i] = FALSE;
                        break;
                    case D3DSPSM_BIASNEG:
						bSrcNegate[i] = TRUE;
                    case D3DSPSM_BIAS:
                        bSrcBias[i]         = TRUE;
                        break;
                    case D3DSPSM_SIGNNEG:           // negative _bx2
                        bSrcNegate[i]       = TRUE; // negate is not part of source modifier
                    case D3DSPSM_SIGN:              // _bx2
                        bSrcBias[i]         = TRUE;
                        bSrcTimes2[i]       = TRUE;
                        break;
                    case D3DSPSM_COMP:
                        bSrcComplement[i]   = TRUE;
                        break;
                    case D3DSPSM_X2NEG:
                        bSrcNegate[i]       = TRUE;
                    case D3DSPSM_X2:
                        bSrcTimes2[i]       = TRUE;
                        break;
                    case D3DSPSM_ABSNEG:
                        bSrcNegate[i]       = TRUE; // negate is not part of source modifier
                    case D3DSPSM_ABS:
                        bEmitSrcMod[i]      = FALSE;
                        bEmitAbs[i]         = TRUE;
                        break;
                    }

                    Assert(!(bSrcComplement[i] && (bSrcTimes2[i]||bSrcBias[i]||bSrcNegate[i])));        // CPSTrans::Initialize - Complement cannot be combined with other modifiers.
                }

                SrcSwizzle[i] = (BYTE)((m_pWorkerData->pInst->SrcParam[i] & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT);
                bEmitSwizzle[i] = (D3DSP_NOSWIZZLE != (m_pWorkerData->pInst->SrcParam[i] & D3DSP_SWIZZLE_MASK));

                if( bEmitSwizzle[i] &&
                 (D3DSPR_SAMPLER == (D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcParam[i]))) )
                {
                    // Here, swizzle indicates how to swizzle sampler RESULT.
                    // The swizzle is done as a special case, and not at the time
                    // bEmitSwizzle causes swizzle to be emitted.
                    bEmitSwizzle[i] = FALSE;
                }
                else if( bEmitSwizzle[i] &&
                    (D3DSPR_TEXTURE == (D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcParam[i]))) &&
                    ((D3DSIO_TEXCOORD == Opcode) || (D3DSIO_TEX == Opcode) ) )
                {
                    bEmitSwizzle[i] = FALSE;
                    bSwizzleOnEval[i] = TRUE;
                }
                else if( bEmitSwizzle[i] )
                {
                    switch(Opcode)
                    {
                    case D3DSIO_SINCOS:
                    case D3DSIO_RSQ:
                    case D3DSIO_RCP:
                    case D3DSIO_EXP:
                    case D3DSIO_LOG:
                    case D3DSIO_POW:
                        if( (D3DSIO_SINCOS == Opcode) && (0 != i) ) // only check the first param for ps_2_0 sincos *macro*
                            break;

                        Assert( (PSTR_REPLICATEALPHA == SrcSwizzle[i])||(PSTR_REPLICATERED == SrcSwizzle[i])||
                                 (PSTR_REPLICATEGREEN == SrcSwizzle[i])||(PSTR_REPLICATEBLUE == SrcSwizzle[i]));        // CPSTrans::Initialize - rsq,rcp,exp,log,pow,sincos require replicate swizzle.
                        if( (D3DSIO_SINCOS != Opcode) || (D3DPS_VERSION(2,255) <= Version ) )
                        {
                            bEmitSwizzle[i] = FALSE; // swizzle (selecting scalar component) is done by the op itself
                        }
                        break;
                    case D3DSIO_CRS:
                        // CRS macro params must have no-swizzle.
                        Assert( D3DSP_NOSWIZZLE == SrcSwizzle[i]);      // CPSTrans::Initialize - crs params require no swizzle.
                        bEmitSwizzle[i] = FALSE;
                        break;
                    case D3DSIO_DP3:
                    case D3DSIO_DP4:
                    case D3DSIO_NRM:
                        break;
                    case D3DSIO_BREAKP:
                        bEmitSwizzle[i] = FALSE;
                        break;
                    case D3DSIO_IF:
                    case D3DSIO_CALLNZ:
                        if( (D3DSIO_IF == Opcode) && (D3DSPR_PREDICATE == D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcParam[i])) )
                        {
                            bEmitSwizzle[i] = FALSE;
                            break;
                        }
                        if( (D3DSIO_CALLNZ == Opcode) && (D3DSPR_PREDICATE == D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcParam[i])) && (1==i) )
                        {
                            bEmitSwizzle[i] = FALSE;
                            break;
                        }
                    default:
                        // do we need to swizzle?
                        {
                            BOOL bNeedSwizzle = FALSE;
                            for( INT comp = 0; comp < 4; comp++ )
                            {
                                if( SourceReadMasksAfterSwizzle[i] & ComponentMask[comp] )
                                {
                                    if(_SelectorFromSwizzleComponent(SrcSwizzle[i],comp) != comp )
                                    {
                                        bNeedSwizzle = TRUE;
                                        break;
                                    }
                                }
                            }
                            if( !bNeedSwizzle )
                                bEmitSwizzle[i] = FALSE;
                        }
                        break;
                    }
                }
            }


            // set clamp values
            if( D3DSPDM_SATURATE & m_pWorkerData->pInst->DstParam ) // note _sat and _pp could be combined, but ref always just ignores _pp
            {
                m_pWorkerData->DstRange[0] = 0.F;
                m_pWorkerData->DstRange[1] = 1.F;
            }
            else
            {
                if(m_pWorkerData->pInst->bTexOp)
                {
                    m_pWorkerData->DstRange[0] = -FLT_MAX;
                    m_pWorkerData->DstRange[1] = FLT_MAX;
                }
                else
                {
                    m_pWorkerData->DstRange[0] = fMin;
                    m_pWorkerData->DstRange[1] = fMax;
                }
            }

            UINT ShiftScale =
                (m_pWorkerData->pInst->DstParam & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
            if (ShiftScale & 0x8)
            {
                ShiftScale = ((~ShiftScale)&0x7)+1; // negative magnitude
                m_pWorkerData->DstScale = 1.f/(FLOAT)(1<<ShiftScale);
            }
            else
            {
                m_pWorkerData->DstScale = (FLOAT)(1<<ShiftScale);
            }

            // finished preliminary setup, now start emitting ops...

            _EnterQuadPixelLoop();

            for (i=0; i < m_pWorkerData->pInst->SrcParamCount; i++)
            {
                if( bForceNeg1To1Clamp[i] )
                {
                    // We come in here when reading constants -> We clamp -1,1 before anything else
                    // because certain hardware only supports [-1,1] (even with PixelShader1xMaxValue > 1)
                    // This clamp could have been done immediately when the constants were set,
                    // but as its undesirable, we're just doing it here each time a constant is read
                    // and leaving the original constants untouched.  This is only for pre-2_0 shaders
                    _NewPSInst(PSTRINST_SRCMOD);
                    _InstParam(PSTRINST_SRCMOD).DstReg._Set(PSTRREG_POSTMODSRC,i);
                    _InstParam(PSTRINST_SRCMOD).SrcReg0            = SrcReg[i];
                    _InstParam(PSTRINST_SRCMOD).WriteMask          = SourceReadMasks[i];
                    _InstParam(PSTRINST_SRCMOD).bBias              = FALSE;
                    _InstParam(PSTRINST_SRCMOD).bTimes2            = FALSE;
                    _InstParam(PSTRINST_SRCMOD).bComplement        = FALSE;
                    _InstParam(PSTRINST_SRCMOD).fRangeMin          = -1.0f;
                    _InstParam(PSTRINST_SRCMOD).fRangeMax          = 1.0f;
                    _InstParam(PSTRINST_SRCMOD).Predication        = m_pWorkerData->PredicateInfo;
                    SrcReg[i]._Set(PSTRREG_POSTMODSRC,i);
                }

                if( bEmitAbs[i] )
                {
                    _NewPSInst(PSTRINST_ABS);
                    _InstParam(PSTRINST_ABS).DstReg._Set(PSTRREG_POSTMODSRC,i);
                    _InstParam(PSTRINST_ABS).SrcReg0                = SrcReg[i];
                    _InstParam(PSTRINST_ABS).WriteMask              = SourceReadMasks[i];
                    _InstParam(PSTRINST_ABS).Predication            = m_pWorkerData->PredicateInfo;
                    SrcReg[i]._Set(PSTRREG_POSTMODSRC,i);
                }

                if( bEmitSrcMod[i] )
                {
                    _NewPSInst(PSTRINST_SRCMOD);
                    _InstParam(PSTRINST_SRCMOD).DstReg._Set(PSTRREG_POSTMODSRC,i);
                    _InstParam(PSTRINST_SRCMOD).SrcReg0            = SrcReg[i];
                    _InstParam(PSTRINST_SRCMOD).WriteMask          = SourceReadMasks[i];
                    _InstParam(PSTRINST_SRCMOD).bBias              = bSrcBias[i];
                    _InstParam(PSTRINST_SRCMOD).bTimes2            = bSrcTimes2[i];
                    _InstParam(PSTRINST_SRCMOD).bComplement        = bSrcComplement[i];
                    _InstParam(PSTRINST_SRCMOD).fRangeMin          = fMin;
                    _InstParam(PSTRINST_SRCMOD).fRangeMax          = fMax;
                    _InstParam(PSTRINST_SRCMOD).Predication        = m_pWorkerData->PredicateInfo;
                    SrcReg[i]._Set(PSTRREG_POSTMODSRC,i);
                }

                if( bEmitSwizzle[i] && !bProjOnEval[i] )
                {
                    _NewPSInst(PSTRINST_SWIZZLE);
                    _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_POSTMODSRC,i);
                    _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[i];
                    _InstParam(PSTRINST_SWIZZLE).WriteMask      = SourceReadMasksAfterSwizzle[i];
                    _InstParam(PSTRINST_SWIZZLE).Swizzle        = SrcSwizzle[i];
                    _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;
                    SrcReg[i]._Set(PSTRREG_POSTMODSRC,i);
                }
            }

            switch(Opcode)
            {
            case D3DSIO_TEXCOORD:
            case D3DSIO_TEXKILL:
                {
                    if(D3DPS_VERSION(2,0) > Version )
                    {
                        if( !(  (D3DSIO_TEXKILL == Opcode)  &&
                                (D3DSPR_TEMP    == (D3DSI_GETREGTYPE(m_pWorkerData->pInst->DstParam)))
                            )
                        )
                        {
                            UINT CoordSet = m_pWorkerData->pInst->SrcParam[0] ? D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[0]) :
                                                                D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);

                            PSTRRegister CoordReg;

                            if( bSwizzleOnEval[0] || bProjOnEval[0] )
                                CoordReg._Set(PSTRREG_POSTMODSRC,0);
                            else
                                CoordReg = DstReg;

                            if( !(PSTRREG_TEXTURE == CoordReg.GetRegType() && (CoordReg.GetRegNum() == CoordSet) ) )
                            {
                                // Destination register is not the same as source texture coordinate register
                                // (note t# registers have been preloaded with texcoords earlier already)
                                // So, copy to dest.
                                _NewPSInst(PSTRINST_MOV);
                                _InstParam(PSTRINST_MOV).DstReg             = CoordReg;
                                _InstParam(PSTRINST_MOV).SrcReg0._Set(PSTRREG_TEXTURE,CoordSet);
                                _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                                _InstParam(PSTRINST_MOV).WriteMask          = PSTR_COMPONENTMASK_ALL;
                                _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;
                            }

                            if( bSwizzleOnEval[0] )
                            {
                                _NewPSInst(PSTRINST_SWIZZLE);
                                _InstParam(PSTRINST_SWIZZLE).DstReg         = DstReg;
                                _InstParam(PSTRINST_SWIZZLE).SrcReg0        = CoordReg;
                                _InstParam(PSTRINST_SWIZZLE).WriteMask      = SourceReadMasksAfterSwizzle[0];
                                _InstParam(PSTRINST_SWIZZLE).Swizzle        = SrcSwizzle[0];
                                _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;
                            }

                            if( bProjOnEval[0] )
                            {
                                _EmitProj(DstReg,DstReg,ProjComponent[0],fMax,TRUE);
                            }

                            // check version (first DWORD of code token stream), and always
                            //  set 4th component to 1_0 for ps_1_3 or earlier
                            if ( (D3DSIO_TEXCOORD == Opcode) && (D3DPS_VERSION(1,3) >= Version) )
                            {
                                _NewPSInst(PSTRINST_MOV);
                                _InstParam(PSTRINST_MOV).DstReg             = DstReg;
                                _InstParam(PSTRINST_MOV).SrcReg0            = OneReg; // 1.0f
                                _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                                _InstParam(PSTRINST_MOV).WriteMask          = PSTR_COMPONENTMASK_3;
                                _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;
                            }
                        }

                        _EmitDstMod(DstReg,DstWriteMask);
                    }

                    if( (D3DSIO_TEXKILL == Opcode) )
                    {
                        _NewPSInst(PSTRINST_KILL);
                        _InstParam(PSTRINST_KILL).SrcReg0       = DstReg;
                        _InstParam(PSTRINST_KILL).WriteMask     = (D3DPS_VERSION(2,0) <= Version) ? DstWriteMask :
                                                                    PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2; // 3 component pre-ps_2_0
                        _InstParam(PSTRINST_KILL).bKillLZ[0]    = TRUE; // FALSE would have killed on >= 0
                        _InstParam(PSTRINST_KILL).bKillLZ[1]    = TRUE; // Currently this control isn't exposed through API (hence hardcoded TRUE)
                        _InstParam(PSTRINST_KILL).bKillLZ[2]    = TRUE;
                        _InstParam(PSTRINST_KILL).bKillLZ[3]    = TRUE;
                        _InstParam(PSTRINST_KILL).Predication   = m_pWorkerData->PredicateInfo;

                        //
                        // Remember that there is a texkill instruction
                        //
                        m_bHasTexKillInstructions = TRUE;
                    }
                }
                break;
            case D3DSIO_TEX:
                {
                    PSTRRegister CoordReg, XGradient, YGradient;
                    XGradient._Set(PSTRREG_XGRADIENT,0);
                    YGradient._Set(PSTRREG_YGRADIENT,0);

                    UINT CoordSet = m_pWorkerData->pInst->SrcParam[0] ? D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[0]) :
                                                         D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);

                    if( m_pWorkerData->pInst->SrcParam[0] )
                    {
                        CoordReg = SrcReg[0];
                    }
                    else // no source param.
                    {
                        CoordReg._Set(PSTRREG_TEXTURE,CoordSet);
                    }

                    if( bSwizzleOnEval[0] )
                    {
                        _NewPSInst(PSTRINST_SWIZZLE);
                        _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_POSTMODSRC,0);
                        _InstParam(PSTRINST_SWIZZLE).SrcReg0        = CoordReg;
                        _InstParam(PSTRINST_SWIZZLE).WriteMask      = SourceReadMasksAfterSwizzle[0];
                        _InstParam(PSTRINST_SWIZZLE).Swizzle        = SrcSwizzle[0];
                        _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;
                        CoordReg._Set(PSTRREG_POSTMODSRC,0);
                    }

                    if( bSrcNegate[0] )
                    {
                        _NewPSInst(PSTRINST_MOV);
                        _InstParam(PSTRINST_MOV).DstReg._Set(PSTRREG_POSTMODSRC,0);
                        _InstParam(PSTRINST_MOV).SrcReg0        = CoordReg;
                        _InstParam(PSTRINST_MOV).bSrcReg0_Negate = TRUE;
                        _InstParam(PSTRINST_MOV).WriteMask      = SourceReadMasksAfterSwizzle[0];
                        _InstParam(PSTRINST_MOV).Predication    = m_pWorkerData->ForceNoPredication;
                        CoordReg._Set(PSTRREG_POSTMODSRC,0);
                    }

                    if( bProjOnEval[0] )
                    {
                        PSTRRegister OldCoordReg = CoordReg;
                        CoordReg._Set(PSTRREG_POSTMODSRC,0);
                        _EmitProj(CoordReg,OldCoordReg,ProjComponent[0],fMax,TRUE);
                    }
                    else if( D3DSI_TEXLD_PROJECT & OpcodeSpecificControl )
                    {
                        // Project by fourth component of texture register.
                        PSTRRegister OldCoordReg = CoordReg;
                        CoordReg._Set(PSTRREG_POSTMODSRC,0);
                        _EmitProj(CoordReg,OldCoordReg,PSTR_COMPONENTMASK_3,fMax,FALSE);
                    }

                    UINT        uiStage = m_pWorkerData->pInst->SrcParam[1] ?
                                          D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[1]) :
                                          D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);

                    BYTE        GradientComponentMask = PSTR_COMPONENTMASK_0;
                    switch( m_SamplerRegDcl[uiStage] )
                    {
                    case D3DSTT_2D:
                        GradientComponentMask |= PSTR_COMPONENTMASK_1;
                        break;
                    case D3DSTT_CUBE:
                    case D3DSTT_VOLUME:
                    default:
                        GradientComponentMask |= PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        break;
                    }

                    BOOL bAddLODBiasFromTexCoord_W = (D3DSI_TEXLD_BIAS & OpcodeSpecificControl);
                                                     // Per-pixel LOD bias (taken from texcoord W)


                    _LeaveQuadPixelLoop();

                    _NewPSInst(PSTRINST_DSX);
                    _InstParam(PSTRINST_DSX).DstReg             = XGradient;
                    _InstParam(PSTRINST_DSX).SrcReg0            = CoordReg;
                    _InstParam(PSTRINST_DSX).WriteMask          = GradientComponentMask;
                    _InstParam(PSTRINST_DSX).bQuadPixelShared   = !bAddLODBiasFromTexCoord_W;
                    _InstParam(PSTRINST_DSX).Predication        = m_pWorkerData->ForceNoPredication;

                    _NewPSInst(PSTRINST_DSY);
                    _InstParam(PSTRINST_DSY).DstReg             = YGradient;
                    _InstParam(PSTRINST_DSY).SrcReg0            = CoordReg;
                    _InstParam(PSTRINST_DSY).WriteMask          = GradientComponentMask;
                    _InstParam(PSTRINST_DSY).bQuadPixelShared   = !bAddLODBiasFromTexCoord_W;
                    _InstParam(PSTRINST_DSY).Predication        = m_pWorkerData->ForceNoPredication;

                    /*if(bAddLODBiasFromTexCoord_W) // Per-pixel LOD bias
                    {
                        _EnterQuadPixelLoop();

                        // dsx == 2^LODsx (by definition)
                        // dsy == 2^LODsy (by definition)
                        // But we want LOD' = LOD + bias, and we have to accomplish this by perturbing dsx, dsy
                        // i.e., we need: dsx == 2^(LODsx + bias) and dsy == 2^(LODsy + bias)
                        //
                        // We accomplish this by using the following property:
                        // (2^(a+b)) = 2^a * 2^b

                        PSTRRegister Scratch;
                        Scratch._Set(PSTRREG_SCRATCH,0);

                        // Scratch.w = 2^(CoordReg.w), where CoordReg.w is the bias value
                        _NewPSInst(PSTRINST_EXP);
                        _InstParam(PSTRINST_EXP).DstReg             = Scratch;
                        _InstParam(PSTRINST_EXP).SrcReg0            = CoordReg;
                        _InstParam(PSTRINST_EXP).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_EXP).WriteMask          = GradientComponentMask;
                        _InstParam(PSTRINST_EXP).SrcReg0_Selector   = PSTR_SELECT_A;
                        _InstParam(PSTRINST_EXP).Predication        = m_pWorkerData->ForceNoPredication;

                        // dsx = dsx * Scratch.w
                        _NewPSInst(PSTRINST_MUL);
                        _InstParam(PSTRINST_MUL).DstReg             = XGradient;
                        _InstParam(PSTRINST_MUL).SrcReg0            = XGradient;
                        _InstParam(PSTRINST_MUL).SrcReg1            = Scratch;
                        _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).WriteMask          = GradientComponentMask;
                        _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->ForceNoPredication;

                        // dsy = dsy * Scratch.w
                        _NewPSInst(PSTRINST_MUL);
                        _InstParam(PSTRINST_MUL).DstReg             = YGradient;
                        _InstParam(PSTRINST_MUL).SrcReg0            = YGradient;
                        _InstParam(PSTRINST_MUL).SrcReg1            = Scratch;
                        _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).WriteMask          = GradientComponentMask;
                        _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->ForceNoPredication;

                        // Call texcoverage per-pixel (so LOD is unique for each pixel's texture sample)
                        _NewPSInst(PSTRINST_TEXCOVERAGE);
                        _InstParam(PSTRINST_TEXCOVERAGE).uiStage            = uiStage;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcXGradient       = XGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcYGradient       = YGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).bQuadPixelShared   = FALSE;
                        _InstParam(PSTRINST_TEXCOVERAGE).bAllowLegacyApproximations = bAllowLegacyApproximations;
                    }
                    else // Standard gradient calculation (no per-pixel lod bias)*/
                    {
                        _NewPSInst(PSTRINST_TEXCOVERAGE);
                        _InstParam(PSTRINST_TEXCOVERAGE).uiStage            = uiStage;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcXGradient       = XGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcYGradient       = YGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_TEXCOVERAGE).bAllowLegacyApproximations = bAllowLegacyApproximations;

                        _EnterQuadPixelLoop();
                    }


                    // if this is a tex op with a sampler parameter, see if there is a swizzle on it.
                    PSTRRegister SampleResult;
                    bool bSwizzlingResult;
                    if( m_pWorkerData->pInst->SrcParam[1] && (D3DSP_NOSWIZZLE != (D3DSP_SWIZZLE_MASK & m_pWorkerData->pInst->SrcParam[1])) )
                    {
                        SampleResult._Set(PSTRREG_SCRATCH,0);
                        bSwizzlingResult = true;
                    }
                    else
                    {
                        SampleResult = DstReg;
                        bSwizzlingResult = false;
                    }

                    _NewPSInst(PSTRINST_SAMPLE);
                    _InstParam(PSTRINST_SAMPLE).DstReg          = SampleResult;
                    _InstParam(PSTRINST_SAMPLE).CoordReg        = CoordReg;
                    _InstParam(PSTRINST_SAMPLE).WriteMask       = bSwizzlingResult ? SourceReadMasks[1] : DstWriteMask;
                    _InstParam(PSTRINST_SAMPLE).uiStage         = uiStage;
                    _InstParam(PSTRINST_SAMPLE).Predication     = bSwizzlingResult ? m_pWorkerData->ForceNoPredication : m_pWorkerData->PredicateInfo;
                    _InstParam(PSTRINST_SAMPLE).bAllowLegacyApproximations = bAllowLegacyApproximations;
                    _InstParam(PSTRINST_SAMPLE).bLODBiasFromW   = bAddLODBiasFromTexCoord_W;
                    _InstParam(PSTRINST_SAMPLE).bForceLODFromW  = FALSE;
                    _InstParam(PSTRINST_SAMPLE).bAlternateGradient = FALSE;

                    if( bSwizzlingResult )
                    {
                        _NewPSInst(PSTRINST_SWIZZLE);
                        _InstParam(PSTRINST_SWIZZLE).DstReg         = DstReg;
                        _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SampleResult;
                        _InstParam(PSTRINST_SWIZZLE).WriteMask      = DstWriteMask;
                        _InstParam(PSTRINST_SWIZZLE).Swizzle        = SrcSwizzle[1];
                        _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->PredicateInfo;
                    }

                    _EmitDstMod(DstReg,DstWriteMask);
                }
                break;
            case D3DSIO_TEXDP3:
            case D3DSIO_TEXDP3TEX:
                {
                    PSTRRegister CoordReg;
                    CoordReg._Set(PSTRREG_TEXTURE,D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam));

                    if( D3DSIO_TEXDP3 == Opcode )
                    {
                        _NewPSInst(PSTRINST_DP3);
                        _InstParam(PSTRINST_DP3).DstReg             = DstReg;
                        _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                        _InstParam(PSTRINST_DP3).SrcReg1            = CoordReg;
                        _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_DP3).WriteMask          = PSTR_COMPONENTMASK_ALL;
                        _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->ForceNoPredication;
                        _EmitDstMod(DstReg,DstWriteMask);
                    }
                    else // D3DSIO_TEXDP3TEX
                    {
                        UINT uiStage = D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);
                        PSTRRegister XGradient, YGradient;
                        XGradient._Set(PSTRREG_XGRADIENT,0);
                        YGradient._Set(PSTRREG_YGRADIENT,0);

                        _NewPSInst(PSTRINST_DP3);
                        _InstParam(PSTRINST_DP3).DstReg             = CoordReg;
                        _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                        _InstParam(PSTRINST_DP3).SrcReg1            = CoordReg;
                        _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_DP3).WriteMask          = PSTR_COMPONENTMASK_0;
                        _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_MOV);
                        _InstParam(PSTRINST_MOV).DstReg             = CoordReg;
                        _InstParam(PSTRINST_MOV).SrcReg0            = ZeroReg; // 0.0f
                        _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MOV).WriteMask          = PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;

                        _LeaveQuadPixelLoop();

                        _NewPSInst(PSTRINST_DSX);
                        _InstParam(PSTRINST_DSX).DstReg             = XGradient;
                        _InstParam(PSTRINST_DSX).SrcReg0            = CoordReg;
                        _InstParam(PSTRINST_DSX).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                        _InstParam(PSTRINST_DSX).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_DSX).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_DSY);
                        _InstParam(PSTRINST_DSY).DstReg             = YGradient;
                        _InstParam(PSTRINST_DSY).SrcReg0            = CoordReg;
                        _InstParam(PSTRINST_DSY).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                        _InstParam(PSTRINST_DSY).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_DSY).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_TEXCOVERAGE);
                        _InstParam(PSTRINST_TEXCOVERAGE).uiStage            = uiStage;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcXGradient       = XGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcYGradient       = YGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_TEXCOVERAGE).bAllowLegacyApproximations = bAllowLegacyApproximations;

                        _EnterQuadPixelLoop();

                        _NewPSInst(PSTRINST_SAMPLE);
                        _InstParam(PSTRINST_SAMPLE).DstReg          = DstReg;
                        _InstParam(PSTRINST_SAMPLE).CoordReg        = CoordReg;
                        _InstParam(PSTRINST_SAMPLE).WriteMask       = DstWriteMask;
                        _InstParam(PSTRINST_SAMPLE).uiStage         = uiStage;
                        _InstParam(PSTRINST_SAMPLE).Predication     = m_pWorkerData->ForceNoPredication;
                        _InstParam(PSTRINST_SAMPLE).bAllowLegacyApproximations = bAllowLegacyApproximations;
                        _InstParam(PSTRINST_SAMPLE).bLODBiasFromW   = FALSE;
                        _InstParam(PSTRINST_SAMPLE).bForceLODFromW  = FALSE;
                        _InstParam(PSTRINST_SAMPLE).bAlternateGradient = FALSE;


                        _EmitDstMod(DstReg,DstWriteMask);
                    }
                }
                break;
            case D3DSIO_TEXREG2AR:
            case D3DSIO_TEXREG2GB:
            case D3DSIO_TEXREG2RGB:
                {
                    PSTRRegister    CoordReg, XGradient, YGradient;
                    BYTE            SwizzleR = 0;
                    BYTE            SwizzleG = 0;
                    BYTE            GradientMask = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    UINT            uiStage = D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);
                    XGradient._Set(PSTRREG_XGRADIENT,0);
                    YGradient._Set(PSTRREG_YGRADIENT,0);

                    switch( Opcode )
                    {
                    case D3DSIO_TEXREG2AR:
                        CoordReg._Set(PSTRREG_SCRATCH,0);
                        SwizzleR = ComponentReplicate[PSTR_SELECT_A];
                        SwizzleG = ComponentReplicate[PSTR_SELECT_R];
                        break;
                    case D3DSIO_TEXREG2GB:
                        CoordReg._Set(PSTRREG_SCRATCH,0);
                        SwizzleR = ComponentReplicate[PSTR_SELECT_G];
                        SwizzleG = ComponentReplicate[PSTR_SELECT_B];
                        break;
                    case D3DSIO_TEXREG2RGB:
                        CoordReg = SrcReg[0];
                        GradientMask |= PSTR_COMPONENTMASK_3;
                        break;
                    }

                    if( (D3DSIO_TEXREG2AR == Opcode) || (D3DSIO_TEXREG2GB == Opcode) )
                    {
                        _NewPSInst(PSTRINST_SWIZZLE);
                        _InstParam(PSTRINST_SWIZZLE).DstReg         = CoordReg;
                        _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[0];
                        _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_0;
                        _InstParam(PSTRINST_SWIZZLE).Swizzle        = SwizzleR;
                        _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_SWIZZLE);
                        _InstParam(PSTRINST_SWIZZLE).DstReg         = CoordReg;
                        _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[0];
                        _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_1;
                        _InstParam(PSTRINST_SWIZZLE).Swizzle        = SwizzleG;
                        _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_MOV);
                        _InstParam(PSTRINST_MOV).DstReg             = CoordReg;
                        _InstParam(PSTRINST_MOV).SrcReg0            = SrcReg[0];
                        _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MOV).WriteMask          = PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;
                    }

                    _LeaveQuadPixelLoop();

                    _NewPSInst(PSTRINST_DSX);
                    _InstParam(PSTRINST_DSX).DstReg             = XGradient;
                    _InstParam(PSTRINST_DSX).SrcReg0            = CoordReg;
                    _InstParam(PSTRINST_DSX).WriteMask          = GradientMask;
                    _InstParam(PSTRINST_DSX).bQuadPixelShared   = TRUE;
                    _InstParam(PSTRINST_DSX).Predication        = m_pWorkerData->ForceNoPredication;

                    _NewPSInst(PSTRINST_DSY);
                    _InstParam(PSTRINST_DSY).DstReg             = YGradient;
                    _InstParam(PSTRINST_DSY).SrcReg0            = CoordReg;
                    _InstParam(PSTRINST_DSY).WriteMask          = GradientMask;
                    _InstParam(PSTRINST_DSY).bQuadPixelShared   = TRUE;
                    _InstParam(PSTRINST_DSY).Predication        = m_pWorkerData->ForceNoPredication;

                    _NewPSInst(PSTRINST_TEXCOVERAGE);
                    _InstParam(PSTRINST_TEXCOVERAGE).uiStage            = uiStage;
                    _InstParam(PSTRINST_TEXCOVERAGE).SrcXGradient       = XGradient;
                    _InstParam(PSTRINST_TEXCOVERAGE).SrcYGradient       = YGradient;
                    _InstParam(PSTRINST_TEXCOVERAGE).bQuadPixelShared   = TRUE;
                    _InstParam(PSTRINST_TEXCOVERAGE).bAllowLegacyApproximations = bAllowLegacyApproximations;

                    _EnterQuadPixelLoop();

                    _NewPSInst(PSTRINST_SAMPLE);
                    _InstParam(PSTRINST_SAMPLE).DstReg          = DstReg;
                    _InstParam(PSTRINST_SAMPLE).CoordReg        = CoordReg;
                    _InstParam(PSTRINST_SAMPLE).WriteMask       = DstWriteMask;
                    _InstParam(PSTRINST_SAMPLE).uiStage         = uiStage;
                    _InstParam(PSTRINST_SAMPLE).Predication     = m_pWorkerData->ForceNoPredication;
                    _InstParam(PSTRINST_SAMPLE).bAllowLegacyApproximations = bAllowLegacyApproximations;
                    _InstParam(PSTRINST_SAMPLE).bLODBiasFromW   = FALSE;
                    _InstParam(PSTRINST_SAMPLE).bForceLODFromW  = FALSE;
                    _InstParam(PSTRINST_SAMPLE).bAlternateGradient = FALSE;

                    _EmitDstMod(DstReg,DstWriteMask);
                }
                break;
            case D3DSIO_TEXBEM:
            case D3DSIO_TEXBEML:
            case D3DSIO_TEXBEM_LEGACY:      // refrast only -> used with legacy fixed function rasterizer
            case D3DSIO_TEXBEML_LEGACY:     // refrast only -> used with legacy fixed function rasterizer
                {
                    BOOL bDoLuminance = ((D3DSIO_TEXBEML == Opcode) || (D3DSIO_TEXBEML_LEGACY == Opcode));
                    PSTRRegister CoordReg, XGradient, YGradient;
                    UINT uiStage = D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);
                    CoordReg._Set(PSTRREG_TEXTURE,D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam));
                    XGradient._Set(PSTRREG_XGRADIENT,0);
                    YGradient._Set(PSTRREG_YGRADIENT,0);

                    _NewPSInst(PSTRINST_BEM);
                    _InstParam(PSTRINST_BEM).DstReg             = CoordReg;
                    _InstParam(PSTRINST_BEM).SrcReg0            = CoordReg;
                    _InstParam(PSTRINST_BEM).SrcReg1            = SrcReg[0];
                    _InstParam(PSTRINST_BEM).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_BEM).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_BEM).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_BEM).uiStage            = m_pWorkerData->pInst->uiTSSNum;

                    _EmitDstMod(CoordReg,PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1);

                    _LeaveQuadPixelLoop();

                    _NewPSInst(PSTRINST_DSX);
                    _InstParam(PSTRINST_DSX).DstReg             = XGradient;
                    _InstParam(PSTRINST_DSX).SrcReg0            = CoordReg;
                    _InstParam(PSTRINST_DSX).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_DSX).bQuadPixelShared   = TRUE;
                    _InstParam(PSTRINST_DSX).Predication        = m_pWorkerData->ForceNoPredication;

                    _NewPSInst(PSTRINST_DSY);
                    _InstParam(PSTRINST_DSY).DstReg             = YGradient;
                    _InstParam(PSTRINST_DSY).SrcReg0            = CoordReg;
                    _InstParam(PSTRINST_DSY).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_DSY).bQuadPixelShared   = TRUE;
                    _InstParam(PSTRINST_DSY).Predication        = m_pWorkerData->ForceNoPredication;

                    _NewPSInst(PSTRINST_TEXCOVERAGE);
                    _InstParam(PSTRINST_TEXCOVERAGE).uiStage            = uiStage;
                    _InstParam(PSTRINST_TEXCOVERAGE).SrcXGradient       = XGradient;
                    _InstParam(PSTRINST_TEXCOVERAGE).SrcYGradient       = YGradient;
                    _InstParam(PSTRINST_TEXCOVERAGE).bQuadPixelShared   = TRUE;
                    _InstParam(PSTRINST_TEXCOVERAGE).bAllowLegacyApproximations = bAllowLegacyApproximations;

                    _EnterQuadPixelLoop();

                    _NewPSInst(PSTRINST_SAMPLE);
                    _InstParam(PSTRINST_SAMPLE).DstReg          = DstReg;
                    _InstParam(PSTRINST_SAMPLE).CoordReg        = CoordReg;
                    _InstParam(PSTRINST_SAMPLE).WriteMask       = DstWriteMask;
                    _InstParam(PSTRINST_SAMPLE).uiStage         = uiStage;
                    _InstParam(PSTRINST_SAMPLE).Predication     = m_pWorkerData->ForceNoPredication;
                    _InstParam(PSTRINST_SAMPLE).bAllowLegacyApproximations = bAllowLegacyApproximations;
                    _InstParam(PSTRINST_SAMPLE).bLODBiasFromW   = FALSE;
                    _InstParam(PSTRINST_SAMPLE).bForceLODFromW  = FALSE;
                    _InstParam(PSTRINST_SAMPLE).bAlternateGradient = FALSE;

                    if( bDoLuminance )
                    {
                        _NewPSInst(PSTRINST_LUMINANCE);
                        _InstParam(PSTRINST_LUMINANCE).DstReg             = DstReg;
                        _InstParam(PSTRINST_LUMINANCE).SrcReg0            = DstReg;
                        _InstParam(PSTRINST_LUMINANCE).SrcReg1            = SrcReg[0];
                        _InstParam(PSTRINST_LUMINANCE).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_LUMINANCE).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_LUMINANCE).uiStage            = m_pWorkerData->pInst->uiTSSNum;
                    }

                    _EmitDstMod(DstReg,DstWriteMask);
                }
                break;
            case D3DSIO_TEXDEPTH:
                // Take r,g values and compute r/g, which
                // can be interpreted is z/w = perspective correct depth.
                // Then set the z coord for the pixel.

                // First, check if denominator is 0.  If so, result of z/w = 1.
                _NewPSInst(PSTRINST_MUL);
                _InstParam(PSTRINST_MUL).DstReg._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_MUL).SrcReg0            = DstReg;
                _InstParam(PSTRINST_MUL).SrcReg1            = DstReg;
                _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_1;
                _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_SWIZZLE);
                _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_SWIZZLE).SrcReg0._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_0;
                _InstParam(PSTRINST_SWIZZLE).Swizzle        = PSTR_REPLICATEGREEN;
                _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;

                // If denominator is 0, set numerator and denominator to 1, so divide will yield 1.
                _NewPSInst(PSTRINST_CMP);
                _InstParam(PSTRINST_CMP).DstReg             = DstReg;
                _InstParam(PSTRINST_CMP).SrcReg0._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_CMP).SrcReg1            = OneReg;
                _InstParam(PSTRINST_CMP).SrcReg2            = DstReg;
                _InstParam(PSTRINST_CMP).bSrcReg0_Negate    = TRUE;
                _InstParam(PSTRINST_CMP).bSrcReg1_Negate    = FALSE;
                _InstParam(PSTRINST_CMP).bSrcReg2_Negate    = FALSE;
                _InstParam(PSTRINST_CMP).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                _InstParam(PSTRINST_CMP).Predication        = m_pWorkerData->ForceNoPredication;

                // Now do the actual divide

                _NewPSInst(PSTRINST_LEGACYRCP);
                _InstParam(PSTRINST_LEGACYRCP).DstReg._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_LEGACYRCP).SrcReg0            = DstReg;
                _InstParam(PSTRINST_LEGACYRCP).bSrcReg0_Negate    = FALSE;
                _InstParam(PSTRINST_LEGACYRCP).WriteMask          = PSTR_COMPONENTMASK_0;
                _InstParam(PSTRINST_LEGACYRCP).SrcReg0_Selector   = PSTR_SELECT_G;
                _InstParam(PSTRINST_LEGACYRCP).fRangeMax          = fMax;
                _InstParam(PSTRINST_LEGACYRCP).Predication        = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_MUL);
                _InstParam(PSTRINST_MUL).DstReg             = DstReg;
                _InstParam(PSTRINST_MUL).SrcReg0            = DstReg;
                _InstParam(PSTRINST_MUL).SrcReg1._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_0;
                _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_DEPTH);
                _InstParam(PSTRINST_DEPTH).SrcReg0          = DstReg;
                break;
            case D3DSIO_TEXLDD:
                {
                    UINT        uiSampler = D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[1]);
                    PSTRRegister    CoordReg, XGradient, YGradient;

                    CoordReg = SrcReg[0];
                    XGradient = SrcReg[2];
                    YGradient = SrcReg[3];

                    if( bSrcNegate[0] )
                    {
                        _NewPSInst(PSTRINST_MOV);
                        _InstParam(PSTRINST_MOV).DstReg._Set(PSTRREG_POSTMODSRC,0);
                        _InstParam(PSTRINST_MOV).SrcReg0        = CoordReg;
                        _InstParam(PSTRINST_MOV).bSrcReg0_Negate = TRUE;
                        _InstParam(PSTRINST_MOV).WriteMask      = SourceReadMasksAfterSwizzle[0];
                        _InstParam(PSTRINST_MOV).Predication    = m_pWorkerData->ForceNoPredication;
                        CoordReg._Set(PSTRREG_POSTMODSRC,0);
                    }

                    if( bSrcNegate[2] )
                    {
                        _NewPSInst(PSTRINST_MOV);
                        _InstParam(PSTRINST_MOV).DstReg._Set(PSTRREG_POSTMODSRC,2);
                        _InstParam(PSTRINST_MOV).SrcReg0        = XGradient;
                        _InstParam(PSTRINST_MOV).bSrcReg0_Negate = TRUE;
                        _InstParam(PSTRINST_MOV).WriteMask      = SourceReadMasksAfterSwizzle[2];
                        _InstParam(PSTRINST_MOV).Predication    = m_pWorkerData->ForceNoPredication;
                        XGradient._Set(PSTRREG_POSTMODSRC,2);
                    }

                    if( bSrcNegate[3] )
                    {
                        _NewPSInst(PSTRINST_MOV);
                        _InstParam(PSTRINST_MOV).DstReg._Set(PSTRREG_POSTMODSRC,3);
                        _InstParam(PSTRINST_MOV).SrcReg0        = YGradient;
                        _InstParam(PSTRINST_MOV).bSrcReg0_Negate = TRUE;
                        _InstParam(PSTRINST_MOV).WriteMask      = SourceReadMasksAfterSwizzle[3];
                        _InstParam(PSTRINST_MOV).Predication    = m_pWorkerData->ForceNoPredication;
                        YGradient._Set(PSTRREG_POSTMODSRC,3);
                    }

                    _NewPSInst(PSTRINST_TEXCOVERAGE);
                    _InstParam(PSTRINST_TEXCOVERAGE).uiStage            = uiSampler;
                    _InstParam(PSTRINST_TEXCOVERAGE).SrcXGradient       = XGradient;
                    _InstParam(PSTRINST_TEXCOVERAGE).SrcYGradient       = YGradient;
                    _InstParam(PSTRINST_TEXCOVERAGE).bQuadPixelShared   = FALSE; // individual LOD for EACH pixel.
                    _InstParam(PSTRINST_TEXCOVERAGE).bAllowLegacyApproximations = bAllowLegacyApproximations;

                    // if this is a tex op with a sampler parameter, see if there is a swizzle on it.
                    PSTRRegister SampleResult;
                    bool bSwizzlingResult;
                    if( D3DSP_NOSWIZZLE != (D3DSP_SWIZZLE_MASK & m_pWorkerData->pInst->SrcParam[1]) )
                    {
                        SampleResult._Set(PSTRREG_SCRATCH,0);
                        bSwizzlingResult = true;
                    }
                    else
                    {
                        SampleResult = DstReg;
                        bSwizzlingResult = false;
                    }

                    _NewPSInst(PSTRINST_SAMPLE);
                    _InstParam(PSTRINST_SAMPLE).DstReg          = SampleResult;
                    _InstParam(PSTRINST_SAMPLE).CoordReg        = CoordReg;
                    _InstParam(PSTRINST_SAMPLE).WriteMask       = bSwizzlingResult ? SourceReadMasks[1] : DstWriteMask;
                    _InstParam(PSTRINST_SAMPLE).uiStage         = uiSampler;
                    _InstParam(PSTRINST_SAMPLE).Predication     = bSwizzlingResult ? m_pWorkerData->ForceNoPredication : m_pWorkerData->PredicateInfo;
                    _InstParam(PSTRINST_SAMPLE).bAllowLegacyApproximations = bAllowLegacyApproximations;
                    _InstParam(PSTRINST_SAMPLE).bLODBiasFromW              = FALSE;
                    _InstParam(PSTRINST_SAMPLE).bForceLODFromW             = FALSE;
                    _InstParam(PSTRINST_SAMPLE).bAlternateGradient         = TRUE;
                    _InstParam(PSTRINST_SAMPLE).SrcXGradient               = XGradient;
                    _InstParam(PSTRINST_SAMPLE).SrcYGradient               = YGradient;

                    if( bSwizzlingResult )
                    {
                        _NewPSInst(PSTRINST_SWIZZLE);
                        _InstParam(PSTRINST_SWIZZLE).DstReg         = DstReg;
                        _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SampleResult;
                        _InstParam(PSTRINST_SWIZZLE).WriteMask      = DstWriteMask;
                        _InstParam(PSTRINST_SWIZZLE).Swizzle        = SrcSwizzle[1];
                        _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->PredicateInfo;
                    }

                    _EmitDstMod(DstReg,DstWriteMask);
                }
                break;
            case D3DSIO_TEXLDL:
                {
                    UINT        uiSampler = D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[1]);
                    PSTRRegister    CoordReg = SrcReg[0];

                    if( bSrcNegate[0] )
                    {
                        _NewPSInst(PSTRINST_MOV);
                        _InstParam(PSTRINST_MOV).DstReg._Set(PSTRREG_POSTMODSRC,0);
                        _InstParam(PSTRINST_MOV).SrcReg0        = CoordReg;
                        _InstParam(PSTRINST_MOV).bSrcReg0_Negate = TRUE;
                        _InstParam(PSTRINST_MOV).WriteMask      = SourceReadMasksAfterSwizzle[0];
                        _InstParam(PSTRINST_MOV).Predication    = m_pWorkerData->ForceNoPredication;
                        CoordReg._Set(PSTRREG_POSTMODSRC,0);
                    }

                    /*_NewPSInst(PSTRINST_FORCELOD);
                    _InstParam(PSTRINST_FORCELOD).uiStage            = uiSampler;
                    _InstParam(PSTRINST_FORCELOD).LODRegister        = CoordReg;
                    _InstParam(PSTRINST_FORCELOD).LODComponent_Selector = PSTR_SELECT_A;*/

                    // if this is a tex op with a sampler parameter, see if there is a swizzle on it.
                    PSTRRegister SampleResult;
                    bool bSwizzlingResult;
                    if( D3DSP_NOSWIZZLE != (D3DSP_SWIZZLE_MASK & m_pWorkerData->pInst->SrcParam[1]) )
                    {
                        SampleResult._Set(PSTRREG_SCRATCH,0);
                        bSwizzlingResult = true;
                    }
                    else
                    {
                        SampleResult = DstReg;
                        bSwizzlingResult = false;
                    }

                    _NewPSInst(PSTRINST_SAMPLE);
                    _InstParam(PSTRINST_SAMPLE).DstReg          = SampleResult;
                    _InstParam(PSTRINST_SAMPLE).CoordReg        = CoordReg;
                    _InstParam(PSTRINST_SAMPLE).WriteMask       = bSwizzlingResult ? SourceReadMasks[1] : DstWriteMask;
                    _InstParam(PSTRINST_SAMPLE).uiStage         = uiSampler;
                    _InstParam(PSTRINST_SAMPLE).Predication     = bSwizzlingResult ? m_pWorkerData->ForceNoPredication : m_pWorkerData->PredicateInfo;
                    _InstParam(PSTRINST_SAMPLE).bAllowLegacyApproximations = bAllowLegacyApproximations;
                    _InstParam(PSTRINST_SAMPLE).bLODBiasFromW   = FALSE;
                    _InstParam(PSTRINST_SAMPLE).bForceLODFromW  = TRUE;
                    _InstParam(PSTRINST_SAMPLE).bAlternateGradient = FALSE;

                    if( bSwizzlingResult )
                    {
                        _NewPSInst(PSTRINST_SWIZZLE);
                        _InstParam(PSTRINST_SWIZZLE).DstReg         = DstReg;
                        _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SampleResult;
                        _InstParam(PSTRINST_SWIZZLE).WriteMask      = DstWriteMask;
                        _InstParam(PSTRINST_SWIZZLE).Swizzle        = SrcSwizzle[1];
                        _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->PredicateInfo;
                    }

                    _EmitDstMod(DstReg,DstWriteMask);
                }
                break;
            case D3DSIO_TEXM3x2PAD:
                {
                    PSTRRegister CoordReg;
                    CoordReg._Set(PSTRREG_TEXTURE,D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam));

                    // do row of transform - tex coord * vector loaded from texture (on previous stage)
                    _NewPSInst(PSTRINST_DP3);
                    _InstParam(PSTRINST_DP3).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP3).SrcReg1            = CoordReg;
                    _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).WriteMask          = PSTR_COMPONENTMASK_0;
                    _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->ForceNoPredication;
                }
                break;
            case D3DSIO_TEXM3x3PAD:
                {
                    BOOL bSecondPad = (D3DSIO_TEXM3x3PAD != ((m_pWorkerData->pInst + 1)->Opcode & D3DSI_OPCODE_MASK));
                    BOOL bInVSPECSequence = (D3DSIO_TEXM3x3VSPEC == (((m_pWorkerData->pInst + (bSecondPad?1:2))->Opcode) & D3DSI_OPCODE_MASK));
                    PSTRRegister CoordReg, EyeReg;
                    CoordReg._Set(PSTRREG_TEXTURE,D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam));
                    EyeReg._Set(PSTRREG_SCRATCH,0);

                    // do row of transform - tex coord * vector loaded from texture (on previous stage)
                    _NewPSInst(PSTRINST_DP3);
                    _InstParam(PSTRINST_DP3).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP3).DstReg._UpdateRegNum(DstReg.GetRegNum()-(bSecondPad?1:0));
                    _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP3).SrcReg1            = CoordReg;
                    _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).WriteMask          = bSecondPad?PSTR_COMPONENTMASK_1:PSTR_COMPONENTMASK_0;
                    _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->ForceNoPredication;

                    if(bInVSPECSequence)
                    {
                        // eye vector encoded in 4th element of texture coordinates
                        _NewPSInst(PSTRINST_SWIZZLE);
                        _InstParam(PSTRINST_SWIZZLE).DstReg         = EyeReg;
                        _InstParam(PSTRINST_SWIZZLE).SrcReg0        = CoordReg;
                        _InstParam(PSTRINST_SWIZZLE).WriteMask      = bSecondPad?PSTR_COMPONENTMASK_1:PSTR_COMPONENTMASK_0;
                        _InstParam(PSTRINST_SWIZZLE).Swizzle        = PSTR_REPLICATEALPHA;
                        _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;
                    }
                }
                break;
            case D3DSIO_TEXM3x2TEX:
            case D3DSIO_TEXM3x3:
            case D3DSIO_TEXM3x3TEX:
            case D3DSIO_TEXM3x3SPEC:
            case D3DSIO_TEXM3x3VSPEC:
            case D3DSIO_TEXM3x2DEPTH:
                {
                    BOOL bIs3D = (D3DSIO_TEXM3x2TEX != Opcode) && (D3DSIO_TEXM3x2DEPTH != Opcode);
                    PSTRRegister CoordReg, EyeReg, XFormCoordReg;
                    UINT uiStage = D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);
                    CoordReg._Set(PSTRREG_TEXTURE,D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam));
                    XFormCoordReg._Set(PSTRREG_TEXTURE,D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam)-(bIs3D?2:1));
                    EyeReg._Set(PSTRREG_SCRATCH,0);

                    // do row of transform - tex coord * vector loaded from texture (on previous stage)
                    _NewPSInst(PSTRINST_DP3);
                    _InstParam(PSTRINST_DP3).DstReg             = XFormCoordReg;
                    _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP3).SrcReg1            = CoordReg;
                    _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).WriteMask          = (bIs3D) ? PSTR_COMPONENTMASK_2 : PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->ForceNoPredication;

                    if(D3DSIO_TEXM3x3VSPEC == Opcode)
                    {
                        // eye vector encoded in 4th element of texture coordinates
                        _NewPSInst(PSTRINST_SWIZZLE);
                        _InstParam(PSTRINST_SWIZZLE).DstReg         = EyeReg;
                        _InstParam(PSTRINST_SWIZZLE).SrcReg0        = CoordReg;
                        _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_SWIZZLE).Swizzle        = PSTR_REPLICATEALPHA;
                        _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;
                    }

                    // Now do stuff that depends on which TEXM3x* instruction this is...

                    if( D3DSIO_TEXM3x3 == Opcode )
                    {
                        _NewPSInst(PSTRINST_MOV);
                        _InstParam(PSTRINST_MOV).DstReg             = DstReg;
                        _InstParam(PSTRINST_MOV).SrcReg0            = XFormCoordReg;
                        _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MOV).WriteMask          = PSTR_COMPONENTMASK_0|PSTR_COMPONENTMASK_1|PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_MOV);
                        _InstParam(PSTRINST_MOV).DstReg             = DstReg;
                        _InstParam(PSTRINST_MOV).SrcReg0            = OneReg; // 1.0f
                        _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MOV).WriteMask          = PSTR_COMPONENTMASK_3;
                        _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;

                        _EmitDstMod(DstReg,DstWriteMask);
                    }
                    else if ( (D3DSIO_TEXM3x2TEX == Opcode) ||
                              (D3DSIO_TEXM3x3TEX == Opcode) )
                    {
                        // do straight lookup with transformed tex coords - this
                        // vector is not normalized, but normalization is not necessary
                        // for a cubemap lookup

                        if( !bIs3D )
                        {
                            _NewPSInst(PSTRINST_MOV);
                            _InstParam(PSTRINST_MOV).DstReg             = XFormCoordReg;
                            _InstParam(PSTRINST_MOV).SrcReg0            = ZeroReg; // 0.0f
                            _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                            _InstParam(PSTRINST_MOV).WriteMask          = PSTR_COMPONENTMASK_2;
                            _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;
                        }

                        // compute gradients for diffuse lookup
                        _LeaveQuadPixelLoop();

                        PSTRRegister XGradient, YGradient;
                        BYTE GradientMask = bIs3D ? PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2 :
                                                    PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                        XGradient._Set(PSTRREG_XGRADIENT,0);
                        YGradient._Set(PSTRREG_YGRADIENT,0);

                        _NewPSInst(PSTRINST_DSX);
                        _InstParam(PSTRINST_DSX).DstReg             = XGradient;
                        _InstParam(PSTRINST_DSX).SrcReg0            = XFormCoordReg;
                        _InstParam(PSTRINST_DSX).WriteMask          = GradientMask;
                        _InstParam(PSTRINST_DSX).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_DSX).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_DSY);
                        _InstParam(PSTRINST_DSY).DstReg             = YGradient;
                        _InstParam(PSTRINST_DSY).SrcReg0            = XFormCoordReg;
                        _InstParam(PSTRINST_DSY).WriteMask          = GradientMask;
                        _InstParam(PSTRINST_DSY).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_DSY).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_TEXCOVERAGE);
                        _InstParam(PSTRINST_TEXCOVERAGE).uiStage            = uiStage;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcXGradient       = XGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcYGradient       = YGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_TEXCOVERAGE).bAllowLegacyApproximations = bAllowLegacyApproximations;

                        _EnterQuadPixelLoop();

                        // do lookup
                        _NewPSInst(PSTRINST_SAMPLE);
                        _InstParam(PSTRINST_SAMPLE).DstReg          = DstReg;
                        _InstParam(PSTRINST_SAMPLE).CoordReg        = XFormCoordReg;
                        _InstParam(PSTRINST_SAMPLE).WriteMask       = DstWriteMask;
                        _InstParam(PSTRINST_SAMPLE).uiStage         = uiStage;
                        _InstParam(PSTRINST_SAMPLE).Predication     = m_pWorkerData->ForceNoPredication;
                        _InstParam(PSTRINST_SAMPLE).bAllowLegacyApproximations = bAllowLegacyApproximations;
                        _InstParam(PSTRINST_SAMPLE).bLODBiasFromW   = FALSE;
                        _InstParam(PSTRINST_SAMPLE).bForceLODFromW   = FALSE;
                        _InstParam(PSTRINST_SAMPLE).bAlternateGradient = FALSE;

                        _EmitDstMod(DstReg,DstWriteMask);
                    }
                    else if ( Opcode == D3DSIO_TEXM3x2DEPTH )
                    {
                        // Take resulting r,g values and compute r/g, which
                        // can be interpreted as z/w = perspective correct depth.
                        // Then set the z coord for the pixel.
                        // The denominator is in g.

                        // First, check if denominator is 0.  If so, result of r/g = 1.
                        _NewPSInst(PSTRINST_MUL);
                        _InstParam(PSTRINST_MUL).DstReg._Set(PSTRREG_SCRATCH,0);
                        _InstParam(PSTRINST_MUL).SrcReg0            = XFormCoordReg;
                        _InstParam(PSTRINST_MUL).SrcReg1            = XFormCoordReg;
                        _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_1;
                        _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_SWIZZLE);
                        _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_SCRATCH,0);
                        _InstParam(PSTRINST_SWIZZLE).SrcReg0._Set(PSTRREG_SCRATCH,0);
                        _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_0;
                        _InstParam(PSTRINST_SWIZZLE).Swizzle        = PSTR_REPLICATEGREEN;
                        _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;

                        // If denominator is 0, set numerator and denominator to 1, so divide will yield 1.
                        _NewPSInst(PSTRINST_CMP);
                        _InstParam(PSTRINST_CMP).DstReg             = XFormCoordReg;
                        _InstParam(PSTRINST_CMP).SrcReg0._Set(PSTRREG_SCRATCH,0);
                        _InstParam(PSTRINST_CMP).SrcReg1            = OneReg;
                        _InstParam(PSTRINST_CMP).SrcReg2            = XFormCoordReg;
                        _InstParam(PSTRINST_CMP).bSrcReg0_Negate    = TRUE;
                        _InstParam(PSTRINST_CMP).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_CMP).bSrcReg2_Negate    = FALSE;
                        _InstParam(PSTRINST_CMP).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                        _InstParam(PSTRINST_CMP).Predication        = m_pWorkerData->ForceNoPredication;

                        // Now do the actual divide
                        _NewPSInst(PSTRINST_LEGACYRCP);
                        _InstParam(PSTRINST_LEGACYRCP).DstReg._Set(PSTRREG_SCRATCH,0);
                        _InstParam(PSTRINST_LEGACYRCP).SrcReg0            = XFormCoordReg;
                        _InstParam(PSTRINST_LEGACYRCP).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_LEGACYRCP).WriteMask          = PSTR_COMPONENTMASK_0;
                        _InstParam(PSTRINST_LEGACYRCP).SrcReg0_Selector   = PSTR_SELECT_G;
                        _InstParam(PSTRINST_LEGACYRCP).fRangeMax          = fMax;
                        _InstParam(PSTRINST_LEGACYRCP).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_MUL);
                        _InstParam(PSTRINST_MUL).DstReg             = DstReg;
                        _InstParam(PSTRINST_MUL).SrcReg0            = XFormCoordReg;
                        _InstParam(PSTRINST_MUL).SrcReg1._Set(PSTRREG_SCRATCH,0);
                        _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_0;
                        _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_DEPTH);
                        _InstParam(PSTRINST_DEPTH).SrcReg0          = DstReg;
                    }
                    else if ( (Opcode == D3DSIO_TEXM3x3SPEC) ||
                              (Opcode == D3DSIO_TEXM3x3VSPEC) )
                    {
                        PSTRRegister NdotE, NdotN, TwoNdotE, EbyNdotN, NbyTwoNdotE, ReflReg, XGradient, YGradient;
                        NdotE._Set(PSTRREG_SCRATCH,1);
                        NdotN._Set(PSTRREG_SCRATCH,2);
                        TwoNdotE = NbyTwoNdotE = NdotE;    // reuse same register
                        EbyNdotN = NdotN;                  // reuse same register
                        ReflReg  = CoordReg;               // reuse same register

                        // compute reflection vector and do lookup - the normal needs
                        // to be normalized here, which is included in this expression
                        if (D3DSIO_TEXM3x3SPEC == Opcode)
                        {
                            // eye vector is constant register
                            EyeReg = SrcReg[1];
                        } // else (TEXM3x3VSPEC) -> eye is what was copied out of the 4th component of 3 texcoords


                        // Compute (non-unit length) exact reflection vector:
                        // N * 2(NdotE) - E * (NdotN)

                        // Calculate NdotE
                        _NewPSInst(PSTRINST_DP3);
                        _InstParam(PSTRINST_DP3).DstReg             = NdotE;
                        _InstParam(PSTRINST_DP3).SrcReg0            = XFormCoordReg; // N
                        _InstParam(PSTRINST_DP3).SrcReg1            = EyeReg; // E
                        _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_DP3).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->ForceNoPredication;

                        // Calculate NdotN
                        _NewPSInst(PSTRINST_DP3);
                        _InstParam(PSTRINST_DP3).DstReg             = NdotN;
                        _InstParam(PSTRINST_DP3).SrcReg0            = XFormCoordReg; // N
                        _InstParam(PSTRINST_DP3).SrcReg1            = XFormCoordReg; // N
                        _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_DP3).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->ForceNoPredication;

                        // Calculate 2*NdotE = NdotE + NdotE:
                        _NewPSInst(PSTRINST_ADD);
                        _InstParam(PSTRINST_ADD).DstReg             = TwoNdotE;
                        _InstParam(PSTRINST_ADD).SrcReg0            = NdotE;
                        _InstParam(PSTRINST_ADD).SrcReg1            = NdotE;
                        _InstParam(PSTRINST_ADD).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_ADD).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_ADD).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_ADD).Predication        = m_pWorkerData->ForceNoPredication;

                        // Calculate N * 2*NdotE
                        _NewPSInst(PSTRINST_MUL);
                        _InstParam(PSTRINST_MUL).DstReg             = NbyTwoNdotE;
                        _InstParam(PSTRINST_MUL).SrcReg0            = XFormCoordReg; // N
                        _InstParam(PSTRINST_MUL).SrcReg1            = TwoNdotE;
                        _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->ForceNoPredication;

                        // Calculate E * NdotN
                        _NewPSInst(PSTRINST_MUL);
                        _InstParam(PSTRINST_MUL).DstReg             = EbyNdotN;
                        _InstParam(PSTRINST_MUL).SrcReg0            = EyeReg;
                        _InstParam(PSTRINST_MUL).SrcReg1            = NdotN;
                        _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                        _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->ForceNoPredication;

                        // Calculate reflection = N * 2(NdotE) - E * (NdotN)
                        _NewPSInst(PSTRINST_ADD);
                        _InstParam(PSTRINST_ADD).DstReg             = ReflReg;
                        _InstParam(PSTRINST_ADD).SrcReg0            = NbyTwoNdotE;
                        _InstParam(PSTRINST_ADD).SrcReg1            = EbyNdotN;
                        _InstParam(PSTRINST_ADD).bSrcReg0_Negate    = FALSE;
                        _InstParam(PSTRINST_ADD).bSrcReg1_Negate    = TRUE;
                        _InstParam(PSTRINST_ADD).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_ADD).Predication        = m_pWorkerData->ForceNoPredication;

                        // compute gradients for reflection lookup
                        _LeaveQuadPixelLoop();

                        XGradient._Set(PSTRREG_XGRADIENT,0);
                        YGradient._Set(PSTRREG_YGRADIENT,0);

                        _NewPSInst(PSTRINST_DSX);
                        _InstParam(PSTRINST_DSX).DstReg             = XGradient;
                        _InstParam(PSTRINST_DSX).SrcReg0            = ReflReg;
                        _InstParam(PSTRINST_DSX).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_DSX).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_DSX).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_DSY);
                        _InstParam(PSTRINST_DSY).DstReg             = YGradient;
                        _InstParam(PSTRINST_DSY).SrcReg0            = ReflReg;
                        _InstParam(PSTRINST_DSY).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        _InstParam(PSTRINST_DSY).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_DSY).Predication        = m_pWorkerData->ForceNoPredication;

                        _NewPSInst(PSTRINST_TEXCOVERAGE);
                        _InstParam(PSTRINST_TEXCOVERAGE).uiStage            = uiStage;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcXGradient       = XGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).SrcYGradient       = YGradient;
                        _InstParam(PSTRINST_TEXCOVERAGE).bQuadPixelShared   = TRUE;
                        _InstParam(PSTRINST_TEXCOVERAGE).bAllowLegacyApproximations = bAllowLegacyApproximations;

                        _EnterQuadPixelLoop();

                        // do lookup
                        _NewPSInst(PSTRINST_SAMPLE);
                        _InstParam(PSTRINST_SAMPLE).DstReg          = DstReg;
                        _InstParam(PSTRINST_SAMPLE).CoordReg        = ReflReg;
                        _InstParam(PSTRINST_SAMPLE).WriteMask       = DstWriteMask;
                        _InstParam(PSTRINST_SAMPLE).uiStage         = D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);
                        _InstParam(PSTRINST_SAMPLE).Predication     = m_pWorkerData->ForceNoPredication;
                        _InstParam(PSTRINST_SAMPLE).bAllowLegacyApproximations = bAllowLegacyApproximations;
                        _InstParam(PSTRINST_SAMPLE).bLODBiasFromW   = FALSE;
                        _InstParam(PSTRINST_SAMPLE).bForceLODFromW  = FALSE;
                        _InstParam(PSTRINST_SAMPLE).bAlternateGradient = FALSE;
                        _EmitDstMod(DstReg,DstWriteMask);
                    }
                }
                break;
            // Arithmetic ops
            case D3DSIO_ABS:
                _NewPSInst(PSTRINST_ABS);
                _InstParam(PSTRINST_ABS).DstReg             = DstReg;
                _InstParam(PSTRINST_ABS).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_ABS).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_ABS).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_ADD:
                _NewPSInst(PSTRINST_ADD);
                _InstParam(PSTRINST_ADD).DstReg             = DstReg;
                _InstParam(PSTRINST_ADD).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_ADD).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_ADD).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_ADD).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_ADD).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_ADD).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_BEM:
                _NewPSInst(PSTRINST_BEM);
                _InstParam(PSTRINST_BEM).DstReg             = DstReg;
                _InstParam(PSTRINST_BEM).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_BEM).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_BEM).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_BEM).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_BEM).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_BEM).uiStage            = D3DSI_GETREGNUM(m_pWorkerData->pInst->DstParam);
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_BREAK:
                _NewPSInst(PSTRINST_BREAK);
                _InstParam(PSTRINST_BREAK).Predication = m_pWorkerData->PredicateInfo;
                /*_LeaveQuadPixelLoop();
                _NewPSInst(PSTRINST_JUMP);
                _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Predication = m_pWorkerData->PredicateInfo;
                m_LoopNestTracker.Break( _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                         _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID));
                _EnterQuadPixelLoop();*/
                break;
            case D3DSIO_BREAKC:
                _NewPSInst(PSTRINST_SETPRED);
                _InstParam(PSTRINST_SETPRED).DstReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                _InstParam(PSTRINST_SETPRED).SrcReg0    = SrcReg[0];
                _InstParam(PSTRINST_SETPRED).SrcReg1    = SrcReg[1];
                _InstParam(PSTRINST_SETPRED).bSrcReg0_Negate = bSrcNegate[0];
                _InstParam(PSTRINST_SETPRED).bSrcReg1_Negate = bSrcNegate[1];
                _InstParam(PSTRINST_SETPRED).Comparison = D3DSI_GETCOMPARISON(OpcodeSpecificControl);
                _InstParam(PSTRINST_SETPRED).WriteMask = PSTR_COMPONENTMASK_0; // just use x for predicate

                _NewPSInst(PSTRINST_BREAK);
                _InstParam(PSTRINST_BREAK).Predication.PredicateReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                _InstParam(PSTRINST_BREAK).Predication.bInvertPredicate = FALSE;
                _InstParam(PSTRINST_BREAK).Predication.PredicateSwizzle = PSTR_REPLICATERED;

                /*_LeaveQuadPixelLoop();
                _NewPSInst(PSTRINST_JUMP);
                _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Predication.PredicateReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                _InstParam(PSTRINST_JUMP).Predication.bInvertPredicate = FALSE;
                _InstParam(PSTRINST_JUMP).Predication.PredicateSwizzle = PSTR_REPLICATERED;
                m_LoopNestTracker.Break( _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                         _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID));
                _EnterQuadPixelLoop();*/
                break;
            case D3DSIO_BREAKP:
                _NewPSInst(PSTRINST_BREAK);
                _InstParam(PSTRINST_BREAK).Predication.PredicateReg = SrcReg[0];
                _InstParam(PSTRINST_BREAK).Predication.bInvertPredicate = bSrcNOT[0];
                _InstParam(PSTRINST_BREAK).Predication.PredicateSwizzle = SrcSwizzle[0];
                /*_LeaveQuadPixelLoop();
                _NewPSInst(PSTRINST_JUMP);
                _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Predication.PredicateReg = SrcReg[0];
                _InstParam(PSTRINST_JUMP).Predication.bInvertPredicate = bSrcNOT[0];
                _InstParam(PSTRINST_JUMP).Predication.PredicateSwizzle = SrcSwizzle[0];
                m_LoopNestTracker.Break( _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                         _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID));
                _EnterQuadPixelLoop();*/
                break;
            case D3DSIO_CALL:
                /*_LeaveQuadPixelLoop();
                _NewPSInst(PSTRINST_CALL);
                _InstParam(PSTRINST_CALL).Destination_Offset = _BAD_VALUE;
                _InstParam(PSTRINST_CALL).Destination_PSTRInstID = _BAD_VALUE;
                m_LabelTracker.NeedLabel(D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[0]),
                                         _GetParamOffset(&_InstParam(PSTRINST_CALL).Destination_Offset),
                                         _GetParamOffset(&_InstParam(PSTRINST_CALL).Destination_PSTRInstID),
                                         m_pWorkerData->pPSTRInstBuffer);
                _EnterQuadPixelLoop();*/
                _NewPSInst(PSTRINST_CALL);
                _InstParam(PSTRINST_CALL).Label = D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[0]);
                break;

            case D3DSIO_LABEL:
                _NewPSInst(PSTRINST_DEFINESUB);
                _InstParam(PSTRINST_DEFINESUB).Label = D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[0]);
                break;

            case D3DSIO_CALLNZ:
                {
                    if( D3DSPR_PREDICATE == D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcParam[1]) )
                    {
                        _NewPSInst(PSTRINST_CALLNZ);
                        _InstParam(PSTRINST_CALLNZ).Label = D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[0]);
                        _InstParam(PSTRINST_CALLNZ).SrcReg0 = SrcReg[1];
                        _InstParam(PSTRINST_CALLNZ).bInvertPredicate = bSrcNOT[1];
                        _InstParam(PSTRINST_CALLNZ).PredSwizzle = SrcSwizzle[1];
                    }
                    else
                    {
                        _NewPSInst(PSTRINST_LOADCONSTBOOL);
                        _InstParam(PSTRINST_LOADCONSTBOOL).DstReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                        _InstParam(PSTRINST_LOADCONSTBOOL).SrcReg0 = SrcReg[1];

                        _NewPSInst(PSTRINST_CALLNZ);
                        _InstParam(PSTRINST_CALLNZ).Label = D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[0]);
                        _InstParam(PSTRINST_CALLNZ).SrcReg0._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                        _InstParam(PSTRINST_CALLNZ).bInvertPredicate = bSrcNOT[1];
                        _InstParam(PSTRINST_CALLNZ).PredSwizzle = PSTR_REPLICATERED;
                    }
                }

                /*{
                    size_t OffsetOfJumpDestOffset, OffsetOfJumpDestPSTRInstID;
                    PSTRRegister Pred;
                    BYTE PredSwizzle;
                    
                    if( D3DSPR_PREDICATE == D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcParam[1]) )
                    {   
                        Pred = SrcReg[1];
                        PredSwizzle = SrcSwizzle[1];
                    }
                    else
                    {
                        Pred._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                        PredSwizzle = PSTR_REPLICATERED;

                        _NewPSInst(PSTRINST_SETPRED);
                        _InstParam(PSTRINST_SETPRED).DstReg = Pred;
                        _InstParam(PSTRINST_SETPRED).SrcReg0    = SrcReg[1];
                        _InstParam(PSTRINST_SETPRED).SrcReg1    = ZeroReg;
                        _InstParam(PSTRINST_SETPRED).bSrcReg0_Negate = FALSE;
                        _InstParam(PSTRINST_SETPRED).bSrcReg1_Negate = FALSE;
                        _InstParam(PSTRINST_SETPRED).Comparison = D3DSPC_NE; 
                        _InstParam(PSTRINST_SETPRED).WriteMask = PSTR_COMPONENTMASK_0; // just use x for predicate
                    }

                    _LeaveQuadPixelLoop();
                    _NewPSInst(PSTRINST_JUMP);
                    _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateReg = Pred;
                    _InstParam(PSTRINST_JUMP).Predication.bInvertPredicate = !bSrcNOT[1]; // flip.
                    _InstParam(PSTRINST_JUMP).Predication.PredicateSwizzle = PredSwizzle;

                    OffsetOfJumpDestOffset = _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset);
                    OffsetOfJumpDestPSTRInstID = _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID);

                    _NewPSInst(PSTRINST_CALL);
                    _InstParam(PSTRINST_CALL).Destination_Offset = _BAD_VALUE;
                    _InstParam(PSTRINST_CALL).Destination_PSTRInstID = _BAD_VALUE;
                    m_LabelTracker.NeedLabel(D3DSI_GETREGNUM(m_pWorkerData->pInst->SrcParam[0]),
                                            _GetParamOffset(&_InstParam(PSTRINST_CALL).Destination_Offset),
                                            _GetParamOffset(&_InstParam(PSTRINST_CALL).Destination_PSTRInstID),
                                            m_pWorkerData->pPSTRInstBuffer);

                    *(size_t*)(m_pWorkerData->pPSTRInstBuffer + OffsetOfJumpDestOffset) = _GetNextInstOffset();
                    *(PSTR_INST_ID*)(m_pWorkerData->pPSTRInstBuffer + OffsetOfJumpDestPSTRInstID) = _GetNextPSTRInstID();
                    _EnterQuadPixelLoop();
                }*/
                break;
            case D3DSIO_CMP:
                _NewPSInst(PSTRINST_CMP);
                _InstParam(PSTRINST_CMP).DstReg             = DstReg;
                _InstParam(PSTRINST_CMP).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_CMP).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_CMP).SrcReg2            = SrcReg[2];
                _InstParam(PSTRINST_CMP).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_CMP).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_CMP).bSrcReg2_Negate    = bSrcNegate[2];
                _InstParam(PSTRINST_CMP).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_CMP).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_CND:
                _NewPSInst(PSTRINST_CND);
                _InstParam(PSTRINST_CND).DstReg             = DstReg;
                _InstParam(PSTRINST_CND).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_CND).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_CND).SrcReg2            = SrcReg[2];
                _InstParam(PSTRINST_CND).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_CND).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_CND).bSrcReg2_Negate    = bSrcNegate[2];
                _InstParam(PSTRINST_CND).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_CND).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_CRS:
                _NewPSInst(PSTRINST_SWIZZLE);
                _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[0];
                _InstParam(PSTRINST_SWIZZLE).WriteMask      = DstWriteMask;
                _InstParam(PSTRINST_SWIZZLE).Swizzle        = _Swizzle(PSTR_SELECT_B,PSTR_SELECT_R,
                                                                       PSTR_SELECT_G,PSTR_SELECT_A);
                _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_SWIZZLE);
                _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_SCRATCH,1);
                _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[1];
                _InstParam(PSTRINST_SWIZZLE).WriteMask      = DstWriteMask;
                _InstParam(PSTRINST_SWIZZLE).Swizzle        = _Swizzle(PSTR_SELECT_G,PSTR_SELECT_B,
                                                                       PSTR_SELECT_R,PSTR_SELECT_A);
                _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_MUL);
                _InstParam(PSTRINST_MUL).DstReg             = DstReg;
                _InstParam(PSTRINST_MUL).SrcReg0._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_MUL).SrcReg1._Set(PSTRREG_SCRATCH,1);
                _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_MUL).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->PredicateInfo;

                _NewPSInst(PSTRINST_SWIZZLE);
                _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[0];
                _InstParam(PSTRINST_SWIZZLE).WriteMask      = DstWriteMask;
                _InstParam(PSTRINST_SWIZZLE).Swizzle        = _Swizzle(PSTR_SELECT_G,PSTR_SELECT_B,
                                                                       PSTR_SELECT_R,PSTR_SELECT_A);
                _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_SWIZZLE);
                _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_SCRATCH,1);
                _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[1];
                _InstParam(PSTRINST_SWIZZLE).WriteMask      = DstWriteMask;
                _InstParam(PSTRINST_SWIZZLE).Swizzle        = _Swizzle(PSTR_SELECT_B,PSTR_SELECT_R,
                                                                       PSTR_SELECT_G,PSTR_SELECT_A);
                _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_MAD);
                _InstParam(PSTRINST_MAD).DstReg             = DstReg;
                _InstParam(PSTRINST_MAD).SrcReg0._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_MAD).SrcReg1._Set(PSTRREG_SCRATCH,1);
                _InstParam(PSTRINST_MAD).SrcReg2            = DstReg;
                _InstParam(PSTRINST_MAD).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_MAD).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_MAD).bSrcReg2_Negate    = TRUE;
                _InstParam(PSTRINST_MAD).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_MAD).Predication        = m_pWorkerData->PredicateInfo;

                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_DP2ADD:
                _NewPSInst(PSTRINST_DP2ADD);
                _InstParam(PSTRINST_DP2ADD).DstReg              = DstReg;
                _InstParam(PSTRINST_DP2ADD).SrcReg0             = SrcReg[0];
                _InstParam(PSTRINST_DP2ADD).SrcReg1             = SrcReg[1];
                _InstParam(PSTRINST_DP2ADD).SrcReg2             = SrcReg[2];
                _InstParam(PSTRINST_DP2ADD).bSrcReg0_Negate     = bSrcNegate[0];
                _InstParam(PSTRINST_DP2ADD).bSrcReg1_Negate     = bSrcNegate[1];
                _InstParam(PSTRINST_DP2ADD).bSrcReg2_Negate     = bSrcNegate[2];
                _InstParam(PSTRINST_DP2ADD).WriteMask           = DstWriteMask;
                _InstParam(PSTRINST_DP2ADD).Predication         = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_DP3:
                _NewPSInst(PSTRINST_DP3);
                _InstParam(PSTRINST_DP3).DstReg             = DstReg;
                _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_DP3).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_DP3).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_DP4:
                _NewPSInst(PSTRINST_DP4);
                _InstParam(PSTRINST_DP4).DstReg             = DstReg;
                _InstParam(PSTRINST_DP4).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_DP4).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_DP4).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_DP4).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_DP4).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_DP4).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_DSX:
                _LeaveQuadPixelLoop();
                _NewPSInst(PSTRINST_DSX);
                _InstParam(PSTRINST_DSX).DstReg             = DstReg;
                _InstParam(PSTRINST_DSX).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_DSX).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_DSX).bQuadPixelShared   = FALSE;
                _InstParam(PSTRINST_DSX).Predication        = m_pWorkerData->PredicateInfo;
                _EnterQuadPixelLoop();
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_DSY:
                _LeaveQuadPixelLoop();
                _NewPSInst(PSTRINST_DSY);
                _InstParam(PSTRINST_DSY).DstReg             = DstReg;
                _InstParam(PSTRINST_DSY).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_DSY).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_DSY).bQuadPixelShared   = FALSE;
                _InstParam(PSTRINST_DSY).Predication        = m_pWorkerData->PredicateInfo;
                _EnterQuadPixelLoop();
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_ELSE:
                /*_LeaveQuadPixelLoop();
                _NewPSInst(PSTRINST_JUMP);
                _InstParam(PSTRINST_JUMP).Destination_Offset        = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Destination_PSTRInstID    = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Predication               = m_pWorkerData->ForceNoPredication;
                m_IfNestTracker.Else(_GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                     _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID),
                                     _GetNextInstOffset(),
                                     _GetNextPSTRInstID(),
                                     m_pWorkerData->pPSTRInstBuffer);
                _EnterQuadPixelLoop();*/
                _NewPSInst(PSTRINST_ELSE);
                break;
            case D3DSIO_ENDIF:
                /*_LeaveQuadPixelLoop();
                m_IfNestTracker.Endif(_GetNextInstOffset(),
                                      _GetNextPSTRInstID(),
                                      m_pWorkerData->pPSTRInstBuffer);
                _EnterQuadPixelLoop();*/
                _NewPSInst(PSTRINST_ENDIF);
                break;
            case D3DSIO_ENDLOOP:
                {
                    _NewPSInst(PSTRINST_ENDLOOP);
                    // Decrement loop counter
                    /*_NewPSInst(PSTRINST_ADD);
                    _InstParam(PSTRINST_ADD).DstReg             = InternalLoopCounterReg;
                    _InstParam(PSTRINST_ADD).SrcReg0            = InternalLoopCounterReg;
                    _InstParam(PSTRINST_ADD).SrcReg1            = OneReg;
                    _InstParam(PSTRINST_ADD).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_ADD).bSrcReg1_Negate    = TRUE;
                    _InstParam(PSTRINST_ADD).WriteMask          = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];
                    _InstParam(PSTRINST_ADD).Predication        = m_pWorkerData->ForceNoPredication;

                    // Increment external loop count
                    _NewPSInst(PSTRINST_SWIZZLE);
                    _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_SCRATCH,0);
                    _InstParam(PSTRINST_SWIZZLE).SrcReg0        = LoopCounterReg;
                    _InstParam(PSTRINST_SWIZZLE).WriteMask      = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];
                    _InstParam(PSTRINST_SWIZZLE).Swizzle        = _SwizzleFromSelector(PSTR_LOOPCOUNT_INCREMENT_SELECTOR);
                    _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;

                    _NewPSInst(PSTRINST_ADD);
                    _InstParam(PSTRINST_ADD).DstReg             = LoopCounterReg;
                    _InstParam(PSTRINST_ADD).SrcReg0            = LoopCounterReg;
                    _InstParam(PSTRINST_ADD).SrcReg1._Set(PSTRREG_SCRATCH,0);
                    _InstParam(PSTRINST_ADD).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_ADD).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_ADD).WriteMask          = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];
                    _InstParam(PSTRINST_ADD).Predication        = m_pWorkerData->ForceNoPredication;


                    // Check loop condition

                    _NewPSInst(PSTRINST_SETPRED);
                    _InstParam(PSTRINST_SETPRED).DstReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                    _InstParam(PSTRINST_SETPRED).SrcReg0    = InternalLoopCounterReg;
                    _InstParam(PSTRINST_SETPRED).SrcReg1    = ZeroReg;
                    _InstParam(PSTRINST_SETPRED).bSrcReg0_Negate = FALSE;
                    _InstParam(PSTRINST_SETPRED).bSrcReg1_Negate = FALSE;
                    _InstParam(PSTRINST_SETPRED).Comparison = D3DSPC_GT;
                    _InstParam(PSTRINST_SETPRED).WriteMask = 1<<PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR;

                    _LeaveQuadPixelLoop();
                    _NewPSInst(PSTRINST_JUMP);
                    _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                    _InstParam(PSTRINST_JUMP).Predication.bInvertPredicate = FALSE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateSwizzle = _SwizzleFromSelector(PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR);

                    m_LoopNestTracker.LoopEnd(TRUE, // FALSE means this is REP (not LOOP)
                                              _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                              _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID),
                                              _GetNextInstOffset(),
                                              _GetNextPSTRInstID(),
                                              m_pWorkerData->pPSTRInstBuffer );

                    // Pop previous internal loop counter contents (restore)
                    _NewPSInst(PSTRINST_POPREG);
                    _InstParam(PSTRINST_POPREG).DstReg         = InternalLoopCounterReg;
                    _InstParam(PSTRINST_POPREG).WriteMask      = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];

                    // Pop previous aL contents (restore)
                    _NewPSInst(PSTRINST_POPREG);
                    _InstParam(PSTRINST_POPREG).DstReg         = LoopCounterReg;
                    _InstParam(PSTRINST_POPREG).WriteMask      = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR] |
                                                                 ComponentMask[PSTR_LOOPCOUNT_INCREMENT_SELECTOR];
                    _EnterQuadPixelLoop();*/

                }
                break;
            case D3DSIO_ENDREP:
                {
                    _NewPSInst(PSTRINST_ENDREP);

                    // Decrement loop counter
                    /*_NewPSInst(PSTRINST_ADD);
                    _InstParam(PSTRINST_ADD).DstReg             = InternalLoopCounterReg;
                    _InstParam(PSTRINST_ADD).SrcReg0            = InternalLoopCounterReg;
                    _InstParam(PSTRINST_ADD).SrcReg1            = OneReg;
                    _InstParam(PSTRINST_ADD).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_ADD).bSrcReg1_Negate    = TRUE;
                    _InstParam(PSTRINST_ADD).WriteMask          = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];
                    _InstParam(PSTRINST_ADD).Predication        = m_pWorkerData->ForceNoPredication;


                    // Check loop condition
                    _NewPSInst(PSTRINST_SETPRED);
                    _InstParam(PSTRINST_SETPRED).DstReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                    _InstParam(PSTRINST_SETPRED).SrcReg0    = InternalLoopCounterReg;
                    _InstParam(PSTRINST_SETPRED).SrcReg1    = ZeroReg;
                    _InstParam(PSTRINST_SETPRED).bSrcReg0_Negate = FALSE;
                    _InstParam(PSTRINST_SETPRED).bSrcReg1_Negate = FALSE;
                    _InstParam(PSTRINST_SETPRED).Comparison = D3DSPC_GT;
                    _InstParam(PSTRINST_SETPRED).WriteMask = 1<<PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR;

                    _LeaveQuadPixelLoop();
                    _NewPSInst(PSTRINST_JUMP);
                    _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                    _InstParam(PSTRINST_JUMP).Predication.bInvertPredicate = FALSE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateSwizzle = _SwizzleFromSelector(PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR);

                    m_LoopNestTracker.LoopEnd(FALSE, // FALSE means this is REP (not LOOP)
                                              _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                              _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID),
                                              _GetNextInstOffset(),
                                              _GetNextPSTRInstID(),
                                              m_pWorkerData->pPSTRInstBuffer );

                    // Pop previous internal loop counter contents (restore)
                    _NewPSInst(PSTRINST_POPREG);
                    _InstParam(PSTRINST_POPREG).DstReg         = InternalLoopCounterReg;
                    _InstParam(PSTRINST_POPREG).WriteMask      = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];

                    _EnterQuadPixelLoop();*/

                }
                break;
            case D3DSIO_EXP:
                _NewPSInst(PSTRINST_EXP);
                _InstParam(PSTRINST_EXP).DstReg             = DstReg;
                _InstParam(PSTRINST_EXP).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_EXP).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_EXP).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_EXP).SrcReg0_Selector   = _SelectorFromSwizzle(SrcSwizzle[0]);
                _InstParam(PSTRINST_EXP).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_FRC:
                _NewPSInst(PSTRINST_FRC);
                _InstParam(PSTRINST_FRC).DstReg             = DstReg;
                _InstParam(PSTRINST_FRC).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_FRC).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_FRC).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_FRC).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_IF:
                {
                    PSTRRegister Pred;
                    BYTE PredSwizzle;
                    
                    if( D3DSPR_PREDICATE == D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcParam[0]) )
                    {   
                        Pred = SrcReg[0];
                        PredSwizzle = SrcSwizzle[0];
                    }
                    else if( D3DSPR_CONSTBOOL == D3DSI_GETREGTYPE(m_pWorkerData->pInst->SrcParam[0]) )
                    {
                        Pred._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                        PredSwizzle = PSTR_REPLICATERED;

                        _NewPSInst(PSTRINST_LOADCONSTBOOL);
                        _InstParam(PSTRINST_LOADCONSTBOOL).DstReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                        _InstParam(PSTRINST_LOADCONSTBOOL).SrcReg0 = SrcReg[0];
                    }
                    else
                    {
                        Pred._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                        PredSwizzle = PSTR_REPLICATERED;

                        _NewPSInst(PSTRINST_SETPRED);
                        _InstParam(PSTRINST_SETPRED).DstReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                        _InstParam(PSTRINST_SETPRED).SrcReg0    = SrcReg[0];
                        _InstParam(PSTRINST_SETPRED).SrcReg1    = ZeroReg;
                        _InstParam(PSTRINST_SETPRED).bSrcReg0_Negate = FALSE;
                        _InstParam(PSTRINST_SETPRED).bSrcReg1_Negate = FALSE;
                        _InstParam(PSTRINST_SETPRED).Comparison = D3DSPC_NE;
                        _InstParam(PSTRINST_SETPRED).WriteMask = PSTR_COMPONENTMASK_0; // just use x for predicate
                    }

                    _NewPSInst(PSTRINST_IF);
                    _InstParam(PSTRINST_IF).Predication.PredicateReg = Pred;
                    _InstParam(PSTRINST_IF).Predication.bInvertPredicate = bSrcNOT[0]; // flip
                    _InstParam(PSTRINST_IF).Predication.PredicateSwizzle = PredSwizzle;


                    /*_LeaveQuadPixelLoop();
                    _NewPSInst(PSTRINST_JUMP);
                    _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateReg = Pred;
                    _InstParam(PSTRINST_JUMP).Predication.bInvertPredicate = !bSrcNOT[0]; // flip
                    _InstParam(PSTRINST_JUMP).Predication.PredicateSwizzle = PredSwizzle;

                    m_IfNestTracker.If( _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                        _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID));
                    _EnterQuadPixelLoop();*/
                }
                break;
            case D3DSIO_IFC:
                _NewPSInst(PSTRINST_SETPRED);
                _InstParam(PSTRINST_SETPRED).DstReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                _InstParam(PSTRINST_SETPRED).SrcReg0    = SrcReg[0];
                _InstParam(PSTRINST_SETPRED).SrcReg1    = SrcReg[1];
                _InstParam(PSTRINST_SETPRED).bSrcReg0_Negate = bSrcNegate[0];
                _InstParam(PSTRINST_SETPRED).bSrcReg1_Negate = bSrcNegate[1];
                _InstParam(PSTRINST_SETPRED).Comparison = D3DSI_GETCOMPARISON(OpcodeSpecificControl);
                _InstParam(PSTRINST_SETPRED).WriteMask = PSTR_COMPONENTMASK_0; // just use x for predicate

                _NewPSInst(PSTRINST_IF);
                _InstParam(PSTRINST_IF).Predication.PredicateReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                _InstParam(PSTRINST_IF).Predication.bInvertPredicate = FALSE;
                _InstParam(PSTRINST_IF).Predication.PredicateSwizzle = PSTR_NOSWIZZLE;

                /*_LeaveQuadPixelLoop();
                _NewPSInst(PSTRINST_JUMP);
                _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                _InstParam(PSTRINST_JUMP).Predication.PredicateReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                _InstParam(PSTRINST_JUMP).Predication.bInvertPredicate = TRUE;
                _InstParam(PSTRINST_JUMP).Predication.PredicateSwizzle = PSTR_NOSWIZZLE;

                m_IfNestTracker.If( _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                    _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID));
                _EnterQuadPixelLoop();*/
                break;
            case D3DSIO_LOG:
                _NewPSInst(PSTRINST_LOG);
                _InstParam(PSTRINST_LOG).DstReg             = DstReg;
                _InstParam(PSTRINST_LOG).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_LOG).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_LOG).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_LOG).SrcReg0_Selector   = _SelectorFromSwizzle(SrcSwizzle[0]);
                _InstParam(PSTRINST_LOG).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_LOOP:
                {
                    _NewPSInst(PSTRINST_BEGINLOOP);
                    _InstParam(PSTRINST_BEGINLOOP).SrcReg0 = SrcReg[1];

                    /*_LeaveQuadPixelLoop();
                    // Push current aL contents (save)
                    _NewPSInst(PSTRINST_PUSHREG);
                    _InstParam(PSTRINST_PUSHREG).SrcReg0        = LoopCounterReg;
                    _InstParam(PSTRINST_PUSHREG).WriteMask      = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR] |
                                                                  ComponentMask[PSTR_LOOPCOUNT_INCREMENT_SELECTOR];
                    // Push current internal loop counter (save)
                    _NewPSInst(PSTRINST_PUSHREG);
                    _InstParam(PSTRINST_PUSHREG).SrcReg0        = InternalLoopCounterReg;
                    _InstParam(PSTRINST_PUSHREG).WriteMask      = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];
                    _EnterQuadPixelLoop();

                    // Initialize internal loop counter
                    _NewPSInst(PSTRINST_MOV);
                    _InstParam(PSTRINST_MOV).DstReg             = InternalLoopCounterReg;
                    _InstParam(PSTRINST_MOV).SrcReg0            = SrcReg[1];
                    _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_MOV).WriteMask          = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];
                    _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;

                    // Initialize aL contents (use PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR channel to store the external loop count)
                    _NewPSInst(PSTRINST_SWIZZLE);
                    _InstParam(PSTRINST_SWIZZLE).DstReg         = LoopCounterReg;
                    _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[1];
                    _InstParam(PSTRINST_SWIZZLE).WriteMask      = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR] |
                                                                  ComponentMask[PSTR_LOOPCOUNT_INCREMENT_SELECTOR];
                    _InstParam(PSTRINST_SWIZZLE).Swizzle        = _Swizzle(PSTR_LOOPCOUNT_INITVALUE_SELECTOR,
                                                                           PSTR_LOOPCOUNT_INCREMENT_SELECTOR, // don't care
                                                                           PSTR_LOOPCOUNT_INCREMENT_SELECTOR,
                                                                           PSTR_LOOPCOUNT_INCREMENT_SELECTOR); // don't care
                    _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;


                    // Do initial check of condition to check for early exit (never entering loop at all)
                    _NewPSInst(PSTRINST_SETPRED);
                    _InstParam(PSTRINST_SETPRED).DstReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                    _InstParam(PSTRINST_SETPRED).SrcReg0    = SrcReg[1];
                    _InstParam(PSTRINST_SETPRED).SrcReg1    = ZeroReg;
                    _InstParam(PSTRINST_SETPRED).bSrcReg0_Negate = FALSE;
                    _InstParam(PSTRINST_SETPRED).bSrcReg1_Negate = FALSE;
                    _InstParam(PSTRINST_SETPRED).Comparison = D3DSPC_LE;
                    _InstParam(PSTRINST_SETPRED).WriteMask = 1<<PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR;

                    _LeaveQuadPixelLoop();
                    _NewPSInst(PSTRINST_JUMP);
                    _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                    _InstParam(PSTRINST_JUMP).Predication.bInvertPredicate = FALSE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateSwizzle = _SwizzleFromSelector(PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR);

                    m_LoopNestTracker.LoopStart(TRUE, // FALSE means this is REP (not LOOP)
                                                _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                                _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID),
                                                _GetNextInstOffset(),
                                                _GetNextPSTRInstID());
                    _EnterQuadPixelLoop();*/
                }
                break;
            case D3DSIO_LRP:
                _NewPSInst(PSTRINST_LRP);
                _InstParam(PSTRINST_LRP).DstReg             = DstReg;
                _InstParam(PSTRINST_LRP).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_LRP).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_LRP).SrcReg2            = SrcReg[2];
                _InstParam(PSTRINST_LRP).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_LRP).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_LRP).bSrcReg2_Negate    = bSrcNegate[2];
                _InstParam(PSTRINST_LRP).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_LRP).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_M3x2:
            case D3DSIO_M3x3:
            case D3DSIO_M3x4:
                if(DstWriteMask & PSTR_COMPONENTMASK_0)
                {
                    _NewPSInst(PSTRINST_DP3);
                    _InstParam(PSTRINST_DP3).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP3).SrcReg1            = SrcReg[1];
                    _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_0;
                    _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->PredicateInfo;
                }

                if(DstWriteMask & PSTR_COMPONENTMASK_1)
                {
                    _NewPSInst(PSTRINST_DP3);
                    _InstParam(PSTRINST_DP3).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP3).SrcReg1            = SrcReg[1];
                    _InstParam(PSTRINST_DP3).SrcReg1._UpdateRegNum(SrcReg[1].GetRegNum()+1);
                    _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->PredicateInfo;
                }

                if((DstWriteMask & PSTR_COMPONENTMASK_2)&&((D3DSIO_M3x3 == Opcode) || (D3DSIO_M3x4 == Opcode)))
                {
                    _NewPSInst(PSTRINST_DP3);
                    _InstParam(PSTRINST_DP3).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP3).SrcReg1            = SrcReg[1];
                    _InstParam(PSTRINST_DP3).SrcReg1._UpdateRegNum(SrcReg[1].GetRegNum()+2);
                    _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_2;
                    _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->PredicateInfo;
                }

                if((DstWriteMask & PSTR_COMPONENTMASK_3)&&(D3DSIO_M3x4 == Opcode))
                {
                    _NewPSInst(PSTRINST_DP3);
                    _InstParam(PSTRINST_DP3).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP3).SrcReg1            = SrcReg[1];
                    _InstParam(PSTRINST_DP3).SrcReg1._UpdateRegNum(SrcReg[1].GetRegNum()+3);
                    _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP3).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_3;
                    _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->PredicateInfo;
                }
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_M4x3:
            case D3DSIO_M4x4:
                if(DstWriteMask & PSTR_COMPONENTMASK_0)
                {
                    _NewPSInst(PSTRINST_DP4);
                    _InstParam(PSTRINST_DP4).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP4).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP4).SrcReg1            = SrcReg[1];
                    _InstParam(PSTRINST_DP4).bSrcReg0_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_DP4).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP4).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_0;
                    _InstParam(PSTRINST_DP4).Predication        = m_pWorkerData->PredicateInfo;
                }

                if(DstWriteMask & PSTR_COMPONENTMASK_1)
                {
                    _NewPSInst(PSTRINST_DP4);
                    _InstParam(PSTRINST_DP4).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP4).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP4).SrcReg1            = SrcReg[1];
                    _InstParam(PSTRINST_DP4).SrcReg1._UpdateRegNum(SrcReg[1].GetRegNum()+1);
                    _InstParam(PSTRINST_DP4).bSrcReg0_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_DP4).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP4).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_DP4).Predication        = m_pWorkerData->PredicateInfo;
                }


                if(DstWriteMask & PSTR_COMPONENTMASK_2)
                {
                    _NewPSInst(PSTRINST_DP4);
                    _InstParam(PSTRINST_DP4).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP4).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP4).SrcReg1            = SrcReg[1];
                    _InstParam(PSTRINST_DP4).SrcReg1._UpdateRegNum(SrcReg[1].GetRegNum()+2);
                    _InstParam(PSTRINST_DP4).bSrcReg0_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_DP4).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP4).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_2;
                    _InstParam(PSTRINST_DP4).Predication        = m_pWorkerData->PredicateInfo;
                }

                if((DstWriteMask & PSTR_COMPONENTMASK_3)&&(D3DSIO_M4x4 == Opcode))
                {
                    _NewPSInst(PSTRINST_DP4);
                    _InstParam(PSTRINST_DP4).DstReg             = DstReg;
                    _InstParam(PSTRINST_DP4).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_DP4).SrcReg1            = SrcReg[1];
                    _InstParam(PSTRINST_DP4).SrcReg1._UpdateRegNum(SrcReg[1].GetRegNum()+3);
                    _InstParam(PSTRINST_DP4).bSrcReg0_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_DP4).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_DP4).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_3;
                    _InstParam(PSTRINST_DP4).Predication        = m_pWorkerData->PredicateInfo;
                }
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_MAD:
                _NewPSInst(PSTRINST_MAD);
                _InstParam(PSTRINST_MAD).DstReg             = DstReg;
                _InstParam(PSTRINST_MAD).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_MAD).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_MAD).SrcReg2            = SrcReg[2];
                _InstParam(PSTRINST_MAD).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_MAD).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_MAD).bSrcReg2_Negate    = bSrcNegate[2];
                _InstParam(PSTRINST_MAD).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_MAD).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_MAX:
                _NewPSInst(PSTRINST_MAX);
                _InstParam(PSTRINST_MAX).DstReg             = DstReg;
                _InstParam(PSTRINST_MAX).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_MAX).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_MAX).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_MAX).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_MAX).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_MAX).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_MIN:
                _NewPSInst(PSTRINST_MIN);
                _InstParam(PSTRINST_MIN).DstReg             = DstReg;
                _InstParam(PSTRINST_MIN).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_MIN).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_MIN).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_MIN).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_MIN).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_MIN).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_MOV:
                _NewPSInst(PSTRINST_MOV);
                _InstParam(PSTRINST_MOV).DstReg             = DstReg;
                _InstParam(PSTRINST_MOV).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_MOV).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_MUL:
                _NewPSInst(PSTRINST_MUL);
                _InstParam(PSTRINST_MUL).DstReg             = DstReg;
                _InstParam(PSTRINST_MUL).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_MUL).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_MUL).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_NRM:
                _NewPSInst(PSTRINST_DP3);
                _InstParam(PSTRINST_DP3).DstReg._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_DP3).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_DP3).SrcReg1            = SrcReg[0];
                _InstParam(PSTRINST_DP3).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_DP3).bSrcReg1_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_DP3).WriteMask          = PSTR_COMPONENTMASK_0;
                _InstParam(PSTRINST_DP3).Predication        = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_RSQ);
                _InstParam(PSTRINST_RSQ).DstReg._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_RSQ).SrcReg0._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_RSQ).bSrcReg0_Negate    = FALSE;
                _InstParam(PSTRINST_RSQ).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_RSQ).SrcReg0_Selector   = PSTR_SELECT_R;
                _InstParam(PSTRINST_RSQ).Predication        = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_MUL);
                _InstParam(PSTRINST_MUL).DstReg             = DstReg;
                _InstParam(PSTRINST_MUL).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_MUL).SrcReg1._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                _InstParam(PSTRINST_MUL).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->PredicateInfo;

                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_POW:
                _NewPSInst(PSTRINST_LOG);
                _InstParam(PSTRINST_LOG).DstReg._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_LOG).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_LOG).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_LOG).WriteMask          = PSTR_COMPONENTMASK_3;
                _InstParam(PSTRINST_LOG).SrcReg0_Selector   = _SelectorFromSwizzle(SrcSwizzle[0]);
                _InstParam(PSTRINST_LOG).Predication        = m_pWorkerData->ForceNoPredication;

                if( PSTR_REPLICATEALPHA != SrcSwizzle[1] )
                {
                    _NewPSInst(PSTRINST_SWIZZLE);
                    _InstParam(PSTRINST_SWIZZLE).DstReg._Set(PSTRREG_SCRATCH,1);
                    _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[1];
                    _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_3;
                    _InstParam(PSTRINST_SWIZZLE).Swizzle        = SrcSwizzle[1];
                    _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->ForceNoPredication;
                    SrcReg[1]._Set(PSTRREG_SCRATCH,1);
                }
                _NewPSInst(PSTRINST_MUL);
                _InstParam(PSTRINST_MUL).DstReg._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_MUL).SrcReg0._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_MUL).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_3;
                _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->ForceNoPredication;

                _NewPSInst(PSTRINST_EXP);
                _InstParam(PSTRINST_EXP).DstReg             = DstReg;
                _InstParam(PSTRINST_EXP).SrcReg0._Set(PSTRREG_SCRATCH,0);
                _InstParam(PSTRINST_EXP).bSrcReg0_Negate    = FALSE;
                _InstParam(PSTRINST_EXP).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_EXP).SrcReg0_Selector   = PSTR_SELECT_A;
                _InstParam(PSTRINST_EXP).Predication        = m_pWorkerData->PredicateInfo;

                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_RCP:
                _NewPSInst(PSTRINST_RCP);
                _InstParam(PSTRINST_RCP).DstReg             = DstReg;
                _InstParam(PSTRINST_RCP).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_RCP).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_RCP).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_RCP).SrcReg0_Selector   = _SelectorFromSwizzle(SrcSwizzle[0]);
                _InstParam(PSTRINST_RCP).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_REP:
                {
                    _NewPSInst(PSTRINST_BEGINREP);
                    _InstParam(PSTRINST_BEGINREP).SrcReg0 = SrcReg[0];

                    /*_LeaveQuadPixelLoop();
                    // Push current internal loop counter (save)
                    _NewPSInst(PSTRINST_PUSHREG);
                    _InstParam(PSTRINST_PUSHREG).SrcReg0        = InternalLoopCounterReg;
                    _InstParam(PSTRINST_PUSHREG).WriteMask      = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];
                    _EnterQuadPixelLoop();

                    // Initialize internal loop counter
                    _NewPSInst(PSTRINST_MOV);
                    _InstParam(PSTRINST_MOV).DstReg             = InternalLoopCounterReg;
                    _InstParam(PSTRINST_MOV).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_MOV).WriteMask          = ComponentMask[PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR];
                    _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;

                    // Do initial check of condition to check for early exit (never entering loop at all)
                    _NewPSInst(PSTRINST_SETPRED);
                    _InstParam(PSTRINST_SETPRED).DstReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                    _InstParam(PSTRINST_SETPRED).SrcReg0    = SrcReg[0];
                    _InstParam(PSTRINST_SETPRED).SrcReg1    = ZeroReg;
                    _InstParam(PSTRINST_SETPRED).bSrcReg0_Negate = FALSE;
                    _InstParam(PSTRINST_SETPRED).bSrcReg1_Negate = FALSE;
                    _InstParam(PSTRINST_SETPRED).Comparison = D3DSPC_LE;
                    _InstParam(PSTRINST_SETPRED).WriteMask = 1<<PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR;

                    _LeaveQuadPixelLoop();
                    _NewPSInst(PSTRINST_JUMP);
                    _InstParam(PSTRINST_JUMP).Destination_Offset = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Destination_PSTRInstID = _BAD_VALUE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateReg._Set(PSTRREG_PREDICATE,PSTR_SCRATCH_PREDICATE_NUM);
                    _InstParam(PSTRINST_JUMP).Predication.bInvertPredicate = FALSE;
                    _InstParam(PSTRINST_JUMP).Predication.PredicateSwizzle = _SelectorFromSwizzle(PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR);

                    m_LoopNestTracker.LoopStart(FALSE, // FALSE means this is REP (not LOOP)
                                                _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_Offset),
                                                _GetParamOffset(&_InstParam(PSTRINST_JUMP).Destination_PSTRInstID),
                                                _GetNextInstOffset(),
                                                _GetNextPSTRInstID());
                    _EnterQuadPixelLoop();*/
                }
                break;
            case D3DSIO_RET:
                //_LeaveQuadPixelLoop();
                _NewPSInst(PSTRINST_RET);
                //_EnterQuadPixelLoop();
                break;
            case D3DSIO_RSQ:
                _NewPSInst(PSTRINST_RSQ);
                _InstParam(PSTRINST_RSQ).DstReg             = DstReg;
                _InstParam(PSTRINST_RSQ).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_RSQ).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_RSQ).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_RSQ).SrcReg0_Selector   = _SelectorFromSwizzle(SrcSwizzle[0]);
                _InstParam(PSTRINST_RSQ).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_SETP:
                _NewPSInst(PSTRINST_SETPRED);
                _InstParam(PSTRINST_SETPRED).DstReg             = DstReg;
                _InstParam(PSTRINST_SETPRED).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_SETPRED).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_SETPRED).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_SETPRED).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(PSTRINST_SETPRED).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_SETPRED).Comparison         = D3DSI_GETCOMPARISON(OpcodeSpecificControl);
                break;
            case D3DSIO_SUB:
                _NewPSInst(PSTRINST_ADD);
                _InstParam(PSTRINST_ADD).DstReg             = DstReg;
                _InstParam(PSTRINST_ADD).SrcReg0            = SrcReg[0];
                _InstParam(PSTRINST_ADD).SrcReg1            = SrcReg[1];
                _InstParam(PSTRINST_ADD).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(PSTRINST_ADD).bSrcReg1_Negate    = !bSrcNegate[1];
                _InstParam(PSTRINST_ADD).WriteMask          = DstWriteMask;
                _InstParam(PSTRINST_ADD).Predication        = m_pWorkerData->PredicateInfo;
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            case D3DSIO_SINCOS:
                if( D3DPS_VERSION(2,0) == Version )
                {
                    PSTRRegister    Src0Squared, Src0X, Src1WZ, Src2WZ, SinXBy2, Src2Z;
                    Src0Squared._Set(PSTRREG_SCRATCH,0);
                    Src0X._Set(PSTRREG_SCRATCH,1);
                    Src2Z = SinXBy2 = Src1WZ = Src2WZ = Src0X; // reusing same register

                    // Note that SrcReg[0] has already been replicated because SINCOS required
                    // a replicate swizzle to select a single component.

                    // Src0Squared.xy <- src0.xy*src0.xy (src0.x == src0.y since src[0] was replicated)
                    _NewPSInst(PSTRINST_MUL);
                    _InstParam(PSTRINST_MUL).DstReg             = Src0Squared;
                    _InstParam(PSTRINST_MUL).SrcReg0            = SrcReg[0];
                    _InstParam(PSTRINST_MUL).SrcReg1            = SrcReg[0];
                    _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_0|PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->PredicateInfo; // only replicate swizzle on predicate expected

                    // mad: dstreg.xy <- Src0Squared*SrcReg1.xy + Src1WZYX (ps_2_0 has wzyx)

                    _NewPSInst(PSTRINST_SWIZZLE);
                    _InstParam(PSTRINST_SWIZZLE).DstReg         = Src1WZ;
                    _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[1];
                    _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_SWIZZLE).Swizzle        = _Swizzle(PSTR_SELECT_A,PSTR_SELECT_B,
                                                                  PSTR_SELECT_G,PSTR_SELECT_R); // don't care for last 2
                    _InstParam(PSTRINST_SWIZZLE).Predication = m_pWorkerData->PredicateInfo; // only replicate swizzle on predicate expected

                    _NewPSInst(PSTRINST_MAD);
                    _InstParam(PSTRINST_MAD).DstReg             = DstReg;
                    _InstParam(PSTRINST_MAD).SrcReg0            = Src0Squared;
                    _InstParam(PSTRINST_MAD).SrcReg1            = SrcReg[1];
                    _InstParam(PSTRINST_MAD).SrcReg2            = Src1WZ;
                    _InstParam(PSTRINST_MAD).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_MAD).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_MAD).bSrcReg2_Negate    = FALSE;
                    _InstParam(PSTRINST_MAD).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_MAD).Predication        = m_pWorkerData->PredicateInfo;  // only replicate swizzle on predicate expected

                    // mad: dstreg.xy <- dstreg*Src0Squared + Src2
                    _NewPSInst(PSTRINST_MAD);
                    _InstParam(PSTRINST_MAD).DstReg             = DstReg;
                    _InstParam(PSTRINST_MAD).SrcReg0            = DstReg;
                    _InstParam(PSTRINST_MAD).SrcReg1            = Src0Squared;
                    _InstParam(PSTRINST_MAD).SrcReg2            = SrcReg[2];
                    _InstParam(PSTRINST_MAD).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_MAD).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_MAD).bSrcReg2_Negate    = FALSE;
                    _InstParam(PSTRINST_MAD).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_MAD).Predication        = m_pWorkerData->PredicateInfo;  // only replicate swizzle on predicate expected

                    // mad: dstreg.xy <- dstreg.xy*Src0Squared + Src2WZ [dstreg.x == partial sin(theta/2), dstreg.y == cos(theta/2)]
                    _NewPSInst(PSTRINST_SWIZZLE);
                    _InstParam(PSTRINST_SWIZZLE).DstReg         = Src2WZ;
                    _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[2];
                    _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_SWIZZLE).Swizzle        = _Swizzle(PSTR_SELECT_A,PSTR_SELECT_B,
                                                                  PSTR_SELECT_G,PSTR_SELECT_R); // WZYX is in ps_2_0
                    _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->PredicateInfo;  // only replicate swizzle on predicate expected

                    _NewPSInst(PSTRINST_MAD);
                    _InstParam(PSTRINST_MAD).DstReg             = DstReg;
                    _InstParam(PSTRINST_MAD).SrcReg0            = DstReg;
                    _InstParam(PSTRINST_MAD).SrcReg1            = Src0Squared;
                    _InstParam(PSTRINST_MAD).SrcReg2            = Src2WZ;
                    _InstParam(PSTRINST_MAD).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_MAD).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_MAD).bSrcReg2_Negate    = FALSE;
                    _InstParam(PSTRINST_MAD).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_MAD).Predication        = m_pWorkerData->PredicateInfo;  // only replicate swizzle on predicate expected

                    // mul: DstReg.x <- dstreg.x*Src0X [dstreg.x == sin(theta/2)]
                    _NewPSInst(PSTRINST_MUL);
                    _InstParam(PSTRINST_MUL).DstReg             = DstReg;
                    _InstParam(PSTRINST_MUL).SrcReg0            = DstReg;
                    _InstParam(PSTRINST_MUL).SrcReg1            = SrcReg[0];
                    _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = bSrcNegate[0];
                    _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_0;
                    _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->PredicateInfo;  // only replicate swizzle on predicate expected

                    // mul: dstreg.xy <- dstreg.xy*SinXby2
                    _NewPSInst(PSTRINST_SWIZZLE);
                    _InstParam(PSTRINST_SWIZZLE).DstReg         = SinXBy2;
                    _InstParam(PSTRINST_SWIZZLE).SrcReg0        = DstReg;
                    _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_SWIZZLE).Swizzle        = PSTR_REPLICATERED;
                    _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->PredicateInfo; // only replicate swizzle on predicate expected

                    _NewPSInst(PSTRINST_MUL);
                    _InstParam(PSTRINST_MUL).DstReg             = DstReg;
                    _InstParam(PSTRINST_MUL).SrcReg0            = DstReg;
                    _InstParam(PSTRINST_MUL).SrcReg1            = SinXBy2;
                    _InstParam(PSTRINST_MUL).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_MUL).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_MUL).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_MUL).Predication        = m_pWorkerData->PredicateInfo; // only replicate swizzle on predicate expected

                    // add: dstreg.xy = dstreg + dstreg
                    _NewPSInst(PSTRINST_ADD);
                    _InstParam(PSTRINST_ADD).DstReg             = DstReg;
                    _InstParam(PSTRINST_ADD).SrcReg0            = DstReg;
                    _InstParam(PSTRINST_ADD).SrcReg1            = DstReg;
                    _InstParam(PSTRINST_ADD).bSrcReg0_Negate    = FALSE;
                    _InstParam(PSTRINST_ADD).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_ADD).WriteMask          = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                    _InstParam(PSTRINST_ADD).Predication        = m_pWorkerData->PredicateInfo; // only replicate swizzle on predicate expected

                    // add: dstreg.x = -dstreg.x + src2.z
                    _NewPSInst(PSTRINST_SWIZZLE);
                    _InstParam(PSTRINST_SWIZZLE).DstReg         = Src2Z;
                    _InstParam(PSTRINST_SWIZZLE).SrcReg0        = SrcReg[2];
                    _InstParam(PSTRINST_SWIZZLE).WriteMask      = PSTR_COMPONENTMASK_0;
                    _InstParam(PSTRINST_SWIZZLE).Swizzle        = PSTR_REPLICATEBLUE;
                    _InstParam(PSTRINST_SWIZZLE).Predication    = m_pWorkerData->PredicateInfo; // only replicate swizzle on predicate expected

                    _NewPSInst(PSTRINST_ADD);
                    _InstParam(PSTRINST_ADD).DstReg             = DstReg;
                    _InstParam(PSTRINST_ADD).SrcReg0            = DstReg;
                    _InstParam(PSTRINST_ADD).SrcReg1            = Src2Z;
                    _InstParam(PSTRINST_ADD).bSrcReg0_Negate    = TRUE;
                    _InstParam(PSTRINST_ADD).bSrcReg1_Negate    = FALSE;
                    _InstParam(PSTRINST_ADD).WriteMask          = PSTR_COMPONENTMASK_0;
                    _InstParam(PSTRINST_ADD).Predication        = m_pWorkerData->PredicateInfo; // only replicate swizzle on predicate expected

                } else { // Don't bother doing the taylor expansion for ps > 2_0
                    if( DstWriteMask & PSTR_COMPONENTMASK_0 )
                    {
                        _NewPSInst(PSTRINST_COS);
                        _InstParam(PSTRINST_COS).DstReg             = DstReg;
                        _InstParam(PSTRINST_COS).SrcReg0            = SrcReg[0];
                        _InstParam(PSTRINST_COS).bSrcReg0_Negate    = bSrcNegate[0];
                        _InstParam(PSTRINST_COS).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_0;
                        _InstParam(PSTRINST_COS).SrcReg0_Selector   = _SelectorFromSwizzle(SrcSwizzle[0]);
                        _InstParam(PSTRINST_COS).Predication        = m_pWorkerData->PredicateInfo;
                    }

                    if( DstWriteMask & PSTR_COMPONENTMASK_1 )
                    {
                        _NewPSInst(PSTRINST_SIN);
                        _InstParam(PSTRINST_SIN).DstReg             = DstReg;
                        _InstParam(PSTRINST_SIN).SrcReg0            = SrcReg[0];
                        _InstParam(PSTRINST_SIN).bSrcReg0_Negate    = bSrcNegate[0];
                        _InstParam(PSTRINST_SIN).WriteMask          = DstWriteMask & PSTR_COMPONENTMASK_1;
                        _InstParam(PSTRINST_SIN).SrcReg0_Selector   = _SelectorFromSwizzle(SrcSwizzle[0]);
                        _InstParam(PSTRINST_SIN).Predication        = m_pWorkerData->PredicateInfo;
                    }
                }
                _EmitDstMod(DstReg,DstWriteMask);
                break;
            default:
                break;
            }

            if( m_pWorkerData->pInst->bFlushQueue )
            {
                Assert(bQueuedWrite);       // CPSTrans::Initialize - Internal error: can't flush if there's nothing queued.
                UNREFERENCED_PARAMETER(bQueuedWrite);
                _EnterQuadPixelLoop();
                _NewPSInst(PSTRINST_MOV);
                _InstParam(PSTRINST_MOV).DstReg             = QueuedWriteDstReg;
                _InstParam(PSTRINST_MOV).SrcReg0._Set(PSTRREG_QUEUEDWRITE,0);
                _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
                _InstParam(PSTRINST_MOV).WriteMask          = QueuedWriteDstWriteMask;
                _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;
                bQueuedWrite = FALSE;
            }

            if( bInsertD3DPSInstMarkers )
                _LeaveQuadPixelLoop();
        }

        // Default predication (force true) for last few ops...
        m_pWorkerData->ForceNoPredication.bInvertPredicate = FALSE;
        m_pWorkerData->ForceNoPredication.PredicateSwizzle = PSTR_NOSWIZZLE;
        m_pWorkerData->ForceNoPredication.PredicateReg._Set(PSTRREG_PREDICATETRUE,0);

        if( D3DPS_VERSION(2,0) > Version )
        {
            _EnterQuadPixelLoop();
            // For pre-2_0 pixelshaders, output is in r0.  Move it to oC0.
            _NewPSInst(PSTRINST_MOV);
            _InstParam(PSTRINST_MOV).DstReg._Set(PSTRREG_COLOROUT,0);
            _InstParam(PSTRINST_MOV).SrcReg0._Set(PSTRREG_TEMP,0);
            _InstParam(PSTRINST_MOV).bSrcReg0_Negate    = FALSE;
            _InstParam(PSTRINST_MOV).WriteMask          = PSTR_COMPONENTMASK_ALL;
            _InstParam(PSTRINST_MOV).Predication        = m_pWorkerData->ForceNoPredication;
            m_ColorOutPresentMask |= 1<<0;
        }

        if( bDepthOutput )
        {
            _EnterQuadPixelLoop();
            _NewPSInst(PSTRINST_DEPTH);
            _InstParam(PSTRINST_DEPTH).SrcReg0._Set(PSTRREG_DEPTHOUT,0);
        }

        _LeaveQuadPixelLoop();

        _NewPSInst(PSTRINST_END);
        if( !bInsertD3DPSInstMarkers )
        {
            m_EndOffset = _GetOffset();
        }

        m_pWorkerData = NULL;
    }
    #if 0
    catch(HRESULT hr)
    {
        EXIT_WITH_STATUS(hr);
    }
    #endif

    if( !bKeepDebugInfo )
    {
        if(m_pD3DPixelShaderInstructionArray)
        {
            WarpPlatform::FreeMemory(m_pD3DPixelShaderInstructionArray);
            m_pD3DPixelShaderInstructionArray = NULL;
        }
        if(m_pCode)
        {
            WarpPlatform::FreeMemory(m_pCode);
            m_pCode = NULL;
        }
    }
    if( (m_IfNestTracker.GetStackDepth() > 0) ||
        (m_LoopNestTracker.GetStackDepth() > 0) ||
        (m_LabelTracker.LabelsAreStillNeeded()) )
    {
        NO_DEFAULT;     // CPSTrans::Initialize - Pixel shader contains broken flow control structure."
    }
    m_Status = S_OK;
    return;
ERROR_EXIT:
    CPSTrans::~CPSTrans();
    return;
}

//-----------------------------------------------------------------------------
//
// CPSTrans::CPSTrans()
//
//-----------------------------------------------------------------------------
CPSTrans::CPSTrans()
{
    m_Status                            = E_FAIL;
    m_cD3DInst                          = 0;
    m_cPSTRInst                         = 0;
    m_EndOffset                         = 0;
    m_pD3DPixelShaderInstructionArray   = NULL;
    m_pCode                             = NULL;
    m_cConstDefsF                       = 0;
    m_pConstDefsF                       = NULL;
    m_cConstDefsI                       = 0;
    m_pConstDefsI                       = NULL;
    m_cConstDefsB                       = 0;
    m_pConstDefsB                       = NULL;
    memset(m_SamplerRegDcl,0,sizeof(m_SamplerRegDcl));
    m_cActiveTextureStages              = 0;
    m_pWorkerData                       = NULL;
    m_ColorOutPresentMask               = 0;
    m_bHasTexKillInstructions           = FALSE;
}

//-----------------------------------------------------------------------------
//
// CPSTrans::~CPSTrans()
//
//-----------------------------------------------------------------------------
CPSTrans::~CPSTrans()
{
    if(m_pD3DPixelShaderInstructionArray)
    {
        WarpPlatform::FreeMemory(m_pD3DPixelShaderInstructionArray);
        m_pD3DPixelShaderInstructionArray = NULL;
    }
    if(m_pCode)
    {
        WarpPlatform::FreeMemory(m_pCode);
        m_pCode = NULL;
    }
    if(m_pConstDefsF)
    {
        WarpPlatform::FreeMemory(m_pConstDefsF);
        m_pConstDefsF = NULL;
    }
    if(m_pConstDefsI)
    {
        WarpPlatform::FreeMemory(m_pConstDefsI);
        m_pConstDefsI = NULL;
    }
    if(m_pConstDefsB)
    {
        WarpPlatform::FreeMemory(m_pConstDefsB);
        m_pConstDefsB = NULL;
    }
}

//-----------------------------------------------------------------------------
//
// CLabelTrack::~CLabelTrack()
//
//-----------------------------------------------------------------------------
CLabelTrack::~CLabelTrack()
{
    while(m_pNeededLabelList)
    {
        NEEDED_LABEL_NODE* pDeleteMe = m_pNeededLabelList;
        m_pNeededLabelList = m_pNeededLabelList->pNext;
        WarpPlatform::FreeMemory(pDeleteMe);
    }
    while(m_pDefinedLabelList)
    {
        DEFINED_LABEL_NODE* pDeleteMe = m_pDefinedLabelList;
        m_pDefinedLabelList = m_pDefinedLabelList->pNext;
        WarpPlatform::FreeMemory(pDeleteMe);
    }
}

//-----------------------------------------------------------------------------
//
// CLabelTrack::FindDefinedLabel()
//
//-----------------------------------------------------------------------------
CLabelTrack::DEFINED_LABEL_NODE* CLabelTrack::FindDefinedLabel(UINT LabelID)
{
    DEFINED_LABEL_NODE* pCurrDefinedLabel = m_pDefinedLabelList;
    while(pCurrDefinedLabel)
    {
        if( LabelID == pCurrDefinedLabel->LabelID )
        {
            return pCurrDefinedLabel;
        }
        pCurrDefinedLabel = pCurrDefinedLabel->pNext;
    }
    return NULL;
}

//-----------------------------------------------------------------------------
//
// CLabelTrack::ResolveNeededLabel()
//
//-----------------------------------------------------------------------------
void CLabelTrack::ResolveNeededLabel(size_t OffsetToOutputLabelOffsetWhenDefined,
                                     size_t OffsetToOutputLabelPSTRInstIDWhenDefined,
                                     const DEFINED_LABEL_NODE* pDefinedLabel,
                                     BYTE*  pPSTRInstBuffer)
{
    *(size_t*)(pPSTRInstBuffer + OffsetToOutputLabelOffsetWhenDefined) = pDefinedLabel->LabelOffset;
    *(PSTR_INST_ID*)(pPSTRInstBuffer + OffsetToOutputLabelPSTRInstIDWhenDefined) = pDefinedLabel->LabelPSTRInstID;
}

//-----------------------------------------------------------------------------
//
// CLabelTrack::ResolveAndDeleteNeededLabel()
//
//-----------------------------------------------------------------------------
void CLabelTrack::ResolveAndDeleteNeededLabel(NEEDED_LABEL_NODE* pNeededLabel,
                                              const DEFINED_LABEL_NODE* pDefinedLabel,
                                              BYTE*  pPSTRInstBuffer)
{
    // Store info about label where it was required.
    ResolveNeededLabel( pNeededLabel->OffsetToOutputLabelOffsetWhenDefined,
                        pNeededLabel->OffsetToOutputLabelPSTRInstIDWhenDefined,
                        pDefinedLabel, pPSTRInstBuffer);

    // Remove this label from the needed label list
    if( pNeededLabel->pPrev )
        pNeededLabel->pPrev->pNext = pNeededLabel->pNext;
    if( pNeededLabel->pNext )
        pNeededLabel->pNext->pPrev = pNeededLabel->pPrev;
    if( m_pNeededLabelList == pNeededLabel )
        m_pNeededLabelList = pNeededLabel->pNext;

    WarpPlatform::FreeMemory(pNeededLabel);
}

//-----------------------------------------------------------------------------
//
// CLabelTrack::AddLabel()
//
//-----------------------------------------------------------------------------
HRESULT CLabelTrack::AddLabel(UINT LabelID, size_t LabelOffset, PSTR_INST_ID LabelPSTRInstID,
                              BYTE*  pPSTRInstBuffer)
{
    if( FindDefinedLabel(LabelID) )
    {
        NO_DEFAULT;     // Label already defined.
    }
    // Allocate new defined label
    DEFINED_LABEL_NODE* pNewLabel = (DEFINED_LABEL_NODE*)WarpPlatform::AllocateMemory(sizeof(DEFINED_LABEL_NODE));
    if( !pNewLabel )
        return E_OUTOFMEMORY;

    // Add to defined label list
    pNewLabel->LabelID = LabelID;
    pNewLabel->LabelOffset = LabelOffset;
    pNewLabel->LabelPSTRInstID = LabelPSTRInstID;
    pNewLabel->pNext = m_pDefinedLabelList;
    pNewLabel->pPrev = NULL;
    if( m_pDefinedLabelList )
    {
        m_pDefinedLabelList->pPrev = pNewLabel;
    }
    m_pDefinedLabelList = pNewLabel;

    // See if there are any entries in the needed label list for this label
    NEEDED_LABEL_NODE* pCurrNeededLabel = m_pNeededLabelList;
    while(pCurrNeededLabel)
    {
        if( LabelID == pCurrNeededLabel->LabelID )
        {
            NEEDED_LABEL_NODE* pDeleteMe = pCurrNeededLabel;
            pCurrNeededLabel = pCurrNeededLabel->pNext;
            ResolveAndDeleteNeededLabel(pDeleteMe,pNewLabel,pPSTRInstBuffer);
        }
        else
        {
            pCurrNeededLabel = pCurrNeededLabel->pNext;
        }
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// CLabelTrack::NeedLabel()
//
//-----------------------------------------------------------------------------
HRESULT CLabelTrack::NeedLabel(UINT LabelID, size_t OffsetToOutputLabelOffsetWhenDefined,
                               size_t OffsetToOutputLabelPSTRInstIDWhenDefined,
                               BYTE*  pPSTRInstBuffer)
{
    const DEFINED_LABEL_NODE* pDefinedLabel = FindDefinedLabel(LabelID);
    if( pDefinedLabel )
    {
        // Label already defined -> write out info.
        ResolveNeededLabel(OffsetToOutputLabelOffsetWhenDefined,OffsetToOutputLabelPSTRInstIDWhenDefined,pDefinedLabel,pPSTRInstBuffer);
        return S_OK;
    }
    // Allocate new needed label
    NEEDED_LABEL_NODE* pNewLabel = (NEEDED_LABEL_NODE*)WarpPlatform::AllocateMemory(sizeof(NEEDED_LABEL_NODE));
    if( !pNewLabel )
        return E_OUTOFMEMORY;

    // Add to needed label list
    pNewLabel->LabelID = LabelID;
    pNewLabel->OffsetToOutputLabelOffsetWhenDefined = OffsetToOutputLabelOffsetWhenDefined;
    pNewLabel->OffsetToOutputLabelPSTRInstIDWhenDefined = OffsetToOutputLabelPSTRInstIDWhenDefined;
    pNewLabel->pNext = m_pNeededLabelList;
    pNewLabel->pPrev = NULL;
    if( m_pNeededLabelList )
    {
        m_pNeededLabelList->pPrev = pNewLabel;
    }
    m_pNeededLabelList = pNewLabel;

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// CLoopNestTrack::~CLoopNestTrack()
//
//-----------------------------------------------------------------------------
CLoopNestTrack::~CLoopNestTrack()
{
    while(m_pStartedLoopStack)
    {
        LOOPSTART_INFO_NODE* pDeleteMe = m_pStartedLoopStack;
        m_pStartedLoopStack = m_pStartedLoopStack->pNext;
        while(pDeleteMe->pBreakList)
        {
            BREAK_INFO_NODE* pBreakDeleteMe = pDeleteMe->pBreakList;
            pDeleteMe->pBreakList = pDeleteMe->pBreakList->pNext;
            WarpPlatform::FreeMemory(pBreakDeleteMe);
        }
        WarpPlatform::FreeMemory(pDeleteMe);
    }
}

//-----------------------------------------------------------------------------
//
// CLoopNestTrack::LoopStart()
//
//-----------------------------------------------------------------------------
HRESULT CLoopNestTrack::LoopStart(   BOOL    bIsLoop, // true means loop, false means rep
                                     size_t  OffsetToOutputLoopEndOffsetWhenDefined,
                                     size_t  OffsetToOutputLoopEndPSTRInstIDWhenDefined,
                                     size_t  LoopStartOffset,
                                     PSTR_INST_ID LoopStartPSTRInstID )
{
    // Allocate new loop start
    LOOPSTART_INFO_NODE* pNewLoopStart = (LOOPSTART_INFO_NODE*)WarpPlatform::AllocateMemory(sizeof(LOOPSTART_INFO_NODE));
    if( !pNewLoopStart )
        return E_OUTOFMEMORY;

    // Push onto started loop stack
    m_StackDepth++;
    if( bIsLoop )
        m_NumNestedLoopsExcludingReps++;
    pNewLoopStart->bIsLoop = bIsLoop;
    pNewLoopStart->OffsetToOutputLoopEndOffsetWhenDefined = OffsetToOutputLoopEndOffsetWhenDefined;
    pNewLoopStart->OffsetToOutputLoopEndPSTRInstIDWhenDefined = OffsetToOutputLoopEndPSTRInstIDWhenDefined;
    pNewLoopStart->LoopStartOffset = LoopStartOffset;
    pNewLoopStart->LoopStartPSTRInstID = LoopStartPSTRInstID;
    pNewLoopStart->pBreakList = NULL;
    pNewLoopStart->pNext = m_pStartedLoopStack;
    m_pStartedLoopStack = pNewLoopStart;

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// CLoopNestTrack::Break()
//
//-----------------------------------------------------------------------------
HRESULT CLoopNestTrack::Break(  size_t  OffsetToOutputLoopEndOffsetWhenDefined,
                                size_t  OffsetToOutputLoopEndPSTRInstIDWhenDefined )
{
    if( !m_pStartedLoopStack )
    {
        NO_DEFAULT;     // Break encountered when not in loop.
    }
    // Allocate new loop start
    BREAK_INFO_NODE* pNewBreak = (BREAK_INFO_NODE*)WarpPlatform::AllocateMemory(sizeof(BREAK_INFO_NODE));
    if( !pNewBreak )
        return E_OUTOFMEMORY;

    // Add break to break list
    pNewBreak->OffsetToOutputLoopEndOffsetWhenDefined = OffsetToOutputLoopEndOffsetWhenDefined;
    pNewBreak->OffsetToOutputLoopEndPSTRInstIDWhenDefined = OffsetToOutputLoopEndPSTRInstIDWhenDefined;
    pNewBreak->pNext = m_pStartedLoopStack->pBreakList;
    m_pStartedLoopStack->pBreakList = pNewBreak;

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// CLoopNestTrack::LoopEnd()
//
//-----------------------------------------------------------------------------
HRESULT CLoopNestTrack::LoopEnd( BOOL    bIsLoop, // true means loop, false means rep
                                 size_t  OffsetToOutputLoopStartOffset,
                                 size_t  OffsetToOutputLoopStartPSTRInstID,
                                 size_t  LoopEndOffset,
                                 PSTR_INST_ID LoopEndPSTRInstID,
                                 BYTE*  pPSTRInstBuffer )
{
    if( !m_pStartedLoopStack )
    {
        NO_DEFAULT;     // End of a loop encountered when none was started.
    }
    if( m_pStartedLoopStack->bIsLoop != bIsLoop )
    {
        NO_DEFAULT;     // Loop end type doesn't match loop start type (loop/endloop or rep/endrep are expected).
    }
    // Pop off started loop stack
    m_StackDepth--;
    *(size_t*)(pPSTRInstBuffer + m_pStartedLoopStack->OffsetToOutputLoopEndOffsetWhenDefined) = LoopEndOffset;
    *(PSTR_INST_ID*)(pPSTRInstBuffer + m_pStartedLoopStack->OffsetToOutputLoopEndPSTRInstIDWhenDefined) = LoopEndPSTRInstID;
    *(size_t*)(pPSTRInstBuffer + OffsetToOutputLoopStartOffset) = m_pStartedLoopStack->LoopStartOffset;
    *(PSTR_INST_ID*)(pPSTRInstBuffer + OffsetToOutputLoopStartPSTRInstID) = m_pStartedLoopStack->LoopStartPSTRInstID;

    while(m_pStartedLoopStack->pBreakList)
    {
        BREAK_INFO_NODE* pBreakDeleteMe = m_pStartedLoopStack->pBreakList;
        m_pStartedLoopStack->pBreakList = m_pStartedLoopStack->pBreakList->pNext;
        *(size_t*)(pPSTRInstBuffer + pBreakDeleteMe->OffsetToOutputLoopEndOffsetWhenDefined) = LoopEndOffset;
        *(PSTR_INST_ID*)(pPSTRInstBuffer + pBreakDeleteMe->OffsetToOutputLoopEndPSTRInstIDWhenDefined) = LoopEndPSTRInstID;
        delete pBreakDeleteMe;
    }

    LOOPSTART_INFO_NODE* pDeleteMe = m_pStartedLoopStack;
    m_pStartedLoopStack = m_pStartedLoopStack->pNext;
    WarpPlatform::FreeMemory(pDeleteMe);
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// CIfNestTrack::~CIfNestTrack()
//
//-----------------------------------------------------------------------------
CIfNestTrack::~CIfNestTrack()
{
    while(m_pStartedIfStack)
    {
        IF_INFO_NODE* pDeleteMe = m_pStartedIfStack;
        m_pStartedIfStack = m_pStartedIfStack->pNext;
        WarpPlatform::FreeMemory(pDeleteMe);
    }
}

//-----------------------------------------------------------------------------
//
// CIfNestTrack::If()
//
//-----------------------------------------------------------------------------
HRESULT CIfNestTrack::If(size_t OffsetToOutputElseOrEndifOffsetWhenDefined,
                         size_t OffsetToOutputElseOrEndifPSTRInstIDWhenDefined)
{
    // Allocate new if
    IF_INFO_NODE* pNewIf = (IF_INFO_NODE*)WarpPlatform::AllocateMemory(sizeof(IF_INFO_NODE));
    if( !pNewIf )
        return E_OUTOFMEMORY;

    // Push onto started if stack
    m_StackDepth++;
    pNewIf->bSeenElse = FALSE;
    pNewIf->OffsetToOutputElseOrEndifOffsetWhenDefined = OffsetToOutputElseOrEndifOffsetWhenDefined;
    pNewIf->OffsetToOutputElseOrEndifPSTRInstIDWhenDefined = OffsetToOutputElseOrEndifPSTRInstIDWhenDefined;
    pNewIf->pNext = m_pStartedIfStack;
    m_pStartedIfStack = pNewIf;

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// CIfNestTrack::Else()
//
//-----------------------------------------------------------------------------
HRESULT CIfNestTrack::Else(size_t OffsetToOutputEndifOffsetWhenDefined,
                           size_t OffsetToOutputEndifPSTRInstIDWhenDefined,
                           size_t ElseOffset,
                           PSTR_INST_ID ElsePSTRInstID,
                           BYTE*  pPSTRInstBuffer )
{
    Assert(m_pStartedIfStack && !m_pStartedIfStack->bSeenElse);

    // Store out info required by the 'if' statement, and record info about the 'else' statement.
    m_pStartedIfStack->bSeenElse = TRUE;
    *(size_t*)(pPSTRInstBuffer + m_pStartedIfStack->OffsetToOutputElseOrEndifOffsetWhenDefined) = ElseOffset;
    *(PSTR_INST_ID*)(pPSTRInstBuffer + m_pStartedIfStack->OffsetToOutputElseOrEndifPSTRInstIDWhenDefined) = ElsePSTRInstID;
    m_pStartedIfStack->OffsetToOutputElseOrEndifOffsetWhenDefined = OffsetToOutputEndifOffsetWhenDefined;
    m_pStartedIfStack->OffsetToOutputElseOrEndifPSTRInstIDWhenDefined = OffsetToOutputEndifPSTRInstIDWhenDefined;
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// CIfNestTrack::Endif()
//
//-----------------------------------------------------------------------------
HRESULT CIfNestTrack::Endif(size_t EndifOffset,
                            PSTR_INST_ID EndifPSTRInstID,
                            BYTE*  pPSTRInstBuffer )
{
    Assert(!m_pStartedIfStack);

    // Pop off started if stack
    m_StackDepth--;
    *(size_t*)(pPSTRInstBuffer + m_pStartedIfStack->OffsetToOutputElseOrEndifOffsetWhenDefined) = EndifOffset;
    *(PSTR_INST_ID*)(pPSTRInstBuffer + m_pStartedIfStack->OffsetToOutputElseOrEndifPSTRInstIDWhenDefined) = EndifPSTRInstID;
    IF_INFO_NODE* pDeleteMe = m_pStartedIfStack;
    m_pStartedIfStack = m_pStartedIfStack->pNext;
    WarpPlatform::FreeMemory(pDeleteMe);
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// CInputRegDclInfo::~CInputRegDclInfo()
//
//-----------------------------------------------------------------------------
CInputRegDclInfo::~CInputRegDclInfo()
{
    INPUT_DCL_NODE* pCurrInputDcl = m_pInputDclList;
    while(pCurrInputDcl)
    {
        INPUT_DCL_NODE* pDeleteMe = pCurrInputDcl;
        pCurrInputDcl = pCurrInputDcl->pNext;
        WarpPlatform::FreeMemory(pDeleteMe);
    }
    m_pInputDclList = NULL;
}

//-----------------------------------------------------------------------------
//
// CInputRegDclInfo::AddNewDcl()
//
//-----------------------------------------------------------------------------
HRESULT CInputRegDclInfo::AddNewDcl(D3DDECLUSAGE Usage, UINT Index, 
                                    PSTR_REGISTER_TYPE PSTRRegType, UINT RegNum, 
                                    BYTE WriteMask, BOOL bSampleAtCentroidWhenMultisampling)
{
    INPUT_DCL_NODE* pNewInputDcl = (INPUT_DCL_NODE*)WarpPlatform::AllocateMemory(sizeof(INPUT_DCL_NODE));
    if(!pNewInputDcl)
        return E_OUTOFMEMORY;

    pNewInputDcl->Usage = Usage;
    pNewInputDcl->Index = Index;
    pNewInputDcl->PSTRRegType = PSTRRegType;
    pNewInputDcl->RegNum = RegNum;
    pNewInputDcl->WriteMask = WriteMask;
    pNewInputDcl->bSampleAtCentroidWhenMultisampling = bSampleAtCentroidWhenMultisampling;
    pNewInputDcl->pNext = m_pInputDclList;
    m_pInputDclList = pNewInputDcl;
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// CInputRegDclInfo::IsRegDeclared()
//
//-----------------------------------------------------------------------------
BOOL CInputRegDclInfo::IsRegDeclared(PSTR_REGISTER_TYPE PSTRRegType, UINT RegNum, BYTE WriteMask)
{
    INPUT_DCL_NODE* pCurrInputDcl = m_pInputDclList;
    DWORD DeclaredMask = 0;
    while(pCurrInputDcl)
    {
        if( (PSTRRegType == pCurrInputDcl->PSTRRegType) && 
            (RegNum == pCurrInputDcl->RegNum) )
        {
            DeclaredMask |= pCurrInputDcl->WriteMask;
        }
        pCurrInputDcl = pCurrInputDcl->pNext;
    }
    return ((DeclaredMask & WriteMask)==WriteMask);
}

//-----------------------------------------------------------------------------
//
// CalculateSourceReadMasks
//
// Given a D3D pixel shader instruction, figure out which components of
// each source parameter get read by the instruction.
//
// There are two calculations that can be done:
//
// bAfterSwizzle = FALSE : Before any source swizzle has been applied,
//                         which components get read.
//
// bAfterSwizzle = TRUE  : After any source swizzle has been applied,
//                         which components get read.
//
//-----------------------------------------------------------------------------
void CalculateSourceReadMasks(const D3DPixelShaderInstruction* pInst,
                              BYTE* pSourceReadMasks,
                              BOOL bAfterSwizzle,
                              const D3DSAMPLER_TEXTURE_TYPE* pSamplerDcl, // ps_2_0+ only
                              DWORD dwVersion)
{
    UINT i, j;
    DWORD Opcode = pInst->Opcode & D3DSI_OPCODE_MASK;
    BYTE  ComponentMask[4]= {PSTR_COMPONENTMASK_0, PSTR_COMPONENTMASK_1, PSTR_COMPONENTMASK_2, PSTR_COMPONENTMASK_3};

    for( i = 0; i < pInst->SrcParamCount; i++ )
    {
        BYTE  NeededComponents = 0;
        BYTE  ReadComponents = 0;

        switch( Opcode )
        {
        case D3DSIO_TEX:
        case D3DSIO_TEXLDD: // ps version 3.0+ only, so won't hit other paths
        case D3DSIO_TEXLDL: // ps version 3.0+ only, so won't hit other paths
            if( D3DPS_VERSION(2,0) <= dwVersion)
            {
                if( 0 == i )
                {
                    Assert( (pInst->SrcParamCount >= 2));       // CalculateSourceReadMasks - Invalid texld instruction.
                    Assert( D3DSPR_SAMPLER == D3DSI_GETREGTYPE(pInst->SrcParam[1]));        // CalculateSourceReadMasks - Second source parameter for texld must be s# (sampler).
                    UINT SamplerNum = D3DSI_GETREGNUM(pInst->SrcParam[1]);
                    Assert( SamplerNum < PSTR_MAX_TEXTURE_SAMPLERS);        // CalculateSourceReadMasks - Invalid sampler number.
                    Assert(pSamplerDcl);        // CalculateSourceReadMasks - pSamplerDcl == NULL
                    switch( pSamplerDcl[SamplerNum] )
                    {
                    case D3DSTT_2D:
                        NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                        break;
                    case D3DSTT_CUBE:
                    case D3DSTT_VOLUME:
                        NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        break;
                    default:
                        NO_DEFAULT;         // CalculateSourceReadMasks - Unrecognized s# texture sampler setup.
                    }
                    if( (pInst->Opcode & D3DSI_TEXLD_PROJECT) || (pInst->Opcode & D3DSI_TEXLD_BIAS) ||
                        (D3DSIO_TEXLDL == Opcode) )
                    {
                        NeededComponents |= PSTR_COMPONENTMASK_3;
                    }
                }
                else if( 1 == i )
                {
                    // pretend for sampler parameter, the needed component mask means which lookup result components are needed
                    NeededComponents = (BYTE)((pInst->DstParam & D3DSP_WRITEMASK_ALL) >> PSTR_COMPONENTMASK_SHIFT);
                }
                else if( (D3DSIO_TEXLDD == Opcode) && (2 <= i) ) // dsx, dsy parameters to texldd
                {
                    UINT SamplerNum = D3DSI_GETREGNUM(pInst->SrcParam[1]);
                    switch( pSamplerDcl[SamplerNum] )
                    {
                    case D3DSTT_2D:
                        NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                        break;
                    case D3DSTT_CUBE:
                    case D3DSTT_VOLUME:
                        NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
                        break;
                    default:
                        NO_DEFAULT;         // CalculateSourceReadMasks - Unrecognized s# texture sampler setup.
                    }
                }
                else
                {
                    NeededComponents = 0;
                }
            }
            else if( D3DPS_VERSION(1,4) == dwVersion )
            {
                // for ps_1_4, texld has a source parameter
                NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
            }
            else // versions < ps_1_4 don't have a src param on tex, so we shouldn't get here.
            {
                NO_DEFAULT;         // CalculateComponentReadMasks - Invalid tex instruction.
            }
            break;
        case D3DSIO_TEXCOORD:
            if( D3DPS_VERSION(1,4) == dwVersion )
            {
                // for ps_1_4, texcrd has a source parameter
                NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
            }
            else // versions < ps_1_4 don't have a src param on texcoord, so we shouldn't get here.  But maybe in ps_2_0...
            {
                NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2 | PSTR_COMPONENTMASK_3;
            }
            break;
        case D3DSIO_TEXBEM:
        case D3DSIO_TEXBEML:
            NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
            break;
        case D3DSIO_DP2ADD:
            if( 2 == i )
            {
                NeededComponents = PSTR_COMPONENTMASK_0; // there will be a replicate swizzle anyway
            }
            else
            {
                NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
            }
            break;
        case D3DSIO_DP3:
        case D3DSIO_CRS:
            NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
            break;
        case D3DSIO_DP4:
            NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2 | PSTR_COMPONENTMASK_3;
            break;
        case D3DSIO_BEM: // ps_1_4
            NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
            break;
        case D3DSIO_REP:
            NeededComponents = PSTR_COMPONENTMASK_0; // loop counter
            break;
        case D3DSIO_LOOP:
            if( 1 == i ) // i# (loop integer register)
                NeededComponents = PSTR_COMPONENTMASK_0|PSTR_COMPONENTMASK_1|PSTR_COMPONENTMASK_2; //loop count/init value/step value
            else // aL
                NeededComponents = PSTR_COMPONENTMASK_ALL; // aL register -> really a scalar.
            break;
        case D3DSIO_IF:
        case D3DSIO_CALLNZ:
        case D3DSIO_IFC:
        case D3DSIO_BREAKC:
        case D3DSIO_BREAKP:
            NeededComponents = PSTR_COMPONENTMASK_0; // pstrans is just using what's in x for dynamic/static conditionals
                                                     // (runtime enforces replicate swizzle anyway if necessary -> so reading x is fine)
            break;
        case D3DSIO_M3x2:
            NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
            break;
        case D3DSIO_M3x3:
            NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
            break;
        case D3DSIO_M3x4:
            NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2;
            break;
        case D3DSIO_M4x3:
            NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2 | PSTR_COMPONENTMASK_3;
            break;
        case D3DSIO_M4x4:
            NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1 | PSTR_COMPONENTMASK_2 | PSTR_COMPONENTMASK_3;
            break;
        case D3DSIO_SINCOS:
            if( D3DPS_VERSION(3,0) > dwVersion )
            {
                // Macro expansion wants the source replicated swizzle to be written out to r and g for
                // the expansion of the macro.  So we declare we need .rg of the source (post-swizzle)
                NeededComponents = PSTR_COMPONENTMASK_0 | PSTR_COMPONENTMASK_1;
                break;
            }
        case D3DSIO_NRM:
        default:
            // standard component-wise instruction,
            // OR an op we know reads .rgba and we also know it will be validated to .rgba writemask
            NeededComponents = (BYTE)((pInst->DstParam & D3DSP_WRITEMASK_ALL) >> PSTR_COMPONENTMASK_SHIFT);
            break;
        }

        if( bAfterSwizzle )
        {
            pSourceReadMasks[i] = NeededComponents;
        }
        else
        {
            // Figure out which components of this source parameter are read (taking into account swizzle)
            for(j = 0; j < 4; j++)
            {
                if( NeededComponents & ComponentMask[j] )
                    ReadComponents |= ComponentMask[((pInst->SrcParam[i] & D3DSP_SWIZZLE_MASK) >> (D3DVS_SWIZZLE_SHIFT + 2*j)) & 0x3];
            }
            pSourceReadMasks[i] = ReadComponents;
        }
    }
}





