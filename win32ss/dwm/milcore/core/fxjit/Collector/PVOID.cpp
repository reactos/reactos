// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent generic pointer variable.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_pVoid::C_pVoid
//
//  Synopsis:
//      Default constructor: just allocate variable ID of vtPointer type.
//
//------------------------------------------------------------------------------
C_pVoid::C_pVoid()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtPointer);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_pVoid::C_pVoid
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_pVoid x = <expression>;
//
//------------------------------------------------------------------------------
C_pVoid::C_pVoid(C_pVoid const & origin)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtPointer);
    pProgram->AddOperator(otPtrAssign, m_ID, origin.m_ID);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_pVoid::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_pVoid variable declared before.
//
//------------------------------------------------------------------------------
C_pVoid &
C_pVoid::operator=(C_pVoid const & origin)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otPtrAssign, m_ID, origin.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_pVoid::C_pVoid
//
//  Synopsis:
//      Initialize C_pVoid variable by given void*.
//
//------------------------------------------------------------------------------
C_pVoid::C_pVoid(void const * pOrigin)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtPointer);
    SOperator *pOperator = pProgram->AddOperator(otPtrAssignImm, m_ID);
    pOperator->m_uDisplacement = (UINT_PTR)pOrigin;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static C_pVoid::GetpVoidArgument
//
//  Synopsis:
//      Construct pointer by copying call argument from stack frame.
//
//  Usage example:
//      Consider function:
//
//          static void __stdcall Foo(void * p1Given, void * p2Given)
//          {
//              void * p1 = p1Given;
//              void * p2 = p2Given;
//              ...
//
//      To generate binary code for Foo, we need following prototype:
//
//          void FooProto()
//          {
//              C_pVoid p1 = C_pVoid::GetpVoidArgument(0);
//              C_pVoid p2 = C_pVoid::GetpVoidArgument(sizeof(void*));
//              ...
//
//------------------------------------------------------------------------------
C_pVoid
C_pVoid::GetpVoidArgument(INT32 nDisplacement)
{
    C_pVoid tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
#ifdef _AMD64_
    if (nDisplacement == 0)
    {
        pProgram->AddOperator(otPtrAssign, tmp.GetID(), pProgram->GetArgument1ID());
    }
    else
#endif
    {
        SOperator *pOperator = pProgram->AddOperator(otPtrAssignArgument, tmp.GetID(), pProgram->GetFramePointerID());
        pOperator->m_uDisplacement = (UINT_PTR)nDisplacement;
    }

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static C_pVoid::GetUINT32Argument
//
//  Synopsis:
//      Construct 32-bit integer by copying call argument from stack frame.
//
//  Usage example:
//      Consider function:
//
//          static void __stdcall Foo(void * p1Given, UINT uArg)
//          {
//              void * p1 = p1Given;
//              UINT32 u2 = uArg;
//              ...
//
//      To generate binary code for Foo, we need following prototype:
//
//          void FooProto()
//          {
//              C_pVoid p1 = C_pVoid::GetpVoidArgument(0);
//              C_u32 u2 = C_pVoid::GetUINT32Argument(sizeof(void*));
//              ...
//
//------------------------------------------------------------------------------
C_u32
C_pVoid::GetUINT32Argument(INT32 nDisplacement)
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otUINT32Load, tmp.GetID(), pProgram->GetFramePointerID());
    pOperator->m_refType = RefType_Base;
    pOperator->m_uDisplacement = (UINT_PTR)(nDisplacement + CAssembleContext::sc_uArgOffset);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_pVoid::GetMemberPtr
//
//  Synopsis:
//      Fetch pointer from a structure.
//
//  Usage example:
//
//      C_pVoid p;  // assuming that "p" points to some
//                  // structure, like following:
//                  //
//                  // struct MyStruct
//                  // {
//                  //      UINT uMyFoo;
//                  //      void *pMyMoo;
//                  // }
//      P_u32 pMoo = p.GetMemberPtr( offsetof(MyStruct, pMyMoo) );
//
//------------------------------------------------------------------------------
C_pVoid
C_pVoid::GetMemberPtr(INT32 nDisplacement) const
{
    C_pVoid tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otPtrAssignMember, tmp.GetID(), GetID());
    pOperator->m_uDisplacement = (UINT_PTR)nDisplacement;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_pVoid::GetMemberPtrIndexed
//
//  Synopsis:
//      Fetch pointer from a structure.
//
//  Usage example:
//
//      C_pVoid p;  // assuming that "p" points to some
//                  // structure, like following:
//                  //
//                  // struct MyStruct
//                  // {
//                  //      UINT uMyFoo;
//                  //      void *pMyMoo[N];
//                  // }
//      C_u32 i;
//      P_u32 pMoo = p.GetMemberPtrIndexed( offsetof(MyStruct, pMyMoo), i);
//
//------------------------------------------------------------------------------
C_pVoid
C_pVoid::GetMemberPtrIndexed(INT32 nDisplacement, C_u32 const & index) const
{
    C_pVoid tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otPtrAssignMemberIndexed, tmp.GetID(), GetID(), index.GetID());
    pOperator->m_uDisplacement = (UINT_PTR)nDisplacement;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_pVoid::GetMemberUINT32
//
//  Synopsis:
//      Fetch UINT32 from a structure.
//
//  Usage example:
//
//      C_pVoid p;  // assuming that "p" points to some
//                  // structure, like following:
//                  //
//                  // struct MyStruct
//                  // {
//                  //      UINT uMyFoo;
//                  //      void *pMyMoo;
//                  // }
//      C_u32 uFoo = p.GetMemberUINT32( offsetof(MyStruct, uMyMoo) );
//
//------------------------------------------------------------------------------
C_u32
C_pVoid::GetMemberUINT32(INT32 nDisplacement) const
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otUINT32Load, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Base;
    pOperator->m_uDisplacement = (UINT_PTR)nDisplacement;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_pVoid::GetMemberXmm
//
//  Synopsis:
//      Fetch XMM value from a structure.
//
//  Usage example:
//
//      C_pVoid p;  // assuming that "p" points to some
//                  // structure, like following:
//                  //
//                  // struct MyStruct
//                  // {
//                  //      ...
//                  //      __m128 data;
//                  // }
//      C_XmmValue pMoo = p.GetMemberXmm( offsetof(MyStruct, uMyMoo) );
//
//------------------------------------------------------------------------------
C_XmmValue
C_pVoid::GetMemberXmm(INT32 nDisplacement) const
{
    C_XmmValue tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmAssignMember, tmp.GetID(), GetID());
    pOperator->m_uDisplacement = (UINT_PTR)nDisplacement;
    return tmp;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      C_pVoid::GetMemberXmm
//
//  Synopsis:
//      Fetch XMM value from a structure.
//
//  Usage example:
//
//      C_pVoid p;  // assuming that "p" points to some
//                  // structure, like following:
//                  //
//                  // struct MyStruct
//                  // {
//                  //      ...
//                  //      __m128 data;
//                  // }
//      C_XmmValue pMoo = p.GetMemberXmm( offsetof(MyStruct, uMyMoo) );
//
//------------------------------------------------------------------------------
C_f32x1
C_pVoid::GetMemberFloat1(INT32 nDisplacement) const
{
    C_f32x1 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmFloat1Load, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Base;
    pOperator->m_uDisplacement = (UINT_PTR)nDisplacement;
    return tmp;
}

//+-----------------------------------------------------------------------------
//  Helper for template class TIndexer: add constant byte offset to the pointer value.
//
void
C_pVoid::ConstOffset(int nDelta)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otPtrCompute, m_ID, m_ID);
    pOperator->m_refType = RefType_Base;
    pOperator->m_uDisplacement = (UINT_PTR)(nDelta);
}

//+-----------------------------------------------------------------------------
//  Helper for template class TIndexer: add variable scaled offset to the pointer value.
//
void
C_pVoid::ScaledOffset(C_u32 const& IndexDelta, RefType IndexScale)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    SOperator *pOperator = pProgram->AddOperator(otPtrCompute, GetID(), GetID(), IndexDelta.GetID());
    pOperator->m_refType = IndexScale;
    pOperator->m_uDisplacement = 0;
}


void
C_pVoid::AddOperator(
    OpType ot,
    UINT32 vResult,
    UINT32 vOperand1,
    UINT32 vOperand2,
    UINT32 vOperand3,
    RefType refType,
    UINT_PTR uDisplacement
    )
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(ot, vResult, vOperand1, vOperand2, vOperand3);
    pOperator->m_refType = refType;
    pOperator->m_uDisplacement = uDisplacement;
}

UINT32
C_pVoid::ScaleIdx(UINT32 uIndexVarID, UINT32 uShift)
{
    C_u32 ScaledIdx;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otUINT32ImmShiftLeft, ScaledIdx.GetID(), uIndexVarID);
    pOperator->m_shift = uShift;
    return ScaledIdx.GetID();
}

