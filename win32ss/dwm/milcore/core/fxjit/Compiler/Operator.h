// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions of class COperator.
//
//-----------------------------------------------------------------------------
#pragma once

class CAssembleContext;
class CMapper;
class CShuffleRecord;
class CInstruction;
class CConnector;

struct Link;
struct Hook;

#if WPFGFX_FXJIT_X86
enum VariableType : UINT8
{
    vtPointer   = 0,
    vtUINT32    = 1,
    vtMm        = 2,
    vtXmm       = 3,
    vtXmmF1     = 4,
    vtXmmF4     = 5,
};
#else //_AMD64_
enum VariableType : UINT8
{
    vtPointer   = 0,
    vtUINT32    = 1,
    vtUINT64    = 2,
    vtXmm       = 3,
    vtXmmF1     = 4,
    vtXmmF4     = 5,
};
#endif

struct SOperator
{
    OpType m_ot;
    UINT8 m_bImmediateByte;

    UINT8 m_uFlags; // temporary data

    RefType m_refType;
    union
    {
        UINT_PTR m_uDisplacement;
        void * m_pData;
    };

    // variable indices (0 == unused)
    UINT32 m_vResult;
    UINT32 m_vOperand1;
    UINT32 m_vOperand2;
    UINT32 m_vOperand3;

    // immediate arguments: content depends on m_ot
    union{
        SOperator *m_pLinkedOperator;   // branch operators are always paired via this pointer

        UINT32 m_immediateData;

        UINT32 m_shift;

        INT32 m_nOffset;
    };//union
};

enum OpFlags
{
    //
    // 4 bits define data type that this operator operates
    //
    ofDataNone          = 0x00000000,
    ofDataR32           = 0x00000001,   // mov
    ofDataM32           = 0x00000002,   // movd
    ofDataM64           = 0x00000003,   // movq
    ofDataI32           = 0x00000004,   // movd
    ofDataI64           = 0x00000005,   // movq
    ofDataI128          = 0x00000006,   // movdqa
    ofDataF32           = 0x00000007,   // movss
    ofDataF128          = 0x00000008,   // movps
    ofDataR64           = 0x00000009,   // REX mov
    ofDataMask          = 0x0000000F,

    ofChangesZF         = 0x00000010,   // this operator changes flag ZF maybe without purpose
    ofCalculatesZF      = 0x00000020,   // this operator changes flag ZF on purpose
    ofConsumesZF        = 0x00000040,   // this operator needs ZF provided by one with ofCalculatesZF
    ofIsControl         = 0x00000080,
    ofIsBranchSplit     = 0x00000100,
    ofIsLoopRepeat      = 0x00000200,

    ofHasOutsideEffect      = 0x00000400,   // the operator can't be removed on optimization since it changes some
                                            // external state (e.g. a memory outside of stack frame)
    ofHasOutsideDependency  = 0x00000800,   // The operator explicitly depends on the memory outside of stack frame

    ofCanTakeOperand1FromMemory = 0x00001000,
    ofCanTakeOperand2FromMemory = 0x00002000,
    ofCanSwapOperands           = 0x00004000,   // this operator allows exchanging arguments

    ofNoBubble          = 0x00008000,   // Bubbler is not allowed to pull register load/unload instruction over this operator
    ofUsesMMX           = 0x00010000,   // this operator involves MMX instruction extension
    ofHasImmediateByte  = 0x00020000,   // instruction code is followed by immediate byte given in m_bImmediateByte

    ofIrregular         = 0x00040000,   // instruction requires special register mapping
    ofStandardBinary    = 0x00080000,   // operator with two operands that respects RefType notation
    ofStandardUnary     = 0x00100000,   // operator with one operand that respects RefType notation
    ofStandardMemDst    = 0x00200000,   // operator with memory destination operand that respects RefType notation
    ofNonTemporalStore  = 0x00400000,   // operator makes non-temporal store to memory
    ofHasOpcodeSuffix   = 0x00800000,   // instruction code is followed by immediate byte derived from opcode
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      COperator
//
//  Synopsis:
//      Holds the formal description of operator as an elementary action.
//      Roughly, COperator defines one processor instruction.
//      However, in certain condition this instruction can be accompanied
//      with one or several "mov" instructions to pump data between registers
//      and stack frame memory.
//
//      The description contains:
//          m_ot:   defines the action specified by the operator
//          m_vResult:  index of the variable where result of action should go.
//                      When operator has no output value, m_vResult contains 0.
//          m_vOperand1, m_vOperand2, m_vOperand3:
//                      indices of the variables where operator takes data.
//                      0 when unused. When operator has only one or two operands,
//                      they are specified by m_vOperand1 or (m_vOperand1, m_vOperand2).
//
//      Depending on operator type, operator can contain immediate arguments
//      like a constant to add, offset in array or structure, amount of
//      bits to shift, etc.
//
//      The instance of COperator serves also as an entity to keep intermediate
//      results during compilation process.
//
//------------------------------------------------------------------------------
class COperator : public SOperator, public CFlushObject
{
public:
    COperator(
        OpType ot = otNone,
        UINT32 vResult = 0,
        UINT32 vOperand1 = 0,
        UINT32 vOperand2 = 0,
        UINT32 vOperand3 = 0
        );

    void Assemble(CAssembleContext & actx);
    void AssembleBinary(CAssembleContext & actx);
    void AssembleUnary(CAssembleContext & actx);
    void AssembleMemDst(CAssembleContext & actx);
    void AssembleIrregular(CAssembleContext & actx);

    UINT32 GetBinaryOffset() const
    {
        return m_uBinaryOffset;
    }

    UINT32 GetFlags() const
    {
        return sc_opFlags[m_ot];
    }

    bool IsControl() const
    {
        return (sc_opFlags[m_ot] & ofIsControl) != 0;
    }

    bool IsLoopStart() const
    {
        return (m_ot == otLoopStart) != 0;
    }

    bool IsLoopRepeat() const
    {
        return (sc_opFlags[m_ot] & ofIsLoopRepeat) != 0;
    }

    bool IsBranchSplit() const
    {
        return (sc_opFlags[m_ot] & ofIsBranchSplit) != 0;
    }

    bool IsBranchMerge() const
    {
        return (m_ot == otBranchMerge) != 0;
    }

    bool HasOutsideEffect() const
    {
        return (sc_opFlags[m_ot] & ofHasOutsideEffect) != 0;
    }

    bool HasOutsideDependency() const
    {
        return (sc_opFlags[m_ot] & ofHasOutsideDependency) != 0;
    }

    bool CanTakeOperand1FromMemory() const
    {
        return (sc_opFlags[m_ot] & ofCanTakeOperand1FromMemory) != 0;
    }

    bool CanTakeOperand2FromMemory() const
    {
        return (sc_opFlags[m_ot] & ofCanTakeOperand2FromMemory) != 0;
    }

    bool CanSwapOperands() const
    {
        return m_refType == RefType_Direct && (sc_opFlags[m_ot] & ofCanSwapOperands) != 0;
    }

    bool ChangesZF() const
    {
        return (sc_opFlags[m_ot] & ofChangesZF) != 0;
    }

    bool CalculatesZF() const
    {
        return (sc_opFlags[m_ot] & ofCalculatesZF) != 0;
    }

    bool ConsumesZF() const
    {
        return (sc_opFlags[m_ot] & ofConsumesZF) != 0;
    }


    bool NoBubble() const
    {
        return (sc_opFlags[m_ot] & ofNoBubble) != 0;
    }

    bool UsesMMX() const
    {
        return (sc_opFlags[m_ot] & ofUsesMMX) != 0;
    }

    bool HasImmediateByte() const
    {
        return (sc_opFlags[m_ot] & ofHasImmediateByte) != 0;
    }

    bool HasOpcodeSuffix() const
    {
        return (sc_opFlags[m_ot] & ofHasOpcodeSuffix) != 0;
    }

    bool IsIrregular() const
    {
        return (sc_opFlags[m_ot] & ofIrregular) != 0;
    }

    bool IsStandardBinary() const
    {
        return (sc_opFlags[m_ot] & ofStandardBinary) != 0;
    }

    bool IsStandardUnary() const
    {
        return (sc_opFlags[m_ot] & ofStandardUnary) != 0;
    }

    bool IsStandardMemDst() const
    {
        return (sc_opFlags[m_ot] & ofStandardMemDst) != 0;
    }

    // returns data type for operands, result might differ
    UINT32 GetDataType() const
    {
        return sc_opFlags[m_ot] & ofDataMask;
    }

    RegXMM RegXMMResult  () const { return m_rResult  .XMM(); }
    RegXMM RegXMMOperand1() const { return m_rOperand1.XMM(); }
    RegXMM RegXMMOperand2() const { return m_rOperand2.XMM(); }
    RegXMM RegXMMOperand3() const { return m_rOperand3.XMM(); }

#if WPFGFX_FXJIT_X86
    RegMMX RegMMXResult  () const { return m_rResult  .MMX(); }
    RegMMX RegMMXOperand1() const { return m_rOperand1.MMX(); }
    RegMMX RegMMXOperand2() const { return m_rOperand2.MMX(); }
    RegMMX RegMMXOperand3() const { return m_rOperand3.MMX(); }
#endif

    RegGPR RegGPRResult  () const { return m_rResult  .GPR() ; }
    RegGPR RegGPROperand1() const { return m_rOperand1.GPR() ; }
    RegGPR RegGPROperand2() const { return m_rOperand2.GPR() ; }
    RegGPR RegGPROperand3() const { return m_rOperand3.GPR() ; }

public:
    // DependencyGraph data
    UINT32 m_uOrder;    // index in CProgram::m_prgOperators
    UINT32 m_uSpanIdx;
    COperator *m_pNextVarProvider;
    Link *m_pProviders;
    Link *m_pConsumers;
    Hook *m_pDependents;
    UINT32 m_uBlockersCount;
    UINT32 m_uChainSize;

    // Mapper data
    CShuffleRecord *m_pShuffles;

    CRegID m_rResult;
    CRegID m_rOperand1;
    CRegID m_rOperand2;
    CRegID m_rOperand3;

    // Coder data
    UINT32 m_uBinaryOffset; // offset of instruction location from binary code start point

private:
    static const UINT32 sc_opFlags[];
    static const UINT32 sc_opCodes[];
    static const UINT32 sc_movCodesRM[];
};

struct Link
{
    Link *m_pNextProvider;
    COperator *m_pProvider;

    Link *m_pNextConsumer;
    COperator *m_pConsumer;
};

struct Hook
{
    Hook *m_pNext;
    COperator *m_pOperator;
};


//============================ Instruction Graph ================================
struct InstructionHook
{
    InstructionHook *m_pNext;
    CInstruction *m_pProvider;
};

class CInstruction : public CFlushObject
{
public:
    CInstruction(COperator * pOperator) : m_pOperator(pOperator)
    {
        m_pProviders = NULL;
        m_uConsumersCount = 0;
    }

public:
    COperator * const m_pOperator;

    InstructionHook * m_pProviders;

    UINT32 m_uConsumersCount;
};

class CConnector : public CFlushObject
{
public:
    CConnector(UINT32 uVarID) : m_instruction(NULL), m_uVarID(uVarID) {}
public:
    CInstruction m_instruction;
    UINT32 const m_uVarID;
    CConnector * m_pNext;
};

