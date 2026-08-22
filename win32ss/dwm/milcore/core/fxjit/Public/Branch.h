// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Execution control classes for prototype routines.
//
//-----------------------------------------------------------------------------

#pragma once

struct SOperator;

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_Loop
//
//  Synopsis:
//      Serves to denote repeatable code snippet in a prototype program.
//
//  Usage pattern:
//      C_u32 myCount = ...
//      C_Loop myLoop;    // do while (uCount != 0)
//      {
//          ... // loop body
//      }
//      loop.CountDownAndRepeatIfNonZero(myCount);
//
//------------------------------------------------------------------------------
class C_Loop
{
public:
    C_Loop();
    void CountDownAndRepeatIfNonZero(C_u32 & count);
    void RepeatIfNonZero(C_u32 const & count);

private:
    SOperator *m_pStartOperator;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_Branch
//
//  Synopsis:
//      Serves to denote a code snippet in a prototype program
//      that can be conditionally bypassed.
//
//  Usage pattern:
//
//      C_u32 myData = ...
//      C_Branch myBranch;
//      myBranch.BranchOnZero(myData)
//      {
//          ... // codes to be executed when myData is not zero
//      }
//      myBranch.BranchHere();
//
//------------------------------------------------------------------------------
class C_Branch
{
public:
    C_Branch();
    void BranchOnZero(C_u32 & src);
    void BranchOnZeroMask(C_u32x4 const & mask);
    void BranchHere();

private:
    SOperator *m_pStartOperator;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_Control
//
//  Synopsis:
//      Contains routines to generate flow control instructions.
//
//------------------------------------------------------------------------------
class C_Control
{
public:
    static C_u32 Call(void * pCallee, C_pVoid argument);
};

class C_Subroutine
{
public:
    C_Subroutine();
    ~C_Subroutine();
    void Begin();
    void End();
    void Call(C_pVoid & pStack);
    void Return(C_pVoid & pStack);

private:
    SOperator *m_pStartOperator;
    SOperator *m_pReturnOperator; // only one return operator is allowed

    // Linked list (via SOperator::m_pLinkedOperator) of all the callers.
    SOperator *m_pCallers;
};


