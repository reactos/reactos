// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions of class CBubbler.
//
#pragma once

//+------------------------------------------------------------------------------
//
//  Class:
//      CBubbler
//
//  Synopsis:
//      Move CShuffleRecord toward span start in order to get move
//      instructions executed as early as possible.
//
//-------------------------------------------------------------------------------
class CBubbler
{
public:
    CBubbler(CProgram * pProgram);

    void BubbleMoves();

private:
    void BubbleRecord(CShuffleRecord * psr, COperator * pOp);
    CShuffleRecord ** FindPrecedingRecord(CShuffleRecord *psr, COperator * pOp);

    bool CanExchange(CShuffleRecord *pPrev, CShuffleRecord *pNext);

    bool CanExchange(CRegID prevSrc, CRegID prevDst, CRegID nextSrc, CRegID nextDst);
    bool CanExchange(CRegID prevDst, CRegID nextSrc);
    bool CanExchange(CRegID prevSrc, CRegID prevDst, UINT32 nextSrc, CRegID nextDst);
    bool CanExchange();
    bool CanExchange(CRegID prevSrc, UINT32 prevDst, UINT32 nextSrc, CRegID nextDst);
    bool CanExchange(UINT32 prevSrc, CRegID prevDst, CRegID nextSrc, CRegID nextDst);
    bool CanExchange(UINT32 prevSrc, CRegID prevDst, CRegID nextSrc, UINT32 nextDst);
    bool CanExchange(UINT32 prevSrc, CRegID prevDst, UINT32 nextSrc, CRegID nextDst);

    bool CanExchangeWithInstruction(COperator * pOperator, CShuffleRecord * psr);
    bool CanExchangeWithInstruction(COperator * pOperator, CRegID nextSrc, CRegID nextDst);
    bool CanExchangeWithInstruction(COperator * pOperator, CRegID nextSrc, UINT32 nextDst);
    bool CanExchangeWithInstruction(COperator * pOperator, UINT32 nextSrc, CRegID nextDst);

#if DBG
    void AssertValid(COperator* pOp);
#else
    void AssertValid(COperator*) {}
#endif

private:
    CProgram * const m_pProgram;

    COperator **m_ppOperators;
    UINT32 m_uOperatorsCount;
};

