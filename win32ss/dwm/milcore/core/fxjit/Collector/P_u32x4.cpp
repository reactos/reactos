// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to C_u32x4 variable.
//

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32x4::operator+=
//
//  Synopsis:
//      Add scaled constant offset to the pointer to u32x4 value.
//
//  Usage example:
//      P_u32x4 pa = ...;
//      pa += 3;
//
//      Result: pa increased by 3.
//      Note that given constant is implicitly multiplied by sizeof(u32x4).
//
//------------------------------------------------------------------------------
P_u32x4 &
P_u32x4::operator+=(int nIndexDelta)
{
    if (nIndexDelta)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otPtrOffset, m_ID, m_ID);
        pOperator->m_uDisplacement = (UINT_PTR)(nIndexDelta * sizeof(u32x4));
    }
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32x4::operator++ (preincrement)
//
//  Synopsis:
//      Increment value by 1 (scaled by sizeof(u32x4))
//      
//  Usage example:
//      P_u32x4 pa = ...;
//      P_u32x4 pb = ++pa;
//
//      Result: pa = pb = incremented value of pa. 
//
//------------------------------------------------------------------------------
P_u32x4&
P_u32x4::operator++()
{
    return *this += 1;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32x4::operator++ (postincrement)
//
//  Synopsis:
//      Increment value by 1 (scaled by sizeof(u32x4))
//      
//  Usage example:
//      P_u32x4 pa = ...;
//      P_u32x4 pb = pa++;
//
//      Result: pa = incremented value of pa;
//              pb = value of pa before incrementing.
//
//------------------------------------------------------------------------------
P_u32x4
P_u32x4::operator++(int)
{
    P_u32x4 tmp = *this;
    ++*this;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      R_u32x4::R_u32x4
//
//  Synopsis:
//      Construct all-const instance of R_u32x4.
//      This instance is required to resolve expessions
//      like p[n] where p is P_u32x4.
//      When such expression is handled by C++, it's yet unknown whether
//      it'll be used for writing or reading. To bypass this trouble,
//      we create intermediate reference type R_u32x4 which
//      in turn has "operator C_u32x4()" for reading and
//      "operator=(C_u32x4 const& origin)" for writing.
//
//------------------------------------------------------------------------------
R_u32x4::R_u32x4(
    RefType refType,
    UINT32 uVarID,
    UINT_PTR uDisplacement
    )
    : m_refType(refType)
    , m_uVarID(uVarID)
    , m_uDisplacement(uDisplacement)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u32x4::operator[]
//
//  Synopsis:
//      Creates intermediate reference type to access in-memory variable.
//
//  Usage examples:
//      1.
//      P_u32x4 ptr = ...;
//      UINT32  idx = ...;
//      C_u32x4 f = ptr[idx];
//
//      Result: f = ptr[idx]; ptr, idx and ptr[idx] unchanged.
//
//      2.
//      P_u32x4 ptr = ...;
//      UINT32  idx = ...;
//      C_u32x4 f = ...;
//      ptr[idx] = f;
//
//      Result: ptr[idx] = f; f, ptr and idx unchanged.
//
//------------------------------------------------------------------------------
R_u32x4
P_u32x4::operator[](int nIndex) const
{
    return R_u32x4(RefType_Base, GetID(), nIndex*sizeof(u32x4));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      R_u32x4::operator C_u32x4
//
//  Synopsis:
//      Cast reference type R_u32x4 to data type C_u32x4.
//      Treated as fetching data from array.
//
//  Usage example:
//      P_u32x4 ptr = ...;
//      UINT32  idx = ...;
//      C_u32x4 f = ptr[idx];
//
//------------------------------------------------------------------------------
R_u32x4::operator C_u32x4() const
{
    C_u32x4 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    SOperator * pOperator = pProgram->AddOperator(otXmmIntLoad, tmp.GetID(), m_uVarID);
    pOperator->m_refType = m_refType;
    pOperator->m_uDisplacement = m_uDisplacement;

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      R_u32x4::operator=
//
//  Synopsis:
//      Stores data in an element of array.
//
//  Usage example:
//      P_u32x4 ptr = ...;
//      UINT32  idx = ...;
//      C_u32x4 f = ...;
//      ptr[idx] = f;
//------------------------------------------------------------------------------
C_u32x4 const&
R_u32x4::operator=(C_u32x4 const& origin)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(otXmmIntStore, 0, origin.GetID(), m_uVarID);
    pOperator->m_refType = m_refType;
    pOperator->m_uDisplacement = m_uDisplacement;
    return origin;
}


