// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      implementation of CProgram flow control.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::Flow::Flow
//
//  Synopsis:
//      Construct flow holder object.
//
//------------------------------------------------------------------------
CProgram::Flow::Flow()
{
    m_prgOperators = NULL;
    m_uOperatorsCount = 0;
    m_uOperatorsAllocated = 0;
    m_fReversed = false;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::SplitFlow
//
//  Synopsis:
//      Split the flow of operators.
//
//      Flow control is an optional jitter capability that can be handy to
//      compose complicated programs. It is represented with three routines:
//          SplitFlow();
//          SetFlow(UINT32 uFlowID);
//          MergeFlow();
//
//      Flow control allows several (MAX_FLOWS, currently 3) fragments of tagret
//      program to be populated in parallel, using following pattern:
//
//          <do something - 1>  // these operators go to main flow
//      SplitFlow();
//          <do something - 2>  // these operators go to flow 0
//      SetFlow(1);
//          <do something - 3>  // these operators go to flow 1
//      SetFlow(0);
//          <do something - 4>  // these operators go to flow 0
//      SetFlow(2);
//          <do something - 5>  // these operators go to flow 2
//      MergeFlow();
//
//      After merging main flow is updated with operators accumulated
//      in flow 0, then flow 1, etc. Resulting sequense
//      in main flow is following:
//
//          <do something - 1>  // these operators remain in main flow
//          <do something - 2>  // these operators came from flow 0
//          <do something - 4>  // these operators came from flow 0
//          <do something - 3>  // these operators came from flow 1
//          <do something - 5>  // these operators came from flow 2
//
//-------------------------------------------------------------------------------
void
CProgram::SplitFlow()
{
    WarpAssert(!m_fFlowIsSplit);
    SwapFlow(m_flowMain, m_flowSplit[0]);
    m_fFlowIsSplit = true;
    m_uCurrentFlow = 0;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::SetFlow
//
//  Synopsis:
//      See comments to SplitFlow().
//
//------------------------------------------------------------------------
void
CProgram::SetFlow(__in_range(0, (MAX_FLOWS-1)) UINT32 uFlowID)
{
    WarpAssert(m_fFlowIsSplit);
    WarpAssert(uFlowID < MAX_FLOWS);
    if (uFlowID != m_uCurrentFlow)
    {
        SwapFlow(m_flowSplit[m_uCurrentFlow], m_flowSplit[uFlowID]);
        m_uCurrentFlow = uFlowID;
    }
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::ReverseFlow
//
//  Synopsis:
//      Reverse the sequence of operators in the flow.
//
//  This member can be useful to code conditional branches in complicated programs.
//  Suppose we have a loop and inside loop body might appear conditions to skip
//  remaining part of body. This can be represented like following:
//      void GenerateNode(Operation Op)
//      {
//          switch(Op)
//          {
//          case OpEarlyOut:
//              {
//                  CJitterAccess::SetFlow(i);
//                  C_UINT32 notAllPixelsOccluded = ...
//                  C_BranchIfZero branch(AllPixelsOccluded);
//
//                  CJitterAccess::SetFlow(j);
//                  branch.BranchHere();
//              }
//              return;
//          ...
//   Flow #j is assumed to preceed loop epilogue codes. It will accumulate branch merge
//   operators. The caveat here is that when loop contains several OpEarlyOut operations
//   then code spans of several branches will overlap. To fix this, flow #j should be
//   reversed. THis can be done by calling ReverseFlow(j) right before merging flows
//
//------------------------------------------------------------------------
void
CProgram::ReverseFlow(__in_range(0, (MAX_FLOWS-1)) UINT32 uFlowID)
{
    WarpAssert(m_fFlowIsSplit);
    WarpAssert(uFlowID < MAX_FLOWS);
    m_flowSplit[uFlowID].m_fReversed = true;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::MergeFlow
//
//  Synopsis:
//      See comments to SplitFlow().
//
//------------------------------------------------------------------------
void
CProgram::MergeFlow()
{
    HRESULT hr = S_OK;
    WarpAssert(m_fFlowIsSplit);

    SwapFlow(m_flowSplit[m_uCurrentFlow], m_flowMain);

    m_fFlowIsSplit = false;
    m_uCurrentFlow = 0;

    UINT32 uTotalOperators = m_flowMain.m_uOperatorsCount;
    for (UINT32 i = 0; i < MAX_FLOWS; i++)
    {
        uTotalOperators += m_flowSplit[i].m_uOperatorsCount;
    }

    if (uTotalOperators > m_uOperatorsAllocated)
    {
        IFC(GrowOperators(uTotalOperators - m_uOperatorsAllocated + 100));
    }

    for (UINT32 i = 0; i < MAX_FLOWS; i++)
    {
        AppendFlow(m_flowSplit[i]);
    }

Cleanup:
    return;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::SwapFlow
//
//  Synopsis:
//      Helper for flow control routines: save current operator flow
//      variables in current flow holder and restore them from new one.
//
//------------------------------------------------------------------------
void
CProgram::SwapFlow(CProgram::Flow &currentFlow, CProgram::Flow &newFlow)
{
    currentFlow.m_prgOperators        = m_prgOperators       ;
    currentFlow.m_uOperatorsCount     = m_uOperatorsCount    ;
    currentFlow.m_uOperatorsAllocated = m_uOperatorsAllocated;

    m_prgOperators        = newFlow.m_prgOperators       ;
    m_uOperatorsCount     = newFlow.m_uOperatorsCount    ;
    m_uOperatorsAllocated = newFlow.m_uOperatorsAllocated;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::AppendFlow
//
//  Synopsis:
//      Helper for flow control routines: append given flow to main one.
//
//------------------------------------------------------------------------
void
CProgram::AppendFlow(CProgram::Flow &flow)
{
    COperator **ppOperators = flow.m_prgOperators;
    int iStep = 1;
    if (flow.m_fReversed)
    {
        iStep = -1;
        ppOperators += (flow.m_uOperatorsCount - 1);
        flow.m_fReversed = false;
    }

    for (UINT32 i = 0; i < flow.m_uOperatorsCount; i++, ppOperators += iStep)
    {
        COperator *pOperator = *ppOperators;

        pOperator->m_uOrder = m_uOperatorsCount;
        m_prgOperators[m_uOperatorsCount++] = pOperator;
    }

    flow.m_uOperatorsCount = 0;
}


