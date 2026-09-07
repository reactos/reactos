// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to u32x2 variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_u32x2
//
//  Synopsis:
//      Represents a reference to variable of type "C_u32x2" in prototype
//      program. Serves as intermediate calculation type for P_u32x2::operator[].
//
//------------------------------------------------------------------------------

class R_u32x2
{
public:
    static const UINT32 IndexShift = 3;
    static const RefType IndexScale = RefType_Index_8;

    R_u32x2(
        RefType refType,
        UINT32 uBaseVarID,
        UINT32 uIndexVarID,
        UINT_PTR uDisplacement
        );

    operator C_u32x2() const;
    C_u32x2 const& operator=(C_u32x2 const& origin);

    C_u32x2 operator+(C_u32x2 const& src) const {return ((C_u32x2)(*this)) + src;}
    C_u32x2 operator-(C_u32x2 const& src) const {return ((C_u32x2)(*this)) - src;}
    C_u32x2 operator&(C_u32x2 const& src) const {return ((C_u32x2)(*this)) & src;}
    C_u32x2 operator|(C_u32x2 const& src) const {return ((C_u32x2)(*this)) | src;}
    C_u32x2 operator^(C_u32x2 const& src) const {return ((C_u32x2)(*this)) ^ src;}

    C_u32x2 operator+(R_u32x2 const& ref) const {return ((C_u32x2)(*this)) + ref;}
    C_u32x2 operator-(R_u32x2 const& ref) const {return ((C_u32x2)(*this)) - ref;}
    C_u32x2 operator&(R_u32x2 const& ref) const {return ((C_u32x2)(*this)) & ref;}
    C_u32x2 operator|(R_u32x2 const& ref) const {return ((C_u32x2)(*this)) | ref;}
    C_u32x2 operator^(R_u32x2 const& ref) const {return ((C_u32x2)(*this)) ^ ref;}

private:
    friend class C_u32x2;
    friend class P_u32x2;

private:
    RefType const m_refType;
    UINT32 const m_uBaseVarID;
    UINT32 const m_uIndexVarID;
    UINT_PTR const m_uDisplacement;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_u32x2
//
//  Synopsis:
//      Represents variable of type "u32x2*" in prototype program.
//
//------------------------------------------------------------------------------
class P_u32x2 : public TIndexer<P_u32x2, R_u32x2>
{
public:
    P_u32x2() {}
    P_u32x2(void * pOrigin) : TIndexer<P_u32x2, R_u32x2>(pOrigin) {}
};

