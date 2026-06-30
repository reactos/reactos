// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


///////////////////////////////////////////////////////////////////////////////

//
// pstrans.hpp
//
// Direct3D Pixel Shader Translator:
//
// Translate all pixel shader versions
// to a common instruction set.
//
// The combination of pstrans.hpp and pstrans.cpp are
// designed to be able to compile outside of Refrast.
// In other words, it should be easy to integrate this
// code into other projects. The only small catch is that
// a project taking this code needs to have a pch.cpp.
//
///////////////////////////////////////////////////////////////////////////////
#pragma once

//---------------------------------------------------------------------
// Compilation controls for Pixel Shader Translator
//---------------------------------------------------------------------

// Refrast defines PSTRANS_DEBUG_PREFIX to identify itself.
#ifndef PSTRANS_DEBUG_PREFIX
#define PSTRANS_DEBUG_PREFIX "PSTrans: "
#endif

// Pixel shader instruction disassembly string length is
// defined in D3DRef to a particular value (due to dependencies).
// Otherwise, here is a default.
#ifndef PSTRANS_DISASM_STRING_LENGTH
#define PSTRANS_DISASM_STRING_LENGTH    128
#endif

// Max number of texture stages
#ifndef PSTRANS_MAX_TEXTURE_SAMPLERS
#define PSTRANS_MAX_TEXTURE_SAMPLERS  16
#endif

//---------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------

const DWORD PSTR_MAX_TEXTURE_SAMPLERS       = PSTRANS_MAX_TEXTURE_SAMPLERS;

const DWORD PSTR_MAX_COISSUED_INSTRUCTIONS  = 2;
const DWORD PSTR_NUM_COMPONENTS_IN_REGISTER = 4;
const DWORD PSTR_PIXEL_QUAD                 = 4;

// version-independent consts for sizing arrays
#ifdef WARP_SM30

const DWORD PSTR_MAX_NUMTEMPREG         = D3DPS_TEMPREG_MAX_V3_0; // aka D3DDM_PS_TEMPREG_MAX_9
const DWORD PSTR_MAX_NUMINPUTREG        = D3DPS_INPUTREG_MAX_SW_DX9; // aka D3DDM_PS_INPUTREG_MAX_9
const DWORD PSTR_MAX_NUMCONSTREG        = D3DPS_CONSTREG_MAX_SW_DX9; // aka D3DDM_PS_CONSTREG_MAX_9
const DWORD PSTR_MAX_NUMCONSTINTREG     = D3DPS_CONSTINTREG_MAX_SW_DX9; // aka D3DDM_PS_CONSTINTREG_MAX_9
const DWORD PSTR_MAX_NUMCONSTBOOLREG    = D3DPS_CONSTBOOLREG_MAX_SW_DX9; // aka D3DDM_PS_CONSTBOOLREG_MAX_9
const DWORD PSTR_MAX_NUMTEXTUREREG      = D3DPS_TEXTUREREG_MAX_V2_0; // t# only exist up to ps_2_x; aka D3DDM_PS_TEXTREG_MAX_9
const DWORD PSTR_MAX_NUMCOLOROUTREG     = D3DPS_COLOROUT_MAX_V3_0; // aka D3DDM_PS_COLOROUTREG_MAX_9

#else

#define D3DPS_TEMPREG_MAX_V2_0          12
#define D3DPS_INPUTREG_MAX_V2_0         2
#define D3DPS_CONSTREG_MAX_V2_0         32
#define D3DPS_TEXTUREREG_MAX_V2_0       8
#define D3DPS_COLOROUT_MAX_V2_0         4
#define D3DPS_PREDICATE_MAX_V3_0        1

const DWORD PSTR_MAX_NUMTEMPREG         = D3DPS_TEMPREG_MAX_V2_0; // aka D3DDM_PS_TEMPREG_MAX_9
const DWORD PSTR_MAX_NUMINPUTREG        = D3DPS_INPUTREG_MAX_V2_0; // aka D3DDM_PS_INPUTREG_MAX_9
const DWORD PSTR_MAX_NUMCONSTREG        = D3DPS_CONSTREG_MAX_V2_0; // aka D3DDM_PS_CONSTREG_MAX_9
const DWORD PSTR_MAX_NUMTEXTUREREG      = D3DPS_TEXTUREREG_MAX_V2_0; // t# only exist up to ps_2_x; aka D3DDM_PS_TEXTREG_MAX_9
const DWORD PSTR_MAX_NUMCOLOROUTREG     = D3DPS_COLOROUT_MAX_V2_0; // aka D3DDM_PS_COLOROUTREG_MAX_9

#endif


const DWORD PSTR_MAX_NUMDEPTHOUTREG     = 1; // add D3DDM_PS_DEPTHOUT_MAX_ if more than 1
const DWORD PSTR_MAX_NUMPREDICATEREG    = D3DPS_PREDICATE_MAX_V3_0 + 1; // 1 extra, scratch for simplicity; aka D3DDM_PS_PREDICATEREG_MAX_9
const DWORD PSTR_SCRATCH_PREDICATE_NUM  = PSTR_MAX_NUMPREDICATEREG - 1;
#define PSTR_MAX_RETADDRESS_STACK_DEPTH  5 // 1 extra for address of D3DSIO_END token offset at bottom of stack; aka D3DDM_PS_CALLSTACK_MAX_9

// sizes for internal register arrays
const DWORD PSTR_MAX_REGISTER_STACK_DEPTH = 8; // 4 * (aL+internal loop counter); aka
const DWORD PSTR_MAX_NUMQUEUEDWRITEREG    = PSTR_MAX_COISSUED_INSTRUCTIONS - 1;
#define PSTR_MAX_NUMSRCPARAMS 4
const DWORD PSTR_MAX_NUMPOSTMODSRCREG     = PSTR_MAX_NUMSRCPARAMS;
const DWORD PSTR_MAX_NUMSCRATCHREG        = 5;

//---------------------------------------------------------------------
// Helper names
//---------------------------------------------------------------------

// refdev-specific pixel shader 'instructions' to match legacy pixel processing
#define D3DSIO_TEXBEM_LEGACY    ((D3DSHADER_INSTRUCTION_OPCODE_TYPE)0xC001)
#define D3DSIO_TEXBEML_LEGACY   ((D3DSHADER_INSTRUCTION_OPCODE_TYPE)0xC002)

// Redefine D3D component masks so they fit in a BYTE.
#define PSTR_COMPONENTMASK_SHIFT    16
#define PSTR_COMPONENTMASK_0        (D3DSP_WRITEMASK_0 >> PSTR_COMPONENTMASK_SHIFT)
#define PSTR_COMPONENTMASK_1        (D3DSP_WRITEMASK_1 >> PSTR_COMPONENTMASK_SHIFT)
#define PSTR_COMPONENTMASK_2        (D3DSP_WRITEMASK_2 >> PSTR_COMPONENTMASK_SHIFT)
#define PSTR_COMPONENTMASK_3        (D3DSP_WRITEMASK_3 >> PSTR_COMPONENTMASK_SHIFT)
#define PSTR_COMPONENTMASK_ALL      (D3DSP_WRITEMASK_ALL >> PSTR_COMPONENTMASK_SHIFT)

// Redefine D3D swizzles so they fit in a BYTE
#define PSTR_NOSWIZZLE          (D3DSP_NOSWIZZLE >> D3DSP_SWIZZLE_SHIFT)
#define PSTR_REPLICATERED       (D3DSP_REPLICATERED >> D3DSP_SWIZZLE_SHIFT)
#define PSTR_REPLICATEGREEN     (D3DSP_REPLICATEGREEN >> D3DSP_SWIZZLE_SHIFT)
#define PSTR_REPLICATEBLUE      (D3DSP_REPLICATEBLUE >> D3DSP_SWIZZLE_SHIFT)
#define PSTR_REPLICATEALPHA     (D3DSP_REPLICATEALPHA >> D3DSP_SWIZZLE_SHIFT)
#define PSTR_SELECT_R           0
#define PSTR_SELECT_G           1
#define PSTR_SELECT_B           2
#define PSTR_SELECT_A           3

#define PSTR_LOOPCOUNT_ITERATIONCOUNT_SELECTOR  PSTR_SELECT_R
#define PSTR_LOOPCOUNT_INITVALUE_SELECTOR       PSTR_SELECT_G
#define PSTR_LOOPCOUNT_INCREMENT_SELECTOR       PSTR_SELECT_B

// creates BYTE swizzle description:  bits xxyyzzww made of PSTR_SELECT_* for each component
#define _Swizzle(x,y,z,w)   ((x)|(y<<2)|(z<<4)|(w<<6))

// Returns PSTR_SELECT_R/G/B/A from a BYTE swizzle: ASSUMES the swizzle is a replicate.
#define _SelectorFromSwizzle(swizzle) (0x3&swizzle)

// Returns PSTR_SELECT_R/G/B/A from a BYTE swizzle and a component in the swizzle to get.
#define _SelectorFromSwizzleComponent(swizzle,selector) (0x3&(swizzle>>(selector*2)))

// Returns a BYTE swizzle from PSTR_SELECT_R/G/B/A (i.e. a replicate swizzle)
#define _SwizzleFromSelector(selector) (_Swizzle(selector,selector,selector,selector))

//---------------------------------------------------------------------
// D3DPixelShaderInstruction
//
// Structure that describes each D3DSIO_ pixelshader instruction.
//---------------------------------------------------------------------
typedef struct _D3DPixelShaderInstruction
{
    char    Text[PSTRANS_DISASM_STRING_LENGTH];
    DWORD*  pComment;
    DWORD   CommentSize;

    // instruction tokens
    size_t  ByteOffset; // offset of instruction in original D3D shader binary.
    DWORD   Opcode;
    DWORD   DstParam;
    DWORD   SrcParam[PSTR_MAX_NUMSRCPARAMS];
    DWORD   SrcParamRelAddr[PSTR_MAX_NUMSRCPARAMS];
    BOOL    bPredicated;
    DWORD   SrcPredicateToken;
    DWORD   DstParamCount;
    __field_range(0, PSTR_MAX_NUMSRCPARAMS) DWORD   SrcParamCount;
    DWORD   DclInfoToken; // only used by dcl statements.
    UINT    uiTSSNum;
    BOOL    bTexOp;

    union
    {
        // DEF'd constants (only for DEF statements)
        FLOAT   fDefValues[PSTR_NUM_COMPONENTS_IN_REGISTER];
        int     iDefValues[PSTR_NUM_COMPONENTS_IN_REGISTER];
        BOOL    bDefValue;
    };

    BOOL    bQueueWrite;
    BOOL    bFlushQueue;        // flush write - TRUE for all singly issued instructions,
                                // and for the last in any sequence of co-issued instructions.
} D3DPixelShaderInstruction;

//---------------------------------------------------------------------
// PSTR_REGISTER_TYPE
//
// Enum listing translated pixelshader register types
//---------------------------------------------------------------------
typedef enum _PSTR_REGISTER_TYPE
{
    PSTRREG_UNINITIALIZED_TYPE = 0,
    PSTRREG_INPUT,
    PSTRREG_TEMP,
    PSTRREG_CONST,
    PSTRREG_CONSTINT,
    PSTRREG_CONSTBOOL,
    PSTRREG_TEXTURE,
    PSTRREG_COLOROUT,
    PSTRREG_DEPTHOUT,
    PSTRREG_POSTMODSRC,
    PSTRREG_SCRATCH,
    PSTRREG_QUEUEDWRITE,
    PSTRREG_ZERO,
    PSTRREG_ONE,
    PSTRREG_0001,
    PSTRREG_XGRADIENT,
    PSTRREG_YGRADIENT,
    PSTRREG_POSITION,
    PSTRREG_FACE,
    PSTRREG_LOOPCOUNTER,
    PSTRREG_INTERNALLOOPCOUNTER,
    PSTRREG_REGISTERSTACK,
    PSTRREG_PREDICATE,
    PSTRREG_PREDICATETRUE,
    PSTRREG_COLOROUTWRITTENMASK,
} PSTR_REGISTER_TYPE;


// Type that is a pointer to an array of RGBA vectors.
typedef union _REGCONTENTS
{
    FLOAT   f[PSTR_NUM_COMPONENTS_IN_REGISTER]; // float
    FLOAT   i[PSTR_NUM_COMPONENTS_IN_REGISTER]; // int (storing as float just to keep access code uniform)
    FLOAT   b[PSTR_NUM_COMPONENTS_IN_REGISTER]; // bool (storing as float just to keep access code uniform)
    BYTE    mask;                               // predicate registers are a 4 channel mask.
} REGCONTENTS;

typedef REGCONTENTS* PREGQUAD;

//---------------------------------------------------------------------
// PSTRRegister
//
// Type used to refer to a register.
//---------------------------------------------------------------------
class PSTRRegister
{
private:
    PSTR_REGISTER_TYPE  m_RegType;
    UINT                m_RegNum;
    BOOL                m_bRelAddr;
    PSTR_REGISTER_TYPE  m_RelAddrRegType;
    UINT                m_RelAddrRegNum;
    BYTE                m_RelAddrSrcSelector;
    void SetImpl(PSTR_REGISTER_TYPE RegType, UINT RegNum,
                 BOOL bRelAddr,
                 PSTR_REGISTER_TYPE RelAddrRegType, UINT RelAddrRegNum,
                 BYTE RelAddrSelector)
    {
        m_RegType = RegType;
        m_RegNum = RegNum;
        m_bRelAddr = bRelAddr;
        m_RelAddrRegType = RelAddrRegType;
        m_RelAddrRegNum = RelAddrRegNum;
    }
public:
    PSTRRegister() {
        m_RegType = PSTRREG_UNINITIALIZED_TYPE;
        m_RegNum = (UINT)-1;
        m_bRelAddr = FALSE;
        m_RelAddrRegType = PSTRREG_UNINITIALIZED_TYPE;
        m_RelAddrRegNum = (UINT)-1;
        m_RelAddrSrcSelector = PSTR_SELECT_R;
    }
    void Set(PSTR_REGISTER_TYPE RegType, UINT RegNum)
    {
        SetImpl(RegType,RegNum,FALSE,PSTRREG_UNINITIALIZED_TYPE,(UINT)-1,PSTR_SELECT_R);
    }
    void SetRelAddr(PSTR_REGISTER_TYPE RegType, UINT RegNum,
                    PSTR_REGISTER_TYPE RelAddrRegType, UINT RelAddrRegNum,
                    BYTE RelAddrSelector)
    {
        SetImpl(RegType,RegNum,TRUE,PSTRREG_UNINITIALIZED_TYPE,(UINT)-1,PSTR_SELECT_R);
        m_RegType = RegType;
        m_RegNum = RegNum;
        m_bRelAddr = TRUE;
        m_RelAddrRegType = RelAddrRegType;
        m_RelAddrRegNum = RelAddrRegNum;
    }
    void UpdateRegNum(UINT RegNum)
    {
        m_RegNum = RegNum;
    }

    inline PSTR_REGISTER_TYPE GetRegType() const {return m_RegType;}
    inline UINT GetRegNum() const {return m_RegNum;}
    inline BOOL GetIsRelAddr() const {return m_bRelAddr;}
    inline PSTR_REGISTER_TYPE GetRelAddrRegType() const {return m_RelAddrRegType;}
    inline UINT GetRelAddrRegNum() const {return m_RelAddrRegNum;}
    inline BYTE GetRelAddrSrcSelector() const {return m_RelAddrSrcSelector;}
};

//---------------------------------------------------------------------
// PSTRPredInfo
//
// Encapsulates how a predicate is being read.
// Used as a parameter to any PSTR instruction that can be predicated.
//---------------------------------------------------------------------
class PSTRPredInfo
{
public:
    PSTRRegister    PredicateReg;
    BOOL            bInvertPredicate;
    BYTE            PredicateSwizzle;
};

//---------------------------------------------------------------------
// ConstDef_F
//
// Structure describing a "def c#, a, b, c, d" instruciton.
//---------------------------------------------------------------------
typedef struct _ConstDef_F
{
    float   f[4];
    UINT    RegNum;
} ConstDef_F;

//---------------------------------------------------------------------
// ConstDef_I
//
// Structure describing a "def i#, a, b, c, d" instruciton.
//---------------------------------------------------------------------
typedef struct _ConstDef_I
{
    int     i[4];
    UINT    RegNum;
} ConstDef_I;

//---------------------------------------------------------------------
// ConstDef_B
//
// Structure describing a "def b#, val" instruciton.
//---------------------------------------------------------------------
typedef struct _ConstDef_B
{
    BOOL    b;
    UINT    RegNum;
} ConstDef_B;

//---------------------------------------------------------------------
// PSTR_INSTRUCTION_OPCODE_TYPE
//
// "RISC" opcodes which are used to implement
// D3DSIO_ pixelshader instructions.
//---------------------------------------------------------------------
typedef enum _PSTR_INSTRUCTION_OPCODE_TYPE
{
    PSTRINST_BEM,
    PSTRINST_DEPTH,
    PSTRINST_DSTMOD,
    PSTRINST_END,
    PSTRINST_EVAL,
    PSTRINST_FORCELOD,
    PSTRINST_KILL,
    PSTRINST_LUMINANCE, // for ps_1_x
    PSTRINST_NEXTD3DPSINST,
    PSTRINST_QUADLOOPBEGIN,
    PSTRINST_QUADLOOPEND,
    PSTRINST_SAMPLE,
    PSTRINST_SRCMOD,    // for ps_1_x
    PSTRINST_SWIZZLE,
    PSTRINST_TEXCOVERAGE,
    // Flow control ops
    PSTRINST_CALL,
    PSTRINST_CALLNZ,
    PSTRINST_JUMP,
    PSTRINST_PUSHREG,
    PSTRINST_POPREG,
    PSTRINST_RET,
    // Arithmetic ops
    PSTRINST_ABS,
    PSTRINST_ADD,
    PSTRINST_CND,
    PSTRINST_CMP,
    PSTRINST_COS,
    PSTRINST_DSX,
    PSTRINST_DSY,
    PSTRINST_DP2ADD,
    PSTRINST_DP3,
    PSTRINST_DP4,
    PSTRINST_EXP,
    PSTRINST_FRC,
    PSTRINST_LEGACYRCP,
    PSTRINST_LOG,
    PSTRINST_LRP,
    PSTRINST_MAD,
    PSTRINST_MAX,
    PSTRINST_MIN,
    PSTRINST_MOV,
    PSTRINST_MUL,
    PSTRINST_RCP,
    PSTRINST_RSQ,
    PSTRINST_SETPRED,
    PSTRINST_SIN,

    // added by blakep
    PSTRINST_BEGINLOOP,
    PSTRINST_ENDLOOP,

    PSTRINST_BEGINREP,
    PSTRINST_ENDREP,

    PSTRINST_BREAK,

    PSTRINST_IF,
    PSTRINST_ENDIF,

    PSTRINST_ELSE,

    PSTRINST_DEFINESUB,

    PSTRINST_LOADCONSTBOOL,

} PSTR_INSTRUCTION_OPCODE_TYPE;

//---------------------------------------------------------------------
// PSTR_INSTRUCTION_OPCODE_TYPE Parameters
//
// Structures defining the parameters for all the "RISC" opcodes listed above.
// PSTRINST_BASE_PARAMS is the root from which the rest are inherited.
//---------------------------------------------------------------------
typedef UINT PSTR_INST_ID;

struct PSTRINST_BASE_PARAMS
{
public:
    union{
        PSTR_INSTRUCTION_OPCODE_TYPE   Inst;

        // Force structure alignment to pointer-size multiples.
        // IA64 (at least) needs this for structure packing to work.
        void*                          AlignmentDummy;
    };
    size_t      InstSize;      // Size of Current instruction

    PSTR_INST_ID D3DInstID;    // D3D instruction number (pre-translation)
                               // Numbering starts at 0, and includes everything
                               // including end token. For REF instructions,
                               // this will have the value -1.

    size_t       D3DInstByteOffset; // Instruction offset (in bytes) into the original
                               // D3D binary shader.  This is used to identify to
                               // the debugger what D3D instruction is current.
                               // For REF instructions, this will have the value -1.

    PSTR_INST_ID PSTRInstID;   // PSTR instruction number (post-translation)
                               // emulating a D3D instruction.
                               // Numbering starts at 0, and includes every
                               // PSTRINST in the post-translation shader,
                               // including the terminating PSTRINST_END.
};

typedef struct _PSTRINST_ABS_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_ABS_PARAMS;

typedef struct _PSTRINST_ADD_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_ADD_PARAMS;

typedef struct _PSTRINST_BEM_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BYTE            WriteMask;
    UINT            uiStage;
} PSTRINST_BEM_PARAMS;

typedef struct _PSTRINST_CALL_PARAMS : public PSTRINST_BASE_PARAMS
{
    UINT    Label;
} PSTRINST_CALL_PARAMS;

typedef struct _PSTRINST_CALLNZ_PARAMS : public PSTRINST_BASE_PARAMS
{
    UINT            Label;
    PSTRRegister    SrcReg0;
    BOOL            bInvertPredicate;
    BYTE            PredSwizzle;
} PSTRINST_CALLNZ_PARAMS;

typedef struct _PSTRINST_COS_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BOOL            bSrcReg0_Negate;
    BYTE            SrcReg0_Selector;  // PSTR_SELECT_R/G/B/A to select single
                               // component from source.
    BYTE            WriteMask; // Result is replicated to all
                               // components in writemask.
    PSTRPredInfo    Predication;
} PSTRINST_COS_PARAMS;

typedef struct _PSTRINST_CMP_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    PSTRRegister    SrcReg2;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BOOL            bSrcReg2_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_CMP_PARAMS;

typedef struct _PSTRINST_CND_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    PSTRRegister    SrcReg2;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BOOL            bSrcReg2_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_CND_PARAMS;

typedef struct _PSTRINST_DEPTH_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    SrcReg0;
} PSTRINST_DEPTH_PARAMS;

typedef struct _PSTRINST_DP2ADD_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    PSTRRegister    SrcReg2;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BOOL            bSrcReg2_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_DP2ADD_PARAMS;

typedef struct _PSTRINST_DP3_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_DP3_PARAMS;

typedef struct _PSTRINST_DP4_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_DP4_PARAMS;

typedef struct _PSTRINST_DSTMOD_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    BYTE            WriteMask;
    FLOAT           fScale;
    FLOAT           fRangeMin;
    FLOAT           fRangeMax;
    PSTRPredInfo    Predication;
} PSTRINST_DSTMOD_PARAMS;

typedef struct _PSTRINST_DSX_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BYTE            WriteMask;
    BOOL            bQuadPixelShared; // TRUE:  Being called once for
                                      //        a four pixel chunk (same result
                                      //        used by each of the pixels)
                                      // FALSE: Being called specifically for a single pixel.
    PSTRPredInfo    Predication;
} PSTRINST_DSX_PARAMS;

typedef struct _PSTRINST_DSY_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BYTE            WriteMask;
    BOOL            bQuadPixelShared; // TRUE:  Being called once for
                                      //        a four pixel chunk (same result
                                      //        used by each of the pixels)
                                      // FALSE: Being called specifically for a single pixel.
    PSTRPredInfo    Predication;
} PSTRINST_DSY_PARAMS;

typedef struct _PSTRINST_END_PARAMS : public PSTRINST_BASE_PARAMS
{
} PSTRINST_END_PARAMS;

typedef struct _PSTRINST_EVAL_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    UINT            RDAttrBaseIndex; // attr index for x (others follow sequentially)
    BYTE            WriteMask;
    BOOL            bIgnoreD3DTTFF_PROJECTED;
    BOOL            bSampleAtCentroidWhenMultisampling;
    BOOL            bClamp;
    D3DDECLUSAGE    Usage;
    UINT            UsageIndex;
} PSTRINST_EVAL_PARAMS;

typedef struct _PSTRINST_EXP_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BOOL            bSrcReg0_Negate;
    BYTE            SrcReg0_Selector;  // PSTR_SELECT_R/G/B/A to select single
                               // component from source.
    BYTE            WriteMask; // Result is replicated to all
                               // components in writemask.
    PSTRPredInfo    Predication;
} PSTRINST_EXP_PARAMS;

typedef struct _PSTRINST_FRC_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BOOL            bSrcReg0_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_FRC_PARAMS;

typedef struct _PSTRINST_JUMP_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTR_INST_ID            Destination_PSTRInstID;
    size_t                  Destination_Offset; // Offset from start of buffer.
    PSTRPredInfo            Predication;
} PSTRINST_JUMP_PARAMS;

typedef struct _PSTRINST_KILL_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    SrcReg0;
    BYTE            WriteMask;
    BYTE            bKillLZ[4]; // TRUE means kill < 0, FALSE means kill >= 0.  [4] means per-component.
    PSTRPredInfo    Predication;
} PSTRINST_KILL_PARAMS;

typedef struct _PSTRINST_LEGACYRCP_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BOOL            bSrcReg0_Negate;
    BYTE            SrcReg0_Selector;  // PSTR_SELECT_R/G/B/A to select single
                                       // component from source.
    BYTE            WriteMask; // Result is replicated to all
                               // components in writemask.
    FLOAT           fRangeMax;
    PSTRPredInfo    Predication;
} PSTRINST_LEGACYRCP_PARAMS;

typedef struct _PSTRINST_LOG_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BOOL            bSrcReg0_Negate;
    BYTE            SrcReg0_Selector;  // PSTR_SELECT_R/G/B/A to select single
                                       // component from source.
    BYTE            WriteMask; // Result is replicated to all
                               // components in writemask.
    PSTRPredInfo    Predication;
} PSTRINST_LOG_PARAMS;

typedef struct _PSTRINST_LRP_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    PSTRRegister    SrcReg2;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BOOL            bSrcReg2_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_LRP_PARAMS;

typedef struct _PSTRINST_LUMINANCE_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    UINT            uiStage;
} PSTRINST_LUMINANCE_PARAMS;

typedef struct _PSTRINST_MAD_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    PSTRRegister    SrcReg2;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BOOL            bSrcReg2_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_MAD_PARAMS;

typedef struct _PSTRINST_MAX_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_MAX_PARAMS;

typedef struct _PSTRINST_MIN_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_MIN_PARAMS;

typedef struct _PSTRINST_MOV_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BOOL            bSrcReg0_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_MOV_PARAMS;

typedef struct _PSTRINST_MUL_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    PSTRRegister    SrcReg1;
    BOOL            bSrcReg0_Negate;
    BOOL            bSrcReg1_Negate;
    BYTE            WriteMask;
    PSTRPredInfo    Predication;
} PSTRINST_MUL_PARAMS;

typedef struct _PSTRINST_NEXTD3DPSINST_PARAMS : public PSTRINST_BASE_PARAMS
{
    D3DPixelShaderInstruction* pInst;
} PSTRINST_NEXTD3DPSINST_PARAMS;

typedef struct _PSTRINST_PUSHREG_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    SrcReg0;
    BYTE            WriteMask;
} PSTRINST_PUSHREG_PARAMS;

typedef struct _PSTRINST_POPREG_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    BYTE            WriteMask;
} PSTRINST_POPREG_PARAMS;

typedef struct _PSTRINST_QUADLOOPBEGIN_PARAMS : public PSTRINST_BASE_PARAMS
{
} PSTRINST_QUADLOOPBEGIN_PARAMS;

typedef struct _PSTRINST_QUADLOOPEND_PARAMS : public PSTRINST_BASE_PARAMS
{
    size_t      JumpBackByOffset;
} PSTRINST_QUADLOOPEND_PARAMS;

typedef struct _PSTRINST_RCP_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BOOL            bSrcReg0_Negate;
    BYTE            SrcReg0_Selector;  // PSTR_SELECT_R/G/B/A to select single
                                       // component from source.
    BYTE            WriteMask; // Result is replicated to all
                               // components in writemask.
    PSTRPredInfo    Predication;
} PSTRINST_RCP_PARAMS;

typedef struct _PSTRINST_RET_PARAMS : public PSTRINST_BASE_PARAMS
{
} PSTRINST_RET_PARAMS;

typedef struct _PSTRINST_RSQ_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BOOL            bSrcReg0_Negate;
    BYTE            SrcReg0_Selector;  // PSTR_SELECT_R/G/B/A to select single
                                       // component from source.
    BYTE            WriteMask; // Result is replicated to all
                               // components in writemask.
    PSTRPredInfo    Predication;
} PSTRINST_RSQ_PARAMS;

typedef struct _PSTRINST_SAMPLE_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister        DstReg;
    PSTRRegister        CoordReg;
    BYTE                WriteMask;
    UINT                uiStage;
    PSTRPredInfo        Predication;
    BOOL                bAllowLegacyApproximations; // Allow approximations in the LOD calculation (1_x shaders only)
    BOOL                bLODBiasFromW;      // added by blakep
    BOOL                bForceLODFromW;     // added by blakep
    BOOL                bAlternateGradient; // added by blakep
    PSTRRegister        SrcXGradient;       // added by blakep
    PSTRRegister        SrcYGradient;       // added by blakep
} PSTRINST_SAMPLE_PARAMS;

typedef struct _PSTRINST_SETPRED_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister            DstReg;
    PSTRRegister            SrcReg0;
    PSTRRegister            SrcReg1;
    BOOL                    bSrcReg0_Negate;
    BOOL                    bSrcReg1_Negate;
    D3DSHADER_COMPARISON    Comparison;
    BYTE                    WriteMask;
} PSTRINST_SETPRED_PARAMS;

typedef struct _PSTRINST_SIN_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BOOL            bSrcReg0_Negate;
    BYTE            SrcReg0_Selector;  // PSTR_SELECT_R/G/B/A to select single
                                       // component from source.
    BYTE            WriteMask; // Result is replicated to all
                               // components in writemask.
    PSTRPredInfo    Predication;
} PSTRINST_SIN_PARAMS;

typedef struct _PSTRINST_SRCMOD_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BYTE            WriteMask;
    BOOL            bBias;
    BOOL            bTimes2;
    BOOL            bComplement;
    FLOAT           fRangeMin;
    FLOAT           fRangeMax;
    PSTRPredInfo    Predication;
} PSTRINST_SRCMOD_PARAMS;

typedef struct _PSTRINST_SWIZZLE_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
    BYTE            WriteMask;
    BYTE            Swizzle;
    PSTRPredInfo    Predication;
} PSTRINST_SWIZZLE_PARAMS;

typedef struct _PSTRINST_TEXCOVERAGE_PARAMS : public PSTRINST_BASE_PARAMS
{
    UINT            uiStage;
    PSTRRegister    SrcXGradient;
    PSTRRegister    SrcYGradient;
    BOOL            bQuadPixelShared; // TRUE:  Being called once for
                                      //        a four pixel chunk (same result
                                      //        used by each of the pixels)
                                      // FALSE: Being called specifically for a single pixel.
    BOOL            bAllowLegacyApproximations; // Allow approximations in the LOD calculation (1_x shaders only)
} PSTRINST_TEXCOVERAGE_PARAMS;

typedef struct _PSTRINST_FORCELOD_PARAMS : public PSTRINST_BASE_PARAMS
{
    UINT            uiStage;
    PSTRRegister    LODRegister;
    BYTE            LODComponent_Selector;  // PSTR_SELECT_R/G/B/A to select single
                                            // component from source.
} PSTRINST_FORCELOD_PARAMS;

typedef struct _PSTRINST_BEGINLOOP_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister SrcReg0; // x = iterations, y = initial value, z = increment
} PSTRINST_BEGINLOOP_PARAMS;

typedef struct _PSTRINST_ENDLOOP_PARAMS : public PSTRINST_BASE_PARAMS
{
} PSTRINST_ENDLOOP_PARAMS;

typedef struct _PSTRINST_BEGINREP_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister SrcReg0; // x = iterations
} PSTRINST_BEGINREP_PARAMS;

typedef struct _PSTRINST_ENDREP_PARAMS : public PSTRINST_BASE_PARAMS
{
} PSTRINST_ENDREP_PARAMS;

typedef struct _PSTRINST_BREAK_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRPredInfo Predication;
} PSTRINST_BREAK_PARAMS;

typedef struct _PSTRINST_IF_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRPredInfo Predication;
} PSTRINST_IF_PARAMS;

typedef struct _PSTRINST_ENDIF_PARAMS : public PSTRINST_BASE_PARAMS
{
} PSTRINST_ENDIF_PARAMS;

typedef struct _PSTRINST_ELSE_PARAMS : public PSTRINST_BASE_PARAMS
{
} PSTRINST_ELSE_PARAMS;

typedef struct _PSTRINST_DEFINESUB_PARAMS : public PSTRINST_BASE_PARAMS
{
    UINT Label;
} PSTRINST_DEFINESUB_PARAMS;

typedef struct _PSTRINST_LOADCONSTBOOL_PARAMS : public PSTRINST_BASE_PARAMS
{
    PSTRRegister    DstReg;
    PSTRRegister    SrcReg0;
} PSTRINST_LOADCONSTBOOL_PARAMS;

// End of "RISC" instruction parameter definitions

//---------------------------------------------------------------------
// Helper classes used by translator to track flow control
//---------------------------------------------------------------------
class CLabelTrack
{
private:
    typedef struct _NEEDED_LABEL_NODE
    {
        UINT                    LabelID;
        size_t                  OffsetToOutputLabelOffsetWhenDefined;
        size_t                  OffsetToOutputLabelPSTRInstIDWhenDefined;
        _NEEDED_LABEL_NODE*     pNext;
        _NEEDED_LABEL_NODE*     pPrev;
    } NEEDED_LABEL_NODE;

    typedef struct _DEFINED_LABEL_NODE
    {
        UINT                    LabelID;
        size_t                  LabelOffset;
        PSTR_INST_ID            LabelPSTRInstID;
        _DEFINED_LABEL_NODE*    pNext;
        _DEFINED_LABEL_NODE*    pPrev;
    } DEFINED_LABEL_NODE;

    NEEDED_LABEL_NODE* m_pNeededLabelList;
    DEFINED_LABEL_NODE* m_pDefinedLabelList;
    DEFINED_LABEL_NODE* FindDefinedLabel(UINT LabelID);
    void ResolveAndDeleteNeededLabel(NEEDED_LABEL_NODE* pNeededLabel, const DEFINED_LABEL_NODE* pDefinedLabel, BYTE* pPSTRInstBuffer);
    void ResolveNeededLabel(size_t OffsetToOutputLabelOffsetWhenDefined, size_t OffsetToOutputLabelPSTRInstIDWhenDefined, const DEFINED_LABEL_NODE* pDefinedLabel, BYTE* pPSTRInstBuffer);
public:
    CLabelTrack() { m_pNeededLabelList = NULL; m_pDefinedLabelList = NULL; };
    ~CLabelTrack();
    HRESULT NeedLabel(UINT LabelID, size_t OffsetToOutputLabelOffsetWhenDefined, size_t OffsetToOutputLabelPSTRInstIDWhenDefined,BYTE* pPSTRInstBuffer);
    HRESULT AddLabel(UINT LabelID, size_t LabelOffset, PSTR_INST_ID LabelPSTRInstID, BYTE* pPSTRInstBuffer);
    BOOL LabelsAreStillNeeded() const {return (NULL != m_pNeededLabelList);};
};

class CLoopNestTrack
{
private:
    typedef struct _BREAK_INFO_NODE
    {
        size_t            OffsetToOutputLoopEndOffsetWhenDefined;
        size_t            OffsetToOutputLoopEndPSTRInstIDWhenDefined;
        _BREAK_INFO_NODE* pNext;
    } BREAK_INFO_NODE;

    typedef struct _LOOPSTART_INFO_NODE
    {
        BOOL                    bIsLoop; // true means loop, false means rep
        size_t                  OffsetToOutputLoopEndOffsetWhenDefined;
        size_t                  OffsetToOutputLoopEndPSTRInstIDWhenDefined;
        size_t                  LoopStartOffset;
        PSTR_INST_ID            LoopStartPSTRInstID;
        BREAK_INFO_NODE*        pBreakList;
        _LOOPSTART_INFO_NODE*   pNext;
    } LOOPSTART_INFO_NODE;

    LOOPSTART_INFO_NODE*    m_pStartedLoopStack;
    UINT                    m_StackDepth;
    UINT                    m_NumNestedLoopsExcludingReps;
public:
    CLoopNestTrack() { m_pStartedLoopStack = NULL; m_NumNestedLoopsExcludingReps = 0; m_StackDepth = 0;};
    ~CLoopNestTrack();
    HRESULT LoopStart(BOOL    bIsLoop, // true means loop, false means rep
                      size_t  OffsetToOutputLoopEndOffsetWhenDefined,
                      size_t  OffsetToOutputLoopEndPSTRInstIDWhenDefined,
                      size_t  LoopStartOffset,
                      PSTR_INST_ID LoopStartPSTRInstID );

    HRESULT Break(size_t  OffsetToOutputLoopEndOffsetWhenDefined,
                  size_t  OffsetToOutputLoopEndPSTRInstIDWhenDefined);

    HRESULT LoopEnd(BOOL    bIsLoop, // true means loop, false means rep
                    size_t  OffsetToOutputLoopStartOffset,
                    size_t  OffsetToOutputLoopStartPSTRInstID,
                    size_t  LoopEndOffset,
                    PSTR_INST_ID LoopEndPSTRInstID,
                    BYTE*  pPSTRInstBuffer );

    UINT GetStackDepth() const {return m_StackDepth;};
    UINT GetNumNestedLoopsExcludingReps() const {return m_NumNestedLoopsExcludingReps;};

};

class CIfNestTrack
{
private:
    typedef struct _IF_INFO_NODE
    {
        size_t                  OffsetToOutputElseOrEndifOffsetWhenDefined;
        size_t                  OffsetToOutputElseOrEndifPSTRInstIDWhenDefined;
        BOOL                    bSeenElse;
        _IF_INFO_NODE*          pNext;
    } IF_INFO_NODE;

    IF_INFO_NODE*           m_pStartedIfStack;
    UINT                    m_StackDepth;
public:
    CIfNestTrack() { m_pStartedIfStack = NULL; m_StackDepth = 0;};
    ~CIfNestTrack();
    HRESULT If(size_t  OffsetToOutputElseOrEndifOffsetWhenDefined,
               size_t  OffsetToOutputElseOrEndifPSTRInstIDWhenDefined);

    HRESULT Else(size_t  OffsetToOutputEndifOffsetWhenDefined,
                 size_t  OffsetToOutputEndifPSTRInstIDWhenDefined,
                 size_t  ElseOffset,
                 PSTR_INST_ID  ElsePSTRInstID,
                 BYTE*  pPSTRInstBuffer);

    HRESULT Endif(size_t EndifOffset,
                  PSTR_INST_ID EndifPSTRInstID,
                  BYTE*  pPSTRInstBuffer);

    UINT GetStackDepth() const {return m_StackDepth;};

};

class CInputRegDclInfo
{
public:
    typedef struct _INPUT_DCL_NODE
    {
        D3DDECLUSAGE            Usage;
        UINT                    Index;
        PSTR_REGISTER_TYPE      PSTRRegType;
        UINT                    RegNum;
        BYTE                    WriteMask;
        BOOL                    bSampleAtCentroidWhenMultisampling;
        _INPUT_DCL_NODE*        pNext;
    } INPUT_DCL_NODE;
private:
    INPUT_DCL_NODE*             m_pInputDclList;
public:
    CInputRegDclInfo() {m_pInputDclList = NULL;}
    ~CInputRegDclInfo();
    HRESULT                     AddNewDcl(  D3DDECLUSAGE Usage, UINT Index, PSTR_REGISTER_TYPE PSTRRegType, UINT RegNum, 
                                            BYTE WriteMask, BOOL bSampleAtCentroidWhenMultisampling = FALSE );
    INPUT_DCL_NODE*             GetInputDclList() {return m_pInputDclList;}
    BOOL                        IsRegDeclared(PSTR_REGISTER_TYPE PSTRRegType, UINT RegNum, BYTE WriteMask);
};

//---------------------------------------------------------------------
// Flags controllsing behavior of Pixel Shader Translator (CPSTrans)
//---------------------------------------------------------------------

// Whether to keep extraneous instruction list
// after parsing shader into instruction buffer.
// List is useful for debug purposes.
#define PSTRANS_FLAGS_KEEP_DEBUGINFO_INSTRUCTION_LIST   0x00000001

// Whether to insert marker instructions (PSTRINST_NEXTD3DPSINST)
// into the output translated instruction buffer, indicating when
// each new D3D pixelshader instruction is encountered.
// This flag is ignored if PSTRANS_FLAGS_KEEP_DEBUGINFO_INSTRUCTION_LIST
// is not set.
#define PSTRANS_FLAGS_INSERT_D3DPSINST_MARKERS          0x00000002

// Whether to output debug spew of translated pixelshader.
#define PSTRANS_FLAGS_DEBUGPRINT_TRANSLATED_PIXELSHADER 0x00000004

// Allow texture filtering to use legacy approximations.
#define PSTRANS_FLAGS_ENABLE_LEGACY_APPROXIMATIONS     0x00000008
//---------------------------------------------------------------------
// CPSTrans
//
// PixelShader translator abstract base class.
// To use this class with your code:
// 1) Derive from it and implement
//    a growable BYTE* array via the pure virtual methods:
//      virtual void    SetOutputBufferGrowSize(DWORD dwGrowSize) = 0;
//      virtual HRESULT GrowOutputBuffer(DWORD dwNewSize) = 0;
//      virtual BYTE*   GetOutputBufferI() = 0;
//
// 2) Instantiate the derived class and Initialize() (see below) from
//    the constructor.  Inputs to Initialize are all the parameters that
//    define the pixelshader and how to translate it.
//
// 3) Call HRESULT GetStatus() to see if the translation succeeded.
//
// 4) Call the various Get*() methods to retrieve information about
//    the translated pixel shader, including BYTE* GetOutputBuffer()
//    to retrieve the actual block of memory containing the translated
//    shader instruction list itself.  See psexec.cpp in refrast for
//    how to interpret the contents of this buffer.
//
//---------------------------------------------------------------------
class CPSTrans
{
protected:
    HRESULT                     m_Status;

    UINT                        m_cD3DInst; // instruction count (pre-translation, including end inst)
    UINT                        m_cPSTRInst; // instructions count (post-translation, including end inst)
    size_t                      m_EndOffset; // offset to end of shader (see clarification comment for GetEndOffset() below)
    D3DPixelShaderInstruction*  m_pD3DPixelShaderInstructionArray;
    DWORD*                      m_pCode; // copy of shader

    UINT                        m_cConstDefsF;
    ConstDef_F*                 m_pConstDefsF;
    UINT                        m_cConstDefsI;
    ConstDef_I*                 m_pConstDefsI;
    UINT                        m_cConstDefsB;
    ConstDef_B*                 m_pConstDefsB;

    CLabelTrack                 m_LabelTracker;
    CIfNestTrack                m_IfNestTracker;
    CLoopNestTrack              m_LoopNestTracker;

    DWORD       m_ColorOutPresentMask;     // mask for which oC# registers are present, ignoring flow
                                           // control, and ignoring component masks: all components or nothing,
                                           // so 1 bit per oC#.  LSB is oC0.

                                // component masks for declared registers
    CInputRegDclInfo            m_InputRegDclInfo;      // (these apply to ps_2_0+ only)
    D3DSAMPLER_TEXTURE_TYPE     m_SamplerRegDcl[PSTR_MAX_TEXTURE_SAMPLERS];   // (these apply to ps_2_0+ only)
    UINT                        m_cActiveTextureStages;

    BOOL                        m_bHasTexKillInstructions; // blakep

    virtual void                SetOutputBufferGrowSize(DWORD dwGrowSize) = 0;
    virtual HRESULT             GrowOutputBuffer(DWORD dwNewSize) = 0;
    virtual BYTE*               GetOutputBufferI() = 0; // Output buffer to be destroyed on ~CPSTrans()
    size_t                      GetInstructionSize(PSTR_INSTRUCTION_OPCODE_TYPE Inst);

    void Initialize(    const DWORD* pCode,
                        const DWORD ByteCodeSize,
                        const DWORD Flags);

    // -------------------------------------------------------------------------
    // Some variables needed ONLY during Initialize()'s third pass through shader.
    // (lifetime is short, but multiple member functions need access)
    class CWorkerData
    {
    public:
        CWorkerData()
        {
            pPSTRInstBuffer             = NULL;
            pPSTRInst                   = NULL;
            PSTROffset                  = 0;
            PSTRLoopOffset              = 0;
            LastPSTRInstSize            = 0;
            D3DInstID                   = 0;  // pre-translated D3D instruction #
            pInst                       = NULL;
            bInQuadPixelLoop            = FALSE;
            bQueuedEnterQuadPixelLoop   = FALSE;
            DstScale                    = 1.0f;
            DstRange[0]                 = -XFLOAT_MAX;
            DstRange[1]                 = XFLOAT_MAX;
        };
        BYTE*                       pPSTRInstBuffer;
        BYTE*                       pPSTRInst;
        size_t                      PSTROffset;
        size_t                      PSTRLoopOffset;
        size_t                      LastPSTRInstSize;
        UINT                        D3DInstID;
        D3DPixelShaderInstruction*  pInst;
        PSTRPredInfo                PredicateInfo;
        PSTRPredInfo                ForceNoPredication;
        bool                        bInQuadPixelLoop;
        bool                        bQueuedEnterQuadPixelLoop;
        FLOAT                       DstScale;           // Result Shift Scale - +/- 2**n only
        FLOAT                       DstRange[2];        // clamp dest to this range

    };
    CWorkerData*                m_pWorkerData;

    // Helper functions used by Initialize()'s third pass through shader,
    // using worker data above.
    size_t  _GetParamOffset(void* pParam) const;
    size_t  _GetOffset() const;
    size_t  _GetNextInstOffset() const;
    UINT    _GetNextPSTRInstID() const;
    void    _NewPSInstImpl(PSTR_INSTRUCTION_OPCODE_TYPE Inst);
    void    _EnterQuadPixelLoop();
    void    _LeaveQuadPixelLoop();
    void    _NewPSInst(PSTR_INSTRUCTION_OPCODE_TYPE Inst);
    void    _NoteInstructionEvent();
    void    _NoteInstructionEventNOBREAK();
    void    _EmitDstMod(const PSTRRegister& DstReg, BYTE WriteMask);
    void    _EmitProj(const PSTRRegister& DstReg,
                      const PSTRRegister& SrcReg,
                      BYTE ProjComponentMask, FLOAT fRangeMax, BOOL bLegacyRCP);
    // End of helper functions used by Initialize()'s third pass through shader.
    // -------------------------------------------------------------------------

public:
    CPSTrans();
    ~CPSTrans();

    D3DPixelShaderInstruction* GetPixelShaderInstructionArray()
    {
        return m_pD3DPixelShaderInstructionArray;
        // Will be NULL if PSTRANS_FLAGS_KEEP_DEBUGINFO_INSTRUCTION_LIST flag not used
        // with CPSTrans::CPSTrans()
        // If not null, note that pointer returned will be deleted on ~CPSTrans().
    }

    UINT        GetD3DInstCount() const { return m_cD3DInst; }; // pre-translation instruction count (including end)
    UINT        GetPSTRInstCount() const { return m_cPSTRInst; }; // translated op count (including end)
    size_t      GetEndOffset() const { return m_EndOffset; }; // byte offset (in translated shader) to shader end token.
                                                        // If D3DSIO_* instructions are left interleaved in the translated shader,
                                                        // for debugger support, then this offset point to the PSTRINST_NEXTD3DPSINST
                                                        // corresponding to D3DSIO_END.  Otherwise this will point to PSTRINST_END.
    UINT        GetNumConstDefsF() const { return m_cConstDefsF; };
    ConstDef_F* GetConstDefsF() { return m_pConstDefsF; };
    UINT        GetNumConstDefsI() const { return m_cConstDefsI; };
    ConstDef_I* GetConstDefsI() { return m_pConstDefsI; };
    UINT        GetNumConstDefsB() const { return m_cConstDefsB; };
    ConstDef_B* GetConstDefsB() { return m_pConstDefsB; };
    CInputRegDclInfo* GetInputRegDclInfo() {return &m_InputRegDclInfo;}; // only applies to ps_2_0+
    D3DSAMPLER_TEXTURE_TYPE* GetSamplerRegDcl() { return m_SamplerRegDcl; }; // only applies to ps_2_0+
    DWORD       GetColorOutPresentMask() const {return m_ColorOutPresentMask; }; // mask for oC# registers appearing in shader (ignoring flow control)
    UINT        GetActiveTextureStageCount() const { return m_cActiveTextureStages; };
    BOOL        HasTexKillInstructions() const { return m_bHasTexKillInstructions; }
    BYTE*       GetOutputBuffer()
    {
        if(SUCCEEDED(m_Status))
            return GetOutputBufferI();
        else
            return NULL;
    };
    HRESULT     GetStatus() const { return m_Status; };
};

//---------------------------------------------------------------------
// Utility functions
//---------------------------------------------------------------------
void CalculateSourceReadMasks(const D3DPixelShaderInstruction* pInst,
                              BYTE* pSourceReadMasks,
                              BOOL bAfterSwizzle,
                              const D3DSAMPLER_TEXTURE_TYPE* pSamplerDcl, // ps_2_0 only
                              DWORD dwVersion);

D3DSHADER_COMPARISON GetOppositeComparison(D3DSHADER_COMPARISON Comparison);




