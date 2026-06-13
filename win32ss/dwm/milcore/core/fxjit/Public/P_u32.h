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
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      R_u32
//
//  Synopsis:
//      Represents a reference to variable of type "C_u32" in prototype
//      program. Serves as intermediate calculation type for P_u32::operator[].
//
//------------------------------------------------------------------------------

class R_u32 : public R_void<R_u32, C_u32>
{
public:
    static const UINT32 IndexShift = 2;
    static const RefType IndexScale = RefType_Index_4;

    R_u32(UINT32 uBaseVarID, UINT32 uIndexVarID, UINT_PTR uDisplacement)
        : R_void<R_u32, C_u32>(uBaseVarID, uIndexVarID, uDisplacement) {}

    operator C_u32() const { return Load(otUINT32Load);}
    C_u32 const& operator=(C_u32 const& origin) const { return Store(origin, otUINT32Store); }

    C_u32 operator+(C_u32 const& src) const {return ((C_u32)(*this)) + src;}
    C_u32 operator-(C_u32 const& src) const {return ((C_u32)(*this)) - src;}
    C_u32 operator*(C_u32 const& src) const {return ((C_u32)(*this)) * src;}
    C_u32 operator/(C_u32 const& src) const {return ((C_u32)(*this)) / src;}
    C_u32 operator%(C_u32 const& src) const {return ((C_u32)(*this)) % src;}
    C_u32 operator&(C_u32 const& src) const {return ((C_u32)(*this)) & src;}
    C_u32 operator|(C_u32 const& src) const {return ((C_u32)(*this)) | src;}
    C_u32 operator^(C_u32 const& src) const {return ((C_u32)(*this)) ^ src;}

    C_u32 operator+(R_u32 const& ref) const {return ((C_u32)(*this)) + ref;}
    C_u32 operator-(R_u32 const& ref) const {return ((C_u32)(*this)) - ref;}
    C_u32 operator*(R_u32 const& ref) const {return ((C_u32)(*this)) * ref;}
    C_u32 operator/(R_u32 const& ref) const {return ((C_u32)(*this)) / ref;}
    C_u32 operator%(R_u32 const& ref) const {return ((C_u32)(*this)) % ref;}
    C_u32 operator&(R_u32 const& ref) const {return ((C_u32)(*this)) & ref;}
    C_u32 operator|(R_u32 const& ref) const {return ((C_u32)(*this)) | ref;}
    C_u32 operator^(R_u32 const& ref) const {return ((C_u32)(*this)) ^ ref;}
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      P_u32
//
//  Synopsis:
//      Represents variable of type "UINT32*" in prototype program.
//
//------------------------------------------------------------------------------
class P_u32 : public TIndexer<P_u32, R_u32>
{
public:
    P_u32() {}
    P_u32(void * pOrigin) : TIndexer<P_u32, R_u32>(pOrigin) {}
};

