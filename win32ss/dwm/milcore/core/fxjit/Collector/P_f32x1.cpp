// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to C_f32x1 variable.
//

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_f32x1::operator+
//
//  Synopsis:
//      Add scaled offset to the pointer to UINT32 value.
//
//  Usage example:
//      P_f32x1 pa = ...;
//      C_u32  n = ...;
//      P_f32x1 pc = pa + b;
//
//      Result: pc = pa + n; pa and n unchanged.
//      Note that "n" is implicitly multiplied by sizeof(float).
//
//------------------------------------------------------------------------------
P_f32x1
P_f32x1::operator+(C_u32 const &nIndexDelta) const
{
    P_f32x1 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otPtrPlus, tmp.GetID(), m_ID, nIndexDelta.GetID());
    pOperator->m_refType = RefType_Index_4;
    pOperator->m_uDisplacement = 0;

    return tmp;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      P_f32x1::operator+=
//
//  Synopsis:
//      Add scaled constant offset to the pointer to XmmFloat value.
//
//  Usage example:
//      P_f32x1 pa = ...;
//      pa += 3;
//
//      Result: pa increased by 3.
//      Note that given constant is implicitly multiplied by sizeof(float).
//
//------------------------------------------------------------------------------
P_f32x1 &
P_f32x1::operator+=(int nIndexDelta)
{
    if (nIndexDelta)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otPtrOffset, m_ID, m_ID);
        pOperator->m_uDisplacement = (UINT_PTR)nIndexDelta * sizeof(float);
    }
    return *this;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      P_f32x1::operator++ (preincrement)
//
//  Synopsis:
//      Increment value by 1 (scaled by sizeof(float))
//      
//  Usage example:
//      P_f32x1 pa = ...;
//      P_f32x1 pb = ++pa;
//
//      Result: pa = pb = incremented value of pa. 
//
//------------------------------------------------------------------------------
P_f32x1&
P_f32x1::operator++()
{
    return *this += 1;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_f32x1::operator++ (postincrement)
//
//  Synopsis:
//      Increment value by 1 (scaled by sizeof(float))
//      
//  Usage example:
//      P_f32x1 pa = ...;
//      P_f32x1 pb = pa++;
//
//      Result: pa = incremented value of pa;
//              pb = value of pa before incrementing.
//
//------------------------------------------------------------------------------
P_f32x1
P_f32x1::operator++(int)
{
    P_f32x1 tmp = *this;
    ++*this;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      R_f32x1::R_f32x1
//
//  Synopsis:
//      Construct all-const intance of R_f32x1.
//      This instance is required to resolve expessions
//      like p[n] where p is P_f32x1.
//      When such expression is handled by C++, it's yet unknown whether
//      it'll be used for writing or reading. To bypass this trouble,
//      we create intermediate reference type R_f32x1 which
//      in turn has "operator C_f32x1()" for reading and
//      "operator=(C_f32x1 const& origin)" for writing.
//
//------------------------------------------------------------------------------
R_f32x1::R_f32x1(
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
//      P_f32x1::operator[]
//
//  Synopsis:
//      Creates intermediate reference type to access in-memory variable.
//
//  Usage examples:
//      1.
//      P_f32x1 pa = ...;
//      UINT32  index = ...;
//      C_f32x1 n = pa[index];
//
//      Result: n = pa[index]; pa, index and pa[index] unchanged.
//
//      2.
//      P_f32x1 pa = ...;
//      UINT32  index = ...;
//      C_f32x1 n = ...;
//      pa[index] = n;
//
//      Result: pa[index] = n; n, pa and index unchanged.
//
//      Note that the value of index is implicitly multiplied by sizeof(float).
//------------------------------------------------------------------------------
R_f32x1
P_f32x1::operator[](int nIndex) const
{
    return R_f32x1(RefType_Base, this->GetID(), 0, nIndex*sizeof(float));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      P_f32x1::operator[]
//
//  Synopsis:
//      Creates intermediate reference type to access the element of the array.
//
//------------------------------------------------------------------------------
R_f32x1
P_f32x1::operator[](C_u32 const& index) const
{
    return R_f32x1(RefType_Index_4, this->GetID(), index.GetID(), 0);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      R_f32x1::operator C_f32x1
//
//  Synopsis:
//      Cast reference type R_f32x1 to data type C_f32x1.
//      Treated as fetching data from array.
//
//  Usage example:
//      P_f32x1 pa = ...;
//      UINT32  index = ...;
//      C_f32x1 n = pa[index];
//
//------------------------------------------------------------------------------
R_f32x1::operator C_f32x1() const
{
    C_f32x1 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    if (m_uBaseVarID == 0 && m_uIndexVarID == 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otXmmFloat1Load, tmp.GetID());
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }
    else if (m_uBaseVarID != 0 && m_uIndexVarID == 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otXmmFloat1Load, tmp.GetID(), m_uBaseVarID);
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }
    else if (m_uBaseVarID == 0 && m_uIndexVarID != 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otXmmFloat1Load, tmp.GetID(), m_uIndexVarID);
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }
    else //if (m_uBaseVarID != 0 && m_uIndexVarID != 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otXmmFloat1Load, tmp.GetID(), m_uBaseVarID, m_uIndexVarID);
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      R_f32x1::operator=
//
//  Synopsis:
//      Stores data in an element of array.
//
//  Usage example:
//      P_f32x1 pa = ...;
//      UINT32  index = ...;
//      C_f32x1 n = ...;
//      pa[index] = n;
//------------------------------------------------------------------------------
C_f32x1 const&
R_f32x1::operator=(C_f32x1 const& origin)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    if (m_uBaseVarID == 0 && m_uIndexVarID == 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otXmmFloat1Store, 0, origin.GetID());
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }
    else if (m_uBaseVarID != 0 && m_uIndexVarID == 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otXmmFloat1Store, 0, origin.GetID(), m_uBaseVarID);
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }
    else if (m_uBaseVarID == 0 && m_uIndexVarID != 0)
    {
        SOperator * pOperator = pProgram->AddOperator(otXmmFloat1Store, 0, origin.GetID(), m_uIndexVarID);
        pOperator->m_refType = m_refType;
        pOperator->m_uDisplacement = m_uDisplacement;
    }
    else //if (m_uBaseVarID != 0 && m_uIndexVarID != 0)
    {
        P_u32 tmp;

        SOperator * pOperator1 = pProgram->AddOperator(otPtrPlus, tmp.GetID(), m_uBaseVarID, m_uIndexVarID);
        pOperator1->m_refType = m_refType;
        pOperator1->m_uDisplacement = 0;

        SOperator * pOperator2 = pProgram->AddOperator(otXmmFloat1Store, 0, origin.GetID(), tmp.GetID());
        pOperator2->m_refType = RefType_Base;
        pOperator2->m_uDisplacement = m_uDisplacement;
    }

    return origin;
}


