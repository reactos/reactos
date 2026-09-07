// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Classes to keep track of variable states.
//
//-----------------------------------------------------------------------------
#pragma once

class CVarState;
class CBitArray;

//+------------------------------------------------------------------------------
//
//  Struct:
//      RegState
//
//  Synopsis:
//      Holds the content of CPU register set.
//      The register is known to be free when corresponding
//      m_uVarID[i] contains zero. Otherwise, it refers the
//      variable currently contained in this register.
//-------------------------------------------------------------------------------
struct RegState
{
    UINT32 m_uVarID[g_uRegsTotal];
};

struct ExtRegState
{
    UINT32 m_uVarID[g_uRegsTotal];
    bool m_rgfIsInMemory[g_uRegsTotal];
};


//+------------------------------------------------------------------------------
//
//  Class:
//      CLocator
//
//  Synopsis:
//      Holds the states of a set of variables after executing particular operator.
//      Stored data include:
//          - whether the variable is in register;
//          - if so, in which register;
//          - whether the variable is in frame stack memory;
//          - whether the variable ever been in frame stack memory;
//          - which variable is currently contained in the register.
//-------------------------------------------------------------------------------
class CLocator
{
public:
    CLocator()
    {
        m_pProgram = NULL;
        m_prgVarState = NULL;
        m_uVarCount = 0;
    }

    __checkReturn HRESULT Init(__in CProgram * pProgram);

    // Controls
    void ConsiderSetValue(UINT32 uVarID, CRegID regID);
    void ConsiderLoadReg(UINT32 uVarID, CRegID regID);
    void ConsiderMoveRegToReg(CRegID regTo, CRegID regFrom);
    void ConsiderSaveReg(CRegID regID);
    void ConsiderOutOfMemory(UINT32 uVarID);
    void ConsiderFreeReg(CRegID regID);
    void ConsiderVarOutOfScope(UINT32 uVarID);
    void ConsiderScope(CBitArray const * pVarsInUse);

    void Setup(CBitArray const * pVarsInUse);

    // Accessors
    UINT32 GetVarID(CRegID reg) const;
    CRegID GetRegID(UINT32 uVarID) const;
    bool IsInMemory(UINT32 uVarID) const;
    bool IsInRegister(UINT32 uVarID) const;
    bool WasInMemory(UINT32 uVarID) const;
    void SnapRegState(ExtRegState * pRegState) const;

private:

    // m_pProgram serves as generic context.
    CProgram * m_pProgram;

    // Current variables state - i.e. where variables are currently kept.
    CVarState *m_prgVarState; // [m_uVarCount]
    UINT32 m_uVarCount;

    // Current registers state - i.e. what is currently contained in them.
    RegState m_regState;
};

