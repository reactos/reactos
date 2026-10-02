// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent pointer to u8x16 variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_u8x16
//
//  Synopsis:
//      Represents a reference to variable of type "C_u8x16" in prototype
//      program. Serves as intermediate calculation type for P_u8x16::operator[].
//
//------------------------------------------------------------------------------

class R_u8x16
{
public:
    static const UINT32 IndexShift = 4;

    R_u8x16(
        RefType refType,
        UINT32 uBaseVarID,
        UINT32 uIndexVarID,
        UINT_PTR uDisplacement
        );

    operator C_u8x16() const;
    C_u8x16 const& operator=(C_u8x16 const& origin);

    C_u8x16 operator+(C_u8x16 const& src) const {return ((C_u8x16)(*this)) + src;}
    C_u8x16 operator-(C_u8x16 const& src) const {return ((C_u8x16)(*this)) - src;}
    C_u8x16 operator&(C_u8x16 const& src) const {return ((C_u8x16)(*this)) & src;}
    C_u8x16 operator|(C_u8x16 const& src) const {return ((C_u8x16)(*this)) | src;}
    C_u8x16 operator^(C_u8x16 const& src) const {return ((C_u8x16)(*this)) ^ src;}

    C_u8x16 operator+(R_u8x16 const& ref) const {return ((C_u8x16)(*this)) + ref;}
    C_u8x16 operator-(R_u8x16 const& ref) const {return ((C_u8x16)(*this)) - ref;}
    C_u8x16 operator&(R_u8x16 const& ref) const {return ((C_u8x16)(*this)) & ref;}
    C_u8x16 operator|(R_u8x16 const& ref) const {return ((C_u8x16)(*this)) | ref;}
    C_u8x16 operator^(R_u8x16 const& ref) const {return ((C_u8x16)(*this)) ^ ref;}

private:
    friend class C_u8x16;
    friend class P_u8x16;

private:
    RefType const m_refType;
    UINT32 const m_uBaseVarID;
    UINT32 const m_uIndexVarID;
    UINT_PTR const m_uDisplacement;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_u8x16
//
//  Synopsis:
//      Represents variable of type "u8x16*" in prototype program.
//
//------------------------------------------------------------------------------
class P_u8x16 : public TIndexer<P_u8x16, R_u8x16>
{
public:
    P_u8x16() {}
    P_u8x16(void * pOrigin) : TIndexer<P_u8x16, R_u8x16>(pOrigin) {}
};

