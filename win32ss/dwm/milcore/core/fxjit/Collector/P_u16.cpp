// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to unsigned
//      integer 16-bit variable.
//

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u16::operator+
//
//  Synopsis:
//      Add scaled offset to the pointer to UINT16 value.
//
//  Usage example:
//      P_u16 pa = ...;
//      C_u32  n = ...;
//      P_u16 pc = pa + n;
//
//      Result: pc = pa + n; pa and n unchanged.
//      Note that "n" is implicitly multiplied by sizeof(UINT16).
//
//------------------------------------------------------------------------------
P_u16
P_u16::operator+(C_u32 const &nIndexDelta) const
{
    P_u16 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(otPtrPlus, tmp.GetID(), GetID(), nIndexDelta.GetID());
    pOperator->m_refType = RefType_Index_2;
    pOperator->m_uDisplacement = 0;

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u16::operator+=
//
//  Synopsis:
//      Add scaled offset to the pointer to UINT16 value.
//
//  Usage example:
//      P_u16 pa = ...;
//      C_u32  n = ...;
//      pa += n;
//
//      Result: pa = pa + n; n unchanged.
//      Note that "n" is implicitly multiplied by sizeof(UINT16).
//
//------------------------------------------------------------------------------
P_u16&
P_u16::operator+=(C_u32 const &nIndexDelta)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(otPtrPlus, GetID(), GetID(), nIndexDelta.GetID());
    pOperator->m_refType = RefType_Index_2;
    pOperator->m_uDisplacement = 0;

    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u16::operator+
//
//  Synopsis:
//      Add scaled constant offset to the pointer to UINT16 value.
//
//  Usage example:
//      P_u16 pa = ...;
//      P_u16 pb = pa + 3;
//
//      Result: pb = pa increased by 3.
//      Note that given constant is implicitly multiplied by sizeof(UINT16).
//
//------------------------------------------------------------------------------
P_u16
P_u16::operator+(int nIndexDelta) const
{
    P_u16 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (nIndexDelta)
    {
        SOperator *pOperator = pProgram->AddOperator(otPtrOffset, tmp.GetID(), GetID());
        pOperator->m_uDisplacement = (UINT_PTR)(nIndexDelta * sizeof(UINT16));
    }
    else
    {
        pProgram->AddOperator(otPtrAssign, tmp.GetID(), GetID());
    }

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u16::operator+=
//
//  Synopsis:
//      Add scaled constant offset to the pointer to UINT16 value.
//
//  Usage example:
//      P_u16 pa = ...;
//      pa += 3;
//
//      Result: pa increased by 3.
//      Note that given constant is implicitly multiplied by sizeof(UINT16).
//
//------------------------------------------------------------------------------
P_u16 &
P_u16::operator+=(int nIndexDelta)
{
    if (nIndexDelta)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otPtrOffset, GetID(), GetID());
        pOperator->m_uDisplacement = (UINT_PTR)(nIndexDelta * sizeof(UINT16));
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u16::operator++ (preincrement)
//
//  Synopsis:
//      Increment value by 1 (scaled by sizeof(UINT16))
//      
//  Usage example:
//      P_u16 pa = ...;
//      P_u16 pb = ++pa;
//
//      Result: pa = pb = incremented value of pa. 
//
//------------------------------------------------------------------------------
P_u16&
P_u16::operator++()
{
    return *this += 1;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u16::operator++ (postincrement)
//
//  Synopsis:
//      Increment value by 1 (scaled by sizeof(UINT16))
//      
//  Usage example:
//      P_u16 pa = ...;
//      P_u16 pb = pa++;
//
//      Result: pa = incremented value of pa;
//              pb = value of pa before incrementing.
//
//------------------------------------------------------------------------------
P_u16
P_u16::operator++(int)
{
    P_u16 tmp = *this;
    ++*this;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_u16::operator[]
//
//  Synopsis:
//      Fetch indexed value from an array pointed by this variable.
//
//  Usage example:
//      P_u16 pa = ...;
//      C_u32  index = ...;
//      C_u32  n = pa[index];
//
//      Result: n = pa[index]; pa and index unchanged.
//      Note that the value of index is implicitly multiplied by sizeof(UINT16).
//------------------------------------------------------------------------------
C_u32
P_u16::operator[](C_u32 const &index) const
{
    C_u32 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otPtrFetch16, tmp.GetID(), GetID(), index.GetID());
    return tmp;
}



