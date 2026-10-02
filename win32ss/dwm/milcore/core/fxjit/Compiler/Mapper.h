// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions of class CMapper.
//
//-----------------------------------------------------------------------------
#pragma once

class CProgram;
class COperator;
class CMapContext;
class CRegID;
struct ExtRegState;
struct OpSpan;

//+------------------------------------------------------------------------------
//
//  Class:
//      CMapper
//
//  Synopsis:
//      Operates on an abstract program contained in an instance of CProgram class.
//      For each operator in the program, mapper associates
//      operands and resulting value with particular IA-32 registers.
//
//      In constrained conditions, when too many variables are being involved
//      so that there are not enought registers to fit all of them, the mapper
//      schedules sub-operations to store variables in stack frame memory.
//
//  Usage pattern:
//
//      // Create the instance
//          CProgram * pProgram = ...
//          CMapper mapper(pProgram);
//
//      // Do mapping:
//          mapper.MapProgram();
//
//      // Get mapping results:
//          // size of stack frame, in bytes:
//          UINT32 uFrameSize = mapper.GetFrameSize()
//
//          // offset in stack frame where variable resides, in bytes
//          UINT32 uVarOffset = GetVarOffset(UINT32 uVarIndex);
//
//          pProgram->GetOperator(...)->m_rResult;
//          pProgram->GetOperator(...)->m_rOperand1;
//          pProgram->GetOperator(...)->m_rOperand2;
//          pProgram->GetOperator(...)->m_rOperand3;
//
//-------------------------------------------------------------------------------
class CMapper
{
public:
    CMapper(CProgram * pProgram);

    __checkReturn HRESULT MapProgram();
    UINT32 GetFrameSize() const { return m_uFrameSize; }
    UINT32 GetFrameAlignment() const{ return m_uFrameAlignment; }
    UINT32 GetVarOffset(UINT32 uVarIndex) const;

private:
    struct RegisterGroup
    {
        UINT32 m_uCount;
        UINT32 m_uRotation;
        CRegID const *m_prgRegs;

        UINT32 Next(UINT32 u) const {return ++u == m_uCount ? 0 : u;}
        void Init(CRegID const *prgRegs, UINT32 uCount)
        {
            m_prgRegs = prgRegs;
            m_uCount = uCount;
            m_uRotation = 0;
        }
    };

    RegisterGroup* GetRegisterGroup(RegisterType rt)
    {
        return rt == rtGPR ? &m_RegisterGroupGPR :
#if WPFGFX_FXJIT_X86
               rt == rtMMX ? &m_RegisterGroupMMX :
#endif
                             &m_RegisterGroupXMM;
    }

    __checkReturn HRESULT MapLoop();
    __checkReturn HRESULT MapBranch();
    __checkReturn HRESULT MapSubroutineCall();
    __checkReturn HRESULT MapSubroutineStart();
    __checkReturn HRESULT MapSubroutineReturn();

    __checkReturn HRESULT MapOperator();
    __checkReturn HRESULT MapCall();
    __checkReturn HRESULT FreeRegister(CRegID regID);
    __checkReturn HRESULT PreAllocRegister(CRegID regID, UINT32 uVarID);
    __checkReturn HRESULT SaveReg(COperator * pOp, UINT32 uVarID, CRegID regSrc);
    __checkReturn HRESULT LoadReg(COperator * pOp, CRegID regDst, UINT32 uVarID);
    __checkReturn HRESULT MoveReg(COperator * pOp, CRegID regDst, CRegID regSrc, VariableType vt);
    void HookShuffleRecord(COperator * pOp, CShuffleRecord * psr);

    CRegID AllocRegister(RegisterGroup * prg);
    void ClearAllocExceptions() { m_uAllocExceptionCount = 0; }
    void SetAllocException(CRegID regID);
    bool IsAllocException(CRegID regID) const;
    __checkReturn HRESULT AcquireRegister(RegisterGroup * prg, CRegID &regID, UINT32 uVarToLoad);

    void AllocateStackFrame();

    __checkReturn HRESULT FreeRegs();
    __checkReturn HRESULT SaveRegs(const CBitArray * pVars);
    __checkReturn HRESULT EqualizeLoopRegState(ExtRegState * const pDesiredRegState);
    __checkReturn HRESULT EqualizeBranchRegState(ExtRegState * const pAltRegState, const CBitArray * pVarsInUse);

private:
    UINT32 const m_uVarCount;
    UINT32 *m_prgOffsets; // [m_uVarCount]

    CProgram * const m_pProgram;

    COperator **m_ppOperators;
    CInstruction **m_ppInstructions;
    UINT32 m_uOperatorsCount;

    CLocator m_locator;

    UINT32 m_uOpIdx;  // current operator index
    COperator * m_pOp; // current operator

    CMapContext * m_pMapContext; // current register mapping context

    UINT32 m_uFrameSize;
    UINT32 m_uFrameAlignment;

    RegisterGroup m_RegisterGroupGPR;
#if WPFGFX_FXJIT_X86
    RegisterGroup m_RegisterGroupMMX;
#endif
    RegisterGroup m_RegisterGroupXMM;

    static const int sc_uMaxAllocExceptionCount = 6;
    CRegID m_regAllocExceptions[sc_uMaxAllocExceptionCount];
    UINT32 m_uAllocExceptionCount;

    // Variables that are used in the group of spans that constitute
    // the body of the loop that follows this span
    CBitArray * m_pVarsUsedInLoop;
    UINT32 m_uBitArraySize;
};


