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
#include "precomp.h"


//+------------------------------------------------------------------------------
//
//  Class:
//      CVarState
//
//  Synopsis:
//      Holds the state of a variable after executing particular operator.
//      Stored data include:
//          - whether the variable is in register;
//          - if so, in which register;
//          - whether the variable is in frame stack memory;
//          - whether the variable ever been in frame stack memory.
//
//      There might exist a technique when a variable can sometimes
//      appear copied in several registers. This is not used.
//      If the variable is in register then it is only in one register.
//
//      All the four combinations of InRegister and InMemory are valid.
//
//-------------------------------------------------------------------------------
class CVarState : public CFlushObject
{
public:
    CVarState() {Flush();}
    void Flush()
    {
        m_bData = m_maskReg;
    }
    void Clear()
    {
        m_bData &= m_maskWasInMem;
        m_bData |= m_maskReg;
    }
    void SetInRegister(CRegID reg)
    {
        WarpAssert(reg.IsDefined());
        m_bData = (UINT8)((m_bData & ~m_maskReg) | (reg.Index() & m_maskReg));
    }
    void SetOutOfRegister()
    {
        m_bData |= m_maskReg;
    }
    void SetInMemory()
    {
        m_bData |= m_maskIsInMem | m_maskWasInMem;
    }
    void SetOutOfMemory()
    {
        m_bData &= ~m_maskIsInMem;
    }

    CRegID GetRegister() const { return m_bData & m_maskReg; }
    bool IsInRegister() const { return GetRegister().IsDefined(); }
    bool IsInMemory() const { return (m_bData & m_maskIsInMem) != 0; }
    bool WasInMemory() const { return (m_bData & m_maskWasInMem) != 0; }


private:
    UINT8 m_bData;
    static const UINT8 m_maskReg = 0x3F;
    static const UINT8 m_maskIsInMem = 0x40;
    static const UINT8 m_maskWasInMem = 0x80;
};

//+------------------------------------------------------------------------------
//
//  Class:
//      CLocatorState
//
//  Synopsis:
//      Holds the state of an instance of CLocator.
//
//-------------------------------------------------------------------------------
class CLocatorState : public CFlushObject
{
public:
    CLocatorState *m_pNextState;
    RegState m_regState;
    // Assumed array of variable state, CVarState[m_uVarCount], follows here.
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::Init
//
//  Synopsis:
//      Allocate memory for given amount of variables,
//      initialize all states as empty.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CLocator::Init(
    __in CProgram * pProgram
    )
{
    HRESULT hr = S_OK;

    // Should not be initilized twice
    WarpAssert(m_prgVarState == NULL && m_uVarCount == 0 && m_pProgram == NULL);

    m_pProgram = pProgram;

    m_uVarCount = pProgram->GetVarsCount();

    m_prgVarState = reinterpret_cast<CVarState*>(m_pProgram->AllocMem(m_uVarCount * sizeof(CVarState)));
    IFCOOM(m_prgVarState);

    for (UINT32 u = 0; u < m_uVarCount; u++)
    {
        CVarState *p = &m_prgVarState[u];
        new(p) CVarState;
    }

    for (int i = 0; i < g_uRegsTotal; i++)
    {
        m_regState.m_uVarID[i] = 0;
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::ConsiderSetValue
//
//  Synopsis:
//      Trace the effect of the instruction that changes
//      the value of the variable. New value is assumed
//      to appear in the register.
//
//------------------------------------------------------------------------------
void
CLocator::ConsiderSetValue(UINT32 uVarID, CRegID regID)
{
    WarpAssert(uVarID < m_uVarCount);
    WarpAssert(regID.IsDefined());

    // Check current state of the register.
    UINT32 uRecentVarID = m_regState.m_uVarID[regID.Index()];

    // If the register contained some (another) variable
    // then mark the state of this variable correspondingly.
    if (uRecentVarID != 0 && uRecentVarID != uVarID)
    {
        WarpAssert(uRecentVarID < m_uVarCount);
        CVarState & vsRecent = m_prgVarState[uRecentVarID];

        // The variable should be in memory already, otherwise
        // its value will be lost. If this variable has been
        // out of scope, the register should be marked empty
        // already so we'll not get this point.
        WarpAssert(vsRecent.IsInMemory());
        WarpAssert(vsRecent.GetRegister() == regID);

        vsRecent.SetOutOfRegister();
    }

    CVarState & vs = m_prgVarState[uVarID];
    if (vs.IsInRegister())
    {
        // Previous value in register becames invalid
        CRegID regID = vs.GetRegister();
        m_regState.m_uVarID[regID.Index()] = 0;
        vs.SetOutOfRegister();
    }

    // New value appeares in the register so old value in memory
    // is obsolete and we should mark that it is no longer in memory.
    vs.SetOutOfMemory();

    vs.SetInRegister(regID);
    m_regState.m_uVarID[regID.Index()] = uVarID;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::ConsiderLoadReg
//
//  Synopsis:
//      Trace the effect of the instruction that moves
//      the value of the variable from stack frame to register.
//
//------------------------------------------------------------------------------
void
CLocator::ConsiderLoadReg(UINT32 uVarID, CRegID regID)
{
    WarpAssert(uVarID < m_uVarCount);
    WarpAssert(regID.IsDefined());

    CVarState & vs = m_prgVarState[uVarID];

    // Can't fetch if the var is not in memory.
    WarpAssert(vs.IsInMemory());

    // Should not be called if the variable is already in register.
    WarpAssert(!vs.IsInRegister());

    // Given register should be freed before fetching.
    WarpAssert(m_regState.m_uVarID[regID.Index()] == 0);

    m_regState.m_uVarID[regID.Index()] = uVarID;
    vs.SetInRegister(regID);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::ConsiderOutOfMemory
//
//  Synopsis:
//      Trace the effect of the instruction that moves
//      the value of the variable from register to stack frame.
//
//------------------------------------------------------------------------------
void
CLocator::ConsiderOutOfMemory(UINT32 uVarID)
{
    CVarState & vs = m_prgVarState[uVarID];

    WarpAssert(vs.IsInRegister());

    vs.SetOutOfMemory();
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::ConsiderSaveReg
//
//  Synopsis:
//      Trace the effect of the instruction that moves
//      the value of the variable from register to stack frame.
//
//------------------------------------------------------------------------------
void
CLocator::ConsiderSaveReg(CRegID regID)
{
    WarpAssert(regID.IsDefined());

    UINT32 uVarID = m_regState.m_uVarID[regID.Index()];

    // This routine should not be called if the register is empty.
    WarpAssert(uVarID);

    CVarState & vs = m_prgVarState[uVarID];

    WarpAssert(!vs.IsInMemory());
    WarpAssert(vs.IsInRegister());
    WarpAssert(vs.GetRegister() == regID);

    vs.SetInMemory();
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::ConsiderFreeReg
//
//  Synopsis:
//      Declare register free.
//
//------------------------------------------------------------------------------
void
CLocator::ConsiderFreeReg(CRegID regID)
{
    WarpAssert(regID.IsDefined());

    UINT32 uVarID = m_regState.m_uVarID[regID.Index()];

    // This routine should not be called if the register is empty.
    WarpAssert(uVarID);

    CVarState & vs = m_prgVarState[uVarID];

    WarpAssert(vs.IsInMemory());
    WarpAssert(vs.IsInRegister());
    WarpAssert(vs.GetRegister() == regID);

    vs.SetOutOfRegister();
    m_regState.m_uVarID[regID.Index()] = 0;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::ConsiderMoveRegToReg
//
//  Synopsis:
//      Trace the effect of the instruction that moved
//      the value from one register to another one.
//
//------------------------------------------------------------------------------
void
CLocator::ConsiderMoveRegToReg(CRegID regTo, CRegID regFrom)
{
    WarpAssert(regTo.IsDefined());
    WarpAssert(regFrom.IsDefined());

    UINT32 uVarTo = m_regState.m_uVarID[regTo.Index()];
    if (uVarTo != 0)
    {
        CVarState & vsTo = m_prgVarState[uVarTo];

        // The variable should be in memory already, otherwise
        // its value will be lost. If this variable has been
        // out of scope, the register should be marked empty
        // already so we'll not get this point.
        WarpAssert(vsTo.IsInMemory());

        WarpAssert(vsTo.IsInRegister());
        WarpAssert(vsTo.GetRegister() == regTo);

        vsTo.SetOutOfRegister();
        m_regState.m_uVarID[regTo.Index()] = 0;
    }

    UINT32 uVarFrom = m_regState.m_uVarID[regFrom.Index()];

    // Shouldn't move a garbage.
    WarpAssert(uVarFrom != 0);

    CVarState & vsFrom = m_prgVarState[uVarFrom];

    WarpAssert(vsFrom.IsInRegister());
    WarpAssert(vsFrom.GetRegister() == regFrom);

    vsFrom.SetInRegister(regTo);

    m_regState.m_uVarID[regTo.Index()] = uVarFrom;
    m_regState.m_uVarID[regFrom.Index()] = 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::ConsiderVarOutOfScope
//
//  Synopsis:
//      Mark the variable going out of scope.
//
//------------------------------------------------------------------------------
void
CLocator::ConsiderVarOutOfScope(UINT32 uVarID)
{
    WarpAssert(uVarID && uVarID < m_uVarCount);

    CVarState & vs = m_prgVarState[uVarID];

    if (vs.IsInRegister())
    {
        CRegID regID = vs.GetRegister();
        WarpAssert(m_regState.m_uVarID[regID.Index()] == uVarID);
        m_regState.m_uVarID[regID.Index()] = 0;
        vs.SetOutOfRegister();
    }

    vs.SetOutOfMemory();
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::ConsiderScope
//
//  Synopsis:
//      Mark variables going out of scope.
//
//------------------------------------------------------------------------------
void
CLocator::ConsiderScope(CBitArray const * pVarsInUse)
{
    for (UINT32 i = 0; i < g_uRegsTotal; i++)
    {
        UINT32 uVar = m_regState.m_uVarID[i];
        if (uVar)
        {
            if (!pVarsInUse->Get(uVar))
            {
                ConsiderVarOutOfScope(uVar);
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::Setup
//
//  Synopsis:
//      Re-initialize variable states.
//
//------------------------------------------------------------------------------
void
CLocator::Setup(CBitArray const * pVarsInUse)
{
    for (UINT32 uVarID = 1; uVarID < m_uVarCount; uVarID++)
    {
        CVarState & vs = m_prgVarState[uVarID];

        bool fIsInMemory = pVarsInUse->Get(uVarID);
        if (fIsInMemory)
            vs.SetInMemory();
        else
            vs.SetOutOfMemory();
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::GetVarID
//
//  Synopsis:
//      Return the index of variable currently contained in given register.
//      Return 0 if register is free.
//
//------------------------------------------------------------------------------
UINT32
CLocator::GetVarID(CRegID regID) const
{
    WarpAssert(regID.IsDefined());

    return m_regState.m_uVarID[regID.Index()];
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::GetRegID
//
//  Synopsis:
//      Return ID of the register that's currently containing given variable.
//
//------------------------------------------------------------------------------
CRegID
CLocator::GetRegID(UINT32 uVarID) const
{
    WarpAssert(uVarID < m_uVarCount);
    CVarState const & vs = m_prgVarState[uVarID];
    return vs.GetRegister();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::IsInMemory
//
//  Synopsis:
//      Check whether the variable is currently stored in memory.
//------------------------------------------------------------------------------
bool
CLocator::IsInMemory(UINT32 uVarID) const
{
    WarpAssert(uVarID < m_uVarCount);

    const CVarState & vs = m_prgVarState[uVarID];

    return vs.IsInMemory();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::IsInRegister
//
//  Synopsis:
//      Check whether the variable is currently stored in a register.
//------------------------------------------------------------------------------
bool
CLocator::IsInRegister(UINT32 uVarID) const
{
    WarpAssert(uVarID < m_uVarCount);

    const CVarState & vs = m_prgVarState[uVarID];

    return vs.IsInRegister();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::WasInMemory
//
//  Synopsis:
//      Check whether the variable ever been stored in memory.
//      Returning "true" means that we need to allocate
//      a slot in stack frame for this variable.
//------------------------------------------------------------------------------
bool
CLocator::WasInMemory(UINT32 uVarID) const
{
    WarpAssert(uVarID < m_uVarCount);

    const CVarState & vs = m_prgVarState[uVarID];

    return vs.WasInMemory();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLocator::SnapRegState
//
//  Synopsis:
//      Copy registers state to caller's buffer.
//
//------------------------------------------------------------------------------
void
CLocator::SnapRegState(ExtRegState * pRegState) const
{
    for (UINT32 i = 0; i < g_uRegsTotal; i++)
    {
        UINT32 vID = m_regState.m_uVarID[i];
        pRegState->m_uVarID[i] = vID;
        if (vID)
        {
            pRegState->m_rgfIsInMemory[i] = IsInMemory(vID);
        }
    }
}


