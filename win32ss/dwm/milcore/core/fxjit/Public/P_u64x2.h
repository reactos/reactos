// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to u64x2 variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_u64x2
//
//  Synopsis:
//      Represents a reference to variable of type "C_u64x2" in prototype
//      program. Serves as intermediate calculation type for P_u64x2::operator[].
//
//------------------------------------------------------------------------------

class R_u64x2
{
public:
    static const UINT32 IndexShift = 4;

    R_u64x2(
        RefType refType,
        UINT32 uBaseVarID,
        UINT32 uIndexVarID,
        UINT_PTR uDisplacement
        );

    operator C_u64x2() const;
    C_u64x2 const& operator=(C_u64x2 const& origin);

    C_u64x2 operator+(C_u64x2 const& src) const {return ((C_u64x2)(*this)) + src;}
    C_u64x2 operator-(C_u64x2 const& src) const {return ((C_u64x2)(*this)) - src;}
    C_u64x2 operator&(C_u64x2 const& src) const {return ((C_u64x2)(*this)) & src;}
    C_u64x2 operator|(C_u64x2 const& src) const {return ((C_u64x2)(*this)) | src;}
    C_u64x2 operator^(C_u64x2 const& src) const {return ((C_u64x2)(*this)) ^ src;}

    C_u64x2 operator+(R_u64x2 const& ref) const {return ((C_u64x2)(*this)) + ref;}
    C_u64x2 operator-(R_u64x2 const& ref) const {return ((C_u64x2)(*this)) - ref;}
    C_u64x2 operator&(R_u64x2 const& ref) const {return ((C_u64x2)(*this)) & ref;}
    C_u64x2 operator|(R_u64x2 const& ref) const {return ((C_u64x2)(*this)) | ref;}
    C_u64x2 operator^(R_u64x2 const& ref) const {return ((C_u64x2)(*this)) ^ ref;}

private:
    friend class C_u64x2;
    friend class P_u64x2;

private:
    RefType const m_refType;
    UINT32 const m_uBaseVarID;
    UINT32 const m_uIndexVarID;
    UINT_PTR const m_uDisplacement;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_u64x2
//
//  Synopsis:
//      Represents variable of type "u64x2*" in prototype program.
//
//------------------------------------------------------------------------------
class P_u64x2 : public TIndexer<P_u64x2, R_u64x2>
{
public:
    P_u64x2() {}
    P_u64x2(void * pOrigin) : TIndexer<P_u64x2, R_u64x2>(pOrigin) {}
};

