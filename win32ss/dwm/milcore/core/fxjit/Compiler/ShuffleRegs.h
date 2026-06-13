// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Permutation of variables across registers and memory.
//
//-----------------------------------------------------------------------------
#pragma once

class CMapper;

class CAssembleContext;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CShuffleRecord
//
//  Synopsis:
//      Holds one instruction to move one variable value
//      to/from memory/register.
//
//------------------------------------------------------------------------------
class CShuffleRecord : public CFlushObject
{
public:
    // construct record to move register to memory
    CShuffleRecord(UINT32 uVarID, CRegID regSrc, VariableType vt)
    {
        WarpAssert(uVarID);
        m_pNext = NULL;
        m_regSrc = regSrc;
        m_uVarID = uVarID;
        m_vt = vt;
    }

    // construct record to move memory to register
    CShuffleRecord(CRegID regDst, UINT32 uVarID, VariableType vt)
    {
        WarpAssert(uVarID);
        m_pNext = NULL;
        m_regDst = regDst;
        m_uVarID = uVarID;
        m_vt = vt;
    }

    // construct record to move register to register
    CShuffleRecord(CRegID regDst, CRegID regSrc, VariableType vt)
    {
        m_pNext = NULL;
        m_regDst = regDst;
        m_regSrc = regSrc;
        m_uVarID = 0;
        m_vt = vt;
    }

    void Assemble(CAssembleContext & actx, CMapper const & mapper);

    CRegID GetRegSrc() const { return m_regSrc; }
    CRegID GetRegDst() const { return m_regDst; }
    UINT32 GetVarID() const { return m_uVarID; }

private:
    CRegID m_regSrc;
    CRegID m_regDst;
    UINT32 m_uVarID;
    VariableType m_vt;

public:
    CShuffleRecord * m_pNext;
};


