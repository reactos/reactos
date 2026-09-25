// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of Just-In-Time code generator exposed routines.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

extern WarpPlatform::LockHandle g_LockJitterAccess;

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::Enter
//
//  Synopsis:
//      Start code generation session.
//
//  Arguments:
//      usCallParametersSize:
//          size, in bytes, of stack bytes to be released on return
//          from generated code.
//
//  Details:
//      This routine creates an instance of class CProgram
//      and conveys it to CJitterSupport. This instance then is used
//      to accumulate the description of the algorithm of desired
//      code.
//
//-------------------------------------------------------------------------------
__checkReturn HRESULT
CJitterAccess::Enter(UINT16 usCallParametersSize)
{
    HRESULT hr = S_OK;
    CProgram * pProgram = NULL;

    WarpPlatform::AcquireLock(g_LockJitterAccess);

    IFC(CProgram::Create(usCallParametersSize, &pProgram));

    WarpPlatform::BeginCompile(pProgram);
    pProgram = NULL;

Cleanup:
    if (pProgram)
    {
        pProgram->Destroy();
    }
    return hr;
}


//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::Leave
//
//  Synopsis:
//      Finish code generation session.
//
//-------------------------------------------------------------------------------
void
CJitterAccess::Leave()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpPlatform::EndCompile();

    // Allow emergency call, when pProgram can be NULL.
    if (pProgram)
    {
        pProgram->Destroy();
    }

    WarpPlatform::ReleaseLock(g_LockJitterAccess);
}


//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::AllocFlushMemory
//
//  Synopsis:
//      Allocate contiguous block in flush memory associated with
//      current CProgram.
//
//-------------------------------------------------------------------------------
UINT8*
CJitterAccess::AllocFlushMemory(UINT32 cbSize)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->AllocFlushMemory(cbSize);
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::Compile
//
//  Synopsis:
//      Generate binary code to implement an algorithm accumulated in
//      current program.
//
//-------------------------------------------------------------------------------
__checkReturn HRESULT
CJitterAccess::Compile(__deref_out UINT8 **ppBinaryCode)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->Compile(ppBinaryCode);
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::GetCodeSize
//
//  Synopsis:
//      Return the size of code generated on recent CJitterAccess::Compile() call.
//
//-------------------------------------------------------------------------------
UINT32
CJitterAccess::GetCodeSize()
{
    const CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->GetCodeSize();
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::CodeFree
//
//  Synopsis:
//      Free memory block obtained by CJitterAccess::Compile.
//
//-------------------------------------------------------------------------------
void
CJitterAccess::CodeFree(__in void *pBinaryCode)
{
    CJitterSupport::CodeFree(pBinaryCode);
}


//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::SplitFlow
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
CJitterAccess::SplitFlow()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->SplitFlow();
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::SetFlow
//
//  Synopsis:
//      See comments to SplitFlow().
//
//-------------------------------------------------------------------------------
void
CJitterAccess::SetFlow(UINT32 uFlowID)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->SetFlow(uFlowID);
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::ReverseFlow
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
//-------------------------------------------------------------------------------
void
CJitterAccess::ReverseFlow(UINT32 uFlowID)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->ReverseFlow(uFlowID);
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::MergeFlow
//
//  Synopsis:
//      See comments to SplitFlow().
//
//-------------------------------------------------------------------------------
void
CJitterAccess::MergeFlow()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->MergeFlow();
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::SetClientData
//
//  Synopsis:
//      Store void* value supplied by jitter client.
//      This value does not affect jitter.
//
//-------------------------------------------------------------------------------
void
CJitterAccess::SetClientData(void * pClientData)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    pProgram->SetClientData(pClientData);
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::GetClientData
//
//  Synopsis:
//      Fetch void* value recently supplied via SetClientData.
//
//-------------------------------------------------------------------------------
void *
CJitterAccess::GetClientData()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->GetClientData();
}

#if DBG_DUMP
//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::SetDumpFile
//
//  Synopsis:
//      Sets the handle of debug dump; enables dump if nonzero.
//
//-------------------------------------------------------------------------------
void
CJitterAccess::SetDumpFile(WarpPlatform::FileHandle hDumpFile)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->SetDumpFile(hDumpFile);
}
#endif //DBG_DUMP

//+------------------------------------------------------------------------------
//
//  Member:
//      CJitterAccess::SetMode
//
//  Synopsis:
//      Setup operation mode parameter.
//
//-------------------------------------------------------------------------------
void
CJitterAccess::SetMode(UINT32 uParameterIdx, INT32 nParameterValue)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    pProgram->SetMode(uParameterIdx, nParameterValue);
}

//+------------------------------------------------------------------------------
//
//  Member:
//      C_Variable::IsInitialized
//
//  Synopsis:
//      See whether there was submitted any operator that sets the value
//      for this variable.
//
//-------------------------------------------------------------------------------
bool
C_Variable::IsInitialized() const
{
    const CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    return pProgram->VarIsInitialized(GetID());
}

