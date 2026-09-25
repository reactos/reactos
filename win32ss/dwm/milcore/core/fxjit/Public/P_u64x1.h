// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to 64-bit MMX variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_u64x1
//
//  Synopsis:
//      Represents a reference to variable of type "C_u64x1" in prototype
//      program. Serves as intermediate calculation type for P_u64x1::operator[].
//
//------------------------------------------------------------------------------

class R_u64x1
{
public:
    static const UINT32 IndexShift = 3;
    static const RefType IndexScale = RefType_Index_8;

    R_u64x1(
        RefType refType,
        UINT32 uBaseVarID,
        UINT32 uIndexVarID,
        UINT_PTR uDisplacement
        );

    operator C_u64x1() const;
    C_u64x1 const& operator=(C_u64x1 const& origin);

    C_u64x1 operator+(C_u64x1 const& src) const {return ((C_u64x1)(*this)) + src;}
    C_u64x1 operator-(C_u64x1 const& src) const {return ((C_u64x1)(*this)) - src;}
    C_u64x1 operator&(C_u64x1 const& src) const {return ((C_u64x1)(*this)) & src;}
    C_u64x1 operator|(C_u64x1 const& src) const {return ((C_u64x1)(*this)) | src;}
    C_u64x1 operator^(C_u64x1 const& src) const {return ((C_u64x1)(*this)) ^ src;}

    C_u64x1 operator+(R_u64x1 const& ref) const {return ((C_u64x1)(*this)) + ref;}
    C_u64x1 operator-(R_u64x1 const& ref) const {return ((C_u64x1)(*this)) - ref;}
    C_u64x1 operator&(R_u64x1 const& ref) const {return ((C_u64x1)(*this)) & ref;}
    C_u64x1 operator|(R_u64x1 const& ref) const {return ((C_u64x1)(*this)) | ref;}
    C_u64x1 operator^(R_u64x1 const& ref) const {return ((C_u64x1)(*this)) ^ ref;}

private:
    friend class C_u64x1;
    friend class P_u64x1;

private:
    RefType const m_refType;
    UINT32 const m_uBaseVarID;
    UINT32 const m_uIndexVarID;
    UINT_PTR const m_uDisplacement;
};
//+-----------------------------------------------------------------------------
//
//  Class:
//      P_u64x1
//
//  Synopsis:
//      Represents a pointer to integer 64-bits value in prototype program.
//
//------------------------------------------------------------------------------
class P_u64x1 : public TIndexer<P_u64x1, R_u64x1>
{
public:
    P_u64x1() {}
    P_u64x1(void * pOrigin) : TIndexer<P_u64x1, R_u64x1>(pOrigin) {}
};


