// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to unsigned
//      integer 32-bit variable.
//

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32::operator+
//
//  Synopsis:
//      Add scaled offset to the pointer to UINT32 value.
//
//  Usage example:
//      P_u32 pa = ...;
//      C_u32  n = ...;
//      P_u32 pc = pa + b;
//
//      Result: pc = pa + n; pa and n unchanged.
//      Note that "n" is implicitly multiplied by sizeof(UINT32).
//
//------------------------------------------------------------------------------
P_u32
P_u32::operator+(C_u32 const &nIndexDelta) const
{
    P_u32 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(otPtrPlus, tmp.GetID(), m_ID, nIndexDelta.GetID());
    pOperator->m_refType = RefType_Index_4;
    pOperator->m_uDisplacement = 0;

    return tmp;
}

P_u32&
P_u32::operator+=(C_u32 const &nIndexDelta)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(otPtrPlus, GetID(), GetID(), nIndexDelta.GetID());
    pOperator->m_refType = RefType_Index_4;
    pOperator->m_uDisplacement = 0;

    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32::operator+=
//
//  Synopsis:
//      Add scaled constant offset to the pointer to UINT32 value.
//
//  Usage example:
//      P_u32 pa = ...;
//      pa += 3;
//
//      Result: pa increased by 3.
//      Note that given constant is implicitly multiplied by sizeof(UINT32).
//
//------------------------------------------------------------------------------
P_u32 &
P_u32::operator+=(int nIndexDelta)
{
    if (nIndexDelta)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otPtrOffset, m_ID, m_ID);
        pOperator->m_uDisplacement = (UINT_PTR)(nIndexDelta * sizeof(UINT32));
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32::operator++ (preincrement)
//
//  Synopsis:
//      Increment value by 1 (scaled by sizeof(UINT32))
//      
//  Usage example:
//      P_u32 pa = ...;
//      P_u32 pb = ++pa;
//
//      Result: pa = pb = incremented value of pa. 
//
//------------------------------------------------------------------------------
P_u32&
P_u32::operator++()
{
    return *this += 1;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32::operator++ (postincrement)
//
//  Synopsis:
//      Increment value by 1 (scaled by sizeof(UINT32))
//      
//  Usage example:
//      P_u32 pa = ...;
//      P_u32 pb = pa++;
//
//      Result: pa = incremented value of pa;
//              pb = value of pa before incrementing.
//
//------------------------------------------------------------------------------
P_u32
P_u32::operator++(int)
{
    P_u32 tmp = *this;
    ++*this;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32::operator[]
//
//  Synopsis:
//      Creates intermediate reference type to access the element of the array.
//
//------------------------------------------------------------------------------
R_u32
P_u32::operator[](C_u32 const& index) const
{
    return R_u32(RefType_Index_4, this->GetID(), index.GetID(), 0);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      R_u32::R_u32
//
//  Synopsis:
//      Construct all-const instance of R_u32.
//      This instance is required to resolve expessions
//      like p[n] where p is P_u32.
//      When such expression is handled by C++, it's yet unknown whether
//      it'll be used for writing or reading. To bypass this trouble,
//      we create intermediate reference type R_u32 which
//      in turn has "operator C_u32()" for reading and
//      "operator=(C_u32 const& origin)" for writing.
//
//------------------------------------------------------------------------------
R_u32::R_u32(
    RefType refType,
    UINT32 uBaseVarID,
    UINT32 uIndexVarID,
    UINT_PTR uDisplacement
    )
    : m_refType(refType)
    , m_uBaseVarID(uBaseVarID)
    , m_uIndexVarID(uIndexVarID)
    , m_uDisplacement(uDisplacement)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32::operator[]
//
//  Synopsis:
//      Creates intermediate reference type to access in-memory variable.
//
//  Usage examples:
//      1.
//      P_u32  ptr = ...;
//      UINT32 idx = ...;
//      C_u32  f = ptr[idx];
//
//      Result: f = ptr[idx]; ptr, idx and ptr[idx] unchanged.
//
//      2.
//      P_u32  ptr = ...;
//      UINT32 idx = ...;
//      C_u32  f = ...;
//      ptr[idx] = f;
//
//      Result: ptr[idx] = f; f, ptr and idx unchanged.
//
//------------------------------------------------------------------------------
R_u32
P_u32::operator[](int nIndex) const
{
    return R_u32(RefType_Base, this->GetID(), 0, nIndex*sizeof(UINT32));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      R_u32::operator C_u32
//
//  Synopsis:
//      Cast reference type R_u32 to data type C_u32.
//      Treated as fetching data from array.
//
//  Usage example:
//      P_u32  ptr = ...;
//      UINT32 idx = ...;
//      C_u32  f = ptr[idx];
//
//------------------------------------------------------------------------------
R_u32::operator C_u32() const
{
    C_u32 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    UINT32 uOperand1 = m_uBaseVarID;
    UINT32 uOperand2 = m_uIndexVarID;
    if (uOperand1 == 0)
    {
        uOperand1 = uOperand2;
        uOperand2 = 0;
    }

    SOperator * pOperator = pProgram->AddOperator(otUINT32Load, tmp.GetID(), uOperand1, uOperand2);
    pOperator->m_refType = m_refType;
    pOperator->m_uDisplacement = m_uDisplacement;

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      R_u32::operator=
//
//  Synopsis:
//      Stores data in an element of array.
//
//  Usage example:
//      P_u32  ptr = ...;
//      UINT32 idx = ...;
//      C_u32  f = ...;
//      ptr[idx] = f;
//------------------------------------------------------------------------------
C_u32 const&
R_u32::operator=(C_u32 const& origin)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    if (m_uBaseVarID == 0 && m_uIndexVarID == 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otUINT32Store, 0, origin.GetID());
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }
    else if (m_uBaseVarID != 0 && m_uIndexVarID == 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otUINT32Store, 0, origin.GetID(), m_uBaseVarID);
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }
    else if (m_uBaseVarID == 0 && m_uIndexVarID != 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otUINT32Store, 0, origin.GetID(), m_uIndexVarID);
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }
    else //if (m_uBaseVarID != 0 && m_uIndexVarID != 0)
    {
        P_u32 tmp;

        SOperator * pOperator1 = pProgram->AddOperator(otPtrPlus, tmp.GetID(), m_uBaseVarID, m_uIndexVarID);
        pOperator1->m_refType = m_refType;
        pOperator1->m_uDisplacement = 0;

        SOperator * pOperator2 = pProgram->AddOperator(otUINT32Store, 0, origin.GetID(), tmp.GetID());
        pOperator2->m_refType = RefType_Base;
        pOperator2->m_uDisplacement = m_uDisplacement;
    }

    return origin;
}


